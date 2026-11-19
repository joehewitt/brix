# Expressions

```brix
(main
  (print 1)
)
```

```output
1
```

```brix
(main
  (let x
    (block
      1
    )
  )
  (print 1)
)
```

```output
1
```

### where in call expression

```brix crash
(main
  (let a 5)
  (let a 6)
  (print a)
)
```

```output
Exception: "a" is already declared on line 3 in this scope; to change its value, assign it with "(= a value)", or choose another name
<no file:4>
      (let a 6)
           ^
```

```brix
(main
  (print
    (call foo a
      (block
        (lazy a 5)
      )
    )
  )
)

(fn-expr + foo _
  (param a _)
  (* a 2)
)
```

```output
10
```

```brix
(main
  (print
    (call foo
      (block
        (lazy a 5)
        (* a 2)
      )
    )
  )
)

(fn-expr + foo _
  (param a _)
  (* a 2)
)
```

```output
20
```

```brix
(main
  (print
    (call foo
      (block
        (lazy a 5)
        (* a 2)
      )
    )
  )
)

(fn-expr + foo _
  (param a _)
  (* a 2)
)
```

```output
20
```

```brix
(main
  (print
    (call foo
      (block
        (* a 2)
        (lazy a 5)
      )
    )
  )
)

(fn-expr + foo _
  (param a _)
  (* a 2)
)
```

```output
20
```

```brix
(main
  (print
    (scope (+ a b)
      (lazy a (call foo 5))
      (lazy b (call foo 2))
    )
  )
)

(fn-do + foo _
  (param a _)
  (if false
    (throw (call Panic "bad"))
    (else
      (return (* a 2))
    )
  )
)
```

```output
14
```

### where in property

```brix
(main
  (let thing (call Thing))
  (print (. thing a))
)

(type + Thing
  (prop + a _
    (=
      (scope b
        (lazy b 10)
      )
    )
  )
)
```

```output
10
```

### where in function

```brix
(main
  (print (call foo 5))
)

(fn-expr + foo _
  (param a _)
  (* a b)
  (where
    (lazy b 5)
  )
)
```

```output
25
```

### where variable referring to argument

```brix
(main
  (print (call foo 5))
)

(fn-expr + foo _
  (param a _)
  (* a b)
  (where
    (lazy b a)
  )
)
```

```output
25
```

### where variables referring to each other

```brix
(main
  (print (call foo))
)

(fn-expr + foo _
  b
  (where
    (lazy c 10)
    (lazy b c)
  )
)
```

```output
10
```

### where variables referring to each other out of order

```brix
(main
  (print (call foo))
)

(fn-expr + foo _
  b
  (where
    (lazy b c)
    (lazy c 10)
  )
)
```

```output
10
```

### lazy variable not used is not called

```brix
(main
  (print (call foo))
)

(fn-do _ get _
  (param n _)
  (print (str "get " n))
  (return n)
)

(fn-expr + foo _
  a
  (where
    (lazy a (call get 1))
    (lazy b (call get 2))
  )
)
```

```output
get 1
1
```

```brix crash
(main
  (call x
    (block
      (print)
    )
  )
)
```

```output
Exception: Illegal statement
<no file:4>
        (block
        ^^^^^
```

```brix
(main
  (let a 2)
  (print
    (block
      (case? true
        1
      )
      (default
        (inline a)
      )
    )
  )
)
```

```output
1
```

```brix
(main
  (do-new
    (print (call foo 5))
    (on-failure failure
      (print failure)
    )
  )
)

(fn-expr + foo _
  (param a _)
  (scope a
    (throw (call Panic "Too big") (if (> b 5)))
  )
  (where
    (lazy b (* a 2))
  )
)
```

```output
Too big
```

```brix crash
(main
  (print (call bar))
)

(fn-do + foo _)

(fn-expr + bar _
  (call foo)
)
```

```output
Exception: Non-do function must return a non-void value
<no file:8>
    (fn-expr + bar _
    ^^^^^^^^^^^^^^^
```

```brix crash
(main
  (print (call bar))
)

(fn-do + foo _
  (print "hi")
)

(fn-expr + bar _
  (scope a
    (lazy a (call foo))
  )
)
```

```output
Exception: Non-do function must return a non-void value
<no file:10>
    (fn-expr + bar _
    ^^^^^^^^^^^^^^^
```

```brix
(main
  (let a (call foo))
  (print a)
)

(fn-do + foo _
  (print "hi")
)
```

```output
hi
core:Void
```

```brix
(main
  (print (call foo))
)

(fn-do + foo _
  (print "hi")
)
```

```output
hi
core:Void
```

```brix
(main
  (let thing (call Thing))
  (let x (= (. thing a) 1))
  (print x)
)

(type + Thing
  (prop + a _ (= 0))
)
```

```output
1
```

```brix
(main
  (let thing (call Thing))
  (let x
    (block
      (+= (. thing a) 1)
    )
  )
  (print x)
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

```brix
(main
  (let thing (call Thing))
  (let x (+= thing 1))
  (print x)
)

(type + Thing
  (prop + a _ (= 10))
  (fn-do + (infix this += (param n _)) Void
    (+= a n)
  )
)
```

```output
core:Void
```

```brix
(main
  (let thing (call Thing))
  (let x
    (block
      (= ([] thing 2) 5)
    )
  )
  (print x)
)

(type + Thing
  (prop + a _ (= 10))
  (fn-do + (index-set this (param index _) (param value _)) _
    (let a (+ index value))
    (return a)
  )
)
```

```output
7
```

```brix crash
(main
  (print (call foo))
)

(fn-expr + foo _
  (scope a
    (lazy a b)
    (lazy b a)
  )
)
```

```output
Exception: "a" depends on its own value, directly or through the values it reads; break the cycle
<no file:8>
        (lazy a b)
              ^
```

### Lazy variable with default value

```brix
(main
  (print (call foo))
)

(fn-expr + foo _
  (scope a
    (lazy a Int)
  )
)
```

```output
0
```

### Lazy tuple deconstruction

```brix
(main
  (print (call foo))
)

(fn-expr + foo _
  (scope a
    (lazy (tuple a b) (tuple 1 2))
  )
)
```

```output
1
```

### Lazy variables first evaluated in different branches should be available to all

```brix
(main
  (print (call foo 11))
  (print (call foo 5))
)

(fn-expr + foo _
  (param n _)
  (scope (if-expr (> n 10) (str b " 1") (str b " 2"))
    (lazy b "foobar")
  )
)
```

```output
foobar 1
foobar 2
```

### Nested lazy variable references with branching between them

```brix
(main
  (print (call foo))
)

(fn-expr + foo _
  (scope b
    (lazy a (call evil 2))
    (lazy b (+ a (call evil 1)))
  )
)

(fn-do + evil _
  (param a _)
  (if (> a 10)
    (throw (call Panic "DIE"))
    (else
      (return 1)
    )
  )
)
```

```output
2
```

```brix
(main
  (print (call foo 5))
)

(fn-expr + foo _
  (param n _)
  b
  (where
    (lazy a (if-expr (< n 5) 100 200))
    (lazy b (if-expr (== a 200) c (+ c 1)))
    (lazy c 10)
  )
)
```

```output
10
```

```brix
(main
  (print
    (block
      b
      (lazy b (if-expr true 1 2))
    )
  )
)
```

```output
1
```

```brix
(main
  (print (call foo 5))
)

(fn-expr + foo _
  (param n _)
  b
  (where
    (lazy a (if-expr (< n 5) 100 200))
    (lazy b (if-expr (== a 200) c (+ c 1)))
    (lazy c 10)
  )
)
```

```output
10
```

### Nested "where"

```brix
(main
  (print expr)
)

(blank)
(prop + expr _
  (=
    (block
      (call Thing d1 d3
        (block
          (lazy d1 (call blah))
        )
      )
      (lazy d3 (call blah))
    )
  )
)

(type + Thing (is (tuple (: a Int) (: b Int)))
  (fn-expr + with _
    (param n _)
    this
  )
)

(fn-expr + blah _
  4
)
```

```output
(4, 4)
```

## Wildcards

```brix
(main
  (print (> 10 5 0))
  (print (> (< 10 11) 0))
  (print (> 10 11 0))
)
```

```output
true
true
false
```

```brix
(main
  (call foo (call noisy 1) (call noisy 2))
)

(fn-do + foo _
  (param a _)
  (param b _)
  (print (str "got " a " and " b))
)

(fn-do + noisy Int
  (param n Int)
  (print n)
  (return n)
)
```

```output
1
2
got 1 and 2
```

```brix
(main
  (do-new
    (call foo 1)
    (call foo 10)
    (on-failure failure
      (print failure)
    )
  )
)

(fn-do + foo _
  (param n _)
  (print
    (scope (* n 2)
      (throw (call Panic "-1") (if (== n 10)))
    )
  )
)
```

```output
2
-1
```

```brix
(main
  (catch? (call foo 1))
  (catch? (call foo 10))
)

(fn-do + foo _
  (param n _)
  (print
    (scope (* n 2)
      (throw (call Error) (if (== n 10)))
    )
  )
)
```

```output
2
```

## Testing order of evaluating lazy variables

```brix
(main
  (print "hi")
  (blank)
  (lazy a (call noisy 5))
)

(fn-do + noisy Int
  (param n Int)
  (print n)
  (return n)
)
```

```output
hi
```

```brix
(main
  (print "hi")
  (print (+ a 1))
  (blank)
  (lazy a (call noisy 5))
)

(fn-do + noisy Int
  (param n Int)
  (print n)
  (return n)
)
```

```output
hi
5
6
```

```brix
(main
  (print "hi")
  (print (+ a 1))
  (print (+ a 2))
  (blank)
  (lazy a (call noisy 5))
)

(fn-do + noisy Int
  (param n Int)
  (print n)
  (return n)
)
```

```output
hi
5
6
7
```

```brix
(main
  (print "hi")
  (print (if-expr false (+ a 1) 0))
  (print (if-expr true (+ a 2) 0))
  (blank)
  (lazy a (call noisy 5))
)

(fn-do + noisy Int
  (param n Int)
  (print n)
  (return n)
)
```

```output
hi
0
5
7
```

```brix
(main
  (= foo 10)
  (print foo)
  (blank)
  (lazy foo (call noisy 42))
)

(fn-do + noisy Int
  (param n Int)
  (print n)
  (return n)
)
```

```output
10
```

```brix
(main
  (print foo)
  (= foo 10)
  (print foo)
  (blank)
  (lazy foo (call noisy 42))
)

(fn-do + noisy Int
  (param n Int)
  (print n)
  (return n)
)
```

```output
42
42
10
```

```brix
(main
  (= foo (call Foo))
  (print foo)
  (blank)
  (lazy foo Int 42)
)

(type + Foo
  (fn-to-expr + Int
    99
  )
)
```

```output
99
```

### Two numbers compare in the type both are brought to, and a chain holds when each part does.

```brix
(main
  (print (< 1 2))
  (print (< 2 1))
  (print (<= 2 2))
  (print (<= 3 2))
  (print (> 2 1))
  (print (> 1 2))
  (print (>= 2 2))
  (print (>= 1 2))
  (print (== 1 1))
  (print (!= 1 1))
  (print (< 1.5 2.5))
  (print (<= 1.5 1.5))
  (print (> 1.5 2.5))
  (print (>= 1.5 1.5))
  (print (== 1.5 1.5))
  (print (!= 1.5 1.5))
  (print (< 1 2.5))
  (print (< 1.5 2))
  (print (== 2 2.0))
  (print (< 1.5f32 2.5))
  (print (== 0.1f32 0.1))
  (print (== 0.1 0.1f32))
  (print (> 250u8 5u8))
  (print (> -6i8 5i8))
  (print (< 200u8 100))
  (print (> 10000000000 5))
  (print (< -10000000000i64 5))
  (print (> 3000000000u32 2000000000u32))
  (print (< 'a' 'b'))
  (print (> 3 2 1))
  (print (> 3 2 2))
  (print (<= (< 1 2) 2))
  (print (== (/ 0.0 0.0) (/ 0.0 0.0)))
  (print (!= (/ 0.0 0.0) (/ 0.0 0.0)))
  (print (< (/ 0.0 0.0) 1.0))
)
```

```output
true
false
true
false
true
false
true
false
true
false
true
true
false
true
true
false
true
true
true
true
true
true
true
false
false
true
true
true
true
true
false
true
false
true
false
```

### `!`, `&&` and `||` take a number as true when it is not zero, and the answer is a Bool.

```brix
(main
  (print true)
  (print false)
  (print (not true))
  (print (not false))
  (print (not 0))
  (print (not 5))
  (print (not (not 5)))
  (print (not 1.5))
  (print (not 0.0))
  (print (and true true))
  (print (and true false))
  (print (and false true))
  (print (or false false))
  (print (or false true))
  (print (and 1 2))
  (print (and 0 2))
  (print (and 1 0))
  (print (or 0 0))
  (print (or 0 3))
  (print (and 1.5 0.0))
  (print (or 0.0 0.5))
  (print (and (< 1 2) (< 2 3)))
  (print (and (< 1 2) (< 3 2)))
  (print (or (> 1 2) (< 2 3)))
  (print (not (< 1 2)))
  (print (== (< 1 2) (< 2 3)))
  (print (== true true))
  (print (!= true false))
)
```

```output
true
false
false
true
true
false
true
false
true
true
false
false
false
true
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
false
true
true
true
```

### The value of one operator as the operand of another that converts it.

```brix
(main
  (print (< (+ 1 2) 2.5))
  (print (> 2.5 (+ 1 2)))
  (print (== (+ 1 2) 3i64))
  (print (== 3i64 (+ 1 2)))
  (print (/and/ 3i64 (+ 1 2)))
  (print (/or/ (+ 1 2) 4i64))
  (print (+ 1.5 (+ 1 2)))
  (print (* (+ 1 2) 2.5))
  (print (/ (as (+ 200u8 100u8) F64) 8))
  (print (< (- (+ 1 2)) 0.5))
  (print (== (< 1 2) (> 2.5 1)))
  (print (/lshift/ (+ 1 2) (+ 1 1)))
  (print (< (+ 1u8 2u8) 3i64))
  (print (or (< (+ 1 2) 2.5) (> (+ 1.5 2) 3)))
  (print (and (< -1 4000000000u32) (== (+ 1 2) 3.0)))
  (print (not (< (+ 1 2) 2.5)))
  (print (== (% 7 (+ 1 2)) 1.0))
)
```

```output
false
false
true
true
3
7
4.5
7.5
5.5
true
true
12
false
true
true
true
true
```

### Arms of different types each take the type they join in.

```brix
(main
  (print
    (block
      (case? (< 1 2)
        1
      )
      (case? (< 2 1)
        3i64
      )
      (default
        2.5
      )
    )
  )
  (print
    (block
      (case? (< 2 1)
        1
      )
      (case? (< 1 2)
        3i64
      )
      (default
        2.5
      )
    )
  )
  (print
    (block
      (case? (< 2 1)
        1
      )
      (default
        2.5
      )
    )
  )
)
```

```output
1
3
2.5
```

### A chain of orderings compares each value with the one after it, however long it is.

```brix
(main
  (print (> 3 2 1))
  (print (> 4 3 2 1))
  (print (> 5 4 3 2 1))
  (print (> 6 5 4 3 2 1))
  (print (< 1 2 3))
  (print (< 1 2 3 4))
  (print (< 1 2 3 4 5))
  (print (< 1 2 3 4 5 6))
  (print (> 9 8 7 6))
  (print (> 9 8 7 0))
  (print (< 1 5 3 4))
  (print (> 3 2 2))
  (print (> (< 1 3) 2))
)
```

```output
true
true
true
true
true
true
true
true
true
true
false
false
true
```

### The middle of a chain of orderings is converted for each ordering it is in.

```brix
(main
  (print (< 1 2 3.5))
  (print (< 1.5 2 3u8))
  (print (<= 1 1 1.0))
  (print (< 1 2 3.5 4i64 5u8))
  (print (< 1 (+ 1 1) 2.5))
  (print (> 3.5 3 2.5))
)
```

```output
true
true
true
true
true
true
```

### The middle of a chain of orderings is evaluated once.

```brix
(main
  (print (< 1 (call two) 3))
  (print (< 0 (call two) 3 (call five) 9))
  (print (< 5 (call two) 3))
  (print (< 1 (call two) 2 (call five)))
)

(fn-do + two Int
  (print "two")
  (return 2)
)

(fn-do + five Int
  (print "five")
  (return 5)
)
```

```output
two
true
two
five
true
two
false
two
false
```

### The middle of a chain of comparisons is a local, read once and converted for each comparison.

```brix
(main
  (let x 3)
  (print (< 1 x 5.5))
  (if (< 1 x 3)
    (print "in")
    (else
      (print "out")
    )
  )
  (let y 2.5)
  (print (> x y 1))
)
```

```output
true
out
true
```

### A chain of four or more comparisons evaluates each middle once, and stops at the first comparison that fails.

```brix
(main
  (let x 5)
  (let y 7)
  (let s "b")
  (let t "c")
  (print (< 1 2 3 4 5))
  (print (< 1 x y 9 10))
  (print (< 1 x (+ y 1) 8 10))
  (print (< 9 x (+ y 1) 8))
  (print (< "a" s (++ t "x") "z" "zz"))
  (print (< "c" s (++ t "x") "z"))
  (print (> (>= (> 5 4 3) 3) 1))
  (if (and (< 0 x y 100) s)
    (print "yes")
  )
)
```

```output
true
true
false
false
true
false
true
yes
```

### An assignment gives a value: what a local was assigned, what a property or a struct's field then holds, or what an assigning operator answers.

```brix
(prop + g _ (= 1))
(blank)

(main
  (let thing (call Thing))
  (let x (= (. thing a) 1))
  (print x)
  (let y
    (block
      (+= (. thing a) 1)
    )
  )
  (print y)
  (let p (call Point 1 2))
  (let rx (= (. p x) 8))
  (let ry (= ([] p 1) 9))
  (print rx)
  (print ry)
  (print p)
  (let n 5)
  (print (+= n (call Ten)))
  (print n)
  (let t (call Triple 20))
  (print (+= t 3))
  (print t)
  (print (. thing foo))
  (print (. thing foo))
  (let m 0)
  (let k (= m 4))
  (print (+ k m))
  (print (= g 7))
  (print g)
  (let q (call Point 3 4))
  (let r (= q (call Point 5 6)))
  (= (. q x) 0)
  (print r)
  (print (call take (= m 9) m))
  (let f Float (= m 2i32))
  (print f)
)

(fn-expr + take Int
  (param a Int)
  (param b Int)
  (+ (* a 10) b)
)

(type + Thing
  (prop + a _ (= 0))
  (prop + foo _ (=> (+= a 1)))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
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

(type + Triple (is Tuple)
  (prop + foo Int)
  (fn-expr + (infix this + (param other ''T)) _
    (call This (* foo other))
  )
)
```

```output
1
2
8
9
(8, 9)
15
15
(60)
(60)
3
4
8
7
7
(5, 6)
99
2
```

### An assignment to a property through an object gives what the property then holds.

```brix
(main
  (let thing (call Thing))
  (let w (= (. thing f) 3.5))
  (print w)
  (print (. thing f))
  (let p (call Point 1 2))
  (let s (= (. p x) 30))
  (print s)
  (print (call (. thing both) 2))
)

(type + Thing
  (prop + a _ (= 0))
  (prop + f _ (= 0.5))
  (fn-do + both Int
    (param v Int)
    (let got (= a v))
    (return got)
  )
)

(type + Point (is Struct)
  (prop + x U8)
  (prop + y U8)
)
```

```output
3.5
3.5
30
2
```

### An assignment to a property or a struct's field of another type gives the converted value.

```brix
(main
  (let p (call Point 1 2))
  (let big 300)
  (let s (= (. p x) (as big U8)))
  (print s)
  (print (. p x))
  (let thing (call Thing))
  (let w (= (. thing f) 3))
  (print (/ w 2))
  (let t (= ([] p 1) (as big U8)))
  (print t)
  (print p)
)

(type + Thing
  (prop + f _ (= 0.5))
)

(type + Point (is Struct)
  (prop + x U8)
  (prop + y U8)
)
```

```output
44
44
1.5
44
(44, 44)
```

### An assignment to a property named alone gives the value as it was before it was stored.

```brix
(main
  (let thing (call Thing))
  (call (. thing both) 3 300)
  (print (. thing f))
  (print (. thing s))
  (let cell (call Cell 1 2))
  (call (. cell put) 300)
  (print cell)
)

(type + Thing
  (prop + f _ (= 0.5))
  (prop + s U8 (= 0))
  (fn-do + both _
    (param v I32)
    (param big Int)
    (let got (= f v))
    (print (/ got 2))
    (let small (= s (as big U8)))
    (print small)
    (print (call take (= s (as (+ big 1) U8)) s))
  )
  (fn-expr _ take Int
    (param a Int)
    (param b Int)
    (+ (* a 1000) b)
  )
)

(type + Cell (is Struct)
  (prop + x U8)
  (prop + y U8)
  (fn-do + put _
    (param big Int)
    (let got (= x (as big U8)))
    (print got)
  )
)
```

```output
1
44
45045
3
45
44
(1, 2)
```

### An assignment to a local gives the value as it was before it was stored.

```brix
(main
  (let f 0.5)
  (let got (= f 3))
  (print (/ got 2))
  (print f)
  (let small U8 0)
  (let big 300)
  (let kept (= small (as big U8)))
  (print kept)
  (print small)
  (let maybe (? Int) null)
  (let held (= maybe 7))
  (print (+ held 1))
  (let p (call Point 1 2))
  (let q (call Point 3 4))
  (let r (= q p))
  (= (. p x) 9)
  (print r)
  (print q)
  (print (call pick (= small (as (+ big 2) U8)) small))
  (let h (call Holder))
  (call (. h run) 5)
)

(fn-expr + pick Int
  (param a Int)
  (param b Int)
  (+ (* a 1000) b)
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)

(type + Holder
  (prop + maybe (? Int) (= null))
  (prop + spot _ (= (call Point 0 0)))
  (prop + text _ (= ""))
  (fn-do + run _
    (param v Int)
    (let held (= maybe v))
    (print (+ held 1))
    (let p (call Point v v))
    (let took (= spot p))
    (= (. p x) 0)
    (print took)
    (print spot)
    (let said (= text (str v)))
    (print said)
    (print text)
  )
)
```

```output
1
3
44
44
8
(1, 2)
(1, 2)
46046
6
(5, 5)
(5, 5)
5
5
```

### Assignment gives the value written to a name and the value stored to a property

```brix
(main
  (let total F64 0)
  (let count 0)
  (let kept (= total 5))
  (print kept)
  (print (type-of kept))
  (let next (= count 7))
  (print next)
  (let box (call Box))
  (let stored (= (. box size) 4))
  (print stored)
  (print (type-of stored))
  (let maybe (? Int) null)
  (let given (= maybe 9))
  (print given)
  (print (type-of given))
)

(type + Box
  (prop + size F64 (= 0))
)
```

```output
5
[TypeInfo I64]
7
4
[TypeInfo F64]
9
[TypeInfo I64]
```

### Assertions under an expression run in their order before the expression is evaluated.

```brix
(main
  (do-new
    (print (call foo 1))
    (print (call foo 7))
    (print (call foo 20))
    (on-failure failure
      (print failure)
    )
  )
)

(fn-expr + foo _
  (param a Int)
  (call tick "value" a
    (block
      (throw (call Panic "over ten") (if (> b 10)))
      (throw (call Panic "over five") (if (> (call tick "second" a) 5)))
    )
  )
  (where
    (lazy b (call tick "b" a))
  )
)

(fn-do + tick _
  (param label String)
  (param value Int)
  (print (str "tick " label))
  (return value)
)
```

```output
tick b
tick second
tick value
1
tick b
tick second
over five
```

### An assertion under the value of a declaration.

```brix
(main
  (do-new
    (call run 3)
    (call run -3)
    (on-failure failure
      (print failure)
    )
  )
)

(fn-do + run _
  (param n Int)
  (let v
    (scope (* (call tick "compute" n) 2)
      (throw (call Panic "negative") (if (< n 0)))
    )
  )
  (print v)
)

(fn-do + tick _
  (param label String)
  (param value Int)
  (print (str "tick " label))
  (return value)
)
```

```output
tick compute
6
negative
```

### An error raised under an expression, with a lazy local beside it.

```brix
(main
  (print (catch (call foo 2)))
  (print (catch (call foo 9)))
)

(fn-expr + foo _
  (param a Int)
  (scope (+ a b)
    (throw (call Error "too big") (if (> a 3)))
    (lazy b (call tick "b" (* a 2)))
  )
)

(fn-do + tick _
  (param label String)
  (param value Int)
  (print (str "tick " label))
  (return value)
)
```

```output
tick b
6
too big
```

### An error raised under what a function returns.

```brix
(main
  (print (catch (call run 2)))
  (print (catch (call run 9)))
)

(fn-do + run Int
  (param n Int)
  (return
    (scope (* (call tick "answer" n) 2)
      (throw (call Error "too big") (if (> n 5)))
    )
  )
)

(fn-do + tick _
  (param label String)
  (param value Int)
  (print (str "tick " label))
  (return value)
)
```

```output
tick answer
4
too big
```

### An assertion under the value an assignment adds.

```brix
(main
  (do-new
    (call show 1)
    (call show 8)
    (on-failure failure
      (print failure)
    )
  )
)

(fn-do + show _
  (param n Int)
  (let list (list 1 2 3))
  (+= list
    (scope n
      (throw (call Panic "eight") (if (== n 8)))
    )
  )
  (print (. list length))
)
```

```output
4
eight
```

### Lazy locals under the values of globals and of properties are evaluated by their reads.

```brix
(main
  (let thing (call Thing))
  (print (. thing a))
  (print (. thing c))
  (print total)
  (print label)
)

(blank)
(prop + total _
  (=
    (scope (+ low high)
      (lazy low (call step "low" 1))
      (lazy high (call step "high" 2))
    )
  )
)
(blank)
(prop + label String
  (=
    (scope (str word "!")
      (lazy word "hi")
    )
  )
)

(fn-do + step _
  (param name String)
  (param n Int)
  (print (str "step " name))
  (return n)
)

(type + Thing
  (prop + a _
    (=
      (scope (* b 2)
        (lazy b (call step "b" 10))
      )
    )
  )
  (blank)
  (prop + c _
    (=
      (scope (if-expr (> a 5) d 0)
        (lazy d (call step "d" 7))
      )
    )
  )
)
```

```output
step low
step high
step b
step d
20
7
3
hi!
```

### A branch on a comparison of integers that are only known when the program runs, and a branch on each negated comparison, takes the path the comparison's value names, signed or unsigned.

```brix
(main
  (let values (list 3 5 7))
  (for (in x values)
    (call signed-paths x ([] values 1))
  )
  (let large (list 3u64 18446744073709551615u64))
  (call unsigned-paths ([] large 0) ([] large 1))
  (call unsigned-paths ([] large 1) ([] large 0))
  (call unsigned-paths ([] large 1) ([] large 1))
)

(fn-do + signed-paths _
  (param x I64)
  (param y I64)
  (let taken "")
  (if (< x y)
    (= taken (str taken " <"))
  )
  (if (not (< x y))
    (= taken (str taken " !<"))
  )
  (if (not (<= x y))
    (= taken (str taken " !<="))
  )
  (if (not (> x y))
    (= taken (str taken " !>"))
  )
  (if (not (>= x y))
    (= taken (str taken " !>="))
  )
  (if (== x y)
    (= taken (str taken " =="))
  )
  (if (not (== x y))
    (= taken (str taken " !=="))
  )
  (if (not (!= x y))
    (= taken (str taken " !!="))
  )
  (print (str x " ? " y ":" taken))
)

(fn-do + unsigned-paths _
  (param x U64)
  (param y U64)
  (let taken "")
  (if (< x y)
    (= taken (str taken " <"))
  )
  (if (not (< x y))
    (= taken (str taken " !<"))
  )
  (if (not (<= x y))
    (= taken (str taken " !<="))
  )
  (if (not (> x y))
    (= taken (str taken " !>"))
  )
  (if (not (>= x y))
    (= taken (str taken " !>="))
  )
  (print (str x " ? " y ":" taken))
)
```

```output
3 ? 5: < !> !>= !==
5 ? 5: !< !> == !!=
7 ? 5: !< !<= !==
3 ? 18446744073709551615: < !> !>=
18446744073709551615 ? 3: !< !<=
18446744073709551615 ? 18446744073709551615: !< !>
```

### A negated ordering of floats holds when either is not a number, where the opposite ordering would not. A negated equality is the opposite equality.

```brix
(main
  (let zero 0.0f64)
  (let values (list 1.0f64 2.0f64 (/ zero zero)))
  (for (in x values)
    (call float-paths x ([] values 1))
  )
  (call float-paths ([] values 1) ([] values 2))
)

(fn-do + float-paths _
  (param x F64)
  (param y F64)
  (let taken "")
  (if (< x y)
    (= taken (str taken " <"))
  )
  (if (not (< x y))
    (= taken (str taken " !<"))
  )
  (if (not (<= x y))
    (= taken (str taken " !<="))
  )
  (if (not (> x y))
    (= taken (str taken " !>"))
  )
  (if (not (>= x y))
    (= taken (str taken " !>="))
  )
  (if (== x y)
    (= taken (str taken " =="))
  )
  (if (not (== x y))
    (= taken (str taken " !=="))
  )
  (if (not (!= x y))
    (= taken (str taken " !!="))
  )
  (print (str "paths:" taken))
)
```

```output
paths: < !> !>= !==
paths: !< !> == !!=
paths: !< !<= !> !>= !==
paths: !< !<= !> !>= !==
```

### A branch on a comparison with a literal takes the same path at the edges of each integer width, and a branch on a Bool or its negation follows the Bool.

```brix
(main
  (for (in x (list 0u8 3u8 255u8))
    (call byte-edges x)
  )
  (for (in x (list -128i8 -1i8 0i8 127i8))
    (call signed-byte-edges x)
  )
  (for (in x (list 4095u16 4096u16 65535u16))
    (call short-edges x)
  )
  (for (in x (list -32768i16 -1i16 4095i16))
    (call signed-short-edges x)
  )
  (for (in x (list -1i32 0i32 4096i32))
    (call word-edges x)
  )
  (for (in x (list -1i64 4095i64 4096i64 4097i64 16773120i64))
    (call wide-edges x)
  )
  (for (in flag (list true false))
    (call flag-paths flag (not flag))
  )
)

(fn-do + byte-edges _
  (param x U8)
  (let taken "")
  (if (== x 0u8)
    (= taken (str taken " ==0"))
  )
  (if (== x 255u8)
    (= taken (str taken " ==255"))
  )
  (if (< x 3u8)
    (= taken (str taken " <3"))
  )
  (if (not (>= x 255u8))
    (= taken (str taken " !>=255"))
  )
  (print (str x ":" taken))
)

(fn-do + signed-byte-edges _
  (param x I8)
  (let taken "")
  (if (== x -128i8)
    (= taken (str taken " ==-128"))
  )
  (if (== x -1i8)
    (= taken (str taken " ==-1"))
  )
  (if (== x 127i8)
    (= taken (str taken " ==127"))
  )
  (if (< x 0i8)
    (= taken (str taken " <0"))
  )
  (if (not (> x -1i8))
    (= taken (str taken " !>-1"))
  )
  (print (str x ":" taken))
)

(fn-do + short-edges _
  (param x U16)
  (let taken "")
  (if (== x 4095u16)
    (= taken (str taken " ==4095"))
  )
  (if (== x 4096u16)
    (= taken (str taken " ==4096"))
  )
  (if (== x 65535u16)
    (= taken (str taken " ==65535"))
  )
  (if (> x 4096u16)
    (= taken (str taken " >4096"))
  )
  (print (str x ":" taken))
)

(fn-do + signed-short-edges _
  (param x I16)
  (let taken "")
  (if (== x -32768i16)
    (= taken (str taken " ==-32768"))
  )
  (if (== x -1i16)
    (= taken (str taken " ==-1"))
  )
  (if (== x 4095i16)
    (= taken (str taken " ==4095"))
  )
  (if (< x 0i16)
    (= taken (str taken " <0"))
  )
  (print (str x ":" taken))
)

(fn-do + word-edges _
  (param x I32)
  (let taken "")
  (if (== x -1i32)
    (= taken (str taken " ==-1"))
  )
  (if (== x 0i32)
    (= taken (str taken " ==0"))
  )
  (if (== x 4096i32)
    (= taken (str taken " ==4096"))
  )
  (if (< x 0i32)
    (= taken (str taken " <0"))
  )
  (print (str x ":" taken))
)

(fn-do + wide-edges _
  (param x I64)
  (let taken "")
  (if (== x -1i64)
    (= taken (str taken " ==-1"))
  )
  (if (== x 4095i64)
    (= taken (str taken " ==4095"))
  )
  (if (== x 4096i64)
    (= taken (str taken " ==4096"))
  )
  (if (== x 4097i64)
    (= taken (str taken " ==4097"))
  )
  (if (== x 16773120i64)
    (= taken (str taken " ==16773120"))
  )
  (if (> x 4096i64)
    (= taken (str taken " >4096"))
  )
  (if (not (>= x 4097i64))
    (= taken (str taken " !>=4097"))
  )
  (print (str x ":" taken))
)

(fn-do + flag-paths _
  (param flag Bool)
  (param other Bool)
  (let taken "")
  (if flag
    (= taken (str taken " flag"))
  )
  (if (not flag)
    (= taken (str taken " !flag"))
  )
  (if (== flag other)
    (= taken (str taken " ==other"))
  )
  (if (!= flag other)
    (= taken (str taken " !=other"))
  )
  (if (not (== flag other))
    (= taken (str taken " !==other"))
  )
  (print (str flag ":" taken))
)
```

```output
0: ==0 <3 !>=255
3: !>=255
255: ==255
-128: ==-128 <0 !>-1
-1: ==-1 <0 !>-1
0:
127: ==127
4095: ==4095
4096: ==4096
65535: ==65535 >4096
-32768: ==-32768 <0
-1: ==-1 <0
4095: ==4095
-1: ==-1 <0
0: ==0
4096: ==4096
-1: ==-1 !>=4097
4095: ==4095 !>=4097
4096: ==4096 !>=4097
4097: ==4097 >4096
16773120: ==16773120 >4096
true: flag !=other !==other
false: !flag !=other !==other
```
