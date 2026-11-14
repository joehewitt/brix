// **********************************************************************************************
// cheaderscan — extract function, record and constant declarations from a C/C++ header via libclang.
// **********************************************************************************************

#include "include/YuCHeaderScan.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <dlfcn.h>
#include <pthread.h>

#include <clang-c/Index.h>

#define CHEADERSCAN_VERSION "0.5.5"

#define CLANG_FUNCTIONS(X) \
    X(clang_Cursor_Evaluate) \
    X(clang_EnumDecl_isScoped) \
    X(clang_Cursor_getArgument) \
    X(clang_Cursor_getMangling) \
    X(clang_Cursor_getNumArguments) \
    X(clang_Cursor_getOffsetOfField) \
    X(clang_Cursor_isMacroBuiltin) \
    X(clang_Cursor_isMacroFunctionLike) \
    X(clang_Cursor_isVariadic) \
    X(clang_EvalResult_dispose) \
    X(clang_EvalResult_getAsLongLong) \
    X(clang_EvalResult_getAsUnsigned) \
    X(clang_EvalResult_getKind) \
    X(clang_EvalResult_isUnsignedInt) \
    X(clang_Type_getAlignOf) \
    X(clang_Type_getSizeOf) \
    X(clang_Type_visitFields) \
    X(clang_createIndex) \
    X(clang_defaultDiagnosticDisplayOptions) \
    X(clang_disposeDiagnostic) \
    X(clang_disposeIndex) \
    X(clang_disposeString) \
    X(clang_disposeTokens) \
    X(clang_disposeTranslationUnit) \
    X(clang_equalTypes) \
    X(clang_formatDiagnostic) \
    X(clang_getArgType) \
    X(clang_getArrayElementType) \
    X(clang_getArraySize) \
    X(clang_getCString) \
    X(clang_getCanonicalType) \
    X(clang_getCursorKind) \
    X(clang_getCursorExtent) \
    X(clang_getCursorLinkage) \
    X(clang_getCursorLocation) \
    X(clang_getCursorResultType) \
    X(clang_getCursorSemanticParent) \
    X(clang_getCursorSpelling) \
    X(clang_getCursorType) \
    X(clang_getCursorUSR) \
    X(clang_getDiagnostic) \
    X(clang_getDiagnosticSeverity) \
    X(clang_getDiagnosticSpelling) \
    X(clang_getEnumConstantDeclUnsignedValue) \
    X(clang_getEnumConstantDeclValue) \
    X(clang_getEnumDeclIntegerType) \
    X(clang_getFieldDeclBitWidth) \
    X(clang_getFileLocation) \
    X(clang_getFileName) \
    X(clang_getFunctionTypeCallingConv) \
    X(clang_getNumArgTypes) \
    X(clang_getNumDiagnostics) \
    X(clang_getPointeeType) \
    X(clang_getResultType) \
    X(clang_getTokenSpelling) \
    X(clang_getTranslationUnitCursor) \
    X(clang_getTypeDeclaration) \
    X(clang_getTypeSpelling) \
    X(clang_isCursorDefinition) \
    X(clang_isFunctionTypeVariadic) \
    X(clang_isInvalidDeclaration) \
    X(clang_isPODType) \
    X(clang_parseTranslationUnit2FullArgv) \
    X(clang_tokenize) \
    X(clang_visitChildren)

#define CLANG_POINTER(name) static __typeof__(&name) name##Pointer;
CLANG_FUNCTIONS(CLANG_POINTER)

#define clang_Cursor_Evaluate clang_Cursor_EvaluatePointer
#define clang_EnumDecl_isScoped clang_EnumDecl_isScopedPointer
#define clang_Cursor_getArgument clang_Cursor_getArgumentPointer
#define clang_Cursor_getMangling clang_Cursor_getManglingPointer
#define clang_Cursor_getNumArguments clang_Cursor_getNumArgumentsPointer
#define clang_Cursor_getOffsetOfField clang_Cursor_getOffsetOfFieldPointer
#define clang_Cursor_isMacroBuiltin clang_Cursor_isMacroBuiltinPointer
#define clang_Cursor_isMacroFunctionLike clang_Cursor_isMacroFunctionLikePointer
#define clang_Cursor_isVariadic clang_Cursor_isVariadicPointer
#define clang_EvalResult_dispose clang_EvalResult_disposePointer
#define clang_EvalResult_getAsLongLong clang_EvalResult_getAsLongLongPointer
#define clang_EvalResult_getAsUnsigned clang_EvalResult_getAsUnsignedPointer
#define clang_EvalResult_getKind clang_EvalResult_getKindPointer
#define clang_EvalResult_isUnsignedInt clang_EvalResult_isUnsignedIntPointer
#define clang_Type_getAlignOf clang_Type_getAlignOfPointer
#define clang_Type_getSizeOf clang_Type_getSizeOfPointer
#define clang_Type_visitFields clang_Type_visitFieldsPointer
#define clang_createIndex clang_createIndexPointer
#define clang_defaultDiagnosticDisplayOptions clang_defaultDiagnosticDisplayOptionsPointer
#define clang_disposeDiagnostic clang_disposeDiagnosticPointer
#define clang_disposeIndex clang_disposeIndexPointer
#define clang_disposeString clang_disposeStringPointer
#define clang_disposeTokens clang_disposeTokensPointer
#define clang_disposeTranslationUnit clang_disposeTranslationUnitPointer
#define clang_equalTypes clang_equalTypesPointer
#define clang_formatDiagnostic clang_formatDiagnosticPointer
#define clang_getArgType clang_getArgTypePointer
#define clang_getArrayElementType clang_getArrayElementTypePointer
#define clang_getArraySize clang_getArraySizePointer
#define clang_getCString clang_getCStringPointer
#define clang_getCanonicalType clang_getCanonicalTypePointer
#define clang_getCursorKind clang_getCursorKindPointer
#define clang_getCursorExtent clang_getCursorExtentPointer
#define clang_getCursorLinkage clang_getCursorLinkagePointer
#define clang_getCursorLocation clang_getCursorLocationPointer
#define clang_getCursorResultType clang_getCursorResultTypePointer
#define clang_getCursorSemanticParent clang_getCursorSemanticParentPointer
#define clang_getCursorSpelling clang_getCursorSpellingPointer
#define clang_getCursorType clang_getCursorTypePointer
#define clang_getCursorUSR clang_getCursorUSRPointer
#define clang_getDiagnostic clang_getDiagnosticPointer
#define clang_getDiagnosticSeverity clang_getDiagnosticSeverityPointer
#define clang_getDiagnosticSpelling clang_getDiagnosticSpellingPointer
#define clang_getEnumConstantDeclUnsignedValue clang_getEnumConstantDeclUnsignedValuePointer
#define clang_getEnumConstantDeclValue clang_getEnumConstantDeclValuePointer
#define clang_getEnumDeclIntegerType clang_getEnumDeclIntegerTypePointer
#define clang_getFieldDeclBitWidth clang_getFieldDeclBitWidthPointer
#define clang_getFileLocation clang_getFileLocationPointer
#define clang_getFileName clang_getFileNamePointer
#define clang_getFunctionTypeCallingConv clang_getFunctionTypeCallingConvPointer
#define clang_getNumArgTypes clang_getNumArgTypesPointer
#define clang_getNumDiagnostics clang_getNumDiagnosticsPointer
#define clang_getPointeeType clang_getPointeeTypePointer
#define clang_getResultType clang_getResultTypePointer
#define clang_getTokenSpelling clang_getTokenSpellingPointer
#define clang_getTranslationUnitCursor clang_getTranslationUnitCursorPointer
#define clang_getTypeDeclaration clang_getTypeDeclarationPointer
#define clang_getTypeSpelling clang_getTypeSpellingPointer
#define clang_isCursorDefinition clang_isCursorDefinitionPointer
#define clang_isFunctionTypeVariadic clang_isFunctionTypeVariadicPointer
#define clang_isInvalidDeclaration clang_isInvalidDeclarationPointer
#define clang_isPODType clang_isPODTypePointer
#define clang_parseTranslationUnit2FullArgv clang_parseTranslationUnit2FullArgvPointer
#define clang_tokenize clang_tokenizePointer
#define clang_visitChildren clang_visitChildrenPointer

static pthread_mutex_t libclangChoiceLock = PTHREAD_MUTEX_INITIALIZER;
static char libclangChoice[PATH_MAX];
static char clangDriver[PATH_MAX] = "clang";

void yumiCHeaderScanUseLibclang(const char *path) {
    pthread_mutex_lock(&libclangChoiceLock);
    snprintf(libclangChoice, sizeof(libclangChoice), "%s", path ? path : "");
    pthread_mutex_unlock(&libclangChoiceLock);
}

static bool libclangPath(char *path, size_t size) {
    const char *configured = getenv("CHEADERSCAN_LIBCLANG");
    if (configured && configured[0]) {
        snprintf(path, size, "%s", configured);
        return true;
    }
    pthread_mutex_lock(&libclangChoiceLock);
    snprintf(path, size, "%s", libclangChoice);
    pthread_mutex_unlock(&libclangChoiceLock);
    if (path[0]) {
        return true;
    }
#ifdef __APPLE__
    FILE *pipe = popen("/usr/bin/xcrun --find clang 2>/dev/null", "r");
    if (!pipe) {
        return false;
    }
    char clang[PATH_MAX] = {0};
    bool found = fgets(clang, sizeof(clang), pipe) != NULL;
    if (pclose(pipe) != 0 || !found) {
        return false;
    }
    clang[strcspn(clang, "\n")] = 0;
    char *slash = strrchr(clang, '/');
    if (!slash) {
        return false;
    }
    *slash = 0;
    snprintf(path, size, "%s/../lib/libclang.dylib", clang);
    return true;
#else
    return false;
#endif
}

static void findClangDriver(const char *libclang) {
    char directory[PATH_MAX];
    snprintf(directory, sizeof(directory), "%s", libclang);
    char *slash = strrchr(directory, '/');
    if (!slash) {
        return;
    }
    *slash = 0;
    char resolved[PATH_MAX];
    if (!realpath(directory, resolved)) {
        return;
    }
    slash = strrchr(resolved, '/');
    if (!slash) {
        return;
    }
    *slash = 0;
    snprintf(clangDriver, sizeof(clangDriver), "%s/bin/clang", resolved);
}

static bool loadLibclang(void) {
    char path[PATH_MAX];
    if (!libclangPath(path, sizeof(path))) {
#ifdef __APPLE__
        fprintf(stderr, "cheaderscan: xcrun --find clang failed; set CHEADERSCAN_LIBCLANG\n");
#else
        fprintf(stderr, "cheaderscan: no libclang; name one as \"libclang\" in the host's libs/build.json entry, or set CHEADERSCAN_LIBCLANG\n");
#endif
        return false;
    }
    void *handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        fprintf(stderr, "cheaderscan: cannot load %s: %s\n", path, dlerror());
        return false;
    }
#define CLANG_LOAD(name) \
    name##Pointer = (__typeof__(name##Pointer))dlsym(handle, #name); \
    if (!name##Pointer) { \
        fprintf(stderr, "cheaderscan: %s has no %s\n", path, #name); \
        return false; \
    }
    CLANG_FUNCTIONS(CLANG_LOAD)
#undef CLANG_LOAD
    findClangDriver(path);
    return true;
}

static pthread_once_t libclangOnce = PTHREAD_ONCE_INIT;
static bool libclangLoaded = false;

static void loadLibclangOnce(void) {
    libclangLoaded = loadLibclang();
}

// **********************************************************************************************
// Small growable pointer array for compile args.

typedef struct {
    const char **data;
    int count;
    int capacity;
} ArgList;

static void argsAdd(ArgList *a, const char *s) {
    if (a->count == a->capacity) {
        a->capacity = a->capacity ? a->capacity * 2 : 16;
        a->data = (const char **)realloc(a->data, sizeof(char *) * a->capacity);
        if (!a->data) {
            fprintf(stderr, "cheaderscan: out of memory\n");
            exit(1);
        }
    }
    a->data[a->count++] = s;
}

static void argsFree(ArgList *a) {
    free(a->data);
    a->data = NULL;
    a->count = 0;
    a->capacity = 0;
}

// **********************************************************************************************
// Tiny string set for deduping record keys we have already emitted.

typedef struct {
    char **data;
    int count;
    int capacity;
} KeySet;

static bool keySetContains(const KeySet *s, const char *key) {
    for (int i = 0; i < s->count; i++) {
        if (strcmp(s->data[i], key) == 0) {
            return true;
        }
    }
    return false;
}

static void keySetAdd(KeySet *s, const char *key) {
    if (s->count == s->capacity) {
        s->capacity = s->capacity ? s->capacity * 2 : 32;
        s->data = (char **)realloc(s->data, sizeof(char *) * s->capacity);
        if (!s->data) {
            fprintf(stderr, "cheaderscan: out of memory\n");
            exit(1);
        }
    }
    s->data[s->count++] = strdup(key);
}

static void keySetFree(KeySet *s) {
    for (int i = 0; i < s->count; i++) {
        free(s->data[i]);
    }
    free(s->data);
    s->data = NULL;
    s->count = 0;
    s->capacity = 0;
}

// **********************************************************************************************
// The macros a parse defines: each one's name, where it is defined, whether it takes arguments,
// and the tokens it is defined as, found by name. A macro defined twice is found as defined last.

typedef struct {
    char **names;
    char **files;
    unsigned *lines;
    bool *functionLike;
    char ***tokens;
    int *tokenCounts;
    char *states;
    int count;
    int capacity;
    int *slots;
    int slotCount;
} MacroList;

static unsigned macroHash(const char *name) {
    unsigned hash = 2166136261u;
    for (const char *c = name; *c; c++) {
        hash = (hash ^ (unsigned char)*c) * 16777619u;
    }
    return hash;
}

static void *growArray(void *data, size_t size) {
    void *grown = realloc(data, size);
    if (!grown) {
        fprintf(stderr, "cheaderscan: out of memory\n");
        exit(1);
    }
    return grown;
}

static void macroListIndex(MacroList *list, int position) {
    unsigned slot = macroHash(list->names[position]) & (unsigned)(list->slotCount - 1);
    while (list->slots[slot] >= 0 && strcmp(list->names[list->slots[slot]], list->names[position]) != 0) {
        slot = (slot + 1) & (unsigned)(list->slotCount - 1);
    }
    list->slots[slot] = position;
}

static int macroListFind(const MacroList *list, const char *name) {
    if (list->slotCount == 0) {
        return -1;
    }
    unsigned slot = macroHash(name) & (unsigned)(list->slotCount - 1);
    while (list->slots[slot] >= 0) {
        if (strcmp(list->names[list->slots[slot]], name) == 0) {
            return list->slots[slot];
        }
        slot = (slot + 1) & (unsigned)(list->slotCount - 1);
    }
    return -1;
}

static void macroListAdd(MacroList *list, const char *name, const char *file, unsigned line,
                         bool functionLike, char **tokens, int tokenCount) {
    if (list->count == list->capacity) {
        list->capacity = list->capacity ? list->capacity * 2 : 256;
        list->names = (char **)growArray(list->names, sizeof(char *) * list->capacity);
        list->files = (char **)growArray(list->files, sizeof(char *) * list->capacity);
        list->lines = (unsigned *)growArray(list->lines, sizeof(unsigned) * list->capacity);
        list->functionLike = (bool *)growArray(list->functionLike, sizeof(bool) * list->capacity);
        list->tokens = (char ***)growArray(list->tokens, sizeof(char **) * list->capacity);
        list->tokenCounts = (int *)growArray(list->tokenCounts, sizeof(int) * list->capacity);
        list->states = (char *)growArray(list->states, (size_t)list->capacity);
    }
    int position = list->count++;
    list->names[position] = strdup(name);
    list->files[position] = strdup(file);
    list->lines[position] = line;
    list->functionLike[position] = functionLike;
    list->tokens[position] = tokens;
    list->tokenCounts[position] = tokenCount;
    list->states[position] = 0;
    if (list->count * 2 > list->slotCount) {
        list->slotCount = list->slotCount ? list->slotCount * 2 : 1024;
        list->slots = (int *)growArray(list->slots, sizeof(int) * (size_t)list->slotCount);
        for (int i = 0; i < list->slotCount; i++) {
            list->slots[i] = -1;
        }
        for (int i = 0; i < list->count; i++) {
            macroListIndex(list, i);
        }
    } else {
        macroListIndex(list, position);
    }
}

static void macroListFree(MacroList *list) {
    for (int i = 0; i < list->count; i++) {
        free(list->names[i]);
        free(list->files[i]);
        for (int t = 0; t < list->tokenCounts[i]; t++) {
            free(list->tokens[i][t]);
        }
        free(list->tokens[i]);
    }
    free(list->names);
    free(list->files);
    free(list->lines);
    free(list->functionLike);
    free(list->tokens);
    free(list->tokenCounts);
    free(list->states);
    free(list->slots);
    memset(list, 0, sizeof(*list));
}

// Whether a macro can stand as an expression without unbalancing what follows it: neither it nor
// any macro it names expands to a brace or a semicolon. A macro that names itself, as glibc's do
// for enum constants, names the constant.
static bool macroIsExpression(MacroList *list, int position) {
    if (list->states[position] == 1 || list->states[position] == 2) {
        return true;
    }
    if (list->states[position] == 3) {
        return false;
    }
    list->states[position] = 1;
    bool expression = true;
    for (int t = 0; t < list->tokenCounts[position] && expression; t++) {
        const char *token = list->tokens[position][t];
        if (strcmp(token, "{") == 0 || strcmp(token, "}") == 0 || strcmp(token, ";") == 0) {
            expression = false;
        } else {
            int named = macroListFind(list, token);
            if (named >= 0 && !macroIsExpression(list, named)) {
                expression = false;
            }
        }
    }
    list->states[position] = expression ? 2 : 3;
    return expression;
}

// **********************************************************************************************
// Visitor state and emit logic.

typedef struct {
    FILE *out;
    int emittedFunctions;
    int emittedAnnotations;
    int emittedRecords;
    int skippedCommas;
    bool logSkips;
    KeySet seenRecords;
    CXTranslationUnit tu;
    MacroList macros;
} EmitCtx;

static bool hasComma(const char *s) {
    return s && strchr(s, ',') != NULL;
}

static void appendString(char **out, size_t *length, size_t *capacity, const char *text) {
    size_t textLen = strlen(text);
    if (*length + textLen + 1 > *capacity) {
        while (*length + textLen + 1 > *capacity) {
            *capacity = *capacity ? *capacity * 2 : 64;
        }
        *out = (char *)realloc(*out, *capacity);
        if (!*out) {
            fprintf(stderr, "cheaderscan: out of memory\n");
            exit(1);
        }
    }
    memcpy(*out + *length, text, textLen + 1);
    *length += textLen;
}

static bool isFunctionType(CXType type) {
    return type.kind == CXType_FunctionProto || type.kind == CXType_FunctionNoProto;
}

static char *typeSpelling(CXType type);

static char *functionPointerSpelling(CXType funcType) {
    char *out = NULL;
    size_t length = 0;
    size_t capacity = 0;

    appendString(&out, &length, &capacity, "fnptr(");

    CXType resultType = clang_getCanonicalType(clang_getResultType(funcType));
    char *result = typeSpelling(resultType);
    appendString(&out, &length, &capacity, result);
    free(result);

    int nargs = clang_getNumArgTypes(funcType);
    for (int i = 0; i < nargs; i++) {
        appendString(&out, &length, &capacity, ";");
        CXType argType = clang_getCanonicalType(clang_getArgType(funcType, (unsigned)i));
        char *arg = typeSpelling(argType);
        appendString(&out, &length, &capacity, arg);
        free(arg);
    }

    if (clang_isFunctionTypeVariadic(funcType)) {
        appendString(&out, &length, &capacity, ";...");
    }

    appendString(&out, &length, &capacity, ")");
    return out;
}

static char *typeSpelling(CXType type) {
    CXType canonical = clang_getCanonicalType(type);

    if (canonical.kind == CXType_LongDouble && clang_Type_getSizeOf(canonical) == sizeof(double)) {
        return strdup("double");
    }

    if (canonical.kind == CXType_Enum) {
        return typeSpelling(clang_getEnumDeclIntegerType(clang_getTypeDeclaration(canonical)));
    }

    if (canonical.kind == CXType_Pointer) {
        CXType pointee = clang_getCanonicalType(clang_getPointeeType(canonical));
        if (isFunctionType(pointee)) {
            return functionPointerSpelling(pointee);
        }
    }

    CXString spelling = clang_getTypeSpelling(canonical);
    const char *text = clang_getCString(spelling);
    char *copy = strdup(text ? text : "");
    clang_disposeString(spelling);
    return copy;
}

static void evidenceString(FILE *out, CXString value) {
    const unsigned char *text = (const unsigned char *)clang_getCString(value);
    if (text) {
        while (*text) {
            fprintf(out, "%02x", *text++);
        }
    }
    fputc(':', out);
    clang_disposeString(value);
}

static bool typeEvidence(FILE *out, CXType type, unsigned depth, CXType *ancestors);

typedef struct {
    FILE *out;
    unsigned depth;
    bool complete;
    CXType *ancestors;
} EvidenceFields;

static enum CXVisitorResult evidenceField(CXCursor field, CXClientData data) {
    EvidenceFields *fields = data;
    long long offset = clang_Cursor_getOffsetOfField(field);
    fprintf(fields->out, "%lld:%d:", offset,
            clang_getFieldDeclBitWidth(field));
    evidenceString(fields->out, clang_getCursorSpelling(field));
    fields->complete = fields->complete && offset >= 0 &&
        typeEvidence(fields->out, clang_getCursorType(field), fields->depth + 1,
                     fields->ancestors);
    return fields->complete ? CXVisit_Continue : CXVisit_Break;
}

static bool typeEvidence(FILE *out, CXType type, unsigned depth, CXType *ancestors) {
    if (depth > 16) {
        return false;
    }
    type = clang_getCanonicalType(type);
    for (unsigned i = 0; i < depth; i++) {
        if (clang_equalTypes(type, ancestors[i])) {
            fprintf(out, "R%u:", i);
            return true;
        }
    }
    ancestors[depth] = type;
    if (type.kind == CXType_Invalid || type.kind == CXType_Unexposed) {
        return false;
    }
    fprintf(out, "[%u:%lld:%lld:", type.kind, clang_Type_getSizeOf(type),
            clang_Type_getAlignOf(type));
    evidenceString(out, clang_getTypeSpelling(type));
    bool complete = true;
    if (isFunctionType(type)) {
        enum CXCallingConv convention = clang_getFunctionTypeCallingConv(type);
        int count = clang_getNumArgTypes(type);
        fprintf(out, "%u:%d:%d:", convention, count, clang_isFunctionTypeVariadic(type));
        complete = convention != CXCallingConv_Invalid &&
            convention != CXCallingConv_Unexposed && count >= 0 && type.kind == CXType_FunctionProto &&
            typeEvidence(out, clang_getResultType(type), depth + 1, ancestors);
        for (int i = 0; complete && i < count; i++) {
            complete = typeEvidence(out, clang_getArgType(type, (unsigned)i), depth + 1, ancestors);
        }
    } else if (type.kind == CXType_Pointer || type.kind == CXType_BlockPointer ||
               type.kind == CXType_LValueReference || type.kind == CXType_RValueReference) {
        CXType pointee = clang_getCanonicalType(clang_getPointeeType(type));
        if (pointee.kind == CXType_Record) {
            fputs("opaque:", out);
            evidenceString(out, clang_getTypeSpelling(pointee));
            evidenceString(out, clang_getCursorUSR(clang_getTypeDeclaration(pointee)));
        } else {
            complete = typeEvidence(out, pointee, depth + 1, ancestors);
        }
    } else if (type.kind == CXType_Record) {
        if (clang_Type_getSizeOf(type) < 0 || clang_Type_getAlignOf(type) < 0 ||
            !clang_isPODType(type)) {
            complete = false;
        } else {
            EvidenceFields fields = {out, depth, true, ancestors};
            clang_Type_visitFields(type, evidenceField, &fields);
            complete = fields.complete;
        }
    } else if (type.kind == CXType_ConstantArray || type.kind == CXType_IncompleteArray) {
        fprintf(out, "%lld:", clang_getArraySize(type));
        complete = typeEvidence(out, clang_getArrayElementType(type), depth + 1, ancestors);
    } else if (type.kind == CXType_Enum) {
        complete = typeEvidence(out,
            clang_getEnumDeclIntegerType(clang_getTypeDeclaration(type)), depth + 1, ancestors);
    } else if (type.kind < CXType_FirstBuiltin || type.kind > CXType_LastBuiltin) {
        complete = false;
    }
    fputc(']', out);
    return complete;
}

static void emitDeclarationEvidence(EmitCtx *ctx, CXCursor cursor, const char *name,
                                    const char *file, unsigned line) {
    if (clang_isInvalidDeclaration(cursor)) {
        return;
    }
    char *evidence = NULL;
    size_t length = 0;
    FILE *out = open_memstream(&evidence, &length);
    if (!out) {
        return;
    }
    unsigned column = 0;
    unsigned offset = 0;
    clang_getFileLocation(clang_getCursorLocation(cursor), NULL, NULL, &column, &offset);
    fprintf(out, "v1:%u:%u:%u:", column, offset, clang_getCursorLinkage(cursor));
    evidenceString(out, clang_getCursorUSR(cursor));
    evidenceString(out, clang_Cursor_getMangling(cursor));
    CXType ancestors[17];
    bool complete = typeEvidence(out, clang_getCursorType(cursor), 0, ancestors);
    fclose(out);
    if (complete) {
        fprintf(ctx->out, "D,%s,%s,%u,%s\n", name, file, line, evidence);
    }
    free(evidence);
}

// **********************************************************************************************
// Annotation collection.

typedef struct {
    char **items;
    int count;
    int capacity;
    bool comma;
} AnnotationList;

static void annotationListFree(AnnotationList *list) {
    for (int i = 0; i < list->count; i++) {
        free(list->items[i]);
    }
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
    list->comma = false;
}

static void annotationListAdd(AnnotationList *list, const char *annotation) {
    if (list->count == list->capacity) {
        list->capacity = list->capacity ? list->capacity * 2 : 4;
        list->items = (char **)realloc(list->items, sizeof(char *) * list->capacity);
        if (!list->items) {
            fprintf(stderr, "cheaderscan: out of memory\n");
            exit(1);
        }
    }
    list->items[list->count++] = strdup(annotation ? annotation : "");
}

static enum CXChildVisitResult annotationVisitor(CXCursor c, CXCursor parent, CXClientData ud) {
    (void)parent;
    AnnotationList *list = (AnnotationList *)ud;
    if (clang_getCursorKind(c) != CXCursor_AnnotateAttr) {
        return CXChildVisit_Continue;
    }

    CXString spelling = clang_getCursorSpelling(c);
    const char *annotation = clang_getCString(spelling);
    if (hasComma(annotation)) {
        list->comma = true;
    }
    annotationListAdd(list, annotation);
    clang_disposeString(spelling);
    return CXChildVisit_Continue;
}

// **********************************************************************************************
// Function emit. Returns true on success, false if skipped (comma in spelling).

static void emitFunction(EmitCtx *ctx, CXCursor c) {
    CXSourceLocation loc = clang_getCursorLocation(c);
    CXFile file = NULL;
    unsigned line = 0;
    clang_getFileLocation(loc, &file, &line, NULL, NULL);
    if (!file) {
        return;
    }

    CXString fileStr = clang_getFileName(file);
    const char *fname = clang_getCString(fileStr);
    if (!fname || !*fname) {
        clang_disposeString(fileStr);
        return;
    }

    CXString nameStr = clang_getCursorSpelling(c);
    const char *name = clang_getCString(nameStr);

    CXType retType = clang_getCanonicalType(clang_getCursorResultType(c));
    char *ret = typeSpelling(retType);

    int nargs = clang_Cursor_getNumArguments(c);
    bool variadic = clang_Cursor_isVariadic(c) != 0;

    AnnotationList annotations = {0};
    clang_visitChildren(c, annotationVisitor, &annotations);

    bool skip = hasComma(name) || hasComma(fname) || hasComma(ret);
    if (annotations.comma) {
        skip = true;
    }

    char **argStrs = NULL;
    if (!skip && nargs > 0) {
        argStrs = (char **)malloc(sizeof(char *) * (size_t)nargs);
        for (int i = 0; i < nargs; i++) {
            CXCursor arg = clang_Cursor_getArgument(c, (unsigned)i);
            CXType at = clang_getCanonicalType(clang_getCursorType(arg));
            argStrs[i] = typeSpelling(at);
            if (hasComma(argStrs[i])) {
                skip = true;
            }
        }
    }

    if (skip) {
        if (ctx->logSkips) {
            fprintf(stderr,
                    "cheaderscan: skipping function '%s' at %s:%u (comma in field spelling)\n",
                    name ? name : "?", fname, line);
        }
        ctx->skippedCommas++;
    } else {
        fprintf(ctx->out, "F,%s,%s,%u,%s", name, fname, line, ret);
        if (argStrs) {
            for (int i = 0; i < nargs; i++) {
                fprintf(ctx->out, ",%s", argStrs[i]);
            }
        }
        if (variadic) {
            fputs(",...", ctx->out);
        }
        fputc('\n', ctx->out);
        ctx->emittedFunctions++;

        emitDeclarationEvidence(ctx, c, name, fname, line);

        for (int i = 0; i < annotations.count; i++) {
            fprintf(ctx->out, "A,%s,%s,%u,%s\n", name, fname, line, annotations.items[i]);
            ctx->emittedAnnotations++;
        }
    }

    annotationListFree(&annotations);
    if (argStrs) {
        for (int i = 0; i < nargs; i++) {
            free(argStrs[i]);
        }
        free(argStrs);
    }
    free(ret);
    clang_disposeString(nameStr);
    clang_disposeString(fileStr);
}

// **********************************************************************************************
// Record emit. Used for struct, class, and union definitions.
//
// Field collection happens up front so we can detect commas in any spelling and skip the
// whole record atomically (no half-written line).

typedef struct {
    char **names;
    char **types;
    int count;
    int capacity;
    bool comma;
} FieldList;

static void fieldListFree(FieldList *fl) {
    for (int i = 0; i < fl->count; i++) {
        free(fl->names[i]);
        free(fl->types[i]);
    }
    free(fl->names);
    free(fl->types);
    fl->names = NULL;
    fl->types = NULL;
    fl->count = 0;
    fl->capacity = 0;
    fl->comma = false;
}

static void fieldListAdd(FieldList *fl, const char *name, const char *type) {
    if (fl->count == fl->capacity) {
        fl->capacity = fl->capacity ? fl->capacity * 2 : 8;
        fl->names = (char **)realloc(fl->names, sizeof(char *) * fl->capacity);
        fl->types = (char **)realloc(fl->types, sizeof(char *) * fl->capacity);
        if (!fl->names || !fl->types) {
            fprintf(stderr, "cheaderscan: out of memory\n");
            exit(1);
        }
    }
    fl->names[fl->count] = strdup(name ? name : "");
    fl->types[fl->count] = strdup(type ? type : "");
    fl->count++;
}

static enum CXVisitorResult fieldVisitor(CXCursor field, CXClientData ud) {
    FieldList *fl = (FieldList *)ud;

    CXString nameStr = clang_getCursorSpelling(field);
    const char *name = clang_getCString(nameStr);

    CXType ft = clang_getCanonicalType(clang_getCursorType(field));
    char *type = typeSpelling(ft);

    if (hasComma(name) || hasComma(type)) {
        fl->comma = true;
    }
    fieldListAdd(fl, name, type);

    free(type);
    clang_disposeString(nameStr);
    return CXVisit_Continue;
}

static void emitRecord(EmitCtx *ctx, CXCursor c, char tag) {
    if (!clang_isCursorDefinition(c)) {
        return;
    }

    CXSourceLocation loc = clang_getCursorLocation(c);
    CXFile file = NULL;
    unsigned line = 0;
    clang_getFileLocation(loc, &file, &line, NULL, NULL);
    if (!file) {
        return;
    }

    CXString fileStr = clang_getFileName(file);
    const char *fname = clang_getCString(fileStr);
    if (!fname || !*fname) {
        clang_disposeString(fileStr);
        return;
    }

    CXType ct = clang_getCanonicalType(clang_getCursorType(c));
    CXString keyStr = clang_getTypeSpelling(ct);
    const char *key = clang_getCString(keyStr);

    if (!key || !*key || keySetContains(&ctx->seenRecords, key)) {
        clang_disposeString(keyStr);
        clang_disposeString(fileStr);
        return;
    }

    if (hasComma(key) || hasComma(fname)) {
        if (ctx->logSkips) {
            fprintf(stderr,
                    "cheaderscan: skipping record '%s' at %s:%u (comma in spelling)\n",
                    key, fname, line);
        }
        ctx->skippedCommas++;
        clang_disposeString(keyStr);
        clang_disposeString(fileStr);
        return;
    }

    FieldList fl = {0};
    clang_Type_visitFields(ct, fieldVisitor, &fl);

    if (fl.comma) {
        if (ctx->logSkips) {
            fprintf(stderr,
                    "cheaderscan: skipping record '%s' at %s:%u (comma in field spelling)\n",
                    key, fname, line);
        }
        ctx->skippedCommas++;
    } else {
        fprintf(ctx->out, "%c,%s,%s,%u", tag, key, fname, line);
        for (int i = 0; i < fl.count; i++) {
            fprintf(ctx->out, ",%s,%s", fl.names[i], fl.types[i]);
        }
        fputc('\n', ctx->out);
        ctx->emittedRecords++;
        keySetAdd(&ctx->seenRecords, key);
    }

    fieldListFree(&fl);
    clang_disposeString(keyStr);
    clang_disposeString(fileStr);
}

// **********************************************************************************************
// Macro collection: every macro a header defines, with the tokens it is defined as.

static void collectMacro(EmitCtx *ctx, CXCursor c) {
    if (clang_Cursor_isMacroBuiltin(c)) {
        return;
    }
    CXFile file = NULL;
    unsigned line = 0;
    clang_getFileLocation(clang_getCursorLocation(c), &file, &line, NULL, NULL);
    if (!file) {
        return;
    }
    CXString nameStr = clang_getCursorSpelling(c);
    CXString fileStr = clang_getFileName(file);
    const char *name = clang_getCString(nameStr);
    const char *fname = clang_getCString(fileStr);
    if (name && fname && *fname && !hasComma(fname)) {
        CXToken *tokens = NULL;
        unsigned tokenCount = 0;
        clang_tokenize(ctx->tu, clang_getCursorExtent(c), &tokens, &tokenCount);
        char **spellings = NULL;
        int spelled = 0;
        if (tokenCount > 1) {
            spellings = (char **)growArray(NULL, sizeof(char *) * (tokenCount - 1));
            for (unsigned t = 1; t < tokenCount; t++) {
                CXString spelling = clang_getTokenSpelling(ctx->tu, tokens[t]);
                spellings[spelled++] = strdup(clang_getCString(spelling));
                clang_disposeString(spelling);
            }
        }
        if (tokens) {
            clang_disposeTokens(ctx->tu, tokens, tokenCount);
        }
        macroListAdd(&ctx->macros, name, fname, line, clang_Cursor_isMacroFunctionLike(c) != 0,
                     spellings, spelled);
    }
    clang_disposeString(fileStr);
    clang_disposeString(nameStr);
}

static bool isUnsignedInteger(CXType type) {
    switch (clang_getCanonicalType(type).kind) {
        case CXType_Bool:
        case CXType_Char_U:
        case CXType_UChar:
        case CXType_UShort:
        case CXType_UInt:
        case CXType_ULong:
        case CXType_ULongLong:
        case CXType_UInt128:
            return true;
        default:
            return false;
    }
}

static void emitEnumConstant(EmitCtx *ctx, CXCursor c) {
    CXCursor declaration = clang_getCursorSemanticParent(c);
    if (clang_EnumDecl_isScoped(declaration) ||
        clang_getCursorKind(clang_getCursorSemanticParent(declaration)) == CXCursor_Namespace) {
        return;
    }
    CXFile file = NULL;
    unsigned line = 0;
    clang_getFileLocation(clang_getCursorLocation(c), &file, &line, NULL, NULL);
    if (!file) {
        return;
    }
    CXString nameStr = clang_getCursorSpelling(c);
    CXString fileStr = clang_getFileName(file);
    const char *name = clang_getCString(nameStr);
    const char *fname = clang_getCString(fileStr);
    if (name && fname && *fname && strncmp(name, "__", 2) != 0 && !hasComma(fname)) {
        fprintf(ctx->out, "K,%s,%s,%u,", name, fname, line);
        if (isUnsignedInteger(clang_getEnumDeclIntegerType(declaration))) {
            fprintf(ctx->out, "%llu\n", clang_getEnumConstantDeclUnsignedValue(c));
        } else {
            fprintf(ctx->out, "%lld\n", clang_getEnumConstantDeclValue(c));
        }
    }
    clang_disposeString(fileStr);
    clang_disposeString(nameStr);
}

// **********************************************************************************************
// Top-level visitor. Dispatches by cursor kind, recurses into namespaces / extern "C" / etc.

static enum CXChildVisitResult visit(CXCursor c, CXCursor parent, CXClientData ud) {
    (void)parent;
    EmitCtx *ctx = (EmitCtx *)ud;

    enum CXCursorKind kind = clang_getCursorKind(c);
    switch (kind) {
        case CXCursor_FunctionDecl:
            if (clang_getCursorKind(clang_getCursorSemanticParent(c)) != CXCursor_Namespace) {
                emitFunction(ctx, c);
            }
            break;
        case CXCursor_StructDecl:
        case CXCursor_ClassDecl:
            emitRecord(ctx, c, 'S');
            break;
        case CXCursor_UnionDecl:
            emitRecord(ctx, c, 'U');
            break;
        case CXCursor_MacroDefinition:
            collectMacro(ctx, c);
            break;
        case CXCursor_EnumConstantDecl:
            emitEnumConstant(ctx, c);
            break;
        default:
            break;
    }

    return CXChildVisit_Recurse;
}

// **********************************************************************************************
// An include the preprocessor could not find, named by the spelling of its diagnostic.

static void emitMissingInclude(FILE *out, CXDiagnostic d) {
    CXString spelling = clang_getDiagnosticSpelling(d);
    const char *text = clang_getCString(spelling);
    const char *suffix = "' file not found";
    size_t length = strlen(text);
    size_t suffixLength = strlen(suffix);
    if (text[0] == '\'' && length > suffixLength + 1 &&
        strcmp(text + length - suffixLength, suffix) == 0) {
        fprintf(out, "M,%.*s\n", (int)(length - suffixLength - 1), text + 1);
    }
    clang_disposeString(spelling);
}

// **********************************************************************************************
// The arguments every parse in a language is given, and the name of its translation unit.

static void parseArguments(ArgList *args, const char *lang, const ArgList *includes) {
    bool isCxx = (strcmp(lang, "c++") == 0);
    argsAdd(args, clangDriver);
    argsAdd(args, "-x");
    argsAdd(args, isCxx ? "c++" : "c");
    if (isCxx) {
        argsAdd(args, "-include");
        argsAdd(args, "cstdlib");
    }
    argsAdd(args, "-ferror-limit=0");
    argsAdd(args, "-w");
    for (int i = 0; i < includes->count; i++) {
        argsAdd(args, includes->data[i]);
    }
}

static const char *unitName(const char *lang) {
    return strcmp(lang, "c++") == 0 ? "_cheaderscan_tu.cpp" : "_cheaderscan_tu.c";
}

static CXTranslationUnit parseUnit(CXIndex index, const char *name, const ArgList *args,
                                   struct CXUnsavedFile *unsaved, unsigned options) {
    CXTranslationUnit unit = NULL;
    if (clang_parseTranslationUnit2FullArgv(index, name, args->data, args->count, unsaved, 1,
                                            options, &unit) != CXError_Success) {
        return NULL;
    }
    return unit;
}

// **********************************************************************************************
// Constant emit. A second parse includes the header and declares a variable for each object-like
// macro the first collected that can stand as an expression, initialized with the macro; each
// whose initializer evaluates to an integer is a K record of the macro's name, where it is
// defined, and the value. The names reserved to the implementation, which start with two
// underscores, are left out. The second parse's errors are those of macros that are not values,
// and fail nothing.

static const char *constantPrefix = "_cheaderscan_constant_";

typedef struct {
    FILE *out;
    const MacroList *macros;
    int emitted;
} ConstantCtx;

static enum CXChildVisitResult constantVisitor(CXCursor c, CXCursor parent, CXClientData ud) {
    (void)parent;
    ConstantCtx *ctx = (ConstantCtx *)ud;
    if (clang_getCursorKind(c) != CXCursor_VarDecl) {
        return CXChildVisit_Continue;
    }
    CXString nameStr = clang_getCursorSpelling(c);
    const char *name = clang_getCString(nameStr);
    size_t prefixLength = strlen(constantPrefix);
    if (name && strncmp(name, constantPrefix, prefixLength) == 0 && !clang_isInvalidDeclaration(c)) {
        int index = atoi(name + prefixLength);
        CXEvalResult result = clang_Cursor_Evaluate(c);
        if (result && index >= 0 && index < ctx->macros->count && clang_EvalResult_getKind(result) == CXEval_Int) {
            fprintf(ctx->out, "K,%s,%s,%u,", ctx->macros->names[index], ctx->macros->files[index],
                    ctx->macros->lines[index]);
            if (clang_EvalResult_isUnsignedInt(result)) {
                fprintf(ctx->out, "%llu\n", clang_EvalResult_getAsUnsigned(result));
            } else {
                fprintf(ctx->out, "%lld\n", clang_EvalResult_getAsLongLong(result));
            }
            ctx->emitted++;
        }
        if (result) {
            clang_EvalResult_dispose(result);
        }
    }
    clang_disposeString(nameStr);
    return CXChildVisit_Continue;
}

static void emitConstants(const char *lang, const ArgList *includes, const char *header,
                          MacroList *macros, FILE *out) {
    char *src = NULL;
    size_t length = 0;
    size_t capacity = 0;
    char line[512];
    snprintf(line, sizeof(line), "#include <%s>\n", header);
    appendString(&src, &length, &capacity, line);
    int declared = 0;
    for (int i = 0; i < macros->count; i++) {
        if (macros->functionLike[i] || strncmp(macros->names[i], "__", 2) == 0 ||
            macros->tokenCounts[i] == 0 || macroListFind(macros, macros->names[i]) != i ||
            !macroIsExpression(macros, i)) {
            continue;
        }
        snprintf(line, sizeof(line), "static __typeof__((%s)) const %s%d = (%s);\n",
                 macros->names[i], constantPrefix, i, macros->names[i]);
        appendString(&src, &length, &capacity, line);
        declared++;
    }
    if (declared == 0) {
        free(src);
        return;
    }

    ArgList args = {0};
    parseArguments(&args, lang, includes);
    struct CXUnsavedFile unsaved;
    unsaved.Filename = unitName(lang);
    unsaved.Contents = src;
    unsaved.Length = (unsigned long)length;

    CXIndex index = clang_createIndex(0, 0);
    CXTranslationUnit tu = parseUnit(index, unitName(lang), &args, &unsaved,
                                     CXTranslationUnit_SkipFunctionBodies | CXTranslationUnit_KeepGoing);
    if (tu) {
        ConstantCtx ctx = {out, macros, 0};
        clang_visitChildren(clang_getTranslationUnitCursor(tu), constantVisitor, &ctx);
        clang_disposeTranslationUnit(tu);
    }
    clang_disposeIndex(index);
    argsFree(&args);
    free(src);
}

// **********************************************************************************************
// One parse attempt in a given language. Returns total emitted records on success, -1 on hard
// failure (TU null, or errors with zero emissions).

static int parseAttempt(const char *lang,
                        const ArgList *includes,
                        const char *header,
                        FILE *out,
                        bool printDiags) {
    ArgList args = {0};
    parseArguments(&args, lang, includes);

    const char *tuName = unitName(lang);

    size_t bufLen = strlen(header) + 32;
    char *src = (char *)malloc(bufLen);
    snprintf(src, bufLen, "#include <%s>\n", header);

    struct CXUnsavedFile unsaved;
    unsaved.Filename = tuName;
    unsaved.Contents = src;
    unsaved.Length = (unsigned long)strlen(src);

    CXIndex index = clang_createIndex(0, 0);
    CXTranslationUnit tu = parseUnit(index, tuName, &args, &unsaved,
                                     CXTranslationUnit_SkipFunctionBodies | CXTranslationUnit_KeepGoing |
                                         CXTranslationUnit_DetailedPreprocessingRecord);

    int result = -1;

    if (tu) {
        int errorCount = 0;
        unsigned nDiags = clang_getNumDiagnostics(tu);
        for (unsigned i = 0; i < nDiags; i++) {
            CXDiagnostic d = clang_getDiagnostic(tu, i);
            enum CXDiagnosticSeverity sev = clang_getDiagnosticSeverity(d);
            if (sev >= CXDiagnostic_Error) {
                errorCount++;
                emitMissingInclude(out, d);
            }
            if (printDiags) {
                CXString text = clang_formatDiagnostic(
                    d, clang_defaultDiagnosticDisplayOptions());
                fprintf(stderr, "%s\n", clang_getCString(text));
                clang_disposeString(text);
            }
            clang_disposeDiagnostic(d);
        }

        EmitCtx ctx = {0};
        ctx.out = out;
        ctx.logSkips = printDiags;
        ctx.tu = tu;

        CXCursor root = clang_getTranslationUnitCursor(tu);
        clang_visitChildren(root, visit, &ctx);

        int totalEmitted = ctx.emittedFunctions + ctx.emittedRecords;
        if (errorCount > 0 && totalEmitted == 0) {
            result = -1;
        } else {
            result = totalEmitted;
        }

        if (result >= 0) {
            emitConstants(lang, includes, header, &ctx.macros, out);
        }
        keySetFree(&ctx.seenRecords);
        macroListFree(&ctx.macros);
        clang_disposeTranslationUnit(tu);
    } else if (printDiags) {
        fprintf(stderr, "cheaderscan: libclang could not parse %s\n", header);
    }

    clang_disposeIndex(index);
    free(src);
    argsFree(&args);

    return result;
}

static char *scanRecords(const char *lang,
                         const ArgList *includes,
                         const char *header,
                         bool printDiags,
                         int *result) {
    char *records = NULL;
    size_t length = 0;
    FILE *out = open_memstream(&records, &length);
    if (!out) {
        *result = -1;
        return NULL;
    }
    *result = parseAttempt(lang, includes, header, out, printDiags);
    fclose(out);
    return records;
}

const char *yumiCHeaderScanVersion(void) {
    return CHEADERSCAN_VERSION;
}

char *yumiCHeaderScan(const char *header, const char *language, const char *arguments, int32_t *status) {
    *status = 1;
    pthread_once(&libclangOnce, loadLibclangOnce);
    if (!libclangLoaded) {
        return NULL;
    }

    char *argumentText = strdup(arguments ? arguments : "");
    if (!argumentText) {
        return NULL;
    }
    ArgList includes = {0};
    char *cursor = argumentText;
    while (*cursor) {
        char *end = strchr(cursor, '\n');
        if (end) {
            *end = 0;
        }
        if (*cursor) {
            argsAdd(&includes, cursor);
        }
        if (!end) {
            break;
        }
        cursor = end + 1;
    }

    int result = -1;
    char *records = NULL;
    if (language && language[0]) {
        records = scanRecords(language, &includes, header, true, &result);
    } else {
        records = scanRecords("c++", &includes, header, false, &result);
        if (records && result < 0) {
            free(records);
            records = scanRecords("c", &includes, header, true, &result);
        }
    }

    argsFree(&includes);
    free(argumentText);
    *status = (records && result >= 0) ? 0 : 1;
    return records;
}

void yumiCHeaderScanFree(char *records) {
    free(records);
}
