# Properties

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

## Eager properties

```brix
(main
  (let x (call Thing))
  (print (. x a))
)

(type + Thing
  (prop + a _ (= 42))
)
```

```output
42
```

```brix
(main
  (let x (call Thing))
  (print (. x a))
)

(type + Base
  (prop + a _ (= 2.0))
)

(type + Thing (is Base)
  (prop + a _ (= 42))
)
```

```output
42
```

```brix
(main
  (print (. (call Thing) a))
  (print (. (call Thing 100) a))
)

(type + Thing
  (prop + a _ (= 42))
  (fn-do + This _
    (param a _)
  )
)
```

```output
42
100
```

```brix
(main
  (print (. (call Thing 100.8) a))
  (print (. (call Thing 110i8) a))
)

(type + Thing
  (prop + a _ (= 42.0))
  (fn-do + This _
    (param a Float)
    (print "float")
  )
  (fn-do + This _
    (param a I8)
    (print "i8")
  )
)
```

```output
float
100.8
i8
110
```

```brix
(main
  (print (. (call Thing) a))
  (print (. (call Thing 100) a))
)

(fn-do + foo _
  (print "called foo")
  (return 42)
)

(type + Thing
  (prop + a _ (= (call foo)))
  (fn-do + This _
    (param a _)
  )
)
```

```output
called foo
42
100
```

```brix crash
(main
  (let x (call Thing))
  (print (. x a))
)

(type + Thing
  (prop + a _ (= 42))
  (prop + a _ (= 42))
)
```

```output
Exception: Thing declares "a" twice (first on line 8); remove one of them, or rename it
<no file:9>
      (prop + a _ (= 42))
      ^^^^^^^^^^^^^^^^^^^
```

```brix crash
(main
  (print (. (call Thing) a))
)

(type + Thing (is Int)
  (prop + a _ (= 1))
)
```

```output
Exception: Property not supported
<no file:7>
      (prop + a _ (= 1))
      ^^^^^^^^^^^^^^^^^^
```

## Constructor assignments

```brix
(main
  (print (. (call Thing) a))
)

(type + Thing
  (prop + a _ (= 0))
  (fn-do + This _
    (lazy a 42)
  )
)
```

```output
42
```

```brix
(main
  (print (call Thing))
)

(type + Thing
  (prop + a _ (= 0))
  (fn-do + This _
    (return b)
    (blank)
    (lazy b 42)
  )
)
```

```output
42
```

```brix
(main
  (print (. (call Thing 42) a))
)

(type + Thing
  (prop + a _ (= 0))
  (fn-do + This _
    (param x _)
    (lazy a (+ x 1))
  )
)
```

```output
43
```

```brix
(main
  (print (. (call Thing 42) a))
)

(type + Thing
  (prop + a _ (= 0))
  (fn-do + This _
    (param x _)
    (lazy b (+ x 1))
    (lazy a b)
  )
)
```

```output
43
```

```brix
(main
  (let thing (call Thing 42))
  (print (. thing a))
  (print (. thing b))
)

(type + Thing
  (prop + a _ (= 0))
  (prop + b _ (= 0))
  (fn-do + This _
    (param b _)
    (lazy c (+ b 1))
    (lazy a c)
  )
)
```

```output
43
42
```

```brix
(main
  (print (. (call Thing) dependent))
)

(fn-do + make-value _
  (print "called")
  (return 42)
)

(type + Thing
  (prop + owner _ (= (call make-value)))
  (prop + dependent _ (= 0))
  (fn-do + This _
    (lazy dependent owner)
  )
)
```

```output
called
42
```

### Constructor where clauses that first read another property inside a branch still initialize that property on every path, evaluating its expression once.

```brix
(main
  (let b (call Labeled "width" "stroke"))
  (print (str (. b label) "|" (. b key)))
  (let c (call Labeled "width" ""))
  (print (str (. c label) "|" (. c key)))
  (let d (call Flagged "width" "stroke" true))
  (print (str (. d label) "|" (. d key) "|" (. d tag) "|" (. d note) "|" (. d wide)))
  (let e (call Flagged "width" "" false))
  (print (str (. e label) "|" (. e key) "|" (. e tag) "|" (. e note) "|" (. e wide)))
  (let f (call Computed "abc" ""))
  (print (str (. f key) "|" (. f label) "|" (call (. f shout))))
  (let g (call Computed "abc" "zz"))
  (print (str (. g key) "|" (. g label) "|" (call (. g shout))))
)

(type + Labeled
  (prop + key String)
  (prop + label String)
  (fn-do + This _
    (param label _)
    (param name String)
    (lazy key (if-expr (. name length) name label))
  )
)

(type + Flagged
  (prop + key String)
  (prop + tag String)
  (prop + note String)
  (prop + wide Bool)
  (prop + label String)
  (fn-do + This _
    (param label _)
    (param name String)
    (param flag Bool)
    (lazy key (if-expr (. name length) name label))
    (lazy tag (if-expr flag label "none"))
    (lazy note (if-expr (and flag (> (. label length) 2)) "yes" label))
    (lazy wide (or (not flag) (> (. label length) 3)))
  )
)

(type + Shouter (is (fn-type String)))

(type + Computed
  (prop + key String)
  (prop + label String)
  (prop + shouter Shouter)
  (fn-do + This _
    (param base String)
    (param name String)
    (lazy key (if-expr (. name length) name label))
    (lazy label (call compute base))
    (lazy shouter
      (=> ()
        (block
          (str label "!")
        )
      )
    )
  )
  (fn-expr + shout String
    (call shouter)
  )
)

(fn-do + compute String
  (param value String)
  (print (str "compute " value))
  (return (+ value "-x"))
)
```

```output
width|stroke
width|width
width|stroke|width|yes|true
width|width|none|width|true
compute abc
abc-x|abc-x|abc-x!
compute abc
zz|abc-x|abc-x!
```

## Lazy

```brix
(main
  (print (. (call Thing) a))
)

(type + Thing
  (prop + a _ (:= (+ (. this b) 1)))
  (prop + b _ (= 42))
)
```

```output
43
```

```brix
(main
  (print (. (call Thing) a))
)

(fn-do + foo _
  (print "called foo")
  (return 1)
)

(type + Thing
  (prop + a _ (:= (call foo)))
  (fn-do + This _
    (lazy a 2)
  )
)
```

```output
2
```

```brix
(main
  (let x (call Thing))
  (print (. x a))
)

(type + Base
  (prop + a _ (:= 42))
)

(type + Thing (is Base)
  (prop + a _ (:= 43))
)
```

```output
43
```

```brix
(main
  (let thing (call Thing))
  (print (. thing a))
  (print (. thing j))
)

(type + Thing
  (prop + a _ (:= (call foo)))
  (prop + b _ (:= (call foo)))
  (prop + c _ (:= (call foo)))
  (prop + d _ (:= (call foo)))
  (prop + e _ (:= (call foo)))
  (prop + f _ (:= (call foo)))
  (prop + g _ (:= (call foo)))
  (prop + h _ (:= (call foo)))
  (blank)
  (prop + i _ (:= (call foo)))
  (prop + j _ (:= (call foo)))
)

(fn-do + foo _
  (print "called foo")
  (return 42)
)
```

```output
called foo
42
called foo
42
```

```brix
(main
  (let thing (call Thing))
  (print (. thing b))
  (print (. thing c))
)

(type + Thing
  (prop + a _ (:= (call foo)))
  (prop + b _ (:= a))
  (prop + c _ (:= a))
)

(fn-do + foo _
  (print "called foo")
  (return 42)
)
```

```output
called foo
42
42
```

### Throwing an exception in lazy property

```brix
(main
  (let thing (call Thing))
  (print (. thing a))
  (print (. thing a))
)

(type + Thing
  (prop + a _ (:= (call foo 1)))
  (prop + b _ (= 42))
)

(fn-do + foo _
  (param a _)
  (print "call foo")
  (if (== a 0)
    (throw (call Panic "fail"))
    (else
      (return 42)
    )
  )
)
```

```output
call foo
42
42
```

### Assignment prevents initializer from being called

```brix
(main
  (let thing (call Thing))
  (= (. thing a) 99)
  (print (. thing a))
)

(fn-do + tick _
  (print "call tick")
  (return 42)
)

(type + Thing
  (prop + a _ (:= (call tick)))
)
```

```output
99
```

### Constructor argument overrides lazy initializer

```brix
(main
  (let thing (call Thing 99))
  (print (. thing a))
)

(fn-do + tick _
  (print "call tick")
  (return 42)
)

(type + Thing
  (prop + a _ (:= (call tick)))
  (fn-do + This _
    (param a _)
  )
)
```

```output
99
```

### Constructor expression overrides lazy initializer

```brix
(main
  (let thing (call Thing))
  (print (. thing a))
)

(fn-do + tick _
  (print "call tick")
  (return 42)
)

(type + Thing
  (prop + a _ (:= (call tick)))
  (fn-do + This _
    (lazy a 99)
  )
)
```

```output
99
```

### Initial value cast three times since each assignment has a type

```brix crash
(main
  (let thing (call Thing))
  (print (. thing a))
)

(type + Base
  (prop + a Float)
)

(type + Thing (is Base)
  (prop + a Int)
)
```

```output
Exception: "a" is inherited from Base, so it keeps the type it has there and cannot state its own; remove "Int"
<no file:12>
      (prop + a Int)
      ^^^^^^^^^^^^^^
```

### Initializers are virtual methods

```brix
(main
  (let base (as (call Thing) Base))
  (print (. base a))
)

(type + Base
  (prop + a _ (:= "Base"))
)

(type + Thing (is Base)
  (prop + a _ (:= "Thing"))
)
```

```output
Thing
```

```brix
(main
  (let thing (call Thing))
  (print (. thing a))
  (blank)
  (let base (call Base))
  (print (. base a))
)

(type + Base
  (prop + a _ (= "Base"))
  (blank)
  (prop + b _ (:= 42))
)

(type + Thing (is Base)
  (prop + a _ (= "Thing"))
)
```

```output
Thing
Base
```

```brix crash
(main
  (let base (as (call Thing) Base))
  (print (. base a))
)

(type + Base
  (prop + a _ (= "Base"))
)

(type + Thing (is Base)
  (prop + a _ (:= "Thing"))
)
```

```output
Exception: "a" is inherited from Base, which declares it with "=", so declare it with "=" here too, not with ":="
<no file:12>
      (prop + a _ (:= "Thing"))
      ^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Inherited lazy property initializers use the declaring module's imports

```brix
(import ./lib/derived)
(blank)

(main
  (print (. (call Child) value))
)
```

```brix file=lib/support.bx
(type + SupportThing
  (fn-to-expr + String
    "from support"
  )
)
```

```brix file=lib/base.bx
(import ./support)

(type + Base
  (prop + value _ (:= (as (call SupportThing) String)))
)
```

```brix file=lib/derived.bx
(import ./base)

(type + Child (is Base))
```

```output
from support
```

```brix crash
(main
  (print (. (call Thing) a))
)

(type + Thing (is Int)
  (prop + a _ (:= 1))
)
```

```output
Exception: Property not supported
<no file:7>
      (prop + a _ (:= 1))
      ^^^^^^^^^^^^^^^^^^^
```

## Getters

```brix
(main
  (let thing (call Thing))
  (print (. thing foo))
  (print (. thing foo))
)

(type + Thing
  (prop + a _ (= 0))
  (prop + foo _ (=> (+= a 1)))
)
```

```output
1
2
```

```brix
(main
  (let thing (call Thing))
  (print (. thing a))
)

(fn-do + foo _
  (print "called foo")
  (return 42)
)

(type + Thing
  (fn-expr + a _ :get
    (call foo)
  )
)
```

```output
called foo
42
```

```brix
(main
  (let thing (call Thing))
  (print (. thing a))
)

(type + Thing (is Int)
  (fn-expr + a _ :get
    42
  )
)
```

```output
42
```

```brix
(main
  (let thing (call Thing))
  (print thing)
)

(type + Thing (is Tuple)
  (prop + a Int)
  (prop + c Int)
  (fn-expr + b _ :get
    42
  )
)
```

```output
(0, 0)
```

```brix
(main
  (let thing (call Thing 1 2))
  (print thing)
  (print (. thing b))
)

(type + Thing (is Tuple)
  (prop + a Int)
  (prop + c Int)
  (fn-expr + b _ :get
    42
  )
)
```

```output
(1, 2)
42
```

```brix crash
(main
  (let thing (call Thing))
  (print (. thing a))
)

(type + Base
  (prop + a _ (= 42))
)

(type + Thing (is Base)
  (fn-expr + a _ :get
    43
  )
)
```

```output
Exception: "a" is inherited from Base, which declares it with "=", so declare it with "=" here too, not as a getter
<no file:12>
      (fn-expr + a _ :get
      ^^^^^^^^^^^^^^^^^^
```

```brix crash
(main
  (let thing (call Thing))
  (print (. thing b))
)

(fn-do + foo _
  (param x _)
  (print "called foo")
  (return x)
)

(type + Thing
  (prop + b _ (= a))
  (fn-expr + a _ :get
    (call foo this)
  )
)
```

```output
Exception: Illegal to reference getters in eager properties
<no file:14>
      (prop + b _ (= a))
                     ^
```

```brix crash
(main
  (let thing (call Thing))
  (print (. thing b))
)

(fn-do + foo _
  (param x _)
  (print "called foo")
  (return 42)
)

(type + Thing
  (prop + b _ (= 0))
  (fn-expr + a _ :get
    (call foo this)
  )
  (fn-do + This _
    (lazy b a)
  )
)
```

```output
Exception: Illegal to reference getters in eager properties
<no file:19>
        (lazy b a)
                ^
```

```brix crash
(main
  (let thing (call Thing))
  (print (. thing b))
)

(type + Thing
  (prop + b _ (= this))
  (fn-do + This _)
  (fn-expr + foo _
    42
  )
)
```

```output
Exception: Illegal to reference 'this' in eager properties or properties referenced by them
<no file:8>
      (prop + b _ (= this))
                     ^^^^
```

```brix crash
(main
  (let thing (call Thing))
  (print (. thing b))
)

(type + Thing
  (prop + b _ (= this))
  (fn-do + This _)
  (fn-expr + foo _
    42
  )
)
```

```output
Exception: Illegal to reference 'this' in eager properties or properties referenced by them
<no file:8>
      (prop + b _ (= this))
                     ^^^^
```

```brix crash
(main
  (let thing (call Thing))
  (print (. thing b))
)

(type + Thing
  (prop + b _ (= 0))
  (fn-do + This _
    (lazy b this)
  )
  (fn-expr + foo _
    42
  )
)
```

```output
Exception: Illegal to reference 'this' in eager properties or properties referenced by them
<no file:10>
        (lazy b this)
                ^^^^
```

```brix crash
(main
  (let thing (call Thing))
  (print (. thing b))
)

(type + Thing
  (prop + b _ (= 0))
  (fn-do + This _
    (lazy b (call foo))
  )
  (fn-expr + foo _
    42
  )
)
```

```output
Exception: A ".." initializer runs before the object exists, so it cannot call "foo", a method of Thing; compute the value from the constructor's parameters, or assign it in the constructor's body
<no file:10>
        (lazy b (call foo))
                ^^^^^^^^^^
```

```brix
(main
  (let thing (call Thing))
  (print (. thing prop))
  (blank)
  (let proto Proto thing)
  (print (. proto prop))
)

(type + Thing
  (implements Proto)
  (prop + prop _ (= "Thing"))
)

(type + Proto (is Interface)
  (fn-expr + prop String :get
    "Proto"
  )
)
```

```output
Thing
Thing
```

```brix
(main
  (let thing (call Thing))
  (print (. thing prop))
  (blank)
  (let proto Proto thing)
  (print (. proto prop))
)

(type + Thing
  (implements Proto)
)

(type + Proto (is Interface)
  (fn-expr + prop String :get
    "Proto"
  )
)
```

```output
Proto
Proto
```

```brix
(main
  (let thing (call Thing))
  (print (. thing prop))
  (blank)
  (let proto Proto thing)
  (print (. proto prop))
)

(type + Thing
  (delegate Proto)
  (prop + proto ProtoThing)
)

(type + ProtoThing
  (implements Proto)
  (fn-expr + prop String :get
    "ProtoThing"
  )
)

(type + Proto (is Interface)
  (fn-expr + prop String :get
    "Proto"
  )
)
```

```output
ProtoThing
ProtoThing
```

```brix
(main
  (let thing (call Thing))
  (print (. thing prop))
  (blank)
  (let proto Proto thing)
  (print (. proto prop))
)

(type + Thing
  (delegate Proto)
  (prop + proto ProtoThing)
)

(type + ProtoThing
  (implements Proto)
  (prop + prop _ (= "ProtoThing"))
)

(type + Proto (is Interface)
  (fn-expr + prop String :get
    "Proto"
  )
)
```

```output
ProtoThing
ProtoThing
```

```brix
(main
  (let thing (call Thing))
  (print (. thing prop))
  (blank)
  (let proto Proto thing)
  (print (. proto prop))
)

(type + Thing
  (delegate Proto)
  (prop + proto ProtoThing)
)

(type + ProtoThing
  (delegate Proto)
  (prop + proto ProtoThing2)
)

(type + ProtoThing2
  (implements Proto)
  (prop + prop _ (= "ProtoThing2"))
)

(type + Proto (is Interface)
  (fn-expr + prop String :get
    "Proto"
  )
)
```

```output
ProtoThing2
ProtoThing2
```

### Tracked property assignment inside an override keeps the base method's error contract

```brix
(main
  (let child (call Child))
  (call (. child hook))
  (print (. child status))
  (print (. child count))
)

(type + Base
  (prop + count _ (= 0))
  (fn-do + hook _
    (+= count 0)
  )
)

(type + Child (is Base)
  (prop + status _ (= ""))
  (blank)
  (depends mark status)
  (fn-do + hook _
    (= status "changed")
  )
  (fn-do + mark _
    (+= count 1)
  )
)
```

```output
changed
1
```

## Setters

```brix
(main
  (let thing (call Thing))
  (= (. thing foo) 42)
  (print (. thing foo))
)

(type + Thing
  (prop + foos _ (= 10))
  (fn-expr + foo _ :get
    foos
  )
  (fn-do + foo _ :set (param x _)
    (= foos x)
  )
)
```

```output
42
```

```brix
(main
  (let thing (call Thing 24))
  (print (. thing half))
  (= (. thing half) 2)
)

(type + Thing (is Int)
  (fn-expr + half _ :get
    (/ this 2)
  )
  (fn-do + half _ :set (param x _)
    (print (str "set " x))
  )
)
```

```output
12
set 2
```

### Setter with eager property

```brix
(main
  (let thing (call Thing))
  (= (. thing foo) 42)
  (print (. thing foo))
)

(type + Thing
  (prop + foo _ (= 42))
  (fn-do + foo _ :set (param x _))
)
```

```output
42
```

### Setter with no getter

```brix crash
(main
  (let thing (call Thing))
  (= (. thing foo) 42)
  (print (. thing foo))
)

(type + Thing
  (fn-do + foo _ :set (param x _))
)
```

```output
Exception: Thing has no property "foo"
<no file:5>
      (print (. thing foo))
             ^^^^^^^^^^^^^
```

### Setter with generic type

```brix
(main
  (let thing (call Thing))
  (= (. thing foo) 42.5)
  (print (. thing foo))
  (= (. thing foo) 28u8)
  (print (. thing foo))
)

(type + Thing
  (prop + foos _ (= 10))
  (fn-expr + foo _ :get
    foos
  )
  (fn-do + foo _ :set (param x ''X)
    (print X)
    (= foos (as x Int))
  )
)
```

```output
[TypeInfo F64]
42
[TypeInfo U8]
28
```

### An assigning operator on a property a getter and a setter answer reads the value through the getter and stores the new one through the setter, after an object, and named alone in a subclass.

```brix
(main
  (let box (call Box))
  (+= (. box total) 2)
  (print (. box total))
  (++= (. box label) "!")
  (print (. box label))
  (call (. (call Kid) grow))
)

(type + Box
  (prop _ stored _ (= 1))
  (prop _ text _ (= "a"))
  (fn-expr + total Int :get
    stored
  )
  (fn-do + total _ :set (param value Int)
    (print (str "total = " value))
    (= stored value)
  )
  (fn-expr + label String :get
    text
  )
  (fn-do + label _ :set (param value String)
    (= text value)
  )
)

(type + Kid (is Box)
  (fn-do + grow _
    (*= total 5)
    (print total)
  )
)
```

```output
total = 3
3
a!
total = 5
5
```

### An inferred property can be read while its owner's types are still resolving

```brix
(main
  (let chooser (call FruitPromptChooser))
  (let palette (call PromptPalette))
  (print (. palette prompt))
  (= (. palette chooser) chooser)
  (print (. palette prompt))
)

(type + PromptChooser
  (prop + palette (? PromptPalette))
  (prop + prompt _ (= "Choose one"))
)

(type + FruitPromptChooser (is PromptChooser)
  (prop + chosen _ (= ""))
)

(type + PromptPalette
  (prop + chooser (? PromptChooser))
  (prop + prompt _ (= (call prompt-of #chooser)))
)

(fn-expr + prompt-of String
  (param chooser (? PromptChooser))
  (case? chooser
    (. chooser prompt)
  )
  (default
    "Type a command"
  )
)
```

```output
Type a command
Choose one
```

### A class is constructed while its types are still resolving, by the value of a property of a class one of its types names

```brix
(main
  (let chunk (call Chunk 3))
  (print (. chunk sprites length))
  (print (. chunk phase))
  (print (. chunk count))
  (print (. chunk is-open))
  (+= (. chunk sprites) (call Sprite))
  (print (. chunk sprites length))
  (print (. ([] (. chunk sprites) 0) chunk phase))
)

(type + Chunk
  (prop + phase U8)
  (prop + count _ (= 7))
  (prop + is-open _ (= false))
  (prop + sprites (list Sprite))
  (fn-do + This _
    (param chunk-phase U8)
    (= phase chunk-phase)
  )
)

(type + Sprite
  (prop + chunk _ (= (call Chunk 1)))
)
```

```output
0
3
7
false
1
1
```

### A property default reads its declared union type when several unions share the case.

```brix
(main
  (let a (call Plain))
  (print (is (. a m) .none))
  (let b (call Concrete))
  (print (is (. b m) .none))
)

(type + Choice (is Union)
  (variant some (tuple Int))
  (variant none)
)

(type + (of Maybe T) (is Union)
  (variant some (tuple T))
  (variant none)
)

(type + Plain
  (prop + m Choice (= .none))
)

(type + Concrete
  (prop + m (of Maybe Int) (= .none))
)
```

```output
true
true
```

### A lazy property a subclass declares again takes the value of the object's class.

```brix
(main
  (let b Base (call Child))
  (print (. b a))
  (print (. b a))
  (let c Base (call Base))
  (print (. c a))
)

(type + Base
  (prop + a _ (:= (call say 1)))
)

(type + Child (is Base)
  (prop + a _ (:= (call say 2)))
)

(fn-do + say Int
  (param v Int)
  (print (str "say " v))
  (return v)
)
```

```output
say 2
2
2
say 1
1
```

### A method of the base reads a lazy property the subclass declares again.

```brix
(main
  (let c (call Child))
  (print (call (. c read)))
  (let b (call Base))
  (print (call (. b read)))
)

(type + Base
  (prop + a _ (:= (call say 1)))
  (fn-expr + read Int
    a
  )
)

(type + Child (is Base)
  (prop + a _ (:= (call say 2)))
)

(fn-do + say Int
  (param v Int)
  (print (str "say " v))
  (return v)
)
```

```output
say 2
2
say 1
1
```

### A lazy property a constructor's argument fills is never evaluated.

```brix
(main
  (let t (call Thing 5))
  (print "made")
  (print (. t a))
  (let u (call Thing))
  (print "made")
  (print (. u a))
)

(type + Thing
  (prop + a _ (:= (call say 1)))
  (fn-do + This _)
  (fn-do + This _
    (param a _)
  )
)

(fn-do + say Int
  (param v Int)
  (print (str "say " v))
  (return v)
)
```

```output
made
5
made
say 1
1
```

### A value that reads a lazy property evaluates it once, and a later read does not evaluate it again.

```brix
(main
  (let t (call Thing))
  (print "made")
  (print (. t b))
  (print (. t a))
)

(type + Thing
  (prop + a _ (:= (call say 1)))
  (prop + b _ (= (+ a (call say 10))))
)

(fn-do + say Int
  (param v Int)
  (print (str "say " v))
  (return v)
)
```

```output
say 1
say 10
made
11
1
```

### A task that reads a lazy property another task is evaluating waits for its value instead of evaluating it again.

```brix
(main
  (let box (call Box))
  (let values (call (of SyncBufferedChannel Int)))
  (do-new
    (<- values (. box value))
  )
  (do-new
    (<- values (. box value))
  )
  (do-new
    (call (. box gate close))
  )
  (print (<- values))
  (print (<- values))
)

(type + Box
  (prop + gate _ (= (call (of SyncBufferedChannel Int))))
  (prop + value _ (:= (call make)))
  (fn-do _ make Int
    (print "made")
    (<- gate)
    (return 7)
  )
)
```

```output
made
7
7
```

### A task that waits on a lazy property reads the value assigned while another task evaluated it, though that evaluation fails.

```brix
(main
  (let box (call Box))
  (let values (call (of SyncBufferedChannel Int)))
  (do-new
    (let failed (catch? (. box value)))
    (if (not failed)
      (<- values -1)
    )
  )
  (do-new
    (let got (catch? (. box value)))
    (if got
      (<- values got)
    )
  )
  (do-new
    (= (. box value) 9)
    (call (. box gate close))
  )
  (print (<- values))
  (print (<- values))
)

(type + Failed (is Error))

(type + Box
  (prop + fails _ (= true))
  (prop + gate _ (= (call (of SyncBufferedChannel Int))))
  (prop + value _ (:= (call make)))
  (fn-do _ make Int (?! Failed)
    (print "made")
    (<- gate)
    (if fails
      (throw (call Failed "no"))
    )
    (return 7)
  )
)
```

```output
made
-1
9
```

### An assigning operator assigns a property of a forced optional and of a cast object.

```brix
(main
  (let box (? Box) (call Box))
  (+= (. (!! box) size) 2)
  (print (. (!! box) size))
  (let thing Thing (call Bucket))
  (+= (. (as thing Bucket) fill) 1.5)
  (print (. (as thing Bucket) fill))
)

(type + Box
  (prop + size Size (= 1))
)

(type + Thing
  (prop + name _ (= "thing"))
)

(type + Bucket (is Thing)
  (prop + fill F64 (= 0))
)
```

```output
3
1.5
```

### An assigning operator evaluates a lazy property before it assigns.

```brix
(main
  (let t (call Thing))
  (+= (. t a) 2)
  (print (. t a))
  (let u (call Thing))
  (= (. u a) 9)
  (print (. u a))
)

(type + Thing
  (prop + a _ (:= (call say 1)))
)

(fn-do + say Int
  (param v Int)
  (print (str "say " v))
  (return v)
)
```

```output
say 1
3
9
```

### A lazy property whose value panics: the trace names the property with its class.

```brix crash
(main
  (let t (call Thing))
  (call show t)
  (print "after")
)

(fn-do + show _
  (param t Thing)
  (print (. t a))
  (print "not here")
)

(type + Thing
  (prop + a _ (:= (call boom)))
)

(fn-do + boom Int
  (throw (call Panic "lazy boom"))
  (return 1)
)
```

```stderr
Unhandled panic: lazy boom
<no file:19>

Task 1

Stack trace:
  0  __source__:boom no file:19:3
  1  __source__:Thing:a no file:15:19
  2  __source__:show no file:10:10
  3  __source__:main no file:4:3
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### A lazy property is evaluated where a value first reads it.

```brix
(main
  (let k (call Thing))
  (print "made")
  (print (. k a))
  (print (. k b))
)

(type + Thing
  (prop + a _ (:= (call say 1)))
  (prop + c _ (= (call say 5)))
  (prop + b _ (= (+ a (call say 10))))
)

(fn-do + say Int
  (param v Int)
  (print (str "say " v))
  (return v)
)
```

```output
say 5
say 1
say 10
made
1
11
```

### `super(…)` evaluates again a lazy property that a value it stores again reads.

```brix
(main
  (let k (call Kid 7))
  (print "made")
  (print (. k a))
  (print (. k b))
  (print (. k c))
)

(type + Thing
  (prop + z _ (= 0))
  (prop + a _ (:= (+ (call say 1) z)))
  (prop + c _ (= (+ (call say 5) z)))
  (prop + b _ (= (+ a (call say 10))))
  (fn-do + This _
    (param z _)
  )
)

(type + Kid (is Thing)
  (fn-do + This _
    (param n Int)
    (call super n)
  )
)

(fn-do + say Int
  (param v Int)
  (print (str "say " v))
  (return v)
)
```

```output
say 5
say 1
say 10
say 5
say 1
say 10
made
8
18
12
```

### `super(…)` leaves a lazy property that no value it stores again reads.

```brix
(main
  (let k (call Kid 7))
  (print "made")
  (print (. k a))
  (print (. k z))
  (print (. k c))
)

(type + Thing
  (prop + z _ (= 0))
  (prop + a _ (:= (+ (call say 1) z)))
  (fn-do + This _
    (param z _)
  )
)

(type + Kid (is Thing)
  (prop + c _ (= (+ a 1)))
  (fn-do + This _
    (param n Int)
    (call super n)
  )
)

(fn-do + say Int
  (param v Int)
  (print (str "say " v))
  (return v)
)
```

```output
say 1
made
1
7
2
```

### `super(…)` stores a lazy property its argument fills.

```brix
(main
  (let k (call Kid 7))
  (print "made")
  (print (. k a))
  (print (. k b))
)

(type + Thing
  (prop + a _ (:= (call say 1)))
  (prop + b _ (= (+ a 1)))
  (fn-do + This _
    (param a _)
  )
)

(type + Kid (is Thing)
  (fn-do + This _
    (param n Int)
    (call super n)
  )
)

(fn-do + say Int
  (param v Int)
  (print (str "say " v))
  (return v)
)
```

```output
say 1
made
7
8
```

### `super(…)` evaluates lazy properties through one another, with the values of the constructor's class.

```brix
(main
  (let k (call Kid 7))
  (print "made")
  (print (. k b))
  (print (. k c))
  (print (. k a))
  (print (. k d))
)

(type + Thing
  (prop + z _ (= 0))
  (prop + d _ (:= (+ (call say 4) a)))
  (prop + a _ (:= (+ (call say 1) z)))
  (prop + b _ (= (+ (call say 2) d)))
  (prop + c _ (:= (+ (call say 3) z)))
  (fn-do + This _
    (param z _)
    (print (str "thing " z " " b))
  )
)

(type + Kid (is Thing)
  (prop + a _ (:= (+ (call say 6) z)))
  (prop + e _ (= (+ c 1)))
  (fn-do + This _
    (param n Int)
    (call super n)
    (print (str "kid " e))
  )
)

(fn-do + say Int
  (param v Int)
  (print (str "say " v))
  (return v)
)
```

```output
say 2
say 4
say 6
say 3
say 2
say 4
say 1
thing 7 14
kid 4
made
14
3
8
12
```

### A lazy property a class two below declares again does not share the flag of another.

```brix
(main
  (let l (call Leaf))
  (print (. l a))
  (print (. l c))
  (print (+ (. l a) (. l c)))
)

(type + Base
  (prop + a _ (:= (call say 1)))
)

(type + Mid (is Base)
  (prop + c _ (:= (call say 30)))
)

(type + Leaf (is Mid)
  (prop + a _ (:= (call say 100)))
)

(fn-do + say Int
  (param v Int)
  (print (str "say " v))
  (return v)
)
```

```output
say 100
100
say 30
30
130
```

### The value of a property names a property of its class, not a local of the module's statements.

```brix
(main
  (let foo (call Foo))
  (let bar 7)
  (print (. foo a))
  (print (. foo c))
  (print (. foo d))
  (print bar)
)

(type + Foo
  (prop + foo _ (= 42))
  (prop + bar _ (= 1))
  (prop + a _ (=> b))
  (prop + b _ (:= foo))
  (prop + c _ (= (+ foo bar)))
  (prop + d _ (:= (+ bar 1)))
)
```

```output
42
43
2
7
```

### A constructor argument is converted to the type of the property it fills.

```brix
(main
  (let own (call Own 3))
  (print (/ (. own a) 8))
  (let typed (call Typed 3))
  (print (/ (. typed a) 8))
  (let under (call Under 3))
  (print (/ (. under a) 8))
  (let again (call Again 3))
  (print (/ (. again a) 8))
  (let wide (call Wide 3))
  (print (. wide a))
  (print (. (type-of (. wide a)) name))
)

(type + Own
  (prop + a _ (= 42.0))
  (fn-do + This _
    (param a I32)
  )
)

(type + Typed
  (prop + a F64)
  (fn-do + This _
    (param a I32)
  )
)

(type + Base
  (prop + a _ (= 42.0))
)

(type + Under (is Base)
  (fn-do + This _
    (param a I32)
  )
)

(type + Again (is Base)
  (prop + a _ (= 99))
  (fn-do + This _
    (param a I32)
  )
)

(type + Wide
  (prop + a _ (= 42))
  (fn-do + This _
    (param a I64)
  )
)
```

```output
0.375
0.375
0.375
0.375
3
I64
```

### A struct or tuple made by a constructor converts the argument that fills a field.

```brix
(main
  (let point (call Point 3))
  (print (/ (. point x) 8))
  (let pair (call Pair 3))
  (print (/ (. pair x) 8))
)

(type + Point (is Struct)
  (prop + x F64)
  (fn-do + This _
    (param x I32)
  )
)

(type + Pair (is Tuple)
  (prop + x F64)
  (fn-do + This _
    (param x I32)
  )
)
```

```output
0.375
0.375
```

### A constructor's untyped parameter that fills a field with a written type takes that type, so its body reads the argument converted as the field holds it.

```brix
(main
  (let text Hex "0a0b")
  (let holder (call Holder text))
  (print (. holder size))
  (print (. holder bytes size))
)

(type + Holder
  (prop + bytes ([] Byte))
  (prop + size Size (= 0))
  (fn-do + This _
    (param bytes _)
    (= size (. bytes size))
  )
)
```

```output
2
2
```

### The value a class with a type parameter writes for a property is typed for each class that holds it.

```brix
(main
  (let a (call (of Box Int)))
  (let b (call (of Box Float)))
  (let c (call Crate))
  (print (. a held))
  (print (. b held))
  (print (. c held))
  (print (. a twice))
  (print (. b twice))
  (print (. c twice))
  (print (. c more))
  (let p (call (of Pair Float)))
  (print p)
  (let q (call (of Pair Int)))
  (print q)
)

(fn-expr + (of make T) T
  (param from Int)
  (/ (call T from) (call T 2))
)

(type + (of Box T)
  (prop + held _ (= (call (of make T) 5)))
  (prop + twice _ (:= (+ held held)))
)

(type + Crate (is (of Box Float))
  (prop + more _ (= (* held 3)))
)

(type + (of Pair T) (is Tuple)
  (prop + first _ (= (call (of make T) 7)))
  (prop + second T)
)
```

```output
2
2.5
2.5
4
5
5
7.5
(3.5, 0)
(3, 0)
```

### A lazy local of a lazy property's name fills it where the constructor runs.

```brix
(main
  (let thing (call Thing))
  (print "made")
  (print (. thing a))
  (print (. thing a))
)

(fn-do + tick _
  (print "call tick")
  (return 42)
)

(fn-do + fill _
  (print "call fill")
  (return 7)
)

(type + Thing
  (prop + a _ (:= (call tick)))
  (fn-do + This _
    (lazy a (call fill))
  )
)
```

```output
call fill
made
7
7
```

### A lazy property a constructor fills from an argument, read by another and assigned.

```brix
(main
  (let thing (call Thing 3))
  (print "made")
  (print (. thing a))
  (print (. thing b))
  (= (. thing a) 5)
  (print (. thing a))
)

(fn-do + tick _
  (print "call tick")
  (return 42)
)

(type + Thing
  (prop + a _ (:= (call tick)))
  (prop + b _ (:= (+ a 1)))
  (fn-do + This _
    (param n Int)
    (print "in")
    (lazy a (* n 2))
  )
)
```

```output
in
made
6
7
5
```

### Lazy locals fill lazy properties in the order the properties are declared, before the body.

```brix
(main
  (let thing (call Thing 3))
  (print "made")
  (print (. thing a))
  (print (. thing b))
)

(fn-do + fill _
  (param n Int)
  (print (str "call fill " n))
  (return n)
)

(type + Thing
  (prop + c _ (= (call fill 1)))
  (prop + a _ (:= (call fill 2)))
  (prop + b _ (:= (call fill 3)))
  (prop + d _ (= (call fill 4)))
  (fn-do + This _
    (param n Int)
    (print "in")
    (lazy a (call fill (* n 10)))
    (print "out")
  )
)
```

```output
call fill 1
call fill 30
call fill 4
in
out
made
30
call fill 3
3
```

### A lazy property of a text filled by a lazy local that chooses.

```brix
(main
  (let thing (call Thing "x"))
  (print "made")
  (print (. thing name))
  (print (. thing size))
  (= (. thing name) "longer")
  (print (. thing size))
  (let bare (call Thing ""))
  (print (. bare name))
)

(type + Thing
  (prop + name String (:= "none"))
  (prop + size _ (:= (. name length)))
  (fn-do + This _
    (param given String)
    (lazy name (if-expr (. given length) given "empty"))
  )
)
```

```output
made
x
1
1
empty
```

### A nested function a constructor keeps reads the lazy local that filled a property.

```brix
(main
  (let made (call Box))
  (print (str "made " (. made held)))
  (print (call (. made run)))
  (print (call (. made run)))
)

(blank)
(prop + count _ (= 0))

(fn-do + tick Int
  (param name String)
  (+= count 1)
  (print (str "tick " name " " count))
  (return count)
)

(type + Get (is (fn-type Int)))

(type + Box
  (prop + held Int)
  (prop + get Get)
  (fn-do + This _
    (lazy held (call tick "held"))
    (lazy get (=> () held))
    (print "body")
  )
  (fn-expr + run Int
    (call get)
  )
)
```

```output
tick held 1
body
made 1
1
1
```

### A lazy local only a kept nested function reads is evaluated when that first runs.

```brix
(main
  (let made (call Box))
  (print "made")
  (print (call (. made run)))
  (print (call (. made run)))
)

(blank)
(prop + count _ (= 0))

(fn-do + tick Int
  (param name String)
  (+= count 1)
  (print (str "tick " name " " count))
  (return count)
)

(type + Get (is (fn-type Int)))

(type + Box
  (prop + get Get)
  (fn-do + This _
    (lazy extra (call tick "extra"))
    (lazy get (=> () (+ extra 100)))
    (print "body")
  )
  (fn-expr + run Int
    (call get)
  )
)
```

```output
body
made
tick extra 1
101
101
```

### A nested function written in a constructor's body captures a local and an argument.

```brix
(main
  (let made (call Box 4))
  (print (str "made " (. made held)))
  (print (call (. made run)))
)

(blank)
(prop + count _ (= 0))

(fn-do + tick Int
  (param name String)
  (+= count 1)
  (print (str "tick " name " " count))
  (return count)
)

(type + Get (is (fn-type Int)))

(type + Box
  (prop + held Int)
  (prop + get Get)
  (fn-do + This _
    (param seed Int)
    (lazy held (+ (call tick "held") seed))
    (let local (* seed 2))
    (= get (=> () (+ local seed)))
    (print (str "body " local))
  )
  (fn-expr + run Int
    (call get)
  )
)
```

```output
tick held 1
body 8
made 5
12
```

### A nested function a constructor keeps reads the object it made.

```brix
(main
  (let made (call Named "ann"))
  (print (call (. made say)))
  (= (. made name) "bob")
  (print (call (. made say)))
)

(type + Greet (is (fn-type String)))

(type + Named
  (prop + name String)
  (prop + greet Greet)
  (fn-do + This _
    (param name _)
    (= greet (=> () (str "hi " (. this name))))
  )
  (fn-expr + say String
    (call greet)
  )
)
```

```output
hi ann
hi bob
```

### Nested functions a constructor keeps capture its argument and a local of its body.

```brix
(main
  (let made (call Named "ann"))
  (print (call (. made say)))
  (= (. made name) "bob")
  (print (call (. made say)))
  (print (call (. made count)))
  (print (call (. made count)))
)

(type + Greet (is (fn-type String)))

(type + Count (is (fn-type Int)))

(type + Named
  (prop + name String)
  (prop + greet Greet)
  (prop + counter Count)
  (fn-do + This _
    (param name _)
    (let calls 0)
    (= greet (=> () (str "hi " name)))
    (= counter
      (=> ()
        (block
          (do
            (+= calls 1)
          )
          calls
        )
      )
    )
  )
  (fn-expr + say String
    (call greet)
  )
  (fn-expr + count Int
    (call counter)
  )
)
```

```output
hi ann
hi ann
1
2
```

### A property stored where the class delegates an interface: read from outside, by its name alone in a method, and through a value of the interface.

```brix
(main
  (let thing (call Thing))
  (print (. thing prop))
  (call (. thing show))
  (let proto Proto thing)
  (print (. proto prop))
)

(type + Thing
  (delegate Proto)
  (prop + proto ProtoThing)
  (fn-do + show _
    (print prop)
  )
)

(type + ProtoThing
  (implements Proto)
  (prop + prop _ (= "held"))
)

(type + Proto (is Interface)
  (fn-expr + prop String :get
    "Proto"
  )
)
```

```output
held
held
held
```

### A class derived from one that delegates reads the delegated property, held as itself, as the interface and as its base.

```brix
(main
  (let thing (call Derived))
  (print (. thing prop))
  (let proto Proto thing)
  (print (. proto prop))
  (let base Base thing)
  (print (. base prop))
)

(type + Base
  (delegate Proto)
  (prop + proto ProtoThing)
)

(type + Derived (is Base)
  (prop + extra _ (= 3))
)

(type + ProtoThing
  (implements Proto)
  (prop + prop _ (= "held"))
)

(type + Proto (is Interface)
  (fn-expr + prop String :get
    "Proto"
  )
)
```

```output
held
held
held
```

### A class of one interface that delegates another: the delegate's stored properties answer both names, also through a value.

```brix
(main
  (let thing (call Thing))
  (print (. thing prop))
  (print (. thing other))
  (let proto Proto thing)
  (print (. proto prop))
  (print (. proto other))
)

(type + Thing
  (implements Other)
  (delegate Proto)
  (prop + proto ProtoThing)
)

(type + ProtoThing
  (implements Proto)
  (prop + prop _ (= "held"))
  (prop + other _ (= "held other"))
)

(type + Proto (is Interface)
  (fn-expr + prop String :get
    "Proto"
  )
  (fn-expr + other String :get
    "Proto other"
  )
)

(type + Other (is Interface)
  (fn-expr + other String :get
    "Other"
  )
)
```

```output
held
held other
held
held other
```

### An interface of the base class answers a name before a property the derived class delegates to.

```brix
(main
  (let thing (call Derived))
  (print (. thing prop))
  (let proto Proto thing)
  (print (. proto prop))
)

(type + Base
  (implements Named)
  (prop + kept _ (= 1))
)

(type + Derived (is Base)
  (delegate Proto)
  (prop + proto ProtoThing)
)

(type + ProtoThing
  (implements Proto)
  (prop + prop _ (= "held"))
)

(type + Proto (is Interface)
  (fn-expr + prop String :get
    "Proto"
  )
)

(type + Named (is Interface)
  (fn-expr + prop String :get
    "Named"
  )
)
```

```output
Named
Named
```

### A delegate held as the interface itself: the property is read from what it holds, and a value's slot runs the interface's getter.

```brix
(main
  (let thing (call Thing))
  (print (. thing prop))
  (let proto Proto thing)
  (print (. proto prop))
)

(type + Thing
  (delegate Proto)
  (prop + proto Proto (= (call ProtoThing)))
)

(type + ProtoThing
  (implements Proto)
  (prop + prop _ (= "held"))
)

(type + Proto (is Interface)
  (fn-expr + prop String :get
    "Proto"
  )
)
```

```output
held
Proto
```

### Getters an interface declares with no body, answered by properties the delegate stores.

```brix
(main
  (let thing (call Thing))
  (print (. thing prop))
  (print (. thing size))
  (let proto Proto thing)
  (print (. proto prop))
  (print (. proto size))
)

(type + Thing
  (delegate Proto)
  (prop + proto ProtoThing)
)

(type + ProtoThing
  (implements Proto)
  (prop + prop _ (= "held"))
  (prop + size _ (= 4))
)

(type + Proto (is Interface)
  (fn-expr + prop String :get)
  (fn-expr + size Int :get)
)
```

```output
held
4
held
4
```

### A struct a class delegates to stores the property.

```brix
(main
  (let thing (call Thing))
  (print (. thing prop))
  (print (. thing pair prop))
)

(type + Thing
  (delegate Proto)
  (prop + pair Pair)
)

(type + Pair (is Struct)
  (implements Proto)
  (prop + prop _ (= "in a struct"))
  (prop + more _ (= 2))
)

(type + Proto (is Interface)
  (fn-expr + prop String :get
    "Proto"
  )
)
```

```output
in a struct
in a struct
```

### A delegate that stores no property of the name leaves the interface's getter to answer.

```brix
(main
  (let thing (call Thing))
  (print (. thing prop))
  (let proto Proto thing)
  (print (. proto prop))
)

(type + Thing
  (delegate Proto)
  (prop + proto ProtoThing)
)

(type + ProtoThing
  (implements Proto)
  (prop + kept _ (= 1))
)

(type + Proto (is Interface)
  (fn-expr + prop String :get
    "Proto"
  )
)
```

```output
Proto
Proto
```

### Properties of a delegate read by their names alone in a method, used in an operator and kept in a local, and one assigned through the delegate.

```brix
(main
  (let thing (call Thing))
  (call (. thing show))
  (call (. thing change))
  (call (. thing show))
)

(type + Thing
  (delegate Proto)
  (prop + proto ProtoThing)
  (fn-do + show _
    (print prop)
    (print (+ size 1))
    (let kept prop)
    (print (. kept length))
  )
  (fn-do + change _
    (= (. proto prop) "changed")
  )
)

(type + ProtoThing
  (implements Proto)
  (prop + prop _ (= "held"))
  (prop + size _ (= 4))
)

(type + Proto (is Interface)
  (fn-expr + prop String :get)
  (fn-expr + size Int :get)
)
```

```output
held
5
4
changed
5
7
```

### A local declared after a delegated name is read shadows it from there on.

```brix
(main
  (let thing (call Thing))
  (call (. thing show))
  (print (. thing prop))
)

(type + Thing
  (delegate Proto)
  (prop + proto ProtoThing)
  (fn-do + show _
    (print prop)
    (let prop "shadow")
    (print prop)
  )
)

(type + ProtoThing
  (implements Proto)
  (prop + prop _ (= "held"))
)

(type + Proto (is Interface)
  (fn-expr + prop String :get)
)
```

```output
held
shadow
held
```

### A property two delegations deep, and a value of an interface the inner class delegates.

```brix
(main
  (let thing (call Thing))
  (print (. thing prop))
  (print (. thing size))
  (let proto Proto thing)
  (print (. proto prop))
  (print (. proto size))
  (let deep Deep (. thing inner))
  (print (. deep size))
)

(type + Thing
  (delegate Proto)
  (prop + inner Inner)
)

(type + Inner
  (delegate Proto Deep)
  (prop + held Holder)
)

(type + Holder
  (implements Proto)
  (implements Deep)
  (prop + prop _ (= "two deep"))
  (prop + size _ (= 9))
)

(type + Proto (is Interface)
  (fn-expr + prop String :get)
  (fn-expr + size Int :get)
)

(type + Deep (is Interface)
  (fn-expr + size Int :get)
)
```

```output
two deep
9
two deep
9
9
```

### A getter of a delegate reads the delegate's property when read from the class, and the class's own when a value of the interface runs it.

```brix
(main
  (let thing (call Thing))
  (print (. thing size))
  (let proto Proto thing)
  (print (. proto size))
)

(type + Proto (is Interface)
  (fn-expr + size String :get)
)

(type + Foo
  (implements Proto)
  (prop + label _ (= "foo"))
  (fn-expr + size String :get
    label
  )
)

(type + Thing
  (delegate Proto)
  (prop + foo Foo)
  (blank)
  (prop + label _ (= "thing"))
)
```

```output
foo
thing
```

### A constructor body reads and writes the field its .. initializer made, evaluated once.

```brix
(prop + made _ (= 0))
(blank)

(main
  (let foo (call Foo 3))
  (= ([] (. foo values) 2) 9)
  (print (tuple ([] (. foo values) 0) ([] (. foo values) 1) ([] (. foo values) 2) made))
)

(type + Foo
  (prop + values ([] I64))
  (fn-do + This _
    (param count Size)
    (lazy values (call make-values count))
    (= ([] values 0) 5)
    (= ([] values 1) (+ ([] values 0) 2))
  )
)

(fn-do + make-values ([] I64)
  (param count Size)
  (+= made 1)
  (return ([] I64 count))
)
```

```output
(5, 7, 9, 1)
```

### A .. initializer written after the constructor body still runs before it.

```brix
(main
  (let foo (call Foo))
  (print (tuple (. foo items length) (. foo first)))
)

(type + Foo
  (prop + items (list Int))
  (prop + first Int)
  (fn-do + This _
    (+= items 4)
    (= first ([] items 0))
    (blank)
    (lazy items (call (of List Int)))
  )
)
```

```output
(1, 4)
```

### A base constructor body writes the field its .. initializer made.

```brix
(main
  (let bar (call Bar))
  (print ([] (. bar values) 0))
)

(type + Foo
  (prop + values ([] I64))
  (fn-do + This _
    (lazy values ([] I64 2))
    (= ([] values 0) 3)
  )
)

(type + Bar (is Foo)
  (fn-do + This _
    (call super)
  )
)
```

```output
3
```

### A .. initializer cannot read a field the constructor body assigns, since it runs first.

```brix crash
(main
  (let foo (call Foo))
)

(type + Foo
  (prop + base Int)
  (prop + total Int)
  (fn-do + This _
    (= base 7)
    (lazy total (+ base 1))
  )
)
```

```output
Exception: Initializer of total reads base before the constructor body assigns it; constructor .. initializers run before the body
<no file:11>
        (lazy total (+ base 1))
                       ^^^^
```

### A lazy property of a class no object of which is ever made reads as nothing through an optional that never holds one.

```brix
(main
  (let reader (call Reader))
  (print (. reader value))
)

(type + Holder
  (prop + seed _ (= 4))
  (prop + table _ (:= (* seed 2)))
)

(type + Reader
  (prop + holder (? Holder))
  (fn-expr + value Int :get
    (?? (?. holder table) 0)
  )
)
```

```output
0
```

### A property a value reads on one path of a condition alone gets its first value there, and on the other path in its own turn, once each, as a flag says; so does one a value so given reads.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let one (call Tag false))
  (print (. one text))
  (let two (call Tag true))
  (print (. two text))
)

(fn-do + mark String
  (param text String)
  (print (str "mark " text))
  (return text)
)

(type + Plain (is Box)
  (prop + text _ (= (call mark "plain")))
)

(type + Tag (is Plain)
  (prop + flag Bool)
  (prop + suffix _ (= (++ (call mark "s") (str count))))
  (prop + count _ (= (call mark "7")))
  (prop + last _ (= (call mark "last")))
  (fn-do + This _
    (param flag _)
  )
  (content
    (<> This
      (= text (++ (if-expr flag (call mark "a") (str "b" suffix)) (str suffix last)))
    )
  )
)
```

```output
mark s
mark 7
mark last
bs7s7last
mark a
mark s
mark 7
mark last
as7last
```

### A constructor gives a property the value of a lazy local that is a list of feeds reading an argument.

```brix
(main
  (let m (call Message "hi"))
  (for (in part (. m parts))
    (print part)
  )
)

(type + Message
  (prop + parts (list String))
  (fn-do + This _
    (param text String)
    (lazy parts
      (block
        (item (str "text: " text))
        (item "end")
      )
    )
  )
)
```

```output
text: hi
end
```

### A lambda a derived class gives as the value of a property its base declares is made where the object is, and captures nothing.

```brix
(main
  (let holder (call Derived))
  (print (call (. holder keyed) (call Entry "a" 1)))
  (print (call (. holder keyed) (call Entry "b" 2)))
  (let second (call Derived))
  (print (call (. second keyed) (call Entry "c" 3)))
)

(type + Entry (is Tuple)
  (prop + key String)
  (prop + size I64)
)

(type + KeyOf (is (fn-type String (param entry Entry))))

(type + Base
  (prop + prefix _ (= "p"))
  (prop + key-builder (? KeyOf))
  (fn-do + keyed String
    (param entry Entry)
    (let builder key-builder)
    (return (if-expr builder (str prefix ":" (call builder entry)) "none"))
  )
)

(type + Derived (is Base)
  (prop + key-builder _ (= (=> (entry) (. entry key))))
)
```

```output
p:a
p:b
p:c
```

### A property with no written type holds a lambda that captures nothing, called through the object with an argument of a stated type and with one its parameter leaves open.

```brix
(main
  (let table (call Table))
  (print (call (. table key-builder) (call Entry "a" 1)))
  (print (call (. table wrap) (call Entry "b" 2)))
  (print (call (. table wrap) 7))
)

(type + Entry
  (prop + key String)
  (prop + value Int)
  (fn-do + This _
    (param key _)
    (param value _)
  )
)

(type + Table
  (prop + key-builder _ (= (=> ((: entry Entry)) (. entry key))))
  (prop + wrap _ (= (=> (item) (str "<" item ">"))))
)
```

```output
a
<[Object __source__:Entry]>
<7>
```

### A getter of a name a property of its class took redefines the property, though nothing uses the class.

```brix crash
(main
  (call print "ok")
)

(type + Lost
  (prop + a Int)
  (fn-expr + a Int :get
    3
  )
)
```

```output
Exception: Lost declares "a" twice (first on line 7); remove one of them, or rename it
<no file:8>
      (fn-expr + a Int :get
      ^^^^^^^^^^^^^^^^^^^^
```

### A class first used in a function called from the module's statements is checked where it is used.

```brix crash
(main
  (call print (call make))
)

(fn-expr _ make Int
  (. (call Holder) size)
)

(type + Holder
  (prop + size _ (= 3))
  (prop + a _ (= b))
  (prop + b _ (= a))
)
```

```output
Exception: "a" depends on its own value, directly or through the values it reads; break the cycle
<no file:12>
      (prop + a _ (= b))
      ^^^^^^^^^^^^^^^^^^
```

### A getter answers with a property an arm tests.

```brix
(main
  (let frame (call Frame ([] Byte 2)))
  (print (. frame body length))
)

(type + Frame (is Tuple)
  (prop + payload ([] Byte))
  (fn-expr + body ([] Byte) :get
    (case? payload)
    (default
      (inline ([] Byte 0))
    )
  )
)
```

```output
2
```

### A property whose written type names nothing is an error where the type is written.

```brix crash
(main
  (let holder (call Holder))
  (print (. holder count))
)

(type _ Holder
  (prop + count _ (= 0))
  (prop _ missing (? Nonexistent))
)
```

```output
Exception: Type "Nonexistent" not found
<no file:9>
      (prop _ missing (? Nonexistent))
                         ^^^^^^^^^^^
```

### A property with no value written holds what its class constructs with no argument. Where those constructions need one another in a cycle they never finish, so the property that starts the cycle is an error.

```brix compile-error
(main
  (let owner (call Owner "x"))
  (print (. owner name))
)

(type + Owner
  (prop + name String)
  (prop + part Part)
  (fn-do + This _
    (param name _)
  )
)

(type + Part
  (prop + owner Owner)
  (fn-do + This _
    (param owner _)
  )
)
```

```output
Exception: Default of part never finishes: Part.owner builds Owner, and Owner.part builds Part again. Give one of these fields a value, make it lazy with :=, or make it optional
<no file:9>
      (prop + part Part)
      ^^^^^^^^^^^^^^^^^^
```

### A property's value can pipe a stream through a function

```brix
(main
  (let holder (call Holder))
  (print (. holder large length))
  (print ([] (. holder doubled) 0))
  (= (. holder values) (list 4 5))
  (print ([] (. holder doubled) 0))
  (print (. holder doubled length))
)

(type + Holder
  (prop + values _ (= (list 1 2 3)))
  (prop + large (list Int) (= (call List (?> (.. 1 6) (> each 4)))))
  (prop + doubled (list Int) (= (call List (*> #values (* each 2)))))
)
```

```output
2
2
8
2
```

```brix
(main
  (let nodes (call (of List Node) (call Node)))
  (-= (. ([] nodes 0) vx) 2.0)
  (*= (. ([] nodes 0) vx) 3.0)
  (print (. ([] nodes 0) vx))
)

(type + Node
  (prop + vx Float (= 4))
)
```

```output
6
```

```brix
(prop _ trace _ (= ""))

(main
  (let nodes (call (of List Node) (call Node)))
  (let original ([] nodes 0))
  (-= (. ([] nodes (call index)) vx) (call amount nodes))
  (print trace)
  (print (tuple (. original vx) (. ([] nodes 0) vx)))
)

(type + Node
  (prop + vx Float (= 4))
)

(fn-do + index Int
  (+= trace "i")
  (return 0)
)

(fn-do + amount Float
  (param nodes (of List Node))
  (+= trace "r")
  (= ([] nodes 0) (call Node))
  (return 2)
)
```

```output
iri
(4, 2)
```
