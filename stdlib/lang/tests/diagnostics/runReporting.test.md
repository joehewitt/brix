# Reporting

A program's errors are all reported, one per line and at most ten, each once

### errors in several functions, in line order

```brix crash
(main
  (print (call f 1))
  (print (+ gone-left gone-right))
  (print gone-below)
)

(fn-expr + f I32
  (param a I32)
  (. a zz)
)
```

```output
Exception: Name "gone-left" not found
<no file:4>
      (print (+ gone-left gone-right))
                ^^^^^^^^^

Exception: Name "gone-below" not found
<no file:5>
      (print gone-below)
             ^^^^^^^^^^

Exception: I32 has no property "zz"
<no file:10>
      (. a zz)
      ^^^^^^^^
```

### a name missing twice in one function

```brix crash
(main
  (print (call f 1))
)

(fn-do + f I32
  (param a I32)
  (print gone-name)
  (print gone-name)
  (return a)
)
```

```output
Exception: Name "gone-name" not found
<no file:8>
      (print gone-name)
             ^^^^^^^^^
```

### one mistake that two passes report

```brix crash
(main
  (print (call test))
)

(fn-expr + test I32
  (return 42)
)
```

```output
Exception: (return …) cannot be the `body` of (fn-expr …), which takes Value. This body is a value, and a statement cannot stand in it. To run steps, write `fn-do`, or `:do` in a closure, and return with `(return value)`; an expression function can also run them first in a `(do …)` in `(before …)`. Otherwise replace the statement with a value.
<no file:7>
      (return 42)
      ^
```

### more than ten errors

```brix crash
(main
  (print gone-one)
  (print gone-two)
  (print gone-three)
  (print gone-four)
  (print gone-five)
  (print gone-six)
  (print gone-seven)
  (print gone-eight)
  (print gone-nine)
  (print gone-ten)
  (print gone-eleven)
)
```

```output
Exception: Name "gone-one" not found
<no file:3>
      (print gone-one)
             ^^^^^^^^

Exception: Name "gone-two" not found
<no file:4>
      (print gone-two)
             ^^^^^^^^

Exception: Name "gone-three" not found
<no file:5>
      (print gone-three)
             ^^^^^^^^^^

Exception: Name "gone-four" not found
<no file:6>
      (print gone-four)
             ^^^^^^^^^

Exception: Name "gone-five" not found
<no file:7>
      (print gone-five)
             ^^^^^^^^^

Exception: Name "gone-six" not found
<no file:8>
      (print gone-six)
             ^^^^^^^^

Exception: Name "gone-seven" not found
<no file:9>
      (print gone-seven)
             ^^^^^^^^^^

Exception: Name "gone-eight" not found
<no file:10>
      (print gone-eight)
             ^^^^^^^^^^

Exception: Name "gone-nine" not found
<no file:11>
      (print gone-nine)
             ^^^^^^^^^

Exception: Name "gone-ten" not found
<no file:12>
      (print gone-ten)
             ^^^^^^^^

1 more error is not shown
```
