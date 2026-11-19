# Types

A type declaration names a kind of value and declares its members: the properties each value
holds, the functions that work on it, and the constructors that make it. What the type is built
on decides what kind of type it is.

This chapter covers classes, tuples, structs and interfaces, and what they share: properties,
constructors, methods, static functions, type parameters, inheritance, delegation, extensions
and facets.
Unions are in Unions and enums, function types in Functions, and number types with units in
Units. Conversions a type declares are in Conversions, operators it defines in Operators, and
events and rules in Reactivity.

## Declaring a type

```brix
(main
  (let counter (call Counter "clicks"))
  (call (. counter add) 3)
  (call (. counter add))
  (print (. counter summary))
  (print (. (call (. Counter named) "taps") summary))
)

(type + Counter
  (prop + label String)
  (prop + count Int)
  (prop + summary String (=> (str label ": " count)))
  (fn-do + This _
    (param label String)
  )
  (fn-do + add _
    (param amount Int (= 1))
    (+= count amount)
  )
  (fn-expr + named Counter :static
    (param label String)
    (call Counter label)
  )
)
```

```output
clicks: 4
taps: 0
```

A type is declared at the top level of a file. Its header holds the access, `+` or `_`, the name,
and what the type is built on, as in `(type + Dog (is Animal))`{.brix}. After the header, the type
names each interface it implements, as in `(type + Square (implements Shape))`{.brix}, and each
rule its type parameters must meet, as in
`(type + (of Box Item) (constraint is Item Titled))`{.brix}. The members follow in order:
properties first, then functions. A type with no members is its header alone, and may be written
with `type-line` in place of `type`.

In rows, a type follows a thick divider, `━`{.rows}, and each of its functions a thin divider,
`─`{.rows}. A type with no members needs no divider.

The name is a capitalized word, such as `Counter` or `HttpRequest`. A type that takes type
parameters names them after the name, as in `(type + (of Box Item))`{.brix}; see
[Generic types](#generic-types). Other names for the type follow its name:
`(type + Point (alias Spot))`{.brix} lets `Spot` stand for `Point`. A lowercase name among them is
a unit suffix, and only a number type can have one; see Units.

### Kinds of type

| Declaration | Kind | Values |
| --- | --- | --- |
| `(type + Name)`{.brix}, or `(type + Name (is Base))`{.brix} with a class as base | A [class](#classes-and-objects) | Objects, shared by reference. |
| `(type + Name (is Tuple))`{.brix}, or `(type + Name (is (tuple (: x Int) (: y Int))))`{.brix} | A [named tuple](#tuples) | Fixed groups of values, copied, never changed in place. |
| `(type + Name (is Struct))`{.brix} | A [struct](#structs) | Fixed groups of values, copied, changed in place. |
| `(type + Name (is Interface))`{.brix} | An [interface](#interfaces) | Any value of a type that implements it. |
| `(type + Name (is Union))`{.brix} | A union; see Unions and enums | One of several cases. |
| `(type + Name (is Int String))`{.brix} | A [type of alternatives](#types-of-alternatives) | A value of any one of the types, which keeps its own type. |
| `(type + Name (is Int))`{.brix}, or another number type | A number type; see Units | Numbers of their own type. |
| `(type + Name (is (fn-type Int)))`{.brix} | A function type; see Functions | Functions. |

`Optional`, `Function`, `CFunction` and `Bool` cannot be a base: a type built on one is an error,
`Type has illegal base class`.

A tuple suits data that is nothing more than its values, compared and copied whole. A class suits
a thing with an identity and a lifetime, which everyone who holds it shares. A struct suits values
changed in place by their one holder. Inheritance suits a family that shares an identity and
behavior worth sharing, at the cost that each subclass depends on choices its base makes. An
interface suits a boundary where callers need only a small capability, and a union a fixed set of
cases each caller should consider.

### Members

| Declaration | Member |
| --- | --- |
| `(prop + name Type (= value))`{.brix} | A [stored property](#stored-properties). |
| `(prop + name Type (:= value))`{.brix} | A [lazy property](#lazy-properties). |
| `(prop + name Type (=> value))`{.brix} | A [computed property](#computed-properties). |
| `(fn-do + This _ (param name Type) body)`{.brix} | A [constructor](#constructors). |
| `(fn-expr + name Type body)`{.brix} | A [method](#methods). |
| `(fn-expr + name Type :get body)`{.brix} | A [getter](#getters-and-setters). |
| `(fn-do + name _ :set (param value Type) body)`{.brix} | A [setter](#getters-and-setters). |
| `(fn-expr + name Type :static body)`{.brix} | A [static function](#static-functions). |
| `(type + Name (implements Interface))`{.brix} | An [interface](#implementing-an-interface) the type implements. |
| `(type + Name (delegate Interface) (prop + name Type))`{.brix} | A [delegation](#delegation). |

A method, getter, setter or static function declared with `fn-do` has a step body, and one
declared with `fn-expr` has a value body, as a function does; see Functions.

### Access

A member written with `+` is public: any module that can see the type can use it. A member
written with `_` is private to the module that declares it, and code in other modules cannot read
it, call it, or see it from a class derived from the type. The type's own access works the same
way: a type declared with `_` can be used only in its own module.

A member written with `^` is shared with the file family of its module, as `(fn-do ^ keep-end _
…)` or `^ do keep-end`{.rows}: code in the family's files uses it as a public member, and
code elsewhere is refused as it is for a private one: `Method "keep-end" of Box is shared,
declared with ^, so only its own file family can call it`. File families are in Modules. Only a
member is shared: `^` on a type, or on a function or value of a module, is an error, `Only a
member can be shared with its file family; declare this + or _`.

A type declares each name once. Declaring two members with one name, such as a property and a
getter, is an error: `Counter declares "count" twice (first on line 8); remove one of them, or
rename it`. Methods are the exception: several may share a name when their parameters differ, as
functions do.

### Inside a type

Inside a member, the type's other members are named alone, as `count` and `label` are in
`summary`, or through `this`, the value the member was called on: `(. this count)`{.brix}. `This`
names the type that declares the code, so `(call This value)`{.brix} inside a method makes another
value of that type.

### Using members

```brix
(main
  (let line (call Line (call Point 1 2) (call Point 5 7)))
  (print (. line end x))
  (= (. line end x) 9)
  (print (. (. line end) x))
  (print (call (. line length-squared)))
  (let measure (. line length-squared))
  (print (call measure))
  (print (. (call (. Point origin)) y))
)

(type + Point
  (prop + x Int)
  (prop + y Int)
  (fn-do + This _
    (param x _)
    (param y _)
  )
  (fn-expr + origin Point :static
    (call Point 0 0)
  )
)

(type + Line
  (prop + start Point)
  (prop + end Point)
  (fn-do + This _
    (param start _)
    (param end _)
  )
  (fn-expr + length-squared Int
    (+ (%% (- (. end x) (. start x)) 2) (%% (- (. end y) (. start y)) 2))
  )
)
```

```output
5
9
89
89
0
```

- **Reading.** `(. line end)`{.brix} reads a property or a getter. Several names read a member of
  each member in turn, so `(. line end x)`{.brix} is `(. (. line end) x)`. In rows it is
  `line.end.x`{.rows}.
- **Assigning.** `(= (. line end x) 9)`{.brix} assigns the last member named. A field of a struct
  is changed where the struct is held; see [Structs](#structs).
- **Calling.** `(call (. line length-squared))`{.brix} calls a method. Named without a call, a
  method is a function value that remembers its object; see [Methods](#methods).
- **Static functions** are read on the type, as in `(call (. Point origin))`{.brix}; see
  [Static functions](#static-functions).
- **Optionals.** `(?. value name)`{.brix} reads a member of a value that may be `null`; see
  Optionals.

## Classes and objects

A type declared with no `is`, or with a class after `is`, is a class. Its values are objects.
Calling the class, as in `(call Account "ana")`{.brix}, makes a new object, and a value of a class
refers to an object: assigning the value or passing it shares the object rather than copying it.

```brix
(main
  (let first (call Account "ana"))
  (let same first)
  (+= (. same balance) 10)
  (print (. first balance))
  (print (== first same))
  (print (== first (call Account "ana")))
)

(type + Account
  (prop + owner String)
  (prop + balance Int)
  (fn-do + This _
    (param owner String)
  )
)
```

```output
10
true
false
```

- **Equality.** Two objects are equal when they are the same object. A class can declare its
  own `==`; see Operators.
- **Text.** An object becomes text as `[Object module:Class]`, such as `[Object app/main:Account]`.
  A class that declares a conversion to `String` becomes that text instead; see Conversions.
- **Null.** A value of a class always holds an object. A value that may hold none has an optional
  type, such as `(? Account)`{.brix}; see Optionals.
- **Lifetime.** An object lives as long as something can reach it, and the collector reclaims it
  after that.

## Properties

A property is a value each value of the type holds, or computes when it is read. Stored, lazy and
computed properties are declared on one row each. Getters and setters are functions.

### Stored properties

```brix
(main
  (let order (call Order "tea"))
  (print (. order item))
  (print (. order quantity))
  (print (. order price))
  (print (. order note))
  (print (. order tags length))
  (print (. order total))
  (= (. order quantity) 3)
  (print (. order total))
)

(type + Order
  (prop + item String)
  (prop + quantity Int (= 1))
  (prop + price _ (= 2.5))
  (prop + note (? String))
  (prop + tags (list String))
  (prop + total Float (=> (* quantity price)))
  (fn-do + This _
    (param item String)
  )
)
```

```output
tea
1
2.5
unknown
0
2.5
7.5
```

A stored property is written with its type, its value, or both:

- `(prop + quantity Int (= 1))`{.brix} has both. The value is converted to the type, so
  `(prop + price Float (= 4))`{.brix} holds a `Float`.
- `(prop + price _ (= 2.5))`{.brix}, with `_` in the place of the type, takes its type from its
  value.
- `(prop + item String)`{.brix} has no value, and starts with the empty value of its type.

A property with neither is an error: `Property 'note' of 'Order' needs either a type or a value`.

A property with no value starts with:

| Type | Value |
| --- | --- |
| A number | `0` |
| `Bool` | `false` |
| `String` | `""` |
| A list or a map | An empty one. |
| An optional | `null` |
| A class, tuple or struct | A new value, made by calling the type with no arguments. |

Properties with no value whose types hold each other in a cycle would never finish being made,
and are an error: `Default of part never finishes: Part.owner builds Owner, and Owner.part
builds Part again. Give one of these fields a value, make it lazy with :=, or make it optional`.

### Initialization

The values of stored properties are computed when a value of the type is made, in the order the
properties are declared, a base class's before its subclass's, and before the constructor's body
runs.

```brix
(main
  (let box (call Box))
  (print (. box area))
)

(type + Box
  (prop + area _ (= (* width height)))
  (prop + width _ (= 3))
  (prop + height _ (= (+ width 1)))
)
```

```output
12
```

- **Reading another property.** A value may read the type's other properties, even ones declared
  after it. A property read before its turn gets its value at that read, once.
- **Cycles.** A value that reads itself, directly or through other values, is an error.
- **Values given another way.** A property that a constructor's parameter or lazy binding fills
  does not compute its own value. See [Constructors](#constructors).
- **What a value may read.** The value of a stored property may read other stored and lazy
  properties, and call functions of the module. It may not read a getter or a computed property,
  `Illegal to reference getters in eager properties`, and may not name `this`: `Illegal to
  reference 'this' in eager properties or properties referenced by them`.

```brix compile-error
(main
  (let pair (call Pair))
)

(type + Pair
  (prop + first _ (= (+ second 1)))
  (prop + second _ (= (+ first 1)))
)
```

```output
Exception: "first" depends on its own value, directly or through the values it reads; break the cycle
<no file:7>
      (prop + first _ (= (+ second 1)))
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Lazy properties

A property written with `:=` computes its value the first time it is read, and keeps it.

```brix
(main
  (let report (call Report))
  (print "made")
  (print (. report summary))
  (print (. report summary))
  (let other (call Report))
  (= (. other summary) "written")
  (print (. other summary))
)

(type + Report
  (prop + summary String (:= (call summarize)))
  (fn-expr + summarize String
    (before
      (print "summarizing")
    )
    "3 rows"
  )
)
```

```output
made
summarizing
3 rows
3 rows
written
```

- **Once.** The value is computed at the first read, and later reads give the same value. A lazy
  property that is never read is never computed.
- **Assignment.** Assigning the property before it is read replaces its value, and the value is
  then never computed. So does a constructor's parameter or lazy binding that fills it.
- **Assigning operators.** An assigning operator such as `+=` reads the property first,
  computing it if it has not been read.
- **What it may read.** Unlike a stored property's value, a lazy value may name `this`, read
  getters and computed properties, and call methods, because it runs after the object is made.
- **No updates.** A lazy property keeps its value when the properties it read later change. For a
  value that follows them, use a computed property, or a tracked one; see Reactivity.
- **Tasks.** A task that reads a lazy property while another task is computing it waits for that
  value.

A lazy property is the property form of a lazy binding, `(lazy name value)`{.brix}; see
Functions.

### Computed properties

A property written with `=>` computes its value each time it is read, as `total` does in
[Stored properties](#stored-properties). It holds nothing, and assigning it is an error:
`Setter not found`. It is a getter written on one row.

To compute the value with steps, write `:do` and a block that returns it:
`(prop + summary String (=> :do (block (let rounded (as celsius Int)) (return (str rounded " C")))))`{.brix}.

### Getters and setters

A getter is a function that is read as a property. A setter is a function that runs when a
property is assigned.

```brix
(main
  (let t (call Temperature))
  (print (. t fahrenheit))
  (= (. t fahrenheit) 212)
  (print (. t celsius))
  (+= (. t fahrenheit) 18)
  (print (. t celsius))
)

(type + Temperature
  (prop + celsius Float (= 20))
  (fn-expr + fahrenheit Float :get
    (+ (/ (* celsius 9) 5) 32)
  )
  (fn-do + fahrenheit _ :set (param value Float)
    (= celsius (/ (* (- value 32) 5) 9))
  )
)
```

```output
68
100
110
```

- **Getters.** `(fn-expr + fahrenheit Float :get body)`{.brix} declares a getter, or `fn-do` in
  place of `fn-expr` for a step body. It takes no parameters, and its body runs at each read. A
  read looks like reading a stored property, so a getter should cost and do no more than a reader
  would expect of one.
- **Setters.** `(fn-do + fahrenheit _ :set (param value Float) body)`{.brix} declares a setter.
  The assigned value is its one parameter, `value` here. A setter has a step body, and gives
  nothing, so its result type is `_`.
- **Together.** A getter and a setter of one name make one property. An assigning operator, such
  as `+=`, reads with the getter and stores with the setter.
- **A setter alone.** A property with a setter and no getter can be assigned but not read. A read
  is an error: `Temperature has no property "fahrenheit"`.

## Constructors

A constructor makes a value of its type. Calling the type, as in `(call Span 2 8)`{.brix}, calls
the constructor whose parameters fit the arguments.

```brix
(main
  (let a (call Span 3))
  (print (str (. a low) " " (. a high) " " (. a span)))
  (let b (call Span 2 8))
  (print (str (. b low) " " (. b high) " " (. b span)))
  (let c (call Span))
  (print (str (. c low) " " (. c high) " " (. c span)))
)

(type + Span
  (prop + low Int)
  (prop + high Int (= 10))
  (prop + span Int)
  (fn-do + This _
    (param high _)
    (lazy span (- high low))
  )
  (fn-do + This _
    (param low _)
    (param high _)
    (lazy span (- high low))
    (print (str "made " low " to " high))
  )
)
```

```output
0 3 3
made 2 to 8
2 8 6
0 10 0
```

A constructor is declared with `fn-do`, with `This` as its name, `_` as its result type, and its
parameters as its first rows: `(fn-do + This _ (param high Int))`{.brix}.

- **No arguments.** A class can always be made with no arguments, as `(call Span)`{.brix} is
  above. Each property then takes its own value, and no constructor's body runs, unless the class
  declares a constructor with no parameters. Calling a class with arguments that no constructor
  takes is an error, `No constructor of Span accepts (String)`, followed by the constructors it
  declares. A tuple or a struct that declares no constructor is made from its fields; see
  [Tuples](#tuples).
- **Parameters that fill properties.** A parameter with the name of a property, the type's own or
  a base class's, fills that property, and the property does not compute its own value. The
  parameter's type may be left out, and is then the property's type. The argument is converted to
  the property's type.
- **Lazy bindings.** A lazy binding such as `(lazy span (- high low))`{.brix} in a constructor
  gives a property its value in place of the property's own. One whose name is not a property's
  binds a local the constructor can read. Lazy bindings run before the body, wherever they are
  written, in the order of the properties they fill.
- **The body.** The constructor's other rows are steps. They run after every property has its
  value, and may assign properties and call methods. A constructor returns early as a step body
  does, with `(return-if (if condition))`{.brix}, and the value is then made. A constructor may
  instead give another value of its type, as in `(return (call This item))`{.brix}, which another
  constructor makes.
- **Several constructors.** A type may declare several constructors, each with different
  parameters, as functions may be overloaded; see Functions.
- **Calling with another type's value.** Calling a type with one value of another type converts
  the value, as `as` does, through a conversion the type declares; see Conversions.

A lazy binding runs before the object exists, so it cannot call the type's methods:
`A "lazy" initializer runs before the object exists, so it cannot call "measure", a method of
Span; compute the value from the constructor's parameters, or assign it in the constructor's
body`. It runs before the body, too, so it cannot read a property the body assigns:

```brix compile-error
(main
  (let log (call Log "build"))
  (print (. log size))
)

(type + Log
  (prop + title String)
  (prop + size Int)
  (fn-do + This _
    (param name String)
    (= title name)
    (lazy size (. title length))
  )
)
```

```output
Exception: Initializer of size reads title before the constructor body assigns it; constructor lazy bindings run before the body
<no file:13>
        (lazy size (. title length))
                      ^^^^^
```

Reading the parameter, `(lazy size (. name length))`{.brix}, gives `5`.

A constructor may take type parameters of its own, written after `This`, and choose the type's
parameters from them, as in
`(fn-do + (of This Storage) _ (constraint = Backing (. Storage (of Backing Item))))`{.brix}. See
[Generic types](#generic-types).

## Methods

A method is a function declared inside a type. It is called on a value of the type, as in
`(call (. route child) "bin")`{.brix}, and that value is `this` in its body.

```brix
(main
  (let route (call Route (list "usr" "local")))
  (print (call (. route text)))
  (let deeper (call (. route child) "bin"))
  (print (call (. deeper text)))
  (print (call (. route text)))
  (print (call (. route text) "\\"))
  (print (call (. (call (. Route parse) "a/b/c") text)))
)

(type + Route
  (prop + parts (list String))
  (fn-do + This _
    (param parts (list String))
  )
  (fn-expr + text String
    (call (. this text) "/")
  )
  (fn-expr + text String
    (param separator String)
    (++ separator (call (. separator join) parts))
  )
  (fn-expr + child Route
    (param name String)
    (call This (++ parts (list name)))
  )
  (fn-expr + parse Route :static
    (param text String)
    (call Route (call List (call (. text split) "/")))
  )
)
```

```output
/usr/local
/usr/local/bin
/usr/local
\usr\local
/a/b/c
```

A method follows every rule of a function in Functions: value and step bodies, parameters,
results, type parameters of its own and overloading. What is particular to methods:

- **Members by name.** Inside a method, the type's properties and methods are named alone, as
  `parts` is in `child`, or through `this`, as in `(call (. this text) "/")`{.brix}.
- **Methods before functions.** Inside a type, a call by name alone calls the type's method of
  that name. A function of the module with the same name is called only when no method's
  parameters fit the arguments.
- **`This`.** `This` names the type that declares the method, so `(call This value)`{.brix} makes
  another value of that type. In a method a subclass inherits, `This` is still the declaring class.
- **Unknown methods.** Calling a method a type does not have is an error: `Route has no method
  "length"`.
- **Bound methods.** A method named without a call, as in `(. route text)`{.brix}, is a function
  value bound to the value; see Functions.

### Static functions

A function declared as `(fn-expr + name Type :static body)`{.brix} belongs to the type rather
than to its values. It is called through the type, as in `(call (. Route parse) "a/b/c")`{.brix}.

- **No `this`.** A static function has no value to work on, so reading a property in it is an
  error: `Required self not defined`.
- **Its own names.** A static function and a method may share a name: called on a value, the name
  calls the method, and called through the type, the static function.
- **Through a type parameter.** A static function an interface requires can be called through a
  type parameter constrained to that interface, as in `(call (. Item kind))`{.brix}; see
  [Constraints](#constraints).

A type holds no values of its own. A value shared by all values of a type is a value of the
module; see Modules.

## Generic types

A type can take type parameters, written after its name, as in `(type + (of Box Item))`{.brix}.
Inside the type, each parameter names a type, and each choice of types for the parameters makes a
separate type.

```brix
(main
  (let box (call Box 5))
  (print (. box item))
  (print (type-of box))
  (let names (call (of Box String) "x"))
  (print (. names item))
  (let doubled (call (. box map) (=> (each) (str "#" each))))
  (print (. doubled item))
  (print (type-of doubled))
  (let pair (call (of Box (tuple Int Int)) (tuple 1 2)))
  (print (. pair item))
)

(type + (of Box Item)
  (prop + item Item)
  (fn-do + This _
    (param item Item)
  )
  (fn-expr + (of map Out) (of Box Out)
    (param transform (fn-type Out (param value Item)))
    (call Box (call transform item))
  )
)
```

```output
5
[TypeInfo Box'I64]
x
#5
[TypeInfo Box'String]
(1, 2)
```

- **Declaring.** `(type + (of Box Item))`{.brix} declares a type with one parameter, and
  `(type + (of Pair First Second))`{.brix} one with two.
- **Choosing the types.** `(of Box String)`{.brix} is the type with `String` for `Item`. In rows, a
  type argument that is itself generic, a tuple or an optional is written in parentheses:
  `Box'(Int, Int)`{.rows}, `Box'(Box'Int)`{.rows}, `Box'(Int?)`{.rows}. A list type needs none:
  `Box'[Int]`{.rows}.
- **Inference.** Calling a generic type without type arguments infers them from the constructor's
  arguments: `(call Box 5)`{.brix} is a `(of Box Int)`{.brix}.
- **Default types.** A rule with `=` under the header gives a parameter a type it takes when none
  is chosen, as `(type + (of Box Item Backing) (constraint = Backing GCBacking))`{.brix} does for
  `Backing`. `(of List Item Backing)`{.brix} declares one, so `(list Int)`{.brix} is
  `(of List Int GCBacking)`{.brix}.
- **Constraints.** `(type + (of Box Item) (constraint is Item Titled))`{.brix} requires every type
  chosen for `Item` to implement `Titled`, and lets the type's members use what `Titled` declares;
  see [Constraints](#constraints).
- **Methods with parameters of their own.** A method can take its own type parameters, as
  `(of map Out)`{.brix} does, as functions can; see Functions.
- **Sharing a name.** A type and a generic type, or two generic types with different numbers of
  parameters, may share a name. Each use picks the one whose number of type arguments matches.

## Tuples

A tuple is a fixed group of values, its fields. Tuples are values, not objects: a tuple is copied
where it is assigned or passed, two tuples are equal when their fields are, and a tuple is never
changed in place.

```brix
(main
  (let pair (tuple 3 "three"))
  (print pair)
  (print ([] pair 1))
  (print (. pair length))
  (let corner (tuple (: x Int) (: y Int)) (tuple 4 5))
  (print (+ (. corner x) (. corner y)))
  (let p (call Point 1 2))
  (let moved (= (. p x) 10))
  (print p)
  (print moved)
  (print (== p (call Point 1 2)))
  (print (== p (tuple 1 2)))
  (let q Point (tuple 7 8))
  (print (. q y))
)

(type + Point (is (tuple (: x Int) (: y Int))))
```

```output
(3, three)
three
2
9
(1, 2)
(10, 2)
true
false
8
```

- **Writing a tuple.** `(tuple 3 "three")`{.brix} makes a tuple of two values. In rows, values
  separated by commas make a tuple, with or without parentheses: `3, "three"`{.rows} and
  `(3, "three")`{.rows} are the same tuple. Tuples nest.
- **Tuple types.** In `(let pair (tuple Int String) value)`{.brix}, `pair` has the type of a tuple
  of an `Int` and a `String`. In `(let corner (tuple (: x Int) (: y Int)) value)`{.brix}, the
  tuple's fields have names.
- **Fields.** A field is read by its position, as in `([] pair 1)`{.brix}, or by its name, as in
  `(. corner x)`{.brix}. The position must be a constant. `length` is the number of fields.
- **Text.** A tuple becomes text as its fields in parentheses: `(3, three)`.
- **Assigning a field.** `(= (. p x) 10)`{.brix} does not change `p`. It gives a new tuple, with
  `x` changed and the other fields copied. To keep it, assign it back:
  `(= p (= (. p x) 10))`{.brix}. An assignment whose new tuple is not kept is an error:
  `Assigning x creates a new Point tuple that is discarded; write p = p.x = value`.
- **Taking a tuple apart.** `(let (tuple x y) q)`{.brix} binds each field to a local; see
  Patterns.

### Named tuple types

A type built on a tuple type, as `Point` is above, or on `Tuple` with properties as its fields, is
a named tuple type.

```brix
(main
  (print (call TextStyle))
  (print (call TextStyle "serif" 14))
  (print (call TextStyle (:size 20) (:bold true)))
  (print (call HeadingStyle "sans" 18 true 2))
)

(type + TextStyle (is Tuple)
  (prop + font String (= "sans"))
  (prop + size Int (= 12))
  (prop + bold Bool)
)

(type + HeadingStyle (is TextStyle)
  (prop + level Int (= 1))
)
```

```output
(sans, 12, false)
(serif, 14, false)
(sans, 20, true)
(sans, 18, true, 2)
```

- **Fields.** Each stored property is a field, in the order declared. Getters and computed
  properties are not fields. Lazy properties are not allowed.
- **Making one.** Without a constructor of its own, a named tuple takes its fields as arguments,
  in order or by name, and a field left out takes its value, or the empty value of its type.
- **Methods.** A named tuple type may declare constructors, methods, getters, conversions and
  operators, as a class does.
- **Deriving.** A tuple type built on another, as `HeadingStyle` is on `TextStyle`, has the
  other's fields first, then its own.
- **Equality.** Two tuples of one named type are equal when each of their fields is, using each
  field's `==`. A named tuple is never equal to a tuple of another type, even one with the same
  fields, so `(== (call Point 1 2) (tuple 1 2))`{.brix} is false.
- **Conversion.** A tuple converts to a named tuple type whose fields its fields convert to, field
  by field in order, as `(tuple 7 8)`{.brix} does to `Point` above. The fields' names are not
  compared.

## Structs

A type built on `Struct` is a struct: a named tuple type whose fields can be changed in place.

```brix
(main
  (let a (call Dimensions 1 2))
  (let b a)
  (= (. b width) 10)
  (print a)
  (print b)
  (call grow a)
  (print a)
  (+= (. a height) 5)
  (print a)
)

(type + Dimensions (is Struct)
  (prop + width Int)
  (prop + height Int)
)

(fn-do + grow _
  (param size Dimensions)
  (+= (. size width) 1)
  (print (str "grown to " size))
)
```

```output
(1, 2)
(10, 2)
grown to (2, 2)
(1, 2)
(1, 7)
```

- **Changed in place.** `(+= (. a height) 5)`{.brix} changes `a`. The value of a field assignment
  is the value assigned.
- **Copied.** Like a tuple, a struct is copied where it is assigned or passed, and when a method
  is called on it: a function that changes its parameter changes its own copy, and so does a
  method that changes a field of `this`. A method that changes a struct gives the changed struct
  back, as in `(= size (call (. size widened) 1))`{.brix}.
- **Stored inline.** A struct held by another struct, or by a class, is stored inside it, so
  `(= (. line start x) 7)`{.brix} changes `line`.
- **Fixed layout.** A struct's fields cannot be added or removed. Lazy properties are not
  allowed: `Property not supported`.
- **A struct nothing holds.** A field of a struct that no local, parameter or property holds, such
  as one `as` gives or a call returns, cannot be assigned, since the change would be lost:
  `(= (. (as size Dimensions) width) 9)`{.brix} is an error, `Assigning width changes a Dimensions
  struct that nothing holds, so the change is lost; assign width where the struct is held`.

Otherwise a struct is declared, made, compared and converted as a named tuple is.

## Inheritance

A class declared `is` another class is derived from it. The derived class has every member of
its base, and may declare more, and may replace the base's methods with its own.

```brix
(main
  (let pets (list Animal) (list (call Dog "Rex" "terrier") (call Cat "Tom")))
  (for (in pet pets)
    (print (call (. pet describe)))
  )
  (let first ([] pets 0))
  (print (is first Dog))
  (print (is first Cat))
  (print (. (as first Dog) breed))
)

(type + Animal
  (prop + name String)
  (fn-do + This _
    (param name String)
    (print (str "new " name))
  )
  (fn-expr + sound String
    "..."
  )
  (fn-expr + describe String
    (str name " says " (call sound))
  )
)

(type + Dog (is Animal)
  (prop + breed String)
  (fn-do + This _
    (param name String)
    (param breed String)
    (call super name)
  )
  (fn-expr + sound String
    "woof"
  )
  (fn-expr + describe String
    (str (call (. super describe)) ", a " breed)
  )
)

(type + Cat (is Animal)
  (fn-expr + sound String
    "meow"
  )
)
```

```output
new Rex
new Tom
Rex says woof, a terrier
Tom says meow
true
false
terrier
```

- **What is inherited.** Properties, methods, getters, setters, conversions and constructors. A
  derived class that declares no constructor is made with its base's, as `(call Cat "Tom")`{.brix}
  is.
- **Overriding.** A method, getter, setter or operator declared again in a derived class replaces
  the base's. A call is made to the method of the object's own class, even through a value whose
  type is the base, as `describe` and `sound` are above.
- **What an override may change.** Its result may be a type the base's result converts to, such
  as a derived class. An override that may raise an error the base's method does not declare is
  an error: `"sound" can fail with Error, but the method it overrides cannot; add "?! Error" to
  that method, or catch the error inside "sound"`.
- **The base's method.** `(call (. super describe))`{.brix} calls the base's method, and
  `(. super label)`{.brix} reads the base's getter, even from an override of it. `super` alone is
  not a value: `Bare super is not a value`.
- **The base's constructor.** `(call super name)`{.brix} calls a constructor of the base, and
  must be the constructor's first row: `super constructor call must be first`.
- **Properties.** A derived class may give an inherited stored property a new value, as in
  `(prop + legs _ (= 4))`{.brix}, and its value is converted to the type the base gave the
  property. It may not give the property another type, `"name" is inherited from Animal, so it
  keeps the type it has there and cannot state its own; remove "Int"`, nor declare it another way:
  a property declared with `=` in the base is declared with `=` in the derived class too.
- **One base.** A class has at most one base class. It may implement any number of interfaces.
- **Generic classes.** A generic class may derive from a generic class, as in
  `(type + (of Child Item) (is (of Base Item)))`{.brix}, or from one choice of its types, as in
  `(type + Numbers (is (of Holder Int)))`{.brix}.

### Values of a base class

A value of a derived class can be used where its base class is wanted: it can be assigned to a
local of the base type, passed for a parameter of it, or added to a list of it. The reverse needs
a test or a conversion:

- **`is`.** `(is pet Dog)`{.brix} is true when the object is a `Dog`, or of a class derived from
  `Dog`. `(is! pet Dog)`{.brix} is true only when the object's class is `Dog` itself.
  `(is-not pet Dog)`{.brix} and `(is!-not pet Dog)`{.brix} are their opposites.
- **`as`.** `(as pet Dog)`{.brix} gives the object as a `Dog`. When it is not one, the program
  panics: `Invalid cast to [Object app/main:Dog]`.
- **Patterns.** A match arm `(case (: dog Dog) value)`{.brix} is taken when the object is a `Dog`,
  and binds it as one; see Patterns.

An `if` that tests `(is pet Dog)`{.brix} does not yet change the type of `pet` inside it.

## Interfaces

An interface declares members without being a kind of value of its own. A type implements the
interface by having those members, and then its values can be used where the interface is wanted.

```brix
(main
  (let shapes (list Shape) (list (call Square 2) (call Circle 1)))
  (for (in shape shapes)
    (print (call (. shape describe)))
  )
  (let held Shape (call Square 3))
  (print (is held Square))
  (print (. (as held Square) side))
)

(type + Shape (is Interface)
  (fn-expr + name String :get)
  (fn-expr + area Float)
  (fn-expr + describe String
    (str name " of area " (call area))
  )
)

(type + Square
  (implements Shape)
  (prop + side Float)
  (prop + name _ (= "square"))
  (fn-do + This _
    (param side Float)
  )
  (fn-expr + area Float
    (* side side)
  )
)

(type + Circle (is (tuple (: radius Float)))
  (implements Shape)
  (fn-expr + name String :get
    "circle"
  )
  (fn-expr + area Float
    (* 3.0 radius radius)
  )
)
```

```output
square of area 4
circle of area 3
true
3
```

### Declaring an interface

An interface is a type built on `Interface`. Its members are functions, getters, setters,
static functions and [associated types](#associated-types):

- **Required members.** A member with no body that gives a value, such as `area`, must be declared
  by every type that implements the interface.
- **Hooks.** A member with no body and no result may be left out; see [Hooks](#hooks).
- **Members with bodies.** A member with a body, such as `describe`, is given to every type that
  implements the interface, which may declare its own instead. Its body may use the interface's
  other members, and calls them on the value it was called for.
- **Static functions.** A static function with no body is required of each implementing type as a
  static function; see [Constraints](#constraints).
- **Deriving.** An interface may be built on other interfaces, as
  `(type + Solid (is Interface) (implements Shape))`{.brix} is on `Shape`. A type that implements
  it implements each of them too.
- **Type parameters.** An interface can take type parameters, as `(of Iterable Item)`{.brix} does.
- **No storage.** An interface holds no values, so a stored property in one is an error: `Shape is
  an interface, so it cannot store "name"; declare a getter for the types that are Shape to answer,
  as in "+ get name String"`.
- **No values of its own.** Calling an interface is an error, `Shape is an interface, so it has no
  constructor; construct a class that is Shape`, and so is declaring a local of an interface type
  with no value.

### Hooks

A member with no body and no result, such as `reset` here, is a hook. A type that implements the
interface may declare it or leave it out, and calling it on a value whose type leaves it out does
nothing:

```brix
(main
  (let shapes (list Shape) (list (call Square 2) (call Dot)))
  (for (in shape shapes)
    (call (. shape reset))
    (print (call (. shape area)))
  )
)

(type + Shape (is Interface)
  (fn-expr + area Float)
  (fn-do + reset _)
)

(type + Square
  (implements Shape)
  (prop + side Float)
  (fn-do + This _
    (param side _)
  )
  (fn-expr + area Float
    (* side side)
  )
  (fn-do + reset _
    (print "resetting the square")
  )
)

(type + Dot
  (implements Shape)
  (fn-expr + area Float
    0.0
  )
)
```

```output
resetting the square
4
0
```

A member that gives a value has no such default, so leaving one out is an error.

### Implementing an interface

A type implements an interface by naming it after its header, as `Square` and `Circle` do:
`(type + Square (implements Shape))`{.brix}. Classes, named tuples, structs and number types can
implement interfaces.

- **What must match.** Each required member must be declared, with a result its own converts to.
  The parameter names need not match. A method may take more parameters than the interface's,
  when each of them has a default.
- **Properties for getters.** A stored property satisfies a required getter of its name, as
  `name` does for `Square`, and a public one satisfies a getter and setter together.
- **Errors.** A method may raise fewer errors than the interface declares, but not more.
- **A missing member** is an error wherever a value of the type is made a value of the interface,
  reported at the type's declaration: `Blob does not implement "area", which Shape declares
  without a body; declare "area" in Blob`.
- **Only interfaces.** A type implements only interfaces. `(type + Box (implements Base))`{.brix},
  with `Base` a class, is an error: `Box cannot implement Base, which is not an interface; a type
  is built on one type, written in its header, as in "+ Box is Base"`.
- **Derived classes.** A class implements every interface its base class does.

### Values of an interface

A value of a type that implements an interface can be used where the interface is wanted, and
converts to it on its own. A call on a value of an interface calls the member of the type the
value holds.

- **`is` and `as`.** `(is held Square)`{.brix} tests what the value holds, and
  `(as held Square)`{.brix} gives it, panicking when it holds something else. A match arm
  `(case (: square Square) value)`{.brix} does both; see Patterns.
- **Copies.** A tuple or a struct held as an interface is a copy, and `as` gives another copy.
- **Equality.** Two values of an interface holding objects are equal when they hold the same
  object. Two holding tuples or numbers are equal when the held values are, by their type's `==`.
- **Narrower interfaces.** A value of an interface does not convert to an interface derived from
  it: `Shape cannot be cast to Solid; there is no conversion between them`.

### Constraints

A constraint among a generic function's parameters, or under a generic type's header, as in
`(type + (of Shelf Item) (constraint is Item Titled))`{.brix}, requires the type chosen for `Item`
to implement `Titled`. Code that knows only that `Item` is `Titled` can then use the members
`Titled` declares, and call its static functions through the type parameter.

```brix
(main
  (let shelf (call (of Shelf Book)))
  (call (. shelf add) (call Book "Dune"))
  (call (. shelf add) (call Book "Emma"))
  (print (call (. shelf titles)))
  (print (call label (call Book "Dune")))
)

(type + Titled (is Interface)
  (fn-expr + title String :get)
  (fn-expr + kind String :static)
)

(type + Book
  (implements Titled)
  (prop + title String)
  (fn-do + This _
    (param title String)
  )
  (fn-expr + kind String :static
    "book"
  )
)

(type + (of Shelf Item)
  (constraint is Item Titled)
  (prop + items (list Item))
  (fn-do + add _
    (param item Item)
    (+= items item)
  )
  (fn-expr + titles String
    (call (. ", " join) (*> items (. each title)))
  )
)

(fn-expr + (of label Item) String
  (param item Item)
  (constraint is Item Titled)
  (str (call (. Item kind)) ": " (. item title))
)
```

```output
Dune, Emma
book: Dune
```

- **Static functions** are called through the type parameter, as in `(call (. Item kind))`{.brix}.
  Calling one through the interface itself is an error: `Static methods cannot be called on an
  interface type`. Calling one through a type parameter with no constraint that declares it is an
  error too: `Static method kind is not declared by constraints on Item`.
- **`is!`.** The constraint in `(type + (of Shelf Item) (constraint is! Item Book))`{.brix}
  requires exactly `Book`. See Functions for constraints on functions.

### Associated types

An interface may declare a type that each implementing type chooses, by a member that names only
a type, such as `(prop + _ Context)`{.brix}, or a type with parameters, such as
`(prop + _ (of Backing Item))`{.brix}. An implementing type binds it with
`(prop + _ Context (= I32))`{.brix}. Code constrained to the interface names the chosen type
through the type parameter, as `(. Storage Context)`{.brix}, and can make a value of it with
`(call (. Storage Context))`{.brix}. A type that implements the interface without binding one of
its associated types is an error: `Must implement associated type Context in Arena`.

## Types of alternatives

A type built on several types stands for any one of them. In rows it is written with `||`, as
`+ Choice is Int || String`{.rows}.

```brix
(main
  (print (call twice 3))
  (print (call twice "ab"))
  (print (call kind 3))
  (print (call kind "x"))
  (let held Choice 4)
  (print (type-of held))
  (print (type-of (call Choice "y")))
  (print (call Choice 2.5))
)

(type + Choice (is Int String))

(fn-expr + twice String
  (param value Choice)
  (str value value)
)

(fn-expr + kind String
  (param value Choice)
  (if-expr (is value Int) "number" "text")
)
```

```output
33
abab
number
text
[TypeInfo I64]
[TypeInfo String]
2
```

- **Not a union.** A type of alternatives has no values of its own. A value of it is a value of one
  of its types, and keeps that type, as `held` keeps `Int`. A union holds a case, and can tell
  which at run time; see Unions and enums.
- **A function for each type.** A parameter of a type of alternatives makes a function generic: the
  compiler makes one function for each of the types the program passes, as for a type parameter;
  see Functions. In each, the parameter has that one type, so `is` is decided as the function is
  made.
- **Other types.** A value of none of the types is an error: `No function "twice" accepts (F64);
  it is declared as "twice(value Choice)"`.
- **Matches.** A `match` over such a parameter is checked in each function made from it, so an arm
  for one type is an error in the others: `Pattern has no possible matches`. Test with `is`
  instead, or declare an overload for each type.
- **Calling the type** with a value converts it as `as` does, to the first type `as` can convert it
  to: `(call Choice 2.5)`{.brix} is the `Int` `2`. With no value it is an error: `Choice stands for
  any one of Int and String, so it has no constructor; give it a value of one of them, as in
  Choice(value)`.
- **Collections.** A list or other collection of a type of alternatives cannot be built yet.
- **The stream family.** `(stream Int)`{.brix} is the best-known type of alternatives: `Stream'Item`
  stands for any one of the stream interfaces; see Streams.

Use a union for a value that can be in one of several states, an interface for behavior many types
share, and a type of alternatives for a parameter that takes several types and handles each of
them as it is compiled.

## Delegation

A class can implement an interface by passing it to one of its properties. A delegation before a
property, as in `(type + Logger (delegate Sink) (prop + console Console))`{.brix}, makes the class
implement `Sink`, and each member of `Sink` the class does not declare itself is called on the
property's value.

```brix
(main
  (let logger (call Logger))
  (call (. logger write) "ready")
  (call (. logger flush))
  (let sink Sink logger)
  (call (. sink write) "go")
  (print (. logger count))
)

(type + Sink (is Interface)
  (fn-do + write _
    (param text String)
  )
  (fn-do + flush _)
)

(type + Console
  (implements Sink)
  (fn-do + write _
    (param text String)
    (print (str "> " text))
  )
  (fn-do + flush _
    (print "flushed")
  )
)

(type + Logger
  (delegate Sink)
  (prop + console Console)
  (prop + count Int)
  (fn-do + write _
    (param text String)
    (+= count 1)
    (call (. console write) (str count ": " text))
  )
)
```

```output
> 1: ready
flushed
> 2: go
2
```

- **The class's own members win.** `Logger` declares `write`, so its own runs. It does not declare
  `flush`, so `flush` is called on `console`.
- **Read at each call.** The property is read each time a member is called, so assigning it
  another value sends later calls there. A getter can be the delegate, and runs at each call.
- **The delegate's type** need not declare that it implements the interface: it needs only the
  members.
- **Several interfaces.** `(type + Logger (delegate Sink Source) (prop + console Console))`{.brix}
  delegates two interfaces to one property, and a class may delegate different interfaces to
  different properties.
- **Identity.** The class's values are the class: `(is logger Logger)`{.brix} is true, and
  `(is logger Console)`{.brix} is false.
- **Derived classes** inherit the delegation, and may declare members of the interface
  themselves.

## Extensions

An extension adds members to a type declared elsewhere, including the core library's types.

```brix
(main
  (let five 5)
  (print (. five doubled))
  (print (call (. "abc" shout)))
  (print (. (list 10 20 30) second))
)

(extend + Int
  (prop + doubled Int (=> (* this 2)))
)

(extend + String
  (fn-expr + shout String
    (++ (. this uppercased) "!")
  )
)

(extend + (of List ''Item ''Backing)
  (prop + second Item (=> ([] this 1)))
)
```

```output
10
ABC!
20
```

An extension is declared with `extend`, and has members as a type has them, as in
`(extend + Type (prop + name Type (=> value)))`{.brix}.

- **What it can add.** Methods, getters and setters, computed properties, conversions, operators
  and constructors. A stored property is an error: `Stored properties are not allowed in
  extensions`.
- **`this`** is the value the member is called on, of the extended type.
- **Where it applies.** An extension's members can be used in its own module and in modules that
  import it. Elsewhere the type does not have them.
- **Derived classes.** An extension of a class reaches classes derived from it, and its members
  call the overrides of the object's class.
- **Generic types.** In `(extend + (of List ''Item ''Backing))`{.brix}, each name after `''` is a
  type parameter of the extension, so it extends every `List`. `(extend + (of Box Int))`{.brix}
  extends only that choice. A `where` clause, as in
  `(extend + (of Box ''Item) (where (constraint is Item Titled)))`{.brix}, extends only the choices
  that meet it.
- **Conflicts.** An extension may add an overload of a method the type declares, but not a member
  with the same name and parameters: `Extension member conflicts with existing member`. When two
  extensions add the same member, a call to it is an error: `More than one extension declares
  "shout" for String, so the call is ambiguous; remove or rename all but one of them`.

An `as` clause, as in `(extend + Thing (as Shape))`{.brix}, is reserved for making a type
implement an interface from outside it. The compiler does not build it yet: `Not yet
implemented`. Such an extension has no body, and is written `(extend-line + Thing (as Shape))`, in
rows `+ extend Thing as Shape`{.rows}.

## Facets

A facet is a part of a class declared apart from the class's body, usually in a file of its own.
It adds stored properties and methods to the class, its owner, and they live in the owner's
objects: an object and its facets are one object, with one identity.

```brix
(main
  (let box (call Box))
  (call (. box scroll-by) 30)
  (call (. box scroll-by) 30)
  (print (. box offset))
  (print (call (. box describe)))
)

(type + Box
  (prop _ height Int (= 50))
  (fn-expr + describe String
    (str "height " height ", scrolled " offset)
  )
)

(type + BoxScroll (facet-of Box)
  (prop + offset Int (= 0))
  (fn-do + scroll-by _
    (param amount Int)
    (+= offset amount)
    (if (> offset height)
      (= offset height)
    )
  )
)
```

```output
50
height 50, scrolled 50
```

A facet is declared with `facet-of`, as in `(type + BoxScroll (facet-of Box))`{.brix}, in rows
`+ BoxScroll is facet of Box`{.rows}.

- **The owner.** `facet-of` names a class, or another facet of one, and the chain ends at a class,
  the facet's owner. A facet of a facet reaches its parent's members. A class that takes type
  parameters has no facets: `BoxScroll can't be part of Box, since a class that takes type
  arguments has no facets`.
- **Where.** A facet is declared in its owner's file or in a file of its family, named after it,
  as `box.scroll.bx` is beside `box.bx`; see Modules. Anywhere else it is an error: `Extra is a
  facet of Box, so it is declared in the file family of box.bx, in a module named after it`.
- **One object.** A facet's stored properties are stored in the owner's objects, after the
  owner's own. Each constructor of the owner, and of the classes derived from it, gives them
  their first values: the owner's first, then each facet's, in the order of the names of the
  files that declare them, and in the order they are declared within a file. A constructor's
  argument named after a facet's property gives it that value instead, as one named after the
  owner's own property does. A facet's private property takes no argument's value: outside the
  facet's file the argument is only an argument, and in it the argument is an error, since two
  facets may keep one name: `hidden is private to facet BoxScroll, so a constructor's argument
  can't give it a value`.
- **No constructors.** A facet has no constructors and no static functions. Each of its stored
  properties needs a value or an optional type: `offset needs a value or an optional type, since
  BoxScroll is a facet and no constructor gives it one`.
- **What it may declare.** Methods, getters and setters, stored, lazy and computed properties,
  operators, conversions, events, `using`, interfaces, sections, `when` and `whenever` rules and
  dependencies.
- **Interfaces.** An interface a facet names, as in `(type + BoxScroll (facet-of Box) (implements
  Scrollable))`{.brix}, in rows `is Scrollable`{.rows} under the facet's line, is its owner's: the
  owner and the classes below it implement it, through the facet's members, which a class below
  may override. A facet names only interfaces: `BoxScroll cannot implement Base, which is not an
  interface; a facet is part of its class and is built on no type`. A member the facet leaves out
  is reported as the owner's, and names the facet: `Box does not implement "position", which
  Scrollable declares without a body; declare "position" in BoxScroll`. The other way, an
  interface the owner names is answered by its facets' public members as by its own, so a class
  hands an interface to a facet by naming it and leaving the members to the facet.
- **No markup.** A class's `content` section, which gives its objects their children, is the
  class's alone, written in its body: `BoxMarkup is a facet, so it writes no markup; a content
  section is its owner's alone`.
- **Sections and rules.** A facet's sections, `when` and `whenever` rules and dependencies are its
  owner's: each object of the owner runs them, after the owner's own and before those of a class
  below, and a facet's section subscribes to any event the facet reaches, its owner's or its own.
- **Styles.** A facet's `using` gives its owner the style or style sheet, after the owner's own
  and in the order of the facets, as if the owner named it: the owner's objects have the style's
  properties wherever the owner is, markup writes them on its elements, and any code reads them,
  the facet's as `$name`. `BoxFocus` uses `GUIBoxFocus`, so the focus code and its style move
  together.
- **Events.** A facet's events are its owner's objects' events. The facet, the owner's file and
  the facets that see it publish one by its name alone, other code subscribes to it on an object
  of the owner's type as to any event, and a public one bubbles as the owner's own do. Closing an
  object ends what subscribes to its facets' events too. An event's name is its owner's alone,
  private or not: `Facets BoxFocus and BoxScroll of Box both declare shaken; an event's name is
  its class's alone, private or not`.
- **`this`** is the object the member is called on, of the owner's type.
- **Uses.** Wherever the owner's type is visible, its objects have the public members of all its
  facets, as they have the owner's own, whatever the code using them imports, and in the owner's
  file family they have the members the facets share with `^` as well. Only the code of the
  facets' other files reaches fewer; see below. The owner's own members use them by name, as
  `describe` uses `offset`.

### Facet values

A facet's name is a type, which goes wherever a class's does: a parameter, a property, a local, the
items of a list, an optional. Its value is an object of the owner seen through the facet.
- **Narrowing is unasked.** A value of the owner, or of a class below it, becomes a value of any of
  its facets, and a facet's value becomes a value of the facets it is part of. The object is the
  same object.
- **What it has.** A facet's value has the members of its facet and of the facets it is part of,
  and no others: `BoxScroll has no property "width"; a facet's value has only its facet's members
  and those of the facets it is part of. Write "as Box" to reach the rest of Box`. A class below
  that overrides one of them answers for it, as on the owner's value. In a facet's file, a value of
  a sibling facet has its members only where the facet sees the sibling. A facet's events are not
  reached through its value yet, and its value becomes an interface only as its owner: write
  `(as value Box)`{.brix} for either.
- **Widening is asked.** `(as value Box)`{.brix}, in rows `value as Box`{.rows}, gives the owner's
  value back, checked when the program is built: `Expected Box here, but this is BoxScroll, which
  becomes Box only when asked; write "(as value Box)"`. A cast to a class below the owner tests the
  object, as one from the owner does, and so do `is` and an arm of a match that name such a class.
- **No test for a facet.** Every object of the owner has every facet, so `is` and an arm don't name
  one: `BoxScroll is a facet, which every Box has, so no value is tested for it`. `(type-of value)`
  of a facet's value is its owner's `TypeInfo`.
- **No constructor.** `BoxScroll is a facet of Box, so it has no constructor; a Box is a BoxScroll`.
  A stored property of a facet's type needs a value or an optional type: `held is a BoxScroll, a
  facet of Box, which has no constructor to give it its first value; give it a value, or make it
  optional`.
- **Names outside the family.** Code outside the owner's family can't import a facet's file, so
  importing the owner's module brings its facets' names, and nothing else of their files: `Type
  "BoxScroll" not found; BoxScroll is a facet of Box, whose name comes with Box's module. Import
  that module to name it`.

### What a facet reaches

Code in a file of the family, other than the owner's own, that declares a facet reaches:
- the facet's own members, and those of the facets it is part of;
- every member the owner declares, private ones included, and the private functions, values and
  types of the owner's file;
- the public and shared members of the sibling facets it names with `sees`.

In `(type + BoxScroll (facet-of Box) (sees BoxFocus))`{.brix}, `sees` names another facet of the
same owner; in rows, `sees BoxFocus`{.rows} goes on the line under the facet's. The grant is one
way and is not passed on. A sibling's member that the facet does not see is an error: `Name
"focused" not found; facet BoxFocus declares it, and only the facets a facet sees are in reach.
Write "sees BoxFocus" under the facet that uses it`. A file that declares several facets of one
owner reaches what they reach together. Other objects of the owner's type, such as a parent or a
child, reach the same there and no more. The owner's own file reaches every facet, also when it
declares one.

- **Private members** of a facet are private to its file. Two facets may each declare a private
  member of one name, and the code of each file reaches its own.
- **Shared members**, declared with `^`, are for the owner's file family: the owner's file, the
  facets that see the facet, and the family's other files reach them, on any object of the
  owner's type. A facet shares what only the owner's family calls, such as a step the owner's
  layout runs, and keeps it from code outside.
- **Public and shared members** are the owner's. A facet can't declare a member the owner has,
  its own or one a facet of a class above declares: `offset is already a member of Box, so facet
  BoxScroll can't declare it`. Two facets of one owner can't both declare a member of one name
  unless both are private.
- **Derived classes** have the owner's facets and their storage. A derived class overrides a
  facet's method, getter or setter as it would one its base declares, with a member of the same
  name and kind. On the derived class's objects, every call of the facet's member runs the
  override, the facet's own calls included, and the override reaches the facet's through
  `super`. It gives a facet's stored or lazy property a new value by declaring the property with
  one, as it would its base's: its objects start with that value, and the facet's own code reads
  it. It can't declare any other member a facet declares: `Panel can't declare offset, since
  facet BoxScroll of Box declares it`.

## Type information

A type's name used as a value is a `TypeInfo`, which describes the type. `(type-of value)`{.brix}
gives the `TypeInfo` of a value's type.

```brix
(main
  (let pet Animal (call Dog))
  (let info (type-of pet))
  (print info)
  (print (. info name))
  (print (. info concrete-name))
  (print (. Dog base))
  (print (. (type-of 5) name))
  (print (. (type-of (tuple 1 2)) size))
  (print (== (type-of pet) Animal))
)

(type + Animal)

(type + Dog (is Animal))
```

```output
[TypeInfo Animal]
Animal
Dog
[TypeInfo Animal]
I64
16
true
```

- **`type-of`** gives the type the value is declared with, and evaluates its operand once.
- **`name`** is the type's name, and `concrete-name` the name of the class the object was made
  as, or the type an interface value holds.
- **`base`** is the type it is built on, and `properties` describes its stored properties.
- **`size`** is the number of bytes a value of the type takes where it is stored. An object takes
  the size of a reference to it.
- **Equality.** `==` compares two `TypeInfo` values.

Built-in types covers `TypeInfo` in full.
