# Literals

A literal is a value written out in a program: a number, a text, a character, a truth, `null`, a
list, a map, a tuple or a range. This chapter shows how each is written and what type it gets.
Text, Collections and Units cover what can be done with them.

The rows view writes some literals differently from the forms. Where it does, this chapter shows
both, as in `(list 1 2 3)`{.brix} and `[1, 2, 3]`{.rows}.

## Integers

```brix
(main
  (print 1'000'000)
  (print -17)
  (print 0xff)
  (print 300u8)
  (print 128i8)
  (print (type-of 42))
  (print (type-of 0x1'0000))
  (print (type-of 9223372036854775808))
)
```

```output
1000000
-17
255
44
-128
[TypeInfo I64]
[TypeInfo U32]
[TypeInfo U64]
```

- **Decimal.** An integer is written in decimal digits. A leading zero is not allowed in rows.
- **Separators.** A `'` between digits groups them and means nothing, as in `1'000'000`. Each `'`
  must have a digit on both sides.
- **Signs.** A minus sign written against the digits is part of the literal, as in `-17`. There is
  no leading `+`.
- **Hexadecimal.** `0x` begins a hex literal, with digits in either case, as in `0xff` or `0xFF`.
  It prints in decimal.
- **No other bases.** There are no binary or octal literals. `0b101` is read as `0` with the unit
  `b101`, and is an error: `Unit "b101" not found`.
- **Type.** An integer with no suffix is an `Int`, which is an `I64`. One of 2^63 or more is a
  `U64`, and the largest is `18446744073709551615`. One more is an error:
  `Integer literal 18446744073709551616 does not fit U64`.
- **A hex literal's type** comes from how many digits it has, counting leading zeros: one or two
  make a `U8`, three or four a `U16`, five to eight a `U32`, and more a `U64`. So
  `(+ 0xff 1)`{.brix} is a `U8` sum, and wraps to `0`.

### Suffixes

A suffix written straight after the number gives it a type:

| Type | Suffixes |
| --- | --- |
| `I8`, `I16`, `I32`, `I64` | `i8`, `i16`, `i32`, and `i64`, `l` or `i` |
| `U8`, `U16`, `U32`, `U64` | `u8`, `u16`, `u32`, and `u64`, `ul` or `u` |
| `F32`, `F64` | `f32`, and `f64` or `f` |
| `Bool` | `b` |
| `I1` | `i1` |
| `Char` | `c` |

- **`f` is `F64`.** `2f` and `1.5f` are `F64`s. Only `f32` makes an `F32`.
- **Wrapping.** A suffixed literal keeps the low bits that fit its type, so `300u8` is 44 and
  `128i8` is -128. Only one past the largest `U64` is an error.
- **Whole numbers.** An integer suffix takes no fraction or exponent: `U8 holds whole numbers, so
  1.5u8 cannot be written; write a whole number`.
- **Units.** Other suffixes are units, such as `16ms` and `25%`, which the modules that declare
  them must be imported for; see Units.

## Floats

```brix
(main
  (print 2.5)
  (print .5)
  (print 2.5e-3)
  (print 1'000.25)
  (print 2.0)
  (print 1e21)
  (print (/ 1.0 3.0))
  (print (/ 1.0f32 3.0f32))
  (print (/ 1.0 0.0))
)
```

```output
2.5
0.5
0.0025
1000.25
2
1e+21
0.3333333333333333
0.33333334
inf
```

- **Writing one.** A number with a point or an exponent is a float: `2.5`, `.5`, `5.`, `1e3`,
  `2.5e-3`, `4.2E+20`. Separators work as in integers. There are no hex floats.
- **Type.** A float with no suffix is a `Float`, which is an `F64`. `f32` makes an `F32`.
- **No `nan` or `inf`.** These have no literal. They come from arithmetic, as `(/ 1.0 0.0)`{.brix}
  does, and print as `nan`, `inf` and `-inf`. A literal too large for its type is `inf`.
- **Printing.** A float prints with the fewest digits that read back as the same number, so a
  whole float prints with no point, as `2` above. From 1e15 up and below 1e-4 it prints with an
  exponent, as `1e+21` and `1e-05`.

## The type a literal takes

A literal with no suffix takes the type of the place that holds it, or of a value beside it.

```brix
(main
  (let small U8 200)
  (let ratio Float 2)
  (let half F32 0.5)
  (print (+ small 100))
  (print (+ small 300))
  (print (type-of ratio))
  (print (type-of (* half 0.5)))
  (print (list 0 4 10.5))
  (print (call twice 21))
)

(fn-expr + twice _
  (param n I32)
  (* n 2)
)
```

```output
44
500
[TypeInfo F64]
[TypeInfo F32]
0, 4, 10.5
42
```

- **Places.** A declared local or property, a parameter, a list's declared item type and a
  result type each give a literal their type, as `small`, `ratio` and `twice` do above.
- **Neighbours.** An operand beside a value of another number type takes that type:
  `(+ small 100)`{.brix} is a `U8` sum, which wraps to 44. A literal that does not fit stays an `Int`, and the sum is an
  `Int`, as `(+ small 300)`{.brix} is.
- **Floats.** An integer beside a float, or in a float's place, becomes a float, as the `0` and
  `4` in the list do. A float never becomes an integer this way: `Expected I64 here, but this is
  F64, which becomes I64 only when asked; write "(as x I64)"`.
- **Not other types.** An integer never becomes a `Bool` or a `Char` by its place.
- **A hex literal** keeps the type of its digit count, and converts only to a wider type.

A literal that does not fit its place is an error:

```brix compile-error
(main
  (let small U8 200)
  (let large U8 300)
  (print (+ small large))
)
```

```output
Exception: Integer literal 300 does not fit U8
<no file:4>
      (let large U8 300)
                    ^^^
```

## Truths and null

```brix
(main
  (let ready true)
  (let missing (? Int) null)
  (print ready)
  (print (not ready))
  (print missing)
  (print (?? missing 0))
)
```

```output
true
false
unknown
0
```

- **`true` and `false`** are the two values of `Bool`.
- **`null`** is the absence of a value, which only an optional can hold. An optional holding
  `null` prints as `unknown`. See Optionals.
- **Names, not keywords.** `true`, `false` and `null` are names the core library declares, read
  as constants.

## Text

```brix
(main
  (let name "Ada")
  (let count 3)
  (print (str "Hello, " name "! You have " count " messages."))
  (print "She said \"hi\" and paid \$5.")
  (print "café \x41\tB")
  (print "two
lines")
)
```

```output
Hello, Ada! You have 3 messages.
She said "hi" and paid $5.
café A	B
two
lines
```

- **Quotes.** A text is written in double quotes, and `""` is the empty text. Its type is
  `String`. Any character can be written as itself, including non-ASCII ones.
- **Escapes.** A backslash begins an escape: `\"`, `\\`, `\$`, `\n`, `\t`, `\r`, `\xHH` for a
  byte and `\uHHHH` for a character. Text lists them all.
- **Dollar signs.** A `$` must be escaped as `\$`. In rows, `$name` and `${…}` insert values, and
  the forms write them with `str`: `"Hi, $name"`{.rows} is `(str "Hi, " name)`{.brix}. See Text.
- **Line breaks.** A line break inside the quotes is part of the text. The rows view writes it as
  `\n`.

## Characters

```brix
(main
  (print 'A')
  (print (str "[" (char " ") "]"))
  (print (as (char "\n") Int))
  (print 66c)
  (print (type-of 'a'))
  (print (+ 'a' 1))
)
```

```output
A
[ ]
10
B
[TypeInfo Char]
98
```

- **Single quotes.** `'A'` is a character. The forms write a visible ASCII character this way,
  except `"`, `'`, `(`, `)`, `;` and `\`.
- **`char`.** `(char " ")`{.brix} makes a character from a text of exactly one character, which
  may hold an escape. It is how the forms write a space, a quote, a non-ASCII character or a
  control character. The rows view writes all of them in single quotes, as `' '`{.rows} and
  `'\n'`{.rows}.
- **By code.** The suffix `c` makes a character from its code point: `65c` is `A`.
- **Type.** A character is a `Char`, one Unicode code point, held in four bytes. It prints as the
  character, and arithmetic on it gives a `U32`, as `(+ 'a' 1)`{.brix} does. See Text.

## Lists

```brix
(main
  (let primes (list 2 3 5))
  (let grid (list (list 1 2) (list 3)))
  (let mixed (list 1 2.5))
  (let ratios (list Float) (list 1 2))
  (let empty (list Int))
  (print primes)
  (print ([] grid 0))
  (print mixed)
  (print (/ ([] ratios 0) 2))
  (print (. empty length))
)
```

```output
2, 3, 5
1, 2
1, 2.5
0.5
0
```

- **Writing one.** `(list 2 3 5)`{.brix} is a list, written `[2, 3, 5]`{.rows} in rows.
- **Item type.** The items' common type is the item type, so `(list 1 2.5)`{.brix} holds `F64`s.
  Items with no common type are an error: `This list mixes I64 and String, which have no common
  type; convert the items to one type`. A declared item type is given to each literal item, as
  for `ratios` above.
- **Empty lists.** `(list Int)`{.brix} as a local's type with no value declares an empty list, and
  `(call (of List Int))`{.brix} makes one. A list literal with no items is an error, even where the
  type is declared: `Incompatible types in list`.
- **Printing** joins the items with `, `. See Collections.

## Maps

There is no map literal. A map is made by calling `Map` with its entries:

```brix
(main
  (let sizes (call Map (:small 1) (:large 3)))
  (let names
    (call Map
      (block
        (entry 1 "one")
        (entry 2 "two")
      )
    )
  )
  (print ([] sizes "large"))
  (print ([] names 2))
  (print ([] names 5))
)
```

```output
3
two
unknown
```

- **Entries.** A named argument, as in `(:small 1)`{.brix}, is an entry whose key is its name, as
  text. `(entry key value)`{.brix} is an entry with any key, and `(entry? key value)`{.brix} leaves
  the entry out when the value is `null`. In rows, `(call Map (:small 1) (:large 3))`{.brix} is
  `Map • small 1 • large 3`{.rows}.
- **Types.** The keys' and the values' types come from the entries. The map type is written
  `(map String Int)`{.brix}, and `[String = Int]`{.rows} in rows.
- **Empty maps.** `(map String Int)`{.brix} as a local's type with no value declares an empty map,
  and `(call (of Map String Int))`{.brix} makes one.
- **Repeated keys.** The same name given twice is an error, while a later `entry` with the same
  key replaces the earlier one.
- **Reading** a key gives an optional, `null` when the key is missing. See Collections.

## Tuples

`(tuple 1 "a")`{.brix} is a tuple, written `(1, "a")`{.rows} in rows. It prints as `(1, a)`.

- **One item.** A tuple of one item is that item, so a `tuple` form with only `1` in it is the
  `Int` 1.
- **Types.** A declared tuple type gives each literal item its type, as in
  `(let point (tuple Float Float) (tuple 1 2))`{.brix}.
- **Fields** are read by a literal position, as in `([] pair 0)`{.brix}. A position that is not a
  literal is an error: `Value has no constant form`. Named tuple types name their fields; see
  Types.

## Ranges

```brix
(main
  (print (.. 1 5))
  (print (..< 0 5))
  (print (call List (.. 0 10 (by: 3))))
  (print (call List (.. 3 1)))
  (print (call List (.. 0 1 (by: 0.25))))
  (print ([] "abcdef" (... 2)))
  (print ([] "abcdef" (... _ 2)))
)
```

```output
[Range 1 thru 5 by 1]
[Range 0 to 5 by 1]
0, 3, 6, 9
3, 2, 1
0, 0.25, 0.5, 0.75, 1
cdef
ab
```

| Form | Rows | Items |
| --- | --- | --- |
| `(.. 1 5)`{.brix} | `1 .. 5`{.rows} | From 1 through 5. |
| `(..< 0 5)`{.brix} | `0 .. < 5`{.rows} | From 0 up to 5, not including 5. |
| `(.. 0 10 (by: 3))`{.brix} | `0 .. 10 by 3`{.rows} | Every third number from 0 through 10. |
| `([] text (... 2))`{.brix} | `text[2 ...]`{.rows} | From index 2 to the end. |
| `([] text (... _ 2))`{.brix} | `text[... 2]`{.rows} | From the start up to index 2. |

- **Types.** A range of `Int`s is a `Range'I64`. The ends and the step join to one type, so
  `(.. 0 1 (by: 0.25))`{.brix} is a range of `F64`s.
- **Direction.** A range whose start is past its end counts down.
- See Collections for ranges as values and Control flow for loops over them.

## Other values written in place

- **Arrays.** `([] Int 4)`{.brix} makes an array of four zeros. There is no array literal of
  items; `(call (of Array Int) (list 2 3 5))`{.brix} copies a list. See Collections.
- **Units.** `16ms`, `1.5s` and `25%` are numbers with units, once their modules are imported. See
  Units.
- **Cases.** `.name` names a case of a union or an enum where its type is known, as in
  `(== color .red)`{.brix}. See Unions.
- **`_`** is not a value. It stands for a type to infer, a pattern that fits anything, or an open
  end of a range.

## Constants

A module value whose value is a literal, such as `(prop + limit _ (= 5))`{.brix}, is a constant:
it is part of the compiled program and can be read before its module's code runs. An integer, a
float, a text, a character, `true` or `false` makes a constant, and so does a minus sign on one
of them. A list, a tuple, a call or any other expression is worked out when the module starts;
see Modules.
