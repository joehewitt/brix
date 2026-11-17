#include "include/BxSyntax.h"
#include "BxSyntaxLanguages.h"

#include <pthread.h>
#include <regex.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef enum {
    BxSyntaxPredicateEq,
    BxSyntaxPredicateMatch,
    BxSyntaxPredicateAnyOf
} BxSyntaxPredicateKind;

typedef struct {
    BxSyntaxPredicateKind kind;
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
} BxSyntaxPredicate;

typedef struct {
    BxSyntaxPredicate* predicates;
    uint32_t count;
} BxSyntaxPattern;

typedef struct {
    bool attempted;
    TSQuery* query;
    BxSyntaxPattern* patterns;
    uint32_t patternCount;
    char** captureNames;
    bool* hiddenCaptures;
    uint32_t captureCount;
    char error[160];
} BxSyntaxHighlightConfig;

typedef enum {
    BxSyntaxFoldNone,
    BxSyntaxFoldNode,
    BxSyntaxFoldAll,
    BxSyntaxFoldBody,
    BxSyntaxFoldRun
} BxSyntaxFoldKind;

typedef struct {
    bool attempted;
    TSQuery* query;
    BxSyntaxFoldKind* kinds;
    char error[160];
} BxSyntaxFoldConfig;

typedef struct {
    uint32_t start;
    uint32_t end;
    TSNode node;
} BxSyntaxFold;

typedef struct {
    BxSyntaxFold* items;
    uint32_t count;
    uint32_t capacity;
} BxSyntaxFoldList;

typedef struct {
    const char* text;
    uint32_t length;
} BxSyntaxInput;

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
    BxSyntaxFoldList folds;
    BxSyntaxFoldList runs;
} BxSyntaxDocument;

typedef struct {
    const char* bytes;
    uint32_t length;
} BxSyntaxText;

static BxSyntaxHighlightConfig* brixSyntaxConfigs = NULL;
static BxSyntaxFoldConfig* brixSyntaxFoldConfigs = NULL;
static pthread_mutex_t brixSyntaxConfigLock = PTHREAD_MUTEX_INITIALIZER;

// **********************************************************************************************

static bool brixSyntaxLanguageIsValid(int32_t language) {
    return language >= 0 && language < brixSyntaxLanguageEntryCount;
}

int32_t brixSyntaxLanguageCount(void) {
    return brixSyntaxLanguageEntryCount;
}

const char* brixSyntaxLanguageName(int32_t language) {
    return brixSyntaxLanguageIsValid(language) ? brixSyntaxLanguageEntries[language].name : "";
}

const char* brixSyntaxLanguageTitle(int32_t language) {
    return brixSyntaxLanguageIsValid(language) ? brixSyntaxLanguageEntries[language].title : "";
}

const char* brixSyntaxLanguageExtensions(int32_t language) {
    return brixSyntaxLanguageIsValid(language) ? brixSyntaxLanguageEntries[language].extensions : "";
}

// **********************************************************************************************

static char* brixSyntaxCopyString(const char* text, uint32_t length) {
    char* copy = malloc(length + 1);
    memcpy(copy, text, length);
    copy[length] = 0;
    return copy;
}

static void brixSyntaxAppend(char* output, size_t* length, const char* text) {
    size_t count = strlen(text);
    memcpy(output + *length, text, count);
    *length += count;
}

static char* brixSyntaxTranslatePattern(const char* pattern, uint32_t length) {
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
                brixSyntaxAppend(output, &count, replacement);
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

static bool brixSyntaxParsePredicate(
    BxSyntaxHighlightConfig* config,
    const TSQueryPredicateStep* steps,
    uint32_t count,
    BxSyntaxPredicate* predicate
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
        predicate->kind = BxSyntaxPredicateAnyOf;
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
            predicate->kind = BxSyntaxPredicateEq;
        } else if (strcmp(base, "match?") == 0) {
            predicate->kind = BxSyntaxPredicateMatch;
        } else {
            return false;
        }
    }

    if (steps[1].type != TSQueryPredicateStepTypeCapture) {
        return false;
    }
    predicate->capture = steps[1].value_id;

    if (predicate->kind == BxSyntaxPredicateAnyOf) {
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
        if (predicate->kind != BxSyntaxPredicateEq) {
            return false;
        }
        predicate->comparesCapture = true;
        predicate->otherCapture = steps[2].value_id;
        return true;
    }
    predicate->text = ts_query_string_value_for_id(config->query, steps[2].value_id, &predicate->textLength);
    if (predicate->kind == BxSyntaxPredicateMatch) {
        char* translated = brixSyntaxTranslatePattern(predicate->text, predicate->textLength);
        int status = regcomp(&predicate->regex, translated, REG_EXTENDED | REG_NOSUB);
        free(translated);
        if (status != 0) {
            return false;
        }
        predicate->hasRegex = true;
    }
    return true;
}

static void brixSyntaxParsePatterns(BxSyntaxHighlightConfig* config) {
    config->patternCount = ts_query_pattern_count(config->query);
    config->patterns = calloc(config->patternCount ? config->patternCount : 1, sizeof(BxSyntaxPattern));
    for (uint32_t pattern = 0; pattern < config->patternCount; pattern++) {
        uint32_t stepCount = 0;
        const TSQueryPredicateStep* steps = ts_query_predicates_for_pattern(config->query, pattern, &stepCount);
        uint32_t predicateCount = 0;
        for (uint32_t index = 0; index < stepCount; index++) {
            if (steps[index].type == TSQueryPredicateStepTypeDone) {
                predicateCount++;
            }
        }
        BxSyntaxPattern* target = &config->patterns[pattern];
        target->predicates = calloc(predicateCount ? predicateCount : 1, sizeof(BxSyntaxPredicate));
        uint32_t start = 0;
        for (uint32_t index = 0; index < stepCount; index++) {
            if (steps[index].type != TSQueryPredicateStepTypeDone) {
                continue;
            }
            if (brixSyntaxParsePredicate(config, steps + start, index - start, &target->predicates[target->count])) {
                target->count++;
            }
            start = index + 1;
        }
    }
}

static const char* brixSyntaxQueryErrorName(TSQueryError error) {
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

static char* brixSyntaxJoinQuery(const char* const* parts, uint32_t* length) {
    size_t capacity = 1;
    for (const char* const* part = parts; *part; part++) {
        capacity += strlen(*part) + 1;
    }
    char* source = malloc(capacity);
    size_t used = 0;
    for (const char* const* part = parts; *part; part++) {
        brixSyntaxAppend(source, &used, *part);
        brixSyntaxAppend(source, &used, "\n");
    }
    source[used] = 0;
    *length = (uint32_t)used;
    return source;
}

static BxSyntaxHighlightConfig* brixSyntaxHighlightConfig(int32_t language) {
    if (!brixSyntaxLanguageIsValid(language)) {
        return NULL;
    }
    pthread_mutex_lock(&brixSyntaxConfigLock);
    if (!brixSyntaxConfigs) {
        brixSyntaxConfigs = calloc(brixSyntaxLanguageEntryCount, sizeof(BxSyntaxHighlightConfig));
    }
    BxSyntaxHighlightConfig* config = &brixSyntaxConfigs[language];
    if (!config->attempted) {
        config->attempted = true;
        const BxSyntaxLanguageEntry* entry = &brixSyntaxLanguageEntries[language];
        uint32_t errorOffset = 0;
        TSQueryError errorType = TSQueryErrorNone;
        uint32_t sourceLength = 0;
        char* source = brixSyntaxJoinQuery(entry->highlights, &sourceLength);
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
                brixSyntaxQueryErrorName(errorType),
                errorOffset
            );
        } else {
            config->captureCount = ts_query_capture_count(config->query);
            config->captureNames = calloc(config->captureCount ? config->captureCount : 1, sizeof(char*));
            config->hiddenCaptures = calloc(config->captureCount ? config->captureCount : 1, sizeof(bool));
            for (uint32_t index = 0; index < config->captureCount; index++) {
                uint32_t length = 0;
                const char* name = ts_query_capture_name_for_id(config->query, index, &length);
                config->captureNames[index] = brixSyntaxCopyString(name, length);
                config->hiddenCaptures[index] = length == 0 || name[0] == '_';
            }
            brixSyntaxParsePatterns(config);
        }
    }
    pthread_mutex_unlock(&brixSyntaxConfigLock);
    return config->query ? config : NULL;
}

int32_t brixSyntaxCaptureCount(int32_t language) {
    BxSyntaxHighlightConfig* config = brixSyntaxHighlightConfig(language);
    return config ? (int32_t)config->captureCount : 0;
}

const char* brixSyntaxCaptureName(int32_t language, int32_t capture) {
    BxSyntaxHighlightConfig* config = brixSyntaxHighlightConfig(language);
    if (!config || capture < 0 || (uint32_t)capture >= config->captureCount) {
        return "";
    }
    return config->captureNames[capture];
}

const char* brixSyntaxQueryError(int32_t language) {
    if (!brixSyntaxLanguageIsValid(language)) {
        return "unknown language";
    }
    brixSyntaxHighlightConfig(language);
    return brixSyntaxConfigs[language].error;
}

// **********************************************************************************************

void* brixSyntaxDocumentCreate(int32_t language) {
    if (!brixSyntaxLanguageIsValid(language)) {
        return NULL;
    }
    TSParser* parser = ts_parser_new();
    if (!ts_parser_set_language(parser, brixSyntaxLanguageEntries[language].language())) {
        ts_parser_delete(parser);
        return NULL;
    }
    BxSyntaxDocument* document = calloc(1, sizeof(BxSyntaxDocument));
    document->language = language;
    document->parser = parser;
    document->cursor = ts_query_cursor_new();
    ts_query_cursor_set_match_limit(document->cursor, 1024);
    return document;
}

void brixSyntaxDocumentRelease(void* handle) {
    BxSyntaxDocument* document = handle;
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

void brixSyntaxDocumentEdit(
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
    BxSyntaxDocument* document = handle;
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

static uint64_t brixSyntaxMicroseconds(void) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (uint64_t)now.tv_sec * 1000000 + (uint64_t)now.tv_nsec / 1000;
}

static const char* brixSyntaxRead(void* payload, uint32_t byteIndex, TSPoint position, uint32_t* bytesRead) {
    BxSyntaxInput* input = payload;
    if (byteIndex >= input->length) {
        *bytesRead = 0;
        return "";
    }
    *bytesRead = input->length - byteIndex;
    return input->text + byteIndex;
}

static bool brixSyntaxParseProgress(TSParseState* state) {
    BxSyntaxDocument* document = state->payload;
    if (atomic_load_explicit(&document->canceled, memory_order_relaxed)) {
        return true;
    }
    return document->deadline && brixSyntaxMicroseconds() >= document->deadline;
}

static TSTree* brixSyntaxRunParser(BxSyntaxDocument* document, const TSTree* base, const char* text, uint32_t length) {
    BxSyntaxInput input = {text, length};
    TSInput source = {&input, brixSyntaxRead, TSInputEncodingUTF8, NULL};
    TSParseOptions options = {document, brixSyntaxParseProgress};
    return ts_parser_parse_with_options(document->parser, base, source, options);
}

static void brixSyntaxInstallTree(BxSyntaxDocument* document, TSTree* tree) {
    if (document->tree) {
        ts_tree_delete(document->tree);
    }
    document->tree = tree;
}

int32_t brixSyntaxDocumentParse(void* handle, const void* text, uint32_t length, uint32_t budgetMicroseconds) {
    BxSyntaxDocument* document = handle;
    if (!document || document->parsing) {
        return -1;
    }
    TSTree* base = document->tree ? ts_tree_copy(document->tree) : NULL;
    atomic_store(&document->canceled, false);
    document->deadline = budgetMicroseconds ? brixSyntaxMicroseconds() + budgetMicroseconds : 0;
    TSTree* next = brixSyntaxRunParser(document, base, text, length);
    document->deadline = 0;
    if (next) {
        if (base) {
            ts_tree_delete(base);
        }
        brixSyntaxInstallTree(document, next);
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

bool brixSyntaxDocumentContinueParse(void* handle) {
    BxSyntaxDocument* document = handle;
    if (!document || !document->parsing) {
        return false;
    }
    document->parseResult = brixSyntaxRunParser(
        document,
        document->parseBase,
        document->snapshot,
        document->snapshotLength
    );
    return document->parseResult != NULL;
}

bool brixSyntaxDocumentFinishParse(void* handle) {
    BxSyntaxDocument* document = handle;
    if (!document || !document->parsing) {
        return false;
    }
    TSTree* next = document->parseResult;
    bool edited = document->pendingEditCount > 0;
    if (next) {
        for (uint32_t index = 0; index < document->pendingEditCount; index++) {
            ts_tree_edit(next, &document->pendingEdits[index]);
        }
        brixSyntaxInstallTree(document, next);
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

void brixSyntaxDocumentCancelParse(void* handle) {
    BxSyntaxDocument* document = handle;
    if (document) {
        atomic_store(&document->canceled, true);
    }
}

bool brixSyntaxDocumentIsParsing(void* handle) {
    BxSyntaxDocument* document = handle;
    return document && document->parsing;
}

bool brixSyntaxDocumentHasTree(void* handle) {
    BxSyntaxDocument* document = handle;
    return document && document->tree;
}

bool brixSyntaxDocumentHasError(void* handle) {
    BxSyntaxDocument* document = handle;
    return document && document->tree && ts_node_has_error(ts_tree_root_node(document->tree));
}

// **********************************************************************************************

static const char* brixSyntaxNodeText(
    BxSyntaxDocument* document,
    const BxSyntaxText* text,
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

static bool brixSyntaxTextEquals(const char* left, uint32_t leftLength, const char* right, uint32_t rightLength) {
    return leftLength == rightLength && memcmp(left, right, leftLength) == 0;
}

static bool brixSyntaxPredicateNodePasses(
    BxSyntaxDocument* document,
    const BxSyntaxPredicate* predicate,
    const TSQueryMatch* match,
    const BxSyntaxText* text,
    TSNode node
) {
    uint32_t length = 0;
    bool result = false;
    switch (predicate->kind) {
        case BxSyntaxPredicateEq: {
            const char* value = brixSyntaxNodeText(document, text, node, &length, false);
            if (predicate->comparesCapture) {
                result = false;
                for (uint16_t index = 0; index < match->capture_count; index++) {
                    if (match->captures[index].index != predicate->otherCapture) {
                        continue;
                    }
                    uint32_t otherLength = 0;
                    const char* other =
                        brixSyntaxNodeText(document, text, match->captures[index].node, &otherLength, false);
                    result = brixSyntaxTextEquals(value, length, other, otherLength);
                    break;
                }
            } else {
                result = brixSyntaxTextEquals(value, length, predicate->text, predicate->textLength);
            }
            break;
        }
        case BxSyntaxPredicateMatch: {
            const char* value = brixSyntaxNodeText(document, text, node, &length, true);
            result = predicate->hasRegex && regexec(&predicate->regex, value, 0, NULL, 0) == 0;
            break;
        }
        case BxSyntaxPredicateAnyOf: {
            const char* value = brixSyntaxNodeText(document, text, node, &length, false);
            for (uint32_t index = 0; index < predicate->valueCount; index++) {
                if (brixSyntaxTextEquals(value, length, predicate->values[index], predicate->valueLengths[index])) {
                    result = true;
                    break;
                }
            }
            break;
        }
    }
    return result != predicate->negated;
}

static bool brixSyntaxPredicatePasses(
    BxSyntaxDocument* document,
    const BxSyntaxPredicate* predicate,
    const TSQueryMatch* match,
    const BxSyntaxText* text
) {
    for (uint16_t index = 0; index < match->capture_count; index++) {
        if (match->captures[index].index != predicate->capture) {
            continue;
        }
        bool passes = brixSyntaxPredicateNodePasses(document, predicate, match, text, match->captures[index].node);
        if (predicate->matchesAny && passes) {
            return true;
        }
        if (!predicate->matchesAny && !passes) {
            return false;
        }
    }
    return !predicate->matchesAny;
}

static bool brixSyntaxMatchPasses(
    BxSyntaxDocument* document,
    const BxSyntaxHighlightConfig* config,
    const TSQueryMatch* match,
    const BxSyntaxText* text
) {
    if (match->pattern_index >= config->patternCount) {
        return true;
    }
    const BxSyntaxPattern* pattern = &config->patterns[match->pattern_index];
    for (uint32_t index = 0; index < pattern->count; index++) {
        if (!brixSyntaxPredicatePasses(document, &pattern->predicates[index], match, text)) {
            return false;
        }
    }
    return true;
}

static void brixSyntaxEnsurePaint(BxSyntaxDocument* document, uint32_t width) {
    if (document->paintCapacity >= width) {
        return;
    }
    document->paintCapacity = width + 1024;
    document->paintCaptures = realloc(document->paintCaptures, document->paintCapacity * sizeof(int32_t));
    document->paintSizes = realloc(document->paintSizes, document->paintCapacity * sizeof(uint32_t));
}

uint32_t brixSyntaxDocumentHighlight(
    void* handle,
    const void* bytes,
    uint32_t length,
    uint32_t startByte,
    uint32_t endByte,
    void* spans,
    uint32_t capacity
) {
    BxSyntaxDocument* document = handle;
    if (!document || !document->tree) {
        return 0;
    }
    BxSyntaxHighlightConfig* config = brixSyntaxHighlightConfig(document->language);
    if (!config) {
        return 0;
    }
    if (endByte > length) {
        endByte = length;
    }
    if (startByte >= endByte) {
        return 0;
    }

    BxSyntaxText text = {bytes, length};
    uint32_t width = endByte - startByte;
    brixSyntaxEnsurePaint(document, width);
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
        if (!brixSyntaxMatchPasses(document, config, &match, &text)) {
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

static BxSyntaxFoldKind brixSyntaxFoldKindNamed(const char* name, uint32_t length) {
    if (brixSyntaxTextEquals(name, length, "fold", 4)) {
        return BxSyntaxFoldNode;
    }
    if (brixSyntaxTextEquals(name, length, "fold.all", 8)) {
        return BxSyntaxFoldAll;
    }
    if (brixSyntaxTextEquals(name, length, "fold.body", 9)) {
        return BxSyntaxFoldBody;
    }
    if (brixSyntaxTextEquals(name, length, "fold.run", 8)) {
        return BxSyntaxFoldRun;
    }
    return BxSyntaxFoldNone;
}

static BxSyntaxFoldConfig* brixSyntaxFoldConfig(int32_t language) {
    if (!brixSyntaxLanguageIsValid(language)) {
        return NULL;
    }
    pthread_mutex_lock(&brixSyntaxConfigLock);
    if (!brixSyntaxFoldConfigs) {
        brixSyntaxFoldConfigs = calloc(brixSyntaxLanguageEntryCount, sizeof(BxSyntaxFoldConfig));
    }
    BxSyntaxFoldConfig* config = &brixSyntaxFoldConfigs[language];
    const BxSyntaxLanguageEntry* entry = &brixSyntaxLanguageEntries[language];
    if (!config->attempted && entry->folds) {
        config->attempted = true;
        uint32_t errorOffset = 0;
        TSQueryError errorType = TSQueryErrorNone;
        uint32_t sourceLength = 0;
        char* source = brixSyntaxJoinQuery(entry->folds, &sourceLength);
        config->query = ts_query_new(entry->language(), source, sourceLength, &errorOffset, &errorType);
        free(source);
        if (!config->query) {
            snprintf(
                config->error,
                sizeof(config->error),
                "%s fold query %s error at offset %u",
                entry->name,
                brixSyntaxQueryErrorName(errorType),
                errorOffset
            );
        } else {
            uint32_t captureCount = ts_query_capture_count(config->query);
            config->kinds = calloc(captureCount ? captureCount : 1, sizeof(BxSyntaxFoldKind));
            for (uint32_t index = 0; index < captureCount; index++) {
                uint32_t length = 0;
                const char* name = ts_query_capture_name_for_id(config->query, index, &length);
                config->kinds[index] = brixSyntaxFoldKindNamed(name, length);
            }
        }
    }
    pthread_mutex_unlock(&brixSyntaxConfigLock);
    return config->query ? config : NULL;
}

const char* brixSyntaxFoldQueryError(int32_t language) {
    if (!brixSyntaxLanguageIsValid(language)) {
        return "unknown language";
    }
    brixSyntaxFoldConfig(language);
    return brixSyntaxFoldConfigs[language].error;
}

static void brixSyntaxFoldAdd(BxSyntaxFoldList* list, uint32_t start, uint32_t end, TSNode node) {
    if (list->count == list->capacity) {
        list->capacity = list->capacity ? list->capacity * 2 : 256;
        list->items = realloc(list->items, list->capacity * sizeof(BxSyntaxFold));
    }
    list->items[list->count++] = (BxSyntaxFold){start, end, node};
}

static int brixSyntaxFoldCompare(const void* left, const void* right) {
    const BxSyntaxFold* a = left;
    const BxSyntaxFold* b = right;
    if (a->start != b->start) {
        return a->start < b->start ? -1 : 1;
    }
    if (a->end != b->end) {
        return a->end < b->end ? -1 : 1;
    }
    return 0;
}

static uint32_t brixSyntaxLastRow(const BxSyntaxText* text, TSNode node) {
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

static bool brixSyntaxStartsLine(const BxSyntaxText* text, uint32_t offset) {
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

static void brixSyntaxCollectFold(
    BxSyntaxDocument* document,
    const BxSyntaxText* text,
    TSNode node,
    BxSyntaxFoldKind kind
) {
    uint32_t first = ts_node_start_point(node).row;
    uint32_t last = brixSyntaxLastRow(text, node);
    if (kind == BxSyntaxFoldRun) {
        if (brixSyntaxStartsLine(text, ts_node_start_byte(node))) {
            brixSyntaxFoldAdd(&document->runs, first, last, node);
        }
        return;
    }
    if (kind == BxSyntaxFoldNode) {
        uint32_t count = ts_node_child_count(node);
        if (count > 1 && last > first) {
            TSNode closer = ts_node_child(node, count - 1);
            if (ts_node_start_point(closer).row == last && brixSyntaxStartsLine(text, ts_node_start_byte(closer))) {
                last--;
            }
        }
    } else if (kind == BxSyntaxFoldBody) {
        TSNode previous = ts_node_prev_sibling(node);
        while (!ts_node_is_null(previous) && ts_node_is_extra(previous)) {
            previous = ts_node_prev_sibling(previous);
        }
        if (!ts_node_is_null(previous)) {
            first = ts_node_end_point(previous).row;
        }
    }
    if (last > first) {
        brixSyntaxFoldAdd(&document->folds, first, last, node);
    }
}

static bool brixSyntaxContinuesRun(const BxSyntaxText* text, TSNode node, TSSymbol symbol) {
    return !ts_node_is_null(node) && ts_node_symbol(node) == symbol &&
        brixSyntaxStartsLine(text, ts_node_start_byte(node));
}

static void brixSyntaxAddRun(
    BxSyntaxDocument* document,
    const BxSyntaxText* text,
    BxSyntaxFold run,
    TSNode tail,
    uint32_t firstRow,
    uint32_t lastRow
) {
    TSSymbol symbol = ts_node_symbol(run.node);
    if (run.start <= firstRow) {
        TSNode previous = ts_node_prev_sibling(run.node);
        while (brixSyntaxContinuesRun(text, previous, symbol) && brixSyntaxLastRow(text, previous) + 1 >= run.start) {
            run.start = ts_node_start_point(previous).row;
            previous = ts_node_prev_sibling(previous);
        }
    }
    if (run.end >= lastRow) {
        TSNode next = ts_node_next_sibling(tail);
        while (brixSyntaxContinuesRun(text, next, symbol) && ts_node_start_point(next).row <= run.end + 1) {
            uint32_t end = brixSyntaxLastRow(text, next);
            if (end > run.end) {
                run.end = end;
            }
            next = ts_node_next_sibling(next);
        }
    }
    if (run.end > run.start) {
        brixSyntaxFoldAdd(&document->folds, run.start, run.end, run.node);
    }
}

static void brixSyntaxMergeRuns(
    BxSyntaxDocument* document,
    const BxSyntaxText* text,
    uint32_t firstRow,
    uint32_t lastRow
) {
    BxSyntaxFoldList* runs = &document->runs;
    if (runs->count == 0) {
        return;
    }
    qsort(runs->items, runs->count, sizeof(BxSyntaxFold), brixSyntaxFoldCompare);
    BxSyntaxFold current = runs->items[0];
    TSNode tail = current.node;
    for (uint32_t index = 1; index < runs->count; index++) {
        BxSyntaxFold next = runs->items[index];
        if (ts_node_symbol(next.node) == ts_node_symbol(current.node) && next.start <= current.end + 1) {
            if (next.end >= current.end) {
                current.end = next.end;
                tail = next.node;
            }
            continue;
        }
        brixSyntaxAddRun(document, text, current, tail, firstRow, lastRow);
        current = next;
        tail = next.node;
    }
    brixSyntaxAddRun(document, text, current, tail, firstRow, lastRow);
}

uint32_t brixSyntaxDocumentFolds(
    void* handle,
    const void* bytes,
    uint32_t length,
    uint32_t firstRow,
    uint32_t lastRow,
    void* rows,
    uint32_t capacity
) {
    BxSyntaxDocument* document = handle;
    if (!document || !document->tree || firstRow > lastRow) {
        return 0;
    }
    BxSyntaxFoldConfig* config = brixSyntaxFoldConfig(document->language);
    if (!config) {
        return 0;
    }
    if (!document->foldCursor) {
        document->foldCursor = ts_query_cursor_new();
        ts_query_cursor_set_match_limit(document->foldCursor, 1024);
    }
    BxSyntaxText text = {bytes, length};
    document->folds.count = 0;
    document->runs.count = 0;
    TSPoint end = lastRow == UINT32_MAX ? (TSPoint){0, 0} : (TSPoint){lastRow + 1, 0};
    ts_query_cursor_set_point_range(document->foldCursor, (TSPoint){firstRow, 0}, end);
    ts_query_cursor_exec(document->foldCursor, config->query, ts_tree_root_node(document->tree));
    TSQueryMatch match;
    while (ts_query_cursor_next_match(document->foldCursor, &match)) {
        for (uint16_t index = 0; index < match.capture_count; index++) {
            const TSQueryCapture* capture = &match.captures[index];
            BxSyntaxFoldKind kind = config->kinds[capture->index];
            if (kind != BxSyntaxFoldNone) {
                brixSyntaxCollectFold(document, &text, capture->node, kind);
            }
        }
    }
    brixSyntaxMergeRuns(document, &text, firstRow, lastRow);

    BxSyntaxFoldList* folds = &document->folds;
    qsort(folds->items, folds->count, sizeof(BxSyntaxFold), brixSyntaxFoldCompare);
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
