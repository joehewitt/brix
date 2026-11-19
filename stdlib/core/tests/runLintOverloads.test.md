# Lint overloads

```brix
(import ./one)
(import ./two)
(blank)

(main
  (print (call pick 10))
  (print (call pick 10 20))
  (let partial (call pick ...))
  (print (call partial 10))
  (print (call partial 10 20))
)
```

```brix file=one.bx
(fn-expr + pick _
  (param value Int)
  1
)
```

```brix file=two.bx
(fn-expr + pick _
  (param value Int)
  (param other Int)
  2
)
```

```output
1
2
1
2
```

```brix
(import ./two)
(import ./one)
(blank)

(main
  (print (call pick 10))
  (print (call pick 10 20))
  (let partial (call pick ...))
  (print (call partial 10))
  (print (call partial 10 20))
)
```

```brix file=one.bx
(fn-expr + pick _
  (param value Int)
  1
)
```

```brix file=two.bx
(fn-expr + pick _
  (param value Int)
  (param other Int)
  2
)
```

```output
1
2
1
2
```
