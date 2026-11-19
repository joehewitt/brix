# Descriptor names

```brix
(import ./lib/shapes)
(blank)

(main
  (let bases (list (call Base)))
  (print (call (. ([] bases 0) a)))
  (blank)
  (let list (list (call Sub1) (call Sub2)))
  (print (call (. ([] list 0) a)))
  (print (call (. ([] list 1) a)))
  (blank)
  (print (call shape-sides))
)

(type + Base
  (fn-expr + a _
    0
  )
)

(type + Base2 (is Base)
  (fn-expr + a _
    1
  )
)

(type + Sub1 (is Base2)
  (fn-expr + a _
    2
  )
)

(type + Sub2 (is Base2)
  (fn-expr + a _
    3
  )
)
```

```brix file=lib/shapes.bx
(type + Base (is Interface)
  (fn-expr + sides Int)
)

(type + Square
  (implements Base)
  (fn-expr + sides _
    4
  )
)

(fn-do + shape-sides Int
  (let shape (as (call Square) Base))
  (return (call (. shape sides)))
)
```

```output
0
2
3
4
```
