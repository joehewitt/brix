# Stream pipelines

A stream pipeline describes a sequence of values, one contribution per line. Some lines
contribute a single value, some contribute an existing sequence, and some contribute only when a
value is present. Together, they let the shape of the code follow the shape of the result.

## At a glance

| Capability | What it gives you |
| --- | --- |
| Construct | `*` contributes an item; `**` expands a source; add `?` to omit absent contributions. |
| Transform | `?>` filters, `*>` maps, and `?*>` maps present results. |
| Observe | `*\|>` performs an effect and forwards the same item. |
| Consume | A loop, `List`, `join`, or another consumer determines when stages run. |
| Combine and summarize | Library operations add reduction, selection, combination, buffering, and sorting. |

## Contributions become a stream

:::example-pair

- [Consumer](#source-contributions--consumer): Collect the resulting stream into a stored list.
- [One item](#source-contributions--item): `*` contributes one value.
- [Expand a source](#source-contributions--source): `**` contributes every item from this inclusive range. The full result is `1, 2, 3, 4, 5`.

```yumi #contributions
-- ⟦consumer⟧List⟦/consumer⟧
    ⟦item⟧* 1⟦/item⟧
    ⟦source⟧** 2 .. 4⟦/source⟧
    * 5
```

:::

## Building a sequence

Use `*` when the expression to its right is one item in the result. Use `**` when that expression
contains items to include individually.

This distinction matters for collections. `* [2, 3]` contributes a list as one item; `** [2, 3]`
contributes the numbers `2` and `3`. An expansion marker expands its immediate operand; it does not
recursively flatten everything inside it.

For a short, fixed list, `[1, 2, 3]` is usually enough. Stream pipelines become useful when a sequence mixes
fixed items, optional items, and values drawn from elsewhere.

```yumi #mixed-inputs
. include-debug = true
. extras = ["timing", "memory"]
-- List
    ⟦fixed⟧* "status"⟦/fixed⟧
    ⟦optional⟧*? "debug" if include-debug⟦/optional⟧
    ⟦extras⟧** extras⟦/extras⟧
    * "done"
```

- [Fixed entry](#source-mixed-inputs--fixed): Always include the status item.
- [Conditional entry](#source-mixed-inputs--optional): Omit `debug` when the condition is false.
- [Additional entries](#source-mixed-inputs--extras): Expand the list in place. The result is `status, debug, timing, memory, done`.

## Optional contributions

Adding `?` to a construction marker makes its operand optional:

| Form | Contribution |
| --- | --- |
| `* value` | One item |
| `** values` | Every item from a sequence |
| `*? optional-value` | One item if the value is present |
| `**? optional-values` | Every item from a sequence if the sequence is present |

An expression such as `"debug" if include-debug` produces an optional value. `*?` includes its
present value or contributes nothing when it is absent. It does not insert an empty placeholder.

`**?` applies the same choice to a whole sequence:

```yumi #optional-source
. extras = [2, 3] if false
-- List
    * 1
    ⟦optional⟧**? extras⟦/optional⟧
    * 4
```

- [Optional expansion](#source-optional-source--optional): An absent sequence contributes nothing. The result is `1, 4`; with a true condition it would be `1, 2, 3, 4`.

## Transforming the stream

Once a stream has been described, subsequent stages can filter or transform its items. Inside
each stage, `each` names the item arriving at that stage.

:::example-pair

- [Input](#source-stages--input): Begin with six integers.
- [Filter](#source-stages--filter): Keep even values.
- [Map](#source-stages--map): Multiply each retained value by ten, producing `20, 40, 60`. Here `each` is the filter's output.

```yumi #stages
-- List
    ⟦input⟧** 1 .. 6⟦/input⟧
    ⟦filter⟧?> each % 2 == 0⟦/filter⟧
    ⟦map⟧*> each * 10⟦/map⟧
```

:::

Read these stages from top to bottom. Every stage receives the output of the preceding stage,
so changing their order can change the result. After a map, `each` refers to the mapped value.

| Stage | Meaning |
| --- | --- |
| `*> expression` | Produce the expression's result for each input item |
| `?> condition` | Keep the input item when the condition is true |
| `?*> optional-expression` | Produce each present result and omit absent results |
| `*\|> effect` | Evaluate the effect and pass the input item through unchanged |

`?*>` combines transformation with omission:

```yumi #filter-map
-- List
    ** 1 .. 6
    ⟦present⟧?*> each * 10 if each % 2 == 0⟦/present⟧
```

- [Map present results](#source-filter-map--present): Emit the transformed value when present; omit absence. This also produces `20, 40, 60`.

## Stage bodies are expressions

A stage evaluates the expression written after its operator for each item that reaches it.
To call a function, write the call explicitly:

```yumi #stage-expression
-- List
    ** 1 .. 3
    ⟦call⟧*> double(each)⟦/call⟧

━━━━━━━━━━━━━━━━━━━━
+ double Int
    - value Int

value * 2
```

- [Call explicitly](#source-stage-expression--call): The expression invokes `double` for every item, producing `2, 4, 6`.

| Stage expression | What it produces |
| --- | --- |
| `*> double(each)` | The result of calling `double` |
| `*> double` | The function value itself |
| `*> 7` | A `7` for each input |
| `*> make()` | A fresh call result for each input, even without mentioning `each` |

Stages can also follow an existing stream directly. These fragments assume `values` is a stream
and `double` is a function:

```yumi
values ?> each > 0
values *> double(each)
```

There is no need to wrap an existing stream in another construction block just to transform it.

## When the work happens

Pipeline stages are lazy: constructing the pipeline does not evaluate their bodies. A consumer
pulls items through the stages as it needs them. `List` requests all the items to build a stored
collection; a loop can stop early.

The tap stage, `*|>`, makes this timing visible. It performs an effect for each item reaching it,
then forwards that same item:

:::example-pair

- [Tap](#source-lazy-effects--tap): Record an item only when consumption brings it through this stage.
- [Pull](#source-lazy-effects--consume): Creating the pipeline does not run the tap; iteration does.
- [Stop early](#source-lazy-effects--stop): The third item never reaches the tap.

```yumi #lazy-effects
. values =
    ** 1 .. 3
    ⟦tap⟧*|> record(each)⟦/tap⟧
-- "created"
⟦consume⟧for value in values⟦/consume⟧
    -- "out $value"
    ⟦stop⟧if value == 2
        break⟦/stop⟧

━━━━━━━━━━━━━━━━━━━━
+ do record
    - value Int

-- "tap $value"
```

:::

The output is:

```text
created
tap 1
out 1
tap 2
out 2
```

Nothing is recorded when `values` is assigned. The loop pulls two items, then stops, so the tap
never sees `3`. An unconsumed pipeline does not run its tap, even if the pipeline appears as a
standalone expression.

A tap runs when an item reaches that stage. A later filter may still discard the item. Place the
tap after the filter when the effect should apply only to items the filter keeps. For immediate
iteration whose purpose is an effect, an explicit `for` loop makes that intent clear.

## Choosing stream pipelines

Stream pipelines are useful when the result is naturally described as contributions followed by
transformations: a menu with optional entries, a sequence assembled from several sources, or a
filtered stream of names to join into text. Start with what belongs in the result, then put the
stages that refine it underneath.

Keep the consumer visible when timing matters. The same stream pipeline can feed a stored list
or a stream consumed a little at a time, and that choice determines when its stages do their work.

## Standard-library operations

Stream syntax and library operations work together. The `/stream` module re-exports the main
operation groups; importing a specific module such as `/stream/aggregate` keeps an example's
dependencies focused.

Function names alone are not enough to determine whether an operation consumes the input,
produces another stream, or stores intermediate values. The examples below show the current
library's behavior, including a few names whose meanings may differ from other libraries.

### Aggregation and running results

```yumi #aggregation
/stream/aggregate

-- sum([1, 2, 3])
-- ⟦reduce⟧reduce(10, [1, 2, 3]) • using :: total, item => total + item⟦/reduce⟧
-- List(⟦scan⟧scan(0, [1, 2, 3]) • using :: total, item => total + item⟦/scan⟧)
-- ⟦first⟧first(List'Int()) ?? -1⟦/first⟧
-- some([1, 2, 3]) • where each == 2
```

- [Final accumulator](#source-aggregation--reduce): Start at `10` and produce `16` after consuming the input.
- [Running results](#source-aggregation--scan): Yield `1, 3, 6`; the seed is not an extra emitted item.
- [Empty input](#source-aggregation--first): An absent first item falls back to `-1`.

The output is:

```text
6
16
1, 3, 6
-1
true
```

`reduce` computes one final accumulator. `scan` exposes its intermediate values as a stream;
the seeded form emits the updated accumulator for each input, rather than emitting the seed
as an extra first item.

| Operation | Consumption and result |
| --- | --- |
| `count`, `sum`, `reduce` | Consume the input to produce a final answer |
| `scan`, `scan-map` | Produce incremental accumulator results |
| `first` | Select the first item; empty input is absent |
| `last` | Consume to the end; empty input is absent |
| `headtail` | Split the first item from the remaining source |
| `some` | Stop once a predicate succeeds |
| `all` from `/stream/aggregate` | Stop once a predicate fails; true for empty input |
| `min`, `max`, `average` | Summarize the complete input |

Empty-input policies differ. Unseeded `reduce` currently returns the item type's default
value for an empty source; the seeded form returns its seed. `min` and `max` use the unseeded
reduction behavior. Do not infer an optional result or an exception just from familiarity
with another language's API. Use a seed or an explicit empty check when the distinction
belongs in the domain model.

The inspection module also exposes stream-producing predicates. For example,
`contains(1, values)` from `/stream/inspect` returns a stream carrying a Boolean. That is
different from the direct Boolean returned by `some` in `/stream/aggregate`.

### Selection and repetition

```yumi #selection
/stream/combine
/stream/filter

-- List(⟦take⟧take(2, [10, 20, 30, 40])⟦/take⟧)
-- List(repeat(2, [3, 4]))
-- List(⟦distinct⟧distinct([1, 1, 2, 1])⟦/distinct⟧)
-- List(⟦adjacent⟧distinct-until-changed([1, 1, 2, 1])⟦/adjacent⟧)
```

- [Select a position](#source-selection--take): This library's `take(2)` yields `30`, the item at zero-based position two.
- [Remember seen values](#source-selection--distinct): Yield `1, 2` across the traversal.
- [Compare adjacent keys](#source-selection--adjacent): Yield `1, 2, 1`; a value may return after a different one.

This prints:

```text
30
3, 4, 3, 4
1, 2
1, 2, 1
```

In the current library, `take(n)` selects the item at zero-based position `n` and yields that
item. It does **not** mean “take the first `n` items.” `take-last(n)` selects by the corresponding
distance from the end and must reach the end first. Use bounded iteration when you mean a
prefix of the source. Other `take` overloads stop according to a terminator stream or pair
items with an interval stream.

`distinct` remembers earlier keys or values across the traversal. Its memory can grow with
the number of distinct items. `distinct-until-changed` compares against the preceding retained
key and allows a value to appear again after a different value. Key-selector overloads let
the program state which part of an item determines equality.

`repeat` repeats a source's sequence. Replayable inputs can be traversed again; a one-pass
iterable may need to be collected so subsequent repetitions have something to read. A
mutable source traversed again can expose mutations made between traversals.

### Combining sources

```yumi #combination
/stream/combine

-- List(⟦zip⟧zip([1, 2, 3], [10, 20])⟦/zip⟧ *> (each[0] + each[1]))
-- List(⟦product⟧product([1, 2], [3, 4])⟦/product⟧ *> (each[0] * 10 + each[1]))
-- List(⟦flatten⟧flat-map([[1, 2], [3, 4]])⟦/flatten⟧)
```

- [Corresponding items](#source-combination--zip): Form pairs until one input ends.
- [Every combination](#source-combination--product): Pair each first-source item with each second-source item.
- [Inner sequences](#source-combination--flatten): Consume the inner streams, yielding `1, 2, 3, 4`.

This prints `11, 22`, then `13, 14, 23, 24`, then `1, 2, 3, 4`.

| Operation | Relationship between inputs |
| --- | --- |
| `and` | Consume the first source, then the second |
| `or` | Merge inputs through concurrently running producers |
| `zip` | Pair corresponding items; stop when a pair can no longer be formed |
| `product` | Pair each item of the first source with each item of the second |
| `flat-map` | Consume the inner streams of a stream of streams |
| `indexed` | Attach a zero-based index to each item |

Concatenation and merging answer different questions. Concatenation preserves the complete
first sequence before beginning the second; if the first never ends, the second is never
reached. Merging allows arrivals from either producer and does not promise one global order
independent of scheduling.

`zip` can wait for the slower side. `product` needs to revisit the second source, and its
iterable overload materializes that input for reuse. The mathematical result is the same
across source families, but the storage and lifetime consequences can differ.

### Buffering, adjacent items, and sorting

```yumi #buffering
/stream/coincidence
/stream/sort

-- " | ".join(⟦buffer⟧buffer(2, [1, 2, 3, 4, 5])⟦/buffer⟧)
-- " | ".join(⟦pairs⟧pairwise([1, 2, 3])⟦/pairs⟧)
-- ⟦sort⟧sort([3, 1, 2])⟦/sort⟧
```

- [Complete groups](#source-buffering--buffer): Emit `1, 2` and `3, 4`; the incomplete final group is omitted.
- [Adjacent pairs](#source-buffering--pairs): Retain one previous item, producing `(1, 2)` and `(2, 3)`.
- [Stored ordering](#source-buffering--sort): Consume the finite input before producing sorted values.

This prints `1, 2 | 3, 4`, then `(1, 2) | (2, 3)`, then `1, 2, 3`.

The count-based `buffer` emits complete groups. In the current implementation, an incomplete
final group is not emitted. Its no-count overload has a different purpose: it starts a
producer task and returns a buffered stream. These two meanings should not be confused.

`pairwise` retains the preceding item and emits adjacent pairs. `partition` routes values
into two buffered outputs according to a predicate. Consumers that neglect one output can
allow its retained values to grow.

The current `window` operation is also worth reading by its actual contract: its predicate
form retains the first item, matching items, and a final nonmatching item when present.
It is not a generic fixed-width sliding-window API.

`sort` and `reversed` need stored input. They are natural operations on finite sources and
poor fits for an unbounded source unless the program first defines a finite group to order.


See [Streams](streams.md) for the common source family, and [Batteries included](batteries-included.md) for the wider library tour.
