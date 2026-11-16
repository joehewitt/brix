# Rendering the reference

The language reference is authored in `docs/ref`. Its `README.md` is the introduction and sets the
order: the 27 numbered chapters, then the symbol index and the form index. Every other Markdown
file in that directory must appear in that list, except `syntax.md`, which holds notes and is left
out of the site.

## Building the reference

Run `bin/yumi www/src/reference/build.yumi -o .tmp/build/build-reference` from the repository root to generate `www/dist/ref`. Add `--watch` to keep the Yumi builder running and rebuild after any Markdown change below the source directory. The whole reference renders into one `index.html`, with shared CSS and JavaScript alongside it. Open that file directly, or serve the directory over HTTP. The build emits a regular deferred script so interactions work in both cases. Individual chapter HTML pages are not generated.

`reference/build.yumi` uses the reusable `www/src/markdown-site/site.yumi` library for recursive document discovery, index ordering, scoped headings, and local links. `reference/forms.yumi` shows Brix code as rows, and `reference/annotations.yumi` adds annotated Yumi examples and paired layouts. Code examples are parsed for presentation through the typed tracked-source API, not executed. The page shell is a Yumi paragraph template; presentation and interactions remain in `reference/style.css` and `reference/script.js`.

## Rows and forms

The reference writes code as Brix forms: ` ```brix ` fences and inline code marked `{.brix}`. The site shows each as rows, the text the Brix editor shows for the same forms, and keeps the forms beside them. A Forms button on each Brix block switches every block and inline span on the page between rows and forms, and the browser remembers the choice. Without JavaScript the page shows rows.

- A fence's rows are syntax-colored as Yumi. Inline rows are plain text; rows of more than one line are shown as a small block in the line.
- Code marked `{.rows}` and ` ```rows ` fences are already rows and are shown as written, as are ` ```output ` and ` ```text ` fences.
- A fence's info string may name how its example ends, as in ` ```brix compile-error `. Rows of a `compile-error` example may not be readable Yumi, so when they cannot be colored they are shown as plain text. Any other example whose rows cannot be colored is shown the same way, and the build prints a warning naming it.
- Forms that cannot be converted to rows fail the build, with the chapter and line.

## Reading layout and chapter navigation

The document is centered in the space to the right of the chapter rail, with at least 48px between the rail and the content on desktop and a matching outer margin on the right. Ordinary prose has a maximum width of 720px; paired examples can use the full 1,240px guide width. Narrow screens use smaller gutters and stack paired examples.

The fixed left rail has one tick per chapter, including the introduction. Hovering the rail opens a table of contents to its right, spanning the viewport height. Keyboard focus also opens it; tapping the tick rail opens it on touch screens. Escape, the close button, an outside click, or choosing a chapter closes the overlay. Chapter links jump instantly. Scrolling highlights the current chapter in both the rail and the contents, without changing the URL or browser history.

Continue authoring chapter links as `streams.md` or `streams.md#heading`. The build converts these to anchors in the combined page. Chapter roots use `#chapter-streams`, with `#guide` for the introduction. Headings, example IDs, and source anchors receive a chapter prefix so chapters can reuse local names safely. Duplicate final IDs and broken internal links fail the build.

## Linking prose to source

Annotations apply to ` ```yumi ` fences. The reference in `docs/ref` writes its examples as Brix and does not use them yet.

Give the code fence a stable name, wrap source ranges with markers, then link ordinary prose to them:

````markdown
```yumi #literal-example
⟦value⟧42⟦/value⟧
```

This [integer literal](#source-literal-example--value) represents a whole number.
````

The link target is `#source-EXAMPLE--RANGE`. Hovering or focusing the prose link highlights the source range. Hovering the source highlights every prose link to that range in the current page. Clicking follows the source anchor and pins its highlight; activating the link again or pressing Escape clears the pin. Modified clicks retain normal link behavior.

There is no separate label list or caption. The explanation belongs in the prose, including ordinary paragraphs, lists, and tables. Links still navigate to the marked source without JavaScript.

- Example names must be unique within a chapter and remain stable when examples move.
- Example and range names use lowercase letters, digits, and single hyphens, starting with a letter. Double hyphens separate the example from the range in links.
- Range names are local to an example. Different examples can both have a `value` range.
- Repeat a range name to connect one prose link to multiple source selections. Each marked occurrence is highlighted; the anchor points to the first occurrence.
- Multiple prose links can refer to the same selection, before or after its code block.
- Nest annotations to describe a feature within another feature. Hovering the source chooses the smallest enclosing annotation; hovering a prose link highlights all its ranges.
- Markers must be balanced and properly nested. Crossing markers and empty ranges are errors.
- Inside an annotated example, write `\⟦` for a literal opening marker character.
- Ordinary fences remain plain `yumi` fences with no markers or example name.
- The build rejects duplicate example names, malformed annotations, and unknown local `#source-...` links. That fragment prefix is reserved for annotations.
- Links from another chapter use `chapter-name.md#source-EXAMPLE--RANGE`. They also support highlighting because all chapters render into the same page. Author local range links without the generated chapter prefix.

## Side-by-side prose and source

Wrap explanatory Markdown and one annotated example in `:::example-pair`:

````markdown
:::example-pair

This [integer literal](#source-literal-example--value) represents a whole number.

```yumi #literal-example
⟦value⟧42⟦/value⟧
```

:::
````

The prose appears on the left and the source on the right, with a 24px gap. The combined pair is constrained by the guide's maximum width. Source has a fixed width of roughly 60 monospace characters plus its padding and border; prose fills all remaining space with no separate reading-width cap. Below 840px of available guide width, the pair stacks with prose first and source shrinks to fit if needed.

Long code lines soft-wrap within the source column. This is visual wrapping only: copying the code preserves its original text and line breaks, and annotations still follow the wrapped ranges. Line numbers are hidden in this layout so wrapped lines do not misalign the gutter. Prefer short examples for comfortable reading beside prose.

The prose can include paragraphs, lists, tables, and annotation links. It can be authored before or after the source fence; the rendered order is always prose then source. Each pair needs nonempty prose and exactly one named, annotated example. Pairs cannot nest. Omit the wrapper to keep the ordinary full-width presentation. Only explicitly wrapped examples use the split layout.

## Selecting syntax nodes

By default, the highlight covers exactly the text between the markers. The build also records the smallest enclosing source AST node or nodes. This keeps punctuation and partial expressions selectable even when they do not have their own AST nodes.

To expand a selection to an enclosing syntax node, add the parser kind to the opening marker: `⟦pattern:TypeList⟧`. Close it with `⟦/pattern⟧` and link to `pattern` as usual.

This selects the smallest enclosing node of that kind. A missing node or ambiguous extent fails the build. Repeated occurrences of a range must use the same node kind. Exact selections do not require knowing parser kind names.

## Mapping to rendered text

1. Strip markers while retaining their clean-source ranges.
2. Parse the cleaned example through the existing tracked Yumi source renderer. Examples are parsed for presentation, not executed.
3. Convert the renderer's Unicode scalar offsets to JavaScript's UTF-16 offsets, then associate selections with source AST ranges. All intervals are half-open: `[start, end)`.
4. Split syntax-colored runs at feature boundaries, retaining every overlapping feature ID. HTML escaping changes markup but preserves the DOM's text content.
5. Index DOM text nodes by cumulative UTF-16 length and build DOM `Range` objects for the resolved selections. Line numbers sit outside the indexed code element.
6. Merge adjacent DOM range rectangles on each line and draw padded, rounded boxes behind the source. The boxes do not change source layout or intercept pointer events; resizing or font loading updates their positions.

Divider runs retain their full-width decorative fill, including in annotated examples. The fill is generated by CSS rather than added to the source text, so copying and source offsets remain unchanged.

The build verifies that parser runs reproduce the cleaned source exactly. The browser verifies that the rendered code's text matches it too. Neither AST association nor highlighting formats or rewrites the example.

## Verification

Build into a scratch folder to check a change without touching `www/dist/ref`:

```sh
bin/yumi www/src/reference/build.yumi -o .tmp/build/build-reference -- --output .tmp/reference
```

Serve that folder over HTTP and switch a block between rows and forms to check the toggle.
