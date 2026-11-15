# Control flow

A step body runs its rows in order. This chapter covers locals and the blocks that hold them, and
what changes the order of rows: conditions, `if` and `else`, values chosen by a condition, decision
tables and loops. Returns are covered in Functions, `match` in Patterns, and raising errors in
Errors.

## Locals

```brix
(main
  (let count 0)
  (let ratio Float 1)
  (let (tuple (: low I32) (: name String)))
  (print (str count " " ratio " " low " [" name "]"))
  (= count 5)
  (+= ratio 0.5)
  (let (tuple first second) (tuple "a" "b"))
  (print (str count " " ratio " " first second))
)
```

```output
0 1 0 []
5 1.5 ab
```

| Declaration | Declares |
| --- | --- |
| `(let name value)`{.brix} | A local of the value's type. |
| `(let name Type value)`{.brix} | A local of the type, which the value converts to. |
| `(let name Type)`{.brix} | A local of the type, holding its zero. |
| `(let (tuple a b) value)`{.brix} | A local for each field of a tuple; see Patterns. |
| `(let (tuple (: a I32) (: b Float)))`{.brix} | Several locals, each holding its type's zero. |
| `(let name _ value)` | A local of the value's type, for a value that is itself a type. |

- **Zero.** A local declared with no value holds its type's zero: `0`, `false`, `""`, an empty
  collection, or `null` for an optional.
- **Assigning.** `(= name value)`{.brix} gives a local a new value, converted to its type, as
  `(+= ratio 0.5)`{.brix} does above. An argument is a local of its function, and can be assigned
  too. A name nothing declares cannot be assigned: `(= total 5)`{.brix} is an error,
  `Variable "total" is not declared; declare it with ". total = 5"`.
- **Types as values.** `(let kind I32)`{.brix} declares a local of type `I32`, holding `0`. To hold
  the type itself, write `_` for the local's type: `(let kind _ I32)`{.brix}, which prints as
  `[TypeInfo I32]`.
- **Lazy locals.** `(lazy name value)`{.brix} declares a local whose value is computed where it is
  first read; see Functions.
- **Order.** A local is known from its declaration on. Reading it before is an error:
  `Name "total" not found`.

### Blocks and scopes

The body of an `if`, `else`, loop or `do` is a block. A local declared in a block is known from its
declaration to the end of the block, and not after it.

```brix
(main
  (let count 5)
  (let label "outer")
  (if (> count 1)
    (print label)
    (let label "inner")
    (print label)
    (= count 9)
  )
  (print label)
  (print count)
  (do
    (let doubled (* count 2))
    (print doubled)
  )
)
```

```output
outer
inner
outer
9
18
```

- **`do`** runs its body as a block, where it stands, so its locals end with it. In a value body,
  `do` blocks run steps before the value; see Functions.
- **Shadowing.** A local declared in a block may have the name of one outside it, even with
  another type. Until the inner declaration, the name reads the outer local, and after the block
  it does again.
- **Assigning in a block.** An assignment, `(= name value)`{.brix} rather than
  `(let name value)`{.brix}, changes the local it names, wherever that local was declared, so
  `count` above is `9` after the `if`.
- **Redeclaring.** A name declared twice in one block is an error: `"x" is already declared on line
  1 in this scope; to change its value, assign it without the ".", as in "x = value", or choose
  another name`.
- **After the block.** Reading a block's local after it is an error: `Name "doubled" not found`.

## Conditions

A condition is the value tested by an `if` and the branches after it, a `while`, a loop clause,
`(and a b)`{.brix}, `(or a b)`{.brix} or `(not a)`{.brix}. It need not be a `Bool`:

| Value | True when |
| --- | --- |
| `Bool` | It is `true`. |
| A number | It is not zero. |
| Text | It is not empty. |
| A list, map or array | It is not empty. |
| An optional | It holds a value, even `0`, `""` or `false`. |
| A value of another type | The type defines `(not value)`{.brix}, and it gives `false`. |

```brix
(main
  (let count 0)
  (let name "Ada")
  (let items (call (of List Int)))
  (let maybe (? Int) 0)
  (print (if-expr count "count" "no count"))
  (print (if-expr name "named" "no name"))
  (print (if-expr items "items" "no items"))
  (print (if-expr maybe "maybe holds a value" "maybe holds nothing"))
  (print (if-expr (and (== count 0) name) "both" "not both"))
  (print (or "" "x"))
)
```

```output
no count
named
no items
maybe holds a value
both
true
```

- **Logic.** `(and a b)`{.brix}, `(or a b)`{.brix} and `(not a)`{.brix} give a `Bool`, whatever
  their operands' types. In `(and a b)`{.brix} and `(or a b)`{.brix}, `b` is evaluated only when
  it decides the result.
- **Optionals.** An optional is tested for whether it holds a value, not for the value it holds.
  Test the value with a comparison, as in `(== maybe 0)`{.brix}. See Optionals.
- **Narrowing.** A condition that tests an optional, or tests a value with `is`, narrows it in the
  code it guards; see Optionals and Patterns.
- **Function values.** A function named without a call is not a condition.
  `(not (. box is-empty))`{.brix} is an error: `Operator "!" is not defined for function is-empty;
  it is a function, so call it first`.

### Types as conditions

A type makes its values conditions by defining `(fn-expr + (prefix ! this) _ body)`{.brix}, which
gives `true` where a value counts as false. The core library's lists, maps, arrays, text and
channels define it.

```brix
(main
  (let box (call Box))
  (if box
    (print "full")
    (else
      (print "empty")
    )
  )
  (= (. box count) 2)
  (if box
    (print "full")
  )
  (print (not box))
)

(type + Box
  (prop + count _ (= 0))
  (fn-expr + (prefix ! this) _
    (== count 0)
  )
)
```

```output
empty
full
false
```

- **Result.** The operator must give a `Bool`. With another result type declared, it is an error:
  `Operator must return Bool`.
- **Without it.** A value of a type that does not define it cannot be a condition: `Thing cannot be
  tested as a condition, since operator "!" is not defined for it`.

## `if` and `else`

```brix
(main
  (for (in score (list 95 72 40))
    (if (>= score 90)
      (print (str score ": excellent"))
      (else-if (>= score 60)
        (print (str score ": pass"))
      )
      (else
        (print (str score ": fail"))
      )
    )
  )
)
```

```output
95: excellent
72: pass
40: fail
```

| Branch | Runs its body |
| --- | --- |
| `(if condition body)`{.brix} | When the condition holds. |
| `(if a x (else-if condition body))`{.brix} | When no branch before it ran, and its condition holds. |
| `(if a x (else body))`{.brix} | When no branch before it ran. |

- **Bodies.** The body of each branch is a block of its own. The branches after the first follow
  the `if`'s body, inside its form, as in `(if a x (else-if b y) (else z))`{.brix}. Branches with
  a condition may repeat, and `else` comes last.
- **Steps, not values.** An `if` with branches is a step. Where a value is wanted, write
  `(if-expr condition a b)`{.brix} or a decision table, below.
- **Patterns.** An `if` can test whether a value fits a pattern, and name its parts:
  `(if (= (| (tuple n word)) pair) body)`{.brix}. See Patterns.

## Values chosen by a condition

```brix
(main
  (let count 3)
  (print (if-expr (== count 1) "item" "items"))
  (let size (if-expr (< count 2) "small" (if-expr (< count 5) "medium" "large")))
  (print size)
  (print (only-if "many" (if (> count 2))))
  (print (only-if "few" (if (< count 2))))
)
```

```output
items
medium
many
unknown
```

- **One of two values.** `(if-expr condition a b)`{.brix} gives `a` when the condition holds and
  `b` otherwise. Only the side chosen is evaluated, and the condition narrows it, as in
  `(if-expr value (+ value 1) 0)`{.brix}. Its type is the join of the two sides' types; see
  Conversions. It chains: `(if-expr a x (if-expr b y z))`{.brix}.
- **A value or `null`.** `(only-if value (if condition))`{.brix} gives the value when the
  condition holds, and `null` otherwise, so its type is an optional. In rows, it binds more loosely
  than every operator but `??`, so `if a then x else y if b`{.rows} is
  `if a then x else (y if b)`{.rows}.
- **A value, not a step.** An `if` after a value never skips a step:
  `(print (only-if "few" (if (< count 2))))`{.brix} prints `unknown`. Only guards skip on a
  condition: `(throw error (if condition))`{.brix} and `(return-if value (if condition))`{.brix},
  and the steps that skip `null`; see Optionals and Errors.
- **Not with `??`.** `(?? (only-if value (if condition)) other)`{.brix} is an error:
  `` `value if condition ?? other` is not a choice of two values. Write `if condition then value
  else other`. ``

## Decision tables

A value can be chosen by a table of conditions, tried in order. Each row is
`(case? condition value)`{.brix}, and a last row `(default value)`{.brix} gives the value when no
condition holds.

```brix
(main
  (print (call grade 95))
  (print (call grade 85))
  (print (call grade 40))
  (let score 72)
  (let passed
    (block
      (case? (>= score 60)
        "yes"
      )
      (default
        "no"
      )
    )
  )
  (print passed)
)

(fn-expr + grade String
  (param score Int)
  (case? (>= score 90)
    "A"
  )
  (case? (>= score 80)
    "B"
  )
  (default
    "C"
  )
)
```

```output
A
B
C
yes
```

- **Where.** A table can be a function's value body, as `grade`'s is, or the value of a
  declaration, assignment or print, as a block:
  `(let passed (block (case? condition a) (default b)))`{.brix}. In rows, `?? value`{.rows} can be
  written on one row.
- **Type.** The table's type is the join of its values' types. A table with no default row gives
  `null` where no condition holds, so its type is an optional.
- **A row with no value** gives what its condition tested: `(case? cached)`{.brix} gives the value
  `cached` holds.
- **Narrowing.** A row's condition narrows what it tests in that row's value, and a condition that
  failed narrows what it tested for the rows after it.
- **Rows that raise.** A row `(throw error (if condition))`{.brix} raises where no row before it
  held, and a last row `(throw error)`{.brix} can stand in for the default row; see Errors.
- **As steps.** In a step body, a table can stand as a step, with steps in its rows. When no row
  holds and there is no default row, the steps after the table run. A `match` run as steps is
  written `(do (inline (match …)))`, and the error for a bare one says so; see Patterns.
- A table is a `match` with no subject: its rows are arms, and the arms of Patterns can mix with
  them.

## Loops

### `for`

`(for (in item source) body)`{.brix} runs its body once for each item of the source, with `item`
naming the item.

```brix
(main
  (for (in word (list "red" "green"))
    (print word)
  )
  (for (in c "hé")
    (print c)
  )
  (let ages (call (of Map String Int)))
  (= ([] ages "Ada") 36)
  (for (in (tuple name age) ages)
    (print (str name " is " age))
  )
  (for (in (list "p" "q"))
    (print each)
  )
  (let total 0)
  (for (in n (.. 1 4))
    (+= total n)
  )
  (print total)
)
```

```output
red
green
h
é
Ada is 36
p
q
10
```

- **Sources.** A loop reads lists, arrays, text, maps, ranges, streams and generators, and values of
  types that implement `Iterable`, `CursorIterable` or `Indexable`; see Collections and Streams.
  A number is not a source: `A for loop cannot iterate over I64; to count, loop over a range, as
  in "0 .. < 42"`.
- **Text** gives its characters, not its bytes. A **map** gives each key and its value as a tuple.
  The order of a map's entries is not defined.
- **Taking items apart.** The item can be a pattern: `(for (in (tuple name age) ages) body)`{.brix}
  names the fields of each tuple, and `_` skips one. A pattern that can fail, such as
  `(for (in (| (: n Int)) values) body)`{.brix}, passes over the items it does not fit. See
  Patterns.
- **Unnamed items.** `(for (in source) body)`{.brix} names no item, and the body reads it as
  `each`, or `it`.
- **Typed items.** `(for (in (: i U8) (..< 0 3)) body)`{.brix} converts each item to the type, as a
  declaration would.
- **The item** is a new local in each pass, known only in the body, and cannot be assigned:
  `Illegal to update const variables`. A function value made in the body keeps that pass's item;
  see Functions.
- **What the loop reads.** The source is evaluated once, before the first pass. A list that grows
  during the loop is read only to the length it had then.

### Ranges

```brix
(main
  (for (in n (..< 0 3))
    (print n)
  )
  (for (in n (.. 5 3))
    (print n)
  )
  (for (in n (.. 0 10 (by: 5)))
    (print n)
  )
  (for (in f (.. 0.0 1.0 (by: 0.5)))
    (print f)
  )
  (for (in c (.. 'a' 'c'))
    (print c)
  )
)
```

```output
0
1
2
5
4
3
0
5
10
0
0.5
1
a
b
c
```

| Range | Gives |
| --- | --- |
| `(.. a b)`{.brix} | From `a` through `b`. |
| `(..< a b)`{.brix} | From `a` up to, but not including, `b`. |
| `(.. a b (by: step))`{.brix} | Every `step` from `a`. |

A range whose start is past its end counts down. A range steps toward its end by the size of its
step, so the step's sign does not matter: `(.. 10 0 (by: -5))`{.brix} gives `10`, `5` and `0`, as
`(.. 10 0 (by: 5))`{.brix} does. A range can hold integers, floats or characters.
An integer range gives `Int`s unless its bounds have another type. A range is a value, with a
`length` and a `start`; see Literals and Collections.

### Clauses

A header can end with clauses that decide, for each item, whether the body runs:

```brix
(main
  (let total 0)
  (for (in n (.. 1 10) (when (== (% n 2) 0)))
    (+= total n)
  )
  (print total)
  (for (in n (.. 1 10) (while (< n 4)))
    (print (str "while " n))
  )
  (for (in n (.. 1 10) (until (== n 3)))
    (print (str "until " n))
  )
)
```

```output
30
while 1
while 2
while 3
until 1
until 2
```

| Clause | For each item |
| --- | --- |
| `(when condition)`{.brix} | Skips the item unless the condition holds. |
| `(while condition)`{.brix} | Ends the loop at the first item where the condition fails. |
| `(until condition)`{.brix} | Ends the loop at the first item where the condition holds. |

A clause reads the item, and the names a pattern in the header gives it. Clauses follow the source,
as in `(for (in item source (when condition)) body)`{.brix}, and are tested in the order written:
the first that skips the item or ends the loop decides, and the clauses after it are not tested
for that item.

### Several headers

A loop can read several sources together, one item of each in each pass. Each header after the
first follows the one before it, and the last holds the body, as in
`(for (in item source) (and-for (in other others) body))`{.brix}. The loop ends when any of its
sources ends. An `(or-for …)` header, for a source joined as an alternative, is read but not built
yet: `Not supported by emission: this statement`.

```brix
(main
  (for (in word (list "a" "b" "c"))
    (and-for (in index (..< 0 10))
      (print (str index " " word))
    )
  )
  (for (in n (list 1 2 3 4 5) (when (% n 2)))
    (and-for (in label (list "x" "y" "z"))
      (print (str n label))
    )
  )
)
```

```output
0 a
1 b
2 c
1x
3y
5z
```

- **Clauses** on a header apply to it alone. A `when` skips items of its own source, and the other
  sources wait: the second loop above pairs the odd numbers with `x`, `y` and `z`. A `while` or
  `until` on any header ends the whole loop.
- **`each` and `it`** name the first header's item when the headers name no items.

### `while`

`(while condition body)`{.brix} runs its body for as long as the condition holds, testing it before
each pass.

```brix
(main
  (let i 0)
  (while (< i 5)
    (+= i 1)
    (if (== i 2)
      (continue)
    )
    (if (== i 4)
      (break)
    )
    (print (str "i is " i))
  )
  (let counter (call Counter))
  (while (= (| (: value Int)) (call (. counter next)))
    (print (str "got " value))
  )
)

(type + Counter
  (prop + count _ (= 0))
  (fn-do + next (? Int)
    (+= count 1)
    (return-if count (if (< count 3)))
    (return null)
  )
)
```

```output
i is 1
i is 3
got 1
got 2
```

- **Endless loops.** `(while true body)`{.brix} loops until a `break`, a return or a raise leaves
  it. A function whose end follows such a loop with no `break` needs no return after it; see
  Functions.
- **Patterns.** `(while (= (| pattern) value) body)`{.brix} evaluates the value before each pass,
  and runs the body while it fits the pattern, with the names the pattern gives. See Patterns.
- There is no `do … while`: a loop that runs at least once is `(while true body)`{.brix} with a
  `break` at its end.

### `break` and `continue`

`break` leaves the nearest loop, and `continue` ends the current pass and goes on to the next item.

- **Nearest loop.** In nested loops they act on the innermost loop that holds them. Loops have no
  labels.
- **Rows.** Each stands alone on its row, as the last row of the block that holds it; to break when
  a condition holds, put it in an `if`. Rows after it in its block are never reached.
- **Several headers.** `continue` goes on to the next item of every header.
- **In `match`.** An arm of a `match` in a loop can break or continue it; see Patterns.
- **Outside a loop**, including in a function value written in a loop, each is an error:
  `` `break` outside a loop `` and `` `continue` outside a loop ``.
- **Cleanup.** A `(finally body)`{.brix} block in a loop's body runs at the end of every pass,
  including one left by `break` or `continue`; see Errors.

## Returns

A step body returns with `(return value)`{.brix}, or with `(return-if value (if condition))`{.brix}
where a condition holds. A return in a loop or a block leaves the whole function. See Functions
for every form of return, and for which bodies must end with one.
