# Interfaces

```brix
(main
  (let thing (call Thing))
  (call foo thing)
)

(type + Proto (is Interface)
  (fn-expr + bar Int)
)

(type + Thing
  (implements Proto)
  (fn-expr + bar _
    42
  )
)

(fn-do + foo _
  (param p Proto)
  (print (call (. p bar)))
)
```

```output
42
```

```brix
(main
  (let thing (call Thing))
  (call foo thing)
)

(type + Proto (is Interface)
  (fn-expr + bar Int :get)
)

(type + Thing
  (implements Proto)
  (fn-expr + bar _ :get
    42
  )
)

(fn-do + foo _
  (param p Proto)
  (print (. p bar))
)
```

```output
42
```

```brix crash
(main
  (print (call Proto))
)

(type + Proto (is Interface)
  (fn-expr + bar Int)
)
```

```output
Exception: Proto is an interface, so it has no constructor; construct a class that is Proto
<no file:3>
      (print (call Proto))
                   ^^^^^
```

```brix
(main
  (print (call Proto))
)

(type + Proto (is Interface)
  (fn-do + This _
    (return 42)
  )
)
```

```output
42
```

### Default function implementation

```brix
(main
  (let thing (call Thing))
  (call foo thing)
)

(type + Proto (is Interface)
  (fn-do + bar _
    (print this)
    (call foo)
    (return 42)
  )
  (fn-expr + foo _)
)

(type + Thing
  (implements Proto)
  (fn-do + foo _
    (print "yes")
  )
)

(fn-do + foo _
  (param p Proto)
  (print (call (. p bar)))
)
```

```output
[Interface __source__:Proto]
yes
42
```

```brix
(main
  (let thing (call Thing))
  (call (. thing foo))
  (call (. thing bar))
  (blank)
  (let proto (as thing Proto))
  (call (. proto foo))
  (call (. proto bar))
)

(type + Proto (is Interface)
  (fn-do + foo _
    (print (str "proto foo " this))
  )
  (fn-do + bar _
    (print (str "proto bar " this))
  )
)

(type + Thing
  (implements Proto)
  (fn-do + bar _
    (print (str "thing bar " this))
  )
)
```

```output
proto foo [Interface __source__:Proto]
thing bar [Object __source__:Thing]
proto foo [Interface __source__:Proto]
thing bar [Object __source__:Thing]
```

```brix
(main
  (let thing (call (of Thing Float Int)))
  (call foo thing)
  (call bar thing)
)

(type + (of Foo T))

(type + (of Bar T) (is Interface))

(type + (of Thing T1 T2) (is (of Foo T2))
  (implements (of Bar T1))
  (prop + a T1 (= 10))
  (prop + b T2 (= 10))
)

(fn-do + (of foo T) _
  (param p (of Foo T))
  (print (call T))
)

(fn-do + (of bar T) _
  (param p (of Bar T))
  (print (call T))
)
```

```output
0
0
```

```brix
(main
  (let genny (call gen))
  (call foo genny)
)

(fn-do + gen _
  (yield 1)
  (yield 2)
)

(fn-do + (of foo T) _
  (param g (stream T))
  (print (call T))
)
```

```output
0
```

```brix
(main
  (let thing (call Thing))
  (call foo thing)
)

(type + Proto (is Interface)
  (fn-do + bar _
    (print this)
  )
)

(type + Thing
  (implements Proto)
  (fn-do + bar _
    (print this)
  )
)

(fn-do + foo _
  (param proto Proto)
  (call (. proto bar))
)
```

```output
[Object __source__:Thing]
```

```brix
(main
  (let thing (call (of Thing Int)))
  (let stream (as thing (stream Int)))
  (call foo thing)
)

(type + (of Thing T)
  (fn-to-do + (of Iterable T)
    (yield 1)
    (yield 2)
  )
)

(fn-do + (of foo T) _
  (param stream (stream T))
  (for (in n stream)
    (print n)
  )
)
```

```output
1
2
```

## Different types implementing interfaces

```brix
(main
  (let thing (call Thing))
  (let proto (as thing Proto))
  (print (type-of proto))
  (call (. proto foo))
  (call (. proto foo) 1)
  (call (. proto foo) 1.5)
)

(type + Proto (is Interface)
  (fn-expr + foo _)
  (fn-expr + foo _
    (param n _)
  )
)

(type + Thing
  (implements Proto)
  (fn-do + foo _
    (print this)
  )
  (fn-do + foo _
    (param n _)
    (print n)
  )
)
```

```output
[TypeInfo Proto]
[Object __source__:Thing]
1
1.5
```

```brix
(main
  (let thing (call Thing 42))
  (let proto (as thing Proto))
  (print (type-of proto))
  (call (. proto foo))
  (call (. proto foo) 1)
  (call (. proto foo) 1.5)
)

(type + Proto (is Interface)
  (fn-expr + foo _)
  (fn-expr + foo _
    (param n _)
  )
)

(type + Thing (is I32)
  (implements Proto)
  (fn-do + foo _
    (print this)
  )
  (fn-do + foo _
    (param n _)
    (print n)
  )
)
```

```output
[TypeInfo Proto]
42
1
1.5
```

```brix
(main
  (print (call get-proto (call gen)))
)

(fn-do + gen _
  (yield 1)
)

(fn-expr + get-proto (stream Int)
  (param thing _)
  thing
)
```

```output
[Interface core/stream:Iterable'core/number:I64]
```

```brix
(main
  (let thing (call Thing 3.5))
  (let proto (as thing Proto))
  (print (type-of proto))
  (call (. proto foo))
  (call (. proto foo) 1)
  (call (. proto foo) 1.5)
)

(type + Proto (is Interface)
  (fn-expr + foo _)
  (fn-expr + foo _
    (param n _)
  )
)

(type + Thing (is F64)
  (implements Proto)
  (fn-do + foo _
    (print this)
  )
  (fn-do + foo _
    (param n _)
    (print n)
  )
)
```

```output
[TypeInfo Proto]
3.5
1
1.5
```

```brix
(main
  (let fn (=> (a) (* a 3)))
  (blank)
  (let thing (as fn Thing))
  (let proto (as thing Proto))
  (print (type-of proto))
  (call (. proto foo))
  (call (. proto foo) 1)
  (call (. proto foo) 1.5)
)

(type + Proto (is Interface)
  (fn-expr + foo _)
  (fn-expr + foo _
    (param n _)
  )
)

(type + ThingFn (is (fn-type I32 (param a Int))))

(type + Thing (is ThingFn)
  (implements Proto)
  (fn-do + foo _
    (print this)
    (print (call this 3))
  )
  (fn-do + foo _
    (param n _)
    (print n)
  )
)
```

```output
[TypeInfo Proto]
[Function Thing]
9
1
1.5
```

```brix
(main
  (let thing (call Thing 1 2))
  (let proto (as thing Proto))
  (print (type-of proto))
  (call (. proto foo))
  (call (. proto foo) 1)
  (call (. proto foo) 1.5)
)

(type + Proto (is Interface)
  (fn-expr + foo _)
  (fn-expr + foo _
    (param n _)
  )
)

(type + Thing (is (tuple (: a Int) (: b Float)))
  (implements Proto)
  (fn-do + foo _
    (print this)
  )
  (fn-do + foo _
    (param n _)
    (print n)
  )
)
```

```output
[TypeInfo Proto]
(1, 2)
1
1.5
```

## Interface inheritance

```brix
(main
  (let p1 (as (call Thing) Proto1))
  (call (. p1 foo))
  (blank)
  (let p2 (as (call Thing) Proto2))
  (call (. p2 foo))
)

(type + Proto1 (is Interface)
  (fn-expr + foo _)
)

(type + Proto2 (is Interface)
  (implements Proto1)
)

(type + Thing
  (implements Proto2)
  (fn-do + foo _
    (print 42)
  )
)
```

```output
42
42
```

### Cast base interface to sub interface, call through sub and then through base

```brix
(main
  (let foobar (as (call Thing) Foobar))
  (call (. foobar foo))
  (call (. foobar bar))
  (blank)
  (let bar (as foobar Bar))
  (call (. bar bar))
  (blank)
  (let foo (as foobar Foo))
  (call (. foo foo))
)

(type + Foo (is Interface)
  (fn-expr + foo _)
)

(type + Bar (is Interface)
  (fn-expr + bar _)
)

(type + Foobar (is Interface)
  (implements Foo)
  (implements Bar)
)

(type + Thing
  (implements Foobar)
  (fn-do + foo _
    (print "foo")
  )
  (fn-do + bar _
    (print "bar")
  )
)
```

```output
foo
bar
bar
foo
```

### Cast base interface to sub interface, call through base and then through sub

```brix
(main
  (let foobar (as (call Thing) Foobar))
  (let bar (as foobar Bar))
  (call (. bar bar))
  (blank)
  (let foo (as foobar Foo))
  (call (. foo foo))
  (blank)
  (call (. foobar foo))
  (call (. foobar bar))
)

(type + Foo (is Interface)
  (fn-expr + foo _)
)

(type + Bar (is Interface)
  (fn-expr + bar _)
)

(type + Foobar (is Interface)
  (implements Foo)
  (implements Bar)
)

(type + Thing
  (implements Foobar)
  (fn-do + foo _
    (print "foo")
  )
  (fn-do + bar _
    (print "bar")
  )
)
```

```output
bar
foo
foo
bar
```

### Cast to sub interface, call through sub

```brix
(main
  (let thing (call Thing))
  (let bar (as thing Bar))
  (call (. bar bar))
  (blank)
  (let foo (as thing Foo))
  (call (. foo foo))
)

(type + Foo (is Interface)
  (fn-expr + foo _)
)

(type + Bar (is Interface)
  (fn-expr + bar _)
)

(type + Foobar (is Interface)
  (implements Foo)
  (implements Bar)
)

(type + Thing
  (implements Foobar)
  (fn-do + foo _
    (print "foo")
  )
  (fn-do + bar _
    (print "bar")
  )
)
```

```output
bar
foo
```

```brix
(main
  (let thing (call Thing))
  (blank)
  (let foobar (as thing Foobar))
  (call (. foobar a))
  (blank)
  (let a (as thing A))
  (call (. a a))
  (blank)
  (let foo (as foobar Foo))
  (call (. foo a))
  (blank)
  (= a (as foobar A))
  (call (. a a))
  (blank)
  (= a (as foo A))
  (call (. a a))
)

(type + A (is Interface)
  (fn-expr + a _)
)

(type + Foo (is Interface)
  (implements A)
  (fn-expr + foo _)
)

(type + Bar (is Interface)
  (fn-expr + bar _)
)

(type + Foobar (is Interface)
  (implements Foo)
  (implements Bar)
)

(type + Thing
  (implements Foobar)
  (fn-do + a _
    (print "a")
  )
  (fn-do + foo _
    (print "foo")
  )
  (fn-do + bar _
    (print "bar")
  )
)
```

```output
a
a
a
a
a
```

### Illegal to cast interface to another interface that inherits from it

```brix crash
(main
  (let p1 (as (call Thing) Proto1))
  (print (as p1 Proto2))
)

(type + Proto1 (is Interface)
  (fn-expr + foo _)
)

(type + Proto2 (is Interface)
  (implements Proto1)
)

(type + Thing
  (implements Proto2)
  (fn-do + foo _
    (print 42)
  )
)
```

```output
Exception: Proto1 cannot be cast to Proto2; there is no conversion between them
<no file:4>
      (print (as p1 Proto2))
             ^^^^^^^^^^^^^^
```

## Class inheritance with interfaces

```brix
(main
  (let bar (call Bar))
  (let foo (as bar Foo))
  (let p (as foo Proto))
  (call (. p foo))
)

(type + Proto (is Interface)
  (fn-expr + foo _)
)

(type + Foo
  (implements Proto)
  (fn-do + foo _
    (print "Foo")
  )
)

(type + Bar (is Foo)
  (fn-do + foo _
    (print "Bar")
  )
)
```

```output
Bar
```

```brix
(main
  (let direct Base (call Middle))
  (let inherited Base (call Leaf))
  (let plain Base (call Base))
  (call print (str "direct marker " (is direct Marker) " named " (is direct Named)))
  (call print (str "inherited marker " (is inherited Marker) " named " (is inherited Named)))
  (call print (str "plain marker " (is plain Marker) " named " (is plain Named)))
)

(type + Marker (is Interface))

(type + Named (is Interface)
  (fn-expr + label String :get)
)

(type + Base
  (prop + size _ (= 1))
)

(type + Middle (is Base)
  (implements Marker)
  (implements Named)
  (fn-expr + label String :get
    "middle"
  )
)

(type + Leaf (is Middle))
```

```output
direct marker true named true
inherited marker true named true
plain marker false named false
```

```brix
(main
  (let inherited Base (call Leaf))
  (let named (as inherited Named))
  (call print (call (. named describe)))
)

(type + Named (is Interface)
  (fn-expr + describe String)
)

(type + Base
  (prop + size _ (= 1))
)

(type + Middle (is Base)
  (implements Named)
  (fn-expr + describe String
    "middle"
  )
)

(type + Leaf (is Middle)
  (fn-expr + describe String
    "leaf"
  )
)
```

```output
leaf
```

## Converters

```brix
(main
  (let thing (call Thing))
  (let proto (as thing Proto))
  (print (as proto Float))
  (print (as thing Float))
)

(type + Proto (is Interface)
  (fn-to-expr + Float
    10.5
  )
)

(type + Thing
  (implements Proto)
)
```

```output
10.5
10.5
```

```brix
(main
  (let thing (as (call Thing) Proto))
  (print thing)
)

(type + Proto (is Interface)
  (fn-expr + foo I32)
  (fn-to-expr + String
    (as (call foo) String)
  )
)

(type + Thing
  (implements Proto)
  (fn-expr + foo _
    42
  )
)
```

```output
42
```

```brix
(main
  (let thing (as (call Thing) Proto))
  (print (call (. " " join) thing))
)

(type + Proto (is Interface)
  (fn-expr + foo String)
  (fn-to-do + (of Iterable String)
    (yield "hey")
    (yield (call foo))
  )
)

(type + Thing
  (implements Proto)
  (fn-expr + foo _
    "you"
  )
)
```

```output
hey you
```

### Make sure base class interfaces are checked when trying to convert to iterator

```brix
(main
  (let thing (call Foo))
  (for (in i thing)
    (print i)
  )
)

(type + Bar
  (implements (of Iterable I32))
  (prop + i I32 (= 0))
  (fn-do + (prefix <- this) _
    (if (< i 2)
      (+= i 1)
      (return i)
    )
  )
)

(type + Foo (is Bar))
```

```output
1
2
```

### Make sure base class converters are checked when trying to convert to iterator

```brix
(main
  (let thing (call Foo))
  (for (in i thing)
    (print i)
  )
)

(type + Bar
  (fn-to-do + (of Iterable I32)
    (yield 42)
  )
)

(type + Foo (is Bar))
```

```output
42
```

```brix
(main
  (let p (as (call Thing) Proto))
  (call (. p act))
)

(type + Foo
  (fn-do + foo _
    (print "Foo")
  )
)

(type + Bar (is Foo)
  (fn-do + foo _
    (print "Bar")
  )
)

(type + Proto (is Interface)
  (fn-expr + act _)
)

(type + Thing
  (implements Proto)
  (fn-do + act _
    (let bar (call Bar))
    (call (. bar foo))
  )
)
```

```output
Bar
```

## Delegates

### Delegate as eager property

```brix
(main
  (let thing (call Thing))
  (call (. thing test))
  (blank)
  (let proto (as thing Proto))
  (call (. proto act))
)

(type + Proto (is Interface)
  (fn-expr + act _)
)

(type + Foo
  (fn-do + act _
    (print "called act")
  )
)

(type + Thing
  (delegate Proto)
  (prop + foo Foo)
  (fn-do + test _
    (call (. foo act))
  )
)
```

```output
called act
called act
```

### Call delegate without object

```brix
(main
  (let thing (call Thing))
  (call (. thing test))
)

(type + Proto (is Interface)
  (fn-expr + act _)
)

(type + Foo
  (fn-do + act _
    (print "called act")
  )
)

(type + Thing
  (delegate Proto)
  (prop + foo Foo)
  (fn-do + test _
    (call act)
  )
)
```

```output
called act
```

### Delegate as lazy property

```brix
(main
  (let thing (call Thing))
  (call (. thing act))
  (blank)
  (let proto (as thing Proto))
  (call (. proto act))
)

(type + Proto (is Interface)
  (fn-expr + act _)
)

(type + Foo
  (fn-do + act _
    (print "called act")
  )
)

(type + Thing
  (delegate Proto)
  (prop + foo _ (:= (call Foo)))
)
```

```output
called act
called act
```

### Delegate as getter

```brix
(main
  (let thing (call Thing))
  (call (. thing act))
  (blank)
  (let proto (as thing Proto))
  (call (. proto act))
)

(type + Proto (is Interface)
  (fn-expr + act _)
)

(type + Foo
  (fn-do + act _
    (print "called act")
  )
)

(type + Thing
  (delegate Proto)
  (fn-expr + foo _ :get
    (call Foo)
  )
)
```

```output
called act
called act
```

### Delegate of delegate

```brix
(main
  (let thing (call Thing))
  (call (. thing act))
  (blank)
  (let proto (as thing Proto))
  (call (. proto act))
)

(type + Proto (is Interface)
  (fn-expr + act _)
)

(type + Foo
  (implements Proto)
  (fn-do + act _
    (print "called act")
  )
)

(type + Bar
  (delegate Proto)
  (prop + foo _ (= (call Foo)))
)

(type + Thing
  (delegate Proto)
  (prop + bar _ (= (call Bar)))
)
```

```output
called act
called act
```

### Delegate of delegate in base class

```brix
(main
  (let thing (call Thing))
  (call (. thing act))
  (blank)
  (let proto (as thing Proto))
  (call (. proto act))
)

(type + Proto (is Interface)
  (fn-expr + act _)
)

(type + Foo
  (implements Proto)
  (fn-do + act _
    (print "called act")
  )
)

(type + Bar
  (delegate Proto)
  (prop + foo _ (= (call Foo)))
)

(type + Base
  (delegate Proto)
  (prop + bar _ (= (call Bar)))
)

(type + Thing (is Base))
```

```output
called act
called act
```

### Delegate in tuple

```brix
(main
  (let thing (call Thing))
  (call (. thing act))
  (blank)
  (let proto (as thing Proto))
  (call (. proto act))
)

(type + Proto (is Interface)
  (fn-expr + act _)
)

(type + Foo
  (prop + a _ (= 0))
  (fn-do + This _
    (param a _)
  )
  (fn-do + act _
    (print (str "called act " a))
  )
)

(type + Thing (is Tuple)
  (delegate Proto)
  (prop + foo _ (= (call Foo 2)))
)
```

```output
called act 2
called act 2
```

### Delegate in binding

```brix
(main
  (let thing (call Thing))
  (let binding (call (. thing act) ...))
  (call binding 42)
  (blank)
  (let proto (as thing Proto))
  (let binding2 (call (. proto act) ...))
  (call binding2 24)
)

(type + Proto (is Interface)
  (fn-expr + act _
    (param x Int)
  )
)

(type + Foo
  (fn-do + act _
    (param x Int)
    (print (str "called act " x))
  )
)

(type + Thing (is Tuple)
  (delegate Proto)
  (prop + foo Foo)
)
```

```output
called act 42
called act 24
```

```brix crash
(main
  (let p Proto)
)

(type + Proto (is Interface))
```

```output
Exception: "p" is declared Proto with no value, but Proto is an interface, so there is nothing to construct; give it a value, as in "(let p Proto value)", or declare it "(? Proto)" to start as null
<no file:3>
      (let p Proto)
             ^^^^^
```

```brix
(main
  (let sink (call Sink))
  (<- sink "hello")
  (let writable (as sink (of Writable String)))
  (<- writable "world")
)

(type + (of Writable Message) (is Interface)
  (fn-expr + (infix this <- (param item Message)) _)
)

(type + Wrapped (is (tuple (: text String)))
  (fn-from-expr + (: other String)
    (call Wrapped other)
  )
  (fn-to-expr + String
    text
  )
)

(type + (of Channel Message)
  (implements (of Writable Message))
  (fn-do + (infix this <- (param item Message)) _
    (print item)
  )
)

(type + Sink
  (delegate (of Writable String))
  (prop _ channel _ (= (call (of Channel Wrapped))))
)
```

```output
hello
world
```

```brix
(main
  (let thing (call Thing))
  (call (. thing act))
  (blank)
  (let proto (as thing Proto))
  (call (. proto act))
)

(type + Proto (is Interface)
  (fn-expr + ping _)
  (fn-do + act _
    (call ping)
  )
)

(type + Foo
  (implements Proto)
  (fn-do + ping _
    (print "foo ping")
  )
)

(type + Thing
  (delegate Proto)
  (prop _ foo _ (= (call Foo)))
)
```

```output
foo ping
foo ping
```

```brix
(main
  (let thing (call Thing))
  (blank)
  (let child (as thing Child))
  (call (. child base))
  (call (. child child))
  (blank)
  (let base (as thing Base))
  (call (. base base))
)

(type + Base (is Interface)
  (fn-expr + base _)
)

(type + Child (is Interface)
  (implements Base)
  (fn-expr + child _)
)

(type + Foo
  (implements Child)
  (fn-do + base _
    (print "base")
  )
  (fn-do + child _
    (print "child")
  )
)

(type + Thing
  (delegate Child)
  (prop _ foo _ (= (call Foo)))
)
```

```output
base
child
base
```

```brix crash
(main
  (print (call Proto))
)

(type + Proto (is Interface)
  (fn-do + This _
    (print this)
    42
  )
)
```

```output
Exception: Required self not defined
<no file:8>
        (print this)
               ^^^^
```

```brix crash
(main
  (print (call Proto))
)

(type + Proto (is Interface)
  (fn-do + This _)
)
```

```output
Exception: Type can not be constructed
<no file:7>
      (fn-do + This _)
      ^^^^^^^^^^^^^^^^
```

```brix
(main
  (let thing (call Thing))
  (let thing1 (as thing Proto))
  (let thing1b (as thing Proto))
  (let thing2 (as (call Thing) Proto))
  (let ding1 (as (call Ding) Proto))
  (print (== thing1 thing1))
  (print (== thing1 thing1b))
  (print (== thing1 thing2))
  (print (!= thing1 thing1b))
  (print (!= thing1 thing2))
)

(type + Proto (is Interface))

(type + Thing
  (implements Proto)
)

(type + Ding
  (implements Proto)
)
```

```output
true
true
false
false
true
```

```brix
(main
  (let thing (call Thing "bob" "roberts"))
  (let thing1 (as thing Proto))
  (let thing1b (as thing Proto))
  (let thing2 (as (call Thing "joe" "blow") Proto))
  (let thing3 (as (call Thing "bob" "roberts") Proto))
  (print (== thing1 thing1))
  (print (== thing1 thing1b))
  (print (== thing1 thing2))
  (print (== thing1 thing3))
)

(type + Proto (is Interface))

(type + Thing (is (tuple (: first String) (: last String)))
  (implements Proto)
)
```

```output
true
true
false
true
```

```brix
(main
  (let thing (call Thing))
  (call (. thing action))
)

(type + Thing
  (implements Proto)
)

(type + Proto (is Interface)
  (fn-do + action _
    (print (str "hello " this))
  )
)
```

```output
hello [Interface __source__:Proto]
```

### Delegate through stream shorthand resolves to the property interface

```brix
(main
  (let thing (call Thing))
  (let stream (as thing (stream String)))
  (print (type-of stream))
)

(type + Producer
  (implements (of Iterable String))
  (fn-expr + (prefix <- this) (? String)
    null
  )
)

(type + Thing
  (delegate (stream String))
  (prop _ producer _ (= (call Producer)))
)
```

```output
[TypeInfo Iterable'String]
```

### Self-referential generic interface conformance (Node as TreeHost'Node) compiles without recursing infinitely during type creation, and constrained generic entry points dispatch the host's concretely typed hook statically.

```brix
(main
  (let parent (call Node))
  (let child (call Node))
  (call tree-append parent child)
  (print (. parent count))
)

(type + (of TreeHost Child) (is Interface)
  (fn-expr + attach-child _
    (param child Child)
  )
)

(type + Node
  (implements (of TreeHost Node))
  (prop + count _ (= 0))
  (fn-do + attach-child _
    (param child Node)
    (+= count 1)
  )
)

(fn-do + (of tree-append Host Child) _
  (param parent Host)
  (param child Child)
  (constraint is Host (of TreeHost Child))
  (call (. parent attach-child) child)
)
```

```output
1
```

### A generic method body can pass an outer type parameter through two interface values without trying to infer that already-identical parameter in an empty call-match scope.

```brix
(main
  (let region (call (of GenericRegion I64) (call NumberDomain 42) (call NumberLayout)))
  (call (. region refresh))
)

(type + (of GenericDomain T) (is Interface)
  (fn-expr + value T :get)
)

(type + (of GenericLayout T) (is Interface)
  (fn-do + fill _
    (param domain (of GenericDomain T))
  )
)

(type + NumberDomain
  (implements (of GenericDomain I64))
  (prop + value I64)
  (fn-do + This _
    (param value _)
  )
)

(type + NumberLayout
  (implements (of GenericLayout I64))
  (fn-do + fill _
    (param domain (of GenericDomain I64))
    (print (. domain value))
  )
)

(type + (of GenericRegion T)
  (prop + domain (of GenericDomain T))
  (prop + layout (of GenericLayout T))
  (fn-do + This _
    (param domain _)
    (param layout _)
  )
  (fn-do + refresh _
    (call (. layout fill) domain)
  )
)
```

```output
42
```

```brix
(import /core/atom-pool)
(blank)

(main
  (let atoms (call AtomPool))
  (finally
    (call (. atoms release))
  )
  (blank)
  (call (of check View) (as 42 Integer) (as 42 Integer))
  (call (of check View) (as 42 Integer) (as 43 Integer))
  (call (of check (of Indexable Char)) "hello" (++ "he" "llo"))
  (call (of check (of Indexable Char)) "hello" "world")
  (call (of check (of Indexable I32)) (list 1 2 3) (list 1 2 3))
  (call (of check (of Indexable I32)) (list 1 2 3) (list 1 2 4))
  (call (of check View) (call AtomPair (call (. atoms store) "one") 0) (call AtomPair (call (. atoms store) "one") 0))
  (call (of check View) (call AtomPair (call (. atoms store) "one") 0) (call AtomPair (call (. atoms store) "two") 0))
  (call (of check View) (as 0.0 Real) (as -0.0 Real))
  (let nan (/ 0.0f64 0.0f64))
  (call (of check View) (as nan Real) (as nan Real))
  (let boxed-nan (as (as nan Real) View))
  (print (== boxed-nan boxed-nan))
  (print (!= boxed-nan boxed-nan))
)

(type + View (is Interface))

(type + Integer (is I32)
  (implements View)
)

(type + Real (is F64)
  (implements View)
)

(type + AtomPair (is (tuple (: value Atom) (: index I32)))
  (implements View)
)

(fn-do + (of check Face T) _
  (param left T)
  (param right T)
  (let boxed-left (as left Face))
  (let boxed-right (as right Face))
  (print (== left right))
  (print (== boxed-left boxed-right))
  (print (!= boxed-left boxed-right))
)
```

```output
true
true
false
false
false
true
true
true
false
false
false
true
true
true
false
false
false
true
true
true
false
false
false
true
true
true
false
false
false
true
false
true
```

```brix
(main
  (let pair (call (of Packet I32) 7 9))
  (let same (call (of Packet I32) 7 9))
  (let different (call (of Packet I32) 7 10))
  (let child (as pair Child))
  (let parent (as same View))
  (print (== child parent))
  (print (== parent child))
  (print (!= child parent))
  (print (== child (as different View)))
  (let adapted (as child View))
  (print (. (type-of adapted) concrete-name))
  (print (== adapted parent))
  (let nested (as (call Nested child 1) View))
  (let nested-same (as (call Nested parent 1) View))
  (let nested-different (as (call Nested (as different View) 1) View))
  (print (== nested nested-same))
  (print (!= nested nested-same))
  (print (== nested nested-different))
  (call unsupported (as pair View) (as (call (of Packet I64) 7i64 9i64) View) false)
  (call unsupported (as pair View) (as (call (of Packet I64) 7i64 9i64) View) true)
)

(type + View (is Interface))

(type + Child (is Interface)
  (implements View)
)

(type + (of Packet T) (is (tuple (: first T) (: second T)))
  (implements Child)
)

(type + Nested (is (tuple (: inner View) (: marker I32)))
  (implements View)
)

(fn-do + unsupported _
  (param left View)
  (param right View)
  (param unequal Bool)
  (let attempt
    (do-new
      (if unequal
        (print (!= left right))
        (else
          (print (== left right))
        )
      )
      (on-failure failure
        (let failure (? Panic) failure)
        (if failure
          (print (str failure))
        )
      )
    )
  )
  (call (. attempt wait))
)
```

```output
true
true
false
false
Packet'I32
true
true
false
false
Unsupported interface equality: concrete types differ
Unsupported interface equality: concrete types differ
```

```brix
(main
  (let state (call Counts))
  (let value (call Measured state 7))
  (let boxed (as value View))
  (print (== value value))
  (print (== boxed boxed))
  (print (!= boxed boxed))
  (print (. state comparisons))
  (= (. state comparisons) 0)
  (print (== (call make-value state) (call make-value state)))
  (print (. state operands))
  (print (. state comparisons))
  (call panic-equality boxed state false)
  (call panic-equality boxed state true)
  (print (. state comparisons))
)

(type + View (is Interface))

(type + Counts
  (prop + operands _ (= 0))
  (prop + comparisons _ (= 0))
  (prop + should-panic _ (= false))
)

(type + Measured (is (tuple (: state Counts) (: value I32)))
  (implements View)
  (fn-do + (infix this == (param other Measured)) _
    (+= (. state comparisons) 1)
    (let allocated (str value ":" (. other value)))
    (if (. state should-panic)
      (throw (call Panic (str "comparison panic " allocated)))
    )
    (return (!= value (. other value)))
  )
)

(fn-do + make-value View
  (param state Counts)
  (+= (. state operands) 1)
  (return (call Measured state 7))
)

(fn-do + panic-equality _
  (param value View)
  (param state Counts)
  (param unequal Bool)
  (let attempt
    (do-new
      (= (. state should-panic) true)
      (if unequal
        (print (!= value value))
        (else
          (print (== value value))
        )
      )
      (on-failure failure
        (let failure (? Panic) failure)
        (if failure
          (print (str failure))
        )
      )
    )
  )
  (call (. attempt wait))
)
```

```output
false
false
true
3
false
2
1
comparison panic 7:7
comparison panic 7:7
3
```

```brix
(main
  (let pair (as (call Pair "first" "second") View))
  (let nested (as (call Nested pair 1) View))
  (print (call equal nested nested))
)

(type + View (is Interface))

(type + Pair (is (tuple (: first String) (: second String)))
  (implements View)
)

(type + Nested (is (tuple (: inner View) (: marker I32)))
  (implements View)
)

(fn-do + equal _
  (param left View)
  (param right View)
  (return (== left right))
)
```

```output
true
```

### A public field satisfies an interface property that requires a setter.

```brix
(main
  (let nodes (list Node))
  (+= nodes (call Leaf))
  (+= nodes (call Branch))
  (for (in node nodes)
    (= (. node id) (+ (. node id) 10))
    (print (. node id))
  )
)

(type + Node (is Interface)
  (fn-expr + id Size :get)
  (fn-do + id _ :set (param value Size))
)

(type + Leaf
  (implements Node)
  (prop + id Size (= 1))
)

(type + Branch
  (implements Node)
  (prop _ stored Size (= 2))
  (fn-expr + id _ :get
    stored
  )
  (fn-do + id _ :set (param value Size)
    (= stored value)
  )
)
```

```output
11
12
```

### Interface values test, match, and cast against implementing classes and their subclasses.

```brix
(main
  (let nodes (list Node))
  (+= nodes (call Leaf))
  (+= nodes (call Twig))
  (+= nodes (call Branch 3))
  (+= nodes (call Stone))
  (for (in node nodes)
    (print (call describe node))
  )
  (print (. (as ([] nodes 2) Branch) kids))
  (print (is ([] nodes 1) Leaf))
  (print (is ([] nodes 3) Leaf))
)

(fn-expr + describe String
  (param node Node)
  (match node
    (case (: leaf Leaf)
      (str "leaf " (. leaf name))
    )
    (case (: branch Branch)
      (str "branch " (. branch kids))
    )
    (default
      "other"
    )
  )
)

(type + Node (is Interface)
  (fn-expr + weight Int :get)
)

(type + Leaf
  (implements Node)
  (prop + weight _ (= 1))
  (fn-expr + name _ :get
    "leaf"
  )
)

(type + Twig (is Leaf)
  (fn-expr + name _ :get
    "twig"
  )
)

(type + Branch
  (implements Node)
  (prop + kids Int)
  (prop + weight _ (= 2))
  (fn-do + This _
    (param kids _)
  )
)

(type + Stone (is Tuple)
  (implements Node)
  (prop + weight _ (= 3))
)
```

```output
leaf leaf
leaf twig
branch 3
other
3
true
false
```

### A covariant getter satisfies an interface getter that returns a wider interface.

```brix
(main
  (let child (call Child))
  (let holder Holder child)
  (print (== (. holder owner) null))
  (= (. child link) (call Parent))
  (print (. (!! (. holder owner)) tag))
)

(type + Owner (is Interface)
  (fn-expr + tag String :get)
)

(type + Holder (is Interface)
  (fn-expr + owner (? Owner) :get)
)

(type + Parent
  (implements Owner)
  (fn-expr + tag _ :get
    "parent"
  )
)

(type + Child
  (implements Holder)
  (prop + link (? Parent))
  (prop + owner _ (=> link))
)
```

```output
true
parent
```

### Argument dispatch does not implicitly narrow an interface value to an implementing class.

```brix crash
(main
  (let named Named (call Leaf))
  (print (call describe (as named Leaf)))
  (print (call describe named))
)

(type + Named (is Interface)
  (fn-expr + label String :get)
)

(type + Leaf
  (implements Named)
  (prop + label _ (= "leaf"))
)

(fn-expr + describe _
  (param leaf Leaf)
  (. leaf label)
)
```

```output
Exception: No function "describe" accepts (Named); it is declared as "describe(leaf Leaf)"
<no file:5>
      (print (call describe named))
             ^^^^^^^^^^^^^^^^^^^^^
```

### Joining an interface with an implementing class keeps the interface type.

```brix
(main
  (for (in i (..< 0 4))
    (print (?? (?. (call pick i) label) "none"))
  )
  (let named Named (call Leaf))
  (let missing (? Leaf) null)
  (print (. (?? missing named) label))
)

(type + Named (is Interface)
  (fn-expr + label String :get)
)

(type + Leaf
  (implements Named)
  (prop + label _ (= "leaf"))
)

(type + Twig
  (implements Named)
  (prop + label _ (= "twig"))
)

(type + Stem
  (implements Named)
  (prop + label _ (= "stem"))
)

(fn-expr + pick (? Named)
  (param index Int)
  (case? (== index 0)
    (call Leaf)
  )
  (case? (== index 1)
    (call Twig)
  )
  (case? (== index 2)
    (call Stem)
  )
  (default
    null
  )
)
```

```output
leaf
twig
stem
none
leaf
```

### An inherited public field satisfies an interface setter on a subclass.

```brix
(main
  (let nodes (list Node))
  (+= nodes (call Leaf))
  (+= nodes (call Twig))
  (for (in node nodes)
    (= (. node id) (+ (. node id) 10))
    (print (. node id))
  )
)

(type + Node (is Interface)
  (fn-expr + id Size :get)
  (fn-do + id _ :set (param value Size))
)

(type + Leaf
  (implements Node)
  (prop + id Size (= 1))
)

(type + Twig (is Leaf))
```

```output
11
11
```

### A recursive list getter yields class values as interface values.

```brix
(main
  (let inner (call Group "inner"))
  (call (. inner add-leaf) (call Leaf "b"))
  (let root (call Group "root"))
  (call (. root add-leaf) (call Leaf "a"))
  (call (. root add-group) inner)
  (for (in node (. root descendants))
    (print (. node label))
  )
  (for (in node (. root nodes))
    (print (. node label))
  )
)

(type + Named (is Interface)
  (fn-expr + label String :get)
)

(type + Leaf
  (implements Named)
  (prop + label String)
  (fn-do + This _
    (param label _)
  )
)

(type + Item
  (prop + leaf (? Leaf))
  (prop + group (? Group))
)

(type + Group
  (implements Named)
  (prop + label String)
  (prop _ items (list Item) (= (call (of List Item))))
  (fn-do + This _
    (param label _)
  )
  (fn-do + add-leaf _
    (param leaf Leaf)
    (let item (call Item))
    (= (. item leaf) leaf)
    (+= items item)
  )
  (fn-do + add-group _
    (param group Group)
    (let item (call Item))
    (= (. item group) group)
    (+= items item)
  )
  (fn-do + nodes (list Named) :get
    (for (in item items)
      (if (. item leaf)
        (yield (. item leaf))
        (else-if (. item group)
          (yield (. item group))
        )
      )
    )
  )
  (fn-do + descendants (list Named) :get
    (for (in item items)
      (if (. item leaf)
        (yield (. item leaf))
        (else-if (. item group)
          (let group (. item group))
          (yield group)
          (for (in node (. group descendants))
            (yield node)
          )
        )
      )
    )
  )
)
```

```output
a
inner
b
a
inner
```

### Feed pipelines over interface-typed lists map, filter, test, and pick items.

```brix
(main
  (let nodes (list Named) (call (of List Named)))
  (+= nodes (call Leaf "a"))
  (+= nodes (call Twig "b"))
  (+= nodes (call Leaf "c"))
  (let found
    (block
      (spread (list "a" "x" "c"))
      (pipe ?*> (call find nodes each))
      (pipe |> List)
    )
  )
  (print (. found length))
  (let labels
    (block
      (spread nodes)
      (pipe *> (. each label))
      (pipe |> List)
    )
  )
  (print ([] labels 2))
  (let twigs
    (block
      (spread nodes)
      (pipe ?> (is each Twig))
      (pipe *> (as each Twig))
      (pipe |> List)
    )
  )
  (print (. ([] twigs 0) label))
  (let matches
    (block
      (spread nodes)
      (pipe ?> (== (. each label) "c"))
      (pipe |> List)
    )
  )
  (print (. ([] matches 0) label))
  (let twig-list
    (block
      (spread nodes)
      (pipe ?> (is each Twig))
      (pipe |> List)
    )
  )
  (print (. twig-list length))
)

(fn-do + find (? Named)
  (param nodes (list Named))
  (param label String)
  (for (in node nodes)
    (return-if node (if (== (. node label) label)))
  )
  (return null)
)

(type + Named (is Interface)
  (fn-expr + label String :get)
)

(type + Leaf
  (implements Named)
  (prop + label String)
  (fn-do + This _
    (param label _)
  )
)

(type + Twig (is Leaf)
  (fn-do + This _
    (param label _)
  )
)
```

```output
2
c
b
c
1
```

### A single map stage over an interface-typed list compiles.

```brix
(main
  (let nodes (list Named) (call (of List Named)))
  (+= nodes (call Leaf "a"))
  (+= nodes (call Leaf "c"))
  (let kept
    (block
      (spread nodes)
      (pipe *> (. each label))
      (pipe |> List)
    )
  )
  (print ([] kept 1))
)

(type + Named (is Interface)
  (fn-expr + label String :get)
)

(type + Leaf
  (implements Named)
  (prop + label String)
  (fn-do + This _
    (param label _)
  )
)
```

```output
c
```

### A panic in a method called through a value of an interface: the trace has the method, then the frame that takes the object out of the value, placed where the method is declared.

```brix crash
(main
  (let thing (call Thing))
  (call foo thing)
)

(type + Proto (is Interface)
  (fn-expr + bar Int
    (param n Int)
  )
)

(type + Thing
  (implements Proto)
  (fn-do + bar _
    (param n Int)
    (throw (call Panic (str "bar " n)) (if (> n 3)))
    (return n)
  )
)

(fn-do + foo _
  (param p Proto)
  (print (call (. p bar) 1))
  (print (call (. p bar) 5))
)
```

```stderr
Unhandled panic: bar 5
<no file:17>

Task 1

Stack trace:
  0  __source__:Thing:bar no file:17:5
  1  __source__:bar(wrapped) no file:15:3
  2  __source__:foo no file:25:10
  3  __source__:main no file:4:3
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### A panic in a function an interface declares with a body, called through a value: no frame stands between it and its caller.

```brix crash
(main
  (let thing (call Thing))
  (call foo thing)
)

(type + Proto (is Interface)
  (fn-do + bar Int
    (param n Int)
    (throw (call Panic (str "default bar " n)) (if (> n 3)))
    (return n)
  )
)

(type + Thing
  (implements Proto)
)

(fn-do + foo _
  (param p Proto)
  (print (call (. p bar) 2))
  (print (call (. p bar) 6))
)
```

```stderr
Unhandled panic: default bar 6
<no file:10>

Task 1

Stack trace:
  0  __source__:Proto:bar no file:10:5
  1  __source__:foo no file:22:10
  2  __source__:main no file:4:3
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### A panic in a function of an interface, run for a class that implements one derived from it: the trace has the function, the class's own frame for it, and the frame of the value.

```brix crash
(main
  (let thing (call Thing))
  (let pair (as thing Foobar))
  (print (call (. pair show) 2))
  (print (call (. pair show) 7))
)

(type + Foo (is Interface)
  (fn-do + show Int
    (param n Int)
    (throw (call Panic (str "show " n)) (if (> n 3)))
    (return n)
  )
)

(type + Foobar (is Interface)
  (implements Foo)
)

(type + Thing
  (implements Foobar)
)
```

```stderr
Unhandled panic: show 7
<no file:12>

Task 1

Stack trace:
  0  __source__:Foo:show no file:12:5
  1  __source__:show no file:10:3
  2  __source__:show(wrapped) no file:10:3
  3  __source__:main no file:6:10
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### A panic in a function of an interface called on an object whose class declares none: the trace has the function as the interface's and as the class's.

```brix crash
(main
  (let thing (call Thing))
  (print (call (. thing show) 1))
  (print (call (. thing show) 8))
)

(type + Foo (is Interface)
  (fn-do + show Int
    (param n Int)
    (throw (call Panic (str "show " n)) (if (> n 3)))
    (return n)
  )
)

(type + Thing
  (implements Foo)
)
```

```stderr
Unhandled panic: show 8
<no file:11>

Task 1

Stack trace:
  0  __source__:Foo:show no file:11:5
  1  __source__:show no file:9:3
  2  __source__:main no file:5:10
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### A panic in a method of a number type, called through a value of an interface a function was given.

```brix crash
(main
  (let count Proto (call Count 9))
  (call foo count)
)

(type + Proto (is Interface)
  (fn-expr + checked Int
    (param n Int)
  )
)

(type + Count (is I32)
  (implements Proto)
  (fn-do + checked _
    (param n Int)
    (throw (call Panic (str "by " n)) (if (< n 2)))
    (return n)
  )
)

(fn-do + foo _
  (param p Proto)
  (print (call (. p checked) 1))
)
```

```stderr
Unhandled panic: by 1
<no file:17>

Task 1

Stack trace:
  0  __source__:Count:checked no file:17:5
  1  __source__:checked(wrapped) no file:15:3
  2  __source__:foo no file:24:10
  3  __source__:main no file:4:3
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### A panic in a method of a tuple type, called through a value of an interface a function was given.

```brix crash
(main
  (let count Proto (call Count 1 2))
  (call foo count)
)

(type + Proto (is Interface)
  (fn-expr + checked Int
    (param n Int)
  )
)

(type + Count (is (tuple (: a Int) (: b Int)))
  (implements Proto)
  (fn-do + checked _
    (param n Int)
    (throw (call Panic (str "by " n)) (if (< n 2)))
    (return n)
  )
)

(fn-do + foo _
  (param p Proto)
  (print (call (. p checked) 1))
)
```

```stderr
Unhandled panic: by 1
<no file:17>

Task 1

Stack trace:
  0  __source__:Count:checked no file:17:5
  1  __source__:checked(wrapped) no file:15:3
  2  __source__:foo no file:24:10
  3  __source__:main no file:4:3
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### A function of an interface run through a value of one derived from it has the declaring interface as its receiver.

```brix
(main
  (let thing (call Thing))
  (let pair (as thing Foobar))
  (call (. pair show))
  (call (. pair tell))
  (let one (as pair Foo))
  (call (. one show))
)

(type + Foo (is Interface)
  (fn-do + show _
    (print (str "show " this))
    (call tell)
  )
  (fn-expr + tell _)
)

(type + Foobar (is Interface)
  (implements Foo)
)

(type + Thing
  (implements Foobar)
  (fn-do + tell _
    (print (str "tell " this))
  )
)
```

```output
show [Interface __source__:Foo]
tell [Object __source__:Thing]
tell [Object __source__:Thing]
show [Interface __source__:Foo]
tell [Object __source__:Thing]
```

### A function of an interface called on an object has the interface as its receiver, and calls the class's own function.

```brix
(main
  (let thing (call Thing))
  (call (. thing show))
)

(type + Proto (is Interface)
  (fn-do + show _
    (print (str "show " this))
    (call tell)
  )
  (fn-expr + tell _)
)

(type + Thing
  (implements Proto)
  (fn-do + tell _
    (print (str "tell " this))
  )
)
```

```output
show [Interface __source__:Proto]
tell [Object __source__:Thing]
```

### Whole numbers, a float, a tuple and a text held as values of an interface.

```brix
(main
  (let count (as (call Count 42) Proto))
  (call (. count show))
  (print (call (. count twice) 2))
  (let real (as (call Real 3.5) Proto))
  (call (. real show))
  (print (call (. real twice) 2))
  (let pair (as (call Pair 1 2.5) Proto))
  (call (. pair show))
  (print (call (. pair twice) 3))
  (let label (as (call Label "hi") Proto))
  (call (. label show))
  (print (call (. label twice) 1))
  (let small (as (call Small 200) Proto))
  (call (. small show))
  (let minus (as (call Count -7) Proto))
  (call (. minus show))
  (print (call (. minus twice) 3))
)

(type + Proto (is Interface)
  (fn-expr + show _)
  (fn-expr + twice Int
    (param n Int)
  )
)

(type + Count (is I32)
  (implements Proto)
  (fn-do + show _
    (print (str "count " this))
  )
  (fn-expr + twice _
    (param n Int)
    (* this n)
  )
)

(type + Small (is U8)
  (implements Proto)
  (fn-do + show _
    (print (str "small " this))
  )
  (fn-expr + twice _
    (param n Int)
    n
  )
)

(type + Real (is F64)
  (implements Proto)
  (fn-do + show _
    (print (str "real " this))
  )
  (fn-expr + twice _
    (param n Int)
    (* n 2)
  )
)

(type + Label (is String)
  (implements Proto)
  (fn-do + show _
    (print (str "label " this))
  )
  (fn-expr + twice _
    (param n Int)
    (+ n 100)
  )
)

(type + Pair (is (tuple (: a Int) (: b Float)))
  (implements Proto)
  (fn-do + show _
    (print (str "pair " a " " b))
  )
  (fn-expr + twice _
    (param n Int)
    (* a n)
  )
)
```

```output
count 42
84
real 3.5
4
pair 1 2.5
3
label hi
101
small 200
count -7
-21
```

### A getter and a setter of an interface answered by a stored property and by functions, and the type of what a value holds.

```brix
(main
  (let leaf (call Leaf))
  (let node Node leaf)
  (= (. node id) (+ (. node id) 10))
  (print (. node id))
  (print (. leaf id))
  (= (. node name) "pear")
  (print (. leaf name))
  (let branch Node (call Branch))
  (= (. branch id) 7)
  (print (. branch id))
  (print (. (type-of node) concrete-name))
  (print (. (type-of branch) concrete-name))
  (print (. (type-of node) name))
  (print (type-of node))
)

(type + Node (is Interface)
  (fn-expr + id Size :get)
  (fn-do + id _ :set (param value Size))
  (fn-do + name _ :set (param value String))
)

(type + Leaf
  (implements Node)
  (prop + id Size (= 1))
  (prop + name _ (= "apple"))
)

(type + Branch
  (implements Node)
  (prop _ stored Size (= 2))
  (fn-expr + id _ :get
    stored
  )
  (fn-do + id _ :set (param value Size)
    (= stored (* value 2))
  )
  (fn-do + name _ :set (param value String)
    (print (str "named " value))
  )
)
```

```output
11
11
pear
14
Leaf
Branch
Node
[TypeInfo Node]
```

### A function of an interface with untyped arguments, called through a value with several types.

```brix
(main
  (let proto Proto (call Thing))
  (call (. proto foo) 1)
  (call (. proto foo) 1.5)
  (call (. proto foo) "text")
  (call (. proto pair) 1 "one")
  (call (. proto pair) 2.5 3)
)

(type + Proto (is Interface)
  (fn-expr + foo _
    (param n _)
  )
  (fn-expr + pair _
    (param a _)
    (param b _)
  )
)

(type + Thing
  (implements Proto)
  (fn-do + foo _
    (param n _)
    (print n)
  )
  (fn-do + pair _
    (param a _)
    (param b _)
    (print (str a " and " b))
  )
)
```

```output
1
1.5
text
1 and one
2.5 and 3
```

### A value of an interface cast to the class it holds and to a class that one is derived from.

```brix
(main
  (let a Node (call Leaf))
  (let b Node (call Twig))
  (let c Node (call Branch 3))
  (print (. (as c Branch) kids))
  (print (. (as a Leaf) name))
  (print (. (as b Leaf) name))
  (print (. (as b Twig) name))
)

(type + Node (is Interface)
  (fn-expr + weight Int :get)
)

(type + Leaf
  (implements Node)
  (prop + weight _ (= 1))
  (fn-expr + name _ :get
    "leaf"
  )
)

(type + Twig (is Leaf)
  (fn-expr + name _ :get
    "twig"
  )
)

(type + Branch
  (implements Node)
  (prop + kids Int)
  (prop + weight _ (= 2))
  (fn-do + This _
    (param kids _)
  )
)

(type + Stone (is Tuple)
  (implements Node)
  (prop + weight _ (= 3))
)
```

```output
3
leaf
twig
twig
```

### A value of an interface tested with is against the classes it may hold.

```brix
(main
  (let a Node (call Leaf))
  (let b Node (call Twig))
  (let c Node (call Branch 3))
  (let d Node (call Stone))
  (print (is a Leaf))
  (print (is b Leaf))
  (print (is c Leaf))
  (print (is d Leaf))
  (print (is a Twig))
  (print (is b Twig))
  (print (is c Branch))
  (print (is d Branch))
)

(type + Node (is Interface)
  (fn-expr + weight Int :get)
)

(type + Leaf
  (implements Node)
  (prop + weight _ (= 1))
  (fn-expr + name _ :get
    "leaf"
  )
)

(type + Twig (is Leaf)
  (fn-expr + name _ :get
    "twig"
  )
)

(type + Branch
  (implements Node)
  (prop + kids Int)
  (prop + weight _ (= 2))
  (fn-do + This _
    (param kids _)
  )
)

(type + Stone (is Tuple)
  (implements Node)
  (prop + weight _ (= 3))
)
```

```output
true
true
false
false
false
true
true
false
```

### A match on a value of an interface by the classes it may hold.

```brix
(main
  (let a Node (call Leaf))
  (let b Node (call Twig))
  (let c Node (call Branch 3))
  (let d Node (call Stone))
  (print (call describe a))
  (print (call describe b))
  (print (call describe c))
  (print (call describe d))
)

(fn-expr + describe String
  (param node Node)
  (match node
    (case (: leaf Leaf)
      (str "leaf " (. leaf name))
    )
    (case (: branch Branch)
      (str "branch " (. branch kids))
    )
    (default
      "other"
    )
  )
)

(type + Node (is Interface)
  (fn-expr + weight Int :get)
)

(type + Leaf
  (implements Node)
  (prop + weight _ (= 1))
  (fn-expr + name _ :get
    "leaf"
  )
)

(type + Twig (is Leaf)
  (fn-expr + name _ :get
    "twig"
  )
)

(type + Branch
  (implements Node)
  (prop + kids Int)
  (prop + weight _ (= 2))
  (fn-do + This _
    (param kids _)
  )
)

(type + Stone (is Tuple)
  (implements Node)
  (prop + weight _ (= 3))
)
```

```output
leaf leaf
leaf twig
branch 3
other
```

### A value of an interface tested as a tuple type and cast to it.

```brix
(main
  (let a Node (call Leaf))
  (let d Node (call Stone))
  (print (is d Stone))
  (print (is a Stone))
  (let s (as d Stone))
  (print (. s weight))
)

(type + Node (is Interface)
  (fn-expr + weight Int :get)
)

(type + Leaf
  (implements Node)
  (prop + weight _ (= 1))
  (fn-expr + name _ :get
    "leaf"
  )
)

(type + Twig (is Leaf)
  (fn-expr + name _ :get
    "twig"
  )
)

(type + Branch
  (implements Node)
  (prop + kids Int)
  (prop + weight _ (= 2))
  (fn-do + This _
    (param kids _)
  )
)

(type + Stone (is Tuple)
  (implements Node)
  (prop + weight _ (= 3))
)
```

```output
true
false
3
```

### A value of an interface cast to a class it does not hold panics.

```brix
(main
  (do-new
    (let a Node (call Leaf))
    (let c (as a Branch))
    (print (. c kids))
    (on-failure failure
      (print failure)
    )
  )
)

(type + Node (is Interface)
  (fn-expr + weight Int :get)
)

(type + Leaf
  (implements Node)
  (prop + weight _ (= 1))
  (fn-expr + name _ :get
    "leaf"
  )
)

(type + Twig (is Leaf)
  (fn-expr + name _ :get
    "twig"
  )
)

(type + Branch
  (implements Node)
  (prop + kids Int)
  (prop + weight _ (= 2))
  (fn-do + This _
    (param kids _)
  )
)

(type + Stone (is Tuple)
  (implements Node)
  (prop + weight _ (= 3))
)
```

```output
Invalid cast to [Object __source__:Branch]
```

### A tuple held as a value of an interface, cast to a class, panics.

```brix
(main
  (do-new
    (let d Node (call Stone))
    (let c (as d Branch))
    (print (. c kids))
    (on-failure failure
      (print failure)
    )
  )
)

(type + Node (is Interface)
  (fn-expr + weight Int :get)
)

(type + Leaf
  (implements Node)
  (prop + weight _ (= 1))
  (fn-expr + name _ :get
    "leaf"
  )
)

(type + Twig (is Leaf)
  (fn-expr + name _ :get
    "twig"
  )
)

(type + Branch
  (implements Node)
  (prop + kids Int)
  (prop + weight _ (= 2))
  (fn-do + This _
    (param kids _)
  )
)

(type + Stone (is Tuple)
  (implements Node)
  (prop + weight _ (= 3))
)
```

```output
Invalid cast to [Object __source__:Branch]
```

### An object held as a value of an interface, cast to a tuple type, panics.

```brix
(main
  (do-new
    (let a Node (call Leaf))
    (let s (as a Stone))
    (print (. s weight))
    (on-failure failure
      (print failure)
    )
  )
)

(type + Node (is Interface)
  (fn-expr + weight Int :get)
)

(type + Leaf
  (implements Node)
  (prop + weight _ (= 1))
  (fn-expr + name _ :get
    "leaf"
  )
)

(type + Twig (is Leaf)
  (fn-expr + name _ :get
    "twig"
  )
)

(type + Branch
  (implements Node)
  (prop + kids Int)
  (prop + weight _ (= 2))
  (fn-do + This _
    (param kids _)
  )
)

(type + Stone (is Tuple)
  (implements Node)
  (prop + weight _ (= 3))
)
```

```output
Invalid cast to Stone
```

### An optional of an interface given objects, a tuple and null.

```brix
(main
  (let a (? Node) (call pick 0))
  (let b (? Node) (call pick 1))
  (let c (? Node) (call pick 2))
  (print (?? (?. a weight) 0))
  (print (?? (?. b weight) 0))
  (print (?? (?. c weight) 0))
  (let w (as (call Stone) (? Node)))
  (print (?? (?. w weight) 0))
)

(fn-expr + pick (? Node)
  (param index Int)
  (case? (== index 0)
    (call Leaf)
  )
  (case? (== index 1)
    (call Branch 4)
  )
  (default
    null
  )
)

(type + Node (is Interface)
  (fn-expr + weight Int :get)
)

(type + Leaf
  (implements Node)
  (prop + weight _ (= 1))
  (fn-expr + name _ :get
    "leaf"
  )
)

(type + Twig (is Leaf)
  (fn-expr + name _ :get
    "twig"
  )
)

(type + Branch
  (implements Node)
  (prop + kids Int)
  (prop + weight _ (= 2))
  (fn-do + This _
    (param kids _)
  )
)

(type + Stone (is Tuple)
  (implements Node)
  (prop + weight _ (= 3))
)
```

```output
1
2
0
3
```

### A tuple derived from another and a struct held as values of an interface: a test names the exact type, and a cast gives a copy.

```brix
(main
  (let a Node (call Leaf))
  (let d Node (call Stone))
  (let p Node (call Pebble))
  (let q Node (call Rock 4 5))
  (print (tuple (. d weight) (. p weight) (. q weight)))
  (print (is p Stone))
  (print (is p Pebble))
  (print (is d Pebble))
  (print (is q Rock))
  (print (is a Rock))
  (let r (as q Rock))
  (print (tuple (. r x) (. r y) (. r weight)))
  (= (. r x) 9)
  (print (tuple (. r x) (. (as q Rock) x)))
)

(type + Pebble (is Stone)
  (prop + extra _ (= 4))
)

(type + Rock (is Struct)
  (implements Node)
  (prop + x Int)
  (prop + y Int)
  (fn-expr + weight Int :get
    (+ x y)
  )
)

(type + Node (is Interface)
  (fn-expr + weight Int :get)
)

(type + Leaf
  (implements Node)
  (prop + weight _ (= 1))
  (fn-expr + name _ :get
    "leaf"
  )
)

(type + Twig (is Leaf)
  (fn-expr + name _ :get
    "twig"
  )
)

(type + Branch
  (implements Node)
  (prop + kids Int)
  (prop + weight _ (= 2))
  (fn-do + This _
    (param kids _)
  )
)

(type + Stone (is Tuple)
  (implements Node)
  (prop + weight _ (= 3))
)
```

```output
(3, 3, 9)
false
true
false
true
false
(4, 5, 9)
(9, 4)
```

### Objects and a tuple tested as an interface they implement.

```brix
(main
  (let leaf (call Leaf))
  (let stone (call Stone))
  (let twig (call Twig))
  (print (is leaf Node))
  (print (is stone Node))
  (print (is twig Node))
  (print (is 5 Node))
  (print (call name leaf))
  (print (call name stone))
  (print (call name twig))
)

(fn-expr + name String
  (param thing _)
  (match thing
    (case (: node Node)
      (str "node " (. node weight))
    )
    (default
      "other"
    )
  )
)

(type + Node (is Interface)
  (fn-expr + weight Int :get)
)

(type + Leaf
  (implements Node)
  (prop + weight _ (= 1))
  (fn-expr + name _ :get
    "leaf"
  )
)

(type + Twig (is Leaf)
  (fn-expr + name _ :get
    "twig"
  )
)

(type + Branch
  (implements Node)
  (prop + kids Int)
  (prop + weight _ (= 2))
  (fn-do + This _
    (param kids _)
  )
)

(type + Stone (is Tuple)
  (implements Node)
  (prop + weight _ (= 3))
)
```

```output
true
true
true
false
node 1
node 3
node 1
```

### Tests of a value of an interface in if, is not and match?.

```brix
(main
  (let a Node (call Leaf))
  (let b Node (call Twig))
  (let c Node (call Branch 3))
  (let d Node (call Stone))
  (print (tuple (call tell a) (call tell b) (call tell c) (call tell d)))
  (let e (? Node) b)
  (if (is e Leaf)
    (print "is leaf")
  )
  (match e
    (case (: twig Twig)
      (print (. twig name))
    )
    (default
      (print "no")
    )
  )
)

(fn-do + tell String
  (param node Node)
  (if (is node Twig)
    (return "twig")
  )
  (if (is-not node Leaf)
    (return "not leaf")
  )
  (match? node
    (case (: leaf Leaf)
      (return (. leaf name))
    )
  )
  (return "none")
)

(type + Node (is Interface)
  (fn-expr + weight Int :get)
)

(type + Leaf
  (implements Node)
  (prop + weight _ (= 1))
  (fn-expr + name _ :get
    "leaf"
  )
)

(type + Twig (is Leaf)
  (fn-expr + name _ :get
    "twig"
  )
)

(type + Branch
  (implements Node)
  (prop + kids Int)
  (prop + weight _ (= 2))
  (fn-do + This _
    (param kids _)
  )
)

(type + Stone (is Tuple)
  (implements Node)
  (prop + weight _ (= 3))
)
```

```output
(leaf, twig, not leaf, not leaf)
is leaf
twig
```

### An object held as a base class converts to an interface with the table of its own class.

```brix
(main
  (let base Base (call Child))
  (let shape Shape base)
  (print (call (. shape name)))
  (let plain Shape (call Base))
  (print (call (. plain name)))
  (let grand Base (call Grand))
  (let third Shape grand)
  (print (call (. third name)))
  (print (. third sides))
  (print (. shape sides))
)

(type + Shape (is Interface)
  (fn-expr + name String)
  (fn-expr + sides Int :get)
)

(type + Base
  (implements Shape)
  (prop + sides _ (= 3))
  (fn-expr + name _
    "base"
  )
)

(type + Child (is Base)
  (fn-expr + name _
    "child"
  )
)

(type + Grand (is Child))
```

```output
child
base
child
3
3
```

### A value of an interface derived from two others, given where each is asked for.

```brix
(main
  (let whole Both (call Thing))
  (print (call (. whole first)))
  (print (call (. whole second)))
  (print (call (. whole third)))
  (let one First whole)
  (print (call (. one first)))
  (let two Second whole)
  (print (call (. two second)))
  (call tell whole)
)

(type + First (is Interface)
  (fn-expr + first String)
)

(type + Second (is Interface)
  (fn-expr + second String)
)

(type + Both (is Interface)
  (implements First)
  (implements Second)
  (fn-expr + third String)
)

(type + Thing
  (implements Both)
  (fn-expr + first _
    "first"
  )
  (fn-expr + second _
    "second"
  )
  (fn-expr + third _
    "third"
  )
)

(fn-do + tell _
  (param second Second)
  (print (str "told " (call (. second second))))
)
```

```output
first
second
third
first
second
told second
```

### Values of an interface kept in a property, a tuple and an optional, and returned.

```brix
(main
  (let holder (call Holder (call Dog)))
  (print (call (. holder pet sound)))
  (= (. holder pet) (call Cat))
  (print (call (. holder pet sound)))
  (print (tuple (call (. (call pick true) sound)) (call (. (call pick false) sound))))
  (let pair (tuple (as (call Dog) Pet) (as (call Cat) Pet)))
  (print (tuple (call (. ([] pair 0) sound)) (call (. ([] pair 1) sound))))
  (print (?? (call (?. (call find 0) sound)) "silence"))
  (print (?? (call (?. (call find 1) sound)) "silence"))
  (print (?? (call (?. (call find 2) sound)) "silence"))
)

(type + Pet (is Interface)
  (fn-expr + sound String)
)

(type + Dog
  (implements Pet)
  (fn-expr + sound _
    "woof"
  )
)

(type + Cat
  (implements Pet)
  (fn-expr + sound _
    "meow"
  )
)

(type + Holder
  (prop + pet Pet)
  (fn-do + This _
    (param pet _)
  )
)

(fn-expr + pick Pet
  (param dog Bool)
  (case? dog
    (call Dog)
  )
  (default
    (call Cat)
  )
)

(fn-expr + find (? Pet)
  (param which Int)
  (case? (== which 1)
    (call Dog)
  )
  (case? (== which 2)
    (call Cat)
  )
  (default
    null
  )
)
```

```output
woof
meow
(woof, meow)
(woof, meow)
silence
woof
meow
```

### Boxed values of an interface survive collection while a list alone holds them.

```brix
(main
  (let kept (list Shape))
  (let i 0)
  (while (< i 200)
    (+= kept (call Pair i 0.5))
    (+= kept (call Label (str "label " i)))
    (+= kept (call Count i))
    (call churn i)
    (+= i 1)
  )
  (let sum 0)
  (= i 0)
  (while (< i (. kept length))
    (+= sum (call (. ([] kept i) weight)))
    (+= i 1)
  )
  (print sum)
  (print (tuple (call (. ([] kept 4) weight)) (call (. ([] kept 301) weight)) (call (. ([] kept 599) weight))))
)

(type + Shape (is Interface)
  (fn-expr + weight Int)
)

(type + Pair (is (tuple (: a Int) (: b Float)))
  (implements Shape)
  (fn-expr + weight _
    a
  )
)

(type + Label (is String)
  (implements Shape)
  (fn-expr + weight _
    (as size Int)
  )
)

(type + Count (is I32)
  (implements Shape)
  (fn-expr + weight _
    (* this 2)
  )
)

(fn-do + churn _
  (param n Int)
  (let junk (list String))
  (let j 0)
  (while (< j 50)
    (+= junk (str "junk " n " " j))
    (+= j 1)
  )
)
```

```output
61390
(7, 9, 398)
```

### A function an interface declares with a body calls a getter and a function that each class answers.

```brix
(main
  (let square Shape (call Square 3))
  (let circle Shape (call Circle 2))
  (print (call (. square describe)))
  (print (call (. circle describe)))
  (print (call (. (call Square 4) describe)))
)

(type + Shape (is Interface)
  (fn-expr + area Int)
  (fn-expr + name String :get)
  (fn-expr + describe String
    (scope (str name " of " area)
      (lazy area (call (. this area)))
    )
  )
)

(type + Square
  (implements Shape)
  (prop + side Int)
  (prop + name _ (= "square"))
  (fn-do + This _
    (param side _)
  )
  (fn-expr + area _
    (* side side)
  )
)

(type + Circle
  (implements Shape)
  (prop + radius Int)
  (fn-do + This _
    (param radius _)
  )
  (fn-expr + area _
    (* radius radius 3)
  )
  (fn-expr + name _ :get
    "circle"
  )
)
```

```output
square of 9
circle of 12
square of 16
```

### A method that changes its receiver, called through a value of an interface: an object is changed, and a struct the value holds is not.

```brix
(main
  (let counter (call Counter 1))
  (let step Stepper counter)
  (call (. step bump))
  (call (. step bump))
  (print (call (. step value)))
  (print (. counter n))
  (let boxed Stepper (call Tally 5))
  (call (. boxed bump))
  (print (call (. boxed value)))
)

(type + Stepper (is Interface)
  (fn-expr + bump _)
  (fn-expr + value Int)
)

(type + Counter
  (implements Stepper)
  (prop + n Int)
  (fn-do + This _
    (param n _)
  )
  (fn-do + bump _
    (+= n 1)
  )
  (fn-expr + value _
    n
  )
)

(type + Tally (is Struct)
  (implements Stepper)
  (prop + n Int)
  (fn-do + bump _
    (+= n 1)
  )
  (fn-expr + value _
    n
  )
)
```

```output
3
3
5
```

### An interface with a type parameter, implemented for two types.

```brix
(main
  (let box (of Box Int) (call IntBox 4))
  (print (. box item))
  (print (call (. box doubled)))
  (let words (of Box String) (call WordBox "hey"))
  (print (. words item))
  (print (call (. words doubled)))
)

(type + (of Box T) (is Interface)
  (fn-expr + item T :get)
  (fn-expr + doubled T
    (+ item item)
  )
)

(type + IntBox
  (implements (of Box Int))
  (prop + item Int)
  (fn-do + This _
    (param item _)
  )
)

(type + WordBox
  (implements (of Box String))
  (prop + item String)
  (fn-do + This _
    (param item _)
  )
)
```

```output
4
8
hey
heyhey
```

### A tree of values of an interface, each node of a class of its own.

```brix
(main
  (let tree Node (call Branch (call Leaf 2) (call Branch (call Leaf 3) (call Leaf 4))))
  (print (call (. tree weight)))
  (print (call (. tree depth)))
  (let leaf Node (call Leaf 9))
  (print (tuple (call (. leaf weight)) (call (. leaf depth))))
)

(type + Node (is Interface)
  (fn-expr + weight Int)
  (fn-expr + depth Int
    1
  )
)

(type + Leaf
  (implements Node)
  (prop + n Int)
  (fn-do + This _
    (param n _)
  )
  (fn-expr + weight _
    n
  )
)

(type + Branch
  (implements Node)
  (prop + left Node)
  (prop + right Node)
  (fn-do + This _
    (param left _)
    (param right _)
  )
  (fn-expr + weight _
    (+ (call (. left weight)) (call (. right weight)))
  )
  (fn-do + depth _
    (let a (call (. left depth)))
    (let b (call (. right depth)))
    (return (+ (if-expr (> a b) a b) 1))
  )
)
```

```output
9
3
(9, 1)
```

### A value of an interface captured by a closure, assigned after, and kept in a struct.

```brix
(main
  (let pet Pet (call Dog))
  (let speak (=> () (call (. pet sound))))
  (print (call speak))
  (= pet (call Cat))
  (print (call speak))
  (print (call call speak))
  (let held (call Held pet 3))
  (print (tuple (call (. held pet sound)) (. held count)))
)

(type + Pet (is Interface)
  (fn-expr + sound String)
)

(type + Dog
  (implements Pet)
  (fn-expr + sound _
    "woof"
  )
)

(type + Cat
  (implements Pet)
  (fn-expr + sound _
    "meow"
  )
)

(type + Held (is Struct)
  (prop + pet Pet)
  (prop + count Int)
)

(fn-do + call String
  (param fn (fn-type String))
  (return (call fn))
)
```

```output
woof
meow
meow
(meow, 3)
```

### A class that implements two interfaces has a table for each.

```brix
(main
  (let thing (call Thing))
  (let a A thing)
  (let b B thing)
  (print (tuple (call (. a name)) (call (. b name))))
  (print (tuple (. (type-of a) concrete-name) (. (type-of b) concrete-name)))
  (print (tuple (is a Thing) (is b Thing)))
  (print (call (. (as b Thing) name)))
  (call show thing)
  (call show (call Other))
)

(type + A (is Interface)
  (fn-expr + name String)
)

(type + B (is Interface)
  (fn-expr + name String)
)

(type + Thing
  (implements A)
  (implements B)
  (fn-expr + name _
    "thing"
  )
)

(type + Other
  (implements A)
  (fn-expr + name _
    "other"
  )
)

(fn-do + show _
  (param a A)
  (print (str "show " (call (. a name))))
)
```

```output
(thing, thing)
(Thing, Thing)
(true, true)
thing
show thing
show other
```

### A function an interface declares with a body that cannot panic, called through a value.

```brix
(main
  (let p Proto (call Thing))
  (print (call (. p name)))
  (print "after")
)

(type + Proto (is Interface)
  (fn-expr + name String
    "proto name"
  )
)

(type + Thing
  (implements Proto)
)
```

```output
proto name
after
```

### A function an interface declares with a body, run through a value that holds a number, has the number as its receiver.

```brix
(main
  (let n Proto (call Num 3))
  (call (. n show))
)

(type + Proto (is Interface)
  (fn-do + show _
    (print (str "show " this))
  )
)

(type + Num (is I32)
  (implements Proto)
)
```

```output
show 3
```

### A function an interface declares with a body, run on a number and on a tuple, has each as its receiver.

```brix
(main
  (call (. (call Num 4) show))
  (call (. (call Tup 1 2) show))
)

(type + Proto (is Interface)
  (fn-do + show _
    (print (str "show " this))
  )
)

(type + Num (is I32)
  (implements Proto)
)

(type + Tup (is (tuple (: a Int) (: b Int)))
  (implements Proto)
)
```

```output
show 4
show (1, 2)
```

### A panic in a function an interface declares with a body, run through a value that holds a number.

```brix crash
(main
  (let n Proto (call Num 3))
  (call run n)
)

(fn-do + run _
  (param p Proto)
  (print (call (. p show) 1))
  (print (call (. p show) 9))
)

(type + Proto (is Interface)
  (fn-do + show Int
    (param n Int)
    (throw (call Panic (str "show " n)) (if (> n 3)))
    (return n)
  )
)

(type + Num (is I32)
  (implements Proto)
)
```

```stderr
Unhandled panic: show 9
<no file:16>

Task 1

Stack trace:
  0  __source__:Proto:show no file:16:5
  1  __source__:show(wrapped) no file:14:3
  2  __source__:run no file:10:10
  3  __source__:main no file:4:3
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### A panic in a function an interface declares with a body, run on a tuple.

```brix crash
(main
  (let n (call Tup 3 4))
  (print (call (. n show) 1))
  (print (call (. n show) 9))
)

(type + Proto (is Interface)
  (fn-do + show Int
    (param n Int)
    (throw (call Panic (str "show " n)) (if (> n 3)))
    (return n)
  )
)

(type + Tup (is (tuple (: a Int) (: b Int)))
  (implements Proto)
)
```

```stderr
Unhandled panic: show 9
<no file:11>

Task 1

Stack trace:
  0  __source__:Proto:show no file:11:5
  1  __source__:main no file:5:10
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

### A function of a delegate that changes it, called on the class and through a value: both change the one object.

```brix
(main
  (let thing (call Thing))
  (call (. thing bump))
  (call (. thing bump))
  (print (. thing count))
  (let proto Proto thing)
  (call (. proto bump))
  (print (. proto count))
  (print (. thing foo count))
)

(type + Proto (is Interface)
  (fn-expr + bump _)
  (fn-expr + count Int :get)
)

(type + Foo
  (prop + count _ (= 0))
  (fn-do + bump _
    (+= count 1)
  )
)

(type + Thing
  (delegate Proto)
  (prop + foo Foo)
)
```

```output
2
3
3
```

### A function of a delegate with untyped arguments, called with whole numbers and a float, and a getter the interface declares with a body.

```brix
(main
  (let thing (call Thing))
  (print (call (. thing total) 1 2))
  (print (call (. thing total) 1.5 2))
  (let proto Proto thing)
  (print (call (. proto total) 3 4))
  (print (. thing name))
  (print (. proto name))
)

(type + Proto (is Interface)
  (fn-expr + total Int
    (param a Int)
    (param b Int)
  )
  (fn-expr + name String :get
    "proto"
  )
)

(type + Foo
  (fn-expr + total _
    (param a _)
    (param b _)
    (+ a b)
  )
)

(type + Thing
  (delegate Proto)
  (prop + foo Foo)
)
```

```output
3
3.5
7
proto
proto
```

### A class that delegates two interfaces to two properties.

```brix
(main
  (let thing (call Thing))
  (call (. thing act))
  (call (. thing other))
  (let proto Proto thing)
  (call (. proto act))
  (let second Second thing)
  (call (. second other))
)

(type + Proto (is Interface)
  (fn-expr + act _)
)

(type + Second (is Interface)
  (fn-expr + other _)
)

(type + Foo
  (fn-do + act _
    (print "foo act")
  )
  (fn-do + other _
    (print "foo other")
  )
)

(type + Bar
  (fn-do + act _
    (print "bar act")
  )
  (fn-do + other _
    (print "bar other")
  )
)

(type + Thing
  (delegate Proto)
  (prop + foo Foo)
  (blank)
  (delegate Second)
  (prop + bar Bar)
)
```

```output
foo act
bar other
foo act
bar other
```

### A function an interface declares with a body runs for the delegate where the class declares none, on the class and through a value, and calls the delegate's own.

```brix
(main
  (let thing (call Thing))
  (call (. thing act))
  (let proto Proto thing)
  (call (. proto act))
  (call (. proto rest))
  (call (. thing rest))
)

(type + Proto (is Interface)
  (fn-expr + act _)
  (fn-do + rest _
    (print "proto rest")
    (call act)
  )
)

(type + Foo
  (implements Proto)
  (fn-do + act _
    (print "foo act")
  )
)

(type + Thing
  (delegate Proto)
  (prop + foo Foo)
  (fn-do + act _
    (print "thing act")
  )
)
```

```output
thing act
thing act
proto rest
foo act
proto rest
foo act
```

### A class derived from one that delegates declares a function of the interface itself; the rest stay the delegate's.

```brix
(main
  (let thing (call Derived))
  (call (. thing act))
  (call (. thing more))
  (let proto Proto thing)
  (call (. proto act))
  (call (. proto more))
  (let base Thing thing)
  (call (. base act))
  (let seen Proto base)
  (call (. seen more))
)

(type + Proto (is Interface)
  (fn-expr + act _)
  (fn-expr + more _)
)

(type + Foo
  (fn-do + act _
    (print "foo act")
  )
  (fn-do + more _
    (print "foo more")
  )
)

(type + Thing
  (delegate Proto)
  (prop + foo Foo)
)

(type + Derived (is Thing)
  (fn-do + more _
    (print "derived more")
  )
)
```

```output
foo act
derived more
foo act
derived more
foo act
derived more
```

### A class that delegates an interface is seen as the interface that one is derived from.

```brix
(main
  (let thing (call Thing))
  (let proto Proto thing)
  (call (. proto act))
  (call (. proto root))
  (let root Root thing)
  (call (. root root))
  (let up Root proto)
  (call (. up root))
)

(type + Root (is Interface)
  (fn-expr + root _)
)

(type + Proto (is Interface)
  (implements Root)
  (fn-expr + act _)
)

(type + Foo
  (fn-do + act _
    (print "foo act")
  )
  (fn-do + root _
    (print "foo root")
  )
)

(type + Thing
  (delegate Proto)
  (prop + foo Foo)
)
```

```output
foo act
foo root
foo root
foo root
```

### A function of a delegate that raises, caught on the class and through a value, and left to end the program.

```brix crash
(main
  (let thing (call Thing))
  (let proto Proto thing)
  (print (catch (call (. proto check) 1)))
  (print (catch (call (. proto check) -1)))
  (print (catch (call (. thing check) 2)))
  (print (catch (call (. thing check) -2)))
  (print (call (. proto check) -3))
)

(type + Proto (is Interface)
  (fn-expr + check Int (?! Problem)
    (param x Int)
  )
)

(type + Problem (is Error))

(type + Foo
  (fn-do + check Int (?! Problem)
    (param x Int)
    (throw (call Problem "negative") (if (< x 0)))
    (return (* x 2))
  )
)

(type + Thing
  (delegate Proto)
  (prop + foo Foo)
)
```

```output
2
negative
4
negative
```

### A value that holds a class that delegates is tested and cast as the class, and is no value of the delegate's class.

```brix
(main
  (let thing (call Thing))
  (let proto Proto thing)
  (print (is proto Thing))
  (print (is proto Foo))
  (let back (as proto Thing))
  (call (. back act))
  (print (. (type-of proto) concrete-name))
  (print (. (type-of proto) name))
)

(type + Proto (is Interface)
  (fn-expr + act _)
)

(type + Foo
  (fn-do + act _
    (print "foo act")
  )
)

(type + Thing
  (delegate Proto)
  (prop + foo Foo)
)
```

```output
true
false
foo act
Thing
Proto
```

### Values of an interface in a list, an optional and an argument, some holding a class that delegates.

```brix
(main
  (let thing (call Thing))
  (let foo Proto (call Foo))
  (let other Proto thing)
  (let list (list Proto) (list other foo other))
  (for (in item list)
    (call (. item act))
  )
  (let maybe (? Proto) thing)
  (call (?. maybe act))
  (call show thing)
)

(fn-do + show _
  (param proto Proto)
  (call (. proto act))
)

(type + Proto (is Interface)
  (fn-expr + act _)
)

(type + Foo
  (implements Proto)
  (prop + name _ (= "foo"))
  (fn-do + act _
    (print (str name " act"))
  )
)

(type + Thing
  (delegate Proto)
  (prop + foo Foo)
)
```

```output
foo act
foo act
foo act
foo act
foo act
```

### A property a class delegates to is assigned again: calls on the class and through a value made before reach the new object.

```brix
(main
  (let thing (call Thing))
  (call (. thing act))
  (= (. thing foo) (call Foo))
  (= (. thing foo tag) "second")
  (call (. thing act))
  (let proto Proto thing)
  (call (. proto act))
  (let third (call Foo))
  (= (. third tag) "third")
  (= (. thing foo) third)
  (call (. proto act))
  (call (. thing act))
)

(type + Proto (is Interface)
  (fn-expr + act _)
)

(type + Foo
  (prop + tag _ (= "first"))
  (fn-do + act _
    (print (str tag " act"))
  )
)

(type + Thing
  (delegate Proto)
  (prop + foo _ (= (call Foo)))
)
```

```output
first act
second act
second act
third act
third act
```

### A class that delegates declares a getter of the interface itself, and a function with a body the delegate runs reads the delegate's.

```brix
(main
  (let thing (call Thing))
  (print (. thing label))
  (print (call (. thing describe)))
  (let proto Proto thing)
  (print (. proto label))
  (print (call (. proto describe)))
)

(type + Proto (is Interface)
  (fn-expr + label String :get)
  (fn-expr + describe String
    (str "<" label ">")
  )
)

(type + Foo
  (implements Proto)
  (prop + label _ (= "foo"))
)

(type + Thing
  (delegate Proto)
  (prop + foo Foo)
  (fn-expr + label String :get
    "thing"
  )
)
```

```output
thing
<foo>
thing
<foo>
```

### A function an interface declares with a body reads a getter of the delegate it runs for.

```brix
(main
  (let thing (call Thing))
  (print (call (. thing describe)))
  (let proto Proto thing)
  (print (call (. proto describe)))
  (print (. proto label))
)

(type + Proto (is Interface)
  (fn-expr + label String :get)
  (fn-expr + describe String
    (str "<" label ">")
  )
)

(type + Foo
  (implements Proto)
  (fn-expr + label String :get
    "foo"
  )
)

(type + Thing
  (delegate Proto)
  (prop + foo Foo)
)
```

```output
<foo>
<foo>
foo
```

### A function of a delegate reads the delegate's own property, called on the class and through a value.

```brix
(main
  (let thing (call Thing))
  (print (call (. thing size)))
  (let proto Proto thing)
  (print (call (. proto size)))
)

(type + Proto (is Interface)
  (fn-expr + size String)
)

(type + Foo
  (implements Proto)
  (prop + label _ (= "foo"))
  (fn-expr + size String
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
foo
```

### A function an interface declares with a body panics, run through a value made from a value of an interface derived from it: the stack trace has the function as the class runs it and the function of the table.

```brix crash
(main
  (let thing (call Thing))
  (let pair (as thing Foobar))
  (let one (as pair Foo))
  (call (. one show))
)

(type + Foo (is Interface)
  (fn-do + show _
    (print "show")
    (let items (list 1 2 3))
    (print ([] items 5))
  )
)

(type + Foobar (is Interface)
  (implements Foo)
)

(type + Thing
  (implements Foobar)
)
```

```stderr
Unhandled panic: List index 5 out of range
<core/list.bx>

Task 1

Stack trace:
  0  core/list:List:absent core/list.bx
  1  __source__:Foo:show no file:13:12
  2  __source__:show no file:10:3
  3  __source__:show(wrapped) no file:10:3
  4  __source__:main no file:6:3
  5  __source__:init.__source__ no file
  6  __source__:brix.main no file
```

### A function an interface declares with a body calls one the class declares, which panics, through a value made from a value of a derived interface.

```brix crash
(main
  (let thing (call Thing))
  (let pair (as thing Foobar))
  (let one (as pair Foo))
  (call (. one show))
)

(type + Foo (is Interface)
  (fn-do + show _
    (print "show")
    (call tell)
  )
  (fn-expr + tell _)
)

(type + Foobar (is Interface)
  (implements Foo)
)

(type + Thing
  (implements Foobar)
  (fn-do + tell _
    (print "tell")
    (let items (list 1 2 3))
    (print ([] items 5))
  )
)
```

```stderr
Unhandled panic: List index 5 out of range
<core/list.bx>

Task 1

Stack trace:
  0  core/list:List:absent core/list.bx
  1  __source__:Thing:tell no file:26:12
  2  __source__:tell(wrapped) no file:23:3
  3  __source__:Foo:show no file:12:5
  4  __source__:show no file:10:3
  5  __source__:show(wrapped) no file:10:3
  6  __source__:main no file:6:3
  7  __source__:init.__source__ no file
  8  __source__:brix.main no file
```

### A function an interface declares with a body panics through a value of an interface two interfaces derive from in turn.

```brix crash
(main
  (let thing (call Thing))
  (let baz (as thing Baz))
  (let bar (as baz Bar))
  (let foo (as bar Foo))
  (call (. foo show))
)

(type + Foo (is Interface)
  (fn-do + show _
    (print "show")
    (let items (list 1 2 3))
    (print ([] items 5))
  )
)

(type + Bar (is Interface)
  (implements Foo)
)

(type + Baz (is Interface)
  (implements Bar)
)

(type + Thing
  (implements Baz)
)
```

```stderr
Unhandled panic: List index 5 out of range
<core/list.bx>

Task 1

Stack trace:
  0  core/list:List:absent core/list.bx
  1  __source__:Foo:show no file:14:12
  2  __source__:show no file:11:3
  3  __source__:show(wrapped) no file:11:3
  4  __source__:main no file:7:3
  5  __source__:init.__source__ no file
  6  __source__:brix.main no file
```

### A getter an interface declares with a body panics, read through a value: the stack trace has the getter and the function of the table.

```brix crash
(main
  (let thing (call Thing))
  (let one (as thing Foo))
  (print (. one name))
)

(type + Foo (is Interface)
  (fn-do + name String :get
    (let items (list 1 2 3))
    (print ([] items 5))
    (return "foo")
  )
)

(type + Foobar (is Interface)
  (implements Foo)
)

(type + Thing
  (implements Foobar)
)
```

```stderr
Unhandled panic: List index 5 out of range
<core/list.bx>

Task 1

Stack trace:
  0  core/list:List:absent core/list.bx
  1  __source__:Foo:name no file:11:12
  2  __source__:name(wrapped) no file:9:3
  3  __source__:main no file:5:10
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### A getter an interface declares with a body panics, read on an object: the stack trace has the getter alone.

```brix crash
(main
  (let thing (call Thing))
  (print (. thing name))
)

(type + Foo (is Interface)
  (fn-do + name String :get
    (let items (list 1 2 3))
    (print ([] items 5))
    (return "foo")
  )
)

(type + Foobar (is Interface)
  (implements Foo)
)

(type + Thing
  (implements Foobar)
)
```

```stderr
Unhandled panic: List index 5 out of range
<core/list.bx>

Task 1

Stack trace:
  0  core/list:List:absent core/list.bx
  1  __source__:Foo:name no file:10:12
  2  __source__:main no file:4:10
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### A getter an interface declares with a body panics, read through a value of an interface derived from it.

```brix crash
(main
  (let thing (call Thing))
  (let pair (as thing Foobar))
  (print (. pair name))
)

(type + Foo (is Interface)
  (fn-do + name String :get
    (let items (list 1 2 3))
    (print ([] items 5))
    (return "foo")
  )
)

(type + Foobar (is Interface)
  (implements Foo)
)

(type + Thing
  (implements Foobar)
)
```

```stderr
Unhandled panic: List index 5 out of range
<core/list.bx>

Task 1

Stack trace:
  0  core/list:List:absent core/list.bx
  1  __source__:Foo:name no file:11:12
  2  __source__:name(wrapped) no file:9:3
  3  __source__:main no file:5:10
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### A setter an interface declares with a body panics, assigned through a value.

```brix crash
(main
  (let thing (call Thing))
  (let one (as thing Foo))
  (= (. one name) "a")
)

(type + Foo (is Interface)
  (fn-do + name _ :set (param value String)
    (let items (list 1 2 3))
    (print ([] items 5))
    (print value)
  )
)

(type + Foobar (is Interface)
  (implements Foo)
)

(type + Thing
  (implements Foobar)
)
```

```stderr
Unhandled panic: List index 5 out of range
<core/list.bx>

Task 1

Stack trace:
  0  core/list:List:absent core/list.bx
  1  __source__:Foo:name no file:11:12
  2  __source__:name(wrapped) no file:9:3
  3  __source__:main no file:5:6
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### A function an interface declares with a body, run through a value of a base interface for an object of a derived class, calls the derived class's function, which panics.

```brix crash
(main
  (let thing (call Derived))
  (let pair (as thing Foobar))
  (let one (as pair Foo))
  (call (. one show))
  (call (. pair show))
  (let base Base thing)
  (let other (as base Foobar))
  (let last (as other Foo))
  (call (. last show))
)

(type + Foo (is Interface)
  (fn-do + show _
    (print (str "show " this))
    (call tell)
  )
  (fn-expr + tell _)
)

(type + Foobar (is Interface)
  (implements Foo)
)

(type + Base
  (implements Foobar)
  (fn-do + tell _
    (print "base tell")
  )
)

(type + Derived (is Base)
  (fn-do + tell _
    (print "derived tell")
    (let items (list 1 2 3))
    (print ([] items 5))
  )
)
```

```stderr
Unhandled panic: List index 5 out of range
<core/list.bx>

Task 1

Stack trace:
  0  core/list:List:absent core/list.bx
  1  __source__:Derived:tell no file:37:12
  2  __source__:tell(wrapped) no file:34:3
  3  __source__:Foo:show no file:17:5
  4  __source__:show no file:15:3
  5  __source__:show(wrapped) no file:15:3
  6  __source__:main no file:6:3
  7  __source__:init.__source__ no file
  8  __source__:brix.main no file
```

### A value of a base interface made by two ways from one object runs the interface's function by either, and the second call panics.

```brix crash
(main
  (let thing (call Thing))
  (let baz (as thing Baz))
  (let bar (as thing Bar))
  (let foo (as baz Foo))
  (let also (as bar Foo))
  (call (. foo show))
  (call (. also show))
)

(type + Foo (is Interface)
  (fn-do + show _
    (print "show")
    (call tell)
  )
  (fn-expr + tell _)
)

(type + Bar (is Interface)
  (implements Foo)
)

(type + Baz (is Interface)
  (implements Bar)
)

(type + Thing
  (implements Baz)
  (prop + count _ (= 0))
  (fn-do + tell _
    (+= count 1)
    (print (str "tell " count))
    (if (== count 2)
      (let items (list 1 2 3))
      (print ([] items 5))
    )
  )
)
```

```stderr
Unhandled panic: List index 5 out of range
<core/list.bx>

Task 1

Stack trace:
  0  core/list:List:absent core/list.bx
  1  __source__:Thing:tell no file:36:14
  2  __source__:tell(wrapped) no file:31:3
  3  __source__:Foo:show no file:15:5
  4  __source__:show no file:13:3
  5  __source__:show(wrapped) no file:13:3
  6  __source__:main no file:9:3
  7  __source__:init.__source__ no file
  8  __source__:brix.main no file
```

### A getter an interface declares with a body, read on an object, has the object as its receiver.

```brix
(main
  (let thing (call Thing))
  (print (. thing label))
)

(type + Foo (is Interface)
  (fn-do + label String :get
    (return (str "label of " this))
  )
  (fn-do + label _ :set (param value String)
    (print (str "label = " value " of " this))
  )
)

(type + Thing
  (implements Foo)
)
```

```output
label of [Object __source__:Thing]
```

### A getter and a setter an interface declares with bodies, used through a value, have the object as their receiver.

```brix
(main
  (let thing (call Thing))
  (let one (as thing Foo))
  (print (. one label))
  (= (. one label) "set")
)

(type + Foo (is Interface)
  (fn-do + label String :get
    (return (str "label of " this))
  )
  (fn-do + label _ :set (param value String)
    (print (str "label = " value " of " this))
  )
)

(type + Thing
  (implements Foo)
)
```

```output
label of [Object __source__:Thing]
label = set of [Object __source__:Thing]
```

### A getter an interface declares with a body calls a function the interface declares with a body, whose receiver is the interface.

```brix
(main
  (let thing (call Thing))
  (let one (as thing Foo))
  (print (. one label))
)

(type + Foo (is Interface)
  (fn-do + label String :get
    (return (call describe))
  )
  (fn-do + describe String
    (return (str "described " this))
  )
)

(type + Thing
  (implements Foo)
)
```

```output
described [Interface __source__:Foo]
```

### A function with an argument and an answer that an interface declares with a body, run through a value of the interface and through one made from a derived interface.

```brix crash
(main
  (let thing (call Thing))
  (let direct (as thing Foo))
  (let pair (as thing Foobar))
  (let one (as pair Foo))
  (print (call (. one show) 1))
  (print (call (. direct show) 2))
)

(type + Foo (is Interface)
  (fn-do + show Int
    (param n Int)
    (print (str "show " n))
    (let items (list 1 2 3))
    (return ([] items (+ n 1)))
  )
)

(type + Foobar (is Interface)
  (implements Foo)
)

(type + Thing
  (implements Foobar)
)
```

```output
show 1
3
show 2
```

### A function with an untyped argument that an interface declares with a body, called with three types through a value made from a derived interface.

```brix
(main
  (let thing (call Thing))
  (let pair (as thing Foobar))
  (let one (as pair Foo))
  (print (call (. one pick) 1))
  (print (call (. one pick) "a"))
  (print (call (. one pick) 2.5))
)

(type + Foo (is Interface)
  (fn-do + pick _
    (param value _)
    (print (str "pick " value))
    (return value)
  )
)

(type + Foobar (is Interface)
  (implements Foo)
)

(type + Thing
  (implements Foobar)
)
```

```output
pick 1
1
pick a
a
pick 2.5
2.5
```

### Getters and a setter an interface declares with bodies, used through a value of the interface and through one made from a derived interface.

```brix
(main
  (let thing (call Thing))
  (let pair (as thing Foobar))
  (let one (as pair Foo))
  (print (. one label))
  (= (. one label) "set")
  (print (. one count))
  (let direct (as thing Foo))
  (print (. direct label))
  (= (. direct label) "again")
)

(type + Foo (is Interface)
  (fn-do + label String :get
    (return (str "label of " this))
  )
  (fn-do + label _ :set (param value String)
    (print (str "label = " value))
  )
  (fn-do + count Int :get
    (let items (list 1 2 3))
    (return ([] items 2))
  )
)

(type + Foobar (is Interface)
  (implements Foo)
)

(type + Thing
  (implements Foobar)
)
```

```output
label of [Object __source__:Thing]
label = set
3
label of [Object __source__:Thing]
label = again
```

### A function that yields, declared by an interface with a body, read through a value made from a derived interface.

```brix
(main
  (let thing (call Thing))
  (let pair (as thing Foobar))
  (let one (as pair Foo))
  (for (in n (call (. one count-up) 3))
    (print n)
  )
)

(type + Foo (is Interface)
  (fn-do + count-up _
    (param limit Int)
    (for (in n (..< 0 limit))
      (yield n)
    )
  )
)

(type + Foobar (is Interface)
  (implements Foo)
)

(type + Thing
  (implements Foobar)
)
```

```output
0
1
2
```

### A function that answers an optional, declared by an interface with a body, called through a value made from a derived interface.

```brix
(main
  (let thing (call Thing))
  (let pair (as thing Foobar))
  (let one (as pair Foo))
  (print (?? (call (. one show)) "none"))
)

(type + Foo (is Interface)
  (fn-do + show (? String)
    (print "show")
    (return null)
  )
)

(type + Foobar (is Interface)
  (implements Foo)
)

(type + Thing
  (implements Foobar)
)
```

```output
show
none
```

### Values of a derived interface in a list, each seen as the base interface: one class takes the interface's function and the other declares its own.

```brix crash
(main
  (let things (list Foobar) (list (call Thing) (call Other)))
  (for (in thing things)
    (let one (as thing Foo))
    (call (. one show))
  )
)

(type + Foo (is Interface)
  (fn-do + show _
    (print (str "show " this))
    (call tell)
  )
  (fn-expr + tell _)
)

(type + Foobar (is Interface)
  (implements Foo)
)

(type + Thing
  (implements Foobar)
  (fn-do + tell _
    (print "thing tell")
  )
)

(type + Other
  (implements Foobar)
  (fn-do + show _
    (print "other show")
    (call tell)
  )
  (fn-do + tell _
    (print "other tell")
    (let items (list 1 2 3))
    (print ([] items 5))
  )
)
```

```output
show [Interface __source__:Foo]
thing tell
other show
other tell
```

### Functions of a value of an interface, one bound with an argument and one the interface declares with a body, are made values of a function type in a loop over two classes.

```brix
(main
  (let things (list Proto))
  (+= things (call Foo))
  (+= things (call Baz))
  (for (in thing things)
    (let held (call (. thing bar) 3 ...))
    (let fn (as held Taker))
    (print (call fn 4))
    (print (call run (. thing twice)))
  )
)

(type + Taker (is (fn-type Int (param a Int))))

(fn-expr + run _
  (param fn Taker)
  (+ (call fn 2) 1)
)

(type + Proto (is Interface)
  (fn-expr + bar Int
    (param a Int)
    (param b Int)
  )
  (fn-expr + twice Int
    (param a Int)
    (* (call bar a a) 2)
  )
)

(type + Foo
  (implements Proto)
  (prop + c _ (= 10))
  (fn-expr + bar _
    (param a _)
    (param b _)
    (+ a b c)
  )
)

(type + Baz
  (implements Proto)
  (fn-expr + bar _
    (param a _)
    (param b _)
    (* a b)
  )
)
```

```output
17
29
12
9
```

### A function of a value of an interface bound with an argument, and one the interface declares with a body, are cast to a function type that gives and takes other numbers.

```brix
(main
  (let foo (as (call Foo) Proto))
  (let fn (as (call (. foo bar) 1 ...) Wide))
  (print (call fn 2.9))
  (let tw (as (. foo twice) Wide))
  (print (call tw 1.5))
)

(type + Wide (is (fn-type Float (param a Float))))

(type + Proto (is Interface)
  (fn-expr + bar Int
    (param a Int)
    (param b Int)
  )
  (fn-expr + twice Int
    (param a Int)
    (* (call bar a a) 2)
  )
)

(type + Foo
  (implements Proto)
  (prop + c _ (= 10))
  (fn-expr + bar _
    (param a _)
    (param b _)
    (+ a b c)
  )
)

(type + Baz
  (implements Proto)
  (fn-expr + bar _
    (param a _)
    (param b _)
    (* a b)
  )
)
```

```output
13
24
```

### A function of a value of an interface, bound with an argument and called as a value of a function type, panics in the class: the stack trace has the class's function, the function of the table and the function made for the value.

```brix crash
(main
  (let foo (as (call Picker) Picks))
  (let fn (as (call (. foo pick) 1 ...) Taker))
  (print (call fn 1))
  (print (call fn 7))
)

(type + Taker (is (fn-type Int (param a Int))))

(type + Picks (is Interface)
  (fn-expr + pick Int
    (param a Int)
    (param b Int)
  )
)

(type + Picker
  (implements Picks)
  (prop + items _ (= (list 1 2 3)))
  (fn-expr + pick _
    (param a _)
    (param b _)
    ([] items (+ a b))
  )
)
```

```stderr
Unhandled panic: List index 8 out of range
<core/list.bx>

Task 1

Stack trace:
  0  core/list:List:absent core/list.bx
  1  __source__:Picker:pick no file:24:5
  2  __source__:pick(wrapped) no file:21:3
  3  __source__:pickCasting no file:4:11
  4  __source__:main no file:6:10
  5  __source__:init.__source__ no file
  6  __source__:brix.main no file
```

### A function an interface declares with a body, given as a function of a value where a function type is taken, calls one of the class that panics.

```brix crash
(main
  (let foo (as (call Picker) Picks))
  (print (call run (. foo pick)))
  (print (call run (. foo both)))
)

(type + Taker (is (fn-type Int (param a Int))))

(type + Picks (is Interface)
  (fn-expr + pick Int
    (param a Int)
  )
  (fn-expr + both Int
    (param a Int)
    (+ (call pick a) (call pick (+ a 1)))
  )
)

(type + Picker
  (implements Picks)
  (prop + items _ (= (list 1 2 3)))
  (fn-expr + pick _
    (param a _)
    ([] items a)
  )
)

(fn-expr + run _
  (param fn Taker)
  (+ (call fn 0) (call fn 2))
)
```

```stderr
Unhandled panic: List index 3 out of range
<core/list.bx>

Task 1

Stack trace:
  0  core/list:List:absent core/list.bx
  1  __source__:Picker:pick no file:25:5
  2  __source__:pick(wrapped) no file:23:3
  3  __source__:Picks:both no file:16:22
  4  __source__:bothCasting no file:5:20
  5  __source__:run no file:31:18
  6  __source__:main no file:5:10
  7  __source__:init.__source__ no file
  8  __source__:brix.main no file
```

### A property a class answers an interface through holds an object of a class derived from the property's: a function called through a value of the interface is the derived class's.

```brix
(main
  (let thing (call Thing))
  (let proto Proto thing)
  (call (. proto act))
)

(type + Proto (is Interface)
  (fn-expr + act _)
)

(type + FooBase
  (fn-do + act _
    (print "foobase act")
  )
)

(type + Foo (is FooBase)
  (fn-do + act _
    (print "foo act")
  )
)

(type + Thing
  (delegate Proto)
  (prop + foo FooBase (= (call Foo)))
)
```

```output
foo act
```

### A property a class answers an interface through is assigned an object of a derived class: functions called on the object and through a value are the derived class's.

```brix
(main
  (let thing (call Thing))
  (= (. thing foo) (call SubFoo))
  (call (. thing act))
  (let proto Proto thing)
  (call (. proto act))
  (print (call (. proto size) 2))
  (print (call (. thing size) 2))
)

(type + Proto (is Interface)
  (fn-expr + act _)
  (fn-expr + size Int
    (param n Int)
  )
)

(type + Foo
  (fn-do + act _
    (print "foo act")
  )
  (fn-expr + size Int
    (param n Int)
    n
  )
)

(type + SubFoo (is Foo)
  (fn-do + act _
    (print "subfoo act")
  )
  (fn-expr + size Int
    (param n Int)
    (* n 100)
  )
)

(type + Thing
  (delegate Proto)
  (prop + foo Foo)
)
```

```output
subfoo act
subfoo act
200
200
```

### A lazy property a class answers an interface through makes an object of a derived class, whose function a value of the interface calls.

```brix
(main
  (let thing (call Thing))
  (call (. thing act))
  (let proto Proto thing)
  (call (. proto act))
)

(type + Proto (is Interface)
  (fn-expr + act _)
)

(type + Foo
  (fn-do + act _
    (print "foo act")
  )
)

(type + SubFoo (is Foo)
  (fn-do + act _
    (print "subfoo act")
  )
)

(type + Thing
  (delegate Proto)
  (prop + foo Foo (:= (call SubFoo)))
)
```

```output
subfoo act
subfoo act
```

### A field of a tuple that answers an interface holds an object of a derived class, whose function a value of the interface calls.

```brix
(main
  (let thing (call Thing (call SubFoo)))
  (call (. thing act))
  (let proto Proto thing)
  (call (. proto act))
)

(type + Proto (is Interface)
  (fn-expr + act _)
)

(type + Foo
  (fn-do + act _
    (print "foo act")
  )
)

(type + SubFoo (is Foo)
  (fn-do + act _
    (print "subfoo act")
  )
)

(type + Thing (is Tuple)
  (delegate Proto)
  (prop + foo Foo)
)
```

```output
subfoo act
subfoo act
```

### A class answers an interface through a property whose class answers it through another, which holds an object of a derived class.

```brix
(main
  (let thing (call Thing))
  (call (. thing act))
  (let proto Proto thing)
  (call (. proto act))
)

(type + Proto (is Interface)
  (fn-expr + act _)
)

(type + Foo
  (implements Proto)
  (fn-do + act _
    (print "foo act")
  )
)

(type + SubFoo (is Foo)
  (fn-do + act _
    (print "subfoo act")
  )
)

(type + Bar
  (delegate Proto)
  (prop + foo Foo (= (call SubFoo)))
)

(type + Thing
  (delegate Proto)
  (prop + bar _ (= (call Bar)))
)
```

```output
subfoo act
subfoo act
```

### A function an interface declares with a body is declared again by a class derived from the class of the property that answers: a value of the interface calls the derived class's.

```brix
(main
  (let thing (call Thing))
  (= (. thing foo) (call SubFoo))
  (let proto Proto thing)
  (call (. proto rest))
  (call (. thing rest))
)

(type + Proto (is Interface)
  (fn-expr + act _)
  (fn-do + rest _
    (print "proto rest")
    (call act)
  )
)

(type + Foo
  (implements Proto)
  (fn-do + act _
    (print "foo act")
  )
)

(type + SubFoo (is Foo)
  (fn-do + act _
    (print "subfoo act")
  )
  (fn-do + rest _
    (print "subfoo rest")
  )
)

(type + Thing
  (delegate Proto)
  (prop + foo Foo)
)
```

```output
subfoo rest
subfoo rest
```

### A function of a value of an interface that a property answers is cast to a function type and bound: each calls the function of the derived class the property holds.

```brix
(main
  (let thing (call Thing))
  (= (. thing foo) (call SubFoo))
  (let proto Proto thing)
  (let f (as (. proto size) Wide))
  (print (call f 2.5))
  (let g (call (. proto size) ...))
  (print (call g 3))
)

(type + Wide (is (fn-type Float (param n Float))))

(type + Proto (is Interface)
  (fn-expr + size Int
    (param n Int)
  )
)

(type + Foo
  (fn-expr + size Int
    (param n Int)
    n
  )
)

(type + SubFoo (is Foo)
  (fn-expr + size Int
    (param n Int)
    (* n 100)
  )
)

(type + Thing
  (delegate Proto)
  (prop + foo Foo)
)
```

```output
200
300
```

### Functions with arguments and a getter answered through a property, before and after it is assigned an object of a derived class.

```brix
(main
  (let thing (call Thing))
  (let proto Proto thing)
  (print (call (. proto total) 1 2))
  (= (. thing foo) (call Sub))
  (print (call (. proto total) 1 2))
  (print (call (. thing total) 1 2))
  (print (. proto name))
  (print (. thing name))
)

(type + Proto (is Interface)
  (fn-expr + total Int
    (param a Int)
    (param b Int)
  )
  (fn-expr + name String :get)
)

(type + Foo
  (prop + name _ (= "foo"))
  (fn-expr + total Int
    (param a Int)
    (param b Int)
    (+ a b)
  )
)

(type + Sub (is Foo)
  (fn-do + This _
    (= name "sub")
  )
  (fn-expr + total Int
    (param a Int)
    (param b Int)
    (* a b 100)
  )
)

(type + Thing
  (delegate Proto)
  (prop + foo Foo)
)
```

```output
3
200
200
sub
sub
```

### A function of a derived class the property that answers holds panics, called through a value of the interface: the stack trace names it.

```brix crash
(main
  (let thing (call Thing))
  (let proto Proto thing)
  (print (call (. proto pick) 1))
  (= (. thing foo) (call SubFoo))
  (print (call (. proto pick) 1))
  (print (call (. proto pick) 7))
)

(type + Proto (is Interface)
  (fn-expr + pick Int
    (param n Int)
  )
)

(type + Foo
  (fn-do + pick Int
    (param n Int)
    (return n)
  )
)

(type + SubFoo (is Foo)
  (fn-do + pick Int
    (param n Int)
    (let items (list 10 20 30))
    (return ([] items n))
  )
)

(type + Thing
  (delegate Proto)
  (prop + foo Foo)
)
```

```stderr
Unhandled panic: List index 7 out of range
<core/list.bx>

Task 1

Stack trace:
  0  core/list:List:absent core/list.bx
  1  __source__:SubFoo:pick no file:28:13
  2  __source__:pick(wrapped) no file:18:3
  3  __source__:main no file:8:10
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### A property that answers an interface is of a class that answers it through its own property; each is assigned an object of a derived class in turn.

```brix
(main
  (let outer (call Outer))
  (let proto Proto outer)
  (call (. proto act))
  (= (. outer thing foo) (call SubFoo))
  (call (. proto act))
  (= (. outer thing) (call SubThing))
  (call (. proto act))
)

(type + Proto (is Interface)
  (fn-expr + act _)
)

(type + Foo
  (fn-do + act _
    (print "foo act")
  )
)

(type + SubFoo (is Foo)
  (fn-do + act _
    (print "subfoo act")
  )
)

(type + Thing
  (delegate Proto)
  (prop + foo Foo)
)

(type + SubThing (is Thing)
  (fn-do + act _
    (print "subthing act")
  )
)

(type + Outer
  (delegate Proto)
  (prop + thing Thing)
)
```

```output
foo act
subfoo act
foo act
```

### A function answered through a property returns an object, and the derived class the property holds returns one of a derived class.

```brix
(main
  (let thing (call Thing))
  (= (. thing foo) (call SubFoo))
  (let proto Proto thing)
  (let made (call (. proto make) 2))
  (print (. made label))
  (let list (list proto proto))
  (for (in item list)
    (print (. (call (. item make) 3) label))
  )
)

(type + Proto (is Interface)
  (fn-expr + make Named
    (param n Int)
  )
)

(type + Named
  (prop + label _ (= "named"))
)

(type + Special (is Named))

(type + Foo
  (fn-do + make Named
    (param n Int)
    (let named (call Named))
    (= (. named label) (str "foo " n))
    (return named)
  )
)

(type + SubFoo (is Foo)
  (fn-do + make Named
    (param n Int)
    (let special (call Special))
    (= (. special label) (str "subfoo " n))
    (return special)
  )
)

(type + Thing
  (delegate Proto)
  (prop + foo Foo)
)
```

```output
subfoo 2
subfoo 3
subfoo 3
```

### A list of values of an interface made of objects whose property holds the class or one derived from it, with functions that take arguments.

```brix
(main
  (let boxes (list Proto) (list (call Thing (call Foo)) (call Thing (call SubFoo)) (call Thing (call Foo))))
  (for (in box boxes)
    (call (. box act))
    (print (call (. box total) 2 3))
  )
)

(type + Proto (is Interface)
  (fn-expr + act _)
  (fn-expr + total Int
    (param a Int)
    (param b Int)
  )
)

(type + Foo
  (fn-do + act _
    (print "foo act")
  )
  (fn-expr + total Int
    (param a Int)
    (param b Int)
    (+ a b)
  )
)

(type + SubFoo (is Foo)
  (fn-do + act _
    (print "subfoo act")
  )
  (fn-expr + total Int
    (param a Int)
    (param b Int)
    (* a b)
  )
)

(type + Thing
  (delegate Proto)
  (prop + foo Foo)
  (fn-do + This _
    (param foo _)
  )
)
```

```output
foo act
5
subfoo act
6
foo act
5
```

### A function the class of the property that answers inherits from its base class, called on the object that delegates.

```brix
(main
  (let thing (call Thing))
  (call (. thing act))
)

(type + Proto (is Interface)
  (fn-expr + act _)
)

(type + FooBase
  (fn-do + act _
    (print (str "foobase act of " this))
  )
)

(type + Foo (is FooBase))

(type + Thing
  (delegate Proto)
  (prop + foo Foo)
)
```

```output
foobase act of [Object __source__:FooBase]
```

### A stored property and a getter the class of the property that answers inherits from its base class, read on the object and through a value.

```brix
(main
  (let thing (call Thing))
  (print (. thing size))
  (print (. thing name))
  (let proto Proto thing)
  (print (. proto size))
  (print (. proto name))
)

(type + Proto (is Interface)
  (fn-expr + size Int :get)
  (fn-expr + name String :get)
)

(type + FooBase
  (prop + size _ (= 4))
  (fn-expr + name String :get
    "base name"
  )
)

(type + Foo (is FooBase))

(type + Thing
  (delegate Proto)
  (prop + foo Foo)
)
```

```output
4
base name
4
base name
```

### The base class of the property that answers declares the interface and the function, called on the object and through a value.

```brix
(main
  (let thing (call Thing))
  (call (. thing act))
  (let proto Proto thing)
  (call (. proto act))
)

(type + Proto (is Interface)
  (fn-expr + act _)
)

(type + FooBase
  (implements Proto)
  (fn-do + act _
    (print (str "foobase act of " this))
  )
)

(type + Foo (is FooBase))

(type + Thing
  (delegate Proto)
  (prop + foo Foo)
)
```

```output
foobase act of [Object __source__:FooBase]
foobase act of [Object __source__:FooBase]
```

### The class of the property that answers inherits one function of the interface and declares another, on the object and through a value.

```brix
(main
  (let thing (call Thing))
  (call (. thing act))
  (call (. thing more))
  (let proto Proto thing)
  (call (. proto act))
  (call (. proto more))
)

(type + Proto (is Interface)
  (fn-expr + act _)
  (fn-expr + more _)
)

(type + FooBase
  (fn-do + act _
    (print (str "foobase act of " this))
  )
  (fn-do + more _
    (print "foobase more")
  )
)

(type + Foo (is FooBase)
  (fn-do + more _
    (print "foo more")
  )
)

(type + Thing
  (delegate Proto)
  (prop + foo Foo)
)
```

```output
foobase act of [Object __source__:FooBase]
foo more
foobase act of [Object __source__:FooBase]
foo more
```

### A function declared two classes above the class of the property that answers, which is assigned an object of a derived class and then one of its own.

```brix
(main
  (let thing (call Thing))
  (= (. thing foo) (call Low))
  (let proto Proto thing)
  (call (. proto act))
  (call (. thing act))
  (= (. thing foo) (call Mid))
  (call (. proto act))
  (call (. thing act))
)

(type + Proto (is Interface)
  (fn-expr + act _)
)

(type + Top
  (fn-do + act _
    (print "top act")
  )
)

(type + Mid (is Top))

(type + Low (is Mid)
  (fn-do + act _
    (print "low act")
  )
)

(type + Thing
  (delegate Proto)
  (prop + foo Mid)
)
```

```output
low act
low act
top act
top act
```

### Delegated function takes a number of another type than the interface gives

```brix
(main
  (let sink (call Sink))
  (<- sink 1)
  (let writable (as sink (of Writable I32)))
  (<- writable 2)
)

(type + Sink
  (delegate (of Writable I32))
  (prop _ channel _ (= (call (of Channel Float))))
)

(type + (of Writable Message) (is Interface)
  (fn-expr + (infix this <- (param item Message)) _)
)

(type + (of Channel Message)
  (implements (of Writable Message))
  (fn-do + (infix this <- (param item Message)) _
    (print item)
  )
)
```

```output
1
2
```

### Writes chained through a delegate give the receiver written

```brix
(main
  (let sink (call Sink))
  (<- sink "a" "b")
  (let writable (as sink (of Writable String)))
  (<- writable "c" "d")
)

(type + Wrapped (is (tuple (: text String)))
  (fn-from-expr + (: other String)
    (call Wrapped other)
  )
  (fn-to-expr + String
    (str "<" text ">")
  )
)

(type + Sink
  (delegate (of Writable String))
  (prop _ channel _ (= (call (of Channel Wrapped))))
)

(type + (of Writable Message) (is Interface)
  (fn-expr + (infix this <- (param item Message)) _)
)

(type + (of Channel Message)
  (implements (of Writable Message))
  (fn-do + (infix this <- (param item Message)) _
    (print item)
  )
)
```

```output
<a>
<b>
<c>
<d>
```

### Function that returns an optional of a class answers an optional of an interface

```brix
(main
  (let child (call Child))
  (let holder Holder child)
  (print (== (call (. holder find) 1) null))
  (let found (call (. holder find) 2))
  (if found
    (print (. found tag))
  )
  (print (call describe holder 2))
  (print (call describe holder 1))
)

(type + Owner (is Interface)
  (fn-expr + tag String :get)
)

(type + Holder (is Interface)
  (fn-expr + find (? Owner)
    (param key Int)
  )
)

(type + Parent
  (implements Owner)
  (fn-expr + tag _ :get
    "parent"
  )
)

(type + Child
  (implements Holder)
  (fn-expr + find (? Parent)
    (param key Int)
    (case? (== key 2)
      (call Parent)
    )
    (default
      null
    )
  )
)

(fn-do + describe String
  (param holder Holder)
  (param key Int)
  (let found (call (. holder find) key))
  (if found
    (return (. found tag))
  )
  (return "none")
)
```

```output
true
parent
parent
none
```

### Optional of a tuple made an optional of an interface

```brix
(main
  (let point (? Point) (call Point 1 2))
  (let shown (? Shown) point)
  (if shown
    (print (. shown text))
  )
  (let none (? Point) null)
  (let empty (? Shown) none)
  (print (== empty null))
  (let list (list (? Shown)) (list point none))
  (for (in item list)
    (if item
      (print (. item text))
      (else
        (print "nothing")
      )
    )
  )
  (print (call first point))
  (print (call first none))
)

(type + Shown (is Interface)
  (fn-expr + text String :get)
)

(type + Point (is Tuple)
  (implements Shown)
  (prop + x Int)
  (prop + y Int)
  (fn-expr + text _ :get
    (str x "," y)
  )
)

(fn-do + first String
  (param shown (? Shown))
  (return (if-expr shown (. shown text) "none"))
)
```

```output
1,2
true
1,2
nothing
1,2
none
```

### Optional of a class made an optional of an interface

```brix
(main
  (let parent (? Parent) (call Parent))
  (let owner (? Owner) parent)
  (print (. (!! owner) tag))
  (let none (? Parent) null)
  (let empty (? Owner) none)
  (print (== empty null))
  (print (call show parent))
  (print (call show none))
)

(type + Owner (is Interface)
  (fn-expr + tag String :get)
)

(type + Parent
  (implements Owner)
  (fn-expr + tag _ :get
    "parent"
  )
)

(fn-do + show String
  (param owner (? Owner))
  (if owner
    (return (. owner tag))
  )
  (return "none")
)
```

```output
parent
true
parent
none
```

### Stack trace through a delegated function of an interface

```brix crash
(main
  (let sink (call Sink))
  (<- sink "hello")
  (let writable (as sink (of Writable String)))
  (<- writable "world")
  (<- writable "")
  (print "never")
)

(type + Sink
  (delegate (of Writable String))
  (prop _ channel _ (= (call (of Channel String))))
)

(type + (of Writable Message) (is Interface)
  (fn-expr + (infix this <- (param item Message)) _)
)

(type + (of Channel Message)
  (implements (of Writable Message))
  (fn-do + (infix this <- (param item Message)) _
    (throw (call Panic "empty item") (if (== (str item) "")))
    (print item)
  )
)
```

```stderr
Unhandled panic: empty item
<no file:23>

Task 1

Stack trace:
  0  __source__:Channel:<-write no file:23:5
  1  __source__:<-write(wrapped) no file:22:3
  2  __source__:main no file:7:3
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### Stack trace through a delegated function that converts its argument

```brix crash
(main
  (let sink (call Sink))
  (<- sink "hello")
  (let writable (as sink (of Writable String)))
  (<- writable "world")
  (<- writable "")
  (print "never")
)

(type + Wrapped (is (tuple (: text String)))
  (fn-from-expr + (: other String)
    (call Wrapped other)
  )
  (fn-to-expr + String
    text
  )
)

(type + Sink
  (delegate (of Writable String))
  (prop _ channel _ (= (call (of Channel Wrapped))))
)

(type + (of Writable Message) (is Interface)
  (fn-expr + (infix this <- (param item Message)) _)
)

(type + (of Channel Message)
  (implements (of Writable Message))
  (fn-do + (infix this <- (param item Message)) _
    (throw (call Panic "empty item") (if (== (str item) "")))
    (print item)
  )
)
```

```stderr
Unhandled panic: empty item
<no file:32>

Task 1

Stack trace:
  0  __source__:Channel:<-write no file:32:5
  1  __source__:<-write(wrapped) no file:26:3
  2  __source__:main no file:7:3
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### Delegate that a getter answers is read at each call

```brix
(main
  (let thing (call Thing))
  (print (call (. thing act) 1))
  (print (call (. thing act) 2))
  (let proto (as thing Proto))
  (print (call (. proto act) 3))
  (print (call (. proto act) 4))
)

(type + Thing
  (delegate Proto)
  (fn-do + foo Foo :get
    (print "made foo")
    (return (call Foo))
  )
)

(type + Proto (is Interface)
  (fn-expr + act String
    (param x Int)
  )
)

(type + Foo
  (prop + label String)
  (fn-do + This _
    (param given String (= "foo"))
    (lazy label given)
  )
  (fn-do + act String
    (param x Int)
    (return (str label " " x))
  )
)
```

```output
made foo
foo 1
made foo
foo 2
made foo
foo 3
made foo
foo 4
```

### Stack trace of a panic in a getter that answers a delegate

```brix crash
(main
  (let thing (call Thing))
  (print (call (. thing act) 1))
  (let proto (as thing Proto))
  (print (call (. proto act) 2))
  (= (. thing broken) true)
  (print (call (. proto act) 3))
  (print "never")
)

(type + Thing
  (prop + broken _ (= false))
  (blank)
  (delegate Proto)
  (fn-do + foo Foo :get
    (throw (call Panic "no foo") (if broken))
    (return (call Foo))
  )
)

(type + Proto (is Interface)
  (fn-expr + act String
    (param x Int)
  )
)

(type + Foo
  (prop + label _ (= "foo"))
  (fn-do + act String
    (param x Int)
    (return (str label " " x))
  )
)
```

```stderr
Unhandled panic: no foo
<no file:17>

Task 1

Stack trace:
  0  __source__:Thing:foo no file:17:5
  1  __source__:act(wrapped) no file:30:3
  2  __source__:main no file:8:10
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### Delegate that a getter answers is the derived class's

```brix
(main
  (let thing Thing (call Loud))
  (print (call (. thing act) 1))
  (let proto (as thing Proto))
  (print (call (. proto act) 2))
  (let plain (call Thing))
  (print (call (. plain act) 3))
  (let other (as plain Proto))
  (print (call (. other act) 4))
)

(type + Thing
  (delegate Proto)
  (fn-expr + foo Foo :get
    (call Foo)
  )
)

(type + Loud (is Thing)
  (fn-expr + foo _ :get
    (call Foo "loud")
  )
)

(type + Proto (is Interface)
  (fn-expr + act String
    (param x Int)
  )
)

(type + Foo
  (prop + label String)
  (fn-do + This _
    (param given String (= "foo"))
    (lazy label given)
  )
  (fn-do + act String
    (param x Int)
    (return (str label " " x))
  )
)
```

```output
loud 1
loud 2
foo 3
foo 4
```

### Delegate of a delegate that a getter answers

```brix
(main
  (let outer (call Outer))
  (print (call (. outer act) 1))
  (let proto (as outer Proto))
  (print (call (. proto act) 2))
)

(type + Outer
  (delegate Proto)
  (fn-do + inner Inner :get
    (print "made inner")
    (return (call Inner))
  )
)

(type + Inner
  (delegate Proto)
  (prop + foo _ (= (call Foo "inner")))
)

(type + Proto (is Interface)
  (fn-expr + act String
    (param x Int)
  )
)

(type + Foo
  (prop + label String)
  (fn-do + This _
    (param given String (= "foo"))
    (lazy label given)
  )
  (fn-do + act String
    (param x Int)
    (return (str label " " x))
  )
)
```

```output
made inner
inner 1
made inner
inner 2
```

### Tuple that delegates to a field with no value written

```brix
(main
  (let thing (call Thing))
  (print (call (. thing foo act) 1))
  (let other (call Thing (call Foo "given")))
  (print (call (. other act) 2))
  (print (call (. thing act) 3))
  (print (. thing count))
  (let binding (call (. other act) ...))
  (print (call binding 4))
  (let proto (as other Proto))
  (let second (call (. proto act) ...))
  (print (call second 5))
)

(type + Thing (is Tuple)
  (delegate Proto)
  (prop + foo Foo)
  (prop + count Int (= 7))
)

(type + Proto (is Interface)
  (fn-expr + act String
    (param x Int)
  )
)

(type + Foo
  (prop + label String)
  (fn-do + This _
    (param given String (= "foo"))
    (lazy label given)
  )
  (fn-do + act String
    (param x Int)
    (return (str label " " x))
  )
)
```

```output
foo 1
given 2
foo 3
7
given 4
given 5
```

### Binding of a function a delegate answers reads the delegate when called

```brix
(main
  (let thing (call Thing))
  (let binding (call (. thing act) ...))
  (print (call binding 1))
  (= (. thing foo label) "changed")
  (print (call binding 2))
  (print (call (. thing act) 3))
)

(type + Thing
  (delegate Proto)
  (prop + foo _ (= (call Foo)))
)

(type + Proto (is Interface)
  (fn-expr + act String
    (param x Int)
  )
)

(type + Foo
  (prop + label String)
  (fn-do + This _
    (param given String (= "foo"))
    (lazy label given)
  )
  (fn-do + act String
    (param x Int)
    (return (str label " " x))
  )
)
```

```output
foo 1
changed 2
changed 3
```

### == of two values of an interface compares a class by identity, and != negates it.

```brix
(main
  (let thing (call Thing))
  (let a (as thing Proto))
  (let b (as (call Thing) Proto))
  (print (== a a))
  (print (== a b))
  (print (!= a b))
)

(type + Proto (is Interface))

(type + Thing
  (implements Proto)
)
```

```output
true
false
true
```

### A panic in a type's own == passes through the equality function a witness table holds, which a trace names with no file.

```brix crash
(main
  (let m (as (call Measured 7) View))
  (let n (as (call Measured 7) View))
  (print (== m n))
  (call panicky m n)
)

(type + View (is Interface))

(type + Measured (is (tuple (: value I32)))
  (implements View)
  (fn-do + (infix this == (param other Measured)) _
    (if (== value 7)
      (throw (call Panic "boom"))
    )
    (return (== value (. other value)))
  )
)

(fn-do + panicky _
  (param left View)
  (param right View)
  (print (== left right))
)
```

```stderr
Unhandled panic: boom
<no file:15>

Task 1

Stack trace:
  0  __source__:Measured:== no file:15:7
  1  __source__:InterfaceEquality:Class:__source__:Measured:Class:__source__:View no file
  2  __source__:main no file:5:10
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### == of two values of an interface whose types differ panics where nothing compares for change.

```brix crash
(main
  (let a (as (call (of Packet I32) 1 2) View))
  (let b (as (call (of Packet I64) 1i64 2i64) View))
  (print "before")
  (call check a b)
)

(type + View (is Interface))

(type + (of Packet T) (is (tuple (: first T) (: second T)))
  (implements View)
)

(fn-do + check _
  (param left View)
  (param right View)
  (print (== left right))
)
```

```stderr
Unhandled panic: Unsupported interface equality: concrete types differ
<no file:18>

Task 1

Stack trace:
  0  __source__:check no file:18:10
  1  __source__:main no file:6:3
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

### Optionals of an interface are equal when both are empty or both hold equal values.

```brix
(main
  (let a (? View) (as (call Packet 1 2) View))
  (let b (? View) null)
  (let c (? View) (as (call Packet 1 2) View))
  (print (== a b))
  (print (== a c))
  (print (== b b))
  (print (!= a c))
  (let d (as (call Packet 1 2) View))
  (print (== d a))
  (print (== a d))
)

(type + View (is Interface))

(type + Packet (is (tuple (: first I32) (: second I32)))
  (implements View)
)
```

```output
false
true
true
false
true
true
```

### The equality function of a generic tuple names its type's parameters.

```brix crash
(main
  (let a (as (call (of Packet I32) 7 1) View))
  (let b (as (call (of Packet I32) 7 1) View))
  (print (== a b))
)

(type + View (is Interface))

(type + (of Packet T) (is (tuple (: first T) (: second T)))
  (implements View)
  (fn-do + (infix this == (param other (of Packet T))) _
    (throw (call Panic "generic boom"))
  )
)
```

```stderr
Unhandled panic: generic boom
<no file:13>

Task 1

Stack trace:
  0  __source__:Packet:== no file:13:5
  1  __source__:InterfaceEquality:Class:__source__:Packet(Class:core/number:I32):Class:__source__:View no file
  2  __source__:main no file:5:10
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### A field of an interface is compared through its own equality function.

```brix crash
(main
  (let inner (as (call Measured 7) View))
  (let a (as (call Nested inner 1) View))
  (let b (as (call Nested (as (call Measured 7) View) 1) View))
  (print (== a b))
)

(type + View (is Interface))

(type + Measured (is (tuple (: value I32)))
  (implements View)
  (fn-do + (infix this == (param other Measured)) _
    (throw (call Panic "inner boom"))
  )
)

(type + Nested (is (tuple (: inner View) (: marker I32)))
  (implements View)
)
```

```stderr
Unhandled panic: inner boom
<no file:14>

Task 1

Stack trace:
  0  __source__:Measured:== no file:14:5
  1  __source__:InterfaceEquality:Class:__source__:Measured:Class:__source__:View no file
  2  __source__:InterfaceEquality:Class:__source__:Nested:Class:__source__:View no file
  3  __source__:main no file:6:10
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### A class compared with a value of an interface is unequal; the other way around it panics.

```brix crash
(main
  (let a (as (call Thing) View))
  (let t (call Thing))
  (print (== t a))
  (print (== a t))
)

(type + View (is Interface))

(type + Thing
  (implements View)
)
```

```stderr
Unhandled panic: Unsupported interface equality with Thing
<no file:6>

Task 1

Stack trace:
  0  __source__:main no file:6:10
  1  __source__:init.__source__ no file
  2  __source__:brix.main no file
```

### A number class's own == decides equality through an interface.

```brix
(main
  (let a (as (call Counter 1) View))
  (let b (as (call Counter 2) View))
  (print (== a b))
  (print (!= a b))
  (print (== a (as (call Counter 1) View)))
)

(type + View (is Interface))

(type + Counter (is I32)
  (implements View)
  (fn-do + (infix this == (param other Counter)) _
    (print "compare")
    (return (== (as this I32) (as other I32)))
  )
)
```

```output
compare
false
compare
true
compare
true
```

### A tuple holding a value of an interface compares that field by its type's equality.

```brix
(main
  (let a (as (call Pair 1 (as (call Label "x") View)) View))
  (let b (as (call Pair 1 (as (call Label "x") View)) View))
  (let c (as (call Pair 1 (as (call Label "y") View)) View))
  (print (== a b))
  (print (== a c))
)

(type + View (is Interface))

(type + Label (is (tuple (: text String)))
  (implements View)
)

(type + Pair (is (tuple (: count I32) (: inner View)))
  (implements View)
)
```

```output
true
false
```

### An interface compared with another type as the right side of a short circuit panics where it is compared.

```brix crash
(main
  (let a (as (call Leaf 1) View))
  (let n 2)
  (let r (or (< n 1) (== a n)))
  (print r)
)

(type + View (is Interface))

(type + Leaf (is (tuple (: value I32)))
  (implements View)
)
```

```stderr
Unhandled panic: Unsupported interface equality with I64
<no file:5>

Task 1

Stack trace:
  0  __source__:main no file:5:22
  1  __source__:init.__source__ no file
  2  __source__:brix.main no file
```

### Values of an interface whose type is a function type are equal when they are one object.

```brix
(main
  (let g (call nums))
  (let a (as g (of Iterable Int)))
  (let b (as g (of Iterable Int)))
  (print (== a b))
  (print (== a (as (call nums) (of Iterable Int))))
  (print (== (as (list 1 2) (of Iterable Int)) (as (list 1 2) (of Iterable Int))))
)

(fn-do + nums _
  (yield 1)
  (yield 2)
)
```

```output
true
false
false
```

### A panic inside an equality function is headed by the line of the comparison.

```brix crash
(main
  (let a (as (call Nested (as (call Leaf 1) View) 1) View))
  (let b (as (call Nested (as (call Thing) View) 1) View))
  (print "go")
  (print (== a b))
)

(type + View (is Interface))

(type + Leaf (is (tuple (: value I32)))
  (implements View)
)

(type + Thing
  (implements View)
)

(type + Nested (is (tuple (: inner View) (: marker I32)))
  (implements View)
)
```

```stderr
Unhandled panic: Unsupported interface equality: concrete types differ
<no file:6>

Task 1

Stack trace:
  0  __source__:InterfaceEquality:Class:__source__:Nested:Class:__source__:View no file
  1  __source__:main no file:6:10
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

### A closure is named Closure by type-of and by the panic of an interface compared with it.

```brix crash
(main
  (let a (as (call Leaf 1) View))
  (let g (=> ((: n Int)) (+ n 1)))
  (print (str (type-of g)))
  (print (== a g))
)

(type + View (is Interface))

(type + Leaf (is (tuple (: value I32)))
  (implements View)
)
```

```stderr
Unhandled panic: Unsupported interface equality with Closure
<no file:6>

Task 1

Stack trace:
  0  __source__:main no file:6:10
  1  __source__:init.__source__ no file
  2  __source__:brix.main no file
```

### Both operands of == on interface values stay alive while a user == allocates and collects.

```brix
(import /BxGC/BxGC.h)
(blank)

(main
  (let left (as (call Pair (call GCNode 17) (call GCNode 29)) View))
  (let same (as (call Pair (call GCNode 17) (call GCNode 29)) View))
  (let different (as (call Pair (call GCNode 17) (call GCNode 30)) View))
  (print (== left same))
  (print (!= left same))
  (print (== left different))
  (print (!= left different))
)

(type + View (is Interface))

(type + Pair (is (tuple (: left GCNode) (: right GCNode)))
  (implements View)
  (fn-do + (infix this == (param other Pair)) _
    (let scratch (list GCNode))
    (for (in index (..< 0 32))
      (+= scratch (call GCNode index))
    )
    (call C:brixCollect)
    (let valid (and (call C:brixGCVerifyObject (as left Pointer)) (call C:brixGCVerifyObject (as right Pointer))))
    (= valid (and valid (call C:brixGCVerifyObject (as (. other left) Pointer))))
    (= valid (and valid (call C:brixGCVerifyObject (as (. other right) Pointer))))
    (return (and valid (== (. left value) (. other left value)) (== (. right value) (. other right value))))
  )
)

(type + GCNode
  (prop + value _ (= 0))
  (fn-do + This _
    (param v Int)
    (= value v)
  )
)
```

```output
true
false
false
true
```

### An interface's function written with a type parameter of its own is checked only for the calls that name it.

```brix
(main
  (let box (call Tally))
  (call send box)
  (print (. box total))
)

(fn-do + send _
  (param sink Sink)
  (<- sink 2.5)
)

(type + Sink (is Interface)
  (fn-do + (infix this <- (param count Int)) _
    (call put count)
  )
  (fn-do + (infix this <- (param value ''Value)) _
    (call put 1)
  )
  (fn-expr + put _ (?! Error)
    (param count Size)
  )
)

(type + Tally
  (implements Sink)
  (prop + total Size (= 0))
  (fn-do + put _
    (param count Size)
    (+= total count)
  )
)
```

```output
1
```

### A function that answers an interface's with feeds gives a generator, which a call through the interface collects into the list the interface's function answers.

```brix
(main
  (let maker (of Shows Record) (call Maker))
  (for (in part (call (. maker parts) (call Record "box")))
    (match part
      (case (call .text text)
        (print (str "text " text))
      )
      (case (call .count count)
        (print (str "count " count))
      )
    )
  )
)

(type + Part (is Union)
  (variant text String)
  (variant count I32)
)

(type + Record (is Tuple)
  (prop + name String)
)

(type + (of Shows Item) (is Interface)
  (fn-do + parts (list Part)
    (param item Item)
  )
)

(type + Maker (is Tuple)
  (implements (of Shows Record))
  (fn-expr + parts _
    (param record Record)
    (item (call (. Part text) "Add "))
    (item (call (. Part text) (. record name)))
    (item (call (. Part count) 2))
  )
)
```

```output
text Add 
text box
count 2
```

### What a type's function answers is made the case of the union the interface's function answers, for a call through the interface.

```brix
(main
  (let maker Shows (call Maker "box"))
  (match (call (. maker value))
    (case (call .text text)
      (print (str "text " text))
    )
    (case (call .names names)
      (print (str "names " (. names length) " " ([] names 0)))
    )
    (case (call .count count)
      (print (str "count " count))
    )
  )
)

(type + Value (is Union)
  (variant text String)
  (variant names (list String))
  (variant count I32)
)

(type + Shows (is Interface)
  (fn-do + value Value)
)

(type + Maker (is Tuple)
  (implements Shows)
  (prop + name String)
  (fn-expr + value _
    (list name "two")
  )
)
```

```output
names 2 box
```

### A type's function that raises the errors the interface's function raises answers a value of another type, converted for a call through the interface, and its error passes through as it is.

```brix
(main
  (for (in name (list "box" ""))
    (let maker Shows (call Maker name))
    (match (?? (catch? (call (. maker value))) (call (. Value text) "failed"))
      (case (call .text text)
        (print (str "text " text))
      )
      (case (call .names names)
        (print (str "names " (. names length) " " ([] names 0)))
      )
    )
  )
)

(type + Value (is Union)
  (variant text String)
  (variant names (list String))
)

(type + Shows (is Interface)
  (fn-do + value Value (?! Error))
)

(type + Maker (is Tuple)
  (implements Shows)
  (prop + name String)
  (fn-expr + value _
    (list (call checked name) "two")
  )
)

(fn-do + checked String (?! Error)
  (param name String)
  (if (== (. name length) 0)
    (throw (call Error "empty"))
  )
  (return name)
)
```

```output
names 2 box
text failed
```

### Two values of an interface are equal where they hold one type whose fields are, an optional field among them compared by the `==` its type declares.

```brix
(main
  (let one Shows (call Stored (call Record (call Spot 1 2) 3)))
  (let two Shows (call Stored (call Record (call Spot 1 2) 3)))
  (let three Shows (call Stored (call Record null 3)))
  (print (== one two))
  (print (== one three))
)

(type + Spot (is Tuple)
  (prop + x I32)
  (prop + y I32)
  (fn-expr + (infix this == (param other Spot)) _
    (and (== x (. other x)) (== y (. other y)))
  )
)

(type + Record (is Tuple)
  (prop + pivot (? Spot))
  (prop + size I32)
)

(type + Shows (is Interface)
  (fn-expr + size I32 :get)
)

(type + Stored (is Tuple)
  (implements Shows)
  (prop + record Record)
  (fn-expr + size I32 :get
    (. record size)
  )
)
```

```output
true
false
```

### A method that takes more arguments than the interface's, each with a default, implements it, and its defaults are evaluated at each call.

```brix
(main
  (let w (call Worker))
  (call run w)
  (call run w)
)

(fn-do + run _
  (param d Doer)
  (print (str "got " (call (. d act) 3)))
  (print (str "pure " (call (. d pure) 4)))
)

(type + Doer (is Interface)
  (fn-do + act Int
    (param n Int)
  )
  (fn-expr + pure Int
    (param n Int)
  )
)

(type + Worker
  (implements Doer)
  (prop + count _ (= 0))
  (fn-do + act Int
    (param n Int)
    (param scale _ (= 10))
    (param extra _ (= (call next)))
    (param flag _ (= false))
    (+= count 1)
    (print (str "act n=" n " scale=" scale " extra=" extra " flag=" flag " count=" count))
    (return (+ (* n scale) extra))
  )
  (fn-expr + pure Int
    (param n Int)
    (param plus (? Int) (= null))
    (+ n (?? plus 100))
  )
)

(fn-do + next Int
  (print "next")
  (return 7)
)
```

```output
next
act n=3 scale=10 extra=7 flag=false count=1
got 37
pure 104
next
act n=3 scale=10 extra=7 flag=false count=2
got 37
pure 104
```

### A loop over a class that delegates two iterables reads the last, converting its items.

```brix
(main
  (let inbox (call Inbox))
  (call (. inbox feed))
  (for (in message inbox)
    (print message)
  )
  (print "done")
)

(type + Inbox
  (delegate (of Iterable Int))
  (prop _ counts (channel Int))
  (delegate (of Iterable Float))
  (prop _ notes (channel Note))
  (fn-do + feed _
    (do-new
      (<- notes (call Note 1))
      (<- notes (call Note 2))
      (call (. notes close))
    )
  )
)

(type + Note (is Tuple)
  (prop + value Int)
  (fn-to-expr + Float
    (* value 1.5)
  )
)
```

```output
1.5
3
done
```

### A class that leaves out a getter its interface declares without a body and with a result does not compile.

```brix crash
(main
  (let source Source (call Lines))
  (print (. source revision))
)

(type + Source (is Interface)
  (fn-expr + revision U64 :get)
)

(type + Lines
  (implements Source)
)
```

```output
Exception: Lines does not implement "revision", which Source declares without a body; declare "revision" in Lines
<no file:11>
    (type + Lines
    ^^^^^^^^^^^^
Interface:
<no file:8>
      (fn-expr + revision U64 :get)
```

### A class that leaves out a function its interface declares without a body and with a result does not compile once a value of it is made a value of the interface, though nothing calls the function.

```brix compile-error
(main
  (let shape Shape (call Blob))
  (print "made")
)

(type + Shape (is Interface)
  (fn-expr + area Float)
)

(type + Blob
  (implements Shape)
)
```

```output
Exception: Blob does not implement "area", which Shape declares without a body; declare "area" in Blob
<no file:11>
    (type + Blob
    ^^^^^^^^^^^
Interface:
<no file:8>
      (fn-expr + area Float)
```

### A class may leave a function its interface requires to the classes derived from it, as long as no value known by the class alone is made a value of the interface.

```brix
(main
  (let shape Shape (call Square))
  (print (call (. shape area)))
)

(type + Shape (is Interface)
  (fn-expr + area Float)
)

(type + Base
  (implements Shape)
)

(type + Square (is Base)
  (fn-expr + area Float
    4.0
  )
)
```

```output
4
```

### An interface stores nothing, so a property it declares is an error that names the getter to declare instead, one that each type that is the interface answers.

```brix compile-error
(main
  (let shape Shape (call Square))
  (print (. shape name))
)

(type + Shape (is Interface)
  (prop + name String)
)

(type + Square
  (implements Shape)
  (prop + name _ (= "square"))
)
```

```output
Exception: Shape is an interface, so it cannot store "name"; declare a getter for the types that are Shape to answer, as in "+ get name String"
<no file:8>
      (prop + name String)
      ^^^^^^^^^^^^^^^^^^^^
```

### An interface stores nothing, so a property it declares with a value is an error that names the getter to declare, which gives the value to each type that is the interface and has no other.

```brix compile-error
(main
  (let shape Shape (call Square))
  (print (. shape name))
)

(type + Shape (is Interface)
  (prop + name _ (= "shape"))
)

(type + Square
  (implements Shape)
)
```

```output
Exception: Shape is an interface, so it cannot store "name"; declare a getter that gives the value, as in "(fn-expr + name _ :get …)"
<no file:8>
      (prop + name _ (= "shape"))
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### An interface cannot hold a state machine, which keeps its cases.

```brix compile-error
(main
  (let t (call Door))
  (print (. t phase))
)

(type + Gate (is Interface)
  (prop + phase _
    (machine
      (state closed)
    )
  )
)

(type + Door
  (implements Gate)
)
```

```output
Exception: Gate is an interface, so it cannot hold a state machine; declare the machine in the classes that are Gate
<no file:10>
          (state closed)
          ^^^^^^^^^^^^^^
```

```brix
(main
  (let sub (call SubHolder (call Named "sub")))
  (print (call (. sub target name)))
  (let box (call HolderBox (call Named "box")))
  (print (call (. box held target name)))
)

(type + Naming (is Interface)
  (fn-expr + name String)
)

(type + Named
  (implements Naming)
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
  (fn-expr + name _
    text
  )
)

(type + Holder
  (prop + target Naming)
  (fn-do + This _
    (param target _)
  )
)

(type + SubHolder (is Holder)
  (fn-do + This _
    (param named Named)
    (call super named)
  )
)

(type + HolderBox
  (prop + held Holder)
  (fn-do + This _
    (param named Named)
    (= held (call Holder named))
  )
)
```

```output
sub
box
```

### An interface a class declares must name a type

```brix compile-error
(main
  (let thing (call Thing))
  (print (. thing name))
)

(type + Thing
  (implements NoSuchInterface)
  (prop + name _ (= "thing"))
)
```

```output
Exception: Type "NoSuchInterface" not found
<no file:8>
      (implements NoSuchInterface)
                  ^^^^^^^^^^^^^^^
```

### A class matched against an interface no class implements compiles, and the arm is never taken.

```brix
(main
  (let node (call Node))
  (let found (call (of (. node find) Opener)))
  (print (if-expr found "found" "none"))
  (print (is node Opener))
)

(type + Opener (is Interface)
  (fn-do + open _)
)

(type + Node
  (fn-do + (of find Target) _
    (let items (list Node) (list this))
    (for (in item items)
      (match? item
        (case (: target Target)
          (return target)
        )
      )
    )
  )
)
```

```output
none
false
```

### A stored property answers an interface getter that returns an optional of the property's type, and a class that leaves the getter to its default answers null.

```brix
(main
  (let plain Shape (call Plain))
  (let owned Shape (call Owned))
  (print (== (. plain revision) null))
  (print (. (!! (. owned revision)) value))
)

(type + Rev
  (prop + value Int (= 7))
)

(type + Shape (is Interface)
  (fn-expr + revision (? Rev) :get
    null
  )
)

(type + Plain
  (implements Shape)
)

(type + Owned
  (implements Shape)
  (prop + revision _ (:= (call Rev)))
)
```

```output
true
7
```

### A step function an interface declares with a result and no steps has no body, so a class made the interface must declare it.

```brix compile-error
(main
  (let walker Walker (call Thing))
  (print (call (. walker step)))
)

(type + Walker (is Interface)
  (fn-do + step Int)
)

(type + Thing
  (implements Walker)
)
```

```output
Exception: Thing does not implement "step", which Walker declares without a body; declare "step" in Thing
<no file:11>
    (type + Thing
    ^^^^^^^^^^^^
Interface:
<no file:8>
      (fn-do + step Int)
```

### A missing operator an interface declares is named as the interface writes it.

```brix compile-error
(main
  (let source Source (call Thing))
  (print "made")
)

(type + Source (is Interface)
  (fn-expr + (prefix <- this) (? Int))
)

(type + Thing
  (implements Source)
)
```

```output
Exception: Thing does not implement "<- this", which Source declares without a body; declare "<- this" in Thing
<no file:11>
    (type + Thing
    ^^^^^^^^^^^^
Interface:
<no file:8>
      (fn-expr + (prefix <- this) (? Int))
```

### An assignment to a property of a class that leaves an interface's getter and setter to their defaults calls the default setter.

```brix
(main
  (let holder (call Holder))
  (= (. holder label) "a")
  (print (. holder label))
)

(type + Host (is Interface)
  (fn-expr + label String :get
    "default"
  )
  (fn-do + label _ :set (param value String)
    (print (str "set " value))
  )
)

(type + Holder
  (implements Host)
)
```

```output
set a
default
```

### A property a subclass stores in place of the default getter and setter of an interface its base class declares answers for them through a value of the base class.

```brix
(main
  (let base Base (call Child))
  (print (. base label))
  (= (. base label) "stored")
  (print (. base label))
)

(type + Host (is Interface)
  (fn-expr + label String :get
    "default"
  )
  (fn-do + label _ :set (param value String)
    (print (str "default " value))
  )
)

(type + Base
  (implements Host)
)

(type + Child (is Base)
  (prop + label _ (= "child"))
)
```

```output
child
stored
```
