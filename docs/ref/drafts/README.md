# Yumi Language Guide

This guide is a tour of what Yumi can do, for readers deciding whether they want to try it.
Each chapter introduces a capability, shows what it looks like, and explains the choices it
opens up. It assumes familiarity with programming, but not with Yumi.

## The language

1. [Yumi at a glance](yumi-at-a-glance.md) — The ideas that come together in the language.
2. [Syntax and expressions](syntax-and-expressions.md) — Reading the notation; expression functions, lazy bindings, and expressive calls.
3. [String templates](string-templates.md) — Text-producing functions, interpolation, composition, and multiline documents.
4. [The type system](type-system.md) — Static typing, inference, optionals, generics, narrowing, and conversions.
5. [Classes, interfaces, and dispatch](classes-interfaces-and-dispatch.md) — Objects, tuples, inheritance, delegation, and overloading.
6. [Sum types and enums](sum-types.md) — Modeling alternatives and the data that belongs to each case.
7. [Structural pattern matching and destructuring](structural-pattern-matching.md) — Recognizing a value's shape and giving its parts names.
8. [Numerical units](numerical-units.md) — Unit literals, conversions, arithmetic, and percentages.
9. [Collections and data layout](collections-and-data-layout.md) — Lists, maps, arrays, views, bitmaps, and tables.
10. [Streams](streams.md) — A common model for stored, generated, and arriving values.
11. [Stream pipelines](stream-pipelines.md) — Constructing sequences and composing transformations.
12. [Stream pattern matching](stream-pattern-matching.md) — Grammars over text, bytes, and other sequences.
13. [Concurrency and reactivity](concurrency-and-reactivity.md) — Tasks, channels, events, and expressions that follow changing state.
14. [State machines](state-machines.md) — Named states, event-driven transitions, payloads, and state-owned lifetimes.
15. [Errors](errors.md) — Automatic propagation, recovery, panics, and cleanup.
16. [Markup and stylesheets](markup-and-stylesheets.md) — Declarative object trees, composition, styling, and live state.
17. [Modules, native code, and memory](modules-native-code-and-memory.md) — Program organization, compilation, C interoperability, and memory control.
18. [Probes and observability](probes-and-observability.md) — Observing execution, values, and object state.
19. [Batteries included](batteries-included.md) — A tour of the standard library's capabilities.

## The reasoning behind Yumi

20. [Why Yumi looks the way it does](syntax-choices.md) — Unusual syntax choices, alternatives, and tradeoffs.
21. [Features you may not like](features-you-may-not-like.md) — Deliberate choices that may influence whether Yumi appeals to you.

## Finding your way

Start with **Yumi at a glance** for the overall picture. The language chapters can then be read
in order or by interest. **Batteries included** answers the practical question of what kinds
of programs the supplied libraries help build. The design essays explain the reasoning and
tradeoffs behind the features shown throughout the guide.

The chapters give each feature a home. Related chapters introduce the connection and link
there for the fuller treatment: Streams explains sources, Stream pipelines explains sequence
construction and transformation, and the two pattern chapters distinguish inspecting an
existing value from recognizing successive input.
