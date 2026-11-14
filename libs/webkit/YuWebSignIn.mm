#include "include/YuWebKit.h"
#include "YuAppleGUI/YuAppleGUI.h"
#include "YuCore/YuCore.h"
#include "YuEvents/YuEvents.h"

#import <AppKit/AppKit.h>
#import <AuthenticationServices/AuthenticationServices.h>

#include <memory>
#include <string>

// **********************************************************************************************

enum : uint32_t {
    kWebSignInSucceeded = 0,
    kWebSignInCanceled = 1,
    kWebSignInFailed = 2,
};

static NSString*
webSignInString(const char* value) {
    NSString* string = value ? [NSString stringWithUTF8String:value] : nil;
    return string ?: @"";
}

struct YuWebSignInState {
    YuWebSignInState(YuWebSignInCallback aCallback, void* aContext):
        callback(aCallback),
        context(aContext),
        contextRoot(aContext ? yumiAddRoot(static_cast<char*>(aContext)) : nullptr),
        finished(false)
    {}

    ~YuWebSignInState() {
        if (contextRoot) {
            yumiRemoveRoot(contextRoot);
        }
    }

    YuWebSignInCallback callback;
    void* context;
    void* contextRoot;
    bool finished;
};

struct YuWebSignInRecord {
    YuWebSignInRecord(YuWebSignInState& state, uint32_t aStatus, NSString* aText):
        callback(state.callback),
        context(state.context),
        contextRoot(state.contextRoot),
        status(aStatus),
        text([aText UTF8String] ?: "")
    {
        state.contextRoot = nullptr;
    }

    ~YuWebSignInRecord() {
        if (contextRoot) {
            yumiRemoveRoot(contextRoot);
        }
    }

    YuWebSignInCallback callback;
    void* context;
    void* contextRoot;
    uint32_t status;
    std::string text;
};

static void
webSignInRunCallback(void* closure) {
    YuWebSignInRecord* record = static_cast<YuWebSignInRecord*>(closure);
    if (!yumiProcessTerminationRequested()) {
        record->callback(record->context, record->status, record->text.c_str());
    }
    delete record;
    yumiTerminateActiveTask();
}

static void
webSignInFinish(const std::shared_ptr<YuWebSignInState>& state, uint32_t status, NSString* text) {
    if (state->finished) {
        return;
    }
    state->finished = true;
    YuWebSignInRecord* record = new YuWebSignInRecord(*state, status, text);
    if (!yumiScheduleMainTaskFromNative(webSignInRunCallback, record)) {
        delete record;
    }
}

static void
webSignInAbandon(const std::shared_ptr<YuWebSignInState>& state) {
    state->finished = true;
    if (state->contextRoot) {
        yumiRemoveRoot(state->contextRoot);
        state->contextRoot = nullptr;
    }
}

// **********************************************************************************************

@interface YuWebSignInHost : NSObject <ASWebAuthenticationPresentationContextProviding> {
    std::shared_ptr<YuWebSignInState> state;
}

@property(nonatomic, retain) NSWindow* anchor;
@property(nonatomic, retain) ASWebAuthenticationSession* session;

- (instancetype)initWithCallback:(YuWebSignInCallback)callback context:(void*)context;
- (void)startURL:(NSString*)url callbackScheme:(NSString*)scheme ephemeral:(BOOL)ephemeral;
- (void)close;

@end

@implementation YuWebSignInHost

- (instancetype)initWithCallback:(YuWebSignInCallback)callback context:(void*)context {
    self = [super init];
    if (self) {
        state = std::make_shared<YuWebSignInState>(callback, context);
    }
    return self;
}

- (void)dealloc {
    [self close];
    [super dealloc];
}

- (ASPresentationAnchor)presentationAnchorForWebAuthenticationSession:(ASWebAuthenticationSession*)session {
    return self.anchor;
}

- (void)startURL:(NSString*)url callbackScheme:(NSString*)scheme ephemeral:(BOOL)ephemeral {
    NSURL* value = [NSURL URLWithString:url];
    NSString* valueScheme = [[value scheme] lowercaseString];
    if (![valueScheme isEqualToString:@"https"] && ![valueScheme isEqualToString:@"http"]) {
        webSignInFinish(state, kWebSignInFailed, @"The sign-in URL must use http or https");
        return;
    }
    if (!self.anchor) {
        webSignInFinish(state, kWebSignInFailed, @"The window is not open");
        return;
    }
    std::shared_ptr<YuWebSignInState> captured = state;
    ASWebAuthenticationSession* session = [[[ASWebAuthenticationSession alloc]
               initWithURL:value
         callbackURLScheme:[scheme length] ? scheme : nil
         completionHandler:^(NSURL* callbackURL, NSError* error) {
             if (callbackURL) {
                 webSignInFinish(captured, kWebSignInSucceeded, [callbackURL absoluteString]);
             } else if ([[error domain] isEqualToString:ASWebAuthenticationSessionErrorDomain] &&
                        [error code] == ASWebAuthenticationSessionErrorCodeCanceledLogin) {
                 webSignInFinish(captured, kWebSignInCanceled, nil);
             } else {
                 webSignInFinish(captured, kWebSignInFailed, [error localizedDescription] ?: @"Sign-in failed");
             }
         }] autorelease];
    session.presentationContextProvider = self;
    session.prefersEphemeralWebBrowserSession = ephemeral;
    self.session = session;
    if (![session start]) {
        webSignInFinish(state, kWebSignInFailed, @"The sign-in session could not start");
    }
}

- (void)close {
    if (state) {
        webSignInAbandon(state);
    }
    [self.session cancel];
    self.session = nil;
    self.anchor = nil;
}

@end

// **********************************************************************************************

extern "C" void*
yumiWebSignInStart(
    void* window,
    const char* url,
    const char* callbackScheme,
    bool ephemeral,
    YuWebSignInCallback callback,
    void* context
) {
    YuWebSignInHost* host = [[YuWebSignInHost alloc] initWithCallback:callback context:context];
    host.anchor = window ? (__bridge NSWindow*)yumiGuiWindowGetNativeWindow(window) : nil;
    [host startURL:webSignInString(url) callbackScheme:webSignInString(callbackScheme) ephemeral:ephemeral];
    return (__bridge void*)host;
}

extern "C" void
yumiWebSignInRelease(void* ptr) {
    if (!ptr) {
        return;
    }
    YuWebSignInHost* host = (__bridge YuWebSignInHost*)ptr;
    [host close];
    CFBridgingRelease(ptr);
}
