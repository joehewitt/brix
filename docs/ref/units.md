# Units

A unit is a name written straight after a number, as in `16ms`, `90°` or `25%`. It makes the
number a value of the unit's type: `16ms` is a `Milliseconds`. Units of one family, such as the
units of time, convert into one another by the ratios their declarations state, and arithmetic
between them converts first.

A unit is declared as a number type with an alias. The standard library declares units for
durations, angles, ratios, distances and more, and a program declares its own the same way.

## Unit literals

```brix
(import /time/duration)

(main
  (let frame 16ms)
  (let pause 1.5s)
  (let back -250ms)
  (print frame)
  (print (as frame UnitString))
  (print (as pause UnitString))
  (print (as back UnitString))
  (print (str "frame lasts " frame))
  (print (str "frame lasts " (as frame UnitString)))
  (print (as 200u8 UnitString))
)
```

```output
16
16ms
1.5seconds
-250ms
frame lasts 16
frame lasts 16ms
200u8
```

- **Spelling.** The unit follows the number with no space between them. Its name is lowercase
  letters and digits, as in `ms` and `km`; a currency symbol, such as `$` or `€`; or `%`, `µ` or
  `°`, followed by more of these characters, as in `%%` and `µs`. In rows, `5 km`{.rows} is a
  syntax error.
- **Numbers.** The number is an integer or a float, with a sign, an exponent or digit separators
  as numbers take them: `-250ms`, `2.5e3ms`. An integer fills a unit stored as a float. A hex number
  takes no unit.
- **Fitting.** A unit stored as an integer takes no fraction: `M holds whole numbers, as I32 does,
  so 3.7m cannot be written; write a whole number`. A literal of a unit the program declares must
  fit its storage: `Integer literal 300 does not fit Tk`.
- **Type.** A literal is a value of the type whose alias its unit is. That type must be in scope:
  a unit declared in a module this file does not import is an error, `Unit "ms" not found; it is
  declared in /time/duration, which this module does not import. Add the import /time/duration`,
  and a unit nothing declares is `Unit "zz" not found`.
- **Number suffixes.** The suffixes of the number types, such as `u8`, `i32` and `f`, are units of
  the same kind: `200u8` is a `U8`. They keep the bits that fit, so `300u8` is 44 and `128i8` is
  -128.
- **Methods** are called on a literal as on any value: `(call (. 0.25s wait))`{.brix}.
- **Constructors.** When the unit's type declares a constructor, a literal is what the constructor
  gives. `3pm`, from `/time/datetime`, is a `Time`. A type that is no number and has no such
  constructor takes no literals: `Name is no number and declares no constructor that takes one, so
  no number can be written in its unit; declare Name as a number type`.

### Printing

A unit value prints as its number alone, in a print and inside a text. `(as value UnitString)`{.brix}
gives the number followed by its unit, as text. The unit printed is the type's last alias:
`Seconds` has the aliases `s` and `seconds`, so `1.5s` prints as `1.5seconds`. A number with no
unit of its own prints with its type's suffix, as `200u8` does. `(. value unit)`{.brix} gives the
unit alone: `(. 3s unit)`{.brix} is `seconds`, and `(. 4 unit)`{.brix} is `i`.

## Declaring units

A unit is a number type with an alias, and a unit of a family is declared from another unit of
that family.

```brix
(main
  (let price 250cents)
  (print (as price Dollars UnitString))
  (print (as 3dollars Cents UnitString))
  (print (as (+ 1dollars 250cents) UnitString))
  (print (as (+ 250cents 1dollars) UnitString))
  (print (call (. 3dollars label)))
)

(type + Money (is I64)
  (fn-expr + label String
    (str this " " unit)
  )
)

(type-line + Cents (alias cents) (is Money))
(type-line + Dollars (alias dollars) (is 100cents))
```

```output
2dollars
300cents
3dollars
350cents
3 dollars
```

| Declaration | Declares |
| --- | --- |
| `(type-line + Grams (alias g) (is F64))`{.brix} | A unit stored as an `F64`. |
| `(type-line + Kilograms (alias kg) (is 1000g))`{.brix} | A unit equal to 1000 of another. It derives from that unit's type and joins its family. |
| `(type-line + Cents (alias cents) (is Money))`{.brix} | A unit of the family whose base type is `Money`. |
| `(unit + 1inches 25.4mm)`{.brix} | An equality between two units, which joins their families. |

- **Aliases.** A unit's type may have several aliases, and each is a unit. The last is the one
  `UnitString` prints. A suffix names one unit, so a unit cannot take one that a number type, an
  imported unit or another unit of its module already has: `"c" already names Char; give C a suffix
  of its own`.
- **Storage.** A unit is stored as the number type its chain of declarations starts from. A unit
  stored as an integer drops the fraction when it converts, toward zero: `250cents` is `2dollars`.
- **Family base.** A unit declared from a number type of the program's own, as `Cents` is from
  `Money`, equals one of it. The methods of the base type belong to every unit of the family, and
  run on the value in its own unit, so `(call (. 3dollars label))`{.brix} reads 3. Inside them,
  `unit` is the receiver's unit, as text.
- **The base holds the first unit.** A value of the base type is in the unit declared from it:
  `Duration` holds nanoseconds, so `1500ms` stored as a `Duration` is 1500000000.
- **Ratios.** The number in `is` is a literal with a unit, such as `100cents` or `2.54cm`. A
  negative or computed ratio is written as an equality. A sign in `is` is an error: `A unit is
  declared from an amount of another unit with no sign, as in "(type-line + Bar (alias bar) (is
  1foo))"; to relate two units by a negative amount, declare Bar from a number type and write an
  equality, as in "(unit + 1bar -1foo)"`.
- **No type parameters.** A unit's type takes none: `Unit types must be numeric and unqualified`.
- **Extending a family.** A module adds units to a family another module declares, as in
  `(type-line + Fortnights (alias fortnights) (is 14days))`{.brix} in a module that imports
  `/time/duration`.

### Equalities

An equality states that an amount of one unit equals an amount of another. It joins two families,
so a program can relate units declared apart, or units from two modules.

```brix
(main
  (print (as 1inches Mm UnitString))
  (print (as 1inches Cm UnitString))
  (print (as (+ 1m 1ft) UnitString))
  (print (> 1m 3ft))
)

(type-line + Mm (alias mm) (is F64))
(type-line + Cm (alias cm) (is 10mm))
(type-line + M (alias m) (is 100cm))
(type-line + Inches (alias inches) (is F64))
(type-line + Ft (alias ft) (is 12inches))
(unit + 1inches 25.4mm)
```

```output
25.4mm
2.54cm
1.3048m
true
```

- **Sides.** Each side is a constant: a number with a unit, or numbers joined by arithmetic and
  bitwise operators, as in `(unit + 1third (/ 1 3whole))`{.brix}. Each side has one unit.
- **Errors.** An equality must be public: `Equalities must be public`. A side with no unit is
  `Ambiguous units`, a side with two is `Conflicting units`, and a side that is not a constant is
  `Invalid constant expression`. A side's unit must be declared, `Unit "zz" not found`, and must not
  be a number type's suffix: `An equality relates units a program declares, and F64 is a number
  type of the language; write each side in a unit the program declares`. Neither side can be zero:
  `A unit cannot be declared as zero of another, which no number of it would convert from; write a
  number other than zero`.
- **Agreeing.** Two equalities between the same two units must state the same ratio: `Equalities
  between Bar and Foo disagree: one makes 1 Bar 2 Foo, another 3 Foo; keep one of them`. Only
  equalities between the same pair are checked. One that disagrees with a path through other
  units is not reported, and is not used, so keep every path between two units in agreement.
- **Across modules.** `/time/flicks` declares `(unit + 705600000flicks 1seconds)`{.brix}, so flicks
  convert to and from every duration.
- **Only ratios.** An equality states a ratio. Units with an offset between them, such as degrees
  Celsius and Fahrenheit, cannot be declared as one family: `An equality relates two units by a
  ratio alone, so a number with no unit cannot be added to or taken from one; write the ratio as one
  number in the unit`.

## Arithmetic

```brix
(import /time/duration)

(main
  (print (as (+ 8hours 30minutes) UnitString))
  (print (as (+ 30minutes 8hours) UnitString))
  (print (as (- 2s 250ms) UnitString))
  (print (as (* 90s 2) UnitString))
  (print (as (* 2 90s) UnitString))
  (print (as (+ 90s 30) UnitString))
  (print (as (/ 2hours 30minutes) UnitString))
  (print (< 500ms 1s))
  (print (== 60s 1minutes))
)
```

```output
8.5hours
510minutes
1.75seconds
180seconds
180seconds
120seconds
4hours
true
true
```

- **The left unit.** For `+`, `-`, `*`, `/` and `%`, the right operand converts to the left
  operand's unit, and the result is in the left's unit.
- **No compound units.** Multiplying or dividing two units does not make a new unit and does not
  cancel them: the right operand converts as for `+`, so `(/ 2hours 30minutes)`{.brix} is `4hours`.
  Units are not dimensional analysis. A quantity such as a speed is a unit of its own, made by a
  function that takes the two units, as in `(fn-expr + per Speed (param distance Length) (param
  time Seconds) body)`.
- **Two families.** `+`, `-`, `*`, `/` and `%` between units of two families that no equality
  joins give a plain number: with `/units` and `/time/duration`, `(+ 100meters 20s)` is `120`.
  Only comparisons between them are errors.
- **Plain numbers.** A plain number on the right is taken in the left's unit:
  `(+ 90s 30)`{.brix} is `120seconds`. A plain number on the left keeps the unit only for `*`; with
  the other operators the result is a plain number.
- **Comparisons** convert the same way, so `(< 500ms 1s)`{.brix} is true. Comparing units of two
  families that no equality joins is an error; see [Conversions](#conversions).
- **Assignment.** `(+= total 30s)`{.brix} converts the value to the unit of `total`.
- **Units on their own.** A unit that no declaration relates to another, such as `px`, is in no
  family. Arithmetic on it gives a plain number, which becomes the unit again when it is stored as
  one: `(let width Coord (+ 3px 2px))`{.brix} is `5px`.
- **Generic functions** keep the unit of what they are given. A function that takes a plain number
  type, such as `abs` from `/math`, takes the value in its own unit and gives a plain number.

## Conversions

```brix
(import /time/duration)

(main
  (let timeout Seconds 1500ms)
  (print (as timeout UnitString))
  (print (call describe 2minutes))
  (print (call describe 45))
  (print (as (call Milliseconds 2.5s) UnitString))
  (print (as 2880minutes Days UnitString))
  (print (as timeout F64))
  (print (as 1500ms Int))
)

(fn-expr + describe String
  (param wait Seconds)
  (str "waits " (as wait UnitString))
)
```

```output
1.5seconds
waits 120seconds
waits 45seconds
2500ms
2days
1.5
1500
```

- **Within a family**, a unit converts to another on its own wherever a value of the other is
  wanted: in a declaration, an argument, a return or an assignment. `as` and calling the type, as
  in `(call Milliseconds 2.5s)`{.brix}, convert the same way.
- **Ratios** multiply along the declarations as exact fractions, so `(as 2880minutes Days)`{.brix}
  is exactly `2days`. A ratio written as a decimal, such as `25.4mm`, is a float like any other.
- **Integers.** A unit stored as an integer drops the fraction, toward zero.
- **Plain numbers to units.** A plain number given for a unit is taken in that unit:
  `(call describe 45)`{.brix} waits `45seconds`. A number literal becomes any unit. A number that is
  not a literal converts as numbers do: a `Float` becomes a unit stored as an `F64`, but an `Int`
  becomes one only with `as`.
- **Units to plain numbers.** A unit becomes a plain number by its value in its own unit, without
  converting: `(as 1500ms Int)`{.brix} is 1500. It does so on its own where the number type holds
  every value of the unit's storage, and with `as` otherwise.
- **Other families.** Units of two families that no equality joins do not convert:

```brix compile-error
(import /time/duration)

(main
  (print (as 3dollars Seconds))
)

(type-line + Cents (alias cents) (is I64))
(type-line + Dollars (alias dollars) (is 100cents))
```

```output
Exception: Dollars cannot be cast to Seconds; there is no conversion between them
<no file:5>
      (print (as 3dollars Seconds))
             ^^^^^^^^^^^^^^^^^^^^^
```

## Units in the standard library

| Module | Units |
| --- | --- |
| `/time/duration` | `ns`, `µs`, `ms`, `s` or `seconds`, `minutes`, `hours`, `days`, `weeks`, `months` (31 days), `years` (365 days), `decades`, `centuries` and `milleniums`, on the base `Duration`. |
| `/time/flicks` | `flicks`, joined to the durations. |
| `/time/datetime` | `second`, `minute` and `hour` of a day, and `am` and `pm`, whose literals are a `Time`: `3pm`. |
| `/math` | `rad`, and `°` or `deg`. `π` is in radians. |
| `/units` | `%%` (`Ratio`) and `%` (`Percent`), and the distances of `/units/distance`. |
| `/units/distance` | `nm`, `µm`, `mm`, `cm`, `meters`, `km`, `inches`, `ft`, `yards` and `miles`, on the base `Distance`. |
| `/graphics/primitives` | `px`, a `Coord`, in no family. |
| `/tui/primitives` | `tx`, a `Coord` stored as an `I16`, in no family. |
| `/ui/virtual` | `vcoord`, in no family. |
| `/media/synth` | `hz` and `khz`; `st` and `oct`. |

- **Ratios.** `%%` is a `Ratio`, in which `1%%` is the whole, and `%` is a `Percent`, in which
  `100%` is the whole: `(as 25% Ratio)`{.brix} is `0.25%%`. Write a fraction with its leading
  zero: `0.75%%` is three quarters, and `75%%` is seventy-five wholes.
- **Scales.** A type that implements `Scalable` gives a range, a `Scale`, onto which a ratio
  converts: `Hour` gives 0 to 23, so `(as 0.75%% Hour)`{.brix} is `18hour`.
- **Durations** have methods: `(call (. 0.25s wait))`{.brix} waits, and `interval` makes a timer.
  Timeouts, waits and timers in the standard library take durations.
- **Styles** use durations for motion, as in `200ms`, and percentages for keyframe stops and
  sizes. See Styles.
