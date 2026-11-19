# Loop closures

### Closures created in a for loop capture that iteration's item

```brix
(main
  (let seen (call (of List String)))
  (let callbacks (call (of List Doer)))
  (for (in item (call (of List String) (list "a" "b" "c")))
    (+= callbacks (=> () :do (+= seen item)))
  )
  (for (in callback callbacks)
    (call callback)
  )
  (print (call (. ", " join) seen))
)
```

```output
a, b, c
```

### Closures created in an integer range loop capture that iteration's index

```brix
(main
  (let seen (call (of List String)))
  (let callbacks (call (of List Doer)))
  (for (in index (..< 0 3))
    (+= callbacks (=> () :do (+= seen (str index))))
  )
  (for (in callback callbacks)
    (call callback)
  )
  (print (call (. ", " join) seen))
)
```

```output
0, 1, 2
```

### Locals declared in a loop body are fresh for each iteration

```brix
(main
  (let seen (call (of List String)))
  (let callbacks (call (of List Doer)))
  (for (in index (..< 0 3))
    (let label (str "item " index))
    (+= callbacks (=> () :do (+= seen label)))
  )
  (for (in callback callbacks)
    (call callback)
  )
  (print (call (. ", " join) seen))
)
```

```output
item 0, item 1, item 2
```

### While loop body locals are fresh for each iteration

```brix
(main
  (let seen (call (of List String)))
  (let callbacks (call (of List Doer)))
  (let count 0)
  (while (< count 3)
    (let snapshot (* count 10))
    (+= callbacks (=> () :do (+= seen (str snapshot))))
    (+= count 1)
  )
  (for (in callback callbacks)
    (call callback)
  )
  (print (call (. ", " join) seen))
)
```

```output
0, 10, 20
```

### Nested loops keep separate bindings for inner and outer items

```brix
(main
  (let seen (call (of List String)))
  (let callbacks (call (of List Doer)))
  (for (in outer (call (of List String) (list "x" "y")))
    (for (in inner (..< 0 2))
      (+= callbacks (=> () :do (+= seen (str outer inner))))
    )
  )
  (for (in callback callbacks)
    (call callback)
  )
  (print (call (. ", " join) seen))
)
```

```output
x0, x1, y0, y1
```

### Variables declared outside a loop are still shared by reference

```brix
(main
  (let total 0)
  (let callbacks (call (of List Doer)))
  (for (in index (.. 1 3))
    (+= callbacks (=> () :do (+= total index)))
  )
  (= total 100)
  (for (in callback callbacks)
    (call callback)
  )
  (print total)
)
```

```output
106
```

### A closure shares its iteration's binding with the rest of that iteration

```brix
(main
  (let seen (call (of List String)))
  (let callbacks (call (of List Doer)))
  (for (in index (..< 0 2))
    (let label "early")
    (+= callbacks (=> () :do (+= seen label)))
    (= label (str "late " index))
  )
  (for (in callback callbacks)
    (call callback)
  )
  (print (call (. ", " join) seen))
)
```

```output
late 0, late 1
```

### Mutations made by a loop closure are visible to its iteration

```brix
(main
  (let seen (call (of List String)))
  (for (in index (..< 0 2))
    (let count index)
    (let bump (=> () :do (+= count 10)))
    (call bump)
    (+= seen (str count))
  )
  (print (call (. ", " join) seen))
)
```

```output
10, 11
```

### Nested closures reach loop bindings through their parent closure

```brix
(main
  (let seen (call (of List String)))
  (let makers (call (of List Doer)))
  (for (in item (call (of List String) (list "p" "q")))
    (+= makers
      (=> () :do
        (block
          (let inner (=> () :do (+= seen item)))
          (call inner)
        )
      )
    )
  )
  (for (in maker makers)
    (call maker)
  )
  (print (call (. ", " join) seen))
)
```

```output
p, q
```

### Loop closures inside methods capture this alongside per-iteration items

```brix
(main
  (let collector (call Collector "tag"))
  (call (. collector collect))
  (print (call (. ", " join) (. collector seen)))
)

(type + Collector
  (prop + prefix String)
  (prop + seen _ (= (call (of List String))))
  (fn-do + This _
    (param prefix _)
  )
  (fn-do + collect _
    (let callbacks (call (of List Doer)))
    (for (in index (..< 0 3))
      (+= callbacks (=> () :do (+= seen (str prefix index))))
    )
    (for (in callback callbacks)
      (call callback)
    )
  )
)
```

```output
tag0, tag1, tag2
```

### Markup loop children capture their own item

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let seen (call (of List String)))
  (blank)
  (<> Box (id: root)
    (for (in item (call (of List String) (list "a" "b" "c")))
      (<> Action
        (= label item)
        (= run (=> () :do (+= seen item)))
      )
    )
  )
  (blank)
  (call walk root seen)
  (print (call (. ", " join) seen))
)

(fn-do + walk _ (?! Error)
  (param box Box)
  (param seen (list String))
  (for (in child (. box children))
    (if (is child Action)
      (let action (as child Action))
      (let callback (. action run))
      (+= seen (str (. action label) ":"))
      (call callback)
      (else
        (call walk child seen)
      )
    )
  )
)

(type + Action (is Box)
  (prop + label _ (= ""))
  (prop + run Doer)
)
```

```output
a:, a, b:, b, c:, c
```

### Loop closures keep their binding when a generator consumer is fused and exits early

```brix
(main
  (let callbacks (call (of List Doer)))
  (let seen (call (of List String)))
  (for (in value (call numbers))
    (+= callbacks (=> () :do (+= seen (str value))))
    (if (== value 2)
      (break)
    )
  )
  (for (in callback callbacks)
    (call callback)
  )
  (print (call (. ", " join) seen))
  (print (call latest))
)

(fn-do + numbers _ @inline
  (yield 1)
  (yield 2)
  (yield 3)
)

(fn-do + latest Int
  (let last (of Getter Int) (=> () 0))
  (for (in value (call numbers))
    (= last (=> () value))
    (break)
  )
  (return (call last))
)
```

```output
1, 2
1
```

### Markup handlers in a loop read their own element through its name, made each turn

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let seen (call (of List String)))
  (let root (call Host seen))
  (call fire root)
  (print (call (. ", " join) seen))
)

(fn-do + fire _
  (param box Box)
  (for (in child (. box children))
    (if (is child Action)
      (<- (. (as child Action) clicked) 1)
      (else
        (call fire child)
      )
    )
  )
)

(type + Host (is Box)
  (prop + seen (list String))
  (prop _ items _ (= (call (of List String) (list "a" "b" "c"))))
  (fn-do + This _
    (param seen _)
  )
  (content
    (<> This
      (for (in item items)
        (<> Action (id: action)
          (= label item)
          (for (on event clicked)
            (call record (. action label))
          )
        )
      )
    )
  )
  (fn-do + record _
    (param text String)
    (+= seen text)
  )
)

(type + Action (is Box)
  (prop + label _ (= ""))
  (event + clicked Int)
)
```

```output
a, b, c
```

### Generator closures capture range indexes and loop items

```brix
(main
  (let seen (call (of List String)))
  (for (in value (call numbers))
    (+= seen (str value))
  )
  (print (call (. ", " join) seen))
)

(fn-do + numbers _
  (let out (call (of List String)))
  (let label "x")
  (let f (=> () :do (+= out label)))
  (call f)
  (for (in index (..< 0 2))
    (let g (=> () :do (+= out (str index))))
    (call g)
  )
  (for (in item (call (of List String) (list "p")))
    (let h (=> () :do (+= out item)))
    (call h)
  )
  (for (in item out)
    (yield item)
  )
)
```

```output
x, 0, 1, p
```

### Generator loop closures keep per-iteration bindings after later iterations

```brix
(main
  (let seen (call (of List String)))
  (for (in value (call numbers))
    (+= seen value)
  )
  (print (call (. ", " join) seen))
)

(fn-do + numbers _
  (let callbacks (call (of List Doer)))
  (let out (call (of List String)))
  (for (in index (..< 0 3))
    (+= callbacks (=> () :do (+= out (str index))))
  )
  (for (in callback callbacks)
    (call callback)
  )
  (for (in item out)
    (yield item)
  )
)
```

```output
0, 1, 2
```

### Closures yielded from generator loops keep their iteration's locals

```brix
(main
  (let seen (call (of List String)))
  (for (in getter (call makers))
    (+= seen (call getter))
  )
  (print (call (. ", " join) seen))
)

(fn-do + makers _
  (for (in index (..< 0 3))
    (let label (str "m" index))
    (let getter (of Getter String) (=> () label))
    (yield getter)
  )
)
```

```output
m0, m1, m2
```

### Generator loops that yield between closures keep nested bindings and shared outer variables

```brix
(main
  (let seen (call (of List String)))
  (for (in text (call mixed))
    (+= seen text)
  )
  (print (call (. ", " join) seen))
)

(fn-do + mixed _
  (let callbacks (call (of List Doer)))
  (let out (call (of List String)))
  (let shared "a")
  (for (in outer (call (of List String) (list "x" "y")))
    (for (in inner (..< 0 2))
      (+= callbacks (=> () :do (+= out (str outer inner shared))))
      (yield (str "yield " outer inner))
    )
  )
  (= shared "b")
  (for (in callback callbacks)
    (call callback)
  )
  (for (in text out)
    (yield text)
  )
)
```

```output
yield x0, yield x1, yield y0, yield y1, x0b, x1b, y0b, y1b
```

### Inline generators with loop closures keep per-iteration bindings

```brix
(main
  (let seen (call (of List String)))
  (for (in getter (call labels))
    (+= seen (call getter))
  )
  (print (call (. ", " join) seen))
)

(fn-do + labels _ @inline
  (for (in index (..< 0 3))
    (let getter (of Getter String) (=> () (str "i" index)))
    (yield getter)
  )
)
```

```output
i0, i1, i2
```

### A closure written in a turn of a `while` loop reads the local of that turn, not of the last.

```brix
(main
  (let count 0)
  (let first (of Getter Int) (=> () -1))
  (while (< count 3)
    (let snapshot (* count 10))
    (if (== count 0)
      (= first (=> () snapshot))
    )
    (+= count 1)
  )
  (print (call first))
)
```

```output
0
```

### Closures written in turns of loops over a range and over a list read the item of their own turn, and in a loop inside a loop the locals of both.

```brix
(main
  (let getters (call (of List (of Getter Int))))
  (for (in index (..< 0 3))
    (+= getters (=> () index))
  )
  (for (in item (list 10 20))
    (+= getters (=> () item))
  )
  (for (in getter getters)
    (print (call getter))
  )
  (let sums (call (of List (of Getter Int))))
  (for (in a (.. 1 2))
    (for (in b (.. 1 2))
      (let both-of (+ (* a 10) b))
      (+= sums (=> () (+ both-of a)))
    )
  )
  (for (in getter sums)
    (print (call getter))
  )
)
```

```output
0
1
2
10
20
12
13
23
24
```

### A closure changes a local of its turn, which the turn then reads, and closures of several turns change a local declared before the loop.

```brix
(main
  (let seen (call (of List Int)))
  (for (in index (..< 0 2))
    (let count index)
    (let bump (=> () :do (+= count 10)))
    (call bump)
    (call bump)
    (+= seen count)
  )
  (print (tuple ([] seen 0) ([] seen 1)))
  (let total 0)
  (let adders (call (of List (of Getter Int))))
  (for (in n (.. 1 3))
    (let scaled (* n 2))
    (+= adders
      (=> () :do
        (block
          (+= total scaled)
          (return total)
        )
      )
    )
  )
  (for (in adder adders)
    (print (call adder))
  )
  (print total)
)
```

```output
(20, 21)
2
6
12
12
```

### Closures in turns of a loop in a function read an argument beside the turn's local, and in a method the receiver beside the item.

```brix
(main
  (print (call run 3))
  (let box (call Box 5))
  (print (call (. box collect)))
)

(fn-do + run Int
  (param limit Int)
  (let first (of Getter Int) (=> () -1))
  (let last (of Getter Int) (=> () -1))
  (let count 0)
  (while (< count limit)
    (let snapshot (* count 10))
    (if (== count 0)
      (= first (=> () (+ snapshot limit)))
    )
    (= last (=> () (+ snapshot limit)))
    (+= count 1)
  )
  (return (+ (call first) (call last)))
)

(type + Box
  (prop + n Int)
  (fn-do + This _
    (param n Int)
    (= (. this n) n)
  )
  (fn-do + collect Int
    (let getters (call (of List (of Getter Int))))
    (for (in index (.. 1 3))
      (+= getters (=> () (* index n)))
    )
    (let sum 0)
    (for (in getter getters)
      (+= sum (call getter))
    )
    (return sum)
  )
)
```

```output
26
30
```

### Closures of one turn share its locals: one that reads only what is declared before the loop, one that reads the turn's, and one that changes a local of the function.

```brix
(main
  (print (call run 2))
)

(fn-do + run Int
  (param base Int)
  (let total 0)
  (let getters (call (of List (of Getter Int))))
  (let plain (call (of List (of Getter Int))))
  (for (in index (.. 1 3))
    (let local (* index base))
    (+= getters (=> () (+ local base)))
    (+= plain (=> () base))
    (let add (=> () :do (+= total local)))
    (call add)
  )
  (let sum total)
  (for (in getter getters)
    (+= sum (call getter))
  )
  (for (in getter plain)
    (+= sum (call getter))
  )
  (return sum)
)
```

```output
36
```

### A closure in a closure reads the local and the item of the turn it is written in, and a closure keeps a count of its own for each turn.

```brix
(main
  (let makers (call (of List (of Getter Int))))
  (for (in index (.. 1 3))
    (let offset (* index 100))
    (let inner
      (=> () :do
        (block
          (let deep (=> () (+ offset index)))
          (return (call deep))
        )
      )
    )
    (+= makers inner)
  )
  (for (in maker makers)
    (print (call maker))
  )
  (let counters (call (of List (of Getter Int))))
  (for (in index (.. 1 2))
    (let count 0)
    (+= counters
      (=> () :do
        (block
          (+= count index)
          (return count)
        )
      )
    )
  )
  (for (in counter counters)
    (print (call counter))
    (print (call counter))
  )
)
```

```output
101
202
303
1
2
2
4
```

### Closures in turns of a loop with a clause, left by `continue` and by `break`, in a loop that takes its item apart, and in one that names no item.

```brix
(main
  (let getters (call (of List (of Getter Int))))
  (for (in index (.. 1 6) (when (== (% index 2) 0)))
    (if (== index 4)
      (continue)
    )
    (+= getters (=> () index))
  )
  (for (in (tuple a b) (list (tuple 1 2) (tuple 3 4)))
    (+= getters (=> () (+ (* a 10) b)))
  )
  (for (in (.. 1 2))
    (+= getters (=> () (+ each it)))
  )
  (for (in n (.. 1 10))
    (let squared (* n n))
    (+= getters (=> () squared))
    (if (== n 2)
      (break)
    )
  )
  (for (in getter getters)
    (print (call getter))
  )
)
```

```output
2
6
12
34
2
4
1
4
```

### A loop in a closure, whose turns each have closures of their own.

```brix
(main
  (let build
    (=> () :do
      (block
        (let made (call (of List (of Getter Int))))
        (for (in index (.. 1 2))
          (let twice (* index 2))
          (+= made (=> () (+ twice index)))
        )
        (return made)
      )
    )
  )
  (for (in getter (call build))
    (print (call getter))
  )
)
```

```output
3
6
```

### A function that yields a closure from each turn of a loop, which reads a local of its turn.

```brix
(main
  (for (in getter (call makers))
    (print (call getter))
  )
)

(fn-do + makers _
  (for (in index (..< 0 3))
    (let label (* index 2))
    (let getter (of Getter Int) (=> () label))
    (yield getter)
  )
)
```

```output
0
2
4
```

### A closure in a turn of a loop reads a lazy local of that turn.

```brix
(main
  (print (call lazily 3))
)

(fn-do + lazily Int
  (param limit Int)
  (let first (of Getter Int) (=> () 0))
  (for (in index (.. 1 limit))
    (lazy heavy (* index 1000))
    (if (== index 2)
      (= first (=> () heavy))
    )
  )
  (return (call first))
)
```

```output
2000
```

### A closure of several lines given to `+=` of a list panics when it is called, and the program ends by the panic.

```brix crash
(main
  (let x (call (of List (of Getter Int))))
  (+= x
    (=> () :do
      (block
        (let n 3)
        (return (call check n))
      )
    )
  )
  (print (call ([] x 0)))
)

(fn-do + check Int
  (param n Int)
  (throw (call Panic "p"))
)
```

### A closure given to `+=` of a list in a `while` loop panics when it is called, and the program ends by the panic.

```brix crash
(main
  (let x (call (of List (of Getter Int))))
  (let n 0)
  (while (< n 1)
    (+= x (=> () (call check 3)))
    (+= n 1)
  )
  (print (call ([] x 0)))
)

(fn-do + check Int
  (param n Int)
  (throw (call Panic "p"))
)
```

### A closure given to `+=` of a list in a loop of another closure panics when it is called, and the program ends by the panic.

```brix crash
(main
  (let x (call (of List (of Getter Int))))
  (let add
    (=> () :do
      (block
        (for (in i (..< 0 1))
          (+= x (=> () (call check 3)))
        )
      )
    )
  )
  (call add)
  (print (call ([] x 0)))
)

(fn-do + check Int
  (param n Int)
  (throw (call Panic "p"))
)
```

### A closure given to `+=` of a list in a closure written in a loop panics when it is called, and the program ends by the panic.

```brix crash
(main
  (let x (call (of List (of Getter Int))))
  (for (in i (..< 0 1))
    (let add
      (=> () :do
        (block
          (+= x (=> () (call check 3)))
        )
      )
    )
    (call add)
  )
  (print (call ([] x 0)))
)

(fn-do + check Int
  (param n Int)
  (throw (call Panic "p"))
)
```

### A closure given to `+=` of a list in a loop of a function that yields panics when it is called, and the program ends by the panic.

```brix crash
(main
  (let x (call (of List (of Getter Int))))
  (for (in n (call gen x))
    (print n)
  )
  (print (call ([] x 0)))
)

(fn-do + gen _
  (param x (of List (of Getter Int)))
  (for (in i (..< 0 1))
    (+= x (=> () (call check 3)))
    (yield i)
  )
  (+= x (=> () (call check 4)))
)

(fn-do + check Int
  (param n Int)
  (throw (call Panic "p"))
)
```

```output
0
```

### A closure held in a local and given to `+=` of a list panics when it is called, and the program ends by the panic.

```brix crash
(main
  (let x (call (of List (of Getter Int))))
  (let f (=> () (call check 3)))
  (+= x f)
  (print (call ([] x 0)))
)

(fn-do + check Int
  (param n Int)
  (throw (call Panic "p"))
)
```

### A function given by its name to `+=` of a list panics when it is called, and the program ends by the panic.

```brix crash
(main
  (let x (call (of List (of Getter Int))))
  (+= x boom)
  (print (call ([] x 0)))
)

(fn-do + boom Int
  (return (call check 3))
)

(fn-do + check Int
  (param n Int)
  (throw (call Panic "p"))
)
```

### A closure given to `+=` of a list in a loop reads a local of the function and the item, and panics when it is called.

```brix crash
(main
  (call run)
)

(fn-do + run _
  (let x (call (of List (of Getter Int))))
  (let base 3)
  (for (in i (..< 0 1))
    (+= x (=> () (call check (+ base i))))
  )
  (call ([] x 0))
)

(fn-do + check Int
  (param n Int)
  (throw (call Panic "p"))
)
```

### Closures given to `+=` of a list in a loop each read their turn's local, and the one that panics ends the program.

```brix crash
(main
  (call run)
)

(fn-do + run _
  (let fs (call (of List (of Getter Int))))
  (for (in i (.. 1 3))
    (let v i)
    (+= fs (=> () (call check v)))
  )
  (for (in f fs)
    (print (call f))
  )
)

(fn-do + check Int
  (param n Int)
  (if (== n 3)
    (throw (call Panic "turn"))
  )
  (return n)
)
```

```output
1
2
```

### A closure of a turn sees what the turn stores after a yield, in loops within loops

```brix
(main
  (let readers (call (of List (of Getter Int))))
  (for (in reader (call counted 3))
    (+= readers reader)
  )
  (for (in reader readers)
    (print (call reader))
  )
  (for (in text (call nested))
    (print text)
  )
)

(fn-do + counted _
  (param limit Int)
  (for (in index (..< 0 limit))
    (let count (* index 10))
    (let reader (of Getter Int) (=> () count))
    (yield reader)
    (+= count 1)
  )
)

(fn-do + nested _
  (let made (call (of List (of Getter String))))
  (for (in outer (list "a" "b"))
    (let prefix (str outer ":"))
    (for (in inner (list 1 2 3))
      (if (== inner 2)
        (continue)
      )
      (let label (str prefix inner))
      (+= made (=> () label))
      (yield (str "made " label))
      (if (== outer "b")
        (break)
      )
    )
    (yield (str "turn " outer))
  )
  (for (in make made)
    (yield (call make))
  )
)
```

```output
1
11
21
made a:1
made a:3
turn a
made b:1
turn b
a:1
a:3
b:1
```

### Closures of turns of loops that match, take apart, have two headers and read a stream, in functions that yield

```brix
(main
  (for (in text (call matched (list (tuple 1 "a") (tuple 2 "b") (tuple 3 "c"))))
    (print text)
  )
  (for (in text (call paired (list "x" "y" "z") (list 7 8 9)))
    (print text)
  )
  (for (in text (call streamed))
    (print text)
  )
)

(fn-do + matched _
  (param pairs (list (tuple Int String)))
  (let fitted (call (of List (of Getter String))))
  (let apart (call (of List (of Getter String))))
  (for (in (| (tuple (: n ({} (!= it 2))) word)) pairs)
    (+= fitted (=> () (str n word)))
    (yield (str "saw " word))
  )
  (for (in (tuple number word) pairs)
    (+= apart (=> () (str word number)))
  )
  (for (in make fitted)
    (yield (call make))
  )
  (for (in make apart)
    (yield (call make))
  )
)

(fn-do + paired _
  (param names (list String))
  (param sizes (list Int))
  (let joined (call (of List (of Getter String))))
  (for (in name names)
    (and-for (in size sizes (when (!= size 8)))
      (+= joined (=> () (str name "=" size)))
      (yield name)
    )
  )
  (for (in make joined)
    (yield (call make))
  )
)

(fn-do + streamed _
  (let summed (call (of List (of Getter Int))))
  (for (in value (call source))
    (let doubled (* value 2))
    (+= summed (=> () (+ doubled value)))
    (yield (str "got " value))
  )
  (for (in make summed)
    (yield (str (call make)))
  )
)

(fn-do + source _
  (yield 1)
  (yield 2)
  (yield 3)
)
```

```output
saw a
saw c
1a
3c
a1
b2
c3
x
y
x=7
y=9
got 1
got 2
got 3
3
6
9
```

### A lazy local of a turn is evaluated once for the turn, by whichever closure or statement reads it first

```brix
(main
  (let readers (call (of List (of Getter Int))))
  (for (in index (.. 1 3))
    (lazy heavy (call noisy index))
    (let one (of Getter Int) (=> () heavy))
    (let two (of Getter Int) (=> () (+ heavy 1)))
    (if (== index 2)
      (print (str "early " heavy))
    )
    (+= readers one)
    (+= readers two)
    (if (== index 3)
      (+= readers (=> () index))
    )
  )
  (print "made")
  (for (in reader readers)
    (print (call reader))
  )
  (print (call unread 3))
  (for (in text (call stepped))
    (print text)
  )
)

(fn-do + noisy Int
  (param value Int)
  (print (str "noisy " value))
  (return (* value 100))
)

(fn-do + unread Int
  (param limit Int)
  (let total 0)
  (for (in (..< 0 limit))
    (lazy heavy (call noisy each))
    (lazy deeper (+ heavy it))
    (let skip (of Getter Int) (=> () deeper))
    (if (== each 1)
      (+= total (call skip))
    )
  )
  (return total)
)

(fn-do + stepped _
  (let made (call (of List (of Getter Int))))
  (for (in index (..< 0 2))
    (lazy heavy (call noisy (+ index 5)))
    (+= made (=> () heavy))
    (yield (str "turn " index))
  )
  (for (in make made)
    (yield (str (call make)))
  )
)
```

```output
noisy 2
early 200
made
noisy 1
100
101
200
201
noisy 3
300
301
3
noisy 1
101
turn 0
turn 1
noisy 5
500
noisy 6
600
```

### A lazy local that closures capture reads a local of its function and one of its turn

```brix
(main
  (print (call lazily 3))
  (print (call twice 4))
)

(fn-do + lazily Int
  (param limit Int)
  (let base (* limit 2))
  (lazy heavy (* base 1000))
  (let first (of Getter Int) (=> () heavy))
  (return (call first))
)

(fn-do + twice Int
  (param limit Int)
  (let total 0)
  (for (in index (.. 1 limit))
    (let scale (* index 10))
    (lazy heavy (+ scale 1))
    (let read (of Getter Int) (=> () heavy))
    (+= total (+ (call read) heavy))
  )
  (return total)
)
```

```output
6000
208
```

### A closure in a while condition can read a variable the loop body changes.

```brix
(main
  (let number 1)
  (let taken (list "a-1" "a-2"))
  (while (call holds taken (:where (== each (str "a-" number))))
    (+= number 1)
  )
  (print number)
)

(fn-do + (of holds Item) _
  (param stream (stream Item))
  (param where (of Predicate Item))
  (for (in item stream)
    (if (call where item)
      (return true)
    )
  )
  (return false)
)
```

```output
3
```
