# Structs

```yumi
. p = Point(1, 2)
-- p
p.x = 5
p[1] = 9
-- p
-- p.sum()
-- (type-of p).is-struct
-- (type-of p).is-tuple

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32

────────────────────
+ sum I32

x + y
```

```output
(1, 2)
(5, 9)
14
true
false
```

```yumi
. p = Point(3)
-- p

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32

────────────────────
+ This
    - n I32

x = n
y = n + 1
```

```output
(3, 4)
```

```yumi
. c = Colored(1, 2, 3)
. p = c as Point
p.x = 7
-- c
-- p

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32

━━━━━━━━━━━━━━━━━━━━
+ Colored is Point

+ color I32
```

```output
(1, 2, 3)
(7, 2)
```

```yumi
. line = Line(Point(1, 2), Point(3, 4))
line.start.x = 7
-- line

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32

━━━━━━━━━━━━━━━━━━━━
+ Line is Struct

+ start Point
+ end Point
```

```output
((7, 2), (3, 4))
```

```yumi
/BxCore/tests.h

. data = Small(81, 92)
C:ctestStructSmall1(data)

. ret = C:ctestStructReturnSmall1() as Small
-- ret
ret.a = 7
-- ret

━━━━━━━━━━━━━━━━━━━━
+ Small is Struct

+ a I32
+ b I32
```

```output
a 81 b 92
(1, 2)
(7, 2)
```

### Default, partial, and full construction

```yumi
. a = Thing()
. b = Thing(10)
. c = Thing(10, 20, 30)
-- a
-- b
-- c
-- a.length

━━━━━━━━━━━━━━━━━━━━
+ Thing is Struct

+ foo I32
+ bar I32 = 20
+ baz = foo + bar
```

```output
(0, 20, 20)
(10, 20, 30)
(10, 20, 30)
3
```

### Defaults can refer to base fields and fields declared later

```yumi
. child = Child()
. later = Later()
-- child
-- later

━━━━━━━━━━━━━━━━━━━━
+ Base is Struct

+ foo I32 = 1
+ bar I32 = 2

━━━━━━━━━━━━━━━━━━━━
+ Child is Base

+ baz I32 = bar + 1

━━━━━━━━━━━━━━━━━━━━
+ Later is Struct

+ zab I32 = bar
+ bar I32 = 20
```

```output
(1, 2, 3)
(20, 20)
```

### Circular default references are rejected

```yumi crash
. thing = Thing()
-- thing

━━━━━━━━━━━━━━━━━━━━
+ Thing is Struct

+ a I32 = b
+ b I32 = a
```

```output
Exception: "a" depends on its own value, directly or through the values it reads; break the cycle
<no file:8>
    + a I32 = b
    ^^^^^^^^^^^
```

### Constructor can return a struct value

```yumi
. thing = Thing(10.5)
-- thing

━━━━━━━━━━━━━━━━━━━━
+ Thing is Struct

+ foo I32
+ bar Float

────────────────────
+ This
    - n Float

= This(1, n)
```

```output
(1, 10.5)
```

### Assignment expressions mutate fields and return the assigned value

```yumi
. p = Point(1, 2)
. rx = p.x = 8
. ry = p[1] = 9
-- rx
-- ry
-- p

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
8
9
(8, 9)
```

### Struct assignment copies values

```yumi
. p = Point(1, 2)
. q = p
q.x = 9
-- p
-- q

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
(1, 2)
(9, 2)
```

### Struct parameters are passed by value

```yumi
. p = Point(1, 2)
mutate(p)
-- p

━━━━━━━━━━━━━━━━━━━━
+ do mutate
    - point Point

point.x = 9
-- point

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
(9, 2)
(1, 2)
```

### Nested struct projections are copies unless assigned through the original path

```yumi
. line = Line(Point(1, 2), Point(3, 4))
. start = line.start
start.x = 8
-- line
-- start

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32

━━━━━━━━━━━━━━━━━━━━
+ Line is Struct

+ start Point
+ end Point
```

```output
((1, 2), (3, 4))
(8, 2)
```

### Struct method receivers are passed by value

```yumi
. p = Point(1, 2)
p.move(5)
-- p

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32

────────────────────
+ do move
    - dx I32

x = x + dx
-- this
```

```output
(6, 2)
(1, 2)
```

### Struct index assignment cannot extend the layout

```yumi crash
. p = Point(1, 2)
p[2] = 3

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
Exception: Index out of bounds
<no file:3>
    p[2] = 3
    ^^^^
```

### Struct addition assignment cannot append to the layout

```yumi crash
. p = Point(1, 2)
p += 3

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
Exception: Function not found
<no file:3>
    p += 3
    ^^^^^^
```

### Struct length cannot be reassigned

```yumi crash
. p = Point(1, 2)
p.length = 1

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
Exception: Struct layout cannot be changed
<no file:3>
    p.length = 1
    ^^^^^^^^
```

### Struct indexes must be constant

```yumi crash
. p = Point(1, 2)
. i = 0
-- p[i]

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
Exception: Value has no constant form
<no file:4>
    -- p[i]
       ^^^^
```

### Struct inheritance and casts keep layout-compatible conversions

```yumi
. c = Colored(1, 2, 3)
. p = c as Point
. t = Point(4, 5) as (I32, I32)
. from-tuple = (6, 7) as Point
-- c
-- p
-- t
-- from-tuple
-- type-of from-tuple

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32

━━━━━━━━━━━━━━━━━━━━
+ Colored is Point

+ color I32
```

```output
(1, 2, 3)
(1, 2)
(4, 5)
(6, 7)
[TypeInfo Point]
```

### Same-layout named structs remain distinct in type patterns

```yumi
-- match A("hi", 1)
    | B
        "B"
    | A
        "A"

━━━━━━━━━━━━━━━━━━━━
+ A is Struct

+ a String
+ b I32

━━━━━━━━━━━━━━━━━━━━
+ B is Struct

+ a String
+ b I32
```

```output
A
```

### Inherited struct fields cannot be redefined

```yumi crash
. bar = Bar()
-- bar

━━━━━━━━━━━━━━━━━━━━
+ Foo is Struct

+ a I32

━━━━━━━━━━━━━━━━━━━━
+ Bar is Foo

+ a I32
```

```output
Exception: "a" is inherited from Foo, so it keeps the type it has there and cannot state its own; remove "I32"
<no file:13>
    + a I32
    ^^^^^^^
```

### Parameterized structs can be explicit or inferred

```yumi
. explicit = Box'I32(42, 1)
. inferred = Box("hi", 2)
-- explicit
-- inferred
-- type-of inferred

━━━━━━━━━━━━━━━━━━━━
+ Box'T is Struct

+ item T
+ count I32
```

```output
(42, 1)
(hi, 2)
[TypeInfo Box'String]
```

### Parameterized structs can inherit from parameterized structs

```yumi
. pair = Pair(1, 2, 3)
-- pair

━━━━━━━━━━━━━━━━━━━━
+ Base'T is Struct

+ a T
+ b I32

━━━━━━━━━━━━━━━━━━━━
+ Pair'T is Base'T

+ c T
```

```output
(1, 2, 3)
```

### Generic functions infer parameters from struct arguments

```yumi
-- values(Pair(1, 2))

━━━━━━━━━━━━━━━━━━━━
+ Pair'T1'T2 is Struct

+ a T1
+ b T2

━━━━━━━━━━━━━━━━━━━━
+ values'T1'T2
    - pair Pair'T1'T2

pair.a, pair.b
```

```output
(1, 2)
```

### Getter properties are not stored fields

```yumi
. thing = Thing(1, 2)
-- thing
-- thing.b
-- thing.length

━━━━━━━━━━━━━━━━━━━━
+ Thing is Struct

+ a I32
+ c I32

────────────────────
+ get b

42
```

```output
(1, 2)
42
2
```

### Operator overrides still dispatch before builtin struct behavior

```yumi
. thing = Thing(20)
-- thing += 3
-- thing + 4

━━━━━━━━━━━━━━━━━━━━
+ Thing is Struct

+ foo Int

────────────────────
+ this += other ''T

This(foo * other)

────────────────────
+ this + other ''T

This(foo + other)
```

```output
(60)
(64)
```

### Index overrides still dispatch before builtin struct behavior

```yumi
. thing = Thing(2, 3)
-- thing[10]
thing[10] = 7

━━━━━━━━━━━━━━━━━━━━
+ Thing is Struct

+ x Int
+ y Int

────────────────────
+ this[index] Int

x + y + index

────────────────────
+ do this[index] = value

-- "set $index $value"
```

```output
15
set 10 7
```

### Lazy properties are not supported on structs

```yumi crash
. thing = Thing()
-- thing

━━━━━━━━━━━━━━━━━━━━
+ Thing is Struct

+ value := 42
```

```output
Exception: Property not supported
<no file:8>
    + value := 42
    ^^^^^^^^^^^^^
```

### State machine properties are not supported on structs

```yumi crash
. thing = Thing()
-- thing

━━━━━━━━━━━━━━━━━━━━
+ Thing is Struct

────────────────────
+ flow ~>

~> idle
~> active
```

```output
Exception: State machine properties are not supported on structs
<no file:9>
    + flow ~>
    ^^^^^^^^^
```

### C struct returns are StructType values, not tuples

```yumi
/BxCore/tests.h

. ret = C:ctestStructReturnSmall1()
-- ret
-- ret.a
-- (type-of ret).is-struct
-- (type-of ret).is-tuple
ret.a = 9
-- ret
```

```output
(1, 2)
1
true
false
(9, 2)
```

### Named structs can be passed to C struct parameters

```yumi
/BxCore/tests.h

. small = Small(81, 92)
. small2 = Small2(81, 92, 33, 45)
. hfa = HFA(1.8, 9.2, 3.3)
C:ctestStructSmall1(small)
C:ctestStructSmall2(small2)
C:ctestStructHFA(hfa)

━━━━━━━━━━━━━━━━━━━━
+ Small is Struct

+ a I32
+ b I32

━━━━━━━━━━━━━━━━━━━━
+ Small2 is Struct

+ a I32
+ b I32
+ c I32
+ d I32

━━━━━━━━━━━━━━━━━━━━
+ HFA is Struct

+ a Float
+ b Float
+ c Float
```

```output
a 81 b 92
a 81 b 92 c 33 d 45
a 1.800000 b 9.200000 c 3.300000
```

### C struct parameters accept tuple values through final casts

```yumi
/BxCore/tests.h

. small = 81, 92
. large = 81, 92, 33, 45, 55
. nested = 81, 92, 33, 45, (88, 99)
C:ctestStructSmall1(small)
C:ctestStructLarge(large)
C:ctestStructLargeNested(nested)
```

```output
a 81 b 92
a 81 b 92 c 33 d 45 e 55
a 81 b 92 c 33 d 45
e.a 88 e.b 99
```

### C struct return values can be cast to named structs and mutated

```yumi
/BxCore/tests.h

. hfa = C:ctestStructReturnHFA() as HFA
. small2 = C:ctestStructReturnSmall2() as Small2
. large = C:ctestStructReturnLarge() as Large
-- hfa
-- small2
-- large
large.e = 99
-- large

━━━━━━━━━━━━━━━━━━━━
+ HFA is Struct

+ a Float
+ b Float
+ c Float

━━━━━━━━━━━━━━━━━━━━
+ Small2 is Struct

+ a I32
+ b I32
+ c I32
+ d I32

━━━━━━━━━━━━━━━━━━━━
+ Large is Struct

+ a I32
+ b I32
+ c I32
+ d I32
+ e I32
```

```output
(1.2, 2.3, 3.4)
(1, 2, 3, 4)
(1, 2, 3, 4, 5)
(1, 2, 3, 4, 99)
```

### A struct's field is assigned in place: by name, by index, through a path and with an operator. A copy is not changed.

```yumi
. p = Point(1, 2)
. q = p
p.x = 5
p[1] = 6
p.x += 10
p.y *= 2
-- p
-- q
. line = Line(p, q)
line.start.y = 7
line.end[0] -= 1
-- line
-- p

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32

━━━━━━━━━━━━━━━━━━━━
+ Line is Struct

+ start Point
+ end Point
```

```output
(15, 12)
(1, 2)
((15, 7), (0, 2))
(15, 12)
```

### A struct is passed by value: a function that assigns to its argument, or a member to its receiver, changes its own copy.

```yumi
. p = Point(1, 2)
-- changed(p)
-- p
. q = p.moved(5)
-- p
-- q
p = p.moved(1).moved(1)
-- p
-- p.sum
-- grown(Box(1, 2, 3, 4))
-- scaled(Vec(1.5, 2.5))

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32

────────────────────
+ do moved Point
    - dx I32

x = x + dx
= this

────────────────────
+ get sum

x + y

━━━━━━━━━━━━━━━━━━━━
+ do changed I32
    - v Point

v.x = 50
= v.x

━━━━━━━━━━━━━━━━━━━━
+ Box is Struct

+ a I64
+ b I64
+ c I64
+ d I64

━━━━━━━━━━━━━━━━━━━━
+ do grown Box
    - v Box

v.d = v.d + 100
= v

━━━━━━━━━━━━━━━━━━━━
+ Vec is Struct

+ x F64
+ y F64

━━━━━━━━━━━━━━━━━━━━
+ scaled Vec
    - v Vec

Vec(v.x * 2.0, v.y * 2.0)
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

```yumi
-- Point()
-- Point(7)
-- Point() • y 9
-- Line()
-- Line(Point(5, 6))
-- Colored(1, 2, "red")
-- Colored(1)
. c = Colored(3, 4, "blue")
. p = c as Point
p.x = 8
-- p
-- c

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32 = 1
+ y I32 = 2

━━━━━━━━━━━━━━━━━━━━
+ Line is Struct

+ start Point
+ end Point

━━━━━━━━━━━━━━━━━━━━
+ Colored is Point

+ color = "none"
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

```yumi
. total = 0
. i = 0
while i < 20000
    . pair = (name(i), name(i + 1))
    . kept = keep(pair)
    . junk = name(i + 2)
    if kept[0] == pair[0] && kept[1] == pair[1]
        total += 1
    . holder = Holder(junk, i, name(i + 3))
    if holder.first == junk && holder.last != junk
        total += 1
    i += 1
-- total

━━━━━━━━━━━━━━━━━━━━
+ Holder is Struct

+ first String
+ count Int
+ last String

━━━━━━━━━━━━━━━━━━━━
+ name String
    - n Int

"item-$n"

━━━━━━━━━━━━━━━━━━━━
+ keep (String, String)
    - pair (String, String)

pair[0], pair[1]
```

```output
40000
```

### A struct given where its base is wanted is a copy of the fields the base has, in registers and in memory.

```yumi
. c = Colored(1, 2, 3)
-- both(c as Point, 7)
-- both(c, 7)
-- c.first(7)
-- (c as Point).first(7)
-- pair(c, c)
. w = Wide(1, 2, 3)
-- wide(w as Base, 7)
-- wide(w, 7)
. v = V3(1.5, 2.5, 3.5)
-- flat(v as V2, 7.0)
-- flat(v, 7.0)

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32

────────────────────
+ first Int
    - second Int

x * 1000 + second

━━━━━━━━━━━━━━━━━━━━
+ Colored is Point

+ color I32

━━━━━━━━━━━━━━━━━━━━
+ Base is Struct

+ x I64
+ y I64

━━━━━━━━━━━━━━━━━━━━
+ Wide is Base

+ z I64

━━━━━━━━━━━━━━━━━━━━
+ V2 is Struct

+ x F64
+ y F64

━━━━━━━━━━━━━━━━━━━━
+ V3 is V2

+ z F64

━━━━━━━━━━━━━━━━━━━━
+ both Int
    - first Point
    - second Int

first.x * 1000 + second

━━━━━━━━━━━━━━━━━━━━
+ pair Int
    - first Point
    - second Point

first.x * 100 + second.x * 10 + second.y

━━━━━━━━━━━━━━━━━━━━
+ wide I64
    - first Base
    - second I64

first.x * 1000 + first.y * 100 + second

━━━━━━━━━━━━━━━━━━━━
+ flat F64
    - v V2
    - k F64

v.x * 1000.0 + v.y * 100.0 + k
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

```yumi compile-error
. c = Colored(1, 2, 3)
(c as Point).x = 9

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32

━━━━━━━━━━━━━━━━━━━━
+ Colored is Point

+ color I32
```

```output
Exception: Assigning x changes a Point struct that nothing holds, so the change is lost; assign x where the struct is held
<no file:3>
    (c as Point).x = 9
    ^^^^^^^^^^^^^^^^^^
```

### An operand that names a struct is copied before a later operand changes it; the receiver of a call is read after its arguments.

```yumi
/string.h
. p = Point(1, 2)
-- p + change(&p)
-- p
p = Point(1, 2)
-- p == changed(&p)
p = Point(1, 2)
-- "$p" ++ text(&p)
p = Point(1, 2)
-- both(p, change(&p))
p = Point(1, 2)
-- (p, change(&p))
p = Point(1, 2)
-- p.first(change(&p))

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32

────────────────────
+ first Int
    - second Int

x * 1000 + second

────────────────────
+ this + n Int

x * 1000 + n

────────────────────
+ this == other

x == other.x

━━━━━━━━━━━━━━━━━━━━
+ both Int
    - first Point
    - second Int

first.x * 1000 + second

━━━━━━━━━━━━━━━━━━━━
+ do change Int
    - target CPointer'Point

. z = Point(42, 43)
C:memcpy(target, &z, 8)
= 7

━━━━━━━━━━━━━━━━━━━━
+ do changed Point
    - target CPointer'Point

. z = Point(42, 43)
C:memcpy(target, &z, 8)
= Point(1, 2)

━━━━━━━━━━━━━━━━━━━━
+ do text String
    - target CPointer'Point

. z = Point(42, 43)
C:memcpy(target, &z, 8)
= "!"
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

```yumi
. p = Point(3)
-- p.x
-- p.y
-- p.sum()

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int

────────────────────
+ This
    - n Int

x = n
y = x * 2

────────────────────
+ sum Int

x + y
```

```output
3
6
9
```

### A constructor written in source starts from the values the struct gives its fields, and may return early.

```yumi
. a = Size(2)
-- a.w
-- a.h
-- a.label
. b = Size(0)
-- b.w
-- b.h

━━━━━━━━━━━━━━━━━━━━
+ Size is Struct

+ w = 7
+ h = 9
+ label = "size"

────────────────────
+ This
    - scale Int

if scale == 0
    =
w = w * scale
h *= scale
```

```output
14
18
size
7
9
```

### A struct too large for registers made by a constructor written in source.

```yumi
. r = Rect(1, 2)
-- r.a
-- r.b
-- r.c
-- r.d
-- total(r)

━━━━━━━━━━━━━━━━━━━━
+ total Int
    - r Rect

r.a + r.b + r.c + r.d

━━━━━━━━━━━━━━━━━━━━
+ Rect is Struct

+ a I64
+ b I64
+ c I64
+ d I64

────────────────────
+ This
    - x I64
    - y I64

a = x
b = y
c = x + y
d = c * 2
```

```output
1
2
3
6
12
```

### A constructor written in source assigns fields that hold text.

```yumi
. n = Named("ab")
-- n.name
-- n.twice

━━━━━━━━━━━━━━━━━━━━
+ Named is Struct

+ name String
+ twice String

────────────────────
+ This
    - text String

name = text
twice = text ++ text
```

```output
ab
abab
```

### A struct's constructor with defaults for its parameters.

```yumi
-- Pt().x
-- Pt(4).x
-- Pt(4, 5).y

━━━━━━━━━━━━━━━━━━━━
+ Pt is Struct

+ x Int
+ y Int

────────────────────
+ This
    - a Int = 1
    - b Int = 2

x = a
y = b
```

```output
1
4
5
```

### A struct made by a constructor written in source is held in a property.

```yumi
. h = Holder()
-- h.p.x
h.p = Point(9)
-- h.p.y

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ p = Point(3)

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int

────────────────────
+ This
    - n Int

x = n
y = n + 1
```

```output
3
10
```

### What the function of an assigning operator answers for a struct is stored where the struct is held: in a local, in a struct's field, and in a property of an object.

```yumi
. thing = Thing(20)
thing += 2
-- thing.foo
-- (thing += 3).foo
-- thing.foo
. pair = Both(Thing(1), Thing(2))
pair.second += 5
-- pair.first.foo
-- pair.second.foo
. box = Box()
box.held += 2
-- box.held.foo
-- box.held += 3
-- box.held.foo
box.grow()
-- box.held.foo

━━━━━━━━━━━━━━━━━━━━
+ Thing is Struct

+ foo I32

────────────────────
+ this += other I32

This(foo * other)

━━━━━━━━━━━━━━━━━━━━
+ Both is Struct

+ first Thing
+ second Thing

━━━━━━━━━━━━━━━━━━━━
+ Box

+ held = Thing(5)

────────────────────
+ do grow

held += 10
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

```yumi crash
. thing = Thing(20)
thing += 3
-- thing.foo

━━━━━━━━━━━━━━━━━━━━
+ Thing is Struct

+ foo I32

────────────────────
+ do this += other I32

foo += other
```

```output
Exception: Expected Thing here, but this is Void, and there is no conversion between them
<no file:3>
    thing += 3
    ^^^^^^^^^^
```

### The function of an assigning operator answers for a struct a value the struct can be assigned.

```yumi crash
. thing = Thing(20)
-- (thing += 3)
-- thing.foo

━━━━━━━━━━━━━━━━━━━━
+ Thing is Struct

+ foo I32

────────────────────
+ this += other I32

foo * other
```

```output
Exception: Expected Thing here, but this is I32, and there is no conversion between them
<no file:3>
    -- (thing += 3)
        ^^^^^^^^^^
```

### An assigning operator whose answer is stored gives the answer it stored as its value.

```yumi
. thing = Thing(5)
-- (thing += 3).foo
-- thing.foo
. box = Box()
-- (box.held += 2).foo
-- box.held.foo

━━━━━━━━━━━━━━━━━━━━
+ Thing is Struct

+ foo I32

────────────────────
+ this += other I32

This(foo * other)

━━━━━━━━━━━━━━━━━━━━
+ Box

+ held = Thing(5)
```

```output
15
15
10
10
```

### An assigning operator reads its place after the value it is given is evaluated.

```yumi
+ thing = Thing(2)

thing += change()
-- thing.foo
. local = Thing(2)
local += say(local.foo + 1)
-- local.foo

━━━━━━━━━━━━━━━━━━━━
+ Thing is Struct

+ foo Int

────────────────────
+ this += other Int

This(foo * other)

━━━━━━━━━━━━━━━━━━━━
+ do change Int

thing = Thing(10)
= 3

━━━━━━━━━━━━━━━━━━━━
+ do say Int
    - v Int

-- "say $v"
= v
```

```output
30
say 3
6
```

### A struct holds no content: a section with markup written in it is rejected.

```yumi crash
. thing = Thing(3)
-- thing.twice

━━━━━━━━━━━━━━━━━━━━
+ Thing is Struct

+ foo I32

────────────────────

<This

────────────────────
+ get twice

foo * 2
```

```output
Exception: Content is not supported on structs
<no file:6>
    + Thing is Struct
    ^^^^^^^^^^^^^^^^
```

### Constructors that return a value of their own from the arms of a match

```yumi
-- Thing(10.5)
-- Thing(-2.0)
-- Box(1).size
-- Box(5).size

━━━━━━━━━━━━━━━━━━━━
+ Thing is Struct

+ foo I32
+ bar Float

────────────────────
+ do This
    - n Float

match n
    | {it < 0.0}
        = This(0, 0.0)
    ??
        = This(1, n)

━━━━━━━━━━━━━━━━━━━━
+ Box

+ size = 0

────────────────────
+ do This
    - kind Int

match kind
    | 1
        = small()
    ??
        = large(kind)

━━━━━━━━━━━━━━━━━━━━
+ SmallBox is Box

+ size = 1

━━━━━━━━━━━━━━━━━━━━
+ do small Box

= SmallBox()

━━━━━━━━━━━━━━━━━━━━
+ do large Box
    - count Int

. box = SmallBox()
box.size = count * 10
= box
```

```output
(1, 10.5)
(0, 0)
1
50
```
