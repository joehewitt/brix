# Live functions

### A live function keeps a state across the runs of the value that calls it. Two calls in one value keep a state each, in the order they are called, and a state that runs its action again runs the value again.

```yumi
/core/tracker

_ last-count Count?

. meter = Meter()
-- "${meter.shown} | ${meter.both}"
meter.value = 3
-- "${meter.shown} | ${meter.both}"
last-count?.poke()
-- "${meter.shown} | ${meter.both}"

━━━━━━━━━━━━━━━━━━━━
+ Meter

+ value = 1
+ shown = counted(#value, true)
+ both = "${counted(#value * 10, false)} and ${counted(#value * 100, false)}"

━━━━━━━━━━━━━━━━━━━━
+ do counted String @live
    - goal Int
    - remembers Bool

. found = live-state'Count()
. count = found ?? Count()
if !found
    keep-live-state(count)
    if remembers
        last-count = count
count.runs += 1
= "$goal run ${count.runs}"

━━━━━━━━━━━━━━━━━━━━
+ Count
    is Tracker

+ runs = 0
+ action LiveAction?

────────────────────
+ do poke

if action
    action()

────────────────────
+ do attach
    - action LiveAction

this.action = action

────────────────────
+ do detach
    - action LiveAction

this.action = null
```

```output
1 run 1 | 10 run 1 and 100 run 1
3 run 2 | 30 run 2 and 300 run 2
3 run 3 | 30 run 2 and 300 run 2
```

### A live function called by the value of a markup attribute keeps its state across the runs of the attribute.

```yumi
/core/tracker

<Foo a
    <Foo b
        name = "B"
    <Foo c
        name = tagged(b.#name)

-- c.name
b.name = "BB"
-- c.name

━━━━━━━━━━━━━━━━━━━━
+ Foo

+ name String
+ contents [Foo] = List'Foo()

────────────────────
+ do this += child Foo

contents += child

━━━━━━━━━━━━━━━━━━━━
+ do tagged String @live
    - text String

. found = live-state'Count()
. count = found ?? Count()
if !found
    keep-live-state(count)
count.runs += 1
= "$text ${count.runs}"

━━━━━━━━━━━━━━━━━━━━
+ Count
    is Tracker

+ runs = 0

────────────────────
+ do attach
    - action LiveAction

────────────────────
+ do detach
    - action LiveAction
```

```output
B 1
BB 2
```

### A live function is called only where a value is read live.

```yumi compile-error
-- twice(4)

━━━━━━━━━━━━━━━━━━━━
+ twice Int @live
    - value Int

value * 2
```

```output
Exception: The live function "twice" is called only where a value is read live
<no file:2>
    -- twice(4)
       ^^^^^^^^
```

### A live function that fails with an error leaves its live scope, so a value read after it is not read live.

```yumi
/core/tracker

. meter = Meter()
-- meter.shown
meter.value = 2
-- meter.shown
probe-outside()

━━━━━━━━━━━━━━━━━━━━
+ do probe-outside

. attempt = do new
    keep-live-state(Count())
    -- "leaked"
catch failure
    . panic Panic? = failure
    if panic
        -- "outside: $panic"
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ Failure is Error

━━━━━━━━━━━━━━━━━━━━
+ Meter

+ value = 1
+ shown = "${catch risky(#value)}"

━━━━━━━━━━━━━━━━━━━━
+ do risky String ?! Failure @live
    - goal Int

if goal % 2 == 1
    ?! Failure("odd $goal")
= "even $goal"

━━━━━━━━━━━━━━━━━━━━
+ Count
    is Tracker

────────────────────
+ do attach
    - action LiveAction

────────────────────
+ do detach
    - action LiveAction
```

```output
odd 1
even 2
outside: Only a live function keeps a live state
```

### A live function that panics leaves its live scope before the panic is recovered.

```yumi
/core/tracker

make()
probe-outside()

━━━━━━━━━━━━━━━━━━━━
+ do make

. attempt = do new
    . meter = Meter()
    -- meter.shown
catch failure
    . panic Panic? = failure
    if panic
        -- "made: $panic"
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ do probe-outside

. attempt = do new
    keep-live-state(Count())
    -- "leaked"
catch failure
    . panic Panic? = failure
    if panic
        -- "outside: $panic"
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ Meter

+ value = 1
+ shown = risky(#value)

━━━━━━━━━━━━━━━━━━━━
+ do risky String @live
    - goal Int

?! Panic("odd $goal")
    if goal % 2 == 1
= "even $goal"

━━━━━━━━━━━━━━━━━━━━
+ Count
    is Tracker

────────────────────
+ do attach
    - action LiveAction

────────────────────
+ do detach
    - action LiveAction
```

```output
made: odd 1
outside: Only a live function keeps a live state
```
