# Array builder property

```yumi
/core/array-builder

. holder = Holder()
holder.append(7)
holder.append(9)
-- holder.builder.length
. result = holder.finish()
-- result.length
-- result

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ builder := ArrayBuilder'I32()

────────────────────
+ do append
    - value I32

builder.append(value)

────────────────────
+ do finish ImmutableArray'I32

= builder.finish-immutable()
```

```output
2
2
7, 9
```

```yumi
/core/array-builder

. holder = Derived() as Base
holder.builder.append(7)
-- holder.builder.length
-- holder.builder.finish-immutable()

━━━━━━━━━━━━━━━━━━━━
+ Base

+ builder := ArrayBuilder'I32()

━━━━━━━━━━━━━━━━━━━━
+ Derived is Base

+ builder := seeded()

━━━━━━━━━━━━━━━━━━━━
+ do seeded ArrayBuilder'I32

. result = ArrayBuilder'I32()
result.append(9)
= result
```

```output
2
9, 7
```

```yumi
/core/array-builder

. holder = Holder()
holder.builder.append(7)
. copy = holder.builder
copy.append(9)
-- holder.builder.length
-- copy.length

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ builder := ArrayBuilder'I32()
```

```output
2
2
```

```yumi
/core/array-builder

. holder = Holder()
holder.builder.reserve(32)
holder.builder.append(7)
-- holder.builder.length
-- holder.builder.finish-immutable()
-- holder.builder.length

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ builder ArrayBuilder'I32

────────────────────
+ This

builder = ArrayBuilder'I32()
```

```output
1
7
0
```

```yumi
/core/array-builder

. builder = ArrayBuilder'U32()
builder.append(1)
fill(builder)
-- builder.length
-- builder.finish-immutable()

━━━━━━━━━━━━━━━━━━━━
_ do fill
    - output ArrayBuilder'U32

output.append(2)
output.append(3)
```

```output
3
1, 2, 3
```

```yumi
/core/array-builder

. builder = ArrayBuilder'U32()
builder.reserve(8)
builder.append(1)
. copy = builder
builder.append(2)
. frozen = builder.finish-immutable()
copy.append(7)
-- frozen
-- copy.length
-- builder.finish-immutable()
```

```output
1, 2
1
7
```
