# Rendering the reference

The language reference is authored in `docs/ref`. Its `README.md` is the introduction and sets the
order: the 27 numbered chapters, then the symbol index and the form index. Every other Markdown
file in that directory must appear in that list, except `syntax.md`, which holds notes and is left
out of the site.

## Building the reference

Run `bin/brix www/reference/build.bx -o .tmp/build/build-reference` from the repository root to generate `.tmp/site/ref`. Add `--watch` to keep the Brix builder running and rebuild after any Markdown change below the source directory. The whole reference renders into one `index.html`, with shared CSS and JavaScript alongside it. Open that file directly, or serve the directory over HTTP. The build emits a regular deferred script so interactions work in both cases. Individual chapter HTML pages are not generated.

`reference/build.bx` uses the reusable `www/markdown-site/site.bx` library for recursive document discovery, index ordering, scoped headings, and local links. `reference/forms.bx` renders code: it shows Brix code as rows with `www/rows/rows.bx`, which the home page shares, and every other fence as escaped text. Brix code is read for presentation, not executed. The page shell is a Brix paragraph template; presentation and interactions remain in `reference/style.css`, after the shared `www/rows/rows.css`, and `reference/script.js`.

## Rows and forms

The reference writes code as Brix forms: ` ```brix ` fences and inline code marked `{.brix}`. The site shows each as rows, the text the Brix editor shows for the same forms, and keeps the forms beside them. A Source button, shown while the pointer is over a Brix block, switches every block and inline span on the page between rows and forms, and the browser remembers the choice, which the home page's examples share. Without JavaScript the page shows rows.

- The rows look as they do in the Brix editor. The build reads the forms into the editor's row document (`stdlib/code/structure`) and takes each row's indent and the kind of each span from it, so the colors are the editor's. Each function stands in a rounded box, a type in a tray with a box for each of its methods, header rows on a wash, and a private declaration's border is dashed. The boxes are drawn in the editor's dark colors, from `stdlib/gui/source-editor`, in both page themes: `style.css` gives them `color-scheme: dark`.
- Inline rows take the same colors, in the editor's light or dark palette to match the page; rows of more than one line are shown as a small block in the line.
- Code marked `{.rows}` and ` ```rows ` fences are already rows and are shown as written, as are ` ```output ` and ` ```text ` fences.
- A fence's info string may name how its example ends, as in ` ```brix compile-error `.
- Forms that cannot be converted to rows fail the build, with the chapter and line.

## Reading layout and chapter navigation

The document is centered in the space to the right of the chapter rail, with at least 48px between the rail and the content on desktop and a matching outer margin on the right. Prose and code have a maximum width of 720px. Narrow screens use smaller gutters.

The fixed left rail has one tick per chapter, including the introduction. Hovering the rail opens a table of contents to its right, spanning the viewport height. Keyboard focus also opens it; tapping the tick rail opens it on touch screens. Escape, the close button, an outside click, or choosing a chapter closes the overlay. Chapter links jump instantly. Scrolling highlights the current chapter in both the rail and the contents, without changing the URL or browser history.

Continue authoring chapter links as `streams.md` or `streams.md#heading`. The build converts these to anchors in the combined page. Chapter roots use `#chapter-streams`, with `#guide` for the introduction. Headings receive a chapter prefix so chapters can reuse local names safely. Duplicate final IDs and broken internal links fail the build.

## Verification

Build the reference, serve `.tmp/site/ref` over HTTP and switch a block between rows and forms to
check the toggle.
