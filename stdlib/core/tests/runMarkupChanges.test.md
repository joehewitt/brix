# Markup changes

```yumi
<Foo a
    <Foo b
        name = "B"
    <Foo c
        name = b.#name

-- c.name
b.name = "BB"
-- c.name
b.name = "BBB"
-- c.name

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ name String
+ contents Foo?

────────────────────
+ do this += child Foo

contents = child
```

```output
B
BB
BBB
```

```yumi
.. binder = Binder(Foo("bob"))
<Foo foo = "root"
    binder.#thing

-- foo.content?.name
binder.thing = Foo("tom")
-- foo.content?.name
binder.thing = Foo("fred")
-- foo.content?.name

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ name String
+ content Foo?

────────────────────
+ This
    - name

────────────────────
+ do replace
    - child Foo
    - replacement Foo

content = replacement

────────────────────
+ do this += child Foo

content = child

━━━━━━━━━━━━━━━━━━━━
+ Binder

+ thing Foo

────────────────────
+ This
    - thing
```

```output
bob
tom
fred
```

```yumi
.. some-name = "bob"

<Foo a
    name = #some-name

-- a.name
some-name = "tom"
-- a.name
some-name = "fred"
-- a.name

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ name String
```

```output
bob
tom
fred
```

```yumi
.. some-name = a.#name

<Foo a
    name = "bob"
    <Foo
        name = #some-name

-- some-name
-- a.contents?.name
a.name = "tom"
-- some-name
-- a.contents?.name

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ name String
+ contents Foo?

────────────────────
+ do this += child Foo

contents = child
```

```output
bob
bob
tom
tom
```

```yumi
.. some-name = a.#name

<Foo a
    name = "bob"
. b = <Foo
    name = #some-name

-- some-name
-- b.name
a.name = "tom"
-- some-name
-- b.name

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ name String
+ contents Foo?

────────────────────
+ do this += child Foo

contents = child
```

```output
bob
bob
tom
tom
```

```yumi
<Thing thing

-- thing.name
thing.prefix = " sr."
-- thing.name

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ prefix = ""

────────────────────

+ name = "bob" + #prefix
```

```output
bob
bob sr.
```

```yumi
.. some-name = "bob"

<Foo foo = #some-name

-- foo.name
some-name = "tom"
-- foo.name
some-name = "fred"
-- foo.name

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ name String

────────────────────
+ This
    - name
```

```output
bob
tom
fred
```

```yumi crash
.. some-name = "bob"

<Foo foo = #some-name

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ This
    - value String
```

```output
Exception: Tracked markup constructor argument must match a property
<no file:4>
    <Foo foo = #some-name
               ^^^^^^^^^^
```

```yumi
<Thing thing

.. name = thing.#name

-- name
thing.prefix = " sr."
-- name

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ prefix = ""

────────────────────

+ name = "bob" + #prefix
```

```output
bob
bob sr.
```

### A tracked read of a list follows changes made to the list in place.

```yumi
/core/tests/support/markup-fixtures

. demo = Demo()
-- demo.label()
demo.items += "a"
-- demo.label()
demo.items += "b"
-- demo.label()
demo.items[0] = "z"
-- demo.label()
demo.items.remove(0)
-- demo.label()
. old = demo.items
demo.items = List'String()
-- demo.label()
old += "stale"
-- demo.label()
demo.items += "c"
-- demo.label()

━━━━━━━━━━━━━━━━━━━━
+ Demo is Box

+ items [String] = List'String()

────────────────────

<This
    <TextBox = "${#items.length} ${",".join(#items)}"
    if #items.length > 1
        <TextBox = "many"

────────────────────
+ do label String

. texts [String]
collect(this, texts)
= "${texts.length}:${"|".join(texts)}"

────────────────────
+ do collect
    - node Box
    - texts [String]

for child in node.children
    if child is TextBox
        texts += (child as TextBox).text
    collect(child, texts)
```

```output
1:0 
1:1 a
2:2 a,b|many
2:2 z,b|many
1:1 b
1:0 
1:0 
1:1 c
```

### A tracked read of a list through another object follows the list.

```yumi
/core/tests/support/markup-fixtures

. model = Model()
. demo = Demo(model)
-- demo.label()
model.items += "a"
-- demo.label()
model.items += "b"
-- demo.label()

━━━━━━━━━━━━━━━━━━━━
+ Model

+ items [String] = List'String()

━━━━━━━━━━━━━━━━━━━━
+ Demo is Box

+ model Model

────────────────────
+ This
    - model

────────────────────

<This
    <TextBox = count-label(model.#items)

────────────────────
+ do label String

. texts [String]
collect(this, texts)
= "${texts.length}:${"|".join(texts)}"

────────────────────
+ do collect
    - node Box
    - texts [String]

for child in node.children
    if child is TextBox
        texts += (child as TextBox).text
    collect(child, texts)

━━━━━━━━━━━━━━━━━━━━
+ count-label
    - items [String]

"${items.length} items"
```

```output
1:0 items
1:1 items
1:2 items
```

### A markup loop over a tracked list still adds rows without rebuilding the others.

```yumi
/core/tests/support/markup-fixtures

. demo = Demo()
demo.items += "a"
demo.items += "b"
demo.items += "c"
-- demo.label()
-- built

+ built = 0

━━━━━━━━━━━━━━━━━━━━
+ Demo is Box

+ items [String] = List'String()

────────────────────

<This
    for item in #items
        <TextBox = stamp(item)

────────────────────
+ do label String

. texts [String]
collect(this, texts)
= "${texts.length}:${"|".join(texts)}"

────────────────────
+ do collect
    - node Box
    - texts [String]

for child in node.children
    if child is TextBox
        texts += (child as TextBox).text
    collect(child, texts)

━━━━━━━━━━━━━━━━━━━━
+ do stamp String
    - text String

built += 1
= text
```

```output
3:a|b|c
3
```

### A tracked read of an optional list follows the list once it has one.

```yumi
/core/tests/support/markup-fixtures

. demo = Demo()
-- demo.label()
demo.items = List'String()
-- demo.label()
demo.items! += "a"
-- demo.label()
demo.items = null
-- demo.label()

━━━━━━━━━━━━━━━━━━━━
+ Demo is Box

+ items [String]?

────────────────────

<This
    <TextBox = "${#items?.length ?? 9}"

────────────────────
+ do label String

. texts [String]
collect(this, texts)
= "${texts.length}:${"|".join(texts)}"

────────────────────
+ do collect
    - node Box
    - texts [String]

for child in node.children
    if child is TextBox
        texts += (child as TextBox).text
    collect(child, texts)
```

```output
1:9
1:0
1:1
1:9
```

```yumi
.. name = "bob"
<Foo first
    title = #name
<Foo second
    title = #name

-- "${first.title}/${second.title}"
first.close()
name = "tom"
-- "${first.title}/${second.title}"
second.close()
name = "fred"
-- "${first.title}/${second.title}"

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ title String

────────────────────
+ do close
```

```output
bob/bob
bob/tom
bob/tom
```

```yumi
.. name = "bob"
<Foo outgoing
    title = #name

.. binder = Binder(outgoing)
<Foo holder
    binder.#thing

-- "${outgoing.title}/${holder.content?.title}"
name = "tom"
-- "${outgoing.title}/${holder.content?.title}"
binder.thing = Foo("fred")
name = "sam"
-- "${outgoing.title}/${holder.content?.title}"

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ title = ""
+ content Foo?

────────────────────
+ This
    - title = ""

────────────────────
+ do replace
    - child Foo
    - replacement Foo

content = replacement

────────────────────
+ do this += child Foo

content = child

━━━━━━━━━━━━━━━━━━━━
+ Binder

+ thing Foo

────────────────────
+ This
    - thing
```

```output
bob/bob
tom/tom
tom/fred
```

```yumi
/core/tests/support/markup-fixtures

.. name = "bob"
<Titled outgoing
    title = #name
    <Titled nested
        title = #name

.. binder = Binder(outgoing)
<Box holder
    binder.#thing

-- "${outgoing.title}/${nested.title}/${shown(holder)}"
name = "tom"
-- "${outgoing.title}/${nested.title}/${shown(holder)}"
binder.thing = Titled("fred")
name = "sam"
-- "${outgoing.title}/${nested.title}/${shown(holder)}"

━━━━━━━━━━━━━━━━━━━━
+ Titled is Box

+ title = ""

────────────────────
+ This
    - title = ""

━━━━━━━━━━━━━━━━━━━━
+ Binder

+ thing Titled

────────────────────
+ This
    - thing

━━━━━━━━━━━━━━━━━━━━
+ do shown String
    - holder Box

for child in holder.children
    = (child as Titled).title
= "none"
```

```output
bob/bob/bob
tom/tom/tom
tom/tom/fred
```

### An error an attribute's value raises as the attribute is given again goes to unhandled-error, and the element keeps the value it had.

```yumi
/core/tests/support/markup-fixtures

unhandled-error = :: error => do
    -- "handled ${error.message}"

. panel = Panel()
-- (panel.%first-child! as Label).text
panel.value = "abcd"
-- (panel.%first-child! as Label).text
panel.value = "x"
-- (panel.%first-child! as Label).text

━━━━━━━━━━━━━━━━━━━━
+ do parse Int
    - text String

?! SyntaxError("bad")
    if text == "x"
= text.length

━━━━━━━━━━━━━━━━━━━━
+ Panel is Box

+ value = "ab"

────────────────────

<This
    <Label
        text = "${parse(#value)}"
```

```output
2
4
handled bad
4
```

### A line of a root `<This` that gives content and reads something tracked gives it again as what it read changes.

```yumi
<Holder holder
    name = "a"

-- holder.content?.name
holder.name = "b"
-- holder.content?.name

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ name String
+ content Item?

────────────────────

<This
    Item(#name)

────────────────────
+ do replace
    - child Item
    - replacement Item

content = replacement

────────────────────
+ do this += child Item

content = child

━━━━━━━━━━━━━━━━━━━━
+ Item

+ name String

────────────────────
+ This
    - name
```

```output
a
b
```

### A line of a root `<This` that is a tracked name alone gives what the name holds as content, and gives it again as that changes.

```yumi
<Holder holder
    item = Item("a")

-- holder.content?.name
holder.item = Item("b")
-- holder.content?.name

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ item Item
+ content Item?

────────────────────

<This
    #item

────────────────────
+ do replace
    - child Item
    - replacement Item

content = replacement

────────────────────
+ do this += child Item

content = child

━━━━━━━━━━━━━━━━━━━━
+ Item

+ name String

────────────────────
+ This
    - name
```

```output
a
b
```

## Lazies attached to a value

### A lazy attached to an attribute's value is part of the value: the attribute is given again as what the lazy reads changes, whether the value reads the lazy plainly or with "#", in a scope or in the block of a call, and a "#" read written in the value itself is still followed. Lazies of one name in several attributes are each their own.

```brix
(main
  (let count 1)
  (let step 10)
  (<> Board (id: board)
    (<> Text (text: "direct")
      (= note (str #count " seen"))
    )
    (<> Text (text: "lazy")
      (= note
        (scope (str shown " seen")
          (lazy shown #count)
        )
      )
    )
    (<> Text (text: "plain")
      (= note
        (scope (str shown " seen, step " #step)
          (lazy shown #count)
        )
      )
    )
    (<> Text (text: "tracked")
      (= note
        (scope (str #shown " seen")
          (lazy shown (* #count 2))
        )
      )
    )
    (<> Text (text: "call")
      (= note
        (call (. (call padded shown) trim)
          (block (lazy shown (+ #count #step)))
        )
      )
    )
  )
  (call (. board show))
  (= count 5)
  (call (. board show))
  (= step 20)
  (call (. board show))
  (= count 6)
  (call (. board show))
)

(fn-expr + padded String
  (param n Int)
  (str "  " n "  ")
)

(type + Text (is MarkupNode)
  (prop + text String)
  (prop + note _ (= ""))
  (fn-do + This _
    (param text _)
  )
)

(type + Board (is MarkupNode)
  (fn-do + show _
    (let parts (list String))
    (for (in child (call (of children-of Text)))
      (+= parts (str (. child text) ": " (. child note)))
    )
    (print (call (. ", " join) parts))
  )
)
```

```output
direct: 1 seen, lazy: 1 seen, plain: 1 seen, step 10, tracked: 2 seen, call: 11
direct: 5 seen, lazy: 5 seen, plain: 5 seen, step 10, tracked: 10 seen, call: 15
direct: 5 seen, lazy: 5 seen, plain: 5 seen, step 20, tracked: 10 seen, call: 25
direct: 6 seen, lazy: 6 seen, plain: 6 seen, step 20, tracked: 12 seen, call: 26
```

### A lazy attached to the value of an attribute in a row of a markup loop or in an arm of a markup `if` is part of the value.

```brix
(main
  (let count 1)
  (let names (list "a" "b"))
  (let open true)
  (<> Board (id: board)
    (for (in name names)
      (<> Text (text: name)
        (= note
          (scope (str shown " seen")
            (lazy shown #count)
          )
        )
      )
    )
    (if #open
      (<> Text (text: "arm")
        (= note
          (scope (str #shown " seen")
            (lazy shown (* #count 2))
          )
        )
      )
    )
  )
  (call (. board show))
  (= count 5)
  (call (. board show))
  (= count 6)
  (call (. board show))
)

(type + Text (is MarkupNode)
  (prop + text String)
  (prop + note _ (= ""))
  (fn-do + This _
    (param text _)
  )
)

(type + Board (is MarkupNode)
  (fn-do + show _
    (let parts (list String))
    (for (in child (call (of children-of Text)))
      (+= parts (str (. child text) ": " (. child note)))
    )
    (print (call (. ", " join) parts))
  )
)
```

```output
a: 1 seen, b: 1 seen, arm: 2 seen
a: 5 seen, b: 5 seen, arm: 10 seen
a: 6 seen, b: 6 seen, arm: 12 seen
```

### A lazy attached to the value of an attribute in a class's markup, in a row of a loop, in an arm of an `if` or on an element alone, is part of the value where the value reads it plainly, reads nothing tracked itself and only formats it.

```brix
(import /core/markup)

(main
  (<> Board (id: board))
  (let form (. board form))
  (call (. board show))
  (= (. form count) 5)
  (call (. board show))
  (= (. form count) 6)
  (call (. board show))
)

(type + Form
  (prop + count _ (= 1))
)

(type + Text (is MarkupNode)
  (prop + text String)
  (prop + note _ (= ""))
  (fn-do + This _
    (param text _)
  )
)

(type + Board (is MarkupNode)
  (prop + form _ (= (call Form)))
  (prop + names _ (= (list "a" "b")))
  (prop + open _ (= true))
  (fn-do + show _
    (let parts (list String))
    (for (in child (call (of children-of Text)))
      (+= parts (str (. child text) ": " (. child note)))
    )
    (print (call (. ", " join) parts))
  )
  (content
    (for (in name #names)
      (<> Text (text: name)
        (= note (scope (str n " seen") (lazy n (. form #count))))
      )
    )
    (if #open
      (<> Text (text: "arm")
        (= note (scope (str n " arm") (lazy n (* (. form #count) 2))))
      )
    )
    (<> Text (text: "alone")
      (= note (scope (str n " alone") (lazy n (. form #count))))
    )
  )
)
```

```output
a: 1 seen, b: 1 seen, arm: 2 arm, alone: 1 alone
a: 5 seen, b: 5 seen, arm: 10 arm, alone: 5 alone
a: 6 seen, b: 6 seen, arm: 12 arm, alone: 6 alone
```

### A lazy attached to the value of an attribute in a row of a class's markup loop is part of the value beside a StyleSheet slot the value reads as "$name": the attribute is given again as the value or the lazy reads something new.

```brix
(import /core/style)

(main
  (<> Page (id: page)
    (<> Gutter (id: gutter))
  )
  (call (. gutter show))
  (= (. gutter is-marked) true)
  (call (. page settle))
  (call (. gutter show))
  (= (. gutter is-marked) false)
  (= ([] (. gutter rows) 0) (call Row "del"))
  (call (. page settle))
  (call (. gutter show))
)

(type + Look (is Style)
  (prop + ink String (= "none"))
  (prop + size Int (= 1))
)

(style Palette
  (using Look)
  (prop + accent String (= "blue"))
  (rule gutter-cell
    (= size 2)
  )
)

(type + Row
  (prop + kind String)
  (fn-do + This _
    (param kind _)
  )
)

(fn-expr + kind-ink String
  (param kind String)
  (param owner Gutter)
  (str kind "-ink")
)

(type + Cell (is MarkupNode)
  (using Look)
)

(type + Gutter (is MarkupNode)
  (using Palette)
  (prop + rows _ (= (list (call Row "add") (call Row "keep"))))
  (prop + is-marked _ (= false))
  (fn-do + show _
    (let parts (list String))
    (for (in cell (call (of children-of Cell)))
      (+= parts (str (. cell $ink) " " (. cell $size)))
    )
    (print (call (. ", " join) parts))
  )
  (content
    (for (in row #rows)
      (<> Cell (• gutter-cell)
        (= ink
          (scope (if-expr #is-marked $accent row-ink)
            (lazy row-ink (call kind-ink (. #row kind) this))
          )
        )
      )
    )
  )
)

(type + Page (is MarkupRoot)
  (using Palette)
)
```

```output
add-ink 2, keep-ink 2
blue 2, blue 2
del-ink 2, keep-ink 2
```
