# Streams

A stream is a source of values that a program can process in sequence. Those values might
already be stored in a list, be calculated by a generator, or arrive through a connection.
The consumer can use the same function in each case.

This is the central idea behind Yumi's streams: **describe the operation in terms of the
values it consumes, without requiring a particular storage layout or delivery schedule.**
Synchronous and asynchronous input do not need separate versions of that operation.

The abstraction connects several parts of the language. Loops consume streams. Generators,
comprehensions, and stream pipelines produce them. Stream patterns recognize sequences and
turn them into structured results. Channels connect producers to consumers, and reactive
expressions expose changing values as streams. The standard library builds selection,
aggregation, sorting, and combination on the same foundation.

## At a glance

| Need | Yumi's model |
| --- | --- |
| Accept stored or arriving values | `<Item>` accepts the whole stream family |
| Consume successive values | `for`, direct reads on readable sources, or a library consumer |
| Produce values | Generators, comprehensions, and [pipelines](stream-pipelines.md) |
| Recognize records | [Stream patterns](stream-pattern-matching.md) |
| Coordinate arrivals | [Tasks, channels, events, and reactive expressions](concurrency-and-reactivity.md) |
| Know the cost | Establish completion, replay, buffering, and ownership for the concrete source |

## One function, several kinds of stream

A parameter of type `<Int>` accepts the stream family for integers. This function reads at
most three items and adds them. Its callers supply a list, a range, a generator, an immutable
array, and a buffered broadcaster:

```yumi #one-consumer
/core/array-builder

⟦stored⟧-- total-first-three([1, 2, 3, 100])
-- total-first-three(1 .. 10)⟦/stored⟧
-- total-first-three(numbers())
-- total-first-three(ImmutableArray([1, 2, 3, 100]))

. published = AsyncBufferedChannel'Int()
published <- 1
published <- 2
published <- 3
⟦published⟧-- total-first-three(published)⟦/published⟧
published.close()

━━━━━━━━━━━━━━━━━━━━
+ do total-first-three Int
    ⟦signature⟧- values <Int>⟦/signature⟧

. total = 0
. count = 0
for value in values
    total += value
    count += 1
    ⟦bound⟧if count == 3
        break⟦/bound⟧
= total

━━━━━━━━━━━━━━━━━━━━
+ do numbers

⟦generator⟧<= 1
<= 2
<= 3
<= 100⟦/generator⟧
```

- [Stored inputs](#source-one-consumer--stored): A list and a range use the same call.
- [Published input](#source-one-consumer--published): The buffered broadcaster retains these pre-subscription values.
- [One signature](#source-one-consumer--signature): Accept successive integers without requiring a list or immediate delivery.
- [Consumer boundary](#source-one-consumer--bound): Stop after three items; the broadcaster need not close first.
- [Generated input](#source-one-consumer--generator): Yielded values participate too. Every call prints `6`.

Nothing in `<Int>` says that reading is immediate. A source may have to wait. The loop still
has the same shape, and the function still has the same signature. Waiting affects when
the answer is available, not how the consumer describes its calculation.

## The stream family

`<Item>` is shorthand for `Stream'Item`. The core library defines `Stream` as an `Or` family
of supported source types. Its declaration is:

```yumi
━━━━━━━━━━━━━━━━━━━━
+ Stream'T is CursorIterable'T || Indexable'T || Iterable'T || Broadcastable'T || ImmutableArray'T
```

Each alternative supplies items in its own way. A source may support more than one.

| Alternative | How it supplies items | Typical use |
| --- | --- | --- |
| `CursorIterable` | Separate iterator state, advanced by the source | Strings, ranges, custom cursors |
| `Indexable` | Length and indexed access | Lists, mutable arrays, stored sequences |
| `Iterable` | Optional reads and iteration lifecycle hooks | Generators and synchronous channels |
| `Broadcastable` | Subscription and publication | Async channels and events |
| `ImmutableArray` | Direct access to immutable array storage | Fixed stored results |

The compiler and adapters make operations work across this family without requiring one large common interface.

### CursorIterable

`CursorIterable'Item'State` separates the source from the state of an iteration. The source
provides an initial state and an operation that returns the next state together with an
optional item. Absence indicates the end of that traversal.

This is useful when iteration has a natural cursor that need not be stored inside the source
itself. A string, for example, can advance through its encoded characters using a position.
Ranges also support cursor iteration. A new traversal can obtain a new initial state.

The state type need not be the item type. A source might yield characters while keeping a
byte offset, or yield records while keeping several counters. The final section shows a
complete implementation.

### Indexable

`Indexable'Item` provides a length and indexed access. Lists, strings, ranges, and mutable
arrays can participate through this interface.

An indexable source can be traversed without a consumable read cursor. It also makes
operations such as accessing the first item possible without walking through the input.
Being indexable does not, by itself, guarantee that indexing is constant-time or that the
underlying values never change.

Some sources support more than one alternative. A range or string does not have to choose
between being cursor-iterable and indexable; both capabilities can be useful to consumers.

### Iterable

`Iterable'Item` is the read-based interface. It extends `Readable'Item`, whose `<-` operation
returns an optional item. It also supplies iteration lifecycle hooks.

Generators and synchronous channels are examples. The next read may resume a producer,
retrieve an already buffered value, or wait for another task. An iterable can carry progress
inside itself, so do not assume that asking to iterate it again restarts the source.

The broad shorthand `<Item>` does **not** mean only `Iterable'Item`. Use the latter spelling
when a function specifically needs the read-based interface rather than the entire family.

### Broadcastable

`Broadcastable'Item` provides subscription and unsubscription. Values are delivered to
subscribers instead of existing only for one reader to remove from a queue.

Async channels and event endpoints can supply this interface. Iteration can adapt a
broadcaster into a readable sequence, but the source's publication and lifetime rules still
matter. In particular, subscribing does not inherently mean receiving a replay of everything
published earlier.

An event endpoint can expose `Broadcastable'Item` without exposing a writable channel.
The right to observe a source and the right to publish into it are separate capabilities.

### ImmutableArray

`ImmutableArray'Item` is explicitly included in the family. A consumer can work with immutable
array storage directly, without requiring the author to wrap it in a generator first.

Immutability describes the array's storage interface. It does not mean that every object
referenced by an item is deeply immutable.

### What the common type does and does not promise

All five alternatives can supply items to stream operations. They do not all promise the
same delivery behavior:

| Question | What to establish about the particular source |
| --- | --- |
| Can a read wait? | Whether more values can arrive later |
| Can it be traversed again? | Whether it can start a fresh traversal or has already advanced |
| Do two consumers see the same values? | Independent traversal, competing reads, or broadcast delivery |
| Does it retain earlier values? | Storage, buffering, or an explicit replay policy |
| How does it end? | Exhaustion, generator completion, channel closure, or unsubscription |
| Can it be indexed? | Whether it actually offers `Indexable`, not merely `Stream` |

These questions describe the source. They are not reasons to duplicate a transformation
into separate synchronous and asynchronous implementations.

## Shorthand and signatures

### Type notation

| Spelling | Meaning |
| --- | --- |
| `<Int>` | `Stream'Int` |
| `<Item>` | A stream family parameterized by `Item` |
| `<<Int>>` | A stream whose items are themselves integer streams |
| `[Int]` | `List'Int`, a stored list |
| `Int[]` | `Array'Int`, contiguous mutable array storage |
| `Int<>` | `SyncChannel'Int` |

The position of the angle brackets matters. `<Int>` describes an accepted family of sources.
`Int<>` describes a particular channel type. A parameter typed `<Int>` does not allocate a
channel just because it is called a stream.

A generic transformation can preserve its source's item type while accepting different
source implementations:

:::example-pair

- [Infer the item type](#source-generic-consumer--calls): The calls produce `1, 1, 2, 2` and `a, a, b, b`.
- [Preserve the relationship](#source-generic-consumer--generic): The same declaration works for integers and characters.
- [Accept the family](#source-generic-consumer--input): The body needs successive items, not knowledge of the caller's storage.

```yumi #generic-consumer
⟦calls⟧-- List(twice([1, 2]))
-- List(twice("ab"))⟦/calls⟧

━━━━━━━━━━━━━━━━━━━━
+ do ⟦generic⟧twice'Item⟦/generic⟧
    - values ⟦input⟧<Item>⟦/input⟧

for value in values
    <= value
    <= value
```

:::

Use a narrower signature when it expresses a real requirement. An algorithm that must jump
to arbitrary positions can ask for `Indexable'Item`. A component that installs subscriptions
can ask for `Broadcastable'Item`. A function that only needs a sequence should usually accept
`<Item>` and leave the representation to its caller.

### Expression notation

Angle brackets around an **expression** mean something different. `<#count * 2>` creates a
reactive stream of the expression's values, with `#count` marking a tracked dependency.
It is not a cast to the stream type and not another spelling of a one-element list.

The type and expression forms meet naturally: a reactive expression produces a source that
can be passed to a stream-oriented function. The details of tracked reads appear in
[Concurrency and reactivity](concurrency-and-reactivity.md#reactive-streams-and-latest-values).

## Consuming values

### Iteration

`for value in source` processes successive items. The source determines how the next item
becomes available. The loop ends when the source is exhausted, or when the body exits early.

Loop clauses can filter or bound a traversal. This example skips values that do not satisfy
`when`:

```yumi #filtered-loop
for value in ⟦source⟧1 .. 6⟦/source⟧ ⟦filter⟧when value % 2 == 0⟦/filter⟧
    -- value
```

- [Input](#source-filtered-loop--source): Visit an inclusive range.
- [Filter](#source-filtered-loop--filter): Only even values reach the body, printing `2`, `4`, and `6`.

It prints `2`, `4`, and `6` on separate lines. `while` and `until` clauses can end a traversal
according to a condition. An explicit `break` is useful when the stopping decision belongs
inside the body.

### Reading one item

For a readable source, `<- source` obtains one optional item. This generator supplies two
values and then completes:

```yumi #direct-read
. source = numbers()
⟦read⟧-- <- source
-- <- source⟦/read⟧
-- ⟦end⟧(<- source) ?? -1⟦/end⟧

━━━━━━━━━━━━━━━━━━━━
+ do numbers

<= 10
<= 20
```

- [Read an item](#source-direct-read--read): The first two reads produce `10` and `20`.
- [Observe completion](#source-direct-read--end): The generator's final read is absent, so the fallback produces `-1`. Zero or `false` would still be present items.

For a waiting channel, temporary lack of input is different from completion: the read can
suspend until a sender provides an item or the channel closes. Not every `Readable` has
identical waiting behavior, however. Use the concrete source's contract rather than treating
every absent direct read as proof that no future publication is possible.

### Collecting and joining

Materialization means consuming values and storing the resulting collection:

```yumi #materialize
. source = 1 .. 4
. values = ⟦store⟧List(source)⟦/store⟧
-- values
-- ⟦join⟧", ".join(values)⟦/join⟧
```

- [Collect](#source-materialize--store): Consume the finite source into reusable storage.
- [Make text](#source-materialize--join): Consume stored values to produce `1, 2, 3, 4`.

Collecting is a policy decision. It uses memory for the result, waits for the input to finish,
and turns a traversal into reusable stored data. An unbounded or never-closed source cannot
be collected to completion. Restrict such a source before applying a consumer that needs
its end.

### Waiting does not change the function

Here the same total function works on a list and a channel fed by another task:

:::example-pair

- [Separate producer](#source-waiting-input--task): Create another task explicitly; the consumer loop is not implicitly parallel.
- [End the sequence](#source-waiting-input--close): `total` needs to know when no more values will arrive.
- [Ordinary call](#source-waiting-input--call): The channel call may wait; both calls print `6`, without `await`.
- [Ordinary function](#source-waiting-input--signature): The same helper accepts the list and the channel, without an `async` declaration.

```yumi #waiting-input
-- total([1, 2, 3])

. incoming Int<>
⟦task⟧do new⟦/task⟧
    incoming <- 1
    incoming <- 2
    incoming <- 3
    ⟦close⟧incoming.close()⟦/close⟧

⟦call⟧-- total(incoming)⟦/call⟧

━━━━━━━━━━━━━━━━━━━━
⟦signature⟧+ do total Int
    - values <Int>⟦/signature⟧

. result = 0
for value in values
    result += value
= result
```

:::

## Producing values

### Generator functions

A generator uses `<=` to supply one item and `<<=` to supply the items from another source:

:::example-pair

- [Yield one](#source-generator--single): Supply an individual item.
- [Yield a source](#source-generator--many): Supply all items from another source. The complete output is `1, 2, 3, 4, 5`.

```yumi #generator
-- List(sequence())

━━━━━━━━━━━━━━━━━━━━
+ do sequence

⟦single⟧<= 1⟦/single⟧
⟦many⟧<<= 2 .. 4⟦/many⟧
<= 5
```

:::

An absent optional value can be omitted with `<=?`. Completion means that there are no more
items to yield. A stored generator represents an ongoing traversal; call its producing
function again when you need a fresh instance rather than assuming the old one rewinds.

### Comprehensions

A comprehension describes an output expression over an input source:

```yumi #comprehension
. squares = ⟦output⟧each * each⟦/output⟧ ⟦source⟧in 1 .. 5⟦/source⟧ ⟦condition⟧when each % 2 != 0⟦/condition⟧
-- List(squares)
```

- [Output expression](#source-comprehension--output): Square each participating input.
- [Source](#source-comprehension--source): Visit the inclusive range.
- [Selection](#source-comprehension--condition): Only odd values participate, producing `1, 9, 25`.

Use a generator when producing the sequence needs a more substantial procedure. Use a stream
pipeline when the sequence is best described as contributions followed by several stages.
These forms can feed one another and the same consumers.

## Stream pipelines

Stream pipelines assemble and transform sequences. A source can contribute individual items or
expand another stream, then filter, map, or observe the values that pass through it. Stages run
as their output is consumed; the source itself may already be active independently.

The [Stream pipelines](stream-pipelines.md) chapter covers the construction markers, `each`,
optional contributions, stage ordering, and the library operations for aggregation, selection,
combination, and buffering. Those operations work with the source family described here.

## Stream pattern matching

A stream can also be the input to a grammar. A pattern can recognize a record, capture pieces
of it, and return a structured value. The same grammar can work over stored text or characters
arriving through a channel, without a separate asynchronous parser.

[Stream pattern matching](stream-pattern-matching.md) covers named rules, captures, repetition,
lookahead, and parsing non-text items. It also explains a significant distinction: matching a
whole input may need to wait for its end, while recognizing successive records can continue
on a connection that stays open.

## Concurrency and reactivity

Channels and reactive expressions are sources in the same stream family. A loop can consume
their values without prescribing how a producer is scheduled. A publication can also feed
subscribers, and a latest-value expression can make changing state available to another
calculation.

[Concurrency and reactivity](concurrency-and-reactivity.md) explains tasks, channel delivery
models, events, subscriptions, tracked reads, and latest values. That chapter also covers the
closure and ownership rules that distinguish a queue from a broadcaster.

## Completion, errors, and cleanup

- **Errors can arrive during consumption.** Catching a factory call alone does not cover later reads; recovery belongs around the work that pulls values.
- **Traversal has a lifecycle.** `Iterable` supplies `begin-iterating` and `end-iterating`; the broadcaster adapter uses them to subscribe and unsubscribe.
- **Cleanup has an owner.** A generator can register `do finally`. Ending its traversal does not automatically destroy a caller-owned socket or stop an independent producer.

See [Errors](errors.md) for propagation, recovery, and cleanup syntax, and
[Concurrency and reactivity](concurrency-and-reactivity.md#closing-is-part-of-the-protocol)
for channel closure. For every source, ask what ends this traversal and who owns the producer.

## Replay, storage, and cost

### A stream is not a promise of replay

Keep these cases separate:

- A list can be traversed again because its items are stored.
- A cursor source can start another traversal from a new initial state.
- A generator instance retains its current progress.
- A channel read can remove an item that another reader will no longer see.
- A broadcaster can publish values that late subscribers never receive.

Calling a generator-producing function again can create a new traversal. Retaining and
reusing the already-created generator is a different operation. Likewise, materializing
a source once and iterating the resulting list differs from re-running the producer twice.
The latter may repeat IO or see changed state.

### Incremental does not mean constant memory

A plain map or filter can often pass one item at a time. Other operations retain information:

| Operation or source | Possible retained state |
| --- | --- |
| `scan` | The accumulator, whose own size may grow |
| `distinct` | Previously seen values or keys |
| `pairwise` | The preceding item |
| Grouping buffers | Items awaiting a complete group |
| Sorting or reversal | The complete finite input |
| Replaying a one-pass input | A materialized sequence |
| Queued channels | Items not yet consumed |
| Pattern lookahead and backtracking | Input needed to retry or complete recognition |

The source can also retain storage independently of the pipeline. Holding a small result
may keep an owner alive, and an abandoned consumer can leave an independent producer active.
Consider the entire producer-adapter-consumer lifetime when investigating retained memory.

### Compilation can remove intermediate machinery

The compiler can fuse eligible producers and consumers, so a chain of stream operations
does not necessarily allocate one iterator object or call one callback for every stage.
An escaping, independently resumable generator can still need its own state.

These optimizations do not change the intended sequence of effects, error propagation, or
cleanup. They also do not make sorting incremental or erase buffering required by the
algorithm. Treat performance as something to measure for the actual source and consumer,
rather than as a property guaranteed by the word “stream.”

## Implementing a source

The simplest producer is usually a generator. Implement a protocol directly when the source
has a representation or lifecycle that deserves one.

A cursor source provides the initial state and a next-state operation. This source yields
`2`, `5`, and `8`:

```yumi #cursor-source
-- List(Steps(3))

━━━━━━━━━━━━━━━━━━━━
+ Steps is Tuple
    ⟦protocol⟧is CursorIterable'Int'Int⟦/protocol⟧

+ limit Int

────────────────────
⟦initial⟧+ get initial-iterator-state Int

0⟦/initial⟧

────────────────────
+ do next-iterator (Int, Int?)
    - state Int

. absent Int?
if state >= limit
    ⟦done⟧= (state, absent)⟦/done⟧
⟦next⟧= (state + 1, 2 + state * 3)⟦/next⟧
```

- [Item and state types](#source-cursor-source--protocol): Both are integers here, but their roles are separate.
- [Fresh traversal](#source-cursor-source--initial): Start with state zero.
- [Completion](#source-cursor-source--done): An absent item reports exhaustion.
- [Advance and yield](#source-cursor-source--next): Return the new cursor and the current item. `List` collects `2, 5, 8` without special knowledge of `Steps`.

For a read-based source, implement the `Iterable` contract, including the optional read and
the iteration lifecycle hooks. For random access, implement `Indexable`. For publications,
implement `Broadcastable` with clear subscription, closure, and delivery behavior. A bare
`Readable` is useful for direct reads but is not, by itself, one of the `Stream` alternatives.

The goal is not to make every source look like the same object internally. It is to make
the operations that only need a sequence reusable across sources. Storage, waiting, parsing,
and delivery can then vary while the function expressing the actual calculation stays the same.
