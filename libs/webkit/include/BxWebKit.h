#ifndef BX_WEBKIT_H
#define BX_WEBKIT_H

#include <stdbool.h>
#include <stdint.h>

// **********************************************************************************************

typedef void (*BxWebViewEventCallback)(
    void* context,
    uint32_t eventKind,
    uint64_t requestId,
    uint32_t detail,
    const char* first,
    const char* second
);
typedef void (*BxWebViewScriptCallback)(void* context, bool succeeded, const char* text);
typedef void (*BxWebSignInCallback)(void* context, uint32_t status, const char* text);

// **********************************************************************************************

extern "C" void* brixWebViewCreate(bool ephemeral);
extern "C" void brixWebViewRelease(void* webView);
extern "C" void brixWebViewSetEventCallback(
    void* webView,
    BxWebViewEventCallback callback,
    void* context
);
extern "C" void brixWebViewClearEventCallback(void* webView);
extern "C" bool brixWebViewLoadURL(void* webView, const char* url);
extern "C" void brixWebViewLoadHTML(void* webView, const char* html, const char* baseURL);
extern "C" void brixWebViewGoBack(void* webView);
extern "C" void brixWebViewGoForward(void* webView);
extern "C" void brixWebViewReload(void* webView);
extern "C" void brixWebViewStop(void* webView);
extern "C" const char* brixWebViewGetURL(void* webView);
extern "C" const char* brixWebViewGetTitle(void* webView);
extern "C" bool brixWebViewIsLoading(void* webView);
extern "C" double brixWebViewGetProgress(void* webView);
extern "C" bool brixWebViewCanGoBack(void* webView);
extern "C" bool brixWebViewCanGoForward(void* webView);
extern "C" double brixWebViewGetContentHeight(void* webView);
extern "C" void brixWebViewSetFitsContent(void* webView, bool fitsContent);
extern "C" void brixWebViewSetTransparent(void* webView, bool transparent);
extern "C" void brixWebViewSetPageZoom(void* webView, double zoom);
extern "C" void brixWebViewSetUserStyle(void* webView, const char* css);
extern "C" void brixWebViewSetUserScript(void* webView, const char* script);
extern "C" void brixWebViewSetInspectable(void* webView, bool inspectable);
extern "C" void brixWebViewClearMessageHandlers(void* webView);
extern "C" void brixWebViewAddMessageHandler(void* webView, const char* name);
extern "C" void brixWebViewDecideNavigation(void* webView, uint64_t requestId, bool allow);
extern "C" void brixWebViewEvaluate(
    void* webView,
    const char* script,
    BxWebViewScriptCallback callback,
    void* context
);

// **********************************************************************************************

extern "C" void* brixWebSignInStart(
    void* window,
    const char* url,
    const char* callbackScheme,
    bool ephemeral,
    BxWebSignInCallback callback,
    void* context
);
extern "C" void brixWebSignInRelease(void* session);

#endif
