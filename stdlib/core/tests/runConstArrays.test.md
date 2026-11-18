# Const arrays

### ConstArray values can be declared as global constants and indexed by position.

```yumi
+ values = ConstArray'U16
    - 0
    - 8
    - 16

+ flags = ConstArray'U8
    - 3
    - 5

-- values[0]
-- values[1]
-- values[2]
-- flags[0]
-- flags[1]
```

```output
0
8
16
3
5
```

### ConstArray values keep their own type identity and are read-only.

```yumi
+ values = ConstArray'U8
    - 65
    - 66

-- type-of values
-- (type-of values).read-only
-- values[0]
-- values[1]
```

```output
[TypeInfo ConstArray'U8]
true
65
66
```

### Inline constructor arguments produce a ConstArray too.

```yumi
. values = ConstArray'U16(3, 5, 8)

-- type-of values
-- values[0]
-- values[1]
-- values[2]
```

```output
[TypeInfo ConstArray'U16]
3
5
8
```

### Numeric items are cast to the declared item type before emission.

```yumi
+ bytes = ConstArray'U8
    - 258 as U8
    - -1 as U8

+ shorts = ConstArray'U16(65537 as U16, -1 as U16)

-- bytes[0]
-- bytes[1]
-- shorts[0]
-- shorts[1]
```

```output
2
255
1
65535
```

### ConstArray values support pointer-sized items such as strings.

```yumi
+ words = ConstArray'String
    - "alpha"
    - "beta"

-- words[0]
-- words[1]
```

```output
alpha
beta
```

### Multiple ConstArrays in one program get independent storage.

```yumi
+ left = ConstArray'U8(1, 2)
+ right = ConstArray'U8(10, 20)

-- left[0]
-- left[1]
-- right[0]
-- right[1]
```

```output
1
2
10
20
```

### Functions can accept ConstArrays without converting them to mutable pointers.

```yumi
+ values = ConstArray'U8(4, 5)

-- first(values)

━━━━━━━━━━━━━━━━━━━━
+ first
    - data ConstArray'U8

data[0]
```

```output
4
```

### ConstArray values are read-only through a global binding.

```yumi compile-error
+ values = ConstArray'U8(1, 2)

values[0] = 9
```

```output
Exception: ConstArray values are read-only
<no file:4>
    values[0] = 9
    ^^^^^^^^^
```

### ConstArray values are read-only through a local binding.

```yumi compile-error
. values = ConstArray'U8(1, 2)

values[0] = 9
```

```output
Exception: ConstArray values are read-only
<no file:4>
    values[0] = 9
    ^^^^^^^^^
```

### Incremental assignment is also rejected.

```yumi compile-error
. values = ConstArray'U8(1, 2)

values[0] += 1
```

```output
Exception: ConstArray values are read-only
<no file:4>
    values[0] += 1
    ^^^^^^^^^
```

### ConstArray values cannot be cast to mutable C pointers.

```yumi compile-error
+ values = ConstArray'U8(1, 2)

. pointer = values as CPointer'U8
```

```output
Exception: ConstArray'U8 cannot be cast to CPointer'U8; there is no conversion between them
<no file:4>
    . pointer = values as CPointer'U8
                ^^^^^^^^^^^^^^^^^^^^^
```

### ConstArray literals require exactly one item type.

```yumi compile-error
-- ConstArray()
```

```output
Exception: ConstArray requires one item type
<no file:2>
    -- ConstArray()
       ^^^^^^^^^^^^
```

### ConstArray items must be positional values.

```yumi compile-error
-- ConstArray'U8
    • first 1
```

```output
Exception: Illegal ConstArray item
<no file:3>
        • first 1
        ^^^^^^^^^
```

### ConstArray items must have a constant representation.

```yumi compile-error
+ values = ConstArray'U8(value())

-- values[0]

━━━━━━━━━━━━━━━━━━━━
+ value U8

42
```

```output
Exception: ConstArray item must be constant
<no file:2>
    + values = ConstArray'U8(value())
                             ^^^^^^^
```
