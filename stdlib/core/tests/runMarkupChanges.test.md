# Markup changes

```brix
(main
  (<> Foo (id: a)
    (<> Foo (id: b)
      (= name "B")
    )
    (<> Foo (id: c)
      (= name (. b #name))
    )
  )
  (blank)
  (print (. c name))
  (= (. b name) "BB")
  (print (. c name))
  (= (. b name) "BBB")
  (print (. c name))
)

(type + Foo
  (prop + name String)
  (prop + contents (? Foo))
  (fn-do + (infix this += (param child Foo)) _
    (= contents child)
  )
)
```

```output
B
BB
BBB
```

```brix
(main
  (lazy binder (call Binder (call Foo "bob")))
  (<> Foo (id: foo) (value: "root")
    (. binder #thing)
  )
  (blank)
  (print (?. (. foo content) name))
  (= (. binder thing) (call Foo "tom"))
  (print (?. (. foo content) name))
  (= (. binder thing) (call Foo "fred"))
  (print (?. (. foo content) name))
)

(type + Foo
  (prop + name String)
  (prop + content (? Foo))
  (fn-do + This _
    (param name _)
  )
  (fn-do + replace _
    (param child Foo)
    (param replacement Foo)
    (= content replacement)
  )
  (fn-do + (infix this += (param child Foo)) _
    (= content child)
  )
)

(type + Binder
  (prop + thing Foo)
  (fn-do + This _
    (param thing _)
  )
)
```

```output
bob
tom
fred
```

```brix
(main
  (lazy some-name "bob")
  (blank)
  (<> Foo (id: a)
    (= name #some-name)
  )
  (blank)
  (print (. a name))
  (= some-name "tom")
  (print (. a name))
  (= some-name "fred")
  (print (. a name))
)

(type + Foo
  (prop + name String)
)
```

```output
bob
tom
fred
```

```brix
(main
  (lazy some-name (. a #name))
  (blank)
  (<> Foo (id: a)
    (= name "bob")
    (<> Foo
      (= name #some-name)
    )
  )
  (blank)
  (print some-name)
  (print (?. (. a contents) name))
  (= (. a name) "tom")
  (print some-name)
  (print (?. (. a contents) name))
)

(type + Foo
  (prop + name String)
  (prop + contents (? Foo))
  (fn-do + (infix this += (param child Foo)) _
    (= contents child)
  )
)
```

```output
bob
bob
tom
tom
```

```brix
(main
  (lazy some-name (. a #name))
  (blank)
  (<> Foo (id: a)
    (= name "bob")
  )
  (let b
    (<> Foo
      (= name #some-name)
    )
  )
  (blank)
  (print some-name)
  (print (. b name))
  (= (. a name) "tom")
  (print some-name)
  (print (. b name))
)

(type + Foo
  (prop + name String)
  (prop + contents (? Foo))
  (fn-do + (infix this += (param child Foo)) _
    (= contents child)
  )
)
```

```output
bob
bob
tom
tom
```

```brix
(main
  (<> Thing (id: thing))
  (blank)
  (print (. thing name))
  (= (. thing prefix) " sr.")
  (print (. thing name))
)

(type + Thing
  (prop + prefix _ (= ""))
  (content
    (prop + name _ (= (+ "bob" #prefix)))
  )
)
```

```output
bob
bob sr.
```

```brix
(main
  (lazy some-name "bob")
  (blank)
  (<> Foo (id: foo) (value: #some-name))
  (blank)
  (print (. foo name))
  (= some-name "tom")
  (print (. foo name))
  (= some-name "fred")
  (print (. foo name))
)

(type + Foo
  (prop + name String)
  (fn-do + This _
    (param name _)
  )
)
```

```output
bob
tom
fred
```

```brix crash
(main
  (lazy some-name "bob")
  (blank)
  (<> Foo (id: foo) (value: #some-name))
)

(type + Foo
  (fn-do + This _
    (param value String)
  )
)
```

```output
Exception: Tracked markup constructor argument must match a property
<no file:5>
      (<> Foo (id: foo) (value: #some-name))
                                ^^^^^^^^^^
```

```brix
(main
  (<> Thing (id: thing))
  (blank)
  (lazy name (. thing #name))
  (blank)
  (print name)
  (= (. thing prefix) " sr.")
  (print name)
)

(type + Thing
  (prop + prefix _ (= ""))
  (content
    (prop + name _ (= (+ "bob" #prefix)))
  )
)
```

```output
bob
bob sr.
```

### A tracked read of a list follows changes made to the list in place.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let demo (call Demo))
  (print (call (. demo label)))
  (+= (. demo items) "a")
  (print (call (. demo label)))
  (+= (. demo items) "b")
  (print (call (. demo label)))
  (= ([] (. demo items) 0) "z")
  (print (call (. demo label)))
  (call (. demo items remove) 0)
  (print (call (. demo label)))
  (let old (. demo items))
  (= (. demo items) (call (of List String)))
  (print (call (. demo label)))
  (+= old "stale")
  (print (call (. demo label)))
  (+= (. demo items) "c")
  (print (call (. demo label)))
)

(type + Demo (is Box)
  (prop + items (list String) (= (call (of List String))))
  (content
    (<> This
      (<> TextBox (value: (str (. #items length) " " (call (. "," join) #items))))
      (if (> (. #items length) 1)
        (<> TextBox (value: "many"))
      )
    )
  )
  (fn-do + label String
    (let texts (list String))
    (call collect this texts)
    (return (str (. texts length) ":" (call (. "|" join) texts)))
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

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let model (call Model))
  (let demo (call Demo model))
  (print (call (. demo label)))
  (+= (. model items) "a")
  (print (call (. demo label)))
  (+= (. model items) "b")
  (print (call (. demo label)))
)

(type + Model
  (prop + items (list String) (= (call (of List String))))
)

(type + Demo (is Box)
  (prop + model Model)
  (fn-do + This _
    (param model _)
  )
  (content
    (<> This
      (<> TextBox (value: (call count-label (. model #items))))
    )
  )
  (fn-do + label String
    (let texts (list String))
    (call collect this texts)
    (return (str (. texts length) ":" (call (. "|" join) texts)))
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

(fn-expr + count-label _
  (param items (list String))
  (str (. items length) " items")
)
```

```output
1:0 items
1:1 items
1:2 items
```

### A markup loop over a tracked list still adds rows without rebuilding the others.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let demo (call Demo))
  (+= (. demo items) "a")
  (+= (. demo items) "b")
  (+= (. demo items) "c")
  (print (call (. demo label)))
  (print built)
)

(blank)
(prop + built _ (= 0))

(type + Demo (is Box)
  (prop + items (list String) (= (call (of List String))))
  (content
    (<> This
      (for (in item #items)
        (<> TextBox (value: (call stamp item)))
      )
    )
  )
  (fn-do + label String
    (let texts (list String))
    (call collect this texts)
    (return (str (. texts length) ":" (call (. "|" join) texts)))
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

(fn-do + stamp String
  (param text String)
  (+= built 1)
  (return text)
)
```

```output
3:a|b|c
3
```

### A tracked read of an optional list follows the list once it has one.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let demo (call Demo))
  (print (call (. demo label)))
  (= (. demo items) (call (of List String)))
  (print (call (. demo label)))
  (+= (!! (. demo items)) "a")
  (print (call (. demo label)))
  (= (. demo items) null)
  (print (call (. demo label)))
)

(type + Demo (is Box)
  (prop + items (? (list String)))
  (content
    (<> This
      (<> TextBox (value: (str (?? (?. #items length) 9))))
    )
  )
  (fn-do + label String
    (let texts (list String))
    (call collect this texts)
    (return (str (. texts length) ":" (call (. "|" join) texts)))
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
1:9
1:0
1:1
1:9
```

```brix
(main
  (lazy name "bob")
  (<> Foo (id: first)
    (= title #name)
  )
  (<> Foo (id: second)
    (= title #name)
  )
  (blank)
  (print (str (. first title) "/" (. second title)))
  (call (. first close))
  (= name "tom")
  (print (str (. first title) "/" (. second title)))
  (call (. second close))
  (= name "fred")
  (print (str (. first title) "/" (. second title)))
)

(type + Foo
  (prop + title String)
  (fn-do + close _)
)
```

```output
bob/bob
bob/tom
bob/tom
```

```brix
(main
  (lazy name "bob")
  (<> Foo (id: outgoing)
    (= title #name)
  )
  (blank)
  (lazy binder (call Binder outgoing))
  (<> Foo (id: holder)
    (. binder #thing)
  )
  (blank)
  (print (str (. outgoing title) "/" (?. (. holder content) title)))
  (= name "tom")
  (print (str (. outgoing title) "/" (?. (. holder content) title)))
  (= (. binder thing) (call Foo "fred"))
  (= name "sam")
  (print (str (. outgoing title) "/" (?. (. holder content) title)))
)

(type + Foo
  (prop + title _ (= ""))
  (prop + content (? Foo))
  (fn-do + This _
    (param title _ (= ""))
  )
  (fn-do + replace _
    (param child Foo)
    (param replacement Foo)
    (= content replacement)
  )
  (fn-do + (infix this += (param child Foo)) _
    (= content child)
  )
)

(type + Binder
  (prop + thing Foo)
  (fn-do + This _
    (param thing _)
  )
)
```

```output
bob/bob
tom/tom
tom/fred
```

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (lazy name "bob")
  (<> Titled (id: outgoing)
    (= title #name)
    (<> Titled (id: nested)
      (= title #name)
    )
  )
  (blank)
  (lazy binder (call Binder outgoing))
  (<> Box (id: holder)
    (. binder #thing)
  )
  (blank)
  (print (str (. outgoing title) "/" (. nested title) "/" (call shown holder)))
  (= name "tom")
  (print (str (. outgoing title) "/" (. nested title) "/" (call shown holder)))
  (= (. binder thing) (call Titled "fred"))
  (= name "sam")
  (print (str (. outgoing title) "/" (. nested title) "/" (call shown holder)))
)

(type + Titled (is Box)
  (prop + title _ (= ""))
  (fn-do + This _
    (param title _ (= ""))
  )
)

(type + Binder
  (prop + thing Titled)
  (fn-do + This _
    (param thing _)
  )
)

(fn-do + shown String
  (param holder Box)
  (for (in child (. holder children))
    (return (. (as child Titled) title))
  )
  (return "none")
)
```

```output
bob/bob/bob
tom/tom/tom
tom/tom/fred
```

### An error an attribute's value raises as the attribute is given again goes to unhandled-error, and the element keeps the value it had.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (= unhandled-error
    (=> (error) :do
      (block
        (print (str "handled " (. error message)))
      )
    )
  )
  (blank)
  (let panel (call Panel))
  (print (. (as (!! (. panel %first-child)) Label) text))
  (= (. panel value) "abcd")
  (print (. (as (!! (. panel %first-child)) Label) text))
  (= (. panel value) "x")
  (print (. (as (!! (. panel %first-child)) Label) text))
)

(fn-do + parse Int
  (param text String)
  (throw (call SyntaxError "bad") (if (== text "x")))
  (return (. text length))
)

(type + Panel (is Box)
  (prop + value _ (= "ab"))
  (content
    (<> This
      (<> Label
        (= text (str (call parse #value)))
      )
    )
  )
)
```

```output
2
4
handled bad
4
```

### A line of a root `<This` that gives content and reads something tracked gives it again as what it read changes.

```brix
(main
  (<> Holder (id: holder)
    (= name "a")
  )
  (blank)
  (print (?. (. holder content) name))
  (= (. holder name) "b")
  (print (?. (. holder content) name))
)

(type + Holder
  (prop + name String)
  (prop + content (? Item))
  (content
    (<> This
      (call Item #name)
    )
  )
  (fn-do + replace _
    (param child Item)
    (param replacement Item)
    (= content replacement)
  )
  (fn-do + (infix this += (param child Item)) _
    (= content child)
  )
)

(type + Item
  (prop + name String)
  (fn-do + This _
    (param name _)
  )
)
```

```output
a
b
```

### A line of a root `<This` that is a tracked name alone gives what the name holds as content, and gives it again as that changes.

```brix
(main
  (<> Holder (id: holder)
    (= item (call Item "a"))
  )
  (blank)
  (print (?. (. holder content) name))
  (= (. holder item) (call Item "b"))
  (print (?. (. holder content) name))
)

(type + Holder
  (prop + item Item)
  (prop + content (? Item))
  (content
    (<> This
      #item
    )
  )
  (fn-do + replace _
    (param child Item)
    (param replacement Item)
    (= content replacement)
  )
  (fn-do + (infix this += (param child Item)) _
    (= content child)
  )
)

(type + Item
  (prop + name String)
  (fn-do + This _
    (param name _)
  )
)
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
