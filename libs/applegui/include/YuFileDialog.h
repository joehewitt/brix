#ifndef YU_FILE_DIALOG_H
#define YU_FILE_DIALOG_H

#include <stdbool.h>
#include <stdint.h>

// **********************************************************************************************

typedef void (*YuFileDialogCallback)(void* context, const char* const* paths, int32_t count);

extern "C" void* yumiFileDialogSave(
    void* window,
    const char* name,
    const char* const* types,
    int32_t typeCount,
    const char* directory,
    const char* message,
    const char* prompt,
    YuFileDialogCallback callback,
    void* context);
extern "C" void* yumiFileDialogOpen(
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
    void* context);
extern "C" void yumiFileDialogRelease(void* dialog);

#endif
