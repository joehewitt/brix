# Stack local edges

```brix
(prop + current _ @stack (= 10))
(blank)

(main
  (do-with current 20
    (print current)
  )
  (print current)
)
```

```output
20
10
```

```brix
(prop + current _ @stack (= 5))
(blank)

(main
  (print (call scoped))
  (print current)
)

(fn-do + scoped Int
  (do-with current 99
    (return current)
  )
)
```

```output
99
5
```

```brix crash
(prop + current Int @stack)
(blank)

(main
  (= current 4)
)
```

```output
Exception: Stack-local variable 'current' can only be rebound with do with
<no file:6>
      (= current 4)
      ^^^^^^^^^^^^^
```

```brix crash
(prop + current Int @stack)
(blank)

(main
  (print current)
)
```

```output
Exception: Stack-local variable 'current' is not bound
<no file:6>
      (print current)
             ^^^^^^^
```

```brix crash
(prop + current Int @stack)
(blank)

(main
  (print (call read-current))
)

(fn-expr + read-current Int
  current
)
```

```output
Exception: Stack-local variable 'current' is not bound: main → read-current
<no file:10>
      current
      ^^^^^^^
```

```brix
(prop + current Int @stack)
(blank)

(main
  (let first (? (of Getter Int)))
  (let second (? (of Getter Int)))
  (do-with current 1
    (let captured (=> () current))
    (= first (as captured (of Getter Int)))
  )
  (do-with current 2
    (let captured (=> () current))
    (= second (as captured (of Getter Int)))
  )
  (blank)
  (print (call first))
  (print (call second))
)
```

```output
1
2
```

```brix
(prop + current Int @stack)
(prop + unused String @stack)
(blank)

(main
  (do-with current 73
    (do-new
      (print current)
    )
  )
)
```

```output
73
```

```brix
(prop + current Int @stack)
(blank)

(main
  (let read (? (of Getter Int)))
  (do-with current 64
    (let captured (=> () (call read-current)))
    (= read (as captured (of Getter Int)))
  )
  (blank)
  (print (call read))
)

(fn-expr + read-current Int
  current
)
```

```output
64
```

```brix
(prop + current _ @stack (= 5))
(blank)

(main
  (let read (? (of Getter Int)))
  (do-with current 64
    (let captured (=> () (call read-current)))
    (= read (as captured (of Getter Int)))
  )
  (blank)
  (print (call read))
  (print current)
)

(fn-expr + read-current Int
  current
)
```

```output
64
5
```

```brix
(prop + current _ @stack (= 5))
(blank)

(main
  (let results (of SyncChannel Int))
  (do-with current 81
    (do-new
      (<- results (call read-current))
    )
  )
  (blank)
  (print (<- results))
  (print current)
)

(fn-expr + read-current Int
  current
)
```

```output
81
5
```

```brix
(prop + current Int @stack)
(blank)

(main
  (do-with current 91
    (print (call odd 3))
  )
)

(fn-do + odd Int
  (param n Int)
  (return (call even (- n 1)))
)

(fn-do + even Int
  (param n Int)
  (if (== n 0)
    (return current)
  )
  (return (call odd (- n 1)))
)
```

```output
91
```

```brix compile-error
(prop + current Int @stack)
(blank)

(main
  (let reader (as (call Impl) Reader))
  (print (call (. reader read)))
)

(type + Reader (is Interface)
  (fn-expr + read Int)
)

(type + Impl
  (implements Reader)
  (fn-expr + read Int
    current
  )
)
```

```output
Exception: Stack-local variable 'current' is not bound: main → Reader.read → Impl.read
<no file:17>
        current
        ^^^^^^^
```

```brix compile-error
(prop + current Int @stack)
(blank)

(main
  (let base Base (call Derived))
  (print (call (. base value)))
)

(type + Base
  (fn-do + This _)
  (fn-expr + value Int
    0
  )
)

(type + Derived (is Base)
  (fn-do + This _)
  (fn-expr + value Int
    current
  )
)
```

```output
Exception: Stack-local variable 'current' is not bound: main → Base.value → Derived.value
<no file:20>
        current
        ^^^^^^^
```

```brix compile-error
(prop + current Int @stack)
(blank)

(main
  (let get (of Getter Int) read-current)
  (do-with current 4
    (print (call get))
  )
)

(fn-expr + read-current Int
  current
)
```

```output
Exception: Stack-local variable 'current' cannot be read through a function value: read-current
<no file:6>
      (let get (of Getter Int) read-current)
               ^^^^^^^^^^^^^^^
```

```brix compile-error
(prop + current Int @stack)
(blank)

(main
  (let get (=> () current))
  (do-with current 4
    (print (call get))
  )
)
```

```output
Exception: Stack-local variable 'current' is not bound: main → (closure)
<no file:6>
      (let get (=> () current))
                      ^^^^^^^
```

```brix compile-error
(prop + current Int @stack)
(blank)

(main
  (print (call made))
)

(fn-do + made Int
  (let get (=> () current))
  (return (call get))
)
```

```output
Exception: Stack-local variable 'current' is not bound: main → made → (closure)
<no file:10>
      (let get (=> () current))
                      ^^^^^^^
```

```brix
(prop + current Int @stack)
(blank)

(main
  (let reader (as (call Impl) Reader))
  (do-with current 4
    (print (call (. reader read)))
    (let get (=> () (+ current 1)))
    (print (call get))
  )
  (print (call made 7))
)

(type + Reader (is Interface)
  (fn-expr + read Int)
)

(type + Impl
  (implements Reader)
  (fn-expr + read Int
    current
  )
)

(fn-do + made Int
  (param n Int)
  (do-with current n
    (let get (=> () (* current 2)))
    (return (call get))
  )
  (return 0)
)
```

```output
4
5
14
```

```brix compile-error
(prop + first Int @stack)
(prop + second Int @stack)
(blank)

(main
  (print (+ first second))
)
```

```output
Exception: Stack-local variable 'first' is not bound
<no file:7>
      (print (+ first second))
                ^^^^^

Exception: Stack-local variable 'second' is not bound
<no file:7>
      (print (+ first second))
                      ^^^^^^
```
