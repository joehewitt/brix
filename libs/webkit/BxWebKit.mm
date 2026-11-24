#include "include/BxWebKit.h"
#include "BxAppleGUI/BxAppleGUI.h"
#include "BxCore/BxCore.h"
#include "BxEvents/BxEvents.h"

#import <AppKit/AppKit.h>
#import <WebKit/WebKit.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// **********************************************************************************************

enum : uint32_t {
    kWebViewStateChanged = 0,
    kWebViewFocused = 1,
    kWebViewNavigationRequested = 2,
    kWebViewLoadFinished = 3,
    kWebViewLoadFailed = 4,
    kWebViewScriptMessage = 5,
};

static const NSUInteger kWebViewKeyEventMemory = 32;
static void* webViewObservationContext = &webViewObservationContext;
static NSString* const webViewObservedKeys[] = {
    @"URL",
    @"title",
    @"loading",
    @"estimatedProgress",
    @"canGoBack",
    @"canGoForward",
};
static std::string webViewReadBuffer;
static NSString* const webViewContentWorldName = @"brix";
static NSString* const webViewContentHeightHandler = @"contentHeight";
static NSString* const webViewContentHeightStop = @"window.brixContentHeight?.stop()";
static NSString* const webViewContentHeightScript =
    @"(() => {"
    "  const root = document.documentElement;"
    "  if (window.brixContentHeight || !root) return;"
    "  const style = document.createElement('style');"
    "  const body = document.compatMode === 'BackCompat' ? 'fit-content' : 'auto';"
    "  style.textContent = 'html { height: fit-content !important; min-height: 0 !important; } "
    "body { height: ' + body + ' !important; min-height: 0 !important; }';"
    "  (document.head || root).appendChild(style);"
    "  let last = -1;"
    "  let step = 0;"
    "  let repeats = 0;"
    "  const report = () => {"
    "    const height = Math.ceil(root.getBoundingClientRect().height);"
    "    if (height === last) return;"
    "    const chasing = last >= 0 && height > last && window.innerHeight === last;"
    "    repeats = chasing && height - last === step ? repeats + 1 : 0;"
    "    step = chasing ? height - last : 0;"
    "    if (repeats >= 3) return;"
    "    last = height;"
    "    window.webkit.messageHandlers.contentHeight.postMessage(height);"
    "  };"
    "  const observer = new ResizeObserver(report);"
    "  observer.observe(root);"
    "  window.brixContentHeight = {"
    "    stop() { observer.disconnect(); style.remove(); delete window.brixContentHeight; }"
    "  };"
    "  report();"
    "})();";

static NSString*
webViewString(const char* value) {
    if (!value) {
        return @"";
    }
    NSString* string = [NSString stringWithUTF8String:value];
    return string ?: @"";
}

static std::string
webViewUTF8(NSString* value) {
    const char* text = [value UTF8String];
    return text ? std::string(text) : std::string();
}

static NSString*
webViewJSONText(id value) {
    if (!value) {
        return @"null";
    }
    NSArray* wrapped = @[value];
    if (![NSJSONSerialization isValidJSONObject:wrapped]) {
        return @"null";
    }
    NSData* data = [NSJSONSerialization dataWithJSONObject:wrapped
                                                   options:NSJSONWritingWithoutEscapingSlashes
                                                     error:nil];
    NSString* text = data
        ? [[[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding] autorelease]
        : nil;
    if ([text length] < 2) {
        return @"null";
    }
    return [text substringWithRange:NSMakeRange(1, [text length] - 2)];
}

static NSString*
webViewStyleScript(NSString* css) {
    return [NSString stringWithFormat:
        @"(() => {"
        "  const css = %@;"
        "  let style = window.brixStyle;"
        "  if (!css) {"
        "    style?.remove();"
        "    delete window.brixStyle;"
        "    return;"
        "  }"
        "  if (!style) {"
        "    style = window.brixStyle = document.createElement('style');"
        "  }"
        "  style.textContent = css;"
        "  if (!style.isConnected) {"
        "    const head = document.head;"
        "    head ? head.prepend(style) : document.documentElement.appendChild(style);"
        "  }"
        "})();",
        webViewJSONText(css)];
}

static uint32_t
webViewNavigationDetail(WKNavigationAction* action) {
    uint32_t kind = 5;
    switch ([action navigationType]) {
        case WKNavigationTypeLinkActivated: kind = 0; break;
        case WKNavigationTypeFormSubmitted: kind = 1; break;
        case WKNavigationTypeBackForward: kind = 2; break;
        case WKNavigationTypeReload: kind = 3; break;
        case WKNavigationTypeFormResubmitted: kind = 4; break;
        case WKNavigationTypeOther: kind = 5; break;
    }
    return [action targetFrame] ? kind : kind | 0x100;
}

static NSString*
webViewScriptErrorText(NSError* error) {
    id message = [[error userInfo] objectForKey:@"WKJavaScriptExceptionMessage"];
    return [message isKindOfClass:[NSString class]] ? message : [error localizedDescription];
}

static SEL
webViewEditAction(NSEvent* event) {
    NSEventModifierFlags flags = [event modifierFlags] & (NSEventModifierFlagCommand |
        NSEventModifierFlagShift | NSEventModifierFlagOption | NSEventModifierFlagControl);
    NSString* key = [[event charactersIgnoringModifiers] lowercaseString];
    if (flags == NSEventModifierFlagCommand) {
        if ([key isEqualToString:@"c"]) return @selector(copy:);
        if ([key isEqualToString:@"x"]) return @selector(cut:);
        if ([key isEqualToString:@"v"]) return @selector(paste:);
        if ([key isEqualToString:@"a"]) return @selector(selectAll:);
        if ([key isEqualToString:@"z"]) return @selector(undo:);
    }
    if (flags == (NSEventModifierFlagCommand | NSEventModifierFlagShift) &&
        [key isEqualToString:@"z"]) {
        return @selector(redo:);
    }
    return nullptr;
}

// **********************************************************************************************

struct BxWebViewShared {
    bool statePending = false;
};

struct BxWebViewEventRecord {
    BxWebViewEventRecord(
        BxWebViewEventCallback aCallback,
        void* aContext,
        std::shared_ptr<BxWebViewShared> aShared,
        uint32_t aKind,
        uint64_t aRequestId,
        uint32_t aDetail,
        NSString* aFirst,
        NSString* aSecond):
        callback(aCallback),
        context(aContext),
        contextRoot(aContext ? brixAddRoot(static_cast<char*>(aContext)) : nullptr),
        shared(std::move(aShared)),
        kind(aKind),
        requestId(aRequestId),
        detail(aDetail),
        first(webViewUTF8(aFirst)),
        second(webViewUTF8(aSecond))
    {}

    ~BxWebViewEventRecord() {
        if (contextRoot) {
            brixRemoveRoot(contextRoot);
        }
    }

    BxWebViewEventCallback callback;
    void* context;
    void* contextRoot;
    std::shared_ptr<BxWebViewShared> shared;
    uint32_t kind;
    uint64_t requestId;
    uint32_t detail;
    std::string first;
    std::string second;
};

static void
webViewRunEventCallback(void* closure) {
    BxWebViewEventRecord* record = static_cast<BxWebViewEventRecord*>(closure);
    if (record->kind == kWebViewStateChanged) {
        record->shared->statePending = false;
    }
    if (!brixProcessTerminationRequested()) {
        record->callback(
            record->context,
            record->kind,
            record->requestId,
            record->detail,
            record->first.c_str(),
            record->second.c_str());
    }
    delete record;
}

struct BxWebViewScriptState {
    BxWebViewScriptState(BxWebViewScriptCallback aCallback, void* aContext):
        callback(aCallback),
        context(aContext),
        contextRoot(aContext ? brixAddRoot(static_cast<char*>(aContext)) : nullptr),
        finished(false)
    {}

    ~BxWebViewScriptState() {
        if (contextRoot) {
            brixRemoveRoot(contextRoot);
        }
    }

    BxWebViewScriptCallback callback;
    void* context;
    void* contextRoot;
    bool finished;
};

struct BxWebViewScriptRecord {
    BxWebViewScriptRecord(BxWebViewScriptState& state, bool aSucceeded, NSString* aText):
        callback(state.callback),
        context(state.context),
        contextRoot(state.contextRoot),
        succeeded(aSucceeded),
        text(webViewUTF8(aText))
    {
        state.contextRoot = nullptr;
    }

    ~BxWebViewScriptRecord() {
        if (contextRoot) {
            brixRemoveRoot(contextRoot);
        }
    }

    BxWebViewScriptCallback callback;
    void* context;
    void* contextRoot;
    bool succeeded;
    std::string text;
};

static void
webViewRunScriptCallback(void* closure) {
    BxWebViewScriptRecord* record = static_cast<BxWebViewScriptRecord*>(closure);
    if (!brixProcessTerminationRequested()) {
        record->callback(record->context, record->succeeded, record->text.c_str());
    }
    delete record;
}

static void
webViewFinishScript(const std::shared_ptr<BxWebViewScriptState>& state, bool succeeded, NSString* text) {
    if (state->finished) {
        return;
    }
    state->finished = true;
    if (!state->callback) {
        return;
    }
    BxWebViewScriptRecord* record = new BxWebViewScriptRecord(*state, succeeded, text);
    if (!brixScheduleMainTaskFromNative(webViewRunScriptCallback, record)) {
        delete record;
    }
}

// **********************************************************************************************

@class BxWebViewHost;

@interface BxWebViewControl : WKWebView

@property(nonatomic, assign) BxWebViewHost* webHost;

- (void)performEditAction:(SEL)action;

@end

@interface BxWebViewMessageProxy : NSObject <WKScriptMessageHandler>

@property(nonatomic, assign) BxWebViewHost* webHost;

@end

@interface BxWebViewHost : NSObject <WKNavigationDelegate, WKUIDelegate> {
    std::shared_ptr<BxWebViewShared> shared;
    std::vector<std::shared_ptr<BxWebViewScriptState>> scripts;
}

@property(nonatomic, retain) BxWebViewControl* control;
@property(nonatomic, retain) BxWebViewMessageProxy* messageProxy;
@property(nonatomic, retain) NSMutableArray* messageNames;
@property(nonatomic, retain) NSMutableDictionary* pendingDecisions;
@property(nonatomic, retain) NSMutableArray* keyEvents;
@property(nonatomic, assign) BxWebViewEventCallback callback;
@property(nonatomic, assign) void* context;
@property(nonatomic, assign) void* contextRoot;
@property(nonatomic, assign) uint64_t nextRequestId;
@property(nonatomic, assign) BOOL observing;
@property(nonatomic, assign) BOOL fitsContent;
@property(nonatomic, assign) double contentHeight;
@property(nonatomic, copy) NSString* userStyle;
@property(nonatomic, copy) NSString* userScript;
@property(nonatomic, copy) NSString* preapprovedURL;

- (instancetype)initEphemeral:(BOOL)ephemeral;

- (BOOL)sendEventKind:(uint32_t)kind
            requestId:(uint64_t)requestId
               detail:(uint32_t)detail
                first:(NSString*)first
               second:(NSString*)second;
- (BOOL)sendEventKind:(uint32_t)kind first:(NSString*)first second:(NSString*)second;
- (void)scheduleState;
- (void)rebuildUserScripts;
- (void)applyLiveScript:(NSString*)script;
- (void)trackScript:(const std::shared_ptr<BxWebViewScriptState>&)state;
- (BOOL)isReturningKeyEvent:(NSEvent*)event;
- (void)noteKeyEvent:(NSEvent*)event;
- (void)forgetKeyEvent:(NSEvent*)event;
- (void)clearMessageHandlers;
- (void)acceptContentHeight:(id)body;
- (void)detach;
- (void)clearEventCallback;

@end

// **********************************************************************************************

@implementation BxWebViewControl

- (BOOL)becomeFirstResponder {
    BOOL accepted = [super becomeFirstResponder];
    if (accepted) {
        [self.webHost sendEventKind:kWebViewFocused first:nil second:nil];
    }
    return accepted;
}

- (NSView*)nextValidKeyView {
    return nil;
}

- (NSView*)previousValidKeyView {
    return nil;
}

- (BOOL)performKeyEquivalent:(NSEvent*)event {
    if ([self.webHost isReturningKeyEvent:event]) {
        SEL action = webViewEditAction(event);
        if (!action) {
            return NO;
        }
        [self.webHost forgetKeyEvent:event];
        [self performEditAction:action];
        return YES;
    }
    BOOL handled = [super performKeyEquivalent:event];
    if (handled && [[self window] firstResponder] == self) {
        [self.webHost noteKeyEvent:event];
    }
    return handled;
}

- (void)keyDown:(NSEvent*)event {
    BxWebViewHost* host = self.webHost;
    if ([host isReturningKeyEvent:event]) {
        [host forgetKeyEvent:event];
        if ([event modifierFlags] & NSEventModifierFlagCommand) {
            brixGuiPeerDispatchKeyEvent((__bridge void*)host, (__bridge void*)event);
        }
        return;
    }
    [host noteKeyEvent:event];
    [super keyDown:event];
}

- (void)performEditAction:(SEL)action {
    if (action == @selector(undo:) || action == @selector(redo:)) {
        NSUndoManager* manager = [self undoManager];
        if (action == @selector(undo:) && [manager canUndo]) {
            [manager undo];
        } else if (action == @selector(redo:) && [manager canRedo]) {
            [manager redo];
        }
        return;
    }
    [self tryToPerform:action with:self];
}

@end

// **********************************************************************************************

@implementation BxWebViewMessageProxy

- (void)userContentController:(WKUserContentController*)controller
      didReceiveScriptMessage:(WKScriptMessage*)message {
    if ([[[message world] name] isEqualToString:webViewContentWorldName]) {
        if ([[message name] isEqualToString:webViewContentHeightHandler]) {
            [self.webHost acceptContentHeight:[message body]];
        }
        return;
    }
    [self.webHost sendEventKind:kWebViewScriptMessage
                          first:[message name]
                         second:webViewJSONText([message body])];
}

@end

// **********************************************************************************************

@implementation BxWebViewHost

- (instancetype)initEphemeral:(BOOL)ephemeral {
    self = [super init];
    if (!self) {
        return nil;
    }
    shared = std::make_shared<BxWebViewShared>();

    WKWebViewConfiguration* configuration = [[[WKWebViewConfiguration alloc] init] autorelease];
    if (ephemeral) {
        configuration.websiteDataStore = [WKWebsiteDataStore nonPersistentDataStore];
    }
    BxWebViewControl* control = [[[BxWebViewControl alloc] initWithFrame:NSZeroRect
                                                           configuration:configuration] autorelease];
    control.webHost = self;
    control.navigationDelegate = self;
    control.UIDelegate = self;
    self.control = control;

    BxWebViewMessageProxy* proxy = [[[BxWebViewMessageProxy alloc] init] autorelease];
    proxy.webHost = self;
    self.messageProxy = proxy;
    self.messageNames = [NSMutableArray array];
    self.pendingDecisions = [NSMutableDictionary dictionary];
    self.keyEvents = [NSMutableArray array];
    self.contentHeight = -1;

    for (NSString* key : webViewObservedKeys) {
        [control addObserver:self forKeyPath:key options:0 context:webViewObservationContext];
    }
    self.observing = YES;
    return self;
}

- (void)dealloc {
    [self detach];
    self.messageProxy = nil;
    self.messageNames = nil;
    self.pendingDecisions = nil;
    self.keyEvents = nil;
    self.userStyle = nil;
    self.userScript = nil;
    self.preapprovedURL = nil;
    [super dealloc];
}

- (void)detach {
    BxWebViewControl* control = self.control;
    if (!control) {
        return;
    }
    if (self.observing) {
        for (NSString* key : webViewObservedKeys) {
            [control removeObserver:self forKeyPath:key context:webViewObservationContext];
        }
        self.observing = NO;
    }
    WKUserContentController* controller = [[control configuration] userContentController];
    [controller removeAllScriptMessageHandlers];
    [controller removeAllUserScripts];
    [self.messageNames removeAllObjects];
    self.fitsContent = NO;
    self.messageProxy.webHost = nil;

    NSDictionary* decisions = [[self.pendingDecisions copy] autorelease];
    [self.pendingDecisions removeAllObjects];
    for (NSNumber* requestId in decisions) {
        void (^decide)(WKNavigationActionPolicy) = [decisions objectForKey:requestId];
        decide(WKNavigationActionPolicyCancel);
    }

    std::vector<std::shared_ptr<BxWebViewScriptState>> pending;
    pending.swap(scripts);
    for (const std::shared_ptr<BxWebViewScriptState>& state : pending) {
        webViewFinishScript(state, false, @"The web view was closed");
    }

    [control stopLoading];
    control.navigationDelegate = nil;
    control.UIDelegate = nil;
    control.webHost = nil;
    [control removeFromSuperview];
    [self.keyEvents removeAllObjects];

    self.callback = nullptr;
    self.context = nullptr;
    if (self.contextRoot) {
        brixRemoveRoot(self.contextRoot);
        self.contextRoot = nullptr;
    }
    self.control = nil;
}

- (void)clearEventCallback {
    NSDictionary* decisions = [[self.pendingDecisions copy] autorelease];
    [self.pendingDecisions removeAllObjects];
    for (NSNumber* requestId in decisions) {
        void (^decide)(WKNavigationActionPolicy) = [decisions objectForKey:requestId];
        decide(WKNavigationActionPolicyAllow);
    }
    self.callback = nullptr;
    self.context = nullptr;
    if (self.contextRoot) {
        brixRemoveRoot(self.contextRoot);
        self.contextRoot = nullptr;
    }
}

- (BOOL)sendEventKind:(uint32_t)kind
            requestId:(uint64_t)requestId
               detail:(uint32_t)detail
                first:(NSString*)first
               second:(NSString*)second {
    if (!self.callback) {
        return NO;
    }
    BxWebViewEventRecord* record = new BxWebViewEventRecord(
        self.callback, self.context, shared, kind, requestId, detail, first, second);
    if (!brixScheduleMainTaskFromNative(webViewRunEventCallback, record)) {
        delete record;
        return NO;
    }
    return YES;
}

- (BOOL)sendEventKind:(uint32_t)kind first:(NSString*)first second:(NSString*)second {
    return [self sendEventKind:kind requestId:0 detail:0 first:first second:second];
}

- (void)scheduleState {
    if (shared->statePending) {
        return;
    }
    shared->statePending = true;
    if (![self sendEventKind:kWebViewStateChanged first:nil second:nil]) {
        shared->statePending = false;
    }
}

- (void)observeValueForKeyPath:(NSString*)keyPath
                      ofObject:(id)object
                        change:(NSDictionary*)change
                       context:(void*)context {
    if (context != webViewObservationContext) {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
        return;
    }
    [self scheduleState];
}

- (void)trackScript:(const std::shared_ptr<BxWebViewScriptState>&)state {
    scripts.erase(
        std::remove_if(scripts.begin(), scripts.end(), [](const std::shared_ptr<BxWebViewScriptState>& item) {
            return item->finished;
        }),
        scripts.end());
    scripts.push_back(state);
}

- (BOOL)isReturningKeyEvent:(NSEvent*)event {
    return [self.keyEvents indexOfObjectIdenticalTo:event] != NSNotFound;
}

- (void)noteKeyEvent:(NSEvent*)event {
    if ([self.keyEvents count] >= kWebViewKeyEventMemory) {
        [self.keyEvents removeObjectAtIndex:0];
    }
    [self.keyEvents addObject:event];
}

- (void)forgetKeyEvent:(NSEvent*)event {
    NSUInteger index = [self.keyEvents indexOfObjectIdenticalTo:event];
    if (index != NSNotFound) {
        [self.keyEvents removeObjectAtIndex:index];
    }
}

- (void)clearMessageHandlers {
    WKUserContentController* controller = [[self.control configuration] userContentController];
    for (NSString* name in self.messageNames) {
        [controller removeScriptMessageHandlerForName:name];
    }
    [self.messageNames removeAllObjects];
}

- (void)rebuildUserScripts {
    WKUserContentController* controller = [[self.control configuration] userContentController];
    WKContentWorld* world = [WKContentWorld worldWithName:webViewContentWorldName];
    [controller removeAllUserScripts];
    if ([self.userStyle length]) {
        [controller addUserScript:[[[WKUserScript alloc] initWithSource:webViewStyleScript(self.userStyle)
                                                         injectionTime:WKUserScriptInjectionTimeAtDocumentStart
                                                      forMainFrameOnly:YES
                                                        inContentWorld:world] autorelease]];
    }
    if ([self.userScript length]) {
        [controller addUserScript:[[[WKUserScript alloc] initWithSource:self.userScript
                                                         injectionTime:WKUserScriptInjectionTimeAtDocumentStart
                                                      forMainFrameOnly:YES
                                                        inContentWorld:[WKContentWorld pageWorld]] autorelease]];
    }
    if (self.fitsContent) {
        [controller addUserScript:[[[WKUserScript alloc] initWithSource:webViewContentHeightScript
                                                         injectionTime:WKUserScriptInjectionTimeAtDocumentEnd
                                                      forMainFrameOnly:YES
                                                        inContentWorld:world] autorelease]];
    }
}

- (void)applyLiveScript:(NSString*)script {
    if (![self.control URL]) {
        return;
    }
    [self.control evaluateJavaScript:script
                             inFrame:nil
                      inContentWorld:[WKContentWorld worldWithName:webViewContentWorldName]
                   completionHandler:nil];
}

- (void)acceptContentHeight:(id)body {
    if (!self.fitsContent || ![body isKindOfClass:[NSNumber class]]) {
        return;
    }
    double height = [body doubleValue];
    if (height == self.contentHeight) {
        return;
    }
    self.contentHeight = height;
    [self scheduleState];
}

- (void)reportLoadError:(NSError*)error {
    if ([[error domain] isEqualToString:NSURLErrorDomain] && [error code] == NSURLErrorCancelled) {
        return;
    }
    if ([[error domain] isEqualToString:@"WebKitErrorDomain"] && [error code] == 102) {
        return;
    }
    id failing = [[error userInfo] objectForKey:NSURLErrorFailingURLErrorKey];
    NSString* url = [failing isKindOfClass:[NSURL class]]
        ? [static_cast<NSURL*>(failing) absoluteString]
        : nil;
    [self sendEventKind:kWebViewLoadFailed
                  first:url ?: [[self.control URL] absoluteString]
                 second:[error localizedDescription]];
}

- (NSAlert*)alertWithMessage:(NSString*)message {
    NSAlert* alert = [[[NSAlert alloc] init] autorelease];
    [alert setMessageText:message ?: @""];
    return alert;
}

// **********************************************************************************************

- (void)webView:(WKWebView*)webView
    decidePolicyForNavigationAction:(WKNavigationAction*)action
                    decisionHandler:(void (^)(WKNavigationActionPolicy))decisionHandler {
    WKFrameInfo* target = [action targetFrame];
    NSString* url = [[[action request] URL] absoluteString];
    if (target && [target isMainFrame] && self.preapprovedURL) {
        BOOL preapproved = [url isEqualToString:self.preapprovedURL];
        self.preapprovedURL = nil;
        if (preapproved) {
            decisionHandler(WKNavigationActionPolicyAllow);
            return;
        }
    }
    if ((target && ![target isMainFrame]) || !self.callback) {
        decisionHandler(WKNavigationActionPolicyAllow);
        return;
    }
    uint64_t requestId = self.nextRequestId + 1;
    self.nextRequestId = requestId;
    [self.pendingDecisions setObject:[[decisionHandler copy] autorelease] forKey:@(requestId)];
    BOOL sent = [self sendEventKind:kWebViewNavigationRequested
                          requestId:requestId
                             detail:webViewNavigationDetail(action)
                              first:url
                             second:nil];
    if (!sent) {
        [self.pendingDecisions removeObjectForKey:@(requestId)];
        decisionHandler(WKNavigationActionPolicyAllow);
    }
}

- (void)webView:(WKWebView*)webView didFinishNavigation:(WKNavigation*)navigation {
    [self sendEventKind:kWebViewLoadFinished first:[[webView URL] absoluteString] second:nil];
}

- (void)webView:(WKWebView*)webView
    didFailProvisionalNavigation:(WKNavigation*)navigation
                       withError:(NSError*)error {
    [self reportLoadError:error];
}

- (void)webView:(WKWebView*)webView didFailNavigation:(WKNavigation*)navigation withError:(NSError*)error {
    [self reportLoadError:error];
}

- (void)webViewWebContentProcessDidTerminate:(WKWebView*)webView {
    [self sendEventKind:kWebViewLoadFailed
                  first:[[webView URL] absoluteString]
                 second:@"The web content process stopped"];
}

- (WKWebView*)webView:(WKWebView*)webView
    createWebViewWithConfiguration:(WKWebViewConfiguration*)configuration
               forNavigationAction:(WKNavigationAction*)action
                    windowFeatures:(WKWindowFeatures*)features {
    self.preapprovedURL = [[[action request] URL] absoluteString];
    [webView loadRequest:[action request]];
    return nil;
}

- (void)webView:(WKWebView*)webView
    runJavaScriptAlertPanelWithMessage:(NSString*)message
                      initiatedByFrame:(WKFrameInfo*)frame
                     completionHandler:(void (^)(void))completionHandler {
    NSWindow* window = [webView window];
    if (!window) {
        completionHandler();
        return;
    }
    NSAlert* alert = [[self alertWithMessage:message] retain];
    [alert addButtonWithTitle:@"OK"];
    [alert beginSheetModalForWindow:window completionHandler:^(NSModalResponse response) {
        [alert release];
        completionHandler();
    }];
}

- (void)webView:(WKWebView*)webView
    runJavaScriptConfirmPanelWithMessage:(NSString*)message
                        initiatedByFrame:(WKFrameInfo*)frame
                       completionHandler:(void (^)(BOOL))completionHandler {
    NSWindow* window = [webView window];
    if (!window) {
        completionHandler(NO);
        return;
    }
    NSAlert* alert = [[self alertWithMessage:message] retain];
    [alert addButtonWithTitle:@"OK"];
    [alert addButtonWithTitle:@"Cancel"];
    [alert beginSheetModalForWindow:window completionHandler:^(NSModalResponse response) {
        [alert release];
        completionHandler(response == NSAlertFirstButtonReturn);
    }];
}

- (void)webView:(WKWebView*)webView
    runJavaScriptTextInputPanelWithPrompt:(NSString*)prompt
                              defaultText:(NSString*)defaultText
                         initiatedByFrame:(WKFrameInfo*)frame
                        completionHandler:(void (^)(NSString*))completionHandler {
    NSWindow* window = [webView window];
    if (!window) {
        completionHandler(nil);
        return;
    }
    NSAlert* alert = [[self alertWithMessage:prompt] retain];
    NSTextField* field = [[[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 260, 24)] autorelease];
    [field setStringValue:defaultText ?: @""];
    [alert setAccessoryView:field];
    [alert addButtonWithTitle:@"OK"];
    [alert addButtonWithTitle:@"Cancel"];
    [[alert window] setInitialFirstResponder:field];
    [alert beginSheetModalForWindow:window completionHandler:^(NSModalResponse response) {
        NSString* text = response == NSAlertFirstButtonReturn ? [field stringValue] : nil;
        [alert release];
        completionHandler(text);
    }];
}

- (void)webView:(WKWebView*)webView
    runOpenPanelWithParameters:(WKOpenPanelParameters*)parameters
              initiatedByFrame:(WKFrameInfo*)frame
             completionHandler:(void (^)(NSArray<NSURL*>*))completionHandler {
    NSWindow* window = [webView window];
    if (!window) {
        completionHandler(nil);
        return;
    }
    NSOpenPanel* panel = [[NSOpenPanel openPanel] retain];
    [panel setCanChooseFiles:YES];
    [panel setCanChooseDirectories:[parameters allowsDirectories]];
    [panel setAllowsMultipleSelection:[parameters allowsMultipleSelection]];
    [panel beginSheetModalForWindow:window completionHandler:^(NSModalResponse result) {
        NSArray<NSURL*>* urls = result == NSModalResponseOK ? [panel URLs] : nil;
        completionHandler(urls);
        [panel release];
    }];
}

@end

// **********************************************************************************************

static BxWebViewHost*
getHost(void* ptr) {
    return (__bridge BxWebViewHost*)ptr;
}

extern "C" void*
brixWebViewCreate(bool ephemeral) {
    BxWebViewHost* host = [[BxWebViewHost alloc] initEphemeral:ephemeral];
    return (__bridge void*)host;
}

extern "C" void
brixWebViewRelease(void* ptr) {
    if (!ptr) {
        return;
    }
    [getHost(ptr) detach];
    CFBridgingRelease(ptr);
}

extern "C" void
brixWebViewSetEventCallback(void* ptr, BxWebViewEventCallback callback, void* context) {
    BxWebViewHost* host = getHost(ptr);
    if (!host || !host.control) {
        return;
    }
    void* newRoot = context ? brixAddRoot(static_cast<char*>(context)) : nullptr;
    void* oldRoot = host.contextRoot;
    host.callback = callback;
    host.context = context;
    host.contextRoot = newRoot;
    if (oldRoot) {
        brixRemoveRoot(oldRoot);
    }
    [host scheduleState];
}

extern "C" void
brixWebViewClearEventCallback(void* ptr) {
    BxWebViewHost* host = getHost(ptr);
    if (host) {
        [host clearEventCallback];
    }
}

extern "C" bool
brixWebViewLoadURL(void* ptr, const char* url) {
    BxWebViewHost* host = getHost(ptr);
    NSURL* value = [NSURL URLWithString:webViewString(url)];
    if (!host.control || !value || ![[value scheme] length]) {
        return false;
    }
    if ([value isFileURL]) {
        [host.control loadFileURL:value allowingReadAccessToURL:[value URLByDeletingLastPathComponent]];
    } else {
        [host.control loadRequest:[NSURLRequest requestWithURL:value]];
    }
    return true;
}

extern "C" void
brixWebViewLoadHTML(void* ptr, const char* html, const char* baseURL) {
    BxWebViewHost* host = getHost(ptr);
    NSString* base = webViewString(baseURL);
    [host.control loadHTMLString:webViewString(html) baseURL:[base length] ? [NSURL URLWithString:base] : nil];
}

extern "C" void
brixWebViewGoBack(void* ptr) {
    [getHost(ptr).control goBack];
}

extern "C" void
brixWebViewGoForward(void* ptr) {
    [getHost(ptr).control goForward];
}

extern "C" void
brixWebViewReload(void* ptr) {
    [getHost(ptr).control reload];
}

extern "C" void
brixWebViewStop(void* ptr) {
    [getHost(ptr).control stopLoading];
}

extern "C" const char*
brixWebViewGetURL(void* ptr) {
    webViewReadBuffer = webViewUTF8([[getHost(ptr).control URL] absoluteString]);
    return webViewReadBuffer.c_str();
}

extern "C" const char*
brixWebViewGetTitle(void* ptr) {
    webViewReadBuffer = webViewUTF8([getHost(ptr).control title]);
    return webViewReadBuffer.c_str();
}

extern "C" bool
brixWebViewIsLoading(void* ptr) {
    return [getHost(ptr).control isLoading];
}

extern "C" double
brixWebViewGetProgress(void* ptr) {
    return [getHost(ptr).control estimatedProgress];
}

extern "C" bool
brixWebViewCanGoBack(void* ptr) {
    return [getHost(ptr).control canGoBack];
}

extern "C" bool
brixWebViewCanGoForward(void* ptr) {
    return [getHost(ptr).control canGoForward];
}

extern "C" double
brixWebViewGetContentHeight(void* ptr) {
    return getHost(ptr).contentHeight;
}

extern "C" void
brixWebViewSetFitsContent(void* ptr, bool fitsContent) {
    BxWebViewHost* host = getHost(ptr);
    if (!host.control || host.fitsContent == fitsContent) {
        return;
    }
    host.fitsContent = fitsContent;
    WKUserContentController* controller = [[host.control configuration] userContentController];
    WKContentWorld* world = [WKContentWorld worldWithName:webViewContentWorldName];
    [host rebuildUserScripts];
    if (fitsContent) {
        [controller addScriptMessageHandler:host.messageProxy contentWorld:world name:webViewContentHeightHandler];
        [host applyLiveScript:webViewContentHeightScript];
        return;
    }
    [controller removeScriptMessageHandlerForName:webViewContentHeightHandler contentWorld:world];
    [host applyLiveScript:webViewContentHeightStop];
    host.contentHeight = -1;
}

extern "C" void
brixWebViewSetUserStyle(void* ptr, const char* css) {
    BxWebViewHost* host = getHost(ptr);
    NSString* value = webViewString(css);
    if (!host.control || [value isEqualToString:host.userStyle ?: @""]) {
        return;
    }
    host.userStyle = value;
    [host rebuildUserScripts];
    [host applyLiveScript:webViewStyleScript(value)];
}

extern "C" void
brixWebViewSetUserScript(void* ptr, const char* script) {
    BxWebViewHost* host = getHost(ptr);
    NSString* value = webViewString(script);
    if (!host.control || [value isEqualToString:host.userScript ?: @""]) {
        return;
    }
    host.userScript = value;
    [host rebuildUserScripts];
}

extern "C" void
brixWebViewSetTransparent(void* ptr, bool transparent) {
    BxWebViewControl* control = getHost(ptr).control;
    if (!control) {
        return;
    }
    [control setValue:@(!transparent) forKey:@"drawsBackground"];
    [control setUnderPageBackgroundColor:transparent ? [NSColor clearColor] : nil];
}

extern "C" void
brixWebViewSetPageZoom(void* ptr, double zoom) {
    BxWebViewControl* control = getHost(ptr).control;
    if (!control || !std::isfinite(zoom) || zoom <= 0) {
        return;
    }
    if (@available(macOS 11.0, *)) {
        if ([control pageZoom] != zoom) {
            [control setPageZoom:zoom];
        }
    }
}

extern "C" void
brixWebViewSetInspectable(void* ptr, bool inspectable) {
    BxWebViewHost* host = getHost(ptr);
    if (@available(macOS 13.3, *)) {
        [host.control setInspectable:inspectable];
    }
}

extern "C" void
brixWebViewClearMessageHandlers(void* ptr) {
    [getHost(ptr) clearMessageHandlers];
}

extern "C" void
brixWebViewAddMessageHandler(void* ptr, const char* name) {
    BxWebViewHost* host = getHost(ptr);
    NSString* value = webViewString(name);
    if (!host.control || ![value length] || [host.messageNames containsObject:value]) {
        return;
    }
    [[[host.control configuration] userContentController] addScriptMessageHandler:host.messageProxy
                                                                             name:value];
    [host.messageNames addObject:value];
}

extern "C" void
brixWebViewDecideNavigation(void* ptr, uint64_t requestId, bool allow) {
    BxWebViewHost* host = getHost(ptr);
    NSNumber* key = @(requestId);
    void (^decide)(WKNavigationActionPolicy) = [[[host.pendingDecisions objectForKey:key] retain] autorelease];
    if (!decide) {
        return;
    }
    [host.pendingDecisions removeObjectForKey:key];
    decide(allow ? WKNavigationActionPolicyAllow : WKNavigationActionPolicyCancel);
}

extern "C" void
brixWebViewEvaluate(void* ptr, const char* script, BxWebViewScriptCallback callback, void* context) {
    BxWebViewHost* host = getHost(ptr);
    std::shared_ptr<BxWebViewScriptState> state = std::make_shared<BxWebViewScriptState>(callback, context);
    if (!host.control) {
        webViewFinishScript(state, false, @"The web view was closed");
        return;
    }
    [host trackScript:state];
    [host.control evaluateJavaScript:webViewString(script) completionHandler:^(id result, NSError* error) {
        webViewFinishScript(state, !error, error ? webViewScriptErrorText(error) : webViewJSONText(result));
    }];
}
