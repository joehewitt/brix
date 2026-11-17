#ifndef BX_FILE_DIALOG_H
#define BX_FILE_DIALOG_H

#include <stdbool.h>
#include <stdint.h>

// **********************************************************************************************

typedef void (*BxFileDialogCallback)(void* context, const char* const* paths, int32_t count);

extern "C" void* brixFileDialogSave(
    void* window,
    const char* name,
    const char* const* types,
    int32_t typeCount,
    const char* directory,
    const char* message,
    const char* prompt,
    BxFileDialogCallback callback,
    void* context);
extern "C" void* brixFileDialogOpen(
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
    void* context);
extern "C" void brixFileDialogRelease(void* dialog);

#endif
