# If

```brix
(main
  (print (if-expr true 1 42))
)
```

```output
1
```

```brix
(main
  (let x 2)
  (if (> x 1)
    (print "yes")
    (else
      (print "no")
    )
  )
  (print "done")
)
```

```output
yes
done
```

```brix
(main
  (call check 1)
  (call check 11)
  (call check 3)
)

(fn-do + check _
  (param x I32)
  (if (> x 10)
    (print "a")
    (else-if (>= x 2)
      (print "b")
    )
    (else
      (print "c")
    )
  )
  (print "done")
)
```

```output
c
done
a
done
b
done
```

```brix
(main
  (let x 6)
  (print (if-expr (> x 10) 3 (only-if 2 (if (> x 5)))))
)
```

```output
2
```

```brix
(main
  (let x 1)
  (print (only-if 1 (if (> x 10))))
  (blank)
  (= x 11)
  (print (only-if 1 (if (> x 10))))
  (blank)
  (= x 1)
  (print (if-expr (> x 10) 1 3))
  (blank)
  (= x 11)
  (print (if-expr (> x 10) 1 3))
  (blank)
  (= x 1)
  (print (if-expr (> x 10) 1 (only-if 2 (if (> x 5)))))
  (blank)
  (= x 6)
  (print (if-expr (> x 10) 1 (only-if 2 (if (> x 5)))))
  (blank)
  (= x 11)
  (print (if-expr (> x 10) 1 (if-expr (> x 5) 2 3)))
  (blank)
  (= x 6)
  (print (if-expr (> x 10) 1 (if-expr (> x 5) 2 3)))
  (blank)
  (= x 1)
  (print (if-expr (> x 10) 1 (if-expr (> x 5) 2 3)))
)
```

```output
unknown
1
3
1
unknown
2
1
2
3
```

```brix
(main
  (let x (? Int))
  (print (?? x 2))
  (= x 1)
  (print (?? x 2))
)
```

```output
2
1
```

```brix
(main
  (let x 0)
  (let y 1)
  (print (and x x))
  (print (and x y))
  (print (and y y))
  (print (and y x))
)
```

```output
false
false
true
false
```

```brix
(main
  (let x 0)
  (let y 1)
  (print (or x x))
  (print (or x y))
  (print (or y y))
  (print (or y x))
)
```

```output
false
true
true
true
```

### If blocks have their own scope

```brix
(main
  (let a 1)
  (if true
    (let a 2)
    (print a)
  )
  (if false
    (print "never")
    (else
      (let a 2)
      (print a)
    )
  )
  (print a)
)
```

```output
2
2
1
```

```brix crash
(main
  (let x
    (block
      (if (> 1 2)
        1
        (else
          2
        )
      )
    )
  )
  (print x)
)
```

```output
Exception: (if …) cannot be a `row` of (block …). To give one value or another, write `(if-expr condition value otherwise)`. To run steps, write `(if condition step … (else step …))` in a `fn-do` body.
<no file:5>
          (if (> 1 2)
          ^
```

```brix
(main
  (print (only-if 1 (if true)))
)
```

```output
1
```

```brix
(main
  (print (only-if 1 (if false)))
)
```

```output
unknown
```

```brix
(main
  (print
    (block
      (case? true
        1
      )
      (default
        2
      )
    )
  )
)
```

```output
1
```

```brix
(main
  (print (call test))
  (print (call test 2))
)

(fn-expr + test _
  (param t (? Int) (= null))
  (case? (> 0 1)
    1
  )
  (case? t)
  (default
    (inline 3)
  )
)
```

```output
3
2
```

```brix
(main
  (print
    (block
      (case? bar
        foo
        (lazy foo 2)
      )
      (lazy bar 1)
    )
  )
)
```

```output
2
```

```brix
(main
  (let a (? Int) 42)
  (if a
    (print a)
    (print (type-of a))
  )
  (print (type-of a))
)
```

```output
42
[TypeInfo I64]
[TypeInfo Optional'I64]
```

### `else if` is tried where the branches before it are not taken, and `else` where none is.

```brix
(main
  (let x 5)
  (if (== x 1)
    (print 1)
    (else-if (== x 2)
      (print 2)
    )
    (else-if (== x 5)
      (print 5)
    )
    (else
      (print 0)
    )
  )
  (if (> x 9)
    (print "big")
    (else
      (print "small")
    )
  )
  (print "end")
)
```

```output
5
small
end
```

### A condition is a Bool, a number that is not zero, or a text that is not empty.

```brix
(main
  (let n 3)
  (let z 0.0)
  (let s "a")
  (let e "")
  (if n
    (print "number")
  )
  (if z
    (print "zero")
    (else
      (print "no zero")
    )
  )
  (if 5u8
    (print "byte")
  )
  (if s
    (print "text")
  )
  (if e
    (print "empty")
    (else-if (not e)
      (print "no text")
    )
  )
  (if (and s (not e))
    (print "both")
  )
  (if (or e z)
    (print "either")
    (else
      (print "neither")
    )
  )
)
```

```output
number
no zero
byte
text
no text
both
neither
```

### A local assigned in a branch keeps what the branch gave it.

```brix
(main
  (let x 3)
  (if (> x 1)
    (= x 10)
    (if (> x 5)
      (+= x 1)
    )
    (else
      (= x 20)
    )
  )
  (print x)
)
```

```output
11
```

### A text tested as the condition of a value given only where it holds is asked whether it is empty

```brix
(main
  (print (call show "a"))
  (print (call show ""))
)

(fn-do + show _
  (param event String)
  (let x (only-if "event" (if event)))
  (return (?? x "none"))
)
```

```output
event
none
```

### A choice between a text and a truth value has no type in common.

```brix crash
(main
  (let flag true)
  (let chosen (if-expr flag "picked" false))
  (print chosen)
)
```

```output
Exception: The two sides of this "if" are String and Bool, which have no common type; convert them to one type
<no file:4>
      (let chosen (if-expr flag "picked" false))
                  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A choice between two numbers neither of which holds every value of the other has no type in common, though either converts to the other with `as`.

```brix crash
(main
  (let flag true)
  (let small I32 3)
  (let ratio F32 1.5)
  (print (if-expr flag small ratio))
)
```

```output
Exception: The two sides of this "if" are I32 and F32, which have no common type; convert them to one type
<no file:6>
      (print (if-expr flag small ratio))
             ^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A choice whose declared type is a union holds each side as its own case instead of joining them as text.

```brix
(main
  (let flag true)
  (print (call describe (call chosen flag)))
  (print (call describe (call chosen (not flag))))
)

(type _ Shape (is Union)
  (variant String)
  (variant (list String))
)

(fn-expr _ chosen Shape
  (param flag Bool)
  (if-expr flag (list "a" "b") "c")
)

(fn-expr _ describe String
  (param shape Shape)
  (match shape
    (case (: text String)
      (str "text " text)
    )
    (case (: items (list String))
      (str "items " (. items length))
    )
  )
)
```

```output
items 2
text c
```
