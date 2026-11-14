# State machines

A state machine describes what can happen next. A connection can be waiting, connected, or
closing; a form can be editing, submitting, or showing a result. Yumi gives those states,
their transitions, and the work that belongs to each state a place in the language.

The same declaration can carry data into a state, listen for events while it is active, and
release its subscriptions when it exits. This keeps the lifetime of an operation close to
the conditions under which the program should care about it.

## At a glance

| Capability | Form | What it gives you |
| --- | --- | --- |
| Name a machine | `+ phase ~>` | A declaration whose body is its cases, like a function body. |
| Declare a state | `~> waiting` | A named case; the first untriggered case is the initial state. |
| React to an event | `~> ready on source` | Enter a case when the source emits. |
| Continue from a state | `then ready on source` | A transition available while the enclosing state is active. |
| Capture event data | `for pattern on source` | Match the event and bind values for the target state. |
| Filter an event | `on source when condition` | Check a guard for each arrival. |
| Decide on entry | `if condition` | Choose an immediate transition when the owning scope enters. |
| Run state-owned work | `do`, `do finally` | Actions on entry and exit. |
| Handle events while active | `for value on source` in a state | A handler that starts on entry and stops on exit. |
| Wait in a state | `then next on delayed(source, 2)` | A library source that starts counting when the state enters. |
| Organize several levels | Nested `~>` cases or named machines | A child state, or independent state axes with a shared lifetime. |

## States and the transitions between them

A named machine is written like a function whose body is its cases. `~>` after the name marks
that body, as `~` marks a pattern function. The machine exposes its state as a union-valued
declaration, and a machine declared in a class gives each instance its own state:

:::example-pair

- [Read the state](#source-door--read): Use the familiar union case test. A new door starts closed.
- [Publish an event](#source-door--publish): Request a transition through the door's event.
- [Observe the result](#source-door--observe): The transition finishes before the publishing call returns, so the next read sees open.
- [Initial state](#source-door--initial): The first case without a trigger starts active.
- [Outgoing transitions](#source-door--transitions): Each `then` belongs to the state enclosing it, so an open request matters while closed and a close request matters while open.

```yumi #door
. door = Door()
-- ⟦read⟧door.phase is .closed⟦/read⟧
⟦publish⟧door.open-requests <- true⟦/publish⟧
-- ⟦observe⟧door.phase is .open⟦/observe⟧

━━━━━━━━━━━━━━━━━━━━
+ Door

+ event open-requests Bool
+ event close-requests Bool

────────────────────
+ phase ~>

⟦initial⟧~> closed⟦/initial⟧
    ⟦transitions⟧then open on open-requests⟦/transitions⟧

~> open
    ⟦transitions⟧then closed on close-requests⟦/transitions⟧
```

:::

Both lines print `true`. Transitions triggered by a declared event run to completion before
the publication returns: the event's ordinary handlers run first, then the machine's
transitions. When a transition's actions publish further events, the transitions those
events trigger wait until the current one finishes, then run in order, still before the
outermost publication returns. A machine is therefore never inside two transitions at once,
and a loop that publishes an event on every step sees each machine's new state on the next
line. A source with its own delivery timing, such as `AsyncChannel`, triggers transitions
when it delivers.

`~>` declares a case and can also attach a trigger that enters it. A trigger on a case at
the machine's root is available throughout that machine's active lifetime. A `then` inside
a case instead describes an outgoing transition from that case. Use that placement to
express which events are meaningful in each state.

An untriggered case is a destination, not an instruction to advance to the next line. The
door stays closed until a transition selects open; it does not run through both declarations.

## Events can carry the next state's data

The `for` part of a transition uses [structural patterns](structural-pattern-matching.md).
It can recognize a union case or tuple and capture values. A following `when` guard can use
those captures; the target's arguments become its payload.

:::example-pair

- [Recognize and capture](#source-submission--pattern): Only a `.entered` submission matches, and its string becomes `name`.
- [Guard the transition](#source-submission--guard): The capture is available before the guard runs.
- [Carry data forward](#source-submission--payload): Store the accepted name with the state.
- [Read the result](#source-submission--result): Matching the machine extracts the payload. This prints `Accepted Ada`.

```yumi #submission
+ submitted AsyncChannel'Submission

+ form ~>
    ~> waiting
    ~> ⟦payload⟧accepted(name)⟦/payload⟧ ⟦pattern⟧for .entered(name) on submitted⟦/pattern⟧ ⟦guard⟧when name.length > 0⟦/guard⟧
    ~> rejected for .entered(name) on submitted when name.length == 0

submitted <- .entered("Ada")
do new
    -- match form
        ⟦result⟧| .accepted(name)
            "Accepted $name"⟦/result⟧
        ??
            "No accepted name"

━━━━━━━━━━━━━━━━━━━━
+ Submission is Union

| entered String
```

:::

The incoming event and the machine's state are separate values. `Submission` describes what
arrived; the inferred state union describes what the form is doing as a result. Several
payload expressions can be supplied as a tuple, as in `authenticating(email, password)`.
[Sum types](sum-types.md) explains the ordinary case tests and matches used to read them.

For a simple guard without destructuring, `each` names the arriving item:

```text
~> positive on numbers when each > 0
```

Sibling transitions sharing a trigger source are tried in source order. The first matching
pattern and guard wins for that decision. Without a matching transition or a fallback, that
decision leaves the active state unchanged. Put a more specific alternative before a broad
one when both could accept the same event.

## Immediate decisions and event decisions

`if` decides when the owning machine or state enters. `when` filters an arriving event.
They are useful at different moments:

:::example-pair

- [Entry decision](#source-immediate--decision): The machine enters `guest`, then immediately selects `member` because access is available.
- [Later change](#source-immediate--change): Changing the condition's input does not itself rerun the entry decision. Both reads print `true`.

```yumi #immediate
+ has-access = true

+ access ~>
    ~> guest
    ⟦decision⟧~> member if has-access⟦/decision⟧

-- access is .member
⟦change⟧has-access = false⟦/change⟧
-- access is .member
```

:::

An immediate decision can also match a shape, using a form such as
`then ready(value) if | .available(value) = result`. As with event decisions, source order
resolves competing immediate transitions. A `then` with no trigger always passes, so a
final `then wandering` after a list of `if` decisions names where the state goes when none
of them match. Immediate decisions cannot lead back to a state that is still being entered,
because entering would never settle; the compiler rejects such a loop, and a trigger such as
`on delayed(ticks, 1)` is how a state comes back around.

To respond to later changes, supply a source of events or updates. A Boolean expression
in an `if` trigger is not a continuously observed condition. The stream and tracked-value
forms in [Concurrency and reactivity](concurrency-and-reactivity.md) provide the vocabulary
for changing sources.

## Entry, exit, and the lifetime of a state

A state body can contain entry actions, exit actions, local bindings, and continuations.
`do` runs on entry; `do finally` runs when that activation exits:

:::example-pair

- [Enter](#source-lifetime--entry): The initial activation prints `enter showing`.
- [Exit](#source-lifetime--exit): Leaving the activation prints `exit showing`.
- [Enter again](#source-lifetime--again): A transition back to the same state exits it and starts a fresh activation. Refresh therefore prints the exit line followed by another entry line.

```yumi #lifetime
+ refresh AsyncChannel'Bool

+ view ~>
    ~> showing
        ⟦entry⟧do
            -- "enter showing"⟦/entry⟧

        ⟦exit⟧do finally
            -- "exit showing"⟦/exit⟧

        ⟦again⟧then showing on refresh⟦/again⟧

refresh <- true
do new
    -- view is .showing
```

:::

The final line prints `true`. A self-transition is a restart of the state's lifetime, so use
it when entry work and local state should be recreated. An ordinary update inside an action
does not require a transition.

Immediate locals (`.`) and lazy locals (`..`) belong to an activation. Re-entering recreates
them; repeated reads of a lazy local within one activation reuse that activation's value.
Continuations can use the state's event captures and local sources.

State-owned transition subscriptions are removed on exit. Callbacks belonging to an old
activation cannot advance a later activation of that state. This is useful for a request
that becomes obsolete when a user cancels or leaves a screen. It does not mean all independent
work behind a source is canceled: the producer's resource and task lifetime still needs its
own completion or cancellation arrangement.

## Work while a state is active

A `for … on` handler inside a state runs for each arrival while that state is active. It
subscribes when the state enters and stops when the state exits. Every source a state names
is evaluated on entry, so a source with its own state starts fresh each time. Timers are
ordinary library sources built this way: in `/stream/timing`, `delayed(source, limit)`
forwards arrivals once their payloads add up to the limit, and `totaled(source)` delivers each
arrival paired with the running total. Together they let a fixed-step loop drive a machine
without counters in the class:

:::example-pair

- [Handle arrivals](#source-kettle--handler): While heating, every tick adds heat.
- [Wait for the total](#source-kettle--timer): Four half-second ticks add up to the limit, so the kettle starts whistling on the fourth tick.
- [Stop on exit](#source-kettle--result): Later ticks no longer reach the handler. This prints `true` and `20`.

```yumi #kettle
/stream/timing

. kettle = Kettle()
for i in 0 .. < 6
    kettle.ticks <- 0.5
-- ⟦result⟧kettle.phase is .whistling⟦/result⟧
-- kettle.heat

━━━━━━━━━━━━━━━━━━━━
+ Kettle

+ event ticks F64
+ heat F64 = 0

────────────────────
+ phase ~>

~> heating
    ⟦handler⟧for dt on ticks
        heat += dt * 10⟦/handler⟧

    ⟦timer⟧then whistling on delayed(ticks, 2)⟦/timer⟧

~> whistling
```

:::

Handlers run with the machine's transitions, after the event's ordinary handlers. Work from
a source that relays an event, like `delayed(ticks, 2)`, waits the same way. A state's
handlers and transitions take each arrival in source order, so a transition written before a
handler can leave the state before that handler sees the arrival. A `when` clause filters
arrivals the same way it does for a transition. A handler's pattern binds the arrival as it
does for any `for … on` subscription.

A timer belongs to the state that names it: the state enclosing a `then` or a handler, or the
machine itself for a trigger on a root case. Re-entering that state, including through a
self-transition, starts the total again from zero, while a parent's timer keeps counting as
its children change. Once `delayed` starts forwarding, the transition checks its guard on each
arrival, so a guard that is false holds the transition until a later arrival passes it. A
handler that needs the time spent in its state reads it from `totaled`:
`for elapsed, dt on totaled(ticks)`.

## Nested states share a parent lifetime

Nested cases describe a more specific state inside an active parent. Entering a child leaves
the parent active:

:::example-pair

- [Parent](#source-nesting--parent): The outer machine starts in `editing`.
- [Child](#source-nesting--child): Its initial child is `focused`. Both case tests are true at once.
- [Leave the parent](#source-nesting--leave): Finishing exits the active child before the parent and selects `done`.

```yumi #nesting
+ finish AsyncChannel'Bool

+ editor ~>
    ⟦parent⟧~> editing⟦/parent⟧
        do finally
            -- "exit editing"

        ⟦child⟧~> focused⟦/child⟧
            do finally
                -- "exit focused"

        ⟦leave⟧then done on finish⟦/leave⟧

    ~> done

-- editor is .editing
-- editor is .focused
finish <- true
do new
    -- editor is .done
```

:::

The output is `true`, `true`, `exit focused`, `exit editing`, and `true`. Sibling cases are
alternatives, but a parent and its active descendant describe different levels of the same
configuration. A machine with nested states therefore has more structure than a single
flat union tag. Use its case tests to ask which levels are active.

Re-entering a parent exits and rebuilds its active subtree. Leaving it also exits any named
machines declared inside it and removes their transition subscriptions.

### Independent state axes

Some choices should vary independently. A session can be authenticated while its network is
offline. Declare separate named machines inside a shared parent instead of enumerating every
combination. This fragment assumes `signed-in` and `connected` event sources:

```text
+ session ~>
    ~> running
        + identity ~>
            ~> guest
            ~> member on signed-in

        + network ~>
            ~> offline
            ~> online on connected
```

Changing `identity` does not select a case in `network`, and both belong to the lifetime of
`running`. Use nested cases for successive levels of one state and nested named machines for
independent choices.

## Anonymous machines expose their cases directly

At module level or in class content, a machine can omit the binding name. Yumi then exposes
each case as a projection of the machine's state: a Boolean for a case without data, or an
optional payload for a case with data.

:::example-pair

- [Boolean projection](#source-projections--boolean): `waiting` starts true.
- [Payload projection](#source-projections--payload): `received` is absent until that case is active.
- [Read the payload](#source-projections--read): After the transition, this prints `hello`.

```yumi #projections
+ messages AsyncChannel'String

⟦boolean⟧~> waiting⟦/boolean⟧
⟦payload⟧~> received(value) for value on messages⟦/payload⟧

-- waiting
-- received == null
messages <- "hello"
do new
    -- ⟦read⟧received ?? "No message"⟦/read⟧
    -- waiting
```

:::

This prints `true`, `true`, `hello`, and `false`. The generated names describe one machine;
they are not unrelated mutable flags. An anonymous machine with only triggered cases can
start inactive, with every projection false or absent, until a trigger selects a case.

Choose the declaration's location according to who owns it:

| Declaration | Ownership and observation |
| --- | --- |
| Module-level `+ phase ~>` | A module-owned named machine, read with case tests or matches. |
| Class `+ phase ~>` | Independent machine storage for every object. |
| Local `. phase ~>` | A machine initialized when execution reaches the declaration. |
| Local `.. phase ~>` | Initialized on first use, once per lazy declaration lifetime. |
| Anonymous cases in class content | Per-object state with directly readable case projections. |

A machine declared in class content, or after a module's thick divider, follows its own thin or
thick divider like any function: the header line, a blank line, then the cases at column zero.
A machine declared among statements, such as a local, a module's top-level code, or a nested
machine inside a state, indents its cases under the header instead, as a local pattern rule does.

## A fallback clears its target

The state-machine spelling `?? state` has a specific meaning: when the preceding transition
decision finds no match, deactivate the named target and its active descendants. It does not
enter that state as a default destination.

:::example-pair

- [Unmatched decision](#source-fallback--decision): The immediate guard is false.
- [Clear the target](#source-fallback--clear): Exit `loading`. Neither state remains active, so both reads print `false`.

```yumi #fallback
+ is-ready = false

~> loading
    ⟦decision⟧then ready if is-ready⟦/decision⟧
    ⟦clear⟧?? loading⟦/clear⟧

~> ready

-- loading
-- ready
```

:::

For event transitions, the fallback belongs to the preceding event decision group and runs
when an arrival matches none of that group's alternatives. Omitting the fallback keeps an
unmatched decision from clearing state. Use an explicit transition when the desired result
is to enter another state.

## Let an interface follow the machine

State tests and projections can participate in tracked expressions. Inside a component,
markup can read `#is-open` to follow an anonymous case, or `#phase is .open` to follow a
named machine. A read through another object uses a form such as `peer.#is-open`.

This class-content fragment assumes a terminal component with open and close event sources:

```text
<This
    <Label
        text = if #is-open then "Open" else "Closed"

~> is-closed
    then is-open on open-requests

~> is-open
    then is-closed on close-requests
```

The machine owns the transitions; the label declares how the active state appears. Styles
can use the same tracked values. [Markup and stylesheets](markup-and-stylesheets.md) explains
the surrounding tree and live-property model.

Use a state machine when the program has meaningful phases, phase-specific events, or work
whose lifetime belongs to a phase. Use an ordinary union when a value only needs to describe
an alternative, and a subscription when each arrival can be handled without a state model.
The benefit of the machine is making the allowed progress and its lifetimes visible together.
