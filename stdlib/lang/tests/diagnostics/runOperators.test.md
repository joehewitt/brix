# Operators

Operator and conversion errors name the operator and the types involved

### binary operator

```brix crash
(main
  (print (- "a" 1))
)
```

```output
Exception: Operator "-" is not defined for String and I64
<no file:3>
      (print (- "a" 1))
             ^^^^^^^^^
```

### unary operator

```brix crash
(main
  (print (- "a"))
)
```

```output
Exception: Operator "-" is not defined for String
<no file:3>
      (print (- "a"))
             ^^^^^^^
```

### compound assignment

```brix crash
(main
  (let items (list 1))
  (+= items (call Pair 1 2))
)

(type + Pair (is Tuple)
  (prop + x I32)
  (prop + y I32)
)
```

```output
Exception: Operator "+=" is not defined for [I64] and Pair
<no file:4>
      (+= items (call Pair 1 2))
      ^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### function value as an operand

```brix crash
(main
  (print (not (. (call Box) is-empty)))
)

(type + Box
  (fn-expr + is-empty Bool
    (param strict Bool)
    strict
  )
)
```

```output
Exception: Operator "!" is not defined for function is-empty; it is a function, so call it first
<no file:3>
      (print (not (. (call Box) is-empty)))
             ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### explicit cast

```brix crash
(main
  (print (as (call Pair 1 2) I32))
)

(type + Pair (is Tuple)
  (prop + x I32)
  (prop + y I32)
)
```

```output
Exception: Pair cannot be cast to I32; there is no conversion between them
<no file:3>
      (print (as (call Pair 1 2) I32))
             ^^^^^^^^^^^^^^^^^^^^^^^^
```

### cast between arrays of items of different sizes

```brix crash
(main
  (let small ([] I32 2))
  (print (. (as small ([] I64)) length))
)
```

```output
Exception: Array'I32 cannot be cast to Array'I64; their items are 4 and 8 bytes, and a cast keeps the count of items
<no file:4>
      (print (. (as small ([] I64)) length))
                ^^^^^^^^^^^^^^^^^^^
```

### implicit conversion

```brix crash
(main
  (let box (call Box))
  (= (. box count) (call Pair 1 2))
)

(type + Box
  (prop + count _ (= 0))
)

(type + Pair (is Tuple)
  (prop + x I32)
  (prop + y I32)
)
```

```output
Exception: Expected I64 here, but this is Pair, and there is no conversion between them
<no file:4>
      (= (. box count) (call Pair 1 2))
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### optional value used as its value

```brix crash
(main
  (let maybe (? I32) 4)
  (let count I32 maybe)
)
```

```output
Exception: Expected I32 here, but this is I32?, which may hold nothing; write "(!! maybe)" to unwrap it, or "(?? maybe fallback)" to give a value for nothing
<no file:4>
      (let count I32 maybe)
                 ^^^
```

### number used as text

```brix crash
(main
  (let label String 42)
)
```

```output
Exception: Expected String here, but this is I64, which becomes text only inside a text or a print, or when asked; write "(as 42 String)", or put it in a "(str …)"
<no file:3>
      (let label String 42)
                 ^^^^^^
```

### base class used as a subclass

```brix crash
(main
  (let pet Animal (call Dog))
  (let dog Dog pet)
)

(type + Animal)

(type + Dog (is Animal))
```

```output
Exception: Expected Dog here, but this is Animal, which is not always Dog; match it with "(case (: name Dog) …)", or write "(as pet Dog)"
<no file:4>
      (let dog Dog pet)
               ^^^
```

### float used as an integer

```brix crash
(main
  (let ratio 2.5)
  (let whole I32 ratio)
)
```

```output
Exception: Expected I32 here, but this is F64, which becomes I32 only when asked; write "(as ratio I32)"
<no file:4>
      (let whole I32 ratio)
                 ^^^
```

### number compared with a Bool

```brix crash
(main
  (print (== 1 true))
)
```

```output
Exception: Operator "==" is not defined for I64 and Bool
<no file:3>
      (print (== 1 true))
             ^^^^^^^^^^^
```
