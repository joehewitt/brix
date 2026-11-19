# Register allocation

### regalloc FPR scratch: 8 live doubles survive a call in straight-line code

```brix
(main
  (let f0 0.25)
  (let f1 1.25)
  (let f2 2.25)
  (let f3 3.25)
  (let f4 4.25)
  (let f5 5.25)
  (let f6 6.25)
  (let f7 7.25)
  (call poke)
  (let fsum 0.0)
  (+= fsum (* f0 1.0))
  (+= fsum (* f1 2.0))
  (+= fsum (* f2 3.0))
  (+= fsum (* f3 4.0))
  (+= fsum (* f4 5.0))
  (+= fsum (* f5 6.0))
  (+= fsum (* f6 7.0))
  (+= fsum (* f7 8.0))
  (print fsum)
)

(fn-do + poke _
  (let x 1)
  (+= x 1)
)
```

```output
177
```

### regalloc FPR scratch: 9 live doubles survive a call in straight-line code

```brix
(main
  (let f0 0.25)
  (let f1 1.25)
  (let f2 2.25)
  (let f3 3.25)
  (let f4 4.25)
  (let f5 5.25)
  (let f6 6.25)
  (let f7 7.25)
  (let f8 8.25)
  (call poke)
  (let fsum 0.0)
  (+= fsum (* f0 1.0))
  (+= fsum (* f1 2.0))
  (+= fsum (* f2 3.0))
  (+= fsum (* f3 4.0))
  (+= fsum (* f4 5.0))
  (+= fsum (* f5 6.0))
  (+= fsum (* f6 7.0))
  (+= fsum (* f7 8.0))
  (+= fsum (* f8 9.0))
  (print fsum)
)

(fn-do + poke _
  (let x 1)
  (+= x 1)
)
```

```output
251.25
```

### regalloc FPR scratch: 12 live doubles survive a call in straight-line code

```brix
(main
  (let f0 0.25)
  (let f1 1.25)
  (let f2 2.25)
  (let f3 3.25)
  (let f4 4.25)
  (let f5 5.25)
  (let f6 6.25)
  (let f7 7.25)
  (let f8 8.25)
  (let f9 9.25)
  (let f10 10.25)
  (let f11 11.25)
  (call poke)
  (let fsum 0.0)
  (+= fsum (* f0 1.0))
  (+= fsum (* f1 2.0))
  (+= fsum (* f2 3.0))
  (+= fsum (* f3 4.0))
  (+= fsum (* f4 5.0))
  (+= fsum (* f5 6.0))
  (+= fsum (* f6 7.0))
  (+= fsum (* f7 8.0))
  (+= fsum (* f8 9.0))
  (+= fsum (* f9 10.0))
  (+= fsum (* f10 11.0))
  (+= fsum (* f11 12.0))
  (print fsum)
)

(fn-do + poke _
  (let x 1)
  (+= x 1)
)
```

```output
591.5
```

### regalloc FPR scratch: 40 live doubles survive a call in straight-line code

```brix
(main
  (let f0 0.25)
  (let f1 1.25)
  (let f2 2.25)
  (let f3 3.25)
  (let f4 4.25)
  (let f5 5.25)
  (let f6 6.25)
  (let f7 7.25)
  (let f8 8.25)
  (let f9 9.25)
  (let f10 10.25)
  (let f11 11.25)
  (let f12 12.25)
  (let f13 13.25)
  (let f14 14.25)
  (let f15 15.25)
  (let f16 16.25)
  (let f17 17.25)
  (let f18 18.25)
  (let f19 19.25)
  (let f20 20.25)
  (let f21 21.25)
  (let f22 22.25)
  (let f23 23.25)
  (let f24 24.25)
  (let f25 25.25)
  (let f26 26.25)
  (let f27 27.25)
  (let f28 28.25)
  (let f29 29.25)
  (let f30 30.25)
  (let f31 31.25)
  (let f32 32.25)
  (let f33 33.25)
  (let f34 34.25)
  (let f35 35.25)
  (let f36 36.25)
  (let f37 37.25)
  (let f38 38.25)
  (let f39 39.25)
  (call poke)
  (let fsum 0.0)
  (+= fsum (* f0 1.0))
  (+= fsum (* f1 2.0))
  (+= fsum (* f2 3.0))
  (+= fsum (* f3 4.0))
  (+= fsum (* f4 5.0))
  (+= fsum (* f5 6.0))
  (+= fsum (* f6 7.0))
  (+= fsum (* f7 8.0))
  (+= fsum (* f8 9.0))
  (+= fsum (* f9 10.0))
  (+= fsum (* f10 11.0))
  (+= fsum (* f11 12.0))
  (+= fsum (* f12 13.0))
  (+= fsum (* f13 14.0))
  (+= fsum (* f14 15.0))
  (+= fsum (* f15 16.0))
  (+= fsum (* f16 17.0))
  (+= fsum (* f17 18.0))
  (+= fsum (* f18 19.0))
  (+= fsum (* f19 20.0))
  (+= fsum (* f20 21.0))
  (+= fsum (* f21 22.0))
  (+= fsum (* f22 23.0))
  (+= fsum (* f23 24.0))
  (+= fsum (* f24 25.0))
  (+= fsum (* f25 26.0))
  (+= fsum (* f26 27.0))
  (+= fsum (* f27 28.0))
  (+= fsum (* f28 29.0))
  (+= fsum (* f29 30.0))
  (+= fsum (* f30 31.0))
  (+= fsum (* f31 32.0))
  (+= fsum (* f32 33.0))
  (+= fsum (* f33 34.0))
  (+= fsum (* f34 35.0))
  (+= fsum (* f35 36.0))
  (+= fsum (* f36 37.0))
  (+= fsum (* f37 38.0))
  (+= fsum (* f38 39.0))
  (+= fsum (* f39 40.0))
  (print fsum)
)

(fn-do + poke _
  (let x 1)
  (+= x 1)
)
```

```output
21525
```

### regalloc FPR scratch: 8 loop-carried doubles survive repeated calls

```brix
(main
  (let f0 0.25)
  (let f1 1.25)
  (let f2 2.25)
  (let f3 3.25)
  (let f4 4.25)
  (let f5 5.25)
  (let f6 6.25)
  (let f7 7.25)
  (let fsum 0.0)
  (for (in iteration (..< 0 3))
    (+= f0 0.5)
    (+= f1 0.5)
    (+= f2 0.5)
    (+= f3 0.5)
    (+= f4 0.5)
    (+= f5 0.5)
    (+= f6 0.5)
    (+= f7 0.5)
    (call poke)
    (+= fsum (* f0 1.0))
    (+= fsum (* f1 2.0))
    (+= fsum (* f2 3.0))
    (+= fsum (* f3 4.0))
    (+= fsum (* f4 5.0))
    (+= fsum (* f5 6.0))
    (+= fsum (* f6 7.0))
    (+= fsum (* f7 8.0))
  )
  (print fsum)
)

(fn-do + poke _
  (let x 1)
  (+= x 1)
)
```

```output
639
```

### regalloc FPR scratch: 9 loop-carried doubles survive repeated calls

```brix
(main
  (let f0 0.25)
  (let f1 1.25)
  (let f2 2.25)
  (let f3 3.25)
  (let f4 4.25)
  (let f5 5.25)
  (let f6 6.25)
  (let f7 7.25)
  (let f8 8.25)
  (let fsum 0.0)
  (for (in iteration (..< 0 3))
    (+= f0 0.5)
    (+= f1 0.5)
    (+= f2 0.5)
    (+= f3 0.5)
    (+= f4 0.5)
    (+= f5 0.5)
    (+= f6 0.5)
    (+= f7 0.5)
    (+= f8 0.5)
    (call poke)
    (+= fsum (* f0 1.0))
    (+= fsum (* f1 2.0))
    (+= fsum (* f2 3.0))
    (+= fsum (* f3 4.0))
    (+= fsum (* f4 5.0))
    (+= fsum (* f5 6.0))
    (+= fsum (* f6 7.0))
    (+= fsum (* f7 8.0))
    (+= fsum (* f8 9.0))
  )
  (print fsum)
)

(fn-do + poke _
  (let x 1)
  (+= x 1)
)
```

```output
888.75
```

### regalloc FPR scratch: 12 loop-carried doubles survive repeated calls

```brix
(main
  (let f0 0.25)
  (let f1 1.25)
  (let f2 2.25)
  (let f3 3.25)
  (let f4 4.25)
  (let f5 5.25)
  (let f6 6.25)
  (let f7 7.25)
  (let f8 8.25)
  (let f9 9.25)
  (let f10 10.25)
  (let f11 11.25)
  (let fsum 0.0)
  (for (in iteration (..< 0 3))
    (+= f0 0.5)
    (+= f1 0.5)
    (+= f2 0.5)
    (+= f3 0.5)
    (+= f4 0.5)
    (+= f5 0.5)
    (+= f6 0.5)
    (+= f7 0.5)
    (+= f8 0.5)
    (+= f9 0.5)
    (+= f10 0.5)
    (+= f11 0.5)
    (call poke)
    (+= fsum (* f0 1.0))
    (+= fsum (* f1 2.0))
    (+= fsum (* f2 3.0))
    (+= fsum (* f3 4.0))
    (+= fsum (* f4 5.0))
    (+= fsum (* f5 6.0))
    (+= fsum (* f6 7.0))
    (+= fsum (* f7 8.0))
    (+= fsum (* f8 9.0))
    (+= fsum (* f9 10.0))
    (+= fsum (* f10 11.0))
    (+= fsum (* f11 12.0))
  )
  (print fsum)
)

(fn-do + poke _
  (let x 1)
  (+= x 1)
)
```

```output
2008.5
```

### regalloc FPR scratch: 40 loop-carried doubles survive repeated calls

```brix
(main
  (let f0 0.25)
  (let f1 1.25)
  (let f2 2.25)
  (let f3 3.25)
  (let f4 4.25)
  (let f5 5.25)
  (let f6 6.25)
  (let f7 7.25)
  (let f8 8.25)
  (let f9 9.25)
  (let f10 10.25)
  (let f11 11.25)
  (let f12 12.25)
  (let f13 13.25)
  (let f14 14.25)
  (let f15 15.25)
  (let f16 16.25)
  (let f17 17.25)
  (let f18 18.25)
  (let f19 19.25)
  (let f20 20.25)
  (let f21 21.25)
  (let f22 22.25)
  (let f23 23.25)
  (let f24 24.25)
  (let f25 25.25)
  (let f26 26.25)
  (let f27 27.25)
  (let f28 28.25)
  (let f29 29.25)
  (let f30 30.25)
  (let f31 31.25)
  (let f32 32.25)
  (let f33 33.25)
  (let f34 34.25)
  (let f35 35.25)
  (let f36 36.25)
  (let f37 37.25)
  (let f38 38.25)
  (let f39 39.25)
  (let fsum 0.0)
  (for (in iteration (..< 0 3))
    (+= f0 0.5)
    (+= f1 0.5)
    (+= f2 0.5)
    (+= f3 0.5)
    (+= f4 0.5)
    (+= f5 0.5)
    (+= f6 0.5)
    (+= f7 0.5)
    (+= f8 0.5)
    (+= f9 0.5)
    (+= f10 0.5)
    (+= f11 0.5)
    (+= f12 0.5)
    (+= f13 0.5)
    (+= f14 0.5)
    (+= f15 0.5)
    (+= f16 0.5)
    (+= f17 0.5)
    (+= f18 0.5)
    (+= f19 0.5)
    (+= f20 0.5)
    (+= f21 0.5)
    (+= f22 0.5)
    (+= f23 0.5)
    (+= f24 0.5)
    (+= f25 0.5)
    (+= f26 0.5)
    (+= f27 0.5)
    (+= f28 0.5)
    (+= f29 0.5)
    (+= f30 0.5)
    (+= f31 0.5)
    (+= f32 0.5)
    (+= f33 0.5)
    (+= f34 0.5)
    (+= f35 0.5)
    (+= f36 0.5)
    (+= f37 0.5)
    (+= f38 0.5)
    (+= f39 0.5)
    (call poke)
    (+= fsum (* f0 1.0))
    (+= fsum (* f1 2.0))
    (+= fsum (* f2 3.0))
    (+= fsum (* f3 4.0))
    (+= fsum (* f4 5.0))
    (+= fsum (* f5 6.0))
    (+= fsum (* f6 7.0))
    (+= fsum (* f7 8.0))
    (+= fsum (* f8 9.0))
    (+= fsum (* f9 10.0))
    (+= fsum (* f10 11.0))
    (+= fsum (* f11 12.0))
    (+= fsum (* f12 13.0))
    (+= fsum (* f13 14.0))
    (+= fsum (* f14 15.0))
    (+= fsum (* f15 16.0))
    (+= fsum (* f16 17.0))
    (+= fsum (* f17 18.0))
    (+= fsum (* f18 19.0))
    (+= fsum (* f19 20.0))
    (+= fsum (* f20 21.0))
    (+= fsum (* f21 22.0))
    (+= fsum (* f22 23.0))
    (+= fsum (* f23 24.0))
    (+= fsum (* f24 25.0))
    (+= fsum (* f25 26.0))
    (+= fsum (* f26 27.0))
    (+= fsum (* f27 28.0))
    (+= fsum (* f28 29.0))
    (+= fsum (* f29 30.0))
    (+= fsum (* f30 31.0))
    (+= fsum (* f31 32.0))
    (+= fsum (* f32 33.0))
    (+= fsum (* f33 34.0))
    (+= fsum (* f34 35.0))
    (+= fsum (* f35 36.0))
    (+= fsum (* f36 37.0))
    (+= fsum (* f37 38.0))
    (+= fsum (* f38 39.0))
    (+= fsum (* f39 40.0))
  )
  (print fsum)
)

(fn-do + poke _
  (let x 1)
  (+= x 1)
)
```

```output
67035
```

### regalloc scratch classes: float and integer pressure coexist across one call

```brix
(main
  (let f0 0.25)
  (let f1 1.25)
  (let f2 2.25)
  (let f3 3.25)
  (let f4 4.25)
  (let f5 5.25)
  (let f6 6.25)
  (let f7 7.25)
  (let f8 8.25)
  (let f9 9.25)
  (let f10 10.25)
  (let f11 11.25)
  (let f12 12.25)
  (let f13 13.25)
  (let f14 14.25)
  (let f15 15.25)
  (let f16 16.25)
  (let f17 17.25)
  (let f18 18.25)
  (let f19 19.25)
  (let f20 20.25)
  (let f21 21.25)
  (let f22 22.25)
  (let f23 23.25)
  (let f24 24.25)
  (let f25 25.25)
  (let f26 26.25)
  (let f27 27.25)
  (let f28 28.25)
  (let f29 29.25)
  (let f30 30.25)
  (let f31 31.25)
  (let f32 32.25)
  (let f33 33.25)
  (let f34 34.25)
  (let f35 35.25)
  (let f36 36.25)
  (let f37 37.25)
  (let f38 38.25)
  (let f39 39.25)
  (let i0 1)
  (let i1 2)
  (let i2 3)
  (let i3 4)
  (let i4 5)
  (let i5 6)
  (let i6 7)
  (let i7 8)
  (let i8 9)
  (let i9 10)
  (let i10 11)
  (let i11 12)
  (let i12 13)
  (let i13 14)
  (let i14 15)
  (let i15 16)
  (let i16 17)
  (let i17 18)
  (let i18 19)
  (let i19 20)
  (let i20 21)
  (let i21 22)
  (let i22 23)
  (let i23 24)
  (let i24 25)
  (let i25 26)
  (let i26 27)
  (let i27 28)
  (let i28 29)
  (let i29 30)
  (let i30 31)
  (let i31 32)
  (let i32 33)
  (let i33 34)
  (call poke)
  (let fsum 0.0)
  (+= fsum (* f0 1.0))
  (+= fsum (* f1 2.0))
  (+= fsum (* f2 3.0))
  (+= fsum (* f3 4.0))
  (+= fsum (* f4 5.0))
  (+= fsum (* f5 6.0))
  (+= fsum (* f6 7.0))
  (+= fsum (* f7 8.0))
  (+= fsum (* f8 9.0))
  (+= fsum (* f9 10.0))
  (+= fsum (* f10 11.0))
  (+= fsum (* f11 12.0))
  (+= fsum (* f12 13.0))
  (+= fsum (* f13 14.0))
  (+= fsum (* f14 15.0))
  (+= fsum (* f15 16.0))
  (+= fsum (* f16 17.0))
  (+= fsum (* f17 18.0))
  (+= fsum (* f18 19.0))
  (+= fsum (* f19 20.0))
  (+= fsum (* f20 21.0))
  (+= fsum (* f21 22.0))
  (+= fsum (* f22 23.0))
  (+= fsum (* f23 24.0))
  (+= fsum (* f24 25.0))
  (+= fsum (* f25 26.0))
  (+= fsum (* f26 27.0))
  (+= fsum (* f27 28.0))
  (+= fsum (* f28 29.0))
  (+= fsum (* f29 30.0))
  (+= fsum (* f30 31.0))
  (+= fsum (* f31 32.0))
  (+= fsum (* f32 33.0))
  (+= fsum (* f33 34.0))
  (+= fsum (* f34 35.0))
  (+= fsum (* f35 36.0))
  (+= fsum (* f36 37.0))
  (+= fsum (* f37 38.0))
  (+= fsum (* f38 39.0))
  (+= fsum (* f39 40.0))
  (let isum 0)
  (+= isum (* i0 1))
  (+= isum (* i1 2))
  (+= isum (* i2 3))
  (+= isum (* i3 4))
  (+= isum (* i4 5))
  (+= isum (* i5 6))
  (+= isum (* i6 7))
  (+= isum (* i7 8))
  (+= isum (* i8 9))
  (+= isum (* i9 10))
  (+= isum (* i10 11))
  (+= isum (* i11 12))
  (+= isum (* i12 13))
  (+= isum (* i13 14))
  (+= isum (* i14 15))
  (+= isum (* i15 16))
  (+= isum (* i16 17))
  (+= isum (* i17 18))
  (+= isum (* i18 19))
  (+= isum (* i19 20))
  (+= isum (* i20 21))
  (+= isum (* i21 22))
  (+= isum (* i22 23))
  (+= isum (* i23 24))
  (+= isum (* i24 25))
  (+= isum (* i25 26))
  (+= isum (* i26 27))
  (+= isum (* i27 28))
  (+= isum (* i28 29))
  (+= isum (* i29 30))
  (+= isum (* i30 31))
  (+= isum (* i31 32))
  (+= isum (* i32 33))
  (+= isum (* i33 34))
  (print fsum)
  (print isum)
)

(fn-do + poke _
  (let x 1)
  (+= x 1)
)
```

```output
21525
13685
```

### regalloc: a recursive call that swaps or rotates its own arguments

```brix
(main
  (print (call swap 10 3 0))
  (print (call swap 10 3 1))
  (print (call swap 10 3 2))
  (print (call swap-float 10.0 3.5 1))
  (print (call swap-float 10.0 3.5 2))
  (print (call rotate 1 2 3 0))
  (print (call rotate 1 2 3 1))
  (print (call rotate 1 2 3 2))
  (print (call rotate 1 2 3 3))
)

(fn-do + swap I64
  (param x I64)
  (param y I64)
  (param depth I64)
  (return-if (- x y) (if (== depth 0)))
  (return (+ 1000 (call swap y x (- depth 1))))
)

(fn-do + swap-float F64
  (param x F64)
  (param y F64)
  (param depth I64)
  (return-if (- x y) (if (== depth 0)))
  (return (+ 1000.0 (call swap-float y x (- depth 1))))
)

(fn-do + rotate I64
  (param x I64)
  (param y I64)
  (param z I64)
  (param depth I64)
  (return-if (+ (* x 100) (* y 10) z) (if (== depth 0)))
  (return (+ 1000 (call rotate z x y (- depth 1))))
)
```

```output
7
993
2007
993.5
2006.5
123
1312
2231
3123
```

### frame coloring: large tuple locals in exclusive, sequential and overlapping scopes

```brix
(main
  (print (call single))
  (print (call single-managed))
  (print (call exclusive true))
  (print (call exclusive false))
  (print (call sequential))
  (print (call managed true))
  (print (call managed false))
  (print (call overlapping 4))
)

(blank)
(type-line + Big (is (tuple (: a U64) (: b U64) (: c U64) (: d U64) (: e U64) (: f U64) (: g U64) (: h U64))))
(type-line + Text (is (tuple (: a String) (: b String) (: c String) (: d String) (: e String) (: f String) (: g String) (: h String))))

(fn-do _ exclusive U64
  (param flag Bool)
  (if flag
    (let local (call Big 1 2 3 4 5 6 7 8))
    (return (. local a))
    (else
      (let local (call Big 8 7 6 5 4 3 2 1))
      (return (. local a))
    )
  )
)

(fn-do _ sequential U64
  (let a (call Big 1 2 3 4 5 6 7 8))
  (let result (. a a))
  (let b (call Big 2 3 4 5 6 7 8 9))
  (+= result (. b b))
  (let c (call Big 3 4 5 6 7 8 9 10))
  (return (+ result (. c c)))
)

(fn-do _ managed String
  (param flag Bool)
  (if flag
    (let local (call Text "a" "b" "c" "d" "e" "f" "g" "h"))
    (return (. local a))
    (else
      (let local (call Text "h" "g" "f" "e" "d" "c" "b" "a"))
      (return (. local b))
    )
  )
)

(fn-do _ single U64
  (let local (call Big 1 2 3 4 5 6 7 8))
  (return (. local a))
)

(fn-do _ single-managed String
  (let local (call Text "a" "b" "c" "d" "e" "f" "g" "h"))
  (return (. local a))
)

(fn-do _ overlapping U64
  (param count U64)
  (let total 0u64)
  (let i 0u64)
  (while (< i count)
    (let first (call Big i 2 3 4 5 6 7 8))
    (let second (call Big 10 i 30 40 50 60 70 80))
    (+= total (+ (. first a) (. second b) (. first h) (. second h)))
    (+= i 1)
  )
  (return total)
)
```

```output
1
a
1
8
9
a
g
364
```

### float copies: a loop that rewrites the float fields of each list element keeps them in float registers and leaves its receiver and argument intact

```brix
(main
  (let holder (call Holder))
  (+= (. holder items) (call Item 1.5 2.5 3.5 4.5 5.5))
  (+= (. holder items) (call Item 10.5 20.5 30.5 40.5 50.5))
  (let shifter (call Shifter))
  (call (. shifter shift) holder 1.0)
  (print (. holder label))
  (print (. shifter shifts))
  (for (in item (. holder items))
    (print (str (. item a) " " (. item b) " " (. item c) " " (. item d) " " (. item e)))
  )
)

(type + Item (is Tuple)
  (prop + a F32)
  (prop + b F32)
  (prop + c F32)
  (prop + d F32)
  (prop + e F32)
)

(type + Holder
  (prop + items (list Item) (= (call (of List Item))))
  (prop + label String (= "holder"))
)

(type + Shifter
  (prop + shifts Index (= 0))
  (fn-do + shift _
    (param holder Holder)
    (param wide Float)
    (let delta (as wide F32))
    (let index Size)
    (while (< index (. holder items length))
      (let item ([] (. holder items) index))
      (= ([] (. holder items) index) (call Item (+ (. item a) delta) (. item b) (. item c) (. item d) (. item e)))
      (+= index 1)
    )
    (+= shifts 1)
  )
)
```

```output
holder
1
2.5 2.5 3.5 4.5 5.5
11.5 20.5 30.5 40.5 50.5
```
