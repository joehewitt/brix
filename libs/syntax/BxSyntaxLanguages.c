#include "BxSyntaxLanguages.h"

#include <stddef.h>

extern const TSLanguage* tree_sitter_c(void);
extern const TSLanguage* tree_sitter_cpp(void);
extern const TSLanguage* tree_sitter_objcpp(void);
extern const TSLanguage* tree_sitter_javascript(void);
extern const TSLanguage* tree_sitter_typescript(void);
extern const TSLanguage* tree_sitter_tsx(void);
extern const TSLanguage* tree_sitter_python(void);
extern const TSLanguage* tree_sitter_rust(void);
extern const TSLanguage* tree_sitter_go(void);
extern const TSLanguage* tree_sitter_json(void);
extern const TSLanguage* tree_sitter_css(void);
extern const TSLanguage* tree_sitter_html(void);
extern const TSLanguage* tree_sitter_bash(void);
extern const TSLanguage* tree_sitter_markdown(void);

extern const char tree_sitter_c_query_highlights[];
extern const char tree_sitter_cpp_query_highlights[];
extern const char tree_sitter_objcpp_query_highlights[];
extern const char tree_sitter_javascript_query_highlights[];
extern const char tree_sitter_javascript_query_highlights_jsx[];
extern const char tree_sitter_javascript_query_highlights_params[];
extern const char tree_sitter_typescript_query_highlights[];
extern const char tree_sitter_python_query_highlights[];
extern const char tree_sitter_rust_query_highlights[];
extern const char tree_sitter_go_query_highlights[];
extern const char tree_sitter_json_query_highlights[];
extern const char tree_sitter_css_query_highlights[];
extern const char tree_sitter_html_query_highlights[];
extern const char tree_sitter_bash_query_highlights[];
extern const char tree_sitter_markdown_query_highlights[];

static const char* const cHighlights[] = {
    tree_sitter_c_query_highlights,
    NULL
};

static const char* const cppHighlights[] = {
    tree_sitter_c_query_highlights,
    tree_sitter_cpp_query_highlights,
    NULL
};

static const char* const objcppHighlights[] = {
    tree_sitter_c_query_highlights,
    tree_sitter_objcpp_query_highlights,
    NULL
};

static const char* const javascriptHighlights[] = {
    tree_sitter_javascript_query_highlights,
    tree_sitter_javascript_query_highlights_jsx,
    tree_sitter_javascript_query_highlights_params,
    NULL
};

static const char* const typescriptHighlights[] = {
    tree_sitter_typescript_query_highlights,
    tree_sitter_javascript_query_highlights,
    NULL
};

static const char* const tsxHighlights[] = {
    tree_sitter_typescript_query_highlights,
    tree_sitter_javascript_query_highlights_jsx,
    tree_sitter_javascript_query_highlights,
    NULL
};

static const char* const pythonHighlights[] = {
    tree_sitter_python_query_highlights,
    NULL
};

static const char* const rustHighlights[] = {
    tree_sitter_rust_query_highlights,
    NULL
};

static const char* const goHighlights[] = {
    tree_sitter_go_query_highlights,
    NULL
};

static const char* const jsonHighlights[] = {
    tree_sitter_json_query_highlights,
    NULL
};

static const char* const cssHighlights[] = {
    tree_sitter_css_query_highlights,
    NULL
};

static const char* const htmlHighlights[] = {
    tree_sitter_html_query_highlights,
    NULL
};

static const char* const bashHighlights[] = {
    tree_sitter_bash_query_highlights,
    NULL
};

static const char* const markdownHighlights[] = {
    tree_sitter_markdown_query_highlights,
    NULL
};

static const char cFoldQuery[] =
    "[(compound_statement) (field_declaration_list) (enumerator_list) (initializer_list)"
    " (parameter_list) (argument_list) (preproc_if) (preproc_ifdef)] @fold\n"
    "[(preproc_else) (preproc_elif) (case_statement)] @fold.all\n"
    "(comment) @fold.run\n";

static const char cppFoldQuery[] =
    "[(declaration_list) (field_initializer_list) (template_parameter_list)"
    " (template_argument_list)] @fold\n"
    "(raw_string_literal) @fold.all\n";

static const char objcppFoldQuery[] =
    "[(class_interface) (class_implementation) (category_interface) (category_implementation)"
    " (protocol_declaration) (dictionary_expression) (array_expression) (message_expression)] @fold\n";

static const char javascriptFoldQuery[] =
    "[(statement_block) (class_body) (object) (array) (object_pattern) (array_pattern) (arguments)"
    " (formal_parameters) (switch_body) (named_imports) (export_clause) (template_string)"
    " (parenthesized_expression)] @fold\n"
    "[(switch_case) (switch_default)] @fold.all\n"
    "(comment) @fold.run\n";

static const char jsxFoldQuery[] =
    "[(jsx_element) (jsx_self_closing_element)] @fold\n";

static const char typescriptFoldQuery[] =
    "[(interface_body) (enum_body) (object_type) (type_parameters) (type_arguments) (tuple_type)] @fold\n";

static const char pythonFoldQuery[] =
    "(block) @fold.body\n"
    "[(list) (dictionary) (set) (tuple) (argument_list) (parameters) (list_comprehension)"
    " (dictionary_comprehension) (set_comprehension) (generator_expression)"
    " (parenthesized_expression) (import_from_statement) (string)] @fold\n"
    "(comment) @fold.run\n";

static const char rustFoldQuery[] =
    "[(block) (declaration_list) (field_declaration_list) (ordered_field_declaration_list)"
    " (enum_variant_list) (match_block) (arguments) (parameters) (array_expression)"
    " (tuple_expression) (use_list) (field_initializer_list) (token_tree) (type_parameters)"
    " (macro_definition) (block_comment)] @fold\n"
    "(line_comment) @fold.run\n";

static const char goFoldQuery[] =
    "[(block) (field_declaration_list) (interface_type) (literal_value) (argument_list)"
    " (parameter_list) (import_spec_list) (var_spec_list) (const_declaration) (type_declaration)"
    " (type_parameter_list) (expression_switch_statement) (type_switch_statement)"
    " (select_statement)] @fold\n"
    "[(expression_case) (default_case) (type_case) (communication_case) (raw_string_literal)] @fold.all\n"
    "(comment) @fold.run\n";

static const char jsonFoldQuery[] =
    "[(object) (array)] @fold\n"
    "(comment) @fold.run\n";

static const char cssFoldQuery[] =
    "[(block) (keyframe_block_list)] @fold\n"
    "(comment) @fold.run\n";

static const char htmlFoldQuery[] =
    "[(element) (script_element) (style_element)] @fold\n"
    "(comment) @fold.run\n";

static const char bashFoldQuery[] =
    "[(compound_statement) (do_group) (subshell) (array) (if_statement) (case_statement)"
    " (heredoc_redirect)] @fold\n"
    "[(case_item) (elif_clause) (else_clause)] @fold.all\n"
    "(comment) @fold.run\n";

static const char markdownFoldQuery[] =
    "[(section) (fenced_code_block) (block_quote) (list_item) (html_block)] @fold.all\n";

static const char* const cFolds[] = {cFoldQuery, NULL};
static const char* const cppFolds[] = {cFoldQuery, cppFoldQuery, NULL};
static const char* const objcppFolds[] = {cFoldQuery, cppFoldQuery, objcppFoldQuery, NULL};
static const char* const javascriptFolds[] = {javascriptFoldQuery, jsxFoldQuery, NULL};
static const char* const typescriptFolds[] = {javascriptFoldQuery, typescriptFoldQuery, NULL};
static const char* const tsxFolds[] = {javascriptFoldQuery, typescriptFoldQuery, jsxFoldQuery, NULL};
static const char* const pythonFolds[] = {pythonFoldQuery, NULL};
static const char* const rustFolds[] = {rustFoldQuery, NULL};
static const char* const goFolds[] = {goFoldQuery, NULL};
static const char* const jsonFolds[] = {jsonFoldQuery, NULL};
static const char* const cssFolds[] = {cssFoldQuery, NULL};
static const char* const htmlFolds[] = {htmlFoldQuery, NULL};
static const char* const bashFolds[] = {bashFoldQuery, NULL};
static const char* const markdownFolds[] = {markdownFoldQuery, NULL};

const BxSyntaxLanguageEntry brixSyntaxLanguageEntries[] = {
    {"c", "C", "c", tree_sitter_c, cHighlights, cFolds},
    {"cpp", "C++", "h hh hpp hxx h++ cc cpp cxx c++", tree_sitter_cpp, cppHighlights, cppFolds},
    {"objcpp", "Objective-C++", "mm", tree_sitter_objcpp, objcppHighlights, objcppFolds},
    {"javascript", "JavaScript", "js mjs cjs jsx", tree_sitter_javascript, javascriptHighlights, javascriptFolds},
    {"typescript", "TypeScript", "ts mts cts", tree_sitter_typescript, typescriptHighlights, typescriptFolds},
    {"tsx", "TSX", "tsx", tree_sitter_tsx, tsxHighlights, tsxFolds},
    {"python", "Python", "py pyi pyw", tree_sitter_python, pythonHighlights, pythonFolds},
    {"rust", "Rust", "rs", tree_sitter_rust, rustHighlights, rustFolds},
    {"go", "Go", "go", tree_sitter_go, goHighlights, goFolds},
    {"json", "JSON", "json jsonc", tree_sitter_json, jsonHighlights, jsonFolds},
    {"css", "CSS", "css", tree_sitter_css, cssHighlights, cssFolds},
    {"html", "HTML", "html htm", tree_sitter_html, htmlHighlights, htmlFolds},
    {"bash", "Bash", "sh bash zsh", tree_sitter_bash, bashHighlights, bashFolds},
    {"markdown", "Markdown", "md markdown", tree_sitter_markdown, markdownHighlights, markdownFolds}
};

const int32_t brixSyntaxLanguageEntryCount =
    (int32_t)(sizeof(brixSyntaxLanguageEntries) / sizeof(brixSyntaxLanguageEntries[0]));
