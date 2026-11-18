# Match exhaustiveness

```yumi
. result = match true
    |= true
        1
    |= false
        0
-- type-of result
-- result + 1
```

```output
[TypeInfo I64]
2
```

```yumi crash
do match true
    |= true
        -- "true"
```

```output
Exception: This match on Bool does not handle false; add "|= false", or "??" for the rest
<no file:3>
        |= true
        ^^^^^^
```

### Held Results require every Error channel.

```yumi crash
. result = match catch load(0)
    | config Config
        config.value
    | err FileNotFound
        "not found"
-- result

━━━━━━━━━━━━━━━━━━━━
+ Config

+ value String

────────────────────
+ This
    - value

━━━━━━━━━━━━━━━━━━━━
+ FileNotFound is Error

━━━━━━━━━━━━━━━━━━━━
+ PermissionDenied is Error

━━━━━━━━━━━━━━━━━━━━
+ do load Config ?! FileNotFound ?! PermissionDenied
    - outcome Int

if outcome == 1
    ?! FileNotFound("missing")
if outcome == 2
    ?! PermissionDenied("denied")
= Config("loaded")
```

```output
Exception: This match on Result does not handle PermissionDenied; add a case for it, or "??" for the rest
<no file:3>
        | config Config
        ^^^^^^^^^^^^^^
```

### Held Results require their success channel.

```yumi crash
. result = match catch load(1)
    | err FileNotFound
        "not found"
    | err PermissionDenied
        "denied"
-- result

━━━━━━━━━━━━━━━━━━━━
+ Config

+ value String

────────────────────
+ This
    - value

━━━━━━━━━━━━━━━━━━━━
+ FileNotFound is Error

━━━━━━━━━━━━━━━━━━━━
+ PermissionDenied is Error

━━━━━━━━━━━━━━━━━━━━
+ do load Config ?! FileNotFound ?! PermissionDenied
    - outcome Int

if outcome == 1
    ?! FileNotFound("missing")
if outcome == 2
    ?! PermissionDenied("denied")
= Config("loaded")
```

```output
Exception: This match on Result does not handle Config; add a case for it, or "??" for the rest
<no file:3>
        | err FileNotFound
        ^^^^^^^^^^^^^^^^^
```

### Exhaustive held Results evaluate only the selected success or Error arm.

```yumi
-- inspect(0)
-- inspect(2)

━━━━━━━━━━━━━━━━━━━━
+ Config

+ value String

────────────────────
+ This
    - value

━━━━━━━━━━━━━━━━━━━━
+ FileNotFound is Error

━━━━━━━━━━━━━━━━━━━━
+ PermissionDenied is Error

━━━━━━━━━━━━━━━━━━━━
+ do load Config ?! FileNotFound ?! PermissionDenied
    - outcome Int

if outcome == 1
    ?! FileNotFound("missing")
if outcome == 2
    ?! PermissionDenied("denied")
= Config("loaded")

━━━━━━━━━━━━━━━━━━━━
+ inspect String
    - outcome Int

match catch load(outcome)
    | config Config
        config.value
    | err FileNotFound
        "not found"
    | err PermissionDenied
        "permission denied"
```

```output
loaded
permission denied
```

### A typed arm that propagates its Error still covers that Result channel.

```yumi
-- catch inspect(1)

━━━━━━━━━━━━━━━━━━━━
+ Config

+ value String

────────────────────
+ This
    - value

━━━━━━━━━━━━━━━━━━━━
+ FileNotFound is Error

━━━━━━━━━━━━━━━━━━━━
+ PermissionDenied is Error

━━━━━━━━━━━━━━━━━━━━
+ do load Config ?! FileNotFound ?! PermissionDenied
    - outcome Int

if outcome == 1
    ?! FileNotFound("missing")
if outcome == 2
    ?! PermissionDenied("denied")
= Config("loaded")

━━━━━━━━━━━━━━━━━━━━
+ do inspect String ?! FileNotFound
    - outcome Int

= match catch load(outcome)
    | config Config
        config.value
    | err FileNotFound
        ?! err
    | err PermissionDenied
        "permission denied"
```

```output
missing
```

### Predicate-only arms still check the held Result domain.

```yumi crash
. result = match catch load()
    |? false
        "never"
-- result

━━━━━━━━━━━━━━━━━━━━
+ Config

━━━━━━━━━━━━━━━━━━━━
+ FileNotFound is Error

━━━━━━━━━━━━━━━━━━━━
+ PermissionDenied is Error

━━━━━━━━━━━━━━━━━━━━
+ do load Config ?! FileNotFound ?! PermissionDenied

= Config()
```

```output
Exception: This match on Result does not handle Config, FileNotFound and PermissionDenied; add a case for each, or "??" for the rest
<no file:3>
        |? false
        ^^^^^^^
```

### Error supertypes, unconditional remainders, and optional matches retain their coverage rules.

```yumi
-- inspect-with-supertype(2)
-- inspect-with-remainder(1)
. partial = match? catch load(2)
    | config Config
        config.value
-- partial ?? "none"

━━━━━━━━━━━━━━━━━━━━
+ Config

+ value String

────────────────────
+ This
    - value

━━━━━━━━━━━━━━━━━━━━
+ FileNotFound is Error

━━━━━━━━━━━━━━━━━━━━
+ PermissionDenied is Error

━━━━━━━━━━━━━━━━━━━━
+ do load Config ?! FileNotFound ?! PermissionDenied
    - outcome Int

if outcome == 1
    ?! FileNotFound("missing")
if outcome == 2
    ?! PermissionDenied("denied")
= Config("loaded")

━━━━━━━━━━━━━━━━━━━━
+ inspect-with-supertype String
    - outcome Int

match catch load(outcome)
    | config Config
        config.value
    | Error
        "error"

━━━━━━━━━━━━━━━━━━━━
+ inspect-with-remainder String
    - outcome Int

match catch load(outcome)
    | config Config
        config.value
    ??
        "remainder"
```

```output
error
remainder
none
```

### A Result success union remains one channel, but its alternatives can be covered together.

```yumi
-- inspect-choice(false)
-- inspect-choice(true)

━━━━━━━━━━━━━━━━━━━━
+ Config

+ value String

────────────────────
+ This
    - value

━━━━━━━━━━━━━━━━━━━━
+ Choice is Union

| Config
| String

━━━━━━━━━━━━━━━━━━━━
+ PermissionDenied is Error

━━━━━━━━━━━━━━━━━━━━
+ do load-choice Choice ?! PermissionDenied
    - as-text Bool

if as-text
    = "text"
= Config("config")

━━━━━━━━━━━━━━━━━━━━
+ inspect-choice String
    - as-text Bool

match catch load-choice(as-text)
    | config Config
        config.value
    | text String
        text
    | err PermissionDenied
        "denied"
```

```output
config
text
```

### One alternative does not cover an entire union-valued success channel.

```yumi crash
. result = match catch load-choice()
    | config Config
        config.value
    | err PermissionDenied
        "denied"
-- result

━━━━━━━━━━━━━━━━━━━━
+ Config

+ value String

────────────────────
+ This
    - value

━━━━━━━━━━━━━━━━━━━━
+ Choice is Union

| Config
| String

━━━━━━━━━━━━━━━━━━━━
+ PermissionDenied is Error

━━━━━━━━━━━━━━━━━━━━
+ do load-choice Choice ?! PermissionDenied

= Config("config")
```

```output
Exception: This match on Result does not handle Choice; add a case for it, or "??" for the rest
<no file:3>
        | config Config
        ^^^^^^^^^^^^^^
```

```yumi crash
. result = match true
    |= true
        1
-- result
```

```output
Exception: This match on Bool does not handle false; add "|= false", or "??" for the rest
<no file:3>
        |= true
        ^^^^^^
```

```yumi
. value Int? = 41
. result = match value
    | n Int
        n + 1
    |= null
        0
-- type-of result
-- result + 1
```

```output
[TypeInfo I64]
43
```

```yumi
. value Int?
. result = match value
    | n Int
        n + 1
    |= null
        0
-- result
```

```output
0
```

```yumi crash
. value Int? = 41
. result = match value
    | n Int
        n + 1
-- result
```

```output
Exception: This match on I64? does not handle null; add "|= null", or "??" for the rest
<no file:4>
        | n Int
        ^^^^^^
```

```yumi crash
. value Int?
. result = match value
    |= null
        0
-- result
```

```output
Exception: This match on I64? handles only null; add a case for a present value, as in "| value I64", or "??" for the rest
<no file:4>
        |= null
        ^^^^^^
```

```yumi
. value = Result.good(41)
. result = match value
    | .good(n)
        n + 1
    | .bad
        0
-- type-of result
-- result + 1

━━━━━━━━━━━━━━━━━━━━
+ Result is Union

| good(n Int)
| bad
```

```output
[TypeInfo I64]
43
```

```yumi crash
. value = Result.good(41)
. result = match value
    | .good(n)
        n + 1
-- result

━━━━━━━━━━━━━━━━━━━━
+ Result is Union

| good(n Int)
| bad
```

```output
Exception: This match on Result does not handle .bad; add a case for it, or "??" for the rest
<no file:4>
        | .good(n)
        ^^^^^^^^^
```

```yumi
. values = List
    * Result.good(1)
    * Result.bad
    ?*> match? each
        | .good(n)
            n
-- values

━━━━━━━━━━━━━━━━━━━━
+ Result is Union

| good(n Int)
| bad
```

```output
1
```

```yumi
. event = match true
    |= true
        ..ok(41)
    ??
        ..failed
. result = match event
    | .ok(n)
        n + 1
    | .failed
        0
-- type-of result
-- result + 1
```

```output
[TypeInfo I64]
43
```

```yumi
. value Choice = 41
. result = match value
    | Int
        1
    | String
        2
-- type-of result
-- result + 1

━━━━━━━━━━━━━━━━━━━━
+ Choice is Union

| Int
| String
```

```output
[TypeInfo I64]
2
```

```yumi
. value Mode = Mode.beta
. result = match value
    |= Mode.alpha
        1
    |= Mode.beta
        2
-- type-of result
-- result + 1

━━━━━━━━━━━━━━━━━━━━
+ Mode is Int

| alpha
| beta
```

```output
[TypeInfo I64]
3
```

```yumi
. value = "x"
. result = match value
    |= "x"
        1
-- type-of result
-- result + 1
```

```output
[TypeInfo I64]
2
```

```yumi
. value = "x"
. result = match? value
    |= "x"
        1
-- type-of result
-- result ?? 0
```

```output
[TypeInfo Optional'I64]
1
```

```yumi
. result = match? true
    |= true
        1
-- type-of result
-- result ?? 0
```

```output
[TypeInfo Optional'I64]
1
```

```yumi
. result = match true
    |= true
        1
    ??
        0
-- type-of result
-- result + 1
```

```output
[TypeInfo I64]
2
```

### List shape patterns nest inside union case patterns.

```yumi
-- describe(Tree.branch("let", [Tree.leaf("total"), Tree.leaf("Int")]))
-- describe(Tree.branch("set", [Tree.leaf("total")]))
-- describe(Tree.branch("run", List'Tree()))
-- describe(Tree.leaf("total"))

━━━━━━━━━━━━━━━━━━━━
+ Tree is Union

| leaf String
| branch(name String, items [Tree])

━━━━━━━━━━━━━━━━━━━━
+ describe String
    - tree Tree

match tree
    | .branch("let", [.leaf(name), ...rest])
        "let $name with ${rest.length} more"
    | .branch(name, [only])
        "$name with one item"
    | .branch(name, [])
        "$name alone"
    | .leaf(text)
        "leaf $text"
    ??
        "other"
```

```output
let total with 1 more
set with one item
run alone
leaf total
```

### A remainder-only list pattern covers its union case.

```yumi
-- count(Tree.branch("let", [Tree.leaf("total"), Tree.leaf("Int")]))
-- count(Tree.leaf("total"))

━━━━━━━━━━━━━━━━━━━━
+ Tree is Union

| leaf String
| branch(name String, items [Tree])

━━━━━━━━━━━━━━━━━━━━
+ count Size
    - tree Tree

match tree
    | .branch(_, [...items])
        items.length
    | .leaf(_)
        0
```

```output
2
0
```

### A list shape pattern with required items does not cover its union case.

```yumi crash
-- count(Tree.branch("let", [Tree.leaf("total")]))

━━━━━━━━━━━━━━━━━━━━
+ Tree is Union

| leaf String
| branch(name String, items [Tree])

━━━━━━━━━━━━━━━━━━━━
+ count Size
    - tree Tree

match tree
    | .branch(_, [first, ...])
        1
    | .leaf(_)
        0
```

```output
Exception: This match on Tree does not handle .branch; add a case for it, or "??" for the rest
<no file:15>
        | .branch(_, [first, ...])
        ^^^^^^^^^^^^^^^^^^^^^^^^^
```

### An interface pattern checks coverage of a Null case without compiling its converter.

```yumi
. text Body = "x"
-- bar-of(text)
. empty Body = null
-- bar-of(empty)
. other Body = Other()
-- bar-of(other)

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ get bar Int

━━━━━━━━━━━━━━━━━━━━
+ Other
    is Proto

────────────────────
+ get bar

7

━━━━━━━━━━━━━━━━━━━━
+ Body is Union

| Null
| String
| Proto

━━━━━━━━━━━━━━━━━━━━
+ bar-of Int
    - body Body

match body
    | proto Proto
        proto.bar
    ??
        0
```

```output
0
0
7
```

### A type written in brackets in an arm of a match on a result covers the list the result holds.

```yumi
-- chosen(2)
-- chosen(0)

━━━━━━━━━━━━━━━━━━━━
+ do chosen String
    - id U64

. result = catch copies-of(id)
do match result
    | error Error
        = "error"
    | found [U64]
        = "found ${found.length}"

━━━━━━━━━━━━━━━━━━━━
+ do copies-of [U64] ?! Error
    - id U64

?! Error("none")
    if id == 0
= List'U64(id, id, id)
```

```output
found 3
error
```

### Arms of a case that holds a list take the case together when one takes some count of items and a rest and others take each smaller count.

```yumi
. empty [Int]
-- count(.many(empty))
-- count(.many([5, 6]))
-- count(.none)

━━━━━━━━━━━━━━━━━━━━
+ count Int
    - bag Bag

match bag
    | .none
        0
    | .many([])
        -1
    | .many([first, ...rest])
        first + rest.length

━━━━━━━━━━━━━━━━━━━━
+ Bag is Union

| none
| many [Int]
```

```output
-1
6
0
```

### Arms of a list that leave out a count of items do not take the case.

```yumi crash
-- count(.many([1]))

━━━━━━━━━━━━━━━━━━━━
+ count Int
    - bag Bag

match bag
    | .none
        0
    | .many([])
        -1
    | .many([first, second, ...rest])
        first

━━━━━━━━━━━━━━━━━━━━
+ Bag is Union

| none
| many [Int]
```

```output
Exception: This match on Bag does not handle .many; add a case for it, or "??" for the rest
<no file:9>
        | .none
        ^^^^^^
```

### `true` and `false` in a pattern are literals, so they match an equal value, as `|= true` does.

```yumi
. flag = false
. result = match flag
    | true
        "yes"
    | false
        "no"
-- result
-- classify(false, true)
-- classify(true, false)
-- classify(true, true)
-- named(3)

━━━━━━━━━━━━━━━━━━━━
_ classify String
    - valid Bool
    - many Bool

match valid, many
    | false, _
        "invalid"
    | _, false
        "few"
    ??
        "many"

━━━━━━━━━━━━━━━━━━━━
_ named String
    - true-count Int

match true-count
    | falsey
        "n=$falsey"
```

```output
no
invalid
few
many
n=3
```

```yumi crash
do match true
    | true
        -- "true"
```

```output
Exception: This match on Bool does not handle false; add "|= false", or "??" for the rest
<no file:3>
        | true
        ^^^^^
```
