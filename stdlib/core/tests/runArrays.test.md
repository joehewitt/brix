# Arrays

```yumi
. buffer = Float[1]
-- buffer[0]
buffer[0] = 1.5
-- buffer[0]
```

```output
0
1.5
```

```yumi
. buffer = Int[1]
-- buffer[0]
buffer[0] = 1
-- buffer[0]
buffer[0] = 2.5 as Int
-- buffer[0]
```

```output
0
1
2
```

```yumi
do new
    . buffer = Float[1]
    -- buffer[1]
catch failure
    -- failure
```

```output
Index out of bounds (1 / 1)
```

```yumi
do new
    . buffer = Float[1]
    -- buffer[1] = 1
catch failure
    -- failure
```

```output
Set out of bounds (1 / 1)
```

```yumi
. buffer = Array'String(2)
buffer[0] = "a"
buffer[1] = "b"

try-read(buffer, -1)
try-write(buffer, -5)
try-slice(buffer, -1)
-- ", ".join(buffer)

━━━━━━━━━━━━━━━━━━━━
_ do try-read
    - buffer Array'String
    - index Index

. attempt = do new
    -- buffer[index]
catch failure
    -- "read: $failure"
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
_ do try-write
    - buffer Array'String
    - index Index

. attempt = do new
    buffer[index] = "z"
catch failure
    -- "write: $failure"
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
_ do try-slice
    - buffer Array'String
    - start Index

. attempt = do new
    -- ", ".join(buffer[start .. < 1])
catch failure
    -- "slice: $failure"
attempt.wait()
```

```output
read: Index out of bounds (-1 / 2)
write: Set out of bounds (-5 / 2)
slice: Range out of bounds (0 / 2)
a, b
```

## Ranges

```yumi
. buffer = Array'Float(5)
buffer[2 .. 4] = 3.5
-- ", ".join(buffer)

-- ", ".join(buffer[2 .. 4])
```

```output
0, 0, 3.5, 3.5, 3.5
3.5, 3.5, 3.5
```

```yumi
do new
    . buffer = Array'Float(5)
    -- buffer[0 .. Position.end]
catch failure
    -- failure
```

```output
Range out of bounds (5 / 5)
```

```yumi
do new
    . buffer = Array'Float(5)
    buffer[0 .. Position.end] = 1
catch failure
    -- failure
```

```output
Set out of bounds (5 / 5)
```

### Range assignment checks source length before copying.

```yumi
. buffer = Byte[5]
buffer[0] = 10u8
buffer[1] = 11u8
. source = Byte[1]
source[0] = 99u8

copy-too-much(buffer, source)
-- ", ".join(buffer)

━━━━━━━━━━━━━━━━━━━━
+ do copy-too-much
    - buffer Byte[]
    - source Byte[]

. attempt = do new
    buffer[0 .. < 5] = source
catch failure
    -- failure
attempt.wait()
```

```output
Source array too short (1 / 5)
10, 11, 0, 0, 0
```

```yumi
. values = Array'Int([1, 2, 3, 2])
-- values == Array'Int([1, 2, 3, 2])
-- values == Array'Int([1, 2, 3])
-- values == [1, 2, 3, 2]
```

```output
true
false
true
```

```yumi
. values = Array'Int([1, 2, 3, 2])
-- values has 2
-- values has not 9
-- values has 3
```

```output
true
true
true
```

```yumi
. values = Array'Int([1, 2, 3, 2])
-- values.find(2)
-- values.find(9) == null
```

```output
1
true
```

```yumi
. values = Array'Int([1, 2, 3, 2])
-- values.find(Array'Int([3, 2]))
-- values.find([2, 3])
```

```output
2
1
```

```yumi
. values = Array'Int([1, 2, 3, 2])
-- values.starts-with([1, 2])
-- values.starts-with([2, 3])
```

```output
true
false
```

```yumi
. values = Array'Int([1, 2, 3, 2])
-- values.ends-with([3, 2])
-- values.ends-with([1, 2])
```

```output
true
false
```

```yumi
. bytes = Array'Byte([0u8, 1u8, 2u8, 253u8, 254u8, 255u8])
-- bytes as Hex
```

```output
000102fdfeff
```

```yumi
-- ", ".join(Hex("000102fdfeff") as Byte[])
-- ", ".join(Hex("0x000102FDFEFF") as Byte[])
```

```output
0, 1, 2, 253, 254, 255
0, 1, 2, 253, 254, 255
```

```yumi
-- Array'Byte("f") as Base64
-- Array'Byte("fo") as Base64
-- Array'Byte("foo") as Base64
```

```output
Zg==
Zm8=
Zm9v
```

```yumi
. decoded = Base64("Zm9v") as Byte[]
-- decoded.slice-string()
```

```output
foo
```

```yumi
. decoded = Base64("") as Byte[]
-- decoded.length
```

```output
0
```

### Array storage copies Arena handles and allocation pointers without lifetime effects.

```yumi
/core/arena

. first = Arena()
. second = Arena()
. first-pointer = first.allocate'U32(1)
first-pointer[0] = 10u32
. second-pointer = second.allocate'U32(1)
second-pointer[0] = 20u32

. arenas = Array'Arena([first])
arenas[0] = second
. first-next = first.allocate'U32(1)
first-next[0] = 11u32
-- first-next[0]
arenas = Array'Arena()
. second-next = second.allocate'U32(1)
second-next[0] = 21u32
-- second-next[0]

. pointers = Array'(CPointer'U32)([first-pointer])
pointers = pointers.replace(0 .. < 1, Array'(CPointer'U32)([second-pointer]))
-- first-pointer[0]
-- pointers[0][0]

first.release()
second.release()
```

```output
11
21
10
20
```

```yumi
/core/arena

-- Array'U32.size, Array'U32'ArenaBacking.size
```

```output
(16, 16)
```

```yumi
/core/arena

. arena = Arena()
do finally
    arena.release()

. values = Array'U32(arena, 4)
values <- [10u32, 20u32, 30u32, 40u32]
. slice = values[1 .. < 3]
slice[0] = 21u32
-- type-of values
-- type-of slice
-- values[0], values[1], values[2], values[3]
```

```output
[TypeInfo Array'U32'ArenaBacking]
[TypeInfo Array'U32'ArenaBacking]
(10, 21, 30, 40)
```

```yumi
/core/arena

. arena = Arena()
do finally
    arena.release()

. values = Array'U32(arena, [10u32, 21u32])
. grown = values.resize(3, arena)
grown[2] = 50u32
-- grown[0], grown[1], grown[2]
```

```output
(10, 21, 50)
```

```yumi
/core/arena

. arena = Arena()
do finally
    arena.release()

. values = Array'U32(arena, [10u32, 21u32, 50u32])
. copy = values.copy(arena)
copy[0] = 99u32
-- values[0], copy[0], copy[1], copy[2]
```

```output
(10, 99, 21, 50)
```

```yumi
/core/arena

. arena = Arena()
do finally
    arena.release()

. source = Array'U32(arena, [99u32, 21u32, 30u32, 50u32])
. destination = Array'U32(arena, 3)
destination <- source[0 .. < 3]
-- destination[0], destination[1], destination[2]
```

```output
(99, 21, 30)
```

```yumi
/core/arena

. arena = Arena()
do finally
    arena.release()
. values = Array'U32(arena, [5u32, 6u32])
-- values[0], values[1]
```

```output
(5, 6)
```

```yumi
/core/arena

. arena = Arena()
do finally
    arena.release()
. values = Array'U32(arena, 3) • with :: index => (index as U32) + 60u32
-- values[0], values[1], values[2]
```

```output
(60, 61, 62)
```

```yumi
/core/arena

. arena = Arena()
do finally
    arena.release()
. values = Array'U32(arena, [60u32, 61u32, 62u32])
. mapped = values.map(arena) • using :: item, index => item + (index as U32)
-- mapped[0], mapped[1], mapped[2]
```

```output
(60, 62, 64)
```

```yumi
/core/arena

. arena = Arena()
do finally
    arena.release()
. values = Array'U32(arena, [60u32, 62u32, 64u32])
. replaced = values.replace(1 .. < 2, arena, 77u32)
-- replaced[0], replaced[1], replaced[2]
```

```output
(60, 77, 64)
```

```yumi
/core/arena

. arena = Arena()
do finally
    arena.release()
. octets = Array'U8(arena, [8u8])
. words = Array'U16(arena, [16u16])
-- octets[0], words[0]
```

```output
(8, 16)
```

```yumi
/core/arena

. arena = Arena()
do finally
    arena.release()
. values = Array'U32(arena, 5)
. bytes = values as Array'Byte'ArenaBacking
-- bytes.size
```

```output
20
```

```yumi
. octets = Array'U8([1u8])
. words = Array'U16([2u16])
-- octets[0], words[0]
```

```output
(1, 2)
```

```yumi
/BxGC/BxGC.h

. values = Array'BackingNode(3)
for index in 0 .. < values.length
    values[index] = BackingNode("node-$index")
values[1] = BackingNode("replacement")
values = values.resize(5)
values[4] = BackingNode("grown")
C:brixGCCollect()
-- values[0].value, values[1].value, values[4].value

━━━━━━━━━━━━━━━━━━━━
+ BackingNode

+ value String

────────────────────
+ This
    - text String

value = text
```

```output
(node-0, replacement, grown)
```

```yumi
/BxGC/BxGC.h

. values = Array'BackingNode([BackingNode("node-0"), BackingNode("replacement")])
values = values.resize(5)
values[4] = BackingNode("grown")
. copy = values.copy()
values[0] = BackingNode("original")
C:brixGCCollect()
-- values[0].value, copy[0].value, copy[1].value, copy[4].value

━━━━━━━━━━━━━━━━━━━━
+ BackingNode

+ value String

────────────────────
+ This
    - text String

value = text
```

```output
(original, node-0, replacement, grown)
```

```yumi
/core/arena

-- arena-copy-result()

━━━━━━━━━━━━━━━━━━━━
+ do arena-copy-result U32 without gc

. local-arena = Arena()
do finally
    local-arena.release()
. values = Array'U32(local-arena, 2)
values[0] = 1u32
values[1] = 2u32
. result = values.copy(local-arena)
= result[1]
```

```output
2
```

```yumi
/core/arena

-- arena-resize-result()

━━━━━━━━━━━━━━━━━━━━
+ do arena-resize-result U32 without gc

. local-arena = Arena()
do finally
    local-arena.release()
. values = Array'U32(local-arena, 2)
values[0] = 1u32
values[1] = 2u32
. result = values.resize(3, local-arena)
result[2] = 3u32
= result[2]
```

```output
3
```

### Byte encoding converters report malformed input.

```yumi
do new
    . bytes = Hex("abc") as Byte[]
catch failure
    -- failure
```

```output
Hex byte arrays require an even digit count
```

```yumi
do new
    . bytes = Base64("abc") as Byte[]
catch failure
    -- failure
```

```output
Invalid Base64 length
```

```yumi
. values = Array'Int([1, 2, 3, 4, 5])
values[1 .. < 4] = [8, 9, 10]
-- ", ".join(values)
```

```output
1, 8, 9, 10, 5
```

```yumi
. values = Array'Int([1, 2, 3])
values.fill(7)
-- ", ".join(values)
```

```output
7, 7, 7
```

```yumi
. values = Array'Int([7, 7, 7, 7, 7])
values = values.replace(1 .. < 4, Array'Int([2, 3]))
-- ", ".join(values)
```

```output
7, 2, 3, 7
```

```yumi
. values = Array'Int([7, 2, 3, 7])
values = values.replace(2 .. < 2, [4, 5])
-- ", ".join(values)
```

```output
7, 2, 4, 5, 3, 7
```

```yumi
. values = Array'Int([7, 2, 4, 5, 3, 7])
values = values.replace(0 .. < 2, 9)
-- ", ".join(values)
```

```output
9, 4, 5, 3, 7
```

```yumi
. values = Array'Int([9, 4, 5, 3, 7])
values -= 5
-- ", ".join(values)
```

```output
9, 4, 3, 7
```

```yumi
. values = Array'Int([9, 4, 3, 7])
-= values[1]
-- ", ".join(values)
```

```output
9, 3, 7
```

```yumi
. values = Array'Int([9, 3, 7])
-= values[1 .. < 2]
-- ", ".join(values)
```

```output
9, 7
```

```yumi
. values = Array'Int([9, 7])
values -= 42
-- ", ".join(values)
```

```output
9, 7
```

### Byte slice-string checks the range before reading.

```yumi
do new
    . buffer = Array'Byte("a")
    -- buffer.slice-string(0 .. < 5)
catch failure
    -- failure
```

```output
Range out of bounds (4 / 1)
```

### Char slice-string checks the range before reading.

```yumi
do new
    . buffer = Array'Char("a")
    -- buffer.slice-string(0 .. < 5)
catch failure
    -- failure
```

```output
Range out of bounds (4 / 1)
```

### Oversized buffer allocation panics instead of returning null data with a huge size.

```yumi
do new
    . buffer = Array'I64((Size.max / I64.size) + 1)
    -- buffer.size
    -- buffer.data.address
catch failure
    -- failure
```

```output
Array size overflow
```

### Oversized buffer resize panics before byte-size arithmetic overflows.

```yumi
do new
    . buffer = I64[1]
    buffer = buffer.resize((Size.max / I64.size) + 1)
    -- buffer.size
catch failure
    -- failure
```

```output
Array size overflow
```

### Oversized buffer growth panics before size arithmetic overflows.

```yumi
do new
    . buffer = Byte[1]
    buffer = buffer.grow(Size.max)
    -- buffer.size
catch failure
    -- failure
```

```output
Array size overflow
```

### Byte buffers constructed from strings use UTF-8 byte length.

```yumi
. buffer = Array'Byte("é")
-- buffer.size
-- buffer.slice-string()
```

```output
2
é
```

### Byte-buffer string range assignment writes UTF-8 bytes without recursion.

```yumi
. buffer = Byte[2]
buffer[0 .. < 2] = "é"
-- buffer.size
-- buffer.slice-string()
```

```output
2
é
```

### Byte-buffer string range assignment rejects length mismatches.

```yumi
do new
    . buffer = Byte[1]
    buffer[0 .. < 1] = "é"
catch failure
    -- failure
```

```output
String range length mismatch (2 / 1)
```

### Byte slice-string rejects invalid UTF-8.

```yumi
do new
    . buffer = Byte[1]
    buffer[0] = 255u8
    -- buffer.slice-string()
catch failure
    -- failure
```

```output
Unable to read UTF8 content
```

## Relative positions

```yumi
. buffer = Float[3]
buffer[Position.first] = 1.5
buffer[Position.last] = 2.5
-- buffer[Position.first]
-- buffer[Position.last]
-- ", ".join(buffer)
```

```output
1.5
2.5
1.5, 0, 2.5
```

```yumi
. buffer = Array'Float(3)
buffer[Position.middle] = 1.5
-- buffer[Position.middle]
-- ", ".join(buffer)

buffer = Array'Float(4)
buffer[Position.middle] = 1.5
-- buffer[Position.middle]
-- ", ".join(buffer)
```

```output
1.5
0, 1.5, 0
1.5
0, 0, 1.5, 0
```

```yumi
. buffer = Array'Float(3)
buffer[0] = 0.5
buffer[1] = 1.5
buffer[2] = 2.5

-- ", ".join(buffer[Position.first .. < 2])
```

```output
0.5, 1.5
```

```yumi
. buffer = Array'Float(5)
buffer[0] = 0.5
buffer[1] = 1.5
buffer[2] = 2.5
buffer[3] = 3.5
buffer[4] = 4.5

-- ", ".join(buffer[0 .. < 5])
-- ", ".join(buffer[Position.first .. < Position.end])
-- ", ".join(buffer[Position.first .. Position.last])

-- ", ".join(buffer[Position.last .. < Position.first])
-- ", ".join(buffer[Position.last .. Position.first])

-- ", ".join(buffer[0 .. < 2])
```

```output
0.5, 1.5, 2.5, 3.5, 4.5
0.5, 1.5, 2.5, 3.5, 4.5
0.5, 1.5, 2.5, 3.5, 4.5
1.5, 2.5, 3.5, 4.5
0.5, 1.5, 2.5, 3.5, 4.5
0.5, 1.5
```

### Storing objects in buffers

```yumi
. buffer = Thing[10]
buffer[0] = Thing(1)
buffer[1] = Thing(2)
-- buffer[0].a
-- buffer[1].a

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ a = 0

────────────────────
+ This
    - v

a = v
```

```output
1
2
```

### Indexed Array struct field reads preserve scalar, aggregate, and managed-field values.

```yumi
. rows = Array'Row([Row(1u8, 2u16, 3u32, 4u64, Point(5, 6), "seven")])

-- rows[0].tiny
-- rows[0].small
-- rows[0].word
-- rows[0].wide
-- rows[0].position.x
-- rows[0].text

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32

━━━━━━━━━━━━━━━━━━━━
+ Row is Struct

+ tiny U8
+ small U16
+ word U32
+ wide U64
+ position Point
+ text String
```

```output
1
2
3
4
5
seven
```

### The indexed Array scalar-field fast path evaluates its receiver and index once.

```yumi
. probe = Probe(Array'Row([Row(42)]))

-- probe.rows()[probe.next()].value
-- probe.rows-calls
-- probe.next-calls

━━━━━━━━━━━━━━━━━━━━
+ Row is Struct

+ value I32

━━━━━━━━━━━━━━━━━━━━
+ Probe

+ values Array'Row
+ rows-calls = 0
+ next-calls = 0

────────────────────
+ This
    - values Array'Row

────────────────────
+ do rows Array'Row

rows-calls += 1
= values

────────────────────
+ do next Index

next-calls += 1
= 0
```

```output
42
1
1
```

### An out-of-bounds indexed Array scalar-field read preserves the accessor panic.

```yumi
do new
    . rows = Array'Row([Row(42)])
    -- rows[1].value
catch failure
    -- failure

━━━━━━━━━━━━━━━━━━━━
+ Row is Struct

+ value I32
```

```output
Index out of bounds (1 / 1)
```

### Canonical scalar Array reads and writes preserve every eligible numeric representation.

```yumi
. bools = Array'Bool([false, true])
. i8s = Array'I8([-128i8, 127i8])
. u8s = Array'U8([0u8, 255u8])
. i16s = Array'I16([-32768i16, 32767i16])
. u16s = Array'U16([0u16, 65535u16])
. i32s = Array'I32([-2147483648i32, 2147483647i32])
. u32s = Array'U32([0u32, 4294967295u32])
. i64s = Array'I64([-9223372036854775808i64, 9223372036854775807i64])
. u64s = Array'U64([0u64, 18446744073709551615u64])
. f32s = Array'F32([1.25f32, 2.5f32])
. f64s = Array'F64([3.25, 4.5])

-- bools[1]
-- i8s[0]
-- u8s[1]
-- i16s[0]
-- u16s[1]
-- i32s[0]
-- u32s[1]
-- i64s[0]
-- u64s[1]
-- f32s[0]
-- f64s[1]

bools[0] = true
i8s[0] = 126i8
u8s[0] = 254u8
i16s[0] = 32766i16
u16s[0] = 65534u16
i32s[0] = 2147483646i32
u32s[0] = 4294967294u32
i64s[0] = 9223372036854775806i64
u64s[0] = 18446744073709551614u64
f32s[0] = 6.25f32
f64s[0] = 7.5

-- bools[0]
-- i8s[0]
-- u8s[0]
-- i16s[0]
-- u16s[0]
-- i32s[0]
-- u32s[0]
-- i64s[0]
-- u64s[0]
-- f32s[0]
-- f64s[0]
```

```output
true
-128
255
-32768
65535
-2147483648
4294967295
-9223372036854775808
18446744073709551615
1.25
4.5
true
126
254
32766
65534
2147483646
4294967294
9223372036854775806
18446744073709551614
6.25
7.5
```

### Canonical scalar Array access evaluates receiver, index, and assigned value once in source order.

```yumi
. probe = ScalarArrayProbe()
probe.array()[probe.index()] = probe.value()
-- probe.order
-- probe.values[0]

probe.order = ""
-- probe.array()[probe.index()]
-- probe.order

━━━━━━━━━━━━━━━━━━━━
+ ScalarArrayProbe

+ values = Array'U8([1u8])
+ order = ""

────────────────────
+ do array Array'U8

order += "receiver>"
= values

────────────────────
+ do index Index

order += "index>"
= 0

────────────────────
+ do value U8

order += "value>"
= 9u8
```

```output
value>receiver>index>
9
9
receiver>index>
```

### Negative canonical scalar Array indices preserve the accessor panic.

```yumi
do new
    . values = Array'U8([1u8])
    . index = -1i8
    -- values[index]
catch failure
    -- failure
```

```output
Index out of bounds (-1 / 1)
```

```yumi
. optional Array'U8? = Array'U8([3u8])
optional?[0] = 4u8
-- optional?[0]
```

```output
4
```

```yumi
. custom = ScalarArrayAccessor(Array'U8([5u8]))
-- custom[0]
custom[0] = 6u8
-- custom[0]
-- custom.reads
-- custom.writes

━━━━━━━━━━━━━━━━━━━━
+ ScalarArrayAccessor

+ values Array'U8
+ reads = 0
+ writes = 0

────────────────────
+ This
    - values

────────────────────
+ do this[index Index] U8

reads += 1
= values.data[index]

────────────────────
+ do this[index Index] = value U8

writes += 1
values.data[index] = value
```

```output
5
6
2
1
```

### Array index extensions retain their accessor dispatch.

```yumi crash
. values = Array'U8([5u8])
-- values[0]

━━━━━━━━━━━━━━━━━━━━
+ extend Array'U8

────────────────────
+ this[index Index] U8

this.data[index] + 10u8
```

```output
Exception: Extension member conflicts with existing member
<no file:3>
    -- values[0]
       ^^^^^^^^^
```

### Resizing buffers

```yumi
. buffer = Float[2]
buffer[0] = 1.5
-- ", ".join(buffer)

buffer = buffer.resize(3)
-- ", ".join(buffer)

buffer[2] = 3.5
-- ", ".join(buffer)
```

```output
1.5, 0
1.5, 0, 0
1.5, 0, 3.5
```

### Pointer math on buffers

```yumi
. buffer = Float[2]
buffer[0] = 1.5
buffer[1] = 2.5
buffer = buffer + 1
-- buffer[0]
```

```output
2.5
```

```yumi
. buffer = Float[2]
buffer[0] = 1.5
buffer[1] = 2.5
buffer += 1
-- buffer[0]
```

```output
2.5
```

```yumi
. buffer = Float[2]
buffer += 1
-- buffer.size
```

```output
1
```

```yumi
do new
    . buffer = Float[2]
    buffer += 3
catch failure
    -- failure
```

```output
Out of bounds 3 / 2
```

```yumi
do new
    . buffer = Float[2]
    buffer += 2
    -- buffer[0]
catch failure
    -- failure
```

```output
Index out of bounds (0 / 0)
```

## Type conversions

### Converting a non-byte buffer to bytes

```yumi
. buffer = I32[2]
buffer[0] = 1
buffer[1] = 2

. bytes = buffer as Array'Byte
-- bytes.size
-- bytes.byte-size
```

```output
8
8
```

## ChunkedTokenStream

```yumi
/core/tests/support/byte-source
/core/tokenstream

. buffer = Byte[9]
buffer[0] = 64u8

buffer[1] = 0u8
buffer[2] = 0u8
buffer[3] = 3u8
buffer[4] = 32u8

buffer[5] = 1u8
buffer[6] = 2u8
buffer[7] = 3u8
buffer[8] = 32u8

. buffer-stream = TestByteSource(buffer)
. stream = ChunkedTokenStream'Byte(buffer-stream)

-- stream.require'Byte()
-- stream.require'Byte()
-- stream.require'Byte()
-- stream.require'U16() as Binary
-- stream.require'U32()
```

```output
64
0
0
00000011 00100000
16909088
```

```yumi
/core/tests/support/byte-source
/core/tokenstream

. buffer = Byte[4]
buffer[0] = 1u8
buffer[1] = 2u8
buffer[2] = 3u8
buffer[3] = 32u8

. buffer-stream = TestByteSource(buffer)
. stream1 = ChunkedTokenStream'Byte(buffer-stream)
-- stream1.require'U32() as Binary

buffer-stream.rewind()
. stream2 = ChunkedTokenStream'Byte(buffer-stream)
-- stream2.require'U32(Endian.big) as Binary

buffer-stream.rewind()
. stream3 = ChunkedTokenStream'Byte(buffer-stream)
-- stream3.require'U32(Endian.little) as Binary
```

```output
00000001 00000010 00000011 00100000
00000001 00000010 00000011 00100000
00100000 00000011 00000010 00000001
```

```yumi
/core/tests/support/byte-source
/core/tokenstream

. data = TestByteSource'Char("abc".codepoints)
. stream = ChunkedTokenStream'Char(data)

. buf = Char[3]
. count = stream.read(3, buf) • should read-once

-- count
-- ",".join(buf[0 .. < count])
```

```output
3
a,b,c
```

```yumi
/core/tests/support/byte-source
/core/tokenstream

. complete = TestByteSource(boundary-bytes(3))
complete.data[4095] = 226u8
complete.data[4096] = 148u8
complete.data[4097] = 129u8
complete.is-encoded = true
complete.chunk-limit = 4096
. text = ChunkedTokenStream'Char(complete).read().slice-string()
-- text.length
-- text.size
-- text[4094]
-- text[4095]

. attempt = do new
    . partial = TestByteSource(boundary-bytes(2))
    partial.data[4095] = 226u8
    partial.data[4096] = 148u8
    partial.is-encoded = true
    partial.chunk-limit = 4096
    -- ChunkedTokenStream'Char(partial).read().length
catch failure
    -- failure
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ do boundary-bytes Byte[]
    - extra Size

. data = Byte[4095 + extra]
for i in 0 .. < 4095
    data[i] = 97u8
= data
```

```output
4096
4098
a
━
Unable to read UTF8 content
```

```yumi
/core/tests/support/byte-source
/core/tokenstream

. attempt = do new
    . buffer-stream = TestByteSource("foo;bar;baz" as Byte[])
    . stream = ChunkedTokenStream'Byte(buffer-stream)
    -- ", ".join(stream.split(";"))
catch failure
    -- failure
attempt.wait()
```

```output
foo, bar, baz
```

### Arena allocation returns raw pointers and honors raw alignment.

```yumi
/core/arena

. arena = Arena(128)
do finally
    arena.release()

. filled = arena.allocate'U32(4)
for index in 0 .. < 4
    filled[index] = (index * 3) as U32
. raw = arena.allocate-raw(24, 32)
. empty = arena.allocate'U64(0)

-- type-of filled
-- filled[0], filled[3]
-- raw.address % 32
-- empty.address != 0
```

```output
[TypeInfo CPointer'U32]
(0, 9)
0
true
```

```yumi
/core/arena

. arena = Arena()
do finally
    arena.release()
. value = arena.allocate'Point(1)
value[0] = Point(10, 20)
-- value[0].x
value[0] = Point(10, 30)
-- value[0]

━━━━━━━━━━━━━━━━━━━━
+ Point is Tuple

+ x I32
+ y I32
```

```output
10
(10, 30)
```

```yumi
/core/arena

. arena = Arena()
do finally
    arena.release()
. values = arena.allocate'U32(4)
-- values[0]
values[0] = 7u32
values[3] = 11u32
-- values[0], values[3]
```

```output
0
(7, 11)
```

```yumi
/core/arena

. arena = Arena()
do finally
    arena.release()

. values = arena.allocate'U32(4)
. outer = arena.mark
. scratch = arena.allocate'Byte(32)
scratch[0] = 99u8
. inner = arena.mark
. extra = arena.allocate'Point(1)
extra[0] = Point(40, 50)
arena.rewind(inner)
arena.rewind(outer)
-- arena.rewind-count

━━━━━━━━━━━━━━━━━━━━
+ Point is Tuple

+ x I32
+ y I32
```

```output
2
```

```yumi
/core/arena

. arena = Arena(64)
do finally
    arena.release()

. values = arena.allocate'U32(4)
values[0] = 7u32
. allocation-count = arena.system-block-allocation-count
arena.reset()
. rebuilt = arena.allocate'U32(4)
rebuilt[0] = 42u32
-- rebuilt[0]
-- arena.system-block-allocation-count == allocation-count
-- arena.reset-count
```

```output
42
true
1
```

### Array'Byte random-access reads and writes.

```yumi
. bytes = Byte[6]
bytes[0] = 1u8
bytes[1] = 65u8
bytes.write'U16(4660u16, 2)
bytes.write'U16(4660u16, 4, Endian.little)

-- bytes.size
-- bytes.read'U16(2)! as Binary
-- bytes.read'U16(4, Endian.little)! as Binary

bytes.write'U32(16909088u32, 1)
-- bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5]
-- bytes.read'U64(100) == null
```

```output
6
00010010 00110100
00010010 00110100
(1, 1, 2, 3, 32, 18)
true
```

```yumi
/core/arena

. arena = Arena(128)
do finally
    arena.release()

. dirty = arena.allocate'U32(4)
for index in 0 .. < 4
    dirty[index] = (index + 10) as U32
arena.reset()
. zeroed = arena.allocate'U32(4)
-- zeroed[0], zeroed[3]
-- arena.system-block-allocation-count
```

```output
(0, 0)
1
```

```yumi
/core/arena

. arena = Arena()
do finally
    arena.release()

. values = arena.allocate'U32(4)
for index in 0 .. < 4
    values[index] = (index + 1) as U32
. middle = values + 1
-- middle[0], middle[1], middle[0] + middle[1]
-- sum-arena-pointer(middle, 2)

━━━━━━━━━━━━━━━━━━━━
+ do sum-arena-pointer U32
    - pointer CPointer'U32
    - length Size

. sum = 0u32
for index in 0 .. < length
    sum += pointer[index]
= sum
```

```output
(2, 3, 5)
5
```

```yumi
/core/arena

. arena = Arena()
do finally
    arena.release()
. values = arena.allocate'U32(4)
. initial-bytes = arena.active-bytes
-- arena.try-allocate-raw(8, 3) == null
-- arena.active-bytes == initial-bytes
```

```output
true
true
```

```yumi
/core/arena

. arena = Arena()
do finally
    arena.release()
. values = arena.allocate'U32(4)
. initial-bytes = arena.active-bytes
-- arena.try-allocate'U64(Size.max) == null
-- arena.active-bytes == initial-bytes
```

```output
true
true
```

```yumi crash
. items = Array'Point(1).immutable
items.rows[0].x = 1

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
Exception: Cannot write through a read-only array view
<no file:3>
    items.rows[0].x = 1
    ^^^^^^^^^^^^^^^
```

```yumi crash
. items = Array'Point(1).immutable
items.fields.x[0] = 1

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
Exception: Cannot write through a read-only array view
<no file:3>
    items.fields.x[0] = 1
    ^^^^^^^^^^^^^^^^^
```

```yumi crash
. items = Array'Point(1).immutable
items.rows[0][0] = 1

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
Exception: Cannot write through a read-only array view
<no file:3>
    items.rows[0][0] = 1
    ^^^^^^^^^^^^^^^^
```

```yumi crash
. items = Array'Point(1).immutable
. row = items.rows[0]
row.x = 1

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
Exception: Cannot write through a read-only array view
<no file:4>
    row.x = 1
    ^^^^^
```

```yumi crash
. items = Array'Point(1).immutable
. range = items.rows[0 .. < 1]
range[0] = Point(1, 2)

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
Exception: Cannot write through a read-only array view
<no file:4>
    range[0] = Point(1, 2)
    ^^^^^^^^
```

```yumi crash
. items = Array'Point(1).immutable
. fields = items.fields.x
fields[0 .. < 1] = fields

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
Exception: Cannot write through a read-only array view
<no file:4>
    fields[0 .. < 1] = fields
    ^^^^^^^^^^^^^^^^
```

```yumi crash
. items = Array'Point(1).immutable
write(items.rows)

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int

━━━━━━━━━━━━━━━━━━━━
+ do write
    - rows ArrayRows'Point'ArrayWriteAccess

rows[0].x = 1
```

```output
Exception: No function "write" accepts (ArrayRows'Point'ArrayReadAccess); it is declared as "write(rows ArrayRows'Point'ArrayWriteAccess)"
<no file:3>
    write(items.rows)
    ^^^^^^^^^^^^^^^^^
```

```yumi crash
. items = Array'Point(1).immutable
. writable = items.rows as ArrayRows'Point'ArrayWriteAccess

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
Exception: ArrayRows'Point'ArrayReadAccess cannot be cast to ArrayRows'Point'ArrayWriteAccess; there is no conversion between them
<no file:3>
    . writable = items.rows as ArrayRows'Point'ArrayWriteAccess
                 ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```yumi crash
. items = Array'Point(1).immutable
. writable = items.rows[0] as ArrayRow'Point'ArrayWriteAccess

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
Exception: ArrayRow'Point'ArrayReadAccess cannot be cast to ArrayRow'Point'ArrayWriteAccess; there is no conversion between them
<no file:3>
    . writable = items.rows[0] as ArrayRow'Point'ArrayWriteAccess
                 ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```yumi crash
. items = Array'Point(1).immutable
. writable = items.rows[0 .. < 1] as ArrayRowRange'Point'ArrayWriteAccess

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
Exception: ArrayRowRange'Point'ArrayReadAccess cannot be cast to ArrayRowRange'Point'ArrayWriteAccess; there is no conversion between them
<no file:3>
    . writable = items.rows[0 .. < 1] as ArrayRowRange'Point'ArrayWriteAccess
                 ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```yumi crash
. items = Array'Point(1).immutable
. writable = items.fields.x as ArrayField'Int'ArrayWriteAccess

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
Exception: ArrayField'I64'ArrayReadAccess cannot be cast to ArrayField'I64'ArrayWriteAccess; there is no conversion between them
<no file:3>
    . writable = items.fields.x as ArrayField'Int'ArrayWriteAccess
                 ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```yumi crash
. items = Int[1]
-- items.rows
```

```output
Exception: Array views require a fixed-layout product aggregate
<no file:3>
    -- items.rows
       ^^^^^^^^^^
```

```yumi crash
. items = Array'(Array'Int)(1)
-- items.fields
```

```output
Exception: Array views require a fixed-layout product aggregate
<no file:3>
    -- items.fields
       ^^^^^^^^^^^^
```

```yumi crash
. items = Array'Variant(1)
-- items.rows

━━━━━━━━━━━━━━━━━━━━
+ Variant is Union

| point Point
| empty

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
Exception: Array views require a fixed-layout product aggregate
<no file:3>
    -- items.rows
       ^^^^^^^^^^
```

```yumi
. items = Array'Point([Point(3, 4), Point(5, 6)])
. rows = items.rows
rows[0].x = 9
-- items[0].x

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
9
```

```yumi
. items = Array'Point([Point(3, 4), Point(5, 6)])
. fields = items.fields.x
fields[1] = 12
-- items[1].x

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
12
```

```yumi
. items = Array'Point([Point(3, 4), Point(5, 6)])
. snapshot = items.rows[0] as Point
items.rows[0].x = 9
-- snapshot.x

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
3
```

```yumi
. items = Array'Point([Point(3, 4), Point(5, 6)])
. rows = items.rows
for row in rows
    -- row.x

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
3
5
```

```yumi
. items = Array'Point([Point(3, 4), Point(5, 6)])
-- items.rows[0 .. < 1].to-array()[0].x

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
3
```

```yumi
. items = Array'Point([Point(3, 4), Point(5, 6)])
-- items.fields.x.to-immutable-array()[1]

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
5
```

```yumi
. items = Array'Point([Point(3, 4), Point(5, 6)])
-- first-x(items.rows)
-- first-x(items.immutable.rows)

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int

━━━━━━━━━━━━━━━━━━━━
+ first-x
    - rows ArrayRows'Point'ArrayReadAccess

rows[0].x
```

```output
3
3
```

```yumi
. items = Array'Point([Point(3, 4), Point(5, 6)])
. detached = items.rows.to-array()
items.rows[0].x = 21
-- detached[0].x

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
3
```

```yumi
. items = Array'Point([Point(3, 4), Point(5, 6)])
. rows = items.immutable.rows
items.rows[0].x = 21
-- rows[0].x

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
21
```

```yumi
. items = Array'Point([Point(3, 4), Point(5, 6)])
. value = items[0]
value.x = 99
-- items[0].x

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
3
```

```yumi
. items = Array'Point([Point(3, 4), Point(5, 6)])
for value in items
    value.x = 100
-- items[0].x

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
3
```

```yumi
. items = Array'Point([Point(3, 4), Point(5, 6)])
. rows = items.rows
rows[0 .. < 1] = rows[1 .. < 2]
-- items[0].x

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
5
```

```yumi
. items = Array'Point([Point(1, 2), Point(3, 4), Point(5, 6)])
. field = items.fields.x
field[1 .. < 3] = field[0 .. < 2]
-- ", ".join(field.to-array())

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
1, 1, 3
```

```yumi
. items = Array'Pair([Pair(7, 8)])
items.rows[0][1] = 10
-- items.fields[1][0]
-- items.rows[0].left

━━━━━━━━━━━━━━━━━━━━
+ Pair is Tuple

+ left Int
+ right Int
```

```output
10
7
```

```yumi
. items = Array'Point()
-- items.rows.size, items.rows[0 .. < 0].size, items.fields.x.size

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
(0, 0, 0)
```

```yumi
/BxCore/BxCore.h

. rows = make-rows()
C:brixCollect()
-- rows[0].x

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int

━━━━━━━━━━━━━━━━━━━━
+ do make-rows ArrayRows'Point'ArrayWriteAccess

. local = Array'Point([Point(41, 42)])
= local.rows
```

```output
41
```

```yumi
do new
    . items = Array'Point([Point(3, 4), Point(5, 6)])
    . view = items.rows[0]
    . grown = items.resize(4)
    -- grown.rows.size
    -- view.x
catch failure
    -- failure

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
4
Stale array view
```

```yumi
/core/arena

. pool = Arena()
do finally
    pool.release()
. attempt = do new
    . items = Array'Point(pool, [Point(31, 32)])
    . view = items.rows[0]
    pool.reset()
    -- view.x
catch failure
    -- failure
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
Stale array view
```

```yumi
do new
    . items = Array'Point([Point(3, 4), Point(5, 6)])
    . view = items.rows[0 .. < 1]
    . grown = items.resize(4)
    -- grown.rows.size
    for row in view
        -- "unexpected loop body"
catch failure
    -- failure

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
4
Stale array view
```

```yumi
/core/arena

. pool = Arena()
do finally
    pool.release()
. attempt = do new
    . items = Array'Point(pool, [Point(31, 32)])
    . view = items.rows[0 .. < 1]
    pool.reset()
    for row in view
        -- "unexpected loop body"
catch failure
    -- failure
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
Stale array view
```

```yumi
do new
    . items = Array'Point([Point(3, 4), Point(5, 6)])
    . view = items.fields.x
    . grown = items.resize(4)
    -- grown.rows.size
    -- view[0]
catch failure
    -- failure

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
4
Stale array view
```

```yumi
/core/arena

. pool = Arena()
do finally
    pool.release()
. attempt = do new
    . items = Array'Point(pool, [Point(31, 32)])
    . view = items.fields.x
    pool.reset()
    -- view[0]
catch failure
    -- failure
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
Stale array view
```

```yumi
do new
    . items = Array'Point([Point(3, 4), Point(5, 6)])
    -- items.rows[2].x
catch failure
    -- failure

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
Array view index out of bounds
```

```yumi
do new
    . items = Array'Point([Point(3, 4), Point(5, 6)])
    -- items.rows[0 .. < 3].size
catch failure
    -- failure

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
Array view range out of bounds
```

```yumi
do new
    . items = Array'Point([Point(51, 52)])
    . row = items.rows[0]
    items = Array'Point([Point(61, 62)])
    -- items.rows[0].x
    -- row.x
catch failure
    -- failure

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
61
Stale array view
```

```yumi
do new
    . items = Array'Point([Point(71, 72), Point(73, 74)])
    . field = items.fields.x
    . smaller = items.resize(1)
    -- smaller.rows[0].x
    -- field[0]
catch failure
    -- failure

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
71
Stale array view
```

```yumi
do new
    . items = Array'(Array'Point)(1)
    items[0] = Array'Point([Point(81, 82)])
    . row = items[0].rows[0]
    items[0] = Array'Point([Point(91, 92)])
    -- items[0].rows[0].x
    -- row.x
catch failure
    -- failure

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
91
Stale array view
```

```yumi
do new
    . holder = Holder(Array'Point([Point(101, 102)]))
    . row = holder.items.rows[0]
    holder.items = Array'Point([Point(111, 112)])
    -- holder.items.rows[0].x
    -- row.x
catch failure
    -- failure

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ items Array'Point

────────────────────
+ This
    - items Array'Point
```

```output
111
Stale array view
```

```yumi
. holder = Holder(Array'Point([Point(111, 112)]))
. row = holder.items.rows[0]
holder.items = holder.items
holder.items[0] = Point(121, 122)
-- row.x

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ items Array'Point

────────────────────
+ This
    - items Array'Point
```

```output
121
```

```yumi
. items = Array'Point()
-- items.rows.to-array().size, items.immutable.fields.x.to-immutable-array().size

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
(0, 0)
```

```yumi
. items = Array'Point([Point(3, 4), Point(5, 6)])
-- items.rows[Position.first .. Position.last].size

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
2
```

```yumi
. items = Array'Point([Point(3, 4), Point(5, 6)])
. field = items.fields.x
for x in field[0 .. < 1]
    -- x

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
3
```

```yumi
/core/arena

. pool = Arena()
do finally
    pool.release()
. kept = Array'Point(pool, [Point(129, 130)])
. row = kept.rows[0]
. boundary = pool.mark
. transient = Array'Point(pool, [Point(131, 132)])
pool.rewind(boundary)
-- row.x

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
129
```

```yumi
/core/arena

. pool = Arena()
do finally
    pool.release()
. attempt = do new
    . kept = Array'Point(pool, [Point(129, 130)])
    . boundary = pool.mark
    . transient = Array'Point(pool, [Point(131, 132)])
    . row = transient.rows[0]
    pool.rewind(boundary)
    -- row.x
catch failure
    -- failure
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
Stale array view
```

```yumi
/core/arena

. attempt = do new
    . pool = Arena()
    . items = Array'Point(pool, [Point(141, 142)])
    . row = items.rows[0]
    pool.release()
    -- row.x
catch failure
    -- failure
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
Stale array view
```

```yumi
. values = Array'Point([Point(1, 2), Point(3, 4)])
-- scan(values.rows), scan(values.immutable.rows)

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int

━━━━━━━━━━━━━━━━━━━━
+ do scan'Access
    - rows ArrayRows'Point'Access

. total = 0
for row in rows
    total += row.x + row.y
= total
```

```output
(10, 10)
```

```yumi
. values = Array'Point([Point(1, 2), Point(3, 4)])
for row in values.rows
    row.x += row.y
-- values[0].x, values[1].x

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
(3, 7)
```

```yumi
. values = Array'Point([Point(1, 2), Point(3, 4)])
. rows = values.rows
for index in 0 .. < rows.length
    rows[index].x += rows[index].y
-- values[0].x, values[1].x

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
(3, 7)
```

```yumi
. values = Array'Point([Point(1, 2), Point(3, 4)])
. saved = Array'(ArrayRow'Point'ArrayWriteAccess)(2)
. position = 0
for row in values.rows
    saved[position] = row
    position += 1
saved[0].x = 20
-- values[0].x

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
20
```

```yumi
. values = Array'Point([Point(1, 2), Point(3, 4)])
. row = first(values.rows)
row.x = 30
-- values[0].x

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int

━━━━━━━━━━━━━━━━━━━━
+ do first ArrayRow'Point'ArrayWriteAccess
    - rows ArrayRows'Point'ArrayWriteAccess

for row in rows
    = row
?! Panic("empty")
```

```output
30
```

```yumi
. values = Array'Point([Point(1, 2), Point(3, 4)])
. snapshot = snapshot-first(values.rows)
values.rows[0].x = 40
-- snapshot.x

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int

━━━━━━━━━━━━━━━━━━━━
+ do snapshot-first Point
    - rows ArrayRows'Point'ArrayReadAccess

for row in rows
    = row as Point
?! Panic("empty")
```

```output
1
```

```yumi
. values = Array'Point([Point(1, 2), Point(3, 4)])
. read-later = capture-first(values.rows)
values.rows[0].x = 50
-- read-later()

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int

━━━━━━━━━━━━━━━━━━━━
+ do capture-first Reader
    - rows ArrayRows'Point'ArrayReadAccess

for row in rows
    = => row.x
?! Panic("empty")

━━━━━━━━━━━━━━━━━━━━
+ Reader is () Int
```

```output
50
```

```yumi
/core/arena

. pool = Arena()
do finally
    pool.release()
. attempt = do new
    . values = Array'Point(pool, [Point(8, 9)])
    for row in values.rows
        pool.reset()
        -- row.x
catch failure
    -- failure
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
Stale array view
```

```yumi
. values = Array'Point([Point(1, 2), Point(3, 4)])
shrink-during-indexed(values)

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int

━━━━━━━━━━━━━━━━━━━━
+ do shrink-during-indexed
    - values Array'Point

. attempt = do new
    . rows = values.rows
    for index in 0 .. < rows.length
        values = values.resize(1)
        -- rows[index].x
catch failure
    -- failure
attempt.wait()
```

```output
Stale array view
```

```yumi
do new
    . values = Array'Point([Point(1, 2), Point(3, 4)])
    . rows = values.rows
    . resized = values.resize(1)
    . count = 0
    for row in rows
        count += 1
        -- row.x
    -- count
catch failure
    -- failure

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
Stale array view
```

```yumi
/BxCore/BxCore.h
/BxGC/BxGC.h

. storage = Array'Cell(2) • with Cell(GCNode((each + 1) as Int), GCNode((each + 10) as Int))
. table = storage.rows
C:brixCollect()

if !C:brixGCVerifyObject(table[0].left as Pointer)
    -- "array-row-left-missing"
else if !C:brixGCVerifyObject(table[1].right as Pointer)
    -- "array-row-right-missing"
else
    -- "array-row-ref-ok ${table[0].left.value} ${table[1].right.value}"

━━━━━━━━━━━━━━━━━━━━
+ Cell is Struct

+ left GCNode
+ right GCNode

━━━━━━━━━━━━━━━━━━━━
+ GCNode

+ value = 0

────────────────────
+ This
    - value Int
```

```output
array-row-ref-ok 1 11
```

```yumi
. items = Array'Entity(3) • with Entity(each as I32, Point(each as I32, (each + 10) as I32))
-- ", ".join(items.fields.id)

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int

━━━━━━━━━━━━━━━━━━━━
+ Entity is Struct

+ id I32
+ position Point
```

```output
0, 1, 2
```

```yumi
. items = Array'Entity(3) • with Entity(each as I32, Point(each as I32, (each + 10) as I32))
-- ", ".join(items.fields.position)

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int

━━━━━━━━━━━━━━━━━━━━
+ Entity is Struct

+ id I32
+ position Point
```

```output
(0, 10), (1, 11), (2, 12)
```

```yumi
. items = Array'Entity(3) • with Entity(each as I32, Point(each as I32, (each + 10) as I32))
items.fields.id[1] = 42
-- items.rows[1] as Entity

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int

━━━━━━━━━━━━━━━━━━━━
+ Entity is Struct

+ id I32
+ position Point
```

```output
(42, (1, 11))
```

```yumi
. items = Array'Entity(3) • with Entity(each as I32, Point(each as I32, (each + 10) as I32))
items.fields.position[2] = Point(7, 8)
-- items.rows[2] as Entity

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int

━━━━━━━━━━━━━━━━━━━━
+ Entity is Struct

+ id I32
+ position Point
```

```output
(2, (7, 8))
```

```yumi
/core/array

. attempt = do new
    . values = Array'InlineRecord(2) • with InlineRecord("kept", each as Int)
    . snapshot = values[0]
    values[0] = InlineRecord("changed", 7)
    -- snapshot.text, snapshot.number
    -- values[0].text, values[0].number
    -- values[1].text, values[1].number
    -- values[2].text
catch failure
    -- failure
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ InlineRecord is Tuple

+ text String
+ number Int
```

```output
(kept, 0)
(changed, 7)
(kept, 1)
Index out of bounds (2 / 2)
```

```yumi
/core/array

. values = Array'InlineMutable(2) • with InlineMutable(each as Int, (each + 10) as Int)
. loaded = values[0]
loaded.x = 99
-- values[0].x, loaded.x
values[1] = loaded
loaded.y = 88
-- values[1].x, values[1].y, loaded.y

━━━━━━━━━━━━━━━━━━━━
+ InlineMutable is Struct

+ x Int
+ y Int
```

```output
(0, 99)
(99, 10, 88)
```

```yumi crash
. values = Array'I32([1]).immutable
values[0] = 2
```

```output
Exception: ImmutableArray does not support element assignment
<no file:3>
    values[0] = 2
    ^^^^^^^^^
```

```yumi crash
. values = Array'I32([1]).immutable
values.resize(4)
```

```output
Exception: ImmutableArray'I32'GCBacking has no method "resize"; did you mean "size"?
<no file:3>
    values.resize(4)
    ^^^^^^^^^^^^^^^^
```

```yumi crash
. values = Array'I32([1]).immutable
values.data[0] = 2
```

```output
Exception: Property "data" not found
<no file:3>
    values.data[0] = 2
    ^^^^^^^^^^^^^^
```

```yumi crash
. values = Array'I32([1]).immutable
values.size = 4
```

```output
Exception: Setter not found
<no file:3>
    values.size = 4
    ^^^^^^^^^^^
```

```yumi crash
accept(Array'I32([1]))

━━━━━━━━━━━━━━━━━━━━
+ accept
    - values ImmutableArray'I32

values.length
```

```output
Exception: No function "accept" accepts (Array'I32); it is declared as "accept(values ImmutableArray'I32)"
<no file:2>
    accept(Array'I32([1]))
    ^^^^^^^^^^^^^^^^^^^^^^
```

### An array of one item type is not an array of another, whose items it would read as the other.

```yumi crash
accept(Array'I32([1]).immutable)

━━━━━━━━━━━━━━━━━━━━
+ accept
    - values ImmutableArray'String

values.length
```

```output
Exception: No function "accept" accepts (ImmutableArray'I32'GCBacking); it is declared as "accept(values ImmutableArray'String)"
<no file:2>
    accept(Array'I32([1]).immutable)
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### `as` reads an array as one of another item type of the same size.

```yumi
. codes = Array'U32([104u32, 105u32])
. chars = codes as Array'Char
-- chars.length
-- chars[1]
```

```output
2
i
```

### An array cast to one whose struct items are of another size panics, as the count it keeps would reach past its storage.

```yumi
. attempt = do new
    . points = Point[2]
    . triples = points as Array'Triple
    -- triples.length
catch failure
    -- failure
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32

━━━━━━━━━━━━━━━━━━━━
+ Triple is Struct

+ a I32
+ b I32
+ c I32
```

```output
An array of 8-byte items cannot be cast to one of 12-byte items
```

```yumi
/core/array-builder

. builder = ArrayBuilder'Node()
. values = builder.finish-immutable()
. direct [Node] = values
. result = fallback(values)
-- result.length

━━━━━━━━━━━━━━━━━━━━
+ fallback [Node]
    - optional ImmutableArray'Node?

optional ?? empty

.. empty [Node]

━━━━━━━━━━━━━━━━━━━━
+ Node is Union

| items [Node]
| number I32
```

```output
0
```

### Byte arrays read and write floating-point values by their bit patterns in either byte order.

```yumi
. bytes = Byte[12]
bytes.write'F64(259.25, 0, .little)
bytes.write'F32(-82.5f32, 8)
-- bytes.read'F64(0, .little)!
-- bytes.read'F32(8)!
-- bytes[7], bytes[8]
```

```output
259.25
-82.5
(64, 194)
```

### An array of text, of objects or of structs that hold text keeps its items across collections.

```yumi
/BxGC/BxGC.h

. kept = Array'String(4)
kept[0] = "zero${kept.size}"
kept[1] = "one${kept.size}"
kept[2] = "two${kept.size}"
kept[3] = "three${kept.size}"
. boxes = Array'Box(2)
boxes[0] = Box("left${kept.size}")
boxes[1] = Box("right${kept.size}")
. pairs = Array'Pair(2)
pairs[1] = Pair(2, "two${kept.size}")
C:brixGCCollect()
. total = 0
for round in 0 .. < 2000
    . text = "junk$round"
    total += text.size
    . more = Array'String(2)
    more[1] = text
    total += more[1].size
C:brixGCCollect()
-- total
-- kept[0], kept[1], kept[2], kept[3]
-- boxes[0].name, boxes[1].name
-- pairs[1].name, pairs[1].id

━━━━━━━━━━━━━━━━━━━━
+ Box

+ name String

────────────────────
+ This
    - name String

━━━━━━━━━━━━━━━━━━━━
+ Pair is Struct

+ id Int
+ name String
```

```output
29780
(zero4, one4, two4, three4)
(left4, right4)
(two4, 2)
```

### An array held by an arena stores and reads numbers, structs, tuples and text, and its copy is apart from it.

```yumi
/core/arena

. arena = Arena()
. values = Array'U32(arena, 3)
values[0] = 10u32
values[1] = 21u32
-- values[0], values[1], values[2]
. copy = values.copy(arena)
copy[0] = 99u32
-- values[0], copy[0], copy[1]
. points = Array'Point(arena, 2)
points[1] = Point(3, 4)
-- points[0]
-- points[1]
-- points[1].y
. tuples = Array'(Int, Int)(arena, 2)
tuples[1] = (7, 8)
-- tuples[1]
. names = Array'String(arena, 2)
names[0] = "kept"
-- names[0]
arena.release()

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
(10, 21, 0)
(10, 99, 21)
(0, 0)
(3, 4)
4
(7, 8)
kept
```

### An array held by an arena is resized into the arena, larger and smaller, and keeps its items.

```yumi
/core/arena

. arena = Arena()
. values = Array'U32(arena, 2)
values[0] = 10u32
values[1] = 21u32
. grown = values.resize(4, arena)
grown[3] = 5u32
-- grown[0], grown[1], grown[2], grown[3], grown.size
. less = grown.resize(1, arena)
-- less[0], less.size
. texts = Array'String(arena, 1)
texts[0] = "kept"
. more = texts.resize(3, arena)
more[2] = "new"
-- more[0], more[2]
arena.release()
```

```output
(10, 21, 0, 5, 4)
(10, 1)
(kept, new)
```

### An array of objects held by an arena may have no items.

```yumi
/core/arena

. arena = Arena()
. lines = Array'Line(arena, 0)
-- lines.size
. after = Array'Line(arena, 1)
-- after.size
arena.release()

━━━━━━━━━━━━━━━━━━━━
+ Line

+ name String
+ parts ImmutableArray'Int
```

```output
0
1
```

### A loop over an array made with a length.

```yumi
. items = Array'Int(3)
items[1] = 5
for item in items
    -- item
```

```output
0
5
0
```

### A range of the items of an array is assigned one value.

```yumi
. buffer = Array'Int(5)
buffer[1 .. < 3] = 7
for item in buffer
    -- item
```

```output
0
7
7
0
0
```

### An array is sliced by ranges of positions, of a number and a position, and by one with its end left open.

```yumi
. buffer = Array'Float(4)
buffer[0] = 0.5
buffer[1] = 1.5
buffer[2] = 2.5
buffer[3] = 3.5
-- ", ".join(buffer[Position.first .. < 2])
-- ", ".join(buffer[1 .. < Position.end])
-- ", ".join(buffer[Position.middle .. Position.last])
-- ", ".join(buffer[1 ...])
```

```output
0.5, 1.5
1.5, 2.5, 3.5
2.5, 3.5
1.5, 2.5, 3.5
```

### Ranges with a side left open assign one value to the items of an array.

```yumi
. buffer = Array'Int(5)
buffer[3 ...] = 7
buffer[... 1] = 9
for item in buffer
    -- item
```

```output
9
0
0
7
7
```

```yumi
. large = Array'U8(4098)
large[4097] = 7u8
-- read-large(large)
-- large[4096]

━━━━━━━━━━━━━━━━━━━━
+ read-large U8
    - values Array'U8

values[4097]
```

```output
7
0
```

```yumi crash
. items = Array'Point(1).immutable
for row in items.rows
    row.x = 1

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
Exception: Cannot write through a read-only array view
<no file:4>
        row.x = 1
        ^^^^^
```

### An element dropped when resize shrinks an array is released.

```yumi
/core/gc

/BxGC/BxGC.h

. resized = shrunk-buffer()
C:brixCollect()
-- resized[0].size
-- resized[1].get == null

━━━━━━━━━━━━━━━━━━━━
+ do shrunk-buffer

. buffer = Array'GCNode(8)
for i in 0 .. < 8
    buffer[i] = GCNode(i)

. tail = Weak(buffer[7])
buffer = buffer.resize(2)
= (buffer, tail)

━━━━━━━━━━━━━━━━━━━━
+ GCNode

+ value = 0

────────────────────
+ This
    - v Int

value = v
```

```output
2
true
```

```yumi
/BxGC/BxGC.h

. anchor = Byte[1048576]
anchor[0] = 7u8
anchor[1048575] = 19u8
dirty-megabytes()
C:brixGCCollect()
C:brixGCScavenge()
-- anchor[0], anchor[1048575]
. fresh = Byte[1048576]
. sum = 0 as Size
for index in 0 .. < fresh.length
    sum += fresh[index] as Size
-- sum

━━━━━━━━━━━━━━━━━━━━
+ do dirty-megabytes

for allocation in 0 .. < 16
    . bytes = Byte[1048576]
    for index in 0 .. < bytes.length
        bytes[index] = 255u8
```

```output
(7, 19)
0
```

```yumi crash
. values = Array'Point([Point(1, 2), Point(3, 4), Point(5, 6)])
. row = values.rows[2]
-= values[0]
-- values.length
-- row.x

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```stderr
Unhandled panic: Stale array view
<no file:6>

Task 1

Stack trace:
  0  __source__:main no file:6:4
  1  __source__:init.__source__ no file
  2  __source__:brix.main no file
```

### An array tested as a condition, alone or beside another, is true where it is not empty.

```yumi
. empty = Array'U32(0).immutable
. full = Array'U32(2).immutable
-- check(empty)
-- check(full)
-- both(empty, true)
-- both(full, true)
-- either(empty, false)
-- either(full, false)
. mutable = Array'U32(0)
-- if mutable then "mutable" else "none"

━━━━━━━━━━━━━━━━━━━━
+ do check String
    - params

=? "truthy" if params
= "falsy"

━━━━━━━━━━━━━━━━━━━━
+ do both String
    - params
    - flag Bool

=? "both" if params && flag
= "not both"

━━━━━━━━━━━━━━━━━━━━
+ do either String
    - params
    - flag Bool

=? "either" if params || flag
= "neither"
```

```output
falsy
truthy
not both
both
neither
either
none
```

### Compound assignment to an item past a pointer reads the item and stores the answer in the item's type.

```yumi
/core/arena

. arena = Arena()
do finally arena.release()
. counts = arena.allocate'U32(4)
. bytes = arena.allocate'U8(2)
. reals = arena.allocate'F64(1)
counts[1] = 10
counts[1] += 5
. place = 2
counts[place + 1] = 3
counts[place + 1] *= 4
bytes[0] = 200u8
bytes[0] += 50u8
bytes[1] = 7u8
bytes[1] *= 2
reals[0] = 1.5
reals[0] *= 3
reals[0] /= 2
-- "${counts[1]} ${counts[3]} ${bytes[0]} ${bytes[1]} ${reals[0]}"
```

```output
15 12 250 14 2.25
```

```yumi
. count = 16777216
. values = Int[count]
for index in 0 .. < count
    values[index] = index % 7
. total = 0
for index in 0 .. < count
    total += values[index]
-- total
```

```output
50331645
```

### A value the array's index operator does not take is refused, not stored in the array's own fields

```yumi compile-error
. codes = Char[2]
. unit U32 = 65
codes[0] = unit
```

```output
Exception: Illegal types for operation: Array'Char and U32
<no file:4>
    codes[0] = unit
    ^^^^^^^^
```

### A value converted to the array's item type is stored

```yumi
. codes = Char[2]
. unit U32 = 65
codes[0] = unit as Char
codes[1] = 'B'
-- codes[0 .. < 2].slice-string()
```

```output
AB
```

### An array of a type made with a count that becomes a Size only when asked is refused, not made empty

```yumi compile-error
. count U64 = 3
-- Byte[count].length
```

```output
Exception: Expected Size here, but this is U64, which becomes Size only when asked; write "as Size"
<no file:3>
    -- Byte[count].length
            ^^^^^
```

### An index assigned as a value gives what the item then holds, a value of another number converted.

```yumi
. wide = I64[2]
. n I32 = 3
-- wide[0] = n
. halves = Float[1]
-- halves[0] = 1
. list [Float] = [0.5]
-- list[0] = 2
```

```output
3
1
2
```

### An array of bytes made of a text holds a copy of its bytes, so writing the array leaves the text as it was.

```yumi
. text = "hello"
. bytes = Array'Byte(text)
bytes[0] = 74
-- text
-- bytes.slice-string()
```

```output
hello
Jello
```

### A text cast to bytes gives a copy of its bytes, so writing them leaves the text, a literal here, as it was.

```yumi
. text = "hello"
. bytes = text as Byte[]
bytes[0] = 74
-- text
-- bytes.slice-string()
```

```output
hello
Jello
```
