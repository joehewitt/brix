# Lazy variables

### Unused lazy variables are not evaluated.

```brix
(main
  (print "start")
  (print
    (scope kept
      (lazy kept (call tick "kept" 1))
      (lazy unused (call tick "unused" 99))
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
start
tick kept
1
```

### A module variable demanded while compiling a cross-module virtual override initializes before the module main call that first dispatches to the override.

```brix
(import ./lib/base)
(blank)

(main
  (print (call read-through-base (call Derived)))
)

(blank)
(prop _ module-value Int (= 37))

(type + Derived (is Base)
  (fn-expr + value Int
    module-value
  )
)
```

```brix file=lib/base.bx
(type + Base
  (fn-expr + value Int
    0
  )
)

(fn-expr + read-through-base Int
  (param item Base)
  (call (. item value))
)
```

```output
37
```

### A single lazy use may be evaluated directly at the use site.

```brix
(main
  (print
    (scope value
      (lazy value (call tick "single" 42))
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
tick single
42
```

### Lazy-only dependency chains evaluate each dependency once in demand order.

```brix
(main
  (print
    (scope c
      (lazy c
        (call tick "c" (+ b 1)
          (block
            (lazy b
              (call tick "b" (+ a 1)
                (block
                  (lazy a (call tick "a" 1))
                )
              )
            )
          )
        )
      )
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
tick a
tick b
tick c
3
```

### Lazy initializer dependency reads do not materialize dependencies at declaration sites.

```brix
(prop _ hits _ (= 0))
(blank)

(main
  (if false
    (print a)
  )
  (blank)
  (print b)
  (print hits)
  (blank)
  (lazy a (+ b b))
  (lazy b (+= hits 1))
)
```

```output
1
1
```

### Repeated reads after a dominating first read evaluate the initializer once.

```brix
(main
  (print
    (scope (+ value value)
      (lazy value (call tick "repeat" 21))
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
tick repeat
42
```

### A lazy value used only in one branch is not evaluated by the other branch.

```brix
(main
  (call choose false)
  (call choose true)
)

(fn-do + choose _
  (param use-value Bool)
  (if use-value
    (print (str "used " value))
    (else
      (print "skipped")
    )
  )
  (blank)
  (lazy value (call tick "branch" 5))
)

(fn-do + tick _
  (param label String)
  (param value Int)
  (print (str "tick " label))
  (return value)
)
```

```output
skipped
tick branch
used 5
```

### First evaluation in either branch is visible after the branch joins.

```brix
(main
  (call choose true)
  (call choose false)
)

(fn-do + choose _
  (param left Bool)
  (if left
    (print (str "left " value))
    (else
      (print (str "right " value))
    )
  )
  (blank)
  (print (str "after " value))
  (blank)
  (lazy value (call tick "join" 8))
)

(fn-do + tick _
  (param label String)
  (param value Int)
  (print (str "tick " label))
  (return value)
)
```

```output
tick join
left 8
after 8
tick join
right 8
after 8
```

### A single lexical read in a loop is still many dynamic reads, but evaluates once per lazy lifetime.

```brix
(main
  (for (in i (..< 0 3))
    (print (str i ":" value))
  )
  (blank)
  (lazy value (call tick "loop" 7))
)

(fn-do + tick _
  (param label String)
  (param value Int)
  (print (str "tick " label))
  (return value)
)
```

```output
tick loop
0:7
1:7
2:7
```

### A single lexical read in a while loop is still many dynamic reads.

```brix
(prop _ i _ (= 0))
(blank)

(main
  (while (< i 3)
    (print (str i ":" value))
    (+= i 1)
  )
  (blank)
  (lazy value (call tick "while" 7))
)

(fn-do + tick _
  (param label String)
  (param value Int)
  (print (str "tick " label))
  (return value)
)
```

```output
tick while
0:7
1:7
2:7
```

### Lazy locals have a fresh lifetime for each function call.

```brix
(main
  (call foo)
  (call foo)
)

(fn-do + foo _
  (print value)
  (print value)
  (blank)
  (lazy value (call tick "call" 9))
)

(fn-do + tick _
  (param label String)
  (param value Int)
  (print (str "tick " label))
  (return value)
)
```

```output
tick call
9
9
tick call
9
9
```

### An escaping closure can capture a lazy local and memoize it for that closure instance.

```brix
(main
  (let fn (call make))
  (print (call fn))
  (print (call fn))
)

(fn-expr + make _
  (=> () value)
  (where
    (lazy value (call tick "closure" 3))
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
tick closure
3
3
```

### Mutating a captured lazy local initializes it before applying the mutation.

```brix
(main
  (let add (call make-counter))
  (print (call add 2))
  (print (call add 3))
)

(fn-expr + make-counter _
  (=> (n) :do
    (block
      (+= total n)
      (return total)
    )
  )
  (where
    (lazy total (call tick "counter" 0))
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
tick counter
2
5
```

### Mutating a local lazy variable before a plain read initializes it first.

```brix
(main
  (+= value 2)
  (print value)
  (blank)
  (lazy value (call tick "mutate" 10))
)

(fn-do + tick _
  (param label String)
  (param value Int)
  (print (str "tick " label))
  (return value)
)
```

```output
tick mutate
12
```

### Lazy variables with only a type use the type's default value and memoize it.

```brix
(main
  (print value)
  (print value)
  (blank)
  (lazy value Int)
)
```

```output
0
0
```

### A recoverable error result from lazy evaluation is memoized, not retried.

```brix
(prop _ attempts _ (= 0))
(blank)

(main
  (print (?? (catch? value) -1))
  (print (?? (catch? value) -1))
  (print value)
  (print attempts)
  (blank)
  (lazy value (call maybe))
)

(fn-do + maybe Int (?! Error)
  (+= attempts 1)
  (print (str "attempt " attempts))
  (blank)
  (if (== attempts 1)
    (throw (call Error "first"))
  )
  (blank)
  (return 12)
)
```

```output
attempt 1
-1
-1
first
1
```

### Circular lazy references are diagnosed.

```brix crash
(main
  (print a)
  (blank)
  (lazy a b)
  (lazy b a)
)
```

```output
Exception: "a" depends on its own value, directly or through the values it reads; break the cycle
<no file:5>
      (lazy a b)
            ^
```

### Lazy tuple destructuring evaluates the shared initializer once for all captures.

```brix
(main
  (call foo)
)

(fn-do + foo _
  (print a)
  (print b)
  (print (+ a b))
  (blank)
  (lazy (tuple a b) (call pair))
)

(fn-do + pair _
  (print "pair")
  (return (tuple 1 2))
)
```

```output
pair
1
2
3
```

### Lazy variables in markup scope are visible to nested content and are not reevaluated.

```brix
(main
  (<> Foo (id: root)
    (lazy value (call tick "markup" 41))
    (<> Foo
      (= a (+ value 1))
      (<> Foo
        (= a (+ value 2))
      )
    )
  )
  (blank)
  (print (?. (. root child) a))
  (print (?. (?. (. root child) child) a))
)

(type + Foo
  (prop + a _ (= 0))
  (prop + child (? Foo))
  (fn-do + (infix this += (param foo _)) _
    (= child foo)
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
tick markup
42
43
```

### Local lazy declaration state machines initialize once per lazy declaration lifetime.

```brix
(main
  (call lazy-local "first")
  (call lazy-local "second")
)

(blank)
(prop _ hits _ (= 0))

(fn-do + lazy-local _
  (param label String)
  (lazy flow
    (machine
      (state idle)
      (state ready (trigger if (call mark label)))
    )
  )
  (blank)
  (print (str "before " label " " hits))
  (print (is flow .ready))
  (print (str "middle " label " " hits))
  (print (is flow .ready))
  (print (str "after " label " " hits))
)

(fn-do + mark Bool
  (param label String)
  (+= hits 1)
  (print (str "mark " label " " hits))
  (return true)
)
```

```output
before first 0
mark first 1
true
middle first 1
true
after first 1
before second 1
mark second 2
true
middle second 2
true
after second 2
```

### A lazy local nothing reads is never evaluated.

```brix
(main
  (print "start")
  (blank)
  (lazy a (call noisy))
  (lazy b (+ a 1))
  (blank)
  (print "done")
)

(fn-do + noisy Int
  (print "noisy")
  (return 1)
)
```

```output
start
done
```

### A lazy local of a constructor that is named as a property gives the property its first value at the turn of the property, and a lazy local a property reads is evaluated then, once.

```brix
(main
  (let h (call Holder 7))
  (print (str "made " (. h a) " " (. h b) " " (. h c) " " (. h d) " " (. h e)))
)

(fn-expr + note Int
  (param text String)
  (param n Int)
  (before
    (print text)
  )
  n
)

(type + Holder
  (prop + a _ (= (call note "a default" 1)))
  (prop + b Int)
  (prop + c _ (= (call note "c default" 3)))
  (prop + d Int)
  (prop + e _ (= (call note "e default" 5)))
  (fn-do + This _
    (param seed Int)
    (lazy d (call note "d lazy" (+ b seed)))
    (lazy b (call note "b lazy" 20))
    (lazy e (call note "e lazy" 50))
    (print "body")
  )
)
```

```output
a default
b lazy
c default
d lazy
e lazy
body
made 1 20 3 27 50
```

### A lazy local that a property of a constructor reads is evaluated once for the properties, and again for the body.

```brix
(main
  (let k (call Keeper))
  (print (str "made " (. k b) " " (. k c)))
  (let m (call Mixed))
  (print (str "made " (. m b)))
)

(fn-expr + note Int
  (param text String)
  (param n Int)
  (before
    (print text)
  )
  n
)

(type + Keeper
  (prop + b Int)
  (prop + c Int)
  (fn-do + This _
    (lazy b (+ c (call note "b lazy" 1)))
    (lazy c (call note "c lazy" 2))
    (print "body")
  )
)

(type + Mixed
  (prop + b Int)
  (fn-do + This _
    (lazy helper (call note "helper" 5))
    (lazy b (+ helper (call note "b lazy" 1)))
    (print (str "body " helper))
  )
)
```

```output
c lazy
b lazy
body
made 3 2
helper
b lazy
helper
body 5
made 6
```

### A struct given before a lazy local is what it was before the local was evaluated

```brix
(prop + kept _ (= (call Vec 1)))
(blank)

(main
  (call run)
)

(fn-do + run _
  (print (call take kept later))
  (= kept (call Vec 1))
  (print (+ kept twice))
  (blank)
  (lazy later (call change 100))
  (lazy twice (call change 200))
)

(fn-expr + take Int
  (param vec Vec)
  (param n Int)
  (+ (. vec a) n)
)

(fn-do + change Int
  (param n Int)
  (= kept (call Vec n))
  (return 5)
)

(type + Vec (is Struct)
  (prop + a Int)
  (fn-expr + (infix this + (param n Int (= Int))) _
    (+ a n)
  )
)
```

```output
6
6
```

### A struct given before a lazy property is what it was before the property was evaluated

```brix
(main
  (print (call (. (call Holder) run)))
)

(fn-expr + take Int
  (param vec Vec)
  (param n Int)
  (+ (. vec a) n)
)

(type + Holder
  (prop + vec _ (= (call Vec 1)))
  (prop + later _ (:= (call change)))
  (fn-expr + run Int
    (call take vec later)
  )
  (fn-do + change Int
    (= vec (call Vec 100))
    (return 5)
  )
)

(type + Vec (is Struct)
  (prop + a Int)
)
```

```output
6
```

### A struct given before a lazy global is what it was before the global was evaluated

```brix
(prop + kept _ (= (call Vec 1)))
(prop + later _ (:= (call change)))
(blank)

(main
  (print (call take kept later))
)

(fn-expr + take Int
  (param vec Vec)
  (param n Int)
  (+ (. vec a) n)
)

(fn-do + change Int
  (= kept (call Vec 100))
  (return 5)
)

(type + Vec (is Struct)
  (prop + a Int)
)
```

```output
6
```

### Operand read before a lazy local that changes what it names

```brix
(main
  (print (call run))
  (print (call twice))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)

(fn-do + add Int
  (param first Point)
  (param second Int)
  (return (+ (. first x) (. first y) second))
)

(fn-do + run Int
  (let point (call Point 1 2))
  (lazy bumped (call bump point))
  (let total (call add point bumped))
  (return (+ total (. point x)))
)

(fn-do + bump Int
  (param point Point)
  (= (. point x) 100)
  (return (. point x))
)

(fn-do + twice Int
  (let count 1)
  (lazy stepped (+= count 10))
  (let first (+ count stepped))
  (let second (+ count stepped))
  (return (+ (* first 100) second))
)
```

```output
104
1222
```

### A lazy local assigned on one path before it is read is not evaluated on that path.

```brix
(main
  (call run true)
  (call run false)
)

(fn-do + run _
  (param c Bool)
  (if c
    (= v 5)
  )
  (print v)
  (+= v 1)
  (print v)
  (blank)
  (lazy v (call tick "v" 1))
)

(fn-do + tick _
  (param label String)
  (param value Int)
  (print (str "tick " label))
  (return value)
)
```

```output
5
6
tick v
1
2
```

### Lazy locals assigned and read in a loop are evaluated once, by the first read that runs.

```brix
(main
  (call run 3)
)

(fn-do + run _
  (param n Int)
  (let i 0)
  (while (< i n)
    (if (== i 1)
      (= v 10)
    )
    (+= total v)
    (+= i 1)
  )
  (print total)
  (print v)
  (blank)
  (lazy v (call tick "v" 1))
  (lazy total (call tick "total" 100))
)

(fn-do + tick _
  (param label String)
  (param value Int)
  (print (str "tick " label))
  (return value)
)
```

```output
tick total
tick v
121
10
```

### A lazy local assigned before anything reads it is never evaluated.

```brix
(main
  (call run)
)

(fn-do + run _
  (= v 7)
  (= w "x")
  (+= w "y")
  (print w)
  (= z 2.5)
  (blank)
  (lazy v (call tick "v" 1))
  (lazy w (call name "w"))
  (lazy z Float (call tick "z" 3))
)

(fn-do + name _
  (param label String)
  (print (str "name " label))
  (return label)
)

(fn-do + tick _
  (param label String)
  (param value I32)
  (print (str "tick " label))
  (return value)
)
```

```output
xy
```

### A lazy local assigned on a path that returns, and read and assigned on the other.

```brix
(main
  (print (call run 1))
  (print (call run 20))
)

(fn-do + run Int
  (param n Int)
  (if (> n 10)
    (= v n)
    (return v)
  )
  (+= v n)
  (return v)
  (blank)
  (lazy v (call tick "v" 1))
)

(fn-do + tick _
  (param label String)
  (param value Int)
  (print (str "tick " label))
  (return value)
)
```

```output
tick v
2
20
```

### A lazy local assigned from itself is evaluated by the read.

```brix
(main
  (print (call run 1))
)

(fn-do + run Int
  (param n Int)
  (= v (+ v n))
  (return v)
  (blank)
  (lazy v (call tick "v" 1))
)

(fn-do + tick _
  (param label String)
  (param value Int)
  (print (str "tick " label))
  (return value)
)
```

```output
tick v
2
```

### Lazy locals declared together are evaluated together, once, by the first read of any.

```brix
(main
  (call run true)
  (call run false)
)

(fn-do + run _
  (param c Bool)
  (if c
    (print b)
  )
  (print (+ a b))
  (print x)
  (print (str y " " z))
  (blank)
  (lazy (tuple a b) (tuple (call tick "a" 1) (call tick "b" 2)))
  (lazy (tuple x y z) (tuple (call tick "x" 5) "s" 2.5))
)

(fn-do + tick _
  (param label String)
  (param value Int)
  (print (str "tick " label))
  (return value)
)
```

```output
tick a
tick b
2
3
tick x
5
s 2.5
tick a
tick b
3
tick x
5
s 2.5
```

### Lazy locals declared together from one value, read in a loop.

```brix
(main
  (print (call run 3))
)

(fn-do + run Int
  (param n Int)
  (let i 0)
  (let sum 0)
  (while (< i n)
    (+= sum a)
    (if (== i 1)
      (+= sum b)
    )
    (+= i 1)
  )
  (return sum)
  (blank)
  (lazy (tuple a b) (call pair n))
)

(fn-do + pair _
  (param n Int)
  (print "pair")
  (return (tuple n (* n 10)))
)
```

```output
pair
39
```

### Lazy locals declared together under an expression are evaluated only where it reads them.

```brix
(main
  (print (call run 3))
  (print (call run 0))
)

(fn-expr + run _
  (param n Int)
  (scope (if-expr (> n 1) (+ a b) 0)
    (lazy (tuple a b) (call pair n))
  )
)

(fn-do + pair _
  (param n Int)
  (print "pair")
  (return (tuple n (* n 10)))
)
```

```output
pair
33
0
```

### Lazy locals declared together in a method, from its properties.

```brix
(main
  (let thing (call Thing))
  (print (call (. thing sum)))
  (print (call (. thing sum)))
)

(type + Thing
  (prop + size _ (= 4))
  (fn-do + sum Int
    (if (> size 3)
      (return (+ w h))
    )
    (return 0)
    (blank)
    (lazy (tuple w h) (tuple size (* size 2)))
  )
)
```

```output
12
12
```

### Lazy locals declared together with types are converted to them.

```brix
(main
  (lazy (tuple (: foo Int) (: bar String) (: baz Float)) (tuple 1 "hi" 2.5))
  (print foo)
  (print bar)
  (print baz)
)
```

```output
1
hi
2.5
```

### A lazy local with a type and no value is the default of the type.

```brix
(main
  (print a)
  (blank)
  (lazy a Int)
)
```

```output
0
```

### A lazy local with no value under an expression.

```brix
(main
  (print (call foo))
)

(fn-expr + foo _
  (scope (+ a 0)
    (lazy a Int)
  )
)
```

```output
0
```

### A captured lazy local the function reads first is evaluated once, for it and for the closure.

```brix
(main
  (let fn (call make))
  (print (call fn))
  (print (call fn))
)

(fn-do + make _
  (print value)
  (return (=> () (+ value 1)))
  (blank)
  (lazy value (call tick "owner first" 3))
)

(fn-do + tick _
  (param label String)
  (param value Int)
  (print (str "tick " label))
  (return value)
)
```

```output
tick owner first
3
4
4
```

### A captured lazy local a closure reads first is evaluated once, for it and for the function.

```brix
(main
  (print (call make))
)

(fn-do + make Int
  (let get (=> () (+ value 1)))
  (let first (call get))
  (return (+ first value))
  (blank)
  (lazy value (call tick "closure first" 3))
)

(fn-do + tick _
  (param label String)
  (param value Int)
  (print (str "tick " label))
  (return value)
)
```

```output
tick closure first
7
```

### The value of a captured lazy local reads an argument that nothing else captures.

```brix
(main
  (let fn (call make 5))
  (print (call fn))
  (print (call fn))
  (let other (call make 7))
  (print (call other))
)

(fn-expr + make _
  (param n Int)
  (=> () (+ value 1))
  (where
    (lazy value (call tick "argument" (* n 2)))
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
tick argument
11
11
tick argument
15
```

### A captured lazy local a closure assigns before any read is never evaluated.

```brix
(main
  (let set (call make))
  (print (call set 2))
  (print (call set 3))
)

(fn-expr + make _
  (=> (n) :do
    (block
      (= total n)
      (return total)
    )
  )
  (where
    (lazy total (call tick "assigned" 0))
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
2
3
```

### A captured lazy local a closure assigns after a read keeps what was assigned.

```brix
(main
  (let fn (call make))
  (print (call fn 1))
  (print (call fn 2))
)

(fn-expr + make _
  (=> (n) :do
    (block
      (if (== n 2)
        (= value 100)
      )
      (return (+ value n))
    )
  )
  (where
    (lazy value (call tick "assigned later" 5))
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
tick assigned later
6
102
```

### The value of a captured lazy local reads another lazy local.

```brix
(main
  (let fn (call make))
  (print (call fn))
  (print (call fn))
)

(fn-expr + make _
  (=> () (+ a 1))
  (where
    (lazy a (* b 2))
    (lazy b (call tick "chain" 4))
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
tick chain
9
9
```

### A lazy local read two closures deep is evaluated once in a run of the function that declares it.

```brix
(main
  (let fn (call make))
  (let inner (call fn))
  (print (call inner))
  (print (call inner))
  (let again (call fn))
  (print (call again))
)

(fn-expr + make _
  (=> ()
    (block
      (=> () (+ value 1))
    )
  )
  (where
    (lazy value (call tick "two deep" 3))
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
tick two deep
4
4
4
```

### The value of a captured lazy local reads a property of the receiver.

```brix
(main
  (let thing (call Thing))
  (let fn (call (. thing make)))
  (print (call fn))
  (= (. thing size) 9)
  (print (call fn))
  (let next (call (. thing make)))
  (print (call next))
)

(type + Thing
  (prop + size _ (= 4))
  (fn-expr + make _
    (=> () (+ value 1))
    (where
      (lazy value (call tick "member" (* size 2)))
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
tick member
9
9
tick member
19
```

### The value of a captured lazy local calls a method of the receiver.

```brix
(main
  (let thing (call Thing))
  (let fn (call (. thing make)))
  (print (call fn))
  (print (call fn))
)

(type + Thing
  (prop + size _ (= 4))
  (fn-expr + make _
    (=> () (+ value 1))
    (where
      (lazy value (call helper 2))
    )
  )
  (fn-do + helper Int
    (param n Int)
    (print "helper")
    (return (* size n))
  )
)
```

```output
helper
9
9
```

### A closure in a method adds a captured lazy local to a property its value changes.

```brix
(main
  (let counter (call Counter))
  (let step (call (. counter stepper)))
  (print (call step))
  (print (call step))
  (print (. counter count))
)

(type + Counter
  (prop + count _ (= 0))
  (fn-expr + stepper _
    (=> () :do
      (block
        (+= count base)
        (return count)
      )
    )
    (where
      (lazy base (call bump 10))
    )
  )
  (fn-do + bump Int
    (param n Int)
    (+= count 1)
    (print "bump")
    (return n)
  )
)
```

```output
bump
10
20
20
```

### The value of a captured lazy local reads a local as it is when the value is evaluated.

```brix
(main
  (print (call make))
)

(fn-do + make Int
  (let n 1)
  (let get (=> () (+ value 1)))
  (= n 5)
  (return (+ (call get) n))
  (blank)
  (lazy value (call tick "later" (* n 10)))
)

(fn-do + tick _
  (param label String)
  (param value Int)
  (print (str "tick " label))
  (return value)
)
```

```output
tick later
56
```

### A captured lazy local whose value reads an optional argument.

```brix
(main
  (print (call make 4))
  (print (call make null))
)

(fn-do + make Int
  (param n (? Int))
  (if n
    (let get (=> () (+ value 1)))
    (return (call get))
  )
  (return 0)
  (blank)
  (lazy value (call tick "narrowed" (* (?? n 7) 10)))
)

(fn-do + tick _
  (param label String)
  (param value Int)
  (print (str "tick " label))
  (return value)
)
```

```output
tick narrowed
41
0
```

### Captured lazy locals of text, a tuple and an object, read after many allocations.

```brix
(main
  (let fn (call make "a"))
  (let junk (list 1))
  (let i 0)
  (while (< i 20000)
    (= junk (list i (+ i 1) (+ i 2)))
    (+= i 1)
  )
  (print (call fn))
  (print (call fn))
  (print (. junk length))
)

(fn-expr + make _
  (param prefix String)
  (=> () (str text "/" (+ (. point x) (. point y)) "/" (. thing size)))
  (where
    (lazy text (call build prefix))
    (lazy point (call Point 3 4))
    (lazy thing (call Thing))
  )
)

(type + Point (is (tuple (: x Int) (: y Int))))

(type + Thing
  (prop + size _ (= 9))
)

(fn-do + build String
  (param prefix String)
  (print "build")
  (return (str prefix "-built"))
)
```

```output
build
a-built/7/9
a-built/7/9
3
```

### A captured lazy local with no value is the default of its type.

```brix
(main
  (let add (call make))
  (print (call add 2))
  (print (call add 3))
)

(fn-expr + make _
  (=> (n) :do
    (block
      (+= total n)
      (return total)
    )
  )
  (where
    (lazy total Int)
  )
)
```

```output
2
5
```

### A captured lazy local with a type is converted to it.

```brix
(main
  (let get (call make))
  (print (call get))
)

(fn-expr + make _
  (=> () (/ value 2))
  (where
    (lazy value Float (call tick "typed" 3))
  )
)

(fn-do + tick _
  (param label String)
  (param value I32)
  (print (str "tick " label))
  (return value)
)
```

```output
tick typed
1.5
```

### A captured lazy local that is an optional holding nothing is evaluated once.

```brix
(main
  (let fn (call make))
  (print (call fn))
  (print (call fn))
)

(fn-expr + make _
  (=> () (?? value -1))
  (where
    (lazy value (? Int) (call maybe))
  )
)

(fn-do + maybe (? Int)
  (print "maybe")
  (return null)
)
```

```output
maybe
-1
-1
```

### A closure that reads a captured lazy local, called in a loop.

```brix
(main
  (print (call run 3))
)

(fn-do + run Int
  (param n Int)
  (let sum 0)
  (let i 0)
  (let add
    (=> () :do
      (block
        (+= sum value)
        (return sum)
      )
    )
  )
  (while (< i n)
    (call add)
    (+= i 1)
  )
  (return sum)
  (blank)
  (lazy value (call tick "loop" 5))
)

(fn-do + tick _
  (param label String)
  (param value Int)
  (print (str "tick " label))
  (return value)
)
```

```output
tick loop
15
```

### A captured lazy local the function reads on one path only.

```brix
(main
  (print (call make true))
  (print (call make false))
)

(fn-do + make Int
  (param early Bool)
  (let get (=> () (+ value 1)))
  (if early
    (print value)
  )
  (return (call get))
  (blank)
  (lazy value (call tick "branch" 5))
)

(fn-do + tick _
  (param label String)
  (param value Int)
  (print (str "tick " label))
  (return value)
)
```

```output
tick branch
5
6
tick branch
6
```

### A captured lazy local of a closure that is never called is never evaluated.

```brix
(main
  (let fn (call make))
  (print 1)
)

(fn-expr + make _
  (=> () (+ value 1))
  (where
    (lazy value (call tick "never" 5))
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
1
```

### Captured lazy locals whose values are a list's length and a conditional.

```brix
(main
  (let fn (call make 3))
  (print (call fn))
  (print (call fn))
)

(fn-expr + make _
  (param n Int)
  (=> () (+ value it-total))
  (where
    (lazy value (. (list n (+ n 1) (+ n 2)) length))
    (lazy it-total (if-expr (> n 2) (* n 2) 0))
  )
)
```

```output
9
9
```

### A panic raised by the value of a captured lazy local, in the closure that reads it.

```brix
(main
  (do-new
    (let fn (call make))
    (print (call fn))
    (on-failure failure
      (print failure)
    )
  )
)

(fn-expr + make _
  (=> () (+ value 1))
  (where
    (lazy value (call fail "lazy"))
  )
)

(fn-do + fail Int
  (param label String)
  (throw (call Panic (str "failed " label)))
)
```

```output
failed lazy
```

### A lazy local declared in a loop body has a fresh lifetime for each iteration, also when its first read is inside a branch.

```brix
(main
  (for (in title (list "a" "b" "c"))
    (lazy shown (call tick title))
    (if (== title "b")
      (print shown)
    )
    (print (str shown "+"))
  )
)

(fn-do + tick String
  (param text String)
  (print (str "tick " text))
  (return (str text "!"))
)
```

```output
tick a
a!+
tick b
b!
b!+
tick c
c!+
```

### Lazy tuple destructuring in a loop body evaluates its initializer once per iteration.

```brix
(main
  (for (in title (list "a" "b"))
    (lazy (tuple first second) (call pair title))
    (print first)
    (print second)
  )
)

(fn-do + pair _
  (param text String)
  (print (str "pair " text))
  (return (tuple (str text "1") (str text "2")))
)
```

```output
pair a
a1
a2
pair b
b1
b2
```

### A lazy local declared in a loop body and read in a nested loop evaluates once per iteration of the loop that declares it.

```brix
(main
  (for (in title (list "a" "b"))
    (lazy shown (call tick title))
    (for (in index (..< 0 2))
      (print (str index " " shown))
    )
  )
)

(fn-do + tick String
  (param text String)
  (print (str "tick " text))
  (return (str text "!"))
)
```

```output
tick a
0 a!
1 a!
tick b
0 b!
1 b!
```

### A lazy local declared in a while loop body has a fresh lifetime for each iteration.

```brix
(main
  (let index 0)
  (while (< index 2)
    (lazy shown (call tick (str "row " index)))
    (if (== index 1)
      (print shown)
    )
    (print (str shown "+"))
    (+= index 1)
  )
)

(fn-do + tick String
  (param text String)
  (print (str "tick " text))
  (return (str text "!"))
)
```

```output
tick row 0
row 0!+
tick row 1
row 1!
row 1!+
```

### A lazy local declared in a loop body and read both directly and by a closure created in that iteration is evaluated once per iteration and shared between the two.

```brix
(main
  (for (in title (list "a" "b" "c"))
    (lazy shown (call tick title))
    (if (== title "b")
      (print shown)
    )
    (let fn (=> () shown))
    (print (call fn))
    (print (call fn))
  )
)

(fn-do + tick String
  (param text String)
  (print (str "tick " text))
  (return (str text "!"))
)
```

```output
tick a
a!
a!
tick b
b!
b!
b!
tick c
c!
c!
```

### A guard written before a function's value reads a lazy local declared under the value.

```brix
(main
  (print (catch (call pick 1)))
  (print (catch (call pick 5)))
)

(fn-expr + pick String (?! Error)
  (param id U32)
  (before
    (throw (call Error "too big") (if (not legal)))
  )
  (scope (str "fine " id)
    (lazy legal
      (match id
        (case= 1
          true
        )
        (default
          false
        )
      )
    )
  )
)
```

```output
fine 1
too big
```

### A lazy local declared under a function's value, read by a guard before the value and by the value, is computed once.

```brix
(main
  (print (catch (call scaled 3)))
  (print (catch (call scaled 1)))
)

(fn-expr + scaled Int (?! Error)
  (param n Int)
  (before
    (throw (call Error "too small") (if (not legal)))
  )
  (scope (if-expr legal (* n 10) 0)
    (lazy legal (call checked n))
  )
)

(fn-do + checked Bool
  (param n Int)
  (print (str "checking " n))
  (return (> n 2))
)
```

```output
checking 3
30
checking 1
too small
```

### A lazy local beside a stream match, read by two of its arms, is computed by the arm that runs.

```brix
(main
  (print (call run "b"))
  (print (call run "a"))
)

(fn-do + run String
  (param text String)
  (return
    (match text
      (~ "a"
        (str "first " label)
      )
      (~ "b"
        (str "second " label)
      )
      (default
        (inline "none")
      )
      (blank)
      (lazy label (call make-label))
    )
  )
)

(fn-do + make-label String
  (print "computing")
  (return "L")
)
```

```output
computing
second L
computing
first L
```

### A lazy local declared twice in one block is diagnosed.

```brix crash
(main
  (print (call pick 1))
)

(fn-expr + pick String
  (param id U32)
  (scope (str "fine " a)
    (lazy a (+ id 1))
    (lazy a (+ id 2))
  )
)
```

```output
Exception: Variable is already defined
<no file:10>
        (lazy a (+ id 2))
              ^
```

### A lazy local read twice may narrow an optional by a test its own value makes.

```brix
(main
  (print (call pick "fallback" 1 null))
  (print (call pick "fallback" 1 "chosen"))
  (print (call pick "fallback" 2 "chosen"))
  (print (call pick "" 1 null))
)

(fn-do + pick String
  (param value String)
  (param index U32)
  (param preferred (? String))
  (let shown
    (scope (only-if (str "<" text ">") (if (. text length)))
      (lazy text (if-expr (and preferred (== index 1)) preferred value))
    )
  )
  (return (?? shown "empty"))
)
```

```output
<fallback>
<chosen>
<fallback>
empty
```

### A lazy local read twice may narrow, by its own test, a lazy local it is the first to read.

```brix
(main
  (print (call label 3))
  (print (call label -1))
)

(fn-expr _ find (? Int)
  (param n Int)
  (only-if n (if (> n 0)))
)

(fn-expr _ label _
  (param n Int)
  (case? record
    (str "found " record)
  )
  (default
    "none"
  )
  (where
    (lazy found (call find n))
    (lazy record (only-if (* found 2) (if found)))
  )
)
```

```output
found 6
none
```

### A lazy attached to a function's result is visible to the statements above the result.

```brix
(main
  (print (call checked 5))
  (print (?? (catch? (call checked 0)) -1))
)

(type + CheckError (is Error))

(fn-expr + checked Int (?! CheckError)
  (param n Int)
  (before
    (throw (call CheckError "illegal") (if (not legal)))
  )
  (scope n
    (lazy legal (> n 0))
  )
)
```

```output
5
-1
```

### A lazy local whose value raises, first read in a comprehension, is evaluated there, and its error leaves the function that declares it.

```brix
(main
  (print (?? (catch? (call listed 3)) "failed"))
  (print (?? (catch? (call listed -1)) "failed"))
)

(fn-do + check Int (?! Error)
  (param n Int)
  (print "check")
  (throw (call Error "negative") (if (< n 0)))
  (return n)
)

(fn-do + note Int
  (param n Int)
  (print (str "item " n))
  (return n)
)

(fn-do + listed String (?! Error)
  (param n Int)
  (print "before")
  (let a (. items length))
  (print "after")
  (return (str a))
  (blank)
  (lazy checked (call check n))
  (lazy items
    (block
      (spread (list 1 2))
      (pipe *> (* (call note each) checked))
      (pipe |> List)
    )
  )
)
```

```output
before
item 1
check
item 2
after
2
before
item 1
check
failed
```

### A lazy local holding a closable value, first read in the content of a markup element, is closed when the element closes.

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let numbers (of AsyncChannel Int))
  (lazy source numbers)
  (blank)
  (<> Label (id: shown)
    (str "closed " (. source is-closed))
  )
  (blank)
  (print (. shown text))
  (call (. shown close))
  (print (. numbers is-closed))
)
```

```output
closed false
true
```

### A lazy local whose value pipes a list through a function, read by a list of feeds, is evaluated where the list reads it, and the function reads the arguments of the one that declares the local.

```brix
(main
  (print (call matches (list "a" "bb" "c" "b") "b" true))
  (print (call matches (list "a" "bb" "c" "b") "b" false))
)

(fn-expr _ matches _
  (param keys (list String))
  (param needle String)
  (param more Bool)
  (call (of List String)
    (block
      (spread found)
      (spread? (only-if others (if more)))
    )
  )
  (where
    (lazy found
      (block
        (spread keys)
        (pipe ?> (call (. each starts-with) needle))
      )
    )
    (lazy others
      (block
        (spread keys)
        (pipe ?> (not (call (. each starts-with) needle)))
      )
    )
  )
)
```

```output
bb, b, a, c
bb, b
```

### The value of a lazy local read under more than one flow, where what a test in the value itself finds of a local holds wherever the value is evaluated.

```brix
(main
  (let finder (call Finder))
  (print (call (. finder hit) 2))
  (print (call (. finder hit) 0))
  (print (call (. finder hit) -1))
)

(type + Box
  (prop + size Index)
  (fn-do + This _
    (param size _)
  )
)

(type + Pick (is Tuple)
  (prop + segment Index)
  (prop + parameter Index)
)

(type + Hit (is Tuple)
  (prop + shape (? Box) (= null))
  (prop + segment Index (= 0))
  (prop + parameter Index (= 0))
  (prop + leaf (? Box) (= null))
)

(type + Doc
  (fn-expr + has Bool
    (param box Box)
    (> (. box size) 5)
  )
  (fn-expr + pick-in (? Pick)
    (param box Box)
    (only-if (call Pick (. box size) 2) (if (> (. box size) 25)))
  )
)

(type + Finder
  (prop + boxes (list Box) (= (list (call Box 1) (call Box 20) (call Box 30))))
  (prop + document _ (= (call Doc)))
  (fn-do + box-at (? Box)
    (param at I64)
    (return-if null (if (< at 0)))
    (return ([] boxes (as at Index)))
  )
  (fn-expr + has Bool
    (param box Box)
    (> (. box size) 5)
  )
  (fn-expr + pick-in (? Pick)
    (param box Box)
    (only-if (call Pick (. box size) 2) (if (> (. box size) 25)))
  )
  (fn-do + hit String
    (param at I64)
    (let found (call node-hit at))
    (return (str (?? (?. (. found shape) size) 0) " " (. found segment) " " (. found parameter) " " (?? (?. (. found leaf) size) 0)))
  )
  (fn-expr + node-hit Hit
    (param at I64)
    (case? (not leaf)
      (call Hit)
    )
    (case? pick
      (call Hit shape (. pick segment) (. pick parameter) leaf)
    )
    (default
      (call Hit shape 0 0 leaf)
    )
    (where
      (lazy leaf (call box-at at))
      (lazy shape (only-if leaf (if (and leaf (call (. document has) leaf)))))
      (lazy pick (only-if (call (. document pick-in) shape) (if shape)))
    )
  )
)
```

```output
30 30 2 30
0 0 0 1
0 0 0 0
```

### Lazy locals nested in the lazy local of an arm are evaluated where the arm reads them.

```brix
(main
  (for (in item (call paths ".abc"))
    (print (str ([] item 0) " " ([] item 1)))
  )
  (for (in item (call paths ".abcdefg"))
    (print (str ([] item 0) " " ([] item 1)))
  )
  (for (in item (call paths "plain"))
    (print (str ([] item 0) " " ([] item 1)))
  )
)

(fn-expr _ paths _
  (param name String)
  (case? (call (. name starts-with) ".")
    (item (tuple full-name 1))
    (item (tuple (str full-name "!") 2))
    (blank)
    (lazy full-name
      (scope (?? local-name default-name)
        (lazy local-name (only-if (str "L" name) (if (> (. name length) 5))))
        (lazy default-name
          (scope (++ base name)
            (lazy base "D")
          )
        )
      )
    )
  )
  (default
    (item (tuple name 3))
  )
)
```

```output
D.abc 1
D.abc! 2
L.abcdefg 1
L.abcdefg! 2
plain 3
```

### A caught read of a lazy property of a class converted to an interface but never constructed is emitted, and never runs.

```brix
(main
  (let holder (? Holder) null)
  (let proto (? Proto) holder)
  (print (?? (call (?. proto bar)) -1))
  (print (?? (call (?. holder read)) -1))
)

(type + Proto (is Interface)
  (fn-expr + bar Int)
)

(fn-do + checked Int (?! Error)
  (param value Int)
  (throw (call Error "bad") (if (< value 0)))
  (return value)
)

(type + Holder
  (implements Proto)
  (prop + value _ (:= (call checked 3)))
  (fn-expr + bar _
    7
  )
  (fn-do + read (? Int)
    (return (catch? value))
  )
)
```

```output
-1
-1
```

### A lazy local whose value reads a property its own value types holds wherever it is read, so an argument the flow narrows before the read is read narrowed in the value.

```brix
(main
  (let holder (call Holder))
  (print (call (. holder note) "a"))
  (print (call (. holder note) "b"))
  (print (call (. holder note) null))
)

(type + Holder
  (prop _ table _ (= (call (of Map String String))))
  (fn-do + This _
    (= ([] table "a") "found a")
  )
  (fn-expr + note _
    (param name (? String))
    (case? (not name)
      "none"
    )
    (case? found
      found
    )
    (default
      name
    )
    (where
      (lazy found ([] table name))
    )
  )
)
```

```output
found a
b
none
```

### A lazy local read again where every way passes an earlier read has its value there, so the later read does not type the value again after a getter the first read called.

```brix
(main
  (print (call (. (call Holder (call Thing)) check)))
  (print (call (. (call Holder null) check)))
)

(type + Owner (is Interface)
  (fn-expr + level (? Int) :get)
)

(type + Thing
  (implements Owner)
  (fn-expr + level (? Int) :get
    7
  )
)

(type + Holder
  (prop + owner (? Owner))
  (fn-do + This _
    (param owner _)
  )
  (fn-expr + check Int
    (case? (not owner)
      0
    )
    (case? (not level)
      1
    )
    (default
      level
    )
    (where
      (lazy level (. owner level))
    )
  )
)
```

```output
7
0
```

### A lazy local read in a branch of another lazy local's value, and again outside it, takes what its value tests as its type, which holds at both reads.

```brix
(main
  (print (call f 1 10 4))
  (print (call f 0 10 4))
)

(fn-expr + f Int
  (param side Int)
  (param track Int)
  (param size Int)
  (+ x width)
  (where
    (lazy align (if-expr (> side 0) side null))
    (lazy width (if-expr (!= align null) size track))
    (lazy x (if-expr (!= align null) (- track width) 0))
  )
)
```

```output
10
10
```

### A lazy local read in an arm a test narrows what its value tests, and again in another arm, takes what its value tests as its type.

```brix
(main
  (print (call f 1 10 4))
  (print (call f 0 10 4))
)

(fn-expr + f Int
  (param side Int)
  (param track Int)
  (param size Int)
  (case? (!= align null)
    (- track width)
  )
  (default
    width
  )
  (where
    (lazy align (if-expr (> side 0) side null))
    (lazy width (if-expr (!= align null) size track))
  )
)
```

```output
6
10
```

### A lazy local whose value reads a field a guard narrowed, read in a later guard's call and again after it, is evaluated once, with the field as the guard left it.

```brix
(main
  (print (call (. (call Holder (call Address 3)) check)))
  (print (call (. (call Holder (call Address 0)) check)))
  (print (call (. (call Holder (call Address 9)) check)))
  (print (call (. (call Holder null) check)))
)

(type + Address
  (prop + limit Int)
  (fn-do + This _
    (param limit _)
  )
  (fn-expr + is-live Bool
    (param n Int)
    (< n limit)
  )
)

(type + Holder
  (prop + keyed (? Address))
  (fn-do + This _
    (param keyed _)
  )
  (fn-expr + check Int
    (case? (not keyed)
      0
    )
    (case? (not (call (. address is-live) 1))
      1
    )
    (case? (> (. address limit) 5)
      2
    )
    (default
      (. address limit)
    )
    (where
      (lazy address keyed)
    )
  )
)
```

```output
3
1
2
0
```

### Forcing a field a guard narrowed is illegal in the value of a lazy local whose later reads each come after an earlier one, which evaluates it there.

```brix compile-error
(main
  (print (call (. (call Holder (call Address 3)) check)))
)

(type + Address
  (prop + limit Int)
  (fn-do + This _
    (param limit _)
  )
  (fn-expr + is-live Bool
    (param n Int)
    (< n limit)
  )
)

(type + Holder
  (prop + keyed (? Address))
  (fn-do + This _
    (param keyed _)
  )
  (fn-expr + check Int
    (case? (not keyed)
      0
    )
    (case? (not (call (. address is-live) 1))
      1
    )
    (default
      (. address limit)
    )
    (where
      (lazy address (!! keyed))
    )
  )
)
```

```output
Exception: "keyed" is Address, which is never null here, so it needs no "!!"
<no file:33>
          (lazy address (!! keyed))
                        ^^^^^^^^^^
```

### A lazy local read twice in a straight line is typed once, though an assignment between the reads changes what is known of a local its value reads.

```brix
(main
  (print (call f 5))
  (print (call f null))
)

(fn-do + f Int
  (param given (? Int))
  (let c given)
  (return-if 0 (if (== c null)))
  (let a doubled)
  (= c null)
  (return (+ a doubled))
  (blank)
  (lazy doubled (* c 2))
)
```

```output
20
0
```

### A lazy local whose value reads a field a guard narrowed is evaluated at its first read, and a read of it in another lazy local's value, after a call, finds it evaluated there: the value is not typed again where the call has ended what is known of the field.

```brix
(main
  (print (call (. (call Holder (call Layout (call Request "a"))) serves) "a"))
  (print (call (. (call Holder (call Layout (call Request "b"))) serves) "a"))
  (print (call (. (call Holder null) serves) "a"))
)

(type + Request
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(type + Layout
  (prop + request Request)
  (fn-do + This _
    (param request _)
  )
)

(type + Holder
  (prop + layout (? Layout))
  (prop + count _ (= 0))
  (fn-do + This _
    (param layout _)
  )
  (fn-expr + matches Bool
    (== count 0)
  )
  (fn-expr + serves Bool
    (param text String)
    (case? (or (not layout) (not (call matches)))
      false
    )
    (default
      (and (== (. request text) text) same)
    )
    (where
      (lazy request (. layout request))
      (lazy same (== (. request text) text))
    )
  )
)
```

```output
true
false
false
```

### Forcing that field in the value is illegal, as the value is typed only where the guard left the field known.

```brix compile-error
(main
  (print (call (. (call Holder (call Layout (call Request "a"))) serves) "a"))
)

(type + Request
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(type + Layout
  (prop + request Request)
  (fn-do + This _
    (param request _)
  )
)

(type + Holder
  (prop + layout (? Layout))
  (prop + count _ (= 0))
  (fn-do + This _
    (param layout _)
  )
  (fn-expr + matches Bool
    (== count 0)
  )
  (fn-expr + serves Bool
    (param text String)
    (case? (or (not layout) (not (call matches)))
      false
    )
    (default
      (and (== (. request text) text) same)
    )
    (where
      (lazy request (. (!! layout) request))
      (lazy same (== (. request text) text))
    )
  )
)
```

```output
Exception: "layout" is Layout, which is never null here, so it needs no "!!"
<no file:38>
          (lazy request (. (!! layout) request))
                           ^^^^^^^^^^^
```

### A lazy local first read in an assertion's condition has its value after the assertion, so what the condition found of a property of it holds at a later read.

```brix
(main
  (print (call require (call Store (list (call Entry 1 5))) 0 1))
  (print (call require (call Store (list (call Entry 1 5) (call Entry 2 7))) 1 2))
)

(type + Entry
  (prop + generation Int)
  (prop + geometry (? Int))
  (fn-do + This _
    (param generation _)
    (param geometry _)
  )
)

(type + Store
  (prop + entries (list Entry))
  (fn-do + This _
    (param entries _)
  )
)

(fn-expr + require Int
  (param store Store)
  (param index Index)
  (param generation Int)
  (before
    (throw (call Panic "stale") (if (or (!= (. entry generation) generation) (not (. entry geometry)))))
  )
  (. entry geometry)
  (where
    (lazy entry ([] (. store entries) index))
  )
)
```

```output
5
7
```

### A lazy local whose value reads a field a guard narrowed, first read in another lazy local's value, is typed again at a later read that no evaluation of it comes before, under what is known there: a call between them has ended what is known of the field, so it is read from an optional.

```brix compile-error
(main
  (print (call (. (call Holder (call Layout (call Request "a"))) serves) "a"))
)

(type + Request
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(type + Layout
  (prop + request Request)
  (fn-do + This _
    (param request _)
  )
)

(type + Holder
  (prop + layout (? Layout))
  (prop + count _ (= 0))
  (fn-do + This _
    (param layout _)
  )
  (fn-expr + matches Bool
    (== count 0)
  )
  (fn-expr + serves Bool
    (param text String)
    (case? (not layout)
      false
    )
    (default
      (and same (== (. request text) text))
    )
    (where
      (lazy request (. layout request))
      (lazy same (and (> (. request text length) 0) (call matches)))
    )
  )
)
```

```output
Exception: Property "request" is read from Layout?, which may be null; unwrap it first with "(!! …)" or "(?. …)"
<no file:38>
          (lazy request (. layout request))
                        ^^^^^^^^^^^^^^^^^^
```

### Forcing that field in the value is legal, as the value is typed again where the field may hold nothing.

```brix
(main
  (print (call (. (call Holder (call Layout (call Request "a"))) serves) "a"))
  (print (call (. (call Holder null) serves) "a"))
)

(type + Request
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(type + Layout
  (prop + request Request)
  (fn-do + This _
    (param request _)
  )
)

(type + Holder
  (prop + layout (? Layout))
  (prop + count _ (= 0))
  (fn-do + This _
    (param layout _)
  )
  (fn-expr + matches Bool
    (== count 0)
  )
  (fn-expr + serves Bool
    (param text String)
    (case? (not layout)
      false
    )
    (default
      (and same (== (. request text) text))
    )
    (where
      (lazy request (. (!! layout) request))
      (lazy same (and (> (. request text length) 0) (call matches)))
    )
  )
)
```

```output
true
false
```

### A local the value reads, narrowed where the value is first typed and assigned again before a later read that types it again, is read there as its type.

```brix compile-error
(main
  (print (call (. (call Holder (call Layout (call Request "a"))) serves) (call Layout (call Request "a")) "a"))
)

(type + Request
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(type + Layout
  (prop + request Request)
  (fn-do + This _
    (param request _)
  )
)

(type + Holder
  (prop + layout (? Layout))
  (prop + count _ (= 0))
  (fn-do + This _
    (param layout _)
  )
  (fn-expr + matches Bool
    (== count 0)
  )
  (fn-do + serves Bool
    (param given (? Layout))
    (param text String)
    (let found given)
    (return-if false (if (not found)))
    (let first (and (!= text "") same))
    (= found given)
    (return (and first (== (. request text) text)))
    (blank)
    (lazy request (. found request))
    (lazy same (> (. request text length) 0))
  )
)
```

```output
Exception: Property "request" is read from Layout?, which may be null; unwrap it first with "(!! …)" or "(?. …)"
<no file:38>
        (lazy request (. found request))
                      ^^^^^^^^^^^^^^^^^
```
