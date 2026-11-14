# Structural pattern matching and destructuring

Structural patterns recognize the shape of an existing value and name the parts that matter.
The same vocabulary works in multi-arm decisions, optional extraction, Boolean tests,
conditional bindings, and destructuring assignments.

The result is less separation between asking a question and using its answer. A branch can say
“a click with coordinates `x` and `y`” instead of testing a tag, casting a payload, and reading
two fields in separate steps.

## At a glance

| Capability | Form | Result |
| --- | --- | --- |
| Choose among shapes | `match value` | The selected arm's value |
| Extract when possible | `match? value` | An optional result |
| Test a shape | `value is pattern` | `Bool` |
| Test and bind | `if \| pattern = value` | Names scoped to the successful branch |
| Destructure a known shape | `. pattern = value` | New local bindings |
| Destructure into variables | `pattern = value` | Updated existing bindings |
| Ignore one part | `_` | No binding |
| Capture a remainder | `...rest` | The unmatched portion of a collection or text |

Patterns can combine literals, types, predicates, tuple positions, union cases, indexed
collections, map keys, and text anchors.

## A complete example

:::example-pair

This function classifies a decoded response while extracting only the data needed by each arm:

- [Literal value pattern](#source-response-summary--literal): The first arm requires the value at `status` to equal `"ok"`.
- [Nested collection pattern](#source-response-summary--nested): The `items` value must be a nonempty string list. The pattern captures its first item and remainder.
- [Arm result](#source-response-summary--result): A match is an expression. The selected arm supplies the function's return value.
- [Exact empty shape](#source-response-summary--empty): The second arm distinguishes an empty list from a missing or malformed `items` value.
- [Typed capture](#source-response-summary--capture): The arm requires a string and binds it as `message`.
- [Fallback arm](#source-response-summary--fallback): `??` handles every shape not recognized above it.

```yumi #response-summary
-- response-summary(response)

━━━━━━━━━━━━━━━━━━━━
+ response-summary
    - response [String = Payload]

match response
    | ["status" = ⟦literal⟧"ok"⟦/literal⟧, "items" = ⟦nested⟧[first String, ...rest]⟦/nested⟧]
        ⟦result⟧"First: $first; ${rest.length} more"⟦/result⟧
    | ["status" = "ok", "items" = ⟦empty⟧[]⟦/empty⟧]
        "No items"
    | ["error" = ⟦capture⟧message String⟦/capture⟧]
        "Failed: $message"
    ⟦fallback⟧??
        "Unrecognized response"⟦/fallback⟧

━━━━━━━━━━━━━━━━━━━━
+ Payload is Union

| String
| [String]
```

:::

The outer map patterns, inner list patterns, literal checks, type checks, captures, and fallback
form one decision. Arms are tried in order, so specific shapes should generally precede broader
ones.

## Pattern building blocks

Patterns can test a value, constrain its type, apply a predicate, or combine those operations
with a capture.

| Pattern | Meaning |
| --- | --- |
| `42` | Equal to the literal value |
| `String` | Compatible with the named type |
| `name String` | A string captured as `name` |
| `{it > 20}` | Satisfies the predicate |
| `score {it > 20}` | A value over 20, captured as `score` |
| `_` | Any value, without a binding |

```yumi #pattern-building-blocks
-- match ("score", 42)
    | ⟦typed⟧label String⟦/typed⟧, ⟦predicate⟧amount {it > 20}⟦/predicate⟧
        "$label is high: $amount"
    ??
        "No high score"
```

- [Typed capture](#source-pattern-building-blocks--typed): The first tuple field must be a `String`; the arm names it `label`.
- [Predicate capture](#source-pattern-building-blocks--predicate): `it` is the value being tested. A successful test makes the value available as `amount`.

For simple decisions, equality and condition arms provide compact alternatives:

```yumi
match count
    |= 0
        "Empty"
    |? it < 3
        "Almost empty"
    ??
        "Available"
```

- `|=` compares the subject with a value.
- `|?` evaluates a condition with the subject available as `it`.
- `??` supplies the fallback.

## Tuples and union cases

Comma-separated patterns unpack tuple-shaped values positionally. Union-case patterns first
select the active case, then unpack its payload.

```yumi #union-cases
. event = Event.click(12, 30)
-- match event
    | ⟦case⟧.click(⟦fields⟧x, y⟦/fields⟧)⟦/case⟧
        "Click at $x, $y"
    | .key(⟦single⟧text⟦/single⟧)
        "Key $text"

━━━━━━━━━━━━━━━━━━━━
+ Event is Union

| click(x Int, y Int)
| key String
```

- [Union-case pattern](#source-union-cases--case): The arm applies only when `event` contains the `click` case.
- [Tuple payload](#source-union-cases--fields): The two payload positions become the local names `x` and `y`.
- [Single payload](#source-union-cases--single): A one-value case can bind that value directly.

A union match is checked against the type's alternatives. Adding a new case can therefore
force exhaustive matches to be reconsidered. See [Sum types and enums](sum-types.md) for the
modeling side of this feature.

Tuple-shaped values can be matched without a union case:

```yumi
-- match (3, 4)
    | x Int, y Int
        x * y
```

The positions are significant even when the tuple type gives its fields names.

## Lists, arrays, and remainders

Bracket patterns recognize supported indexable collections. Without a remainder marker, the
length is exact.

| Pattern | Accepted shape |
| --- | --- |
| `[]` | Empty |
| `[first, second]` | Exactly two items |
| `[first, ...]` | One or more items |
| `[..., last]` | One or more items, capturing the last |
| `[first, ...rest]` | A first item plus any remainder |
| `[first, ...middle, last]` | Two endpoints with any middle |
| `[10, _, ...]` | Starts with `10` and contains at least two items |

```yumi #array-remainder
. values = Array'Int([10, 20, 30, 40])
. middle = match values
    | [⟦first⟧first⟦/first⟧, ⟦rest⟧...middle⟦/rest⟧, ⟦last⟧last⟦/last⟧]
        middle

-- type-of middle
-- ", ".join(middle)
```

- [First item](#source-array-remainder--first): The collection must contain an item at the beginning.
- [Remainder capture](#source-array-remainder--rest): `middle` receives the slice between the two endpoints.
- [Final item](#source-array-remainder--last): The pattern also requires and captures an item at the end.

For this array, `middle` is an `Array'Int` view containing `20, 30`. Remainder captures follow
the source's slicing behavior; they do not promise a detached list or copy. Lists, arrays,
tables, and other supported indexable values can therefore retain their useful representation.

Strings use text patterns instead of bracket patterns. Streams arriving over time use
[stream pattern matching](stream-pattern-matching.md).

## Maps and nested data

A map pattern requires named keys and applies a pattern to each corresponding value. Extra
entries are allowed; `...rest` can capture them when they matter.

```yumi #map-entries
. settings = Map'String'Int()
settings["width"] = 80
settings["height"] = 24
settings["scale"] = 2

. unused = match settings
    | ["width" = ⟦width⟧width {it > 0}⟦/width⟧, "height" = ⟦height⟧height {it > 0}⟦/height⟧, ⟦rest⟧...rest⟦/rest⟧]
        -- "$width by $height"
        rest
```

- [Required key and predicate](#source-map-entries--width): The map must contain a positive value at `"width"`; that value becomes `width`.
- [Second required key](#source-map-entries--height): A missing or nonpositive height makes the arm fail.
- [Unmatched entries](#source-map-entries--rest): `rest` contains entries not consumed by the named key patterns, including `"scale"` here.

Patterns can nest through maps, lists, tuples, and union-valued fields:

```yumi #nested-data
. first = match? response
    | ["choices" = [⟦nested⟧["message" = message, ...]⟦/nested⟧, ...], ...]
        message
```

- [Nested shape](#source-nested-data--nested): The pattern requires a `choices` collection whose first item is a map containing `message`.

`match?` returns an absent value when any required layer is missing or has the wrong shape.
That makes nested patterns useful for decoded data whose complete schema is larger than the
part a caller needs.

## Text patterns

Structural text patterns recognize literal anchors and capture the gaps between them.

```yumi #text-anchors
-- match "name:ada;"
    | ⟦prefix⟧"name:"⟦/prefix⟧ ⟦name⟧...name⟦/name⟧ ⟦suffix⟧";"⟦/suffix⟧
        name
    ??
        "Invalid record"
```

- [Literal anchor](#source-text-anchors--prefix): The complete string must begin with `name:`.
- [Captured gap](#source-text-anchors--name): `name` receives the text before the next literal anchor.
- [Final anchor](#source-text-anchors--suffix): The complete string must end with a semicolon.

The gap may be empty. Structural text patterns allow one remainder, which makes this form
suited to a small, anchored text shape. Use
[stream pattern matching](stream-pattern-matching.md) for repetition, character classes,
recursive rules, binary decoding, successive records, or input that arrives over time.

## Destructuring and conditional binding

When the shape is already expected, a destructuring declaration introduces its parts directly:

```yumi #destructuring-declaration
. ⟦pattern⟧[head, _, ...tail]⟦/pattern⟧ = [10, 20, 30, 40]
-- head
-- tail
```

- [Destructuring declaration](#source-destructuring-declaration--pattern): `head` becomes `10`, `_` ignores `20`, and `tail` becomes `[30, 40]`.

Existing variables can receive destructured values too:

```yumi #destructuring-assignment
. x = 0
. y = 0
⟦assignment⟧x, y = (10, 20)⟦/assignment⟧
-- x + y
```

- [Destructuring assignment](#source-destructuring-assignment--assignment): The tuple's two fields update the existing variables together.

When the shape is uncertain, a conditional binding keeps the extracted names inside the
successful branch:

```yumi #conditional-binding
. values = [5, 6, 7, 8]
if | ⟦binding⟧[_, second, ...rest] = values⟦/binding⟧
    -- second
    -- rest
else
    -- "Not enough items"
```

- [Conditional pattern binding](#source-conditional-binding--binding): The condition both tests the shape and introduces `second` and `rest` for the branch.

The visual distinction matters: `. values [Int] = source` binds the entire value with a type
annotation, while `. [first, ...rest] = source` binds parts through a pattern.

## Choosing a matching context

The pattern can stay the same while its surrounding form chooses what failure means:

```yumi #optional-extraction
. values = [10, 20, 30]

. optional-head = ⟦optional⟧match? values
    | [head, ...]
        head⟦/optional⟧
```

- [Optional extraction](#source-optional-extraction--optional): A failed pattern produces an absent value rather than selecting another result.

```yumi
. values = [10, 20, 30]
-- values is [_, _, _]
```

`is` answers whether the value has exactly three items.

```yumi #imperative-match
. values = [10, 20, 30]
⟦imperative⟧do match values
    | [head, ...]
        -- head
    ??
        -- "Empty"⟦/imperative⟧
```

- [Imperative match](#source-imperative-match--imperative): `do match` selects statements instead of producing one expression value.

| Context | On failure |
| --- | --- |
| `match value` | Another arm, `??`, an explicit failure, or a non-exhaustive-match error |
| `match? value` | An absent result |
| `value is pattern` | `false` |
| `if \| pattern = value` | The next branch or `else` |
| `. pattern = value` | The declared expectation fails |
| `do match? value` | Imperative execution continues without an arm |

Closed domains such as union cases support exhaustiveness checking. An arbitrary predicate,
list shape, or map shape does not generally prove that every possible value has been handled.

## Composing structural patterns

| Combine patterns with… | To… |
| --- | --- |
| Sum types | Select a case and unpack its payload |
| Optionals | Extract a value only when the expected shape is present |
| Error results | Match successful and failed outcomes after `catch` |
| JSON and other decoded data | Reach a needed field through nested maps, lists, and unions |
| Tables and array views | Preserve useful slice and column representations |
| Stream parsers | Classify the structured records produced by a grammar |

This fragment combines structural matching with the error system. It assumes `load` returns
either a `Config`, `FileNotFound`, or `PermissionDenied`:

```yumi #error-results
. label = match catch load(path)
    | ⟦success⟧config Config⟦/success⟧
        config.name
    | ⟦missing⟧error FileNotFound⟦/missing⟧
        "Use defaults"
    | ⟦denied⟧error PermissionDenied⟦/denied⟧
        "Ask for access"
```

- [Successful result](#source-error-results--success): The typed capture recognizes and names the function's success value.
- [One error alternative](#source-error-results--missing): Prefix `catch` holds the error so a pattern can handle it as a value.
- [Another error alternative](#source-error-results--denied): The match keeps different recovery policies in one exhaustive decision.

The central composition rule is simple: an outer pattern establishes a shape, and each nested
pattern receives the value at that position. Successful captures become ordinary typed names
inside the selected result or branch.

## Things to know

- Arms are ordered. Put a specific pattern before another pattern that would also accept it.
- `_` matches without binding; `...` changes the collection-length requirement.
- A captured remainder may be a view of the original representation rather than a copied list.
- Map patterns require the named keys but allow extra entries.
- Structural text patterns match an existing complete string; stream patterns consume successive input.
- Destructuring declares an expectation. Use `match?` or a conditional binding when failure is ordinary.
- Exhaustiveness applies to closed alternatives, not every collection shape or predicate.

Continue with [Sum types and enums](sum-types.md) for closed domain alternatives,
[Collections and data layout](collections-and-data-layout.md) for the representations being
matched, or [Stream pattern matching](stream-pattern-matching.md) for grammars over sequences.
