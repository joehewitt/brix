# Methods

### Super calls exact base method implementation

```yumi
. child = Child()
child.speak()

━━━━━━━━━━━━━━━━━━━━
+ Base

────────────────────
+ do speak

-- "base"

━━━━━━━━━━━━━━━━━━━━
+ Child is Base

────────────────────
+ do speak

-- "child"
super.speak()
```

```output
child
base
```

### Super starts at the immediate base but can find an ancestor method

```yumi
. leaf = Leaf()
leaf.speak()

━━━━━━━━━━━━━━━━━━━━
+ Root

────────────────────
+ do speak

-- "root"

━━━━━━━━━━━━━━━━━━━━
+ Middle is Root

━━━━━━━━━━━━━━━━━━━━
+ Leaf is Middle

────────────────────
+ do speak

-- "leaf"
super.speak()
```

```output
leaf
root
```

### Super method keeps the current receiver

```yumi
. child = Child()
child.label = "child"
child.show()

━━━━━━━━━━━━━━━━━━━━
+ Base

+ label = "base"

────────────────────
+ do show

-- "base sees $label"

━━━━━━━━━━━━━━━━━━━━
+ Child is Base

+ label = "child"

────────────────────
+ do show

-- "child sees $label"
super.show()
```

```output
child sees child
base sees child
```

### Super helper thunks do not enter sibling class method tables

```yumi
. calls = CallsSuper()
calls.hook()
. tracked = TrackedChild()
tracked.hook()
-- calls.count
-- tracked.status
-- tracked.count

━━━━━━━━━━━━━━━━━━━━
+ Base

+ count = 0

────────────────────
+ do hook

count += 1

━━━━━━━━━━━━━━━━━━━━
+ CallsSuper is Base

────────────────────
+ do hook

super.hook()

━━━━━━━━━━━━━━━━━━━━
+ TrackedChild is Base

+ status = ""

. mark < status

────────────────────
+ do hook

status = "changed"

────────────────────
+ do mark

count += 1
```

```output
1
changed
1
```

### Super call on numeric core type

```yumi
. number = TaggedInt(7)
-- number.pad(3)

━━━━━━━━━━━━━━━━━━━━
+ TaggedInt (tagged) is I32

────────────────────
+ pad
    - n Int

"tagged ${super.pad(n)}"
```

```output
tagged 007
```

### Super call on tuple core type

```yumi
. pair = OffsetPair(2, 3)
-- pair.sum()

━━━━━━━━━━━━━━━━━━━━
+ Pair is (x Int, y Int)

────────────────────
+ sum Int

x + y

━━━━━━━━━━━━━━━━━━━━
+ OffsetPair is Pair

────────────────────
+ sum Int

super.sum() + 10
```

```output
15
```

### Super call on string core type

```yumi
. name = LoudName("Ada")
-- name.decorate()

━━━━━━━━━━━━━━━━━━━━
+ Name is String

────────────────────
+ decorate String

"<$this>"

━━━━━━━━━━━━━━━━━━━━
+ LoudName is Name

────────────────────
+ decorate String

"LOUD ${super.decorate()}"
```

```output
LOUD <Ada>
```

### Super constructor initializes the same object

```yumi
. child = Child(4)
-- child.base-value

━━━━━━━━━━━━━━━━━━━━
+ Base

+ base-value = 0

────────────────────
+ This
    - n Int

base-value = n
-- "base ${this.base-value}"

━━━━━━━━━━━━━━━━━━━━
+ Child is Base

────────────────────
+ This
    - n Int

super(n + 1)
-- "child ${this.base-value}"
```

```output
base 5
child 5
5
```

### Super constructor uses base dispatch inside the constructor body

```yumi
Child()

━━━━━━━━━━━━━━━━━━━━
+ Base

────────────────────
+ This

report()

────────────────────
+ do report

-- "base"

━━━━━━━━━━━━━━━━━━━━
+ Child is Base

────────────────────
+ This

super()

────────────────────
+ do report

-- "child"
```

```output
base
```

### Super constructor can target a default base constructor

```yumi
. child = Child()
-- child.base-value

━━━━━━━━━━━━━━━━━━━━
+ Base

+ base-value = 8

━━━━━━━━━━━━━━━━━━━━
+ Child is Base

────────────────────
+ This

super()
```

```output
8
```

### Super constructor may only be called once

```yumi crash
Child()

━━━━━━━━━━━━━━━━━━━━
+ Base

────────────────────
+ This

-- "base"

━━━━━━━━━━━━━━━━━━━━
+ Child is Base

────────────────────
+ This

super()
super()
```

```output
Exception: super constructor call must be first
<no file:19>
    super()
    ^^^^^^^
```

### Super constructor must be first executable statement

```yumi crash
Child()

━━━━━━━━━━━━━━━━━━━━
+ Base

────────────────────
+ This

-- "base"

━━━━━━━━━━━━━━━━━━━━
+ Child is Base

────────────────────
+ This

-- "before"
super()
```

```output
Exception: super constructor call must be first
<no file:19>
    super()
    ^^^^^^^
```

### Super constructor only works in constructors

```yumi crash
Child().run()

━━━━━━━━━━━━━━━━━━━━
+ Base

━━━━━━━━━━━━━━━━━━━━
+ Child is Base

────────────────────
+ run

super()
```

```output
Exception: super constructor call is only allowed in constructors
<no file:13>
    super()
    ^^^^^^^
```

### Super requires an inherited base type

```yumi crash
Thing().run()

━━━━━━━━━━━━━━━━━━━━
+ Thing

────────────────────
+ run

super.run()
```

```output
Exception: super requires a base type
<no file:10>
    super.run()
    ^^^^^
```

```yumi
. thing = Thing()
thing.foo(5)
thing.bar(5)

━━━━━━━━━━━━━━━━━━━━
+ Thing

────────────────────
+ do foo
    - x

-- "foo$x"

────────────────────
+ do bar
    - x

-- "bar$x"
```

```output
foo5
bar5
```

### Infer return value of method call

```yumi
. thing = Thing()
-- thing.foo()

━━━━━━━━━━━━━━━━━━━━
+ Thing

────────────────────
+ foo

plop()

────────────────────
+ plop

42
```

```output
42
```

```yumi
-- plop()

━━━━━━━━━━━━━━━━━━━━
+ Thing

────────────────────
+ foo

42

━━━━━━━━━━━━━━━━━━━━
+ do plop

. thing = Thing()
= thing.foo()
```

```output
42
```

```yumi crash
. thing = Thing()
thing.foo()

━━━━━━━━━━━━━━━━━━━━
+ Thing
```

```output
Exception: Thing has no method "foo"
<no file:3>
    thing.foo()
    ^^^^^^^^^^^
```

```yumi
. thing = Thing()
thing.foo()

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ a = 42

────────────────────
+ do foo

-- a
```

```output
42
```

### Generic default arguments are specialized by type parameters

```yumi
/core/markup
/core/tests/support/markup-fixtures

. parent = Box()
. child = Box()
. form = Form()

markup-append(parent, child)
markup-append(form, parent)

-- child.ancestor'Box
-- child.ancestor'Form
```

```output
[Object core/tests/support/markup-fixtures:Box]
[Object core/tests/support/markup-fixtures:Form]
```

```yumi
. thing = Thing()
thing.foo()

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ a = 42

────────────────────
+ do foo

-- this.a
```

```output
42
```

```yumi
. thing = Thing()
thing.foo()

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ a = 42

────────────────────
+ do foo

bar()

────────────────────
+ do bar

-- a
```

```output
42
```

```yumi
. thing = Thing()
thing.foo()

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ a = 42

────────────────────
+ do foo

this.bar()

────────────────────
+ do bar

-- a
```

```output
42
```

```yumi
. thing = Thing()
thing.bling.foo()

━━━━━━━━━━━━━━━━━━━━
+ Bling

+ a = 42

────────────────────
+ do foo

-- a

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ bling = Bling()
```

```output
42
```

### Method takes precedence over static function

```yumi
. thing = Thing()
thing.foo()

━━━━━━━━━━━━━━━━━━━━
+ do bar

-- 1

━━━━━━━━━━━━━━━━━━━━
+ Thing

────────────────────
+ do foo

bar()

────────────────────
+ do bar

-- 2
```

```output
2
```

### Static function and method with same name but different arguments can be called

```yumi
. thing = Thing()
thing.foo()

━━━━━━━━━━━━━━━━━━━━
+ do bar
    - a
    - b

-- 1

━━━━━━━━━━━━━━━━━━━━
+ Thing

────────────────────
+ do foo

bar(1, 2)
bar(0)

────────────────────
+ do bar
    - x
-- 2
```

```output
1
2
```

### Method with type variables

```yumi
. thing = Thing()
thing.foo(42.5)

━━━━━━━━━━━━━━━━━━━━
+ Thing

────────────────────
+ do foo'T
    - x T

-- x
```

```output
42.5
```

```yumi
. thing = Thing'Float()
thing.foo(5, 42.5)

━━━━━━━━━━━━━━━━━━━━
+ Thing'T

────────────────────
+ do foo'G
    - x
    - y G

-- x + y
```

```output
47.5
```

```yumi
. thing = Thing'Float()
thing.foo(42)

━━━━━━━━━━━━━━━━━━━━
+ Thing'T

────────────────────
+ do foo
    - x T

-- x
```

```output
42
```

```yumi
. thing = Thing()
thing.foo(24)
thing.foo(24, 26)

━━━━━━━━━━━━━━━━━━━━
+ Thing

────────────────────
+ do foo
    - a I32
    - b I32 = 25

-- a
-- b
```

```output
24
25
24
26
```

### Class override may not add undeclared errors to a non-error base slot

```yumi crash
. base Base = Child()
-- base.value()

━━━━━━━━━━━━━━━━━━━━
+ Base

────────────────────
+ value Int

1

━━━━━━━━━━━━━━━━━━━━
+ Child is Base

────────────────────
+ value Int

?! Error("bad")
```

```output
Exception: "value" can fail with Error, but the method it overrides cannot; add "?! Error" to that method, or catch the error inside "value"
<no file:17>
    + value Int
    ^^^^^^^^^^
Base method:
<no file:9>
    + value Int
```

### An override that states no errors carries its base's errors, even when it is called through its own class before anything else compiles the base method

```yumi
. child = Child()
child.run()
    catch error Error
        -- "caught $error"
-- child.count

━━━━━━━━━━━━━━━━━━━━
+ Base

+ count = 0

────────────────────
+ do run ?! Error

count += 1

━━━━━━━━━━━━━━━━━━━━
+ Child is Base

────────────────────
+ do run

count += 2
```

```output
2
```

### Inherited methods called on subclasses still dispatch to overrides

```yumi
. nodes = List'Node()
nodes += Plain(1)
nodes += Fancy(2)
nodes += Plain(3)
for node in nodes
    -- node.describe()
. fancy = Fancy(5)
-- fancy.describe()
-- fancy.total()

━━━━━━━━━━━━━━━━━━━━
+ Node

+ value Int = 0

────────────────────
+ This
    - value

────────────────────
+ label String

"node"

────────────────────
+ describe String

"${label()} ${value}"

────────────────────
+ total Int

value + weight()

────────────────────
+ weight Int

1

━━━━━━━━━━━━━━━━━━━━
+ Plain is Node

━━━━━━━━━━━━━━━━━━━━
+ Fancy is Node

────────────────────
+ label String

"fancy"

────────────────────
+ weight Int

10
```

```output
node 1
fancy 2
node 3
fancy 5
15
```

### Inherited generator getters recurse through base-typed children

```yumi
. root = Group()
. inner = Region()
inner.kids += Leaf(2)
inner.kids += Leaf(3)
root.kids += Leaf(1)
root.kids += inner
root.kids += Leaf(4)
for node in root.flattened
    -- node.value

━━━━━━━━━━━━━━━━━━━━
+ Node

+ value Int = 0
+ kids = List'Node()
+ is-transparent = false

────────────────────
+ do get flattened [Node]

for child in kids
    if child.is-transparent
        <<= child.flattened
    else
        <= child

━━━━━━━━━━━━━━━━━━━━
+ Group is Node

━━━━━━━━━━━━━━━━━━━━
+ Region is Node

+ is-transparent = true

━━━━━━━━━━━━━━━━━━━━
+ Leaf is Node

────────────────────
+ This
    - value
```

```output
1
2
3
4
```

### A super constructor sets only the base properties its arguments determine.

```yumi
+ count = 0

. c = Child(3)
-- "id ${c.id} k ${c.k} double ${c.double} tag ${c.tag} label ${c.label} extra ${c.extra}"
-- "count $count"

━━━━━━━━━━━━━━━━━━━━
+ do next-id Int

count += 1
= count

━━━━━━━━━━━━━━━━━━━━
+ Base

+ id = next-id()
+ k Int = 0
+ double = k * 2
+ tag = "$id:$k"
+ label String = "none"

────────────────────
+ This
    - k Int

.. label = "base $k"

━━━━━━━━━━━━━━━━━━━━
+ Child is Base

+ extra Int = 5

────────────────────
+ This
    - n Int

super(n + 1)
extra = n
```

```output
id 1 k 4 double 8 tag 1:4 label base 4 extra 3
count 1
```

### Recursive generator getters dispatch virtually from the declaring class

```yumi
. root = Node()
. inner = Region()
inner.kids += Leaf(2)
inner.kids += Leaf(3)
root.kids += Leaf(1)
root.kids += inner
root.kids += Leaf(4)
for node in root.flattened
    -- node.value

━━━━━━━━━━━━━━━━━━━━
+ Node

+ value Int = 0
+ kids = List'Node()
+ is-transparent = false

────────────────────
+ do get flattened [Node]

for child in kids
    if child.is-transparent
        <<= child.flattened
    else
        <= child

━━━━━━━━━━━━━━━━━━━━
+ Region is Node

+ is-transparent = true

━━━━━━━━━━━━━━━━━━━━
+ Leaf is Node

────────────────────
+ This
    - value
```

```output
1
2
3
4
```

### Inherited getters and setters reach subclass overrides through dynamic dispatch

```yumi
. shapes = List'Shape()
shapes += Shape()
shapes += Circle()
for shape in shapes
    -- shape.description
    shape.resize(3)
    -- shape.log
. circle = Circle()
-- circle.description

━━━━━━━━━━━━━━━━━━━━
+ Shape

+ log = ""
+ kind => "shape"
+ description => "a $kind"

────────────────────
+ do size = value Int

log = "shape $value"

────────────────────
+ do resize
    - value Int

size = value

━━━━━━━━━━━━━━━━━━━━
+ Circle is Shape

+ kind => "circle"

────────────────────
+ do size = value Int

log = "circle $value"
```

```output
a shape
shape 3
a circle
circle 3
a circle
```

### Inherited getters see This as their declaring class, like methods

```yumi
. circle = Circle()
-- circle.made-kind
-- circle.made-kind-method()

━━━━━━━━━━━━━━━━━━━━
+ Shape

+ kind => "shape"
+ made-kind => This().kind

────────────────────
+ do made-kind-method

= This().kind

━━━━━━━━━━━━━━━━━━━━
+ Circle is Shape

+ kind => "circle"
```

```output
shape
shape
```

### Inherited getters reach subclass implementations of interface default getters

```yumi
. nodes = List'Node()
nodes += Node()
nodes += Region()
for node in nodes
    -- node.is-transparent-node

━━━━━━━━━━━━━━━━━━━━
+ Host is Interface

────────────────────
+ get transparent

false

━━━━━━━━━━━━━━━━━━━━
+ Node
    is Host

+ is-transparent-node => transparent

━━━━━━━━━━━━━━━━━━━━
+ Region is Node

────────────────────
+ get transparent

true
```

```output
false
true
```

### Optional chaining of a method call on a receiver that is not optional is illegal.

```yumi crash
. thing = Thing()
thing?.show()

━━━━━━━━━━━━━━━━━━━━
+ Thing

────────────────────
+ do show

-- "shown"
```

```output
Exception: "thing" is Thing, which is never null, so it needs no "?"; write "thing.show()"
<no file:3>
    thing?.show()
    ^^^^^^^^^^^^^
```

### Optional chaining of a method call on the receiver's base is illegal.

```yumi crash
Kid().go()

━━━━━━━━━━━━━━━━━━━━
+ Thing

────────────────────
+ do show

-- "shown"

━━━━━━━━━━━━━━━━━━━━
+ Kid is Thing

────────────────────
+ do go

super?.show()
```

```output
Exception: "super" is Thing, which is never null, so it needs no "?"; write "super.show()"
<no file:18>
    super?.show()
    ^^^^^^^^^^^^^
```
