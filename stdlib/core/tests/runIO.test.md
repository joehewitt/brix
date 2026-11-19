# Io

```brix
(main
  (<-* (call Receiver) (list 1 2 3))
)

(type + Receiver
  (fn-do + (infix this <- (param item Int)) _
    (print (str "got " item))
  )
)

(fn-do + river _
  (yield 10)
  (yield 20)
  (yield 30)
)
```

```output
got 1
got 2
got 3
```

```brix
(main
  (<-* (call Receiver) (call river))
)

(type + Receiver
  (fn-do + (infix this <- (param item Int)) _
    (print (str "got " item))
  )
)

(fn-do + river _
  (yield 10)
  (yield 20)
  (yield 30)
)
```

```output
got 10
got 20
got 30
```

```brix
(main
  (print (<- (as (..< 101 103) Iterable)))
)
```

```output
101
```
