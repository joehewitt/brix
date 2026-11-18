# Bitmap types

```yumi
-- I1.bit-width
-- I8.bit-width
-- U16.bit-width
-- I32.bit-width
-- U64.bit-width
```

```output
1
8
16
32
64
```

```yumi
-- Size.bit-width
-- Byte.bit-width
-- Bool.bit-width
```

```output
64
8
1
```

```yumi
-- I1.is-integer
-- F64.is-integer
```

```output
true
false
```

```yumi
-- I1.min
-- I1.max
```

```output
0
1
```

```yumi
/core/bitmap

. narrow = Bitmap'I8()
-- narrow.capacity
-- (type-of narrow).size
-- narrow as I8
. wide = Bitmap'U64()
-- wide.capacity
-- (type-of wide).size
-- wide as U64
```

```output
8
1
0
64
8
0
```

```yumi
/core/bitmap

-- Bitmap'Char().capacity
-- Bitmap'Char8().capacity
-- Bitmap'Index().capacity
-- Bitmap'Flags().capacity

━━━━━━━━━━━━━━━━━━━━
_ Flags is U16
```

```output
32
8
64
16
```

```yumi crash
Bits'F32()

━━━━━━━━━━━━━━━━━━━━
+ Bits'Integer

────────────────────
+ This

Integer.bit-width
```

```output
Exception: Integer type required for bit-width
<no file:10>
    Integer.bit-width
    ^^^^^^^^^^^^^^^^^
```

```yumi crash
Bits'F64()

━━━━━━━━━━━━━━━━━━━━
+ Bits'Integer

────────────────────
+ This

Integer.bit-width
```

```output
Exception: Integer type required for bit-width
<no file:10>
    Integer.bit-width
    ^^^^^^^^^^^^^^^^^
```

```yumi crash
Bits'Float()

━━━━━━━━━━━━━━━━━━━━
+ Bits'Integer

────────────────────
+ This

Integer.bit-width
```

```output
Exception: Integer type required for bit-width
<no file:10>
    Integer.bit-width
    ^^^^^^^^^^^^^^^^^
```
