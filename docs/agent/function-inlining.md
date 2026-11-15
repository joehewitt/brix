# Ordinary function inlining

Place `@inline` after a function's return type and declared errors, alongside its other
trailing annotations. It requests expansion during ordinary compilation and does not change
the function's effects, overload ranking, generic inference, visibility, or dispatch.
Block getters accept the same trailing annotations, for example `+ get width @inline`.
Conversion members accept them too, for example `+ to Int @inline`.

```yumi
━━━━━━━━━━━━━━━━━━━━
+ increment Int @inline @no-gc
    - value Int

value + 1
```

The compiler expands statically resolved ordinary calls in typed IR after language semantics,
cleanup paths, and argument conversions have been resolved. Arguments can include scalar values,
managed references, tuples, and structs. Tuple parameters and receivers retain their typed value
representation through expansion, before machine-level argument marshalling. Instantiated
generic functions and resolved ordinary methods can expand across
module boundaries. Each expansion gets fresh variables, registers, blocks, and return joins.
Immutable parameters become SSA values, including rebinding through branches and loops, without
field-count or managed-reference limits. Tuple field snapshots preserve pass-by-value semantics
when caller storage can change; field projection removes unnecessary aggregate materialization.
Mutable struct parameters retain the copies needed to isolate callee mutations. Normal inline
returns bypass call-ABI panic signalling and polling; a panic exit branches to the caller's landing
pad, as a call's table entry sends it there.
Existing argument evaluation, defaults, local mutation, loops, error results, cleanup, panic
signals, GC roots, and surviving calls retain their ordinary semantics.

Known closure arguments expand with their annotated consumer. Callback aliases, forwarding,
returned closures, and control-flow joins retain their known targets. Other known calls of the
same callback in the caller can expand too, including calls before the annotated consumer.
Nested callback calls and implicit `each` callbacks follow the same rules. A join with an
unknown runtime callback keeps an indirect call for that alternative.

Closure capture lowering is prepared only for expansion targets that need captured storage.
The caller is prepared when one of those targets is actually expanded. Unrelated closures keep
their ordinary representation, including siblings that share an environment with an expanded
callback. Scalar-only inlining does not trigger capture lowering or callback-variable promotion.

Captured values are evaluated at their original creation point. Expanded calls share mutable
capture storage with surviving ordinary calls and escaped aliases. Separate closure creations
keep separate environments. When no surviving use needs an environment, its fields become
caller values or local storage and the allocation disappears. Immutable captures selected from
different known environments can become value joins. Identity comparisons fold only when the
identity is known; escaping and opaque uses retain the shared closure representation.

`@inline` requests full expansion without depth or instruction-count budgets. Unannotated
calls outside the resolved callback and interface expansion remain ordinary calls. The compiler detects direct and mutual recursion before copying
any body, including cycles through unannotated functions. Calls to a recursive function remain
entirely out of line and produce a missed-inline warning; the pass never partially expands a
recursive function.

Unsupported requests retain normal calls. These include unresolved dynamic dispatch,
unavailable generator origins, table row adapters, and forwarded returns whose
representation differs from the declared result. Compatible aggregate results, including
ordinary declared-error results, use the existing IR merge and copy rules. Generator makers
are treated as generator requests; expanding only a maker is not generator inlining.

An annotated generator consumed by a visible `for` loop is fused into that loop. The compiler
preserves a typed producer body before suspension lowering replaces yields with frame stores
and optional results. Each yield runs a fresh copy of the consumer body, then jumps directly
to its producer continuation. The fused instance has no maker allocation, generator frame,
resume dispatch, or per-item optional-result protocol. Loop aliases, compatible interface
views, annotated forwarding, and branches retain known producer origins. A mixed branch keeps
the ordinary protocol for its opaque alternative. Separate instances may fuse independently;
an escaped or repeatedly consumed instance keeps its ordinary implementation.

Argument conversions and tuple snapshots remain at generator creation. Producer entry stays
lazy until consumption, and yielded mutable aggregates retain value-copy semantics. Producer
locals and consumer iterations share the original ordering of effects. Continue resumes the
producer; break, return, declared errors, and panic run the applicable suspended cleanup once.
Unconsumed producers do not enter their body or finalizers. Captured storage follows the same
closure rules as ordinary inlining, including escaped yielded closures and managed references.

Fusion composes with callback and interface expansion and with nested annotated generators.
When the consumer is itself a surviving generator, its optimized typed body is lowered back
into its own suspension frame. Inner generator storage disappears while the outer generator's
continuations and live values remain materialized. Recursion analysis includes the edge from
a generator maker to its resume implementation. Recursive generator cycles retain their
ordinary boundary without partial expansion. There is no yield-count, layout, nesting-depth,
or instruction-count cutoff.

Bullet expressions and comprehensions request fusion automatically. Their implicit map,
filter, and optional-map stages use the same generator lowering and fusion machinery.
A fully consumed chain can feed a visible loop or list conversion without intermediate
generator frames. List output storage remains. An escaping chain retains one outer
generator frame when its internal producer origins are known and exclusively consumed.
The compiler specializes the surviving generator with those origins before lowering its
suspensions. Generator arguments are still captured at creation, and body effects remain
lazy. Aliases, annotated helpers, and module boundaries follow the same rules as explicit
generator requests; opaque or shared producer origins retain their normal representation.

Exclusive use is checked across loop iterations as well as source call sites. Rebuilding a
pipeline stage in a loop from one existing producer retains that producer's identity and
exhausted state. Creating a fresh producer within each iteration still permits specialization.
Locals whose incoming values cross pending consumer loops stay in local storage until fusion
has finalized those paths, then become SSA values where supported. Each phi's incoming blocks
and values are extended independently when consumer paths are copied.

`--warnings` enables all compiler warnings. They are off by default, go to stderr, and remain
nonfatal. In a `.test` case, `%warnings` enables warnings and includes them in the expected
output, before stdout. `%warn` enables them without comparing their text.

A missed-inline warning identifies the surviving source call, target, reason, annotated
declaration, and any outer expansion context. Warnings are deduplicated by source call,
specialization, and reason. Successful calls and unused annotated declarations produce no
missed-inline warning. Unknown runtime function values do not produce guessed target warnings.
Warning settings are scoped to each compilation, including asynchronous compilation.
