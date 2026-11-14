#ifndef YU_WEBKIT_H
#define YU_WEBKIT_H

#include <stdbool.h>
#include <stdint.h>

// **********************************************************************************************

typedef void (*YuWebViewEventCallback)(
    void* context,
    uint32_t eventKind,
    uint64_t requestId,
    uint32_t detail,
    const char* first,
    const char* second
);
typedef void (*YuWebViewScriptCallback)(void* context, bool succeeded, const char* text);
typedef void (*YuWebSignInCallback)(void* context, uint32_t status, const char* text);

// **********************************************************************************************

extern "C" void* yumiWebViewCreate(bool ephemeral);
extern "C" void yumiWebViewRelease(void* webView);
extern "C" void yumiWebViewSetEventCallback(
    void* webView,
    YuWebViewEventCallback callback,
    void* context
);
extern "C" void yumiWebViewClearEventCallback(void* webView);
extern "C" bool yumiWebViewLoadURL(void* webView, const char* url);
extern "C" void yumiWebViewLoadHTML(void* webView, const char* html, const char* baseURL);
extern "C" void yumiWebViewGoBack(void* webView);
extern "C" void yumiWebViewGoForward(void* webView);
extern "C" void yumiWebViewReload(void* webView);
extern "C" void yumiWebViewStop(void* webView);
extern "C" const char* yumiWebViewGetURL(void* webView);
extern "C" const char* yumiWebViewGetTitle(void* webView);
extern "C" bool yumiWebViewIsLoading(void* webView);
extern "C" double yumiWebViewGetProgress(void* webView);
extern "C" bool yumiWebViewCanGoBack(void* webView);
extern "C" bool yumiWebViewCanGoForward(void* webView);
extern "C" double yumiWebViewGetContentHeight(void* webView);
extern "C" void yumiWebViewSetFitsContent(void* webView, bool fitsContent);
extern "C" void yumiWebViewSetTransparent(void* webView, bool transparent);
extern "C" void yumiWebViewSetPageZoom(void* webView, double zoom);
extern "C" void yumiWebViewSetUserStyle(void* webView, const char* css);
extern "C" void yumiWebViewSetUserScript(void* webView, const char* script);
extern "C" void yumiWebViewSetInspectable(void* webView, bool inspectable);
extern "C" void yumiWebViewClearMessageHandlers(void* webView);
extern "C" void yumiWebViewAddMessageHandler(void* webView, const char* name);
extern "C" void yumiWebViewDecideNavigation(void* webView, uint64_t requestId, bool allow);
extern "C" void yumiWebViewEvaluate(
    void* webView,
    const char* script,
    YuWebViewScriptCallback callback,
    void* context
);

// **********************************************************************************************

extern "C" void* yumiWebSignInStart(
    void* window,
    const char* url,
    const char* callbackScheme,
    bool ephemeral,
    YuWebSignInCallback callback,
    void* context
);
extern "C" void yumiWebSignInRelease(void* session);

#endif
