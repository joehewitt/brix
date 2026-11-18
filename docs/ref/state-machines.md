# State machines

A state machine is a value that is in one of several cases, and moves from case to case as values
arrive from events, channels and other sources. A case can hold values, run rows when it is
entered and left, and own locals, subscriptions and other machines that last while it is active.
Rows in a machine are a description of what moves it; the machine runs on its own.

Use a machine when a value moves through phases, when which values matter depends on the phase,
or when work should last only as long as a phase. Use a union for a value that only says which of
several things it is, and a subscription for values each handled the same way whatever came
before.

## A first machine

```brix
(main
  (let gate (call Turnstile))
  (print (. gate phase))
  (<- (. gate pushes) true)
  (print (. gate phase))
  (<- (. gate coins) 25)
  (print (. gate phase))
  (<- (. gate coins) 25)
  (print (is (. gate phase) .unlocked))
  (<- (. gate pushes) true)
  (print (is (. gate phase) .locked))
  (print (. gate phase name))
)

(type + Turnstile
  (event + coins Int)
  (event + pushes Bool)
  (prop + phase _
    (machine
      (state locked
        (then unlocked (trigger on coins))
      )
      (state unlocked
        (then locked (trigger on pushes))
      )
    )
  )
)
```

```output
locked
locked
unlocked
true
true
locked
```

- **Cases.** `(state locked)`{.brix} declares a case. The machine starts in its first case that has
  no trigger.
- **Transitions.** `(then unlocked (trigger on coins))`{.brix} in a case moves the machine to
  `unlocked` when a value arrives from `coins` while that case is active.
- **Ignored values.** A value that no transition takes is ignored. There is no invalid transition:
  pushing a locked turnstile leaves it locked.
- **Reading.** `(is (. gate phase) .unlocked)`{.brix} tests the case, a machine prints as its case,
  and `name` is the case's name as text.

## Where a machine lives

| Declaration | Machine |
| --- | --- |
| `(prop + phase _ (machine (state idle)))`{.brix} in a class | One for each object, started when the constructor ends. |
| `(prop + phase _ (machine (state idle)))`{.brix} in a module | One for the module, started when the module starts. |
| `(let flow (machine (state idle)))`{.brix} | A local, started where it is declared. It keeps running after its function returns. |
| `(lazy flow (machine (state idle)))`{.brix} | A local started where it is first read. |
| `(state idle)`{.brix} rows in `main`, or in a class's `content` | An anonymous machine; see [Anonymous machines](#anonymous-machines). |
| `(state ready (prop + auth _ (machine (state out))))`{.brix} | A machine nested in a case, which runs while the case is active. |

- **An initial case** is required: `State machine has no initial state`. Only an anonymous machine
  may start with no case active.
- **Structs** cannot hold one: `State machine properties are not supported on structs`. Nor can
  interfaces: `Gate is an interface, so it cannot hold a state machine; declare the machine in the
  classes that are Gate`.
- **The machine's type** is a union of its cases that the program cannot name, so the property's
  type is written `_`.

## Cases

A case is also called a state, as the `state` form and the compiler's messages call it.

- **Payloads.** `(state (call placed item count))`{.brix} declares a case that holds two values;
  see [Payloads](#payloads).
- **Triggers.** A case written with a trigger, as in
  `(state (call playing track) (trigger for track plays))`{.brix}, is entered each time the trigger
  fires, from any case.
- **Nested cases** are active together with the case that holds them. Entering a case enters its
  first nested case that has no trigger, and a nested case with a trigger is entered only by it.
- **One name, one case.** A case name written twice in one machine, at any depth, is an error:
  `Redefinition of state case 'idle'`.
- **Undeclared targets.** A `then` that names a case no `state` declares declares it, at the place
  the `then` is written.

## Transitions

| Transition | Moves |
| --- | --- |
| `(then cooling (trigger on readings))`{.brix} | On each value from `readings`. |
| `(then cooling (trigger on readings (> each 30)))`{.brix} | On each value that passes the guard. `each` is the value. |
| `(then cooling (trigger when-on (> each 30) readings))`{.brix} | The same, with the guard written first. |
| `(then (call placed item count) (trigger for (call .place item count) requests))`{.brix} | On each value that fits the pattern, which names its parts. A guard may follow the source. |
| `(then loading (trigger whenever #feed))`{.brix} | Each time what the value reads with `#` changes, while the case is active. |
| `(then hunting (trigger if ready))`{.brix} | When the case is entered, if the condition holds. |
| `(then (call valid n) (trigger if-match (call .valid n) login))`{.brix} | When the case is entered, if `login` fits the pattern. |
| `(then stopped)`{.brix} | When the case is entered, always. |
| `(else-state checking)`{.brix} | When no transition of the group before it moves; see [Fallbacks](#fallbacks). |

```brix
(main
  (let unit (call Thermostat))
  (for (in reading (list 21 31 26 23 12 35 22))
    (<- (. unit readings) reading)
    (print (str reading " " (. unit phase)))
  )
)

(type + Thermostat
  (event + readings Int)
  (prop + phase _
    (machine
      (state idle
        (then cooling (trigger on readings (> each 30)))
        (then heating (trigger on readings (< each 15)))
      )
      (state cooling
        (then idle (trigger on readings (< each 24)))
      )
      (state heating
        (then idle (trigger on readings (> each 20)))
      )
    )
  )
)
```

```output
21 idle
31 cooling
26 cooling
23 idle
12 heating
35 idle
22 idle
```

- **When a transition listens.** A `then` listens while its case is active. A trigger on a case at
  the top of the machine listens for the machine's whole life, and a trigger on a nested case while
  the case that holds it is active.
- **Groups.** The transitions of one case on one source are a group, tried in the order written.
  The first whose pattern and guard pass moves the machine, and a value moves it once: a reading of
  35 in `heating` goes to `idle`, not on to `cooling`.
- **Sources.** A source is an event, a channel or another `Broadcastable`, or a list, a text, a
  generator or another stream, whose items arrive in turn as the case is entered. Anything else is
  an error: `State trigger source is not broadcastable or iterable`. The source is worked out each
  time its case is entered.
- **Only by transitions.** Assigning a machine, or one of its cases, is an error:
  `State machine "phase" moves only by its transitions, so it cannot be assigned; add a transition
  that enters the case`.
- **Re-entry.** A transition to the active case, or to a case that holds it, leaves that case and
  enters it again, running both its exit and entry rows. A move between branches leaves the cases
  up to the one both share, and enters down from it.
- **Changes.** A `whenever` trigger follows the reads its value makes with `#`, as a derived
  property does, while its case is active. The value is read as the case is entered, which moves
  nothing, and each change after that moves the machine. A change while another case is active is
  dropped. A value that reads nothing with `#` is an error: `A "whenever" trigger fires as what it
  reads with "#" changes, and this one reads nothing with "#", so it would never fire; …`.
- **Decisions on entry.** A transition with `if`, `if-match` or no trigger is decided when its case
  is entered, in the order written, and is not tried again when what it reads changes; use
  `whenever` for that. Decisions that would enter a case again without a trigger are an error:

```brix compile-error
(prop + ready _ (= false))

(prop + phase _
  (machine
    (state a
      (then b (trigger if (not ready)))
    )
    (state b
      (then a (trigger if (not ready)))
    )
  )
)
```

```output
Exception: Entry decisions loop back to 'a' without a trigger
<no file:10>
          (then a (trigger if (not ready)))
          ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Fallbacks

```brix
(main
  (let login (call Login))
  (<- (. login results) .denied)
  (print (. login phase))
  (<- (. login results) (call .granted "token-1"))
  (print (. login phase))
  (print (. login phase path))

  (let strict (call Login))
  (<- (. strict results) .throttled)
  (print (is (. strict phase) .checking))
  (print (. strict phase path length))
)

(type + Result (is Union)
  (variant granted (tuple String))
  (variant denied)
  (variant throttled)
)

(type + Login
  (event + results Result)
  (prop + phase _
    (machine
      (state checking
        (then (call signed-in token) (trigger for (call .granted token) results (!= token "")))
        (then checking (trigger for .denied results))
        (else-state checking)
      )
      (state (call signed-in token))
    )
  )
)
```

```output
checking
token-1
signed-in
false
0
```

- **What it does.** `(else-state checking)`{.brix} acts when a value arrives that no transition of
  the group before it takes. It leaves the named case and the cases active inside it, and enters
  nothing.
- **No active case.** A fallback that leaves the machine's only active case leaves the machine with
  none: `is` is false for every case, and `path` is empty. Nothing starts it again.
- **Errors.** A group has one fallback: `Multiple fallbacks in one state transition group`. A
  fallback needs a group before it: `State fallback has no transition group`.

## Payloads

```brix
(main
  (let order (call Order))
  (print (. order summary))
  (<- (. order requests) .cancel)
  (print (. order summary))
  (<- (. order requests) (call .place "tea" 2))
  (print (. order summary))
  (<- (. order payments) 7.5)
  (print (. order summary))
  (print (. order phase))
)

(type + Request (is Union)
  (variant place (tuple String Int))
  (variant cancel)
)

(type + Order
  (event + requests Request)
  (event + payments F64)
  (prop + phase _
    (machine
      (state empty
        (then (call placed item count) (trigger for (call .place item count) requests))
      )
      (state (call placed item count)
        (then (call paid item count each) (trigger on payments))
      )
      (state (call paid item count amount))
    )
  )
  (fn-expr + summary String :get
    (match phase
      (case .empty
        "empty"
      )
      (case (call .placed item count)
        (str count " x " item)
      )
      (case (call .paid item count amount)
        (str count " x " item ", paid " amount)
      )
    )
  )
)
```

```output
empty
empty
2 x tea
2 x tea, paid 7.5
(tea, 2, 7.5)
```

- **Giving values.** The target of a transition gives the case its values:
  `(then (call paid item count each) (trigger on payments))`{.brix} reads the payload of the case it
  leaves, and the value that arrived as `each`. A target may also read the names a pattern binds and
  the members of the object.
- **Names.** The names in a case's own parentheses, such as `amount`, name its values in its rows
  and in `match` arms.
- **Types.** A payload's types are those of the values its transitions give. A transition that gives
  values of other types is an error where it gives them, as an argument of the wrong type is.
- **The first case.** Nothing enters a body's first case with a transition, so it holds values
  only where its head gives them, as `(state (call counting 0))`{.brix} does. A name there that
  names no value is an error: `Initial state case 'answering' cannot take a payload, since nothing
  enters it to give it one; give it values, or write a case with no payload before it`.
- **Count.** A transition gives a case as many values as it holds: `Case "held" holds one value,
  and this transition gives none; give it one value`.
- **Captures.** A pattern on a case's trigger may bind names the case does not hold. They last for
  that activation: the case's entry rows, guards and targets read them, but they are not part of
  the machine's value.
- **Printing.** A case with a payload prints as its payload, as a union's case does.

## Reading a machine

```brix
(main
  (let editor (call Editor))
  (call (. editor report))
  (<- (. editor focuses) true)
  (call (. editor report))
  (<- (. editor saves) "notes.txt")
  (call (. editor report))
  (print (. editor status))
)

(type + Editor
  (event + focuses Bool)
  (event + saves String)
  (prop + status _ (= (if-expr (is #phase .saved) "saved" "unsaved")))
  (prop + phase _
    (machine
      (state editing
        (state idle)
        (state focused (trigger on focuses))
        (then (call saved name) (trigger for name saves))
      )
      (state (call saved name))
    )
  )
  (fn-do + report _
    (print (call (. " > " join) (. phase path)))
    (print (str "editing " (is phase .editing) ", focused " (is phase .focused)))
    (match phase
      (case (call .saved name)
        (print (str "saved as " name))
      )
      (case .focused
        (print "match: focused")
      )
      (default
        (print (str "match: " (. phase name)))
      )
    )
  )
)
```

```output
editing > idle
editing true, focused false
match: idle
editing > focused
editing true, focused true
match: focused
saved
editing false, focused false
saved as notes.txt
saved
```

| Read | Gives |
| --- | --- |
| `(is phase .editing)`{.brix} | Whether the case is active, itself or through a case nested in it. |
| A `match` on the machine | The arm of the innermost active case. Arms bind the payload, and the match must cover every case. |
| `(. phase name)`{.brix} | The innermost active case's name. |
| `(. phase case)`{.brix} | Its position among the machine's cases, from 0. |
| `(. phase path)`{.brix} | The names of the active cases, outermost first, as a `[String]`. |
| The machine in a print or a text | The innermost case's name, or its payload. |

- **A copy.** Reading a machine copies its case and payload. A local that holds the copy keeps them
  after the machine moves on, and `is`, `match` and `path` read the copy. `==` compares two copies.
  A case is tested with `is`, not `==`: `A state machine's case is tested with "is" or "is not",
  which count the cases nested in it; write "is .locked" in place of "== .locked"`.
- **Tracked reads.** `(is #phase .saved)`{.brix} in a property's value or a rule's test follows the
  machine, as `status` does above; see Reactivity. Without `#`, the value is worked out once.
- **Cases** are named with a dot, and a case the machine does not have is an error:
  `Case ".nope" not found in state machine (.locked | .unlocked)`.
- **Nested machines** are read only inside the cases of the case that holds them: from elsewhere,
  `Name "auth" not found`.

### Anonymous machines

Cases written in `main`, or in a class's `content`, with no machine around them are an anonymous
machine. Each case is a value of its own name: a `Bool` for a case with no payload, and an optional
of its payload for one with a payload, present while the case is active.

```brix
(prop + messages (of AsyncChannel String))
(prop + counts (of AsyncChannel Int))
(prop + inbox _
  (machine
    (state empty)
    (state (call holding count) (trigger for count counts))
  )
)

(main
  (state waiting)
  (state (call received value) (trigger for value messages))
  (print (str waiting " " (?? received "none") " " inbox))
  (<- messages "hello")
  (<- counts 3)
  (print (str waiting " " (?? received "none") " " inbox))
  (do-new
    (print (str waiting " " (?? received "none") " " inbox))
  )
)
```

```output
true none empty
true none empty
false hello 3
```

- **Starting.** An anonymous machine starts in its first case with no trigger. With none, it starts
  with no case active, and every case's value is false or absent.
- **Channels.** Values written to an `AsyncChannel` move a machine in a task of their own, after
  the writer waits, so the machines here have moved only when the task `do-new` starts runs.

## Entering and leaving

```brix
(main
  (let player (call Player))
  (print "-- play")
  (<- (. player plays) "intro")
  (print "-- play again")
  (<- (. player plays) "outro")
  (print "-- stop")
  (<- (. player stops) true)
  (print (. player phase path))
)

(type + Player
  (event + plays String)
  (event + stops Bool)
  (prop + phase _
    (machine
      (state stopped
        (do
          (print "enter stopped")
        )
        (finally
          (print "exit stopped")
        )
      )
      (state (call playing track) (trigger for track plays)
        (do
          (print (str "enter playing " track))
        )
        (finally
          (print (str "exit playing " track))
        )
        (state buffering
          (do
            (print "enter buffering")
          )
          (finally
            (print "exit buffering")
          )
        )
        (then stopped (trigger on stops)
          (do
            (print (str "stopping " track))
          )
        )
      )
    )
  )
)
```

```output
enter stopped
-- play
exit stopped
enter playing intro
enter buffering
-- play again
exit buffering
exit playing intro
enter playing outro
enter buffering
-- stop
exit buffering
exit playing outro
stopping outro
enter stopped
stopped
```

- **Entry and exit.** A case's `do` rows, as in `(do (print "enter"))`{.brix}, run when it is
  entered, and its `finally` rows, as in `(finally (print "exit"))`{.brix}, when it is left.
- **Order.** A move leaves the active cases from the innermost out, runs the transition's own `do`
  rows, and then enters the target from the outermost in.
- **Again.** `playing` has a trigger of its own, so a second track leaves it and enters it again.

### Values that live in a case

```brix
(import /stream/timing)

(main
  (let kettle (call Kettle))
  (<- (. kettle ticks) 0.5)
  (<- (. kettle switches) 10.0)
  (for (in i (..< 0 5))
    (<- (. kettle ticks) 0.5)
    (print (str (. kettle phase name) " " (. kettle heat)))
  )
  (<- (. kettle switches) 20.0)
  (<- (. kettle ticks) 0.5)
  (<- (. kettle switches) 30.0)
  (for (in i (..< 0 4))
    (<- (. kettle ticks) 0.5)
  )
  (print (str (. kettle phase name) " " (. kettle heat)))
)

(type + Kettle
  (event + switches F64)
  (event + ticks F64)
  (prop + heat F64 (= 0))
  (prop + runs _ (= 0))
  (prop + phase _
    (machine
      (state off)
      (state (call heating rate) (trigger for rate switches)
        (let run (call next-run))
        (do
          (print (str "run " run " at " rate))
        )
        (for (on dt ticks)
          (+= heat (* dt rate))
        )
        (then whistling (trigger on (call delayed ticks 2)))
      )
      (state whistling
        (do
          (print "whistling")
        )
      )
    )
  )
  (fn-do _ next-run Int
    (+= runs 1)
    (return runs)
  )
)
```

```output
run 1 at 10
heating 5
heating 10
heating 15
whistling
whistling 20
whistling 20
run 2 at 20
run 3 at 30
whistling
whistling 90
```

- **Locals.** A `let` or `lazy` in a case is made each time the case is entered. The case's entry
  rows and its transitions' sources read it.
- **Handlers.** `(for (on dt ticks) (+= heat dt))`{.brix} in a case runs for each value while the
  case is active. It subscribes as the case is entered and ends as it is left. It takes one source
  and only a `when` clause: `A state handler subscribes to a single source`, `A state handler
  accepts only a when clause`. It gives no value: `A state machine's handler gives no value, so it
  cannot return one with "="; end it early with "= alone"`.
- **Order.** The handlers and transitions of a case take a value in the order they are written, so
  a transition written first may leave before a handler sees the value.
- **Timers.** `/stream/timing` makes timers from a stream of `F64` ticks, each the time since the
  last. `(call delayed ticks 2)`{.brix} passes ticks on once they add up to 2, `totaled` gives each
  tick with the total so far, and `driven` passes on the ticks a function accepts. A source is
  worked out each time its case is entered, so entering `heating` again starts its timer again.
  A timer belongs to the case whose transition or handler names it, so a timer on an enclosing
  case keeps counting while the cases nested in it change. Once `delayed` passes ticks on, its
  transition's guard is tested on each tick, so a guard that fails holds the move until a later
  tick passes it.
- **Nested machines** start when the case that holds them is entered and stop when it is left;
  see [Machines side by side](#machines-side-by-side).
- **Tasks.** A task that a case's `do` rows or handlers start, or the `do` rows of a transition
  into it, belongs to that time the case is entered, as does one started in a nested body there,
  such as a function value's. The case's `finally` rows can still use it, and leaving the case
  cancels it once they end, without waiting for it. A task that leaves its own case, by sending
  what moves the machine, is not cancelled as it leaves; the case's next exit cancels it, or its
  object's `close`. A task the `finally` rows start, or one marked `@detached`, does not belong to
  the case; see [Owned tasks](concurrency.md#owned-tasks).
- **Closing.** Calling an object's `close` leaves every machine its properties hold, and the one its
  class's `content` writes, before the body of `close` runs, so their exit rows see the object as
  it was. The object's events and the live actions it owns close with them, and no transition
  moves them after.
- **Errors** that a case's entry or exit rows raise go to `unhandled-error`.

Here the timer belongs to `baking`, so the move from `top` to `bottom` does not restart it. It
reaches 4 at the fourth tick, while the door is open, and the move waits for the next tick:

```brix
(import /stream/timing)

(main
  (let oven (call Oven))
  (for (in i (..< 1 7))
    (= (. oven door-open) (== i 4))
    (if (== i 2)
      (<- (. oven flips) true)
    )
    (<- (. oven ticks) 1.0)
    (print (str "tick " i ": " (call (. " > " join) (. oven phase path))))
  )
)

(type + Oven
  (event + ticks F64)
  (event + flips Bool)
  (prop + door-open _ (= false))
  (prop + phase _
    (machine
      (state baking
        (state top
          (then bottom (trigger on flips))
        )
        (state bottom)
        (then done (trigger on (call delayed ticks 4) (not door-open)))
      )
      (state done)
    )
  )
)
```

```output
tick 1: baking > top
tick 2: baking > bottom
tick 3: baking > bottom
tick 4: baking > bottom
tick 5: done
tick 6: done
```

### Machines side by side

A case can hold several machines. They move on their own, and all of them stop when the case is
left. So two things that vary apart, such as who is signed in and whether the network is up, are
two machines in one case, not a case for each combination.

```brix
(main
  (let session (call Session))
  (<- (. session sign-ins) true)
  (<- (. session connects) true)
  (<- (. session drops) true)
  (<- (. session quits) true)
  (print (. session phase))
)

(type + Session
  (event + sign-ins Bool)
  (event + connects Bool)
  (event + drops Bool)
  (event + quits Bool)
  (prop + phase _
    (machine
      (state running
        (prop + who _
          (machine
            (state guest
              (do
                (print "guest")
              )
            )
            (state member (trigger on sign-ins)
              (do
                (print "member")
              )
              (finally
                (print "member ends")
              )
            )
          )
        )
        (prop + link _
          (machine
            (state offline
              (do
                (print (str "offline, " who))
              )
            )
            (state online (trigger on connects)
              (do
                (print (str "online, " who))
              )
              (finally
                (print "online ends")
              )
              (then offline (trigger on drops))
            )
          )
        )
        (then ended (trigger on quits))
      )
      (state ended)
    )
  )
)
```

```output
guest
offline, guest
member
online, member
online ends
offline, member
member ends
ended
```

- **Reading each other.** The cases of a machine can read a machine declared before it in the same
  case, as `link`'s cases read `who`. One declared after it is not found: `Name "link" not found`.

## Timing

- **Events.** A value published to an event moves the machine before the publish goes on. The
  event's other subscribers run first, then the machine's transitions.
- **One move at a time.** A value published while a machine is moving waits until the move ends,
  and is delivered before the outermost publish goes on. Publishes nest only within a task: a
  subscriber that waits, or is canceled, does not hold back another task's publish.
- **Channels.** A value written to an `AsyncChannel` moves the machine in a task of its own, after
  the writer waits. A machine does not see what was written before it subscribed.

## Refining a machine in a subclass

A subclass can declare a machine property of its base class again. Its cases name cases of the
base machine, and add nested cases, entry and exit rows, handlers and transitions to them; the two
make one machine.

- A refinement adds no case at the top: `Refinement cannot add top-level state case 'extra'`.
- A case is refined where the base declares it: `State case 'inner' is declared elsewhere in the
  inherited machine`.
- A refined case keeps its trigger and payload: `Refined state case 'busy' cannot change its trigger
  or payload`.
- Only a machine can be refined: `Inherited property 'body' is not a state machine`.
- Where the base writes a first case in a case, it stays the first, so a case the refinement adds
  there may come first and hold a payload.
- Transitions on one source at several levels are one group, the base's first.
