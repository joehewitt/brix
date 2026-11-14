# Classes, interfaces, and dispatch

Yumi supports object-oriented programming alongside tuples, unions, and generic functions.
Classes can own state and behavior, interfaces describe capabilities, and dispatch selects
an implementation suited to the values involved in a call.

The interesting choice is how much structure a problem needs. Some data is naturally a tuple.
Some behavior belongs to an object with identity and a lifetime. Some operations describe a
relationship between several types and fit better as overloaded functions.

## At a glance

| Capability | What it gives you |
| --- | --- |
| Tuple | Group value fields and give them behavior. |
| Class | Model identity, changing state, and virtual methods. |
| Interface | Ask for a capability without choosing its implementation. |
| Inheritance / delegation | Share a base implementation, or forward behavior to a contained value. |
| Dispatch / operators / extensions | Define operations around the types and relationships involved. |

## Data can carry behavior

:::example-pair

- [Value type](#source-value-method--type): `Point` represents a small value whose meaning comes from its contents.
- [Shared shape](#source-value-method--fields): Every point contains two coordinates.
- [Behavior](#source-value-method--method): The method reads those fields; the call produces `(3, 4)`.

```yumi #value-method
. point = Point(3, 4)
-- point.label()

━━━━━━━━━━━━━━━━━━━━
⟦type⟧+ Point is Tuple⟦/type⟧

⟦fields⟧+ x Int
+ y Int⟦/fields⟧

────────────────────
⟦method⟧+ label

"($x, $y)"⟦/method⟧
```

:::

Yumi also provides structs and explicit collection layouts. The representation question is
covered further in [Collections and data layout](collections-and-data-layout.md); a type's
methods and its storage needs are related decisions, but they are not the same decision.

## Objects can own changing state

:::example-pair

- [Stored state](#source-state--state): The object owns the counter.
- [Changing behavior](#source-state--mutation): `advance` updates the field, so the subsequent read prints `1`.

```yumi #state
. counter = Counter()
counter.advance()
-- counter.count

━━━━━━━━━━━━━━━━━━━━
+ Counter

⟦state⟧+ count Int⟦/state⟧

────────────────────
+ do advance

⟦mutation⟧count += 1⟦/mutation⟧
```

:::

The object owns the counter and exposes an operation that changes it. Constructors use the
name `This`, and a type can provide constructors appropriate to different inputs. Properties
can expose stored data or computed getters; their syntax lets callers read a value through
the same property access form.

That convenience makes API design important. A property read should have a sensible cost
and effect model for the object it belongs to. A getter is not automatically a field access,
and an object's public properties need not expose its complete internal representation.

## Constructors initialize fields with `..`

```yumi
. log = Log("build")
-- log.lines.length, log.title

━━━━━━━━━━━━━━━━━━━━
+ Log

+ title String
+ lines [String]

────────────────────
+ This
    - name String

lines += "started $name"
title = name

.. lines = List'String()
```

A `..` line in a constructor that names a field gives the field its starting value. It replaces
the field's declared default, so the default is never built. Every such initializer runs once,
before the first statement of the constructor body, wherever it is written. The body then reads
and writes the field itself, so `lines` above already holds a list when the body appends to it.

An initializer can read constructor parameters and other fields. It cannot read a field that the
constructor body assigns: the initializer would see the value from before the assignment, so the
compiler rejects it.

## Interfaces describe a capability

```yumi #capability
-- describe(Label("Ready"))

━━━━━━━━━━━━━━━━━━━━
⟦contract⟧+ Described is Interface⟦/contract⟧

────────────────────
+ description String

━━━━━━━━━━━━━━━━━━━━
+ Label is Tuple
    ⟦implementation⟧is Described⟦/implementation⟧

+ text String

────────────────────
+ description

text

━━━━━━━━━━━━━━━━━━━━
+ describe
    ⟦consumer⟧- value Described⟦/consumer⟧

value.description()
```

- [Contract](#source-capability--contract): The interface requires a description.
- [Implementation](#source-capability--implementation): `Label` supplies the capability using its stored text.
- [Consumer boundary](#source-capability--consumer): `describe` needs the capability, not the concrete class. Other implementations can carry different state.

This is useful at boundaries where callers need a small capability. A renderer may need
something drawable; a stream consumer needs a supported way to obtain items. Neither should
necessarily depend on one concrete storage or implementation choice.

An interface value can be narrowed back to a concrete implementer with `is`, `as`, or a `match`
type pattern, and the test also accepts subclasses of the named class. Argument dispatch never
narrows implicitly: a function that takes a concrete class does not accept an interface value.

An interface can declare `using SomeStyle`. Style reads such as `value.$fill` and imperative
style writes such as `value.fill = paint` then work directly on interface values, and every
implementing class must use the same styles.

## Inheritance and virtual methods

A class can derive from another class, share its behavior, and override methods. Calls through
the base type can then reach an implementation supplied by the actual object.

Inside a class's methods, `This` names the class that declares them, the same type `this` has
there. A subclass inherits those methods as written, so a base method that returns `This`
returns the base type even when it is called on a subclass.

- **Useful when:** a family shares a conceptual identity and useful behavior.
- **Cost:** subclasses depend on decisions in the base class.
- **Contract:** overrides should preserve callers' expectations about effects, assumptions, and cost.

An interface can express common behavior without requiring the same class hierarchy. A union
can express a closed set of alternatives when callers should consider each case explicitly.
Yumi supplies all three approaches; [Sum types](sum-types.md) shows the latter in detail.

## Delegation makes composition convenient

An object can contain another value and expose selected behavior through it. Yumi has
declaration syntax for forwarding interfaces to stored members. A stream wrapper, for
example, can expose a readable capability through its input member and a writable capability
through a different output member.

This reduces repetitive forwarding methods while keeping the ownership structure visible:
the enclosing object has an implementation it uses, rather than needing to inherit all of
that implementation's identity. The wrapper can still add its own state and operations.

Delegation also raises a lifetime question. Forwarding a capability does not by itself say
whether the outer object owns the delegated resource, shares it, or merely borrows access.
That relationship belongs in the component's contract.

## More than one argument can select behavior

Overloaded functions allow implementations specialized for different parameter types.
Multiple dispatch makes the types of several arguments relevant, rather than requiring
every operation to be organized around one distinguished receiver.

For geometry, the relationship can select the algorithm:

| Inputs to an intersection operation | Implementation needed |
| --- | --- |
| Line and line | Line intersection |
| Line and curve | Line–curve intersection |
| Curve and curve | Curve intersection |

The combinations can share one operation name. See the [overload example](features-you-may-not-like.md#source-dispatch--numbers) for the declaration shape.

This can be more direct than asking one object to discover the second object's type and
manually branch. It also makes the set of available overloads part of understanding a call.
Ambiguous or overly broad definitions can make that set harder to reason about.

## Operators and extensions

Operators can be implemented by types. Adding two vectors or comparing two domain values can
therefore use ordinary expression syntax. Named infix operators offer a readable name when
the relationship does not fit a familiar symbol.

Extensions let behavior be declared outside the original type body. This is useful when an
operation belongs to a library's use of a type, rather than to the type's original definition.
Alongside conversions and overloads, extensions give libraries room to integrate with existing
values without requiring every integration to be built into the base class.

The benefit is expressive composition. The cost is that an expression's meaning may depend
on declarations elsewhere in the program. Good names, focused interfaces, and restrained
overloading make that tradeoff easier to live with.
