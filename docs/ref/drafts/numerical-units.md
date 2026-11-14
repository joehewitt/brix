# Numerical units

Yumi can attach a unit to a number at the type level. A value such as `500ms` is not a plain
number followed by display text: it is a numeric value whose type is `Milliseconds`. Functions can
name that type in their signatures, conversions can account for the relationship between
milliseconds and seconds, and arithmetic can preserve the unit chosen by the expression.

Units are ordinary numeric classes. They inherit a machine representation from a numeric base such
as `F64` or `I16`, and they can participate in the rest of the class system. The unit suffix is a
compact way to construct a value of one of those classes.

## At a glance

| Capability | What it gives you |
| --- | --- |
| `16ms` | A typed quantity, not a number with display text appended. |
| Unit equality | Declare multiplicative relationships; conversions follow them transitively. |
| Arithmetic | Convert the right operand to the left unit and preserve that unit. |
| Aliases | Use short suffixes in literals and a canonical suffix for presentation. |
| Limits | No automatic compound dimensions, cancellation, or offset conversions. |

## Unit literals

Write the unit immediately after an integer or floating-point literal:

:::example-pair

- [Duration](#source-literals--time): The suffix chooses `Milliseconds`.
- [Angle](#source-literals--angle): The suffix chooses `Degrees`.
- [Coordinate](#source-literals--space): A visible alias connects the literal to a numeric class.
- [Percentage](#source-literals--ratio): Supported symbol suffixes work too. There is no space before a suffix.

```yumi #literals
. frame-time = ⟦time⟧16ms⟦/time⟧
. rotation = ⟦angle⟧90deg⟦/angle⟧
. width = ⟦space⟧200px⟦/space⟧
. opacity = ⟦ratio⟧25%⟦/ratio⟧
```

:::

The suffix determines the type of the entire literal. For example, `16ms` has type `Milliseconds`
and `90deg` has type `Degrees`. In a type position, use the class name rather than its literal
suffix:

```yumi
+ frame-delay Milliseconds = 16ms

━━━━━━━━━━━━━━━━━━━━
+ rotate
    - angle Degrees
```

## Declaring a unit family

A unit family begins with a numeric class. Its first suffixed unit inherits from that family, and
the remaining units state their size in terms of an existing unit:

:::example-pair

- [Representation](#source-family--base): The family uses floating-point storage and can own shared behavior.
- [Conversion equality](#source-family--equality): One microsecond equals one thousand nanoseconds. Chains compose transitively.
- [Literal aliases](#source-family--aliases): Both spellings are accepted; the last, `seconds`, is the canonical suffix used by `UnitString`.

```yumi #family
⟦base⟧+ Duration is F64⟦/base⟧

+ Nanoseconds (ns) is Duration
⟦equality⟧+ Microseconds (µs) is 1000ns⟦/equality⟧
+ Milliseconds (ms) is 1000µs
+ Seconds ⟦aliases⟧(s, seconds)⟦/aliases⟧ is 1000ms
+ Minutes (minutes) is 60s
+ Hours (hours) is 60minutes
```

:::

The family class is a useful home for behavior shared by every unit. In the standard library,
`Duration` inherits from `F64` and defines operations such as waiting, timers, and conversion to
native time values. Every duration unit inherits that behavior through its base chain.

A unit does not need a separate family class when no shared domain is required. A direct numeric
unit can be declared in one line:

```yumi
+ Coord (px) is Float
```

Here, `Coord` is the type, `px` is its literal suffix, and `Float` supplies its representation and
numeric behavior.

## Converting values

Yumi converts between connected units when a surrounding type requires it:

```yumi #conversion
. timeout ⟦destination⟧Seconds⟦/destination⟧ = ⟦source⟧1500ms⟦/source⟧
```

- [Destination type](#source-conversion--destination): The context requests seconds.
- [Source quantity](#source-conversion--source): Implicit conversion stores `1.5seconds`.

The value stored in `timeout` is `1.5seconds`. Write an explicit cast when the destination is part
of the expression rather than supplied by a declaration, argument, or return type:

```yumi
. radians = 180° as Radians
```

Conversions use the product of the equalities connecting the source and destination. The direction
of the conversion is derived automatically from the declaration, so declaring `Seconds` in terms
of `Milliseconds` also permits conversion from seconds to milliseconds.

Unit equalities are multiplicative. They can express seconds and milliseconds, radians and degrees,
or ratios and percentages. They cannot directly express conversions with an offset, such as two
temperature scales with different zero points. Such conversions need functions or converters that
perform the complete calculation.

## Arithmetic and comparisons

For arithmetic between connected unit types, Yumi converts the right operand to the left operand's
unit before applying the numeric operator. The result retains the left operand's unit. A plain
number on the right takes the left operand's unit, so `2km * 2` is `4km`, and a multiplication
keeps the unit on either side, so `2 * 3s` is `6s`.

| Expression | Result |
| --- | --- |
| `8hours + 30minutes` | `8.5hours` |
| `30minutes + 8hours` | `510minutes` |
| `8hours - 30minutes` | `7.5hours` |
| `180° as Radians` | approximately `3.141593rad` |

The same conversion makes comparisons between connected units natural. For example, `500ms < 1s`
compares the same quantity in a common unit.

The left-unit rule is significant: changing operand order can change the result type and its
canonical presentation even when the quantities are equal. Choose the left operand deliberately
when later code expects a particular unit.

Yumi's unit system is not dimensional analysis. Multiplication and division do not synthesize
compound types such as meters per second, nor do they cancel matching units. Once connected
operands have been converted, these operators perform ordinary numeric arithmetic and preserve the
left unit. Programs that need types such as velocity, area, or energy should declare those domain
types and the operations that produce them.

## Connecting separately declared units

A public module-level equality can connect unit types that were declared independently:

```yumi
+ 1csec = 1000000000cnsec
+ 1csec = 1seconds
```

This form is useful at system boundaries. In this example, native C time units are connected to
each other and then to the `Duration` family. The conversion graph is transitive, so the second
declaration also makes `cnsec` convertible to duration units through `csec`.

Both sides should be unit-bearing constants. A raw built-in numeric type does not identify a unit
and is therefore not a suitable endpoint for an equality declaration.

## Ratios and percentages

The standard units module defines normalized ratios and percentages with the same mechanism:

```yumi #ratios
⟦ratio⟧+ Ratio (%%) is F64⟦/ratio⟧
⟦percent⟧+ Percent (%) is 0.01%%⟦/percent⟧
```

- [Normalized ratio](#source-ratios--ratio): `1%%` is one; `0.5%%` is one half.
- [Percentage equality](#source-ratios--percent): `25%` converts to `0.25%%`, and `100%` to `1%%`.

The module also provides `Scale` and `Scalable` for mapping values to normalized ratios and between
ranges. Scaling is built on numerical units, but it is separate from unit equality: a scale maps a
position within a range, while a unit equality states that two numeric representations denote the
same quantity.

### Example: choosing an hour with a slider

The `Hour` type implements `Scalable` and advertises its range as `Scale(0hour, 23hour)`. Therefore,
`0.75%% as Hour` produces `18hour`. The cast maps the normalized ratio onto the range owned by
`Hour`; the caller does not need to know or repeat its endpoints.

The leading zero is significant. `0.75%%` is a ratio of three quarters, while `75%%` is a ratio of
seventy-five and would map far beyond the hour range. When starting with a percentage, `75% as
Ratio` produces the equivalent `0.75%%`.

## Unit names and strings

Every numeric value exposes its canonical suffix through `unit`. Values without a unit return an
empty suffix. Cast a value to `UnitString` when its unit should be included in its textual form:

```yumi #unit-text
. suffix = ⟦suffix⟧(2days).unit⟦/suffix⟧
. text = ⟦text⟧2days as UnitString⟦/text⟧
```

- [Inspect the suffix](#source-unit-text--suffix): This returns `"days"`.
- [Format with the unit](#source-unit-text--text): Use the underlying numeric formatting followed by the canonical suffix.

## Standard unit families

The standard library uses numerical units in several domains:

- `/time/duration` defines durations from nanoseconds through larger calendar approximations.
- `/math` defines radians and degrees.
- `/units/distance` defines a distance family.
- `/graphics/primitives` uses `px` coordinates.
- `/tui/primitives` uses `tx` terminal coordinates.
- `/units` defines ratios, percentages, scales, and the `Scalable` interface.

These families all use the same language feature. Their differing behavior comes from the methods
on their numeric classes and from the equalities declared between their units.

## Designing units

- **Choose storage first.** Use a floating-point base for fractional quantities. An integer-backed destination still has integer storage after conversion.
- **Keep equalities consistent.** Prefer one obvious base and short conversion paths. Every path between the same types should agree.
- **Choose the last alias deliberately.** It becomes the public suffix used by `UnitString`.
- **Stay within the model.** Use explicit functions or domain types for offsets, compound dimensions, and relationships beyond a multiplicative ratio.
