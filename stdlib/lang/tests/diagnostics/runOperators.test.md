# Operators

Operator and conversion errors name the operator and the types involved

### binary operator

```yumi crash
-- "a" - 1
```

```output
Exception: Operator "-" is not defined for String and I64
<no file:2>
    -- "a" - 1
       ^^^^^^^
```

### unary operator

```yumi crash
-- -"a"
```

```output
Exception: Operator "-" is not defined for String
<no file:2>
    -- -"a"
       ^^^^
```

### compound assignment

```yumi crash
. items = [1]
items += Pair(1, 2)

━━━━━━━━━━━━━━━━━━━━
+ Pair is Tuple

+ x I32
+ y I32
```

```output
Exception: Operator "+=" is not defined for [I64] and Pair
<no file:3>
    items += Pair(1, 2)
    ^^^^^^^^^^^^^^^^^^^
```

### function value as an operand

```yumi crash
-- !Box().is-empty

━━━━━━━━━━━━━━━━━━━━
+ Box

────────────────────
+ is-empty Bool
    - strict Bool

strict
```

```output
Exception: Operator "!" is not defined for function is-empty; it is a function, so call it first
<no file:2>
    -- !Box().is-empty
       ^^^^^^^^^^^^^^^
```

### explicit cast

```yumi crash
-- Pair(1, 2) as I32

━━━━━━━━━━━━━━━━━━━━
+ Pair is Tuple

+ x I32
+ y I32
```

```output
Exception: Pair cannot be cast to I32; there is no conversion between them
<no file:2>
    -- Pair(1, 2) as I32
       ^^^^^^^^^^^^^^^^^
```

### cast between arrays of items of different sizes

```yumi crash
. small = I32[2]
-- (small as I64[]).length
```

```output
Exception: Array'I32 cannot be cast to Array'I64; their items are 4 and 8 bytes, and a cast keeps the count of items
<no file:3>
    -- (small as I64[]).length
        ^^^^^^^^^^^^^^
```

### implicit conversion

```yumi crash
. box = Box()
box.count = Pair(1, 2)

━━━━━━━━━━━━━━━━━━━━
+ Box

+ count = 0

━━━━━━━━━━━━━━━━━━━━
+ Pair is Tuple

+ x I32
+ y I32
```

```output
Exception: Expected I64 here, but this is Pair, and there is no conversion between them
<no file:3>
    box.count = Pair(1, 2)
    ^^^^^^^^^^^^^^^^^^^^^^
```

### optional value used as its value

```yumi crash
. maybe I32? = 4
. count I32 = maybe
```

```output
Exception: Expected I32 here, but this is I32?, which may hold nothing; write "!" after it to unwrap it, or "?? fallback" to give a value for nothing
<no file:3>
    . count I32 = maybe
            ^^^
```

### number used as text

```yumi crash
. label String = 42
```

```output
Exception: Expected String here, but this is I64, which becomes text only inside a text or a print, or when asked; write "as String", or write it inside a text, as in "$name"
<no file:2>
    . label String = 42
            ^^^^^^
```

### base class used as a subclass

```yumi crash
. pet Animal = Dog()
. dog Dog = pet

━━━━━━━━━━━━━━━━━━━━
+ Animal

━━━━━━━━━━━━━━━━━━━━
+ Dog is Animal
```

```output
Exception: Expected Dog here, but this is Animal, which is not always Dog; match it with "| name Dog", or write "as Dog"
<no file:3>
    . dog Dog = pet
          ^^^
```

### float used as an integer

```yumi crash
. ratio = 2.5
. whole I32 = ratio
```

```output
Exception: Expected I32 here, but this is F64, which becomes I32 only when asked; write "as I32"
<no file:3>
    . whole I32 = ratio
            ^^^
```

### number compared with a Bool

```yumi crash
-- 1 == true
```

```output
Exception: Operator "==" is not defined for I64 and Bool
<no file:2>
    -- 1 == true
       ^^^^^^^^^
```
