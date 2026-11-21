# GUI Display Scene

`DisplayScene` is the renderer-owned retained representation of a painted `Box` tree. Every
`Window` frame goes through it: the Metal path encodes the committed scene with `frame-metal`, and
the fallback path replays it into a retained CoreGraphics surface with `frame`.

## Frame phases

Scene use has three non-interleaved phases:

1. `build` or `update` mutates the working view. Paint invalidation selects chunks to re-record,
   invalid-below prunes descent, and the Box child-list generation detects membership changes.
2. `commit` publishes that complete view.
3. `frame` reads the committed view and replays damage. It cannot invoke maintenance or any Box
   paint hook.

`maintain` is the Window-facing convenience operation for the first two phases.

## Stable packed item layout

Every `DisplayChunk.items` list is one contiguous, z-ordered store of `DisplayItem` values. The
layout is the native bulk-encode seam and must not be replaced with per-item FFI calls:

| Offset | Field | Type | Meaning |
| ---: | --- | --- | --- |
| 0 | `kind` | `U8` | quad, border, glyph, vector, image, opaque, shadow, styled, canvas image, backdrop, canvas pattern, sprite, backdrop shader, or quad batch |
| 1 | `flags` | `U8` | kind-specific presence and representation flags |
| 2 | `aux` | `U16` | kind-specific small value, currently text alignment |
| 4 | `x` | `F32` | logical geometry |
| 8 | `y` | `F32` | logical geometry |
| 12 | `width` | `F32` | logical geometry |
| 16 | `height` | `F32` | logical geometry |
| 20 | `value` | `F32` | kind-specific scalar, currently border width |
| 24 | `data0` | `U32` | packed RGBA8, handle index, or side-table index |
| 28 | `data1` | `U32` | handle generation or secondary payload |
| 32 | `data2` | `U32` | packed RGBA8, palette offset, or tint |
| 36 | `data3` | `U32` | palette count or clip side-table index |

The record is exactly 40 bytes, aligned to four bytes, with no padding or managed reference.
RGBA8 words use little significance order: red in bits 0–7, green 8–15, blue 16–23, alpha
24–31.

Vector geometry and images use `(index, generation)` resource handles. The CoreGraphics-era glyph
item may index the chunk's retained `TextLayout` side table. `DisplayGlyphRegistrar` is the
record-time extraction seam: when a registrar returns a handle, the same item stores its index and
generation instead, without re-recording any surrounding display data.

## Quad batches

`Canvas.quads` draws a list of `Quad` records, each a rounded rectangle with a packed RGBA8 color:
five `F32` fields and a `U32`, 24 bytes with no padding. The recorder copies the list into the
chunk side table's `quads` and adds one quads item whose geometry is the batch's bounds and whose
`data1` and `data2` give the start and count. The Metal renderer turns that range into a single
packet op and fills its instances in one loop, with the clip, scissor, and blend resolved once.
CoreGraphics replay draws each record as an outline. A `DisplayFrame` carries the records in its
`quads` table, and `draw-frame` re-records each batch under the mapped state.

Under a fill paint, a shape fill, or a transform that is not a translation, the recorder records
each quad as its own item instead, so both forms render identically.

## Chunks, groups, and opaque paint

A painting box has up to two chunks: pre contains background plus self; post contains border plus
scroll indicators. Empty recordings append neither a chunk nor an item. Groups preserve ordered
membership and carry clip/scroll properties. Their ink union is published onto the Box
`LayoutIndex` sidecar, alongside its layout bounds.

The recorder captures both packed supported items and raw CoreGraphics commands. If a hook uses a
path, oval, transform, or another non-item operation, its chunk is marked opaque and replay
interprets the captured commands. Opaque replay never calls `Box.paint`, `paint-self`, or any other
managed paint callback.

## Item culling

Chunk bounds come from per-state ink unions: a recording adds each item's ink to its state's
`local-ink`, or to the side table's `root-ink` for state 0, and `refresh-bounds` maps one union per
state through the state's matrix. A transform-only change refreshes bounds without touching items.

Before encoding, `expand-damage` grows damage that touches a backdrop to cover what the backdrop
samples, and damage that touches a filtered layer to the layer's outer bounds, until neither adds
more. `frame-metal` then clamps damage to the surface. A layer with filters encodes its members
over the group's outer bounds, so filter sampling still reaches past the damage. A chunk of `display-cull-minimum` or more items without
mask or blur layers builds a `DisplayCullIndex` on its second encode after a recording, when the
damage does not cover its bounds. The index keeps one grid for items under the content transform,
in content space, and one for the rest, so pan and zoom query it without rebuilding. Each frame
selects the items whose ink touches the damage and hands the renderer their indices in paint order;
when the grids estimate that half or more of the items are in view, the chunk draws everything.
Drawability is checked from per-state requirements computed with the index, so it never depends on
which items are visible.

Code that edits a recorded chunk's items in place must set its `cull-index` to null.

## Frame times

After each committed frame, `Window.last-frame-times` holds a `FrameTimes` with the main-thread
time the frame spent in each phase:

| Part | Covers |
| --- | --- |
| `input` | event handling since the previous frame, plus resampled scroll dispatch |
| `tick` | the window clock, the animations it steps, and the `(on tick frames)` loops they run |
| `update` | the markup checkpoint, focus, and settling for paint |
| `layout` | `layout`, or popup placement alone |
| `record` | `maintain` |
| `encode` | `frame-metal` or `frame`; `walk` and `native` are the renderer's packet walk and its native expand and encode within it |
| `commit` | handing the frame to the window |

`total` runs from the start of the frame to its end and adds the input handled before it, so it is
at least the sum of the parts; popups, geometry sync, and the opening frame's layout show only in
it. `frame` is the Metal frame id, or -1 when the frame did not go through Metal.

`FrameTicks` sends each tick with `AsyncOnDemandChannel.deliver`, so a loop on `frames` runs
inside the tick part, before update, and what it changes draws in the same frame. A loop that
waits stalls the frame.

`metal-frame-input-latency-nanos` gives the time from the first pointer or touch event that left
work for a frame to that frame's presentation; key and text events carry no timestamp, so they do
not count. It returns -1 when no such input came or the frame has not presented yet, and may return
-1 for a frame more than 16 frames old. Presentation trails the commit by several frames, so ask
about a frame some frames back.
