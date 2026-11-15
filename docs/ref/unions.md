# Unions and enums

A union is a value that is one of several cases. Each case has a name, a type, or both, and may
carry a value of its own, its payload. An enum is a type whose values are a fixed set of named
values of one representation, such as numbers or tuples.

This chapter covers declaring unions, making and reading their values, unions of types, members,
exhaustiveness, anonymous cases and enums. Matching in general is in Patterns, and optionals in
Optionals.

## Declaring a union

```brix
(main
  (let downloads
    (list Download)
    (list
      (. Download waiting)
      (call (. Download progress) 300 1200)
      (call (. Download complete) "report.pdf")
    )
  )
  (for (in download downloads)
    (print (call describe download))
  )
  (let done (call (. Download complete) "notes.txt"))
  (print (is done .complete))
  (print (is done .waiting))
  (print done)
  (print (. Download waiting))
  (print (== done (call (. Download complete) "notes.txt")))
)

(type + Download (is Union)
  (variant waiting)
  (variant progress (tuple (: received Int) (: total Int)))
  (variant complete String)
)

(fn-expr + describe String
  (param download Download)
  (match download
    (case .waiting
      "waiting"
    )
    (case (call .progress received total)
      (str received " of " total " bytes")
    )
    (case (call .complete path)
      (str "saved to " path)
    )
  )
)
```

```output
waiting
300 of 1200 bytes
saved to report.pdf
true
false
notes.txt
waiting
true
```

A union is a type built on `Union` that lists its cases. A case has a name or a type, and may have
a payload or a fixed value:

| Declaration | Case |
| --- | --- |
| `(type + Download (is Union) (variant waiting))`{.brix} | A name and no payload. |
| `(type + Download (is Union) (variant complete String))`{.brix} | A name and a payload of one type. |
| `(type + Download (is Union) (variant progress (tuple (: received Int) (: total Int))))`{.brix} | A name and a payload of several fields. |
| `(type + Value (is Union) (variant Int))`{.brix} | A type, with no name; see [Unions of types](#unions-of-types). |
| `(type + Status (is Union) (variant failed (= 1)))`{.brix} | A name and a fixed value. |

The kinds of case can be mixed in one union. A union may take type parameters, as in
`(type + (of Maybe Item) (is Union) (variant some (tuple Item)) (variant none))`{.brix}, and may
hold itself through a list or an object, as in
`(type + Tree (is Union) (variant branch (list Tree)))`{.brix}.

## Making a union value

- **By case.** `(call (. Download complete) "report.pdf")`{.brix} makes the `complete` case with
  its payload. A case whose payload has several fields takes the fields as arguments, as
  `(call (. Download progress) 300 1200)`{.brix} does. A case with no payload is named without a
  call: `(. Download waiting)`{.brix}.
- **By the case's name alone.** Where the union's type is known, as for a parameter, a declared
  local, a returned value, an argument of `==` or the other items of a list, `.waiting` names the
  case without the type. When more than one visible type has a case of that name and the type is
  not known, the name is an error: `Case '.waiting' is ambiguous between 'Download', 'Upload'`.
- **By conversion.** A value of a case's type converts to the union on its own: a local declared
  `Value` can be given `3` when `Value` has an `Int` case. A value that converts to a case's type
  only with `as` must be written with `as`.
- **By calling the union.** `(call Value 3)`{.brix} makes the case whose type the value has.

## Reading a union

```brix
(main
  (let a (call (. Status failed) "disk"))
  (print (. a name))
  (print (. a case))
  (print (== a (call (. Status failed) "disk")))
  (print (== a .done))
  (print (call label .pending))
  (print (call label a))
)

(type + Status (is Union)
  (variant pending)
  (variant done)
  (variant failed String)
)

(fn-expr + label (? String)
  (param status Status)
  (match? status
    (case .pending "pending")
    (case .done "done")
  )
)
```

```output
failed
2
true
false
pending
unknown
```

- **`match`.** An arm `(case (call .complete path) value)`{.brix} is taken when the union holds
  that case, and binds its payload. A payload of several fields binds each, as in
  `(case (call .progress received total) value)`{.brix}. An arm for a case with no payload is
  `(case .waiting value)`{.brix}. See Patterns for every arm a match can have.
- **`is`.** `(is done .complete)`{.brix} is true when the union holds that case. After the test, the
  compiler treats the value as that case; see [Narrowing](#narrowing).
- **`case` and `name`.** `(. value case)`{.brix} is the number of the case the union holds,
  counting from 0 in the order the cases are declared. `(. value name)`{.brix} is the case's name,
  and is empty for a case that is a type.
  A union may not declare members with these names, or `type`: `Union property 'name' conflicts
  with reserved union property`.
- **`as`.** `(as value String)`{.brix} gives the payload as that type when the union holds a case
  that converts to it, and raises an error otherwise, which becomes the text `Invalid union cast`.
- **Equality.** Two values of a union are equal when they hold the same case and equal payloads.
  A value compared with a union converts to the union first.
- **Text.** A union becomes the text of its payload, or its case's name when it has none, as
  `done` and `(. Download waiting)`{.brix} print above. A union that declares a conversion to
  `String` becomes that text instead.

### Narrowing

A test that settles which case a union holds lets the code it guards read that case's payload.

```brix
(main
  (let event (call (. Event click) 10 20))
  (if (is event .click)
    (print (str "click at " (. event x) ", " (. event y)))
  )
  (for (in fill (list .inherit .none (call (. Fill color) "blue")))
    (print (call describe fill))
  )
)

(type + Event (is Union)
  (variant click (tuple (: x Int) (: y Int)))
  (variant key String)
)

(type + Fill (is Union)
  (variant inherit)
  (variant none)
  (variant color String)
)

(fn-expr + describe String
  (param fill Fill)
  (case? (== fill .inherit)
    "inherited"
  )
  (case? (== fill .none)
    "no paint"
  )
  (default
    (str "paint " (. fill uppercased))
  )
)
```

```output
click at 10, 20
inherited
no paint
paint BLUE
```

- **`is`** narrows to the case it names, so `(. event x)`{.brix} reads the click's field.
- **`==` and `!=`** against a case with no payload narrow as `is` does, in `if` and `else-if`
  chains, guards and decision tables, and so does `is-not`.
- **Ruling cases out.** Where every case but one has been ruled out, the value is narrowed to the
  last: the default row of `describe` reads the `color` case's text.
- **Payloads do not narrow.** A comparison with a case that holds a value, as
  `(== fill (call (. Fill color) "blue"))`{.brix}, is an ordinary comparison and narrows nothing.

## Unions of types

A case may be a type alone, with no name. A value of that type converts to the union on its own,
and an arm that names the type matches it. A list written where a list of the union is wanted, as
`values` is below, converts each item.

```brix
(main
  (let values (list Value) (list 3 "three" 4.5 null))
  (for (in value values)
    (print (str (call describe value) ", case " (. value case)))
  )
  (let held Value 7)
  (print (is held Int))
  (print (as held String))
  (print (. held type))
)

(type + Value (is Union)
  (variant Null)
  (variant Int)
  (variant String)
  (variant Float)
)

(fn-expr + describe String
  (param value Value)
  (match value
    (case (: n Int)
      (str "number " n)
    )
    (case (: s String)
      (str "text " s)
    )
    (case (: f Float)
      (str "float " f)
    )
    (case Null
      "nothing"
    )
  )
)
```

```output
number 3, case 1
text three, case 2
float 4.5, case 3
nothing, case 0
true
7
Class:core/number:I64
```

- **Which case.** A value goes into the case whose type it has. When it has none of them, it goes
  into the first case it converts to on its own, before one it converts to only with `as`.
- **`Null`.** A `Null` case holds `null`, and an optional converts to a union with a `Null` case:
  an absent value to `Null`, and a present one to its case. A local of a union whose first case
  is `Null` starts as `null` when it is declared with no value.
- **`type`.** `(. value type)`{.brix} describes the type of the value the union holds.
- **Arms.** An arm `(case (: n Int) value)`{.brix} is taken when the union holds an `Int`, and
  binds it. `(is value Int)`{.brix} tests for it.

Named cases and type cases can be mixed. Named cases tell apart payloads of one type, such as two
cases that each hold a `String`; type cases need no names when the types differ:

```brix
(main
  (for (in move (list (call (. Move vertical) 12) (call (. Move horizontal) 5)))
    (print (call describe move))
  )
  (print (== (call (. Move vertical) 12) (call (. Move horizontal) 12)))
)

(type + Move (is Union)
  (variant horizontal Int)
  (variant vertical Int)
)

(fn-expr + describe String
  (param move Move)
  (match move
    (case (call .horizontal distance)
      (str "across " distance)
    )
    (case (call (. Move vertical) distance)
      (str "down " distance)
    )
  )
)
```

```output
down 12
across 5
false
```

## Members of a union

A union can declare methods, getters, conversions and operators, and `this` is the union. A
member the union does not declare is passed on to the value its case holds, when every case
holds a value that has it.

```brix
(main
  (let shapes (list Shape) (list (call Circle 1) (call Square 2)))
  (for (in shape shapes)
    (print (str (. shape label) ": " (call (. shape area))))
  )
)

(type + Shape (is Union)
  (variant Circle)
  (variant Square)
  (fn-expr + label String :get
    (match this
      (case Circle "circle")
      (case Square "square")
    )
  )
)

(type + Circle (is (tuple (: radius Float)))
  (fn-expr + area Float
    (* 3.0 radius radius)
  )
)

(type + Square (is (tuple (: side Float)))
  (fn-expr + area Float
    (* side side)
  )
)
```

```output
circle: 3
square: 4
```

- **The union's own members win** over the cases'.
- **Every case must have it.** A member some case lacks is an error, such as `Union case
  'empty' has no payload for method 'area'`, or `Union case 'right' has no complete dispatch for
  method 'area'`.
- **Results.** When the cases' members give different types, the result is a union of them.
- **No stored properties.** A union declares no stored properties of its own.

## Covering every case

A `match` over a union must have an arm for each case, or end with `(default value)`{.brix} for
the rest. A missing case is an error:

```brix compile-error
(main
  (print (call label .pending))
)

(type + Status (is Union)
  (variant pending)
  (variant done)
  (variant failed String)
)

(fn-expr + label String
  (param status Status)
  (match status
    (case .pending "pending")
    (case .done "done")
  )
)
```

```output
Exception: This match on Status does not handle .failed; add a case for it, or "??" for the rest
<no file:15>
        (case .pending "pending")
        ^^^^^^^^^^^^^^^^^^^^^^^^^
```

List every case when a new case should send each `match` back to be reconsidered, and end with a
`default` arm when the cases left over share one answer on purpose: a `default` also takes any case
added later, without a word.

With several missing, the message names each: `This match on Result does not handle Config,
FileNotFound and PermissionDenied; add a case for each, or "??" for the rest`. `match?` need not
cover every case, and gives `null` when no arm is taken, as `label` does in
[Reading a union](#reading-a-union). An arm for a type covers its derived classes.

## Anonymous cases

A function can give one of several cases without a declared union. A value written `..accepted`
or `(call ..accepted name)`{.brix} is a case of a union the compiler makes from every `..` case
that can be the result.

```brix
(main
  (print (call describe (call check "Yumi")))
  (print (call describe (call check "")))
  (let result (call check "x"))
  (print (is result .accepted))
  (print (. result name))
)

(fn-expr + check _
  (param name String)
  (case? (> (. name length) 0)
    (call ..accepted name)
  )
  (default
    ..empty
  )
)

(fn-expr + describe String
  (param result _)
  (match result
    (case (call .accepted name)
      (str "Hello, " name)
    )
    (case .empty
      "Enter a name"
    )
  )
)
```

```output
Hello, Yumi
Enter a name
true
accepted
```

- **Owners.** The cases belong to the value they are given to: a function's result, a local, or a
  generator's items. A `..` case where the result has no such owner is an error, `Anonymous match
  case '..ok' needs a stable result owner`.
- **Payloads.** Each use of a case must give it the same number of values, or it is an error:
  `Anonymous case '..value' has conflicting arity`. The types of the values join, so
  `(call ..value 1)`{.brix} and `(call ..value 1.5)`{.brix} make a payload of `Float`.
- **Reading.** Anonymous cases are read as named ones are: by match arms, `is`,
  `(. value name)`{.brix} and `(. value case)`{.brix}. Once the union exists, `.ok` names one of
  its cases.

## Enums

A type whose body lists cases, but which is built on a number, a tuple or a class rather than on
`Union`, is an enum. Each case is one value of the type, with a name.

| | Enum | Union |
| --- | --- | --- |
| A case is | One value, declared with the type | A kind of value, made by a call |
| Its data comes from | The declaration: every case has the same fields | The call: each case has its own payload |
| Use it for | Named values of one shape: priorities, protocol codes, days | Alternatives that need different data, or one type in different roles |

```brix
(main
  (let level Level .medium)
  (print level)
  (print (call describe level))
  (print (call describe (as 3 Level)))
  (print (as (. Level urgent) Int))
  (print (. Planet earth))
  (print (. Planet mars weight))
  (print (== level .medium))
)

(type + Level (is U8)
  (variant low)
  (variant medium)
  (variant high)
  (variant urgent (= 10))
)

(type + Planet (is (tuple (: name String) (: gravity Float)))
  (variant mercury (= (tuple "Mercury" 0.5)))
  (variant earth (= (tuple "Earth" 1.0)))
  (variant mars (= (tuple "Mars" 0.25)))
  (fn-expr + weight Float :get
    (* 80 gravity)
  )
  (fn-to-expr + String
    name
  )
)

(fn-expr + describe String
  (param level Level)
  (match level
    (case= .low
      "fine"
    )
    (case= .medium
      "watch"
    )
    (default
      "act"
    )
  )
)
```

```output
1
watch
act
10
Earth
20
true
```

- **Number enums.** In an enum built on a number type, each case is a number. A case without a
  value is its place among the cases, counting from 0, and a case can be given its own value, as
  `urgent` is in `(type + Level (is U8) (variant urgent (= 10)))`{.brix}. A number enum becomes
  text as its number.
- **Tuple and class enums.** In an enum built on a tuple type, each case's value fills the tuple's
  fields in order. In one built on a class, each case is one object of the class, made from its
  value by the class's constructor. Their cases are values of the type, with its members, and
  become text by its conversion to `String`.
- **Naming a case.** `(. Level medium)`{.brix}, or `.medium` where the type is known, as for a
  union.
- **Numbers.** `(as 3 Level)`{.brix} gives the value of a number enum that is that number. For a
  tuple or class enum, a number gives the case at that place, counting from 0, and a number past the
  last case gives a new value made with no arguments. A number becomes an enum only with `as`:
  `Expected Level here, but this is I64, which becomes Level only when asked; write "as Level"`.
- **Comparing.** `==` and `!=` compare enum values. The cases of a class enum are single objects,
  so a case equals itself.

### Matching enum values

An enum's cases are values, not union cases, so an arm matches one by equality, written
`(case= .low value)`{.brix}. An arm `(case .low value)`{.brix} on a number enum is an error,
`Level.low is an enum value, not a union case; match it with '|= .low'`, and an arm that is a name
alone binds the name: `Arm '| low' binds a new name, not the value Level.low; match the value with
'|= Level.low'`.

A match over a number enum must cover every case, or end with `(default value)`{.brix}: `This match
on Level does not handle Level.high; add "|= Level.high", or "??" for the rest`. A match over a
class enum is not checked for coverage, and fails when the program runs if no arm is taken.

### Enums of `Union`

A union whose cases have no payloads, such as
`(type + Color (is Union) (variant red) (variant green) (variant blue))`{.brix}, serves as an enum
whose values print as their names. Its arms are written `(case .red value)`{.brix}, as for any
union.
