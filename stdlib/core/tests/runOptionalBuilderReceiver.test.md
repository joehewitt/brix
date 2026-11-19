# Optional builder receiver

```brix
(import /core/array-builder)
(blank)

(main
  (let builder (? (of ArrayBuilder Int)))
  (= builder (call (of ArrayBuilder Int)))
  (call (. builder reserve) 8)
)
```

```brix
(import /core/array-builder)
(blank)

(main
  (let builder (? (of ArrayBuilder Int)) (call (of ArrayBuilder Int)))
  (if builder
    (call (. builder append) 7)
    (print (. builder length))
  )
  (print (. (!! builder) length))
)
```

```output
1
1
```

```brix
(import /core/array-builder)
(blank)

(main
  (let builder (? (of ArrayBuilder Int)) (call (of ArrayBuilder Int)))
  (call (. (!! builder) append) 7)
  (print (. (!! builder) length))
)
```

```output
1
```

```brix
(main
  (let value (? Int))
  (= value 7)
  (print (type-of value))
  (if value
    (print (type-of value))
  )
)
```

```output
[TypeInfo I64]
[TypeInfo I64]
```
