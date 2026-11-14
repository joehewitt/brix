# Array row and field views

`Array` and `ImmutableArray` expose `.rows` and `.fields` for fixed-layout struct and tuple elements.
Ordinary indexing and iteration still return element values. Access through `.rows` explicitly
selects live references to the backing storage.

| Surface | View type | Read behavior |
| --- | --- | --- |
| `items.rows` | `ArrayRows'Item'Access` | Indexing and iteration yield live row references. |
| `items.rows[index]` | `ArrayRow'Item'Access` | Named or positional fields read the current row. |
| `items.rows[range]` | `ArrayRowRange'Item'Access` | A live contiguous range of row references. |
| `items.fields.name` | `ArrayField'Value'Access` | A live strided projection of a stored field. |
| `items.fields[index]` | `ArrayField'Value'Access` | A projection selected by a constant field position. |

`Access` is `ArrayWriteAccess` for mutable arrays and `ArrayReadAccess` for immutable arrays.
Omitting the access parameter selects read-only access. A parameter with read-only access accepts
writable views of the same element type. Read-only access cannot be cast back to writable access.
Stored, passed, returned, and sliced views keep their access qualification.

Writing a mutable row field or field-view element updates the original storage. Row-range and
field-range assignment copy matching ranges; overlapping assignments use a detached snapshot.
Range bounds and unit increments are checked before access.

Casting a row reference to its element type creates a detached value. Ranges and fields provide
`to-array()` and `to-immutable-array()` to copy their current elements into contiguous storage.
Empty source arrays produce empty views and empty materializations.

Each view retains a backing token instead of a pointer to the source handle. GC-backed storage
remains reachable when a view outlives its source variable. The token records arena identity and
generation for arena-backed storage. Replacing storage, relocating or shrinking it during resize,
and resetting, rewinding, or releasing its arena invalidate affected views. Same-size element
writes remain visible through live views. Invalidated views raise `Stale array view` before
access; iteration validates before entering its body.

The compiler derives aggregate projection, indexing, slicing, and access checks. The shared
iteration and materialization methods are implemented in `stdlib/core/array.yumi`.

Stable row loops and zero-based indexed loops over a row view's length share the array-of-structs
cursor lowering. They evaluate the receiver and validate its token before loop effects, keep
storage and length outside the loop, and access scalar fields directly using the loop bounds.
Mutable and immutable views share read lowering; only writable views permit stores.

Escaping rows retain their durable view representation. Closure captures, unknown calls,
structural mutations, custom getters, and unsupported field operations use the general validated
path. Ordinary array iteration continues to return values.
