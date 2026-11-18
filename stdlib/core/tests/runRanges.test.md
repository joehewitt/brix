# Ranges

```yumi
. range = 0 .. < 1
-- range.start
-- range.end
-- range.increment
-- range.is-through
```

```output
0
1
1
false
```

```yumi
. range = 0 .. < 1 by 0.25
-- range.start
-- range.end
-- range.increment
-- range.is-through
```

```output
0
1
0.25
false
```

```yumi
. range = 0 .. 1
-- range.start
-- range.end
-- range.increment
-- range.is-through
```

```output
0
1
1
true
```

```yumi
for i in 0 .. < 3
    -- i
```

```output
0
1
2
```

```yumi
for i in 0 .. 3
    -- i
```

```output
0
1
2
3
```

```yumi
for i in 3 .. < 0
    -- i
```

```output
3
2
1
```

```yumi
for i in 3 .. 0
    -- i
```

```output
3
2
1
0
```

```yumi
for i in 126i8 .. 127i8
    -- i
```

```output
126
127
```

```yumi
for i in -127i8 .. -128i8
    -- i
```

```output
-127
-128
```

```yumi
-- ", ".join
    ** 1 .. < 5
```

```output
1, 2, 3, 4
```

```yumi
for i in 0 .. < 1 by 0.25
    -- i
```

```output
0
0.25
0.5
0.75
```

```yumi
for i in 0 .. 1 by 0.25
    -- i
```

```output
0
0.25
0.5
0.75
1
```

```yumi
for i in 1 .. 0 by 0.25
    -- i
```

```output
1
0.75
0.5
0.25
0
```

### A range runs from its start toward its end, so a step written with a sign counts by its size.

```yumi
for n in 10 .. 0 by -5
    -- n
. rising = 0 .. 10 by -5
-- rising.length
for x in 1.0 .. 0.0 by -0.5
    -- x
```

```output
10
5
0
3
1
0.5
0
```

## Forward conversion

```yumi
. range = 3 .. < 0
for i in range.forward()
    -- i
```

```output
1
2
3
```

```yumi
. range = 3 .. 0
for i in range.forward()
    -- i
```

```output
0
1
2
3
```

```yumi
. range = 1 .. 0 by 0.25
for i in range.forward()
    -- i
```

```output
0
0.25
0.5
0.75
1
```

```yumi
. range = 1 .. < 0 by 0.25
for i in range.forward()
    -- i
```

```output
0.25
0.5
0.75
1
```

```yumi
. iter = (0 .. < 3) as <Int>
for a in iter
    -- a
```

```output
0
1
2
```

## Relative positions

```yumi
. range = Position.first .. < Position.end
for i in range.normalize(0 .. < 3)
    -- i
```

```output
0
1
2
```

```yumi
. range = Position.first .. < Position.last
for i in range.normalize(0 .. < 3)
    -- i
```

```output
0
1
```

```yumi
. range = Position.first .. Position.last
for i in range.normalize(0 .. < 3)
    -- i
```

```output
0
1
2
```

```yumi
. range = Position.first .. < Position.end
for i in range.normalize(0 .. < 3)
    -- i
```

```output
0
1
2
```

```yumi
. range = Position.first .. Position.end
for i in range.normalize(0 .. < 3)
    -- i
```

```output
0
1
2
3
```

```yumi
. range = Position.first .. Position.end by 2
for i in range.normalize(0 .. < 5)
    -- i
```

```output
0
2
4
```

```yumi
. base = 0 .. < 1 by 0.25
. range = Position.end .. Position.first
for i in range.normalize(base)
    -- i
```

```output
4
3
2
1
0
```

```yumi
. base = 0 .. < 1 by 0.25
. range = Position.end .. < Position.first
for i in range.normalize(base)
    -- i
```

```output
4
3
2
1
```

```yumi
. base = 0 .. < 1 by 0.25
. range = Position.first .. Position.middle
for i in range.normalize(base)
    -- base[i]
```

```output
0
0.25
0.5
```

```yumi
-- (0 .. < 17 by 80).length
-- List(0 .. < 17 by 80)
-- (0 .. < 17 by 8).length
-- List(0 .. < 17 by 8)
-- List(17 .. < 0 by 8)
-- (0 .. < 16 by 8).length
-- List(0 .. < 16 by 8)
-- List(0 .. 16 by 8)
-- List(0 .. 17 by 8)
-- (0 .. < 0 by 8).length
-- List(0 .. 0 by 8)
-- (0u64 .. U64.max - 1 by U64.max).length
-- List(0.0 .. < 1.125 by 0.5)
-- List(0.0 .. 1.125 by 0.5)
-- List(0.0 .. 1.0 by 0.5)
-- List(1.125 .. < 0.0 by 0.5)
```

```output
1
0
3
0, 8, 16
17, 9, 1
2
0, 8
0, 8, 16
0, 8, 16
0
0
1
0, 0.5, 1
0, 0.5, 1
0, 0.5, 1
1.125, 0.625, 0.125
```

### The item of a loop over a range cannot be assigned.

```yumi crash
for i in 0 .. < 3
    i = 5
```

```output
Exception: Illegal to update const variables
<no file:3>
        i = 5
        ^^^^^
```

### The item of a loop over a range cannot be assigned by an operator.

```yumi crash
for i in 0 .. < 3
    i += 5
```

```output
Exception: Illegal to update const variables
<no file:3>
        i += 5
        ^^^^^^
```

### A range loop counts up or down, by the order of its ends. `..` takes its end and `.. <` stops before it.

```yumi
for i in 0 .. 3
    -- i
for i in 3 .. 0
    -- i
for i in 0 .. < 3
    -- i
for i in 3 .. < 0
    -- i
for i in 5 .. 5
    -- i
for i in 5 .. < 5
    -- i
for i in -2 .. 1
    -- i
-- "end"
```

```output
0
1
2
3
3
2
1
0
0
1
2
3
2
1
5
-2
-1
0
1
end
```

### A range that ends at the edge of its type does not wrap.

```yumi
for i in 126i8 .. 127i8
    -- i
for i in -127i8 .. -128i8
    -- i
for i in 254u8 .. 255u8
    -- i
for i in 1u8 .. 0u8
    -- i
for i in 2147483646 .. 2147483647
    -- i
for i in 18446744073709551614u64 .. 18446744073709551615u64
    -- i
for i in 1u64 .. 0u64
    -- i
```

```output
126
127
-127
-128
254
255
1
0
2147483646
2147483647
18446744073709551614
18446744073709551615
1
0
```

### The ends of a range are evaluated once, in the type both promote to.

```yumi
. n = 3
for i in 0 .. < n
    n = 10
    -- i
-- n
for i in 0u8 .. < 2
    -- i
for j in 5000000000 .. < 5000000002
    -- j
for k in 1i8 .. 2u8
    -- k
. a = 2
for i in a * 2 .. a * 3
    -- i
```

```output
0
1
2
10
0
1
5000000000
5000000001
1
2
4
5
6
```

### A range that starts at a written 0 and ends at an integer is of the type of its end.

```yumi
. n = 3000000000u32
. passes = 0
for i in 0 .. < n
    -- i
    passes += 1
    if passes == 3
        break
. span = 0 .. < n
-- span.end
```

```output
0
1
2
3000000000
```

### The clauses of a range loop: `when` skips a pass, and `while` and `until` end the loop.

```yumi
for i in 0 .. < 10 when i % 3 == 0
    -- i
for i in 0 .. 5 while i < 2
    -- i
for i in 0 .. 5 until i == 3
    -- i
for i in 0 .. 3 when i < 3
    -- i
-- "end"
```

```output
0
3
6
9
0
1
0
1
2
0
1
2
end
```

### `continue` goes to the next item, also at an end the range takes, and `break` leaves the nearest loop.

```yumi
for i in 0 .. 3
    if i == 3
        continue
    -- i
for i in 0 .. < 3
    for j in 0 .. < 3
        if j == 2
            break
        if i == 1
            continue
        -- i * 10 + j
-- "end"
```

```output
0
1
2
0
1
20
21
end
```

### The item of a range loop hides a local of its name, and is a number of the range's type.

```yumi
. i = 9
for i in 0 .. < 2
    -- i
-- i
for i in 100i8 .. 101i8
    . t = i + 100
    -- t
```

```output
0
1
9
-56
-55
```

### Ranges written as values: their ends, step and length, with a step written and counted down.

```yumi
. len = 5
. span = 0 .. < len
-- span.start
-- span.end
-- span.increment
-- span.is-through
-- span.length
show(1 .. 3)
show(note(4) .. < note(9))
. wide = 2u64 .. 10u64
-- wide.length
. real = 0.5 .. 2.5
-- real.start
-- real.increment
. back = 9 .. 3
-- back.length
-- back[1]
. stepped = 0 .. 10 by 5
-- stepped.increment
-- stepped.length

━━━━━━━━━━━━━━━━━━━━
+ do note Int
    - n Int

-- "note $n"
= n

━━━━━━━━━━━━━━━━━━━━
+ do show
    - range Range'Int

-- range.start
-- range.end
-- range.is-through
-- range.length
```

```output
0
5
1
false
5
1
3
true
3
note 4
note 9
4
9
false
5
9
0.5
1
7
8
5
3
```

### A range kept in a property, passed, indexed and counted down.

```yumi
. holder = Holder(2 .. 6)
-- holder.span.start, holder.span.end, holder.span.is-through
-- holder.span.length
holder.span = 1 .. < 4
-- holder.span.length
. down = 5 .. < 1
-- down.start, down.end, down.length
-- down[0], down[3]
. stepped = 10 .. 0 by 5
-- stepped.length, stepped[1]
. bytes = 1u8 .. 3u8
-- bytes.length
-- total(0 .. 4)

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ span Range'Int

────────────────────
+ This
    - span

━━━━━━━━━━━━━━━━━━━━
+ do total Int
    - range Range'Int

. sum = 0
. i = 0
while i < range.length
    sum += range[i]
    i += 1
= sum
```

```output
(2, 6, true)
5
3
(5, 1, 4)
(5, 2)
(3, 5)
3
10
```

### A loop over a range held in a local.

```yumi
. range = 0 .. < 3
for i in range
    -- i
```

```output
0
1
2
```

### A range held in a local: given to a function that takes a stream, converted to a stream, and looped over.

```yumi
. range = 1 .. 4
-- total(range)
for n in range as <Int>
    -- n
for n in range
    -- n

━━━━━━━━━━━━━━━━━━━━
+ do total'T T
    - items <T>

. sum T
for item in items
    sum += item
= sum
```

```output
10
1
2
3
4
1
2
3
4
```

### Ranges with a side left open are made numbers for a container.

```yumi
. range = ... 3
for i in range.normalize(0 .. < 5)
    -- i
. open = 2 ...
for i in open.normalize(0 .. < 5)
    -- i
```

```output
0
1
2
2
3
4
```

### Ranges of positions, written and with a side left open, are given to a function and slice a list.

```yumi
. head = ... 2
show(head)
. tail = 1 ...
show(tail)
show(Position.first .. Position.last)
show(Position.last .. Position.first)
show(Position.middle .. < Position.end)

━━━━━━━━━━━━━━━━━━━━
+ do show
    - range Range'Position

. items = ["a", "b", "c", "d"]
-- ", ".join(items[range])
```

```output
a, b
b, c, d
a, b, c, d
a, b, c, d
c, d
```

```yumi
. text = "hello world"
. count = 5
. spot = Position()
-- text[... count]
-- text[spot.column ...]
-- [1, 2, 3, 4][... 2]
-- [1, 2, 3, 4][2 ...]

━━━━━━━━━━━━━━━━━━━━
+ Position

+ column Int = 6
```

```output
hello
world
1, 2
3, 4
```

### A range made forward holds the same items from the lowest up, whatever its step and direction.

```yumi
-- List((0 .. 10 by 3).forward())
-- List((0 .. < 10 by 3).forward())
-- List((10 .. 0 by 3).forward())
-- List((10 .. < 0 by 3).forward())
-- List((0.0 .. 1.0 by 0.25).forward())
-- List((2 .. < 2).forward())
```

```output
0, 3, 6, 9
0, 3, 6, 9
1, 4, 7, 10
1, 4, 7, 10
0, 0.25, 0.5, 0.75, 1
```

### An index into a range is one of its items or out of range, and a slice by index holds the items at those places.

```yumi
. r = 0 .. 10 by 3
-- r[3]
-- List(r[1 .. < 3])
-- List(r[0 .. 3 by 2])
-- List((0 .. 10)[2 .. < 4])
-- List((10 .. 0 by 2)[1 .. 3])
-- List((0.0 .. 1.0 by 0.25)[1 .. < 3])
try-index(r, 4)
try-index(r, -1)
try-slice(r)

━━━━━━━━━━━━━━━━━━━━
+ do try-index
    - r Range'Int
    - i Index

. attempt = do new
    -- r[i]
catch failure
    -- "index: $failure"
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ do try-slice
    - r Range'Int

. attempt = do new
    -- List(r[2 .. < 5])
catch failure
    -- "slice: $failure"
attempt.wait()
```

```output
9
3, 6
0, 6
2, 3
8, 6, 4
0.25, 0.5
index: Range index 4 out of range
index: Range index -1 out of range
slice: Range out of bounds (4 / 4)
```

### A range that steps by zero panics where its items are counted.

```yumi
try-list()
try-loop()

━━━━━━━━━━━━━━━━━━━━
+ do try-list

. attempt = do new
    . r = 0 .. 5 by 0
    -- List(r)
catch failure
    -- "list: $failure"
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ do try-loop

. attempt = do new
    for i in 0 .. 5 by 0
        -- i
catch failure
    -- "loop: $failure"
attempt.wait()
```

```output
list: A range cannot step by 0
loop: A range cannot step by 0
```
