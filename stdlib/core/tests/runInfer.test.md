# Infer

```brix
(main
  (let a 1)
  (print a)
)
```

```output
1
```

### A do block of void calls does not suppress the final expression

```brix
(main
  (print (call foo))
)

(fn-do + touch _)

(fn-expr + foo Int
  (before
    (do
      (call touch)
    )
  )
  42
)
```

```output
42
```

### Infer return type from argument

```brix
(main
  (print (call foo 1))
  (print (call foo 1.5))
)

(fn-expr + foo _
  (param a _)
  a
)
```

```output
1
1.5
```

### Infer return type from type arguments

```brix
(main
  (let thing (call bar))
  (print (call foo thing))
)

(fn-expr + (of foo T) _
  (param t (of Thing T))
  (. t a)
)

(fn-expr + bar _
  (call (of Thing Float))
)

(type + (of Thing T)
  (prop + a T (= 0))
)
```

```output
0
```

### Infer return type with multiple returns of same type

```brix
(main
  (print (call foo 2))
  (print (call foo 0))
)

(fn-expr + foo _
  (param x _)
  (case? (> x 1)
    1
  )
  (default
    2
  )
)
```

```output
1
2
```

```brix
(main
  (print (call foo 2))
  (print (call foo 0))
)

(fn-do + foo _
  (param x _)
  (while (> x 1)
    (return 1)
  )
  (blank)
  (return 2)
)
```

```output
1
2
```

### Infer optional types from functions that do not always return

```brix
(main
  (print (call foo 1))
  (print (call foo 2))
)

(fn-do + foo _
  (param x _)
  (if (> x 1)
    (return x)
  )
)
```

```output
unknown
2
```

### A conditional return of an optional makes the inferred return type optional

```brix
(main
  (print (call sooner null 0))
  (print (call sooner 5 2))
  (print (call sooner 1 2))
)

(fn-do + sooner _
  (param first (? Int))
  (param position Int)
  (return-if first (if (== position 0)))
  (return-if first (if (and (!= first null) (<= first position))))
  (return position)
)
```

```output
unknown
2
1
```

### Infer union types from return values of different types

```brix
(main
  (print (call foo 2))
  (print (call foo 1))
)

(fn-do + foo _
  (param x _)
  (if (> x 1)
    (return 1)
    (else
      (return 2.5)
    )
  )
)
```

```output
1
2.5
```

### Infer nested parameter via converter

```brix
(main
  (let thing (call Thing))
  (call foo thing)
)

(type + Thing
  (fn-to-do + (of Iterable String)
    (yield "hi")
  )
)

(fn-do + (of foo T) _
  (param stream (of Iterable T))
  (print (<- stream))
)
```

```output
hi
```

### Infer from fromverter

```brix
(main
  (print (call foo (call (of Foo Int Float))))
)

(type + (of Bar F G)
  (fn-from-expr + (: foo (of Foo Int Float))
    (call (of This Int Float))
  )
)

(type + (of Foo T1 T2))

(fn-expr + foo _
  (param x (of Bar ''T ''V))
  x
)
```

```output
[Object __source__:Bar'core/number:I64'core/number:F64]
```

### Try and fail to infer return type from incompatible types

```brix crash
(main
  (print (call foo 2))
  (print (call foo 1))
)

(type + Foo)

(fn-do + foo _
  (param x _)
  (if (> x 1)
    (return (call Foo))
    (else
      (return 2.5)
    )
  )
)
```

```output
Exception: Function "foo" returns Foo here and F64 elsewhere, which have no common type; return one type, or declare the return type
<no file:12>
        (return (call Foo))
        ^^^^^^^^^^^^^^^^^^^
```

```brix
(main
  (call foo (call Thing) 42.5)
)

(type + Thing
  (fn-to-expr + Float
    99
  )
)

(fn-do + (of foo T) _
  (param a T)
  (param b T)
  (print (tuple T a b))
)
```

```output
([TypeInfo F64], 99, 42.5)
```

### Values of a type parameter that are a text and a number have no type in common.

```brix crash
(main
  (call foo "10" 42.5)
)

(fn-do + (of foo T) _
  (param a T)
  (param b T)
  (print (tuple T a b))
)
```

```output
Exception: No function "foo" accepts (String, F64); it is declared as "foo(a T, b T)"
<no file:3>
      (call foo "10" 42.5)
      ^^^^^^^^^^^^^^^^^^^^
```

### Values of a type parameter that are a Bool and a number have no type in common.

```brix crash
(main
  (call foo false 42.5)
)

(fn-do + (of foo T) _
  (param a T)
  (param b T)
  (print (tuple T a b))
)
```

```output
Exception: No function "foo" accepts (Bool, F64); it is declared as "foo(a T, b T)"
<no file:3>
      (call foo false 42.5)
      ^^^^^^^^^^^^^^^^^^^^^
```

```brix
(main
  (let foo (call Foo (call Bar 1)))
  (print (type-of foo))
  (print foo)
)

(type + (of Bar T) (is Tuple)
  (prop + b T)
)

(type + (of Foo T) (is Tuple)
  (prop + a (of Bar T))
)
```

```output
[TypeInfo Foo'I64]
((1))
```

```brix
(main
  (call foo 1foo 2bar)
)

(blank)
(type-line + Foo (alias foo) (is Float))
(type-line + Bar (alias bar) (is 2foo))

(fn-do + (of foo T) _
  (param a T)
  (param b T)
  (print (tuple T a b))
)
```

```output
([TypeInfo Foo], 1, 4)
```

## Inferring types from bindings

### From static function binding

```brix
(main
  (call get-funk foo)
)

(type + (of Funk T) (is (fn-type T)))

(fn-expr + foo _
  42
)

(fn-do + (of get-funk T) _
  (param fn (of Funk T))
  (print (call fn))
)
```

```output
42
```

```brix
(main
  (call get-funk foo)
)

(type + (of Funk T1 T2) (is (fn-type T2 (param n T1))))

(fn-expr + foo _
  (param n Float)
  (+ n 42)
)

(fn-do + (of get-funk T) _
  (param fn (of Funk T))
  (print T)
  (print (call fn 1.5))
)
```

```output
[TypeInfo F64]
43.5
```

### From method binding

```brix
(main
  (let thing (call Thing))
  (call get-funk (. thing foo))
)

(type + (of Funk T1 T2) (is (fn-type T2 (param n T1))))

(fn-do + (of get-funk T) _
  (param fn (of Funk T))
  (print T)
  (print (call fn 1.5))
)

(type + Thing
  (fn-expr + foo _
    (param n Float)
    (+ n 42)
  )
)
```

```output
[TypeInfo F64]
43.5
```

### From interface method binding

```brix
(main
  (let thing (as (call Thing) Proto))
  (call get-funk (. thing foo))
)

(type + (of Funk T1 T2) (is (fn-type T2 (param n T1))))

(fn-do + (of get-funk T) _
  (param fn (of Funk T))
  (print T)
  (print (call fn 1.5))
)

(type + Proto (is Interface)
  (fn-expr + foo Float
    (param n Float)
  )
)

(type + Thing
  (implements Proto)
  (fn-expr + foo _
    (param n _)
    (+ n 42)
  )
)
```

```output
[TypeInfo F64]
43.5
```

```brix
(main
  (call get-funk foo)
)

(type + (of Funk T1 T2) (is (fn-type T2 (param n T1))))

(fn-expr + foo _
  (param n _)
  (+ n 42)
)

(fn-do + (of get-funk T) _
  (param fn (of Funk Float T))
  (print T)
  (print (call fn 1.5))
)
```

```output
[TypeInfo F64]
43.5
```

## Inferring types from closures

```brix
(main
  (call get-funk (=> () 42))
)

(type + (of Funk T) (is (fn-type T)))

(fn-do + (of get-funk T) _
  (param fn (of Funk T))
  (print (call fn))
)
```

```output
42
```

```brix
(main
  (call get-funk (=> ((: n Float)) (+ n 42)))
)

(type + (of Funk TRet TArg) (is (fn-type TRet (param n TArg))))

(fn-do + (of get-funk T) _
  (param fn (of Funk T))
  (print T)
  (print (call fn 1.5))
)
```

```output
[TypeInfo F64]
43.5
```

```brix
(main
  (call get-funk (=> ((: n Float)) (+ n 42)))
)

(type + (of Funk T1 T2) (is (fn-type T2 (param n T1))))

(fn-do + (of get-funk T) _
  (param fn (of Funk T))
  (print T)
  (print (call fn 1.5))
)
```

```output
[TypeInfo F64]
43.5
```

```brix
(main
  (call get-funk (=> ((: foo (of Foo Float))) (+ (. foo n) 42)))
)

(type + (of Foo T) (is Tuple)
  (prop + n T)
)

(type + (of Funk T1 T2) (is (fn-type T1 (param n T2))))

(fn-do + (of get-funk T) _
  (param fn (of Funk T))
  (print T)
  (print (call fn (call Foo 1.5)))
)
```

```output
[TypeInfo F64]
43.5
```

```brix
(main
  (call get-funk (=> ((: foo (of Foo Float))) (+ (. foo n) 42)))
)

(type + (of Foo T) (is Tuple)
  (prop + n T)
)

(type + (of Funk T1 T2) (is (fn-type T2 (param n T1))))

(fn-do + (of get-funk T) _
  (param fn (of Funk (of Foo T)))
  (print T)
  (print (call fn (call Foo 1.5)))
)
```

```output
[TypeInfo F64]
43.5
```

```brix
(main
  (call get-funk (=> ((: foo Foo)) (+ (. foo n) 42)))
  (call get-funk (=> ((: foo (of Foo Float))) (+ (. foo n) 42)))
)

(type + (of Foo T) (is Tuple)
  (prop + n T)
)

(type + (of Funk TRet) (is (fn-type TRet (param n (of Foo Int)))))

(fn-do + get-funk _
  (param fn Funk)
  (print (type-of fn))
  (print (call fn (call Foo 1)))
)
```

```output
[TypeInfo Funk'I64]
43
[TypeInfo Funk'F64]
43
```

```brix
(main
  (call get-funk (=> (n) (+ n 42)))
)

(type + (of Funk T1 T2) (is (fn-type T2 (param n T1))))

(fn-do + (of get-funk T) _
  (param fn (of Funk Float T))
  (print T)
  (print (call fn 1.5))
)
```

```output
[TypeInfo F64]
43.5
```

```brix
(main
  (call get-funk (=> () 42))
)

(type + (of Funk T1) (is (fn-type T1)))

(fn-do + (of get-funk T) _
  (param fn (of Funk T))
  (print T)
  (print (call fn))
)
```

```output
[TypeInfo I64]
42
```

```brix
(main
  (call get-funk (=> (n) (call Foo (+ n 42))))
)

(type + (of Foo T) (is Tuple)
  (prop + n T)
)

(type + (of Funk T1 T2) (is (fn-type (of Foo T2) (param n T1))))

(fn-do + (of get-funk T) _
  (param fn (of Funk Float T))
  (print T)
  (print (call fn 1.5))
)
```

```output
[TypeInfo F64]
(43.5)
```

```brix
(main
  (call get-funk (=> (n) (call Foo (+ n 42))))
)

(type + (of Foo T) (is Tuple)
  (prop + n T)
)

(type + (of Funk T1 T2) (is (fn-type T2 (param n T1))))

(fn-do + (of get-funk T) _
  (param fn (of Funk Float (of Foo T)))
  (print T)
  (print (call fn 1.5))
)
```

```output
[TypeInfo F64]
(43.5)
```

## Inferring function from another function

```brix
(main
  (call get-funk (=> () 42))
)

(type + (of Funk T) (is (fn-type T)))

(fn-do + (of get-funk T) _
  (param fn (of Funk T))
  (print (call fn))
)
```

```output
42
```

## Inferring nested structures

```brix
(main
  (let case (call TestCase))
  (call (. case with) (tuple "foo" "bar"))
)

(type + (of TestPair T1 T2) (is (tuple (: expecting T1) (: actual T2))))

(type + TestCase
  (fn-do + (of with TA1 TA2) _
    (param pair (of TestPair TA1 TA2))
    (print pair)
    (return this)
  )
)
```

```output
(foo, bar)
```

### Unify as we infer

```brix
(main
  (let case (call TestCase))
  (call (. case with) (tuple 24 4.2))
)

(type + (of TestPair T) (is (tuple (: expecting T) (: actual T))))

(type + TestCase
  (fn-do + (of with T1) _
    (param pair (of TestPair T1))
    (print pair)
    (print (type-of pair))
    (return this)
  )
)
```

```output
(24, 4.2)
[TypeInfo TestPair'F64]
```

```brix
(main
  (let case (call TestCase))
  (call (. case with) (tuple "foo" (call (of Foo Int))))
)

(type + (of Foo T))

(type + (of TestPair T1 T2) (is (tuple (: expecting T1) (: actual T2))))

(type + TestCase
  (fn-do + (of with TA1 TA2) _
    (param pair (of TestPair TA1 (of Foo TA2)))
    (print TA2)
    (print pair)
    (return this)
  )
)
```

```output
[TypeInfo I64]
(foo, [Object __source__:Foo'core/number:I64])
```

```brix
(main
  (let case (call TestCase))
  (call (. case with) (tuple "foo" (tuple "bar" (call (of Foo Int)))))
)

(type + (of Foo T))

(type + (of TestPair T1 T2) (is (tuple (: expecting T1) (: actual T2))))

(type + (of TestThing TT1 TT2) (is (tuple (: expecting TT1) (: actual (of TestPair TT1 (of Foo TT2))))))

(type + TestCase
  (fn-do + (of with TA1 TA2) _
    (param pair (of TestThing TA1 TA2))
    (print TA2)
    (print pair)
    (return this)
  )
)
```

```output
[TypeInfo I64]
(foo, (bar, [Object __source__:Foo'core/number:I64]))
```

```brix
(main
  (let case (call TestCase))
  (call (. case with) (tuple "foo" (tuple "bar" (call (of Foo Int)))))
)

(type + (of Foo T))

(type + (of TestPair T1 T2) (is (tuple (: expecting T1) (: actual T2))))

(type + (of TestThing TT1 TT2) (is (tuple (: expecting TT1) (: actual TT2))))

(type + TestCase
  (fn-do + (of with TA1 TA2) _
    (param pair (of TestThing TA1 (of TestPair TA1 (of Foo TA2))))
    (print TA2)
    (print pair)
    (return this)
  )
)
```

```output
[TypeInfo I64]
(foo, (bar, [Object __source__:Foo'core/number:I64]))
```

```brix
(main
  (let case (call TestCase))
  (let t (=> () "bar"))
  (call (. case with) (tuple "foo" t))
)

(fn-expr + func _
  (param c _)
  c
)

(type + (of TestFunc TRet) (is (fn-type TRet)))

(type + (of TestFuncPair T1) (is (tuple (: expecting T1) (: actual (of TestFunc T1)))))

(type + TestCase
  (fn-do + (of with T) _
    (param pair (of TestFuncPair T))
    (print pair)
    (let fn (. pair actual))
    (print (call fn))
    (return this)
  )
)
```

```output
(foo, [Function TestFunc'String])
bar
```

```brix
(main
  (let a (list 1 2 3))
  (blank)
  (print (as a (of List Int)))
  (print (as a List))
  (blank)
  (print (as a (of Iterable Int)))
  (print (as a Iterable))
  (blank)
  (print (as a (of Stream Int)))
  (print (as a Stream))
)
```

```output
1, 2, 3
1, 2, 3
[Interface core/stream:Iterable'core/number:I64]
[Interface core/stream:Iterable'core/number:I64]
[Interface core/stream:Indexable'core/number:I64]
[Interface core/stream:Indexable'core/number:I64]
```

```brix
(main
  (let a (list 1 2 3))
  (let z (list a a a))
  (blank)
  (call dump (inline (as z List)))
  (blank)
  (call dump (inline (as z (of List List))))
  (call dump (inline (as z (of List (of List Int)))))
  (call dump (inline (as z (of List (of List String)))))
  (blank)
  (call dump (inline (as z (of List Iterable))))
  (call dump (inline (as z (of List (of Iterable Int)))))
  (call dump (inline (as z (of List (of Iterable String)))))
  (blank)
  (call dump (inline (as z (of List Stream))))
  (call dump (inline (as z (of List (of Stream Int)))))
  (call dump (inline (as z (of List (of Stream String)))))
  (blank)
  (call dump (inline (call foo z)))
  (call dump (inline (call bar z)))
  (call dump (inline (call baz z)))
)

(fn-expr + (of foo T) _
  (param list (of List (of List T)))
  list
)

(fn-expr + (of bar T) _
  (param list (of List (of Iterable T)))
  list
)

(fn-expr + (of baz T) _
  (param list (of List (of Stream T)))
  list
)

(fn-do + dump _
  (param x _)
  (print (type-of x))
)
```

```output
[TypeInfo List'List'I64'GCBacking'GCBacking]
[TypeInfo List'List'I64'GCBacking'GCBacking]
[TypeInfo List'List'I64'GCBacking'GCBacking]
[TypeInfo List'List'String'GCBacking'GCBacking]
[TypeInfo List'Iterable'I64'GCBacking]
[TypeInfo List'Iterable'I64'GCBacking]
[TypeInfo List'Iterable'String'GCBacking]
[TypeInfo List'Indexable'I64'GCBacking]
[TypeInfo List'Indexable'I64'GCBacking]
[TypeInfo List'Indexable'String'GCBacking]
[TypeInfo List'List'I64'GCBacking'GCBacking]
[TypeInfo List'Iterable'I64'GCBacking]
[TypeInfo List'Indexable'I64'GCBacking]
```

```brix
(main
  (let a (list 1 2 3))
  (let z (list a a a))
  (blank)
  (call dump (inline (as z Iterable)))
  (blank)
  (call dump (inline (as z (of Iterable List))))
  (call dump (inline (as z (of Iterable (of List Int)))))
  (call dump (inline (as z (of Iterable (of List String)))))
  (blank)
  (call dump (inline (as z (of Iterable Iterable))))
  (call dump (inline (as z (of Iterable (of Iterable Int)))))
  (call dump (inline (as z (of Iterable (of Iterable String)))))
  (blank)
  (call dump (inline (as z (of Iterable Stream))))
  (call dump (inline (as z (of Iterable (of Stream Int)))))
  (call dump (inline (as z (of Iterable (of Stream String)))))
  (blank)
  (call dump (inline (call foo z)))
  (call dump (inline (call bar z)))
  (call dump (inline (call baz z)))
)

(fn-expr + (of foo T) _
  (param list (of Iterable (of List T)))
  list
)

(fn-expr + (of bar T) _
  (param list (of Iterable (of Iterable T)))
  list
)

(fn-expr + (of baz T) _
  (param list (of Iterable (of Stream T)))
  list
)

(fn-do + dump _
  (param x _)
  (print (type-of x))
  ; -- type-of x, joined
  ;     .. joined = ", ".join of (flat-map of x)
)
```

```output
[TypeInfo Iterable'List'I64'GCBacking]
[TypeInfo Iterable'List'I64'GCBacking]
[TypeInfo Iterable'List'I64'GCBacking]
[TypeInfo Iterable'List'String'GCBacking]
[TypeInfo Iterable'Iterable'I64]
[TypeInfo Iterable'Iterable'I64]
[TypeInfo Iterable'Iterable'String]
[TypeInfo Iterable'Indexable'I64]
[TypeInfo Iterable'Indexable'I64]
[TypeInfo Iterable'Indexable'String]
[TypeInfo Iterable'List'I64'GCBacking]
[TypeInfo Iterable'Iterable'I64]
[TypeInfo Iterable'Indexable'I64]
```

```brix
(main
  (let a (list 1 2 3))
  (let z (list a a a))
  (blank)
  (call dump (inline (as z Stream)))
  (blank)
  (call dump (inline (as z (of Stream List))))
  (call dump (inline (as z (of Stream (of List Int)))))
  (call dump (inline (as z (of Stream (of List String)))))
  (blank)
  (call dump (inline (as z (of Stream Iterable))))
  (call dump (inline (as z (of Stream (of Iterable Int)))))
  (call dump (inline (as z (of Stream (of Iterable String)))))
  (blank)
  (call dump (inline (as z (of Stream Stream))))
  (call dump (inline (as z (of Stream (of Stream Int)))))
  (call dump (inline (as z (of Stream (of Stream String)))))
  (blank)
  (call dump (inline (call foo z)))
  (call dump (inline (call bar z)))
  (call dump (inline (call baz z)))
)

(fn-expr + (of foo T) _
  (param list (of Stream (of List T)))
  list
)

(fn-expr + (of bar T) _
  (param list (of Stream (of Iterable T)))
  list
)

(fn-expr + (of baz T) _
  (param list (of Stream (of Stream T)))
  list
)

(fn-do + dump _
  (param x _)
  (print (type-of x))
  ; -- type-of x, joined
  ;     .. joined = ", ".join of (flat-map of x)
)
```

```output
[TypeInfo Indexable'List'I64'GCBacking]
[TypeInfo Indexable'List'I64'GCBacking]
[TypeInfo Indexable'List'I64'GCBacking]
[TypeInfo Indexable'List'String'GCBacking]
[TypeInfo Indexable'Iterable'I64]
[TypeInfo Indexable'Iterable'I64]
[TypeInfo Indexable'Iterable'String]
[TypeInfo Indexable'Indexable'I64]
[TypeInfo Indexable'Indexable'I64]
[TypeInfo Indexable'Indexable'String]
[TypeInfo Indexable'List'I64'GCBacking]
[TypeInfo Indexable'Iterable'I64]
[TypeInfo Indexable'Indexable'I64]
```

```brix
(main
  (let a (list 1 2 3))
  (let z (list a a a))
  (let i (as z (of Iterable Iterable)))
  (blank)
  (call dump (inline (as i Iterable)))
  (blank)
  (call dump (inline (as i (of Iterable Iterable))))
  (call dump (inline (as i (of Iterable (of Iterable Int)))))
  (call dump (inline (as i (of Iterable (of Iterable String)))))
  (blank)
  (call dump (inline (as i (of Iterable Stream))))
  (call dump (inline (as i (of Iterable (of Stream Int)))))
  (call dump (inline (as i (of Iterable (of Stream String)))))
  (blank)
  (call dump (inline (call bar1 i)))
  (call dump (inline (call baz1 i)))
  (blank)
  (call dump (inline (as i Stream)))
  (blank)
  (call dump (inline (as i (of Stream Iterable))))
  (call dump (inline (as i (of Stream (of Iterable Int)))))
  (call dump (inline (as i (of Stream (of Iterable String)))))
  (blank)
  (call dump (inline (as i (of Stream Stream))))
  (call dump (inline (as i (of Stream (of Stream Int)))))
  (call dump (inline (as i (of Stream (of Stream String)))))
  (blank)
  (call dump (inline (call bar2 i)))
  (call dump (inline (call baz2 i)))
)

(fn-expr + (of bar1 T) _
  (param list (of Iterable (of Iterable T)))
  list
)

(fn-expr + (of baz1 T) _
  (param list (of Iterable (of Stream T)))
  list
)

(fn-expr + (of bar2 T) _
  (param list (of Stream (of Iterable T)))
  list
)

(fn-expr + (of baz2 T) _
  (param list (of Stream (of Stream T)))
  list
)

(fn-do + dump _
  (param x _)
  (print (type-of x))
  ;  for list in x
  ;      for item in list
  ;          -- item
  ;  -- type-of x, joined
  ;    .. joined = ", ".join of (flat-map of x)
)
```

```output
[TypeInfo Iterable'Iterable'I64]
[TypeInfo Iterable'Iterable'I64]
[TypeInfo Iterable'Iterable'I64]
[TypeInfo Iterable'Iterable'String]
[TypeInfo Iterable'Iterable'I64]
[TypeInfo Iterable'Iterable'I64]
[TypeInfo Iterable'Iterable'String]
[TypeInfo Iterable'Iterable'I64]
[TypeInfo Iterable'Iterable'I64]
[TypeInfo Iterable'Iterable'I64]
[TypeInfo Iterable'Iterable'I64]
[TypeInfo Iterable'Iterable'I64]
[TypeInfo Iterable'Iterable'String]
[TypeInfo Iterable'Iterable'I64]
[TypeInfo Iterable'Iterable'I64]
[TypeInfo Iterable'Iterable'String]
[TypeInfo Iterable'Iterable'I64]
[TypeInfo Iterable'Iterable'I64]
```

```brix crash
(main
  (let thing (call (of Thing Int)))
  (print (as thing (of Quack String Int)))
)

(type + (of Thing T)
  (implements (of Bar T))
)

(type + (of Foo T) (is Interface))

(type + (of Bar T) (is Interface))

(type + (of Quack A B) (is (of Foo B) (of Bar A)))
```

```output
Exception: Thing'I64 cannot be cast to Quack'String'I64; there is no conversion between them
<no file:4>
      (print (as thing (of Quack String Int)))
             ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix
(main
  (let thing (call (of Thing Int)))
  (print (as thing (of Quack Int Int)))
  (print (as thing (of Quack Int String)))
  (blank)
  (let wing (call (of Wing String)))
  (print (as wing (of Quack Int String)))
  (print (as wing (of Quack String String)))
)

(type + (of Thing T)
  (implements (of Bar T))
)

(type + (of Wing T)
  (implements (of Foo T))
)

(type + (of Foo T) (is Interface))

(type + (of Bar T) (is Interface))

(type + (of Quack A B) (is (of Foo B) (of Bar A)))
```

```output
[Interface __source__:Bar'core/number:I64]
[Interface __source__:Bar'core/number:I64]
[Interface __source__:Foo'core/string:String]
[Interface __source__:Foo'core/string:String]
```

```brix
(main
  (call foo (tuple 4 5))
)

(fn-do + (of foo T V) _
  (param n (tuple (: x T) (: y V)))
  (print (tuple "hi" T V))
)
```

```output
(hi, [TypeInfo I64], [TypeInfo I64])
```

```brix
(main
  (call foo (tuple (list 1 2 3) 4))
)

(fn-do + (of foo T) _
  (param n (tuple (: x (of Stream T)) (: y T)))
  (print (. n x))
  (print (tuple "hi" T))
)
```

```output
[Interface core/stream:Indexable'core/number:I64]
(hi, [TypeInfo I64])
```

```brix
(main
  (let (tuple (: a Int) (: b Stream)) (tuple 42 (list 1 2 3)))
  (print (type-of b))
  (print (call (. "," join) b))
)
```

```output
[TypeInfo Indexable'I64]
1,2,3
```

### Constructor inference for ordinary generic tuple constructors stays concrete

```brix
(main
  (let pairs (list (call Pair "foo" 42)))
  (let typed (as pairs (list (of Pair String Int))))
  (print (. ([] typed 0) key))
  (print (. ([] typed 0) value))
)

(type + (of Pair Key Value) (is (tuple (: key Key) (: value Value))))
```

```output
foo
42
```

### A declared list return type materializes a generator without recursively compiling that same converter while ranking the list constructors. Bool keeps the recursive union on the conversion path that exposed the incomplete converter body.

```brix
(main
  (let values (call collect (list "hello" "goodbye")))
  (print (. values length))
  (print (as ([] values 0) String))
  (print (as ([] values 1) String))
)

(type + Value (is Union)
  (variant Bool)
  (variant String)
  (variant (list Value))
)

(fn-expr _ collect (list Value)
  (param source (list String))
  (in (as each Value) source)
)
```

```output
2
hello
goodbye
```

### A table whose arms return sibling subclasses, one of them optional, joins to their optional common base class, both under a declared return type and when inferred.

```brix
(main
  (print (call describe (call pick 0)))
  (print (call describe (call pick 1)))
  (print (call describe (call pick 2)))
  (print (call describe (call pick-inferred 0)))
  (print (call describe (call pick-inferred 1)))
  (print (call describe (call pick-inferred 2)))
)

(type + Animal
  (prop + name _ (= ""))
)

(type + Cat (is Animal))

(type + Dog (is Animal))

(fn-expr _ pick (? Animal)
  (param index Int)
  (case? (== index 0)
    cat
  )
  (default
    dog
  )
  (where
    (lazy cat (call named-cat))
    (lazy dog (only-if (call named-dog) (if (== index 1))))
  )
)

(fn-expr _ pick-inferred _
  (param index Int)
  (case? (== index 0)
    (call named-cat)
  )
  (default
    (only-if (call named-dog) (if (== index 1)))
  )
)

(fn-do _ named-cat _
  (let cat (call Cat))
  (= (. cat name) "tom")
  (return cat)
)

(fn-do _ named-dog _
  (let dog (call Dog))
  (= (. dog name) "rex")
  (return dog)
)

(fn-expr _ describe _
  (param animal (? Animal))
  (?? (?. animal name) "none")
)
```

```output
tom
rex
none
tom
rex
none
```

### A type parameter is bound by the plain argument, and a function given that takes a wider number and answers one the parameter holds is converted to it.

```brix
(main
  (print (call once half 5i32))
  (print (call once half 5.0))
)

(fn-expr + (of once T) _
  (param fn (fn-type T (param n T)))
  (param value T)
  (call fn value)
)

(fn-expr + half I32
  (param n Float)
  (as (/ n 2) I32)
)

(fn-expr + double _
  (param n Int)
  (* n 2)
)
```

```output
2
2
```

### A closure with and without a stated argument type given with a value of its number: the value binds the type parameter.

```brix
(main
  (print (call once (=> (n) (/ n 2)) 5))
  (print (call once (=> (n) (/ n 2)) 5.0))
  (print (call once (=> ((: n Float)) (/ n 2)) 5.0))
  (print (call once (=> ((: n Int)) (/ n 2)) 5))
)

(fn-expr + (of once T) _
  (param fn (fn-type T (param n T)))
  (param value T)
  (call fn value)
)
```

```output
2
2.5
2.5
2
```

### A function given with two values that bind a type parameter, of one number and of two.

```brix
(main
  (print (call first half 5i32 7))
  (print (call first half 5.5 7))
  (print (call first half 5 7.5))
)

(fn-expr + (of first T) _
  (param fn (fn-type T (param n T)))
  (param a T)
  (param b T)
  (+ (call fn a) (call fn b))
)

(fn-expr + half I32
  (param n Float)
  (as (/ n 2) I32)
)
```

```output
5
5
5
```

### A function given where a generic function type is taken with a value of another number.

```brix
(main
  (print (call once half 5i32))
)

(fn-expr + (of once T) _
  (param fn (of Fn T))
  (param value T)
  (call fn value)
)

(type + (of Fn T) (is (fn-type T (param n T))))

(fn-expr + half I32
  (param n Float)
  (as (/ n 2) I32)
)
```

```output
2
```

### A function and a closure given with a value that binds one type parameter beside another parameter.

```brix
(main
  (print (call pair half 5i32 "x"))
  (print (call pair (=> ((: n Float)) (as (/ n 2) I32)) 5i32 "x"))
)

(fn-expr + (of pair T U) _
  (param fn (fn-type T (param n T)))
  (param value T)
  (param other U)
  (str (call fn value) " " other)
)

(fn-expr + half I32
  (param n Float)
  (as (/ n 2) I32)
)
```

```output
2 x
2 x
```

### Two overloads that take a function and a value in either order, and a function that takes two functions, with a value of another number.

```brix
(main
  (print (call apply half 5i32))
  (print (call apply 5i32 half))
  (print (call chain half half 5i32))
)

(fn-expr + (of apply T) T
  (param fn (fn-type T (param n T)))
  (param value T)
  (call fn value)
)

(fn-expr + (of apply T) T
  (param value T)
  (param fn (fn-type T (param n T)))
  (+ (call fn value) 1)
)

(fn-expr + (of chain T) T
  (param a (fn-type T (param n T)))
  (param b (fn-type T (param n T)))
  (param value T)
  (call a (call b value))
)

(fn-expr + half I32
  (param n Float)
  (as (/ n 2) I32)
)
```

```output
2
3
1
```

### Closures that state another number than the value given, and a function called twice on its own answer.

```brix
(main
  (print (call apply (=> ((: n Int)) (as (+ n 1) I32)) 2i32))
  (print (call apply (=> ((: n Int)) (+ n 1)) 2))
  (print (call twice (=> ((: n Float)) (as (/ n 2) I32)) 5i32))
  (print (call twice half 5i32))
)

(fn-expr + (of apply T) T
  (param fn (fn-type T (param n T)))
  (param value T)
  (call fn value)
)

(fn-expr + (of twice T) T
  (param fn (fn-type T (param n T)))
  (param value T)
  (call fn (call fn value))
)

(fn-expr + half I32
  (param n Float)
  (as (/ n 2) I32)
)
```

```output
3
3
1
1
```

### Two functions of different numbers given with one value that binds the type parameter of both.

```brix
(main
  (print (call both half double 5i32))
  (print (call both double half 5i32))
)

(fn-expr + (of both T) _
  (param first (fn-type T (param n T)))
  (param second (fn-type T (param n T)))
  (param value T)
  (call second (call first value))
)

(fn-expr + half I32
  (param n Float)
  (as (/ n 2) I32)
)

(fn-expr + double I32
  (param n Int)
  (as (* n 2) I32)
)
```

```output
4
5
```

### A type parameter only the function's answer names is bound by the function given.

```brix
(main
  (print (call shown label 5))
  (print (call shown half 5i32))
  (print (call shown (=> ((: n Float)) (/ n 2)) 5i32))
)

(fn-expr + (of shown T U) U
  (param fn (fn-type U (param n T)))
  (param value T)
  (call fn value)
)

(fn-expr + label _
  (param n Int)
  (str "n " n)
)

(fn-expr + half _
  (param n Float)
  (/ n 2)
)
```

```output
n 5
2.5
2.5
```

### A function of a generic class takes a function over its type parameter, given functions and closures of other numbers.

```brix
(main
  (let box (call (of Box I32) 5))
  (print (call (. box through) half))
  (print (call (. box through) (=> ((: n Float)) (as (/ n 2) I32))))
  (print (call (. box through) (=> (n) (* n 2))))
)

(type + (of Box T)
  (prop + held T)
  (fn-do + This _
    (param held _)
  )
  (fn-expr + through T
    (param fn (fn-type T (param n T)))
    (call fn held)
  )
)

(fn-expr + half I32
  (param n Float)
  (as (/ n 2) I32)
)
```

```output
2
2
10
```

### Closures and a function that take a tuple of other numbers than the function type gives

```brix
(main
  (call get-funk (=> ((: foo (of Foo Float))) (+ (. foo n) 42)))
  (call get-funk (=> ((: foo (of Foo F32))) (* (. foo n) 2)))
  (call get-funk wide)
)

(type + (of Foo T) (is Tuple)
  (prop + n T)
)

(type + (of Funk TRet) (is (fn-type TRet (param n (of Foo Int)))))

(fn-do + wide Float
  (param foo (of Foo Float))
  (return (/ (. foo n) 2))
)

(fn-do + get-funk _
  (param fn Funk)
  (print (type-of fn))
  (print (call fn (call Foo 1)))
  (print (call fn (call Foo 7)))
)
```

```output
[TypeInfo Funk'F64]
43
49
[TypeInfo Funk'F32]
2
14
[TypeInfo Funk'F64]
0.5
3.5
```

### Closure that takes a tuple of tuples of other numbers than the function type gives

```brix
(main
  (call show (=> ((: both (of Both Float))) (+ (. both first n) (. both second n) 0.5)))
)

(type + (of Foo T) (is Tuple)
  (prop + n T)
)

(type + (of Both T) (is Tuple)
  (prop + first (of Foo T))
  (prop + second (of Foo T))
)

(type + (of Shower TRet) (is (fn-type TRet (param both (of Both Int)))))

(fn-do + show _
  (param fn Shower)
  (print (call fn (call Both (call Foo 1) (call Foo 2))))
)
```

```output
3.5
```

### Function type converts a tuple its function returns to one of other numbers

```brix
(main
  (let make Maker (=> ((: n Int)) (call Foo n)))
  (print (. (call make 3) n))
  (let wide Wider make-int)
  (print (. (call wide 4) n))
)

(type + (of Foo T) (is Tuple)
  (prop + n T)
)

(type + Maker (is (fn-type (of Foo Float) (param n Int))))

(type + Wider (is (fn-type (of Foo Float) (param n Int))))

(fn-do + make-int (of Foo Int)
  (param n Int)
  (return (call Foo (+ n 1)))
)
```

```output
3
5
```

### A function that states no return type and returns `null` under a condition returns an optional of what it returns otherwise.

```brix
(main
  (print (str (?? (call pick 1) 0) " " (?? (call pick 5) 0)))
)

(fn-do _ pick _
  (param n Int)
  (return-if null (if (> n 3)))
  (return n)
)
```

```output
1 0
```

### A null given for an optional of a type parameter tells nothing of the type, which another argument binds.

```brix
(main
  (print (type-of (call pick 5 null)))
)

(fn-expr + (of pick T) T
  (param a T)
  (param b (? T))
  a
)
```

```output
[TypeInfo I64]
```

### An integer given for an optional of a type parameter that another argument binds to a float is that float.

```brix
(main
  (let holder (call (of Holder F64) 2.5))
  (print (call pick holder 0 1))
  (print (call pick holder null 3))
)

(type + (of Holder Value)
  (prop + value Value)
  (fn-do + This _
    (param value _)
  )
)

(fn-expr + (of pick Value) String
  (param holder (of Holder Value))
  (param from (? Value))
  (param to (? Value))
  (str from " .. " to " of " (. holder value) " " (type-of from))
)
```

```output
0 .. 1 of 2.5 [TypeInfo Optional'F64]
unknown .. 3 of 2.5 [TypeInfo Optional'F64]
```

### A float given for a type parameter that another argument binds to a unit of floats is that unit.

```brix
(main
  (let holder (call (of Holder Turns) 1turn))
  (let half 0.5)
  (print (call pick holder 0 2.5))
  (print (call pick holder null (* half 3)))
  (print (call one holder (/ half 2)))
)

(blank)
(type-line + Turns (alias turn) (is F64))

(type + (of Holder Value)
  (prop + value Value)
  (fn-do + This _
    (param value _)
  )
)

(fn-expr + (of pick Value) String
  (param holder (of Holder Value))
  (param from (? Value))
  (param to (? Value))
  (str from " .. " to " " (type-of to))
)

(fn-expr + (of one Value) String
  (param holder (of Holder Value))
  (param value Value)
  (str value " " (type-of value))
)
```

```output
0 .. 2.5 [TypeInfo Optional'Turns]
unknown .. 1.5 [TypeInfo Optional'Turns]
0.25 [TypeInfo Turns]
```

### The arguments alone bind a type parameter, and the default of a parameter of that type is cast to what they bind.

```brix
(main
  (print (call scaled 2))
  (print (call scaled 2.5))
  (print (call scaled 2 3))
)

(fn-expr + (of scaled T) String
  (param a T)
  (param b T (= 0.5))
  (str a " " b)
)
```

```output
2 0
2.5 0.5
2 3
```
