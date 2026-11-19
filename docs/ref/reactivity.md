# Reactivity

A value can follow state. A read written with `#`, as in `#count`, is tracked, and a value that
makes tracked reads is worked out again when what it read changes. A property whose value reads
tracked state stays up to date, a rule in a class runs when what it reads changes, and an
assignment or a `lazy` that reads tracked state follows it too. Events are the other half: a class
declares an event, and its subscribers run as soon as a value is published to it.

This chapter covers tracked reads, derived properties, rules, live statements, streams of a value,
events, and the order in which all of these run. Tasks and channels are in Concurrency, and markup
that follows state is in Markup.

## Tracked reads

`#name` reads a property, a local or a parameter and tracks it, and `(. object #name)`{.brix} does
so for another object's property. A tracked read gives the same value as a plain one. Where it is
written in a value that follows state, such as a property's value, a rule's test or a live
statement, the value follows what it read. Anywhere else nothing would follow it, so it is an
error: `"#count" tracks nothing here: a read is followed only where it is written directly in a live
value, …`. Write `count` there to read it plainly.

| Read | Tracked |
| --- | --- |
| A stored property of a class: `#celsius`, `(. t #celsius)`{.brix} | Yes. |
| A local or a parameter: `#n` | Yes. |
| A list | Yes, with the changes made to it in place, such as `(+= items 4)`{.brix}. |
| A module value | No: `Untrackable name`. |
| A getter, or a property computed each time it is read | No: `Tracked getters are not supported; track concrete state instead`. |
| A property of a struct | No: `"#x" reads a field of Point, whose values are copied, so it cannot be tracked; track a property of a class`. |
| A method call: `(call (. c #size))`{.brix} | No: `"size" is a method, so "#" before it tracks nothing; track the properties it reads instead`. |
| An item of a pipeline given to a call: `(call count (in each #items (when (. each #done))))`{.brix} | Yes, while a function works the pipeline through as it is called, so the value runs again as an item changes. The compiler reads the function's body: it works the pipeline through where it only loops over it, pulls from it with `<-`, or gives it to another function that does. A function that hands it back, as a generator such as `scan` does, passes that on to the call that takes what it hands back, as in `(call last (call scan 0 (in each #items)))`{.brix}. One that keeps the pipeline any other way, such as by storing it, is reported where the item is read, and so is a pipeline given to no call, such as one that is the whole value: `"#done" is read for each item of a pipeline, which is followed only where the pipeline is given to a call in a live value, …`. |

- **Only reads written in the value.** A tracked read inside a function the value calls is an
  error, as nothing follows it there. Pass the tracked read to the function as an argument instead,
  as in `(call noted (. source #count))`{.brix}. The same holds for a closure that is the whole
  value of an attribute, which runs once the attribute is given, and for the `keyed-by` clause of a
  markup loop.
- **Values given to rules.** A value an element gives a rule it applies may make tracked reads: the
  rule's declarations that read the value follow them.
- **Chains.** Each `#` in a chain is followed: `(. v #source #count)`{.brix} follows a change to
  `count` and a new `source`. A chain that starts at a local, a parameter or a property of the
  object itself, as `(. source #count)`{.brix} does in a class, follows that name too.

### What a store notifies

A store notifies what follows the property, unless the value stored is the one already there.

- **Numbers** compare by their bits, so storing `-0.0` over `0.0` notifies.
- **Tuples** compare by value.
- **Objects, text and lists** compare by identity: storing a new list with the same items notifies.
  A `whenever-rule` then compares its own value again with `!=`; see [Rules](#rules).

## Derived properties

A property whose value makes a tracked read is derived. It is worked out when the object is made,
and again after what it reads changes.

```brix
(main
  (let cart (call Cart))
  (print (. cart total))
  (= (. cart quantity) 3)
  (print (. cart total))
  (= (. cart total) 0)
  (print (. cart total))
  (= (. cart price) 4)
  (print (. cart total))
)

(type + Cart
  (prop + quantity _ (= 1))
  (prop + price _ (= 5))
  (prop + total _ (= (* #quantity #price)))
)
```

```output
5
15
0
12
```

- **When it is read.** A derived property that nothing follows is worked out again only when it is
  read after a change, once, however many changes came before.
- **Never mixed.** A rule that reads a derived property and what it derives from sees them agree.
- **Writing one** stores a value that stays until what the property reads changes.
- **Errors** that its value raises when it is worked out again go to `unhandled-error`, and the
  property keeps its value.
- **Cycles.** Values that read each other are an error: `"a" depends on its own value, directly or
  through the values it reads; break the cycle`.
- **Computed once.** A property computed the first time it is read, `(prop + p _ (:= value))`{.brix},
  cannot make tracked reads: `A lazy property is worked out once, when it is first read, so it
  cannot follow "#a"; write "=" in place of ":=" to keep it live, or read "a" without "#"`.
- **Closing.** Calling the object's `close` stops its derived properties, rules and subscriptions.
  The class must declare `close`, even with an empty body, as in `(fn-do + close _)`{.brix}.

## Rules

A rule is a section of a class that runs when what it reads changes.

```brix
(main
  (let t (call Thermostat))
  (print (str "start " (. t fahrenheit) " F"))
  (= (. t celsius) 30)
  (print "stored 30")
  (= (. t celsius) 30)
  (print "stored 30 again")
  (= (. t celsius) 18)
  (print (str "stored 18, changes " (. t changes)))
)

(type + Thermostat
  (prop + celsius _ (= 20.0))
  (prop + fahrenheit _ (= (+ (/ (* #celsius 9) 5) 32)))
  (prop + changes _ (= 0))
  (depends counted celsius)
  (when-rule (> #celsius 25)
    (print (str "hot at " celsius))
  )
  (whenever-rule #fahrenheit
    (print (str "now " fahrenheit " F"))
  )
  (fn-do + counted _
    (+= changes 1)
  )
)
```

```output
start 68 F
now 86 F
hot at 30
stored 30
stored 30 again
now 64.4 F
stored 18, changes 2
```

| Rule | Runs |
| --- | --- |
| `(when-rule (> #celsius 25))`{.brix} | As the object is made, if the test holds, and then each time something it reads changes while the test holds. |
| `(whenever-rule #fahrenheit)`{.brix} | Each time the value changes, compared with `!=`. Not as the object is made. |
| `(depends counted celsius)`{.brix} | Calls the method `counted` each time one of the named properties changes. Not as the object is made. |

- **`when` and `whenever`.** Storing 1, 2, 0, -1 and 3 in turn runs `(when-rule (> #a 0))`{.brix}
  at 1, 2 and 3, while the test holds, and `(whenever-rule (> #a 0))`{.brix} at 1, 0 and 3, each time
  the test's value changes.
- **After the constructor.** Rules start once the constructor ends. A store in the constructor runs
  no rule, and a `when-rule` first sees the values the constructor set.
- **Tracked reads.** A rule's test must read tracked state: `when rules must read at least one
  tracked property`, and `whenever value triggers must read at least one tracked property (like
  #bar)`. It may read other objects, as `(whenever-rule (. meter #level))`{.brix} does.
- **`depends`** stands with the class's properties. The method it names takes no argument it
  requires: `A dependency calls "counted" with nothing, but it takes "n"; give "n" a default, or
  name a method that takes nothing`. The properties it names are stored ones: `"twice" is a
  getter, which is never stored, so a dependency on it never runs; depend on the properties it
  reads`.
- **Inherited.** A subclass runs the rules of its base class.
- **Leaving early.** `(return)`{.brix} ends one run of a rule. A rule gives no value: `A rule gives
  no value, so its body cannot return one with "(return …)"; end it early with "(return)"`.
- **Only in a class.** A rule written at the top level of a file is a syntax error.

## Live statements

An assignment or a `lazy` whose value makes a tracked read is live: it runs again each time what it
reads changes, in `main` or in any function.

```brix
(main
  (let counter (call Counter))
  (let label (call Label))
  (= (. label text) (str "count " (. counter #count)))
  (lazy doubled (* (. counter #count) 2))
  (let values (stream (* (. counter #count) 10)))
  (print (. label text))
  (print doubled)
  (print (<- values))
  (= (. counter count) 2)
  (= (. counter count) 2)
  (= (. counter count) 3)
  (print (. label text))
  (print doubled)
  (print (<- values))
  (print (<- values))
  (call (. label close))
  (call (. values close))
  (= (. counter count) 4)
  (print (. label text))
  (print doubled)
)

(type + Counter
  (prop + count _ (= 1))
)

(type + Label
  (prop + text _ (= ""))
  (fn-do + close _)
)
```

```output
count 1
2
10
count 3
6
20
30
count 3
8
```

- **Assignments.** A live assignment stores again each time what it reads changes, until the
  object it stores to closes.
- **Lazies.** A live `lazy` is worked out at its first read, and then kept up to date.
- **`let` is set once**, even when its value makes tracked reads. A local that follows state is a
  `lazy`.
- **Content.** The rows of a class's `content`, such as `(content (= echo #shown))`{.brix}, run as
  the object is made, and those that read tracked state are live.

### Lazies attached to a value

A lazy attached to a live value, with `scope` or in the `block` of a call, is part of the value,
as one attached to the value of a derived property is. It is worked out again each time the value
runs, and the value follows the tracked reads made in it.

```brix
(main
  (let cart (call Cart))
  (let label (call Label))
  (= (. label text)
    (scope (str q " at " (. cart #price))
      (lazy q (. cart #quantity))
    )
  )
  (print (. label text))
  (= (. cart quantity) 3)
  (print (. label text))
  (= (. cart price) 7)
  (print (. label text))
)

(type + Cart
  (prop + quantity _ (= 1))
  (prop + price _ (= 5))
)

(type + Label
  (prop + text _ (= ""))
)
```

```output
1 at 5
3 at 5
3 at 7
```

- **Plain or tracked.** The value follows what `q` reads whether it reads `q` plainly or as `#q`,
  and it follows the `#` reads written in the value itself too.
- **Where.** A live assignment, an attribute of a markup element and a row of a class's `content`
  each work out their attached lazies so.

### Streams of a value

`(stream (* (. counter #count) 10))`{.brix} is a channel that sends the value now, and then the
value each time it changes. Its type is `AsyncBufferedChannel`, so a reader that is not waiting gets
the values in order later, and `close` ends it. Storing the same value sends nothing. The value must
make a tracked read: `A stream of a value follows the reads written with "#", and this one has none,
so it would never change; write "#" before what it follows, as in "<c.#count>"`.
Only a statement in a function's body makes the stream, as the `let` above does. Anywhere else,
such as in a closure, a property's value, a rule or a trigger, it is an error: `A stream of a value
is made only by a statement in a function's body, …`. To move a machine as a value changes, use a
`whenever` trigger, as in `(then loading (trigger whenever #feed))`{.brix}; see
[State machines](state-machines.md#transitions). To follow changes in a property's value or a rule,
publish an event from a `whenever-rule`.

`(## readings)`{.brix} is the latest value a stream has sent, and a value that reads it follows the
stream. Before the stream sends anything, it is the type's empty value, such as 0. Named with a
`lazy`, as in `(lazy latest (## readings))`{.brix}, it is read tracked as `#latest`.

```brix
(main
  (let readings (call (of AsyncChannel Int)))
  (lazy latest (## readings))
  (lazy enabled true)
  (let board (call Board))
  (= (. board text) (str "latest " #latest))
  (for (on r readings (whenever #enabled))
    (print (str "logged " r))
  )
  (print (. board text))
  (<- readings 4)
  (<- readings 7)
  (print "sent 4 and 7")
  (call let-tasks-run)
  (print (. board text))
  (= enabled false)
  (<- readings 9)
  (call let-tasks-run)
  (print (. board text))
  (call (. readings close))
)

(type + Board
  (prop + text _ (= ""))
)

(fn-do + let-tasks-run _
  (let done (call Semaphore))
  (do-new
    (call (. done post))
  )
  (call (. done wait))
)
```

```output
latest 0
sent 4 and 7
logged 4
logged 7
latest 7
latest 9
```

- **Delivery.** A channel's values reach `##` and subscriptions in tasks of their own, after the
  publisher waits, so a value that follows `##` may skip values sent in between.
- **`whenever` clauses.** `(for (on r readings (whenever #enabled)) body)`{.brix} subscribes while
  the clause holds, and values sent while it does not are dropped. The clause must read tracked
  state, and cannot read the loop's item: `whenever clauses must read at least one tracked property
  (like #is-open)`.

| When the program needs | Read the stream with |
| --- | --- |
| Every value, as in processing each log record | A loop or a subscription, which takes each one. |
| The current state, as in showing a reading | `##`, which may skip values. |
| A history or a total | A loop that stores or adds up each value. |

## Events

A class declares an event with the type of the value it carries, and publishes values to it with
`<-`. Each subscriber runs at once, before the publisher goes on.

```brix
(main
  (let door (call Door))
  (for (on who (. door rang))
    (print (str "outside: " who))
  )
  (print "before")
  (call (. door ring) "ann")
  (print "after")
  (<- (. door rang) "bob")
)

(type + Door
  (event + rang String)
  (prop + rings _ (= 0))
  (fn-do + This _
    (for (on who rang)
      (print (str "constructor: " who))
    )
  )
  (subscribe-for (on who rang)
    (+= rings 1)
    (print (str "class body: " who ", rings " rings))
  )
  (fn-do + ring _
    (param who String)
    (<- rang who)
    (print "ring returns")
  )
)
```

```output
before
constructor: ann
class body: ann, rings 1
outside: ann
ring returns
after
constructor: bob
class body: bob, rings 2
outside: bob
```

- **Declaring.** `(event + rang String)`{.brix} declares an event in a class or an interface. A
  struct or a tuple cannot declare one: `Only a class can declare an event, which belongs to an
  object, and P is a struct, whose values are copied; declare P as a class`. The type is required;
  an event with nothing to carry is declared with `Bool`: `An event gives each subscriber a value,
  so it cannot carry Void; an event that only signals carries Bool, as in "+ event rang Bool"`. A
  private event, with `_`, can be published and subscribed only in its own module: `Event "hidden"
  is private`.
- **Publishing.** `(<- rang who)`{.brix} publishes inside the class, and
  `(<- (. door rang) "bob")`{.brix} outside it. When nothing subscribes, the value is not even
  worked out.
- **Not a value.** An event cannot be stored or passed: `"rang" is an event of Door; it cannot be
  used as a value`. Nor can it be read once: `"rang" is an event of Door, which gives each value to
  its subscribers and keeps none to read with "<-"; hear the next one with "(for (on value (. d
  rang)) …)", and end the loop with "break"`. Declared with the type `Broadcastable`, as in `(let e
  (of Broadcastable String) (. door rang))`{.brix}, it can be, and its `subscribe` and `unsubscribe`
  methods take a function. It can subscribe but not publish: `Operator "<-" is not defined for
  Broadcastable'String`. So a type can hand out an event as a `Broadcastable` to let others listen,
  and keep publishing to itself.

| Subscription | Written |
| --- | --- |
| From any code | `(for (on who (. door rang)) body)`{.brix} |
| In the class's body | `(subscribe-for (on who rang))`{.brix}, followed by its rows |
| In the class's body, with the value as `each` | `(subscribe (on rang))`{.brix}, followed by its rows |
| In the constructor | A `for` loop on the event, as in the example |
| In a class's body, to another object's event | `(subscribe-for (on who (. door rang)))`{.brix} |

- **Order.** For one publication, subscriptions made in the constructor run first, then those in
  the class's body, the base class's first, then those made outside, in the order they were made.
- **Nesting.** A value published inside a subscriber is delivered before that subscriber goes on.
  A subscription made during a publication starts with the next one.
- **Clauses.** `when`, `while` and `until` work as in other loops; see Control flow.
- **Errors** a subscriber raises go to `unhandled-error`, and the subscribers after it still run.
  A panic in a subscriber ends the task that published; see Errors.
- **One source.** A loop subscribes to one event: `A loop on a stream subscribes to one source, so
  it cannot be joined to another; write a loop for each source`.

### Ending a subscription

```brix
(main
  (let ticker (call Ticker))
  (let log (call Log ticker))
  (for (on n (. ticker ticked))
    (print (str "first " n))
    (if (== n 2)
      (break)
    )
  )
  (for (on n (. ticker ticked) (until (== n 3)))
    (print (str "second " n))
  )
  (for (in n (.. 1 4))
    (print (str "tick " n))
    (<- (. ticker ticked) n)
    (if (== n 3)
      (call (. log close))
    )
  )
  (call (. ticker close))
  (<- (. ticker ticked) 5)
  (print (str "log saw " (. log seen)))
)

(type + Ticker
  (event + ticked Int)
  (fn-do + close _)
)

(type + Log
  (prop + ticker Ticker)
  (prop + seen _ (= 0))
  (fn-do + This _
    (param ticker _)
  )
  (subscribe-for (on n (. ticker ticked))
    (+= seen 1)
    (print (str "log " n))
  )
  (fn-do + close _)
)
```

```output
tick 1
log 1
first 1
second 1
tick 2
log 2
first 2
second 2
tick 3
log 3
tick 4
log saw 3
```

- **`break`** ends the subscription, and so does a `while` or `until` clause. `until` is tested
  before the body runs.
- **Closing the event's owner** ends every subscription to its events.
- **Closing the subscriber** ends the subscriptions in its class's body.
- **Leaving a state machine's case** ends the loops written in it; see
  [Values that live in a case](state-machines.md#values-that-live-in-a-case).
- **`close`** must be declared for either: `Ticker has no method "close"`.

### Keyed and routed events

- **Keyed.** `(event + messages Int U64)`{.brix} declares an event keyed by a `U64`. A value is
  published to one address, as in `(<- ([] (. box messages) 7) 3)`{.brix}, and an address has one
  receiver: a second is a runtime error, `Duplicate keyed event receiver for 7:0`. A key is an
  integer, a `Char`, a `Bool` or an interned identity, or a tuple of an index and a generation. A
  subscription names its address: `Keyed event "messages" must be addressed with [key]`.
- **Routed.** A value that implements `RoutedEvent` passes from its target to the next owners in
  turn, until a subscriber cancels it. The public events of an `EventNode` class bubble to its
  ancestors, which subscribe to them by the class's name. Markup uses both.

## Timing and order

```brix
(main
  (let order (call Order))
  (= (. order quantity) 3)
  (print (str "after the store: total " (. order total)))
)

(type + Order
  (prop + quantity _ (= 1))
  (prop + price _ (= 5))
  (prop + total _ (= (* #quantity #price)))
  (whenever-rule #quantity
    (print (str "A: quantity " quantity ", total " total))
    (= price 4)
    (print (str "A: stored price, total " total))
  )
  (whenever-rule #total
    (print (str "B: total " total))
  )
  (whenever-rule #price
    (print (str "C: price " price))
  )
)
```

```output
B: total 15
A: quantity 3, total 15
A: stored price, total 12
B: total 12
C: price 4
after the store: total 12
```

- **Before the next statement.** A store runs every rule, derived property and live statement that
  follows it before the statement that stored goes on.
- **In turn.** What follows a property runs in the order it began to follow it. A store made while
  these run, as rule A's store to `price` is, queues what it notifies behind what is already
  waiting, so rule C runs after rule B. Each runs once for each change.
- **Events nest; stores queue.** A value published in a subscriber is delivered at once, inside it.
  A store made in a rule waits its turn.
- **Threads.** Rules and live statements run in the task that stores, and subscribers in the task
  that publishes. Values sent to a channel reach its subscribers in tasks on the publisher's
  queue; see Concurrency.
- **Settling.** Rules that keep changing what they read end the program after a million runs:
  `Live actions kept changing what they read and never settled`. A rule that stores to what it
  reads is fine when the stores stop.

## Live functions

A function marked `@live` keeps state from one run of the live value that calls it to the next. It
is for a value that changes over time on its own, as an animation does: GUI's `animated` is a live
function, whose state is the motion that moves the value, and each frame of the motion runs the
value again.

```brix
(main
  (let meter (call Meter))
  (print (. meter shown))
  (= (. meter value) 3)
  (print (. meter shown))
  (= (. meter value) 4)
  (print (. meter shown))
)

(type + Meter
  (prop + value _ (= 1))
  (prop + shown _ (= (call counted #value)))
)

(fn-do + counted String @live
  (param goal Int)
  (let found (call (of live-state Count)))
  (let count (?? found (call Count)))
  (if (not found)
    (call keep-live-state count)
  )
  (+= (. count runs) 1)
  (return (str goal ", run " (. count runs)))
)

(type + Count
  (implements Tracker)
  (prop + runs _ (= 0))
  (fn-do + attach _
    (param action LiveAction)
  )
  (fn-do + detach _
    (param action LiveAction)
  )
)
```

```output
1, run 1
3, run 2
4, run 3
```

- **State.** `(call (of live-state Count))`{.brix} finds the state of that type the function kept
  in the last run, or `null`, and `(call keep-live-state count)`{.brix} keeps one. A state is a
  `Tracker`: it is attached to the live value, and calling that value's action runs it again.
- **Several calls** in one value keep a state each, in the order of the calls.
- **Only in live values.** A live function called where nothing is read live is an error: `The live
  function "counted" is called only where a value is read live`. `keep-live-state` outside a live
  function panics: `Only a live function keeps a live state`.
