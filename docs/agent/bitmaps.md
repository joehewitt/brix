# Bitmaps

Import `/core/bitmap` for fixed-capacity boolean collections.

`Bitmap'Integer` extends `Integer` directly. Its capacity is always `Integer.bit-width`,
and its storage is exactly the underlying integer. `Bitmap'U16()` creates sixteen clear bits;
`Bitmap'U16(12 as U16)` imports the integer value 12. There is no stored bit length or
highest-written position. Index zero is the least significant bit, and signed values preserve
all bits, including the sign bit. Export with `bitmap as Integer`.

A Bitmap is a value. `filled = bitmap.fill(true)` returns a replacement and leaves `bitmap`
unchanged. Write `bitmap = bitmap.fill(false)` to replace your variable. Indexed assignment
`bitmap[index] = truth` writes back the value returned by its setter. Assignment and `copy()`
produce independent values; neither requires release.

`BitmapBuffer(capacity)` packs bits into U64 words with a partial final word. Its capacity is
fixed at construction, and it defaults to GCBacking. Assignment shares storage: indexed writes
and `fill(truth)` mutate the shared buffer in place. `copy()` allocates independent GC storage.
`BitmapBuffer(arena, capacity)` selects ArenaBacking; `copy(arena)` copies using the supplied
Arena context. Arena handles and iterators must not outlive their backing Arena storage.

Both types expose `capacity`, boolean indexing, `find(truth)`, `find-consecutive(count, truth)`,
`count(truth)`, boolean iteration, and `indices(truth)`. `count(true)` counts set bits;
`count(false)` counts clear bits. Their sum always equals capacity. Neither type has a growing
length. Searches return the lowest matching index or absence. A zero-bit run and a run larger
than capacity return absence. Runs can cross word boundaries but never wrap around the end.
Padding is excluded from searches, counts, and iteration. Out-of-range reads and writes raise
OutOfBoundsError, which uses Yumi's panic recovery.

Inline iterators capture a value snapshot. Buffer iterators retain shared storage and observe
writes to positions they have not read yet. Iterate an explicit `copy()` for a buffer snapshot.
GC storage stays traced through a retained bitmap or iterator.

## Focused coverage

`stdlib/core/tests/runBitmap.test` and `stdlib/core/tests/runBitmapTypes.test` state the Bitmap
behavior: integer storage size and canonical widths, buffer capacity, read and write panics at
capacity, conversions, value and buffer copy semantics, fill and counts, indexing at word
boundaries, iteration order and snapshots, and search rules.
