# Markup

Markup builds a tree of objects. An element names a class, makes an object of it, sets its
properties, and hands it the elements written inside it as children. Nothing about it is text: the
result is the objects. Any class can be an element, so markup is as much for data as for interfaces,
and the GUI and terminal libraries build their views from it. Markup can also follow state: an
element whose properties or children read tracked values is updated when they change.

This chapter covers elements, attributes, children, control flow inside markup, markup that follows
state, components and their slots, and the links between elements. Style sheets, which markup
elements use, are in Styles.

## Elements

```brix
(main
  (<> Menu (id: lunch) (text: "Lunch")
    (<> Dish (text: "Soup")
      (= price 4)
    )
    (<> Dish (text: "Salad")
      (= price 6)
      (= vegetarian true)
    )
    (<> Menu (text: "Drinks")
      (<> Dish (text: "Tea")
        (= price 2)
        (= vegetarian true)
      )
    )
  )
  (call (. lunch show) "")
)

(type + Item
  (prop + name String)
  (fn-do + This _
    (param name _)
  )
)

(type + Dish (is Item)
  (prop + price _ (= 0))
  (prop + vegetarian _ (= false))
)

(type + Menu (is Item)
  (prop + items (list Item))
  (fn-do + (infix this += (param item Item)) _
    (+= items item)
  )
  (fn-do + show _
    (param indent String)
    (print (str indent name))
    (for (in item items)
      (if (= (| (: dish Dish)) item)
        (print (str indent "  " (. dish name) " " (. dish price) (if-expr (. dish vegetarian) " (v)" "")))
        (else-if (= (| (: menu Menu)) item)
          (call (. menu show) (str indent "  "))
        )
      )
    )
  )
)
```

```output
Lunch
  Soup 4
  Salad 6 (v)
  Drinks
    Tea 2 (v)
```

An element is `(<> Type …)`, in rows `<Type`{.rows}. After the type come, in this order, the
element's name, the slot it fills, a parameter, its value, style rules, theme tags, and then its
rows. Rows write them in the same order.

| Form | Rows | Gives the element |
| --- | --- | --- |
| `(id: lunch)` | `<Menu lunch`{.rows} | A name; see [Names](#names). |
| `(slot: title)` | `<Text (title)`{.rows} | The slot of a component it fills; see [Slots](#slots). |
| `(param: who)` | `<Text (row) - who`{.rows} | The name of a value a repeated slot passes it. |
| `(value: 3)` | `<Count = 3`{.rows} | Its constructor's first argument. |
| `(text: "Lunch")` | `<Menu "Lunch"`{.rows} | The same, for a text. |
| `(• loud)` | `<Card • loud`{.rows} | A style rule it uses; see Styles. |
| `(for: dark)` | `<Card • loud for dark`{.rows} | A theme tag; see Styles. |

- **Making.** An element calls a constructor of its class. Its value and its attributes fill the
  constructor's parameters by position and by name, and parameters with defaults may be left out.
- **Order.** The constructor runs first. Then every attribute is set, wherever it is written among
  the rows. Then the rows run in order. A child is made and its own rows run before it is handed to
  its parent, so a parent receives each child complete.
- **What can be an element.** A class, including a generic class with its type arguments written,
  as in `(<> (of Box Int) (value: 3))`. Core collections are classes too, so
  `(<> (of List Int) 1 2 3)` is a list. A struct, a tuple, a number or a `String` cannot be an
  element, and neither can an interface: `Constructor not found`.
- **No imports.** Markup works in any program. A module that writes markup imports `/core/markup`
  without writing it.
- **Messages.** The messages write an element as rows do, as in `Markup element <Book gives no
  value for "title", which the constructor of Book requires`.

### Where elements go

An element is a value. It can be written wherever a row can:

- a row of a body, or the body of a `fn-expr`;
- the value of a `let`, `lazy`, `=`, `return` or attribute;
- a positional argument written as a row, `(call show (block (arg (<> Text))))`{.brix}, in rows
  `- <Text`{.rows} below the call;
- a named argument, `(call show (:t (<> Text)))`{.brix}, in rows `show • t <Text`{.rows};
- an item of a list built from bullets, `(bullet (<> Text))`{.brix}, in rows `* <Text`{.rows};
- the value of a `match` arm.

It cannot be written inside an expression: among the arguments in `(call …)`, the items of
`(list …)`, or the operands of `if-expr`. There it is `(<> …) cannot be written inside an
expression`, with the rows above as the way to write it. To choose between two elements, assign
one in each arm of an `if`.

### Names

`(id: lunch)` names the element. The name is a local of the block the element is written in, and
can be read after the element and by the element's own rows. A statement above the element that
reads the name makes the element there, once. A function written above the element, or as one of
its own attributes, reads the name when it runs, and panics if it runs before the element is made;
the element's type arguments must then be written. A task started above the element cannot read
it, as a task keeps what it reads when it starts. In a component's content, the name is a private
property of the class; see [Components](#components).

## Attributes

A row `(= price 4)`{.brix} in an element's body is an attribute: it sets the element's property
`price`, or fills a constructor parameter of that name.

- **Values** may be any expression: a block of bullets, a tuple, a function value, or another
  element.
- **What the value can read.** The value is worked out outside the element, so it reads the names
  around the element, not the element's own members. It cannot read the element by its name:
  `Named markup element "t" is read before its markup constructs it`.
- **Setters** run as for any assignment.
- **Defaults.** A property no attribute sets keeps its declared value.
- **Always an attribute.** `(= name value)` in an element's body sets the element's property, even
  where a local of that name exists. To assign a local, write the assignment in a `(do …)` row,
  as the error says: `Property "count" not found; an assignment in an element's body sets the
  element's property. To assign a local, put it in a do row: "do count = …"`.
- **Private properties** can be set from markup in their own module only.
- **Messages.** An unknown property gives `Property "pages" not found; …`, as above, a value of the
  wrong type `Expected I64 here, but this is String, and there is no conversion between them`, a
  required constructor parameter left out `Markup element <Book gives no value for "title", which
  the constructor of Book requires`, and an attribute for a parameter the element's header already
  gives `Parameter "title" is given twice, by the element's header and by this attribute; give it
  once`.

## Children

The rows of an element that give values are its content. Each is handed to the element's `+=`, as
`(+= menu dish)` would, and a collection to its `++=` when `+=` does not take it. `+=` may be
overloaded, so an element can take texts, names and elements of different types in one body:
`"Hello, "`, `name` and `(<> Bold (value: "friend"))` are all content.

- **Steps.** Rows that give no value run as steps: `print`, `let`, `lazy`, `do`, and calls that
  return nothing. An element written inside a `(do …)` row is made but not added.
- **What a class needs.** A class takes children when it declares `+=` for their type. A line that
  no `+=` takes gives `Content not accepted`.
- **More than children.** Three classes in core give an element more:

| Class | Adds |
| --- | --- |
| Any class with `+=` | Children, attributes that follow state, and [slots](#slots). |
| A class that implements `(of MarkupParent Child)` | Children of class `Child` with no `+=` written, kept in `children`, and the [links](#links-between-elements) between them. |
| A class that extends `MarkupNode` | Children of any `MarkupNode` class, links, and loops and branches that follow state. |
| A class that extends `MarkupRoot` | The same, with updates gathered into batches; see [Batches](#batches). |

- **`MarkupNode`** gives `children`, and `(call (of children-of Text))` gives the children of one
  class, looking through the rows of loops and branches.

## Control flow in markup

```brix
(main
  (let scores (list 7 2 9))
  (let winner Player (call Human "Ann"))
  (<> Report (id: report)
    (for (in s scores (when (> s 5)))
      (<> Line (text: (str "score " s)))
    )
    (if (> (. scores length) 2)
      (<> Line (text: "a full game"))
      (else
        (<> Line (text: "a short game"))
      )
    )
    (match winner
      (case (: h Human)
        (<> Line (text: (str "won by " (. h name))))
      )
      (case (: b Bot)
        (<> Line (text: "won by a bot"))
      )
    )
    (lazy rounds (. scores length))
    (<> Line (text: (str rounds " rounds")))
    (do
      (<> Line (text: "made, not added"))
    )
  )
  (for (in line (. report lines))
    (print line)
  )
)

(type + Player (is Union)
  (variant Human)
  (variant Bot)
)

(type + Human
  (prop + name String)
  (fn-do + This _
    (param name _)
  )
)

(type-line + Bot)

(type + Line
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(type + Report
  (prop + lines (list String))
  (fn-do + (infix this += (param line Line)) _
    (+= lines (. line text))
  )
)
```

```output
score 7
score 9
a full game
won by Ann
3 rounds
```

`for`, `if`, `match`, `let`, `lazy` and `print` work in an element's body as they do anywhere, and
the elements their rows give are children of the element. A loop or an arm may give several
elements, and an `if` may test a pattern. There is no fragment element: control flow is how one row
gives several children.

## Markup that follows state

```brix
(main
  (let players (list "ann" "bo"))
  (<> Board (id: board)
    (<> Text (text: "players")
      (= note (str (. #players length) " playing"))
    )
    (for (in name #players (keyed-by name))
      (<> Text (text: name))
    )
    (if (> (. #players length) 2)
      (<> Text (text: "table full"))
      (else
        (<> Text (text: "seats free"))
      )
    )
  )
  (call (. board show))
  (+= players "cy")
  (call (. board show))
  (-= players "ann")
  (call (. board show))
  (= players (list "bo" "cy"))
  (call (. board show))
)

(type + Text (is MarkupNode)
  (prop + text String)
  (prop + note _ (= ""))
  (fn-do + This _
    (param text _)
    (print (str "  make " text))
  )
)

(type + Board (is MarkupNode)
  (fn-do + show _
    (let parts (list String))
    (for (in child (call (of children-of Text)))
      (+= parts (if-expr (== (. child note) "") (. child text) (str (. child text) " (" (. child note) ")")))
    )
    (print (call (. ", " join) parts))
  )
)
```

```output
  make players
  make ann
  make bo
  make seats free
players (2 playing), ann, bo, seats free
  make cy
  make table full
players (3 playing), ann, bo, cy, table full
  make seats free
players (2 playing), bo, cy, seats free
players (2 playing), bo, cy, seats free
```

Markup that makes tracked reads, `#players` here, follows the values it reads; see Reactivity.

| Where the read is | When the value changes |
| --- | --- |
| An attribute | The attribute is set again. Nothing is made again. |
| `(value: …)` or `(text: …)` | The property of the constructor parameter's name is set again. The parameter must name a property: `Tracked markup constructor argument must match a property`. |
| A content row | The row is made again, and replaces the old child. A class that is not a `MarkupNode` must declare `replace` to take it. |
| The condition of an `if` or the subject of a `match` | The branch is made again, only when the arm it takes changes. |
| The source of a `for` | The rows follow the collection. |

- **Keys.** `(keyed-by name)` keeps each row with its key. Moving items, or assigning a new list
  with the same keys, makes nothing, as the last assignment above shows. Only new keys make rows,
  and the rows of removed keys go. Without keys, rows follow positions, so removing the first item
  makes again every row after it.
- **Loops and branches that follow state** need a `MarkupNode` parent: `Dynamic markup loop parent
  must implement MarkupElement`. A row may give several elements, and hold an `if`, a `match` or a
  `for`, as any body does; such a row is a region of its own, and an `if` or `match` in it that
  makes tracked reads follows them in each row. A loop of this kind takes no `when`, `while` or
  `until` clauses.
- **When.** Without a batch root, each change updates the markup before the statement that made it
  goes on, as above.
- **Lifetime.** An element follows its reads until it is closed. A `MarkupNode` closes its children
  when it closes.
- **Errors** raised while an attribute is set again go to `unhandled-error`, and the element keeps
  its old value.

### Bindings

```brix
(main
  (let mixer (call Mixer))
  (<> Dial (id: dial)
    (<-> level (. mixer volume))
  )
  (print (str (. mixer volume) " " (. dial level)))
  (call (. dial turn) 7)
  (print (str (. mixer volume) " " (. dial level)))
  (= (. mixer volume) 3)
  (print (str (. mixer volume) " " (. dial level)))
)

(type + Mixer
  (prop + volume _ (= 5))
)

(type + Dial
  (prop + level _ (= 0))
  (fn-do + turn _
    (param next Int)
    (= level next)
  )
)
```

```output
5 5
7 7
3 3
```

`(<-> level (. mixer volume))`{.brix} binds the element's `level` to the place `(. mixer volume)`
both ways. The element takes the place's value as it is made and follows it, as
`(= level (. mixer #volume))`{.brix} would. Each time the element's `level` changes, it is stored
back in the place.

- **The place keeps its value** as the element is made: only a later change of the element is
  stored back.
- **The place** is a name or a chain of members, written without `#`; the binding follows its last
  member. Anything else is an error: `The place of “<->” is a name or a chain of members, written
  without “#”, as in “(<-> value (. settings volume))”`.
- **The property** is one the element stores. One a getter or a setter answers is an error, and so
  is a name of no property, as a parameter of the constructor alone is.
- **Conversions.** Each side becomes the other on its own, as an assignment converts it, or the
  binding is an error: `"<->" needs each side to become the other on its own, and I64 becomes F64
  only when asked; bind a place of the property's type`. Where the property holds a number or a
  `Bool` and the place another type, the element is given back what the place holds after each
  store, so a conversion that loses something shows what was kept.
- **When.** Without a batch root, a change of the element is stored back before the statement that
  made it goes on. Under a batch root it is stored back when the batch runs, as other actions are,
  so a handler of an event the element publishes as it changes still sees the place's old value.
- **Where.** A binding is written in an element's body. Anywhere else, in a root `<This`, or in the
  body of a slot outlet it is an error, as in `"<->" binds a property of a markup element to a
  place, so it is written in the element's body; to store a value here, write "="`.
- **Events** remain for what a binding does not do: cancelling a change, and telling a change in
  progress from a finished one.

### Batches

```brix
(main
  (<> Outline (id: doc)
    (<> Heading (text: "Intro"))
    (for (in title #chapters)
      (<> Heading (text: title))
    )
  )
  (print (. doc summary))
  (+= (. doc chapters) "Ending")
  (print (. doc summary))
  (call (. doc settle))
  (print (. doc summary))
  (for (in h (. doc headings))
    (print (str (. h text) " in " (. (!! (. h outline)) name)))
  )
)

(type + Heading (is MarkupNode)
  (prop + text String)
  (prop + outline (of Ancestor Outline))
  (fn-do + This _
    (param text _)
  )
)

(type + Outline (is MarkupRoot)
  (prop + name _ (= "doc"))
  (prop + chapters (list String) (= (list "Middle")))
  (prop + headings (of Descendants Heading))
  (prop + summary _ (= (str (. #headings length) " headings")))
)
```

```output
2 headings
2 headings
3 headings
Intro in doc
Middle in doc
Ending in doc
```

Under a `MarkupRoot`, changes to the tree wait and are made together: when the program next waits,
or at `(call (. doc settle))`. So `summary` still counts 2 headings after the third chapter is added,
until the root settles. What a [binding](#bindings) stores back
waits for the root too.

- **Queries.** A property of type `(of Ancestor Outline)` holds the nearest `Outline` above the
  element, and one of type `(of Descendants Heading)` the `Heading`s below it. Both follow the tree
  as it changes, and both need a `MarkupRoot` above them; under a plain `MarkupNode` they hold
  nothing.

## Components

A component is a class with a `content` section: rows that run each time an object of it is made,
whether by markup or by a call. In rows, `content` is the section after a thin divider with no
header. The elements it gives are the object's own children, and its rows read the class's members
by name, tracked or not.

- **Named elements** in `content` are private properties of the class, which its methods can read.
  A name the class already declares as a property receives the element.
- **`(<> This …)`** as the only row of `content` sets the object's own properties, as defaults that
  a caller's attributes override.
- **Order.** `content` runs as the object is made, after attributes for stored properties are set
  and before the caller's rows.
- **Messages.** An empty `content` gives `Empty content section not permitted`, and two give `Only
  one content section permitted`.

### Slots

```brix
(main
  (<> Card (id: empty))
  (print (call outline empty))
  (<> Card (id: note)
    (<> Text (slot: title) (text: "Note"))
    (<> Text (text: "Buy milk."))
  )
  (print (call outline note))
  (<> Card (id: letter)
    (<> Text (text: "Dear Bo,"))
    (<> Text (text: "See you soon."))
  )
  (print (call outline letter))
)

(fn-do + outline String
  (param node MarkupNode)
  (let parts (list String))
  (for (in child (. node children))
    (if (is child Text)
      (+= parts (. (as child Text) text))
      (else
        (+= parts (str "[" (call outline child) "]"))
      )
    )
  )
  (return (call (. " " join) parts))
)

(type + Text (is MarkupNode)
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(type + Card (is MarkupNode)
  (content
    (+<> Text (id: title)
      (<> Text (text: "Untitled:"))
    )
    (+<> MarkupNode
      (<> Text (text: "(empty)"))
    )
    (<> Text (text: "--"))
  )
)
```

```output
Untitled: (empty) --
Note Buy milk. --
Untitled: [Dear Bo, See you soon.] --
```

A slot is a place in a component's content that a caller fills. `(+<> Text (id: title) …)`, in
rows `+ <Text title`{.rows}, declares the slot `title`, and a caller's element with
`(slot: title)` fills it. `(+<> MarkupNode …)`, with no name, is the default slot, which the
caller's other rows fill.

- **Placement.** What a caller gives goes where the slot stands, in the component's order, whatever
  order the caller writes it in.
- **Fallback.** The elements in a slot's body are used when the caller gives none, and must be of
  the slot's type. Attributes in its body are set on what the caller gives instead.
- **One or several.** One element given to the default slot fills it. Several are built into one
  element of the slot's type, as the letter's two texts are.
- **Subclasses.** A subclass's `content` fills its base's default slot when the base has one, and
  follows the base's content otherwise. A caller's rows given to a component with no default slot
  follow its content.
- **Messages.** Filling a slot twice gives `Duplicate slot content`, a slot the class lacks
  `Unknown slot`, and a slot filled from inside an `if` or a `for` `Slot binding is only valid
  directly inside markup body`.

### Repeated slots and passing slots on

```brix
(main
  (<> Roster (id: plain))
  (print (call outline plain))
  (<> Roster (id: team)
    (<> Text (slot: row) (param: who) (text: (str "* " who)))
  )
  (print (call outline team))
  (+= (. team names) "cy")
  (print (call outline team))
  (<> Page (id: page)
    (<> Text (slot: row) (param: who) (text: (str who "!")))
  )
  (print (call outline page))
)

(fn-do + outline String
  (param node MarkupNode)
  (let parts (list String))
  (for (in child (. node children))
    (if (is child Text)
      (+= parts (. (as child Text) text))
      (else
        (+= parts (str "[" (call outline child) "]"))
      )
    )
  )
  (return (call (. " " join) parts))
)

(type + Text (is MarkupNode)
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(type + Roster (is MarkupNode)
  (prop + names (list String) (= (list "ann" "bo")))
  (content
    (for (in name #names)
      (+<> Text (id: row) (param: name String)
        (<> Text (text: name))
      )
    )
  )
)

(type + Page (is MarkupNode)
  (content
    (<> Text (text: "Team:"))
    (<> Roster
      (<+ row)
    )
  )
)
```

```output
ann bo
* ann * bo
* ann * bo * cy
Team: [ann! bo!]
```

- **Repeated slots.** A slot inside a loop of the content is filled once for each item.
  `(param: name String)` passes the item to the caller's element, which names it with its own
  `(param: who)`.
- **Passing a slot on.** `(<+ row)`, in rows `<+ (row)`{.rows}, gives the content a component's
  caller gave for `row` to the element it is written in. It declares the slot `row` for the
  component, as `Page` does, and an unfilled slot leaves the inner element its own fallback.

## Links between elements

```brix
(main
  (<> Toolbar (id: bar)
    (<> Button (id: save) (text: "save"))
    (<> Group
      (<> Button (id: undo) (text: "undo")
        (for (on name pressed)
          (print (str "undo heard its own press, label " label))
        )
      )
    )
  )
  (call (. save press))
  (call (. undo press))
  (print (. bar log))
  (print (is (!! (. undo %parent)) Group))
  (print (. (as (!! (. (!! (. save %next-sibling)) %first-child)) Button) label))
)

(type + Node (is MarkupNode)
  (implements EventNode)
  (fn-expr + event-parent (? EventNode) :get
    (as %parent (? Node))
  )
)

(type-line + Group (is Node))

(type + Button (is Node)
  (event + pressed String)
  (prop + label String)
  (fn-do + This _
    (param label _)
  )
  (fn-do + press _
    (<- pressed label)
  )
)

(type + Toolbar (is Node)
  (prop + log _ (= "pressed:"))
  (subscribe-for (on name (. Button pressed))
    (+= log (str " " name))
  )
)
```

```output
undo heard its own press, label undo
pressed: save undo
true
undo
```

- **Links.** `%parent`, `%first-child`, `%last-child`, `%previous-sibling` and `%next-sibling` read
  an element's neighbors, as `(. undo %parent)`{.brix}, or `%parent` alone in a method. Each gives
  an optional. They work on classes that implement `MarkupParent` or extend `MarkupNode`: `Markup
  link "%first-child" not found`.
- **Types.** On a `MarkupNode`, a link gives a `MarkupNode?`, so read a member of another class
  after `as`.
- **Handlers.** A `(for (on name pressed) …)` row in an element's body subscribes to the element's
  own event, and reads the element's members by name. It ends when the element closes.
- **Events that rise.** A class that implements `EventNode` and gives its parent as `event-parent`
  sends its events up the tree, where an ancestor subscribes to them by class, as `Toolbar` does to
  `(. Button pressed)`. See Reactivity.
- **Function values** given as attributes, such as `(= on-hover (=> () :do …))`, are ordinary
  attributes.
