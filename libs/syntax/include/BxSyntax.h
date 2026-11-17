#ifndef BX_SYNTAX_H
#define BX_SYNTAX_H

#include <stdbool.h>
#include <stdint.h>

#include <BxGC/BxGCAnnotations.h>

#ifdef __cplusplus
#define BX_SYNTAX_API extern "C"
#else
#define BX_SYNTAX_API extern
#endif

// **********************************************************************************************

BX_SYNTAX_API BRIX_WITHOUT_GC int32_t brixSyntaxLanguageCount(void);
BX_SYNTAX_API BRIX_WITHOUT_GC const char* brixSyntaxLanguageName(int32_t language);
BX_SYNTAX_API BRIX_WITHOUT_GC const char* brixSyntaxLanguageTitle(int32_t language);
BX_SYNTAX_API BRIX_WITHOUT_GC const char* brixSyntaxLanguageExtensions(int32_t language);

BX_SYNTAX_API int32_t brixSyntaxCaptureCount(int32_t language);
BX_SYNTAX_API const char* brixSyntaxCaptureName(int32_t language, int32_t capture);
BX_SYNTAX_API const char* brixSyntaxQueryError(int32_t language);
BX_SYNTAX_API const char* brixSyntaxFoldQueryError(int32_t language);

// **********************************************************************************************

BX_SYNTAX_API void* brixSyntaxDocumentCreate(int32_t language);
BX_SYNTAX_API void brixSyntaxDocumentRelease(void* document);
BX_SYNTAX_API BRIX_WITHOUT_GC void brixSyntaxDocumentEdit(
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
BX_SYNTAX_API int32_t brixSyntaxDocumentParse(
    void* document,
    const void* text,
    uint32_t length,
    uint32_t budgetMicroseconds
);
BX_SYNTAX_API bool brixSyntaxDocumentContinueParse(void* document);
BX_SYNTAX_API BRIX_WITHOUT_GC bool brixSyntaxDocumentFinishParse(void* document);
BX_SYNTAX_API BRIX_WITHOUT_GC void brixSyntaxDocumentCancelParse(void* document);
BX_SYNTAX_API BRIX_WITHOUT_GC bool brixSyntaxDocumentIsParsing(void* document);
BX_SYNTAX_API BRIX_WITHOUT_GC bool brixSyntaxDocumentHasTree(void* document);
BX_SYNTAX_API BRIX_WITHOUT_GC bool brixSyntaxDocumentHasError(void* document);
BX_SYNTAX_API uint32_t brixSyntaxDocumentHighlight(
    void* document,
    const void* text,
    uint32_t length,
    uint32_t startByte,
    uint32_t endByte,
    void* spans,
    uint32_t capacity
);
BX_SYNTAX_API uint32_t brixSyntaxDocumentFolds(
    void* document,
    const void* text,
    uint32_t length,
    uint32_t firstRow,
    uint32_t lastRow,
    void* rows,
    uint32_t capacity
);

#endif
