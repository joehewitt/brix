# Patterns

A pattern describes a shape a value can have: a value it equals, a type it has, a condition it
meets, or parts it is made of. Matching a value against a pattern tells whether the value fits,
and names the parts the pattern names.

This chapter covers `match`, every kind of pattern, the other places patterns are written, taking
values apart into locals, and the rule that a `match` covers every case. Unions are covered in
Unions and enums, and patterns over streams of text and bytes in Grammars.

## `match`

```brix
(main
  (for (in value (list 0 -4 7 25 12))
    (print (call size value))
  )
  (let count 3)
  (let word
    (match count
      (case= 1
        "one"
      )
      (case? (< it 5)
        "a few"
      )
      (default
        (inline "many")
      )
    )
  )
  (print word)
)

(fn-expr + size String
  (param value Int)
  (match value
    (case= 0
      "zero"
    )
    (case ({} (< it 0))
      "negative"
    )
    (case (| 3 7)
      "lucky"
    )
    (case (: n ({} (> it 20)))
      (str "large " n)
    )
    (case n
      (str "small " n)
    )
  )
)
```

```output
zero
negative
lucky
large 25
small 12
a few
```

A `match` takes a subject, then arms, each with its value, as in
`(match subject (case pattern value))`{.brix}.

| Arm | Taken when |
| --- | --- |
| `(case pattern value)`{.brix} | The subject fits the pattern. |
| `(case= value result)`{.brix} | The subject equals the value. |
| `(case? condition value)`{.brix} | The condition holds. |
| `(default value)`{.brix} | No arm before it was taken. |
| `(throw error)`{.brix} | No arm before it was taken; it raises the error. See Errors. |

- **Order.** Arms are tried in the order written, and the first that is taken gives the match its
  value. Later arms are not tried.
- **`it`** is the subject, in every arm's pattern, condition and value. In an arm whose pattern
  names a type, `it` has that type.
- **Values.** An arm's value is its body. A default arm can hold its value inline, on one row:
  `(default (inline value))`{.brix}. An arm with no value gives what its pattern passed on, as in
  `(case ({} (> it 20)))`{.brix}, which gives the subject.
- **Type.** The match's type is the join of its arms' values; see Conversions. Arms whose types
  have no join give an anonymous union; see Unions and enums. Arms that raise add nothing to it.
  To keep each arm's type even where a join exists, see
  [Arms that keep their type](#arms-that-keep-their-type).
- **Value first.** `(select value (| pattern))`{.brix} is an arm written with its value before
  its pattern, and is taken as `(case pattern value)`{.brix} is.
- **Raising arms.** `(throw error)`{.brix} raises for every value no arm before it took. With a
  condition, as in `(throw error (if condition))`{.brix}, it raises only where the condition holds,
  and the arms after it are tried otherwise.
- **Lazy locals.** An arm can declare lazy locals after its value, as a value body can; see
  Functions.
- **No arm taken.** Where no arm fits a value, the program panics: `Pattern match failed`. For a
  subject of a type with a fixed set of values, the compiler requires an arm for each; see
  [Covering every case](#covering-every-case).

### Arms that keep their type

```brix
(main
  (for (in sound (list "woof" "meow"))
    (let pet (call adopt sound))
    (let kept (call adopt-kept sound))
    (print (str (type-of pet) " / case " (. kept case) ", a Dog: " (is kept Dog)))
  )
)

(type + Animal)

(type + Dog (is Animal))

(type + Cat (is Animal))

(fn-expr + adopt _
  (param sound String)
  (match sound
    (case= "woof" (call Dog))
    (default (call Cat))
  )
)

(fn-expr + adopt-kept _
  (param sound String)
  (match sound
    (case-union= "woof" (call Dog))
    (default (call Cat))
  )
)
```

```output
[TypeInfo Animal] / case 0, a Dog: true
[TypeInfo Animal] / case 1, a Dog: false
```

`case-union`, `case-union=` and `case-union?` are taken as `case`, `case=` and `case?` are, but
their values are not joined. From the first such arm on, the match's type is a union with a case
for each arm, so `adopt` gives an `Animal` and `adopt-kept` gives a `Dog | Cat`. In rows they are
`||`, `||=` and `||?`.

- **Later arms.** Every arm after the first `case-union` arm adds its case to the union, whatever
  its kind, as the `default` arm does above.
- **Earlier arms** are not in the union, so their values must convert to it:
  `Expected I64 | F64 here, but this is String, and there is no conversion between them`.
- **A declared type.** Where the match's type is declared, as by a function's result, its values
  convert to that type and no union is made.

### `match?`

`(match? subject (case pattern value))`{.brix} gives `null` where no arm is taken, so its type is
an optional, and it needs no arm for every value.

- **An arm for every value.** A `(default value)`{.brix} arm, or a `(throw error)`{.brix} arm with
  no condition, is taken wherever no arm before it is, so a `match?` with one would never differ
  from a `match`. It is an error that names the fix: `"match?" differs from "match" only where no
  arm is taken, and this default arm is taken wherever no arm before it is; write "match"`.

### Matches as steps

A `match` on a row of its own in a body of steps, `(match subject (case pattern body))`{.brix},
runs the arm that is taken, as steps, and uses no value, so arms may give values of any type or
none. A body of steps is one whose rows run in order: that of a `fn-do`, `main`, a `do` form, an
`if` or `else`, a loop, a `subscribe-for`, a closure with `:do`, or an arm of a match run as steps.

- An arm can return from the function, raise, or `break` or `continue` an enclosing loop.
- With `match?`, a match run as steps goes on to the next step where no arm is taken; with
  `match`, it panics there.
- A `match` whose value is used, as a `let`'s value, a function's result or what a `pipe` stage
  below it works on, gives the value of the arm it takes: that of the arm's last row, once the
  rows before it have run as steps. An arm whose last row gives no value, such as a `print`, is an
  error that names the fix: `This arm gives no value, and here a match gives the value of the arm
  it takes; to run its arms as steps, write the match on a row of its own in a body of steps`.
  Where the body that holds the match is a value, as that of a `fn-expr` or a closure without
  `:do` is, the fix it names is `fn-do`, or `:do` in a closure. A `match` written as a row of
  markup content runs its arm as content instead; see Markup.
- A function whose last step is a match run as steps, with an arm for every value, each of which
  returns or raises, needs no return after it.

### Several subjects

`(match (tuple a b) (case (tuple x y) value))`{.brix} matches the tuple of its subjects, so each
arm is a tuple pattern, as in
`(case (tuple (: name String) (: count ({} (> it 20)))) value)`{.brix}. See [Tuples](#tuples).

## Kinds of pattern

| Pattern | Fits |
| --- | --- |
| `_` | Any value. It names nothing. |
| `name` | Any value, and names it. |
| `42`, `"text"`, `null` | A value equal to it. |
| `Type` | A value of the type. |
| `(: name Type)`{.brix} | A value of the type, and names it. |
| `(exact Type)`{.brix} | A value of exactly the type, not one that converts to it. |
| `(similar Type)`{.brix} | A value that converts to the type. |
| `({} condition)`{.brix} | A value for which the condition holds. |
| `(all a b)`{.brix} | Both patterns, in turn. |
| `(\| a b)`{.brix} | Either pattern. |
| `(tuple a b)`{.brix} | A tuple whose fields fit. |
| `.case`, `(call .case a b)`{.brix} | A union value holding the case. |
| `(list a b)`{.brix}, `(case (list a ...rest) value)`{.brix} | A list whose items fit. |
| `(case (map "key" a) value)`{.brix} | A map holding the key, with a value that fits. |
| `(all "start" ...rest)`{.brix} | Text that begins and ends as written. |
| `..name` | A value with a property `name`, which it names. |

- **Tree relations.** `(/ a b)`, `(*/ a b)`, `(\ a b)`, `(*\ a b)`, `(++ a b)` and `(-- a b)` are
  reserved for patterns over trees: a child, a descendant, a parent, an ancestor, the next sibling
  and the previous sibling. The compiler reads them and refuses them: `Not supported`.

### Values and conditions

- **Literals.** A number, text, `null` or other literal fits an equal value. A number literal takes
  the subject's type, so the arm `(case 3 value)`{.brix} matches a `U8`.
  `(case= value result)`{.brix} compares with any value, not only a literal, as
  `(case= limit result)`{.brix} does.
- **Conditions.** `({} condition)`{.brix} fits where the condition holds, with `it` naming the
  value being matched at that point of the pattern. A condition can read the names given before it
  in the pattern, as in `(all (: n Int) ({} (> n 10)))`{.brix}.
- **Alternatives.** `(| a b)`{.brix} tries each pattern in turn, and fits where one does. All of
  them share the arm's value. A name the alternatives give is one local, which the alternative
  taken fills, so `(| (call .a n) (call .b n))`{.brix} reads `n` from either case.
- **Sequences.** The patterns of `(all a b)`{.brix} must all fit, from left to right, and each takes
  what the one before passed on. A type pattern passes on the value as its type,
  `(similar Type)`{.brix} passes on the converted value, and a condition passes on what it tested,
  so in `(all (: n (similar Int)) ({} (> it 7)))`{.brix} the condition tests the converted value.

### Names

- **A name alone** fits any value and names it, so no arm may follow it: `Arm "(case n …)" takes
  every value, so the arms after it are never reached`. `_` is the same, and names nothing.
- **A name before a pattern** names what fits the pattern, as in `(: n ({} (> it 20)))`{.brix} and
  `(: n Int)`{.brix}.
- **Scope.** A name a pattern gives is known in its arm's value, or the body the pattern guards,
  and not after it. Two arms may give the same name, but one pattern may not give a name twice:
  `"a" is bound twice in this pattern; give each place a name of its own`.
- **Case names.** A name alone is a name even where the subject's union has a case of that name:
  `Arm "(case red …)" binds a new name, not the case Light.red; match the case with "(case .red …)"`.

### Types

```brix
(main
  (let pets (list Animal) (list (call Dog) (call Cat) (call Animal)))
  (for (in pet pets)
    (print (call describe pet))
  )
  (let maybe (? Int) null)
  (print (call presence maybe))
  (= maybe 4)
  (print (call presence maybe))
  (print (call measure 3))
  (print (call measure 2.5))
  (print (call measure "seven"))
  (print (call read 4.0))
)

(type + Animal)

(type + Dog (is Animal)
  (fn-expr + bark String
    "woof"
  )
)

(type + Cat (is Animal)
  (fn-expr + meow String
    "meow"
  )
)

(type + Value (is Union)
  (variant Int)
  (variant Float)
  (variant String)
)

(fn-expr + describe String
  (param pet Animal)
  (match pet
    (case (: dog Dog)
      (str "a dog: " (call (. dog bark)))
    )
    (case Cat
      (str "a cat: " (call (. it meow)))
    )
    (default
      (inline "an animal")
    )
  )
)

(fn-expr + presence String
  (param value (? Int))
  (match value
    (case= null
      "nothing"
    )
    (case (: n Int)
      (str "holds " n)
    )
  )
)

(fn-expr + measure String
  (param value Value)
  (match value
    (case (: n Int)
      (str "an Int: " n)
    )
    (case (: f Float)
      (str "a Float: " f)
    )
    (case (: s String)
      (str "text: " s)
    )
  )
)

(fn-expr + read String
  (param value Float)
  (match value
    (case (: n (similar Int))
      (str "converts to " n)
    )
    (default
      (inline "does not convert")
    )
  )
)
```

```output
a dog: woof
a cat: meow
an animal
nothing
holds 4
an Int: 3
a Float: 2.5
text: seven
converts to 4
```

- **Classes.** A class fits an object of the class or of a class derived from it. In the arm, the
  name, or `it`, has the class, so its members can be read.
- **Optionals.** A type fits an optional only where it holds a value of the type, so
  `(: n Int)`{.brix} does not fit `null`. Match `null` with `(case= null value)`{.brix}.
- **Unions.** A type fits a union value holding a case of that type, and names the case's value.
- **Generic classes.** A generic class written bare, such as `List`, fits a value of the class
  with any type arguments.
- **Exact and converted types.** `(exact Type)`{.brix} fits a value of exactly the type, without
  converting it, and an object only of exactly the class, not of a class derived from it.
  `(similar Type)`{.brix} fits a value that converts to the type, as `as` would convert it, and
  passes on the converted value, so `4.0` fits `(similar Int)`{.brix} as `4`. Text does not convert
  to a number this way.

### Union cases

`.case` fits a union value holding the case, and `(call .case a b)`{.brix} also matches the case's
values against patterns. A case of another union can be named in full, as in
`(. Light green)`{.brix}.

- `.case` passes on the union value. `(call .case pattern)`{.brix} passes on what its pattern
  passed.
- Anonymous cases, made with `(call ..case value)`{.brix}, are matched the same way.
- See Unions and enums for unions, and for matching values of enums.

### Tuples

`(tuple a b)`{.brix} fits a tuple whose fields fit the patterns, by position. In rows, it can be
written in parentheses, as in `(x, y)`{.rows}. A field pattern can be any pattern:
`(case (tuple (: name String) (: count ({} (> it 20)))) value)`{.brix},
`(case (tuple (: x ({} (> it 5))) _) value)`{.brix}, or a literal, as in
`(case (tuple 1 name) value)`{.brix}.

### Lists

```brix
(main
  (for (in items (list (list 1 2 3) (list 4) (call (of List Int)) (list 5 6)))
    (print (call shape items))
  )
)

(fn-expr + shape String
  (param items (list Int))
  (match items
    (case (list)
      "empty"
    )
    (case (list only)
      (str "just " only)
    )
    (case (list first second)
      (str "two: " first " and " second)
    )
    (case (list first ... last)
      (str "from " first " to " last)
    )
  )
)
```

```output
from 1 to 3
just 4
empty
two: 5 and 6
```

- **Items.** `(list a b)`{.brix} fits a list of exactly as many items as it has patterns, each
  fitting its pattern. `(list)`{.brix} fits only an empty list.
- **Rest.** `...` stands for any number of items, none included:
  `(case (list first ...) value)`{.brix}, `(case (list ... last) value)`{.brix} and
  `(case (list first ... last) value)`{.brix}. `...name` names them, as a slice of the list's own
  type, which shares the list's storage as any slice does; see Collections. A pattern holds at
  most one rest: `Pattern may contain only one remainder`.
- **Sources.** A list pattern fits lists, arrays, and values of any type that implements
  `Indexable`. It reads the length first, then the items it tests. It never fits text, which has
  patterns of its own.
- **One item.** In `(case (list (: only Int)) value)`{.brix}, the pattern is a list of one item.
  In a type, `(list Int)`{.brix} is a list type.

### Maps

`(list (= "key" pattern) (= "other" pattern))`{.brix} fits a map that holds each key, with a value
that fits its pattern. The map may hold other keys too. `...name` names a map of the entries the
pattern did not name.

```brix
(main
  (let settings (call (of Map String Int)))
  (= ([] settings "width") 10)
  (= ([] settings "height") 4)
  (match settings
    (case (list (= "width" w) (= "depth" d))
      (print (str "box " w " by " d))
    )
    (case (list (= "width" w) ...others)
      (print (str "width " w ", and " (. others length) " more"))
    )
    (default
      (print "no width")
    )
  )
)
```

```output
width 10, and 1 more
```

A key can be text, a number, or the value of a local, written `$name`, as in
`(case (map $key value) result)`{.brix}.

### Text

`(all "start" ...rest "end")`{.brix} fits text that begins and ends with the literals written, and
names the text between them, which may be empty. The literals and one rest may come in any order, as in
`(all ...name ".txt")`{.brix}, and the pattern fits the whole text. A literal can be named too, as
in `(: f "foo")`{.brix}.

```brix
(main
  (for (in file (list "notes.txt" "photo.png" "README"))
    (print (call kind file))
  )
)

(fn-expr + kind String
  (param file String)
  (match file
    (case (all ...name ".txt")
      (str "text named " name)
    )
    (case (all "photo." ...format)
      (str "a photo in " format)
    )
    (default
      (inline (str file " is something else"))
    )
  )
)
```

```output
text named notes
a photo in png
README is something else
```

Text patterns that read pieces, repeat, or choose between alternatives are grammars; see
Grammars.

### Properties

`..name` fits a value with a property `name`, and names a local `name` holding the property's
value. A test can follow it: `(all ..length ({} (> it 3)))`{.brix}. A property the type does not
have is an error: `Pattern has no possible matches`.

- `(let ..length "hello")`{.brix} declares `length`, holding `5`.
- `(let (: word ..length) "hello")`{.brix} names the whole value `word`, and declares no `length`.
- In a `match`, a property pattern can only be the only arm.

### Patterns inside patterns

Each part of a list, map or tuple pattern is a pattern, so one pattern can reach through several
layers of a value, such as decoded JSON, and test each layer on the way. With `match?`, a missing
key, a list that is too short or a value of the wrong type all give `null`, so a caller can pick
out the one part it needs without a test for each layer.

```brix
(import /data/json)

(main
  (let replies
    (list
      "{\"choices\": [{\"message\": \"hi\", \"index\": 0}]}"
      "{\"choices\": [{\"index\": 0}]}"
      "{\"choices\": []}"
      "{\"error\": \"busy\"}"
    )
  )
  (for (in text replies)
    (print (?? (call first-message (call JSON text)) "no message"))
  )
)

(fn-expr + first-message (? String)
  (param response JSON)
  (match? response
    (case (list (= "choices" (list (list (= "message" (: message String))) ...)))
      message
    )
  )
)
```

```output
hi
no message
no message
no message
```

## Patterns outside `match`

```brix
(main
  (let pair (tuple 3 "three"))
  (if (= (| (tuple n word)) pair)
    (print (str n " is " word))
  )
  (let maybe (? Int) 42)
  (if (= (| (all (: n Int) ({} (> it 40)))) maybe)
    (print (str "big " n))
  )
  (print (is (list 1 2 3) (list 1 ...)))
  (print (case (= (all Int ({} (> it 40))) maybe)))
  (print (case (= (all Int ({} (> it 50))) maybe)))
  (print
    (match? 7
      (case ({} (> it 50))
        "huge"
      )
    )
  )
  (for (in (| (: n Int)) (list 1 null 3))
    (print (str "got " n))
  )
)
```

```output
3 is three
big 42
true
42
unknown
unknown
got 1
got 3
```

| Code | Gives |
| --- | --- |
| `(if (= (\| pattern) value) body)`{.brix} | Runs the body where the value fits, with the pattern's names. |
| `(if condition body (else-if (= (\| pattern) value) body))`{.brix} | The same, as a branch of an `if`. |
| `(while (= (\| pattern) value) body)`{.brix} | Runs the body for as long as the value fits; see Control flow. |
| `(for (in (\| pattern) source) body)`{.brix} | Runs the body for each item that fits. |
| `(is value pattern)`{.brix} | `true` where the value fits. |
| `(case (= pattern value))`{.brix} | What the pattern passes on, or `null` where the value does not fit. |
| `(= (select result (\| pattern)) value)`{.brix} | `result` where the value fits, or `null`. |
| `(let pattern value)`{.brix} | Declares the pattern's names; see below. |

- **Fit, not truth.** `(if (= (| (: n Int)) count) body)`{.brix} runs its body whenever `count`
  holds an `Int`, even `0`. The test is whether the value fits.
- **`is`.** The right side of `is` is a type, a tuple type, as in
  `(is pair (tuple Int String))`{.brix}, or a list, map or text pattern, or `_`.
  `(is-not value pattern)`{.brix} is its opposite. See Types for `is` with types.
- **Never fits.** A `match` none of whose arms can ever fit its subject, and an `if`, `while` or
  declaration whose pattern can never fit its value, is an error: `Pattern has no possible
  matches`.

### Where a value does not fit

| Code | Where no pattern fits |
| --- | --- |
| `match` | Panics, `Pattern match failed`, or, on a type with a fixed set of values, is an error unless every value has an arm |
| `match?` | Gives `null` |
| `match?` run as steps | Goes on to the next step |
| `if`, `else-if` with a pattern | Runs the `else`, if any |
| `while` with a pattern | Ends the loop |
| `for` with a pattern | Skips the item |
| `is` | Gives `false` |
| `(case (= pattern value))`{.brix} and `select` | Give `null` |
| A declaration with a pattern | Panics, `Pattern match failed` |

## Taking values apart

A declaration whose name is a pattern takes its value apart, which other languages call
destructuring, declaring a local for each name the pattern gives. The pattern states what the
value is sure to be. Where a value may not fit as a matter of course, use `match?` or an `if`
with a pattern instead.

```brix
(main
  (let pair (tuple 3 "three"))
  (let (tuple first second) pair)
  (print (str first ", " second))
  (let (tuple x (tuple y z)) (tuple 1 (tuple 2 3)))
  (print (+ x y z))
  (let (tuple _ label) pair)
  (print label)
  (let (list head ...tail) (list 10 20 30))
  (print (str head " then " tail))
  (let a 1)
  (let b 2)
  (= (tuple a b) (tuple b a))
  (print (str a " " b))
)
```

```output
3, three
6
three
10 then 20, 30
2 1
```

- **Tuples.** `(let (tuple a b) pair)`{.brix} declares a local for each field, by position: names
  are matched to fields by place, not by the fields' names. Fields can be nested,
  `(let (tuple x (tuple y z)) value)`{.brix}, skipped with `_`, or given types,
  `(let (tuple (: a Int) (: b String)) value)`{.brix}, which convert them. Fewer names than fields
  take the first fields. More names than fields is an error: `Pattern has no possible matches`.
- **Existing locals.** `(= (tuple a b) pair)`{.brix} assigns the fields to locals that exist, so `(=
  (tuple a b) (tuple b a))`{.brix} swaps two locals. Only a tuple is assigned this way: `(= (list x
  y) items)`{.brix} is an error, `Names in a list pattern can be declared from a list, as in "(let
  (list x y) items)", but not assigned again; assign a tuple to them, as in "(= (tuple x y) (tuple
  ([] items 0) ([] items 1)))"`.
- **Optionals.** A tuple taken apart from an optional takes apart the tuple it holds, and panics
  where it holds nothing: `Optional'Pair is null`.
- **Patterns that can fail.** A declaration with a pattern that does not fit its value panics, at
  the pattern: `Pattern match failed`.
- **Lazy declarations.** `(lazy (tuple a b) value)`{.brix} declares lazy locals; the value is
  computed once, where any of them is first read. See Functions.
- **Loops.** A loop header takes each item apart the same way:
  `(for (in (tuple name age) ages) body)`{.brix}. See Control flow.

## Covering every case

A `match` over a value whose type has a fixed set of values must have an arm for each of them, or
a `(default value)`{.brix} arm, unless it is a `match?`. The fixed sets are:

| Subject | Covered by |
| --- | --- |
| `Bool` | `(case= true value)`{.brix} and `(case= false value)`{.brix}. |
| An optional | An arm for `null`, such as `(case= null value)`{.brix}, and arms that cover the value. |
| A union | An arm for each case: `.case`, a pattern of the case's type, or `_`. |
| An enum | `(case= (. Type value) result)`{.brix} for each value. |

```brix compile-error
(main
  (print (call describe 4))
)

(fn-expr + describe String
  (param value (? Int))
  (match value
    (case (: n Int)
      (str "holds " n)
    )
  )
)
```

```output
Exception: This match on I64? does not handle null; add "(case= null …)", or "(default …)" for the rest
<no file:9>
        (case (: n Int)
        ^^^^^^^^^^^^^^
```

- **Arms that do not count.** An arm with a condition, such as
  `(case (all (: n Int) ({} (> it 0))) value)`{.brix} or `(case? ready value)`{.brix}, does not
  cover what it tests.
- **Lists in cases.** Arms over a union case that holds a list cover it together when one takes
  some count of items and a rest, and the others take each smaller count:
  `(case (call .many (list)) value)`{.brix} and
  `(case (call .many (list first ...rest)) value)`{.brix} cover `.many`, as
  `(case (call .many (list ...items)) value)`{.brix} does alone.
- **Other types.** A `match` over numbers, text, lists or classes is not checked: where no arm fits
  a value, it panics at run time.
- **Messages.** Each names what is missing and how to add it: `This match on Bool does not handle
  false; add "(case= false …)", or "(default …)" for the rest`, `This match on Result does not
  handle .bad; add a case for it, or "(default …)" for the rest`, and `This match on I64? handles
  only null; add a case for a present value, as in "(case (: value I64) …)", or "(default …)" for
  the rest`.
