# Bitwise

```brix
(main
  (print (/and/ 0xff 0x0f))
)
```

```output
15
```

```brix
(main
  (print (/or/ 0xf0 0x0f))
)
```

```output
255
```

```brix
(main
  (print (/xor/ 0xf0 0x0f))
)
```

```output
255
```

```brix
(main
  (print (as (/lshift/ 0x00ff 1) Hex))
)
```

```output
0x01fe
```

```brix
(main
  (print (as (/rshift/ 0x00ff 1) Hex))
)
```

```output
0x007f
```

```brix
(main
  (print (as (/zrshift/ 0x00ffffff 1) Hex))
)
```

```output
0x007fffff
```

```brix
(main
  (print (as (/and/ 255i1 1) UnitString))
  (print (as (/and/ 255i8 1) UnitString))
  (print (as (/and/ 255i16 1) UnitString))
  (print (as (/and/ 255i32 1) UnitString))
  (print (as (/and/ 255i64 1) UnitString))
)
```

```output
1i1
1i8
1i16
1i32
1i
```

### A bitwise operation takes no float: one becomes an integer only when asked.

```brix crash
(main
  (print (/and/ 16.5f32 16))
)
```

```output
Exception: Expected I64 here, but this is F32, which becomes I64 only when asked; write "(as 16.5f32 I64)"
<no file:3>
      (print (/and/ 16.5f32 16))
                    ^^^^^^^
```

```brix
(main
  (print (/not/ 0 1u8))
  (print (/not/ 0 254u8))
)
```

```output
254
1
```

### A float is shifted once `as` makes it an integer.

```brix
(main
  (print (as (/lshift/ (as 7.9 I32) 1) UnitString))
  (print (as (/lshift/ 1 (as 2.5 I32)) UnitString))
  (print (as (/rshift/ (as 7.9 I32) 1) UnitString))
  (print (as (/zrshift/ (as 7.9 I64) (as 1.5 I64)) UnitString))
)
```

```output
14i32
4i32
3i32
3i
```

### A bitwise operator on a number narrower than 32 bits extends it by its own sign first.

```brix
(main
  (print (/zrshift/ -16i8 2))
  (print (/rshift/ -16i8 2))
  (print (/rshift/ 240u8 2))
  (print (/zrshift/ -16i16 2))
  (print (/rshift/ 129u8 1))
  (print (/rshift/ -127i8 1))
  (print (as (/lshift/ 100i8 1) UnitString))
  (print (/lshift/ 1u8 7))
  (print (/lshift/ 1u8 8))
  (print (/lshift/ 1i8 7))
  (print (/lshift/ 1i16 15))
  (print (/lshift/ 200u8 1))
  (print (as (/not/ 0 5u8) UnitString))
  (print (/not/ 0 5))
  (print (/and/ 200u8 0xF0))
  (print (/or/ 200u8 0x100))
  (print (as (/or/ 0x100 200u8) UnitString))
  (print (/xor/ 200u8 255u8))
  (print (/lshift/ 1 31))
  (print (/lshift/ 1 32))
  (print (/lshift/ 1i64 32))
  (print (/lshift/ 1i64 63))
  (print (/zrshift/ -16 2))
  (print (/zrshift/ -16i64 2))
  (print (/rshift/ 4000000000u32 4))
  (print -200u8)
  (print (- (+ 1 2)))
)
```

```output
-4
-4
60
-4
64
-64
-56i8
128
0
-128
-32768
144
250u8
-6
192
456
456u16
55
2147483648
4294967296
4294967296
-9223372036854775808
4611686018427387900
4611686018427387900
4276531840
56
-3
```

### `count-leading-zeros` counts in the width of its argument.

```brix
(import /core/intrinsics)
(blank)

(main
  (let n 5)
  (print (call count-leading-zeros 5))
  (print (call count-leading-zeros 1u8))
  (print (call count-leading-zeros n))
  (print (call count-leading-zeros 0))
)
```

```output
61
7
61
64
```

### A shift to the right of an integer narrower than a register shifts the value with its own sign, whatever a cast left above it.

```brix
(main
  (let x U16 200)
  (let y (call I8 x))
  (print (/rshift/ y 1))
  (print (/zrshift/ y 1))
  (let a U16 40000)
  (let b (call I16 a))
  (print (/rshift/ b 1))
  (print (/zrshift/ b 1))
  (let d I16 -2)
  (let e (call U16 d))
  (print (/rshift/ e 1))
  (print (/zrshift/ e 1))
  (let n I8 -16)
  (print (/rshift/ n 2))
  (print (/zrshift/ n 2))
  (print (call half n))
)

(fn-do + half I8
  (param n I8)
  (return (/rshift/ n 1))
)
```

```output
-28
-28
-12768
-12768
32767
32767
-4
-4
-8
```

### A shift to the right of a 64-bit integer narrowed by `as` keeps the bits the shift brings down from the high word.

```brix
(main
  (print (call top-byte 0x9e3779b97f4a7c15))
  (print (call top-bit 0x9e3779b97f4a7c15))
  (print (call top-word 0x9e3779b97f4a7c15))
)

(fn-expr + top-byte U8
  (param state U64)
  (as (/zrshift/ state 56) U8)
)

(fn-expr + top-bit U8
  (param state U64)
  (as (/zrshift/ state 63) U8)
)

(fn-expr + top-word U32
  (param state U64)
  (as (/zrshift/ state 40) U32)
)
```

```output
158
1
10368889
```

### A shift to the right of a 64-bit integer narrowed to 32 bits brings down none of the bits above them, whatever widens its result afterward.

```brix
(main
  (print (call low-byte 0x9e3779b97f4a7c15))
  (print (call low-half 0x9e3779b97f4a7c15))
)

(fn-expr + low-byte U64
  (param state U64)
  (as (/zrshift/ (as state U32) 24) U8 U64)
)

(fn-expr + low-half U64
  (param state U64)
  (as (/zrshift/ (as state U32) 16) U16 U64)
)
```

```output
127
32586
```

```brix crash
(import /core/intrinsics)
(blank)

(main
  (print (call count-leading-zeros 1.0))
)
```

```output
Exception: count-leading-zeros requires an integer argument
<no file:6>
      (print (call count-leading-zeros 1.0))
             ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### count-leading-zeros counts within the width of its argument, and zero gives the whole width.

```brix
(import /core/intrinsics)
(blank)

(main
  (print (tuple (call count-leading-zeros 0u8) (call count-leading-zeros 1u8)))
  (print (tuple (call count-leading-zeros 0u16) (call count-leading-zeros 1u16)))
  (print (tuple (call count-leading-zeros 0u32) (call count-leading-zeros 1u32)))
  (print (tuple (call count-leading-zeros 0u64) (call count-leading-zeros 1u64)))
)
```

```output
(8, 7)
(16, 15)
(32, 31)
(64, 63)
```

### A bitwise operator takes no optional operand.

```brix crash
(main
  (do-new
    (call mixed 12 2)
    (on-failure failure
      (print failure)
    )
  )
)

(fn-do _ mixed _
  (param key (? U64))
  (param amount (? U8))
  (print (/zrshift/ key amount))
)
```

```output
Exception: Expected U64 here, but this is U64?, which may hold nothing; write "(!! key)" to unwrap it, or "(?? key fallback)" to give a value for nothing
<no file:14>
      (print (/zrshift/ key amount))
                        ^^^
```
