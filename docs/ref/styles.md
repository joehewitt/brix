# Styles

A style is a set of properties that describe how something looks or behaves, such as a font, a
width or a color, kept apart from what the thing is. Elements of markup take style properties from
the elements around them, from style sheets, and from their own markup. A style sheet holds style
rules, which set style properties on the elements they select, and slots, values a whole tree of
elements can share and a theme can change. The values an element ends up with are worked out
together, by settling, and read with `$`.

Nothing here is tied to a screen. The GUI and terminal libraries build their looks from styles,
and so can any program: every example in this chapter prints its results.

A style rule is not the rule of Reactivity, which runs code when tracked values change. This
chapter says "style rule" where the two could be confused, and "rule" otherwise.

## Styles and style properties

```brix
(import /core/style)

(main
  (<> Page (id: page)
    (<> Para (id: intro) (text: "Intro")
      (= font "serif")
      (= size 14)
      (<> Para (id: note) (text: "Note"))
    )
    (<> Para (id: footer) (text: "Footer"))
  )
  (for (in para (list intro note footer))
    (call (. para describe))
  )
  (print (. note $font))
  (let loose (call Para "Loose"))
  (call (. loose describe))
)

(type + Lettering (is Style)
  (prop + font String @inherited (= "sans"))
  (prop + size Int (= 12))
)

(type + Para (is MarkupNode)
  (using Lettering)
  (prop + title String)
  (fn-do + This _
    (param title _)
  )
  (fn-do + describe _
    (print (str title ": " $font " " $size))
  )
)

(type + Page (is MarkupRoot))
```

```output
Intro: serif 14
Note: serif 12
Footer: sans 12
serif
Loose: sans 12
```

- **A style** is a class that extends `Style`. Its properties are **style properties**. A property's
  initial value is what an element reads when nothing sets it.
- **`(using Lettering)`** gives a class the style's properties. In rows it is `using Lettering` in
  the class's header.
- **Setting a style property** in markup is written as setting any property, `(= font "serif")`.
- **Reading it** is `$font` on `this`, or `(. note $font)` on another object. In rows it is `$font`
  and `note.$font`.
- **`@inherited`**: an element that sets nothing takes its parent's value, so `note` is in serif.
  `size` is not inherited, so `note` has the initial `12`. `@inherited` is the only annotation a
  style property can have.
- **The tree.** Style properties set in markup take effect on elements under a root: a class that
  extends `MarkupRoot`. The elements in it extend `MarkupNode`. A root works out its elements'
  values when its markup is built, and again when the program next waits or the root is told to
  `settle`; see [When values change](#when-values-change).
- **Outside a tree**, an element reads initial values, as `loose` does.
- **The import.** A module that declares or extends a style sheet imports `/core/style` on its own.
  One that only declares a style or reads `$` values imports it itself, as this one does.

The UI libraries' `Box` types are a `MarkupNode` and a `MarkupRoot` of this kind, with styles of
their own.

## Style sheets

```brix
(main
  (<> Form (id: form)
    (<> Field (id: user) (text: "User"))
    (<> Field (id: email) (text: "Email") (• wide))
    (<> Field (id: notes) (text: "Notes") (• wide) (• rows 4))
    (<> Field (id: code) (text: "Code") (• boxed)
      (= width 8)
    )
  )
  (for (in field (of (. form children-of) Field))
    (call (. field describe))
  )
)

(type + Sizing (is Style)
  (prop + width Int (= 10))
  (prop + height Int (= 1))
  (prop + border String (= "none"))
)

(style FormSheet
  (using Sizing)
  (rule Field
    (= width 20)
    (= border "thin")
  )
  (rule user
    (= width 30)
  )
  (rule wide
    (= width 40)
  )
  (rule rows
    (param count Int)
    (= height count)
  )
  (rule boxed (• wide)
    (= border "double")
  )
)

(type + Field (is MarkupNode)
  (using Sizing)
  (prop + label String)
  (fn-do + This _
    (param label _)
  )
  (fn-do + describe _
    (print (str label " " $width "x" $height " " $border))
  )
)

(type + Form (is MarkupRoot)
  (using FormSheet)
)
```

```output
User 30x1 thin
Email 40x1 thin
Notes 40x4 thin
Code 8x1 double
```

`(style FormSheet …)` declares a style sheet, a class that extends `StyleSheet`. In rows it is
`• FormSheet`{.rows}. It holds:

- **`using`** rows, naming the styles whose properties its rules set.
- **Style rules**, `(rule name …)`, which set style properties.
- **Slots**, `(prop + name Type (= value))`, values its rules and markup can read; see
  [Slots and themes](#slots-and-themes).
- **Keyframes**, slots that describe motion; see [Motion](#motion).
- **Domains**, `(style-domains …)`; see [Domains](#domains).

A class that uses the sheet, as `Form` does, makes its rules reach the elements beneath it.

### Which rules apply

A rule is applied to an element of markup. These apply to an element, in this order, and the
order they are written in the sheet does not matter:

1. **The rule named for the element's type**, or for a type it extends: `(rule Field …)` applies
   to every `Field` and every subclass of `Field`. A generic type is matched by its name alone.
2. **The rule named for the element's id**: `(rule user …)` applies to `(<> Field (id: user))`.
3. **The rules the element names** with `(• name)`, from left to right. In rows they follow the
   element's name: `<Field email • wide`{.rows}.
4. **The element's own assignments**, which come last, so `code` is 8 wide.

A later rule's value replaces an earlier one's. A name that no module of the program declares a
rule for is an error, so a misspelled `(• wied)` is reported. A library that applies a name only
for a theme to style declares it with an empty rule, `(rule wide)`, in the class whose markup
applies it. A name some rule declares but none the element sees is ignored, and so is a rule named
for a type that does not exist.

### Parameters

A rule takes at most one parameter: `(rule rows (param count Int) (= height count))`, applied as
`(• rows 4)`. In rows the rule is `• rows:count Int`{.rows} and the element applies it as
`• rows:4` or `• rows(4)`.

- **Several values** go in one tuple parameter: `(param size (tuple Int Int))`, applied as
  `(• sized (tuple 3 4))`.
- **Overloads.** Rules of one name can take parameters of different types; the argument picks one,
  an exact type before one it converts to.
- **The argument** must fit: a missing, extra or wrongly typed argument is `Type mismatch`, and a
  parameter with no type is `Argument type not found`. A parameter's default value is not used.

### Rules that apply rules

Rules named in a rule's header run before its body: `(rule boxed (• wide) (= border "double"))`
sets the width first, then the border. A rule cannot reach itself through others:
`Recursive style rule application: loud -> quiet -> loud`. A rule in a sheet names rules only in
its header; its body holds declarations and generated content.

### Conditions

`(• name (if: #flag))` applies a rule while a tracked `Bool` property of the element is true, and
follows it as it changes. In rows it is `• name = #flag`. The condition must be a tracked property
of the element, written with `#`; `(if: is-on)` and `(if: (not #is-on))` are refused:
`Style rule conditions must be a tracked boolean property reference (like #is-disabled)`. For a
condition that is false, use a state; see [States](#states).

## What a rule sets

A rule's rows are assignments, `(= property value)`, nested rules, states and generated content.
`+=` and other steps are refused: ``(+= …) cannot be a `row` of (rule …), which takes Style.``

- **Values** are ordinary values of the property's type: text, numbers, `Bool`, characters, tuples,
  union cases such as `(. Align right)`, `null`, units such as `50%` or `250ms`, colors and objects.
  A value that does not convert to the property's type is an error, as anywhere.
- **What a value can read.** A sheet rule's value can read literals, names of the module, the
  rule's parameter, and the sheet's slots. It cannot read the element's properties or style values.
- **`inherit`**: `(= font inherit)` makes an `@inherited` property take its parent's value, over a
  lower layer's. On a property that is not `@inherited` it is an error:
  `Style property "pad" is not @inherited, so it cannot be assigned "inherit"`.
- **Properties a rule may set** are those of the styles the element's class uses and those the
  visible sheets use. Setting any other name in a sheet rule is `Not supported`; in markup it is
  `Property "colour" not found; …`, which says how to assign a local instead.

## Which value wins

Every assignment to a style property is on a layer. A later layer wins over an earlier one, and in
one layer the assignment made later wins.

| Layer | Written as |
| --- | --- |
| Defaults | `(rule This …)` in the element's class, and the root of its `content` |
| Content | Attributes of the elements a class's `content` declares |
| Rules | Rules by type, by id and by `•`, from the class or a sheet |
| Default states | `when-style` in `(rule This …)` |
| States | `when-style` and `style-set` in other rules, and rules applied with `(if: …)` |
| Inline | Assignments in the element's own markup |
| Code | An assignment from code, as `(= (. lamp tone) "code")` |

```brix
(main
  (<> Room (id: room)
    (<> Lamp (text: "a"))
    (<> Lamp (text: "b") (• warm))
    (<> Lamp (text: "c") (• lit)
      (= tone "inline")
    )
    (<> Lamp (id: d) (text: "d") (• warm))
  )
  (call (. room show) "off")
  (for (in lamp (of (. room children-of) Lamp))
    (= (. lamp is-on) true)
  )
  (call (. room settle))
  (call (. room show) "on")
  (= (. d tone) "code")
  (call (. room settle))
  (call (. room show) "code")
)

(type + Shade (is Style)
  (prop + tone String (= "none"))
)

(style RoomSheet
  (using Shade)
  (rule warm
    (= tone "warm")
  )
  (rule lit
    (when-style #is-on
      (= tone "lit")
    )
  )
)

(type + Lamp (is MarkupNode)
  (using Shade)
  (prop + label String)
  (prop + is-on _ (= false))
  (rule This
    (= tone "default")
    (when-style #is-on
      (= tone "default-on")
    )
  )
  (fn-do + This _
    (param label _)
  )
)

(type + Room (is MarkupRoot)
  (using RoomSheet)
  (fn-do + show _
    (param heading String)
    (print heading)
    (for (in lamp (of children-of Lamp))
      (print (str "  " (. lamp label) " " (. lamp $tone)))
    )
  )
)
```

```output
off
  a default
  b warm
  c inline
  d warm
on
  a default-on
  b default-on
  c inline
  d default-on
code
  a default-on
  b default-on
  c inline
  d code
```

- **A rule beats the defaults**, so `b` is `warm`.
- **A state beats a rule**, even the class's default state: once `b` is on, its `default-on` beats
  `warm`.
- **Inline beats every rule**, so `c` stays `inline`.
- **Code beats inline.** An assignment from code lasts until code assigns again.

## States

```brix
(main
  (<> Board (id: board)
    (<> Task (id: wash) (text: "wash up"))
    (<> Task (id: pay) (text: "pay rent") (• flagged (if: #is-late)))
  )
  (call (. board show))
  (= (. wash is-done) true)
  (= (. pay is-late) true)
  (call (. board show))
  (call (. board settle))
  (call (. board show))
)

(type + Marks (is Style)
  (prop + box String (= "[ ]"))
  (prop + note String (= ""))
)

(style BoardSheet
  (using Marks)
  (rule Task
    (when-style #is-done
      (= box "[x]")
    )
    (when-style (not #is-done)
      (= note " (open)")
    )
    (style-set is-late note " (late)")
  )
  (rule flagged
    (= box "[!]")
  )
)

(type + Task (is MarkupNode)
  (using Marks)
  (prop + label String)
  (prop + is-done _ (= false))
  (prop + is-late _ (= false))
  (fn-do + This _
    (param label _)
  )
)

(type + Board (is MarkupRoot)
  (using BoardSheet)
  (fn-do + show _
    (for (in task (of children-of Task))
      (print (str (. task $box) " " (. task label) (. task $note)))
    )
    (print "--")
  )
)
```

```output
[ ] wash up (open)
[ ] pay rent (open)
--
[ ] wash up (open)
[ ] pay rent (open)
--
[x] wash up
[!] pay rent (late)
--
```

A state is a tracked `Bool` property of the element. `(when-style #is-done …)` holds what applies
while it is true, and `(when-style (not #is-done) …)` while it is false. In rows they are
`when #is-done`{.rows} and `when !#is-done`{.rows}. `(style-set is-late note " (late)")`, in rows
`is-late:note = " (late)"`{.rows}, is a state with one assignment.

- **What a state holds**: assignments of style properties, `(• name)` rows, nested rules, which
  then apply only while the state holds, and generated content. An assignment of a property that is
  not a style property is refused:
  `Conditional style declarations may only set style properties, and "glow" is an object property`.
- **The property** is looked up on each class the rule is applied to. A class without it is an error
  at the rule: `Style state "is-on" is not a property of Line`. One that is not a `Bool` is too:
  `Style state "count" must be a boolean property, but Lamp.count is I64`.
- **Only in rules.** A state cannot be written in markup:
  ``(when-style …) cannot be a `row` of (<> …), which takes Child.``
- **Where states come from**: any tracked `Bool`. The GUI library's `Box` has `is-hovered`,
  `is-pressed`, `is-focused`, `is-entering` and `is-leaving`, among others. The case of a state
  machine in a class's `content` is one too:

```brix
(main
  (<> House (id: house)
    (<> Door (id: front))
  )
  (print (str (. front is-open) " " (. front $sign)))
  (<- (. front knocks) true)
  (call (. house settle))
  (print (str (. front is-open) " " (. front $sign)))
)

(type + Signs (is Style)
  (prop + sign String (= "closed"))
)

(style HouseSheet
  (using Signs)
  (rule Door
    (when-style #is-open
      (= sign "come in")
    )
  )
)

(type + Door (is MarkupNode)
  (using Signs)
  (event + knocks Bool)
  (content
    (state is-shut
      (then is-open (trigger on knocks))
    )
  )
)

(type + House (is MarkupRoot)
  (using HouseSheet)
)
```

```output
false closed
true come in
```

### When values change

The values a state selects, like every style value, are worked out by **settling**. A root settles
when its markup is built, so values are right as soon as the markup statement ends. After that, a
change to anything style values follow, such as a state, an `(if: …)` condition, a theme tag or a
tracked value in an assignment, shows at the next settle: when the program next waits, or when code
calls the root's `settle`. Until then, reads give the old values, as the second block of the
board's output shows. The same markup statement records its assignments once, however many
elements it makes.

## Slots and themes

```brix
(main
  (let late false)
  (<> Page (id: page)
    (<> Line (text: "day"))
    (<> Line (text: "night") (• loud) (for: dark))
    (<> Line (text: "dusk") (for: dark #late))
  )
  (call (. page show))
  (= late true)
  (call (. page settle))
  (call (. page show))
)

(type + Look (is Style)
  (prop + ink String (= "none"))
)

(style Palette
  (using Look)
  (prop + text-color String (= "black"))
  (prop + accent String (= "blue"))
  (rule Line
    (= ink text-color)
  )
)

(style DarkPalette (is Palette) (for dark)
  (prop + text-color _ (= "white"))
  (prop + accent _ (= "orange"))
)

(style-extend Palette
  (rule loud
    (= ink (++ accent "!"))
  )
)

(type + Line (is MarkupNode)
  (using Look)
  (prop + label String)
  (fn-do + This _
    (param label _)
  )
)

(type + Page (is MarkupRoot)
  (using Palette)
  (fn-do + show _
    (for (in line (of children-of Line))
      (print (str (. line label) " " (. line $ink)))
    )
  )
)
```

```output
day black
night orange!
dusk black
day black
night orange!
dusk white
```

### Slots

A **slot** is a property of a sheet: `(prop + accent String (= "blue"))`. Rules read it by its bare
name, as `text-color` above.

- **Where a slot is read**: in a sheet rule's values, in an element's own assignments, alone as a
  rule's argument, and in other slots' values. In ordinary code a slot's name is not found.
- **Late.** A value that reads a slot reads it from the sheet in force at the element, so a theme
  changes it without the markup running again: `dusk` turns white when `late` does.
- **One sheet's slots.** A value reads slots of one sheet only. A slot whose name is also a local or
  a module's name is an error, not a choice:
  `"bright" is ambiguous: it names both a StyleSheet slot of AppTheme and a lexical value`.
- **No other values.** A value that reads a slot can read no local, member, style value or tracked
  value: `A style value that reads StyleSheet slots may read no local, member, style or tracked
  value; read a slot as "$name" to compute with one`.
- **Alone as an argument.** A slot passed to a rule is passed alone, as `(• ink-of dim)`, not in an
  expression.

### Themes

`(style DarkPalette (is Palette) (for dark) …)` derives a sheet from `Palette`, for the **theme
tag** `dark`. In rows it is `• DarkPalette is Palette for dark`{.rows}. The base and the sheets
derived from it, at any depth, are a **family**.

- **What a derived sheet holds**: new values of its base's slots, and rules. It cannot add slots:
  `Derived StyleSheet "Dark" can only override slots of Base, which has no slot "extra"`. A rule it
  writes replaces its base's rule of that name; the two are not merged.
- **Tags** are turned on for an element and everything under it with the attribute `(for: dark)`,
  in rows `for dark`. `(for: dark #late)`, in rows `for dark = #late`, follows a value; `false`
  turns a tag off beneath, and `null` takes the tag from above.
- **One sheet of a family** is in force at each element: of the sheets whose tags, with their
  bases' tags, are all on, the one with the most tags, then the one derived furthest, then the
  first declared. Sheets of one family never mix, so two sibling sheets whose tags are both on do
  not combine: one of them applies. To combine looks, such as a shape and a color scheme, give each
  its own family.
- **A tag only** names a sheet. `(style Dark (for dark) …)` without `is` is refused.

### Extending a sheet

`(style-extend Palette …)`, in rows `• extend Palette`{.rows}, adds `using` rows and rules to a
sheet declared elsewhere, in the same module or one that imports it. An extension in a module that
nothing imports is not merged.

- **What an extension cannot add**: slots, `StyleSheet extensions may not declare slots`, and a
  `This` rule.
- **Libraries** spread one theme across modules this way: each terminal component module extends
  the terminal library's default theme with its own rules.

## Nested rules, own rules and generated content

```brix
(main
  (<> Page (id: page)
    (<> Card (text: "Card") (• framed)
      (<> Part (text: "body"))
    )
    (<> Part (text: "after"))
  )
  (for (in part (of (. page children-of) Part))
    (call (. part show) "")
  )
)

(type + Look (is Style)
  (prop + ink String (= "plain"))
  (prop + weight String (= "normal"))
)

(style PageSheet
  (using Look)
  (rule framed
    (<> Part (slot: first-child) (text: "rule"))
    (rule Part
      (= ink "framed")
    )
    (own-rule Part
      (= weight "bold")
    )
  )
)

(type + Part (is MarkupNode)
  (using Look)
  (prop + label String)
  (fn-do + This _
    (param label _)
  )
  (fn-do + show _
    (param indent String)
    (print (str indent label " " $ink " " $weight))
    (for (in part (of children-of Part))
      (call (. part show) (++ indent "  "))
    )
  )
)

(type + Card (is Part)
  (fn-do + This _
    (param label String)
    (call super label)
  )
  (content
    (<> This
      (<> Part (text: "heading"))
    )
  )
)

(type + Page (is MarkupRoot)
  (using PageSheet)
)
```

```output
Card plain normal
  rule framed bold
  heading framed bold
  body framed normal
after plain normal
```

- **A nested rule** reaches every matching element beneath the element its rule is applied to: the
  content its class declares, the content written in its body, slot content, content inside loops
  and branches, and generated content. `(rule Part …)` in `framed` reaches `heading` and `body`,
  not `after`, and not the card.
- **`(own-rule …)`**, in rows `• own Part`{.rows}, reaches only the markup the element's own class
  and its bases declare, and the content its rules generate: `heading` and `rule`, not `body`.
- **Generated content.** Markup in a rule adds elements next to the element or inside it, in the
  slot `(slot: …)` names: `before`, `after`, `first-child` or `last-child`. In rows it is
  `<Part (first-child) "rule"`{.rows}. Markup without a slot is refused, and so is any other slot
  name: `Unknown style content slot "middle"; expected before, after, first-child, or last-child`.
  Generated content can be inside a state, so it comes and goes with it. The content a rule
  generates does not take generated content from that rule again.

## Rules on any class

```brix
(main
  (<> Button (id: ok) (• primary))
  (<> Button (id: cancel))
  (<> Button (id: help) (• label "Help?") (• doubled))
  (<> Button (id: wide) (• doubled)
    (= width 3)
  )
  (for (in button (list ok cancel help wide))
    (print (str (. button text) " " (. button width)))
  )
)

(type + Button
  (prop + text _ (= ""))
  (prop + width _ (= 4))
  (rule Button
    (= text "button")
  )
  (rule ok
    (= text "OK")
  )
  (rule label
    (param value String)
    (= text value)
  )
  (rule primary
    (= text (++ text "!"))
  )
  (rule wider
    (= width (+ width 2))
  )
  (rule doubled (• wider)
    (= width (* width 2))
  )
)
```

```output
OK! 4
button 4
Help? 12
button 3
```

A class can declare rules of its own, and so can a class with no styles at all. Its rules apply to
its instances made with markup, in the order of [Which rules apply](#which-rules-apply).

- **Ordinary properties.** A class rule can set the class's ordinary properties. The assignment is
  made once, when the element is built, after its constructor. A later rule sees what earlier rules
  set, as `primary` and `doubled` do. An assignment that reads a tracked value with `#` follows it.
- **What a class rule can read**: the class's properties, as in `(++ text "!")`, and `$` values. A
  `$` read in a class rule is made once, while the element is built and before it settles.
- **Shadowing.** A class rule hides a sheet rule of the same name and parameter.
- **`(rule This …)`** is the class's defaults: it applies to every instance, at the defaults layer.
  It takes no parameter, a sheet cannot declare one, and `•` cannot name it. Today it takes effect
  only in a program that loads `/core/style`, which a module that declares a style sheet does.
- **A class's `content`** can set defaults too: assignments on its root `(<> This …)` are at the
  defaults layer.

## Reading style values

- **`$name`** reads the value of a style property that settling worked out, on `this`.
  `(. node $name)` reads it on another object, and `(?. node $name)` gives `null` for a `null`
  object.
- **Checked by type.** A read is checked against the styles the object's type uses, including those
  an interface it implements declares. A property those styles lack is
  `Style property "tone" not found`, whatever rules set at run time. A bare `$tone` in `main`, which
  has no `this`, is that error too.
- **Reads never settle.** Before a root first settles, an element reads initial values: in its
  constructor and in its `mounted` hook. After a change, reads give the old values until the next
  settle.
- **Out of the tree.** An element outside a tree reads initial values, and one removed from its
  tree goes back to them. An assignment from code to an element never placed in a tree is read
  back at once.
- **Not tracked.** A style value is not a tracked value: a derived property that reads `$tone` does
  not follow it, and `#$tone` is not a name.
- **Not assignable.** `(= $tone "red")` is refused:
  `Style values are computed by settling; "$tone" is not assignable`. Assign the property itself,
  `(= (. lamp tone) "red")`, which is an assignment at the code layer. Reading it without `$`,
  `(. lamp tone)`, is `Not supported`.

## Motion

The language gives styles two things for motion: keyframes, which describe a change over time as a
value, and a host that a library installs to play changes out over time. The GUI library builds
its animations on both. Without a host, as in a program with no GUI, a change of style takes effect
at once.

### Keyframes

```brix
(import /time/duration)
(import /units)

(main
  (<> Stage
    (<> Sprite (id: ghost) (• fading))
    (<> Sprite (id: bell) (• shaking))
  )
  (for (in sprite (list ghost bell))
    (print (?? (?. (. sprite $clip) summary) "none"))
  )
)

(type + Look (is Style)
  (prop + opacity Float (= 1.0))
  (prop + offset Float (= 0.0))
  (prop + clip (? Clip) (= null))
)

(style Motions
  (using Look)
  (keyframes fade animation 200ms
    (= repeat 2)
    (range opacity 0.0 1.0)
  )
  (keyframes shake (param distance Float) animation 300ms
    (track offset
      (at 0% 0.0)
      (at 50% distance)
      (at 100% 0.0)
    )
  )
  (rule fading
    (= clip fade)
  )
  (rule shaking
    (= clip (shape shake 4.0))
  )
)

(type + Clip
  (prop + summary String)
  (fn-do + This _
    (param duration Duration)
    (param repeat Int)
    (= summary (str (as duration Milliseconds UnitString) " x" repeat ":"))
  )
  (fn-do + (of range Value) Clip
    (param property (of StyleProperty Value))
    (param from Value)
    (param to Value)
    (+= summary (str " " from " to " to))
    (return this)
  )
  (fn-do + (of track Value) Clip
    (param property (of StyleProperty Value))
    (return this)
  )
  (fn-do + (of at Value) Clip
    (param property (of StyleProperty Value))
    (param at Ratio)
    (param value Value)
    (+= summary (str " " value "@" (as at Percent UnitString)))
    (return this)
  )
)

(fn-expr + animation Clip
  (param duration Duration)
  (param repeat Int (= 1))
  (call Clip duration repeat)
)

(type + Sprite (is MarkupNode)
  (using Look)
)

(type + Stage (is MarkupRoot)
  (using Motions)
)
```

```output
200ms x2: 0 to 1
300ms x1: 0@0% 4@50% 0@100%
```

`(keyframes fade animation 200ms …)` declares a slot whose value is built by calling the function
its kind names, here `animation`:

1. **`animation(200ms, repeat: 2)`**, from the duration and the settings, `(= name value)`.
2. **Then one call per row**, in order, on what the last call returned:
   - `(range opacity 0.0 1.0)` calls `.range(property, from, to)`. `_` for an end passes `null`.
   - `(track offset (at 0% 0.0) …)` calls `.track(property)` and then `.at(property, at, value)`
     for each stop.
   - A bare `(at 50% 1.2)` calls `.at(at, value)`.
   - Options after a row, as `(:easing (. Easing ease-out))`, are named arguments of its call.
3. **Each property** is passed as a `StyleProperty`, naming the style property the row moves.

Each row must name a style property the sheet uses, `Style property "glow" not found`, and give
values of its type. A sheet that derives from this one can restate a keyframes slot, as a theme
for reduced motion does.

A **shape** has one parameter: `(keyframes shake (param distance Float) …)`. A rule gives it a
value with `(shape shake 4.0)`, in rows `shake:4.0`. Its argument must be a constant; a rule's
parameter is refused.

A rule plays keyframes by assigning their slot, as `(= clip fade)`. In rows a keyframes slot is:

```rows
• fade animation 200ms
    repeat = 2
    opacity = 0.0 .. 1.0
```

Rows know two kinds, `animation`, which the GUI library defines, and `motion`, which `/math/motion`
defines. Its `motion` builds a `Motion` that can be sampled at a time:

```brix
(import /math/easing)
(import /math/motion)
(import /time/duration)
(import /units)

(main
  (<> Stage
    (<> Dot (id: dot) (• bouncy))
  )
  (let curve (as (!! (. dot $curve)) KeyframeMotion))
  (for (in ms (list 0 75 150 225 300))
    (let place (call (. curve progress) (as ms Milliseconds)))
    (print (str ms "ms " (call (. (. place 0) places) 2)))
  )
)

(type + Moves (is Style)
  (prop + curve (? Motion) (= null))
)

(style StageSheet
  (using Moves)
  (keyframes overshoot motion 300ms
    (at 0% 0.0)
    (at 50% 1.2 (:easing (. Easing ease-out)))
    (at 100% 1.0)
  )
  (rule bouncy
    (= curve overshoot)
  )
)

(type + Dot (is MarkupNode)
  (using Moves)
)

(type + Stage (is MarkupRoot)
  (using StageSheet)
)
```

```output
0ms 0
75ms 0.82
150ms 1.2
225ms 1.1
300ms 1
```

### Keyed properties and the motion host

- **Keyed properties.** A style can declare a property whose name has a dot,
  `(prop + motion.size Motion …)`. It is a property of its own, set by its whole name, in Brix
  `(= (. motion size) …)`, in rows `motion.size = …`. Only a style can declare one:
  `Only a Style may declare a keyed property such as "motion.size"`. A keyed property cannot be read
  with `$`.
- **The motion host.** Settling reports each changed style value to a `StyleMotionHost`, if one is
  installed. A value the host presents is what `$` reads until it takes the value away, which is
  how a change plays out over frames. Values that can be blended implement `Animatable`. The core
  library installs no host.
- **In the GUI library**, `Box` installs a host and drives it from the frame clock. Its styles have
  `motion`, one `Motion` for every property that only repaints, and keyed `motion.size`,
  `motion.position`, `motion.opacity` and others for one property each, as well as `animation`,
  which plays keyframes. Motions come from `/math/motion`: `spring`, `ease-out`, `linear`,
  `cubic-bezier`, `steps` and `instant`. `#is-entering` and `#is-leaving` style arrival and
  departure. See the standard library.

## How styles reach elements

- **Using.** A class takes a style's properties with `using`. Without it, setting the property is
  `Property "tone" not found; …` and reading it `Style property "tone" not found`. `using` names a
  style or a sheet; anything else is `Using target must be a Style or StyleSheet`. An interface can
  name styles with `using`, and a class that implements it must use them:
  `Chip must use TintStyle to implement Tinted`.
- **Visible sheets.** The sheets an element sees are those its own class uses and those the classes
  of the elements around it use, through bodies and component content. A rule from a sheet no
  element above uses does nothing, silently.
- **Two sheets, one name.** Two visible sheets with a rule of one name, or a slot of one name, are
  an error where the name is used: `Style rule "loud" is ambiguous between visible style sheets One
  and Two`.
- **Two styles, one property.** Two visible styles with a property of one name and different types
  are an error:
  `Style property "tone" has conflicting types in visible styles TextStyle and OtherStyle`.
- **A sheet's styles reach further.** A sheet's `using` lets its rules set those properties on
  elements whose classes do not use the style, such as a root, and an `@inherited` value set there
  passes down.

### Domains

A style can say what work a change to a property makes necessary:

```brix
(import /core/style)

(main
  (<> Screen (id: screen)
    (<> Panel (id: side)
      (<> Panel (id: rows))
    )
  )
  (= (. rows ink) "red")
  (call (. screen settle))
  (call (. screen redraw) side rows)
  (= (. rows width) 20)
  (call (. screen settle))
  (call (. screen redraw) side rows)
)

(type + Look (is Style)
  (prop + ink String (= "black"))
  (prop + width Int (= 10))
  (style-domains ink (domain paint))
  (style-domains width (domain layout) (domain layout parent))
)

(type + Panel (is MarkupNode)
  (using Look)
)

(type + Screen (is MarkupRoot)
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (print (str "settled: paint " (. flush is-paint-invalid) ", layout " (. flush is-layout-invalid)))
  )
  (fn-do + redraw _
    (param side Panel)
    (param rows Panel)
    (print (str "  side: paint " (. side $is-paint-invalid) ", layout " (. side $is-layout-invalid)))
    (print (str "  rows: paint " (. rows $is-paint-invalid) ", layout " (. rows $is-layout-invalid)))
    (for (in panel (list side rows))
      (= (. panel $is-paint-invalid) false)
      (= (. panel $is-layout-invalid) false)
    )
  )
)
```

```output
settled: paint false, layout false
settled: paint true, layout false
  side: paint false, layout false
  rows: paint true, layout false
settled: paint false, layout true
  side: paint false, layout true
  rows: paint false, layout true
```

- **`(style-domains width (domain layout) (domain layout parent))`**, in rows
  `. width -> layout, layout on parent`{.rows}, says a change to `width` needs layout of the element
  and of its parent. A target is `self`, the default, or `parent`. Names are free, and a program
  has at most 32.
- **Flags.** Settling a change raises the domain on the element as `$is-layout-invalid`, and on
  each element above it as `$is-layout-invalid-below`. These are the only `$` values code can
  assign: the code that does the work clears them.
- **The root** is told after each settle that changed something, by `markup-batch-did-flush`, with
  a `MarkupBatchFlush` whose flags are the domains raised. A settle that changes nothing does not
  call it. The terminal library decides when to repaint this way.

## Mistakes

| Mistake | Message |
| --- | --- |
| A style without `/core/style` | `Type "Style" not found; it is declared in /core/style, which this module does not import. Add the import /core/style` |
| A name no rule declares | `No style rule is named "wied". Write one in a style sheet, or, where a library applies it for a theme to style, declare it there with an empty rule` |
| An unknown annotation | `Unknown style property annotation "@glowing"; only "@inherited" is supported` |
| A slot in an extension | `StyleSheet extensions may not declare slots` |
| A `This` rule in a sheet | `StyleSheets may not declare a "This" defaults rule` |
| A base that is not a sheet | `StyleSheet base "Plain" is not a StyleSheet` |
| A tag given twice | `StyleSheet tag "dark" is repeated` |
| Two rules of one name and parameter | `Redefinition of style rule` |
| A second parameter | ``(param …) cannot be a `row` of (rule …), which takes Style.`` |
| A step in a rule | ``(print …) cannot be a `row` of (rule …), which takes Style.`` |
| A rule named in a sheet rule's body | `StyleSheet style rules may only contain style declarations and generated content` |
| A non-`Bool` condition | `Style rule conditions must be Bool, but this condition is I64` |
| A `This` rule with a parameter | `Style rule "This" must be parameterless` |
| A domain of a missing property | `Style property "nope" does not exist` |
| An unknown domain target | `Unknown style invalidation target "child"; expected "self" or "parent"` |

## Rows

| Brix | Rows |
| --- | --- |
| `(style Palette (using Look) …)` | `• Palette`, with `using Look` in its header |
| `(style Dark (is Palette) (for dark light) …)` | `• Dark is Palette for dark, light` |
| `(style-extend Palette …)` | `• extend Palette` |
| `(rule set-x (param value Int) (= x value))` | `• set-x:value Int`, with `x = value` under it |
| `(rule answer (param n Int) (• double-x) (= x n))` | `• answer:n Int • double-x`, with `x = n` under it |
| `(own-rule inner …)` | `• own inner` |
| `(when-style #is-on …)`, `(when-style (not #is-on) …)` | `when #is-on`, `when !#is-on` |
| `(style-set is-on x 3)` | `is-on:x = 3` |
| `(shape shake 4)` | `shake:4` |
| `(<> Thing (id: thing) (• set-x 3) (• double-x) (• answer 5))` | `<Thing thing • set-x(3) double-x answer:5` |
| `(• bold (if: #heavy))` | `• bold = #heavy` |
| `(for: dark)`, `(for: dark #late)` | `for dark`, `for dark = #late` |
| `(<> Part (slot: first-child) (text: "rule"))` | `<Part (first-child) "rule"` |
| `(style-domains ink (domain paint) (domain layout parent))` | `. ink -> paint, layout on parent` |
| `(prop + font String @inherited (= "sans"))` | `+ @inherited font String = "sans"` |
| `$name`, `(. node $name)`, `(?. node $name)` | `$name`, `node.$name`, `node?.$name` |

On an element's row, rules come before theme tags: `<Line "dusk" • loud:4 for dark = #late`.
