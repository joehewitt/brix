# Methods

### Super calls exact base method implementation

```brix
(main
  (let child (call Child))
  (call (. child speak))
)

(type + Base
  (fn-do + speak _
    (print "base")
  )
)

(type + Child (is Base)
  (fn-do + speak _
    (print "child")
    (call (. super speak))
  )
)
```

```output
child
base
```

### Super starts at the immediate base but can find an ancestor method

```brix
(main
  (let leaf (call Leaf))
  (call (. leaf speak))
)

(type + Root
  (fn-do + speak _
    (print "root")
  )
)

(type + Middle (is Root))

(type + Leaf (is Middle)
  (fn-do + speak _
    (print "leaf")
    (call (. super speak))
  )
)
```

```output
leaf
root
```

### Super method keeps the current receiver

```brix
(main
  (let child (call Child))
  (= (. child label) "child")
  (call (. child show))
)

(type + Base
  (prop + label _ (= "base"))
  (fn-do + show _
    (print (str "base sees " label))
  )
)

(type + Child (is Base)
  (prop + label _ (= "child"))
  (fn-do + show _
    (print (str "child sees " label))
    (call (. super show))
  )
)
```

```output
child sees child
base sees child
```

### Super helper thunks do not enter sibling class method tables

```brix
(main
  (let calls (call CallsSuper))
  (call (. calls hook))
  (let tracked (call TrackedChild))
  (call (. tracked hook))
  (print (. calls count))
  (print (. tracked status))
  (print (. tracked count))
)

(type + Base
  (prop + count _ (= 0))
  (fn-do + hook _
    (+= count 1)
  )
)

(type + CallsSuper (is Base)
  (fn-do + hook _
    (call (. super hook))
  )
)

(type + TrackedChild (is Base)
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
1
changed
1
```

### Super call on numeric core type

```brix
(main
  (let number (call TaggedInt 7))
  (print (call (. number pad) 3))
)

(type + TaggedInt (alias tagged) (is I32)
  (fn-expr + pad _
    (param n Int)
    (str "tagged " (call (. super pad) n))
  )
)
```

```output
tagged 007
```

### Super call on tuple core type

```brix
(main
  (let pair (call OffsetPair 2 3))
  (print (call (. pair sum)))
)

(type + Pair (is (tuple (: x Int) (: y Int)))
  (fn-expr + sum Int
    (+ x y)
  )
)

(type + OffsetPair (is Pair)
  (fn-expr + sum Int
    (+ (call (. super sum)) 10)
  )
)
```

```output
15
```

### Super call on string core type

```brix
(main
  (let name (call LoudName "Ada"))
  (print (call (. name decorate)))
)

(type + Name (is String)
  (fn-expr + decorate String
    (str "<" this ">")
  )
)

(type + LoudName (is Name)
  (fn-expr + decorate String
    (str "LOUD " (call (. super decorate)))
  )
)
```

```output
LOUD <Ada>
```

### Super constructor initializes the same object

```brix
(main
  (let child (call Child 4))
  (print (. child base-value))
)

(type + Base
  (prop + base-value _ (= 0))
  (fn-do + This _
    (param n Int)
    (= base-value n)
    (print (str "base " (. this base-value)))
  )
)

(type + Child (is Base)
  (fn-do + This _
    (param n Int)
    (call super (+ n 1))
    (print (str "child " (. this base-value)))
  )
)
```

```output
base 5
child 5
5
```

### Super constructor uses base dispatch inside the constructor body

```brix
(main
  (call Child)
)

(type + Base
  (fn-do + This _
    (call report)
  )
  (fn-do + report _
    (print "base")
  )
)

(type + Child (is Base)
  (fn-do + This _
    (call super)
  )
  (fn-do + report _
    (print "child")
  )
)
```

```output
base
```

### Super constructor can target a default base constructor

```brix
(main
  (let child (call Child))
  (print (. child base-value))
)

(type + Base
  (prop + base-value _ (= 8))
)

(type + Child (is Base)
  (fn-do + This _
    (call super)
  )
)
```

```output
8
```

### Super constructor may only be called once

```brix crash
(main
  (call Child)
)

(type + Base
  (fn-do + This _
    (print "base")
  )
)

(type + Child (is Base)
  (fn-do + This _
    (call super)
    (call super)
  )
)
```

```output
Exception: super constructor call must be first
<no file:15>
        (call super)
        ^^^^^^^^^^^^
```

### Super constructor must be first executable statement

```brix crash
(main
  (call Child)
)

(type + Base
  (fn-do + This _
    (print "base")
  )
)

(type + Child (is Base)
  (fn-do + This _
    (print "before")
    (call super)
  )
)
```

```output
Exception: super constructor call must be first
<no file:15>
        (call super)
        ^^^^^^^^^^^^
```

### Super constructor only works in constructors

```brix crash
(main
  (call (. (call Child) run))
)

(type + Base)

(type + Child (is Base)
  (fn-expr + run _
    (call super)
  )
)
```

```output
Exception: super constructor call is only allowed in constructors
<no file:10>
        (call super)
        ^^^^^^^^^^^^
```

### Super requires an inherited base type

```brix crash
(main
  (call (. (call Thing) run))
)

(type + Thing
  (fn-expr + run _
    (call (. super run))
  )
)
```

```output
Exception: super requires a base type
<no file:8>
        (call (. super run))
                 ^^^^^
```

```brix
(main
  (let thing (call Thing))
  (call (. thing foo) 5)
  (call (. thing bar) 5)
)

(type + Thing
  (fn-do + foo _
    (param x _)
    (print (str "foo" x))
  )
  (fn-do + bar _
    (param x _)
    (print (str "bar" x))
  )
)
```

```output
foo5
bar5
```

### Infer return value of method call

```brix
(main
  (let thing (call Thing))
  (print (call (. thing foo)))
)

(type + Thing
  (fn-expr + foo _
    (call plop)
  )
  (fn-expr + plop _
    42
  )
)
```

```output
42
```

```brix
(main
  (print (call plop))
)

(type + Thing
  (fn-expr + foo _
    42
  )
)

(fn-do + plop _
  (let thing (call Thing))
  (return (call (. thing foo)))
)
```

```output
42
```

```brix crash
(main
  (let thing (call Thing))
  (call (. thing foo))
)

(type + Thing)
```

```output
Exception: Thing has no method "foo"
<no file:4>
      (call (. thing foo))
      ^^^^^^^^^^^^^^^^^^^^
```

```brix
(main
  (let thing (call Thing))
  (call (. thing foo))
)

(type + Thing
  (prop + a _ (= 42))
  (fn-do + foo _
    (print a)
  )
)
```

```output
42
```

### Generic default arguments are specialized by type parameters

```brix
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let parent (call Box))
  (let child (call Box))
  (let form (call Form))
  (blank)
  (call markup-append parent child)
  (call markup-append form parent)
  (blank)
  (print (of (. child ancestor) Box))
  (print (of (. child ancestor) Form))
)
```

```output
[Object core/tests/support/markup-fixtures:Box]
[Object core/tests/support/markup-fixtures:Form]
```

```brix
(main
  (let thing (call Thing))
  (call (. thing foo))
)

(type + Thing
  (prop + a _ (= 42))
  (fn-do + foo _
    (print (. this a))
  )
)
```

```output
42
```

```brix
(main
  (let thing (call Thing))
  (call (. thing foo))
)

(type + Thing
  (prop + a _ (= 42))
  (fn-do + foo _
    (call bar)
  )
  (fn-do + bar _
    (print a)
  )
)
```

```output
42
```

```brix
(main
  (let thing (call Thing))
  (call (. thing foo))
)

(type + Thing
  (prop + a _ (= 42))
  (fn-do + foo _
    (call (. this bar))
  )
  (fn-do + bar _
    (print a)
  )
)
```

```output
42
```

```brix
(main
  (let thing (call Thing))
  (call (. thing bling foo))
)

(type + Bling
  (prop + a _ (= 42))
  (fn-do + foo _
    (print a)
  )
)

(type + Thing
  (prop + bling _ (= (call Bling)))
)
```

```output
42
```

### Method takes precedence over static function

```brix
(main
  (let thing (call Thing))
  (call (. thing foo))
)

(fn-do + bar _
  (print 1)
)

(type + Thing
  (fn-do + foo _
    (call bar)
  )
  (fn-do + bar _
    (print 2)
  )
)
```

```output
2
```

### Static function and method with same name but different arguments can be called

```brix
(main
  (let thing (call Thing))
  (call (. thing foo))
)

(fn-do + bar _
  (param a _)
  (param b _)
  (print 1)
)

(type + Thing
  (fn-do + foo _
    (call bar 1 2)
    (call bar 0)
  )
  (fn-do + bar _
    (param x _)
    (print 2)
  )
)
```

```output
1
2
```

### Method with type variables

```brix
(main
  (let thing (call Thing))
  (call (. thing foo) 42.5)
)

(type + Thing
  (fn-do + (of foo T) _
    (param x T)
    (print x)
  )
)
```

```output
42.5
```

```brix
(main
  (let thing (call (of Thing Float)))
  (call (. thing foo) 5 42.5)
)

(type + (of Thing T)
  (fn-do + (of foo G) _
    (param x _)
    (param y G)
    (print (+ x y))
  )
)
```

```output
47.5
```

```brix
(main
  (let thing (call (of Thing Float)))
  (call (. thing foo) 42)
)

(type + (of Thing T)
  (fn-do + foo _
    (param x T)
    (print x)
  )
)
```

```output
42
```

```brix
(main
  (let thing (call Thing))
  (call (. thing foo) 24)
  (call (. thing foo) 24 26)
)

(type + Thing
  (fn-do + foo _
    (param a I32)
    (param b I32 (= 25))
    (print a)
    (print b)
  )
)
```

```output
24
25
24
26
```

### Class override may not add undeclared errors to a non-error base slot

```brix crash
(main
  (let base Base (call Child))
  (print (call (. base value)))
)

(type + Base
  (fn-expr + value Int
    1
  )
)

(type + Child (is Base)
  (fn-expr + value Int
    (throw (call Error "bad"))
  )
)
```

```output
Exception: "value" can fail with Error, but the method it overrides cannot; add "(?! Error)" to that method, or catch the error inside "value"
<no file:14>
      (fn-expr + value Int
      ^^^^^^^^^^^^^^^^^^^
Base method:
<no file:8>
      (fn-expr + value Int
```

### An override that states no errors carries its base's errors, even when it is called through its own class before anything else compiles the base method

```brix
(main
  (let child (call Child))
  (call (. child run)
    (block
      (on-error (: error Error)
        (print (str "caught " error))
      )
    )
  )
  (print (. child count))
)

(type + Base
  (prop + count _ (= 0))
  (fn-do + run _ (?! Error)
    (+= count 1)
  )
)

(type + Child (is Base)
  (fn-do + run _
    (+= count 2)
  )
)
```

```output
2
```

### Inherited methods called on subclasses still dispatch to overrides

```brix
(main
  (let nodes (call (of List Node)))
  (+= nodes (call Plain 1))
  (+= nodes (call Fancy 2))
  (+= nodes (call Plain 3))
  (for (in node nodes)
    (print (call (. node describe)))
  )
  (let fancy (call Fancy 5))
  (print (call (. fancy describe)))
  (print (call (. fancy total)))
)

(type + Node
  (prop + value Int (= 0))
  (fn-do + This _
    (param value _)
  )
  (fn-expr + label String
    "node"
  )
  (fn-expr + describe String
    (str (call label) " " value)
  )
  (fn-expr + total Int
    (+ value (call weight))
  )
  (fn-expr + weight Int
    1
  )
)

(type + Plain (is Node))

(type + Fancy (is Node)
  (fn-expr + label String
    "fancy"
  )
  (fn-expr + weight Int
    10
  )
)
```

```output
node 1
fancy 2
node 3
fancy 5
15
```

### Inherited generator getters recurse through base-typed children

```brix
(main
  (let root (call Group))
  (let inner (call Region))
  (+= (. inner kids) (call Leaf 2))
  (+= (. inner kids) (call Leaf 3))
  (+= (. root kids) (call Leaf 1))
  (+= (. root kids) inner)
  (+= (. root kids) (call Leaf 4))
  (for (in node (. root flattened))
    (print (. node value))
  )
)

(type + Node
  (prop + value Int (= 0))
  (prop + kids _ (= (call (of List Node))))
  (prop + is-transparent _ (= false))
  (fn-do + flattened (list Node) :get
    (for (in child kids)
      (if (. child is-transparent)
        (yield-all (. child flattened))
        (else
          (yield child)
        )
      )
    )
  )
)

(type + Group (is Node))

(type + Region (is Node)
  (prop + is-transparent _ (= true))
)

(type + Leaf (is Node)
  (fn-do + This _
    (param value _)
  )
)
```

```output
1
2
3
4
```

### A super constructor sets only the base properties its arguments determine.

```brix
(prop + count _ (= 0))
(blank)

(main
  (let c (call Child 3))
  (print (str "id " (. c id) " k " (. c k) " double " (. c double) " tag " (. c tag) " label " (. c label) " extra " (. c extra)))
  (print (str "count " count))
)

(fn-do + next-id Int
  (+= count 1)
  (return count)
)

(type + Base
  (prop + id _ (= (call next-id)))
  (prop + k Int (= 0))
  (prop + double _ (= (* k 2)))
  (prop + tag _ (= (str id ":" k)))
  (prop + label String (= "none"))
  (fn-do + This _
    (param k Int)
    (lazy label (str "base " k))
  )
)

(type + Child (is Base)
  (prop + extra Int (= 5))
  (fn-do + This _
    (param n Int)
    (call super (+ n 1))
    (= extra n)
  )
)
```

```output
id 1 k 4 double 8 tag 1:4 label base 4 extra 3
count 1
```

### Recursive generator getters dispatch virtually from the declaring class

```brix
(main
  (let root (call Node))
  (let inner (call Region))
  (+= (. inner kids) (call Leaf 2))
  (+= (. inner kids) (call Leaf 3))
  (+= (. root kids) (call Leaf 1))
  (+= (. root kids) inner)
  (+= (. root kids) (call Leaf 4))
  (for (in node (. root flattened))
    (print (. node value))
  )
)

(type + Node
  (prop + value Int (= 0))
  (prop + kids _ (= (call (of List Node))))
  (prop + is-transparent _ (= false))
  (fn-do + flattened (list Node) :get
    (for (in child kids)
      (if (. child is-transparent)
        (yield-all (. child flattened))
        (else
          (yield child)
        )
      )
    )
  )
)

(type + Region (is Node)
  (prop + is-transparent _ (= true))
)

(type + Leaf (is Node)
  (fn-do + This _
    (param value _)
  )
)
```

```output
1
2
3
4
```

### Inherited getters and setters reach subclass overrides through dynamic dispatch

```brix
(main
  (let shapes (call (of List Shape)))
  (+= shapes (call Shape))
  (+= shapes (call Circle))
  (for (in shape shapes)
    (print (. shape description))
    (call (. shape resize) 3)
    (print (. shape log))
  )
  (let circle (call Circle))
  (print (. circle description))
)

(type + Shape
  (prop + log _ (= ""))
  (prop + kind _ (=> "shape"))
  (prop + description _ (=> (str "a " kind)))
  (fn-do + size _ :set (param value Int)
    (= log (str "shape " value))
  )
  (fn-do + resize _
    (param value Int)
    (= size value)
  )
)

(type + Circle (is Shape)
  (prop + kind _ (=> "circle"))
  (fn-do + size _ :set (param value Int)
    (= log (str "circle " value))
  )
)
```

```output
a shape
shape 3
a circle
circle 3
a circle
```

### Inherited getters see This as their declaring class, like methods

```brix
(main
  (let circle (call Circle))
  (print (. circle made-kind))
  (print (call (. circle made-kind-method)))
)

(type + Shape
  (prop + kind _ (=> "shape"))
  (prop + made-kind _ (=> (. (call This) kind)))
  (fn-do + made-kind-method _
    (return (. (call This) kind))
  )
)

(type + Circle (is Shape)
  (prop + kind _ (=> "circle"))
)
```

```output
shape
shape
```

### Inherited getters reach subclass implementations of interface default getters

```brix
(main
  (let nodes (call (of List Node)))
  (+= nodes (call Node))
  (+= nodes (call Region))
  (for (in node nodes)
    (print (. node is-transparent-node))
  )
)

(type + Host (is Interface)
  (fn-expr + transparent _ :get
    false
  )
)

(type + Node
  (implements Host)
  (prop + is-transparent-node _ (=> transparent))
)

(type + Region (is Node)
  (fn-expr + transparent _ :get
    true
  )
)
```

```output
false
true
```

### Optional chaining of a method call on a receiver that is not optional is illegal.

```brix crash
(main
  (let thing (call Thing))
  (call (?. thing show))
)

(type + Thing
  (fn-do + show _
    (print "shown")
  )
)
```

```output
Exception: "thing" is Thing, which is never null, so it needs no "?"; write "(call (. thing show))"
<no file:4>
      (call (?. thing show))
      ^^^^^^^^^^^^^^^^^^^^^^
```

### Optional chaining of a method call on the receiver's base is illegal.

```brix crash
(main
  (call (. (call Kid) go))
)

(type + Thing
  (fn-do + show _
    (print "shown")
  )
)

(type + Kid (is Thing)
  (fn-do + go _
    (call (?. super show))
  )
)
```

```output
Exception: "super" is Thing, which is never null, so it needs no "?"; write "(call (. super show))"
<no file:14>
        (call (?. super show))
        ^^^^^^^^^^^^^^^^^^^^^^
```
