# GUI Code Editor

The code editor is split across two libraries. Brix has an editor of its own, which shows forms as rows.

| Concern | Where | Notes |
| --- | --- | --- |
| Text and editing | `stdlib/code` | `CodeText`, `CodeSession` (caret, selection, undo, parse, highlights), `CodeDocument` (a session bound to a file) |
| Find and replace | `stdlib/code/code-find.bx` | `CodeFind` searches a `CodeFindTarget` with case, whole-word and regex options, and replaces one hit or all |
| Folding | `stdlib/code/code-folds.bx` | `CodeFolds`, the collapsed ranges of a session (`CodeSession.folds`) |
| Parsing and roles | `stdlib/code/syntax.bx` | tree-sitter documents and queries, `SyntaxRole`, `syntax-styled-runs` |
| View | `stdlib/gui/code-editor` | `CodeEditor`, `CodeGutter`, `CodeOverview`, `CodeLinkCard`, `CodeSyntaxStyle` |
| Find bar | `stdlib/gui/code-editor/code-find-bar.bx` | `FindBar` runs a `CodeFind` over its `target`; `FindOptions` keeps the case, word and regex toggles in an `AppState` |
| Brix rows | `stdlib/gui/source-editor` | `SourceEditor` shows a `SourceSession` as rows, `SourceListingPane` shows the forms beside it, and `SourceSurface` paints the boxes behind them |
| Row styles | `stdlib/gui/source-editor/source-style.bx` | `SourceStructureTheme`, the divider heights, and `source-span-color`, `source-preamble-color` and `source-is-bold`, which style a span by its kind on any element that uses the theme; the editor's rows and Markdown's code spans share them |
| Soft wrap | `stdlib/gui/source-editor/source-wrap.bx` | `source-wrap` breaks a row into visual lines, `SourceWrap` maps columns to and from them, and `SourceWrapRows` gives each shown row its height |

`CodeEditor.show(session, scroll)` displays a `CodeSession`; several editors may show one session.
The editor never reads or writes files. It publishes `save-requested`, and the owner saves, usually
through `CodeDocument.save()`, which writes atomically through a temporary file and keeps the
file's permissions. `CodeDocument.watch()` reloads an unmodified document when its file changes on
disk and sets `is-changed-on-disk` instead when there are unsaved edits.

## Editing

| Keys | Action |
| --- | --- |
| ⌘S | Publish `save-requested` |
| ⌘Z, ⇧⌘Z, ⌘Y | Undo, redo |
| ⌘C, ⌘X, ⌘V | Copy, cut, paste |
| ⌘A | Select all |
| Tab, ⇧Tab | Indent or outdent; Tab inserts one indent unit when nothing is selected |
| ⌘], ⌘[ | Indent or outdent the selected lines |
| Return | New line with the current indentation, one more level after `{ ( [ :`, and bracket pairs split onto their own lines |
| Arrows, ⌥ arrows, ⌘ arrows | Character, word, and line or document movement; add ⇧ to extend |
| ⌘← | First non-blank character, then column zero |
| Double-click, triple-click | Select word, select line |

The Edit menu's roles reach the editor through `perform-edit`. Typing a closing bracket on a blank
line outdents it. Backspace in leading spaces removes one indent unit. Consecutive typing coalesces
into one undo step. Indentation style, tabs or two, four, or eight spaces, is detected from the
text. Tabs display at four columns. CRLF text is edited as LF and saved as CRLF. Wide characters
count as two columns, which matches most monospace fonts but is not exact.

`CodeEditor` is a `CodeFindTarget`; the find bar itself is an application view. Fold ranges come
from each language's tree-sitter fold query. The gutter shows a fold arrow on each foldable line,
and clicking it toggles the fold. `fold`, `unfold`, `fold-all` and `unfold-all` act on the session.

Not supported: IME composition and multiple carets. `CodeEditor` keeps each line on one visual
line and scrolls sideways; `SourceEditor` wraps its rows, as Soft Wrap describes.

## Soft Wrap

`SourceEditor` wraps rows that do not fit the visible text area, which ends one box indent short
of the box's right edge, and rewraps them when the pane resizes, keeping the row at the top of the
pane in place. `wraps-rows`, true by default, is the only switch; code that creates an editor sets
it to false to keep each row on one line, with sideways scrolling as before. There is no setting,
menu item or command for it. Wrapping changes only how rows are laid out, never the text.

- **Breaks.** A line breaks after the last space that fits. A word wider than the line is broken
  where the line ends. Dividers never wrap.
- **Continuation indent.** Each later line of a code row starts at the row's leading spaces plus
  one indent unit, `source-indent-columns`. A row the language renders with `is-prose`, such as a
  line of a Brix text body or text block, has no extra indent: its later lines start under its
  first. When the indent leaves fewer than `source-wrap-least` (20) columns, it shrinks so each line
  keeps at least that many, and a row indented past the pane overflows it rather than wrapping a
  character per line.
- **Caret.** A column at a break belongs to the line it starts. Up and Down move by visual line and
  keep the caret's visual column across rows; Page Up and Page Down move by a page of visual
  lines. ⌘← and ⌘→, and Home and End, go to the start and end of the visual line, and the end of a
  line that continues is just before its break. ⌃A and ⌃E still go to the start and end of the
  whole row. A click maps to the column under it on the clicked visual line, and a moved caret
  scrolls its visual line into view.
- **Decorations.** Selections, find hits, link underlines and insight marks split across visual
  lines. Line numbers, fold arrows and add buttons sit on a row's first visual line; the fold chip,
  inferred types, captions and notes follow its last. Boxes, bands, held marks, sticky rows, the
  change bars and the overview take the wrapped heights.
- **Heights.** `SourceWrapCache` renders each declaration's rows once, as the find cache does, and
  keeps their texts and line counts by declaration, so an edit recounts only what changed and a
  resize recounts without rendering. `SourceWrapRows` lays the counts end to end for the region's
  `VerticalOffsetLayout`, and the editor updates it in place.
- **Listing.** `SourceListingPane` shows listing lines, not rows, in a region of its own, so its
  lines stay on one line each and scroll sideways.

## Highlighting

`stdlib/code/syntax.bx` wraps the C shim in `libs/syntax`, which runs tree-sitter 0.27.1. Each session keeps
its tree; edits call `ts_tree_edit` and reparse incrementally. A reparse runs on the UI thread with
a 4 ms budget, or 40 ms when a session opens. A parse that runs over resumes on a worker thread from
a private copy of the text, while highlighting uses the edited previous tree; edits made in the
meantime are replayed onto the result, and `is-parsing` is set until it lands.

Highlighting runs the language's upstream `highlights.scm` query over a window around the visible
rows, resolves overlaps so the innermost and latest pattern wins, and caches the result per
revision. `eq?`, `match?`, and `any-of?` predicates and their negations are evaluated; other
predicates are ignored. Capture names map to `SyntaxRole`s through `syntax-role`, and
`CodeSyntaxStyle` colors each role with a GitHub-dark palette exposed as inherited style properties.

| Language | Extensions |
| --- | --- |
| C | `.c` |
| C++ | `.h .hh .hpp .hxx .h++ .cc .cpp .cxx .c++` |
| Objective-C++ | `.mm` |
| JavaScript | `.js .mjs .cjs .jsx` |
| TypeScript | `.ts .mts .cts` |
| TSX | `.tsx` |
| Python | `.py .pyi .pyw` |
| Rust | `.rs` |
| Go | `.go` |
| JSON | `.json .jsonc` |
| CSS | `.css` |
| HTML | `.html .htm` |
| Bash | `.sh .bash .zsh` |
| Markdown | `.md .markdown` |

## Tree-sitter

Tree-sitter is an external dependency, like utf8proc. `libs/build.json` names it `treesitter`,
which `brixsyntax` requires, so only programs that import `/BxSyntax/` headers, through
`stdlib/code/syntax.bx`, link it. The entry points at `build/deps/tree-sitter`, which resolves to
`.tmp/build/deps/tree-sitter` and holds `include/tree_sitter/api.h`, `lib/libtree-sitter.a`, and
one `lib/libtree-sitter-<language>.a` per grammar. Each grammar archive also defines its highlight queries as strings named
`tree_sitter_<language>_query_<file>`, such as `tree_sitter_javascript_query_highlights_jsx`. These
come from the grammar's repository and from `toolchain/deps/tree-sitter/queries/<language>/`, which holds
queries for grammars that ship none.

`toolchain/deps/tree-sitter/recipe.json` pins each repository's archive by URL and sha256, and
`toolchain/deps/tree-sitter/build.sh` builds the archives. `toolchain/runtime.py` builds the recipe into
`~/.brix/deps` and links it as `.tmp/build/deps/tree-sitter`. `libs/syntax/BxSyntaxLanguages.c`
lists each language's name, title, extensions, parser, the highlight queries it combines, and its
fold query; C++ and Objective-C++ add their queries to C's, and TypeScript and TSX add theirs to
JavaScript's. Adding a language means adding its archive to `recipe.json`, its grammar to
`build.sh`, its library to `libs/build.json`, and its entry to `BxSyntaxLanguages.c`.

Objective-C++ uses Panic's `tree-sitter-objcpp`, the only Objective-C++ grammar. It has no queries
of its own, so `queries/objcpp/highlights.scm` adapts tree-sitter-cpp's queries to its older node
shapes and adds Objective-C. It reads `[[` at the start of a nested message send, as in
`[[NSFileManager defaultManager] fileExistsAtPath:path]`, as a C++ attribute, and it misses a few
newer C++ forms such as `enum : uint32_t`. Markdown uses the block grammar from
tree-sitter-markdown. Its inline grammar needs injections, which the shim does not support, so
emphasis, code spans, and links inside paragraphs stay plain.
