# Markup

```yumi
. direct = MarkupDefaults()
. inherited = InheritedDefaults()
. overridden = OverriddenDefaults()
. configured = ConfiguredDefaults(900)
. fallback = ConfiguredDefaults(null)
. markup = OverriddenMarkupDefaults()
-- direct.width
-- inherited.width
-- overridden.width
-- configured.width
-- fallback.width
-- markup.width

━━━━━━━━━━━━━━━━━━━━
+ DefaultsBase

+ width = 640

━━━━━━━━━━━━━━━━━━━━
+ MarkupDefaults is DefaultsBase

────────────────────

<This
    width = 1120

━━━━━━━━━━━━━━━━━━━━
+ InheritedDefaults is MarkupDefaults

━━━━━━━━━━━━━━━━━━━━
+ OverriddenDefaults is MarkupDefaults

+ width = 800

━━━━━━━━━━━━━━━━━━━━
+ ConfiguredDefaults is MarkupDefaults

────────────────────
+ This
    - width I64?

━━━━━━━━━━━━━━━━━━━━
+ OverriddenMarkupDefaults is MarkupDefaults

────────────────────

<This
    width = 720
```

```output
1120
1120
800
900
1120
720
```

```yumi
./defaults

. value = Derived()
-- value.width

━━━━━━━━━━━━━━━━━━━━
+ Derived is MarkupDefaults

+ multiplier = 3
```

```yumi file=defaults.yumi

━━━━━━━━━━━━━━━━━━━━
+ DefaultsBase

+ width = 640

━━━━━━━━━━━━━━━━━━━━
+ MarkupDefaults is DefaultsBase

+ multiplier = 2

────────────────────

<This
    width = preferred-width() * multiplier

━━━━━━━━━━━━━━━━━━━━
_ preferred-width

400

```

```output
1200
```

```yumi
/core/markup
/core/tests/support/markup-fixtures

. root = IdentityQueryRoot()
markup-mount-root(root, root)
. owner = EqualQueryHost()
. first = EqualQueryHost()
. middle = DifferentQueryHost()
. last = EqualQueryHost()
markup-append(root, owner)
markup-append(root, DifferentQueryHost())
markup-append(owner, first)
markup-append(owner, middle)
markup-append(owner, last)
root.markup-batch-queue.checkpoint()

-- markup-query-first-descendant-host(owner) as Pointer == first as Pointer
-- markup-query-next-descendant-host(owner, first) as Pointer == middle as Pointer
-- markup-query-next-descendant-host(owner, middle) as Pointer == last as Pointer
-- !markup-query-next-descendant-host(owner, last)

━━━━━━━━━━━━━━━━━━━━
+ EqualQueryHost is Box

────────────────────
+ this == other EqualQueryHost

true

━━━━━━━━━━━━━━━━━━━━
+ DifferentQueryHost is Box

━━━━━━━━━━━━━━━━━━━━
+ IdentityQueryRoot is Box
    is MarkupRootElement

+ markup-batch-queue := MarkupBatchQueue(this)

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

. ignored = flush.applied
```

```output
true
true
true
true
```

```yumi
. x = foo()
-- x.a
-- x.contents

━━━━━━━━━━━━━━━━━━━━
+ do foo

= <Foo
    a = 10
    <Foo

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ a = 0
+ contents Foo?

────────────────────
+ do this += child Foo

contents = child
```

```output
10
[Object __source__:Foo]
```

### Descendant queries see targets declared in a module that has no markup syntax of its own.

```yumi
/core/markup
/core/tests/support/markup-fixtures
./lib/marker

<MarkerRoot root

root.markup-batch-queue.checkpoint()
-- root.markers.length

━━━━━━━━━━━━━━━━━━━━
+ MarkerRoot is Box
    is MarkupRootElement

+ markers Descendants'Marker
+ markup-batch-queue := MarkupBatchQueue(this)

────────────────────
+ This

markup-mount-root(this, this)

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

. ignored = flush.applied

────────────────────

<This
    <Marker
    <Box
        <Marker
```

```yumi file=lib/marker.yumi

/core/tests/support/markup-fixtures

━━━━━━━━━━━━━━━━━━━━
+ Marker is Box

```

```output
2
```

### Descendant queries stop at an explicit boundary when their owner is outside it, while a query owned by the boundary continues to observe its own structural subtree across target toggles.

```yumi
/core/markup
/core/tests/support/markup-fixtures

<BoundaryQueryRoot root

root.markup-batch-queue.checkpoint()
-- root.markers.length
-- boundary-marker-list(root.boundary.markers)

root.boundary.show-extra = true
root.markup-batch-queue.checkpoint()
-- root.markers.length
-- boundary-marker-list(root.boundary.markers)

root.boundary.show-extra = false
root.markup-batch-queue.checkpoint()
-- root.markers.length
-- boundary-marker-list(root.boundary.markers)

━━━━━━━━━━━━━━━━━━━━
+ BoundaryMarker is Box

+ text String

────────────────────
+ This
    - text

━━━━━━━━━━━━━━━━━━━━
+ BoundaryQueryHost is Box

+ show-extra Bool = false
+ markers Descendants'BoundaryMarker

────────────────────
+ get markup-query-boundary Bool

true

────────────────────

<This
    <BoundaryMarker = "steady"
    if #show-extra
        <BoundaryMarker = "extra"

━━━━━━━━━━━━━━━━━━━━
+ BoundaryQueryRoot is Box
    is MarkupRootElement

+ boundary BoundaryQueryHost
+ markers Descendants'BoundaryMarker
+ markup-batch-queue := MarkupBatchQueue(this)

────────────────────
+ This

markup-mount-root(this, this)

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

. ignored = flush.applied

────────────────────

<This
    <BoundaryQueryHost boundary

━━━━━━━━━━━━━━━━━━━━
+ do boundary-marker-list String
    - markers [BoundaryMarker]

= "|".join(each.text in markers)
```

```output
0
steady
0
steady|extra
0
steady
```

### A live owner keeps one markup attachment across ordinary unmount/remount. Dropping the mounted, parentless owner lets reclamation clear lane 3, and deterministic cleanup drain completes the detached cleanup token without firing a visible unmounted callback.

```yumi
/core/markup
/core/tests/support/markup-fixtures
/BxGC/BxGC.h

. lane = C:brixGCMarkupAttachmentsSidePayloadLane()
. root = MarkupCleanupRoot()
markup-mount-root(root, root)
root.markup-batch-queue.checkpoint()
. opening = C:brixGCObjectSidePayloadCount(lane)

do exercise-markup-owner-lifetime(root, lane, opening)
C:brixCollect()
do C:brixGCDrainCleanups()

-- "reclaimed=${C:brixGCObjectSidePayloadCount(lane) - opening}"

━━━━━━━━━━━━━━━━━━━━
+ do exercise-markup-owner-lifetime
    - root MarkupCleanupRoot
    - lane U64
    - opening U64

.. child MarkupCleanupChild? = MarkupCleanupChild()

markup-adopt(child!, null, root)
root.markup-batch-queue.checkpoint()
-- "mounted=${child!.events} lane=${C:brixGCObjectSidePayloadCount(lane) - opening}"

markup-release(child!)
root.markup-batch-queue.checkpoint()
-- "unmounted=${child!.events} lane=${C:brixGCObjectSidePayloadCount(lane) - opening}"

markup-adopt(child!, null, root)
root.markup-batch-queue.checkpoint()
-- "remounted=${child!.events} lane=${C:brixGCObjectSidePayloadCount(lane) - opening}"

child = null

━━━━━━━━━━━━━━━━━━━━
+ MarkupCleanupRoot is Box
    is MarkupRootElement

+ markup-batch-queue := MarkupBatchQueue(this)

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

━━━━━━━━━━━━━━━━━━━━
+ MarkupCleanupChild is Box

+ events String = ""

────────────────────
+ do mounted

events += "M"

────────────────────
+ do unmounted

events += "U"
```

```output
mounted=M lane=1
unmounted=MU lane=1
remounted=MUM lane=1
reclaimed=0
```

```yumi crash
<Foo
    "bar"

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ do this += child Foo
```

```output
Exception: Content not accepted
<no file:3>
        "bar"
        ^^^^^
```

```yumi
. foo =
    <Foo = "bar"

-- foo

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ content String

────────────────────
+ This
    - content

────────────────────
+ to String

"[Foo $content]"
```

```output
[Foo bar]
```

```yumi crash
<Foo
    b = 10

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ a = 0
```

```output
Exception: Property "b" not found; an assignment in an element's body sets the element's property. To assign a local, put it in a do row: "do b = …"
<no file:3>
        b = 10
        ^
```

```yumi crash
<Foo ""
    text = "x"

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ text String

────────────────────
+ This
    - text String
```

```output
Exception: Parameter "text" is given twice, by the element's header and by this attribute; give it once
<no file:3>
        text = "x"
        ^^^^^^^^^^
```

### An attribute that fills a constructor parameter an earlier attribute filled.

```yumi crash
<Foo
    text = "x"
    text = "y"

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ text String

────────────────────
+ This
    - text String
```

```output
Exception: Parameter "text" is given twice, by an attribute above and by this one; remove one
<no file:4>
        text = "y"
        ^^^^^^^^^^
```

```yumi
<Foo foo
-- foo

━━━━━━━━━━━━━━━━━━━━
+ Foo
```

```output
[Object __source__:Foo]
```

```yumi
<Foo foo
    -- "inner $foo"
    <Bar bar
        -- "inner $bar"
-- foo
-- bar

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ do this += child

━━━━━━━━━━━━━━━━━━━━
+ Bar
```

```output
inner [Object __source__:Foo]
inner [Object __source__:Bar]
[Object __source__:Foo]
[Object __source__:Bar]
```

### Names can be referenced out of order due to lazy declarations

```yumi
<Foo
    -- foo
    <Foo foo

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ do this += child
```

```output
[Object __source__:Foo]
```

```yumi
<Window win
    for text in ["foo", "bar", "baz"]
        <Label = text

-- win.contents

━━━━━━━━━━━━━━━━━━━━
+ Window

+ contents [Label]

────────────────────
+ do this += child Label

contents += child

━━━━━━━━━━━━━━━━━━━━
+ Label

+ text String

────────────────────
+ This
    - text

────────────────────
+ to String

"[Label $text]"
```

```output
[Label foo], [Label bar], [Label baz]
```

```yumi
<Window win
    if true
        <Label = "a"
    if false
        <Label = "b"
    else if true
        <Label = "c"

-- win.contents

━━━━━━━━━━━━━━━━━━━━
+ Window

+ contents [Label]

────────────────────
+ do this += child Label

contents += child

━━━━━━━━━━━━━━━━━━━━
+ Label

+ text String

────────────────────
+ This
    - text

────────────────────
+ to String

"[Label $text]"
```

```output
[Label a], [Label c]
```

```yumi
-- test()

━━━━━━━━━━━━━━━━━━━━
+ do test

= <Window

━━━━━━━━━━━━━━━━━━━━
+ Window
```

```output
[Object __source__:Window]
```

```yumi
<Window
    -- "a"
    do finally
        -- "b"
    -- "c"

━━━━━━━━━━━━━━━━━━━━
+ Window
```

```output
a
c
b
```

```yumi
<Window window
    a = b
.. b = 42

-- window.a

━━━━━━━━━━━━━━━━━━━━
+ Window

+ a = 0
```

```output
42
```

### Lazy variables accessible throughout the subtree (and not reevaluated obviously)

```yumi
<Foo foo
    .. b = bar()
    <Foo
        a = b + 1

-- foo.child?.a

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ a = 0
+ child Foo?

────────────────────
+ do this += foo

child = foo

━━━━━━━━━━━━━━━━━━━━
+ do bar

-- "called"
= 42
```

```output
called
43
```

```yumi
<Window
    foo()

━━━━━━━━━━━━━━━━━━━━
+ Window

━━━━━━━━━━━━━━━━━━━━
+ do foo

-- "foo"
```

```output
foo
```

```yumi
do new
    <Window
        foo()
catch failure
    -- failure
━━━━━━━━━━━━━━━━━━━━
+ Window

━━━━━━━━━━━━━━━━━━━━
+ do foo

?! Panic("fail")
```

```output
fail
```

```yumi crash
-- test()

━━━━━━━━━━━━━━━━━━━━
+ test

<Window
    <Window
        <Window
            = 42

━━━━━━━━━━━━━━━━━━━━
+ Window

────────────────────
+ do this += child
```

```output
Exception: Function "test" is not declared with do, so its body is one expression and cannot return with "="; write "+ do test"
<no file:10>
                = 42
                ^^^^
```

```yumi
<Window
    <Window
    do
        <Window

━━━━━━━━━━━━━━━━━━━━
+ Window

────────────────────
+ do this += child

-- child
```

```output
[Object __source__:Window]
```

```yumi
-- <List'Int
    1
    2
    3
```

```output
1, 2, 3
```

```yumi
-- <Map'String'Int
    "a", 1
    "b", 2
    "c", 3
```

```output
[b = 2, a = 1, c = 3]
```

### Just verifying that closures work inside markup

```yumi
. clicks = 0

<Foo foo
    for event on click
        clicks += 1

foo.click <- 1
-- clicks

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ click AsyncChannel'Int
```

```output
0
```

### Class content

```yumi
<Window win

-- win.child

━━━━━━━━━━━━━━━━━━━━
+ Window is Box

────────────────────

<Box box

━━━━━━━━━━━━━━━━━━━━
+ Box

+ child Box?

────────────────────
+ do this += box Box

child = box
```

```output
[Object __source__:Box]
```

### Class-level subscriptions can reference mounted class-content ids

```yumi
. exported = ExportedIdHost()
exported.target.emits <- 1
yield-once()
yield-once()
-- "exported=${exported.count}"

. hidden = PrivateIdHost()
hidden.child.emits <- 1
yield-once()
yield-once()
-- "private=${hidden.count}"

━━━━━━━━━━━━━━━━━━━━
+ do yield-once

. done = Semaphore()
do new
    done.post()
done.wait()

━━━━━━━━━━━━━━━━━━━━
+ EventNode

+ emits AsyncChannel'Int

━━━━━━━━━━━━━━━━━━━━
+ Box

+ child := EventNode()

────────────────────
+ do this += child EventNode

this.child = child
= child

━━━━━━━━━━━━━━━━━━━━
+ ExportedIdHost is Box

+ count = 0
+ target := EventNode()

────────────────────

<EventNode target

for n on target.emits
    count += n

━━━━━━━━━━━━━━━━━━━━
+ PrivateIdHost is Box

+ count = 0

────────────────────

<EventNode node

for n on node.emits
    count += n
```

```output
exported=1
private=1
```

### Closure binds to content

```yumi
<Window

━━━━━━━━━━━━━━━━━━━━
+ Window is Box

────────────────────

<Box
    <Box b
        cb = => foo()

        -- (b.cb!)()

────────────────────
+ This

`foo()

────────────────────
+ do foo

-- "foo"
-- child
= 42

━━━━━━━━━━━━━━━━━━━━
+ Box

+ child Box?
+ cb Getter'Int?

────────────────────
+ do this += box Box

child = box
```

```output
foo
unknown
42
```

### Class content

```yumi
<Window

━━━━━━━━━━━━━━━━━━━━
+ Window is Box

────────────────────

<Box
    <Box
        b = submit()
        -- submit()

────────────────────
+ This

`foo()

────────────────────
+ do submit

-- "submit", child

= 42

━━━━━━━━━━━━━━━━━━━━
+ Box

+ child Box?
+ b Int

────────────────────
+ do this += box Box

child = box
```

```output
(submit, unknown)
(submit, unknown)
42
```

```yumi
<Foo
    bar = Bar("Joe")

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ bar Bar

━━━━━━━━━━━━━━━━━━━━
+ Bar

+ name String

────────────────────
+ This

-- "create Bar"

────────────────────
+ This
    - name

-- "create Bar $name"
```

```output
create Bar Joe
```

```yumi
<Foo foo
    b = Bar("Joe")

-- foo.a

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ a = "$b.name Blow"
+ b Bar

━━━━━━━━━━━━━━━━━━━━
+ Bar

+ name String

────────────────────
+ This
    - name
```

```output
Joe Blow
```

### A named element of top-level markup is made where it is written, so it reads the locals declared above it.

```yumi
-- "before"
. label = "local label"
<Thing thing
    name = label
-- thing.name

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ name = "unnamed"

────────────────────
+ This

-- "constructed"
```

```output
before
constructed
local label
```

```yumi
<Foo a = 1
    <Foo b = 2
        <Foo c = 3

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ a = 0
+ contents Foo?

────────────────────
+ This
    - a

-- "create $a"

────────────────────
+ do this += child Foo

contents = child
```

```output
create 1
create 2
create 3
```

### Nested named markup updates matching properties after mount

```yumi
<Demo demo

demo.child.value = 7
. mounted = demo.first?.first as Child
-- mounted == demo.child
-- mounted.value
-- demo.child.value

━━━━━━━━━━━━━━━━━━━━
+ Box

+ first Box?

────────────────────
+ do this += child Box

if !first
    first = child
= child

━━━━━━━━━━━━━━━━━━━━
+ Demo is Box

+ child Child

────────────────────

<Box
    <Child child

━━━━━━━━━━━━━━━━━━━━
+ Child is Box

+ value = 0
```

```output
true
7
7
```

### A named markup element nested in an element whose class has a member of the same name keeps its own identity instead of resolving to that member.

```yumi
/core/tests/support/markup-fixtures

<Demo demo
-- demo.rows
demo.items += "c"
-- demo.rows
-- demo.holder.list == null

━━━━━━━━━━━━━━━━━━━━
+ Holder is Box

────────────────────
+ get list Box?

null

━━━━━━━━━━━━━━━━━━━━
+ Demo is Box

+ items [String]

────────────────────
+ This

items = List'String(["a", "b"])

────────────────────

<This
    <Holder holder
        <Box list
            for item in #items
                <TextBox = item

────────────────────
+ do get rows

. count = 0
for child in list.%first-child!.children
    count += 1
= count
```

```output
2
3
true
```

### Class-content markup ids are private properties that methods can read and assign through

```yumi
<Demo demo

demo.update("hi")
-- demo.read()
. panel = demo.first!.first as Panel
-- panel.read()

━━━━━━━━━━━━━━━━━━━━
+ Box

+ first Box?

────────────────────
+ do this += child Box

if !first
    first = child
= child

━━━━━━━━━━━━━━━━━━━━
+ Label is Box

+ text = ""

━━━━━━━━━━━━━━━━━━━━
+ Panel is Box

────────────────────

<Label status
    text = "inner"

────────────────────
+ read String

status.text

━━━━━━━━━━━━━━━━━━━━
+ Demo is Box

────────────────────

<Box
    <Panel
        <Label status
            text = "outer"
        -- status.text

────────────────────
+ do update
    - text String

status.text = text

────────────────────
+ read String

status.text
```

```output
outer
hi
inner
```

### Class-content markup ids construct each named element once, in reference order

```yumi
-- Demo().read()

━━━━━━━━━━━━━━━━━━━━
+ Box

────────────────────
+ do this += child Box

= child

━━━━━━━━━━━━━━━━━━━━
+ Label is Box

+ text = ""

────────────────────
+ This

-- "constructed Label"

━━━━━━━━━━━━━━━━━━━━
+ Echo is Box

+ source Label?

━━━━━━━━━━━━━━━━━━━━
+ Demo is Box

────────────────────

<Box
    <Echo echo
        source = status
    <Label status
        text = "named"

────────────────────
+ read String

"${status.text} ${echo.source?.text ?? "none"}"
```

```output
constructed Label
named named
```

### Property initializers that read class-content markup ids initialize after the markup constructs them

```yumi
-- Demo().early

━━━━━━━━━━━━━━━━━━━━
+ Box

────────────────────
+ do this += child Box

= child

━━━━━━━━━━━━━━━━━━━━
+ Label is Box

+ text = ""

━━━━━━━━━━━━━━━━━━━━
+ Demo is Box

+ early = status.text

────────────────────

<Label status
    text = "named"
```

```output
named
```

### Deferred property initializers construct each named element once

```yumi
. demo = Demo()
-- demo.selected ?? "none"

━━━━━━━━━━━━━━━━━━━━
+ Box

────────────────────
+ do this += child Box

= child

━━━━━━━━━━━━━━━━━━━━
+ Label is Box

+ text = ""

────────────────────
+ This

-- "constructed Label"

━━━━━━━━━━━━━━━━━━━━
+ Demo is Box

+ selected String? = status.text

────────────────────

<Label status
    text = "named"
```

```output
constructed Label
named
```

### Tracked readers of a deferred property follow the named element it reads

```yumi
. demo = Demo()
-- demo.shout
demo.rename()
-- demo.current ?? "none"
-- demo.shout

━━━━━━━━━━━━━━━━━━━━
+ Box

────────────────────
+ do this += child Box

= child

━━━━━━━━━━━━━━━━━━━━
+ Label is Box

+ text = ""

━━━━━━━━━━━━━━━━━━━━
+ Demo is Box

+ current String? = status.#text
+ shout = "${#current ?? "none"}!"

────────────────────

<Label status
    text = "named"

────────────────────
+ do rename

status.text = "renamed"
```

```output
named!
renamed
renamed!
```

### Initializers that read a deferred property initialize after it

```yumi
-- Demo().echo

━━━━━━━━━━━━━━━━━━━━
+ Box

────────────────────
+ do this += child Box

= child

━━━━━━━━━━━━━━━━━━━━
+ Label is Box

+ text = ""

━━━━━━━━━━━━━━━━━━━━
+ Demo is Box

+ label String = status.text
+ echo = label

────────────────────

<Label status
    text = "named"
```

```output
named
```

### Deferred property readers initialize after their sources regardless of declaration order

```yumi
. demo = Demo()
-- demo.loud
-- demo.echo

━━━━━━━━━━━━━━━━━━━━
+ Box

────────────────────
+ do this += child Box

= child

━━━━━━━━━━━━━━━━━━━━
+ Label is Box

+ text = ""

━━━━━━━━━━━━━━━━━━━━
+ Demo is Box

+ loud = "${echo}!"
+ echo = label
+ label String = status.text

────────────────────

<Label status
    text = "named"
```

```output
named!
named
```

### Markup bindings that read a deferred property follow it once it is given

```yumi
-- Demo().echo.text

━━━━━━━━━━━━━━━━━━━━
+ Box

────────────────────
+ do this += child Box

= child

━━━━━━━━━━━━━━━━━━━━
+ Label is Box

+ text = ""

━━━━━━━━━━━━━━━━━━━━
+ Demo is Box

+ shout = "${status.#text}!"

────────────────────

<Label echo
    text = #shout
<Label status
    text = "named"
```

```output
named!
```

### A value with locals of its own gives the properties it reads their values first

```yumi
<Demo demo
    client = Client()
-- ", ".join(demo.names)

━━━━━━━━━━━━━━━━━━━━
+ Box

────────────────────
+ do this += child Box

= child

━━━━━━━━━━━━━━━━━━━━
+ Base is Box

+ names [String]

━━━━━━━━━━━━━━━━━━━━
+ Client

+ names = ["a", "b"]

━━━━━━━━━━━━━━━━━━━━
+ Demo is Base

+ client Client

────────────────────

<This
    names = List'String("<$each>" in client.names)
```

```output
<a>, <b>
```

### Properties that depend on each other through deferred properties are rejected

```yumi compile-error
-- Demo().label

━━━━━━━━━━━━━━━━━━━━
+ Box

────────────────────
+ do this += child Box

= child

━━━━━━━━━━━━━━━━━━━━
+ Label is Box

+ text = ""

━━━━━━━━━━━━━━━━━━━━
+ Demo is Box

+ label String = "${status.text}${echo}"
+ echo = label

────────────────────

<Label status
    text = "named"
```

```output
Exception: Property "label" depends on itself through properties that initialize after its markup
<no file:20>
    + label String = "${status.text}${echo}"
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Properties that initialize after their markup must have a type that has a value until then

```yumi compile-error
-- Demo().words.length

━━━━━━━━━━━━━━━━━━━━
+ Box

────────────────────
+ do this += child Box

= child

━━━━━━━━━━━━━━━━━━━━
+ Label is Box

+ text = ""

━━━━━━━━━━━━━━━━━━━━
+ Demo is Box

+ label String? = status.text
+ words = [label ?? ""]

────────────────────

<Label status
    text = "named"
```

```output
Exception: Property "words" initializes after its markup, so its type must be optional
<no file:21>
    + words = [label ?? ""]
    ^^^^^^^^^^^^^^^^^^^^^^^
```

### When rules that read a named markup element run after the markup constructs it

```yumi
. demo = Demo()
-- demo.runs
demo.status.text = ""
demo.status.text = "again"
-- demo.runs

━━━━━━━━━━━━━━━━━━━━
+ Box

────────────────────
+ do this += child Box

= child

━━━━━━━━━━━━━━━━━━━━
+ Label is Box

+ text = ""

━━━━━━━━━━━━━━━━━━━━
+ Demo is Box

+ runs = 0

────────────────────

<Label status
    text = "named"

────────────────────
when status.#text

runs += 1
```

```output
1
2
```

### Constructor bodies cannot read class-content markup ids before the markup constructs them

```yumi compile-error
. demo = Demo()

━━━━━━━━━━━━━━━━━━━━
+ Box

────────────────────
+ do this += child Box

= child

━━━━━━━━━━━━━━━━━━━━
+ Label is Box

+ text = ""

━━━━━━━━━━━━━━━━━━━━
+ Demo is Box

────────────────────

<Label status
    text = "named"

────────────────────
+ This

-- status.text
```

```output
Exception: Named markup element "status" is read before its markup constructs it
<no file:28>
    -- status.text
       ^^^^^^
```

### A class-content markup id assigns an inherited property instead of declaring its own

```yumi
-- Sub().read()

━━━━━━━━━━━━━━━━━━━━
+ Box

────────────────────
+ do this += child Box

= child

━━━━━━━━━━━━━━━━━━━━
+ Label is Box

+ text = ""

━━━━━━━━━━━━━━━━━━━━
+ Base is Box

+ status Label?

────────────────────
+ read String

status?.text ?? "none"

━━━━━━━━━━━━━━━━━━━━
+ Sub is Base

────────────────────

<Label status
    text = "sub"
```

```output
sub
```

### A class-content markup id cannot share its name with a property of the class that cannot hold the element, where the element's body would read the element for the property.

```yumi compile-error
/core/tests/support/markup-fixtures

. status = Status()
-- status.activity

━━━━━━━━━━━━━━━━━━━━
+ Status is Box

+ activity = "busy"

────────────────────

<Label activity
    text = #activity
```

```output
Exception: Named markup element "activity" clashes with property "activity", which holds String, not Label; rename the element
<no file:14>
    <Label activity
    ^^^^^^^^^^^^^^
```

### A class-content markup id in a branch of the markup cannot share its name with a property of the class that cannot hold the element.

```yumi compile-error
/core/tests/support/markup-fixtures

. status = Status()
-- status.activity

━━━━━━━━━━━━━━━━━━━━
+ Status is Box

+ busy = true
+ activity = "busy"

────────────────────

<This
    if #busy
        <Label activity
            text = #activity
    else
        <Label
            text = "idle"
```

```output
Exception: Named markup element "activity" clashes with property "activity", which holds String, not Label; rename the element
<no file:17>
            <Label activity
            ^^^^^^^^^^^^^^
```

### Property in content section

```yumi
<Thing foo
-- foo.bar
-- foo.baz

━━━━━━━━━━━━━━━━━━━━
+ Thing

────────────────────

+ bar = xxx
+ baz = this

.. xxx = 42
```

```output
42
[Object __source__:Thing]
```

### Broadcaster property in content section

```yumi
<Thing foo
for item on foo.bar
    -- item

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ a = 99

────────────────────

+ bar = "hi $a $y $x" <* x on xxx

.. xxx = [1, 2, 3]
.. y = false
```

```output
hi 99 false 1
hi 99 false 2
hi 99 false 3
```

### Class content

```yumi
-- Foo()

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ do this += content

-- content

────────────────────

<Bar

━━━━━━━━━━━━━━━━━━━━
+ Bar
```

```output
[Object __source__:Bar]
[Object __source__:Foo]
```

### Structural % reads are sidecar-backed and infer concrete MarkupParent child types.

```yumi
/core/markup

. root = Node("root")
. a = Node("a")
. b = Node("b")
. c = Node("c")

markup-append(root, a)
markup-append(root, b)
markup-append(a, c)

-- root.describe()
-- a.describe()
-- b.describe()
-- c.describe()

markup-release(root)

-- c.describe()

markup-remove(root, a)

-- root.describe()
-- a.describe()
-- c.describe()

━━━━━━━━━━━━━━━━━━━━
+ Node
    is MarkupParent'Node

+ name String

────────────────────
+ This
    - name

────────────────────
+ markup-content Node
    - child Node

child

────────────────────
+ do markup-attach-child
    - child Node
    - anchor Node?

────────────────────
+ do markup-detach-child
    - child Node

────────────────────
+ do describe String

. parent Node? = %parent
. first Node? = this.%first-child
. last Node? = %last-child
. previous Node? = this.%previous-sibling
. next Node? = %next-sibling

= "$name parent=${name-of(parent)} first=${name-of(first)} last=${name-of(last)} previous=${name-of(previous)} next=${name-of(next)}"

────────────────────
+ do name-of String
    - node Node?

=? node.name
    if node
= "-"
```

```output
root parent=- first=a last=b previous=- next=-
a parent=root first=c last=c previous=- next=b
b parent=root first=- last=- previous=a next=-
c parent=a first=- last=- previous=- next=-
c parent=a first=- last=- previous=- next=-
root parent=- first=b last=b previous=- next=-
a parent=- first=c last=c previous=- next=-
c parent=a first=- last=- previous=- next=-
```

### Mounted callbacks run after the committed structural tree is coherent.

```yumi
/core/markup

. log = MountLog()
. root = MountNode("root", log)
. parent = MountNode("parent", log)
. child = MountNode("child", log)

markup-append(root, parent)
markup-append(parent, child)
-- "before=${log.text}"

markup-mount-root(root, root)
-- log.text

log.clear()
. late = MountNode("late", log)
. late-child = MountNode("late-child", log)
markup-append(late, late-child)
markup-append(root, late)
-- log.text

log.clear()
. replacement = MountNode("replacement", log)
markup-replace(root, parent, replacement)
-- log.text

━━━━━━━━━━━━━━━━━━━━
+ MountLog

+ text = ""

────────────────────
+ do clear

text = ""

────────────────────
+ do record
    - line String

if text.length
    text += " | "
text += line

━━━━━━━━━━━━━━━━━━━━
+ MountNode
    is MarkupParent'MountNode
    is MarkupRootElement

+ name String
+ log MountLog
+ markup-batch-queue := MarkupBatchQueue(this)

────────────────────
+ This
    - name
    - log

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

. ignored = flush.applied

────────────────────
+ do markup-attach-child
    - child MountNode
    - anchor MountNode?

────────────────────
+ do markup-detach-child
    - child MountNode

────────────────────
+ do mounted

log.record(describe())

────────────────────
+ describe String

"$name parent=${name-of(%parent)} first=${name-of(%first-child)} next=${name-of(%next-sibling)} root=${markup-batch-root != null}"

────────────────────
+ do name-of String
    - node MountNode?

=? node.name
    if node
= "-"
```

```output
before=
root parent=- first=parent next=- root=true | parent parent=root first=child next=- root=true | child parent=parent first=- next=- root=true
late parent=root first=late-child next=- root=true | late-child parent=late first=- next=- root=true
replacement parent=root first=- next=late root=true
```

### A host with hook-shaped methods but no MarkupParent conformance is rejected at the mutation site; the structural hooks are an interface contract, not duck typing.

```yumi crash
/core/markup

. host = DuckNode()
markup-append(host, DuckNode())

━━━━━━━━━━━━━━━━━━━━
+ DuckNode
    is MarkupElement

────────────────────
+ do markup-attach-child
    - child DuckNode
    - anchor DuckNode?

────────────────────
+ do markup-detach-child
    - child DuckNode
```

```output
Exception: No function "markup-append" accepts (DuckNode, DuckNode); it is declared as "markup-append(parent Host, child Child)"
<no file:5>
    markup-append(host, DuckNode())
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A root <This> in class content configures existing properties as construction defaults and lowers its body as normal content.

```yumi
. plain = Card()
. child = ChildCard("Message")
<Card custom
    title = "Custom"

-- plain.title
-- custom.title
-- child.title
-- plain.first!
-- child.first!

━━━━━━━━━━━━━━━━━━━━
+ Box

+ first PanelLabel?

────────────────────
+ do this += child PanelLabel

first = child

━━━━━━━━━━━━━━━━━━━━
+ Label

━━━━━━━━━━━━━━━━━━━━
+ Card is Box

+ title = "Default"

────────────────────

<This
    <PanelLabel
        title = title

━━━━━━━━━━━━━━━━━━━━
+ PanelLabel is Label

+ title = "Panel Default"

────────────────────
+ to String

"[Label $title]"

━━━━━━━━━━━━━━━━━━━━
+ ChildCard is Card

+ message String
+ title = "Child"

────────────────────
+ This
    - message
```

```output
Default
Custom
Child
[Label Default]
[Label Child]
```

### A root <This> property default that reads tracked state stays live, like a declared default.

```yumi
. source = Source()
. card = Card(source)
. child = ChildCard(source)
-- card.label
-- card.declared
-- child.label
source.count = 5
-- card.label
-- card.declared
-- child.label

━━━━━━━━━━━━━━━━━━━━
+ Source

+ count = 1

━━━━━━━━━━━━━━━━━━━━
+ Box

━━━━━━━━━━━━━━━━━━━━
+ Card is Box

+ source Source
+ label Int
+ declared = source.#count * 10

────────────────────
+ This
    - source

────────────────────

<This
    label = source.#count * 10

━━━━━━━━━━━━━━━━━━━━
+ ChildCard is Card

+ scale = 100

────────────────────
+ This
    - source

────────────────────

<This
    label = source.#count * scale
```

```output
10
10
100
50
50
500
```

### A subclass default for a property overrides an inherited constructor argument's default, but an argument the caller passes still wins.

```yumi
. declared = DeclaredButton()
. rooted = RootedButton()
. given = RootedButton("Given")
<RootedButton named
    title = "Named"

-- declared.title
-- rooted.title
-- given.title
-- named.title

━━━━━━━━━━━━━━━━━━━━
+ Button

+ title = ""

────────────────────
+ This
    - title = ""

━━━━━━━━━━━━━━━━━━━━
+ DeclaredButton is Button

+ title = "Declared"

━━━━━━━━━━━━━━━━━━━━
+ RootedButton is Button

────────────────────

<This
    title = "Rooted"
```

```output
Declared
Rooted
Given
Named
```

### A lambda a line of a root <This> gives a property reads the object when it runs, as one the class's markup gives a child does: its members, its methods and the elements its markup names.

```yumi
. panel = Panel()
panel.fire()
panel.fire()
-- panel.seen

━━━━━━━━━━━━━━━━━━━━
+ Box

+ run Doer?

────────────────────
+ do this += child Note

────────────────────
+ do fire

. callback = run
if callback
    callback()

━━━━━━━━━━━━━━━━━━━━
+ Note

+ text = "note"

━━━━━━━━━━━━━━━━━━━━
+ Panel is Box

+ count = 0
+ seen = ""

────────────────────

<This
    run = => do bump(note.text)
    <Note note

────────────────────
+ do bump
    - text String

count += 1
seen = "$text $count"
```

```output
note 2
```

### An anonymous state machine may sit beside an explicit root <This> and runs after earlier markup.

```yumi
. card = Card()

-- card.first != null
-- card.initialized

━━━━━━━━━━━━━━━━━━━━
+ Box

+ first PanelLabel?

────────────────────
+ do this += child PanelLabel

first = child

━━━━━━━━━━━━━━━━━━━━
+ PanelLabel

━━━━━━━━━━━━━━━━━━━━
+ Card is Box

+ initialized = false

────────────────────

<This
    <PanelLabel

~> ready
    do
        initialized = first != null
```

```output
true
true
```

### An anonymous state machine may sit beside a root <This> with no lines.

```yumi
. knob = Knob()

-- knob.is-idle

━━━━━━━━━━━━━━━━━━━━
+ Knob

────────────────────

<This

~> is-idle
```

```output
true
```

### Root <This> must wrap all class content.

```yumi crash
<Card

━━━━━━━━━━━━━━━━━━━━
+ Card

────────────────────

<This
<Card
```

```output
Exception: Root <This> must wrap all class content
<no file:9>
    <This
    ^^^^^
```

### Root <This> assignments do not declare new properties.

```yumi crash
<Card

━━━━━━━━━━━━━━━━━━━━
+ Card

────────────────────

<This
    missing = 1
```

```output
Exception: Property "missing" not found
<no file:10>
        missing = 1
        ^^^^^^^
```

### Root <This> configures an existing object; it does not call a constructor.

```yumi crash
<Card

━━━━━━━━━━━━━━━━━━━━
+ Card

────────────────────

<This = 1
```

```output
Exception: Root <This> cannot pass constructor arguments
<no file:9>
    <This = 1
            ^
```

### Root <This> uses normal markup body rules: "when" belongs only inside style rules.

```yumi crash
<Card

━━━━━━━━━━━━━━━━━━━━
+ Card

+ is-on = false

────────────────────

<This
    when #is-on
        is-on = true
```

```output
Exception: "when" state branches are only allowed in style rule bodies
<no file:12>
        when #is-on
        ^^^^^^^^^^
```

### Markup list properties materialize heterogeneous union feeds once before initial assignment.

```yumi
<MarkupListHost host
    items =
        * NumberItem(7)
        * TextItem("seven")

-- host.items.length
-- host.items[0] is NumberItem
-- host.items[1] is TextItem

━━━━━━━━━━━━━━━━━━━━
+ NumberItem is Tuple

+ value Int

━━━━━━━━━━━━━━━━━━━━
+ TextItem is Tuple

+ value String

━━━━━━━━━━━━━━━━━━━━
+ MarkupListItem is Union

| NumberItem
| TextItem

━━━━━━━━━━━━━━━━━━━━
+ MarkupListHost

+ items [MarkupListItem]
```

```output
2
true
true
```

```yumi
-- null-stream().children.length

━━━━━━━━━━━━━━━━━━━━
+ null-stream ChildHost

<ChildHost
    no-children()

━━━━━━━━━━━━━━━━━━━━
+ do no-children ImmutableArray'ChildHost?

=? if true

━━━━━━━━━━━━━━━━━━━━
+ ChildHost

+ children [This]

────────────────────
+ do this += kids <This>?

if kids
    for kid in kids
        children += kid as This
```

```output
0
```

```yumi crash
. sink = make-optional-stream-sink()

━━━
+ OptionalStreamSink

+ name String
+ children [This]

───
+ This
    - name String

───
+ do this += kids <This>

for kid in kids
    children += kid as This

━━━
+ make-optional-stream-sink OptionalStreamSink

<OptionalStreamSink "root"
    no-optional-stream-children()

━━━
+ do no-optional-stream-children ImmutableArray'OptionalStreamSink?

=? if true
```

```output
Exception: Content not accepted
<no file:24>
        no-optional-stream-children()
        ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```yumi crash
/core/markup

. root = Node()
. child = Node()
markup-attach-new-child(root, child, null)

━━━━━━━━━━━━━━━━━━━━
+ Node
    is MarkupParent'Node

────────────────────
+ markup-content Node
    - child Node

child

────────────────────
+ do markup-attach-child
    - child Node
    - anchor Node?

=

────────────────────
+ do markup-detach-child
    - child Node

=
```

```output
Exception: Function "markup-attach-new-child" is private, declared with _, so only its own module can call it
<no file:6>
    markup-attach-new-child(root, child, null)
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```yumi crash
/core/markup

` @markup-runtime-internal

. root = Node()
. child = Node()
markup-attach-new-child(root, child, null)

━━━━━━━━━━━━━━━━━━━━
+ Node
    is MarkupParent'Node

────────────────────
+ markup-content Node
    - child Node

child

────────────────────
+ do markup-attach-child
    - child Node
    - anchor Node?

=

────────────────────
+ do markup-detach-child
    - child Node

=
```

```output
Exception: Function "markup-attach-new-child" is private, declared with _, so only its own module can call it
<no file:8>
    markup-attach-new-child(root, child, null)
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Collection content goes through ++= when += takes a single child.

```yumi
. extra = [Label("b"), Label("c")]
<Window win
    <Label = "a"
    extra

-- win.contents

━━━━━━━━━━━━━━━━━━━━
+ Window

+ contents [Label]

────────────────────
+ do this += child Label

contents += child

────────────────────
+ do this ++= children <Label>

contents ++= children

━━━━━━━━━━━━━━━━━━━━
+ Label

+ text String

────────────────────
+ This
    - text

────────────────────
+ to String

"[Label $text]"
```

```output
[Label a], [Label b], [Label c]
```

```yumi
/core/markup
/core/tests/support/markup-fixtures

<QueryRoot root

root.markup-batch-queue.checkpoint()
-- root.summary

root.show-extra = true
root.markup-batch-queue.checkpoint()
-- root.summary

root.show-first = false
root.markup-batch-queue.checkpoint()
-- root.summary

root.show-extra = false
root.markup-batch-queue.checkpoint()
-- root.summary

root.show-first = true
root.markup-batch-queue.checkpoint()
-- root.summary

━━━━━━━━━━━━━━━━━━━━
+ Marker is Box

+ text String

────────────────────
+ This
    - text

━━━━━━━━━━━━━━━━━━━━
+ QueryRoot is Box
    is MarkupRootElement

+ show-first = true
+ show-extra = false
+ markers Descendants'Marker
+ summary = "|".join(each.text in #markers)
+ markup-batch-queue := MarkupBatchQueue(this)

────────────────────
+ This

markup-mount-root(this, this)

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

────────────────────

<This
    if #show-first
        <Marker = "first"
    <Marker = "base"
    if #show-extra
        <Marker = "extra"
```

```output
first|base
first|base|extra
base|extra
base
first|base
```

```yumi
/core/markup
/core/tests/support/markup-fixtures

<QueryRoot root
    <Box
        <Mirror mirror

-- mirror.text
root.markup-batch-queue.checkpoint()
-- mirror.text

root.title = "renamed"
-- mirror.text
root.markup-batch-queue.checkpoint()
-- mirror.text

root.suffix = "!"
root.markup-batch-queue.checkpoint()
-- mirror.text

━━━━━━━━━━━━━━━━━━━━
+ Mirror is Box

+ owner Ancestor'QueryRoot
+ text = #owner?.#label ?? "none"

━━━━━━━━━━━━━━━━━━━━
+ QueryRoot is Box
    is MarkupRootElement

+ title = "root"
+ suffix = ""
+ label = #title + #suffix
+ markup-batch-queue := MarkupBatchQueue(this)

────────────────────
+ This

markup-mount-root(this, this)

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush
```

```output
root
root
renamed
renamed
renamed!
```

```yumi
/core/markup
/core/tests/support/markup-fixtures

. root = ProbeRoot()
. parent = Probe()
. child = Probe()
markup-append(root, parent)
markup-append(parent, child)
-- "${parent.mounted-count} ${child.mounted-count}"

markup-mount-root(root, root)
-- "${parent.mounted-count} ${child.mounted-count}"

markup-mount-root(root, root)
-- "${parent.mounted-count} ${child.mounted-count}"

━━━━━━━━━━━━━━━━━━━━
+ ProbeRoot is Box
    is MarkupRootElement

+ markup-batch-queue := MarkupBatchQueue(this)

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

━━━━━━━━━━━━━━━━━━━━
+ Probe is Box

+ mounted-count = 0

────────────────────
+ do mounted

mounted-count += 1
```

```output
0 0
1 1
1 1
```

### An if line that matches a pattern runs its body as content of the element.

```yumi
. item Item = Note("one")
<Window win
    if | note Note = item
        <Label = note.text
    else if | mark Mark = item
        <Label = "mark"

-- win.contents

━━━━━━━━━━━━━━━━━━━━
+ Item is Union

| Note
| Mark

━━━━━━━━━━━━━━━━━━━━
+ Note

+ text String

────────────────────
+ This
    - text

━━━━━━━━━━━━━━━━━━━━
+ Mark

+ count Int

────────────────────
+ This
    - count

━━━━━━━━━━━━━━━━━━━━
+ Window

+ contents [Label]

────────────────────
+ do this += child Label

contents += child

━━━━━━━━━━━━━━━━━━━━
+ Label

+ text String

────────────────────
+ This
    - text

────────────────────
+ to String

"[Label $text]"
```

```output
[Label one]
```

### Rejects a section of a class with nothing written in it, whether or not the class is used.

```yumi crash
-- 1

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ count = 1

────────────────────

────────────────────
+ get doubled

count * 2
```

```output
Exception: Empty content section not permitted
<no file:9>
    ────────────────────
    ^^^^^^^^^^^^^^^^^^^
```

### Rejects a second content section of a class, whether or not the class is used.

```yumi crash
-- 1

━━━━━━━━━━━━━━━━━━━━
+ Thing

────────────────────

<This

────────────────────

<This
```

```output
Exception: Only one content section permitted
<no file:11>
    ────────────────────
    ^^^^^^^^^^^^^^^^^^^
```

### Reads a property a markup element's name declares from another module.

```yumi
/core/markup
/core/tests/support/markup-fixtures
./meter

. meter = Meter()
. fill = meter.fill
-- "read"
```

```yumi file=meter.yumi

/core/markup
/core/tests/support/markup-fixtures

━━━━━━━━━━━━━━━━━━━━
+ Meter is Box

────────────────────

<This
    <Box fill

```

```output
read
```

### An error the markup of a class raises as it fills an object goes to unhandled-error, and the lines before it stay.

```yumi
/core/tests/support/markup-fixtures

unhandled-error = :: error => do
    -- "handled ${error.message}"

. panel = Panel()
-- panel.%first-child! is Label
-- "made"

━━━━━━━━━━━━━━━━━━━━
+ do parse Int
    - text String

?! SyntaxError("bad")
    if text == "x"
= text.length

━━━━━━━━━━━━━━━━━━━━
+ Panel is Box

+ value = "x"

────────────────────

<This
    <Label
        text = "a"
    <Label
        text = "${parse(value)}"
```

```output
handled bad
true
made
```

### A closure an attribute of a markup element gives reads a member of an element around it through that element's name, from its object when it runs.

```yumi
/core/tests/support/markup-fixtures

. panel = Panel()
. outer = panel.%first-child! as Item
. inner = outer.%first-child! as Item
inner.fire()
-- panel.seen
outer.title = "changed"
inner.fire()
-- panel.seen

━━━━━━━━━━━━━━━━━━━━
+ Item is Box

+ title = ""
+ run Doer?

────────────────────
+ This
    - title = ""

────────────────────
+ do fire

. callback = run
if callback
    callback()

━━━━━━━━━━━━━━━━━━━━
+ Panel is Box

+ seen = ""

────────────────────

<This
    <Item outer "outer"
        <Item "inner"
            run = => do note(outer.title)

────────────────────
+ do note
    - text String

seen = text
```

```output
outer
changed
```

### A read of a markup link written after `?` reads from what the link before it holds, and gives nothing where that holds nothing.

```yumi
/core/tests/support/markup-fixtures

. panel = Panel()
-- panel.grandchild() != null
-- panel.great-grandchild() != null

━━━━━━━━━━━━━━━━━━━━
+ Panel is Box

────────────────────

<This
    <Box
        <Box

────────────────────
+ do grandchild Box?

= %first-child?.%first-child

────────────────────
+ do great-grandchild Box?

= %first-child?.%first-child?.%first-child
```

```output
true
false
```

### A loop on a stream in the body of a markup element subscribes to the element's event, and reads the scope outside the element as its attributes do: the item of the loop around it, not the element's member of that name.

```yumi
/core/tests/support/markup-fixtures

. panel = Panel()
. first = panel.%first-child! as Item
. second = first.%next-sibling! as Item
first.clicked <- 3
second.clicked <- 4
do new
    -- panel.seen
    second.tag = "changed"
    second.clicked <- 5
    do new
        -- panel.seen

━━━━━━━━━━━━━━━━━━━━
+ Item is Box

+ tag = ""
+ event clicked Int

━━━━━━━━━━━━━━━━━━━━
+ Panel is Box

+ seen = ""

────────────────────

<This
    for tag in ["a", "b"]
        <Item
            tag = tag ++ "!"
            for n on clicked
                note("$tag $n")

────────────────────
+ do note
    - text String

seen = seen ++ text ++ ";"
```

```output
a 3;b 4;
a 3;b 4;b 5;
```

### A descendant query of a class with type parameters looks for its target for the parameters the object's class gives it.

```yumi
/core/markup
/core/tests/support/markup-fixtures

<MarkerRoot'Int root

root.markup-batch-queue.checkpoint()
-- root.markers.length
-- "|".join("${each.value}" in root.markers)

━━━━━━━━━━━━━━━━━━━━
+ Marker'T is Box

+ value T

────────────────────
+ This
    - value T

━━━━━━━━━━━━━━━━━━━━
+ MarkerRoot'T is Box
    is MarkupRootElement

+ markers Descendants'(Marker'T)
+ markup-batch-queue := MarkupBatchQueue(this)

────────────────────
+ This

markup-mount-root(this, this)

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

. ignored = flush.applied

────────────────────

<This
    <Marker'T = 1 as T
    <Box
        <Marker'T = 2 as T
```

```output
2
1|2
```

### A descendant query of a class with type parameters, made by a call, finds the objects of a class whose constructor was compiled before the query's class was made for it.

```yumi
/core/markup
/core/tests/support/markup-fixtures

. early = Leaf()
. root = Finder'Leaf()

root.markup-batch-queue.checkpoint()
-- root.found.length

━━━━━━━━━━━━━━━━━━━━
+ Leaf is Box

━━━━━━━━━━━━━━━━━━━━
+ Finder'T is Box
    is MarkupRootElement

+ found Descendants'T
+ markup-batch-queue := MarkupBatchQueue(this)

────────────────────
+ This

markup-mount-root(this, this)

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

. ignored = flush.applied

────────────────────

<This
    <Leaf
    <Box
        <Leaf
```

```output
2
```

### A tracked read in a closure an attribute gives is an error, as the closure runs outside the attribute's live value.

```yumi crash
/core/tests/support/markup-fixtures

. panel = Panel()
. item = panel.%first-child! as Item
item.fire()
-- panel.seen
panel.count = 4
item.fire()
-- panel.seen
panel.count = 6
item.fire()
-- panel.seen

━━━━━━━━━━━━━━━━━━━━
+ Item is Box

+ run Doer?

────────────────────
+ do fire

. callback = run
if callback
    callback()

━━━━━━━━━━━━━━━━━━━━
+ Panel is Box

+ seen = ""
+ count = 1

────────────────────

<This
    <Item
        run = => do note("count ${#count}")

────────────────────
+ do note
    - text String

seen = text
```

```output
Exception: "#count" tracks nothing here: a read is followed only where it is written directly in a live value, such as markup, a derived property, a rule or a live statement. Write "count" to read it plainly, or read "#count" in the live value and pass it in as an argument
<no file:37>
            run = => do note("count ${#count}")
                                      ^^^^^^
```

### An attribute a setter answers is given again when the tracked state its value reads changes.

```yumi
/core/tests/support/markup-fixtures

. panel = Panel()
. item = panel.%first-child! as Item
-- "${item.calls} ${item.last}"
panel.count = 4
-- "${item.calls} ${item.last}"

━━━━━━━━━━━━━━━━━━━━
+ Item is Box

+ calls = 0
+ last = 0

────────────────────
+ get shown Int

last

────────────────────
+ do shown = value Int

calls += 1
last = value

━━━━━━━━━━━━━━━━━━━━
+ Panel is Box

+ count = 1

────────────────────

<This
    <Item
        shown = #count * 10
```

```output
1 10
2 40
```

### The body of a `whenever` rule on a value runs only once the value changes, after the markup has made the element it reads by name.

```yumi
/core/tests/support/markup-fixtures

<Demo demo
-- "made ${demo.inner.count}"
demo.owner.stamp = 5
-- "after ${demo.inner.count}"

━━━━━━━━━━━━━━━━━━━━
+ Owner

+ stamp U64 = 0

━━━━━━━━━━━━━━━━━━━━
+ Inner is Box

+ count = 0

────────────────────
+ do reload

count += 1

━━━━━━━━━━━━━━━━━━━━
+ Demo is Box

+ owner = Owner()

────────────────────

<This
    <Inner inner

────────────────────
whenever owner.#stamp

-- "rule ran"
inner.reload()
```

```output
made 0
rule ran
after 1
```

### A property a markup element sets that also names an untyped argument of the constructor of a class above the element's class is given to the constructor.

```yumi
/core/markup
/core/tests/support/markup-fixtures

<Row row
    note = "first"
    title = "hello"

-- "${row.note} ${row.title}"

━━━━━━━━━━━━━━━━━━━━
+ Tag is Box

+ title String

────────────────────
+ This
    - title = ""

this.title = title

━━━━━━━━━━━━━━━━━━━━
+ Row is Tag

+ note String
```

```output
first hello
```

### An event of a markup element is no value: a handler in the element's body that declares a local of it is an error at the event.

```yumi crash
/core/tests/support/markup-fixtures

. panel = Panel()

━━━━━━━━━━━━━━━━━━━━
+ Panel is Box

+ seen = ""

────────────────────

<This
    <Button button
        for event on press
            . pressed = button.press
            seen = "pressed"

━━━━━━━━━━━━━━━━━━━━
+ Button is Box

+ event press Int
```

```output
Exception: "press" is an event of Button; it cannot be used as a value
<no file:16>
                . pressed = button.press
                            ^^^^^^^^^^^^
```

### An event of a markup element is no value: a handler in the element's body that prints it is an error at the event.

```yumi crash
/core/tests/support/markup-fixtures

. panel = Panel()

━━━━━━━━━━━━━━━━━━━━
+ Panel is Box

────────────────────

<This
    <Button button
        for event on press
            -- button.press

━━━━━━━━━━━━━━━━━━━━
+ Button is Box

+ event press Int
```

```output
Exception: "press" is an event of Button; it cannot be used as a value
<no file:14>
                -- button.press
                   ^^^^^^^^^^^^
```

### An event given to a parameter of a type its endpoint is not, which only converting it as a value could give, is an error at the event.

```yumi crash
/core/tests/support/markup-fixtures

. button = Button()
button.describe()

━━━━━━━━━━━━━━━━━━━━
+ Button is Box

+ event press Int

────────────────────
+ do describe

note(press)

────────────────────
+ do note
    - items Iterable'Int

for item in items
    -- item
```

```output
Exception: "press" is an event of Button; it cannot be used as a value
<no file:15>
    note(press)
         ^^^^^
```

### An event of a markup element is no value: a handler in the element's body that assigns it to a property of another type is an error at the event.

```yumi crash
/core/tests/support/markup-fixtures

. panel = Panel()

━━━━━━━━━━━━━━━━━━━━
+ Panel is Box

+ count = 0

────────────────────

<This
    <Button button
        for event on press
            count = button.press

━━━━━━━━━━━━━━━━━━━━
+ Button is Box

+ event press Int
```

```output
Exception: "press" is an event of Button; it cannot be used as a value
<no file:16>
                count = button.press
                        ^^^^^^^^^^^^
```

### An event of a markup element is no value: a handler in the element's body that makes it an item of a list is an error at the event.

```yumi crash
/core/tests/support/markup-fixtures

. panel = Panel()

━━━━━━━━━━━━━━━━━━━━
+ Panel is Box

+ seen = 0

────────────────────

<This
    <Button button
        for event on press
            . all = [button.press]
            seen = all.length

━━━━━━━━━━━━━━━━━━━━
+ Button is Box

+ event press Int
```

```output
Exception: "press" is an event of Button; it cannot be used as a value
<no file:16>
                . all = [button.press]
                         ^^^^^^^^^^^^
```

### An event is no value: one made a field of a tuple is an error at the event.

```yumi crash
/core/tests/support/markup-fixtures

. button = Button()
button.describe()

━━━━━━━━━━━━━━━━━━━━
+ Button is Box

+ event press Int

────────────────────
+ do describe

. pair = (press, 1)
-- "made"
```

```output
Exception: "press" is an event of Button; it cannot be used as a value
<no file:15>
    . pair = (press, 1)
              ^^^^^
```

### An event of a markup element is no value: a handler in the element's body that casts it to a type its endpoint is not is an error at the event.

```yumi crash
/core/tests/support/markup-fixtures

. panel = Panel()

━━━━━━━━━━━━━━━━━━━━
+ Panel is Box

+ seen = ""

────────────────────

<This
    <Button button
        for event on press
            seen = button.press as String

━━━━━━━━━━━━━━━━━━━━
+ Button is Box

+ event press Int
```

```output
Exception: "press" is an event of Button; it cannot be used as a value
<no file:16>
                seen = button.press as String
                       ^^^^^^^^^^^^
```

### An event of a markup element is no value: a handler in the element's body that gives it to a parameter of a type its endpoint is not is an error at the event.

```yumi crash
/core/tests/support/markup-fixtures

. panel = Panel()

━━━━━━━━━━━━━━━━━━━━
+ Panel is Box

+ seen = ""

────────────────────

<This
    <Button button
        for event on press
            record(button.press)

────────────────────
+ do record
    - items Iterable'Int

for item in items
    seen = "$item"

━━━━━━━━━━━━━━━━━━━━
+ Button is Box

+ event press Int
```

```output
Exception: "press" is an event of Button; it cannot be used as a value
<no file:16>
                record(button.press)
                       ^^^^^^^^^^^^
```

### An event is no value: an attribute of a markup element that gives it to a property of another type is an error at the event.

```yumi crash
/core/tests/support/markup-fixtures

. panel = Panel()

━━━━━━━━━━━━━━━━━━━━
+ Panel is Box

+ event poke Int

────────────────────

<This
    <Button
        count = poke

━━━━━━━━━━━━━━━━━━━━
+ Button is Box

+ count = 0
```

```output
Exception: "poke" is an event of Panel; it cannot be used as a value
<no file:15>
            count = poke
                    ^^^^
```

### A tracked property of the class, tested in an element nested in another, holds a value later in the same expression.

```yumi
/core/tests/support/markup-fixtures

<Demo demo

-- (demo.%first-child!.%first-child! as TextBox).text
demo.limit = 1
-- (demo.%first-child!.%first-child! as TextBox).text

━━━━━━━━━━━━━━━━━━━━
+ Demo is Box

+ limit Int?

────────────────────

<This
    <Box
        <TextBox
            text = "${#limit != null && 2 >= #limit}"
```

```output
false
true
```

### A markup element whose class declares only constructors that take arguments must give them; a name after the class names the element, even where a local of that name is in scope.

```yumi compile-error
Host()

━━━━━━━━━━━━━━━━━━━━
+ Box

────────────────────
+ do this += child Box

= child

━━━━━━━━━━━━━━━━━━━━
+ Row is Box

+ index Int

────────────────────
+ This
    - index

━━━━━━━━━━━━━━━━━━━━
+ Host is Box

────────────────────

<This
    for index in 0 .. < 3
        <Row index
```

```output
Exception: Markup element <Row gives no value for "index", which the constructor of Row requires; "index" after <Row names the element
<no file:28>
            <Row index
             ^^^
```

```yumi crash
/core/tests/support/markup-fixtures

. demo = Demo()

━━━━━━━━━━━━━━━━━━━━
+ Pane is Box

+ value Int = 2

━━━━━━━━━━━━━━━━━━━━
+ Demo is Box

+ count => pane.value

────────────────────

<This
    <Pane pane
        for index in 0 .. < count
            <Label "$index"
```

```stderr
Unhandled panic: Named markup element "pane" is read before its markup constructs it
<no file:14>

Task 1

Stack trace:
  0  __source__:Demo:count no file:14:12
  1  __source__:content no file:20:29
  2  __source__:Demo:Demo no file:12:1
  3  __source__:main no file:4:10
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### A statement above a named element that reads its name makes the element there, once.

```yumi
-- "before"
-- thing.name
<Thing thing
    name = "made early"
-- thing.name

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ name = "unnamed"

────────────────────
+ This

-- "constructed"
```

```output
before
constructed
made early
made early
```

### A named element in a function body reads the locals declared above it, and so do the named lines of its body.

```yumi
-- make().inner!.name

━━━━━━━━━━━━━━━━━━━━
+ do make Holder

. label = "inner label"
<Holder holder
    title = label
    <Thing inner
        name = "${label}!"
-- holder.title
= holder

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ title = ""
+ inner Thing?

────────────────────
+ do this += item Thing

inner = item

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ name = "unnamed"
```

```output
inner label
inner label!
```

### An optional chain reads a named element another class's markup declares.

```yumi
/core/markup
/core/tests/support/markup-fixtures

. reader = Reader()
-- reader.inner-text()
reader.holder = Holder()
-- reader.inner-text()

━━━━━━━━━━━━━━━━━━━━
+ Inner is Box

+ text = "inner"

━━━━━━━━━━━━━━━━━━━━
+ Holder is Box

────────────────────

<This
    <Inner inner

━━━━━━━━━━━━━━━━━━━━
+ Reader

+ holder Holder?

────────────────────
+ do inner-text String

= holder?.inner.text ?? "none"
```

```output
none
inner
```

### A live statement that sets a property of a markup element follows what its value reads: the element's action runs at once where the element has no batch root, and with its root's batch where it has one.

```yumi
/core/markup
/core/tests/support/markup-fixtures

. counter = Counter()
. loose = Label()
loose.text = "loose ${counter.#count}"

<Demo demo
    <Label label

label.text = "held ${counter.#count}"
demo.markup-batch-queue.checkpoint()
counter.count = 3
-- loose.text
-- label.text
demo.markup-batch-queue.checkpoint()
-- label.text

━━━━━━━━━━━━━━━━━━━━
+ Counter

+ count Int

━━━━━━━━━━━━━━━━━━━━
+ Demo is Box
    is MarkupRootElement

+ markup-batch-queue := MarkupBatchQueue(this)

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush
```

```output
loose 3
held 0
held 3
```

### A tracked read of each item of a pipeline that a call in an attribute works out is followed: the attribute is given again as an item changes.

```yumi
/core/tests/support/markup-fixtures
/stream/aggregate

. source = Source()
<Counter c
    value = count(each in source.#items when each.#is-pending)

-- c.value
source.items[0].is-pending = false
-- c.value
source.items[1].is-pending = false
-- c.value
. added = Item()
source.items += added
-- c.value
added.is-pending = false
-- c.value
source.items[0].is-pending = true
-- c.value

━━━━━━━━━━━━━━━━━━━━
+ Item

+ is-pending = true

━━━━━━━━━━━━━━━━━━━━
+ Source

+ items = [Item(), Item()]

━━━━━━━━━━━━━━━━━━━━
+ Counter is Box

+ value = 0
```

```output
2
1
0
1
0
1
```

### A closure in a function's markup reads a named element the function makes after it, a child of its own element included, when it runs. So does a task the closure starts.

```yumi
run()

━━━━━━━━━━━━━━━━━━━━
+ do run

<Box tool
    run = => do -- "tool reads ${seed.text}"
    <Box seed
        text = "seed"
    <Box go
        run = => do
            do new
                -- "task reads ${note.text}"
    <Box note
        text = "note"
tool.fire()
go.fire()
━━━━━━━━━━━━━━━━━━━━
+ Box

+ run Doer?
+ text = "box"

────────────────────
+ do this += child Box

────────────────────
+ do fire

. callback = run
if callback
    callback()
```

```output
tool reads seed
task reads note
```

### Two closures in a function's markup read each other's elements, and a closure reads the element it is an attribute of.

```yumi
run()

━━━━━━━━━━━━━━━━━━━━
+ do run

<Box
    <Box yes
        text = "yes"
        run = => do -- "yes reads ${no.text}"
    <Box no
        text = "no"
        run = => do -- "no reads ${yes.text} and ${no.text}"
yes.fire()
no.fire()
━━━━━━━━━━━━━━━━━━━━
+ Box

+ run Doer?
+ text = "box"

────────────────────
+ do this += child Box

────────────────────
+ do fire

. callback = run
if callback
    callback()
```

```output
yes reads no
no reads yes and no
```

### A closure in a function's markup that runs before the element it reads is made panics.

```yumi crash
run()

━━━━━━━━━━━━━━━━━━━━
+ do run

<Box
    <Box first
        run = => do -- "first reads ${second.text}"
    <Box middle
        text = call-first(first)
    <Box second
        text = "second"

━━━━━━━━━━━━━━━━━━━━
+ do call-first String
    - box Box

box.fire()
= "middle"
━━━━━━━━━━━━━━━━━━━━
+ Box

+ run Doer?
+ text = "box"

────────────────────
+ do this += child Box

────────────────────
+ do fire

. callback = run
if callback
    callback()
```

```stderr
Unhandled panic: Named markup element "second" is read before its markup constructs it
<no file:9>

Task 1

Stack trace:
  0  __source__:run no file:9:39
  1  __source__:runCasting no file:9:9
  2  __source__:Box:fire no file:35:5
  3  __source__:call-first no file:19:1
  4  __source__:run no file:11:16
  5  __source__:main no file:2:1
  6  __source__:init.__source__ no file
  7  __source__:brix.main no file
```

### A closure in class content that runs before the element it reads is made panics.

```yumi crash
. panel = Panel()

━━━━━━━━━━━━━━━━━━━━
+ Panel is Box

────────────────────

<This
    <Box first
        run = => do -- "first reads ${second.text}"
    <Box middle
        text = call-first(first)
    <Box second
        text = "second"

━━━━━━━━━━━━━━━━━━━━
+ do call-first String
    - box Box

box.fire()
= "middle"
━━━━━━━━━━━━━━━━━━━━
+ Box

+ run Doer?
+ text = "box"

────────────────────
+ do this += child Box

────────────────────
+ do fire

. callback = run
if callback
    callback()
```

```stderr
Unhandled panic: Named markup element "second" is read before its markup constructs it
<no file:11>

Task 1

Stack trace:
  0  __source__:run no file:11:39
  1  __source__:runCasting no file:11:9
  2  __source__:Box:fire no file:37:5
  3  __source__:call-first no file:21:1
  4  __source__:content no file:13:16
  5  __source__:Panel:Panel no file:5:1
  6  __source__:main no file:2:11
  7  __source__:init.__source__ no file
  8  __source__:brix.main no file
```

### A task a function starts before its markup cannot read an element the markup makes, as the task keeps what it reads when it starts.

```yumi compile-error
run()

━━━━━━━━━━━━━━━━━━━━
+ do run

do new
    -- note.text
<Box
    <Box note
        text = "note"
━━━━━━━━━━━━━━━━━━━━
+ Box

+ run Doer?
+ text = "box"

────────────────────
+ do this += child Box

────────────────────
+ do fire

. callback = run
if callback
    callback()
```

```output
Exception: Named markup element "note" is read in a task started before its markup constructs it, and a task keeps what it reads as it starts; start the task after the markup
<no file:8>
        -- note.text
           ^^^^
```

### A closure that reads an element made after it needs the element's type arguments written.

```yumi compile-error
run()

━━━━━━━━━━━━━━━━━━━━
+ do run

<Box
    <Box go
        run = => do -- "go reads ${held.value}"
    <Holder held = 3
go.fire()
━━━━━━━━━━━━━━━━━━━━
+ Box

+ run Doer?
+ text = "box"

────────────────────
+ do this += child Box

────────────────────
+ do fire

. callback = run
if callback
    callback()

━━━━━━━━━━━━━━━━━━━━
+ Holder'T is Box

+ value T

────────────────────
+ This
    - value T
```

```output
Exception: Named markup element "held" is read in a closure made before its markup constructs it, so it needs its type arguments written after "Holder"
<no file:9>
            run = => do -- "go reads ${held.value}"
                                       ^^^^
```

### A binding with no batch root stores back as the element changes, before the statement that changed it goes on. Its place may be a local.

```yumi
. level = 5
<Knob knob
    value <-> level
-- "${level} ${knob.value}"
knob.turn(3)
-- "${level} ${knob.value}"
level = 7
-- "${level} ${knob.value}"

━━━━━━━━━━━━━━━━━━━━
+ Knob

+ value = 0

────────────────────
+ do turn
    - next Int

value = next
```

```output
5 5
3 3
7 7
```

### A binding's place may be a member of another object. A binding converts each way, and where the element holds a number the conversion loses something of, the element is given back what the place holds.

```yumi
/core/markup
/core/tests/support/markup-fixtures

. settings = Settings()
. panel = Panel(settings)
-- "${settings.volume} ${panel.dial.value} ${panel.steps.n} ${panel.knob.value}"
panel.dial.turn(3)
panel.knob.turn(2.6)
-- "${settings.volume} ${panel.dial.value} ${panel.steps.n} ${panel.knob.value}"
panel.knob.turn(2.2)
-- "${settings.volume} ${panel.dial.value} ${panel.steps.n} ${panel.knob.value}"
settings.volume = 9
-- "${settings.volume} ${panel.dial.value} ${panel.steps.n} ${panel.knob.value}"

━━━━━━━━━━━━━━━━━━━━
+ Settings

+ volume = 5

━━━━━━━━━━━━━━━━━━━━
+ Steps is (n Int)

────────────────────
+ to F64

n as F64

────────────────────
+ from value F64

Steps(value as Int)

━━━━━━━━━━━━━━━━━━━━
+ Dial is Box

+ value = 0

────────────────────
+ do turn
    - next Int

value = next

━━━━━━━━━━━━━━━━━━━━
+ Knob is Box

+ value = 0.0

────────────────────
+ do turn
    - next F64

value = next

━━━━━━━━━━━━━━━━━━━━
+ Panel is Box

+ settings Settings
+ steps = Steps(1)

────────────────────
+ This
    - settings

────────────────────

<This
    <Dial dial
        value <-> settings.volume
    <Knob knob
        value <-> steps
```

```output
5 5 1 1
3 3 2 2
3 3 2 2
9 9 2 2
```

### A binding in a row of a keyed loop binds that row's item, also after the rows move.

```yumi
/core/markup
/core/tests/support/markup-fixtures

. list = Mixer()
-- list.state()
list.knob(1).turn(9)
-- list.state()
list.tracks = list.tracks.reversed()
-- list.state()
list.knob(0).turn(4)
-- list.state()
list.tracks[1].level = 6
-- list.state()

━━━━━━━━━━━━━━━━━━━━
+ Track

+ name String
+ level = 0

────────────────────
+ This
    - name
    - level

━━━━━━━━━━━━━━━━━━━━
+ Knob is Box

+ value = 0

────────────────────
+ do turn
    - next Int

value = next

━━━━━━━━━━━━━━━━━━━━
+ Mixer is Box

+ tracks = [Track("a", 1), Track("b", 2)]

────────────────────
+ do knob Knob
    - index Int

= (%first-child!.child-at(index))! as Knob

────────────────────
+ do state String

. parts [String]
for track in tracks
    parts += "${track.name}=${track.level}"
for index in 0 .. < 2
    parts += "k${knob(index).value}"
= " ".join(parts)

────────────────────

<This
    for track in #tracks keyed by track.name
        <Knob
            value <-> track.level
```

```output
a=1 b=2 k1 k2
a=1 b=9 k1 k9
b=9 a=1 k9 k1
b=4 a=1 k4 k1
b=4 a=6 k4 k6
```

### A non-optional place bound to an optional property stores back what the property holds, and nothing where it holds nothing.

```yumi
. miles = false
<Picker picker
    selected <-> miles
-- "${miles} ${picker.selected}"
picker.pick(true)
-- "${miles} ${picker.selected}"
picker.clear()
-- "${miles} ${picker.selected}"
miles = false
-- "${miles} ${picker.selected}"

━━━━━━━━━━━━━━━━━━━━
+ Picker

+ selected Bool?

────────────────────
+ do pick
    - next Bool

selected = next

────────────────────
+ do clear

selected = null
```

```output
false false
true true
true unknown
false false
```

### An optional property may bind a place that is a member of another object, and an optional place still takes nothing.

```yumi
. settings = Settings()
. maybe Int? = 4
<Chooser first
    chosen <-> settings.size
<Chooser second
    chosen <-> maybe
first.choose(7)
second.choose(8)
-- "${settings.size} ${maybe}"
first.clear()
second.clear()
-- "${settings.size} ${maybe}"
settings.size = 2
-- "${settings.size} ${first.chosen}"

━━━━━━━━━━━━━━━━━━━━
+ Settings

+ size = 3

━━━━━━━━━━━━━━━━━━━━
+ Chooser

+ chosen Int?

────────────────────
+ do choose
    - next Int

chosen = next

────────────────────
+ do clear

chosen = null
```

```output
7 8
7 unknown
2 2
```

### A binding written outside a markup element's body is reported.

```yumi crash
. a = 1
. b = 2
a <-> b
-- a
```

```output
Exception: "<->" binds a property of a markup element to a place, so it is written in the element's body; to store a value here, write "="
<no file:4>
    a <-> b
    ^^^^^^^
```

### A root <This> cannot bind, since its attributes give the object's own properties their first values.

```yumi crash
/core/markup
/core/tests/support/markup-fixtures

. p = Panel()
-- p.level

━━━━━━━━━━━━━━━━━━━━
+ Panel is Box

+ level = 1
+ other = 2

────────────────────

<This
    level <-> other
```

```output
Exception: A root <This> cannot bind with "<->", since its attributes give the object's own properties their first values; bind the property where the object is made
<no file:17>
        level <-> other
        ^^^^^^^^^^^^^^^
```

### A binding follows a property the element stores, so one a getter answers is reported, and so is a name of no property, as a parameter of the constructor alone is.

```yumi crash
. level = 1
<Knob knob
    shown <-> level

━━━━━━━━━━━━━━━━━━━━
+ Knob

+ stored = 0

────────────────────
+ get shown Int

stored
```

```output
Exception: "<->" binds a property the element stores, and Knob answers "shown" with a getter or a setter, which a binding cannot follow; bind a property Knob stores
<no file:4>
        shown <-> level
        ^^^^^
```

```yumi crash
. level = 1
<Knob knob
    start <-> level

━━━━━━━━━━━━━━━━━━━━
+ Knob

+ value = 0

────────────────────
+ This
    - start Int

value = start
```

```output
Exception: "<->" binds a property of the element, and Knob has no property "start"
<no file:4>
        start <-> level
        ^^^^^
```

### A binding converts each way on its own, and a value that converts only when asked is reported where the binding is written, or where it names the property for the way back.

```yumi crash
. level = 1
<Knob knob
    value <-> level

━━━━━━━━━━━━━━━━━━━━
+ Knob

+ value = 0.0
```

```output
Exception: "<->" needs each side to become the other on its own, and I64 becomes F64 only when asked; bind a place of the property's type
<no file:4>
        value <-> level
        ^^^^^^^^^^^^^^^
```

```yumi crash
. level = 1i32
<Knob knob
    value <-> level

━━━━━━━━━━━━━━━━━━━━
+ Knob

+ value = 0
```

```output
Exception: "<->" needs each side to become the other on its own, and I64 becomes I32 only when asked; bind a place of the property's type
<no file:4>
        value <-> level
        ^^^^^
```

### A slot outlet cannot bind, since what fills the slot is made elsewhere.

```yumi crash
/core/markup
/core/tests/support/markup-fixtures

. p = Panel()

━━━━━━━━━━━━━━━━━━━━
+ Panel is Box

+ caption = "a"

────────────────────
+ <TextBox footer
    text <-> caption
```

```output
Exception: A slot outlet cannot bind with "<->", since what fills the slot is made elsewhere; bind the property in the element that fills it
<no file:14>
        text <-> caption
        ^^^^^^^^^^^^^^^^
```

### A handler in the body of a markup element reads and assigns the names around the element, though the element has members of those names, and reads the element through its name.

```yumi
/core/tests/support/markup-fixtures

. panel = Panel()
panel.button.press <- 1
-- "${panel.count} ${panel.button.count}"

━━━━━━━━━━━━━━━━━━━━
+ Panel is Box

+ count = 7

────────────────────

<This
    <Button button
        for event on press
            count = count + button.count

━━━━━━━━━━━━━━━━━━━━
+ Button is Box

+ count = 3
+ event press Int
```

```output
10 3
```
