# Markup names

### Each arm and loop body written as a line of markup content names the elements written in it, and makes them where it starts each time it runs, so its rows read the names, above the element too, and the element's handlers read them; a handler's body makes its own where they are written.

```yumi
/core/tests/support/markup-fixtures

. model = Model()
. flag = true
. word String? = "n"

<Box root
    if flag
        <TextBox = "<${later.label}"
        <Pinger later
            label = "if"
            for event on pinged
                later.log += "ping ${later.label}"
        <TextBox = "${later.label}>"
    if model.#flag
        <TextBox = "<${later.label}"
        <Pinger later
            label = "live"
            for event on pinged
                later.log += "ping ${later.label}"
        <TextBox = "${later.label}>"
    if !flag
        <Box
    else
        <TextBox = "<${later.label}"
        <Pinger later
            label = "else"
            for event on pinged
                later.log += "ping ${later.label}"
        <TextBox = "${later.label}>"
    match word
        | text String
            <TextBox = "<${later.label}"
            <Pinger later
                label = "static ${text}"
                for event on pinged
                    later.log += "ping ${later.label}"
            <TextBox = "${later.label}>"
        | _
            <Box
    match model.#name
        | "x"
            <Box
        | text String
            <TextBox = "<${later.label}"
            <Pinger later
                label = "match"
                for event on pinged
                    later.log += "ping ${later.label}"
            <TextBox = "${later.label}>"
    for item in model.#items keyed by item
        if item != ""
            <TextBox = "<${later.label}"
            <Pinger later
                label = "row ${item}"
                for event on pinged
                    later.log += "ping ${later.label}"
            <TextBox = "${later.label}>"
    <Pinger handled
        label = "handler"
        for event on pinged
            <TextBox made = "made ${handled.label}"
            handled.log += made.text

-- dump(root)
-- pings(root)
model.flag = false
model.flag = true
model.name = null
model.name = "n"
model.items += "b"
-- dump(root)
-- pings(root)

━━━━━━━━━━━━━━━━━━━━
+ do dump String
    - box Box

. parts = List'String()
for child in box.children
    if child is TextBox
        parts += (child as TextBox).text
    else
        parts += "[${dump(child)}]"
= ",".join(parts)

━━━━━━━━━━━━━━━━━━━━
+ do pings String
    - box Box

. heard = List'String()
collect(box, heard)
= ",".join(heard)

━━━━━━━━━━━━━━━━━━━━
+ do collect
    - box Box
    - heard [String]

for child in box.children
    if child is Pinger
        . pinger = child as Pinger
        pinger.pinged <- 1
        for line in pinger.log
            heard += line
        pinger.log = List'String()
    collect(child, heard)

━━━━━━━━━━━━━━━━━━━━
+ Pinger is Box

+ label = ""
+ log = List'String()
+ event pinged Int

━━━━━━━━━━━━━━━━━━━━
+ Model

+ flag = true
+ name String? = "n"
+ items = List'String(["a"])
```

```output
<if,[],if>,[<live,[],live>],<else,[],else>,<static n,[],static n>,[<match,[],match>],[[[<row a,[],row a>]]],[]
ping if,ping live,ping else,ping static n,ping match,ping row a,made handler
<if,[],if>,[<live,[],live>],<else,[],else>,<static n,[],static n>,[<match,[],match>],[[[<row a,[],row a>]],[[<row b,[],row b>]]],[]
ping if,ping live,ping else,ping static n,ping match,ping row a,ping row b,made handler
```

### So does each in a class's content, whose rows read the class's members by name.

```yumi
/core/tests/support/markup-fixtures

. demo = Demo()
-- dump(demo)
-- pings(demo)
demo.flag = false
demo.flag = true
demo.name = null
demo.name = "n"
demo.items += "b"
-- dump(demo)
-- pings(demo)

━━━━━━━━━━━━━━━━━━━━
+ Demo is Box

+ flag = true
+ name String? = "n"
+ items = List'String(["a"])

────────────────────

<This
    if flag
        <TextBox = "<${later.label}"
        <Pinger later
            label = "if"
            for event on pinged
                later.log += "ping ${later.label}"
        <TextBox = "${later.label}>"
    if #flag
        <TextBox = "<${later.label}"
        <Pinger later
            label = "live"
            for event on pinged
                later.log += "ping ${later.label}"
        <TextBox = "${later.label}>"
    if !flag
        <Box
    else
        <TextBox = "<${later.label}"
        <Pinger later
            label = "else"
            for event on pinged
                later.log += "ping ${later.label}"
        <TextBox = "${later.label}>"
    match name
        | text String
            <TextBox = "<${later.label}"
            <Pinger later
                label = "static ${text}"
                for event on pinged
                    later.log += "ping ${later.label}"
            <TextBox = "${later.label}>"
        | _
            <Box
    match #name
        | "x"
            <Box
        | text String
            <TextBox = "<${later.label}"
            <Pinger later
                label = "match"
                for event on pinged
                    later.log += "ping ${later.label}"
            <TextBox = "${later.label}>"
    for item in #items keyed by item
        if item != ""
            <TextBox = "<${later.label}"
            <Pinger later
                label = "row ${item}"
                for event on pinged
                    later.log += "ping ${later.label}"
            <TextBox = "${later.label}>"
    <Pinger handled
        label = "handler"
        for event on pinged
            <TextBox made = "made ${handled.label}"
            handled.log += made.text

━━━━━━━━━━━━━━━━━━━━
+ do dump String
    - box Box

. parts = List'String()
for child in box.children
    if child is TextBox
        parts += (child as TextBox).text
    else
        parts += "[${dump(child)}]"
= ",".join(parts)

━━━━━━━━━━━━━━━━━━━━
+ do pings String
    - box Box

. heard = List'String()
collect(box, heard)
= ",".join(heard)

━━━━━━━━━━━━━━━━━━━━
+ do collect
    - box Box
    - heard [String]

for child in box.children
    if child is Pinger
        . pinger = child as Pinger
        pinger.pinged <- 1
        for line in pinger.log
            heard += line
        pinger.log = List'String()
    collect(child, heard)

━━━━━━━━━━━━━━━━━━━━
+ Pinger is Box

+ label = ""
+ log = List'String()
+ event pinged Int
```

```output
<if,[],if>,[<live,[],live>],<else,[],else>,<static n,[],static n>,[<match,[],match>],[[[<row a,[],row a>]]],[]
ping if,ping live,ping else,ping static n,ping match,ping row a,made handler
<if,[],if>,[<live,[],live>],<else,[],else>,<static n,[],static n>,[<match,[],match>],[[[<row a,[],row a>]],[[<row b,[],row b>]]],[]
ping if,ping live,ping else,ping static n,ping match,ping row a,ping row b,made handler
```

### So does each in the markup a function writes.

```yumi
/core/tests/support/markup-fixtures

run()

━━━━━━━━━━━━━━━━━━━━
+ do run

. model = Model()
. flag = true
. word String? = "n"
<Box root
    if flag
        <TextBox = "<${later.label}"
        <Pinger later
            label = "if"
            for event on pinged
                later.log += "ping ${later.label}"
        <TextBox = "${later.label}>"
    if model.#flag
        <TextBox = "<${later.label}"
        <Pinger later
            label = "live"
            for event on pinged
                later.log += "ping ${later.label}"
        <TextBox = "${later.label}>"
    if !flag
        <Box
    else
        <TextBox = "<${later.label}"
        <Pinger later
            label = "else"
            for event on pinged
                later.log += "ping ${later.label}"
        <TextBox = "${later.label}>"
    match word
        | text String
            <TextBox = "<${later.label}"
            <Pinger later
                label = "static ${text}"
                for event on pinged
                    later.log += "ping ${later.label}"
            <TextBox = "${later.label}>"
        | _
            <Box
    match model.#name
        | "x"
            <Box
        | text String
            <TextBox = "<${later.label}"
            <Pinger later
                label = "match"
                for event on pinged
                    later.log += "ping ${later.label}"
            <TextBox = "${later.label}>"
    for item in model.#items keyed by item
        if item != ""
            <TextBox = "<${later.label}"
            <Pinger later
                label = "row ${item}"
                for event on pinged
                    later.log += "ping ${later.label}"
            <TextBox = "${later.label}>"
    <Pinger handled
        label = "handler"
        for event on pinged
            <TextBox made = "made ${handled.label}"
            handled.log += made.text
-- dump(root)
-- pings(root)
model.flag = false
model.flag = true
model.name = null
model.name = "n"
model.items += "b"
-- dump(root)
-- pings(root)

━━━━━━━━━━━━━━━━━━━━
+ do dump String
    - box Box

. parts = List'String()
for child in box.children
    if child is TextBox
        parts += (child as TextBox).text
    else
        parts += "[${dump(child)}]"
= ",".join(parts)

━━━━━━━━━━━━━━━━━━━━
+ do pings String
    - box Box

. heard = List'String()
collect(box, heard)
= ",".join(heard)

━━━━━━━━━━━━━━━━━━━━
+ do collect
    - box Box
    - heard [String]

for child in box.children
    if child is Pinger
        . pinger = child as Pinger
        pinger.pinged <- 1
        for line in pinger.log
            heard += line
        pinger.log = List'String()
    collect(child, heard)

━━━━━━━━━━━━━━━━━━━━
+ Pinger is Box

+ label = ""
+ log = List'String()
+ event pinged Int

━━━━━━━━━━━━━━━━━━━━
+ Model

+ flag = true
+ name String? = "n"
+ items = List'String(["a"])
```

```output
<if,[],if>,[<live,[],live>],<else,[],else>,<static n,[],static n>,[<match,[],match>],[[[<row a,[],row a>]]],[]
ping if,ping live,ping else,ping static n,ping match,ping row a,made handler
<if,[],if>,[<live,[],live>],<else,[],else>,<static n,[],static n>,[<match,[],match>],[[[<row a,[],row a>]],[[<row b,[],row b>]]],[]
ping if,ping live,ping else,ping static n,ping match,ping row a,ping row b,made handler
```

### A named element in an arm of an if or a match in a class's content, the if following state or not, is given to the class's property of its name, which a method and the class's users read.

```yumi
/core/tests/support/markup-fixtures

. demo = Demo()
-- demo.told()
-- demo.shown?.label ?? "none"
-- demo.tracked?.label ?? "none"
-- demo.matched?.label ?? "none"

━━━━━━━━━━━━━━━━━━━━
+ Pinger is Box

+ label = ""

━━━━━━━━━━━━━━━━━━━━
+ Demo is Box

+ flag = true
+ name String? = "n"
+ shown Pinger? = null
+ tracked Pinger? = null
+ matched Pinger? = null

────────────────────

<This
    if flag
        <Pinger shown
            label = "static"
    if #flag
        <Pinger tracked
            label = "live"
    match name
        | text String
            <Pinger matched
                label = "match ${text}"
        | _
            <Box

────────────────────
+ do told String

= "${shown?.label ?? "none"} ${tracked?.label ?? "none"} ${matched?.label ?? "none"}"
```

```output
static live match n
static
live
match n
```

### A named element in an arm of an if in a class's content that the class does not declare is the class's private property, which a method and a row after the if read.

```yumi
/core/tests/support/markup-fixtures

. demo = Demo()
-- demo.told()
-- dump(demo)

━━━━━━━━━━━━━━━━━━━━
+ do dump String
    - box Box

. parts = List'String()
for child in box.children
    if child is TextBox
        parts += (child as TextBox).text
    else
        parts += "[${dump(child)}]"
= ",".join(parts)

━━━━━━━━━━━━━━━━━━━━
+ Demo is Box

+ flag = true

────────────────────

<This
    if flag
        <TextBox shown = "static"
    <TextBox = "after ${shown.text}"

────────────────────
+ do told String

= shown.text
```

```output
static
static,after static
```

### A name an arm or a loop body gives is not read outside it.

```yumi compile-error
/core/tests/support/markup-fixtures

. model = Model()
. flag = true
. items = List'String(["a"])

<Box root
    if flag
        <TextBox inner = "i"
-- inner.text

━━━━━━━━━━━━━━━━━━━━
+ Model

+ flag = true
+ name String? = "n"
+ items = List'String(["a"])
```

```output
Exception: Name "inner" not found
<no file:11>
    -- inner.text
       ^^^^^
```

```yumi compile-error
/core/tests/support/markup-fixtures

. model = Model()
. flag = true
. items = List'String(["a"])

<Box root
    match model.#name
        | text String
            <TextBox inner = "i"
        | _
            <Box
-- inner.text

━━━━━━━━━━━━━━━━━━━━
+ Model

+ flag = true
+ name String? = "n"
+ items = List'String(["a"])
```

```output
Exception: Name "inner" not found
<no file:14>
    -- inner.text
       ^^^^^
```

```yumi compile-error
/core/tests/support/markup-fixtures

. model = Model()
. flag = true
. items = List'String(["a"])

<Box root
    for item in items
        <TextBox inner = "i"
-- inner.text

━━━━━━━━━━━━━━━━━━━━
+ Model

+ flag = true
+ name String? = "n"
+ items = List'String(["a"])
```

```output
Exception: Name "inner" not found
<no file:11>
    -- inner.text
       ^^^^^
```

### An element an arm of an if in content names is made where the arm starts, so what makes it does not read a local of the arm.

```yumi compile-error
/core/tests/support/markup-fixtures

. flag = true
<Box root
    if flag
        . word = "w"
        <TextBox inner = word
```

```output
Exception: Name "word" not found
<no file:8>
            <TextBox inner = word
                             ^^^^
```

### The arm a match written as a line of markup content takes runs as content, as an arm of an if does: each of its rows gives the element a child, and a match? that takes no arm gives none. A match run with do makes the elements of its arm without adding them.

```yumi
/core/tests/support/markup-fixtures

. name String? = "n"
. none String? = null

<Box root
    match name
        | text String
            <TextBox = "a ${text}"
            <TextBox = "b"
        | _
            <Box
    match? none
        | text String
            <TextBox = text
    do match name
        | text String
            <TextBox = "made"
        | _
            <Box
    match none
        | text String
            <TextBox = text
        | _
            <TextBox = "c"

for child in root.children
    -- (child as TextBox).text
```

```output
a n
b
c
```

### A name an arm of a match in content gives is not read outside it.

```yumi compile-error
/core/tests/support/markup-fixtures

. name String? = "n"

<Box root
    match name
        | text String
            <TextBox inner = text
        | _
            <Box
-- inner.text
```

```output
Exception: Name "inner" not found
<no file:12>
    -- inner.text
       ^^^^^
```

### An element an arm of a match in content names is made where the arm starts, so what makes it reads the names the arm's pattern binds but not a local of the arm.

```yumi compile-error
/core/tests/support/markup-fixtures

. name String? = "n"
<Box root
    match name
        | text String
            . word = text
            <TextBox inner = word
        | _
            <Box
```

```output
Exception: Name "word" not found
<no file:9>
                <TextBox inner = word
                                 ^^^^
```

### A table of conditions written as a line of markup content runs the row it takes as content, and so does a match with an arm that raises, where the arm taken does not raise.

```yumi
/core/tests/support/markup-fixtures

. flag = true
. name String? = "n"

<Box root
    |? !flag
        <Box
    ??
        <TextBox = "a"
        <TextBox = "b"
    match name
        | text String
            <TextBox = "c ${text}"
            <TextBox = "d"
        ?! ValueMissing()

for child in root.children
    -- (child as TextBox).text

━━━━━━━━━━━━━━━━━━━━
+ ValueMissing is Error
```

```output
a
b
c n
d
```
