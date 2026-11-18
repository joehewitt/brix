# Inheritance

```yumi
. bar = Bar()
. thing Foo = bar
-- thing.a

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ a = 0

━━━━━━━━━━━━━━━━━━━━
+ Bar is Foo
```

```output
0
```

```yumi
. bar = Bar()
-- bar.b
-- bar.a

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ a = 42

━━━━━━━━━━━━━━━━━━━━
+ Bar is Foo

+ b = 99
```

```output
99
42
```

### Overriding base property

```yumi
. bar = Bar()
-- bar.a
-- show(bar)

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ a = 42

━━━━━━━━━━━━━━━━━━━━
+ Bar is Foo

+ a = 99

━━━━━━━━━━━━━━━━━━━━
+ show
    - foo Foo

foo.a
```

```output
99
99
```

### Overriding base property with different type should cast to base type

```yumi
. bar = Bar()
-- bar.a
-- show(bar)

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ a = 42.0

━━━━━━━━━━━━━━━━━━━━
+ Bar is Foo

+ a = 99

━━━━━━━━━━━━━━━━━━━━
+ show
    - foo Foo

foo.a
```

```output
99
99
```

```yumi
. bar = Bar()
bar.bar()

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ a = 42

━━━━━━━━━━━━━━━━━━━━
+ Bar is Foo

+ b = 99

────────────────────
+ do bar

-- b
-- a
```

```output
99
42
```

```yumi
. bar = Bar()
bar.bar()
bar.foo()

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ a = 42

────────────────────
+ do foo

-- "Foo.a = $a"

━━━━━━━━━━━━━━━━━━━━
+ Bar is Foo

+ b = 99

────────────────────
+ do bar

-- "Bar.b = $b"
-- "Bar.a = $a"
```

```output
Bar.b = 99
Bar.a = 42
Foo.a = 42
```

```yumi
. bar = Bar()
act(bar)

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ a = 42

────────────────────
+ do foo

-- "Foo.a = $a"

━━━━━━━━━━━━━━━━━━━━
+ Bar is Foo

+ b = 99

────────────────────
+ do bar

-- "Bar.b = $b"
-- "Bar.a = $a"

━━━━━━━━━━━━━━━━━━━━
+ do act
    - thing Foo

thing.foo()
```

```output
Foo.a = 42
```

```yumi
. bar = Bar()
bar.f1()

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ do f1

-- "Foo 1"

━━━━━━━━━━━━━━━━━━━━
+ Bar is Foo

────────────────────
+ do f1

-- "Bar 1"
```

```output
Bar 1
```

```yumi
. bar = Bar()

` Downcast still calls method on subclass
. foo Foo = bar
foo.f1()

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ do f1

-- "Foo 1"


━━━━━━━━━━━━━━━━━━━━
+ Bar is Foo

────────────────────
+ do f1

-- "Bar 1"
```

```output
Bar 1
```

```yumi
. bar = Bar()
. foo = Foo()

bar.f1()
foo.f2()

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ do f1

-- "Foo 1"

────────────────────
+ do f2

-- "Foo 2"

━━━━━━━━━━━━━━━━━━━━
+ Bar is Foo

────────────────────
+ do f1

-- "Bar 1"
```

```output
Bar 1
Foo 2
```

```yumi
. bar = Bar()

` Downcast still calls method on subclass
. foo Foo = bar
foo.f2()

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ do f1

-- "Foo 1"

────────────────────
+ do f2

-- "Foo 2"

━━━━━━━━━━━━━━━━━━━━
+ Bar is Foo

────────────────────
+ do f1

-- "Bar 1"
```

```output
Foo 2
```

```yumi
. bar = Bar()

` Downcast in argument still calls method on subclass
. foo Foo = bar
act(foo)

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ do f1

-- "Foo 1"

━━━━━━━━━━━━━━━━━━━━
+ Bar is Foo

────────────────────
+ do f1

-- "Bar 1"

━━━━━━━━━━━━━━━━━━━━
+ do act
    - foo Foo

foo.f1()
```

```output
Bar 1
```

```yumi
. bar = Bar()

` Downcast still calls method on subclass
. foo Foo = bar
foo.f1(5.5)
foo.f1(3)

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ do f1'T
    - a T

-- "Foo 1 $a"

━━━━━━━━━━━━━━━━━━━━
+ Bar is Foo

────────────────────
+ do f1'T
    - a T

-- "Bar 1 $a"
```

```output
Bar 1 5.5
Bar 1 3
```

### This test verifies a fix I made to the layout of class structs which puts the lazy flag in a predictable place for all subclasses.

```yumi
. foo = Foo()
-- foo.a
. bar = Bar()
-- bar.a

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ foo = 42
+ a => b
+ b := foo

━━━━━━━━━━━━━━━━━━━━
+ Bar is Foo

` This is here just to shift the properties down in the struct
` which, in the old code, would move the lazy flag lower in the subclass struct only
+ bar = 1
```

```output
42
42
```

### Direct dispatch must account for declared subclasses that are instantiated later

```yumi
. base = Base()
act(base)

. child = Child()
act(child)

━━━━━━━━━━━━━━━━━━━━
+ Base

────────────────────
+ do name

-- "Base"

━━━━━━━━━━━━━━━━━━━━
+ Child is Base

────────────────────
+ do name

-- "Child"

━━━━━━━━━━━━━━━━━━━━
+ do act
    - thing Base

thing.name()
```

```output
Base
Child
```

### Direct dispatch must account for generic declared subclasses that are instantiated later

```yumi
. base = Base'Int()
act(base)

. child = Child'Int()
act(child)

━━━━━━━━━━━━━━━━━━━━
+ Base'T

────────────────────
+ do name

-- "Base"

━━━━━━━━━━━━━━━━━━━━
+ Child'T is Base'T

────────────────────
+ do name

-- "Child"

━━━━━━━━━━━━━━━━━━━━
+ do act
    - thing Base'Int

thing.name()
```

```output
Base
Child
```

### Direct dispatch must account for generic subclasses of non-generic bases

```yumi
. base = Base()
act(base)

. child = Child'Int()
act(child)

━━━━━━━━━━━━━━━━━━━━
+ Base

────────────────────
+ do name

-- "Base"

━━━━━━━━━━━━━━━━━━━━
+ Child'T is Base

────────────────────
+ do name

-- "Child"

━━━━━━━━━━━━━━━━━━━━
+ do act
    - thing Base

thing.name()
```

```output
Base
Child
```

### An inherited event cannot be redeclared as a property

```yumi crash
EventPropertyChild()

━━━━━━━━━━━━━━━━━━━━
+ EventBase

+ event changed Int

━━━━━━━━━━━━━━━━━━━━
+ EventPropertyChild is EventBase

+ changed = 1
```

```output
Exception: Inherited event "changed" cannot be redeclared as a property
<no file:12>
    + changed = 1
    ^^^^^^^^^^^^^
```

### An inherited property cannot be redeclared as a event

```yumi crash
PropertyEventChild()

━━━━━━━━━━━━━━━━━━━━
+ PropertyBase

+ changed = 1

━━━━━━━━━━━━━━━━━━━━
+ PropertyEventChild is PropertyBase

+ event changed Int
```

```output
Exception: Inherited property "changed" cannot be redeclared as an event
<no file:12>
    + event changed Int
    ^^^^^^^^^^^^^^^^^^^
```

### An inherited event cannot be redeclared as a method

```yumi crash
EventMethodChild().run()

━━━━━━━━━━━━━━━━━━━━
+ EventMethodBase

+ event submit String

━━━━━━━━━━━━━━━━━━━━
+ EventMethodChild is EventMethodBase

────────────────────
+ do run

submit("x")

────────────────────
_ do submit
    - text String

-- text
```

```output
Exception: Inherited event "submit" cannot be redeclared as a method
<no file:18>
    _ do submit
    ^^^^^^^^^^
```

### An inherited method cannot be redeclared as an event

```yumi crash
MethodEventChild()

━━━━━━━━━━━━━━━━━━━━
+ MethodEventBase

────────────────────
+ do changed
    - value Int

━━━━━━━━━━━━━━━━━━━━
+ MethodEventChild is MethodEventBase

+ event changed Int
```

```output
Exception: Inherited method "changed" cannot be redeclared as an event
<no file:14>
    + event changed Int
    ^^^^^^^^^^^^^^^^^^^
```

### Lowered inherited subscriptions preserve base-before-derived lexical order

```yumi
. owner = OrderedEventChild()

for value on owner.changed
    -- "external $value"

owner.emit(9)

━━━━━━━━━━━━━━━━━━━━
+ OrderedEventBase

+ event changed Int

────────────────────
for value on changed

-- "base-1 $value"

────────────────────
for value on changed

-- "base-2 $value"

────────────────────
+ do emit
    - value Int

changed <- value

━━━━━━━━━━━━━━━━━━━━
+ OrderedEventChild is OrderedEventBase

────────────────────
for value on changed

-- "derived-1 $value"

────────────────────
for value on changed

-- "derived-2 $value"
```

```output
base-1 9
base-2 9
derived-1 9
derived-2 9
external 9
```

```yumi
. child = Child'I32()
child.target = Target'I32()
child.value = 42
child.run()

━━━━━━━━━━━━━━━━━━━━
+ Root

━━━━━━━━━━━━━━━━━━━━
+ Base is Root

────────────────────
+ do run

execute()

────────────────────
+ do execute

-- "base"

━━━━━━━━━━━━━━━━━━━━
+ Target'T

────────────────────
+ do accept
    - value T

-- value

━━━━━━━━━━━━━━━━━━━━
+ Child'T is Base

+ target Target'T
+ value T

────────────────────
+ do execute

target.accept(value)
```

```output
42
```

```yumi
. derived = pick(true)
. base = pick(false)
-- derived is Derived
-- base is Derived
-- base.name()
. fallback = if false then Derived() else Base()
-- fallback.name()

━━━━━━━━━━━━━━━━━━━━
+ Base

────────────────────
+ do name String

= "base"

━━━━━━━━━━━━━━━━━━━━
+ Derived is Base

────────────────────
+ do name String

= "derived"

━━━━━━━━━━━━━━━━━━━━
+ pick Base
    - derived Bool

if derived then Derived() else Base()
```

```output
true
false
base
base
```

### A subclass getter can read its base getter through super without dispatching back to itself.

```yumi
-- SuperReadChild().label
-- SuperReadChild().is-ready

━━━━━━━━━━━━━━━━━━━━
+ SuperReadBase

────────────────────
+ do get label String

= "base"

────────────────────
+ do get is-ready Bool

= true

━━━━━━━━━━━━━━━━━━━━
+ SuperReadChild is SuperReadBase

────────────────────
+ do get label

= "child of ${super.label}"

────────────────────
+ do get is-ready

= super.is-ready && false
```

```output
child of base
false
```

### This in an inherited method names the declaring class, the type of `this` there.

```yumi
. keyed = ThisKeyed()
. same = keyed.me()
-- same.count
-- keyed.with'Int(:: item => item.count + 1)

━━━━━━━━━━━━━━━━━━━━
+ ThisBase

+ count = 1

────────────────────
+ me This

this

────────────────────
+ with'Result
    - func () Result
        - This

func(this)

━━━━━━━━━━━━━━━━━━━━
+ ThisKeyed is ThisBase

+ extra = 2
```

```output
1
2
```

### A member only the subclass declares is not visible through an inherited This.

```yumi compile-error
. keyed = ThisKeyed()
-- keyed.me().extra

━━━━━━━━━━━━━━━━━━━━
+ ThisBase

────────────────────
+ me This

this

━━━━━━━━━━━━━━━━━━━━
+ ThisKeyed is ThisBase

+ extra = 2
```

```output
Exception: ThisBase has no property "extra"
<no file:3>
    -- keyed.me().extra
       ^^^^^^^^^^^^^^^^
```

### A method called in a constructor is the one of the constructor's own class, whether it is called by name, on `this`, or by another method the constructor calls.

```yumi
Child()
-- "--"
Base()

━━━━━━━━━━━━━━━━━━━━
+ Base

────────────────────
+ This

report()
this.report()
relay()

────────────────────
+ do relay

report()

────────────────────
+ do report

-- "base"

━━━━━━━━━━━━━━━━━━━━
+ Child is Base

────────────────────
+ This

super()
-- "child ctor"
report()
relay()

────────────────────
+ do report

-- "child"
```

```output
base
base
child
child ctor
child
child
--
base
base
base
```

### A constructor the compiler generates gives the base's properties their values before its own, and a method a base's constructor calls is the base's.

```yumi
Child()
-- "--"
Grand()

━━━━━━━━━━━━━━━━━━━━
+ Base

+ a = say("a")

────────────────────
+ This

report()
relay()

────────────────────
+ do relay

report()

────────────────────
+ do report

-- "base"

━━━━━━━━━━━━━━━━━━━━
+ Child is Base

+ b = say("b")

────────────────────
+ do report

-- "child"

━━━━━━━━━━━━━━━━━━━━
+ Grand is Child

+ c = say("c")

────────────────────
+ This

super()
-- "grand after"

────────────────────
+ do report

-- "grand"

━━━━━━━━━━━━━━━━━━━━
+ do say String
    - s String

-- "say $s"
= s
```

```output
say a
say b
child
child
--
say a
say b
say c
child
grand
grand after
```

### A setter a subclass declares again is called through a value held as the base.

```yumi
. b Base = Child()
b.value = 5
-- b.stored
. c Base = Base()
c.value = 5
-- c.stored

━━━━━━━━━━━━━━━━━━━━
+ Base

+ stored = 0

────────────────────
+ do value = v Int

stored = v

━━━━━━━━━━━━━━━━━━━━
+ Child is Base

────────────────────
+ do value = v Int

stored = v * 2
```

```output
10
5
```

### An operator a subclass declares again is called through a value held as the base.

```yumi
. a Base = Child()
. b Base = Base()
-- a + 1
-- b + 1
-- a == b
-- !a
-- !b

━━━━━━━━━━━━━━━━━━━━
+ Base

────────────────────
+ this + n Int

n + 100

────────────────────
+ this == other Base

true

────────────────────
+ !this

false

━━━━━━━━━━━━━━━━━━━━
+ Child is Base

────────────────────
+ this + n Int

n + 200

────────────────────
+ !this

true
```

```output
201
101
true
true
false
```

### A panic raised by a method a subclass declares again passes through a call made on the base.

```yumi crash
. b Base = Child()
run(b)
-- "after"

━━━━━━━━━━━━━━━━━━━━
+ do run
    - b Base

b.go()
-- "not here"

━━━━━━━━━━━━━━━━━━━━
+ Base

────────────────────
+ do go

-- "base"

━━━━━━━━━━━━━━━━━━━━
+ Child is Base

────────────────────
+ do go

?! Panic("child boom")
```

```stderr
Unhandled panic: child boom
<no file:27>

Task 1

Stack trace:
  0  __source__:Child:go no file:27:1
  1  __source__:run no file:10:1
  2  __source__:main no file:3:1
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### A panic raised by a getter a subclass declares again passes through a read made on the base, and the trace names the getter with its class.

```yumi crash
. b Base = Child()
show(b)
-- "after"

━━━━━━━━━━━━━━━━━━━━
+ do show
    - b Base

-- b.label
-- "not here"

━━━━━━━━━━━━━━━━━━━━
+ Base

────────────────────
+ get label

"base"

━━━━━━━━━━━━━━━━━━━━
+ Child is Base

────────────────────
+ get label

boom()

━━━━━━━━━━━━━━━━━━━━
+ do boom String

?! Panic("getter boom")
= "child"
```

```stderr
Unhandled panic: getter boom
<no file:32>

Task 1

Stack trace:
  0  __source__:boom no file:32:1
  1  __source__:Child:label no file:27:1
  2  __source__:show no file:10:4
  3  __source__:main no file:3:1
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### `super(…)` gives the base's constructor the object being made: the properties its arguments fill are stored again, with the values that read them.

```yumi
. c = Child(5)
-- c.n
-- c.double
-- c.other
-- c.extra

━━━━━━━━━━━━━━━━━━━━
+ Base

+ n = 1
+ double = n * 2
+ other = say(7)

────────────────────
+ This
    - n

-- "base body $n $double"

━━━━━━━━━━━━━━━━━━━━
+ Child is Base

+ extra = say(9)

────────────────────
+ This
    - k Int

super(k)
-- "child body $n $double"

━━━━━━━━━━━━━━━━━━━━
+ do say Int
    - v Int

-- "say $v"
= v
```

```output
say 7
say 9
base body 5 10
child body 5 10
5
10
7
9
```

### `super(…)` through three classes.

```yumi
. g = Grand()
-- g.a
-- g.b
-- g.c

━━━━━━━━━━━━━━━━━━━━
+ Base

+ a = 1

────────────────────
+ This
    - a

-- "base $a"

━━━━━━━━━━━━━━━━━━━━
+ Child is Base

+ b = 2

────────────────────
+ This
    - b

super(b + 10)
-- "child $a $b"

━━━━━━━━━━━━━━━━━━━━
+ Grand is Child

+ c = 3

────────────────────
+ This

super(20)
-- "grand $a $b $c"
```

```output
base 30
child 30 20
grand 30 20 3
30
20
3
```

### A getter reads the one it replaces through `super`.

```yumi
. c = Child()
-- c.label
-- c.both()

━━━━━━━━━━━━━━━━━━━━
+ Base

────────────────────
+ get label

"base"

────────────────────
+ name String

"base name"

━━━━━━━━━━━━━━━━━━━━
+ Child is Base

────────────────────
+ get label

"child+${super.label}"

────────────────────
+ name String

"child name"

────────────────────
+ both String

"${super.name()} / ${name()}"
```

```output
child+base
base name / child name
```

### A method makes another object of its class with `This(…)`.

```yumi
. c = Child(1)
-- c.n
. d = c.again()
-- d.n

━━━━━━━━━━━━━━━━━━━━
+ Base

+ n = 0

────────────────────
+ This
    - n

━━━━━━━━━━━━━━━━━━━━
+ Child is Base

────────────────────
+ This
    - k Int

super(k + 1)

────────────────────
+ again Child

This(n * 10)
```

```output
2
21
```

### An object is tested for its class and for a class derived from it.

```yumi
. b Base = Child()
-- b is Base
-- b is Child
-- b is Other
-- b is not Child
-- b is not Other

━━━━━━━━━━━━━━━━━━━━
+ Base

+ n = 1

━━━━━━━━━━━━━━━━━━━━
+ Child is Base

+ m = 7

━━━━━━━━━━━━━━━━━━━━
+ Other is Base

━━━━━━━━━━━━━━━━━━━━
+ Lone
```

```output
true
true
false
false
true
```

### An object is never of a class that is not in its chain.

```yumi
. c = Child()
-- c is Other

━━━━━━━━━━━━━━━━━━━━
+ Base

+ n = 1

━━━━━━━━━━━━━━━━━━━━
+ Child is Base

+ m = 7

━━━━━━━━━━━━━━━━━━━━
+ Other is Base

━━━━━━━━━━━━━━━━━━━━
+ Lone
```

```output
false
```

### An object of a class two below is of each class above it.

```yumi
. g Base = Grand()
-- g is Child
-- g is Grand
-- g is Other
. c Child = Grand()
-- c is Grand
-- (c as Grand).deep

━━━━━━━━━━━━━━━━━━━━
+ Grand is Child

+ deep = 99

━━━━━━━━━━━━━━━━━━━━
+ Base

+ n = 1

━━━━━━━━━━━━━━━━━━━━
+ Child is Base

+ m = 7

━━━━━━━━━━━━━━━━━━━━
+ Other is Base

━━━━━━━━━━━━━━━━━━━━
+ Lone
```

```output
true
true
false
true
99
```

### An object held as its base is cast to the class it was made as.

```yumi
. b Base = Child()
. c = b as Child
-- c.m
-- (b as Child).m
. u = c as Base
-- u.n

━━━━━━━━━━━━━━━━━━━━
+ Base

+ n = 1

━━━━━━━━━━━━━━━━━━━━
+ Child is Base

+ m = 7

━━━━━━━━━━━━━━━━━━━━
+ Other is Base

━━━━━━━━━━━━━━━━━━━━
+ Lone
```

```output
7
7
1
```

### A cast to a class the object is not of panics.

```yumi crash
. b Base = Child()
-- "before"
. o = b as Other
-- "not here"

━━━━━━━━━━━━━━━━━━━━
+ Base

+ n = 1

━━━━━━━━━━━━━━━━━━━━
+ Child is Base

+ m = 7

━━━━━━━━━━━━━━━━━━━━
+ Other is Base

━━━━━━━━━━━━━━━━━━━━
+ Lone
```

```stderr
Unhandled panic: Invalid cast to [Object __source__:Other]
<no file:4>

Task 1

Stack trace:
  0  __source__:main no file:4:7
  1  __source__:init.__source__ no file
  2  __source__:brix.main no file
```

### A value of a base class cast with as and stored in an optional property of a derived class panics when the object is not of that class.

```yumi crash
. h = Holder()
h.f(Child())
-- h.c?.value
h.f(Box())

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ c Child?

────────────────────
+ do f
    - b Box

c = b as Child

━━━━━━━━━━━━━━━━━━━━
+ Box

━━━━━━━━━━━━━━━━━━━━
+ Child is Box

+ value = 5
```

```stderr
Unhandled panic: Invalid cast to [Object __source__:Child]
<no file:16>

Task 1

Stack trace:
  0  __source__:Holder:f no file:16:5
  1  __source__:main no file:5:1
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

### An optional of a base class cast to a derived class unwraps, then panics when the object is not of that class.

```yumi crash
. b = Box()
b.first = Child()
. c = b.first as Child
-- c.value
b.first = Box()
. d = b.first as Child
-- "not here"

━━━━━━━━━━━━━━━━━━━━
+ Box

+ first Box?

━━━━━━━━━━━━━━━━━━━━
+ Child is Box

+ value = 3
```

```stderr
Unhandled panic: Invalid cast to [Object __source__:Child]
<no file:7>

Task 1

Stack trace:
  0  __source__:main no file:7:7
  1  __source__:init.__source__ no file
  2  __source__:brix.main no file
```

### A member a class inherits through its base is the nearest one declared above it.

```yumi
. leaf = Leaf()
. held Mid = Leaf()
-- leaf.one(3)
-- held.one(4)
-- leaf.label
leaf.value = 4
-- leaf.stored
-- leaf.plain()
-- leaf.both()
-- Last().one(5)
-- Last().both()

━━━━━━━━━━━━━━━━━━━━
+ Base

+ stored = 0

────────────────────
+ one String
    - n Int

"base one $n"

────────────────────
+ get label

"base label"

────────────────────
+ do value = v Int

stored = v

────────────────────
+ plain String

"base plain"

────────────────────
+ both String

"${one(1)} ${label}"

━━━━━━━━━━━━━━━━━━━━
+ Mid is Base

────────────────────
+ one String
    - n Int

"mid one $n"

────────────────────
+ get label

"mid label"

────────────────────
+ do value = v Int

stored = v * 10

━━━━━━━━━━━━━━━━━━━━
+ Leaf is Mid

━━━━━━━━━━━━━━━━━━━━
+ Last is Leaf
```

```output
mid one 3
mid one 4
mid label
40
base plain
mid one 1 mid label
mid one 5
mid one 1 mid label
```

### `super` reaches the nearest member declared above the base.

```yumi
-- Leaf().name()
-- Leaf().label
-- Leaf().n

━━━━━━━━━━━━━━━━━━━━
+ Base

+ n = 1

────────────────────
+ name String

"base"

────────────────────
+ get label

"base label"

━━━━━━━━━━━━━━━━━━━━
+ Mid1 is Base

+ n = 2

────────────────────
+ name String

"mid1"

────────────────────
+ get label

"mid1 label"

━━━━━━━━━━━━━━━━━━━━
+ Mid2 is Mid1

━━━━━━━━━━━━━━━━━━━━
+ Leaf is Mid2

────────────────────
+ name String

"leaf<${super.name()}>"

────────────────────
+ get label

"leaf<${super.label}>"
```

```output
leaf<mid1>
leaf<mid1 label>
2
```

### A value a class gives a property of its base is converted to the type the base gave the property, through every class between.

```yumi
. leaf = Leaf()
-- leaf.a
-- half(leaf)
-- (type-of leaf.a).name
-- half(Mid())
-- half(Base())
leaf.a = 3
-- half(leaf)

━━━━━━━━━━━━━━━━━━━━
+ Base

+ a = 42.0

━━━━━━━━━━━━━━━━━━━━
+ Mid is Base

+ a = 99

━━━━━━━━━━━━━━━━━━━━
+ Leaf is Mid

+ a = 7

━━━━━━━━━━━━━━━━━━━━
+ half
    - base Base

base.a / 2
```

```output
7
3.5
F64
49.5
21
1.5
```

### A class gives a property its base declared a type for a value of another type, which is converted as one assigned is.

```yumi
. bar = Bar()
-- bar.a
-- (type-of bar.a).name
-- bar.wide
-- (type-of bar.wide).name
-- bar.count
-- (type-of bar.count).name
-- total(bar)

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ a F64 = 42.0
+ wide = 1 as I64
+ count = 3

━━━━━━━━━━━━━━━━━━━━
+ Bar is Foo

+ a = 99
+ wide = 7
+ count = 12u8

━━━━━━━━━━━━━━━━━━━━
+ total
    - foo Foo

foo.a + foo.wide + foo.count
```

```output
99
F64
7
I64
12
I64
118
```

### A getter and a method of the base are reached after super.

```yumi
. box = Box()
box.grow()

━━━━━━━━━━━━━━━━━━━━
+ Base

+ held = 1

────────────────────
+ get twice

held * 2

────────────────────
+ do show

-- "base show"

━━━━━━━━━━━━━━━━━━━━
+ Box is Base

────────────────────
+ get twice

held * 3

────────────────────
+ do show

-- "box show"

────────────────────
+ do grow

-- super.twice
-- twice
super.show()
show()
```

```output
2
3
base show
box show
```

### super is no value: a stored property is not read after it.

```yumi crash
. box = Box()
box.grow()

━━━━━━━━━━━━━━━━━━━━
+ Base

+ held = 1

━━━━━━━━━━━━━━━━━━━━
+ Box is Base

────────────────────
+ do grow

-- super.held
```

```output
Exception: Bare super is not a value
<no file:16>
    -- super.held
       ^^^^^
```

### super is no value: a property is not assigned after it.

```yumi crash
. box = Box()
box.grow()

━━━━━━━━━━━━━━━━━━━━
+ Base

+ held = 1

━━━━━━━━━━━━━━━━━━━━
+ Box is Base

────────────────────
+ do grow

super.held = 5
```

```output
Exception: Bare super is not a value
<no file:16>
    super.held = 5
    ^^^^^
```

### super is no value: it is not given as an argument.

```yumi crash
. box = Box()
box.grow()

━━━━━━━━━━━━━━━━━━━━
+ Base

+ held = 1

━━━━━━━━━━━━━━━━━━━━
+ Box is Base

────────────────────
+ do grow

take(super)

━━━━━━━━━━━━━━━━━━━━
+ do take
    - base Base

-- base.held
```

```output
Exception: Bare super is not a value
<no file:16>
    take(super)
         ^^^^^
```

### A method with type parameters of its own that a subclass declares again is called through a value held as the base, with the type parameters the call gave.

```yumi
. bar = Bar()
. foo Foo = bar
foo.f1(5.5)
foo.f1(3)
foo.f2'Int(1)
foo.f2'String("a")
-- foo.f3'Int()
-- foo.f3'String()
. plain Foo = Foo()
plain.f1(2)
plain.f2'Int(1)
-- plain.f3'Int()

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ do f1'T
    - a T

-- "Foo 1 $a"

────────────────────
+ do f2'T
    - a T

-- "Foo 2 $a"

────────────────────
+ f3'T String

"Foo 3"

━━━━━━━━━━━━━━━━━━━━
+ Bar is Foo

────────────────────
+ do f1'T
    - a T

-- "Bar 1 $a"

────────────────────
+ do f2
    - a Int

-- "Bar 2 $a"

────────────────────
+ f3'T String
    - T is! Int

"Bar 3"
```

```output
Bar 1 5.5
Bar 1 3
Foo 2 1
Foo 2 a
Bar 3
Foo 3
Foo 1 2
Foo 2 1
Foo 3
```

### A method whose type parameter must be exactly This answers only for that class, called directly or through the base.

```yumi
. box Box = Square()
-- box.pick'Square()
-- box.pick'Circle()
. square = Square()
-- square.pick'Square()
-- square.pick'Circle()

━━━━━━━━━━━━━━━━━━━━
+ Box

────────────────────
+ pick'Target String

"box"

━━━━━━━━━━━━━━━━━━━━
+ Circle is Box

━━━━━━━━━━━━━━━━━━━━
+ Square is Box

────────────────────
+ pick'Target String
    - Target is! This

"square"
```

```output
square
box
square
box
```

### A lazy property's value is typed once, for the class that declares it, and reads the object it is evaluated for.

```yumi
. d = Derived()
-- d.who
. b = Base()
-- b.who
. held Base = Derived()
-- held.who
-- Other().who
. other Base = Other()
-- other.who
-- d.said
-- b.said
-- held.said
-- d.loud
-- name-of(d.me)
-- name-of(Other().me)

━━━━━━━━━━━━━━━━━━━━
+ describe String
    - b Base

"base"

━━━━━━━━━━━━━━━━━━━━
+ describe String
    - d Derived

"derived"

━━━━━━━━━━━━━━━━━━━━
+ describe String
    - o Other

"other"

━━━━━━━━━━━━━━━━━━━━
+ name-of String
    - b Base

"${describe(b)} ${b.label}"

━━━━━━━━━━━━━━━━━━━━
+ Base

+ who := describe(this)
+ said := label ++ "!"
+ loud := this.label ++ "?"
+ me := this

────────────────────
+ get label

"plain"

━━━━━━━━━━━━━━━━━━━━
+ Derived is Base

────────────────────
+ get label

"fancier"

━━━━━━━━━━━━━━━━━━━━
+ Other is Base

+ who := describe(this)
```

```output
base
base
base
other
other
fancier!
plain!
fancier!
fancier?
base fancier
base plain
```

### A method that writes a type for an argument overrides a base method that writes none, called through the base, from inside it, and through a class between.

```yumi
. held Base = Derived()
held.put(1)
held.relay(2)
. lower Base = Bottom()
lower.put(3)

━━━━━━━━━━━━━━━━━━━━
+ Base

────────────────────
+ do put
    - thing

-- "(base any, $thing)"

────────────────────
+ do relay
    - value Int

put(value)

━━━━━━━━━━━━━━━━━━━━
+ Derived is Base

────────────────────
+ do put
    - thing Int

-- "(derived int, $thing)"

━━━━━━━━━━━━━━━━━━━━
+ Middle is Base

━━━━━━━━━━━━━━━━━━━━
+ Bottom is Middle

────────────────────
+ do put
    - thing Int

-- "(bottom int, $thing)"
```

```output
(derived int, 1)
(derived int, 2)
(bottom int, 3)
```

### An override that answers a subclass of what the base answers is called through an adapter, which a trace names.

```yumi crash
. base Animal = Dog()
-- base.make().name

━━━━━━━━━━━━━━━━━━━━
+ Animal

+ name = "animal"

────────────────────
+ make Animal

Animal()

━━━━━━━━━━━━━━━━━━━━
+ Dog is Animal

────────────────────
+ make Dog

?! Panic("boom")
```

```stderr
Unhandled panic: boom
<no file:21>

Task 1

Stack trace:
  0  __source__:Dog:make no file:21:1
  1  __source__:make(class-adapter) no file:19:1
  2  __source__:main no file:3:4
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### An override that answers a narrower number than the base converts it where it is called as the base.

```yumi
. base Source = Narrow()
-- base.value()

━━━━━━━━━━━━━━━━━━━━
+ Source

────────────────────
+ value Int

1

━━━━━━━━━━━━━━━━━━━━
+ Narrow is Source

────────────────────
+ value I16

7
```

```output
7
```

### A panic in an override that answers a narrower number names the adapter in its trace.

```yumi crash
. base Source = Narrow()
-- base.value()

━━━━━━━━━━━━━━━━━━━━
+ Source

────────────────────
+ value Int

1

━━━━━━━━━━━━━━━━━━━━
+ Narrow is Source

────────────────────
+ value I16

?! Panic("boom")
```

```stderr
Unhandled panic: boom
<no file:19>

Task 1

Stack trace:
  0  __source__:Narrow:value no file:19:1
  1  __source__:value(class-adapter) no file:17:1
  2  __source__:main no file:3:4
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### An override answering a type its base's answer converts to fills the base's slot with an adapter.

```yumi
. b Base = Sub()
-- b.value()

━━━━━━━━━━━━━━━━━━━━
+ Base

────────────────────
+ value Float

1.5

━━━━━━━━━━━━━━━━━━━━
+ Sub is Base

────────────────────
+ value Int

300
```

```output
300
```

### A getter that overrides another may not state its type.

```yumi crash
. base Source = Narrow()
-- base.value

━━━━━━━━━━━━━━━━━━━━
+ Source

────────────────────
+ get value Int

1

━━━━━━━━━━━━━━━━━━━━
+ Narrow is Source

────────────────────
+ get value Int

7
```

```output
Exception: "value" is inherited from Source, so it keeps the type it has there and cannot state its own; remove "Int"
<no file:17>
    + get value Int
    ^^^^^^^^^^^^^^
```

### A getter may state its type over a base getter that states none.

```yumi
. base Source = Narrow()
-- base.value
-- Narrow().value

━━━━━━━━━━━━━━━━━━━━
+ Source

────────────────────
+ get value

1

━━━━━━━━━━━━━━━━━━━━
+ Narrow is Source

────────────────────
+ get value Int

7
```

```output
7
7
```

### A getter may state its type over a chain of getters whose first states none, though the one it overrides states one.

```yumi
. base Source = Leaf()
-- base.value
-- Mid().value

━━━━━━━━━━━━━━━━━━━━
+ Source

────────────────────
+ get value

1

━━━━━━━━━━━━━━━━━━━━
+ Mid is Source

────────────────────
+ get value Int

2

━━━━━━━━━━━━━━━━━━━━
+ Leaf is Mid

────────────────────
+ get value Int

3
```

```output
3
2
```

### A getter may not state its type over a chain of getters whose first states one, though the one it overrides states none.

```yumi crash
. base Source = Leaf()
-- base.value

━━━━━━━━━━━━━━━━━━━━
+ Source

────────────────────
+ get value Int

1

━━━━━━━━━━━━━━━━━━━━
+ Mid is Source

────────────────────
+ get value

2

━━━━━━━━━━━━━━━━━━━━
+ Leaf is Mid

────────────────────
+ get value Int

3
```

```output
Exception: "value" is inherited from Mid, so it keeps the type it has there and cannot state its own; remove "Int"
<no file:25>
    + get value Int
    ^^^^^^^^^^^^^^
```

### Two classes made from one generic class, each overriding a method of their base with one that answers another type, each have an adapter of their own in the base's table.

```yumi
. first Base = Holder'String(["a", "b"])
. second Base = Holder'I64([1, 2, 3])
-- first.total
-- second.total

━━━━━━━━━━━━━━━━━━━━
+ Base

────────────────────
+ get total Index

0

━━━━━━━━━━━━━━━━━━━━
+ Holder'Item is Base

+ items [Item]

────────────────────
+ This
    - items

────────────────────
+ get total

items.length
```

```output
2
3
```

### A class nothing uses is not checked, so its override of a method called on its base is never typed.

```yumi
. shape Shape = Square()
print(shape.area())

━━━━━━━━━━━━━━━━━━━━
+ Shape

────────────────────
+ area Int

0

━━━━━━━━━━━━━━━━━━━━
+ Square is Shape

────────────────────
+ area Int

4

━━━━━━━━━━━━━━━━━━━━
+ Broken is Shape

+ size Int

────────────────────
+ area Int

size * missing
```

```output
4
```

### A base-typed reference and a subclass-typed reference to one object compare equal.

```yumi
. bar = Bar()
. other = Bar()
. holder = Holder()
holder.held = bar
-- holder.held! == bar
-- bar == holder.held!
-- holder.held! != bar
-- holder.held! == other
-- holder.held! != other
bar.hold(holder)

━━━━━━━━━━━━━━━━━━━━
+ Foo

━━━━━━━━━━━━━━━━━━━━
+ Bar is Foo

────────────────────
+ do hold
    - holder Holder

holder.held = this
-- holder.held! == this

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ held Foo?
```

```output
true
true
false
false
true
true
```

### A method that returns nothing does not override one that returns a truth or an optional.

```yumi crash
. shapes [Shape]
shapes += Shape()
shapes += Dot()
for shape in shapes
    -- shape.check()
    -- shape.label()

━━━━━━━━━━━━━━━━━━━━
+ Shape

────────────────────
+ do check Bool

= true

────────────────────
+ do label String?

= "shape"

━━━━━━━━━━━━━━━━━━━━
+ Dot is Shape

────────────────────
+ do check

-- "dot check"

────────────────────
+ do label

-- "dot label"
```

```output
Exception: Expected Bool here, but this is Void, which becomes Bool only when asked; write "as Bool"
<no file:26>
    + do check
    ^^^^^^^^^

Exception: Expected String? here, but this is Void, which becomes String? only when asked; write "as String?"
<no file:31>
    + do label
    ^^^^^^^^^
```

### A class named as a core class is the base of a subclass declared as of that name, so a method called through it reaches the subclass's override, though the call is compiled first.

```yumi
-- show(List())
-- show(Thing())

━━━━━━━━━━━━━━━━━━━━
+ List

────────────────────
+ name String

"list"

━━━━━━━━━━━━━━━━━━━━
+ Thing is List

────────────────────
+ name String

"thing"

━━━━━━━━━━━━━━━━━━━━
+ show String
    - list List

list.name()
```

```output
list
thing
```

### A method that declares no type, and gives no value as nothing after a statement that leaves is compiled, overrides one that gives a value, and runs when called through the base.

```yumi crash
. node Node = Failing()
-- node.execute()

━━━━━━━━━━━━━━━━━━━━
+ Node

────────────────────
+ do execute I32

= 0

━━━━━━━━━━━━━━━━━━━━
+ Failing is Node

────────────────────
+ This

────────────────────
+ do execute

. prior = 1
?! Panic("boom")
= prior
```

```stderr
Unhandled panic: boom
<no file:23>

Task 1

Stack trace:
  0  __source__:Failing:execute no file:23:1
  1  __source__:execute(class-adapter) no file:20:1
  2  __source__:main no file:3:4
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

```yumi
. numbers = Numbers()
-- numbers[1]
. plain = PlainNumbers()
-- plain[0]

━━━━━━━━━━━━━━━━━━━━
+ Holder'T

+ items [T] = List'T()

────────────────────
+ this[index Index]

items[index]

━━━━━━━━━━━━━━━━━━━━
+ Numbers is Holder'Int

────────────────────
+ This

items += 4
items += 5

━━━━━━━━━━━━━━━━━━━━
+ PlainHolder

+ items [Int] = List'Int()

────────────────────
+ this[index Index]

items[index]

━━━━━━━━━━━━━━━━━━━━
+ PlainNumbers is PlainHolder

────────────────────
+ This

items += 6
```

```output
5
6
```

### A getter over one whose type a base states keeps that type where the class reads it, so an integer it gives is of that type.

```yumi
. boat = Rowboat()
-- boat.quarter
. held Boat = boat
-- held.drop

━━━━━━━━━━━━━━━━━━━━
+ Boat

────────────────────
+ get drop F64

0.5

━━━━━━━━━━━━━━━━━━━━
+ Rowboat is Boat

────────────────────
+ get drop

30

────────────────────
+ get quarter

drop / 4
```

```output
7.5
30
```

```yumi
. plain = PlainPainter()
. checked = CheckedPainter()
draw(plain, 2)
draw(checked, 3)
-- plain.value
-- checked.value
plain.transform(4)
-- plain.value

━━━━━━━━━━━━━━━━━━━━
+ Painter

+ value Float

────────────────────
+ do translate
    - x Float

transform(x)

────────────────────
+ do transform
    - x Float

value = x

━━━━━━━━━━━━━━━━━━━━
+ PlainPainter is Painter

────────────────────
+ do translate
    - x Float

transform(x)

━━━━━━━━━━━━━━━━━━━━
+ CheckedPainter is Painter

────────────────────
+ do transform
    - x Float

?! Panic("negative") if x < 0
value = x + 10

━━━━━━━━━━━━━━━━━━━━
+ do draw
    - painter Painter
    - x Float

painter.translate(x)
```

```output
2
13
4
```

### A class is built on one type, written in its header: a row under it that names a class names no interface, and is an error.

```yumi compile-error
. c = C()

━━━━━━━━━━━━━━━━━━━━
+ A

━━━━━━━━━━━━━━━━━━━━
+ B

━━━━━━━━━━━━━━━━━━━━
+ C is A
    is B
```

```output
Exception: C cannot implement B, which is not an interface; C is built on A, and a type is built on one type
<no file:12>
        is B
           ^
```

### A row under a class's header that names a class, where the header names none, says to write it in the header.

```yumi compile-error
. c = C()

━━━━━━━━━━━━━━━━━━━━
+ A

━━━━━━━━━━━━━━━━━━━━
+ C
    is A
```

```output
Exception: C cannot implement A, which is not an interface; a type is built on one type, written in its header, as in "+ C is A"
<no file:9>
        is A
           ^
```
