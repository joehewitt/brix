# Types

```brix crash
(main
  (print (. (call Foo) foo))
)

(type + Foo
  (prop + foo _)
)
```

```output
Exception: Property 'foo' of 'Foo' needs either a type or a value
<no file:7>
      (prop + foo _)
      ^^^^^^^^^^^^^^
```

## Recursive Generic Variable Lookup

```brix
(main
  (let node (call (of Node String Int) "left" 42 (call (of CPointer (of Node String Int)))))
  (call accept node)
)

(type + (of Node Left Right) (is Tuple)
  (prop + left Left)
  (prop + right Right)
  (prop + next (of CPointer (of Node Left Right)))
)

(fn-do + (of accept Left Right) _
  (param node (of Node Left Right))
  (print (type-of node))
  (print (. node left))
  (print (. node right))
)
```

```output
[TypeInfo Node'String'I64]
left
42
```

## Conversions between aliases

```brix
(main
  (let foo (call Foo))
  (let bar Bar foo)
  (print bar)
)

(blank)
(type-line + Foo (alias Bar))
```

```output
[Object __source__:Foo]
```

```brix
(type-line + Foo)
(type-line + Bar (alias Baz) (is Foo))
(blank)

(main
  (let foo (call Bar))
  (let bar Baz foo)
  (print bar)
)
```

```output
[Object __source__:Bar]
```

### Numeric aliases

```brix
(type-line + Stuff (is I32))
(blank)

(main
  (let s Stuff 42)
  (print s)
)
```

```output
42
```

```brix
(main
  (let s Stuff 42)
  (print s)
  (let s2 Fluff 42)
  (print s2)
)

(blank)
(type-line + Stuff (alias Fluff) (is I32))
```

```output
42
42
```

## Casts

```brix
(main
  (print (as (call Foo) Bar))
)

(type + Foo
  (fn-to-expr + Bar
    (call Bar)
  )
)

(type + Bar)
```

```output
[Object __source__:Bar]
```

```brix crash
(main
  (let foo (call Foo))
  (print (as (call Foo) Bar))
)

(type + Foo
  (fn-to-expr _ Bar
    (call Bar)
  )
)

(type + Bar)
```

```output
Exception: Converters must be public
<no file:8>
      (fn-to-expr _ Bar
      ^^^^^^^^^^^^^^^^
```

## Constructors

### Builtin type constructors

```brix
(main
  (print (call F32))
)
```

```output
0
```

```brix
(main
  (let s (call String))
  (print (str "got '" s "'"))
)
```

```output
got ''
```

```brix
(main
  (let s (call I32))
  (print (str "got " s))
  (blank)
  (= s (call I32 4.5))
  (print (str "got " s))
  (blank)
  (= s (call I32 " 42 "))
  (print (str "got " s))
)
```

```output
got 0
got 4
got 42
```

### A number type called with text that is no number of it panics.

```brix crash
(main
  (print (call I32 "42x"))
)
```

```stderr
Unhandled panic: "42x" is not a whole number
<core/number.bx>

Task 1

Stack trace:
  0  core/number:read-integer core/number.bx
  1  core/number:I32:from String6 core/number.bx
  2  __source__:main no file:3:10
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

## Subclassing builtin types

```brix
(main
  (print (call Foo 10))
)

(type + Foo (is (of CPointer I32)))
```

```output
[CPointer core/number:I32*]
```

```brix crash
(main
  (print (call Foo 10))
)

(type + Foo (is (of Optional I32)))
```

```output
Exception: Type has illegal base class
<no file:6>
    (type + Foo (is (of Optional I32)))
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix crash
(main
  (print (call Foo 10))
)

(type + Foo (is Function))
```

```output
Exception: Type has illegal base class
<no file:6>
    (type + Foo (is Function))
    ^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix crash
(main
  (print (call Foo 10))
)

(type + Foo (is CFunction))
```

```output
Exception: Type has illegal base class
<no file:6>
    (type + Foo (is CFunction))
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix crash
(main
  (print (call Foo 10))
)

(type + Foo (is Bool))
```

```output
Exception: Type has illegal base class
<no file:6>
    (type + Foo (is Bool))
    ^^^^^^^^^^^^^^^^^^^^^^
```

```brix
(main
  (let bar (call Bar))
  (= (. bar a) 99)
  (print (as bar Foo))
  (print (call Foo bar))
)

(type + Foo
  (prop + bar Bar)
  (fn-from-do + (: b Bar)
    (let foo (call This))
    (= (. foo bar) b)
    (return foo)
  )
  (fn-to-expr + String
    (str "Foo " bar)
  )
)

(type + Bar
  (prop + a _ (= 0))
  (fn-to-expr + String
    (str "Bar " a)
  )
)
```

```output
Foo Bar 99
Foo Bar 99
```

### Can't use parameterized types as units

```brix crash
(type-line + (of Foo T) (alias foo) (is Float))
(type-line + Bar (is Float))
(blank)

(main
  (print 1foo)
)
```

```output
Exception: Unit types must be numeric and unqualified
<no file:7>
      (print 1foo)
              ^^^
```

### Make sure units declared in class declaration are seen even if type was never used

```brix
(main
  (print (call (. Foo compare) Baz))
  (print (as 1foo Baz))
)

(blank)
(type-line + Foo (alias foo) (is Float))
(type-line + Bar (alias bar) (is 2foo))
(type-line + Bazoo (alias bazoo) (is Float))

(type + Baz (alias baz) (is Float)
  (fn-from-expr + (: foo Foo)
    ; Would never be called if 2foo unit is not seen
    (as (* 4 foo) Float)
  )
)

(unit + 1baz 1bazoo)
```

```output
[Fromverter Foo = Baz]
4
```

### Downcasting hack

```brix
(main
  (do-new
    (let box Box (call Square "bob"))
    (print (. (call (of (. box downcast) Square)) id))
    (print (. (call (of (. box downcast) Circle)) id))
    (on-failure failure
      (print failure)
    )
  )
)

(type + Box
  (prop + id String)
  (fn-do + This _
    (param id _)
  )
  (fn-do + (of downcast TargetBox) TargetBox
    (throw (call Panic "Invalid cast"))
  )
)

(type + Circle (is Box))

(type + Square (is Box)
  (fn-expr + (of downcast TargetBox) TargetBox
    (constraint is! TargetBox This)
    this
  )
)
```

```output
bob
Invalid cast
```

```brix
(main
  (let foo Foo (call Bar))
  (print (as foo Bar))
)

(blank)
(type-line + Foo)
(type-line + Bar (is Foo))
```

```output
[Object __source__:Bar]
```

### Downcasting

```brix
(main
  (let foo Foo (call Bar))
  (print (is foo Bar))
  (print (as foo Bar))
  (blank)
  (print (is foo Baz))
)

(blank)
(type-line + Foo)
(type-line + Bar (is Foo))
(type-line + Baz (is Foo))
```

```output
true
[Object __source__:Bar]
false
```

```brix
(main
  (let foo Foo (call Baz))
  (print (is foo Bar))
  (print (as foo Bar))
  (blank)
  (print (is foo Other))
)

(blank)
(type-line + Foo)
(type-line + Bar (is Foo))
(type-line + Baz (is Bar))
(type-line + Other (is Foo))
```

```output
true
[Object __source__:Bar]
false
```

## Or Typing

```brix
(main
  (call fetch (call Thing))
  (call fetch (call Bling))
)

(type + Thing
  (implements IFoo)
)

(type + Bling)

(type + IFoo (is Interface))

(type + IBar (is Interface))

(type + Donald (is IFoo IBar))

(fn-do + fetch _
  (param thing Donald)
  (print (tuple "duck" thing))
)

(fn-do + fetch _
  (param thing _)
  (print (tuple "fallthrough" thing))
)
```

```output
(duck, [Interface __source__:IFoo])
(fallthrough, [Object __source__:Bling])
```

```brix
(main
  (call fetch (call Thing))
)

(type + Thing (is Bling))

(type + Bling)

(type + Donald (is Bling Void))

(fn-do + fetch _
  (param thing Donald)
  (print (tuple "duck" thing))
)

(fn-do + fetch _
  (param thing _)
  (print (tuple "fallthrough" thing))
)
```

```output
(duck, [Object __source__:Bling])
```

```brix
(main
  (call fetch (call Thing))
)

(type + Thing)

(type + Donald (is Thing Void))

(fn-do + fetch _
  (param thing Donald)
  (print (tuple "duck" thing))
)

(fn-do + fetch _
  (param thing _)
  (print (tuple "fallthrough" thing))
)
```

```output
(duck, [Object __source__:Thing])
```

```brix
(main
  (call fetch (call Thing))
)

(type + Thing
  (fn-to-expr + IFoo
    (call FooThing)
  )
)

(type + FooThing
  (implements IFoo)
  (fn-expr + foopy _
    "foopy do"
  )
)

(type + IFoo (is Interface)
  (fn-expr + foopy String)
)

(type + IBar (is Interface))

(type + Donald (is IFoo IBar))

(fn-do + fetch _
  (param thing Donald)
  (print (tuple "duck" (call (. (as thing IFoo) foopy))))
)

(fn-do + fetch _
  (param thing _)
  (print (tuple "fallthrough" thing))
)
```

```output
(duck, foopy do)
```

```brix
(main
  (let thing (call Thing))
  (print (as thing Donald))
)

(type + Thing
  (fn-to-expr + Donald
    (call Goober)
  )
)

(type + Goober
  (implements Foo)
  (implements Bar)
)

(type + Foo (is Interface))

(type + Bar (is Interface))

(type + Donald (is Foo Bar))
```

```output
[Interface __source__:Foo]
```

```brix
(main
  (print (as (call Thing) Donald))
  (call fetch (call Thing))
)

(type + Thing
  (implements IFoo)
)

(type + IFoo (is Interface))

(type + Donald (is IFoo Void))

(fn-do + fetch _
  (param thing Donald)
  (print (tuple "duck" thing))
)
```

```output
[Interface __source__:IFoo]
(duck, [Interface __source__:IFoo])
```

```brix
(main
  (let bling (call Bling))
  (print (as bling Donald))
  (call fetch bling)
)

(type + Thing
  (implements IFoo)
  (implements IBar)
)

(type + Bling
  (fn-to-expr + Donald
    (call Thing)
  )
)

(type + IFoo (is Interface))

(type + IBar (is Interface))

(type + Donald (is IFoo IBar))

(fn-do + fetch _
  (param thing Donald)
  (print (tuple "duck" thing))
)
```

```output
[Interface __source__:IFoo]
(duck, [Interface __source__:IFoo])
```

### Choosing an or-type alternative never compiles the converters of the alternatives it rejects.

```brix
(main
  (call fetch (call Thing))
)

(type + Thing
  (implements IFoo)
  (fn-to-expr + IBar
    (call Missing)
  )
)

(type + IFoo (is Interface))

(type + IBar (is Interface))

(type + Donald (is IFoo IBar))

(fn-do + fetch _
  (param thing Donald)
  (print (tuple "duck" thing))
)
```

```output
(duck, [Interface __source__:IFoo])
```

### A type of alternatives called with a value gives the value as the alternative it converts to, as `as` does.

```brix
(main
  (let count (call Choice 3))
  (print (+ count 1))
  (let text (call Choice "x"))
  (print (++ text "y"))
  (let duck (call Donald (call Thing)))
  (print (. duck name))
)

(type + Choice (is Int String))

(type + IFoo (is Interface)
  (fn-expr + name String :get)
)

(type + IBar (is Interface))

(type + Donald (is IFoo IBar))

(type + Thing
  (implements IFoo)
  (prop + name _ (= "thing"))
)
```

```output
4
xy
thing
```

### A type of alternatives has no constructor of its own, so calling it with nothing is an error.

```brix compile-error
(main
  (let choice (call Choice))
)

(type + Choice (is Int String))
```

```output
Exception: Choice stands for any one of Int and String, so it has no constructor; give it a value of one of them, as in Choice(value)
<no file:3>
      (let choice (call Choice))
                        ^^^^^^
```

## Matching types with the same name

### Arena allocations expose ordinary pointer types.

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (let pointer (call (of (. arena allocate) U16) 2))
  (let mark (. arena mark))
  (blank)
  (print (type-of arena))
  (print (type-of pointer))
  (print (type-of mark))
)
```

```output
[TypeInfo Arena]
[TypeInfo CPointer'U16]
[TypeInfo ArenaMark]
```

### Arena handles and allocation pointers remain pointer-free through ordinary aggregates.

```brix
(import /core/arena)
(blank)

(main
  (let inner (call Arena))
  (let pointer (call (of (. inner allocate) U32) 1))
  (= ([] pointer 0) 42u32)
  (let bundle (call ArenaBundle inner pointer (. inner mark)))
  (let outer (call Arena))
  (let stored (call (of (. outer allocate) ArenaBundle) 1))
  (= ([] stored 0) bundle)
  (blank)
  (print (type-of bundle))
  (print ([] (. ([] stored 0) pointer) 0))
  (call (. outer release))
  (print ([] pointer 0))
  (call (. inner release))
)

(type + ArenaBundle (is Tuple)
  (prop + arena Arena)
  (prop + pointer (of CPointer U32))
  (prop + mark ArenaMark)
)
```

```output
[TypeInfo ArenaBundle]
42
42
```

```brix
(import /core/table)
(blank)

(main
  (let value (? (of Table Payload)) null)
  (print (== value null))
  (let constructed (call (of Table Payload)))
  (print (. constructed marker))
)

(type + Payload
  (prop + text _ (= "payload"))
)

(type + (of Table Item)
  (prop + marker _ (= 7))
)
```

```output
true
7
```

### A converter a subclass declares again answers for its objects, whatever type they are held as.

```brix
(main
  (let x A (call B))
  (print x)
  (let y A (call A))
  (print y)
  (let z B (call B))
  (print z)
  (let w A (call C))
  (print w)
  (print (str x " and " w))
)

(type + A
  (fn-to-expr + String
    "A's text"
  )
)

(type + B (is A)
  (fn-to-expr + String
    "B's text"
  )
)

(type + C (is B))
```

```output
B's text
A's text
B's text
B's text
B's text and B's text
```

### A type given as a value prints its name, and answers its name and key.

```brix
(main
  (let n 5)
  (let t (tuple 1 2.5))
  (let o (? Int) 4)
  (let b (call (of Box Int)))
  (let p (call Pair 1 2))
  (let s "text")
  (let thing Base (call Sub))
  (let named (call Named 1 2))
  (print (type-of n))
  (print (type-of t))
  (print (type-of o))
  (print (type-of b))
  (print (type-of p))
  (print (type-of s))
  (print (type-of thing))
  (print (type-of named))
  (print (. (type-of b) name))
  (print (. (type-of t) name))
  (print (. (type-of t) key))
  (print (. (type-of named) name))
  (print (. (type-of o) name))
  (print (. (type-of thing) name))
  (print (. (type-of thing) concrete-name))
  (print (call (of label (tuple Int String))))
  (print (call (of short (of Box Float))))
  (print (call (of label (? Float))))
)

(type + (of Box T))

(type + Pair (is Struct)
  (prop + a I32)
  (prop + b I32)
)

(type + Named (is (tuple (: x Int) (: y Int))))

(type + Base)

(type + Sub (is Base))

(fn-expr + (of label Item) String
  (str (. Item name) " / " (. Item key))
)

(fn-expr + (of short Item) String
  (. Item name)
)
```

```output
[TypeInfo I64]
[TypeInfo Tuple]
[TypeInfo Optional'I64]
[TypeInfo Box'I64]
[TypeInfo Pair]
[TypeInfo String]
[TypeInfo Base]
[TypeInfo Named]
Box'I64
Tuple
Tuple:((0:Class:core/number:I64), (1:Class:core/number:F64))
Named
Optional'I64
Base
Sub
Tuple / Tuple:((0:Class:core/number:I64), (1:Class:core/string:String))
Box'F64
Optional'F64 / Struct:optional(Class:core/number:F64, Class:core/number:I8)
```

### The concrete name and key of a tuple and of an optional.

```brix
(main
  (let t (tuple 1 2.5))
  (let o (? Int) 4)
  (print (. (type-of t) concrete-name))
  (print (. (type-of o) concrete-key))
  (print (. (type-of o) concrete-name))
  (print (. (type-of t) concrete-key))
)
```

```output
Tuple
Struct:optional(Class:core/number:I64, Class:core/number:I8)
Optional'I64
Tuple:((0:Class:core/number:I64), (1:Class:core/number:F64))
```

### The name and the key of each kind of type given as a type parameter.

```brix
(main
  (print (call (of label Int)))
  (print (call (of label Bool)))
  (print (call (of label Char)))
  (print (call (of label F32)))
  (print (call (of label String)))
  (print (call (of label (tuple Int String))))
  (print (call (of label (tuple (: a Int) (: b String)))))
  (print (call (of named Named)))
  (print (call (of named (of Gen Int))))
  (print (call (of named Pair)))
  (print (call (of label (? Int))))
  (print (call (of label (? String))))
  (print (call (of named (? Named))))
  (print (call (of named (of Box Int))))
  (print (call (of named (of Box (tuple Int Int)))))
  (print (call (of label (of CPointer U8))))
  (print (call (of label Pointer)))
  (print (call (of named Color)))
  (print (call (of named Shape)))
  (print (call (of label (list Int))))
  (print (call (of label Size)))
  (print (call (of named Meters)))
)

(type + (of Box T))

(type + Pair (is Struct)
  (prop + a I32)
  (prop + b I32)
)

(type + Named (is (tuple (: x Int) (: y Int))))

(type + (of Gen T) (is (tuple (: x T) (: y T))))

(type + Color (is Union)
  (variant red)
  (variant green)
)

(type + Shape (is Union)
  (variant circle Float)
  (variant square Int)
)

(type + Meters (alias m) (is Float))

(fn-expr + (of label Item) String
  (str (. Item name) " / " (. Item key))
)

(fn-expr + (of named Item) String
  (. Item name)
)
```

```output
I64 / Class:core/number:I64
Bool / Class:core/number:Bool
Char / Class:core/number:Char
F32 / Class:core/number:F32
String / Class:core/string:String
Tuple / Tuple:((0:Class:core/number:I64), (1:Class:core/string:String))
Tuple / Tuple:((a:Class:core/number:I64), (b:Class:core/string:String))
Named
Gen'I64
Pair
Optional'I64 / Struct:optional(Class:core/number:I64, Class:core/number:I8)
Optional'String / Struct:optional(Class:core/string:String, Class:core/number:I8)
Optional'Named
Box'I64
Box'Tuple
CPointer'U8 / Class:core:CPointer(Class:core/number:U8)
Pointer / Class:core:Pointer
Color
Shape
List'I64'GCBacking / Class:core/list:List(Class:core/number:I64, Class:core/backing:GCBacking)
Size / Class:core/range:Size
Meters
```

### A tuple with no name is named Tuple and an optional is keyed as the struct that holds it, wherever they are nested.

```brix
(main
  (print (call (of named (of Box (? Int)))))
  (print (call (of label (tuple (? Int) String))))
  (print (call (of label (? (tuple (? Int))))))
  (print (call (of named (of Box (of Box (tuple Int Int))))))
  (print (call (of named (of Pair (? Int) (tuple Int Int)))))
  (print (call (of label (tuple (: a (? Int)) (: b (tuple Int Int))))))
  (print (call (of label (list (? Int)))))
  (print (call (of label (list (tuple Int (? Int))))))
  (let box Base (call (of Box (tuple Int Int))))
  (print (. (type-of box) concrete-name))
  (print (type-of box))
  (let deep Base (call (of Box (? Int))))
  (print (. (type-of deep) concrete-name))
)

(type + Base)

(type + (of Box T) (is Base))

(type + (of Pair A B))

(fn-expr + (of label Item) String
  (str (. Item name) " / " (. Item key))
)

(fn-expr + (of named Item) String
  (. Item name)
)
```

```output
Box'Optional'I64
Tuple / Tuple:((0:Struct:optional(Class:core/number:I64, Class:core/number:I8)), (1:Class:core/string:String))
Optional'Optional'I64 / Struct:optional(Struct:optional(Class:core/number:I64, Class:core/number:I8), Class:core/number:I8)
Box'Box'Tuple
Pair'Optional'I64'Tuple
Tuple / Tuple:((a:Struct:optional(Class:core/number:I64, Class:core/number:I8)), (b:Tuple:((0:Class:core/number:I64), (1:Class:core/number:I64))))
List'Optional'I64'GCBacking / Class:core/list:List(Struct:optional(Class:core/number:I64, Class:core/number:I8), Class:core/backing:GCBacking)
List'Tuple'GCBacking / Class:core/list:List(Tuple:((0:Class:core/number:I64), (1:Struct:optional(Class:core/number:I64, Class:core/number:I8))), Class:core/backing:GCBacking)
Box'Tuple
[TypeInfo Base]
Box'Optional'I64
```

### The size of a type given as a type parameter is the bytes a value of it is held in.

```brix
(main
  (print (call (of choose Int)))
  (print (call (of choose (tuple Int Int Int Int Int))))
  (print (call (of choose String)))
  (print (call (of choose Thing)))
  (print (call (of choose Pair)))
  (print (call (of choose (? Int))))
  (print (call (of choose (tuple Int String))))
  (print (call (of choose Bool)))
  (print (call (of choose F64)))
  (print (call (of choose (tuple U8 U8 U8))))
)

(type + Thing
  (prop + a _ (= 0))
  (prop + b _ (= 0))
  (prop + c _ (= 0))
)

(type + Pair (is Struct)
  (prop + a I64)
  (prop + b U8)
)

(fn-do + (of choose Item) Size
  (return (. Item size))
)
```

```output
8
40
8
8
16
16
16
1
8
3
```

### A variable of a type that is one of several interfaces holds the one its value is

```brix
(main
  (let first Donald (call Thing))
  (let second Donald (call Bling))
  (let third Donald (call Both))
  (print (tuple first (call (. first name))))
  (print (tuple second (call (. second name))))
  (print (tuple third (call (. third name))))
)

(type + Thing
  (implements IFoo)
  (fn-expr + name String
    "thing"
  )
)

(type + Bling
  (implements IBar)
  (fn-expr + name String
    "bling"
  )
)

(type + Both
  (implements IBar)
  (implements IFoo)
  (fn-expr + name String
    "both"
  )
)

(type + IFoo (is Interface)
  (fn-expr + name String)
)

(type + IBar (is Interface)
  (fn-expr + name String)
)

(type + Donald (is IFoo IBar))
```

```output
([Interface __source__:IFoo], thing)
([Interface __source__:IBar], bling)
([Interface __source__:IFoo], both)
```

### A parameter of a type that is one of several interfaces is the one each argument is

```brix
(main
  (call show (call Thing))
  (call show (call Bling))
  (call show (call Both))
)

(type + Thing
  (implements IFoo)
  (fn-expr + name String
    "thing"
  )
)

(type + Bling
  (implements IBar)
  (fn-expr + name String
    "bling"
  )
)

(type + Both
  (implements IBar)
  (implements IFoo)
  (fn-expr + name String
    "both"
  )
)

(type + IFoo (is Interface)
  (fn-expr + name String)
)

(type + IBar (is Interface)
  (fn-expr + name String)
)

(type + Donald (is IFoo IBar))

(fn-do + show _
  (param value Donald)
  (print (tuple value (call (. value name))))
)
```

```output
([Interface __source__:IFoo], thing)
([Interface __source__:IBar], bling)
([Interface __source__:IFoo], both)
```

### A function that returns one of several interfaces returns the one its value is

```brix
(main
  (print (call pick true))
  (print (call pick false))
  (let picked (call pick true))
  (print (call (. picked name)))
)

(type + Thing
  (implements IFoo)
  (fn-expr + name String
    "thing"
  )
)

(type + Bling
  (implements IBar)
  (fn-expr + name String
    "bling"
  )
)

(type + Both
  (implements IBar)
  (implements IFoo)
  (fn-expr + name String
    "both"
  )
)

(type + IFoo (is Interface)
  (fn-expr + name String)
)

(type + IBar (is Interface)
  (fn-expr + name String)
)

(type + Donald (is IFoo IBar))

(fn-do + pick Donald
  (param first Bool)
  (if first
    (return (call Thing))
  )
  (return (call Thing))
)
```

```output
[Interface __source__:IFoo]
[Interface __source__:IFoo]
thing
```

### Two parameters of a type that is one of several interfaces choose alone

```brix
(main
  (call show (call Thing) (call Bling))
  (call show (call Bling) (call Thing))
  (call show (call Both) (call Both))
)

(type + Thing
  (implements IFoo)
  (fn-expr + name String
    "thing"
  )
)

(type + Bling
  (implements IBar)
  (fn-expr + name String
    "bling"
  )
)

(type + Both
  (implements IBar)
  (implements IFoo)
  (fn-expr + name String
    "both"
  )
)

(type + IFoo (is Interface)
  (fn-expr + name String)
)

(type + IBar (is Interface)
  (fn-expr + name String)
)

(type + Donald (is IFoo IBar))

(fn-do + show _
  (param left Donald)
  (param right Donald)
  (print (tuple (call (. left name)) (call (. right name))))
  (print (tuple left right))
)
```

```output
(thing, bling)
([Interface __source__:IFoo], [Interface __source__:IBar])
(bling, thing)
([Interface __source__:IBar], [Interface __source__:IFoo])
(both, both)
([Interface __source__:IFoo], [Interface __source__:IFoo])
```

### A list and a method parameter of a type that is one of several interfaces

```brix
(main
  (let items (list (as (call Thing) Donald) (as (call Thing) Donald)))
  (for (in item items)
    (print (call (. item name)))
  )
  (let box (call Box))
  (call (. box keep) (call Thing))
  (call (. box keep) (call Bling))
)

(type + Thing
  (implements IFoo)
  (fn-expr + name String
    "thing"
  )
)

(type + Bling
  (implements IBar)
  (fn-expr + name String
    "bling"
  )
)

(type + Both
  (implements IBar)
  (implements IFoo)
  (fn-expr + name String
    "both"
  )
)

(type + IFoo (is Interface)
  (fn-expr + name String)
)

(type + IBar (is Interface)
  (fn-expr + name String)
)

(type + Donald (is IFoo IBar))

(type + Box
  (fn-do + keep _
    (param value Donald)
    (print (tuple "kept" (call (. value name))))
  )
)
```

```output
thing
thing
(kept, thing)
(kept, bling)
```

### A do function that returns one of several interfaces returns the second

```brix
(main
  (print (call pick))
)

(type + Thing
  (implements IFoo)
  (fn-expr + name String
    "thing"
  )
)

(type + Bling
  (implements IBar)
  (fn-expr + name String
    "bling"
  )
)

(type + IFoo (is Interface)
  (fn-expr + name String)
)

(type + IBar (is Interface)
  (fn-expr + name String)
)

(type + Donald (is IFoo IBar))

(fn-do + pick Donald
  (return (call Bling))
)
```

```output
[Interface __source__:IBar]
```

### An expression function that returns one of several interfaces returns the second

```brix
(main
  (print (call pick))
)

(type + Thing
  (implements IFoo)
  (fn-expr + name String
    "thing"
  )
)

(type + Bling
  (implements IBar)
  (fn-expr + name String
    "bling"
  )
)

(type + IFoo (is Interface)
  (fn-expr + name String)
)

(type + IBar (is Interface)
  (fn-expr + name String)
)

(type + Donald (is IFoo IBar))

(fn-expr + pick Donald
  (call Bling)
)
```

```output
[Interface __source__:IBar]
```

### A converter to a type that is one of several interfaces gives the one it returns

```brix
(main
  (let source (call Source))
  (print (as source Donald))
  (let held Donald source)
  (print (call (. held name)))
  (call show source)
  (call show (call Wrapper))
  (call show (call Thing))
)

(type + Thing
  (implements IFoo)
  (fn-expr + name String
    "thing"
  )
)

(type + Both
  (implements IBar)
  (implements IFoo)
  (fn-expr + name String
    "both"
  )
)

(type + IFoo (is Interface)
  (fn-expr + name String)
)

(type + IBar (is Interface)
  (fn-expr + name String)
)

(type + Donald (is IFoo IBar))

(type + Source
  (fn-to-expr + Donald
    (call Thing)
  )
)

(type + Wrapper
  (fn-to-expr + Donald
    (call Both)
  )
)

(fn-do + show _
  (param value Donald)
  (print (tuple "shown" value (call (. value name))))
)
```

```output
[Interface __source__:IFoo]
thing
(shown, [Interface __source__:IFoo], thing)
(shown, [Interface __source__:IFoo], both)
(shown, [Interface __source__:IFoo], thing)
```

### An overload of a type that is one of several interfaces and one of a number

```brix
(main
  (call fetch (call Thing))
  (call fetch (call Bling))
  (call fetch 5)
)

(type + Thing
  (implements IFoo)
)

(type + Bling
  (implements IBar)
)

(type + IFoo (is Interface))

(type + IBar (is Interface))

(type + Donald (is IFoo IBar))

(fn-do + fetch _
  (param thing Donald)
  (print "duck")
  (print thing)
)

(fn-do + fetch _
  (param thing Int)
  (print "int")
)
```

```output
duck
[Interface __source__:IFoo]
duck
[Interface __source__:IBar]
int
```

### A value declared and cast as a type that is one of several interfaces

```brix
(main
  (let thing (call Thing))
  (let donald Donald thing)
  (print donald)
  (let other (as (call Bling) Donald))
  (print other)
)

(type + Thing
  (implements IFoo)
)

(type + Bling
  (implements IBar)
)

(type + IFoo (is Interface))

(type + IBar (is Interface))

(type + Donald (is IFoo IBar))
```

```output
[Interface __source__:IFoo]
[Interface __source__:IBar]
```

### A converter declared to give one of several types converts to each of them: an argument and a declaration of one take what it gives, and a function that takes that one is chosen.

```brix
(main
  (call take (call Source))
  (let held IBar (call Source))
  (print held)
  (call only (call Source))
)

(type + Barred
  (implements IBar)
)

(type + IFoo (is Interface))

(type + IBar (is Interface))

(type + Donald (is IFoo IBar))

(type + Source
  (fn-to-expr + Donald
    (call Barred)
  )
)

(fn-do + take _
  (param value IBar)
  (print (tuple "bar" value))
)

(fn-do + take _
  (param value Donald)
  (print (tuple "donald" value))
)

(fn-do + only _
  (param value IBar)
  (print (tuple "only" value))
)
```

```output
(bar, [Interface __source__:IBar])
[Interface __source__:IBar]
(only, [Interface __source__:IBar])
```

```brix crash
(main
  (let value (? Arena) null)
)
```

```output
Exception: Type "Arena" not found; it is declared in /core/arena, which this module does not import. Add the import /core/arena
<no file:3>
      (let value (? Arena) null)
                    ^^^^^
```

```brix crash
(main
  (let value (? (of Table Int)) null)
)
```

```output
Exception: Type "Table" not found; it is declared in /core/table and /tui/table, which this module does not import. Import one of them
<no file:3>
      (let value (? (of Table Int)) null)
                    ^^^^^^^^^^^^^^
```

```brix crash
(main
  (let value (? (of ArrayBuilder Int)) null)
)
```

```output
Exception: Type "ArrayBuilder" not found; it is declared in /core/array-builder, which this module does not import. Add the import /core/array-builder
<no file:3>
      (let value (? (of ArrayBuilder Int)) null)
                    ^^^^^^^^^^^^^^^^^^^^^
```

```brix crash
(main
  (print (== (call later) null))
)

(fn-expr _ later (? Arena)
  null
)
```

```output
Exception: Type "Arena" not found; it is declared in /core/arena, which this module does not import. Add the import /core/arena
<no file:6>
    (fn-expr _ later (? Arena)
                        ^^^^^
```

```brix crash
(main
  (print (call take null))
)

(fn-expr _ take Bool
  (param value (? Arena))
  (== value null)
)
```

```output
Exception: Type "Arena" not found; it is declared in /core/arena, which this module does not import. Add the import /core/arena
<no file:7>
      (param value (? Arena))
                      ^^^^^
```

```brix crash
(prop _ size (? Arena) (= null))
(blank)

(main
  (print (== size null))
)
```

```output
Exception: Type "Arena" not found; it is declared in /core/arena, which this module does not import. Add the import /core/arena
<no file:2>
    (prop _ size (? Arena) (= null))
                    ^^^^^
```

```brix crash
(main
  (let value 3)
  (print (is value Arena))
)
```

```output
Exception: Type "Arena" not found; it is declared in /core/arena, which this module does not import. Add the import /core/arena
<no file:4>
      (print (is value Arena))
                       ^^^^^
```

```brix crash
(main
  (let found (call (of List Arena)))
  (print (. found length))
)
```

```output
Exception: Type "Arena" not found; it is declared in /core/arena, which this module does not import. Add the import /core/arena
<no file:3>
      (let found (call (of List Arena)))
                                ^^^^^
```

```brix crash
(main
  (let value 3)
  (match value
    (case (: held Arena)
      (print "arena")
    )
    (default
      (print "other")
    )
  )
)
```

```output
Exception: Type "Arena" not found; it is declared in /core/arena, which this module does not import. Add the import /core/arena
<no file:5>
        (case (: held Arena)
                      ^^^^^
```

```brix crash
(main
  (let b (call (of Box Int) 1))
  (let x (list Int) b)
  (print "never")
)

(type + (of Box Item)
  (prop + k Item)
  (fn-do + This _
    (param k _)
  )
  (fn-to-expr + (of List Item)
    (throw (call Panic "conv"))
  )
)
```

```stderr
Unhandled panic: conv
<no file:14>

Task 1

Stack trace:
  0  __source__:Box:to List'Item2 no file:14:5
  1  __source__:main no file:4:10
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

### A tuple of one field whose type its value gives is made from a value of that type, not converted by its `from`.

```brix
(main
  (print (. (call Wrapped "abc") name))
  (let w Wrapped "def")
  (print (. w name))
)

(type + Wrapped (is Tuple)
  (prop + name _ (= "none"))
  (fn-from-do + (: value String)
    (print (str "from " value))
    (return (call Wrapped (str "w-" value)))
  )
)
```

```output
abc
from def
w-def
```

### A member read from a name that names no type is reported where the name is written.

```brix crash
(main
  (print (. Missing word))
)
```

```output
Exception: Type "Missing" not found
<no file:3>
      (print (. Missing word))
                ^^^^^^^
```

### A type written after a function's name that names no type is reported where it is written.

```brix crash
(main
  (print (call (of convert Missing) 3))
)

(fn-expr + (of convert Unit) Int
  (param value Int)
  value
)
```

```output
Exception: Type "Missing" not found
<no file:3>
      (print (call (of convert Missing) 3))
                               ^^^^^^^
```

### A type a value is cast to that names no type is reported where it is written.

```brix crash
(main
  (let id (as 3 Missing))
  (print id)
)
```

```output
Exception: Type "Missing" not found
<no file:3>
      (let id (as 3 Missing))
                    ^^^^^^^
```

### A parameter's type that names no type is reported where it is written when a call reads it.

```brix crash
(main
  (print (call describe 3))
)

(fn-do + describe Int
  (param value Missing)
  (return 1)
)
```

```output
Exception: Type "Missing" not found
<no file:7>
      (param value Missing)
                   ^^^^^^^
```

### A method overriding a base's whose parameter's type names no type is reported where it is written, not left to the base's.

```brix crash
(main
  (print (call ask (call Thing)))
)

(fn-do + ask String
  (param base Base)
  (return (call (. base describe) 3))
)

(type + Base
  (fn-do + describe String
    (param value Int)
    (return "base")
  )
)

(type + Thing (is Base)
  (fn-do + describe String
    (param value Missing)
    (return "thing")
  )
)
```

```output
Exception: Type "Missing" not found
<no file:20>
        (param value Missing)
                     ^^^^^^^
```

### A method parameter's type that names no type is reported where it is written, though a function of the module has the method's name.

```brix crash
(main
  (print (call (. (call Gauge) reading)))
)

(type + Gauge
  (fn-expr + reading Int
    (call scaled 3)
  )
  (fn-expr _ scaled Int
    (param value Missing)
    1
  )
)

(fn-expr + scaled Int
  (param value Int)
  2
)
```

```output
Exception: Type "Missing" not found
<no file:11>
        (param value Missing)
                     ^^^^^^^
```

### A constructor parameter's type that names no type is reported where it is written.

```brix crash
(main
  (print (. (call Gauge 3) level))
)

(type + Gauge
  (prop + level Int)
  (fn-do + This _
    (param value Missing)
  )
)
```

```output
Exception: Type "Missing" not found
<no file:9>
        (param value Missing)
                     ^^^^^^^
```

### A local whose written type names nothing is an error where the type is written.

```brix crash
(main
  (let x Nonexistent)
  (= x 3)
  (print x)
)
```

```output
Exception: Type "Nonexistent" not found
<no file:3>
      (let x Nonexistent)
             ^^^^^^^^^^^
```

### A class and a generic class may share a name in one module, whichever is declared first. Construction, a written type, and markup each choose the one with as many type parameters as they give, and the class may extend an instance of the generic and override its methods.

```brix
(main
  (let plain (call Slot "plain"))
  (let typed (call (of Slot Int) "typed"))
  (print (call describe plain))
  (print (call describe typed))
  (print (call (. plain parse) "1"))
  (print (call (. typed parse) "2"))
  (let held (of Slot String) plain)
  (print (call (. held parse) "3"))
  (<> Slot (id: marked) (text: "marked"))
  (<> (of Slot Int) (id: counted) (text: "counted"))
  (print (call describe marked))
  (print (call describe counted))
  (let box Box (call Box))
  (print (call (. box label)))
  (print (call (. (call (of Box Int)) label)))
)

(type + Box
  (fn-expr + label String
    "plain box"
  )
)

(type + (of Box T)
  (fn-expr + label String
    "typed box"
  )
)

(type + Named
  (prop + name String)
  (fn-do + This _
    (param name String)
  )
)

(type + (of Slot T) (is Named)
  (fn-do + This _
    (param name String)
    (call super name)
  )
  (fn-expr + parse String
    (param text String)
    (str "typed " text)
  )
)

(type + Slot (is (of Slot String))
  (fn-do + This _
    (param name String)
    (call super name)
  )
  (fn-expr + parse String
    (param text String)
    (str "plain " text)
  )
)

(fn-expr + describe String
  (param slot Slot)
  (str "Slot " (. slot name))
)

(fn-expr + describe String
  (param slot (of Slot Int))
  (str "Slot'Int " (. slot name))
)
```

```output
Slot plain
Slot'Int typed
plain 1
typed 2
plain 3
Slot marked
Slot'Int counted
plain box
typed box
```
