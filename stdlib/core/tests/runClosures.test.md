# Closures

```brix
(main
  (let a
    (=> ()
      (block
        42
      )
    )
  )
  (print (call a))
)
```

```output
42
```

```brix
(main
  (let a
    (=> () :do
      (block
        (print "hello")
      )
    )
  )
  (call a)
)
```

```output
hello
```

```brix
(main
  (let a (=> (x) 42))
  (print (call a 1))
)
```

```output
42
```

### Different instances of closure with different generic argument types

```brix
(main
  (let a
    (=> (b) :do
      (block
        (print (str "hello " b))
      )
    )
  )
  (call a 24)
  (call a 24.5)
)
```

```output
hello 24
hello 24.5
```

### Call closure passed in as argument

```brix
(main
  (let a (=> (b) b))
  (print (call foo a 24))
  (print (call foo a 24.5))
)

(fn-expr + foo _
  (param fn _)
  (param n _)
  (call fn n)
)
```

```output
24
24.5
```

```brix
(main
  (let n 42)
  (let a (=> (b) (* b n k)))
  (blank)
  (print (call foo a 2))
  (= n 100)
  (print (call foo a 4))
)

(blank)
(prop _ k _ (= 10))

(fn-expr _ foo _
  (param fn _)
  (param n _)
  (call fn n)
)
```

```output
840
4000
```

```brix
(main
  (let n 42)
  (let a
    (=> (b)
      (block
        bn
        (lazy bn (* b n))
      )
    )
  )
  (blank)
  (print (call a 2))
)
```

```output
84
```

### Multiple closures

```brix
(main
  (let n 42)
  (let d 2)
  (let a (=> (b) (* b n)))
  (let a2 (=> (b) (* b n d)))
  (blank)
  (print (call a 2))
  (print (call a2 2))
)
```

```output
84
168
```

### Modify closure

```brix
(main
  (let n 42)
  (let mutate
    (=> (b) :do
      (block
        (*= n b)
      )
    )
  )
  (blank)
  (call mutate 2)
  (print n)
)
```

```output
84
```

### Multiple closures, one modifies, then the other can see modification

```brix
(main
  (let n 42)
  (let mutate
    (=> (b) :do
      (block
        (*= n b)
      )
    )
  )
  (let ref (=> (b) (* n b)))
  (blank)
  (call mutate 2)
  (print (call ref 2))
)
```

```output
168
```

### Assign to variable declared after closure

```brix
(main
  (let mutate
    (=> () :do
      (block
        (= n 2)
      )
    )
  )
  (let n 42)
  (blank)
  (call mutate)
  (print n)
)
```

```output
2
```

### Nested closure

```brix
(main
  (let n 42)
  (let a
    (=> (b)
      (block
        (=> ()
          (block
            (* n b)
          )
        )
      )
    )
  )
  (blank)
  (let f (call a 2))
  (print (call f))
)
```

```output
84
```

### Doubly nested closure

```brix
(main
  (let n 42)
  (let a
    (=> ()
      (block
        (=> ()
          (block
            (=> ()
              (block
                n
              )
            )
          )
        )
      )
    )
  )
  (blank)
  (let f (call a))
  (let b (call f))
  (print (call b))
)
```

```output
42
```

### Nested closure capturing lazy variable

```brix
(main
  (let n 42)
  (let a
    (=> ()
      (block
        (=> (b)
          (block
            (* n b c)
          )
        )
        (lazy c 2)
      )
    )
  )
  (blank)
  (print (call (call a) 2))
)
```

```output
168
```

### Nested closure mutating lazy variable

```brix
(main
  (let a
    (=> ()
      (block
        (=> (b) :do
          (block
            (+= c b)
            (return c)
          )
        )
        (lazy c 0)
      )
    )
  )
  (blank)
  (let f (call a))
  (print (call f 2))
  (print (call f 3))
)
```

```output
2
5
```

### Closures at same level reference each other

```brix
(main
  (let n 42)
  (let x
    (=> ()
      (block
        n
      )
    )
  )
  (let a
    (=> ()
      (block
        (call x)
      )
    )
  )
  (blank)
  (print (call a))
)
```

```output
42
```

### Nested closure mutation

```brix
(main
  (let n 42)
  (let a
    (=> ()
      (block
        (=> (b) :do
          (block
            (*= n b)
          )
        )
      )
    )
  )
  (blank)
  (call (call a) 2)
  (print n)
)
```

```output
84
```

```brix
(main
  (let n 42)
  (let a
    (=> ()
      (block
        (=> ()
          (block
            (=> (b) :do
              (block
                (*= n b)
              )
            )
          )
        )
      )
    )
  )
  (blank)
  (call (call (call a)) 2)
  (print n)
)
```

```output
84
```

### Closure in method binds to 'this'

```brix
(main
  (let thing (call Thing))
  (let get-self (. thing get-self))
  (print (. (call get-self) a))
  (blank)
  (let get-a (. thing get-a))
  (print (call get-a))
  (= (. thing a) 10)
  (print (call get-a))
  (print (. (call get-self) a))
)

(type + Thing
  (prop + a _ (= 5))
  (blank)
  (prop + get-self (of Getter This) (:= (=> () this)))
  (prop + get-a (of Getter Int) (:= (=> () a)))
)
```

```output
5
5
10
10
```

### Illegal closure in property

```brix crash
(main
  (let thing (call Thing))
  (let get-self (. thing get-self))
  (print (call get-self))
)

(type + Thing
  (prop + get-self _
    (=
      (scope (as cb (of Getter This))
        (lazy cb (=> () this))
      )
    )
  )
)
```

```output
Exception: Illegal to reference 'this' in eager properties or properties referenced by them
<no file:12>
            (lazy cb (=> () this))
                            ^^^^
```

### Refer to property in closure within method

```brix
(main
  (let thing (call Thing))
  (print (call (call (. thing get-self))))
)

(type + Thing
  (prop + a _ (= 5))
  (fn-expr + get-self _
    (=> ()
      (block
        a
      )
    )
  )
)
```

```output
5
```

## Closures in methods

```brix
(main
  (let thing (call Thing))
  (let s (call (. thing foo)))
  (print (call (call s) 42))
)

(type + Thing
  (fn-expr + foo _
    (=> ()
      (block
        bar
      )
    )
  )
  (fn-expr + bar _
    (param n I32)
    (str "bar " n)
  )
)
```

```output
bar 42
```

```brix
(main
  (let thing (call Thing))
  (let s (call (. thing foo)))
  (print (call s))
)

(type + Thing
  (prop + baz _ (= 42))
  (fn-expr + foo _
    (=> ()
      (block
        baz
      )
    )
  )
)
```

```output
42
```

```brix
(main
  (let thing (call Thing))
  (let s (call (. thing foo)))
  (print (call (call s) 42))
)

(type + Thing
  (fn-expr + foo _
    (=> ()
      (block
        baz
      )
    )
  )
  (fn-expr + baz _
    (param n I32)
    (str "baz " n)
  )
)
```

```output
baz 42
```

```brix crash
(main
  (let thing (call Thing))
  (let s (call (. thing foo)))
  (print (call (call s) 42))
)

(type + Thing
  (fn-expr + foo _
    (=> ()
      (block
        bar
      )
    )
  )
)
```

```output
Exception: Name "bar" not found
<no file:12>
            bar
            ^^^
```

## Closures in methods of non-classes

```brix
(main
  (let thing (call Thing))
  (let s (call (. thing foo)))
  (print (call (call s) 42))
)

(type + Thing (is I32)
  (fn-expr + foo _
    (=> ()
      (block
        bar
      )
    )
  )
  (fn-expr + bar _
    (param n I32)
    (str "bar " n)
  )
)
```

```output
bar 42
```

```brix
(main
  (let thing (call Thing))
  (let s (call (. thing foo)))
  (print (call s))
)

(blank)
(prop + baz _ (= 42))

(type + Thing (is I32)
  (fn-expr + foo _
    (=> () baz)
  )
)
```

```output
42
```

```brix
(main
  (let thing (call Thing))
  (let s (call (. thing foo)))
  (print (call (call s) 42))
)

(type + Thing (is I32)
  (fn-expr + foo _
    (=> () baz)
  )
  (fn-expr + baz _
    (param n I32)
    (str "baz " n)
  )
)
```

```output
baz 42
```

```brix crash
(main
  (let thing (call Thing))
  (let s (call (. thing foo)))
  (print (call (call s) 42))
)

(type + Thing (is I32)
  (fn-expr + foo _
    (=> () bar)
  )
)
```

```output
Exception: Name "bar" not found
<no file:10>
        (=> () bar)
               ^^^
```

## Closures in generators

```brix
(main
  (let gen (call foo 1.5))
  (for (in f gen)
    (call f)
  )
)

(fn-do + foo _
  (param n _)
  (yield
    (=> () :do
      (block
        (print n)
      )
    )
  )
)
```

```output
1.5
```

## Casting closures to function types

```brix
(main
  (let a (=> () 42))
  (let foo (as a Foo))
  (print (call foo))
)

(type + Foo (is (fn-type Int)))
```

```output
42
```

```brix
(main
  (let a (=> (n) (+ 42 n)))
  (let foo (as a Foo))
  (print (call foo 1))
)

(type + Foo (is (fn-type Int (param n Int))))
```

```output
43
```

```brix
(main
  (let a (=> ((: n Float)) (+ 42.5 n)))
  (let foo (as a Foo))
  (print (call foo 1))
)

(type + Foo (is (fn-type Int (param n Int))))
```

```output
43
```

```brix
(main
  (let a (=> ((: n Int)) (+ 42 n)))
  (print (call call1 a))
)

(type + Fooi (is (fn-type Int (param n Int))))

(type + Foof (is (fn-type Int (param n I32))))

(fn-expr + call1 _
  (param fn Fooi)
  (call call2 fn)
)

(fn-expr + call2 _
  (param fn Foof)
  (call fn 10)
)
```

```output
52
```

```brix
(main
  (let a (=> ((: n Int)) (+ 42 n)))
  (let f1 (as a Fooi))
  (let f2 (as f1 Foof))
  (print (call f2 10))
)

(type + Thing
  (fn-from-expr + (: other Int)
    (call Thing)
  )
  (fn-to-expr + Int
    5
  )
)

(type + Fooi (is (fn-type Int (param n Int))))

(type + Foof (is (fn-type Float (param n Thing))))
```

```output
47
```

```brix crash
(main
  (let a (=> (n) (+ 42 n)))
  (let foo (as a Foo))
  (print (call foo 1 2))
)

(type + Foo (is (fn-type Int (param n Int))))
```

```output
Exception: Wrong number of arguments
<no file:5>
      (print (call foo 1 2))
             ^^^^^^^^^^^^^^
```

```brix crash
(main
  (let a (=> (n) (+ 42 n)))
  (let foo (as a Foo))
  (print (call foo 1 2))
)

(type + Foo (is (fn-type Int (param n Int))))
```

```output
Exception: Wrong number of arguments
<no file:5>
      (print (call foo 1 2))
             ^^^^^^^^^^^^^^
```

```brix
(main
  (let a
    (=> () :do
      (block
        (print "hi")
      )
    )
  )
  (blank)
  (let foo (as a Foo))
  (call foo)
)

(type + Foo (is (fn-type Void)))
```

```output
hi
```

```brix
(main
  (let a
    (=> (n) :do
      (block
        (print n)
      )
    )
  )
  (let foo (as a Foo))
  (call foo 42)
)

(type + Foo (is (fn-type Void (param n Int))))
```

```output
42
```

### Cast to function with different return type

```brix
(main
  (let a (=> () 42))
  (let foo (as a Foo))
  (print (call foo))
)

(type + Foo (is (fn-type Float)))
```

```output
42
```

### Cast to function with return type with compatible converter

```brix
(main
  (let a (=> () 42))
  (let foo (as a Foo))
  (print (. (call foo) num))
)

(type + Thing
  (prop + num _ (= 0))
  (fn-from-do + (: n Int)
    (let t (call Thing))
    (= (. t num) n)
    (return t)
  )
)

(type + Foo (is (fn-type Thing)))
```

```output
42
```

### Cast to function with different arguments

```brix crash
(main
  (let a
    (=> (n)
      (block
        (print n)
      )
    )
  )
  (let foo (as a Foo))
  (call foo)
)

(type + Foo (is (fn-type Void)))
```

```output
Exception: closure cannot be cast to Foo; there is no conversion between them
<no file:10>
      (let foo (as a Foo))
               ^^^^^^^^^^
```

### Cast function with no return type to function with return type

```brix crash
(main
  (let a
    (=> () :do
      (block
        (print "hi")
      )
    )
  )
  (blank)
  (let foo (as a Foo))
  (print (call foo))
)

(type + Foo (is (fn-type Int)))
```

```output
Exception: Void cannot be cast to I64; there is no conversion between them
<no file:11>
      (let foo (as a Foo))
               ^^^^^^^^^^
```

### Cast to function with incompatible return type

```brix crash
(main
  (let a (=> () 42))
  (let foo (as a Foo))
  (print (call foo))
)

(type + Thing)

(type + Foo (is (fn-type Thing)))
```

```output
Exception: I64 cannot be cast to Thing; there is no conversion between them
<no file:4>
      (let foo (as a Foo))
               ^^^^^^^^^^
```

### Closure within interface function

```brix
(main
  (let thing (as (call Thing) Foo))
  (print thing)
  (call (. thing foo))
)

(type + Foo (is Interface)
  (fn-expr + foo _)
)

(type + Thing
  (implements Foo)
  (fn-do + foo _
    (let x 42)
    (let a
      (=> () :do
        (block
          (print x)
        )
      )
    )
    (call a)
  )
)
```

```output
[Interface __source__:Foo]
42
```

### Crashes because we don't correctly rewrite closure inside of a generator It crashes accessing foo because this was loaded from closure field 2 but was written to field 0

```brix
(main
  (let thing (call Thing))
  (for (in a (call (. thing action)))
    (print a)
  )
)

(type + Thing
  (prop + foo _ (= 42))
  (fn-do + action _
    (let getter (=> () foo))
    (print (call getter))
    (yield 1)
  )
)
```

```output
42
1
```

### A generic function whose parameter is an inline function type infers its type argument from the closure.

```brix
(main
  (print (call apply 4 (=> (n) (+ n 1))))
  (print (call apply 4 (=> (n) (str "n = " n))))
)

(fn-expr + (of apply Result) _
  (param n Int)
  (param func (fn-type Result (param value Int)))
  (call func n)
)
```

```output
5
n = 4
```

### A local is one cell for the function that declares it and the closure that captures it.

```brix
(main
  (let total 0)
  (let add
    (=> (amount) :do
      (block
        (+= total amount)
        (return total)
      )
    )
  )
  (print (call add 2))
  (+= total 10)
  (print (call add 3))
  (print total)
)
```

```output
2
15
15
```

### Two closures share a local, and a closure in a closure reaches it two environments out.

```brix
(main
  (let count 0)
  (let bump
    (=> () :do
      (block
        (+= count 1)
      )
    )
  )
  (let read (=> () count))
  (call bump)
  (call bump)
  (print (call read))
  (let outer
    (=> (step) :do
      (block
        (let inner
          (=> (times) :do
            (block
              (+= count (* step times))
              (return count)
            )
          )
        )
        (return (+ (call inner 2) (call inner 3)))
      )
    )
  )
  (print (call outer 10))
  (print count)
)
```

```output
2
74
52
```

### Each run of a function has its own environment, which outlives the run.

```brix
(main
  (let first (call counter 10))
  (let second (call counter 100))
  (print (call first 1))
  (print (call first 2))
  (print (call second 5))
  (print (call first 3))
)

(fn-do + counter _
  (param start Int)
  (let held start)
  (let step
    (=> (amount) :do
      (block
        (+= held amount)
        (return held)
      )
    )
  )
  (return step)
)
```

```output
11
13
105
16
```

### A closure whose value is a closure, called after the closure that wrote it has returned.

```brix
(main
  (let scale 3)
  (let make
    (=> (base)
      (block
        (=> (amount) (+ (* amount scale) base))
      )
    )
  )
  (let plus-one (call make 1))
  (let plus-ten (call make 10))
  (print (call plus-one 2))
  (= scale 4)
  (print (call plus-one 2))
  (print (call plus-ten 2))
)
```

```output
7
9
18
```

### A closure written in a method reads and assigns the properties of the object it was made on.

```brix
(main
  (let first (call Tally))
  (let second (call Tally))
  (let add (call (. first adder)))
  (let other (call (. second adder)))
  (print (call add 2))
  (print (call add 3))
  (print (call other 10))
  (= (. first total) 100)
  (print (call add 1))
  (print (. second total))
)

(type + Tally
  (prop + total _ (= 0))
  (fn-do + adder _
    (let add
      (=> (amount) :do
        (block
          (+= total amount)
          (return total)
        )
      )
    )
    (return add)
  )
)
```

```output
2
5
10
101
10
```

### A text and an object that a closure captures survive the collector.

```brix
(main
  (let label "start")
  (let box (call Box))
  (let note
    (=> (text) :do
      (block
        (= label (str label "/" text))
        (+= (. box count) 1)
      )
    )
  )
  (call note "a")
  (call churn)
  (call note "b")
  (call churn)
  (print label)
  (print (. box count))
)

(type + Box
  (prop + count _ (= 0))
)

(fn-do + churn _
  (let kept "")
  (let turn 0)
  (while (< turn 20000)
    (= kept (str "text " turn))
    (+= turn 1)
  )
)
```

```output
start/a/b
2
```

### A closure written as the value of a method reads a property and calls a method.

```brix
(main
  (let thing (call Thing))
  (let read (call (. thing reader)))
  (print (call read))
  (= (. thing size) 7)
  (print (call read))
  (print (call (call (. thing twice))))
)

(type + Thing
  (prop + size _ (= 2))
  (fn-expr + reader _
    (=> () (* size 10))
  )
  (fn-expr + twice _
    (=> () (call double size))
  )
  (fn-expr + double Int
    (param amount Int)
    (* amount 2)
  )
)
```

```output
20
70
14
```

### A method read by its name inside a closure binds the object, and is called outside its class.

```brix
(main
  (let thing (call Thing))
  (let s (call (. thing held)))
  (let bound (call s))
  (print (call bound 42))
)

(type + Thing
  (fn-do + foo _
    (let getter (=> () bar))
    (let bound (call getter))
    (return (call bound 42))
  )
  (fn-do + direct _
    (let bound bar)
    (return (call bound 42))
  )
  (fn-expr + held _
    (=> () bar)
  )
  (fn-expr + bar _
    (param n I32)
    (str "bar " n)
  )
)
```

```output
bar 42
```

### Nested functions in the members of a number type read the number.

```brix
(main
  (let m (call Meters 5))
  (let add (call (. m adder) 10))
  (print (call add 1))
  (print (call add 2))
  (let twice (call (. m twice)))
  (print (call twice))
)

(type + Meters (is I32)
  (fn-expr + adder _
    (param base I32)
    (=> ((: n I32)) (+ (as this I32) base n))
  )
  (fn-expr + twice _
    (=> () (call scaled 2))
  )
  (fn-expr + scaled I32
    (param by I32)
    (* (as this I32) by)
  )
)
```

```output
16
17
10
```

### A nested function in a member of an enum calls a member on the case.

```brix
(main
  (let c (. Color green))
  (let name (call (. c namer)))
  (print (call name 1))
  (let other (call (. Color red namer)))
  (print (call other 2))
)

(type + Color (is U8)
  (variant red)
  (variant green)
  (fn-expr + namer _
    (=> ((: n Int)) (+ n (call code)))
  )
  (fn-expr + code Int
    (if-expr (== this (. Color red)) 10 20)
  )
)
```

```output
21
12
```

### A nested function in one, in a member of a number type.

```brix
(main
  (let m (call Meters 5))
  (let far (call (. m later)))
  (print (call (call far)))
)

(type + Meters (is I32)
  (fn-expr + later _
    (=> () (=> () (+ (as this I32) 1)))
  )
)
```

```output
6
```

### A nested function in a member of a type that is a text.

```brix
(main
  (let w (call Word "hey"))
  (let shout (call (. w shouter)))
  (print (call shout 2))
  (print (call shout 1))
)

(type + Word (is String)
  (fn-expr + shouter _
    (=> ((: n Int)) (if-expr (> n 1) (str (as this String) "!") (str (as this String))))
  )
)
```

```output
hey!
hey
```

### A closure that states its argument's type is made a value of a function type that gives it a number of another type and takes another from it.

```brix
(main
  (let a (=> ((: n Int)) (+ 42 n)))
  (let f (as a Foof))
  (print (call f 10.7))
  (print (call f -3.2))
)

(type + Foof (is (fn-type Float (param n Float))))
```

```output
52
39
```

### A closure of two typed arguments that captures a local is made a value of a function type whose arguments and answer are of other types.

```brix
(main
  (let k 3)
  (let a (=> ((: x Int) (: y Float)) (+ (* x k) y)))
  (let f (as a Mix))
  (print (call f 2.9 4))
  (= k 5)
  (print (call f 2.9 4))
)

(type + Mix (is (fn-type Int (param x Float) (param y Int))))
```

```output
10
14
```

### A closure's answer is made a number from a truth, and a truth from a number, by the function type it is given to.

```brix
(main
  (let a (=> ((: n Int)) (> n 2)))
  (let f (as a Num))
  (print (call f 3.5))
  (print (call f 1.5))
  (let b (=> ((: n Int)) (- n 3)))
  (let g (as b Truth))
  (print (call g 3))
  (print (call g 4))
)

(type + Num (is (fn-type Int (param n Float))))

(type + Truth (is (fn-type Bool (param n Int))))
```

```output
1
0
false
true
```

### A closure's answer is made an optional, and a narrower number, by the function type it is given to.

```brix
(main
  (let a (=> ((: n Int)) (+ n 1)))
  (let f (as a Opt))
  (print (?? (call f 3) -1))
  (let g (as a Small))
  (print (call g 300))
  (print (call g 127))
)

(type + Opt (is (fn-type (? Int) (param n Int))))

(type + Small (is (fn-type I8 (param n I16))))
```

```output
4
45
-128
```

### A function, a closure with no type stated and a closure with one are added to a list of a function type and called in turn.

```brix
(main
  (let fns (list Whole))
  (+= fns half)
  (+= fns (=> (n) (* n 3)))
  (let third (=> ((: n Float)) (/ n 3)))
  (+= fns third)
  (for (in fn fns)
    (print (call fn 9))
  )
)

(type + Whole (is (fn-type Float (param n I32))))

(fn-expr + half _
  (param n Float)
  (/ n 2)
)
```

```output
4.5
27
3
```

### A closure with a typed argument is given to a function, written where it is given, and returned as a value of a function type.

```brix
(main
  (let a (=> ((: n Int)) (+ n 0.5)))
  (print (call run a))
  (print (call run (=> ((: n Int)) (* n 2.0))))
  (print (call (call keep a) 4))
)

(type + Half (is (fn-type Float (param n I32))))

(fn-expr + run _
  (param fn Half)
  (call fn 2)
)

(fn-expr + keep Half
  (param fn Half)
  fn
)
```

```output
2.5
4
4.5
```

### Closures written where a function takes a function type state other types for their argument, or none.

```brix
(main
  (print (call run (=> ((: n Int)) (+ n 0.5))))
  (print (call run (=> ((: n Float)) (* n 2))))
  (print (call run (=> (n) (* n 1.5))))
)

(type + Half (is (fn-type Float (param n I32))))

(fn-expr + run _
  (param fn Half)
  (call fn 3)
)
```

```output
3.5
6
4.5
```

### Closures that take a value of an interface and an object of a base class are given to function types that give an object of the class and of one derived from it.

```brix
(main
  (let a (=> ((: label Label)) (call (. label label))))
  (let f (as a Taker))
  (print (call f (call Thing "one")))
  (let b (=> ((: base Base)) (call (. base kind))))
  (print (call run b))
  (let c (=> ((: thing Thing)) thing))
  (let g (as c Shown))
  (print (call (. (call g (call Thing "two")) label)))
)

(type + Taker (is (fn-type String (param thing Thing))))

(type + Shown (is (fn-type Label (param thing Thing))))

(type + Lower (is (fn-type String (param derived Derived))))

(type + Label (is Interface)
  (fn-expr + label String)
)

(type + Thing
  (implements Label)
  (prop + name _ (= ""))
  (fn-do + This _
    (param name _)
  )
  (fn-expr + label String
    (str "thing " name)
  )
)

(type + Base
  (fn-expr + kind String
    "base"
  )
)

(type + Derived (is Base)
  (fn-expr + kind String
    "derived"
  )
)

(fn-expr + run _
  (param fn Lower)
  (call fn (call Derived))
)
```

```output
thing one
derived
thing two
```

### A closure with a typed argument, written where a function takes a function type, panics.

```brix crash
(main
  (let shown
    (call run
      (:fn
        (=> ((: n Int)) :do
          (block
            (let items (list 1 2 3))
            (return ([] items n))
          )
        )
      )
    )
  )
  (print shown)
)

(type + Pick (is (fn-type Int (param n I32))))

(fn-expr + run _
  (param fn Pick)
  (+ (call fn 1) (call fn 9))
)
```

```stderr
Unhandled panic: List index 9 out of range
<core/list.bx>

Task 1

Stack trace:
  0  core/list:List:absent core/list.bx
  1  __source__:shown no file:9:21
  2  __source__:shownCasting no file:5:7
  3  __source__:run no file:22:18
  4  __source__:main no file:4:5
  5  __source__:init.__source__ no file
  6  __source__:brix.main no file
```

### A closure and a function that take an object are given where a function type gives a number the object's class is made from.

```brix
(main
  (let a (=> ((: thing Thing)) (. thing n)))
  (let f (as a Taker))
  (print (call f 4))
  (print (call run show))
)

(type + Taker (is (fn-type Int (param n Int))))

(fn-expr + show _
  (param thing Thing)
  (+ (. thing n) 2)
)

(fn-expr + run _
  (param fn Taker)
  (call fn 7)
)

(type + Thing
  (prop + n _ (= 0))
  (fn-from-do + (: n Int)
    (let made (call Thing))
    (= (. made n) (* n 10))
    (return made)
  )
  (fn-to-expr + Int
    (+ n 1)
  )
  (fn-expr + (prefix ! this) _
    (== n 0)
  )
)
```

```output
40
72
```

### A closure that takes a text is given a number by a function type, and one that answers a number is cast to a type that answers text.

```brix
(main
  (let a (=> ((: text String)) (. text length)))
  (let f (as a Count))
  (print (call f 42))
  (let b (=> ((: n Int)) n))
  (let g (as b Text))
  (print (call g 7))
)

(type + Count (is (fn-type Int (param n Int))))

(type + Text (is (fn-type String (param n Int))))
```

```output
2
7
```

### A closure that takes a number is cast to a function type that gives it text.

```brix
(main
  (let a (=> ((: n Int)) (+ n 1)))
  (let f (as a Texty))
  (print (call f "0"))
)

(type + Texty (is (fn-type Int (param n String))))
```

```output
1
```

### A closure with a typed argument panics, called as a value of a function type that gives it another number: the stack trace has the closure and the function made for the value.

```brix crash
(main
  (let items (list 1 2 3))
  (let picker (=> ((: n Int)) ([] items n)))
  (let f (as picker Pick))
  (print (call f 1.9))
  (print (call f 7.5))
)

(type + Pick (is (fn-type Float (param n Float))))
```

```stderr
Unhandled panic: List index 7 out of range
<core/list.bx>

Task 1

Stack trace:
  0  core/list:List:absent core/list.bx
  1  __source__:picker no file:4:31
  2  __source__:pickerCasting no file:5:10
  3  __source__:main no file:7:10
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### A closure with a typed argument added to a list of a function type panics, called in a loop.

```brix crash
(main
  (let chosen (list Pick))
  (let items (list 1 2 3))
  (+= chosen (=> ((: n Int)) ([] items n)))
  (for (in fn chosen)
    (print (call fn 0))
    (print (call fn 5))
  )
)

(type + Pick (is (fn-type Int (param n I32))))
```

```stderr
Unhandled panic: List index 5 out of range
<core/list.bx>

Task 1

Stack trace:
  0  core/list:List:absent core/list.bx
  1  __source__:chosen no file:5:30
  2  __source__:chosenCasting no file:6:14
  3  __source__:main no file:8:12
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### A value of a function type is printed as the name of its type

```brix
(main
  (let gen (call numbers 3))
  (print gen)
  (print (str "made " gen))
  (let words (call texts))
  (print words)
  (let adder (of Mapper Int Int) (=> (value) (+ value 1)))
  (print adder)
  (let getter (of Getter String) (=> () "x"))
  (print getter)
  (print (str "held " getter))
)

(fn-do + numbers _
  (param a Int)
  (yield a)
)

(fn-do + texts (of Generator String)
  (yield "t")
)
```

```output
[Function Generator'I64]
made [Function Generator'I64]
[Function Generator'String]
[Function Mapper'I64'I64]
[Function Getter'String]
held [Function Getter'String]
```

### Nested functions in the values of two lazy properties read the object they were made for.

```brix
(main
  (let t (call Thing))
  (let f (. t get-a))
  (let g (. t get-b))
  (print (call f))
  (print (call g))
  (= (. t a) 3)
  (print (+ (call f) (call g)))
  (let h (=> ((: n Int)) (* n 2)))
  (print (call h 4))
)

(type + Thing
  (prop + a _ (= 5))
  (prop + b _ (= 7))
  (blank)
  (prop + get-a (of Getter Int) (:= (=> () a)))
  (prop + get-b (of Getter Int) (:= (=> () (+ b a))))
)
```

```output
5
12
13
8
```

### A nested function in a lazy property's value, inherited by a subclass.

```brix
(main
  (let s (call Sub))
  (let f (. s get-a))
  (print (call f))
  (= (. s a) 9)
  (print (call f))
)

(type + Base
  (prop + a _ (= 1))
  (blank)
  (prop + get-a (of Getter Int) (:= (=> () (* a 10))))
)

(type + Sub (is Base))
```

```output
10
90
```

### A nested function in a lazy property's value of a generic class.

```brix
(main
  (let b (call (of Box Int) 4))
  (let r (. b reader))
  (print (call r))
  (let s (call (of Box String) "x"))
  (let q (. s reader))
  (print (call q))
)

(type + (of Box T)
  (prop + value T)
  (blank)
  (prop + reader (of Getter T) (:= (=> () value)))
  (fn-do + This _
    (param value T)
  )
)
```

```output
4
x
```

### A lazy property whose value holds a closure reads its fields by their bare names.

```brix
(main
  (let t (call Thing))
  (print (. t total))
  (print (. t total))
  (let u (call Thing))
  (= (. u n) 10)
  (print (. u total))
)

(type + Thing
  (prop + k _ (= 3))
  (prop + n _ (= 4))
  (blank)
  (prop + total Int (:= (+ n (call apply (=> () k)))))
)

(fn-do + apply Int
  (param f (of Getter Int))
  (return (call f))
)
```

```output
7
7
13
```

### Closures made in a loop read the names match patterns bind in the turn they are made in.

```brix
(main
  (let items (list (call (. Item num) 1) (call (. Item word) "ab") (call (. Item pair) (tuple 3 "c"))))
  (let fs (call (of List Doer)))
  (let total 0)
  (for (in item items)
    (match item
      (case (call .num n)
        (+= fs
          (=> () :do
            (block
              (+= total n)
              (print (str "num " n))
            )
          )
        )
      )
      (case (call .word w)
        (+= fs
          (=> () :do
            (block
              (+= total (. w length))
              (print (str "word " w))
            )
          )
        )
      )
      (case (call .pair a b)
        (+= fs
          (=> () :do
            (block
              (+= total a)
              (print (str "pair " a " " b))
            )
          )
        )
      )
    )
  )
  (for (in f fs)
    (call f)
  )
  (print total)
)

(type + Item (is Union)
  (variant num Int)
  (variant word String)
  (variant pair (tuple Int String))
)
```

```output
num 1
word ab
pair 3 c
6
```

### A closure picks a function-typed overload over one whose parameter is a bare type variable.

```brix
(main
  (print (call pick (=> () (+ 1 1))))
  (print (call pick 3))
  (print (call pick "s"))
)

(fn-expr + (of pick T) _
  (param value T)
  "eager"
)

(fn-expr + (of pick T) _
  (param value (of Getter T))
  (str "lazy " (call value))
)
```

```output
lazy 2
eager
eager
```

### A lazy expectation runs its closure and catches the error it raises.

```brix
(main
  (let t (call Checks))
  (call (. t expect) "infallible" 2 (=> () (+ 1 1)))
  (call (. t expect) "fallible" 2 (=> () (call half 4)))
  (call (. t expect) "failing" 2 (=> () (call half 3)))
  (call (. t expect) "eager" 2 2)
  (print (str (. t passed) " passed, " (. t failed) " failed, " (. t caught) " caught"))
)

(type + Checks
  (prop + passed _ (= 0))
  (prop + failed _ (= 0))
  (prop + caught _ (= 0))
  (fn-do + (of expect Expecting Actual) _
    (param label String)
    (param expected Expecting)
    (param actual Actual)
    (print (str "Assertion " label " is eager"))
    (+= passed 1)
    (blank)
    (return this)
  )
  (fn-do + (of expect Expecting Actual) _
    (param label String)
    (param expected Expecting)
    (param actual (of Getter Actual))
    (let fn actual)
    (let actual-result (catch? (as (call fn) Expecting)))
    (if actual-result
      (if (== actual-result expected)
        (+= passed 1)
        (else
          (+= failed 1)
        )
      )
      (else
        (print (str "Assertion " label " caught"))
        (+= caught 1)
      )
    )
    (blank)
    (return this)
  )
)

(type + OddNumber (is Error))

(fn-expr + half Int (?! OddNumber)
  (param value Int)
  (before
    (throw (call OddNumber "odd") (if (!= (% value 2) 0)))
  )
  (/ value 2)
)
```

```output
Assertion failing caught
Assertion eager is eager
3 passed, 0 failed, 1 caught
```

### A nested function in a method of a generic class names the class's type parameter.

```brix
(main
  (let b (call (of Holder Int) 5))
  (print (. (call (. b listed)) length))
  (print (call (. b cast)))
)

(type + (of Holder T)
  (prop + value T)
  (fn-do + This _
    (param value _)
  )
  (fn-do + listed (list T)
    (let g (=> () (call (of List T) (list value value))))
    (return (call g))
  )
  (fn-do + cast T
    (let f (=> () (as value T)))
    (return (call f))
  )
)
```

```output
2
5
```

### A lambda whose body is optional where its function type answers a value unwraps the body's value with `!`.

```brix
(main
  (print (call ordered 3 1))
  (print (call compared 1 3 (:using (=> (a b) (!! (if-expr (< a b) .lesser (only-if .greater (if (> a b)))))))))
)

(fn-do _ ordered Comparison
  (param left Int)
  (param right Int)
  (let maybe (? Comparison) .greater)
  (return (call compared left right (:using (=> (a b) (!! maybe)))))
)

(fn-expr _ compared Comparison
  (param left Int)
  (param right Int)
  (param using (of Comparer Int))
  (call using left right)
)
```

```output
greater
lesser
```

### The unwrap of a lambda's optional result panics where the body holds no value.

```brix crash
(main
  (print (call compared 1 1 (:using (=> (a b) (!! (only-if .greater (if (> a b))))))))
)

(fn-expr _ compared Comparison
  (param left Int)
  (param right Int)
  (param using (of Comparer Int))
  (call using left right)
)
```

```stderr
Unhandled panic: Optional'Comparison is null
<no file:3>

Task 1

Stack trace:
  0  __source__:anon no file:3:47
  1  __source__:anonCasting no file:3:29
  2  __source__:compared no file:10:3
  3  __source__:main no file:3:10
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### A constructor argument named as an optional property of a function type takes a lambda and a method.

```brix
(main
  (let counter (call Counter))
  (print (call (. (call Holder (. counter next)) run)))
  (print (call (. (call Holder (=> () 7)) run)))
)

(type + Maker (is (fn-type Int)))

(type + Counter
  (prop + count _ (= 0))
  (fn-do + next Int
    (+= count 1)
    (return (* count 10))
  )
)

(type + Holder
  (prop + maker (? Maker))
  (fn-do + This _
    (param maker _)
  )
  (fn-do + run Int
    (return (if-expr maker (call maker) 0))
  )
)
```

```output
10
7
```

### A closure that raises an error its function type declares binds the type's return to what it gives.

```brix
(main
  (let got (call first (=> (n) (call half n))))
  (print (+ got 1))
  (let listed (call gather (=> (n) (call half n))))
  (print (+ ([] listed 0) 1))
)

(type + Odd (is Error))

(fn-do + half Int (?! Odd)
  (param n Int)
  (throw (call Odd "odd") (if (== (% n 2) 1)))
  (return (/ n 2))
)

(fn-do + (of first Item) Item (?! Odd)
  (param decode (fn-type Item (?! Odd) (param n Int)))
  (return (call decode 4))
)

(fn-do + (of gather Item) (list Item) (?! Odd)
  (param decode (fn-type Item (?! Odd) (param n Int)))
  (let found (list Item))
  (+= found (call decode 6))
  (return found)
)
```

```output
3
4
```

### An index assignment whose setter answers the new value stores it in a captured local or argument, from a closure or task that captures it or from the function that declares it.

```brix
(import /core/bitmap)
(blank)

(main
  (let bits (call (of Bitmap U8)))
  (let read (=> () ([] bits 1)))
  (let change
    (=> () :do
      (block
        (= ([] bits 2) true)
      )
    )
  )
  (call change)
  (= ([] bits 1) true)
  (print (str (call read) " " ([] bits 2)))
  (call flip (call (of Bitmap U8)))
)

(fn-do + flip _
  (param bits (of Bitmap U8))
  (let run
    (do-new
      (= ([] bits 3) true)
      (print ([] bits 3))
    )
  )
  (call (. run wait))
)
```

```output
true true
true
```
