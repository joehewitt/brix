# Infer

```yumi
. a = 1
-- a
```

```output
1
```

### A do block of void calls does not suppress the final expression

```yumi
-- foo()

━━━━━━━━━━━━━━━━━━━━
+ do touch

━━━━━━━━━━━━━━━━━━━━
+ foo Int

do
    touch()
42
```

```output
42
```

### Infer return type from argument

```yumi
-- foo(1)
-- foo(1.5)

━━━━━━━━━━━━━━━━━━━━
+ foo
    - a

a
```

```output
1
1.5
```

### Infer return type from type arguments

```yumi
. thing = bar()
-- foo(thing)

━━━━━━━━━━━━━━━━━━━━
+ foo'T
    - t Thing'T

t.a

━━━━━━━━━━━━━━━━━━━━
+ bar

Thing'Float()

━━━━━━━━━━━━━━━━━━━━
+ Thing'T

+ a T = 0
```

```output
0
```

### Infer return type with multiple returns of same type

```yumi
-- foo(2)
-- foo(0)

━━━━━━━━━━━━━━━━━━━━
+ foo
    - x

|? x > 1
    1
??
    2
```

```output
1
2
```

```yumi
-- foo(2)
-- foo(0)

━━━━━━━━━━━━━━━━━━━━
+ do foo
    - x

while x > 1
    = 1

= 2
```

```output
1
2
```

### Infer optional types from functions that do not always return

```yumi
-- foo(1)
-- foo(2)

━━━━━━━━━━━━━━━━━━━━
+ do foo
    - x

if x > 1
    = x
```

```output
unknown
2
```

### A conditional return of an optional makes the inferred return type optional

```yumi
-- sooner(null, 0)
-- sooner(5, 2)
-- sooner(1, 2)

━━━━━━━━━━━━━━━━━━━━
+ do sooner
    - first Int?
    - position Int

=? first if position == 0
=? first if first != null && first <= position
= position
```

```output
unknown
2
1
```

### Infer union types from return values of different types

```yumi
-- foo(2)
-- foo(1)

━━━━━━━━━━━━━━━━━━━━
+ do foo
    - x

if x > 1
    = 1
else
    = 2.5
```

```output
1
2.5
```

### Infer nested parameter via converter

```yumi
. thing = Thing()
foo(thing)

━━━━━━━━━━━━━━━━━━━━
+ Thing

────────────────────
+ do to Iterable'String

<= "hi"

━━━━━━━━━━━━━━━━━━━━
+ do foo'T
    - stream Iterable'T

-- <- stream
```

```output
hi
```

### Infer from fromverter

```yumi
-- foo(Foo'Int'Float())

━━━━━━━━━━━━━━━━━━━━
+ Bar'F'G

────────────────────
+ from foo Foo'Int'Float

This'Int'Float()

━━━━━━━━━━━━━━━━━━━━
+ Foo'T1'T2

━━━━━━━━━━━━━━━━━━━━
+ foo
    - x Bar''T''V

x
```

```output
[Object __source__:Bar'core/number:I64'core/number:F64]
```

### Try and fail to infer return type from incompatible types

```yumi crash
-- foo(2)
-- foo(1)

━━━━━━━━━━━━━━━━━━━━
+ Foo

━━━━━━━━━━━━━━━━━━━━
+ do foo
    - x

if x > 1
    = Foo()
else
    = 2.5
```

```output
Exception: Function "foo" returns Foo here and F64 elsewhere, which have no common type; return one type, or declare the return type
<no file:13>
        = Foo()
        ^^^^^^^
```

```yumi
foo(Thing(), 42.5)

━━━━━━━━━━━━━━━━━━━━
+ Thing

────────────────────
+ to Float

99

━━━━━━━━━━━━━━━━━━━━
+ do foo'T
    - a T
    - b T

-- T, a, b
```

```output
([TypeInfo F64], 99, 42.5)
```

### Values of a type parameter that are a text and a number have no type in common.

```yumi crash
foo("10", 42.5)

━━━━━━━━━━━━━━━━━━━━
+ do foo'T
    - a T
    - b T

-- T, a, b
```

```output
Exception: No function "foo" accepts (String, F64); it is declared as "foo(a T, b T)"
<no file:2>
    foo("10", 42.5)
    ^^^^^^^^^^^^^^^
```

### Values of a type parameter that are a Bool and a number have no type in common.

```yumi crash
foo(false, 42.5)

━━━━━━━━━━━━━━━━━━━━
+ do foo'T
    - a T
    - b T

-- T, a, b
```

```output
Exception: No function "foo" accepts (Bool, F64); it is declared as "foo(a T, b T)"
<no file:2>
    foo(false, 42.5)
    ^^^^^^^^^^^^^^^^
```

```yumi
. foo = Foo(Bar(1))
-- type-of foo
-- foo

━━━━━━━━━━━━━━━━━━━━
+ Bar'T is Tuple

+ b T

━━━━━━━━━━━━━━━━━━━━
+ Foo'T is Tuple

+ a Bar'T
```

```output
[TypeInfo Foo'I64]
((1))
```

```yumi
foo(1foo, 2bar)

+ Foo (foo) is Float
+ Bar (bar) is 2foo

━━━━━━━━━━━━━━━━━━━━
+ do foo'T
    - a T
    - b T

-- T, a, b
```

```output
([TypeInfo Foo], 1, 4)
```

## Inferring types from bindings

### From static function binding

```yumi
get-funk(foo)

━━━━━━━━━━━━━━━━━━━━
+ Funk'T is () T

━━━━━━━━━━━━━━━━━━━━
+ foo

42

━━━━━━━━━━━━━━━━━━━━
+ do get-funk'T
    - fn Funk'T

-- fn()
```

```output
42
```

```yumi
get-funk(foo)

━━━━━━━━━━━━━━━━━━━━
+ Funk'T1'T2 is () T2
    - n T1

━━━━━━━━━━━━━━━━━━━━
+ foo
    - n Float

n + 42

━━━━━━━━━━━━━━━━━━━━
+ do get-funk'T
    - fn Funk'T

-- T
-- fn(1.5)
```

```output
[TypeInfo F64]
43.5
```

### From method binding

```yumi
. thing = Thing()
get-funk(thing.foo)

━━━━━━━━━━━━━━━━━━━━
+ Funk'T1'T2 is () T2
    - n T1

━━━━━━━━━━━━━━━━━━━━
+ do get-funk'T
    - fn Funk'T

-- T
-- fn(1.5)

━━━━━━━━━━━━━━━━━━━━
+ Thing

────────────────────
+ foo
    - n Float

n + 42
```

```output
[TypeInfo F64]
43.5
```

### From interface method binding

```yumi
. thing = Thing() as Proto
get-funk(thing.foo)

━━━━━━━━━━━━━━━━━━━━
+ Funk'T1'T2 is () T2
    - n T1

━━━━━━━━━━━━━━━━━━━━
+ do get-funk'T
    - fn Funk'T

-- T
-- fn(1.5)

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ foo Float
    - n Float

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Proto

────────────────────
+ foo
    - n

n + 42
```

```output
[TypeInfo F64]
43.5
```

```yumi
get-funk(foo)

━━━━━━━━━━━━━━━━━━━━
+ Funk'T1'T2 is () T2
    - n T1

━━━━━━━━━━━━━━━━━━━━
+ foo
    - n

n + 42

━━━━━━━━━━━━━━━━━━━━
+ do get-funk'T
    - fn Funk'Float'T

-- T
-- fn(1.5)
```

```output
[TypeInfo F64]
43.5
```

## Inferring types from closures

```yumi
get-funk(=> 42)

━━━━━━━━━━━━━━━━━━━━
+ Funk'T is () T

━━━━━━━━━━━━━━━━━━━━
+ do get-funk'T
    - fn Funk'T

-- fn()
```

```output
42
```

```yumi
get-funk(:: n Float => n + 42)

━━━━━━━━━━━━━━━━━━━━
+ Funk'TRet'TArg is () TRet
    - n TArg

━━━━━━━━━━━━━━━━━━━━
+ do get-funk'T
    - fn Funk'T

-- T
-- fn(1.5)
```

```output
[TypeInfo F64]
43.5
```

```yumi
get-funk(:: n Float => n + 42)

━━━━━━━━━━━━━━━━━━━━
+ Funk'T1'T2 is () T2
    - n T1

━━━━━━━━━━━━━━━━━━━━
+ do get-funk'T
    - fn Funk'T

-- T
-- fn(1.5)
```

```output
[TypeInfo F64]
43.5
```

```yumi
get-funk(:: foo Foo'Float => foo.n + 42)

━━━━━━━━━━━━━━━━━━━━
+ Foo'T is Tuple

+ n T

━━━━━━━━━━━━━━━━━━━━
+ Funk'T1'T2 is () T1
    - n T2

━━━━━━━━━━━━━━━━━━━━
+ do get-funk'T
    - fn Funk'T

-- T
-- fn(Foo(1.5))
```

```output
[TypeInfo F64]
43.5
```

```yumi
get-funk(:: foo Foo'Float => foo.n + 42)

━━━━━━━━━━━━━━━━━━━━
+ Foo'T is Tuple

+ n T

━━━━━━━━━━━━━━━━━━━━
+ Funk'T1'T2 is () T2
    - n T1

━━━━━━━━━━━━━━━━━━━━
+ do get-funk'T
    - fn Funk'(Foo'T)

-- T
-- fn(Foo(1.5))
```

```output
[TypeInfo F64]
43.5
```

```yumi
get-funk(:: foo Foo => foo.n + 42)
get-funk(:: foo Foo'Float => foo.n + 42)

━━━━━━━━━━━━━━━━━━━━
+ Foo'T is Tuple

+ n T

━━━━━━━━━━━━━━━━━━━━
+ Funk'TRet is () TRet
    - n Foo'Int

━━━━━━━━━━━━━━━━━━━━
+ do get-funk
    - fn Funk

-- type-of fn
-- fn(Foo(1))
```

```output
[TypeInfo Funk'I64]
43
[TypeInfo Funk'F64]
43
```

```yumi
get-funk(:: n => n + 42)

━━━━━━━━━━━━━━━━━━━━
+ Funk'T1'T2 is () T2
    - n T1

━━━━━━━━━━━━━━━━━━━━
+ do get-funk'T
    - fn Funk'Float'T

-- T
-- fn(1.5)
```

```output
[TypeInfo F64]
43.5
```

```yumi
get-funk(=> 42)

━━━━━━━━━━━━━━━━━━━━
+ Funk'T1 is () T1

━━━━━━━━━━━━━━━━━━━━
+ do get-funk'T
    - fn Funk'T

-- T
-- fn()
```

```output
[TypeInfo I64]
42
```

```yumi
get-funk(:: n => Foo(n + 42))

━━━━━━━━━━━━━━━━━━━━
+ Foo'T is Tuple

+ n T

━━━━━━━━━━━━━━━━━━━━
+ Funk'T1'T2 is () Foo'T2
    - n T1

━━━━━━━━━━━━━━━━━━━━
+ do get-funk'T
    - fn Funk'Float'T

-- T
-- fn(1.5)
```

```output
[TypeInfo F64]
(43.5)
```

```yumi
get-funk(:: n => Foo(n + 42))

━━━━━━━━━━━━━━━━━━━━
+ Foo'T is Tuple

+ n T

━━━━━━━━━━━━━━━━━━━━
+ Funk'T1'T2 is () T2
    - n T1

━━━━━━━━━━━━━━━━━━━━
+ do get-funk'T
    - fn Funk'Float'(Foo'T)

-- T
-- fn(1.5)
```

```output
[TypeInfo F64]
(43.5)
```

## Inferring function from another function

```yumi
get-funk(=> 42)

━━━━━━━━━━━━━━━━━━━━
+ Funk'T is () T

━━━━━━━━━━━━━━━━━━━━
+ do get-funk'T
    - fn Funk'T

-- fn()
```

```output
42
```

## Inferring nested structures

```yumi
. case = TestCase()
case.with(("foo", "bar"))

━━━━━━━━━━━━━━━━━━━━
+ TestPair'T1'T2 is (expecting T1, actual T2)

━━━━━━━━━━━━━━━━━━━━
+ TestCase

────────────────────
+ do with'TA1'TA2
    - pair TestPair'TA1'TA2

-- pair
= this
```

```output
(foo, bar)
```

### Unify as we infer

```yumi
. case = TestCase()
case.with((24, 4.2))

━━━━━━━━━━━━━━━━━━━━
+ TestPair'T is (expecting T, actual T)

━━━━━━━━━━━━━━━━━━━━
+ TestCase

────────────────────
+ do with'T1
    - pair TestPair'T1

-- pair
-- type-of pair
= this
```

```output
(24, 4.2)
[TypeInfo TestPair'F64]
```

```yumi
. case = TestCase()
case.with(("foo", Foo'Int()))

━━━━━━━━━━━━━━━━━━━━
+ Foo'T

━━━━━━━━━━━━━━━━━━━━
+ TestPair'T1'T2 is (expecting T1, actual T2)

━━━━━━━━━━━━━━━━━━━━
+ TestCase

────────────────────
+ do with'TA1'TA2
    - pair TestPair'TA1'(Foo'TA2)

-- TA2
-- pair
= this
```

```output
[TypeInfo I64]
(foo, [Object __source__:Foo'core/number:I64])
```

```yumi
. case = TestCase()
case.with(("foo", ("bar", Foo'Int())))

━━━━━━━━━━━━━━━━━━━━
+ Foo'T

━━━━━━━━━━━━━━━━━━━━
+ TestPair'T1'T2 is (expecting T1, actual T2)

━━━━━━━━━━━━━━━━━━━━
+ TestThing'TT1'TT2 is (expecting TT1, actual TestPair'TT1'(Foo'TT2))

━━━━━━━━━━━━━━━━━━━━
+ TestCase

────────────────────
+ do with'TA1'TA2
    - pair TestThing'TA1'TA2

-- TA2
-- pair
= this
```

```output
[TypeInfo I64]
(foo, (bar, [Object __source__:Foo'core/number:I64]))
```

```yumi
. case = TestCase()
case.with(("foo", ("bar", Foo'Int())))

━━━━━━━━━━━━━━━━━━━━
+ Foo'T

━━━━━━━━━━━━━━━━━━━━
+ TestPair'T1'T2 is (expecting T1, actual T2)

━━━━━━━━━━━━━━━━━━━━
+ TestThing'TT1'TT2 is (expecting TT1, actual TT2)

━━━━━━━━━━━━━━━━━━━━
+ TestCase

────────────────────
+ do with'TA1'TA2
    - pair TestThing'TA1'(TestPair'TA1'(Foo'TA2))

-- TA2
-- pair
= this
```

```output
[TypeInfo I64]
(foo, (bar, [Object __source__:Foo'core/number:I64]))
```

```yumi
. case = TestCase()
. t = => "bar"
case.with(("foo", t))

━━━━━━━━━━━━━━━━━━━━
+ func
    - c
c

━━━━━━━━━━━━━━━━━━━━
+ TestFunc'TRet is () TRet

━━━━━━━━━━━━━━━━━━━━
+ TestFuncPair'T1 is (expecting T1, actual TestFunc'T1)

━━━━━━━━━━━━━━━━━━━━
+ TestCase

────────────────────
+ do with'T
    - pair TestFuncPair'T

-- pair
. fn = pair.actual
-- fn()
= this
```

```output
(foo, [Function TestFunc'String])
bar
```

```yumi
. a = [1, 2, 3]

-- a as List'Int
-- a as List

-- a as Iterable'Int
-- a as Iterable

-- a as Stream'Int
-- a as Stream
```

```output
1, 2, 3
1, 2, 3
[Interface core/stream:Iterable'core/number:I64]
[Interface core/stream:Iterable'core/number:I64]
[Interface core/stream:Indexable'core/number:I64]
[Interface core/stream:Indexable'core/number:I64]
```

```yumi
. a = [1, 2, 3]
. z = [a, a, a]

dump: z as List

dump: z as List'List
dump: z as List'(List'Int)
dump: z as List'(List'String)

dump: z as List'Iterable
dump: z as List'(Iterable'Int)
dump: z as List'(Iterable'String)

dump: z as List'Stream
dump: z as List'(Stream'Int)
dump: z as List'(Stream'String)

dump: foo(z)
dump: bar(z)
dump: baz(z)

━━━━━━━━━━━━━━━━━━━━
+ foo'T
    - list List'(List'T)

list

━━━━━━━━━━━━━━━━━━━━
+ bar'T
    - list List'(Iterable'T)

list

━━━━━━━━━━━━━━━━━━━━
+ baz'T
    - list List'(Stream'T)

list

━━━━━━━━━━━━━━━━━━━━
+ do dump
    - x

-- type-of x
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

```yumi
. a = [1, 2, 3]
. z = [a, a, a]

dump: z as Iterable

dump: z as Iterable'List
dump: z as Iterable'(List'Int)
dump: z as Iterable'(List'String)

dump: z as Iterable'Iterable
dump: z as Iterable'(Iterable'Int)
dump: z as Iterable'(Iterable'String)

dump: z as Iterable'Stream
dump: z as Iterable'(Stream'Int)
dump: z as Iterable'(Stream'String)

dump: foo(z)
dump: bar(z)
dump: baz(z)

━━━━━━━━━━━━━━━━━━━━
+ foo'T
    - list Iterable'(List'T)

list

━━━━━━━━━━━━━━━━━━━━
+ bar'T
    - list Iterable'(Iterable'T)

list

━━━━━━━━━━━━━━━━━━━━
+ baz'T
    - list Iterable'(Stream'T)

list

━━━━━━━━━━━━━━━━━━━━
+ do dump
    - x

-- type-of x
` -- type-of x, joined
`     .. joined = ", ".join of (flat-map of x)
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

```yumi
. a = [1, 2, 3]
. z = [a, a, a]

dump: z as Stream

dump: z as Stream'List
dump: z as Stream'(List'Int)
dump: z as Stream'(List'String)

dump: z as Stream'Iterable
dump: z as Stream'(Iterable'Int)
dump: z as Stream'(Iterable'String)

dump: z as Stream'Stream
dump: z as Stream'(Stream'Int)
dump: z as Stream'(Stream'String)

dump: foo(z)
dump: bar(z)
dump: baz(z)

━━━━━━━━━━━━━━━━━━━━
+ foo'T
    - list Stream'(List'T)

list

━━━━━━━━━━━━━━━━━━━━
+ bar'T
    - list Stream'(Iterable'T)

list

━━━━━━━━━━━━━━━━━━━━
+ baz'T
    - list Stream'(Stream'T)

list

━━━━━━━━━━━━━━━━━━━━
+ do dump
    - x

-- type-of x
` -- type-of x, joined
`     .. joined = ", ".join of (flat-map of x)
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

```yumi
. a = [1, 2, 3]
. z = [a, a, a]
. i = z as Iterable'Iterable

dump: i as Iterable

dump: i as Iterable'Iterable
dump: i as Iterable'(Iterable'Int)
dump: i as Iterable'(Iterable'String)

dump: i as Iterable'Stream
dump: i as Iterable'(Stream'Int)
dump: i as Iterable'(Stream'String)

dump: bar1(i)
dump: baz1(i)

dump: i as Stream

dump: i as Stream'Iterable
dump: i as Stream'(Iterable'Int)
dump: i as Stream'(Iterable'String)

dump: i as Stream'Stream
dump: i as Stream'(Stream'Int)
dump: i as Stream'(Stream'String)

dump: bar2(i)
dump: baz2(i)

━━━━━━━━━━━━━━━━━━━━
+ bar1'T
    - list Iterable'(Iterable'T)

list

━━━━━━━━━━━━━━━━━━━━
+ baz1'T
    - list Iterable'(Stream'T)

list

━━━━━━━━━━━━━━━━━━━━
+ bar2'T
    - list Stream'(Iterable'T)

list

━━━━━━━━━━━━━━━━━━━━
+ baz2'T
    - list Stream'(Stream'T)

list

━━━━━━━━━━━━━━━━━━━━
+ do dump
    - x

-- type-of x
`  for list in x
`      for item in list
`          -- item
`  -- type-of x, joined
`    .. joined = ", ".join of (flat-map of x)
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

```yumi crash
. thing = Thing'Int()
-- thing as Quack'String'Int

━━━━━━━━━━━━━━━━━━━━
+ Thing'T
    is Bar'T

━━━━━━━━━━━━━━━━━━━━
+ Foo'T is Interface

━━━━━━━━━━━━━━━━━━━━
+ Bar'T is Interface

━━━━━━━━━━━━━━━━━━━━
+ Quack'A'B is Foo'B || Bar'A
```

```output
Exception: Thing'I64 cannot be cast to Quack'String'I64; there is no conversion between them
<no file:3>
    -- thing as Quack'String'Int
       ^^^^^^^^^^^^^^^^^^^^^^^^^
```

```yumi
. thing = Thing'Int()
-- thing as Quack'Int'Int
-- thing as Quack'Int'String

. wing = Wing'String()
-- wing as Quack'Int'String
-- wing as Quack'String'String

━━━━━━━━━━━━━━━━━━━━
+ Thing'T
    is Bar'T

━━━━━━━━━━━━━━━━━━━━
+ Wing'T
    is Foo'T

━━━━━━━━━━━━━━━━━━━━
+ Foo'T is Interface

━━━━━━━━━━━━━━━━━━━━
+ Bar'T is Interface

━━━━━━━━━━━━━━━━━━━━
+ Quack'A'B is Foo'B || Bar'A
```

```output
[Interface __source__:Bar'core/number:I64]
[Interface __source__:Bar'core/number:I64]
[Interface __source__:Foo'core/string:String]
[Interface __source__:Foo'core/string:String]
```

```yumi
foo((4, 5))

━━━━━━━━━━━━━━━━━━━━
+ do foo'T'V
    - n (x T, y V)

-- "hi", T, V
```

```output
(hi, [TypeInfo I64], [TypeInfo I64])
```

```yumi
foo(([1,2,3], 4))

━━━━━━━━━━━━━━━━━━━━
+ do foo'T
    - n (x Stream'T, y T)

-- n.x
-- "hi", T
```

```output
[Interface core/stream:Indexable'core/number:I64]
(hi, [TypeInfo I64])
```

```yumi
. (a Int, b Stream) = (42, [1,2,3])
-- type-of b
-- ",".join(b)
```

```output
[TypeInfo Indexable'I64]
1,2,3
```

### Constructor inference for ordinary generic tuple constructors stays concrete

```yumi
. pairs = [Pair("foo", 42)]
. typed = pairs as [Pair'String'Int]
-- typed[0].key
-- typed[0].value

━━━━━━━━━━━━━━━━━━━━
+ Pair'Key'Value is (key Key, value Value)
```

```output
foo
42
```

### A declared list return type materializes a generator without recursively compiling that same converter while ranking the list constructors. Bool keeps the recursive union on the conversion path that exposed the incomplete converter body.

```yumi
. values = collect(["hello", "goodbye"])
-- values.length
-- values[0] as String
-- values[1] as String

━━━━━━━━━━━━━━━━━━━━
+ Value is Union

| Bool
| String
| [Value]

━━━━━━━━━━━━━━━━━━━━
_ collect [Value]
    - source [String]

each as Value in source
```

```output
2
hello
goodbye
```

### A table whose arms return sibling subclasses, one of them optional, joins to their optional common base class, both under a declared return type and when inferred.

```yumi
-- describe(pick(0))
-- describe(pick(1))
-- describe(pick(2))
-- describe(pick-inferred(0))
-- describe(pick-inferred(1))
-- describe(pick-inferred(2))

━━━━━━━━━━━━━━━━━━━━
+ Animal

+ name = ""

━━━━━━━━━━━━━━━━━━━━
+ Cat is Animal

━━━━━━━━━━━━━━━━━━━━
+ Dog is Animal

━━━━━━━━━━━━━━━━━━━━
_ pick Animal?
    - index Int

|? index == 0
    cat
??
    dog

.. cat = named-cat()
.. dog = named-dog() if index == 1

━━━━━━━━━━━━━━━━━━━━
_ pick-inferred
    - index Int

|? index == 0
    named-cat()
??
    named-dog() if index == 1

━━━━━━━━━━━━━━━━━━━━
_ do named-cat

. cat = Cat()
cat.name = "tom"
= cat

━━━━━━━━━━━━━━━━━━━━
_ do named-dog

. dog = Dog()
dog.name = "rex"
= dog

━━━━━━━━━━━━━━━━━━━━
_ describe
    - animal Animal?

animal?.name ?? "none"
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

```yumi
-- once(half, 5i32)
-- once(half, 5.0)

━━━━━━━━━━━━━━━━━━━━
+ once'T
    - fn () T
        - n T
    - value T

fn(value)

━━━━━━━━━━━━━━━━━━━━
+ half I32
    - n Float

(n / 2) as I32

━━━━━━━━━━━━━━━━━━━━
+ double
    - n Int

n * 2
```

```output
2
2
```

### A closure with and without a stated argument type given with a value of its number: the value binds the type parameter.

```yumi
-- once(:: n => n / 2, 5)
-- once(:: n => n / 2, 5.0)
-- once(:: n Float => n / 2, 5.0)
-- once(:: n Int => n / 2, 5)

━━━━━━━━━━━━━━━━━━━━
+ once'T
    - fn () T
        - n T
    - value T

fn(value)
```

```output
2
2.5
2.5
2
```

### A function given with two values that bind a type parameter, of one number and of two.

```yumi
-- first(half, 5i32, 7)
-- first(half, 5.5, 7)
-- first(half, 5, 7.5)

━━━━━━━━━━━━━━━━━━━━
+ first'T
    - fn () T
        - n T
    - a T
    - b T

fn(a) + fn(b)

━━━━━━━━━━━━━━━━━━━━
+ half I32
    - n Float

(n / 2) as I32
```

```output
5
5
5
```

### A function given where a generic function type is taken with a value of another number.

```yumi
-- once(half, 5i32)

━━━━━━━━━━━━━━━━━━━━
+ once'T
    - fn Fn'T
    - value T

fn(value)

━━━━━━━━━━━━━━━━━━━━
+ Fn'T is () T
    - n T

━━━━━━━━━━━━━━━━━━━━
+ half I32
    - n Float

(n / 2) as I32
```

```output
2
```

### A function and a closure given with a value that binds one type parameter beside another parameter.

```yumi
-- pair(half, 5i32, "x")
-- pair(:: n Float => (n / 2) as I32, 5i32, "x")

━━━━━━━━━━━━━━━━━━━━
+ pair'T'U
    - fn () T
        - n T
    - value T
    - other U

"${fn(value)} $other"

━━━━━━━━━━━━━━━━━━━━
+ half I32
    - n Float

(n / 2) as I32
```

```output
2 x
2 x
```

### Two overloads that take a function and a value in either order, and a function that takes two functions, with a value of another number.

```yumi
-- apply(half, 5i32)
-- apply(5i32, half)
-- chain(half, half, 5i32)

━━━━━━━━━━━━━━━━━━━━
+ apply'T T
    - fn () T
        - n T
    - value T

fn(value)

━━━━━━━━━━━━━━━━━━━━
+ apply'T T
    - value T
    - fn () T
        - n T

fn(value) + 1

━━━━━━━━━━━━━━━━━━━━
+ chain'T T
    - a () T
        - n T
    - b () T
        - n T
    - value T

a(b(value))

━━━━━━━━━━━━━━━━━━━━
+ half I32
    - n Float

(n / 2) as I32
```

```output
2
3
1
```

### Closures that state another number than the value given, and a function called twice on its own answer.

```yumi
-- apply(:: n Int => (n + 1) as I32, 2i32)
-- apply(:: n Int => n + 1, 2)
-- twice(:: n Float => (n / 2) as I32, 5i32)
-- twice(half, 5i32)

━━━━━━━━━━━━━━━━━━━━
+ apply'T T
    - fn () T
        - n T
    - value T

fn(value)

━━━━━━━━━━━━━━━━━━━━
+ twice'T T
    - fn () T
        - n T
    - value T

fn(fn(value))

━━━━━━━━━━━━━━━━━━━━
+ half I32
    - n Float

(n / 2) as I32
```

```output
3
3
1
1
```

### Two functions of different numbers given with one value that binds the type parameter of both.

```yumi
-- both(half, double, 5i32)
-- both(double, half, 5i32)

━━━━━━━━━━━━━━━━━━━━
+ both'T
    - first () T
        - n T
    - second () T
        - n T
    - value T

second(first(value))

━━━━━━━━━━━━━━━━━━━━
+ half I32
    - n Float

(n / 2) as I32

━━━━━━━━━━━━━━━━━━━━
+ double I32
    - n Int

(n * 2) as I32
```

```output
4
5
```

### A type parameter only the function's answer names is bound by the function given.

```yumi
-- shown(label, 5)
-- shown(half, 5i32)
-- shown(:: n Float => n / 2, 5i32)

━━━━━━━━━━━━━━━━━━━━
+ shown'T'U U
    - fn () U
        - n T
    - value T

fn(value)

━━━━━━━━━━━━━━━━━━━━
+ label
    - n Int

"n $n"

━━━━━━━━━━━━━━━━━━━━
+ half
    - n Float

n / 2
```

```output
n 5
2.5
2.5
```

### A function of a generic class takes a function over its type parameter, given functions and closures of other numbers.

```yumi
. box = Box'I32(5)
-- box.through(half)
-- box.through(:: n Float => (n / 2) as I32)
-- box.through(:: n => n * 2)

━━━━━━━━━━━━━━━━━━━━
+ Box'T

+ held T

────────────────────
+ This
    - held

────────────────────
+ through T
    - fn () T
        - n T

fn(held)

━━━━━━━━━━━━━━━━━━━━
+ half I32
    - n Float

(n / 2) as I32
```

```output
2
2
10
```

### Closures and a function that take a tuple of other numbers than the function type gives

```yumi
get-funk(:: foo Foo'Float => foo.n + 42)
get-funk(:: foo Foo'F32 => foo.n * 2)
get-funk(wide)

━━━━━━━━━━━━━━━━━━━━
+ Foo'T is Tuple

+ n T

━━━━━━━━━━━━━━━━━━━━
+ Funk'TRet is () TRet
    - n Foo'Int

━━━━━━━━━━━━━━━━━━━━
+ do wide Float
    - foo Foo'Float

= foo.n / 2

━━━━━━━━━━━━━━━━━━━━
+ do get-funk
    - fn Funk

-- type-of fn
-- fn(Foo(1))
-- fn(Foo(7))
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

```yumi
show(:: both Both'Float => both.first.n + both.second.n + 0.5)

━━━━━━━━━━━━━━━━━━━━
+ Foo'T is Tuple

+ n T

━━━━━━━━━━━━━━━━━━━━
+ Both'T is Tuple

+ first Foo'T
+ second Foo'T

━━━━━━━━━━━━━━━━━━━━
+ Shower'TRet is () TRet
    - both Both'Int

━━━━━━━━━━━━━━━━━━━━
+ do show
    - fn Shower

-- fn(Both(Foo(1), Foo(2)))
```

```output
3.5
```

### Function type converts a tuple its function returns to one of other numbers

```yumi
. make Maker = :: n Int => Foo(n)
-- make(3).n
. wide Wider = make-int
-- wide(4).n

━━━━━━━━━━━━━━━━━━━━
+ Foo'T is Tuple

+ n T

━━━━━━━━━━━━━━━━━━━━
+ Maker is () Foo'Float
    - n Int

━━━━━━━━━━━━━━━━━━━━
+ Wider is () Foo'Float
    - n Int

━━━━━━━━━━━━━━━━━━━━
+ do make-int Foo'Int
    - n Int

= Foo(n + 1)
```

```output
3
5
```

### A function that states no return type and returns `null` under a condition returns an optional of what it returns otherwise.

```yumi
-- "${pick(1) ?? 0} ${pick(5) ?? 0}"

━━━━━━━━━━━━━━━━━━━━
_ do pick
    - n Int

=? null if n > 3
= n
```

```output
1 0
```

### A null given for an optional of a type parameter tells nothing of the type, which another argument binds.

```yumi
-- type-of pick(5, null)

━━━━━━━━━━━━━━━━━━━━
+ pick'T T
    - a T
    - b T?

a
```

```output
[TypeInfo I64]
```

### An integer given for an optional of a type parameter that another argument binds to a float is that float.

```yumi
. holder = Holder'F64(2.5)
-- pick(holder, 0, 1)
-- pick(holder, null, 3)

━━━━━━━━━━━━━━━━━━━━
+ Holder'Value

+ value Value

────────────────────
+ This
    - value

━━━━━━━━━━━━━━━━━━━━
+ pick'Value String
    - holder Holder'Value
    - from Value?
    - to Value?

"$from .. $to of ${holder.value} ${type-of from}"
```

```output
0 .. 1 of 2.5 [TypeInfo Optional'F64]
unknown .. 3 of 2.5 [TypeInfo Optional'F64]
```

### A float given for a type parameter that another argument binds to a unit of floats is that unit.

```yumi
. holder = Holder'Turns(1turn)
. half = 0.5
-- pick(holder, 0, 2.5)
-- pick(holder, null, half * 3)
-- one(holder, half / 2)

+ Turns (turn) is F64

━━━━━━━━━━━━━━━━━━━━
+ Holder'Value

+ value Value

────────────────────
+ This
    - value

━━━━━━━━━━━━━━━━━━━━
+ pick'Value String
    - holder Holder'Value
    - from Value?
    - to Value?

"$from .. $to ${type-of to}"

━━━━━━━━━━━━━━━━━━━━
+ one'Value String
    - holder Holder'Value
    - value Value

"$value ${type-of value}"
```

```output
0 .. 2.5 [TypeInfo Optional'Turns]
unknown .. 1.5 [TypeInfo Optional'Turns]
0.25 [TypeInfo Turns]
```

### The arguments alone bind a type parameter, and the default of a parameter of that type is cast to what they bind.

```yumi
-- scaled(2)
-- scaled(2.5)
-- scaled(2, 3)

━━━━━━━━━━━━━━━━━━━━
+ scaled'T String
    - a T
    - b T = 0.5

"$a $b"
```

```output
2 0
2.5 0.5
2 3
```
