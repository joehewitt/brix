# Type info

```yumi
-- type-of 42
```

```output
[TypeInfo I64]
```

```yumi
. ti = I32
-- ti
-- ti.name
```

```output
[TypeInfo I32]
I32
```

### Long fixed-width names are compatibility aliases for the short canonical names.

```yumi
. ti = Int32
-- ti
-- ti.name
-- Int32 == I32
-- UInt64 == U64
-- Float32 == F32
```

```output
[TypeInfo I32]
I32
true
true
true
```

```yumi
-- I1.size
-- I8.size
-- U8.size
-- I16.size
-- U16.size
-- I32.size
-- U32.size
-- I64.size
-- U64.size
-- F32.size
-- F64.size
-- Bool.size
-- Char.size
```

```output
1
1
1
2
2
4
4
8
8
4
8
1
4
```

```yumi
-- I32.base
```

```output
[TypeInfo Number]
```

```yumi crash
-- Number.base
```

```output
Exception: No base type
<no file:2>
    -- Number.base
       ^^^^^^^^^^^
```

```yumi
-- Bar.base

━━━━━━━━━━━━━━━━━━━━
+ Foo

━━━━━━━━━━━━━━━━━━━━
+ Bar is Foo
```

```output
[TypeInfo Foo]
```

### Runtime class identity

```yumi
-- (type-of Thing()).name
-- (type-of Thing()).concrete-name
-- type-of Thing()

━━━━━━━━━━━━━━━━━━━━
+ Thing
```

```output
Thing
Thing
[TypeInfo Thing]
```

### Static and concrete identities for a base-typed class value

```yumi
. thing Base = Child()
-- (type-of thing).name
-- (type-of thing).concrete-name
-- type-of thing
-- (type-of thing).compare(Base)
-- (type-of thing).key == Base.key
-- (type-of thing).key == Child.key
-- (type-of thing).concrete-key == Child.key

━━━━━━━━━━━━━━━━━━━━
+ Base

━━━━━━━━━━━━━━━━━━━━
+ Child is Base
```

```output
Base
Child
[TypeInfo Base]
[Same Base = Base]
true
false
true
```

### Static and concrete identities for an interface value

```yumi
. proto = Thing() as Proto
-- (type-of proto).name
-- (type-of proto).concrete-name
-- type-of proto
proto.foo()

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ foo

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Proto

────────────────────
+ do foo

-- "ok"
```

```output
Proto
Thing
[TypeInfo Proto]
ok
```

### Concrete identity survives interface-to-interface subtable casts

```yumi
. child = Thing() as Child
. base = child as Base
-- (type-of child).name
-- (type-of child).concrete-name
-- (type-of base).name
-- (type-of base).concrete-name
base.base()

━━━━━━━━━━━━━━━━━━━━
+ Base is Interface

────────────────────
+ base

━━━━━━━━━━━━━━━━━━━━
+ Child is Interface
    is Base

────────────────────
+ child

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Child

────────────────────
+ do base

-- "base"

────────────────────
+ do child

-- "child"
```

```output
Child
Thing
Base
Thing
base
```

## Comparing types

```yumi crash
-- I32.compare(1)
```

```output
Exception: Invalid argument
<no file:2>
    -- I32.compare(1)
                   ^
```

```yumi
-- Foo.compare(Bar)
-- Bar.compare(Foo)

+ Foo
+ Bar
```

```output
[Illegal Foo = Bar]
[Illegal Bar = Foo]
```

```yumi
-- I32.compare(I32)
```

```output
[Same I32 = I32]
```

```yumi
-- Foo.compare(Foo)
-- Foo.compare(Bar)
-- Bar.compare(Foo)

+ Foo (foo) is I32
+ Bar (bar) is  2foo
```

```output
[Same Foo = Foo]
[Equal Foo = Bar]
[Equal Bar = Foo]
```

```yumi
-- I32.compare(I64)
-- I32.compare(F32)
```

```output
[SideCast I32 = I64]
[SideCast I32 = F32]
```

```yumi
-- Foo.compare(I32)
-- Bar.compare(I32)
-- Bar.compare(Foo)
-- Foo.compare(Bar)

+ Foo is I32
+ Bar is Foo
```

```output
[SideCast Foo = I32]
[SideCast Bar = I32]
[SideCast Bar = Foo]
[SideCast Foo = Bar]
```

```yumi
-- Optional'I32.compare(Bool)
-- I32.compare(Optional'I32)
```

```output
[Converter Optional'I32 = Bool]
[SideCast I32 = Optional'I32]
```

```yumi
. t = (1,2)
. t2 = (1.0, 2.0)
-- (type-of t).compare(type-of t2)
```

```output
[FinalCast Tuple = Tuple]
```

```yumi
-- Foo.compare(Foo)
-- Foo.compare(Bar)
-- Bar.compare(Foo)

+ Foo
+ Bar is Foo
```

```output
[Same Foo = Foo]
[DownCast Foo = Bar]
[UpCast Bar = Foo]
```

```yumi
-- Foo.compare(Foo)
-- Foo.compare(Bar)
-- Bar.compare(Foo)

+ Foo

━━━━━━━━━━━━━━━━━━━━
+ Bar

────────────────────
+ to Foo

Foo()
```

```output
[Same Foo = Foo]
[Illegal Foo = Bar]
[Converter Bar = Foo]
```

```yumi
-- Bar.compare(Foo)
-- Foo.compare(Bar)

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ from bar Bar

This()

━━━━━━━━━━━━━━━━━━━━
+ Bar
```

```output
[Fromverter Bar = Foo]
[Illegal Foo = Bar]
```

### Show fromverter takes precedence over converter

```yumi
-- Bar.compare(Foo)
-- Foo.compare(Bar)

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ from bar Bar

This()

━━━━━━━━━━━━━━━━━━━━
+ Bar

────────────────────
+ to Foo

Foo()
```

```output
[Fromverter Bar = Foo]
[Illegal Foo = Bar]
```

```yumi
-- Foo.compare-unified(Bar)
-- Bar.compare-unified(Foo)

+ Base
+ Foo is Base
+ Bar is Base
```

```output
[CommonUpCast Foo = Bar]
[CommonUpCast Bar = Foo]
```

```yumi
-- Foo.compare-unified(Bar)
-- Bar.compare-unified(Foo)

+ Proto is Interface

━━━━━━━━━━━━━━━━━━━━
+ Foo
    is Proto

━━━━━━━━━━━━━━━━━━━━
+ Bar
    is Proto
```

```output
[CommonInterface Foo = Bar]
[CommonInterface Bar = Foo]
```

```yumi
-- Foo.compare-unified(Bar)
-- Bar.compare-unified(Foo)

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ to Baz

Baz()

━━━━━━━━━━━━━━━━━━━━
+ Bar

────────────────────
+ to Baz

Baz()

━━━━━━━━━━━━━━━━━━━━
+ Baz
```

```output
[CommonConverter Foo = Bar]
[CommonConverter Bar = Foo]
```

```yumi
-- Foo.compare-unified(Baz)
-- Foo.compare-unified(Bazoo)

+ Baz (baz) is Int
+ Bazoo (bazoo) is Int

+ 1baz = 2bazoo

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ to Baz

Baz()
```

```output
[Converter Foo = Baz]
[CommonEquality Foo = Bazoo]
```

```yumi
-- String.compare(Bar)
-- Bar.compare(String)
-- Foo.compare(String)

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ to Baz

Baz()

━━━━━━━━━━━━━━━━━━━━
+ Bar

────────────────────
+ to Baz

Baz()

────────────────────
+ to String

"Bar"

━━━━━━━━━━━━━━━━━━━━
+ Baz
```

```output
[Illegal String = Bar]
[Converter Bar = String]
[StringCast Foo = String]
```

```yumi
-- String.compare(CPointer'Char)
```

```output
[SideCast String = CPointer'Char]
```

```yumi
-- Foo.compare(String)

+ Foo
```

```output
[StringCast Foo = String]
```

## Casting

### Illegal comparison

```yumi crash
+ Foo
+ Bar

-- Foo.compare(Bar).cast(Foo())
```

```output
Exception: Expected Bar here, but this is Foo, and there is no conversion between them
<no file:5>
    -- Foo.compare(Bar).cast(Foo())
       ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Same comparison

```yumi
-- Foo.compare(Foo).cast(Foo())

+ Foo
```

```output
[Object __source__:Foo]
```

### Equality comparison

```yumi
-- Foo.compare(Foo).cast(1foo)
-- Foo.compare(Bar).cast(1foo)
-- Bar.compare(Foo).cast(1bar)

+ Foo (foo) is Float
+ Bar (bar) is 2foo
```

```output
1
0.5
2
```

### SideCast to Number comparison

```yumi
-- I32.compare(I64).cast(4) as UnitString
-- I32.compare(F32).cast(4) as UnitString
```

```output
4i
4f32
```

### Cast numeric subtypes to base builting numeric types

```yumi
-- Foo.compare(I32).cast(1foo) as UnitString
-- Bar.compare(I32).cast(1bar) as UnitString

+ Foo (foo) is I32
+ Bar (bar) is Foo
```

```output
1i32
1i32
```

```yumi
-- Bar.compare(Foo).cast(1bar)

+ Foo (foo) is I32
+ Bar (bar) is Foo
```

```output
1
```

### SideCast to Bool comparison

```yumi
-- I32.compare(Bool).cast(0)
-- I32.compare(Bool).cast(1)
```

```output
false
true
```

### SideCast Optional comparison

```yumi
. a I32?
. b I32? = 1
-- Optional'I32.compare(Bool).cast(a)
-- Optional'I32.compare(Bool).cast(b)
```

```output
false
true
```

### SideCast comparison

```yumi
-- type-of CPointer'Char.compare(String).cast(Char[1].data)
-- type-of String.compare(CPointer'Char).cast("foo")
-- String.compare(CPointer'Char8).cast("foo")
```

```output
[TypeInfo String]
[TypeInfo CPointer'Char]
foo
```

### UpCast comparison

```yumi
-- Foo.compare(Foo).cast(Foo())
-- Bar.compare(Foo).cast(Bar())

+ Foo
+ Bar is Foo
```

```output
[Object __source__:Foo]
[Object __source__:Foo]
```

### Converter comparisonType

```yumi
-- Bar.compare(Foo).cast(Bar())

━━━━━━━━━━━━━━━━━━━━
+ Foo

━━━━━━━━━━━━━━━━━━━━
+ Bar

────────────────────
+ to Foo

Foo()
```

```output
[Object __source__:Foo]
```

### Fromverter comparisonType

```yumi
-- Bar.compare(Foo).cast(Bar())

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ from bar Bar

This()

━━━━━━━━━━━━━━━━━━━━
+ Bar
```

```output
[Object __source__:Foo]
```

### StringCast comparison

```yumi
-- type-of I32.compare(String).cast(4.2)
```

```output
[TypeInfo String]
```

```yumi
-- Foo.compare(String).cast(Foo())

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ to String

"foo"
```

```output
foo
```

## Unifying

### Unifying to common equality

```yumi
-- Foo.compare-unified(Bazoo).unify(Foo())

+ Baz (baz) is Float
+ Bazoo (bazoo) is Float

+ 1baz = 2bazoo

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ to Baz

1baz
```

```output
2
```

### Unifying to common converter

```yumi
-- Foo.compare-unified(Bar).unify(Foo())
-- Bar.compare-unified(Foo).unify(Bar())

━━━━━━━━━━━━━━━━━━━━
+ Foo

────────────────────
+ to Baz

Baz()

━━━━━━━━━━━━━━━━━━━━
+ Bar

────────────────────
+ to Baz

Baz()

━━━━━━━━━━━━━━━━━━━━
+ Baz
```

```output
[Object __source__:Baz]
[Object __source__:Baz]
```

### Unify to common base class

```yumi
-- type-of Foo.compare-unified(Bar).unify(Foo())
-- type-of Bar.compare-unified(Foo).unify(Bar())

+ Base
+ Foo is Base
+ Bar is Base
```

```output
[TypeInfo Base]
[TypeInfo Base]
```

## Comparison operators

```yumi
-- I32 == I32
-- I32 == I64
```

```output
true
false
```

```yumi
-- Optional'I32 == Optional'I32
-- Optional'I32 == Optional'I64
```

```output
true
false
```

### A number type answers what kind of number it is.

```yumi
-- I8.is-signed
-- U8.is-signed
-- F64.is-signed
-- Bool.is-signed
-- Char.is-signed
-- String.is-signed
-- I8.is-integer
-- F32.is-integer
-- Bool.is-integer
-- Char.is-integer
-- String.is-integer
-- F32.is-number
-- Bool.is-number
-- Char.is-number
-- String.is-number
-- I32.is-tuple
-- String.is-tuple
-- I32.is-struct
-- I32.read-only
-- String.read-only
```

```output
true
false
true
false
false
false
true
false
true
true
false
true
true
true
false
false
false
false
true
true
```

### A number type answers its least and its greatest value.

```yumi
-- I8.max
-- I8.min
-- U8.max
-- U8.min
-- I16.max
-- I16.min
-- U16.max
-- I32.max
-- I32.min
-- U32.max
-- I64.max
-- I64.min
-- U64.max
-- U64.min
-- Size.max
-- Int.max
-- Byte.max
-- Index.max
-- Char.max as U32
-- Char.min as U32
-- F64.max
-- F64.min
-- F32.max
-- F32.min
-- Bool.max
-- Bool.min
```

```output
127
-128
255
0
32767
-32768
65535
2147483647
-2147483648
4294967295
9223372036854775807
-9223372036854775808
18446744073709551615
0
9223372036854775807
9223372036854775807
255
9223372036854775807
4294967295
0
1.7976931348623157e+308
-1.7976931348623157e+308
3.4028235e+38
-3.4028235e+38
true
false
```

### An integer type answers its width in bits, and a number type its size in bytes.

```yumi
-- I8.bit-width
-- U8.bit-width
-- I16.bit-width
-- I32.bit-width
-- U64.bit-width
-- Size.bit-width
-- Char.bit-width
-- Int.bit-width
-- Bool.bit-width
-- I8.size
-- I16.size
-- I32.size
-- I64.size
-- F32.size
-- F64.size
-- Bool.size
-- Char.size
```

```output
8
8
16
32
64
64
32
64
1
1
2
4
8
4
8
1
4
```

### A type answers its name and its key.

```yumi
-- Int.name
-- I8.name
-- U64.name
-- F64.name
-- Float.name
-- Char.name
-- Bool.name
-- String.name
-- Size.name
-- Index.name
-- Byte.name
-- I32.key
-- String.key
-- Bool.key
-- I32.concrete-name
-- I32.concrete-key
-- Char.concrete-name
-- I8.name ++ "!"
-- I8.name.size
```

```output
I64
I8
U64
F64
F64
Char
Bool
String
Size
Index
U8
Class:core/number:I32
Class:core/string:String
Class:core/number:Bool
I32
Class:core/number:I32
Char
I8!
2
```

### A generic function reads the constants of the type it is given.

```yumi
-- kind(1)
-- kind(1.5)
-- kind(2u8)
-- kind(true)
-- top(1i8)
-- top(1u16)
-- bottom(1i64)
-- label(7)
-- label('c')
-- bits(1u8) + bits(1i64)

━━━━━━━━━━━━━━━━━━━━
+ do kind'Item String
    - value Item

if Item.is-integer && Item.is-signed
    = "integer"
if Item.is-integer
    = "unsigned"
= "other"

━━━━━━━━━━━━━━━━━━━━
+ top'Item Item
    - value Item

Item.max

━━━━━━━━━━━━━━━━━━━━
+ bottom'Item Item
    - value Item

Item.min

━━━━━━━━━━━━━━━━━━━━
+ label'Item String
    - value Item

Item.name

━━━━━━━━━━━━━━━━━━━━
+ bits'Item Int
    - value Item

Item.bit-width
```

```output
integer
other
unsigned
unsigned
127
65535
-9223372036854775808
I64
Char
72
```

### The constant of a type is a value like any other.

```yumi
. limit = I32.max
. small I8 = I8.min
-- limit - 1
-- small
-- I32.max / 2
-- (U8.max as Int) + 1
. wide I64 = I32.max
-- wide + 1
if I8.is-signed
    -- "signed"
-- "bits ${U16.bit-width}"
-- twice(I16.max)
-- U8.max == 255
-- I8.min < 0

━━━━━━━━━━━━━━━━━━━━
+ twice I32
    - n I32

n * 2
```

```output
2147483646
-128
1073741823
256
2147483648
signed
bits 16
65534
true
true
```

### The width in bits of a type that is no integer is an error.

```yumi crash
-- F64.bit-width
```

```output
Exception: Integer type required for bit-width
<no file:2>
    -- F64.bit-width
       ^^^^^^^^^^^^^
```

### `type-of` evaluates its operand before a constant of the type is read.

```yumi
-- (type-of noisy()).max
-- (type-of noisy()).name
-- (type-of noisy()).is-signed

━━━━━━━━━━━━━━━━━━━━
+ do noisy I8

-- "noisy"
= 1
```

```output
noisy
127
noisy
I8
noisy
true
```

### A pointer type is an integer without a sign, 64 bits wide.

```yumi
. cell I64 = 7
-- CPointer'I64.is-number
-- CPointer'I64.is-integer
-- CPointer'I64.is-signed
-- CPointer'I64.bit-width
-- kind(&cell)
-- kind(cell)
-- kind(1.5)

━━━━━━━━━━━━━━━━━━━━
+ do kind'Item String
    - value Item

if Item.is-integer
    = "integer"
= "other"
```

```output
true
true
false
64
integer
integer
other
```

### The base of a type, and the name of the class an object was made as.

```yumi
-- Child.base
-- Child.base.name
. c Base = Child()
-- (type-of c).concrete-name
-- (type-of c).name
-- c.kind()
-- Base().kind()

━━━━━━━━━━━━━━━━━━━━
+ Base

────────────────────
+ kind String

(type-of this).concrete-name

━━━━━━━━━━━━━━━━━━━━
+ Child is Base
```

```output
[TypeInfo Base]
Base
Child
Base
Child
Base
```

### Two types given as values are equal when the types the values are held as are one type.

```yumi
. a Base = Child()
. b Base = Base()
-- type-of a == type-of b
-- type-of a != type-of b
-- type-of a == Base
-- type-of a == Child
-- Base == Child
-- Base != Child
-- Int == Int
-- type-of make() == Int

━━━━━━━━━━━━━━━━━━━━
+ do make Int

-- "made"
= 1

━━━━━━━━━━━━━━━━━━━━
+ Base

━━━━━━━━━━━━━━━━━━━━
+ Child is Base
```

```output
true
false
true
false
false
true
true
made
true
```

### `type-of` evaluates its operand.

```yumi
-- type-of make()
-- (type-of make()).name
. t = type-of thing()
-- t.concrete-name

━━━━━━━━━━━━━━━━━━━━
+ do make Int

-- "made"
= 1

━━━━━━━━━━━━━━━━━━━━
+ do thing Thing

-- "thing"
= Thing()

━━━━━━━━━━━━━━━━━━━━
+ Thing
```

```output
made
[TypeInfo I64]
made
I64
thing
Thing
```

### A relation runs the types it is asked of and the value it casts, each once and in the order written, and is text where it is printed or joined into text

```yumi
. seen = Seen()
-- (type-of seen.next()).compare(type-of seen.next())
-- seen.count
-- "is ${I32.compare(I64)} and ${Foo.compare(Bar)}"
-- I32.compare(I64).cast(seen.next())
-- seen.count
-- Bar.compare(Foo).cast(make(seen))
-- seen.count
-- (type-of make(seen)).compare-unified(type-of other(seen)).unify(make(seen))
-- seen.count

━━━━━━━━━━━━━━━━━━━━
+ do make Bar
    - seen Seen

seen.next()
= Bar()

━━━━━━━━━━━━━━━━━━━━
+ do other Baz
    - seen Seen

seen.next()
= Baz()

━━━━━━━━━━━━━━━━━━━━
+ Foo

━━━━━━━━━━━━━━━━━━━━
+ Bar is Foo

━━━━━━━━━━━━━━━━━━━━
+ Baz is Foo

━━━━━━━━━━━━━━━━━━━━
+ Seen

+ count = 0

────────────────────
+ do next

count += 1
= count
```

```output
[Same I64 = I64]
2
is [SideCast I32 = I64] and [DownCast Foo = Bar]
3
3
[Object __source__:Foo]
4
[Object __source__:Foo]
7
```

### A relation that is a converter takes a value of another type as the converter's receiver, and one of a type with itself gives the value as it is

```yumi
-- I32.compare(String).cast(4.2)
-- I32.compare(String).cast(7l)
-- Bar.compare(Foo).cast(Baz())
-- type-of Bar.compare(Foo).cast(Baz())
-- I32.compare(I32).cast(4.5)
-- Foo.compare(Foo).cast(Baz())

━━━━━━━━━━━━━━━━━━━━
+ Foo

━━━━━━━━━━━━━━━━━━━━
+ Bar

────────────────────
+ to Foo

Foo()

━━━━━━━━━━━━━━━━━━━━
+ Baz is Bar
```

```output
4
7
[Object __source__:Foo]
[TypeInfo Foo]
4.5
[Object __source__:Baz]
```

### A relation that is a converter refuses a value that is no receiver of the converter

```yumi crash
-- I32.compare(String).cast(Named())

━━━━━━━━━━━━━━━━━━━━
+ Named

────────────────────
+ to String

"named"
```

```output
Exception: Expected I32 here, but this is Named, and there is no conversion between them
<no file:2>
    -- I32.compare(String).cast(Named())
                                ^^^^^^^
```

### A relation that is a converter refuses a number written that its receiver cannot hold

```yumi crash
-- U8.compare(String).cast(300)
```

```output
Exception: Integer literal 300 does not fit U8
<no file:2>
    -- U8.compare(String).cast(300)
                               ^^^
```

### An array on the collected heap is named without its backing

```yumi
. plain = Array'Int(2)
. stated = Array'Float'GCBacking(1)
-- type-of plain
-- type-of stated
-- type-of [plain]
-- type-of plain.immutable
-- Array'Int.name, Array'String.name
-- Array'(Array'Int).name
-- Array'Int.compare(Array'I64)
```

```output
[TypeInfo Array'I64]
[TypeInfo Array'F64]
[TypeInfo List'Array'I64'GCBacking]
[TypeInfo ImmutableArray'I64'GCBacking]
(Array'I64, Array'String)
Array'Array'I64
[Same Array'I64 = Array'I64]
```

### A relation refuses a number written that the type it leads to cannot hold

```yumi crash
-- I32.compare(U8).cast(300)
```

```output
Exception: Integer literal 300 does not fit U8
<no file:2>
    -- I32.compare(U8).cast(300)
                            ^^^
```

### A relation that leads to an optional refuses a number written that the optional cannot hold

```yumi crash
-- I32.compare(Optional'U8).cast(-1)
```

```output
Exception: Integer literal -1 does not fit U8
<no file:2>
    -- I32.compare(Optional'U8).cast(-1)
                                      ^
```

### A class lists its stored properties, base class first, without getters or lazy properties

```yumi
for property in Square.properties
    -- "${property.name} ${property.type-name} ${kind-name(property)} ${property.size}"

━━━━━━━━━━━━━━━━━━━━
+ kind-name String
    - property PropertyInfo

|? "aggregate" if property.is-aggregate
|? "float" if property.is-float
|? "signed" if property.is-signed
|? "unsigned" if property.is-unsigned
|? "boolean" if property.is-boolean
|? "reference" if property.is-reference
?? "other"

━━━━━━━━━━━━━━━━━━━━
+ Shape

+ label = "shape"
+ visible = true

━━━━━━━━━━━━━━━━━━━━
+ Square is Shape

+ width F64 = 1
+ count I32 = -2
+ flags U8 = 3
+ center Point = Point(1, 2)
+ cached F64 := width * 3

────────────────────
+ get area F64

width * width

━━━━━━━━━━━━━━━━━━━━
+ Point is Tuple

+ x F32
+ y F32
```

```output
label String reference 8
visible Bool boolean 1
width F64 float 8
count I32 signed 4
flags U8 unsigned 1
center Point aggregate 8
```

### The type of a value seen through its base class lists the dynamic class's properties

```yumi
. shape Shape = Square()
-- (type-of shape).properties.length
-- Shape.properties.length

━━━━━━━━━━━━━━━━━━━━
+ Shape

+ label = "shape"

━━━━━━━━━━━━━━━━━━━━
+ Square is Shape

+ width F64 = 1
```

```output
2
1
```

### A tuple-typed property lists the properties of its tuple

```yumi
. center = Segment.properties.named("start")!
-- center.type-name
for property in center.properties
    -- "${property.name} ${property.offset}"

━━━━━━━━━━━━━━━━━━━━
+ Segment is Tuple

+ start Point
+ end Point

━━━━━━━━━━━━━━━━━━━━
+ Point is Tuple

+ x F64
+ y F64
```

```output
Point
x 0
y 8
```

### Property offsets read a value's stored fields

```yumi
. square = Square()
. address = square as Pointer
for property in Square.properties
    if property.is-float
        -- (address + property.offset as CPointer'F64)[0]
    else if property.is-signed
        -- (address + property.offset as CPointer'I32)[0]

━━━━━━━━━━━━━━━━━━━━
+ Square

+ width F64 = 1.5
+ count I32 = -2
```

```output
1.5
-2
```

### A tuple lists its fields, including those of a type with no storage

```yumi
for property in Sample.properties
    -- "${property.name} ${property.offset} ${property.size} ${property.properties.length}"

━━━━━━━━━━━━━━━━━━━━
+ Sample is Tuple

+ tick Tick
+ value F32
+ weight F64

━━━━━━━━━━━━━━━━━━━━
+ Tick is Tuple
```

```output
tick 0 0 0
value 0 4 0
weight 8 8 0
```
