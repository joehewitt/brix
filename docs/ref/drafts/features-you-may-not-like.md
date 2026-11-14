# Features you may not like

Some things about Yumi take time to get used to. Others may be things you understand perfectly
well and still disagree with. This page is about the second category.

These are choices with consequences for how you structure programs, reason about execution,
and manage resources. Learning the notation does not make their tradeoffs disappear. I chose
them because they support the kind of programming I want Yumi to make comfortable, but they
also make Yumi less suitable for some preferences and requirements.

## At a glance

| Choice area | What may influence your decision |
| --- | --- |
| Source structure | Significant indentation and executable module bodies. |
| Type behavior | Implicit conversion, inferred signatures, operator overloading, multiple dispatch. |
| Object model | Garbage collection, inheritance, and virtual methods. |
| Control flow | Ordinary calls can wait or propagate recoverable errors. |

## 1. Python-like significant indentation

Indentation determines block structure. Moving a line can change behavior.

:::example-pair

- [Conditional work](#source-indentation--inside): The indented print runs only when `ready` is true.
- [Following work](#source-indentation--outside): This prints regardless. Indenting it would make it conditional too.

```yumi #indentation
. ready = false
⟦inside⟧if ready
    -- "Starting"⟦/inside⟧
⟦outside⟧-- "Finished"⟦/outside⟧
```

:::

**Why I chose it**

- Indentation is already how I read nested code. I want the visual grouping to be the compiler's grouping.
- Dividers establish declaration boundaries; indentation describes structure inside them.

**Tradeoffs**

- Whitespace can be fragile to copy and harder to inspect than explicit closing delimiters.
- Moving text and changing scope are closely connected. Formatting becomes part of correctness.
- If closing delimiters are central to how you navigate, this removes something useful.

## 2. Implicit type conversion

A supported conversion can happen because an assignment or argument expects another type.

:::example-pair

- [Source representation](#source-conversion--source): Begin with an integer.
- [Implicit conversion](#source-conversion--destination): The destination asks for floating point. Printing it produces `3.000000`.

```yumi #conversion
⟦source⟧. count Int = 3⟦/source⟧
⟦destination⟧. amount F64 = count⟦/destination⟧
-- amount
```

:::

**Why I chose it**

- When types already know the relationship, repeated casts can obscure the operation I want to express.
- Connected units are a useful case: a function can ask for seconds while a caller supplies milliseconds.

**Tradeoffs**

- Yumi permits integer narrowing as well as widening. Representation changes can lose precision.
- A float becomes an integer, a number becomes a `Bool`, and a value becomes text only where the code asks, with `as` or a string template.
- Custom conversions can perform work that the assignment does not visibly announce.
- API authors should reserve implicit conversions for predictable relationships; technical convertibility is not enough.

## 3. Garbage collection

Managed object memory uses garbage collection. Ordinary references do not require every caller to specify exactly who frees an object.

| Managed automatically | Still a program decision |
| --- | --- |
| Reclamation of managed object memory | How long references remain reachable |
| Ordinary sharing of managed references | When a socket, file, or subscription should stop |
| Collector-owned allocations | Lifetimes of arenas, raw pointers, and native allocations |

See [memory choices](modules-native-code-and-memory.md) and the [cleanup example](errors.md#source-cleanup--register).

**Why I chose it**

- I want sharing and object relationships to be ordinary: parent links, callbacks retaining objects, and several components referring to the same state.
- Automating memory reclamation avoids making every relationship an ownership-design problem.

**Tradeoffs**

- The collector uses runtime work and memory, and reclamation need not coincide with an object's last useful moment.
- Strict latency or memory requirements can make that default unsuitable.
- Reachable objects can retain unwanted memory. Files, locks, native allocations, and other resources still require deliberate lifetimes.

## 4. Operator overloading

Types can define operators for their values.

:::example-pair

- [Domain expression](#source-overloading--use): Add two offsets with ordinary arithmetic spelling.
- [Type-defined operator](#source-overloading--definition): `Offset` owns this meaning of addition.
- [Implementation](#source-overloading--answer): Combine coordinates, producing `6` and `8`.

```yumi #overloading
. combined = ⟦use⟧Offset(2, 3) + Offset(4, 5)⟦/use⟧
-- combined.x
-- combined.y

━━━━━━━━━━━━━━━━━━━━
+ Offset is Tuple

+ x Int
+ y Int

────────────────────
⟦definition⟧+ this + other Offset⟦/definition⟧

⟦answer⟧Offset(x + other.x, y + other.y)⟦/answer⟧
```

:::

**Why I chose it**

- Coordinates and quantities have established mathematical forms. I want expressions to preserve the shape of those calculations.

**Tradeoffs**

- `a + b` can allocate, traverse data, or invoke domain behavior; the symbol does not state cost.
- An inappropriate overload misleads readers. I expect `+` to mean a recognizable addition or combination.
- Use a named function when the operation needs information its symbol cannot carry.

## 5. Multiple dispatch

An implementation can depend on more than one argument's type rather than one distinguished receiver.

:::example-pair

- [Shared operation name](#source-dispatch--calls): The calls produce `5` and `Yumi!`.
- [Numeric combination](#source-dispatch--numbers): Declare the implementation for two integer inputs.
- [Text combination](#source-dispatch--strings): Declare the implementation for two strings. These concrete calls do not require runtime selection.

```yumi #dispatch
⟦calls⟧-- combine(2, 3)
-- combine("Yumi", "!")⟦/calls⟧

━━━━━━━━━━━━━━━━━━━━
⟦numbers⟧+ combine Int
    - left Int
    - right Int⟦/numbers⟧

left + right

━━━━━━━━━━━━━━━━━━━━
⟦strings⟧+ combine String
    - left String
    - right String⟦/strings⟧

left + right
```

:::

**Why I chose it**

- Combining values, converting representations, and handling interactions can naturally concern several participants.
- Separate definitions describe the relevant combinations without making one type own every decision.

**Tradeoffs**

- Finding the called implementation may require argument types, visible overloads, and dispatch rules.
- New overloads can change selection or introduce ambiguity; implicit conversions add another consideration.
- This is less local than a single receiver. Good diagnostics, navigation, and restrained API design matter.

## 6. Async functions are not “colored”

“Function coloring” separates async declarations and awaited calls from ordinary ones. Yumi allows suspension through ordinary calls. This fragment assumes `/net/http` and a `URL` named `url`.

:::example-pair

- [Caller](#source-network-call--caller): Print the returned body using an ordinary call.
- [Helper](#source-network-call--helper): The declaration needs no `async` marker.
- [Network operation](#source-network-call--request): The request can suspend; neither level needs `await`.

```yumi #network-call
⟦caller⟧-- page-body(url)⟦/caller⟧

━━━━━━━━━━━━━━━━━━━━
⟦helper⟧+ page-body String
    - url URL⟦/helper⟧

page.text

.. page = ⟦request⟧http-get(url)⟦/request⟧
```

:::

The request follows the repository's [HTTP client example](../../../examples/net/http-client.yumi). See [Concurrency and reactivity](concurrency-and-reactivity.md) for the task model.

**Why I chose it**

- I want to introduce helpers without propagating an asynchronous calling convention through every layer.
- Sequential source remains useful for a sequence of operations even when execution waits between them.

**Tradeoffs**

- The call site loses a visible suspension warning. Reconsider state observed before a wait and resources held across it.
- Removing `await` does not make shared mutable state safe or make an operation atomic.
- It also does not make every native blocking call cooperate with the scheduler. Timing remains part of the API contract.

## 7. Errors propagate automatically

A fallible call propagates errors unless a caller chooses a handling policy.

:::example-pair

- [Declared error](#source-propagation--contract): The signature exposes the possible escaping failure.
- [Ordinary-looking call](#source-propagation--call): Add on success; otherwise propagate without a call-site marker.
- [Recovery policy](#source-propagation--policy): Discard the error details, then replace absence with zero. This call prints `0`.

```yumi #propagation
-- ⟦policy⟧catch? adjusted-count(false) ?? 0⟦/policy⟧

━━━━━━━━━━━━━━━━━━━━
+ CountFailure is Error

━━━━━━━━━━━━━━━━━━━━
+ do read-count Int ?! CountFailure
    - available Bool

if available
    = 41
?! CountFailure("unavailable")

━━━━━━━━━━━━━━━━━━━━
⟦contract⟧+ adjusted-count Int ?! CountFailure⟦/contract⟧
    - available Bool

⟦call⟧read-count(available) + 1⟦/call⟧
```

:::

**Why I chose it**

- Many callers can only pass an error along. Repeating that instruction can overwhelm the calculation.
- I want handling to stand out where a caller can recover, add context, or inspect the outcome.

**Tradeoffs**

- An ordinary call does not announce a possible early exit. Explicit checking or propagation markers make that fact more local.
- A fallback is a domain decision: zero must actually be acceptable for an unavailable count.
- Propagation does not undo earlier mutations. Cleanup and partial progress still need design.

## 8. Class inheritance and virtual methods

Classes can inherit behavior and override methods selected according to the actual object.

:::example-pair

- [Base-typed reference](#source-inheritance--base): The variable is declared as `Greeting`.
- [Virtual behavior](#source-inheritance--call): The actual object supplies the implementation.
- [Specialization](#source-inheritance--override): The override returns `Welcome`.

```yumi #inheritance
⟦base⟧. greeting Greeting = FriendlyGreeting()⟦/base⟧
-- ⟦call⟧greeting.text()⟦/call⟧

━━━━━━━━━━━━━━━━━━━━
+ Greeting

────────────────────
+ text String

"Hello"

━━━━━━━━━━━━━━━━━━━━
⟦override⟧+ FriendlyGreeting is Greeting

────────────────────
+ text String

"Welcome"⟦/override⟧
```

:::

**Why I chose it**

- Some families share a useful foundation of state and behavior.
- Virtual methods keep the caller's contract common while allowing specialized behavior, without repetitive forwarding.

**Tradeoffs**

- Behavior spreads across the hierarchy, and base decisions can constrain subclasses.
- Changing shared behavior can affect distant code. A virtual call may reach a different implementation than the declared type suggests.
- Composition, interfaces, and unions remain alternatives. Wanting a few methods is not enough reason to inherit.

## 9. Function parameter and return types can be inferred

Inference can apply at function boundaries as well as local bindings.

:::example-pair

- [Concrete uses](#source-inferred-signature--calls): Calls supply integer and string inputs.
- [Inferred boundary](#source-inferred-signature--signature): The declaration omits parameter and return types while remaining statically typed.

```yumi #inferred-signature
⟦calls⟧-- identity(42)
-- identity("hello")⟦/calls⟧

━━━━━━━━━━━━━━━━━━━━
⟦signature⟧+ identity
    - value⟦/signature⟧

value
```

:::

**Why I chose it**

- For small computations, annotations can duplicate what inputs and the result already establish.
- Straightforward reusable helpers should not need repetitive type information everywhere.

**Tradeoffs**

- An explicit signature is a readable contract and can constrain accidental interface changes.
- Inference can make readers inspect the body or tooling, and a diagnostic may appear at a use site.
- I still favor explicit types for public contracts, domain boundaries, and non-obvious results. The language permits inference; it does not require minimal signatures.

## 10. Executable code at module scope

A file can execute statements without wrapping them in a special entry-point function.

:::example-pair

- [Start with the work](#source-module-code--work): The module prints `Hello, Yumi`.
- [Add structure](#source-module-code--helper): The same file defines a reusable helper.

```yumi #module-code
⟦work⟧. name = "Yumi"
-- greeting(name)⟦/work⟧

━━━━━━━━━━━━━━━━━━━━
⟦helper⟧+ greeting String
    - name String⟦/helper⟧

"Hello, $name"
```

:::

**Why I chose it**

- A small utility should begin with what it does; an entry-point wrapper need not be the first concept.
- Functions and types can appear as the program needs structure.

**Tradeoffs**

- A module is not necessarily passive: initialization can perform IO, change state, or fail.
- Imports and startup dependencies become behavioral questions.
- Substantial startup work benefits from named operations and deliberate sequencing; module code can be the small visible place that calls them.

## What these choices ask of the reader

Several of these decisions let a short expression stand for behavior that another language
would spell out at the call site. A conversion may be inserted, an implementation selected,
a task suspended, or an error propagated. I want that support because repeated mechanical
steps can make the program's purpose harder to see.

The corresponding obligation is that the behavior must remain discoverable. Types,
declarations, documentation, diagnostics, and tools have to answer the questions that a
short call expression leaves open. Concision is useful only when it does not turn necessary
information into guesswork.

These tradeoffs will not suit everyone. I would rather explain what I chose and why than
pretend that disagreement is only a matter of getting used to a new language.
