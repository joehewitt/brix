# Do functions

### Expression functions return their final expression

```brix
(main
  (print (call label 1))
  (print (call label 2))
)

(fn-expr + label String
  (param n Int)
  (case? (== n 1)
    "one"
  )
  (default
    "other"
  )
)
```

```output
one
other
```

### Non-optional do returns accept an infinite loop whose reachable paths return.

```brix
(main
  (print (call always))
  (print (call always-boolean))
)

(fn-do + always String
  (while 1
    (return "returned")
  )
)

(fn-do + always-boolean String
  (while true
    (return "boolean returned")
  )
)
```

```output
returned
boolean returned
```

### Declared non-optional do returns cannot silently fall through to a null object.

```brix crash
(main
  (print (. (call missing) value))
)

(type + Item
  (prop + value Int (= 1))
)

(fn-do + missing Item
  (call Item)
)
```

```output
Exception: Function missing ends with a value it does not return; return it with "(return value)", or remove it
<no file:11>
      (call Item)
      ^^^^^^^^^^^
```

### Expression functions reject explicit return

```brix crash
(main
  (print (call foo))
)

(fn-expr + foo _
  (return 1)
)
```

```output
Exception: (return …) cannot be the `body` of (fn-expr …), which takes Value. This body is a value, and a statement cannot stand in it. To run steps, write `fn-do`, or `:do` in a closure, and return with `(return value)`; an expression function can also run them first in a `(do …)` in `(before …)`. Otherwise replace the statement with a value.
<no file:7>
      (return 1)
      ^
```

### Expression functions reject return-if

```brix crash
(main
  (print (call foo))
)

(fn-expr + foo _
  (return-if 1 (if true))
  2
)
```

```output
Exception: (return-if …) cannot be the `body` of (fn-expr …), which takes Value. This body is a value, and a statement cannot stand in it. To run steps, write `fn-do`, or `:do` in a closure, and return with `(return value)`; an expression function can also run them first in a `(do …)` in `(before …)`. Otherwise replace the statement with a value.
<no file:7>
      (return-if 1 (if true))
      ^
```

### Expression functions reject condition-only return-if

```brix crash
(main
  (print (call foo))
)

(fn-expr + foo _
  (return-if (if true))
  2
)
```

```output
Exception: (return-if …) cannot be the `body` of (fn-expr …), which takes Value. This body is a value, and a statement cannot stand in it. To run steps, write `fn-do`, or `:do` in a closure, and return with `(return value)`; an expression function can also run them first in a `(do …)` in `(before …)`. Otherwise replace the statement with a value.
<no file:7>
      (return-if (if true))
      ^
```

### Expression functions reject immediate declarations

```brix crash
(main
  (print (call foo))
)

(fn-expr + foo _
  (let value 1)
  value
)
```

```output
Exception: (let …) cannot be the `body` of (fn-expr …), which takes Value. This body is a value, and a statement cannot stand in it. To run steps, write `fn-do`, or `:do` in a closure, and return with `(return value)`; an expression function can also run them first in a `(do …)` in `(before …)`. Otherwise replace the statement with a value.
<no file:7>
      (let value 1)
      ^
```

### Expression return-mode validation reaches helpers used by streamed constructors

```brix crash
(main
  (for (in item (call items))
    (print (. item value))
    (break)
  )
)

(type + Item
  (prop + value (? String))
  (fn-do + This _
    (= value (call invalid-helper))
  )
)

(fn-expr _ invalid-helper _
  (let result "hello")
  result
)

(fn-do + items (stream Item)
  (yield (call Item))
)
```

```output
Exception: (let …) cannot be the `body` of (fn-expr …), which takes Value. This body is a value, and a statement cannot stand in it. To run steps, write `fn-do`, or `:do` in a closure, and return with `(return value)`; an expression function can also run them first in a `(do …)` in `(before …)`. Otherwise replace the statement with a value.
<no file:17>
      (let result "hello")
      ^
```

### Expression functions reject assignments outside a do block

```brix crash
(main
  (print (call foo 1))
)

(fn-expr + foo _
  (param n Int)
  (= n 2)
  n
)
```

```output
Exception: (= …) takes (| …) here, and `n` stands in its place. Its shape is (= (| pattern:Pattern+) value:Expr).
<no file:8>
      (= n 2)
         ^
```

### Expression functions run assignments in a do block before their value

```brix
(main
  (print (call foo 1))
)

(fn-expr + foo _
  (param n Int)
  (before
    (do
      (= n 2)
    )
  )
  n
)
```

```output
2
```

### Expression functions reject for loops

```brix crash
(main
  (print (call foo))
)

(fn-expr + foo _
  (for (in n (..< 0 1))
    (print n)
  )
  0
)
```

```output
Exception: (for …) cannot be the `body` of (fn-expr …), which takes Value. This body is a value, and a statement cannot stand in it. To run steps, write `fn-do`, or `:do` in a closure, and return with `(return value)`; an expression function can also run them first in a `(do …)` in `(before …)`. Otherwise replace the statement with a value.
<no file:7>
      (for (in n (..< 0 1))
      ^
```

### Expression functions reject while loops

```brix crash
(main
  (print (call foo))
)

(fn-expr + foo _
  (while false
    (print "never")
  )
  0
)
```

```output
Exception: (while …) cannot be the `body` of (fn-expr …), which takes Value. This body is a value, and a statement cannot stand in it. To run steps, write `fn-do`, or `:do` in a closure, and return with `(return value)`; an expression function can also run them first in a `(do …)` in `(before …)`. Otherwise replace the statement with a value.
<no file:7>
      (while false
      ^
```

### Expression functions reject if statements

```brix crash
(main
  (print (call foo 3))
)

(fn-expr + foo Int
  (param n Int)
  (if (> n 5)
    (print "big")
  )
  n
)
```

```output
Exception: (if …) cannot be the `body` of (fn-expr …), which takes Value. To give one value or another, write `(if-expr condition value otherwise)`. To run steps, write `(if condition step … (else step …))` in a `fn-do` body.
<no file:8>
      (if (> n 5)
      ^
```

### Expression functions reject calls before their value

```brix crash
(main
  (print (call foo))
)

(fn-expr + foo Int
  (call touch)
  42
)

(fn-do + touch _
  (print "touched")
)
```

```output
Exception: A `fn-expr` body has one value, and `42` follows the value (call …). Put checks, prints, probes and `do` forms in `(before …)`, other steps in a `(do …)` there, and lazies in `(where …)`, or write `fn-do` and return with `(return value)`.
<no file:8>
      42
      ^
```

### Expression functions name lazies after their value

```brix crash
(main
  (print (call foo))
)

(fn-expr + foo Int
  (lazy base 40)
  (+ base 2)
)
```

```output
Exception: (+ …) cannot be a `row` of (fn-expr …), which takes Lazy. Only lazies follow a lazy, and a body of only lazies gives no value. Put the value before the lazies, which in an expression function go in `(where …)`.
<no file:8>
      (+ base 2)
      ^
```

### Expression functions run checks, prints and do forms before their value

```brix
(main
  (print (call foo 3))
)

(fn-expr + foo Int
  (param n Int)
  (before
    (throw (call Panic "big") (if (> n 5)))
    (do
      (print "first")
    )
    (finally
      (print "last")
    )
    (print "printed")
  )
  n
)
```

```output
first
printed
last
3
```

### Expression functions reject yield

```brix crash
(main
  (print (call foo))
)

(fn-expr + foo _
  (yield 1)
  0
)
```

```output
Exception: (yield …) cannot be the `body` of (fn-expr …), which takes Value. This body is a value, and a statement cannot stand in it. To run steps, write `fn-do`, or `:do` in a closure, and return with `(return value)`; an expression function can also run them first in a `(do …)` in `(before …)`. Otherwise replace the statement with a value.
<no file:7>
      (yield 1)
      ^
```

### Expression functions allow prep statements

```brix
(main
  (print (call foo))
)

(fn-expr + foo _
  (before
    (throw (call Panic "bad") (if false))
    (do
      (print "prep")
    )
    (finally
      (print "cleanup")
    )
    (print "before")
  )
  42
)
```

```output
prep
before
cleanup
42
```

### Imperative do functions return explicitly

```brix
(main
  (print (call choose true))
  (print (call choose false))
)

(fn-do + choose _
  (param flag Bool)
  (if flag
    (return 1)
  )
  (return 2)
)
```

```output
1
2
```

### Imperative do functions infer optional return on fallthrough

```brix
(main
  (print (call maybe true))
  (print (call maybe false))
)

(fn-do + maybe _
  (param flag Bool)
  (if flag
    (return 7)
  )
)
```

```output
7
unknown
```

### Do functions reject a value as their last step

```brix crash
(main
  (call foo)
  (print "done")
)

(fn-do + foo _
  42
)
```

```output
Exception: `42` cannot be a `row` of (fn-do …), which takes Step. This body runs steps, and each row is a step: a statement form, a `call`, markup, or a value that `pipe` stages after it work on. To return a value, write `(return value)`. If the body is only a value, write `fn-expr`, or remove `:do` from a closure.
<no file:8>
      42
      ^
```

### Bool do functions reject an unreturned final value

```brix crash
(main
  (print (call either true false))
)

(fn-do + either Bool
  (param left Bool)
  (param right Bool)
  (let together (and left right))
  (return-if true (if together))
  (or left right)
)
```

```output
Exception: (or …) cannot be a `row` of (fn-do …), which takes Step. This body runs steps, and each row is a step: a statement form, a `call`, markup, or a value that `pipe` stages after it work on. To return a value, write `(return value)`. If the body is only a value, write `fn-expr`, or remove `:do` from a closure.
<no file:11>
      (or left right)
      ^
```

### Optional do functions reject an unreturned final call

```brix crash
(main
  (print (call first-even 4))
)

(fn-do + first-even (? Int)
  (param n Int)
  (return-if null (if (< n 0)))
  (call evens n)
)

(fn-expr + evens Int
  (param n Int)
  (* n 2)
)
```

```output
Exception: Function first-even ends with a value it does not return; return it with "(return value)", or remove it
<no file:9>
      (call evens n)
      ^^^^^^^^^^^^^^
```

### Interface overrides of Bool methods reject an unreturned final value

```brix crash
(main
  (print (call ready-of (call Probe)))
)

(type + Readiness (is Interface)
  (fn-expr + is-ready Bool :get)
)

(type + Probe
  (implements Readiness)
  (prop + ready _ (= true))
  (fn-do + is-ready _ :get
    ready
  )
)

(fn-expr + ready-of _
  (param readiness Readiness)
  (. readiness is-ready)
)
```

```output
Exception: `ready` cannot be a `row` of (fn-do …), which takes Step. This body runs steps, and each row is a step: a statement form, a `call`, markup, or a value that `pipe` stages after it work on. To return a value, write `(return value)`. If the body is only a value, write `fn-expr`, or remove `:do` from a closure.
<no file:14>
        ready
        ^
```

### Class overrides of Bool methods reject an unreturned final value

```brix crash
(main
  (let check Checker (call StrictChecker))
  (print (. check passes))
)

(type + Checker
  (fn-do + passes Bool :get
    (return false)
  )
)

(type + StrictChecker (is Checker)
  (prop + strict _ (= true))
  (fn-do + passes _ :get
    strict
  )
)
```

```output
Exception: `strict` cannot be a `row` of (fn-do …), which takes Step. This body runs steps, and each row is a step: a statement form, a `call`, markup, or a value that `pipe` stages after it work on. To return a value, write `(return value)`. If the body is only a value, write `fn-expr`, or remove `:do` from a closure.
<no file:16>
        strict
        ^
```

### Inferred do returns reject an unreturned final value

```brix crash
(main
  (print (call pick true))
)

(fn-do + pick _
  (param flag Bool)
  (if flag
    (return 7)
  )
  8
)
```

```output
Exception: `8` cannot be a `row` of (fn-do …), which takes Step. This body runs steps, and each row is a step: a statement form, a `call`, markup, or a value that `pipe` stages after it work on. To return a value, write `(return value)`. If the body is only a value, write `fn-expr`, or remove `:do` from a closure.
<no file:11>
      8
      ^
```

### Bool and optional do functions fall through after final statements without values

```brix
(main
  (let counter (call Counter))
  (print (call (. counter bump)))
  (print (?? (call (. counter last-even)) "none"))
  (print (. counter count))
)

(type + Counter
  (prop + count Int)
  (prop + label (? String))
  (fn-do + bump Bool
    (return-if true (if (> count 3)))
    (+= count 1)
  )
  (fn-do + last-even (? String)
    (return-if label (if (> count 3)))
    (= label "odd")
    (= count (+ count 1))
  )
)
```

```output
false
none
2
```

### Empty imperative do functions are concrete no-op Void bodies

```brix
(main
  (call empty-noop)
  (print "done")
)

(fn-do + empty-noop _)
```

```output
done
```

### Empty non-do functions reject inferred Void

```brix crash
(main
  (call empty-noop)
)

(fn-expr + empty-noop _)
```

```output
Exception: Non-do function must return a non-void value
<no file:6>
    (fn-expr + empty-noop _)
    ^^^^^^^^^^^^^^^^^^^^^^^^
```

### Non-do functions reject explicit Void

```brix crash
(main
  (call explicit-void)
)

(fn-expr + explicit-void Void)
```

```output
Exception: Non-do function must return a non-void value
<no file:6>
    (fn-expr + explicit-void Void)
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Non-do functions with a declared return reject a body that produces no value

```brix crash
(main
  (print (call stored 1))
)

(fn-expr + stored Int
  (param n Int)
  (do
    (= n 4)
  )
)
```

```output
Exception: (do …) cannot be the `body` of (fn-expr …), which takes Value. This body is a value, and a statement cannot stand in it. To run steps, write `fn-do`, or `:do` in a closure, and return with `(return value)`; an expression function can also run them first in a `(do …)` in `(before …)`. Otherwise replace the statement with a value.
<no file:8>
      (do
      ^
```

### This constructors are implicitly imperative

```brix
(main
  (print (. (call Thing 5) value))
)

(type + Thing
  (prop + value Int)
  (fn-do + This _
    (param value Int)
    (= (. this value) value)
  )
)
```

```output
5
```

### Constructors ignore unused method results

```brix
(main
  (print (. (call Thing) value))
)

(type + Thing
  (prop + value _ (= 0))
  (fn-do + This _
    (call initialize)
  )
  (fn-do _ initialize Int
    (= value 5)
    (return 42)
  )
)
```

```output
5
```

### Imperative closures return explicitly

```brix
(main
  (let fn
    (=> () :do
      (block
        (return 9)
      )
    )
  )
  (blank)
  (print (call fn))
)
```

```output
9
```

### Expression closures reject explicit return

```brix crash
(main
  (let fn
    (=> ()
      (block
        (return 9)
      )
    )
  )
  (blank)
  (print (call fn))
)
```

```output
Exception: (return …) cannot be the `body` of (block …), which takes Answer. A function value without `:do` is a value, and a statement cannot stand in it. To run steps, write `:do` and return with `(return value)`, or run them first in a `(do …)`. Otherwise replace the statement with a value.
<no file:6>
            (return 9)
            ^
```

### Expression closures reject assignments before their value

```brix crash
(main
  (let calls 0)
  (let counter
    (=> ()
      (block
        (+= calls 1)
        calls
      )
    )
  )
  (blank)
  (print (call counter))
)
```

```output
Exception: (+= …) cannot be the `body` of (block …), which takes Answer. A function value without `:do` is a value, and a statement cannot stand in it. To run steps, write `:do` and return with `(return value)`, or run them first in a `(do …)`. Otherwise replace the statement with a value.
<no file:7>
            (+= calls 1)
            ^
```

### Expression closures run checks, prints and do forms before their value

```brix
(main
  (let calls 0)
  (let counter
    (=> ((: n Int))
      (block
        (throw (call Panic "big") (if (> n 5)))
        (do
          (+= calls 1)
        )
        (finally
          (print "last")
        )
        (print "printed")
        (+ calls n)
      )
    )
  )
  (blank)
  (print (call counter 3))
)
```

```output
printed
last
4
```

### Module top-level rejects explicit return

```brix crash
(main
  (return 1)
)
```

```output
Exception: The top level of a module cannot return a value with "return"; remove the "return", or print the value with "print"
<no file:3>
      (return 1)
      ^^^^^^^^^^
```

### Module top-level rejects value-return-if

```brix crash
(main
  (return-if 1 (if true))
)
```

```output
Exception: The top level of a module cannot return a value with "return"; remove the "return", or print the value with "print"
<no file:3>
      (return-if 1 (if true))
      ^^^^^^^^^^^^^^^^^^^^^^^
```

```brix
(main
  (print (call (of choose-size Int)))
  (print (call (of choose-size (tuple Int Int Int Int Int))))
)

(fn-do + (of choose-size Item) Int
  (if (<= (. Item size) 16)
    (return 1)
    (else
      (return 2)
    )
  )
)
```

```output
1
2
```

### A Void function can return a Void call

```brix
(main
  (call check "abc")
  (call check "a")
  (call finish "done")
)

(fn-do + noted _
  (param text String)
  (print (str "noted " text))
)

(fn-do + check _
  (param text String)
  (return-if (call noted text) (if (> (. text length) 2)))
  (print (str "short " text))
)

(fn-do + finish _
  (param text String)
  (return (call noted text))
)
```

```output
noted abc
short a
noted done
```

### The returns of a `do` function: a value, nothing, a value under a condition, and the condition alone.

```brix
(main
  (print (call pick true))
  (print (call pick false))
  (print (call small 1))
  (print (call small 50))
  (call show 1)
  (call show 5)
  (print (call first-over 3))
)

(fn-do + pick Int
  (param flag Bool)
  (return-if 1 (if flag))
  (return 2)
)

(fn-do + small Bool
  (param n Int)
  (return-if (if (> n 10)))
  (return true)
)

(fn-do + show _
  (param n Int)
  (if (> n 3)
    (return)
  )
  (print n)
)

(fn-do + first-over Int
  (param limit Int)
  (let i 0)
  (while true
    (+= i 1)
    (if (> i limit)
      (return i)
    )
  )
)
```

```output
1
2
true
false
1
4
```

### A `do` function that runs off its end returns nothing, or false when it returns a Bool.

```brix
(main
  (print (call found 8))
  (print (call found 2))
  (call quiet 1)
  (print "end")
)

(fn-do + found Bool
  (param n Int)
  (if (> n 5)
    (return true)
  )
)

(fn-do + quiet _
  (param n Int)
  (+= n 1)
)
```

```output
true
false
end
```

### A returned value is converted to the type the function declares.

```brix
(main
  (let x I32 3)
  (print (call f))
  (print (call g x))
  (print (call h x))
)

(fn-do + f Float
  (return 1)
)

(fn-do + g I64
  (param n I32)
  (return n)
)

(fn-do + h Int
  (param n I32)
  (return-if 200 (if (> n 2)))
  (return n)
)
```

```output
1
3
200
```

### A `do` function with no return type returns the type its returns promote to.

```brix
(main
  (print (call pick true))
  (print (call pick false))
)

(fn-do + pick _
  (param flag Bool)
  (if flag
    (return 1)
  )
  (return 2.5)
)
```

```output
1
2.5
```

### The body of a `do` function has locals, branches and loops.

```brix
(main
  (print (call spin 3))
  (print (call spin 100000))
  (print (call label 7))
)

(fn-do + spin Int
  (param n Int)
  (let total 0)
  (for (in i (..< 0 n))
    (+= total (% i 7))
  )
  (return total)
)

(fn-do + label String
  (param n Int)
  (let text "")
  (let left n)
  (while (> left 0)
    (if (== (% left 2) 0)
      (++= text "e")
      (else
        (++= text "o")
      )
    )
    (-= left 1)
  )
  (return text)
)
```

```output
3
299995
oeoeoeo
```

### A `do` function that declares a value to return cannot run off its end.

```brix crash
(main
  (print (call missing))
)

(fn-do + missing Int
  (let a 1)
)
```

```output
Exception: Function missing can fall through without returning I64
<no file:6>
    (fn-do + missing Int
    ^^^^^^^^^^^^^^^^^^^
```

### A branch that does not return leaves the end of its function in reach.

```brix crash
(main
  (print (call part 3))
)

(fn-do + part Float
  (param n Int)
  (if (> n 2)
    (return 1.5)
  )
)
```

```output
Exception: Function part can fall through without returning F64
<no file:6>
    (fn-do + part Float
    ^^^^^^^^^^^^^^^^^^
```

### The end of a function is out of reach after a return, an endless loop, or an `if` on the constant true.

```brix
(main
  (print (call after 3))
  (print (call endless 3))
  (print (call always 3))
)

(fn-do + after Int
  (param n Int)
  (if (> n 5)
    (return 1)
  )
  (return 2)
  (+= n 1)
)

(fn-do + endless Int
  (param n Int)
  (while true
    (+= n 1)
    (if (> n 5)
      (return n)
    )
  )
)

(fn-do + always Int
  (param n Int)
  (if true
    (return n)
  )
)
```

```output
2
6
3
```

### A function may end on a match that must match: no arm matching is a panic, so the end is not in reach.

```brix
(main
  (print (call pick (call (. Item left) 1)))
  (print (call pick (call (. Item right) 2)))
)

(type + Left (is Tuple)
  (prop + value I32)
)

(type + Right (is Tuple)
  (prop + value I32)
)

(type + Item (is Union)
  (variant left Left)
  (variant right Right)
)

(fn-do + pick Int
  (param item Item)
  (match item
    (case (call .left row)
      (if (== (. row value) 1)
        (return 10)
      )
      (return 11)
    )
    (case (call .right row)
      (return 20)
    )
  )
)
```

```output
10
20
```

### A match written as a step, whose value nothing uses, runs its arms as steps, so an arm may only print.

```brix
(main
  (call step 3)
  (call step 4)
)

(fn-do + step _
  (param n Int)
  (match n
    (case= 3
      (print "three")
    )
    (default
      (print "other")
    )
  )
)
```

```output
three
other
```

### In Brix, a match on a row of its own in a body of steps runs its arms as steps: in a function's body, in an `if`, an `else` or a loop, in an arm of a match run as steps, in a closure that runs steps and in `main`. A `match?` run as steps goes on where no arm is taken, and a function may end on a match whose arms all return.

```brix
(type + Shade (is Union)
  (variant red)
  (variant blue)
)

(fn-do + show _
  (param s Shade)
  (param t Shade)
  (match s
    (case .red (print "red"))
    (case .blue
      (match t
        (case .red (print "blue, red"))
        (case .blue (print "blue, blue"))
      )
    )
  )
)

(fn-do + tally _
  (param shades (list Shade))
  (param loud Bool)
  (let reds 0)
  (for (in s shades)
    (match s
      (case .red (+= reds 1))
      (case .blue (print "blue"))
    )
  )
  (if loud
    (match reds
      (case= 0 (print "no red"))
      (default (print "some red"))
    )
    (else
      (inline
        (match? ([] shades 0)
          (case .red (print "quiet red"))
        )
      )
    )
  )
  (print (str "reds " reds))
)

(fn-do + code I64
  (param s Shade)
  (match s
    (case .red (return 1))
    (case .blue (return 2))
  )
)

(main
  (call show .red .blue)
  (call show .blue .red)
  (call tally (list (. Shade red) (. Shade blue)) true)
  (call tally (list (. Shade blue)) false)
  (print (str (call code .red) " " (call code .blue)))
  (let s Shade .blue)
  (let run (=> () :do (match s (case .red (print "closure red")) (case .blue (print "closure blue")))))
  (call run)
  (match s
    (case .red (print "main red"))
    (case .blue (print "main blue"))
  )
)
```

```output
red
blue, red
blue
some red
reds 1
blue
reds 0
1 2
closure blue
main blue
```

### A loop on the constant true can be left by a break in an arm of a match, so what follows it is in reach.

```brix crash
(main
  (print (call pick (call (. Item left) 1)))
)

(type + Left (is Tuple)
  (prop + value I32)
)

(type + Right (is Tuple)
  (prop + value I32)
)

(type + Item (is Union)
  (variant left Left)
  (variant right Right)
)

(fn-do + pick Int
  (param item Item)
  (while true
    (match? item
      (case (call .left row)
        (break)
      )
    )
    (return 3)
  )
)
```

```output
Exception: Function pick can fall through without returning I64
<no file:19>
    (fn-do + pick Int
    ^^^^^^^^^^^^^^^^
```

### A function an interface requires and gives no body has no end to run off.

```brix
(main
  (let thing (call Thing))
  (call foo thing)
)

(type + Proto (is Interface)
  (fn-do + bar Int)
)

(type + Thing
  (implements Proto)
  (fn-do + bar Int
    (return 42)
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

### A condition that is the constant false is its else, and a return under the constant true ends the function.

```brix
(main
  (print (call first))
  (print (call second))
  (print (call third))
)

(fn-do + first Int
  (if false
    (print 0)
    (else
      (return 1)
    )
  )
)

(fn-do + second Int
  (return-if 4 (if true))
)

(fn-do + third Int
  (if 0
    (print 5)
    (else
      (return 6)
    )
  )
)
```

```output
1
4
6
```

### A condition that is the constant false and has no else leaves the end in reach.

```brix crash
(main
  (print (call pick))
)

(fn-do + pick Int
  (if false
    (return 1)
  )
)
```

```output
Exception: Function pick can fall through without returning I64
<no file:6>
    (fn-do + pick Int
    ^^^^^^^^^^^^^^^^
```

### A return of a call that returns nothing leaves the function returning nothing, and a conditional return of a value makes it optional.

```brix
(main
  (call check "abc")
  (call check "a")
  (call finish "done")
  (print (call pick 1))
  (print (?? (call pick 0) 9))
  (call both true)
  (call both false)
)

(fn-do + noted _
  (param text String)
  (print (str "noted " text))
)

(fn-do + check _
  (param text String)
  (return-if (call noted text) (if (> (. text length) 2)))
  (print (str "short " text))
)

(fn-do + finish _
  (param text String)
  (return (call noted text))
)

(fn-do + pick _
  (param n Int)
  (return-if (* n 2) (if (> n 0)))
  (print "none")
)

(fn-do + both _
  (param loud Bool)
  (if loud
    (return (call noted "loud"))
  )
  (call noted "quiet")
)
```

```output
noted abc
short a
noted done
2
none
9
noted loud
noted quiet
```

### A method written with no body gives what the type it returns makes with nothing written.

```brix
(main
  (let h (call Holder))
  (print (str "[" (call (. h text)) "]"))
  (print (call (. h maybe)))
  (print (call (. h point)))
  (print (call (. h flag)))
  (print (call (. h ratio)))
  (print (. (call (. h items)) length))
  (print "end")
)

(type + Point (is (tuple (: x Int) (: y Int))))

(type + Holder
  (fn-expr + text String)
  (fn-expr + maybe (? Int))
  (fn-expr + point Point)
  (fn-expr + flag Bool)
  (fn-expr + ratio Float)
  (fn-expr + items (list Int))
)
```

```output
[]
unknown
(0, 0)
false
0
0
end
```

### A function declared outside any type with no body, which no compiler intrinsic answers, has nothing to give.

```brix compile-error
(main
  (print (call twice 2.0))
)

(fn-expr + twice F64
  (param n F64)
)
```

```output
Exception: Function "twice" has no body, and no compiler intrinsic supplies one
<no file:6>
    (fn-expr + twice F64
    ^^^^^^^^^^^^^^^^^^^
```

### A generic method written with no body gives an object of the type it names.

```brix
(main
  (let d (call Holder))
  (print (call (of (. d get-foo) Int)))
  (print "after")
)

(type + (of Foo T))

(type + Holder
  (fn-expr + (of get-foo Other) (of Foo Other))
)
```

```output
[Object __source__:Foo'core/number:I64]
after
```

### Non-do functions reject a call before their value even when the body can fail

```brix crash
(main
  (print (catch (call pair 1)))
)

(fn-expr + pair Int (?! Failure)
  (param n Int)
  (call checked n)
  (call checked (+ n 1))
)

(fn-do + checked Int (?! Failure)
  (param n Int)
  (throw (call Failure "negative") (if (< n 0)))
  (return n)
)

(type + Failure (is Error))
```

```output
Exception: A `fn-expr` body has one value, and (call …) follows the value (call …). Put checks, prints, probes and `do` forms in `(before …)`, other steps in a `(do …)` there, and lazies in `(where …)`, or write `fn-do` and return with `(return value)`.
<no file:9>
      (call checked (+ n 1))
      ^
```

### Expression functions reject explicit return where they are declared, though nothing calls them

```brix crash
(main
  (print "never")
)

(fn-expr + foo _
  (return 1)
)
```

```output
Exception: (return …) cannot be the `body` of (fn-expr …), which takes Value. This body is a value, and a statement cannot stand in it. To run steps, write `fn-do`, or `:do` in a closure, and return with `(return value)`; an expression function can also run them first in a `(do …)` in `(before …)`. Otherwise replace the statement with a value.
<no file:7>
      (return 1)
      ^
```

### Getters that are not do reject immediate declarations

```brix crash
(main
  (print (. (call Holder) name))
)

(type + Holder
  (prop + value Int (= 1))
  (fn-expr + name String :get
    (let text "held")
    text
  )
)
```

```output
Exception: (let …) cannot be the `body` of (fn-expr …), which takes Value. This body is a value, and a statement cannot stand in it. To run steps, write `fn-do`, or `:do` in a closure, and return with `(return value)`; an expression function can also run them first in a `(do …)` in `(before …)`. Otherwise replace the statement with a value.
<no file:9>
        (let text "held")
        ^
```

### A `do` function that returns from inside a `do use` block returns what it gives there.

```brix
(main
  (print (. (call make 3) size))
)

(fn-do + make _
  (param size U32)
  (let res (call Res))
  (do-use res
    (return (call Box size))
  )
)

(type + Res
  (implements (of Usable Void))
  (fn-expr + begin-using _
    this
  )
  (fn-do + end-using _
    (print "end")
  )
)

(type + Box
  (prop + size U32)
  (fn-do + This _
    (param size U32)
  )
)
```

```output
end
3
```

### A `do` function that ends in a loop returns an optional, as though it could run past the loop, even where the loop never ends.

```brix crash
(main
  (print (+ (call find 3) 1))
)

(fn-do + find _
  (param n Int)
  (let i 0)
  (while true
    (if (== i n)
      (return i)
    )
    (+= i 1)
  )
)
```

```output
Exception: Operator "+" is not defined for I64? and I64
<no file:3>
      (print (+ (call find 3) 1))
             ^^^^^^^^^^^^^^^^^^^
```

### A `do` function that ends in a match that must match returns what its arms return when each arm returns.

```brix
(main
  (print (+ (call size (call (. Shape circle) 2)) 1))
)

(type + Shape (is Union)
  (variant circle (tuple (: r Int)))
  (variant square (tuple (: s Int)))
)

(fn-do + size _
  (param shape Shape)
  (match shape
    (case (call .circle r)
      (return r)
    )
    (case (call .square s)
      (return (* s 2))
    )
  )
)
```

```output
3
```

### Statements after one that returns are never reached, and are not typed.

```brix
(main
  (print (call value))
)

(fn-do + value Int
  (return 1)
  (pipe *> each)
)
```

```output
1
```

### `=? value` of an optional returns only where the optional holds a value, so the statements after it run where it holds none.

```brix
(main
  (print (call first null))
  (print (call first 2))
)

(fn-do + first Int
  (param value (? Int))
  (return? value)
  (print "none")
  (return 0)
)
```

```output
none
0
2
```

### Statements after one that leaves are never bound either, so the names they use need not exist: after a return, a `continue`, and a raise under no condition.

```brix
(main
  (print (call value))
  (print (call skipped))
  (print (call failing))
)

(fn-do + value Int
  (return 1)
  (print missing-name)
  (let x (call missing-function))
)

(fn-do + skipped Int
  (let total 0)
  (for (in item (list 1 2))
    (if (== item 1)
      (continue)
      (print unknown-name)
    )
    (+= total item)
  )
  (return total)
)

(fn-do + failing Int
  (let maybe (? Int) 3)
  (return? maybe)
  (throw (call Panic "never"))
  (print after-panic)
  (return 0)
)
```

```output
1
2
3
```
