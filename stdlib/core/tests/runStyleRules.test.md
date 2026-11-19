# Style rules

### Applies a no-parameter rule to the receiving element.

```brix
(main
  (<> Thing (id: thing) (• answer))
  (print (. thing x))
)

(type + Thing
  (prop + x _ (= 0))
  (blank)
  (rule answer
    (= x 42)
  )
)
```

```output
42
```

### Applies all property assignments from a single rule.

```brix
(main
  (<> Thing (id: thing) (• pair))
  (print (. thing x))
  (print (. thing text))
)

(type + Thing
  (prop + x _ (= 0))
  (prop + text _ (= ""))
  (blank)
  (rule pair
    (= x 7)
    (= text "seven")
  )
)
```

```output
7
seven
```

### Passes a typed argument to a parameterized rule.

```brix
(main
  (<> Thing (id: thing) (• set-x 42))
  (print (. thing x))
)

(type + Thing
  (prop + x _ (= 0))
  (blank)
  (rule set-x
    (param value Int)
    (= x value)
  )
)
```

```output
42
```

### Passes a single typed argument with colon shorthand.

```brix
(main
  (<> Thing (id: thing) (• set-x 42))
  (print (. thing x))
)

(type + Thing
  (prop + x _ (= 0))
  (blank)
  (rule set-x
    (param value Int)
    (= x value)
  )
)
```

```output
42
```

### Allows nested calls with whitespace inside colon shorthand arguments.

```brix
(main
  (<> Thing (id: thing) (• set-point (call Point 1 2)))
  (print (. thing x))
  (print (. thing y))
)

(type + Thing
  (prop + x _ (= 0))
  (prop + y _ (= 0))
  (blank)
  (rule set-point
    (param point Point)
    (= x (. point x))
    (= y (. point y))
  )
)

(type + Point (is (tuple (: x Int) (: y Int))))
```

```output
1
2
```

### Applies overloads by argument type.

```brix
(main
  (<> Thing (id: thing) (• set 42) (• set "hi"))
  (print (. thing x))
  (print (. thing text))
)

(type + Thing
  (prop + x _ (= 0))
  (prop + text _ (= ""))
  (blank)
  (rule set
    (param value Int)
    (= x value)
  )
  (blank)
  (rule set
    (param value String)
    (= text value)
  )
)
```

```output
42
hi
```

### Converts rule arguments through the parameter type's `from` constructor.

```brix
(main
  (<> Thing (id: thing) (• tint "red") (• maybe-tint "blue"))
  (print (. thing tint))
  (print (. thing maybe-tint))
)

(type + Thing
  (prop + tint _ (= (call Tint)))
  (prop + maybe-tint (? Tint))
  (blank)
  (rule tint
    (param value Tint)
    (= tint value)
  )
  (blank)
  (rule maybe-tint
    (param value (? Tint))
    (= maybe-tint value)
  )
)

(type + Tint (is Tuple)
  (prop + name _ (= "none"))
  (prop + converted _ (= false))
  (fn-from-do + (: value String)
    (return (call Tint value true))
  )
)
```

```output
(red, true)
(blue, true)
```

### Prefers a direct overload over one that converts the argument.

```brix
(main
  (<> Thing (id: thing) (• set "hi"))
  (print (. thing text))
  (print (. thing tint))
)

(type + Thing
  (prop + text _ (= ""))
  (prop + tint _ (= (call Tint)))
  (blank)
  (rule set
    (param value Tint)
    (= tint value)
  )
  (blank)
  (rule set
    (param value String)
    (= text value)
  )
)

(type + Tint (is Tuple)
  (prop + name _ (= "none"))
  (prop + converted _ (= false))
  (fn-from-do + (: value String)
    (return (call Tint value true))
  )
)
```

```output
hi
(none, false)
```

### Accepts a tuple parameter for rules that need multiple values.

```brix
(main
  (<> Thing (id: thing) (• set (tuple (+ 2 3) "five")))
  (print (. thing x))
  (print (. thing text))
)

(type + Thing
  (prop + x _ (= 0))
  (prop + text _ (= ""))
  (blank)
  (rule set
    (param value (tuple (: x Int) (: text String)))
    (= x (. value x))
    (= text (. value text))
  )
)
```

```output
5
five
```

### Applies rules left-to-right when they assign the same property.

```brix
(main
  (<> Thing (id: thing) (• set-x 1) (• set-x 2))
  (print (. thing x))
)

(type + Thing
  (prop + x _ (= 0))
  (blank)
  (rule set-x
    (param value Int)
    (= x value)
  )
)
```

```output
2
```

### Applies implicit type and id selectors before explicit selectors.

```brix
(main
  (<> Button (id: ok-button) (• special-button))
  (print (. ok-button text))
)

(type + Button
  (prop + text _ (= ""))
  (blank)
  (rule Button
    (= text (++ text "type"))
  )
  (blank)
  (rule ok-button
    (= text (++ text ":id"))
  )
  (blank)
  (rule special-button
    (= text (++ text ":bullet"))
  )
)
```

```output
type:id:bullet
```

### Ignores missing implicit type and id selectors.

```brix
(main
  (<> Button (id: plain-button))
  (print (. plain-button text))
)

(type + Button
  (prop + text _ (= "plain"))
)
```

```output
plain
```

### Uses the base type name as the implicit selector for generic markup types.

```brix
(main
  (<> (of Cell String) (id: cell))
  (print (. cell text))
)

(type + (of Cell Item)
  (prop + text _ (= ""))
  (blank)
  (rule Cell
    (= text "generic")
  )
)
```

```output
generic
```

### Lets explicit markup-body assignments override rule-expanded assignments.

```brix
(main
  (<> Thing (id: thing) (• set-x 1)
    (= x 2)
  )
  (blank)
  (print (. thing x))
)

(type + Thing
  (prop + x _ (= 0))
  (blank)
  (rule set-x
    (param value Int)
    (= x value)
  )
)
```

```output
2
```

### Lets explicit markup-body assignments override colon shorthand rule-expanded assignments.

```brix
(main
  (<> Thing (id: thing) (• set-x 1)
    (= x 2)
  )
  (blank)
  (print (. thing x))
)

(type + Thing
  (prop + x _ (= 0))
  (blank)
  (rule set-x
    (param value Int)
    (= x value)
  )
)
```

```output
2
```

### Allows later rules to see properties assigned by earlier rules.

```brix
(main
  (<> Thing (id: thing) (• set-x 3) (• double-x))
  (print (. thing x))
)

(type + Thing
  (prop + x _ (= 0))
  (blank)
  (rule set-x
    (param value Int)
    (= x value)
  )
  (blank)
  (rule double-x
    (= x (* x 2))
  )
)
```

```output
6
```

### Applies shorthand rules named on a style-rule header before the rule's own body.

```brix
(main
  (<> Thing (id: thing) (• button))
  (print (. thing x))
  (print (. thing y))
)

(type + Thing
  (prop + x _ (= 0))
  (prop + y _ (= 0))
  (blank)
  (rule base
    (= x 1)
  )
  (blank)
  (rule side
    (= y 2)
  )
  (blank)
  (rule button (• base) (• side)
    (= x 3)
  )
)
```

```output
3
2
```

### Applies shorthand rules inside a tracked state branch.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel) (• hot))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (. panel $ink))
  (print (. panel $tone))
  (blank)
  (= (. panel is-hot) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (. panel $ink))
  (print (. panel $tone))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
  (prop + tone I64 (= 0))
)

(type + Panel (is Box)
  (using TintStyle)
  (prop + is-hot _ (= false))
  (blank)
  (rule set-ink
    (param value I64)
    (= ink value)
  )
  (blank)
  (rule set-tone
    (= tone 4)
  )
  (blank)
  (rule hot
    (when-style #is-hot
      (• set-ink 2)
      (• set-tone)
      (= ink 3)
    )
  )
)
```

```output
0
0
3
4
```

### Shorthand rules inside one tracked state branch share its gated block.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel) (• hot))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-authored-snapshot panel))
  (blank)
  (= (. panel is-hot) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (. panel $ink))
  (print (. panel $tone))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
  (prop + tone I64 (= 0))
)

(type + Panel (is Box)
  (using TintStyle)
  (prop + is-hot _ (= false))
  (blank)
  (rule set-ink
    (param value I64)
    (= ink value)
  )
  (blank)
  (rule set-tone
    (= tone 4)
  )
  (blank)
  (rule hot
    (when-style #is-hot
      (• set-ink 2)
      (• set-tone)
      (= ink 3)
    )
  )
)
```

```output
node disabled-blocks=1
node TintStyle.ink layer=4 order=0 kind=static bits=2 block=0
node TintStyle.tone layer=4 order=1 kind=static bits=4 block=0
node TintStyle.ink layer=4 order=2 kind=static bits=3 block=0
3
4
```

### A nested state branch closes the enclosing gated block, which reopens for later declarations.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel) (• hot))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-authored-snapshot panel))
  (blank)
  (= (. panel is-hot) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (. panel $ink))
  (print (. panel $tone))
  (blank)
  (= (. panel is-loud) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (. panel $ink))
  (print (. panel $tone))
  (blank)
  (= (. panel is-hot) false)
  (call (. demo markup-batch-queue checkpoint))
  (print (. panel $ink))
  (print (. panel $tone))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
  (prop + tone I64 (= 0))
)

(type + Panel (is Box)
  (using TintStyle)
  (prop + is-hot _ (= false))
  (prop + is-loud _ (= false))
  (blank)
  (rule warm
    (= tone 5)
    (when-style #is-loud
      (= tone 6)
    )
  )
  (blank)
  (rule hot
    (when-style #is-hot
      (• warm)
      (= ink 3)
    )
  )
)
```

```output
node disabled-blocks=7
node TintStyle.tone layer=4 order=0 kind=static bits=5 block=0
node TintStyle.tone layer=4 order=1 kind=static bits=6 block=1
node TintStyle.ink layer=4 order=2 kind=static bits=3 block=2
3
5
3
6
0
0
```

### A class rule applied by type records the same declarations at every element, and later rules and inline declarations at an element keep their order after it.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: first))
    (<> Panel (id: second) (• tone 9)
      (= ink 8)
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-authored-snapshot first))
  (print (call style-authored-snapshot second))
  (print (. first $ink))
  (print (. second $tone))
  (blank)
  (= (. second is-hot) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (. first $ink))
  (print (. first $tone))
  (print (. second $ink))
  (print (. second $tone))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
  (prop + tone I64 (= 0))
)

(type + Panel (is Box)
  (using TintStyle)
  (prop + is-hot _ (= false))
  (blank)
  (rule tone
    (param value I64)
    (= tone value)
  )
  (blank)
  (rule Panel (• tone 1)
    (= ink 2)
    (when-style #is-hot
      (• tone 4)
      (= ink 3)
    )
  )
)
```

```output
node disabled-blocks=1
node TintStyle.tone layer=2 order=0 kind=static bits=1
node TintStyle.ink layer=2 order=1 kind=static bits=2
node TintStyle.tone layer=4 order=2 kind=static bits=4 block=0
node TintStyle.ink layer=4 order=3 kind=static bits=3 block=0
node disabled-blocks=1
node TintStyle.tone layer=2 order=0 kind=static bits=1
node TintStyle.ink layer=2 order=1 kind=static bits=2
node TintStyle.tone layer=4 order=2 kind=static bits=4 block=0
node TintStyle.ink layer=4 order=3 kind=static bits=3 block=0
node TintStyle.tone layer=2 order=4 kind=static bits=9
node TintStyle.ink layer=5 order=5 kind=static bits=8
2
9
2
1
8
4
```

### A class rule applied by type re-authors tracked declarations per element.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: first))
    (<> Panel (id: second)
      (= level 5)
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (. first $ink))
  (print (. first $tone))
  (print (. second $ink))
  (print (. second $tone))
  (blank)
  (= (. first level) 9)
  (= (. second is-hot) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (. first $ink))
  (print (. first $tone))
  (print (. second $ink))
  (print (. second $tone))
  (blank)
  (= (. second level) 20)
  (call (. demo markup-batch-queue checkpoint))
  (print (. first $tone))
  (print (. second $tone))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
  (prop + tone I64 (= 0))
)

(type + Panel (is Box)
  (using TintStyle)
  (prop + level I64 (= 1))
  (prop + is-hot _ (= false))
  (blank)
  (rule Panel
    (= ink 7)
    (= tone (+ #level 1))
    (when-style #is-hot
      (= tone (+ #level 100))
    )
  )
)
```

```output
7
2
7
6
7
10
7
105
10
120
```

### A class rule inherited by several element types applies to each of them.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Card (id: card))
    (<> Chip (id: chip))
    (<> Card (id: other))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-authored-snapshot card))
  (print (call style-authored-snapshot chip))
  (print (call style-authored-snapshot other))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
  (prop + tone I64 (= 0))
)

(type + Surface (is Box)
  (using TintStyle)
  (rule Surface
    (= ink 1)
    (= tone 2)
  )
)

(type + Card (is Surface)
  (rule Card
    (= ink 3)
    (= tone 4)
  )
)

(type + Chip (is Surface))
```

```output
node TintStyle.ink layer=2 order=0 kind=static bits=1
node TintStyle.tone layer=2 order=1 kind=static bits=2
node TintStyle.ink layer=2 order=2 kind=static bits=3
node TintStyle.tone layer=2 order=3 kind=static bits=4
node TintStyle.ink layer=2 order=0 kind=static bits=1
node TintStyle.tone layer=2 order=1 kind=static bits=2
node TintStyle.ink layer=2 order=0 kind=static bits=1
node TintStyle.tone layer=2 order=1 kind=static bits=2
node TintStyle.ink layer=2 order=2 kind=static bits=3
node TintStyle.tone layer=2 order=3 kind=static bits=4
```

### A class rule and its subclass's class rule declared in different modules each apply their own declarations to an element of the subclass.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(import ./lib/card)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Card (id: card))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-authored-snapshot card))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)
```

```brix file=lib/surface.bx
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
  (prop + tone I64 (= 0))
)

(type + Surface (is Box)
  (using TintStyle)
  (rule Surface
    (= ink 1)
    (= tone 2)
  )
)
```

```brix file=lib/card.bx
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(import ./surface)

(type + Card (is Surface)
  (rule Card
    (= ink 3)
    (= tone 4)
  )
)
```

```output
node TintStyle.ink layer=2 order=0 kind=static bits=1
node TintStyle.tone layer=2 order=1 kind=static bits=2
node TintStyle.ink layer=2 order=2 kind=static bits=3
node TintStyle.tone layer=2 order=3 kind=static bits=4
```

### A class rule applied by type resolves its property names through the styles visible at each element: elements under different styles record different properties, and an unrelated visible style changes nothing.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> WarmBox
      (<> Panel (id: warm))
    )
    (<> CoolBox
      (<> Panel (id: cool))
    )
    (<> WarmNoteBox
      (<> Panel (id: noted))
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-authored-snapshot warm))
  (print (call style-authored-snapshot cool))
  (print (call style-authored-snapshot noted))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + WarmStyle (is Style)
  (prop + ink I64 (= 0))
  (prop + tone I64 (= 0))
)

(type + CoolStyle (is Style)
  (prop + ink I64 (= 0))
  (prop + tone I64 (= 0))
)

(type + NoteStyle (is Style)
  (prop + note I64 (= 0))
)

(type + WarmBox (is Box)
  (using WarmStyle)
)

(type + CoolBox (is Box)
  (using CoolStyle)
)

(type + WarmNoteBox (is Box)
  (using NoteStyle)
  (using WarmStyle)
)

(type + Panel (is Box)
  (rule Panel
    (= ink 1)
    (= tone 2)
  )
)
```

```output
node WarmStyle.ink layer=2 order=0 kind=static bits=1
node WarmStyle.tone layer=2 order=1 kind=static bits=2
node CoolStyle.ink layer=2 order=0 kind=static bits=1
node CoolStyle.tone layer=2 order=1 kind=static bits=2
node WarmStyle.ink layer=2 order=0 kind=static bits=1
node WarmStyle.tone layer=2 order=1 kind=static bits=2
```

### A class rule applied by type layers a nested rule scoped by an enclosing element over the shorthand it applies, and elements outside that scope are unaffected.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: before))
    (<> Group (• blue)
      (<> Panel (id: scoped))
    )
    (<> Panel (id: after))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-authored-snapshot before))
  (print (call style-authored-snapshot scoped))
  (print (call style-authored-snapshot after))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
  (prop + tone I64 (= 0))
)

(type + Group (is Box)
  (using TintStyle)
  (rule blue
    (rule part
      (= ink 7)
    )
  )
)

(type + Panel (is Box)
  (using TintStyle)
  (rule part
    (= ink 1)
  )
  (blank)
  (rule Panel (• part)
    (= tone 2)
  )
)
```

```output
node TintStyle.ink layer=2 order=0 kind=static bits=1
node TintStyle.tone layer=2 order=1 kind=static bits=2
node TintStyle.ink layer=2 order=0 kind=static bits=1
node TintStyle.ink layer=2 order=1 kind=static bits=7
node TintStyle.tone layer=2 order=2 kind=static bits=2
node TintStyle.ink layer=2 order=0 kind=static bits=1
node TintStyle.tone layer=2 order=1 kind=static bits=2
```

### Gated shorthand rules named on style-rule headers inherit the outer condition.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (lazy selected false)
  (blank)
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel) (• button (if: #selected)))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-authored-snapshot panel))
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
  (blank)
  (style-domains ink (domain paint))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Panel (is Box)
  (using TintStyle)
  (rule base
    (= ink 7)
  )
  (blank)
  (rule button (• base)
    (= ink 9)
  )
)
```

```output
node disabled-blocks=3
node TintStyle.ink layer=4 order=0 kind=static bits=7 block=0
node TintStyle.ink layer=4 order=1 kind=static bits=9 block=1
```

### Header-composed rules contribute nested rule scopes to component content.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel) (• button))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (let part (!! (. panel %first-child)))
  (print (call style-authored-snapshot part))
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
  (blank)
  (style-domains ink (domain paint))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Panel (is Box)
  (using TintStyle)
  (rule This
    (<> Segment (• part))
  )
  (blank)
  (rule base
    (rule part
      (= ink 7)
    )
  )
  (blank)
  (rule button (• base))
)

(type + Segment (is Box)
  (using TintStyle)
  (rule part
    (= ink 1)
  )
)
```

```output
node TintStyle.ink layer=2 order=0 kind=static bits=1
node TintStyle.ink layer=2 order=1 kind=static bits=7
```

### Nested rule scopes preserve parent rule argument values.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (lazy shade 7)
  (blank)
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel) (• themed shade))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (let part (!! (. panel %first-child)))
  (print (call style-authored-snapshot part))
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
  (blank)
  (style-domains ink (domain paint))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Panel (is Box)
  (using TintStyle)
  (rule This
    (<> Segment (• part))
  )
  (blank)
  (rule themed
    (param value Int)
    (rule part
      (= ink value)
    )
  )
)

(type + Segment (is Box)
  (using TintStyle)
  (rule part
    (= ink 1)
  )
)
```

```output
node TintStyle.ink layer=2 order=0 kind=static bits=1
node TintStyle.ink layer=2 order=1 kind=static bits=7
```

### Rejects recursive shorthand rules named on style-rule headers.

```brix crash
(main
  (<> Thing (id: thing) (• a))
)

(type + Thing
  (prop + x _ (= 0))
  (blank)
  (rule a (• b)
    (= x 1)
  )
  (blank)
  (rule b (• a)
    (= x 2)
  )
)
```

```output
Exception: Recursive style rule application: a -> b -> a
<no file:13>
      (rule b (• a)
              ^^^^^
```

### Rejects a style rule that takes a value written with no type.

```brix crash
(main
  (<> Thing (id: thing) (• tint 3))
)

(type + Thing
  (prop + x _ (= 0))
  (blank)
  (rule tint
    (param dark _)
    (= x 1)
  )
)
```

```output
Exception: Argument type not found
<no file:10>
        (param dark _)
        ^^^^^^^^^^^^^^
```

### Rejects a style rule that takes a value whose type names nothing.

```brix crash
(main
  (<> Thing (id: thing) (• tint 3))
)

(type + Thing
  (prop + x _ (= 0))
  (blank)
  (rule tint
    (param value Dark)
    (= x 1)
  )
)
```

```output
Exception: Type "Dark" not found
<no file:10>
        (param value Dark)
                     ^^^^
```

### Rejects a style rule that takes a value written as a type with no name.

```brix crash
(main
  (<> Thing (id: thing) (• tint 3))
)

(type + Thing
  (prop + x _ (= 0))
  (blank)
  (rule tint
    (param _ I64)
    (= x 1)
  )
)
```

```output
Exception: Invalid argument
<no file:10>
        (param _ I64)
                 ^^^
```

### Rejects a nested style rule that takes a value written with no type.

```brix crash
(main
  (<> Panel (id: panel))
)

(type + Panel
  (rule This
    (rule part
      (rule tint
        (param dark _)
      )
    )
  )
  (content
    (<> This
      (<> Thing (• part) (• tint 3))
    )
  )
)

(type + Thing
  (prop + x _ (= 0))
  (blank)
  (rule part
    (= x 1)
  )
  (blank)
  (rule tint
    (param value I64)
    (= x value)
  )
)
```

```output
Exception: Argument type not found
<no file:10>
            (param dark _)
            ^^^^^^^^^^^^^^
```

### Makes a base-class rule available to subclass instances.

```brix
(main
  (<> Child (id: child) (• set-x 9))
  (print (. child x))
)

(type + Base
  (prop + x _ (= 0))
  (blank)
  (rule set-x
    (param value Int)
    (= x value)
  )
)

(type + Child (is Base))
```

```output
9
```

### Falls back to a base-class overload when a subclass rule has the wrong arity.

```brix
(main
  (<> Child (id: child) (• themed 9))
  (print (. child x))
)

(type + Base
  (prop + x _ (= 0))
  (blank)
  (rule themed
    (param value Int)
    (= x value)
  )
)

(type + Child (is Base)
  (rule themed
    (= x 1)
  )
)
```

```output
9
```

### Falls back to a base-class overload when a subclass rule has the wrong type.

```brix
(main
  (<> Child (id: child) (• themed 9))
  (print (. child x))
)

(type + Base
  (prop + x _ (= 0))
  (blank)
  (rule themed
    (param value Int)
    (= x value)
  )
)

(type + Child (is Base)
  (rule themed
    (param value String)
    (= x 1)
  )
)
```

```output
9
```

### Lets a subclass-scoped rule assign subclass-only properties.

```brix
(main
  (<> Child (id: child) (• label "kid"))
  (print (. child text))
)

(type + Base)

(type + Child (is Base)
  (prop + text _ (= ""))
  (blank)
  (rule label
    (param value String)
    (= text value)
  )
)
```

```output
kid
```

### Uses rules imported from another module.

```brix
(import ./rules)
(blank)

(main
  (<> Thing (id: thing) (• imported))
  (print (. thing x))
)
```

```brix file=rules.bx
(type + Thing
  (prop + x _ (= 0))
  (blank)
  (rule imported
    (= x 12)
  )
)
```

```output
12
```

### A class rule another module writes, applied by type at elements made repeatedly, declares and gates its styles for every object.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(import ./tiles)
(blank)

(main
  (<> BatchDemo (id: demo))
  (blank)
  (call (. demo fill))
  (call (. demo markup-batch-queue checkpoint))
  (= (. ([] (. demo made) 2) is-hot) true)
  (call (. demo markup-batch-queue checkpoint))
  (for (in tile (. demo made))
    (print (. tile $lift))
  )
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (prop + made (list Tile))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
  (fn-do + fill _
    (for (in index (..< 0 3))
      (<> Tile (id: tile))
      (call markup-append this tile)
      (+= made tile)
    )
  )
)
```

```brix file=tiles.bx
(import /core/style)
(import /core/tests/support/markup-fixtures)

(type + LiftStyle (is Style)
  (prop + lift I64 (= 0))
)

(type + Tile (is Box)
  (using LiftStyle)
  (prop + is-hot _ (= false))
  (blank)
  (rule Tile
    (= lift 1)
    (when-style #is-hot
      (= lift 9)
    )
  )
)
```

```output
1
1
9
```

### Resolves names in an imported rule's body in the rule's own module.

```brix
(import ./rules)
(blank)

(main
  (<> Thing (id: thing) (• boosted 2))
  (print (. thing x))
)
```

```brix file=rules.bx
(prop _ base-x _ (= 40))

(type + Thing
  (prop + x _ (= 0))
  (blank)
  (rule boosted
    (param amount I64)
    (= x (+ base-x amount))
  )
)
```

```output
42
```

### Lets a subclass rule shadow an imported base-class rule with the same name.

```brix
(import ./rules)
(blank)

(main
  (<> Child (id: child) (• themed))
  (print (. child x))
)

(type + Child (is Base)
  (rule themed
    (= x 2)
  )
)
```

```brix file=rules.bx
(type + Base
  (prop + x _ (= 0))
  (blank)
  (rule themed
    (= x 1)
  )
)
```

```output
2
```

### A parent rule's nested rules scope component-owned declarative content but not siblings.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel) (• blue))
    (<> Segment (id: outside) (• part))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (let panel-node (!! (. demo %first-child)))
  (let inside (!! (. panel-node %first-child)))
  (let outside-node (!! (. panel-node %next-sibling)))
  (blank)
  (print (call style-authored-snapshot panel-node))
  (print (call style-authored-snapshot inside))
  (print (call style-authored-snapshot outside-node))
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
  (blank)
  (style-domains ink (domain paint))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Panel (is Box)
  (using TintStyle)
  (rule This
    (<> Segment (• part))
  )
  (blank)
  (rule blue
    (= ink 3)
    (blank)
    (rule part
      (= ink 7)
    )
  )
)

(type + Segment (is Box)
  (using TintStyle)
  (rule part
    (= ink 1)
  )
)
```

```output
node TintStyle.ink layer=2 order=0 kind=static bits=3
node TintStyle.ink layer=2 order=0 kind=static bits=1
node TintStyle.ink layer=2 order=1 kind=static bits=7
node TintStyle.ink layer=2 order=0 kind=static bits=1
```

### A parent rule's nested rules scope the element's body content at any depth but not siblings.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Group (id: group) (• blue)
      (<> Segment (• part))
      (<> Group
        (<> Segment (• part))
      )
    )
    (<> Segment (• part))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (let direct (!! (. group %first-child)))
  (let nested (!! (. (!! (. direct %next-sibling)) %first-child)))
  (let apart (!! (. group %next-sibling)))
  (blank)
  (print (call style-authored-snapshot direct))
  (print (call style-authored-snapshot nested))
  (print (call style-authored-snapshot apart))
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
  (blank)
  (style-domains ink (domain paint))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Group (is Box)
  (using TintStyle)
  (rule blue
    (rule part
      (= ink 7)
    )
  )
)

(type + Segment (is Box)
  (using TintStyle)
  (rule part
    (= ink 1)
  )
)
```

```output
node TintStyle.ink layer=2 order=0 kind=static bits=1
node TintStyle.ink layer=2 order=1 kind=static bits=7
node TintStyle.ink layer=2 order=0 kind=static bits=1
node TintStyle.ink layer=2 order=1 kind=static bits=7
node TintStyle.ink layer=2 order=0 kind=static bits=1
```

### A parent rule's nested rules scope body content that declares ids.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Group (• blue)
      (<> Segment (id: direct) (• part))
      (<> Group
        (<> Segment (id: nested) (• part))
      )
    )
    (<> Segment (id: apart) (• part))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (print (call style-authored-snapshot direct))
  (print (call style-authored-snapshot nested))
  (print (call style-authored-snapshot apart))
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
  (blank)
  (style-domains ink (domain paint))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Group (is Box)
  (using TintStyle)
  (rule blue
    (rule part
      (= ink 7)
    )
  )
)

(type + Segment (is Box)
  (using TintStyle)
  (rule part
    (= ink 1)
  )
)
```

```output
node TintStyle.ink layer=2 order=0 kind=static bits=1
node TintStyle.ink layer=2 order=1 kind=static bits=7
node TintStyle.ink layer=2 order=0 kind=static bits=1
node TintStyle.ink layer=2 order=1 kind=static bits=7
node TintStyle.ink layer=2 order=0 kind=static bits=1
```

### A parent rule's nested rules scope body content inside markup loops and conditionals.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Host (id: host))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (print (call style-authored-snapshot (!! (. (!! (. host looped %first-child)) %first-child))))
  (print (call style-authored-snapshot (!! (. (!! (. host guarded %first-child)) %first-child))))
  (print (call style-authored-snapshot (!! (. (!! (. host apart %first-child)) %first-child))))
)

(type + Host (is Box)
  (prop + names _ (= (list "a" "b")))
  (prop + is-shown _ (= true))
  (content
    (<> This
      (<> Group (id: looped) (• blue)
        (for (in name #names (keyed-by name))
          (<> Segment (• part))
        )
      )
      (<> Group (id: guarded) (• blue)
        (if #is-shown
          (<> Segment (• part))
        )
      )
      (<> Group (id: apart)
        (if #is-shown
          (<> Segment (• part))
        )
      )
    )
  )
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
  (blank)
  (style-domains ink (domain paint))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Group (is Box)
  (using TintStyle)
  (rule blue
    (rule part
      (= ink 7)
    )
  )
)

(type + Segment (is Box)
  (using TintStyle)
  (rule part
    (= ink 1)
  )
)
```

```output
node TintStyle.ink layer=2 order=0 kind=static bits=1
node TintStyle.ink layer=2 order=1 kind=static bits=7
node TintStyle.ink layer=2 order=0 kind=static bits=1
node TintStyle.ink layer=2 order=1 kind=static bits=7
node TintStyle.ink layer=2 order=0 kind=static bits=1
```

### An outer rule's argument reaches its nested rules in template, body, slot, loop, conditional, and generated content.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Host (id: host))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (print (call style-authored-snapshot (. host group inner)))
  (print (call style-authored-snapshot (!! (. host group inner %next-sibling))))
  (print (call style-authored-snapshot (!! (. host slotted %first-child))))
  (print (call style-authored-snapshot (!! (. (!! (. host looped %first-child)) %first-child))))
  (print (call style-authored-snapshot (!! (. (!! (. host guarded %first-child)) %first-child))))
  (print (call style-authored-snapshot (!! (. host adorned %first-child))))
)

(type + Host (is Box)
  (prop + amount I64 (= 6))
  (prop + names _ (= (list "a")))
  (prop + is-shown _ (= true))
  (content
    (<> This
      (<> Group (id: group) (• tint amount)
        (<> Segment (• part))
      )
      (<> Slotted (id: slotted) (• tint amount)
        (<> Segment (• part))
      )
      (<> Tinted (id: looped) (• tint amount)
        (for (in name #names (keyed-by name))
          (<> Segment (• part))
        )
      )
      (<> Tinted (id: guarded) (• tint amount)
        (if #is-shown
          (<> Segment (• part))
        )
      )
      (<> Tinted (id: adorned) (• tint amount) (• adorn))
    )
  )
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
  (blank)
  (style-domains ink (domain paint))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Tinted (is Box)
  (using TintStyle)
  (rule tint
    (param value I64)
    (rule part
      (= ink value)
    )
  )
  (blank)
  (rule adorn
    (<> Segment (slot: first-child) (• part))
  )
)

(type + Group (is Tinted)
  (content
    (<> This
      (<> Segment (id: inner) (• part))
    )
  )
)

(type + Slotted (is Tinted)
  (content
    (<> This
      (+<> Box)
    )
  )
)

(type + Segment (is Box)
  (using TintStyle)
  (rule part
    (= ink 1)
  )
)
```

```output
node TintStyle.ink layer=2 order=0 kind=static bits=1
node TintStyle.ink layer=2 order=1 kind=static bits=6
node TintStyle.ink layer=2 order=0 kind=static bits=1
node TintStyle.ink layer=2 order=1 kind=static bits=6
node TintStyle.ink layer=2 order=0 kind=static bits=1
node TintStyle.ink layer=2 order=1 kind=static bits=6
node TintStyle.ink layer=2 order=0 kind=static bits=1
node TintStyle.ink layer=2 order=1 kind=static bits=6
node TintStyle.ink layer=2 order=0 kind=static bits=1
node TintStyle.ink layer=2 order=1 kind=static bits=6
node TintStyle.ink layer=2 order=0 kind=static bits=1
node TintStyle.ink layer=2 order=1 kind=static bits=6
```

### An outer rule's argument reaches a rule nested two levels below it.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel) (• tint 5)
      (<> Segment (• row)
        (<> Segment (• part))
      )
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot (!! (. (!! (. panel %first-child)) %first-child))))
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Panel (is Box)
  (using TintStyle)
  (rule tint
    (param value I64)
    (rule row
      (rule part
        (= ink value)
      )
    )
  )
)

(type + Segment (is Box)
  (using TintStyle)
  (rule part
    (= ink 1)
  )
)
```

```output
node TintStyle seg=owned gen=1 mask=1 ink=5
```

### Generated child content is scoped by its target's nested rules and by enclosing rule scopes.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Group (• blue)
      (<> Panel (id: panel) (• adorn))
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (let generated (!! (. panel %first-child)))
  (blank)
  (print (call markup-is-style-shadow generated))
  (print (call style-authored-snapshot generated))
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
  (prop + tone I64 (= 0))
  (blank)
  (style-domains ink (domain paint))
  (style-domains tone (domain paint))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Group (is Box)
  (using TintStyle)
  (rule blue
    (rule part
      (= ink 7)
    )
  )
)

(type + Panel (is Box)
  (using TintStyle)
  (rule adorn
    (<> Segment (slot: first-child) (• part) (• mark))
    (blank)
    (rule mark
      (= tone 9)
    )
  )
)

(type + Segment (is Box)
  (using TintStyle)
  (rule part
    (= ink 1)
  )
)
```

```output
true
node TintStyle.ink layer=2 order=0 kind=static bits=1
node TintStyle.ink layer=2 order=1 kind=static bits=7
node TintStyle.tone layer=2 order=2 kind=static bits=9
```

### Generated sibling content is scoped by its target's nested rules and by enclosing rule scopes.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Group (• blue)
      (<> Panel (id: panel) (• trail))
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (let generated (!! (. panel %next-sibling)))
  (blank)
  (print (call markup-is-style-shadow generated))
  (print (call style-authored-snapshot generated))
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
  (prop + tone I64 (= 0))
  (blank)
  (style-domains ink (domain paint))
  (style-domains tone (domain paint))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Group (is Box)
  (using TintStyle)
  (rule blue
    (rule part
      (= ink 7)
    )
  )
)

(type + Panel (is Box)
  (using TintStyle)
  (rule trail
    (<> Segment (slot: after) (• part) (• mark))
    (blank)
    (rule mark
      (= tone 9)
    )
  )
)

(type + Segment (is Box)
  (using TintStyle)
  (rule part
    (= ink 1)
  )
)
```

```output
true
node TintStyle.ink layer=2 order=0 kind=static bits=1
node TintStyle.ink layer=2 order=1 kind=static bits=7
node TintStyle.tone layer=2 order=2 kind=static bits=9
```

### Scoped nested rules specialize component content cache keys.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: blue-panel) (• blue))
    (<> Panel (id: red-panel) (• red))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (let blue-part (!! (. blue-panel %first-child)))
  (let red-part (!! (. red-panel %first-child)))
  (blank)
  (print (call style-authored-snapshot blue-part))
  (print (call style-authored-snapshot red-part))
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
  (blank)
  (style-domains ink (domain paint))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Panel (is Box)
  (using TintStyle)
  (rule This
    (<> Segment (• part))
  )
  (blank)
  (rule blue
    (rule part
      (= ink 7)
    )
  )
  (blank)
  (rule red
    (rule part
      (= ink 9)
    )
  )
)

(type + Segment (is Box)
  (using TintStyle)
  (rule part
    (= ink 1)
  )
)
```

```output
node TintStyle.ink layer=2 order=0 kind=static bits=1
node TintStyle.ink layer=2 order=1 kind=static bits=7
node TintStyle.ink layer=2 order=0 kind=static bits=1
node TintStyle.ink layer=2 order=1 kind=static bits=9
```

### Inherited `This` nested rules scope subclass-owned declarative content.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> ChildPanel (id: child))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (let part (!! (. child %first-child)))
  (blank)
  (print (call style-authored-snapshot part))
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
  (blank)
  (style-domains ink (domain paint))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + BasePanel (is Box)
  (using TintStyle)
  (rule This
    (rule part
      (= ink 5)
    )
  )
)

(type + ChildPanel (is BasePanel)
  (content
    (<> Segment (• part))
  )
)

(type + Segment (is Box)
  (using TintStyle)
  (rule part
    (= ink 1)
  )
)
```

```output
node TintStyle.ink layer=2 order=0 kind=static bits=1
node TintStyle.ink layer=2 order=1 kind=static bits=5
```

### A subclass `This` nested rule scopes declarative content inherited from its base class, with and without a "when" state branch.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> ChildPanel (id: child))
    (<> GatedPanel (id: gated))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print "plain nested rule")
  (print (call style-authored-snapshot (!! (. child %first-child))))
  (print "gated nested rule, off")
  (print (call style-authored-snapshot (!! (. gated %first-child))))
  (= (. gated is-on) true)
  (call (. demo markup-batch-queue checkpoint))
  (print "gated nested rule, on")
  (print (call style-authored-snapshot (!! (. gated %first-child))))
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
  (blank)
  (style-domains ink (domain paint))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + BasePanel (is Box)
  (using TintStyle)
  (content
    (<> Segment (• part))
  )
)

(type + ChildPanel (is BasePanel)
  (rule This
    (rule part
      (= ink 5)
    )
  )
)

(type + GatedPanel (is BasePanel)
  (prop + is-on _ (= false))
  (blank)
  (rule This
    (when-style #is-on
      (rule part
        (= ink 7)
      )
    )
  )
)

(type + Segment (is Box)
  (using TintStyle)
  (rule part
    (= ink 1)
  )
)
```

```output
plain nested rule
node TintStyle.ink layer=2 order=0 kind=static bits=1
node TintStyle.ink layer=2 order=1 kind=static bits=5
gated nested rule, off
node disabled-blocks=1
node TintStyle.ink layer=2 order=0 kind=static bits=1
node TintStyle.ink layer=4 order=1 kind=static bits=7 block=0
gated nested rule, on
node TintStyle.ink layer=2 order=0 kind=static bits=1
node TintStyle.ink layer=4 order=1 kind=static bits=7 block=0
```

### A "when" state branch that styles nested content keeps following its state after a collection.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(import /BxGC/BxGC.h)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> GatedPanel (id: gated))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (call C:brixGCCollect)
  (call C:brixGCCollect)
  (= (. gated is-on) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (. (as (!! (. gated %first-child)) Segment) $ink))
  (= (. gated is-on) false)
  (call (. demo markup-batch-queue checkpoint))
  (print (. (as (!! (. gated %first-child)) Segment) $ink))
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
  (blank)
  (style-domains ink (domain paint))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + GatedPanel (is Box)
  (using TintStyle)
  (prop + is-on _ (= false))
  (blank)
  (rule This
    (when-style #is-on
      (rule part
        (= ink 7)
      )
    )
  )
  (content
    (<> Segment (• part))
  )
)

(type + Segment (is Box)
  (using TintStyle)
  (rule part
    (= ink 1)
  )
)
```

```output
7
1
```

### Static style rule values preserve zero-valued union payloads.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel) (• dark))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (print (== (. panel $background-color) null))
  (print (call ansi-background (. panel $background-color)))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Panel (is Box)
  (rule dark
    (= background-color (call (. BackgroundColor ansi) (call AnsiColor 0)))
  )
)

(fn-do + ansi-background AnsiColor
  (param background (? BackgroundColor))
  (return-if (call AnsiColor 255) (if (not background)))
  (blank)
  (let live background)
  (return
    (match live
      (case (call .ansi color)
        color
      )
      (default
        (call AnsiColor 8)
      )
    )
  )
)
```

```output
false
0
```

### Static object-valued nested rule thunks use module scope, not the component content closure.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> ConfirmLite (id: confirm))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (let backdrop (!! (. confirm %first-child)))
  (blank)
  (print (call ansi-background (. backdrop $background-color)))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + BaseDialogLite (is Box)
  (rule This
    (rule modal-backdrop
      (= background-color (call (. BackgroundColor ansi) (call AnsiColor 0)))
    )
  )
  (content
    (<> This
      (<> BackdropLite (• modal-backdrop))
      (<> DialogPartLite)
    )
  )
)

(type + ConfirmLite (is BaseDialogLite)
  (content
    (<> This
      (<> DialogPartLite)
    )
  )
)

(type + BackdropLite (is Box))

(type + DialogPartLite (is Box))

(fn-do + ansi-background AnsiColor
  (param background (? BackgroundColor))
  (return-if (call AnsiColor 255) (if (not background)))
  (blank)
  (let live background)
  (return
    (match live
      (case (call .ansi color)
        color
      )
      (default
        (call AnsiColor 8)
      )
    )
  )
)
```

```output
0
```

### Slot outlet nested rules scope supplied slot content but not caller siblings.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel)
      (<> ActionBox (slot: actions)
        (<> Segment (• part))
      )
    )
    (<> Segment (id: outside) (• part))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (let actions (!! (. panel %first-child)))
  (let segment (!! (. actions %first-child)))
  (let outside-node (!! (. panel %next-sibling)))
  (blank)
  (print (call style-authored-snapshot actions))
  (print (call style-authored-snapshot segment))
  (print (call style-authored-snapshot outside-node))
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
  (blank)
  (style-domains ink (domain paint))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Panel (is Box)
  (using TintStyle)
  (rule This
    (rule actions
      (= ink 2)
      (blank)
      (rule part
        (= ink 7)
      )
    )
  )
  (content
    (+<> ActionBox (id: actions) (• actions)
      (<> ActionBox)
    )
  )
)

(type + ActionBox (is Box)
  (using TintStyle)
)

(type + Segment (is Box)
  (using TintStyle)
  (rule part
    (= ink 1)
  )
)
```

```output
node TintStyle.ink layer=2 order=0 kind=static bits=2
node TintStyle.ink layer=2 order=0 kind=static bits=1
node TintStyle.ink layer=2 order=1 kind=static bits=7
node TintStyle.ink layer=2 order=0 kind=static bits=1
```

### An own rule written in a parent rule scopes the template of the element the parent rule is applied to, at any depth, but not the element itself, its body content, or the templates of components in its template or its body.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel) (• blue) (• part)
      (<> Segment (id: written) (• part))
      (<> Card (id: card))
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (print (call style-computed-snapshot panel))
  (print (call style-computed-snapshot (. panel near)))
  (print (call style-computed-snapshot (. panel deep)))
  (print (call style-computed-snapshot (. panel held inside)))
  (print (call style-computed-snapshot written))
  (print (call style-computed-snapshot (. card inside)))
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Panel (is Box)
  (using TintStyle)
  (rule blue
    (own-rule part
      (= ink 7)
    )
  )
  (blank)
  (rule part
    (= ink 2)
  )
  (content
    (<> This
      (<> Segment (id: near) (• part))
      (<> Box
        (<> Segment (id: deep) (• part))
      )
      (<> Card (id: held))
    )
  )
)

(type + Card (is Box)
  (content
    (<> This
      (<> Segment (id: inside) (• part))
    )
  )
)

(type + Segment (is Box)
  (using TintStyle)
  (rule part
    (= ink 1)
  )
)
```

```output
node TintStyle seg=owned gen=1 mask=1 ink=2
node TintStyle seg=owned gen=1 mask=1 ink=7
node TintStyle seg=owned gen=1 mask=1 ink=7
node TintStyle seg=owned gen=1 mask=1 ink=1
node TintStyle seg=owned gen=1 mask=1 ink=1
node TintStyle seg=owned gen=1 mask=1 ink=1
```

### An own rule reaches the template a class inherits and content the element's rules generate, but not the content a caller gives a slot of the template.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel) (• blue) (• adorn)
      (<> Segment (id: supplied) (• part))
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (print (call markup-is-style-shadow (!! (. panel %first-child))))
  (print (call style-computed-snapshot (!! (. panel %first-child))))
  (print (call style-computed-snapshot (. panel inherited)))
  (print (call style-computed-snapshot supplied))
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Frame (is Box)
  (using TintStyle)
  (rule blue
    (own-rule part
      (= ink 7)
    )
  )
  (blank)
  (rule adorn
    (<> Segment (slot: first-child) (• part))
  )
  (content
    (<> This
      (<> Segment (id: inherited) (• part))
      (+<> Box)
    )
  )
)

(type + Panel (is Frame))

(type + Segment (is Box)
  (using TintStyle)
  (rule part
    (= ink 1)
  )
)
```

```output
true
node TintStyle seg=owned gen=1 mask=1 ink=7
node TintStyle seg=owned gen=1 mask=1 ink=7
node TintStyle seg=owned gen=1 mask=1 ink=1
```

### An own rule reaches what its element's template gives a slot of a component written there, and what a subclass's template gives a slot of the template it inherits, but not the template of the component whose slot is given.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel) (• blue))
    (<> Filled (id: filled) (• blue))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (print (call style-computed-snapshot (. panel given)))
  (print (call style-computed-snapshot (. panel framed inner)))
  (print (call style-computed-snapshot (. filled filler)))
  (print (call style-computed-snapshot (. filled inner)))
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Tinted (is Box)
  (using TintStyle)
  (rule blue
    (own-rule part
      (= ink 7)
    )
  )
)

(type + Panel (is Tinted)
  (content
    (<> This
      (<> Frame (id: framed)
        (<> Segment (id: given) (• part))
      )
    )
  )
)

(type + Frame (is Tinted)
  (content
    (<> This
      (<> Segment (id: inner) (• part))
      (+<> Box)
    )
  )
)

(type + Filled (is Frame)
  (content
    (<> This
      (<> Segment (id: filler) (• part))
    )
  )
)

(type + Segment (is Box)
  (using TintStyle)
  (rule part
    (= ink 1)
  )
)
```

```output
node TintStyle seg=owned gen=1 mask=1 ink=7
node TintStyle seg=owned gen=1 mask=1 ink=1
node TintStyle seg=owned gen=1 mask=1 ink=7
node TintStyle seg=owned gen=1 mask=1 ink=7
```

### An own rule inside a "when" state branch of a class "This" rule applies in the class template only while the state holds, and never in the content a caller gives a slot.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Toggle (id: toggle)
      (<> Segment (id: supplied) (• part))
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot (. toggle mark)))
  (print (call style-computed-snapshot supplied))
  (blank)
  (= (. toggle is-on) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot (. toggle mark)))
  (print (call style-computed-snapshot supplied))
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Toggle (is Box)
  (using TintStyle)
  (prop + is-on _ (= false))
  (blank)
  (rule This
    (when-style #is-on
      (own-rule part
        (= ink 7)
      )
    )
  )
  (content
    (<> This
      (<> Segment (id: mark) (• part))
      (+<> Box)
    )
  )
)

(type + Segment (is Box)
  (using TintStyle)
  (rule part
    (= ink 1)
  )
)
```

```output
node TintStyle seg=owned gen=1 mask=1 ink=1
node TintStyle seg=owned gen=1 mask=1 ink=1
node TintStyle seg=owned gen=2 mask=1 ink=7
node TintStyle seg=owned gen=1 mask=1 ink=1
```

### An own rule takes the value given to the rule it is written in and generates content where it matches in the template, but not where it would match in body content.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel) (• tint 5)
      (<> Slab (id: written) (• host))
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (blank)
  (print (call style-computed-snapshot (. panel near)))
  (print (call markup-is-style-shadow (!! (. panel near %first-child))))
  (print (call style-computed-snapshot (!! (. panel near %first-child))))
  (print (call style-computed-snapshot written))
  (print (== (. written %first-child) null))
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Panel (is Box)
  (using TintStyle)
  (rule tint
    (param value I64)
    (own-rule host
      (= ink value)
      (blank)
      (<> Segment (slot: first-child) (• part))
    )
  )
  (content
    (<> This
      (<> Slab (id: near) (• host))
    )
  )
)

(type + Slab (is Box)
  (using TintStyle)
  (rule host
    (= ink 2)
  )
)

(type + Segment (is Box)
  (using TintStyle)
  (rule part
    (= ink 1)
  )
)
```

```output
node TintStyle seg=owned gen=1 mask=1 ink=5
true
node TintStyle seg=owned gen=1 mask=1 ink=1
node TintStyle seg=owned gen=1 mask=1 ink=2
true
```

### Reports a rule name nothing declares.

```brix compile-error
(main
  (<> Thing (id: thing) (• missing))
)

(type + Thing)
```

```output
Exception: No style rule is named "missing". Write one in a style sheet, or, where a library applies it for a theme to style, declare it there with an empty rule
<no file:3>
      (<> Thing (id: thing) (• missing))
                            ^^^^^^^^^^^
```

### An empty rule declares a name for a theme to style.

```brix
(main
  (<> Thing (id: thing) (• hook))
  (call print "ok")
)

(type + Thing
  (rule hook)
)
```

```output
ok
```

### Attaches generated style content and gates it with a tracked state.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel) (• adorn))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (let adornment (!! (. panel %next-sibling)))
  (print (== (. panel %first-child) null))
  (print (call markup-is-style-shadow adornment))
  (print (== (as (!! (call markup-style-parent-host adornment)) Pointer) (as panel Pointer)))
  (blank)
  (= (. panel has-adornment) false)
  (call (. demo markup-batch-queue checkpoint))
  (print (== (. panel %next-sibling) null))
  (blank)
  (= (. panel has-adornment) true)
  (call (. demo markup-batch-queue checkpoint))
  (let next-adornment (!! (. panel %next-sibling)))
  (print (call markup-is-style-shadow next-adornment))
  (print (== (as (!! (call markup-style-parent-host next-adornment)) Pointer) (as panel Pointer)))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Panel (is Box)
  (prop + has-adornment _ (= true))
  (blank)
  (rule adorn
    (when-style #has-adornment
      (<> Label (slot: after) (text: "!"))
    )
  )
)
```

```output
true
true
true
true
true
true
```

### Attaches generated style content while a negated tracked state is false.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel) (• adorn))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call markup-is-style-shadow (!! (. panel %next-sibling))))
  (blank)
  (= (. panel is-plain) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (== (. panel %next-sibling) null))
  (blank)
  (= (. panel is-plain) false)
  (call (. demo markup-batch-queue checkpoint))
  (print (call markup-is-style-shadow (!! (. panel %next-sibling))))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Panel (is Box)
  (prop + is-plain _ (= false))
  (blank)
  (rule adorn
    (when-style (not #is-plain)
      (<> Label (slot: after) (text: "!"))
    )
  )
)
```

```output
true
true
true
```

### A nested rule inside a "when" state branch of a class "This" rule applies in the class template only while the state holds.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Section (id: section))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot (. section header)))
  (blank)
  (= (. section is-expanded) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot (. section header)))
  (blank)
  (= (. section is-expanded) false)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot (. section header)))
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Section (is Box)
  (using TintStyle)
  (prop + is-expanded _ (= false))
  (blank)
  (rule This
    (rule part
      (= ink 3)
    )
    (blank)
    (when-style #is-expanded
      (rule part
        (= ink 7)
      )
    )
  )
  (content
    (<> This
      (<> Segment (id: header) (• part))
    )
  )
)

(type + Segment (is Box)
  (using TintStyle)
  (rule part
    (= ink 1)
  )
)
```

```output
node TintStyle seg=owned gen=1 mask=1 ink=3
node TintStyle seg=owned gen=2 mask=1 ink=7
node TintStyle seg=owned gen=3 mask=1 ink=3
```

### Nested rules inside a "when" state branch follow the element's state in slot content, body children, generated content, and content nested inside another component.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Host (id: host))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (call show host)
  (blank)
  (= (. host slotted is-open) false)
  (= (. host plain is-open) false)
  (= (. host adorned is-open) false)
  (= (. host deep is-open) false)
  (call (. demo markup-batch-queue checkpoint))
  (print "closed")
  (call show host)
  (blank)
  (= (. host slotted is-open) true)
  (= (. host plain is-open) true)
  (= (. host adorned is-open) true)
  (= (. host deep is-open) true)
  (call (. demo markup-batch-queue checkpoint))
  (print "open")
  (call show host)
)

(fn-do _ show _
  (param host Host)
  (print (str "slot " (call style-computed-snapshot (!! (. host slotted %first-child)))))
  (print (str "body " (call style-computed-snapshot (!! (. host plain %first-child)))))
  (print (str "generated " (call style-computed-snapshot (!! (. host adorned %first-child)))))
  (print (str "deep " (call style-computed-snapshot (!! (. (!! (. host deep %first-child)) %first-child)))))
)

(type + Host (is Box)
  (content
    (<> This
      (<> Slotted (id: slotted)
        (= is-open true)
        (<> Segment (• part))
      )
      (<> Panel (id: plain) (• openable)
        (= is-open true)
        (<> Segment (• part))
      )
      (<> Panel (id: adorned) (• openable) (• adorn)
        (= is-open true)
      )
      (<> Slotted (id: deep)
        (= is-open true)
        (<> Wrapper)
      )
    )
  )
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Slotted (is Box)
  (using TintStyle)
  (prop + is-open _ (= false))
  (blank)
  (rule This
    (when-style #is-open
      (rule part
        (= ink 7)
      )
    )
  )
  (content
    (<> This
      (+<> Box)
    )
  )
)

(type + Panel (is Box)
  (using TintStyle)
  (prop + is-open _ (= false))
  (blank)
  (rule openable
    (when-style #is-open
      (rule part
        (= ink 7)
      )
    )
  )
  (blank)
  (rule adorn
    (<> Segment (slot: first-child) (• part))
  )
)

(type + Wrapper (is Box)
  (content
    (<> This
      (<> Segment (id: inner) (• part))
    )
  )
)

(type + Segment (is Box)
  (using TintStyle)
  (rule part
    (= ink 1)
  )
)
```

```output
slot node TintStyle seg=owned gen=1 mask=1 ink=7
body node TintStyle seg=owned gen=1 mask=1 ink=7
generated node TintStyle seg=owned gen=1 mask=1 ink=7
deep node TintStyle seg=owned gen=1 mask=1 ink=7
closed
slot node TintStyle seg=owned gen=2 mask=1 ink=1
body node TintStyle seg=owned gen=2 mask=1 ink=1
generated node TintStyle seg=owned gen=2 mask=1 ink=1
deep node TintStyle seg=owned gen=2 mask=1 ink=1
open
slot node TintStyle seg=owned gen=3 mask=1 ink=7
body node TintStyle seg=owned gen=3 mask=1 ink=7
generated node TintStyle seg=owned gen=3 mask=1 ink=7
deep node TintStyle seg=owned gen=3 mask=1 ink=7
```

### Nested rules inside a "when" state branch follow the element's state for inherited defaults, root rule applications, rule arguments, deeper nested rules with generated content, and rules applied by a slot outlet.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Host (id: host))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (call show host)
  (blank)
  (= (. host derived is-open) true)
  (= (. host rooted is-open) true)
  (= (. host tinted is-open) true)
  (= (. host chained is-open) true)
  (let held (as (!! (. host holder %first-child)) Panel))
  (= (. held is-open) true)
  (call (. demo markup-batch-queue checkpoint))
  (print "open")
  (call show host)
)

(fn-do _ show _
  (param host Host)
  (print (str "derived " (call style-computed-snapshot (. host derived inner))))
  (print (str "rooted " (call style-computed-snapshot (. host rooted inner))))
  (print (str "tinted " (call style-computed-snapshot (!! (. host tinted %first-child)))))
  (print (str "chained " (call style-computed-snapshot (!! (. (!! (. host chained %first-child)) %first-child)))))
  (print (str "chained mark " (!= (. (!! (. (!! (. host chained %first-child)) %first-child)) %first-child) null)))
  (print (str "outlet " (call style-computed-snapshot (!! (. (!! (. host holder %first-child)) %first-child)))))
)

(type + Host (is Box)
  (content
    (<> This
      (<> Derived (id: derived))
      (<> Rooted (id: rooted))
      (<> Panel (id: tinted) (• tint 5)
        (<> Segment (• part))
      )
      (<> Panel (id: chained) (• chain)
        (<> Segment (• row)
          (<> Segment (• part))
        )
      )
      (<> Holder (id: holder)
        (<> Panel
          (<> Segment (• part))
        )
      )
    )
  )
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Panel (is Box)
  (using TintStyle)
  (prop + is-open _ (= false))
  (blank)
  (rule This
    (when-style #is-open
      (rule part
        (= ink 7)
      )
    )
  )
  (blank)
  (rule tint
    (param value I64)
    (when-style #is-open
      (rule part
        (= ink value)
      )
    )
  )
  (blank)
  (rule chain
    (when-style #is-open
      (rule row
        (rule part
          (= ink 8)
          (blank)
          (<> Segment (slot: first-child))
        )
      )
    )
  )
  (blank)
  (rule opened
    (when-style #is-open
      (rule part
        (= ink 6)
      )
    )
  )
)

(type + Derived (is Panel)
  (content
    (<> This
      (<> Segment (id: inner) (• part))
    )
  )
)

(type + Rooted (is Box)
  (using TintStyle)
  (prop + is-open _ (= false))
  (blank)
  (rule rooted
    (when-style #is-open
      (rule part
        (= ink 4)
      )
    )
  )
  (content
    (<> This (• rooted)
      (<> Segment (id: inner) (• part))
    )
  )
)

(type + Holder (is Box)
  (content
    (<> This
      (+<> Panel (• opened))
    )
  )
)

(type + Segment (is Box)
  (using TintStyle)
  (rule part
    (= ink 1)
  )
)
```

```output
derived node TintStyle seg=owned gen=1 mask=1 ink=1
rooted node TintStyle seg=owned gen=1 mask=1 ink=1
tinted node TintStyle seg=owned gen=1 mask=1 ink=1
chained node TintStyle seg=owned gen=1 mask=1 ink=1
chained mark false
outlet node TintStyle seg=owned gen=1 mask=1 ink=1
open
derived node TintStyle seg=owned gen=2 mask=1 ink=7
rooted node TintStyle seg=owned gen=2 mask=1 ink=4
tinted node TintStyle seg=owned gen=2 mask=1 ink=5
chained node TintStyle seg=owned gen=2 mask=1 ink=8
chained mark true
outlet node TintStyle seg=owned gen=2 mask=1 ink=6
```

### Rejects nested rules inside a "when" state branch of a rule that is applied conditionally.

```brix crash
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel) (• outer)
      (<> Segment (• part))
    )
  )
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Panel (is Box)
  (using TintStyle)
  (prop + is-open _ (= false))
  (prop + is-hot _ (= false))
  (blank)
  (rule opened
    (when-style #is-open
      (rule part
        (= ink 7)
      )
    )
  )
  (blank)
  (rule outer
    (when-style #is-hot
      (• opened)
    )
  )
)

(type + Segment (is Box)
  (using TintStyle)
  (rule part
    (= ink 1)
  )
)
```

```output
Exception: Conditionally applied style rules may only contain style declarations
<no file:34>
          (rule part
          ^^^^^^^^^
```

### Ignores a rule selector whose receiver type is unrelated to the element type.

```brix
(main
  (<> Other (id: other) (• set-x 1))
)

(type + Thing
  (prop + x _ (= 0))
  (blank)
  (rule set-x
    (param value Int)
    (= x value)
  )
)

(type + Other)
```

### Ignores a subclass-scoped rule selector on an instance of the base class.

```brix
(main
  (<> Base (id: base) (• child-only))
)

(type + Base)

(type + Child (is Base)
  (rule child-only)
)
```

### Type-checks rule arguments against the rule signature.

```brix crash
(main
  (<> Thing (id: thing) (• set-x (call Other)))
)

(type + Thing
  (prop + x _ (= 0))
  (blank)
  (rule set-x
    (param value Int)
    (= x value)
  )
)

(type + Other)
```

```output
Exception: Type mismatch
<no file:3>
      (<> Thing (id: thing) (• set-x (call Other)))
                                     ^^^^^^^^^^^^
```

### Rejects a parameterized rule used without arguments.

```brix crash
(main
  (<> Thing (id: thing) (• set-x))
)

(type + Thing
  (prop + x _ (= 0))
  (blank)
  (rule set-x
    (param value Int)
    (= x value)
  )
)
```

```output
Exception: Type mismatch
<no file:3>
      (<> Thing (id: thing) (• set-x))
                            ^^^^^^^^^
```

### Rejects extra style-rule arguments.

```brix crash
(main
  (<> Thing (id: thing) (• set-x 1 2))
)

(type + Thing
  (prop + x _ (= 0))
  (blank)
  (rule set-x
    (param value Int)
    (= x value)
  )
)
```

```output
Exception: Type mismatch
<no file:3>
      (<> Thing (id: thing) (• set-x 1 2))
                            ^^^^^^^^^^^^^
```

### Rejects a colon argument passed to a no-parameter rule.

```brix crash
(main
  (<> Thing (id: thing) (• answer 1))
)

(type + Thing
  (prop + x _ (= 0))
  (blank)
  (rule answer
    (= x 42)
  )
)
```

```output
Exception: Type mismatch
<no file:3>
      (<> Thing (id: thing) (• answer 1))
                            ^^^^^^^^^^^^
```

### Treats a colon shorthand value as one argument even when it contains commas.

```brix crash
(main
  (<> Thing (id: thing) (• set (call Point 1 2)))
)

(type + Thing
  (prop + x _ (= 0))
  (prop + y _ (= 0))
  (blank)
  (rule set
    (param value Int)
    (= x value)
  )
)

(type + Point (is (tuple (: x Int) (: y Int))))
```

```output
Exception: Type mismatch
<no file:3>
      (<> Thing (id: thing) (• set (call Point 1 2)))
                                   ^^^^^^^^^^^^^^^^
```

### Type-checks rule bodies against the receiver properties.

```brix crash
(main
  (<> Thing (id: thing) (• bad))
)

(type + Thing
  (prop + x _ (= 0))
  (blank)
  (rule bad
    (= missing 1)
  )
)
```

```output
Exception: Property "missing" not found
<no file:10>
        (= missing 1)
           ^^^^^^^
```

### Rejects duplicate rule names for the same receiver and signature.

```brix crash
(main
  (<> Thing (id: thing) (• themed))
)

(type + Thing
  (rule themed)
  (blank)
  (rule themed)
)
```

```output
Exception: Redefinition of style rule
<no file:9>
      (rule themed)
      ^^^^^^^^^^^^^
```

### A subclass style default overrides its base class default for the same property, even when the base declares more defaults before it.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Child)
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (let child (!! (. demo %first-child)))
  (print (call style-authored-snapshot child))
  (print (call style-computed-snapshot child))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 (= 0))
  (prop + bold _ (= false))
  (prop + size I64 (= 0))
)

(type + Base (is Box)
  (using TextStyle)
  (rule This
    (= bold true)
    (= size 12)
    (= ink 1)
  )
)

(type + Child (is Base)
  (rule This
    (= ink 9)
  )
)
```

```output
node TextStyle.bold layer=0 order=65536 kind=static bits=1
node TextStyle.size layer=0 order=65537 kind=static bits=12
node TextStyle.ink layer=0 order=65538 kind=static bits=1
node TextStyle.ink layer=0 order=131072 kind=static bits=9
node TextStyle seg=owned gen=1 mask=7 ink=9 bold=1 size=12
```

### Converts rule arguments through fromverters, including rules applied inside other rules.

```brix
(main
  (<> Thing (id: direct) (• tint "ab"))
  (<> Thing (id: nested) (• accent))
  (print (. direct shade level))
  (print (. nested shade level))
)

(type + Thing
  (prop + shade _ (= (call Shade 0)))
  (blank)
  (rule tint
    (param value Shade)
    (= shade value)
  )
  (blank)
  (rule accent (• tint "abcd"))
)

(type + Shade
  (prop + level Int)
  (fn-do + This _
    (param level _)
  )
  (fn-from-expr + (: value String)
    (call Shade (. value length))
  )
)
```

```output
2
4
```

### Prefers an exact rule overload over one that needs a conversion.

```brix
(main
  (<> Thing (id: thing) (• pick "abc"))
  (print (. thing text))
  (print (. thing shade level))
)

(type + Thing
  (prop + text _ (= ""))
  (prop + shade _ (= (call Shade 0)))
  (blank)
  (rule pick
    (param value Shade)
    (= shade value)
  )
  (blank)
  (rule pick
    (param value String)
    (= text value)
  )
)

(type + Shade
  (prop + level Int)
  (fn-do + This _
    (param level _)
  )
  (fn-from-expr + (: value String)
    (call Shade (. value length))
  )
)
```

```output
abc
0
```

### A component's own rules scope over slot content the same way they scope over its template: nested rules of its defaults matched on a template element, and nested rules of a rule applied by its root, including "when" branches.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> List (id: list)
      (<> Segment (id: icon) (• icon))
      (<> Segment (id: mark) (• mark))
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot icon))
  (print (call style-computed-snapshot mark))
  (= (. list item is-selected) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot icon))
  (= (. list is-framed) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot mark))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 (= 7))
)

(type + Segment (is Box)
  (using TextStyle)
)

(type + List (is Box)
  (using TextStyle)
  (prop + is-framed _ (= false))
  (blank)
  (rule This
    (rule Item
      (when-style #is-selected
        (rule icon
          (= ink 21)
        )
      )
    )
  )
  (blank)
  (rule framed
    (rule mark
      (= ink 4)
    )
    (blank)
    (when-style #is-framed
      (rule mark
        (= ink 8)
      )
    )
  )
  (content
    (<> This (• framed)
      (<> Item (id: item)
        (+<> Box)
      )
    )
  )
)

(type + Item (is Box)
  (prop + is-selected _ (= false))
)
```

```output
initial
node TextStyle seg=owned gen=1 mask=1 ink=4
node TextStyle seg=owned gen=1 mask=1 ink=21
node TextStyle seg=owned gen=2 mask=1 ink=8
```

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/task)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (lazy marked false)
  (let done (call Semaphore))
  (blank)
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel) (• accent (if: #marked)) (• wide 2 (if: #marked)))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot panel))
  (blank)
  (= marked true)
  (do-new
    (print (call style-computed-snapshot panel))
    (call (. done post))
  )
  (call (. done wait))
  (blank)
  (= marked false)
  (do-new
    (print (call style-computed-snapshot panel))
    (call (. done post))
  )
  (call (. done wait))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 1))
  (prop + tone I64 (= 0))
)

(type + Panel (is Box)
  (using TintStyle)
  (rule accent
    (= ink 9)
  )
  (blank)
  (rule wide
    (param amount I64)
    (= tone amount)
  )
)
```

```output
initial
node TintStyle seg=owned gen=1 mask=3 ink=9 tone=2
initial
```

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel) (• loud))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot panel))
)

(blank)
(prop + loud _ (= 7))

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 1))
)

(type + Panel (is Box)
  (using TintStyle)
  (rule loud
    (= ink loud)
  )
)
```

```output
node TintStyle seg=owned gen=1 mask=1 ink=7
```

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel) (• loud))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot panel))
)

(blank)
(prop + loud _ (= 7))

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 1))
)

(type + Panel (is Box)
  (using TintStyle)
  (using Theme)
)

(style Theme
  (using TintStyle)
  (rule loud
    (= ink loud)
  )
)
```

```output
node TintStyle seg=owned gen=1 mask=1 ink=7
```

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/task)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let done (call Semaphore))
  (blank)
  (<> BatchDemo (id: demo)
    (<> Tag (id: tag) (• badge))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot tag))
  (blank)
  (= (. tag is-active) true)
  (do-new
    (print (call style-computed-snapshot tag))
    (call (. done post))
  )
  (call (. done wait))
  (blank)
  (= (. tag is-active) false)
  (do-new
    (print (call style-computed-snapshot tag))
    (call (. done post))
  )
  (call (. done wait))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 (= 7))
)

(type + Tag (is Box)
  (using TextStyle)
  (prop + is-active _ (= false))
  (blank)
  (rule badge
    (= ink 9)
    (blank)
    (style-set is-active ink 11)
  )
)
```

```output
node TextStyle seg=owned gen=1 mask=1 ink=9
node TextStyle seg=owned gen=2 mask=1 ink=11
node TextStyle seg=owned gen=3 mask=1 ink=9
```

### Rules written in a rule a slot outlet applies reach elements inside what a line gives the slot.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Holder (id: holder)
      (<> Panel
        (<> Segment (• part))
      )
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot (!! (. (!! (. holder %first-child)) %first-child))))
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Panel (is Box)
  (using TintStyle)
  (rule opened
    (rule part
      (= ink 6)
    )
  )
)

(type + Holder (is Box)
  (content
    (<> This
      (+<> Panel (• opened))
    )
  )
)

(type + Segment (is Box)
  (using TintStyle)
  (rule part
    (= ink 1)
  )
)
```

```output
node TintStyle seg=owned gen=1 mask=1 ink=6
```

### A tracked name in a value a rule an element of a class's markup applies is read again as it changes, and the rule's line that sets a property with it runs again, as an attribute would.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let panel (call Panel))
  (print (call (. panel report)))
  (= (. panel amount) 3)
  (print (call (. panel report)))
)

(type + Thing (is Box)
  (prop + x _ (= 0))
  (blank)
  (rule set
    (param value Int)
    (= x value)
  )
)

(type + Panel (is Box)
  (prop + amount _ (= 1))
  (content
    (<> This
      (<> Box
        (<> Thing (• set #amount))
      )
    )
  )
  (fn-do + report String
    (let text "")
    (let node %first-child)
    (while node
      (= text (++ text "[box"))
      (let inner (. node %first-child))
      (while inner
        (= text (++ text (str " " (. (as inner Thing) x))))
        (= inner (. inner %next-sibling))
      )
      (= text (++ text "]"))
      (= node (. node %next-sibling))
    )
    (return text)
  )
)
```

```output
[box 1]
[box 3]
```

### A tracked name in a value a rule an element of a class's markup applies is read again as it changes, and the rule's declaration that reads it is declared again, as the same line written on the element is.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (str (. panel short $ink) " " (. panel long $ink)))
  (= (. panel amount) 3)
  (call (. demo markup-batch-queue checkpoint))
  (print (str (. panel short $ink) " " (. panel long $ink)))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
)

(type + Mover (is Box)
  (using TintStyle)
  (rule tint
    (param value I64)
    (= ink value)
  )
)

(type + Panel (is Box)
  (prop + amount _ (= 1))
  (content
    (<> This
      (<> Mover (id: short) (• tint #amount))
      (<> Mover (id: long)
        (= ink #amount)
      )
    )
  )
)
```

```output
1 1
3 3
```

### A tracked name in a value a rule an element in an arm of a markup branch region applies is read again as it changes, and the rule's declaration that reads it is declared again, as one an element outside a region applies is.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (let mover (as (!! (. (!! (. panel %first-child)) %first-child)) Mover))
  (print (. mover $ink))
  (= (. panel amount) 3)
  (call (. demo markup-batch-queue checkpoint))
  (print (. mover $ink))
  (print (== mover (!! (. (!! (. panel %first-child)) %first-child))))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
)

(type + Mover (is Box)
  (using TintStyle)
  (rule tint
    (param value I64)
    (= ink value)
  )
)

(type + Panel (is Box)
  (prop + amount _ (= 1))
  (prop + is-on _ (= true))
  (content
    (<> This
      (if #is-on
        (<> Mover (• tint #amount))
      )
    )
  )
)
```

```output
1
3
true
```

### A tracked name in a value a class's rule `• This` gives a rule it applies under a `when` is read again as it changes.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Mover (id: mover))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (. mover $ink))
  (= (. mover amount) 3)
  (call (. demo markup-batch-queue checkpoint))
  (print (. mover $ink))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
)

(type + Mover (is Box)
  (using TintStyle)
  (prop + amount _ (= 1))
  (prop + is-on _ (= true))
  (blank)
  (rule tint
    (param value I64)
    (= ink value)
  )
  (blank)
  (rule This
    (when-style #is-on
      (• tint #amount)
    )
  )
)
```

```output
1
3
```

### A tracked read in a value a rule the root `<This` of a class's markup applies is read again as it changes.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Mover (id: mover))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (. mover $ink))
  (= (. mover rect) (call Rect 0 0 3 1))
  (call (. demo markup-batch-queue checkpoint))
  (print (. mover $ink))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TintStyle (is Style)
  (prop + ink Coord (= 0))
)

(type + Mover (is Box)
  (using TintStyle)
  (prop + rect _ (= (call Rect 0 0 1 1)))
  (blank)
  (rule tint
    (param value Coord)
    (= ink value)
  )
  (content
    (<> This (• tint (. #rect width)))
  )
)
```

```output
1
3
```

### The values the root `<This` gives its rules, and those its rules give the rules they apply, are evaluated once, by the constructor.

```brix
(import /core/style)
(blank)

(main
  (let thing (call Thing))
  (print (. thing x))
  (print (. thing y))
)

(fn-expr + twice I64
  (param value I64)
  (before
    (print (str "twice " value))
  )
  (* value 2)
)

(type + Thing
  (prop + x _ (= 0))
  (prop + y _ (= 0))
  (blank)
  (rule set-x
    (param value I64)
    (= x value)
  )
  (blank)
  (rule set-y
    (param value I64)
    (= y value)
  )
  (blank)
  (rule doubled (• set-x (call twice 2)))
  (content
    (<> This (• doubled) (• set-y (call twice 3)))
  )
)
```

```output
twice 2
twice 3
4
6
```

### A style state that is not Bool names the property's type as the reference prints types.

```brix crash
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Panel (id: panel) (• lit))
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
)

(type + Panel (is Box)
  (using TintStyle)
  (prop + hovered _ (= 0))
  (blank)
  (rule lit
    (when-style hovered
      (= ink 3)
    )
  )
)
```

```output
Exception: Style state "hovered" must be a boolean property, but Panel.hovered is I64
<no file:20>
        (when-style hovered
        ^^^^^^^^^^^^^^^^^^
```

### The styles a markup element sees are those of the markup it is written in, not those another module's markup typed before it saw: each module's rule declares the property of its own style.

```brix
(import ./lib/first)
(import ./lib/second)
(blank)

(main
  (call show-first)
  (call show-second)
)
```

```brix file=lib/first.bx
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)

(fn-do + show-first _
  (<> Holder (id: holder)
    (<> Panel (id: panel))
  )
  (call (. holder markup-batch-queue checkpoint))
  (print (call style-authored-snapshot (!! (. panel %first-child))))
)

(type _ TintStyle (is Style)
  (prop + ink I64 (= 0))
  (blank)
  (style-domains ink (domain paint))
)

(type _ Holder (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type _ Panel (is Box)
  (using TintStyle)
  (rule This
    (<> Segment (• part))
  )
)

(type _ Segment (is Box)
  (using TintStyle)
  (rule part
    (= ink 1)
  )
)
```

```brix file=lib/second.bx
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)

(fn-do + show-second _
  (<> Holder (id: holder)
    (<> Panel (id: panel))
  )
  (call (. holder markup-batch-queue checkpoint))
  (print (call style-authored-snapshot (!! (. panel %first-child))))
)

(type _ TintStyle (is Style)
  (prop + ink I64 (= 0))
  (blank)
  (style-domains ink (domain paint))
)

(type _ Holder (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type _ Panel (is Box)
  (using TintStyle)
  (rule This
    (<> Segment (• part))
  )
)

(type _ Segment (is Box)
  (using TintStyle)
  (rule part
    (= ink 2)
  )
)
```

```output
node TintStyle.ink layer=2 order=0 kind=static bits=1
node TintStyle.ink layer=2 order=0 kind=static bits=2
```

### The values given to the rules written after the name of a rule an element applies are evaluated as the element is prepared, after its attributes, and again as the rules apply.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: panel) (• header) (• extra)
      (= note (call pick 5))
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (. panel $ink))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
)

(type + Panel (is Box)
  (using TintStyle)
  (prop + note I64 (= 0))
  (blank)
  (rule set-ink
    (param value I64)
    (= ink value)
  )
  (blank)
  (rule header (• mid) (• set-ink (call pick 1)))
  (blank)
  (rule mid (• set-ink (call pick 2)))
  (blank)
  (rule extra (• set-ink (call pick 3)))
)

(fn-do + pick I64
  (param n I64)
  (print (str "pick " n))
  (return n)
)
```

```output
pick 5
pick 2
pick 1
pick 3
pick 2
pick 1
pick 3
3
```

### An element whose body holds only its own style lines still generates the content of the rules it applies once it is mounted under its parent.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Panel (id: plain) (• trail))
    (<> Panel (id: lined) (• trail)
      (= ink 3)
    )
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call markup-is-style-shadow (!! (. plain %next-sibling))))
  (print (call markup-is-style-shadow (!! (. lined %next-sibling))))
  (print (. lined $ink))
)

(type + TintStyle (is Style)
  (prop + ink I64 (= 0))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Panel (is Box)
  (using TintStyle)
  (rule trail
    (<> Segment (slot: after))
  )
)

(type + Segment (is Box)
  (using TintStyle)
)
```

```output
true
true
3
```

### A line of a style rule that sets a property of its class from something tracked sets it again as that changes.

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let child (call Stateful 5))
  (print (. child text))
  (= (. child address) 7)
  (print (. child text))
  (= (. child address) 9)
  (print (. child text))
)

(type + Stateful (is Box)
  (prop + address I64)
  (prop + text _ (= ""))
  (blank)
  (rule This
    (= text (str "at " #address))
  )
  (fn-do + This _
    (param address _)
  )
)
```

```output
at 5
at 7
at 9
```

### A style rule applied in markup reads the names in its condition on the element it applies to.

```brix
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (lazy heavy false)
  (blank)
  (<> BatchDemo (id: demo)
    (<> Label (id: label) (• bold (if: #heavy)))
  )
  (blank)
  (= heavy true)
  (call (. demo markup-batch-queue checkpoint))
  (print (. label $ink))
  (= (. label heavy) true)
  (call (. demo markup-batch-queue checkpoint))
  (print (. label $ink))
)

(type + TextStyle (is Style)
  (prop + ink I64 (= 7))
  (blank)
  (style-domains ink (domain paint))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Label (is Box)
  (using TextStyle)
  (prop + heavy _ (= false))
  (blank)
  (rule bold
    (= ink 77)
  )
)
```

```output
7
77
```

### A main that makes an element of a class whose theme, in another module, gates the rule of an element of the class's markup by a state of the class runs.

```brix
(import /core/markup)
(import ./lib/widget)
(blank)

(main
  (<> Widget (id: widget))
  (blank)
  (print (. widget is-hot))
)
```

```brix file=lib/widget.bx
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)

(type + PaintStyle (is Style)
  (prop + ink I64 (= 0))
  (blank)
  (style-domains ink (domain paint))
)

(style WidgetTheme
  (using PaintStyle)
  (rule Widget
    (when-style #is-hot
      (rule leading
        (= ink 2)
      )
    )
  )
)

(type + Widget (is Box)
  (using PaintStyle)
  (using WidgetTheme)
  (prop + is-hot _ (= false))
  (content
    (<> This
      (<> Box (id: leading))
    )
  )
)
```

```output
false
```

### A line of a rule that reads the value the rule takes and a property of a style is no live one, as a style's property is not tracked: it declares once, where the rule is applied.

```brix
(import /core/style)
(import /core/style-debug)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Leaf (id: leaf) (• heavy 3))
  )
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (print (call style-computed-snapshot leaf))
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using TextStyle)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 (= 7))
  (prop + weight I64 (= 1))
)

(type + Leaf (is Box)
  (using TextStyle)
  (rule heavy
    (param value I64)
    (= weight (+ value $ink))
  )
)
```

```output
node TextStyle seg=owned gen=1 mask=2 ink=7 weight=10
```

```brix
(import /core/markup)
(import /core/style)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let root (call Root))
  (call markup-mount-root root root)
  (call (. root markup-batch-queue checkpoint))
  (let row (as (!! (. (!! (. root %first-child)) %first-child)) Thing))
  (print (. row x))
  (print (. row $span))
  (= (. root items) (list (call Item "a" 12)))
  (call (. root markup-batch-queue checkpoint))
  (print (. row x))
  (print (. row $span))
  (print (== row (!! (. (!! (. root %first-child)) %first-child))))
  (call (. root close))
)

(type + Item (is Tuple)
  (prop + id String)
  (prop + size Int)
)

(type + Thing (is Box)
  (using Sized)
  (prop + x Int)
  (blank)
  (rule sized
    (param value Int)
    (= x value)
    (= span value)
  )
)

(type + Sized (is Style)
  (prop + span Int)
  (blank)
  (style-domains span (domain layout))
)

(type + Root (is Box)
  (implements MarkupRootElement)
  (prop + items _ (= (list (call Item "a" 1))))
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (content
    (<> This
      (for (in item #items (keyed-by (. item id)))
        (<> Thing (• sized (. item size)))
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
1
1
12
12
true
```
