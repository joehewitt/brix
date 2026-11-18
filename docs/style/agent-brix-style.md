# Brix Style Rules For Agents

Use these rules to review working Brix code. Cite violations by rule ID, such as `YS-04`.
These are style rules, not syntax requirements; justified exceptions are allowed when the rule's
tradeoff does not fit the code.

## YS-01: Put The Result Or Primary Effect First

In a pure function, state the result immediately after any guards or assertions. Put pure supporting
values below it as `..` lazy dependencies, especially when they are expensive, fallible, or derived.
In an imperative function, make the primary effect easy to find and keep setup subordinate to it.

Incorrect:

```yumi
━━━━━━━━━━━━━━━━━━━━
+ do display-name
    - first String
    - last String

. full = "$first $last"
= "[$full]"
```

Correct:

```yumi
━━━━━━━━━━━━━━━━━━━━
+ display-name
    - first String
    - last String

"[$full]"

.. full = "$first $last"
```

### Lazy dependencies and their cost

A local `..` binding is demand-driven and memoized, not uniformly implemented as a heap thunk. Its
initializer runs when the value is first needed, at most once during that binding's lifetime. An
unused lazy does not run. Both a successful value and a recoverable error result are memoized rather
than recomputed. The lifetime is fresh for each function invocation, for each iteration of a loop
whose body declares the lazy, or for each enclosing closure instance when the lazy is captured.

A lazy costs no more than an eager local in almost every shape. A lazy read once is evaluated at
that read. A lazy read several times, in branches or guards, in another lazy's initializer, or in a
closure compiles like an eager local, and a new object it holds stays off the heap the same way. An
unused, non-property lazy is eliminated. One shape still costs more: a lazy read inside a loop or
comprehension body checks its memo flag on every element.

So cost is rarely a reason to keep `do`. What a rewrite changes is when each initializer runs: at
the lazy's first read, and not at all on a path that never reads it.

- Keep `do` when the function's value is the result of an effect; see YS-10.
- Check that each lazy's first read is where its value should be taken. A first read after a call
  that changes what the initializer reads takes a different value, and lazy `<-` reads happen in the
  order of their first reads rather than their declarations. An initializer with a hidden effect,
  such as a call that fills a cache or index on first use, must also run before anything that reads
  what it fills. Keep `do` when no arrangement of the expression form puts each first read in the
  right place.
- Keep `do` when a lazy would be read inside the body of a hot loop or comprehension.
- Keep `do` when the expression form would nest a table inside an arm of another table instead of
  becoming one ordered table; see YS-02.
- `use`, `catch`, and tracked reads work in lazies and are no reason to keep `do`.

## YS-02: Keep One Decision In One Table

When one subject drives a decision, use one `match subject`. When independent conditions drive it,
use one bare `|?`/`??` table. Do not scatter one decision across early returns, nested conditionals,
or helpers unless separating it clarifies genuinely independent work.

`=?` is the right form for an early exit, the counterpart to `?!`: it leaves a function whose real
work follows, especially when several statements separate the exit from the result. Do not use a
run of `=?` lines to return each case of the function's value; that is one decision scattered
across returns. When a single exit sits directly before the result, the two may be combined as
`if condition then value else fallback`.

Choose `match` or `match?` according to whether no arm matching is intentional, including for a
match written as a step. Running its arms as steps does not make the match optional. Use `match?`
as a step only when execution should continue without an effect if no arm matches. Use `match`
when the arms are exhaustive.

Incorrect:

```yumi
━━━━━━━━━━━━━━━━━━━━
+ do stock-label
    - count Int

=? "empty" if count == 0
=? "low" if count < 3
= "ready"
```

Correct:

```yumi
━━━━━━━━━━━━━━━━━━━━
+ stock-label
    - count Int

match count
    |= 0
        "empty"
    |? it < 3
        "low"
    ??
        "ready"
```

For a table without one subject, write bare arms rather than matching a dummy Boolean:

```text
|? !is-enabled
    "disabled"
|? is-pressed
    "pressed"
??
    "ready"
```

A `|= null` arm narrows the subject in the arms after it, so a match can handle an absent value
first and then read its members directly:

```yumi
━━━━━━━━━━━━━━━━━━━━
+ stock-label
    - item Item?

match item
    |= null
        "missing"
    |? item.count == 0
        "empty"
    ??
        "${item.count} left"
```

## YS-04: Construct Pure Collections And Text As Values

Use streams, comprehensions, filters, reducers, feed expressions, `join of`, and templates for
pure collection and text construction. Use an imperative accumulator only when mutation or measured
cost is the point of the code.

Incorrect:

```yumi
━━━━━━━━━━━━━━━━━━━━
+ do labels
    - include-debug Bool
    - extras [String]

. result [String]
result += "status"
if include-debug
    result += "debug"
for extra in extras
    result += extra
result += "done"
= result
```

Correct:

```yumi
━━━━━━━━━━━━━━━━━━━━
+ labels
    - include-debug Bool
    - extras [String]

* "status"
*? "debug" if include-debug
** extras
* "done"
```

Text follows the same rule. Write a function whose job is to produce text as a `¶` template: the
literal text is the body, and its values sit below the `════════════════════` boundary as `..`
lazies. Write optional text as a lazy such as `if draft then " (draft)" else ""`, and repeated lines as
`"\n".join(item-line(each) in items)`, calling a smaller template for each line. Keep a string or
line accumulator for character-level work such as escaping, encoding, and parsing (YS-10).

Incorrect:

```yumi
━━━━━━━━━━━━━━━━━━━━
+ do summary
    - title String
    - items [Item]
    - draft Bool

. text = title
if draft
    text += " (draft)"
for item in items
    text += "\n${item.name}: ${item.count}"
= text
```

Correct:

```yumi
━━━━━━━━━━━━━━━━━━━━
+ ¶ summary
    - title String
    - items [Item]
    - draft Bool

$title$status
$lines

════════════════════
.. status = if draft then " (draft)" else ""
.. lines = "\n".join(summary-line(each) in items)

━━━━━━━━━━━━━━━━━━━━
+ ¶ summary-line
    - item Item

${item.name}: ${item.count}
```

## YS-06: Let Inference Carry Obvious Local Types

Omit local types that inference makes obvious. Keep annotations where they select a necessary
unified type, describe a non-obvious domain value, or constrain storage. This rule does not cover
function return types: keep the return type a function declares.

Incorrect:

```yumi
━━━━━━━━━━━━━━━━━━━━
+ do doubled Int
    - value Int

. result Int = value * 2
= result
```

Correct:

```yumi
━━━━━━━━━━━━━━━━━━━━
+ doubled Int
    - value Int

value * 2
```

## YS-07: Cast Only Where A Number Narrows

Widening is implicit at arguments, fields, ranges, and container indexes, including from ID
aliases and `U32` to `Index` and `Size`. Write `as` where a value meets a type that may not hold
it, such as a `Size` where a `U32` id is wanted, and nowhere else. An integer written with no
suffix takes the type it is used as, so it needs neither a suffix nor `as`.

Incorrect:

```text
values[id as Index]
total += 1u32
```

Correct:

```text
values[id]
total += 1
. id SyntaxId = nodes.length as SyntaxId
```

## YS-08: Write Units In Numeric Literals

Use typed unit literals for time, angles, percentages, distances, and other unit-bearing numbers.
Do not obscure the unit with manual scaling arithmetic.

Incorrect:

```yumi
/time/duration

━━━━━━━━━━━━━━━━━━━━
+ retry-delay

500 * 1000 * 1000ns
```

Correct:

```yumi
/time/duration

━━━━━━━━━━━━━━━━━━━━
+ retry-delay

500ms
```

## YS-09: Use Grammars For Structured Text

When structured text is naturally described as a grammar, use a `~` grammar function rather
than manual splitting, cursor movement, or indexing. Keep direct string operations when the input is
not meaningfully grammatical.

Incorrect:

```text
━━━━━━━━━━━━━━━━━━━━
+ do parse-http-status-line HTTPStatusLine
    - source String

. parts = source.split(" ")
. version = parts[0]
. status = parts[1] as I32
= HTTPStatusLine(version, status, "")
```

Correct:

```text
+ HTTPStatus is I32

━━━━━━━━━━━━━━━━━━━━
+ HTTPStatusLine is (version String, status HTTPStatus, reason String)

────────────────────
+ This ~ Char

~ version (!" " .)+ // status HTTPStatus / reason (!\nl .)*
    This(version, status, reason)
```

## YS-10: Keep Effects And Low-Level Work Imperative

Use `do` and ordered statements for IO, mutation, buffers, cursors, cleanup, error paths, and other
effects. Do not conceal an effect in a lazy or nominally value-shaped expression. Manual low-level
code belongs only in the measured hot core; keep its boundaries idiomatic.

Incorrect:

```text
━━━━━━━━━━━━━━━━━━━━
+ save-message
    - path String
    - contents String

"saved"

.. ignored = write-file(path, contents)
```

Correct:

```text
━━━━━━━━━━━━━━━━━━━━
+ do save-message
    - path String
    - contents String

write-file(path, contents)
= "saved"
```

## YS-11: Use `use` For Locally Owned Resources

When a function acquires a resource and would unconditionally release it with `do finally`, make
the resource type `Closable`, or `Usable` when using it does more than close it at the end, and do
not write `begin-using` and `end-using` that only give the object and close it: `use` closes a
`Closable` that has no `begin-using`. Use `use resource` when the resource should remain live for
the rest of the scope. Use `do use resource` when its lifetime should end after a specific block,
before the function continues; nest blocks when one resource must close before another. Keep
`do finally` for conditional cleanup, ownership transfer, or restoration of surrounding state
that is not intrinsic to the resource.

## Domain-Specific Style Rules

### Markup And Style Sheets

Read and follow `docs/style/agent-brix-style-markup.md` when reviewing declarative markup,
components, style sheets, style shorthands, reactive UI structure, or markup rendering.

### Graphical UIs

Read and follow `docs/style/agent-brix-style-gui.md` when reviewing GUI applications or libraries,
especially app and window structure, controls, pointer and focus behavior, layout, custom painting,
retained rendering, or native GUI integration.

### Terminal UIs

Read and follow `docs/style/agent-brix-style-tui.md` when reviewing TUI applications or libraries,
especially keyboard shortcuts, focus behavior, component ownership, terminal rendering, canvas
updates, or compositing behavior.
