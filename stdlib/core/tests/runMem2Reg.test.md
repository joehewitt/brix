# Mem2reg

### mem2reg: else-if should re-read the original argument in the chained branch

```yumi
check(1)
check(11)
check(3)

━━━━━━━━━━━━━━━━━━━━
+ do check
    - x I32

if x > 10
    -- "a"
else if x >= 2
    -- "b"
else
    -- "c"
-- "done"
```

```output
c
done
a
done
b
done
```

### mem2reg: a join with one updated local should not clobber an unrelated local

```yumi
. cond = 1
. a = 10
. b = 20
if cond > 0
    a = 1
else
    a = 3
-- a
-- b
```

```output
1
20
```

### mem2reg: both sides of the same join should preserve the correct values

```yumi
. cond = 0
. a = 10
. b = 20
if cond > 0
    a = 1
else
    a = 3
-- a
-- b
```

```output
3
20
```

### mem2reg: a multi-variable join should keep the branch-specific pair

```yumi
. cond = 1
. a = 10
. b = 20
if cond > 0
    a = 1
    b = 2
else
    a = 3
    b = 4
-- a
-- b
```

```output
1
2
```

### mem2reg: the false branch of the same multi-variable join should also work

```yumi
. cond = 0
. a = 10
. b = 20
if cond > 0
    a = 1
    b = 2
else
    a = 3
    b = 4
-- a
-- b
```

```output
3
4
```

### mem2reg: a loop-carried single local should converge to the final value

```yumi
. a = 3
while a > 0
    a -= 1
-- a
```

```output
0
```

### mem2reg: multiple loop-carried locals should keep their own values

```yumi
. a = 2
. b = 20
while a > 0
    a -= 1
    b -= 10
-- a
-- b
```

```output
0
0
```

### mem2reg: a partially-defined optional local should not crash on the defined path

```yumi
. cond = 1
. a I32?
if cond > 0
    a = 1
-- a
```

```output
1
```

### mem2reg: a partially-defined optional local should not crash on the undefined path

```yumi
. cond = 0
. a I32?
if cond > 0
    a = 1
-- a
```

```output
unknown
```

### mem2reg: a while loop should serialize a two-variable swap as a parallel copy

```yumi
. a = 1
. b = 2
. i = 0
while i < 5
    . t = a
    a = b
    b = t
    i += 1
-- a
-- b
```

```output
2
1
```

### mem2reg: a for loop should serialize a two-variable swap as a parallel copy

```yumi
. a = 1
. b = 2
for i in 0 .. < 4
    . t = a
    a = b
    b = t
-- a
-- b
```

```output
1
2
```

### mem2reg: a while loop should break a three-variable phi cycle

```yumi
. a = 1
. b = 2
. c = 3
. i = 0
while i < 4
    . t = a
    a = b
    b = c
    c = t
    i += 1
-- a
-- b
-- c
```

```output
2
3
1
```

### mem2reg: a for loop should break a three-variable phi cycle

```yumi
. a = 1
. b = 2
. c = 3
for i in 0 .. < 5
    . t = a
    a = b
    b = c
    c = t
-- a
-- b
-- c
```

```output
3
1
2
```

### mem2reg: an acyclic phi overwrite chain should retain every pre-edge value

```yumi
. previous = 0
. current = 1
. i = 0
while i < 10
    . next = previous + current
    previous = current
    current = next
    i += 1
-- previous
-- current
```

```output
55
89
```

### mem2reg: register cycles and constant phi inputs should coexist on one edge

```yumi
. a = 1
. b = 2
. state = 0
for i in 0 .. < 3
    . t = a
    a = b
    b = t
    state = 7
-- a
-- b
-- state
```

```output
2
1
7
```

### mem2reg: self-copies and a split conditional-continue edge should not stall serialization

```yumi
. a = 1
. b = 2
. swaps = 0
for i in 0 .. < 5
    if i % 2 == 0
        continue

    . t = a
    a = b
    b = t
    swaps += 1
-- a
-- b
-- swaps
```

```output
1
2
2
```
