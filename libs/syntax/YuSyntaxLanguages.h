#ifndef YU_SYNTAX_LANGUAGES_H
#define YU_SYNTAX_LANGUAGES_H

#include <stdint.h>

#include <tree_sitter/api.h>

typedef struct {
    const char* name;
    const char* title;
    const char* extensions;
    const TSLanguage* (*language)(void);
    const char* const* highlights;
    const char* const* folds;
} YuSyntaxLanguageEntry;

extern const YuSyntaxLanguageEntry yumiSyntaxLanguageEntries[];
extern const int32_t yumiSyntaxLanguageEntryCount;

#endif
