# Structure match

```brix
(main
  (print (call matched-name null))
  (print (call matched-name (call NamedMatchNode)))
  (print (call matched-name (call MatchNode)))
)

(type + MatchNode)

(type + NamedMatchNode (is MatchNode)
  (prop + name _ (= "named"))
)

(fn-expr + matched-name _
  (param node (? MatchNode))
  (match node
    (case= null
      "null"
    )
    (case (: named NamedMatchNode)
      (. named name)
    )
    (default
      "other"
    )
  )
)
```

```output
null
named
other
```

### An arm that is a name alone takes every value, so no arm may follow it.

```brix compile-error
(main
  (print (call first-name (call MatchNode)))
)

(type + MatchNode)

(type + NamedMatchNode (is MatchNode)
  (prop + name _ (= "named"))
)

(fn-expr + first-name _
  (param node (? MatchNode))
  (match node
    (case any
      "first"
    )
    (case (: named NamedMatchNode)
      (. named name)
    )
    (default
      "other"
    )
  )
)
```

```output
Exception: Arm "(case any …)" takes every value, so the arms after it are never reached
<no file:15>
        (case any
              ^^^
```

### An arm that is a name alone binds the name even where the subject has a case of that name.

```brix compile-error
(main
  (print (call describe (. Light green)))
)

(type + Light (is Union)
  (variant red)
  (variant green)
)

(fn-expr + describe String
  (param light Light)
  (match light
    (case (. Light green)
      "green"
    )
    (case red
      "red"
    )
  )
)
```

```output
Exception: Arm "(case red …)" binds a new name, not the case Light.red; match the case with "(case .red …)"
<no file:17>
        (case red
              ^^^
```

```brix
(main
  (print (is 42 Int))
  (print (is! 42 String))
)
```

```output
true
false
```

```brix
(main
  (let selected 0)
  (let trace "")
  (blank)
  (match 2
    (case= 1
      (= selected 10)
      (+= trace "one")
    )
    (case= 2
      (= selected 20)
      (+= trace "two")
      (+= selected 2)
    )
    (default
      (= selected 30)
      (+= trace "default")
    )
  )
  (blank)
  (print selected)
  (print trace)
)
```

```output
22
two
```

```brix
(main
  (let selected 0)
  (blank)
  (match 7
    (case? (< it 0)
      (= selected -1)
    )
    (case? (> it 5)
      (let offset 40)
      (= selected (+ it offset))
    )
    (default
      (= selected 1)
    )
  )
  (blank)
  (print selected)
)
```

```output
47
```

```brix
(main
  (let selected 4)
  (blank)
  (match? 7
    (case= 1
      (= selected 10)
    )
    (case= 2
      (= selected 20)
    )
  )
  (blank)
  (print selected)
)
```

```output
4
```

```brix
(main
  (do-new
    (match 8
      (case? (> it 10)
        (print "unreachable")
      )
    )
    (on-failure failure
      (print failure)
    )
  )
)
```

```output
Pattern match failed
```

```brix
(main
  (do-new
    (match 8
      (case? (> it 10)
        (print "unreachable")
      )
      (throw (call Panic "too much"))
    )
    (on-failure failure
      (print failure)
    )
  )
)
```

```output
too much
```

```brix
(main
  (print (call choose 1))
  (print (call choose 2))
)

(fn-do + choose I32
  (param value I32)
  (match value
    (case= 1
      (return 10)
    )
    (default
      (return 20)
    )
  )
)
```

```output
10
20
```

```brix
(main
  (let total 0)
  (blank)
  (for (in value (list 1 2 3 4))
    (match value
      (case= 2
        (continue)
      )
      (case= 4
        (break)
      )
      (default
        (+= total value)
      )
    )
  )
  (blank)
  (print total)
)
```

```output
4
```

```brix
(main
  (let counter (call Counter))
  (let selected 0)
  (blank)
  (match (call (. counter next))
    (case= 1
      (= selected 10)
    )
    (case= 2
      (= selected 20)
    )
    (default
      (= selected 30)
    )
  )
  (blank)
  (print (. counter count))
  (print selected)
)

(type + Counter
  (prop + count I32)
  (fn-do + next I32
    (+= count 1)
    (return 2)
  )
)
```

```output
1
20
```

```brix
(main
  (let event
    (match true
      (case= true
        (call ..opened 7)
      )
      (default
        ..closed
      )
    )
  )
  (let selected 0)
  (blank)
  (match event
    (case (call .opened value)
      (= selected value)
    )
    (case .closed
      (= selected -1)
    )
  )
  (blank)
  (print selected)
)

(type + Event (is Union)
  (variant opened I32)
  (variant closed)
)
```

```output
7
```

```brix
(main
  (match 1
    (case= 1
      42
    )
    (default
      "unused"
    )
  )
  (blank)
  (print "done")
)
```

```output
done
```

### A number is not a Bool, but converts to one.

```brix
(main
  (print (= (select it (| (similar Bool))) 4.5))
  (print (is 42 Bool))
  (print (is~ 42 Bool))
)
```

```output
true
false
true
```

```brix
(main
  (print
    (match 42
      (case Int
        (* it 2)
      )
    )
  )
)
```

```output
84
```

```brix
(main
  (print
    (match 42
      (case (exact String)
        (* it 2)
      )
      (default
        (inline 10)
      )
    )
  )
)
```

```output
10
```

```brix crash
(main
  (print
    (match 42
      (case (exact String)
        (* it 2)
      )
    )
  )
)
```

```output
Exception: Pattern has no possible matches
<no file:5>
          (case (exact String)
          ^^^^^^^^^^^^^^^^^^^
```

```brix
(main
  (print
    (match 42
      (case (: a Int)
        (* a 2.0)
      )
    )
  )
)
```

```output
84
```

```brix
(main
  (let x
    (match? 42
      (case ({} (> it 20)))
    )
  )
  (print (type-of x))
  (print x)
)
```

```output
[TypeInfo Optional'I64]
42
```

```brix
(main
  (let x
    (match 42
      (case ({} (> it 20)))
      (default
        (inline 1)
      )
    )
  )
  (print (type-of x))
  (print x)
)
```

```output
[TypeInfo I64]
42
```

```brix
(main
  (print
    (match 42
      (case Int
        "Ok"
      )
      (default
        "Not"
      )
    )
  )
)
```

```output
Ok
```

```brix
(main
  (print
    (match 42
      (case 42
        "Ok"
      )
      (default
        "Not"
      )
    )
  )
)
```

```output
Ok
```

```brix
(main
  (print
    (match 42
      (case ({} (> it 5))
        "Ok"
      )
      (default
        "Not"
      )
    )
  )
)
```

```output
Ok
```

```brix
(main
  (print
    (match 42
      (case (all Int ({} (> it 5)))
        "Ok"
      )
      (default
        "Not"
      )
    )
  )
)
```

```output
Ok
```

```brix
(main
  (print
    (match 42
      (case (| String ({} (> it 5)))
        "Ok"
      )
      (default
        "Not"
      )
    )
  )
)
```

```output
Ok
```

```brix
(main
  (print
    (match 42
      (case (| ({} (> it 5)) String)
        "Ok"
      )
      (default
        "Not"
      )
    )
  )
)
```

```output
Ok
```

```brix
(main
  (print
    (match 42
      (case (| (exact String) List)
        "Ok"
      )
      (default
        (inline "Not")
      )
    )
  )
)
```

```output
Not
```

```brix crash
(main
  (print
    (match 42
      (case (| (exact String) List)
        "Ok"
      )
    )
  )
)
```

```output
Exception: Pattern has no possible matches
<no file:5>
          (case (| (exact String) List)
          ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix
(main
  (print
    (match 42
      (case (exact String)
        "Black Hole"
      )
      (case Int
        "Universe"
      )
      (default
        "Nope"
      )
    )
  )
)
```

```output
Universe
```

```brix
(main
  (print (call test (inline (tuple "foo" 42))))
  (print (call test (inline (tuple "bar" "foo"))))
  (print (call test (inline (tuple 42 "bar"))))
  (print (call test (inline 24)))
)

(fn-expr + test _
  (param expr _)
  (match expr
    (case (tuple (exact String) (exact Int))
      "Black Hole"
    )
    (case Int
      "Universe"
    )
    (default
      "Nope"
    )
  )
)
```

```output
Black Hole
Nope
Nope
Universe
```

```brix
(main
  (print
    (match (list 1 2 3)
      (case (list ... last)
        last
      )
    )
  )
  (print
    (match (list 1 2 3)
      (case (list first ...)
        first
      )
    )
  )
  (print
    (match (list 1 2 3)
      (case (list first ... last)
        (+ first last)
      )
    )
  )
)
```

```output
3
1
4
```

```brix
(main
  (print
    (match (tuple "foo" 42)
      (case (tuple (: a String) (: b Int))
        (str a " = " b)
      )
      (default
        "Nope"
      )
    )
  )
)
```

```output
foo = 42
```

```brix
(main
  (print
    (match "foo"
      (case (: a String)
        key
        (lazy key a)
      )
      (default
        (inline "Nope")
      )
    )
  )
)
```

```output
foo
```

```brix
(main
  (print
    (match (list 1 2 3)
      (case Int
        "num"
      )
      (case (: x List)
        (str foo)
        (lazy foo (call (. "," join) x))
      )
      (default
        "Nope"
      )
    )
  )
)
```

```output
1,2,3
```

```brix
(main
  (print
    (match (tuple "foo" 42)
      (case (tuple (: a String) (: b Int))
        key
        (lazy key (str a " = " b))
      )
      (default
        (inline "Nope")
      )
    )
  )
)
```

```output
foo = 42
```

```brix
(main
  (print
    (match (tuple "foo" 42)
      (case (tuple (: a String) (: b ({} (> it 20))))
        key
        (lazy key (str a " = " b))
      )
      (default
        (inline "Nope")
      )
    )
  )
)
```

```output
foo = 42
```

```brix
(main
  (print
    (match (list 42)
      (case (: a List)
        key
        (lazy key (call (. ", " join) value))
        (lazy value (in (* each 2) a))
      )
      (default
        (inline "Nope")
      )
    )
  )
)
```

```output
84
```

```brix
(main
  (print (call foo))
)

(fn-expr + foo _
  (case? (is 42 List)
    1
  )
  (default
    y
    (lazy y 2)
  )
)
```

```output
2
```

```brix
(main
  (print
    (match (list 42)
      (case (: x List)
        (str "list " x)
      )
      (case Int
        "num"
      )
    )
  )
)
```

```output
list 42
```

```brix
(main
  (let v 2)
  (print
    (match v
      (case 3
        13
      )
      (case 2
        12
      )
      (case 1
        11
      )
      (default
        14
      )
    )
  )
)
```

```output
12
```

```brix
(main
  (let opcode U8 3u8)
  (print
    (match opcode
      (case 0
        "load"
      )
      (case 1
        "add"
      )
      (case 2
        "store"
      )
      (case 3
        "halt"
      )
      (default
        "unknown"
      )
    )
  )
)
```

```output
halt
```

```brix
(main
  (let opcode U8 8u8)
  (print
    (match opcode
      (case 0
        "load"
      )
      (case 3
        "add"
      )
      (case 8
        "store"
      )
      (case 16
        "halt"
      )
      (default
        "unknown"
      )
    )
  )
)
```

```output
store
```

```brix
(main
  (let v 1)
  (print
    (match v
      (case? (> it 0)
        10
      )
      (default
        12
      )
    )
  )
)
```

```output
10
```

```brix
(main
  (let attempt
    (do-new
      (print (call check 1))
      (print (call check 3))
      (print (call check 2))
      (on-failure failure
        (print (str "Error: " failure))
      )
    )
  )
  (call (. attempt wait))
)

(fn-expr + check _
  (param v _)
  (match v
    (case 1
      10
    )
    (throw (call Panic "Failed") (if (== it 2)))
    (default
      12
    )
  )
)
```

```output
10
12
Error: Failed
```

```brix
(main
  (do-new
    (print
      (match 8
        (case? (> it 10)
          10
        )
        (throw (call Panic "too much"))
      )
    )
    (on-failure failure
      (print failure)
    )
  )
)
```

```output
too much
```

```brix
(main
  (print
    (call (. ", " join)
      (:stream
        (iterate (in (list 1 2 3))
          (block
            (case (: n Int)
              (* n 2)
            )
            (default
              0
            )
          )
        )
      )
    )
  )
)
```

```output
2, 4, 6
```

```brix
(main
  (let iter1
    (iterate (in (list 1 2 3 2 4))
      (block
        (<* ({} (> it 2)))
      )
    )
  )
  (print (type-of iter1))
  (print (call ", " iter1))
  (blank)
  (let iter2 (case-each (in ({} (> it 2)) (list 1 2 3))))
  (print (type-of iter2))
  (print (call ", " iter2))
)
```

```output
[TypeInfo Generator'I64]
3, 4
[TypeInfo Generator'I64]
3
```

### Output of each pattern goes to the next one in "and" sequence

```brix
(main
  (print
    (match 10foo
      (case (all (similar Bar) ({} (> it 20))))
    )
  )
  (blank)
  (print
    (match 10foo
      (case (all (similar Bar) ({} (> it 20)) (similar Foo) ({} (< it 30))))
    )
  )
)

(blank)
(type-line + Foo (alias foo) (is F64))
(type-line + Bar (alias bar) (is I32))
(blank)
(unit + 1bar 0.333333333333foo)
```

```output
30
9.99999999999
```

```brix
(main
  (print
    (match (call Foo2)
      (case Foo2
        1
      )
      (default
        2
      )
    )
  )
  (blank)
  (print
    (match (call Foo)
      (case Foo2
        1
      )
      (default
        2
      )
    )
  )
)

(type + Foo)

(type + Foo2 (is Foo))
```

```output
1
2
```

## Imperative conditional matches

```brix
(main
  (if (= (| (: foo ({} (== it 42)))) 42)
    (print (str "pass " foo))
    (print "ok")
    (else
      (print "fail")
    )
  )
)
```

```output
pass 42
ok
```

```brix
(main
  (if (= (| (: foo ({} (== it 43)))) 42)
    (print (str "pass " foo))
    (else
      (print "fail")
    )
  )
)
```

```output
fail
```

```brix
(main
  (let x 0)
  (while (= (| (: foo ({} (< it 3)))) x)
    (print (str "pass " foo))
    (+= x 1)
  )
)
```

```output
pass 0
pass 1
pass 2
```

## Optional matching

```brix
(main
  (let foo (? Int) 42)
  (print (case (= Int foo)))
  (print (case (= (all Int ({} (> it 40))) foo)))
)
```

```output
42
42
```

## Property matching

```brix
(main
  (let ..length "foo")
  (print length)
)
```

```output
3
```

```brix
(main
  (let (: x ..length) "foo")
  (print x)
)
```

```output
3
```

```brix crash
(main
  (let (: x ..length) "foo")
  (print length)
)
```

```output
Exception: Name "length" not found
<no file:4>
      (print length)
             ^^^^^^
```

```brix crash
(main
  (let ..nada "foo")
)
```

```output
Exception: Pattern has no possible matches
<no file:3>
      (let ..nada "foo")
           ^^^^^^
```

```brix
(main
  (do-new
    (let (: y (all ..length ({} (== it 1)))) "f")
    (print y)
    (blank)
    (let (: x (all ..length ({} (== it 3)))) "f")
    (print x)
    (on-failure failure
      (print failure)
    )
  )
)
```

```output
1
Pattern match failed
```

### Two patterns binding same variable name should not conflict

```brix
(main
  (let foo (call Foo "hi"))
  (blank)
  (print
    (match foo
      (case (: a (similar Int))
        a
      )
      (case (: a (similar String))
        10
      )
    )
  )
)

(type + Foo (is Union)
  (variant Int)
  (variant String)
)
```

```output
10
```

```brix
(main
  (print
    (match 42
      (case= 4
        1
      )
      (case= 42
        2
      )
      (default
        3
      )
    )
  )
)
```

```output
2
```

```brix
(main
  (print
    (match 1
      (case= 4
        1
      )
      (case= 42
        2
      )
      (default
        3
      )
    )
  )
)
```

```output
3
```

```brix
(main
  (print
    (match? 1
      (case= 4
        1
      )
      (case= 42
        2
      )
    )
  )
)
```

```output
unknown
```

### Integer match compares against constants that are not add/sub immediates

```brix
(main
  (print (call few -3))
  (print (call few -4))
  (print (call few 7))
  (print (call many -3))
  (print (call many 5000))
  (print (call many 70000))
  (print (call many -100000))
  (print (call many 2))
  (print (call many 9))
)

(fn-expr + few String
  (param status I32)
  (match status
    (case= -3
      "minus three"
    )
    (case= -4
      "minus four"
    )
    (default
      "other"
    )
  )
)

(fn-expr + many String
  (param value I64)
  (match value
    (case= -100000
      "minus hundred thousand"
    )
    (case= -3
      "minus three"
    )
    (case= 2
      "two"
    )
    (case= 5000
      "five thousand"
    )
    (case= 70000
      "seventy thousand"
    )
    (default
      "other"
    )
  )
)
```

```output
minus three
minus four
other
minus three
five thousand
seventy thousand
minus hundred thousand
two
other
```

### A match no arm of which matches panics where its first arm is written

```brix
(main
  (do-new
    (let value 6)
    (print
      (match value
        (case= 4
          "four"
        )
        (case= 5
          "five"
        )
      )
    )
    (on-failure failure
      (print failure)
    )
  )
)
```

```output
Pattern match failed
```

### A match statement panics where no arm matches, and one with ? goes on

```brix
(main
  (do-new
    (let value 6)
    (match value
      (case= 6
        (print "six")
      )
    )
    (match? value
      (case= 4
        (print "four")
      )
    )
    (print "after")
    (match value
      (case= 4
        (print "four")
      )
      (case= 5
        (print "five")
      )
    )
    (print "unreachable")
    (on-failure failure
      (print failure)
    )
  )
)
```

```output
six
after
Pattern match failed
```

### A type with a condition that fails leaves the match unmatched

```brix
(main
  (do-new
    (print (call size 7))
    (print (call size 1))
    (on-failure failure
      (print failure)
    )
  )
)

(fn-expr + size Int
  (param value Int)
  (match value
    (case (all Int ({} (> it 5)))
      it
    )
  )
)
```

```output
7
Pattern match failed
```

### A condition in a match on a subject tests a text for emptiness

```brix
(main
  (print (call label "" 5))
  (print (call label "x" 5))
  (print (call label "" 1))
)

(fn-expr + label String
  (param name String)
  (param count Int)
  (match count
    (case? name
      "named"
    )
    (case? (> it 3)
      "big"
    )
    (default
      "small"
    )
  )
)
```

```output
big
named
small
```

### A condition in a match with no subject tests a list and a text for emptiness

```brix
(main
  (let none (list Int))
  (print (call pick none ""))
  (print (call pick (list 1) ""))
  (print (call pick none "n"))
)

(fn-expr + pick String
  (param items (list Int))
  (param name String)
  (match true
    (case? items
      "items"
    )
    (case? name
      "name"
    )
    (default
      "none"
    )
  )
)
```

```output
none
items
name
```

### A condition after a type tests what the type passed of an optional

```brix
(main
  (print (call size 7))
  (print (call size 2))
  (print (call size null))
)

(fn-expr + size Int
  (param value (? Int))
  (match value
    (case (all Int ({} (> it 5)))
      it
    )
    (case (: n Int)
      (+ n 100)
    )
    (case= null
      -1
    )
  )
)
```

```output
7
102
-1
```

### A condition after a type tests the case of a union the type matched

```brix
(main
  (print (call describe (call .count 7)))
  (print (call describe (call .count 2)))
  (print (call describe (call .circle 2.0)))
  (print (call describe (call .circle 1.0)))
  (print (call describe (call .named "a")))
  (print (call describe (call .named "")))
)

(type + Shape (is Union)
  (variant circle Float)
  (variant count Int)
  (variant named String)
)

(fn-expr + describe String
  (param shape Shape)
  (match shape
    (case (all Int ({} (> it 5)))
      (str "big " it)
    )
    (case Int
      (str "small " it)
    )
    (case (all Float ({} (> it 1.5)))
      "wide"
    )
    (case (all String ({} it))
      (str "named " it)
    )
    (default
      "other"
    )
  )
)
```

```output
big 7
small 2
wide
other
named a
other
```

### A union matches text only where it holds its case of text

```brix
(main
  (let u Shape (call .circle 1.0))
  (print
    (match u
      (case (all Float ({} (> it 1.5)))
        "wide"
      )
      (case (all String ({} it))
        (str "named " it)
      )
      (default
        (inline "other")
      )
    )
  )
)

(type + Shape (is Union)
  (variant circle Float)
  (variant count Int)
  (variant named String)
)
```

```output
other
```

### A union with no case of text converts to text where it holds the case picked for text

```brix
(main
  (let u Mixed (call .count 1))
  (print
    (match u
      (case (similar String)
        (str "s " it)
      )
      (default
        (inline "other")
      )
    )
  )
  (let w Mixed (call .flag true))
  (print
    (match w
      (case (similar String)
        (str "s " it)
      )
      (default
        (inline "other")
      )
    )
  )
)

(type + Mixed (is Union)
  (variant count Int)
  (variant flag Bool)
)
```

```output
other
s true
```

### The alternatives of an arm bind the same names, which the arm reads whichever of them matched.

```brix
(main
  (print (call show (call .a 3)))
  (print (call show (call .b 4)))
  (print (call show .c))
  (print (call pick (tuple 1 2)))
  (print (call pick (tuple 5 1)))
)

(fn-expr + show String
  (param shape Shape)
  (match shape
    (case (| (call .a n) (call .b n))
      (str "n is " n)
    )
    (default
      "none"
    )
  )
)

(fn-expr + pick Int
  (param pair (tuple Int Int))
  (match pair
    (case (| (tuple 1 x) (tuple x 1))
      x
    )
    (default
      0
    )
  )
)

(type + Shape (is Union)
  (variant a Int)
  (variant b Int)
  (variant c)
)
```

```output
n is 3
n is 4
none
2
5
```

### A name the alternatives of an arm bind is of one type in each.

```brix crash
(main
  (print (call show (call .a 3)))
)

(fn-expr + show String
  (param shape Shape)
  (match shape
    (case (| (call .a n) (call .b n))
      (str "n is " n)
    )
    (default
      "none"
    )
  )
)

(type + Shape (is Union)
  (variant a Int)
  (variant b String)
)
```

```output
Exception: "n" is I64 in one alternative and String in another; bind it to one type in each
<no file:9>
        (case (| (call .a n) (call .b n))
                             ^^^^^^^^^^^
```

### A name one alternative of an arm binds is bound by the others too.

```brix crash
(main
  (print (call show (call .a 3)))
)

(fn-expr + show String
  (param shape Shape)
  (match shape
    (case (| (call .a n) .c)
      (str "n is " n)
    )
    (default
      "none"
    )
  )
)

(type + Shape (is Union)
  (variant a Int)
  (variant c)
)
```

```output
Exception: "n" is bound by the alternative before this one but not by this one; bind it in each alternative
<no file:9>
        (case (| (call .a n) .c)
                             ^^
```

### A pattern binds a name once; one written twice in it is an error, not a test that the two places are equal.

```brix crash
(main
  (print
    (match (list 1 2)
      (case (list a a)
        (str "same " a)
      )
      (default
        "other"
      )
    )
  )
)
```

```output
Exception: "a" is bound twice in this pattern; give each place a name of its own
<no file:5>
          (case (list a a)
                        ^
```

### A match whose arms give values of types that share none gives a union of them all, however many there are and in whatever order.

```brix
(main
  (for (in n (.. 1 3))
    (print (call describe n))
    (print (call weigh n))
  )
)

(fn-do + describe String
  (param n Int)
  (let value
    (match n
      (case= 1
        "one"
      )
      (case= 2
        2
      )
      (default
        2.5
      )
    )
  )
  (return (str value))
)

(fn-do + weigh String
  (param n Int)
  (let value
    (match n
      (case= 1
        2.5
      )
      (case= 2
        3
      )
      (default
        "many"
      )
    )
  )
  (return (str value))
)
```

```output
one
2.5
2
3
2.5
many
```

### Captures with types take the cases of a union

```brix
(main
  (print (call show (call .count 7)))
  (print (call show (call .wide 2.5)))
  (print (call show (call .named "n")))
  (print (call show .empty))
)

(fn-expr + show String
  (param shape Shape)
  (match shape
    (case (: n Int)
      (str "int " (+ n 1))
    )
    (case (: f Float)
      (str "float " (* f 2.0))
    )
    (case (: s String)
      (str "text " s)
    )
    (default
      "none"
    )
  )
)

(type + Shape (is Union)
  (variant count Int)
  (variant wide Float)
  (variant named String)
  (variant empty)
)
```

```output
int 8
float 5
text n
none
```

### Cases named with patterns for what they hold

```brix
(main
  (print (call show (call .count 7)))
  (print (call show (call .wide 2.5)))
  (print (call show (call .named "n")))
  (print (call show .empty))
  (print (call show (call .pair 1 "p")))
)

(fn-expr + show String
  (param shape Shape)
  (match shape
    (case (call .count n)
      (str "count " n)
    )
    (case (call .wide Float)
      (str "wide " it)
    )
    (case .named
      "named"
    )
    (case (call .pair a b)
      (str "pair " a " " b)
    )
    (case .empty
      "empty"
    )
  )
)

(type + Shape (is Union)
  (variant count Int)
  (variant wide Float)
  (variant named String)
  (variant pair (tuple (: a Int) (: b String)))
  (variant empty)
)
```

```output
count 7
wide 2.5
named
empty
pair 1 p
```

### An optional matched against null, a capture with a condition, and a type

```brix
(main
  (print (call pick null))
  (print (call pick 3))
  (print (call pick 30))
  (print (call twice null))
  (print (call twice 4))
)

(fn-expr + pick String
  (param value (? Int))
  (match value
    (case= null
      "nothing"
    )
    (case (all (: n Int) ({} (> n 10)))
      (str "big " n)
    )
    (case Int
      (str "small " it)
    )
  )
)

(fn-expr + twice (? Int)
  (param value (? Int))
  (match? value
    (case (: n Int)
      (* n 2)
    )
  )
)
```

```output
nothing
small 3
big 30
unknown
8
```

### Classes derived from the subject's class

```brix
(main
  (print (call sound (call Dog)))
  (print (call sound (call Cat)))
  (print (call sound (call Animal)))
)

(fn-expr + sound String
  (param animal Animal)
  (match animal
    (case (: d Dog)
      (. d bark)
    )
    (case Cat
      (. it meow)
    )
    (default
      "silence"
    )
  )
)

(type + Animal
  (prop + legs _ (= 4))
)

(type + Dog (is Animal)
  (prop + bark _ (= "woof"))
)

(type + Cat (is Animal)
  (prop + meow _ (= "meow"))
)
```

```output
woof
meow
silence
```

### A tuple of subjects with conditions on its fields

```brix
(main
  (print (call pair "foo" 42))
  (print (call pair "foo" 2))
  (print (call pair "bar" 42))
)

(fn-expr + pair String
  (param name String)
  (param count Int)
  (match (tuple name count)
    (case (tuple (: a String) (: b ({} (> it 20))))
      (str a " = " b)
    )
    (case (tuple (: a ({} (== it "bar"))) n)
      (str "bar " n)
    )
    (default
      (inline "Nope")
    )
  )
)
```

```output
foo = 42
Nope
bar = 42
```

### Match statements, conditions on the subject, and a struct captured

```brix
(main
  (let point (call Point 1 2))
  (print
    (match point
      (case (all (: p Point) ({} (== (. p x) 1)))
        (+ (. p x) (. p y))
      )
      (default
        (inline 0)
      )
    )
  )
  (let sum 0)
  (match (. point x)
    (case= 1
      (+= sum 10)
    )
    (case= 2
      (+= sum 20)
    )
  )
  (match? (. point y)
    (case= 7
      (+= sum 100)
    )
  )
  (print sum)
  (print (call grade 95))
  (print (call grade 85))
  (print (call grade 10))
)

(fn-expr + grade String
  (param score Int)
  (match score
    (case? (>= it 90)
      "A"
    )
    (case? (>= it 80)
      "B"
    )
    (default
      "F"
    )
  )
)

(type + Point (is (tuple (: x Int) (: y Int))))
```

```output
3
10
A
B
F
```

### A match in an arm of a match

```brix
(main
  (print (call nest (call .count 7) true))
  (print (call nest (call .count 7) false))
  (print (call nest (call .named "n") true))
)

(fn-expr + nest String
  (param shape Shape)
  (param loud Bool)
  (match shape
    (case (call .count n)
      (match loud
        (case= true
          (str "COUNT " n)
        )
        (default
          (str "count " n)
        )
      )
    )
    (case (call .named s)
      s
    )
  )
)

(type + Shape (is Union)
  (variant count Int)
  (variant named String)
)
```

```output
COUNT 7
count 7
n
```

### What a case named passes on: the union, or what its pattern passed

```brix
(main
  (let v (? Shape) (call (. Shape count) 3))
  (print
    (match v
      (case .count
        (str "count " (type-of it))
      )
      (default
        "none"
      )
    )
  )
  (let w Shape (call .named "x"))
  (print
    (match w
      (case (call .count Int)
        (str "count " it)
      )
      (case (call .named (all String ({} (> (. it length) 3))))
        (str "long " it)
      )
      (case (call .named s)
        (str "short " s " " it)
      )
    )
  )
)

(type + Shape (is Union)
  (variant count Int)
  (variant named String)
)
```

```output
count [TypeInfo Shape]
short x x
```

### An arm that raises a panic for whatever no arm before it matched

```brix
(main
  (do-new
    (let value 8)
    (let text
      (match value
        (case= 8
          "eight"
        )
        (throw (call Panic "not eight"))
      )
    )
    (print text)
    (print
      (match value
        (case= 9
          "nine"
        )
        (throw (call Panic "not nine"))
      )
    )
    (on-failure failure
      (print failure)
    )
  )
)
```

```output
eight
not nine
```

### is and is not between numbers, texts and optionals hold for the value's own type, never by conversion

```brix
(main
  (print (is 42 Int))
  (print (is! 42 String))
  (print (is 42 Bool))
  (print (is 42 String))
  (print (is! 42 Int))
  (print (is~ 42 Float))
  (print (is-not 42 Int))
  (print (is-not 4.5 String))
  (let o (? Int) 3)
  (print (is o Int))
  (let n (? Int))
  (print (is n Int))
  (print (is-not n Int))
  (print (is "x" String))
  (print (is "x" Int))
)
```

```output
true
false
false
false
true
true
false
true
true
false
true
true
false
```

### A type named with how near a value must be to it is matched over a union and over a number, and captured.

```brix
(main
  (print (call near (call Num 1)))
  (print (call near (call Num 2.5)))
  (print (call near (call Num "x")))
  (print (call exact (call Num 1)))
  (print (call exact (call Num 2.5)))
  (print (call exact (call Num "x")))
  (print (call plain 7))
  (print (call plain 8))
)

(fn-do + near String
  (param n Num)
  (return
    (match n
      (case (: a (similar Int))
        (str "int " a)
      )
      (case (: b (similar String))
        (str "text " b)
      )
      (default
        "other"
      )
    )
  )
)

(fn-do + exact String
  (param n Num)
  (return
    (match n
      (case (: a (exact Float))
        (str "float " a)
      )
      (case (: b (exact Int))
        (str "int " b)
      )
      (default
        "other"
      )
    )
  )
)

(fn-do + plain String
  (param n Int)
  (return
    (match n
      (case (all (: a (similar Int)) ({} (> it 7)))
        (str "big " a)
      )
      (case (: a (exact Int))
        (str "small " a)
      )
    )
  )
)

(type + Num (is Union)
  (variant Int)
  (variant Float)
  (variant String)
)
```

```output
int 1
other
text x
int 1
float 2.5
other
small 7
big 8
```

### Pattern conditions chained over the cases of a union

```brix
(main
  (let kinds (list Shape) (list (call (. Shape circle) 2) (call (. Shape square) 3) (. Shape dot) (call (. Shape rect) 2 5)))
  (let index 0)
  (while (< index 4)
    (call describe ([] kinds index))
    (+= index 1)
  )
)

(type + Shape (is Union)
  (variant dot)
  (variant circle (tuple (: radius Int)))
  (variant square (tuple (: side Int)))
  (variant rect (tuple (: wide Int) (: tall Int)))
)

(fn-do + describe _
  (param shape Shape)
  (if (= (| (call .circle radius)) shape)
    (print (str "circle " radius))
    (else-if (= (| (call .square side)) shape)
      (print (str "square " side))
    )
    (else-if (= (| (call .rect wide tall)) shape)
      (print (str "rect " (* wide tall)))
    )
    (else
      (print "something else")
    )
  )
  (print "done")
)
```

```output
circle 2
done
square 3
done
something else
done
rect 10
done
```

### Pattern condition that always matches

```brix
(main
  (if (= (| x) 42)
    (print (str "always " x))
    (else
      (print "never")
    )
  )
  (if (= (| (: y Int)) 7)
    (print (str "int " y))
  )
  (print "end")
)
```

```output
always 42
int 7
end
```

### Break and continue in a pattern loop inside another loop

```brix
(main
  (let outer 0)
  (while (< outer 3)
    (let inner 0)
    (while (= (| (: step ({} (< it 5)))) inner)
      (+= inner 1)
      (if (== step 1)
        (continue)
      )
      (if (== step 3)
        (break)
      )
      (print (str "outer " outer " step " step))
    )
    (print (str "left inner at " inner))
    (+= outer 1)
  )
  (print "end")
)
```

```output
outer 0 step 0
outer 0 step 2
left inner at 4
outer 1 step 0
outer 1 step 2
left inner at 4
outer 2 step 0
outer 2 step 2
left inner at 4
end
```

### Pattern loop over an optional its body assigns

```brix
(main
  (let current (? Int) 3)
  (while (= (| (: held Int)) current)
    (print (str "held " held))
    (if (== held 0)
      (= current null)
      (else
        (= current (- held 1))
      )
    )
  )
  (print "end")
)
```

```output
held 3
held 2
held 1
held 0
end
```

### Return from the body of a pattern condition

```brix
(main
  (print (call find 5))
  (print (call find 12))
  (print (call find -1))
)

(fn-do + find String
  (param number Int)
  (if (= (| (: small ({} (and (< it 10) (>= it 0))))) number)
    (return (str "small " small))
  )
  (if (= (| (: large ({} (>= it 10)))) number)
    (return (str "large " large))
  )
  (return "negative")
)
```

```output
small 5
large 12
negative
```

### Raise from the body of a pattern condition

```brix
(main
  (print (?? (catch? (call run 1)) -1))
  (print (?? (catch? (call run 20)) -1))
  (print (?? (catch? (call run -4)) -1))
  (print (?? (catch? (call run 5)) -1))
)

(fn-do + run Int (?! Failed)
  (param number Int)
  (if (= (| (: small ({} (and (< it 10) (>= it 0))))) number)
    (throw (call Failed) (if (== small 1)))
    (return small)
  )
  (if (= (| (: large ({} (>= it 10)))) number)
    (return (* large 2))
  )
  (return 0)
)

(type + Failed (is Error))
```

```output
-1
40
0
5
```

### Pattern loop evaluates its value on each pass

```brix
(main
  (let total 0)
  (let count 0)
  (while (= (| (: value ({} (< it 20)))) (call next count))
    (+= count 1)
    (+= total value)
  )
  (print (str "count " count " total " total))
)

(fn-do + next Int
  (param at-step Int)
  (print (str "next " at-step))
  (return (* at-step at-step))
)
```

```output
next 0
next 1
next 2
next 3
next 4
next 5
count 5 total 30
```

### Pattern condition after a plain condition

```brix
(main
  (let flag true)
  (if (== flag false)
    (print "no")
    (else-if (= (| (call .valid email password)) (call submit "joe" "secret"))
      (print (str email " / " password))
    )
    (else
      (print "invalid")
    )
  )
  (if (= (| (call .valid email password)) (call submit "" "secret"))
    (print (str email " / " password))
    (else-if (= (| .invalid-email) (call submit "" "secret"))
      (print "bad email")
    )
    (else
      (print "invalid")
    )
  )
)

(type + Authorization (is Union)
  (variant invalid-password)
  (variant invalid-email)
  (variant valid (tuple (: email String) (: password String)))
)

(fn-do + submit Authorization
  (param email String)
  (param password String)
  (if (== (. email length) 0)
    (return (. Authorization invalid-email))
  )
  (return (call (. Authorization valid) email password))
)
```

```output
joe / secret
bad email
```

### Pattern conditions nested with a cleanup

```brix
(main
  (let first (? Int) 4)
  (let second (? Int) null)
  (if (= (| (: a Int)) first)
    (if (= (| (: b Int)) second)
      (print (str "both " a " " b))
      (else
        (print (str "only a " a))
      )
    )
    (finally
      (print "leaving a")
    )
    (print "inside a")
  )
  (print "end")
)
```

```output
only a 4
inside a
leaving a
end
```

### Cleanup and continue in a pattern loop

```brix
(main
  (let count 0)
  (while (= (| (: n ({} (< it 4)))) count)
    (finally
      (print (str "after " n))
    )
    (+= count 1)
    (if (== n 2)
      (continue)
    )
    (print (str "body " n))
  )
  (print (str "end " count))
)
```

```output
body 0
after 0
body 1
after 1
after 2
body 3
after 3
end 4
```

### Pattern conditions that take a tuple apart

```brix
(main
  (let point (call Point 3 4))
  (if (= (| (tuple x y)) point)
    (print (str "point " x " " y))
  )
  (if (= (| (tuple (: x ({} (> it 5))) y)) point)
    (print (str "big " x " " y))
    (else
      (print "small")
    )
  )
  (let steps 0)
  (while (= (| (tuple (: x ({} (< it 6))) y)) point)
    (= point (call Point (+ x 1) (* y 2)))
    (+= steps 1)
  )
  (print (str "steps " steps " " (. point x) " " (. point y)))
)

(type + Point (is (tuple (: x Int) (: y Int))))
```

```output
point 3 4
small
steps 3 6 32
```

### Pattern conditions over the types a union holds

```brix
(main
  (let value Foo 12)
  (if (= (| (: number Int)) value)
    (print (str "number " number))
    (else-if (= (| (: text String)) value)
      (print (str "text " text))
    )
  )
  (= value "words")
  (if (= (| (: number Int)) value)
    (print (str "number " number))
    (else-if (= (| (: text String)) value)
      (print (str "text " text))
    )
  )
  (let tries 0)
  (while (= (| (: text String)) value)
    (print (str "still " text))
    (+= tries 1)
    (if (== tries 2)
      (= value 5)
    )
  )
  (print (str "end " tries))
)

(type + Foo (is Union)
  (variant Int)
  (variant String)
)
```

```output
number 12
text words
still words
still words
end 2
```

### Pattern conditions over optionals

```brix
(main
  (let maybe (? Int) null)
  (if (= (| (: held Int)) maybe)
    (print (str "held " held))
    (else
      (print "nothing")
    )
  )
  (= maybe 9)
  (if (= (| (: held Int)) maybe)
    (print (str "held " held))
    (else
      (print "nothing")
    )
  )
  (let text (? String) "word")
  (while (= (| (: held String)) text)
    (print held)
    (= text null)
  )
  (print "end")
)
```

```output
nothing
held 9
word
end
```

### Break from a pattern loop inside a loop of a function

```brix
(main
  (print (call count-down 3))
)

(fn-do + count-down Int
  (param from Int)
  (let left from)
  (let rounds 0)
  (while true
    (while (= (| (: n ({} (> it 0)))) left)
      (-= left 1)
      (+= rounds 1)
      (if (== n 2)
        (break)
      )
    )
    (if (== left 0)
      (return rounds)
    )
    (+= rounds 10)
  )
)
```

```output
13
```

### Pattern conditions over anonymous cases

```brix
(main
  (let result (call attempt 3))
  (if (= (| (call .ok n)) result)
    (print (str "ok " n))
    (else-if (= (| .failed) result)
      (print "failed")
    )
  )
  (let other (call attempt -1))
  (if (= (| (call .ok n)) other)
    (print (str "ok " n))
    (else-if (= (| .failed) other)
      (print "failed")
    )
  )
  (let count 5)
  (while (= (| (call .ok n)) (call attempt count))
    (print (str "again " n))
    (-= count 2)
  )
  (print "end")
)

(fn-expr + attempt _
  (param number Int)
  (if-expr (> number 0) (call ..ok number) ..failed)
)
```

```output
ok 3
failed
again 5
again 3
again 1
end
```

### Capture of a pattern condition is not named after it

```brix crash
(main
  (let value 3)
  (if (= (| (: found ({} (> it 1)))) value)
    (print (str "found " found))
  )
  (print found)
)
```

```output
Exception: Name "found" not found
<no file:7>
      (print found)
             ^^^^^
```

### Pattern condition naming a case of a class that has values

```brix crash
(main
  (let level (. Level low))
  (if (= (| .low) level)
    (print "low")
    (else
      (print "other")
    )
  )
)

(type + Level
  (variant low)
  (variant middle)
  (variant high)
)
```

```output
Exception: Pattern has no possible matches
<no file:4>
      (if (= (| .low) level)
      ^^^^^^^^^^^^^^^^^^^^^
```

### Pattern loop naming a case of a class that has values

```brix crash
(main
  (let level (. Level low))
  (let passes 0)
  (while (= (| .low) level)
    (+= passes 1)
    (if (== passes 3)
      (= level (. Level high))
    )
  )
  (if (= (| .high) level)
    (print (str "high after " passes))
    (else
      (print "still low")
    )
  )
  (if (= (| .low) level)
    (print "low")
    (else-if (= (| .middle) level)
      (print "middle")
    )
    (else
      (print "neither")
    )
  )
)

(type + Level
  (variant low)
  (variant middle)
  (variant high)
)
```

```output
Exception: Pattern has no possible matches
<no file:5>
      (while (= (| .low) level)
      ^^^^^^^^^^^^^^^^^^^^^^^^
```

### Pattern condition naming a case no type has

```brix crash
(main
  (let shape (. Shape dot))
  (if (= (| (call .circle radius)) shape)
    (print (str "circle " radius))
    (else-if (= (| .missing) shape)
      (print "missing")
    )
    (else
      (print "other")
    )
  )
)

(type + Shape (is Union)
  (variant dot)
  (variant circle (tuple (: radius Int)))
)
```

```output
Exception: Case ".missing" not found in Shape
<no file:6>
        (else-if (= (| .missing) shape)
                       ^^^^^^^^
```

### Pattern condition naming a case over an optional of a class

```brix crash
(main
  (let level (? Level) (. Level low))
  (if (= (| .low) level)
    (print "low")
    (else
      (print "other")
    )
  )
)

(type + Level
  (variant low)
  (variant middle)
)
```

```output
Exception: Pattern has no possible matches
<no file:4>
      (if (= (| .low) level)
      ^^^^^^^^^^^^^^^^^^^^^
```

### Arm naming a case of a class that has values gives the match no type

```brix
(main
  (let level (. Level low))
  (let x
    (match level
      (case .low
        1
      )
      (default
        "other"
      )
    )
  )
  (print x)
  (print (type-of x))
)

(type + Level
  (variant low)
  (variant middle)
  (variant high)
)
```

```output
other
[TypeInfo String]
```

### Arm naming a case of a class that has values before an arm of its type

```brix
(main
  (let level (. Level low))
  (let x
    (match level
      (case .low
        1.5
      )
      (case Level
        2
      )
    )
  )
  (print x)
  (print (type-of x))
)

(type + Level
  (variant low)
  (variant middle)
  (variant high)
)
```

```output
2
[TypeInfo I64]
```

### Arms naming cases of a class that has values match nothing

```brix
(main
  (let level (. Level low))
  (match level
    (case .low
      (print "low")
    )
    (case .middle
      (print "middle")
    )
    (default
      (print "other")
    )
  )
)

(type + Level
  (variant low)
  (variant middle)
  (variant high)
)
```

```output
other
```

### Field pattern naming a case of a class that has values keeps its arm

```brix
(main
  (let pair (call Pair (. Level low) 4))
  (let x
    (match pair
      (case (tuple .low n)
        (str n)
      )
      (default
        "other"
      )
    )
  )
  (print x)
  (print (type-of x))
)

(type + Pair (is (tuple (: level Level) (: count Int))))

(type + Level
  (variant low)
  (variant middle)
)
```

```output
other
[TypeInfo String]
```

### Case patterns of a name two unions have

```brix
(main
  (let first (. Color red))
  (let second (. Light red))
  (print (call describe first))
  (print
    (match second
      (case .red
        "light red"
      )
      (default
        "light other"
      )
    )
  )
)

(fn-expr + describe String
  (param color Color)
  (match color
    (case .red
      "red"
    )
    (case .green
      "green"
    )
  )
)

(type + Color (is Union)
  (variant red)
  (variant green)
)

(type + Light (is Union)
  (variant red)
  (variant amber)
)
```

```output
red
light red
```

### Case pattern naming a value of a class a union holds

```brix
(main
  (let value Mixed (. Level low))
  (print
    (match value
      (case .low
        "low"
      )
      (case Int
        "int"
      )
      (default
        "other"
      )
    )
  )
  (let pair (tuple value 3))
  (print
    (match pair
      (case (tuple .low n)
        (str "low " n)
      )
      (default
        "other pair"
      )
    )
  )
)

(type + Mixed (is Union)
  (variant Int)
  (variant Level)
)

(type + Level
  (variant low)
  (variant middle)
)
```

```output
other
other pair
```

### Match whose only arm names a case of a class that has values

```brix crash
(main
  (let level (. Level low))
  (print
    (match level
      (case .low
        "low"
      )
    )
  )
)

(type + Level
  (variant low)
  (variant middle)
  (variant high)
)
```

```output
Exception: Pattern has no possible matches
<no file:6>
          (case .low
          ^^^^^^^^^
```

### Optional match whose only arm names a case of a class that has values

```brix crash
(main
  (let level (. Level low))
  (let answer
    (match? level
      (case .low
        true
      )
    )
  )
  (print answer)
)

(type + Level
  (variant low)
  (variant middle)
  (variant high)
)
```

```output
Exception: Pattern has no possible matches
<no file:6>
          (case .low
          ^^^^^^^^^
```

### Arm naming a case its union lacks and no type has

```brix crash
(main
  (let shape (. Shape dot))
  (print
    (match shape
      (case .missing
        "missing"
      )
      (default
        "other"
      )
    )
  )
)

(type + Shape (is Union)
  (variant dot)
  (variant circle (tuple (: radius Int)))
)
```

```output
Exception: Case ".missing" not found in Shape
<no file:6>
          (case .missing
                ^^^^^^^^
```

### Optional match naming a case no type has

```brix crash
(main
  (let shape (. Shape dot))
  (let answer
    (match? shape
      (case .missing
        "missing"
      )
    )
  )
  (print (?? answer "none"))
)

(type + Shape (is Union)
  (variant dot)
  (variant circle (tuple (: radius Int)))
)
```

```output
Exception: Case ".missing" not found in Shape
<no file:6>
          (case .missing
                ^^^^^^^^
```

### Arm naming a case over a number

```brix crash
(main
  (let number 5)
  (let answer
    (match? number
      (case .low
        "low"
      )
      (case (: text String)
        text
      )
    )
  )
  (print (?? answer "none"))
)
```

```output
Exception: Case ".low" not found: I64 has no cases
<no file:6>
          (case .low
                ^^^^
```

### Field pattern naming a case no type has

```brix crash
(main
  (let pair (call Pair (. Shape dot) 4))
  (print
    (match pair
      (case (tuple .missing n)
        (str "missing " n)
      )
      (case (tuple .dot n)
        (str "dot " n)
      )
      (default
        "other"
      )
    )
  )
)

(type + Pair (is (tuple (: shape Shape) (: count Int))))

(type + Shape (is Union)
  (variant dot)
  (variant circle (tuple (: radius Int)))
)
```

```output
Exception: Case ".missing" not found in Shape
<no file:6>
          (case (tuple .missing n)
                       ^^^^^^^^
```

### Declaration that tests the name it reads

```brix crash
(main
  (call first "foo")
  (call second "foo")
)

(fn-do + first _
  (param text String)
  (let (all ..length ({} (== it 3))) text)
  (print length)
)

(fn-do + second _
  (param text String)
  (print "before")
  (let (all ..length ({} (== it 4))) text)
  (print (str "not reached " length))
)
```

```output
3
before
```

### Declarations that test names of different types

```brix
(main
  (let point (call Point 3.5 4.0))
  (let (: a (all ..length ({} (== it 1)))) "f")
  (let (: b (all ..x ({} (> it 2.5)))) point)
  (let (: c (all ..length ({} (> it 1)))) "four")
  (print a)
  (print b)
  (print c)
  (print (type-of b))
)

(type + Point (is (tuple (: x Float) (: y Float))))
```

```output
1
3.5
4
[TypeInfo F64]
```

### Locals declared by a name read are assigned

```brix
(main
  (let (: x ..length) "foo")
  (= x (+ x 1))
  (print x)
  (let text "hello")
  (let ..length text)
  (+= length 10)
  (print length)
  (print (. text length))
)
```

```output
4
15
5
```

### Local declared by a name read holds a copy

```brix
(main
  (let rect (call Rect))
  (let (: p ..origin) rect)
  (let ..size rect)
  (= (. rect origin) (call Point 9 9))
  (print (. p x))
  (print (. size y))
  (print (. rect origin x))
)

(type + Point (is (tuple (: x Int) (: y Int))))

(type + Rect
  (prop + origin _ (= (call Point 1 2)))
  (prop + size _ (= (call Point 3 4)))
)
```

```output
1
4
9
```

### Declaration reads its name once

```brix
(main
  (let counter (call Counter))
  (let ..next counter)
  (let (: second ..next) counter)
  (print next)
  (print second)
  (print (. counter calls))
)

(type + Counter
  (prop + calls _ (= 0))
  (fn-do + next Int :get
    (+= calls 1)
    (return (* calls 10))
  )
)
```

```output
10
20
2
```

### Declaration in a loop that tests the name it reads

```brix crash
(main
  (print (call measure "ab"))
  (print (call measure "abcdef"))
)

(fn-do + measure Int
  (param text String)
  (let total 0)
  (let round 0)
  (while (< round 3)
    (let (: n (all ..length ({} (< it 5)))) text)
    (+= total n)
    (+= round 1)
  )
  (return total)
)
```

```output
6
```

### Declaration with two tests of the name it reads

```brix
(main
  (let attempt
    (do-new
      (let point (call Point 3.5 4.0))
      (let (: a (all ..x ({} (> it 2.5)) ({} (< it 4.0)))) point)
      (print a)
      (let (: b (all ..y ({} (> it 2.5)) ({} (< it 4.0)))) point)
      (print b)
      (on-failure failure
        (print failure)
        (print "after")
      )
    )
  )
  (call (. attempt wait))
)

(type + Point (is (tuple (: x Float) (: y Float))))
```

```output
3.5
Pattern match failed
after
```

### Declaration by a name read in a function

```brix
(main
  (call show "foo")
)

(fn-do + show _
  (param text String)
  (let ..length text)
  (print length)
)
```

```output
3
```

### Arm that captures a name read

```brix
(main
  (print
    (match "foo"
      (case (: x ..length)
        x
      )
    )
  )
)
```

```output
3
```

### Arm that reads a name

```brix
(main
  (print
    (match "hey"
      (case ..length
        (+ length 1)
      )
    )
  )
)
```

```output
4
```

### Optional match whose arm reads a name

```brix
(main
  (let n
    (match? "hello"
      (case ..length
        length
      )
    )
  )
  (print n)
)
```

```output
5
```

### Name read under a captured name is not declared

```brix crash
(main
  (let (: x ..length) "foo")
  (print x)
  (print length)
)
```

```output
Exception: Name "length" not found
<no file:5>
      (print length)
             ^^^^^^
```

### Declaration reading a name its value lacks

```brix crash
(main
  (let ..missing "foo")
  (print missing)
)
```

```output
Exception: Pattern has no possible matches
<no file:3>
      (let ..missing "foo")
           ^^^^^^^^^
```

### Arm among several that tests a name read

```brix crash
(main
  (print
    (match "hello"
      (case (all ..length ({} (== it 4)))
        length
      )
      (default
        99
      )
    )
  )
)
```

```output
Exception: Type not found
<no file:5>
          (case (all ..length ({} (== it 4)))
                     ^^^^^^^^
```

### Arm among several that captures a tested name read

```brix crash
(main
  (print
    (match "hello"
      (case (: n (all ..length ({} (== it 5))))
        n
      )
      (default
        99
      )
    )
  )
)
```

```output
Exception: Type not found
<no file:5>
          (case (: n (all ..length ({} (== it 5))))
                          ^^^^^^^^
```

### Arm among several that reads a name

```brix crash
(main
  (print
    (match "hello"
      (case Int
        1
      )
      (case ..length
        length
      )
    )
  )
)
```

```output
Exception: Type not found
<no file:8>
          (case ..length
                ^^^^^^^^
```

### Arm among several that captures a name read

```brix crash
(main
  (print
    (match "hello"
      (case (: x ..length)
        x
      )
      (case String
        0
      )
    )
  )
)
```

```output
Exception: Type not found
<no file:5>
          (case (: x ..length)
                     ^^^^^^^^
```

### Arm reading a name under a captured name does not declare it

```brix crash
(main
  (print
    (match "foo"
      (case (: x ..length)
        length
      )
    )
  )
)
```

```output
Exception: Name "length" not found
<no file:6>
            length
            ^^^^^^
```

### Match that ends a function whose arms each return

```brix
(main
  (print (call Thing 10.5))
  (print (call Thing -2.0))
  (print (call describe 3))
  (print (call describe -3))
)

(type + Thing (is Struct)
  (prop + foo I32)
  (prop + bar Float)
  (fn-do + This _
    (param n Float)
    (if (< n 0.0)
      (return (call This 0 0.0))
    )
    (return (call This 1 n))
  )
)

(fn-do + describe String
  (param n Int)
  (match n
    (case ({} (< it 0))
      (return "negative")
    )
    (default
      (return "other")
    )
  )
)
```

```output
(1, 10.5)
(0, 0)
other
negative
```

### Match whose arms return with no arm for what is left

```brix crash
(main
  (print (call sign -4))
  (print (call sign 0))
  (print (call sign 9))
)

(fn-do + sign String
  (param n Int)
  (match n
    (case ({} (< it 0))
      (return "negative")
    )
    (case 0
      (return "zero")
    )
  )
  (return "positive")
)
```

```output
negative
zero
```

### Matches whose arms return, raise or go on

```brix
(main
  (print (?? (catch? (call grade 95)) "failed"))
  (print (?? (catch? (call grade 40)) "failed"))
  (print (call name 1))
  (print (call name 2))
  (print (call name 3))
)

(type + Failed (is Error))

(fn-do + grade String (?! Failed)
  (param score Int)
  (match score
    (case ({} (>= it 90))
      (return "high")
    )
    (default
      (throw (call Failed))
    )
  )
)

(fn-do + name String
  (param n Int)
  (match n
    (case 1
      (return "one")
    )
    (case 2
      (return "two")
    )
    (default
      (print "many is printed")
    )
  )
  (return "many")
)
```

```output
high
failed
one
two
many is printed
many
```

### Alternatives written in one arm are tried in turn and share the arm's body

```brix
(main
  (print (call pick 3))
  (print (call pick 7))
  (print (call pick 20))
  (print (call name "a"))
  (print (call name "b"))
  (print (call name "z"))
)

(fn-expr + pick _
  (param v Int)
  (match v
    (case (| ({} (< it 5)) ({} (> it 10)))
      "out"
    )
    (default
      "in"
    )
  )
)

(fn-expr + name _
  (param v String)
  (match v
    (case (| "a" "b")
      1
    )
    (default
      0
    )
  )
)
```

```output
out
in
out
1
1
0
```

### Alternatives of one arm that name cases of a union

```brix
(main
  (print (call kind (call (. Shape circle) 2)))
  (print (call kind (call (. Shape square) 3)))
  (print (call kind (. Shape dot)))
  (print (call size (call (. Shape circle) 2)))
  (print (call size (call (. Shape square) 3)))
  (print (call size (. Shape dot)))
)

(type + Shape (is Union)
  (variant circle (tuple (: r Int)))
  (variant square (tuple (: s Int)))
  (variant dot)
)

(fn-expr + kind _
  (param s Shape)
  (match s
    (case (| .circle .dot)
      "round"
    )
    (case .square
      "boxy"
    )
  )
)

(fn-expr + size _
  (param s Shape)
  (match s
    (case (| .dot .circle)
      0
    )
    (case (call .square n)
      (* n n)
    )
  )
)
```

```output
round
boxy
round
0
9
0
```

### Three alternatives in one arm, and alternatives that name types of a union

```brix
(main
  (print (call sort 3))
  (print (call sort -4))
  (print (call sort 0))
  (print (call sort 50))
  (print (call word "x"))
  (print (call word 7))
  (print (call word true))
)

(fn-expr + sort _
  (param v Int)
  (match v
    (case (| 0 ({} (> it 40)) ({} (< it -3)))
      "edge"
    )
    (case ({} (> it 0))
      "plus"
    )
    (default
      "minus"
    )
  )
)

(type + Any (is Union)
  (variant String)
  (variant Int)
  (variant Bool)
)

(fn-expr + word _
  (param v Any)
  (match v
    (case (| Bool Int)
      "scalar"
    )
    (case String
      "text"
    )
  )
)
```

```output
plus
edge
edge
edge
text
scalar
scalar
```

### An arm that raises a panic where its condition holds, written among other arms

```brix
(main
  (let attempt
    (do-new
      (print (call grade 50))
      (print (call grade 10))
      (print (call grade 101))
      (print (call grade 60))
      (on-failure failure
        (print (str "caught " failure))
      )
    )
  )
  (call (. attempt wait))
)

(fn-expr + grade _
  (param v Int)
  (match v
    (case 1000
      "top"
    )
    (throw (call Panic "too big") (if (> it 100)))
    (case ({} (> it 40))
      "pass"
    )
    (default
      "fail"
    )
  )
)
```

```output
pass
fail
caught too big
```

### Types named one after another in a pattern each convert what the one before made

```brix
(main
  (print
    (match 10foo
      (case (all (similar Bar) ({} (> it 20)) (similar Foo) ({} (< it 30))))
    )
  )
  (print
    (match 30foo
      (case (all (similar Bar) ({} (> it 20)) (similar Foo) ({} (< it 30))))
      (default
        0foo
      )
    )
  )
  (print
    (match 5
      (case (all Int ({} (> it 3)) (similar String) ({} (== (. it length) 1))))
    )
  )
  (print
    (match 7
      (case (all Int ({} (> it 3)) ({} (< it 10))))
    )
  )
  (print (type-of (call chained 12)))
  (print (call chained 12))
  (print (call chained 2))
  (print (call shown 9bar))
)

(blank)
(type-line + Foo (alias foo) (is F64))
(type-line + Bar (alias bar) (is I32))
(blank)
(unit + 1bar 0.333333333333foo)

(fn-expr + chained _
  (param n I8)
  (match n
    (case (all (similar Int) ({} (> it 3)) (similar String) ({} (== (. it length) 2))))
    (default
      "none"
    )
  )
)

(fn-expr + shown _
  (param b Bar)
  (match b
    (case (all (similar Foo) ({} (> it 2)) (similar Bar))
      (str "wide " it)
    )
    (default
      "narrow"
    )
  )
)
```

```output
9.99999999999
29.99999999997
5
7
[TypeInfo String]
12
none
wide 9
```

### An arm written with a test narrows what the arms after it read, with a subject and with none.

```brix
(main
  (print (call of-subject 0))
  (print (call of-subject 5))
  (print (call of-subject null))
  (print (call of-true 0))
  (print (call of-true 5))
  (print (call of-true null))
  (print (call of-number 0))
  (print (call of-number 5))
  (print (call of-number null))
  (print (call of-rest 0))
  (print (call of-rest 5))
  (print (call of-rest null))
)

(fn-expr + of-subject String
  (param value (? Int))
  (match value
    (case? (not it)
      "absent"
    )
    (case? it
      "truthy"
    )
    (default
      "falsy"
    )
  )
)

(fn-expr + of-true String
  (param value (? Int))
  (match true
    (case? (not value)
      "absent"
    )
    (case? value
      "truthy"
    )
    (default
      "falsy"
    )
  )
)

(fn-expr + of-number String
  (param value (? Int))
  (match 1
    (case? (== value null)
      "absent"
    )
    (case? value
      "truthy"
    )
    (default
      "falsy"
    )
  )
)

(fn-expr + of-rest String
  (param value (? Int))
  (match 1
    (case? (not value)
      "absent"
    )
    (default
      (if-expr value "truthy" "falsy")
    )
  )
)
```

```output
falsy
truthy
absent
falsy
truthy
absent
falsy
truthy
absent
falsy
truthy
absent
```

### A match whose arms stand in a block with the lazy locals they read.

```brix
(main
  (print (call (. Spot first place) 10))
  (print (call (. Spot middle place) 10))
  (print (call (. Spot last place) 10))
  (let spot (call (. Spot index) 7))
  (print (call (. spot place) 10))
)

(type + Spot (is Union)
  (variant first)
  (variant middle)
  (variant last)
  (variant index Int)
  (fn-expr + place Int
    (param count Int)
    (match this
      ;
      (case (. Spot first)
        start
      )
      (case (. Spot middle)
        (/ length 2)
      )
      (case (. Spot last)
        (- length 1)
      )
      (case (call (. Spot index) n)
        n
      )
      ;
      (lazy start 0)
      (lazy length count)
    )
  )
)
```

```output
0
5
9
7
```

### A lazy local under the arms of a match is evaluated once, by the arm that reads it: in match, match? and a match? run as steps.

```brix
(main
  (print (call (. Spot first place) 10))
  (print (call (. Spot middle place) 10))
  (print (call (. Spot last place) 10))
  (let spot (call (. Spot index) 7))
  (print (call (. spot place) 10))
  (print (call (. spot named) 3))
  (print (call (. Spot last named) 3))
  (call (. spot shown) 4)
  (call (. Spot first shown) 4)
)

(fn-do _ noisy Int
  (param n Int)
  (print (str "noisy " n))
  (return n)
)

(type + Spot (is Union)
  (variant first)
  (variant middle)
  (variant last)
  (variant index Int)
  (fn-expr + place Int
    (param count Int)
    (match this
      ;
      (case (. Spot first)
        start
      )
      (case (. Spot middle)
        (/ length 2)
      )
      (case (. Spot last)
        (+ (- length 1) length)
      )
      (case (call (. Spot index) n)
        n
      )
      ;
      (lazy start (call noisy 0))
      (lazy length (call noisy count))
    )
  )
  (fn-expr + named (? String)
    (param count Int)
    (match? this
      ;
      (case (call (. Spot index) n)
        (str "index " n " of " length)
      )
      ;
      (lazy length (call noisy count))
    )
  )
  (fn-do + shown _
    (param count Int)
    (match? this
      ;
      (case (call (. Spot index) n)
        (print (str "index " n " of " length " and " length))
      )
      (case (. Spot first)
        (print "first")
      )
      ;
      (lazy length (call noisy count))
    )
  )
)
```

```output
noisy 0
0
noisy 10
5
noisy 10
19
7
noisy 3
index 7 of 3
unknown
noisy 4
index 7 of 4 and 4
first
```

### A comprehension written with the arms of a match gives what the arm an item fits gives, and passes over an item no arm fits

```brix
(main
  (let doubled
    (iterate (in (list 1 2 3))
      (block
        (case (: n Int)
          (* n 2)
        )
        (default
          0
        )
      )
    )
  )
  (print (type-of doubled))
  (print (call (. ", " join) doubled))
  (let picked
    (iterate (in (list 1 2 3 4))
      (block
        (case 2
          20
        )
        (case 4
          40
        )
      )
    )
  )
  (print (call (. ", " join) picked))
  (let lengths
    (iterate (in (list "a" "bb" "" "dddd"))
      (block
        (case ""
          99
        )
        (case (: text String)
          (. text length)
        )
      )
    )
  )
  (print (call (. ", " join) lengths))
  (let shapes (call (of List Shape)))
  (+= shapes (call Circle 2))
  (+= shapes (call Square 3))
  (+= shapes (call Dot))
  (+= shapes (call Circle 1))
  (let sizes
    (iterate (in shapes)
      (block
        (case (: c Circle)
          (* (. c radius) 3)
        )
        (case (: s Square)
          (* (. s side) 4)
        )
      )
    )
  )
  (print (call (. ", " join) sizes))
  (let named
    (iterate (in (list (tuple 1 "x") (tuple 2 "y") (tuple 1 "z")))
      (block
        (case (tuple 1 name)
          (str "one " name)
        )
        (case (tuple count name)
          (str count " of " name)
        )
      )
    )
  )
  (for (in text named)
    (print text)
  )
  (print (. (call List (call scaled (list 1 2 3) 10)) length))
  (for (in n (call scaled (list 1 2 3) 10))
    (print n)
  )
)

(type + Shape
  (prop + sides _ (= 0))
)

(type + Circle (is Shape)
  (prop + radius Int)
  (fn-do + This _
    (param radius _)
  )
)

(type + Square (is Shape)
  (prop + side Int)
  (fn-do + This _
    (param side _)
  )
)

(type + Dot (is Shape))

(fn-expr + scaled _
  (param items (list Int))
  (param by Int)
  (iterate (in items)
    (block
      (case 2
        by
      )
      (case (: n Int)
        (+ (* n by) 1)
      )
    )
  )
)
```

```output
[TypeInfo Generator'I64]
2, 4, 6
20, 40
1, 2, 99, 4
6, 12, 3
one x
2 of y
one z
3
11
10
31
```

### An arm of a comprehension that gives an optional that holds nothing passes over its item

```brix
(main
  (let halves
    (iterate (in (list 1 2 3 4))
      (block
        (case (: n Int)
          (call half n)
        )
      )
    )
  )
  (print (type-of halves))
  (print (call (. ", " join) halves))
  (let mixed
    (iterate (in (list 1 2 3))
      (block
        (case 1
          "one"
        )
        (case (: n Int)
          (str n)
        )
      )
    )
  )
  (print (type-of mixed))
  (for (in item mixed)
    (print item)
  )
  (let readers
    (call List
      (block
        (iterate (in (list 5 6 7))
          (block
            (case 6
              (call reader 60)
            )
            (case (: n Int)
              (call reader (+ n 1))
            )
          )
        )
      )
    )
  )
  (for (in read readers)
    (print (call read))
  )
)

(fn-expr + half (? Int)
  (param n Int)
  (case? (== (% n 2) 0)
    (/ n 2)
  )
  (default
    null
  )
)

(fn-expr + reader (of Getter Int)
  (param n Int)
  (=> () n)
)
```

```output
[TypeInfo Generator'I64]
1, 2
[TypeInfo Generator'String]
one
2
3
6
60
8
```

### List pattern over a base class reads by the derived class

```brix
(main
  (let base Base (call Derived))
  (print
    (match base
      (case (list a b)
        (str "two " a " " b)
      )
      (default
        "none"
      )
    )
  )
  (print (is base (list 5 _)))
)

(type + Base
  (implements (of Indexable Int))
  (fn-expr + length Size :get
    2
  )
  (fn-expr + (index-get this (param index Index)) Int
    (+ index 1)
  )
  (fn-expr + (index-get this (param range (of Range Index))) Base
    this
  )
)

(type + Derived (is Base)
  (fn-expr + (index-get this (param index Index)) Int
    (+ (* index 100) 5)
  )
)
```

```output
two 5 105
true
```

### A generic class written bare is tested against numbers, texts, lists and maps

```brix
(main
  (print (is 42 List))
  (print (is (list 1 2) List))
  (print (is "text" List))
  (print (is-not (list 1 2) List))
  (print (is-not 42 List))
  (let items (list 1 2 3))
  (if (is items List)
    (print "a list")
  )
  (let map (call (of Map String Int)))
  (print (is map Map))
  (print (is map List))
  (print (is items Map))
)
```

```output
false
true
false
false
true
a list
true
false
false
```

### A generic class written bare is tested against optionals and tuples

```brix
(main
  (let maybe (? (list Int)) (list 1 2))
  (print (is maybe List))
  (let none (? (list Int)) null)
  (print (is none List))
  (let box (call Box 5))
  (print (is box Box))
  (print (is box List))
  (print (is 5 Box))
  (let pair (call Pair 1 "a"))
  (print (is pair Pair))
  (print (is pair Box))
)

(type + (of Box Item) (is Tuple)
  (prop + item Item)
)

(type + (of Pair Left Right) (is Tuple)
  (prop + left Left)
  (prop + right Right)
)
```

```output
true
false
true
false
false
true
false
```

### A generic class written bare in a condition and in a match of a generic function

```brix
(main
  (print (call check 42))
  (print (call check (list 1 2)))
  (print (call check "text"))
  (print (call pick 1))
  (print (call pick (list 1)))
)

(fn-expr + check String
  (param value _)
  (case? (is value List)
    "list"
  )
  (default
    "other"
  )
)

(fn-expr + (of pick T) Int
  (param value T)
  (match value
    (case List
      1
    )
    (default
      2
    )
  )
)
```

```output
other
list
other
2
1
```

### A list written bare as an arm of a match over values and unions

```brix
(main
  (call show 42)
  (call show (list 1 2))
  (call show "text")
  (call show (call Foo 7))
  (call show (call Foo (list (call Foo 1))))
)

(type + Foo (is Union)
  (variant Int)
  (variant (list Foo))
)

(fn-do + show _
  (param value _)
  (match value
    (case List
      (print "list")
    )
    (case Int
      (print "int")
    )
    (default
      (print "other")
    )
  )
)
```

```output
int
list
other
int
list
```

### The arm of a match that takes whatever is left may itself hold a match.

```brix
(main
  (print (call pick 1 1))
  (print (call pick 2 1))
  (print (call pick 2 2))
)

(fn-expr + pick String
  (param a Int)
  (param b Int)
  (match a
    (case= 1
      "first"
    )
    (default
      (match b
        (case= 1
          "second"
        )
        (default
          "neither"
        )
      )
    )
  )
)
```

```output
first
second
neither
```

### A case named over an optional union binds what the case holds.

```brix
(main
  (print (call show (call .optional "q")))
  (print (call show (call .buffer "b")))
  (print (call show null))
)

(type + Suffix (is Union)
  (variant optional (tuple (: name String)))
  (variant buffer (tuple (: name String)))
)

(fn-expr + show String
  (param suffix (? Suffix))
  (match suffix
    (case (call .optional name)
      (str "optional " name)
    )
    (case (call .buffer name)
      (str "buffer " name)
    )
    (default
      "none"
    )
  )
)
```

```output
optional q
buffer b
none
```

### A match whose every arm returns or raises ends a `do` function that gives a value.

```brix
(main
  (print (call size-of 3))
  (print (?? (catch? (call size-of -3)) 99))
)

(fn-do + size-of Int (?! Error)
  (param value Int)
  (match value
    (case (all (: number Int) ({} (>= it 0)))
      (return number)
    )
    (throw (call Error "negative"))
  )
)
```

```output
3
99
```

### A comprehension over a pattern naming a case of a union keeps the items holding that case, as the union.

```brix
(main
  (let items (list Pick) (list .first (call .third 4) .second (call .third 5)))
  (for (in pick (in (<* .third) items))
    (print (is pick .third))
    (print
      (match pick
        (case (call .third n)
          n
        )
        (default
          0
        )
      )
    )
  )
)

(type + Pick (is Union)
  (variant first)
  (variant second)
  (variant third (tuple Int))
)
```

```output
true
4
true
5
```

### A match statement's arm may hold a value chosen by conditions, written as a statement.

```brix
(main
  (print (call fill 2))
  (print (call fill 5))
)

(fn-do + fill String
  (param size Int)
  (let out ([] Int 3))
  (match size
    (case= 2
      (for (in index (..< 0 3))
        (= ([] out index)
          (block
            (case? (== index 1)
              10
            )
            (default
              20
            )
          )
        )
      )
    )
    (default
      (= ([] out 0) 1)
    )
  )
  (return (str ([] out 0) " " ([] out 1) " " ([] out 2)))
)
```

```output
20 10 20
1 0 0
```

### A pattern that names a generic class without its arguments, with a guard, gives the guard's `it` the class the case it matches holds.

```brix
(main
  (let pair Item (as (list "a" "b") (list Item)))
  (let single Item "c")
  (print (call describe pair))
  (print (call describe single))
)

(type + Item (is Union)
  (variant String)
  (variant (list Item))
)

(fn-expr _ describe String
  (param value Item)
  (match value
    (case (all (: items (similar List)) ({} (== (. it length) 2)))
      (str "pair " (. items length))
    )
    (default
      "other"
    )
  )
)
```

```output
pair 2
other
```

### A generic class written bare matches the case of a union that is of the class, not one that only converts to it.

```brix
(main
  (let items (list Value))
  (+= items (as 1 Value))
  (print (call after-data (as items Value)))
  (print (call after-data (as ([] Byte 2) Value)))
  (print (call alone (as items Value)))
  (print (call alone (as ([] Byte 2) Value)))
)

(type + Value (is Union)
  (variant Int)
  (variant ([] Byte))
  (variant (list Value))
)

(fn-do _ after-data String
  (param value Value)
  (match value
    (case (: number Int)
      (return (str number))
    )
    (case (: data ([] Byte))
      (return (str "data " (. data size)))
    )
    (case (: list (similar List))
      (return (str "list " (. list length)))
    )
  )
)

(fn-do _ alone String
  (param value Value)
  (match value
    (case (: number Int)
      (return (str number))
    )
    (case (: list List)
      (return (str "list " (. list length)))
    )
    (default
      (return "other")
    )
  )
)
```

```output
list 1
data 2
list 1
other
```

### Arms after a null arm read the subject, named or as it, as what it holds, and so does what follows a match statement whose null arm leaves.

```brix
(main
  (print (call by-it null))
  (print (call by-it (call Holder)))
  (print (call by-name null))
  (print (call by-name (call Holder)))
  (print (call after-tests (call Holder)))
  (print (call after-statement null))
  (print (call after-statement (call Holder)))
)

(type + Holder
  (prop + count _ (= 3))
)

(fn-expr + by-it Int
  (param held (? Holder))
  (match held
    (case= null
      0
    )
    (case? (> (. it count) 5)
      1
    )
    (default
      (. it count)
    )
  )
)

(fn-expr + by-name Int
  (param held (? Holder))
  (match held
    (case= null
      0
    )
    (default
      (. held count)
    )
  )
)

(fn-expr + after-tests Int
  (param held (? Holder))
  (match held
    (case? (== it null)
      0
    )
    (case= null
      1
    )
    (case? (> (. held count) 1)
      (+ (. held count) 10)
    )
    (default
      (. held count)
    )
  )
)

(fn-do + after-statement Int
  (param held (? Holder))
  (match held
    (case= null
      (return 0)
    )
    (default
      (print (. held count))
    )
  )
  (return (+ (. held count) 100))
)
```

```output
0
3
0
3
13
0
3
103
```

### The body of a condition arm of a match on a subject reads the subject as it.

```brix
(main
  (print (call larger 7))
  (print (?? (call larger 2) 0))
  (print (?? (?. (call first-held (call Holder 4)) count) 0))
)

(type + Holder
  (prop + count Int)
  (fn-do + This _
    (param count _)
  )
)

(fn-expr + larger (? Int)
  (param value Int)
  (match? value
    (case? (> it 5)
      (+ it 100)
    )
  )
)

(fn-expr + first-held (? Holder)
  (param held Holder)
  (match? held
    (case? (> (. it count) 1)
      it
    )
  )
)
```

```output
107
0
4
```

### An optional match whose one arm no subject can match gives null where an optional is expected

```brix
(main
  (print (?? (call (?. (call found (call Shape)) hosted)) "none"))
  (print (?? (call (?. (call found null) hosted)) "none"))
)

(type + Host (is Interface)
  (fn-do + hosted String)
)

(type + Shape
  (prop + name _ (= "shape"))
)

(fn-do + found (? Host)
  (param shape (? Shape))
  (return
    (match? shape
      (case (: host Host)
        host
      )
    )
  )
)
```

```output
none
none
```

### The value of an arm is that of its last row, and the rows before it run as steps, whatever they give.

```brix
(type + Shade (is Union)
  (variant red)
  (variant blue)
)

(fn-do + counted I64
  (print "counted")
  (return 3)
)

(fn-do + pick I64
  (param s Shade)
  (let total 0)
  (let x (match s
    (case .red (+= total 1) (call counted) (print "red") (+ 5 total))
    (case .blue 2)
  ))
  (return x)
)

(main
  (print (str (call pick .red)))
  (print (str (call pick .blue)))
)
```

```output
counted
red
6
2
```
