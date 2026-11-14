# Modules, native code, and memory

Yumi's expression-oriented surface sits on a native foundation. Programs are organized into
modules, compiled to native code, and able to call C libraries. Garbage collection handles
ordinary object memory, while explicit storage facilities provide more control for code
with a different lifetime or layout requirement.

These capabilities let a program use high-level models without treating operating-system
interfaces and data representation as somebody else's problem.

## At a glance

| Capability | What it gives you |
| --- | --- |
| Modules | Bare-path imports, visibility markers, and executable module bodies. |
| Native compilation | Concrete uses guide specialization, inlining, and other optimizations. |
| C bridge | Headers, native functions and data, pointers, and callbacks. |
| Managed memory | Garbage collection for ordinary object relationships. |
| Explicit storage | Arenas, backing storage, and GC-free regions for specific lifetime requirements. |
| Language tooling | Source, syntax-tree, type-analysis, and compiler facilities. |

## Modules use paths directly

Imports are bare paths at the beginning of a source file:

:::example-pair

- [Library modules](#source-imports--library): Absolute module paths identify declarations through Yumi's module lookup.
- [Nearby module](#source-imports--relative): Resolve a module relative to the current file. Public `+` and private `_` declarations control exposure.

```yumi #imports
⟦library⟧/net/http
/net/url⟦/library⟧
⟦relative⟧./response⟦/relative⟧
```

:::

A module can contain executable code as well as declarations. Small programs can therefore
begin with their work directly, without introducing a special entry-point function merely
to call another function. A larger module can define types and helpers and keep its top-level
work small.

Top-level execution is convenient, but initialization is still behavior. Import relationships,
global state, and effects should remain understandable as a program grows. A module that mainly
supplies reusable declarations has a different role from one that starts an application.

## Compilation follows the program's use of declarations

Yumi compiles to native code. Its toolchain includes its own native-code machinery rather than
using LLVM as its compiler backend. On-demand compilation lets the compiler work from the
declarations and concrete uses needed by the program.

That is especially relevant to generics and overloads. Source declarations describe families
of operations; a particular program supplies concrete combinations of types and calls.
Whole-program information gives the compiler opportunities to specialize, inline, and remove
intermediate machinery.

An abstraction's appearance in source does not dictate one fixed runtime representation.
A composed stream operation may be fused with its consumer, while a generator that escapes
and resumes independently may need stored state. The intended behavior remains the contract;
the compiler can choose how to realize it.

Native compilation does not by itself mean that a program has no runtime support. Tasks,
garbage collection, IO, and other facilities have runtime responsibilities. Likewise, platform
integration still depends on which backend and library facilities are available for that platform.

## The C bridge reaches existing native libraries

Yumi can import C declarations through headers and call them through its C namespace. This
fragment shows the shape of a C-library use:

:::example-pair

- [C declarations](#source-native-call--header): Import the header that supplies the foreign declaration.
- [C namespace](#source-native-call--call): Call the native function with a value supplied by the surrounding program.

```yumi #native-call
⟦header⟧/stdlib.h⟦/header⟧

⟦call⟧C:abs(value)⟦/call⟧
```

:::

The bridge includes native function and data types, pointers, and the declarations needed to
interoperate with existing APIs. It lets a Yumi library wrap an operating-system or third-party
facility and expose an interface suited to ordinary Yumi code.

A native wrapper should settle these questions near the bridge:

| Concern | Contract to expose |
| --- | --- |
| Representation | How Yumi values map to native data |
| Ownership | Whether a pointer is borrowed, owned, or managed by another library |
| Lifetime | When an allocation or handle must be released |
| Failure | How native failure becomes an ordinary Yumi outcome |
| Callbacks | How long native code can retain the callback and its captured state |

## Garbage collection is the ordinary memory model

Yumi normally manages object memory with garbage collection. Values can be retained by data
structures and shared between parts of a program without requiring every function signature
to describe a manual allocation protocol.

Memory reachability is not the same thing as resource ownership. A socket, file, subscription,
or worker can need an explicit end to its activity even when the language manages the memory
of its wrapper. Cleanup belongs to the operation or component that owns that resource.

[Errors](errors.md) covers `do finally` and cleanup during exits. The stream and concurrency
chapters describe the corresponding lifetime decisions for traversals and producers.

## Explicit memory for explicit lifetimes

| Storage choice | Useful when | Obligation |
| --- | --- | --- |
| Garbage-collected objects | Values have shared, interdependent lifetimes | Avoid unwanted retention |
| Arena-backed data | Many values belong to one computation | Do not use values after the arena is released |
| Native allocation or handle | A foreign API owns the representation | Follow that API's release and borrowing rules |

Builders and collections can use storage appropriate to their owner.

This can make temporary compiler data, parsing work, or other bounded computations easier to
reason about as a group. It also creates an obligation: values backed by that storage must
not be used after the storage is released. Raw pointers and native interoperation require
similarly explicit reasoning.

Yumi also has facilities for expressing GC-free regions of implementation. These are useful
when avoiding garbage-collected allocation is an actual requirement, rather than a preference
that should shape every ordinary helper.

[Collections and data layout](collections-and-data-layout.md) surveys arrays, views, bitmaps,
and tables. Their purpose is complementary: one decision concerns how long storage exists,
and another concerns how the values are arranged within it.

## Language tooling is part of the project

Yumi's library includes parsing, syntax-tree, type-analysis, and compiler-related facilities,
with ongoing self-hosted compiler work. These make the language itself a domain that Yumi
programs can work on. A language guide can show that capability without requiring readers
to learn the compiler's internal passes.

The combination is useful to evaluate as a whole: a program can begin with ordinary modules,
types, and managed objects, then reach native APIs or choose a more explicit storage model
where the problem calls for one.
