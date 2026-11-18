# Optional initialization join

```yumi
/core/array-builder

. builder ArrayBuilder'Int?
if !builder
    builder = ArrayBuilder'Int()
builder.append(7)
-- builder.length
```

```output
1
```

```yumi
/core/array-builder

-- finish(null).length
-- finish(ArrayBuilder'Int()).length

━━━━━━━━━━━━━━━━━━━━
+ finish Array'Int
    - builder ArrayBuilder'Int?

|? builder
    builder.finish-mutable()
??
    Array'Int(0 as CPointer'Int, 0)
```

```output
0
0
```

```yumi
-- choose(7)
-- choose(null)

━━━━━━━━━━━━━━━━━━━━
+ do choose Int
    - value Int?

|? value
    = value
??
    = 0
```

```output
7
0
```

```yumi
. value Int?
value = 7
if value
    value = null
    -- type-of value
-- value ?? 0
value = 8
-- value + 1
value = null
-- value ?? 0
```

```output
[TypeInfo Optional'I64]
0
9
0
```

```yumi
-- choose(true)
-- choose(false)

━━━━━━━━━━━━━━━━━━━━
+ do choose Int
    - first Bool

. value Int?
if first
    value = 7
else
    value = 8
= value + 1
```

```output
8
9
```

```yumi
-- choose(null)
-- choose(7)
-- choose(8)

━━━━━━━━━━━━━━━━━━━━
+ choose Int
    - value Int?

|? !value
    0
|? value + 1 == 8
    1
??
    value
```

```output
0
1
8
```

```yumi
. value Int?
for index in 0 .. < 0
    value = index
-- type-of value
-- value ?? 0

value = 7
for index in 0 .. < 2
    -- value ?? 0
    value = null
-- type-of value
```

```output
[TypeInfo Optional'I64]
0
7
0
[TypeInfo Optional'I64]
```

```yumi
. value Int?
value = 7
while value
    -- value + 1
    value = null
-- value ?? 0
-- type-of value
```

```output
8
0
[TypeInfo Optional'I64]
```

```yumi
. value Int?
value = 7
. clear = => do
    value = null
clear()
-- value ?? 0
-- type-of value
```

```output
0
[TypeInfo Optional'I64]
```

```yumi crash
/core/array-builder

. builder ArrayBuilder'Int?
builder.reserve(8)
```

```output
Exception: Method "reserve" is called on ArrayBuilder'I64'GCBacking?, which may be null; unwrap it first with ! or ?.
<no file:5>
    builder.reserve(8)
    ^^^^^^^^^^^^^^^^^^
```

```yumi
. limit Int?
limit = 2
for index in 0 .. < limit
    -- index
    limit = null
-- limit ?? 0
```

```output
0
1
0
```

```yumi
-- choose(true)
-- choose(false)

━━━━━━━━━━━━━━━━━━━━
+ do choose Int
    - first Bool

. value Int?
if first
    value = 7
-- type-of value
= value ?? 0
```

```output
[TypeInfo Optional'I64]
7
[TypeInfo Optional'I64]
0
```

```yumi
-- choose(true)
-- choose(false)

━━━━━━━━━━━━━━━━━━━━
+ do choose Int
    - first Bool

|? first
    = 7
= 8
```

```output
7
8
```

```yumi
-- choose(7)
-- choose(null)

━━━━━━━━━━━━━━━━━━━━
+ do choose Int
    - input Int?

. value Int? = input
if value
    -- value + 1
else
    value = 8
. result = value + 1
value = null
-- value ?? 0
= result
```

```output
8
0
8
0
9
```

### A table of conditions run as a statement, with arms that return, beside if and else arms that print

```yumi
-- classify(5)
-- classify(-5)
-- classify(0)
show(1)
show(2)
show(3)

━━━━━━━━━━━━━━━━━━━━
+ do classify String
    - n Int

|? n > 0
    = "plus"
|? n < 0
    = "minus"
= "zero"

━━━━━━━━━━━━━━━━━━━━
+ do show
    - n Int

if n == 1
    -- "one"
else if n == 2
    -- "two"
else
    -- "many"
-- "done $n"
```

```output
plus
minus
zero
one
done 1
two
done 2
many
done 3
```

### A table of conditions over optionals narrows each in its arm

```yumi
-- first(3, null)
-- first(null, 4)
-- first(null, null)
-- first(0, 9)

━━━━━━━━━━━━━━━━━━━━
+ do first Int
    - a Int?
    - b Int?

|? a
    = a + 1
|? b
    = b + 2
= 0
```

```output
4
6
0
1
```
