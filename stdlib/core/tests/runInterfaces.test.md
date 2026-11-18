# Interfaces

```yumi
. thing = Thing()
foo(thing)

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ bar Int

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Proto

────────────────────
+ bar

42

━━━━━━━━━━━━━━━━━━━━
+ do foo
    - p Proto

-- p.bar()
```

```output
42
```

```yumi
. thing = Thing()
foo(thing)

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ get bar Int

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Proto

────────────────────
+ get bar

42

━━━━━━━━━━━━━━━━━━━━
+ do foo
    - p Proto

-- p.bar
```

```output
42
```

```yumi crash
-- Proto()

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ bar Int
```

```output
Exception: Proto is an interface, so it has no constructor; construct a class that is Proto
<no file:2>
    -- Proto()
       ^^^^^
```

```yumi
-- Proto()

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ This

= 42
```

```output
42
```

### Default function implementation

```yumi
. thing = Thing()
foo(thing)

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ do bar

-- this
foo()
= 42

────────────────────
+ foo

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Proto

────────────────────
+ do foo

-- "yes"

━━━━━━━━━━━━━━━━━━━━
+ do foo
    - p Proto

-- p.bar()
```

```output
[Interface __source__:Proto]
yes
42
```

```yumi
. thing = Thing()
thing.foo()
thing.bar()

. proto = thing as Proto
proto.foo()
proto.bar()

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ do foo

-- "proto foo $this"

────────────────────
+ do bar

-- "proto bar $this"

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Proto

────────────────────
+ do bar

-- "thing bar $this"
```

```output
proto foo [Interface __source__:Proto]
thing bar [Object __source__:Thing]
proto foo [Interface __source__:Proto]
thing bar [Object __source__:Thing]
```

```yumi
. thing = Thing'Float'Int()
foo(thing)
bar(thing)

━━━━━━━━━━━━━━━━━━━━
+ Foo'T

━━━━━━━━━━━━━━━━━━━━
+ Bar'T is Interface

━━━━━━━━━━━━━━━━━━━━
+ Thing'T1'T2 is Foo'T2
    is Bar'T1

+ a T1 = 10
+ b T2 = 10

━━━━━━━━━━━━━━━━━━━━
+ do foo'T
    - p Foo'T

-- T()

━━━━━━━━━━━━━━━━━━━━
+ do bar'T
    - p Bar'T

-- T()
```

```output
0
0
```

```yumi
. genny = gen()
foo(genny)

━━━━━━━━━━━━━━━━━━━━
+ do gen

<= 1
<= 2

━━━━━━━━━━━━━━━━━━━━
+ do foo'T
    - g <T>

-- T()
```

```output
0
```

```yumi
. thing = Thing()
foo(thing)

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ do bar

-- this

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Proto

────────────────────
+ do bar

-- this

━━━━━━━━━━━━━━━━━━━━
+ do foo
    - proto Proto

proto.bar()
```

```output
[Object __source__:Thing]
```

```yumi
. thing = Thing'Int()
. stream = thing as <Int>
foo(thing)

━━━━━━━━━━━━━━━━━━━━
+ Thing'T

────────────────────
+ do to Iterable'T

<= 1
<= 2

━━━━━━━━━━━━━━━━━━━━
+ do foo'T
    - stream <T>

for n in stream
    -- n
```

```output
1
2
```

## Different types implementing interfaces

```yumi
. thing = Thing()
. proto = thing as Proto
-- type-of proto
proto.foo()
proto.foo(1)
proto.foo(1.5)

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ foo

────────────────────
+ foo
    - n

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Proto

────────────────────
+ do foo

-- this

────────────────────
+ do foo
    - n

-- n
```

```output
[TypeInfo Proto]
[Object __source__:Thing]
1
1.5
```

```yumi
. thing = Thing(42)
. proto = thing as Proto
-- type-of proto
proto.foo()
proto.foo(1)
proto.foo(1.5)

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ foo

────────────────────
+ foo
    - n

━━━━━━━━━━━━━━━━━━━━
+ Thing is I32
    is Proto

────────────────────
+ do foo

-- this

────────────────────
+ do foo
    - n

-- n
```

```output
[TypeInfo Proto]
42
1
1.5
```

```yumi
-- get-proto(gen())

━━━━━━━━━━━━━━━━━━━━
+ do gen

<= 1

━━━━━━━━━━━━━━━━━━━━
+ get-proto <Int>
    - thing

thing
```

```output
[Interface core/stream:Iterable'core/number:I64]
```

```yumi
. thing = Thing(3.5)
. proto = thing as Proto
-- type-of proto
proto.foo()
proto.foo(1)
proto.foo(1.5)

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ foo

────────────────────
+ foo
    - n

━━━━━━━━━━━━━━━━━━━━
+ Thing is F64
    is Proto

────────────────────
+ do foo

-- this

────────────────────
+ do foo
    - n

-- n
```

```output
[TypeInfo Proto]
3.5
1
1.5
```

```yumi
. fn = :: a => a * 3

. thing = fn as Thing
. proto = thing as Proto
-- type-of proto
proto.foo()
proto.foo(1)
proto.foo(1.5)

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ foo

────────────────────
+ foo
    - n

━━━━━━━━━━━━━━━━━━━━
+ ThingFn is () I32
    - a Int

━━━━━━━━━━━━━━━━━━━━
+ Thing is ThingFn
    is Proto

────────────────────
+ do foo

-- this
-- this(3)

────────────────────
+ do foo
    - n

-- n
```

```output
[TypeInfo Proto]
[Function Thing]
9
1
1.5
```

```yumi
. thing = Thing(1, 2)
. proto = thing as Proto
-- type-of proto
proto.foo()
proto.foo(1)
proto.foo(1.5)

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ foo

────────────────────
+ foo
    - n

━━━━━━━━━━━━━━━━━━━━
+ Thing is (a Int, b Float)
    is Proto

────────────────────
+ do foo

-- this

────────────────────
+ do foo
    - n
-- n
```

```output
[TypeInfo Proto]
(1, 2)
1
1.5
```

## Interface inheritance

```yumi
. p1 = Thing() as Proto1
p1.foo()

. p2 = Thing() as Proto2
p2.foo()

━━━━━━━━━━━━━━━━━━━━
+ Proto1 is Interface

────────────────────
+ foo

━━━━━━━━━━━━━━━━━━━━
+ Proto2 is Interface
    is Proto1

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Proto2

────────────────────
+ do foo

-- 42
```

```output
42
42
```

### Cast base interface to sub interface, call through sub and then through base

```yumi
. foobar = Thing() as Foobar
foobar.foo()
foobar.bar()

. bar = foobar as Bar
bar.bar()

. foo = foobar as Foo
foo.foo()

━━━━━━━━━━━━━━━━━━━━
+ Foo is Interface

────────────────────
+ foo

━━━━━━━━━━━━━━━━━━━━
+ Bar is Interface

────────────────────
+ bar

━━━━━━━━━━━━━━━━━━━━
+ Foobar is Interface
    is Foo
    is Bar

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Foobar

────────────────────
+ do foo

-- "foo"

────────────────────
+ do bar

-- "bar"
```

```output
foo
bar
bar
foo
```

### Cast base interface to sub interface, call through base and then through sub

```yumi
. foobar = Thing() as Foobar
. bar = foobar as Bar
bar.bar()

. foo = foobar as Foo
foo.foo()

foobar.foo()
foobar.bar()

━━━━━━━━━━━━━━━━━━━━
+ Foo is Interface

────────────────────
+ foo

━━━━━━━━━━━━━━━━━━━━
+ Bar is Interface

────────────────────
+ bar

━━━━━━━━━━━━━━━━━━━━
+ Foobar is Interface
    is Foo
    is Bar

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Foobar

────────────────────
+ do foo

-- "foo"

────────────────────
+ do bar

-- "bar"
```

```output
bar
foo
foo
bar
```

### Cast to sub interface, call through sub

```yumi
. thing = Thing()
. bar = thing as Bar
bar.bar()

. foo = thing as Foo
foo.foo()

━━━━━━━━━━━━━━━━━━━━
+ Foo is Interface

────────────────────
+ foo

━━━━━━━━━━━━━━━━━━━━
+ Bar is Interface

────────────────────
+ bar

━━━━━━━━━━━━━━━━━━━━
+ Foobar is Interface
    is Foo
    is Bar

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Foobar

────────────────────
+ do foo

-- "foo"

────────────────────
+ do bar

-- "bar"
```

```output
bar
foo
```

```yumi
. thing = Thing()

. foobar = thing as Foobar
foobar.a()

. a = thing as A
a.a()

. foo = foobar as Foo
foo.a()

a = foobar as A
a.a()

a = foo as A
a.a()

━━━━━━━━━━━━━━━━━━━━
+ A is Interface

────────────────────
+ a

━━━━━━━━━━━━━━━━━━━━
+ Foo is Interface
    is A

────────────────────
+ foo

━━━━━━━━━━━━━━━━━━━━
+ Bar is Interface

────────────────────
+ bar

━━━━━━━━━━━━━━━━━━━━
+ Foobar is Interface
    is Foo
    is Bar

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Foobar

────────────────────
+ do a

-- "a"

────────────────────
+ do foo

-- "foo"

────────────────────
+ do bar

-- "bar"
```

```output
a
a
a
a
a
```

### Illegal to cast interface to another interface that inherits from it

```yumi crash
. p1 = Thing() as Proto1
-- p1 as Proto2

━━━━━━━━━━━━━━━━━━━━
+ Proto1 is Interface

────────────────────
+ foo

━━━━━━━━━━━━━━━━━━━━
+ Proto2 is Interface
    is Proto1

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Proto2

────────────────────
+ do foo

-- 42
```

```output
Exception: Proto1 cannot be cast to Proto2; there is no conversion between them
<no file:3>
    -- p1 as Proto2
       ^^^^^^^^^^^^
```

## Class inheritance with interfaces

```yumi
. bar = Bar()
. foo = bar as Foo
. p = foo as Proto
p.foo()

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ foo

━━━━━━━━━━━━━━━━━━━━
+ Foo
    is Proto

────────────────────
+ do foo

-- "Foo"

━━━━━━━━━━━━━━━━━━━━
+ Bar is Foo

────────────────────
+ do foo

-- "Bar"
```

```output
Bar
```

```yumi
. direct Base = Middle()
. inherited Base = Leaf()
. plain Base = Base()
print("direct marker ${direct is Marker} named ${direct is Named}")
print("inherited marker ${inherited is Marker} named ${inherited is Named}")
print("plain marker ${plain is Marker} named ${plain is Named}")

━━━━━━━━━━━━━━━━━━━━
+ Marker is Interface

━━━━━━━━━━━━━━━━━━━━
+ Named is Interface

────────────────────
+ get label String

━━━━━━━━━━━━━━━━━━━━
+ Base

+ size = 1

━━━━━━━━━━━━━━━━━━━━
+ Middle is Base
    is Marker
    is Named

────────────────────
+ get label String

"middle"

━━━━━━━━━━━━━━━━━━━━
+ Leaf is Middle
```

```output
direct marker true named true
inherited marker true named true
plain marker false named false
```

```yumi
. inherited Base = Leaf()
. named = inherited as Named
print(named.describe())

━━━━━━━━━━━━━━━━━━━━
+ Named is Interface

────────────────────
+ describe String

━━━━━━━━━━━━━━━━━━━━
+ Base

+ size = 1

━━━━━━━━━━━━━━━━━━━━
+ Middle is Base
    is Named

────────────────────
+ describe String

"middle"

━━━━━━━━━━━━━━━━━━━━
+ Leaf is Middle

────────────────────
+ describe String

"leaf"
```

```output
leaf
```

## Converters

```yumi
. thing = Thing()
. proto = thing as Proto
-- proto as Float
-- thing as Float

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ to Float

10.5

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Proto
```

```output
10.5
10.5
```

```yumi
. thing = Thing() as Proto
-- thing

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ foo I32

────────────────────
+ to String

foo() as String

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Proto

────────────────────
+ foo

42
```

```output
42
```

```yumi
. thing = Thing() as Proto
-- " ".join(thing)

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ foo String

────────────────────
+ do to Iterable'String

<= "hey"
<= foo()

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Proto

────────────────────
+ foo

"you"
```

```output
hey you
```

### Make sure base class interfaces are checked when trying to convert to iterator

```yumi
. thing = Foo()
for i in thing
    -- i

━━━━━━━━━━━━━━━━━━━━
+ Bar
    is Iterable'I32

+ i I32 = 0

────────────────────
+ do <- this

if i < 2
    i += 1
    = i

━━━━━━━━━━━━━━━━━━━━
+ Foo is Bar
```

```output
1
2
```

### Make sure base class converters are checked when trying to convert to iterator

```yumi
. thing = Foo()
for i in thing
    -- i

━━━━━━━━━━━━━━━━━━━━
+ Bar

────────────────────
+ do to Iterable'I32

<= 42

━━━━━━━━━━━━━━━━━━━━
+ Foo is Bar
```

```output
42
```

```yumi
. p = Thing() as Proto
p.act()

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ do foo

-- "Foo"

━━━━━━━━━━━━━━━━━━━━
+ Bar is Foo

────────────────────
+ do foo

-- "Bar"

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ act

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Proto

────────────────────
+ do act

. bar = Bar()
bar.foo()
```

```output
Bar
```

## Delegates

### Delegate as eager property

```yumi
. thing = Thing()
thing.test()

. proto = thing as Proto
proto.act()

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ act

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ do act

-- "called act"

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Proto for
+ foo Foo

────────────────────
+ do test

foo.act()
```

```output
called act
called act
```

### Call delegate without object

```yumi
. thing = Thing()
thing.test()

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ act

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ do act

-- "called act"

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Proto for
+ foo Foo

────────────────────
+ do test

act()
```

```output
called act
```

### Delegate as lazy property

```yumi
. thing = Thing()
thing.act()

. proto = thing as Proto
proto.act()

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ act

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ do act

-- "called act"

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Proto for
+ foo := Foo()
```

```output
called act
called act
```

### Delegate as getter

```yumi
. thing = Thing()
thing.act()

. proto = thing as Proto
proto.act()

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ act

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ do act

-- "called act"

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Proto for
────────────────────
+ get foo

Foo()
```

```output
called act
called act
```

### Delegate of delegate

```yumi
. thing = Thing()
thing.act()

. proto = thing as Proto
proto.act()

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ act

━━━━━━━━━━━━━━━━━━━━
+ Foo
    is Proto

────────────────────
+ do act

-- "called act"

━━━━━━━━━━━━━━━━━━━━
+ Bar

as Proto for
+ foo = Foo()

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Proto for
+ bar = Bar()
```

```output
called act
called act
```

### Delegate of delegate in base class

```yumi
. thing = Thing()
thing.act()

. proto = thing as Proto
proto.act()

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ act

━━━━━━━━━━━━━━━━━━━━
+ Foo
    is Proto

────────────────────
+ do act

-- "called act"

━━━━━━━━━━━━━━━━━━━━
+ Bar

as Proto for
+ foo = Foo()

━━━━━━━━━━━━━━━━━━━━
+ Base

as Proto for
+ bar = Bar()

━━━━━━━━━━━━━━━━━━━━
+ Thing is Base
```

```output
called act
called act
```

### Delegate in tuple

```yumi
. thing = Thing()
thing.act()

. proto = thing as Proto
proto.act()

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ act

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ a = 0

────────────────────
+ This
    - a

────────────────────
+ do act

-- "called act $a"

━━━━━━━━━━━━━━━━━━━━
+ Thing is Tuple

as Proto for
+ foo = Foo(2)
```

```output
called act 2
called act 2
```

### Delegate in binding

```yumi
. thing = Thing()
. binding = thing.act(...)
binding(42)

. proto = thing as Proto
. binding2 = proto.act(...)
binding2(24)

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ act
    - x Int

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ do act
    - x Int

-- "called act $x"

━━━━━━━━━━━━━━━━━━━━
+ Thing is Tuple

as Proto for
+ foo Foo
```

```output
called act 42
called act 24
```

```yumi crash
. p Proto

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface
```

```output
Exception: "p" is declared Proto with no value, but Proto is an interface, so there is nothing to construct; give it a value, as in ". p Proto = value", or declare it "Proto?" to start as null
<no file:2>
    . p Proto
        ^^^^^
```

```yumi
. sink = Sink()
sink <- "hello"
. writable = sink as Writable'String
writable <- "world"

━━━━━━━━━━━━━━━━━━━━
+ Writable'Message is Interface

────────────────────
+ this <- item Message

━━━━━━━━━━━━━━━━━━━━
+ Wrapped is (text String)

────────────────────
+ from other String

Wrapped(other)

────────────────────
+ to String

text

━━━━━━━━━━━━━━━━━━━━
+ Channel'Message
    is Writable'Message

────────────────────
+ do this <- item Message

-- item

━━━━━━━━━━━━━━━━━━━━
+ Sink

as Writable'String for
_ channel = Channel'Wrapped()
```

```output
hello
world
```

```yumi
. thing = Thing()
thing.act()

. proto = thing as Proto
proto.act()

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ ping

────────────────────
+ do act

ping()

━━━━━━━━━━━━━━━━━━━━
+ Foo
    is Proto

────────────────────
+ do ping

-- "foo ping"

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Proto for
_ foo = Foo()
```

```output
foo ping
foo ping
```

```yumi
. thing = Thing()

. child = thing as Child
child.base()
child.child()

. base = thing as Base
base.base()

━━━━━━━━━━━━━━━━━━━━
+ Base is Interface

────────────────────
+ base

━━━━━━━━━━━━━━━━━━━━
+ Child is Interface
    is Base

────────────────────
+ child

━━━━━━━━━━━━━━━━━━━━
+ Foo
    is Child

────────────────────
+ do base

-- "base"

────────────────────
+ do child

-- "child"

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Child for
_ foo = Foo()
```

```output
base
child
base
```

```yumi crash
-- Proto()

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ This

-- this
42
```

```output
Exception: Required self not defined
<no file:10>
    -- this
       ^^^^
```

```yumi crash
-- Proto()

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ This
```

```output
Exception: Type can not be constructed
<no file:8>
    + This
    ^^^^^
```

```yumi
. thing = Thing()
. thing1 = thing as Proto
. thing1b = thing as Proto
. thing2 = Thing() as Proto
. ding1 = Ding() as Proto
-- thing1 == thing1
-- thing1 == thing1b
-- thing1 == thing2
-- thing1 != thing1b
-- thing1 != thing2

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Proto

━━━━━━━━━━━━━━━━━━━━
+ Ding
    is Proto
```

```output
true
true
false
false
true
```

```yumi
. thing = Thing("bob", "roberts")
. thing1 = thing as Proto
. thing1b = thing as Proto
. thing2 = Thing("joe", "blow") as Proto
. thing3 = Thing("bob", "roberts") as Proto
-- thing1 == thing1
-- thing1 == thing1b
-- thing1 == thing2
-- thing1 == thing3

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

━━━━━━━━━━━━━━━━━━━━
+ Thing is (first String, last String)
    is Proto
```

```output
true
true
false
true
```

```yumi
. thing = Thing()
thing.action()

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Proto

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ do action

-- "hello $this"
```

```output
hello [Interface __source__:Proto]
```

### Delegate through stream shorthand resolves to the property interface

```yumi
. thing = Thing()
. stream = thing as <String>
-- type-of stream

━━━━━━━━━━━━━━━━━━━━
+ Producer
    is Iterable'String

────────────────────
+ <- this String?

null

━━━━━━━━━━━━━━━━━━━━
+ Thing

as <String> for
_ producer = Producer()
```

```output
[TypeInfo Iterable'String]
```

### Self-referential generic interface conformance (Node as TreeHost'Node) compiles without recursing infinitely during type creation, and constrained generic entry points dispatch the host's concretely typed hook statically.

```yumi
. parent = Node()
. child = Node()
tree-append(parent, child)
-- parent.count

━━━━━━━━━━━━━━━━━━━━
+ TreeHost'Child is Interface

────────────────────
+ attach-child
    - child Child

━━━━━━━━━━━━━━━━━━━━
+ Node
    is TreeHost'Node

+ count = 0

────────────────────
+ do attach-child
    - child Node

count += 1

━━━━━━━━━━━━━━━━━━━━
+ do tree-append'Host'Child
    - parent Host
    - child Child
    - Host is TreeHost'Child

parent.attach-child(child)
```

```output
1
```

### A generic method body can pass an outer type parameter through two interface values without trying to infer that already-identical parameter in an empty call-match scope.

```yumi
. region = GenericRegion'I64(NumberDomain(42), NumberLayout())
region.refresh()

━━━━━━━━━━━━━━━━━━━━
+ GenericDomain'T is Interface

────────────────────
+ get value T

━━━━━━━━━━━━━━━━━━━━
+ GenericLayout'T is Interface

────────────────────
+ do fill
    - domain GenericDomain'T

━━━━━━━━━━━━━━━━━━━━
+ NumberDomain
    is GenericDomain'I64

+ value I64

────────────────────
+ This
    - value

━━━━━━━━━━━━━━━━━━━━
+ NumberLayout
    is GenericLayout'I64

────────────────────
+ do fill
    - domain GenericDomain'I64

-- domain.value

━━━━━━━━━━━━━━━━━━━━
+ GenericRegion'T

+ domain GenericDomain'T
+ layout GenericLayout'T

────────────────────
+ This
    - domain
    - layout

────────────────────
+ do refresh

layout.fill(domain)
```

```output
42
```

```yumi
/core/atom-pool

. atoms = AtomPool()
do finally
    atoms.release()

check'View(42 as Integer, 42 as Integer)
check'View(42 as Integer, 43 as Integer)
check'(Indexable'Char)("hello", "he" ++ "llo")
check'(Indexable'Char)("hello", "world")
check'(Indexable'I32)([1, 2, 3], [1, 2, 3])
check'(Indexable'I32)([1, 2, 3], [1, 2, 4])
check'View(AtomPair(atoms.store("one"), 0), AtomPair(atoms.store("one"), 0))
check'View(AtomPair(atoms.store("one"), 0), AtomPair(atoms.store("two"), 0))
check'View(0.0 as Real, -0.0 as Real)
. nan = 0.0f64 / 0.0f64
check'View(nan as Real, nan as Real)
. boxed-nan = (nan as Real) as View
-- boxed-nan == boxed-nan
-- boxed-nan != boxed-nan

━━━━━━━━━━━━━━━━━━━━
+ View is Interface

━━━━━━━━━━━━━━━━━━━━
+ Integer is I32
    is View

━━━━━━━━━━━━━━━━━━━━
+ Real is F64
    is View

━━━━━━━━━━━━━━━━━━━━
+ AtomPair is (value Atom, index I32)
    is View

━━━━━━━━━━━━━━━━━━━━
+ do check'Face'T
    - left T
    - right T

. boxed-left = left as Face
. boxed-right = right as Face
-- left == right
-- boxed-left == boxed-right
-- boxed-left != boxed-right
```

```output
true
true
false
false
false
true
true
true
false
false
false
true
true
true
false
false
false
true
true
true
false
false
false
true
true
true
false
false
false
true
false
true
```

```yumi
. pair = Packet'I32(7, 9)
. same = Packet'I32(7, 9)
. different = Packet'I32(7, 10)
. child = pair as Child
. parent = same as View
-- child == parent
-- parent == child
-- child != parent
-- child == (different as View)
. adapted = child as View
-- (type-of adapted).concrete-name
-- adapted == parent
. nested = Nested(child, 1) as View
. nested-same = Nested(parent, 1) as View
. nested-different = Nested(different as View, 1) as View
-- nested == nested-same
-- nested != nested-same
-- nested == nested-different
unsupported(pair as View, Packet'I64(7i64, 9i64) as View, false)
unsupported(pair as View, Packet'I64(7i64, 9i64) as View, true)

━━━━━━━━━━━━━━━━━━━━
+ View is Interface

━━━━━━━━━━━━━━━━━━━━
+ Child is Interface
    is View

━━━━━━━━━━━━━━━━━━━━
+ Packet'T is (first T, second T)
    is Child

━━━━━━━━━━━━━━━━━━━━
+ Nested is (inner View, marker I32)
    is View

━━━━━━━━━━━━━━━━━━━━
+ do unsupported
    - left View
    - right View
    - unequal Bool

. attempt = do new
    if unequal
        -- left != right
    else
        -- left == right
catch failure
    . failure Panic? = failure
    if failure
        -- "$failure"
attempt.wait()
```

```output
true
true
false
false
Packet'I32
true
true
false
false
Unsupported interface equality: concrete types differ
Unsupported interface equality: concrete types differ
```

```yumi
. state = Counts()
. value = Measured(state, 7)
. boxed = value as View
-- value == value
-- boxed == boxed
-- boxed != boxed
-- state.comparisons
state.comparisons = 0
-- make-value(state) == make-value(state)
-- state.operands
-- state.comparisons
panic-equality(boxed, state, false)
panic-equality(boxed, state, true)
-- state.comparisons

━━━━━━━━━━━━━━━━━━━━
+ View is Interface

━━━━━━━━━━━━━━━━━━━━
+ Counts

+ operands = 0
+ comparisons = 0
+ should-panic = false

━━━━━━━━━━━━━━━━━━━━
+ Measured is (state Counts, value I32)
    is View

────────────────────
+ do this == other Measured

state.comparisons += 1
. allocated = "$value:${other.value}"
if state.should-panic
    ?! Panic("comparison panic $allocated")
= value != other.value

━━━━━━━━━━━━━━━━━━━━
+ do make-value View
    - state Counts

state.operands += 1
= Measured(state, 7)

━━━━━━━━━━━━━━━━━━━━
+ do panic-equality
    - value View
    - state Counts
    - unequal Bool

. attempt = do new
    state.should-panic = true
    if unequal
        -- value != value
    else
        -- value == value
catch failure
    . failure Panic? = failure
    if failure
        -- "$failure"
attempt.wait()
```

```output
false
false
true
3
false
2
1
comparison panic 7:7
comparison panic 7:7
3
```

```yumi
. pair = Pair("first", "second") as View
. nested = Nested(pair, 1) as View
-- equal(nested, nested)

━━━━━━━━━━━━━━━━━━━━
+ View is Interface

━━━━━━━━━━━━━━━━━━━━
+ Pair is (first String, second String)
    is View

━━━━━━━━━━━━━━━━━━━━
+ Nested is (inner View, marker I32)
    is View

━━━━━━━━━━━━━━━━━━━━
+ do equal
    - left View
    - right View

= left == right
```

```output
true
```

### A public field satisfies an interface property that requires a setter.

```yumi
. nodes [Node]
nodes += Leaf()
nodes += Branch()
for node in nodes
    node.id = node.id + 10
    -- node.id

━━━━━━━━━━━━━━━━━━━━
+ Node is Interface

────────────────────
+ get id Size

────────────────────
+ do id = value Size

━━━━━━━━━━━━━━━━━━━━
+ Leaf
    is Node

+ id Size = 1

━━━━━━━━━━━━━━━━━━━━
+ Branch
    is Node

_ stored Size = 2

────────────────────
+ get id

stored

────────────────────
+ do id = value Size

stored = value
```

```output
11
12
```

### Interface values test, match, and cast against implementing classes and their subclasses.

```yumi
. nodes [Node]
nodes += Leaf()
nodes += Twig()
nodes += Branch(3)
nodes += Stone()
for node in nodes
    -- describe(node)
-- (nodes[2] as Branch).kids
-- nodes[1] is Leaf
-- nodes[3] is Leaf

━━━━━━━━━━━━━━━━━━━━
+ describe String
    - node Node

match node
    | leaf Leaf
        "leaf ${leaf.name}"
    | branch Branch
        "branch ${branch.kids}"
    ??
        "other"

━━━━━━━━━━━━━━━━━━━━
+ Node is Interface

────────────────────
+ get weight Int

━━━━━━━━━━━━━━━━━━━━
+ Leaf
    is Node

+ weight = 1

────────────────────
+ get name

"leaf"

━━━━━━━━━━━━━━━━━━━━
+ Twig is Leaf

────────────────────
+ get name

"twig"

━━━━━━━━━━━━━━━━━━━━
+ Branch
    is Node

+ kids Int
+ weight = 2

────────────────────
+ This
    - kids

━━━━━━━━━━━━━━━━━━━━
+ Stone is Tuple
    is Node

+ weight = 3
```

```output
leaf leaf
leaf twig
branch 3
other
3
true
false
```

### A covariant getter satisfies an interface getter that returns a wider interface.

```yumi
. child = Child()
. holder Holder = child
-- holder.owner == null
child.link = Parent()
-- (holder.owner!).tag

━━━━━━━━━━━━━━━━━━━━
+ Owner is Interface

────────────────────
+ get tag String

━━━━━━━━━━━━━━━━━━━━
+ Holder is Interface

────────────────────
+ get owner Owner?

━━━━━━━━━━━━━━━━━━━━
+ Parent
    is Owner

────────────────────
+ get tag

"parent"

━━━━━━━━━━━━━━━━━━━━
+ Child
    is Holder

+ link Parent?
+ owner => link
```

```output
true
parent
```

### Argument dispatch does not implicitly narrow an interface value to an implementing class.

```yumi crash
. named Named = Leaf()
-- describe(named as Leaf)
-- describe(named)

━━━━━━━━━━━━━━━━━━━━
+ Named is Interface

────────────────────
+ get label String

━━━━━━━━━━━━━━━━━━━━
+ Leaf
    is Named

+ label = "leaf"

━━━━━━━━━━━━━━━━━━━━
+ describe
    - leaf Leaf

leaf.label
```

```output
Exception: No function "describe" accepts (Named); it is declared as "describe(leaf Leaf)"
<no file:4>
    -- describe(named)
       ^^^^^^^^^^^^^^^
```

### Joining an interface with an implementing class keeps the interface type.

```yumi
for i in 0 .. < 4
    -- pick(i)?.label ?? "none"
. named Named = Leaf()
. missing Leaf? = null
-- (missing ?? named).label

━━━━━━━━━━━━━━━━━━━━
+ Named is Interface

────────────────────
+ get label String

━━━━━━━━━━━━━━━━━━━━
+ Leaf
    is Named

+ label = "leaf"

━━━━━━━━━━━━━━━━━━━━
+ Twig
    is Named

+ label = "twig"

━━━━━━━━━━━━━━━━━━━━
+ Stem
    is Named

+ label = "stem"

━━━━━━━━━━━━━━━━━━━━
+ pick Named?
    - index Int

|? index == 0
    Leaf()
|? index == 1
    Twig()
|? index == 2
    Stem()
??
    null
```

```output
leaf
twig
stem
none
leaf
```

### An inherited public field satisfies an interface setter on a subclass.

```yumi
. nodes [Node]
nodes += Leaf()
nodes += Twig()
for node in nodes
    node.id = node.id + 10
    -- node.id

━━━━━━━━━━━━━━━━━━━━
+ Node is Interface

────────────────────
+ get id Size

────────────────────
+ do id = value Size

━━━━━━━━━━━━━━━━━━━━
+ Leaf
    is Node

+ id Size = 1

━━━━━━━━━━━━━━━━━━━━
+ Twig is Leaf
```

```output
11
11
```

### A recursive list getter yields class values as interface values.

```yumi
. inner = Group("inner")
inner.add-leaf(Leaf("b"))
. root = Group("root")
root.add-leaf(Leaf("a"))
root.add-group(inner)
for node in root.descendants
    -- node.label
for node in root.nodes
    -- node.label

━━━━━━━━━━━━━━━━━━━━
+ Named is Interface

────────────────────
+ get label String

━━━━━━━━━━━━━━━━━━━━
+ Leaf
    is Named

+ label String

────────────────────
+ This
    - label

━━━━━━━━━━━━━━━━━━━━
+ Item

+ leaf Leaf?
+ group Group?

━━━━━━━━━━━━━━━━━━━━
+ Group
    is Named

+ label String
_ items [Item] = List'Item()

────────────────────
+ This
    - label

────────────────────
+ do add-leaf
    - leaf Leaf

. item = Item()
item.leaf = leaf
items += item

────────────────────
+ do add-group
    - group Group

. item = Item()
item.group = group
items += item

────────────────────
+ do get nodes [Named]

for item in items
    if item.leaf
        <= item.leaf
    else if item.group
        <= item.group

────────────────────
+ do get descendants [Named]

for item in items
    if item.leaf
        <= item.leaf
    else if item.group
        . group = item.group
        <= group
        for node in group.descendants
            <= node
```

```output
a
inner
b
a
inner
```

### Feed pipelines over interface-typed lists map, filter, test, and pick items.

```yumi
. nodes [Named] = List'Named()
nodes += Leaf("a")
nodes += Twig("b")
nodes += Leaf("c")
. found =
    ** ["a", "x", "c"]
    ?*> find(nodes, each)
    |> List
-- found.length
. labels =
    ** nodes
    *> each.label
    |> List
-- labels[2]
. twigs =
    ** nodes
    ?> each is Twig
    *> each as Twig
    |> List
-- twigs[0].label
. matches =
    ** nodes
    ?> each.label == "c"
    |> List
-- matches[0].label
. twig-list =
    ** nodes
    ?> each is Twig
    |> List
-- twig-list.length

━━━━━━━━━━━━━━━━━━━━
+ do find Named?
    - nodes [Named]
    - label String

for node in nodes
    =? node if node.label == label
= null

━━━━━━━━━━━━━━━━━━━━
+ Named is Interface

────────────────────
+ get label String

━━━━━━━━━━━━━━━━━━━━
+ Leaf
    is Named

+ label String

────────────────────
+ This
    - label

━━━━━━━━━━━━━━━━━━━━
+ Twig is Leaf

────────────────────
+ This
    - label
```

```output
2
c
b
c
1
```

### A single map stage over an interface-typed list compiles.

```yumi
. nodes [Named] = List'Named()
nodes += Leaf("a")
nodes += Leaf("c")
. kept =
    ** nodes
    *> each.label
    |> List
-- kept[1]

━━━━━━━━━━━━━━━━━━━━
+ Named is Interface

────────────────────
+ get label String

━━━━━━━━━━━━━━━━━━━━
+ Leaf
    is Named

+ label String

────────────────────
+ This
    - label
```

```output
c
```

### A panic in a method called through a value of an interface: the trace has the method, then the frame that takes the object out of the value, placed where the method is declared.

```yumi crash
. thing = Thing()
foo(thing)

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ bar Int
    - n Int

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Proto

────────────────────
+ do bar
    - n Int

?! Panic("bar $n")
    if n > 3
= n

━━━━━━━━━━━━━━━━━━━━
+ do foo
    - p Proto

-- p.bar(1)
-- p.bar(5)
```

```stderr
Unhandled panic: bar 5
<no file:20>

Task 1

Stack trace:
  0  __source__:Thing:bar no file:20:1
  1  __source__:bar(wrapped) no file:17:1
  2  __source__:foo no file:29:4
  3  __source__:main no file:3:1
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### A panic in a function an interface declares with a body, called through a value: no frame stands between it and its caller.

```yumi crash
. thing = Thing()
foo(thing)

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ do bar Int
    - n Int

?! Panic("default bar $n")
    if n > 3
= n

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Proto

━━━━━━━━━━━━━━━━━━━━
+ do foo
    - p Proto

-- p.bar(2)
-- p.bar(6)
```

```stderr
Unhandled panic: default bar 6
<no file:12>

Task 1

Stack trace:
  0  __source__:Proto:bar no file:12:1
  1  __source__:foo no file:25:4
  2  __source__:main no file:3:1
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### A panic in a function of an interface, run for a class that implements one derived from it: the trace has the function, the class's own frame for it, and the frame of the value.

```yumi crash
. thing = Thing()
. pair = thing as Foobar
-- pair.show(2)
-- pair.show(7)

━━━━━━━━━━━━━━━━━━━━
+ Foo is Interface

────────────────────
+ do show Int
    - n Int

?! Panic("show $n")
    if n > 3
= n

━━━━━━━━━━━━━━━━━━━━
+ Foobar is Interface
    is Foo

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Foobar
```

```stderr
Unhandled panic: show 7
<no file:14>

Task 1

Stack trace:
  0  __source__:Foo:show no file:14:1
  1  __source__:show no file:11:1
  2  __source__:show(wrapped) no file:11:1
  3  __source__:main no file:5:4
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### A panic in a function of an interface called on an object whose class declares none: the trace has the function as the interface's and as the class's.

```yumi crash
. thing = Thing()
-- thing.show(1)
-- thing.show(8)

━━━━━━━━━━━━━━━━━━━━
+ Foo is Interface

────────────────────
+ do show Int
    - n Int

?! Panic("show $n")
    if n > 3
= n

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Foo
```

```stderr
Unhandled panic: show 8
<no file:13>

Task 1

Stack trace:
  0  __source__:Foo:show no file:13:1
  1  __source__:show no file:10:1
  2  __source__:main no file:4:4
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### A panic in a method of a number type, called through a value of an interface a function was given.

```yumi crash
. count Proto = Count(9)
foo(count)

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ checked Int
    - n Int

━━━━━━━━━━━━━━━━━━━━
+ Count is I32
    is Proto

────────────────────
+ do checked
    - n Int

?! Panic("by $n")
    if n < 2
= n

━━━━━━━━━━━━━━━━━━━━
+ do foo
    - p Proto

-- p.checked(1)
```

```stderr
Unhandled panic: by 1
<no file:20>

Task 1

Stack trace:
  0  __source__:Count:checked no file:20:1
  1  __source__:checked(wrapped) no file:17:1
  2  __source__:foo no file:28:4
  3  __source__:main no file:3:1
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### A panic in a method of a tuple type, called through a value of an interface a function was given.

```yumi crash
. count Proto = Count(1, 2)
foo(count)

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ checked Int
    - n Int

━━━━━━━━━━━━━━━━━━━━
+ Count is (a Int, b Int)
    is Proto

────────────────────
+ do checked
    - n Int

?! Panic("by $n")
    if n < 2
= n

━━━━━━━━━━━━━━━━━━━━
+ do foo
    - p Proto

-- p.checked(1)
```

```stderr
Unhandled panic: by 1
<no file:20>

Task 1

Stack trace:
  0  __source__:Count:checked no file:20:1
  1  __source__:checked(wrapped) no file:17:1
  2  __source__:foo no file:28:4
  3  __source__:main no file:3:1
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### A function of an interface run through a value of one derived from it has the declaring interface as its receiver.

```yumi
. thing = Thing()
. pair = thing as Foobar
pair.show()
pair.tell()
. one = pair as Foo
one.show()

━━━━━━━━━━━━━━━━━━━━
+ Foo is Interface

────────────────────
+ do show

-- "show $this"
tell()

────────────────────
+ tell

━━━━━━━━━━━━━━━━━━━━
+ Foobar is Interface
    is Foo

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Foobar

────────────────────
+ do tell

-- "tell $this"
```

```output
show [Interface __source__:Foo]
tell [Object __source__:Thing]
tell [Object __source__:Thing]
show [Interface __source__:Foo]
tell [Object __source__:Thing]
```

### A function of an interface called on an object has the interface as its receiver, and calls the class's own function.

```yumi
. thing = Thing()
thing.show()

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ do show

-- "show $this"
tell()

────────────────────
+ tell

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Proto

────────────────────
+ do tell

-- "tell $this"
```

```output
show [Interface __source__:Proto]
tell [Object __source__:Thing]
```

### Whole numbers, a float, a tuple and a text held as values of an interface.

```yumi
. count = Count(42) as Proto
count.show()
-- count.twice(2)
. real = Real(3.5) as Proto
real.show()
-- real.twice(2)
. pair = Pair(1, 2.5) as Proto
pair.show()
-- pair.twice(3)
. label = Label("hi") as Proto
label.show()
-- label.twice(1)
. small = Small(200) as Proto
small.show()
. minus = Count(-7) as Proto
minus.show()
-- minus.twice(3)

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ show

────────────────────
+ twice Int
    - n Int

━━━━━━━━━━━━━━━━━━━━
+ Count is I32
    is Proto

────────────────────
+ do show

-- "count $this"

────────────────────
+ twice
    - n Int

this * n

━━━━━━━━━━━━━━━━━━━━
+ Small is U8
    is Proto

────────────────────
+ do show

-- "small $this"

────────────────────
+ twice
    - n Int

n

━━━━━━━━━━━━━━━━━━━━
+ Real is F64
    is Proto

────────────────────
+ do show

-- "real $this"

────────────────────
+ twice
    - n Int

n * 2

━━━━━━━━━━━━━━━━━━━━
+ Label is String
    is Proto

────────────────────
+ do show

-- "label $this"

────────────────────
+ twice
    - n Int

n + 100

━━━━━━━━━━━━━━━━━━━━
+ Pair is (a Int, b Float)
    is Proto

────────────────────
+ do show

-- "pair $a $b"

────────────────────
+ twice
    - n Int

a * n
```

```output
count 42
84
real 3.5
4
pair 1 2.5
3
label hi
101
small 200
count -7
-21
```

### A getter and a setter of an interface answered by a stored property and by functions, and the type of what a value holds.

```yumi
. leaf = Leaf()
. node Node = leaf
node.id = node.id + 10
-- node.id
-- leaf.id
node.name = "pear"
-- leaf.name
. branch Node = Branch()
branch.id = 7
-- branch.id
-- (type-of node).concrete-name
-- (type-of branch).concrete-name
-- (type-of node).name
-- type-of node

━━━━━━━━━━━━━━━━━━━━
+ Node is Interface

────────────────────
+ get id Size

────────────────────
+ do id = value Size

────────────────────
+ do name = value String

━━━━━━━━━━━━━━━━━━━━
+ Leaf
    is Node

+ id Size = 1
+ name = "apple"

━━━━━━━━━━━━━━━━━━━━
+ Branch
    is Node

_ stored Size = 2

────────────────────
+ get id

stored

────────────────────
+ do id = value Size

stored = value * 2

────────────────────
+ do name = value String

-- "named $value"
```

```output
11
11
pear
14
Leaf
Branch
Node
[TypeInfo Node]
```

### A function of an interface with untyped arguments, called through a value with several types.

```yumi
. proto Proto = Thing()
proto.foo(1)
proto.foo(1.5)
proto.foo("text")
proto.pair(1, "one")
proto.pair(2.5, 3)

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ foo
    - n

────────────────────
+ pair
    - a
    - b

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Proto

────────────────────
+ do foo
    - n

-- n

────────────────────
+ do pair
    - a
    - b

-- "$a and $b"
```

```output
1
1.5
text
1 and one
2.5 and 3
```

### A value of an interface cast to the class it holds and to a class that one is derived from.

```yumi
. a Node = Leaf()
. b Node = Twig()
. c Node = Branch(3)
-- (c as Branch).kids
-- (a as Leaf).name
-- (b as Leaf).name
-- (b as Twig).name

━━━━━━━━━━━━━━━━━━━━
+ Node is Interface

────────────────────
+ get weight Int

━━━━━━━━━━━━━━━━━━━━
+ Leaf
    is Node

+ weight = 1

────────────────────
+ get name

"leaf"

━━━━━━━━━━━━━━━━━━━━
+ Twig is Leaf

────────────────────
+ get name

"twig"

━━━━━━━━━━━━━━━━━━━━
+ Branch
    is Node

+ kids Int
+ weight = 2

────────────────────
+ This
    - kids

━━━━━━━━━━━━━━━━━━━━
+ Stone is Tuple
    is Node

+ weight = 3
```

```output
3
leaf
twig
twig
```

### A value of an interface tested with is against the classes it may hold.

```yumi
. a Node = Leaf()
. b Node = Twig()
. c Node = Branch(3)
. d Node = Stone()
-- a is Leaf
-- b is Leaf
-- c is Leaf
-- d is Leaf
-- a is Twig
-- b is Twig
-- c is Branch
-- d is Branch

━━━━━━━━━━━━━━━━━━━━
+ Node is Interface

────────────────────
+ get weight Int

━━━━━━━━━━━━━━━━━━━━
+ Leaf
    is Node

+ weight = 1

────────────────────
+ get name

"leaf"

━━━━━━━━━━━━━━━━━━━━
+ Twig is Leaf

────────────────────
+ get name

"twig"

━━━━━━━━━━━━━━━━━━━━
+ Branch
    is Node

+ kids Int
+ weight = 2

────────────────────
+ This
    - kids

━━━━━━━━━━━━━━━━━━━━
+ Stone is Tuple
    is Node

+ weight = 3
```

```output
true
true
false
false
false
true
true
false
```

### A match on a value of an interface by the classes it may hold.

```yumi
. a Node = Leaf()
. b Node = Twig()
. c Node = Branch(3)
. d Node = Stone()
-- describe(a)
-- describe(b)
-- describe(c)
-- describe(d)

━━━━━━━━━━━━━━━━━━━━
+ describe String
    - node Node

match node
    | leaf Leaf
        "leaf ${leaf.name}"
    | branch Branch
        "branch ${branch.kids}"
    ??
        "other"

━━━━━━━━━━━━━━━━━━━━
+ Node is Interface

────────────────────
+ get weight Int

━━━━━━━━━━━━━━━━━━━━
+ Leaf
    is Node

+ weight = 1

────────────────────
+ get name

"leaf"

━━━━━━━━━━━━━━━━━━━━
+ Twig is Leaf

────────────────────
+ get name

"twig"

━━━━━━━━━━━━━━━━━━━━
+ Branch
    is Node

+ kids Int
+ weight = 2

────────────────────
+ This
    - kids

━━━━━━━━━━━━━━━━━━━━
+ Stone is Tuple
    is Node

+ weight = 3
```

```output
leaf leaf
leaf twig
branch 3
other
```

### A value of an interface tested as a tuple type and cast to it.

```yumi
. a Node = Leaf()
. d Node = Stone()
-- d is Stone
-- a is Stone
. s = d as Stone
-- s.weight

━━━━━━━━━━━━━━━━━━━━
+ Node is Interface

────────────────────
+ get weight Int

━━━━━━━━━━━━━━━━━━━━
+ Leaf
    is Node

+ weight = 1

────────────────────
+ get name

"leaf"

━━━━━━━━━━━━━━━━━━━━
+ Twig is Leaf

────────────────────
+ get name

"twig"

━━━━━━━━━━━━━━━━━━━━
+ Branch
    is Node

+ kids Int
+ weight = 2

────────────────────
+ This
    - kids

━━━━━━━━━━━━━━━━━━━━
+ Stone is Tuple
    is Node

+ weight = 3
```

```output
true
false
3
```

### A value of an interface cast to a class it does not hold panics.

```yumi
do new
    . a Node = Leaf()
    . c = a as Branch
    -- c.kids
catch failure
    -- failure

━━━━━━━━━━━━━━━━━━━━
+ Node is Interface

────────────────────
+ get weight Int

━━━━━━━━━━━━━━━━━━━━
+ Leaf
    is Node

+ weight = 1

────────────────────
+ get name

"leaf"

━━━━━━━━━━━━━━━━━━━━
+ Twig is Leaf

────────────────────
+ get name

"twig"

━━━━━━━━━━━━━━━━━━━━
+ Branch
    is Node

+ kids Int
+ weight = 2

────────────────────
+ This
    - kids

━━━━━━━━━━━━━━━━━━━━
+ Stone is Tuple
    is Node

+ weight = 3
```

```output
Invalid cast to [Object __source__:Branch]
```

### A tuple held as a value of an interface, cast to a class, panics.

```yumi
do new
    . d Node = Stone()
    . c = d as Branch
    -- c.kids
catch failure
    -- failure

━━━━━━━━━━━━━━━━━━━━
+ Node is Interface

────────────────────
+ get weight Int

━━━━━━━━━━━━━━━━━━━━
+ Leaf
    is Node

+ weight = 1

────────────────────
+ get name

"leaf"

━━━━━━━━━━━━━━━━━━━━
+ Twig is Leaf

────────────────────
+ get name

"twig"

━━━━━━━━━━━━━━━━━━━━
+ Branch
    is Node

+ kids Int
+ weight = 2

────────────────────
+ This
    - kids

━━━━━━━━━━━━━━━━━━━━
+ Stone is Tuple
    is Node

+ weight = 3
```

```output
Invalid cast to [Object __source__:Branch]
```

### An object held as a value of an interface, cast to a tuple type, panics.

```yumi
do new
    . a Node = Leaf()
    . s = a as Stone
    -- s.weight
catch failure
    -- failure

━━━━━━━━━━━━━━━━━━━━
+ Node is Interface

────────────────────
+ get weight Int

━━━━━━━━━━━━━━━━━━━━
+ Leaf
    is Node

+ weight = 1

────────────────────
+ get name

"leaf"

━━━━━━━━━━━━━━━━━━━━
+ Twig is Leaf

────────────────────
+ get name

"twig"

━━━━━━━━━━━━━━━━━━━━
+ Branch
    is Node

+ kids Int
+ weight = 2

────────────────────
+ This
    - kids

━━━━━━━━━━━━━━━━━━━━
+ Stone is Tuple
    is Node

+ weight = 3
```

```output
Invalid cast to Stone
```

### An optional of an interface given objects, a tuple and null.

```yumi
. a Node? = pick(0)
. b Node? = pick(1)
. c Node? = pick(2)
-- a?.weight ?? 0
-- b?.weight ?? 0
-- c?.weight ?? 0
. w = Stone() as Node?
-- w?.weight ?? 0

━━━━━━━━━━━━━━━━━━━━
+ pick Node?
    - index Int

|? index == 0
    Leaf()
|? index == 1
    Branch(4)
??
    null

━━━━━━━━━━━━━━━━━━━━
+ Node is Interface

────────────────────
+ get weight Int

━━━━━━━━━━━━━━━━━━━━
+ Leaf
    is Node

+ weight = 1

────────────────────
+ get name

"leaf"

━━━━━━━━━━━━━━━━━━━━
+ Twig is Leaf

────────────────────
+ get name

"twig"

━━━━━━━━━━━━━━━━━━━━
+ Branch
    is Node

+ kids Int
+ weight = 2

────────────────────
+ This
    - kids

━━━━━━━━━━━━━━━━━━━━
+ Stone is Tuple
    is Node

+ weight = 3
```

```output
1
2
0
3
```

### A tuple derived from another and a struct held as values of an interface: a test names the exact type, and a cast gives a copy.

```yumi
. a Node = Leaf()
. d Node = Stone()
. p Node = Pebble()
. q Node = Rock(4, 5)
-- d.weight, p.weight, q.weight
-- p is Stone
-- p is Pebble
-- d is Pebble
-- q is Rock
-- a is Rock
. r = q as Rock
-- r.x, r.y, r.weight
r.x = 9
-- r.x, (q as Rock).x

━━━━━━━━━━━━━━━━━━━━
+ Pebble is Stone

+ extra = 4

━━━━━━━━━━━━━━━━━━━━
+ Rock is Struct
    is Node

+ x Int
+ y Int

────────────────────
+ get weight Int

x + y

━━━━━━━━━━━━━━━━━━━━
+ Node is Interface

────────────────────
+ get weight Int

━━━━━━━━━━━━━━━━━━━━
+ Leaf
    is Node

+ weight = 1

────────────────────
+ get name

"leaf"

━━━━━━━━━━━━━━━━━━━━
+ Twig is Leaf

────────────────────
+ get name

"twig"

━━━━━━━━━━━━━━━━━━━━
+ Branch
    is Node

+ kids Int
+ weight = 2

────────────────────
+ This
    - kids

━━━━━━━━━━━━━━━━━━━━
+ Stone is Tuple
    is Node

+ weight = 3
```

```output
(3, 3, 9)
false
true
false
true
false
(4, 5, 9)
(9, 4)
```

### Objects and a tuple tested as an interface they implement.

```yumi
. leaf = Leaf()
. stone = Stone()
. twig = Twig()
-- leaf is Node
-- stone is Node
-- twig is Node
-- 5 is Node
-- name(leaf)
-- name(stone)
-- name(twig)

━━━━━━━━━━━━━━━━━━━━
+ name String
    - thing

match thing
    | node Node
        "node ${node.weight}"
    ??
        "other"

━━━━━━━━━━━━━━━━━━━━
+ Node is Interface

────────────────────
+ get weight Int

━━━━━━━━━━━━━━━━━━━━
+ Leaf
    is Node

+ weight = 1

────────────────────
+ get name

"leaf"

━━━━━━━━━━━━━━━━━━━━
+ Twig is Leaf

────────────────────
+ get name

"twig"

━━━━━━━━━━━━━━━━━━━━
+ Branch
    is Node

+ kids Int
+ weight = 2

────────────────────
+ This
    - kids

━━━━━━━━━━━━━━━━━━━━
+ Stone is Tuple
    is Node

+ weight = 3
```

```output
true
true
true
false
node 1
node 3
node 1
```

### Tests of a value of an interface in if, is not and match?.

```yumi
. a Node = Leaf()
. b Node = Twig()
. c Node = Branch(3)
. d Node = Stone()
-- tell(a), tell(b), tell(c), tell(d)
. e Node? = b
if e is Leaf
    -- "is leaf"
match e
    | twig Twig
        -- twig.name
    ??
        -- "no"

━━━━━━━━━━━━━━━━━━━━
+ do tell String
    - node Node

if node is Twig
    = "twig"
if node is not Leaf
    = "not leaf"
match? node
    | leaf Leaf
        = leaf.name
= "none"

━━━━━━━━━━━━━━━━━━━━
+ Node is Interface

────────────────────
+ get weight Int

━━━━━━━━━━━━━━━━━━━━
+ Leaf
    is Node

+ weight = 1

────────────────────
+ get name

"leaf"

━━━━━━━━━━━━━━━━━━━━
+ Twig is Leaf

────────────────────
+ get name

"twig"

━━━━━━━━━━━━━━━━━━━━
+ Branch
    is Node

+ kids Int
+ weight = 2

────────────────────
+ This
    - kids

━━━━━━━━━━━━━━━━━━━━
+ Stone is Tuple
    is Node

+ weight = 3
```

```output
(leaf, twig, not leaf, not leaf)
is leaf
twig
```

### An object held as a base class converts to an interface with the table of its own class.

```yumi
. base Base = Child()
. shape Shape = base
-- shape.name()
. plain Shape = Base()
-- plain.name()
. grand Base = Grand()
. third Shape = grand
-- third.name()
-- third.sides
-- shape.sides

━━━━━━━━━━━━━━━━━━━━
+ Shape is Interface

────────────────────
+ name String

────────────────────
+ get sides Int

━━━━━━━━━━━━━━━━━━━━
+ Base
    is Shape

+ sides = 3

────────────────────
+ name

"base"

━━━━━━━━━━━━━━━━━━━━
+ Child is Base

────────────────────
+ name

"child"

━━━━━━━━━━━━━━━━━━━━
+ Grand is Child
```

```output
child
base
child
3
3
```

### A value of an interface derived from two others, given where each is asked for.

```yumi
. whole Both = Thing()
-- whole.first()
-- whole.second()
-- whole.third()
. one First = whole
-- one.first()
. two Second = whole
-- two.second()
tell(whole)

━━━━━━━━━━━━━━━━━━━━
+ First is Interface

────────────────────
+ first String

━━━━━━━━━━━━━━━━━━━━
+ Second is Interface

────────────────────
+ second String

━━━━━━━━━━━━━━━━━━━━
+ Both is Interface
    is First
    is Second

────────────────────
+ third String

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Both

────────────────────
+ first

"first"

────────────────────
+ second

"second"

────────────────────
+ third

"third"

━━━━━━━━━━━━━━━━━━━━
+ do tell
    - second Second

-- "told ${second.second()}"
```

```output
first
second
third
first
second
told second
```

### Values of an interface kept in a property, a tuple and an optional, and returned.

```yumi
. holder = Holder(Dog())
-- holder.pet.sound()
holder.pet = Cat()
-- holder.pet.sound()
-- pick(true).sound(), pick(false).sound()
. pair = (Dog() as Pet, Cat() as Pet)
-- pair[0].sound(), pair[1].sound()
-- find(0)?.sound() ?? "silence"
-- find(1)?.sound() ?? "silence"
-- find(2)?.sound() ?? "silence"

━━━━━━━━━━━━━━━━━━━━
+ Pet is Interface

────────────────────
+ sound String

━━━━━━━━━━━━━━━━━━━━
+ Dog
    is Pet

────────────────────
+ sound

"woof"

━━━━━━━━━━━━━━━━━━━━
+ Cat
    is Pet

────────────────────
+ sound

"meow"

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ pet Pet

────────────────────
+ This
    - pet

━━━━━━━━━━━━━━━━━━━━
+ pick Pet
    - dog Bool

|? dog
    Dog()
??
    Cat()

━━━━━━━━━━━━━━━━━━━━
+ find Pet?
    - which Int

|? which == 1
    Dog()
|? which == 2
    Cat()
??
    null
```

```output
woof
meow
(woof, meow)
(woof, meow)
silence
woof
meow
```

### Boxed values of an interface survive collection while a list alone holds them.

```yumi
. kept [Shape]
. i = 0
while i < 200
    kept += Pair(i, 0.5)
    kept += Label("label $i")
    kept += Count(i)
    churn(i)
    i += 1
. sum = 0
i = 0
while i < kept.length
    sum += kept[i].weight()
    i += 1
-- sum
-- kept[4].weight(), kept[301].weight(), kept[599].weight()

━━━━━━━━━━━━━━━━━━━━
+ Shape is Interface

────────────────────
+ weight Int

━━━━━━━━━━━━━━━━━━━━
+ Pair is (a Int, b Float)
    is Shape

────────────────────
+ weight

a

━━━━━━━━━━━━━━━━━━━━
+ Label is String
    is Shape

────────────────────
+ weight

size as Int

━━━━━━━━━━━━━━━━━━━━
+ Count is I32
    is Shape

────────────────────
+ weight

this * 2

━━━━━━━━━━━━━━━━━━━━
+ do churn
    - n Int

. junk [String]
. j = 0
while j < 50
    junk += "junk $n $j"
    j += 1
```

```output
61390
(7, 9, 398)
```

### A function an interface declares with a body calls a getter and a function that each class answers.

```yumi
. square Shape = Square(3)
. circle Shape = Circle(2)
-- square.describe()
-- circle.describe()
-- Square(4).describe()

━━━━━━━━━━━━━━━━━━━━
+ Shape is Interface

────────────────────
+ area Int

────────────────────
+ get name String

────────────────────
+ describe String

"$name of $area"
    .. area = this.area()

━━━━━━━━━━━━━━━━━━━━
+ Square
    is Shape

+ side Int
+ name = "square"

────────────────────
+ This
    - side

────────────────────
+ area

side * side

━━━━━━━━━━━━━━━━━━━━
+ Circle
    is Shape

+ radius Int

────────────────────
+ This
    - radius

────────────────────
+ area

radius * radius * 3

────────────────────
+ get name

"circle"
```

```output
square of 9
circle of 12
square of 16
```

### A method that changes its receiver, called through a value of an interface: an object is changed, and a struct the value holds is not.

```yumi
. counter = Counter(1)
. step Stepper = counter
step.bump()
step.bump()
-- step.value()
-- counter.n
. boxed Stepper = Tally(5)
boxed.bump()
-- boxed.value()

━━━━━━━━━━━━━━━━━━━━
+ Stepper is Interface

────────────────────
+ bump

────────────────────
+ value Int

━━━━━━━━━━━━━━━━━━━━
+ Counter
    is Stepper

+ n Int

────────────────────
+ This
    - n

────────────────────
+ do bump

n += 1

────────────────────
+ value

n

━━━━━━━━━━━━━━━━━━━━
+ Tally is Struct
    is Stepper

+ n Int

────────────────────
+ do bump

n += 1

────────────────────
+ value

n
```

```output
3
3
5
```

### An interface with a type parameter, implemented for two types.

```yumi
. box Box'Int = IntBox(4)
-- box.item
-- box.doubled()
. words Box'String = WordBox("hey")
-- words.item
-- words.doubled()

━━━━━━━━━━━━━━━━━━━━
+ Box'T is Interface

────────────────────
+ get item T

────────────────────
+ doubled T

item + item

━━━━━━━━━━━━━━━━━━━━
+ IntBox
    is Box'Int

+ item Int

────────────────────
+ This
    - item

━━━━━━━━━━━━━━━━━━━━
+ WordBox
    is Box'String

+ item String

────────────────────
+ This
    - item
```

```output
4
8
hey
heyhey
```

### A tree of values of an interface, each node of a class of its own.

```yumi
. tree Node = Branch(Leaf(2), Branch(Leaf(3), Leaf(4)))
-- tree.weight()
-- tree.depth()
. leaf Node = Leaf(9)
-- leaf.weight(), leaf.depth()

━━━━━━━━━━━━━━━━━━━━
+ Node is Interface

────────────────────
+ weight Int

────────────────────
+ depth Int

1

━━━━━━━━━━━━━━━━━━━━
+ Leaf
    is Node

+ n Int

────────────────────
+ This
    - n

────────────────────
+ weight

n

━━━━━━━━━━━━━━━━━━━━
+ Branch
    is Node

+ left Node
+ right Node

────────────────────
+ This
    - left
    - right

────────────────────
+ weight

left.weight() + right.weight()

────────────────────
+ do depth

. a = left.depth()
. b = right.depth()
= (if a > b then a else b) + 1
```

```output
9
3
(9, 1)
```

### A value of an interface captured by a closure, assigned after, and kept in a struct.

```yumi
. pet Pet = Dog()
. speak = => pet.sound()
-- speak()
pet = Cat()
-- speak()
-- call(speak)
. held = Held(pet, 3)
-- held.pet.sound(), held.count

━━━━━━━━━━━━━━━━━━━━
+ Pet is Interface

────────────────────
+ sound String

━━━━━━━━━━━━━━━━━━━━
+ Dog
    is Pet

────────────────────
+ sound

"woof"

━━━━━━━━━━━━━━━━━━━━
+ Cat
    is Pet

────────────────────
+ sound

"meow"

━━━━━━━━━━━━━━━━━━━━
+ Held is Struct

+ pet Pet
+ count Int

━━━━━━━━━━━━━━━━━━━━
+ do call String
    - fn () String

= fn()
```

```output
woof
meow
meow
(meow, 3)
```

### A class that implements two interfaces has a table for each.

```yumi
. thing = Thing()
. a A = thing
. b B = thing
-- a.name(), b.name()
-- (type-of a).concrete-name, (type-of b).concrete-name
-- a is Thing, b is Thing
-- (b as Thing).name()
show(thing)
show(Other())

━━━━━━━━━━━━━━━━━━━━
+ A is Interface

────────────────────
+ name String

━━━━━━━━━━━━━━━━━━━━
+ B is Interface

────────────────────
+ name String

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is A
    is B

────────────────────
+ name

"thing"

━━━━━━━━━━━━━━━━━━━━
+ Other
    is A

────────────────────
+ name

"other"

━━━━━━━━━━━━━━━━━━━━
+ do show
    - a A

-- "show ${a.name()}"
```

```output
(thing, thing)
(Thing, Thing)
(true, true)
thing
show thing
show other
```

### A function an interface declares with a body that cannot panic, called through a value.

```yumi
. p Proto = Thing()
-- p.name()
-- "after"

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ name String

"proto name"

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Proto
```

```output
proto name
after
```

### A function an interface declares with a body, run through a value that holds a number, has the number as its receiver.

```yumi
. n Proto = Num(3)
n.show()

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ do show

-- "show $this"

━━━━━━━━━━━━━━━━━━━━
+ Num is I32
    is Proto
```

```output
show 3
```

### A function an interface declares with a body, run on a number and on a tuple, has each as its receiver.

```yumi
Num(4).show()
Tup(1, 2).show()

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ do show

-- "show $this"

━━━━━━━━━━━━━━━━━━━━
+ Num is I32
    is Proto

━━━━━━━━━━━━━━━━━━━━
+ Tup is (a Int, b Int)
    is Proto
```

```output
show 4
show (1, 2)
```

### A panic in a function an interface declares with a body, run through a value that holds a number.

```yumi crash
. n Proto = Num(3)
run(n)

━━━━━━━━━━━━━━━━━━━━
+ do run
    - p Proto

-- p.show(1)
-- p.show(9)

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ do show Int
    - n Int

?! Panic("show $n")
    if n > 3
= n

━━━━━━━━━━━━━━━━━━━━
+ Num is I32
    is Proto
```

```stderr
Unhandled panic: show 9
<no file:19>

Task 1

Stack trace:
  0  __source__:Proto:show no file:19:1
  1  __source__:show(wrapped) no file:16:1
  2  __source__:run no file:10:4
  3  __source__:main no file:3:1
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### A panic in a function an interface declares with a body, run on a tuple.

```yumi crash
. n = Tup(3, 4)
-- n.show(1)
-- n.show(9)

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ do show Int
    - n Int

?! Panic("show $n")
    if n > 3
= n

━━━━━━━━━━━━━━━━━━━━
+ Tup is (a Int, b Int)
    is Proto
```

```stderr
Unhandled panic: show 9
<no file:13>

Task 1

Stack trace:
  0  __source__:Proto:show no file:13:1
  1  __source__:main no file:4:4
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

### A function of a delegate that changes it, called on the class and through a value: both change the one object.

```yumi
. thing = Thing()
thing.bump()
thing.bump()
-- thing.count
. proto Proto = thing
proto.bump()
-- proto.count
-- thing.foo.count

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ bump

────────────────────
+ get count Int

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ count = 0

────────────────────
+ do bump

count += 1

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Proto for
+ foo Foo
```

```output
2
3
3
```

### A function of a delegate with untyped arguments, called with whole numbers and a float, and a getter the interface declares with a body.

```yumi
. thing = Thing()
-- thing.total(1, 2)
-- thing.total(1.5, 2)
. proto Proto = thing
-- proto.total(3, 4)
-- thing.name
-- proto.name

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ total Int
    - a Int
    - b Int

────────────────────
+ get name String

"proto"

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ total
    - a
    - b

a + b

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Proto for
+ foo Foo
```

```output
3
3.5
7
proto
proto
```

### A class that delegates two interfaces to two properties.

```yumi
. thing = Thing()
thing.act()
thing.other()
. proto Proto = thing
proto.act()
. second Second = thing
second.other()

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ act

━━━━━━━━━━━━━━━━━━━━
+ Second is Interface

────────────────────
+ other

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ do act

-- "foo act"

────────────────────
+ do other

-- "foo other"

━━━━━━━━━━━━━━━━━━━━
+ Bar

────────────────────
+ do act

-- "bar act"

────────────────────
+ do other

-- "bar other"

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Proto for
+ foo Foo

as Second for
+ bar Bar
```

```output
foo act
bar other
foo act
bar other
```

### A function an interface declares with a body runs for the delegate where the class declares none, on the class and through a value, and calls the delegate's own.

```yumi
. thing = Thing()
thing.act()
. proto Proto = thing
proto.act()
proto.rest()
thing.rest()

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ act

────────────────────
+ do rest

-- "proto rest"
act()

━━━━━━━━━━━━━━━━━━━━
+ Foo
    is Proto

────────────────────
+ do act

-- "foo act"

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Proto for
+ foo Foo

────────────────────
+ do act

-- "thing act"
```

```output
thing act
thing act
proto rest
foo act
proto rest
foo act
```

### A class derived from one that delegates declares a function of the interface itself; the rest stay the delegate's.

```yumi
. thing = Derived()
thing.act()
thing.more()
. proto Proto = thing
proto.act()
proto.more()
. base Thing = thing
base.act()
. seen Proto = base
seen.more()

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ act

────────────────────
+ more

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ do act

-- "foo act"

────────────────────
+ do more

-- "foo more"

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Proto for
+ foo Foo

━━━━━━━━━━━━━━━━━━━━
+ Derived is Thing

────────────────────
+ do more

-- "derived more"
```

```output
foo act
derived more
foo act
derived more
foo act
derived more
```

### A class that delegates an interface is seen as the interface that one is derived from.

```yumi
. thing = Thing()
. proto Proto = thing
proto.act()
proto.root()
. root Root = thing
root.root()
. up Root = proto
up.root()

━━━━━━━━━━━━━━━━━━━━
+ Root is Interface

────────────────────
+ root

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface
    is Root

────────────────────
+ act

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ do act

-- "foo act"

────────────────────
+ do root

-- "foo root"

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Proto for
+ foo Foo
```

```output
foo act
foo root
foo root
foo root
```

### A function of a delegate that raises, caught on the class and through a value, and left to end the program.

```yumi crash
. thing = Thing()
. proto Proto = thing
-- catch proto.check(1)
-- catch proto.check(-1)
-- catch thing.check(2)
-- catch thing.check(-2)
-- proto.check(-3)

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ check Int ?! Problem
    - x Int

━━━━━━━━━━━━━━━━━━━━
+ Problem is Error

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ do check Int ?! Problem
    - x Int

?! Problem("negative")
    if x < 0
= x * 2

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Proto for
+ foo Foo
```

```output
2
negative
4
negative
```

### A value that holds a class that delegates is tested and cast as the class, and is no value of the delegate's class.

```yumi
. thing = Thing()
. proto Proto = thing
-- proto is Thing
-- proto is Foo
. back = proto as Thing
back.act()
-- (type-of proto).concrete-name
-- (type-of proto).name

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ act

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ do act

-- "foo act"

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Proto for
+ foo Foo
```

```output
true
false
foo act
Thing
Proto
```

### Values of an interface in a list, an optional and an argument, some holding a class that delegates.

```yumi
. thing = Thing()
. foo Proto = Foo()
. other Proto = thing
. list [Proto] = [other, foo, other]
for item in list
    item.act()
. maybe Proto? = thing
maybe?.act()
show(thing)

━━━━━━━━━━━━━━━━━━━━
+ do show
    - proto Proto

proto.act()

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ act

━━━━━━━━━━━━━━━━━━━━
+ Foo
    is Proto

+ name = "foo"

────────────────────
+ do act

-- "$name act"

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Proto for
+ foo Foo
```

```output
foo act
foo act
foo act
foo act
foo act
```

### A property a class delegates to is assigned again: calls on the class and through a value made before reach the new object.

```yumi
. thing = Thing()
thing.act()
thing.foo = Foo()
thing.foo.tag = "second"
thing.act()
. proto Proto = thing
proto.act()
. third = Foo()
third.tag = "third"
thing.foo = third
proto.act()
thing.act()

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ act

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ tag = "first"

────────────────────
+ do act

-- "$tag act"

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Proto for
+ foo = Foo()
```

```output
first act
second act
second act
third act
third act
```

### A class that delegates declares a getter of the interface itself, and a function with a body the delegate runs reads the delegate's.

```yumi
. thing = Thing()
-- thing.label
-- thing.describe()
. proto Proto = thing
-- proto.label
-- proto.describe()

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ get label String

────────────────────
+ describe String

"<$label>"

━━━━━━━━━━━━━━━━━━━━
+ Foo
    is Proto

+ label = "foo"

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Proto for
+ foo Foo

────────────────────
+ get label String

"thing"
```

```output
thing
<foo>
thing
<foo>
```

### A function an interface declares with a body reads a getter of the delegate it runs for.

```yumi
. thing = Thing()
-- thing.describe()
. proto Proto = thing
-- proto.describe()
-- proto.label

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ get label String

────────────────────
+ describe String

"<$label>"

━━━━━━━━━━━━━━━━━━━━
+ Foo
    is Proto

────────────────────
+ get label String

"foo"

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Proto for
+ foo Foo
```

```output
<foo>
<foo>
foo
```

### A function of a delegate reads the delegate's own property, called on the class and through a value.

```yumi
. thing = Thing()
-- thing.size()
. proto Proto = thing
-- proto.size()

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ size String

━━━━━━━━━━━━━━━━━━━━
+ Foo
    is Proto

+ label = "foo"

────────────────────
+ size String

label

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Proto for
+ foo Foo

+ label = "thing"
```

```output
foo
foo
```

### A function an interface declares with a body panics, run through a value made from a value of an interface derived from it: the stack trace has the function as the class runs it and the function of the table.

```yumi crash
. thing = Thing()
. pair = thing as Foobar
. one = pair as Foo
one.show()

━━━━━━━━━━━━━━━━━━━━
+ Foo is Interface

────────────────────
+ do show

-- "show"
. items = [1, 2, 3]
-- items[5]

━━━━━━━━━━━━━━━━━━━━
+ Foobar is Interface
    is Foo

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Foobar
```

```stderr
Unhandled panic: List index 5 out of range
<core/list.yumi>

Task 1

Stack trace:
  0  core/list:List:absent core/list.yumi
  1  __source__:Foo:show no file:15:4
  2  __source__:show no file:11:1
  3  __source__:show(wrapped) no file:11:1
  4  __source__:main no file:5:1
  5  __source__:init.__source__ no file
  6  __source__:brix.main no file
```

### A function an interface declares with a body calls one the class declares, which panics, through a value made from a value of a derived interface.

```yumi crash
. thing = Thing()
. pair = thing as Foobar
. one = pair as Foo
one.show()

━━━━━━━━━━━━━━━━━━━━
+ Foo is Interface

────────────────────
+ do show

-- "show"
tell()

────────────────────
+ tell

━━━━━━━━━━━━━━━━━━━━
+ Foobar is Interface
    is Foo

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Foobar

────────────────────
+ do tell

-- "tell"
. items = [1, 2, 3]
-- items[5]
```

```stderr
Unhandled panic: List index 5 out of range
<core/list.yumi>

Task 1

Stack trace:
  0  core/list:List:absent core/list.yumi
  1  __source__:Thing:tell no file:32:4
  2  __source__:tell(wrapped) no file:28:1
  3  __source__:Foo:show no file:14:1
  4  __source__:show no file:11:1
  5  __source__:show(wrapped) no file:11:1
  6  __source__:main no file:5:1
  7  __source__:init.__source__ no file
  8  __source__:brix.main no file
```

### A function an interface declares with a body panics through a value of an interface two interfaces derive from in turn.

```yumi crash
. thing = Thing()
. baz = thing as Baz
. bar = baz as Bar
. foo = bar as Foo
foo.show()

━━━━━━━━━━━━━━━━━━━━
+ Foo is Interface

────────────────────
+ do show

-- "show"
. items = [1, 2, 3]
-- items[5]

━━━━━━━━━━━━━━━━━━━━
+ Bar is Interface
    is Foo

━━━━━━━━━━━━━━━━━━━━
+ Baz is Interface
    is Bar

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Baz
```

```stderr
Unhandled panic: List index 5 out of range
<core/list.yumi>

Task 1

Stack trace:
  0  core/list:List:absent core/list.yumi
  1  __source__:Foo:show no file:16:4
  2  __source__:show no file:12:1
  3  __source__:show(wrapped) no file:12:1
  4  __source__:main no file:6:1
  5  __source__:init.__source__ no file
  6  __source__:brix.main no file
```

### A getter an interface declares with a body panics, read through a value: the stack trace has the getter and the function of the table.

```yumi crash
. thing = Thing()
. one = thing as Foo
-- one.name

━━━━━━━━━━━━━━━━━━━━
+ Foo is Interface

────────────────────
+ do get name String

. items = [1, 2, 3]
-- items[5]
= "foo"

━━━━━━━━━━━━━━━━━━━━
+ Foobar is Interface
    is Foo

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Foobar
```

```stderr
Unhandled panic: List index 5 out of range
<core/list.yumi>

Task 1

Stack trace:
  0  core/list:List:absent core/list.yumi
  1  __source__:Foo:name no file:13:4
  2  __source__:name(wrapped) no file:10:1
  3  __source__:main no file:4:4
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### A getter an interface declares with a body panics, read on an object: the stack trace has the getter alone.

```yumi crash
. thing = Thing()
-- thing.name

━━━━━━━━━━━━━━━━━━━━
+ Foo is Interface

────────────────────
+ do get name String

. items = [1, 2, 3]
-- items[5]
= "foo"

━━━━━━━━━━━━━━━━━━━━
+ Foobar is Interface
    is Foo

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Foobar
```

```stderr
Unhandled panic: List index 5 out of range
<core/list.yumi>

Task 1

Stack trace:
  0  core/list:List:absent core/list.yumi
  1  __source__:Foo:name no file:12:4
  2  __source__:main no file:3:4
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### A getter an interface declares with a body panics, read through a value of an interface derived from it.

```yumi crash
. thing = Thing()
. pair = thing as Foobar
-- pair.name

━━━━━━━━━━━━━━━━━━━━
+ Foo is Interface

────────────────────
+ do get name String

. items = [1, 2, 3]
-- items[5]
= "foo"

━━━━━━━━━━━━━━━━━━━━
+ Foobar is Interface
    is Foo

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Foobar
```

```stderr
Unhandled panic: List index 5 out of range
<core/list.yumi>

Task 1

Stack trace:
  0  core/list:List:absent core/list.yumi
  1  __source__:Foo:name no file:13:4
  2  __source__:name(wrapped) no file:10:1
  3  __source__:main no file:4:4
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### A setter an interface declares with a body panics, assigned through a value.

```yumi crash
. thing = Thing()
. one = thing as Foo
one.name = "a"

━━━━━━━━━━━━━━━━━━━━
+ Foo is Interface

────────────────────
+ do name = value String

. items = [1, 2, 3]
-- items[5]
-- value

━━━━━━━━━━━━━━━━━━━━
+ Foobar is Interface
    is Foo

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Foobar
```

```stderr
Unhandled panic: List index 5 out of range
<core/list.yumi>

Task 1

Stack trace:
  0  core/list:List:absent core/list.yumi
  1  __source__:Foo:name no file:13:4
  2  __source__:name(wrapped) no file:10:1
  3  __source__:main no file:4:1
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### A function an interface declares with a body, run through a value of a base interface for an object of a derived class, calls the derived class's function, which panics.

```yumi crash
. thing = Derived()
. pair = thing as Foobar
. one = pair as Foo
one.show()
pair.show()
. base Base = thing
. other = base as Foobar
. last = other as Foo
last.show()

━━━━━━━━━━━━━━━━━━━━
+ Foo is Interface

────────────────────
+ do show

-- "show $this"
tell()

────────────────────
+ tell

━━━━━━━━━━━━━━━━━━━━
+ Foobar is Interface
    is Foo

━━━━━━━━━━━━━━━━━━━━
+ Base
    is Foobar

────────────────────
+ do tell

-- "base tell"

━━━━━━━━━━━━━━━━━━━━
+ Derived is Base

────────────────────
+ do tell

-- "derived tell"
. items = [1, 2, 3]
-- items[5]
```

```stderr
Unhandled panic: List index 5 out of range
<core/list.yumi>

Task 1

Stack trace:
  0  core/list:List:absent core/list.yumi
  1  __source__:Derived:tell no file:45:4
  2  __source__:tell(wrapped) no file:41:1
  3  __source__:Foo:show no file:19:1
  4  __source__:show no file:16:1
  5  __source__:show(wrapped) no file:16:1
  6  __source__:main no file:5:1
  7  __source__:init.__source__ no file
  8  __source__:brix.main no file
```

### A value of a base interface made by two ways from one object runs the interface's function by either, and the second call panics.

```yumi crash
. thing = Thing()
. baz = thing as Baz
. bar = thing as Bar
. foo = baz as Foo
. also = bar as Foo
foo.show()
also.show()

━━━━━━━━━━━━━━━━━━━━
+ Foo is Interface

────────────────────
+ do show

-- "show"
tell()

────────────────────
+ tell

━━━━━━━━━━━━━━━━━━━━
+ Bar is Interface
    is Foo

━━━━━━━━━━━━━━━━━━━━
+ Baz is Interface
    is Bar

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Baz

+ count = 0

────────────────────
+ do tell

count += 1
-- "tell $count"
if count == 2
    . items = [1, 2, 3]
    -- items[5]
```

```stderr
Unhandled panic: List index 5 out of range
<core/list.yumi>

Task 1

Stack trace:
  0  core/list:List:absent core/list.yumi
  1  __source__:Thing:tell no file:43:8
  2  __source__:tell(wrapped) no file:37:1
  3  __source__:Foo:show no file:17:1
  4  __source__:show no file:14:1
  5  __source__:show(wrapped) no file:14:1
  6  __source__:main no file:8:1
  7  __source__:init.__source__ no file
  8  __source__:brix.main no file
```

### A getter an interface declares with a body, read on an object, has the object as its receiver.

```yumi
. thing = Thing()
-- thing.label

━━━━━━━━━━━━━━━━━━━━
+ Foo is Interface

────────────────────
+ do get label String

= "label of $this"

────────────────────
+ do label = value String

-- "label = $value of $this"

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Foo
```

```output
label of [Object __source__:Thing]
```

### A getter and a setter an interface declares with bodies, used through a value, have the object as their receiver.

```yumi
. thing = Thing()
. one = thing as Foo
-- one.label
one.label = "set"

━━━━━━━━━━━━━━━━━━━━
+ Foo is Interface

────────────────────
+ do get label String

= "label of $this"

────────────────────
+ do label = value String

-- "label = $value of $this"

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Foo
```

```output
label of [Object __source__:Thing]
label = set of [Object __source__:Thing]
```

### A getter an interface declares with a body calls a function the interface declares with a body, whose receiver is the interface.

```yumi
. thing = Thing()
. one = thing as Foo
-- one.label

━━━━━━━━━━━━━━━━━━━━
+ Foo is Interface

────────────────────
+ do get label String

= describe()

────────────────────
+ do describe String

= "described $this"

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Foo
```

```output
described [Interface __source__:Foo]
```

### A function with an argument and an answer that an interface declares with a body, run through a value of the interface and through one made from a derived interface.

```yumi crash
. thing = Thing()
. direct = thing as Foo
. pair = thing as Foobar
. one = pair as Foo
-- one.show(1)
-- direct.show(2)

━━━━━━━━━━━━━━━━━━━━
+ Foo is Interface

────────────────────
+ do show Int
    - n Int

-- "show $n"
. items = [1, 2, 3]
= items[n + 1]

━━━━━━━━━━━━━━━━━━━━
+ Foobar is Interface
    is Foo

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Foobar
```

```output
show 1
3
show 2
```

### A function with an untyped argument that an interface declares with a body, called with three types through a value made from a derived interface.

```yumi
. thing = Thing()
. pair = thing as Foobar
. one = pair as Foo
-- one.pick(1)
-- one.pick("a")
-- one.pick(2.5)

━━━━━━━━━━━━━━━━━━━━
+ Foo is Interface

────────────────────
+ do pick
    - value

-- "pick $value"
= value

━━━━━━━━━━━━━━━━━━━━
+ Foobar is Interface
    is Foo

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Foobar
```

```output
pick 1
1
pick a
a
pick 2.5
2.5
```

### Getters and a setter an interface declares with bodies, used through a value of the interface and through one made from a derived interface.

```yumi
. thing = Thing()
. pair = thing as Foobar
. one = pair as Foo
-- one.label
one.label = "set"
-- one.count
. direct = thing as Foo
-- direct.label
direct.label = "again"

━━━━━━━━━━━━━━━━━━━━
+ Foo is Interface

────────────────────
+ do get label String

= "label of $this"

────────────────────
+ do label = value String

-- "label = $value"

────────────────────
+ do get count Int

. items = [1, 2, 3]
= items[2]

━━━━━━━━━━━━━━━━━━━━
+ Foobar is Interface
    is Foo

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Foobar
```

```output
label of [Object __source__:Thing]
label = set
3
label of [Object __source__:Thing]
label = again
```

### A function that yields, declared by an interface with a body, read through a value made from a derived interface.

```yumi
. thing = Thing()
. pair = thing as Foobar
. one = pair as Foo
for n in one.count-up(3)
    -- n

━━━━━━━━━━━━━━━━━━━━
+ Foo is Interface

────────────────────
+ do count-up
    - limit Int

for n in 0 .. < limit
    <= n

━━━━━━━━━━━━━━━━━━━━
+ Foobar is Interface
    is Foo

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Foobar
```

```output
0
1
2
```

### A function that answers an optional, declared by an interface with a body, called through a value made from a derived interface.

```yumi
. thing = Thing()
. pair = thing as Foobar
. one = pair as Foo
-- (one.show() ?? "none")

━━━━━━━━━━━━━━━━━━━━
+ Foo is Interface

────────────────────
+ do show String?

-- "show"
= null

━━━━━━━━━━━━━━━━━━━━
+ Foobar is Interface
    is Foo

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Foobar
```

```output
show
none
```

### Values of a derived interface in a list, each seen as the base interface: one class takes the interface's function and the other declares its own.

```yumi crash
. things [Foobar] = [Thing(), Other()]
for thing in things
    . one = thing as Foo
    one.show()

━━━━━━━━━━━━━━━━━━━━
+ Foo is Interface

────────────────────
+ do show

-- "show $this"
tell()

────────────────────
+ tell

━━━━━━━━━━━━━━━━━━━━
+ Foobar is Interface
    is Foo

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Foobar

────────────────────
+ do tell

-- "thing tell"

━━━━━━━━━━━━━━━━━━━━
+ Other
    is Foobar

────────────────────
+ do show

-- "other show"
tell()

────────────────────
+ do tell

-- "other tell"
. items = [1, 2, 3]
-- items[5]
```

```output
show [Interface __source__:Foo]
thing tell
other show
other tell
```

### Functions of a value of an interface, one bound with an argument and one the interface declares with a body, are made values of a function type in a loop over two classes.

```yumi
. things [Proto]
things += Foo()
things += Baz()
for thing in things
    . held = thing.bar(3, ...)
    . fn = held as Taker
    -- fn(4)
    -- run(thing.twice)

━━━━━━━━━━━━━━━━━━━━
+ Taker is () Int
    - a Int

━━━━━━━━━━━━━━━━━━━━
+ run
    - fn Taker

fn(2) + 1

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ bar Int
    - a Int
    - b Int

────────────────────
+ twice Int
    - a Int

bar(a, a) * 2

━━━━━━━━━━━━━━━━━━━━
+ Foo
    is Proto

+ c = 10

────────────────────
+ bar
    - a
    - b

a + b + c

━━━━━━━━━━━━━━━━━━━━
+ Baz
    is Proto

────────────────────
+ bar
    - a
    - b

a * b
```

```output
17
29
12
9
```

### A function of a value of an interface bound with an argument, and one the interface declares with a body, are cast to a function type that gives and takes other numbers.

```yumi
. foo = Foo() as Proto
. fn = foo.bar(1, ...) as Wide
-- fn(2.9)
. tw = foo.twice as Wide
-- tw(1.5)

━━━━━━━━━━━━━━━━━━━━
+ Wide is () Float
    - a Float

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ bar Int
    - a Int
    - b Int

────────────────────
+ twice Int
    - a Int

bar(a, a) * 2

━━━━━━━━━━━━━━━━━━━━
+ Foo
    is Proto

+ c = 10

────────────────────
+ bar
    - a
    - b

a + b + c

━━━━━━━━━━━━━━━━━━━━
+ Baz
    is Proto

────────────────────
+ bar
    - a
    - b

a * b
```

```output
13
24
```

### A function of a value of an interface, bound with an argument and called as a value of a function type, panics in the class: the stack trace has the class's function, the function of the table and the function made for the value.

```yumi crash
. foo = Picker() as Picks
. fn = foo.pick(1, ...) as Taker
-- fn(1)
-- fn(7)

━━━━━━━━━━━━━━━━━━━━
+ Taker is () Int
    - a Int

━━━━━━━━━━━━━━━━━━━━
+ Picks is Interface

────────────────────
+ pick Int
    - a Int
    - b Int

━━━━━━━━━━━━━━━━━━━━
+ Picker
    is Picks

+ items = [1, 2, 3]

────────────────────
+ pick
    - a
    - b

items[a + b]
```

```stderr
Unhandled panic: List index 8 out of range
<core/list.yumi>

Task 1

Stack trace:
  0  core/list:List:absent core/list.yumi
  1  __source__:Picker:pick no file:30:1
  2  __source__:pick(wrapped) no file:26:1
  3  __source__:pickCasting no file:3:8
  4  __source__:main no file:5:4
  5  __source__:init.__source__ no file
  6  __source__:brix.main no file
```

### A function an interface declares with a body, given as a function of a value where a function type is taken, calls one of the class that panics.

```yumi crash
. foo = Picker() as Picks
-- run(foo.pick)
-- run(foo.both)

━━━━━━━━━━━━━━━━━━━━
+ Taker is () Int
    - a Int

━━━━━━━━━━━━━━━━━━━━
+ Picks is Interface

────────────────────
+ pick Int
    - a Int

────────────────────
+ both Int
    - a Int

pick(a) + pick(a + 1)

━━━━━━━━━━━━━━━━━━━━
+ Picker
    is Picks

+ items = [1, 2, 3]

────────────────────
+ pick
    - a

items[a]

━━━━━━━━━━━━━━━━━━━━
+ run
    - fn Taker

fn(0) + fn(2)
```

```stderr
Unhandled panic: List index 3 out of range
<core/list.yumi>

Task 1

Stack trace:
  0  core/list:List:absent core/list.yumi
  1  __source__:Picker:pick no file:33:1
  2  __source__:pick(wrapped) no file:30:1
  3  __source__:Picks:both no file:21:11
  4  __source__:bothCasting no file:4:8
  5  __source__:run no file:39:9
  6  __source__:main no file:4:4
  7  __source__:init.__source__ no file
  8  __source__:brix.main no file
```

### A property a class answers an interface through holds an object of a class derived from the property's: a function called through a value of the interface is the derived class's.

```yumi
. thing = Thing()
. proto Proto = thing
proto.act()

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ act

━━━━━━━━━━━━━━━━━━━━
+ FooBase

────────────────────
+ do act

-- "foobase act"

━━━━━━━━━━━━━━━━━━━━
+ Foo is FooBase

────────────────────
+ do act

-- "foo act"

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Proto for
+ foo FooBase = Foo()
```

```output
foo act
```

### A property a class answers an interface through is assigned an object of a derived class: functions called on the object and through a value are the derived class's.

```yumi
. thing = Thing()
thing.foo = SubFoo()
thing.act()
. proto Proto = thing
proto.act()
-- proto.size(2)
-- thing.size(2)

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ act

────────────────────
+ size Int
    - n Int

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ do act

-- "foo act"

────────────────────
+ size Int
    - n Int

n

━━━━━━━━━━━━━━━━━━━━
+ SubFoo is Foo

────────────────────
+ do act

-- "subfoo act"

────────────────────
+ size Int
    - n Int

n * 100

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Proto for
+ foo Foo
```

```output
subfoo act
subfoo act
200
200
```

### A lazy property a class answers an interface through makes an object of a derived class, whose function a value of the interface calls.

```yumi
. thing = Thing()
thing.act()
. proto Proto = thing
proto.act()

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ act

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ do act

-- "foo act"

━━━━━━━━━━━━━━━━━━━━
+ SubFoo is Foo

────────────────────
+ do act

-- "subfoo act"

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Proto for
+ foo Foo := SubFoo()
```

```output
subfoo act
subfoo act
```

### A field of a tuple that answers an interface holds an object of a derived class, whose function a value of the interface calls.

```yumi
. thing = Thing(SubFoo())
thing.act()
. proto Proto = thing
proto.act()

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ act

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ do act

-- "foo act"

━━━━━━━━━━━━━━━━━━━━
+ SubFoo is Foo

────────────────────
+ do act

-- "subfoo act"

━━━━━━━━━━━━━━━━━━━━
+ Thing is Tuple

as Proto for
+ foo Foo
```

```output
subfoo act
subfoo act
```

### A class answers an interface through a property whose class answers it through another, which holds an object of a derived class.

```yumi
. thing = Thing()
thing.act()
. proto Proto = thing
proto.act()

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ act

━━━━━━━━━━━━━━━━━━━━
+ Foo
    is Proto

────────────────────
+ do act

-- "foo act"

━━━━━━━━━━━━━━━━━━━━
+ SubFoo is Foo

────────────────────
+ do act

-- "subfoo act"

━━━━━━━━━━━━━━━━━━━━
+ Bar

as Proto for
+ foo Foo = SubFoo()

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Proto for
+ bar = Bar()
```

```output
subfoo act
subfoo act
```

### A function an interface declares with a body is declared again by a class derived from the class of the property that answers: a value of the interface calls the derived class's.

```yumi
. thing = Thing()
thing.foo = SubFoo()
. proto Proto = thing
proto.rest()
thing.rest()

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ act

────────────────────
+ do rest

-- "proto rest"
act()

━━━━━━━━━━━━━━━━━━━━
+ Foo
    is Proto

────────────────────
+ do act

-- "foo act"

━━━━━━━━━━━━━━━━━━━━
+ SubFoo is Foo

────────────────────
+ do act

-- "subfoo act"

────────────────────
+ do rest

-- "subfoo rest"

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Proto for
+ foo Foo
```

```output
subfoo rest
subfoo rest
```

### A function of a value of an interface that a property answers is cast to a function type and bound: each calls the function of the derived class the property holds.

```yumi
. thing = Thing()
thing.foo = SubFoo()
. proto Proto = thing
. f = proto.size as Wide
-- f(2.5)
. g = proto.size(...)
-- g(3)

━━━━━━━━━━━━━━━━━━━━
+ Wide is () Float
    - n Float

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ size Int
    - n Int

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ size Int
    - n Int

n

━━━━━━━━━━━━━━━━━━━━
+ SubFoo is Foo

────────────────────
+ size Int
    - n Int

n * 100

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Proto for
+ foo Foo
```

```output
200
300
```

### Functions with arguments and a getter answered through a property, before and after it is assigned an object of a derived class.

```yumi
. thing = Thing()
. proto Proto = thing
-- proto.total(1, 2)
thing.foo = Sub()
-- proto.total(1, 2)
-- thing.total(1, 2)
-- proto.name
-- thing.name

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ total Int
    - a Int
    - b Int

────────────────────
+ get name String

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ name = "foo"

────────────────────
+ total Int
    - a Int
    - b Int

a + b

━━━━━━━━━━━━━━━━━━━━
+ Sub is Foo

────────────────────
+ This

name = "sub"

────────────────────
+ total Int
    - a Int
    - b Int

a * b * 100

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Proto for
+ foo Foo
```

```output
3
200
200
sub
sub
```

### A function of a derived class the property that answers holds panics, called through a value of the interface: the stack trace names it.

```yumi crash
. thing = Thing()
. proto Proto = thing
-- proto.pick(1)
thing.foo = SubFoo()
-- proto.pick(1)
-- proto.pick(7)

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ pick Int
    - n Int

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ do pick Int
    - n Int

= n

━━━━━━━━━━━━━━━━━━━━
+ SubFoo is Foo

────────────────────
+ do pick Int
    - n Int

. items = [10, 20, 30]
= items[n]

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Proto for
+ foo Foo
```

```stderr
Unhandled panic: List index 7 out of range
<core/list.yumi>

Task 1

Stack trace:
  0  core/list:List:absent core/list.yumi
  1  __source__:SubFoo:pick no file:33:3
  2  __source__:pick(wrapped) no file:20:1
  3  __source__:main no file:7:4
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### A property that answers an interface is of a class that answers it through its own property; each is assigned an object of a derived class in turn.

```yumi
. outer = Outer()
. proto Proto = outer
proto.act()
outer.thing.foo = SubFoo()
proto.act()
outer.thing = SubThing()
proto.act()

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ act

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ do act

-- "foo act"

━━━━━━━━━━━━━━━━━━━━
+ SubFoo is Foo

────────────────────
+ do act

-- "subfoo act"

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Proto for
+ foo Foo

━━━━━━━━━━━━━━━━━━━━
+ SubThing is Thing

────────────────────
+ do act

-- "subthing act"

━━━━━━━━━━━━━━━━━━━━
+ Outer

as Proto for
+ thing Thing
```

```output
foo act
subfoo act
foo act
```

### A function answered through a property returns an object, and the derived class the property holds returns one of a derived class.

```yumi
. thing = Thing()
thing.foo = SubFoo()
. proto Proto = thing
. made = proto.make(2)
-- made.label
. list = [proto, proto]
for item in list
    -- item.make(3).label

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ make Named
    - n Int

━━━━━━━━━━━━━━━━━━━━
+ Named

+ label = "named"

━━━━━━━━━━━━━━━━━━━━
+ Special is Named

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ do make Named
    - n Int

. named = Named()
named.label = "foo $n"
= named

━━━━━━━━━━━━━━━━━━━━
+ SubFoo is Foo

────────────────────
+ do make Named
    - n Int

. special = Special()
special.label = "subfoo $n"
= special

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Proto for
+ foo Foo
```

```output
subfoo 2
subfoo 3
subfoo 3
```

### A list of values of an interface made of objects whose property holds the class or one derived from it, with functions that take arguments.

```yumi
. boxes [Proto] = [Thing(Foo()), Thing(SubFoo()), Thing(Foo())]
for box in boxes
    box.act()
    -- box.total(2, 3)

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ act

────────────────────
+ total Int
    - a Int
    - b Int

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ do act

-- "foo act"

────────────────────
+ total Int
    - a Int
    - b Int

a + b

━━━━━━━━━━━━━━━━━━━━
+ SubFoo is Foo

────────────────────
+ do act

-- "subfoo act"

────────────────────
+ total Int
    - a Int
    - b Int

a * b

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Proto for
+ foo Foo

────────────────────
+ This
    - foo
```

```output
foo act
5
subfoo act
6
foo act
5
```

### A function the class of the property that answers inherits from its base class, called on the object that delegates.

```yumi
. thing = Thing()
thing.act()

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ act

━━━━━━━━━━━━━━━━━━━━
+ FooBase

────────────────────
+ do act

-- "foobase act of $this"

━━━━━━━━━━━━━━━━━━━━
+ Foo is FooBase

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Proto for
+ foo Foo
```

```output
foobase act of [Object __source__:FooBase]
```

### A stored property and a getter the class of the property that answers inherits from its base class, read on the object and through a value.

```yumi
. thing = Thing()
-- thing.size
-- thing.name
. proto Proto = thing
-- proto.size
-- proto.name

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ get size Int

────────────────────
+ get name String

━━━━━━━━━━━━━━━━━━━━
+ FooBase

+ size = 4

────────────────────
+ get name String

"base name"

━━━━━━━━━━━━━━━━━━━━
+ Foo is FooBase

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Proto for
+ foo Foo
```

```output
4
base name
4
base name
```

### The base class of the property that answers declares the interface and the function, called on the object and through a value.

```yumi
. thing = Thing()
thing.act()
. proto Proto = thing
proto.act()

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ act

━━━━━━━━━━━━━━━━━━━━
+ FooBase
    is Proto

────────────────────
+ do act

-- "foobase act of $this"

━━━━━━━━━━━━━━━━━━━━
+ Foo is FooBase

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Proto for
+ foo Foo
```

```output
foobase act of [Object __source__:FooBase]
foobase act of [Object __source__:FooBase]
```

### The class of the property that answers inherits one function of the interface and declares another, on the object and through a value.

```yumi
. thing = Thing()
thing.act()
thing.more()
. proto Proto = thing
proto.act()
proto.more()

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ act

────────────────────
+ more

━━━━━━━━━━━━━━━━━━━━
+ FooBase

────────────────────
+ do act

-- "foobase act of $this"

────────────────────
+ do more

-- "foobase more"

━━━━━━━━━━━━━━━━━━━━
+ Foo is FooBase

────────────────────
+ do more

-- "foo more"

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Proto for
+ foo Foo
```

```output
foobase act of [Object __source__:FooBase]
foo more
foobase act of [Object __source__:FooBase]
foo more
```

### A function declared two classes above the class of the property that answers, which is assigned an object of a derived class and then one of its own.

```yumi
. thing = Thing()
thing.foo = Low()
. proto Proto = thing
proto.act()
thing.act()
thing.foo = Mid()
proto.act()
thing.act()

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ act

━━━━━━━━━━━━━━━━━━━━
+ Top

────────────────────
+ do act

-- "top act"

━━━━━━━━━━━━━━━━━━━━
+ Mid is Top

━━━━━━━━━━━━━━━━━━━━
+ Low is Mid

────────────────────
+ do act

-- "low act"

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Proto for
+ foo Mid
```

```output
low act
low act
top act
top act
```

### Delegated function takes a number of another type than the interface gives

```yumi
. sink = Sink()
sink <- 1
. writable = sink as Writable'I32
writable <- 2

━━━━━━━━━━━━━━━━━━━━
+ Sink

as Writable'I32 for
_ channel = Channel'Float()

━━━━━━━━━━━━━━━━━━━━
+ Writable'Message is Interface

────────────────────
+ this <- item Message

━━━━━━━━━━━━━━━━━━━━
+ Channel'Message
    is Writable'Message

────────────────────
+ do this <- item Message

-- item
```

```output
1
2
```

### Writes chained through a delegate give the receiver written

```yumi
. sink = Sink()
sink <- "a" <- "b"
. writable = sink as Writable'String
writable <- "c" <- "d"

━━━━━━━━━━━━━━━━━━━━
+ Wrapped is (text String)

────────────────────
+ from other String

Wrapped(other)

────────────────────
+ to String

"<$text>"

━━━━━━━━━━━━━━━━━━━━
+ Sink

as Writable'String for
_ channel = Channel'Wrapped()

━━━━━━━━━━━━━━━━━━━━
+ Writable'Message is Interface

────────────────────
+ this <- item Message

━━━━━━━━━━━━━━━━━━━━
+ Channel'Message
    is Writable'Message

────────────────────
+ do this <- item Message

-- item
```

```output
<a>
<b>
<c>
<d>
```

### Function that returns an optional of a class answers an optional of an interface

```yumi
. child = Child()
. holder Holder = child
-- holder.find(1) == null
. found = holder.find(2)
if found
    -- found.tag
-- describe(holder, 2)
-- describe(holder, 1)

━━━━━━━━━━━━━━━━━━━━
+ Owner is Interface

────────────────────
+ get tag String

━━━━━━━━━━━━━━━━━━━━
+ Holder is Interface

────────────────────
+ find Owner?
    - key Int

━━━━━━━━━━━━━━━━━━━━
+ Parent
    is Owner

────────────────────
+ get tag

"parent"

━━━━━━━━━━━━━━━━━━━━
+ Child
    is Holder

────────────────────
+ find Parent?
    - key Int

|? key == 2
    Parent()
??
    null

━━━━━━━━━━━━━━━━━━━━
+ do describe String
    - holder Holder
    - key Int

. found = holder.find(key)
if found
    = found.tag
= "none"
```

```output
true
parent
parent
none
```

### Optional of a tuple made an optional of an interface

```yumi
. point Point? = Point(1, 2)
. shown Shown? = point
if shown
    -- shown.text
. none Point? = null
. empty Shown? = none
-- empty == null
. list [Shown?] = [point, none]
for item in list
    if item
        -- item.text
    else
        -- "nothing"
-- first(point)
-- first(none)

━━━━━━━━━━━━━━━━━━━━
+ Shown is Interface

────────────────────
+ get text String

━━━━━━━━━━━━━━━━━━━━
+ Point is Tuple
    is Shown

+ x Int
+ y Int

────────────────────
+ get text

"$x,$y"

━━━━━━━━━━━━━━━━━━━━
+ do first String
    - shown Shown?

= if shown then shown.text else "none"
```

```output
1,2
true
1,2
nothing
1,2
none
```

### Optional of a class made an optional of an interface

```yumi
. parent Parent? = Parent()
. owner Owner? = parent
-- (owner!).tag
. none Parent? = null
. empty Owner? = none
-- empty == null
-- show(parent)
-- show(none)

━━━━━━━━━━━━━━━━━━━━
+ Owner is Interface

────────────────────
+ get tag String

━━━━━━━━━━━━━━━━━━━━
+ Parent
    is Owner

────────────────────
+ get tag

"parent"

━━━━━━━━━━━━━━━━━━━━
+ do show String
    - owner Owner?

if owner
    = owner.tag
= "none"
```

```output
parent
true
parent
none
```

### Stack trace through a delegated function of an interface

```yumi crash
. sink = Sink()
sink <- "hello"
. writable = sink as Writable'String
writable <- "world"
writable <- ""
-- "never"

━━━━━━━━━━━━━━━━━━━━
+ Sink

as Writable'String for
_ channel = Channel'String()

━━━━━━━━━━━━━━━━━━━━
+ Writable'Message is Interface

────────────────────
+ this <- item Message

━━━━━━━━━━━━━━━━━━━━
+ Channel'Message
    is Writable'Message

────────────────────
+ do this <- item Message

?! Panic("empty item")
    if "$item" == ""
-- item
```

```stderr
Unhandled panic: empty item
<no file:28>

Task 1

Stack trace:
  0  __source__:Channel:<-write no file:28:1
  1  __source__:<-write(wrapped) no file:26:1
  2  __source__:main no file:6:1
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### Stack trace through a delegated function that converts its argument

```yumi crash
. sink = Sink()
sink <- "hello"
. writable = sink as Writable'String
writable <- "world"
writable <- ""
-- "never"

━━━━━━━━━━━━━━━━━━━━
+ Wrapped is (text String)

────────────────────
+ from other String

Wrapped(other)

────────────────────
+ to String

text

━━━━━━━━━━━━━━━━━━━━
+ Sink

as Writable'String for
_ channel = Channel'Wrapped()

━━━━━━━━━━━━━━━━━━━━
+ Writable'Message is Interface

────────────────────
+ this <- item Message

━━━━━━━━━━━━━━━━━━━━
+ Channel'Message
    is Writable'Message

────────────────────
+ do this <- item Message

?! Panic("empty item")
    if "$item" == ""
-- item
```

```stderr
Unhandled panic: empty item
<no file:41>

Task 1

Stack trace:
  0  __source__:Channel:<-write no file:41:1
  1  __source__:<-write(wrapped) no file:32:1
  2  __source__:main no file:6:1
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### Delegate that a getter answers is read at each call

```yumi
. thing = Thing()
-- thing.act(1)
-- thing.act(2)
. proto = thing as Proto
-- proto.act(3)
-- proto.act(4)

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Proto for

────────────────────
+ do get foo Foo

-- "made foo"
= Foo()

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
made foo
foo 1
made foo
foo 2
made foo
foo 3
made foo
foo 4
```

### Stack trace of a panic in a getter that answers a delegate

```yumi crash
. thing = Thing()
-- thing.act(1)
. proto = thing as Proto
-- proto.act(2)
thing.broken = true
-- proto.act(3)
-- "never"

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ broken = false

as Proto for

────────────────────
+ do get foo Foo

?! Panic("no foo")
    if broken
= Foo()

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ act String
    - x Int

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ label = "foo"

────────────────────
+ do act String
    - x Int

= "$label $x"
```

```stderr
Unhandled panic: no foo
<no file:20>

Task 1

Stack trace:
  0  __source__:Thing:foo no file:20:1
  1  __source__:act(wrapped) no file:37:1
  2  __source__:main no file:7:4
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### Delegate that a getter answers is the derived class's

```yumi
. thing Thing = Loud()
-- thing.act(1)
. proto = thing as Proto
-- proto.act(2)
. plain = Thing()
-- plain.act(3)
. other = plain as Proto
-- other.act(4)

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Proto for

────────────────────
+ get foo Foo

Foo()

━━━━━━━━━━━━━━━━━━━━
+ Loud is Thing

────────────────────
+ get foo

Foo("loud")

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
loud 1
loud 2
foo 3
foo 4
```

### Delegate of a delegate that a getter answers

```yumi
. outer = Outer()
-- outer.act(1)
. proto = outer as Proto
-- proto.act(2)

━━━━━━━━━━━━━━━━━━━━
+ Outer

as Proto for

────────────────────
+ do get inner Inner

-- "made inner"
= Inner()

━━━━━━━━━━━━━━━━━━━━
+ Inner

as Proto for
+ foo = Foo("inner")

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
made inner
inner 1
made inner
inner 2
```

### Tuple that delegates to a field with no value written

```yumi
. thing = Thing()
-- thing.foo.act(1)
. other = Thing(Foo("given"))
-- other.act(2)
-- thing.act(3)
-- thing.count
. binding = other.act(...)
-- binding(4)
. proto = other as Proto
. second = proto.act(...)
-- second(5)

━━━━━━━━━━━━━━━━━━━━
+ Thing is Tuple

as Proto for
+ foo Foo
+ count Int = 7

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
foo 1
given 2
foo 3
7
given 4
given 5
```

### Binding of a function a delegate answers reads the delegate when called

```yumi
. thing = Thing()
. binding = thing.act(...)
-- binding(1)
thing.foo.label = "changed"
-- binding(2)
-- thing.act(3)

━━━━━━━━━━━━━━━━━━━━
+ Thing

as Proto for
+ foo = Foo()

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
foo 1
changed 2
changed 3
```

### == of two values of an interface compares a class by identity, and != negates it.

```yumi
. thing = Thing()
. a = thing as Proto
. b = Thing() as Proto
-- a == a
-- a == b
-- a != b

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Proto
```

```output
true
false
true
```

### A panic in a type's own == passes through the equality function a witness table holds, which a trace names with no file.

```yumi crash
. m = Measured(7) as View
. n = Measured(7) as View
-- m == n
panicky(m, n)

━━━━━━━━━━━━━━━━━━━━
+ View is Interface

━━━━━━━━━━━━━━━━━━━━
+ Measured is (value I32)
    is View

────────────────────
+ do this == other Measured

if value == 7
    ?! Panic("boom")
= value == other.value

━━━━━━━━━━━━━━━━━━━━
+ do panicky
    - left View
    - right View

-- left == right
```

```stderr
Unhandled panic: boom
<no file:18>

Task 1

Stack trace:
  0  __source__:Measured:== no file:18:5
  1  __source__:InterfaceEquality:Class:__source__:Measured:Class:__source__:View no file
  2  __source__:main no file:4:4
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### == of two values of an interface whose types differ panics where nothing compares for change.

```yumi crash
. a = Packet'I32(1, 2) as View
. b = Packet'I64(1i64, 2i64) as View
-- "before"
check(a, b)

━━━━━━━━━━━━━━━━━━━━
+ View is Interface

━━━━━━━━━━━━━━━━━━━━
+ Packet'T is (first T, second T)
    is View

━━━━━━━━━━━━━━━━━━━━
+ do check
    - left View
    - right View

-- left == right
```

```stderr
Unhandled panic: Unsupported interface equality: concrete types differ
<no file:19>

Task 1

Stack trace:
  0  __source__:check no file:19:4
  1  __source__:main no file:5:1
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

### Optionals of an interface are equal when both are empty or both hold equal values.

```yumi
. a View? = Packet(1, 2) as View
. b View? = null
. c View? = Packet(1, 2) as View
-- a == b
-- a == c
-- b == b
-- a != c
. d = Packet(1, 2) as View
-- d == a
-- a == d

━━━━━━━━━━━━━━━━━━━━
+ View is Interface

━━━━━━━━━━━━━━━━━━━━
+ Packet is (first I32, second I32)
    is View
```

```output
false
true
true
false
true
true
```

### The equality function of a generic tuple names its type's parameters.

```yumi crash
. a = Packet'I32(7, 1) as View
. b = Packet'I32(7, 1) as View
-- a == b

━━━━━━━━━━━━━━━━━━━━
+ View is Interface

━━━━━━━━━━━━━━━━━━━━
+ Packet'T is (first T, second T)
    is View

────────────────────
+ do this == other Packet'T

?! Panic("generic boom")
```

```stderr
Unhandled panic: generic boom
<no file:16>

Task 1

Stack trace:
  0  __source__:Packet:== no file:16:1
  1  __source__:InterfaceEquality:Class:__source__:Packet(Class:core/number:I32):Class:__source__:View no file
  2  __source__:main no file:4:4
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### A field of an interface is compared through its own equality function.

```yumi crash
. inner = Measured(7) as View
. a = Nested(inner, 1) as View
. b = Nested(Measured(7) as View, 1) as View
-- a == b

━━━━━━━━━━━━━━━━━━━━
+ View is Interface

━━━━━━━━━━━━━━━━━━━━
+ Measured is (value I32)
    is View

────────────────────
+ do this == other Measured

?! Panic("inner boom")

━━━━━━━━━━━━━━━━━━━━
+ Nested is (inner View, marker I32)
    is View
```

```stderr
Unhandled panic: inner boom
<no file:17>

Task 1

Stack trace:
  0  __source__:Measured:== no file:17:1
  1  __source__:InterfaceEquality:Class:__source__:Measured:Class:__source__:View no file
  2  __source__:InterfaceEquality:Class:__source__:Nested:Class:__source__:View no file
  3  __source__:main no file:5:4
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### A class compared with a value of an interface is unequal; the other way around it panics.

```yumi crash
. a = Thing() as View
. t = Thing()
-- t == a
-- a == t

━━━━━━━━━━━━━━━━━━━━
+ View is Interface

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is View
```

```stderr
Unhandled panic: Unsupported interface equality with Thing
<no file:5>

Task 1

Stack trace:
  0  __source__:main no file:5:4
  1  __source__:init.__source__ no file
  2  __source__:brix.main no file
```

### A number class's own == decides equality through an interface.

```yumi
. a = Counter(1) as View
. b = Counter(2) as View
-- a == b
-- a != b
-- a == (Counter(1) as View)

━━━━━━━━━━━━━━━━━━━━
+ View is Interface

━━━━━━━━━━━━━━━━━━━━
+ Counter is I32
    is View

────────────────────
+ do this == other Counter

-- "compare"
= (this as I32) == (other as I32)
```

```output
compare
false
compare
true
compare
true
```

### A tuple holding a value of an interface compares that field by its type's equality.

```yumi
. a = Pair(1, Label("x") as View) as View
. b = Pair(1, Label("x") as View) as View
. c = Pair(1, Label("y") as View) as View
-- a == b
-- a == c

━━━━━━━━━━━━━━━━━━━━
+ View is Interface

━━━━━━━━━━━━━━━━━━━━
+ Label is (text String)
    is View

━━━━━━━━━━━━━━━━━━━━
+ Pair is (count I32, inner View)
    is View
```

```output
true
false
```

### An interface compared with another type as the right side of a short circuit panics where it is compared.

```yumi crash
. a = Leaf(1) as View
. n = 2
. r = n < 1 || a == n
-- r

━━━━━━━━━━━━━━━━━━━━
+ View is Interface

━━━━━━━━━━━━━━━━━━━━
+ Leaf is (value I32)
    is View
```

```stderr
Unhandled panic: Unsupported interface equality with I64
<no file:4>

Task 1

Stack trace:
  0  __source__:main no file:4:16
  1  __source__:init.__source__ no file
  2  __source__:brix.main no file
```

### Values of an interface whose type is a function type are equal when they are one object.

```yumi
. g = nums()
. a = g as Iterable'Int
. b = g as Iterable'Int
-- a == b
-- a == (nums() as Iterable'Int)
-- ([1, 2] as Iterable'Int) == ([1, 2] as Iterable'Int)

━━━━━━━━━━━━━━━━━━━━
+ do nums

<= 1
<= 2
```

```output
true
false
false
```

### A panic inside an equality function is headed by the line of the comparison.

```yumi crash
. a = Nested(Leaf(1) as View, 1) as View
. b = Nested(Thing() as View, 1) as View
-- "go"
-- a == b

━━━━━━━━━━━━━━━━━━━━
+ View is Interface

━━━━━━━━━━━━━━━━━━━━
+ Leaf is (value I32)
    is View

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is View

━━━━━━━━━━━━━━━━━━━━
+ Nested is (inner View, marker I32)
    is View
```

```stderr
Unhandled panic: Unsupported interface equality: concrete types differ
<no file:5>

Task 1

Stack trace:
  0  __source__:InterfaceEquality:Class:__source__:Nested:Class:__source__:View no file
  1  __source__:main no file:5:4
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

### A closure is named Closure by type-of and by the panic of an interface compared with it.

```yumi crash
. a = Leaf(1) as View
. g = :: n Int => n + 1
-- "${type-of g}"
-- a == g

━━━━━━━━━━━━━━━━━━━━
+ View is Interface

━━━━━━━━━━━━━━━━━━━━
+ Leaf is (value I32)
    is View
```

```stderr
Unhandled panic: Unsupported interface equality with Closure
<no file:5>

Task 1

Stack trace:
  0  __source__:main no file:5:4
  1  __source__:init.__source__ no file
  2  __source__:brix.main no file
```

### Both operands of == on interface values stay alive while a user == allocates and collects.

```yumi
/BxGC/BxGC.h

. left = Pair(GCNode(17), GCNode(29)) as View
. same = Pair(GCNode(17), GCNode(29)) as View
. different = Pair(GCNode(17), GCNode(30)) as View
-- left == same
-- left != same
-- left == different
-- left != different

━━━━━━━━━━━━━━━━━━━━
+ View is Interface

━━━━━━━━━━━━━━━━━━━━
+ Pair is (left GCNode, right GCNode)
    is View

────────────────────
+ do this == other Pair

. scratch [GCNode]
for index in 0 .. < 32
    scratch += GCNode(index)
C:brixCollect()
. valid = C:brixGCVerifyObject(left as Pointer) && C:brixGCVerifyObject(right as Pointer)
valid = valid && C:brixGCVerifyObject(other.left as Pointer)
valid = valid && C:brixGCVerifyObject(other.right as Pointer)
= valid && left.value == other.left.value && right.value == other.right.value

━━━━━━━━━━━━━━━━━━━━
+ GCNode

+ value = 0

────────────────────
+ This
    - v Int

value = v
```

```output
true
false
false
true
```

### An interface's function written with a type parameter of its own is checked only for the calls that name it.

```yumi
. box = Tally()
send(box)
-- box.total

━━━━━━━━━━━━━━━━━━━━
+ do send
    - sink Sink

sink <- 2.5

━━━━━━━━━━━━━━━━━━━━
+ Sink is Interface

────────────────────
+ do this <- count Int

put(count)

────────────────────
+ do this <- value ''Value

put(1)

────────────────────
+ put ?! Error
    - count Size

━━━━━━━━━━━━━━━━━━━━
+ Tally
    is Sink

+ total Size = 0

────────────────────
+ do put
    - count Size

total += count
```

```output
1
```

### A function that answers an interface's with feeds gives a generator, which a call through the interface collects into the list the interface's function answers.

```yumi
. maker Shows'Record = Maker()
for part in maker.parts(Record("box"))
    match part
        | .text(text)
            -- "text ${text}"
        | .count(count)
            -- "count ${count}"

━━━━━━━━━━━━━━━━━━━━
+ Part is Union

| text String
| count I32

━━━━━━━━━━━━━━━━━━━━
+ Record is Tuple

+ name String

━━━━━━━━━━━━━━━━━━━━
+ Shows'Item is Interface

────────────────────
+ do parts [Part]
    - item Item

━━━━━━━━━━━━━━━━━━━━
+ Maker is Tuple
    is Shows'Record

────────────────────
+ parts
    - record Record

* Part.text("Add ")
* Part.text(record.name)
* Part.count(2)
```

```output
text Add 
text box
count 2
```

### What a type's function answers is made the case of the union the interface's function answers, for a call through the interface.

```yumi
. maker Shows = Maker("box")
match maker.value()
    | .text(text)
        -- "text ${text}"
    | .names(names)
        -- "names ${names.length} ${names[0]}"
    | .count(count)
        -- "count ${count}"

━━━━━━━━━━━━━━━━━━━━
+ Value is Union

| text String
| names [String]
| count I32

━━━━━━━━━━━━━━━━━━━━
+ Shows is Interface

────────────────────
+ do value Value

━━━━━━━━━━━━━━━━━━━━
+ Maker is Tuple
    is Shows

+ name String

────────────────────
+ value

[name, "two"]
```

```output
names 2 box
```

### A type's function that raises the errors the interface's function raises answers a value of another type, converted for a call through the interface, and its error passes through as it is.

```yumi
for name in ["box", ""]
    . maker Shows = Maker(name)
    match catch? maker.value() ?? Value.text("failed")
        | .text(text)
            -- "text ${text}"
        | .names(names)
            -- "names ${names.length} ${names[0]}"

━━━━━━━━━━━━━━━━━━━━
+ Value is Union

| text String
| names [String]

━━━━━━━━━━━━━━━━━━━━
+ Shows is Interface

────────────────────
+ do value Value ?! Error

━━━━━━━━━━━━━━━━━━━━
+ Maker is Tuple
    is Shows

+ name String

────────────────────
+ value

[checked(name), "two"]

━━━━━━━━━━━━━━━━━━━━
+ do checked String ?! Error
    - name String

if name.length == 0
    ?! Error("empty")
= name
```

```output
names 2 box
text failed
```

### Two values of an interface are equal where they hold one type whose fields are, an optional field among them compared by the `==` its type declares.

```yumi
. one Shows = Stored(Record(Spot(1, 2), 3))
. two Shows = Stored(Record(Spot(1, 2), 3))
. three Shows = Stored(Record(null, 3))
-- one == two
-- one == three

━━━━━━━━━━━━━━━━━━━━
+ Spot is Tuple

+ x I32
+ y I32

────────────────────
+ this == other Spot

x == other.x && y == other.y

━━━━━━━━━━━━━━━━━━━━
+ Record is Tuple

+ pivot Spot?
+ size I32

━━━━━━━━━━━━━━━━━━━━
+ Shows is Interface

────────────────────
+ get size I32

━━━━━━━━━━━━━━━━━━━━
+ Stored is Tuple
    is Shows

+ record Record

────────────────────
+ get size I32

record.size
```

```output
true
false
```

### A method that takes more arguments than the interface's, each with a default, implements it, and its defaults are evaluated at each call.

```yumi
. w = Worker()
run(w)
run(w)

━━━━━━━━━━━━━━━━━━━━
+ do run
    - d Doer

-- "got ${d.act(3)}"
-- "pure ${d.pure(4)}"

━━━━━━━━━━━━━━━━━━━━
+ Doer is Interface

────────────────────
+ do act Int
    - n Int

────────────────────
+ pure Int
    - n Int

━━━━━━━━━━━━━━━━━━━━
+ Worker
    is Doer

+ count = 0

────────────────────
+ do act Int
    - n Int
    - scale = 10
    - extra = next()
    - flag = false

count += 1
-- "act n=$n scale=$scale extra=$extra flag=$flag count=$count"
= n * scale + extra

────────────────────
+ pure Int
    - n Int
    - plus Int? = null

n + (plus ?? 100)

━━━━━━━━━━━━━━━━━━━━
+ do next Int

-- "next"
= 7
```

```output
next
act n=3 scale=10 extra=7 flag=false count=1
got 37
pure 104
next
act n=3 scale=10 extra=7 flag=false count=2
got 37
pure 104
```

### A loop over a class that delegates two iterables reads the last, converting its items.

```yumi
. inbox = Inbox()
inbox.feed()
for message in inbox
    -- message
-- "done"

━━━━━━━━━━━━━━━━━━━━
+ Inbox

as Iterable'Int for
_ counts Int<>
as Iterable'Float for
_ notes Note<>

────────────────────
+ do feed

do new
    notes <- Note(1)
    notes <- Note(2)
    notes.close()

━━━━━━━━━━━━━━━━━━━━
+ Note is Tuple

+ value Int

────────────────────
+ to Float

value * 1.5
```

```output
1.5
3
done
```

### A class that leaves out a getter its interface declares without a body and with a result does not compile.

```yumi crash
. source Source = Lines()
-- source.revision

━━━━━━━━━━━━━━━━━━━━
+ Source is Interface

────────────────────
+ get revision U64

━━━━━━━━━━━━━━━━━━━━
+ Lines
    is Source
```

```output
Exception: Lines does not implement "revision", which Source declares without a body; declare "revision" in Lines
<no file:12>
    + Lines
    ^^^^^^
Interface:
<no file:9>
    + get revision U64
```

### A class that leaves out a function its interface declares without a body and with a result does not compile once a value of it is made a value of the interface, though nothing calls the function.

```yumi compile-error
. shape Shape = Blob()
-- "made"

━━━━━━━━━━━━━━━━━━━━
+ Shape is Interface

────────────────────
+ area Float

━━━━━━━━━━━━━━━━━━━━
+ Blob
    is Shape
```

```output
Exception: Blob does not implement "area", which Shape declares without a body; declare "area" in Blob
<no file:12>
    + Blob
    ^^^^^
Interface:
<no file:9>
    + area Float
```

### A class may leave a function its interface requires to the classes derived from it, as long as no value known by the class alone is made a value of the interface.

```yumi
. shape Shape = Square()
-- shape.area()

━━━━━━━━━━━━━━━━━━━━
+ Shape is Interface

────────────────────
+ area Float

━━━━━━━━━━━━━━━━━━━━
+ Base
    is Shape

━━━━━━━━━━━━━━━━━━━━
+ Square is Base

────────────────────
+ area Float

4.0
```

```output
4
```

### An interface stores nothing, so a property it declares is an error that names the getter to declare instead, one that each type that is the interface answers.

```yumi compile-error
. shape Shape = Square()
-- shape.name

━━━━━━━━━━━━━━━━━━━━
+ Shape is Interface

+ name String

━━━━━━━━━━━━━━━━━━━━
+ Square
    is Shape

+ name = "square"
```

```output
Exception: Shape is an interface, so it cannot store "name"; declare a getter for the types that are Shape to answer, as in "+ get name String"
<no file:8>
    + name String
    ^^^^^^^^^^^^^
```

### An interface stores nothing, so a property it declares with a value is an error that names the getter to declare, which gives the value to each type that is the interface and has no other.

```yumi compile-error
. shape Shape = Square()
-- shape.name

━━━━━━━━━━━━━━━━━━━━
+ Shape is Interface

+ name = "shape"

━━━━━━━━━━━━━━━━━━━━
+ Square
    is Shape
```

```output
Exception: Shape is an interface, so it cannot store "name"; declare a getter that gives the value, with "=>" in place of "="
<no file:8>
    + name = "shape"
    ^^^^^^^^^^^^^^^^
```

### An interface cannot hold a state machine, which keeps its cases.

```yumi compile-error
. t = Door()
-- t.phase

━━━━━━━━━━━━━━━━━━━━
+ Gate is Interface

────────────────────
+ phase ~>

~> closed

━━━━━━━━━━━━━━━━━━━━
+ Door
    is Gate
```

```output
Exception: Gate is an interface, so it cannot hold a state machine; declare the machine in the classes that are Gate
<no file:11>
    ~> closed
    ^^^^^^^^^
```

```yumi
. sub = SubHolder(Named("sub"))
-- sub.target.name()
. box = HolderBox(Named("box"))
-- box.held.target.name()

━━━━━━━━━━━━━━━━━━━━
+ Naming is Interface

────────────────────
+ name String

━━━━━━━━━━━━━━━━━━━━
+ Named
    is Naming

+ text String

────────────────────
+ This
    - text

────────────────────
+ name

text

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ target Naming

────────────────────
+ This
    - target

━━━━━━━━━━━━━━━━━━━━
+ SubHolder is Holder

────────────────────
+ This
    - named Named

super(named)

━━━━━━━━━━━━━━━━━━━━
+ HolderBox

+ held Holder

────────────────────
+ This
    - named Named

held = Holder(named)
```

```output
sub
box
```

### An interface a class declares must name a type

```yumi compile-error
. thing = Thing()
-- thing.name

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is NoSuchInterface

+ name = "thing"
```

```output
Exception: Type "NoSuchInterface" not found
<no file:7>
        is NoSuchInterface
           ^^^^^^^^^^^^^^^
```

### A class matched against an interface no class implements compiles, and the arm is never taken.

```yumi
. node = Node()
. found = node.find'Opener()
-- if found then "found" else "none"
-- node is Opener

━━━━━━━━━━━━━━━━━━━━
+ Opener is Interface

────────────────────
+ do open

━━━━━━━━━━━━━━━━━━━━
+ Node

────────────────────
+ do find'Target

. items [Node] = [this]
for item in items
    match? item
        | target Target
            = target
```

```output
none
false
```

### A stored property answers an interface getter that returns an optional of the property's type, and a class that leaves the getter to its default answers null.

```yumi
. plain Shape = Plain()
. owned Shape = Owned()
-- plain.revision == null
-- owned.revision!.value

━━━━━━━━━━━━━━━━━━━━
+ Rev

+ value Int = 7

━━━━━━━━━━━━━━━━━━━━
+ Shape is Interface

────────────────────
+ get revision Rev?

null

━━━━━━━━━━━━━━━━━━━━
+ Plain
    is Shape

━━━━━━━━━━━━━━━━━━━━
+ Owned
    is Shape

+ revision := Rev()
```

```output
true
7
```

### A step function an interface declares with a result and no steps has no body, so a class made the interface must declare it.

```yumi compile-error
. walker Walker = Thing()
-- walker.step()

━━━━━━━━━━━━━━━━━━━━
+ Walker is Interface

────────────────────
+ do step Int

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Walker
```

```output
Exception: Thing does not implement "step", which Walker declares without a body; declare "step" in Thing
<no file:12>
    + Thing
    ^^^^^^
Interface:
<no file:9>
    + do step Int
```

### A missing operator an interface declares is named as the interface writes it.

```yumi compile-error
. source Source = Thing()
-- "made"

━━━━━━━━━━━━━━━━━━━━
+ Source is Interface

────────────────────
+ <- this Int?

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Source
```

```output
Exception: Thing does not implement "<- this", which Source declares without a body; declare "<- this" in Thing
<no file:12>
    + Thing
    ^^^^^^
Interface:
<no file:9>
    + <- this Int?
```

### An assignment to a property of a class that leaves an interface's getter and setter to their defaults calls the default setter.

```yumi
. holder = Holder()
holder.label = "a"
-- holder.label

━━━━━━━━━━━━━━━━━━━━
+ Host is Interface

────────────────────
+ get label String

"default"

────────────────────
+ do label = value String

-- "set $value"

━━━━━━━━━━━━━━━━━━━━
+ Holder
    is Host
```

```output
set a
default
```

### A property a subclass stores in place of the default getter and setter of an interface its base class declares answers for them through a value of the base class.

```yumi
. base Base = Child()
-- base.label
base.label = "stored"
-- base.label

━━━━━━━━━━━━━━━━━━━━
+ Host is Interface

────────────────────
+ get label String

"default"

────────────────────
+ do label = value String

-- "default $value"

━━━━━━━━━━━━━━━━━━━━
+ Base
    is Host

━━━━━━━━━━━━━━━━━━━━
+ Child is Base

+ label = "child"
```

```output
child
stored
```
