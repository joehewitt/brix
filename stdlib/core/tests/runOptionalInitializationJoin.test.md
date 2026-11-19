# Optional initialization join

```brix
(import /core/array-builder)
(blank)

(main
  (let builder (? (of ArrayBuilder Int)))
  (if (not builder)
    (= builder (call (of ArrayBuilder Int)))
  )
  (call (. builder append) 7)
  (print (. builder length))
)
```

```output
1
```

```brix
(import /core/array-builder)
(blank)

(main
  (print (. (call finish null) length))
  (print (. (call finish (call (of ArrayBuilder Int))) length))
)

(fn-expr + finish (of Array Int)
  (param builder (? (of ArrayBuilder Int)))
  (case? builder
    (call (. builder finish-mutable))
  )
  (default
    (call (of Array Int) (as 0 (of CPointer Int)) 0)
  )
)
```

```output
0
0
```

```brix
(main
  (print (call choose 7))
  (print (call choose null))
)

(fn-do + choose Int
  (param value (? Int))
  (case? value
    (return value)
  )
  (default
    (return 0)
  )
)
```

```output
7
0
```

```brix
(main
  (let value (? Int))
  (= value 7)
  (if value
    (= value null)
    (print (type-of value))
  )
  (print (?? value 0))
  (= value 8)
  (print (+ value 1))
  (= value null)
  (print (?? value 0))
)
```

```output
[TypeInfo Optional'I64]
0
9
0
```

```brix
(main
  (print (call choose true))
  (print (call choose false))
)

(fn-do + choose Int
  (param first Bool)
  (let value (? Int))
  (if first
    (= value 7)
    (else
      (= value 8)
    )
  )
  (return (+ value 1))
)
```

```output
8
9
```

```brix
(main
  (print (call choose null))
  (print (call choose 7))
  (print (call choose 8))
)

(fn-expr + choose Int
  (param value (? Int))
  (case? (not value)
    0
  )
  (case? (== (+ value 1) 8)
    1
  )
  (default
    value
  )
)
```

```output
0
1
8
```

```brix
(main
  (let value (? Int))
  (for (in index (..< 0 0))
    (= value index)
  )
  (print (type-of value))
  (print (?? value 0))
  (blank)
  (= value 7)
  (for (in index (..< 0 2))
    (print (?? value 0))
    (= value null)
  )
  (print (type-of value))
)
```

```output
[TypeInfo Optional'I64]
0
7
0
[TypeInfo Optional'I64]
```

```brix
(main
  (let value (? Int))
  (= value 7)
  (while value
    (print (+ value 1))
    (= value null)
  )
  (print (?? value 0))
  (print (type-of value))
)
```

```output
8
0
[TypeInfo Optional'I64]
```

```brix
(main
  (let value (? Int))
  (= value 7)
  (let clear
    (=> () :do
      (block
        (= value null)
      )
    )
  )
  (call clear)
  (print (?? value 0))
  (print (type-of value))
)
```

```output
0
[TypeInfo Optional'I64]
```

```brix crash
(import /core/array-builder)
(blank)

(main
  (let builder (? (of ArrayBuilder Int)))
  (call (. builder reserve) 8)
)
```

```output
Exception: Method "reserve" is called on ArrayBuilder'I64'GCBacking?, which may be null; unwrap it first with "(!! …)" or "(?. …)"
<no file:7>
      (call (. builder reserve) 8)
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix
(main
  (let limit (? Int))
  (= limit 2)
  (for (in index (..< 0 limit))
    (print index)
    (= limit null)
  )
  (print (?? limit 0))
)
```

```output
0
1
0
```

```brix
(main
  (print (call choose true))
  (print (call choose false))
)

(fn-do + choose Int
  (param first Bool)
  (let value (? Int))
  (if first
    (= value 7)
  )
  (print (type-of value))
  (return (?? value 0))
)
```

```output
[TypeInfo Optional'I64]
7
[TypeInfo Optional'I64]
0
```

```brix
(main
  (print (call choose true))
  (print (call choose false))
)

(fn-do + choose Int
  (param first Bool)
  (case? first
    (return 7)
  )
  (return 8)
)
```

```output
7
8
```

```brix
(main
  (print (call choose 7))
  (print (call choose null))
)

(fn-do + choose Int
  (param input (? Int))
  (let value (? Int) input)
  (if value
    (print (+ value 1))
    (else
      (= value 8)
    )
  )
  (let result (+ value 1))
  (= value null)
  (print (?? value 0))
  (return result)
)
```

```output
8
0
8
0
9
```

### A table of conditions run as a statement, with arms that return, beside if and else arms that print

```brix
(main
  (print (call classify 5))
  (print (call classify -5))
  (print (call classify 0))
  (call show 1)
  (call show 2)
  (call show 3)
)

(fn-do + classify String
  (param n Int)
  (case? (> n 0)
    (return "plus")
  )
  (case? (< n 0)
    (return "minus")
  )
  (return "zero")
)

(fn-do + show _
  (param n Int)
  (if (== n 1)
    (print "one")
    (else-if (== n 2)
      (print "two")
    )
    (else
      (print "many")
    )
  )
  (print (str "done " n))
)
```

```output
plus
minus
zero
one
done 1
two
done 2
many
done 3
```

### A table of conditions over optionals narrows each in its arm

```brix
(main
  (print (call first 3 null))
  (print (call first null 4))
  (print (call first null null))
  (print (call first 0 9))
)

(fn-do + first Int
  (param a (? Int))
  (param b (? Int))
  (case? a
    (return (+ a 1))
  )
  (case? b
    (return (+ b 2))
  )
  (return 0)
)
```

```output
4
6
0
1
```
