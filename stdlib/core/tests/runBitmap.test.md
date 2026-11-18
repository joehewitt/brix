# Bitmap

```yumi
. value = 7 as Value'U64
value[0] = 9ul
-- value as U64
━━━━━━━━━━━━━━━━━━━━
+ Value'Integer is Integer
────────────────────
+ this[index Index]

(this as Integer) + (index as Integer)
────────────────────
+ this[index Index] = value Integer

value as This
```

```output
9
```

```yumi
. value = 7 as Value'I8
. copy = value
value[0] = -8 as I8
-- value as I8
-- copy as I8
━━━━━━━━━━━━━━━━━━━━
+ Value'Integer is Integer
────────────────────
+ this[index Index]

(this as Integer) + (index as Integer)
────────────────────
+ this[index Index] = value Integer

value as This
```

```output
-8
7
```

```yumi
. holder = Holder(7 as Value'U64)
holder.value[0] = 9ul
-- holder.value as U64
━━━━━━━━━━━━━━━━━━━━
+ Value'Integer is Integer
────────────────────
+ this[index Index]

(this as Integer) + (index as Integer)
────────────────────
+ this[index Index] = value Integer

value as This
━━━━━━━━━━━━━━━━━━━━
+ Holder

+ value Value'U64
────────────────────
+ This
    - value
```

```output
9
```

```yumi
. values = Array'(Value'U64)([7 as Value'U64])
values[0][0] = 9ul
-- values[0] as U64
━━━━━━━━━━━━━━━━━━━━
+ Value'Integer is Integer
────────────────────
+ this[index Index]

(this as Integer) + (index as Integer)
────────────────────
+ this[index Index] = value Integer

value as This
```

```output
9
```

```yumi
. probe = Probe()
probe.values()[probe.index()][probe.index()] = 9ul
-- probe.items[0] as U64
-- probe.calls
-- probe.indices
━━━━━━━━━━━━━━━━━━━━
+ Value'Integer is Integer
────────────────────
+ this[index Index]

(this as Integer) + (index as Integer)
────────────────────
+ this[index Index] = value Integer

value as This
━━━━━━━━━━━━━━━━━━━━
+ Probe

+ items = Array'(Value'U64)([7 as Value'U64])
+ calls = 0
+ indices = 0
────────────────────
+ do values

calls += 1
= items
────────────────────
+ do index

indices += 1
= 0
```

```output
9
1
2
```

### What []= answers for a number an index gave is stored through that index

```yumi
+ kept = Array'(Count'U8)([0 as Count'U8])

. counts = Array'(Count'U8)([0 as Count'U8, 0 as Count'U8])
counts[1][3] = 10u8
counts[1][1] = 20u8
counts[0][7] = 1u8
-- counts[0] as U8, counts[1] as U8
kept[0][2] = 5u8
-- kept[0] as U8
. longs = Array'(Count'I64)([5 as Count'I64])
. copy = longs[0]
longs[0][1] = -8
-- longs[0] as I64, copy as I64

━━━━━━━━━━━━━━━━━━━━
+ Count'Integer is Integer

────────────────────
+ this[index Index] = value Integer

((this as Integer) + value + (index as Integer)) as This
```

```output
(8, 34)
7
(-2, 5)
```

### What a number's []= answers is stored where the number is held: a local, a property, an argument

```yumi
. value = 1 as Count'U8
value[2] = 10u8
-- value as U8
. holder = Holder(3 as Count'U8)
holder.count[1] = 4u8
-- holder.count as U8
holder.add(5u8)
-- holder.count as U8
-- raise(value) as U8, value as U8

━━━━━━━━━━━━━━━━━━━━
+ do raise Count'U8
    - given Count'U8

given[0] = 100u8
= given

━━━━━━━━━━━━━━━━━━━━
+ Count'Integer is Integer

────────────────────
+ this[index Index] = value Integer

((this as Integer) + value + (index as Integer)) as This

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ count Count'U8

────────────────────
+ This
    - count

────────────────────
+ do add
    - value U8

count[0] = value
```

```output
13
8
13
(113, 13)
```

```yumi
/core/bitmap

for capacity in [0, 1, 63, 64, 65, 130]
    . bits = BitmapBuffer(capacity)
    -- "${bits.capacity} ${bits.count(false)} ${bits.count(true)}"
```

```output
0 0 0
1 1 0
63 63 0
64 64 0
65 65 0
130 130 0
```

```yumi
/core/bitmap

read(Bitmap'U8(), 8)
read(Bitmap'U8(), Size.max)
write(Bitmap'U8(), 8)
write(Bitmap'U8(), Size.max)
read(BitmapBuffer(8), 8)
read(BitmapBuffer(8), Size.max)
write(BitmapBuffer(8), 8)
write(BitmapBuffer(8), Size.max)
read(BitmapBuffer(0), 0)
read(BitmapBuffer(0), Size.max)
write(BitmapBuffer(0), 0)
write(BitmapBuffer(0), Size.max)

━━━━━━━━━━━━━━━━━━━━
_ do read'Bits
    - bits Bits
    - index Index

. attempt = do new
    . value = bits[index]
catch failure
    . panic Panic? = failure
    -- "read $index: ${panic is OutOfBoundsError} $panic"
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
_ do write'Bits
    - bits Bits
    - index Index

. attempt = do new
    bits[index] = true
catch failure
    . panic Panic? = failure
    -- "write $index: ${panic is OutOfBoundsError} $panic"
attempt.wait()
```

```output
read 8: true Bitmap index out of bounds
read 9223372036854775807: true Bitmap index out of bounds
write 8: true Bitmap index out of bounds
write 9223372036854775807: true Bitmap index out of bounds
read 8: true Bitmap index out of bounds
read 9223372036854775807: true Bitmap index out of bounds
write 8: true Bitmap index out of bounds
write 9223372036854775807: true Bitmap index out of bounds
read 0: true Bitmap index out of bounds
read 9223372036854775807: true Bitmap index out of bounds
write 0: true Bitmap index out of bounds
write 9223372036854775807: true Bitmap index out of bounds
```

```yumi
/core/bitmap

read(Bitmap'U8(), -1)
write(Bitmap'U8(), -1)
read(BitmapBuffer(8), -1)
write(BitmapBuffer(8), -1)

━━━━━━━━━━━━━━━━━━━━
_ do read'Bits
    - bits Bits
    - index Index

. attempt = do new
    . value = bits[index]
catch failure
    -- "read $index: ${failure is OutOfBoundsError} $failure"
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
_ do write'Bits
    - bits Bits
    - index Index

. attempt = do new
    bits[index] = true
catch failure
    -- "write $index: ${failure is OutOfBoundsError} $failure"
attempt.wait()
```

```output
read -1: true Bitmap index out of bounds
write -1: true Bitmap index out of bounds
read -1: true Bitmap index out of bounds
write -1: true Bitmap index out of bounds
```

```yumi
/core/bitmap

for index in 0 .. < 8
    . raw = (1 as I8) /lshift/ index
    . bits = Bitmap'I8(raw)
    -- "$raw ${bits as I8} ${bits[index]} ${bits.count(true)}"
. one = Bitmap'I1(1 as I1)
-- "${one as I1} ${one[0]}"
. top = Bitmap'I64((1 as I64) /lshift/ 63)
-- "${top as I64} ${top[63]} ${top.count(true)}"
. high = Bitmap'U64(9223372036854775809ul)
-- "${high as U64} ${high[63]} ${high[0]} ${high.count(true)}"
. ones = Bitmap'I64(-1l)
-- "${ones as I64} ${ones.count(true)}"
```

```output
1 1 true 1
2 2 true 1
4 4 true 1
8 8 true 1
16 16 true 1
32 32 true 1
64 64 true 1
-128 -128 true 1
1 true
-9223372036854775808 true 1
9223372036854775809 true true 2
-1 64
```

```yumi
/core/bitmap

. bits = BitmapBuffer(65)
. alias = bits
alias[64] = true
-- bits[64]
alias.fill(true)
-- bits.count(true)
. copy = bits.copy()
bits[64] = false
-- copy[64]
copy[0] = false
-- bits[0]
-- copy.capacity
. empty = BitmapBuffer(0).copy()
-- "${empty.capacity} ${empty.count(true)}"
```

```output
true
65
true
true
65
0 0
```

```yumi
/core/arena
/core/bitmap

. source-arena = Arena()
. target-arena = Arena()
. bits = BitmapBuffer(source-arena, 65)
bits[64] = true
. copy = bits.copy(target-arena)
source-arena.release()
-- copy[64]
copy[0] = true
-- copy.count(true)
target-arena.release()
```

```output
true
2
```

```yumi
/core/bitmap

. empty = Bitmap'I1()
. full = empty.fill(true)
-- "${full.count(true)} ${full.count(false)} ${empty.count(false)} ${full.fill(false) as I1}"
. wide-empty = Bitmap'U64()
. wide-full = wide-empty.fill(true)
-- "${wide-full.count(true)} ${wide-full.count(false)} ${wide-empty.count(false)}"
-- wide-full as U64
-- wide-full.fill(false) as U64
. sparse = Bitmap'U8()
for index in [0, 3, 7]
    sparse[index] = true
-- "${sparse.count(true)} ${sparse.count(false)} ${sparse as U8}"
```

```output
1 0 1 0
64 0 64
18446744073709551615
0
3 5 137
```

```yumi
/core/bitmap

for capacity in [0, 1, 64, 65, 130]
    . bits = BitmapBuffer(capacity)
    bits.fill(true)
    . set = bits.count(true)
    . clear = bits.count(false)
    bits.fill(false)
    -- "$set $clear ${bits.count(false)} ${bits.capacity}"
. sparse = BitmapBuffer(8)
for index in [0, 3, 7]
    sparse[index] = true
-- "${sparse.count(true)} ${sparse.count(false)}"
```

```output
0 0 0 0
1 0 1 1
64 0 64 64
65 0 65 65
130 0 130 130
3 5
```

```yumi
/core/bitmap

. bits = Bitmap'I8()
bits[7] = true
-- "${bits[7]} ${bits.count(true)} ${bits as I8}"
bits[7] = false
-- bits as I8
. flags = Bitmap'U8()
flags[3] = true
flags[3] = true
-- "${flags[3]} ${flags.count(true)}"
flags[4] = true
flags[3] = false
flags[3] = false
-- "${flags[3]} ${flags[4]} ${flags.count(true)}"
```

```output
true 1 -128
0
true 1
false true 1
```

```yumi
/core/bitmap

. bits = BitmapBuffer(130)
for index in [0, 63, 64, 129]
    bits[index] = true
    -- "${bits[index]} ${bits.count(true)} ${bits.find(true)!}"
    bits[index] = false
    -- bits.count(true)
```

```output
true 1 0
0
true 1 63
0
true 1 64
0
true 1 129
0
```

```yumi
/core/bitmap

. bits = Bitmap'U8()
. iterator = bits as Iterable'Bool
bits = bits.fill(true)
. clear = 0
for bit in iterator
    if !bit
        clear += 1
-- clear
-- bits.count(true)
. changing = Bitmap'U8()
. seen = 0
for bit in changing
    changing = changing.fill(true)
    if !bit
        seen += 1
-- seen
. source = Bitmap'U8()
. indices = source.indices(false)
source = source.fill(true)
. count = 0
for index in indices
    count += 1
-- count
```

```output
8
8
8
8
```

```yumi
/core/bitmap

. bits = BitmapBuffer(3)
. iterator = bits as Iterable'Bool
bits[2] = true
for bit in iterator
    -- "before $bit"
. live = BitmapBuffer(3)
for bit in live
    -- "during $bit"
    live[2] = true
. source = BitmapBuffer(3)
. snapshot = source.copy() as Iterable'Bool
source.fill(true)
for bit in snapshot
    -- "copy $bit"
```

```output
before false
before false
before true
during false
during false
during true
copy false
copy false
copy false
```

```yumi
/core/bitmap

. packed = Bitmap'U8(66 as U8)
for index in packed.indices(true)
    -- "packed true $index"
for index in packed.indices(false)
    -- "packed false $index"
. buffer = BitmapBuffer(8)
buffer[1] = true
buffer[6] = true
for index in buffer.indices(true)
    -- "buffer true $index"
for index in buffer.indices(false)
    -- "buffer false $index"
. empty = BitmapBuffer(0)
for bit in empty
    -- "empty bit"
for index in empty.indices(false)
    -- "empty index"
-- "done"
```

```output
packed true 1
packed true 6
packed false 0
packed false 2
packed false 3
packed false 4
packed false 5
packed false 7
buffer true 1
buffer true 6
buffer false 0
buffer false 2
buffer false 3
buffer false 4
buffer false 5
buffer false 7
done
```

```yumi
/BxGC/BxGCTest.h
/core/bitmap

. iterator = retained-iterator()
C:brixCollect()
. seen = 0
for bit in iterator
    if bit
        seen += 1
-- seen

━━━━━━━━━━━━━━━━━━━━
_ do retained-iterator

. bits = BitmapBuffer(257)
bits.fill(true)
= bits as Iterable'Bool
```

```output
257
```

```yumi
/core/bitmap

. bits = Bitmap'U8()
-- bits.find(true)
-- bits.fill(true).find(false)
bits[6] = true
bits[2] = true
-- bits.find(true)
-- bits.find(false)
bits[0] = true
bits[1] = true
-- bits.find(false)
```

```output
unknown
unknown
2
0
3
```

```yumi
/core/bitmap

. bits = Bitmap'U8()
-- bits.find-consecutive(0, false)
-- bits.find-consecutive(8, false)
-- bits.find-consecutive(9, false)
-- bits.find-consecutive(Size.max, false)
for index in [1, 2, 5, 6, 7]
    bits[index] = true
-- bits.find-consecutive(2, true)
-- bits.find-consecutive(3, true)
-- bits.find-consecutive(4, true)
-- bits.find-consecutive(2, false)
-- bits.find-consecutive(3, false)
. ends = Bitmap'U8()
for index in [0, 1, 6, 7]
    ends[index] = true
-- ends.find-consecutive(3, true)
-- ends.find-consecutive(4, false)
. alternating = Bitmap'U8()
for index in [0, 2, 4, 6]
    alternating[index] = true
-- alternating.find-consecutive(2, true)
-- alternating.find-consecutive(2, false)
```

```output
unknown
0
unknown
unknown
1
5
unknown
3
unknown
unknown
2
unknown
unknown
```

```yumi
/core/bitmap

. bits = BitmapBuffer(193)
for index in 60 .. < 140
    bits[index] = true
-- bits.find-consecutive(80, true)
-- bits.find-consecutive(81, true)
. word = BitmapBuffer(130)
for index in 62 .. < 66
    word[index] = true
-- word.find-consecutive(4, true)
-- word.find-consecutive(5, true)
. padded = BitmapBuffer(65)
padded.fill(true)
-- padded.find(false)
-- padded.find-consecutive(1, false)
-- padded.find-consecutive(65, true)
. empty = BitmapBuffer(0)
-- empty.find(true)
-- empty.find(false)
-- empty.find-consecutive(1, false)
```

```output
60
unknown
62
unknown
unknown
unknown
0
unknown
unknown
unknown
```

### BitmapBuffer rejects a negative capacity and counts the bits of a partial final word.

```yumi
/core/arena
/core/bitmap

. arena = Arena()
for capacity in [-1, -64]
    . gc = do new
        . bits = BitmapBuffer(capacity)
    catch failure
        -- "gc $capacity: $failure"
    gc.wait()
    . arena-attempt = do new
        . bits = BitmapBuffer(arena, capacity)
    catch failure
        -- "arena $capacity: $failure"
    arena-attempt.wait()
arena.release()
for capacity in [63, 64, 65]
    . bits = BitmapBuffer(capacity)
    bits.fill(true)
    -- "${bits.capacity} ${bits[capacity - 1]} ${bits.count(true)} ${bits.count(false)}"
```

```output
gc -1: BitmapBuffer capacity is negative
arena -1: BitmapBuffer capacity is negative
gc -64: BitmapBuffer capacity is negative
arena -64: BitmapBuffer capacity is negative
63 true 63 0
64 true 64 0
65 true 65 0
```

