# Sum types

A sum type describes a value that can be one of several alternatives. A download might be
waiting, in progress, or complete. Each alternative can carry the data that belongs to it:
progress needs a byte count, while completion needs the downloaded file's path.

A value holds one active case together with its associated data, also called its payload.

## At a glance

| Capability | What it gives you |
| --- | --- |
| Named union cases | Distinguish domain states and give each its own payload. |
| Type alternatives | Use existing types as the distinctions in a union. |
| Anonymous cases | Infer local alternatives from expressions such as `..accepted(name)`. |
| Matches and tests | Handle cases together with `match`, or narrow one branch with `is`. |
| Enums | Name particular values that share one class's representation and behavior. |

## A value with alternatives

Yumi calls these types unions. Declare one with `is Union`, then list its cases with `|`:

:::example-pair

- [Construct a state](#source-download--construction): Choose the completed case and supply its path.
- [Read its payload](#source-download--match): The arm binds the path and prints `Saved to report.pdf`.
- [No payload](#source-download--waiting): Waiting needs no extra data.
- [Several fields](#source-download--progress): Progress carries both byte counts.
- [One field](#source-download--complete): A completed download carries its path as part of construction.

```yumi #download
. download = ⟦construction⟧Download.complete("report.pdf")⟦/construction⟧
-- match download
    | .waiting
        "Waiting"
    | .progress(received, total)
        "$received of $total bytes"
    | ⟦match⟧.complete(path)⟦/match⟧
        "Saved to $path"

━━━━━━━━━━━━━━━━━━━━
+ Download is Union

⟦waiting⟧| waiting⟦/waiting⟧
⟦progress⟧| progress(received Int, total Int)⟦/progress⟧
⟦complete⟧| complete(path String)⟦/complete⟧
```

:::

## Declaring and constructing cases

A case can have no payload, a single payload type, or a tuple of named fields. These declaration
fragments illustrate the forms:

```text
| waiting
| complete String
| progress(received Int, total Int)
```

Construct a case through its union type. A case without data is a value such as `Download.waiting`.
A case with data takes arguments, as in `Download.complete("report.pdf")` or
`Download.progress(20, 100)`.

Case names preserve distinctions that the payload type alone cannot express. For example, a
horizontal move and a vertical move can both carry an integer:

```yumi #same-payload
. move = ⟦choice⟧Move.vertical(12)⟦/choice⟧
-- match move
    | .horizontal(distance)
        "Across $distance"
    | .vertical(distance)
        "Down $distance"

━━━━━━━━━━━━━━━━━━━━
+ Move is Union

⟦cases⟧| horizontal Int
| vertical Int⟦/cases⟧
```

- [Choose by name](#source-same-payload--choice): Both alternatives carry `Int`; the constructor says which meaning applies.
- [Distinct roles](#source-same-payload--cases): A shared payload type does not collapse the cases. The match prints `Down 12`.

## Matching cases

The patterns in this chapter are part of Yumi's broader
[structural pattern matching and destructuring](structural-pattern-matching.md) vocabulary.
Here the focus is how those patterns work with a union's alternatives.

`match` selects behavior according to the active case. A pattern such as `.complete(path)` checks
the case and binds its payload to a local name. A tuple pattern such as `.progress(received, total)`
binds the individual fields. The names in the pattern are chosen by the match; they do not have
to repeat the field names in the declaration.

The leading dot in these patterns lets Yumi resolve the case against the subject's type. You can
also qualify the case, as in `Download.complete(path)`, when spelling out the type makes the code
clearer.

An ordinary match over a union must cover all its cases. Omitting a case is a compile error,
even when the value was just constructed using one of the cases that you did handle. This keeps
the match tied to the full type's possibilities.

A `??` fallback can handle the remaining cases. Prefer explicit cases when adding a new
alternative should force callers to reconsider their behavior; use a fallback when the remaining
cases deliberately share one answer.

When an unmatched case should produce no value, use `match?`:

```yumi #optional-case
. download = Download.waiting
. path = ⟦partial⟧match? download⟦/partial⟧
    | .complete(path)
        path
-- ⟦fallback⟧path ?? "No file yet"⟦/fallback⟧

━━━━━━━━━━━━━━━━━━━━
+ Download is Union

| waiting
| complete(path String)
```

- [Partial interest](#source-optional-case--partial): Return a path only when the completed case matches.
- [Handle absence](#source-optional-case--fallback): The waiting case produces no value, so this prints `No file yet`.

## Testing and narrowing

Use `is` when you only need to test a case. Within a branch established by that test, Yumi can
narrow the value to the case's payload:

```yumi #narrowing
. event = Event.click(10, 20)
if ⟦test⟧event is .click⟦/test⟧
    ⟦fields⟧-- event.x
    -- event.y⟦/fields⟧

━━━━━━━━━━━━━━━━━━━━
+ Event is Union

| click(x Int, y Int)
| key String
```

- [Establish the case](#source-narrowing--test): The test narrows `event` inside the branch.
- [Use the payload](#source-narrowing--fields): The narrowed value exposes the click fields, printing `10` and `20`.

Comparing with `==` or `!=` against a case that carries no payload, such as `Fill.none` or
`.none`, narrows the same way as `is`. Ruling out each payload-free case leaves the remaining
case, so a chain that handles `inherit` and `none` first can use the payload in its final branch.
A comparison with a case that holds a value stays an ordinary value comparison and does not
narrow.

Narrowing is useful for a focused check. Matching is useful when the alternatives together form
the decision you want to express.

Named cases also expose payload accessors, such as `download.complete`. These accessors do not
prove that the case is active: in the current implementation, an accessor for an inactive case
returns the payload type's default value. Use a case test or a match when the distinction matters.
For example, an empty string by itself cannot tell you whether a string case was active.

## Alternatives identified by type

A union can list existing types directly, without introducing case names:

:::example-pair

- [Enter the union](#source-type-alternatives--conversion): The string converts to the corresponding alternative.
- [Match by type](#source-type-alternatives--capture): Capture the string directly; the result is `Text hello`.
- [Existing distinctions](#source-type-alternatives--types): The types themselves identify the alternatives.

```yumi #type-alternatives
⟦conversion⟧. value NumberOrText = "hello"⟦/conversion⟧
-- match value
    | number Int
        "Number $number"
    | ⟦capture⟧text String⟦/capture⟧
        "Text $text"

━━━━━━━━━━━━━━━━━━━━
+ NumberOrText is Union

⟦types⟧| Int
| String⟦/types⟧
```

:::

Use this form when the existing types already communicate the distinction. Use named cases when
the distinction belongs to your domain, particularly when different cases carry the same type.
The two styles can coexist in one union. For example, the standard library's `Fill` combines
named `inherit`, `none`, and `stroke` cases with `RGBA` and `FillStack` type alternatives.

An explicit cast can extract a type alternative, but a cast to an inactive alternative raises an
error. A match makes the alternatives and their handling visible in one place.

## Anonymous cases

Some decisions need several possible outcomes without needing a separately declared type. Yumi
can infer an anonymous union from case expressions written with two leading dots:

```yumi #anonymous
-- describe(check-name("Yumi"))
-- describe(check-name(""))

━━━━━━━━━━━━━━━━━━━━
+ check-name
    - name String

|? name.length > 0
    ⟦accepted⟧..accepted(name)⟦/accepted⟧
??
    ⟦empty⟧..empty⟦/empty⟧

━━━━━━━━━━━━━━━━━━━━
+ describe String
    - result

match result
    ⟦consume⟧| .accepted(name)⟦/consume⟧
        "Hello, $name"
    | .empty
        "Enter a name"
```

- [Infer a payload case](#source-anonymous--accepted): This outcome contributes an `accepted` case with a string payload.
- [Infer an empty case](#source-anonymous--empty): This outcome carries no payload.
- [Match normally](#source-anonymous--consume): The caller uses a single-dot case pattern. The calls print `Hello, Yumi` and `Enter a name`.

The punctuation distinguishes introducing an inferred case with `..accepted` from referring to
an existing case with `.accepted`. Anonymous cases need a stable owner for the inferred result,
such as a function result or a bound variable. They cannot be introduced in every arbitrary
expression position. Repeated uses of the same inferred case name must also agree on their
number of payload arguments.

Anonymous cases are convenient for local decisions and inferred return values. A declared union
gives a shared concept an explicit name and a single place to define its alternatives.

## Behavior on a union

A union can define methods alongside its cases. This is useful when callers should ask the value
for an answer instead of repeating the same match:

```yumi #union-method
-- Download.waiting.label()
-- Download.complete("report.pdf").label()

━━━━━━━━━━━━━━━━━━━━
+ Download is Union

| waiting
| complete(path String)

────────────────────
⟦method⟧+ label String⟦/method⟧

⟦decision⟧match this⟦/decision⟧
    | .waiting
        "Waiting"
    | .complete(path)
        "Saved to $path"
```

- [Shared operation](#source-union-method--method): Callers ask every case for a label.
- [Decision inside the type](#source-union-method--decision): The method owns the case analysis. The calls print `Waiting` and `Saved to report.pdf`.

## Related feature: enums

Yumi also uses `|` declarations to give names to particular values of a class. These are enum
members. They share the enclosing class's representation and behavior, and they can be more
than integer constants.

The simplest numeric form lists names inside an integer-based class:

:::example-pair

- [Numeric class](#source-numeric-enum--representation): `Priority` supplies the representation; there is no separate enum keyword.
- [Named values](#source-numeric-enum--members): Implicit values start at zero. These members print `0`, `1`, and `2`.

```yumi #numeric-enum
-- Priority.low
-- Priority.normal
-- Priority.high

━━━━━━━━━━━━━━━━━━━━
⟦representation⟧+ Priority is Int⟦/representation⟧

⟦members⟧| low
| normal
| high⟦/members⟧
```

:::

### Explicit values

Use `=` to supply a member's value. This is useful when the numbers have meaning outside the
declaration, such as protocol codes:

```yumi #explicit-enum
. code = ResponseCode.not-found
-- code
-- match code
    |= ResponseCode.ok
        "Found"
    ⟦comparison⟧|= ResponseCode.not-found⟦/comparison⟧
        "Missing"
    ??
        "Other response"

━━━━━━━━━━━━━━━━━━━━
+ ResponseCode is Int

| ok = 200
⟦value⟧| not-found = 404⟦/value⟧
```

- [External meaning](#source-explicit-enum--value): Assign the numeric value required by the protocol.
- [Value match](#source-explicit-enum--comparison): `|=` compares against the member; it does not extract a union payload. The output is `404`, then `Missing`.

Enum members are matched as values with `|=`. A union pattern such as `| .complete(path)`
checks a case and extracts its payload; an enum arm such as `|= ResponseCode.ok` compares against
the named value. When the subject supplies the type, the shorter `|= .ok` form is also available.

### Members with shared data

Enum members can be instances of a tuple or an ordinary class. Each member then has the same
fields, initialized with its own values:

:::example-pair

- [Shared fields](#source-enum-data--shape): Every member has the same field layout; no case test is needed before reading it.
- [Member data](#source-enum-data--member): Each named value initializes that layout. The calls print `North`, `1`, and `Go east`.

```yumi #enum-data
-- Direction.north.label
-- Direction.east.dx
-- match Direction.east
    |= Direction.north
        "Go north"
    |= Direction.east
        "Go east"
    ??
        "Other direction"

━━━━━━━━━━━━━━━━━━━━
⟦shape⟧+ Direction is (dx Int, dy Int, label String)⟦/shape⟧

| north = (0, -1, "North")
⟦member⟧| east = (1, 0, "East")⟦/member⟧
```

:::

For an ordinary class, member initializers use the class's construction and conversion behavior.
The enum declaration does not invent fields from the initializer. Define the fields and any
constructor the class needs, just as for other instances.

The standard library uses this feature for calendar values. `DayOfWeek` members carry full and
abbreviated names, while `Month` members carry names, a month number, and a day count. Methods
and conversions on their enclosing types operate on that shared data.

### Enums and unions

Both features give alternatives readable names, but their declarations describe different things:

| Feature | What the names describe | Where the data is defined |
| --- | --- | --- |
| Enum members | Particular values of one class | Shared fields or numeric representation on the class |
| Union cases | Alternatives that a union value can hold | A separate payload shape for each case |

An enum member such as `Direction.east` comes with its declared field values. A union constructor
such as `Download.progress(20, 100)` creates a case value with data supplied by the caller.

Use enums for named values with a common shape: priorities, protocol codes, or calendar entries
with shared metadata. Use unions when the alternatives need different data or represent distinct
roles for the same payload type. A union containing only cases without payloads is also useful
for a set of named choices that does not need a numeric representation or shared fields.

## Choosing the alternatives

Start with the states or values the program actually needs to distinguish. Give each case the
data required to make it meaningful. Avoid adding fields to every case merely because one case
needs them.

An optional type such as `String?` expresses a simpler choice: a string is present or absent.
A union is useful when absence has several meanings, or when each alternative carries different
data. A tuple combines its fields; a union chooses among its cases. Combining the two lets a type
say both which situation exists and what is known about it.
