# Markup names

### Each arm and loop body written as a line of markup content names the elements written in it, and makes them where it starts each time it runs, so its rows read the names, above the element too, and the element's handlers read them; a handler's body makes its own where they are written.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let model (call Model))
  (let flag true)
  (let word (? String) "n")
  (blank)
  (<> Box (id: root)
    (if flag
      (<> TextBox (value: (str "<" (. later label))))
      (<> Pinger (id: later)
        (= label "if")
        (for (on event pinged)
          (+= (. later log) (str "ping " (. later label)))
        )
      )
      (<> TextBox (value: (str (. later label) ">")))
    )
    (if (. model #flag)
      (<> TextBox (value: (str "<" (. later label))))
      (<> Pinger (id: later)
        (= label "live")
        (for (on event pinged)
          (+= (. later log) (str "ping " (. later label)))
        )
      )
      (<> TextBox (value: (str (. later label) ">")))
    )
    (if (not flag)
      (<> Box)
      (else
        (<> TextBox (value: (str "<" (. later label))))
        (<> Pinger (id: later)
          (= label "else")
          (for (on event pinged)
            (+= (. later log) (str "ping " (. later label)))
          )
        )
        (<> TextBox (value: (str (. later label) ">")))
      )
    )
    (match word
      (case (: text String)
        (<> TextBox (value: (str "<" (. later label))))
        (<> Pinger (id: later)
          (= label (str "static " text))
          (for (on event pinged)
            (+= (. later log) (str "ping " (. later label)))
          )
        )
        (<> TextBox (value: (str (. later label) ">")))
      )
      (case _
        (<> Box)
      )
    )
    (match (. model #name)
      (case "x"
        (<> Box)
      )
      (case (: text String)
        (<> TextBox (value: (str "<" (. later label))))
        (<> Pinger (id: later)
          (= label "match")
          (for (on event pinged)
            (+= (. later log) (str "ping " (. later label)))
          )
        )
        (<> TextBox (value: (str (. later label) ">")))
      )
    )
    (for (in item (. model #items) (keyed-by item))
      (if (!= item "")
        (<> TextBox (value: (str "<" (. later label))))
        (<> Pinger (id: later)
          (= label (str "row " item))
          (for (on event pinged)
            (+= (. later log) (str "ping " (. later label)))
          )
        )
        (<> TextBox (value: (str (. later label) ">")))
      )
    )
    (<> Pinger (id: handled)
      (= label "handler")
      (for (on event pinged)
        (<> TextBox (id: made) (value: (str "made " (. handled label))))
        (+= (. handled log) (. made text))
      )
    )
  )
  (blank)
  (print (call dump root))
  (print (call pings root))
  (= (. model flag) false)
  (= (. model flag) true)
  (= (. model name) null)
  (= (. model name) "n")
  (+= (. model items) "b")
  (print (call dump root))
  (print (call pings root))
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

(fn-do + pings String
  (param box Box)
  (let heard (call (of List String)))
  (call collect box heard)
  (return (call (. "," join) heard))
)

(fn-do + collect _
  (param box Box)
  (param heard (list String))
  (for (in child (. box children))
    (if (is child Pinger)
      (let pinger (as child Pinger))
      (<- (. pinger pinged) 1)
      (for (in line (. pinger log))
        (+= heard line)
      )
      (= (. pinger log) (call (of List String)))
    )
    (call collect child heard)
  )
)

(type + Pinger (is Box)
  (prop + label _ (= ""))
  (prop + log _ (= (call (of List String))))
  (event + pinged Int)
)

(type + Model
  (prop + flag _ (= true))
  (prop + name (? String) (= "n"))
  (prop + items _ (= (call (of List String) (list "a"))))
)
```

```output
<if,[],if>,[<live,[],live>],<else,[],else>,<static n,[],static n>,[<match,[],match>],[[[<row a,[],row a>]]],[]
ping if,ping live,ping else,ping static n,ping match,ping row a,made handler
<if,[],if>,[<live,[],live>],<else,[],else>,<static n,[],static n>,[<match,[],match>],[[[<row a,[],row a>]],[[<row b,[],row b>]]],[]
ping if,ping live,ping else,ping static n,ping match,ping row a,ping row b,made handler
```

### So does each in a class's content, whose rows read the class's members by name.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let demo (call Demo))
  (print (call dump demo))
  (print (call pings demo))
  (= (. demo flag) false)
  (= (. demo flag) true)
  (= (. demo name) null)
  (= (. demo name) "n")
  (+= (. demo items) "b")
  (print (call dump demo))
  (print (call pings demo))
)

(type + Demo (is Box)
  (prop + flag _ (= true))
  (prop + name (? String) (= "n"))
  (prop + items _ (= (call (of List String) (list "a"))))
  (content
    (<> This
      (if flag
        (<> TextBox (value: (str "<" (. later label))))
        (<> Pinger (id: later)
          (= label "if")
          (for (on event pinged)
            (+= (. later log) (str "ping " (. later label)))
          )
        )
        (<> TextBox (value: (str (. later label) ">")))
      )
      (if #flag
        (<> TextBox (value: (str "<" (. later label))))
        (<> Pinger (id: later)
          (= label "live")
          (for (on event pinged)
            (+= (. later log) (str "ping " (. later label)))
          )
        )
        (<> TextBox (value: (str (. later label) ">")))
      )
      (if (not flag)
        (<> Box)
        (else
          (<> TextBox (value: (str "<" (. later label))))
          (<> Pinger (id: later)
            (= label "else")
            (for (on event pinged)
              (+= (. later log) (str "ping " (. later label)))
            )
          )
          (<> TextBox (value: (str (. later label) ">")))
        )
      )
      (match name
        (case (: text String)
          (<> TextBox (value: (str "<" (. later label))))
          (<> Pinger (id: later)
            (= label (str "static " text))
            (for (on event pinged)
              (+= (. later log) (str "ping " (. later label)))
            )
          )
          (<> TextBox (value: (str (. later label) ">")))
        )
        (case _
          (<> Box)
        )
      )
      (match #name
        (case "x"
          (<> Box)
        )
        (case (: text String)
          (<> TextBox (value: (str "<" (. later label))))
          (<> Pinger (id: later)
            (= label "match")
            (for (on event pinged)
              (+= (. later log) (str "ping " (. later label)))
            )
          )
          (<> TextBox (value: (str (. later label) ">")))
        )
      )
      (for (in item #items (keyed-by item))
        (if (!= item "")
          (<> TextBox (value: (str "<" (. later label))))
          (<> Pinger (id: later)
            (= label (str "row " item))
            (for (on event pinged)
              (+= (. later log) (str "ping " (. later label)))
            )
          )
          (<> TextBox (value: (str (. later label) ">")))
        )
      )
      (<> Pinger (id: handled)
        (= label "handler")
        (for (on event pinged)
          (<> TextBox (id: made) (value: (str "made " (. handled label))))
          (+= (. handled log) (. made text))
        )
      )
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

(fn-do + pings String
  (param box Box)
  (let heard (call (of List String)))
  (call collect box heard)
  (return (call (. "," join) heard))
)

(fn-do + collect _
  (param box Box)
  (param heard (list String))
  (for (in child (. box children))
    (if (is child Pinger)
      (let pinger (as child Pinger))
      (<- (. pinger pinged) 1)
      (for (in line (. pinger log))
        (+= heard line)
      )
      (= (. pinger log) (call (of List String)))
    )
    (call collect child heard)
  )
)

(type + Pinger (is Box)
  (prop + label _ (= ""))
  (prop + log _ (= (call (of List String))))
  (event + pinged Int)
)
```

```output
<if,[],if>,[<live,[],live>],<else,[],else>,<static n,[],static n>,[<match,[],match>],[[[<row a,[],row a>]]],[]
ping if,ping live,ping else,ping static n,ping match,ping row a,made handler
<if,[],if>,[<live,[],live>],<else,[],else>,<static n,[],static n>,[<match,[],match>],[[[<row a,[],row a>]],[[<row b,[],row b>]]],[]
ping if,ping live,ping else,ping static n,ping match,ping row a,ping row b,made handler
```

### So does each in the markup a function writes.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (call run)
)

(fn-do + run _
  (let model (call Model))
  (let flag true)
  (let word (? String) "n")
  (<> Box (id: root)
    (if flag
      (<> TextBox (value: (str "<" (. later label))))
      (<> Pinger (id: later)
        (= label "if")
        (for (on event pinged)
          (+= (. later log) (str "ping " (. later label)))
        )
      )
      (<> TextBox (value: (str (. later label) ">")))
    )
    (if (. model #flag)
      (<> TextBox (value: (str "<" (. later label))))
      (<> Pinger (id: later)
        (= label "live")
        (for (on event pinged)
          (+= (. later log) (str "ping " (. later label)))
        )
      )
      (<> TextBox (value: (str (. later label) ">")))
    )
    (if (not flag)
      (<> Box)
      (else
        (<> TextBox (value: (str "<" (. later label))))
        (<> Pinger (id: later)
          (= label "else")
          (for (on event pinged)
            (+= (. later log) (str "ping " (. later label)))
          )
        )
        (<> TextBox (value: (str (. later label) ">")))
      )
    )
    (match word
      (case (: text String)
        (<> TextBox (value: (str "<" (. later label))))
        (<> Pinger (id: later)
          (= label (str "static " text))
          (for (on event pinged)
            (+= (. later log) (str "ping " (. later label)))
          )
        )
        (<> TextBox (value: (str (. later label) ">")))
      )
      (case _
        (<> Box)
      )
    )
    (match (. model #name)
      (case "x"
        (<> Box)
      )
      (case (: text String)
        (<> TextBox (value: (str "<" (. later label))))
        (<> Pinger (id: later)
          (= label "match")
          (for (on event pinged)
            (+= (. later log) (str "ping " (. later label)))
          )
        )
        (<> TextBox (value: (str (. later label) ">")))
      )
    )
    (for (in item (. model #items) (keyed-by item))
      (if (!= item "")
        (<> TextBox (value: (str "<" (. later label))))
        (<> Pinger (id: later)
          (= label (str "row " item))
          (for (on event pinged)
            (+= (. later log) (str "ping " (. later label)))
          )
        )
        (<> TextBox (value: (str (. later label) ">")))
      )
    )
    (<> Pinger (id: handled)
      (= label "handler")
      (for (on event pinged)
        (<> TextBox (id: made) (value: (str "made " (. handled label))))
        (+= (. handled log) (. made text))
      )
    )
  )
  (print (call dump root))
  (print (call pings root))
  (= (. model flag) false)
  (= (. model flag) true)
  (= (. model name) null)
  (= (. model name) "n")
  (+= (. model items) "b")
  (print (call dump root))
  (print (call pings root))
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

(fn-do + pings String
  (param box Box)
  (let heard (call (of List String)))
  (call collect box heard)
  (return (call (. "," join) heard))
)

(fn-do + collect _
  (param box Box)
  (param heard (list String))
  (for (in child (. box children))
    (if (is child Pinger)
      (let pinger (as child Pinger))
      (<- (. pinger pinged) 1)
      (for (in line (. pinger log))
        (+= heard line)
      )
      (= (. pinger log) (call (of List String)))
    )
    (call collect child heard)
  )
)

(type + Pinger (is Box)
  (prop + label _ (= ""))
  (prop + log _ (= (call (of List String))))
  (event + pinged Int)
)

(type + Model
  (prop + flag _ (= true))
  (prop + name (? String) (= "n"))
  (prop + items _ (= (call (of List String) (list "a"))))
)
```

```output
<if,[],if>,[<live,[],live>],<else,[],else>,<static n,[],static n>,[<match,[],match>],[[[<row a,[],row a>]]],[]
ping if,ping live,ping else,ping static n,ping match,ping row a,made handler
<if,[],if>,[<live,[],live>],<else,[],else>,<static n,[],static n>,[<match,[],match>],[[[<row a,[],row a>]],[[<row b,[],row b>]]],[]
ping if,ping live,ping else,ping static n,ping match,ping row a,ping row b,made handler
```

### A named element in an arm of an if or a match in a class's content, the if following state or not, is given to the class's property of its name, which a method and the class's users read.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let demo (call Demo))
  (print (call (. demo told)))
  (print (?? (?. (. demo shown) label) "none"))
  (print (?? (?. (. demo tracked) label) "none"))
  (print (?? (?. (. demo matched) label) "none"))
)

(type + Pinger (is Box)
  (prop + label _ (= ""))
)

(type + Demo (is Box)
  (prop + flag _ (= true))
  (prop + name (? String) (= "n"))
  (prop + shown (? Pinger) (= null))
  (prop + tracked (? Pinger) (= null))
  (prop + matched (? Pinger) (= null))
  (content
    (<> This
      (if flag
        (<> Pinger (id: shown)
          (= label "static")
        )
      )
      (if #flag
        (<> Pinger (id: tracked)
          (= label "live")
        )
      )
      (match name
        (case (: text String)
          (<> Pinger (id: matched)
            (= label (str "match " text))
          )
        )
        (case _
          (<> Box)
        )
      )
    )
  )
  (fn-do + told String
    (return (str (?? (?. shown label) "none") " " (?? (?. tracked label) "none") " " (?? (?. matched label) "none")))
  )
)
```

```output
static live match n
static
live
match n
```

### A named element in an arm of an if in a class's content that the class does not declare is the class's private property, which a method and a row after the if read.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let demo (call Demo))
  (print (call (. demo told)))
  (print (call dump demo))
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

(type + Demo (is Box)
  (prop + flag _ (= true))
  (content
    (<> This
      (if flag
        (<> TextBox (id: shown) (value: "static"))
      )
      (<> TextBox (value: (str "after " (. shown text))))
    )
  )
  (fn-do + told String
    (return (. shown text))
  )
)
```

```output
static
static,after static
```

### A name an arm or a loop body gives is not read outside it.

```brix compile-error
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let model (call Model))
  (let flag true)
  (let items (call (of List String) (list "a")))
  (blank)
  (<> Box (id: root)
    (if flag
      (<> TextBox (id: inner) (value: "i"))
    )
  )
  (print (. inner text))
)

(type + Model
  (prop + flag _ (= true))
  (prop + name (? String) (= "n"))
  (prop + items _ (= (call (of List String) (list "a"))))
)
```

```output
Exception: Name "inner" not found
<no file:15>
      (print (. inner text))
                ^^^^^
```

```brix compile-error
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let model (call Model))
  (let flag true)
  (let items (call (of List String) (list "a")))
  (blank)
  (<> Box (id: root)
    (match (. model #name)
      (case (: text String)
        (<> TextBox (id: inner) (value: "i"))
      )
      (case _
        (<> Box)
      )
    )
  )
  (print (. inner text))
)

(type + Model
  (prop + flag _ (= true))
  (prop + name (? String) (= "n"))
  (prop + items _ (= (call (of List String) (list "a"))))
)
```

```output
Exception: Name "inner" not found
<no file:20>
      (print (. inner text))
                ^^^^^
```

```brix compile-error
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let model (call Model))
  (let flag true)
  (let items (call (of List String) (list "a")))
  (blank)
  (<> Box (id: root)
    (for (in item items)
      (<> TextBox (id: inner) (value: "i"))
    )
  )
  (print (. inner text))
)

(type + Model
  (prop + flag _ (= true))
  (prop + name (? String) (= "n"))
  (prop + items _ (= (call (of List String) (list "a"))))
)
```

```output
Exception: Name "inner" not found
<no file:15>
      (print (. inner text))
                ^^^^^
```

### An element an arm of an if in content names is made where the arm starts, so what makes it does not read a local of the arm.

```brix compile-error
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let flag true)
  (<> Box (id: root)
    (if flag
      (let word "w")
      (<> TextBox (id: inner) (value: word))
    )
  )
)
```

```output
Exception: Name "word" not found
<no file:10>
          (<> TextBox (id: inner) (value: word))
                                          ^^^^
```

### The arm a match written as a line of markup content takes runs as content, as an arm of an if does: each of its rows gives the element a child, and a match? that takes no arm gives none. A match in a `do` block runs as steps, so it makes the elements of its arm without adding them.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let name (? String) "n")
  (let none (? String) null)
  (blank)
  (<> Box (id: root)
    (match name
      (case (: text String)
        (<> TextBox (value: (str "a " text)))
        (<> TextBox (value: "b"))
      )
      (case _
        (<> Box)
      )
    )
    (match? none
      (case (: text String)
        (<> TextBox (value: text))
      )
    )
    (do
      (match name
        (case (: text String)
          (<> TextBox (value: "made"))
        )
        (case _
          (<> Box)
        )
      )
    )
    (match none
      (case (: text String)
        (<> TextBox (value: text))
      )
      (case _
        (<> TextBox (value: "c"))
      )
    )
  )
  (blank)
  (for (in child (. root children))
    (print (. (as child TextBox) text))
  )
)
```

```output
a n
b
c
```

### A name an arm of a match in content gives is not read outside it.

```brix compile-error
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let name (? String) "n")
  (blank)
  (<> Box (id: root)
    (match name
      (case (: text String)
        (<> TextBox (id: inner) (value: text))
      )
      (case _
        (<> Box)
      )
    )
  )
  (print (. inner text))
)
```

```output
Exception: Name "inner" not found
<no file:18>
      (print (. inner text))
                ^^^^^
```

### An element an arm of a match in content names is made where the arm starts, so what makes it reads the names the arm's pattern binds but not a local of the arm.

```brix compile-error
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let name (? String) "n")
  (<> Box (id: root)
    (match name
      (case (: text String)
        (let word text)
        (<> TextBox (id: inner) (value: word))
      )
      (case _
        (<> Box)
      )
    )
  )
)
```

```output
Exception: Name "word" not found
<no file:11>
            (<> TextBox (id: inner) (value: word))
                                            ^^^^
```

### A table of conditions written as a line of markup content runs the row it takes as content, and so does a match with an arm that raises, where the arm taken does not raise.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let flag true)
  (let name (? String) "n")
  (blank)
  (<> Box (id: root)
    (case? (not flag)
      (<> Box)
    )
    (default
      (<> TextBox (value: "a"))
      (<> TextBox (value: "b"))
    )
    (match name
      (case (: text String)
        (<> TextBox (value: (str "c " text)))
        (<> TextBox (value: "d"))
      )
      (throw (call ValueMissing))
    )
  )
  (blank)
  (for (in child (. root children))
    (print (. (as child TextBox) text))
  )
)

(type + ValueMissing (is Error))
```

```output
a
b
c n
d
```
