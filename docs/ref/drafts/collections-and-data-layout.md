# Collections and data layout

Yumi offers both everyday collections and more explicit control over how their data is stored.
A list or map can express the application's data model. Arrays, views, bitmaps, and tables
give the author more ways to match a representation to the operations performed on it.

These collections can also be stream sources. Choosing stored data does not prevent using
the same sequence operations as a generator or a channel.

## At a glance

| Capability | What it gives you |
| --- | --- |
| List / map / range | Ordered values, keyed values, or a compact progression. |
| Array / view | Contiguous mutable storage, or access to a region of existing storage. |
| Immutable array / builder | Construct a collection, then expose fixed storage. |
| Table | Present rows while storing fields in separate columns. |
| Bitmap | Represent positional Boolean choices as bits. |
| Backing storage | Choose managed storage or a more explicit lifetime independently of layout. |

## Familiar collections

:::example-pair

- [Ordered values](#source-collections--list): The list preserves the item order and type.
- [Keyed values](#source-collections--map): This map associates string keys with integers; its type shorthand is `[String = Int]`.
- [Missing key](#source-collections--absence): An absent lookup uses the fallback. The list type shorthand is `[String]`.

```yumi #collections
. names = ⟦list⟧["Ada", "Lin", "Sam"]⟦/list⟧
. scores = ⟦map⟧Map'String'Int()⟦/map⟧
scores["Ada"] = 10
scores["Lin"] = 12

-- ", ".join(names)
-- ⟦absence⟧scores["Sam"] ?? 0⟦/absence⟧
```

:::

`list += item` and `list.append(item)` add exactly one item, even when that item is itself a
collection. `list ++= items` and `list.append-all(items)` add each item of a stream in place.
`+` and `++` are the copying counterparts.

Ranges describe a progression without spelling out every item. `1 .. 4` includes both endpoints;
`0 .. < 4` excludes the upper endpoint. A range can supply loop bounds, stream input, or a
slice specification.

## Arrays make representation explicit

`Array'Item`, written `Item[]` in a type position, provides contiguous mutable array storage.
Arrays are useful for indexed processing, binary data, native interfaces, and algorithms
where layout matters.

```yumi #array-view
. values = ⟦storage⟧Array'Int([10, 20, 30, 40])⟦/storage⟧
. middle = ⟦slice⟧values[1 .. < 3]⟦/slice⟧
-- ", ".join(middle)
```

- [Array storage](#source-array-view--storage): Choose contiguous mutable storage explicitly.
- [Region](#source-array-view--slice): The half-open slice selects `20, 30`. A view stays connected to its backing storage rather than promising a separate snapshot.

That distinction matters when a function promises an independent snapshot. The source text
of a slice does not, by itself, promise a deep copy. A small retained view may also keep a
larger backing allocation reachable.

## Construction and immutability

Yumi has immutable array storage and builder-oriented construction. This supports a common
shape of algorithm: accumulate a result while constructing it, then expose a fixed collection
for consumption.

An immutable collection limits mutation through that collection's interface. If its elements
refer to mutable objects, those objects do not become deeply immutable merely because the
container is immutable.

Backing storage is another dimension. Garbage-collected storage is convenient for ordinary
application data. Arena-backed facilities make grouped lifetimes explicit. The choice can
affect what may outlive a computation and which operations can allocate, so it belongs near
the owner of the data rather than being scattered through unrelated consumers.

## Tables separate rows from physical layout

A table can present rows while storing each field in its own column. This is a
structure-of-arrays layout: all the `x` values together, all the `y` values together, and so on.

:::example-pair

- [Construct rows](#source-columns--construct): Describe three point-shaped values.
- [Read a column](#source-columns--column): Access the x coordinates together, producing `1, 2, 3`.
- [Row shape](#source-columns--shape): The type declares fields; the table stores each field in its own column.

```yumi #columns
/core/table

. points = ⟦construct⟧Table'Point(3) • with Point(each + 1, each + 10)⟦/construct⟧
-- ", ".join(⟦column⟧points.x⟦/column⟧)

━━━━━━━━━━━━━━━━━━━━
⟦shape⟧+ Point is Struct

+ x I32
+ y I32⟦/shape⟧
```

:::

Column layout can be useful when an operation reads one field across many rows. Processing
every field of one row at a time has different access patterns. A table is therefore a
representation choice, not a blanket promise that all operations become faster.

The distinction also makes some abstractions more interesting. Destructuring a supported
table can expose columns; iterating it visits row-shaped values. The same underlying storage
supports more than one useful view of the data.

## Bitmaps represent compact choices

Bitmaps store Boolean information as bits. They are useful for flags, occupancy, selection,
and other problems in which each position answers a yes-or-no question. Bitmap operations
can count values, find positions, and search for runs without requiring a collection of
separately represented Boolean elements.

This is another case where the representation expresses the problem. A set of selected rows
may fit a bitmap more naturally than a list of objects. A map remains more appropriate when
the question is about arbitrary keys rather than positions.

## Stored data and streams work together

| Need | Useful representation |
| --- | --- |
| Random access, reuse, or a stable stored result | Collection |
| Successive processing without retaining every item | Stream |
| Transform stored items | Feed a collection into a pipeline |
| Retain a finite pipeline's result | Collect it into a list or array |

[Structural patterns](structural-pattern-matching.md) can inspect a collection's shape.
[Stream pipelines](stream-pipelines.md) can transform its items. Neither requires the chapter
using the collection to repeat its storage implementation.

The useful question is what must remain available, in what form, and for how long. Yumi gives
that question answers ranging from a simple list literal to an explicitly owned region of
columnar or native storage.
