# Yumi Correctness Essentials For Agents

Read this before writing Yumi. It covers recurring compiler errors.

## Builtin Module Imports

Treat every module named by `builtin-module-names` in `stdlib/lang/constants.yumi` as implicitly
imported by each non-builtin Yumi module. Do not explicitly import these modules.

A builtin module is the exception. It must explicitly import every other builtin module it depends
on so that dependencies within the builtin set remain explicit.

## Declaration Dividers

Dividers control declaration nesting:

- Put a thick divider (`━`) before a module-level function or type with a body.
- Put a thin divider (`─`) before a function inside a type body.
- Do not use a thin divider for a module-level function. It will be parsed as a nested function.
- Do not use a thick divider inside a type body. It ends the type.

Use the repository's usual divider form when writing source:

```yumi
━━━━━━━━━━━━━━━━━━━━
+ Label is Tuple

+ text String

────────────────────
+ to String

text

━━━━━━━━━━━━━━━━━━━━
+ label-text String
    - label Label

label.text
```

Bodyless declarations such as type aliases do not need dividers.

## Function Bodies

A function without `do` is an expression function. Its body must produce a non-`Void` value from
its result expression. It cannot contain explicit returns (`=` or `=?`), immediate locals (`.`),
loops, or yields. Lazy dependencies (`..`) are allowed below its value, never above it. Before its
value it runs only checks (`?! error if condition`), prints, probes and `do` blocks such as `do`
and `do finally`; an assignment, an `if` statement or a call goes inside a `do` block.

```yumi
━━━━━━━━━━━━━━━━━━━━
+ label String
    - name String

"$open$name$close"
.. open = "["
.. close = "]"
```

A function with `do` is imperative. Use it for ordered statements, immediate locals, loops, yields,
or early returns. Return a value explicitly with `=` or `=?`. When the function returns a value, a
final expression it does not return is a compile error; otherwise a final call's result is
discarded.

A print is a step, never a value: it stands on a line of its own or after `:` as a one-step body,
so `. a = -- 5` and `foo(-- 1)` are errors. A match whose arms print is written `do match`, and a
block that gives a value, such as a `catch` arm or a pattern's action, runs a print before its value
as `do -- text`. A value alone is not a step either: a row such as
`count` or `x + 1` in a `do` body, in a loop or in a module's statements is an error, unless the
lines below it are pipe stages that work on it.

```yumi
━━━━━━━━━━━━━━━━━━━━
+ do chosen-name String?
    - should-use Bool
    - primary String?

=? primary if should-use
= null
```

Closures follow the same rules: a closure written `=>` is one expression, and one written `=> do`
is imperative.

## Parameters

A parameter is optional only when it has a written default. `- primary String?` alone is required:
a call must give it, though it may give `null`. Write `- primary String? = null` for a parameter
calls may leave out. A required parameter may follow one that may be null, but not one with a
written default.

Every word except `in`, `on`, `and`, `or`, `do`, `for`, `while`, `if` and `else` may name a local or
a parameter, including `from`, `to`, `at`, `before` and `after`. A stream pattern's capture is never
named `with`, which starts the helper of a pipe such as `source ~> \record with Helper(scratch)`.

## Calls

A call takes its arguments in parentheses, as in `sort(keys)`. A named argument is a bullet with the
parameter's name and the value, as in `format(n) • width 4`, never `• width = 4`. Pass a callback
whose body reads `each` as a named bullet, as in `some(items) • where each > 2`; a callback passed
by position is a closure that names its inputs. The bullets on a line all belong to one call, so a
nested call takes its bullets inside parentheses: `some(groups) • where (some(each.kids) • where
each > 2)`. A bullet at the end of a line takes the indented block below as its value, except on an
`if`, `else`, `while`, `match` or `for` line, whose block is the body.

## Numbers

`Int` is `I64`, `UInt` is `U64` and `Float` is `F64`. `Size` and `Index` are `I64`, so a count or
a position is signed and `index - 1` can be below zero. Use `I32`, `U32` or a smaller type only
where the size matters: compact storage such as ids, a file format, or a C function.

An integer written with no suffix is an `Int`, except beside or in place of a value of another
integer type that holds it, where it is of that type: `offset + 1`, `ids += 0`, `take(0)`,
`List'U8(1, 2)`, `= 0` in a function that also returns a `U64`, and `. id SyntaxId = 0` need no
suffix and no `as`, nor do the arms of a match a typed declaration gives its value. Beside or in
place of a float it is of that float's type, so `[0, 4, 10.5]`, `width ?? 1`, `Point(0, 2.5)`,
`0 .. < 1 by 0.25` and a function that returns `1` in one place and `2.5` in another need no `as`.
Tuples of such numbers join place by place, so `[(2.5, 1), (1, 2.5)]` is a list of `(Float, Float)`.
A constant a C header defines is such an integer, so `. offset I16 = C:SEEK_END` needs no `as`. A
suffix fixes the type, as in `5i32`, `5u8` and `5u` for a `U64`. A float written with no suffix is
a `Float`, except in place of or beside an `F32`, so `. half F32 = 0.5` and `half * 0.5` stay in
`F32`. A range of such integers is of the type a loop's pattern states, as in
`for i U8 in 0 .. < 3`, and a range a bullet spreads is of the type of the list's items.

Arithmetic and bitwise operators on two values of one integer type stay in that type and wrap,
so `200u8 + 100u8` is `44u8`. Two integer types are brought to the wider, as in
`200u8 /or/ 0x100`, which is a `U16`, and so are the two ends of a range. An integer and a float
are brought to the float by arithmetic and by a comparison, so `count * 0.5` and `t < count` need
no `as`.

## Conversions

A value takes the type a place asks for on its own only where its meaning is kept:

- a number becomes a number type that holds every value of its own, so `U32` becomes `Size` and
  `Float`, but a `Size` becomes a `U32` or a `Float` only with `as`;
- a class becomes its base class or an interface it implements;
- a function, a closure or a bound method becomes a function type where a call through the type
  would convert each argument to what the function takes, and what it answers to what the type
  returns, on their own, so a function from `Float` to `I32` becomes `() Int - n I32`;
- a value becomes an optional, or a union with a case it becomes on its own, so a `U64` becomes
  `JSON` only with `as`;
- an optional becomes a union with a `Null` case and a case its value becomes on its own, so a
  `String?` becomes `JSON`, and an absent one becomes the `Null` case;
- a type converts by a converter it declares, when each item it converts would convert on its own
  too, so `[I32]` becomes `[F64]`;
- a pointer becomes a pointer to the same type or to `Void`, a pointer to `Void` becomes any
  pointer, and a pointer to `Char8`, `I8` or `U8` becomes a pointer to any of them; an array
  becomes a pointer to its items or to `Void`, a text a pointer to its bytes, `null` any pointer,
  and any other object only a pointer to `Void`;
- an optional where a `Bool` is wanted tells whether it holds a value, unless it is a `Bool?`;
- an optional given for a parameter with a default takes the default when it holds nothing.

Write every other conversion:

- An optional becomes its value with `value!` or `value ?? fallback`, or inside `if value`. A
  row written `•? "key" value` is left out of what it builds when the value holds nothing.
- A base class, an interface or a union becomes a narrower type in a pattern, as in `| dog Dog`,
  or with `as Dog`. A declaration with a type, as in `. dog Dog = animal`, does not narrow.
- A float becomes an integer with `as Int`, and an integer becomes a type that may not hold it with
  `as`, as in `nodes.length as SyntaxId` and `index as Float`. Numbers, `Bool` and `Char` become one another only with
  `as`, and so does a number that becomes an enum or a pointer, as in `address as Pointer`.
- A pointer, an array or an object becomes a pointer to another type with `as`, as in
  `bytes.data as CPointer'I32` and `box as Pointer`.
- Any other function becomes a function type with `as`, as in `half as Whole`. A callback's
  answer converts as a return does, so write `• where each % 2 == 1`, not `• where each % 2`.
- A C function takes any argument that `as` converts to its parameter, as C does, except a
  number, a `Bool` or a `Char` for a pointer. Pass `null` for a null pointer, not `0`.
- `as` binds more loosely than arithmetic, `/and/` and `<-`: `a + b as U32` casts the sum,
  `mask /or/ (bits as U64)` casts one operand, and `channel <- (count as Size)` casts what is sent.
- Calling a type converts as `as` does: `Int(2.5)` is `2`, and `Char(65)` is `A`.
- A parameter written with `~`, as in `- count Int~`, takes any value that `as` converts, and so
  do `is~` and a pattern such as `| flag Bool~`.
- A value becomes text only inside a text, as in `"$count"`, in a print, or with `as String`. An
  argument, an assignment or a return never makes text of it. Pass a `Path` to APIs that take
  one, and write `"$path"` or `path as String` where text is wanted.
- `null` is a value only of an optional type or a pointer.

These have no answer: `if ready then 1 else "none"`, `[1, "x"]`, `1 == true`, `"ab" == 'a'` and
`[1, 2] ++ "x"`.

```yumi
. count = 3
. ratio = 2.5
. maybe Int? = 4
. label String = "count is $count"
. whole Int = ratio as Int
. rounded = Int(ratio)
. known Int = maybe ?? 0
. present Bool = maybe
. flag = count as Bool
. small U8 = count as U8
-- label
-- whole + rounded + known
-- present && flag
-- small + 1
```

## Do Not Guess Syntax

When syntax, grammar, or type behavior is surprising, inspect nearby working code and the relevant
grammar or tests before inventing alternate syntax or compiler permissiveness. A successful compile
proves that syntax is accepted, not that the source expresses the intended model.

## Documentation Examples

Compile-check complete fenced `yumi` examples. Mark incomplete or illustrative fragments as
`text` instead.
