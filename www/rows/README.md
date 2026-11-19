# Rows on the web

`rows.bx` shows Brix code on a web page as rows, the text the Brix editor shows for the same
forms. The home page and the reference both use it.

- `rows-document` reads forms into the editor's row document (`stdlib/code/structure`), and names
  the source in the error when they cannot be read.
- `rows-html` draws a document as a block: each row's indent and the kind of each span come from
  the document, so the colors are the editor's. Each function stands in a rounded box, a type in a
  tray with a box for each of its methods, header rows on a wash, and a private declaration's
  border is dashed.
- `rows-inline-lines` gives a document's rows as lines of spans, for code in a line of text.
- `html-text` and `html-attribute` escape text for HTML.

`rows.css` styles them: the spans by kind and the boxes, in the colors of the editor's light and
dark themes from `stdlib/gui/source-editor` and `stdlib/gui/theme.bx`. A block takes the theme of
its `color-scheme`, which it inherits from the page, and the page frames it. A page puts `rows.css`
before its own style. Inline rows read the `--s-` colors from the page, so a page sets them to
match its theme.
