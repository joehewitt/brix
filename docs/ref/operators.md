# Operators

An operator combines values, such as `(+ a b)`{.brix}, or changes one, such as `(- a)`{.brix}. In
rows, most operators are written between two values, with a space on either side. A type can
define what most of them mean for its values.

This chapter covers every operator, the order in which operators bind in rows, what the built-in
ones do, and how a type defines its own. Operators on optionals are in Optionals, `as` in
Conversions, `is` in Types and Patterns, conditions in Control flow, and the stream operators in
Streams.

## Writing operators

```brix
(main
  (print (+ 1 (* 2 3)))
  (print (* (+ 1 2) 3))
  (print (- 10 4 3))
  (print (/ 7 2))
  (print (/ -7 2))
  (print (% -7 3))
  (print (%% 2 10))
  (print (/ 7.0 2))
  (print (++ "ab" "cd"))
  (print (++ (list 1 2) (list 3)))
  (print (is-in 3 (list 1 2 3)))
  (print (< 1 2 3))
  (print (and (> 5 3) (> 2 1)))
  (print (not (> 5 3)))
)
```

```output
7
9
3
3
-3
-1
1024
3.5
abcd
1, 2, 3
true
true
true
false
```

- **Spaces.** In rows, a binary operator has a space on each side. `a+1`{.rows} is an error: `An
  operator needs a space on each side; write "a + 1"`. A prefix operator is written against its
  operand, as in `-x`{.rows} and `!x`{.rows}, and `! x`{.rows} is an error.
- **One row.** An expression ends where its row ends, so a row cannot end with an operator: `An
  expression ends at the end of its line, so nothing follows this "+"; write the whole expression
  on one line`.
- **Forms.** An operator's form has the operator as its head and the operands after it, as in
  `(+ a b)`{.brix}. An operator repeated in a row has one form for all its operands, as in
  `(- 10 4 3)`{.brix}.
- **Parentheses** group an expression as one operand in rows, as in `(1 + 2) * 3`{.rows}. In
  forms, nesting does the same.

## Precedence

Precedence belongs to rows. When operators are mixed in rows, the one that binds more tightly
takes its operands first. In forms, nesting decides. From the loosest to the tightest:

| Operators | Kind |
| --- | --- |
| `(tuple a b)`{.brix} | A tuple; see Types. |
| `(=> (a) value)`{.brix}, `(if-expr condition a b)`{.brix} | A function value, and a value chosen by a condition. |
| `(?? a b)`{.brix} | Fallback; see Optionals. |
| `(only-if value (if condition))`{.brix} | A value only where a condition holds; see Control flow. |
| `(= a b)`{.brix} `(+= a b)`{.brix} `(-= a b)`{.brix} `(*= a b)`{.brix} `(/= a b)`{.brix} `(%= a b)`{.brix} `(%%= a b)`{.brix} `(++= a b)`{.brix} `(<-* a b)`{.brix}, and each with `?`, as in `(=? a b)`{.brix} | Assignment. |
| `(\|> a b)`{.brix} `(\|=> a b)`{.brix} `(*> a b)`{.brix} `(?> a b)`{.brix} `(?*> a b)`{.brix} `(*\|> a b)`{.brix} `(-> a b)`{.brix} `(*-> a b)`{.brix} `(~> a \rule)`{.brix} | Pipelines and stages; see Streams, and Stream patterns for `~>`. |
| `(or a b)`{.brix} | Either is true. |
| `(and a b)`{.brix} | Both are true. |
| `(< a b)`{.brix} `(> a b)`{.brix} `(<= a b)`{.brix} `(>= a b)`{.brix} | Order. |
| `(== a b)`{.brix} `(!= a b)`{.brix} | Equality. |
| `(as a Type)`{.brix} | Conversion; see Conversions. |
| `(is a Type)`{.brix} `(is-not a Type)`{.brix} `(is! a Type)`{.brix} `(is~ a Type)`{.brix} `(is-in a b)`{.brix} `(not-in a b)`{.brix} `(has a b)`{.brix} `(has-not a b)`{.brix} | Tests and membership. |
| `(<- a b)`{.brix} `(?<- a b)`{.brix} | Writing to a stream or channel; see Streams. |
| `(.. a b)`{.brix} `(..< a b)`{.brix} `(.. a b (by: step))`{.brix} | Ranges; see Control flow. |
| `(///name/// a b)`{.brix} | A named operator, loosest form. |
| `(//name// a b)`{.brix} | A named operator, looser form. |
| `(/name/ a b)`{.brix} | A named operator. |
| `(+ a b)`{.brix} `(- a b)`{.brix} `(++ a b)`{.brix} | Sums and joins. |
| `(* a b)`{.brix} `(/ a b)`{.brix} `(% a b)`{.brix} `(%% a b)`{.brix} | Products, quotients, remainders and powers. |

Prefix operators bind more tightly than any of these: `(- a)`{.brix}, `(not a)`{.brix},
`(-= a)`{.brix} (removing an item; see Collections), `(<- a)`{.brix} (reading from a stream) and
`(type-of a)`{.brix}. Tighter still are the suffixes that read a value: `(call a b)`{.brix},
`(. a name)`{.brix}, `([] a index)`{.brix}, `(?. a name)`{.brix}, `(?[] a index)`{.brix} and
`(!! a)`{.brix}.

- **Left to right.** Operators of one level group from the left: `10 - 4 - 3`{.rows} is `3`, and
  `2 %% 3 %% 2`{.rows} is `(2 %% 3) %% 2`{.rows}, which is `64`. `%%` binds as tightly as `*`, so
  `1 + 2 * 3 %% 2`{.rows} is `1 + ((2 * 3) %% 2)`{.rows}, which is `37`.
- **Equality binds more tightly than order**, so `a < b == c`{.rows} compares `b == c`{.rows}
  first. Write `(a < b) == c`{.rows} to compare the result of `<`.
- **Orders chain.** `(< a b c)`{.brix} means `(and (< a b) (< b c))`{.brix}, with `b` evaluated
  once, and directions may mix: `(> (< 1 3) 2)`{.brix} is true. The chain stops at the first order
  that fails. In forms, a chain in one direction is one form, and a chain that changes direction
  nests. Equality does not chain: `(== a b c)`{.brix} compares `(== a b)`{.brix} with `c`.
- **`as`** cannot be followed by a tighter operator: `x as I64 + 1`{.rows} is an error, `A cast
  cannot be followed by an operator; wrap it in parentheses, as in "(x as I64) + 1"`. It can be
  followed by `==` or a looser operator.
- **Named operators bind more loosely than arithmetic**, so `1 + 6 /least/ 3`{.rows} is
  `(1 + 6) /least/ 3`{.rows}. The forms with more slashes bind more loosely still, so one named
  operator can take the result of another without parentheses: `a //least// b /least/ c`{.rows}
  is `a //least// (b /least/ c)`{.rows}.

## Arithmetic

| Operation | Value |
| --- | --- |
| `(+ a b)`{.brix} | The sum. |
| `(- a b)`{.brix} | The difference. |
| `(- a)`{.brix} | The negation. |
| `(* a b)`{.brix} | The product. |
| `(/ a b)`{.brix} | The quotient. |
| `(% a b)`{.brix} | The remainder. |
| `(%% a b)`{.brix} | `a` raised to the power `b`. |

```brix
(main
  (let big I32 2147483647)
  (print (+ big 1))
  (let small U8 250)
  (print (+ small 10))
  (let tiny I8 127)
  (+= tiny 1)
  (print tiny)
  (print (- 0 1u32))
  (let whole I32 5)
  (let part F32 1.5)
  (print (type-of (+ whole part)))
  (print (+ 'a' 1))
  (print (%% 2 0.5))
  (print (/ 7.0 0.0))
  (print (% -7.5 2))
)
```

```output
-2147483648
4
-128
4294967295
[TypeInfo F32]
98
1.4142135623730951
inf
-1.5
```

- **Integer division** drops the fraction, rounding toward zero: `(/ 7 2)`{.brix} is `3`, and
  `(/ -7 2)`{.brix} is `-3`.
- **Remainders** take the sign of the number divided: `(% -7 3)`{.brix} is `-1`, and
  `(% 7 -3)`{.brix} is `1`. On floats, `%` is the remainder of a division rounded toward zero:
  `(% -7.5 2)`{.brix} is `-1.5`.
- **Dividing by zero** is not checked. For integers, `(/ x 0)`{.brix} is `0` and `(% x 0)`{.brix}
  is `x`. For floats, it gives an infinity, or a value that is not a number.
- **Overflow.** Integer arithmetic that goes past its type's range wraps around, so
  `(+ 127i8 1i8)`{.brix} is `-128i8`. An assigning operator computes in the type of its two
  operands, then wraps the result into the type of what it assigns.
- **Mixed types.** The operands are brought to one type before the operation:
  - A float wins: the result is `Float` when either operand is a `Float`, and `F32` otherwise, so an
    `I32` plus an `F32` is an `F32`.
  - Two integers of one signedness give the wider type.
  - A signed and an unsigned integer give the unsigned type when it is at least as wide as the
    signed one, and the signed type otherwise.
  - A `Char` counts as a `U32`, so `(+ 'a' 1)`{.brix} is `98`.
  - A literal takes the other operand's type, so `(- 0 1u32)`{.brix} is `4294967295`, and
    `(/ 7.0 2)`{.brix} is `3.5`.

  This is not the join of Conversions: arithmetic may give a type that one operand does not
  convert to on its own.
- **Powers.** `(%% a b)`{.brix} has the type of `a`. It is computed as a `Float` and converted
  back, so `(%% 2 0.5)`{.brix} is `1.4142135623730951`, but with `a` an `Int` holding 2 it is `1`.
  An integer power too large for the type gives its largest value.
- **Updates.** `+=`, `-=`, `*=`, `/=`, `%=` and `%%=` apply their operator to what a local,
  property or item holds and store the result there, so with `n` holding 30, `(/= n 4)`{.brix}
  leaves `7`. `+=` and `-=` on a list add and remove items instead; see Collections.
- **Paths.** A type can give `/` another meaning; `Path` from `/os/path` joins with it:
  `(/ (call Path "/tmp") "notes.txt")`{.brix} is `/tmp/notes.txt`.

## Comparison

```brix
(main
  (print (== 2 2.0))
  (print (< -1 4000000000u32))
  (print (== (list 1 2) (list 1 2)))
  (print (== (tuple 1 "a") (tuple 1 "a")))
  (print (< "apple" "banana"))
  (print (< 'a' 'b'))
  (let maybe (? Int) null)
  (print (== maybe null))
  (print (!= maybe 3))
)
```

```output
true
true
true
true
true
true
true
true
```

- **Numbers** are compared by value, in the type their operands are brought to, so
  `(== 2 2.0)`{.brix} is true. A signed and an unsigned integer are compared by sign first, so
  `(< -1 4000000000u32)`{.brix} is true. A float that is not a number is equal to nothing, itself
  included, and every order with it is false.
- **Text** is compared by its characters, and ordered by them. **Characters** are ordered by code
  point.
- **Lists and arrays** are equal when they have equal items in the same order.
- **Tuples** are equal when their fields are, field by field, and named tuples only when their
  fields have the same names.
- **Objects** are equal when they are the same object, unless their class defines `==`.
- **Optionals.** `(== x null)`{.brix} tests that an optional holds nothing. One that holds nothing
  is not equal to a value, and two that hold nothing are equal. An order cannot take an optional:
  `Operator "<" is not defined for I64 and I64?`.
- **`!=`** is true where `==` is false.
- **Types that do not compare** are an error: `Operator "==" is not defined for I64 and Bool`.

## Logic

`(and a b)`{.brix} is true when both operands are, and `(or a b)`{.brix} when either is.
`(not a)`{.brix} is true when `a` is false. In `(and a b)`{.brix} and `(or a b)`{.brix}, `b` is
evaluated only when it decides the result. Each gives a `Bool`, and takes as operands anything a
condition can be; see Control flow.

## Joining and membership

- **`++`** joins two texts, or two collections of one item type, into a new one. With text on its
  left, it makes its right operand text, so `(++ "x" 5)`{.brix} is `x5`. `++=` joins onto what a
  local or property holds.
- **`+`** on a list makes a new list with one more item: `(+ (list 1 2) 3)`{.brix} is `1, 2, 3`.
  `+=` adds one item to the list in place, and `++=` adds each item of a collection. See
  Collections, and Text for the operators of text.
- **`has`** tests whether a collection holds a value, as in `(has items 3)`{.brix}, and
  `(has-not items 3)`{.brix} whether it does not. `(is-in 3 items)`{.brix} and
  `(not-in 3 items)`{.brix} ask the same with the value first. A type answers all four by defining
  `has`; see below.

## Bits

The bitwise operations are named operators, and take integers:

| Operation | Value |
| --- | --- |
| `(/and/ a b)`{.brix} | The bits set in both. |
| `(/or/ a b)`{.brix} | The bits set in either. |
| `(/xor/ a b)`{.brix} | The bits set in one but not the other. |
| `(/not/ 0 b)`{.brix} | The bits of `b`, inverted. The left operand is not used. |
| `(/lshift/ a n)`{.brix} | The bits of `a`, moved `n` places toward the high end. |
| `(/rshift/ a n)`{.brix} | The bits of `a`, moved `n` places toward the low end, copying the sign bit. |
| `(/zrshift/ a n)`{.brix} | The bits of `a`, moved `n` places toward the low end, filling with zeros. |

- **Types.** Operands of different widths are brought to one type, as for arithmetic. A shift
  keeps the type of `a`, so `(/lshift/ 1u8 8)`{.brix} is `0`. A float is an error: `Expected I64
  here, but this is F32, which becomes I64 only when asked; write "as I64"`.
- **Narrow types.** A number narrower than 32 bits is extended by its sign before a shift, so on
  those types `/zrshift/` gives what `/rshift/` does.
- **Shift counts** are not checked.
- A function declared with one of these names replaces the built-in operation.

## Named operators

Any function of two parameters can be used as an operator, with its name between slashes:
`(/clamped-to/ 7 5)`{.brix} calls `(call clamped-to 7 5)`{.brix}. A named operator puts a function
between its values without a new symbol, and the slashes keep the name from being read as a
value. See [Precedence](#precedence)
for the forms with more slashes.

## Reserved operators

`<<`, `>>` and `<|` are read as operators, but no built-in type defines them:
`Operator "<<" is not defined for I64 and I64`. A type can define them as it defines any operator;
see [Operators a type defines](#operators-a-type-defines). `?<<` and `?>>`, in rows `a? << b`{.rows}
and `a? >> b`{.rows}, are reserved: `Brix form “(?<< …)” is not supported yet`.

## Operators a type defines

A type defines an operator for its values with a function whose name is the operator written with
`this`, as in `(fn-expr + (infix this + (param other Vec)) _ body)`{.brix}. Define one only where
it means what the symbol means for numbers, such as adding or combining; otherwise use a function
whose name says what it does.

```brix
(main
  (let a (call Vec 1 2))
  (let b (call Vec 3 4))
  (print (+ a b))
  (print (* a 3))
  (print (* 3 a))
  (print (- a))
  (+= a b)
  (print a)
  (print (== a (call Vec 4 6)))
  (print (!= a b))
  (let grid (call Grid))
  (= ([] grid 1) 5)
  (print ([] grid 1))
  (print (has grid 5))
  (print (not-in 7 grid))
  (print (call grid 10))
  (print (/clamped-to/ 7 5))
)

(type + Vec (is (tuple (: x Int) (: y Int)))
  (fn-expr + (infix this + (param other Vec)) _
    (call Vec (+ x (. other x)) (+ y (. other y)))
  )
  (fn-expr + (infix this * (param factor Int)) _
    (call Vec (* x factor) (* y factor))
  )
  (fn-expr + (prefix - this) _
    (call Vec (- x) (- y))
  )
)

(extend + Int
  (fn-expr + (infix this * (param vec Vec)) _
    (* vec this)
  )
)

(type + Grid
  (prop + cells _ (= (list 0 0 0)))
  (fn-expr + (index-get this (param index Int)) _
    ([] cells index)
  )
  (fn-do + (index-set this (param index Int) (param value Int)) _
    (= ([] cells index) value)
  )
  (fn-expr + (infix this has (param value Int)) _
    (is-in value cells)
  )
  (fn-expr + this _
    (param bonus Int)
    (+ (. cells length) bonus)
  )
)

(fn-expr + clamped-to Int
  (param value Int)
  (param limit Int)
  (if-expr (> value limit) limit value)
)
```

```output
(4, 6)
(3, 6)
(3, 6)
(-1, -2)
(4, 6)
true
true
5
true
true
13
5
```

| Declaration | Defines |
| --- | --- |
| `(fn-expr + (infix this + (param other Vec)) _ body)`{.brix} | A binary operator. |
| `(fn-expr + (prefix - this) _ body)`{.brix} | A prefix operator. |
| `(fn-expr + (index-get this (param index Int)) _ body)`{.brix} | Reading `([] value index)`{.brix}. |
| `(fn-do + (index-set this (param index Int) (param value Int)) _ body)`{.brix} | Assigning `([] value index)`{.brix}. |
| `(fn-do + (index-delete this (param index Int)) _ body)`{.brix} | Removing with `(-= ([] value index))`{.brix}. |
| `(fn-expr + this _ (param bonus Int) body)`{.brix} | Calling the value, as in `(call grid 10)`{.brix}. |

- **What can be defined.** The binary operators `+`, `-`, `*`, `/`, `%`, `%%`, `++`, `==`, `!=`,
  `<`, `>`, `<=`, `>=`, `has` and `(has-not a b)`{.brix}, the assigning operators such as `+=`
  and `++=`, `<-`, the prefix operators `(- a)`{.brix}, `(not a)`{.brix} (see Control flow) and
  `(<- a)`{.brix} (see Streams), indexing, and calls.
- **Bodies and results.** A `fn-expr` body is a value, and a `fn-do` body is steps that return
  with `(return value)`{.brix}. A result type, as `Int` in
  `(fn-expr + (infix this + (param n Int)) Int body)`{.brix}, is `_` where it is left to the body.
- **Left operand.** An operator is looked up on the type of its left operand, and never on the
  right one's. For a value of another type on the left, as for `(* 3 a)`{.brix} above, declare
  the operator in an extension of that type; see Types.
- **Derived operators.** A type that defines `==` gets `!=`, and one that defines `!=` gets `==`. A
  type that defines `>` and `==` gets `<`, and an order no function answers is the negation of its
  opposite, so `<=` is answered by `>`. `(has a b)`{.brix} and `(has-not a b)`{.brix} each give
  the other, and `(is-in b a)`{.brix} and `(not-in b a)`{.brix} call them with the operands
  swapped.
- **Comparisons give `Bool`.** An equality, order or membership operator with another result type
  is an error: `Operator must return Bool`.
- **Assigning operators.** `(+= a b)`{.brix} uses the type's `+=` when it defines one, and stores
  `(+ a b)`{.brix} in `a` otherwise. A `+=` declared with `fn-expr` gives a new value, which is
  stored where `a` is held.
- **Indexes.** An index parameter can be a range, as in `(param range (of Range Int))`{.brix}, to
  read, assign or remove a slice. Only one index parameter is allowed.
- **Overloads.** A type may define one operator several times, for operands of different types,
  as `Vec` could for `*` with `(param factor Int)`{.brix} and with `(param other Vec)`{.brix}.
- **Inheritance and interfaces.** A derived class may define an operator again, and its version is
  used even where its object is held as the base class. An interface may declare operators, with
  or without bodies, for its types to define.
