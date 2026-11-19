# Markup loops

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo)
    (for (in item (. demo #items))
      (<> TextBox (value: item))
    )
  )
  (print "made")
  (print (call dump demo))
  (+= (. demo items) "c")
  (print (call dump demo))
  (-= (. demo items) "b")
  (print (call dump demo))
  (= ([] (. demo items) 1) "z")
  (print (call dump demo))
)

(fn-expr + dump String
  (param demo Demo)
  (str count ":" first-text ":" last-text)
  (where
    (lazy region (!! (. demo %first-child)))
    (lazy count (call count-children region))
    (lazy first-text (. (as (!! (. region %first-child)) TextBox) text))
    (lazy last-text (. (as (!! (. region %last-child)) TextBox) text))
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
  (prop + items (list String))
  (fn-do + This _
    (= items (call (of List String) (list "a" "b")))
  )
)
```

```output
made
2:a:b
3:a:c
2:a:c
2:a:z
```

```brix crash
(main
  (let items (call (of List String) (list "a")))
  (blank)
  (<> Plain (id: root)
    (for (in item #items)
      (<> Plain)
    )
  )
)

(type + Plain)
```

```output
Exception: Dynamic markup loop parent must implement MarkupElement
<no file:6>
        (for (in item #items)
        ^^^^^^^^^^^^^^^^^^^^
```

```brix crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let list (call (of List String) (list "a")))
  (let items (of Collection String) list)
  (blank)
  (<> Box (id: box)
    (for (in item #items)
      (<> TextBox (value: item))
    )
  )
)
```

```output
Exception: Markup loop requires a concrete Collection source
<no file:10>
        (for (in item #items)
        ^^^^^^^^^^^^^^^^^^^^
```

### A markup loop row with an `if` is a region of its own, and the arm it takes is its content.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let items (call (of List String) (list "a" "")))
  (blank)
  (<> Box (id: box)
    (for (in item #items)
      (if (. item length)
        (<> TextBox (value: item))
        (else
          (<> TextBox (value: "empty"))
        )
      )
    )
  )
  (print (call texts box))
  (+= items "b")
  (print (call texts box))
)

(fn-do + texts String
  (param box Box)
  (let parts (call (of List String)))
  (call gather box parts)
  (return (call (. " " join) parts))
)

(fn-do + gather _
  (param box Box)
  (param parts (list String))
  (for (in child (. box children))
    (if (is child TextBox)
      (+= parts (. (as child TextBox) text))
      (else
        (call gather child parts)
      )
    )
  )
)
```

```output
a empty
a empty b
```

### A markup loop row with two sibling elements is a region of its own that holds both.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let items (call (of List String) (list "a")))
  (blank)
  (<> Box (id: box)
    (for (in item #items (keyed-by item))
      (<> TextBox (value: item))
      (<> TextBox (value: "/"))
    )
  )
  (print (call texts box))
  (+= items "b")
  (print (call texts box))
  (-= items "a")
  (print (call texts box))
)

(fn-do + texts String
  (param box Box)
  (let parts (call (of List String)))
  (call gather box parts)
  (return (call (. " " join) parts))
)

(fn-do + gather _
  (param box Box)
  (param parts (list String))
  (for (in child (. box children))
    (if (is child TextBox)
      (+= parts (. (as child TextBox) text))
      (else
        (call gather child parts)
      )
    )
  )
)
```

```output
a /
a / b /
b /
```

```brix crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let items (call (of List String) (list "a")))
  (blank)
  (<> Box (id: box)
    (for (in item #items)
      item
    )
  )
)
```

```output
Exception: Expected Box here, but this is String, and there is no conversion between them
<no file:9>
        (for (in item #items)
        ^^^^^^^^^^^^^^^^^^^^
```

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo)
    (for (in item (. demo #items))
      (<> TextBox (value: item))
    )
  )
  (print (call dump demo))
  (++= (. demo items) (call (of List String) (list "c" "d")))
  (print (call dump demo))
)

(fn-expr + dump String
  (param demo Demo)
  (str count ":" first-text ":" last-text)
  (where
    (lazy region (!! (. demo %first-child)))
    (lazy count (call count-children region))
    (lazy first-text (. (as (!! (. region %first-child)) TextBox) text))
    (lazy last-text (. (as (!! (. region %last-child)) TextBox) text))
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
  (prop + items (list String))
  (fn-do + This _
    (= items (call (of List String) (list "a" "b")))
  )
)
```

```output
2:a:b
4:a:d
```

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo)
    (for (in item (. demo #items))
      (<> TextBox (value: item))
    )
  )
  (print (call dump demo))
  (call (. demo items slide) -1)
  (print (call dump demo))
)

(fn-expr + dump String
  (param demo Demo)
  (str count ":" first-text ":" last-text)
  (where
    (lazy region (!! (. demo %first-child)))
    (lazy count (call count-children region))
    (lazy first-text (. (as (!! (. region %first-child)) TextBox) text))
    (lazy last-text (. (as (!! (. region %last-child)) TextBox) text))
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
  (prop + items (list String))
  (fn-do + This _
    (= items (call (of List String) (list "a" "b" "c")))
  )
)
```

```output
3:a:c
2:b:c
```

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo)
    (for (in item (. demo #items))
      (<> TextBox (value: item))
    )
  )
  (print (call dump demo))
  (call (. demo items insert) "x" 1)
  (print (call dump demo))
)

(fn-expr + dump String
  (param demo Demo)
  (str count ":" first-text ":" last-text)
  (where
    (lazy region (!! (. demo %first-child)))
    (lazy count (call count-children region))
    (lazy first-text (. (as (!! (. region %first-child)) TextBox) text))
    (lazy last-text (. (as (!! (. region %last-child)) TextBox) text))
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
  (prop + items (list String))
  (fn-do + This _
    (= items (call (of List String) (list "a" "b")))
  )
)
```

```output
2:a:b
3:a:b
```

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let items (call (of List String) (list "a" "b")))
  (blank)
  (<> Box (id: box)
    (for (in item #items)
      (<> TextBox (value: item))
    )
  )
  (blank)
  (let old-items items)
  (print (call dump box))
  (+= items "c")
  (print (call dump box))
  (= items (call (of List String) (list "x" "y")))
  (print (call dump box))
  (+= old-items "stale")
  (print (call dump box))
  (+= items "z")
  (print (call dump box))
)

(fn-expr + dump String
  (param box Box)
  (str count ":" first-text ":" last-text)
  (where
    (lazy region (!! (. box %first-child)))
    (lazy count (call count-children region))
    (lazy first-text (. (as (!! (. region %first-child)) TextBox) text))
    (lazy last-text (. (as (!! (. region %last-child)) TextBox) text))
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
```

```output
2:a:b
3:a:c
2:x:y
2:x:y
3:x:z
```

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo)
    (for (in item (. demo #items))
      (<> TextBox (value: item))
    )
  )
  (let old-items (. demo items))
  (print (call dump demo))
  (= (. demo items) (call (of List String) (list "x" "y")))
  (print (call dump demo))
  (+= old-items "stale")
  (print (call dump demo))
  (+= (. demo items) "z")
  (print (call dump demo))
)

(fn-expr + dump String
  (param demo Demo)
  (str count ":" first-text ":" last-text)
  (where
    (lazy region (!! (. demo %first-child)))
    (lazy count (call count-children region))
    (lazy first-text (. (as (!! (. region %first-child)) TextBox) text))
    (lazy last-text (. (as (!! (. region %last-child)) TextBox) text))
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
  (prop + items (list String))
  (fn-do + This _
    (= items (call (of List String) (list "a" "b")))
  )
)
```

```output
2:a:b
2:x:y
2:x:y
3:x:z
```

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo)
    (for (in item (. demo #items) (keyed-by (. item id)))
      (<> TextBox (value: (. item text)))
    )
  )
  (let region (!! (. demo %first-child)))
  (let a-row (!! (. region %first-child)))
  (print (call dump demo))
  (call (. demo items insert) (call RowData "x" "X") 0)
  (print (call dump demo))
  (print (if-expr (== a-row (!! (call (. region child-at) 1))) "preserved" "lost"))
  (call (. demo items remove) 0)
  (print (call dump demo))
  (print (if-expr (== a-row (!! (. region %first-child))) "preserved" "lost"))
  (++= (. demo items) (call (of List RowData) (list (call RowData "c" "C") (call RowData "d" "D"))))
  (let d-row (call (. region child-at) 3))
  (print (call dump demo))
  (print (if-expr (== a-row (!! (. region %first-child))) "append-preserved" "append-lost"))
  (let b-row (call (. region child-at) 1))
  (call (. demo items move) 1 0)
  (print (call dump demo))
  (print (if-expr (== b-row (!! (. region %first-child))) "preserved" "lost"))
  (print (if-expr (== d-row (!! (call (. region child-at) 3))) "bulk-key-preserved" "bulk-key-lost"))
)

(fn-expr + dump String
  (param demo Demo)
  (str count ":" first-text ":" last-text)
  (where
    (lazy region (!! (. demo %first-child)))
    (lazy count (call count-children region))
    (lazy first-text (. (as (!! (. region %first-child)) TextBox) text))
    (lazy last-text (. (as (!! (. region %last-child)) TextBox) text))
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

(type + RowData
  (prop + id String)
  (prop + text String)
  (fn-do + This _
    (param id String)
    (param text String)
  )
)

(type + Demo (is Box)
  (prop + items (list RowData))
  (fn-do + This _
    (= items (call (of List RowData) (list (call RowData "a" "A") (call RowData "b" "B"))))
  )
)
```

```output
2:A:B
3:X:B
preserved
2:A:B
preserved
4:A:D
append-preserved
4:B:D
preserved
bulk-key-preserved
```

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo)
    (for (in item (. demo #items) (keyed-by (. item id)))
      (<> TextBox (value: (. item text)))
    )
  )
  (blank)
  (let region (!! (. demo %first-child)))
  (let a-row (!! (. region %first-child)))
  (let c-row (call (. region child-at) 2))
  (print (call dump demo))
  (= (. demo items) (call (of List RowData) (list (. demo c-item) (. demo a-item))))
  (print (call dump demo))
  (print (if-expr (== c-row (!! (. region %first-child))) "c-preserved" "c-lost"))
  (print (if-expr (== a-row (!! (call (. region child-at) 1))) "a-preserved" "a-lost"))
  (print (if-expr (not (call (. region child-at) 2)) "tail-ok" "tail-extra"))
)

(fn-expr + dump String
  (param demo Demo)
  (str count ":" first-text ":" last-text)
  (where
    (lazy region (!! (. demo %first-child)))
    (lazy count (call count-children region))
    (lazy first-text (. (as (!! (. region %first-child)) TextBox) text))
    (lazy last-text (. (as (!! (. region %last-child)) TextBox) text))
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

(type + RowData
  (prop + id String)
  (prop + text String)
  (fn-do + This _
    (param id String)
    (param text String)
  )
)

(type + Demo (is Box)
  (prop + a-item RowData)
  (prop + b-item RowData)
  (prop + c-item RowData)
  (prop + items (list RowData))
  (fn-do + This _
    (= a-item (call RowData "a" "A"))
    (= b-item (call RowData "b" "B"))
    (= c-item (call RowData "c" "C"))
    (= items (call (of List RowData) (list a-item b-item c-item)))
  )
)
```

```output
3:A:C
2:C:A
c-preserved
a-preserved
tail-ok
```

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let items (call make-rows 1030))
  (blank)
  (<> Demo (id: demo)
    (for (in item #items (keyed-by (. item id)))
      (<> TextBox (value: (. item text)))
    )
  )
  (blank)
  (let region (!! (. demo %first-child)))
  (let first-row (!! (. region %first-child)))
  (let last-row (call (. region child-at) 1029))
  (print (call dump demo))
  (= items (call (of List RowData) (list (call RowData "1029" "row 1029") (call RowData "0" "row 0"))))
  (print (call dump demo))
  (print (if-expr (== last-row (!! (. region %first-child))) "last-preserved" "last-lost"))
  (print (if-expr (== first-row (!! (call (. region child-at) 1))) "first-preserved" "first-lost"))
  (print (if-expr (not (call (. region child-at) 2)) "tail-ok" "tail-extra"))
)

(fn-expr + dump String
  (param demo Demo)
  (str count ":" first-text ":" last-text)
  (where
    (lazy region (!! (. demo %first-child)))
    (lazy count (call count-children region))
    (lazy first-text (. (as (!! (. region %first-child)) TextBox) text))
    (lazy last-text (. (as (!! (. region %last-child)) TextBox) text))
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

(fn-do + make-rows (list RowData)
  (param count Size)
  (let rows (list RowData))
  (for (in index (..< 0 count))
    (+= rows (call RowData (str index) (str "row " index)))
  )
  (return rows)
)

(type + RowData
  (prop + id String)
  (prop + text String)
  (fn-do + This _
    (param id String)
    (param text String)
  )
)

(type + Demo (is Box))
```

```output
1030:row 0:row 1029
2:row 1029:row 0
last-preserved
first-preserved
tail-ok
```

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo)
    (for (in item (. demo #items))
      (<> TextBox (value: item))
    )
  )
  (blank)
  (let region (!! (. demo %first-child)))
  (let a-row (!! (. region %first-child)))
  (let b-row (call (. region child-at) 1))
  (let c-row (call (. region child-at) 2))
  (call (. demo items insert) "x" 1)
  (print (if-expr (== a-row (!! (. region %first-child))) "prefix-preserved" "prefix-lost"))
  (print (if-expr (== b-row (!! (call (. region child-at) 2))) "insert-suffix-preserved" "insert-suffix-remounted"))
  (print (if-expr (== c-row (!! (call (. region child-at) 3))) "insert-tail-preserved" "insert-tail-remounted"))
  (blank)
  (let after-insert (call (. region child-at) 2))
  (call (. demo items remove) 1)
  (print (if-expr (== after-insert (!! (call (. region child-at) 1))) "remove-suffix-preserved" "remove-suffix-remounted"))
)

(type + Demo (is Box)
  (prop + items (list String))
  (fn-do + This _
    (= items (call (of List String) (list "a" "b" "c")))
  )
)
```

```output
prefix-preserved
insert-suffix-remounted
insert-tail-remounted
remove-suffix-remounted
```

```brix crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo)
    (for (in item #items)
      (and-for (in label labels)
        (<> TextBox (value: item))
      )
    )
  )
)

(type + Demo (is Box)
  (prop + items (list String))
  (prop + labels (list String))
  (fn-do + This _
    (= items (call (of List String) (list "a")))
    (= labels (call (of List String) (list "x")))
  )
)
```

```output
Exception: Combined markup loops not supported
<no file:7>
        (for (in item #items)
        ^^^^^^^^^^^^^^^^^^^^
```

```brix crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let list (call (of List String) (list "a" "b")))
  (let items (of Indexable String) list)
  (blank)
  (<> Box (id: box)
    (for (in item #items)
      (<> TextBox (value: item))
    )
  )
)
```

```output
Exception: Markup loop cannot observe Indexable sources because Indexable has no mutation events
<no file:10>
        (for (in item #items)
        ^^^^^^^^^^^^^^^^^^^^
```

```brix
(import /core/channel)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo)
    (for (in item (. demo #events))
      (<> TextBox (value: item))
    )
  )
  (blank)
  (print (call dump demo))
  (<- (. demo events) "a")
  (call yield-once)
  (print (call dump demo))
  (let old-events (. demo events))
  (= (. demo events) (call (of AsyncChannel String)))
  (print (call dump demo))
  (<- old-events "stale")
  (call yield-once)
  (print (call dump demo))
  (<- (. demo events) "b")
  (call yield-once)
  (print (call dump demo))
  (call (. demo close))
  (call yield-once)
  (<- (. demo events) "closed")
  (call yield-once)
  (print (call dump demo))
)

(fn-do + yield-once _
  (let done (call Semaphore))
  (do-new
    (call (. done post))
  )
  (call (. done wait))
)

(fn-do + dump String
  (param demo Demo)
  (if (== count 0)
    (return "0")
  )
  (return (str count ":" first-text ":" last-text))
  (blank)
  (lazy region (!! (. demo %first-child)))
  (lazy count (call count-children region))
  (lazy first-text (. (as (!! (. region %first-child)) TextBox) text))
  (lazy last-text (. (as (!! (. region %last-child)) TextBox) text))
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
  (prop + events (of AsyncChannel String))
)
```

```output
0
1:a:a
0
0
1:b:b
1:b:b
```

### Key-builder failures above the keyed crossover remain synchronous and deterministic.

```brix crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> KeyFailureDemo (id: demo)
    (for (in item (. demo #items) (keyed-by (call failure-key item)))
      (<> TextBox (value: (. item text)))
    )
  )
  (blank)
  (let replacement (call make-key-failure-items 33))
  (= ([] replacement 1) (call KeyFailureItem "boom" "boom"))
  (= (. demo items) replacement)
)

(type + KeyFailureItem (is Tuple)
  (prop + id String)
  (prop + text String)
)

(type + KeyFailureDemo (is Box)
  (prop + items (list KeyFailureItem) (= (call make-key-failure-items 33)))
)

(fn-do + make-key-failure-items (list KeyFailureItem)
  (param count Size)
  (let items (list KeyFailureItem))
  (for (in index (..< 0 count))
    (+= items (call KeyFailureItem (str "id-" index) (str "row-" index)))
  )
  (return items)
)

(fn-do + failure-key String
  (param item KeyFailureItem)
  (if (== (. item id) "boom")
    (throw (call Panic "key failure"))
  )
  (return (. item id))
)
```

```brix
(import /core/collection)
(blank)

(main
  (let items (call (of List String) (list "a" "b")))
  (blank)
  (<> MarkNode (id: root)
    (for (in item #items)
      (<> MarkText (value: item))
    )
  )
  (blank)
  (print (call dump root))
  (+= items "c")
  (print (call dump root))
  (let old-items items)
  (= items (call (of List String) (list "x")))
  (print (call dump root))
  (+= old-items "stale")
  (print (call dump root))
  (+= items "y")
  (print (call dump root))
)

(fn-expr + dump String
  (param node MarkNode)
  (str count ":" first-text ":" last-text)
  (where
    (lazy region (!! (. node first-child)))
    (lazy count (call count-children region))
    (lazy first-text (. (as (!! (. region first-child)) MarkText) text))
    (lazy last-text (. (as (!! (. region last-child)) MarkText) text))
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
```

```output
2:a:b
3:a:c
1:x:x
1:x:x
2:x:y
```

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo)
    (for (in item (. demo #items) (keyed-by (. item id)))
      (<> TextBox (value: (. item text)))
    )
  )
  (blank)
  (let region (!! (. demo %first-child)))
  (= (. demo items) (call (of List RowData) (list (call RowData "x" "X1") (call RowData "x" "X2"))))
  (print (if-expr (!= (!! (. region %first-child)) (!! (call (. region child-at) 1))) "distinct" "same"))
  (print (if-expr (not (call (. region child-at) 2)) "tail-ok" "tail-extra"))
)

(type + RowData
  (prop + id String)
  (prop + text String)
  (fn-do + This _
    (param id String)
    (param text String)
  )
)

(type + Demo (is Box)
  (prop + items (list RowData))
  (fn-do + This _
    (= items (call (of List RowData) (list (call RowData "x" "X0"))))
  )
)
```

```output
distinct
tail-ok
```

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo)
    (for (in item (. demo #items) (keyed-by (. item id)))
      (<> TextBox (value: (. item text)))
    )
  )
  (blank)
  (let region (!! (. demo %first-child)))
  (let first-row (!! (. region %first-child)))
  (let second-row (call (. region child-at) 1))
  (= (. demo items) (call (of List RowData) (list (. demo first-item) (. demo second-item))))
  (print (if-expr (== first-row (!! (. region %first-child))) "first-preserved" "first-lost"))
  (print (if-expr (== second-row (!! (call (. region child-at) 1))) "second-preserved" "second-lost"))
  (= (. demo items) (call (of List RowData) (list (. demo second-item) (. demo first-item))))
  (print (if-expr (== first-row (!! (. region %first-child))) "duplicate-fifo" "duplicate-reordered"))
)

(type + RowData
  (prop + id String)
  (prop + text String)
  (fn-do + This _
    (param id String)
    (param text String)
  )
)

(type + Demo (is Box)
  (prop + first-item RowData)
  (prop + second-item RowData)
  (prop + items (list RowData))
  (fn-do + This _
    (= first-item (call RowData "x" "X1"))
    (= second-item (call RowData "x" "X2"))
    (= items (call (of List RowData) (list first-item second-item)))
  )
)
```

```output
first-preserved
second-preserved
duplicate-fifo
```

```brix
(import /core/channel)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo)
    (for (in item (. demo #events))
      (<> TextBox (value: item))
    )
  )
  (blank)
  (<- (. demo events) "a")
  (call yield-once)
  (print (call dump demo))
  (let old-events (. demo events))
  (= (. demo events) (call (of SyncBufferedChannel String)))
  (print (call dump demo))
  (<- old-events "stale")
  (call (. old-events close))
  (call yield-once)
  (print (call dump demo))
  (<- (. demo events) "b")
  (call yield-once)
  (print (call dump demo))
  (call (. demo events close))
  (call yield-once)
  (call (. demo close))
  (<- (. demo events) "closed")
  (call yield-once)
  (print (call dump demo))
)

(fn-do + yield-once _
  (let done (call Semaphore))
  (do-new
    (call (. done post))
  )
  (call (. done wait))
)

(fn-do + dump String
  (param demo Demo)
  (if (== count 0)
    (return "0")
  )
  (return (str count ":" first-text ":" last-text))
  (blank)
  (lazy region (!! (. demo %first-child)))
  (lazy count (call count-children region))
  (lazy first-text (. (as (!! (. region %first-child)) TextBox) text))
  (lazy last-text (. (as (!! (. region %last-child)) TextBox) text))
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
  (prop + events (of SyncBufferedChannel String))
)
```

```output
1:a:a
0
0
1:b:b
1:b:b
```

### A tracked range is an iterable source: rows follow the range as its bound changes.

```brix
(import /core/task)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo)
    (for (in index (..< 0 (. demo #count)))
      (<> TextBox (value: (str "row " index)))
    )
  )
  (call yield-once)
  (print (call dump demo))
  (= (. demo count) 3)
  (call yield-once)
  (print (call dump demo))
)

(fn-do + yield-once _
  (let done (call Semaphore))
  (do-new
    (call (. done post))
  )
  (call (. done wait))
)

(fn-do + dump String
  (param demo Demo)
  (if (== count 0)
    (return "0")
  )
  (return (str count ":" first-text ":" last-text))
  (blank)
  (lazy region (!! (. demo %first-child)))
  (lazy count (call count-children region))
  (lazy first-text (. (as (!! (. region %first-child)) TextBox) text))
  (lazy last-text (. (as (!! (. region %last-child)) TextBox) text))
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
  (prop + count _ (= 2))
)
```

```output
2:row 0:row 1
3:row 0:row 2
```

### A shrinking keyed loop removes its rows before their bindings read the new source.

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let panel (call LoopPanel))
  (call markup-mount-root panel panel)
  (call (. panel markup-batch-queue checkpoint))
  (print (call dump panel))
  (= (. panel items) (call (of List String) (list "x")))
  (call (. panel markup-batch-queue checkpoint))
  (print (call dump panel))
  (= (. panel items) (call (of List String) (list "p" "q" "r")))
  (call (. panel markup-batch-queue checkpoint))
  (print (call dump panel))
)

(fn-do + indexes (list Index)
  (param count Size)
  (let result (call (of List Index)))
  (for (in i (..< 0 count))
    (+= result i)
  )
  (return result)
)

(fn-do + dump String
  (param panel LoopPanel)
  (let texts (call (of List String)))
  (for (in child (. (!! (. panel %first-child)) children))
    (+= texts (. (as child TextBox) text))
  )
  (return (call (. "," join) texts))
)

(type + LoopPanel (is Box)
  (implements MarkupRootElement)
  (prop + items (list String) (= (call (of List String) (list "a" "b"))))
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue (as this MarkupRootElement))))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
  (content
    (<> This
      (for (in index (call indexes (. #items length)) (keyed-by index))
        (<> TextBox (value: ([] #items index)))
      )
    )
  )
)
```

```output
a,b
x
p,q,r
```

### A keyed row re-derives body locals that read the row item when the collection replaces an element with a new object under the same key, while preserving the physical row.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo)
    (for (in item (. demo #items) (keyed-by (. item id)))
      (let label (str (. item text) "!"))
      (let fixed "row")
      (<> TextBox (value: (str label " " fixed)))
    )
  )
  (blank)
  (let region (!! (. demo %first-child)))
  (let first (!! (. region %first-child)))
  (print (call row-texts demo))
  (= ([] (. demo items) 0) (call RowData "a" "Z"))
  (print (call row-texts demo))
  (print (if-expr (== first (!! (. region %first-child))) "preserved" "lost"))
)

(fn-do + row-texts String
  (param demo Demo)
  (let parts (list String) (call (of List String)))
  (for (in child (. (!! (. demo %first-child)) children))
    (+= parts (. (as child TextBox) text))
  )
  (return (call (. "," join) parts))
)

(type + RowData
  (prop + id String)
  (prop + text String)
  (fn-do + This _
    (param id String)
    (param text String)
  )
)

(type + Demo (is Box)
  (prop + items (list RowData))
  (fn-do + This _
    (= items (call (of List RowData) (list (call RowData "a" "A") (call RowData "b" "B"))))
  )
)
```

```output
A! row,B! row
Z! row,B! row
preserved
```

### A lazy local in a markup loop body compiles, and in a keyed row it follows the replaced item.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo)
    (for (in item (. demo #items) (keyed-by (. item id)))
      (lazy label (str (. item text) "?"))
      (<> TextBox (value: label))
    )
  )
  (blank)
  (<> Demo (id: plain)
    (for (in item (. plain #items))
      (lazy label (str (. item text) "."))
      (<> TextBox (value: label))
    )
  )
  (blank)
  (print (call row-texts demo))
  (print (call row-texts plain))
  (= ([] (. demo items) 0) (call RowData "a" "Z"))
  (print (call row-texts demo))
)

(fn-do + row-texts String
  (param demo Demo)
  (let parts (list String) (call (of List String)))
  (for (in child (. (!! (. demo %first-child)) children))
    (+= parts (. (as child TextBox) text))
  )
  (return (call (. "," join) parts))
)

(type + RowData
  (prop + id String)
  (prop + text String)
  (fn-do + This _
    (param id String)
    (param text String)
  )
)

(type + Demo (is Box)
  (prop + items (list RowData))
  (fn-do + This _
    (= items (call (of List RowData) (list (call RowData "a" "A") (call RowData "b" "B"))))
  )
)
```

```output
A?,B?
A.,B.
Z?,B?
```

### A row-derived local read where a nested loop shadows the name it depends on is rejected rather than silently resolving to the inner binding.

```brix warn crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo)
    (for (in item (. demo #items) (keyed-by (. item id)))
      (let label (. item text))
      (<> Box
        (for (in item (. demo #extras))
          (<> TextBox (value: label))
        )
      )
    )
  )
)

(type + RowData
  (prop + id String)
  (prop + text String)
  (fn-do + This _
    (param id String)
    (param text String)
  )
)

(type + Demo (is Box)
  (prop + items (list RowData))
  (prop + extras (list String))
  (fn-do + This _
    (= items (call (of List RowData) (list (call RowData "a" "A"))))
    (= extras (call (of List String) (list "x")))
  )
)
```

```output
Exception: Markup row local label reads item, which is shadowed here; rename one of them
<no file:11>
              (<> TextBox (value: label))
                                  ^^^^^
```

### An id on the root element of a static markup loop row is a local of that row, so the row's own content reads it and each row sees its own element. Ids nested in the row are row locals too.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let titles (call (of List String) (list "a" "b")))
  (blank)
  (<> Box (id: root)
    (for (in title titles)
      (<> Holder (id: columns)
        (= title title)
        (blank)
        (<> Pane (id: pane)
          (= label (. columns title))
        )
        (<> Pane
          (= label (str (. pane label) "+" (. columns title)))
        )
      )
    )
  )
  (blank)
  (<> Box (id: listed)
    (iterate (in titles)
      (block
        (<> Holder (id: columns)
          (= title each)
          (blank)
          (<> Pane
            (= label (. columns title))
          )
        )
      )
    )
  )
  (blank)
  (print (call row-labels root))
  (print (call row-labels listed))
)

(type + Holder (is Box)
  (prop + title _ (= ""))
)

(type + Pane (is Box)
  (prop + label _ (= ""))
)

(fn-do + row-labels String
  (param rows Box)
  (let parts (list String) (call (of List String)))
  (for (in row (. rows children))
    (for (in pane (. (as row Box) children))
      (+= parts (. (as pane Pane) label))
    )
  )
  (return (call (. "," join) parts))
)
```

```output
a,a+a,b,b+b
a,b
```

### Supplied slot content in a static markup loop row reads the row's slot host by the host's markup id. Each row binds its own host, so a tracked read follows only that row's host, and a closure in the content reads its row's host when it runs.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let titles (call (of List String) (list "a" "b")))
  (blank)
  (<> Box (id: root)
    (for (in title titles)
      (<> Holder (id: columns)
        (= title title)
        (blank)
        (<> Pane (slot: detail)
          (= tracked (. columns #title))
          (= untracked (. columns title))
          (= action (=> () :do (= (. columns title) "hit")))
        )
      )
    )
  )
  (blank)
  (print (call pane-labels root))
  (= (. (as (!! (. root %first-child)) Holder) title) "z")
  (print (call pane-labels root))
  (let action (. (as (!! (. (!! (. root %last-child)) %first-child)) Pane) action))
  (if action
    (call action)
  )
  (print (call pane-labels root))
)

(type + Holder (is Box)
  (prop + title _ (= ""))
  (content
    (<> This
      (+<> Box (id: detail))
    )
  )
)

(type + Pane (is Box)
  (prop + tracked _ (= ""))
  (prop + untracked _ (= ""))
  (prop + action (? Doer))
)

(fn-do + pane-labels String
  (param rows Box)
  (let parts (list String) (call (of List String)))
  (for (in row (. rows children))
    (let pane (as (!! (. row %first-child)) Pane))
    (+= parts (str (. pane tracked) "/" (. pane untracked)))
  )
  (return (call (. "," join) parts))
)
```

```output
a/a,b/b
z/a,b/b
z/a,hit/b
```

### In tracked markup loops, keyed or not, supplied slot content reads the row's slot host by the host's markup id. A tracked read follows that row's host, and rows added later bind their own.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let titles (call (of List String) (list "a" "b")))
  (blank)
  (<> Box (id: keyed)
    (for (in title #titles (keyed-by title))
      (<> Holder (id: columns)
        (= title title)
        (blank)
        (<> Pane (slot: detail)
          (= label (. columns #title))
        )
      )
    )
  )
  (blank)
  (<> Box (id: plain)
    (for (in title #titles)
      (<> Holder (id: columns)
        (= title title)
        (blank)
        (<> Pane (slot: detail)
          (= label (. columns #title))
        )
      )
    )
  )
  (blank)
  (let keyed-rows (!! (. keyed %first-child)))
  (let plain-rows (!! (. plain %first-child)))
  (print (str (call row-labels keyed-rows) " " (call row-labels plain-rows)))
  (= (. (as (!! (. keyed-rows %first-child)) Holder) title) "y")
  (= (. (as (!! (. plain-rows %first-child)) Holder) title) "z")
  (print (str (call row-labels keyed-rows) " " (call row-labels plain-rows)))
  (+= titles "c")
  (print (str (call row-labels keyed-rows) " " (call row-labels plain-rows)))
)

(type + Holder (is Box)
  (prop + title _ (= ""))
  (content
    (<> This
      (+<> Box (id: detail))
    )
  )
)

(type + Pane (is Box)
  (prop + label _ (= ""))
)

(fn-do + row-labels String
  (param rows Box)
  (let parts (list String) (call (of List String)))
  (for (in row (. rows children))
    (+= parts (. (as (!! (. row %first-child)) Pane) label))
  )
  (return (call (. "," join) parts))
)
```

```output
a,b a,b
y,b z,b
y,b,c z,b,c
```

### In class content, supplied slot content in a static markup loop row reads the row's slot host by the host's markup id, and each row binds its own host.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo))
  (print (call row-labels demo))
  (= (. (as (!! (. demo %first-child)) Holder) title) "z")
  (print (call row-labels demo))
)

(type + Demo (is Box)
  (prop + titles _ (= (call (of List String) (list "a" "b"))))
  (content
    (<> This
      (for (in title titles)
        (<> Holder (id: columns)
          (= title title)
          (blank)
          (<> Pane (slot: detail)
            (= label (. columns #title))
          )
        )
      )
    )
  )
)

(type + Holder (is Box)
  (prop + title _ (= ""))
  (content
    (<> This
      (+<> Box (id: detail))
    )
  )
)

(type + Pane (is Box)
  (prop + label _ (= ""))
)

(fn-do + row-labels String
  (param rows Box)
  (let parts (list String) (call (of List String)))
  (for (in row (. rows children))
    (+= parts (. (as (!! (. row %first-child)) Pane) label))
  )
  (return (call (. "," join) parts))
)
```

```output
a,b
z,b
```

### A markup loop row's id is not visible outside its row.

```brix compile-error
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let titles (call (of List String) (list "a" "b")))
  (blank)
  (<> Box (id: root)
    (for (in title titles)
      (<> Holder (id: columns)
        (= title title)
      )
    )
  )
  (blank)
  (print (. columns title))
)

(type + Holder (is Box)
  (prop + title _ (= ""))
)
```

```output
Exception: Name "columns" not found
<no file:16>
      (print (. columns title))
                ^^^^^^^
```

### A lazy local declared in a static markup loop body is a local of that body, so the row's content reads it.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let titles (call (of List String) (list "a" "b")))
  (blank)
  (<> Box (id: root)
    (for (in title titles)
      (lazy shown (str title "!"))
      (<> Holder
        (<> Pane
          (= label shown)
        )
      )
    )
  )
  (blank)
  (print (call row-labels root))
)

(type + Holder (is Box))

(type + Pane (is Box)
  (prop + label _ (= ""))
)

(fn-do + row-labels String
  (param rows Box)
  (let parts (list String) (call (of List String)))
  (for (in row (. rows children))
    (for (in pane (. (as row Box) children))
      (+= parts (. (as pane Pane) label))
    )
  )
  (return (call (. "," join) parts))
)
```

```output
a!,b!
```

### The `in` form of a static markup loop declares the lazy locals of its body the same way.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let titles (call (of List String) (list "a" "b")))
  (blank)
  (<> Box (id: root)
    (iterate (in titles)
      (block
        (lazy shown (str each "?"))
        (<> Holder
          (<> Pane
            (= label shown)
          )
        )
      )
    )
  )
  (blank)
  (print (call row-labels root))
)

(type + Holder (is Box))

(type + Pane (is Box)
  (prop + label _ (= ""))
)

(fn-do + row-labels String
  (param rows Box)
  (let parts (list String) (call (of List String)))
  (for (in row (. rows children))
    (for (in pane (. (as row Box) children))
      (+= parts (. (as pane Pane) label))
    )
  )
  (return (call (. "," join) parts))
)
```

```output
a?,b?
```

### In class content, each static markup loop body declares its own lazy locals, so two loops can use the same name.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo))
  (print (call row-labels demo))
)

(type + Demo (is Box)
  (prop + titles _ (= (call (of List String) (list "a" "b"))))
  (content
    (<> This
      (for (in title titles)
        (lazy shown (str title "!"))
        (<> Holder
          (<> Pane
            (= label shown)
          )
        )
      )
      (iterate (in titles)
        (block
          (lazy shown (str each "?"))
          (<> Holder
            (<> Pane
              (= label shown)
            )
          )
        )
      )
    )
  )
)

(type + Holder (is Box))

(type + Pane (is Box)
  (prop + label _ (= ""))
)

(fn-do + row-labels String
  (param rows Box)
  (let parts (list String) (call (of List String)))
  (for (in row (. rows children))
    (for (in pane (. (as row Box) children))
      (+= parts (. (as pane Pane) label))
    )
  )
  (return (call (. "," join) parts))
)
```

```output
a!,b!,a?,b?
```

### A lazy local that reads the loop variable is evaluated by each row that reads it, once per row, and a lazy local no row reads is never evaluated.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let titles (call (of List String) (list "a" "b")))
  (blank)
  (<> Box (id: root)
    (for (in title titles)
      (lazy pair (list (call stamp title) (str title "?")))
      (lazy unread (call stamp "unread"))
      (<> Holder
        (<> Pane
          (= label ([] pair 0))
        )
        (<> Pane
          (= label ([] pair 1))
        )
      )
    )
  )
  (blank)
  (print (call row-labels root))
)

(fn-do + stamp String
  (param text String)
  (print (str "stamp " text))
  (return (str text "!"))
)

(type + Holder (is Box))

(type + Pane (is Box)
  (prop + label _ (= ""))
)

(fn-do + row-labels String
  (param rows Box)
  (let parts (list String) (call (of List String)))
  (for (in row (. rows children))
    (for (in pane (. (as row Box) children))
      (+= parts (. (as pane Pane) label))
    )
  )
  (return (call (. "," join) parts))
)
```

```output
stamp a
stamp b
a!,a?,b!,b?
```

### Each row of a static markup loop evaluates its own lazy locals when the first read is inside an `if` arm, when the read is tracked, and when the lazy local is destructured.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let titles (call (of List String) (list "a" "b")))
  (blank)
  (<> Box (id: arms)
    (for (in title titles)
      (lazy shown (call stamp title))
      (if (== title "b")
        (<> TextBox (value: shown))
      )
      (<> TextBox (value: (str shown "+")))
    )
  )
  (blank)
  (<> Box (id: tracked)
    (for (in title titles)
      (lazy shown (str title "?"))
      (<> TextBox (value: #shown))
    )
  )
  (blank)
  (<> Box (id: pairs)
    (for (in title titles)
      (lazy (tuple first second) (call pair title))
      (<> TextBox (value: first))
      (<> TextBox (value: second))
    )
  )
  (blank)
  (print (call texts arms))
  (print (call texts tracked))
  (print (call texts pairs))
)

(fn-do + stamp String
  (param text String)
  (print (str "stamp " text))
  (return (str text "!"))
)

(fn-do + pair _
  (param text String)
  (print (str "pair " text))
  (return (tuple (str text "1") (str text "2")))
)

(fn-do + texts String
  (param rows Box)
  (let parts (list String) (call (of List String)))
  (for (in row (. rows children))
    (+= parts (. (as row TextBox) text))
  )
  (return (call (. "," join) parts))
)
```

```output
stamp a
stamp b
pair a
pair b
a!+,b!,b!+
a?,b?
a1,a2,b1,b2
```

### A lazy local of a static markup loop row that is read by the row's own content and by the rows of a tracked loop nested in it is evaluated once per row, also for rows added later.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let titles (call (of List String) (list "a" "b")))
  (let items (call (of List String) (list "x" "y")))
  (blank)
  (<> Box (id: root)
    (for (in title titles)
      (lazy shown (call stamp title))
      (<> Box
        (<> TextBox (value: shown))
        (for (in item #items)
          (<> TextBox (value: (str shown item)))
        )
      )
    )
  )
  (blank)
  (print (call texts root))
  (+= items "z")
  (print (call texts root))
)

(fn-do + stamp String
  (param text String)
  (print (str "stamp " text))
  (return (str text "!"))
)

(fn-do + texts String
  (param rows Box)
  (let parts (list String) (call (of List String)))
  (for (in row (. rows children))
    (+= parts (. (as (!! (. row %first-child)) TextBox) text))
    (for (in child (. (!! (. row %last-child)) children))
      (+= parts (. (as child TextBox) text))
    )
  )
  (return (call (. "," join) parts))
)
```

```output
stamp a
stamp b
a!,a!x,a!y,b!,b!x,b!y
a!,a!x,a!y,a!z,b!,b!x,b!y,b!z
```

### A lazy state machine declared in a static markup loop body belongs to its row.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let titles (call (of List String) (list "a" "b")))
  (blank)
  (<> Box (id: root)
    (for (in title titles)
      (lazy flow
        (machine
          (state idle)
          (state ready (trigger if (call mark title)))
        )
      )
      (<> TextBox (value: (if-expr (is flow .ready) "ready" "idle")))
    )
  )
  (blank)
  (print (call texts root))
)

(fn-do + mark Bool
  (param label String)
  (print (str "mark " label))
  (return (== label "b"))
)

(fn-do + texts String
  (param rows Box)
  (let parts (list String) (call (of List String)))
  (for (in row (. rows children))
    (+= parts (. (as row TextBox) text))
  )
  (return (call (. "," join) parts))
)
```

```output
mark a
mark b
idle,ready
```

```brix
(import /core/tests/support/markup-fixtures)
(blank)
(prop _ evaluations _ (= 0))
(blank)

(main
  (<> Looped (id: looped))
  (blank)
  (print evaluations)
  (for (in i (..< 0 4))
    (+= (. looped value) 1)
    (print evaluations)
  )
)

(fn-do _ observe _
  (param value F64)
  (+= evaluations 1)
  (return value)
)

(type + Item (is Box)
  (prop + v F64)
)

(type + Looped (is Box)
  (prop + value F64)
  (content
    (<> This
      (for (in index (..< 0 3))
        (<> Item
          (= v (call observe #value))
        )
      )
    )
  )
)
```

```output
3
6
9
12
15
```

### An attribute of an element a plain loop in a class's markup makes, which reads a tracked property, follows the property for every element the loop made.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let panel (call Panel))
  (= (. panel current) "c")
  (for (in child (. panel children))
    (print (. (as child Label) text))
  )
  (= (. panel current) "a")
  (for (in child (. panel children))
    (print (. (as child Label) text))
  )
)

(type + Panel (is Box)
  (prop + current _ (= "b"))
  (prop + items _ (= (list "a" "b" "c")))
  (content
    (<> This
      (for (in item items)
        (<> Label
          (= text (str item " " (== #current item)))
        )
      )
    )
  )
)
```

```output
a false
b false
c true
a true
b false
c false
```

### A closure an attribute of a row of a keyed loop region gives reads the row's item when it runs, the one the row holds then.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo)
    (for (in item (. demo #items) (keyed-by (. item id)))
      (<> Item
        (= run (=> () :do (call (. demo note) (. item text))))
      )
    )
  )
  (let region (!! (. demo %first-child)))
  (let first (as (!! (. region %first-child)) Item))
  (call (. first fire))
  (print (. demo seen))
  (let old (. first run))
  (= ([] (. demo items) 0) (call RowData "a" "Z"))
  (let again (as (!! (. region %first-child)) Item))
  (print (if-expr (== again first) "kept" "replaced"))
  (call (. again fire))
  (print (. demo seen))
  (print (if-expr (== old (. again run)) "same closure" "new closure"))
)

(type + Item (is Box)
  (prop + run (? Doer))
  (fn-do + fire _
    (let callback run)
    (if callback
      (call callback)
    )
  )
)

(type + RowData
  (prop + id String)
  (prop + text String)
  (fn-do + This _
    (param id String)
    (param text String)
  )
)

(type + Demo (is Box)
  (prop + items (list RowData))
  (prop + seen _ (= ""))
  (fn-do + This _
    (= items (call (of List RowData) (list (call RowData "a" "A") (call RowData "b" "B"))))
  )
  (fn-do + note _
    (param text String)
    (= seen text)
  )
)
```

```output
A
kept
Z
same closure
```

### A keyed collection controller passes its row builder's error to the code that builds it.

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (print (call build (call (of List String) (list "a" "b"))))
  (print (call build (call (of List String) (list "a" "bad"))))
)

(type + RowFailure (is Error))

(fn-do + build String
  (param items (list String))
  (let region (call Box))
  (let builder (as row-box (of MarkupCollectionRowBuilder String Box)))
  (let key-builder (as row-key (of MarkupKeyBuilder String)))
  (let held (catch (call (of MarkupCollectionController String Box) region items builder key-builder)))
  (return
    (match held
      (case (: error Error)
        (. error message)
      )
      (case (of MarkupCollectionController String Box)
        (str (call count-children region) " rows")
      )
    )
  )
)

(fn-do + row-box Box (?! RowFailure)
  (param row-item (of MarkupRowItem String))
  (throw (call RowFailure (str "Cannot build " (. row-item item))) (if (== (. row-item item) "bad")))
  (return (call TextBox (. row-item item)))
)

(fn-expr + row-key String
  (param item String)
  item
)

(fn-do + count-children _
  (param box Box)
  (let count 0)
  (for (in child (. box children))
    (+= count 1)
  )
  (return count)
)
```

```output
2 rows
Cannot build bad
```

### A named element in the row of a tracked loop gives the property of its name the object.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let h (call Holder))
  (print (!= (. h inner) null))
)

(type + Holder (is Box)
  (prop + items (list String) (= (call (of List String) (list "a"))))
  (blank)
  (prop _ inner (? Box))
  (content
    (<> This
      (for (in cur (call snaps #items))
        (<> Box (id: inner))
      )
    )
  )
)

(fn-expr _ snaps (list (list String))
  (param items (list String))
  (item items)
)
```

```output
true
```

### A loop on a stream among the lines of an element in a keyed loop reads the row's item when an item comes.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let panel (call Panel))
  (let first (as (!! (. (!! (. panel %first-child)) %first-child)) Item))
  (let second (as (!! (. first %next-sibling)) Item))
  (<- (. first clicked) 3)
  (<- (. second clicked) 4)
  (print (. panel seen))
  (call (. panel rename) 0 "z")
  (do-new
    (print (. (as (!! (. (!! (. panel %first-child)) %first-child)) Item) tag))
    (<- (. first clicked) 5)
    (print (. panel seen))
  )
)

(type + Draft (is Tuple)
  (prop + serial U64)
  (prop + name String)
  (fn-do + This _
    (param serial _)
    (param name _)
  )
)

(type + Item (is Box)
  (prop + tag _ (= ""))
  (event + clicked Int)
)

(type + Panel (is Box)
  (prop + seen _ (= ""))
  (prop + drafts (list Draft) (= (list (call Draft 1 "a") (call Draft 2 "b"))))
  (content
    (<> This
      (for (in draft #drafts (keyed-by (. draft serial)))
        (<> Item
          (= tag (. draft name))
          (for (on n clicked)
            (call note (. draft serial) n (. draft name))
          )
        )
      )
    )
  )
  (fn-do + note _
    (param serial U64)
    (param n Int)
    (param name String)
    (= seen (++ seen (str serial ":" n ":" name ";")))
  )
  (fn-do + rename _
    (param index Index)
    (param name String)
    (let next (call (of List Draft) drafts))
    (= ([] next index) (call Draft (. ([] next index) serial) name))
    (= drafts next)
  )
)
```

```output
1:3:a;2:4:b;
z
1:3:a;2:4:b;1:5:z;
```

### In the body of an element of a keyed row, a name the row gives is the row's item, though the element has a member of the name too: the body reads the scope outside the element.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let lane (call Lane))
  (= (. lane items) (list (call Row "a" true) (call Row "b" false) (call Row "c" true)))
  (print (call dump lane))
)

(type + Row (is Tuple)
  (prop + id String)
  (prop + resizable Bool)
)

(type + ItemBox (is Box)
  (prop + item Row (= (call Row "dflt" true)))
)

(type + Lane (is Box)
  (prop + items (list Row) (= (call (of List Row))))
  (content
    (<> This
      (for (in item #items (keyed-by (. item id)))
        (<> ItemBox
          (= item (call Row (str "outer-" (. item id)) true))
          (if (. item resizable)
            (<> ItemBox
              (= item item)
            )
          )
        )
      )
    )
  )
)

(fn-do + dump String
  (param lane Lane)
  (let parts (list String) (call (of List String)))
  (let region (!! (. lane %first-child)))
  (for (in row (. region children))
    (let box (as row ItemBox))
    (let inner "-")
    (for (in child (. box children))
      (= inner (. (as child ItemBox) item id))
    )
    (+= parts (str (. box item id) "/" inner))
  )
  (return (call (. "," join) parts))
)
```

```output
outer-a/a,outer-b/-,outer-c/c
```

### A name in the body of a markup element is the local outside the element, though the element has an event of the name: a handler reads the local.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let root (call Host))
  (call fire root)
  (print (call (. ", " join) (. root seen)))
)

(fn-do + fire _
  (param box Box)
  (for (in child (. box children))
    (if (is child Action)
      (<- (. (as child Action) click) 1)
      (else
        (call fire child)
      )
    )
  )
)

(type + Delta (is Tuple)
  (prop + text String)
)

(type + Host (is Box)
  (prop + seen (list String) (= (call (of List String))))
  (prop _ changes _ (= (call (of List Delta) (list (call Delta "a") (call Delta "b")))))
  (content
    (<> This
      (for (in change changes)
        (<> Action
          (= label (. change text))
          (for (on event click)
            (+= seen (. change text))
          )
        )
      )
    )
  )
)

(type + Action (is Box)
  (prop + label _ (= ""))
  (event + change Int)
  (event + click Int)
)
```

```output
a, b
```

### A name in the attribute of an element in the body of another is the local outside both, though the element around it has a property of the name.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let root (call Host))
  (print (call (. "," join) (call labels root)))
)

(fn-do + labels (list String)
  (param box Box)
  (let parts (list String) (call (of List String)))
  (for (in child (. box children))
    (if (is child Action)
      (let label (. (as child Action) label))
      (if (!= label "")
        (+= parts label)
      )
    )
    (for (in label (call labels child))
      (+= parts label)
    )
  )
  (return parts)
)

(type + Delta (is Tuple)
  (prop + text String)
)

(type + Host (is Box)
  (prop _ changes _ (= (call (of List Delta) (list (call Delta "a") (call Delta "b")))))
  (content
    (<> This
      (for (in change changes)
        (<> Action
          (<> Action
            (= label (. change text))
          )
        )
      )
    )
  )
)

(type + Action (is Box)
  (prop + label _ (= ""))
  (prop + change _ (= 0))
)
```

```output
a,b
```

### A line of an element in the row of a keyed loop region reads the row's item as it is, under no live action.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo)
    (for (in item (. demo #items) (keyed-by (. item id)))
      (<> Box
        (if (!= (. item text) "")
          (<> TextBox (value: (. item text)))
        )
        (<> TextBox (value: (. item id)))
      )
    )
  )
  (blank)
  (print (call dump demo))
  (call (. demo items insert) (call RowData "y" "") 0)
  (print (call dump demo))
)

(fn-do + dump String
  (param demo Demo)
  (let text "")
  (for (in row (. (!! (. demo %first-child)) children))
    (for (in child (. row children))
      (+= text (str (. (as child TextBox) text) ";"))
    )
  )
  (return text)
)

(type + RowData
  (prop + id String)
  (prop + text String)
  (fn-do + This _
    (param id String)
    (param text String)
  )
)

(type + Demo (is Box)
  (prop + items (list RowData))
  (fn-do + This _
    (= items (call (of List RowData) (list (call RowData "x" "X"))))
  )
)
```

```output
X;x;
y;X;x;
```

### An item of a keyed loop read untracked for its key and tracked in its body updates when the tracked property changes.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo)
    (for (in item (call (. demo listed) (. demo #count)) (keyed-by (. item id)))
      (<> TextBox (id: text) (value: (call label (. item #text))))
    )
  )
  (print (call dump demo))
  (= (. ([] (. demo items) 0) text) "changed")
  (print (call dump demo))
)

(fn-expr + label String
  (param text String)
  (str "<" text ">")
)

(fn-expr + dump String
  (param demo Demo)
  (str first-text ":" last-text)
  (where
    (lazy region (!! (. demo %first-child)))
    (lazy first-text (. (as (!! (. region %first-child)) TextBox) text))
    (lazy last-text (. (as (!! (. region %last-child)) TextBox) text))
  )
)

(type + Demo (is Box)
  (prop + items (list RowData))
  (prop + count _ (= 0))
  (fn-expr + listed (list RowData)
    (param n Int)
    items
  )
  (fn-do + This _
    (= items (call (of List RowData) (list (call RowData "a" "A") (call RowData "b" "B"))))
  )
)

(type + RowData
  (prop + id String)
  (prop + text String)
  (fn-do + This _
    (param id String)
    (param text String)
  )
)
```

```output
<A>:<B>
<changed>:<B>
```

### A handler in the row of a loop, in the body of an element written among a module's statements, assigns a property of that element through the element's name.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo)
    (for (in item (. demo #items) (keyed-by item))
      (<> Pinger
        (for (on n pings)
          (= (. demo chosen) item)
          (+= (. demo total) n)
        )
      )
    )
  )
  (blank)
  (let first (as (!! (. (!! (. demo %first-child)) %first-child)) Pinger))
  (let second (as (!! (. first %next-sibling)) Pinger))
  (<- (. first pings) 2)
  (print (str (. demo chosen) ":" (. demo total)))
  (<- (. second pings) 5)
  (print (str (. demo chosen) ":" (. demo total)))
)

(type + Pinger (is Box)
  (event + pings Int)
)

(type + Demo (is Box)
  (prop + items (list String))
  (prop + chosen _ (= "none"))
  (prop + total _ (= 0))
  (fn-do + This _
    (= items (call (of List String) (list "a" "b")))
  )
)
```

```output
a:2
b:7
```

### A value given again where the body of an element is written, as an argument or an attribute of an element there, reads a member of an element around by its name alone, tracked.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo)
    (<> TextBox (value: (. demo #chosen)))
    (<> Box
      (<> TextBox
        (= text (str "<" (. demo #chosen) ">"))
      )
    )
  )
  (blank)
  (= (. demo chosen) "x")
  (print (. (as (!! (. demo %first-child)) TextBox) text))
  (print (. (as (!! (. (!! (. demo %last-child)) %first-child)) TextBox) text))
)

(type + Demo (is Box)
  (prop + chosen _ (= "none"))
)
```

```output
x
<x>
```

### A markup loop over an optional source has no rows while the source holds none and follows it once it holds a collection, as when the source initializes after its markup.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let demo (call Demo))
  (print (call dump demo))
  (= (. demo picker picked) 3)
  (print (call dump demo))
  (= (. demo picker picked) null)
  (print (call dump demo))
)

(fn-do + dump String
  (param demo Demo)
  (let region (!! (. demo %first-child)))
  (let count (call count-children region))
  (if (== count 0)
    (return "0")
  )
  (return (str count ":" (. (as (!! (. region %first-child)) TextBox) text) ":" (. (as (!! (. region %last-child)) TextBox) text)))
)

(fn-do + count-children _
  (param box Box)
  (let count 0)
  (for (in child (. box children))
    (+= count 1)
  )
  (return count)
)

(fn-do + labels (? (list String))
  (param chosen (? Size))
  (if (not chosen)
    (return null)
  )
  (let rows (list String))
  (for (in index (..< 0 chosen))
    (+= rows (str "row " index))
  )
  (return rows)
)

(type + Demo (is Box)
  (prop + chosen (? Size) (= (. picker #picked)))
  (prop + rows (? (list String)) (= (call labels #chosen)))
  (content
    (<> This
      (for (in row #rows (keyed-by row))
        (<> TextBox (value: row))
      )
      (<> Picker (id: picker))
    )
  )
)

(type + Picker (is Box)
  (prop + picked (? Size) (= 2))
)
```

```output
2:row 0:row 1
3:row 0:row 2
0
```

### An unkeyed markup loop over an optional collection follows it as a keyed one does.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let items (? (list String)) (call (of List String) (list "a" "b")))
  (blank)
  (<> Box (id: box)
    (for (in item #items)
      (<> TextBox (value: item))
    )
  )
  (blank)
  (print (call dump box))
  (= items null)
  (print (call dump box))
  (= items (call (of List String) (list "c")))
  (print (call dump box))
)

(fn-do + dump String
  (param box Box)
  (let texts (list String))
  (for (in child (. (!! (. box %first-child)) children))
    (+= texts (. (as child TextBox) text))
  )
  (return (str (. texts length) ":" (call (. "," join) texts)))
)
```

```output
2:a,b
0:
1:c
```

### An optional container that converts to the stream a markup loop reads is converted while it holds one, as a container that is never null is.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let items (? (of Iterable String)) (call letters))
  (blank)
  (<> Box (id: box)
    (for (in item #items)
      (<> TextBox (value: item))
    )
  )
  (blank)
  (call yield-once)
  (print (call dump box))
  (= items null)
  (print (call dump box))
)

(fn-do + letters (of Iterable String)
  (yield "a")
  (yield "b")
)

(fn-do + yield-once _
  (let done (call Semaphore))
  (do-new
    (call (. done post))
  )
  (call (. done wait))
)

(fn-do + dump String
  (param box Box)
  (let texts (list String))
  (for (in child (. (!! (. box %first-child)) children))
    (+= texts (. (as child TextBox) text))
  )
  (return (str (. texts length) ":" (call (. "," join) texts)))
)
```

```output
2:a,b
0:
```

### A loop over a collection read without tracking is no region, so it has no rows to key.

```brix crash
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let items (call (of List String) (list "a")))
  (blank)
  (<> Box (id: box)
    (for (in item items (keyed-by item))
      (<> TextBox (value: item))
    )
  )
)
```

```output
Exception: keyed by is only valid on markup loops
<no file:9>
        (for (in item items (keyed-by item))
                            ^^^^^^^^^^^^^^^
```

### A tracked property tested before a row's item is read in the same expression still holds a value after it.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo))
  (blank)
  (print (. (as (!! (. (!! (. demo %first-child)) %first-child)) TextBox) text))
  (= (. demo limit) 1)
  (print (. (as (!! (. (!! (. demo %first-child)) %first-child)) TextBox) text))
)

(type + Demo (is Box)
  (prop + limit (? Int))
  (prop + rows _ (= (list 2)))
  (content
    (<> This
      (for (in row #rows (keyed-by row))
        (<> TextBox
          (= text (str (and (!= #limit null) (>= row #limit))))
        )
      )
    )
  )
)
```

```output
false
true
```

### A keyed row that departs stays where it was until it has gone, and revives when its key returns.

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)
(prop + log _ (= (call (of List String))))
(blank)

(main
  (<> Demo (id: demo)
    (for (in name (. demo #names) (keyed-by name))
      (<> Leaver
        (= name name)
      )
    )
  )
  (blank)
  (print (call dump demo))
  (call (. demo names remove) 1)
  (print (call dump demo))
  (call (. demo names insert) "b" 1)
  (print (call dump demo))
  (call (. demo names remove) 2)
  (print (call dump demo))
  (call markup-departed (!! (. (!! (. demo %first-child)) %last-child)))
  (print (call dump demo))
  (print (call (. " " join) log))
)

(fn-do + dump String
  (param demo Demo)
  (let parts (call (of List String)))
  (for (in child (. (!! (. demo %first-child)) children))
    (let leaver (as child Leaver))
    (if (. leaver is-leaving)
      (+= parts (str (. leaver name) "*"))
      (else
        (+= parts (. leaver name))
      )
    )
  )
  (return (call (. " " join) parts))
)

(type + Demo (is Box)
  (prop + names _ (= (call (of List String) (list "a" "b" "c"))))
)

(type + Leaver (is Box)
  (prop + name _ (= ""))
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
    (+= log (str "close:" name))
    (call (. super close))
  )
)
```

```output
a b c
a b* c
a b c
a b c*
a b
build build build close:c
```

### Making the rows of a loop is not tracked by the loop's action. A row that reads a style-sheet slot while it is made does not run the loop again when that read settles after mounting.

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
  (= (. demo items) (call (of List I64) (list 1 2)))
  (call (. demo markup-batch-queue checkpoint))
  (print (str "builds=" builds " pending=" (. demo markup-batch-queue actions length)))
)

(type + Demo (is Box)
  (implements MarkupRootElement)
  (using AppTheme)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (prop + items _ (= (call (of List I64) (list 1))))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
  (content
    (<> This
      (for (in item #items)
        (<> Row
          (= value item)
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
builds=3 pending=0
```

### A function written where a list is declared adds to it and takes from it, and a loop over the list follows.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let items (list "a" "b"))
  (<> Box (id: box)
    (for (in item #items)
      (<> TextBox (value: item))
    )
  )
  (let add (=> () :do (+= items "c")))
  (let take (=> () :do (-= items "a")))
  (call add)
  (print (call texts box))
  (call take)
  (print (call texts box))
)

(fn-do + texts String
  (param box Box)
  (let seen (list String))
  (for (in child (. (!! (. box %first-child)) children))
    (+= seen (. (as child TextBox) text))
  )
  (return (call (. ", " join) seen))
)
```

```output
a, b, c
b, c
```

### A handler in the row of a keyed loop stores to the list the loop reads, declared where the loop is written, with the row's item in what it stores, and the loop follows.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let items (list "a" "b" "c"))
  (<> Box (id: box)
    (for (in item #items (keyed-by item))
      (<> Pinger
        (= text item)
        (for (on n pings)
          (= items (list item (str item n)))
        )
      )
    )
  )
  (<- (. (as (!! (. (!! (. box %first-child)) %last-child)) Pinger) pings) 2)
  (print (call texts box))
)

(type + Pinger (is TextBox)
  (event + pings Int)
)

(fn-do + texts String
  (param box Box)
  (let seen (list String))
  (for (in child (. (!! (. box %first-child)) children))
    (+= seen (. (as child TextBox) text))
  )
  (return (call (. ", " join) seen))
)
```

```output
c, c2
```

### A handler in the row of a keyed loop reads the row's item in the value of an assignment written with an operator other than `=` when it runs, the item the row holds then, and the assignment does not run again as the item changes.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let items (list (call Entry "a" "A") (call Entry "b" "B")))
  (let seen (list String))
  (<> Box (id: box)
    (for (in item #items (keyed-by (. item id)))
      (<> Pinger
        (for (on n pings)
          (+= seen (. item text))
        )
      )
    )
  )
  (let first (as (!! (. (!! (. box %first-child)) %first-child)) Pinger))
  (<- (. first pings) 1)
  (= ([] items 0) (call Entry "a" "Z"))
  (let again (as (!! (. (!! (. box %first-child)) %first-child)) Pinger))
  (print (if-expr (== again first) "kept" "replaced"))
  (print (call (. ", " join) seen))
  (<- (. again pings) 1)
  (print (call (. ", " join) seen))
)

(type + Pinger (is Box)
  (event + pings Int)
)

(type + Entry
  (prop + id String)
  (prop + text String)
  (fn-do + This _
    (param id String)
    (param text String)
  )
)
```

```output
kept
A
A, Z
```

### A closure an attribute of a row of a keyed loop gives takes the row's item from the list the loop reads, and the loop follows.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let items (list "a" "b" "c"))
  (<> Box (id: box)
    (for (in item #items (keyed-by item))
      (<> Item
        (= text item)
        (= run (=> () :do (-= items item)))
      )
    )
  )
  (call (. (as (!! (. (!! (. (!! (. box %first-child)) %first-child)) %next-sibling)) Item) fire))
  (print (call texts box))
)

(type + Item (is TextBox)
  (prop + run (? Doer))
  (fn-do + fire _
    (let callback run)
    (if callback
      (call callback)
    )
  )
)

(fn-do + texts String
  (param box Box)
  (let seen (list String))
  (for (in child (. (!! (. box %first-child)) children))
    (+= seen (. (as child TextBox) text))
  )
  (return (call (. ", " join) seen))
)
```

```output
a, c
```

### A keyed row that is a region keeps its live `if` following the item its key holds now.

```brix
(import /core/markup)
(blank)

(main
  (let todos (call (of List Todo) (list (call Todo "a" false) (call Todo "b" true))))
  (<> Board (id: board)
    (for (in todo #todos (keyed-by (. todo name)))
      (<> Text (value: (. todo name)))
      (if (. todo #done)
        (<> Text (value: "done"))
      )
    )
  )
  (call (. board show))
  (= (. ([] todos 0) done) true)
  (call (. board show))
  (= todos (call (of List Todo) (list (call Todo "a" false) (call Todo "b" true))))
  (call (. board show))
  (= (. ([] todos 1) done) false)
  (call (. board show))
)

(type + Todo
  (prop + name String)
  (prop + done Bool)
  (fn-do + This _
    (param name _)
    (param done _)
  )
)

(type + Text (is MarkupNode)
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(type + Board (is MarkupNode)
  (fn-do + show _
    (let parts (call (of List String)))
    (for (in child (call (of children-of Text)))
      (+= parts (. child text))
    )
    (print (call (. ", " join) parts))
  )
)
```

```output
a, b, done
a, done, b, done
a, b, done
a, b
```

### An if or a match that is a line of a keyed row reads the row's item tracked, so it selects its arm again when the key comes to hold another item.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let places (call (of List Place) (list (call Place "a" true) (call Place "b" false))))
  (blank)
  (<> Box (id: root)
    (for (in place #places (keyed-by (. place name)))
      (if (. place flag)
        (<> TextBox (value: (str (. place name) " on")))
        (else
          (<> TextBox (value: (str (. place name) " off")))
        )
      )
    )
  )
  (<> Box (id: kinds)
    (for (in place #places (keyed-by (. place name)))
      (match (. place flag)
        (case= true
          (<> TextBox (value: "yes"))
        )
        (case= false
          (<> TextBox (value: "no"))
        )
      )
    )
  )
  (blank)
  (print (str (call dump root) " | " (call dump kinds)))
  (= places (call (of List Place) (list (call Place "a" false) (call Place "b" true))))
  (print (str (call dump root) " | " (call dump kinds)))
  (+= places (call Place "c" true))
  (print (str (call dump root) " | " (call dump kinds)))
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

(type + Place
  (prop + name String)
  (prop + flag Bool)
  (fn-do + This _
    (param name _)
    (param flag _)
  )
)
```

```output
[[[a on]],[[b off]]] | [[[yes]],[[no]]]
[[[a off]],[[b on]]] | [[[no]],[[yes]]]
[[[a off]],[[b on]],[[c on]]] | [[[no]],[[yes]],[[yes]]]
```

### So does one in a row of a loop in a function's markup.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (call run)
)

(fn-do + run _
  (let places (call (of List Place) (list (call Place "a" true) (call Place "b" false))))
  (<> Box (id: root)
    (for (in place #places (keyed-by (. place name)))
      (if (. place flag)
        (<> TextBox (value: (str (. place name) " on")))
        (else
          (<> TextBox (value: (str (. place name) " off")))
        )
      )
    )
  )
  (print (call dump root))
  (= places (call (of List Place) (list (call Place "a" false) (call Place "b" true))))
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

(type + Place
  (prop + name String)
  (prop + flag Bool)
  (fn-do + This _
    (param name _)
    (param flag _)
  )
)
```

```output
[[[a on]],[[b off]]]
[[[a off]],[[b on]]]
```

### A named element in an arm of an if that is a line of a keyed row is made by the arm.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let places (call (of List Place) (list (call Place "a" true) (call Place "b" false))))
  (blank)
  (<> Box (id: root)
    (for (in place #places (keyed-by (. place name)))
      (if (. place flag)
        (<> TextBox (id: on) (value: (str (. place name) " on")))
        (else
          (<> TextBox (id: off) (value: (str (. place name) " off")))
        )
      )
    )
  )
  (blank)
  (print (call dump root))
  (= places (call (of List Place) (list (call Place "a" false) (call Place "b" true))))
  (print (call dump root))
  (+= places (call Place "c" true))
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

(type + Place
  (prop + name String)
  (prop + flag Bool)
  (fn-do + This _
    (param name _)
    (param flag _)
  )
)
```

```output
[[[a on]],[[b off]]]
[[[a off]],[[b on]]]
[[[a off]],[[b on]],[[c on]]]
```

### A row that is a region makes the elements it names where it starts, so a later line reads them.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let places (call (of List Place) (list (call Place "a" true) (call Place "b" false))))
  (blank)
  (<> Box (id: root)
    (for (in place #places (keyed-by (. place name)))
      (<> TextBox (id: first) (value: (. place name)))
      (<> TextBox (value: (str (. first text) "!")))
      (if (. place flag)
        (<> TextBox (id: inner) (value: "on"))
      )
    )
  )
  (blank)
  (print (call dump root))
  (+= places (call Place "c" true))
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

(type + Place
  (prop + name String)
  (prop + flag Bool)
  (fn-do + This _
    (param name _)
    (param flag _)
  )
)
```

```output
[[a,a!,[on]],[b,b!,[]]]
[[a,a!,[on]],[b,b!,[]],[c,c!,[on]]]
```

### So does a row of a loop in a function's markup, with named elements in the arms of a plain if.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (call run)
)

(fn-do + run _
  (let places (call (of List Place) (list (call Place "a" true) (call Place "b" false))))
  (<> Box (id: root)
    (for (in place #places)
      (if (. place flag)
        (<> TextBox (id: on) (value: (str (. place name) " on")))
        (else
          (<> TextBox (id: off) (value: (str (. place name) " off")))
        )
      )
      (<> TextBox (value: "|"))
    )
  )
  (print (call dump root))
  (+= places (call Place "c" false))
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

(type + Place
  (prop + name String)
  (prop + flag Bool)
  (fn-do + This _
    (param name _)
    (param flag _)
  )
)
```

```output
[[a on,|],[b off,|]]
[[a on,|],[b off,|],[c off,|]]
```

### A row may hold a match, and a live match selects its arm again in each row.

```brix
(import /core/markup)
(blank)

(main
  (let items (call (of List Int) (list 1 2)))
  (let mode "a")
  (<> Board (id: board)
    (for (in n #items)
      (match n
        (case 1
          (<> Text (value: "one"))
        )
        (case _
          (<> Text (value: "many"))
        )
      )
      (match #mode
        (case "a"
          (<> Text (value: (str n ":a")))
        )
        (case _
          (<> Text (value: (str n ":other")))
        )
      )
    )
  )
  (call (. board show))
  (= mode "b")
  (call (. board show))
)

(type + Text (is MarkupNode)
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(type + Board (is MarkupNode)
  (fn-do + show _
    (let parts (call (of List String)))
    (for (in child (call (of children-of Text)))
      (+= parts (. child text))
    )
    (print (call (. ", " join) parts))
  )
)
```

```output
one, 1:a, many, 2:a
one, 1:other, many, 2:other
```

### A `when` clause on a loop that follows state is tested in each row on its own, as the item changes.

```brix
(import /core/markup)
(blank)

(main
  (let todos (call (of List Todo) (list (call Todo "a" false) (call Todo "b" true) (call Todo "c" false))))
  (<> Board (id: board)
    (for (in todo #todos (when (not (. todo #done))))
      (<> Text (value: (. todo name)))
    )
  )
  (call (. board show))
  (= (. ([] todos 1) done) false)
  (call (. board show))
  (= (. ([] todos 0) done) true)
  (call (. board show))
  (+= todos (call Todo "d" true))
  (call (. board show))
  (= (. ([] todos 3) done) false)
  (call (. board show))
)

(type + Todo
  (prop + name String)
  (prop + done Bool)
  (fn-do + This _
    (param name _)
    (param done _)
  )
)

(type + Text (is MarkupNode)
  (prop + text String)
  (fn-do + This _
    (param text _)
    (print (str "  make " text))
  )
)

(type + Board (is MarkupNode)
  (fn-do + show _
    (let parts (call (of List String)))
    (for (in child (call (of children-of Text)))
      (+= parts (. child text))
    )
    (print (call (. ", " join) parts))
  )
)
```

```output
  make a
  make c
a, c
  make b
a, b, c
b, c
b, c
  make d
b, c, d
```

### A keyed row tests its `when` clause again when its key comes to hold another item.

```brix
(import /core/markup)
(blank)

(main
  (let todos (call (of List Todo) (list (call Todo "a" false) (call Todo "b" true))))
  (<> Board (id: board)
    (for (in todo #todos (keyed-by (. todo name)) (when (. todo #done)))
      (<> Text (value: (. todo name)))
      (<> Text (value: "!"))
    )
  )
  (call (. board show))
  (= todos (call (of List Todo) (list (call Todo "a" true) (call Todo "b" false))))
  (call (. board show))
  (= (. ([] todos 1) done) true)
  (call (. board show))
)

(type + Todo
  (prop + name String)
  (prop + done Bool)
  (fn-do + This _
    (param name _)
    (param done _)
  )
)

(type + Text (is MarkupNode)
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(type + Board (is MarkupNode)
  (fn-do + show _
    (let parts (call (of List String)))
    (for (in child (call (of children-of Text)))
      (+= parts (. child text))
    )
    (print (call (. ", " join) parts))
  )
)
```

```output
b, !
a, !
a, !, b, !
```

### A `when` clause on a loop that follows state reads a tracked local nowhere it could follow it.

```brix compile-error
(import /core/markup)
(blank)

(main
  (let names (call (of List String) (list "a")))
  (let long false)
  (<> Board (id: board)
    (for (in name #names (when (or (> (. name length) 1) #long)))
      (<> Text (value: name))
    )
  )
  (= long true)
)

(type + Text (is MarkupNode)
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(type + Board (is MarkupNode))
```

```output
Exception: The when clause of a markup loop that follows state tracks the row's item and properties, not a local; keep "long" in a property and read it with "#"
<no file:9>
        (for (in name #names (when (or (> (. name length) 1) #long)))
                                                             ^^^^^
```
