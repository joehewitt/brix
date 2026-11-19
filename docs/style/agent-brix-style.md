# Brix Style Rules For Agents

Use these rules to review working Brix code. Cite violations by rule ID, such as `YS-04`.
These are style rules, not syntax requirements; justified exceptions are allowed when the rule's
tradeoff does not fit the code.

## YS-01: Put The Result Or Primary Effect First

In a pure function, state the result immediately after any guards or assertions. Put pure supporting
values in its `where` as `lazy` dependencies, especially when they are expensive, fallible, or
derived. In an imperative function, make the primary effect easy to find and keep setup subordinate
to it.

Incorrect:

```brix
(fn-do + display-name _
  (param first String)
  (param last String)
  (let full (str first " " last))
  (return (str "[" full "]"))
)
```

Correct:

```brix
(fn-expr + display-name _
  (param first String)
  (param last String)
  (str "[" full "]")
  (where
    (lazy full (str first " " last))
  )
)
```

### Lazy dependencies and their cost

A `lazy` binding is demand-driven and memoized, not uniformly implemented as a heap thunk. Its
initializer runs when the value is first needed, at most once during that binding's lifetime. An
unused lazy does not run. Both a successful value and a recoverable error result are memoized rather
than recomputed. The lifetime is fresh for each function invocation, for each iteration of a loop
whose body declares the lazy, or for each enclosing closure instance when the lazy is captured.

A lazy costs no more than an eager local in almost every shape. A lazy read once is evaluated at
that read. A lazy read several times, in branches or guards, in another lazy's initializer, or in a
closure compiles like an eager local, and a new object it holds stays off the heap the same way. An
unused, non-property lazy is eliminated. One shape still costs more: a lazy read inside a loop or
comprehension body checks its memo flag on every element.

So cost is rarely a reason to keep `fn-do`. What a rewrite changes is when each initializer runs: at
the lazy's first read, and not at all on a path that never reads it.

- Keep `fn-do` when the function's value is the result of an effect; see YS-10.
- Check that each lazy's first read is where its value should be taken. A first read after a call
  that changes what the initializer reads takes a different value, and lazy `<-` reads happen in the
  order of their first reads rather than their declarations. An initializer with a hidden effect,
  such as a call that fills a cache or index on first use, must also run before anything that reads
  what it fills. Keep `fn-do` when no arrangement of the expression form puts each first read in the
  right place.
- Keep `fn-do` when a lazy would be read inside the body of a hot loop or comprehension.
- Keep `fn-do` when the expression form would nest a table inside an arm of another table instead of
  becoming one ordered table; see YS-02.
- `use`, `catch`, and tracked reads work in lazies and are no reason to keep `fn-do`.

## YS-02: Keep One Decision In One Table

When one subject drives a decision, use one `(match subject …)`. When independent conditions drive
it, use one bare table of `case?` arms and a `default`. Do not scatter one decision across early
returns, nested conditionals, or helpers unless separating it clarifies genuinely independent work.

`return-if` is the right form for an early exit, the counterpart to `throw` with a condition: it
leaves a function whose real work follows, especially when several statements separate the exit
from the result. Do not use a run of `return-if` rows to return each case of the function's value;
that is one decision scattered across returns. When a single exit sits directly before the result,
the two may be combined as `(if-expr condition value fallback)`.

Choose `match` or `match?` according to whether no arm matching is intentional, including for a
match written as a step. Running its arms as steps does not make the match optional. Use `match?`
as a step only when execution should continue without an effect if no arm matches. Use `match`
when the arms are exhaustive.

Incorrect:

```brix
(fn-do + stock-label _
  (param count Int)
  (return-if "empty" (if (== count 0)))
  (return-if "low" (if (< count 3)))
  (return "ready")
)
```

Correct:

```brix
(fn-expr + stock-label _
  (param count Int)
  (match count
    (case= 0
      "empty"
    )
    (case? (< it 3)
      "low"
    )
    (default
      "ready"
    )
  )
)
```

For a table without one subject, write bare arms rather than matching a dummy Boolean:

```brix
(case? (not is-enabled)
  "disabled"
)
(case? is-pressed
  "pressed"
)
(default
  "ready"
)
```

A `(case= null …)` arm narrows the subject in the arms after it, so a match can handle an absent
value first and then read its members directly:

```brix
(fn-expr + stock-label _
  (param item (? Item))
  (match item
    (case= null
      "missing"
    )
    (case? (== (. item count) 0)
      "empty"
    )
    (default
      (str (. item count) " left")
    )
  )
)
```

## YS-04: Construct Pure Collections And Text As Values

Use streams, comprehensions, filters, reducers, feed expressions, `join`, and templates for pure
collection and text construction. Use an imperative accumulator only when mutation or measured cost
is the point of the code.

Incorrect:

```brix
(fn-do + labels _
  (param include-debug Bool)
  (param extras (list String))
  (let result (list String))
  (+= result "status")
  (if include-debug
    (+= result "debug")
  )
  (for (in extra extras)
    (+= result extra)
  )
  (+= result "done")
  (return result)
)
```

Correct:

```brix
(fn-expr + labels _
  (param include-debug Bool)
  (param extras (list String))
  (item "status")
  (item? (only-if "debug" (if include-debug)))
  (spread extras)
  (item "done")
)
```

Text follows the same rule. Write a function whose job is to produce text as a text body: the
literal text is the body, a `(paragraph …)`, and its values sit in its `where` as `lazy` bindings.
Write optional text as a lazy such as `(if-expr draft " (draft)" "")`, and repeated lines as `(call
(. "\n" join) (in (call item-line each) items))`, calling a smaller template for each line. Keep a
string or line accumulator for character-level work such as escaping, encoding, and parsing (YS-10).

Incorrect:

```brix
(fn-do + summary _
  (param title String)
  (param items (list Item))
  (param draft Bool)
  (let text title)
  (if draft
    (+= text " (draft)")
  )
  (for (in item items)
    (+= text (str "\n" (. item name) ": " (. item count)))
  )
  (return text)
)
```

Correct:

```brix
(fn-expr + summary _
  (param title String)
  (param items (list Item))
  (param draft Bool)
  (paragraph
    (str title status)
    (str lines)
  )
  (where
    (lazy status (if-expr draft " (draft)" ""))
    (lazy lines (call (. "\n" join) (in (call summary-line each) items)))
  )
)

(fn-expr + summary-line _
  (param item Item)
  (paragraph
    (str (. item name) ": " (. item count))
  )
)
```

## YS-06: Let Inference Carry Obvious Local Types

Omit local types that inference makes obvious. Keep annotations where they select a necessary
unified type, describe a non-obvious domain value, or constrain storage. This rule does not cover
function return types: keep the return type a function declares.

Incorrect:

```brix
(fn-do + doubled Int
  (param value Int)
  (let result Int (* value 2))
  (return result)
)
```

Correct:

```brix
(fn-expr + doubled Int
  (param value Int)
  (* value 2)
)
```

## YS-07: Cast Only Where A Number Narrows

Widening is implicit at arguments, fields, ranges, and container indexes, including from ID
aliases and `U32` to `Index` and `Size`. Write `as` where a value meets a type that may not hold
it, such as a `Size` where a `U32` id is wanted, and nowhere else. An integer written with no
suffix takes the type it is used as, so it needs neither a suffix nor `as`.

Incorrect:

```brix
([] values (as id Index))
(+= total 1u32)
```

Correct:

```brix
([] values id)
(+= total 1)
(let id SyntaxId (as (. nodes length) SyntaxId))
```

## YS-08: Write Units In Numeric Literals

Use typed unit literals for time, angles, percentages, distances, and other unit-bearing numbers.
Do not obscure the unit with manual scaling arithmetic.

Incorrect:

```brix
(import /time/duration)

(fn-expr + retry-delay _
  (* 500 1000 1000ns)
)
```

Correct:

```brix
(import /time/duration)

(fn-expr + retry-delay _
  500ms
)
```

## YS-09: Use Grammars For Structured Text

When structured text is naturally described as a grammar, use a grammar function, whose arms are
`(~ …)` forms, rather than manual splitting, cursor movement, or indexing. Keep direct string
operations when the input is not meaningfully grammatical.

Incorrect:

```brix
(fn-do + parse-http-status-line HTTPStatusLine
  (param source String)
  (let parts (call (. source split) " "))
  (let version ([] parts 0))
  (let status (as ([] parts 1) I32))
  (return (call HTTPStatusLine version status ""))
)
```

Correct:

```brix
(type-line + HTTPStatus (is I32))

(type + HTTPStatusLine (is (tuple (: version String) (: status HTTPStatus) (: reason String)))
  (fn-expr + This _ (over: Char)
    (~ (seq (: version (+ (seq (! " ") .))) // (: status HTTPStatus) / (: reason (* (seq (! \nl) .))))
      (call This version status reason)
    )
  )
)
```

## YS-10: Keep Effects And Low-Level Work Imperative

Use `fn-do` and ordered statements for IO, mutation, buffers, cursors, cleanup, error paths, and
other effects. Do not conceal an effect in a lazy or nominally value-shaped expression. Manual
low-level code belongs only in the measured hot core; keep its boundaries idiomatic.

Incorrect:

```brix
(fn-expr + save-message _
  (param path String)
  (param contents String)
  "saved"
  (where
    (lazy ignored (call write-file path contents))
  )
)
```

Correct:

```brix
(fn-do + save-message _
  (param path String)
  (param contents String)
  (call write-file path contents)
  (return "saved")
)
```

## YS-11: Use `use` For Locally Owned Resources

When a function acquires a resource and would unconditionally release it with `finally`, make the
resource type `Closable`, or `Usable` when using it does more than close it at the end, and do not
write `begin-using` and `end-using` that only give the object and close it: `use` closes a
`Closable` that has no `begin-using`. Use `(use resource)` when the resource should remain live for
the rest of the scope. Use `(do-use resource …)` when its lifetime should end after a specific
block, before the function continues; nest blocks when one resource must close before another. Keep
`finally` for conditional cleanup, ownership transfer, or restoration of surrounding state that is
not intrinsic to the resource.

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
