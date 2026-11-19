# Functions

A function is declared at the top level of a file or inside a type. Its header names it and states
what it takes and gives. Its body is one of two kinds: a value body, whose rows describe the value
the function gives, or a step body, whose rows run in order and return with `(return value)`{.brix}.

This chapter covers declarations, the two kinds of body, lazy bindings, results, parameters,
calls, function values, generic functions, overloading and annotations. Methods, constructors and
properties are functions inside a type; what is particular to them is in Types. Conversions a type
declares are in Conversions, and operators a type defines are in Operators.

## Declaring a function

```brix
(main
  (print (call describe 3))
  (print (call area 2.0 3.5))
)

(fn-expr + describe String
  (param count Int)
  (str count " " (call noun count))
)

(fn-expr _ noun _
  (param count Int)
  (if-expr (== count 1) "item" "items")
)

(fn-expr + area Float @inline
  (param width Float)
  (param height Float)
  (* width height)
)
```

```output
3 items
7
```

A declaration is a header, a parameter for each value the function takes, and then the body. In
rows, the parameters are rows indented below the header row, and the body is flush with the left
margin after a blank row. A thick divider (`━`{.rows}) comes before a function at the top level of
a file, and a thin divider (`─`{.rows}) before a function inside a type.

A function with a value body is declared with `fn-expr`, and one with a step body with `fn-do`, as
in `(fn-do + name Type body)`{.brix}. The header then holds, in order:

- **Access.** `+` makes the function public: other modules that import this one can call it. `_`
  makes it private to its module. Private functions can be called anywhere in the module that
  declares them.
- **The name**, in lowercase words joined by hyphens.
- **The result type**, when it is written. Without one, `_` in its place, the compiler infers it
  from the body.
- **Error types**, each after `?!`, naming the errors the function can raise, as in
  `(fn-expr + ratio Float (?! Error) body)`{.brix}. See Errors.
- **Annotations**, each starting with `@`.

## Value bodies

A function declared with `fn-expr`, an expression function, has a value body. The body describes
one value, and that value is what the function gives. A value body is the usual way to write a
function, not a shorthand for small ones.

```brix
(main
  (print (call ratio 6.0 4.0))
)

(fn-expr + ratio Float (?! Error)
  (param width Float)
  (param height Float)
  (before
    (throw (call Error "height is zero") (if (== height 0)))
    (print (str "dividing " width " by " height))
  )
  (/ width height)
)
```

```output
dividing 6 by 4
1.5
```

The value can be any expression. It can also be one of these forms, which give a value without
being a single expression:

- **A decision table**: `(case? condition value)`{.brix} arms, each with a condition and the value
  it gives, and a `(default value)`{.brix} arm for every other case. A `match` is a table over one
  subject. See Control flow.
- **Feeds**: rows such as `(item value)`{.brix}, each adding items to the list the function
  gives. See Streams.
- **A pipeline**: a value followed by stage rows such as `(pipe ?> condition)`{.brix} and
  `(pipe *> value)`{.brix}, each working on the result of the row above. See Streams.
- **A text body**: the text the function gives, written as the text itself, as in
  `(fn-expr + greeting _ (paragraph "Hello."))`{.brix}. See Text.
- **Sections**: a `(header value)` row, then value rows, each passed to the `with` method of what
  came before: the first to the header's value, and each later one to what the previous `with`
  gave. The function gives the last answer. In rows a section starts with a `════` divider.

```brix
(main
  (print (call size-label 2))
  (print (call (. ", " join) (call steps true)))
  (print (call (. ", " join) (call doubled-evens (list 1 2 3 4))))
)

(fn-expr + size-label String
  (param count Int)
  (case? (< count 3)
    "few"
  )
  (case? (< count 20)
    "some"
  )
  (default
    "many"
  )
)

(fn-expr + steps (list String)
  (param verbose Bool)
  (item "start")
  (item? (only-if "trace" (if verbose)))
  (item "stop")
)

(fn-expr + doubled-evens (list Int)
  (param values (list Int))
  values
  (pipe ?> (== (% each 2) 0))
  (pipe *> (* each 2))
)
```

```output
few
start, trace, stop
4, 8
```

The value cannot be `Void`. A value body that gives nothing, or a function declared `Void` with
`fn-expr`, is an error: a function that only does work is a step body.

### Rows before the value

A value body may run steps before its value, in its `before` section, in the order they are
written:

- **Checks**: `(throw error (if condition))`{.brix} raises the error when the condition holds. See
  Errors.
- **Prints**: `(print value)`{.brix}.
- **Probes**. See Probes.
- **`do` blocks**: `(do body)`{.brix}, whose steps may assign to parameters and call functions.
- **`finally`**: `(finally body)`{.brix}, steps that run when the function ends, however it ends.

Anything else that is a step is an error before the value: a return, a local declared with
`(let name value)`{.brix}, an assignment or a call outside a `do` block, an `if` statement, a loop,
a yield, and a lazy binding. Put the step in a `do` block, move the lazy after the value, or
declare the function with `fn-do`.

These rules are checked where a function is declared, whether or not anything calls it. Function
values written with `=>` follow the same rules; see [Function values](#function-values).

## Lazy bindings

A lazy binding names a value that is worked out when it is first read. It is written
`(lazy name value)`{.brix}. In a value body, lazy bindings come after the value, in its `where`
section, so a function can give its answer first and define the parts it is built from below it,
where a reader looks for them next.

```brix
(main
  (print (call describe true))
  (print (call describe false))
)

(fn-expr + describe String
  (param detailed Bool)
  (if-expr detailed (str "total " total ", still " total) "no total")
  (where
    (lazy total (call computed-total))
  )
)

(fn-expr + computed-total Int
  (before
    (print "computing")
  )
  42
)
```

```output
computing
total 42, still 42
no total
```

`(call describe true)`{.brix} reads `total` twice, and `computed-total` runs once.
`(call describe false)`{.brix} never reads it, and `computed-total` does not run.

- **When it runs.** The value is worked out at the first read of the name, and not at all if
  nothing reads it. A lazy binding's value may read other lazy bindings, and they run in the order
  the reads reach them.
- **How often.** At most once in each lifetime of the binding. Later reads give the same value.
  When the value raises a recoverable error, the error is kept too, and each later read raises it
  again without running the value a second time.
- **Lifetime.** A binding has a new lifetime in each call of its function. A binding declared in
  the body of a loop has a new lifetime in each turn of the loop. A function value created in a
  call shares the binding with that call, and whichever reads it first runs it.
- **What it reads.** The value reads names as they are when it runs, not as they were where the
  binding is written. A step that changes a name the value reads, run before the first read,
  changes the value.
- **No updates.** Once worked out, the value stays, though the names it read change later. A
  value that follows them is a matter for Reactivity.
- **Assigning.** A lazy binding can be assigned like a local. Assigning before the first read means
  its value never runs. `+=` and other updates read the binding first, which runs its value.
- **A type.** `(lazy name Type value)`{.brix} converts the value to the type.
  `(lazy name Type)`{.brix}, with no value, is the type's default.
- **A pattern.** `(lazy (tuple first second) (call pair))`{.brix} takes a value apart into several
  names. The value runs once, at the first read of any of them. See Patterns.
- **Under one value.** `(scope value rows)` gives a value with the lazy bindings it reads written
  under it, as in `(print (scope (* width height) (lazy width 3) (lazy height 4)))`{.brix}. In rows
  the bindings are indented below the value's row. A call takes such rows in its `block`.

```brix
(main
  (call reprice)
)

(fn-do + reprice _
  (let price 3)
  (lazy total (* price 2))
  (= price 4)
  (print total)
  (= price 10)
  (print total)
)
```

```output
8
8
```

`total` reads `price` after it changes to 4, and keeps its value after it changes to 10.

A lazy binding that reads itself, directly or through the bindings it reads, is an error. So is a
second binding of the same name in one body.

In a step body, a lazy binding can stand anywhere among the statements, including after the last
return.

Because a lazy binding's value runs at its first read rather than where it is written, the order in
which effects happen follows the order of reads. A value with an effect, such as reading from a
channel or changing state, runs where it is first needed, and not at all on a path that never
needs it.

## Step bodies

A function declared with `fn-do` has a step body: rows that run in order. A step body can declare
locals with `(let name value)`{.brix}, assign, branch, loop and yield, and it gives its result with
a return.

```brix
(main
  (print (call first-negative (list 4 -2 7 -9)))
  (print (call first-negative (list 1 2)))
)

(fn-do + first-negative (? Int)
  (param values (list Int))
  (for (in value values)
    (return-if value (if (< value 0)))
  )
  (return null)
)
```

```output
-2
unknown
```

The second call finds no negative value and returns `null`, which prints as `unknown`.

### Returns

| Return | Meaning |
| --- | --- |
| `(return value)`{.brix} | Return the value. |
| `(return)`{.brix} | Return from a function that gives nothing. |
| `(return-if value (if condition))`{.brix} | Return the value when the condition holds. Otherwise go on. |
| `(return-if (if condition))`{.brix} | Return nothing when the condition holds. |
| `(return? value)`{.brix} | Return the value when it is not `null`. Otherwise go on. |

A returned value is converted to the function's result type, as an assignment would convert it.
A function that gives nothing may return a call that gives nothing, as in
`(return (call note text))`{.brix}.

```brix
(main
  (print (call grade 95))
  (print (call grade 40))
  (call log "ready")
  (call log "")
  (print (call cached-or-default null))
  (print (call cached-or-default 7))
)

(fn-do + grade String
  (param score Int)
  (return-if "excellent" (if (>= score 90)))
  (let passed (>= score 50))
  (return (if-expr passed "passed" "failed"))
)

(fn-do + log _
  (param message String)
  (return-if (if (== message "")))
  (print (str "log: " message))
)

(fn-do + cached-or-default Int
  (param cached (? Int))
  (return? cached)
  (return 0)
)
```

```output
excellent
failed
log: ready
0
7
```

### The end of the body

When the rows run out without a return, the function ends. What it gives then depends on its
result type:

- A function that gives nothing returns.
- A `Bool` function gives `false`.
- An optional function gives `null`.
- Any other result type is an error, reported where the function is declared: the function can
  fall through without returning a value of its type.

The end is out of reach, and needs no return, after a return, a raise that always happens, a loop
on the constant `true` with no `break` that leaves it, an `if` and `else` that both return or
raise, and a match run as steps that must match and returns in every arm, as in
`(match subject (case pattern (return value)))`{.brix}. A condition that is the
constant `false` stands for its `else`.

A value standing alone as the last row is an error, not a return. In a function that gives a
value, the compiler asks for `(return value)`{.brix}; in one that gives nothing, it asks to use the
value or remove it. An assignment as the last row is a step, not a value, so the function falls
through as usual.

Rows after a return, a `continue` or a raise that always happens are never reached, and the
compiler does not check the names in them.

## Result types

The result type follows the name in the header. When it is left out, `_` in its place, the
compiler infers it from the body.

- **A value body** gives the type of its value. The arms of a decision table join into one type.
- **A step body** gives the type its returns join into: returns of `Int` and `Float` join into
  `Float`, and returns of two classes join into a class both derive from. A body whose end can be
  reached after a return that may not happen gives an optional, as `half` below does. Returns with
  no common type are an error, which asks for one type or a declared result.
- **A step body with no returns of a value**, or none at all, gives nothing. `Void` can also be
  written as the result type.

```brix
(main
  (print (call half 10))
  (print (call half 7))
  (print (call countdown 3))
)

(fn-do + half _
  (param value Int)
  (return-if (/ value 2) (if (== (% value 2) 0)))
)

(fn-expr + countdown String
  (param value Int)
  (if-expr (== value 0) "liftoff" (str value " " (call countdown (- value 1))))
)
```

```output
5
unknown
3 2 1 liftoff
```

A function that calls itself, directly or through other functions, must declare its result type,
as `countdown` does. Otherwise the compiler reaches the call before it knows the type, and reports
an error that asks for the declaration.

## Parameters

Each parameter follows the header, with its name and its type: `(param name Type)`{.brix}.

```brix
(main
  (print (call greet "Ada"))
  (print (call greet "Ada" "Hi"))
  (print (call greet "Grace" (:punctuation "?")))
  (print (call total 1 2 3))
  (print (call total))
  (print (call repeat "ab" 2.7))
  (print (call plus 1))
  (print (call plus 1.5))
)

(fn-expr + greet String
  (param name String)
  (param greeting String (= "Hello"))
  (param punctuation String (= "!"))
  (str greeting ", " name punctuation)
)

(fn-do + total Int
  (param ... values (list Int) (= (list)))
  (let sum 0)
  (for (in value values)
    (+= sum value)
  )
  (return sum)
)

(fn-expr + repeat String
  (param text String)
  (param count (similar Int))
  (call (. "" join) (in text (..< 0 count)))
)

(fn-expr + plus _
  (param value _)
  (+ value 1)
)
```

```output
Hello, Ada!
Hi, Ada!
Hello, Grace?
6
0
abab
2
2.5
```

- **Defaults.** `(param greeting String (= "Hello"))`{.brix} gives the parameter a default, which a
  call can leave out. The default is worked out by each call that leaves the argument out, when
  that call is made, so it can call functions and reads values as they are then. It is converted to
  the parameter's type. A default cannot read the members of the object a method is called on.
- **Order.** A parameter without a default cannot follow one with a default.
- **Optional parameters.** A parameter can be left out of a call only when it has a default. A
  parameter of an optional type, such as `(param nickname (? String))`{.brix}, still has to be
  given, though it can be given `null`; `(param nickname (? String) (= null))`{.brix} can be left
  out. An optional argument that holds nothing, given for a parameter with a default, takes the
  default.
- **Rest parameters.** A parameter marked `...` takes every remaining positional argument as a
  list, as `values` does in `(fn-expr + sum Int (param ... values (list Int)) body)`{.brix}. It
  must be the last parameter. A call must give it at least one argument unless it has a default, as
  `total` does.
- **Converting parameters.** `(param count (similar Int))`{.brix} takes any value that `as`
  converts to `Int`, and converts it, so `(call repeat "ab" 2.7)`{.brix} repeats twice.
  `(param count (exact Int))`{.brix} takes exactly an `Int`, which matters when it is
  [overloaded](#overloading).
- **Untyped parameters.** A parameter with no type, `(param value _)`{.brix}, takes a type from
  each call: the compiler makes a separate function for each type the parameter is called with, as
  for `plus`.
- **Function types.** A parameter's type can be a [function type](#function-types) written in
  place, with the function's own parameters inside it, as in
  `(fn-expr + name _ (param func (fn-type Int (param value Int))) body)`{.brix}.
- **Type parameters.** A row can also name a type parameter of a generic function, to give it a
  default or a constraint. See [Generic functions](#generic-functions).
- **Names.** A parameter can be named by any word except `in`, `on`, `and`, `or`, `do`, `for`,
  `while`, `if` and `else`. Prepositions such as `from`, `to`, `at`, `before` and `after` make a
  call by name read as a phrase, `(call distance (:from origin) (:to corner))`{.brix}, and are
  called prepositional arguments. They help most where two parameters share a type and swapping
  them would still compile.

A parameter is a local of the function. Assigning to it changes the function's local, not what the
caller passed.

## Calls

A call names a function and gives its arguments, as in `(call name a b)`{.brix}.

```brix
(import /stream/aggregate)

(main
  (let scores (list 12 40 7 33))
  (print (call label "Ada"))
  (print (call label "Ada" (:greeting "Hi")))
  (print
    (call label
      (block
        (:name "Grace")
        (:greeting "Hey")
      )
    )
  )
  (print
    (call clamp
      (block
        (arg 150)
        (arg 0)
        (arg 100)
      )
    )
  )
  (print (call some scores (:where (> each 30))))
  (print (|> 5 (call clamp 0 3)))
)

(fn-expr + label String
  (param name String)
  (param greeting String (= "Hello"))
  (str greeting ", " name)
)

(fn-expr + clamp Int
  (param value Int)
  (param low Int)
  (param high Int)
  (if-expr (< value low) low (if-expr (> value high) high value))
)
```

```output
Hello, Ada
Hi, Ada
Hey, Grace
100
true
3
```

- **Arguments by name.** `(:greeting "Hi")`{.brix} gives the parameter named `greeting`. Arguments
  by name follow the positional ones, in the call or one to a row in its `block`, and can give any
  parameter, with or without a default.
- **One argument to a row.** A call can give its arguments as rows in a `block`, each
  `(arg value)`{.brix}, as the call of `clamp` does.
- **A block as an argument.** An argument by name can take a `block` as its value, as in
  `(:name (block step value))`{.brix}: steps, then the value as the last row.
- **Feeds as the argument.** A call whose `block` holds feeds gives the list they build as its
  argument, as in `(call (. ", " join) (block (item "a") (item "b")))`{.brix}.
- **Callbacks that read `each`.** An argument by name for a parameter of a function type can be an
  expression that reads `each`, as in `(call some scores (:where (> each 30)))`{.brix}. The
  expression becomes the body of a function whose parameter is `each`. A callback given by position
  is written as a [function value](#function-values) that names its parameters.
- **The rest of the row.** `(call name (inline value))`{.brix} gives the value as the last
  argument, written in rows as the rest of the row after a colon.
- **Pipes.** `(|> value (call f more))`{.brix} calls `(call f more value)`{.brix}: the piped
  value comes after the written arguments. See Streams.
- **Named operators.** `(/name/ a b)`{.brix} calls `(call name a b)`{.brix}. See Operators.

Arguments are worked out in the order they are written, arguments by name included. A piped value
is worked out before the written arguments. Each argument is converted to its parameter's type as an
assignment converts it, and a number written with no suffix takes the parameter's type.

A method is called through its object, as in `(call (. text trim))`{.brix}. The object is worked
out before the arguments. `(call (?. object method) argument)`{.brix} gives `null`, without working
out the arguments, when the object is `null`. Methods are covered in Types.

A call that no function of its name can take is an error that names the argument types and the
functions it could have called.

## Function values

A function is a value. A name declared as a function can be passed, stored and called like any
other value, and a function value can be written in place with `=>`.

```brix
(main
  (let add (=> (a b) (+ a b)))
  (let greeting (=> () "hello"))
  (let log
    (=> ((: text String)) :do
      (block
        (print (str "log: " text))
      )
    )
  )
  (blank)
  (print (call add 2 3))
  (print (call greeting))
  (call log "ready")
  (print (call apply 4 double))
  (print (call apply 4 (=> (n) (str "n is " n))))
)

(fn-expr + double Int
  (param value Int)
  (* value 2)
)

(fn-expr + (of apply Result) _
  (param value Int)
  (param func (fn-type Result (param value Int)))
  (call func value)
)
```

```output
5
hello
log: ready
8
n is 4
```

- **Parameters.** `(=> (a b) value)`{.brix} names the parameters `a` and `b`. A function value
  with none is `(=> () value)`{.brix}. A parameter may have a type, as in
  `(=> ((: text String)) value)`{.brix}, and the last may take the rest of the arguments, as in
  `(=> ((: ...keys (list Int))) value)`{.brix}.
- **Untyped parameters.** Where a function type is expected, as for the `func` parameter of `apply`,
  the parameters take their types from it. Elsewhere an untyped parameter is generic: each type it
  is called with makes a separate function.
- **Bodies.** `(=> () value)`{.brix} has a value body and follows the rules of
  [value bodies](#value-bodies): checks, prints and `do` blocks may run before the value, and lazy
  bindings may follow it. With `:do` after the parameter list, as in `(=> () :do body)`{.brix}, it
  has a step body and returns with `(return value)`{.brix}.
- **Named functions.** The name of a declared function, without a call, is a function value, as
  `double` is above.
- **Methods.** A method read from an object without a call, as in `(. counter bump)`{.brix}, is a
  function value bound to that object. Calling it calls the method on that object. Inside a method,
  the bare name of another method of the same type is bound to the same object.

A function value prints as its type, such as `[Function Mapper'I64'I64]`.

### Capture

A function value can read and assign the locals of the body that creates it. It does not copy
them: the body and every function value created in it share each local, and an assignment by one
is seen by all.

```brix
(main
  (let total 0)
  (let add
    (=> ((: amount Int)) :do
      (block
        (+= total amount)
      )
    )
  )
  (blank)
  (call add 3)
  (call add 4)
  (print total)
  (blank)
  (let readers (call (of List (of Getter Int))))
  (for (in item (list 1 2 3))
    (+= readers (=> () (* item 10)))
  )
  (for (in reader readers)
    (print (call reader))
  )
)
```

```output
7
10
20
30
```

- **Each call has its own locals.** A function value created in one call of a function shares the
  locals of that call only, and keeps them after the call returns.
- **Each turn of a loop has its own locals.** A loop's item, and each local declared in the loop's
  body, is new in every turn, so a function value created in a turn reads that turn's values. Locals
  declared before the loop are shared by every turn.
- **Reads happen at the call.** A function value reads a local when it runs, so it sees the local's
  value at that moment, not the value it had when the function value was created.
- **Lazy bindings** are shared the same way, and run at the first read by the body or by any
  function value created in it.
- **Locals declared later.** A function value may name a local declared after it in the same body.

A task started with `(do-new body)`{.brix} is different: it copies the locals of the body that
starts it. See Concurrency.

### Function types

A function type is written with `fn-type`: the result type, then the parameters, as in
`(type + Step (is (fn-type Int (param amount Int))))`{.brix}. A function type whose result is
`Void` gives nothing, and error types follow the result, as in
`(param parse (fn-type Int (?! ParseError)))`{.brix}. A function type can be declared with a name,
or written directly where a type is wanted, as the type of a parameter or a property.

```brix
(main
  (let steps (call (of List Step)))
  (+= steps double)
  (+= steps (=> (amount) (+ amount 1)))
  (for (in step steps)
    (print (call step 5))
  )
  (blank)
  (let scale Scale (=> (value factor) (* value factor)))
  (print (call scale 3 4))
  (print (call scale 5 (:factor 2)))
)

(type + Step (is (fn-type Int (param amount Int))))

(type + Scale (is (fn-type Int (param value Int) (param factor Int))))

(fn-expr + double Int
  (param value Int)
  (* value 2)
)
```

```output
10
6
12
10
```

The core library declares common function types, which every module can use:

| Type | Function |
| --- | --- |
| `Doer` | Takes nothing, does something and may raise an `Error`. |
| `(of Getter Result)`{.brix} | Takes nothing and gives a result. |
| `(of Mapper Item Result)`{.brix} | Takes an item and gives a result. |
| `(of Predicate Item)`{.brix} | Takes an item and gives a `Bool`. |
| `(of Comparer Item)`{.brix} | Takes two items and compares them. |

A value of a function type is called like a function. Its parameters' names can be used for
arguments by name, as `factor` is above, and the arguments are worked out in the order written.
An optional function value must be unwrapped before it is called, as in
`(call (!! hook) value)`{.brix} or inside `(if hook (call hook value))`{.brix}.

**Conversion.** A function or function value converts to a function type on its own when a call
through the type would convert each argument to what the function takes, and the function's result
to what the type gives, on their own. So a function that takes a `Float` converts to a type whose
parameter is an `Int`; a function that takes a base class converts to a type whose parameter is a
class derived from it; any function converts to a type that gives `Void`; and a function that
raises fewer errors converts to a type that allows more. Any other conversion between function
types is written with `as`, and then each argument and the result convert as `as` converts them.
A function that raises an error the type does not allow is an error at the conversion; a function
with a different number of parameters cannot be converted at all.

**Equality.** Two function values are equal when they run the same code with the same captured
locals. A declared function is equal to itself; two function values created by one call that
capture nothing are equal; two created by separate calls that capture locals are not.

### Partial application

A call that ends with `...` in place of its remaining arguments, as in `(call add 10 ...)`{.brix},
does not call the function. It gives a function value that takes the missing arguments and then
makes the call. Arguments by name can follow the `...`, and a call of a generic function can leave
its type arguments to the later call too.

```brix
(main
  (let add-ten (call add 10 ...))
  (print (call add-ten 5))
  (print (call add-ten 32))
)

(fn-expr + add Int
  (param a Int)
  (param b Int)
  (+ a b)
)
```

```output
15
42
```

## Generic functions

A function can have type parameters, written after its name: `(of both Item)`{.brix} has one type
parameter, `Item`, which its parameters and result can use.

```brix
(main
  (print (call both 4))
  (print (call both "ab"))
  (print (call (of both Float) 2))
  (print (call kind 3))
  (print (call kind 2.5))
  (print (call kind "three"))
)

(fn-expr + (of both Item) _
  (param value Item)
  (tuple value value)
)

(fn-expr + (of kind Item) _
  (param value Item)
  (constraint is Item Number)
  "a number"
)

(fn-expr + (of kind Item) _
  (param value Item)
  "something else"
)
```

```output
(4, 4)
(ab, ab)
(2, 2)
a number
a number
something else
```

- **Inference.** A call usually leaves the type arguments out, and the compiler infers them from
  the arguments, including from the result of a function value passed as an argument.
- **Explicit type arguments.** A call can give them after the name, as in
  `(call (of both Float) 2)`{.brix}. It can give the first ones and leave the rest to inference.
- **Each type makes a function.** The compiler makes a separate function for each set of type
  arguments a program uses. A parameter with no type works the same way: it is a type parameter of
  its own.
- **Calling itself.** A generic function can call itself, directly or through other functions, but
  not with type arguments that grow at each call, and neither can a method of a generic type on an
  object of that type. A function `deep` whose parameter `x` has a type parameter for its type, and
  that calls `(call deep (list x) (- n 1))`{.brix}, would need a separate function for each depth
  of list, without end, and is an error,
  `Function "deep" calls itself with type arguments that grow without end; call it with its own type parameters`.
- **Types as values.** Inside the function, a type parameter is a value of its own, the type it
  stands for, and can be printed or passed.
- **Defaults.** A type parameter can have a default type, which a call uses when it gives no type
  argument and none can be inferred. It is written as a parameter row, as `String` is for `Item` in
  `(fn-expr + (of both Item) _ (constraint = Item String) body)`{.brix}.

### Constraints

A constraint is a parameter row that states a rule a type parameter must meet:

| Declaration | Rule |
| --- | --- |
| `(fn-expr + (of name Item) _ (constraint is Item Type) value)`{.brix} | `Item` is `Type`, derives from it, or implements it. |
| `(fn-expr + (of name Item) _ (constraint is! Item Type) value)`{.brix} | `Item` is exactly `Type`. |

When several functions of one name could take a call, a function whose constraints the call meets
is chosen over one without constraints, as `kind` shows. A static method called on a type parameter
must be declared by one of its constraints.

## Overloading

Several functions can share a name when their parameters differ in type or number. A call chooses
among them by the types of its arguments, when the program is compiled.

```brix
(main
  (print (call describe 3))
  (print (call describe 2.5))
  (print (call describe "three"))
)

(fn-expr + describe String
  (param value Int)
  (str "the integer " value)
)

(fn-expr + describe String
  (param value Float)
  (str "the float " value)
)

(fn-expr + describe String
  (param value String)
  (str "the text " value)
)
```

```output
the integer 3
the float 2.5
the text three
```

When more than one function could take a call, the one that needs the least conversion is chosen:

- An argument that matches a parameter's type exactly is preferred to one that must be converted.
- A parameter of a derived class is preferred to a parameter of its base class.
- An integer parameter is preferred to a pointer parameter for an integer argument.
- A function whose constraints the call meets is preferred to one without constraints.
- A generic function and one that is not are compared after the generic one's type arguments are
  inferred.
- A function value as the argument chooses a parameter of a function type over a parameter of a
  bare type parameter.
- A call that gives type arguments considers only functions that have type parameters.

The choice uses the types the arguments are declared with. An argument whose declared type is an
interface or a base class does not choose a function for a class that implements or derives from
it, even when the object is one. Choosing by the object's class at run time happens only for the
object a method is called on; see Types.

Two functions of one name with the same parameter types are an error. A call that no function of
its name can take is an error that lists the functions it could have called. Functions of one name
from different imported modules join into one set, whatever the order of the imports, and private
functions of other modules are not part of it.

## Annotations

An annotation is a word starting with `@`, written after a function's result and error types.
Fields and grammar rules take annotations of their own, described with them.

| Annotation | Meaning |
| --- | --- |
| `@inline` | Put the function's body in place of each call to it. |
| `@live` | Keep state across the runs of the live value that calls the function. See Reactivity. |
| `@rollback` | On a grammar rule or action: undo the changes it made through the match's helper when the match goes back past it. See Grammars. |

A function that calls itself, directly or through other functions, cannot be inlined. The
compiler calls it as usual and, with `--warnings`, reports each call it could not inline.

An annotation the compiler does not know is an error, and so are two that contradict each other.

### Without and with

After its annotations, a function's header can take a `(without …)` part and `(with …)` parts:

```brix
(fn-do + step _ @inline (without heap suspend) (with depth 2) (with scope "step")
  (call tick)
)
```

- **Without.** Each name keeps the function from something, which the compiler checks in the
  function and in everything it calls:

  | Name | The function may not |
  | --- | --- |
  | `heap` | Allocate memory the collector manages. |
  | `suspend` | Suspend its task. |
  | `safepoints` | Hold a point where the collector may stop it. |
  | `probe-safepoints` | Hold a point where probes may stop it. |
  | `gc` | Do any of `heap`, `safepoints` and `suspend`. |

  Any other name bans an effect a library declares: see Effects and doors. On a C function in a
  header module, `(without gc)` says the function never reaches the collector, and
  `(without blocking)` that it is called between two of the collector's checks; see Native code
  and memory. A name that is neither a restriction nor an effect the file sees, or one named
  twice, is an error.
- **With.** Each `(with name value)` sets a `@stack` property for the whole body, as a `do-with`
  around it would, the first outermost, so a later value can read an earlier one. A name that is
  an effect binds a fake to it: see Effects and doors. It needs a `fn-do`.
- **Blocks.** A `(do …)` takes the same parts after its annotations, as in
  `(do (without gc) (with depth 3) …)`. A `(do …)` kept from any of these but `gc` runs in
  place: the functions it calls are kept from the same, and its loops poll only as far as that
  allows. A block can also name `cancel`, which a function cannot: a task cancelled while it runs
  the block goes on to the block's end and ends at its first wait after it; see Concurrency.

### Effects and doors

An effect is a kind of contact with the world outside the program, such as the network. A library
declares one at the top of a file, with the C functions it claims, and marks the functions through
which a program makes that contact, its doors, with a `(door …)` part after their annotations:

```brix
(effect + network
  (claim C:socket)
  (claim C:connect)
)

(fn-do + connect-to I32 (door network)
  (param host String)
  (param port Port)
  (return (call C:socket C:AF_INET C:SOCK_STREAM 0))
)
```

- **Claims.** Only a door of the effect may call a C function the effect claims, and so may a
  function written inside a door, such as a closure or a task's body. A call anywhere else is an
  error that names the effect. A C function two effects claim is an error where it is called.
- **Bans.** `(without network)` on a function, or on a `(do …)`, keeps its code, and everything that
  code calls, from reaching a door of `network`. Reaching one is an error that names the path from
  the ban to the call of the door, as in
  `parse-document (without network) → load-schema → connect-to(host, port)`. A ban follows calls
  of interface members to each function that may answer them, and it holds in a task the code
  starts. A call through a function value, or a comparison of interface values, is an error
  under a ban, as it is under a restriction.
- **What a ban promises.** A ban covers only what libraries claim. A C function that no effect
  claims passes, and so does what native code does by itself, so a ban is a guard against
  reaching the outside world by mistake, not a sandbox.

`(with network net)` on a `do` or a `fn-do`, or a `do-with` form, binds a fake to an effect for
the code under it and everything that code calls. Each door of `network` that code reaches calls
the method of the same name on `net` in its place, with the door's arguments, and gives what that
gives:

```brix
(type + Switchboard
  (fn-do + connect-to I32
    (param host String)
    (param port Port)
    (return 3)
  )
)

(fn-do + fetch-offline _ (with network (call Switchboard))
  (call load-schema)
)
```

- **Fakes.** `net` is an object of a class, which must have a method for each door the code under
  it reaches, taking and giving what the door does. A missing or different method is an error that
  names the path from the binding to the door, as in
  `fetch-offline (with network) → load-schema → connect-to(host, port)`. Only a door written as
  a function of its file can be answered so.
- **What a fake's methods reach.** A fake's method runs as the code around its binding does: a
  door of the same effect it calls is the real door, or the fake of a binding outside, so a fake
  can record what reaches the outside world and pass it on. Everything else is as where the door
  was called. A ban outside a binding lets the fake's doors through and still bans the real doors
  its methods reach.
- **Tasks and function values.** A task started under a binding keeps it, and so does a closure
  made under it. A named function made a value, under a binding or outside every one, is compiled
  outside them all: in a program that binds the effect anywhere, its doors read at each call the
  fake the running task has bound, and call that fake's method, or panic where it has none. So a
  function given as a callback under a binding reaches the fake while the binding is open and the
  real doors once it ends. A closure made outside a binding reads what was
  bound where it was made, as it keeps a `@stack` property's value. In a program that never binds
  an effect, a call of one of its doors costs what a call of the door's body does. A program can
  bind fakes of at most 255 classes to one effect.
- **Names.** An effect's name is seen as the file's other names are: a public effect from the files
  that import its file, and a private one only in its own. A program can ban or bind at most 32
  effects.

The standard library declares `clock` in `/time/clock`, beside the function of the same name. Every read of the clock, the wall clock, the
monotonic clock and the local time zone alike, goes through its doors, and so do the waits that
end at a time: `wait`, `freeze`, the timers, and a wait for a file or a socket with a deadline. A
wait for a handle with no deadline, as every blocking read and write makes, is not a door, so
`(without clock)` lets code read files. `FakeClock`, in `/time/fake-clock`, is a fake for `clock`
whose clock starts at the start of 2026 in UTC and moves only when `advance` moves it:

```brix
(import /time)
(import /time/fake-clock)

(main
  (let fake (call FakeClock))
  (do-with clock fake
    (let reminder (do-new
      (call (. 5minutes wait))
      (print "ring")
    ))
    (call (. fake advance) 5minutes)
    (call (. reminder wait))
    (print (. (call clock) s))
  )
)
```

```output
ring
1767225900
```

- **Advancing.** `advance` first lets every task that is ready to run go on until it waits, so a
  task just started reaches its wait. Then it ends the waits whose time it passes one at a time,
  earliest first, with the clocks set to each wait's time, and lets the task and the tasks it makes
  ready run until they wait again before it takes the next. It leaves the clocks at the time it
  was given. `freeze` advances instead of stopping the thread.
- **Handles.** A wait for a handle with a deadline ends when the handle is ready, or as a timeout
  when `advance` reaches the deadline.

The standard library declares `random` in `/math`. Every draw from the system goes through its
doors: `random-seed`, which also seeds `Random()`, the draws of `random` and `random(min, max)`,
and `random-bytes` in `/crypto`. A `Random` made from a seed draws without them. `FakeRandom`, in
`/math/fake-random`, is a fake for `random` that draws what `Random` does from its seed.

`environment`, in `/os/env`, has the reads and writes of environment variables as its doors, `env`,
`set-env` and `environment`, and `executable-path` in `/os/path`. Probes read their own settings
outside it. `FakeEnvironment`, in `/os/fake-environment`, starts with no variables unless it is
given some, and keeps what is set under it.

`network`, in `/net/network`, has as its doors `connect-to`, which `Socket(...)` calls,
`connect-to-host`, which `TCP(host, port)` calls, `listen-on` and `accept-from`, which
`TCPListener` calls, `get-addr-info`, and `resolve-service` and `start-service-browse` in
`/net/dns-sd`. Reads and writes on a connection are not doors. `FakeNetwork`, in
`/net/fake-network`, is a switchboard: a listener made under it and a client that connects to its
host and port meet through a socket pair, so a server and its client run in one program with no
port, and it answers a secure connection with a plain one. It answers no resolving or finding
door, so code that reaches one under it is refused.
