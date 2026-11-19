# Markup nodes

### A class that implements `MarkupParent` writes none of its members: the interface takes each child as the class it names, and `children` gives them back as that class.

```brix
(main
  (<> Shelf (id: shelf)
    (<> Book (value: "Dune"))
    (<> Book (value: "Emma"))
  )
  (blank)
  (for (in book (. shelf children))
    (print (. book title))
  )
)

(type + Book
  (implements (of MarkupParent Book))
  (prop + title String)
  (fn-do + This _
    (param title _)
  )
)

(type + Shelf
  (implements (of MarkupParent Book))
)
```

```output
Dune
Emma
```

### A `MarkupRoot` works its tree's styles out once its markup has been built, so `$` values are right as soon as the tree is made.

```brix
(main
  (<> Shelf (id: shelf)
    (<> Book (value: "Dune"))
    (<> Book (value: "Emma") (• loud))
  )
  (blank)
  (for (in book (call (of (. shelf children-of) Book)))
    (print (str (. book title) " " (. book $volume)))
  )
)

(type + Reading (is Style)
  (prop + volume I64 @inherited (= 1))
)

(style Library
  (using Reading)
  (rule Shelf
    (= volume 2)
  )
  (blank)
  (rule loud
    (= volume 9)
  )
)

(type + Book (is MarkupNode)
  (using Reading)
  (prop + title String)
  (fn-do + This _
    (param title _)
  )
)

(type + Shelf (is MarkupRoot)
  (using Library)
)
```

```output
Dune 2
Emma 9
```

### `settle` works out what a tracked change has changed in a root's styles since they were last worked out.

```brix
(main
  (<> Cart (id: cart)
    (<> Line (value: "kettle"))
    (<> Line (value: "tea") (• food))
  )
  (blank)
  (call (. cart show))
  (= (. cart is-member) true)
  (call (. cart settle))
  (call (. cart show))
)

(type + Pricing (is Style)
  (prop + discount I64 @inherited (= 0))
  (prop + tax I64 (= 20))
)

(style Policy
  (using Pricing)
  (rule Cart
    (when-style #is-member
      (= discount 10)
    )
  )
  (blank)
  (rule food
    (= tax 0)
  )
)

(type + Line (is MarkupNode)
  (using Pricing)
  (prop + name String)
  (fn-do + This _
    (param name _)
  )
)

(type + Cart (is MarkupRoot)
  (using Policy)
  (prop + is-member _ (= false))
  (fn-do + show _
    (for (in line (call (of children-of Line)))
      (print (str (. line name) " " (. line $discount) " " (. line $tax)))
    )
  )
)
```

```output
kettle 0 20
tea 0 0
kettle 10 20
tea 10 0
```

### The rows of a live loop and the content of a live branch are children of the node that holds them: `children` looks through the regions that hold them.

```brix
(main
  (<> Group (id: group)
    (<> Leaf (value: "first"))
    (for (in name (. group #names) (keyed-by name))
      (<> Leaf (value: name))
    )
    (if (== (. group #names length) 3)
      (<> Leaf (value: "three"))
    )
  )
  (blank)
  (call (. group show))
  (+= (. group names) "d")
  (call (. group settle))
  (call (. group show))
)

(type + Leaf (is MarkupNode)
  (prop + name String)
  (fn-do + This _
    (param name _)
  )
)

(type + Group (is MarkupRoot)
  (prop + names (list String))
  (fn-do + This _
    (= names (call (of List String) (list "a" "b" "c")))
  )
  (fn-do + show _
    (let shown (call (of List String)))
    (for (in leaf (call (of children-of Leaf)))
      (+= shown (. leaf name))
    )
    (print (call (. " " join) shown))
  )
)
```

```output
first a b c three
first a b c d
```
