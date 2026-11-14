# Yumi at a glance

Yumi brings together features that are often associated with different kinds of language:
static types and inference, objects and pattern matching, lazy expressions and imperative
procedures, native code and garbage collection, grammars and declarative interfaces.

Its central attraction is the way these features meet. A stream can contain numbers already
in memory or characters arriving from a socket. A pattern can turn those characters into
records. A pipeline can transform the records, and a live expression can expose the latest
result to an interface. Each step can use the form that expresses its own job clearly.

## At a glance

| Capability | What it gives you |
| --- | --- |
| Describe values | Static types, inference, tuples, classes, interfaces, unions, and numerical units. |
| Transform sequences | One stream family for stored collections, generators, channels, and publications. |
| Recognize input | Structural patterns for values; stream grammars for successive items. |
| Coordinate work | Tasks, channels, ordinary calls that can wait, and automatic error propagation. |
| Build interfaces | Markup, styles, events, and expressions that follow changing state. |
| Reach the machine | Native compilation, a C bridge, managed memory, and explicit storage choices. |

## Start with the result

Yumi often puts the answer first and its supporting calculations below it:

:::example-pair

- [Answer first](#source-result-first--answer): The body describes the returned text; no explicit return is needed.
- [Name a dependency](#source-result-first--join): A lazy binding explains how the text is assembled.
- [Filter and transform](#source-result-first--sequence): Keep odd inputs and square them. The call prints `Squares of odd values: 9, 25`.

```yumi #result-first
-- summary([2, 3, 4, 5])

━━━━━━━━━━━━━━━━━━━━
+ summary
    - values [Int]

⟦answer⟧"Squares of odd values: $text"⟦/answer⟧

⟦join⟧.. text = ", ".join(squares)⟦/join⟧
.. squares = ⟦sequence⟧each * each in values when each % 2 != 0⟦/sequence⟧
```

:::

The notation is intentionally distinctive. Dividers separate declarations, identifiers use
kebab case, types begin with capitals, and several common operations have symbolic forms.
After the initial adjustment, these conventions make declarations and operations visually
recognizable. [Syntax and expressions](syntax-and-expressions.md) introduces that vocabulary.

## Choose a model that fits the data

| Model | Use it to describe |
| --- | --- |
| [Classes and interfaces](classes-interfaces-and-dispatch.md) | State, behavior, and shared capabilities |
| [Tuples and unions](sum-types.md) | Values with several fields, or alternatives with different payloads |
| [Numerical units](numerical-units.md) | Quantities whose types carry their meaning |
| [Generic types](type-system.md) | Operations that preserve relationships across concrete types |

These choices work with static typing, but a program need not repeat every type that the
compiler can infer. Generics let an operation work across types while retaining the information
it needs. Pattern matching can use that information to recognize and unpack values.

A completed download, for example, can carry a path as part of its union case. A duration
can carry its unit in its type. Neither requires relying on a comment to explain what a
bare string or number is supposed to mean.

## Sequences connect stored data and ongoing work

The stream family accepts multiple representations of successive values. Consumers can work
over collections, generators, channels, and broadcasters. The operation can be written once
even when some sources must wait for their next item.

- [Stream pipelines](stream-pipelines.md) assemble, filter, and transform sequences.
- [Stream patterns](stream-pattern-matching.md) recognize sequences using grammar rules.
- [Structural patterns](structural-pattern-matching.md) inspect tuples, maps, lists, and other values.

## Waiting can pass through ordinary functions

An operation that waits for IO does not force every enclosing function to acquire an `async`
declaration or every call to acquire `await`. Tasks make concurrency explicit; ordinary calls
can still suspend the task performing them.

This keeps helper functions easy to compose, while making timing something the reader must
understand from an operation's behavior. Recoverable errors similarly propagate through calls
until a caller chooses to handle them. These choices reduce ceremony and also move some
responsibility into the design of APIs and their documentation.

## Interfaces are part of the language story

Yumi has markup for object trees, stylesheets, events, and tracked values. These are useful
for interfaces whose appearance and content follow application state. The supplied terminal
and graphical libraries provide concrete components on top of those language features.

The standard library reaches beyond interfaces: it includes networking, structured data,
files and storage, time, graphics, AI integrations, and language tooling. The
[Batteries included](batteries-included.md) chapter surveys those capabilities.

## High-level code with a native foundation

Yumi compiles to native code and provides a C bridge. Garbage collection is the normal memory
model, while arrays, tables, explicit backing storage, and lower-level memory facilities
provide more control where representation matters. Probes make observations about execution
part of the language's tooling story.

The guide explores these capabilities without assuming that every reader wants all of them.
For the deliberate compromises behind the design, start with
[Features you may not like](features-you-may-not-like.md). For the notation itself, read
[Why Yumi looks the way it does](syntax-choices.md).
