# Scopes

```brix crash
(main
  (let a I32 1)
  (let a I32 2)
)
```

```output
Exception: "a" is already declared on line 3 in this scope; to change its value, assign it with "(= a value)", or choose another name
<no file:4>
      (let a I32 2)
           ^^^^^
```

```brix crash
(main
  (= (call a) 1)
)
```

```output
Exception: Illegal assignment
<no file:3>
      (= (call a) 1)
      ^^^^^^^^^^^^^^
```

```brix
(main
  (let a I32 1)
  (do
    (let a Float 2.0)
    (print a)
  )
  (print a)
)
```

```output
2
1
```

```brix
(main
  (let a I32 1)
  (do
    (print a)
    (let a Float 2.0)
  )
)
```

```output
1
```

```brix
(main
  (let a I32 1)
  (do
    (let a Float a)
    (print a)
  )
)
```

```output
1
```

```brix crash
(main
  (do
    (let a Float 2.0)
  )
  (print a)
)
```

```output
Exception: Name "a" not found
<no file:6>
      (print a)
             ^
```

```brix
(main
  (let a 2)
  (do
    (= a 4)
  )
  (print a)
)
```

```output
4
```

```brix
(main
  (let thing (call Thing))
)

(type + Thing
  (prop + a _ (= 10))
  (fn-do + This _
    (do
      (let a 1)
      (print a)
    )
    (print a)
  )
)
```

```output
1
10
```

```brix
(main
  (let (tuple (: foo Int) (: bar String) (: baz Float)))
  (print foo)
  (print bar)
  (print baz)
)
```

```output
0

0
```

```brix
(main
  (lazy (tuple (: foo Int) (: bar String) (: baz Float)) (tuple 1 "hi" 2))
  (print foo)
  (print bar)
  (print baz)
)
```

```output
1
hi
2
```

```brix crash
(main
  (let (tuple (: foo Int) (: bar String) (: baz Float)) (tuple 1 "hi"))
)
```

```output
Exception: Pattern has no possible matches
<no file:3>
      (let (tuple (: foo Int) (: bar String) (: baz Float)) (tuple 1 "hi"))
           ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix
(main
  (lazy foo Int 1)
  (print foo)
)
```

```output
1
```

```brix
(main
  (lazy foo
    (scope bar
      (lazy bar 1)
    )
  )
  (print foo)
)
```

```output
1
```

```brix
(main
  (print
    (block
      (case? false
        a
      )
      (default
        (inline a)
      )
    )
  )
  (blank)
  (lazy a (call noisy 10))
)

(fn-do + noisy Int
  (param n Int)
  (print n)
  (return n)
)
```

```output
10
10
```

### A name nothing declares cannot be assigned.

```brix crash
(main
  (= a 1)
)
```

```output
Exception: Variable "a" is not declared; declare it with "(let a 1)"
<no file:3>
      (= a 1)
      ^^^^^^^
```

### A name a function declares cannot be assigned.

```brix crash
(main
  (= helper 5)
)

(fn-expr + helper U32
  3
)
```

```output
Exception: Function "helper" cannot be assigned
<no file:3>
      (= helper 5)
      ^^^^^^^^^^^^
```

### A local takes its type from its value, or the type written for it, which the value converts to.

```brix
(main
  (let a 5)
  (let b I64 5)
  (let c Float 1)
  (let d F32 1)
  (let e I64 5u8)
  (let f U8 7)
  (let g "text")
  (let h true)
  (print a)
  (print b)
  (print c)
  (print d)
  (print e)
  (print f)
  (print g)
  (print h)
)
```

```output
5
5
1
1
5
7
text
true
```

### A local declared with no value has the zero of its type, and one statement declares several.

```brix
(main
  (let x I32)
  (let f Float)
  (let b Bool)
  (let s String)
  (let (tuple (: m I32) (: n Float)))
  (print x)
  (print f)
  (print b)
  (print (++ s "!"))
  (= m 2)
  (= n m)
  (print m)
  (print n)
)
```

```output
0
0
false
!
2
2
```

### A local declared in a loop is declared anew on each pass.

```brix
(main
  (for (in i (..< 0 3))
    (let z Int)
    (let s String)
    (+= z i)
    (++= s "k")
    (print z)
    (print s)
  )
)
```

```output
0
k
1
k
2
k
```

### A local hides another of its name in a scope within, and the outer one is read until it is declared.

```brix
(main
  (let a I32 1)
  (do
    (let a Float a)
    (+= a 0.5)
    (print a)
  )
  (print a)
  (do
    (= a 2)
    (do
      (let a 3)
      (print a)
    )
    (print a)
  )
  (print a)
)
```

```output
1.5
1
3
2
2
```

### A local declared in a branch is not the one declared after it.

```brix
(main
  (let x 1)
  (if x
    (let y "in")
    (print y)
  )
  (let y 2.5)
  (print y)
)
```

```output
in
2.5
```

### Assignment converts the value to the local's type.

```brix
(main
  (let x I64 5)
  (= x 6i32)
  (print x)
  (let f 1.5)
  (= f 2)
  (print f)
  (let n 3)
  (= n 2i64)
  (print n)
  (let u U8 5)
  (= u 200)
  (print u)
  (let b true)
  (= b (> 1 2))
  (print b)
)
```

```output
6
2
2
200
false
```

### A value cannot stand alone as a step.

```brix crash
(main
  (let x 1)
  (+ x 1)
  (print x)
)
```

```output
Exception: (+ …) cannot be a `row` of (main …), which takes Step. This body runs steps, and each row is a step: a statement form, a `call`, markup, or a value that `pipe` stages after it work on. To return a value, write `(return value)`. If the body is only a value, write `fn-expr`, or remove `:do` from a closure.
<no file:4>
      (+ x 1)
      ^
```

### A local may have a name that JavaScript objects inherit.

```brix
(main
  (let total 0)
  (for (in index (..< 0 3))
    (let constructor (* index 2))
    (+= total constructor)
  )
  (print total)
)
```

```output
6
```

### Past a test that finds an optional local holds a value, a local may be declared again.

```brix
(main
  (call run 3)
  (call run 0)
)

(fn-do + run _
  (param limit U64)
  (let kept (+ limit 1))
  (print (str "early " kept))
  (let path (only-if limit (if (> limit 1))))
  (return-if (if (not path)))
  (let kept (str "late " path))
  (print kept)
)
```

```output
early 4
late 3
early 1
```

### Inside the body of a test that finds an optional local holds a value, a local is declared once.

```brix crash
(main
  (let path (? U64) 3)
  (if path
    (let kept (+ (!! path) 1))
    (print kept)
    (let kept (+ (!! path) 2))
    (print kept)
  )
)
```

```output
Exception: "kept" is already declared on line 5 in this scope; to change its value, assign it with "(= kept value)", or choose another name
<no file:7>
        (let kept (+ (!! path) 2))
             ^^^^
```
