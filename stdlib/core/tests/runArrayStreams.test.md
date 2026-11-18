# Array streams

```yumi
/core/array-builder
. mutable = Array((1 .. 3) *> (each * 2))
. immutable = ImmutableArray((1 .. 3) *> (each * 3), 20)
-- type-of mutable
-- type-of immutable
-- ", ".join(mutable)
-- ", ".join(immutable)
. empty = Array((1 .. 3) ?> false, 100)
-- empty.length, empty.data.address
```

```output
[TypeInfo Array'I64]
[TypeInfo ImmutableArray'I64'GCBacking]
2, 4, 6
3, 6, 9
(0, 0)
```

```yumi
/core/array-builder
/core/arena
. arena = Arena()
do finally
    arena.release()
. initial-bytes = arena.active-bytes
. empty = Array(arena, (1 .. 3) ?> false, 100)
. frozen = ImmutableArray(arena, (1 .. 3) ?> false, 100)
-- empty.length, empty.data.address, frozen.length
-- arena.active-bytes == initial-bytes
. mutable = Array(arena, (1 .. 3) *> (each * 2))
. immutable = ImmutableArray(arena, (1 .. 3) *> (each * 3), 20)
-- type-of mutable
-- type-of immutable
-- ", ".join(mutable)
-- ", ".join(immutable)
```

```output
(0, 0, 0)
true
[TypeInfo Array'I64'ArenaBacking]
[TypeInfo ImmutableArray'I64'ArenaBacking]
2, 4, 6
3, 6, 9
```

```yumi
/core/array-builder
. values = Array((1 .. < 1) *> each, Size.max)
. frozen = ImmutableArray((1 .. < 1) *> each, Size.max)
-- values.length, values.data.address, frozen.length
for item in values
    -- "unexpected"
for item in frozen
    -- "unexpected"
-- values[0 .. < 0].length, frozen[0 .. < 0].length
-- values.copy().length
-- values.immutable.length
-- (values as Array'Byte).length
. grown = values.resize(2)
grown[0] = 7
grown[1] = 9
-- ", ".join(grown)
```

```output
(0, 0, 0)
(0, 0)
0
0
0
7, 9
```

```yumi
/core/array-builder
/core/arena
. arena = Arena()
do finally
    arena.release()
. values = Array(arena, (1 .. < 1) *> each, Size.max)
. frozen = ImmutableArray(arena, (1 .. < 1) *> each, Size.max)
-- values.length, values.data.address, frozen.length
for item in values
    -- "unexpected"
for item in frozen
    -- "unexpected"
-- values[0 .. < 0].length, frozen[0 .. < 0].length
-- values.copy(arena).length
-- values.immutable.length
-- (values as Array'Byte'ArenaBacking).length
. grown = values.resize(2, arena)
grown[0] = 7
grown[1] = 9
-- ", ".join(grown)
```

```output
(0, 0, 0)
(0, 0)
0
0
0
7, 9
```

```yumi
/core/array-builder
for capacity in [0, 1, 65, 256]
    . mutable = Array(source(), capacity as Size)
    . frozen = ImmutableArray(source(), capacity as Size)
    -- mutable.length, frozen.length
    . total = 0
    for item in mutable
        total += item
    for item in frozen
        total += item
    -- total, mutable[64], frozen[64]
. singleton = Array((7 .. 7) *> each)
-- singleton.length, singleton[0]

━━━━━━━━━━━━━━━━━━━━
+ do source @inline

for i in 0 .. < 65
    <= i
```

```output
(65, 65)
(4160, 64, 64)
(65, 65)
(4160, 64, 64)
(65, 65)
(4160, 64, 64)
(65, 65)
(4160, 64, 64)
(1, 7)
```

```yumi
/core/array-builder
/core/arena
. arena = Arena()
do finally
    arena.release()
for capacity in [0, 1, 65, 256]
    . mutable = Array(arena, source(), capacity as Size)
    . frozen = ImmutableArray(arena, source(), capacity as Size)
    -- mutable.length, frozen.length
    . total = 0
    for item in mutable
        total += item
    for item in frozen
        total += item
    -- total, mutable[64], frozen[64]
. singleton = Array(arena, (7 .. 7) *> each)
-- singleton.length, singleton[0]

━━━━━━━━━━━━━━━━━━━━
+ do source @inline

for i in 0 .. < 65
    <= i
```

```output
(65, 65)
(4160, 64, 64)
(65, 65)
(4160, 64, 64)
(65, 65)
(4160, 64, 64)
(65, 65)
(4160, 64, 64)
(1, 7)
```

```yumi
/core/array-builder
/BxGC/BxGC.h
/core/arena
. arena = Arena()
do finally
    arena.release()
. allocated = C:brixGCAllocatedObjectCount()
. a = Array((1 .. < 1) *> each, Size.max)
. b = ImmutableArray((1 .. 3) ?> false, Size.max)
. c = Array(arena, (1 .. < 1) *> each, Size.max)
. d = ImmutableArray(arena, (1 .. 3) ?> false, Size.max)
. delta = C:brixGCAllocatedObjectCount() - allocated
-- delta
-- arena.active-bytes, arena.system-block-allocation-count
-- a.length, b.length, c.length, d.length
```

```output
0
(0, 0)
(0, 0, 0, 0)
```

```yumi
/core/array-builder
. mutable =
    ** source()
    *> each * 2
    ?> each > 2
    |> Array
. frozen =
    ** source()
    * 5
    |> ImmutableArray
-- ", ".join(mutable)
-- ", ".join(frozen)

━━━━━━━━━━━━━━━━━━━━
+ do source @inline

do finally
    -- "closed"
for i in 1 .. 3
    -- "yield $i"
    <= i
```

```output
yield 1
yield 2
yield 3
closed
yield 1
yield 2
yield 3
closed
4, 6
1, 2, 3, 5
```

```yumi
/core/array-builder
/BxGC/BxGC.h
. first = ArrayStreamBox("first")
. second = ArrayStreamBox("second")
. mutable = Array(source(first, second), 256)
. frozen = ImmutableArray(source(first, second), 256)
C:brixGCCollect()
mutable[0].text = "changed"
-- frozen[0].text, first.text
-- mutable[1].text, frozen[1].text

━━━━━━━━━━━━━━━━━━━━
+ ArrayStreamBox
+ text String
────────────────────
+ This
    - text

━━━━━━━━━━━━━━━━━━━━
+ do source @inline
    - first ArrayStreamBox
    - second ArrayStreamBox

<= first
C:brixGCCollect()
<= second
```

```output
(changed, changed)
(second, second)
```

```yumi
/core/array-builder
/BxGC/BxGC.h
/core/arena
. arena = Arena()
do finally
    arena.release()
. first = ArrayStreamBox("first")
. second = ArrayStreamBox("second")
. mutable = Array(arena, source(first, second), 256)
. frozen = ImmutableArray(arena, source(first, second), 256)
C:brixGCCollect()
mutable[0].text = "changed"
-- frozen[0].text, first.text
-- mutable[1].text, frozen[1].text

━━━━━━━━━━━━━━━━━━━━
+ ArrayStreamBox
+ text String
────────────────────
+ This
    - text

━━━━━━━━━━━━━━━━━━━━
+ do source @inline
    - first ArrayStreamBox
    - second ArrayStreamBox

<= first
C:brixGCCollect()
<= second
```

```output
(changed, changed)
(second, second)
```

```yumi
/core/array-builder
. result = catch collect()
-- result is ArrayStreamFailure

━━━━━━━━━━━━━━━━━━━━
+ ArrayStreamFailure is Error

━━━━━━━━━━━━━━━━━━━━
+ do collect Int ?! ArrayStreamFailure

. values = Array(source())
= values.length as Int

━━━━━━━━━━━━━━━━━━━━
+ do source <Int> ?! ArrayStreamFailure @inline

do finally
    -- "closed"
<= 1
-- "fail"
?! ArrayStreamFailure("failed")
<= 2
```

```output
fail
closed
true
```

```yumi
/core/array-builder
. result = catch collect()
-- result is ArrayStreamFailure

━━━━━━━━━━━━━━━━━━━━
+ ArrayStreamFailure is Error

━━━━━━━━━━━━━━━━━━━━
+ do collect Int ?! ArrayStreamFailure

. values = ImmutableArray(source())
= values.length as Int

━━━━━━━━━━━━━━━━━━━━
+ do source <Int> ?! ArrayStreamFailure @inline

do finally
    -- "closed"
<= 1
-- "fail"
?! ArrayStreamFailure("failed")
<= 2
```

```output
fail
closed
true
```

```yumi
/core/array-builder
. attempt = do new
    . values = Array((1 .. < 1) *> each)
    -- values[0]
catch failure
    -- failure
attempt.wait()
```

```output
Index out of bounds (0 / 0)
```

```yumi
/core/array-builder
. attempt = do new
    . values = Array((1 .. < 1) *> each)
    -- values[0] = 1
catch failure
    -- failure
attempt.wait()
```

```output
Set out of bounds (0 / 0)
```

```yumi
/core/array-builder
. attempt = do new
    . values = Array((1 .. < 1) *> each)
    -- values[0 .. 1]
catch failure
    -- failure
attempt.wait()
```

```output
Range out of bounds (1 / 0)
```

```yumi
/core/array-builder
/core/arena
. arena = Arena()
do finally
    arena.release()
. attempt = do new
    . values = Array(arena, (1 .. < 1) *> each)
    -- values[0]
catch failure
    -- failure
attempt.wait()
```

```output
Index out of bounds (0 / 0)
```

```yumi
/core/array-builder
/core/arena
. arena = Arena()
do finally
    arena.release()
. attempt = do new
    . values = Array(arena, (1 .. < 1) *> each)
    -- values[0] = 1
catch failure
    -- failure
attempt.wait()
```

```output
Set out of bounds (0 / 0)
```

```yumi
/core/array-builder
/core/arena
. arena = Arena()
do finally
    arena.release()
. attempt = do new
    . values = Array(arena, (1 .. < 1) *> each)
    -- values[0 .. 1]
catch failure
    -- failure
attempt.wait()
```

```output
Range out of bounds (1 / 0)
```

```yumi
/core/array-builder
/core/arena
. arena = Arena()
do finally
    arena.release()
. mutable =
    ** 1 .. 3
    *> each * 2
    |> Array(arena, ...)
. frozen =
    ** 1 .. 3
    *> each * 3
    |> ImmutableArray(arena, ...)
-- ", ".join(mutable)
-- ", ".join(frozen)
```

```output
2, 4, 6
3, 6, 9
```

```yumi
/core/array-builder
/BxGC/BxGC.h
. mutable = Array(source())
. frozen = ImmutableArray(source(), 256)
C:brixGCCollect()
-- mutable.length, frozen.length
-- mutable[0].text, frozen[64].text

━━━━━━━━━━━━━━━━━━━━
+ ArrayStreamBox
+ text String
────────────────────
+ This
    - text

━━━━━━━━━━━━━━━━━━━━
+ do source @inline

for i in 0 .. < 65
    <= ArrayStreamBox("item $i")
    C:brixGCCollect()
```

```output
(65, 65)
(item 0, item 64)
```

```yumi
/core/array-builder
/core/arena
. arena = Arena()
do finally
    arena.release()
. result = catch collect(arena)
-- result is ArrayStreamFailure

━━━━━━━━━━━━━━━━━━━━
+ ArrayStreamFailure is Error

━━━━━━━━━━━━━━━━━━━━
+ do collect Int ?! ArrayStreamFailure
    - arena Arena

. values = Array(arena, source(), 20)
= values.length as Int

━━━━━━━━━━━━━━━━━━━━
+ do source <Int> ?! ArrayStreamFailure @inline

do finally
    -- "closed"
<= 1
-- "fail"
?! ArrayStreamFailure("failed")
<= 2
```

```output
fail
closed
true
```

```yumi
/core/array-builder
/core/arena
. arena = Arena()
do finally
    arena.release()
. result = catch collect(arena)
-- result is ArrayStreamFailure

━━━━━━━━━━━━━━━━━━━━
+ ArrayStreamFailure is Error

━━━━━━━━━━━━━━━━━━━━
+ do collect Int ?! ArrayStreamFailure
    - arena Arena

. values = ImmutableArray(arena, source(), 20)
= values.length as Int

━━━━━━━━━━━━━━━━━━━━
+ do source <Int> ?! ArrayStreamFailure @inline

do finally
    -- "closed"
<= 1
-- "fail"
?! ArrayStreamFailure("failed")
<= 2
```

```output
fail
closed
true
```

```yumi
/core/array-builder
/BxGC/BxGC.h
for capacity in [65, 256]
    . input = source()
    . initial-count = C:brixGCAllocatedObjectCount()
    . initial-bytes = C:brixGCRequestedByteCount()
    . values = Array(input, capacity as Size)
    . count = C:brixGCAllocatedObjectCount() - initial-count
    . bytes = C:brixGCRequestedByteCount() - initial-bytes
    -- count, bytes, values.length, values[64]

━━━━━━━━━━━━━━━━━━━━
+ do source

for i in 0 .. < 65
    <= i
```

```output
(2, 552, 65, 64)
(2, 2080, 65, 64)
```

```yumi
/core/array-builder
/BxGC/BxGC.h
for capacity in [65, 256]
    . input = source()
    . initial-count = C:brixGCAllocatedObjectCount()
    . initial-bytes = C:brixGCRequestedByteCount()
    . values = ImmutableArray(input, capacity as Size)
    . count = C:brixGCAllocatedObjectCount() - initial-count
    . bytes = C:brixGCRequestedByteCount() - initial-bytes
    -- count, bytes, values.length, values[64]

━━━━━━━━━━━━━━━━━━━━
+ do source

for i in 0 .. < 65
    <= i
```

```output
(2, 552, 65, 64)
(2, 2080, 65, 64)
```
