# The type system

Yumi is statically typed, but it leaves much of the repetition out of the source. The compiler
can infer types from initializers, function bodies, and calls. Explicit types remain useful
when they explain a domain boundary or choose among possible representations.

The type system is also a way to describe meaning. A value can be optional, a choice between
named alternatives, a number with a unit, or an object offering a particular interface.
Generics let operations preserve those distinctions while working across concrete types.

## At a glance

| Capability | What it gives you |
| --- | --- |
| Inference | Omit information the compiler can establish; keep explicit types for meaningful boundaries. |
| Optionals | `T?` describes presence or absence; `??` supplies a fallback. |
| Generics | `List'Int`, `[Int]`, and `<Int>` preserve item-type information. |
| Alternatives | Unions model domain choices; `Or` families accept several source types. |
| Conversions | Types can adapt values implicitly or through an explicit `as` expression. |

## Inference and explicit intent

:::example-pair

- [Inferred locals](#source-inference--inferred): Initializers supply the types.
- [Explicit representation](#source-inference--explicit): The declaration chooses a wider integer. Function results and, with sufficient context, parameters can also be inferred.

```yumi #inference
⟦inferred⟧. count = 3
. title = "Report"⟦/inferred⟧
⟦explicit⟧. total I64 = count⟦/explicit⟧
-- "$title: $total"
```

:::

Inference does not make a value dynamically typed. An operation still needs a suitable
implementation for the types involved. The source simply does not have to spell out every
piece of information the compiler can establish.

That flexibility is particularly useful for small helpers. Explicit parameter types become
more valuable when a signature is meant to communicate a stable contract to other readers.

## Absence is represented in the type

An optional type adds the possibility that a value is absent:

```yumi #optional
. nickname ⟦type⟧String?⟦/type⟧
-- ⟦fallback⟧nickname ?? "Anonymous"⟦/fallback⟧
```

- [Optional type](#source-optional--type): A string may be present or absent. An empty string is still present.
- [Fallback](#source-optional--fallback): Use the nickname when present, otherwise `"Anonymous"`.

A conditional expression can produce an optional value, and `match?` can return an optional
result when only some shapes matter. This lets absence flow through an expression without
inventing a sentinel such as an empty string or a negative number.

The force operator `!` asserts that an optional is present. It is useful when a stronger
invariant exists, but it transfers responsibility to the author. Where absence is ordinary,
a fallback, branch, or optional match says more about the intended behavior.

Absence is also distinct from a recoverable error. A lookup can reasonably find nothing;
a network request can fail to obtain an answer. [Errors](errors.md) explains the separate
failure channel and its automatic propagation.

## Generics preserve useful information

Apostrophes attach type arguments to parameterized types: `List'Int` is a list of integers,
and `Map'String'Int` maps strings to integers. Familiar shapes have shorthand, including
`[Int]` for a list and `<Int>` for the stream family.

A generic function can keep its item type while accepting different inputs:

:::example-pair

- [Different callers](#source-generic-stream--calls): One call produces integers, the other characters.
- [Type parameter](#source-generic-stream--parameter): `Item` connects the input type to the yielded values.
- [Source family](#source-generic-stream--family): Accept any supported stream representation without converting every item to a common boxed value.

```yumi #generic-stream
⟦calls⟧-- List(twice([1, 2]))
-- List(twice("ab"))⟦/calls⟧

━━━━━━━━━━━━━━━━━━━━
+ do ⟦parameter⟧twice'Item⟦/parameter⟧
    - values ⟦family⟧<Item>⟦/family⟧

for value in values
    <= value
    <= value
```

:::

Generic constraints let a declaration require a capability or a type family. The important
question is what the implementation needs: indexing, arithmetic, comparison, iteration, or
some domain-specific interface. Generics are most useful when that requirement stays smaller
than a particular implementation's complete API.

## Alternatives have more than one role

A union describes a value with alternatives. It can name cases and attach data to them, as
in a response that is either pending or complete. An `Or` type describes an accepted family
of types. The stream declaration is an important example:

```yumi #source-family
━━━━━━━━━━━━━━━━━━━━
+ Stream'T is ⟦alternatives⟧CursorIterable'T || Indexable'T || Iterable'T || Broadcastable'T || ImmutableArray'T⟦/alternatives⟧
```

- [Accepted alternatives](#source-source-family--alternatives): The family joins supported ways to obtain items. It does not introduce named domain states such as `Download.pending`.

This allows a stream-oriented function to accept sources with different iteration mechanisms.
It is a different purpose from introducing named domain states such as `Download.pending`.
The [Sum types](sum-types.md) and [Streams](streams.md) chapters show those two uses in context.

Interfaces provide another form of generality: an operation asks for a capability that multiple
types can implement. Choosing among an interface, a union, and an accepted type family depends
on whether the program is describing shared behavior, distinct alternatives, or supported inputs.

## Matching refines what is known

A type test or pattern can establish more specific information about a value. Inside a branch
for one union case, the program can use the payload that belongs to that case. A nested pattern
can recognize a map entry whose value is a particular kind of collection.

The useful consequence is local clarity. A branch can operate on the value its condition
established, rather than repeat casts and defensive checks. See
[Structural pattern matching](structural-pattern-matching.md) for recognition and extraction.

## Conversions are part of composition

Yumi converts a value implicitly where its meaning is kept: an integer becomes an integer type
that holds every value of its own, or a float, a class becomes its base, a value becomes an
optional or a union that holds it, and an optional where a truth is wanted tells whether it holds
a value. Types can define their own conversion behavior. A conversion that can lose meaning or
fail is written: `as` or a call of the type asks for one, `!` and `??` unwrap an optional, and a
pattern narrows. A parameter marked `~` takes any value `as` converts. A value becomes text on its
own only inside a string template or a print, so an argument or an assignment never turns a
number into text.

Integers are 64 bits unless a program asks for fewer: `Int` is `I64`, and `Size` and `Index` are
signed 64-bit counts and positions. A smaller type such as `U32` is for storage that should stay
compact, and a value narrows into it with `as`. An integer written with no suffix takes the type of
the integer beside it or of the place it is given to, when that type holds it, so literals rarely
need a suffix or a cast.

Implicit conversion is convenient, but domain conversions can perform meaningful work. A signature
and its conversions should make the interpretation of an input predictable.

[Numerical units](numerical-units.md) shows a particularly useful application: different numeric
representations remain connected by declared unit relationships. The types participate in
ordinary expressions while preserving more meaning than a bare machine number would carry.

## What the types buy

The attraction is a combination of concise code and expressive contracts. A function can avoid
obvious annotations while still describing absence, capabilities, alternatives, or units at
its boundary. Those distinctions help both the compiler and the person reading the program.

Inference, conversions, and dispatch also mean that understanding a difficult call may require
looking beyond its local spelling. Yumi accepts that tradeoff; the guide's
[design discussion](features-you-may-not-like.md) considers it directly.
