# Facets

### A module named after another beside it belongs to that module's file family, and is built for every platform

```yumi
./lib/box

-- box-name()
```

```yumi file=lib/box.yumi

./box.scroll

━━━━━━━━━━━━━━━━━━━━
+ box-name String

"box ${scroll-name()}"

```

```yumi file=lib/box.scroll.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ scroll-name String

"scroll"

```

```output
box scroll
```

### Only a module's file family can import the family's modules

```yumi compile-error
./lib/box.scroll

-- 1
```

```yumi file=lib/box.yumi

-- 2
```

```yumi file=lib/box.scroll.yumi

-- 3
```

```output
Exception: Module "./lib/box.scroll" belongs to the file family of box.yumi, so only that family can import it
<no file:2>
    ./lib/box.scroll
    ^^^^^^^^^^^^^^^^
```

### One module of a family can import another

```yumi
./lib/box

-- box-name()
```

```yumi file=lib/box.yumi

./box.scroll

━━━━━━━━━━━━━━━━━━━━
+ box-name String

scroll-name()

```

```yumi file=lib/box.scroll.yumi

./box.focus

━━━━━━━━━━━━━━━━━━━━
+ scroll-name String

"scroll ${focus-name()}"

```

```yumi file=lib/box.focus.yumi

━━━━━━━━━━━━━━━━━━━━
+ focus-name String

"focus"

```

```output
scroll focus
```

### A facet's stored property is held in its class's objects, with the value the facet writes for it, and its methods are the class's

```yumi
./lib/box

. box = Box()
-- box.offset
box.scroll-to(5)
-- box.offset
```

```yumi file=lib/box.yumi

./box.scroll

━━━━━━━━━━━━━━━━━━━━
+ Box

+ width = 10

```

```yumi file=lib/box.scroll.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxScroll is facet of Box

+ offset = 3

────────────────────
+ do scroll-to
    - point Int

offset = point + width

```

```output
3
15
```

### The class's own code uses its facets' public members by name, as its own

```yumi
./lib/box

. box = Box()
box.grow()
-- box.describe()
```

```yumi file=lib/box.yumi

./box.scroll

━━━━━━━━━━━━━━━━━━━━
+ Box

+ width = 10

────────────────────
+ describe String

"width $width offset $offset"

────────────────────
+ do grow

scroll-to(2)
this.offset += 1

```

```yumi file=lib/box.scroll.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxScroll is facet of Box

+ offset = 3

────────────────────
+ do scroll-to
    - point Int

offset = point + width

```

```output
width 10 offset 13
```

### A class derived from one with facets has their properties, given their values by its constructors, after the class's own and before its own

```yumi
./lib/panel

. panel = Panel()
-- panel.offset
-- panel.title
panel.scroll-to(1)
-- panel.offset
-- panel.area
```

```yumi file=lib/panel.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ Panel is Box

+ title = "main"

```

```yumi file=lib/box.yumi

./box.scroll

━━━━━━━━━━━━━━━━━━━━
+ Box

+ width = 10

```

```yumi file=lib/box.scroll.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxScroll is facet of Box

+ offset = 3
+ area := width * 2

────────────────────
+ do scroll-to
    - point Int

offset = point + width

```

```output
3
main
11
20
```

### A facet reaches its class's private members and its module's private declarations, which the class's module does not reach of the facet's

```yumi
./lib/box

-- Box().measure()
```

```yumi file=lib/box.yumi

./box.scroll

━━━━━━━━━━━━━━━━━━━━
+ Box

_ width = 10

━━━━━━━━━━━━━━━━━━━━
_ doubled Int
    - value Int

value * 2

```

```yumi file=lib/box.scroll.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxScroll is facet of Box

────────────────────
+ measure Int

doubled(width)

```

```output
20
```

### A facet can be part of another facet, and reaches its members and its class's

```yumi
./lib/box

. box = Box()
-- box.indicator-text()
```

```yumi file=lib/box.yumi

./box.scroll
./box.scroll.indicator

━━━━━━━━━━━━━━━━━━━━
+ Box

+ width = 10

```

```yumi file=lib/box.scroll.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxScroll is facet of Box

+ offset = 3

```

```yumi file=lib/box.scroll.indicator.yumi

./box
./box.scroll

━━━━━━━━━━━━━━━━━━━━
+ BoxScrollIndicator is facet of BoxScroll

+ shown = true

────────────────────
+ indicator-text String

"$shown $offset $width"

```

```output
true 3 10
```

### Facets of one class name their private members as they like, and each module reaches its own

```yumi
./lib/box

. box = Box()
-- box.scroll-text()
-- box.focus-text()
```

```yumi file=lib/box.yumi

./box.focus
./box.scroll

━━━━━━━━━━━━━━━━━━━━
+ Box

```

```yumi file=lib/box.focus.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxFocus is facet of Box

_ cursor = "focus"

────────────────────
_ get marker String

"focus marker"

────────────────────
_ helper String

"focus helper"

────────────────────
+ focus-text String

"$cursor $marker ${helper()}"

```

```yumi file=lib/box.scroll.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxScroll is facet of Box

_ cursor = 7

────────────────────
_ get marker Int

cursor + 1

────────────────────
_ helper String

"scroll helper"

────────────────────
+ scroll-text String

"$cursor $marker ${helper()}"

```

```output
7 8 scroll helper
focus focus marker focus helper
```

### A facet reaches the public members of the facets it sees, on its own object and on any other of its class, and code outside the family reaches every facet's

```yumi
./lib/box

. box = Box()
. other = Box()
-- box.scroll-text(other)
-- box.focused
-- box.offset
```

```yumi file=lib/box.yumi

./box.focus
./box.scroll

━━━━━━━━━━━━━━━━━━━━
+ Box

```

```yumi file=lib/box.focus.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxFocus is facet of Box

+ focused = true

```

```yumi file=lib/box.scroll.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxScroll is facet of Box
    sees BoxFocus

+ offset = 3

────────────────────
+ scroll-text String
    - other Box

"$focused ${other.focused} ${other.offset}"

```

```output
true true 3
true
3
```

### A facet does not reach a sibling it does not see, on its own object or on another of its class

```yumi compile-error
./lib/box

-- Box().scroll-text(Box())
-- Box().focused-text(Box())
-- Box().ring-text(Box())
```

```yumi file=lib/box.yumi

./box.focus
./box.scroll

━━━━━━━━━━━━━━━━━━━━
+ Box

```

```yumi file=lib/box.focus.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxFocus is facet of Box

+ focused = true

────────────────────
^ ring Int

2

```

```yumi file=lib/box.scroll.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxScroll is facet of Box

────────────────────
+ scroll-text String
    - other Box

"$focused"

────────────────────
+ focused-text String
    - other Box

"${other.focused}"

────────────────────
+ ring-text String
    - other Box

"${other.ring()}"

```

```output
Exception: Name "focused" not found; facet BoxFocus declares it, and only the facets a facet sees are in reach. Write "sees BoxFocus" under the facet that uses it
<lib/box.scroll.yumi:11>
    "$focused"
      ^^^^^^^

Exception: Box has no property "focused" here; facet BoxFocus declares it, and only the facets a facet sees are in reach. Write "sees BoxFocus" under the facet that uses it
<lib/box.scroll.yumi:17>
    "${other.focused}"
       ^^^^^

Exception: Box has no method "ring" here; facet BoxFocus declares it, and only the facets a facet sees are in reach. Write "sees BoxFocus" under the facet that uses it
<lib/box.scroll.yumi:23>
    "${other.ring()}"
       ^^^^^^^^^^^^
```

### A member declared with ^ is shared with its class's file family: the class's own code, the facets that see its facet and the family's other modules reach it, on any object of the class

```yumi
./lib/box

. box = Box()
box.grow()
-- box.describe()
-- Box().describe()
```

```yumi file=lib/box.yumi

./box.focus
./box.scroll
./box.tools

━━━━━━━━━━━━━━━━━━━━
+ Box

^ width = 10

────────────────────
+ This

────────────────────
^ This
    - start Int

width = start

────────────────────
+ describe String

"$offset ${focus-text()} ${tools-text(this)}"

────────────────────
+ do grow

keep-end()

```

```yumi file=lib/box.focus.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxFocus is facet of Box
    sees BoxScroll

────────────────────
+ focus-text String

"${pinned()} $width"

```

```yumi file=lib/box.scroll.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxScroll is facet of Box

^ offset = 3

────────────────────
^ pinned Bool

offset > 3

────────────────────
^ do keep-end

offset = width + 1

```

```yumi file=lib/box.tools.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ tools-text String
    - box Box

"${box.offset} ${box.pinned()} ${box.width} ${Box(4).width}"

```

```output
11 true 10 11 true 10 4
3 false 10 3 false 10 4
```

### Code outside a class's file family can't use a member declared with ^, nor can a class below it there, and a module declares its own values, functions and types with + or _

```yumi compile-error
./lib/other

keep()
-- other-text()
```

```yumi file=lib/box.yumi

./box.scroll

━━━━━━━━━━━━━━━━━━━━
+ Box

^ width = 10

────────────────────
+ This

────────────────────
^ This
    - start Int

width = start

```

```yumi file=lib/box.scroll.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxScroll is facet of Box

^ offset = 3

────────────────────
^ do keep-end

offset = width + 1

```

```yumi file=lib/other.yumi

./box
./helper
./kinds

^ limit = 3

━━━━━━━━━━━━━━━━━━━━
+ other-text String

"${width()} ${offset()} ${made()} ${Panel().panel-text()}"

━━━━━━━━━━━━━━━━━━━━
+ do keep

Box().keep-end()

━━━━━━━━━━━━━━━━━━━━
+ width Int

Box().width

━━━━━━━━━━━━━━━━━━━━
+ offset Int

Box().offset

━━━━━━━━━━━━━━━━━━━━
+ made Int

Box(5).width

━━━━━━━━━━━━━━━━━━━━
+ Panel is Box

────────────────────
+ panel-text String

"${this.width}"

```

```yumi file=lib/helper.yumi

━━━━━━━━━━━━━━━━━━━━
^ helper Int

1

```

```yumi file=lib/kinds.yumi

━━━━━━━━━━━━━━━━━━━━
^ Kind

```

```output
Exception: Only a member can be shared with its file family; declare this + or _
<lib/helper.yumi:3>
    ^ helper Int
    ^^^^^^^^^^^

Exception: Only a member can be shared with its file family; declare this + or _
<lib/kinds.yumi:3>
    ^ Kind
    ^^^^^

Exception: Only a member can be shared with its file family; declare this + or _
<lib/other.yumi:6>
    ^ limit = 3
    ^^^^^^^^^^^

Exception: Method "keep-end" of Box is shared, declared with ^, so only its own file family can call it
<lib/other.yumi:16>
    Box().keep-end()
    ^^^^^^^^^^^^^^^^

Exception: Property "width" is shared, declared with ^, so only its own file family can use it
<lib/other.yumi:21>
    Box().width
    ^^^^^^^^^^^

Exception: Property "offset" is shared, declared with ^, so only its own file family can use it
<lib/other.yumi:26>
    Box().offset
    ^^^^^^^^^^^^

Exception: No constructor of Box accepts (I64); its declared constructors are "Box()"; "Box(start Int)" is shared with its file family
<lib/other.yumi:31>
    Box(5).width
    ^^^

Exception: Property "width" is shared, declared with ^, so only its own file family can use it
<lib/other.yumi:39>
    "${this.width}"
       ^^^^
```

### A member declared with ^ takes its name as a public one does: no two facets of a class declare it, and a class below the class declares it only as a public one's

```yumi compile-error
./lib/panel

-- 1
```

```yumi file=lib/box.yumi

./box.focus
./box.scroll

━━━━━━━━━━━━━━━━━━━━
+ Box

```

```yumi file=lib/box.focus.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxFocus is facet of Box

^ step = 1

```

```yumi file=lib/box.scroll.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxScroll is facet of Box

+ step = 2

```

```yumi file=lib/panel.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ Panel is Box

_ step => 3

```

```output
Exception: Facets BoxFocus and BoxScroll of Box both declare step; a facet's members that are not private are its class's, so a name is one facet's
<lib/box.focus.yumi:7>
    ^ step = 1
    ^^^^^^^^^^

Exception: Facets BoxScroll and BoxFocus of Box both declare step; a facet's members that are not private are its class's, so a name is one facet's
<lib/box.scroll.yumi:7>
    + step = 2
    ^^^^^^^^^^

Exception: Panel can't declare step, since facet BoxFocus of Box declares it
<lib/panel.yumi:7>
    _ step => 3
    ^^^^^^^^^^^
```

### A facet declares events, which its class's objects publish: the facet, the class and the facets that see it name one alone, and other code reaches it on the object

```yumi
./lib/box

. box = Box()
. other = Box()
box.watch(other)
for value on box.scrolled
    -- "heard $value"
box.scroll-by(3)
box.jump()
box.nudge()
other.scroll-by(7)
```

```yumi file=lib/box.yumi

./box.focus
./box.scroll

━━━━━━━━━━━━━━━━━━━━
+ Box

────────────────────
+ do jump

scrolled <- 10

```

```yumi file=lib/box.scroll.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxScroll is facet of Box

+ event scrolled Int

────────────────────
+ do scroll-by
    - amount Int

scrolled <- amount

```

```yumi file=lib/box.focus.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxFocus is facet of Box
    sees BoxScroll

────────────────────
+ do nudge

scrolled <- 1

────────────────────
+ do watch
    - other Box

for value on other.scrolled
    -- "focus heard $value"

```

```output
heard 3
heard 10
heard 1
focus heard 7
```

### A facet's private event is its own file's and its shared one its class's family's, and closing an object ends what subscribes to its facets' events

```yumi
./lib/box

. box = Box()
box.listen()
box.track()
for value on box.scrolled
    -- "heard $value"
box.scroll-by(1)
box.close()
box.scroll-by(2)
-- "done"
```

```yumi file=lib/box.yumi

./box.scroll

━━━━━━━━━━━━━━━━━━━━
+ Box

────────────────────
+ do listen

for value on pinned
    -- "pinned $value"

────────────────────
+ do close

-- "closed"

```

```yumi file=lib/box.scroll.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxScroll is facet of Box

+ event scrolled Int
^ event pinned Bool
_ event moved Int

────────────────────
+ do track

for value on moved
    -- "moved $value"

────────────────────
+ do scroll-by
    - amount Int

moved <- amount
pinned <- (amount > 1)
scrolled <- amount

```

```output
moved 1
pinned false
heard 1
closed
done
```

### A facet's public event bubbles on an EventNode class, and an ancestor outside the family names it by its class

```yumi
./lib/picker

. root = Panel("root")
. inner = Picker("inner", root)
inner.pick("a")

━━━━━━━━━━━━━━━━━━━━
+ Panel is Node

────────────────────
for value on Picker.picked

-- "$name heard $value"
```

```yumi file=lib/picker.yumi

./picker.pick

━━━━━━━━━━━━━━━━━━━━
+ Node
    is EventNode

+ name String
+ up Node?

────────────────────
+ This
    - name
    - up = null

────────────────────
+ get event-parent EventNode?

up

━━━━━━━━━━━━━━━━━━━━
+ Picker is Node

```

```yumi file=lib/picker.pick.yumi

./picker

━━━━━━━━━━━━━━━━━━━━
+ PickerPick is facet of Picker

+ event picked String

────────────────────
+ do pick
    - text String

picked <- text

```

```output
root heard a
```

### Code reaches a facet's events as it reaches its other members, an event's name is its class's alone, and a facet's event carries a value

```yumi compile-error
./lib/box
./lib/other
./lib/panel

. box = Box()
box.peek()
box.nudge()
box.watch(box)
hear(box)
```

```yumi file=lib/box.yumi

./box.blink
./box.focus
./box.held
./box.scroll

━━━━━━━━━━━━━━━━━━━━
+ Box

────────────────────
+ do peek

for value on this.moved
    -- value

```

```yumi file=lib/box.scroll.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxScroll is facet of Box

+ event scrolled Int
^ event pinned Bool
_ event moved Int
_ event shaken Int

```

```yumi file=lib/box.focus.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxFocus is facet of Box

_ event shaken Int

────────────────────
+ do nudge

scrolled <- 1

────────────────────
+ do watch
    - other Box

for value on other.scrolled
    -- value

```

```yumi file=lib/box.blink.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxBlink is facet of Box

+ event blinked Void

```

```yumi file=lib/box.held.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxHeld is facet of Box

_ held = 1
_ event held Int

```

```yumi file=lib/other.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ do hear
    - box Box

for value on box.pinned
    -- value

```

```yumi file=lib/panel.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ Panel is Box

+ event scrolled Int

```

```output
Exception: An event gives each subscriber a value, so it cannot carry Void; an event that only signals carries Bool, as in "+ event blinked Bool"
<lib/box.blink.yumi:7>
    + event blinked Void
                    ^^^^

Exception: Facets BoxFocus and BoxScroll of Box both declare shaken; an event's name is its class's alone, private or not
<lib/box.focus.yumi:7>
    _ event shaken Int
    ^^^^^^^^^^^^^^^^^^

Exception: Name "scrolled" not found; facet BoxScroll declares it, and only the facets a facet sees are in reach. Write "sees BoxScroll" under the facet that uses it
<lib/box.focus.yumi:12>
    scrolled <- 1
    ^^^^^^^^

Exception: Box has no property "scrolled" here; facet BoxScroll declares it, and only the facets a facet sees are in reach. Write "sees BoxScroll" under the facet that uses it
<lib/box.focus.yumi:18>
    for value on other.scrolled
                 ^^^^^^^^^^^^^^

Exception: Redefinition of class member
<lib/box.held.yumi:8>
    _ event held Int
    ^^^^^^^^^^^^^^^^

Exception: Event "pinned" is shared, declared with ^, so only its own file family can use it
<lib/box.scroll.yumi:8>
    ^ event pinned Bool
    ^^^^^^^^^^^^^^^^^^^

Exception: Event "moved" is private
<lib/box.scroll.yumi:9>
    _ event moved Int
    ^^^^^^^^^^^^^^^^^

Exception: Facets BoxScroll and BoxFocus of Box both declare shaken; an event's name is its class's alone, private or not
<lib/box.scroll.yumi:10>
    _ event shaken Int
    ^^^^^^^^^^^^^^^^^^

Exception: Panel can't declare scrolled, since facet BoxScroll of Box declares it
<lib/panel.yumi:7>
    + event scrolled Int
    ^^^^^^^^^^^^^^^^^^^^
```

### A class below one with facets overrides a facet's methods, computed properties and setters, which code calls through the owner's type and the facet through this, and its override reaches the facet's own through super

```yumi
./lib/box
./lib/panel

. box Box = Panel()
box.title = "a"
-- box.describe()
-- box.label
-- box.shout()
-- box.last
. plain = Box()
plain.title = "b"
-- plain.describe()
-- plain.label
-- plain.shout()
-- plain.last
```

```yumi file=lib/box.yumi

./box.text

━━━━━━━━━━━━━━━━━━━━
+ Box

```

```yumi file=lib/box.text.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxText is facet of Box

+ last = ""
+ label => "box"

────────────────────
+ describe String

"plain $label"

────────────────────
+ shout String

"${describe()}!"

────────────────────
+ do title = value String

last = "box $value"

```

```yumi file=lib/panel.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ Panel is Box

+ label => "panel"

────────────────────
+ describe String

"fancy ${super.describe()}"

────────────────────
+ do title = value String

last = "panel $value"

```

```output
fancy plain panel
panel
fancy plain panel!
panel a
plain box
box
plain box!
box b
```

### A constructor's argument named after a facet's property gives it its value, a class below gives a facet's stored and lazy properties new values, which the facet's own code reads, and a facet's private property takes no argument's value outside its file

```yumi
./lib/box
./lib/panel

. box = Box(3, 4, 5, 6)
-- box.reach()
-- box.cached
-- box.secret()
-- Box().offset
-- Box().cached
-- Panel(8).offset
-- Panel().offset
-- Panel().reach()
-- Panel().cached
-- Tab().offset
-- Tab().cached
```

```yumi file=lib/box.yumi

./box.scroll

━━━━━━━━━━━━━━━━━━━━
+ Box

+ size = 1

────────────────────
+ This
    - offset Int
    - limit Int
    - cached Int
    - hidden Int

size = hidden

────────────────────
+ This

```

```yumi file=lib/box.scroll.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxScroll is facet of Box

+ offset = 0
^ limit = 1
+ cached := size * 100
_ hidden = 2

────────────────────
+ reach Int

offset + limit

────────────────────
+ secret Int

hidden + size

```

```yumi file=lib/panel.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ Panel is Box

+ offset = 9
+ cached := size * 7

────────────────────
+ This
    - offset Int

────────────────────
+ This

━━━━━━━━━━━━━━━━━━━━
+ Tab is Panel

```

```output
7
5
8
0
100
8
9
10
7
9
7
```

### A facet uses styles as its class would: the class's objects have them wherever the class is, the facet reads them as its own, and markup writes them on the class's elements

```yumi
/core/style
/core/markup
/core/tests/support/markup-fixtures
./lib/card

<Deck demo
    <Card
        ink = 9
        margin = 3

. card = demo.%first-child! as Card
-- card.describe()
demo.markup-batch-queue.checkpoint()
-- card.describe()
-- card.$ink

━━━━━━━━━━━━━━━━━━━━
+ Deck is Box
    is MarkupRootElement

+ markup-batch-queue := MarkupBatchQueue(this)

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush
```

```yumi file=lib/card.yumi

/core/style
/core/tests/support/markup-fixtures
./card.paint

━━━━━━━━━━━━━━━━━━━━
+ Card is Box

━━━━━━━━━━━━━━━━━━━━
+ CardStyle is Style

+ @inherited ink I64 = 7
+ margin I64 = 0

. ink -> paint
. margin -> layout

```

```yumi file=lib/card.paint.yumi

/core/style
./card

━━━━━━━━━━━━━━━━━━━━
+ CardPaint is facet of Card
    using CardStyle

────────────────────
+ do describe String

. ink = $ink
. margin = $margin
= "ink=$ink margin=$margin"

```

```output
ink=7 margin=0
ink=9 margin=3
9
```

### A facet's sections, when and whenever rules and dependencies run for its class's objects, after the class's own and before those of a class below

```yumi
./lib/box
./lib/panel

. box = Box()
box.move(3)
box.nudge(4)
-- ", ".join(box.log)
-- box.seen
. panel = Panel()
panel.move(5)
-- ", ".join(panel.log)
```

```yumi file=lib/box.yumi

./box.watch

━━━━━━━━━━━━━━━━━━━━
+ Box

+ event moved Int
+ log = List'String()

────────────────────
for value on moved

log += "box $value"

────────────────────
+ do move
    - amount Int

moved <- amount

```

```yumi file=lib/box.watch.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxWatch is facet of Box

+ event nudged Int
+ seen = 0

. seen-changed < seen

────────────────────
for value on moved

log += "watch $value"
seen += value

────────────────────
for value on nudged

log += "nudged $value"

────────────────────
+ do nudge
    - amount Int

nudged <- amount

────────────────────
whenever #seen

log += "seen $seen"

────────────────────
when #seen > 4

log += "big"

────────────────────
+ do seen-changed

log += "changed $seen"

```

```yumi file=lib/panel.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ Panel is Box

────────────────────
for value on moved

log += "panel $value"

```

```output
box 3, watch 3, changed 3, seen 3, nudged 4
3
box 5, watch 5, changed 5, seen 5, big, panel 5
```

### A facet is part of a class or of another facet of it, declared in the class's file family, and sees only other facets of the class

```yumi compile-error
./lib/box
./lib/other

-- 1
```

```yumi file=lib/box.yumi

./box.focus
./box.proto
./box.ring
./box.scroll
./box.view

━━━━━━━━━━━━━━━━━━━━
+ Box

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

```

```yumi file=lib/box.scroll.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxScroll is facet of Bxo

```

```yumi file=lib/box.focus.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxFocus is facet of Box
    sees Box

```

```yumi file=lib/box.view.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxView is facet of Box
    sees BoxNothing

```

```yumi file=lib/box.ring.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ RingA is facet of RingB

━━━━━━━━━━━━━━━━━━━━
+ RingB is facet of RingA

```

```yumi file=lib/box.proto.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ ProtoPart is facet of Proto

```

```yumi file=lib/other.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ Extra is facet of Box

```

```output
Exception: "Box" is no other facet of Box, so BoxFocus can't see it
<lib/box.focus.yumi:6>
        sees Box
             ^^^

Exception: ProtoPart can't be part of Proto, since only a class has facets
<lib/box.proto.yumi:5>
    + ProtoPart is facet of Proto
                            ^^^^^

Exception: RingA is part of itself, through the facets it is part of
<lib/box.ring.yumi:5>
    + RingA is facet of RingB
                        ^^^^^

Exception: "Bxo" names no class or facet that BoxScroll can be part of
<lib/box.scroll.yumi:5>
    + BoxScroll is facet of Bxo
                            ^^^

Exception: "BoxNothing" is no other facet of Box, so BoxView can't see it
<lib/box.view.yumi:6>
        sees BoxNothing
             ^^^^^^^^^^

Exception: Extra is a facet of Box, so it is declared in the file family of box.yumi, in a module named after it
<lib/other.yumi:5>
    + Extra is facet of Box
                        ^^^
```

### A facet holds what a class body may but for what only a class has, and markup, and its stored properties have values

```yumi compile-error
./lib/box

-- 1
```

```yumi file=lib/box.yumi

./box.base
./box.count
./box.make
./box.markup
./box.params
./box.static

━━━━━━━━━━━━━━━━━━━━
+ Box

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

```

```yumi file=lib/box.params.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxParams'T is facet of Box

```

```yumi file=lib/box.base.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxBase is facet of Box is Proto

```

```yumi file=lib/box.count.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxCount is facet of Box

+ label String? = null
+ parent Box?

+ count Int

```

```yumi file=lib/box.make.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxMake is facet of Box

────────────────────
+ This

```

```yumi file=lib/box.markup.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxMarkup is facet of Box

────────────────────

<This

```

```yumi file=lib/box.static.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxStatic is facet of Box

────────────────────
+ static make Int

1

```

```output
Exception: BoxBase is a facet, part of its owner, so it derives from no class
<lib/box.base.yumi:5>
    + BoxBase is facet of Box is Proto
                                 ^^^^^

Exception: count needs a value or an optional type, since BoxCount is a facet and no constructor gives it one
<lib/box.count.yumi:10>
    + count Int
    ^^^^^^^^^^^

Exception: BoxMake is a facet, so it has no constructors; its owner's give its properties their first values
<lib/box.make.yumi:8>
    + This
    ^^^^^

Exception: BoxMarkup is a facet, so it writes no markup; a content section is its owner's alone
<lib/box.markup.yumi:7>
    ────────────────────
    ^^^^^^^^^^^^^^^^^^^

Exception: BoxParams is a facet, so it takes no type parameters
<lib/box.params.yumi:5>
    + BoxParams'T is facet of Box
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Exception: BoxStatic is a facet, so it has no static functions
<lib/box.static.yumi:8>
    + static make Int
    ^^^^^^^^^^^^^^^^
```

### A facet's member may not take a name its class answers, nor a public name another facet of the class declares

```yumi compile-error
./lib/box

Box()
```

```yumi file=lib/box.yumi

./box.depth
./box.focus
./box.scroll
./box.width

━━━━━━━━━━━━━━━━━━━━
+ Base

+ depth = 0

━━━━━━━━━━━━━━━━━━━━
+ Box is Base

_ width = 10

```

```yumi file=lib/box.width.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxWidth is facet of Box

_ width = 1

```

```yumi file=lib/box.depth.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxDepth is facet of Box

────────────────────
+ depth Int

2

```

```yumi file=lib/box.scroll.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxScroll is facet of Box

+ shared = 3

```

```yumi file=lib/box.focus.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxFocus is facet of Box

+ shared = 4

```

```output
Exception: depth is already a member of Box, so facet BoxDepth can't declare it
<lib/box.depth.yumi:8>
    + depth Int
    ^^^^^^^^^^

Exception: Facets BoxFocus and BoxScroll of Box both declare shared; a facet's members that are not private are its class's, so a name is one facet's
<lib/box.focus.yumi:7>
    + shared = 4
    ^^^^^^^^^^^^

Exception: Facets BoxScroll and BoxFocus of Box both declare shared; a facet's members that are not private are its class's, so a name is one facet's
<lib/box.scroll.yumi:7>
    + shared = 3
    ^^^^^^^^^^^^

Exception: width is already a member of Box, so facet BoxWidth can't declare it
<lib/box.width.yumi:7>
    _ width = 1
    ^^^^^^^^^^^
```

### A class below one with facets can't declare a name a facet declares but to override a method, getter or setter of the facet with one of its kind or to give a property it stores a new value, and a constructor's argument can't give a facet's private property its value in the facet's file

```yumi compile-error
./lib/box
./lib/pane
./lib/panel

-- Box().tab().width
```

```yumi file=lib/panel.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ Panel is Box

+ offset => 9

```

```yumi file=lib/pane.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ Pane is Box

+ nudge => 2

```

```yumi file=lib/box.yumi

./box.scroll

━━━━━━━━━━━━━━━━━━━━
+ Box

+ width = 10

```

```yumi file=lib/box.scroll.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxScroll is facet of Box

+ offset = 3
_ hidden = 2

────────────────────
+ nudge Int

1

────────────────────
+ tab Box

Tab(1)

━━━━━━━━━━━━━━━━━━━━
+ Tab is Box

────────────────────
+ This
    - hidden Int

```

```output
Exception: hidden is private to facet BoxScroll, so a constructor's argument can't give it a value
<lib/box.scroll.yumi:25>
        - hidden Int
          ^^^^^^^^^^

Exception: Pane can't declare nudge, since facet BoxScroll of Box declares it
<lib/pane.yumi:7>
    + nudge => 2
    ^^^^^^^^^^^^

Exception: Panel can't declare offset, since facet BoxScroll of Box declares it
<lib/panel.yumi:7>
    + offset => 9
    ^^^^^^^^^^^^^
```

### A facet declares lazy and computed properties, getters and setters, operators and conversions, and reaches the private values, types, functions and constructors of its class's module

```yumi
./lib/box

. box = Box()
-- box.cached
-- box.twice
box.label = "abcde"
-- box.total
-- box.label
-- box + 10
-- box as String
-- box.reached()
-- box.copy().size-text()
```

```yumi file=lib/box.yumi

./box.extra

_ secret = 7

━━━━━━━━━━━━━━━━━━━━
+ Box

_ size = 4

────────────────────
+ This

────────────────────
_ This
    - start Int

size = start

────────────────────
+ size-text String

"size $size"

━━━━━━━━━━━━━━━━━━━━
_ Hidden is Tuple

+ n Int

━━━━━━━━━━━━━━━━━━━━
_ helper Int
    - n Int

n * 10

```

```yumi file=lib/box.extra.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxExtra is facet of Box

+ total = 1
+ cached := size * 3
+ twice => total * 2

────────────────────
+ get label String

"label $total"

────────────────────
+ do label = value String

total = value.length

────────────────────
+ this + more Int

total + more

────────────────────
+ to String

"box of $size"

────────────────────
+ reached Int

helper(Hidden(secret).n)

────────────────────
+ copy Box

Box(size + 1)

```

```output
12
2
5
label 5
15
box of 4
70
size 5
```

### A class's constructors give its own properties their values first, then each facet's, by the name of the facet's module and in declaration order within it, whatever order the modules are imported in

```yumi
./lib/box

. box = Box()
-- " ".join(order)
```

```yumi file=lib/box.yumi

./box.zeta
./box.alpha

+ order = List'String()

━━━━━━━━━━━━━━━━━━━━
+ Box

+ own = note("own")

━━━━━━━━━━━━━━━━━━━━
+ do note Int
    - name String

order += name
= 0

```

```yumi file=lib/box.zeta.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxZeta is facet of Box

+ z1 = note("zeta 1")
+ z2 = note("zeta 2")

```

```yumi file=lib/box.alpha.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxAlpha is facet of Box

+ a = note("alpha")

```

```output
own alpha zeta 1 zeta 2
```

### A facet may be declared in its class's own module, whose code reaches every facet of the class, as code outside the facets' modules does

```yumi
./lib/box

-- Box().both()
```

```yumi file=lib/box.yumi

./box.focus

━━━━━━━━━━━━━━━━━━━━
+ Box

────────────────────
+ both String

"$offset $focused"

━━━━━━━━━━━━━━━━━━━━
+ BoxScroll is facet of Box

+ offset = 3

```

```yumi file=lib/box.focus.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxFocus is facet of Box

+ focused = true

```

```output
3 true
```

### A facet of a class below one with facets can't take the name of a public member of a facet of a class above, which its class already has

```yumi compile-error
./lib/window

-- Window().offset
```

```yumi file=lib/box.yumi

./box.scroll

━━━━━━━━━━━━━━━━━━━━
+ Box

```

```yumi file=lib/box.scroll.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxScroll is facet of Box

+ offset = 3

```

```yumi file=lib/window.yumi

./box
./window.pane

━━━━━━━━━━━━━━━━━━━━
+ Window is Box

```

```yumi file=lib/window.pane.yumi

./window

━━━━━━━━━━━━━━━━━━━━
+ WindowPane is facet of Window

+ offset = 8

```

```output
Exception: offset is already a member of Window, so facet WindowPane can't declare it
<lib/window.pane.yumi:7>
    + offset = 8
    ^^^^^^^^^^^^
```

### A facet's name is a type. A value of its class or of a class below becomes one unasked, as does a facet's value the facets it is part of, and has the members of its facet and of those, which a class below that declares one again answers for. It becomes its class again only when asked, is tested for a class below as its class is, and its type information is its class's

```yumi
./lib/box

-- describe(Panel())
-- describe(Box())
```

```yumi file=lib/box.yumi

./box.scroll
./box.scroll.bar

━━━━━━━━━━━━━━━━━━━━
+ Box

+ width = 10

━━━━━━━━━━━━━━━━━━━━
+ Panel is Box

+ height = 4

────────────────────
+ do scroll-to
    - point Int

offset = point * 100

━━━━━━━━━━━━━━━━━━━━
+ height-of Int
    - scroll BoxScroll

match scroll
    | panel Panel
        panel.height
    ??
        0

━━━━━━━━━━━━━━━━━━━━
+ do describe String
    - box Box

. bar BoxScrollBar = box
. scroll BoxScroll = bar
scroll.scroll-to(2)
bar.shown = true
. maybe BoxScroll? = if bar.shown then scroll else null
. either = if scroll.offset > 100 then box else scroll
. all [BoxScroll] = [scroll, box]
. owner = scroll as Box
. shown = "${scroll.offset} ${scroll.doubled} ${scroll.area} ${bar.offset} ${bar.shown} ${maybe!.offset}"
= "$shown ${either.offset} ${all.length} ${owner.width} ${scroll is Panel} ${height-of(scroll)} ${type-of scroll}"

```

```yumi file=lib/box.scroll.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxScroll is facet of Box

+ offset = 3
+ doubled => offset * 2
+ area := width * 2

────────────────────
+ do scroll-to
    - point Int

offset = point + width

```

```yumi file=lib/box.scroll.bar.yumi

./box
./box.scroll

━━━━━━━━━━━━━━━━━━━━
+ BoxScrollBar is facet of BoxScroll

+ shown = false

```

```output
200 400 20 200 true 200 200 2 10 true 4 [TypeInfo Box]
12 24 20 12 true 12 12 2 10 false 0 [TypeInfo Box]
```

### Code outside a class's file family names the class's facets by importing the class's module, which brings their names and nothing else of their modules

```yumi
./lib/box

-- offset-of(Box())

━━━━━━━━━━━━━━━━━━━━
+ offset-of Int
    - scroll BoxScroll

scroll.offset
```

```yumi file=lib/box.yumi

./box.scroll

━━━━━━━━━━━━━━━━━━━━
+ Box

```

```yumi file=lib/box.scroll.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxScroll is facet of Box

+ offset = 3

```

```output
3
```

### A facet's value becomes its class only when asked and has no members of its class or of facets it is not part of, a value is not tested for a facet, a facet has no constructor, a stored property of a facet's type needs a value, a facet's value reaches no member of a facet the module's facet does not see, and code that does not import the class's module can't name its facets

```yumi compile-error
./lib/box
./lib/other

Holder()
widen(Box())
owned(Box())
sibling(Box())
tested(Box())
matched(Box())
made()
Box().peek()
other-offset()
```

```yumi file=lib/box.yumi

./box.focus
./box.scroll

━━━━━━━━━━━━━━━━━━━━
+ Box

+ width = 10

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ held BoxScroll

━━━━━━━━━━━━━━━━━━━━
+ scrolled BoxScroll
    - box Box

box

━━━━━━━━━━━━━━━━━━━━
+ widen Box
    - scroll BoxScroll

scroll

━━━━━━━━━━━━━━━━━━━━
+ owned Int
    - scroll BoxScroll

scroll.width

━━━━━━━━━━━━━━━━━━━━
+ sibling Bool
    - scroll BoxScroll

scroll.focused

━━━━━━━━━━━━━━━━━━━━
+ tested Bool
    - box Box

box is BoxScroll

━━━━━━━━━━━━━━━━━━━━
+ matched Int
    - box Box

match box
    | BoxScroll
        1
    ??
        2

━━━━━━━━━━━━━━━━━━━━
+ made BoxScroll

BoxScroll()

```

```yumi file=lib/box.focus.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxFocus is facet of Box

+ focused = false

────────────────────
+ peek Int

scrolled(this).offset

```

```yumi file=lib/box.scroll.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxScroll is facet of Box

+ offset = 3

```

```yumi file=lib/other.yumi

━━━━━━━━━━━━━━━━━━━━
+ do other-offset Int

. scroll BoxScroll? = null
= 0

```

```output
Exception: BoxScroll has no property "offset" here; facet BoxScroll declares it, and only the facets a facet sees are in reach. Write "sees BoxScroll" under the facet that uses it
<lib/box.focus.yumi:12>
    scrolled(this).offset
    ^^^^^^^^^^^^^^^^^^^^^

Exception: held is a BoxScroll, a facet of Box, which has no constructor to give it its first value; give it a value, or make it optional
<lib/box.yumi:13>
    + held BoxScroll
    ^^^^^^^^^^^^^^^^

Exception: Expected Box here, but this is BoxScroll, which becomes Box only when asked; write "as Box"
<lib/box.yumi:25>
    scroll
    ^^^^^^

Exception: BoxScroll has no property "width"; a facet's value has only its facet's members and those of the facets it is part of. Write "as Box" to reach the rest of Box
<lib/box.yumi:31>
    scroll.width
    ^^^^^^^^^^^^

Exception: BoxScroll has no property "focused"; a facet's value has only its facet's members and those of the facets it is part of. Write "as Box" to reach the rest of Box
<lib/box.yumi:37>
    scroll.focused
    ^^^^^^^^^^^^^^

Exception: BoxScroll is a facet, which every Box has, so no value is tested for it
<lib/box.yumi:43>
    box is BoxScroll
           ^^^^^^^^^

Exception: BoxScroll is a facet, which every Box has, so no value is tested for it
<lib/box.yumi:50>
        | BoxScroll
          ^^^^^^^^^

Exception: BoxScroll is a facet of Box, so it has no constructor; a Box is a BoxScroll
<lib/box.yumi:58>
    BoxScroll()
    ^^^^^^^^^

Exception: Type "BoxScroll" not found; BoxScroll is a facet of Box, whose name comes with Box's module. Import that module to name it
<lib/other.yumi:5>
    . scroll BoxScroll? = null
             ^^^^^^^^^
```

### A facet that names an interface makes its class and the classes below implement it through the facet's members, which a class below may override, and a class that names an interface may answer it with its facets' members

```yumi
./lib/box
./lib/kinds

. box = Box()
. scroll Scrollable = box
scroll.scroll-by(4)
-- scroll.position
. panel Scrollable = Panel()
panel.scroll-by(1)
-- panel.position
. named Named = box
-- named.label
. part BoxScroll = box
. again Scrollable = part as Box
-- again.position
```

```yumi file=lib/kinds.yumi

━━━━━━━━━━━━━━━━━━━━
+ Scrollable is Interface

────────────────────
+ get position Int

────────────────────
+ do scroll-by
    - amount Int

━━━━━━━━━━━━━━━━━━━━
+ Named is Interface

────────────────────
+ get label String

```

```yumi file=lib/box.yumi

./box.focus
./box.scroll
./kinds

━━━━━━━━━━━━━━━━━━━━
+ Box
    is Named

━━━━━━━━━━━━━━━━━━━━
+ Panel is Box

────────────────────
+ do scroll-by
    - amount Int

offset = amount * 10

```

```yumi file=lib/box.focus.yumi

./box

━━━━━━━━━━━━━━━━━━━━
+ BoxFocus is facet of Box

────────────────────
+ get label String

"box"

```

```yumi file=lib/box.scroll.yumi

./box
./kinds

━━━━━━━━━━━━━━━━━━━━
+ BoxScroll is facet of Box
    is Scrollable

+ offset = 3

────────────────────
+ get position Int

offset

────────────────────
+ do scroll-by
    - amount Int

offset += amount

```

```output
7
10
box
7
```

### A facet names only interfaces, answers each with its members, and its value becomes an interface only as its class

```yumi compile-error
./lib/box
./lib/kinds

. part BoxScroll = Box()
. scroll Scrollable = part
. whole Scrollable = Box()
```

```yumi file=lib/kinds.yumi

━━━━━━━━━━━━━━━━━━━━
+ Scrollable is Interface

────────────────────
+ get position Int

────────────────────
+ do scroll-by
    - amount Int

━━━━━━━━━━━━━━━━━━━━
+ Base

```

```yumi file=lib/box.yumi

./box.scroll
./kinds

━━━━━━━━━━━━━━━━━━━━
+ Box

```

```yumi file=lib/box.scroll.yumi

./box
./kinds

━━━━━━━━━━━━━━━━━━━━
+ BoxScroll is facet of Box
    is Scrollable
    is Base

────────────────────
+ do scroll-by
    - amount Int

```

```output
Exception: "scroll" is declared Scrollable, but its value is BoxScroll, a facet's value, which becomes only the facets it is part of unasked; write "as Box" to convert its class
<no file:6>
    . scroll Scrollable = part
      ^^^^^^^^^^^^^^^^^

Exception: BoxScroll cannot implement Base, which is not an interface; a facet is part of its class and is built on no type
<lib/box.scroll.yumi:8>
        is Base
           ^^^^

Exception: Box does not implement "position", which Scrollable declares without a body; declare "position" in BoxScroll
<lib/box.yumi:6>
    + Box
    ^^^^
Interface:
<lib/kinds.yumi:6>
    + get position Int
```
