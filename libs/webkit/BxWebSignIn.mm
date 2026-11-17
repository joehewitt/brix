#include "include/BxWebKit.h"
#include "BxAppleGUI/BxAppleGUI.h"
#include "BxCore/BxCore.h"
#include "BxEvents/BxEvents.h"

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

struct BxWebSignInState {
    BxWebSignInState(BxWebSignInCallback aCallback, void* aContext):
        callback(aCallback),
        context(aContext),
        contextRoot(aContext ? brixAddRoot(static_cast<char*>(aContext)) : nullptr),
        finished(false)
    {}

    ~BxWebSignInState() {
        if (contextRoot) {
            brixRemoveRoot(contextRoot);
        }
    }

    BxWebSignInCallback callback;
    void* context;
    void* contextRoot;
    bool finished;
};

struct BxWebSignInRecord {
    BxWebSignInRecord(BxWebSignInState& state, uint32_t aStatus, NSString* aText):
        callback(state.callback),
        context(state.context),
        contextRoot(state.contextRoot),
        status(aStatus),
        text([aText UTF8String] ?: "")
    {
        state.contextRoot = nullptr;
    }

    ~BxWebSignInRecord() {
        if (contextRoot) {
            brixRemoveRoot(contextRoot);
        }
    }

    BxWebSignInCallback callback;
    void* context;
    void* contextRoot;
    uint32_t status;
    std::string text;
};

static void
webSignInRunCallback(void* closure) {
    BxWebSignInRecord* record = static_cast<BxWebSignInRecord*>(closure);
    if (!brixProcessTerminationRequested()) {
        record->callback(record->context, record->status, record->text.c_str());
    }
    delete record;
    brixTerminateActiveTask();
}

static void
webSignInFinish(const std::shared_ptr<BxWebSignInState>& state, uint32_t status, NSString* text) {
    if (state->finished) {
        return;
    }
    state->finished = true;
    BxWebSignInRecord* record = new BxWebSignInRecord(*state, status, text);
    if (!brixScheduleMainTaskFromNative(webSignInRunCallback, record)) {
        delete record;
    }
}

static void
webSignInAbandon(const std::shared_ptr<BxWebSignInState>& state) {
    state->finished = true;
    if (state->contextRoot) {
        brixRemoveRoot(state->contextRoot);
        state->contextRoot = nullptr;
    }
}

// **********************************************************************************************

@interface BxWebSignInHost : NSObject <ASWebAuthenticationPresentationContextProviding> {
    std::shared_ptr<BxWebSignInState> state;
}

@property(nonatomic, retain) NSWindow* anchor;
@property(nonatomic, retain) ASWebAuthenticationSession* session;

- (instancetype)initWithCallback:(BxWebSignInCallback)callback context:(void*)context;
- (void)startURL:(NSString*)url callbackScheme:(NSString*)scheme ephemeral:(BOOL)ephemeral;
- (void)close;

@end

@implementation BxWebSignInHost

- (instancetype)initWithCallback:(BxWebSignInCallback)callback context:(void*)context {
    self = [super init];
    if (self) {
        state = std::make_shared<BxWebSignInState>(callback, context);
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
    std::shared_ptr<BxWebSignInState> captured = state;
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
brixWebSignInStart(
    void* window,
    const char* url,
    const char* callbackScheme,
    bool ephemeral,
    BxWebSignInCallback callback,
    void* context
) {
    BxWebSignInHost* host = [[BxWebSignInHost alloc] initWithCallback:callback context:context];
    host.anchor = window ? (__bridge NSWindow*)brixGuiWindowGetNativeWindow(window) : nil;
    [host startURL:webSignInString(url) callbackScheme:webSignInString(callbackScheme) ephemeral:ephemeral];
    return (__bridge void*)host;
}

extern "C" void
brixWebSignInRelease(void* ptr) {
    if (!ptr) {
        return;
    }
    BxWebSignInHost* host = (__bridge BxWebSignInHost*)ptr;
    [host close];
    CFBridgingRelease(ptr);
}
