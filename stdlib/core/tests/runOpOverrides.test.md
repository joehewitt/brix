# Op overrides

Binary operators

```brix
(main
  (let thing (call Thing))
  (print (+ thing 1))
  (print (+ thing thing))
)

(type + Thing
  (prop + a I32 (= 10))
  (fn-expr + (infix this + (param n I32)) Int
    (+ a n)
  )
  (fn-expr + (infix this + (param n Thing)) Float
    (+ a (. n a))
  )
)
```

```output
11
20
```

## Incremental assignment operators.

```brix
(main
  (let thing (call Thing))
  (+= thing 1)
  (print (. thing a))
)

(type + Thing
  (prop + a _ (= 10))
  (fn-do + (infix this += (param n _)) _
    (+= a n)
  )
)
```

```output
11
```

### If incremental operator not overriden but math op is, use that

```brix
(main
  (let thing (call Thing 10))
  (+= thing 1)
  (print (. thing a))
)

(type + Thing
  (prop + a _ (= 0))
  (fn-do + This _
    (param aa _)
    (= a aa)
  )
  (fn-expr + (infix this + (param n _)) _
    (call Thing (+ a n))
  )
)
```

```output
11
```

### Incremental operator on a property of a member

```brix
(main
  (let thing (call Thing))
  (+= (. thing member) 1)
  (print (. thing member a))
)

(type + Member
  (prop + a _ (= 10))
  (fn-do + (infix this += (param n _)) _
    (+= a n)
  )
)

(type + Thing
  (prop + member _ (= (call Member)))
)
```

```output
11
```

## Concat operator

```brix
(main
  (let thing (call Thing))
  (print (str thing " you"))
)

(type + Thing
  (prop + a _ (= "me"))
  (fn-to-expr + String
    a
  )
  (fn-expr + (infix this ++ (param other _)) _
    (str a " with" other)
  )
)
```

```output
me you
```

## Comparison operators.

```brix
(main
  (let a (call Thing 10))
  (let b (call Thing 20))
  (print (== a b))
  (print (== a a))
)

(type + Thing
  (prop + a _ (= 10))
  (fn-do + This _
    (param v _)
    (= a v)
  )
  (fn-expr + (infix this == (param other _)) _
    (== a (. other a))
  )
)
```

```output
false
true
```

```brix
(main
  (let a (call Thing 10))
  (let b (call Thing 10))
  (print (== a b))
  (print (== b a))
  (print (!= a b))
  (print (!= b a))
  (= (. b a) 11)
  (print (== a b))
  (print (== b a))
  (print (!= a b))
  (print (!= b a))
)

(type + Thing
  (prop + a _ (= 10))
  (fn-do + This _
    (param v _)
    (= a v)
  )
  (fn-expr + (infix this != (param other Thing)) _
    (!= a (. other a))
  )
)
```

```output
true
true
false
false
false
false
true
true
```

### != deduced from ==

```brix
(main
  (let a (call Thing 10))
  (let b (call Thing 10))
  (print (== a b))
  (print (== b a))
  (print (!= a b))
  (print (!= b a))
  (= (. b a) 11)
  (print (== a b))
  (print (== b a))
  (print (!= a b))
  (print (!= b a))
)

(type + Thing
  (prop + a _ (= 10))
  (fn-do + This _
    (param v _)
    (= a v)
  )
  (fn-expr + (infix this == (param other Thing)) _
    (== a (. other a))
  )
)
```

```output
true
true
false
false
false
false
true
true
```

```brix
(main
  (let n10 (call Thing 10))
  (let n11 (call Thing 11))
  (print (< n10 n11))
  (print (> n10 n11))
  (print (< n11 n10))
  (print (> n11 n10))
  (= (. n11 a) 10)
  (print (< n10 n11))
  (print (> n10 n11))
  (print (< n11 n10))
  (print (> n11 n10))
)

(type + Thing
  (prop + a _ (= 10))
  (fn-do + This _
    (param v _)
    (= a v)
  )
  (fn-expr + (infix this > (param other Thing)) _
    (> a (. other a))
  )
  (fn-expr + (infix this == (param other Thing)) _
    (== a (. other a))
  )
)
```

```output
true
false
false
true
false
false
false
false
```

```brix crash
(main
  (let a (call Thing 10))
  (let b (call Thing 10))
  (print (< a b))
)

(type + Thing
  (prop + value Int)
  (fn-do + This _
    (param value _)
  )
  (fn-expr + (infix this > (param other Thing)) _
    (> value (. other value))
  )
)
```

```output
Exception: Must implement == for Thing
<no file:5>
      (print (< a b))
             ^^^^^^^
```

```brix
(main
  (let a (call Thing 10))
  (let b (call Thing 10))
  (print (== a b))
  (print (!= a b))
)

(type + Thing
  (prop + value Int)
  (fn-do + This _
    (param value _)
  )
  (fn-expr + (infix this == (param other Thing)) _
    (== value (. other value))
  )
)
```

```output
true
false
```

```brix
(main
  (let a (call Thing 10))
  (let b (call Thing 10))
  (print (== a b))
  (print (!= a b))
)

(type + Thing
  (prop + value Int)
  (fn-do + This _
    (param value _)
  )
  (fn-expr + (infix this != (param other Thing)) _
    (!= value (. other value))
  )
)
```

```output
true
false
```

```brix
(main
  (let foo (call Foo 10))
  (let bar (call Bar 10))
  (print (> foo bar))
  (print (< foo bar))
  (print (== foo bar))
  (print (> bar foo))
  (print (< bar foo))
  (= (. bar value) 11)
  (print (> foo bar))
  (print (< foo bar))
  (print (== foo bar))
  (print (> bar foo))
  (print (< bar foo))
)

(type + Foo
  (prop + value Int)
  (fn-do + This _
    (param value _)
  )
  (fn-expr + (infix this > (param other _)) _
    (> value (. other value))
  )
)

(type + Bar
  (prop + value Int)
  (fn-do + This _
    (param value _)
  )
  (fn-expr + (infix this == (param other _)) _
    (== value (. other value))
  )
)
```

```output
false
false
true
false
false
false
true
false
true
false
```

### Comparison ops require Boolean return value

```brix crash
(main
  (let a (call Thing))
  (print (> a 1))
)

(type + Thing
  (fn-expr + (infix this > (param other _)) String
    "hi"
  )
)
```

```output
Exception: Operator must return Bool
<no file:8>
      (fn-expr + (infix this > (param other _)) String
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A comparison operator answers a Bool, which text does not become unasked.

```brix crash
(main
  (let a (call Thing))
  (print (> a 1))
  (print (< a 1))
)

(type + Thing
  (fn-expr + (infix this > (param other _)) _
    "hi"
  )
  (fn-expr + (infix this < (param other _)) _
    ""
  )
)
```

```output
Exception: Expected Bool here, but this is String, which becomes Bool only when asked; write "(as "hi" Bool)"
<no file:10>
        "hi"
        ^^^^

Exception: Expected Bool here, but this is String, which becomes Bool only when asked; write "(as "" Bool)"
<no file:13>
        ""
        ^^
```

```brix
(main
  (let a (call Thing))
  (let b (call Thing))
  (print (is-in a a))
  (print (is-in a b))
  (blank)
  (print (not-in a a))
  (print (not-in a b))
)

(type + Thing
  (fn-expr + (infix this has (param other _)) _
    (== this other)
  )
  (fn-expr + (infix this has-not (param other _)) _
    (!= this other)
  )
)
```

```output
true
false
false
true
```

### "has not" deduced from "has" operator

```brix
(main
  (let a (call Thing 10))
  (print (is-in 10 a))
  (print (not-in 10 a))
)

(type + Thing
  (prop + value _ (= 0))
  (fn-do + This _
    (param value _)
  )
  (fn-expr + (infix this has (param other _)) _
    (== value other)
  )
)
```

```output
true
false
```

### "has" deduced from "has not" operator

```brix
(main
  (let a (call Thing 10))
  (print (is-in 10 a))
  (print (not-in 10 a))
)

(type + Thing
  (prop + value _ (= 0))
  (fn-do + This _
    (param value _)
  )
  (fn-expr + (infix this has-not (param other _)) _
    (!= value other)
  )
)
```

```output
true
false
```

### Error if both "is in" and "not in" not supported and you call one

```brix crash
(main
  (let a (call Thing))
  (let b (call Thing))
  (print (not-in a b))
)

(type + Thing)
```

```output
Exception: Operator "not in" is not defined for Thing and Thing
<no file:5>
      (print (not-in a b))
             ^^^^^^^^^^^^
```

## Prefix operators.

### Truth override

```brix
(main
  (let thing (call Thing))
  (print (not thing))
)

(type + Thing
  (fn-expr + (prefix ! this) _
    true
  )
)
```

```output
true
```

### Truth override in and expressions

```brix
(main
  (let a (call Thing 1))
  (let b (call Thing 11))
  (print (and a true))
  (print (and true a))
  (print (and b true))
  (print (and true b))
)

(type + Thing
  (prop + a _ (= 0))
  (fn-do + This _
    (param n _)
    (= a n)
  )
  (fn-expr + (prefix ! this) _
    (<= a 10)
  )
)
```

```output
false
false
true
true
```

```brix
(main
  (let a (call Thing 1))
  (let b (call Thing 11))
  (print (or a false))
  (print (or false a))
  (print (or b false))
  (print (or false b))
)

(type + Thing
  (prop + a _ (= 0))
  (fn-do + This _
    (param n _)
    (= a n)
  )
  (fn-expr + (prefix ! this) _
    (<= a 10)
  )
)
```

```output
false
false
true
true
```

### Truth override in if statement

```brix
(main
  (let a (call Thing 1))
  (let b (call Thing 11))
  (if a
    (print "hi")
    (else-if b
      (print "oh")
    )
    (else
      (print "bye")
    )
  )
)

(type + Thing
  (prop + a _ (= 0))
  (fn-do + This _
    (param n _)
    (= a n)
  )
  (fn-expr + (prefix ! this) _
    (<= a 10)
  )
)
```

```output
oh
```

### Truth override in if expression

```brix
(main
  (let a (call Thing 1))
  (let b (call Thing 11))
  (print (if-expr a "hi a" "bye a"))
  (print (if-expr b "hi b" "bye b"))
)

(type + Thing
  (prop + a _ (= 0))
  (fn-do + This _
    (param n _)
    (= a n)
  )
  (fn-expr + (prefix ! this) _
    (<= a 10)
  )
)
```

```output
bye a
hi b
```

### Truth override in while loop

```brix
(main
  (let thing (call Thing))
  (while thing
    (print (. thing a))
    (-= (. thing a) 1)
  )
)

(type + Thing
  (prop + a _ (= 3))
  (fn-expr + (prefix ! this) _
    (== a 0)
  )
)
```

```output
3
2
1
```

```brix crash
(main
  (let thing (call Thing))
  (print (not thing))
)

(type + Thing
  (prop + a _ (= 0))
  (fn-expr + (prefix ! this) I32
    2
  )
)
```

```output
Exception: Operator must return Bool
<no file:9>
      (fn-expr + (prefix ! this) I32
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### An operator that answers a truth answers a Bool, which a number does not become unasked.

```brix crash
(main
  (let thing (call Thing))
  (print (not thing))
  (= (. thing a) 5)
  (print (not thing))
)

(type + Thing
  (prop + a _ (= 0))
  (fn-expr + (prefix ! this) _
    a
  )
)
```

```output
Exception: Expected Bool here, but this is I64, which becomes Bool only when asked; write "(as a Bool)"
<no file:12>
        a
        ^
```

### An operator that answers a truth answers a Bool, which an object does not become unasked.

```brix crash
(main
  (let thing (call Thing))
  (print (not thing))
  (= (. thing state) (call State false))
  (print (not thing))
)

(type + State
  (prop + truth _ (= false))
  (fn-do + This _
    (param t _)
    (= truth t)
  )
  (fn-expr + (prefix ! this) _
    (not truth)
  )
)

(type + Thing
  (prop + state _ (= (call State true)))
  (fn-expr + (prefix ! this) _
    state
  )
)
```

```output
Exception: Expected Bool here, but this is State, which becomes Bool only when asked; write "(as state Bool)"
<no file:23>
        state
        ^^^^^
```

```brix
(main
  (let thing (call Thing))
  (let nthing (- thing))
  (print (. nthing a))
)

(type + Thing
  (prop + a _ (= 5))
  (fn-do + (prefix - this) _
    (let neg (call Thing))
    (= (. neg a) (- a))
    (return neg)
  )
)
```

```output
-5
```

## Index operators.

```brix
(main
  (let thing (call Thing))
  (print ([] thing 10))
)

(type + Thing
  (fn-expr + (index-get this (param index _)) _
    (+ index 5)
  )
)
```

```output
15
```

```brix
(main
  (let thing (call Thing))
  (= ([] thing 10) 5)
  (print (. thing a))
)

(type + Thing
  (prop + a _ (= 10))
  (fn-do + (index-set this (param index _) (param value _)) _
    (= a (+ index value))
  )
)
```

```output
15
```

```brix
(main
  (let thing (call Thing))
  (-= ([] thing 1))
  (print (. thing a))
)

(type + Thing
  (prop + a _ (= 10))
  (fn-do + (index-delete this (param index _)) _
    (-= a index)
  )
)
```

```output
9
```

## Slice operators.

```brix
(main
  (let thing (call Thing))
  (print ([] thing (..< 10 100 (by: 2))))
)

(type + Thing
  (fn-expr + (index-get this (param range (of Range ''T))) _
    (+ (. range start) (. range end) (. range increment))
  )
)
```

```output
112
```

```brix
(main
  (let thing (call Thing))
  (print ([] thing (..< 10 100)))
)

(type + Thing
  (fn-expr + (index-get this (param range (of Range ''T))) _
    (+ (. range start) (. range end) (. range increment))
  )
)
```

```output
111
```

```brix
(main
  (let thing (call Thing))
  (= ([] thing (..< 10 100 (by: 1))) 5)
  (print (. thing a))
)

(type + Thing
  (prop + a _ (= 10))
  (fn-do + (index-set this (param range (of Range ''T)) (param value _)) _
    (= a (+ (. range start) (. range end) (. range increment) value))
  )
)
```

```output
116
```

```brix
(main
  (let thing (call Thing))
  (-= ([] thing (..< 10 100 (by: 1))))
  (print (. thing a))
)

(type + Thing
  (prop + a _ (= 10))
  (fn-do + (index-delete this (param range (of Range ''T))) _
    (-= a (+ (. range start) (. range end) (. range increment)))
  )
)
```

```output
-101
```

## Call operators

```brix
(main
  (let thing (call Thing))
  (call thing)
  (call thing 42)
)

(type + Thing
  (fn-do + this _
    (print "Called")
  )
  (fn-do + this _
    (param a _)
    (print (str "Called " a))
  )
)
```

```output
Called
Called 42
```

## A number on the left of an operator

### A number on the left of `*` calls the `*` an extension of its type declares for the other operand.

```brix
(main
  (print (* (call Three) 5))
  (print (* 5 (call Three)))
  (print (* 2.5 (call Three)))
)

(type + Three
  (fn-expr + (infix this * (param that _)) _
    (* 3 that)
  )
)

(extend + Number
  (fn-expr + (infix this * (param that Three)) _
    (* that this)
  )
)
```

```output
15
15
7.5
```

### A number on the left of `+` calls the `+` an extension of its type declares for the other operand.

```brix
(main
  (print (+ (call Ten) 5))
  (print (+ 5 (call Ten)))
)

(type + Ten
  (fn-expr + (infix this + (param that _)) _
    (+ 10 that)
  )
)

(extend + Number
  (fn-expr + (infix this + (param that Ten)) _
    (+ that this)
  )
)
```

```output
15
15
```

### A number assigned with `+=` calls the `+` an extension of its type declares for the other operand.

```brix
(main
  (let x 5)
  (+= x (call Ten))
  (print x)
)

(type + Ten
  (fn-expr + (infix this + (param that _)) _
    (+ 10 that)
  )
)

(extend + Number
  (fn-expr + (infix this + (param that Ten)) _
    (+ that this)
  )
)
```

```output
15
```

### The right operand is never asked for an operator the left one lacks: a number on the left needs its own.

```brix compile-error
(main
  (print (* 5 (call Three)))
)

(type + Three
  (fn-expr + (infix this * (param that _)) _
    (* 3 that)
  )
)
```

```output
Exception: Expected I64 here, but this is Three, and there is no conversion between them
<no file:3>
      (print (* 5 (call Three)))
                  ^^^^^^^^^^^^
```

### A list given text to add is an error: its `+` takes an item, and the text is not joined to it reversed.

```brix compile-error
(main
  (print (+ (list 1 2 3) "1"))
)
```

```output
Exception: Operator "+" is not defined for [I64] and String
<no file:3>
      (print (+ (list 1 2 3) "1"))
             ^^^^^^^^^^^^^^^^^^^^
```

### Text on the right of a number is not added to it, nor joined to it reversed.

```brix compile-error
(main
  (print (+ 5 "1"))
)
```

```output
Exception: Expected I64 here, but this is String, and there is no conversion between them
<no file:3>
      (print (+ 5 "1"))
                  ^^^
```

### An ordering no function answers is the opposite of the ordering that negates it.

```brix
(main
  (print (<= "a" "b"))
  (print (<= "b" "a"))
  (print (<= "a" "a"))
  (print (>= "a" "b"))
  (print (>= "b" "a"))
  (print (>= "a" "a"))
)
```

```output
true
false
true
false
true
true
```

### `<=` deduced from `>=` and `==` holds when the two are equal.

```brix
(main
  (let a (call Thing 1))
  (let b (call Thing 1))
  (let c (call Thing 2))
  (print (<= a b))
  (print (<= a c))
  (print (<= c a))
  (print (>= a b))
  (print (< a c))
  (print (< c a))
  (print (< a b))
)

(type + Thing
  (prop + value Int)
  (fn-do + This _
    (param value _)
  )
  (fn-expr + (infix this >= (param other Thing)) _
    (>= value (. other value))
  )
  (fn-expr + (infix this == (param other Thing)) _
    (== value (. other value))
  )
)
```

```output
true
true
false
true
true
false
false
```

### An operator a subclass declares again answers for its objects, held as the base class.

```brix
(main
  (print (== (as (call B) A) (call A)))
  (print (== (call A) (call A)))
  (print (!= (as (call B) A) (call A)))
  (print (== (call A) (call B)))
  (print (++ (as (call B) A) "!"))
  (print (++ (call A) "!"))
)

(type + A
  (fn-do + This _)
  (fn-expr + (infix this == (param other A)) _
    true
  )
  (fn-expr + (infix this ++ (param other String)) _
    (++ "a" other)
  )
)

(type + B (is A)
  (fn-do + This _)
  (fn-expr + (infix this == (param other A)) _
    false
  )
  (fn-expr + (infix this ++ (param other String)) _
    (++ "b" other)
  )
)
```

```output
false
true
true
true
b!
a!
```

### An assigning operator on a property a method names alone calls the function the property's class declares, and reads the property as it is written.

```brix
(main
  (let box (call Box))
  (call (. box grow))
  (call (. box grow))
  (print (. box count))
  (print (. box label))
)

(type + Counter
  (prop + n _ (= 0))
  (fn-do + (infix this += (param other I32)) _
    (+= n other)
  )
)

(type + Box
  (prop + held _ (= (call Counter)))
  (prop + label _ (= "box"))
  (fn-do + grow _
    (+= held 10)
    (+= label "!")
  )
  (fn-expr + count _ :get
    (. held n)
  )
)
```

```output
20
box!!
```

### A value held by a name is called through the function its type declares for a call

```brix
(main
  (let parts (list "x" "y" "z"))
  (print (call "-" parts))
  (print (call "" parts))
  (let glue " + ")
  (print (call glue (in (* each 2) (list 1 2 3))))
  (let adder (call Adder 10))
  (print (call adder 5))
  (print (call adder (:factor 7)))
  (let shout (call Shout))
  (call shout "hey")
)

(type + Adder
  (prop + base Int)
  (fn-do + This _
    (param base _)
  )
  (fn-expr + this Int
    (param n Int)
    (+ base n)
  )
  (fn-expr + this Int
    (param factor Int)
    (* base factor)
  )
)

(type + Shout
  (fn-do + this _
    (param text String)
    (print (str text "!"))
  )
)
```

```output
x-y-z
xyz
2 + 4 + 6
50
70
hey!
```

### The fallback operator keeps a tuple its truth test passes and takes the other where it fails.

```brix
(main
  (print (. (?? (call Reach 3 9) (call Reach 1 1)) to))
  (print (. (?? (call Reach 4 4) (call Reach 1 2)) to))
  (print (. (call wider (call Reach 5 5) (call Reach 6 8)) to))
  (print (. (call wider (call Reach 5 7) (call Reach 6 8)) to))
)

(type + Reach (is Tuple)
  (prop + from Int)
  (prop + to Int)
  (fn-expr + (prefix ! this) _
    (== from to)
  )
)

(fn-expr + wider Reach
  (param reach Reach)
  (param other Reach)
  (?? reach other)
)
```

```output
9
2
8
7
```

### A method named without a call is a function value, which has no truth to test.

```brix crash
(main
  (let box (call Box))
  (if (. box has-items)
    (print "items")
  )
)

(type + Box
  (prop + items (list Int))
  (fn-do + This _)
  (fn-expr + has-items Bool
    (> (. items length) 0)
  )
)
```

```output
Exception: function has-items cannot be tested as a condition; it is a function, so call it first
<no file:4>
      (if (. box has-items)
          ^^^^^^^^^^^^^^^^^
```

### An object of a class that does not declare `!this` has no truth to test or deny.

```brix crash
(main
  (let thing (call Thing))
  (if thing
    (print "thing")
  )
)

(type + Thing)
```

```output
Exception: Thing cannot be tested as a condition, since operator "!" is not defined for it
<no file:4>
      (if thing
          ^^^^^
```

```brix crash
(main
  (let thing (call Thing))
  (print (not thing))
)

(type + Thing)
```

```output
Exception: Operator "!" is not defined for Thing
<no file:4>
      (print (not thing))
             ^^^^^^^^^^^
```

### `!` of a function value has no truth to deny.

```brix crash
(main
  (let box (call Box))
  (print (str (not (. box has-items))))
)

(type + Box
  (prop + items (list Int))
  (fn-do + This _)
  (fn-expr + has-items Bool
    (> (. items length) 0)
  )
)
```

```output
Exception: Operator "!" is not defined for function has-items; it is a function, so call it first
<no file:4>
      (print (str (not (. box has-items))))
                  ^^^^^^^^^^^^^^^^^^^^^^^
```

### A range, a tuple, a table or a builder that does not say whether it is empty cannot be tested as a condition.

```brix crash
(main
  (let r (.. 0 3))
  (if r
    (print "yes")
  )
)
```

```output
Exception: Range'I64 cannot be tested as a condition, since operator "!" is not defined for it
<no file:4>
      (if r
          ^
```

### A table has no truth of its own either.

```brix crash
(import /core/table)
(blank)

(main
  (let t (call (of Table Point) 3))
  (print (only-if "made" (if t)))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
Exception: Table'Point cannot be tested as a condition, since operator "!" is not defined for it
<no file:7>
      (print (only-if "made" (if t)))
             ^^^^^^^^^^^^^^^^^^^^^^^
```

### Values ordered by "<" need an operator that orders them; two lists, tuples or atoms have none.

```brix crash
(main
  (print (< "a" "b"))
  (let xs (list 1 2))
  (let ys (list 1 3))
  (print (< xs ys))
)
```

```output
Exception: Operator "<" is not defined for [I64] and [I64]
<no file:6>
      (print (< xs ys))
             ^^^^^^^^^
```
