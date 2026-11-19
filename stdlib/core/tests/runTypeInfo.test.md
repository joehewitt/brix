# Type info

```brix
(main
  (print (type-of 42))
)
```

```output
[TypeInfo I64]
```

```brix
(main
  (let ti _ I32)
  (print ti)
  (print (. ti name))
)
```

```output
[TypeInfo I32]
I32
```

### Long fixed-width names are compatibility aliases for the short canonical names.

```brix
(main
  (let ti _ Int32)
  (print ti)
  (print (. ti name))
  (print (== Int32 I32))
  (print (== UInt64 U64))
  (print (== Float32 F32))
)
```

```output
[TypeInfo I32]
I32
true
true
true
```

```brix
(main
  (print (. I1 size))
  (print (. I8 size))
  (print (. U8 size))
  (print (. I16 size))
  (print (. U16 size))
  (print (. I32 size))
  (print (. U32 size))
  (print (. I64 size))
  (print (. U64 size))
  (print (. F32 size))
  (print (. F64 size))
  (print (. Bool size))
  (print (. Char size))
)
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

```brix
(main
  (print (. I32 base))
)
```

```output
[TypeInfo Number]
```

```brix crash
(main
  (print (. Number base))
)
```

```output
Exception: No base type
<no file:3>
      (print (. Number base))
             ^^^^^^^^^^^^^^^
```

```brix
(main
  (print (. Bar base))
)

(type + Foo)

(type + Bar (is Foo))
```

```output
[TypeInfo Foo]
```

### Runtime class identity

```brix
(main
  (print (. (type-of (call Thing)) name))
  (print (. (type-of (call Thing)) concrete-name))
  (print (type-of (call Thing)))
)

(type + Thing)
```

```output
Thing
Thing
[TypeInfo Thing]
```

### Static and concrete identities for a base-typed class value

```brix
(main
  (let thing Base (call Child))
  (print (. (type-of thing) name))
  (print (. (type-of thing) concrete-name))
  (print (type-of thing))
  (print (call (. (type-of thing) compare) Base))
  (print (== (. (type-of thing) key) (. Base key)))
  (print (== (. (type-of thing) key) (. Child key)))
  (print (== (. (type-of thing) concrete-key) (. Child key)))
)

(type + Base)

(type + Child (is Base))
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

```brix
(main
  (let proto (as (call Thing) Proto))
  (print (. (type-of proto) name))
  (print (. (type-of proto) concrete-name))
  (print (type-of proto))
  (call (. proto foo))
)

(type + Proto (is Interface)
  (fn-expr + foo _)
)

(type + Thing
  (implements Proto)
  (fn-do + foo _
    (print "ok")
  )
)
```

```output
Proto
Thing
[TypeInfo Proto]
ok
```

### Concrete identity survives interface-to-interface subtable casts

```brix
(main
  (let child (as (call Thing) Child))
  (let base (as child Base))
  (print (. (type-of child) name))
  (print (. (type-of child) concrete-name))
  (print (. (type-of base) name))
  (print (. (type-of base) concrete-name))
  (call (. base base))
)

(type + Base (is Interface)
  (fn-expr + base _)
)

(type + Child (is Interface)
  (implements Base)
  (fn-expr + child _)
)

(type + Thing
  (implements Child)
  (fn-do + base _
    (print "base")
  )
  (fn-do + child _
    (print "child")
  )
)
```

```output
Child
Thing
Base
Thing
base
```

## Comparing types

```brix crash
(main
  (print (call (. I32 compare) 1))
)
```

```output
Exception: Invalid argument
<no file:3>
      (print (call (. I32 compare) 1))
                                   ^
```

```brix
(main
  (print (call (. Foo compare) Bar))
  (print (call (. Bar compare) Foo))
)

(blank)
(type-line + Foo)
(type-line + Bar)
```

```output
[Illegal Foo = Bar]
[Illegal Bar = Foo]
```

```brix
(main
  (print (call (. I32 compare) I32))
)
```

```output
[Same I32 = I32]
```

```brix
(main
  (print (call (. Foo compare) Foo))
  (print (call (. Foo compare) Bar))
  (print (call (. Bar compare) Foo))
)

(blank)
(type-line + Foo (alias foo) (is I32))
(type-line + Bar (alias bar) (is 2foo))
```

```output
[Same Foo = Foo]
[Equal Foo = Bar]
[Equal Bar = Foo]
```

```brix
(main
  (print (call (. I32 compare) I64))
  (print (call (. I32 compare) F32))
)
```

```output
[SideCast I32 = I64]
[SideCast I32 = F32]
```

```brix
(main
  (print (call (. Foo compare) I32))
  (print (call (. Bar compare) I32))
  (print (call (. Bar compare) Foo))
  (print (call (. Foo compare) Bar))
)

(blank)
(type-line + Foo (is I32))
(type-line + Bar (is Foo))
```

```output
[SideCast Foo = I32]
[SideCast Bar = I32]
[SideCast Bar = Foo]
[SideCast Foo = Bar]
```

```brix
(main
  (print (call (. (of Optional I32) compare) Bool))
  (print (call (. I32 compare) (of Optional I32)))
)
```

```output
[Converter Optional'I32 = Bool]
[SideCast I32 = Optional'I32]
```

```brix
(main
  (let t (tuple 1 2))
  (let t2 (tuple 1.0 2.0))
  (print (call (. (type-of t) compare) (type-of t2)))
)
```

```output
[FinalCast Tuple = Tuple]
```

```brix
(main
  (print (call (. Foo compare) Foo))
  (print (call (. Foo compare) Bar))
  (print (call (. Bar compare) Foo))
)

(blank)
(type-line + Foo)
(type-line + Bar (is Foo))
```

```output
[Same Foo = Foo]
[DownCast Foo = Bar]
[UpCast Bar = Foo]
```

```brix
(main
  (print (call (. Foo compare) Foo))
  (print (call (. Foo compare) Bar))
  (print (call (. Bar compare) Foo))
)

(blank)
(type-line + Foo)

(type + Bar
  (fn-to-expr + Foo
    (call Foo)
  )
)
```

```output
[Same Foo = Foo]
[Illegal Foo = Bar]
[Converter Bar = Foo]
```

```brix
(main
  (print (call (. Bar compare) Foo))
  (print (call (. Foo compare) Bar))
)

(type + Foo
  (fn-from-expr + (: bar Bar)
    (call This)
  )
)

(type + Bar)
```

```output
[Fromverter Bar = Foo]
[Illegal Foo = Bar]
```

### Show fromverter takes precedence over converter

```brix
(main
  (print (call (. Bar compare) Foo))
  (print (call (. Foo compare) Bar))
)

(type + Foo
  (fn-from-expr + (: bar Bar)
    (call This)
  )
)

(type + Bar
  (fn-to-expr + Foo
    (call Foo)
  )
)
```

```output
[Fromverter Bar = Foo]
[Illegal Foo = Bar]
```

```brix
(main
  (print (call (. Foo compare-unified) Bar))
  (print (call (. Bar compare-unified) Foo))
)

(blank)
(type-line + Base)
(type-line + Foo (is Base))
(type-line + Bar (is Base))
```

```output
[CommonUpCast Foo = Bar]
[CommonUpCast Bar = Foo]
```

```brix
(main
  (print (call (. Foo compare-unified) Bar))
  (print (call (. Bar compare-unified) Foo))
)

(blank)
(type-line + Proto (is Interface))

(type + Foo
  (implements Proto)
)

(type + Bar
  (implements Proto)
)
```

```output
[CommonInterface Foo = Bar]
[CommonInterface Bar = Foo]
```

```brix
(main
  (print (call (. Foo compare-unified) Bar))
  (print (call (. Bar compare-unified) Foo))
)

(type + Foo
  (fn-to-expr + Baz
    (call Baz)
  )
)

(type + Bar
  (fn-to-expr + Baz
    (call Baz)
  )
)

(type + Baz)
```

```output
[CommonConverter Foo = Bar]
[CommonConverter Bar = Foo]
```

```brix
(main
  (print (call (. Foo compare-unified) Baz))
  (print (call (. Foo compare-unified) Bazoo))
)

(blank)
(type-line + Baz (alias baz) (is Int))
(type-line + Bazoo (alias bazoo) (is Int))
(blank)
(unit + 1baz 2bazoo)

(type + Foo
  (fn-to-expr + Baz
    (call Baz)
  )
)
```

```output
[Converter Foo = Baz]
[CommonEquality Foo = Bazoo]
```

```brix
(main
  (print (call (. String compare) Bar))
  (print (call (. Bar compare) String))
  (print (call (. Foo compare) String))
)

(type + Foo
  (fn-to-expr + Baz
    (call Baz)
  )
)

(type + Bar
  (fn-to-expr + Baz
    (call Baz)
  )
  (fn-to-expr + String
    "Bar"
  )
)

(type + Baz)
```

```output
[Illegal String = Bar]
[Converter Bar = String]
[StringCast Foo = String]
```

```brix
(main
  (print (call (. String compare) (of CPointer Char)))
)
```

```output
[SideCast String = CPointer'Char]
```

```brix
(main
  (print (call (. Foo compare) String))
)

(blank)
(type-line + Foo)
```

```output
[StringCast Foo = String]
```

## Casting

### Illegal comparison

```brix crash
(type-line + Foo)
(type-line + Bar)
(blank)

(main
  (print (call (. (call (. Foo compare) Bar) cast) (call Foo)))
)
```

```output
Exception: Expected Bar here, but this is Foo, and there is no conversion between them
<no file:7>
      (print (call (. (call (. Foo compare) Bar) cast) (call Foo)))
             ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Same comparison

```brix
(main
  (print (call (. (call (. Foo compare) Foo) cast) (call Foo)))
)

(blank)
(type-line + Foo)
```

```output
[Object __source__:Foo]
```

### Equality comparison

```brix
(main
  (print (call (. (call (. Foo compare) Foo) cast) 1foo))
  (print (call (. (call (. Foo compare) Bar) cast) 1foo))
  (print (call (. (call (. Bar compare) Foo) cast) 1bar))
)

(blank)
(type-line + Foo (alias foo) (is Float))
(type-line + Bar (alias bar) (is 2foo))
```

```output
1
0.5
2
```

### SideCast to Number comparison

```brix
(main
  (print (as (call (. (call (. I32 compare) I64) cast) 4) UnitString))
  (print (as (call (. (call (. I32 compare) F32) cast) 4) UnitString))
)
```

```output
4i
4f32
```

### Cast numeric subtypes to base builting numeric types

```brix
(main
  (print (as (call (. (call (. Foo compare) I32) cast) 1foo) UnitString))
  (print (as (call (. (call (. Bar compare) I32) cast) 1bar) UnitString))
)

(blank)
(type-line + Foo (alias foo) (is I32))
(type-line + Bar (alias bar) (is Foo))
```

```output
1i32
1i32
```

```brix
(main
  (print (call (. (call (. Bar compare) Foo) cast) 1bar))
)

(blank)
(type-line + Foo (alias foo) (is I32))
(type-line + Bar (alias bar) (is Foo))
```

```output
1
```

### SideCast to Bool comparison

```brix
(main
  (print (call (. (call (. I32 compare) Bool) cast) 0))
  (print (call (. (call (. I32 compare) Bool) cast) 1))
)
```

```output
false
true
```

### SideCast Optional comparison

```brix
(main
  (let a (? I32))
  (let b (? I32) 1)
  (print (call (. (call (. (of Optional I32) compare) Bool) cast) a))
  (print (call (. (call (. (of Optional I32) compare) Bool) cast) b))
)
```

```output
false
true
```

### SideCast comparison

```brix
(main
  (print (type-of (call (. (call (. (of CPointer Char) compare) String) cast) (. ([] Char 1) data))))
  (print (type-of (call (. (call (. String compare) (of CPointer Char)) cast) "foo")))
  (print (call (. (call (. String compare) (of CPointer Char8)) cast) "foo"))
)
```

```output
[TypeInfo String]
[TypeInfo CPointer'Char]
foo
```

### UpCast comparison

```brix
(main
  (print (call (. (call (. Foo compare) Foo) cast) (call Foo)))
  (print (call (. (call (. Bar compare) Foo) cast) (call Bar)))
)

(blank)
(type-line + Foo)
(type-line + Bar (is Foo))
```

```output
[Object __source__:Foo]
[Object __source__:Foo]
```

### Converter comparisonType

```brix
(main
  (print (call (. (call (. Bar compare) Foo) cast) (call Bar)))
)

(type + Foo)

(type + Bar
  (fn-to-expr + Foo
    (call Foo)
  )
)
```

```output
[Object __source__:Foo]
```

### Fromverter comparisonType

```brix
(main
  (print (call (. (call (. Bar compare) Foo) cast) (call Bar)))
)

(type + Foo
  (fn-from-expr + (: bar Bar)
    (call This)
  )
)

(type + Bar)
```

```output
[Object __source__:Foo]
```

### StringCast comparison

```brix
(main
  (print (type-of (call (. (call (. I32 compare) String) cast) 4.2)))
)
```

```output
[TypeInfo String]
```

```brix
(main
  (print (call (. (call (. Foo compare) String) cast) (call Foo)))
)

(type + Foo
  (fn-to-expr + String
    "foo"
  )
)
```

```output
foo
```

## Unifying

### Unifying to common equality

```brix
(main
  (print (call (. (call (. Foo compare-unified) Bazoo) unify) (call Foo)))
)

(blank)
(type-line + Baz (alias baz) (is Float))
(type-line + Bazoo (alias bazoo) (is Float))
(blank)
(unit + 1baz 2bazoo)

(type + Foo
  (fn-to-expr + Baz
    1baz
  )
)
```

```output
2
```

### Unifying to common converter

```brix
(main
  (print (call (. (call (. Foo compare-unified) Bar) unify) (call Foo)))
  (print (call (. (call (. Bar compare-unified) Foo) unify) (call Bar)))
)

(type + Foo
  (fn-to-expr + Baz
    (call Baz)
  )
)

(type + Bar
  (fn-to-expr + Baz
    (call Baz)
  )
)

(type + Baz)
```

```output
[Object __source__:Baz]
[Object __source__:Baz]
```

### Unify to common base class

```brix
(main
  (print (type-of (call (. (call (. Foo compare-unified) Bar) unify) (call Foo))))
  (print (type-of (call (. (call (. Bar compare-unified) Foo) unify) (call Bar))))
)

(blank)
(type-line + Base)
(type-line + Foo (is Base))
(type-line + Bar (is Base))
```

```output
[TypeInfo Base]
[TypeInfo Base]
```

## Comparison operators

```brix
(main
  (print (== I32 I32))
  (print (== I32 I64))
)
```

```output
true
false
```

```brix
(main
  (print (== (of Optional I32) (of Optional I32)))
  (print (== (of Optional I32) (of Optional I64)))
)
```

```output
true
false
```

### A number type answers what kind of number it is.

```brix
(main
  (print (. I8 is-signed))
  (print (. U8 is-signed))
  (print (. F64 is-signed))
  (print (. Bool is-signed))
  (print (. Char is-signed))
  (print (. String is-signed))
  (print (. I8 is-integer))
  (print (. F32 is-integer))
  (print (. Bool is-integer))
  (print (. Char is-integer))
  (print (. String is-integer))
  (print (. F32 is-number))
  (print (. Bool is-number))
  (print (. Char is-number))
  (print (. String is-number))
  (print (. I32 is-tuple))
  (print (. String is-tuple))
  (print (. I32 is-struct))
  (print (. I32 read-only))
  (print (. String read-only))
)
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

```brix
(main
  (print (. I8 max))
  (print (. I8 min))
  (print (. U8 max))
  (print (. U8 min))
  (print (. I16 max))
  (print (. I16 min))
  (print (. U16 max))
  (print (. I32 max))
  (print (. I32 min))
  (print (. U32 max))
  (print (. I64 max))
  (print (. I64 min))
  (print (. U64 max))
  (print (. U64 min))
  (print (. Size max))
  (print (. Int max))
  (print (. Byte max))
  (print (. Index max))
  (print (as (. Char max) U32))
  (print (as (. Char min) U32))
  (print (. F64 max))
  (print (. F64 min))
  (print (. F32 max))
  (print (. F32 min))
  (print (. Bool max))
  (print (. Bool min))
)
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

```brix
(main
  (print (. I8 bit-width))
  (print (. U8 bit-width))
  (print (. I16 bit-width))
  (print (. I32 bit-width))
  (print (. U64 bit-width))
  (print (. Size bit-width))
  (print (. Char bit-width))
  (print (. Int bit-width))
  (print (. Bool bit-width))
  (print (. I8 size))
  (print (. I16 size))
  (print (. I32 size))
  (print (. I64 size))
  (print (. F32 size))
  (print (. F64 size))
  (print (. Bool size))
  (print (. Char size))
)
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

```brix
(main
  (print (. Int name))
  (print (. I8 name))
  (print (. U64 name))
  (print (. F64 name))
  (print (. Float name))
  (print (. Char name))
  (print (. Bool name))
  (print (. String name))
  (print (. Size name))
  (print (. Index name))
  (print (. Byte name))
  (print (. I32 key))
  (print (. String key))
  (print (. Bool key))
  (print (. I32 concrete-name))
  (print (. I32 concrete-key))
  (print (. Char concrete-name))
  (print (++ (. I8 name) "!"))
  (print (. I8 name size))
)
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

```brix
(main
  (print (call kind 1))
  (print (call kind 1.5))
  (print (call kind 2u8))
  (print (call kind true))
  (print (call top 1i8))
  (print (call top 1u16))
  (print (call bottom 1i64))
  (print (call label 7))
  (print (call label 'c'))
  (print (+ (call bits 1u8) (call bits 1i64)))
)

(fn-do + (of kind Item) String
  (param value Item)
  (if (and (. Item is-integer) (. Item is-signed))
    (return "integer")
  )
  (if (. Item is-integer)
    (return "unsigned")
  )
  (return "other")
)

(fn-expr + (of top Item) Item
  (param value Item)
  (. Item max)
)

(fn-expr + (of bottom Item) Item
  (param value Item)
  (. Item min)
)

(fn-expr + (of label Item) String
  (param value Item)
  (. Item name)
)

(fn-expr + (of bits Item) Int
  (param value Item)
  (. Item bit-width)
)
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

```brix
(main
  (let limit (. I32 max))
  (let small I8 (. I8 min))
  (print (- limit 1))
  (print small)
  (print (/ (. I32 max) 2))
  (print (+ (as (. U8 max) Int) 1))
  (let wide I64 (. I32 max))
  (print (+ wide 1))
  (if (. I8 is-signed)
    (print "signed")
  )
  (print (str "bits " (. U16 bit-width)))
  (print (call twice (. I16 max)))
  (print (== (. U8 max) 255))
  (print (< (. I8 min) 0))
)

(fn-expr + twice I32
  (param n I32)
  (* n 2)
)
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

```brix crash
(main
  (print (. F64 bit-width))
)
```

```output
Exception: Integer type required for bit-width
<no file:3>
      (print (. F64 bit-width))
             ^^^^^^^^^^^^^^^^^
```

### `type-of` evaluates its operand before a constant of the type is read.

```brix
(main
  (print (. (type-of (call noisy)) max))
  (print (. (type-of (call noisy)) name))
  (print (. (type-of (call noisy)) is-signed))
)

(fn-do + noisy I8
  (print "noisy")
  (return 1)
)
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

```brix
(main
  (let cell I64 7)
  (print (. (of CPointer I64) is-number))
  (print (. (of CPointer I64) is-integer))
  (print (. (of CPointer I64) is-signed))
  (print (. (of CPointer I64) bit-width))
  (print (call kind &cell))
  (print (call kind cell))
  (print (call kind 1.5))
)

(fn-do + (of kind Item) String
  (param value Item)
  (if (. Item is-integer)
    (return "integer")
  )
  (return "other")
)
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

```brix
(main
  (print (. Child base))
  (print (. Child base name))
  (let c Base (call Child))
  (print (. (type-of c) concrete-name))
  (print (. (type-of c) name))
  (print (call (. c kind)))
  (print (call (. (call Base) kind)))
)

(type + Base
  (fn-expr + kind String
    (. (type-of this) concrete-name)
  )
)

(type + Child (is Base))
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

```brix
(main
  (let a Base (call Child))
  (let b Base (call Base))
  (print (== (type-of a) (type-of b)))
  (print (!= (type-of a) (type-of b)))
  (print (== (type-of a) Base))
  (print (== (type-of a) Child))
  (print (== Base Child))
  (print (!= Base Child))
  (print (== Int Int))
  (print (== (type-of (call make)) Int))
)

(fn-do + make Int
  (print "made")
  (return 1)
)

(type + Base)

(type + Child (is Base))
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

```brix
(main
  (print (type-of (call make)))
  (print (. (type-of (call make)) name))
  (let t (type-of (call thing)))
  (print (. t concrete-name))
)

(fn-do + make Int
  (print "made")
  (return 1)
)

(fn-do + thing Thing
  (print "thing")
  (return (call Thing))
)

(type + Thing)
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

```brix
(main
  (let seen (call Seen))
  (print (call (. (type-of (call (. seen next))) compare) (type-of (call (. seen next)))))
  (print (. seen count))
  (print (str "is " (call (. I32 compare) I64) " and " (call (. Foo compare) Bar)))
  (print (call (. (call (. I32 compare) I64) cast) (call (. seen next))))
  (print (. seen count))
  (print (call (. (call (. Bar compare) Foo) cast) (call make seen)))
  (print (. seen count))
  (print (call (. (call (. (type-of (call make seen)) compare-unified) (type-of (call other seen))) unify) (call make seen)))
  (print (. seen count))
)

(fn-do + make Bar
  (param seen Seen)
  (call (. seen next))
  (return (call Bar))
)

(fn-do + other Baz
  (param seen Seen)
  (call (. seen next))
  (return (call Baz))
)

(type + Foo)

(type + Bar (is Foo))

(type + Baz (is Foo))

(type + Seen
  (prop + count _ (= 0))
  (fn-do + next _
    (+= count 1)
    (return count)
  )
)
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

```brix
(main
  (print (call (. (call (. I32 compare) String) cast) 4.2))
  (print (call (. (call (. I32 compare) String) cast) 7l))
  (print (call (. (call (. Bar compare) Foo) cast) (call Baz)))
  (print (type-of (call (. (call (. Bar compare) Foo) cast) (call Baz))))
  (print (call (. (call (. I32 compare) I32) cast) 4.5))
  (print (call (. (call (. Foo compare) Foo) cast) (call Baz)))
)

(type + Foo)

(type + Bar
  (fn-to-expr + Foo
    (call Foo)
  )
)

(type + Baz (is Bar))
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

```brix crash
(main
  (print (call (. (call (. I32 compare) String) cast) (call Named)))
)

(type + Named
  (fn-to-expr + String
    "named"
  )
)
```

```output
Exception: Expected I32 here, but this is Named, and there is no conversion between them
<no file:3>
      (print (call (. (call (. I32 compare) String) cast) (call Named)))
                                                          ^^^^^^^^^^^^
```

### A relation that is a converter refuses a number written that its receiver cannot hold

```brix crash
(main
  (print (call (. (call (. U8 compare) String) cast) 300))
)
```

```output
Exception: Integer literal 300 does not fit U8
<no file:3>
      (print (call (. (call (. U8 compare) String) cast) 300))
                                                         ^^^
```

### An array on the collected heap is named without its backing

```brix
(main
  (let plain (call (of Array Int) 2))
  (let stated (call (of Array Float GCBacking) 1))
  (print (type-of plain))
  (print (type-of stated))
  (print (type-of (list plain)))
  (print (type-of (. plain immutable)))
  (print (tuple (. (of Array Int) name) (. (of Array String) name)))
  (print (. (of Array (of Array Int)) name))
  (print (call (. (of Array Int) compare) (of Array I64)))
)
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

```brix crash
(main
  (print (call (. (call (. I32 compare) U8) cast) 300))
)
```

```output
Exception: Integer literal 300 does not fit U8
<no file:3>
      (print (call (. (call (. I32 compare) U8) cast) 300))
                                                      ^^^
```

### A relation that leads to an optional refuses a number written that the optional cannot hold

```brix crash
(main
  (print (call (. (call (. I32 compare) (of Optional U8)) cast) -1))
)
```

```output
Exception: Integer literal -1 does not fit U8
<no file:3>
      (print (call (. (call (. I32 compare) (of Optional U8)) cast) -1))
                                                                    ^^
```

### A class lists its stored properties, base class first, without getters or lazy properties

```brix
(main
  (for (in property (. Square properties))
    (print (str (. property name) " " (. property type-name) " " (call kind-name property) " " (. property size)))
  )
)

(fn-expr + kind-name String
  (param property PropertyInfo)
  (case? (only-if "aggregate" (if (. property is-aggregate))))
  (case? (only-if "float" (if (. property is-float))))
  (case? (only-if "signed" (if (. property is-signed))))
  (case? (only-if "unsigned" (if (. property is-unsigned))))
  (case? (only-if "boolean" (if (. property is-boolean))))
  (case? (only-if "reference" (if (. property is-reference))))
  (default
    (inline "other")
  )
)

(type + Shape
  (prop + label _ (= "shape"))
  (prop + visible _ (= true))
)

(type + Square (is Shape)
  (prop + width F64 (= 1))
  (prop + count I32 (= -2))
  (prop + flags U8 (= 3))
  (prop + center Point (= (call Point 1 2)))
  (prop + cached F64 (:= (* width 3)))
  (fn-expr + area F64 :get
    (* width width)
  )
)

(type + Point (is Tuple)
  (prop + x F32)
  (prop + y F32)
)
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

```brix
(main
  (let shape Shape (call Square))
  (print (. (type-of shape) properties length))
  (print (. Shape properties length))
)

(type + Shape
  (prop + label _ (= "shape"))
)

(type + Square (is Shape)
  (prop + width F64 (= 1))
)
```

```output
2
1
```

### A tuple-typed property lists the properties of its tuple

```brix
(main
  (let center (!! (call (. Segment properties named) "start")))
  (print (. center type-name))
  (for (in property (. center properties))
    (print (str (. property name) " " (. property offset)))
  )
)

(type + Segment (is Tuple)
  (prop + start Point)
  (prop + end Point)
)

(type + Point (is Tuple)
  (prop + x F64)
  (prop + y F64)
)
```

```output
Point
x 0
y 8
```

### Property offsets read a value's stored fields

```brix
(main
  (let square (call Square))
  (let address (as square Pointer))
  (for (in property (. Square properties))
    (if (. property is-float)
      (print ([] (as (+ address (. property offset)) (of CPointer F64)) 0))
      (else-if (. property is-signed)
        (print ([] (as (+ address (. property offset)) (of CPointer I32)) 0))
      )
    )
  )
)

(type + Square
  (prop + width F64 (= 1.5))
  (prop + count I32 (= -2))
)
```

```output
1.5
-2
```

### A tuple lists its fields, including those of a type with no storage

```brix
(main
  (for (in property (. Sample properties))
    (print (str (. property name) " " (. property offset) " " (. property size) " " (. property properties length)))
  )
)

(type + Sample (is Tuple)
  (prop + tick Tick)
  (prop + value F32)
  (prop + weight F64)
)

(type + Tick (is Tuple))
```

```output
tick 0 0 0
value 0 4 0
weight 8 8 0
```
