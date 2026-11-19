# Structs

```brix
(main
  (let p (call Point 1 2))
  (print p)
  (= (. p x) 5)
  (= ([] p 1) 9)
  (print p)
  (print (call (. p sum)))
  (print (. (type-of p) is-struct))
  (print (. (type-of p) is-tuple))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
  (fn-expr + sum I32
    (+ x y)
  )
)
```

```output
(1, 2)
(5, 9)
14
true
false
```

```brix
(main
  (let p (call Point 3))
  (print p)
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
  (fn-do + This _
    (param n I32)
    (= x n)
    (= y (+ n 1))
  )
)
```

```output
(3, 4)
```

```brix
(main
  (let c (call Colored 1 2 3))
  (let p (as c Point))
  (= (. p x) 7)
  (print c)
  (print p)
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)

(type + Colored (is Point)
  (prop + color I32)
)
```

```output
(1, 2, 3)
(7, 2)
```

```brix
(main
  (let line (call Line (call Point 1 2) (call Point 3 4)))
  (= (. line start x) 7)
  (print line)
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)

(type + Line (is Struct)
  (prop + start Point)
  (prop + end Point)
)
```

```output
((7, 2), (3, 4))
```

```brix
(import /BxCore/tests.h)
(blank)

(main
  (let data (call Small 81 92))
  (call C:ctestStructSmall1 data)
  (blank)
  (let ret (as (call C:ctestStructReturnSmall1) Small))
  (print ret)
  (= (. ret a) 7)
  (print ret)
)

(type + Small (is Struct)
  (prop + a I32)
  (prop + b I32)
)
```

```output
a 81 b 92
(1, 2)
(7, 2)
```

### Default, partial, and full construction

```brix
(main
  (let a (call Thing))
  (let b (call Thing 10))
  (let c (call Thing 10 20 30))
  (print a)
  (print b)
  (print c)
  (print (. a length))
)

(type + Thing (is Struct)
  (prop + foo I32)
  (prop + bar I32 (= 20))
  (prop + baz _ (= (+ foo bar)))
)
```

```output
(0, 20, 20)
(10, 20, 30)
(10, 20, 30)
3
```

### Defaults can refer to base fields and fields declared later

```brix
(main
  (let child (call Child))
  (let later (call Later))
  (print child)
  (print later)
)

(type + Base (is Struct)
  (prop + foo I32 (= 1))
  (prop + bar I32 (= 2))
)

(type + Child (is Base)
  (prop + baz I32 (= (+ bar 1)))
)

(type + Later (is Struct)
  (prop + zab I32 (= bar))
  (prop + bar I32 (= 20))
)
```

```output
(1, 2, 3)
(20, 20)
```

### Circular default references are rejected

```brix crash
(main
  (let thing (call Thing))
  (print thing)
)

(type + Thing (is Struct)
  (prop + a I32 (= b))
  (prop + b I32 (= a))
)
```

```output
Exception: "a" depends on its own value, directly or through the values it reads; break the cycle
<no file:8>
      (prop + a I32 (= b))
      ^^^^^^^^^^^^^^^^^^^^
```

### Constructor can return a struct value

```brix
(main
  (let thing (call Thing 10.5))
  (print thing)
)

(type + Thing (is Struct)
  (prop + foo I32)
  (prop + bar Float)
  (fn-do + This _
    (param n Float)
    (return (call This 1 n))
  )
)
```

```output
(1, 10.5)
```

### Assignment expressions mutate fields and return the assigned value

```brix
(main
  (let p (call Point 1 2))
  (let rx (= (. p x) 8))
  (let ry (= ([] p 1) 9))
  (print rx)
  (print ry)
  (print p)
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
8
9
(8, 9)
```

### Struct assignment copies values

```brix
(main
  (let p (call Point 1 2))
  (let q p)
  (= (. q x) 9)
  (print p)
  (print q)
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
(1, 2)
(9, 2)
```

### Struct parameters are passed by value

```brix
(main
  (let p (call Point 1 2))
  (call mutate p)
  (print p)
)

(fn-do + mutate _
  (param point Point)
  (= (. point x) 9)
  (print point)
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
(9, 2)
(1, 2)
```

### Nested struct projections are copies unless assigned through the original path

```brix
(main
  (let line (call Line (call Point 1 2) (call Point 3 4)))
  (let start (. line start))
  (= (. start x) 8)
  (print line)
  (print start)
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)

(type + Line (is Struct)
  (prop + start Point)
  (prop + end Point)
)
```

```output
((1, 2), (3, 4))
(8, 2)
```

### Struct method receivers are passed by value

```brix
(main
  (let p (call Point 1 2))
  (call (. p move) 5)
  (print p)
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
  (fn-do + move _
    (param dx I32)
    (= x (+ x dx))
    (print this)
  )
)
```

```output
(6, 2)
(1, 2)
```

### Struct index assignment cannot extend the layout

```brix crash
(main
  (let p (call Point 1 2))
  (= ([] p 2) 3)
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
Exception: Index out of bounds
<no file:4>
      (= ([] p 2) 3)
         ^^^^^^^^
```

### Struct addition assignment cannot append to the layout

```brix crash
(main
  (let p (call Point 1 2))
  (+= p 3)
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
Exception: Function not found
<no file:4>
      (+= p 3)
      ^^^^^^^^
```

### Struct length cannot be reassigned

```brix crash
(main
  (let p (call Point 1 2))
  (= (. p length) 1)
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
Exception: Struct layout cannot be changed
<no file:4>
      (= (. p length) 1)
         ^^^^^^^^^^^^
```

### Struct indexes must be constant

```brix crash
(main
  (let p (call Point 1 2))
  (let i 0)
  (print ([] p i))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
Exception: Value has no constant form
<no file:5>
      (print ([] p i))
             ^^^^^^^^
```

### Struct inheritance and casts keep layout-compatible conversions

```brix
(main
  (let c (call Colored 1 2 3))
  (let p (as c Point))
  (let t (as (call Point 4 5) (tuple I32 I32)))
  (let from-tuple (as (tuple 6 7) Point))
  (print c)
  (print p)
  (print t)
  (print from-tuple)
  (print (type-of from-tuple))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)

(type + Colored (is Point)
  (prop + color I32)
)
```

```output
(1, 2, 3)
(1, 2)
(4, 5)
(6, 7)
[TypeInfo Point]
```

### Same-layout named structs remain distinct in type patterns

```brix
(main
  (print
    (match (call A "hi" 1)
      (case B
        "B"
      )
      (case A
        "A"
      )
    )
  )
)

(type + A (is Struct)
  (prop + a String)
  (prop + b I32)
)

(type + B (is Struct)
  (prop + a String)
  (prop + b I32)
)
```

```output
A
```

### Inherited struct fields cannot be redefined

```brix crash
(main
  (let bar (call Bar))
  (print bar)
)

(type + Foo (is Struct)
  (prop + a I32)
)

(type + Bar (is Foo)
  (prop + a I32)
)
```

```output
Exception: "a" is inherited from Foo, so it keeps the type it has there and cannot state its own; remove "I32"
<no file:12>
      (prop + a I32)
      ^^^^^^^^^^^^^^
```

### Parameterized structs can be explicit or inferred

```brix
(main
  (let explicit (call (of Box I32) 42 1))
  (let inferred (call Box "hi" 2))
  (print explicit)
  (print inferred)
  (print (type-of inferred))
)

(type + (of Box T) (is Struct)
  (prop + item T)
  (prop + count I32)
)
```

```output
(42, 1)
(hi, 2)
[TypeInfo Box'String]
```

### Parameterized structs can inherit from parameterized structs

```brix
(main
  (let pair (call Pair 1 2 3))
  (print pair)
)

(type + (of Base T) (is Struct)
  (prop + a T)
  (prop + b I32)
)

(type + (of Pair T) (is (of Base T))
  (prop + c T)
)
```

```output
(1, 2, 3)
```

### Generic functions infer parameters from struct arguments

```brix
(main
  (print (call values (call Pair 1 2)))
)

(type + (of Pair T1 T2) (is Struct)
  (prop + a T1)
  (prop + b T2)
)

(fn-expr + (of values T1 T2) _
  (param pair (of Pair T1 T2))
  (tuple (. pair a) (. pair b))
)
```

```output
(1, 2)
```

### Getter properties are not stored fields

```brix
(main
  (let thing (call Thing 1 2))
  (print thing)
  (print (. thing b))
  (print (. thing length))
)

(type + Thing (is Struct)
  (prop + a I32)
  (prop + c I32)
  (fn-expr + b _ :get
    42
  )
)
```

```output
(1, 2)
42
2
```

### Operator overrides still dispatch before builtin struct behavior

```brix
(main
  (let thing (call Thing 20))
  (print (+= thing 3))
  (print (+ thing 4))
)

(type + Thing (is Struct)
  (prop + foo Int)
  (fn-expr + (infix this += (param other ''T)) _
    (call This (* foo other))
  )
  (fn-expr + (infix this + (param other ''T)) _
    (call This (+ foo other))
  )
)
```

```output
(60)
(64)
```

### Index overrides still dispatch before builtin struct behavior

```brix
(main
  (let thing (call Thing 2 3))
  (print ([] thing 10))
  (= ([] thing 10) 7)
)

(type + Thing (is Struct)
  (prop + x Int)
  (prop + y Int)
  (fn-expr + (index-get this (param index _)) Int
    (+ x y index)
  )
  (fn-do + (index-set this (param index _) (param value _)) _
    (print (str "set " index " " value))
  )
)
```

```output
15
set 10 7
```

### Lazy properties are not supported on structs

```brix crash
(main
  (let thing (call Thing))
  (print thing)
)

(type + Thing (is Struct)
  (prop + value _ (:= 42))
)
```

```output
Exception: Property not supported
<no file:8>
      (prop + value _ (:= 42))
      ^^^^^^^^^^^^^^^^^^^^^^^^
```

### State machine properties are not supported on structs

```brix crash
(main
  (let thing (call Thing))
  (print thing)
)

(type + Thing (is Struct)
  (prop + flow _
    (machine
      (state idle)
      (state active)
    )
  )
)
```

```output
Exception: State machine properties are not supported on structs
<no file:8>
      (prop + flow _
      ^^^^^^^^^^^^^
```

### C struct returns are StructType values, not tuples

```brix
(import /BxCore/tests.h)
(blank)

(main
  (let ret (call C:ctestStructReturnSmall1))
  (print ret)
  (print (. ret a))
  (print (. (type-of ret) is-struct))
  (print (. (type-of ret) is-tuple))
  (= (. ret a) 9)
  (print ret)
)
```

```output
(1, 2)
1
true
false
(9, 2)
```

### Named structs can be passed to C struct parameters

```brix
(import /BxCore/tests.h)
(blank)

(main
  (let small (call Small 81 92))
  (let small2 (call Small2 81 92 33 45))
  (let hfa (call HFA 1.8 9.2 3.3))
  (call C:ctestStructSmall1 small)
  (call C:ctestStructSmall2 small2)
  (call C:ctestStructHFA hfa)
)

(type + Small (is Struct)
  (prop + a I32)
  (prop + b I32)
)

(type + Small2 (is Struct)
  (prop + a I32)
  (prop + b I32)
  (prop + c I32)
  (prop + d I32)
)

(type + HFA (is Struct)
  (prop + a Float)
  (prop + b Float)
  (prop + c Float)
)
```

```output
a 81 b 92
a 81 b 92 c 33 d 45
a 1.800000 b 9.200000 c 3.300000
```

### C struct parameters accept tuple values through final casts

```brix
(import /BxCore/tests.h)
(blank)

(main
  (let small (tuple 81 92))
  (let large (tuple 81 92 33 45 55))
  (let nested (tuple 81 92 33 45 (tuple 88 99)))
  (call C:ctestStructSmall1 small)
  (call C:ctestStructLarge large)
  (call C:ctestStructLargeNested nested)
)
```

```output
a 81 b 92
a 81 b 92 c 33 d 45 e 55
a 81 b 92 c 33 d 45
e.a 88 e.b 99
```

### C struct return values can be cast to named structs and mutated

```brix
(import /BxCore/tests.h)
(blank)

(main
  (let hfa (as (call C:ctestStructReturnHFA) HFA))
  (let small2 (as (call C:ctestStructReturnSmall2) Small2))
  (let large (as (call C:ctestStructReturnLarge) Large))
  (print hfa)
  (print small2)
  (print large)
  (= (. large e) 99)
  (print large)
)

(type + HFA (is Struct)
  (prop + a Float)
  (prop + b Float)
  (prop + c Float)
)

(type + Small2 (is Struct)
  (prop + a I32)
  (prop + b I32)
  (prop + c I32)
  (prop + d I32)
)

(type + Large (is Struct)
  (prop + a I32)
  (prop + b I32)
  (prop + c I32)
  (prop + d I32)
  (prop + e I32)
)
```

```output
(1.2, 2.3, 3.4)
(1, 2, 3, 4)
(1, 2, 3, 4, 5)
(1, 2, 3, 4, 99)
```

### A struct's field is assigned in place: by name, by index, through a path and with an operator. A copy is not changed.

```brix
(main
  (let p (call Point 1 2))
  (let q p)
  (= (. p x) 5)
  (= ([] p 1) 6)
  (+= (. p x) 10)
  (*= (. p y) 2)
  (print p)
  (print q)
  (let line (call Line p q))
  (= (. line start y) 7)
  (-= ([] (. line end) 0) 1)
  (print line)
  (print p)
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)

(type + Line (is Struct)
  (prop + start Point)
  (prop + end Point)
)
```

```output
(15, 12)
(1, 2)
((15, 7), (0, 2))
(15, 12)
```

### A struct is passed by value: a function that assigns to its argument, or a member to its receiver, changes its own copy.

```brix
(main
  (let p (call Point 1 2))
  (print (call changed p))
  (print p)
  (let q (call (. p moved) 5))
  (print p)
  (print q)
  (= p (call (. (call (. p moved) 1) moved) 1))
  (print p)
  (print (. p sum))
  (print (call grown (call Box 1 2 3 4)))
  (print (call scaled (call Vec 1.5 2.5)))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
  (fn-do + moved Point
    (param dx I32)
    (= x (+ x dx))
    (return this)
  )
  (fn-expr + sum _ :get
    (+ x y)
  )
)

(fn-do + changed I32
  (param v Point)
  (= (. v x) 50)
  (return (. v x))
)

(type + Box (is Struct)
  (prop + a I64)
  (prop + b I64)
  (prop + c I64)
  (prop + d I64)
)

(fn-do + grown Box
  (param v Box)
  (= (. v d) (+ (. v d) 100))
  (return v)
)

(type + Vec (is Struct)
  (prop + x F64)
  (prop + y F64)
)

(fn-expr + scaled Vec
  (param v Vec)
  (call Vec (* (. v x) 2.0) (* (. v y) 2.0))
)
```

```output
50
(1, 2)
(1, 2)
(6, 2)
(3, 2)
5
(1, 2, 3, 104)
(3, 5)
```

### A struct is made with the defaults of the fields left out, and one inside it with its own.

```brix
(main
  (print (call Point))
  (print (call Point 7))
  (print (call Point (:y 9)))
  (print (call Line))
  (print (call Line (call Point 5 6)))
  (print (call Colored 1 2 "red"))
  (print (call Colored 1))
  (let c (call Colored 3 4 "blue"))
  (let p (as c Point))
  (= (. p x) 8)
  (print p)
  (print c)
)

(type + Point (is Struct)
  (prop + x I32 (= 1))
  (prop + y I32 (= 2))
)

(type + Line (is Struct)
  (prop + start Point)
  (prop + end Point)
)

(type + Colored (is Point)
  (prop + color _ (= "none"))
)
```

```output
(1, 2)
(7, 2)
(1, 9)
((1, 2), (1, 2))
((5, 6), (1, 2))
(1, 2, red)
(1, 2, none)
(8, 4)
(3, 4, blue)
```

### A struct that holds text is kept alive in a local, as an argument and as a result while memory is collected.

```brix
(main
  (let total 0)
  (let i 0)
  (while (< i 20000)
    (let pair (tuple (call name i) (call name (+ i 1))))
    (let kept (call keep pair))
    (let junk (call name (+ i 2)))
    (if (and (== ([] kept 0) ([] pair 0)) (== ([] kept 1) ([] pair 1)))
      (+= total 1)
    )
    (let holder (call Holder junk i (call name (+ i 3))))
    (if (and (== (. holder first) junk) (!= (. holder last) junk))
      (+= total 1)
    )
    (+= i 1)
  )
  (print total)
)

(type + Holder (is Struct)
  (prop + first String)
  (prop + count Int)
  (prop + last String)
)

(fn-expr + name String
  (param n Int)
  (str "item-" n)
)

(fn-expr + keep (tuple String String)
  (param pair (tuple String String))
  (tuple ([] pair 0) ([] pair 1))
)
```

```output
40000
```

### A struct given where its base is wanted is a copy of the fields the base has, in registers and in memory.

```brix
(main
  (let c (call Colored 1 2 3))
  (print (call both (as c Point) 7))
  (print (call both c 7))
  (print (call (. c first) 7))
  (print (call (. (as c Point) first) 7))
  (print (call pair c c))
  (let w (call Wide 1 2 3))
  (print (call wide (as w Base) 7))
  (print (call wide w 7))
  (let v (call V3 1.5 2.5 3.5))
  (print (call flat (as v V2) 7.0))
  (print (call flat v 7.0))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
  (fn-expr + first Int
    (param second Int)
    (+ (* x 1000) second)
  )
)

(type + Colored (is Point)
  (prop + color I32)
)

(type + Base (is Struct)
  (prop + x I64)
  (prop + y I64)
)

(type + Wide (is Base)
  (prop + z I64)
)

(type + V2 (is Struct)
  (prop + x F64)
  (prop + y F64)
)

(type + V3 (is V2)
  (prop + z F64)
)

(fn-expr + both Int
  (param first Point)
  (param second Int)
  (+ (* (. first x) 1000) second)
)

(fn-expr + pair Int
  (param first Point)
  (param second Point)
  (+ (* (. first x) 100) (* (. second x) 10) (. second y))
)

(fn-expr + wide I64
  (param first Base)
  (param second I64)
  (+ (* (. first x) 1000) (* (. first y) 100) second)
)

(fn-expr + flat F64
  (param v V2)
  (param k F64)
  (+ (* (. v x) 1000.0) (* (. v y) 100.0) k)
)
```

```output
1007
1007
1007
1007
112
1207
1207
1757
1757
```

### A field of a struct that nothing holds, as a struct cast to its base or one a call gives, cannot be assigned: the change would be lost.

```brix compile-error
(main
  (let c (call Colored 1 2 3))
  (= (. (as c Point) x) 9)
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)

(type + Colored (is Point)
  (prop + color I32)
)
```

```output
Exception: Assigning x changes a Point struct that nothing holds, so the change is lost; assign x where the struct is held
<no file:4>
      (= (. (as c Point) x) 9)
      ^^^^^^^^^^^^^^^^^^^^^^^^
```

### An operand that names a struct is copied before a later operand changes it; the receiver of a call is read after its arguments.

```brix
(import /string.h)

(main
  (let p (call Point 1 2))
  (print (+ p (call change &p)))
  (print p)
  (= p (call Point 1 2))
  (print (== p (call changed &p)))
  (= p (call Point 1 2))
  (print (++ (str p) (call text &p)))
  (= p (call Point 1 2))
  (print (call both p (call change &p)))
  (= p (call Point 1 2))
  (print (tuple p (call change &p)))
  (= p (call Point 1 2))
  (print (call (. p first) (call change &p)))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
  (fn-expr + first Int
    (param second Int)
    (+ (* x 1000) second)
  )
  (fn-expr + (infix this + (param n Int)) _
    (+ (* x 1000) n)
  )
  (fn-expr + (infix this == (param other _)) _
    (== x (. other x))
  )
)

(fn-expr + both Int
  (param first Point)
  (param second Int)
  (+ (* (. first x) 1000) second)
)

(fn-do + change Int
  (param target (of CPointer Point))
  (let z (call Point 42 43))
  (call C:memcpy target &z 8)
  (return 7)
)

(fn-do + changed Point
  (param target (of CPointer Point))
  (let z (call Point 42 43))
  (call C:memcpy target &z 8)
  (return (call Point 1 2))
)

(fn-do + text String
  (param target (of CPointer Point))
  (let z (call Point 42 43))
  (call C:memcpy target &z 8)
  (return "!")
)
```

```output
1007
(42, 43)
true
(1, 2)!
1007
((1, 2), 7)
42007
```

### A struct made by a constructor written in source.

```brix
(main
  (let p (call Point 3))
  (print (. p x))
  (print (. p y))
  (print (call (. p sum)))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
  (fn-do + This _
    (param n Int)
    (= x n)
    (= y (* x 2))
  )
  (fn-expr + sum Int
    (+ x y)
  )
)
```

```output
3
6
9
```

### A constructor written in source starts from the values the struct gives its fields, and may return early.

```brix
(main
  (let a (call Size 2))
  (print (. a w))
  (print (. a h))
  (print (. a label))
  (let b (call Size 0))
  (print (. b w))
  (print (. b h))
)

(type + Size (is Struct)
  (prop + w _ (= 7))
  (prop + h _ (= 9))
  (prop + label _ (= "size"))
  (fn-do + This _
    (param scale Int)
    (if (== scale 0)
      (return)
    )
    (= w (* w scale))
    (*= h scale)
  )
)
```

```output
14
18
size
7
9
```

### A struct too large for registers made by a constructor written in source.

```brix
(main
  (let r (call Rect 1 2))
  (print (. r a))
  (print (. r b))
  (print (. r c))
  (print (. r d))
  (print (call total r))
)

(fn-expr + total Int
  (param r Rect)
  (+ (. r a) (. r b) (. r c) (. r d))
)

(type + Rect (is Struct)
  (prop + a I64)
  (prop + b I64)
  (prop + c I64)
  (prop + d I64)
  (fn-do + This _
    (param x I64)
    (param y I64)
    (= a x)
    (= b y)
    (= c (+ x y))
    (= d (* c 2))
  )
)
```

```output
1
2
3
6
12
```

### A constructor written in source assigns fields that hold text.

```brix
(main
  (let n (call Named "ab"))
  (print (. n name))
  (print (. n twice))
)

(type + Named (is Struct)
  (prop + name String)
  (prop + twice String)
  (fn-do + This _
    (param text String)
    (= name text)
    (= twice (++ text text))
  )
)
```

```output
ab
abab
```

### A struct's constructor with defaults for its parameters.

```brix
(main
  (print (. (call Pt) x))
  (print (. (call Pt 4) x))
  (print (. (call Pt 4 5) y))
)

(type + Pt (is Struct)
  (prop + x Int)
  (prop + y Int)
  (fn-do + This _
    (param a Int (= 1))
    (param b Int (= 2))
    (= x a)
    (= y b)
  )
)
```

```output
1
4
5
```

### A struct made by a constructor written in source is held in a property.

```brix
(main
  (let h (call Holder))
  (print (. h p x))
  (= (. h p) (call Point 9))
  (print (. h p y))
)

(type + Holder
  (prop + p _ (= (call Point 3)))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
  (fn-do + This _
    (param n Int)
    (= x n)
    (= y (+ n 1))
  )
)
```

```output
3
10
```

### What the function of an assigning operator answers for a struct is stored where the struct is held: in a local, in a struct's field, and in a property of an object.

```brix
(main
  (let thing (call Thing 20))
  (+= thing 2)
  (print (. thing foo))
  (print (. (+= thing 3) foo))
  (print (. thing foo))
  (let pair (call Both (call Thing 1) (call Thing 2)))
  (+= (. pair second) 5)
  (print (. pair first foo))
  (print (. pair second foo))
  (let box (call Box))
  (+= (. box held) 2)
  (print (. box held foo))
  (print (+= (. box held) 3))
  (print (. box held foo))
  (call (. box grow))
  (print (. box held foo))
)

(type + Thing (is Struct)
  (prop + foo I32)
  (fn-expr + (infix this += (param other I32)) _
    (call This (* foo other))
  )
)

(type + Both (is Struct)
  (prop + first Thing)
  (prop + second Thing)
)

(type + Box
  (prop + held _ (= (call Thing 5)))
  (fn-do + grow _
    (+= held 10)
  )
)
```

```output
40
120
120
1
10
10
(30)
30
300
```

### The function of an assigning operator that answers nothing for a struct has no value to assign.

```brix crash
(main
  (let thing (call Thing 20))
  (+= thing 3)
  (print (. thing foo))
)

(type + Thing (is Struct)
  (prop + foo I32)
  (fn-do + (infix this += (param other I32)) _
    (+= foo other)
  )
)
```

```output
Exception: Expected Thing here, but this is Void, and there is no conversion between them
<no file:4>
      (+= thing 3)
      ^^^^^^^^^^^^
```

### The function of an assigning operator answers for a struct a value the struct can be assigned.

```brix crash
(main
  (let thing (call Thing 20))
  (print (+= thing 3))
  (print (. thing foo))
)

(type + Thing (is Struct)
  (prop + foo I32)
  (fn-expr + (infix this += (param other I32)) _
    (* foo other)
  )
)
```

```output
Exception: Expected Thing here, but this is I32, and there is no conversion between them
<no file:4>
      (print (+= thing 3))
             ^^^^^^^^^^^^
```

### An assigning operator whose answer is stored gives the answer it stored as its value.

```brix
(main
  (let thing (call Thing 5))
  (print (. (+= thing 3) foo))
  (print (. thing foo))
  (let box (call Box))
  (print (. (+= (. box held) 2) foo))
  (print (. box held foo))
)

(type + Thing (is Struct)
  (prop + foo I32)
  (fn-expr + (infix this += (param other I32)) _
    (call This (* foo other))
  )
)

(type + Box
  (prop + held _ (= (call Thing 5)))
)
```

```output
15
15
10
10
```

### An assigning operator reads its place after the value it is given is evaluated.

```brix
(prop + thing _ (= (call Thing 2)))
(blank)

(main
  (+= thing (call change))
  (print (. thing foo))
  (let local (call Thing 2))
  (+= local (call say (+ (. local foo) 1)))
  (print (. local foo))
)

(type + Thing (is Struct)
  (prop + foo Int)
  (fn-expr + (infix this += (param other Int)) _
    (call This (* foo other))
  )
)

(fn-do + change Int
  (= thing (call Thing 10))
  (return 3)
)

(fn-do + say Int
  (param v Int)
  (print (str "say " v))
  (return v)
)
```

```output
30
say 3
6
```

### A struct holds no content: a section with markup written in it is rejected.

```brix crash
(main
  (let thing (call Thing 3))
  (print (. thing twice))
)

(type + Thing (is Struct)
  (prop + foo I32)
  (content
    (<> This)
  )
  (fn-expr + twice _ :get
    (* foo 2)
  )
)
```

```output
Exception: Content is not supported on structs
<no file:7>
    (type + Thing (is Struct)
    ^^^^^^^^^^^^^^^^^^^^^^^^
```

### Constructors that return a value of their own from the arms of a match

```brix
(main
  (print (call Thing 10.5))
  (print (call Thing -2.0))
  (print (. (call Box 1) size))
  (print (. (call Box 5) size))
)

(type + Thing (is Struct)
  (prop + foo I32)
  (prop + bar Float)
  (fn-do + This _
    (param n Float)
    (match n
      (case ({} (< it 0.0))
        (return (call This 0 0.0))
      )
      (default
        (return (call This 1 n))
      )
    )
  )
)

(type + Box
  (prop + size _ (= 0))
  (fn-do + This _
    (param kind Int)
    (match kind
      (case 1
        (return (call small))
      )
      (default
        (return (call large kind))
      )
    )
  )
)

(type + SmallBox (is Box)
  (prop + size _ (= 1))
)

(fn-do + small Box
  (return (call SmallBox))
)

(fn-do + large Box
  (param count Int)
  (let box (call SmallBox))
  (= (. box size) (* count 10))
  (return box)
)
```

```output
(1, 10.5)
(0, 0)
1
50
```
