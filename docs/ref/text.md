# Text

Text is a `String`: a sequence of Unicode characters, stored as UTF-8. One character is a
`Char`. This chapter covers writing text, building it from values, text bodies, the members and
operators of `String` and `Char`, converting to and from text, and atoms, which are names that
compare by identity.

Text patterns are in Patterns, and grammars over text in Stream patterns.

## Strings

```brix
(main
  (let word "Aé漢😀")
  (print (. word length))
  (print (. word size))
  (print (. word display-width))
  (print ([] word 1))
  (print ([] word (..< 1 3)))
  (for (in letter "héy")
    (print (as letter Int))
  )
  (print (call List "abc"))
  (print (not ""))
)
```

```output
4
10
6
é
é漢
104
233
121
a, b, c
true
```

- **Three sizes.** `length` counts characters, which are Unicode code points. `size` counts the
  bytes of the UTF-8 encoding. `display-width` counts terminal columns: a wide character such as
  `漢` takes two, and a combining mark takes none. All three are `Size`, which is `I64`.
- **Characters are code points.** An accented letter written as a letter and a combining mark is
  two characters, so `"e\u0301"` has a length of 2. Yumi does not count grapheme clusters.
- **Looping** over a text gives each `Char` in turn. A text converts to a list of its characters,
  and to a stream of them; see Streams.
- **Empty text is false.** A text used as a condition is true when it is not empty, so
  `(not "")`{.brix} is `true`. See Control flow.
- **`(call String)`{.brix}** makes the empty text, `""`.
- **Values.** A text never changes. Assigning to an item or a range of it stores a new text in the
  variable; see [Indexing and slicing](#indexing-and-slicing).

## Writing text

A text is written in double quotes. Inside the quotes, a backslash begins an escape:

| Escape | Character |
| --- | --- |
| `\"` | A double quote. |
| `\\` | A backslash. |
| `\$` | A dollar sign. |
| `\n` | A line feed. |
| `\t` | A tab. |
| `\r` | A carriage return. |
| `\xHH` | The character with the code given by two hex digits, such as `\x00`. |
| `\uHHHH` | The character with the code given by four hex digits, such as `\u00e9`. |

```brix
(main
  (print "She said \"hi\".")
  (print "C:\\temp")
  (print "costs \$5")
  (print "a\tb")
  (print "caf\u00e9")
  (print (. "a\x00b" length))
)
```

```output
She said "hi".
C:\temp
costs $5
a	b
café
3
```

- **A dollar sign must be escaped.** A bare `$` is an error: `Unescaped “$” in Brix text. Write
  “\$” for a dollar sign, or build the text from its parts with (str …).`
- **Characters beyond `\uFFFF`**, such as emoji, are written as themselves.
- **A NUL character** is an ordinary character: it counts in `length` and `size`, and what
  follows it is kept.

### In rows

The rows view writes the same text with the same escapes, and adds `\0`, `\b`, `\f` and `\v` for
the control characters the forms write as `\x00`, `\x08`, `\x0c` and `\x0b`. A backslash at the end
of a line continues the text on the next line, adding nothing.

## Building text from values

A `str` form makes one text from its parts, as in `(str "Hello, " name "!")`{.brix}. A part that is not text is converted with
its type's `to String`, the same conversion `print` and `(as value String)`{.brix} use.

```brix
(main
  (let name "Ada")
  (let count 3)
  (let nobody (? String) null)
  (print (str "Hello, " name "!"))
  (print (str count " items, " (* count 2) " halves"))
  (print (str "pi is about " 3.14))
  (print (str "items: " (list 1 2 3)))
  (print (str "pair: " (tuple 1 "a")))
  (print (str "nobody: " nobody))
  (print (str "\${" (?? nobody "?") "}"))
)
```

```output
Hello, Ada!
3 items, 6 halves
pi is about 3.14
items: 1, 2, 3
pair: (1, a)
nobody: unknown
${?}
```

- **Order.** The parts are evaluated and converted from left to right, each converted before the
  next is evaluated.
- **Null** becomes `unknown`.
- **Conversion.** A type's own `to String` decides how its values appear; see Conversions. A `++`
  the type declares is never used for a part.
- **Nesting.** A part may be any expression, including another `str` form.

### In rows

In rows, a text with parts is one quoted text. `$` followed by a name inserts that value, as in
`"Hello, $name!"`{.rows}, and `${…}` inserts any expression, as in
`"${count * 2} halves"`{.rows}.

- `$` starts a part only before a lowercase letter, a Greek letter, `°` or `{`. Elsewhere it is
  an ordinary character, so `"$5"`{.rows} is the text `$5`.
- `$name` reads a whole name, hyphens included, and then a chain of `.member` and `?.member`, but
  no call. `"$n-1"`{.rows} reads the name `n-1`: `Name "n-1" not found; in a string, "$n-1" reads
  the whole name "n-1". Write "${n}-1" to put "-1" after the value`. Punctuation after the name,
  as in `"$name."`{.rows} or `"$name's"`{.rows}, stays text.

## Text bodies

A function whose body is a `paragraph` form returns text written as lines. Each line is a text
or a `str` form, and a `blank` form is an empty line. The lines are joined with line feeds.

```brix
(main
  (print (call receipt "Apples" 3 2))
  (print (call Item "pear" 2))
)

(fn-expr + receipt _
  (param name String)
  (param quantity Int)
  (param price Int)
  (paragraph
    "Receipt"
    "======="
    (str "Item: " name)
    (str "Quantity: " quantity)
    (blank)
    (str "Total: " total " (" (if-expr (> quantity 1) "several" "one") ")")
    "    with \"quotes\" and \$ signs"
  )
  (where
    (lazy total (* quantity price))
  )
)

(type + Item (is (tuple (: name String) (: count Int)))
  (fn-to-expr + String
    (paragraph
      (str name " × " count)
    )
  )
)
```

```output
Receipt
=======
Item: Apples
Quantity: 3

Total: 6 (several)
    with "quotes" and $ signs
pear × 2
```

- **Result.** The function's result type is written `_`, and it returns a `String`.
- **Lazy bindings** go in a `where` form after the paragraph, and the lines read them. See
  Functions.
- **White space.** Each line keeps its leading spaces. White space at the very end of the body is
  trimmed.
- **Converters.** A type's `to String` may have a paragraph body, as `Item` does here.
- **Only bodies.** A paragraph is a function body, not a value: ``(paragraph …) cannot be the
  `value` of (let …), which takes Expr or Block.``

### In rows

In rows, a text body is a function marked `¶`, as in `+ ¶ receipt`{.rows}. After its parameters
and a blank row, every row up to the next divider is text, written without quotes: quotes need no
escapes, and `$name` and `${…}` insert values. A `════` divider ends the text and begins the lazy
bindings.

## Characters

A `Char` is one Unicode code point, held in four bytes.

```brix
(main
  (let letter 'a')
  (print letter)
  (print (as letter Int))
  (print (as 66 Char))
  (print 67c)
  (print (. letter uppercase))
  (print (as (+ letter 1) Char))
  (print (< 'a' 'b'))
  (print (as "Hi" Char))
  (print (str "[" (char " ") "]"))
  (print (. (char "漢") display-width))
  (print (== (. (char "\u0301") category) .mark-nonspacing))
  (for (in c (.. 'a' 'e'))
    (print c)
  )
)
```

```output
a
97
B
C
A
b
true
H
[ ]
2
true
a
b
c
d
e
```

- **Literals.** A visible ASCII character is written in single quotes, as in `'A'`, except for
  `"`, `'`, `(`, `)`, `;` and `\`. Any character, including those, a space and a non-ASCII
  character, is written as a `char` form with a one-character text, as in `(char "é")`{.brix}. A text of
  any other length is an error: `(char …) takes a text of one character, as in (char "a"), but
  this one has 2`. A number with the suffix
  `c` is the character with that code, as in `65c`.
- **Numbers.** A character converts to and from a number only when asked:
  `(as 'A' Int)`{.brix} is `65`, and `(as 65 Char)`{.brix} and `(call Char 65)`{.brix} are `A`.
  Without `as`, the error names the conversion: `Expected Char here, but this is I64, which
  becomes Char only when asked; write "as Char"`.
- **Arithmetic** treats characters as `U32`: `(+ 'a' 1)`{.brix} is the number `98`, and
  `(- 'b' 'a')`{.brix} is `1`. Convert the result back with `as Char`.
- **Text.** A character becomes text inside a text, in a print, or with `(as c String)`{.brix}.
  `(as "Hi" Char)`{.brix} and `(call Char "Hi")`{.brix} give the first character, and on empty
  text they panic: `Index out of bounds (0 / 0)`.
- **Comparison** is by code point. A character never equals a text: `Operator "==" is not
  defined for Char and String`.
- **Ranges** of characters loop through the codes between them; see Control flow.

| Member | Value |
| --- | --- |
| `(. c uppercase)`{.brix} | The uppercase character, or `c`. |
| `(. c lowercase)`{.brix} | The lowercase character, or `c`. |
| `(. c display-width)`{.brix} | The terminal columns it takes: 0, 1 or 2. |
| `(. c category)`{.brix} | Its Unicode general category, a `UTF8Category` such as `.letter-uppercase`, `.number-decimal` or `.separator-space`. |

`Char8` is a C `char`, one byte, written with the suffix `c8`. Its `uppercase` and `lowercase`
change ASCII letters only. See Native code and memory.

## Indexing and slicing

`([] text index)`{.brix} gives the character at that index, counting characters from 0. A range
gives the text it covers.

```brix
(main
  (let word "foobar")
  (print ([] word 0))
  (print ([] word .last))
  (print ([] word (..< 0 3)))
  (print ([] word (.. 0 3)))
  (print ([] word (... 3)))
  (print ([] word (... _ 3)))
  (print ([] word (.. 4 1)))
  (let before word)
  (= ([] word 0) "J")
  (= ([] word (.. 1 2)) "ee")
  (= ([] word .last) 'Z')
  (print word)
  (print before)
)
```

```output
f
r
foo
foob
bar
foo
ooba
JeebaZ
foobar
```

- **Indexes** may be any integer type, or a `Position`: `.first`, `.middle` (half the length),
  `.last`, `.end` (the length) and `(call .index n)`{.brix}.
- **Out of range** panics with the index and the length, when reading and when assigning:
  `Index out of bounds (6 / 6)`. A negative index panics too.
- **Ranges.** `(.. a b)`{.brix} includes `b`, and `(..< a b)`{.brix} stops before it.
  `([] text (... a))`{.brix} runs from `a` to the end, and `([] text (... _ b))`{.brix} from the
  start to before `b`. A range that counts down covers the same characters as the one that counts
  up. A range past either end panics: `Range out of bounds (8 / 5)`. A range with a step other
  than 1 panics: `Only increments of 1 are currently supported`.
- **Assigning** to an item or a range stores a new text in the variable, property or item that
  held the old one. A copy made before keeps the old text. The new part may be a text of any
  length, or a `Char`. The assignment's value is the new text.

## Comparing text

```brix
(main
  (let composed "\u00e9")
  (let decomposed "e\u0301")
  (print (== composed decomposed))
  (print (== composed (. decomposed normalized)))
  (print (< "Z" "a"))
  (print (< "apple" "apricot"))
  (print (>= "b" "a"))
)
```

```output
false
true
true
true
true
```

- **Equality** compares the bytes. Two texts that look the same can differ, as a precomposed `é`
  and an `e` with a combining accent do. `normalized` makes both the precomposed form.
- **Order** is by code point, so every uppercase ASCII letter comes before every lowercase one.
  It is not a dictionary order for any language.

## Operators on text

| Operation | Value |
| --- | --- |
| `(+ a b)`{.brix} | `a` followed by `b`, where `b` is a text or a `Char`. |
| `(++ a b)`{.brix} | `a` followed by `b` converted to text; `a` must be a text. |
| `(+= text more)`{.brix} | Appends a text or a `Char`. |
| `(++= text value)`{.brix} | Appends any value, converted to text. |
| `(* text count)`{.brix} | The text repeated `count` times. |
| `(== a b)`{.brix}, `(< a b)`{.brix} and the other comparisons | See [Comparing text](#comparing-text). |
| `(not text)`{.brix} | `true` when the text is empty. |

```brix
(main
  (print (+ "ab" "cd"))
  (print (+ "ab" 'c'))
  (print (++ "count: " 5))
  (print (++ "on: " true))
  (print (* "=" 10))
  (let line "a")
  (+= line "b")
  (++= line 3)
  (print line)
)
```

```output
abcd
abc
count: 5
on: true
==========
ab3
```

- **`+` takes text only.** `(+ "foo" 3)`{.brix} is an error, `Operator "+" is not defined for
  String and I64`; use `++` or a `str` form.
- **`++` needs text on its left.** `(++ 5 "x")`{.brix} is an error: `Operator "++" is not
  defined for I64 and String`.
- **`*` needs a count on its right**, of any integer type. A count of 0 gives `""`, and a
  negative count panics: `Repeated String size overflow`.
- **No subtraction or membership.** `-`, `has` and `is in` are not defined for text. To look for
  a part, use `find`, `starts-with` or `ends-with`, or a text pattern; see Patterns.

## String members

```brix
(main
  (let path "src/lib/main.bx")
  (print (call (. path find) "/"))
  (print (call (. path find) "/" 4))
  (print (call (. path find-reverse) "/"))
  (print (call (. path find) "?"))
  (print (call (. path starts-with) "src"))
  (print (call (. path ends-with) ".bx"))
  (print (call (. "  padded \n" trim)))
  (print (call (. "hello world" truncate) 5))
  (print (. "Straße" uppercased))
  (print (. "ÀB" lowercased))
  (for (in part (call (. "a,b,,c" split) ","))
    (print (str "[" part "]"))
  )
  (print (call (. " | " join) (list 1 2 3)))
  (print (call ", " (list "x" "y")))
  (print (call join (list "a" "b") "/" true))
)
```

```output
3
7
7
unknown
true
true
padded
hello...
STRAẞE
àb
[a]
[b]
[]
[c]
1 | 2 | 3
x, y
a/b/
```

| Member | Value |
| --- | --- |
| `(. s length)`{.brix} | The number of characters. |
| `(. s size)`{.brix} | The number of bytes. |
| `(. s display-width)`{.brix} | The number of terminal columns. |
| `(call (. s find) part)`{.brix} | The index of the first `part` at or after index 0, or `null`. It takes the start index as a second argument. |
| `(call (. s find-reverse) part)`{.brix} | The index of the last `part`, or `null`. |
| `(call (. s starts-with) part)`{.brix} | Whether `s` begins with `part`. |
| `(call (. s ends-with) part)`{.brix} | Whether `s` ends with `part`. |
| `(call (. s trim))`{.brix} | `s` without spaces, tabs, carriage returns and line feeds at either end. |
| `(call (. s truncate) n)`{.brix} | The first `n` characters followed by `...`, when `s` is longer than `n`; otherwise `s`. |
| `(call (. s split) delimiter)`{.brix} | A stream of the parts between delimiters. |
| `(call (. s join) items)`{.brix} | The items of a stream as text, with `s` between them. |
| `(call s items)`{.brix} | The same as `join`. |
| `(. s uppercased)`{.brix} | `s` with each letter uppercase. |
| `(. s lowercased)`{.brix} | `s` with each letter lowercase. |
| `(. s normalized)`{.brix} | `s` in Unicode normal form C, with characters composed. |
| `(. s decomposed)`{.brix} | `s` in Unicode normal form D, with characters decomposed. |
| `(. s codepoints)`{.brix} | An array of its characters; see Collections. |

- **`find`** returns an `I32?` index counted in characters. An empty `part` is found at the
  start.
- **`split`** gives a stream, not a list: a loop reads it, and `(call List parts)`{.brix}
  collects it; see Streams. Empty parts are kept, including one after a trailing delimiter, and
  `(call (. "" split) ",")`{.brix} gives one empty part. An empty delimiter panics: `String split
  delimiter must not be empty`.
- **`join`** converts each item to text. The free function `(call join items delimiter
  has-tail)`{.brix} does the same, and with `has-tail` true it also puts the delimiter after the
  last item.
- **Case** changes one character into one character, so `ß` becomes `ẞ`, not `SS`.
- **`trim`** removes ASCII white space only.

## Converting to and from text

```brix
(main
  (print (as 42 String))
  (print (call String 2.5))
  (print (call (. 7 pad) 3))
  (print (call (. 3.14159 places) 2))
  (print (as 255u8 Hex))
  (print (as 5u8 Binary))
  (print (+ (as " 42" Int) 1))
  (print (* (as "2.5" Float) 2))
  (print ([] (as "é" ([] Byte)) 0))
  (print (call (. (as "héllo" ([] Byte)) slice-string)))
)
```

```output
42
2.5
007
3.14
0xff
00000101
43
5
195
héllo
```

- **To text.** Any value converts with `(as value String)`{.brix} or `(call String value)`{.brix},
  using its type's `to String`. A type declares its own with `(fn-to-expr + String body)`{.brix}; see
  Conversions.
- **Numbers as text.** `pad` writes a number with at least that many digits, padded with zeros.
  `places` writes a number with that many digits after the point. `Hex` and `Binary` write an
  integer in base 16 and base 2, padded to the width of its type.
- **Text to numbers.** Text becomes a number only when asked, with `as` or by calling the
  type. White space around the number and a leading `+` are allowed. Text that is not a number of
  that kind panics: `"42x" is not a whole number` for an integer type, and `"2.5x" is not a
  number` for a floating type.
- **Bytes.** `(as text ([] Byte))`{.brix} gives a copy of the UTF-8 bytes of a text, and
  `slice-string` reads bytes back as text. Bytes that are not UTF-8 panic: `Unable to read UTF8
  content`. See Collections.

## Kinds of text

A type can be declared as a kind of text, as in `(type + Tag (is String))`{.brix}. Its values
are texts, and they convert to `String` on their own. The core library declares these kinds:

| Type | Text |
| --- | --- |
| `Hex` | Base 16 digits, as `as Hex` writes them. |
| `Base64` | Base 64 text. |
| `Binary` | Base 2 digits. |
| `UnitString` | A number with its unit, as in `5i`; see Units. |
| `Abbreviation` | A short form of a name. |

## Atoms

An `Atom` is a name kept by its address: two atoms are equal only when they point at the same
text. Atoms make names cheap to store and compare.

```brix
(import /core/atom-pool)

(main
  (let pool (call AtomPool))
  (finally (call (. pool release)))
  (let red (call (. pool store) "red"))
  (let again (call (. pool store) (++ "re" "d")))
  (let blue (call (. pool store) "blue"))
  (print (== red again))
  (print (== red blue))
  (print (. red text))
  (print (. pool length))
  (print (call (. pool ordinal) blue))
)
```

```output
true
false
red
2
1
```

- **Pools.** An `AtomPool`, from `/core/atom-pool`, stores each distinct text once, so storing
  the same text again gives the same atom. `release` frees the pool, and its atoms with it.
- **From text.** `(as text Atom)`{.brix} makes an atom of a text without copying or pooling it.
  Two such atoms are equal only when they come from the same text in memory.
- **Members.** `text` is the atom's text, and `address` its address as a `U64`.
- **No conversion on its own.** An atom is not text. It must be converted to be printed, joined
  or compared with text, as in `(. red text)`{.brix}: `Operator "==" is not defined for Atom and
  String`.
