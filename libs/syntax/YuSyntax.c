#include "include/YuSyntax.h"
#include "YuSyntaxLanguages.h"

#include <pthread.h>
#include <regex.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef enum {
    YuSyntaxPredicateEq,
    YuSyntaxPredicateMatch,
    YuSyntaxPredicateAnyOf
} YuSyntaxPredicateKind;

typedef struct {
    YuSyntaxPredicateKind kind;
    bool negated;
    bool matchesAny;
    uint32_t capture;
    bool comparesCapture;
    uint32_t otherCapture;
    const char* text;
    uint32_t textLength;
    bool hasRegex;
    regex_t regex;
    const char** values;
    uint32_t* valueLengths;
    uint32_t valueCount;
} YuSyntaxPredicate;

typedef struct {
    YuSyntaxPredicate* predicates;
    uint32_t count;
} YuSyntaxPattern;

typedef struct {
    bool attempted;
    TSQuery* query;
    YuSyntaxPattern* patterns;
    uint32_t patternCount;
    char** captureNames;
    bool* hiddenCaptures;
    uint32_t captureCount;
    char error[160];
} YuSyntaxHighlightConfig;

typedef enum {
    YuSyntaxFoldNone,
    YuSyntaxFoldNode,
    YuSyntaxFoldAll,
    YuSyntaxFoldBody,
    YuSyntaxFoldRun
} YuSyntaxFoldKind;

typedef struct {
    bool attempted;
    TSQuery* query;
    YuSyntaxFoldKind* kinds;
    char error[160];
} YuSyntaxFoldConfig;

typedef struct {
    uint32_t start;
    uint32_t end;
    TSNode node;
} YuSyntaxFold;

typedef struct {
    YuSyntaxFold* items;
    uint32_t count;
    uint32_t capacity;
} YuSyntaxFoldList;

typedef struct {
    const char* text;
    uint32_t length;
} YuSyntaxInput;

typedef struct {
    int32_t language;
    TSParser* parser;
    TSTree* tree;
    TSQueryCursor* cursor;
    bool parsing;
    atomic_bool canceled;
    uint64_t deadline;
    TSTree* parseBase;
    TSTree* parseResult;
    char* snapshot;
    uint32_t snapshotLength;
    TSInputEdit* pendingEdits;
    uint32_t pendingEditCount;
    uint32_t pendingEditCapacity;
    int32_t* paintCaptures;
    uint32_t* paintSizes;
    uint32_t paintCapacity;
    char* scratch;
    uint32_t scratchCapacity;
    TSQueryCursor* foldCursor;
    YuSyntaxFoldList folds;
    YuSyntaxFoldList runs;
} YuSyntaxDocument;

typedef struct {
    const char* bytes;
    uint32_t length;
} YuSyntaxText;

static YuSyntaxHighlightConfig* yumiSyntaxConfigs = NULL;
static YuSyntaxFoldConfig* yumiSyntaxFoldConfigs = NULL;
static pthread_mutex_t yumiSyntaxConfigLock = PTHREAD_MUTEX_INITIALIZER;

// **********************************************************************************************

static bool yumiSyntaxLanguageIsValid(int32_t language) {
    return language >= 0 && language < yumiSyntaxLanguageEntryCount;
}

int32_t yumiSyntaxLanguageCount(void) {
    return yumiSyntaxLanguageEntryCount;
}

const char* yumiSyntaxLanguageName(int32_t language) {
    return yumiSyntaxLanguageIsValid(language) ? yumiSyntaxLanguageEntries[language].name : "";
}

const char* yumiSyntaxLanguageTitle(int32_t language) {
    return yumiSyntaxLanguageIsValid(language) ? yumiSyntaxLanguageEntries[language].title : "";
}

const char* yumiSyntaxLanguageExtensions(int32_t language) {
    return yumiSyntaxLanguageIsValid(language) ? yumiSyntaxLanguageEntries[language].extensions : "";
}

// **********************************************************************************************

static char* yumiSyntaxCopyString(const char* text, uint32_t length) {
    char* copy = malloc(length + 1);
    memcpy(copy, text, length);
    copy[length] = 0;
    return copy;
}

static void yumiSyntaxAppend(char* output, size_t* length, const char* text) {
    size_t count = strlen(text);
    memcpy(output + *length, text, count);
    *length += count;
}

static char* yumiSyntaxTranslatePattern(const char* pattern, uint32_t length) {
    char* output = malloc((size_t)length * 16 + 1);
    size_t count = 0;
    bool inBracket = false;
    for (uint32_t index = 0; index < length; index++) {
        char c = pattern[index];
        if (c == '\\' && index + 1 < length) {
            char escaped = pattern[++index];
            const char* replacement = NULL;
            switch (escaped) {
                case 'd':
                    replacement = inBracket ? "0-9" : "[0-9]";
                    break;
                case 'w':
                    replacement = inBracket ? "A-Za-z0-9_" : "[A-Za-z0-9_]";
                    break;
                case 's':
                    replacement = inBracket ? " \t\n\r\f\v" : "[[:space:]]";
                    break;
                case 'D':
                    replacement = inBracket ? NULL : "[^0-9]";
                    break;
                case 'W':
                    replacement = inBracket ? NULL : "[^A-Za-z0-9_]";
                    break;
                case 'S':
                    replacement = inBracket ? NULL : "[^[:space:]]";
                    break;
                case 'n':
                    replacement = "\n";
                    break;
                case 't':
                    replacement = "\t";
                    break;
                default:
                    break;
            }
            if (replacement) {
                yumiSyntaxAppend(output, &count, replacement);
            } else if (inBracket) {
                output[count++] = escaped;
            } else {
                output[count++] = '\\';
                output[count++] = escaped;
            }
            continue;
        }
        if (!inBracket && c == '[') {
            inBracket = true;
            output[count++] = c;
            if (index + 1 < length && pattern[index + 1] == '^') {
                output[count++] = pattern[++index];
            }
            if (index + 1 < length && pattern[index + 1] == ']') {
                output[count++] = pattern[++index];
            }
            continue;
        }
        if (inBracket && c == ']') {
            inBracket = false;
        }
        output[count++] = c;
    }
    output[count] = 0;
    return output;
}

static bool yumiSyntaxParsePredicate(
    YuSyntaxHighlightConfig* config,
    const TSQueryPredicateStep* steps,
    uint32_t count,
    YuSyntaxPredicate* predicate
) {
    if (count < 2 || steps[0].type != TSQueryPredicateStepTypeString) {
        return false;
    }
    uint32_t nameLength = 0;
    const char* name = ts_query_string_value_for_id(config->query, steps[0].value_id, &nameLength);
    char operator[32];
    if (nameLength >= sizeof(operator)) {
        return false;
    }
    memcpy(operator, name, nameLength);
    operator[nameLength] = 0;

    memset(predicate, 0, sizeof(*predicate));
    if (strcmp(operator, "any-of?") == 0 || strcmp(operator, "not-any-of?") == 0) {
        predicate->kind = YuSyntaxPredicateAnyOf;
        predicate->negated = operator[0] == 'n';
    } else {
        const char* base = operator;
        if (strncmp(base, "any-", 4) == 0) {
            predicate->matchesAny = true;
            base += 4;
        }
        if (strncmp(base, "not-", 4) == 0) {
            predicate->negated = true;
            base += 4;
        }
        if (strcmp(base, "eq?") == 0) {
            predicate->kind = YuSyntaxPredicateEq;
        } else if (strcmp(base, "match?") == 0) {
            predicate->kind = YuSyntaxPredicateMatch;
        } else {
            return false;
        }
    }

    if (steps[1].type != TSQueryPredicateStepTypeCapture) {
        return false;
    }
    predicate->capture = steps[1].value_id;

    if (predicate->kind == YuSyntaxPredicateAnyOf) {
        uint32_t valueCount = 0;
        for (uint32_t index = 2; index < count; index++) {
            if (steps[index].type == TSQueryPredicateStepTypeString) {
                valueCount++;
            }
        }
        predicate->values = calloc(valueCount ? valueCount : 1, sizeof(const char*));
        predicate->valueLengths = calloc(valueCount ? valueCount : 1, sizeof(uint32_t));
        for (uint32_t index = 2; index < count; index++) {
            if (steps[index].type != TSQueryPredicateStepTypeString) {
                continue;
            }
            uint32_t valueLength = 0;
            const char* value = ts_query_string_value_for_id(config->query, steps[index].value_id, &valueLength);
            predicate->values[predicate->valueCount] = value;
            predicate->valueLengths[predicate->valueCount] = valueLength;
            predicate->valueCount++;
        }
        return true;
    }

    if (count < 3) {
        return false;
    }
    if (steps[2].type == TSQueryPredicateStepTypeCapture) {
        if (predicate->kind != YuSyntaxPredicateEq) {
            return false;
        }
        predicate->comparesCapture = true;
        predicate->otherCapture = steps[2].value_id;
        return true;
    }
    predicate->text = ts_query_string_value_for_id(config->query, steps[2].value_id, &predicate->textLength);
    if (predicate->kind == YuSyntaxPredicateMatch) {
        char* translated = yumiSyntaxTranslatePattern(predicate->text, predicate->textLength);
        int status = regcomp(&predicate->regex, translated, REG_EXTENDED | REG_NOSUB);
        free(translated);
        if (status != 0) {
            return false;
        }
        predicate->hasRegex = true;
    }
    return true;
}

static void yumiSyntaxParsePatterns(YuSyntaxHighlightConfig* config) {
    config->patternCount = ts_query_pattern_count(config->query);
    config->patterns = calloc(config->patternCount ? config->patternCount : 1, sizeof(YuSyntaxPattern));
    for (uint32_t pattern = 0; pattern < config->patternCount; pattern++) {
        uint32_t stepCount = 0;
        const TSQueryPredicateStep* steps = ts_query_predicates_for_pattern(config->query, pattern, &stepCount);
        uint32_t predicateCount = 0;
        for (uint32_t index = 0; index < stepCount; index++) {
            if (steps[index].type == TSQueryPredicateStepTypeDone) {
                predicateCount++;
            }
        }
        YuSyntaxPattern* target = &config->patterns[pattern];
        target->predicates = calloc(predicateCount ? predicateCount : 1, sizeof(YuSyntaxPredicate));
        uint32_t start = 0;
        for (uint32_t index = 0; index < stepCount; index++) {
            if (steps[index].type != TSQueryPredicateStepTypeDone) {
                continue;
            }
            if (yumiSyntaxParsePredicate(config, steps + start, index - start, &target->predicates[target->count])) {
                target->count++;
            }
            start = index + 1;
        }
    }
}

static const char* yumiSyntaxQueryErrorName(TSQueryError error) {
    switch (error) {
        case TSQueryErrorSyntax:
            return "syntax";
        case TSQueryErrorNodeType:
            return "node type";
        case TSQueryErrorField:
            return "field";
        case TSQueryErrorCapture:
            return "capture";
        case TSQueryErrorStructure:
            return "structure";
        case TSQueryErrorLanguage:
            return "language";
        default:
            return "unknown";
    }
}

static char* yumiSyntaxJoinQuery(const char* const* parts, uint32_t* length) {
    size_t capacity = 1;
    for (const char* const* part = parts; *part; part++) {
        capacity += strlen(*part) + 1;
    }
    char* source = malloc(capacity);
    size_t used = 0;
    for (const char* const* part = parts; *part; part++) {
        yumiSyntaxAppend(source, &used, *part);
        yumiSyntaxAppend(source, &used, "\n");
    }
    source[used] = 0;
    *length = (uint32_t)used;
    return source;
}

static YuSyntaxHighlightConfig* yumiSyntaxHighlightConfig(int32_t language) {
    if (!yumiSyntaxLanguageIsValid(language)) {
        return NULL;
    }
    pthread_mutex_lock(&yumiSyntaxConfigLock);
    if (!yumiSyntaxConfigs) {
        yumiSyntaxConfigs = calloc(yumiSyntaxLanguageEntryCount, sizeof(YuSyntaxHighlightConfig));
    }
    YuSyntaxHighlightConfig* config = &yumiSyntaxConfigs[language];
    if (!config->attempted) {
        config->attempted = true;
        const YuSyntaxLanguageEntry* entry = &yumiSyntaxLanguageEntries[language];
        uint32_t errorOffset = 0;
        TSQueryError errorType = TSQueryErrorNone;
        uint32_t sourceLength = 0;
        char* source = yumiSyntaxJoinQuery(entry->highlights, &sourceLength);
        config->query = ts_query_new(
            entry->language(),
            source,
            sourceLength,
            &errorOffset,
            &errorType
        );
        free(source);
        if (!config->query) {
            snprintf(
                config->error,
                sizeof(config->error),
                "%s query %s error at offset %u",
                entry->name,
                yumiSyntaxQueryErrorName(errorType),
                errorOffset
            );
        } else {
            config->captureCount = ts_query_capture_count(config->query);
            config->captureNames = calloc(config->captureCount ? config->captureCount : 1, sizeof(char*));
            config->hiddenCaptures = calloc(config->captureCount ? config->captureCount : 1, sizeof(bool));
            for (uint32_t index = 0; index < config->captureCount; index++) {
                uint32_t length = 0;
                const char* name = ts_query_capture_name_for_id(config->query, index, &length);
                config->captureNames[index] = yumiSyntaxCopyString(name, length);
                config->hiddenCaptures[index] = length == 0 || name[0] == '_';
            }
            yumiSyntaxParsePatterns(config);
        }
    }
    pthread_mutex_unlock(&yumiSyntaxConfigLock);
    return config->query ? config : NULL;
}

int32_t yumiSyntaxCaptureCount(int32_t language) {
    YuSyntaxHighlightConfig* config = yumiSyntaxHighlightConfig(language);
    return config ? (int32_t)config->captureCount : 0;
}

const char* yumiSyntaxCaptureName(int32_t language, int32_t capture) {
    YuSyntaxHighlightConfig* config = yumiSyntaxHighlightConfig(language);
    if (!config || capture < 0 || (uint32_t)capture >= config->captureCount) {
        return "";
    }
    return config->captureNames[capture];
}

const char* yumiSyntaxQueryError(int32_t language) {
    if (!yumiSyntaxLanguageIsValid(language)) {
        return "unknown language";
    }
    yumiSyntaxHighlightConfig(language);
    return yumiSyntaxConfigs[language].error;
}

// **********************************************************************************************

void* yumiSyntaxDocumentCreate(int32_t language) {
    if (!yumiSyntaxLanguageIsValid(language)) {
        return NULL;
    }
    TSParser* parser = ts_parser_new();
    if (!ts_parser_set_language(parser, yumiSyntaxLanguageEntries[language].language())) {
        ts_parser_delete(parser);
        return NULL;
    }
    YuSyntaxDocument* document = calloc(1, sizeof(YuSyntaxDocument));
    document->language = language;
    document->parser = parser;
    document->cursor = ts_query_cursor_new();
    ts_query_cursor_set_match_limit(document->cursor, 1024);
    return document;
}

void yumiSyntaxDocumentRelease(void* handle) {
    YuSyntaxDocument* document = handle;
    if (!document) {
        return;
    }
    if (document->tree) {
        ts_tree_delete(document->tree);
    }
    if (document->parseBase) {
        ts_tree_delete(document->parseBase);
    }
    if (document->parseResult) {
        ts_tree_delete(document->parseResult);
    }
    ts_query_cursor_delete(document->cursor);
    if (document->foldCursor) {
        ts_query_cursor_delete(document->foldCursor);
    }
    ts_parser_delete(document->parser);
    free(document->snapshot);
    free(document->pendingEdits);
    free(document->paintCaptures);
    free(document->paintSizes);
    free(document->scratch);
    free(document->folds.items);
    free(document->runs.items);
    free(document);
}

void yumiSyntaxDocumentEdit(
    void* handle,
    uint32_t startByte,
    uint32_t oldEndByte,
    uint32_t newEndByte,
    uint32_t startRow,
    uint32_t startColumn,
    uint32_t oldEndRow,
    uint32_t oldEndColumn,
    uint32_t newEndRow,
    uint32_t newEndColumn
) {
    YuSyntaxDocument* document = handle;
    if (!document) {
        return;
    }
    TSInputEdit edit = {
        .start_byte = startByte,
        .old_end_byte = oldEndByte,
        .new_end_byte = newEndByte,
        .start_point = {startRow, startColumn},
        .old_end_point = {oldEndRow, oldEndColumn},
        .new_end_point = {newEndRow, newEndColumn},
    };
    if (document->tree) {
        ts_tree_edit(document->tree, &edit);
    }
    if (document->parsing) {
        if (document->pendingEditCount == document->pendingEditCapacity) {
            document->pendingEditCapacity = document->pendingEditCapacity ? document->pendingEditCapacity * 2 : 16;
            document->pendingEdits = realloc(
                document->pendingEdits,
                document->pendingEditCapacity * sizeof(TSInputEdit)
            );
        }
        document->pendingEdits[document->pendingEditCount++] = edit;
    }
}

static uint64_t yumiSyntaxMicroseconds(void) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (uint64_t)now.tv_sec * 1000000 + (uint64_t)now.tv_nsec / 1000;
}

static const char* yumiSyntaxRead(void* payload, uint32_t byteIndex, TSPoint position, uint32_t* bytesRead) {
    YuSyntaxInput* input = payload;
    if (byteIndex >= input->length) {
        *bytesRead = 0;
        return "";
    }
    *bytesRead = input->length - byteIndex;
    return input->text + byteIndex;
}

static bool yumiSyntaxParseProgress(TSParseState* state) {
    YuSyntaxDocument* document = state->payload;
    if (atomic_load_explicit(&document->canceled, memory_order_relaxed)) {
        return true;
    }
    return document->deadline && yumiSyntaxMicroseconds() >= document->deadline;
}

static TSTree* yumiSyntaxRunParser(YuSyntaxDocument* document, const TSTree* base, const char* text, uint32_t length) {
    YuSyntaxInput input = {text, length};
    TSInput source = {&input, yumiSyntaxRead, TSInputEncodingUTF8, NULL};
    TSParseOptions options = {document, yumiSyntaxParseProgress};
    return ts_parser_parse_with_options(document->parser, base, source, options);
}

static void yumiSyntaxInstallTree(YuSyntaxDocument* document, TSTree* tree) {
    if (document->tree) {
        ts_tree_delete(document->tree);
    }
    document->tree = tree;
}

int32_t yumiSyntaxDocumentParse(void* handle, const void* text, uint32_t length, uint32_t budgetMicroseconds) {
    YuSyntaxDocument* document = handle;
    if (!document || document->parsing) {
        return -1;
    }
    TSTree* base = document->tree ? ts_tree_copy(document->tree) : NULL;
    atomic_store(&document->canceled, false);
    document->deadline = budgetMicroseconds ? yumiSyntaxMicroseconds() + budgetMicroseconds : 0;
    TSTree* next = yumiSyntaxRunParser(document, base, text, length);
    document->deadline = 0;
    if (next) {
        if (base) {
            ts_tree_delete(base);
        }
        yumiSyntaxInstallTree(document, next);
        return 0;
    }
    document->parsing = true;
    document->parseBase = base;
    document->snapshot = malloc(length ? length : 1);
    memcpy(document->snapshot, text, length);
    document->snapshotLength = length;
    document->pendingEditCount = 0;
    return 1;
}

bool yumiSyntaxDocumentContinueParse(void* handle) {
    YuSyntaxDocument* document = handle;
    if (!document || !document->parsing) {
        return false;
    }
    document->parseResult = yumiSyntaxRunParser(
        document,
        document->parseBase,
        document->snapshot,
        document->snapshotLength
    );
    return document->parseResult != NULL;
}

bool yumiSyntaxDocumentFinishParse(void* handle) {
    YuSyntaxDocument* document = handle;
    if (!document || !document->parsing) {
        return false;
    }
    TSTree* next = document->parseResult;
    bool edited = document->pendingEditCount > 0;
    if (next) {
        for (uint32_t index = 0; index < document->pendingEditCount; index++) {
            ts_tree_edit(next, &document->pendingEdits[index]);
        }
        yumiSyntaxInstallTree(document, next);
    } else {
        ts_parser_reset(document->parser);
    }
    if (document->parseBase) {
        ts_tree_delete(document->parseBase);
    }
    free(document->snapshot);
    document->snapshot = NULL;
    document->snapshotLength = 0;
    document->parseBase = NULL;
    document->parseResult = NULL;
    document->pendingEditCount = 0;
    document->parsing = false;
    return edited;
}

void yumiSyntaxDocumentCancelParse(void* handle) {
    YuSyntaxDocument* document = handle;
    if (document) {
        atomic_store(&document->canceled, true);
    }
}

bool yumiSyntaxDocumentIsParsing(void* handle) {
    YuSyntaxDocument* document = handle;
    return document && document->parsing;
}

bool yumiSyntaxDocumentHasTree(void* handle) {
    YuSyntaxDocument* document = handle;
    return document && document->tree;
}

bool yumiSyntaxDocumentHasError(void* handle) {
    YuSyntaxDocument* document = handle;
    return document && document->tree && ts_node_has_error(ts_tree_root_node(document->tree));
}

// **********************************************************************************************

static const char* yumiSyntaxNodeText(
    YuSyntaxDocument* document,
    const YuSyntaxText* text,
    TSNode node,
    uint32_t* length,
    bool terminated
) {
    uint32_t start = ts_node_start_byte(node);
    uint32_t end = ts_node_end_byte(node);
    if (start > text->length) {
        start = text->length;
    }
    if (end > text->length) {
        end = text->length;
    }
    if (end < start) {
        end = start;
    }
    *length = end - start;
    if (!terminated) {
        return text->bytes + start;
    }
    if (document->scratchCapacity < *length + 1) {
        document->scratchCapacity = *length + 64;
        document->scratch = realloc(document->scratch, document->scratchCapacity);
    }
    memcpy(document->scratch, text->bytes + start, *length);
    document->scratch[*length] = 0;
    return document->scratch;
}

static bool yumiSyntaxTextEquals(const char* left, uint32_t leftLength, const char* right, uint32_t rightLength) {
    return leftLength == rightLength && memcmp(left, right, leftLength) == 0;
}

static bool yumiSyntaxPredicateNodePasses(
    YuSyntaxDocument* document,
    const YuSyntaxPredicate* predicate,
    const TSQueryMatch* match,
    const YuSyntaxText* text,
    TSNode node
) {
    uint32_t length = 0;
    bool result = false;
    switch (predicate->kind) {
        case YuSyntaxPredicateEq: {
            const char* value = yumiSyntaxNodeText(document, text, node, &length, false);
            if (predicate->comparesCapture) {
                result = false;
                for (uint16_t index = 0; index < match->capture_count; index++) {
                    if (match->captures[index].index != predicate->otherCapture) {
                        continue;
                    }
                    uint32_t otherLength = 0;
                    const char* other =
                        yumiSyntaxNodeText(document, text, match->captures[index].node, &otherLength, false);
                    result = yumiSyntaxTextEquals(value, length, other, otherLength);
                    break;
                }
            } else {
                result = yumiSyntaxTextEquals(value, length, predicate->text, predicate->textLength);
            }
            break;
        }
        case YuSyntaxPredicateMatch: {
            const char* value = yumiSyntaxNodeText(document, text, node, &length, true);
            result = predicate->hasRegex && regexec(&predicate->regex, value, 0, NULL, 0) == 0;
            break;
        }
        case YuSyntaxPredicateAnyOf: {
            const char* value = yumiSyntaxNodeText(document, text, node, &length, false);
            for (uint32_t index = 0; index < predicate->valueCount; index++) {
                if (yumiSyntaxTextEquals(value, length, predicate->values[index], predicate->valueLengths[index])) {
                    result = true;
                    break;
                }
            }
            break;
        }
    }
    return result != predicate->negated;
}

static bool yumiSyntaxPredicatePasses(
    YuSyntaxDocument* document,
    const YuSyntaxPredicate* predicate,
    const TSQueryMatch* match,
    const YuSyntaxText* text
) {
    for (uint16_t index = 0; index < match->capture_count; index++) {
        if (match->captures[index].index != predicate->capture) {
            continue;
        }
        bool passes = yumiSyntaxPredicateNodePasses(document, predicate, match, text, match->captures[index].node);
        if (predicate->matchesAny && passes) {
            return true;
        }
        if (!predicate->matchesAny && !passes) {
            return false;
        }
    }
    return !predicate->matchesAny;
}

static bool yumiSyntaxMatchPasses(
    YuSyntaxDocument* document,
    const YuSyntaxHighlightConfig* config,
    const TSQueryMatch* match,
    const YuSyntaxText* text
) {
    if (match->pattern_index >= config->patternCount) {
        return true;
    }
    const YuSyntaxPattern* pattern = &config->patterns[match->pattern_index];
    for (uint32_t index = 0; index < pattern->count; index++) {
        if (!yumiSyntaxPredicatePasses(document, &pattern->predicates[index], match, text)) {
            return false;
        }
    }
    return true;
}

static void yumiSyntaxEnsurePaint(YuSyntaxDocument* document, uint32_t width) {
    if (document->paintCapacity >= width) {
        return;
    }
    document->paintCapacity = width + 1024;
    document->paintCaptures = realloc(document->paintCaptures, document->paintCapacity * sizeof(int32_t));
    document->paintSizes = realloc(document->paintSizes, document->paintCapacity * sizeof(uint32_t));
}

uint32_t yumiSyntaxDocumentHighlight(
    void* handle,
    const void* bytes,
    uint32_t length,
    uint32_t startByte,
    uint32_t endByte,
    void* spans,
    uint32_t capacity
) {
    YuSyntaxDocument* document = handle;
    if (!document || !document->tree) {
        return 0;
    }
    YuSyntaxHighlightConfig* config = yumiSyntaxHighlightConfig(document->language);
    if (!config) {
        return 0;
    }
    if (endByte > length) {
        endByte = length;
    }
    if (startByte >= endByte) {
        return 0;
    }

    YuSyntaxText text = {bytes, length};
    uint32_t width = endByte - startByte;
    yumiSyntaxEnsurePaint(document, width);
    int32_t* captures = document->paintCaptures;
    uint32_t* sizes = document->paintSizes;
    for (uint32_t index = 0; index < width; index++) {
        captures[index] = -1;
        sizes[index] = UINT32_MAX;
    }

    ts_query_cursor_set_byte_range(document->cursor, startByte, endByte);
    ts_query_cursor_exec(document->cursor, config->query, ts_tree_root_node(document->tree));
    TSQueryMatch match;
    uint32_t captureIndex = 0;
    while (ts_query_cursor_next_capture(document->cursor, &match, &captureIndex)) {
        if (!yumiSyntaxMatchPasses(document, config, &match, &text)) {
            ts_query_cursor_remove_match(document->cursor, match.id);
            continue;
        }
        const TSQueryCapture* capture = &match.captures[captureIndex];
        if (config->hiddenCaptures[capture->index]) {
            continue;
        }
        uint32_t nodeStart = ts_node_start_byte(capture->node);
        uint32_t nodeEnd = ts_node_end_byte(capture->node);
        uint32_t size = nodeEnd - nodeStart;
        uint32_t from = nodeStart > startByte ? nodeStart : startByte;
        uint32_t to = nodeEnd < endByte ? nodeEnd : endByte;
        for (uint32_t offset = from; offset < to; offset++) {
            uint32_t index = offset - startByte;
            if (sizes[index] >= size) {
                captures[index] = (int32_t)capture->index;
                sizes[index] = size;
            }
        }
    }

    uint32_t* output = spans;
    uint32_t count = 0;
    uint32_t index = 0;
    while (index < width) {
        int32_t capture = captures[index];
        uint32_t next = index + 1;
        while (next < width && captures[next] == capture) {
            next++;
        }
        if (capture >= 0) {
            if (count < capacity) {
                output[count * 3] = startByte + index;
                output[count * 3 + 1] = startByte + next;
                output[count * 3 + 2] = (uint32_t)capture;
            }
            count++;
        }
        index = next;
    }
    return count;
}

// **********************************************************************************************

static YuSyntaxFoldKind yumiSyntaxFoldKindNamed(const char* name, uint32_t length) {
    if (yumiSyntaxTextEquals(name, length, "fold", 4)) {
        return YuSyntaxFoldNode;
    }
    if (yumiSyntaxTextEquals(name, length, "fold.all", 8)) {
        return YuSyntaxFoldAll;
    }
    if (yumiSyntaxTextEquals(name, length, "fold.body", 9)) {
        return YuSyntaxFoldBody;
    }
    if (yumiSyntaxTextEquals(name, length, "fold.run", 8)) {
        return YuSyntaxFoldRun;
    }
    return YuSyntaxFoldNone;
}

static YuSyntaxFoldConfig* yumiSyntaxFoldConfig(int32_t language) {
    if (!yumiSyntaxLanguageIsValid(language)) {
        return NULL;
    }
    pthread_mutex_lock(&yumiSyntaxConfigLock);
    if (!yumiSyntaxFoldConfigs) {
        yumiSyntaxFoldConfigs = calloc(yumiSyntaxLanguageEntryCount, sizeof(YuSyntaxFoldConfig));
    }
    YuSyntaxFoldConfig* config = &yumiSyntaxFoldConfigs[language];
    const YuSyntaxLanguageEntry* entry = &yumiSyntaxLanguageEntries[language];
    if (!config->attempted && entry->folds) {
        config->attempted = true;
        uint32_t errorOffset = 0;
        TSQueryError errorType = TSQueryErrorNone;
        uint32_t sourceLength = 0;
        char* source = yumiSyntaxJoinQuery(entry->folds, &sourceLength);
        config->query = ts_query_new(entry->language(), source, sourceLength, &errorOffset, &errorType);
        free(source);
        if (!config->query) {
            snprintf(
                config->error,
                sizeof(config->error),
                "%s fold query %s error at offset %u",
                entry->name,
                yumiSyntaxQueryErrorName(errorType),
                errorOffset
            );
        } else {
            uint32_t captureCount = ts_query_capture_count(config->query);
            config->kinds = calloc(captureCount ? captureCount : 1, sizeof(YuSyntaxFoldKind));
            for (uint32_t index = 0; index < captureCount; index++) {
                uint32_t length = 0;
                const char* name = ts_query_capture_name_for_id(config->query, index, &length);
                config->kinds[index] = yumiSyntaxFoldKindNamed(name, length);
            }
        }
    }
    pthread_mutex_unlock(&yumiSyntaxConfigLock);
    return config->query ? config : NULL;
}

const char* yumiSyntaxFoldQueryError(int32_t language) {
    if (!yumiSyntaxLanguageIsValid(language)) {
        return "unknown language";
    }
    yumiSyntaxFoldConfig(language);
    return yumiSyntaxFoldConfigs[language].error;
}

static void yumiSyntaxFoldAdd(YuSyntaxFoldList* list, uint32_t start, uint32_t end, TSNode node) {
    if (list->count == list->capacity) {
        list->capacity = list->capacity ? list->capacity * 2 : 256;
        list->items = realloc(list->items, list->capacity * sizeof(YuSyntaxFold));
    }
    list->items[list->count++] = (YuSyntaxFold){start, end, node};
}

static int yumiSyntaxFoldCompare(const void* left, const void* right) {
    const YuSyntaxFold* a = left;
    const YuSyntaxFold* b = right;
    if (a->start != b->start) {
        return a->start < b->start ? -1 : 1;
    }
    if (a->end != b->end) {
        return a->end < b->end ? -1 : 1;
    }
    return 0;
}

static uint32_t yumiSyntaxLastRow(const YuSyntaxText* text, TSNode node) {
    uint32_t row = ts_node_end_point(node).row;
    uint32_t start = ts_node_start_byte(node);
    uint32_t offset = ts_node_end_byte(node);
    if (offset > text->length) {
        offset = text->length;
    }
    while (offset > start) {
        char c = text->bytes[offset - 1];
        if (c == '\n') {
            row--;
        } else if (c != ' ' && c != '\t' && c != '\r') {
            break;
        }
        offset--;
    }
    return row;
}

static bool yumiSyntaxStartsLine(const YuSyntaxText* text, uint32_t offset) {
    if (offset > text->length) {
        return false;
    }
    while (offset > 0) {
        char c = text->bytes[offset - 1];
        if (c == '\n') {
            return true;
        }
        if (c != ' ' && c != '\t') {
            return false;
        }
        offset--;
    }
    return true;
}

static void yumiSyntaxCollectFold(
    YuSyntaxDocument* document,
    const YuSyntaxText* text,
    TSNode node,
    YuSyntaxFoldKind kind
) {
    uint32_t first = ts_node_start_point(node).row;
    uint32_t last = yumiSyntaxLastRow(text, node);
    if (kind == YuSyntaxFoldRun) {
        if (yumiSyntaxStartsLine(text, ts_node_start_byte(node))) {
            yumiSyntaxFoldAdd(&document->runs, first, last, node);
        }
        return;
    }
    if (kind == YuSyntaxFoldNode) {
        uint32_t count = ts_node_child_count(node);
        if (count > 1 && last > first) {
            TSNode closer = ts_node_child(node, count - 1);
            if (ts_node_start_point(closer).row == last && yumiSyntaxStartsLine(text, ts_node_start_byte(closer))) {
                last--;
            }
        }
    } else if (kind == YuSyntaxFoldBody) {
        TSNode previous = ts_node_prev_sibling(node);
        while (!ts_node_is_null(previous) && ts_node_is_extra(previous)) {
            previous = ts_node_prev_sibling(previous);
        }
        if (!ts_node_is_null(previous)) {
            first = ts_node_end_point(previous).row;
        }
    }
    if (last > first) {
        yumiSyntaxFoldAdd(&document->folds, first, last, node);
    }
}

static bool yumiSyntaxContinuesRun(const YuSyntaxText* text, TSNode node, TSSymbol symbol) {
    return !ts_node_is_null(node) && ts_node_symbol(node) == symbol &&
        yumiSyntaxStartsLine(text, ts_node_start_byte(node));
}

static void yumiSyntaxAddRun(
    YuSyntaxDocument* document,
    const YuSyntaxText* text,
    YuSyntaxFold run,
    TSNode tail,
    uint32_t firstRow,
    uint32_t lastRow
) {
    TSSymbol symbol = ts_node_symbol(run.node);
    if (run.start <= firstRow) {
        TSNode previous = ts_node_prev_sibling(run.node);
        while (yumiSyntaxContinuesRun(text, previous, symbol) && yumiSyntaxLastRow(text, previous) + 1 >= run.start) {
            run.start = ts_node_start_point(previous).row;
            previous = ts_node_prev_sibling(previous);
        }
    }
    if (run.end >= lastRow) {
        TSNode next = ts_node_next_sibling(tail);
        while (yumiSyntaxContinuesRun(text, next, symbol) && ts_node_start_point(next).row <= run.end + 1) {
            uint32_t end = yumiSyntaxLastRow(text, next);
            if (end > run.end) {
                run.end = end;
            }
            next = ts_node_next_sibling(next);
        }
    }
    if (run.end > run.start) {
        yumiSyntaxFoldAdd(&document->folds, run.start, run.end, run.node);
    }
}

static void yumiSyntaxMergeRuns(
    YuSyntaxDocument* document,
    const YuSyntaxText* text,
    uint32_t firstRow,
    uint32_t lastRow
) {
    YuSyntaxFoldList* runs = &document->runs;
    if (runs->count == 0) {
        return;
    }
    qsort(runs->items, runs->count, sizeof(YuSyntaxFold), yumiSyntaxFoldCompare);
    YuSyntaxFold current = runs->items[0];
    TSNode tail = current.node;
    for (uint32_t index = 1; index < runs->count; index++) {
        YuSyntaxFold next = runs->items[index];
        if (ts_node_symbol(next.node) == ts_node_symbol(current.node) && next.start <= current.end + 1) {
            if (next.end >= current.end) {
                current.end = next.end;
                tail = next.node;
            }
            continue;
        }
        yumiSyntaxAddRun(document, text, current, tail, firstRow, lastRow);
        current = next;
        tail = next.node;
    }
    yumiSyntaxAddRun(document, text, current, tail, firstRow, lastRow);
}

uint32_t yumiSyntaxDocumentFolds(
    void* handle,
    const void* bytes,
    uint32_t length,
    uint32_t firstRow,
    uint32_t lastRow,
    void* rows,
    uint32_t capacity
) {
    YuSyntaxDocument* document = handle;
    if (!document || !document->tree || firstRow > lastRow) {
        return 0;
    }
    YuSyntaxFoldConfig* config = yumiSyntaxFoldConfig(document->language);
    if (!config) {
        return 0;
    }
    if (!document->foldCursor) {
        document->foldCursor = ts_query_cursor_new();
        ts_query_cursor_set_match_limit(document->foldCursor, 1024);
    }
    YuSyntaxText text = {bytes, length};
    document->folds.count = 0;
    document->runs.count = 0;
    TSPoint end = lastRow == UINT32_MAX ? (TSPoint){0, 0} : (TSPoint){lastRow + 1, 0};
    ts_query_cursor_set_point_range(document->foldCursor, (TSPoint){firstRow, 0}, end);
    ts_query_cursor_exec(document->foldCursor, config->query, ts_tree_root_node(document->tree));
    TSQueryMatch match;
    while (ts_query_cursor_next_match(document->foldCursor, &match)) {
        for (uint16_t index = 0; index < match.capture_count; index++) {
            const TSQueryCapture* capture = &match.captures[index];
            YuSyntaxFoldKind kind = config->kinds[capture->index];
            if (kind != YuSyntaxFoldNone) {
                yumiSyntaxCollectFold(document, &text, capture->node, kind);
            }
        }
    }
    yumiSyntaxMergeRuns(document, &text, firstRow, lastRow);

    YuSyntaxFoldList* folds = &document->folds;
    qsort(folds->items, folds->count, sizeof(YuSyntaxFold), yumiSyntaxFoldCompare);
    uint32_t* output = rows;
    uint32_t count = 0;
    for (uint32_t index = 0; index < folds->count; index++) {
        if (index > 0 && folds->items[index].start == folds->items[index - 1].start) {
            continue;
        }
        if (count < capacity) {
            output[count * 2] = folds->items[index].start;
            output[count * 2 + 1] = folds->items[index].end;
        }
        count++;
    }
    return count;
}
