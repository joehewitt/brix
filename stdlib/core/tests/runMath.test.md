# Math

```brix
(main
  (print (+ 1 1))
)
```

```output
2
```

```brix
(main
  (print (call local-sqrt 9.0))
)

(fn-expr + sqrt F64
  (param value F64)
  (+ value 1.0)
)

(fn-expr + local-sqrt F64
  (param value F64)
  (call sqrt value)
)
```

```output
10
```
