# Arrays And Bytes

Yumi's public contiguous storage type is `Array'T`. The shorthand `T[]` means
`Array'T`; for locally defined type parameters, `Array'T` and `T[]` keep the
leading apostrophe because it is part of the type parameter name.

Generic arrays own contiguous typed storage. Their core behavior is allocation,
indexing, ranges, fixed-size range assignment, equality, membership, search,
prefix/suffix tests, fill, replacement, removal, growth, resize, copying,
pointer conversion, iteration, and inspection. Generic arrays can expose their
memory as bytes, but byte parsing and byte construction helpers should not make
every array API look byte-specific.
Fixed-size range assignment mutates existing storage; size-changing `replace`
returns a new array value. Infix `-=` removes matching values. Prefix `-=`
removes by position or range, as in `-= items[0]` or `-= items[1 .. < 3]`.

Byte-oriented storage behavior lives on `Array'Byte` extensions when it is
random-access and storage-oriented. The initial surface includes fixed-width
numeric reads and writes at absolute offsets. Byte encoding helpers such as
`Hex` and `Base64` are converters rather than UTF-8-flavored append helpers.

Sequential parsing and generation use cursor adapters. `ByteReadable` and
`ByteWritable` describe byte IO. `DataStream'T` provides a cursor over typed
array data for sequential reads and writes. `ChunkedTokenStream'Token` adapts a
byte-readable source into the `TokenStream` interface while preserving decoding,
partial-token buffering, putback, and location behavior.

## Endian

Byte-order-sensitive numeric reads and writes take an `Endian` argument. `Endian.big` is the
default: `read'U32()` or `write(32u32)` without one uses big-endian bytes. Use it for portable
file formats, protocols, and network data. Pass `Endian.little` only for formats specified as
little endian or for compatibility with existing little-endian data, such as probe trace records.

Native endian is not a serialized-data default. Reinterpreting memory as typed storage depends on
the host machine and is unsuitable for portable files and protocols. If a public native-endian API
is ever added, its name should say so directly and call sites should opt in explicitly.

## Implementation vocabulary

The word `buffer` is still correct for implementation storage. JavaScript and
Node `Buffer`, `ArrayBuffer`, GC allocation buffers, runtime entry points such
as `yumiAllocBuffer` and `yumiResizeBuffer`, and local variables that describe
scratch storage are internal implementation vocabulary. They are distinct from
the public Yumi `Array'T` type.
