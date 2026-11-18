# Conversions

A value of one type becomes a value of another in two ways: on its own, where a place wants the
other type, or when asked, with `as`. The rule that divides them is that a conversion happens on
its own only where it keeps the value's meaning: where every value of one type has a value of the
other that means the same. Every other conversion is written.

This chapter covers both, calling a type to convert, the conversions a type declares, parameters
that take any value that converts, and the type an expression gets when its parts have different
types.

## Conversions on their own

```brix
(main
  (let small I32 5)
  (let wide Int small)
  (let real Float small)
  (print (str wide " " real))
  (let ints (list I32) (list 1 2 3))
  (let floats (list Float) ints)
  (print floats)
  (let pair (tuple I32 I32) (tuple 1 2))
  (let joined (tuple Int Float) pair)
  (print joined)
  (let maybe (? Int) wide)
  (print maybe)
)
```

```output
5 5
1, 2, 3
(1, 2)
5
```

A value converts on its own where it is assigned, declared with a type, passed as an argument,
returned, or added to a collection, when one of these holds:

- **Numbers.** A number becomes a number type that holds every value of its own: `I32` becomes
  `Int` and `Float`, `U8` becomes `Int` and `F32`, and `F32` becomes `Float`. A number that may
  not fit, or not exactly, needs `as`: `Int` becomes `Float` or `I32` only when asked. `Size` and
  `Index` are the same width as `Int`, and convert both ways, and so does a number type declared
  on another, such as `(type + Id (is Int))`{.brix}, with its base. `Int` is `I64`, `Float` is `F64` and `Byte` is
  `U8`, and messages name them so.
- **Classes.** An object becomes its base class, or an interface its class implements; see Types.
- **Functions.** A function becomes a function type where calling it through the type would
  convert each argument and its result on their own; see Functions.
- **Optionals and unions.** A value becomes an optional of its type, or a union with a case it
  converts to on its own. An optional becomes a union with a `Null` case and a case its value
  converts to. See Optionals, and Unions and enums.
- **Collections and tuples.** A list, map or other collection becomes one of another item type
  when each item would convert on its own, so `(list I32)`{.brix} becomes `(list Float)`{.brix}. A tuple becomes another
  tuple type, or a named tuple type, field by field.
- **Declared conversions.** A value becomes a type that declares a conversion from its type, or
  that its type declares a conversion to; see [Declared conversions](#declared-conversions).

Where none of these holds, a conversion the compiler knows of is an error that says to write it:

```brix compile-error
(main
  (let wide Int 5)
  (let small I32 wide)
)
```

```output
Exception: Expected I32 here, but this is I64, which becomes I32 only when asked; write "(as wide I32)"
<no file:4>
      (let small I32 wide)
                 ^^^
```

Where the compiler knows no conversion, the error says so: `Expected Point here, but this is
String, and there is no conversion between them`.

### Literals

A number written in the source takes the type the place wants, when it fits:
`(let byte U8 200)`{.brix} makes a `U8`. One that does not fit is an error: `Integer literal 300 does not fit U8`. The place can
be a declaration, a parameter, a list's other items, or the other operand of an operator, so in
`(list 0 4 10.5)`{.brix} every item is a `Float`. Without a place to decide, a whole number is an `Int` and
one with a point a `Float`. A suffix gives a literal its own type, as in `200u8`; see Literals.

### Text

A value becomes text on its own only inside a text, as in `(str count " items")`{.brix}, and where
it is printed. Elsewhere it is converted with `(as value String)`{.brix}: `Expected String here, but this is I64, which
becomes text only inside a text or a print, or when asked; write "as String", or write it inside
a text, as in "$name"`.

## `as`

`(as value Type)`{.brix} converts a value to a type, by any conversion the compiler knows.

```brix
(main
  (print (as 2.7 Int))
  (print (as -2.7 Int))
  (print (as 300 U8))
  (print (as -1 U8))
  (print (as 65 Char))
  (print (as 'B' Int))
  (print (as true Int))
  (print (as 0 Bool))
  (print (as "42" Int))
  (print (as "4.5" Float))
  (print (as 4.5 String))
  (print (+ (as 3 Float) 0.5))
)
```

```output
2
-2
44
255
A
66
1
false
42
4.5
4.5
3.5
```

Beyond the conversions that happen on their own, `as` converts:

- **Floats to integers**, dropping the fraction, so `2.7` and `-2.7` become `2` and `-2`.
- **Integers to narrower integers**, keeping the low bits, so `(as 300 U8)`{.brix} is `44` and
  `(as -1 U8)`{.brix} is `255`. A signed integer converted to a wider unsigned type is
  sign-extended, so `(as -128i8 U16)`{.brix} is `65408`. **Integers to floats**, to the nearest float.
- **Numbers, `Bool` and `Char`** to one another. A `Char` is its code point. `true` is `1`, and a
  number is `true` when it is not zero.
- **Text to numbers**, reading the whole text, white space aside, as a number. Text that is not
  one panics: `"42x" is not a whole number`, or for a float type, `"1.5x" is not a number`. A
  whole number too large for the type gives its largest value, so `(as "300" U8)`{.brix} is `255`.
  **Anything to text**, by its conversion to `String`.
- **Enums and numbers**, both ways; see Unions and enums.
- **Down a class hierarchy**, from a base class or an interface to a class the object is, panicking
  when it is not; see Types. **Out of a union**, to the type of the case it holds.
- **Optionals**, to the type of their value, panicking when they hold nothing; see Optionals.
- **Pointers**; see Native code and memory.

A conversion the compiler cannot make at all is an error, even with `as`: `Color cannot be cast to
I64; there is no conversion between them`. A value of a declared type that can never hold such a
value says so: `"n" is declared Int, but its value is String, which can never be Int; convert the
value, or declare "n" without a type`.

In rows, `as` binds more loosely than arithmetic and named operators such as `/and/`, so
`a + b as U32`{.rows} converts the sum, and `a + (b as U32)`{.rows} converts one operand. It binds
more tightly than `==` and assignment, and chains from left to right: `value as Far as U32`{.rows}.

### Calling a type

Calling a type with one value of another type converts it as `as` does: `(call Int 2.5)`{.brix}
is `2`, and `(call Char 66)`{.brix} is `B`.

```brix
(main
  (print (call Int 2.5))
  (print (call Char 66))
  (print (call Float 3))
  (print (call String 42))
)
```

```output
2
B
3
42
```

A tuple of one field is the exception: called with a value of its field's type, it is made from
the value rather than converted. Calling a class with arguments its constructors take calls a
constructor; see Types.

## Declared conversions

A type can declare how its values become another type, with `(fn-to-expr + Type body)`{.brix}, and
how values of another type become its own, with `(fn-from-expr + (: name Type) body)`{.brix}, which
names the value it converts. Each has a value body, or a step body when declared with `fn-to-do`
or `fn-from-do`.

```brix
(main
  (let m (call Meters 3))
  (let f Float m)
  (print f)
  (print (call half m))
  (let c (call Centimeters m))
  (print (. c amount))
  (let cms (list Centimeters) (list m (call Meters 0.5)))
  (print (. ([] cms 1) amount))
  (print (str "it is " m))
)

(type + Meters (is (tuple (: amount Float)))
  (fn-to-expr + Float
    amount
  )
  (fn-to-expr + String
    (str amount " m")
  )
)

(type + Centimeters (is (tuple (: amount Float)))
  (fn-from-expr + (: m Meters)
    (call Centimeters (* (. m amount) 100))
  )
)

(fn-expr + half Float
  (param value Float)
  (/ value 2)
)
```

```output
3
1.5
300
50
it is 3 m
```

- **On their own.** A declared conversion is used on its own, wherever a value of one type is
  given where the other is wanted, as `m` is for `half` and for the list of `Centimeters`. `as`
  and calling the type use it too. A conversion to `String` is the exception; see below.
- **One step.** Conversions do not chain, even with `as`: a value converts by one declared
  conversion, not by one and then another. A type that converts to `Centi`, which converts to
  `Kilo`, does not convert to `Kilo`, and one that converts to `I32` does not convert to `Int`.
- **`from` before `to`.** When one type declares a conversion from a second, and the second a
  conversion to the first, the `from` is used.
- **Derived classes and interfaces.** A class uses its base class's conversions, and declares its
  own to replace them, which are used even when the object is held as its base. An interface can
  declare conversions with bodies for the types that implement it.
- **Text.** A conversion to `String` is how the type becomes text: where it is printed, inside a
  text, and with `(as value String)`{.brix}, and nowhere else on its own. A type without one becomes text in its
  own way: see Types.
- **Step bodies.** `(fn-to-do + (of Iterable Item) body)`{.brix} can give a stream, yielding its
  items; see Streams.
- **Errors.** A `from` that can raise an error raises it wherever it converts: in a declaration,
  an argument, an assignment, a return, `as` or a call of the type.
- **Public.** A conversion cannot be private: `Converters must be public`.
- **Extensions.** An extension can declare conversions for a type declared elsewhere; see Types.

```brix
(main
  (let c Celsius (call Fahrenheit 212.0))
  (print (. c degrees))
  (print (. (as (call Fahrenheit 50.0) Celsius) degrees))
)

(type + Fahrenheit (is (tuple (: degrees Float))))

(type + Celsius (is (tuple (: degrees Float)))
  (fn-from-do + (: f Fahrenheit)
    (let above (- (. f degrees) 32))
    (print (str "converting " (. f degrees)))
    (return (call Celsius (/ (* above 5) 9)))
  )
)
```

```output
converting 212
100
converting 50
10
```

`fn-from-do` is a conversion from another type with a step body, which returns with
`(return value)`{.brix}. In rows it is `+ do from f Fahrenheit`{.rows}, under a thin divider.

## Parameters that take what converts

A parameter whose type is `(similar Float)`{.brix}, as in `(param value (similar Float))`{.brix},
takes any value that converts to `Float` with `as`, and converts it. Text is the exception: `as`
reads a number from text, but `similar` does not take text for a number. `is~` tests the same:
`(is~ 3 Float)`{.brix} is true, and `(is~ "4" Float)`{.brix} false, and `(is~-not value Type)`{.brix}
is the opposite. A pattern such as
`(: amount (similar Float))`{.brix} takes what converts too; see Patterns.

```brix
(main
  (print (call describe 3))
  (print (call describe (as 2 U8)))
  (print (is~ 3 Float))
  (print (is~ "x" Float))
)

(fn-expr + describe String
  (param value (similar Float))
  (str "value " value)
)
```

```output
value 3
value 2
true
false
```

## Joins

An expression whose parts may have different types, such as an `if` with a value on each branch,
a list literal, `??`, or a function with several returns, has a type that every part converts to
on its own:

- **Numbers** join to the one that holds the other: an `I32` and an `Int` join to `Int`. A
  number literal takes the type of the other parts, so `(if-expr exact small 7)`{.brix}, with
  `small` an `I32`, is an `I32`.
- **Classes** join to their nearest common base class, and a class and an interface it implements
  join to the interface.
- **`null` and a value** join to the value's optional.
- **Tuples** join field by field, so `(list (tuple 2.5 1) (tuple 1 2.5))`{.brix} has the type
  `(list (tuple Float Float))`{.brix}.
- **No common type** is an error: `This list mixes I64 and String, which have no common type;
  convert the items to one type`. For the sides of an `if` it is `The two sides of this "if" are
  I32 and F32, which have no common type; convert them to one type`, and for returns `Function
  "measure" returns Box here and F64 elsewhere, which have no common type; return one type, or
  declare the return type`.

A declared type decides instead, where there is one: `(let ratio Float (if-expr exact 1 0.5))`{.brix}.
