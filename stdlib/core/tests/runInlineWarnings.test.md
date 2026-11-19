# Inline warnings

```brix warn
(import /core)
(blank)

(main
  (print (call recursive-warning 2))
)

(fn-expr + recursive-warning Int @inline
  (param value Int)
  (match value
    (case 0
      0
    )
    (default
      (+ (call recursive-warning (- value 1)) 1)
    )
  )
)
```

```stderr
Warning: Cannot inline __source__:recursive-warning: recursive function; @inline declaration at no file:9
<no file:16>
          (+ (call recursive-warning (- value 1)) 1)
             ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Warning: Cannot inline __source__:recursive-warning: recursive function; @inline declaration at no file:9
<no file:6>
      (print (call recursive-warning 2))
             ^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```output
2
```

```brix warn
(import /core)
(blank)

(main
  (print (call even-warning 2))
)

(fn-do + even-warning Bool @inline
  (param value Int)
  (if (== value 0)
    (return true)
  )
  (return (call odd-warning (- value 1)))
)

(fn-do + odd-warning Bool @inline
  (param value Int)
  (if (== value 0)
    (return false)
  )
  (return (call even-warning (- value 1)))
)
```

```stderr
Warning: Cannot inline __source__:even-warning: recursive function; @inline declaration at no file:9
<no file:22>
      (return (call even-warning (- value 1)))
              ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Warning: Cannot inline __source__:odd-warning: recursive function; @inline declaration at no file:17
<no file:14>
      (return (call odd-warning (- value 1)))
              ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Warning: Cannot inline __source__:even-warning: recursive function; @inline declaration at no file:9
<no file:6>
      (print (call even-warning 2))
             ^^^^^^^^^^^^^^^^^^^^^
```

```output
true
```

```brix warn
(import /core)
(blank)

(main
  (for (in value (call generator-warning))
    (print value)
  )
)

(fn-do + generator-warning _ @inline
  (yield 7)
)
```

```stderr
```

```output
7
```

```brix warn
(import /core)

(main
  (print (call dispatch (call Child)))
)

(type + Base
  (fn-expr + value Int @inline
    1
  )
)

(type + Child (is Base)
  (fn-expr + value Int
    2
  )
)

(fn-expr + dispatch Int
  (param object Base)
  (call (. object value))
)
```

```stderr
Warning: Cannot inline __source__:Base:value: unresolved target: virtual dispatch; @inline declaration at no file:9
<no file:22>
      (call (. object value))
      ^^^^^^^^^^^^^^^^^^^^^^^
```

```output
2
```

```brix warn
(import /core)
(blank)

(main
  (print (call successful-warning 2))
)

(fn-expr + successful-warning Int @inline
  (param value Int)
  (+ value 1)
)

(fn-expr + unused-warning Int @inline
  (param value Int)
  (call unused-warning value)
)
```

```stderr
```

```output
3
```
