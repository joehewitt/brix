# Calls

```brix
(import /BxCore/tests.h)
(blank)

(main
  (let data1 (tuple 81 92 33 45 55))
  (call C:ctestStructLarge data1)
  (blank)
  (let data1n (tuple 81 92 33 45 (tuple 88 99)))
  (call C:ctestStructLargeNested data1n)
  (blank)
  (let data2 (tuple 81 92))
  (call C:ctestStructSmall1 data2)
  (blank)
  (let data3 (tuple 81 92 33 45))
  (call C:ctestStructSmall2 data3)
  (blank)
  (let data4 (tuple 1.8 9.2 3.3))
  (call C:ctestStructHFA data4)
  (call C:ctestStructHFASpill 1 2 3 4 5 6 data4)
)
```

```output
a 81 b 92 c 33 d 45 e 55
a 81 b 92 c 33 d 45
e.a 88 e.b 99
a 81 b 92
a 81 b 92 c 33 d 45
a 1.800000 b 9.200000 c 3.300000
a 1.000000 b 2.000000 c 3.000000 d 4.000000 e 5.000000 f 6.000000
a 1.800000 b 9.200000 c 3.300000
```

```brix
(import /core/table)
(blank)
(import /BxEvents/BxGCHandshake.h)
(import /core/arena)
(blank)

(main
  (let arena (call Arena 32768))
  (finally
    (inline (call (. arena release)))
  )
  (let rows (call (of Table Node) arena 2))
  (call (. rows append) (call (. Node wide) 1 10 20 30 40))
  (blank)
  (match ([] rows 0)
    (case (call .small small)
      (call show-call-cursor (. small value))
    )
    (case (call .wide wide)
      (call show-call-cursor (call direct-wide wide))
      (call show-call-cursor (call forward-wide wide))
      (call show-call-cursor (call ignore-wide wide))
      (call show-call-cursor (call conditional-wide wide true))
      (call show-call-cursor (call recursive-wide wide 2))
      (call show-call-cursor (. (call escape-wide wide) external-a))
      (call show-call-cursor (call pair-wide wide wide))
      (call show-call-cursor (call safepoint-wide wide))
      (call show-call-cursor (call direct-wide (call Wide 9 11 12 13 14)))
      (call show-call-cursor (call unsafe-wide wide rows))
    )
  )
)

(fn-do + show-call-cursor _
  (param value U64)
  (print value)
)

(fn-expr + direct-wide U64
  (param wide Wide)
  (+ (. wide external-a) (. wide external-b))
)

(fn-expr + forward-wide U64
  (param wide Wide)
  (call direct-wide wide)
)

(fn-expr + ignore-wide U64
  (param wide Wide)
  7
)

(fn-expr + conditional-wide U64
  (param wide Wide)
  (param take Bool)
  (case? take
    (. wide external-a)
  )
  (default
    0
  )
)

(fn-expr + recursive-wide U64
  (param wide Wide)
  (param count U64)
  (case? (== count 0)
    (. wide external-b)
  )
  (default
    (call recursive-wide wide (- count 1))
  )
)

(fn-expr + escape-wide Wide
  (param wide Wide)
  wide
)

(fn-expr + pair-wide U64
  (param left Wide)
  (param right Wide)
  (+ (. left external-a) (. right external-b))
)

(fn-do + safepoint-wide U64
  (param wide Wide)
  (call C:brixGCSafepoint)
  (return (. wide external-a))
)

(fn-expr + unsafe-wide U64
  (param wide Wide)
  (param rows (of Table Node))
  (before
    (do
      (call mutate-call-cursor-rows rows)
    )
  )
  (. wide external-a)
)

(fn-do + mutate-call-cursor-rows _
  (param rows (of Table Node))
  (= ([] rows 0) (call (. Node wide) 2 100 200 300 400))
)

(type + Syntax (is Tuple)
  (prop + source U64)
)

(type + Small (is Syntax)
  (prop + value U64)
)

(type + Wide (is Syntax)
  (prop + inline-a U64 @hot)
  (prop + inline-b U64 @hot)
  (prop + external-a U64)
  (prop + external-b U64)
)

(type + Node (is Union)
  (variant small Small)
  (variant wide Wide)
)
```

```output
70
70
7
30
40
30
70
30
27
30
```

```brix
(import /BxCore/tests.h)
(blank)

(main
  (print (call C:ctestStructReturnHFA))
  (print (call C:ctestStructReturnSmall1))
  (print (call C:ctestStructReturnSmall2))
  (print (call C:ctestStructReturnLarge))
)
```

```output
(1.2, 2.3, 3.4)
(1, 2)
(1, 2, 3, 4)
(1, 2, 3, 4, 5)
```

```brix
(main
  (let data1a (tuple 81 92 33 45 55))
  (call struct-pass data1a)
  (blank)
  (let data2a (tuple 81 92))
  (call struct-pass data2a)
  (call struct-pass-overflow9 1 2 3 4 5 6 7 8 data2a 9)
  (blank)
  (let data3a (tuple 81 92 33 45))
  (call struct-pass data3a)
  (call struct-pass-overflow8 1 2 3 4 5 6 7 data3a 9 10)
  (blank)
  (let data4a (tuple 1.8 9.2 3.3))
  (call struct-pass data4a)
  (call struct-pass-overflow8 1 2 3 4 5 6 7 data4a 9 10)
)

(fn-do + struct-pass _
  (param x _)
  (print x)
  (return 0)
)

(fn-do + struct-pass-overflow8 _
  (param a _)
  (param b _)
  (param c _)
  (param d _)
  (param e _)
  (param f _)
  (param g _)
  (param x _)
  (param i _)
  (param j _)
  (print (tuple a b c d e f g i j))
  (print x)
  (return 0)
)

(fn-do + struct-pass-overflow9 _
  (param a _)
  (param b _)
  (param c _)
  (param d _)
  (param e _)
  (param f _)
  (param g _)
  (param h _)
  (param x _)
  (param j _)
  (print x)
  (return 0)
)
```

```output
(81, 92, 33, 45, 55)
(81, 92)
(81, 92)
(81, 92, 33, 45)
(1, 2, 3, 4, 5, 6, 7, 9, 10)
(81, 92, 33, 45)
(1.8, 9.2, 3.3)
(1, 2, 3, 4, 5, 6, 7, 9, 10)
(1.8, 9.2, 3.3)
```

```brix
(main
  (print (call struct-return-hfa))
  (print (call struct-return-small1))
  (print (call struct-return-small2))
  (print (call struct-return-large))
)

(fn-do + struct-return-hfa _
  (return (tuple 1.8 9.2 3.3))
)

(fn-do + struct-return-small1 _
  (return (tuple 8 9))
)

(fn-do + struct-return-small2 _
  (return (tuple 8 9 10 11))
)

(fn-do + struct-return-large _
  (return (tuple 8 9 10 11 12))
)
```

```output
(1.8, 9.2, 3.3)
(8, 9)
(8, 9, 10, 11)
(8, 9, 10, 11, 12)
```

```brix
(main
  (let sink (call Sink))
  (call set-before-overflow sink 7)
  (call set-after-overflow "a" "b" "c" "d" "e" "f" "g" "h" sink)
  (print (. sink value))
)

(type + Sink
  (prop + value _ (= 0))
)

(fn-do + set-before-overflow _
  (param sink Sink)
  (param value Int)
  (= (. sink value) value)
)

(fn-do + set-after-overflow _
  (param a String)
  (param b String)
  (param c String)
  (param d String)
  (param e String)
  (param f String)
  (param g String)
  (param h String)
  (param sink Sink)
  (call set-before-overflow sink 123)
)
```

```output
123
```

```brix
(main
  (call callme 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18)
)

(fn-do + callme _
  (param a _)
  (param b _)
  (param c _)
  (param d _)
  (param e _)
  (param f _)
  (param g _)
  (param h _)
  (param i _)
  (param j _)
  (param k _)
  (param l _)
  (param m _)
  (param n _)
  (param o _)
  (param p _)
  (param q _)
  (param r _)
  (print (tuple a b c d e f g h i j k l m n o p q r))
)
```

```output
(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18)
```

```brix
(main
  (print (call foo))
)

(fn-do + foo I32
  (let abc 5)
  (return 5)
)
```

```output
5
```

```brix
(main
  (let x I32 1)
  (print (call foo x))
)

(fn-expr + foo I32
  (param i I32)
  (+ i 2)
)
```

```output
3
```

```brix
(main
  (print (call bar 1))
  (print (call bar 1.0))
)

(fn-expr + (of bar T) T
  (param b T)
  (as (+ b 2.5) T)
)
```

```output
3
3.5
```

```brix
(main
  (print (call bar 1))
  (print (call bar 1.0))
)

(fn-expr + bar T
  (param b ''T)
  (as (+ b 2.5) T)
)
```

```output
3
3.5
```

```brix crash
(main
  (print (call bar 1))
)

(fn-expr + (of bar T B) T
  (param b (of T ''B))
  (+ b 2.5)
)
```

```output
Exception: Duplicate type parameter
<no file:7>
      (param b (of T ''B))
                     ^^^
```

```brix
(main
  (let r (call (of Range Index) 0 1))
)
```

```brix
(main
  (print (call bar 1))
  (print (call bar 1.0))
)

(fn-do + (of bar T) T
  (param b T)
  (let a T (as (+ b 2.5) T))
  (return a)
)
```

```output
3
3.5
```

```brix crash
(main
  (call foo)
)
```

```output
Exception: Function "foo" not found
<no file:3>
      (call foo)
      ^^^^^^^^^^
```

```brix crash
(main
  (print foo)
)
```

```output
Exception: Name "foo" not found
<no file:3>
      (print foo)
             ^^^
```

```brix
(main
  (call foo 24)
  (call foo 24 26)
)

(fn-do + foo _
  (param a I32)
  (param b _ (= 25))
  (print a)
  (print b)
)
```

```output
24
25
24
26
```

```brix
(main
  (call foo 24)
  (call foo 24 26)
)

(fn-do + foo _
  (param a I32)
  (param b I32 (= 25))
  (print a)
  (print b)
)
```

```output
24
25
24
26
```

```brix
(main
  (call foo 1 2)
)

(fn-do + foo _
  (param a I32)
  (param b _ (= 25))
  (print a)
  (print b)
)
```

```output
1
2
```

### Unresolved type parameter

```brix
(main
  (print (call foo 24 ...))
)

(fn-do + (of foo T G) _
  (param a T)
  (let b G)
  (print a)
)
```

```output
[Binding foo]
```

```brix
(main
  (call foo 24)
  (call foo 24.5)
)

(fn-do + foo _
  (param a _)
  (param b _ (= 25))
  (print (+ a b))
)
```

```output
49
49.5
```

```brix
(main
  (print (call foo 1))
  (print (call foo 1.0))
)

(fn-expr + foo _
  (param b Int)
  (param a _ (= 10))
  (+ b a)
)

(fn-expr + foo _
  (param b Float)
  (param a _ (= 5.5))
  (+ b a)
)
```

```output
11
6.5
```

```brix
(main
  (print (call foo))
)

(fn-expr + foo _
  (param a _ (= (call bar)))
  (+ a 1)
)

(fn-expr + bar _
  42
)
```

```output
43
```

### A default cannot read a member of the method's class

```brix crash
(main
  (let holder (call Holder))
  (call (. holder show))
)

(type + Holder
  (prop + name _ (= "kept"))
  (fn-do + show _
    (param text String (= name))
    (print text)
  )
)
```

```output
Exception: Name "name" not found
<no file:10>
        (param text String (= name))
                              ^^^^
```

```brix crash
(main
  (call foo 24)
)

(fn-do + foo _
  (param a _)
  (param b _ (= 25))
  (param c _)
  (print (+ a b))
)
```

```output
Exception: Parameter "c" has no default but comes after "b", which has a default; declare required parameters first, or give "c" a default
<no file:9>
      (param c _)
      ^^^^^^^^
```

### Too many arguments

```brix crash
(main
  (call foo 24 25)
)

(fn-do + foo _
  (param a _)
  (print a)
)
```

```output
Exception: No function "foo" accepts (I64, I64); it is declared as "foo(a)"
<no file:3>
      (call foo 24 25)
      ^^^^^^^^^^^^^^^^
```

```brix
(main
  (call foo 42l)
  (call foo 34i)
)

(fn-do + foo _
  (param n I64)
  (print "i64")
)

(fn-do + foo _
  (param n I32)
  (print "i32")
)
```

```output
i64
i64
```

```brix
(main
  (print (call foo (call Foo)))
)

(blank)
(type-line + Bar)
(type-line + Foo (is Bar))

(fn-expr + foo _
  (param n Bar)
  "Bar"
)

(fn-expr + foo _
  (param n Foo)
  "Foo"
)
```

```output
Foo
```

```brix
(main
  (call Thing (call Foo))
)

(blank)
(type-line + Bar)
(type-line + Foo (is Bar))

(type + Thing
  (fn-do + This _
    (param n Bar)
    (print "Bar")
  )
  (fn-do + This _
    (param n Foo)
    (print "Foo")
  )
)
```

```output
Foo
```

### Overriding base class constructor with same signature

```brix
(main
  (call Thing (call Bar))
)

(type + Bar)

(type + ThingBase
  (fn-do + This _
    (param n Bar)
    (print "Bar")
  )
)

(type + Thing (is ThingBase)
  (fn-do + This _
    (param n Bar)
    (print "Foo")
  )
)
```

```output
Foo
```

### More specific base class constructor wins

```brix
(main
  (call Thing (call Foo))
)

(blank)
(type-line + Bar)
(type-line + Foo (is Bar))

(type + ThingBase
  (fn-do + This _
    (param n Foo)
    (print "Foo")
  )
)

(type + Thing (is ThingBase)
  (fn-do + This _
    (param n Bar)
    (print "Bar")
  )
)
```

```output
Foo
```

## Named arguments

```brix
(main
  (print (call foo 1))
)

(fn-expr + foo _
  (param a _ (= 2))
  a
)
```

```output
1
```

```brix
(main
  (print (call foo 1 (:c 1)))
)

(fn-expr + foo _
  (param a _)
  (param b _ (= 1))
  (param c _ (= 2))
  (+ a b c)
)
```

```output
3
```

```brix
(main
  (print (call foo 0 (:c 3)))
)

(fn-expr + foo _
  (param a _)
  (param b _ (= 1))
  (param c _ (= 2))
  (str a " " b " " c)
)
```

```output
0 1 3
```

```brix
(main
  (print (call foo 0 (:c 3) (:d 4)))
)

(fn-expr + foo _
  (param a _)
  (param b _ (= 1))
  (param c _ (= 2))
  (param d _ (= 3))
  (str a " " b " " c " " d)
)
```

```output
0 1 3 4
```

```brix
(main
  (print (call foo 1 (:c 1)))
)

(fn-expr + foo _
  (param a _)
  (param b _ (= 1))
  (param c _ (= 2))
  (+ a b c)
)
```

```output
3
```

```brix
(main
  (print (call foo 2 (:c 1)))
)

(fn-expr + foo _
  (param a _)
  (param b _ (= 1))
  (param c _ (= 2))
  (+ a b c)
)
```

```output
4
```

```brix crash
(main
  (print (call foo (:to 1) (:with 2)))
)

(fn-expr + foo _
  (param a _)
  (param b _ (= 1))
  (+ a b)
)
```

```output
Exception: No function "foo" accepts to I64 with I64; it is declared as "foo(a, b)"
<no file:3>
      (print (call foo (:to 1) (:with 2)))
             ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

## Storing type parameters in bindings

```brix
(main
  (let c (call (of foo F32) 1 2 ...))
  (print (call c 3))
)

(fn-expr + (of foo T1 T2) _
  (param a I32)
  (param b T1)
  (param c T2)
  (* a b c)
)
```

```output
6
```

```brix
(main
  (print (call (of foo Int) 1 3 2))
  (print (call (of foo Int) 1 2 3))
  (print (call foo 1 2.0 3.0))
  (print (call foo 1 2.0 3.0))
)

(fn-do + (of foo T T2) _
  (param a T2)
  (param b T)
  (param c T)
  (print T)
  (return (* a b c))
)
```

```output
[TypeInfo I64]
6
[TypeInfo I64]
6
[TypeInfo F64]
6
[TypeInfo F64]
6
```

```brix
(main
  (call (of foo Int) 1 3 2)
  (call foo 1 2.0 3.0)
)

(fn-do + (of foo T T2) _
  (param a T2)
  (param b T)
  (param c T)
  (print T)
)
```

```output
[TypeInfo I64]
[TypeInfo F64]
```

```brix
(main
  (let c (call (of foo F32) ...))
  (print (call (of c F32) 1))
)

(fn-expr + (of foo T1 T2) _
  (param a _)
  (as (as a T1) T2)
)
```

```output
1
```

```brix
(main
  (call foo 2)
)

(fn-do + foo _
  (param a ''A)
  (print (type-of a))
)
```

```output
[TypeInfo I64]
```

## Infer argument type from its default value (but only if not provided by caller)

```brix
(main
  (print
    (call foo
      (block
        (:a "hi")
        (:b 42)
      )
    )
  )
)

(fn-expr + foo _
  (param a String)
  (param b Int)
  (str a " = " b)
)
```

```output
hi = 42
```

```brix
(main
  (print (call Foo 5))
  (print (call Foo))
)

(type + Foo
  (prop + bar Int (= 10))
  (fn-do + This _
    (param bar (? Int))
  )
  (fn-to-expr + String
    (str "[Foo " bar "]")
  )
)
```

```output
[Foo 5]
[Foo 10]
```

```brix crash
(main
  (print (call foo))
)

(fn-expr + foo _
  5
  6
)
```

```output
Exception: A `fn-expr` body has one value, and `6` follows the value `5`. Put checks, prints, probes and `do` forms in `(before …)`, other steps in a `(do …)` there, and lazies in `(where …)`, or write `fn-do` and return with `(return value)`.
<no file:8>
      6
      ^
```

```brix crash
(main
  (print (call foo))
)

(fn-expr + foo _
  (call bar)
  6
)

(fn-do + bar _
  (print "bar")
)
```

```output
Exception: A `fn-expr` body has one value, and `6` follows the value (call …). Put checks, prints, probes and `do` forms in `(before …)`, other steps in a `(do …)` there, and lazies in `(where …)`, or write `fn-do` and return with `(return value)`.
<no file:8>
      6
      ^
```

```brix
(main
  (print (call foo))
)

(fn-do + foo _
  (print "hi")
  (return 6)
)
```

```output
hi
6
```

### Empty do functions are ok

```brix
(main
  (let foo (call Foo))
  (call (. foo act))
  (print "ok")
)

(type + Foo
  (fn-do + act _)
)
```

```output
ok
```

### Empty explicit-void do functions are ok

```brix
(main
  (let foo (call Foo))
  (call (. foo act))
  (print "ok")
)

(type + Foo
  (fn-do + act Void)
)
```

```output
ok
```

### Ability to reference params in argument defaults

```brix
(main
  (print (call (of foo Int)))
)

(fn-expr + (of foo T) _
  (param type _ (= T))
  type
)
```

```output
[TypeInfo I64]
```

### Write operation always returns left hand side, ignoring return value of override

```brix
(main
  (print (<- (call Foo) 42))
)

(type + Foo
  (fn-do + (infix this <- (param other _)) _)
)
```

```output
[Object __source__:Foo]
```

```brix
(main
  (call foo)
)

(fn-do + (of foo T) _
  (constraint = T String)
  (print T)
)
```

```output
[TypeInfo String]
```

```brix
(main
  (call foo 3.5)
  (call foo 3)
)

(fn-do + foo _
  (param n (exact Int))
  (print "!")
)

(fn-do + foo _
  (param n (similar Int))
  (print "~")
)
```

```output
~
!
```

## Variadic arguments

```brix
(main
  (call foo "hi"
    (block
      (arg 1)
      (arg 2)
      (arg 3)
    )
  )
)

(fn-do + foo _
  (param text _)
  (param ... keys (list Int))
  (print text)
  (print (. keys length))
  (print keys)
)
```

```output
hi
3
1, 2, 3
```

```brix
(main
  (call foo "hi")
)

(fn-do + foo _
  (param text _)
  (param ... keys _ (= (call (of List Int))))
  (print text)
  (print (. keys length))
  (print keys)
)
```

```output
hi
0
```

```brix
(main
  (call foo "hi"
    (block
      (arg 1)
      (arg 2.5)
      (arg 3)
    )
  )
)

(fn-do + foo _
  (param text _)
  (param ... keys _)
  (print text)
  (print (. keys length))
  (print keys)
)
```

```output
hi
3
1, 2.5, 3
```

```brix
(main
  (let f (call Foo))
  (call (. f foo) "hi"
    (block
      (arg 1)
      (arg 2)
      (arg 3)
    )
  )
)

(type + Foo
  (fn-do + foo _
    (param text _)
    (param ... keys (list Int))
    (print text)
    (print (. keys length))
    (print keys)
  )
)
```

```output
hi
3
1, 2, 3
```

```brix
(main
  (call Foo
    (block
      (arg 1)
      (arg 2)
      (arg 3)
    )
  )
)

(type + Foo
  (fn-do + This _
    (param ... keys (list Int))
    (print (. keys length))
    (print keys)
  )
)
```

```output
3
1, 2, 3
```

```brix
(main
  (let fn (=> ((: text String) (: ...keys (list Int))) (str text " " keys)))
  (print (call fn "hi" 1 2 3))
)
```

```output
hi 1, 2, 3
```

```brix crash
(main
  (call foo "hi")
)

(fn-do + foo _
  (param ... keys _)
  (param a _)
)
```

```output
Exception: Variadic arguments must be the last one
<no file:8>
      (param a _)
      ^^^^^^^^
```

```brix crash
(main
  (call foo "hi")
)

(fn-do + foo _
  (param ... keys _)
  (param ... bees _)
)
```

```output
Exception: Variadic arguments must be the last one
<no file:8>
      (param ... bees _)
      ^^^^^^^^^^^^^^^
```

### Constructor assignments select omitted owner parameters before class defaults.

```brix
(main
  (let arena (call ConstructorArena))
  (let managed (call (of ConstructorSelectedBox U32)))
  (let native (call (of ConstructorSelectedBox U32) arena))
  (print (type-of managed))
  (print (type-of native))
)

(type + ConstructorArena)

(type + (of GCConstructorBacking Value))

(type + (of ArenaConstructorBacking Value))

(type + (of ConstructorSelectedBox Value Backing)
  (constraint = Backing (of GCConstructorBacking Value))
  (fn-do + This _)
  (fn-do + This _
    (param arena ConstructorArena)
    (constraint = Backing (of ArenaConstructorBacking Value))
  )
)
```

```output
[TypeInfo ConstructorSelectedBox'U32'GCConstructorBacking'U32]
[TypeInfo ConstructorSelectedBox'U32'ArenaConstructorBacking'U32]
```

### Explicit owner arguments reject a conflicting constructor assignment.

```brix crash
(main
  (let arena (call ConstructorConflictArena))
  (call (of ConstructorConflictBox U32 (of GCConflictBacking U32)) arena)
)

(type + ConstructorConflictArena)

(type + (of GCConflictBacking Value))

(type + (of ArenaConflictBacking Value) (is Tuple))

(type + (of ConstructorConflictBox Value Backing)
  (constraint = Backing (of GCConflictBacking Value))
  (fn-do + This _
    (param arena ConstructorConflictArena)
    (constraint = Backing (of ArenaConflictBacking Value))
  )
)
```

```output
Exception: No constructor of ConstructorConflictBox matches these arguments; its declared constructors are "ConstructorConflictBox(arena ConstructorConflictArena)"
<no file:4>
      (call (of ConstructorConflictBox U32 (of GCConflictBacking U32)) arena)
                ^^^^^^^^^^^^^^^^^^^^^^
```

### Ordinary overload ranking wins before a constructor assignment selects the owner type.

```brix
(main
  (let selected (call (of RankedConstructorBox U32) 3))
  (print (type-of selected))
)

(type + (of ExactRankedBacking Value))

(type + (of SimilarRankedBacking Value))

(type + (of RankedConstructorBox Value Backing)
  (fn-do + This _
    (param value (similar Int))
    (constraint = Backing (of SimilarRankedBacking Value))
  )
  (fn-do + This _
    (param value (exact Int))
    (constraint = Backing (of ExactRankedBacking Value))
  )
)
```

```output
[TypeInfo RankedConstructorBox'U32'ExactRankedBacking'U32]
```

### Equally ranked constructors selecting different owner types are ambiguous.

```brix crash
(main
  (call (of AmbiguousConstructorBox U32) true)
)

(type + (of FirstAmbiguousBacking Value))

(type + (of SecondAmbiguousBacking Value))

(type + (of AmbiguousConstructorBox Value Backing)
  (fn-do + This _
    (param marker Bool)
    (constraint = Backing (of FirstAmbiguousBacking Value))
  )
  (fn-do + This _
    (param marker Bool)
    (constraint = Backing (of SecondAmbiguousBacking Value))
  )
)
```

```output
Exception: Ambiguous constructor
<no file:3>
      (call (of AmbiguousConstructorBox U32) true)
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### An omitted owner parameter without a constructor assignment or class default is diagnosed.

```brix crash
(main
  (call UnresolvedConstructorBox true)
)

(type + (of UnresolvedConstructorBox Backing)
  (fn-do + This _
    (param marker Bool)
  )
)
```

```output
Exception: Could not resolve parameter Backing
<no file:3>
      (call UnresolvedConstructorBox true)
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Runtime arguments infer earlier owner parameters before constructor assignments and later defaults.

```brix
(main
  (let selected (call InferredConstructorBox 5u32 (call InferenceArenaToken)))
  (print (type-of selected))
)

(type + InferenceArenaToken)

(type + (of InferredManagedBacking Value))

(type + (of InferredArenaBacking Value))

(type + (of InferredPolicy Value))

(type + (of InferredConstructorBox Value Backing Policy)
  (constraint = Backing (of InferredManagedBacking Value))
  (constraint = Policy (of InferredPolicy Value))
  (fn-do + This _
    (param value Value)
    (param token InferenceArenaToken)
    (constraint = Backing (of InferredArenaBacking Value))
  )
)
```

```output
[TypeInfo InferredConstructorBox'U32'InferredArenaBacking'U32'InferredPolicy'U32]
```

### An imported extension can contribute a factory constructor that selects an omitted owner parameter.

```brix
(main
  (let selected (call (of ExtensionConstructorBox U32) (call ExtensionConstructorToken) 7u32))
  (print (type-of selected))
  (print (. selected value))
)

(type + ExtensionConstructorToken)

(type + (of ExtensionManagedBacking Value))

(type + (of ExtensionArenaBacking Value))

(type + (of ExtensionConstructorBox Value Backing) (is Tuple)
  (constraint = Backing (of ExtensionManagedBacking Value))
  (prop + value Value)
  (fn-do + This _
    (param item Value)
    (param marker U8)
    (= value item)
  )
)

(extend + (of ExtensionConstructorBox ''Value ''Backing)
  (fn-do + This _
    (param token ExtensionConstructorToken)
    (param item Value)
    (constraint = Backing (of ExtensionArenaBacking Value))
    (return (call (of ExtensionConstructorBox Value Backing) item 0u8))
  )
)
```

```output
[TypeInfo ExtensionConstructorBox'U32'ExtensionArenaBacking'U32]
7
```

```brix
(main
  (print (call second))
)

(fn-expr + first Int
  1
)

(fn-expr _ second Int
  2
)
```

```output
2
```

```brix
(main
  (let value (tuple (as 41 Size) 9u32))
  (print (call forward value))
  (print (call reverse value))
  (print (call nested (tuple value 7u32)))
  (blank)
  (let address EntryAddress 41)
  (let generation EntryGeneration 9)
  (let typed (tuple address generation))
  (print (call forward typed))
  (print (call reverse typed))
  (print (call nested (tuple typed 7u32)))
  (blank)
  (let pointer (as 41 EntryPointer))
  (let count EntryCount 9)
  (print (call forward (tuple pointer count)))
  (print (call reverse (tuple pointer count)))
)

(type + EntryAddress (is Size))

(type + EntryGeneration (is U32))

(type + EntryPointer (is (of CPointer U8)))

(type + EntryCount (is Size))

(type + ExactEntry (is (tuple (: address Size) (: generation U32))))

(type + ConvertedEntry (is (tuple (: address (of CPointer U8)) (: generation Size))))

(type + ExactNested (is (tuple (: entry ExactEntry) (: count U32))))

(type + ConvertedNested (is (tuple (: entry ConvertedEntry) (: count U32))))

(fn-expr + forward _
  (param entry ExactEntry)
  "exact"
)

(fn-expr + forward _
  (param entry ConvertedEntry)
  "converted"
)

(fn-expr + reverse _
  (param entry ConvertedEntry)
  "converted"
)

(fn-expr + reverse _
  (param entry ExactEntry)
  "exact"
)

(fn-expr + nested _
  (param entry ExactNested)
  "exact nested"
)

(fn-expr + nested _
  (param entry ConvertedNested)
  "converted nested"
)
```

```output
exact
exact
exact nested
exact
exact
exact nested
converted
converted
```

### An integer argument takes an integer overload, and a C pointer parameter takes an integer only when asked

```brix
(main
  (let byte U8 2)
  (let unsigned U32 2)
  (let signed I32 2)
  (let wide I64 2)
  (let pointer (as (as 0 Pointer) (of CPointer U8)))
  (print (. (call Box byte) address))
  (print (. (call Box unsigned) address))
  (print (. (call Box signed) address))
  (print (. (call Box wide) address))
  (print (call pick byte))
  (print (call pick unsigned))
  (print (call pick signed))
  (print (call pick wide))
  (print (call pick pointer))
  (print (call reversed unsigned))
  (print (call reversed pointer))
  (print (call only-pointer (as 0 (of CPointer Char8))))
  (print (call only-pointer (as unsigned (of CPointer Char8))))
)

(type + Box (is Tuple)
  (prop _ native (of CPointer Char8))
  (fn-do + This _
    (param count Int (= 1))
    (print (str "public " count))
    (return (call This (as (as 0 Pointer) (of CPointer Char8))))
  )
  (fn-do _ This _
    (param native (of CPointer Char8))
  )
  (fn-expr + address _ :get
    (. native address)
  )
)

(fn-expr + pick _
  (param value Int)
  "integer"
)

(fn-expr + pick _
  (param value (of CPointer Char8))
  "pointer"
)

(fn-expr + reversed _
  (param value (of CPointer Char8))
  "pointer"
)

(fn-expr + reversed _
  (param value Int)
  "integer"
)

(fn-expr + only-pointer _
  (param value (of CPointer Char8))
  (. value address)
)
```

```output
public 2
0
public 2
0
public 2
0
public 2
0
integer
integer
integer
integer
pointer
integer
pointer
0
2
```

```brix
(main
  (print (call choose (tuple 1 2)))
  (print (call choose (tuple 1 (=> () 2))))
  (print (call choose (tuple 1 produce)))
  (let chooser (call PairChooser))
  (print (call (. chooser choose) (tuple 1 2)))
  (print (call (. chooser choose) (tuple 1 (=> () 2))))
  (print (call (. chooser choose) (tuple 1 produce)))
)

(type + (of ValuePair Expected Actual) (is Tuple)
  (prop + expected Expected)
  (prop + actual Actual)
)

(type + (of LazyPair Expected Actual) (is Tuple)
  (prop + expected Expected)
  (prop + actual (of Getter Actual))
)

(fn-expr + (of choose Expected Actual) _
  (param pair (of ValuePair Expected Actual))
  "value"
)

(fn-expr + (of choose Expected Actual) _
  (param pair (of LazyPair Expected Actual))
  "lazy"
)

(type + PairChooser
  (fn-expr + (of choose Expected Actual) _
    (param pair (of ValuePair Expected Actual))
    "value"
  )
  (fn-expr + (of choose Expected Actual) _
    (param pair (of LazyPair Expected Actual))
    "lazy"
  )
)

(fn-expr + produce _
  2
)
```

```output
value
lazy
lazy
value
lazy
lazy
```

```brix
(main
  (print (call inline-add 2))
  (print (call inline-add 5))
)

(fn-expr _ inline-add Int @inline
  (param value Int)
  (+ value 1)
)
```

```output
3
6
```

```brix
(main
  (print (call score32 (call roundtrip32 (call Quad32 (call Pair32 1 2) (call Pair32 3 4)))))
  (print (call pressured32 1 2 3 4 5 6 7 (call Quad32 (call Pair32 1 2) (call Pair32 3 4))))
  (print (call score64 (call roundtrip64 (call Quad64 (call Pair64 1 2) (call Pair64 3 4)))))
  (print (call pressured64 1 2 3 4 5 6 7 (call Quad64 (call Pair64 1 2) (call Pair64 3 4))))
)

(type + Pair32 (is (tuple (: x F32) (: y F32))))

(type + Quad32 (is (tuple (: a Pair32) (: b Pair32))))

(fn-expr _ roundtrip32 Quad32
  (param input Quad32)
  input
)

(fn-expr _ score32 F32
  (param input Quad32)
  (+ (. input a x) (* (. input a y) 10) (* (. input b x) 100) (* (. input b y) 1000))
)

(fn-expr _ pressured32 F32
  (param a F32)
  (param b F32)
  (param c F32)
  (param d F32)
  (param e F32)
  (param f F32)
  (param g F32)
  (param input Quad32)
  (+ (call score32 input) a b c d e f g)
)

(type + Pair64 (is (tuple (: x F64) (: y F64))))

(type + Quad64 (is (tuple (: a Pair64) (: b Pair64))))

(fn-expr _ roundtrip64 Quad64
  (param input Quad64)
  input
)

(fn-expr _ score64 F64
  (param input Quad64)
  (+ (. input a x) (* (. input a y) 10) (* (. input b x) 100) (* (. input b y) 1000))
)

(fn-expr _ pressured64 F64
  (param a F64)
  (param b F64)
  (param c F64)
  (param d F64)
  (param e F64)
  (param f F64)
  (param g F64)
  (param input Quad64)
  (+ (call score64 input) a b c d e f g)
)
```

```output
4321
4349
4321
4349
```

### Two definitions of a function with the same signature are rejected.

```brix crash
(main
  (print (call shape))
)

(fn-expr + shape Int
  1
)

(fn-expr + shape Int
  2
)
```

```output
Exception: Duplicate definition of shape with the same signature
<no file:10>
    (fn-expr + shape Int
    ^^^^^^^^^^^^^^^^^^^
```

### Overloads that differ by argument type remain legal.

```brix
(main
  (print (call describe 1))
  (print (call describe "one"))
)

(fn-expr + describe String
  (param value Int)
  "int"
)

(fn-expr + describe String
  (param value String)
  "string"
)
```

```output
int
string
```

### An aggregate operand keeps its value when a later operand changes its storage.

```brix
(prop + g (? Int) (= 5))
(prop + h (tuple Int Int) (= (tuple 1 2)))
(blank)

(main
  (let box (call Box))
  (print (call show (. box value) (call (. box clear))))
  (print (call show g (call reset)))
  (= g 5)
  (print (tuple g (call reset)))
  (= g 5)
  (print (== g (call reset-to 6)))
  (= g 5)
  (print (|> g (call trail (call reset))))
  (print (call sum h (call bump)))
)

(type + Box
  (prop + value (? Int) (= 5))
  (fn-do + clear Int
    (= value null)
    (return 0)
  )
)

(fn-do + reset Int
  (= g null)
  (return 0)
)

(fn-do + reset-to (? Int)
  (param value Int)
  (= g value)
  (return value)
)

(fn-do + bump Int
  (= h (tuple 10 20))
  (return 0)
)

(fn-expr + sum Int
  (param pair (tuple Int Int))
  (param extra Int)
  (+ ([] pair 0) ([] pair 1) extra)
)

(fn-expr + show String
  (param value (? Int))
  (param extra Int (= 0))
  (str value " " extra)
)

(fn-expr + trail String
  (param extra Int)
  (param value (? Int))
  (str value " " extra)
)
```

```output
5 0
5 0
(5, 0)
false
5 0
3
```

### A method call evaluates its receiver before its arguments, and a skipped optional call evaluates none.

```brix
(prop + box _ (= (call Step "first")))
(blank)

(main
  (print (call (. box then) (call swap)))
  (print (. (call (. (call (. (call step "a") then) (call step "x")) then) (call step "y")) name))
  (let none (? Step) null)
  (print (call (?. none then) (call step "skipped")))
)

(fn-do + swap Step
  (= box (call Step "second"))
  (return (call Step "x"))
)

(fn-do + step Step
  (param name String)
  (print (str "eval " name))
  (return (call Step name))
)

(type + Step
  (prop + name String)
  (fn-do + This _
    (param name String)
    (= (. this name) name)
  )
  (fn-do + then Step
    (param next Step)
    (print (str "call " name " then " (. next name)))
    (return next)
  )
)
```

```output
call first then x
[Object __source__:Step]
eval a
eval x
call a then x
eval y
call x then y
y
unknown
```

### An optional argument that holds no value takes the default its parameter declares.

```brix
(main
  (let none (? I64))
  (let some (? I64) 5)
  (print (call pick 1 none))
  (print (call pick 1 some))
  (print (call pick 1))
  (print (call maybe 1 none))
  (print (call maybe 1 some))
  (print (call maybe 1))
)

(fn-do + pick I64
  (param a I64)
  (param b I64 (= 7))
  (return (+ a b))
)

(fn-do + maybe I64
  (param a I64)
  (param b (? I64) (= 9))
  (return (+ a (?? b 100)))
)
```

```output
8
6
8
10
6
10
```

### An optional argument given to a constructor parameter that writes a property of a class type takes the parameter's default where it holds no value.

```brix
(main
  (let none (? (list Int)))
  (let some (? (list Int)) (list 1 2))
  (print (. (call Basket none) items length))
  (print (. (call Basket some) items length))
  (print (. (call Basket) items length))
)

(type + Basket
  (prop + items (list Int))
  (fn-do + This _
    (param items _ (= (list 9 9 9)))
  )
)
```

```output
3
2
3
```

### The first eight integers and the first eight floats are passed in registers, counted apart, and the rest on the stack.

```brix
(main
  (print (call sum10 1 2 3 4 5 6 7 8 9 10))
  (print (call fsum10 1.0 2.0 3.0 4.0 5.0 6.0 7.0 8.0 9.0 10.5))
  (print (call mixed 1 2.5 3 4.5 5 6.5 7 8.5 9 10.5 11 12.5))
  (print (call texts "a" "b" "c" "d" "e" "f" "g" "h" "i" "j"))
)

(fn-expr + sum10 Int
  (param a Int)
  (param b Int)
  (param c Int)
  (param d Int)
  (param e Int)
  (param f Int)
  (param g Int)
  (param h Int)
  (param i Int)
  (param j Int)
  (+ a (* b 2) c d e f g h (* i 3) j)
)

(fn-expr + fsum10 Float
  (param a Float)
  (param b Float)
  (param c Float)
  (param d Float)
  (param e Float)
  (param f Float)
  (param g Float)
  (param h Float)
  (param i Float)
  (param j Float)
  (+ a b c d e f g h (* i 2) j)
)

(fn-expr + mixed Float
  (param a Int)
  (param b Float)
  (param c Int)
  (param d Float)
  (param e Int)
  (param f Float)
  (param g Int)
  (param h Float)
  (param i Int)
  (param j Float)
  (param k Int)
  (param l Float)
  (+ a b c d e f g h i j k l)
)

(fn-do + texts String
  (param a String)
  (param b String)
  (param c String)
  (param d String)
  (param e String)
  (param f String)
  (param g String)
  (param h String)
  (param i String)
  (param j String)
  (++= i "!")
  (return (++ a h i j))
)
```

```output
75
64.5
81
ahi!j
```

### An argument and a result are numbers of any width: each widens to the type its parameter declares, and narrows to it when asked.

```brix
(main
  (let x 5)
  (let y I64 7)
  (print (call add8 100 27))
  (print (call wide x))
  (print (call narrow (as y U8)))
  (print (call f32 1.5))
  (print (call both true false))
)

(fn-expr + add8 I8
  (param a I8)
  (param b I8)
  (+ a b)
)

(fn-expr + wide I64
  (param n I64)
  (* n 2)
)

(fn-expr + narrow U8
  (param n U8)
  (+ n 250)
)

(fn-expr + f32 F32
  (param n F32)
  (* n 2)
)

(fn-expr + both Bool
  (param a Bool)
  (param b Bool)
  (and a (not b))
)
```

```output
127
10
1
3
true
```

### Arguments are evaluated in the order they are written, also when they are given by name.

```brix
(main
  (print (call add (call show 1) (call show 2)))
  (print (call add (call show 3) (call add (call show 4) (call show 5))))
  (print
    (call pair
      (block
        (:b (call show 6))
        (:a (call show 7))
      )
    )
  )
)

(fn-expr + add Int
  (param a Int)
  (param b Int)
  (+ a b)
)

(fn-expr + pair Int
  (param a Int)
  (param b Int)
  (+ (* a 10) b)
)

(fn-do + show Int
  (param n Int)
  (print n)
  (return n)
)
```

```output
1
2
3
3
4
5
12
6
7
76
```

### A call is a statement, a value, a condition, an argument, and the bound of a loop.

```brix
(main
  (let n 3)
  (call note n)
  (call note (call add n (call add 1 2)))
  (if (call big n)
    (print "big")
    (else-if (call small n)
      (print "small")
    )
  )
  (while (call below n 6)
    (+= n 1)
  )
  (for (in i (..< 0 (call add 1 2)))
    (call note i)
  )
  (print n)
)

(fn-do + note _
  (param n Int)
  (print n)
)

(fn-expr + add Int
  (param a Int)
  (param b Int)
  (+ a b)
)

(fn-expr + big Bool
  (param n Int)
  (> n 10)
)

(fn-expr + small Bool
  (param n Int)
  (< n 5)
)

(fn-expr + below Bool
  (param n Int)
  (param limit Int)
  (< n limit)
)
```

```output
3
6
small
0
1
2
6
```

### An argument is a local of its function, and an assignment to it stays there.

```brix
(main
  (let n 1)
  (let s "a")
  (print (call bump n))
  (print (call grow s))
  (print n)
  (print s)
)

(fn-do + bump Int
  (param n Int)
  (+= n 1)
  (= n (* n 10))
  (return n)
)

(fn-do + grow String
  (param s String)
  (++= s "b")
  (return s)
)
```

```output
20
ab
1
a
```

### A function of one name is chosen by the types of its arguments.

```brix
(main
  (print (call describe 1))
  (print (call describe "one"))
  (print (call describe 1.5))
  (call width 42l)
  (call width 34i)
)

(fn-expr + describe String
  (param value Int)
  "int"
)

(fn-expr + describe String
  (param value String)
  "string"
)

(fn-expr + describe String
  (param value Float)
  "float"
)

(fn-do + width _
  (param n I64)
  (print "i64")
)

(fn-do + width _
  (param n I32)
  (print "i32")
)
```

```output
int
string
float
i64
i64
```

### A parameter with no type, or with a type parameter, makes a function for each type it is called with.

```brix
(main
  (print (call plus 1))
  (print (call plus 1.5))
  (print (call same 1))
  (print (call same 1.0))
)

(fn-expr + plus _
  (param a _)
  (+ a 1)
)

(fn-expr + (of same T) T
  (param b T)
  (as (+ b 2.5) T)
)
```

```output
2
2.5
3
3.5
```

### A pipe gives its left side to the function on its right as the first argument.

```brix
(main
  (print (|> 5 double))
  (print (|> 5 (call add 2)))
  (print (|> 5 double (call add 1)))
)

(fn-expr + double Int
  (param n Int)
  (* n 2)
)

(fn-expr + add Int
  (param a Int)
  (param b Int)
  (+ a b)
)
```

```output
10
7
11
```

### A function is called as an operator between slashes, and one named as a bitwise operator replaces it.

```brix
(main
  (print (/and/ 6 3))
  (print (/mix/ 6 3))
  (print (/xor/ 6 3))
)

(fn-expr + mix Int
  (param a Int)
  (param b Int)
  (+ (* a 10) b)
)

(fn-expr + xor Int
  (param a Int)
  (param b Int)
  (- a b)
)
```

```output
2
63
3
```

### `@inline` functions and those `without gc` or `without heap` are called as any other.

```brix
(main
  (print (call inline-add 2))
  (print (call square 4))
  (print (call cube 3))
  (print (call through 5))
)

(fn-expr _ inline-add Int @inline
  (param value Int)
  (+ value 1)
)

(fn-expr + square Int (without gc)
  (param n Int)
  (* n n)
)

(fn-do + cube Int (without heap safepoints)
  (param n Int)
  (let t (* n n))
  (return (* t n))
)

(fn-do + through Int (without gc)
  (param n Int)
  (return (+ (call square n) 1))
)
```

```output
3
16
27
26
```

### A function calls one declared after it, through as many calls as it takes.

```brix
(main
  (print (call a 1))
)

(fn-expr + a Int
  (param n Int)
  (+ (call b n) 1)
)

(fn-expr + b Int
  (param n Int)
  (+ (call c n) 1)
)

(fn-do _ c Int
  (param n Int)
  (return (* n 100))
)
```

```output
102
```

### A call a value is piped into evaluates the value first, then the arguments written, and its receiver last.

```brix
(main
  (print (call (. (call show 2) plus) (call show 3) (call show 4)))
  (print (|> (call show 1) (call (. (call show 2) plus) (call show 3))))
  (print (|> (call show 8) (call two (call show 9))))
  (print (|> (+ (call show 1) (call show 2)) (call (. (call show 3) plus) (call show 4))))
)

(fn-do + show Int
  (param n Int)
  (print n)
  (return n)
)

(fn-expr + two Int
  (param a Int)
  (param b Int)
  (+ (* a 10) b)
)

(extend + Int
  (fn-expr + plus Int
    (param a Int)
    (param b Int)
    (+ (* this 100) (* a 10) b)
  )
)
```

```output
2
3
4
234
1
3
2
231
8
9
98
1
2
4
3
343
```

### A parameter with no type written that is given `null` keeps its place, and the arguments after it arrive as given.

```brix
(main
  (let a (call Node null 3))
  (print (tuple (. a value) (== (. a next) null)))
  (let c (call Node a 6))
  (print (tuple (. c value) (. (!! (. c next)) value)))
  (print (call pick null 4 5))
  (let nothing null)
  (print (call pick nothing 6 7))
  (print (. (call Node nothing 8) value))
)

(fn-expr + pick _
  (param first _)
  (param second _)
  (param third _)
  (+ (* second 10) third)
)

(type + Node
  (prop + next (? Node))
  (prop + value I32)
  (fn-do + This _
    (param next _)
    (param value _)
  )
)
```

```output
(3, true)
(6, 3)
45
67
8
```

### A function with a variadic tail and one with a fixed argument, each declared first: a number that converts to either.

```brix
(main
  (print (call first 5))
  (print (call second 5))
  (print (call first 5 6))
  (print (call second 5 6))
  (let n I32 5)
  (print (call first n))
  (print (call second n))
)

(fn-do _ first Int
  (param ... items (list Int64))
  (return (+ 100 (. items length)))
)

(fn-do _ first Int
  (param item Float)
  (return 7)
)

(fn-do _ second Int
  (param item Float)
  (return 7)
)

(fn-do _ second Int
  (param ... items (list Int64))
  (return (+ 100 (. items length)))
)
```

```output
101
7
102
102
101
7
```

### One tuple given where a variadic tail of pairs, a stream of pairs and a pair can each take it.

```brix
(main
  (print (call first (tuple "a" 1)))
  (print (call second (tuple "a" 1)))
  (print (call third (tuple "a" 1)))
  (print (call fourth (tuple "a" 1)))
)

(fn-do _ first Int
  (param ... pairs (list (of KeyValuePair String Int)))
  (return (+ 100 (. pairs length)))
)

(fn-do _ first Int
  (param pairs (stream (of KeyValuePair String Int)))
  (return 7)
)

(fn-do _ second Int
  (param pairs (stream (of KeyValuePair String Int)))
  (return 7)
)

(fn-do _ second Int
  (param ... pairs (list (of KeyValuePair String Int)))
  (return (+ 100 (. pairs length)))
)

(fn-do _ third Int
  (param pair (of KeyValuePair String Int))
  (return 50)
)

(fn-do _ third Int
  (param pairs (stream (of KeyValuePair String Int)))
  (return 7)
)

(fn-do _ fourth Int
  (param pairs (stream (of KeyValuePair String Int)))
  (return 7)
)

(fn-do _ fourth Int
  (param pair (of KeyValuePair String Int))
  (return 50)
)
```

```output
101
101
50
50
```

### One tuple given where a variadic tail of pairs with a union and a stream or an array of them can each take it.

```brix
(main
  (print (call first (tuple "a" true)))
  (print (call second (tuple "a" 2)))
  (print (call first (tuple "a" true) (tuple "b" 2)))
)

(fn-do _ first Int
  (param ... pairs (list (of KeyValuePair String Value)))
  (return (+ 100 (. pairs length)))
)

(fn-do _ first Int
  (param pairs (stream (of KeyValuePair String Value)))
  (return 7)
)

(fn-do _ second Int
  (param pairs (of ImmutableArray (of KeyValuePair String Value)))
  (return 7)
)

(fn-do _ second Int
  (param ... pairs (list (of KeyValuePair String Value)))
  (return (+ 100 (. pairs length)))
)

(type + Value (is Union)
  (variant Null)
  (variant Bool)
  (variant Int)
)
```

```output
101
101
102
```

### A variadic tail and a fixed argument of one type, and of two: one argument, and two.

```brix
(main
  (print (call text "a"))
  (print (call text "a" "b"))
  (print (call number 1))
  (print (call number 1 2))
  (print (call either 3))
  (print (call either "x"))
)

(fn-do _ text String
  (param one String)
  (return (str "one " one))
)

(fn-do _ text String
  (param ... all (list String))
  (return (str "all " (. all length)))
)

(fn-do _ number String
  (param ... all (list Int))
  (return (str "all " (. all length)))
)

(fn-do _ number String
  (param one Int)
  (return (str "one " one))
)

(fn-do _ either String
  (param ... all (list Int))
  (return (str "numbers " (. all length)))
)

(fn-do _ either String
  (param one String)
  (return (str "text " one))
)
```

```output
one a
all 2
one 1
all 2
numbers 1
text x
```

### A variadic tail after an untyped argument ranks by what its arguments convert to, against fixed arguments, each declared first.

```brix
(main
  (print (call first 1 2))
  (print (call second 1 2))
)

(fn-do _ first String
  (param name _)
  (param ... rest (list String))
  (return "variadic")
)

(fn-do _ first String
  (param count Int)
  (param other (? Int))
  (return "fixed")
)

(fn-do _ second String
  (param count Int)
  (param other (? Int))
  (return "fixed")
)

(fn-do _ second String
  (param name _)
  (param ... rest (list String))
  (return "variadic")
)
```

```output
fixed
fixed
```

### An overload that types its parameter is chosen over one that leaves it untyped

```brix
(main
  (call fetch (call Thing))
  (call fetch (call Bling))
)

(type + Thing
  (implements IFoo)
)

(type + Bling)

(type + IFoo (is Interface))

(fn-do + fetch _
  (param thing IFoo)
  (print "duck")
)

(fn-do + fetch _
  (param thing _)
  (print "fallthrough")
)
```

```output
duck
fallthrough
```

### Overloads that each leave a different parameter untyped

```brix
(main
  (call fetch (call Thing) 1)
  (call fetch (call Bling) 2)
  (call fetch (call Thing) "a")
)

(type + Thing
  (implements IFoo)
)

(type + Bling)

(type + IFoo (is Interface))

(fn-do + fetch _
  (param thing IFoo)
  (param count _)
  (print "duck")
)

(fn-do + fetch _
  (param thing _)
  (param count Int)
  (print "fallthrough")
)
```

```output
duck
fallthrough
duck
```

### Methods that leave a parameter untyped take the call from the ones before them

```brix
(main
  (let box (call Box))
  (call (. box put) (call Thing) 1)
  (call (. box put) (call Bling) 2)
  (call (. box put) (call Thing) "a")
  (call (. box put) 1 2)
)

(type + Thing
  (implements IFoo)
)

(type + Bling)

(type + IFoo (is Interface))

(type + Box
  (fn-do + put _
    (param thing IFoo)
    (param count _)
    (print "duck")
  )
  (fn-do + put _
    (param thing _)
    (param count Int)
    (print "fallthrough")
  )
  (fn-do + put _
    (param thing Int)
    (param count Int)
    (print "ints")
  )
)
```

```output
fallthrough
fallthrough
duck
ints
```

### An untyped parameter keeps what an earlier overload compared its argument to

```brix
(main
  (call show (call Thing) (call Thing))
  (call show (call Thing) 1)
  (call show 1 (call Thing))
  (call show 1 1)
)

(type + Thing
  (implements IFoo)
)

(type + IFoo (is Interface))

(fn-do + show _
  (param a _)
  (param b _)
  (print "any any")
)

(fn-do + show _
  (param a IFoo)
  (param b _)
  (print "foo any")
)

(fn-do + show _
  (param a _)
  (param b IFoo)
  (print "any foo")
)

(fn-do + show _
  (param a Thing)
  (param b Thing)
  (print "thing thing")
)
```

```output
any any
any any
any any
any any
```

### Overloads with untyped parameters declared most specific first

```brix
(main
  (call show (call Thing) (call Thing))
  (call show (call Thing) 1)
  (call show 1 (call Thing))
  (call show 1 1)
)

(type + Thing
  (implements IFoo)
)

(type + IFoo (is Interface))

(fn-do + show _
  (param a Thing)
  (param b Thing)
  (print "thing thing")
)

(fn-do + show _
  (param a _)
  (param b IFoo)
  (print "any foo")
)

(fn-do + show _
  (param a IFoo)
  (param b _)
  (print "foo any")
)

(fn-do + show _
  (param a _)
  (param b _)
  (print "any any")
)
```

```output
thing thing
foo any
any foo
any any
```

### Constructors with a typed and an untyped parameter

```brix
(main
  (let made (call Maker 1))
  (let other (call Maker "a"))
  (let third (call Maker (call Thing)))
  (print (. made tag))
  (print (. other tag))
  (print (. third tag))
)

(type + Thing
  (implements IFoo)
)

(type + IFoo (is Interface))

(type + Maker
  (prop + tag String)
  (fn-do + This _
    (param value IFoo)
    (lazy tag "foo")
  )
  (fn-do + This _
    (param value _)
    (lazy tag "any")
  )
  (fn-do + This _
    (param value Int)
    (lazy tag "int")
  )
)
```

```output
int
any
any
```

### A function whose types fit the arguments is chosen over a generic one, compared under what the arguments bind its type parameter to.

```brix
(main
  (call show "a" 2)
  (call show 1 2)
  (call pick 1.5 2)
  (call pick 1 2)
  (let shower (call Shower))
  (call (. shower show) "a" 2)
  (call (. shower show) 1 2)
)

(fn-do + (of show T) _
  (param a T)
  (param b T)
  (print (str "(generic, " a ", " b ")"))
)

(fn-do + show _
  (param a String)
  (param b Int)
  (print (str "(plain, " a ", " b ")"))
)

(fn-do + (of pick T) _
  (param a T)
  (param b T)
  (print "same")
)

(fn-do + pick _
  (param a Float)
  (param b Int)
  (print "float int")
)

(type + Shower
  (fn-do + (of show T) _
    (param a T)
    (param b T)
    (print (str "(method generic, " a ", " b ")"))
  )
  (fn-do + show _
    (param a String)
    (param b Int)
    (print (str "(method plain, " a ", " b ")"))
  )
)
```

```output
(plain, a, 2)
(generic, 1, 2)
float int
same
(method plain, a, 2)
(method generic, 1, 2)
```

### A constructor default that builds an object is built again for each call.

```brix
(main
  (let first (call Bag))
  (+= (. first items) "a")
  (let second (call Bag))
  (+= (. second items) "b")
  (print (. first items length))
  (print (. second items length))
)

(type + Bag
  (prop + items (list String))
  (fn-do + This _
    (param items (list String) (= (call (of List String))))
  )
)
```

```output
1
1
```

### A function default that builds an object is built again for each call.

```brix
(main
  (print (. (call grown) length))
  (print (. (call grown) length))
)

(fn-do + grown _
  (param items (list String) (= (call (of List String))))
  (+= items "a")
  (return items)
)
```

```output
1
1
```

### An argument default is evaluated only by a call that omits the argument.

```brix
(main
  (print (str "start " (. counter count)))
  (print (call next-id))
  (print (call next-id 10))
  (print (call next-id))
  (print (. counter count))
  (print (call level))
  (= level-floor 9)
  (print (call level))
  (print (call side))
  (print (call side .left))
  (print (call side (. Side right)))
)

(blank)
(prop + counter _ (= (call Counter)))
(prop + level-floor _ (= 3))

(type + Counter
  (prop + count _ (= 0))
)

(type + Side
  (variant left)
  (variant right)
)

(fn-do + next-id _
  (param id Int (= (call bump)))
  (return id)
)

(fn-do + bump Int
  (+= (. counter count) 1)
  (return (. counter count))
)

(fn-expr + level _
  (param floor _ (= level-floor))
  (+ floor 1)
)

(fn-expr + side _
  (param which Side (= (. Side right)))
  (if-expr (== which (. Side left)) "left" "right")
)
```

```output
start 0
1
10
2
2
4
10
right
left
right
```

```brix
(main
  (let a Circ (tuple (call Point 1 2) 3))
  (let b Circ (tuple (call Point 4 5) 6))
  (let c Circ (tuple (call Point 7 8) 9))
  (call show a b)
  (call show a b c 10)
)

(type + Point (is Tuple)
  (prop + x Float)
  (prop + y Float)
)

(type + Circ (is (tuple (: pt Point) (: radius Float))))

(fn-do + show _
  (param a Circ)
  (param b Circ)
  (print a)
  (print b)
)

(fn-do + show _
  (param a Circ)
  (param b Circ)
  (param c Circ)
  (param last Float)
  (print a)
  (print b)
  (print c)
  (print last)
)
```

```output
((1, 2), 3)
((4, 5), 6)
((1, 2), 3)
((4, 5), 6)
((7, 8), 9)
10
```

### A scalar overload and a labelled stream overload of one name resolve side by side.

```brix
(main
  (print (call least 4 2))
  (print (call least (list 5 3 4)))
)

(fn-expr + (of least Num) _
  (param n1 Num)
  (param n2 Num)
  (case? (< n2 n1)
    n2
  )
  (default
    n1
  )
)

(fn-do + (of least Item) _
  (param stream (stream Item))
  (let result (call Item))
  (let found false)
  (for (in item stream)
    (if (or (not found) (< item result))
      (= result item)
      (= found true)
    )
  )
  (return result)
)
```

```output
2
3
```

### An untyped argument with a literal default takes the type of what a call gives.

```brix
(main
  (call scaled 1)
  (call scaled)
  (call scaled "s")
  (print (. (!! (. (call Node "leaf" (call Node "root")) parent)) name))
)

(fn-do + scaled _
  (param x _ (= 0.5))
  (print x)
)

(type + Node
  (prop + name String)
  (prop + parent (? Node))
  (fn-do + This _
    (param name _)
    (param parent _ (= null))
  )
)
```

```output
1
0.5
s
root
```

### A default is cast to the type its parameter writes.

```brix
(main
  (print (call plain))
)

(fn-expr + plain String
  (param b Int (= 0.5))
  (str b)
)
```

```output
0
```

### A symbol given for an untyped argument whose default is a symbol names a case of the default's type.

```brix
(main
  (call faced .right)
  (call faced)
  (call faced 5)
)

(type + Side
  (variant left)
  (variant right)
)

(fn-do + faced _
  (param side _ (= .left))
  (print side)
)
```

```output
[Object __source__:Side]
[Object __source__:Side]
5
```

### Type arguments a call writes rule out an overload that declares no type parameters, though one of its arguments has no type written.

```brix
(main
  (print (call (of (. (call Reader) read) U32) (. Endian big)))
)

(type + Reader
  (prop + count Int (= 0))
  (fn-do + (of read Num) (? Num)
    (param endian _ (= .big))
    (constraint is Num Number)
    (return (as 7 Num))
  )
  (fn-do + read (? String)
    (param terminal String)
    (param limit _ (= (. Size max)))
    (return "until")
  )
)
```

```output
7
```

### A default no call takes is not typed for the instance, which it does not fit.

```brix
(main
  (let items (call (of List Foo) (list (call Foo 2) (call Foo 1) (call Foo 3))))
  (print (?. (call (of pick Foo) items by-n) n))
)

(type + Foo
  (prop + n Int)
  (fn-do + This _
    (param n Int)
  )
)

(fn-expr + by-n Bool
  (param a Foo)
  (param b Foo)
  (< (. a n) (. b n))
)

(type + (of Better Item) (is (fn-type Bool (param a Item) (param b Item))))

(fn-do + (of pick Item) (? Item)
  (param stream (list Item))
  (param better (of Better Item) (= basic-better))
  (let best (? Item) null)
  (for (in item stream)
    (if (or (not best) (call better item best))
      (= best item)
    )
  )
  (return best)
)

(fn-expr + basic-better _
  (param a _)
  (param b _)
  (< (. a length) (. b length))
)
```

```output
1
```

### A call of a type name that names nothing finds no constructor

```brix crash
(main
  (let items (list (call (of Missing Int) "y")))
  (print (. items length))
)
```

```output
Exception: Type "Missing" not found
<no file:3>
      (let items (list (call (of Missing Int) "y")))
                                 ^^^^^^^
```

### A pipe gives its left side to the constructor of a generic class on its right, which infers the class's type parameters from it.

```brix
(main
  (let box (|> (list "a" "b") Box))
  (print (. box value))
)

(type + (of Box Item)
  (prop + value (list Item))
  (fn-do + This _
    (param value (list Item))
  )
)
```

```output
a, b
```

### A method called by its name alone with arguments it does not take is reported as a method of its class.

```brix crash
(main
  (let b (call Button))
  (call (. b describe))
)

(type + Button
  (fn-do + describe _
    (call note 5)
  )
  (fn-do + note _
    (param text String)
    (print text)
  )
)
```

```output
Exception: No method "note" of Button accepts (I64); it is declared as "note(text String)"
<no file:9>
        (call note 5)
        ^^^^^^^^^^^^^
```

### A constructor given bullets it does not take is reported with them, and a field declared by its default is shown with it.

```brix crash
(main
  (let b1 129u8)
  (let packet (call Packet b1 (:is-fin b1)))
  (print (. packet is-fin))
)

(type + Packet (is Tuple)
  (prop + value U8)
  (prop + is-fin _ (= true))
)
```

```output
Exception: No constructor of Packet accepts (U8) is-fin U8; its declared constructors are "Packet(value U8, is-fin = true)"
<no file:4>
      (let packet (call Packet b1 (:is-fin b1)))
                        ^^^^^^
```
