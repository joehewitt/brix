# Property changes

```yumi
./base

. value = Derived()
-- value.result
value.input = 7
-- value.result

━━━━━━━━━━━━━━━━━━━━
+ Derived is Base
```

```yumi file=base.yumi

━━━━━━━━━━━━━━━━━━━━
+ Base

+ input = 2
+ result = transform(#input)

━━━━━━━━━━━━━━━━━━━━
_ transform
    - value Int

value * 3

```

```output
6
21
```

```yumi
. f = Foo()
-- f.foo

f.bar = 50
-- f.foo

f.bar = 25
-- f.foo

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ foo = #bar * 2
+ bar = 42
```

```output
84
100
50
```

```yumi crash
. thing = Thing()
. name = thing.#name
-- name

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ prefix = ""

────────────────────
+ get name

"bob" + prefix
```

```output
Exception: Tracked getters are not supported; track concrete state instead
<no file:3>
    . name = thing.#name
             ^^^^^^^^^^^
```

### A tracked read in a lazy property's value is reported, as the value is worked out once.

```yumi crash
. f = Foo()
-- f.later

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ a = 1
+ later := #a * 3
```

```output
Exception: A lazy property is worked out once, when it is first read, so it cannot follow "#a"; write "=" in place of ":=" to keep it live, or read "a" without "#"
<no file:9>
    + later := #a * 3
               ^^
```

### A tracked read of each item of a pipeline that a call in a derived value works out is followed: the value runs again as an item changes and as items are added and removed, and no longer for an item removed.

```yumi
. b = Box()
-- b.waiting
b.items[0].is-pending = false
-- b.waiting
. added = Item()
b.items += added
-- b.waiting
added.is-pending = false
-- b.waiting
. removed = b.items[1]
b.items.remove(1)
-- b.waiting
removed.is-pending = false
-- b.waiting

━━━━━━━━━━━━━━━━━━━━
+ Item

+ is-pending = true

━━━━━━━━━━━━━━━━━━━━
+ Box

+ items = [Item(), Item(), Item()]
+ waiting = noted(tally(each in #items when each.#is-pending))

━━━━━━━━━━━━━━━━━━━━
+ do noted Int
    - n Int

-- "ran $n"
= n

━━━━━━━━━━━━━━━━━━━━
+ do tally'T
    - stream <T>

. total = 0
for item in stream
    total += 1
= total
```

```output
ran 3
3
ran 2
2
ran 3
3
ran 2
2
ran 1
1
1
```

### The items a pipeline reads in the value of an object made while another value runs are followed by that object's value alone: the other value runs again only for what it reads itself.

```yumi
. o = Outer()
-- o.made.waiting
o.made.items[0].is-pending = false
-- o.made.waiting
o.flag = true
-- o.made.waiting

━━━━━━━━━━━━━━━━━━━━
+ Item

+ is-pending = true

━━━━━━━━━━━━━━━━━━━━
+ Box

+ items = [Item(), Item()]
+ waiting = noted("box", tally(each in #items when each.#is-pending))

━━━━━━━━━━━━━━━━━━━━
+ Outer

+ flag = false
+ made = built(#flag)

━━━━━━━━━━━━━━━━━━━━
+ do built Box
    - flag Bool

-- "outer ran $flag"
= Box()

━━━━━━━━━━━━━━━━━━━━
+ do noted Int
    - what String
    - n Int

-- "$what ran $n"
= n

━━━━━━━━━━━━━━━━━━━━
+ do tally'T
    - stream <T>

. total = 0
for item in stream
    total += 1
= total
```

```output
outer ran false
box ran 2
2
box ran 1
1
outer ran true
box ran 2
2
```

### A tracked read of a list for each item of a pipeline follows the changes made to each list in place.

```yumi
. b = Box()
-- b.filled
b.groups[1].members += 3
-- b.filled
b.groups[1].members.clear()
-- b.filled

━━━━━━━━━━━━━━━━━━━━
+ Group

+ members = List'Int()

━━━━━━━━━━━━━━━━━━━━
+ Box

+ groups = [Group(), Group()]
+ filled = tally(each in #groups when each.#members.length > 0)

━━━━━━━━━━━━━━━━━━━━
+ do tally'T
    - stream <T>

. total = 0
for item in stream
    total += 1
= total
```

```output
0
1
0
```

### A rule that works out a pipeline follows a tracked read of each item, running as the count of them changes.

```yumi
. b = Box()
b.items[0].is-pending = false
b.items[1].is-pending = false
b.items[0].is-pending = true

━━━━━━━━━━━━━━━━━━━━
+ Item

+ is-pending = true

━━━━━━━━━━━━━━━━━━━━
+ Box

+ items = [Item(), Item()]

────────────────────
whenever tally(each in #items when each.#is-pending)

-- "waiting $waiting"

.. waiting = tally(each in items when each.is-pending)

━━━━━━━━━━━━━━━━━━━━
+ do tally'T
    - stream <T>

. total = 0
for item in stream
    total += 1
= total
```

```output
waiting 1
waiting 0
waiting 1
```

### A tracked read of each item of a pipeline that is the whole value is reported: the pipeline is worked out after the value, where nothing follows it.

```yumi crash
. b = Box()
-- b.waiting

━━━━━━━━━━━━━━━━━━━━
+ Item

+ is-pending = true

━━━━━━━━━━━━━━━━━━━━
+ Box

+ items = [Item()]
+ waiting = (each in #items when each.#is-pending)
```

```output
Exception: "#is-pending" is read for each item of a pipeline, which is followed only where the pipeline is given to a call in a live value, as in count(each in #items when each.#is-done); write "is-pending" to read it plainly
<no file:14>
    + waiting = (each in #items when each.#is-pending)
                                     ^^^^^^^^^^^^^^^^
```

### A pipeline given to a function that hands it back, as a generator does, is followed where what it hands back is given to a function that works it through in the same value: the reads happen in that call. One whose loop works it through follows it directly.

```yumi
. b = Bag()
-- "${b.latest} ${b.counted}"
b.items[1].n = 7
-- "${b.latest} ${b.counted}"
b.items[0].n = 0
-- "${b.latest} ${b.counted}"

━━━━━━━━━━━━━━━━━━━━
+ Bag

+ items = [Item(), Item()]
+ latest = last-of(pass(each.n in #items when each.#n > 1)) ?? 0
+ counted = tally(later(each in #items when each.#n > 1))

━━━━━━━━━━━━━━━━━━━━
+ Item

+ n = 2

━━━━━━━━━━━━━━━━━━━━
+ do tally'T
    - stream <T>

. total = 0
for item in stream
    total += 1
= total

━━━━━━━━━━━━━━━━━━━━
+ do last-of'T T?
    - stream <T>

. found T?
for item in stream
    found = item
= found

━━━━━━━━━━━━━━━━━━━━
+ do pass'T
    - stream <T>

for item in stream
    <= item

━━━━━━━━━━━━━━━━━━━━
+ do later'T
    - stream <T>

= stream

━━━━━━━━━━━━━━━━━━━━
+ do hold
    - sink [Generator'Int]
    - stream Generator'Int

sink += stream
= 0
```

```output
2 2
7 2
7 1
```

### A pipeline given to a function that keeps it, rather than working it through as it is called, is reported where it reads an item with "#".

```yumi crash
. b = Bag()
-- b.kept

━━━━━━━━━━━━━━━━━━━━
+ Bag

+ items = [Item(), Item()]
+ sink [Generator'Int]
+ kept = hold(sink, each.n in #items when each.#n > 1)

━━━━━━━━━━━━━━━━━━━━
+ Item

+ n = 2

━━━━━━━━━━━━━━━━━━━━
+ do tally'T
    - stream <T>

. total = 0
for item in stream
    total += 1
= total

━━━━━━━━━━━━━━━━━━━━
+ do last-of'T T?
    - stream <T>

. found T?
for item in stream
    found = item
= found

━━━━━━━━━━━━━━━━━━━━
+ do pass'T
    - stream <T>

for item in stream
    <= item

━━━━━━━━━━━━━━━━━━━━
+ do later'T
    - stream <T>

= stream

━━━━━━━━━━━━━━━━━━━━
+ do hold
    - sink [Generator'Int]
    - stream Generator'Int

sink += stream
= 0
```

```output
Exception: "#n" is read for each item of a pipeline that "hold" keeps rather than working through as it is called, so the read would not be followed; give the pipeline to a function that works it through, as count does, or write "n" to read it plainly
<no file:10>
    + kept = hold(sink, each.n in #items when each.#n > 1)
                                              ^^^^^^^
```

### A pipeline that a function hands back is reported where nothing in the value works through what it hands back. A pipeline with no "#" read may be given to any function.

```yumi crash
. b = Bag()
-- b.kept

━━━━━━━━━━━━━━━━━━━━
+ Bag

+ items = [Item(), Item()]
+ sink [Generator'Int]
+ plain = hold(sink, each.n in items when each.n > 1)
+ kept = later(each.n in #items when each.#n > 1)

━━━━━━━━━━━━━━━━━━━━
+ Item

+ n = 2

━━━━━━━━━━━━━━━━━━━━
+ do tally'T
    - stream <T>

. total = 0
for item in stream
    total += 1
= total

━━━━━━━━━━━━━━━━━━━━
+ do last-of'T T?
    - stream <T>

. found T?
for item in stream
    found = item
= found

━━━━━━━━━━━━━━━━━━━━
+ do pass'T
    - stream <T>

for item in stream
    <= item

━━━━━━━━━━━━━━━━━━━━
+ do later'T
    - stream <T>

= stream

━━━━━━━━━━━━━━━━━━━━
+ do hold
    - sink [Generator'Int]
    - stream Generator'Int

sink += stream
= 0
```

```output
Exception: "#n" is read for each item of a pipeline that "later" hands back without working it through, and nothing works through what it hands back as the value is made; give that to a function that works it through, as count does, or write "n" to read it plainly
<no file:11>
    + kept = later(each.n in #items when each.#n > 1)
                                         ^^^^^^^
```

### A pipeline given to a method, or by name, or piped with "|>", is followed by the parameter it fills, wherever that parameter is in the list.

```yumi
. b = Bag()
-- "${b.by-method} ${b.by-default} ${b.by-pipe} ${b.by-named}"
b.items[0].n = 0
-- "${b.by-method} ${b.by-default} ${b.by-pipe} ${b.by-named}"

━━━━━━━━━━━━━━━━━━━━
+ Bag

+ items = [Item(), Item()]
+ counter = Counter()
+ by-method = counter.tally(10, each.n in #items when each.#n > 1)
+ by-default = scaled(0, each.n in #items when each.#n > 1)
+ by-pipe = (each.n in #items when each.#n > 1) |> piped(100)
+ by-named = scaled(0)
    • scale 3
    • stream each.n in #items when each.#n > 1

━━━━━━━━━━━━━━━━━━━━
+ Item

+ n = 2

━━━━━━━━━━━━━━━━━━━━
+ Counter

+ base = 1

────────────────────
+ do tally Int
    - scale Int
    - stream Generator'Int

. total = base
for item in stream
    total += scale
= total

━━━━━━━━━━━━━━━━━━━━
+ do scaled Int
    - base Int
    - stream Generator'Int
    - scale Int = 2

. total = base
for item in stream
    total += scale
= total

━━━━━━━━━━━━━━━━━━━━
+ do piped Int
    - scale Int
    - stream Generator'Int

. total = 0
for item in stream
    total += scale
= total
```

```output
21 4 200 6
11 2 100 3
```

```yumi
. foo = Foo()
-- foo.foo

foo.bar.count = 2

-- foo.foo

foo.bar = Bar(5)

-- foo.foo

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ foo = bar.#count * 10
+ bar = Bar(1)

━━━━━━━━━━━━━━━━━━━━
+ Bar

+ count = 1

────────────────────
+ This
    - count
```

```output
10
20
50
```

```yumi
+ bb = Bar(1)

. foo = Foo()
-- foo.foo
bb.count = 2
-- foo.foo
bb.count = 3
-- foo.foo

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ foo = bar.#count * 10
    .. bar = bb

━━━━━━━━━━━━━━━━━━━━
+ Bar

+ count = 1

────────────────────
+ This
    - count
```

```output
10
20
30
```

### Cross-class tracked setter dispatch is finalized after later reader compilation

```yumi
. writer = Writer()
writer.set(1)
. reader = Reader(writer)
-- reader.mirror
writer.set(2)
-- reader.mirror

━━━━━━━━━━━━━━━━━━━━
+ Writer

+ value = 0

────────────────────
+ do set
    - next Int

value = next

━━━━━━━━━━━━━━━━━━━━
+ Reader

+ writer Writer
+ mirror = writer.#value * 10

────────────────────
+ This
    - writer
```

```output
10
20
```

```yumi crash
+ bar = 42

. foo = Foo()

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ foo = #bar * 2
```

```output
Exception: Untrackable name
<no file:9>
    + foo = #bar * 2
            ^^^^
```

```yumi
. foo = Foo()
-- foo.foo

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ foo =
    . bar = 42
    #bar * 2
```

```output
84
```

```yumi
. foo = Foo()
foo.bar = 10
foo.bar = 5
foo.bar = 5
foo.bar = 10
foo.bar = 10
foo.bar = 5

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ bar = 0

────────────────────
when #bar < 10

-- "lesser"

────────────────────
when #bar >= 10

-- "greater"
```

```output
lesser
greater
lesser
greater
lesser
```

### whenever observes every value change without an initial fire

```yumi
. foo = Foo()
foo.bar = 1
foo.bar = 2
foo.bar = 2
foo.bar = 0
foo.bar = 0
foo.bar = 3

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ bar = 0

────────────────────
whenever #bar

-- "bar $bar"
```

```output
bar 1
bar 2
bar 0
bar 3
```

### whenever over an interface-typed property fires when the value switches concrete type

```yumi
. holder = ShapeHolder()
. round = RoundShape(1)
. square = SquareShape(2)
holder.shape = round
holder.shape = square
holder.shape = square
holder.shape = round
holder.shape = null

━━━━━━━━━━━━━━━━━━━━
+ ShapeKind is Interface

────────────────────
+ get label String

━━━━━━━━━━━━━━━━━━━━
+ RoundShape
    is ShapeKind

+ size I64

────────────────────
+ This
    - size

────────────────────
+ get label String

"round $size"

━━━━━━━━━━━━━━━━━━━━
+ SquareShape
    is ShapeKind

+ size I64

────────────────────
+ This
    - size

────────────────────
+ get label String

"square $size"

━━━━━━━━━━━━━━━━━━━━
+ ShapeHolder

+ shape ShapeKind?

────────────────────
whenever #shape

-- shape?.label ?? "none"
```

```output
round 1
square 2
round 1
none
```

### whenever over a list of interface values fires when an element switches concrete type, and equality elsewhere still refuses

```yumi
. holder = ShapeList()
holder.shapes = shapes-of(RoundShape(1))
holder.shapes = shapes-of(SquareShape(2))
holder.shapes = shapes-of(SquareShape(2))
holder.shapes = shapes-of(SquareShape(3))
holder.shapes = shapes-of(RoundShape(3))
mismatch(RoundShape(1), SquareShape(1))

━━━━━━━━━━━━━━━━━━━━
+ ShapeKind is Interface

────────────────────
+ get label String

━━━━━━━━━━━━━━━━━━━━
+ RoundShape is (size I64)
    is ShapeKind

────────────────────
+ get label String

"round $size"

━━━━━━━━━━━━━━━━━━━━
+ SquareShape is (size I64)
    is ShapeKind

────────────────────
+ get label String

"square $size"

━━━━━━━━━━━━━━━━━━━━
+ do shapes-of [ShapeKind]
    - shape ShapeKind

. shapes [ShapeKind]
shapes += shape
= shapes

━━━━━━━━━━━━━━━━━━━━
+ do mismatch
    - left ShapeKind
    - right ShapeKind

. attempt = do new
    -- left == right
catch failure
    . failure Panic? = failure
    if failure
        -- "$failure"
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ ShapeList

+ shapes [ShapeKind]

────────────────────
whenever #shapes

-- shapes[0].label
if shapes[0].label == "round 3"
    mismatch(RoundShape(1), SquareShape(1))
```

```output
round 1
square 2
square 3
round 3
Unsupported interface equality: concrete types differ
Unsupported interface equality: concrete types differ
```

### whenever over a value that holds an interface value fires when the held value switches concrete type

```yumi
. holder = FrameHolder()
holder.frame = Frame(RoundShape(1), 1)
holder.frame = Frame(SquareShape(2), 1)
holder.frame = Frame(SquareShape(2), 1)
holder.frame = Frame(RoundShape(2), 1)

━━━━━━━━━━━━━━━━━━━━
+ ShapeKind is Interface

────────────────────
+ get label String

━━━━━━━━━━━━━━━━━━━━
+ RoundShape is (size I64)
    is ShapeKind

────────────────────
+ get label String

"round $size"

━━━━━━━━━━━━━━━━━━━━
+ SquareShape is (size I64)
    is ShapeKind

────────────────────
+ get label String

"square $size"

━━━━━━━━━━━━━━━━━━━━
+ Frame is (inner ShapeKind, marker I64)
    is ShapeKind

────────────────────
+ get label String

"framed ${inner.label}"

━━━━━━━━━━━━━━━━━━━━
+ FrameHolder

+ frame ShapeKind?

────────────────────
whenever #frame

-- frame?.label ?? "none"
```

```output
framed round 1
framed square 2
framed round 2
```

### whenever value triggers require a tracked read

```yumi crash
. foo = Foo()
foo.bar = 1

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ bar = 0

────────────────────
whenever bar

-- "bar $bar"
```

```output
Exception: whenever value triggers must read at least one tracked property (like #bar)
<no file:11>
    whenever bar
             ^^^
```

## Dependencies

### Same-value primitive property stores keep the value but do not dispatch to live subscribers

```yumi
. foo = Foo()
-- foo.value
foo.value = 0
-- foo.value
foo.value = 1
-- foo.value
foo.value = 1
-- foo.value

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ value = 0

. changed < value

────────────────────
+ do changed

-- "changed", value
```

```output
0
0
(changed, 1)
1
1
```

### Primitive store gates compare floating-point bits rather than IEEE equality

```yumi
. foo = Foo()
foo.value = 0.0
foo.value = -0.0
foo.value = -0.0

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ value = 0.0

. changed < value

────────────────────
+ do changed

-- "changed"
```

```output
changed
```

### Property stores compare references by identity, not content equality

```yumi
. first = Thing("same")
. second = Thing("same")
. holder = Holder(first)
holder.thing = first
-- holder.thing.name
holder.thing = second
-- holder.thing.name
holder.thing = second
-- holder.thing.name

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ name String

────────────────────
+ This
    - name

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ thing Thing

. changed < thing

────────────────────
+ This
    - thing

────────────────────
+ do changed

-- "changed", thing.name
```

```output
same
(changed, same)
same
same
```

### Same-value primitive stores preserve tracked module-local memoization

```yumi
.. value = 1
.. generation = 0
.. observed = IntSnapshot(#value, generation)

-- observed.generation
generation = 1
value = 1
-- observed.generation
generation = 2
value = 2
-- observed.generation
generation = 3
value = 2
-- observed.generation

━━━━━━━━━━━━━━━━━━━━
_ IntSnapshot is (value Int, generation Int)
```

```output
0
0
2
2
```

### Tracked module-local reference stores use identity rather than content equality

```yumi
.. first = Thing("same")
.. second = Thing("same")
.. value = first
.. generation = 0
.. observed = ThingSnapshot(#value, generation)

-- observed.generation
generation = 1
value = first
-- observed.generation
generation = 2
value = second
-- observed.generation
generation = 3
value = second
-- observed.generation

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ name String

────────────────────
+ This
    - name

━━━━━━━━━━━━━━━━━━━━
_ ThingSnapshot is (value Thing, generation Int)
```

```output
0
0
2
2
```

```yumi
. foo = Foo()
-- foo.x
foo.x = 1
foo.x = 2
foo.x = 3

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ x = 0

. bar < x

────────────────────
+ do bar

-- "bar", x
```

```output
0
(bar, 1)
(bar, 2)
(bar, 3)
```

```yumi crash
. foo = Foo()

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ x = 0

. bar < x

────────────────────
+ do bar
    - nd

-- "bar", x
```

```output
Exception: A dependency calls "bar" with nothing, but it takes "nd"; give "nd" a default, or name a method that takes nothing
<no file:9>
    . bar < x
      ^^^
```

```yumi
. foo = Foo()
-- foo.x
foo.x = 1

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ x = 0

. bar < x

────────────────────
+ do bar
    - n = 0

-- "bar", x, n
```

```output
0
(bar, 1, 0)
```

```yumi
. foo = Bar()
-- foo.x
foo.x = 1
foo.x = 2
foo.x = 3

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ x = 0

. bar < x

────────────────────
+ do bar

-- "bar", x

━━━━━━━━━━━━━━━━━━━━
+ Bar is Foo
```

```output
0
(bar, 1)
(bar, 2)
(bar, 3)
```

```yumi crash
. foo = Foo()

━━━━━━━━━━━━━━━━━━━━
+ Foo

. bar < x

────────────────────
+ do bar

-- "bar", x
```

```output
Exception: Property "x" does not exist
<no file:7>
    . bar < x
    ^^^^^^^^^
```

```yumi crash
. foo = Foo()

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ x = 0

. bar < x
```

```output
Exception: Function "bar" does not exist
<no file:9>
    . bar < x
    ^^^^^^^^^
```

```yumi
.. foo = #bar
.. bar = 1
-- foo
bar = 2
-- foo
bar = 3
-- foo
```

```output
1
2
3
```

```yumi
.. foo = Thing()
.. bar = Thing()

foo.name = bar.#name
-- foo.name
bar.name = "tom"
-- foo.name
bar.name = "fred"
-- foo.name

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ name = "bob"
```

```output
bob
tom
fred
```

```yumi
.. foo = Thing()
.. bar = Thing()

foo.name = bar.#name
-- foo.echo
bar.name = "tom"
-- foo.echo
bar.name = "fred"
-- foo.echo

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ name = "bob"
+ echo = ""

────────────────────

echo = #name

────────────────────
+ do this += child Foo

content = child
```

```output
bob
tom
fred
```

Canonical ids, rather than the display name String, retain a stable tracker. Reading one
dependency twice in the same evaluation still emits one membership.

Reordering preserves both tracker objects and their new observation order.

Removing one dependency, adding another, and replacing the owner have exact membership effects.

Runtime names retain a String-equality fallback without colliding with compiler ids.

```yumi
/core/tracker

. owner-a = DependencyIdentityOwner()
. owner-b = DependencyIdentityOwner()
. first-subscribers [PropertySubscriber]
. second-subscribers [PropertySubscriber]
. third-subscribers [PropertySubscriber]
. previous [Tracker] = List'Tracker()
. next [Tracker] = List'Tracker()

. first = track-property-dependency(previous, next, "first", 1, first-subscribers, owner-a as Pointer)
. repeated = track-property-dependency(previous, next, "renamed-first", 1, first-subscribers, owner-a as Pointer)
if first as Pointer != repeated as Pointer || next.length != 1
    ?! Panic("canonical property identity did not deduplicate a repeated read")

. second = track-property-dependency(previous, next, "second", 2, second-subscribers, owner-a as Pointer)
previous = next
next = List'Tracker()

. reordered-second = track-property-dependency(previous, next, "second", 2, second-subscribers, owner-a as Pointer)
. reordered-first = track-property-dependency(previous, next, "first", 1, first-subscribers, owner-a as Pointer)
if reordered-second as Pointer != second as Pointer || reordered-first as Pointer != first as Pointer
    ?! Panic("reordered dependencies lost their canonical trackers")
if next.length != 2 || next[0] as Pointer != second as Pointer || next[1] as Pointer != first as Pointer
    ?! Panic("reordered dependency set did not preserve observation order")

previous = next
next = List'Tracker()

. retained-first = track-property-dependency(previous, next, "first", 1, first-subscribers, owner-a as Pointer)
. added-third = track-property-dependency(previous, next, "third", 3, third-subscribers, owner-a as Pointer)
if next.length != 2 || retained-first as Pointer != first as Pointer
    ?! Panic("added/removed dependency set did not retain its unchanged membership")
if added-third as Pointer == first as Pointer || added-third as Pointer == second as Pointer
    ?! Panic("new dependency reused an unrelated tracker")

previous = next
next = List'Tracker()
. replaced-owner = track-property-dependency(previous, next, "first", 1, first-subscribers, owner-b as Pointer)
if replaced-owner as Pointer == retained-first as Pointer || next.length != 1
    ?! Panic("replaced dependency owner retained the old owner's tracker")

previous = List'Tracker()
next = List'Tracker()
. dynamic-first = track-property-dependency(previous, next, "dynamic", first-subscribers, owner-a as Pointer)
previous = next
next = List'Tracker()
. dynamic-reused = track-property-dependency(previous, next, "dynamic", first-subscribers, owner-a as Pointer)
. dynamic-other = track-property-dependency(previous, next, "other", first-subscribers, owner-a as Pointer)
if dynamic-reused as Pointer != dynamic-first as Pointer || next.length != 2
    ?! Panic("dynamic property-name fallback did not preserve exact membership")
if dynamic-other as Pointer == dynamic-first as Pointer
    ?! Panic("different dynamic property names shared one tracker")

-- "property-dependency-identity-ok"

━━━━━━━━━━━━━━━━━━━━
+ DependencyIdentityOwner
```

```output
property-dependency-identity-ok
```

### Live actions compare heterogeneous tracker implementations by identity.

```yumi
/core/tracker

. owner = TrackerRefreshOwner()
. trackers [Tracker] = List'Tracker()
trackers += FirstRefreshTracker() as Tracker
. callback SubscriberCallback = => do
    do owner.value
. action = LiveAction(trackers, callback)
action()
action.close()
-- "heterogeneous-tracker-refresh-ok"

━━━━━━━━━━━━━━━━━━━━
+ FirstRefreshTracker
    is Tracker

────────────────────
+ do attach
    - action LiveAction

do action

────────────────────
+ do detach
    - action LiveAction

do action

━━━━━━━━━━━━━━━━━━━━
+ TrackerRefreshOwner

+ value = 0
```

```output
heterogeneous-tracker-refresh-ok
```

### Property subscriptions retain live actions weakly and release their side-payload lane with the observed object.

```yumi
/BxGC/BxGC.h

+ property-change-observer-reclaimed = false

. foo = Foo()
if C:brixGCHasObjectSidePayload(foo as Pointer, 0)
    -- "has"
C:brixGCClearObjectSidePayload(foo as Pointer, 0)
if !C:brixGCHasObjectSidePayload(foo as Pointer, 0)
    -- "cleared"

C:brixCollect()
do C:brixGCDrainCleanups()
. payloads-before = C:brixGCObjectSidePayloadCount(0)

-- exercise-weak-property-subscription()

for cycle in 0 .. < 3
    C:brixCollect()
    do C:brixGCDrainCleanups()

-- C:brixGCObjectSidePayloadCount(0) - payloads-before

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ bar = 1
+ doubled = #bar * 2

━━━━━━━━━━━━━━━━━━━━
+ do exercise-weak-property-subscription String

. source = WeakPropertySource()
. live = WeakPropertyObserver(source)
make-dead-property-observer(source)

C:brixCollect()
do C:brixGCDrainCleanups()
if !property-change-observer-reclaimed
    = "dead-observer-retained"

source.value = 9
if live.mirror != 18
    = "live-observer-not-updated"

= "weak-property-subscription-ok"

━━━━━━━━━━━━━━━━━━━━
+ do make-dead-property-observer
    - source WeakPropertySource

. observer = WeakPropertyObserver(source)
?! Panic("Unable to register dead property observer cleanup")
    if !C:brixGCRegisterCleanup(observer as Pointer, context as Pointer, callback)
.. callback = property-observer-did-reclaim as PropertyCleanupCallback
.. context = PropertyCleanupContext()
do observer.mirror
observer.close()

━━━━━━━━━━━━━━━━━━━━
+ PropertyCleanupContext

━━━━━━━━━━━━━━━━━━━━
+ PropertyCleanupCallback is (*C) Size
    - context Pointer

━━━━━━━━━━━━━━━━━━━━
+ do property-observer-did-reclaim Size
    - context Pointer

do context
property-change-observer-reclaimed = true
= 1

━━━━━━━━━━━━━━━━━━━━
+ WeakPropertySource

+ value = 1

━━━━━━━━━━━━━━━━━━━━
+ WeakPropertyObserver

+ source WeakPropertySource
+ mirror = source.#value * 2

────────────────────
+ This
    - source

────────────────────
+ do close
```

```output
has
cleared
weak-property-subscription-ok
0
```

```yumi
. source = Source()
source.next = 10
source.refresh()

. observer = Observer()
observer.source = source
-- "0:${observer.text}"

source.next = 20
source.refresh()
-- "1:${observer.text}"

source.value = 30
-- "2:${observer.text}"

━━━━━━━━━━━━━━━━━━━━
+ Source

+ value = 0
+ next = 0

────────────────────
+ do refresh

value = next

━━━━━━━━━━━━━━━━━━━━
+ Observer

+ source Source?
+ text = "${source?.#value ?? 0}"
```

```output
0:10
1:20
2:30
```

```yumi
. leaf = NestedOptionalLeaf()
. middle = NestedOptionalMiddle()
. root = NestedOptionalRoot()
. observer = NestedOptionalObserver()
-- observer.value

observer.root = root
-- observer.value

root.middle = middle
-- observer.value

middle.leaf = leaf
-- observer.value

leaf.value = 7
-- observer.value

middle.leaf = null
-- observer.value

━━━━━━━━━━━━━━━━━━━━
+ NestedOptionalLeaf

+ value = 1

━━━━━━━━━━━━━━━━━━━━
+ NestedOptionalMiddle

+ leaf NestedOptionalLeaf?

━━━━━━━━━━━━━━━━━━━━
+ NestedOptionalRoot

+ middle NestedOptionalMiddle?

━━━━━━━━━━━━━━━━━━━━
+ NestedOptionalObserver

+ root NestedOptionalRoot?
+ value = #root?.#middle?.#leaf?.#value ?? 0
```

```output
0
0
0
1
7
0
```

## Stream changes

```yumi
.. a = ##[1, 2, 3]
. started = a
yield-once()
-- a

━━━━━━━━━━━━━━━━━━━━
+ do yield-once

. done = Semaphore()
do new
    done.post()
done.wait()
```

```output
3
```

```yumi
.. foo = 42
. stream = <#foo>
-- stream
-- <- stream
foo = 52
-- <- stream
```

```output
[Object core/channel:AsyncBufferedChannel'core/number:I64]
42
52
```

```yumi
.. foo = 10
. stream = <#foo>
-- stream

do new
    for item in stream
        -- item

yield-once()
foo = 11
foo = 12
stream.close()

━━━━━━━━━━━━━━━━━━━━
+ do yield-once

. done = Semaphore()
do new
    done.post()
done.wait()
```

```output
[Object core/channel:AsyncBufferedChannel'core/number:I64]
10
11
12
```

A live owner retains the scalar entry after its action dies; the target clears and explicit
release removes the entry exactly.

A dead owner removes its entry even while the action remains live.

PropertyTracker detach must release the production scalar subscription immediately.

Compiler-generated markup may share one property dependency tracker between actions. Each
action needs its own scalar handle and indexed slot, and detaching one must leave the other live.

Detaching a live peer leaves an unrelated cleared action for dispatch to prune. Detaching the
cleared action's tracker after that prune must make the stale scalar release a clean no-op.

Every subscriber present when a property changes is queued before any of them runs, so a later
subscriber an earlier one detaches still runs once for that change.

A subscriber added during dispatch waits for the next generation, after every subscriber that
was present at dispatch start. The second dispatch preserves insertion order exactly.

A subscriber can remove itself. A dispatch made while a subscriber runs queues the subscribers
it reaches behind those already queued, once each, and a running subscriber runs again last.

A successful load during concurrent mark shades a target that was otherwise reachable only
through the non-marking registry.

Owner and action dying in the same cycle removes the entry without a surviving scalar handle.

```yumi
/BxGC/BxGC.h
/BxGC/BxGCTest.h
/core/tracker

C:brixGCSetHeapGoal(1073741824)
C:brixGCInit()
C:brixCollect()
. baseline = C:brixGCTestOwnerWeakRegistrationCount()

. live-owner = OwnerWeakProbe(1)
. dead-action-handle = owner-weak-register-dead-action(live-owner)
C:brixCollect()
if C:brixGCOwnerWeakLoad(dead-action-handle)
    ?! Panic("dead action survived owner-weak collection")
if C:brixGCTestOwnerWeakRegistrationCount() != baseline + 1
    ?! Panic("cleared owner-weak entry count mismatch")
if live-owner.value != 1
    ?! Panic("live owner did not survive collection")
if !C:brixGCReleaseOwnerWeak(dead-action-handle)
    ?! Panic("unable to release cleared owner-weak entry")
if C:brixGCTestOwnerWeakRegistrationCount() != baseline
    ?! Panic("cleared owner-weak entry survived release")

. live-action = LiveAction(owner-weak-empty-trackers)
. dead-owner-handle = owner-weak-register-dead-owner(live-action)
C:brixCollect()
if C:brixGCOwnerWeakLoad(dead-owner-handle)
    ?! Panic("owner-reclaimed scalar handle remained loadable")
if C:brixGCTestOwnerWeakRegistrationCount() != baseline
    ?! Panic("owner-reclaimed scalar handle remained registered")
do live-action.is-closed

. detach-owner = OwnerWeakProbe(2)
. detach-subscribers [PropertySubscriber]
. detach-tracker = PropertyTracker("value", detach-subscribers)
detach-tracker.owner = detach-owner as Pointer
. detach-action = LiveAction(owner-weak-empty-trackers)
detach-tracker.attach(detach-action)
if C:brixGCTestOwnerWeakRegistrationCount() != baseline + 1
    ?! Panic("property tracker attach did not register one scalar handle")
detach-tracker.detach(detach-action)
if C:brixGCTestOwnerWeakRegistrationCount() != baseline
    ?! Panic("explicit property tracker detach leaked its scalar handle")
detach-tracker.detach(detach-action)
. detach-active = owner-weak-active-subscriber-count(detach-subscribers)
if C:brixGCTestOwnerWeakRegistrationCount() != baseline || detach-active
    ?! Panic("repeated property tracker detach changed the registry")

. shared-owner = OwnerWeakProbe(9)
. shared-subscribers [PropertySubscriber]
. shared-tracker = PropertyTracker("value", shared-subscribers)
shared-tracker.owner = shared-owner as Pointer
. shared-first-action = LiveAction(owner-weak-empty-trackers)
. shared-second-action = LiveAction(owner-weak-empty-trackers)
shared-tracker.attach(shared-first-action)
shared-tracker.attach(shared-second-action)
. shared-active = owner-weak-active-subscriber-count(shared-subscribers)
if C:brixGCTestOwnerWeakRegistrationCount() != baseline + 2 || shared-active != 2
    ?! Panic("shared property tracker did not register both actions")
shared-tracker.detach(shared-first-action)
shared-active = owner-weak-active-subscriber-count(shared-subscribers)
if C:brixGCTestOwnerWeakRegistrationCount() != baseline + 1 || shared-active != 1
    ?! Panic("shared property tracker detached the wrong action")
shared-tracker.detach(shared-second-action)
shared-active = owner-weak-active-subscriber-count(shared-subscribers)
if C:brixGCTestOwnerWeakRegistrationCount() != baseline || shared-active
    ?! Panic("shared property tracker leaked its final action")

. prune-owner = OwnerWeakProbe(3)
. prune-subscribers [PropertySubscriber]
. pruned-tracker = owner-weak-attach-dead-action(prune-owner, prune-subscribers)
. prune-live-tracker = PropertyTracker("value", prune-subscribers)
prune-live-tracker.owner = prune-owner as Pointer
. prune-live-action = LiveAction(owner-weak-empty-trackers)
prune-live-tracker.attach(prune-live-action)
C:brixCollect()
if C:brixGCTestOwnerWeakRegistrationCount() != baseline + 2
    ?! Panic("dead action was removed before the prune fixture")
prune-live-tracker.detach(prune-live-action)
. prune-active = owner-weak-active-subscriber-count(prune-subscribers)
if C:brixGCTestOwnerWeakRegistrationCount() != baseline + 1 || prune-active != 1
    ?! Panic("property tracker detach reaped an unrelated cleared action")
property-dispatch(prune-subscribers, "value")
prune-active = owner-weak-active-subscriber-count(prune-subscribers)
if C:brixGCTestOwnerWeakRegistrationCount() != baseline || prune-active
    ?! Panic("first property dispatch did not prune the cleared scalar handle")
pruned-tracker.detach(prune-live-action)
prune-active = owner-weak-active-subscriber-count(prune-subscribers)
if C:brixGCTestOwnerWeakRegistrationCount() != baseline || prune-active
    ?! Panic("detach after dispatch prune changed the empty registry")
property-dispatch(prune-subscribers, "value")
prune-active = owner-weak-active-subscriber-count(prune-subscribers)
if C:brixGCTestOwnerWeakRegistrationCount() != baseline || prune-active
    ?! Panic("repeated property dispatch changed the empty registry")

. reentrant-owner = OwnerWeakProbe(8)
. reentrant-subscribers [PropertySubscriber]
. reentrant-probe = OwnerWeakDispatchProbe()
. reentrant-first-tracker = PropertyTracker("value", reentrant-subscribers)
reentrant-first-tracker.owner = reentrant-owner as Pointer
. reentrant-later-tracker = PropertyTracker("value", reentrant-subscribers)
reentrant-later-tracker.owner = reentrant-owner as Pointer
. reentrant-first-action = LiveAction(reentrant-probe.detach-later)
. reentrant-later-action = LiveAction(reentrant-probe.run-later)
reentrant-probe.later-tracker = reentrant-later-tracker
reentrant-probe.later-action = reentrant-later-action
reentrant-first-tracker.attach(reentrant-first-action)
reentrant-later-tracker.attach(reentrant-later-action)
property-dispatch(reentrant-subscribers, "value")
if reentrant-probe.later-runs != 1
    ?! Panic("a subscriber queued for a change did not run once")
. reentrant-active = owner-weak-active-subscriber-count(reentrant-subscribers)
if C:brixGCTestOwnerWeakRegistrationCount() != baseline + 1 || reentrant-active != 1
    ?! Panic("reentrant dispatch left the wrong scalar subscriptions")
reentrant-first-tracker.detach(reentrant-first-action)
reentrant-active = owner-weak-active-subscriber-count(reentrant-subscribers)
if C:brixGCTestOwnerWeakRegistrationCount() != baseline || reentrant-active
    ?! Panic("reentrant dispatch fixture leaked a scalar subscription")

. add-owner = OwnerWeakProbe(10)
. add-subscribers [PropertySubscriber]
. add-probe = OwnerWeakDispatchProbe()
. add-first-tracker = PropertyTracker("value", add-subscribers)
add-first-tracker.owner = add-owner as Pointer
. add-existing-tracker = PropertyTracker("value", add-subscribers)
add-existing-tracker.owner = add-owner as Pointer
. add-late-tracker = PropertyTracker("value", add-subscribers)
add-late-tracker.owner = add-owner as Pointer
. add-first-action = LiveAction(add-probe.attach-late)
. add-existing-action = LiveAction(add-probe.run-existing)
. add-late-action = LiveAction(add-probe.run-added)
add-probe.later-tracker = add-late-tracker
add-probe.later-action = add-late-action
add-first-tracker.attach(add-first-action)
add-existing-tracker.attach(add-existing-action)
property-dispatch(add-subscribers, "value")
if add-probe.order != "fe"
    ?! Panic("subscriber added during dispatch ran in the starting generation")
property-dispatch(add-subscribers, "value")
if add-probe.order != "fefel"
    ?! Panic("next-generation subscriber order was not deterministic")
add-first-tracker.detach(add-first-action)
add-existing-tracker.detach(add-existing-action)
add-late-tracker.detach(add-late-action)

. self-owner = OwnerWeakProbe(11)
. self-subscribers [PropertySubscriber]
. self-probe = OwnerWeakDispatchProbe()
. self-tracker = PropertyTracker("value", self-subscribers)
self-tracker.owner = self-owner as Pointer
. self-action = LiveAction(self-probe.remove-self)
self-probe.self-tracker = self-tracker
self-probe.self-action = self-action
self-tracker.attach(self-action)
property-dispatch(self-subscribers, "value")
property-dispatch(self-subscribers, "value")
if self-probe.self-runs != 1
    ?! Panic("self-removing subscriber ran after detaching itself")

. nested-owner = OwnerWeakProbe(12)
. nested-subscribers [PropertySubscriber]
. nested-probe = OwnerWeakDispatchProbe()
. nested-first-tracker = PropertyTracker("value", nested-subscribers)
nested-first-tracker.owner = nested-owner as Pointer
. nested-second-tracker = PropertyTracker("value", nested-subscribers)
nested-second-tracker.owner = nested-owner as Pointer
. nested-first-action = LiveAction(nested-probe.run-nested-first)
. nested-second-action = LiveAction(nested-probe.run-nested-second)
nested-probe.nested-subscribers = nested-subscribers
nested-first-tracker.attach(nested-first-action)
nested-second-tracker.attach(nested-second-action)
property-dispatch(nested-subscribers, "value")
if nested-probe.order != "fsf"
    ?! Panic("nested property dispatch corrupted subscriber order")
nested-first-tracker.detach(nested-first-action)
nested-second-tracker.detach(nested-second-action)

. mark-owner = OwnerWeakProbe(4)
. mark-target OwnerWeakProbe? = OwnerWeakProbe(5)
. mark-target-pointer = mark-target! as Pointer
. mark-handle = C:brixGCRegisterOwnerWeak(mark-owner as Pointer, mark-target-pointer)
C:brixCollect()
mark-target = null
C:brixGCStartConcurrentMark()
. loaded-target = C:brixGCOwnerWeakLoad(mark-handle)
if loaded-target != mark-target-pointer || !C:brixGCTestObjectMarked(mark-target-pointer)
    ?! Panic("mark-phase owner-weak load did not shade its target")
if mark-owner.value != 4
    ?! Panic("mark-phase owner did not survive collection")
C:brixGCFinishConcurrentMark()
C:brixGCFinishAutomaticCycle()
if !C:brixGCReleaseOwnerWeak(mark-handle)
    ?! Panic("unable to release mark-phase owner-weak handle")
if C:brixGCTestOwnerWeakRegistrationCount() != baseline
    ?! Panic("mark-phase owner-weak handle survived release")

. same-cycle-handle = owner-weak-register-dead-pair()
C:brixCollect()
if C:brixGCOwnerWeakLoad(same-cycle-handle)
    ?! Panic("same-cycle owner/action handle remained loadable")
if C:brixGCTestOwnerWeakRegistrationCount() != baseline
    ?! Panic("same-cycle owner/action handle remained registered")

-- "owner-weak-property-ok"

━━━━━━━━━━━━━━━━━━━━
+ OwnerWeakProbe

+ value U64

────────────────────
+ This
    - value

━━━━━━━━━━━━━━━━━━━━
+ do owner-weak-empty-trackers [Tracker]

= List'Tracker()

━━━━━━━━━━━━━━━━━━━━
+ do owner-weak-register-dead-action U64
    - owner OwnerWeakProbe

. action = LiveAction(owner-weak-empty-trackers)
= C:brixGCRegisterOwnerWeak(owner as Pointer, action as Pointer)

━━━━━━━━━━━━━━━━━━━━
+ do owner-weak-register-dead-owner U64
    - action LiveAction

. owner = OwnerWeakProbe(6)
= C:brixGCRegisterOwnerWeak(owner as Pointer, action as Pointer)

━━━━━━━━━━━━━━━━━━━━
+ do owner-weak-attach-dead-action PropertyTracker
    - owner OwnerWeakProbe
    - subscribers [PropertySubscriber]

. tracker = PropertyTracker("value", subscribers)
tracker.owner = owner as Pointer
. action = LiveAction(owner-weak-empty-trackers)
tracker.attach(action)
= tracker

━━━━━━━━━━━━━━━━━━━━
+ OwnerWeakDispatchProbe

+ later-tracker PropertyTracker?
+ later-action LiveAction?
+ later-runs Size = 0
+ self-tracker PropertyTracker?
+ self-action LiveAction?
+ self-runs Size = 0
+ nested-subscribers [PropertySubscriber]?
+ is-nested = false
+ order = ""

────────────────────
+ do detach-later [Tracker]

if later-tracker && later-action
    later-tracker.detach(later-action)
= List'Tracker()

────────────────────
+ do run-later [Tracker]

later-runs += 1
= List'Tracker()

────────────────────
+ do attach-late

order += "f"
if later-tracker && later-action && order == "f"
    later-tracker.attach(later-action)

────────────────────
+ do run-existing

order += "e"

────────────────────
+ do run-added

order += "l"

────────────────────
+ do remove-self

self-runs += 1
if self-tracker && self-action
    self-tracker.detach(self-action)

────────────────────
+ do run-nested-first

order += "f"
if !is-nested && nested-subscribers
    is-nested = true
    property-dispatch(nested-subscribers, "value")

────────────────────
+ do run-nested-second

order += "s"

━━━━━━━━━━━━━━━━━━━━
+ do owner-weak-register-dead-pair U64

. owner = OwnerWeakProbe(7)
. action = LiveAction(owner-weak-empty-trackers)
= C:brixGCRegisterOwnerWeak(owner as Pointer, action as Pointer)

━━━━━━━━━━━━━━━━━━━━
+ do owner-weak-active-subscriber-count Size
    - subscribers [PropertySubscriber]

. count Size = 0
for subscriber in subscribers
    if subscriber.slot && subscriber.handle
        count += 1
= count
```

```output
owner-weak-property-ok
```

```yumi crash
/BxGC/BxGC.h
/core/tracker

C:brixGCSetHeapGoal(1073741824)
C:brixGCInit()
. subscribers [PropertySubscriber]
. action = LiveAction(owner-weak-attach-guard-empty-trackers)
. tracker = PropertyTracker("value", subscribers)
tracker.owner = action as Pointer
tracker.attach(action)
tracker.attach(action)

━━━━━━━━━━━━━━━━━━━━
+ do owner-weak-attach-guard-empty-trackers [Tracker]

= List'Tracker()
```

### Derived properties and property handlers follow writes to the object's own properties, and stop when the object closes

```yumi
. f = Foo()
-- f.double
f.bar = 50
-- f.double
-- f.label
f.bar = 25
-- f.double
-- f.label
-- f.changes
f.close()
f.bar = 7
-- f.double
-- f.changes

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ bar = 42
+ double = #bar * 2
+ label = "bar is ${#bar} and double is ${#double}"
+ changes = 0

. bar-changed < bar

────────────────────
+ do bar-changed

changes += 1

────────────────────
+ do close
```

```output
84
100
bar is 50 and double is 100
50
bar is 25 and double is 50
2
50
2
```

### A derived property declared after the property that reads it still follows its own dependencies, whether the earlier property reads it tracked or untracked

```yumi
. tracked = Tracked()
-- "${tracked.double} ${tracked.label}"
tracked.bar = 5
-- "${tracked.double} ${tracked.label}"

. plain = Plain()
-- "${plain.double} ${plain.label}"
plain.bar = 5
-- "${plain.double} ${plain.label}"

━━━━━━━━━━━━━━━━━━━━
+ Tracked

+ bar = 1
+ label = "double is ${#double}"
+ double = #bar * 2

━━━━━━━━━━━━━━━━━━━━
+ Plain

+ bar = 1
+ label = "double is $double"
+ double = #bar * 2
```

```output
2 double is 2
10 double is 10
2 double is 2
10 double is 2
```

### A subclass default for an inherited property can read the subclass's own derived properties, and both keep following their dependencies

```yumi
. item = Scaled()
-- "${item.double} ${item.total}"
item.bar = 5
-- "${item.double} ${item.total}"

━━━━━━━━━━━━━━━━━━━━
+ Totalled

+ total = 0

━━━━━━━━━━━━━━━━━━━━
+ Scaled is Totalled

+ bar = 1
+ double = #bar * 2
+ total = #double + 1
```

```output
2 3
10 11
```

### A handler of a property reads the derived properties it feeds already updated, on every write, and a derived property no rule reads is evaluated again only when read

```yumi
. foo = Foo()
. watcher = Watcher(foo)
-- "${watcher.seen} ${"|".join(foo.log)}"

foo.bar = 5
-- "${watcher.seen} ${"|".join(foo.log)}"

foo.bar = 7
-- "${watcher.seen} ${"|".join(foo.log)}"

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ log [String] = List'String()
+ bar = 42
+ double = #bar * 2

. bar-changed < bar

────────────────────
+ do bar-changed

log += "bar:$bar/$double"

━━━━━━━━━━━━━━━━━━━━
+ Watcher

+ source Foo
+ seen = noted(source, source.#bar + source.#double)

────────────────────
+ This
    - source

━━━━━━━━━━━━━━━━━━━━
+ do noted Int
    - foo Foo
    - sum Int

foo.log += "seen:$sum"
= sum
```

```output
126 seen:126
15 seen:126|bar:5/10|seen:15
21 seen:126|bar:5/10|seen:15|bar:7/14|seen:21
```

### A lazy property another initializer reads during construction is constructed once: the object keeps the instance its other properties saw, tracked reads through it follow that instance, and a lazy property nothing reads stays unevaluated

```yumi
. panel = Panel()
-- "$made ${panel.title}"
panel.session.current = "one"
-- "$made ${panel.title} ${panel.first as Pointer == panel.session as Pointer}"
panel.session.current = "two"
-- "$made ${panel.title}"

. idle = Idle()
-- "$made"
idle.session.current = "used"
-- "$made ${idle.session.current}"

+ made = 0

━━━━━━━━━━━━━━━━━━━━
+ Session

+ current = "none"

────────────────────
+ This

made += 1

━━━━━━━━━━━━━━━━━━━━
+ Panel

+ session := Session()
+ first Session = session
+ title = session.#current

━━━━━━━━━━━━━━━━━━━━
+ Idle

+ session := Session()
+ title = "idle"
```

```output
1 none
1 one true
1 two
1
2 used
```

### A lazy property read under a condition, or by another lazy property, is still one instance

```yumi
. shown = Panel(true)
shown.session.current = "one"
-- "$made ${shown.title}"

. quiet = Panel(false)
-- "${quiet.title}"
quiet.session.current = "two"
. count = made
quiet.uses = true
-- "${quiet.title} ${made == count}"

. chain = Chain()
chain.inner.current = "three"
-- "${chain.seen} ${chain.outer.inner as Pointer == chain.inner as Pointer}"

+ made = 0

━━━━━━━━━━━━━━━━━━━━
+ Session

+ current = "none"

────────────────────
+ This

made += 1

━━━━━━━━━━━━━━━━━━━━
+ Wrapper

+ inner Session

────────────────────
+ This
    - inner

━━━━━━━━━━━━━━━━━━━━
+ Panel

+ uses Bool
+ session := Session()
+ title = if #uses then session.#current else "off"

────────────────────
+ This
    - uses

━━━━━━━━━━━━━━━━━━━━
+ Chain

+ inner := Session()
+ outer := Wrapper(inner)
+ seen = outer.inner.#current
```

```output
1 one
off
two true
three true
```

```yumi
.. name = "bob"
. foo = Foo()

foo.title = #name
-- foo.title
name = "tom"
-- foo.title
foo.close()
name = "fred"
-- foo.title

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ title = ""

────────────────────
+ do close
```

```output
bob
tom
tom
```

### a when rule stores to a property of its object

```yumi
. counter = Counter()
counter.bump(3)
counter.bump(4)
-- "count ${counter.count} seen ${counter.seen}"

━━━━━━━━━━━━━━━━━━━━
+ Counter

+ count = 0
+ seen = 0

────────────────────
when #count > 5

seen += 1

────────────────────
+ do bump
    - value Int

count += value
```

```output
count 7 seen 1
```

### A tracked optional local set to nothing and back gives again what reads it.

```yumi
.. value Int? = 3
. holder = Holder()
holder.text = "v=${#value}"
-- holder.text
value = null
-- holder.text
value = 7
-- holder.text

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ text = ""
```

```output
v=3
v=unknown
v=7
```

### A return in the body of a `whenever` rule of a class leaves only the body, and the rule runs again on the next change.

```yumi
. foo = Foo()
foo.bar = "x"
foo.bar = null
foo.bar = "y"
-- foo.count

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ bar String? = null
+ count = 0

────────────────────
whenever #bar

=? if bar == null
count += 1
-- "changed ${bar ?? "-"}"
```

```output
changed x
changed y
2
```

### A return in the body of a `when` rule of a class leaves only the body.

```yumi
. foo = Foo()
foo.bar = 3
foo.bar = 12
foo.bar = 1
foo.bar = 20
-- foo.count

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ bar = 0
+ count = 0

────────────────────
when #bar >= 10

=? if bar > 15
count += 1
-- "greater $bar"
```

```output
greater 12
1
```

### A `whenever` rule of a base class on a property a class below gives a live value runs each time that value changes.

```yumi
. sub = Sub()
-- sub.runs
sub.message = "a"
-- sub.runs
sub.message = "b"
-- sub.runs
sub.message = "c"
-- sub.runs

━━━━━━━━━━━━━━━━━━━━
+ Base

+ text String = ""
+ runs = 0

────────────────────
whenever #text

runs += 1

━━━━━━━━━━━━━━━━━━━━
+ Sub is Base

+ message = "start"
+ text = "${#message}!"
```

```output
0
1
2
3
```

### A `whenever` rule of a class on a property it gives a new value runs where the property is stored through the class above that declares it.

```yumi
. sub = Sub()
. base Base = sub
-- sub.runs
base.text = "a"
-- sub.runs
base.set("b")
-- sub.runs

━━━━━━━━━━━━━━━━━━━━
+ Base

+ text String = ""

────────────────────
+ do set
    - value String

text = value

━━━━━━━━━━━━━━━━━━━━
+ Sub is Base

+ text = "init"
+ runs = 0

────────────────────
whenever #text

runs += 1
```

```output
0
1
2
```

### A store to a property a later read tracks, inside a test of another property, where nothing after it reads what the test found.

```yumi
. panel = Panel()
panel.reveal(1)
panel.reveal(2)
-- panel.describe()

━━━━━━━━━━━━━━━━━━━━
+ Step

+ id U64
+ is-revealed = false
+ is-shown = #is-revealed

────────────────────
+ This
    - id

━━━━━━━━━━━━━━━━━━━━
+ Panel

_ revealed Step?
_ steps = [Step(1), Step(2)]

────────────────────
+ do reveal
    - id U64

if revealed
    revealed.is-revealed = false
revealed = null
for step in steps
    if step.id == id
        step.is-revealed = true
        revealed = step

────────────────────
+ describe String

"${steps[0].is-revealed} ${steps[1].is-revealed}"
```

```output
false true
```

### A `close` that returns a value closes the live actions the object owns as it returns, at each return.

```yumi
. source = Source()
. view = View(source)
source.count = 2
-- view.doubled
-- view.close()
source.count = 5
-- view.doubled
-- view.close()

━━━━━━━━━━━━━━━━━━━━
+ Source

+ count = 1

━━━━━━━━━━━━━━━━━━━━
+ View

+ source Source
+ doubled = #source.#count * 2
+ closings = 0

────────────────────
+ This
    - source

────────────────────
+ do close Bool

closings += 1
=? false if closings > 1
= true
```

```output
4
true
4
false
```

### A `whenever` rule whose body loops runs again on each change of what it reads.

```yumi
. bar = Bar()
bar.preview = Preview(["a", "b"], ["b", "c"])
-- bar.seen
bar.preview = Preview(["x"], ["y"])
-- bar.seen

━━━━━━━━━━━━━━━━━━━━
+ Preview

+ active [String]
+ empty [String]

────────────────────
+ This
    - active
    - empty

━━━━━━━━━━━━━━━━━━━━
+ Bar

+ preview Preview? = null
+ seen = ""

────────────────────
whenever #preview

. current = preview
if current
    . open = List'String(current.active)
    for key in current.empty
        if !(open has key)
            open += key
    seen = ",".join(open)
```

```output
a,b,c
x,y
```

### A `when` rule whose body loops runs it first as the object is made, and again on each change.

```yumi
. bar = Bar()
-- bar.seen
bar.on = false
bar.on = true
-- bar.seen

━━━━━━━━━━━━━━━━━━━━
+ Bar

+ on = true
+ seen = 0

────────────────────
when #on

for step in 0 .. < 3
    seen += step
```

```output
3
6
```

### A tracked value that reads a later property on some paths alone gives that property its value there, and both values follow what they read.

```yumi
. thing = Thing()
-- thing.shows
thing.flag = false
-- thing.shows
thing.ready = false
thing.flag = true
-- thing.shows
thing.ready = true
-- thing.shows

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ ready = true
+ shows = #ready && #hovered
+ flag = true

_ hovered = #flag
```

```output
true
false
false
true
```

### An error a tracked value raises as it is evaluated again goes to unhandled-error, and the property keeps the value it had.

```yumi
unhandled-error = :: error => do
    -- "handled ${error.message}"

. thing = Thing()
-- thing.ok
thing.n = 3
-- thing.ok
thing.n = 9
-- thing.ok
thing.n = 4
-- thing.ok

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ n I64 = 0
+ ok = #n > 0 && check(#n)

━━━━━━━━━━━━━━━━━━━━
_ do check Bool
    - n I64

if n > 5
    ?! Error("too big $n")
= n != 3
```

```output
false
false
handled too big 9
false
true
```

### A narrowed read stored after a store to a watched property stores what the narrowed read holds.

```yumi
. doc = Doc()
. watcher = Watcher(doc)
. closed [Closed]
closed += Closed(1, Jump(4, "x"))
closed += Closed(2, null)
reopen(closed, doc)
-- doc.jump?.text ?? "none"
reopen(closed, doc)
-- doc.jump?.text ?? "none"
-- watcher.seen

━━━━━━━━━━━━━━━━━━━━
_ do reopen
    - closed [Closed]
    - doc Doc

while closed.length > 0
    . entry = closed.pop()
    if entry.index > 0
        if entry.jump
            doc.wants = true
            doc.jump = entry.jump
        =

━━━━━━━━━━━━━━━━━━━━
_ Closed is (index Int, jump Jump?)

━━━━━━━━━━━━━━━━━━━━
+ Jump is Tuple

+ line Size
+ text String

━━━━━━━━━━━━━━━━━━━━
+ Doc

+ wants = false
+ jump Jump?

━━━━━━━━━━━━━━━━━━━━
+ Watcher

+ doc Doc
+ seen = 0

────────────────────
+ This
    - doc

────────────────────
whenever doc.#jump

seen += 1

────────────────────
whenever doc.#wants

seen += 10
```

```output
none
x
11
```

### A narrowed read stored after a store to a property its function tracked before stores what the narrowed read holds.

```yumi
. doc = Doc()
reopen(Closed(1, Jump(4, "x")), doc)
-- doc.jump?.text ?? "none"
-- doc.wants

━━━━━━━━━━━━━━━━━━━━
_ do reopen
    - entry Closed
    - doc Doc

. wanted = doc.wants
if entry.jump
    doc.wants = !wanted
    doc.jump = entry.jump

━━━━━━━━━━━━━━━━━━━━
_ Closed is (index Int, jump Jump?)

━━━━━━━━━━━━━━━━━━━━
+ Jump is Tuple

+ line Size
+ text String

━━━━━━━━━━━━━━━━━━━━
+ Doc

+ wants = false
+ is-wanting = #wants
+ jump Jump?
```

```output
x
true
```

### A property a test found to hold a value, forced after a store to a property a value tracks, is what it holds.

```yumi
. holder = Holder()
holder.options = Options()
holder.run()

━━━━━━━━━━━━━━━━━━━━
+ Options

+ name = "x"

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ options Options?
+ flag = false
+ shown = "${#flag}"

────────────────────
+ do run

if options
    flag = true
    -- options!.name
```

```output
x
```

### A store to a tracked property runs the rules that track it before the next statement, so a property a test found to hold a value is tested again where it is forced after the store.

```yumi crash
. holder = Holder()
holder.options = Options()
holder.run()

━━━━━━━━━━━━━━━━━━━━
+ Options

+ name = "x"

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ options Options?
+ flag = false

────────────────────
+ do run

if options
    flag = true
    -- options!.name

────────────────────
whenever #flag

options = null
```

```stderr
Unhandled panic: Optional'[Object __source__:Options] is null
<no file:22>

Task 1

Stack trace:
  0  __source__:Holder:run no file:22:8
  1  __source__:main no file:4:1
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

### A store to a tracked property ends what is known of every property, so a property a test found to hold a value is optional again after it.

```yumi crash
. holder = Holder()
holder.options = Options()
holder.run()

━━━━━━━━━━━━━━━━━━━━
+ Options

+ name = "x"

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ options Options?
+ flag = false
+ shown = "${#flag}"

────────────────────
+ do run

if options
    flag = true
    -- options.name
```

```output
Exception: Property "name" is read from Options?, which may be null; unwrap it first with ! or ?.
<no file:23>
        -- options.name
           ^^^^^^^^^^^^
```

### A `when` rule of a class runs its body as the object is made and on every change while its test holds.

```yumi
. meter = Meter()
meter.level = 5
meter.level = 0
meter.level = 7

━━━━━━━━━━━━━━━━━━━━
+ Meter

+ level = 2

────────────────────
when #level > 0

-- "level $level"
```

```output
level 2
level 5
level 7
```

### A tracked lazy local is evaluated, and starts watching what it read, at whichever of its reads runs first, even where a read written before it does not run.

```yumi
.. foo = #bar * 10
.. bar = 1
. flag = true
if !flag
    -- "branch one $foo"
if flag
    -- "branch two $foo"
-- "plain $foo"
bar = 2
-- "after change $foo"
```

```output
branch two 10
plain 10
after change 20
```

### A `when` rule that leaves early on its first run, as the object is made, leaves only its body.

```yumi
. foo = Foo()
-- "made ${foo.name}"
foo.bar = 5
foo.bar = 7

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ bar = 0
+ name = "none"

────────────────────
+ This

name = "foo"

────────────────────
when #bar < 6

=? if bar == 0
-- "lesser ${bar}"

────────────────────
when #bar > 6

-- "greater ${bar}"
```

```output
made foo
lesser 5
greater 7
```

### A rule whose nested function reads the object and a local the rule declares, where the constructor runs it first and where what it tests changes.

```yumi
. holder = Holder()
-- ", ".join(holder.first)
-- ", ".join(holder.later)
holder.context = "b"
-- ", ".join(holder.first)
-- ", ".join(holder.later)
holder.hidden = "z"
holder.context = "c"
-- ", ".join(holder.first)
-- ", ".join(holder.later)

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ context = "a"
+ hidden = "x"
+ first = List'String()
+ later = List'String("none")
_ names = List'String("x", "y", "z")

────────────────────
when #context != ""

. suffix = context
first = List'String("$each$suffix" in names when each != hidden)

────────────────────
whenever #context

. suffix = context
later = List'String("$each$suffix" in names when each != hidden)
```

```output
ya, za
none
yb, zb
yb, zb
xc, yc
xc, yc
```

### A derived property that reads another one never shows a rule a mix of old and new values.

```yumi
. f = Foo()
f.a = 2
-- "|".join(f.seen)

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ a = 1
+ total = #a + #b
+ b = #a * 10
+ seen [String] = List'String()

────────────────────
whenever #total

seen += "total:$total"
```

```output
total:22
```

### A derived property nothing subscribes to is evaluated again only when read after changes, once, from fresh values.

```yumi
. f = Foo()
-- f.d
f.a = 2
f.a = 3
-- f.d

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ a = 1
+ b = #a * 10
+ c = #a * 100
+ d = summed(#b, #c)

━━━━━━━━━━━━━━━━━━━━
+ do summed Int
    - left Int
    - right Int

-- "run $left $right"
= left + right
```

```output
run 10 100
110
run 30 300
330
```

### A class may give a property of its base a value that reads tracked state, read through the base.

```yumi
. s = Sub()
. b Base = s
-- b.x
s.y = 5
-- b.x

━━━━━━━━━━━━━━━━━━━━
+ Base

+ x = 1

━━━━━━━━━━━━━━━━━━━━
+ Sub is Base

+ y = 2
+ x = #y * 10
```

```output
20
50
```

### A value written to a derived property stays until what it reads changes.

```yumi
. f = Foo()
-- f.total
f.total = 99
-- f.total
f.a = 3
-- f.total

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ a = 1
+ total = #a * 10
```

```output
10
99
30
```

### A stream of a derived value gives nothing when the value is evaluated again to the same value.

```yumi
. f = Foo()
. values = <f.#positive>
-- <- values
f.a = 2
f.a = -1
-- <- values
values.close()

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ a = 1
+ positive = #a > 0
```

```output
true
false
```

### A derived value that makes another one it reads stale before it subscribes still sees later changes.

```yumi
. f = Foo()
-- f.d
f.on = true
-- f.d
f.src.a = 7
-- f.d

━━━━━━━━━━━━━━━━━━━━
+ Source

+ a = 1

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ src = Source()
+ on = false
+ total = src.#a * 10
+ d = if #on then bumped(src, #total) else 0

━━━━━━━━━━━━━━━━━━━━
+ do bumped Int
    - source Source
    - value Int

if source.a == 1
    source.a = 5
= value
```

```output
0
10
70
```

### A rule that reads a list and a derived value of it sees them agree when the list changes.

```yumi
. f = Foo()
f.on = true
-- f.m
f.items += 7
-- "|".join(f.seen)

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ items = List'Int()
+ on = false
+ m = if #on then #items.length else 0
+ seen [String] = List'String()

────────────────────
whenever "${#items.length}:${#m}"

seen += "${items.length}:$m"
```

```output
0
1:1
```

### A derived property that settles a stale derived value it reads, and comes out the same, still runs its rules when what it reads changes again.

```yumi
. source = Source()
. middle = Middle(source)
. f = Foo(middle, "a")
source.pick("b")
source.pick("c")
source.pick("a")

━━━━━━━━━━━━━━━━━━━━
+ Source

+ revision U64
_ picked = "a"

────────────────────
+ do pick
    - id String

picked = id
revision += 1

────────────────────
+ do current String

= picked

━━━━━━━━━━━━━━━━━━━━
+ Middle

+ source Source
+ revision = #source.#revision

────────────────────
+ This
    - source

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ middle Middle
+ id String
+ is-picked = revised(middle.source.current(), middle.#revision) == #id

────────────────────
+ This
    - middle
    - id

────────────────────
whenever #is-picked

-- "$id picked $is-picked"

━━━━━━━━━━━━━━━━━━━━
_ revised String
    - value String
    - revision U64

value

.. ignored = revision
```

```output
a picked false
a picked true
```

### A dependency on a getter is rejected, as a getter is never stored to run it.

```yumi crash
. foo = Foo()

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ a = 1
+ twice => a * 2

. shown < twice

────────────────────
+ do shown

-- "shown"
```

```output
Exception: "twice" is a getter, which is never stored, so a dependency on it never runs; depend on the properties it reads
<no file:10>
    . shown < twice
              ^^^^^
```

### A method called with "#" before its name is rejected, as only a stored property is tracked.

```yumi crash
. c = Counter()
. h = Holder()
h.text = "${c.#size()}"

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ text = ""

━━━━━━━━━━━━━━━━━━━━
+ Counter

+ count = 1

────────────────────
+ size Int

count
```

```output
Exception: "size" is a method, so "#" before it tracks nothing; track the properties it reads instead
<no file:4>
    h.text = "${c.#size()}"
                ^
```

### A rule that returns a value with "=" is rejected, as a rule gives none.

```yumi crash
. f = Foo()
f.a = 1

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ a = 0

────────────────────
whenever #a

= 5
```

```output
Exception: A rule gives no value, so its body cannot return one with "="; end it early with "= alone"
<no file:13>
    = 5
    ^^^
```

### A tracked read of a struct's field is rejected, as a struct's values are copied.

```yumi crash
. h = Holder()
-- h.sum

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ p = Point(1, 2)
+ sum = p.#x + 1
```

```output
Exception: "#x" reads a field of Point, whose values are copied, so it cannot be tracked; track a property of a class
<no file:15>
    + sum = p.#x + 1
            ^^^^
```

### A tracked read of "this" is rejected, as only a property is tracked.

```yumi crash
. c = Counter()
-- c.mirror

━━━━━━━━━━━━━━━━━━━━
+ Counter

+ count = 1
+ mirror = "${#this}"
```

```output
Exception: "#this" tracks nothing, as only a property is tracked; track the properties the value reads, as in "#count"
<no file:9>
    + mirror = "${#this}"
                  ^^^^^
```

### A function written where a local is declared stores to it, and what reads the local tracked follows.

```yumi
. count = 0
. holder = Holder()
holder.text = "count ${#count}"
. bump = => do count += 1
bump()
-- holder.text
bump()
-- holder.text

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ text = ""
```

```output
count 1
count 2
```

### A function written in another, written where a local is declared, stores to the local.

```yumi
. word = "a"
. holder = Holder()
holder.text = "word ${#word}"
. outer = => do
    . inner = => do word = "b"
    inner()
outer()
-- holder.text

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ text = ""
```

```output
word b
```

### A function written before the read that tracks a local stores to it before the read and after.

```yumi
. count = 0
. bump = => do count += 1
bump()
. holder = Holder()
holder.text = "count ${#count}"
-- holder.text
bump()
-- holder.text

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ text = ""
```

```output
count 1
count 2
```
