# Tuples

```yumi
-- Tuple(1, 2)
```

```output
(1, 2)
```

```yumi
-- Tuple
    - 1
    - "abc"
```

```output
(1, abc)
```

```yumi
. a = foo(5)
-- a

━━━━━━━━━━━━━━━━━━━━
+ foo
    - n

n, n * 2
```

```output
(5, 10)
```

```yumi
. a = 1
. b = 2
a, b = foo(5)
-- a
-- b

━━━━━━━━━━━━━━━━━━━━
+ foo
    - n

n, n * 2
```

```output
5
10
```

```yumi
. a, b = foo(5)
-- a
-- b

━━━━━━━━━━━━━━━━━━━━
+ foo
    - n

n, n * 2
```

```output
5
10
```

```yumi
. a, b = foo(5)
-- a
-- b

━━━━━━━━━━━━━━━━━━━━
+ foo
    - n

a, b

.. a, b = 1, 2
```

```output
1
2
```

```yumi
. a = foo(5)
-- a[0]
-- a[1]

━━━━━━━━━━━━━━━━━━━━
+ foo
    - n

n, n * 2
```

```output
5
10
```

### Create new tuple by assignment

```yumi
. a = foo(5)
. b = a[0] = 3
-- b

━━━━━━━━━━━━━━━━━━━━
+ foo
    - n

n, n * 2
```

```output
(3, 10)
```

### Create new tuple by assignment with casting

```yumi
. a = foo(5.0)
. b = a[0] = 3
-- b

━━━━━━━━━━━━━━━━━━━━
+ foo
    - n

n, n * 2
```

```output
(3, 10)
```

### Create new tuple by tail assignment

```yumi
. a = foo(5)
. b = a[2] = 3
-- b

━━━━━━━━━━━━━━━━━━━━
+ foo
    - n

n, n * 2
```

```output
(5, 10, 3)
```

### Create new tuple by increment

```yumi
. a = foo(5)
. b = a += 3
-- b

━━━━━━━━━━━━━━━━━━━━
+ foo
    - n

n, n * 2
```

```output
(5, 10, 3)
```

### tuple truncation

```yumi
. a = 1, 2, 3, 4, 5
. b = a.length = 3
-- b
```

```output
(1, 2, 3)
```

### tuple length

```yumi crash
. a = 1, 2, 3, 4, 5
. n = 5
. b = a.length = n
-- b
```

```output
Exception: Value has no constant form
<no file:4>
    . b = a.length = n
          ^^^^^^^^
```

```yumi
. a = foo(5)
-- a.length

━━━━━━━━━━━━━━━━━━━━
+ foo
    - n

n, n * 2
```

```output
2
```

### nested tuple

```yumi
. a = (1, (2, 3), 4)
-- a.length
-- a[0]
-- a[1]
-- a[2]
```

```output
3
1
(2, 3)
4
```

```yumi crash
. a = foo(5)
-- a[2]

━━━━━━━━━━━━━━━━━━━━
+ foo
    - n

n, n * 2
```

```output
Exception: Index out of bounds
<no file:3>
    -- a[2]
       ^^^^
```

```yumi crash
. a = foo(5)
. n = 0
-- a[n]

━━━━━━━━━━━━━━━━━━━━
+ foo
    - n

n, n * 2
```

```output
Exception: Value has no constant form
<no file:4>
    -- a[n]
       ^^^^
```

```yumi
. thing = Thing()
-- thing

━━━━━━━━━━━━━━━━━━━━
+ Thing is (foo Int, bar Float)
```

```output
(0, 0)
```

```yumi
. thing = Thing(10, 20)
-- thing

━━━━━━━━━━━━━━━━━━━━
+ Thing is (foo Int, bar Float)
```

```output
(10, 20)
```

```yumi
. bling = Bling(10, 20)
-- bling

━━━━━━━━━━━━━━━━━━━━
+ Thing is (foo Int, bar Float)

━━━━━━━━━━━━━━━━━━━━
+ Bling is Thing
```

```output
(10, 20)
```

```yumi
. bling = Bling(10, 20, "blah")
-- bling

━━━━━━━━━━━━━━━━━━━━
+ Thing is (foo Int, bar Float)

━━━━━━━━━━━━━━━━━━━━
+ Bling is Thing

_ baz String
```

```output
(10, 20, blah)
```

### Tuple with base class from another module (to show it evaluates base in correct scope)

```yumi
./lib/foo

. bling = Bling()
-- bling

━━━━━━━━━━━━━━━━━━━━
+ Bling is Thing

_ baz Int
```

```yumi file=lib/foo.yumi

━━━━━━━━━━━━━━━━━━━━
+ Thing is Tuple

_ foo Foo
_ bar Float = goo()

━━━━━━━━━━━━━━━━━━━━
_ goo I32

42

━━━━━━━━━━━━━━━━━━━━
+ Foo is Tuple

_ bar Int = goo()

```

```output
((42), 42, 0)
```

```yumi
. thing = Thing(10)
-- thing

━━━━━━━━━━━━━━━━━━━━
+ Thing is (foo Int, bar Float)
```

```output
(10, 0)
```

```yumi
. thing = Thing(10)
-- thing

━━━━━━━━━━━━━━━━━━━━
+ Thing is Tuple

_ foo Int
_ bar Float = 20
```

```output
(10, 20)
```

### Refer to other fields in default values

```yumi
. thing = Thing(10)
-- thing

━━━━━━━━━━━━━━━━━━━━
+ Thing is Tuple

_ foo Int
_ zab Float = 20
_ bar Float = foo * zab
```

```output
(10, 20, 200)
```

### Type inference of default values for fields

```yumi
. thing = Thing(10)
-- thing

━━━━━━━━━━━━━━━━━━━━
+ Thing is Tuple

_ foo Int
_ zab Float = 20
_ bar = foo
```

```output
(10, 20, 10)
```

### Refer to other fields in default values of base tuple

```yumi
. thing = Bling()
-- thing

━━━━━━━━━━━━━━━━━━━━
+ Thing is Tuple

_ foo Int
_ bar Float = 20

━━━━━━━━━━━━━━━━━━━━
+ Bling is Thing

_ baz Float = bar
```

```output
(0, 20, 20)
```

```yumi
. thing = Thing()
-- thing

━━━━━━━━━━━━━━━━━━━━
+ Thing is Tuple

_ bar Float = 20
_ baz Float = bar

━━━━━━━━━━━━━━━━━━━━
+ Bling is Thing
```

```output
(20, 20)
```

### Refer to other fields out of order in default values

```yumi
. thing = Thing()
-- thing

━━━━━━━━━━━━━━━━━━━━
+ Thing is Tuple

_ zab Float = bar
_ bar Float = 20
```

```output
(20, 20)
```

### Circular reference in fields

```yumi crash
. thing = Thing()
-- thing

━━━━━━━━━━━━━━━━━━━━
+ Thing is Tuple

_ zab Float = bar
_ bar Float = zab
```

```output
Exception: "zab" depends on its own value, directly or through the values it reads; break the cycle
<no file:8>
    _ zab Float = bar
    ^^^^^^^^^^^^^^^^^
```

### Parameterized tuple type constructor

```yumi
. thing = Thing'Int(10, 20.0)
-- thing

━━━━━━━━━━━━━━━━━━━━
+ Thing'T is (foo T, bar Float)
```

```output
(10, 20)
```

### Parameterized tuple type inferred from constructor arguments

```yumi
. thing = Thing(10, 20.0)
-- thing

━━━━━━━━━━━━━━━━━━━━
+ Thing'T is (foo T, bar Float)
```

```output
(10, 20)
```

### Parameterized tuple type with base class constructor

```yumi
. thing = Thing'Int(10, 20, 30)
-- thing

━━━━━━━━━━━━━━━━━━━━
+ Bling'T is (foo T, bar Float)

━━━━━━━━━━━━━━━━━━━━
+ Thing'T is Bling'T

+ baz Float
```

```output
(10, 20, 30)
```

### Parameterized tuple type with base class inferred from constructor arguments

```yumi
. thing = Thing(10, 20, 30)
-- thing

+ Bling'T is (foo T, bar Float)

━━━━━━━━━━━━━━━━━━━━
+ Thing'T is Bling'T

+ baz T
```

```output
(10, 20, 30)
```

### Able to infer type if parameter is resolved in base type

```yumi
. thing = Thing(10, 20, 30)
-- thing

━━━━━━━━━━━━━━━━━━━━
+ Bling'T is (foo T, bar Float)

━━━━━━━━━━━━━━━━━━━━
+ Thing'T is Bling'T

+ baz Float
```

```output
(10, 20, 30)
```

### Tuple constructor re-routed to builtin constructor using "This"

```yumi
. thing = Thing(10.5)
-- thing

━━━━━━━━━━━━━━━━━━━━
+ Thing is (foo Int, bar Float)

────────────────────
+ This
    - xx Float

= This(1, xx)
```

```output
(1, 10.5)
```

```yumi
. thing = Thing(10.5, ...)
-- thing'Float()

━━━━━━━━━━━━━━━━━━━━
+ Thing'T is (foo Int, bar T)

────────────────────
+ This
    - n Float

= This(1, n)
```

```output
(1, 10.5)
```

```yumi
. thing = Thing(10, ...)
-- thing'Float()

━━━━━━━━━━━━━━━━━━━━
+ Thing'T is (foo Int, bar T)
```

```output
(10, 0)
```

### Replace tuple property (creates a new tuple)

```yumi
. thing = Thing()
. thing2 = thing.foo = 99
-- thing
-- thing2

━━━━━━━━━━━━━━━━━━━━
+ Thing is Tuple

+ foo I32
+ bar I32
```

```output
(0, 0)
(99, 0)
```

```yumi crash
. foo = Foo()
-- foo
. bar = Bar()
-- bar

━━━━━━━━━━━━━━━━━━━━
+ Foo is (bar Int, baz Int)

━━━━━━━━━━━━━━━━━━━━
+ Bar is Foo

+ bar Int
```

```output
Exception: "bar" is inherited from Foo, so it keeps the type it has there and cannot state its own; remove "Int"
<no file:13>
    + bar Int
    ^^^^^^^^^
```

### Casting tuple

```yumi
. n = (1, 2) as Pair
-- n
-- type-of n

━━━━━━━━━━━━━━━━━━━━
+ Pair is (a Int, b Float)
```

```output
(1, 2)
[TypeInfo Pair]
```

### Casting tuple to parameterized tuple

```yumi
. n = (1,2) as Pair'Int'Int
-- n
-- type-of n

━━━━━━━━━━━━━━━━━━━━
+ Pair'T1'T2 is (a T1, b T2)
```

```output
(1, 2)
[TypeInfo Pair'I64'I64]
```

### Casting tuple to parameterized tuple with parameters inferred

```yumi
. n = (1,2) as Pair
-- n
-- type-of n

━━━━━━━━━━━━━━━━━━━━
+ Pair'T1'T2 is (a T1, b T2)
```

```output
(1, 2)
[TypeInfo Pair'I64'I64]
```

```yumi
-- foo((1, 2))

━━━━━━━━━━━━━━━━━━━━
+ Pair'T1'T2 is (a T1, b T2)

━━━━━━━━━━━━━━━━━━━━
+ foo'T1'T2
    - pair Pair'T1'T2

pair.a, pair.b
```

```output
(1, 2)
```

### Implicit tuple constructors respect named arguments and default omitted fields

```yumi
. positional = Thing(10)
. named = Thing() • bar 20
-- positional
-- named

━━━━━━━━━━━━━━━━━━━━
+ Thing is (foo Int, bar Int, baz Int)
```

```output
(10, 0, 0)
(0, 20, 0)
```

### Implicit tuple constructors accept trailing boolean ability arguments

```yumi
. thing = Thing(7) • does complete
-- thing

━━━━━━━━━━━━━━━━━━━━
+ Thing is (index Int, does-complete Bool)
```

```output
(7, true)
```

### Override increment function

```yumi
. thing = Thing(20)
-- thing += 3

━━━━━━━━━━━━━━━━━━━━
+ Thing is Tuple

+ foo Int

────────────────────
+ this += other ''T

This(foo * other)
```

```output
(60)
```

```yumi
. thing = Thing(20)
-- thing + 3

━━━━━━━━━━━━━━━━━━━━
+ Thing is Tuple

+ foo Int

────────────────────
+ this + other ''T

This(foo * other)
```

```output
(60)
```

```yumi
. thing = Thing(20)
-- thing += 3

━━━━━━━━━━━━━━━━━━━━
+ Thing is Tuple

+ foo Int

────────────────────
+ this + other ''T

This(foo * other)
```

```output
(60)
```

```yumi
. foos = [Foo("hi", "you"), Foo("aa", "vv"), Foo("dd", "cc")]
-- ("dd", "cc") is in foos
-- ("dfd", "cc") is in foos
-- ("dd", "cc") not in foos
-- ("dfd", "cc") not in foos

━━━━━━━━━━━━━━━━━━━━
+ Foo is (a String, b String)
```

```output
true
false
false
true
```

```yumi
-- Foo("foo", 42) == Foo("foo", 42)
-- Foo("foo", 42) != Foo("foo", 42)
-- Foo("foo", 43) == Foo("foo", 42)
-- Foo("foo", 43) != Foo("foo", 42)

━━━━━━━━━━━━━━━━━━━━
+ Foo is (a String, b Int)
```

```output
true
false
false
true
```

```yumi
-- RecursiveComputedTuple(1) == RecursiveComputedTuple(1)

━━━━━━━━━━━━━━━━━━━━
+ RecursiveComputedTuple is Tuple

+ value Int

────────────────────
+ get child RecursiveComputedTuple

RecursiveComputedTuple(value)
```

```output
true
```

### Tests that comparing named tuples against each other does not use structural comparison

```yumi
-- match A("hi", 1)
    | B
        "B"
    | A
        "A"

━━━━━━━━━━━━━━━━━━━━
+ A is (a String, b Int)

━━━━━━━━━━━━━━━━━━━━
+ B is (a String, b Int)
```

```output
A
```

### Constructor-selected backing parameters produce exact tuple types with distinct layouts.

```yumi
. managed ConstructorLayoutBox'U32'(ManagedLayoutBacking'U32) = ConstructorLayoutBox'U32()
. native ConstructorLayoutBox'U32'(ArenaLayoutBacking'U32) = ConstructorLayoutBox'U32(LayoutArenaToken(), true)
-- type-of managed
-- type-of native
-- type-of managed.storage
-- type-of native.storage
-- managed.storage.label
-- native.storage.generation

━━━━━━━━━━━━━━━━━━━━
+ LayoutArenaToken

━━━━━━━━━━━━━━━━━━━━
+ ManagedLayoutBacking'Value is Tuple

+ value Value
+ label String = "managed"

━━━━━━━━━━━━━━━━━━━━
+ ArenaLayoutBacking'Value is Tuple

+ value Value
+ generation U64 = 7u64

━━━━━━━━━━━━━━━━━━━━
+ ConstructorLayoutBox'Value'Backing is Tuple
    - Backing = ManagedLayoutBacking'Value

+ storage Backing

────────────────────
+ This

storage = Backing()

────────────────────
+ This
    - token LayoutArenaToken
    - marker Bool
    - Backing = ArenaLayoutBacking'Value

storage = Backing()
```

```output
[TypeInfo ConstructorLayoutBox'U32'ManagedLayoutBacking'U32]
[TypeInfo ConstructorLayoutBox'U32'ArenaLayoutBacking'U32]
[TypeInfo ManagedLayoutBacking'U32]
[TypeInfo ArenaLayoutBacking'U32]
managed
7
```

### Tuple constructors retain property values returned by function calls.

```yumi
. box = CalledValueBox(41)
-- box.value

━━━━━━━━━━━━━━━━━━━━
+ CalledValueBox is Tuple

+ value Int

────────────────────
+ This
    - input Int

value = make-called-value(input)

━━━━━━━━━━━━━━━━━━━━
+ make-called-value Int
    - input Int

input + 1
```

```output
42
```

```yumi
. derived = NarrowPointSource()
. base PointSource = derived
-- derived.point
-- base.point
. tiny = TinyPointSource()
. middle NarrowPointSource = tiny
. root PointSource = tiny
-- tiny.point
-- middle.point
-- root.point

━━━━━━━━━━━━━━━━━━━━
+ PointSource

+ point => 0, 0

━━━━━━━━━━━━━━━━━━━━
+ NarrowPointSource is PointSource

+ point => 12i16, 27i16

━━━━━━━━━━━━━━━━━━━━
+ TinyPointSource is NarrowPointSource

+ point => -3i8, 4i8
```

```output
(12, 27)
(12, 27)
(-3, 4)
(-3, 4)
(-3, 4)
```

### A shorthand tuple with one named field declares a tuple, not a subtype of the field type.

```yumi
. single = Solo(7)
-- single.amount
-- single

━━━━━━━━━━━━━━━━━━━━
+ Solo is (amount I32)
```

```output
7
(7)
```

### A one-field tuple's own constructor wins over its conversion when the argument is the field type.

```yumi
. wrapped Wrapped = "hi"
-- wrapped
-- wrapped.text

━━━━━━━━━━━━━━━━━━━━
+ Wrapped is (text String)

────────────────────
+ from other String

Wrapped(other)
```

```output
(hi)
hi
```

### A tuple property assignment whose new tuple is discarded is an error.

```yumi crash
. record = Record(17)
record.flag = true

━━━━━━━━━━━━━━━━━━━━
+ Record is Tuple

+ size I32
+ flag = false
```

```output
Exception: Assigning flag creates a new Record tuple that is discarded; write record = record.flag = value
<no file:3>
    record.flag = true
    ^^^^^^^^^^^^^^^^^^
```

### A tuple property assignment whose new tuple is discarded is an error where its value has statements of its own.

```yumi crash
. record = Record(17)
record.flag = match record.size
    | 17
        . big = true
        big
    ??
        false

━━━━━━━━━━━━━━━━━━━━
+ Record is Tuple

+ size I32
+ flag = false
```

```output
Exception: Assigning flag creates a new Record tuple that is discarded; write record = record.flag = value
<no file:3>
    record.flag = match record.size
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### The error for a discarded tuple names the path its owner is read through.

```yumi crash
. outer = Outer(Record(17))
outer.record.flag = true

━━━━━━━━━━━━━━━━━━━━
+ Record is Tuple

+ size I32
+ flag = false

━━━━━━━━━━━━━━━━━━━━
+ Outer is Tuple

+ record Record
```

```output
Exception: Assigning flag creates a new Record tuple that is discarded; write outer.record = outer.record.flag = value
<no file:3>
    outer.record.flag = true
    ^^^^^^^^^^^^^^^^^^^^^^^^
```

### A class method cannot update a tuple property in place.

```yumi crash
Holder().touch()

━━━━━━━━━━━━━━━━━━━━
+ Record is Tuple

+ size I32

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ record = Record(3)

────────────────────
+ do touch

record.size = 9
```

```output
Exception: Assigning size creates a new Record tuple that is discarded; write record = record.size = value
<no file:17>
    record.size = 9
    ^^^^^^^^^^^^^^^
```

### Assigning the updated tuple back keeps the update.

```yumi
. record = Record(17)
record = record.flag = true
-- record
. copy = record.size = 3
-- copy

━━━━━━━━━━━━━━━━━━━━
+ Record is Tuple

+ size I32
+ flag = false
```

```output
(17, true)
(3, true)
```

### An expression function cannot return a tuple property assignment.

```yumi crash
-- flagged(Record(3))

━━━━━━━━━━━━━━━━━━━━
+ Record is Tuple

+ size I32
+ flag = false

━━━━━━━━━━━━━━━━━━━━
+ flagged Record
    - record Record

record.flag = true
```

```output
Exception: Assigning flag creates a new Record tuple that is discarded; write record = record.flag = value
<no file:14>
    record.flag = true
    ^^^^^^^^^^^^^^^^^^
```

### A local that holds a tuple is copied where it is assigned, in a loop and under a condition.

```yumi
. a = (1, "one")
. b = a
b = (2, "two")
-- a[0], a[1]
-- b[0], b[1]
. sum = 0
. i = 0
while i < 3
    . step = (i, i * 2)
    if step[1] > 1
        a = (step[0] + step[1], "more")
    sum += step[0] + step[1]
    i += 1
-- sum
-- a
```

```output
(1, one)
(2, two)
9
(6, more)
```

### A field of a tuple is read by a constant index, by name and through a path, and `length` counts its fields.

```yumi
. nested = ((1, 2), (3, (4, "deep")))
-- nested[0][1]
-- nested[1][1][1]
-- nested.length
-- nested[1].length
. named = Outer(Inner(5, 6), 7)
-- named.inner.b
-- named[0][0]
-- named.length

━━━━━━━━━━━━━━━━━━━━
+ Inner is (a Int, b Int)

━━━━━━━━━━━━━━━━━━━━
+ Outer is (inner Inner, last Int)
```

```output
2
deep
2
2
6
5
2
```

### An assignment to a field of a tuple gives a new tuple and leaves the old one as it was.

```yumi
. a = (5, 10)
. b = a[0] = 3
-- a
-- b
. r = Record(17)
. s = r.flag = true
-- r
-- s
r = r.size = 4
-- r

━━━━━━━━━━━━━━━━━━━━
+ Record is Tuple

+ size I32
+ flag = false
```

```output
(5, 10)
(3, 10)
(17, false)
(17, true)
(4, false)
```

### A tuple is extended by `+=` and by an assignment one past its last field, and truncated by an assignment to its length.

```yumi
. a = (5, "ten")
. b = a += 3.5
-- b
-- b.length
. c = a[2] = true
-- c
. d = (1, "two", 3.5, true, 5).length = 2
-- d
-- d.length
-- a
```

```output
(5, ten, 3.5)
3
(5, ten, true)
(1, two)
2
(5, ten)
```

### A tuple is taken apart into new locals and into old ones, and two locals are swapped.

```yumi
. a, b = (1, "one")
-- a
-- b
. x = 1
. y = 2
x, y = y, x
-- x, y
x, y = pair(x)
-- x, y
. first, second, third = (1.5, (2, 3), "last")
-- first
-- second
-- third

━━━━━━━━━━━━━━━━━━━━
+ pair (Int, Int)
    - n Int

n * 10, n * 100
```

```output
1
one
(2, 1)
(20, 200)
1.5
(2, 3)
last
```

### A tuple is taken apart into globals and task-locals that exist.

```yumi
. pair = (1u32, 2u16)
ga, gb = pair
ta, tb = pair
-- ga + gb
-- ta, tb

+ ga = 0u32
+ gb = 0u16
+ @task ta U32 = 0
+ @task tb U16 = 0
```

```output
3
(1, 2)
```

### A named tuple is made by its fields in order, by name, and with none; a field left out takes its default.

```yumi
-- Thing(1)
-- Thing(1, 2.5)
-- Thing(1, 2.5, "three")
-- Thing() • bar 4.5
-- Thing() • baz "six" • foo 5
-- Thing()
-- Pair(1, "x")
-- Wide(7)

━━━━━━━━━━━━━━━━━━━━
+ Thing is Tuple

+ foo Int
+ bar Float = 20
+ baz = "none"

━━━━━━━━━━━━━━━━━━━━
+ Pair'T1'T2 is Tuple

+ a T1
+ b T2

━━━━━━━━━━━━━━━━━━━━
+ Wide is Thing

+ last = 9
```

```output
(1, 20, none)
(1, 2.5, none)
(1, 2.5, three)
(0, 4.5, none)
(5, 20, six)
(0, 20, none)
(1, x)
(7, 20, none, 9)
```

### A tuple is passed and returned in registers when it is small or all floats, and through memory when it is not.

```yumi
-- small((1, 2))
-- words((1, 2, 3))
-- floats((1.5, 2.5, 3.5, 4.5))
-- singles((1.5f32, 2.5f32))
-- mixed((1, "two", 3.5))
-- large((1, 2, 3, 4, 5))
-- many(1, (2, 3), 4, (5, 6), 7, (8, 9), 10, (11, 12), (13, 14), 15)

━━━━━━━━━━━━━━━━━━━━
+ small (I32, I32)
    - v (I32, I32)

v[1], v[0]

━━━━━━━━━━━━━━━━━━━━
+ words (I64, I32, I8)
    - v (I8, I32, I64)

v[2], v[1], v[0]

━━━━━━━━━━━━━━━━━━━━
+ floats (F64, F64, F64, F64)
    - v (F64, F64, F64, F64)

v[3], v[2], v[1], v[0]

━━━━━━━━━━━━━━━━━━━━
+ singles (F32, F32)
    - v (F32, F32)

v[1], v[0]

━━━━━━━━━━━━━━━━━━━━
+ mixed (F64, String, Int)
    - v (Int, String, F64)

v[2], v[1], v[0]

━━━━━━━━━━━━━━━━━━━━
+ large (Int, Int, Int, Int, Int)
    - v (Int, Int, Int, Int, Int)

v[4], v[3], v[2], v[1], v[0]

━━━━━━━━━━━━━━━━━━━━
+ many I64
    - a I64
    - b (I64, I64)
    - c I64
    - d (I64, I64)
    - e I64
    - f (I64, I64)
    - g I64
    - h (I64, I64)
    - i (I64, I64)
    - j I64

a + b[0] * 2 + b[1] * 3 + c * 4 + d[0] * 5 + d[1] * 6 + e * 7 + f[0] * 8 + f[1] * 9 + g * 10 + h[0] * 11 + h[1] * 12 + i[0] * 13 + i[1] * 14 + j * 15
```

```output
(2, 1)
(3, 2, 1)
(4.5, 3.5, 2.5, 1.5)
(2.5, 1.5)
(3.5, two, 1)
(5, 4, 3, 2, 1)
1240
```

### A tuple is cast to a named tuple field by field, where it is written and where a type is declared.

```yumi
. n = (1, 2) as Pair
-- n
-- n.b
. p Pair = (3, 4)
-- p
-- wide((1, (2, 3)))
-- make()

━━━━━━━━━━━━━━━━━━━━
+ Pair is (a Int, b Float)

━━━━━━━━━━━━━━━━━━━━
+ make Pair

5, 6

━━━━━━━━━━━━━━━━━━━━
+ wide Float
    - t (Float, (Int, Float))

t[0] / 2 + t[1][1]
```

```output
(1, 2)
2
(3, 4)
3.5
(5, 6)
```

### A tuple is made text field by field, each by its own conversion, and a tuple inside it the same way.

```yumi
. t = (1, "two", 3.5, true, 'c')
-- t
-- "<$t>"
. nested = (t, (1, (2, "three")), Named(2, Named2("in")))
-- nested
-- 1, "two", (3, 4)

━━━━━━━━━━━━━━━━━━━━
+ Named is (count Int, inner Named2)

━━━━━━━━━━━━━━━━━━━━
+ Named2 is Tuple

+ text String

────────────────────
+ to String

"[$text]"
```

```output
(1, two, 3.5, true, c)
<(1, two, 3.5, true, c)>
((1, two, 3.5, true, c), (1, (2, three)), (2, [in]))
(1, two, (3, 4))
```

### Two tuples are equal when every field is, compared by the machine or by the `==` the field declares.

```yumi
. a = (1, "x", (2.5, true))
. b = (1, "x", (2.5, true))
. c = (1, "x", (2.5, false))
. d = (1, "y", (2.5, true))
-- a == b
-- a != b
-- a == c
-- a != c
-- a == d
-- Loose(1, 100) == Loose(1, 200)
-- (Loose(1, 100), 5) == (Loose(1, 200), 5)
-- (Loose(1, 100), 5) != (Loose(2, 100), 5)

━━━━━━━━━━━━━━━━━━━━
+ Loose is Tuple

+ key Int
+ note Int

────────────────────
+ this == other

key == other.key
```

```output
true
false
false
true
false
true
true
true
```

### An operand that names a tuple is copied before a later operand that changes it.

```yumi
+ g = (1, 2)

-- both(g, change())
-- g
g = (1, 2)
-- (g == (1, 2), change-flag())
-- (g, change())
-- g

━━━━━━━━━━━━━━━━━━━━
+ both String
    - first (Int, Int)
    - second (Int, Int)

"$first $second"

━━━━━━━━━━━━━━━━━━━━
+ do change (Int, Int)

g = (g[0] + 10, g[1] + 10)
= g

━━━━━━━━━━━━━━━━━━━━
+ do change-flag Bool

g = (0, 0)
= true
```

```output
(1, 2) (11, 12)
(11, 12)
(true, true)
((0, 0), (10, 10))
(10, 10)
```

### `Tuple` called with values is the tuple of them.

```yumi
. t = Tuple(1, "two", 3.5)
-- t
-- t[1]
-- Tuple(1, 2) == (1, 2)
```

```output
(1, two, 3.5)
two
true
```

### Two tuples whose fields have other names are not equal.

```yumi
-- Named(1, 2) == (1, 2)
-- (1, 2) == Named(1, 2)
-- Named(1, 2) == Other(1, 2)
-- Named(1, 2) != Other(1, 2)
-- Named(1, 2) == Named(1, 2)
-- (1, 2) == (1, 2)

━━━━━━━━━━━━━━━━━━━━
+ Named is (a Int, b Int)

━━━━━━━━━━━━━━━━━━━━
+ Other is (c Int, d Int)
```

```output
false
false
false
true
true
true
```

### A field left out of a named tuple takes its class's value; a tuple with no name has no bits set.

```yumi
-- Holder(1)
-- Holder()

━━━━━━━━━━━━━━━━━━━━
+ Part is Struct

+ x I32 = 7
+ y I32

━━━━━━━━━━━━━━━━━━━━
+ Holder is Tuple

+ n Int
+ part Part
+ loose (Part, Int)
+ made Part = Part(1, 2)
```

```output
(1, (7, 0), ((0, 0), 0), (1, 2))
(0, (7, 0), ((0, 0), 0), (1, 2))
```

### A tuple's constructor assigns its fields.

```yumi
. t = Trip(1)
-- t.a + t.b + t.c
-- Trip(4).c
-- t

━━━━━━━━━━━━━━━━━━━━
+ Trip is Tuple

+ a Int
+ b Int
+ c Int

────────────────────
+ This
    - n Int

a = n
b = n + 1
c = b + 1
```

```output
6
6
(1, 2, 3)
```

### What the function of an assigning operator answers for a tuple is stored where the tuple is held.

```yumi
. pair = Pair(1, 2)
pair += 3
-- pair.a
-- pair.b
-- (pair += 1).a
-- pair.b
. holder = Holder()
holder.held += 5
-- holder.held.a
-- holder.held.b

━━━━━━━━━━━━━━━━━━━━
+ Pair is Tuple

+ a I32
+ b I32

────────────────────
+ this += other I32

This(a + other, b + other)

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ held = Pair(1, 1)
```

```output
4
5
5
6
6
6
```

### A tuple called with one value of its only field's type is constructed from it, and any other single value goes through the function that converts from it.

```yumi
. made = Wrapped("made")
-- made.text
. cast Wrapped = "cast"
-- cast.text
. other = Count("four")
-- other.n

━━━━━━━━━━━━━━━━━━━━
+ Wrapped is (text String)

────────────────────
+ from other String

Wrapped("$other!")

━━━━━━━━━━━━━━━━━━━━
+ Count is (n Int)

────────────────────
+ from other String

Count(other.size)
```

```output
made
cast!
4
```

### A tuple or struct declared with no value has the first values of its fields.

```yumi
. thing Thing
-- thing.a
. pair Pair
-- pair
. set Preset
-- set
. deep Deep
-- deep

━━━━━━━━━━━━━━━━━━━━
+ Thing is (a Int, b CPointer'Char8)

━━━━━━━━━━━━━━━━━━━━
+ Pair is (x Int, y Float)

━━━━━━━━━━━━━━━━━━━━
+ Preset is Tuple

_ x Int = 3
_ y Float = 1.5

━━━━━━━━━━━━━━━━━━━━
+ Deep is (pair Pair, name String, flag Bool)
```

```output
0
(0, 0)
(3, 1.5)
((0, 0), , false)
```

### A field's first value reads other fields, in whatever order they are declared, with the arguments the call gave.

```yumi
-- Thing(10)
-- Thing(10, 2)
-- Thing(10, 2, 3)
-- Later()
-- Later(5)
-- Child()
-- Holder().inner
-- Chain()
-- Chain(5)
-- Made(3)
-- Made(3).wide
. local Later
-- local

━━━━━━━━━━━━━━━━━━━━
+ Thing is Tuple

_ foo Int
_ zab Float = 20
_ bar Float = foo * zab

━━━━━━━━━━━━━━━━━━━━
+ Later is Struct

+ zab I32 = bar
+ bar I32 = 20

━━━━━━━━━━━━━━━━━━━━
+ Base is Struct

+ foo I32 = 1
+ bar I32 = 2

━━━━━━━━━━━━━━━━━━━━
+ Child is Base

+ baz I32 = bar + 1

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ scale = 3
+ inner = Thing(scale)

━━━━━━━━━━━━━━━━━━━━
+ Chain is Tuple

_ a Int = b + 1
_ b Int = c + 1
_ c Int = 1
_ d (Int, Int) = (a, c)

━━━━━━━━━━━━━━━━━━━━
+ Made is Struct

+ n I32
+ wide I64 = n * 2
+ text = "n$n"

────────────────────
+ This
    - n I32

n += 1
```

```output
(10, 20, 200)
(10, 2, 20)
(10, 2, 3)
(20, 20)
(5, 20)
(1, 2, 3)
(3, 20, 60)
(3, 2, 1, (3, 1))
(5, 2, 1, (5, 1))
(3, 6, n3)
6
(20, 20)
```

### An assignment to an argument that is a tuple changes the function's own copy.

```yumi
. original = Pair(1, 2)
-- rebind(original)
-- original
-- keep(original, true)
-- keep(original, false)
-- original
-- grow(original)
-- original
. holder = Holder()
-- holder.swap(original)
-- original

━━━━━━━━━━━━━━━━━━━━
+ Pair is (a I64, b I64)

━━━━━━━━━━━━━━━━━━━━
+ do rebind I64
    - value Pair

value = Pair(9, 8)
= value.a + value.b

━━━━━━━━━━━━━━━━━━━━
+ do keep I64
    - value Pair
    - replace Bool

if replace
    value = Pair(10, 9)
= value.a + value.b

━━━━━━━━━━━━━━━━━━━━
+ do grow I64
    - value Pair

for index in 0 .. < 3
    value = Pair(value.a + 1, value.b)
= value.a

━━━━━━━━━━━━━━━━━━━━
+ Holder

────────────────────
+ do swap Pair
    - value Pair

value = Pair(value.b, value.a)
= value
```

```output
17
(1, 2)
19
3
(1, 2)
4
(1, 2)
(2, 1)
(1, 2)
```

### A field's value is evaluated for a tuple and for a tuple derived from it.

```yumi
. plain = Thing()
-- plain
. bling = Bling()
-- bling

━━━━━━━━━━━━━━━━━━━━
+ Thing is Tuple

_ bar Int = goo()

━━━━━━━━━━━━━━━━━━━━
+ Bling is Thing

_ baz Int

━━━━━━━━━━━━━━━━━━━━
_ goo

42
```

```output
(42)
(42, 0)
```

### A tuple with no name declared with no value has no bits set, whatever its fields' types write for theirs.

```yumi
. o (Int, S)
-- o
. p (S, S)
-- p
. q (Int, T)
-- q
. r (Int, P)
-- r
-- "[${r[1].y}]"
. named S
-- named

━━━━━━━━━━━━━━━━━━━━
+ S is Struct

+ a I32 = 3
+ b F64

━━━━━━━━━━━━━━━━━━━━
+ T is Tuple

+ a I32 = 3
+ b F64

━━━━━━━━━━━━━━━━━━━━
+ P is (x Int, y String)
```

```output
(0, (0, 0))
((0, 0), (0, 0))
(0, (0, 0))
(0, (0, ))
[]
(3, 0)
```

### A property that holds a tuple and has no value is made by the tuple's constructor, with the defaults it declares

```yumi
/core/arena

. holder = Holder()
-- holder.size()
holder.release()

━━━━━━━━━━━━━━━━━━━━
+ Holder

_ arena Arena
_ numbers Array'U32'ArenaBacking

────────────────────
+ This

.. numbers = Array'U32(arena, 8)

────────────────────
+ size Size

numbers.length

────────────────────
+ do release

arena.release()
```

```output
8
```

### Properties that hold tuples and have no values: one with a constructor written, one without

```yumi
. holder = Holder()
-- holder.counter.count
-- holder.plain.left

━━━━━━━━━━━━━━━━━━━━
+ Counter is Tuple

+ count = 1
+ label = "plain"

────────────────────
+ This
    - start = 40

count = start + 2
label = "made"

━━━━━━━━━━━━━━━━━━━━
+ Plain is Tuple

+ left = 7
+ right = 8

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ counter Counter
+ plain Plain

━━━━━━━━━━━━━━━━━━━━
+ Inner is Tuple

+ counter Counter
+ extra = 3

━━━━━━━━━━━━━━━━━━━━
+ Nest

+ inner Inner
```

```output
42
7
```

### Globals with no values, of a class and of a tuple that write a constructor

```yumi
-- kept.count
-- tally.count

_ kept Counter
_ tally Tally

━━━━━━━━━━━━━━━━━━━━
+ Counter

+ count = 1

────────────────────
+ This

count = 9

━━━━━━━━━━━━━━━━━━━━
+ Tally is Tuple

+ count = 1

────────────────────
+ This

count = 8
```

```output
9
8
```

### Tuple fields that hold objects with no value written

```yumi
. pair = Pair()
-- pair.left.label
-- pair.right.label
-- pair.size
. named = Pair(Foo("l"))
-- named.left.label
-- named.right.label

━━━━━━━━━━━━━━━━━━━━
+ Pair is Tuple

+ left Foo
+ right Foo
+ size Int = 2

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ act String
    - x Int

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ label String

────────────────────
+ This
    - given String = "foo"

.. label = given

────────────────────
+ do act String
    - x Int

= "$label $x"
```

```output
foo
foo
2
l
foo
```

### Fields of a tuple written in parentheses that hold an object, an array and a list with no value written are made by their types' constructors, for a tuple declared with no value and for one given some of its fields.

```yumi
. pair Pair
-- pair.holder.label
-- pair.bytes.size
-- pair.items.length
. given = Pair(2)
-- given.count
-- given.holder.label
-- given.items.length

━━━━━━━━━━━━━━━━━━━━
+ Pair is (count Int, holder Foo, bytes Byte[], items [Int])

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ label String

────────────────────
+ This
    - given String = "foo"

.. label = given
```

```output
foo
0
0
2
foo
0
```

### Fields of another module's tuple written in parentheses that hold an array and a list with no value written are made by their types' constructors.

```yumi
./lib/pair

. pair Pair
-- pair.bytes.size
-- pair.items.length
. given = Pair(3)
-- given.count
-- given.items.length
```

```yumi file=lib/pair.yumi

━━━━━━━━━━━━━━━━━━━━
+ Pair is (count Int, bytes Byte[], items [Int])

```

```output
0
0
3
0
```

### A function prints several values, one an interface value

```yumi
show(Thing())

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is IFoo

━━━━━━━━━━━━━━━━━━━━
+ IFoo is Interface

━━━━━━━━━━━━━━━━━━━━
+ do show
    - thing IFoo

-- "shown", thing
```

```output
(shown, [Interface __source__:IFoo])
```

### A function prints several values, one an object

```yumi
show(Thing())

━━━━━━━━━━━━━━━━━━━━
+ Thing

━━━━━━━━━━━━━━━━━━━━
+ do show
    - thing Thing

-- "shown", thing
```

```output
(shown, [Object __source__:Thing])
```

### A tuple nested in a tuple is taken apart, by a declaration and by a lazy one.

```yumi
. (a, (b, c)) = ("x", (1, "y"))
-- a
-- b
-- c
.. p, (q, r) = ("z", (2, "w"))
-- r
-- q
```

```output
x
1
y
w
2
```

### A generic tuple named bare in a cast and a declaration

```yumi
. n = (1, "two") as Pair
-- n.b
-- type-of n
. m Pair = (3, 4)
-- m
-- type-of m

━━━━━━━━━━━━━━━━━━━━
+ Pair'T1'T2 is (a T1, b T2)
```

```output
two
[TypeInfo Pair'I64'String]
(3, 4)
[TypeInfo Pair'I64'I64]
```

### A generic tuple named bare declared with a list field

```yumi
. m Pair = ([1], "x")
-- type-of m
-- m.b

━━━━━━━━━━━━━━━━━━━━
+ Pair'T1'T2 is (a T1, b T2)
```

```output
[TypeInfo Pair'List'I64'GCBacking'String]
x
```

### A generic tuple named bare as a return type

```yumi
. m = make()
-- type-of m
-- m

━━━━━━━━━━━━━━━━━━━━
+ Pair'T1'T2 is (a T1, b T2)

━━━━━━━━━━━━━━━━━━━━
+ do make Pair

= (1, "two")
```

```output
[TypeInfo Pair'I64'String]
(1, two)
```

### A generic tuple named bare cast from fields read from another

```yumi
. q = (1, 2) as Pair
. r = (q.a + 1, q.b * 2) as Pair
-- r
. s = (1.5, 2) as Pair
-- type-of s

━━━━━━━━━━━━━━━━━━━━
+ Pair'T1'T2 is (a T1, b T2)
```

```output
(2, 4)
[TypeInfo Pair'F64'I64]
```

### A generic tuple named bare whose field is a tuple of its parameter

```yumi
. p = ((1, 2), 3) as Wrap
-- type-of p
-- p.inner

━━━━━━━━━━━━━━━━━━━━
+ Wrap'T is (inner (T, T), extra T)
```

```output
[TypeInfo Wrap'I64]
(1, 2)
```

### A generic tuple named bare whose field is a list of its parameter

```yumi
. p = ([1, 2], 3) as Bag
-- type-of p
-- p.extra

━━━━━━━━━━━━━━━━━━━━
+ Bag'T is (items [T], extra T)
```

```output
[TypeInfo Bag'I64]
3
```

### A tuple parameter whose field is an interface given a list

```yumi
foo(([1, 2], 3))

━━━━━━━━━━━━━━━━━━━━
+ do foo
    - n (x Iterable'Int, y Int)

for v in n.x
    -- v
-- n.y
```

```output
1
2
3
```

### A tuple parameter whose two fields are streams

```yumi
foo(([1, 2], ["a", "b"]))

━━━━━━━━━━━━━━━━━━━━
+ do foo
    - n (x Stream'Int, y Stream'String)

-- type-of n.x
-- type-of n.y
for v in n.y
    -- v
```

```output
[TypeInfo Indexable'I64]
[TypeInfo Indexable'String]
a
b
```

### A nested tuple parameter whose field is an interface

```yumi
foo(((1, [2, 3]), 4))

━━━━━━━━━━━━━━━━━━━━
+ do foo
    - n (p (a Int, b Iterable'Int), c Int)

for v in n.p.b
    -- v
```

```output
2
3
```

### A binding of a generic constructor called with two parameters in turn

```yumi
. mk = Thing(1, ...)
. a = mk'Int()
. b = mk'String()
-- a
-- b
-- type-of a
-- type-of b

━━━━━━━━━━━━━━━━━━━━
+ Thing'T is (foo Int, bar T)
```

```output
(1, 0)
(1, )
[TypeInfo Thing'I64]
[TypeInfo Thing'String]
```

### A binding of a constructor of two parameters that holds the first field

```yumi
. mk = Duo(3, ...)
-- mk'Int'Bool()
-- mk'Int'Bool()

━━━━━━━━━━━━━━━━━━━━
+ Duo'A'B is (c Int, a A, b B)
```

```output
(3, 0, false)
(3, 0, false)
```

### A binding of a generic constructor whose held argument converts

```yumi
. mk = Thing(10u8, ...)
. t = mk'Float()
-- t.bar
-- t.foo

━━━━━━━━━━━━━━━━━━━━
+ Thing'T is (foo Int, bar T)
```

```output
0
10
```

### A tuple whose field holds a number class with its own == compares that field by it.

```yumi
. a = Holder(Counter(1), 2)
. b = Holder(Counter(1), 2)
-- a == b

━━━━━━━━━━━━━━━━━━━━
+ Counter is I32

────────────────────
+ do this == other Counter

-- "counter compare"
= false

━━━━━━━━━━━━━━━━━━━━
+ Holder is (count Counter, other I32)
```

```output
counter compare
false
```

### A number class with its own == is compared by it, not by its machine value.

```yumi
. a = Counter(1)
. b = Counter(1)
-- a == b

━━━━━━━━━━━━━━━━━━━━
+ Counter is I32

────────────────────
+ do this == other Counter

-- "counter compare"
= false
```

```output
counter compare
false
```

### A binding of a generic tuple's constructor that holds nothing makes the default once its parameter is written.

```yumi
. mk = Thing(...)
. t = mk'Int()
-- t

━━━━━━━━━━━━━━━━━━━━
+ Thing'T is (a T, b T)
```

```output
(0, 0)
```

### A binding of a generic tuple's constructor takes its parameters and no argument.

```yumi crash
. mk = Thing(...)
. t = mk'Float(1.5, 2.5)
-- t
-- type-of t

━━━━━━━━━━━━━━━━━━━━
+ Thing'T is (a T, b T)
```

```output
Exception: Illegal arguments to binding
<no file:3>
    . t = mk'Float(1.5, 2.5)
          ^^^^^^^^^^^^^^^^^^
```

### A binding of a generic tuple's constructor that holds an argument takes no more.

```yumi crash
. mk = Trio(1, ...)
-- mk'Int(5, 6)
-- mk'Float(5, 6.5)

━━━━━━━━━━━━━━━━━━━━
+ Trio'T is (a Int, b Int, c T)
```

```output
Exception: Illegal arguments to binding
<no file:3>
    -- mk'Int(5, 6)
       ^^^^^^^^^^^^
```

### A tuple class named bare takes the join of the types its fields give one parameter.

```yumi
. p = (1, 2.5) as Same
-- type-of p
-- p
. q = (1u8, 300) as Same
-- type-of q
-- q

━━━━━━━━━━━━━━━━━━━━
+ Same'T is (a T, b T)
```

```output
[TypeInfo Same'F64]
(1, 2.5)
[TypeInfo Same'I64]
(1, 300)
```

### A declared list of a tuple class named bare is completed from its items.

```yumi
. ps [Pair] = [(1, "x"), (2, "y")]
-- type-of ps
-- ps[0].a + 10
-- ps[1].b
. qs [Pair?] = [(3, "z"), null]
-- type-of qs

━━━━━━━━━━━━━━━━━━━━
+ Pair'T1'T2 is (a T1, b T2)
```

```output
[TypeInfo List'Pair'I64'String'GCBacking]
11
y
[TypeInfo List'Optional'Pair'I64'String'GCBacking]
```

### A tuple compared field by field compares a field of a function type as one object.

```yumi
. f = => 1
-- Holder(f, 1) == Holder(f, 1)
-- Holder(f, 1) == Holder(f, 2)

━━━━━━━━━━━━━━━━━━━━
+ Holder is (fn () Int, n I32)
```

```output
true
false
```

```yumi
/BxGC/BxGCTest.h

. local = Wide(1, 2, 3, 4, 5, 6, 7, 8)
. alias = local
. optional Wide? = local
. values = [local, Wide(8, 7, 6, 5, 4, 3, 2, 2)]
. callback WideCallback = :: value Wide => sum(value)
-- sum(local)
-- sum(alias)
-- sum(optional!)
-- sum(values[0])
-- sum(values[1])
-- callback(local)
-- catch? sum-fallible(local)
-- sum-after-collection(local)
-- sum(local.shifted(1))
-- local

━━━━━━━━━━━━━━━━━━━━
+ Wide is Tuple

+ a Float
+ b Float
+ c Float
+ d Float
+ e Float
+ f Float
+ g Float
+ h Float

────────────────────
+ do shifted Wide
    - amount Float

= Wide
    - a + amount
    - b + amount
    - c + amount
    - d + amount
    - e + amount
    - f + amount
    - g + amount
    - h + amount

━━━━━━━━━━━━━━━━━━━━
+ WideCallback is () Float
    - value Wide

━━━━━━━━━━━━━━━━━━━━
_ sum Float
    - value Wide

value.a + value.b + value.c + value.d + value.e + value.f + value.g + value.h

━━━━━━━━━━━━━━━━━━━━
_ do sum-fallible Float ?! Error
    - value Wide

= sum(value)

━━━━━━━━━━━━━━━━━━━━
_ do sum-after-collection Float
    - value Wide

C:brixCollect()
= sum(value) + 1
```

```output
36
36
36
36
37
36
36
37
44
(1, 2, 3, 4, 5, 6, 7, 8)
```

```yumi
. nested = Nested
    - Pair(1, 2)
    - Pair(3, 4)
    - Pair(5, 6)
    - Pair(7, 8)
-- sum-nested(nested)
-- sum-ten(Ten(1, 2, 3, 4, 5, 6, 7, 8, 9, 10))
-- sum-one(One(36))
-- sum-pair(Pair(17, 19))
-- sum-four(Four(6, 8, 10, 12))

━━━━━━━━━━━━━━━━━━━━
+ One is Tuple

+ a Float

━━━━━━━━━━━━━━━━━━━━
+ Pair is Tuple

+ a Float
+ b Float

━━━━━━━━━━━━━━━━━━━━
+ Four is Tuple

+ a Float
+ b Float
+ c Float
+ d Float

━━━━━━━━━━━━━━━━━━━━
+ Ten is Tuple

+ a Float
+ b Float
+ c Float
+ d Float
+ e Float
+ f Float
+ g Float
+ h Float
+ i Float
+ j Float

━━━━━━━━━━━━━━━━━━━━
+ Nested is Tuple

+ a Pair
+ b Pair
+ c Pair
+ d Pair

━━━━━━━━━━━━━━━━━━━━
_ sum-one Float
    - value One

value.a

━━━━━━━━━━━━━━━━━━━━
_ sum-pair Float
    - value Pair

value.a + value.b

━━━━━━━━━━━━━━━━━━━━
_ sum-four Float
    - value Four

value.a + value.b + value.c + value.d

━━━━━━━━━━━━━━━━━━━━
_ sum-ten Float
    - value Ten

value.a + value.b + value.c + value.d + value.e + value.f + value.g + value.h + value.i + value.j

━━━━━━━━━━━━━━━━━━━━
_ sum-nested Float
    - value Nested

sum-pair(value.a) + sum-pair(value.b) + sum-pair(value.c) + sum-pair(value.d)
```

```output
36
55
36
36
36
```

```yumi
/BxGC/BxGC.h

. nested-value = nested("live").value
-- nested-value!.a
-- reused("same").right.a
. interrupted-value = interrupted("safe").value
-- interrupted-value!.a

+ Text is (a String, b String, c String, d String)
+ Box is (value Text?, marker U64)
+ Pair is (left Text, right Text)

━━━━━━━━━━━━━━━━━━━━
_ do nested Box
    - seed String

. result = Box(Text(seed, "b", "c", "d"), 7)
C:brixCollect()
= result

━━━━━━━━━━━━━━━━━━━━
_ do reused Pair
    - seed String

. text = Text(seed, "b", "c", "d")
. result = Pair(text, text)
C:brixCollect()
= result

━━━━━━━━━━━━━━━━━━━━
_ do interrupted Box
    - seed String

. text = Text(seed, "b", "c", "d")
C:brixCollect()
. result = Box(text, 8)
= result
```

```output
live
same
safe
```

```yumi
/BxGC/BxGC.h

. wrapped = wrap(1)
. base = Big(1 as U64, 2 as U64, 3 as U64, 4 as U64)
. forwarded = forward-input(base)
. text = wrap-text("a")
C:brixCollect()
-- wrapped!.a
-- wrapped!.b
-- wrapped!.c
-- wrapped!.d
-- forwarded.a
-- text!.a
-- text!.d

+ Big is (a U64, b U64, c U64, d U64)
+ BigText is (a String, b String, c String, d String)

━━━━━━━━━━━━━━━━━━━━
_ make Big
    - seed U64

Big(seed, 2 as U64, 3 as U64, 4 as U64)

━━━━━━━━━━━━━━━━━━━━
_ wrap Big?
    - seed U64

make(seed)

━━━━━━━━━━━━━━━━━━━━
_ return-input Big
    - value Big

value

━━━━━━━━━━━━━━━━━━━━
_ forward-input Big
    - value Big

return-input(value)

━━━━━━━━━━━━━━━━━━━━
_ make-text BigText
    - seed String

BigText(seed, "b", "c", "d")

━━━━━━━━━━━━━━━━━━━━
_ wrap-text BigText?
    - seed String

make-text(seed)
```

```output
1
2
3
4
1
a
d
```

```yumi
-- Pair("a", null)
-- Pair("b", "c")

+ Pair is (name String, detail String?)
```

```output
(a, unknown)
(b, c)
```

```yumi
. owner = Owner(Box(3))
-- owner.held.box.n

━━━━━━━━━━━━━━━━━━━━
+ Owner

_ inner Pair

────────────────────
+ This
    - box Box

-- inner.box.n
inner = Pair(box)

────────────────────
+ get held Pair

inner

━━━━━━━━━━━━━━━━━━━━
+ Pair is Tuple

+ box Box

────────────────────
+ This
    - box Box

━━━━━━━━━━━━━━━━━━━━
+ Box

+ n I32 = 7

────────────────────
+ This
    - n I32
```

```output
7
3
```

```yumi
. h = Holder()
-- h.inner.n

━━━━━━━━━━━━━━━━━━━━
+ Holder is Tuple

+ inner Inner

━━━━━━━━━━━━━━━━━━━━
+ Inner is Tuple

+ n Size

────────────────────
+ This
    - start Size = 4

n = start + 1
```

```output
5
```

### A tuple given where one of other field types is taken converts each field by its place.

```yumi
-- show(make(1, 7))
-- show(make(2, 9))
-- show(make(5, 0))

━━━━━━━━━━━━━━━━━━━━
+ Id is Int

━━━━━━━━━━━━━━━━━━━━
+ Partial is (container Id, count Int)

━━━━━━━━━━━━━━━━━━━━
+ make Partial
    - count Int
    - container Id

(count, container)

━━━━━━━━━━━━━━━━━━━━
+ show String
    - partial Partial

describe(pick(container), count)

.. count, container = partial

━━━━━━━━━━━━━━━━━━━━
+ pick Id
    - value Id?

value ?? 0

━━━━━━━━━━━━━━━━━━━━
+ describe String
    - id Id
    - count Int

"$id $count"
```

```output
7 1
9 2
0 5
```

### A function value a tuple pattern binds is called by its name.

```yumi
. b = Boxed(:: n Int => "k$n")
-- b.key(3, "p")
-- Boxed(null).key(4, "q")
-- Boxed(null).key(5, "")

━━━━━━━━━━━━━━━━━━━━
+ Keyer is () String
    - n Int

━━━━━━━━━━━━━━━━━━━━
+ Boxed

+ keyer Keyer?

────────────────────
+ This
    - keyer Keyer?

────────────────────
+ key String
    - n Int
    - prefix String

match keyer, prefix
    | f Keyer, _
        f(n)
    | _, {it}
        "$prefix/$n"
    ??
        "$n"
```

```output
k3
q/4
5
```

### A tuple made with one field given by name gives the fields before it their values.

```yumi
. p = Pair
    • b 4
-- p.a

━━━━━━━━━━━━━━━━━━━━
+ Pair is Tuple

+ a Size = 5
+ b Size = 0
+ c Size = 0

━━━━━━━━━━━━━━━━━━━━
_ both Size
    - first Size
    - second Size

first + second
```

```output
5
```

### The value of a field of a tuple calls with arguments written as lines and reads fields declared after it.

```yumi
. u = Usage
    • reasoning-tokens 3
-- u.total-tokens ?? 0
-- u.input-tokens ?? 0

━━━━━━━━━━━━━━━━━━━━
+ Usage is Tuple

+ input-tokens Size? = usage-sum
    - cached-input-tokens
    - tool-prompt-tokens
+ output-tokens Size? = usage-sum(reasoning-tokens, thought-tokens)
+ total-tokens Size? = usage-sum(input-tokens, output-tokens)
+ cached-input-tokens Size? = null
+ reasoning-tokens Size? = null
+ thought-tokens Size? = null
+ tool-prompt-tokens Size? = null

━━━━━━━━━━━━━━━━━━━━
_ usage-sum Size?
    - first Size?
    - second Size?

|? first is Size || second is Size
    (first ?? 0) + (second ?? 0)
??
    null
```

```output
3
0
```

### Assigning a field of a tuple as the value of an arm gives the new tuple.

```yumi
-- describe(with-field(Pair(1, 2), "left", 7))
-- describe(with-field(Pair(1, 2), "right", 8))
-- describe(with-field(Pair(1, 2), "other", 9))

━━━━━━━━━━━━━━━━━━━━
+ Pair is Tuple

+ left U64
+ right U64

────────────────────
+ This
    - left
    - right

━━━━━━━━━━━━━━━━━━━━
+ describe String
    - pair Pair?

if pair then "${pair.left} ${pair.right}" else "none"

━━━━━━━━━━━━━━━━━━━━
+ with-field
    - pair Pair
    - field String
    - value U64

match? field
    |= "left"
        pair.left = value
    |= "right"
        pair.right = value
```

```output
7 2
1 8
none
```

### Two tuples are equal where their fields are, an optional field of a type that declares `==` compared by it where both hold a value, with the right one first.

```yumi
. one = Record(Spot(1, 2), 3)
. two = Record(Spot(5, 6), 3)
. three = Record(null, 3)
. four = Record(null, 3)
-- one == two
-- one == three
-- three == four
-- three == one
. a Spot? = Spot(1, 2)
. b Spot? = Spot(5, 6)
-- a == b

━━━━━━━━━━━━━━━━━━━━
+ Spot is Tuple

+ x I32
+ y I32

────────────────────
+ this == other Spot

-- "called ${x} ${other.x}"
true

━━━━━━━━━━━━━━━━━━━━
+ Record is Tuple

+ pivot Spot?
+ size I32
```

```output
called 5 1
true
false
true
false
called 5 1
true
```

### What two optional fields hold is compared with the right one first, down to the `==` a field within declares.

```yumi
. one = Record(Wrap(Spot(1, 2)), 3)
. two = Record(Wrap(Spot(5, 6)), 3)
-- one == two

━━━━━━━━━━━━━━━━━━━━
+ Spot is Tuple

+ x I32
+ y I32

────────────────────
+ this == other Spot

-- "called ${x} ${other.x}"
true

━━━━━━━━━━━━━━━━━━━━
+ Wrap is Tuple

+ spot Spot

━━━━━━━━━━━━━━━━━━━━
+ Record is Tuple

+ inner Wrap?
+ size I32
```

```output
called 5 1
true
```

### A field whose type declares `==` only for a type it converts to is compared by that `==`, the other field converted to it, and an optional one so where both hold a value.

```yumi
. one = Record(Stamp(1), Stamp(4))
. two = Record(Stamp(1), Stamp(4))
. three = Record(Stamp(1), null)
-- one == two
-- one == three
-- [one, three] == [two, three]

━━━━━━━━━━━━━━━━━━━━
+ Stamp is Tuple

+ seconds Int

────────────────────
+ to Int

seconds

────────────────────
+ this == other Int

-- "compared $seconds $other"
seconds == other

━━━━━━━━━━━━━━━━━━━━
+ Record is Tuple

+ start Stamp
+ end Stamp?
```

```output
compared 1 1
compared 4 4
true
compared 1 1
false
compared 1 1
compared 4 4
compared 1 1
true
```

### A tuple that declares a field named `length` has that field assigned, as any other, and keeps all its fields.

```yumi
. span = Span(4, 0)
span = span.length = span.size + 1
-- "$span.size $span.length"

━━━━━━━━━━━━━━━━━━━━
+ Span is Tuple

+ size U32
+ length U32
```

```output
4 5
```

### A tuple pattern on an optional tuple matches its fields where there is one, and nothing where there is none.

```yumi
. found = search(1)
do match? found
    | name, size
        -- "$name $size"
. none = search(5)
do match? none
    | name, size
        -- "$name $size"
-- "done"

━━━━━━━━━━━━━━━━━━━━
_ do search (String, Int)?
    - least Int

for pair in pairs()
    if pair[1] > least
        = pair
= null

━━━━━━━━━━━━━━━━━━━━
_ pairs

* "x", 1
* "yy", 2
* "zzz", 3
```

```output
yy 2
done
```

### An assignment to a field of a tuple an index names evaluates the value before the tuple, and stores the new tuple it gives.

```yumi
. l [T]
l += T(1, 2)
l += T(3, 4)
. n = 0
l[1] = l[1].a = bump(l, n)
-- "l1 ${l[1].a} ${l[1].b}"
l[0] = l[0].b = bump(l, 5)
-- "l0 ${l[0].a} ${l[0].b}"
. h = Holder()
h.items[0] = h.items[0].a = h.grow()
-- "h ${h.items[0].a} ${h.items[0].b} ${h.items.length}"

━━━━━━━━━━━━━━━━━━━━
+ T is Tuple

+ a Int
+ b Int

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ items [T]

────────────────────
+ This

items += T(8, 9)

────────────────────
+ do grow Int

items[0] = T(100, 200)
= 3

━━━━━━━━━━━━━━━━━━━━
+ do bump Int
    - l [T]
    - n Int

l[1] = T(30, 40)
l[0] = T(10, 20)
= n + 7
```

```output
l1 7 40
l0 10 12
h 3 200 1
```

### An assignment to a field of a tuple evaluates the value, then the index of the tuple, then the index it is stored at, and gives a new tuple without changing the one it read.

```yumi
. l [T]
l += T(1, 2)
l += T(3, 4)
l[at("outer", 1)] = l[at("inner", 1)].a = val("rhs")
-- "l1 ${l[1].a} ${l[1].b}"
. t = T(5, 6)
. u = (t.b = val("tb"))
-- "u ${u.a} ${u.b} t ${t.a} ${t.b}"

━━━━━━━━━━━━━━━━━━━━
+ T is Tuple

+ a Int
+ b Int

━━━━━━━━━━━━━━━━━━━━
+ do at Index
    - tag String
    - i Index

-- tag
= i

━━━━━━━━━━━━━━━━━━━━
+ do val Int
    - tag String

-- tag
= 9
```

```output
rhs
inner
outer
l1 9 4
tb
u 5 9 t 5 6
```

### An assigning operator on a tuple's field gives a new tuple and leaves the tuple it read as it was.

```yumi
. p = Pt(5, 7)
. v = p.x -= 1
-- "local ${v.x} ${v.y} / ${p.x} ${p.y}"
-- shift(Pt(5, 7), Pt(1, 2))

━━━━━━━━━━━━━━━━━━━━
+ Pt is (x I32, y I32)

━━━━━━━━━━━━━━━━━━━━
_ do shift String
    - a Pt
    - c Pt

. v = a.x -= c.x
a = a.y -= c.y
= "param ${v.x} ${v.y} / ${a.x} ${a.y}"
```

```output
local 4 7 / 5 7
param 4 7 / 5 5
```

### A statement that applies an assigning operator to a tuple's field loses the new tuple.

```yumi crash
-- shift(Pt(5, 7), Pt(1, 2))

━━━━━━━━━━━━━━━━━━━━
+ Pt is (x I32, y I32)

━━━━━━━━━━━━━━━━━━━━
_ do shift String
    - a Pt
    - c Pt

a.x -= c.x
= "${a.x} ${a.y}"
```

```output
Exception: Assigning x creates a new Pt tuple that is discarded; write a = a.x = value
<no file:12>
    a.x -= c.x
    ^^^^^^^^^^
```

### An assigning operator that the field's own type answers in place, as an array's `+=` does, assigns no field of the tuple, so the statement loses nothing.

```yumi
. boxes = fill([1, 2])
-- "${boxes.tail.items.length} ${boxes.tail.count}"

━━━━━━━━━━━━━━━━━━━━
+ Box is Tuple

+ count Int
+ items [Int]

━━━━━━━━━━━━━━━━━━━━
_ do fill [Box]
    - numbers [Int]

. boxes = [Box(0)]
for number in numbers
    boxes.tail.items += number
    boxes.tail.items += number
= boxes
```

```output
4 0
```

### A tuple written in parentheses after a local's name is the local's type: the value is converted to it field by field, as an argument is.

```yumi
. t (Float, Int) = (1, 2)
. (x, y) = t
-- x / 2
-- t
. k (I8, Float) = (3, 4)
-- k
```

```output
0.5
(1, 2)
(3, 4)
```

### A local written with a named tuple type has the fields by name, and declares no local for them.

```yumi
. a (x Int, y Int) = (1, 2)
-- a.x
-- a.y
-- a
```

```output
1
2
(1, 2)
```

### A local written with a tuple type and no value holds the tuple with nothing written.

```yumi
. t (Float, Int)
-- t
```

```output
(0, 0)
```

### A field of a local written with a named tuple type is not a local.

```yumi crash
. a (x Int, y Int) = (1, 2)
-- x
```

```output
Exception: Name "x" not found
<no file:3>
    -- x
       ^
```

### A named tuple given as its base is a copy of the base's fields: cast, passed, or held by a local declared as the base.

```yumi
. c = Colored(1, 2, 3)
-- c as Point
-- (c as Point).length
-- both(c, 7)
. p Point = c
-- p

━━━━━━━━━━━━━━━━━━━━
+ Point is (x Int, y Int)

━━━━━━━━━━━━━━━━━━━━
+ Colored is Point

+ color Int

━━━━━━━━━━━━━━━━━━━━
+ both Int
    - first Point
    - second Int

first.x * 1000 + first.y * 10 + second
```

```output
(1, 2)
2
1027
(1, 2)
```

### A struct held by a local declared as its base is a copy of the base's fields.

```yumi
. c = Colored(1, 2, 3)
. p Point = c
-- p
-- both(c, 7)

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int

━━━━━━━━━━━━━━━━━━━━
+ Colored is Point

+ color Int

━━━━━━━━━━━━━━━━━━━━
+ both Int
    - first Point
    - second Int

first.x * 1000 + first.y * 10 + second
```

```output
(1, 2)
1027
```

### A tuple taken apart from an optional takes apart what it holds, and panics where it holds nothing.

```yumi crash
for seed in 0 .. < 3
    . lane, checksum = make(seed)
    -- "$lane $checksum"

━━━━━━━━━━━━━━━━━━━━
+ Pair is (lane Index, checksum Size)

━━━━━━━━━━━━━━━━━━━━
+ make Pair?
    - seed Int

Pair(seed + 1, seed + 2) if seed != 1
```

```stderr
Unhandled panic: Optional'Pair is null
<no file:3>

Task 1

Stack trace:
  0  __source__:main no file:3:7
  1  __source__:init.__source__ no file
  2  __source__:brix.main no file
```

### A field of an object a tuple written as `(…)` is not given holds the object its class constructs.

```yumi
. made Pair
-- made.count
-- made.thing.value
. rev = Rev(5)
-- rev.thing.value
. map [Int = Pair]
map[1] = Pair(Thing(), 2)
-= map[1]
-- map.length

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ value = 7

━━━━━━━━━━━━━━━━━━━━
+ Pair is (thing Thing, count Int)

━━━━━━━━━━━━━━━━━━━━
+ Rev is (count Int, thing Thing)
```

```output
0
7
7
0
```

```yumi
. holder = Holder()
holder.data.items += Item(7)
-- "${holder.data.items.length} ${holder.data.items[0].id} ${holder.data.count}"
. other = Holder()
-- other.data.items.length

━━━━━━━━━━━━━━━━━━━━
+ Item is Tuple

+ id U64

━━━━━━━━━━━━━━━━━━━━
+ Data is Tuple

+ items [Item]
+ count I64

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ data = Data()
```

```output
1 7 0
0
```

### A tuple written with no name becomes a tuple class unasked only where each field would convert unasked to the field at its place.

```yumi crash
-- make(true, 7).container

━━━━━━━━━━━━━━━━━━━━
+ Id is U32

━━━━━━━━━━━━━━━━━━━━
+ Partial is (container Id, flag Bool)

━━━━━━━━━━━━━━━━━━━━
+ make Partial
    - flag Bool
    - container Id

(flag, container)
```

```output
Exception: Expected Partial here, but this is (Bool, Id), which becomes Partial only when asked; write "as Partial"
<no file:15>
    (flag, container)
     ^^^^^^^^^^^^^^^
```

### A tuple written with integers with no suffix is given where a tuple class is expected before anything else has read the class, and each integer takes the type of its field.

```yumi
. holder = Holder()
holder.go()

━━━━━━━━━━━━━━━━━━━━
+ Holder

────────────────────
+ do go

move((3, 0))

────────────────────
+ do move
    - spot Spot

-- spot.x + spot.y

━━━━━━━━━━━━━━━━━━━━
+ Spot is Tuple

+ x I16
+ y I16
```

```output
3
```

### Tuples of numbers written with no suffix in a list join place by place, a float at a place where either holds one.

```yumi
. pairs = [(2.5, 1), (1, 2.5)]
. floats [(Float, Float)] = pairs
-- floats[0]
-- floats[1]
```

```output
(2.5, 1)
(1, 2.5)
```

### A tuple whose own constructors all take arguments starts at its fields' first values where a parameter's default holds it with no value written.

```yumi
-- shown()

━━━━━━━━━━━━━━━━━━━━
+ Spot is Tuple

+ x Float
+ y Float

────────────────────
+ This
    - parts [Float]

= This(parts[0], parts[1])

━━━━━━━━━━━━━━━━━━━━
+ Holder is Tuple

+ place Spot
+ count U64

━━━━━━━━━━━━━━━━━━━━
+ shown Float
    - holder Holder = Holder()

holder.place.x + holder.place.y
```

```output
0
```

### A returned tuple of unsuffixed numbers takes the named tuple type another return gives.

```yumi
. found = first-found(3)
-- found.ok
-- found.x
. missing = first-found(0)
-- missing.ok
-- missing.x

━━━━━━━━━━━━━━━━━━━━
_ do first-found
    - count I64

for index in 0 .. < count
    . item = item-at(index)
    if item.ok
        = item

= (false, 0)

━━━━━━━━━━━━━━━━━━━━
_ do item-at (ok Bool, x F64)
    - index I64

= (index > 1, 1.5)
```

```output
true
1.5
false
0
```

### A list of tuples larger than a pair store can reach builds, clearing each one whole.

```yumi
. items [Huge] = [Huge()]
-- items[0].e.d.d

━━━━━━━━━━━━━━━━━━━━
_ Quad is Tuple

+ a F64 = 1
+ b F64 = 2
+ c F64 = 3
+ d F64 = 4

━━━━━━━━━━━━━━━━━━━━
_ Wide is Tuple

+ a Quad = Quad()
+ b Quad = Quad()
+ c Quad = Quad()
+ d Quad = Quad()

━━━━━━━━━━━━━━━━━━━━
_ Huge is Tuple

+ a Wide = Wide()
+ b Wide = Wide()
+ c Wide = Wide()
+ d Wide = Wide()
+ e Wide = Wide()
```

```output
4
```

### `is` tests a tuple against the types written in parentheses, one for each field.

```yumi
. pair = (3, "three")
-- pair is (Int, String)
-- pair is (String, String)
-- pair is not (Int, String)
. maybe (Int, String)? = pair
-- maybe is (Int, String)
. none (Int, String)? = null
-- none is (Int, String)
```

```output
true
false
false
true
false
```

### Only types are written in parentheses after `is`; what the fields hold is tested by an arm of a match.

```yumi crash
. pair = (3, "three")
-- pair is (3, String)
```

```output
Exception: Only types may be written in parentheses after "is"; to test what the fields hold, match the value with an arm, as in "| (3, name)"
<no file:3>
    -- pair is (3, String)
                ^^^^^^^^^
```

### Names in brackets are declared from a list but not assigned again; names are assigned again from a tuple.

```yumi crash
. x = 0
. y = 0
[x, y] = [3, 4]
```

```output
Exception: Names in brackets can be declared from a list, as in ". [x, y] = items", but not assigned again; assign a tuple to them, as in "x, y = items[0], items[1]"
<no file:4>
    [x, y] = [3, 4]
    ^^^^^^
```
