# Markup branches

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo)
    (if (. demo #flag)
      (<> TextBox (value: "on"))
      (else
        (<> TextBox (value: "off"))
      )
    )
  )
  (blank)
  (print (call dump demo))
  (= (. demo flag) false)
  (print (call dump demo))
  (= (. demo flag) true)
  (print (call dump demo))
)

(fn-expr + dump String
  (param demo Demo)
  (str count ":" text)
  (where
    (lazy region (!! (. demo %first-child)))
    (lazy count (call count-children region))
    (lazy text (. (as (!! (. region %first-child)) TextBox) text))
  )
)

(fn-do + count-children _
  (param box Box)
  (let count 0)
  (for (in child (. box children))
    (+= count 1)
  )
  (return count)
)

(type + Demo (is Box)
  (prop + flag _ (= true))
)
```

```output
1:on
1:off
1:on
```

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo)
    (if (. demo #flag)
      (<> TextBox (value: "shown"))
    )
  )
  (blank)
  (print (call dump demo))
  (= (. demo flag) false)
  (print (call dump demo))
  (= (. demo flag) true)
  (print (call dump demo))
)

(fn-do + dump String
  (param demo Demo)
  (if (== count 0)
    (return "0")
  )
  (return (str count ":" (. (as (!! (. region %first-child)) TextBox) text)))
  (blank)
  (lazy region (!! (. demo %first-child)))
  (lazy count (call count-children region))
)

(fn-do + count-children _
  (param box Box)
  (let count 0)
  (for (in child (. box children))
    (+= count 1)
  )
  (return count)
)

(type + Demo (is Box)
  (prop + flag _ (= true))
)
```

```output
1:shown
0
1:shown
```

```brix
(main
  (let flag true)
  (let cleanup-flag true)
  (let holder (call Holder (call MarkText "live")))
  (blank)
  (<> MarkNode (id: root)
    (if #flag
      (<> MarkText (value: "yes"))
      (else
        (<> MarkText (value: "no"))
      )
    )
  )
  (blank)
  (print (call dump root))
  (= flag false)
  (print (call dump root))
  (= flag true)
  (print (call dump root))
  (blank)
  (<> MarkNode (id: cleanup-root)
    (if #cleanup-flag
      (. holder #node)
      (else
        (<> MarkText (value: "off"))
      )
    )
  )
  (blank)
  (print (call dump cleanup-root))
  (= cleanup-flag false)
  (print (call dump cleanup-root))
  (= (. holder node) (call MarkText "stale"))
  (print (call dump cleanup-root))
)

(fn-expr + dump String
  (param node MarkNode)
  (str count ":" text)
  (where
    (lazy region (!! (. node first-child)))
    (lazy count (call count-children region))
    (lazy text (. (as (!! (. region first-child)) MarkText) text))
  )
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
    (if (not anchor)
      (if (not first-child)
        (= first-child (= last-child child))
        (else
          (= (?. last-child next) child)
          (= (. child previous) last-child)
          (= last-child child)
        )
      )
      (else
        (let target anchor)
        (= (. child previous) (. target previous))
        (= (. child next) target)
        (if (. target previous)
          (= (. target previous next) child)
          (else
            (= first-child child)
          )
        )
        (= (. target previous) child)
      )
    )
  )
  (fn-do + markup-detach-child _
    (param child MarkNode)
    (if (== (. child parent) this)
      (= (. child parent) null)
      (if (== first-child child)
        (= first-child (. child next))
      )
      (if (== last-child child)
        (= last-child (. child previous))
      )
      (= (?. (. child previous) next) (. child next))
      (= (?. (. child next) previous) (. child previous))
      (= (. child next) null)
      (= (. child previous) null)
    )
  )
  (fn-do + clear-children Void
    (while true
      (let maybe-child first-child)
      (if (not maybe-child)
        (break)
      )
      (let child maybe-child)
      (call markup-remove this child)
      (call (. child close))
    )
  )
  (fn-do + close Void
    (call markup-close-region this)
    (call clear-children)
  )
  (fn-expr + markup-make-region (? MarkNode)
    (call MarkNode)
  )
)

(type + MarkText (is MarkNode)
  (prop + text String)
  (fn-do + This _
    (param text String)
  )
)

(type + Holder
  (prop + node MarkNode)
  (fn-do + This _
    (param node _)
  )
)
```

```output
1:yes
1:no
1:yes
1:live
1:off
1:off
```

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo)
    (match (. demo #state)
      (case= "a"
        (<> TextBox (value: "alpha"))
      )
      (case= "b"
        (<> TextBox (value: "beta"))
      )
      (case _
        (<> TextBox (value: "fallback"))
      )
    )
  )
  (blank)
  (print (call dump demo))
  (= (. demo state) "b")
  (print (call dump demo))
  (= (. demo state) "z")
  (print (call dump demo))
)

(fn-expr + dump String
  (param demo Demo)
  (str count ":" text)
  (where
    (lazy region (!! (. demo %first-child)))
    (lazy count (call count-children region))
    (lazy text (. (as (!! (. region %first-child)) TextBox) text))
  )
)

(fn-do + count-children _
  (param box Box)
  (let count 0)
  (for (in child (. box children))
    (+= count 1)
  )
  (return count)
)

(type + Demo (is Box)
  (prop + state _ (= "a"))
)
```

```output
1:alpha
1:beta
1:fallback
```

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo)
    (match (. demo #state)
      (case (: text String)
        (<> TextBox (value: text))
      )
    )
  )
  (blank)
  (print (call dump demo))
  (= (. demo state) "changed")
  (print (call dump demo))
)

(fn-expr + dump String
  (param demo Demo)
  (str count ":" text)
  (where
    (lazy region (!! (. demo %first-child)))
    (lazy count (call count-children region))
    (lazy text (. (as (!! (. region %first-child)) TextBox) text))
  )
)

(fn-do + count-children _
  (param box Box)
  (let count 0)
  (for (in child (. box children))
    (+= count 1)
  )
  (return count)
)

(type + Demo (is Box)
  (prop + state _ (= "initial"))
)
```

```output
1:initial
1:changed
```

```brix crash
(main
  (let flag true)
  (blank)
  (<> Plain (id: root)
    (if #flag
      (<> Plain)
    )
  )
)

(type + Plain
  (fn-expr + (infix this += (param child Plain)) _)
)
```

```output
Exception: Dynamic markup branch parent must implement MarkupElement
<no file:6>
        (if #flag
        ^^^^^^^^
```

```brix crash
(main
  (let flag true)
  (blank)
  (<> MarkNode (id: root)
    (if #flag
      "bad"
    )
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
  (fn-do + close _
    (call markup-close-region this)
  )
  (fn-expr + markup-make-region (? MarkNode)
    (call MarkNode)
  )
)
```

```output
Exception: Content not accepted
<no file:7>
          "bad"
          ^^^^^
```

### Storing an equal tuple into a tracked property keeps branch children and keyed rows in place.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo)
    (if (. demo #state enabled)
      (<> TextBox (id: branch) (value: "on"))
    )
    (for (in index (..< 0 (. demo #state count)) (keyed-by index))
      (<> TextBox (value: "row"))
    )
  )
  (blank)
  (let region (!! (. demo %first-child)))
  (let original (!! (. region %first-child)))
  (let original-rows (call (. demo row-pointers)))
  (= (. demo state) (call DemoState true 2))
  (print (== (as (!! (. region %first-child)) Pointer) (as original Pointer)))
  (print (== (call (. demo row-pointers)) original-rows))
  (= (. demo state) (call DemoState true 3))
  (print (== (as (!! (. region %first-child)) Pointer) (as original Pointer)))
)

(type + DemoState (is Tuple)
  (prop + enabled Bool)
  (prop + count I32)
)

(type + Demo (is Box)
  (prop + state _ (= (call DemoState true 2)))
  (fn-do + row-pointers (list Pointer)
    (let result (list Pointer))
    (let first true)
    (for (in child children)
      (if first
        (= first false)
        (else
          (for (in row (. child children))
            (+= result (as row Pointer))
          )
        )
      )
    )
    (return result)
  )
)
```

```output
true
true
true
```

### An if nested in another if's else arm follows its own condition while the outer arm is kept.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let demo (call Demo))
  (print (call (. demo label)))
  (= (. demo visible) false)
  (print (call (. demo label)))
  (= (. demo removing) true)
  (print (call (. demo label)))
  (= (. demo removing) false)
  (print (call (. demo label)))
  (= (. demo visible) true)
  (print (call (. demo label)))
)

(type + Demo (is Box)
  (prop + removing _ (= false))
  (prop + visible _ (= true))
  (content
    (<> This
      (if #removing
        (<> TextBox (value: "remove"))
        (else
          (if #visible
            (<> TextBox (value: "shown"))
            (else
              (<> TextBox (value: "hidden"))
            )
          )
        )
      )
    )
  )
  (fn-do + label String
    (let texts (list String))
    (call collect this texts)
    (return (call (. "," join) texts))
  )
  (fn-do + collect _
    (param node Box)
    (param texts (list String))
    (for (in child (. node children))
      (if (is child TextBox)
        (+= texts (. (as child TextBox) text))
      )
      (call collect child texts)
    )
  )
)
```

```output
shown
hidden
remove
hidden
shown
```

### An if nested in another if's then arm of class markup follows its own condition.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let demo (call Demo))
  (print (call (. demo label)))
  (= (. demo visible) false)
  (print (call (. demo label)))
)

(type + Demo (is Box)
  (prop + shown _ (= true))
  (prop + visible _ (= true))
  (content
    (<> This
      (if #shown
        (if #visible
          (<> TextBox (value: "shown"))
          (else
            (<> TextBox (value: "hidden"))
          )
        )
      )
    )
  )
  (fn-do + label String
    (let texts (list String))
    (call collect this texts)
    (return (call (. "," join) texts))
  )
  (fn-do + collect _
    (param node Box)
    (param texts (list String))
    (for (in child (. node children))
      (if (is child TextBox)
        (+= texts (. (as child TextBox) text))
      )
      (call collect child texts)
    )
  )
)
```

```output
shown
hidden
```

### An if nested in an else arm of top-level markup follows its own condition.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo)
    (if (. demo #removing)
      (<> TextBox (value: "remove"))
      (else
        (if (. demo #visible)
          (<> TextBox (value: "shown"))
          (else
            (<> TextBox (value: "hidden"))
          )
        )
      )
    )
  )
  (blank)
  (print (call (. demo label)))
  (= (. demo visible) false)
  (print (call (. demo label)))
)

(type + Demo (is Box)
  (prop + removing _ (= false))
  (prop + visible _ (= true))
  (fn-do + label String
    (let texts (list String))
    (call collect this texts)
    (return (call (. "," join) texts))
  )
  (fn-do + collect _
    (param node Box)
    (param texts (list String))
    (for (in child (. node children))
      (if (is child TextBox)
        (+= texts (. (as child TextBox) text))
      )
      (call collect child texts)
    )
  )
)
```

```output
shown
hidden
```

### An if inside a loop row follows the element's own property.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let demo (call Demo))
  (print (call (. demo label)))
  (= (. demo visible) false)
  (print (call (. demo label)))
)

(type + Demo (is Box)
  (prop + items (list String))
  (prop + visible _ (= true))
  (fn-do + This _
    (= items (call (of List String) (list "a" "b")))
  )
  (content
    (<> This
      (for (in item #items)
        (<> Box
          (if #visible
            (<> TextBox (value: "shown"))
            (else
              (<> TextBox (value: "hidden"))
            )
          )
        )
      )
    )
  )
  (fn-do + label String
    (let texts (list String))
    (call collect this texts)
    (return (call (. "," join) texts))
  )
  (fn-do + collect _
    (param node Box)
    (param texts (list String))
    (for (in child (. node children))
      (if (is child TextBox)
        (+= texts (. (as child TextBox) text))
      )
      (call collect child texts)
    )
  )
)
```

```output
shown,shown
hidden,hidden
```

### A named child inside an else-if or else branch is a property of its owner.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let picker (call Picker))
  (= (. picker mode) 1)
  (print (. picker one-box text))
  (= (. picker mode) 3)
  (print (. picker three-box text))
  (= (. picker mode) 4)
  (print (. picker four-box text))
)

(type + Picker (is Box)
  (prop + mode _ (= 0))
  (content
    (<> This
      (if (== #mode 0)
        (<> TextBox (id: zero-box) (value: "zero"))
        (else-if (== #mode 1)
          (<> TextBox (id: one-box) (value: "one"))
        )
        (else-if (== #mode 2)
          (<> TextBox (id: two-box) (value: "two"))
        )
        (else-if (== #mode 3)
          (<> TextBox (id: three-box) (value: "three"))
        )
        (else
          (<> TextBox (id: four-box) (value: "four"))
        )
      )
    )
  )
)
```

```output
one
three
four
```

### A named element in an if nested in another if's else arm is assigned to its host.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let demo (call Demo))
  (print (call (. demo label)))
  (= (. demo editing) true)
  (print (call (. demo label)))
  (= (. demo editing) false)
  (print (call (. demo label)))
)

(type + Demo (is Box)
  (prop + editing _ (= false))
  (prop + extra _ (= true))
  (prop + name _ (= "row"))
  (content
    (<> This
      (if #editing
        (<> TextBox (value: (str "entry " #name)))
        (else
          (<> TextBox (value: #name))
          (if #extra
            (<> TextBox (id: extra-box) (value: (str "extra " #name)))
          )
        )
      )
    )
  )
  (fn-do + label String
    (let texts (list String))
    (call collect this texts)
    (return (str (call (. "," join) texts) ":" (. extra-box text)))
  )
  (fn-do + collect _
    (param node Box)
    (param texts (list String))
    (for (in child (. node children))
      (if (is child TextBox)
        (+= texts (. (as child TextBox) text))
      )
      (call collect child texts)
    )
  )
)
```

```output
row,extra row:extra row
entry row:extra row
row,extra row:extra row
```

### A local tracked by an if and again inside its arm keeps one subscriber list.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let note (call Note "one"))
  (blank)
  (<> Box (id: demo)
    (if (> (. note #text size) 0)
      (<> TextBox (value: (. note #text)))
    )
  )
  (blank)
  (print (call label demo))
  (= (. note text) "")
  (print (call label demo))
  (= (. note text) "back")
  (print (call label demo))
)

(type + Note
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(fn-do + label String
  (param node Box)
  (let texts (list String))
  (for (in child (. node children))
    (for (in inner (. child children))
      (if (is inner TextBox)
        (+= texts (. (as inner TextBox) text))
      )
    )
  )
  (return (str (. texts length) ":" (call (. "," join) texts)))
)
```

```output
1:one
0:
1:back
```

### An if nested in a match arm follows a property of the arm's binding.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let demo (call Demo))
  (print (call (. demo label)))
  (= (. (as (. demo item) Note) text) "")
  (print (call (. demo label)))
  (= (. (as (. demo item) Note) text) "back")
  (print (call (. demo label)))
  (= (. demo item) (call Mark 5))
  (print (call (. demo label)))
  (= (. demo item) (call Note "again"))
  (print (call (. demo label)))
)

(type + Item (is Union)
  (variant Note)
  (variant Mark)
)

(type + Note
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(type + Mark
  (prop + count Int)
  (fn-do + This _
    (param count _)
  )
)

(type + Demo (is Box)
  (prop + item Item (= (call Note "one")))
  (content
    (<> This
      (match #item
        (case (: note Note)
          (if (> (. note #text size) 0)
            (<> TextBox (value: (. note #text)))
          )
        )
        (case (: mark Mark)
          (<> TextBox (value: (str "mark " (. mark #count))))
        )
      )
    )
  )
  (fn-do + label String
    (let texts (list String))
    (call collect this texts)
    (return (str (. texts length) ":" (call (. "," join) texts)))
  )
  (fn-do + collect _
    (param node Box)
    (param texts (list String))
    (for (in child (. node children))
      (if (is child TextBox)
        (+= texts (. (as child TextBox) text))
      )
      (call collect child texts)
    )
  )
)
```

```output
1:one
0:
1:back
1:mark 5
1:again
```

### A pattern condition binds its arm and follows the tracked subject.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let demo (call Demo))
  (print (call (. demo label)))
  (= (. (as (. demo item) Note) text) "mutated")
  (print (call (. demo label)))
  (= (. demo item) (call Note "replaced"))
  (print (call (. demo label)))
  (= (. demo item) (call Mark 5))
  (print (call (. demo label)))
)

(type + Item (is Union)
  (variant Note)
  (variant Mark)
)

(type + Note
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(type + Mark
  (prop + count Int)
  (fn-do + This _
    (param count _)
  )
)

(type + Demo (is Box)
  (prop + item Item (= (call Note "one")))
  (content
    (<> This
      (if (= (| (: note Note)) #item)
        (<> TextBox (value: (. note #text)))
        (else-if (= (| (: mark Mark)) #item)
          (<> TextBox (value: (str "mark " (. mark #count))))
        )
      )
    )
  )
  (fn-do + label String
    (let texts (list String))
    (call collect this texts)
    (return (str (. texts length) ":" (call (. "," join) texts)))
  )
  (fn-do + collect _
    (param node Box)
    (param texts (list String))
    (for (in child (. node children))
      (if (is child TextBox)
        (+= texts (. (as child TextBox) text))
      )
      (call collect child texts)
    )
  )
)
```

```output
1:one
1:mutated
1:replaced
1:mark 5
```

### A pattern condition shares a branch chain with plain conditions.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let demo (call Demo))
  (print (call (. demo label)))
  (= (. demo is-hidden) true)
  (print (call (. demo label)))
  (= (. demo item) (call Mark 1))
  (print (call (. demo label)))
  (= (. demo is-hidden) false)
  (print (call (. demo label)))
  (= (. demo item) (call Note "back"))
  (print (call (. demo label)))
)

(type + Item (is Union)
  (variant Note)
  (variant Mark)
)

(type + Note
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(type + Mark
  (prop + count Int)
  (fn-do + This _
    (param count _)
  )
)

(type + Demo (is Box)
  (prop + item Item (= (call Note "one")))
  (prop + is-hidden _ (= false))
  (content
    (<> This
      (if #is-hidden
        (<> TextBox (value: "hidden"))
        (else-if (= (| (: note Note)) #item)
          (<> TextBox (value: (. note #text)))
        )
        (else
          (<> TextBox (value: "other"))
        )
      )
    )
  )
  (fn-do + label String
    (let texts (list String))
    (call collect this texts)
    (return (str (. texts length) ":" (call (. "," join) texts)))
  )
  (fn-do + collect _
    (param node Box)
    (param texts (list String))
    (for (in child (. node children))
      (if (is child TextBox)
        (+= texts (. (as child TextBox) text))
      )
      (call collect child texts)
    )
  )
)
```

```output
1:one
1:hidden
1:hidden
1:other
1:back
```

### A pattern condition over an untracked subject picks its arm once.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let first (call Demo (call Note "one") false))
  (let second (call Demo (call Mark 5) false))
  (let third (call Demo (call Mark 5) true))
  (print (call (. first label)))
  (print (call (. second label)))
  (print (call (. third label)))
  (= (. (as (. first item) Note) text) "mutated")
  (= (. first item) (call Mark 7))
  (print (call (. first label)))
)

(type + Item (is Union)
  (variant Note)
  (variant Mark)
)

(type + Note
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(type + Mark
  (prop + count Int)
  (fn-do + This _
    (param count _)
  )
)

(type + Demo (is Box)
  (prop + item Item)
  (prop + is-plain Bool)
  (fn-do + This _
    (param item _)
    (param is-plain _)
  )
  (content
    (<> This
      (if is-plain
        (<> TextBox (value: "plain"))
        (else-if (= (| (: note Note)) item)
          (<> TextBox (value: (. note #text)))
        )
        (else-if (= (| (: mark Mark)) item)
          (<> TextBox (value: (str "mark " (. mark count))))
        )
      )
    )
  )
  (fn-do + label String
    (let texts (list String))
    (call collect this texts)
    (return (str (. texts length) ":" (call (. "," join) texts)))
  )
  (fn-do + collect _
    (param node Box)
    (param texts (list String))
    (for (in child (. node children))
      (if (is child TextBox)
        (+= texts (. (as child TextBox) text))
      )
      (call collect child texts)
    )
  )
)
```

```output
1:one
1:mark 5
1:plain
1:mutated
```

### A tracked member read through a cast follows the member and the cast subject.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let demo (call Demo))
  (print (call (. demo label)))
  (= (. (as (. demo item) Note) text) "mutated")
  (print (call (. demo label)))
  (= (. demo item) (call Note "replaced"))
  (print (call (. demo label)))
)

(type + Item (is Union)
  (variant Note)
  (variant Mark)
)

(type + Note
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(type + Mark
  (prop + count Int)
  (fn-do + This _
    (param count _)
  )
)

(type + Demo (is Box)
  (prop + item Item (= (call Note "one")))
  (content
    (<> This
      (<> TextBox (value: (. (as #item Note) #text)))
    )
  )
  (fn-do + label String
    (let texts (list String))
    (call collect this texts)
    (return (str (. texts length) ":" (call (. "," join) texts)))
  )
  (fn-do + collect _
    (param node Box)
    (param texts (list String))
    (for (in child (. node children))
      (if (is child TextBox)
        (+= texts (. (as child TextBox) text))
      )
      (call collect child texts)
    )
  )
)
```

```output
1:one
1:mutated
1:replaced
```

### A class-level subscription follows a named element that a dynamic branch builds after mount, hearing each rebuilt element once and none that the branch removed.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo))
  (= (. demo shows) true)
  (let first (. demo pinger))
  (<- (. first pinged) 1)
  (print (str "late=" (. demo pings)))
  (blank)
  (= (. demo shows) false)
  (= (. demo shows) true)
  (<- (. demo pinger pinged) 1)
  (print (str "rebuilt=" (. demo pings)))
  (blank)
  (<- (. first pinged) 1)
  (print (str "removed=" (. demo pings)))
)

(type + Pinger (is Box)
  (event + pinged Int)
)

(type + Demo (is Box)
  (prop + shows _ (= false))
  (prop + pings _ (= 0))
  (content
    (<> This
      (if #shows
        (<> Pinger (id: pinger))
      )
    )
  )
  (subscribe-for (on event (. pinger pinged))
    (+= pings 1)
  )
)
```

```output
late=1
rebuilt=2
removed=2
```

### A named element a dynamic branch of a class's markup makes is given to the class's property of that name each time the branch makes it.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let section (call Section))
  (print (. section header title))
  (= (. section title) "B")
  (print (. section header title))
  (= (. section title) "")
  (print (. section header title))
  (= (. section title) "C")
  (print (. section header title))
)

(type + Titled (is Box)
  (prop + title _ (= ""))
)

(type + Section (is Box)
  (prop + title _ (= "A"))
  (content
    (<> This
      (if (!= #title "")
        (<> Titled (id: header)
          (= title #title)
        )
      )
    )
  )
)
```

```output
A
B
B
C
```

### A named element of a branch the markup has not made panics where it is read.

```brix crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let demo (call Demo))
  (print (. demo panel syncs))
)

(type + Panel (is Box)
  (prop + syncs Size (= 0))
)

(type + Demo (is Box)
  (prop + shows _ (= false))
  (content
    (<> This
      (if #shows
        (<> Panel (id: panel))
      )
    )
  )
)
```

```stderr
Unhandled panic: Named markup element "panel" is read before its markup constructs it
<no file:7>

Task 1

Stack trace:
  0  __source__:main no file:7:13
  1  __source__:init.__source__ no file
  2  __source__:brix.main no file
```

### A branch arm of top-level markup reads a property of the element around it by its name alone, and follows it while the arm is shown.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo)
    (if (. demo #shown)
      (<> TextBox (value: (. demo #label)))
    )
  )
  (blank)
  (print (call (. demo text)))
  (= (. demo label) "two")
  (print (call (. demo text)))
  (= (. demo shown) false)
  (print (call (. demo text)))
  (= (. demo shown) true)
  (print (call (. demo text)))
)

(type + Demo (is Box)
  (prop + shown _ (= true))
  (prop + label _ (= "one"))
  (fn-do + text String
    (let region (!! (. this %first-child)))
    (return (if-expr (not (. region %first-child)) "none" (. (as (!! (. region %first-child)) TextBox) text)))
  )
)
```

```output
one
two
none
two
```

### A class subscription to an element a branch removes ends when the branch removes it

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo))
  (= (. demo shows) true)
  (let first (. demo pinger))
  (<- (. first pinged) 1)
  (print (str "shown=" (. demo pings)))
  (= (. demo shows) false)
  (<- (. first pinged) 1)
  (print (str "hidden, old element pinged=" (. demo pings)))
  (= (. demo shows) true)
  (<- (. first pinged) 1)
  (print (str "rebuilt, old element pinged=" (. demo pings)))
  (<- (. demo pinger pinged) 1)
  (print (str "rebuilt, new element pinged=" (. demo pings)))
)

(type + Pinger (is Box)
  (event + pinged Int)
)

(type + Demo (is Box)
  (prop + shows _ (= false))
  (prop + pings _ (= 0))
  (content
    (<> This
      (if #shows
        (<> Pinger (id: pinger))
      )
    )
  )
  (subscribe-for (on event (. pinger pinged))
    (+= pings 1)
  )
)
```

```output
shown=1
hidden, old element pinged=1
rebuilt, old element pinged=1
rebuilt, new element pinged=2
```

### A class loop on an event nothing publishes, of an element a branch builds, subscribes nothing

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let demo (call Demo))
  (print "constructed")
  (= (. demo shows) true)
  (print (str "shown pings=" (. demo pings)))
)

(type + Pinger (is Box)
  (event + pinged Int)
)

(type + Demo (is Box)
  (prop + shows _ (= false))
  (prop + pings _ (= 0))
  (content
    (<> This
      (if #shows
        (<> Pinger (id: pinger))
      )
    )
  )
  (subscribe-for (on event (. pinger pinged))
    (+= pings 1)
  )
)
```

```output
constructed
shown pings=0
```

### A tracked property tested in a conditional expression holds a value in the branch that found it.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo))
  (blank)
  (print (. (as (!! (. demo %first-child)) TextBox) text))
  (let tag (call Tag))
  (= (. tag label) "ada")
  (= (. demo tag) tag)
  (print (. (as (!! (. demo %first-child)) TextBox) text))
)

(type + Tag
  (prop + label _ (= ""))
)

(type + Demo (is Box)
  (prop + tag (? Tag))
  (content
    (<> This
      (<> TextBox
        (= text (if-expr #tag (. #tag label) "none"))
      )
    )
  )
)
```

```output
none
ada
```

### A departing parent waits for its leaving child, then closes with it.

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)
(prop + log _ (= (call (of List String))))
(blank)

(main
  (<> Demo (id: demo)
    (if (. demo #shown)
      (<> Box
        (for (in name (. demo #names) (keyed-by name))
          (<> Leaver
            (= name name)
          )
        )
      )
    )
  )
  (blank)
  (call (. demo names remove) 0)
  (print (call dump demo))
  (= (. demo shown) false)
  (print (call dump demo))
  (call markup-departed (!! (. (!! (. (!! (. (!! (. demo %first-child)) %first-child)) %first-child)) %first-child)))
  (print (call dump demo))
  (print (call (. " " join) log))
)

(fn-do + dump String
  (param demo Demo)
  (let branch (!! (. demo %first-child)))
  (return-if "empty" (if (not (. branch %first-child))))
  (let parts (call (of List String)))
  (for (in child (. (!! (. (!! (. branch %first-child)) %first-child)) children))
    (let leaver (as child Leaver))
    (if (. leaver is-leaving)
      (+= parts (str (. leaver name) "*"))
      (else
        (+= parts (. leaver name))
      )
    )
  )
  (let text (call (. " " join) parts))
  (return-if (str "(" text ")") (if (call markup-is-departing (!! (. branch %first-child)))))
  (return text)
)

(type + Demo (is Box)
  (prop + shown _ (= true))
  (prop + names _ (= (call (of List String) (list "a" "b"))))
)

(type + Leaver (is Box)
  (prop + name _ (= ""))
  (prop + is-leaving _ (= false))
  (fn-do + markup-depart Bool
    (= is-leaving true)
    (return true)
  )
  (fn-do + markup-revive _
    (= is-leaving false)
  )
  (fn-do + close _
    (+= log (str "close:" name))
    (call (. super close))
  )
)
```

```output
a* b
(a* b)
empty
close:a close:b
```

### A branch that is shown again while its old arm departs revives that arm instead of building it.

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)
(prop + log _ (= (call (of List String))))
(blank)

(main
  (<> Demo (id: demo)
    (if (. demo #shown)
      (<> Leaver)
    )
  )
  (blank)
  (= (. demo shown) false)
  (print (call dump demo))
  (= (. demo shown) true)
  (print (call dump demo))
  (= (. demo shown) false)
  (call markup-departed (!! (. (!! (. demo %first-child)) %first-child)))
  (print (call dump demo))
  (print (call (. " " join) log))
)

(fn-do + dump String
  (param demo Demo)
  (let parts (call (of List String)))
  (for (in child (. (!! (. demo %first-child)) children))
    (if (. (as child Leaver) is-leaving)
      (+= parts "leaving")
      (else
        (+= parts "shown")
      )
    )
  )
  (return (str "[" (call (. " " join) parts) "]"))
)

(type + Demo (is Box)
  (prop + shown _ (= true))
)

(type + Leaver (is Box)
  (prop + is-leaving _ (= false))
  (fn-do + This _
    (+= log "build")
  )
  (fn-do + markup-depart Bool
    (= is-leaving true)
    (return true)
  )
  (fn-do + markup-revive _
    (= is-leaving false)
  )
  (fn-do + close _
    (+= log "close")
    (call (. super close))
  )
)
```

```output
[leaving]
[shown]
[]
build close
```

### A branch shown again after only part of its old arm departed builds the arm again.

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)
(prop + log _ (= (call (of List String))))
(blank)

(main
  (<> Demo (id: demo)
    (if (. demo #shown)
      (<> Leaver)
      (<> Plain)
    )
  )
  (blank)
  (= (. demo shown) false)
  (print (call dump demo))
  (= (. demo shown) true)
  (print (call dump demo))
  (print (call (. " " join) log))
)

(fn-do + dump String
  (param demo Demo)
  (let parts (call (of List String)))
  (for (in child (. (!! (. demo %first-child)) children))
    (match child
      (case (: leaver Leaver)
        (+= parts (if-expr (. leaver is-leaving) "leaving" "leaver"))
      )
      (case _
        (+= parts "plain")
      )
    )
  )
  (return (str "[" (call (. " " join) parts) "]"))
)

(type + Demo (is Box)
  (prop + shown _ (= true))
)

(type + Leaver (is Box)
  (prop + is-leaving _ (= false))
  (fn-do + This _
    (+= log "leaver")
  )
  (fn-do + markup-depart Bool
    (= is-leaving true)
    (return true)
  )
  (fn-do + markup-revive _
    (= is-leaving false)
  )
)

(type + Plain (is Box)
  (fn-do + This _
    (+= log "plain")
  )
)
```

```output
[leaving]
[leaving leaver plain]
leaver plain leaver plain
```

### Bindings in a departing arm hold still while it leaves, and catch up when it revives.

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> HeldRoot (id: root))
  (let demo (call Demo))
  (call markup-adopt demo null root)
  (call (. root markup-batch-queue checkpoint))
  (print (call dump demo))
  (= (. demo item) null)
  (call (. root markup-batch-queue checkpoint))
  (print (call dump demo))
  (= (. demo item) (call Item "second"))
  (call (. root markup-batch-queue checkpoint))
  (print (call dump demo))
)

(fn-do + dump String
  (param demo Demo)
  (let leaver (as (!! (. (!! (. demo %first-child)) %first-child)) Leaver))
  (let state (if-expr (. leaver is-leaving) "leaving" "shown"))
  (return (str state " " (. (as (!! (. leaver %first-child)) TextBox) text)))
)

(type + Item (is Tuple)
  (prop + name String)
)

(type + HeldRoot (is Box)
  (implements MarkupRootElement)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue (as this MarkupRootElement))))
  (fn-do + This _
    (call markup-mount-root this this)
  )
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + Demo (is Box)
  (prop + item (? Item) (= (call Item "first")))
  (content
    (<> This
      (if (!= #item null)
        (<> Leaver
          (<> TextBox
            (= text (. (!! #item) name))
          )
        )
      )
    )
  )
)

(type + Leaver (is Box)
  (prop + is-leaving _ (= false))
  (fn-do + markup-depart Bool
    (= is-leaving true)
    (return true)
  )
  (fn-do + markup-revive _
    (= is-leaving false)
  )
)
```

```output
shown first
leaving first
shown second
```

### A line of an arm that forces a tracked optional gives the arm what it holds again as it changes.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Holder (id: holder)
    (= item (call Box))
  )
  (blank)
  (let region (!! (. holder %first-child)))
  (print (== (as (!! (. region %first-child)) Pointer) (as (!! (. holder item)) Pointer)))
  (= (. holder item) (call Box))
  (print (== (as (!! (. region %first-child)) Pointer) (as (!! (. holder item)) Pointer)))
)

(type + Holder (is Box)
  (prop + item (? Box))
  (content
    (<> This
      (if #item
        (!! #item)
      )
    )
  )
)
```

```output
true
true
```

### With no batch to order them, an arm's binding that forces what the branch tests is not run once that is gone.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Holder (id: holder))
  (blank)
  (print (call shown holder))
  (= (. holder item) null)
  (print (call shown holder))
)

(fn-expr + shown String
  (param holder Holder)
  (if-expr named (. (as named Named) name) "none")
  (where
    (lazy named (. (!! (. holder %first-child)) %first-child))
  )
)

(type + Item (is Tuple)
  (prop + name String)
)

(type + Holder (is Box)
  (prop + item (? Item) (= (call Item "first")))
  (content
    (<> This
      (if (!= #item null)
        (<> Named
          (= name (. (!! #item) name))
        )
      )
    )
  )
)

(type + Named (is Box)
  (prop + name _ (= ""))
)
```

```output
first
none
```

### A branch closes what its arm embedded when the arm leaves, so an arm that embeds a box held elsewhere panics when it is built again, though the box is still linked inside what closed.

```brix crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Holder (id: holder))
  (blank)
  (= (. holder open) false)
  (= (. holder open) true)
)

(type + Holder (is Box)
  (prop + open _ (= true))
  (prop + empty _ (= (call Box)))
  (content
    (<> This
      (if #open
        (<> Box
          #empty
        )
      )
    )
  )
)
```

```stderr
Unhandled panic: Markup child was closed by the markup that embedded it and cannot be embedded again
<core/markup.bx>

Task 1

Stack trace:
  0  core/markup:markup-assert-structural-child-linkable core/markup.bx
  1  core/markup:markup-attach-new-child core/markup.bx
  2  core/markup:markup-append core/markup.bx
  3  __source__:markup-branch-arm no file:19:11
  4  __source__:markup-branch-armCasting no file:17:7
  5  core/markup:MarkupBranchController:select-markup-branch core/markup.bx
  6  core/markup:markup-select-branch-region core/markup.bx
  7  __source__:markup-branch no file:17:7
  8  __source__:markup-branchCasting no file:17:7
  9  core/tracker:LiveAction:refresh-trackers-into core/tracker.bx
  10  core/tracker:LiveAction:refresh-trackers core/tracker.bx
  11  core/markup:MarkupBatchAction:run-markup-batch-action-now core/markup.bx
  12  core/markup:MarkupBatchAction:run-markup-batch-action core/markup.bx
  13  core/markup:MarkupBatchAction:refresh-markup-batch-action core/markup.bx
  14  core/markup:MarkupBatchAction:run-now core/markup.bx
  15  core/tracker:LiveAction:run-queued core/tracker.bx
  16  core/tracker:LiveBatch:run core/tracker.bx
  17  core/tracker:LiveBatch:leave core/tracker.bx
  18  core/tracker:property-dispatch core/tracker.bx
  19  __source__:main no file:9:3
  20  __source__:init.__source__ no file
  21  __source__:brix.main no file
```

### Making the arm a match selects is not tracked by the match's action. An element that reads a style-sheet slot while it is made does not run the match again when that read settles after mounting.

```brix
(import /core/markup)
(import /core/style)
(import /core/tests/support/markup-fixtures)
(blank)
(prop + builds _ (= 0))
(blank)

(main
  (<> Demo (id: demo))
  (blank)
  (call (. demo markup-batch-queue checkpoint))
  (= (. demo kind) 2)
  (call (. demo markup-batch-queue checkpoint))
  (print (str "builds=" builds " pending=" (. demo markup-batch-queue actions length)))
)

(type + Demo (is Box)
  (implements MarkupRootElement)
  (using AppTheme)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (prop + kind _ (= 1))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
  (content
    (<> This
      (match #kind
        (case 1
          (<> Row
            (= value 1)
          )
        )
        (case _
          (<> Row
            (= value 2)
          )
        )
      )
    )
  )
)

(style AppTheme
  (prop + bright I64 (= 11))
)

(type + TextStyle (is Style)
  (prop + ink I64 (= 7))
  (blank)
  (style-domains ink (domain paint))
)

(fn-expr + row-ink I64
  (param row Row)
  (. row $bright)
)

(type + Row (is Box)
  (using AppTheme)
  (using TextStyle)
  (prop + value I64)
  (prop + bright-at-build I64)
  (fn-do + This _
    (+= builds 1)
    (= bright-at-build $bright)
  )
  (content
    (<> This
      (= ink (+ (call row-ink this) #value))
    )
  )
)
```

```output
builds=2 pending=0
```

### An object's `close` closes the live actions it owns before its body closes its children, so a child that changes what a branch reads as it closes does not select an arm of a region that has closed.

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo)
    (if (not (. panel #is-open))
      (<> Box)
    )
    (<> Panel (id: panel))
  )
  (blank)
  (= (. panel is-open) true)
  (call (. demo close))
  (print (str "closed " (. panel is-open)))
)

(type + Demo (is Box))

(type + Panel (is Box)
  (prop + is-open _ (= false))
  (fn-do + close _
    (= is-open false)
    (call (. super close))
  )
)
```

```output
closed false
```

### A named element in an arm of a match that is a region is made by the arm, so a later line of the arm reads it.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let model (call Model))
  (blank)
  (<> Box (id: root)
    (match (. model #name)
      (case (: name String)
        (<> TextBox (id: first) (value: name))
        (<> TextBox (value: (str (. first text) "!")))
      )
      (case _
        (<> TextBox (id: none) (value: "none"))
        (<> TextBox (value: (str (. none text) "?")))
      )
    )
  )
  (blank)
  (print (call dump root))
  (= (. model name) null)
  (print (call dump root))
  (= (. model name) "back")
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

(type + Model
  (prop + name (? String) (= "on"))
)
```

```output
[on,on!]
[none,none?]
[back,back!]
```

### A handler in a named element of an arm of a match in a keyed row reads the element by its name.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let heard (call (of List String)))
  (let places (call (of List Place) (list (call Place "a") (call Place null))))
  (blank)
  (<> Box (id: root)
    (for (in place #places (keyed-by place))
      (match (. place name)
        (case (: name String)
          (<> Pinger (id: named)
            (= label name)
            (for (on event pinged)
              (+= heard (str "named " (. named label)))
            )
          )
        )
        (case _
          (<> Pinger (id: plain)
            (= label "plain")
            (for (on event pinged)
              (+= heard (str "plain " (. plain label)))
            )
          )
        )
      )
    )
  )
  (blank)
  (call ping root)
  (print (call (. "," join) heard))
)

(fn-do + ping _
  (param box Box)
  (for (in child (. box children))
    (if (is child Pinger)
      (<- (. (as child Pinger) pinged) 1)
    )
    (call ping child)
  )
)

(type + Pinger (is Box)
  (prop + label _ (= ""))
  (event + pinged Int)
)

(type + Place
  (prop + name (? String))
  (fn-do + This _
    (param name _)
  )
)
```

```output
named a,plain plain
```

### A named element an arm of a match in a class's markup makes is given to the class's property of that name each time the arm makes it.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let section (call Section))
  (print (. section header title))
  (= (. section title) "B")
  (print (. section header title))
  (= (. section title) null)
  (print (. section header title))
  (= (. section title) "C")
  (print (. section header title))
)

(type + Titled (is Box)
  (prop + title _ (= ""))
)

(type + Section (is Box)
  (prop + title (? String) (= "A"))
  (content
    (<> This
      (match #title
        (case (: text String)
          (<> Titled (id: header)
            (= title text)
          )
        )
        (case _
          (<> Box)
        )
      )
    )
  )
)
```

```output
A
B
B
C
```
