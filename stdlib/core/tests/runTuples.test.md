# Tuples

```brix
(main
  (print (call Tuple 1 2))
)
```

```output
(1, 2)
```

```brix
(main
  (print
    (call Tuple
      (block
        (arg 1)
        (arg "abc")
      )
    )
  )
)
```

```output
(1, abc)
```

```brix
(main
  (let a (call foo 5))
  (print a)
)

(fn-expr + foo _
  (param n _)
  (tuple n (* n 2))
)
```

```output
(5, 10)
```

```brix
(main
  (let a 1)
  (let b 2)
  (= (tuple a b) (call foo 5))
  (print a)
  (print b)
)

(fn-expr + foo _
  (param n _)
  (tuple n (* n 2))
)
```

```output
5
10
```

```brix
(main
  (let (tuple a b) (call foo 5))
  (print a)
  (print b)
)

(fn-expr + foo _
  (param n _)
  (tuple n (* n 2))
)
```

```output
5
10
```

```brix
(main
  (let (tuple a b) (call foo 5))
  (print a)
  (print b)
)

(fn-expr + foo _
  (param n _)
  (tuple a b)
  (where
    (lazy (tuple a b) (tuple 1 2))
  )
)
```

```output
1
2
```

```brix
(main
  (let a (call foo 5))
  (print ([] a 0))
  (print ([] a 1))
)

(fn-expr + foo _
  (param n _)
  (tuple n (* n 2))
)
```

```output
5
10
```

### Create new tuple by assignment

```brix
(main
  (let a (call foo 5))
  (let b (= ([] a 0) 3))
  (print b)
)

(fn-expr + foo _
  (param n _)
  (tuple n (* n 2))
)
```

```output
(3, 10)
```

### Create new tuple by assignment with casting

```brix
(main
  (let a (call foo 5.0))
  (let b (= ([] a 0) 3))
  (print b)
)

(fn-expr + foo _
  (param n _)
  (tuple n (* n 2))
)
```

```output
(3, 10)
```

### Create new tuple by tail assignment

```brix
(main
  (let a (call foo 5))
  (let b (= ([] a 2) 3))
  (print b)
)

(fn-expr + foo _
  (param n _)
  (tuple n (* n 2))
)
```

```output
(5, 10, 3)
```

### Create new tuple by increment

```brix
(main
  (let a (call foo 5))
  (let b (+= a 3))
  (print b)
)

(fn-expr + foo _
  (param n _)
  (tuple n (* n 2))
)
```

```output
(5, 10, 3)
```

### tuple truncation

```brix
(main
  (let a (tuple 1 2 3 4 5))
  (let b (= (. a length) 3))
  (print b)
)
```

```output
(1, 2, 3)
```

### tuple length

```brix crash
(main
  (let a (tuple 1 2 3 4 5))
  (let n 5)
  (let b (= (. a length) n))
  (print b)
)
```

```output
Exception: Value has no constant form
<no file:5>
      (let b (= (. a length) n))
                ^^^^^^^^^^^^
```

```brix
(main
  (let a (call foo 5))
  (print (. a length))
)

(fn-expr + foo _
  (param n _)
  (tuple n (* n 2))
)
```

```output
2
```

### nested tuple

```brix
(main
  (let a (tuple 1 (tuple 2 3) 4))
  (print (. a length))
  (print ([] a 0))
  (print ([] a 1))
  (print ([] a 2))
)
```

```output
3
1
(2, 3)
4
```

```brix crash
(main
  (let a (call foo 5))
  (print ([] a 2))
)

(fn-expr + foo _
  (param n _)
  (tuple n (* n 2))
)
```

```output
Exception: Index out of bounds
<no file:4>
      (print ([] a 2))
             ^^^^^^^^
```

```brix crash
(main
  (let a (call foo 5))
  (let n 0)
  (print ([] a n))
)

(fn-expr + foo _
  (param n _)
  (tuple n (* n 2))
)
```

```output
Exception: Value has no constant form
<no file:5>
      (print ([] a n))
             ^^^^^^^^
```

```brix
(main
  (let thing (call Thing))
  (print thing)
)

(type + Thing (is (tuple (: foo Int) (: bar Float))))
```

```output
(0, 0)
```

```brix
(main
  (let thing (call Thing 10 20))
  (print thing)
)

(type + Thing (is (tuple (: foo Int) (: bar Float))))
```

```output
(10, 20)
```

```brix
(main
  (let bling (call Bling 10 20))
  (print bling)
)

(type + Thing (is (tuple (: foo Int) (: bar Float))))

(type + Bling (is Thing))
```

```output
(10, 20)
```

```brix
(main
  (let bling (call Bling 10 20 "blah"))
  (print bling)
)

(type + Thing (is (tuple (: foo Int) (: bar Float))))

(type + Bling (is Thing)
  (prop _ baz String)
)
```

```output
(10, 20, blah)
```

### Tuple with base class from another module (to show it evaluates base in correct scope)

```brix
(import ./lib/foo)
(blank)

(main
  (let bling (call Bling))
  (print bling)
)

(type + Bling (is Thing)
  (prop _ baz Int)
)
```

```brix file=lib/foo.bx
(type + Thing (is Tuple)
  (prop _ foo Foo)
  (prop _ bar Float (= (call goo)))
)

(fn-expr _ goo I32
  42
)

(type + Foo (is Tuple)
  (prop _ bar Int (= (call goo)))
)
```

```output
((42), 42, 0)
```

```brix
(main
  (let thing (call Thing 10))
  (print thing)
)

(type + Thing (is (tuple (: foo Int) (: bar Float))))
```

```output
(10, 0)
```

```brix
(main
  (let thing (call Thing 10))
  (print thing)
)

(type + Thing (is Tuple)
  (prop _ foo Int)
  (prop _ bar Float (= 20))
)
```

```output
(10, 20)
```

### Refer to other fields in default values

```brix
(main
  (let thing (call Thing 10))
  (print thing)
)

(type + Thing (is Tuple)
  (prop _ foo Int)
  (prop _ zab Float (= 20))
  (prop _ bar Float (= (* foo zab)))
)
```

```output
(10, 20, 200)
```

### Type inference of default values for fields

```brix
(main
  (let thing (call Thing 10))
  (print thing)
)

(type + Thing (is Tuple)
  (prop _ foo Int)
  (prop _ zab Float (= 20))
  (prop _ bar _ (= foo))
)
```

```output
(10, 20, 10)
```

### Refer to other fields in default values of base tuple

```brix
(main
  (let thing (call Bling))
  (print thing)
)

(type + Thing (is Tuple)
  (prop _ foo Int)
  (prop _ bar Float (= 20))
)

(type + Bling (is Thing)
  (prop _ baz Float (= bar))
)
```

```output
(0, 20, 20)
```

```brix
(main
  (let thing (call Thing))
  (print thing)
)

(type + Thing (is Tuple)
  (prop _ bar Float (= 20))
  (prop _ baz Float (= bar))
)

(type + Bling (is Thing))
```

```output
(20, 20)
```

### Refer to other fields out of order in default values

```brix
(main
  (let thing (call Thing))
  (print thing)
)

(type + Thing (is Tuple)
  (prop _ zab Float (= bar))
  (prop _ bar Float (= 20))
)
```

```output
(20, 20)
```

### Circular reference in fields

```brix crash
(main
  (let thing (call Thing))
  (print thing)
)

(type + Thing (is Tuple)
  (prop _ zab Float (= bar))
  (prop _ bar Float (= zab))
)
```

```output
Exception: "zab" depends on its own value, directly or through the values it reads; break the cycle
<no file:8>
      (prop _ zab Float (= bar))
      ^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Parameterized tuple type constructor

```brix
(main
  (let thing (call (of Thing Int) 10 20.0))
  (print thing)
)

(type + (of Thing T) (is (tuple (: foo T) (: bar Float))))
```

```output
(10, 20)
```

### Parameterized tuple type inferred from constructor arguments

```brix
(main
  (let thing (call Thing 10 20.0))
  (print thing)
)

(type + (of Thing T) (is (tuple (: foo T) (: bar Float))))
```

```output
(10, 20)
```

### Parameterized tuple type with base class constructor

```brix
(main
  (let thing (call (of Thing Int) 10 20 30))
  (print thing)
)

(type + (of Bling T) (is (tuple (: foo T) (: bar Float))))

(type + (of Thing T) (is (of Bling T))
  (prop + baz Float)
)
```

```output
(10, 20, 30)
```

### Parameterized tuple type with base class inferred from constructor arguments

```brix
(main
  (let thing (call Thing 10 20 30))
  (print thing)
)

(blank)
(type-line + (of Bling T) (is (tuple (: foo T) (: bar Float))))

(type + (of Thing T) (is (of Bling T))
  (prop + baz T)
)
```

```output
(10, 20, 30)
```

### Able to infer type if parameter is resolved in base type

```brix
(main
  (let thing (call Thing 10 20 30))
  (print thing)
)

(type + (of Bling T) (is (tuple (: foo T) (: bar Float))))

(type + (of Thing T) (is (of Bling T))
  (prop + baz Float)
)
```

```output
(10, 20, 30)
```

### Tuple constructor re-routed to builtin constructor using "This"

```brix
(main
  (let thing (call Thing 10.5))
  (print thing)
)

(type + Thing (is (tuple (: foo Int) (: bar Float)))
  (fn-do + This _
    (param xx Float)
    (return (call This 1 xx))
  )
)
```

```output
(1, 10.5)
```

```brix
(main
  (let thing (call Thing 10.5 ...))
  (print (call (of thing Float)))
)

(type + (of Thing T) (is (tuple (: foo Int) (: bar T)))
  (fn-do + This _
    (param n Float)
    (return (call This 1 n))
  )
)
```

```output
(1, 10.5)
```

```brix
(main
  (let thing (call Thing 10 ...))
  (print (call (of thing Float)))
)

(type + (of Thing T) (is (tuple (: foo Int) (: bar T))))
```

```output
(10, 0)
```

### Replace tuple property (creates a new tuple)

```brix
(main
  (let thing (call Thing))
  (let thing2 (= (. thing foo) 99))
  (print thing)
  (print thing2)
)

(type + Thing (is Tuple)
  (prop + foo I32)
  (prop + bar I32)
)
```

```output
(0, 0)
(99, 0)
```

```brix crash
(main
  (let foo (call Foo))
  (print foo)
  (let bar (call Bar))
  (print bar)
)

(type + Foo (is (tuple (: bar Int) (: baz Int))))

(type + Bar (is Foo)
  (prop + bar Int)
)
```

```output
Exception: "bar" is inherited from Foo, so it keeps the type it has there and cannot state its own; remove "Int"
<no file:12>
      (prop + bar Int)
      ^^^^^^^^^^^^^^^^
```

### Casting tuple

```brix
(main
  (let n (as (tuple 1 2) Pair))
  (print n)
  (print (type-of n))
)

(type + Pair (is (tuple (: a Int) (: b Float))))
```

```output
(1, 2)
[TypeInfo Pair]
```

### Casting tuple to parameterized tuple

```brix
(main
  (let n (as (tuple 1 2) (of Pair Int Int)))
  (print n)
  (print (type-of n))
)

(type + (of Pair T1 T2) (is (tuple (: a T1) (: b T2))))
```

```output
(1, 2)
[TypeInfo Pair'I64'I64]
```

### Casting tuple to parameterized tuple with parameters inferred

```brix
(main
  (let n (as (tuple 1 2) Pair))
  (print n)
  (print (type-of n))
)

(type + (of Pair T1 T2) (is (tuple (: a T1) (: b T2))))
```

```output
(1, 2)
[TypeInfo Pair'I64'I64]
```

```brix
(main
  (print (call foo (tuple 1 2)))
)

(type + (of Pair T1 T2) (is (tuple (: a T1) (: b T2))))

(fn-expr + (of foo T1 T2) _
  (param pair (of Pair T1 T2))
  (tuple (. pair a) (. pair b))
)
```

```output
(1, 2)
```

### Implicit tuple constructors respect named arguments and default omitted fields

```brix
(main
  (let positional (call Thing 10))
  (let named (call Thing (:bar 20)))
  (print positional)
  (print named)
)

(type + Thing (is (tuple (: foo Int) (: bar Int) (: baz Int))))
```

```output
(10, 0, 0)
(0, 20, 0)
```

### Implicit tuple constructors accept trailing boolean ability arguments

```brix
(main
  (let thing (call Thing 7 (:does-complete true)))
  (print thing)
)

(type + Thing (is (tuple (: index Int) (: does-complete Bool))))
```

```output
(7, true)
```

### Override increment function

```brix
(main
  (let thing (call Thing 20))
  (print (+= thing 3))
)

(type + Thing (is Tuple)
  (prop + foo Int)
  (fn-expr + (infix this += (param other ''T)) _
    (call This (* foo other))
  )
)
```

```output
(60)
```

```brix
(main
  (let thing (call Thing 20))
  (print (+ thing 3))
)

(type + Thing (is Tuple)
  (prop + foo Int)
  (fn-expr + (infix this + (param other ''T)) _
    (call This (* foo other))
  )
)
```

```output
(60)
```

```brix
(main
  (let thing (call Thing 20))
  (print (+= thing 3))
)

(type + Thing (is Tuple)
  (prop + foo Int)
  (fn-expr + (infix this + (param other ''T)) _
    (call This (* foo other))
  )
)
```

```output
(60)
```

```brix
(main
  (let foos (list (call Foo "hi" "you") (call Foo "aa" "vv") (call Foo "dd" "cc")))
  (print (is-in (tuple "dd" "cc") foos))
  (print (is-in (tuple "dfd" "cc") foos))
  (print (not-in (tuple "dd" "cc") foos))
  (print (not-in (tuple "dfd" "cc") foos))
)

(type + Foo (is (tuple (: a String) (: b String))))
```

```output
true
false
false
true
```

```brix
(main
  (print (== (call Foo "foo" 42) (call Foo "foo" 42)))
  (print (!= (call Foo "foo" 42) (call Foo "foo" 42)))
  (print (== (call Foo "foo" 43) (call Foo "foo" 42)))
  (print (!= (call Foo "foo" 43) (call Foo "foo" 42)))
)

(type + Foo (is (tuple (: a String) (: b Int))))
```

```output
true
false
false
true
```

```brix
(main
  (print (== (call RecursiveComputedTuple 1) (call RecursiveComputedTuple 1)))
)

(type + RecursiveComputedTuple (is Tuple)
  (prop + value Int)
  (fn-expr + child RecursiveComputedTuple :get
    (call RecursiveComputedTuple value)
  )
)
```

```output
true
```

### Tests that comparing named tuples against each other does not use structural comparison

```brix
(main
  (print
    (match (call A "hi" 1)
      (case B
        "B"
      )
      (case A
        "A"
      )
    )
  )
)

(type + A (is (tuple (: a String) (: b Int))))

(type + B (is (tuple (: a String) (: b Int))))
```

```output
A
```

### Constructor-selected backing parameters produce exact tuple types with distinct layouts.

```brix
(main
  (let managed (of ConstructorLayoutBox U32 (of ManagedLayoutBacking U32)) (call (of ConstructorLayoutBox U32)))
  (let native (of ConstructorLayoutBox U32 (of ArenaLayoutBacking U32)) (call (of ConstructorLayoutBox U32) (call LayoutArenaToken) true))
  (print (type-of managed))
  (print (type-of native))
  (print (type-of (. managed storage)))
  (print (type-of (. native storage)))
  (print (. managed storage label))
  (print (. native storage generation))
)

(type + LayoutArenaToken)

(type + (of ManagedLayoutBacking Value) (is Tuple)
  (prop + value Value)
  (prop + label String (= "managed"))
)

(type + (of ArenaLayoutBacking Value) (is Tuple)
  (prop + value Value)
  (prop + generation U64 (= 7u64))
)

(type + (of ConstructorLayoutBox Value Backing) (is Tuple)
  (constraint = Backing (of ManagedLayoutBacking Value))
  (prop + storage Backing)
  (fn-do + This _
    (= storage (call Backing))
  )
  (fn-do + This _
    (param token LayoutArenaToken)
    (param marker Bool)
    (constraint = Backing (of ArenaLayoutBacking Value))
    (= storage (call Backing))
  )
)
```

```output
[TypeInfo ConstructorLayoutBox'U32'ManagedLayoutBacking'U32]
[TypeInfo ConstructorLayoutBox'U32'ArenaLayoutBacking'U32]
[TypeInfo ManagedLayoutBacking'U32]
[TypeInfo ArenaLayoutBacking'U32]
managed
7
```

### Tuple constructors retain property values returned by function calls.

```brix
(main
  (let box (call CalledValueBox 41))
  (print (. box value))
)

(type + CalledValueBox (is Tuple)
  (prop + value Int)
  (fn-do + This _
    (param input Int)
    (= value (call make-called-value input))
  )
)

(fn-expr + make-called-value Int
  (param input Int)
  (+ input 1)
)
```

```output
42
```

```brix
(main
  (let derived (call NarrowPointSource))
  (let base PointSource derived)
  (print (. derived point))
  (print (. base point))
  (let tiny (call TinyPointSource))
  (let middle NarrowPointSource tiny)
  (let root PointSource tiny)
  (print (. tiny point))
  (print (. middle point))
  (print (. root point))
)

(type + PointSource
  (prop + point _ (=> (tuple 0 0)))
)

(type + NarrowPointSource (is PointSource)
  (prop + point _ (=> (tuple 12i16 27i16)))
)

(type + TinyPointSource (is NarrowPointSource)
  (prop + point _ (=> (tuple -3i8 4i8)))
)
```

```output
(12, 27)
(12, 27)
(-3, 4)
(-3, 4)
(-3, 4)
```

### A shorthand tuple with one named field declares a tuple, not a subtype of the field type.

```brix
(main
  (let single (call Solo 7))
  (print (. single amount))
  (print single)
)

(type + Solo (is (tuple (: amount I32))))
```

```output
7
(7)
```

### A one-field tuple's own constructor wins over its conversion when the argument is the field type.

```brix
(main
  (let wrapped Wrapped "hi")
  (print wrapped)
  (print (. wrapped text))
)

(type + Wrapped (is (tuple (: text String)))
  (fn-from-expr + (: other String)
    (call Wrapped other)
  )
)
```

```output
(hi)
hi
```

### A tuple property assignment whose new tuple is discarded is an error.

```brix crash
(main
  (let record (call Record 17))
  (= (. record flag) true)
)

(type + Record (is Tuple)
  (prop + size I32)
  (prop + flag _ (= false))
)
```

```output
Exception: Assigning flag creates a new Record tuple that is discarded; write record = record.flag = value
<no file:4>
      (= (. record flag) true)
      ^^^^^^^^^^^^^^^^^^^^^^^^
```

### A tuple property assignment whose new tuple is discarded is an error where its value has statements of its own.

```brix crash
(main
  (let record (call Record 17))
  (= (. record flag)
    (match (. record size)
      (case 17
        (let big true)
        big
      )
      (default
        false
      )
    )
  )
)

(type + Record (is Tuple)
  (prop + size I32)
  (prop + flag _ (= false))
)
```

```output
Exception: Assigning flag creates a new Record tuple that is discarded; write record = record.flag = value
<no file:4>
      (= (. record flag)
      ^^^^^^^^^^^^^^^^^
```

### The error for a discarded tuple names the path its owner is read through.

```brix crash
(main
  (let outer (call Outer (call Record 17)))
  (= (. outer record flag) true)
)

(type + Record (is Tuple)
  (prop + size I32)
  (prop + flag _ (= false))
)

(type + Outer (is Tuple)
  (prop + record Record)
)
```

```output
Exception: Assigning flag creates a new Record tuple that is discarded; write outer.record = outer.record.flag = value
<no file:4>
      (= (. outer record flag) true)
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A class method cannot update a tuple property in place.

```brix crash
(main
  (call (. (call Holder) touch))
)

(type + Record (is Tuple)
  (prop + size I32)
)

(type + Holder
  (prop + record _ (= (call Record 3)))
  (fn-do + touch _
    (= (. record size) 9)
  )
)
```

```output
Exception: Assigning size creates a new Record tuple that is discarded; write record = record.size = value
<no file:13>
        (= (. record size) 9)
        ^^^^^^^^^^^^^^^^^^^^^
```

### Assigning the updated tuple back keeps the update.

```brix
(main
  (let record (call Record 17))
  (= record (= (. record flag) true))
  (print record)
  (let copy (= (. record size) 3))
  (print copy)
)

(type + Record (is Tuple)
  (prop + size I32)
  (prop + flag _ (= false))
)
```

```output
(17, true)
(3, true)
```

### An expression function cannot return a tuple property assignment.

```brix crash
(main
  (print (call flagged (call Record 3)))
)

(type + Record (is Tuple)
  (prop + size I32)
  (prop + flag _ (= false))
)

(fn-expr + flagged Record
  (param record Record)
  (= (. record flag) true)
)
```

```output
Exception: (= …) takes (| …) here, and (. …) stands in its place. Its shape is (= (| pattern:Pattern+) value:Expr).
<no file:13>
      (= (. record flag) true)
         ^
```

### A local that holds a tuple is copied where it is assigned, in a loop and under a condition.

```brix
(main
  (let a (tuple 1 "one"))
  (let b a)
  (= b (tuple 2 "two"))
  (print (tuple ([] a 0) ([] a 1)))
  (print (tuple ([] b 0) ([] b 1)))
  (let sum 0)
  (let i 0)
  (while (< i 3)
    (let step (tuple i (* i 2)))
    (if (> ([] step 1) 1)
      (= a (tuple (+ ([] step 0) ([] step 1)) "more"))
    )
    (+= sum (+ ([] step 0) ([] step 1)))
    (+= i 1)
  )
  (print sum)
  (print a)
)
```

```output
(1, one)
(2, two)
9
(6, more)
```

### A field of a tuple is read by a constant index, by name and through a path, and `length` counts its fields.

```brix
(main
  (let nested (tuple (tuple 1 2) (tuple 3 (tuple 4 "deep"))))
  (print ([] ([] nested 0) 1))
  (print ([] ([] ([] nested 1) 1) 1))
  (print (. nested length))
  (print (. ([] nested 1) length))
  (let named (call Outer (call Inner 5 6) 7))
  (print (. named inner b))
  (print ([] ([] named 0) 0))
  (print (. named length))
)

(type + Inner (is (tuple (: a Int) (: b Int))))

(type + Outer (is (tuple (: inner Inner) (: last Int))))
```

```output
2
deep
2
2
6
5
2
```

### An assignment to a field of a tuple gives a new tuple and leaves the old one as it was.

```brix
(main
  (let a (tuple 5 10))
  (let b (= ([] a 0) 3))
  (print a)
  (print b)
  (let r (call Record 17))
  (let s (= (. r flag) true))
  (print r)
  (print s)
  (= r (= (. r size) 4))
  (print r)
)

(type + Record (is Tuple)
  (prop + size I32)
  (prop + flag _ (= false))
)
```

```output
(5, 10)
(3, 10)
(17, false)
(17, true)
(4, false)
```

### A tuple is extended by `+=` and by an assignment one past its last field, and truncated by an assignment to its length.

```brix
(main
  (let a (tuple 5 "ten"))
  (let b (+= a 3.5))
  (print b)
  (print (. b length))
  (let c (= ([] a 2) true))
  (print c)
  (let d (= (. (tuple 1 "two" 3.5 true 5) length) 2))
  (print d)
  (print (. d length))
  (print a)
)
```

```output
(5, ten, 3.5)
3
(5, ten, true)
(1, two)
2
(5, ten)
```

### A tuple is taken apart into new locals and into old ones, and two locals are swapped.

```brix
(main
  (let (tuple a b) (tuple 1 "one"))
  (print a)
  (print b)
  (let x 1)
  (let y 2)
  (= (tuple x y) (tuple y x))
  (print (tuple x y))
  (= (tuple x y) (call pair x))
  (print (tuple x y))
  (let (tuple first second third) (tuple 1.5 (tuple 2 3) "last"))
  (print first)
  (print second)
  (print third)
)

(fn-expr + pair (tuple Int Int)
  (param n Int)
  (tuple (* n 10) (* n 100))
)
```

```output
1
one
(2, 1)
(20, 200)
1.5
(2, 3)
last
```

### A tuple is taken apart into globals and task-locals that exist.

```brix
(main
  (let pair (tuple 1u32 2u16))
  (= (tuple ga gb) pair)
  (= (tuple ta tb) pair)
  (print (+ ga gb))
  (print (tuple ta tb))
)

(blank)
(prop + ga _ (= 0u32))
(prop + gb _ (= 0u16))
(prop + ta U32 @task (= 0))
(prop + tb U16 @task (= 0))
```

```output
3
(1, 2)
```

### A named tuple is made by its fields in order, by name, and with none; a field left out takes its default.

```brix
(main
  (print (call Thing 1))
  (print (call Thing 1 2.5))
  (print (call Thing 1 2.5 "three"))
  (print (call Thing (:bar 4.5)))
  (print (call Thing (:baz "six") (:foo 5)))
  (print (call Thing))
  (print (call Pair 1 "x"))
  (print (call Wide 7))
)

(type + Thing (is Tuple)
  (prop + foo Int)
  (prop + bar Float (= 20))
  (prop + baz _ (= "none"))
)

(type + (of Pair T1 T2) (is Tuple)
  (prop + a T1)
  (prop + b T2)
)

(type + Wide (is Thing)
  (prop + last _ (= 9))
)
```

```output
(1, 20, none)
(1, 2.5, none)
(1, 2.5, three)
(0, 4.5, none)
(5, 20, six)
(0, 20, none)
(1, x)
(7, 20, none, 9)
```

### A tuple is passed and returned in registers when it is small or all floats, and through memory when it is not.

```brix
(main
  (print (call small (tuple 1 2)))
  (print (call words (tuple 1 2 3)))
  (print (call floats (tuple 1.5 2.5 3.5 4.5)))
  (print (call singles (tuple 1.5f32 2.5f32)))
  (print (call mixed (tuple 1 "two" 3.5)))
  (print (call large (tuple 1 2 3 4 5)))
  (print (call many 1 (tuple 2 3) 4 (tuple 5 6) 7 (tuple 8 9) 10 (tuple 11 12) (tuple 13 14) 15))
)

(fn-expr + small (tuple I32 I32)
  (param v (tuple I32 I32))
  (tuple ([] v 1) ([] v 0))
)

(fn-expr + words (tuple I64 I32 I8)
  (param v (tuple I8 I32 I64))
  (tuple ([] v 2) ([] v 1) ([] v 0))
)

(fn-expr + floats (tuple F64 F64 F64 F64)
  (param v (tuple F64 F64 F64 F64))
  (tuple ([] v 3) ([] v 2) ([] v 1) ([] v 0))
)

(fn-expr + singles (tuple F32 F32)
  (param v (tuple F32 F32))
  (tuple ([] v 1) ([] v 0))
)

(fn-expr + mixed (tuple F64 String Int)
  (param v (tuple Int String F64))
  (tuple ([] v 2) ([] v 1) ([] v 0))
)

(fn-expr + large (tuple Int Int Int Int Int)
  (param v (tuple Int Int Int Int Int))
  (tuple ([] v 4) ([] v 3) ([] v 2) ([] v 1) ([] v 0))
)

(fn-expr + many I64
  (param a I64)
  (param b (tuple I64 I64))
  (param c I64)
  (param d (tuple I64 I64))
  (param e I64)
  (param f (tuple I64 I64))
  (param g I64)
  (param h (tuple I64 I64))
  (param i (tuple I64 I64))
  (param j I64)
  (+ a (* ([] b 0) 2) (* ([] b 1) 3) (* c 4) (* ([] d 0) 5) (* ([] d 1) 6) (* e 7) (* ([] f 0) 8) (* ([] f 1) 9) (* g 10) (* ([] h 0) 11) (* ([] h 1) 12) (* ([] i 0) 13) (* ([] i 1) 14) (* j 15))
)
```

```output
(2, 1)
(3, 2, 1)
(4.5, 3.5, 2.5, 1.5)
(2.5, 1.5)
(3.5, two, 1)
(5, 4, 3, 2, 1)
1240
```

### A tuple is cast to a named tuple field by field, where it is written and where a type is declared.

```brix
(main
  (let n (as (tuple 1 2) Pair))
  (print n)
  (print (. n b))
  (let p Pair (tuple 3 4))
  (print p)
  (print (call wide (tuple 1 (tuple 2 3))))
  (print (call make))
)

(type + Pair (is (tuple (: a Int) (: b Float))))

(fn-expr + make Pair
  (tuple 5 6)
)

(fn-expr + wide Float
  (param t (tuple Float (tuple Int Float)))
  (+ (/ ([] t 0) 2) ([] ([] t 1) 1))
)
```

```output
(1, 2)
2
(3, 4)
3.5
(5, 6)
```

### A tuple is made text field by field, each by its own conversion, and a tuple inside it the same way.

```brix
(main
  (let t (tuple 1 "two" 3.5 true 'c'))
  (print t)
  (print (str "<" t ">"))
  (let nested (tuple t (tuple 1 (tuple 2 "three")) (call Named 2 (call Named2 "in"))))
  (print nested)
  (print (tuple 1 "two" (tuple 3 4)))
)

(type + Named (is (tuple (: count Int) (: inner Named2))))

(type + Named2 (is Tuple)
  (prop + text String)
  (fn-to-expr + String
    (str "[" text "]")
  )
)
```

```output
(1, two, 3.5, true, c)
<(1, two, 3.5, true, c)>
((1, two, 3.5, true, c), (1, (2, three)), (2, [in]))
(1, two, (3, 4))
```

### Two tuples are equal when every field is, compared by the machine or by the `==` the field declares.

```brix
(main
  (let a (tuple 1 "x" (tuple 2.5 true)))
  (let b (tuple 1 "x" (tuple 2.5 true)))
  (let c (tuple 1 "x" (tuple 2.5 false)))
  (let d (tuple 1 "y" (tuple 2.5 true)))
  (print (== a b))
  (print (!= a b))
  (print (== a c))
  (print (!= a c))
  (print (== a d))
  (print (== (call Loose 1 100) (call Loose 1 200)))
  (print (== (tuple (call Loose 1 100) 5) (tuple (call Loose 1 200) 5)))
  (print (!= (tuple (call Loose 1 100) 5) (tuple (call Loose 2 100) 5)))
)

(type + Loose (is Tuple)
  (prop + key Int)
  (prop + note Int)
  (fn-expr + (infix this == (param other _)) _
    (== key (. other key))
  )
)
```

```output
true
false
false
true
false
true
true
true
```

### An operand that names a tuple is copied before a later operand that changes it.

```brix
(prop + g _ (= (tuple 1 2)))
(blank)

(main
  (print (call both g (call change)))
  (print g)
  (= g (tuple 1 2))
  (print (tuple (== g (tuple 1 2)) (call change-flag)))
  (print (tuple g (call change)))
  (print g)
)

(fn-expr + both String
  (param first (tuple Int Int))
  (param second (tuple Int Int))
  (str first " " second)
)

(fn-do + change (tuple Int Int)
  (= g (tuple (+ ([] g 0) 10) (+ ([] g 1) 10)))
  (return g)
)

(fn-do + change-flag Bool
  (= g (tuple 0 0))
  (return true)
)
```

```output
(1, 2) (11, 12)
(11, 12)
(true, true)
((0, 0), (10, 10))
(10, 10)
```

### `Tuple` called with values is the tuple of them.

```brix
(main
  (let t (call Tuple 1 "two" 3.5))
  (print t)
  (print ([] t 1))
  (print (== (call Tuple 1 2) (tuple 1 2)))
)
```

```output
(1, two, 3.5)
two
true
```

### Two tuples whose fields have other names are not equal.

```brix
(main
  (print (== (call Named 1 2) (tuple 1 2)))
  (print (== (tuple 1 2) (call Named 1 2)))
  (print (== (call Named 1 2) (call Other 1 2)))
  (print (!= (call Named 1 2) (call Other 1 2)))
  (print (== (call Named 1 2) (call Named 1 2)))
  (print (== (tuple 1 2) (tuple 1 2)))
)

(type + Named (is (tuple (: a Int) (: b Int))))

(type + Other (is (tuple (: c Int) (: d Int))))
```

```output
false
false
false
true
true
true
```

### A field left out of a named tuple takes its class's value; a tuple with no name has no bits set.

```brix
(main
  (print (call Holder 1))
  (print (call Holder))
)

(type + Part (is Struct)
  (prop + x I32 (= 7))
  (prop + y I32)
)

(type + Holder (is Tuple)
  (prop + n Int)
  (prop + part Part)
  (prop + loose (tuple Part Int))
  (prop + made Part (= (call Part 1 2)))
)
```

```output
(1, (7, 0), ((0, 0), 0), (1, 2))
(0, (7, 0), ((0, 0), 0), (1, 2))
```

### A tuple's constructor assigns its fields.

```brix
(main
  (let t (call Trip 1))
  (print (+ (. t a) (. t b) (. t c)))
  (print (. (call Trip 4) c))
  (print t)
)

(type + Trip (is Tuple)
  (prop + a Int)
  (prop + b Int)
  (prop + c Int)
  (fn-do + This _
    (param n Int)
    (= a n)
    (= b (+ n 1))
    (= c (+ b 1))
  )
)
```

```output
6
6
(1, 2, 3)
```

### What the function of an assigning operator answers for a tuple is stored where the tuple is held.

```brix
(main
  (let pair (call Pair 1 2))
  (+= pair 3)
  (print (. pair a))
  (print (. pair b))
  (print (. (+= pair 1) a))
  (print (. pair b))
  (let holder (call Holder))
  (+= (. holder held) 5)
  (print (. holder held a))
  (print (. holder held b))
)

(type + Pair (is Tuple)
  (prop + a I32)
  (prop + b I32)
  (fn-expr + (infix this += (param other I32)) _
    (call This (+ a other) (+ b other))
  )
)

(type + Holder
  (prop + held _ (= (call Pair 1 1)))
)
```

```output
4
5
5
6
6
6
```

### A tuple called with one value of its only field's type is constructed from it, and any other single value goes through the function that converts from it.

```brix
(main
  (let made (call Wrapped "made"))
  (print (. made text))
  (let cast Wrapped "cast")
  (print (. cast text))
  (let other (call Count "four"))
  (print (. other n))
)

(type + Wrapped (is (tuple (: text String)))
  (fn-from-expr + (: other String)
    (call Wrapped (str other "!"))
  )
)

(type + Count (is (tuple (: n Int)))
  (fn-from-expr + (: other String)
    (call Count (. other size))
  )
)
```

```output
made
cast!
4
```

### A tuple or struct declared with no value has the first values of its fields.

```brix
(main
  (let thing Thing)
  (print (. thing a))
  (let pair Pair)
  (print pair)
  (let set Preset)
  (print set)
  (let deep Deep)
  (print deep)
)

(type + Thing (is (tuple (: a Int) (: b (of CPointer Char8)))))

(type + Pair (is (tuple (: x Int) (: y Float))))

(type + Preset (is Tuple)
  (prop _ x Int (= 3))
  (prop _ y Float (= 1.5))
)

(type + Deep (is (tuple (: pair Pair) (: name String) (: flag Bool))))
```

```output
0
(0, 0)
(3, 1.5)
((0, 0), , false)
```

### A field's first value reads other fields, in whatever order they are declared, with the arguments the call gave.

```brix
(main
  (print (call Thing 10))
  (print (call Thing 10 2))
  (print (call Thing 10 2 3))
  (print (call Later))
  (print (call Later 5))
  (print (call Child))
  (print (. (call Holder) inner))
  (print (call Chain))
  (print (call Chain 5))
  (print (call Made 3))
  (print (. (call Made 3) wide))
  (let local Later)
  (print local)
)

(type + Thing (is Tuple)
  (prop _ foo Int)
  (prop _ zab Float (= 20))
  (prop _ bar Float (= (* foo zab)))
)

(type + Later (is Struct)
  (prop + zab I32 (= bar))
  (prop + bar I32 (= 20))
)

(type + Base (is Struct)
  (prop + foo I32 (= 1))
  (prop + bar I32 (= 2))
)

(type + Child (is Base)
  (prop + baz I32 (= (+ bar 1)))
)

(type + Holder
  (prop + scale _ (= 3))
  (prop + inner _ (= (call Thing scale)))
)

(type + Chain (is Tuple)
  (prop _ a Int (= (+ b 1)))
  (prop _ b Int (= (+ c 1)))
  (prop _ c Int (= 1))
  (prop _ d (tuple Int Int) (= (tuple a c)))
)

(type + Made (is Struct)
  (prop + n I32)
  (prop + wide I64 (= (* n 2)))
  (prop + text _ (= (str "n" n)))
  (fn-do + This _
    (param n I32)
    (+= n 1)
  )
)
```

```output
(10, 20, 200)
(10, 2, 20)
(10, 2, 3)
(20, 20)
(5, 20)
(1, 2, 3)
(3, 20, 60)
(3, 2, 1, (3, 1))
(5, 2, 1, (5, 1))
(3, 6, n3)
6
(20, 20)
```

### An assignment to an argument that is a tuple changes the function's own copy.

```brix
(main
  (let original (call Pair 1 2))
  (print (call rebind original))
  (print original)
  (print (call keep original true))
  (print (call keep original false))
  (print original)
  (print (call grow original))
  (print original)
  (let holder (call Holder))
  (print (call (. holder swap) original))
  (print original)
)

(type + Pair (is (tuple (: a I64) (: b I64))))

(fn-do + rebind I64
  (param value Pair)
  (= value (call Pair 9 8))
  (return (+ (. value a) (. value b)))
)

(fn-do + keep I64
  (param value Pair)
  (param replace Bool)
  (if replace
    (= value (call Pair 10 9))
  )
  (return (+ (. value a) (. value b)))
)

(fn-do + grow I64
  (param value Pair)
  (for (in index (..< 0 3))
    (= value (call Pair (+ (. value a) 1) (. value b)))
  )
  (return (. value a))
)

(type + Holder
  (fn-do + swap Pair
    (param value Pair)
    (= value (call Pair (. value b) (. value a)))
    (return value)
  )
)
```

```output
17
(1, 2)
19
3
(1, 2)
4
(1, 2)
(2, 1)
(1, 2)
```

### A field's value is evaluated for a tuple and for a tuple derived from it.

```brix
(main
  (let plain (call Thing))
  (print plain)
  (let bling (call Bling))
  (print bling)
)

(type + Thing (is Tuple)
  (prop _ bar Int (= (call goo)))
)

(type + Bling (is Thing)
  (prop _ baz Int)
)

(fn-expr _ goo _
  42
)
```

```output
(42)
(42, 0)
```

### A tuple with no name declared with no value has no bits set, whatever its fields' types write for theirs.

```brix
(main
  (let o (tuple Int S))
  (print o)
  (let p (tuple S S))
  (print p)
  (let q (tuple Int T))
  (print q)
  (let r (tuple Int P))
  (print r)
  (print (str "[" (. ([] r 1) y) "]"))
  (let named S)
  (print named)
)

(type + S (is Struct)
  (prop + a I32 (= 3))
  (prop + b F64)
)

(type + T (is Tuple)
  (prop + a I32 (= 3))
  (prop + b F64)
)

(type + P (is (tuple (: x Int) (: y String))))
```

```output
(0, (0, 0))
((0, 0), (0, 0))
(0, (0, 0))
(0, (0, ))
[]
(3, 0)
```

### A property that holds a tuple and has no value is made by the tuple's constructor, with the defaults it declares

```brix
(import /core/arena)
(blank)

(main
  (let holder (call Holder))
  (print (call (. holder size)))
  (call (. holder release))
)

(type + Holder
  (prop _ arena Arena)
  (prop _ numbers (of Array U32 ArenaBacking))
  (fn-do + This _
    (lazy numbers (call (of Array U32) arena 8))
  )
  (fn-expr + size Size
    (. numbers length)
  )
  (fn-do + release _
    (call (. arena release))
  )
)
```

```output
8
```

### Properties that hold tuples and have no values: one with a constructor written, one without

```brix
(main
  (let holder (call Holder))
  (print (. holder counter count))
  (print (. holder plain left))
)

(type + Counter (is Tuple)
  (prop + count _ (= 1))
  (prop + label _ (= "plain"))
  (fn-do + This _
    (param start _ (= 40))
    (= count (+ start 2))
    (= label "made")
  )
)

(type + Plain (is Tuple)
  (prop + left _ (= 7))
  (prop + right _ (= 8))
)

(type + Holder
  (prop + counter Counter)
  (prop + plain Plain)
)

(type + Inner (is Tuple)
  (prop + counter Counter)
  (prop + extra _ (= 3))
)

(type + Nest
  (prop + inner Inner)
)
```

```output
42
7
```

### Globals with no values, of a class and of a tuple that write a constructor

```brix
(main
  (print (. kept count))
  (print (. tally count))
)

(blank)
(prop _ kept Counter)
(prop _ tally Tally)

(type + Counter
  (prop + count _ (= 1))
  (fn-do + This _
    (= count 9)
  )
)

(type + Tally (is Tuple)
  (prop + count _ (= 1))
  (fn-do + This _
    (= count 8)
  )
)
```

```output
9
8
```

### Tuple fields that hold objects with no value written

```brix
(main
  (let pair (call Pair))
  (print (. pair left label))
  (print (. pair right label))
  (print (. pair size))
  (let named (call Pair (call Foo "l")))
  (print (. named left label))
  (print (. named right label))
)

(type + Pair (is Tuple)
  (prop + left Foo)
  (prop + right Foo)
  (prop + size Int (= 2))
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
foo
foo
2
l
foo
```

### Fields of a tuple written in parentheses that hold an object, an array and a list with no value written are made by their types' constructors, for a tuple declared with no value and for one given some of its fields.

```brix
(main
  (let pair Pair)
  (print (. pair holder label))
  (print (. pair bytes size))
  (print (. pair items length))
  (let given (call Pair 2))
  (print (. given count))
  (print (. given holder label))
  (print (. given items length))
)

(type + Pair (is (tuple (: count Int) (: holder Foo) (: bytes ([] Byte)) (: items (list Int)))))

(type + Foo
  (prop + label String)
  (fn-do + This _
    (param given String (= "foo"))
    (lazy label given)
  )
)
```

```output
foo
0
0
2
foo
0
```

### Fields of another module's tuple written in parentheses that hold an array and a list with no value written are made by their types' constructors.

```brix
(import ./lib/pair)
(blank)

(main
  (let pair Pair)
  (print (. pair bytes size))
  (print (. pair items length))
  (let given (call Pair 3))
  (print (. given count))
  (print (. given items length))
)
```

```brix file=lib/pair.bx
(type + Pair (is (tuple (: count Int) (: bytes ([] Byte)) (: items (list Int)))))
```

```output
0
0
3
0
```

### A function prints several values, one an interface value

```brix
(main
  (call show (call Thing))
)

(type + Thing
  (implements IFoo)
)

(type + IFoo (is Interface))

(fn-do + show _
  (param thing IFoo)
  (print (tuple "shown" thing))
)
```

```output
(shown, [Interface __source__:IFoo])
```

### A function prints several values, one an object

```brix
(main
  (call show (call Thing))
)

(type + Thing)

(fn-do + show _
  (param thing Thing)
  (print (tuple "shown" thing))
)
```

```output
(shown, [Object __source__:Thing])
```

### A tuple nested in a tuple is taken apart, by a declaration and by a lazy one.

```brix
(main
  (let (tuple a (tuple b c)) (tuple "x" (tuple 1 "y")))
  (print a)
  (print b)
  (print c)
  (lazy (tuple p (tuple q r)) (tuple "z" (tuple 2 "w")))
  (print r)
  (print q)
)
```

```output
x
1
y
w
2
```

### A generic tuple named bare in a cast and a declaration

```brix
(main
  (let n (as (tuple 1 "two") Pair))
  (print (. n b))
  (print (type-of n))
  (let m Pair (tuple 3 4))
  (print m)
  (print (type-of m))
)

(type + (of Pair T1 T2) (is (tuple (: a T1) (: b T2))))
```

```output
two
[TypeInfo Pair'I64'String]
(3, 4)
[TypeInfo Pair'I64'I64]
```

### A generic tuple named bare declared with a list field

```brix
(main
  (let m Pair (tuple (list 1) "x"))
  (print (type-of m))
  (print (. m b))
)

(type + (of Pair T1 T2) (is (tuple (: a T1) (: b T2))))
```

```output
[TypeInfo Pair'List'I64'GCBacking'String]
x
```

### A generic tuple named bare as a return type

```brix
(main
  (let m (call make))
  (print (type-of m))
  (print m)
)

(type + (of Pair T1 T2) (is (tuple (: a T1) (: b T2))))

(fn-do + make Pair
  (return (tuple 1 "two"))
)
```

```output
[TypeInfo Pair'I64'String]
(1, two)
```

### A generic tuple named bare cast from fields read from another

```brix
(main
  (let q (as (tuple 1 2) Pair))
  (let r (as (tuple (+ (. q a) 1) (* (. q b) 2)) Pair))
  (print r)
  (let s (as (tuple 1.5 2) Pair))
  (print (type-of s))
)

(type + (of Pair T1 T2) (is (tuple (: a T1) (: b T2))))
```

```output
(2, 4)
[TypeInfo Pair'F64'I64]
```

### A generic tuple named bare whose field is a tuple of its parameter

```brix
(main
  (let p (as (tuple (tuple 1 2) 3) Wrap))
  (print (type-of p))
  (print (. p inner))
)

(type + (of Wrap T) (is (tuple (: inner (tuple T T)) (: extra T))))
```

```output
[TypeInfo Wrap'I64]
(1, 2)
```

### A generic tuple named bare whose field is a list of its parameter

```brix
(main
  (let p (as (tuple (list 1 2) 3) Bag))
  (print (type-of p))
  (print (. p extra))
)

(type + (of Bag T) (is (tuple (: items (list T)) (: extra T))))
```

```output
[TypeInfo Bag'I64]
3
```

### A tuple parameter whose field is an interface given a list

```brix
(main
  (call foo (tuple (list 1 2) 3))
)

(fn-do + foo _
  (param n (tuple (: x (of Iterable Int)) (: y Int)))
  (for (in v (. n x))
    (print v)
  )
  (print (. n y))
)
```

```output
1
2
3
```

### A tuple parameter whose two fields are streams

```brix
(main
  (call foo (tuple (list 1 2) (list "a" "b")))
)

(fn-do + foo _
  (param n (tuple (: x (of Stream Int)) (: y (of Stream String))))
  (print (type-of (. n x)))
  (print (type-of (. n y)))
  (for (in v (. n y))
    (print v)
  )
)
```

```output
[TypeInfo Indexable'I64]
[TypeInfo Indexable'String]
a
b
```

### A nested tuple parameter whose field is an interface

```brix
(main
  (call foo (tuple (tuple 1 (list 2 3)) 4))
)

(fn-do + foo _
  (param n (tuple (: p (tuple (: a Int) (: b (of Iterable Int)))) (: c Int)))
  (for (in v (. n p b))
    (print v)
  )
)
```

```output
2
3
```

### A binding of a generic constructor called with two parameters in turn

```brix
(main
  (let mk (call Thing 1 ...))
  (let a (call (of mk Int)))
  (let b (call (of mk String)))
  (print a)
  (print b)
  (print (type-of a))
  (print (type-of b))
)

(type + (of Thing T) (is (tuple (: foo Int) (: bar T))))
```

```output
(1, 0)
(1, )
[TypeInfo Thing'I64]
[TypeInfo Thing'String]
```

### A binding of a constructor of two parameters that holds the first field

```brix
(main
  (let mk (call Duo 3 ...))
  (print (call (of mk Int Bool)))
  (print (call (of mk Int Bool)))
)

(type + (of Duo A B) (is (tuple (: c Int) (: a A) (: b B))))
```

```output
(3, 0, false)
(3, 0, false)
```

### A binding of a generic constructor whose held argument converts

```brix
(main
  (let mk (call Thing 10u8 ...))
  (let t (call (of mk Float)))
  (print (. t bar))
  (print (. t foo))
)

(type + (of Thing T) (is (tuple (: foo Int) (: bar T))))
```

```output
0
10
```

### A tuple whose field holds a number class with its own == compares that field by it.

```brix
(main
  (let a (call Holder (call Counter 1) 2))
  (let b (call Holder (call Counter 1) 2))
  (print (== a b))
)

(type + Counter (is I32)
  (fn-do + (infix this == (param other Counter)) _
    (print "counter compare")
    (return false)
  )
)

(type + Holder (is (tuple (: count Counter) (: other I32))))
```

```output
counter compare
false
```

### A number class with its own == is compared by it, not by its machine value.

```brix
(main
  (let a (call Counter 1))
  (let b (call Counter 1))
  (print (== a b))
)

(type + Counter (is I32)
  (fn-do + (infix this == (param other Counter)) _
    (print "counter compare")
    (return false)
  )
)
```

```output
counter compare
false
```

### A binding of a generic tuple's constructor that holds nothing makes the default once its parameter is written.

```brix
(main
  (let mk (call Thing ...))
  (let t (call (of mk Int)))
  (print t)
)

(type + (of Thing T) (is (tuple (: a T) (: b T))))
```

```output
(0, 0)
```

### A binding of a generic tuple's constructor takes its parameters and no argument.

```brix crash
(main
  (let mk (call Thing ...))
  (let t (call (of mk Float) 1.5 2.5))
  (print t)
  (print (type-of t))
)

(type + (of Thing T) (is (tuple (: a T) (: b T))))
```

```output
Exception: Illegal arguments to binding
<no file:4>
      (let t (call (of mk Float) 1.5 2.5))
             ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A binding of a generic tuple's constructor that holds an argument takes no more.

```brix crash
(main
  (let mk (call Trio 1 ...))
  (print (call (of mk Int) 5 6))
  (print (call (of mk Float) 5 6.5))
)

(type + (of Trio T) (is (tuple (: a Int) (: b Int) (: c T))))
```

```output
Exception: Illegal arguments to binding
<no file:4>
      (print (call (of mk Int) 5 6))
             ^^^^^^^^^^^^^^^^^^^^^^
```

### A tuple class named bare takes the join of the types its fields give one parameter.

```brix
(main
  (let p (as (tuple 1 2.5) Same))
  (print (type-of p))
  (print p)
  (let q (as (tuple 1u8 300) Same))
  (print (type-of q))
  (print q)
)

(type + (of Same T) (is (tuple (: a T) (: b T))))
```

```output
[TypeInfo Same'F64]
(1, 2.5)
[TypeInfo Same'I64]
(1, 300)
```

### A declared list of a tuple class named bare is completed from its items.

```brix
(main
  (let ps (list Pair) (list (tuple 1 "x") (tuple 2 "y")))
  (print (type-of ps))
  (print (+ (. ([] ps 0) a) 10))
  (print (. ([] ps 1) b))
  (let qs (list (? Pair)) (list (tuple 3 "z") null))
  (print (type-of qs))
)

(type + (of Pair T1 T2) (is (tuple (: a T1) (: b T2))))
```

```output
[TypeInfo List'Pair'I64'String'GCBacking]
11
y
[TypeInfo List'Optional'Pair'I64'String'GCBacking]
```

### A tuple compared field by field compares a field of a function type as one object.

```brix
(main
  (let f (=> () 1))
  (print (== (call Holder f 1) (call Holder f 1)))
  (print (== (call Holder f 1) (call Holder f 2)))
)

(type + Holder (is (tuple (: fn (fn-type Int)) (: n I32))))
```

```output
true
false
```

```brix
(import /BxGC/BxGCTest.h)
(blank)

(main
  (let local (call Wide 1 2 3 4 5 6 7 8))
  (let alias local)
  (let optional (? Wide) local)
  (let values (list local (call Wide 8 7 6 5 4 3 2 2)))
  (let callback WideCallback (=> ((: value Wide)) (call sum value)))
  (print (call sum local))
  (print (call sum alias))
  (print (call sum (!! optional)))
  (print (call sum ([] values 0)))
  (print (call sum ([] values 1)))
  (print (call callback local))
  (print (catch? (call sum-fallible local)))
  (print (call sum-after-collection local))
  (print (call sum (call (. local shifted) 1)))
  (print local)
)

(type + Wide (is Tuple)
  (prop + a Float)
  (prop + b Float)
  (prop + c Float)
  (prop + d Float)
  (prop + e Float)
  (prop + f Float)
  (prop + g Float)
  (prop + h Float)
  (fn-do + shifted Wide
    (param amount Float)
    (return
      (call Wide
        (block
          (arg (+ a amount))
          (arg (+ b amount))
          (arg (+ c amount))
          (arg (+ d amount))
          (arg (+ e amount))
          (arg (+ f amount))
          (arg (+ g amount))
          (arg (+ h amount))
        )
      )
    )
  )
)

(type + WideCallback (is (fn-type Float (param value Wide))))

(fn-expr _ sum Float
  (param value Wide)
  (+ (. value a) (. value b) (. value c) (. value d) (. value e) (. value f) (. value g) (. value h))
)

(fn-do _ sum-fallible Float (?! Error)
  (param value Wide)
  (return (call sum value))
)

(fn-do _ sum-after-collection Float
  (param value Wide)
  (call C:brixCollect)
  (return (+ (call sum value) 1))
)
```

```output
36
36
36
36
37
36
36
37
44
(1, 2, 3, 4, 5, 6, 7, 8)
```

```brix
(main
  (let nested
    (call Nested
      (block
        (arg (call Pair 1 2))
        (arg (call Pair 3 4))
        (arg (call Pair 5 6))
        (arg (call Pair 7 8))
      )
    )
  )
  (print (call sum-nested nested))
  (print (call sum-ten (call Ten 1 2 3 4 5 6 7 8 9 10)))
  (print (call sum-one (call One 36)))
  (print (call sum-pair (call Pair 17 19)))
  (print (call sum-four (call Four 6 8 10 12)))
)

(type + One (is Tuple)
  (prop + a Float)
)

(type + Pair (is Tuple)
  (prop + a Float)
  (prop + b Float)
)

(type + Four (is Tuple)
  (prop + a Float)
  (prop + b Float)
  (prop + c Float)
  (prop + d Float)
)

(type + Ten (is Tuple)
  (prop + a Float)
  (prop + b Float)
  (prop + c Float)
  (prop + d Float)
  (prop + e Float)
  (prop + f Float)
  (prop + g Float)
  (prop + h Float)
  (prop + i Float)
  (prop + j Float)
)

(type + Nested (is Tuple)
  (prop + a Pair)
  (prop + b Pair)
  (prop + c Pair)
  (prop + d Pair)
)

(fn-expr _ sum-one Float
  (param value One)
  (. value a)
)

(fn-expr _ sum-pair Float
  (param value Pair)
  (+ (. value a) (. value b))
)

(fn-expr _ sum-four Float
  (param value Four)
  (+ (. value a) (. value b) (. value c) (. value d))
)

(fn-expr _ sum-ten Float
  (param value Ten)
  (+ (. value a) (. value b) (. value c) (. value d) (. value e) (. value f) (. value g) (. value h) (. value i) (. value j))
)

(fn-expr _ sum-nested Float
  (param value Nested)
  (+ (call sum-pair (. value a)) (call sum-pair (. value b)) (call sum-pair (. value c)) (call sum-pair (. value d)))
)
```

```output
36
55
36
36
36
```

```brix
(import /BxGC/BxGC.h)
(blank)

(main
  (let nested-value (. (call nested "live") value))
  (print (. (!! nested-value) a))
  (print (. (call reused "same") right a))
  (let interrupted-value (. (call interrupted "safe") value))
  (print (. (!! interrupted-value) a))
)

(blank)
(type-line + Text (is (tuple (: a String) (: b String) (: c String) (: d String))))
(type-line + Box (is (tuple (: value (? Text)) (: marker U64))))
(type-line + Pair (is (tuple (: left Text) (: right Text))))

(fn-do _ nested Box
  (param seed String)
  (let result (call Box (call Text seed "b" "c" "d") 7))
  (call C:brixCollect)
  (return result)
)

(fn-do _ reused Pair
  (param seed String)
  (let text (call Text seed "b" "c" "d"))
  (let result (call Pair text text))
  (call C:brixCollect)
  (return result)
)

(fn-do _ interrupted Box
  (param seed String)
  (let text (call Text seed "b" "c" "d"))
  (call C:brixCollect)
  (let result (call Box text 8))
  (return result)
)
```

```output
live
same
safe
```

```brix
(import /BxGC/BxGC.h)
(blank)

(main
  (let wrapped (call wrap 1))
  (let base (call Big (as 1 U64) (as 2 U64) (as 3 U64) (as 4 U64)))
  (let forwarded (call forward-input base))
  (let text (call wrap-text "a"))
  (call C:brixCollect)
  (print (. (!! wrapped) a))
  (print (. (!! wrapped) b))
  (print (. (!! wrapped) c))
  (print (. (!! wrapped) d))
  (print (. forwarded a))
  (print (. (!! text) a))
  (print (. (!! text) d))
)

(blank)
(type-line + Big (is (tuple (: a U64) (: b U64) (: c U64) (: d U64))))
(type-line + BigText (is (tuple (: a String) (: b String) (: c String) (: d String))))

(fn-expr _ make Big
  (param seed U64)
  (call Big seed (as 2 U64) (as 3 U64) (as 4 U64))
)

(fn-expr _ wrap (? Big)
  (param seed U64)
  (call make seed)
)

(fn-expr _ return-input Big
  (param value Big)
  value
)

(fn-expr _ forward-input Big
  (param value Big)
  (call return-input value)
)

(fn-expr _ make-text BigText
  (param seed String)
  (call BigText seed "b" "c" "d")
)

(fn-expr _ wrap-text (? BigText)
  (param seed String)
  (call make-text seed)
)
```

```output
1
2
3
4
1
a
d
```

```brix
(main
  (print (call Pair "a" null))
  (print (call Pair "b" "c"))
)

(blank)
(type-line + Pair (is (tuple (: name String) (: detail (? String)))))
```

```output
(a, unknown)
(b, c)
```

```brix
(main
  (let owner (call Owner (call Box 3)))
  (print (. owner held box n))
)

(type + Owner
  (prop _ inner Pair)
  (fn-do + This _
    (param box Box)
    (print (. inner box n))
    (= inner (call Pair box))
  )
  (fn-expr + held Pair :get
    inner
  )
)

(type + Pair (is Tuple)
  (prop + box Box)
  (fn-do + This _
    (param box Box)
  )
)

(type + Box
  (prop + n I32 (= 7))
  (fn-do + This _
    (param n I32)
  )
)
```

```output
7
3
```

```brix
(main
  (let h (call Holder))
  (print (. h inner n))
)

(type + Holder (is Tuple)
  (prop + inner Inner)
)

(type + Inner (is Tuple)
  (prop + n Size)
  (fn-do + This _
    (param start Size (= 4))
    (= n (+ start 1))
  )
)
```

```output
5
```

### A tuple given where one of other field types is taken converts each field by its place.

```brix
(main
  (print (call show (call make 1 7)))
  (print (call show (call make 2 9)))
  (print (call show (call make 5 0)))
)

(type + Id (is Int))

(type + Partial (is (tuple (: container Id) (: count Int))))

(fn-expr + make Partial
  (param count Int)
  (param container Id)
  (tuple count container)
)

(fn-expr + show String
  (param partial Partial)
  (call describe (call pick container) count)
  (where
    (lazy (tuple count container) partial)
  )
)

(fn-expr + pick Id
  (param value (? Id))
  (?? value 0)
)

(fn-expr + describe String
  (param id Id)
  (param count Int)
  (str id " " count)
)
```

```output
7 1
9 2
0 5
```

### A function value a tuple pattern binds is called by its name.

```brix
(main
  (let b (call Boxed (=> ((: n Int)) (str "k" n))))
  (print (call (. b key) 3 "p"))
  (print (call (. (call Boxed null) key) 4 "q"))
  (print (call (. (call Boxed null) key) 5 ""))
)

(type + Keyer (is (fn-type String (param n Int))))

(type + Boxed
  (prop + keyer (? Keyer))
  (fn-do + This _
    (param keyer (? Keyer))
  )
  (fn-expr + key String
    (param n Int)
    (param prefix String)
    (match (tuple keyer prefix)
      (case (tuple (: f Keyer) _)
        (call f n)
      )
      (case (tuple _ ({} it))
        (str prefix "/" n)
      )
      (default
        (str n)
      )
    )
  )
)
```

```output
k3
q/4
5
```

### A tuple made with one field given by name gives the fields before it their values.

```brix
(main
  (let p
    (call Pair
      (block
        (:b 4)
      )
    )
  )
  (print (. p a))
)

(type + Pair (is Tuple)
  (prop + a Size (= 5))
  (prop + b Size (= 0))
  (prop + c Size (= 0))
)

(fn-expr _ both Size
  (param first Size)
  (param second Size)
  (+ first second)
)
```

```output
5
```

### The value of a field of a tuple calls with arguments written as lines and reads fields declared after it.

```brix
(main
  (let u
    (call Usage
      (block
        (:reasoning-tokens 3)
      )
    )
  )
  (print (?? (. u total-tokens) 0))
  (print (?? (. u input-tokens) 0))
)

(type + Usage (is Tuple)
  (prop + input-tokens (? Size)
    (=
      (call usage-sum
        (block
          (arg cached-input-tokens)
          (arg tool-prompt-tokens)
        )
      )
    )
  )
  (prop + output-tokens (? Size) (= (call usage-sum reasoning-tokens thought-tokens)))
  (prop + total-tokens (? Size) (= (call usage-sum input-tokens output-tokens)))
  (prop + cached-input-tokens (? Size) (= null))
  (prop + reasoning-tokens (? Size) (= null))
  (prop + thought-tokens (? Size) (= null))
  (prop + tool-prompt-tokens (? Size) (= null))
)

(fn-expr _ usage-sum (? Size)
  (param first (? Size))
  (param second (? Size))
  (case? (or (is first Size) (is second Size))
    (+ (?? first 0) (?? second 0))
  )
  (default
    null
  )
)
```

```output
3
0
```

### Assigning a field of a tuple as the value of an arm gives the new tuple.

```brix
(main
  (print (call describe (call with-field (call Pair 1 2) "left" 7)))
  (print (call describe (call with-field (call Pair 1 2) "right" 8)))
  (print (call describe (call with-field (call Pair 1 2) "other" 9)))
)

(type + Pair (is Tuple)
  (prop + left U64)
  (prop + right U64)
  (fn-do + This _
    (param left _)
    (param right _)
  )
)

(fn-expr + describe String
  (param pair (? Pair))
  (if-expr pair (str (. pair left) " " (. pair right)) "none")
)

(fn-expr + with-field _
  (param pair Pair)
  (param field String)
  (param value U64)
  (match? field
    (case= "left"
      (= (. pair left) value)
    )
    (case= "right"
      (= (. pair right) value)
    )
  )
)
```

```output
7 2
1 8
none
```

### Two tuples are equal where their fields are, an optional field of a type that declares `==` compared by it where both hold a value, with the right one first.

```brix
(main
  (let one (call Record (call Spot 1 2) 3))
  (let two (call Record (call Spot 5 6) 3))
  (let three (call Record null 3))
  (let four (call Record null 3))
  (print (== one two))
  (print (== one three))
  (print (== three four))
  (print (== three one))
  (let a (? Spot) (call Spot 1 2))
  (let b (? Spot) (call Spot 5 6))
  (print (== a b))
)

(type + Spot (is Tuple)
  (prop + x I32)
  (prop + y I32)
  (fn-expr + (infix this == (param other Spot)) _
    (before
      (print (str "called " x " " (. other x)))
    )
    true
  )
)

(type + Record (is Tuple)
  (prop + pivot (? Spot))
  (prop + size I32)
)
```

```output
called 5 1
true
false
true
false
called 5 1
true
```

### What two optional fields hold is compared with the right one first, down to the `==` a field within declares.

```brix
(main
  (let one (call Record (call Wrap (call Spot 1 2)) 3))
  (let two (call Record (call Wrap (call Spot 5 6)) 3))
  (print (== one two))
)

(type + Spot (is Tuple)
  (prop + x I32)
  (prop + y I32)
  (fn-expr + (infix this == (param other Spot)) _
    (before
      (print (str "called " x " " (. other x)))
    )
    true
  )
)

(type + Wrap (is Tuple)
  (prop + spot Spot)
)

(type + Record (is Tuple)
  (prop + inner (? Wrap))
  (prop + size I32)
)
```

```output
called 5 1
true
```

### A field whose type declares `==` only for a type it converts to is compared by that `==`, the other field converted to it, and an optional one so where both hold a value.

```brix
(main
  (let one (call Record (call Stamp 1) (call Stamp 4)))
  (let two (call Record (call Stamp 1) (call Stamp 4)))
  (let three (call Record (call Stamp 1) null))
  (print (== one two))
  (print (== one three))
  (print (== (list one three) (list two three)))
)

(type + Stamp (is Tuple)
  (prop + seconds Int)
  (fn-to-expr + Int
    seconds
  )
  (fn-expr + (infix this == (param other Int)) _
    (before
      (print (str "compared " seconds " " other))
    )
    (== seconds other)
  )
)

(type + Record (is Tuple)
  (prop + start Stamp)
  (prop + end (? Stamp))
)
```

```output
compared 1 1
compared 4 4
true
compared 1 1
false
compared 1 1
compared 4 4
compared 1 1
true
```

### A tuple that declares a field named `length` has that field assigned, as any other, and keeps all its fields.

```brix
(main
  (let span (call Span 4 0))
  (= span (= (. span length) (+ (. span size) 1)))
  (print (str (. span size) " " (. span length)))
)

(type + Span (is Tuple)
  (prop + size U32)
  (prop + length U32)
)
```

```output
4 5
```

### A tuple pattern on an optional tuple matches its fields where there is one, and nothing where there is none.

```brix
(main
  (let found (call search 1))
  (match? found
    (case (tuple name size)
      (print (str name " " size))
    )
  )
  (let none (call search 5))
  (match? none
    (case (tuple name size)
      (print (str name " " size))
    )
  )
  (print "done")
)

(fn-do _ search (? (tuple String Int))
  (param least Int)
  (for (in pair (call pairs))
    (if (> ([] pair 1) least)
      (return pair)
    )
  )
  (return null)
)

(fn-expr _ pairs _
  (item (tuple "x" 1))
  (item (tuple "yy" 2))
  (item (tuple "zzz" 3))
)
```

```output
yy 2
done
```

### An assignment to a field of a tuple an index names evaluates the value before the tuple, and stores the new tuple it gives.

```brix
(main
  (let l (list T))
  (+= l (call T 1 2))
  (+= l (call T 3 4))
  (let n 0)
  (= ([] l 1) (= (. ([] l 1) a) (call bump l n)))
  (print (str "l1 " (. ([] l 1) a) " " (. ([] l 1) b)))
  (= ([] l 0) (= (. ([] l 0) b) (call bump l 5)))
  (print (str "l0 " (. ([] l 0) a) " " (. ([] l 0) b)))
  (let h (call Holder))
  (= ([] (. h items) 0) (= (. ([] (. h items) 0) a) (call (. h grow))))
  (print (str "h " (. ([] (. h items) 0) a) " " (. ([] (. h items) 0) b) " " (. h items length)))
)

(type + T (is Tuple)
  (prop + a Int)
  (prop + b Int)
)

(type + Holder
  (prop + items (list T))
  (fn-do + This _
    (+= items (call T 8 9))
  )
  (fn-do + grow Int
    (= ([] items 0) (call T 100 200))
    (return 3)
  )
)

(fn-do + bump Int
  (param l (list T))
  (param n Int)
  (= ([] l 1) (call T 30 40))
  (= ([] l 0) (call T 10 20))
  (return (+ n 7))
)
```

```output
l1 7 40
l0 10 12
h 3 200 1
```

### An assignment to a field of a tuple evaluates the value, then the index of the tuple, then the index it is stored at, and gives a new tuple without changing the one it read.

```brix
(main
  (let l (list T))
  (+= l (call T 1 2))
  (+= l (call T 3 4))
  (= ([] l (call at "outer" 1)) (= (. ([] l (call at "inner" 1)) a) (call val "rhs")))
  (print (str "l1 " (. ([] l 1) a) " " (. ([] l 1) b)))
  (let t (call T 5 6))
  (let u (= (. t b) (call val "tb")))
  (print (str "u " (. u a) " " (. u b) " t " (. t a) " " (. t b)))
)

(type + T (is Tuple)
  (prop + a Int)
  (prop + b Int)
)

(fn-do + at Index
  (param tag String)
  (param i Index)
  (print tag)
  (return i)
)

(fn-do + val Int
  (param tag String)
  (print tag)
  (return 9)
)
```

```output
rhs
inner
outer
l1 9 4
tb
u 5 9 t 5 6
```

### An assigning operator on a tuple's field gives a new tuple and leaves the tuple it read as it was.

```brix
(main
  (let p (call Pt 5 7))
  (let v (-= (. p x) 1))
  (print (str "local " (. v x) " " (. v y) " / " (. p x) " " (. p y)))
  (print (call shift (call Pt 5 7) (call Pt 1 2)))
)

(type + Pt (is (tuple (: x I32) (: y I32))))

(fn-do _ shift String
  (param a Pt)
  (param c Pt)
  (let v (-= (. a x) (. c x)))
  (= a (-= (. a y) (. c y)))
  (return (str "param " (. v x) " " (. v y) " / " (. a x) " " (. a y)))
)
```

```output
local 4 7 / 5 7
param 4 7 / 5 5
```

### A statement that applies an assigning operator to a tuple's field loses the new tuple.

```brix crash
(main
  (print (call shift (call Pt 5 7) (call Pt 1 2)))
)

(type + Pt (is (tuple (: x I32) (: y I32))))

(fn-do _ shift String
  (param a Pt)
  (param c Pt)
  (-= (. a x) (. c x))
  (return (str (. a x) " " (. a y)))
)
```

```output
Exception: Assigning x creates a new Pt tuple that is discarded; write a = a.x = value
<no file:11>
      (-= (. a x) (. c x))
      ^^^^^^^^^^^^^^^^^^^^
```

### An assigning operator that the field's own type answers in place, as an array's `+=` does, assigns no field of the tuple, so the statement loses nothing.

```brix
(main
  (let boxes (call fill (list 1 2)))
  (print (str (. boxes tail items length) " " (. boxes tail count)))
)

(type + Box (is Tuple)
  (prop + count Int)
  (prop + items (list Int))
)

(fn-do _ fill (list Box)
  (param numbers (list Int))
  (let boxes (list (call Box 0)))
  (for (in number numbers)
    (+= (. boxes tail items) number)
    (+= (. boxes tail items) number)
  )
  (return boxes)
)
```

```output
4 0
```

### A tuple written in parentheses after a local's name is the local's type: the value is converted to it field by field, as an argument is.

```brix
(main
  (let t (tuple Float Int) (tuple 1 2))
  (let (tuple x y) t)
  (print (/ x 2))
  (print t)
  (let k (tuple I8 Float) (tuple 3 4))
  (print k)
)
```

```output
0.5
(1, 2)
(3, 4)
```

### A local written with a named tuple type has the fields by name, and declares no local for them.

```brix
(main
  (let a (tuple (: x Int) (: y Int)) (tuple 1 2))
  (print (. a x))
  (print (. a y))
  (print a)
)
```

```output
1
2
(1, 2)
```

### A local written with a tuple type and no value holds the tuple with nothing written.

```brix
(main
  (let t (tuple Float Int))
  (print t)
)
```

```output
(0, 0)
```

### A field of a local written with a named tuple type is not a local.

```brix crash
(main
  (let a (tuple (: x Int) (: y Int)) (tuple 1 2))
  (print x)
)
```

```output
Exception: Name "x" not found
<no file:4>
      (print x)
             ^
```

### A named tuple given as its base is a copy of the base's fields: cast, passed, or held by a local declared as the base.

```brix
(main
  (let c (call Colored 1 2 3))
  (print (as c Point))
  (print (. (as c Point) length))
  (print (call both c 7))
  (let p Point c)
  (print p)
)

(type + Point (is (tuple (: x Int) (: y Int))))

(type + Colored (is Point)
  (prop + color Int)
)

(fn-expr + both Int
  (param first Point)
  (param second Int)
  (+ (* (. first x) 1000) (* (. first y) 10) second)
)
```

```output
(1, 2)
2
1027
(1, 2)
```

### A struct held by a local declared as its base is a copy of the base's fields.

```brix
(main
  (let c (call Colored 1 2 3))
  (let p Point c)
  (print p)
  (print (call both c 7))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)

(type + Colored (is Point)
  (prop + color Int)
)

(fn-expr + both Int
  (param first Point)
  (param second Int)
  (+ (* (. first x) 1000) (* (. first y) 10) second)
)
```

```output
(1, 2)
1027
```

### A tuple taken apart from an optional takes apart what it holds, and panics where it holds nothing.

```brix crash
(main
  (for (in seed (..< 0 3))
    (let (tuple lane checksum) (call make seed))
    (print (str lane " " checksum))
  )
)

(type + Pair (is (tuple (: lane Index) (: checksum Size))))

(fn-expr + make (? Pair)
  (param seed Int)
  (only-if (call Pair (+ seed 1) (+ seed 2)) (if (!= seed 1)))
)
```

```stderr
Unhandled panic: Optional'Pair is null
<no file:4>

Task 1

Stack trace:
  0  __source__:main no file:4:10
  1  __source__:init.__source__ no file
  2  __source__:brix.main no file
```

### A field of an object a tuple written as `(…)` is not given holds the object its class constructs.

```brix
(main
  (let made Pair)
  (print (. made count))
  (print (. made thing value))
  (let rev (call Rev 5))
  (print (. rev thing value))
  (let map (map Int Pair))
  (= ([] map 1) (call Pair (call Thing) 2))
  (-= ([] map 1))
  (print (. map length))
)

(type + Thing
  (prop + value _ (= 7))
)

(type + Pair (is (tuple (: thing Thing) (: count Int))))

(type + Rev (is (tuple (: count Int) (: thing Thing))))
```

```output
0
7
7
0
```

```brix
(main
  (let holder (call Holder))
  (+= (. holder data items) (call Item 7))
  (print (str (. holder data items length) " " (. ([] (. holder data items) 0) id) " " (. holder data count)))
  (let other (call Holder))
  (print (. other data items length))
)

(type + Item (is Tuple)
  (prop + id U64)
)

(type + Data (is Tuple)
  (prop + items (list Item))
  (prop + count I64)
)

(type + Holder
  (prop + data _ (= (call Data)))
)
```

```output
1 7 0
0
```

### A tuple written with no name becomes a tuple class unasked only where each field would convert unasked to the field at its place.

```brix crash
(main
  (print (. (call make true 7) container))
)

(type + Id (is U32))

(type + Partial (is (tuple (: container Id) (: flag Bool))))

(fn-expr + make Partial
  (param flag Bool)
  (param container Id)
  (tuple flag container)
)
```

```output
Exception: Expected Partial here, but this is (Bool, Id), which becomes Partial only when asked; write "(as (tuple flag container) Partial)"
<no file:13>
      (tuple flag container)
      ^^^^^^^^^^^^^^^^^^^^^^
```

### A tuple written with integers with no suffix is given where a tuple class is expected before anything else has read the class, and each integer takes the type of its field.

```brix
(main
  (let holder (call Holder))
  (call (. holder go))
)

(type + Holder
  (fn-do + go _
    (call move (tuple 3 0))
  )
  (fn-do + move _
    (param spot Spot)
    (print (+ (. spot x) (. spot y)))
  )
)

(type + Spot (is Tuple)
  (prop + x I16)
  (prop + y I16)
)
```

```output
3
```

### Tuples of numbers written with no suffix in a list join place by place, a float at a place where either holds one.

```brix
(main
  (let pairs (list (tuple 2.5 1) (tuple 1 2.5)))
  (let floats (list (tuple Float Float)) pairs)
  (print ([] floats 0))
  (print ([] floats 1))
)
```

```output
(2.5, 1)
(1, 2.5)
```

### A tuple whose own constructors all take arguments starts at its fields' first values where a parameter's default holds it with no value written.

```brix
(main
  (print (call shown))
)

(type + Spot (is Tuple)
  (prop + x Float)
  (prop + y Float)
  (fn-do + This _
    (param parts (list Float))
    (return (call This ([] parts 0) ([] parts 1)))
  )
)

(type + Holder (is Tuple)
  (prop + place Spot)
  (prop + count U64)
)

(fn-expr + shown Float
  (param holder Holder (= (call Holder)))
  (+ (. holder place x) (. holder place y))
)
```

```output
0
```

### A returned tuple of unsuffixed numbers takes the named tuple type another return gives.

```brix
(main
  (let found (call first-found 3))
  (print (. found ok))
  (print (. found x))
  (let missing (call first-found 0))
  (print (. missing ok))
  (print (. missing x))
)

(fn-do _ first-found _
  (param count I64)
  (for (in index (..< 0 count))
    (let item (call item-at index))
    (if (. item ok)
      (return item)
    )
  )
  (blank)
  (return (tuple false 0))
)

(fn-do _ item-at (tuple (: ok Bool) (: x F64))
  (param index I64)
  (return (tuple (> index 1) 1.5))
)
```

```output
true
1.5
false
0
```

### A list of tuples larger than a pair store can reach builds, clearing each one whole.

```brix
(main
  (let items (list Huge) (list (call Huge)))
  (print (. ([] items 0) e d d))
)

(type _ Quad (is Tuple)
  (prop + a F64 (= 1))
  (prop + b F64 (= 2))
  (prop + c F64 (= 3))
  (prop + d F64 (= 4))
)

(type _ Wide (is Tuple)
  (prop + a Quad (= (call Quad)))
  (prop + b Quad (= (call Quad)))
  (prop + c Quad (= (call Quad)))
  (prop + d Quad (= (call Quad)))
)

(type _ Huge (is Tuple)
  (prop + a Wide (= (call Wide)))
  (prop + b Wide (= (call Wide)))
  (prop + c Wide (= (call Wide)))
  (prop + d Wide (= (call Wide)))
  (prop + e Wide (= (call Wide)))
)
```

```output
4
```

### `is` tests a tuple against the types written in parentheses, one for each field.

```brix
(main
  (let pair (tuple 3 "three"))
  (print (is pair (tuple Int String)))
  (print (is pair (tuple String String)))
  (print (is-not pair (tuple Int String)))
  (let maybe (? (tuple Int String)) pair)
  (print (is maybe (tuple Int String)))
  (let none (? (tuple Int String)) null)
  (print (is none (tuple Int String)))
)
```

```output
true
false
false
true
false
```

### Only types are written in parentheses after `is`; what the fields hold is tested by an arm of a match.

```brix crash
(main
  (let pair (tuple 3 "three"))
  (print (is pair (tuple 3 String)))
)
```

```output
Exception: Only types may be written in parentheses after "is"; to test what the fields hold, match the value with an arm, as in "| (3, name)"
<no file:4>
      (print (is pair (tuple 3 String)))
                      ^^^^^^^^^^^^^^^^
```

### Names in brackets are declared from a list but not assigned again; names are assigned again from a tuple.

```brix crash
(main
  (let x 0)
  (let y 0)
  (= (list x y) (list 3 4))
)
```

```output
Exception: Names in brackets can be declared from a list, as in ". [x, y] = items", but not assigned again; assign a tuple to them, as in "x, y = items[0], items[1]"
<no file:5>
      (= (list x y) (list 3 4))
         ^^^^^^^^^^
```
