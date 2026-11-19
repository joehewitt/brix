# Numbers

Bounds for each numeric type

### Unsuffixed integer literals retain their exact written values across default and annotated types.

```brix
(main
  (let annotated-negative I64 -2147483649)
  (let large-negative -5000000000)
  (let minimum-i64 I64 -9223372036854775808)
  (let annotated-u32 U32 3000000000)
  (blank)
  (print annotated-negative)
  (print large-negative)
  (print 3000000000)
  (print minimum-i64)
  (print -2147483648)
  (print -2147483649)
  (print 2147483647)
  (print 2147483648)
  (print 4294967295)
  (print 4294967296)
  (print annotated-u32)
)
```

```output
-2147483649
-5000000000
3000000000
-9223372036854775808
-2147483648
-2147483649
2147483647
2147483648
4294967295
4294967296
3000000000
```

### An unsuffixed integer literal cannot be narrowed by an annotation that it does not fit.

```brix crash
(main
  (let value I32 3000000000)
  (print value)
)
```

```output
Exception: Integer literal 3000000000 does not fit I32
<no file:3>
      (let value I32 3000000000)
                     ^^^^^^^^^^
```

### A whole number an arm of a match gives must fit the type the function returns.

```brix crash
(main
  (print (call pick 0))
)

(fn-expr + pick U8
  (param index Int)
  (match index
    (case 0
      3
    )
    (default
      300
    )
  )
)
```

```output
Exception: Integer literal 300 does not fit U8
<no file:13>
          300
          ^^^
```

### An unsuffixed integer literal given to an optional must fit what the optional holds.

```brix crash
(main
  (let value (? I32) 3000000000)
  (print (!! value))
)
```

```output
Exception: Integer literal 3000000000 does not fit I32
<no file:3>
      (let value (? I32) 3000000000)
                         ^^^^^^^^^^
```

### An unsuffixed integer literal given to a union must fit the case that holds it.

```brix crash
(main
  (let value Value 3000000000)
  (print value)
)

(type + Value (is Union)
  (variant I32)
  (variant String)
)
```

```output
Exception: Integer literal 3000000000 does not fit I32
<no file:3>
      (let value Value 3000000000)
                       ^^^^^^^^^^
```

### An unsuffixed integer literal outside the supported 64-bit range does not compile.

```brix crash
(main
  (print 99999999999999999999)
)
```

```output
Exception: Integer literal 99999999999999999999 does not fit U64
<no file:3>
      (print 99999999999999999999)
             ^^^^^^^^^^^^^^^^^^^^
```

### A positive unsuffixed integer above I64 infers U64 and retains its exact value.

```brix
(main
  (let inferred 9223372036854775808)
  (let value U64 inferred)
  (print value)
)
```

```output
9223372036854775808
```

### A known I64 context rejects the same literal because it does not fit.

```brix crash
(main
  (let value I64 9223372036854775808)
  (print value)
)
```

```output
Exception: Integer literal 9223372036854775808 does not fit I64
<no file:3>
      (let value I64 9223372036854775808)
                     ^^^^^^^^^^^^^^^^^^^
```

### A hexadecimal literal outside the supported 64-bit range does not compile.

```brix crash
(main
  (print 0xfffffffffffffffff)
)
```

```output
Exception: Integer literal 295147905179352825855 does not fit U64
<no file:3>
      (print 0xfffffffffffffffff)
             ^^^^^^^^^^^^^^^^^^^
```

### A width-suffixed literal cannot exceed the supported 64-bit bit pattern.

```brix crash
(main
  (print 18446744073709551616u64)
)
```

```output
Exception: Integer literal 18446744073709551616 does not fit U64
<no file:3>
      (print 18446744073709551616u64)
             ^^^^^^^^^^^^^^^^^^^^^^^
```

```brix
(main
  (print -1i8)
  (print 0i8)
  (print 127i8)
  (print 128i8)
  (print (as 128i16 I8))
  (print "")
  (print (as 127i8 U8))
  (print (as 127i8 I16))
  (print (as 127i8 U16))
  (print (as 127i8 I32))
  (print (as 127i8 U32))
  (print (as 127i8 I64))
  (print (as 127i8 U64))
  (print "")
  (print (as -128i8 U8))
  (print (as -128i8 I16))
  (print (as -128i8 U16))
  (print (as -128i8 I32))
  (print (as -128i8 U32))
  (print (as -128i8 I64))
  (print (as -128i8 U64))
  (print "")
  (print (as 127i8 F32))
  (print (as 127i8 F64))
)
```

```output
-1
0
127
-128
-128

127
127
127
127
127
127
127

128
-128
65408
-128
4294967168
-128
18446744073709551488

127
127
```

```brix
(main
  (print -1u8)
  (print 0u8)
  (print 255u8)
  (print "")
  (print (as 255u8 I8))
  (print (as 255u8 I16))
  (print (as 255u8 I32))
  (print (as 255u8 I64))
  (print "")
  (print (as 255u8 U16))
  (print (as 255u8 U32))
  (print (as 255u8 U64))
  (print "")
  (print (as 255u8 F32))
  (print (as 255u8 F64))
)
```

```output
255
0
255

-1
255
255
255

255
255
255

255
255
```

```brix
(main
  (print -1i16)
  (print 0i16)
  (print 32767i16)
  (print "")
  (print (as 32767i16 I8))
  (print (as 32767i16 I16))
  (print (as 32767i16 I32))
  (print (as 32767i16 I64))
  (print "")
  (print (as 32767i16 U8))
  (print (as 32767i16 U32))
  (print (as 32767i16 U64))
  (print "")
  (print (as 32767i16 F32))
  (print (as 32767i16 F64))
)
```

```output
-1
0
32767

-1
32767
32767
32767

255
32767
32767

32767
32767
```

```brix
(main
  (print -1u16)
  (print 0u16)
  (print 65535u16)
  (print "")
  (print (as 65535u16 I8))
  (print (as 65535u16 I16))
  (print (as 65535u16 I32))
  (print (as 65535u16 I64))
  (print "")
  (print (as 65535u16 U8))
  (print (as 65535u16 U32))
  (print (as 65535u16 U64))
  (print "")
  (print (as 65535u16 F32))
  (print (as 65535u16 F64))
)
```

```output
65535
0
65535

-1
-1
65535
65535

255
65535
65535

65535
65535
```

```brix
(main
  (print -1i32)
  (print -2147483648i32)
  (print 0i32)
  (print 2147483647i32)
  (print 2147483648i32)
  (print "")
  (print (as 2147483647i32 I8))
  (print (as 2147483647i32 I16))
  (print (as 2147483647i32 I64))
  (print "")
  (print (as 2147483647i32 U8))
  (print (as 2147483647i32 U16))
  (print (as 2147483647i32 U32))
  (print (as 2147483647i32 U64))
  (print "")
  (print (as 2147483647i32 F32))
  (print (as 2147483647i32 F64))
)
```

```output
-1
-2147483648
0
2147483647
-2147483648

-1
-1
2147483647

255
65535
2147483647
2147483647

2.1474836e+09
2147483647
```

```brix
(main
  (print -1u32)
  (print 0u32)
  (print 4294967295u32)
  (print "")
  (print (as 4294967295u32 I8))
  (print (as 4294967295u32 I16))
  (print (as 4294967295u32 I32))
  (print (as 4294967295u32 I64))
  (print "")
  (print (as 4294967295u32 U8))
  (print (as 4294967295u32 U16))
  (print (as 4294967295u32 U64))
  (print "")
  (print (as 4294967295u32 F32))
  (print (as 4294967295u32 F64))
)
```

```output
4294967295
0
4294967295

-1
-1
-1
4294967295

255
65535
4294967295

4.2949673e+09
4294967295
```

```brix
(main
  (print -9223372036854775807i64)
  (print -9223372036854775808i64)
  (print -1i64)
  (print 0i64)
  (print 9223372036854775807i64)
  (print "")
  (print (as 9223372036854775807i64 I8))
  (print (as 9223372036854775807i64 I16))
  (print (as 9223372036854775807i64 I32))
  (print "")
  (print (as 9223372036854775807i64 U8))
  (print (as 9223372036854775807i64 U16))
  (print (as 9223372036854775807i64 U32))
  (print (as 9223372036854775807i64 U64))
  (print "")
  (print (as 9223372036854775807i64 F32))
  (print (as 9223372036854775807i64 F64))
)
```

```output
-9223372036854775807
-9223372036854775808
-1
0
9223372036854775807

-1
-1
-1

255
65535
4294967295
9223372036854775807

9.223372e+18
9.223372036854776e+18
```

```brix
(main
  (print -1u64)
  (print 0u64)
  (print 18446744073709551615u64)
  (print "")
  (print (as 18446744073709551615u64 I8))
  (print (as 18446744073709551615u64 I16))
  (print (as 18446744073709551615u64 I32))
  (print (as 18446744073709551615u64 I64))
  (print "")
  (print (as 18446744073709551615u64 U8))
  (print (as 18446744073709551615u64 U16))
  (print (as 18446744073709551615u64 U32))
  (print "")
  (print (as 18446744073709551615u64 F32))
  (print (as 18446744073709551615u64 F64))
)
```

```output
18446744073709551615
0
18446744073709551615

-1
-1
-1
-1

255
65535
4294967295

1.8446744e+19
1.8446744073709552e+19
```

## Scientific notation

```brix
(main
  (print 4.2e+0)
  (print 4.2e-0)
  (print 4.2e+1)
  (print 4.2e-1)
  (print 1.7976931348623158e+308)
  (print 1.7976931348623159e+308)
  (print 2.7976931348623158e+308)
  (print (as 4.2e+0 UnitString))
)
```

```output
4.2
4.2
42
0.42
1.7976931348623157e+308
inf
inf
4.2f
```

### Numeric formats larger than the stack fast path retain their complete ASCII output.

```brix
(main
  (let n 5)
  (let padded (call (. n pad) 200))
  (let big 1.0)
  (for (in i (..< 0 300))
    (= big (* big 10.0))
  )
  (let formatted (str big))
  (print (. padded size))
  (print (== padded (++ (* "0" 199) "5")))
  (print (. formatted size))
  (print ([] formatted (..< 0 16)))
  (print ([] formatted (..< 301 308)))
)
```

```output
200
true
23
1.00000000000000
```

## Hex notation

```brix
(main
  (print (as 0xff UnitString))
  (print (as 0xffff UnitString))
  (print (as 0xffffffff UnitString))
  (print (as 0xffffffffffffffff UnitString))
)
```

```output
255u8
65535u16
4294967295u32
18446744073709551615u
```

### A hexadecimal literal's unit is the smallest one that holds every digit written.

```brix
(main
  (print (as 0xf UnitString))
  (print (as 0x000f UnitString))
  (print (as 0x400 UnitString))
  (print (as 0x12345 UnitString))
  (print (as 0x1'0000'0000 UnitString))
)
```

```output
15u8
15u16
1024u16
74565u32
4294967296u
```

```brix
(main
  (print (as 255u8 Hex))
  (print (as 65535u16 Hex))
  (print (as 4294967295u Hex))
  (print (as 18446744073709551615ul Hex))
)
```

```output
0xff
0xffff
0x00000000ffffffff
0xffffffffffffffff
```

## Math operations

```brix
(main
  (print (+ 155u8 100))
  (print (- 255u8 100))
  (print (* 127u8 2))
  (print (/ 255u8 2))
  (print (% 255u8 2))
)
```

```output
255
155
254
127
1
```

```brix
(main
  (print (+ 2147483647u 100))
  (print (- 4294967295u 100))
  (print (* 2147483647u 2))
  (print (/ 4294967295u 2))
  (print (% 4294967295u 2))
)
```

```output
2147483747
4294967195
4294967294
2147483647
1
```

```brix
(main
  (let i8 8i8)
  (let i16 16i16)
  (let i32 32i32)
  (let i64 64i64)
  (print (+ i8 i8))
  (print (+ i8 i16))
  (print (+ i8 i32))
  (print (+ i8 i64))
  (print (+ i16 i8))
  (print (+ i16 i16))
  (print (+ i16 i32))
  (print (+ i16 i64))
  (print (+ i32 i8))
  (print (+ i32 i16))
  (print (+ i32 i32))
  (print (+ i32 i64))
  (print (+ i64 i8))
  (print (+ i64 i16))
  (print (+ i64 i32))
  (print (+ i64 i64))
)
```

```output
16
24
40
72
24
32
48
80
40
48
64
96
72
80
96
128
```

```brix
(main
  (let i8 8i8)
  (let i16 16i16)
  (let i32 32i32)
  (let i64 64i64)
  (let f32 32.0f32)
  (let f64 64.0f64)
  (print (+ i8 f32))
  (print (+ i8 f64))
  (print (+ i16 f32))
  (print (+ i16 f64))
  (print (+ i32 f32))
  (print (+ i32 f64))
  (print (+ i64 f32))
  (print (+ i64 f64))
  (print (+ f32 i8))
  (print (+ f32 i16))
  (print (+ f32 i32))
  (print (+ f32 i64))
  (print (+ f64 i8))
  (print (+ f64 i16))
  (print (+ f64 i32))
  (print (+ f64 i64))
  (print (+ f32 f64))
  (print (+ f64 f32))
)
```

```output
40
72
48
80
64
96
96
128
40
48
64
96
72
80
96
128
96
96
```

```brix
(main
  (print (- 15 3))
  (print (- 15.0 3))
  (print (- 15.0 3.5))
  (print (- 15 3.5))
)
```

```output
12
12
11.5
11.5
```

```brix
(main
  (print (* 15 3))
  (print (* 15.0 3))
  (print (* 15.0 3.5))
  (print (* 15 3.5))
)
```

```output
45
45
52.5
52.5
```

```brix
(main
  (print (/ 15 3))
  (print (/ 15.0 3))
  (print (/ 15.0 3.5))
  (print (/ 15 3.5))
)
```

```output
5
5
4.285714285714286
4.285714285714286
```

```brix
(main
  (print (% 15 2))
  (print (% 15.0 2))
  (print (% 15.0 2.0))
  (print (% 15 2.0))
)
```

```output
1
1
1
1
```

```brix
(main
  (print (%% 5 2))
  (print (%% 5.0 2))
  (print (%% 5.0 2.0))
  (print (%% 5 2.0))
)
```

```output
25
25
25
25
```

```brix
(main
  (let x 5)
  (print (- x))
)
```

```output
-5
```

```brix
(main
  (let x 5.0)
  (print (- x))
)
```

```output
-5
```

```brix crash
(main
  (let x "foo")
  (print (- x))
)
```

```output
Exception: Operator "-" is not defined for String
<no file:4>
      (print (- x))
             ^^^^^
```

```brix
(main
  (let x 1)
  (print (not x))
)
```

```output
false
```

```brix
(main
  (let x 0)
  (print (not x))
)
```

```output
true
```

```brix
(main
  (let x 1.0)
  (print (not x))
)
```

```output
false
```

```brix
(main
  (let x 0.0)
  (print (not x))
)
```

```output
true
```

```brix
(main
  (print (== 5 5))
  (print (== 5.0 5.0))
  (print (== 5.0f64 5.0f32))
  (print (== 5 5.0))
  (print (== 5.0 5))
)
```

```output
true
true
true
true
true
```

```brix
(main
  (print (== 5 3))
  (print (== 5.0 3.0))
  (print (== 5.0f64 3.0f32))
  (print (== 5 3.0))
  (print (== 5 5.1))
  (print (== 5.0 3))
)
```

```output
false
false
false
false
false
false
```

```brix
(main
  (print (!= 5 5))
  (print (!= 5.0 5.0))
  (print (!= 5.0f64 5.0f32))
  (print (!= 5 5.0))
  (print (!= 5.0 5))
)
```

```output
false
false
false
false
false
```

```brix
(main
  (print (!= 5 3))
  (print (!= 5.0 3.0))
  (print (!= 5.0f64 3.0f32))
  (print (!= 5 3.0))
  (print (!= 5 5.1))
  (print (!= 5.0 3))
)
```

```output
true
true
true
true
true
true
```

```brix
(main
  (print (> 5 3))
  (print (>= 5 3))
  (print (>= 5 5))
  (print (< 5 3))
  (print (<= 5 3))
  (print (<= 5 5))
)
```

```output
true
true
true
false
false
true
```

```brix
(main
  (print (> 5.0 3.0))
  (print (>= 5.0 3.0))
  (print (>= 5.0 5.0))
  (print (< 5.0 3.0))
  (print (<= 5.0 3.0))
  (print (<= 5.0 5.0))
  (print (< 3.0 3.5))
  (print (< 3.5 3.0))
)
```

```output
true
true
true
false
false
true
true
false
```

```brix
(main
  (print (> 5.0 3))
  (print (>= 5.0 3))
  (print (>= 5.0 5))
  (print (< 5.0 3))
  (print (<= 5.0 3))
  (print (<= 5.0 5))
)
```

```output
true
true
true
false
false
true
```

```brix
(main
  (print (> 5 3.0))
  (print (>= 5 3.0))
  (print (>= 5 5.0))
  (print (< 5 3.0))
  (print (<= 5 3.0))
  (print (<= 5 5.0))
)
```

```output
true
true
true
false
false
true
```

```brix crash
(main
  (print (> 5 (call Foo)))
)

(type + Foo)
```

```output
Exception: Operator ">" is not defined for I64 and Foo
<no file:3>
      (print (> 5 (call Foo)))
             ^^^^^^^^^^^^^^^^
```

```brix
(main
  (let a 1)
  (+= a 3)
  (print a)
)
```

```output
4
```

```brix
(main
  (let a 1)
  (-= a 3)
  (print a)
)
```

```output
-2
```

```brix
(main
  (let a 2)
  (%%= a 4)
  (print a)
  (blank)
  (let b 2.0)
  (%%= b 4.0)
  (print b)
)
```

```output
16
16
```

```brix
(main
  (print (as 0 Bool))
  (print (as 1 Bool))
  (print (as 2 Bool))
)
```

```output
false
true
true
```

```brix
(main
  (print (as 0 I1))
  (print (as 1 I1))
  (print (as 2 I1))
)
```

```output
0
1
0
```

### Numeric class method

```brix
(main
  (let f (call Foo 44))
  (call (. f foo))
)

(type + Foo (alias ff) (is I32)
  (fn-do + foo _
    (print (str "foo " this " " unit))
  )
)
```

```output
foo 44 ff
```

### unit read from a number names the unit its type is written with.

```brix
(main
  (let x 3foo)
  (print (. x unit))
  (print (. 2 unit))
  (print (call show 4foo))
  (call (. 5foo label))
)

(fn-do + show String
  (param n Foo)
  (return (. n unit))
)

(type + Foo (alias foo) (is F64)
  (fn-do + label _
    (print (str this " " (. this unit)))
  )
)
```

```output
foo
i
foo
5 foo
```

```brix
(main
  (print (as (call I8 1) UnitString))
  (print (as (call I8 "2") UnitString))
)
```

```output
1i8
2i8
```

### Signed flag

```brix
(main
  (print (. I8 is-signed))
  (print (. U8 is-signed))
  (print "")
  (print (. I16 is-signed))
  (print (. U16 is-signed))
  (print "")
  (print (. I32 is-signed))
  (print (. U32 is-signed))
  (print "")
  (print (. I64 is-signed))
  (print (. U64 is-signed))
  (print "")
  (print (. F32 is-signed))
  (print (. F64 is-signed))
  (print "")
  (print (. Bool is-signed))
  (print (. Char is-signed))
)
```

```output
true
false

true
false

true
false

true
false

true
true

false
false
```

### Minimum and maximum numbers

```brix
(main
  (print (== (call (of get-min I8)) (. I8 min)))
  (print (== (call (of get-min U8)) (. U8 min)))
  (print (== (call (of get-max I8)) (. I8 max)))
  (print (== (call (of get-max U8)) (. U8 max)))
  (print "")
  (print (== (call (of get-min I16)) (. I16 min)))
  (print (== (call (of get-min U16)) (. U16 min)))
  (print (== (call (of get-max I16)) (. I16 max)))
  (print (== (call (of get-max U16)) (. U16 max)))
  (print "")
  (print (== (call (of get-min I32)) (. I32 min)))
  (print (== (call (of get-min U32)) (. U32 min)))
  (print (== (call (of get-max I32)) (. I32 max)))
  (print (== (call (of get-max U32)) (. U32 max)))
  (print "")
  (print (== (call (of get-min I64)) (. I64 min)))
  (print (== (call (of get-min U64)) (. U64 min)))
  (print (== (call (of get-max I64)) (. I64 max)))
  (print (== (call (of get-max U64)) (. U64 max)))
  (print "")
  (print (== -3.402823466e+38 (. F32 min)))
  (print (== 3.402823466e+38 (. F32 max)))
  (print "")
  (print (== -1.7976931348623157e+308 (. F64 min)))
  (print (== 1.7976931348623157e+308 (. F64 max)))
  (print "")
  (print (== false (. Bool min)))
  (print (== true (. Bool max)))
  (print "")
  (print (== (call (of get-min U32)) (. Char min)))
  (print (== (call (of get-max U32)) (. Char max)))
)

(fn-do _ (of get-min T) _
  (let min (call T))
  (if (. T is-signed)
    (= min (/or/ min (/lshift/ (call T 1) (- (* (. T size) 8) 1))))
  )
  (return min)
)

(fn-do _ (of get-max T) _
  (let max (call T))
  (let bits (- (* (. T size) 8) (if-expr (. T is-signed) 1 0)))
  (for (in i (..< 0 bits))
    (= max (/or/ max (/lshift/ (call T 1) i)))
  )
  (return max)
)
```

```output
true
true
true
true

true
true
true
true

true
true
true
true

true
true
true
true

true
true

true
true

true
true

true
true
```

## Integer promotion preserves high bits and inferred return types

```brix
(main
  (print (call add 1i32 4294967296i64))
  (print (call reverse 1i32 4294967296i64))
  (print (. (type-of (call add 1i32 4294967296i64)) name))
  (print (. (type-of (call reverse 1i32 4294967296i64)) name))
  (print (+ 1i32 4294967296i64))
  (print (. (type-of (+ 1i32 4294967296i64)) name))
)

(fn-expr + add _
  (param left I32)
  (param right I64)
  (+ left right)
)

(fn-expr + reverse _
  (param left I32)
  (param right I64)
  (+ right left)
)
```

```output
4294967297
4294967297
I64
I64
4294967297
I64
```

## Integer comparisons preserve values across widths and signs

```brix
(main
  (call compare 1i8 257i32)
  (call compare -1i64 18446744073709551615u64)
  (call compare 9223372036854775807i64 9223372036854775808u64)
  (call compare 0i64 0u64)
  (print (== 1i8 257i32))
  (print (< -1i64 18446744073709551615u64))
)

(fn-do + (of compare L R) _
  (param left L)
  (param right R)
  (print (== left right))
  (print (== right left))
  (print (!= left right))
  (print (!= right left))
  (print (< left right))
  (print (> right left))
  (print (<= left right))
  (print (>= right left))
  (print (> left right))
  (print (< right left))
  (print (>= left right))
  (print (<= right left))
)
```

```output
false
false
true
true
true
true
true
true
false
false
false
false
false
false
true
true
true
true
true
true
false
false
false
false
false
false
true
true
true
true
true
true
false
false
false
false
true
true
false
false
false
false
true
true
false
false
true
true
false
true
```

## Every integer width compares boundary values in both orders

```brix
(main
  (call check-order -128i8 -128i8 0)
  (call check-order 127i8 127i8 0)
  (call check-order -128i8 127i8 -1)
  (call check-order 127i8 -128i8 1)
  (call check-order 0i8 0i8 0)
  (call check-order 1i8 1i8 0)
  (call check-order -128i8 0u8 -1)
  (call check-order 127i8 255u8 -1)
  (call check-order -128i8 255u8 -1)
  (call check-order 127i8 0u8 1)
  (call check-order 0i8 0u8 0)
  (call check-order 1i8 1u8 0)
  (call check-order 127i8 127u8 0)
  (call check-order -128i8 -32768i16 1)
  (call check-order 127i8 32767i16 -1)
  (call check-order -128i8 32767i16 -1)
  (call check-order 127i8 -32768i16 1)
  (call check-order 0i8 0i16 0)
  (call check-order 1i8 1i16 0)
  (call check-order 127i8 127i16 0)
  (call check-order -128i8 0u16 -1)
  (call check-order 127i8 65535u16 -1)
  (call check-order -128i8 65535u16 -1)
  (call check-order 127i8 0u16 1)
  (call check-order 0i8 0u16 0)
  (call check-order 1i8 1u16 0)
  (call check-order 127i8 127u16 0)
  (call check-order -128i8 -2147483648i32 1)
  (call check-order 127i8 2147483647i32 -1)
  (call check-order -128i8 2147483647i32 -1)
  (call check-order 127i8 -2147483648i32 1)
  (call check-order 0i8 0i32 0)
  (call check-order 1i8 1i32 0)
  (call check-order 127i8 127i32 0)
  (call check-order -128i8 0u32 -1)
  (call check-order 127i8 4294967295u32 -1)
  (call check-order -128i8 4294967295u32 -1)
  (call check-order 127i8 0u32 1)
  (call check-order 0i8 0u32 0)
  (call check-order 1i8 1u32 0)
  (call check-order 127i8 127u32 0)
  (call check-order -128i8 -9223372036854775808i64 1)
  (call check-order 127i8 9223372036854775807i64 -1)
  (call check-order -128i8 9223372036854775807i64 -1)
  (call check-order 127i8 -9223372036854775808i64 1)
  (call check-order 0i8 0i64 0)
  (call check-order 1i8 1i64 0)
  (call check-order 127i8 127i64 0)
  (call check-order -128i8 0u64 -1)
  (call check-order 127i8 18446744073709551615u64 -1)
  (call check-order -128i8 18446744073709551615u64 -1)
  (call check-order 127i8 0u64 1)
  (call check-order 0i8 0u64 0)
  (call check-order 1i8 1u64 0)
  (call check-order 127i8 127u64 0)
  (call check-order 0u8 -128i8 1)
  (call check-order 255u8 127i8 1)
  (call check-order 0u8 127i8 -1)
  (call check-order 255u8 -128i8 1)
  (call check-order 0u8 0i8 0)
  (call check-order 1u8 1i8 0)
  (call check-order 127u8 127i8 0)
  (call check-order 0u8 0u8 0)
  (call check-order 255u8 255u8 0)
  (call check-order 0u8 255u8 -1)
  (call check-order 255u8 0u8 1)
  (call check-order 1u8 1u8 0)
  (call check-order 0u8 -32768i16 1)
  (call check-order 255u8 32767i16 -1)
  (call check-order 0u8 32767i16 -1)
  (call check-order 255u8 -32768i16 1)
  (call check-order 0u8 0i16 0)
  (call check-order 1u8 1i16 0)
  (call check-order 255u8 255i16 0)
  (call check-order 0u8 0u16 0)
  (call check-order 255u8 65535u16 -1)
  (call check-order 0u8 65535u16 -1)
  (call check-order 255u8 0u16 1)
  (call check-order 1u8 1u16 0)
  (call check-order 255u8 255u16 0)
  (call check-order 0u8 -2147483648i32 1)
  (call check-order 255u8 2147483647i32 -1)
  (call check-order 0u8 2147483647i32 -1)
  (call check-order 255u8 -2147483648i32 1)
  (call check-order 0u8 0i32 0)
  (call check-order 1u8 1i32 0)
  (call check-order 255u8 255i32 0)
  (call check-order 0u8 0u32 0)
  (call check-order 255u8 4294967295u32 -1)
  (call check-order 0u8 4294967295u32 -1)
  (call check-order 255u8 0u32 1)
  (call check-order 1u8 1u32 0)
  (call check-order 255u8 255u32 0)
  (call check-order 0u8 -9223372036854775808i64 1)
  (call check-order 255u8 9223372036854775807i64 -1)
  (call check-order 0u8 9223372036854775807i64 -1)
  (call check-order 255u8 -9223372036854775808i64 1)
  (call check-order 0u8 0i64 0)
  (call check-order 1u8 1i64 0)
  (call check-order 255u8 255i64 0)
  (call check-order 0u8 0u64 0)
  (call check-order 255u8 18446744073709551615u64 -1)
  (call check-order 0u8 18446744073709551615u64 -1)
  (call check-order 255u8 0u64 1)
  (call check-order 1u8 1u64 0)
  (call check-order 255u8 255u64 0)
  (call check-order -32768i16 -128i8 -1)
  (call check-order 32767i16 127i8 1)
  (call check-order -32768i16 127i8 -1)
  (call check-order 32767i16 -128i8 1)
  (call check-order 0i16 0i8 0)
  (call check-order 1i16 1i8 0)
  (call check-order 127i16 127i8 0)
  (call check-order -32768i16 0u8 -1)
  (call check-order 32767i16 255u8 1)
  (call check-order -32768i16 255u8 -1)
  (call check-order 32767i16 0u8 1)
  (call check-order 0i16 0u8 0)
  (call check-order 1i16 1u8 0)
  (call check-order 255i16 255u8 0)
  (call check-order -32768i16 -32768i16 0)
  (call check-order 32767i16 32767i16 0)
  (call check-order -32768i16 32767i16 -1)
  (call check-order 32767i16 -32768i16 1)
  (call check-order 0i16 0i16 0)
  (call check-order 1i16 1i16 0)
  (call check-order -32768i16 0u16 -1)
  (call check-order 32767i16 65535u16 -1)
  (call check-order -32768i16 65535u16 -1)
  (call check-order 32767i16 0u16 1)
  (call check-order 0i16 0u16 0)
  (call check-order 1i16 1u16 0)
  (call check-order 32767i16 32767u16 0)
  (call check-order -32768i16 -2147483648i32 1)
  (call check-order 32767i16 2147483647i32 -1)
  (call check-order -32768i16 2147483647i32 -1)
  (call check-order 32767i16 -2147483648i32 1)
  (call check-order 0i16 0i32 0)
  (call check-order 1i16 1i32 0)
  (call check-order 32767i16 32767i32 0)
  (call check-order -32768i16 0u32 -1)
  (call check-order 32767i16 4294967295u32 -1)
  (call check-order -32768i16 4294967295u32 -1)
  (call check-order 32767i16 0u32 1)
  (call check-order 0i16 0u32 0)
  (call check-order 1i16 1u32 0)
  (call check-order 32767i16 32767u32 0)
  (call check-order -32768i16 -9223372036854775808i64 1)
  (call check-order 32767i16 9223372036854775807i64 -1)
  (call check-order -32768i16 9223372036854775807i64 -1)
  (call check-order 32767i16 -9223372036854775808i64 1)
  (call check-order 0i16 0i64 0)
  (call check-order 1i16 1i64 0)
  (call check-order 32767i16 32767i64 0)
  (call check-order -32768i16 0u64 -1)
  (call check-order 32767i16 18446744073709551615u64 -1)
  (call check-order -32768i16 18446744073709551615u64 -1)
  (call check-order 32767i16 0u64 1)
  (call check-order 0i16 0u64 0)
  (call check-order 1i16 1u64 0)
  (call check-order 32767i16 32767u64 0)
  (call check-order 0u16 -128i8 1)
  (call check-order 65535u16 127i8 1)
  (call check-order 0u16 127i8 -1)
  (call check-order 65535u16 -128i8 1)
  (call check-order 0u16 0i8 0)
  (call check-order 1u16 1i8 0)
  (call check-order 127u16 127i8 0)
  (call check-order 0u16 0u8 0)
  (call check-order 65535u16 255u8 1)
  (call check-order 0u16 255u8 -1)
  (call check-order 65535u16 0u8 1)
  (call check-order 1u16 1u8 0)
  (call check-order 255u16 255u8 0)
  (call check-order 0u16 -32768i16 1)
  (call check-order 65535u16 32767i16 1)
  (call check-order 0u16 32767i16 -1)
  (call check-order 65535u16 -32768i16 1)
  (call check-order 0u16 0i16 0)
  (call check-order 1u16 1i16 0)
  (call check-order 32767u16 32767i16 0)
  (call check-order 0u16 0u16 0)
  (call check-order 65535u16 65535u16 0)
  (call check-order 0u16 65535u16 -1)
  (call check-order 65535u16 0u16 1)
  (call check-order 1u16 1u16 0)
  (call check-order 0u16 -2147483648i32 1)
  (call check-order 65535u16 2147483647i32 -1)
  (call check-order 0u16 2147483647i32 -1)
  (call check-order 65535u16 -2147483648i32 1)
  (call check-order 0u16 0i32 0)
  (call check-order 1u16 1i32 0)
  (call check-order 65535u16 65535i32 0)
  (call check-order 0u16 0u32 0)
  (call check-order 65535u16 4294967295u32 -1)
  (call check-order 0u16 4294967295u32 -1)
  (call check-order 65535u16 0u32 1)
  (call check-order 1u16 1u32 0)
  (call check-order 65535u16 65535u32 0)
  (call check-order 0u16 -9223372036854775808i64 1)
  (call check-order 65535u16 9223372036854775807i64 -1)
  (call check-order 0u16 9223372036854775807i64 -1)
  (call check-order 65535u16 -9223372036854775808i64 1)
  (call check-order 0u16 0i64 0)
  (call check-order 1u16 1i64 0)
  (call check-order 65535u16 65535i64 0)
  (call check-order 0u16 0u64 0)
  (call check-order 65535u16 18446744073709551615u64 -1)
  (call check-order 0u16 18446744073709551615u64 -1)
  (call check-order 65535u16 0u64 1)
  (call check-order 1u16 1u64 0)
  (call check-order 65535u16 65535u64 0)
  (call check-order -2147483648i32 -128i8 -1)
  (call check-order 2147483647i32 127i8 1)
  (call check-order -2147483648i32 127i8 -1)
  (call check-order 2147483647i32 -128i8 1)
  (call check-order 0i32 0i8 0)
  (call check-order 1i32 1i8 0)
  (call check-order 127i32 127i8 0)
  (call check-order -2147483648i32 0u8 -1)
  (call check-order 2147483647i32 255u8 1)
  (call check-order -2147483648i32 255u8 -1)
  (call check-order 2147483647i32 0u8 1)
  (call check-order 0i32 0u8 0)
  (call check-order 1i32 1u8 0)
  (call check-order 255i32 255u8 0)
  (call check-order -2147483648i32 -32768i16 -1)
  (call check-order 2147483647i32 32767i16 1)
  (call check-order -2147483648i32 32767i16 -1)
  (call check-order 2147483647i32 -32768i16 1)
  (call check-order 0i32 0i16 0)
  (call check-order 1i32 1i16 0)
  (call check-order 32767i32 32767i16 0)
  (call check-order -2147483648i32 0u16 -1)
  (call check-order 2147483647i32 65535u16 1)
  (call check-order -2147483648i32 65535u16 -1)
  (call check-order 2147483647i32 0u16 1)
  (call check-order 0i32 0u16 0)
  (call check-order 1i32 1u16 0)
  (call check-order 65535i32 65535u16 0)
  (call check-order -2147483648i32 -2147483648i32 0)
  (call check-order 2147483647i32 2147483647i32 0)
  (call check-order -2147483648i32 2147483647i32 -1)
  (call check-order 2147483647i32 -2147483648i32 1)
  (call check-order 0i32 0i32 0)
  (call check-order 1i32 1i32 0)
  (call check-order -2147483648i32 0u32 -1)
  (call check-order 2147483647i32 4294967295u32 -1)
  (call check-order -2147483648i32 4294967295u32 -1)
  (call check-order 2147483647i32 0u32 1)
  (call check-order 0i32 0u32 0)
  (call check-order 1i32 1u32 0)
  (call check-order 2147483647i32 2147483647u32 0)
  (call check-order -2147483648i32 -9223372036854775808i64 1)
  (call check-order 2147483647i32 9223372036854775807i64 -1)
  (call check-order -2147483648i32 9223372036854775807i64 -1)
  (call check-order 2147483647i32 -9223372036854775808i64 1)
  (call check-order 0i32 0i64 0)
  (call check-order 1i32 1i64 0)
  (call check-order 2147483647i32 2147483647i64 0)
  (call check-order -2147483648i32 0u64 -1)
  (call check-order 2147483647i32 18446744073709551615u64 -1)
  (call check-order -2147483648i32 18446744073709551615u64 -1)
  (call check-order 2147483647i32 0u64 1)
  (call check-order 0i32 0u64 0)
  (call check-order 1i32 1u64 0)
  (call check-order 2147483647i32 2147483647u64 0)
  (call check-order 0u32 -128i8 1)
  (call check-order 4294967295u32 127i8 1)
  (call check-order 0u32 127i8 -1)
  (call check-order 4294967295u32 -128i8 1)
  (call check-order 0u32 0i8 0)
  (call check-order 1u32 1i8 0)
  (call check-order 127u32 127i8 0)
  (call check-order 0u32 0u8 0)
  (call check-order 4294967295u32 255u8 1)
  (call check-order 0u32 255u8 -1)
  (call check-order 4294967295u32 0u8 1)
  (call check-order 1u32 1u8 0)
  (call check-order 255u32 255u8 0)
  (call check-order 0u32 -32768i16 1)
  (call check-order 4294967295u32 32767i16 1)
  (call check-order 0u32 32767i16 -1)
  (call check-order 4294967295u32 -32768i16 1)
  (call check-order 0u32 0i16 0)
  (call check-order 1u32 1i16 0)
  (call check-order 32767u32 32767i16 0)
  (call check-order 0u32 0u16 0)
  (call check-order 4294967295u32 65535u16 1)
  (call check-order 0u32 65535u16 -1)
  (call check-order 4294967295u32 0u16 1)
  (call check-order 1u32 1u16 0)
  (call check-order 65535u32 65535u16 0)
  (call check-order 0u32 -2147483648i32 1)
  (call check-order 4294967295u32 2147483647i32 1)
  (call check-order 0u32 2147483647i32 -1)
  (call check-order 4294967295u32 -2147483648i32 1)
  (call check-order 0u32 0i32 0)
  (call check-order 1u32 1i32 0)
  (call check-order 2147483647u32 2147483647i32 0)
  (call check-order 0u32 0u32 0)
  (call check-order 4294967295u32 4294967295u32 0)
  (call check-order 0u32 4294967295u32 -1)
  (call check-order 4294967295u32 0u32 1)
  (call check-order 1u32 1u32 0)
  (call check-order 0u32 -9223372036854775808i64 1)
  (call check-order 4294967295u32 9223372036854775807i64 -1)
  (call check-order 0u32 9223372036854775807i64 -1)
  (call check-order 4294967295u32 -9223372036854775808i64 1)
  (call check-order 0u32 0i64 0)
  (call check-order 1u32 1i64 0)
  (call check-order 4294967295u32 4294967295i64 0)
  (call check-order 0u32 0u64 0)
  (call check-order 4294967295u32 18446744073709551615u64 -1)
  (call check-order 0u32 18446744073709551615u64 -1)
  (call check-order 4294967295u32 0u64 1)
  (call check-order 1u32 1u64 0)
  (call check-order 4294967295u32 4294967295u64 0)
  (call check-order -9223372036854775808i64 -128i8 -1)
  (call check-order 9223372036854775807i64 127i8 1)
  (call check-order -9223372036854775808i64 127i8 -1)
  (call check-order 9223372036854775807i64 -128i8 1)
  (call check-order 0i64 0i8 0)
  (call check-order 1i64 1i8 0)
  (call check-order 127i64 127i8 0)
  (call check-order -9223372036854775808i64 0u8 -1)
  (call check-order 9223372036854775807i64 255u8 1)
  (call check-order -9223372036854775808i64 255u8 -1)
  (call check-order 9223372036854775807i64 0u8 1)
  (call check-order 0i64 0u8 0)
  (call check-order 1i64 1u8 0)
  (call check-order 255i64 255u8 0)
  (call check-order -9223372036854775808i64 -32768i16 -1)
  (call check-order 9223372036854775807i64 32767i16 1)
  (call check-order -9223372036854775808i64 32767i16 -1)
  (call check-order 9223372036854775807i64 -32768i16 1)
  (call check-order 0i64 0i16 0)
  (call check-order 1i64 1i16 0)
  (call check-order 32767i64 32767i16 0)
  (call check-order -9223372036854775808i64 0u16 -1)
  (call check-order 9223372036854775807i64 65535u16 1)
  (call check-order -9223372036854775808i64 65535u16 -1)
  (call check-order 9223372036854775807i64 0u16 1)
  (call check-order 0i64 0u16 0)
  (call check-order 1i64 1u16 0)
  (call check-order 65535i64 65535u16 0)
  (call check-order -9223372036854775808i64 -2147483648i32 -1)
  (call check-order 9223372036854775807i64 2147483647i32 1)
  (call check-order -9223372036854775808i64 2147483647i32 -1)
  (call check-order 9223372036854775807i64 -2147483648i32 1)
  (call check-order 0i64 0i32 0)
  (call check-order 1i64 1i32 0)
  (call check-order 2147483647i64 2147483647i32 0)
  (call check-order -9223372036854775808i64 0u32 -1)
  (call check-order 9223372036854775807i64 4294967295u32 1)
  (call check-order -9223372036854775808i64 4294967295u32 -1)
  (call check-order 9223372036854775807i64 0u32 1)
  (call check-order 0i64 0u32 0)
  (call check-order 1i64 1u32 0)
  (call check-order 4294967295i64 4294967295u32 0)
  (call check-order -9223372036854775808i64 -9223372036854775808i64 0)
  (call check-order 9223372036854775807i64 9223372036854775807i64 0)
  (call check-order -9223372036854775808i64 9223372036854775807i64 -1)
  (call check-order 9223372036854775807i64 -9223372036854775808i64 1)
  (call check-order 0i64 0i64 0)
  (call check-order 1i64 1i64 0)
  (call check-order -9223372036854775808i64 0u64 -1)
  (call check-order 9223372036854775807i64 18446744073709551615u64 -1)
  (call check-order -9223372036854775808i64 18446744073709551615u64 -1)
  (call check-order 9223372036854775807i64 0u64 1)
  (call check-order 0i64 0u64 0)
  (call check-order 1i64 1u64 0)
  (call check-order 9223372036854775807i64 9223372036854775807u64 0)
  (call check-order 0u64 -128i8 1)
  (call check-order 18446744073709551615u64 127i8 1)
  (call check-order 0u64 127i8 -1)
  (call check-order 18446744073709551615u64 -128i8 1)
  (call check-order 0u64 0i8 0)
  (call check-order 1u64 1i8 0)
  (call check-order 127u64 127i8 0)
  (call check-order 0u64 0u8 0)
  (call check-order 18446744073709551615u64 255u8 1)
  (call check-order 0u64 255u8 -1)
  (call check-order 18446744073709551615u64 0u8 1)
  (call check-order 1u64 1u8 0)
  (call check-order 255u64 255u8 0)
  (call check-order 0u64 -32768i16 1)
  (call check-order 18446744073709551615u64 32767i16 1)
  (call check-order 0u64 32767i16 -1)
  (call check-order 18446744073709551615u64 -32768i16 1)
  (call check-order 0u64 0i16 0)
  (call check-order 1u64 1i16 0)
  (call check-order 32767u64 32767i16 0)
  (call check-order 0u64 0u16 0)
  (call check-order 18446744073709551615u64 65535u16 1)
  (call check-order 0u64 65535u16 -1)
  (call check-order 18446744073709551615u64 0u16 1)
  (call check-order 1u64 1u16 0)
  (call check-order 65535u64 65535u16 0)
  (call check-order 0u64 -2147483648i32 1)
  (call check-order 18446744073709551615u64 2147483647i32 1)
  (call check-order 0u64 2147483647i32 -1)
  (call check-order 18446744073709551615u64 -2147483648i32 1)
  (call check-order 0u64 0i32 0)
  (call check-order 1u64 1i32 0)
  (call check-order 2147483647u64 2147483647i32 0)
  (call check-order 0u64 0u32 0)
  (call check-order 18446744073709551615u64 4294967295u32 1)
  (call check-order 0u64 4294967295u32 -1)
  (call check-order 18446744073709551615u64 0u32 1)
  (call check-order 1u64 1u32 0)
  (call check-order 4294967295u64 4294967295u32 0)
  (call check-order 0u64 -9223372036854775808i64 1)
  (call check-order 18446744073709551615u64 9223372036854775807i64 1)
  (call check-order 0u64 9223372036854775807i64 -1)
  (call check-order 18446744073709551615u64 -9223372036854775808i64 1)
  (call check-order 0u64 0i64 0)
  (call check-order 1u64 1i64 0)
  (call check-order 9223372036854775807u64 9223372036854775807i64 0)
  (call check-order 0u64 0u64 0)
  (call check-order 18446744073709551615u64 18446744073709551615u64 0)
  (call check-order 0u64 18446744073709551615u64 -1)
  (call check-order 18446744073709551615u64 0u64 1)
  (call check-order 1u64 1u64 0)
  (call check-order 127i8 128u8 -1)
  (call check-order 255u8 256i16 -1)
  (call check-order -128i8 -129i16 1)
  (call check-order 32767i16 32768u16 -1)
  (call check-order 65535u16 65536i32 -1)
  (call check-order -32768i16 -32769i32 1)
  (call check-order 2147483647i32 2147483648u32 -1)
  (call check-order 4294967295u32 4294967296i64 -1)
  (call check-order -2147483648i32 -2147483649i64 1)
  (call check-order 9223372036854775807i64 9223372036854775808u64 -1)
  (print "integer comparison boundaries passed")
)

(fn-do + (of check-order L R) _
  (param left L)
  (param right R)
  (param order I32)
  (let valid (and (== (== left right) (== order 0)) (== (== right left) (== order 0))))
  (= valid (and valid (== (!= left right) (!= order 0)) (== (!= right left) (!= order 0))))
  (= valid (and valid (== (< left right) (< order 0)) (== (> right left) (< order 0))))
  (= valid (and valid (== (<= left right) (<= order 0)) (== (>= right left) (<= order 0))))
  (= valid (and valid (== (> left right) (> order 0)) (== (< right left) (> order 0))))
  (= valid (and valid (== (>= left right) (>= order 0)) (== (<= right left) (>= order 0))))
  (if (not valid)
    (print (str "comparison failed: " left " " right " " order))
  )
)
```

```output
integer comparison boundaries passed
```

## Arithmetic operators share common types for literals and typed parameters

```brix
(main
  (call arithmetic 127i8 256i16)
  (print (str (+ 127i8 256i16) " " (. (type-of (+ 127i8 256i16)) name)))
  (print (str (- 127i8 256i16) " " (. (type-of (- 127i8 256i16)) name)))
  (print (str (* 127i8 256i16) " " (. (type-of (* 127i8 256i16)) name)))
  (print (str (/ 127i8 256i16) " " (. (type-of (/ 127i8 256i16)) name)))
  (print (str (% 127i8 256i16) " " (. (type-of (% 127i8 256i16)) name)))
  (call arithmetic 256i16 127i8)
  (print (str (+ 256i16 127i8) " " (. (type-of (+ 256i16 127i8)) name)))
  (print (str (- 256i16 127i8) " " (. (type-of (- 256i16 127i8)) name)))
  (print (str (* 256i16 127i8) " " (. (type-of (* 256i16 127i8)) name)))
  (print (str (/ 256i16 127i8) " " (. (type-of (/ 256i16 127i8)) name)))
  (print (str (% 256i16 127i8) " " (. (type-of (% 256i16 127i8)) name)))
  (call arithmetic 255u8 65535u16)
  (print (str (+ 255u8 65535u16) " " (. (type-of (+ 255u8 65535u16)) name)))
  (print (str (- 255u8 65535u16) " " (. (type-of (- 255u8 65535u16)) name)))
  (print (str (* 255u8 65535u16) " " (. (type-of (* 255u8 65535u16)) name)))
  (print (str (/ 255u8 65535u16) " " (. (type-of (/ 255u8 65535u16)) name)))
  (print (str (% 255u8 65535u16) " " (. (type-of (% 255u8 65535u16)) name)))
  (call arithmetic 65535u16 255u8)
  (print (str (+ 65535u16 255u8) " " (. (type-of (+ 65535u16 255u8)) name)))
  (print (str (- 65535u16 255u8) " " (. (type-of (- 65535u16 255u8)) name)))
  (print (str (* 65535u16 255u8) " " (. (type-of (* 65535u16 255u8)) name)))
  (print (str (/ 65535u16 255u8) " " (. (type-of (/ 65535u16 255u8)) name)))
  (print (str (% 65535u16 255u8) " " (. (type-of (% 65535u16 255u8)) name)))
  (call arithmetic 32767i16 65536i32)
  (print (str (+ 32767i16 65536i32) " " (. (type-of (+ 32767i16 65536i32)) name)))
  (print (str (- 32767i16 65536i32) " " (. (type-of (- 32767i16 65536i32)) name)))
  (print (str (* 32767i16 65536i32) " " (. (type-of (* 32767i16 65536i32)) name)))
  (print (str (/ 32767i16 65536i32) " " (. (type-of (/ 32767i16 65536i32)) name)))
  (print (str (% 32767i16 65536i32) " " (. (type-of (% 32767i16 65536i32)) name)))
  (call arithmetic 65536i32 32767i16)
  (print (str (+ 65536i32 32767i16) " " (. (type-of (+ 65536i32 32767i16)) name)))
  (print (str (- 65536i32 32767i16) " " (. (type-of (- 65536i32 32767i16)) name)))
  (print (str (* 65536i32 32767i16) " " (. (type-of (* 65536i32 32767i16)) name)))
  (print (str (/ 65536i32 32767i16) " " (. (type-of (/ 65536i32 32767i16)) name)))
  (print (str (% 65536i32 32767i16) " " (. (type-of (% 65536i32 32767i16)) name)))
  (call arithmetic 65535u16 2147483648u32)
  (print (str (+ 65535u16 2147483648u32) " " (. (type-of (+ 65535u16 2147483648u32)) name)))
  (print (str (- 65535u16 2147483648u32) " " (. (type-of (- 65535u16 2147483648u32)) name)))
  (print (str (* 65535u16 2147483648u32) " " (. (type-of (* 65535u16 2147483648u32)) name)))
  (print (str (/ 65535u16 2147483648u32) " " (. (type-of (/ 65535u16 2147483648u32)) name)))
  (print (str (% 65535u16 2147483648u32) " " (. (type-of (% 65535u16 2147483648u32)) name)))
  (call arithmetic 2147483648u32 65535u16)
  (print (str (+ 2147483648u32 65535u16) " " (. (type-of (+ 2147483648u32 65535u16)) name)))
  (print (str (- 2147483648u32 65535u16) " " (. (type-of (- 2147483648u32 65535u16)) name)))
  (print (str (* 2147483648u32 65535u16) " " (. (type-of (* 2147483648u32 65535u16)) name)))
  (print (str (/ 2147483648u32 65535u16) " " (. (type-of (/ 2147483648u32 65535u16)) name)))
  (print (str (% 2147483648u32 65535u16) " " (. (type-of (% 2147483648u32 65535u16)) name)))
  (call arithmetic 1i32 4294967296i64)
  (print (str (+ 1i32 4294967296i64) " " (. (type-of (+ 1i32 4294967296i64)) name)))
  (print (str (- 1i32 4294967296i64) " " (. (type-of (- 1i32 4294967296i64)) name)))
  (print (str (* 1i32 4294967296i64) " " (. (type-of (* 1i32 4294967296i64)) name)))
  (print (str (/ 1i32 4294967296i64) " " (. (type-of (/ 1i32 4294967296i64)) name)))
  (print (str (% 1i32 4294967296i64) " " (. (type-of (% 1i32 4294967296i64)) name)))
  (call arithmetic 4294967296i64 1i32)
  (print (str (+ 4294967296i64 1i32) " " (. (type-of (+ 4294967296i64 1i32)) name)))
  (print (str (- 4294967296i64 1i32) " " (. (type-of (- 4294967296i64 1i32)) name)))
  (print (str (* 4294967296i64 1i32) " " (. (type-of (* 4294967296i64 1i32)) name)))
  (print (str (/ 4294967296i64 1i32) " " (. (type-of (/ 4294967296i64 1i32)) name)))
  (print (str (% 4294967296i64 1i32) " " (. (type-of (% 4294967296i64 1i32)) name)))
  (call arithmetic 1u32 4294967296u64)
  (print (str (+ 1u32 4294967296u64) " " (. (type-of (+ 1u32 4294967296u64)) name)))
  (print (str (- 1u32 4294967296u64) " " (. (type-of (- 1u32 4294967296u64)) name)))
  (print (str (* 1u32 4294967296u64) " " (. (type-of (* 1u32 4294967296u64)) name)))
  (print (str (/ 1u32 4294967296u64) " " (. (type-of (/ 1u32 4294967296u64)) name)))
  (print (str (% 1u32 4294967296u64) " " (. (type-of (% 1u32 4294967296u64)) name)))
  (call arithmetic 4294967296u64 1u32)
  (print (str (+ 4294967296u64 1u32) " " (. (type-of (+ 4294967296u64 1u32)) name)))
  (print (str (- 4294967296u64 1u32) " " (. (type-of (- 4294967296u64 1u32)) name)))
  (print (str (* 4294967296u64 1u32) " " (. (type-of (* 4294967296u64 1u32)) name)))
  (print (str (/ 4294967296u64 1u32) " " (. (type-of (/ 4294967296u64 1u32)) name)))
  (print (str (% 4294967296u64 1u32) " " (. (type-of (% 4294967296u64 1u32)) name)))
  (call arithmetic -1i32 2147483648u32)
  (print (str (+ -1i32 2147483648u32) " " (. (type-of (+ -1i32 2147483648u32)) name)))
  (print (str (- -1i32 2147483648u32) " " (. (type-of (- -1i32 2147483648u32)) name)))
  (print (str (* -1i32 2147483648u32) " " (. (type-of (* -1i32 2147483648u32)) name)))
  (print (str (/ -1i32 2147483648u32) " " (. (type-of (/ -1i32 2147483648u32)) name)))
  (print (str (% -1i32 2147483648u32) " " (. (type-of (% -1i32 2147483648u32)) name)))
  (call arithmetic 2147483648u32 -1i32)
  (print (str (+ 2147483648u32 -1i32) " " (. (type-of (+ 2147483648u32 -1i32)) name)))
  (print (str (- 2147483648u32 -1i32) " " (. (type-of (- 2147483648u32 -1i32)) name)))
  (print (str (* 2147483648u32 -1i32) " " (. (type-of (* 2147483648u32 -1i32)) name)))
  (print (str (/ 2147483648u32 -1i32) " " (. (type-of (/ 2147483648u32 -1i32)) name)))
  (print (str (% 2147483648u32 -1i32) " " (. (type-of (% 2147483648u32 -1i32)) name)))
  (call arithmetic 4294967295u32 4294967296i64)
  (print (str (+ 4294967295u32 4294967296i64) " " (. (type-of (+ 4294967295u32 4294967296i64)) name)))
  (print (str (- 4294967295u32 4294967296i64) " " (. (type-of (- 4294967295u32 4294967296i64)) name)))
  (print (str (* 4294967295u32 4294967296i64) " " (. (type-of (* 4294967295u32 4294967296i64)) name)))
  (print (str (/ 4294967295u32 4294967296i64) " " (. (type-of (/ 4294967295u32 4294967296i64)) name)))
  (print (str (% 4294967295u32 4294967296i64) " " (. (type-of (% 4294967295u32 4294967296i64)) name)))
  (call arithmetic 4294967296i64 4294967295u32)
  (print (str (+ 4294967296i64 4294967295u32) " " (. (type-of (+ 4294967296i64 4294967295u32)) name)))
  (print (str (- 4294967296i64 4294967295u32) " " (. (type-of (- 4294967296i64 4294967295u32)) name)))
  (print (str (* 4294967296i64 4294967295u32) " " (. (type-of (* 4294967296i64 4294967295u32)) name)))
  (print (str (/ 4294967296i64 4294967295u32) " " (. (type-of (/ 4294967296i64 4294967295u32)) name)))
  (print (str (% 4294967296i64 4294967295u32) " " (. (type-of (% 4294967296i64 4294967295u32)) name)))
  (call arithmetic -1i64 9223372036854775808u64)
  (print (str (+ -1i64 9223372036854775808u64) " " (. (type-of (+ -1i64 9223372036854775808u64)) name)))
  (print (str (- -1i64 9223372036854775808u64) " " (. (type-of (- -1i64 9223372036854775808u64)) name)))
  (print (str (* -1i64 9223372036854775808u64) " " (. (type-of (* -1i64 9223372036854775808u64)) name)))
  (print (str (/ -1i64 9223372036854775808u64) " " (. (type-of (/ -1i64 9223372036854775808u64)) name)))
  (print (str (% -1i64 9223372036854775808u64) " " (. (type-of (% -1i64 9223372036854775808u64)) name)))
  (call arithmetic 9223372036854775808u64 -1i64)
  (print (str (+ 9223372036854775808u64 -1i64) " " (. (type-of (+ 9223372036854775808u64 -1i64)) name)))
  (print (str (- 9223372036854775808u64 -1i64) " " (. (type-of (- 9223372036854775808u64 -1i64)) name)))
  (print (str (* 9223372036854775808u64 -1i64) " " (. (type-of (* 9223372036854775808u64 -1i64)) name)))
  (print (str (/ 9223372036854775808u64 -1i64) " " (. (type-of (/ 9223372036854775808u64 -1i64)) name)))
  (print (str (% 9223372036854775808u64 -1i64) " " (. (type-of (% 9223372036854775808u64 -1i64)) name)))
  (call arithmetic 127i8 127i8)
  (print (str (+ 127i8 127i8) " " (. (type-of (+ 127i8 127i8)) name)))
  (print (str (- 127i8 127i8) " " (. (type-of (- 127i8 127i8)) name)))
  (print (str (* 127i8 127i8) " " (. (type-of (* 127i8 127i8)) name)))
  (print (str (/ 127i8 127i8) " " (. (type-of (/ 127i8 127i8)) name)))
  (print (str (% 127i8 127i8) " " (. (type-of (% 127i8 127i8)) name)))
  (call arithmetic 127i8 127i8)
  (print (str (+ 127i8 127i8) " " (. (type-of (+ 127i8 127i8)) name)))
  (print (str (- 127i8 127i8) " " (. (type-of (- 127i8 127i8)) name)))
  (print (str (* 127i8 127i8) " " (. (type-of (* 127i8 127i8)) name)))
  (print (str (/ 127i8 127i8) " " (. (type-of (/ 127i8 127i8)) name)))
  (print (str (% 127i8 127i8) " " (. (type-of (% 127i8 127i8)) name)))
)

(fn-do + (of arithmetic L R) _
  (param left L)
  (param right R)
  (print (str (+ left right) " " (. (type-of (+ left right)) name)))
  (print (str (- left right) " " (. (type-of (- left right)) name)))
  (print (str (* left right) " " (. (type-of (* left right)) name)))
  (print (str (/ left right) " " (. (type-of (/ left right)) name)))
  (print (str (% left right) " " (. (type-of (% left right)) name)))
)
```

```output
383 I16
-129 I16
32512 I16
0 I16
127 I16
383 I16
-129 I16
32512 I16
0 I16
127 I16
383 I16
129 I16
32512 I16
2 I16
2 I16
383 I16
129 I16
32512 I16
2 I16
2 I16
254 U16
256 U16
65281 U16
0 U16
255 U16
254 U16
256 U16
65281 U16
0 U16
255 U16
254 U16
65280 U16
65281 U16
257 U16
0 U16
254 U16
65280 U16
65281 U16
257 U16
0 U16
98303 I32
-32769 I32
2147418112 I32
0 I32
32767 I32
98303 I32
-32769 I32
2147418112 I32
0 I32
32767 I32
98303 I32
32769 I32
2147418112 I32
2 I32
2 I32
98303 I32
32769 I32
2147418112 I32
2 I32
2 I32
2147549183 U32
2147549183 U32
2147483648 U32
0 U32
65535 U32
2147549183 U32
2147549183 U32
2147483648 U32
0 U32
65535 U32
2147549183 U32
2147418113 U32
2147483648 U32
32768 U32
32768 U32
2147549183 U32
2147418113 U32
2147483648 U32
32768 U32
32768 U32
4294967297 I64
-4294967295 I64
4294967296 I64
0 I64
1 I64
4294967297 I64
-4294967295 I64
4294967296 I64
0 I64
1 I64
4294967297 I64
4294967295 I64
4294967296 I64
4294967296 I64
0 I64
4294967297 I64
4294967295 I64
4294967296 I64
4294967296 I64
0 I64
4294967297 U64
18446744069414584321 U64
4294967296 U64
0 U64
1 U64
4294967297 U64
18446744069414584321 U64
4294967296 U64
0 U64
1 U64
4294967297 U64
4294967295 U64
4294967296 U64
4294967296 U64
0 U64
4294967297 U64
4294967295 U64
4294967296 U64
4294967296 U64
0 U64
2147483647 U32
2147483647 U32
2147483648 U32
1 U32
2147483647 U32
2147483647 U32
2147483647 U32
2147483648 U32
1 U32
2147483647 U32
2147483647 U32
2147483649 U32
2147483648 U32
0 U32
2147483648 U32
2147483647 U32
2147483649 U32
2147483648 U32
0 U32
2147483648 U32
8589934591 I64
-1 I64
-4294967296 I64
0 I64
4294967295 I64
8589934591 I64
-1 I64
-4294967296 I64
0 I64
4294967295 I64
8589934591 I64
1 I64
-4294967296 I64
1 I64
1 I64
8589934591 I64
1 I64
-4294967296 I64
1 I64
1 I64
9223372036854775807 U64
9223372036854775807 U64
9223372036854775808 U64
1 U64
9223372036854775807 U64
9223372036854775807 U64
9223372036854775807 U64
9223372036854775808 U64
1 U64
9223372036854775807 U64
9223372036854775807 U64
9223372036854775809 U64
9223372036854775808 U64
0 U64
9223372036854775808 U64
9223372036854775807 U64
9223372036854775809 U64
9223372036854775808 U64
0 U64
9223372036854775808 U64
-2 I8
0 I8
1 I8
1 I8
0 I8
-2 I8
0 I8
1 I8
1 I8
0 I8
-2 I8
0 I8
1 I8
1 I8
0 I8
-2 I8
0 I8
1 I8
1 I8
0 I8
```

### F32 + F64 infers F64, so a global typed by inference before its initializer compiles keeps F64 precision.

```brix
(main
  (print (call describe total))
  (print (- total 3.6))
)

(blank)
(prop + early _ (= (call total-or-zero false)))
(prop + small F32 (= 1.5))
(prop + large F64 (= 2.1))
(prop + total _ (= (+ small large)))

(fn-do + total-or-zero F64
  (param wanted Bool)
  (if wanted
    (return total)
  )
  (return 0.0)
)

(fn-expr + describe String
  (param x F32)
  (str "F32 " x)
)

(fn-expr + describe String
  (param x F64)
  (str "F64 " x)
)
```

```output
F64 3.6
0
```

### C pointers convert to narrower integers

```brix
(main
  (let pointer (as (as 300 Pointer) (of CPointer U8)))
  (let signed I32 pointer)
  (let unsigned U32 pointer)
  (let short U16 pointer)
  (let byte U8 pointer)
  (let wide U64 pointer)
  (print signed)
  (print unsigned)
  (print short)
  (print byte)
  (print wide)
)
```

```output
300
300
300
44
300
```

### A signed number compares with an unsigned one by its sign first, so a negative literal need not fit.

```brix
(main
  (print (< -1 4000000000u32))
  (print (== -1 4294967295u32))
  (print (!= -1 4294967295u32))
  (print (< -1 18446744073709551615))
  (print (> 4000000000u32 -1))
  (print (<= -1 0u8))
  (print (>= 0u64 -5))
)
```

```output
true
false
true
true
true
true
true
```

### Arithmetic on a character is unsigned and 32 bits wide, whatever other types the program names.

```brix
(main
  (print (as 'a' UnitString))
  (print (as (+ 'a' 1) UnitString))
  (print (as (+ 1 'a') UnitString))
  (print (as (+ 'a' 'b') UnitString))
  (print (as (+ 'a' 1i64) UnitString))
  (print (as (- 'a' 1) UnitString))
)
```

```output
ac
98u32
98u32
195u32
98i
96u32
```

### A minus sign before a negative number makes it positive again.

```brix
(main
  (print (- -5))
  (print -5)
  (print (- -1.5))
  (print (- (- -7)))
  (print (- 3 -2))
)
```

```output
5
-5
1.5
-7
5
```

### A number written wider than its type compares as the value its type holds.

```brix
(main
  (print (== 128i8 -128i8))
  (print (!= 128i8 -128i8))
  (print (< 128i8 0i8))
  (print (< 128i8 0))
  (print (== 300u8 44u8))
  (print (> 300u8 100u8))
  (print (== 65536u16 0u16))
  (print (== 3i1 1i1))
)
```

```output
true
false
true
true
true
false
true
true
```

### Arithmetic on numbers of different widths, signs or kinds is done in the type both promote to.

```brix
(main
  (print (as (+ 200u8 100u8) UnitString))
  (print (as (+ 127i8 1i8) UnitString))
  (print (as (+ 65535u16 1u16) UnitString))
  (print (as (+ 10000000000 1) UnitString))
  (print (as (+ 4000000000u32 1) UnitString))
  (print (as (+ 1 4000000000u32) UnitString))
  (print (as (+ 1.5 2) UnitString))
  (print (as (+ 2 1.5) UnitString))
  (print (as (+ 1.5f32 2) UnitString))
  (print (as (+ 1.5f32 1.25) UnitString))
  (print (as (+ 1.25 1.5f32) UnitString))
  (print (as (+ 10000000000 1.5f32) UnitString))
  (print (as (+ 18446744073709551615 0.0) UnitString))
  (print (+ 2147483647 1))
  (print (- -2147483648 1))
  (print (+ 4294967295u32 1u32))
  (print (+ 9223372036854775807 1))
  (print (- 0 1u32))
  (print (- 0 1u64))
  (print (+ 0.1f32 0.2f32))
  (print (+ 0.1 0.2))
)
```

```output
44u8
-128i8
0u16
10000000001i
4000000001u32
4000000001u32
3.5f
3.5f
3.5f32
2.75f32
2.75f32
1e+10f32
1.8446744073709552e+19f
2147483648
-2147483649
0
-9223372036854775808
4294967295
18446744073709551615
0.3
0.30000000000000004
```

### Division and remainder are not checked: signed ones round toward zero, and x / 0 is 0.

```brix
(main
  (print (/ 7 2))
  (print (/ -7 2))
  (print (/ 7 -2))
  (print (% 7 3))
  (print (% -7 3))
  (print (% 7 -3))
  (print (/ 7 0))
  (print (% 7 0))
  (print (/ 4000000000u32 3))
  (print (% 4000000000u32 7))
  (print (/ 18446744073709551615 3))
  (print (% 18446744073709551615 10))
  (print (/ -10000000000i64 3))
  (print (% -10000000000i64 7))
  (print (/ 200u8 3u8))
  (print (% 200u8 7u8))
  (print (/ -100i8 3i8))
  (print (/ 7.0 2))
  (print (/ 1.5 2.25))
  (print (% 7.5 2))
  (print (% -7.5 2))
  (print (% 7.5 -2))
  (print (% 7.5f32 2f32))
  (print (/ 7.0 0.0))
)
```

```output
3
-3
-3
1
-1
1
0
7
1333333333
3
6148914691236517205
5
-3333333333
-4
66
4
-33
3.5
0.6666666666666666
1.5
-1.5
1.5
1.5
inf
```

### A float literal is the nearest float, and a cast between float and integer keeps what fits.

```brix
(main
  (print 1.5)
  (print 1.5f32)
  (print 4.2e+1)
  (print 1.5e-3)
  (print 1e10)
  (print 3f32)
  (print 0.000001)
  (print -0.5)
  (print (- (+ 1.5 2)))
  (print -1.5f32)
  (print (as 3.7 I32))
  (print (as -3.7 I32))
  (print (as 3.7 U8))
  (print (as 300.7 U8))
  (print (as 3.7f32 I64))
  (print (as 1e10 I64))
  (print (as 5 F64))
  (print (as -5 F64))
  (print (as 5 F32))
  (print (as 4000000000u32 F64))
  (print (as 18446744073709551615 F64))
  (print (as -1i64 F64))
  (print (as 200u8 F64))
  (print (as -56i8 F64))
  (print (as 1.1 F32))
  (print (as 1.5f32 F64))
)
```

```output
1.5
1.5
42
0.0015
10000000000
3
1e-06
-0.5
-3.5
-1.5
3
-3
3
44
3
10000000000
5
-5
5
4000000000
1.8446744073709552e+19
-1
200
-56
1.1
1.5
```

### A number cast to Bool is whether it is not zero, and cast to a one-bit integer is its lowest bit.

```brix
(main
  (print (as 2 Bool))
  (print (as 0 Bool))
  (print (as -1 Bool))
  (print (as 256 Bool))
  (print (as 1.5 Bool))
  (print (as 0.0 Bool))
  (print (as 2 I1))
  (print (as 3 I1))
  (print 255i1)
  (print (as (as 3 I1) I32))
  (print (as (as 2 Bool) I32))
  (print (as true I32))
  (print (as false I32))
  (print (as true F64))
  (print (as (< 1 2) I32))
  (print (as 1 I1 Bool))
  (print (as 1i1 UnitString))
  (print (as true UnitString))
)
```

```output
true
false
true
true
true
false
0
1
1
1
1
1
0
1
1
true
1i1
trueb
```

### Every type of number prints as its value, with its unit when asked, and in hex when asked.

```brix
(main
  (print 5i8)
  (print 200u8)
  (print 5i16)
  (print 65535u16)
  (print 5)
  (print 4000000000u32)
  (print 10000000000)
  (print 18446744073709551615)
  (print 'A')
  (print (as 0xFF UnitString))
  (print (as 0xFFFF UnitString))
  (print (as 0xFFFFFFFF UnitString))
  (print (as 0x1FFFFFFFF UnitString))
  (print (as 5 UnitString))
  (print (as 5i64 UnitString))
  (print (as 5u8 UnitString))
  (print (as 5i8 UnitString))
  (print (as 5u16 UnitString))
  (print (as 5i16 UnitString))
  (print (as 5u32 UnitString))
  (print (as 5u64 UnitString))
  (print (as 1.5 UnitString))
  (print (as 1.5f32 UnitString))
  (print (as 'A' UnitString))
  (print (as 255u8 Hex))
  (print (as 255 Hex))
  (print (as -1 Hex))
  (print (as 65535u16 Hex))
  (print (as 1i64 Hex))
  (print (as 65 Char))
  (print (as 0x1F600 Char))
  (print (as 'A' I32))
  (print (- 'z' 'a'))
)
```

```output
5
200
5
65535
5
4000000000
10000000000
18446744073709551615
A
255u8
65535u16
4294967295u32
8589934591u
5i
5i
5u8
5i8
5u16
5i16
5u32
5u
1.5f
1.5f32
Ac
0xff
0x00000000000000ff
0xffffffffffffffff
0xffff
0x0000000000000001
A
😀
65
25
```

### The name a number of a type is written with names nothing in an expression.

```brix crash
(main
  (print i32)
)
```

```output
Exception: Name "i32" not found; did you mean "f32" or "u32"?
<no file:3>
      (print i32)
             ^^^
```

### The name a number of a type is written with does not construct one.

```brix crash
(main
  (print (call i32 5))
)
```

```output
Exception: Function "i32" not found; did you mean "f32" or "u32"?
<no file:3>
      (print (call i32 5))
             ^^^^^^^^^^^^
```

### An assigning operator works in the type its two numbers promote to, and stores in the local's type.

```brix
(main
  (let x 5)
  (+= x 2)
  (print x)
  (-= x 3)
  (print x)
  (*= x 4)
  (print x)
  (/= x 3)
  (print x)
  (%= x 3)
  (print x)
  (let f 1.5)
  (+= f 1)
  (print f)
  (/= f 2)
  (print f)
  (%= f 1)
  (print f)
  (let i 5)
  (+= i 1)
  (print i)
  (+= i 2i64)
  (print i)
  (/= i 2)
  (print i)
)
```

```output
7
4
16
5
2
2.5
1.25
0.25
6
8
4
```

### An assigning operator wraps in the local's type.

```brix
(main
  (let b I8 127)
  (+= b 1)
  (print b)
  (let u U8 0)
  (-= u 1)
  (print u)
  (let w U16 300)
  (*= w 300)
  (print w)
  (let v U8 200)
  (+= v 100)
  (print v)
  (let g F32 1.5)
  (+= g 2)
  (*= g 1.5)
  (print g)
)
```

```output
-128
255
24464
44
5.25
```

### A number type called as a function converts its argument.

```brix
(main
  (let x 300)
  (print (call I8 1))
  (print (call Float 2))
  (print (call U8 x))
  (print (call I64 2.9))
)
```

```output
1
2
44
2
```

### `%%` raises a number to a power, in the type its two numbers promote to.

```brix
(main
  (let a -7)
  (let b 3)
  (let c I64 2)
  (print (%% -7 3))
  (print (%% 7.5 2))
  (print (%% a b))
  (print (%% c 40))
  (print (%% 2 0.5))
)
```

```output
-343
56.25
-343
1099511627776
1.4142135623730951
```

### The leading zeros of an integer are counted within its own width, which for a truth is one bit.

```brix
(import /core/intrinsics)
(blank)

(main
  (let a U8 1)
  (let b U16 1)
  (let c U32 1)
  (let d U64 1)
  (let e I8 -1)
  (print (call count-leading-zeros a))
  (print (call count-leading-zeros b))
  (print (call count-leading-zeros c))
  (print (call count-leading-zeros d))
  (print (call count-leading-zeros e))
  (print (call count-leading-zeros true))
  (print (call count-leading-zeros false))
)
```

```output
7
15
31
63
0
0
1
```

### The leading zeros of a float cannot be counted.

```brix crash
(import /core/intrinsics)
(blank)

(main
  (print (call count-leading-zeros 1.5))
)
```

```output
Exception: count-leading-zeros requires an integer argument
<no file:6>
      (print (call count-leading-zeros 1.5))
             ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Text becomes a number only where asked: by `as`, or by calling the number's type with it.

```brix
(main
  (print (+ (as "42" Int) 1))
  (print (as "2.5" Float))
  (print (call half (as "10" Int)))
  (print (call U8 "200"))
  (print (as "-9000000000" I64))
  (print (* (call F32 "1.5") 2))
  (print (as " 12 " Int))
  (print (as "18446744073709551615" U64))
  (print (call twice "4"))
)

(fn-expr + half Int
  (param n Int)
  (/ n 2)
)

(fn-do + twice Int
  (param text String)
  (let n (as text Int))
  (return (* n 2))
)
```

```output
43
2.5
5
200
-9000000000
3
12
18446744073709551615
8
```

### Text cast to a number that it does not hold panics.

```brix crash
(main
  (print (as "2.5x" Float))
)
```

```stderr
Unhandled panic: "2.5x" is not a number
<core/number.bx>

Task 1

Stack trace:
  0  core/number:read-float core/number.bx
  1  core/number:F64:from String11 core/number.bx
  2  __source__:main no file:3:10
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### Text given to a declaration of a number is not read as one.

```brix compile-error
(main
  (let n Int "42")
  (print n)
)
```

```output
Exception: "n" is declared Int, but its value is String, which can never be Int; convert the value, or declare "n" without a type
<no file:3>
      (let n Int "42")
           ^^^^^
```

### Text given as an argument that takes a number is not read as one.

```brix compile-error
(main
  (print (call half "10"))
)

(fn-expr + half Int
  (param n Int)
  (/ n 2)
)
```

```output
Exception: No function "half" accepts (String); it is declared as "half(n Int)"
<no file:3>
      (print (call half "10"))
             ^^^^^^^^^^^^^^^^
```

### Text is not read as a character, which is a number.

```brix compile-error
(main
  (let c Char "q")
  (print c)
)
```

```output
Exception: "c" is declared Char, but its value is String, which can never be Char; convert the value, or declare "c" without a type
<no file:3>
      (let c Char "q")
           ^^^^^^
```

### Text added to an array is not read as the number of items to drop: no `+` of an array takes text.

```brix compile-error
(main
  (let letters ([] String))
  (+= letters "x")
  (print (. letters length))
)
```

```output
Exception: Operator "+=" is not defined for Array'String and String
<no file:4>
      (+= letters "x")
      ^^^^^^^^^^^^^^^^
```

### Text that holds an integer too large or too small for its type is read as the type's largest or smallest value.

```brix
(main
  (print (as "3000000000" Int))
  (print (as "-3000000000" Int))
  (print (as "99999999999999999999" Int))
  (print (as "300" U8))
  (print (as "-5" U8))
  (print (as "-5" U64))
  (print (as "99999999999999999999" U64))
  (print (as "99999999999999999999" I64))
)
```

```output
3000000000
-3000000000
9223372036854775807
255
0
0
18446744073709551615
9223372036854775807
```

### A number type that declares an assigning operator answers an assignment of its values with it.

```brix
(main
  (let n (call Num 5))
  (+= n (call Num 2))
  (print (as n I32))
  (-= n (call Num 2))
  (print (as n I32))
  (let plain 5)
  (+= plain 2)
  (print plain)
)

(type + Num (is I32)
  (fn-expr + (infix this += (param other Num)) _
    (call This (* (as this I32) (as other I32)))
  )
)
```

```output
5
3
7
```

### A pointer answers its address, and converts to an integer of any width.

```brix
(main
  (let pointer (as (as 300 Pointer) (of CPointer U8)))
  (let plain (as 77 Pointer))
  (print (. pointer address))
  (print (. plain address))
  (let signed I32 pointer)
  (let byte U8 pointer)
  (let wide U64 plain)
  (print signed)
  (print byte)
  (print wide)
  (print (== (. pointer address) 300))
  (if (!= (. pointer address) 0)
    (print "set")
  )
)
```

```output
300
77
300
44
77
true
set
```

### an assigning operator gives a name the value before its store converts it, a field the value stored

```brix
(main
  (let x I8 127)
  (print (+= x 1))
  (print x)
  (let p (call Pair 127 3))
  (print (+= (. p small) 1))
  (print (. p small))
  (+= (. p size) 4)
  (print (. p size))
  (let s Size 3)
  (print (type-of (+= s 1)))
  (print s)
)

(type + Pair (is Struct)
  (prop + small I8)
  (prop + size Size)
)
```

```output
-128
-128
-128
-128
7
[TypeInfo I64]
4
```

### A narrow number read from memory, chosen from constants or computed keeps the value its type holds.

```brix
(main
  (let bytes (call (of Array U8) (list 0u8 1u8 127u8 128u8 200u8 255u8)))
  (let signed (call (of Array I8) (list -128i8 -1i8 0i8 127i8)))
  (let shorts (call (of Array U16) (list 0u16 255u16 256u16 65535u16)))
  (let signed-shorts (call (of Array I16) (list -32768i16 -1i16 0i16 32767i16)))
  (let flags (call (of Array Bool) (list true false)))
  (for (in index (..< 0 6))
    (call loaded-byte bytes index)
  )
  (for (in index (..< 0 4))
    (call loaded-signed-byte signed index)
  )
  (for (in index (..< 0 4))
    (call loaded-short shorts signed-shorts index)
  )
  (for (in index (..< 0 2))
    (call loaded-flag flags index)
  )
  (for (in index (..< 0 6))
    (call chosen-byte bytes index)
  )
  (call summed-bytes bytes)
  (let record (call Narrow 200u8 -56i8 65535u16 -2i16 true))
  (call record-fields record)
)

(type + Narrow (is Struct)
  (prop + byte U8)
  (prop + signed I8)
  (prop + short U16)
  (prop + signed-short I16)
  (prop + flag Bool)
)

(fn-expr + wrapped U8
  (param x U8)
  (param y U8)
  (+ x y)
)

(fn-do + loaded-byte _
  (param bytes (of Array U8))
  (param index I64)
  (let byte ([] bytes index))
  (let wide (as byte I64))
  (let sum (as (+ byte 100u8) U8))
  (let called (call wrapped byte 100u8))
  (let narrowed (as (* wide 3) U8))
  (let reinterpreted (as byte I8))
  (print (str byte ": " wide " " (as sum I64) " " (as called I64) " " (as narrowed I64) " " (as reinterpreted I64) " " (+ (* wide 256) (as sum I64))))
)

(fn-do + loaded-signed-byte _
  (param signed (of Array I8))
  (param index I64)
  (let byte ([] signed index))
  (let wide (as byte I64))
  (let sum (as (+ byte 100i8) I8))
  (let unsigned (as byte U8))
  (print (str byte ": " wide " " (as sum I64) " " (as unsigned I64) " " (* wide 256)))
)

(fn-do + loaded-short _
  (param shorts (of Array U16))
  (param signed-shorts (of Array I16))
  (param index I64)
  (let short ([] shorts index))
  (let signed-short ([] signed-shorts index))
  (let sum (as (+ short 1u16) U16))
  (let low (as short U8))
  (let signed-low (as signed-short I8))
  (print (str short " " signed-short ": " (as short I64) " " (as signed-short I64) " " (as sum I64) " " (as low I64) " " (as signed-low I64)))
)

(fn-do + loaded-flag _
  (param flags (of Array Bool))
  (param index I64)
  (let flag ([] flags index))
  (let taken "")
  (if flag
    (= taken (str taken " flag"))
  )
  (if (not flag)
    (= taken (str taken " !flag"))
  )
  (if (== flag ([] flags 0))
    (= taken (str taken " ==first"))
  )
  (print (str flag ":" taken))
)

(fn-do + chosen-byte _
  (param bytes (of Array U8))
  (param index I64)
  (let constant 100u8)
  (let signed-constant 100i8)
  (let mixed 7u8)
  (let loaded 3u8)
  (if (> index 2)
    (= constant 200u8)
    (= signed-constant -56i8)
    (= mixed (as (+ ([] bytes index) 250u8) U8))
    (= loaded ([] bytes index))
  )
  (print (str index ": " (as constant I64) " " (as signed-constant I64) " " (as mixed I64) " " (as loaded I64)))
)

(fn-do + summed-bytes _
  (param bytes (of Array U8))
  (let total 0u8)
  (let largest 0u8)
  (for (in byte bytes)
    (= total (as (+ total byte) U8))
    (if (> byte largest)
      (= largest byte)
    )
  )
  (print (str (as total I64) " " (as largest I64)))
)

(fn-do + record-fields _
  (param record Narrow)
  (let taken "")
  (if (. record flag)
    (= taken " flag")
  )
  (print (str (as (. record byte) I64) " " (as (. record signed) I64) " " (as (. record short) I64) " " (as (. record signed-short) I64) taken))
  (print (str (as (as (+ (. record byte) 100u8) U8) I64) " " (as (as (- (. record signed) 100i8) I8) I64) " " (as (as (+ (. record short) 1u16) U16) I64)))
)
```

```output
0: 0 100 100 0 0 100
1: 1 101 101 3 1 357
127: 127 227 227 125 127 32739
128: 128 228 228 128 -128 32996
200: 200 44 44 88 -56 51244
255: 255 99 99 253 -1 65379
-128: -128 -28 128 -32768
-1: -1 99 255 -256
0: 0 100 0 0
127: 127 -29 127 32512
0 -32768: 0 -32768 1 0 0
255 -1: 255 -1 256 255 -1
256 0: 256 0 257 0 0
65535 32767: 65535 32767 0 255 -1
true: flag ==first
false: !flag
0: 100 100 7 3
1: 100 100 7 3
2: 100 100 7 3
3: 200 -56 122 128
4: 200 -56 194 200
5: 200 -56 249 255
199 255
200 -56 65535 -2 flag
44 100 0
```

```brix
(main
  (print (call eighth -7))
  (print (call eighth -9))
  (print (call eighth -8))
  (print (call eighth 7))
  (print (call unsigned-eighth 18446744073709551615u64))
  (print (call unsigned-eighth 15u64))
)

(fn-do + eighth I64
  (param n I64)
  (return (/ n 8))
)

(fn-do + unsigned-eighth U64
  (param n U64)
  (return (/ n 8))
)
```

```output
0
-1
-1
0
2305843009213693951
1
```

### Text captured by a pattern typed as Float is read at full double precision.

```brix
(main
  (print
    (match "-2.8421709430404007e-14"
      (~ \float)
      (blank)
      (lazy float Float
        (pattern
          (~ (seq (? "-") (+ (chars "0-9")) "." (+ (chars "0-9")) (chars "eE") (? (chars "+-")) (+ (chars "0-9"))))
        )
      )
    )
  )
)
```

```output
-2.842170943040401e-14
```

### A class that is a number becomes a Bool by a test against zero.

```brix
(main
  (print (call truth (call Id 0)))
  (print (call truth (call Id 7)))
  (print (call shade (. Shade dark)))
  (print (call shade (. Shade light)))
  (print (call level (call Level 0)))
  (print (call level (call Level 3)))
)

(type + Id (is U32))

(type + Shade (is U8)
  (variant light)
  (variant dark)
)

(type + Level (is F64))

(fn-expr + truth Bool
  (param id Id)
  (as id Bool)
)

(fn-expr + shade Bool
  (param value Shade)
  (as value Bool)
)

(fn-expr + level Bool
  (param value Level)
  (as value Bool)
)
```

```output
false
true
true
false
false
true
```

### A signed number cast to a wider unsigned type is sign-extended, whether it is a constant or not.

```brix
(main
  (let back -1)
  (let small I8 -2)
  (print (as back U64))
  (print (as small U32))
  (print (as -1 U64))
  (print (as -2i8 U32))
)
```

```output
18446744073709551615
4294967294
18446744073709551615
4294967294
```

### An integer written with no suffix takes the type of the values beside it: another value a function returns, the type a declaration gives a match, or the item type of a list made from it.

```brix
(main
  (let none (list U64))
  (print (call smallest none))
  (let small U8
    (match (. none length)
      (case= 0
        250
      )
      (default
        1
      )
    )
  )
  (print (+ small 10))
  (let bytes (call (of List U8) 250 3))
  (print (+ ([] bytes 0) (* ([] bytes 1) 3)))
)

(fn-do + smallest _
  (param values (list U64))
  (if (not values)
    (return 0)
  )
  (return ([] values 0))
)
```

```output
0
4
3
```

### A float written with no suffix fits a declaration of F32 and stays in F32 beside a value of it.

```brix
(main
  (let half F32 0.5)
  (let quarter (* half 0.5))
  (let kept F32 quarter)
  (print kept)
)
```

```output
0.25
```

### An integer of 64 bits becomes a float only when asked, since the float holds only some of its values. One of 32 bits or fewer becomes F64 unasked.

```brix
(main
  (let small 7u32)
  (let wide F64 small)
  (print wide)
  (let count 3)
  (let part (/ (as count Float) 2))
  (print part)
)
```

```output
7
1.5
```

### An Int given to a declaration of a Float needs "as".

```brix compile-error
(main
  (let count 3)
  (let f Float count)
  (print f)
)
```

```output
Exception: Expected F64 here, but this is I64, which becomes F64 only when asked; write "(as count F64)"
<no file:4>
      (let f Float count)
             ^^^^^
```

### A float compared with an integer brings both to the float, as arithmetic on the two does.

```brix
(main
  (let t 0.5)
  (let count 3)
  (print (< t count))
  (print (>= count t))
  (print (* t count))
)
```

```output
true
true
1.5
```

### An integer written with no suffix beside a float takes the float's type: in a list, a choice, a fallback, and among the arguments that bind a type parameter, which a float binds before it.

```brix
(main
  (let xs (list 0 4 10.5))
  (print xs)
  (let r 0.25)
  (print (if-expr (> r 1.0) 0 r))
  (let width (? F64) null)
  (print (?? width 1))
  (print (call low 0 0.59))
  (print (. (call Span 0 0.5) high))
)

(fn-expr + (of low Num) _
  (param a Num)
  (param b Num)
  a
)

(type + (of Span Num) (is (tuple (: low Num) (: high Num))))
```

```output
0, 4, 10.5
0.25
1
0
0.5
```

### Integers with no suffix that arms of conditions written in a block of their own give take the type a declaration states, as the arms of a match do.

```brix
(main
  (let flag true)
  (let code I32
    (block
      (case? flag
        1
      )
      (default
        0
      )
    )
  )
  (print (+ code 2147483647))
)
```

```output
-2147483648
```

### Integers with no suffix that a match, arms of conditions or a value written alone in a block of its own give take the type of the local or property they are assigned to.

```brix
(main
  (let fast true)
  (let speed F64 1)
  (= speed
    (match fast
      (case= true
        75
      )
      (default
        55
      )
    )
  )
  (print (/ speed 2))
  (= speed
    (block
      (case? (not fast)
        75
      )
      (default
        55
      )
    )
  )
  (print (/ speed 2))
  (= speed
    (block
      75
    )
  )
  (print (/ speed 2))
  (let car (call Car))
  (= (. car speed)
    (match fast
      (case= true
        75
      )
      (default
        55
      )
    )
  )
  (print (/ (. car speed) 2))
  (= (. car speed)
    (block
      15
    )
  )
  (print (/ (. car speed) 2))
  (call (. car slow))
  (print (/ (. car speed) 2))
)

(type + Car
  (prop + speed F64 (= 1))
  (fn-do + slow _
    (= speed
      (block
        (case? (> speed 10)
          5
        )
        (default
          1
        )
      )
    )
  )
)
```

```output
37.5
27.5
37.5
37.5
7.5
2.5
```

### An optional local assigned a match whose arms each give a value, or give a match whose own arms each give one, holds one after it, while integers with no suffix the arms give take the type it holds.

```brix
(main
  (let fast true)
  (let speed (? F64) null)
  (= speed
    (match fast
      (case= true
        75
      )
      (default
        55
      )
    )
  )
  (print (/ speed 2))
  (let count (? Int) null)
  (= count
    (match fast
      (case= true
        (match fast
          (case= true
            1
          )
          (default
            2
          )
        )
      )
      (default
        3
      )
    )
  )
  (print (+ count 1))
)
```

```output
37.5
2
```

### Integers with no suffix written alone in a block of their own below a declaration, a property or a global take the type it states, and so do those the arms of a match a property or a global is given.

```brix
(main
  (let d F64
    (block
      75
    )
  )
  (print (/ d 2))
  (print (/ level 2))
  (let car (call Car))
  (print (/ (. car speed) 2))
  (print (/ (. car limit) 2))
)

(blank)
(prop + high _ (= true))
(prop + level F64
  (=
    (match high
      (case= true
        75
      )
      (default
        55
      )
    )
  )
)

(type + Car
  (prop + fast _ (= true))
  (prop + speed F64
    (=
      (block
        (case? fast
          75
        )
        (default
          55
        )
      )
    )
  )
  (prop + limit F64
    (=
      (block
        95
      )
    )
  )
)
```

```output
37.5
37.5
37.5
47.5
```

### Integers with no suffix that arms of conditions give where a function returns them take the type it declares, as the arms of a match it returns do.

```brix
(main
  (print (/ (call pick true) 2))
  (print (/ (call pick false) 2))
)

(fn-do + pick F64
  (param fast Bool)
  (return
    (block
      (case? fast
        75
      )
      (default
        55
      )
    )
  )
)
```

```output
37.5
27.5
```

### A choice between integers with no suffix, an operand of arithmetic, takes the type of the other operand, as one such integer does.

```brix
(main
  (let probe U32 3)
  (let flag true)
  (let keys (list U32))
  (+= keys (+ (if-expr flag 1 0) (* probe 2)))
  (+= keys (+ (* probe 2) (if-expr flag 1 0)))
  (print keys)
  (let wide U64 18446744073709551615)
  (print (- wide (if-expr flag 1 0)))
)
```

```output
7, 7
18446744073709551614
```

### A sum of a value known to fit a shifted immediate and an immediate gives their total.

```brix
(main
  (let depth Size 65536)
  (print (+ depth 1))
)
```

```output
65537
```
