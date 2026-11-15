# Concurrency and reactivity

Yumi can wait for IO, coordinate independent tasks, publish events, and keep expressions connected to changing state. These are related capabilities, but each answers a different question: when work can proceed, who receives a value, or what should update after a change.

## At a glance

| Capability | What it gives you |
| --- | --- |
| Waiting | Ordinary calls can suspend; no async/await calling convention. |
| Tasks | `do new` explicitly starts independent scheduled work. |
| Delivery | Choose rendezvous, queues, or subscriptions according to who should receive each item. |
| Reactive expressions | `<#value>` produces updates; `##source` reads a latest value. |
| Ownership | Closing, cancellation, and subscription lifetime remain explicit design choices. |

## Waiting without changing the call syntax

A function can call a network operation that suspends the current task while waiting for IO.
Its callers do not need a different spelling just because that wait occurs somewhere below
them. This fragment assumes a `URL` named `url` and imports from `/net/http`:

:::example-pair

- [Caller](#source-network-helper--caller): Invoke the helper normally and print its answer.
- [Helper](#source-network-helper--helper): No `async` declaration is needed.
- [Network request](#source-network-helper--request): This call can suspend for IO. Neither this call nor its caller needs `await`.

```yumi #network-helper
⟦caller⟧-- page-body(url)⟦/caller⟧

━━━━━━━━━━━━━━━━━━━━
⟦helper⟧+ page-body String
    - url URL⟦/helper⟧

page.text

.. page = ⟦request⟧http-get(url)⟦/request⟧
```

:::

This makes it easier to introduce a helper or replace a local source with a remote one.
The operation still has a timing contract: a call can take time, fail, or allow other tasks
to make progress before it returns. State observed before an IO call may need to be reconsidered
afterward. The absence of a marker does not imply uninterrupted execution.

## Tasks make concurrent work explicit

`do new` starts work in another task. Channels let the tasks exchange values:

:::example-pair

- [Rendezvous channel](#source-task--channel): Connect producer and consumer.
- [Create work](#source-task--task): Start the producer in another task.
- [Complete the source](#source-task--finish): The consumer can finish after reading `42`.

```yumi #task
⟦channel⟧. results Int<>⟦/channel⟧
⟦task⟧do new⟦/task⟧
    results <- 21 * 2
    ⟦finish⟧results.close()⟦/finish⟧

for value in results
    -- value
```

:::

Yumi tasks are lightweight scheduled work, often called green threads. Native workers provide
another level of execution, and the language includes parallel task forms. A program can
overlap operations that spend time waiting or place work on native threads when it needs
parallel execution.

Those are separate decisions. Starting a task does not promise one new operating-system thread
per operation, and a loop over a stream does not implicitly parallelize its body. The source
of a value, its delivery mechanism, and the placement of the work can be chosen independently.

`do new` also gives the task's `Task`, which can cancel the task, wait for it, and tell how it
ended: still `running`, `finished`, `raised` where an error left its body, `panicked`, or
`cancelled`. A `Task` holds no result; a task delivers what it makes through a channel or a
captured local. A `catch` written after the block, at its indentation, handles a panic that
ends the task, after the task's cleanups have run:

```yumi
. drain = do new @large-stack
    read-errors(process)
catch failure
    -- "drain failed: $failure.message"
drain.wait()
```

A panic no such `catch` handles crashes the process. `@large-stack` gives the task the main
task's larger stack, and `@priority` schedules it ahead of others. A task's cleanup can read the
panic ending it with `Task().failure`, and a panic tells its `stack` and when it was `raised-at`.

## Shared state and lifetimes

Channels give concurrent code a way to communicate values explicitly. Mutexes and other
synchronization facilities are available when tasks or threads coordinate shared state.
Task-local and thread-local declarations distinguish state associated with one task from
state associated with its executing native thread.

A mutex held across an operation that can wait has different consequences from one held
around a short update. Moving work to another thread also raises questions that cooperative
task scheduling alone does not settle. Clear ownership is useful in both cases: who may
change the state, and what event makes that change visible to another participant?

Tasks can also be canceled. A cancelled task ends at its next wait, such as a channel read, a
sleep or a wait for another task: its cleanups run, a wait inside one of them ends that cleanup,
and its spawn-site `catch` does not run, since cancellation is not a failure. Its `Task` then says
it ended `cancelled`. Cancellation, normal completion, and a consumer losing interest are related
but distinct events. A producer should have a way to finish and release resources, and a consumer
should not assume that dropping its own result automatically ends every independent task behind
it.

## State transitions

Yumi includes state-machine constructs for behavior organized around named states and
transitions. This is useful when the question is what an object may do next: a connection
can be opening, active, or closing; a component can wait for input before continuing an action.

An explicit state model keeps those transitions together. Events and tasks can trigger
progress while the machine describes which transitions belong to the current state.
[State machines](state-machines.md) covers the declarations, payloads, nested states, and
subscription lifetimes, including how event transitions run to completion before a
publication returns.

## Channels and subscriptions

A channel makes the connection between a producer and consumer explicit. Writing an item
uses `destination <- value`; a readable channel can supply an item through `<- source`.
`destination <-* source` writes a source's items to the destination.

The core library provides several delivery models:

| Type | Main behavior |
| --- | --- |
| `SyncChannel'Item` | Rendezvous between a writer and reader; operations can wait for their counterpart |
| `SyncBufferedChannel'Item` | Queue items for readers; an empty open channel can wait for input |
| `AsyncChannel'Item` | Publish to current subscribers without retaining a pre-subscription history |
| `AsyncBufferedChannel'Item` | Buffer publications while there are no subscribers, then deliver buffered values on subscription |
| `AsyncOnDemandChannel'Item'Used` | Associate active subscriptions with acquisition and release of an underlying resource |

The names do not divide the stream world into “can suspend” and “cannot suspend.” A
`SyncChannel` can suspend the current task while waiting. A `Broadcastable` can invoke
subscribers synchronously. Delivery, buffering, and task scheduling are separate questions.

Buffered also does not imply bounded. If producers outrun consumers, queued data can grow.
A rendezvous channel directly couples producer progress to consumer readiness; buffering
relaxes that coupling by spending memory.

### Iteration versus subscription

`for item in source` consumes a traversal. `for item on source` installs a subscription.
The latter lets subsequent code continue while the handler remains interested in arrivals.
The lifetime and ownership of that subscription matter.

This example uses a declared event, whose publication delivers to its subscribers synchronously:

:::example-pair

- [Subscribe](#source-subscription--subscribe): Install a handler, then continue with subsequent code.
- [Publication](#source-subscription--publish): This event delivers during the publishing call: `before`, `heard 42`, `after`.
- [Declared event](#source-subscription--event): Expose a stream source for subscribers.

```yumi #subscription
. source = Source()
⟦subscribe⟧for value on source.changed⟦/subscribe⟧
    -- "heard $value"

-- "before"
⟦publish⟧source.emit(42)⟦/publish⟧
-- "after"

━━━━━━━━━━━━━━━━━━━━
+ Source

⟦event⟧+ event changed Int⟦/event⟧

────────────────────
+ do emit
    - value Int

changed <- value
```

:::

Multiple subscribers can each observe a publication. Multiple readers of a queue instead
take items from that queue; they do not automatically each receive a copy. A subscription
can stop with `break` from its handler, and low-level subscribers can explicitly unsubscribe.
Class-owned subscriptions and event endpoints also have owner lifetimes.

### Closing is part of the protocol

Close a finite channel when the producer finishes; otherwise a consumer may wait indefinitely.

Closing delivers first, then ends. Every item a channel accepted before `close()` still reaches
the readers or subscribers it was headed for, even when the close comes in the same turn as the
write. After that, readers see the end of the stream and subscriptions end. A write after closing
does what it did before: `SyncChannel` and `AsyncChannel` panic, and the other channels drop the
item.

| Source | After closing |
| --- | --- |
| `SyncChannel` | A writer still waiting hands its item to the next reader; then reads end |
| `SyncBufferedChannel` | Reads drain the queued items, then end |
| `AsyncChannel` | Each subscription gets the items queued for it, then ends |
| `AsyncBufferedChannel` | Subscriptions get their queued items; reads drain the buffer kept for future subscribers; no new subscriptions |
| `AsyncOnDemandChannel` | Subscriptions get their queued items; the underlying resource is released at the close |

Stopping a subscription is different from closing its channel. A subscription that stops, by
`break` or a `while` or `until` clause, by its owner going away, or by an explicit unsubscribe,
drops any items still queued for it. That holds even while its closed channel is still delivering
to other subscribers.

Stopping a consumer does not automatically stop every independent producer task or close a
socket owned elsewhere. If a producer might otherwise keep working or block on a write,
give the consumer and producer an explicit completion or cancellation arrangement.

## Reactive streams and latest values

Streams can represent changes to an expression, not only explicit sends or generator yields.
The expression form `<expression>` creates a stream, and tracked reads identify the state
whose changes should cause new values to be produced:

:::example-pair

- [Tracked expression](#source-reactive-expression--expression): `#count` identifies the dependency; the initial value is `20`.
- [Update](#source-reactive-expression--change): Changing the binding produces the next value, `30`.
- [Lifetime](#source-reactive-expression--close): Finish observing when the consumer no longer needs updates.

```yumi #reactive-expression
.. count = 2
. values = ⟦expression⟧<#count * 10>⟦/expression⟧
-- <- values
⟦change⟧count = 3⟦/change⟧
-- <- values
⟦close⟧values.close()⟦/close⟧
```

:::

Tracked property reads use forms such as `model.#count`. Tracking applies to supported
state locations; it is not a promise that any arbitrary getter can be observed automatically.

The `##` form goes in the other direction: it makes a stream's latest value available to
a surrounding expression. A fragment such as `.. current = ##updates` gives a name to that
latest-value expression. Inside a live declaration, stream arrivals can cause dependent
expressions to be re-evaluated.

| Requirement | Model |
| --- | --- |
| Process each log record or transaction | Consume every item |
| Show the current state when a display updates | Read the latest value |
| Retain a history or compute an aggregate | Store or reduce the stream explicitly |

A latest-value read does not guarantee that rendering observes every intermediate value.

Reactive streams have subscription lifetimes too. Arrange an owner or explicit cleanup for
them, especially when the source outlives the display or computation observing it. Detailed
markup and stylesheet behavior belongs in the corresponding UI documentation; the stream
model supplies their sequence of updates.

### Live functions

A function annotated `@live` keeps state across the runs of the live value that calls it. It finds
its state with `live-state`, among the trackers the value kept from its last run, and keeps a new
one with `keep-live-state`. The state is a `Tracker`, so the value attaches it while it runs and
detaches it when it stops, and the state can run the value again by calling the action it was
attached to. Two calls in one value keep a state each, in the order they run. A call written where
no value is read live is an error.

```text
+ do counted String @live
    - goal Int

. found = live-state'Count()
. count = found ?? Count()
if !found
    keep-live-state(count)
count.runs += 1
= "$goal, run ${count.runs}"
```

GUI's `animated(target, motion)` is a live function: its state is the run that moves the value,
and each frame of the run runs the value again.
