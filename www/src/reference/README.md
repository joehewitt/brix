# Rendering the guide and annotating examples

The guide is authored in `docs/ref/drafts`. Its `README.md` sets chapter order; every Markdown chapter in that directory must appear in the index.

## Building the book

Run `bin/yumi www/src/reference/build.yumi -o .tmp/build/build-reference` from the repository root to generate `www/dist/ref`. Add `--watch` to keep the Yumi builder running and rebuild after any Markdown change below the source directory. The whole guide renders into one `index.html`, with shared CSS and JavaScript alongside it. Open that file directly, or serve the directory over HTTP. The build emits a regular deferred script so interactions work in both cases. Individual chapter HTML pages are no longer generated.

`reference/build.yumi` uses the reusable `www/src/markdown-site/site.yumi` library for recursive document discovery, index ordering, scoped headings, and local links. `reference/annotations.yumi` adds the guide's annotated Yumi examples and paired layouts. Code examples are parsed for presentation through the typed tracked-source API, not executed. The page shell is a Yumi paragraph template; presentation and interactions remain in `reference/style.css` and `reference/script.js`.

## Reading layout and chapter navigation

The document is centered in the space to the right of the chapter rail, with at least 48px between the rail and the content on desktop and a matching outer margin on the right. Ordinary prose has a maximum width of 720px; paired examples can use the full 1,240px guide width. Narrow screens use smaller gutters and stack paired examples.

The fixed left rail has one tick per chapter, including the guide introduction. Hovering the rail opens a table of contents to its right, spanning the viewport height. Keyboard focus also opens it; tapping the tick rail opens it on touch screens. Escape, the close button, an outside click, or choosing a chapter closes the overlay. Chapter links jump instantly. Scrolling highlights the current chapter in both the rail and the contents, without changing the URL or browser history.

Continue authoring chapter links as `streams.md` or `streams.md#heading`. The build converts these to anchors in the combined page. Chapter roots use `#chapter-streams`, with `#guide` for the introduction. Headings, example IDs, and source anchors receive a chapter prefix so chapters can reuse local names safely. Duplicate final IDs and broken internal links fail the build.

## Linking prose to source

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

[Structural pattern matching](../../docs/ref/drafts/structural-pattern-matching.md) contains worked examples, including nested annotations and multiline selections.

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

The prose can include paragraphs, lists, tables, and annotation links. It can be authored before or after the source fence; the rendered order is always prose then source. Each pair needs nonempty prose and exactly one named, annotated example. Pairs cannot nest. Omit the wrapper to keep the ordinary full-width presentation.

Paired examples appear throughout the guide. Structural pattern matching demonstrates nested ranges; String templates links document text to its supporting calculations; the design essays pair syntax with concise explanations. Only explicitly wrapped examples use the split layout.

## Chapter presentation

Use the Structural pattern matching chapter as the presentation model:

- Open with a short statement of the capability and an at-a-glance table where it helps readers choose among forms.
- Let examples carry the explanation. Link meaningful syntax or expressions from ordinary prose and bullet lists rather than introducing a separate feature legend.
- Use paired layouts for compact walkthroughs. Keep longer programs and reference-like tables in the normal flow when that reads better.
- Give each callout a short label and a concrete explanation of behavior. Include the example's result when it clarifies the point.
- Use tables for alternatives, contracts, and costs; lists for independent considerations. Retain prose where reasoning needs a connected explanation.
- In the design essays, distinguish the author's rationale from the tradeoffs using short, clearly labeled lists.
- Cross-link the chapter that owns a topic instead of repeating its full explanation.

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

The configured selector is `file:www/tests/source_annotations.py`. It covers how highlight boxes join the rectangles of syntax fragments on each line. Run configured verification through Ometer's MCP tools.
