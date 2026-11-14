# Stream array constructors

Import `/core/array-builder` to enable the stream constructor extensions.
Basic array use retains its existing optional-storage import boundary.

`Array(items, capacity = 0)` and `ImmutableArray(items, capacity = 0)` consume a
stream once, preserve its order, and infer its element type. The provider forms
accept `Array(arena, items, capacity)` and `ImmutableArray(arena, items, capacity)`.
Bullet streams can use these constructors as their final consumer.

Capacity is a reservation hint. It does not change the number of returned
items. The constructors use ArrayBuilder's ordinary growth and non-compacting
finish, so an insufficient hint grows and spare capacity remains allocated.

The mutable constructor defers builder creation and reservation until the first
item. Its empty result uses a null pointer and zero length. The immutable
constructor delegates consumption to the mutable constructor and returns an
immutable view. Both overloads of each constructor are inline.

Empty input ignores even an overflowing capacity hint. It allocates no collection
storage in either backing, including no arena builder identity header. Producer
allocations are outside this guarantee. Ordinary arena builders retain their
existing identity header and checkpoint behavior. Arena-backed references retain
the backing's existing untraced ownership requirements.

Empty iteration, slicing, copying, conversion, bounds checks, and later mutable
resize are covered by `stdlib/core/tests/runArrayStreams.test`. The native zero-length
array copy helpers return before calling memmove or the bulk write barrier, so
storage-free arrays do not pass null pointers to those operations.
