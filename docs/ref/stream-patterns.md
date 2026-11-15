# Stream patterns

A stream pattern describes a sequence of items: the characters of a text, the bytes of a file, or
the values of a list or a stream. A `match` tests a value against patterns, a rule names a pattern
so that others can use it, and `~>` reads one match after another from a stream. Together these
are a grammar language for parsing.

Patterns match the way parsing expression grammars do. A choice takes the first alternative that
matches and never comes back to try another, and a repetition takes as many items as it can.

Structural patterns, which take apart one value, are in Patterns. This chapter covers patterns
over sequences of items.

## Matching text

```brix
(main
  (for (in text (list "v1.2.3" "v10.0.7-beta" "1.2" "v1.2.x"))
    (print (call version text))
  )
)

(fn-expr + version String
  (param text String)
  (match text
    (~ (seq "v" (: major (of \num Int)) "." (: minor (of \num Int)) "." (: patch (of \num Int)) (? (seq "-" (: tag (+ (chars "a-z"))))))
      (str "major " major ", minor " minor ", patch " patch ", tag " (?? tag "none"))
    )
    (default
      (inline (str "not a version: " text))
    )
  )
)
```

```output
major 1, minor 2, patch 3, tag none
major 10, minor 0, patch 7, tag beta
not a version: 1.2
not a version: v1.2.x
```

- **Arms.** A `~` arm of a `match` holds a pattern, then an action: the rows that give the arm's
  value. The arms are tried in order, and the first that matches gives the value.
- **The whole input.** An arm matches only when its pattern takes every item of the subject.
  `"v1.2.x"` fails the first arm, since `x` is not a number.
- **Fallbacks.** A `default` arm gives a value when no `~` arm matches, and a `throw` arm raises an
  error. Without either, the `match` gives an optional, `null` when nothing matches.
- **Captures.** `(: name \rule)`{.brix} gives a name to what a part matched, which the action
  reads; see [Captures](#captures).
- **Actions.** The action reads `it` as the whole text the arm matched. An arm with no action gives
  that text, converted to the `match`'s type.

## Pattern pieces

In this table, `\a`, `\b` and `\p` stand for any patterns.

| Form | Matches |
| --- | --- |
| `"text"` | Those characters, in order. |
| `'c'` | One item equal to the character. |
| `5` | One item equal to the number. |
| `.` | Any one item. |
| `(chars "a-z0-9_")`{.brix} | One character of a set; see [Character sets](#character-sets). |
| `(\{} (> it 0))`{.brix} | One item for which the condition holds, read as `it`. |
| `$name` | The value of `name`: the characters of a text, one item equal to a character or a number, or the items of a pattern value. |
| `\rule` | What the rule matches; see [Rules](#rules). |
| `Type` | A value of the type made by the type's stream constructor, or one item of the type. |
| `/`, `//` | Spaces and tabs: any number with `/`, and at least one with `//`. |
| `(seq \a \b)`{.brix} | `\a`, then `\b`. |
| `(\| \a \b)`{.brix} | `\a`, or else `\b`. |
| `(? \p)`{.brix} | `\p` or nothing. |
| `(* \p)`{.brix}, `(+ \p)`{.brix} | `\p` any number of times, or at least once. |
| `({} \p n)`{.brix}, `({} \p low high)`{.brix} | `\p` exactly `n` times, or from `low` to `high` times. |
| `(& \p)`{.brix} | Nothing, when `\p` would match here. |
| `(! \p)`{.brix} | Nothing, when `\p` would not match here. |
| `(: name \p)`{.brix} | `\p`, and gives the name what it matched. |

- **Choices commit.** `(\| \a \b)`{.brix} takes `\b` only when `\a` fails at that point, and once
  `\a` has matched, a later failure does not go back to try `\b`. So `(seq (\| "a" "ab") "c")`{.brix}
  never matches `"abc"`: order the longer alternative first.
- **Repetitions are greedy.** `(* \p)`{.brix} takes every `\p` it can and gives none back, so
  `(seq (* "a") "a")`{.brix} never matches. A repetition of something that can match nothing still
  ends.
- **Counts** in `{}` may be any expression, such as a number captured earlier.
- **End of input** is `(! .)`{.brix}: no item follows.
- **White space.** `/` and `//` call the rule `ws`. A local rule named `ws` changes what they
  match.

### Character sets

`(chars "a-z0-9_")`{.brix} matches one character that is in a range, such as `a-z`, or is one of
the characters listed. A range or a character may be written `\uXXXX`. A `-` at the start or the
end stands for itself. A set has no negation; write `(seq (! (chars "abc")) .)`{.brix} for any
character but those.

### Built-in rules

These rules are always available over characters:

| Rule | Matches |
| --- | --- |
| `\alpha`, `\numeric`, `\alphanumeric` | One ASCII letter, digit, or either. |
| `\ws`, `\wsnl` | One space or tab; one of those or a line break. |
| `\cr`, `\lf`, `\nl` | A carriage return, a line feed, or any line break: `"\r\n"`, `"\r"` or `"\n"`. |
| `(of \num Int)`{.brix} | A whole number of that type in decimal, such as `0`, `42` or `-7`, and gives it. |
| `(of \dec Float)`{.brix} | A decimal number such as `2`, `-2.5` or `.5`, with no exponent, and gives it. |
| `(of \hex U8)`{.brix} | From one hex digit to as many as the type holds, and gives the number. |
| `(call \delimited "," pattern)`{.brix} | The pattern one or more times, with the delimiter and optional spaces between, and gives a list. |

### In rows

In rows, a sequence is written with spaces between its parts, as in `"v" major \num'Int`{.rows},
and a choice with `|`. A choice binds more tightly than a sequence, so `"a" "b" | "c" "d"`{.rows}
is `"a" ("b" | "c") "d"`{.rows}. A set is written in brackets, `[a-z0-9_]`{.rows}, and a count in
braces, `p{2, 4}`{.rows}.

## Captures

```brix
(main
  (print
    (match "ab12cd"
      (~ (seq (: word (+ (chars "a-z"))) (: digits (+ \digit)) (: rest (* .)))
        (str "word=" word " digits=" digits " sum=" (+ ([] digits 0) ([] digits 1)) " rest=" rest)
      )
    )
  )
  (print
    (match "k=1;k=2;k=3"
      (~ (seq "k=" (: first (chars "0-9")) (* (seq ";k=" (: more (chars "0-9")))))
        (str "first=" first " more=" more)
      )
    )
  )
  (print
    (match "a1b2c3"
      (~ (+ (seq (: ...letters (chars "a-z")) (: ...numbers \digit)))
        (str "letters=" letters " numbers=" numbers)
      )
    )
  )
  (print
    (match "x"
      (~ (seq (: name (chars "a-z")) (? (seq ":" (: kind (+ (chars "a-z"))))))
        (str "name=" name " kind=" (?? kind "none"))
      )
    )
  )
)

(fn-expr + digit Int (over: Char)
  (~ (chars "0-9")
    (call Int it)
  )
)
```

```output
word=ab digits=1, 2 sum=3 rest=cd
first=1 more=2, 3
letters=a, b, c numbers=1, 2, 3
name=x kind=none
```

| Captured part | Type |
| --- | --- |
| Text, a set, `.` over characters, a group, a lookahead | `String` |
| `'c'`, `(\{} (> it 'a'))`{.brix}, `$` of a character | `Char` |
| `.` or a type over other items | The item's type. |
| A rule | The rule's result type. |
| `(? \p)`{.brix} | An optional of `\p`'s type. |
| `(* \p)`{.brix}, `(+ \p)`{.brix}, `({} \p n)`{.brix} of text | `String`, all of the text. |
| `(* \p)`{.brix}, `(+ \p)`{.brix}, `({} \p n)`{.brix} of a rule or type whose action gives a value | An `ImmutableArray` of the values. |

- **Captures in repetitions.** A capture inside a repeated part gives an array of what it matched
  each time, as `more` does above.
- **Accumulating captures.** `(: ...name \p)`{.brix} adds what `\p` matches to one array for the
  whole pattern, wherever it is nested. Parts that fail are taken back out.
- **Optional parts.** Each capture inside an optional part is optional.
- **Choices.** A name captured in every alternative of a choice has the join of their types. A
  name captured in only some of them cannot be read.
- **One name, one capture.** A name captured twice in one sequence is an error.

## Rules

A rule is a function whose body is pattern arms. Its header names the type of item it reads with
`over:`, and its result type.

```brix
(main
  (for (in text (list "1 + 2 * 3" "(1 + 2) * 3" "2 * (3 + 4) * 5" "2 +"))
    (let value (~ \calc (of: text)))
    (print (str text " = " (?? value -1)))
  )
)

(fn-expr + calc Int (over: Char)
  (~ (seq / (: value \sum) /)
    value
  )
  (where
    (lazy sum Int
      (pattern
        (~ (seq (: left \product) / "+" / (: right \sum))
          (+ left right)
        )
        (~ \product)
      )
    )
    (lazy product Int
      (pattern
        (~ (seq (: left \factor) / "*" / (: right \product))
          (* left right)
        )
        (~ \factor)
      )
    )
    (lazy factor Int
      (pattern
        (~ (seq "(" / (: inner \sum) / ")")
          inner
        )
        (~ (+ (chars "0-9")))
      )
    )
  )
)
```

```output
1 + 2 * 3 = 7
(1 + 2) * 3 = 9
2 * (3 + 4) * 5 = 70
2 + = -1
```

- **Declaring.** A rule is declared with `fn-expr`, an `over:` option naming its item type, and
  `~` arms, as in `(fn-expr + digits Int (over: Char) (~ (+ (chars "0-9"))))`{.brix}. Rules over
  other items name another type, such as `Byte` or `Int`.
- **Local rules.** A lazy binding whose value is a `pattern` form is a rule. One in a rule's
  `where` can be called by that rule's arms and by the other local rules, and one written below
  the arms of a `match` by those arms.
- **Calling.** `\name` calls a rule, `(call \name argument)`{.brix} passes it arguments, and
  `(of \name Type)`{.brix} gives it a type argument. Rules declared with `fn-expr` take
  parameters; local rules do not.
- **Results.** An arm's action gives the rule's value. An arm without an action gives the text it
  matched, converted to the rule's result type. A rule with no result type gives the join of its
  actions' values.
- **Recursion.** A rule may call itself, and rules may call each other, as `sum`, `product` and
  `factor` do. A rule that calls itself must state its result type: `Can't infer return type
  after recursion`. A rule that calls itself before reading anything would never end, and is an
  error: `"sum" calls itself before reading any input, so it never ends; read input before this
  call, or write the repetition with "*"`.
- **Guards.** A rule's arms may sit under `case?` conditions and a `default`, as in a decision
  table. A last arm that is a value, in a `default`, always matches and takes nothing. A last arm
  that is a `throw` raises; see [Failure](#failure).

### Patterns as arguments and types as patterns

```brix
(main
  (print (match "[(1,2), (3,4)]" (~ (seq "[" (: points (call \delimited "," (~ Point))) "]") points)))
  (print (match "<b>bold</b>" (~ (call \tagged "b"))))
  (print (match "<i>x</b>" (~ (call \tagged "i")) (default (inline "mismatched"))))
  (print (match "ff" (~ (: value (of \hex U8)) (+ (as value Int) 1))))
)

(type + Point (is (tuple (: x Int) (: y Int)))
  (fn-expr + This _ (over: Char)
    (~ (seq "(" (: x (of \num Int)) "," / (: y (of \num Int)) ")")
      (call This x y)
    )
  )
)

(fn-expr + tagged String (over: Char)
  (param name String)
  (~ (seq "<" $name ">" (: body (* (seq (! "<") .))) "</" $name ">")
    (str name ": " body)
  )
)
```

```output
(1, 2), (3, 4)
b: bold
mismatched
256
```

- **Pattern arguments.** A `~` form as an argument, as in
  `(call \delimited "," (~ Point))`{.brix}, passes a pattern, which the rule runs
  where it reads the parameter, as `delimited` does.
- **Stream constructors.** A constructor with an `over:` option and pattern arms, as `Point` has, is run by naming the type in a pattern. A type with none is an error there: `Type has no
  stream constructor for this stream item type`.
- **Values in patterns.** `$name` matches the current value of `name`, so a rule can match a text
  it was given, or one it captured earlier.

## Other kinds of item

A pattern can match any sequence, not only text.

```brix
(main
  (print (call describe (list 1u8 0u8 3u8 104u8 105u8 33u8)))
  (print (call describe (list 1u8 0u8 9u8 104u8)))
  (print (call flags (list "-v" "-o" "out.txt" "in.txt")))
  (let shapes (list Shape) (list (call Circle 1.0) (call Square 2.0) (call Square 3.0)))
  (print
    (match shapes
      (~ (seq (: c Circle) (: squares (+ Square)))
        (str "circle " (. c r) " then " (. squares length) " squares")
      )
      (default
        (inline "other")
      )
    )
  )
)

(fn-expr + describe String
  (param packet (list Byte))
  (match packet
    (~ (seq (: kind U8) (: size U16) (: body ({} Byte size)))
      (str "kind " kind ", " (. body size) " bytes, first " ([] body 0))
    )
    (default
      (inline "malformed")
    )
  )
)

(fn-expr + flags String
  (param arguments (list String))
  (match arguments
    (~ (seq (: options (* (\{} (call (. it starts-with) "-")))) (: files (* .)))
      (str (. options length) " options, " (. files length) " files")
    )
    (default
      (inline "none")
    )
  )
)

(type + Shape)

(type + Circle (is Shape)
  (prop + r Float)
  (fn-do + This _
    (param r _)
  )
)

(type + Square (is Shape)
  (prop + side Float)
  (fn-do + This _
    (param side _)
  )
)
```

```output
kind 1, 3 bytes, first 104
malformed
2 options, 2 files
circle 1 then 2 squares
```

- **Bytes.** `.` is one byte. A number type reads that many bytes, highest first, so `U16` reads
  two. `({} Byte n)`{.brix} takes `n` bytes. A number such as `104u8` matches one byte equal to it.
  Text and sets do not match bytes.
- **Other values.** `.` is one item, `(\{} (> it 0))`{.brix} tests one, a number matches one equal item,
  and a type name matches one item of that type.

## Running patterns

| Form | Gives |
| --- | --- |
| A `match` with `~` arms | The value of the first arm that matches; see [Matching text](#matching-text). |
| `(~ \record (of: subject))`{.brix} | The pattern's value when it matches the subject, or `null`. |
| An `if` whose condition is `(= (~ \record) subject)`{.brix} | Runs its rows when the pattern matches, with its captures. |
| `(~ \record (in: source))`{.brix} | A stream of the values of one match after another. |
| `(~> source \record)`{.brix} | A stream of one match after another; see [Reading records](#reading-records). |

```brix
(main
  (if (= (~ (seq "v" (: major (+ (chars "0-9"))) "." (: minor (+ (chars "0-9"))))) "v12.4")
    (print (str "major " major ", minor " minor))
  )
  (let words
    (~ (seq (: w (+ (chars "a-z"))) (? " ")) (in: "ab cd 12 ef")
      w
    )
  )
  (print (call (. ", " join) words))
  (print (call word-of (call letters)))
  (print (call word-of "gh!"))
)

(fn-do + letters (of Iterable Char)
  (yield 'x')
  (yield 'y')
)

(fn-expr + word-of String
  (param source (stream Char))
  (match source
    (~ (+ (chars "a-z")))
    (default
      (inline "none")
    )
  )
)
```

```output
major 12, minor 4
ab, cd
xy
none
```

### Subjects

A pattern can match a text, an array or a list, a range, a generator, a stream parameter, a map's
entries, a channel, or a file or socket, whose bytes are read as characters or bytes. A number is
not a sequence: `Can't convert to iterable`.

A `TokenStream`, from `/core/tokenstream`, is a source that remembers how far it has been read.
A pattern matches only as much of it as it needs, and the rest stays for the next match. Over any
other subject, a `match` arm must take the whole input.

### Reading records

`(~> source \record)`{.brix} gives a stream of one match after another: each match starts where
the last one ended. The stream is lazy, and ends at the first point where the pattern does not
match.

```brix
(import /core/tokenstream)

(main
  (let input (call (of ChunkedTokenStream Char) (. "ada:36;bob:41;oops;cy:9;" codepoints)))
  (for (in person (~> input \person))
    (print person)
  )
  (print (str "stopped at " (. input location)))
  (print
    (match input
      (~ (: rest (* .))
        (str "left: " rest)
      )
    )
  )
)

(fn-expr + person String (over: Char)
  (~ (seq (: name (+ (chars "a-z"))) ":" (: age (+ (chars "0-9"))) ";")
    (str name " (" age ")")
  )
)
```

```output
ada (36)
bob (41)
stopped at (0, 14)
left: oops;cy:9;
```

- **Stage rows.** In a block, a `pipe` row with `~>` reads records from the value
  above it, and its action gives each record's value; see Streams.
- **Progress.** Each record must take at least one item. A pattern that can match nothing is an
  error: `Stream pattern pipeline must consume input`. One that matches nothing at run time panics:
  `Stream pattern pipeline made no input progress`.
- **Item type.** `~>` reads items of the type the rule declares. A text given to a rule over bytes
  must be converted: `(as text ([] Byte))`{.brix}.

### Failure

A rule's last arm may be a `throw`. It raises when no other arm matches, and inside `~>` it
raises for leftover input that is not a record, where the stream would otherwise end.

```brix
(main
  (print (catch (call total "3,4,5")))
  (print (catch (call total "3,x,5")))
)

(type + BadList (is Error))

(fn-do + total Int (?! BadList)
  (param text String)
  (let sum 0)
  (for (in n (~> text \item))
    (+= sum n)
  )
  (return sum)
)

(fn-expr + item Int (over: Char) (?! BadList)
  (~ (seq (: digits (+ (chars "0-9"))) (? ","))
    (as digits Int)
  )
  (throw (call BadList (str "bad item at column " (+ (. location column) 1))))
)
```

```output
12
bad item at column 3
```

### Locations

`location` in an action is where the arm matched, a `MatchLocation` of its `begin` and `end`.
Each is a `TokenLocation` of a `line` and a `column`, both counted from 0, and `end` is the last
item matched. In a `throw` arm, `location` is the farthest point the match reached before failing.
Lines end at `"\n"`, `"\r\n"` or `"\r"`, and a column counts characters.

### Helpers

A `with` form as a `match`'s first row, or after the pattern of `~>`, gives the match an object
that takes part in it. A helper implements one or more interfaces from `/core/parsing`, such as
`PatternRecordBoundary`, whose `begin-pattern-record` runs before each record.

```brix
(main
  (let counter (call Counter))
  "one\ntwo\nthree\n"
  (pipe ~> (seq (: word (+ (chars "a-z"))) \nl)
    (with counter)
    (str word " spans " location)
  )
  (pipe |> show)
  (print (str (. counter count) " records"))
)

(fn-do + (of show Item) _
  (param items (stream Item))
  (for (in item items)
    (print item)
  )
)

(type + Counter
  (implements PatternRecordBoundary)
  (prop + count Size)
  (fn-do + begin-pattern-record _
    (+= count 1)
  )
)
```

```output
one spans ((0, 0), (0, 3))
two spans ((1, 0), (1, 3))
three spans ((2, 0), (2, 5))
3 records
```
