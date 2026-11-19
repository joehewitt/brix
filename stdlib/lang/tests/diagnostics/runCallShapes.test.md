# Call shapes

A call that matches no declaration shows the arguments given and the shapes declared

### function arguments

```brix crash
(main
  (print (call area "a"))
)

(fn-expr + area I32
  (param width I32)
  (param height I32)
  (* width height)
)
```

```output
Exception: No function "area" accepts (String); it is declared as "area(width I32, height I32)"
<no file:3>
      (print (call area "a"))
             ^^^^^^^^^^^^^^^
```

### labeled arguments

```brix crash
(main
  (print (call move (:to 1)))
)

(fn-expr + move I32
  (param by I32)
  by
)
```

```output
Exception: No function "move" accepts to I64; it is declared as "move(by I32)"
<no file:3>
      (print (call move (:to 1)))
             ^^^^^^^^^^^^^^^^^^^
```

### method arguments

```brix crash
(main
  (print (call (. (call Counter) add) 1 2))
)

(type + Counter
  (prop + count _ (= 0))
  (fn-expr + add I32
    (param amount I32)
    (+ count amount)
  )
)
```

```output
Exception: No method "add" of Counter accepts (I64, I64); it is declared as "add(amount I32)"
<no file:3>
      (print (call (. (call Counter) add) 1 2))
             ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### extension method arguments

```brix crash
(main
  (print (call (. (call Counter) add) 1 2))
)

(type + Counter
  (prop + count _ (= 0))
)

(extend + Counter
  (fn-expr + add I32
    (param amount I32)
    (+ count amount)
  )
)
```

```output
Exception: No method "add" of Counter accepts (I64, I64); it is declared as "add(amount I32)"
<no file:3>
      (print (call (. (call Counter) add) 1 2))
             ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### tuple constructor

```brix crash
(main
  (print (. (call Pair 1 2 3) x))
)

(type + Pair (is Tuple)
  (prop + x I32)
  (prop + y I32)
)
```

```output
Exception: No constructor of Pair accepts (I64, I64, I64); its declared constructors are "Pair(x I32, y I32)"
<no file:3>
      (print (. (call Pair 1 2 3) x))
                      ^^^^
```

### class constructor

```brix crash
(main
  (print (. (call Box 1 2) label))
)

(type + Box
  (prop + label String)
  (fn-do + This _
    (param label String)
    (= (. this label) label)
  )
)
```

```output
Exception: No constructor of Box accepts (I64, I64); its declared constructors are "Box(label String)"
<no file:3>
      (print (. (call Box 1 2) label))
                      ^^^
```
