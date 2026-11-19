# Stack locals

```brix
(prop + current _ @stack (= 10))
(blank)

(main
  (print current)
)
```

```output
10
```

```brix
(prop + current Int @stack)
(blank)

(main
  (do-with current 10
    (print current)
    (do-with current 20
      (print current)
    )
    (print current)
  )
)
```

```output
10
20
10
```

```brix
(prop + current Int @stack)
(blank)

(main
  (do-with current 42
    (print (call read-current))
  )
)

(fn-expr + read-current Int
  current
)
```

```output
42
```

```brix
(prop + current Int @stack)
(blank)

(main
  (let read (? (of Getter Int)))
  (do-with current 31
    (let captured (=> () current))
    (= read (as captured (of Getter Int)))
  )
  (blank)
  (print (call read))
)
```

```output
31
```

```brix
(prop + current Int @stack)
(blank)

(main
  (let results (of SyncChannel Int))
  (do-with current 77
    (do-new
      (<- results current)
    )
  )
  (blank)
  (print (<- results))
)
```

```output
77
```

```brix
(prop + current Int @stack)
(blank)

(main
  (let results (of SyncChannel Int))
  (do-with current 88
    (do-new
      (<- results (call read-current))
    )
  )
  (blank)
  (print (<- results))
)

(fn-expr + read-current Int
  current
)
```

```output
88
```

### A stack-local struct is bound and read inside and outside the binding, and by a function.

```brix
(prop + where _ @stack (= (call Spot 1 "home")))
(blank)

(main
  (print (tuple (. where x) (. where label)))
  (do-with where (call Spot 2 "away")
    (print (tuple (. where x) (. where label)))
    (call show)
  )
  (call show)
)

(type + Spot (is Struct)
  (prop + x Int)
  (prop + label String)
)

(fn-do + show _
  (print (tuple "show" (. where x) (. where label)))
)
```

```output
(1, home)
(2, away)
(show, 2, away)
(show, 1, home)
```

### A binding in a loop is put back at each turn, and where the turn continues.

```brix
(prop + depth _ @stack (= 0))
(blank)

(main
  (for (in i (..< 0 3))
    (do-with depth (* i 10)
      (if (== i 1)
        (continue)
      )
      (print (tuple "in" depth (call peek)))
    )
    (print (tuple "out" depth (call peek)))
  )
)

(fn-do + peek Int
  (return depth)
)
```

```output
(in, 0, 0)
(out, 0, 0)
(in, 20, 20)
(out, 0, 0)
```

### A recursive function binds the stack-local from its own read of it, and returns from inside the binding.

```brix
(prop + depth _ @stack (= 1))
(blank)

(main
  (print (call find 3))
  (print depth)
)

(fn-do + find Int
  (param n Int)
  (do-with depth (+ depth n)
    (if (== n 0)
      (return depth)
    )
    (return (call find (- n 1)))
  )
)
```

```output
7
1
```

### A spawned task reads the stack-local it captured, binds its own, and calls a reader inside and outside that binding.

```brix
(prop + depth _ @stack (= 1))
(blank)

(main
  (let done (call Semaphore))
  (do-new
    (print (tuple "task" depth))
    (do-with depth 7
      (print (tuple "inner" depth (call peek)))
    )
    (print (tuple "after" depth (call peek)))
    (call (. done post))
  )
  (call (. done wait))
)

(fn-do + peek Int
  (return depth)
)
```

```output
(task, 1)
(inner, 7, 7)
(after, 1, 1)
```

### Lambdas made under different bindings hold the state each was made under, read directly and through a function, inside and outside a later binding.

```brix
(prop + current _ @stack (= 5))
(blank)

(main
  (let a (? (of Getter Int)))
  (let b (? (of Getter Int)))
  (do-with current 1
    (= a (=> () current))
  )
  (do-with current 2
    (= b (=> () (call read)))
  )
  (print (call a))
  (print (call b))
  (do-with current 3
    (print (call a))
    (print (call b))
  )
  (print current)
)

(fn-expr + read Int
  (+ current 100)
)
```

```output
1
102
1
102
5
```

### A lambda made under a binding in a function holds that binding after the function returns.

```brix
(prop + current Int @stack)
(blank)

(main
  (let f (call make 7))
  (print (call f))
)

(fn-do + make (of Getter Int)
  (param n Int)
  (let made (? (of Getter Int)))
  (do-with current n
    (= made (=> () (+ current 1)))
  )
  (return made)
)
```

```output
8
```

### A spawned task's body makes a lambda that reads the stack-local the task captured.

```brix
(prop + current _ @stack (= 5))
(blank)

(main
  (let done (call Semaphore))
  (do-with current 70
    (do-new
      (let f (=> () (+ current 1)))
      (print (call f))
      (call (. done post))
    )
  )
  (call (. done wait))
)
```

```output
71
```

### A task spawned from a spawned task's body reads the stack-local the outer body never reads.

```brix
(prop + current _ @stack (= 5))
(blank)

(main
  (let done (call Semaphore))
  (do-with current 71
    (do-new
      (do-new
        (print (tuple "inner" current))
        (call (. done post))
      )
    )
  )
  (call (. done wait))
)
```

```output
(inner, 71)
```

### A task spawned from a spawned task's body calls a reader of the stack-local.

```brix
(prop + current _ @stack (= 5))
(blank)

(main
  (let done (call Semaphore))
  (do-with current 72
    (do-new
      (do-new
        (print (tuple "inner" (call read)))
        (call (. done post))
      )
    )
  )
  (call (. done wait))
)

(fn-expr + read Int
  current
)
```

```output
(inner, 72)
```

### A lambda calls a function that reads the stack-local through another, outside any binding.

```brix
(prop + current _ @stack (= 5))
(blank)

(main
  (let made (? (of Getter Int)))
  (do-with current 90
    (= made (=> () (call outer)))
  )
  (print (call made))
  (do-with current 91
    (print (call made))
  )
)

(fn-expr + outer Int
  (* (call inner) 2)
)

(fn-expr + inner Int
  current
)
```

```output
180
180
```

### A lambda spawns a task that reads the stack-local the lambda holds.

```brix
(prop + current _ @stack (= 5))
(blank)

(main
  (let done (call Semaphore))
  (let made (? (of Getter Int)))
  (do-with current 30
    (= made
      (=> () :do
        (block
          (do-new
            (print (tuple "task" current))
            (call (. done post))
          )
          (return current)
        )
      )
    )
  )
  (print (call made))
  (call (. done wait))
)
```

```output
30
(task, 30)
```

### A spawned task's slot set for a call that panics is put back as it was before the call.

```brix
(prop + depth _ @stack (= 1))
(blank)

(main
  (let results (of SyncChannel String))
  (let o Base (call Derived))
  (do-with depth 7
    (do-new
      (finally
        (let p (. (call Task) failure))
        (<- results (str "after " (!= p null) " " (call (. o read))))
      )
      (call boom)
      (on-failure _
        (print)
      )
    )
  )
  (print (<- results))
)

(fn-do + boom _
  (print (str "boom sees " depth))
  (throw (call Panic "x"))
)

(type + Base
  (fn-do + read String
    (return (str "base " depth))
  )
)

(type + Derived (is Base)
  (fn-do + read String
    (return (str "derived " depth))
  )
)
```

```output
boom sees 7
after true derived 1
```

### A lambda that yields reads the state it holds, not the binding open where it is iterated.

```brix
(prop + cur _ @stack (= 5))
(blank)

(main
  (let gen
    (=> () :do
      (block
        (yield cur)
        (yield (+ cur 1))
      )
    )
  )
  (for (in v (call gen))
    (print v)
  )
  (do-with cur 6
    (for (in v (call gen))
      (print v)
    )
  )
)
```

```output
5
6
5
6
```

### A lambda called from a lambda made inside a do with is given nothing: it reads what it holds.

```brix
(prop + depth _ @stack (= 1))
(blank)

(main
  (let inner
    (=> () :do
      (block
        (for (in v (call gen))
          (print (str "gen " v))
        )
      )
    )
  )
  (call inner)
  (let outer (? Doer))
  (do-with depth 3
    (= outer
      (=> () :do
        (block
          (print (str "outer " depth))
          (call inner)
        )
      )
    )
  )
  (call outer)
)

(fn-do + gen (stream Int)
  (yield depth)
)
```

```output
gen 1
outer 3
gen 1
```

### Lambdas that hold stack state made in a while loop whose turn has no environment of its own.

```brix
(prop + cur _ @stack (= 5))
(blank)

(main
  (let fs (list (of Getter Int)) (call (of List (of Getter Int))))
  (let i 0)
  (while (< i 2)
    (+= fs (=> () cur))
    (+= i 1)
  )
  (for (in f fs)
    (print (call f))
  )
)
```

```output
5
5
```

### A comprehension holds stack state as it was where it was made, which a function it calls reads, wherever it runs.

```brix
(prop + current Int @stack)
(blank)

(main
  (let made (call make))
  (do-with current 5
    (for (in text made)
      (print text)
    )
    (print (call (. ", " join) (in (call label each) (list 1 2))))
  )
)

(fn-do + make (of Iterable String)
  (do-with current 3
    (return (in (call label each) (list 1 2)))
  )
)

(fn-expr + label String
  (param n Int)
  (str (* n current))
)
```

```output
3
6
5, 10
```

### A with section binds stack values in turn, the first outermost, on a do block or on the header of a do function, where it binds for the whole body.

```brix
(prop + scope _ @stack (= "none"))
(prop + depth Int @stack (= 0))
(blank)

(main
  (do (with scope "one") (with depth 1)
    (call report)
  )
  (do (without gc) (with depth 2)
    (let doubled (* 2 2))
  )
  (call scoped)
  (call report)
)

(fn-do + report _
  (print (str scope " " depth))
)

(fn-do + scoped _ (with scope "header") (with depth (+ depth 5))
  (call report)
)
```

```output
one 1
header 5
none 0
```

```brix compile-error
(prop + depth Int @stack (= 0))
(blank)

(main
  (print (call deeper))
)

(fn-expr + deeper Int (with depth 1)
  depth
)
```

```output
Exception: A 'with' section needs a do function
<no file:9>
    (fn-expr + deeper Int (with depth 1)
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A global nothing binds with do with holds its first value everywhere. Where that value is a literal it is a constant, read where the task's memory is not: in a block without gc.

```brix
(prop + level _ @stack (= 7))
(prop + label _ @stack (= "plain"))
(blank)

(main
  (let done (of SyncChannel Int))
  (do-new
    (<- done (* level 2))
  )
  (print (<- done))
  (let reader (=> () (+ level 1)))
  (print (call reader))
  (do (without gc)
    (let doubled (* level 2))
  )
  (print (call add 3))
  (print (call (. ", " join) (in (str (+ each level) " " label) (list 1 2))))
)

(fn-do + add Int (without heap)
  (param n Int)
  (return (+ n level))
)
```

```output
14
8
10
8 plain, 9 plain
```
