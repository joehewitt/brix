# Recursion

```brix
(main
  (print (call foo 3))
)

(fn-do + foo Int
  (param n _)
  (print n)
  (if (> n 0)
    (print (call foo (- n 1)))
  )
  (blank)
  (return n)
)
```

```output
3
2
1
0
0
1
2
3
```

```brix
(main
  (print ([] (call recursive-map 3) "value"))
)

(fn-expr + recursive-map _ (?! Error)
  (param n Int)
  (call (of Map String Int)
    (block
      (:value (call recursive-value n))
    )
  )
)

(fn-do + recursive-value _ (?! Error)
  (param n Int)
  (return-if n (if (== n 0)))
  (return (!! ([] (call recursive-map (- n 1)) "value")))
)
```

```output
0
```

```brix crash
(main
  (print (call foo))
)

(fn-do + foo _
  (call foo)
  (return 1)
)
```

```output
Exception: Function "foo" calls itself before its return type is known, so the type cannot be inferred; declare it, as in "(fn-do + foo I64 …)"
<no file:6>
    (fn-do + foo _
    ^^^^^^^^^^^^^
```

```brix crash
(main
  (call bar)
)

(fn-do + foo _
  (call bar)
  (return 1)
)

(fn-expr + bar _
  (call foo)
)
```

```output
Exception: Function "bar" calls itself before its return type is known, so the type cannot be inferred; declare it, as in "(fn-expr + bar I64 …)"
<no file:11>
    (fn-expr + bar _
    ^^^^^^^^^^^^^^^
```

### A rule that calls itself without stating its type.

```brix crash
(import /core/tokenstream)
(blank)

(main
  (let stream (call (of ArrayTokenStream Char) (. "ab" codepoints)))
  (print
    (match stream
      (~ (: v \word)
        v
      )
      (default
        (inline "miss")
      )
    )
  )
)

(fn-expr + word _ (over: Char)
  (~ (seq "a" \word))
  (~ "b")
)
```

```output
Exception: Function "word" calls itself before its return type is known, so the type cannot be inferred; declare it, as in "(fn-expr + word ReturnType (over: Char) …)"
<no file:19>
    (fn-expr + word _ (over: Char)
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A local rule that calls itself without stating its type.

```brix crash
(main
  (print
    (match "((a))"
      (~ \nest)
      (default
        (inline "none")
      )
      (blank)
      (lazy nest
        (pattern
          (~ (| (seq "(" \nest ")") "a"))
        )
      )
    )
  )
)
```

```output
Exception: Can't infer return type after recursion
<no file:10>
          (lazy nest
                ^^^^
```

### A rule whose arm reads the value of its own call without stating its type.

```brix crash
(main
  (print
    (match "((a))"
      (~ \nest)
      (default
        (inline -1)
      )
    )
  )
)

(fn-expr + nest _ (over: Char)
  (~ (seq "(" (: inner \nest) ")")
    (+ inner 1)
  )
  (~ "a"
    0
  )
)
```

```output
Exception: Function "nest" calls itself before its return type is known, so the type cannot be inferred; declare it, as in "(fn-expr + nest ReturnType (over: Char) …)"
<no file:13>
    (fn-expr + nest _ (over: Char)
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Recursion in method

```brix
(main
  (let foo (call Foo))
  (print (call (. foo bar) 3))
)

(type + Foo
  (fn-do + bar Int
    (param n _)
    (print n)
    (if (> n 0)
      (print (call bar (- n 1)))
    )
    (return n)
  )
)
```

```output
3
2
1
0
0
1
2
3
```

```brix crash
(main
  (let foo (call Foo))
  (call (. foo bar))
)

(type + Foo
  (fn-do + foo _
    (call bar)
    (return 1)
  )
  (fn-expr + bar _
    (call foo)
  )
)
```

```output
Exception: Function "bar" calls itself before its return type is known, so the type cannot be inferred; declare it, as in "(fn-expr + bar I64 …)"
<no file:12>
      (fn-expr + bar _
      ^^^^^^^^^^^^^^^
```

### Recursion in converter

```brix
(main
  (let foo (call Foo))
  (= (. foo a) 1)
  (print (as foo Bar))
)

(type + Foo
  (prop + a _ (= 0))
  (fn-do + This _
    (param a _)
  )
  (fn-to-do + Bar
    (print a)
    (return (if-expr (== a 0) (call Bar) (call Foo (- a 1))))
  )
)

(type + Bar)
```

```output
1
0
[Object __source__:Bar]
```

### A panic from a recursive call stops the caller, directly and through mutual recursion.

```brix
(prop + count _ (= 0))
(blank)

(main
  (print (call sum 4))
  (call guard (=> () :do (call countdown 2)))
  (call guard (=> () :do (call ping 3)))
)

(fn-do + guard _
  (param body _)
  (let attempt
    (do-new
      (call body)
      (on-failure failure
        (let panic (? Panic) failure)
        (print (str "count " count ", " panic))
        (= count 0)
      )
    )
  )
  (call (. attempt wait))
)

(fn-do + sum Int
  (param n Int)
  (return-if 0 (if (== n 0)))
  (return (+ n (call sum (- n 1))))
)

(fn-do + countdown _
  (param n Int)
  (if (> n 0)
    (call countdown (- n 1))
    (+= count 1)
  )
  (throw (call Panic (str "countdown " n)))
)

(fn-do + ping _
  (param n Int)
  (if (> n 0)
    (call pong (- n 1))
    (+= count 1)
  )
  (throw (call Panic (str "ping " n)))
)

(fn-do + pong _
  (param n Int)
  (call ping n)
  (+= count 10)
)
```

```output
10
count 0, countdown 0
count 0, ping 0
```

### Two functions call each other.

```brix
(main
  (print (call is-even 10))
  (print (call is-odd 7))
  (print (call is-even 7))
)

(fn-expr + is-even Bool
  (param n Int)
  (case? (== n 0)
    true
  )
  (default
    (call is-odd (- n 1))
  )
)

(fn-expr + is-odd Bool
  (param n Int)
  (case? (== n 0)
    false
  )
  (default
    (call is-even (- n 1))
  )
)
```

```output
true
true
false
```

### A generic function that calls itself with type arguments that grow is an error at the call that does.

```brix crash
(main
  (print (call deep 3 2))
)

(fn-expr + deep Int
  (param x ''T)
  (param n Int)
  (if-expr (== n 0) 0 (call deep (list x) (- n 1)))
)
```

```output
Exception: Function "deep" calls itself with type arguments that grow without end; call it with its own type parameters
<no file:9>
      (if-expr (== n 0) 0 (call deep (list x) (- n 1)))
                          ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix crash
(type + (of Box T)
  (prop + item T)
  (fn-do + This _
    (param item T)
  )
  (fn-expr + depth Int
    (param n Int)
    (if-expr (== n 0) 0 (call (. (call Box (list item)) depth) (- n 1)))
  )
)

(main
  (print (call (. (call Box 3) depth) 2))
)
```

```output
Exception: Function "depth" calls itself with type arguments that grow without end; call it with its own type parameters
<no file:9>
        (if-expr (== n 0) 0 (call (. (call Box (list item)) depth) (- n 1)))
                            ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A `do` function calls itself a hundred thousand deep.

```brix
(main
  (print (call depth 100000))
)

(fn-do + depth Int
  (param n Int)
  (return-if 0 (if (== n 0)))
  (return (+ 1 (call depth (- n 1))))
)
```

```output
100000
```

### Unbounded recursion reports a stack overflow with the recursing frame collapsed.

```brix crash
(main
  (call recurse 0)
)

(fn-do + recurse Int
  (param n Int)
  (return (+ (call recurse (+ n 1)) 1))
)
```

```stderr
Stack overflow

Task 1

Stack trace:
  0  __source__:recurse no file:6:1
  1  __source__:recurse no file:8:14
  ... recursive calls
  2  __source__:main no file:3:3
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### A frame larger than the stack guard still overflows cleanly in a spawned task.

```brix crash
(import /core/channel)
(blank)

(main
  (let done (of SyncChannel Int))
  (do-new
    (<- done (call deep 0))
  )
  (print (<- done))
)

(fn-do + deep Int
  (param n Int)
  (let big (call T5))
  (return (+ (call deep (+ n 1)) (. big h h h h h)))
)

(type + T1 (is Tuple)
  (prop + a I32 (= 0))
  (prop + b I32 (= 0))
  (prop + c I32 (= 0))
  (prop + d I32 (= 0))
  (prop + e I32 (= 0))
  (prop + f I32 (= 0))
  (prop + g I32 (= 0))
  (prop + h I32 (= 0))
)

(type + T2 (is Tuple)
  (prop + a T1 (= (call T1)))
  (prop + b T1 (= (call T1)))
  (prop + c T1 (= (call T1)))
  (prop + d T1 (= (call T1)))
  (prop + e T1 (= (call T1)))
  (prop + f T1 (= (call T1)))
  (prop + g T1 (= (call T1)))
  (prop + h T1 (= (call T1)))
)

(type + T3 (is Tuple)
  (prop + a T2 (= (call T2)))
  (prop + b T2 (= (call T2)))
  (prop + c T2 (= (call T2)))
  (prop + d T2 (= (call T2)))
  (prop + e T2 (= (call T2)))
  (prop + f T2 (= (call T2)))
  (prop + g T2 (= (call T2)))
  (prop + h T2 (= (call T2)))
)

(type + T4 (is Tuple)
  (prop + a T3 (= (call T3)))
  (prop + b T3 (= (call T3)))
  (prop + c T3 (= (call T3)))
  (prop + d T3 (= (call T3)))
  (prop + e T3 (= (call T3)))
  (prop + f T3 (= (call T3)))
  (prop + g T3 (= (call T3)))
  (prop + h T3 (= (call T3)))
)

(type + T5 (is Tuple)
  (prop + a T4 (= (call T4)))
  (prop + b T4 (= (call T4)))
  (prop + c T4 (= (call T4)))
  (prop + d T4 (= (call T4)))
  (prop + e T4 (= (call T4)))
  (prop + f T4 (= (call T4)))
  (prop + g T4 (= (call T4)))
  (prop + h T4 (= (call T4)))
)
```

```stderr
Stack overflow

Task 2, spawned by task 1 from no file:7:3

Stack trace:
  0  __source__:deep no file:13:1
  1  __source__:deep no file:16:14
  ... recursive calls
  2  __source__:Fork no file:8:14
```
