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
