# Facets

### A module named after another beside it belongs to that module's file family, and is built for every platform

```brix
(import ./lib/box)
(blank)

(main
  (print (call box-name))
)
```

```brix file=lib/box.bx
(import ./box.scroll)

(fn-expr + box-name String
  (str "box " (call scroll-name))
)
```

```brix file=lib/box.scroll.bx
(import ./box)

(fn-expr + scroll-name String
  "scroll"
)
```

```output
box scroll
```

### Only a module's file family can import the family's modules

```brix compile-error
(import ./lib/box.scroll)
(blank)

(main
  (print 1)
)
```

```brix file=lib/box.bx
(main
  (print 2)
)
```

```brix file=lib/box.scroll.bx
(main
  (print 3)
)
```

```output
Exception: Module "./lib/box.scroll" belongs to the file family of box.bx, so only that family can import it
<no file:2>
    (import ./lib/box.scroll)
    ^^^^^^^^^^^^^^^^^^^^^^^^^
```

### One module of a family can import another

```brix
(import ./lib/box)
(blank)

(main
  (print (call box-name))
)
```

```brix file=lib/box.bx
(import ./box.scroll)

(fn-expr + box-name String
  (call scroll-name)
)
```

```brix file=lib/box.scroll.bx
(import ./box.focus)

(fn-expr + scroll-name String
  (str "scroll " (call focus-name))
)
```

```brix file=lib/box.focus.bx
(fn-expr + focus-name String
  "focus"
)
```

```output
scroll focus
```

### A facet's stored property is held in its class's objects, with the value the facet writes for it, and its methods are the class's

```brix
(import ./lib/box)
(blank)

(main
  (let box (call Box))
  (print (. box offset))
  (call (. box scroll-to) 5)
  (print (. box offset))
)
```

```brix file=lib/box.bx
(import ./box.scroll)

(type + Box
  (prop + width _ (= 10))
)
```

```brix file=lib/box.scroll.bx
(import ./box)

(type + BoxScroll (facet-of Box)
  (prop + offset _ (= 3))
  (fn-do + scroll-to _
    (param point Int)
    (= offset (+ point width))
  )
)
```

```output
3
15
```

### The class's own code uses its facets' public members by name, as its own

```brix
(import ./lib/box)
(blank)

(main
  (let box (call Box))
  (call (. box grow))
  (print (call (. box describe)))
)
```

```brix file=lib/box.bx
(import ./box.scroll)

(type + Box
  (prop + width _ (= 10))
  (fn-expr + describe String
    (str "width " width " offset " offset)
  )
  (fn-do + grow _
    (call scroll-to 2)
    (+= (. this offset) 1)
  )
)
```

```brix file=lib/box.scroll.bx
(import ./box)

(type + BoxScroll (facet-of Box)
  (prop + offset _ (= 3))
  (fn-do + scroll-to _
    (param point Int)
    (= offset (+ point width))
  )
)
```

```output
width 10 offset 13
```

### A class derived from one with facets has their properties, given their values by its constructors, after the class's own and before its own

```brix
(import ./lib/panel)
(blank)

(main
  (let panel (call Panel))
  (print (. panel offset))
  (print (. panel title))
  (call (. panel scroll-to) 1)
  (print (. panel offset))
  (print (. panel area))
)
```

```brix file=lib/panel.bx
(import ./box)

(type + Panel (is Box)
  (prop + title _ (= "main"))
)
```

```brix file=lib/box.bx
(import ./box.scroll)

(type + Box
  (prop + width _ (= 10))
)
```

```brix file=lib/box.scroll.bx
(import ./box)

(type + BoxScroll (facet-of Box)
  (prop + offset _ (= 3))
  (prop + area _ (:= (* width 2)))
  (fn-do + scroll-to _
    (param point Int)
    (= offset (+ point width))
  )
)
```

```output
3
main
11
20
```

### A facet reaches its class's private members and its module's private declarations, which the class's module does not reach of the facet's

```brix
(import ./lib/box)
(blank)

(main
  (print (call (. (call Box) measure)))
)
```

```brix file=lib/box.bx
(import ./box.scroll)

(type + Box
  (prop _ width _ (= 10))
)

(fn-expr _ doubled Int
  (param value Int)
  (* value 2)
)
```

```brix file=lib/box.scroll.bx
(import ./box)

(type + BoxScroll (facet-of Box)
  (fn-expr + measure Int
    (call doubled width)
  )
)
```

```output
20
```

### A facet can be part of another facet, and reaches its members and its class's

```brix
(import ./lib/box)
(blank)

(main
  (let box (call Box))
  (print (call (. box indicator-text)))
)
```

```brix file=lib/box.bx
(import ./box.scroll)
(import ./box.scroll.indicator)

(type + Box
  (prop + width _ (= 10))
)
```

```brix file=lib/box.scroll.bx
(import ./box)

(type + BoxScroll (facet-of Box)
  (prop + offset _ (= 3))
)
```

```brix file=lib/box.scroll.indicator.bx
(import ./box)
(import ./box.scroll)

(type + BoxScrollIndicator (facet-of BoxScroll)
  (prop + shown _ (= true))
  (fn-expr + indicator-text String
    (str shown " " offset " " width)
  )
)
```

```output
true 3 10
```

### Facets of one class name their private members as they like, and each module reaches its own

```brix
(import ./lib/box)
(blank)

(main
  (let box (call Box))
  (print (call (. box scroll-text)))
  (print (call (. box focus-text)))
)
```

```brix file=lib/box.bx
(import ./box.focus)
(import ./box.scroll)

(type + Box)
```

```brix file=lib/box.focus.bx
(import ./box)

(type + BoxFocus (facet-of Box)
  (prop _ cursor _ (= "focus"))
  (fn-expr _ marker String :get
    "focus marker"
  )
  (fn-expr _ helper String
    "focus helper"
  )
  (fn-expr + focus-text String
    (str cursor " " marker " " (call helper))
  )
)
```

```brix file=lib/box.scroll.bx
(import ./box)

(type + BoxScroll (facet-of Box)
  (prop _ cursor _ (= 7))
  (fn-expr _ marker Int :get
    (+ cursor 1)
  )
  (fn-expr _ helper String
    "scroll helper"
  )
  (fn-expr + scroll-text String
    (str cursor " " marker " " (call helper))
  )
)
```

```output
7 8 scroll helper
focus focus marker focus helper
```

### A facet reaches the public members of the facets it sees, on its own object and on any other of its class, and code outside the family reaches every facet's

```brix
(import ./lib/box)
(blank)

(main
  (let box (call Box))
  (let other (call Box))
  (print (call (. box scroll-text) other))
  (print (. box focused))
  (print (. box offset))
)
```

```brix file=lib/box.bx
(import ./box.focus)
(import ./box.scroll)

(type + Box)
```

```brix file=lib/box.focus.bx
(import ./box)

(type + BoxFocus (facet-of Box)
  (prop + focused _ (= true))
)
```

```brix file=lib/box.scroll.bx
(import ./box)

(type + BoxScroll (facet-of Box)
  (sees BoxFocus)
  (prop + offset _ (= 3))
  (fn-expr + scroll-text String
    (param other Box)
    (str focused " " (. other focused) " " (. other offset))
  )
)
```

```output
true true 3
true
3
```

### A facet does not reach a sibling it does not see, on its own object or on another of its class

```brix compile-error
(import ./lib/box)
(blank)

(main
  (print (call (. (call Box) scroll-text) (call Box)))
  (print (call (. (call Box) focused-text) (call Box)))
  (print (call (. (call Box) ring-text) (call Box)))
)
```

```brix file=lib/box.bx
(import ./box.focus)
(import ./box.scroll)

(type + Box)
```

```brix file=lib/box.focus.bx
(import ./box)

(type + BoxFocus (facet-of Box)
  (prop + focused _ (= true))
  (fn-expr ^ ring Int
    2
  )
)
```

```brix file=lib/box.scroll.bx
(import ./box)

(type + BoxScroll (facet-of Box)
  (fn-expr + scroll-text String
    (param other Box)
    (str focused)
  )
  (fn-expr + focused-text String
    (param other Box)
    (str (. other focused))
  )
  (fn-expr + ring-text String
    (param other Box)
    (str (call (. other ring)))
  )
)
```

```output
Exception: Name "focused" not found; facet BoxFocus declares it, and only the facets a facet sees are in reach. Write "sees BoxFocus" under the facet that uses it
<lib/box.scroll.bx:6>
        (str focused)
             ^^^^^^^

Exception: Box has no property "focused" here; facet BoxFocus declares it, and only the facets a facet sees are in reach. Write "sees BoxFocus" under the facet that uses it
<lib/box.scroll.bx:10>
        (str (. other focused))
             ^^^^^^^^^^^^^^^^^

Exception: Box has no method "ring" here; facet BoxFocus declares it, and only the facets a facet sees are in reach. Write "sees BoxFocus" under the facet that uses it
<lib/box.scroll.bx:14>
        (str (call (. other ring)))
             ^^^^^^^^^^^^^^^^^^^^^
```

### A member declared with ^ is shared with its class's file family: the class's own code, the facets that see its facet and the family's other modules reach it, on any object of the class

```brix
(import ./lib/box)
(blank)

(main
  (let box (call Box))
  (call (. box grow))
  (print (call (. box describe)))
  (print (call (. (call Box) describe)))
)
```

```brix file=lib/box.bx
(import ./box.focus)
(import ./box.scroll)
(import ./box.tools)

(type + Box
  (prop ^ width _ (= 10))
  (fn-do + This _)
  (fn-do ^ This _
    (param start Int)
    (= width start)
  )
  (fn-expr + describe String
    (str offset " " (call focus-text) " " (call tools-text this))
  )
  (fn-do + grow _
    (call keep-end)
  )
)
```

```brix file=lib/box.focus.bx
(import ./box)

(type + BoxFocus (facet-of Box)
  (sees BoxScroll)
  (fn-expr + focus-text String
    (str (call pinned) " " width)
  )
)
```

```brix file=lib/box.scroll.bx
(import ./box)

(type + BoxScroll (facet-of Box)
  (prop ^ offset _ (= 3))
  (fn-expr ^ pinned Bool
    (> offset 3)
  )
  (fn-do ^ keep-end _
    (= offset (+ width 1))
  )
)
```

```brix file=lib/box.tools.bx
(import ./box)

(fn-expr + tools-text String
  (param box Box)
  (str (. box offset) " " (call (. box pinned)) " " (. box width) " " (. (call Box 4) width))
)
```

```output
11 true 10 11 true 10 4
3 false 10 3 false 10 4
```

### Code outside a class's file family can't use a member declared with ^, nor can a class below it there, and a module declares its own values, functions and types with + or _

```brix compile-error
(import ./lib/other)
(blank)

(main
  (call keep)
  (print (call other-text))
)
```

```brix file=lib/box.bx
(import ./box.scroll)

(type + Box
  (prop ^ width _ (= 10))
  (fn-do + This _)
  (fn-do ^ This _
    (param start Int)
    (= width start)
  )
)
```

```brix file=lib/box.scroll.bx
(import ./box)

(type + BoxScroll (facet-of Box)
  (prop ^ offset _ (= 3))
  (fn-do ^ keep-end _
    (= offset (+ width 1))
  )
)
```

```brix file=lib/other.bx
(import ./box)
(import ./helper)
(import ./kinds)
(blank)
(prop ^ limit _ (= 3))

(fn-expr + other-text String
  (str (call width) " " (call offset) " " (call made) " " (call (. (call Panel) panel-text)))
)

(fn-do + keep _
  (call (. (call Box) keep-end))
)

(fn-expr + width Int
  (. (call Box) width)
)

(fn-expr + offset Int
  (. (call Box) offset)
)

(fn-expr + made Int
  (. (call Box 5) width)
)

(type + Panel (is Box)
  (fn-expr + panel-text String
    (str (. this width))
  )
)
```

```brix file=lib/helper.bx
(fn-expr ^ helper Int
  1
)
```

```brix file=lib/kinds.bx
(type ^ Kind)
```

```output
Exception: Only a member can be shared with its file family; declare this + or _
<lib/helper.bx:1>
    (fn-expr ^ helper Int
    ^^^^^^^^^^^^^^^^^^^^

Exception: Only a member can be shared with its file family; declare this + or _
<lib/kinds.bx:1>
    (type ^ Kind)
    ^^^^^^^^^^^^^

Exception: Only a member can be shared with its file family; declare this + or _
<lib/other.bx:5>
    (prop ^ limit _ (= 3))
    ^^^^^^^^^^^^^^^^^^^^^^

Exception: Method "keep-end" of Box is shared, declared with ^, so only its own file family can call it
<lib/other.bx:12>
      (call (. (call Box) keep-end))
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Exception: Property "width" is shared, declared with ^, so only its own file family can use it
<lib/other.bx:16>
      (. (call Box) width)
      ^^^^^^^^^^^^^^^^^^^^

Exception: Property "offset" is shared, declared with ^, so only its own file family can use it
<lib/other.bx:20>
      (. (call Box) offset)
      ^^^^^^^^^^^^^^^^^^^^^

Exception: No constructor of Box accepts (I64); its declared constructors are "Box()"; "Box(start Int)" is shared with its file family
<lib/other.bx:24>
      (. (call Box 5) width)
               ^^^

Exception: Property "width" is shared, declared with ^, so only its own file family can use it
<lib/other.bx:29>
        (str (. this width))
             ^^^^^^^^^^^^^^
```

### A member declared with ^ takes its name as a public one does: no two facets of a class declare it, and a class below the class declares it only as a public one's

```brix compile-error
(import ./lib/panel)
(blank)

(main
  (print 1)
)
```

```brix file=lib/box.bx
(import ./box.focus)
(import ./box.scroll)

(type + Box)
```

```brix file=lib/box.focus.bx
(import ./box)

(type + BoxFocus (facet-of Box)
  (prop ^ step _ (= 1))
)
```

```brix file=lib/box.scroll.bx
(import ./box)

(type + BoxScroll (facet-of Box)
  (prop + step _ (= 2))
)
```

```brix file=lib/panel.bx
(import ./box)

(type + Panel (is Box)
  (prop _ step _ (=> 3))
)
```

```output
Exception: Facets BoxFocus and BoxScroll of Box both declare step; a facet's members that are not private are its class's, so a name is one facet's
<lib/box.focus.bx:4>
      (prop ^ step _ (= 1))
      ^^^^^^^^^^^^^^^^^^^^^

Exception: Facets BoxScroll and BoxFocus of Box both declare step; a facet's members that are not private are its class's, so a name is one facet's
<lib/box.scroll.bx:4>
      (prop + step _ (= 2))
      ^^^^^^^^^^^^^^^^^^^^^

Exception: Panel can't declare step, since facet BoxFocus of Box declares it
<lib/panel.bx:4>
      (prop _ step _ (=> 3))
      ^^^^^^^^^^^^^^^^^^^^^^
```

### A facet declares events, which its class's objects publish: the facet, the class and the facets that see it name one alone, and other code reaches it on the object

```brix
(import ./lib/box)
(blank)

(main
  (let box (call Box))
  (let other (call Box))
  (call (. box watch) other)
  (for (on value (. box scrolled))
    (print (str "heard " value))
  )
  (call (. box scroll-by) 3)
  (call (. box jump))
  (call (. box nudge))
  (call (. other scroll-by) 7)
)
```

```brix file=lib/box.bx
(import ./box.focus)
(import ./box.scroll)

(type + Box
  (fn-do + jump _
    (<- scrolled 10)
  )
)
```

```brix file=lib/box.scroll.bx
(import ./box)

(type + BoxScroll (facet-of Box)
  (event + scrolled Int)
  (fn-do + scroll-by _
    (param amount Int)
    (<- scrolled amount)
  )
)
```

```brix file=lib/box.focus.bx
(import ./box)

(type + BoxFocus (facet-of Box)
  (sees BoxScroll)
  (fn-do + nudge _
    (<- scrolled 1)
  )
  (fn-do + watch _
    (param other Box)
    (for (on value (. other scrolled))
      (print (str "focus heard " value))
    )
  )
)
```

```output
heard 3
heard 10
heard 1
focus heard 7
```

### A facet's private event is its own file's and its shared one its class's family's, and closing an object ends what subscribes to its facets' events

```brix
(import ./lib/box)
(blank)

(main
  (let box (call Box))
  (call (. box listen))
  (call (. box track))
  (for (on value (. box scrolled))
    (print (str "heard " value))
  )
  (call (. box scroll-by) 1)
  (call (. box close))
  (call (. box scroll-by) 2)
  (print "done")
)
```

```brix file=lib/box.bx
(import ./box.scroll)

(type + Box
  (fn-do + listen _
    (for (on value pinned)
      (print (str "pinned " value))
    )
  )
  (fn-do + close _
    (print "closed")
  )
)
```

```brix file=lib/box.scroll.bx
(import ./box)

(type + BoxScroll (facet-of Box)
  (event + scrolled Int)
  (event ^ pinned Bool)
  (event _ moved Int)
  (fn-do + track _
    (for (on value moved)
      (print (str "moved " value))
    )
  )
  (fn-do + scroll-by _
    (param amount Int)
    (<- moved amount)
    (<- pinned (> amount 1))
    (<- scrolled amount)
  )
)
```

```output
moved 1
pinned false
heard 1
closed
done
```

### A facet's public event bubbles on an EventNode class, and an ancestor outside the family names it by its class

```brix
(import ./lib/picker)
(blank)

(main
  (let root (call Panel "root"))
  (let inner (call Picker "inner" root))
  (call (. inner pick) "a")
)

(type + Panel (is Node)
  (subscribe-for (on value (. Picker picked))
    (print (str name " heard " value))
  )
)
```

```brix file=lib/picker.bx
(import ./picker.pick)

(type + Node
  (implements EventNode)
  (prop + name String)
  (prop + up (? Node))
  (fn-do + This _
    (param name _)
    (param up _ (= null))
  )
  (fn-expr + event-parent (? EventNode) :get
    up
  )
)

(type + Picker (is Node))
```

```brix file=lib/picker.pick.bx
(import ./picker)

(type + PickerPick (facet-of Picker)
  (event + picked String)
  (fn-do + pick _
    (param text String)
    (<- picked text)
  )
)
```

```output
root heard a
```

### Code reaches a facet's events as it reaches its other members, an event's name is its class's alone, and a facet's event carries a value

```brix compile-error
(import ./lib/box)
(import ./lib/other)
(import ./lib/panel)
(blank)

(main
  (let box (call Box))
  (call (. box peek))
  (call (. box nudge))
  (call (. box watch) box)
  (call hear box)
)
```

```brix file=lib/box.bx
(import ./box.blink)
(import ./box.focus)
(import ./box.held)
(import ./box.scroll)

(type + Box
  (fn-do + peek _
    (for (on value (. this moved))
      (print value)
    )
  )
)
```

```brix file=lib/box.scroll.bx
(import ./box)

(type + BoxScroll (facet-of Box)
  (event + scrolled Int)
  (event ^ pinned Bool)
  (event _ moved Int)
  (event _ shaken Int)
)
```

```brix file=lib/box.focus.bx
(import ./box)

(type + BoxFocus (facet-of Box)
  (event _ shaken Int)
  (fn-do + nudge _
    (<- scrolled 1)
  )
  (fn-do + watch _
    (param other Box)
    (for (on value (. other scrolled))
      (print value)
    )
  )
)
```

```brix file=lib/box.blink.bx
(import ./box)

(type + BoxBlink (facet-of Box)
  (event + blinked Void)
)
```

```brix file=lib/box.held.bx
(import ./box)

(type + BoxHeld (facet-of Box)
  (prop _ held _ (= 1))
  (event _ held Int)
)
```

```brix file=lib/other.bx
(import ./box)

(fn-do + hear _
  (param box Box)
  (for (on value (. box pinned))
    (print value)
  )
)
```

```brix file=lib/panel.bx
(import ./box)

(type + Panel (is Box)
  (event + scrolled Int)
)
```

```output
Exception: An event gives each subscriber a value, so it cannot carry Void; an event that only signals carries Bool, as in "+ event blinked Bool"
<lib/box.blink.bx:4>
      (event + blinked Void)
                       ^^^^

Exception: Facets BoxFocus and BoxScroll of Box both declare shaken; an event's name is its class's alone, private or not
<lib/box.focus.bx:4>
      (event _ shaken Int)
      ^^^^^^^^^^^^^^^^^^^^

Exception: Name "scrolled" not found; facet BoxScroll declares it, and only the facets a facet sees are in reach. Write "sees BoxScroll" under the facet that uses it
<lib/box.focus.bx:6>
        (<- scrolled 1)
            ^^^^^^^^

Exception: Box has no property "scrolled" here; facet BoxScroll declares it, and only the facets a facet sees are in reach. Write "sees BoxScroll" under the facet that uses it
<lib/box.focus.bx:10>
        (for (on value (. other scrolled))
                       ^^^^^^^^^^^^^^^^^^

Exception: Redefinition of class member
<lib/box.held.bx:5>
      (event _ held Int)
      ^^^^^^^^^^^^^^^^^^

Exception: Event "pinned" is shared, declared with ^, so only its own file family can use it
<lib/box.scroll.bx:5>
      (event ^ pinned Bool)
      ^^^^^^^^^^^^^^^^^^^^^

Exception: Event "moved" is private
<lib/box.scroll.bx:6>
      (event _ moved Int)
      ^^^^^^^^^^^^^^^^^^^

Exception: Facets BoxScroll and BoxFocus of Box both declare shaken; an event's name is its class's alone, private or not
<lib/box.scroll.bx:7>
      (event _ shaken Int)
      ^^^^^^^^^^^^^^^^^^^^

Exception: Panel can't declare scrolled, since facet BoxScroll of Box declares it
<lib/panel.bx:4>
      (event + scrolled Int)
      ^^^^^^^^^^^^^^^^^^^^^^
```

### A class below one with facets overrides a facet's methods, computed properties and setters, which code calls through the owner's type and the facet through this, and its override reaches the facet's own through super

```brix
(import ./lib/box)
(import ./lib/panel)
(blank)

(main
  (let box Box (call Panel))
  (= (. box title) "a")
  (print (call (. box describe)))
  (print (. box label))
  (print (call (. box shout)))
  (print (. box last))
  (let plain (call Box))
  (= (. plain title) "b")
  (print (call (. plain describe)))
  (print (. plain label))
  (print (call (. plain shout)))
  (print (. plain last))
)
```

```brix file=lib/box.bx
(import ./box.text)

(type + Box)
```

```brix file=lib/box.text.bx
(import ./box)

(type + BoxText (facet-of Box)
  (prop + last _ (= ""))
  (prop + label _ (=> "box"))
  (fn-expr + describe String
    (str "plain " label)
  )
  (fn-expr + shout String
    (str (call describe) "!")
  )
  (fn-do + title _ :set (param value String)
    (= last (str "box " value))
  )
)
```

```brix file=lib/panel.bx
(import ./box)

(type + Panel (is Box)
  (prop + label _ (=> "panel"))
  (fn-expr + describe String
    (str "fancy " (call (. super describe)))
  )
  (fn-do + title _ :set (param value String)
    (= last (str "panel " value))
  )
)
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

```brix
(import ./lib/box)
(import ./lib/panel)
(blank)

(main
  (let box (call Box 3 4 5 6))
  (print (call (. box reach)))
  (print (. box cached))
  (print (call (. box secret)))
  (print (. (call Box) offset))
  (print (. (call Box) cached))
  (print (. (call Panel 8) offset))
  (print (. (call Panel) offset))
  (print (call (. (call Panel) reach)))
  (print (. (call Panel) cached))
  (print (. (call Tab) offset))
  (print (. (call Tab) cached))
)
```

```brix file=lib/box.bx
(import ./box.scroll)

(type + Box
  (prop + size _ (= 1))
  (fn-do + This _
    (param offset Int)
    (param limit Int)
    (param cached Int)
    (param hidden Int)
    (= size hidden)
  )
  (fn-do + This _)
)
```

```brix file=lib/box.scroll.bx
(import ./box)

(type + BoxScroll (facet-of Box)
  (prop + offset _ (= 0))
  (prop ^ limit _ (= 1))
  (prop + cached _ (:= (* size 100)))
  (prop _ hidden _ (= 2))
  (fn-expr + reach Int
    (+ offset limit)
  )
  (fn-expr + secret Int
    (+ hidden size)
  )
)
```

```brix file=lib/panel.bx
(import ./box)

(type + Panel (is Box)
  (prop + offset _ (= 9))
  (prop + cached _ (:= (* size 7)))
  (fn-do + This _
    (param offset Int)
  )
  (fn-do + This _)
)

(type + Tab (is Panel))
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

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(import ./lib/card)
(blank)

(main
  (<> Deck (id: demo)
    (<> Card
      (= ink 9)
      (= margin 3)
    )
  )
  (blank)
  (let card (as (!! (. demo %first-child)) Card))
  (print (call (. card describe)))
  (call (. demo markup-batch-queue checkpoint))
  (print (call (. card describe)))
  (print (. card $ink))
)

(type + Deck (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)
```

```brix file=lib/card.bx
(import /core/style)
(import /core/tests/support/markup-fixtures)
(import ./card.paint)

(type + Card (is Box))

(type + CardStyle (is Style)
  (prop + ink I64 @inherited (= 7))
  (prop + margin I64 (= 0))
  (blank)
  (style-domains ink (domain paint))
  (style-domains margin (domain layout))
)
```

```brix file=lib/card.paint.bx
(import /core/style)
(import ./card)

(type + CardPaint (facet-of Card)
  (using CardStyle)
  (fn-do + describe String
    (let ink $ink)
    (let margin $margin)
    (return (str "ink=" ink " margin=" margin))
  )
)
```

```output
ink=7 margin=0
ink=9 margin=3
9
```

### A facet's sections, when and whenever rules and dependencies run for its class's objects, after the class's own and before those of a class below

```brix
(import ./lib/box)
(import ./lib/panel)
(blank)

(main
  (let box (call Box))
  (call (. box move) 3)
  (call (. box nudge) 4)
  (print (call (. ", " join) (. box log)))
  (print (. box seen))
  (let panel (call Panel))
  (call (. panel move) 5)
  (print (call (. ", " join) (. panel log)))
)
```

```brix file=lib/box.bx
(import ./box.watch)

(type + Box
  (event + moved Int)
  (prop + log _ (= (call (of List String))))
  (subscribe-for (on value moved)
    (+= log (str "box " value))
  )
  (fn-do + move _
    (param amount Int)
    (<- moved amount)
  )
)
```

```brix file=lib/box.watch.bx
(import ./box)

(type + BoxWatch (facet-of Box)
  (event + nudged Int)
  (prop + seen _ (= 0))
  (blank)
  (depends seen-changed seen)
  (subscribe-for (on value moved)
    (+= log (str "watch " value))
    (+= seen value)
  )
  (subscribe-for (on value nudged)
    (+= log (str "nudged " value))
  )
  (fn-do + nudge _
    (param amount Int)
    (<- nudged amount)
  )
  (whenever-rule #seen
    (+= log (str "seen " seen))
  )
  (when-rule (> #seen 4)
    (+= log "big")
  )
  (fn-do + seen-changed _
    (+= log (str "changed " seen))
  )
)
```

```brix file=lib/panel.bx
(import ./box)

(type + Panel (is Box)
  (subscribe-for (on value moved)
    (+= log (str "panel " value))
  )
)
```

```output
box 3, watch 3, changed 3, seen 3, nudged 4
3
box 5, watch 5, changed 5, seen 5, big, panel 5
```

### A facet is part of a class or of another facet of it, declared in the class's file family, and sees only other facets of the class

```brix compile-error
(import ./lib/box)
(import ./lib/other)
(blank)

(main
  (print 1)
)
```

```brix file=lib/box.bx
(import ./box.focus)
(import ./box.proto)
(import ./box.ring)
(import ./box.scroll)
(import ./box.view)

(type + Box)

(type + Proto (is Interface))
```

```brix file=lib/box.scroll.bx
(import ./box)

(type + BoxScroll (facet-of Bxo))
```

```brix file=lib/box.focus.bx
(import ./box)

(type + BoxFocus (facet-of Box)
  (sees Box)
)
```

```brix file=lib/box.view.bx
(import ./box)

(type + BoxView (facet-of Box)
  (sees BoxNothing)
)
```

```brix file=lib/box.ring.bx
(import ./box)

(type + RingA (facet-of RingB))

(type + RingB (facet-of RingA))
```

```brix file=lib/box.proto.bx
(import ./box)

(type + ProtoPart (facet-of Proto))
```

```brix file=lib/other.bx
(import ./box)

(type + Extra (facet-of Box))
```

```output
Exception: "Box" is no other facet of Box, so BoxFocus can't see it
<lib/box.focus.bx:4>
      (sees Box)
            ^^^

Exception: ProtoPart can't be part of Proto, since only a class has facets
<lib/box.proto.bx:3>
    (type + ProtoPart (facet-of Proto))
                                ^^^^^

Exception: RingA is part of itself, through the facets it is part of
<lib/box.ring.bx:3>
    (type + RingA (facet-of RingB))
                            ^^^^^

Exception: "Bxo" names no class or facet that BoxScroll can be part of
<lib/box.scroll.bx:3>
    (type + BoxScroll (facet-of Bxo))
                                ^^^

Exception: "BoxNothing" is no other facet of Box, so BoxView can't see it
<lib/box.view.bx:4>
      (sees BoxNothing)
            ^^^^^^^^^^

Exception: Extra is a facet of Box, so it is declared in the file family of box.bx, in a module named after it
<lib/other.bx:3>
    (type + Extra (facet-of Box))
                            ^^^
```

### A facet holds what a class body may but for what only a class has, and markup, and its stored properties have values

```brix compile-error
(import ./lib/box)
(blank)

(main
  (print 1)
)
```

```brix file=lib/box.bx
(import ./box.base)
(import ./box.count)
(import ./box.make)
(import ./box.markup)
(import ./box.params)
(import ./box.static)

(type + Box)

(type + Proto (is Interface))
```

```brix file=lib/box.params.bx
(import ./box)

(type + (of BoxParams T) (facet-of Box))
```

```brix file=lib/box.base.bx
(import ./box)

(type + BoxBase (facet-of Box) (is Proto))
```

```brix file=lib/box.count.bx
(import ./box)

(type + BoxCount (facet-of Box)
  (prop + label (? String) (= null))
  (prop + parent (? Box))
  (blank)
  (prop + count Int)
)
```

```brix file=lib/box.make.bx
(import ./box)

(type + BoxMake (facet-of Box)
  (fn-do + This _)
)
```

```brix file=lib/box.markup.bx
(import ./box)

(type + BoxMarkup (facet-of Box)
  (content
    (<> This)
  )
)
```

```brix file=lib/box.static.bx
(import ./box)

(type + BoxStatic (facet-of Box)
  (fn-expr + make Int :static
    1
  )
)
```

```output
Exception: BoxBase is a facet, part of its owner, so it derives from no class
<lib/box.base.bx:3>
    (type + BoxBase (facet-of Box) (is Proto))
                                       ^^^^^

Exception: count needs a value or an optional type, since BoxCount is a facet and no constructor gives it one
<lib/box.count.bx:7>
      (prop + count Int)
      ^^^^^^^^^^^^^^^^^^

Exception: BoxMake is a facet, so it has no constructors; its owner's give its properties their first values
<lib/box.make.bx:4>
      (fn-do + This _)
      ^^^^^^^^^^^^^^^^

Exception: BoxMarkup is a facet, so it writes no markup; a content section is its owner's alone
<lib/box.markup.bx:4>
      (content
      ^^^^^^^

Exception: BoxParams is a facet, so it takes no type parameters
<lib/box.params.bx:3>
    (type + (of BoxParams T) (facet-of Box))
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Exception: BoxStatic is a facet, so it has no static functions
<lib/box.static.bx:4>
      (fn-expr + make Int :static
      ^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A facet's member may not take a name its class answers, nor a public name another facet of the class declares

```brix compile-error
(import ./lib/box)
(blank)

(main
  (call Box)
)
```

```brix file=lib/box.bx
(import ./box.depth)
(import ./box.focus)
(import ./box.scroll)
(import ./box.width)

(type + Base
  (prop + depth _ (= 0))
)

(type + Box (is Base)
  (prop _ width _ (= 10))
)
```

```brix file=lib/box.width.bx
(import ./box)

(type + BoxWidth (facet-of Box)
  (prop _ width _ (= 1))
)
```

```brix file=lib/box.depth.bx
(import ./box)

(type + BoxDepth (facet-of Box)
  (fn-expr + depth Int
    2
  )
)
```

```brix file=lib/box.scroll.bx
(import ./box)

(type + BoxScroll (facet-of Box)
  (prop + shared _ (= 3))
)
```

```brix file=lib/box.focus.bx
(import ./box)

(type + BoxFocus (facet-of Box)
  (prop + shared _ (= 4))
)
```

```output
Exception: depth is already a member of Box, so facet BoxDepth can't declare it
<lib/box.depth.bx:4>
      (fn-expr + depth Int
      ^^^^^^^^^^^^^^^^^^^

Exception: Facets BoxFocus and BoxScroll of Box both declare shared; a facet's members that are not private are its class's, so a name is one facet's
<lib/box.focus.bx:4>
      (prop + shared _ (= 4))
      ^^^^^^^^^^^^^^^^^^^^^^^

Exception: Facets BoxScroll and BoxFocus of Box both declare shared; a facet's members that are not private are its class's, so a name is one facet's
<lib/box.scroll.bx:4>
      (prop + shared _ (= 3))
      ^^^^^^^^^^^^^^^^^^^^^^^

Exception: width is already a member of Box, so facet BoxWidth can't declare it
<lib/box.width.bx:4>
      (prop _ width _ (= 1))
      ^^^^^^^^^^^^^^^^^^^^^^
```

### A class below one with facets can't declare a name a facet declares but to override a method, getter or setter of the facet with one of its kind or to give a property it stores a new value, and a constructor's argument can't give a facet's private property its value in the facet's file

```brix compile-error
(import ./lib/box)
(import ./lib/pane)
(import ./lib/panel)
(blank)

(main
  (print (. (call (. (call Box) tab)) width))
)
```

```brix file=lib/panel.bx
(import ./box)

(type + Panel (is Box)
  (prop + offset _ (=> 9))
)
```

```brix file=lib/pane.bx
(import ./box)

(type + Pane (is Box)
  (prop + nudge _ (=> 2))
)
```

```brix file=lib/box.bx
(import ./box.scroll)

(type + Box
  (prop + width _ (= 10))
)
```

```brix file=lib/box.scroll.bx
(import ./box)

(type + BoxScroll (facet-of Box)
  (prop + offset _ (= 3))
  (prop _ hidden _ (= 2))
  (fn-expr + nudge Int
    1
  )
  (fn-expr + tab Box
    (call Tab 1)
  )
)

(type + Tab (is Box)
  (fn-do + This _
    (param hidden Int)
  )
)
```

```output
Exception: hidden is private to facet BoxScroll, so a constructor's argument can't give it a value
<lib/box.scroll.bx:16>
        (param hidden Int)
        ^^^^^^^^^^^^^

Exception: Pane can't declare nudge, since facet BoxScroll of Box declares it
<lib/pane.bx:4>
      (prop + nudge _ (=> 2))
      ^^^^^^^^^^^^^^^^^^^^^^^

Exception: Panel can't declare offset, since facet BoxScroll of Box declares it
<lib/panel.bx:4>
      (prop + offset _ (=> 9))
      ^^^^^^^^^^^^^^^^^^^^^^^^
```

### A facet declares lazy and computed properties, getters and setters, operators and conversions, and reaches the private values, types, functions and constructors of its class's module

```brix
(import ./lib/box)
(blank)

(main
  (let box (call Box))
  (print (. box cached))
  (print (. box twice))
  (= (. box label) "abcde")
  (print (. box total))
  (print (. box label))
  (print (+ box 10))
  (print (as box String))
  (print (call (. box reached)))
  (print (call (. (call (. box copy)) size-text)))
)
```

```brix file=lib/box.bx
(import ./box.extra)
(blank)
(prop _ secret _ (= 7))

(type + Box
  (prop _ size _ (= 4))
  (fn-do + This _)
  (fn-do _ This _
    (param start Int)
    (= size start)
  )
  (fn-expr + size-text String
    (str "size " size)
  )
)

(type _ Hidden (is Tuple)
  (prop + n Int)
)

(fn-expr _ helper Int
  (param n Int)
  (* n 10)
)
```

```brix file=lib/box.extra.bx
(import ./box)

(type + BoxExtra (facet-of Box)
  (prop + total _ (= 1))
  (prop + cached _ (:= (* size 3)))
  (prop + twice _ (=> (* total 2)))
  (fn-expr + label String :get
    (str "label " total)
  )
  (fn-do + label _ :set (param value String)
    (= total (. value length))
  )
  (fn-expr + (infix this + (param more Int)) _
    (+ total more)
  )
  (fn-to-expr + String
    (str "box of " size)
  )
  (fn-expr + reached Int
    (call helper (. (call Hidden secret) n))
  )
  (fn-expr + copy Box
    (call Box (+ size 1))
  )
)
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

```brix
(import ./lib/box)
(blank)

(main
  (let box (call Box))
  (print (call (. " " join) order))
)
```

```brix file=lib/box.bx
(import ./box.zeta)
(import ./box.alpha)
(blank)
(prop + order _ (= (call (of List String))))

(type + Box
  (prop + own _ (= (call note "own")))
)

(fn-do + note Int
  (param name String)
  (+= order name)
  (return 0)
)
```

```brix file=lib/box.zeta.bx
(import ./box)

(type + BoxZeta (facet-of Box)
  (prop + z1 _ (= (call note "zeta 1")))
  (prop + z2 _ (= (call note "zeta 2")))
)
```

```brix file=lib/box.alpha.bx
(import ./box)

(type + BoxAlpha (facet-of Box)
  (prop + a _ (= (call note "alpha")))
)
```

```output
own alpha zeta 1 zeta 2
```

### A facet may be declared in its class's own module, whose code reaches every facet of the class, as code outside the facets' modules does

```brix
(import ./lib/box)
(blank)

(main
  (print (call (. (call Box) both)))
)
```

```brix file=lib/box.bx
(import ./box.focus)

(type + Box
  (fn-expr + both String
    (str offset " " focused)
  )
)

(type + BoxScroll (facet-of Box)
  (prop + offset _ (= 3))
)
```

```brix file=lib/box.focus.bx
(import ./box)

(type + BoxFocus (facet-of Box)
  (prop + focused _ (= true))
)
```

```output
3 true
```

### A facet of a class below one with facets can't take the name of a public member of a facet of a class above, which its class already has

```brix compile-error
(import ./lib/window)
(blank)

(main
  (print (. (call Window) offset))
)
```

```brix file=lib/box.bx
(import ./box.scroll)

(type + Box)
```

```brix file=lib/box.scroll.bx
(import ./box)

(type + BoxScroll (facet-of Box)
  (prop + offset _ (= 3))
)
```

```brix file=lib/window.bx
(import ./box)
(import ./window.pane)

(type + Window (is Box))
```

```brix file=lib/window.pane.bx
(import ./window)

(type + WindowPane (facet-of Window)
  (prop + offset _ (= 8))
)
```

```output
Exception: offset is already a member of Window, so facet WindowPane can't declare it
<lib/window.pane.bx:4>
      (prop + offset _ (= 8))
      ^^^^^^^^^^^^^^^^^^^^^^^
```

### A facet's name is a type. A value of its class or of a class below becomes one unasked, as does a facet's value the facets it is part of, and has the members of its facet and of those, which a class below that declares one again answers for. It becomes its class again only when asked, is tested for a class below as its class is, and its type information is its class's

```brix
(import ./lib/box)
(blank)

(main
  (print (call describe (call Panel)))
  (print (call describe (call Box)))
)
```

```brix file=lib/box.bx
(import ./box.scroll)
(import ./box.scroll.bar)

(type + Box
  (prop + width _ (= 10))
)

(type + Panel (is Box)
  (prop + height _ (= 4))
  (fn-do + scroll-to _
    (param point Int)
    (= offset (* point 100))
  )
)

(fn-expr + height-of Int
  (param scroll BoxScroll)
  (match scroll
    (case (: panel Panel)
      (. panel height)
    )
    (default
      0
    )
  )
)

(fn-do + describe String
  (param box Box)
  (let bar BoxScrollBar box)
  (let scroll BoxScroll bar)
  (call (. scroll scroll-to) 2)
  (= (. bar shown) true)
  (let maybe (? BoxScroll) (if-expr (. bar shown) scroll null))
  (let either (if-expr (> (. scroll offset) 100) box scroll))
  (let all (list BoxScroll) (list scroll box))
  (let owner (as scroll Box))
  (let shown (str (. scroll offset) " " (. scroll doubled) " " (. scroll area) " " (. bar offset) " " (. bar shown) " " (. (!! maybe) offset)))
  (return (str shown " " (. either offset) " " (. all length) " " (. owner width) " " (is scroll Panel) " " (call height-of scroll) " " (type-of scroll)))
)
```

```brix file=lib/box.scroll.bx
(import ./box)

(type + BoxScroll (facet-of Box)
  (prop + offset _ (= 3))
  (prop + doubled _ (=> (* offset 2)))
  (prop + area _ (:= (* width 2)))
  (fn-do + scroll-to _
    (param point Int)
    (= offset (+ point width))
  )
)
```

```brix file=lib/box.scroll.bar.bx
(import ./box)
(import ./box.scroll)

(type + BoxScrollBar (facet-of BoxScroll)
  (prop + shown _ (= false))
)
```

```output
200 400 20 200 true 200 200 2 10 true 4 [TypeInfo Box]
12 24 20 12 true 12 12 2 10 false 0 [TypeInfo Box]
```

### Code outside a class's file family names the class's facets by importing the class's module, which brings their names and nothing else of their modules

```brix
(import ./lib/box)
(blank)

(main
  (print (call offset-of (call Box)))
)

(fn-expr + offset-of Int
  (param scroll BoxScroll)
  (. scroll offset)
)
```

```brix file=lib/box.bx
(import ./box.scroll)

(type + Box)
```

```brix file=lib/box.scroll.bx
(import ./box)

(type + BoxScroll (facet-of Box)
  (prop + offset _ (= 3))
)
```

```output
3
```

### A facet's value becomes its class only when asked and has no members of its class or of facets it is not part of, a value is not tested for a facet, a facet has no constructor, a stored property of a facet's type needs a value, a facet's value reaches no member of a facet the module's facet does not see, and code that does not import the class's module can't name its facets

```brix compile-error
(import ./lib/box)
(import ./lib/other)
(blank)

(main
  (call Holder)
  (call widen (call Box))
  (call owned (call Box))
  (call sibling (call Box))
  (call tested (call Box))
  (call matched (call Box))
  (call made)
  (call (. (call Box) peek))
  (call other-offset)
)
```

```brix file=lib/box.bx
(import ./box.focus)
(import ./box.scroll)

(type + Box
  (prop + width _ (= 10))
)

(type + Holder
  (prop + held BoxScroll)
)

(fn-expr + scrolled BoxScroll
  (param box Box)
  box
)

(fn-expr + widen Box
  (param scroll BoxScroll)
  scroll
)

(fn-expr + owned Int
  (param scroll BoxScroll)
  (. scroll width)
)

(fn-expr + sibling Bool
  (param scroll BoxScroll)
  (. scroll focused)
)

(fn-expr + tested Bool
  (param box Box)
  (is box BoxScroll)
)

(fn-expr + matched Int
  (param box Box)
  (match box
    (case BoxScroll
      1
    )
    (default
      2
    )
  )
)

(fn-expr + made BoxScroll
  (call BoxScroll)
)
```

```brix file=lib/box.focus.bx
(import ./box)

(type + BoxFocus (facet-of Box)
  (prop + focused _ (= false))
  (fn-expr + peek Int
    (. (call scrolled this) offset)
  )
)
```

```brix file=lib/box.scroll.bx
(import ./box)

(type + BoxScroll (facet-of Box)
  (prop + offset _ (= 3))
)
```

```brix file=lib/other.bx
(fn-do + other-offset Int
  (let scroll (? BoxScroll) null)
  (return 0)
)
```

```output
Exception: held is a BoxScroll, a facet of Box, which has no constructor to give it its first value; give it a value, or make it optional
<lib/box.bx:9>
      (prop + held BoxScroll)
      ^^^^^^^^^^^^^^^^^^^^^^^

Exception: Expected Box here, but this is BoxScroll, which becomes Box only when asked; write "(as scroll Box)"
<lib/box.bx:19>
      scroll
      ^^^^^^

Exception: BoxScroll has no property "width"; a facet's value has only its facet's members and those of the facets it is part of. Write "(as … Box)" to reach the rest of Box
<lib/box.bx:24>
      (. scroll width)
      ^^^^^^^^^^^^^^^^

Exception: BoxScroll has no property "focused"; a facet's value has only its facet's members and those of the facets it is part of. Write "(as … Box)" to reach the rest of Box
<lib/box.bx:29>
      (. scroll focused)
      ^^^^^^^^^^^^^^^^^^

Exception: BoxScroll is a facet, which every Box has, so no value is tested for it
<lib/box.bx:34>
      (is box BoxScroll)
              ^^^^^^^^^

Exception: BoxScroll is a facet, which every Box has, so no value is tested for it
<lib/box.bx:40>
        (case BoxScroll
              ^^^^^^^^^

Exception: BoxScroll is a facet of Box, so it has no constructor; a Box is a BoxScroll
<lib/box.bx:50>
      (call BoxScroll)
            ^^^^^^^^^

Exception: BoxScroll has no property "offset" here; facet BoxScroll declares it, and only the facets a facet sees are in reach. Write "sees BoxScroll" under the facet that uses it
<lib/box.focus.bx:6>
        (. (call scrolled this) offset)
        ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Exception: Type "BoxScroll" not found; BoxScroll is a facet of Box, whose name comes with Box's module. Import that module to name it
<lib/other.bx:2>
      (let scroll (? BoxScroll) null)
                     ^^^^^^^^^
```

### A facet that names an interface makes its class and the classes below implement it through the facet's members, which a class below may override, and a class that names an interface may answer it with its facets' members

```brix
(import ./lib/box)
(import ./lib/kinds)
(blank)

(main
  (let box (call Box))
  (let scroll Scrollable box)
  (call (. scroll scroll-by) 4)
  (print (. scroll position))
  (let panel Scrollable (call Panel))
  (call (. panel scroll-by) 1)
  (print (. panel position))
  (let named Named box)
  (print (. named label))
  (let part BoxScroll box)
  (let again Scrollable (as part Box))
  (print (. again position))
)
```

```brix file=lib/kinds.bx
(type + Scrollable (is Interface)
  (fn-expr + position Int :get)
  (fn-do + scroll-by _
    (param amount Int)
  )
)

(type + Named (is Interface)
  (fn-expr + label String :get)
)
```

```brix file=lib/box.bx
(import ./box.focus)
(import ./box.scroll)
(import ./kinds)

(type + Box
  (implements Named)
)

(type + Panel (is Box)
  (fn-do + scroll-by _
    (param amount Int)
    (= offset (* amount 10))
  )
)
```

```brix file=lib/box.focus.bx
(import ./box)

(type + BoxFocus (facet-of Box)
  (fn-expr + label String :get
    "box"
  )
)
```

```brix file=lib/box.scroll.bx
(import ./box)
(import ./kinds)

(type + BoxScroll (facet-of Box)
  (implements Scrollable)
  (prop + offset _ (= 3))
  (fn-expr + position Int :get
    offset
  )
  (fn-do + scroll-by _
    (param amount Int)
    (+= offset amount)
  )
)
```

```output
7
10
box
7
```

### A facet names only interfaces, answers each with its members, and its value becomes an interface only as its class

```brix compile-error
(import ./lib/box)
(import ./lib/kinds)
(blank)

(main
  (let part BoxScroll (call Box))
  (let scroll Scrollable part)
  (let whole Scrollable (call Box))
)
```

```brix file=lib/kinds.bx
(type + Scrollable (is Interface)
  (fn-expr + position Int :get)
  (fn-do + scroll-by _
    (param amount Int)
  )
)

(type + Base)
```

```brix file=lib/box.bx
(import ./box.scroll)
(import ./kinds)

(type + Box)
```

```brix file=lib/box.scroll.bx
(import ./box)
(import ./kinds)

(type + BoxScroll (facet-of Box)
  (implements Scrollable)
  (implements Base)
  (fn-do + scroll-by _
    (param amount Int)
  )
)
```

```output
Exception: "scroll" is declared Scrollable, but its value is BoxScroll, a facet's value, which becomes only the facets it is part of unasked; write "(as … Box)" to convert its class
<no file:8>
      (let scroll Scrollable part)
           ^^^^^^^^^^^^^^^^^

Exception: Box does not implement "position", which Scrollable declares without a body; declare "position" in BoxScroll
<lib/box.bx:4>
    (type + Box)
    ^^^^^^^^^^^^
Interface:
<lib/kinds.bx:2>
      (fn-expr + position Int :get)

Exception: BoxScroll cannot implement Base, which is not an interface; a facet is part of its class and is built on no type
<lib/box.scroll.bx:6>
      (implements Base)
                  ^^^^
```
