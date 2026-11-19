# Brix Correctness Essentials For Agents

Read this before writing Brix. It covers recurring compiler errors.

## Builtin Module Imports

Treat every module named by `builtin-module-names` in `stdlib/lang/constants.bx` as implicitly
imported by each non-builtin Brix module. Do not explicitly import these modules.

A builtin module is the exception. It must explicitly import every other builtin module it depends
on so that dependencies within the builtin set remain explicit.

## Function Bodies

A function declared with `fn-expr` is an expression function. Its body must produce a non-`Void`
value from its result expression. It cannot contain explicit returns (`return` or `return-if`),
immediate locals (`let`), loops, or yields. Lazy dependencies (`lazy`) go in its `(where …)`, after
its value, never before it. Before its value, in a `(before …)`, it runs only checks (`(throw error
(if condition))`), prints, probes and `do` forms such as `do` and `finally`; an assignment, an `if`
statement or a call goes inside a `(do …)`.

```brix
(fn-expr + label String
  (param name String)
  (str open name close)
  (where
    (lazy open "[")
    (lazy close "]")
  )
)
```

A function declared with `fn-do` is imperative. Use it for ordered statements, immediate locals,
loops, yields, or early returns. Return a value explicitly with `return` or `return-if`. When the
function returns a value, a final expression it does not return is a compile error; otherwise a
final call's result is discarded.

A print is a step, never a value: it stands on a row of its own, so `(let a (print 5))` and
`(call foo (print 1))` are errors. A match on a row of its own in a body of steps runs its arms as
steps, so they may print, and a block that gives a value, such as a `catch` arm or a pattern's
action, runs a print before its value as `(do (print text))`. A value alone is not a step either: a
row such as `count` or `(+ x 1)` in a `fn-do` body, in a loop or in a module's statements is an
error, unless the rows below it are pipe stages that work on it.

```brix
(fn-do + chosen-name (? String)
  (param should-use Bool)
  (param primary (? String))
  (return-if primary (if should-use))
  (return null)
)
```

Closures follow the same rules: a closure written `(=> (…) value)` is one expression, and one
written `(=> (…) :do …)` is imperative.

## Parameters

A parameter is optional only when it has a written default. `(param primary (? String))` alone is
required: a call must give it, though it may give `null`. Write `(param primary (? String) (=
null))` for a parameter calls may leave out. A required parameter may follow one that may be null,
but not one with a written default.

Every word except `in`, `on`, `and`, `or`, `do`, `for`, `while`, `if` and `else` may name a local or
a parameter, including `from`, `to`, `at`, `before` and `after`. A grammar's capture is never
named `with`, which starts the helper of a pipe such as `source ~> \record with Helper(scratch)`.

## Calls

A call names the function and then its arguments, as in `(call sort keys)`. A named argument is the
parameter's name after a colon and then the value, as in `(call format n (:width 4))`. Pass a
callback whose body reads `each` as a named argument, as in `(call some items (:where (> each 2)))`;
a callback passed by position is a closure that names its inputs. A nested call keeps its own named
arguments inside its parentheses: `(call some groups (:where (call some (. each kids) (:where (>
each 2)))))`.

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
`for i U8 in 0 .. < 3`, and a range a spread gives is of the type of the list's items.

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

- An optional local or parameter that a test shows to hold a value is that value wherever the
  test guards it, with no `!`: inside `if value`, in the rest of the body after `=? 0 if !value`,
  later in the same `value && …` or `!value || …`, and in `if value then … else …` and
  `… if value`. A `!` there is an error. An optional no test guards becomes its value with
  `value!` or `value ?? fallback`. [What narrows](../ref/optionals.md#what-narrows) lists every
  test. A row written `•? "key" value` is left out of what it builds when the value holds nothing.
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

```brix
(main
  (let count 3)
  (let ratio 2.5)
  (let maybe (? Int) 4)
  (let label String (str "count is " count))
  (let whole Int (as ratio Int))
  (let rounded (call Int ratio))
  (let known Int (?? maybe 0))
  (let present Bool maybe)
  (let flag (as count Bool))
  (let small U8 (as count U8))
  (print label)
  (print (+ whole rounded known))
  (print (and present flag))
  (print (+ small 1))
)
```

## Do Not Guess Syntax

When syntax, grammar, or type behavior is surprising, inspect nearby working code and the relevant
grammar or tests before inventing alternate syntax or compiler permissiveness. A successful compile
proves that syntax is accepted, not that the source expresses the intended model.

## Ask What Was Inferred

Before guessing why a function raises an error, needs a `@stack` global bound, or was compiled
again under a restriction, ask the compiler. `bin/brix <program> --explain <function>` prints the
function's header with its inferred parameter types, result and errors filled in, then a line for
each error it raises, each `@stack` global it needs or holds, and each copy made under a caller's
restriction, with one call path that shows why. Name the function as `name`, `Type.method`,
`path:name` where names repeat, or `file:line`.

## Documentation Examples

Compile-check complete fenced `brix` examples. Mark incomplete or illustrative fragments as
`text` instead.
