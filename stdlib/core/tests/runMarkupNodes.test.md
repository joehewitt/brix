# Markup nodes

### A class that implements `MarkupParent` writes none of its members: the interface takes each child as the class it names, and `children` gives them back as that class.

```yumi
<Shelf shelf
    <Book = "Dune"
    <Book = "Emma"

for book in shelf.children
    -- book.title

━━━━━━━━━━━━━━━━━━━━
+ Book
    is MarkupParent'Book

+ title String

────────────────────
+ This
    - title

━━━━━━━━━━━━━━━━━━━━
+ Shelf
    is MarkupParent'Book
```

```output
Dune
Emma
```

### A `MarkupRoot` works its tree's styles out once its markup has been built, so `$` values are right as soon as the tree is made.

```yumi
<Shelf shelf
    <Book = "Dune"
    <Book = "Emma" • loud

for book in shelf.children-of'Book()
    -- "${book.title} ${book.$volume}"

━━━━━━━━━━━━━━━━━━━━
+ Reading is Style

+ @inherited volume I64 = 1

━━━━━━━━━━━━━━━━━━━━
• Library
    using Reading

• Shelf
    volume = 2

• loud
    volume = 9

━━━━━━━━━━━━━━━━━━━━
+ Book is MarkupNode
    using Reading

+ title String

────────────────────
+ This
    - title

━━━━━━━━━━━━━━━━━━━━
+ Shelf is MarkupRoot
    using Library
```

```output
Dune 2
Emma 9
```

### `settle` works out what a tracked change has changed in a root's styles since they were last worked out.

```yumi
<Cart cart
    <Line = "kettle"
    <Line = "tea" • food

cart.show()
cart.is-member = true
cart.settle()
cart.show()

━━━━━━━━━━━━━━━━━━━━
+ Pricing is Style

+ @inherited discount I64 = 0
+ tax I64 = 20

━━━━━━━━━━━━━━━━━━━━
• Policy
    using Pricing

• Cart
    when #is-member
        discount = 10

• food
    tax = 0

━━━━━━━━━━━━━━━━━━━━
+ Line is MarkupNode
    using Pricing

+ name String

────────────────────
+ This
    - name

━━━━━━━━━━━━━━━━━━━━
+ Cart is MarkupRoot
    using Policy

+ is-member = false

────────────────────
+ do show

for line in children-of'Line()
    -- "${line.name} ${line.$discount} ${line.$tax}"
```

```output
kettle 0 20
tea 0 0
kettle 10 20
tea 10 0
```

### The rows of a live loop and the content of a live branch are children of the node that holds them: `children` looks through the regions that hold them.

```yumi
<Group group
    <Leaf = "first"
    for name in group.#names keyed by name
        <Leaf = name
    if group.#names.length == 3
        <Leaf = "three"

group.show()
group.names += "d"
group.settle()
group.show()

━━━━━━━━━━━━━━━━━━━━
+ Leaf is MarkupNode

+ name String

────────────────────
+ This
    - name

━━━━━━━━━━━━━━━━━━━━
+ Group is MarkupRoot

+ names [String]

────────────────────
+ This

names = List'String(["a", "b", "c"])

────────────────────
+ do show

. shown = List'String()
for leaf in children-of'Leaf()
    shown += leaf.name
-- " ".join(shown)
```

```output
first a b c three
first a b c d
```
