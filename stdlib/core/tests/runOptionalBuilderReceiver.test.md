# Optional builder receiver

```yumi
/core/array-builder

. builder ArrayBuilder'Int?
builder = ArrayBuilder'Int()
builder.reserve(8)
```

```yumi
/core/array-builder

. builder ArrayBuilder'Int? = ArrayBuilder'Int()
if builder
    builder.append(7)
    -- builder.length
-- builder!.length
```

```output
1
1
```

```yumi
/core/array-builder

. builder ArrayBuilder'Int? = ArrayBuilder'Int()
builder!.append(7)
-- builder!.length
```

```output
1
```

```yumi
. value Int?
value = 7
-- type-of value
if value
    -- type-of value
```

```output
[TypeInfo I64]
[TypeInfo I64]
```
