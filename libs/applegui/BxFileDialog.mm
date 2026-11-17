#include "BxAppleGUI/BxFileDialog.h"
#include "BxAppleGUI/BxAppleGUI.h"
#include "BxCore/BxCore.h"
#include "BxEvents/BxEvents.h"

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

struct BxFileDialogState {
    BxFileDialogState(BxFileDialogCallback aCallback, void* aContext):
        callback(aCallback),
        context(aContext),
        contextRoot(aContext ? brixAddRoot(static_cast<char*>(aContext)) : nullptr),
        finished(false)
    {}

    ~BxFileDialogState() {
        if (contextRoot) {
            brixRemoveRoot(contextRoot);
        }
    }

    BxFileDialogCallback callback;
    void* context;
    void* contextRoot;
    bool finished;
};

struct BxFileDialogRecord {
    BxFileDialogRecord(BxFileDialogState& state, NSArray<NSURL*>* urls):
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

    ~BxFileDialogRecord() {
        if (contextRoot) {
            brixRemoveRoot(contextRoot);
        }
    }

    BxFileDialogCallback callback;
    void* context;
    void* contextRoot;
    std::vector<std::string> paths;
};

static void
fileDialogRunCallback(void* closure) {
    BxFileDialogRecord* record = static_cast<BxFileDialogRecord*>(closure);
    if (!brixProcessTerminationRequested()) {
        std::vector<const char*> paths;
        for (const std::string& path : record->paths) {
            paths.push_back(path.c_str());
        }
        record->callback(record->context, paths.data(), static_cast<int32_t>(paths.size()));
    }
    delete record;
    brixTerminateActiveTask();
}

static void
fileDialogFinish(const std::shared_ptr<BxFileDialogState>& state, NSArray<NSURL*>* urls) {
    if (state->finished) {
        return;
    }
    state->finished = true;
    BxFileDialogRecord* record = new BxFileDialogRecord(*state, urls);
    if (!brixScheduleMainTaskFromNative(fileDialogRunCallback, record)) {
        delete record;
    }
}

static void
fileDialogAbandon(const std::shared_ptr<BxFileDialogState>& state) {
    state->finished = true;
    if (state->contextRoot) {
        brixRemoveRoot(state->contextRoot);
        state->contextRoot = nullptr;
    }
}

// **********************************************************************************************

@interface BxFileDialogHost : NSObject {
    std::shared_ptr<BxFileDialogState> state;
}

@property(nonatomic, strong) NSWindow* anchor;
@property(nonatomic, strong) NSSavePanel* panel;

- (instancetype)initWithCallback:(BxFileDialogCallback)callback context:(void*)context;
- (void)present:(NSSavePanel*)panel directory:(NSString*)directory message:(NSString*)message prompt:(NSString*)prompt;
- (void)close;

@end

@implementation BxFileDialogHost

- (instancetype)initWithCallback:(BxFileDialogCallback)callback context:(void*)context {
    self = [super init];
    if (self) {
        state = std::make_shared<BxFileDialogState>(callback, context);
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

    std::shared_ptr<BxFileDialogState> captured = state;
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

static BxFileDialogHost*
fileDialogHost(void* window, BxFileDialogCallback callback, void* context) {
    BxFileDialogHost* host = [[BxFileDialogHost alloc] initWithCallback:callback context:context];
    host.anchor = window ? (__bridge NSWindow*)brixGuiWindowGetNativeWindow(window) : nil;
    return host;
}

extern "C" void*
brixFileDialogSave(
    void* window,
    const char* name,
    const char* const* types,
    int32_t typeCount,
    const char* directory,
    const char* message,
    const char* prompt,
    BxFileDialogCallback callback,
    void* context
) {
    BxFileDialogHost* host = fileDialogHost(window, callback, context);
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
brixFileDialogOpen(
    void* window,
    const char* const* types,
    int32_t typeCount,
    const char* directory,
    bool allowsMultiple,
    bool choosesFiles,
    bool choosesDirectories,
    const char* message,
    const char* prompt,
    BxFileDialogCallback callback,
    void* context
) {
    BxFileDialogHost* host = fileDialogHost(window, callback, context);
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
brixFileDialogRelease(void* ptr) {
    if (!ptr) {
        return;
    }
    BxFileDialogHost* host = (__bridge BxFileDialogHost*)ptr;
    [host close];
    CFBridgingRelease(ptr);
}
