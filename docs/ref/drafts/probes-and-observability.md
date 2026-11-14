# Probes and observability

Yumi includes probes for observing a running program. A probe can record that something
happened, expose a value, group related activity, or make an object available for inspection.
The goal is to preserve useful context about execution instead of reducing every observation
to an isolated line of text.

This matters particularly for programs with tasks, streams, and live interfaces. The question
is often not just what value was returned, but which task produced it, what the object was
doing, and what happened before another component stopped making progress.

## At a glance

| Capability | What it gives you |
| --- | --- |
| Event / value | Record what happened and the data that explains it. |
| Groups | Connect the beginning and end of related activity. |
| Object identity | Follow one request or component across modules and tasks. |
| Snapshot / live inspection | Inspect recorded state or a still-present object. |
| Counters / profiling | Measure frequency and accumulated work. |
| Selection | Enable relevant observations without relying on them for application behavior. |

## Observations have a place in the source

A probe names an observation at the point where it helps explain the program. Here the application computes a value, then exposes it to an enabled probe:

:::example-pair

- [Application work](#source-observation--work): Compute the value independently of whether observation is enabled.
- [Named probe](#source-observation--name): Associate the observation with `http.value` and this source location.
- [Observed value](#source-observation--value): Expose `42` to the selected probe. The print remains ordinary application behavior.

```yumi #observation
⟦work⟧. value = 21 * 2⟦/work⟧
⟦name⟧//http.value⟦/name⟧ ⟦value⟧-> value⟦/value⟧
-- value
```

:::

The `//` form is executable probe notation. Choose names at the level that explains the work: a parser might record recognized records, while a channel implementation records waiting readers.

## Observation can be selected

Probe selection lets an investigation focus on a relevant area. A program can contain useful
instrumentation without every run collecting every possible observation. Probe-only expressions
are not ordinary application work that should be relied upon to run when the probe is disabled.

This distinction allows an observation to include a derived value without making that value
part of the program's result. It also means effects required by the application belong in
ordinary code, not hidden inside an optional observation.

The useful design goal is to keep probes near the event they explain while retaining control
over what an investigation records.

## Objects make a timeline easier to interpret

An object identity can connect observations that occur far apart in the source. A request
might be created in one module, queued in another, and completed by a task elsewhere. Seeing
those observations as one request's history is more informative than matching text by eye.

Snapshots expose state at a point in time. Inspection support can also make information
available from a live object when the runtime and object provide the relevant capability.
For a channel, useful state might include buffered values, waiting activity, or subscriptions.

The distinction between a snapshot and live inspection matters. One describes what was
recorded at an earlier point; the other asks about an object that is still present. A useful
tool should make the origin and time of that information visible.

## Counts and durations answer different questions

| Evidence | Question it answers |
| --- | --- |
| Event trace | In what order did the work happen? |
| Counter | How often did an operation run? |
| Duration / profile | Where did time or work accumulate? |
| Snapshot | What state existed at the recorded moment? |
| Live inspection | What can the object report now? |

Group boundaries and counters can describe a batch without recording every item as a large snapshot:

:::example-pair

- [Begin a group](#source-measurements--start): Associate the following activity with one processing group.
- [Count work](#source-measurements--count): An enabled counter advances for each visited item.
- [End the group](#source-measurements--end): Mark completion of the related activity.

```yumi #measurements
⟦start⟧//+batch.process⟦/start⟧
for value in [10, 20, 30]
    ⟦count⟧//batch.items ++⟦/count⟧
    -- value
⟦end⟧//-batch.process⟦/end⟧
```

:::

Probe names and object associations connect measurements to the program's concepts. The profile example in the repository uses the same group and counter forms for parsing and rendering work.

## Following a stream through the program

Consider a producer writing work into a channel and a consumer updating an interface. Useful
observations might be:

1. A work item is created, with its identity and input size.
2. The item is queued, together with the channel's current state.
3. A task begins processing it.
4. The result is published.
5. A component accepts the result or ignores it because its state has changed.

Those observations can explain a delay that would be difficult to locate with only the
producer's input and the final display. They also separate waiting from processing and
publication from consumption.

The chapter on [Concurrency and reactivity](concurrency-and-reactivity.md) explains the
behaviors these observations describe. Probes provide a way to inspect those behaviors in
a particular run.

## Observation is still work

- **Choose the scope.** Large snapshots and frequent events have different costs from occasional counters.
- **Account for instrumentation.** Collection uses time and storage; interpret measurements with the enabled observations in mind.
- **Keep expectations separate.** Tests establish expected behavior. A trace explains one run and does not guarantee the same ordering in every run.

For someone evaluating Yumi, the significant capability is that observation has language and
runtime support. Tasks, objects, and streams can remain recognizable in the evidence collected
about them, rather than disappearing behind a wall of unrelated print statements.
