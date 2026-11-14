# Syntax and expressions

Yumi's syntax gives the shape of a program a visible vocabulary. Dividers identify boundaries,
capitalization helps distinguish types from values, and symbols mark operations such as return,
yield, matching, and stream transformation. Expression functions and lazy bindings let code
begin with the result it is trying to describe.

The details become easier to read together than in isolation. This chapter introduces the
forms that recur throughout the guide; [the syntax essay](syntax-choices.md) explains their
design tradeoffs.

## At a glance

| Capability | What it gives you |
| --- | --- |
| Declaration boundaries | Thick dividers separate top-level declarations; thin dividers separate members. |
| Names and spacing | `display-name` is a value name; `DisplayName` is a type; `a - b` is subtraction. |
| Expression functions | State the answer, then explain its lazy dependencies with `..`. |
| Procedures | `do` gives ordered steps, mutation, loops, and explicit returns. |
| Calls and text | Named arguments, function values, interpolation, and `¶` templates. |

## Declarations have visible boundaries

:::example-pair

- [New declaration](#source-declaration--boundary): The thick divider establishes a top-level boundary.
- [Public name](#source-declaration--public): `+` exposes the declaration.
- [Parameter](#source-declaration--parameter): The indented line names an input and its type.
- [Flush-left body](#source-declaration--body): The expression supplies the result. Thin dividers separate members inside a type.

```yumi #declaration
-- greeting("Ada")

⟦boundary⟧━━━━━━━━━━━━━━━━━━━━⟦/boundary⟧
⟦public⟧+ greeting⟦/public⟧
    ⟦parameter⟧- name String⟦/parameter⟧

⟦body⟧"Hello, $name"⟦/body⟧
```

:::

Indentation groups nested code, but function bodies begin flush to the left. This gives a
function a distinctive silhouette: its name and parameter block above, its work below.
Dividers are part of the syntax rather than optional visual decoration.

Types such as `String` begin with a capital. Ordinary names such as `display-name` use lowercase
kebab case. Whitespace separates operators from their operands, keeping subtraction distinct
from the hyphens in names: `starting-count - removed-count`.

## An expression can be the whole function

```yumi #expression-body
-- doubled(21)

━━━━━━━━━━━━━━━━━━━━
+ doubled
    - value Int

⟦result⟧value * 2⟦/result⟧
```

- [Result expression](#source-expression-body--result): The body is the answer; the call produces `42`. A conditional, match, pipeline, or text expression can occupy the same position.

For example, a decision can be written as a table of expressions:

```yumi #decision
-- stock-label(2)

━━━━━━━━━━━━━━━━━━━━
+ stock-label
    - count Int

match count
    ⟦literal⟧|= 0⟦/literal⟧
        "Empty"
    ⟦predicate⟧|? it < 3⟦/predicate⟧
        "Low"
    ⟦fallback⟧??
        "Available"⟦/fallback⟧
```

- [Value arm](#source-decision--literal): Zero selects `"Empty"`.
- [Predicate arm](#source-decision--predicate): The next test selects `"Low"` for this call.
- [Fallback](#source-decision--fallback): Every remaining value produces `"Available"`; the selected arm supplies the function result.

## Lazy bindings put dependencies below the result

`..` introduces a lazy binding. A function can use a name before the binding appears in its
source, so its main expression can stay at the top:

:::example-pair

- [Document first](#source-lazy-dependency--answer): The output uses `total` before its binding appears in the source.
- [Lazy calculation](#source-lazy-dependency--lazy): The initializer runs when its value is needed. An unused dependency need not run.

```yumi #lazy-dependency
-- receipt-line("Apples", 3, 2)

━━━━━━━━━━━━━━━━━━━━
+ receipt-line
    - label String
    - quantity Int
    - price Int

⟦answer⟧"$label: $quantity for $total"⟦/answer⟧

⟦lazy⟧.. total = quantity * price⟦/lazy⟧
```

:::

Evaluation timing matters if an initializer performs IO or changes state. A lazy binding
is not a decorative synonym for an ordinary assignment. Likewise, it does not by itself
mean that a result automatically stays synchronized with changing inputs. Tracked dependencies
and live expressions are covered in [Concurrency and reactivity](concurrency-and-reactivity.md).

## Procedures make state changes explicit

Yumi also supports ordinary local variables, loops, mutation, and early exits. A `do` function
provides an imperative body:

```yumi #procedure
-- total([1, 2, 3])

━━━━━━━━━━━━━━━━━━━━
⟦procedure⟧+ do total⟦/procedure⟧
    - values [Int]

⟦local⟧. result = 0⟦/local⟧
for value in values
    result += value
⟦return⟧= result⟦/return⟧
```

- [Procedure](#source-procedure--procedure): `do` introduces a body of ordered steps.
- [Local state](#source-procedure--local): `.` creates a variable that the loop updates.
- [Explicit return](#source-procedure--return): `=` supplies the answer. Procedures and expression functions can call each other.

Yumi uses related symbols for other control-flow operations. `<=` yields one item from a
generator, `<<=` yields the items of another source, and `?!` signals an error or panic.
Their fuller contracts belong to [Streams](streams.md) and [Errors](errors.md).

## Calls can name relationships

An ordinary call uses parentheses, as in `greeting("Ada")`. A longer call can place one
argument on each line. A bullet passes an argument by its parameter's name, making its role
part of the call's wording.

This fragment declares the parameters and calls the function by name:

```yumi #named-arguments
-- distance • from origin • to destination

━━━━━━━━━━━━━━━━━━━━
+ distance
    ⟦from⟧- from Point⟦/from⟧
    ⟦to⟧- to Point⟦/to⟧
```

- [Source relationship](#source-named-arguments--from): `from` names the argument at the call site and in the body.
- [Destination relationship](#source-named-arguments--to): The call reads `distance • from origin • to destination`.

Named infix operators offer another way to express a relationship between operands. Together
with operator overloading, they let domain operations participate in expressions. Their value
depends on restraint: an unfamiliar name should make the relationship clearer, and its behavior
should remain discoverable.

## Functions can be values

An operation can accept a function, retain it, or return one. Short expression functions can
be written inline; this fragment supplies a two-argument addition function:

```yumi #function-value
⟦inputs⟧:: total, item⟦/inputs⟧ ⟦answer⟧=> total + item⟦/answer⟧
```

- [Inputs](#source-function-value--inputs): The inline function accepts two values.
- [Answer](#source-function-value--answer): The expression after `=>` computes their sum.

Stream stages have a convenient per-item name, `each`, so a transformation can read
`values *> each * 2`. Calling a function in such a stage still requires a call expression.
`*> double(each)` invokes `double`; `*> double` produces the function value.

## Text can have the shape of its output

Strings support interpolation, as the earlier examples show. Larger text can use a template
body, introduced with `¶`:

```yumi #text-body
━━━━━━━━━━━━━━━━━━━━
⟦template⟧+ ¶ page String⟦/template⟧
    - title String
    - body String

<html>
    <head><title>⟦expression⟧$title⟦/expression⟧</title></head>
    <body>$body</body>
</html>
```

- [Template function](#source-text-body--template): `¶` makes the body text that produces a string.
- [Interpolation](#source-text-body--expression): Values are inserted into the text using ordinary Yumi expressions.

This is text construction. [Markup](markup-and-stylesheets.md) is a separate capability that
constructs object trees. Both let the source resemble the structure of the result, but they
produce different kinds of value.

See [String templates](string-templates.md) for interpolation, supporting calculations,
conditional content, repetition, composition, and whitespace.

That is the recurring theme of the syntax: expressions for answers, patterns for shapes,
pipelines for sequences, and markup for trees. The language offers procedural control too;
the useful choice is the form that makes the current operation easiest to see.
