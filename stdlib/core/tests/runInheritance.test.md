# Inheritance

```brix
(main
  (let bar (call Bar))
  (let thing Foo bar)
  (print (. thing a))
)

(type + Foo
  (prop + a _ (= 0))
)

(type + Bar (is Foo))
```

```output
0
```

```brix
(main
  (let bar (call Bar))
  (print (. bar b))
  (print (. bar a))
)

(type + Foo
  (prop + a _ (= 42))
)

(type + Bar (is Foo)
  (prop + b _ (= 99))
)
```

```output
99
42
```

### Overriding base property

```brix
(main
  (let bar (call Bar))
  (print (. bar a))
  (print (call show bar))
)

(type + Foo
  (prop + a _ (= 42))
)

(type + Bar (is Foo)
  (prop + a _ (= 99))
)

(fn-expr + show _
  (param foo Foo)
  (. foo a)
)
```

```output
99
99
```

### Overriding base property with different type should cast to base type

```brix
(main
  (let bar (call Bar))
  (print (. bar a))
  (print (call show bar))
)

(type + Foo
  (prop + a _ (= 42.0))
)

(type + Bar (is Foo)
  (prop + a _ (= 99))
)

(fn-expr + show _
  (param foo Foo)
  (. foo a)
)
```

```output
99
99
```

```brix
(main
  (let bar (call Bar))
  (call (. bar bar))
)

(type + Foo
  (prop + a _ (= 42))
)

(type + Bar (is Foo)
  (prop + b _ (= 99))
  (fn-do + bar _
    (print b)
    (print a)
  )
)
```

```output
99
42
```

```brix
(main
  (let bar (call Bar))
  (call (. bar bar))
  (call (. bar foo))
)

(type + Foo
  (prop + a _ (= 42))
  (fn-do + foo _
    (print (str "Foo.a = " a))
  )
)

(type + Bar (is Foo)
  (prop + b _ (= 99))
  (fn-do + bar _
    (print (str "Bar.b = " b))
    (print (str "Bar.a = " a))
  )
)
```

```output
Bar.b = 99
Bar.a = 42
Foo.a = 42
```

```brix
(main
  (let bar (call Bar))
  (call act bar)
)

(type + Foo
  (prop + a _ (= 42))
  (fn-do + foo _
    (print (str "Foo.a = " a))
  )
)

(type + Bar (is Foo)
  (prop + b _ (= 99))
  (fn-do + bar _
    (print (str "Bar.b = " b))
    (print (str "Bar.a = " a))
  )
)

(fn-do + act _
  (param thing Foo)
  (call (. thing foo))
)
```

```output
Foo.a = 42
```

```brix
(main
  (let bar (call Bar))
  (call (. bar f1))
)

(type + Foo
  (fn-do + f1 _
    (print "Foo 1")
  )
)

(type + Bar (is Foo)
  (fn-do + f1 _
    (print "Bar 1")
  )
)
```

```output
Bar 1
```

```brix
(main
  (let bar (call Bar))
  (blank)
  ; Downcast still calls method on subclass
  (let foo Foo bar)
  (call (. foo f1))
)

(type + Foo
  (fn-do + f1 _
    (print "Foo 1")
  )
)

(type + Bar (is Foo)
  (fn-do + f1 _
    (print "Bar 1")
  )
)
```

```output
Bar 1
```

```brix
(main
  (let bar (call Bar))
  (let foo (call Foo))
  (blank)
  (call (. bar f1))
  (call (. foo f2))
)

(type + Foo
  (fn-do + f1 _
    (print "Foo 1")
  )
  (fn-do + f2 _
    (print "Foo 2")
  )
)

(type + Bar (is Foo)
  (fn-do + f1 _
    (print "Bar 1")
  )
)
```

```output
Bar 1
Foo 2
```

```brix
(main
  (let bar (call Bar))
  (blank)
  ; Downcast still calls method on subclass
  (let foo Foo bar)
  (call (. foo f2))
)

(type + Foo
  (fn-do + f1 _
    (print "Foo 1")
  )
  (fn-do + f2 _
    (print "Foo 2")
  )
)

(type + Bar (is Foo)
  (fn-do + f1 _
    (print "Bar 1")
  )
)
```

```output
Foo 2
```

```brix
(main
  (let bar (call Bar))
  (blank)
  ; Downcast in argument still calls method on subclass
  (let foo Foo bar)
  (call act foo)
)

(type + Foo
  (fn-do + f1 _
    (print "Foo 1")
  )
)

(type + Bar (is Foo)
  (fn-do + f1 _
    (print "Bar 1")
  )
)

(fn-do + act _
  (param foo Foo)
  (call (. foo f1))
)
```

```output
Bar 1
```

```brix
(main
  (let bar (call Bar))
  (blank)
  ; Downcast still calls method on subclass
  (let foo Foo bar)
  (call (. foo f1) 5.5)
  (call (. foo f1) 3)
)

(type + Foo
  (fn-do + (of f1 T) _
    (param a T)
    (print (str "Foo 1 " a))
  )
)

(type + Bar (is Foo)
  (fn-do + (of f1 T) _
    (param a T)
    (print (str "Bar 1 " a))
  )
)
```

```output
Bar 1 5.5
Bar 1 3
```

### This test verifies a fix I made to the layout of class structs which puts the lazy flag in a predictable place for all subclasses.

```brix
(main
  (let foo (call Foo))
  (print (. foo a))
  (let bar (call Bar))
  (print (. bar a))
)

(type + Foo
  (prop + foo _ (= 42))
  (prop + a _ (=> b))
  (prop + b _ (:= foo))
)

(type + Bar (is Foo)
  ; This is here just to shift the properties down in the struct
  ; which, in the old code, would move the lazy flag lower in the subclass struct only
  (prop + bar _ (= 1))
)
```

```output
42
42
```

### Direct dispatch must account for declared subclasses that are instantiated later

```brix
(main
  (let base (call Base))
  (call act base)
  (blank)
  (let child (call Child))
  (call act child)
)

(type + Base
  (fn-do + name _
    (print "Base")
  )
)

(type + Child (is Base)
  (fn-do + name _
    (print "Child")
  )
)

(fn-do + act _
  (param thing Base)
  (call (. thing name))
)
```

```output
Base
Child
```

### Direct dispatch must account for generic declared subclasses that are instantiated later

```brix
(main
  (let base (call (of Base Int)))
  (call act base)
  (blank)
  (let child (call (of Child Int)))
  (call act child)
)

(type + (of Base T)
  (fn-do + name _
    (print "Base")
  )
)

(type + (of Child T) (is (of Base T))
  (fn-do + name _
    (print "Child")
  )
)

(fn-do + act _
  (param thing (of Base Int))
  (call (. thing name))
)
```

```output
Base
Child
```

### Direct dispatch must account for generic subclasses of non-generic bases

```brix
(main
  (let base (call Base))
  (call act base)
  (blank)
  (let child (call (of Child Int)))
  (call act child)
)

(type + Base
  (fn-do + name _
    (print "Base")
  )
)

(type + (of Child T) (is Base)
  (fn-do + name _
    (print "Child")
  )
)

(fn-do + act _
  (param thing Base)
  (call (. thing name))
)
```

```output
Base
Child
```

### An inherited event cannot be redeclared as a property

```brix crash
(main
  (call EventPropertyChild)
)

(type + EventBase
  (event + changed Int)
)

(type + EventPropertyChild (is EventBase)
  (prop + changed _ (= 1))
)
```

```output
Exception: Inherited event "changed" cannot be redeclared as a property
<no file:11>
      (prop + changed _ (= 1))
      ^^^^^^^^^^^^^^^^^^^^^^^^
```

### An inherited property cannot be redeclared as a event

```brix crash
(main
  (call PropertyEventChild)
)

(type + PropertyBase
  (prop + changed _ (= 1))
)

(type + PropertyEventChild (is PropertyBase)
  (event + changed Int)
)
```

```output
Exception: Inherited property "changed" cannot be redeclared as an event
<no file:11>
      (event + changed Int)
      ^^^^^^^^^^^^^^^^^^^^^
```

### An inherited event cannot be redeclared as a method

```brix crash
(main
  (call (. (call EventMethodChild) run))
)

(type + EventMethodBase
  (event + submit String)
)

(type + EventMethodChild (is EventMethodBase)
  (fn-do + run _
    (call submit "x")
  )
  (fn-do _ submit _
    (param text String)
    (print text)
  )
)
```

```output
Exception: Inherited event "submit" cannot be redeclared as a method
<no file:14>
      (fn-do _ submit _
      ^^^^^^^^^^^^^^^^
```

### An inherited method cannot be redeclared as an event

```brix crash
(main
  (call MethodEventChild)
)

(type + MethodEventBase
  (fn-do + changed _
    (param value Int)
  )
)

(type + MethodEventChild (is MethodEventBase)
  (event + changed Int)
)
```

```output
Exception: Inherited method "changed" cannot be redeclared as an event
<no file:13>
      (event + changed Int)
      ^^^^^^^^^^^^^^^^^^^^^
```

### Lowered inherited subscriptions preserve base-before-derived lexical order

```brix
(main
  (let owner (call OrderedEventChild))
  (blank)
  (for (on value (. owner changed))
    (print (str "external " value))
  )
  (blank)
  (call (. owner emit) 9)
)

(type + OrderedEventBase
  (event + changed Int)
  (subscribe-for (on value changed)
    (print (str "base-1 " value))
  )
  (subscribe-for (on value changed)
    (print (str "base-2 " value))
  )
  (fn-do + emit _
    (param value Int)
    (<- changed value)
  )
)

(type + OrderedEventChild (is OrderedEventBase)
  (subscribe-for (on value changed)
    (print (str "derived-1 " value))
  )
  (subscribe-for (on value changed)
    (print (str "derived-2 " value))
  )
)
```

```output
base-1 9
base-2 9
derived-1 9
derived-2 9
external 9
```

```brix
(main
  (let child (call (of Child I32)))
  (= (. child target) (call (of Target I32)))
  (= (. child value) 42)
  (call (. child run))
)

(type + Root)

(type + Base (is Root)
  (fn-do + run _
    (call execute)
  )
  (fn-do + execute _
    (print "base")
  )
)

(type + (of Target T)
  (fn-do + accept _
    (param value T)
    (print value)
  )
)

(type + (of Child T) (is Base)
  (prop + target (of Target T))
  (prop + value T)
  (fn-do + execute _
    (call (. target accept) value)
  )
)
```

```output
42
```

```brix
(main
  (let derived (call pick true))
  (let base (call pick false))
  (print (is derived Derived))
  (print (is base Derived))
  (print (call (. base name)))
  (let fallback (if-expr false (call Derived) (call Base)))
  (print (call (. fallback name)))
)

(type + Base
  (fn-do + name String
    (return "base")
  )
)

(type + Derived (is Base)
  (fn-do + name String
    (return "derived")
  )
)

(fn-expr + pick Base
  (param derived Bool)
  (if-expr derived (call Derived) (call Base))
)
```

```output
true
false
base
base
```

### A subclass getter can read its base getter through super without dispatching back to itself.

```brix
(main
  (print (. (call SuperReadChild) label))
  (print (. (call SuperReadChild) is-ready))
)

(type + SuperReadBase
  (fn-do + label String :get
    (return "base")
  )
  (fn-do + is-ready Bool :get
    (return true)
  )
)

(type + SuperReadChild (is SuperReadBase)
  (fn-do + label _ :get
    (return (str "child of " (. super label)))
  )
  (fn-do + is-ready _ :get
    (return (and (. super is-ready) false))
  )
)
```

```output
child of base
false
```

### This in an inherited method names the declaring class, the type of `this` there.

```brix
(main
  (let keyed (call ThisKeyed))
  (let same (call (. keyed me)))
  (print (. same count))
  (print (call (of (. keyed with) Int) (=> (item) (+ (. item count) 1))))
)

(type + ThisBase
  (prop + count _ (= 1))
  (fn-expr + me This
    this
  )
  (fn-expr + (of with Result) _
    (param func (fn-type Result (param _ This)))
    (call func this)
  )
)

(type + ThisKeyed (is ThisBase)
  (prop + extra _ (= 2))
)
```

```output
1
2
```

### A member only the subclass declares is not visible through an inherited This.

```brix compile-error
(main
  (let keyed (call ThisKeyed))
  (print (. (call (. keyed me)) extra))
)

(type + ThisBase
  (fn-expr + me This
    this
  )
)

(type + ThisKeyed (is ThisBase)
  (prop + extra _ (= 2))
)
```

```output
Exception: ThisBase has no property "extra"
<no file:4>
      (print (. (call (. keyed me)) extra))
             ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A method called in a constructor is the one of the constructor's own class, whether it is called by name, on `this`, or by another method the constructor calls.

```brix
(main
  (call Child)
  (print "--")
  (call Base)
)

(type + Base
  (fn-do + This _
    (call report)
    (call (. this report))
    (call relay)
  )
  (fn-do + relay _
    (call report)
  )
  (fn-do + report _
    (print "base")
  )
)

(type + Child (is Base)
  (fn-do + This _
    (call super)
    (print "child ctor")
    (call report)
    (call relay)
  )
  (fn-do + report _
    (print "child")
  )
)
```

```output
base
base
child
child ctor
child
child
--
base
base
base
```

### A constructor the compiler generates gives the base's properties their values before its own, and a method a base's constructor calls is the base's.

```brix
(main
  (call Child)
  (print "--")
  (call Grand)
)

(type + Base
  (prop + a _ (= (call say "a")))
  (fn-do + This _
    (call report)
    (call relay)
  )
  (fn-do + relay _
    (call report)
  )
  (fn-do + report _
    (print "base")
  )
)

(type + Child (is Base)
  (prop + b _ (= (call say "b")))
  (fn-do + report _
    (print "child")
  )
)

(type + Grand (is Child)
  (prop + c _ (= (call say "c")))
  (fn-do + This _
    (call super)
    (print "grand after")
  )
  (fn-do + report _
    (print "grand")
  )
)

(fn-do + say String
  (param s String)
  (print (str "say " s))
  (return s)
)
```

```output
say a
say b
child
child
--
say a
say b
say c
child
grand
grand after
```

### A setter a subclass declares again is called through a value held as the base.

```brix
(main
  (let b Base (call Child))
  (= (. b value) 5)
  (print (. b stored))
  (let c Base (call Base))
  (= (. c value) 5)
  (print (. c stored))
)

(type + Base
  (prop + stored _ (= 0))
  (fn-do + value _ :set (param v Int)
    (= stored v)
  )
)

(type + Child (is Base)
  (fn-do + value _ :set (param v Int)
    (= stored (* v 2))
  )
)
```

```output
10
5
```

### An operator a subclass declares again is called through a value held as the base.

```brix
(main
  (let a Base (call Child))
  (let b Base (call Base))
  (print (+ a 1))
  (print (+ b 1))
  (print (== a b))
  (print (not a))
  (print (not b))
)

(type + Base
  (fn-expr + (infix this + (param n Int)) _
    (+ n 100)
  )
  (fn-expr + (infix this == (param other Base)) _
    true
  )
  (fn-expr + (prefix ! this) _
    false
  )
)

(type + Child (is Base)
  (fn-expr + (infix this + (param n Int)) _
    (+ n 200)
  )
  (fn-expr + (prefix ! this) _
    true
  )
)
```

```output
201
101
true
true
false
```

### A panic raised by a method a subclass declares again passes through a call made on the base.

```brix crash
(main
  (let b Base (call Child))
  (call run b)
  (print "after")
)

(fn-do + run _
  (param b Base)
  (call (. b go))
  (print "not here")
)

(type + Base
  (fn-do + go _
    (print "base")
  )
)

(type + Child (is Base)
  (fn-do + go _
    (throw (call Panic "child boom"))
  )
)
```

```stderr
Unhandled panic: child boom
<no file:22>

Task 1

Stack trace:
  0  __source__:Child:go no file:22:5
  1  __source__:run no file:10:3
  2  __source__:main no file:4:3
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### A panic raised by a getter a subclass declares again passes through a read made on the base, and the trace names the getter with its class.

```brix crash
(main
  (let b Base (call Child))
  (call show b)
  (print "after")
)

(fn-do + show _
  (param b Base)
  (print (. b label))
  (print "not here")
)

(type + Base
  (fn-expr + label _ :get
    "base"
  )
)

(type + Child (is Base)
  (fn-expr + label _ :get
    (call boom)
  )
)

(fn-do + boom String
  (throw (call Panic "getter boom"))
  (return "child")
)
```

```stderr
Unhandled panic: getter boom
<no file:27>

Task 1

Stack trace:
  0  __source__:boom no file:27:3
  1  __source__:Child:label no file:22:5
  2  __source__:show no file:10:10
  3  __source__:main no file:4:3
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### `super(…)` gives the base's constructor the object being made: the properties its arguments fill are stored again, with the values that read them.

```brix
(main
  (let c (call Child 5))
  (print (. c n))
  (print (. c double))
  (print (. c other))
  (print (. c extra))
)

(type + Base
  (prop + n _ (= 1))
  (prop + double _ (= (* n 2)))
  (prop + other _ (= (call say 7)))
  (fn-do + This _
    (param n _)
    (print (str "base body " n " " double))
  )
)

(type + Child (is Base)
  (prop + extra _ (= (call say 9)))
  (fn-do + This _
    (param k Int)
    (call super k)
    (print (str "child body " n " " double))
  )
)

(fn-do + say Int
  (param v Int)
  (print (str "say " v))
  (return v)
)
```

```output
say 7
say 9
base body 5 10
child body 5 10
5
10
7
9
```

### `super(…)` through three classes.

```brix
(main
  (let g (call Grand))
  (print (. g a))
  (print (. g b))
  (print (. g c))
)

(type + Base
  (prop + a _ (= 1))
  (fn-do + This _
    (param a _)
    (print (str "base " a))
  )
)

(type + Child (is Base)
  (prop + b _ (= 2))
  (fn-do + This _
    (param b _)
    (call super (+ b 10))
    (print (str "child " a " " b))
  )
)

(type + Grand (is Child)
  (prop + c _ (= 3))
  (fn-do + This _
    (call super 20)
    (print (str "grand " a " " b " " c))
  )
)
```

```output
base 30
child 30 20
grand 30 20 3
30
20
3
```

### A getter reads the one it replaces through `super`.

```brix
(main
  (let c (call Child))
  (print (. c label))
  (print (call (. c both)))
)

(type + Base
  (fn-expr + label _ :get
    "base"
  )
  (fn-expr + name String
    "base name"
  )
)

(type + Child (is Base)
  (fn-expr + label _ :get
    (str "child+" (. super label))
  )
  (fn-expr + name String
    "child name"
  )
  (fn-expr + both String
    (str (call (. super name)) " / " (call name))
  )
)
```

```output
child+base
base name / child name
```

### A method makes another object of its class with `This(…)`.

```brix
(main
  (let c (call Child 1))
  (print (. c n))
  (let d (call (. c again)))
  (print (. d n))
)

(type + Base
  (prop + n _ (= 0))
  (fn-do + This _
    (param n _)
  )
)

(type + Child (is Base)
  (fn-do + This _
    (param k Int)
    (call super (+ k 1))
  )
  (fn-expr + again Child
    (call This (* n 10))
  )
)
```

```output
2
21
```

### An object is tested for its class and for a class derived from it.

```brix
(main
  (let b Base (call Child))
  (print (is b Base))
  (print (is b Child))
  (print (is b Other))
  (print (is-not b Child))
  (print (is-not b Other))
)

(type + Base
  (prop + n _ (= 1))
)

(type + Child (is Base)
  (prop + m _ (= 7))
)

(type + Other (is Base))

(type + Lone)
```

```output
true
true
false
false
true
```

### An object is never of a class that is not in its chain.

```brix
(main
  (let c (call Child))
  (print (is c Other))
)

(type + Base
  (prop + n _ (= 1))
)

(type + Child (is Base)
  (prop + m _ (= 7))
)

(type + Other (is Base))

(type + Lone)
```

```output
false
```

### An object of a class two below is of each class above it.

```brix
(main
  (let g Base (call Grand))
  (print (is g Child))
  (print (is g Grand))
  (print (is g Other))
  (let c Child (call Grand))
  (print (is c Grand))
  (print (. (as c Grand) deep))
)

(type + Grand (is Child)
  (prop + deep _ (= 99))
)

(type + Base
  (prop + n _ (= 1))
)

(type + Child (is Base)
  (prop + m _ (= 7))
)

(type + Other (is Base))

(type + Lone)
```

```output
true
true
false
true
99
```

### An object held as its base is cast to the class it was made as.

```brix
(main
  (let b Base (call Child))
  (let c (as b Child))
  (print (. c m))
  (print (. (as b Child) m))
  (let u (as c Base))
  (print (. u n))
)

(type + Base
  (prop + n _ (= 1))
)

(type + Child (is Base)
  (prop + m _ (= 7))
)

(type + Other (is Base))

(type + Lone)
```

```output
7
7
1
```

### A cast to a class the object is not of panics.

```brix crash
(main
  (let b Base (call Child))
  (print "before")
  (let o (as b Other))
  (print "not here")
)

(type + Base
  (prop + n _ (= 1))
)

(type + Child (is Base)
  (prop + m _ (= 7))
)

(type + Other (is Base))

(type + Lone)
```

```stderr
Unhandled panic: Invalid cast to [Object __source__:Other]
<no file:5>

Task 1

Stack trace:
  0  __source__:main no file:5:10
  1  __source__:init.__source__ no file
  2  __source__:brix.main no file
```

### A value of a base class cast with as and stored in an optional property of a derived class panics when the object is not of that class.

```brix crash
(main
  (let h (call Holder))
  (call (. h f) (call Child))
  (print (?. (. h c) value))
  (call (. h f) (call Box))
)

(type + Holder
  (prop + c (? Child))
  (fn-do + f _
    (param b Box)
    (= c (as b Child))
  )
)

(type + Box)

(type + Child (is Box)
  (prop + value _ (= 5))
)
```

```stderr
Unhandled panic: Invalid cast to [Object __source__:Child]
<no file:13>

Task 1

Stack trace:
  0  __source__:Holder:f no file:13:10
  1  __source__:main no file:6:3
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

### An optional of a base class cast to a derived class unwraps, then panics when the object is not of that class.

```brix crash
(main
  (let b (call Box))
  (= (. b first) (call Child))
  (let c (as (. b first) Child))
  (print (. c value))
  (= (. b first) (call Box))
  (let d (as (. b first) Child))
  (print "not here")
)

(type + Box
  (prop + first (? Box))
)

(type + Child (is Box)
  (prop + value _ (= 3))
)
```

```stderr
Unhandled panic: Invalid cast to [Object __source__:Child]
<no file:8>

Task 1

Stack trace:
  0  __source__:main no file:8:10
  1  __source__:init.__source__ no file
  2  __source__:brix.main no file
```

### A member a class inherits through its base is the nearest one declared above it.

```brix
(main
  (let leaf (call Leaf))
  (let held Mid (call Leaf))
  (print (call (. leaf one) 3))
  (print (call (. held one) 4))
  (print (. leaf label))
  (= (. leaf value) 4)
  (print (. leaf stored))
  (print (call (. leaf plain)))
  (print (call (. leaf both)))
  (print (call (. (call Last) one) 5))
  (print (call (. (call Last) both)))
)

(type + Base
  (prop + stored _ (= 0))
  (fn-expr + one String
    (param n Int)
    (str "base one " n)
  )
  (fn-expr + label _ :get
    "base label"
  )
  (fn-do + value _ :set (param v Int)
    (= stored v)
  )
  (fn-expr + plain String
    "base plain"
  )
  (fn-expr + both String
    (str (call one 1) " " label)
  )
)

(type + Mid (is Base)
  (fn-expr + one String
    (param n Int)
    (str "mid one " n)
  )
  (fn-expr + label _ :get
    "mid label"
  )
  (fn-do + value _ :set (param v Int)
    (= stored (* v 10))
  )
)

(type + Leaf (is Mid))

(type + Last (is Leaf))
```

```output
mid one 3
mid one 4
mid label
40
base plain
mid one 1 mid label
mid one 5
mid one 1 mid label
```

### `super` reaches the nearest member declared above the base.

```brix
(main
  (print (call (. (call Leaf) name)))
  (print (. (call Leaf) label))
  (print (. (call Leaf) n))
)

(type + Base
  (prop + n _ (= 1))
  (fn-expr + name String
    "base"
  )
  (fn-expr + label _ :get
    "base label"
  )
)

(type + Mid1 (is Base)
  (prop + n _ (= 2))
  (fn-expr + name String
    "mid1"
  )
  (fn-expr + label _ :get
    "mid1 label"
  )
)

(type + Mid2 (is Mid1))

(type + Leaf (is Mid2)
  (fn-expr + name String
    (str "leaf<" (call (. super name)) ">")
  )
  (fn-expr + label _ :get
    (str "leaf<" (. super label) ">")
  )
)
```

```output
leaf<mid1>
leaf<mid1 label>
2
```

### A value a class gives a property of its base is converted to the type the base gave the property, through every class between.

```brix
(main
  (let leaf (call Leaf))
  (print (. leaf a))
  (print (call half leaf))
  (print (. (type-of (. leaf a)) name))
  (print (call half (call Mid)))
  (print (call half (call Base)))
  (= (. leaf a) 3)
  (print (call half leaf))
)

(type + Base
  (prop + a _ (= 42.0))
)

(type + Mid (is Base)
  (prop + a _ (= 99))
)

(type + Leaf (is Mid)
  (prop + a _ (= 7))
)

(fn-expr + half _
  (param base Base)
  (/ (. base a) 2)
)
```

```output
7
3.5
F64
49.5
21
1.5
```

### A class gives a property its base declared a type for a value of another type, which is converted as one assigned is.

```brix
(main
  (let bar (call Bar))
  (print (. bar a))
  (print (. (type-of (. bar a)) name))
  (print (. bar wide))
  (print (. (type-of (. bar wide)) name))
  (print (. bar count))
  (print (. (type-of (. bar count)) name))
  (print (call total bar))
)

(type + Foo
  (prop + a F64 (= 42.0))
  (prop + wide _ (= (as 1 I64)))
  (prop + count _ (= 3))
)

(type + Bar (is Foo)
  (prop + a _ (= 99))
  (prop + wide _ (= 7))
  (prop + count _ (= 12u8))
)

(fn-expr + total _
  (param foo Foo)
  (+ (. foo a) (. foo wide) (. foo count))
)
```

```output
99
F64
7
I64
12
I64
118
```

### A getter and a method of the base are reached after super.

```brix
(main
  (let box (call Box))
  (call (. box grow))
)

(type + Base
  (prop + held _ (= 1))
  (fn-expr + twice _ :get
    (* held 2)
  )
  (fn-do + show _
    (print "base show")
  )
)

(type + Box (is Base)
  (fn-expr + twice _ :get
    (* held 3)
  )
  (fn-do + show _
    (print "box show")
  )
  (fn-do + grow _
    (print (. super twice))
    (print twice)
    (call (. super show))
    (call show)
  )
)
```

```output
2
3
base show
box show
```

### super is no value: a stored property is not read after it.

```brix crash
(main
  (let box (call Box))
  (call (. box grow))
)

(type + Base
  (prop + held _ (= 1))
)

(type + Box (is Base)
  (fn-do + grow _
    (print (. super held))
  )
)
```

```output
Exception: Bare super is not a value
<no file:13>
        (print (. super held))
                  ^^^^^
```

### super is no value: a property is not assigned after it.

```brix crash
(main
  (let box (call Box))
  (call (. box grow))
)

(type + Base
  (prop + held _ (= 1))
)

(type + Box (is Base)
  (fn-do + grow _
    (= (. super held) 5)
  )
)
```

```output
Exception: Bare super is not a value
<no file:13>
        (= (. super held) 5)
              ^^^^^
```

### super is no value: it is not given as an argument.

```brix crash
(main
  (let box (call Box))
  (call (. box grow))
)

(type + Base
  (prop + held _ (= 1))
)

(type + Box (is Base)
  (fn-do + grow _
    (call take super)
  )
)

(fn-do + take _
  (param base Base)
  (print (. base held))
)
```

```output
Exception: Bare super is not a value
<no file:13>
        (call take super)
                   ^^^^^
```

### A method with type parameters of its own that a subclass declares again is called through a value held as the base, with the type parameters the call gave.

```brix
(main
  (let bar (call Bar))
  (let foo Foo bar)
  (call (. foo f1) 5.5)
  (call (. foo f1) 3)
  (call (of (. foo f2) Int) 1)
  (call (of (. foo f2) String) "a")
  (print (call (of (. foo f3) Int)))
  (print (call (of (. foo f3) String)))
  (let plain Foo (call Foo))
  (call (. plain f1) 2)
  (call (of (. plain f2) Int) 1)
  (print (call (of (. plain f3) Int)))
)

(type + Foo
  (fn-do + (of f1 T) _
    (param a T)
    (print (str "Foo 1 " a))
  )
  (fn-do + (of f2 T) _
    (param a T)
    (print (str "Foo 2 " a))
  )
  (fn-expr + (of f3 T) String
    "Foo 3"
  )
)

(type + Bar (is Foo)
  (fn-do + (of f1 T) _
    (param a T)
    (print (str "Bar 1 " a))
  )
  (fn-do + f2 _
    (param a Int)
    (print (str "Bar 2 " a))
  )
  (fn-expr + (of f3 T) String
    (constraint is! T Int)
    "Bar 3"
  )
)
```

```output
Bar 1 5.5
Bar 1 3
Foo 2 1
Foo 2 a
Bar 3
Foo 3
Foo 1 2
Foo 2 1
Foo 3
```

### A method whose type parameter must be exactly This answers only for that class, called directly or through the base.

```brix
(main
  (let box Box (call Square))
  (print (call (of (. box pick) Square)))
  (print (call (of (. box pick) Circle)))
  (let square (call Square))
  (print (call (of (. square pick) Square)))
  (print (call (of (. square pick) Circle)))
)

(type + Box
  (fn-expr + (of pick Target) String
    "box"
  )
)

(type + Circle (is Box))

(type + Square (is Box)
  (fn-expr + (of pick Target) String
    (constraint is! Target This)
    "square"
  )
)
```

```output
square
box
square
box
```

### A lazy property's value is typed once, for the class that declares it, and reads the object it is evaluated for.

```brix
(main
  (let d (call Derived))
  (print (. d who))
  (let b (call Base))
  (print (. b who))
  (let held Base (call Derived))
  (print (. held who))
  (print (. (call Other) who))
  (let other Base (call Other))
  (print (. other who))
  (print (. d said))
  (print (. b said))
  (print (. held said))
  (print (. d loud))
  (print (call name-of (. d me)))
  (print (call name-of (. (call Other) me)))
)

(fn-expr + describe String
  (param b Base)
  "base"
)

(fn-expr + describe String
  (param d Derived)
  "derived"
)

(fn-expr + describe String
  (param o Other)
  "other"
)

(fn-expr + name-of String
  (param b Base)
  (str (call describe b) " " (. b label))
)

(type + Base
  (prop + who _ (:= (call describe this)))
  (prop + said _ (:= (++ label "!")))
  (prop + loud _ (:= (++ (. this label) "?")))
  (prop + me _ (:= this))
  (fn-expr + label _ :get
    "plain"
  )
)

(type + Derived (is Base)
  (fn-expr + label _ :get
    "fancier"
  )
)

(type + Other (is Base)
  (prop + who _ (:= (call describe this)))
)
```

```output
base
base
base
other
other
fancier!
plain!
fancier!
fancier?
base fancier
base plain
```

### A method that writes a type for an argument overrides a base method that writes none, called through the base, from inside it, and through a class between.

```brix
(main
  (let held Base (call Derived))
  (call (. held put) 1)
  (call (. held relay) 2)
  (let lower Base (call Bottom))
  (call (. lower put) 3)
)

(type + Base
  (fn-do + put _
    (param thing _)
    (print (str "(base any, " thing ")"))
  )
  (fn-do + relay _
    (param value Int)
    (call put value)
  )
)

(type + Derived (is Base)
  (fn-do + put _
    (param thing Int)
    (print (str "(derived int, " thing ")"))
  )
)

(type + Middle (is Base))

(type + Bottom (is Middle)
  (fn-do + put _
    (param thing Int)
    (print (str "(bottom int, " thing ")"))
  )
)
```

```output
(derived int, 1)
(derived int, 2)
(bottom int, 3)
```

### An override that answers a subclass of what the base answers is called through an adapter, which a trace names.

```brix crash
(main
  (let base Animal (call Dog))
  (print (. (call (. base make)) name))
)

(type + Animal
  (prop + name _ (= "animal"))
  (fn-expr + make Animal
    (call Animal)
  )
)

(type + Dog (is Animal)
  (fn-expr + make Dog
    (throw (call Panic "boom"))
  )
)
```

```stderr
Unhandled panic: boom
<no file:16>

Task 1

Stack trace:
  0  __source__:Dog:make no file:16:5
  1  __source__:make(class-adapter) no file:15:3
  2  __source__:main no file:4:13
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### An override that answers a narrower number than the base converts it where it is called as the base.

```brix
(main
  (let base Source (call Narrow))
  (print (call (. base value)))
)

(type + Source
  (fn-expr + value Int
    1
  )
)

(type + Narrow (is Source)
  (fn-expr + value I16
    7
  )
)
```

```output
7
```

### A panic in an override that answers a narrower number names the adapter in its trace.

```brix crash
(main
  (let base Source (call Narrow))
  (print (call (. base value)))
)

(type + Source
  (fn-expr + value Int
    1
  )
)

(type + Narrow (is Source)
  (fn-expr + value I16
    (throw (call Panic "boom"))
  )
)
```

```stderr
Unhandled panic: boom
<no file:15>

Task 1

Stack trace:
  0  __source__:Narrow:value no file:15:5
  1  __source__:value(class-adapter) no file:14:3
  2  __source__:main no file:4:10
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### An override answering a type its base's answer converts to fills the base's slot with an adapter.

```brix
(main
  (let b Base (call Sub))
  (print (call (. b value)))
)

(type + Base
  (fn-expr + value Float
    1.5
  )
)

(type + Sub (is Base)
  (fn-expr + value Int
    300
  )
)
```

```output
300
```

### A getter that overrides another may not state its type.

```brix crash
(main
  (let base Source (call Narrow))
  (print (. base value))
)

(type + Source
  (fn-expr + value Int :get
    1
  )
)

(type + Narrow (is Source)
  (fn-expr + value Int :get
    7
  )
)
```

```output
Exception: "value" is inherited from Source, so it keeps the type it has there and cannot state its own; remove "Int"
<no file:14>
      (fn-expr + value Int :get
      ^^^^^^^^^^^^^^^^^^^^^^^^
```

### A getter may state its type over a base getter that states none.

```brix
(main
  (let base Source (call Narrow))
  (print (. base value))
  (print (. (call Narrow) value))
)

(type + Source
  (fn-expr + value _ :get
    1
  )
)

(type + Narrow (is Source)
  (fn-expr + value Int :get
    7
  )
)
```

```output
7
7
```

### A getter may state its type over a chain of getters whose first states none, though the one it overrides states one.

```brix
(main
  (let base Source (call Leaf))
  (print (. base value))
  (print (. (call Mid) value))
)

(type + Source
  (fn-expr + value _ :get
    1
  )
)

(type + Mid (is Source)
  (fn-expr + value Int :get
    2
  )
)

(type + Leaf (is Mid)
  (fn-expr + value Int :get
    3
  )
)
```

```output
3
2
```

### A getter may not state its type over a chain of getters whose first states one, though the one it overrides states none.

```brix crash
(main
  (let base Source (call Leaf))
  (print (. base value))
)

(type + Source
  (fn-expr + value Int :get
    1
  )
)

(type + Mid (is Source)
  (fn-expr + value _ :get
    2
  )
)

(type + Leaf (is Mid)
  (fn-expr + value Int :get
    3
  )
)
```

```output
Exception: "value" is inherited from Mid, so it keeps the type it has there and cannot state its own; remove "Int"
<no file:20>
      (fn-expr + value Int :get
      ^^^^^^^^^^^^^^^^^^^^^^^^
```

### Two classes made from one generic class, each overriding a method of their base with one that answers another type, each have an adapter of their own in the base's table.

```brix
(main
  (let first Base (call (of Holder String) (list "a" "b")))
  (let second Base (call (of Holder I64) (list 1 2 3)))
  (print (. first total))
  (print (. second total))
)

(type + Base
  (fn-expr + total Index :get
    0
  )
)

(type + (of Holder Item) (is Base)
  (prop + items (list Item))
  (fn-do + This _
    (param items _)
  )
  (fn-expr + total _ :get
    (. items length)
  )
)
```

```output
2
3
```

### A class nothing uses is not checked, so its override of a method called on its base is never typed.

```brix
(main
  (let shape Shape (call Square))
  (call print (call (. shape area)))
)

(type + Shape
  (fn-expr + area Int
    0
  )
)

(type + Square (is Shape)
  (fn-expr + area Int
    4
  )
)

(type + Broken (is Shape)
  (prop + size Int)
  (fn-expr + area Int
    (* size missing)
  )
)
```

```output
4
```

### A base-typed reference and a subclass-typed reference to one object compare equal.

```brix
(main
  (let bar (call Bar))
  (let other (call Bar))
  (let holder (call Holder))
  (= (. holder held) bar)
  (print (== (!! (. holder held)) bar))
  (print (== bar (!! (. holder held))))
  (print (!= (!! (. holder held)) bar))
  (print (== (!! (. holder held)) other))
  (print (!= (!! (. holder held)) other))
  (call (. bar hold) holder)
)

(type + Foo)

(type + Bar (is Foo)
  (fn-do + hold _
    (param holder Holder)
    (= (. holder held) this)
    (print (== (!! (. holder held)) this))
  )
)

(type + Holder
  (prop + held (? Foo))
)
```

```output
true
true
false
false
true
true
```

### A method that returns nothing does not override one that returns a truth or an optional.

```brix crash
(main
  (let shapes (list Shape))
  (+= shapes (call Shape))
  (+= shapes (call Dot))
  (for (in shape shapes)
    (print (call (. shape check)))
    (print (call (. shape label)))
  )
)

(type + Shape
  (fn-do + check Bool
    (return true)
  )
  (fn-do + label (? String)
    (return "shape")
  )
)

(type + Dot (is Shape)
  (fn-do + check _
    (print "dot check")
  )
  (fn-do + label _
    (print "dot label")
  )
)
```

```output
Exception: Expected Bool here, but this is Void, which becomes Bool only when asked; write "(as … Bool)"
<no file:22>
      (fn-do + check _
      ^^^^^^^^^^^^^^^

Exception: Expected String? here, but this is Void, which becomes String? only when asked; write "(as … (? String))"
<no file:25>
      (fn-do + label _
      ^^^^^^^^^^^^^^^
```

### A class named as a core class is the base of a subclass declared as of that name, so a method called through it reaches the subclass's override, though the call is compiled first.

```brix
(main
  (print (call show (call List)))
  (print (call show (call Thing)))
)

(type + List
  (fn-expr + name String
    "list"
  )
)

(type + Thing (is List)
  (fn-expr + name String
    "thing"
  )
)

(fn-expr + show String
  (param list List)
  (call (. list name))
)
```

```output
list
thing
```

### A method that declares no type, and gives no value as nothing after a statement that leaves is compiled, overrides one that gives a value, and runs when called through the base.

```brix crash
(main
  (let node Node (call Failing))
  (print (call (. node execute)))
)

(type + Node
  (fn-do + execute I32
    (return 0)
  )
)

(type + Failing (is Node)
  (fn-do + This _)
  (fn-do + execute _
    (let prior 1)
    (throw (call Panic "boom"))
    (return prior)
  )
)
```

```stderr
Unhandled panic: boom
<no file:17>

Task 1

Stack trace:
  0  __source__:Failing:execute no file:17:5
  1  __source__:execute(class-adapter) no file:15:3
  2  __source__:main no file:4:10
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

```brix
(main
  (let numbers (call Numbers))
  (print ([] numbers 1))
  (let plain (call PlainNumbers))
  (print ([] plain 0))
)

(type + (of Holder T)
  (prop + items (list T) (= (call (of List T))))
  (fn-expr + (index-get this (param index Index)) _
    ([] items index)
  )
)

(type + Numbers (is (of Holder Int))
  (fn-do + This _
    (+= items 4)
    (+= items 5)
  )
)

(type + PlainHolder
  (prop + items (list Int) (= (call (of List Int))))
  (fn-expr + (index-get this (param index Index)) _
    ([] items index)
  )
)

(type + PlainNumbers (is PlainHolder)
  (fn-do + This _
    (+= items 6)
  )
)
```

```output
5
6
```

### A getter over one whose type a base states keeps that type where the class reads it, so an integer it gives is of that type.

```brix
(main
  (let boat (call Rowboat))
  (print (. boat quarter))
  (let held Boat boat)
  (print (. held drop))
)

(type + Boat
  (fn-expr + drop F64 :get
    0.5
  )
)

(type + Rowboat (is Boat)
  (fn-expr + drop _ :get
    30
  )
  (fn-expr + quarter _ :get
    (/ drop 4)
  )
)
```

```output
7.5
30
```

```brix
(main
  (let plain (call PlainPainter))
  (let checked (call CheckedPainter))
  (call draw plain 2)
  (call draw checked 3)
  (print (. plain value))
  (print (. checked value))
  (call (. plain transform) 4)
  (print (. plain value))
)

(type + Painter
  (prop + value Float)
  (fn-do + translate _
    (param x Float)
    (call transform x)
  )
  (fn-do + transform _
    (param x Float)
    (= value x)
  )
)

(type + PlainPainter (is Painter)
  (fn-do + translate _
    (param x Float)
    (call transform x)
  )
)

(type + CheckedPainter (is Painter)
  (fn-do + transform _
    (param x Float)
    (throw (call Panic "negative") (if (< x 0)))
    (= value (+ x 10))
  )
)

(fn-do + draw _
  (param painter Painter)
  (param x Float)
  (call (. painter translate) x)
)
```

```output
2
13
4
```

### A class is built on one type, written in its header: a row under it that names a class names no interface, and is an error.

```brix compile-error
(main
  (let c (call C))
)

(type + A)

(type + B)

(type + C (is A)
  (implements B)
)
```

```output
Exception: C cannot implement B, which is not an interface; C is built on A, and a type is built on one type
<no file:11>
      (implements B)
                  ^
```

### A row under a class's header that names a class, where the header names none, says to write it in the header.

```brix compile-error
(main
  (let c (call C))
)

(type + A)

(type + C
  (implements A)
)
```

```output
Exception: C cannot implement A, which is not an interface; a type is built on one type, written in its header, as in "+ C is A"
<no file:9>
      (implements A)
                  ^
```
