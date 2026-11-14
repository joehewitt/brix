#ifndef YU_SYNTAX_H
#define YU_SYNTAX_H

#include <stdbool.h>
#include <stdint.h>

#include <YuGC/YuGCAnnotations.h>

#ifdef __cplusplus
#define YU_SYNTAX_API extern "C"
#else
#define YU_SYNTAX_API extern
#endif

// **********************************************************************************************

YU_SYNTAX_API YUMI_GC_LEAF int32_t yumiSyntaxLanguageCount(void);
YU_SYNTAX_API YUMI_GC_LEAF const char* yumiSyntaxLanguageName(int32_t language);
YU_SYNTAX_API YUMI_GC_LEAF const char* yumiSyntaxLanguageTitle(int32_t language);
YU_SYNTAX_API YUMI_GC_LEAF const char* yumiSyntaxLanguageExtensions(int32_t language);

YU_SYNTAX_API int32_t yumiSyntaxCaptureCount(int32_t language);
YU_SYNTAX_API const char* yumiSyntaxCaptureName(int32_t language, int32_t capture);
YU_SYNTAX_API const char* yumiSyntaxQueryError(int32_t language);
YU_SYNTAX_API const char* yumiSyntaxFoldQueryError(int32_t language);

// **********************************************************************************************

YU_SYNTAX_API void* yumiSyntaxDocumentCreate(int32_t language);
YU_SYNTAX_API void yumiSyntaxDocumentRelease(void* document);
YU_SYNTAX_API YUMI_GC_LEAF void yumiSyntaxDocumentEdit(
    void* document,
    uint32_t startByte,
    uint32_t oldEndByte,
    uint32_t newEndByte,
    uint32_t startRow,
    uint32_t startColumn,
    uint32_t oldEndRow,
    uint32_t oldEndColumn,
    uint32_t newEndRow,
    uint32_t newEndColumn
);
YU_SYNTAX_API YUMI_GC_BLOCKING int32_t yumiSyntaxDocumentParse(
    void* document,
    const void* text,
    uint32_t length,
    uint32_t budgetMicroseconds
);
YU_SYNTAX_API YUMI_GC_BLOCKING bool yumiSyntaxDocumentContinueParse(void* document);
YU_SYNTAX_API YUMI_GC_LEAF bool yumiSyntaxDocumentFinishParse(void* document);
YU_SYNTAX_API YUMI_GC_LEAF void yumiSyntaxDocumentCancelParse(void* document);
YU_SYNTAX_API YUMI_GC_LEAF bool yumiSyntaxDocumentIsParsing(void* document);
YU_SYNTAX_API YUMI_GC_LEAF bool yumiSyntaxDocumentHasTree(void* document);
YU_SYNTAX_API YUMI_GC_LEAF bool yumiSyntaxDocumentHasError(void* document);
YU_SYNTAX_API uint32_t yumiSyntaxDocumentHighlight(
    void* document,
    const void* text,
    uint32_t length,
    uint32_t startByte,
    uint32_t endByte,
    void* spans,
    uint32_t capacity
);
YU_SYNTAX_API uint32_t yumiSyntaxDocumentFolds(
    void* document,
    const void* text,
    uint32_t length,
    uint32_t firstRow,
    uint32_t lastRow,
    void* rows,
    uint32_t capacity
);

#endif
