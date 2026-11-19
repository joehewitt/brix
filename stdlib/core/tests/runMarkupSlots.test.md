# Markup slots

### Demonstrates named slots, default slot routing, declaration order, and slot overlay updates.

```brix
(main
  (let title "Settings")
  (blank)
  (<> Panel (id: panel)
    (<> MarkText (slot: header) (value: #title))
    (<> MarkText (value: "body one"))
    (<> MarkText (value: "body two"))
  )
  (blank)
  (print (call text-at panel 0))
  (print (call text-at panel 1))
  (print (call text-at panel 2))
  (print (call text-at panel 3))
  (print (call count-children panel))
  (= title "Account")
  (= (. panel footer-label) "Done")
  (print (call text-at panel 0))
  (print (call text-at panel 1))
  (blank)
  (<> OrderedPanel (id: ordered)
    (<> MarkText (slot: beta) (value: "B"))
    (<> MarkText (slot: alpha) (value: "A"))
  )
  (print (call text-at ordered 0))
  (print (call text-at ordered 1))
  (blank)
  (<> OverlayPanel (id: overlay)
    (<> MarkText (slot: header) (value: "caller"))
  )
  (print (call text-at overlay 0))
  (= (. overlay header-title) "Updated")
  (print (call text-at overlay 0))
)

(fn-expr + text-at String
  (param node MarkNode)
  (param index Index)
  (. (as (!! (call (. node child-at) index)) MarkText) text)
)

(fn-do + count-children _
  (param node MarkNode)
  (let count 0)
  (for (in child (. node children))
    (+= count 1)
  )
  (return count)
)

(type + MarkNode
  (implements (of MarkupParent MarkNode))
  (prop + parent (? MarkNode))
  (prop + first-child (? MarkNode))
  (prop + last-child (? MarkNode))
  (prop + previous (? MarkNode))
  (prop + next (? MarkNode))
  (fn-do + children _ :get
    (let maybe-child first-child)
    (if maybe-child
      (let child maybe-child)
      (while true
        (yield child)
        (if (not (. child next))
          (break)
        )
        (= child (. child next))
      )
    )
  )
  (fn-do + child-at (? MarkNode)
    (param index Index)
    (let cursor Index 0)
    (for (in child children)
      (if (== cursor index)
        (return child)
      )
      (+= cursor 1)
    )
  )
  (fn-expr + markup-content MarkNode
    (param child MarkNode)
    child
  )
  (fn-do + markup-attach-child _
    (param child MarkNode)
    (param anchor (? MarkNode))
    (= (. child parent) this)
    (if (not first-child)
      (= first-child (= last-child child))
      (else
        (= (?. last-child next) child)
        (= (. child previous) last-child)
        (= last-child child)
      )
    )
  )
)

(type + MarkText (is MarkNode)
  (prop + text String)
  (fn-do + This _
    (param text String)
  )
)

(type + Panel (is MarkNode)
  (prop + footer-label _ (= "Foot"))
  (content
    (+<> MarkText (id: header))
    (+<> MarkText (id: footer)
      (<> MarkText (value: #footer-label))
    )
  )
)

(type + OrderedPanel (is MarkNode)
  (content
    (+<> MarkText (id: alpha))
    (+<> MarkText (id: beta))
  )
)

(type + OverlayPanel (is MarkNode)
  (prop + header-title _ (= "Overlay"))
  (content
    (+<> MarkText (id: header)
      (= text #header-title)
    )
  )
)
```

```output
Settings
Foot
body one
body two
4
Account
Done
A
B
Overlay
Updated
```

### A named object supplied to a slot remains the caller's binding when the host absorbs it.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> AbsorbingRoot (id: root))
  (let absorbed (!! (. root host absorbed)))
  (print (. absorbed name))
  (print (== (as absorbed Pointer) (as (. root named) Pointer)))
)

(type + AbsorbedSlotValue
  (prop + name _ (= ""))
)

(type + AbsorbingSlotHost (is Box)
  (prop + absorbed (? AbsorbedSlotValue))
  (content
    (+<> AbsorbedSlotValue)
  )
  (fn-do + markup-absorb _
    (param content AbsorbedSlotValue)
    (= absorbed content)
  )
)

(type + AbsorbingRoot (is Box)
  (prop + host AbsorbingSlotHost)
  (prop + named AbsorbedSlotValue)
  (content
    (<> This
      (<> AbsorbingSlotHost (id: host)
        (<> AbsorbedSlotValue (id: named)
          (= name "slot")
        )
      )
    )
  )
)
```

```output
slot
true
```

### Demonstrates repeated row slots that receive a component-owned item value.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> RowHost (id: host))
  (+= (. host items) (call RowData "a"))
  (+= (. host items) (call RowData "b"))
  (call (. host markup-batch-queue checkpoint))
  (let panel (. host panel))
  (print (call row-texts panel))
  (+= (. host items) (call RowData "c"))
  (call (. host markup-batch-queue checkpoint))
  (print (call row-texts panel))
)

(type + RowData
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(type + (of RowPanel Item) (is Box)
  (implements MarkupRootElement)
  (prop + items (list Item) (= (call (of List Item))))
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (content
    (<> This
      (for (in item #items)
        (<> Box
          (+<> Box (id: row) (param: item Item))
        )
      )
    )
  )
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (let ignored (. flush applied))
  )
)

(fn-do + row-texts String
  (param panel Box)
  (let parts (list String) (call (of List String)))
  (let region (. panel %first-child))
  (if region
    (for (in child (. region children))
      (+= parts (. (as (!! (. child %first-child)) TextBox) text))
    )
  )
  (return (call (. "," join) parts))
)

(type + RowHost (is Box)
  (implements MarkupRootElement)
  (prop + items (list RowData) (= (call (of List RowData))))
  (prop + panel (of RowPanel RowData))
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (content
    (<> This
      (<> (of RowPanel RowData) (id: panel)
        (= items #items)
        (<> TextBox (slot: row) (param: item) (value: (. item text)))
      )
    )
  )
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
    (let ignored (. flush applied))
  )
)
```

```output
a,b
a,b,c
```

### Demonstrates nested repeated row slot forwarding through another component.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let left (call (of List RowData) (list (call RowData "a") (call RowData "b"))))
  (let right (call (of List RowData) (list (call RowData "x"))))
  (let columns (list (list RowData)) (call (of List (of List RowData))))
  (+= columns left)
  (+= columns right)
  (blank)
  (<> (of GridPanel RowData) (id: panel)
    (= columns #columns)
    (<> TextBox (slot: row) (param: item) (value: (. item text)))
  )
  (blank)
  (print (call grid-texts panel))
  (+= left (call RowData "c"))
  (print (call grid-texts panel))
  (+= columns (call (of List RowData) (list (call RowData "z"))))
  (print (call grid-texts panel))
)

(type + RowData
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(type + (of RowPanel Item) (is Box)
  (prop + items (list Item))
  (content
    (<> This
      (for (in item #items)
        (+<> Box (id: row) (param: item Item))
      )
    )
  )
)

(type + (of GridPanel Item) (is Box)
  (prop + columns (list (list Item)))
  (content
    (<> This
      (for (in items #columns)
        (<> (of RowPanel Item)
          (= items items)
          (<+ row)
        )
      )
    )
  )
)

(fn-do + grid-texts String
  (param panel Box)
  (let column-texts (list String) (call (of List String)))
  (let column-region (. panel %first-child))
  (if column-region
    (for (in column (. column-region children))
      (let row-texts (list String) (call (of List String)))
      (let row-region (. column %first-child))
      (if row-region
        (for (in child (. row-region children))
          (+= row-texts (. (as child TextBox) text))
        )
      )
      (+= column-texts (call (. "," join) row-texts))
    )
  )
  (return (call (. "|" join) column-texts))
)
```

```output
a,b|x
a,b,c|x
a,b,c|x|z
```

### Demonstrates inherited repeated row slots see markup-provided properties during initial content construction.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let items (call (of List RowData) (list (call RowData "a") (call RowData "b"))))
  (<> (of DerivedRowPanel RowData) (id: panel)
    (= items #items)
    (<> TextBox (slot: row) (param: item) (value: (. item text)))
  )
  (blank)
  (print (call row-texts panel))
)

(type + RowData
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(type + (of RowPanel Item) (is Box)
  (prop + items (list Item))
  (content
    (<> This
      (for (in item #items)
        (+<> Box (id: row) (param: item Item))
      )
    )
  )
)

(type + (of DerivedRowPanel Item) (is (of RowPanel Item)))

(fn-do + row-texts String
  (param panel Box)
  (let parts (list String) (call (of List String)))
  (let region (. panel %first-child))
  (if region
    (for (in child (. region children))
      (+= parts (. (as child TextBox) text))
    )
  )
  (return (call (. "," join) parts))
)
```

```output
a,b
```

### Demonstrates nested repeated row forwarding into inherited base content.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let left (call (of List RowData) (list (call RowData "a") (call RowData "b"))))
  (let right (call (of List RowData) (list (call RowData "x"))))
  (let columns (list (list RowData)) (call (of List (of List RowData))))
  (+= columns left)
  (+= columns right)
  (blank)
  (<> (of DerivedGridPanel RowData) (id: panel)
    (= columns #columns)
    (<> TextBox (slot: row) (param: item) (value: (. item text)))
  )
  (blank)
  (print (call grid-texts panel))
)

(type + RowData
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(type + (of RowPanel Item) (is Box)
  (prop + items (list Item))
  (content
    (<> This
      (for (in item #items)
        (+<> Box (id: row) (param: item Item))
      )
    )
  )
)

(type + (of DerivedRowPanel Item) (is (of RowPanel Item)))

(type + (of DerivedGridPanel Item) (is Box)
  (prop + columns (list (list Item)))
  (content
    (<> This
      (for (in items #columns)
        (<> (of DerivedRowPanel Item)
          (= items items)
          (<+ row)
        )
      )
    )
  )
)

(fn-do + grid-texts String
  (param panel Box)
  (let column-texts (list String) (call (of List String)))
  (let column-region (. panel %first-child))
  (if column-region
    (for (in column (. column-region children))
      (let row-texts (list String) (call (of List String)))
      (let row-region (. column %first-child))
      (if row-region
        (for (in child (. row-region children))
          (+= row-texts (. (as child TextBox) text))
        )
      )
      (+= column-texts (call (. "," join) row-texts))
    )
  )
  (return (call (. "|" join) column-texts))
)
```

```output
a,b|x
```

### Demonstrates repeated row slot content can pass the item as a property to an explicit row constructor.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let items (call (of List RowData) (list (call RowData "a") (call RowData "b"))))
  (<> (of RowPanel RowData) (id: panel)
    (= items #items)
    (<> ExplicitRow (slot: row) (param: item)
      (= item item)
    )
  )
  (blank)
  (print (call row-texts panel))
)

(type + RowData
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(type + (of RowPanel Item) (is Box)
  (prop + items (list Item))
  (content
    (<> This
      (for (in item #items)
        (+<> Box (id: row) (param: item Item))
      )
    )
  )
)

(type + ExplicitRow (is TextBox)
  (prop + item RowData)
  (fn-do + This _
    (param item _)
    (= text (. item text))
  )
)

(fn-do + row-texts String
  (param panel Box)
  (let parts (list String) (call (of List String)))
  (let region (. panel %first-child))
  (if region
    (for (in child (. region children))
      (+= parts (. (as child TextBox) text))
    )
  )
  (return (call (. "," join) parts))
)
```

```output
a,b
```

### Demonstrates that a slot outlet can be referenced locally inside the component body.

```brix
(main
  (<> ReferencePanel (id: panel)
    (<> MarkText (slot: header) (value: "slot"))
  )
  (blank)
  (print (call text-at panel 0))
  (print (call text-at panel 1))
)

(fn-expr + text-at String
  (param node MarkNode)
  (param index Index)
  (. (as (!! (call (. node child-at) index)) MarkText) text)
)

(type + MarkNode
  (implements (of MarkupParent MarkNode))
  (prop + first-child (? MarkNode))
  (prop + last-child (? MarkNode))
  (prop + next (? MarkNode))
  (fn-do + child-at (? MarkNode)
    (param index Index)
    (let cursor Index 0)
    (let child first-child)
    (while child
      (if (== cursor index)
        (return child)
      )
      (+= cursor 1)
      (= child (. child next))
    )
  )
  (fn-expr + markup-content MarkNode
    (param child MarkNode)
    child
  )
  (fn-do + markup-attach-child _
    (param child MarkNode)
    (param anchor (? MarkNode))
    (if (not first-child)
      (= first-child (= last-child child))
      (else
        (= (?. last-child next) child)
        (= last-child child)
      )
    )
  )
)

(type + MarkText (is MarkNode)
  (prop + text String)
  (fn-do + This _
    (param text String)
  )
)

(type + ReferencePanel (is MarkNode)
  (content
    (+<> MarkText (id: header))
    (<> MarkText (value: (. header text)))
  )
)
```

```output
slot
slot
```

### Demonstrates that a slot outlet that sets attributes on its content can be referenced locally inside the component body.

```brix
(main
  (<> ReferencePanel (id: panel)
    (<> MarkText (slot: header) (value: "slot"))
  )
  (blank)
  (print (call text-at panel 0))
  (print (call text-at panel 1))
)

(fn-expr + text-at String
  (param node MarkNode)
  (param index Index)
  (. (as (!! (call (. node child-at) index)) MarkText) text)
)

(type + MarkNode
  (implements (of MarkupParent MarkNode))
  (prop + first-child (? MarkNode))
  (prop + last-child (? MarkNode))
  (prop + next (? MarkNode))
  (fn-do + child-at (? MarkNode)
    (param index Index)
    (let cursor Index 0)
    (let child first-child)
    (while child
      (if (== cursor index)
        (return child)
      )
      (+= cursor 1)
      (= child (. child next))
    )
  )
  (fn-expr + markup-content MarkNode
    (param child MarkNode)
    child
  )
  (fn-do + markup-attach-child _
    (param child MarkNode)
    (param anchor (? MarkNode))
    (if (not first-child)
      (= first-child (= last-child child))
      (else
        (= (?. last-child next) child)
        (= last-child child)
      )
    )
  )
)

(type + MarkText (is MarkNode)
  (prop + text String)
  (fn-do + This _
    (param text String)
  )
)

(type + ReferencePanel (is MarkNode)
  (content
    (+<> MarkText (id: header)
      (= text "overlaid")
    )
    (<> MarkText (value: (. header text)))
  )
)
```

```output
overlaid
overlaid
```

### Demonstrates local ids on slotted nodes and live property overlays on slot fallbacks.

```brix
(main
  (let title "Inline")
  (blank)
  (<> Panel (id: panel)
    (<> MarkText (slot: header) (value: #title))
    (<> MarkText (id: footer-text) (slot: footer) (value: "Tail"))
    (<> MarkText (value: "body"))
  )
  (blank)
  (print (call count-children panel))
  (print (call text-at panel 0))
  (print (call text-at panel 1))
  (print (call text-at panel 2))
  (print (. footer-text text))
  (= title "Changed")
  (= (. panel footer-label) "Overlay Tail")
  (print (call text-at panel 0))
  (print (call text-at panel 1))
)

(fn-expr + text-at String
  (param node MarkNode)
  (param index Index)
  (. (as (!! (call (. node child-at) index)) MarkText) text)
)

(fn-do + count-children _
  (param node MarkNode)
  (let count 0)
  (for (in child (. node children))
    (+= count 1)
  )
  (return count)
)

(type + MarkNode
  (implements (of MarkupParent MarkNode))
  (prop + parent (? MarkNode))
  (prop + first-child (? MarkNode))
  (prop + last-child (? MarkNode))
  (prop + previous (? MarkNode))
  (prop + next (? MarkNode))
  (fn-do + children _ :get
    (let maybe-child first-child)
    (if maybe-child
      (let child maybe-child)
      (while true
        (yield child)
        (if (not (. child next))
          (break)
        )
        (= child (. child next))
      )
    )
  )
  (fn-do + child-at (? MarkNode)
    (param index Index)
    (let cursor Index 0)
    (for (in child children)
      (if (== cursor index)
        (return child)
      )
      (+= cursor 1)
    )
  )
  (fn-expr + markup-content MarkNode
    (param child MarkNode)
    child
  )
  (fn-do + markup-attach-child _
    (param child MarkNode)
    (param anchor (? MarkNode))
    (= (. child parent) this)
    (if (not first-child)
      (= first-child (= last-child child))
      (else
        (= (?. last-child next) child)
        (= (. child previous) last-child)
        (= last-child child)
      )
    )
  )
)

(type + MarkText (is MarkNode)
  (prop + text String)
  (fn-do + This _
    (param text String)
  )
)

(type + Panel (is MarkNode)
  (prop + footer-label _ (= "Foot"))
  (content
    (+<> MarkText (id: header))
    (+<> MarkText (id: footer)
      (= text #footer-label)
    )
  )
)
```

```output
3
Inline
Foot
body
Foot
Changed
Overlay Tail
```

### Demonstrates that default slot content can be a single host node containing nested children.

```brix
(main
  (<> DefaultPanel (id: panel)
    (<> MarkGroup
      (<> MarkText (value: "one"))
      (<> MarkText (value: "two"))
    )
  )
  (blank)
  (print (call count-children panel))
  (print (call child-texts (!! (. panel %first-child))))
)

(fn-do + child-texts String
  (param node MarkNode)
  (let parts (call (of List String)))
  (for (in child (. node children))
    (+= parts (. (as child MarkText) text))
  )
  (return (call (. "," join) parts))
)

(fn-do + count-children _
  (param node MarkNode)
  (let count 0)
  (for (in child (. node children))
    (+= count 1)
  )
  (return count)
)

(type + MarkNode
  (implements (of MarkupParent MarkNode))
  (prop + parent (? MarkNode))
  (prop + first-child (? MarkNode))
  (prop + last-child (? MarkNode))
  (prop + previous (? MarkNode))
  (prop + next (? MarkNode))
  (fn-do + children _ :get
    (let maybe-child first-child)
    (if maybe-child
      (let child maybe-child)
      (while true
        (yield child)
        (if (not (. child next))
          (break)
        )
        (= child (. child next))
      )
    )
  )
  (fn-expr + markup-content MarkNode
    (param child MarkNode)
    child
  )
  (fn-do + markup-attach-child _
    (param child MarkNode)
    (param anchor (? MarkNode))
    (= (. child parent) this)
    (if (not first-child)
      (= first-child (= last-child child))
      (else
        (= (?. last-child next) child)
        (= (. child previous) last-child)
        (= last-child child)
      )
    )
  )
)

(type + MarkGroup (is MarkNode))

(type + MarkText (is MarkNode)
  (prop + text String)
  (fn-do + This _
    (param text String)
  )
)

(type + DefaultPanel (is MarkNode)
  (content
    (+<> MarkNode)
  )
)
```

```output
1
one,two
```

### Demonstrates inherited default slot routing and caller override precedence for the default slot.

```brix
(main
  (<> DefaultSubPanel (id: panel))
  (blank)
  (print (call count-children panel))
  (print (call text-at panel 0))
  (print (call text-at panel 1))
  (print (call text-at panel 2))
  (blank)
  (<> DefaultSubPanel (id: caller)
    (<> MarkText (value: "caller body"))
  )
  (print (call text-at caller 0))
  (print (call text-at caller 1))
  (print (call text-at caller 2))
)

(fn-expr + text-at String
  (param node MarkNode)
  (param index Index)
  (. (as (!! (call (. node child-at) index)) MarkText) text)
)

(fn-do + count-children _
  (param node MarkNode)
  (let count 0)
  (for (in child (. node children))
    (+= count 1)
  )
  (return count)
)

(type + MarkNode
  (implements (of MarkupParent MarkNode))
  (prop + parent (? MarkNode))
  (prop + first-child (? MarkNode))
  (prop + last-child (? MarkNode))
  (prop + previous (? MarkNode))
  (prop + next (? MarkNode))
  (fn-do + children _ :get
    (let maybe-child first-child)
    (if maybe-child
      (let child maybe-child)
      (while true
        (yield child)
        (if (not (. child next))
          (break)
        )
        (= child (. child next))
      )
    )
  )
  (fn-do + child-at (? MarkNode)
    (param index Index)
    (let cursor Index 0)
    (for (in child children)
      (if (== cursor index)
        (return child)
      )
      (+= cursor 1)
    )
  )
  (fn-expr + markup-content MarkNode
    (param child MarkNode)
    child
  )
  (fn-do + markup-attach-child _
    (param child MarkNode)
    (param anchor (? MarkNode))
    (= (. child parent) this)
    (if (not first-child)
      (= first-child (= last-child child))
      (else
        (= (?. last-child next) child)
        (= (. child previous) last-child)
        (= last-child child)
      )
    )
  )
)

(type + MarkText (is MarkNode)
  (prop + text String)
  (fn-do + This _
    (param text String)
  )
)

(type + BasePanel (is MarkNode)
  (content
    (+<> MarkText (id: header)
      (<> MarkText (value: "base header"))
    )
    (+<> MarkNode
      (<> MarkText (value: "base body"))
    )
    (blank)
    (+<> MarkText (id: footer)
      (<> MarkText (value: "base footer"))
    )
  )
)

(type + DefaultSubPanel (is BasePanel)
  (content
    (<> MarkText (value: "sub body"))
  )
)
```

```output
3
base header
sub body
base footer
base header
caller body
base footer
```

### The lines of a class's markup that give the default slot of the class above make each element where it is written, and an element written with a name gives the class's property of the name its object.

```brix
(main
  (<> Sub (id: sub))
  (print (. sub spare label))
)

(type + MarkNode
  (implements (of MarkupParent MarkNode))
  (prop + parent (? MarkNode))
  (prop + first-child (? MarkNode))
  (prop + last-child (? MarkNode))
  (prop + previous (? MarkNode))
  (prop + next (? MarkNode))
  (fn-expr + markup-content MarkNode
    (param child MarkNode)
    child
  )
  (fn-do + markup-attach-child _
    (param child MarkNode)
    (param anchor (? MarkNode))
    (= (. child parent) this)
    (if (not first-child)
      (= first-child (= last-child child))
      (else
        (= (?. last-child next) child)
        (= (. child previous) last-child)
        (= last-child child)
      )
    )
  )
)

(type + Note (is MarkNode)
  (prop + label String)
  (fn-do + This _
    (param label String)
    (print (str "made " label))
  )
)

(type + Panel (is MarkNode)
  (content
    (<> This
      (+<> MarkNode)
    )
  )
)

(type + Sub (is Panel)
  (content
    (<> This
      (<> Note (text: "one")
        (<> Note (id: spare) (text: "two"))
      )
      (<> Note (text: "three"))
    )
  )
)
```

```output
made one
made two
made three
two
```

### Demonstrates subclass-provided named slot defaults and caller override precedence.

```brix
(main
  (<> SubPanel (id: panel))
  (print (call text-at panel 0))
  (print (call text-at panel 1))
  (blank)
  (<> SubPanel (id: caller)
    (<> MarkText (slot: header) (value: "caller header"))
  )
  (print (call text-at caller 0))
  (print (call text-at caller 1))
)

(fn-expr + text-at String
  (param node MarkNode)
  (param index Index)
  (. (as (!! (call (. node child-at) index)) MarkText) text)
)

(type + MarkNode
  (implements (of MarkupParent MarkNode))
  (prop + parent (? MarkNode))
  (prop + first-child (? MarkNode))
  (prop + last-child (? MarkNode))
  (prop + previous (? MarkNode))
  (prop + next (? MarkNode))
  (fn-do + children _ :get
    (let maybe-child first-child)
    (if maybe-child
      (let child maybe-child)
      (while true
        (yield child)
        (if (not (. child next))
          (break)
        )
        (= child (. child next))
      )
    )
  )
  (fn-do + child-at (? MarkNode)
    (param index Index)
    (let cursor Index 0)
    (for (in child children)
      (if (== cursor index)
        (return child)
      )
      (+= cursor 1)
    )
  )
  (fn-expr + markup-content MarkNode
    (param child MarkNode)
    child
  )
  (fn-do + markup-attach-child _
    (param child MarkNode)
    (param anchor (? MarkNode))
    (= (. child parent) this)
    (if (not first-child)
      (= first-child (= last-child child))
      (else
        (= (?. last-child next) child)
        (= (. child previous) last-child)
        (= last-child child)
      )
    )
  )
)

(type + MarkText (is MarkNode)
  (prop + text String)
  (fn-do + This _
    (param text String)
  )
)

(type + BasePanel (is MarkNode)
  (content
    (+<> MarkText (id: header)
      (<> MarkText (value: "base header"))
    )
    (+<> MarkNode
      (<> MarkText (value: "base body"))
    )
  )
)

(type + SubPanel (is BasePanel)
  (content
    (<> MarkText (slot: header) (value: "sub header"))
    (<> MarkText (value: "sub body"))
  )
)
```

```output
sub header
sub body
caller header
sub body
```

### Demonstrates that subclasses can declare additional named slot outlets.

```brix
(main
  (<> SubPanel (id: panel)
    (<> MarkText (slot: footer) (value: "caller footer"))
  )
  (print (call text-at panel 0))
  (print (call text-at panel 1))
  (print (call text-at panel 2))
)

(fn-expr + text-at String
  (param node MarkNode)
  (param index Index)
  (. (as (!! (call (. node child-at) index)) MarkText) text)
)

(type + MarkNode
  (implements (of MarkupParent MarkNode))
  (prop + parent (? MarkNode))
  (prop + first-child (? MarkNode))
  (prop + last-child (? MarkNode))
  (prop + previous (? MarkNode))
  (prop + next (? MarkNode))
  (fn-do + children _ :get
    (let maybe-child first-child)
    (if maybe-child
      (let child maybe-child)
      (while true
        (yield child)
        (if (not (. child next))
          (break)
        )
        (= child (. child next))
      )
    )
  )
  (fn-do + child-at (? MarkNode)
    (param index Index)
    (let cursor Index 0)
    (for (in child children)
      (if (== cursor index)
        (return child)
      )
      (+= cursor 1)
    )
  )
  (fn-expr + markup-content MarkNode
    (param child MarkNode)
    child
  )
  (fn-do + markup-attach-child _
    (param child MarkNode)
    (param anchor (? MarkNode))
    (= (. child parent) this)
    (if (not first-child)
      (= first-child (= last-child child))
      (else
        (= (?. last-child next) child)
        (= (. child previous) last-child)
        (= last-child child)
      )
    )
  )
)

(type + MarkText (is MarkNode)
  (prop + text String)
  (fn-do + This _
    (param text String)
  )
)

(type + BasePanel (is MarkNode)
  (content
    (+<> MarkText (id: header)
      (<> MarkText (value: "base header"))
    )
  )
)

(type + SubPanel (is BasePanel)
  (content
    (+<> MarkText (id: footer))
    (<> MarkText (value: "sub body"))
  )
)
```

```output
base header
caller footer
sub body
```

### Demonstrates that subclasses cannot redeclare inherited slot outlets.

```brix crash
(main
  (<> SubPanel (id: panel))
)

(type + MarkNode
  (implements (of MarkupParent MarkNode))
  (fn-expr + markup-content MarkNode
    (param child MarkNode)
    child
  )
  (fn-do + markup-attach-child _
    (param child MarkNode)
    (param anchor (? MarkNode))
  )
)

(type + MarkText (is MarkNode)
  (prop + text String)
  (fn-do + This _
    (param text String)
  )
)

(type + BasePanel (is MarkNode)
  (content
    (+<> MarkText (id: header))
  )
)

(type + SubPanel (is BasePanel)
  (content
    (+<> MarkText (id: header))
  )
)
```

```output
Exception: Duplicate slot outlet
<no file:33>
        (+<> MarkText (id: header))
        ^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Demonstrates recursive slot signature discovery reports a diagnostic instead of recursing.

```brix crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> RecursivePanel
    (<> TextBox (slot: header) (value: "caller"))
  )
)

(type + RecursivePanel (is Box)
  (content
    (<> This
      (<> RecursivePanel
        (<+ header)
      )
      (+<> Box (id: header))
    )
  )
)
```

```output
Exception: Recursive markup slot signature discovery for RecursivePanel
<no file:14>
          (<> RecursivePanel
          ^^^^^^^^^^^^^^^^^
```

### Demonstrates that subclass body content appends as ordinary child content when the base has no default slot.

```brix
(main
  (<> ChildSubPanel (id: panel))
  (print (call count-children panel))
  (print (call text-at panel 0))
  (print (call text-at panel 1))
)

(fn-expr + text-at String
  (param node MarkNode)
  (param index Index)
  (. (as (!! (call (. node child-at) index)) MarkText) text)
)

(fn-do + count-children _
  (param node MarkNode)
  (let count 0)
  (for (in child (. node children))
    (+= count 1)
  )
  (return count)
)

(type + MarkNode
  (implements (of MarkupParent MarkNode))
  (prop + parent (? MarkNode))
  (prop + first-child (? MarkNode))
  (prop + last-child (? MarkNode))
  (prop + previous (? MarkNode))
  (prop + next (? MarkNode))
  (fn-do + children _ :get
    (let maybe-child first-child)
    (if maybe-child
      (let child maybe-child)
      (while true
        (yield child)
        (if (not (. child next))
          (break)
        )
        (= child (. child next))
      )
    )
  )
  (fn-do + child-at (? MarkNode)
    (param index Index)
    (let cursor Index 0)
    (for (in child children)
      (if (== cursor index)
        (return child)
      )
      (+= cursor 1)
    )
  )
  (fn-expr + markup-content MarkNode
    (param child MarkNode)
    child
  )
  (fn-do + markup-attach-child _
    (param child MarkNode)
    (param anchor (? MarkNode))
    (= (. child parent) this)
    (if (not first-child)
      (= first-child (= last-child child))
      (else
        (= (?. last-child next) child)
        (= (. child previous) last-child)
        (= last-child child)
      )
    )
  )
)

(type + MarkText (is MarkNode)
  (prop + text String)
  (fn-do + This _
    (param text String)
  )
)

(type + ShellPanel (is MarkNode)
  (content
    (<> MarkText (value: "base shell"))
  )
)

(type + ChildSubPanel (is ShellPanel)
  (content
    (<> MarkText (value: "sub child"))
  )
)
```

```output
2
base shell
sub child
```

### Demonstrates that a caller cannot provide duplicate content for the same slot.

```brix crash
(main
  (<> Panel
    (<> MarkText (slot: header) (value: "a"))
    (<> MarkText (slot: header) (value: "b"))
  )
)

(type + MarkNode
  (implements (of MarkupParent MarkNode))
  (fn-expr + markup-content MarkNode
    (param child MarkNode)
    child
  )
  (fn-do + markup-attach-child _
    (param child MarkNode)
    (param anchor (? MarkNode))
  )
)

(type + MarkText (is MarkNode)
  (prop + text String)
  (fn-do + This _
    (param text String)
  )
)

(type + Panel (is MarkNode)
  (content
    (+<> MarkText (id: header))
  )
)
```

```output
Exception: Duplicate slot content
<no file:5>
        (<> MarkText (slot: header) (value: "b"))
                            ^^^^^^
```

### Demonstrates that a caller cannot provide content for an undeclared slot.

```brix crash
(main
  (<> Panel
    (<> MarkText (slot: footer) (value: "x"))
  )
)

(type + MarkNode
  (implements (of MarkupParent MarkNode))
  (fn-expr + markup-content MarkNode
    (param child MarkNode)
    child
  )
  (fn-expr + markup-attach-child _
    (param child MarkNode)
    (param anchor (? MarkNode))
  )
)

(type + MarkText (is MarkNode)
  (prop + text String)
  (fn-do + This _
    (param text String)
  )
)

(type + Panel (is MarkNode)
  (content
    (+<> MarkText (id: header))
  )
)
```

```output
Exception: Unknown slot
<no file:4>
        (<> MarkText (slot: footer) (value: "x"))
                            ^^^^^^
```

### Root <This> unwraps before inherited slot default routing.

```brix
(main
  (<> ChildPanel (id: panel))
  (blank)
  (print (call text-at panel 0))
  (print (call text-at panel 1))
)

(fn-expr + text-at String
  (param node MarkNode)
  (param index Index)
  (. (as (!! (call (. node child-at) index)) MarkText) text)
)

(type + MarkNode
  (implements (of MarkupParent MarkNode))
  (prop + first-child (? MarkNode))
  (prop + last-child (? MarkNode))
  (prop + previous (? MarkNode))
  (prop + next (? MarkNode))
  (fn-do + children _ :get
    (let maybe-child first-child)
    (if maybe-child
      (let child maybe-child)
      (while true
        (yield child)
        (if (not (. child next))
          (break)
        )
        (= child (. child next))
      )
    )
  )
  (fn-do + child-at (? MarkNode)
    (param index Index)
    (let cursor Index 0)
    (for (in child children)
      (if (== cursor index)
        (return child)
      )
      (+= cursor 1)
    )
  )
  (fn-expr + markup-content MarkNode
    (param child MarkNode)
    child
  )
  (fn-do + markup-attach-child _
    (param child MarkNode)
    (param anchor (? MarkNode))
    (if (not first-child)
      (= first-child (= last-child child))
      (else
        (= (?. last-child next) child)
        (= (. child previous) last-child)
        (= last-child child)
      )
    )
  )
)

(type + MarkText (is MarkNode)
  (prop + text String)
  (fn-do + This _
    (param text String)
  )
)

(type + BasePanel (is MarkNode)
  (content
    (+<> MarkText (id: header))
  )
)

(type + ChildPanel (is BasePanel)
  (content
    (<> This
      (<> MarkText (slot: header) (value: "Root"))
      (<> MarkText (value: "Body"))
    )
  )
)
```

```output
Root
Body
```

### Component-authored named slot content can wrap supplied content in an identified node without requiring that id to exist in the caller scope.

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> ConfirmLike (id: confirm))
  (blank)
  (let actions Box (!! (call (. confirm child-at) 0)))
  (let ok (call first-text-descendant actions))
  (print (. ok text))
)

(type + DialogLike (is Box)
  (content
    (<> This
      (+<> Box (id: actions)
        (<> Box)
      )
    )
  )
)

(type + ConfirmLike (is DialogLike)
  (content
    (<> This
      (<> Box (slot: actions)
        (<> TextBox (id: ok) (value: "OK"))
      )
    )
  )
)

(fn-do + first-text-descendant TextBox
  (param box Box)
  (let first (!! (call (. box child-at) 0)))
  (if (is first TextBox)
    (return (as first TextBox))
  )
  (let wrapper (as first Box))
  (return (as (!! (call (. wrapper child-at) 0)) TextBox))
)
```

```output
OK
```

### A rebindable repeated-slot scope updates tracked reads without rebuilding the physical row, and a closure that reads the stable scope directly observes its current item when invoked.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let items (call (of List RowData) (list (call RowData "a") (call RowData "b"))))
  (<> (of RebindPanel RowData) (id: panel)
    (= items items)
    (<> RebindCommandRow (slot: row) (param: slot)
      (= text (. slot #item text))
      (= command (=> () (. slot item text)))
    )
  )
  (blank)
  (let region (!! (. panel %first-child)))
  (let first (as (!! (. region %first-child)) RebindCommandRow))
  (= (. first uid) 41)
  (print (call row-texts panel))
  (print (. first uid))
  (blank)
  (call (. panel rebind) 0 10 100 (call RowData "x"))
  (call (. panel markup-batch-queue checkpoint))
  (blank)
  (let rebound (as (!! (. region %first-child)) RebindCommandRow))
  (print (call row-texts panel))
  (print (. rebound uid))
  (print (call (!! (. rebound command))))
)

(type + RowData
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(type + RebindCommandRow (is TextBox)
  (prop + uid U64 (= 0))
  (prop + command (? (of Getter String)))
)

(fn-do + row-texts String
  (param panel Box)
  (let texts (list String) (call (of List String)))
  (let row-region (. panel %first-child))
  (if row-region
    (for (in child (. row-region children))
      (+= texts (. (as child TextBox) text))
    )
  )
  (return (call (. "," join) texts))
)
```

```output
a,b
41
x,b
41
x
```

### Branches, nested loops, and styles all re-evaluate from one stable rebindable scope.

```brix
(import /core/tests/support/markup-fixtures)
(import /core/style)
(blank)

(main
  (let items (call (of List RebindComplexData) (list (call RebindComplexData true (call (of List String) (list "a" "b"))) (call RebindComplexData false (call (of List String) (list "z"))))))
  (<> (of RebindPanel RebindComplexData) (id: panel)
    (= items items)
    (<> RebindComplexRow (slot: row) (param: slot)
      (= address (. slot #address))
      (if (. slot #item flag)
        (<> TextBox (value: "on"))
        (else
          (<> TextBox (value: "off"))
        )
      )
      (for (in part (. slot #item parts))
        (<> TextBox (value: part))
      )
    )
  )
  (blank)
  (let region (!! (. panel %first-child)))
  (let row (as (!! (. region %first-child)) RebindComplexRow))
  (print (str (. row $address) ":" (call descendant-texts row)))
  (blank)
  (call (. panel rebind) 0 9 90 (call RebindComplexData false (call (of List String) (list "x"))))
  (call (. panel markup-batch-queue checkpoint))
  (print (str (. row $address) ":" (call descendant-texts row)))
)

(type + RebindComplexData
  (prop + flag Bool)
  (prop + parts (list String))
  (fn-do + This _
    (param flag _)
    (param parts _)
  )
)

(type + RebindAddressStyle (is Style)
  (prop + address I64 (= 0))
  (blank)
  (style-domains address (domain paint))
)

(type + RebindComplexRow (is Box)
  (using RebindAddressStyle)
)

(fn-do + descendant-texts String
  (param box Box)
  (let texts (list String) (call (of List String)))
  (for (in child (. box children))
    (if (is child TextBox)
      (+= texts (. (as child TextBox) text))
      (else
        (let nested (call descendant-texts child))
        (if nested
          (+= texts nested)
        )
      )
    )
  )
  (return (call (. "|" join) texts))
)
```

```output
0:on|a|b
9:off|x
```

### Untracked long-lived reads of a rebindable slot scope are rejected at the member access.

```brix crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> (of RebindPanel RowData)
    (= items (call (of List RowData) (list (call RowData "a"))))
    (<> TextBox (slot: row) (param: slot) (value: (. slot item text)))
  )
)

(type + RowData
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)
```

```output
Exception: Rebindable slot scope reads must be tracked as slot.#item; use an explicit snapshot inside do for a stable value
<no file:8>
        (<> TextBox (slot: row) (param: slot) (value: (. slot item text)))
                                                             ^^^^^
```

### Closure locals and parameters may shadow a rebindable-scope-derived outer local.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> (of RebindPanel RowData) (id: panel)
    (= items (call (of List RowData) (list (call RowData "a"))))
    (<> ShadowCaptureRow (slot: row) (param: slot)
      (let text (. slot #item text))
      (= eager
        (=> () :do
          (block
            (let text "eager")
            (return text)
          )
        )
      )
      (= parameter (=> ((: text String)) text))
      (= nested
        (=> () :do
          (block
            (let text "nested")
            (let read (=> () text))
            (return (call read))
          )
        )
      )
    )
  )
  (blank)
  (let region (!! (. panel %first-child)))
  (let row (as (!! (. region %first-child)) ShadowCaptureRow))
  (print (call (!! (. row eager))))
  (print (call (!! (. row parameter)) "parameter"))
  (print (call (!! (. row nested))))
)

(type + RowData
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(type + TextMapper (is (fn-type String (param text String))))

(type + ShadowCaptureRow (is Box)
  (prop + eager (? (of Getter String)))
  (prop + parameter (? TextMapper))
  (prop + nested (? (of Getter String)))
)
```

```output
eager
parameter
nested
```

### Eager values derived from a rebindable scope cannot escape in an event-style closure.

```brix warn crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> (of RebindPanel RowData)
    (= items (call (of List RowData) (list (call RowData "a"))))
    (<> CaptureRow (slot: row) (param: slot)
      (let text (. slot #item text))
      (= command
        (=> () :do
          (block
            (if false
              (let text "unrelated")
            )
            (return text)
          )
        )
      )
    )
  )
)

(type + RowData
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(type + CaptureRow (is Box)
  (prop + command (of Getter String))
)
```

```output
Exception: Local text derived from rebindable slot scope slot cannot escape into a closure; read slot directly in the closure or take an explicit snapshot inside do
<no file:16>
                (return text)
                        ^^^^
```

### Closures in supplied slot content may refer to the slot host's own markup id; the id resolves to the owning class property when the closure runs.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo))
  (print (call (. demo run)))
)

(type + Demo (is Box)
  (content
    (<> This
      (<> Holder (id: panel)
        (<> Clicker (slot: actions)
          (= action (=> () :do (+= (. panel hits) 1)))
        )
      )
    )
  )
  (fn-do + run _
    (let callback (. (!! (call find-clicker panel)) action))
    (if callback
      (call callback)
    )
    (return (. panel hits))
  )
)

(type + Holder (is Box)
  (prop + hits _ (= 0))
  (content
    (<> This
      (+<> Box (id: actions))
    )
  )
)

(type + Clicker (is Box)
  (prop + action (? Doer))
)

(fn-do + find-clicker (? Clicker)
  (param node Box)
  (if (is node Clicker)
    (return (as node Clicker))
  )
  (for (in child (. node children))
    (if (is child Box)
      (let found (call find-clicker (as child Box)))
      (return-if found (if found))
    )
  )
  (return null)
)
```

```output
1
```

### A closure in a named element of top-level markup may read an earlier named element whose class has slots it gives no content to.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Box (id: root)
    (<> Holder (id: holder)
      (= hits 4)
    )
    (<> Trigger (id: trigger)
      (= action (=> () :do (print (. holder hits))))
    )
  )
  (blank)
  (call (. trigger action))
)

(type + Holder (is Box)
  (prop + hits _ (= 0))
  (content
    (<> This
      (+<> Box (id: actions))
    )
  )
)

(type + Trigger (is Box)
  (prop + action Doer (= (=> () :do (print "none"))))
)
```

```output
4
```

### A closure may refer to a slot host declared later in the same content; the reference reads the owning class property when the closure runs instead of constructing the host early.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo))
  (print (call (. demo run)))
)

(type + Demo (is Box)
  (prop + hits _ (= 0))
  (content
    (<> This
      (<> Clicker
        (= action (=> () :do (call (. panel fire))))
      )
      (<> Holder (id: panel)
        (<> Box (slot: body)
          (<> TextBox (id: field) (value: "hello"))
        )
      )
    )
  )
  (subscribe-for (on value (. panel fired))
    (+= hits (. field text length))
  )
  (fn-do + run _
    (let callback (. (!! (call find-clicker this)) action))
    (if callback
      (call callback)
    )
    (return hits)
  )
)

(type + Holder (is Box)
  (event + fired Bool)
  (content
    (<> This
      (+<> Box (id: body))
    )
  )
  (fn-do + fire _
    (<- fired true)
  )
)

(type + Clicker (is Box)
  (prop + action (? Doer))
)

(fn-do + find-clicker (? Clicker)
  (param node Box)
  (if (is node Clicker)
    (return (as node Clicker))
  )
  (for (in child (. node children))
    (if (is child Box)
      (let found (call find-clicker (as child Box)))
      (return-if found (if found))
    )
  )
  (return null)
)
```

```output
5
```

### A repeated slot outlet inside a keyed row rebuilds its caller content when the row item is replaced by a new object under the same key, while the physical row is preserved. Arguments passed through a derived local follow the replacement too.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let items (call (of List RowData) (list (call RowData "a" "A") (call RowData "b" "B"))))
  (<> KeyedSlotPanel (id: panel)
    (= items items)
    (<> TextBox (slot: row) (param: item) (value: (. item text)))
    (<> TextBox (slot: label) (param: item) (value: (str (. item text) "~")))
  )
  (blank)
  (let region (!! (. panel %first-child)))
  (let first (!! (. region %first-child)))
  (print (call slot-texts panel))
  (= ([] items 0) (call RowData "a" "Z"))
  (print (call slot-texts panel))
  (print (if-expr (== first (!! (. region %first-child))) "preserved" "lost"))
  (= ([] items 1) (call RowData "b" "B"))
  (print (call slot-texts panel))
)

(type + RowData
  (prop + id String)
  (prop + text String)
  (fn-do + This _
    (param id String)
    (param text String)
  )
)

(type + KeyedSlotPanel (is Box)
  (prop + items (list RowData) (= (call (of List RowData))))
  (content
    (<> This
      (for (in item #items (keyed-by (. item id)))
        (let same item)
        (<> Box
          (+<> Box (id: row) (param: item RowData))
          (+<> Box (id: label) (param: same RowData))
        )
      )
    )
  )
)

(fn-do + slot-texts String
  (param panel Box)
  (let parts (list String) (call (of List String)))
  (for (in child (. (!! (. panel %first-child)) children))
    (for (in slot (. (as child Box) children))
      (+= parts (call text-in (as slot Box)))
    )
  )
  (return (call (. "," join) parts))
)

(fn-do + text-in String
  (param box Box)
  (return-if (. (as box TextBox) text) (if (is box TextBox)))
  (for (in child (. box children))
    (return (call text-in (as child Box)))
  )
  (return "")
)
```

```output
A,A~,B,B~
Z,Z~,B,B~
preserved
Z,Z~,B,B~
```

### Slot outlets under an `if` in a keyed markup row, one plain and one that rebuilds on the row's item, come and go with the condition.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let items (call (of List RowData) (list (call RowData "a" "A") (call RowData "b" "B"))))
  (<> KeyedSlotPanel (id: panel)
    (= items items)
    (<> TextBox (slot: mark) (value: "*"))
    (<> TextBox (slot: label) (param: item) (value: (. item text)))
  )
  (blank)
  (print (call slot-texts panel))
  (= (. panel show) false)
  (print (call slot-texts panel))
  (= ([] items 0) (call RowData "a" "Z"))
  (= (. panel show) true)
  (print (call slot-texts panel))
)

(type + RowData
  (prop + id String)
  (prop + text String)
  (fn-do + This _
    (param id String)
    (param text String)
  )
)

(type + KeyedSlotPanel (is Box)
  (prop + items (list RowData) (= (call (of List RowData))))
  (prop + show _ (= true))
  (content
    (<> This
      (for (in item #items (keyed-by (. item id)))
        (<> Box
          (if #show
            (+<> Box (id: mark))
            (+<> Box (id: label) (param: item RowData))
          )
        )
      )
    )
  )
)

(fn-do + slot-texts String
  (param panel Box)
  (let parts (list String) (call (of List String)))
  (for (in row (. (!! (. panel %first-child)) children))
    (+= parts (call texts-in (as row Box)))
  )
  (return (call (. "|" join) parts))
)

(fn-do + texts-in String
  (param box Box)
  (return-if (. (as box TextBox) text) (if (is box TextBox)))
  (let parts (list String) (call (of List String)))
  (for (in child (. box children))
    (+= parts (call texts-in (as child Box)))
  )
  (return (call (. "" join) parts))
)
```

```output
*A|*B
|
*Z|*B
```

### A repeated slot outlet placed directly in a static loop inside a keyed row also rebuilds its caller content when the row item is replaced, while the physical row is preserved.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let items (call (of List RowData) (list (call RowData "a" "A") (call RowData "b" "B"))))
  (<> KeyedSlotPanel (id: panel)
    (= items items)
    (<> TextBox (slot: row) (param: item) (value: (. item text)))
  )
  (blank)
  (let region (!! (. panel %first-child)))
  (let first (!! (. region %first-child)))
  (print (call slot-texts panel))
  (= ([] items 0) (call RowData "a" "Z"))
  (print (call slot-texts panel))
  (print (if-expr (== first (!! (. region %first-child))) "preserved" "lost"))
)

(type + RowData
  (prop + id String)
  (prop + text String)
  (fn-do + This _
    (param id String)
    (param text String)
  )
)

(type + KeyedSlotPanel (is Box)
  (prop + items (list RowData) (= (call (of List RowData))))
  (prop + passes _ (= (call (of List String) (list "x" "y"))))
  (content
    (<> This
      (for (in item #items (keyed-by (. item id)))
        (<> Box
          (for (in pass passes)
            (+<> Box (id: row) (param: item RowData))
          )
        )
      )
    )
  )
)

(fn-do + slot-texts String
  (param panel Box)
  (let parts (list String) (call (of List String)))
  (for (in child (. (!! (. panel %first-child)) children))
    (for (in slot (. (as child Box) children))
      (+= parts (call text-in (as slot Box)))
    )
  )
  (return (call (. "," join) parts))
)

(fn-do + text-in String
  (param box Box)
  (return-if (. (as box TextBox) text) (if (is box TextBox)))
  (for (in child (. box children))
    (return (call text-in (as child Box)))
  )
  (return "")
)
```

```output
A,A,B,B
Z,Z,B,B
preserved
```

### A loop that is the only default slot content fills a node of the slot's type.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let names (call (of List String) (list "a" "b")))
  (let plain (list "x" "y"))
  (blank)
  (<> Card (id: keyed)
    (for (in name #names (keyed-by name))
      (<> TextBox (value: name))
    )
  )
  (blank)
  (<> Card (id: unkeyed)
    (for (in name #names)
      (<> TextBox (value: name))
    )
  )
  (blank)
  (<> Card (id: static)
    (for (in name plain)
      (<> TextBox (value: name))
    )
  )
  (blank)
  (print (call dump keyed))
  (print (call dump unkeyed))
  (print (call dump static))
  (+= names "c")
  (print (call dump keyed))
  (print (call dump unkeyed))
)

(fn-do + dump String
  (param box Box)
  (let parts (call (of List String)))
  (for (in child (. box children))
    (if (is child TextBox)
      (+= parts (. (as child TextBox) text))
      (else
        (+= parts (str "[" (call dump child) "]"))
      )
    )
  )
  (return (call (. "," join) parts))
)

(type + Card (is Box)
  (content
    (<> This
      (<> TextBox (value: "head"))
      (+<> Box)
    )
  )
)
```

```output
head,[[a,b]]
head,[[a,b]]
head,[x,y]
head,[[a,b,c]]
head,[[a,b,c]]
```

### A branch or match that is the only default slot content fills a node of the slot's type.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let is-shown true)
  (let kind "a")
  (let is-fixed false)
  (blank)
  (<> Card (id: branched)
    (if #is-shown
      (<> TextBox (value: "shown"))
    )
  )
  (blank)
  (<> Card (id: matched)
    (match #kind
      (case= "a"
        (<> TextBox (value: "alpha"))
      )
      (case _
        (<> TextBox (value: "other"))
      )
    )
  )
  (blank)
  (<> Card (id: static)
    (if is-fixed
      (<> TextBox (value: "fixed"))
      (else
        (<> TextBox (value: "loose"))
      )
    )
  )
  (blank)
  (print (call dump branched))
  (print (call dump matched))
  (print (call dump static))
  (= is-shown false)
  (= kind "b")
  (print (call dump branched))
  (print (call dump matched))
  (= is-shown true)
  (print (call dump branched))
)

(fn-do + dump String
  (param box Box)
  (let parts (call (of List String)))
  (for (in child (. box children))
    (if (is child TextBox)
      (+= parts (. (as child TextBox) text))
      (else
        (+= parts (str "[" (call dump child) "]"))
      )
    )
  )
  (return (call (. "," join) parts))
)

(type + Card (is Box)
  (content
    (<> This
      (<> TextBox (value: "head"))
      (+<> Box)
    )
  )
)
```

```output
head,[[shown]]
head,[[alpha]]
head,[loose]
head,[[]]
head,[[other]]
head,[[shown]]
```

### A slot outlet's styles apply to the node that holds lone control flow, as they do to the node that holds several children.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let names (call (of List String) (list "a" "b")))
  (blank)
  (<> BatchDemo (id: demo)
    (<> Card (id: looped)
      (for (in name #names (keyed-by name))
        (<> TextBox (value: name))
      )
    )
    (blank)
    (<> Card (id: paired)
      (<> TextBox (value: "x"))
      (<> TextBox (value: "y"))
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot (!! (. looped %last-child))))
  (print (call style-computed-snapshot (!! (. paired %last-child))))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Card (is Box)
  (content
    (<> This
      (<> TextBox (value: "head"))
      (+<> Box (• flex 2))
    )
  )
)
```

```output
node BoxPlacement seg=owned gen=1 mask=1 flex=2
node BoxPlacement seg=owned gen=1 mask=1 flex=2
```

### A slot outlet's fallback content can be lone control flow, and caller content replaces it.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Listing (id: listing))
  (print (call dump listing))
  (+= (. listing names) "c")
  (print (call dump listing))
  (blank)
  (<> Listing (id: replaced)
    (<> TextBox (value: "caller"))
  )
  (print (call dump replaced))
)

(fn-do + dump String
  (param box Box)
  (let parts (call (of List String)))
  (for (in child (. box children))
    (if (is child TextBox)
      (+= parts (. (as child TextBox) text))
      (else
        (+= parts (str "[" (call dump child) "]"))
      )
    )
  )
  (return (call (. "," join) parts))
)

(type + Listing (is Box)
  (prop + names _ (= (call (of List String) (list "a" "b"))))
  (content
    (<> This
      (+<> Box
        (for (in name #names (keyed-by name))
          (<> TextBox (value: name))
        )
      )
    )
  )
)
```

```output
[[a,b]]
[[a,b,c]]
caller
```

### Loops and branches in slot content under a batch root read locals of the enclosing function.

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let names (call (of List String) (list "a" "b")))
  (let is-shown true)
  (blank)
  (<> BatchDemo (id: demo)
    (<> Card (id: keyed)
      (<> Box
        (for (in name #names (keyed-by name))
          (<> TextBox (value: name))
        )
      )
    )
    (blank)
    (<> Card (id: unkeyed)
      (<> Box
        (for (in name #names)
          (<> TextBox (value: name))
        )
      )
    )
    (blank)
    (<> Card (id: branched)
      (<> Box
        (if #is-shown
          (<> TextBox (value: "shown"))
        )
      )
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call dump keyed))
  (print (call dump unkeyed))
  (print (call dump branched))
  (+= names "c")
  (= is-shown false)
  (call (. demo markup-batch-queue checkpoint))
  (print (call dump keyed))
  (print (call dump unkeyed))
  (print (call dump branched))
)

(fn-do + dump String
  (param box Box)
  (let parts (call (of List String)))
  (for (in child (. box children))
    (if (is child TextBox)
      (+= parts (. (as child TextBox) text))
      (else
        (+= parts (str "[" (call dump child) "]"))
      )
    )
  )
  (return (call (. "," join) parts))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Card (is Box)
  (content
    (<> This
      (<> TextBox (value: "head"))
      (+<> Box)
    )
  )
)
```

```output
head,[[a,b]]
head,[[a,b]]
head,[[shown]]
head,[[a,b,c]]
head,[[a,b,c]]
head,[[]]
```

### A slot filled inside a live if holds the caller's content, one region deeper, while the condition holds, and nothing otherwise, as the unfilled slot shows.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let is-shown true)
  (blank)
  (<> Card (id: card)
    (if #is-shown
      (<> TextBox (slot: icon) (value: "badge"))
    )
  )
  (blank)
  (print (call dump card))
  (= is-shown false)
  (print (call dump card))
  (= is-shown true)
  (print (call dump card))
)

(fn-do + dump String
  (param box Box)
  (let parts (call (of List String)))
  (for (in child (. box children))
    (if (is child TextBox)
      (+= parts (. (as child TextBox) text))
      (else
        (+= parts (str "[" (call dump child) "]"))
      )
    )
  )
  (return (call (. "," join) parts))
)

(type + Card (is Box)
  (content
    (<> This
      (+<> Box (id: icon))
      (+<> Box)
    )
  )
)
```

```output
[badge]
[]
[badge]
```

### A slot filled inside a live if holds the caller's content while the condition holds, and the slot's fallback otherwise.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let is-shown true)
  (blank)
  (<> Card (id: card)
    (if #is-shown
      (<> TextBox (slot: icon) (value: "badge"))
    )
    (<> TextBox (value: "body"))
  )
  (blank)
  (print (call dump card))
  (= is-shown false)
  (print (call dump card))
  (= is-shown true)
  (print (call dump card))
)

(fn-do + dump String
  (param box Box)
  (let parts (call (of List String)))
  (for (in child (. box children))
    (if (is child TextBox)
      (+= parts (. (as child TextBox) text))
      (else
        (+= parts (str "[" (call dump child) "]"))
      )
    )
  )
  (return (call (. "," join) parts))
)

(type + Card (is Box)
  (content
    (<> This
      (+<> Box (id: icon)
        (<> TextBox (value: "no icon"))
      )
      (+<> Box)
    )
  )
)
```

```output
[badge],body
[no icon],body
[badge],body
```

### A static if decides the slot's content as the element is made.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let shown true)
  (let hidden false)
  (blank)
  (<> Card (id: visible)
    (if shown
      (<> TextBox (slot: icon) (value: "badge"))
    )
  )
  (<> Card (id: invisible)
    (if hidden
      (<> TextBox (slot: icon) (value: "badge"))
    )
  )
  (blank)
  (print (call dump visible))
  (print (call dump invisible))
)

(fn-do + dump String
  (param box Box)
  (let parts (call (of List String)))
  (for (in child (. box children))
    (if (is child TextBox)
      (+= parts (. (as child TextBox) text))
      (else
        (+= parts (str "[" (call dump child) "]"))
      )
    )
  )
  (return (call (. "," join) parts))
)

(type + Card (is Box)
  (content
    (<> This
      (+<> Box (id: icon)
        (<> TextBox (value: "no icon"))
      )
      (+<> Box
        (<> TextBox (value: "no body"))
      )
    )
  )
)
```

```output
[badge],no body
[no icon],no body
```

### Each arm of an if and else, or of a match, may fill the same slot; an arm that fills nothing leaves the fallback.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let is-draft true)
  (let kind "a")
  (blank)
  (<> Card (id: toggled)
    (if #is-draft
      (<> TextBox (slot: icon) (value: "draft"))
      (else
        (<> TextBox (slot: icon) (value: "final"))
      )
    )
  )
  (<> Card (id: matched)
    (match #kind
      (case= "a"
        (<> TextBox (slot: icon) (value: "alpha"))
      )
      (case= "b"
        (<> TextBox (slot: icon) (value: "beta"))
      )
      (case _
        (<> TextBox (value: "plain"))
      )
    )
  )
  (blank)
  (print (call dump toggled))
  (print (call dump matched))
  (= is-draft false)
  (= kind "b")
  (print (call dump toggled))
  (print (call dump matched))
  (= kind "c")
  (print (call dump matched))
)

(fn-do + dump String
  (param box Box)
  (let parts (call (of List String)))
  (for (in child (. box children))
    (if (is child TextBox)
      (+= parts (. (as child TextBox) text))
      (else
        (+= parts (str "[" (call dump child) "]"))
      )
    )
  )
  (return (call (. "," join) parts))
)

(type + Card (is Box)
  (content
    (<> This
      (+<> Box (id: icon)
        (<> TextBox (value: "no icon"))
      )
      (+<> Box
        (<> TextBox (value: "no body"))
      )
    )
  )
)
```

```output
[draft],no body
[alpha],[[]]
[final],no body
[beta],[[]]
[no icon],[[plain]]
```

### An arm may fill a slot and give the default slot content too, and both come and go with it. A fallback that reads the component's state follows it while it shows.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let is-open true)
  (blank)
  (<> Card (id: card)
    (if #is-open
      (<> TextBox (slot: icon) (value: "badge"))
      (<> TextBox (value: "details"))
    )
  )
  (blank)
  (print (call dump card))
  (= is-open false)
  (print (call dump card))
  (= (. card label) "closed")
  (print (call dump card))
)

(fn-do + dump String
  (param box Box)
  (let parts (call (of List String)))
  (for (in child (. box children))
    (if (is child TextBox)
      (+= parts (. (as child TextBox) text))
      (else
        (+= parts (str "[" (call dump child) "]"))
      )
    )
  )
  (return (call (. "," join) parts))
)

(type + Card (is Box)
  (prop + label _ (= "no icon"))
  (content
    (<> This
      (+<> Box (id: icon)
        (<> TextBox (value: #label))
      )
      (+<> Box
        (<> TextBox (value: "no body"))
      )
    )
  )
)
```

```output
[badge],[[details]]
[no icon],[[]]
[closed],[[]]
```

### One component given a slot unfilled, filled and filled in a branch in one program.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let is-shown false)
  (blank)
  (<> Card (id: plain))
  (<> Card (id: filled)
    (<> TextBox (slot: icon) (value: "given"))
  )
  (<> Card (id: branched)
    (if #is-shown
      (<> TextBox (slot: icon) (value: "maybe"))
    )
  )
  (blank)
  (print (call dump plain))
  (print (call dump filled))
  (print (call dump branched))
  (= is-shown true)
  (print (call dump branched))
)

(fn-do + dump String
  (param box Box)
  (let parts (call (of List String)))
  (for (in child (. box children))
    (if (is child TextBox)
      (+= parts (. (as child TextBox) text))
      (else
        (+= parts (str "[" (call dump child) "]"))
      )
    )
  )
  (return (call (. "," join) parts))
)

(type + Card (is Box)
  (content
    (<> This
      (+<> Box (id: icon)
        (<> TextBox (value: "no icon"))
      )
      (+<> Box)
    )
  )
)
```

```output
no icon
given
[no icon]
[maybe]
```

### Each arm of an else-if chain may fill the slot, alone or with default slot content, and an arm that fills nothing leaves the fallback.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let level 0)
  (let extra "x")
  (blank)
  (<> Card (id: only)
    (if (== #level 0)
      (<> TextBox (slot: icon) (value: "zero"))
      (else-if (== #level 1)
        (<> TextBox (slot: icon) (value: (str "one " extra)))
      )
      (else-if (== #level 2)
        (<> TextBox (slot: icon) (value: "two"))
      )
    )
  )
  (<> Card (id: mixed)
    (if (== #level 0)
      (<> TextBox (slot: icon) (value: "zero"))
      (else-if (== #level 1)
        (<> TextBox (slot: icon) (value: "one"))
        (<> TextBox (value: "one body"))
      )
      (else
        (<> TextBox (value: "else body"))
      )
    )
  )
  (blank)
  (print (str (call dump only) " | " (call dump mixed)))
  (= level 1)
  (print (str (call dump only) " | " (call dump mixed)))
  (= level 2)
  (print (str (call dump only) " | " (call dump mixed)))
  (= level 0)
  (print (str (call dump only) " | " (call dump mixed)))
)

(fn-do + dump String
  (param box Box)
  (let parts (call (of List String)))
  (for (in child (. box children))
    (if (is child TextBox)
      (+= parts (. (as child TextBox) text))
      (else
        (+= parts (str "[" (call dump child) "]"))
      )
    )
  )
  (return (call (. "," join) parts))
)

(type + Card (is Box)
  (content
    (<> This
      (+<> Box (id: icon)
        (<> TextBox (value: "no icon"))
      )
      (+<> Box
        (<> TextBox (value: "no body"))
      )
    )
  )
)
```

```output
[zero],no body | [zero],[[]]
[one x],no body | [one],[[one body]]
[two],no body | [no icon],[[else body]]
[zero],no body | [zero],[[]]
```

### A slot filled both outside and inside a branch is filled twice.

```brix crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let is-shown true)
  (blank)
  (<> Card (id: card)
    (<> TextBox (slot: icon) (value: "first"))
    (if #is-shown
      (<> TextBox (slot: icon) (value: "second"))
    )
  )
)

(type + Card (is Box)
  (content
    (<> This
      (+<> Box (id: icon))
      (+<> Box)
    )
  )
)
```

```output
Exception: Duplicate slot content
<no file:11>
          (<> TextBox (slot: icon) (value: "second"))
          ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Two branches filling one slot fill it twice.

```brix crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let a true)
  (let b false)
  (blank)
  (<> Card (id: card)
    (if #a
      (<> TextBox (slot: icon) (value: "first"))
    )
    (if #b
      (<> TextBox (slot: icon) (value: "second"))
    )
  )
)

(type + Card (is Box)
  (content
    (<> This
      (+<> Box (id: icon))
      (+<> Box)
    )
  )
)
```

```output
Exception: Duplicate slot content
<no file:14>
          (<> TextBox (slot: icon) (value: "second"))
          ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### One arm filling a slot twice fills it twice.

```brix crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let a true)
  (blank)
  (<> Card (id: card)
    (if #a
      (<> TextBox (slot: icon) (value: "first"))
      (<> TextBox (slot: icon) (value: "second"))
    )
  )
)

(type + Card (is Box)
  (content
    (<> This
      (+<> Box (id: icon))
      (+<> Box)
    )
  )
)
```

```output
Exception: Duplicate slot content
<no file:11>
          (<> TextBox (slot: icon) (value: "second"))
                             ^^^^
```

### A slot filled in a branch inside a loop is rejected.

```brix crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let names (call (of List String) (list "a" "b")))
  (blank)
  (<> Card (id: card)
    (for (in name names)
      (if (== name "a")
        (<> TextBox (slot: icon) (value: name))
      )
    )
  )
)

(type + Card (is Box)
  (content
    (<> This
      (+<> Box (id: icon))
      (+<> Box)
    )
  )
)
```

```output
Exception: A slot is filled by a line of the element's body, or by a line of an arm of an if or match there
<no file:11>
            (<> TextBox (slot: icon) (value: name))
                               ^^^^
```

### A slot filled in a loop inside a branch is rejected.

```brix crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let a true)
  (let names (call (of List String) (list "a" "b")))
  (blank)
  (<> Card (id: card)
    (if #a
      (for (in name names)
        (<> TextBox (slot: icon) (value: name))
      )
    )
  )
)

(type + Card (is Box)
  (content
    (<> This
      (+<> Box (id: icon))
      (+<> Box)
    )
  )
)
```

```output
Exception: A slot is filled by a line of the element's body, or by a line of an arm of an if or match there
<no file:12>
            (<> TextBox (slot: icon) (value: name))
                               ^^^^
```

### A match arm that binds names does not fill a slot.

```brix crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let pick (? Int) 3)
  (blank)
  (<> Card (id: card)
    (match pick
      (case (: n Int)
        (<> TextBox (slot: icon) (value: "number"))
      )
    )
  )
)

(type + Card (is Box)
  (content
    (<> This
      (+<> Box (id: icon))
      (+<> Box)
    )
  )
)
```

```output
Exception: A match arm that fills a slot binds no names; test the subject without binding, as |= value or a type alone does
<no file:10>
          (case (: n Int)
                ^^^^^^^^^
```

### A slot filled in an if two deep is rejected.

```brix crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let shown true)
  (let more true)
  (blank)
  (<> Card (id: card)
    (if shown
      (if more
        (<> TextBox (slot: icon) (value: "badge"))
      )
    )
  )
)

(type + Card (is Box)
  (content
    (<> This
      (+<> Box (id: icon))
      (+<> Box)
    )
  )
)
```

```output
Exception: A slot is filled by a line of the element's body, or by a line of an arm of an if or match there
<no file:12>
            (<> TextBox (slot: icon) (value: "badge"))
                               ^^^^
```

### A slot is not passed on with <+ inside an if.

```brix crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Wrap (id: wrap)
    (<> TextBox (slot: icon) (value: "badge"))
  )
)

(type + Card (is Box)
  (content
    (<> This
      (+<> Box (id: icon))
      (+<> Box)
    )
  )
)

(type + Wrap (is Box)
  (prop + shown _ (= true))
  (content
    (<> This
      (<> Card
        (if shown
          (<+ icon)
        )
      )
    )
  )
)
```

```output
Exception: A slot is not passed on with <+ inside an if or match; write <+ as a line of the element's body
<no file:26>
              (<+ icon)
              ^^^^^^^^^
```

### A slot filled inside an if is not passed on with <+.

```brix crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let shown true)
  (blank)
  (<> Wrap (id: wrap)
    (if shown
      (<> TextBox (slot: icon) (value: "badge"))
    )
  )
)

(type + Card (is Box)
  (content
    (<> This
      (+<> Box (id: icon))
      (+<> Box)
    )
  )
)

(type + Wrap (is Box)
  (content
    (<> This
      (<> Card
        (<+ icon)
      )
    )
  )
)
```

```output
Exception: A slot filled inside an if or match is not passed on with <+; give it content here instead
<no file:28>
            (<+ icon)
            ^^^^^^^^^
```

### A subclass does not fill its base's slots inside an if.

```brix crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Sub (id: sub))
)

(type + Card (is Box)
  (content
    (<> This
      (+<> Box (id: icon))
      (+<> Box)
    )
  )
)

(type + Sub (is Card)
  (prop + shown _ (= true))
  (content
    (if shown
      (<> TextBox (slot: icon) (value: "badge"))
    )
  )
)
```

```output
Exception: A subclass fills its base's slots with lines of its content, not inside an if or match
<no file:21>
        (if shown
        ^^^^^^^^
```

### A component does not read an outlet by its name where its slot is filled inside an if.

```brix crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let shown true)
  (blank)
  (<> Card (id: card)
    (if shown
      (<> TextBox (slot: icon) (value: "badge"))
    )
  )
)

(fn-do + dump String
  (param box Box)
  (let parts (call (of List String)))
  (for (in child (. box children))
    (if (is child TextBox)
      (+= parts (. (as child TextBox) text))
      (else
        (+= parts (str "[" (call dump child) "]"))
      )
    )
  )
  (return (call (. "," join) parts))
)

(type + Card (is Box)
  (content
    (<> This
      (+<> Box (id: icon))
      (<> TextBox (value: (call dump icon)))
    )
  )
)
```

```output
Exception: Outlet "icon" is not read by its name where its slot is filled inside an if or match
<no file:33>
          (<> TextBox (value: (call dump icon)))
                                         ^^^^
```

### A slot filled inside a live if reaches an outlet in each row of a keyed loop, rows added later too.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let shown true)
  (blank)
  (<> List (id: list)
    (if #shown
      (<> TextBox (slot: icon) (value: "badge"))
    )
  )
  (blank)
  (print (call dump list))
  (= shown false)
  (print (call dump list))
  (+= (. list names) "c")
  (print (call dump list))
  (= shown true)
  (print (call dump list))
)

(fn-do + dump String
  (param box Box)
  (let parts (call (of List String)))
  (for (in child (. box children))
    (if (is child TextBox)
      (+= parts (. (as child TextBox) text))
      (else
        (+= parts (str "[" (call dump child) "]"))
      )
    )
  )
  (return (call (. "," join) parts))
)

(type + List (is Box)
  (prop + names _ (= (call (of List String) (list "a" "b"))))
  (content
    (<> This
      (for (in name #names (keyed-by name))
        (<> Box
          (+<> Box (id: icon)
            (<> TextBox (value: "no icon"))
          )
          (<> TextBox (value: name))
        )
      )
    )
  )
)
```

```output
[[[badge],a],[[badge],b]]
[[[no icon],a],[[no icon],b]]
[[[no icon],a],[[no icon],b],[[no icon],c]]
[[[badge],a],[[badge],b],[[badge],c]]
```

### A repeated slot is not filled inside an if.

```brix crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let shown true)
  (blank)
  (<> List (id: list)
    (if shown
      (<> TextBox (slot: row) (param: who) (value: who))
    )
  )
)

(type + List (is Box)
  (prop + names _ (= (call (of List String) (list "a" "b"))))
  (content
    (<> This
      (for (in name names)
        (+<> TextBox (id: row) (param: name String))
      )
    )
  )
)
```

```output
Exception: A repeated slot cannot be filled inside an if or match
<no file:10>
          (<> TextBox (slot: row) (param: who) (value: who))
          ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A slot filled inside a live if in a class's content follows the class's property.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Page (id: page))
  (print (call dump page))
  (= (. page shown) false)
  (print (call dump page))
  (= (. page shown) true)
  (print (call dump page))
)

(fn-do + dump String
  (param box Box)
  (let parts (call (of List String)))
  (for (in child (. box children))
    (if (is child TextBox)
      (+= parts (. (as child TextBox) text))
      (else
        (+= parts (str "[" (call dump child) "]"))
      )
    )
  )
  (return (call (. "," join) parts))
)

(type + Card (is Box)
  (content
    (<> This
      (+<> Box (id: icon)
        (<> TextBox (value: "no icon"))
      )
      (+<> Box
        (<> TextBox (value: "no body"))
      )
    )
  )
)

(type + Page (is Box)
  (prop + shown _ (= true))
  (content
    (<> This
      (<> Card
        (if #shown
          (<> TextBox (slot: icon) (value: "badge"))
        )
      )
    )
  )
)
```

```output
[[badge],no body]
[[no icon],no body]
[[badge],no body]
```

### An element whose slot is filled inside a live if may itself be in the arm of a live if.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let outer true)
  (let inner true)
  (blank)
  (<> Box (id: root)
    (if #outer
      (<> Card
        (if #inner
          (<> TextBox (slot: icon) (value: "badge"))
        )
      )
    )
  )
  (blank)
  (print (call dump root))
  (= inner false)
  (print (call dump root))
  (= outer false)
  (print (call dump root))
  (= outer true)
  (print (call dump root))
  (= inner true)
  (print (call dump root))
)

(fn-do + dump String
  (param box Box)
  (let parts (call (of List String)))
  (for (in child (. box children))
    (if (is child TextBox)
      (+= parts (. (as child TextBox) text))
      (else
        (+= parts (str "[" (call dump child) "]"))
      )
    )
  )
  (return (call (. "," join) parts))
)

(type + Card (is Box)
  (content
    (<> This
      (+<> Box (id: icon)
        (<> TextBox (value: "no icon"))
      )
      (+<> Box
        (<> TextBox (value: "no body"))
      )
    )
  )
)
```

```output
[[[badge],no body]]
[[[no icon],no body]]
[]
[[[no icon],no body]]
[[[badge],no body]]
```

### A slot filled inside an if in a keyed loop row follows the row's item.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let names (call (of List String) (list "a" "bb")))
  (blank)
  (<> Box (id: root)
    (for (in name #names (keyed-by name))
      (<> Card
        (if (== name "bb")
          (<> TextBox (slot: icon) (value: name))
        )
      )
    )
  )
  (blank)
  (print (call dump root))
  (+= names "cc")
  (print (call dump root))
)

(fn-do + dump String
  (param box Box)
  (let parts (call (of List String)))
  (for (in child (. box children))
    (if (is child TextBox)
      (+= parts (. (as child TextBox) text))
      (else
        (+= parts (str "[" (call dump child) "]"))
      )
    )
  )
  (return (call (. "," join) parts))
)

(type + Card (is Box)
  (content
    (<> This
      (+<> Box (id: icon)
        (<> TextBox (value: "no icon"))
      )
      (+<> Box
        (<> TextBox (value: "no body"))
      )
    )
  )
)
```

```output
[[[no icon],no body],[[bb],no body]]
[[[no icon],no body],[[bb],no body],[[no icon],no body]]
```

### A fill reads a local as the arm's condition narrows it.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let maybe-name (? String) "ann")
  (let none (? String) null)
  (blank)
  (<> Card (id: a)
    (if maybe-name
      (<> TextBox (slot: icon) (value: maybe-name))
    )
  )
  (<> Card (id: b)
    (if none
      (<> TextBox (slot: icon) (value: none))
    )
  )
  (blank)
  (print (call dump a))
  (print (call dump b))
)

(fn-do + dump String
  (param box Box)
  (let parts (call (of List String)))
  (for (in child (. box children))
    (if (is child TextBox)
      (+= parts (. (as child TextBox) text))
      (else
        (+= parts (str "[" (call dump child) "]"))
      )
    )
  )
  (return (call (. "," join) parts))
)

(type + Card (is Box)
  (content
    (<> This
      (+<> Box (id: icon)
        (<> TextBox (value: "no icon"))
      )
      (+<> Box
        (<> TextBox (value: "no body"))
      )
    )
  )
)
```

```output
[ann],no body
[no icon],no body
```

### A slot whose outlet sets attributes on its content gets them on content given inside an if.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let shown true)
  (blank)
  (<> Card (id: a)
    (if #shown
      (<> TextBox (slot: header) (value: "slot"))
    )
  )
  (<> Card (id: b)
    (<> TextBox (slot: header) (value: "slot"))
  )
  (blank)
  (print (call dump a))
  (print (call dump b))
  (= shown false)
  (print (call dump a))
  (= shown true)
  (print (call dump a))
)

(fn-do + dump String
  (param box Box)
  (let parts (call (of List String)))
  (for (in child (. box children))
    (if (is child TextBox)
      (+= parts (. (as child TextBox) text))
      (else
        (+= parts (str "[" (call dump child) "]"))
      )
    )
  )
  (return (call (. "," join) parts))
)

(type + Card (is Box)
  (content
    (<> This
      (+<> TextBox (id: header)
        (= text "overlaid")
      )
      (<> TextBox (value: "end"))
    )
  )
)
```

```output
[overlaid],end
overlaid,end
[],end
[overlaid],end
```

### A subclass that leaves a slot alone passes on what its caller fills inside an if.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let shown true)
  (blank)
  (<> Sub (id: a)
    (if #shown
      (<> TextBox (slot: icon) (value: "badge"))
    )
  )
  (blank)
  (print (call dump a))
  (= shown false)
  (print (call dump a))
)

(fn-do + dump String
  (param box Box)
  (let parts (call (of List String)))
  (for (in child (. box children))
    (if (is child TextBox)
      (+= parts (. (as child TextBox) text))
      (else
        (+= parts (str "[" (call dump child) "]"))
      )
    )
  )
  (return (call (. "," join) parts))
)

(type + Card (is Box)
  (content
    (<> This
      (+<> Box (id: icon)
        (<> TextBox (value: "no icon"))
      )
      (+<> Box
        (<> TextBox (value: "no body"))
      )
    )
  )
)

(type + Sub (is Card)
  (content
    (<> TextBox (value: "sub body"))
  )
)
```

```output
[badge],sub body
[no icon],sub body
```

### An if that fills a slot tests a condition, not a pattern.

```brix crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let pick (? Int) 3)
  (blank)
  (<> Card (id: a)
    (if (= (| (: n Int)) pick)
      (<> TextBox (slot: icon) (value: (str "num " n)))
    )
  )
  (blank)
  (print (call dump a))
)

(fn-do + dump String
  (param box Box)
  (let parts (call (of List String)))
  (for (in child (. box children))
    (if (is child TextBox)
      (+= parts (. (as child TextBox) text))
      (else
        (+= parts (str "[" (call dump child) "]"))
      )
    )
  )
  (return (call (. "," join) parts))
)

(type + Card (is Box)
  (content
    (<> This
      (+<> Box (id: icon)
        (<> TextBox (value: "no icon"))
      )
      (+<> Box
        (<> TextBox (value: "no body"))
      )
    )
  )
)
```

```output
Exception: An if that fills a slot tests a condition, not a pattern; write is or == instead
<no file:9>
        (if (= (| (: n Int)) pick)
                  ^^^^^^^^^
```

### A subclass does not fill a slot its caller fills inside an if.

```brix crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let shown true)
  (blank)
  (<> Sub (id: a)
    (if #shown
      (<> TextBox (slot: icon) (value: "badge"))
    )
  )
  (<> Sub (id: plain))
  (blank)
  (print (call dump a))
  (print (call dump plain))
  (= shown false)
  (print (call dump a))
)

(fn-do + dump String
  (param box Box)
  (let parts (call (of List String)))
  (for (in child (. box children))
    (if (is child TextBox)
      (+= parts (. (as child TextBox) text))
      (else
        (+= parts (str "[" (call dump child) "]"))
      )
    )
  )
  (return (call (. "," join) parts))
)

(type + Card (is Box)
  (content
    (<> This
      (+<> Box (id: icon)
        (<> TextBox (value: "no icon"))
      )
      (+<> Box
        (<> TextBox (value: "no body"))
      )
    )
  )
)

(type + Sub (is Card)
  (content
    (<> TextBox (slot: icon) (value: "sub icon"))
    (<> TextBox (value: "sub body"))
  )
)
```

```output
Exception: Slot "icon" is filled inside an if or match where this object is made, so the class's content does not fill it too
<no file:50>
        (<> TextBox (slot: icon) (value: "sub icon"))
        ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A slot binding on a loop row is rejected.

```brix crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let names (call (of List String) (list "a" "b")))
  (blank)
  (<> Card (id: card)
    (for (in name #names (keyed-by name))
      (<> TextBox (slot: icon) (value: name))
    )
  )
)

(type + Card (is Box)
  (content
    (<> This
      (+<> Box (id: icon))
      (+<> Box)
    )
  )
)
```

```output
Exception: A slot is filled by a line of the element's body, or by a line of an arm of an if or match there
<no file:10>
          (<> TextBox (slot: icon) (value: name))
                             ^^^^
```

### An id on the only node supplied to a default slot names the node the slot mounts, as it does when the slot receives several nodes, and ids nested beneath it resolve the same way.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Box (id: demo)
    (<> Holder (id: one)
      (<> Box (id: sole)
        (<> Box (id: inner))
      )
    )
    (<> Holder (id: two)
      (<> Box (id: first))
      (<> Box (id: second))
    )
  )
  (blank)
  (print (str "sole is the slot node " (== (as (. one %first-child) (? Pointer)) (as sole (? Pointer)))))
  (print (str "inner is its child " (== (as (. sole %first-child) (? Pointer)) (as inner (? Pointer)))))
  (print (str "first is mounted " (!= (. first %parent) null)))
)

(type + Holder (is Box)
  (content
    (<> This
      (+<> Box)
    )
  )
)
```

```output
sole is the slot node true
inner is its child true
first is mounted true
```

### A class subscription to an event on the only node supplied to a default slot receives the event, as it does for a direct child and for one of several slot nodes.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo))
  (blank)
  (<- (. demo direct pinged) 1)
  (<- (. demo multi pinged) 1)
  (<- (. demo single pinged) 1)
  (print (str "direct=" (. demo direct-pings) " multi=" (. demo multi-pings) " single=" (. demo single-pings)))
  (print (str "single is the slot node " (== (as (. demo holder %first-child) (? Pointer)) (as (. demo single) (? Pointer)))))
)

(type + Holder (is Box)
  (content
    (<> This
      (+<> Box)
    )
  )
)

(type + Pinger (is Box)
  (event + pinged Int)
)

(type + Demo (is Box)
  (prop + direct-pings _ (= 0))
  (prop + multi-pings _ (= 0))
  (prop + single-pings _ (= 0))
  (content
    (<> This
      (<> Pinger (id: direct))
      (<> Holder
        (<> Pinger (id: multi))
        (<> Box)
      )
      (<> Holder (id: holder)
        (<> Pinger (id: single))
      )
    )
  )
  (subscribe-for (on event (. direct pinged))
    (+= direct-pings 1)
  )
  (subscribe-for (on event (. multi pinged))
    (+= multi-pings 1)
  )
  (subscribe-for (on event (. single pinged))
    (+= single-pings 1)
  )
)
```

```output
direct=1 multi=1 single=1
single is the slot node true
```

### An id on a slot host supplied through nested slot content binds from inside the inner slot builder, both as a class property and as a module-level name.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo))
  (print (str "class name=" (. demo inner name) " mounted=" (!= (. demo inner %parent) null)))
  (blank)
  (<> Box (id: root)
    (<> Wrap
      (<> Wrap
        (<> Host (id: local)
          (= name "local")
        )
      )
    )
  )
  (blank)
  (print (str "module name=" (. local name) " mounted=" (!= (. local %parent) null)))
)

(type + Demo (is Box)
  (prop + inner Host)
  (content
    (<> This
      (<> Wrap
        (<> Wrap
          (<> Host (id: inner)
            (= name "bound")
          )
        )
      )
    )
  )
)

(type + Wrap (is Box)
  (content
    (<> This
      (+<> Box)
    )
  )
)

(type + Host (is Box)
  (prop + name _ (= ""))
  (content
    (<> This
      (+<> Box (id: extra))
    )
  )
)
```

```output
class name=bound mounted=true
module name=local mounted=true
```

### An id on the only node supplied to a default slot binds the owning class property when the slot host sits inside another element, whether or not the node is itself a slot host.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo))
  (print (str "host name=" (. demo inner name) " mounted=" (!= (. demo inner %parent) null)))
  (print (str "plain name=" (. demo plain name) " mounted=" (!= (. demo plain %parent) null)))
)

(type + Demo (is Box)
  (prop + inner Host)
  (prop + plain Plain)
  (content
    (<> This
      (<> Box
        (<> Wrap
          (<> Host (id: inner)
            (= name "bound")
          )
        )
      )
      (<> Box
        (<> Wrap
          (<> Plain (id: plain)
            (= name "bound")
          )
        )
      )
    )
  )
)

(type + Wrap (is Box)
  (content
    (<> This
      (+<> Box)
    )
  )
)

(type + Host (is Box)
  (prop + name _ (= ""))
  (content
    (<> This
      (+<> Box (id: extra))
    )
  )
)

(type + Plain (is Box)
  (prop + name _ (= ""))
)
```

```output
host name=bound mounted=true
plain name=bound mounted=true
```

### Supplied slot content reads its slot host by the host's markup id, in named and default slots. The id is bound before the host builds its content, so an untracked read sees the host and a tracked read follows it.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo))
  (print (call pane-labels demo))
  (= (. demo named title) "two")
  (= (. demo plain title) "two")
  (print (call pane-labels demo))
)

(type + Demo (is Box)
  (content
    (<> This
      (<> NamedHolder (id: named)
        (<> Pane (id: named-pane) (slot: detail)
          (= tracked (. named #title))
          (= untracked (. named title))
        )
      )
      (<> PlainHolder (id: plain)
        (<> Pane (id: plain-pane)
          (= tracked (. plain #title))
          (= untracked (. plain title))
        )
      )
    )
  )
)

(type + NamedHolder (is Box)
  (prop + title _ (= "one"))
  (content
    (<> This
      (+<> Box (id: detail))
    )
  )
)

(type + PlainHolder (is Box)
  (prop + title _ (= "one"))
  (content
    (<> This
      (+<> Box)
    )
  )
)

(type + Pane (is Box)
  (prop + tracked _ (= ""))
  (prop + untracked _ (= ""))
)

(fn-do + pane-labels String
  (param demo Demo)
  (let named (. demo named-pane))
  (let plain (. demo plain-pane))
  (return (str "named=" (. named tracked) "/" (. named untracked) " plain=" (. plain tracked) "/" (. plain untracked)))
)
```

```output
named=one/one plain=one/one
named=two/one plain=two/one
```

### A slot outlet nested inside an element of a generic host's content receives content that reads a computed field of the host by the host's markup id, including when the host is itself supplied as slot content.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo))
  (print (str (?? (. demo direct-pane shown) "none") " " (?? (. demo nested-pane shown) "none")))
  (= (. demo direct title) "two")
  (= (. demo nested title) "two")
  (print (str (?? (. demo direct-pane shown) "none") " " (?? (. demo nested-pane shown) "none")))
)

(type + Demo (is Box)
  (content
    (<> This
      (<> (of Holder String) (id: direct)
        (= seed "x")
        (blank)
        (<> Pane (id: direct-pane) (slot: detail)
          (= shown (. direct #shown))
        )
      )
      (<> Wrap
        (<> (of Holder String) (id: nested)
          (<> Pane (id: nested-pane) (slot: detail)
            (= shown (. nested #shown))
          )
        )
      )
    )
  )
)

(type + (of Holder Item) (is Box)
  (prop + seed (? Item))
  (prop + title _ (= "one"))
  (prop + shown (? String) (= #title))
  (content
    (<> This
      (<> Box (id: detail-pane)
        (+<> Box (id: detail))
      )
    )
  )
)

(type + Wrap (is Box)
  (content
    (<> This
      (+<> Box)
    )
  )
)

(type + Pane (is Box)
  (prop + shown (? String))
)
```

```output
one one
two two
```

### Slot content that reads its slot host through an explicitly declared id property sees the host the markup constructs, not the property's default value.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo))
  (print (. demo pane label))
  (print (== (as (. demo pane holder) Pointer) (as (. demo columns) Pointer)))
)

(type + Demo (is Box)
  (prop + columns Holder)
  (prop + pane Pane)
  (content
    (<> This
      (<> Holder (id: columns)
        (= title "markup")
        (blank)
        (<> Pane (id: pane) (slot: detail)
          (= label (. columns title))
          (= holder columns)
        )
      )
    )
  )
)

(type + Holder (is Box)
  (prop + title _ (= "default"))
  (content
    (<> This
      (+<> Box (id: detail))
    )
  )
)

(type + Pane (is Box)
  (prop + label _ (= ""))
  (prop + holder (? Holder))
)
```

```output
markup
true
```

### An element's own property value cannot read the element by its markup id.

```brix compile-error
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo))
)

(type + Demo (is Box)
  (content
    (<> This
      (<> Holder (id: columns)
        (= label (. columns title))
      )
    )
  )
)

(type + Holder (is Box)
  (prop + title _ (= "one"))
  (prop + label _ (= ""))
)
```

```output
Exception: Named markup element "columns" is read before its markup constructs it
<no file:13>
            (= label (. columns title))
                        ^^^^^^^
```

### Supplied slot content in module-level markup reads its slot host by the host's markup id, in named and default slots. The id is bound before the host builds its content, so an untracked read sees the host and a tracked read follows it.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Box (id: root)
    (<> NamedHolder (id: named)
      (<> Pane (id: named-pane) (slot: detail)
        (= tracked (. named #title))
        (= untracked (. named title))
      )
    )
    (<> PlainHolder (id: plain)
      (<> Pane (id: plain-pane)
        (= tracked (. plain #title))
        (= untracked (. plain title))
      )
    )
  )
  (blank)
  (print (call pane-labels named-pane plain-pane))
  (= (. named title) "two")
  (= (. plain title) "two")
  (print (call pane-labels named-pane plain-pane))
)

(type + NamedHolder (is Box)
  (prop + title _ (= "one"))
  (content
    (<> This
      (+<> Box (id: detail))
    )
  )
)

(type + PlainHolder (is Box)
  (prop + title _ (= "one"))
  (content
    (<> This
      (+<> Box)
    )
  )
)

(type + Pane (is Box)
  (prop + tracked _ (= ""))
  (prop + untracked _ (= ""))
)

(fn-do + pane-labels String
  (param named Pane)
  (param plain Pane)
  (return (str "named=" (. named tracked) "/" (. named untracked) " plain=" (. plain tracked) "/" (. plain untracked)))
)
```

```output
named=one/one plain=one/one
named=two/one plain=two/one
```

### Supplied slot content in function-level markup reads its slot host by the host's markup id, and a closure in that content reads the host when it runs.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (call show "one")
)

(fn-do + show _
  (param first String)
  (<> Box (id: root)
    (<> Holder (id: columns)
      (= title first)
      (blank)
      (<> Pane (id: pane) (slot: detail)
        (= label (. columns #title))
        (= action (=> () :do (+= (. columns hits) 1)))
      )
    )
  )
  (blank)
  (print (. pane label))
  (= (. columns title) "two")
  (print (. pane label))
  (let action (. pane action))
  (if action
    (call action)
  )
  (print (. columns hits))
)

(type + Holder (is Box)
  (prop + title _ (= ""))
  (prop + hits _ (= 0))
  (content
    (<> This
      (+<> Box (id: detail))
    )
  )
)

(type + Pane (is Box)
  (prop + label _ (= ""))
  (prop + action (? Doer))
)
```

```output
one
two
1
```

### A slot host that is itself supplied as slot content outside class content is read by its markup id from its own slot content.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Box (id: root)
    (<> Wrap
      (<> Holder (id: columns)
        (<> Pane (id: pane) (slot: detail)
          (= label (. columns #title))
        )
      )
    )
  )
  (blank)
  (print (. pane label))
  (= (. columns title) "two")
  (print (. pane label))
)

(type + Wrap (is Box)
  (content
    (<> This
      (+<> Box)
    )
  )
)

(type + Holder (is Box)
  (prop + title _ (= "one"))
  (content
    (<> This
      (+<> Box (id: detail))
    )
  )
)

(type + Pane (is Box)
  (prop + label _ (= ""))
)
```

```output
one
two
```

### Outside class content, a slot host's own property value cannot read the host by its markup id.

```brix compile-error
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Box (id: root)
    (<> Holder (id: columns)
      (= label (. columns title))
      (blank)
      (<> Pane (slot: detail))
    )
  )
)

(type + Holder (is Box)
  (prop + title _ (= "one"))
  (prop + label _ (= ""))
  (content
    (<> This
      (+<> Box (id: detail))
    )
  )
)

(type + Pane (is Box))
```

```output
Exception: Named markup element "columns" is read before its markup constructs it
<no file:8>
          (= label (. columns title))
                      ^^^^^^^
```

### Supplied slot content in a method's markup reads its slot host by the host's markup id, which is not a property of the method's class.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo))
  (print (call (. demo show)))
)

(type + Demo (is Box)
  (fn-do + show String
    (<> Box (id: outer)
      (<> Holder (id: columns)
        (<> Pane (id: pane) (slot: detail)
          (= label (. columns #title))
        )
      )
    )
    (blank)
    (= (. columns title) "two")
    (return (. pane label))
  )
)

(type + Holder (is Box)
  (prop + title _ (= "one"))
  (content
    (<> This
      (+<> Box (id: detail))
    )
  )
)

(type + Pane (is Box)
  (prop + label _ (= ""))
)
```

```output
two
```

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> NestedHost (id: host)
    (<> NestedLeaf (slot: row) (param: row)
      (= address (. row #address))
    )
  )
  (blank)
  (let opening (call leaves host))
  (print (call addresses opening))
  (blank)
  (call (. host panel rebind) 0 10 100 "x")
  (call (. host panel markup-batch-queue checkpoint))
  (blank)
  (let closing (call leaves host))
  (print (call addresses closing))
  (print (and (== (. opening length) (. closing length)) (== ([] opening 0) ([] closing 0)) (== ([] opening 1) ([] closing 1))))
)

(type + NestedRow (is Box)
  (prop + address I64 (= 0))
)

(type + NestedLeaf (is Box)
  (prop + address I64 (= 0))
)

(type + NestedHost (is Box)
  (content
    (<> This
      (<> (of RebindPanel String) (id: panel)
        (= items (call (of List String) (list "a" "b")))
        (<> NestedRow (id: host-row) (slot: row) (param: slot)
          (= address (. slot #address))
          (blank)
          (+<> Box (id: row) (param: host-row NestedRow))
        )
      )
    )
  )
)

(fn-do + leaves (list NestedLeaf)
  (param box Box)
  (let found (list NestedLeaf) (call (of List NestedLeaf)))
  (call collect-leaves box found)
  (return found)
)

(fn-do + collect-leaves _
  (param box Box)
  (param found (list NestedLeaf))
  (if (is box NestedLeaf)
    (+= found (as box NestedLeaf))
  )
  (for (in child (. box children))
    (call collect-leaves child found)
  )
)

(fn-expr + addresses String
  (param found (list NestedLeaf))
  (call (. "," join) (in (str (. each address)) found))
)
```

```output
0,1
10,1
true
```

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(import ./lib/panel)
(blank)

(main
  (<> Panel (id: panel)
    (<> Box)
    (<> Box)
  )
  (blank)
  (print (call size panel))
)

(fn-do + size Int
  (param box Box)
  (let total 1)
  (for (in child (. box children))
    (+= total (call size child))
  )
  (return total)
)
```

```brix file=lib/panel.bx
(import /core/markup)
(import /core/tests/support/markup-fixtures)

(type + Panel (is Box)
  (content
    (<> This
      (<> PanelFrame
        (+<> Box)
      )
    )
  )
)

(type _ PanelFrame (is Box))
```

```output
5
```

### A closure an attribute of an element whose class has slots gives reads the name of a markup element of its body, made after the closure, as the receiver's property when it runs.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let panel (call Panel))
  (let holder (as (!! (. panel %first-child)) Holder))
  (call (. holder fire))
  (print (. panel seen))
  (= (. panel later title) "changed")
  (call (. holder fire))
  (print (. panel seen))
)

(type + Item (is Box)
  (prop + title _ (= ""))
  (fn-do + This _
    (param title _ (= ""))
  )
)

(type + Holder (is Box)
  (prop + run (? Doer))
  (content
    (<> This
      (+<> Box (id: trailing))
    )
  )
  (fn-do + fire _
    (let callback run)
    (if callback
      (call callback)
    )
  )
)

(type + Panel (is Box)
  (prop + seen _ (= ""))
  (content
    (<> This
      (<> Holder
        (= run (=> () :do (call note (. later title))))
        (<> Item (id: later) (text: "b"))
      )
    )
  )
  (fn-do + note _
    (param text String)
    (= seen text)
  )
)
```

```output
b
changed
```

### The lines of an element's body that give its class's default slot are built into a node of the slot's type of their own, though the body also gives an attribute and another slot content.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let panel (call Panel))
  (print (call describe panel))
  (print (. (as (!! (. panel %first-child)) Frame) size))
  (= (. panel n) 5)
  (print (call describe panel))
  (print (. (as (!! (. panel %first-child)) Frame) size))
)

(fn-do + describe String
  (param node Box)
  (let text "")
  (if (is node TextBox)
    (= text (. (as node TextBox) text))
  )
  (let child (. node %first-child))
  (if child
    (= text (++ text "["))
    (while child
      (= text (++ text (call describe child) " "))
      (= child (. child %next-sibling))
    )
    (= text (++ text "]"))
  )
  (return text)
)

(type + Frame (is Box)
  (prop + size _ (= 0))
  (content
    (<> This
      (+<> Box (id: header))
      (+<> Box)
    )
  )
)

(type + Panel (is Box)
  (prop + n _ (= 2))
  (content
    (<> This
      (<> Frame
        (= size #n)
        (<> TextBox (text: "a"))
        (<> TextBox (text: (str "b " #n)))
        (<> TextBox (slot: header) (text: (str "h " #n)))
      )
    )
  )
)
```

```output
[[h 2 [a b 2 ] ] ]
2
[[h 5 [a b 5 ] ] ]
5
```

### The name of a slot outlet written in a nested element body holds the node it mounts for the lines after it.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Panel (id: panel)
    (<> TextBox (slot: header) (value: "H"))
  )
  (blank)
  (for (in child (. (as (!! (. panel %first-child)) Box) children))
    (print (. (as child TextBox) text))
  )
)

(type + Panel (is Box)
  (content
    (<> This
      (<> Box
        (+<> TextBox (id: header))
        (<> TextBox (value: (str "n" (. header text))))
      )
    )
  )
)
```

```output
H
nH
```

### A class whose markup gives the default slot of the class above several lines builds them into one node, and a line it binds to another slot fills that slot.

```brix
(main
  (let panel (call Fancy))
  (call show panel "")
)

(fn-do + show _
  (param node MarkNode)
  (param indent String)
  (if (is node MarkText)
    (print (str indent (. (as node MarkText) text)))
    (else
      (print (str indent "node"))
    )
  )
  (for (in child (. node children))
    (call show child (str indent "  "))
  )
)

(type + MarkNode
  (implements (of MarkupParent MarkNode))
  (prop + parent (? MarkNode))
  (prop + first-child (? MarkNode))
  (prop + last-child (? MarkNode))
  (prop + previous (? MarkNode))
  (prop + next (? MarkNode))
  (fn-do + children _ :get
    (let maybe-child first-child)
    (if maybe-child
      (let child maybe-child)
      (while true
        (yield child)
        (if (not (. child next))
          (break)
        )
        (= child (. child next))
      )
    )
  )
  (fn-do + child-at (? MarkNode)
    (param index Index)
    (let cursor Index 0)
    (for (in child children)
      (if (== cursor index)
        (return child)
      )
      (+= cursor 1)
    )
  )
  (fn-expr + markup-content MarkNode
    (param child MarkNode)
    child
  )
  (fn-do + markup-attach-child _
    (param child MarkNode)
    (param anchor (? MarkNode))
    (= (. child parent) this)
    (if (not first-child)
      (= first-child (= last-child child))
      (else
        (= (?. last-child next) child)
        (= (. child previous) last-child)
        (= last-child child)
      )
    )
  )
)

(type + MarkText (is MarkNode)
  (prop + text String)
  (fn-do + This _
    (param text String)
  )
)

(type + Panel (is MarkNode)
  (content
    (+<> MarkText (id: header)
      (<> MarkText (value: "head"))
    )
    (+<> MarkNode)
    (+<> MarkText (id: footer)
      (<> MarkText (value: "foot"))
    )
  )
)

(type + Fancy (is Panel)
  (content
    (<> This
      (<> MarkText (value: "one"))
      (<> MarkText (value: "two"))
      (<> MarkText (slot: footer) (value: "fancy foot"))
    )
  )
)
```

```output
node
  head
  node
    one
    two
  fancy foot
```

### A named element among the lines a subclass gives its base's default slot binds the property while its own slot content reads it.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo))
  (print (call row-texts (. demo panel)))
  (print (call (. demo size)))
)

(type + RowData
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(type + (of RowPanel Item) (is Box)
  (prop + items (list Item) (= (call (of List Item))))
  (content
    (<> This
      (for (in item #items)
        (<> Box
          (+<> Box (id: row) (param: item Item))
        )
      )
    )
  )
)

(type + Wrap (is Box)
  (content
    (<> This
      (+<> Box)
    )
  )
)

(fn-do + row-texts String
  (param panel Box)
  (let parts (list String) (call (of List String)))
  (let region (. panel %first-child))
  (if region
    (for (in child (. region children))
      (+= parts (. (as (!! (. child %first-child)) TextBox) text))
    )
  )
  (return (call (. "," join) parts))
)

(type + Base (is Box)
  (content
    (<> This
      (+<> Box)
    )
  )
)

(type + Demo (is Base)
  (prop + items (list RowData) (= (list (call RowData "a") (call RowData "b"))))
  (prop + names (list String) (= (list "x")))
  (content
    (<> This
      (<> (of RowPanel RowData) (id: panel)
        (= items #items)
        (<> TextBox (slot: row) (param: item) (value: (str (. item text) (. panel items length))))
      )
      (for (in name #names (keyed-by name))
        (<> TextBox
          (= text name)
        )
      )
    )
  )
  (fn-do + size Size
    (return (. panel items length))
  )
)
```

```output
a2,b2
2
```

### An element whose class has slots but whose body gives none is made as any other: a named element among its lines is made where the block starts, so an attribute of the host reads it.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo))
  (print (. demo button is-on))
  (<- (. demo button clicked) 1)
  (do-new
    (print (. demo button is-on))
  )
)

(type + Pop (is Box)
  (prop + is-open Bool (= false))
  (fn-do + toggle _
    (= is-open (not is-open))
  )
)

(type + Btn (is Box)
  (prop + is-on Bool (= false))
  (event + clicked Int)
  (content
    (<> This
      (<> Box
        (+<> Box (id: trailing))
      )
    )
  )
)

(type + Demo (is Box)
  (content
    (<> This
      (<> Btn (id: button)
        (= is-on (. pop #is-open))
        (for (on n clicked)
          (call (. pop toggle))
        )
        (<> Pop (id: pop))
      )
    )
  )
)
```

```output
false
true
```

### Content a repeated slot gives that holds an outlet of its class's own slot, whose fallback reads the item tracked: where the caller gives that slot, the fallback is not built and reads nothing.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Box (id: panel)
    (<> View (id: view)
      (<> Bar (slot: content) (param: item-box)
        (= item (. item-box #item))
      )
    )
  )
  (print (call found panel))
  (print (call bars panel))
)

(type + Row (is Tuple)
  (prop + id String)
  (prop + selected Bool)
)

(type + ItemBox (is Box)
  (prop + item Row (= (call Row "" false)))
)

(type + LaneBox (is Box)
  (prop + items (list Row) (= (call (of List Row))))
  (content
    (<> This
      (for (in item #items (keyed-by (. item id)))
        (<> ItemBox (id: item-box)
          (= item item)
          (+<> Box (id: content) (param: item-box ItemBox))
        )
      )
    )
  )
)

(type + Body (is Box)
  (prop + is-selected Bool (= false))
)

(type + View (is Box)
  (prop + lanes (list (list Row)) (= (list (list (call Row "a" true) (call Row "b" false)))))
  (content
    (<> This
      (for (in lane #lanes)
        (<> LaneBox
          (= items lane)
          (<> Box (slot: content) (param: item-box)
            (+<> Box (id: content) (param: item-box ItemBox)
              (<> Body
                (= is-selected (. item-box #item selected))
              )
            )
          )
        )
      )
    )
  )
)

(type + Bar (is Box)
  (prop + item Row (= (call Row "" false)))
)

(fn-do + bars String
  (param box Box)
  (let parts (list String) (call (of List String)))
  (for (in child (. box children))
    (if (is child Bar)
      (+= parts (. (as child Bar) item id))
    )
    (let inner (call bars child))
    (if (!= (. inner size) 0)
      (+= parts inner)
    )
  )
  (return (call (. "," join) parts))
)

(fn-do + found String
  (param box Box)
  (let parts (list String) (call (of List String)))
  (for (in child (. box children))
    (if (is child Body)
      (+= parts (str (. (as child Body) is-selected)))
    )
    (let inner (call found child))
    (if (!= (. inner size) 0)
      (+= parts inner)
    )
  )
  (return (call (. "," join) parts))
)
```

```output
a,b
```

### A class whose markup holds the default slot outlet does not change how an element of a class of the same name in another module takes the lines of its body.

```brix
(import ./lib/outlet)
(import ./lib/plain)
(blank)

(main
  (print (call outlet-size))
  (print (call plain-size))
)
```

```brix file=lib/outlet.bx
(import /core/markup)
(import /core/tests/support/markup-fixtures)

(fn-do + outlet-size Int
  (<> Panel (id: panel))
  (return (call size panel))
)

(fn-do + size Int
  (param box Box)
  (let total 1)
  (for (in child (. box children))
    (+= total (call size child))
  )
  (return total)
)

(type _ Panel (is Box)
  (content
    (<> This
      (<> Box
        (+<> Box)
      )
    )
  )
)
```

```brix file=lib/plain.bx
(import /core/markup)
(import /core/tests/support/markup-fixtures)

(fn-do + plain-size Int
  (<> Panel (id: panel)
    (<> Box)
    (<> Box)
  )
  (return (call size panel))
)

(fn-do + size Int
  (param box Box)
  (let total 1)
  (for (in child (. box children))
    (+= total (call size child))
  )
  (return total)
)

(type _ Panel (is Box))
```

```output
2
3
```

### A named slot outlet whose body holds only attributes and no default content mounts nothing when its slot is not supplied, and mounts the supplied node when it is.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Panel (id: bare))
  (<> Panel (id: full)
    (<> TextBox (slot: footer) (value: "foot"))
  )
  (print (str "bare=" (== (. bare %first-child) null) " full=" (. (as (!! (. full %first-child)) TextBox) text)))
)

(type + Panel (is Box)
  (content
    (<> This
      (+<> Box (id: footer)
        (= flex 1)
      )
    )
  )
)
```

```output
bare=true full=foot
```

### A named slot outlet whose body holds only an attribute that reads a tracked property mounts nothing when its slot is not supplied, and gives the supplied node the attribute again when the property changes.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Panel (id: bare))
  (<> Panel (id: full)
    (<> TextBox (slot: footer) (value: "foot"))
  )
  (print (str "bare=" (== (. bare %first-child) null) " full=" (. (as (!! (. full %first-child)) TextBox) text)))
  (= (. full is-wide) true)
  (print (str "full=" (. (as (!! (. full %first-child)) TextBox) text)))
)

(type + Panel (is Box)
  (prop + is-wide _ (= false))
  (content
    (<> This
      (+<> TextBox (id: footer)
        (= text (if-expr #is-wide "wide" "narrow"))
      )
    )
  )
)
```

```output
bare=true full=narrow
full=wide
```

### The one line of an element's body that gives its class's default slot content may be a value rather than an element: the value is converted to the slot's type.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> CaptionHost (id: first)
    (call Label "plain")
  )
  (<> CaptionHost (id: second)
    (call Plain "converted")
  )
  (print (. (!! (. first caption)) text))
  (print (. (!! (. second caption)) text))
)

(type + Plain
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
  (fn-to-expr + Label
    (call Label text)
  )
)

(type + CaptionHost (is Box)
  (prop + caption (? Label))
  (content
    (+<> Label)
  )
  (fn-do + markup-absorb _
    (param content Label)
    (= caption content)
  )
)
```

```output
plain
converted
```

### Default slot content that a `use` lends is converted to the slot's type before the `use` ends.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> CaptionHost (id: host)
    (use (call Lender))
  )
  (print (. (!! (. host caption)) text))
)

(type + Lender
  (prop + open _ (= false))
  (fn-do + begin-using _
    (= open true)
    (return this)
  )
  (fn-do + end-using _
    (= open false)
    (print "end-using")
  )
  (fn-to-expr + Label
    (call Label (if-expr open "open" "closed"))
  )
)

(type + CaptionHost (is Box)
  (prop + caption (? Label))
  (content
    (+<> Label)
  )
  (fn-do + markup-absorb _
    (param content Label)
    (= caption content)
  )
)
```

```output
end-using
open
```

### A slot host named under an `if` line binds the owning class property before it builds its content, so the content reads the host by its markup id, as it does outside the `if`.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo))
  (print (call (. demo label)))
)

(type + Demo (is Box)
  (prop + is-shown _ (= true))
  (content
    (<> This
      (if #is-shown
        (<> Holder (id: picked)
          (<> Pane (slot: detail)
            (= untracked (. picked title))
          )
        )
      )
    )
  )
  (fn-do + label String
    (let pane (as (!! (. picked %first-child)) Pane))
    (return (str (. picked title) " " (. pane untracked)))
  )
)

(type + Holder (is Box)
  (prop + title _ (= "one"))
  (content
    (<> This
      (+<> Box (id: detail))
    )
  )
)

(type + Pane (is Box)
  (prop + untracked _ (= ""))
)
```

```output
one one
```

### Demonstrates that a subclass of a class that forwards a named slot appends its own lines as ordinary child content.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> ForwardingSubPanel (id: panel)
    (<> TextBox (slot: extra) (text: "added"))
  )
  (blank)
  (print (call count-children panel))
  (print (call count-children (. panel holder)))
)

(fn-do + count-children Index
  (param box Box)
  (let found 0)
  (for (in child (. box children))
    (+= found 1)
  )
  (return found)
)

(type + ExtraHolder (is Box)
  (content
    (<> This
      (+<> Box (id: extra))
    )
  )
)

(type + ForwardingPanel (is Box)
  (content
    (<> This
      (<> ExtraHolder (id: holder)
        (<+ extra)
      )
    )
  )
)

(type + ForwardingSubPanel (is ForwardingPanel)
  (content
    (<> This
      (<> TextBox (text: "own"))
    )
  )
)
```

```output
2
1
```

### Demonstrates that forwarding a slot the component was not given gives the inner element nothing for that slot.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> ForwardingPanel (id: panel)
    (<> TextBox (text: "plain"))
  )
  (blank)
  (print (call count-children panel))
  (print (call count-children (. panel holder)))
  (let made (call ForwardingPanel))
  (print (call count-children (. made holder)))
)

(fn-do + count-children Index
  (param box Box)
  (let found 0)
  (for (in child (. box children))
    (+= found 1)
  )
  (return found)
)

(type + ExtraHolder (is Box)
  (content
    (<> This
      (+<> Box (id: extra))
    )
  )
)

(type + ForwardingPanel (is Box)
  (content
    (<> This
      (<> ExtraHolder (id: holder)
        (<+ extra)
      )
    )
  )
)
```

```output
2
0
0
```

### A named element of a slot's default content panics where it is read once a caller fills the slot.

```brix crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Holder (id: holder)
    (<> Box (slot: detail))
  )
  (print (. holder marker title))
)

(type + Holder (is Box)
  (content
    (<> This
      (+<> Box (id: detail)
        (<> Pane (id: marker))
      )
    )
  )
)

(type + Pane (is Box)
  (prop + title _ (= "one"))
)
```

```stderr
Unhandled panic: Named markup element "marker" is read before its markup constructs it
<no file:9>

Task 1

Stack trace:
  0  __source__:main no file:9:13
  1  __source__:init.__source__ no file
  2  __source__:brix.main no file
```

### Slot content cannot read an element it writes later: it makes its elements in order.

```brix compile-error
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Pair (id: pair))
)

(type + Holder (is Box)
  (prop + value _ (= false))
)

(type + Shelf (is Box)
  (content
    (<> This
      (+<> Box)
    )
  )
)

(type + Pair (is Shelf)
  (content
    (<> This
      (<> Holder (id: first)
        (= value (. second value))
      )
      (<> Holder (id: second)
        (= value true)
      )
    )
  )
)
```

```output
Exception: Named markup element "second" is read before its markup constructs it
<no file:25>
            (= value (. second value))
                        ^^^^^^
```

### A tracked read in slot content cannot read an element the content writes later.

```brix compile-error
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Pair (id: pair))
)

(type + Holder (is Box)
  (prop + value _ (= false))
)

(type + Shelf (is Box)
  (content
    (<> This
      (+<> Box)
    )
  )
)

(type + Pair (is Shelf)
  (content
    (<> This
      (<> Holder (id: first)
        (= value (. second #value))
      )
      (<> Holder (id: second)
        (= value true)
      )
    )
  )
  (fn-do + peek Bool
    (return (. second value))
  )
)
```

```output
Exception: Named markup element "second" is read before its markup constructs it
<no file:25>
            (= value (. second #value))
                        ^^^^^^
```

### Class content cannot read a slot host it writes above an element it names: the host is made where it is written, after the elements the block makes where it starts.

```brix compile-error
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo))
)

(type + Holder (is Box)
  (prop + value _ (= false))
)

(type + Shelf (is Box)
  (prop + value _ (= true))
  (content
    (<> This
      (+<> Box)
    )
  )
)

(type + Demo (is Box)
  (content
    (<> This
      (<> Shelf (id: shelf)
        (<> Box)
      )
      (<> Holder (id: holder)
        (= value (. shelf value))
      )
    )
  )
)
```

```output
Exception: Named markup element "shelf" is read before its markup constructs it
<no file:29>
            (= value (. shelf value))
                        ^^^^^
```
