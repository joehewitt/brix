#include "YuAppleGUI/YuFileDialog.h"
#include "YuAppleGUI/YuAppleGUI.h"
#include "YuCore/YuCore.h"
#include "YuEvents/YuEvents.h"

#import <AppKit/AppKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#include <memory>
#include <string>
#include <vector>

// **********************************************************************************************

static NSString*
fileDialogString(const char* value) {
    NSString* string = value ? [NSString stringWithUTF8String:value] : nil;
    return string ?: @"";
}

static NSArray<UTType*>*
fileDialogTypes(const char* const* types, int32_t count) {
    NSMutableArray<UTType*>* found = [NSMutableArray array];
    for (int32_t index = 0; types && index < count; ++index) {
        NSString* name = fileDialogString(types[index]);
        if (![name length]) {
            continue;
        }
        UTType* type = [name containsString:@"."]
            ? [UTType typeWithIdentifier:name]
            : [UTType typeWithFilenameExtension:name];
        if (type) {
            [found addObject:type];
        }
    }
    return found;
}

struct YuFileDialogState {
    YuFileDialogState(YuFileDialogCallback aCallback, void* aContext):
        callback(aCallback),
        context(aContext),
        contextRoot(aContext ? yumiAddRoot(static_cast<char*>(aContext)) : nullptr),
        finished(false)
    {}

    ~YuFileDialogState() {
        if (contextRoot) {
            yumiRemoveRoot(contextRoot);
        }
    }

    YuFileDialogCallback callback;
    void* context;
    void* contextRoot;
    bool finished;
};

struct YuFileDialogRecord {
    YuFileDialogRecord(YuFileDialogState& state, NSArray<NSURL*>* urls):
        callback(state.callback),
        context(state.context),
        contextRoot(state.contextRoot)
    {
        state.contextRoot = nullptr;
        for (NSURL* url in urls) {
            const char* path = [[url path] UTF8String];
            if (path) {
                paths.push_back(path);
            }
        }
    }

    ~YuFileDialogRecord() {
        if (contextRoot) {
            yumiRemoveRoot(contextRoot);
        }
    }

    YuFileDialogCallback callback;
    void* context;
    void* contextRoot;
    std::vector<std::string> paths;
};

static void
fileDialogRunCallback(void* closure) {
    YuFileDialogRecord* record = static_cast<YuFileDialogRecord*>(closure);
    if (!yumiProcessTerminationRequested()) {
        std::vector<const char*> paths;
        for (const std::string& path : record->paths) {
            paths.push_back(path.c_str());
        }
        record->callback(record->context, paths.data(), static_cast<int32_t>(paths.size()));
    }
    delete record;
    yumiTerminateActiveTask();
}

static void
fileDialogFinish(const std::shared_ptr<YuFileDialogState>& state, NSArray<NSURL*>* urls) {
    if (state->finished) {
        return;
    }
    state->finished = true;
    YuFileDialogRecord* record = new YuFileDialogRecord(*state, urls);
    if (!yumiScheduleMainTaskFromNative(fileDialogRunCallback, record)) {
        delete record;
    }
}

static void
fileDialogAbandon(const std::shared_ptr<YuFileDialogState>& state) {
    state->finished = true;
    if (state->contextRoot) {
        yumiRemoveRoot(state->contextRoot);
        state->contextRoot = nullptr;
    }
}

// **********************************************************************************************

@interface YuFileDialogHost : NSObject {
    std::shared_ptr<YuFileDialogState> state;
}

@property(nonatomic, strong) NSWindow* anchor;
@property(nonatomic, strong) NSSavePanel* panel;

- (instancetype)initWithCallback:(YuFileDialogCallback)callback context:(void*)context;
- (void)present:(NSSavePanel*)panel directory:(NSString*)directory message:(NSString*)message prompt:(NSString*)prompt;
- (void)close;

@end

@implementation YuFileDialogHost

- (instancetype)initWithCallback:(YuFileDialogCallback)callback context:(void*)context {
    self = [super init];
    if (self) {
        state = std::make_shared<YuFileDialogState>(callback, context);
    }
    return self;
}

- (void)dealloc {
    [self close];
}

- (void)present:(NSSavePanel*)panel directory:(NSString*)directory message:(NSString*)message prompt:(NSString*)prompt {
    if ([directory length]) {
        panel.directoryURL = [NSURL fileURLWithPath:directory isDirectory:YES];
    }
    if ([message length]) {
        panel.message = message;
    }
    if ([prompt length]) {
        panel.prompt = prompt;
    }
    self.panel = panel;

    std::shared_ptr<YuFileDialogState> captured = state;
    NSSavePanel* shown = panel;
    void (^finished)(NSModalResponse) = ^(NSModalResponse response) {
        if (response != NSModalResponseOK) {
            fileDialogFinish(captured, @[]);
        } else if ([shown isKindOfClass:[NSOpenPanel class]]) {
            fileDialogFinish(captured, [(NSOpenPanel*)shown URLs]);
        } else {
            NSURL* url = [shown URL];
            fileDialogFinish(captured, url ? @[url] : @[]);
        }
    };
    if (self.anchor) {
        [panel beginSheetModalForWindow:self.anchor completionHandler:finished];
    } else {
        [panel beginWithCompletionHandler:finished];
    }
}

- (void)close {
    bool pending = state && !state->finished;
    if (state) {
        fileDialogAbandon(state);
    }
    if (pending && self.panel) {
        [self.panel cancel:nil];
    }
    self.panel = nil;
    self.anchor = nil;
}

@end

// **********************************************************************************************

static YuFileDialogHost*
fileDialogHost(void* window, YuFileDialogCallback callback, void* context) {
    YuFileDialogHost* host = [[YuFileDialogHost alloc] initWithCallback:callback context:context];
    host.anchor = window ? (__bridge NSWindow*)yumiGuiWindowGetNativeWindow(window) : nil;
    return host;
}

extern "C" void*
yumiFileDialogSave(
    void* window,
    const char* name,
    const char* const* types,
    int32_t typeCount,
    const char* directory,
    const char* message,
    const char* prompt,
    YuFileDialogCallback callback,
    void* context
) {
    YuFileDialogHost* host = fileDialogHost(window, callback, context);
    NSSavePanel* panel = [NSSavePanel savePanel];
    panel.canCreateDirectories = YES;
    panel.extensionHidden = NO;
    NSString* suggested = fileDialogString(name);
    if ([suggested length]) {
        panel.nameFieldStringValue = suggested;
    }
    NSArray<UTType*>* allowed = fileDialogTypes(types, typeCount);
    if ([allowed count]) {
        panel.allowedContentTypes = allowed;
        panel.allowsOtherFileTypes = NO;
    }
    [host
        present:panel
        directory:fileDialogString(directory)
        message:fileDialogString(message)
        prompt:fileDialogString(prompt)];
    return (__bridge_retained void*)host;
}

extern "C" void*
yumiFileDialogOpen(
    void* window,
    const char* const* types,
    int32_t typeCount,
    const char* directory,
    bool allowsMultiple,
    bool choosesFiles,
    bool choosesDirectories,
    const char* message,
    const char* prompt,
    YuFileDialogCallback callback,
    void* context
) {
    YuFileDialogHost* host = fileDialogHost(window, callback, context);
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    panel.allowsMultipleSelection = allowsMultiple;
    panel.canChooseFiles = choosesFiles;
    panel.canChooseDirectories = choosesDirectories;
    panel.canCreateDirectories = choosesDirectories;
    NSArray<UTType*>* allowed = fileDialogTypes(types, typeCount);
    if ([allowed count]) {
        panel.allowedContentTypes = allowed;
    }
    [host
        present:panel
        directory:fileDialogString(directory)
        message:fileDialogString(message)
        prompt:fileDialogString(prompt)];
    return (__bridge_retained void*)host;
}

extern "C" void
yumiFileDialogRelease(void* ptr) {
    if (!ptr) {
        return;
    }
    YuFileDialogHost* host = (__bridge YuFileDialogHost*)ptr;
    [host close];
    CFBridgingRelease(ptr);
}
