# Bindings

```brix
(main
  (print bar)
  (print (call bar 1))
)

(fn-expr + bar _
  (param a Int)
  a
)
```

```output
[Binding bar]
1
```

```brix
(main
  (let fn bar)
  (print (call fn 1 2))
  (blank)
  (let fn2 (call bar 3 ...))
  (print (call fn2 4))
)

(fn-expr + bar _
  (param a Int)
  (param b Int)
  (+ a b)
)
```

```output
3
7
```

```brix
(main
  (let fn (call bar 1 ...))
  (print (call fn ...))
  (blank)
  (let fn2 (call fn 2 ...))
  (print (call fn2 3))
  (blank)
  (let gn bar)
  (let gn2 (call bar 3 ...))
  (let gn3 (call gn2 4 ...))
  (print (call gn3 5))
)

(fn-expr + bar _
  (param a Int)
  (param b Int)
  (param c Int)
  (+ a b c)
)
```

```output
[Binding bar]
6
12
```

```brix
(main
  (let fn (call bar 1 ...))
  (print (call fn 2))
)

(fn-expr + bar _
  (param a Int)
  (param b _)
  (+ a b)
)
```

```output
3
```

```brix
(main
  (let fn bar)
  (print (call fn 1))
  (print (call fn 1.5))
)

(fn-expr + bar _
  (param a Int)
  (+ a 2)
)

(fn-expr + bar _
  (param a Float)
  (+ a 1)
)
```

```output
3
2.5
```

```brix
(main
  (let fn bar)
  (print (call fn 1))
  (print (call fn 1 2))
)

(fn-expr + bar _
  (param g _)
  g
)

(fn-expr + bar _
  (param a Int)
  (param b Int)
  (+ a b)
)
```

```output
1
3
```

```brix
(main
  (let fn bar)
  (let fn2 (call fn 1 2 ...))
  (print fn2)
  (print (call fn2 3))
)

(fn-expr + bar _
  (param g _)
  g
)

(fn-expr + bar _
  (param a _)
  (param b _)
  (param c _)
  (+ a b c)
)
```

```output
[Binding bar]
6
```

```brix
(main
  (let fn bar)
  (blank)
  (let fn2 (call fn 1 ...))
  (print fn2)
  (blank)
  (let fn3 (call fn2 2 ...))
  (print fn3)
  (blank)
  (print (call fn3 3))
)

(fn-expr + bar _
  (param a _)
  (param b _)
  (param c _)
  (+ a b c)
)
```

```output
[Binding bar]
[Binding bar]
6
```

```brix
(main
  (let fn bar)
  (print (call fn 1))
  (print (call fn 1 2))
)

(fn-expr + bar _
  (param a Int)
  (param b Int)
  (+ a b)
)

(fn-expr + bar _
  (param g _)
  g
)
```

```output
1
3
```

## With type parameters

```brix
(main
  (let fn (call (of bar Float) 1 ...))
  (print (call fn 2))
)

(fn-expr + (of bar T) _
  (param a T)
  (param b T)
  (+ a b)
)
```

```output
3
```

```brix
(main
  (let fn (call bar 1.0 ...))
  (print (call fn 2))
)

(fn-expr + (of bar T) _
  (param a T)
  (param b T)
  (+ a b)
)
```

```output
3
```

## With exceptions

```brix
(main
  (do-new
    (let fn bar)
    (print (call fn 1))
    (on-failure failure
      (print failure)
    )
  )
)

(fn-do + bar _
  (param a _)
  (if (> a 0)
    (throw (call Panic "naughty"))
    (else
      (return a)
    )
  )
)
```

```output
naughty
```

## Cast to Function types

```brix
(main
  (let fn (call bar 1 ...))
  (let fn-bar (as fn Bar))
  (print (call fn-bar 2))
)

(type + Bar (is (fn-type Int (param aa Float))))

(fn-expr + bar _
  (param a _)
  (param b _)
  (+ a b)
)

(fn-expr + bar _
  (param a _)
  (param b _)
  (param c _)
  (+ a b c)
)
```

```output
3
```

```brix
(main
  (let thing (call Thing))
  (print (call (. thing act)))
)

(type + Bar (is (fn-type Int (param aa Float))))

(type + Thing
  (fn-expr + bar _
    (param a Int)
    (param b Int)
    (+ a b)
  )
  (fn-expr + bar _
    (param a Int)
    (param b Int)
    (param c Int)
    (+ a b c)
  )
  (fn-expr + act _
    (call fn-bar 2)
    (where
      (lazy fn (call bar 1 ...))
      (lazy fn-bar (as fn Bar))
    )
  )
)
```

```output
3
```

```brix
(main
  (let thing (call Thing))
  (call (. thing act))
)

(type + Bar (is (fn-type Int (param aa Float))))

(type + Thing
  (fn-expr + bar _
    (param a _)
    (param b _)
    (param c _)
    (+ a b c)
  )
  (fn-do + act _
    (let fn (call bar 1 2 ...))
    (let fn-bar (as fn Bar))
    (print (call fn-bar 2))
  )
)
```

```output
5
```

```brix
(main
  (let fn (call bar 1 ...))
  (let fn-bar (as fn Bar))
  (print (call fn-bar 2 3))
)

(type + Bar (is (fn-type Int (param aa Float) (param bb Float))))

(fn-expr + bar _
  (param a Int)
  (param b Int)
  (+ a b)
)

(fn-expr + bar _
  (param a _)
  (param b _)
  (param c _)
  (+ a b c)
)
```

```output
6
```

```brix crash
(main
  (let fn (call bar 1 ...))
  (print (as fn Bar))
)

(type + Foo)

(type + Bar (is (fn-type Int (param aa Foo))))

(fn-expr + bar _
  (param a Int)
  (param b Int)
  (+ a b)
)
```

```output
Exception: function bar cannot be cast to Bar; there is no conversion between them
<no file:4>
      (print (as fn Bar))
             ^^^^^^^^^^^
```

```brix crash
(main
  (let fn (call bar 1 ...))
  (print (as fn Bar))
)

(type + Foo)

(type + Bar (is (fn-type Int (param aa Foo))))

(fn-expr + bar _
  (param a _)
  (param b Int)
  (+ a b)
)
```

```output
Exception: function bar cannot be cast to Bar; there is no conversion between them
<no file:4>
      (print (as fn Bar))
             ^^^^^^^^^^^
```

## Cast to CFunction types

```brix
(import /JaCore/tests.h)
(blank)

(main
  (print (call C:ctestCallback (as foo TestCallback)))
)

(type + TestCallback (is (c-fn-type I32 (param n I32))))

(fn-expr + foo _
  (param n _)
  (* n 2)
)
```

```output
84
```

```brix crash
(import /JaCore/tests.h)
(blank)

(main
  (let thing (call Thing))
  (print (call C:ctestCallback (as (. thing foo) TestCallback)))
)

(type + TestCallback (is (c-fn-type I32 (param n I32))))

(type + Thing
  (fn-expr + foo _
    (param n _)
    (* n 2)
  )
)
```

```output
Exception: Illegal to cast this function to CFunction
<no file:7>
      (print (call C:ctestCallback (as (. thing foo) TestCallback)))
                                   ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A function made a C function type runs when the native code that calls it keeps its own value in the task context register.

```brix
(import /JaCore/tests.h)
(blank)

(main
  (print (call C:ctestClobberedCallback (as measure TextCallback)))
)

(type + TextCallback (is (c-fn-type I32 (param text (of CPointer Char8)))))

(fn-do + measure I32
  (param text (of CPointer Char8))
  (let copied (as text String))
  (print copied)
  (return (as (. copied length) I32))
)
```

```output
clobbered
9
```

### A panic in a function made a C function type is reported when the native code that calls it keeps its own value in the task context register.

```brix crash
(import /JaCore/tests.h)
(blank)

(main
  (print (call C:ctestClobberedCallback (as explode TextCallback)))
)

(type + TextCallback (is (c-fn-type I32 (param text (of CPointer Char8)))))

(fn-do + explode I32
  (param text (of CPointer Char8))
  (throw (call Panic (str "boom " (as text String))))
)
```

```stderr
Unhandled panic: boom clobbered
<no file:13>

Task 1

Stack trace:
  0  __source__:explode no file:13:3
  1  __source__:explodeCasting no file:9:1
```

## Bindings from Function objects

```brix
(main
  (let fn (call bar 1 ...))
  (let fn-bar (as fn Bar))
  (let fn-bar2 (call fn-bar 2 ...))
  (print fn-bar2)
  (print (call fn-bar2 3))
)

(type + Bar (is (fn-type Int (param aa Float) (param bb Float))))

(fn-expr + bar _
  (param a _)
  (param b _)
  (param c _)
  (+ a b c)
)
```

```output
[Binding ]
6
```

## Bindings from CFunctions

```brix
(import /c/unistd.h)
(blank)

(main
  (let fn (call C:strtoull "3" null ...))
  (print fn)
  (print (call fn 10))
)
```

```output
[Binding strtoull]
3
```

## Bindings from Closure

```brix
(main
  (let c 10)
  (let foo (=> (a b) (+ a b c)))
  (print (call foo 1))
)
```

```output
[Binding foo]
```

```brix
(main
  (let c 10)
  (let foo (=> (a b) (+ a b c)))
  (let fn (call foo 1))
  (print (call fn 2))
)
```

```output
13
```

```brix
(main
  (let d 10)
  (let foo (=> (a b c) (+ a b c d)))
  (let fn (as (call foo 1) Foo))
  (print (call (call fn 2) 3))
)

(type + Foo (is (fn-type Int (param a Int) (param b Int))))
```

```output
16
```

## Bindings from Constructor

```brix
(main
  (print (call Foo 1 ...))
)

(type + Foo
  (fn-do + This _
    (param a _)
    (param b _)
  )
)
```

```output
[Binding Foo]
```

```brix
(main
  (let fn (call Foo 1 ...))
  (print (call fn 2))
)

(type + Foo
  (fn-do + This _
    (param a _)
    (param b _)
  )
)
```

```output
[Object __source__:Foo]
```

```brix
(main
  (let fn (call Foo 1 ...))
  (let fn-cast (as fn Foopy))
  (print (call fn-cast 2))
)

(type + Foopy (is (fn-type Foo (param a Int))))

(type + Foo
  (fn-do + This _
    (param a _)
    (param b _)
  )
)
```

```output
[Object __source__:Foo]
```

## Bindings from methods

```brix
(main
  (let foo (call Foo))
  (print (. foo bar))
)

(type + Foo
  (fn-expr + bar _
    (param a Int)
    (param b Int)
    (+ a b)
  )
)
```

```output
[Binding bar]
```

```brix
(main
  (let foo (call Foo))
  (let fn (. foo bar))
  (print fn)
  (print (call fn))
)

(type + Foo
  (prop + c _ (= 10))
  (fn-expr + bar _
    c
  )
)
```

```output
[Binding bar]
10
```

```brix
(main
  (let foo (call Foo))
  (let fn (. foo bar))
  (print fn)
  (print (call fn 1 2))
)

(type + Foo
  (prop + c _ (= 10))
  (fn-expr + bar _
    (param a _)
    (param b _)
    (+ a b c)
  )
)
```

```output
[Binding bar]
13
```

```brix
(main
  (let foo (call Foo))
  (let fn (call (. foo bar) 1 ...))
  (print fn)
  (print (call fn 2))
)

(type + Foo
  (prop + c _ (= 10))
  (fn-expr + bar _
    (param a _)
    (param b _)
    (+ a b c)
  )
)
```

```output
[Binding bar]
13
```

```brix
(main
  (let foo (call Foo))
  (let fn (call (. foo bar) 1 ...))
  (let fn-cast (as fn Foopy))
  (print (call fn-cast 2))
)

(type + Foopy (is (fn-type Int (param a Int))))

(type + Foo
  (prop + c _ (= 10))
  (fn-expr + bar _
    (param a _)
    (param b _)
    (+ a b c)
  )
)
```

```output
13
```

```brix
(main
  (let thing (call Thing))
  (let b (call (. thing foo)))
  (print (call b 10 1))
)

(fn-expr + bar _
  (param b _)
  (param c _)
  (+ 100 b c)
)

(type + Thing
  (prop + a _ (= 42))
  (fn-expr + foo _
    bar
  )
  (fn-expr + bar _
    (param b _)
    (+ a b)
  )
)
```

```output
111
```

```brix crash
(main
  (let thing (call Thing))
  (let b (call (. thing foo)))
  (print (call b 10 1 3))
)

(fn-expr + bar _
  (param b _)
  (param c _)
  (+ 100 b c)
)

(type + Thing
  (prop + a _ (= 42))
  (fn-expr + foo _
    bar
  )
  (fn-expr + bar _
    (param b _)
    (+ a b)
  )
)
```

```output
Exception: Illegal arguments to binding
<no file:5>
      (print (call b 10 1 3))
             ^^^^^^^^^^^^^^^
```

## Bindings from interface methods

```brix
(main
  (let foo (as (call Foo) Proto))
  (print (. foo bar))
)

(type + Proto (is Interface)
  (fn-expr + bar _
    (param a Int)
    (param b Float)
  )
)

(type + Foo
  (implements Proto)
  (fn-expr + bar _
    (param a Int)
    (param b Int)
    (+ a b)
  )
)
```

```output
[Binding bar]
```

```brix
(main
  (let foo (as (call Foo) Proto))
  (let fn (. foo bar))
  (print fn)
  (print (call fn))
)

(type + Proto (is Interface)
  (fn-expr + bar Int)
)

(type + Foo
  (implements Proto)
  (prop + c _ (= 10))
  (fn-expr + bar _
    c
  )
)
```

```output
[Binding bar]
10
```

```brix
(main
  (let foo (as (call Foo) Proto))
  (let fn (. foo bar))
  (print fn)
  (print (call fn 1 2))
)

(type + Proto (is Interface)
  (fn-expr + bar Int
    (param a Int)
    (param b Int)
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
```

```output
[Binding bar]
13
```

```brix
(main
  (let foo (as (call Foo) Proto))
  (let fn (call (. foo bar) 1 ...))
  (let fn-cast (as fn Foopy))
  (print (call fn-cast 2))
)

(type + Foopy (is (fn-type Int (param a Int))))

(type + Proto (is Interface)
  (fn-expr + bar Int
    (param a Int)
    (param b Int)
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
```

```output
13
```

```brix
(main
  (let f (call foo 2 ...))
  (print (call (of f Int) 3))
)

(fn-expr + (of foo T1 T2) _
  (param n _)
  (param m T2)
  (* n m)
)
```

```output
6
```

```brix crash
(main
  (print (call foo 1))
)

(type + (of Foo T))

(fn-expr + (of foo T1) _
  (param n (of Foo T1))
  (* n m)
)
```

```output
Exception: No function "foo" accepts (I64); it is declared as "foo(n Foo'T1)"
<no file:3>
      (print (call foo 1))
             ^^^^^^^^^^^^
```

```brix
(main
  (let foo (call Foo))
  (call (call (call (. foo bar) ...) 1 ...) 2)
)

(type + Foo
  (fn-do + bar _
    (param n _)
    (param v _)
    (print this)
    (print (+ n v))
  )
)
```

```output
[Object __source__:Foo]
3
```

### A nested function called with too few arguments gives a binding that holds it and them.

```brix
(main
  (let base 100)
  (let add (=> (a b c) (+ a b c base)))
  (let two (call add 1 2))
  (print (call two 3))
  (= base 200)
  (print (call two 4))
  (print (call (call add 1 2) 3))
  (print (call (call add 1) 2 3))
  (print two)
)
```

```output
106
207
206
206
[Binding add]
```

### A binding of a nested function that holds text, called after many allocations.

```brix
(main
  (let greet (=> (greeting name mark) (str greeting " " name mark)))
  (let hello (call greet "hello" "ann"))
  (let junk (list 0))
  (let i 0)
  (while (< i 20000)
    (= junk (list i (+ i 1)))
    (+= i 1)
  )
  (print (call hello "!"))
  (print (call (call greet "bye") "bob" "?"))
  (print (. junk length))
)
```

```output
hello ann!
bye bob?
2
```

### A binding of a nested function that reads a property of the receiver it captured.

```brix
(main
  (let thing (call Thing))
  (let step (call (. thing stepper)))
  (let bound (call step 1))
  (print (call bound 2))
  (= (. thing size) 50)
  (print (call bound 3))
)

(type + Thing
  (prop + size _ (= 4))
  (fn-expr + stepper _
    (=> (a b) (+ a b size))
  )
)
```

```output
7
54
```

### A binding of a nested function as a value of a function type.

```brix
(main
  (let base 10)
  (let add (=> (a b) (+ a b base)))
  (let bound Step (call add 5))
  (print (call bound 1))
  (= base 20)
  (print (call run bound))
  (print (call run (call add 7)))
)

(type + Step (is (fn-type Int (param n Int))))

(fn-expr + run _
  (param step Step)
  (call step 100)
)
```

```output
16
125
127
```

### A C function read as a value and given its arguments in steps.

```brix
(import /JaCore/tests.h)
(blank)

(main
  (let a (call C:ctest5 2 ...))
  (print (call a 3))
  (let b C:ctest5)
  (print (call b 4 5))
  (let c (call b 1 ...))
  (print (call c 2))
  (print a)
  (print b)
  (print c)
)
```

```output
5
9
3
[Binding ctest5]
[Binding ctest5]
[Binding ctest5]
```

### A binding of a C function copied, and kept in an optional property of a function type.

```brix
(import /JaCore/tests.h)
(blank)

(main
  (let a (call C:ctest5 1 ...))
  (let b a)
  (print (call b 6))
  (let thing (call Thing))
  (= (. thing add) (call C:ctest5 10 ...))
  (print (call (. thing total) 5))
)

(type + Add (is (fn-type I32 (param n I32))))

(type + Thing
  (prop + add (? Add))
  (fn-do + total I32
    (param n I32)
    (let held add)
    (if held
      (return (call held n))
    )
    (return 0)
  )
)
```

```output
7
15
```

### A binding of a C function captured by closures.

```brix
(import /JaCore/tests.h)
(blank)

(main
  (let f (call C:ctest5 1 ...))
  (let g (=> (n) (call f n)))
  (print (call g 4))
  (let h Step (=> (n) (+ (call f n) 1)))
  (print (call h 4))
)

(type + Step (is (fn-type I32 (param n I32))))
```

```output
5
6
```

### A function given to C as a callback allocates each time C calls it.

```brix
(import /JaCore/tests.h)
(blank)

(main
  (let sum 0)
  (let i 0)
  (while (< i 50000)
    (+= sum (call C:ctestCallback (as step TestCallback)))
    (+= i 1)
  )
  (print sum)
)

(type + TestCallback (is (c-fn-type I32 (param n I32))))

(fn-do + step _
  (param n I32)
  (let made (call Cell))
  (= (. made held) n)
  (return (- (. made held) 41))
)

(type + Cell
  (prop + held I32 (= 0))
)
```

```output
50000
```

### An object a callback makes after a C call of its own waited out the start of a mark survives the collection.

```brix
(import /JaCore/tests.h)
(import /unistd.h)
(import /BxGC/BxGC.h)
(import /BxGC/BxGCTest.h)
(blank)
(prop + stage _ (= 0))
(prop + outcome _ (= ""))
(blank)

(main
  (call C:brixCollect)
  (let results (of SyncChannel String))
  (let pool (call Worker 1))
  (do-new (worker: pool)
    (call C:ctestCallback (as nested TestCallback))
    (<- results outcome)
  )
  (while (!= stage 1)
    (call C:usleep 100)
  )
  (call C:brixGCStartConcurrentMark)
  (= stage 2)
  (while (!= stage 3)
    (call C:usleep 100)
  )
  (call C:brixCollect)
  (= stage 4)
  (print (<- results))
  (call (. pool stop))
)

(type + TestCallback (is (c-fn-type I32 (param n I32))))

(fn-do + nested I32
  (param n I32)
  (let first (call Cell 1))
  (= stage 1)
  (while (!= stage 2)
    (call C:usleep 2000)
  )
  (let made (call Cell 2))
  (= stage 3)
  (while (!= stage 4)
    (call C:usleep 100)
  )
  (= outcome (if-expr (and (call C:brixGCVerifyObject (as made Pointer)) (== (. made held) 2) (== (. first held) 1)) "alive" "lost"))
  (return n)
)

(type + Cell
  (prop + held _ (= 0))
  (fn-do + This _
    (param value Int)
    (= held value)
  )
)
```

```output
alive
```

### Functions that take a number of another type than a function type gives are made its values by a cast and where a function takes one.

```brix
(main
  (let f (as half Whole))
  (print (call f 9))
  (print (call apply half 7))
  (print (call apply twice 7))
)

(type + Whole (is (fn-type Int (param n I8))))

(fn-expr + half I32
  (param n Float)
  (as (/ n 2) I32)
)

(fn-expr + twice _
  (param n I16)
  (* n 2)
)

(fn-expr + apply _
  (param fn Whole)
  (param n I8)
  (+ (call fn n) 1)
)
```

```output
4
4
15
```

### A method bound with no argument is made a value of two function types that give and take numbers of other types.

```brix
(main
  (let thing (call Thing 4))
  (let f (as (call (. thing scaled) ...) Scale))
  (print (call f 2.5))
  (let g (as (call (. thing scaled) ...) Wide))
  (print (call g 3))
)

(type + Scale (is (fn-type Int (param n Float))))

(type + Wide (is (fn-type Float (param n I16))))

(type + Thing
  (prop + size _ (= 0))
  (fn-do + This _
    (param size _)
  )
  (fn-expr + scaled _
    (param by Int)
    (* size by)
  )
)
```

```output
8
12
```

### Functions that take and answer other numbers than a function type are added to a list of it and returned as its values.

```brix
(main
  (let fns (list Whole))
  (+= fns half)
  (+= fns (=> (n) (* n 3)))
  (+= fns wide)
  (for (in fn fns)
    (print (call fn 9))
  )
  (print (call (call pick true) 5))
  (print (call (call pick false) 5))
)

(type + Whole (is (fn-type Int (param n I16))))

(fn-expr + half I32
  (param n Float)
  (as (/ n 2) I32)
)

(fn-expr + wide I64
  (param n I32)
  (* n 1000)
)

(fn-do + pick Whole
  (param first Bool)
  (if first
    (return half)
  )
  (return wide)
)
```

```output
4
27
9000
2
5000
```

### A method, and one bound with an argument of another type than it takes, are given to a function that takes a function type, and kept.

```brix
(main
  (let thing (call Thing 4))
  (print (call run (. thing scaled)))
  (print (call run (call (. thing grown) 2 ...)))
  (let kept (call keep (. thing scaled)))
  (= (. thing size) 10)
  (print (call kept 1))
)

(type + Scale (is (fn-type Int (param n I16))))

(type + Thing
  (prop + size _ (= 0))
  (fn-do + This _
    (param size _)
  )
  (fn-expr + scaled _
    (param by Int)
    (* size by)
  )
  (fn-expr + grown _
    (param step I8)
    (param by Int)
    (+ size (* step by))
  )
)

(fn-expr + run _
  (param fn Scale)
  (call fn 2)
)

(fn-expr + keep Scale
  (param fn Scale)
  fn
)
```

```output
8
8
10
```

### A function's answer is made an optional of another number by the function type it is cast to.

```brix
(main
  (let f (as opt Maybe))
  (print (?? (call f 3) -1))
  (print (?? (call f 0) -1))
)

(type + Maybe (is (fn-type (? Float) (param n I8))))

(fn-expr + opt Int
  (param n Int)
  (* n 2)
)
```

```output
6
0
```

### A function's answer is made a number from a truth, and a truth from a number, by the function type it is cast to.

```brix
(main
  (let g (as flag Num))
  (print (call g 3.9))
  (print (call g 0.2))
  (let h (as num Truth))
  (print (call h 3))
  (print (call h 4))
)

(type + Num (is (fn-type Int (param n Float))))

(type + Truth (is (fn-type Bool (param n Int))))

(fn-expr + flag Bool
  (param n Int)
  (> n 2)
)

(fn-expr + num Int
  (param n Int)
  (- n 3)
)
```

```output
1
0
false
true
```

### A function that answers is cast to a function type that answers nothing and gives it a number of another type.

```brix
(main
  (let q (as noisy Quiet))
  (call q 4.9)
  (print "done")
)

(type + Quiet (is (fn-type Void (param n Float))))

(fn-do + noisy Int
  (param n Int)
  (print (str "got " n))
  (return (* n 2))
)
```

```output
got 4
done
```

### Functions that answer an object, a value of an interface and a new object are given where a function type answers the interface.

```brix
(main
  (print (call named shown (call Thing "two")))
  (print (call named same (call Thing "three")))
  (print (call named lower (call Thing "four")))
)

(type + Shown (is (fn-type Label (param thing Thing))))

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

(fn-expr + shown _
  (param thing Thing)
  thing
)

(fn-expr + same _
  (param label Label)
  label
)

(fn-expr + lower _
  (param thing Thing)
  (call Thing (str "lower " (. thing name)))
)

(fn-expr + named _
  (param fn Shown)
  (param thing Thing)
  (call (. (call fn thing) label))
)
```

```output
thing two
thing three
thing lower four
```

### Functions that take a value of an interface and an object of a base class are cast to function types that give an object of the class and of one derived from it.

```brix
(main
  (let f (as of-label Taker))
  (print (call f (call Thing "one")))
  (let g (as of-base Lower))
  (print (call g (call Derived)))
)

(type + Taker (is (fn-type String (param thing Thing))))

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

(fn-expr + of-label _
  (param label Label)
  (call (. label label))
)

(fn-expr + of-base _
  (param base Base)
  (call (. base kind))
)
```

```output
thing one
derived
```

### A function that takes an object of a base class is given where a function type gives one of a class derived from it.

```brix
(main
  (print (call run of-base))
)

(type + Lower (is (fn-type String (param derived Derived))))

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

(fn-expr + of-base _
  (param base Base)
  (call (. base kind))
)

(fn-expr + run _
  (param fn Lower)
  (call fn (call Derived))
)
```

```output
derived
```

### A function that takes another number than a function type gives panics, called through a value of the type.

```brix crash
(main
  (print (call run pick))
)

(type + Pick (is (fn-type Int (param n I32))))

(fn-do + pick _
  (param n Int)
  (let items (list 1 2 3))
  (return ([] items n))
)

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
  1  __source__:pick no file:11:11
  2  __source__:pickCasting no file:3:20
  3  __source__:run no file:16:18
  4  __source__:main no file:3:10
  5  __source__:init.__source__ no file
  6  __source__:brix.main no file
```

### A method bound with an argument of another type than it takes panics, called as a value of a function type inside a function.

```brix crash
(main
  (let thing (call Thing))
  (call go thing)
)

(type + Pick (is (fn-type Float (param n Float))))

(type + Thing
  (prop + items _ (= (list 1 2 3)))
  (fn-expr + pick _
    (param skip I8)
    (param n Int)
    ([] items (+ n skip))
  )
)

(fn-do + go _
  (param thing Thing)
  (let held (call (. thing pick) 1 ...))
  (let fn (as held Pick))
  (print (call fn 1.2))
  (print (call fn 9.9))
)
```

```stderr
Unhandled panic: List index 10 out of range
<core/list.bx>

Task 1

Stack trace:
  0  core/list:List:absent core/list.bx
  1  __source__:Thing:pick no file:14:5
  2  __source__:pickCasting no file:21:11
  3  __source__:go no file:23:10
  4  __source__:main no file:4:3
  5  __source__:init.__source__ no file
  6  __source__:brix.main no file
```

### A function and a closure that answer an object are cast to a function type that answers a number the object converts to, and to one that gives it an object made from a number.

```brix
(main
  (let f (as make Maker))
  (print (call f 4))
  (let b (=> ((: n Int)) (call Thing)))
  (let g (as b Maker))
  (print (call g 1))
  (let h (as make Wrap))
  (print (. (call h 3) n))
)

(type + Maker (is (fn-type Int (param n Int))))

(type + Wrap (is (fn-type Thing (param thing Thing))))

(fn-do + make _
  (param n Int)
  (let thing (call Thing))
  (= (. thing n) n)
  (return thing)
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
5
1
31
```

### A function that answers a number is cast to a function type that answers nothing and to one that answers text.

```brix
(main
  (let q (as noisy Quiet))
  (call q 4.9)
  (let t (as noisy Text))
  (print (call t 2.2))
)

(type + Quiet (is (fn-type Void (param n Float))))

(type + Text (is (fn-type String (param n Float))))

(fn-do + noisy Int
  (param n Int)
  (print (str "got " n))
  (return (* n 2))
)
```

```output
got 4
got 2
4
```

### A function's answer is converted by a function that panics, called through a value of a function type: the first call's answer is printed and the second ends the program.

```brix crash
(main
  (print (. (call run make) count))
)

(type + Maker (is (fn-type CastTally (param n Int))))

(fn-do + make _
  (param n Int)
  (let slot (call Slot))
  (= (. slot n) n)
  (return slot)
)

(fn-do + run _
  (param fn Maker)
  (let first (call fn 1))
  (print (. first count))
  (return (call fn 8))
)

(type + CastTally
  (prop + count _ (= 0))
)

(type + Slot
  (prop + n _ (= 0))
  (fn-to-do + CastTally
    (let items (list 10 20 30))
    (let tally (call CastTally))
    (= (. tally count) ([] items n))
    (return tally)
  )
)
```

```output
20
```

### A function's answer is converted by a function that panics, called through a value of a function type: the stack trace has the converter and the function made for the value.

```brix crash
(main
  (print (. (call run make) count))
)

(type + Maker (is (fn-type CastLedger (param n Int))))

(fn-do + make _
  (param n Int)
  (let slot (call Slot))
  (= (. slot n) n)
  (return slot)
)

(fn-do + run _
  (param fn Maker)
  (let first (call fn 1))
  (print (. first count))
  (return (call fn 8))
)

(type + CastLedger
  (prop + count _ (= 0))
)

(type + Slot
  (prop + n _ (= 0))
  (fn-to-do + CastLedger
    (let items (list 10 20 30))
    (let tally (call CastLedger))
    (= (. tally count) ([] items n))
    (return tally)
  )
)
```

```stderr
Unhandled panic: List index 8 out of range
<core/list.bx>

Task 1

Stack trace:
  0  core/list:List:absent core/list.bx
  1  __source__:Slot:to CastLedger no file:31:24
  2  __source__:makeCasting no file:3:23
  3  __source__:run no file:19:11
  4  __source__:main no file:3:13
  5  __source__:init.__source__ no file
  6  __source__:brix.main no file
```

### Bound constructor made a function value constructs at each call

```brix
(main
  (let make (call Foo 1 ...))
  (let cast (as make Foopy))
  (let first (call cast 2))
  (print (. first sum))
  (let second (call cast 40))
  (print (. second sum))
  (print (. first sum))
)

(type + Foopy (is (fn-type Foo (param b Int))))

(type + Foo
  (prop + sum Int)
  (fn-do + This _
    (param a Int)
    (param b Int)
    (lazy sum (+ a b))
  )
)
```

```output
3
41
3
```

### Stack trace of a panic in a constructor bound as a function value

```brix crash
(main
  (let make Foopy (call Foo 1 ...))
  (print (. (call make 2) sum))
  (print (. (call make 11) sum))
  (print "never")
)

(type + Foopy (is (fn-type Foo (param b Int))))

(type + Foo
  (prop + sum Int)
  (fn-do + This _
    (param a Int)
    (param b Int)
    (lazy sum (+ a b))
    (throw (call Panic "too many") (if (> b 10)))
  )
)
```

```stderr
Unhandled panic: too many
<no file:17>

Task 1

Stack trace:
  0  __source__:Foo:This no file:17:5
  1  __source__:ThisCasting no file:3:13
  2  __source__:main no file:5:13
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### Binding of a generic function converts what it holds for the instance called

```brix
(main
  (let c (call (of foo F32) 1 2 ...))
  (print (call c 3))
  (let d (call (of foo F32 F64) 2 ...))
  (print (call d 1.5 2))
  (let e (call foo 3 ...))
  (print (call e 2 2.5))
)

(fn-expr + (of foo T1 T2) _
  (param a I32)
  (param b T1)
  (param c T2)
  (* a b c)
)
```

```output
6
6
15
```

### Binding of a generic function called with arguments of two types

```brix
(main
  (let c (call scale 2 3 ...))
  (print (call c 4))
  (print (call c 4.5))
  (let d (call scale 2 ...))
  (print (call d 1 1))
  (print (call d 1.5 2))
)

(fn-expr + (of scale T) Float
  (param a Float)
  (param b Float)
  (param c T)
  (+ (* a b) c)
)
```

```output
10
10.5
3
5
```

### Binding of a generic method converts what it holds

```brix
(main
  (let object (call Adder 10))
  (let c (call (of (. object add) F32) 1 ...))
  (print (call c 2))
  (print (call c 2.5))
)

(type + Adder
  (prop + base Int)
  (fn-do + This _
    (param start Int)
    (lazy base start)
  )
  (fn-expr + (of add T) F32
    (param a F32)
    (param b T)
    (+ base a b)
  )
)
```

```output
13
13.5
```

### Bound constructor that raises made a value of a function type that raises

```brix
(main
  (let make Foopy (call Foo 1 ...))
  (print (. (call make 2) sum))
  (let failed (catch (call make 20)))
  (print (is failed TooBig))
)

(type + TooBig (is Error))

(type + Foopy (is (fn-type Foo (?! TooBig) (param b Int))))

(type + Foo
  (prop + sum Int)
  (fn-do + This _ (?! TooBig)
    (param a Int)
    (param b Int)
    (throw (call TooBig "too big") (if (> b 10)))
    (lazy sum (+ a b))
  )
)
```

```output
3
true
```

### A binding of a generic class's constructor that holds nothing makes the default.

```brix
(main
  (let mk (call Box ...))
  (print (call (of mk Int)))
)

(type + (of Box T)
  (prop + value (? T) (= null))
)
```

```output
[Object __source__:Box'core/number:I64]
```

### A binding of a generic class's own constructor that holds nothing takes its arguments.

```brix
(main
  (let mk (call Box ...))
  (print (. (call (of mk Int) 5) v))
)

(type + (of Box T)
  (prop + v T)
  (fn-do + This _
    (param v T)
    (= (. this v) v)
  )
)
```

```output
5
```

### A binding of a generic class whose own constructor takes only defaulted arguments makes the default.

```brix
(main
  (let mk (call Box ...))
  (let b (call (of mk Int)))
  (print (. b n))
)

(type + (of Box T)
  (prop + n _ (= 0))
  (fn-do + This _
    (param n Int (= 4))
    (print "own")
    (= (. this n) n)
  )
)
```

```output
0
```

### A binding of a generic constructor prints the type's parameters once.

```brix
(main
  (print (call Thing ...))
  (print (call Duo ...))
)

(type + (of Thing T) (is (tuple (: a T) (: b T))))

(type + (of Duo A B) (is (tuple (: a A) (: b B))))
```

```output
[Binding Thing'T]
[Binding Duo'A'B]
```

### A method read as a value binds the defaults its first parameters are written with, evaluated where it is read.

```brix
(main
  (let foo (call Foo))
  (let f (fn-type Void) (. foo bar))
  (print "bound")
  (call f)
  (call f)
)

(fn-do _ seed Int
  (print "seed")
  (return 7)
)

(type + Foo
  (fn-do + bar _
    (param n _ (= (call seed)))
    (param m _ (= 2))
    (print (tuple n m))
  )
)
```

```output
seed
bound
(7, 2)
(7, 2)
```

### A method read as a value with its first parameter bound to its default takes no argument for it.

```brix crash
(main
  (let foo (call Foo))
  (let g Taker (. foo qux))
)

(type + Taker (is (fn-type Void (param a Int))))

(type + Foo
  (fn-do + qux _
    (param a _ (= 1))
    (print a)
  )
)
```

```output
Exception: "g" is declared Taker, but its value is function qux, which can never be Taker; convert the value, or declare "g" without a type
<no file:4>
      (let g Taker (. foo qux))
           ^^^^^^^
```

### A type written with the shorthand `[]` is named with it where a value can never be of it.

```brix crash
(main
  (let pets ([] Animal) (list (call Animal)))
)

(type + Animal)
```

```output
Exception: "pets" is declared Array'Animal, but its value is [Animal], which can never be Array'Animal; convert the value, or declare "pets" without a type
<no file:3>
      (let pets ([] Animal) (list (call Animal)))
           ^^^^^^^^^^^^^^^^
```

### A statement that names a method without calling it is rejected, since it has no effect.

```brix crash
(main
  (let thing (call Thing))
  (call (. thing run))
)

(type + Thing
  (fn-do + run _
    close
  )
  (fn-do + close _
    (print "closed")
  )
)
```

```output
Exception: `close` cannot be a `row` of (fn-do …), which takes Step. This body runs steps, and each row is a step: a statement form, a `call`, markup, or a value that `pipe` stages after it work on. To return a value, write `(return value)`. If the body is only a value, write `fn-expr`, or remove `:do` from a closure.
<no file:9>
        close
        ^
```

### A statement that names a member method without calling it is rejected.

```brix crash
(main
  (let thing (call Thing))
  (. thing close)
)

(type + Thing
  (fn-do + close _
    (print "closed")
  )
)
```

```output
Exception: (. …) cannot be a `row` of (main …), which takes Step. This body runs steps, and each row is a step: a statement form, a `call`, markup, or a value that `pipe` stages after it work on. To return a value, write `(return value)`. If the body is only a value, write `fn-expr`, or remove `:do` from a closure.
<no file:4>
      (. thing close)
      ^
```

### A statement that names a method through this without calling it is rejected.

```brix crash
(main
  (let thing (call Thing))
  (call (. thing run))
)

(type + Thing
  (fn-do + run _
    (. this close)
  )
  (fn-do + close _
    (print "closed")
  )
)
```

```output
Exception: (. …) cannot be a `row` of (fn-do …), which takes Step. This body runs steps, and each row is a step: a statement form, a `call`, markup, or a value that `pipe` stages after it work on. To return a value, write `(return value)`. If the body is only a value, write `fn-expr`, or remove `:do` from a closure.
<no file:9>
        (. this close)
        ^
```

### Method references used as values, and calls with arguments on following lines, still compile.

```brix
(main
  (let thing (call Thing))
  (call (call (. thing handler)))
  (call (call (. thing lambda)))
  (call (call (. thing picked)))
  (call (. thing write-twice)
    (block
      (arg "x")
    )
  )
)

(type + Thing
  (fn-expr + handler Doer
    close
  )
  (fn-do + lambda Doer
    (return (=> () :do (call close)))
  )
  (fn-do + picked Doer
    (return close)
  )
  (fn-do + write-twice _
    (param text String)
    (print text)
    (print text)
  )
  (fn-do + close _
    (print "closed")
  )
)
```

```output
closed
closed
closed
x
x
```

### A do lambda whose body names a method without calling it is rejected.

```brix crash
(main
  (let thing (call Thing))
  (let f (call (. thing make)))
  (call f)
)

(type + Thing
  (fn-do + make Doer
    (return (=> () :do close))
  )
  (fn-do + close _
    (print "closed")
  )
)
```

```output
Exception: `close` cannot be the `body` of (=> …), which takes Step. This body runs steps, and each row is a step: a statement form, a `call`, markup, or a value that `pipe` stages after it work on. To return a value, write `(return value)`. If the body is only a value, write `fn-expr`, or remove `:do` from a closure.
<no file:10>
        (return (=> () :do close))
                           ^
```
