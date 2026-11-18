# Brix Lint Rules For Agents

Use this guide when asked to lint Brix or perform a Brix style review. `brix lint` reads its
configuration from the project's `.brix/lint.json`. It lints `.yumi` and `.bx` files alike: in a
`.bx` file `syntax/parse-error` reports every form the Brix catalog rejects, and the layout rules
measure the rows of its Yumi view.

## Core Lint Pass

Run `brix lint <targets> --semantic-rules --fix` with each rule of this set as `--rule <id>`:

```text
events/no-effectful-payload
imports/sort
layout/depth
layout/divider-length
layout/line-length
names/case-owner-conflict
names/import-conflicts
names/local-shadows-import
names/type-param-shadows-type
style/no-nested-match-assertions
style/no-redundant-context-cast
style/no-redundant-default-initializer
style/no-redundant-literal-suffix
style/no-redundant-null-fallback
style/no-unused-catch-optional
style/prefer-constructor-initializer
style/prefer-decision-table
style/prefer-expression-function
style/prefer-map-literal
style/prefer-optional-match
style/prefer-quantifier
style/prefer-shorthand
style/prefer-text-template
tui/prefer-shortcut-node
```

`--fix` applies every safe fix the rules make, repeats until none is left, and writes the files it
changes; the run then reports only the findings it could not fix. Never make a safe fix by hand:
run `--fix` again instead. `--fix-dry-run` reports the fixes without writing them. Leave
intentional invalid-style fixtures out of a fix run. Fix the findings that remain as the rule
guidance below says.

Use `brix lint --list-rules` to confirm the inventory if a rule is unavailable or the configuration
has changed. If semantic coverage cannot compile or understand a target, report that limitation; do
not claim that the semantic rules passed.

## Finding Statuses

- `actionable`: the linter has enough evidence for the usual fix. Apply it unless the source is an
  intentional invalid-style fixture or the linter is wrong.
- `review`: the linter found a candidate but cannot prove that rewriting it preserves behavior.
  Evaluate the rule-specific guidance before changing it.

Do not distort correct code to silence a false positive or add suppression comments. Report a
repeatable false positive with the smallest code that shows it.

## Rule Guidance

### `events/no-effectful-payload`

Event payloads are lazy and run only when observed. Move assignments, channel writes, and imperative
calls before the event emission, then pass the captured value as the payload. Do not ignore this
finding in application or library code. Ignore it only in a focused test whose purpose is to
exercise effectful payload diagnostics or lazy event behavior.

Finding:

```text
submitted <- log-and-build()
```

Fix:

```text
. payload = log-and-build()
submitted <- payload
```

### `imports/sort`

Sort each contiguous import group alphabetically. If the diagnostic says overlapping names make the
fix unsafe, resolve the name conflict first and then sort. Ignore only in a fixture intentionally
testing import order; ordinary import-order dependence is a compiler or language bug, not a style
exception.

### `layout/depth`

Brix only. A row nested more than five levels deep is reported once where the nesting starts. Move
the inner rows into a function of their own. The rows of a `¶` text block are text: they do not
count, and they do not end the nesting around them.

### `layout/divider-length`

Brix only. Normalize dividers to 20 identical glyphs. Mixed-glyph dividers require a manual
choice of the correct nesting level. Ignore only in parser or linter fixtures intentionally
containing malformed dividers.

### `layout/line-length`

Break long calls into block-call form or name a complex input with a lazy. The text body of a `¶`
template and the rows of a `¶` text block are exempt: lay them out as the text they produce, and
never wrap them with line continuations to fit the limit. Write a long quoted text as a text block
instead. Ignore an unbreakable literal or protocol token when splitting it would change the
value, and intentional golden input where physical line length is what the test covers.

The core pass's `--fix` rewrites a long row only where the rewrite keeps the program's meaning and
every row it makes fits:

- a call that is the whole value of its row gets its arguments one to a row;
- a quoted text, or the last operand of `++` when it is one, becomes a text block;
- `if … then … else …` chains that compare one subject with `==` become a `match` of that subject
  in a `let`, a lazy or a return, and other chains become a decision table in a `let` or a lazy;
- `=? value if condition` becomes `if condition` with `= value` below it;
- the value of a `let` or a lazy declared without a type moves to the row below.

It leaves a row alone when the rewrite would change typing, such as a decision table or a value
below an assignment or a declared type, and in a `.bx` file when the rows it would make start a new
run nested past the `layout/depth` limit. Fix what remains by hand: name parts of a long condition,
arithmetic or list with lazies, or move code into a function.

Finding:

```text
render-card(name, email, status, theme, primary-action, secondary-action, accessibility-label)
```

Fix:

```text
render-card
    - name
    - email
    - status
    - theme
    - primary-action
    - secondary-action
    - accessibility-label
```

### `names/case-owner-conflict`

A bare case such as `.enter` was passed to a call whose overloads accept more than one union that
has that case, so the compiler took the last visible owner. Write the owner the finding names, which
keeps the current behavior and no longer depends on declaration or import order. When one union
repeats another's cases only so bare cases read well, remove the copies instead. Ignore only in a
fixture that exercises owner order.

Finding, where `describe` has overloads for `Key` and `Part`, both with an `enter` case:

```text
describe(.enter)
```

Fix:

```text
describe(Key.enter)
```

The rule sees overloaded calls only. A bare case converted to an expected type through `from`
declarations is not reported.

### `names/import-conflicts`

For `actionable` findings, remove an unused import, rename a declaration, or otherwise eliminate the
overlapping visible name. For `review` findings, inspect every affected call site and keep the imports
only when overload domains are disjoint and candidate selection is demonstrably order-independent.
Do not ignore a conflict merely because the current calls happen to compile.

### `names/local-shadows-import`

Prefer renaming the local declaration or removing the import. A `review` finding may remain only when
the local and imported overloads are intentionally disjoint and every affected call resolves
unambiguously without depending on declaration order.

### `names/type-param-shadows-type`

Rename the type parameter. The overlap makes type expressions harder to read and has no useful
ordinary exception. Ignore only in a diagnostic fixture built to contain the shadowing.

### `style/no-nested-match-assertions`

Lift assertion-only statements from a nested fallback block of a match or bare table into direct
raising arms, written as its last cases. `--fix` makes this rewrite.

Finding:

```text
match value
    | text String
        text
    ??
        ?! Error("bad") if value == "bad"
        ?! Panic("unexpected")
```

Fix:

```text
match value
    | text String
        text
    ?! Error("bad") if value == "bad"
    ?! Panic("unexpected")
```

### `style/no-redundant-context-cast`

Remove the cast when the surrounding assignment, argument, field, or return context already selects
the same type and the value converts to it unasked. A cast that narrows, such as a `Size` to a
`U32`, is never reported. A cast in a function or class that takes type parameters is a `review`
finding, since the rule sees only the instantiations its roots reach: keep it when another type
argument needs it, or when it can affect overload dispatch. So is a cast whose instantiations
provide different target types.

### `style/no-redundant-literal-suffix`

A number written with no suffix takes the type it is used as (YS-07 in `agent-brix-style.md`), so
a suffix that names that same type says nothing. The rule reports a suffix such as `u32`, `u64` or
`f64` where removing it leaves the number the same type: the value of a declaration, assignment or
return of that type, or of each side of a choice that gives one; an argument the call takes as that
type and still chooses the same function without the suffix; and an operand of arithmetic, a
comparison or a bitwise operation beside a value of that type. `--fix` deletes the suffix.

A suffix that selects a type is never reported: the value of a local declared without a type, the
value a shift moves, a number beside another written number, one whose unsuffixed form would not
fit or would take another type, and an argument whose suffix chooses the function. A hexadecimal
number takes its width from its digits and has no suffix to remove. The rule does not look inside
a function or class that takes type parameters.

Finding:

```text
mask = flags /and/ 255u32
total += 1u32
```

Fix:

```text
mask = flags /and/ 255
total += 1
```

### `style/no-redundant-default-initializer`

Omit an explicit initializer when the declared type already supplies the same default. This includes
zero for ordinary numeric types, `false` for `Bool`, the empty string, and a zero-argument class
constructor when it is the type's default constructor. Keep the initializer when it changes the
value, construction, storage, or initialization timing. The rule requires an explicit type and
compiler evidence; it does not infer a type from an initializer that would be removed.
Generic declarations and defaults without equivalent construction evidence are left alone.

Finding:

```text
+ foo [Int] = List'Int()
. count UInt64 = 0
```

Fix:

```text
+ foo [Int]
. count UInt64
```

### `style/no-redundant-null-fallback`

A value written with `if` is already null when its condition fails, so a choice whose other side is
`null` says the same thing at more length. `--fix` rewrites the choice as a conditional value.

Finding:

```text
. label = if name.length then name else null
```

Fix:

```text
. label = name if name.length
```

### `style/no-unused-catch-optional`

Replace `catch?` with `catch` when its optional result is discarded. If the optional result matters,
consume it explicitly instead. Ignore only in a focused parser or linter fixture.

### `style/prefer-constructor-initializer`

A constructor that assigns a property with `=` first builds the property's default and then
discards it. When that default constructs an object, the waste is real, and if the object owns an
Arena or another native resource that nothing releases, it leaks. Initialize the property with
`..` in the constructor instead: a `..` initializer replaces the default, so the default is never
built.

An `actionable` finding is an assignment in the constructor's leading run of such assignments, to a
property without an explicit initializer, whose value reads only constructor parameters and
properties the body does not assign; `--fix` applies it. A `review` finding's value depends on
earlier statements, follows other statements, calls a method of the object, reads a name that is
also a getter or lazy property, replaces an explicit initializer, or targets a property with a
setter or an observing `when` or `whenever` rule. A `..` initializer runs before the constructor
body and resolves names as properties first, so move the value there only when it can be computed
from the parameters alone; otherwise restructure the constructor or declare a cheap or optional
default.

Finding:

```text
+ frames TransformFrames

────────────────────
+ This
    - document

frames = TransformFrames(document)
```

Fix:

```text
.. frames = TransformFrames(document)
```

### `style/prefer-decision-table`

Two or more `=?` lines that each return a value, followed directly by `=`, usually spell one
decision as a series of returns. Decide with YS-02 in `agent-brix-style.md`: when the lines choose
the function's result, write one `match` on their shared subject or one bare `|?`/`??` table. Keep
them when they are early exits ahead of real work. The rule skips functions that
`style/prefer-expression-function` reports, since that rule's fix already makes the chain one
match.

Finding:

```text
record(count)
=? "empty" if count == 0
=? "low" if count < 3
= "ready"
```

Fix:

```text
record(count)
= match count
    |= 0
        "empty"
    |? it < 3
        "low"
    ??
        "ready"
```

### `style/prefer-expression-function`

Remove `do` and explicit returns when the function selects a value. Put assertions first, the result
expression next, and dependencies below as `..` lazies. Convert unless one of these holds:

- The function's value is the result of an effect, such as a load, save, send, or write, or of
  buffer or cursor work. The linter cannot see effects, so check for one in every `review` finding.
- No arrangement of the expression form puts each lazy's first read where its value should be taken.
  When a finding says a first read moves past assertion work, it names the locals to check. The
  linter cannot see an initializer that fills a cache or index that earlier guards read, so check
  what each initializer calls.
- A lazy would be read inside the body of a hot loop or comprehension.
- The expression form would nest a table inside an arm of another table instead of becoming one
  ordered table.

YS-01 in `agent-brix-style.md` gives the cost model behind these exceptions. Treat the diagnostic's
`review` status as a prompt to check them, not as permission to ignore the finding without
inspection.

Finding:

```text
━━━━━━━━━━━━━━━━━━━━
+ do answer

= 42
```

Fix:

```text
━━━━━━━━━━━━━━━━━━━━
+ answer

42
```

### `style/prefer-map-literal`

Replace key/value tuple arguments or straight-line assignments into a fresh map with map literal
entries. This is the map-specific form of YS-04 in `agent-brix-style.md`. Keep an empty constructor
when the map is genuinely empty or when selecting capacity, storage, or backing behavior. Keep
assignments that mutate an already constructed map.

Finding:

```text
. item = Map'String'JSON()
    - ("stableKey", mapping.stable-key as JSON)
    - ("disabled", mapping.disabled as JSON)
    - ("key", key)
```

Fix:

```text
. item = Map'String'JSON
    • "stableKey" = mapping.stable-key as JSON
    • "disabled" = mapping.disabled as JSON
    • "key" = key
```

### `style/prefer-optional-match`

For an `actionable` finding, replace `match` with `match?` and remove the terminal `?? null` arm. For
a `review` finding, first prove that the explicit fallback does not carry distinct `Result`, error,
or null-only semantics. Keep the original form if that proof fails.

Finding:

```text
match value
    | text String
        text
    ??
        null
```

Fix:

```text
match? value
    | text String
        text
```

### `style/prefer-quantifier`

Replace a loop that only searches, a `for` whose single statement is `=? true if …` followed by
`= false`, with `some(items) • where …`. For `=? false if …` followed by `= true`, use
`all(items) • where …` and negate the condition. Read the item as `each`; both quantifiers come from
`/stream/aggregate`. Write `!(some(items) • where !p)` as `all(items) • where p`, and
`!(all(items) • where !p)` as `some(items) • where p`. A loop that compares two lists element by
element is `left == right`.

The finding is `actionable` when the module already imports `/stream/aggregate` and the loop binds
one plain name. Otherwise it is `review`: keep the loop where the module cannot import
`/stream/aggregate`, as in `stdlib/core`, in a measured hot path, or where the condition has an
effect that a predicate would hide (YS-10).

Finding:

```text
for item in items
    =? true if item.is-open
= false
```

Fix:

```text
= some of items where each.is-open
```

### `style/prefer-shorthand`

Replace the property assignment with the reported visible shorthand only after confirming argument
compatibility and identical style precedence. Keep the assignment when it is tracked, transformed,
ambiguous, belongs to an ordinary receiver property, appears inside the shorthand definition, or
would change which style wins.

Finding:

```text
<Box
    background-color = black
```

Fix after verifying the visible shorthand and precedence:

```text
<Box • bg:black
```

### `style/prefer-text-template`

The rule reports a function that builds its text by appending to a string, by appending lines to a
`[String]` and joining them, or by returning an interpolated string of 80 or more characters, or
one holding `\n`, from a body of only locals. Under YS-04 in `agent-brix-style.md`, write it as a
`¶` template: the text is the body, and its values sit below the `════════════════════` boundary as
`..` lazies. Write optional text as a lazy that falls back to `""`, and repeated lines as
`"\n".join(item-line(each) in items)` with a smaller template for each line. Keep the
accumulator for character-level work such as escaping, encoding, and parsing, and keep `do` where
an effect or eager snapshot must run in order (YS-10). The rule takes precedence over
`style/prefer-expression-function` for the functions it reports.

Finding:

```text
_ do inherited-style-css String

. size = $font-size
. family = $font-family
= ":root { --brix-font-size: ${size}px; --brix-font-family: $family; font: ${size}px $family }"
```

Fix:

```text
_ ¶ inherited-style-css

:root {
    --brix-font-size: ${size}px;
    --brix-font-family: $family;
    font: ${size}px $family;
}

════════════════════
.. size = $font-size
.. family = $font-family
```

### `tui/prefer-shortcut-node`

Replace raw `key-press` or `key-input` command dispatch with a declarative `<Shortcut>` when a
compatible shortcut exists. Preserve focus and modal eligibility, cancellation, propagation,
repeat and sequence behavior, and handler order. Keep the raw handler for text editing, low-level
controls, raw terminal events, key repeats or sequences, cancellation-only behavior, or event data
that `<Shortcut>` cannot represent.

Finding:

```text
for event on key-press
    if event.key == Key.escape
        close()
        event.cancel()
```

Fix when the handler is an ordinary command:

```text
<Shortcut "Close"
    key = .escape
    action = => do close()
```
