# Declarations

Declaration errors say which rule a function or parameter breaks and how to fix it

### immediate declaration needs do

```brix crash
(main
  (print (call total))
)

(fn-expr + total Int
  (let x 1)
  x
)
```

```output
Exception: (let …) cannot be the `body` of (fn-expr …), which takes Value. This body is a value, and a statement cannot stand in it. To run steps, write `fn-do`, or `:do` in a closure, and return with `(return value)`; an expression function can also run them first in a `(do …)` in `(before …)`. Otherwise replace the statement with a value.
<no file:7>
      (let x 1)
      ^
```

### loop needs do

```brix crash
(main
  (print (call total))
)

(fn-expr + total I32
  (for (in i (..< 0 3))
    i
  )
  3
)
```

```output
Exception: (for …) cannot be the `body` of (fn-expr …), which takes Value. This body is a value, and a statement cannot stand in it. To run steps, write `fn-do`, or `:do` in a closure, and return with `(return value)`; an expression function can also run them first in a `(do …)` in `(before …)`. Otherwise replace the statement with a value.
<no file:7>
      (for (in i (..< 0 3))
      ^
```

### explicit return needs do

```brix crash
(main
  (print (call total))
)

(fn-expr + total I32
  (return 3)
)
```

```output
Exception: (return …) cannot be the `body` of (fn-expr …), which takes Value. This body is a value, and a statement cannot stand in it. To run steps, write `fn-do`, or `:do` in a closure, and return with `(return value)`; an expression function can also run them first in a `(do …)` in `(before …)`. Otherwise replace the statement with a value.
<no file:7>
      (return 3)
      ^
```

### value not returned

```brix crash
(main
  (print (call total))
)

(fn-do + total I32
  (let x 1)
  x
)
```

```output
Exception: `x` cannot be a `row` of (fn-do …), which takes Step. This body runs steps, and each row is a step: a statement form, a `call`, markup, or a value that `pipe` stages after it work on. To return a value, write `(return value)`. If the body is only a value, write `fn-expr`, or remove `:do` from a closure.
<no file:8>
      x
      ^
```

### required after a default

```brix crash
(main
  (print (call f 1 2))
)

(fn-expr + f I32
  (param a I32 (= 1))
  (param b I32)
  (+ a b)
)
```

```output
Exception: Parameter "b" has no default but comes after "a", which has a default; declare required parameters first, or give "b" a default
<no file:8>
      (param b I32)
      ^^^^^^^^
```

### a call that leaves out a parameter that may be null

```brix crash
(main
  (print (call f 1))
)

(fn-expr + f I32
  (param a I32)
  (param b (? I32))
  (+ a (?? b 0))
)
```

```output
Exception: No function "f" accepts (I64); it is declared as "f(a I32, b I32?)"; "b" may be null but has no default, so a call must give it: pass null, or declare "b I32? = null"
<no file:3>
      (print (call f 1))
             ^^^^^^^^^^
```

### typed declaration that cannot match

```brix crash
(main
  (let n I32 (call Pair 1 2))
)

(type + Pair (is Tuple)
  (prop + x I32)
  (prop + y I32)
)
```

```output
Exception: "n" is declared I32, but its value is Pair, which can never be I32; convert the value, or declare "n" without a type
<no file:3>
      (let n I32 (call Pair 1 2))
           ^^^^^
```
