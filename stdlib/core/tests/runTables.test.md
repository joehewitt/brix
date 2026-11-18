# Tables

Table rows

These tests specify the v1 fixed-size struct-of-arrays Table'T / TableRow'T /
TableSpan'T / TableField'T behavior from JOE-52.

### Table(size) zero-initializes stored fields and exposes row/span/column view types.

```yumi
/core/table

. table = Table'Particle(3)
-- table.size
-- table.length
-- table[0].id
-- table[1].mass
-- table[2].alive

table[1].id = 7
table[1].mass = 2.5
table[1].alive = true
-- table[1] as Particle

-- type-of table[1]
-- type-of table[0 .. < 2]
-- type-of table.id

━━━━━━━━━━━━━━━━━━━━
+ Particle is Struct

+ id I32
+ mass Float
+ alive Bool
```

```output
3
3
0
0
false
(7, 2.5, true)
[TypeInfo TableRow'Particle]
[TypeInfo TableSpan'Particle]
[TypeInfo TableField'I32]
```

```yumi
/core/table

. table = Table'LoopRow(3) • with LoopRow((each + 5) as U32, 2)
. store = LoopTableStore(table)

for offset in 0 .. < 2u32
    . row = store.rows[(1u32 + offset) as Index]
    -- row.value

. cursor = store.rows[0]
for offset in 0 .. < cursor.count
    . row = store.rows[(cursor.value + offset - 4) as Index]
    -- row.value

━━━━━━━━━━━━━━━━━━━━
+ LoopRow is Struct

+ value U32
+ count U32

━━━━━━━━━━━━━━━━━━━━
+ LoopTableStore

+ rows Table'LoopRow

────────────────────
+ This
    - rows Table'LoopRow
```

```output
6
7
6
7
```

```yumi
/core/table

/core/arena

. arena = Arena(16384)
do finally arena.release()
. rows = Table'Node(arena, 2)
rows.append(Node.wide(1, 10, 20, 30, 40))
rows.append(Node.wide(2, 50, 60, 70, 80))
. take-external = false

match rows[0]
    | .small(small)
        show-scalar-spill(small.value)
    | .wide(wide)
        show-scalar-spill(wide.external-a + wide.external-b)
        wide.external-a += 5
        show-scalar-spill(wide.external-a + wide.external-b)
        show-scalar-spill(wide.inline-a + wide.inline-b)
        if take-external
            show-scalar-spill(wide.external-a)

match rows[0]
    | .small(small)
        show-scalar-spill(small.value)
    | .wide(wide)
        . first = wide.external-a
        rows.reserve(32)
        show-scalar-spill(first + wide.external-b)

match rows[0]
    | .small(small)
        show-scalar-spill(small.value)
    | .wide(wide)
        show-scalar-spill(wide.external-a)

match rows[1]
    | .small(small)
        show-scalar-spill(small.value)
    | .wide(wide)
        show-scalar-spill(wide.external-b)

━━━━━━━━━━━━━━━━━━━━
+ do show-scalar-spill
    - value U64

-- value

━━━━━━━━━━━━━━━━━━━━
+ Syntax is Tuple

+ source U64

━━━━━━━━━━━━━━━━━━━━
+ Small is Syntax

+ value U64

━━━━━━━━━━━━━━━━━━━━
+ Wide is Syntax

+ @hot inline-a U64
+ @hot inline-b U64
+ external-a U64
+ external-b U64

━━━━━━━━━━━━━━━━━━━━
+ Node is Union

| small Small
| wide Wide
```

```output
70
75
30
75
35
80
```

```yumi
/core/table

/core/arena

. arena = Arena(8192)
do finally arena.release()
. rows = Table'Node(arena, 1)
rows.append(Node.wide(1, 10, 20, 30, 40))

match rows[0]
    | .small(small)
        show-table-case-call(small.value)
    | .wide(wide)
        show-table-case-call(pair-table-cases(wide, wide))
        show-table-case-call(unsafe-table-case(wide, rows))

━━━━━━━━━━━━━━━━━━━━
+ do show-table-case-call
    - value U64

-- value

━━━━━━━━━━━━━━━━━━━━
+ pair-table-cases U64
    - left Wide
    - right Wide

left.external-a + right.external-b

━━━━━━━━━━━━━━━━━━━━
+ unsafe-table-case U64
    - wide Wide
    - rows Table'Node

do
    mutate-table-case-source(rows)
wide.external-a

━━━━━━━━━━━━━━━━━━━━
+ do mutate-table-case-source
    - rows Table'Node

rows[0] = Node.wide(2, 100, 200, 300, 400)

━━━━━━━━━━━━━━━━━━━━
+ Syntax is Tuple

+ source U64

━━━━━━━━━━━━━━━━━━━━
+ Small is Syntax

+ value U64

━━━━━━━━━━━━━━━━━━━━
+ Wide is Syntax

+ @hot inline-a U64
+ @hot inline-b U64
+ external-a U64
+ external-b U64

━━━━━━━━━━━━━━━━━━━━
+ Node is Union

| small Small
| wide Wide
```

```output
70
30
```

```yumi
/core/table

/core/arena

. arena = Arena(4096)
do finally arena.release()
. rows = Table'Node(arena, 1)
rows.append(Node.wide(10, 1, 2, 200, 300))
-- read-wide(rows[0], false)

━━━━━━━━━━━━━━━━━━━━
+ Syntax is Tuple

+ source U32

━━━━━━━━━━━━━━━━━━━━
+ Wide is Syntax

+ @hot inline-a U64
+ @hot inline-b U64
+ external-a U64
+ external-b U64

━━━━━━━━━━━━━━━━━━━━
+ Small is Syntax

+ value U8

━━━━━━━━━━━━━━━━━━━━
+ Node is Union

| small Small
| wide Wide

━━━━━━━━━━━━━━━━━━━━
+ do read-wide U64
    - row TableRow'Node
    - include-first Bool

= match row
    | .small
        0u64
    | .wide(wide)
        . first = 0u64
        if include-first
            first = wide.external-a
        first + wide.external-b
```

```output
300
```

```yumi compile-error
/core/table

/core/arena

. arena = Arena(8192)
do finally arena.release()
. rows = Table'Point(arena, 1)
rows.append(Point(1, 2))
. fixed = rows.finish()
fixed[0 .. < 1][0] = Point(3, 4)

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
Exception: ImmutableTableSpan is read-only
<no file:11>
    fixed[0 .. < 1][0] = Point(3, 4)
    ^^^^^^^^^^^^^^^^^^
```

```yumi compile-error
/core/table

/core/arena

. arena = Arena(8192)
do finally arena.release()
. rows = Table'Point(arena, 1)
rows.append(Point(1, 2))
. fixed = rows.finish()
fixed.x[0] = 3

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
Exception: ImmutableTableField is read-only
<no file:11>
    fixed.x[0] = 3
    ^^^^^^^^^^
```

```yumi compile-error
/core/table

. fixed = ImmutableTable'Point()
-- fixed.length

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
Exception: No constructor of ImmutableTable matches these arguments; its constructors are private, declared with _, so only its own module can call them
<no file:4>
    . fixed = ImmutableTable'Point()
              ^^^^^^^^^^^^^^
```

```yumi
/core/table

/core/arena

. arena = Arena(8192)
do finally arena.release()
. rows = Table'Point(arena, 1)
rows.append(Point(10, 20))
rows.append(Point(30, 40))
. fixed = rows.finish()

-- type-of fixed
-- type-of fixed[0]
-- type-of fixed[0 .. < 1]
-- type-of fixed.x
-- "${fixed.length}/${fixed.size}"
-- fixed[0] as Point
-- fixed[1] as Point
-- ",".join(fixed.x)
-- fixed[1 .. < 2][0] as Point

. total = 0
for row in fixed
    total += row.x + row.y
-- total

. empty = Table'Point(arena).finish()
-- "${empty.length}/${empty.size}"

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
[TypeInfo ImmutableTable'Point]
[TypeInfo ImmutableTableRow'Point]
[TypeInfo ImmutableTableSpan'Point]
[TypeInfo ImmutableTableField'I32]
2/2
(10, 20)
(30, 40)
10,30
(30, 40)
100
0/0
```

```yumi
/core/table

/core/arena
/core/array-builder

. arena = Arena(8192)
do finally arena.release()
. rows = Table'Point(arena, 2)
rows.append(Point(10, 20))
rows.append(Point(30, 40))
rows.append(Point(50, 60))
. fixed = rows.finish()
. ids = Array'U32([2u32, 0u32, 2u32])
. selected = fixed[ids]
-- type-of selected
-- type-of selected[0]
-- selected.length
for row in selected
    -- row as Point
. middle = selected[1 .. < 3]
for row in middle
    -- row as Point
. sequence = ids.immutable
-- fixed[sequence][1] as Point
. builder = ArrayBuilder'U32(arena)
builder.append(1u32)
builder.append(0u32)
for row in fixed[builder]
    -- row as Point

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
[TypeInfo ImmutableTableSelection'Point'U32]
[TypeInfo ImmutableTableRow'Point]
3
(50, 60)
(10, 20)
(50, 60)
(10, 20)
(50, 60)
(10, 20)
(30, 40)
(10, 20)
```

```yumi compile-error
/core/table

/core/arena

. arena = Arena(8192)
do finally arena.release()
. rows = Table'Point(arena, 1)
rows.append(Point(1, 2))
. fixed = rows.finish()
fixed[Array'U32([0u32])][0].x = 3

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
Exception: ImmutableTableRow is read-only
<no file:11>
    fixed[Array'U32([0u32])][0].x = 3
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```yumi
/core/table

/core/arena

. arena = Arena(8192)
do finally arena.release()
. rows = Table'Point(arena, 1)
rows.append(Point(1, 2))
. alias = rows
. old-row = rows[0]
. old-span = rows[0 .. < 1]
. old-field = rows.x
. fixed = rows.finish()

show-sealed-table(alias)
show-sealed-row(old-row)
show-sealed-span(old-span)
show-sealed-field(old-field)
show-sealed-finish(alias)
-- fixed[0] as Point

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32

━━━━━━━━━━━━━━━━━━━━
+ do show-sealed-table
    - table Table'Point

. attempt = do new
    table.append(Point(3, 4))
catch failure
    -- failure
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ do show-sealed-row
    - row TableRow'Point

. attempt = do new
    row.x = 3
catch failure
    -- failure
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ do show-sealed-span
    - span TableSpan'Point

. attempt = do new
    span[0] = Point(3, 4)
catch failure
    -- failure
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ do show-sealed-field
    - field TableField'I32

. attempt = do new
    field[0] = 3
catch failure
    -- failure
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ do show-sealed-finish
    - table Table'Point

. attempt = do new
    . discarded = table.finish()
catch failure
    -- failure
attempt.wait()
```

```output
Table is sealed
Table is sealed
Table is sealed
Table is sealed
Table is sealed
(1, 2)
```

```yumi compile-error
/core/table

/core/arena

. arena = Arena(8192)
do finally arena.release()
. rows = Table'Point(arena, 1)
rows.append(Point(1, 2))
. fixed = rows.finish()
fixed[0] = Point(3, 4)

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
Exception: ImmutableTable is read-only
<no file:11>
    fixed[0] = Point(3, 4)
    ^^^^^^^^
```

```yumi compile-error
/core/table

/core/arena

. arena = Arena(8192)
do finally arena.release()
. rows = Table'Point(arena, 1)
rows.append(Point(1, 2))
. fixed = rows.finish()
fixed[0].x = 3

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
Exception: ImmutableTableRow is read-only
<no file:11>
    fixed[0].x = 3
    ^^^^^^^^^^
```

```yumi
/core/table

/core/arena

. arena = Arena(8192)
do finally arena.release()
. rows = Table'Node(arena, 1)
rows.append(Node.small(10, 100))
rows.append(Node.wide(20, 1, 2, 300, 400))
. fixed = rows.finish()

-- type-of fixed
-- type-of fixed.source
-- ",".join(fixed.source)
-- fixed[0] as Node
-- fixed[1 .. < 2][0] as Node

for row in fixed
    -- match row
        | .small(small)
            small.value
        | .wide(wide)
            wide.inline-a + wide.external-b

━━━━━━━━━━━━━━━━━━━━
+ Syntax is Tuple

+ source U32

━━━━━━━━━━━━━━━━━━━━
+ Small is Syntax

+ @hot value U64

━━━━━━━━━━━━━━━━━━━━
+ Wide is Syntax

+ @hot inline-a U64
+ @hot inline-b U64
+ external-a U64
+ external-b U64

━━━━━━━━━━━━━━━━━━━━
+ Node is Union

| small Small
| wide Wide
```

```output
[TypeInfo ImmutableTable'Node]
[TypeInfo ImmutableTableField'U32]
10,20
(10, 100)
(20, 1, 2, 300, 400)
100
401
```

```yumi
/core/table

. rows = Table'Node(4)
rows[0] = Node.low(10, Pair(1, 2))
rows[1] = Node.middle(20, Pair(2, 3))
rows[2] = Node.high(30, Pair(3, 4))
rows[3] = Node.other(40, Pair(4, 5))

-- match rows[0]
    | .low(low)
        low.value
    | .middle(middle)
        middle.value
    | .high(high)
        high.value
    ??
        -1

-- match rows[1]
    | .low(low)
        low.value
    | .middle(middle) { middle.pair.right == 3 }
        middle.value + middle.pair.right
    | .high(high)
        high.value
    ??
        -1

. span = rows[1 .. < 4]
for row in span
    -- match row
        | .low(low)
            low.value
        | .middle(middle)
            middle.value + middle.pair.right
        | .high(high)
            high.value
        ??
            -1

. selected = 0
do match rows[2]
    | .low(low)
        selected = low.value
    | .middle(middle)
        selected = middle.value
    | .high(high)
        selected = high.value
    ??
        selected = -1
-- selected

━━━━━━━━━━━━━━━━━━━━
+ Pair is Tuple

+ left I64
+ right I64

━━━━━━━━━━━━━━━━━━━━
+ Base is Tuple

+ value I64
+ pair Pair

━━━━━━━━━━━━━━━━━━━━
+ Low is Base

━━━━━━━━━━━━━━━━━━━━
+ Middle is Base

━━━━━━━━━━━━━━━━━━━━
+ High is Base

━━━━━━━━━━━━━━━━━━━━
+ Other is Base

━━━━━━━━━━━━━━━━━━━━
+ Node is Union

| low Low
| middle Middle
| high High
| other Other
```

```output
10
23
23
30
-1
30
```

### Table(size) with filler(index) gathers each row value and scatters it into columns.

```yumi
/core/table

. calls = ""
. table = Table'Point(4) • with :: each => do
    calls += "$each"
    = Point(each as I32, (each * each) as I32)

-- calls
for i in 0 .. < table.length
    -- table[i] as Point

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
0123
(0, 0)
(1, 1)
(2, 4)
(3, 9)
```

### A stored TableRow is a live row cursor; row as T gathers a stable value snapshot.

```yumi
/core/table

. table = Table'Point(1)
table[0] = Point(1, 2)

. row = table[0]
. snapshot = row as Point
row.x = 9
snapshot.y = 8

-- table[0] as Point
-- row as Point
-- snapshot

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
(9, 2)
(9, 2)
(1, 8)
```

### Assigning table[i] scatters a real T into every column and copies the source value.

```yumi
/core/table

. table = Table'Particle(2)
. value = Particle(1, 2, 3)
table[0] = value
value.x = 99
table[1] = Particle(4, 5, 6)

-- table[0] as Particle
-- table[1] as Particle
-- value

━━━━━━━━━━━━━━━━━━━━
+ Particle is Struct

+ x I32
+ y I32
+ z I32
```

```output
(1, 2, 3)
(4, 5, 6)
(99, 2, 3)
```

### Struct-valued fields are column values; assigning the field column writes the table.

```yumi
/core/table

. table = Table'Entity(1)
table[0] = Entity(1, Point(2, 3), Point(4, 5))

. copied-position = table[0].position
copied-position.y = 30
-- table[0] as Entity
-- copied-position

table.position[0] = Point(7, 8)
-- table[0] as Entity

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32

━━━━━━━━━━━━━━━━━━━━
+ Entity is Struct

+ id I32
+ position Point
+ velocity Point
```

```output
(1, (2, 3), (4, 5))
(2, 30)
(1, (7, 8), (4, 5))
```

### table.field returns a writable TableField over one contiguous field buffer.

```yumi
/core/table

. table = Table'Point(3) • with Point(each as I32, (each + 10) as I32)
-- ", ".join(table.x)
-- ", ".join(table.y)
-- table.x.length

table.x[1] = 42
table.y.fill(7)
-- table[0] as Point
-- table[1] as Point
-- table[2] as Point
-- ", ".join(table.y)

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
0, 1, 2
10, 11, 12
3
(0, 7)
(42, 7)
(2, 7)
7, 7, 7
```

### TableField.data exposes the contiguous field buffer, proving struct-of-arrays layout.

```yumi
/core/table

. table = Table'Point(3) • with Point((each + 1) as I32, (each + 10) as I32)
. xs = table.x.data
. ys = table.y.data

-- xs[0]
-- xs[1]
-- xs[2]
-- ys[0]
-- ys[1]
-- ys[2]

xs[1] = 42
ys[2] = 77
-- table[1] as Point
-- table[2] as Point

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
1
2
3
10
11
12
(42, 11)
(3, 77)
```

### TableField columns zip into a typed TableSpan with live writable rows.

```yumi
/core/table

. table = Table'Point(3) • with Point((each + 1) as I32, (each + 10) as I32)
. points TableSpan'Point = table.x + table.y

-- points.length
-- type-of points
-- points[0] as Point
-- points[2] as Point

points[1].x = 42
points[2].y = 77
-- table[1] as Point
-- table[2] as Point

for point in points
    point.y += point.x

-- table[0] as Point
-- table[1] as Point
-- table[2] as Point

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
3
[TypeInfo TableSpan'Point]
(1, 10)
(3, 12)
(42, 11)
(3, 77)
(1, 11)
(42, 53)
(3, 80)
```

### Sliced TableField columns zip as zero-copy span-relative rows.

```yumi
/core/table

. table = Table'Point(5) • with Point((each + 1) as I32, (each + 10) as I32)
. middle TableSpan'Point = table.x[1 .. < 4] + table.y[1 .. < 4]

-- middle.length
-- middle[0] as Point
-- middle[2] as Point

middle[0].x = 22
middle[2].y = 99
-- table[0] as Point
-- table[1] as Point
-- table[3] as Point
-- table[4] as Point

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
3
(2, 11)
(4, 13)
(1, 10)
(22, 11)
(4, 99)
(5, 14)
```

### Chained TableField zip maps columns to row fields in destination field order.

```yumi
/core/table

. table = Table'Particle(2) • with Particle((each + 1) as I32, (each + 10) as I32, (each + 100) as I32)
. particles TableSpan'Particle = table.id + table.x + table.y

-- particles.length
-- particles[0] as Particle
-- particles[1] as Particle

particles[0].y = 500
-- table[0] as Particle

━━━━━━━━━━━━━━━━━━━━
+ Particle is Struct

+ id I32
+ x I32
+ y I32
```

```output
2
(1, 10, 100)
(2, 11, 101)
(1, 10, 500)
```

### Destructuring TableSpan and Table unzips live field views.

```yumi
/core/table

. table = Table'Point(4) • with Point((each + 1) as I32, (each + 10) as I32)
. span = table[1 .. < 3]
. xs, ys = span

-- type-of xs
-- xs.length
-- ", ".join(xs)
-- ", ".join(ys)

xs[0] = 40
ys[1] = 90
-- table[1] as Point
-- table[2] as Point

. all-x, all-y = table
all-x[0] = 70
all-y[3] = 120
-- table[0] as Point
-- table[3] as Point

. typed-x TableField'I32, typed-y TableField'I32 = span
typed-x[0] = 400
typed-y[1] = 900
-- type-of typed-y
-- table[1] as Point
-- table[2] as Point

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
[TypeInfo TableField'I32]
2
2, 3
11, 12
(40, 11)
(3, 90)
(70, 10)
(4, 120)
[TypeInfo TableField'I32]
(400, 11)
(3, 900)
```

### TableField zip rejects columns with different lengths.

```yumi crash
/core/table

. table = Table'Point(4) • with Point((each + 1) as I32, (each + 10) as I32)
. bad TableSpan'Point = table.x[0 .. < 2] + table.y[0 .. < 3]
-- bad.length

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

### table[range] returns a span; span rows and span columns are relative to the range.

```yumi
/core/table

. table = Table'Point(5) • with Point(each as I32, (each + 10) as I32)
. span = table[1 .. < 4]

-- span.length
-- span[0] as Point
-- span[2] as Point
-- ", ".join(span.x)

span.x[1] = 99
span[2].y = 77
-- table[1] as Point
-- table[2] as Point
-- table[3] as Point

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
3
(1, 11)
(3, 13)
1, 2, 3
(1, 11)
(99, 12)
(3, 77)
```

### Table, span, and column Position ranges normalize before the compiler table slice path.

```yumi
/core/table

. table = Table'Point(4) • with Point((each + 1) as I32, (each + 10) as I32)
. full = table[Position.first .. < Position.end]
. tail = full[Position.middle .. Position.last]
. xs = table.x[Position.first .. Position.last]

-- full.length
-- full[0] as Point
-- full[3] as Point
-- tail.length
-- tail[0] as Point
-- tail[1] as Point
-- xs.length
-- xs[2]

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
4
(1, 10)
(4, 13)
2
(3, 12)
(4, 13)
4
3
```

### Indexable witnesses call the Table slice overloads without recursive slow-path crashes.

```yumi
/core/table

. table = Table'Point(4) • with Point((each + 1) as I32, (each + 10) as I32)
. rows Indexable'(TableRow'Point) = table
. span-rows Indexable'(TableRow'Point) = table[1 .. < 4]
. xs Indexable'I32 = table.x

-- rows[1] as Point
-- (rows[1 .. < 3])[0] as Point
-- (span-rows[Position.first .. Position.last])[2] as Point
-- (xs[1 .. < 3])[1]

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
(2, 11)
(2, 11)
(4, 13)
3
```

### Column range assignment and table span assignment copy selected columns/rows.

```yumi
/core/table

. source = Table'Point(4) • with Point((each + 1) as I32, (each + 10) as I32)
. target = Table'Point(4)

target.x[1 .. < 3] = source.x[0 .. < 2]
target[0 .. < 2] = source[2 .. < 4]

for i in 0 .. < target.length
    -- target[i] as Point

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
(3, 12)
(4, 13)
(2, 0)
(0, 0)
```

### Iterating a table yields live row cursors without forcing row struct materialization.

```yumi
/core/table

. table = Table'Point(3) • with Point(each as I32, (each + 1) as I32)

for row in table
    row.x += 10
    -- row as Point

for i in 0 .. < table.length
    -- table[i] as Point

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
(10, 1)
(11, 2)
(12, 3)
(10, 1)
(11, 2)
(12, 3)
```

### Repeated hot field reads in row and indexed loops preserve table values.

```yumi
/core/table

. table = Table'Point(4) • with Point(each as Size, (each + 10) as Size)
. row-sum = 0 as Size
. index-sum = 0 as Size

for row in table
    row-sum += row.x + row.x + row.y

for i in 0 .. < table.length
    index-sum += table[i].x + table[i].x + table[i].y

-- "$row-sum $index-sum"

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Size
+ y Size
```

```output
58 58
```

### Iterating a span yields live row cursors with span-relative indexes.

```yumi
/core/table

. table = Table'Point(4) • with Point(each as I32, (each + 10) as I32)
. span = table[1 .. < 3]

for row in span
    row.y += 100
    -- row as Point

for i in 0 .. < table.length
    -- table[i] as Point

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
(1, 111)
(2, 112)
(0, 10)
(1, 111)
(2, 112)
(3, 13)
```

### Reassigning a table local inside an indexed loop does not reuse stale column bases.

```yumi
/core/table

. first = Table'Point(2) • with Point((each + 1) as I32, (each + 10) as I32)
. second = Table'Point(2) • with Point((each + 101) as I32, (each + 110) as I32)
. table = first

for i in 0 .. < 2
    if i == 1
        table = second
    -- table[i].x

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
1
102
```

### A table held by a loop variable is read after the loop binds it, never from before the loop.

```yumi
/core/table

. one = Table'Point(1) • with Point((each + 1) as I32, (each + 10) as I32)
. two = Table'Point(2) • with Point((each + 21) as I32, (each + 30) as I32)
. three = Table'Point(3) • with Point((each + 41) as I32, (each + 50) as I32)
. tables [Table'Point]
tables += one
tables += two
tables += three

for table in tables
    -- "${table.length} ${table[0].x}"
-- ", ".join("${each.length}" in tables)
-- total-rows(tables)

━━━━━━━━━━━━━━━━━━━━
_ do total-rows Size
    - tables [Table'Point]

. total Size = 0
for table in tables
    total += table.length
= total

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
1 1
2 21
3 41
1, 2, 3
6
```

### copy creates independent fixed-size column storage.

```yumi
/core/table

. original = Table'Point(2) • with Point((each + 1) as I32, (each + 10) as I32)
. copied = original.copy()

copied[0].x = 99
original[1].y = 88

-- original[0] as Point
-- original[1] as Point
-- copied[0] as Point
-- copied[1] as Point

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
(1, 10)
(2, 88)
(99, 10)
(2, 11)
```

### Function arguments materialize TableRow cursors as live row views.

```yumi
/core/table

. table = Table'Point(4) • with Point((each + 1) as I32, (each + 10) as I32)

-- bump(table[1])
for row in table when row.x == 1
    -- bump(row)
for in table when each.x == 3
    -- bump(each)
for in table when it.x == 4
    -- bump(it)

for i in 0 .. < table.length
    -- table[i] as Point

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32

━━━━━━━━━━━━━━━━━━━━
+ do bump Point
    - row TableRow'Point

row.x += 10
= row as Point
```

```output
(12, 11)
(11, 10)
(13, 12)
(14, 13)
(11, 10)
(12, 11)
(13, 12)
(14, 13)
```

### Passing rows to a struct parameter gathers stable snapshots.

```yumi
/core/table

. table = Table'Point(2) • with Point((each + 1) as I32, (each + 10) as I32)

-- move(table[0])
for row in table when row.x == 2
    -- move(row)

-- table[0] as Point
-- table[1] as Point

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32

━━━━━━━━━━━━━━━━━━━━
+ do move Point
    - point Point

point.x += 100
point.y += 1000
= point
```

```output
(101, 1010)
(102, 1011)
(1, 10)
(2, 11)
```

### Escaped TableRows stay live, while gathered row values are snapshots.

```yumi
/core/table

. table = Table'Point(1)
table[0] = Point(1, 2)

. row = table[0]
. snapshot = table[0] as Point
table[0].x = 7
row.y = 8
snapshot.x = 9

-- row as Point
-- snapshot
-- table[0] as Point

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
(7, 8)
(9, 2)
(7, 8)
```

### Returning table[i] or a loop row materializes a live TableRow value.

```yumi
/core/table

. table = Table'Point(2) • with Point((each + 1) as I32, (each + 10) as I32)
. direct = row-at(table, 1)
. first = first-row(table)

table[1].x = 50
direct.y = 60
first.x = 70

-- direct as Point
-- first as Point
-- table[0] as Point
-- table[1] as Point

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32

━━━━━━━━━━━━━━━━━━━━
+ row-at TableRow'Point
    - table Table'Point
    - index Index

table[index]

━━━━━━━━━━━━━━━━━━━━
+ do first-row TableRow'Point
    - table Table'Point

for row in table
    = row
?! Panic("Expected a non-empty table")
```

```output
(50, 60)
(70, 10)
(70, 10)
(50, 60)
```

### Yielding table[i] and row materializes live TableRow values.

```yumi
/core/table

. table = Table'Point(2) • with Point((each + 1) as I32, (each + 10) as I32)
. stream = rows(table)
. direct = stream()!
. looped = stream()!

table[1].x = 50
direct.y = 60
looped.x = 70

-- direct as Point
-- looped as Point
-- table[0] as Point
-- table[1] as Point

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32

━━━━━━━━━━━━━━━━━━━━
+ do rows
    - table Table'Point

<= table[1]
for row in table
    <= row
```

```output
(50, 60)
(70, 10)
(70, 10)
(50, 60)
```

### Arrays, lists, and object fields can store live TableRow values.

```yumi
/core/table

. table = Table'Point(2) • with Point((each + 1) as I32, (each + 10) as I32)
. array = [table[0], table[1]]
. rows = List'(TableRow'Point)()
rows += table[0]
rows += table[1]
. holder = Holder(table[0])

table[0].x = 30
array[1].y = 40
rows[0].y = 50
holder.row.x = 60

-- array[0] as Point
-- array[1] as Point
-- rows[0] as Point
-- rows[1] as Point
-- holder.row as Point
-- table[0] as Point
-- table[1] as Point

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ row TableRow'Point

────────────────────
+ This
    - row TableRow'Point
```

```output
(60, 50)
(2, 40)
(60, 50)
(2, 40)
(60, 50)
(60, 50)
(2, 40)
```

### A lazy table[i] alias captured by a closure is evaluated at call time.

```yumi
/core/table

. table = Table'Point(2) • with Point((each + 1) as I32, (each + 10) as I32)
. getter = make-getter(table)

table[0].x = 8
table[1].x = 9
-- getter() as Point
getter().y = 77
-- table[0] as Point
-- table[1] as Point

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32

━━━━━━━━━━━━━━━━━━━━
+ do make-getter Getter'(TableRow'Point)
    - table Table'Point

. index = 0
.. row = table[index]
. getter = => row
index = 1
= getter
```

```output
(9, 11)
(8, 10)
(9, 77)
```

### Whole-row address-taking cannot pass table[i] as a contiguous row struct pointer.

```yumi crash
/core/table

/JaCore/tests.h

. table = Table'Thing(1)
C:ctest15(&table[0])

━━━━━━━━━━━━━━━━━━━━
+ Thing is Struct

+ a Int
+ b String
```

```output
Exception: No C function "ctest15" accepts (Table'Thing); it is declared as "ctest15(CPointer'I8)"
<no file:7>
    C:ctest15(&table[0])
    ^^^^^^^^^^^^^^^^^^^^
```

### Table row types must be concrete fixed-layout structs, not class/object types.

```yumi crash
/core/table

. table = Table'Thing(1)
-- table.length

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ value I32 = 0
```

```output
Exception: Table row types must be concrete fixed-layout structs
<no file:8>
    + Thing
    ^^^^^^
```

### Table row types cannot include getter or lazy row fields in v1.

```yumi crash
/core/table

. getter-table = Table'GetterRow(1)
. lazy-table = Table'LazyRow(1)
-- getter-table.length + lazy-table.length

━━━━━━━━━━━━━━━━━━━━
+ GetterRow is Struct

+ stored I32

────────────────────
+ get derived

stored + 1

━━━━━━━━━━━━━━━━━━━━
+ LazyRow is Struct

+ stored I32
+ cached := stored + 1
```

```output
Exception: Table row fields must be eager stored fields
<no file:14>
    + get derived
    ^^^^^^^^^^^^
```

### Table row types cannot rely on delegated interface-shaped behavior in v1.

```yumi crash
/core/table

. table = Table'DelegatingRow(1)
-- table.length

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ act

━━━━━━━━━━━━━━━━━━━━
+ Delegate

────────────────────
+ act

━━━━━━━━━━━━━━━━━━━━
+ DelegatingRow is Struct

as Proto for
+ delegate Delegate
```

```output
Exception: Table row types cannot use delegated interface behavior
<no file:20>
    + DelegatingRow is Struct
    ^^^^^^^^^^^^^^^^^^^^^^^^
```

### Table row types must be named structs, not tuple aliases.

```yumi crash
/core/table

. table = Table'Thing(1)
-- table.length

━━━━━━━━━━━━━━━━━━━━
+ Thing is (a Int, b String)
```

```output
Exception: Table row types must be concrete fixed-layout structs
<no file:8>
    + Thing is (a Int, b String)
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Pointerful scalar row field stores emit barriers during concurrent marking.

```yumi
/core/table

/BxCore/BxCore.h
/BxGC/BxGCTest.h

. table = Table'Cell(1)
table[0] = Cell(GCNode(1), GCNode(2))

. barrier-count-before = C:brixGCBarrierCount()
C:brixGCStartConcurrentMark()
table[0].left = GCNode(3)
C:brixGCFinishConcurrentMark()
. barrier-count-after = C:brixGCBarrierCount()
C:brixCollect()

if barrier-count-after <= barrier-count-before
    -- "barrier-missing"
else if !C:brixGCVerifyObject(table[0].left as Pointer)
    -- "left-missing"
else
    -- "barrier-ok ${table[0].left.value}"

━━━━━━━━━━━━━━━━━━━━
+ Cell is Struct

+ left GCNode
+ right GCNode

━━━━━━━━━━━━━━━━━━━━
+ GCNode

+ value = 0

────────────────────
+ This
    - value Int
```

```output
barrier-ok 3
```

### Pointerful row scatter keeps newly stored row objects alive.

```yumi
/core/table

/BxCore/BxCore.h
/BxGC/BxGCTest.h

. table = Table'Cell(2)
table[0] = Cell(GCNode(10), GCNode(20))

. barrier-count-before = C:brixGCBarrierCount()
C:brixGCStartConcurrentMark()
table[1] = Cell(GCNode(30), GCNode(40))
C:brixGCFinishConcurrentMark()
. barrier-count-after = C:brixGCBarrierCount()

C:brixCollect()

if barrier-count-after <= barrier-count-before
    -- "scatter-barrier-missing"
else if !C:brixGCVerifyObject(table[1].left as Pointer)
    -- "left-missing"
else if !C:brixGCVerifyObject(table[1].right as Pointer)
    -- "right-missing"
else
    -- "scatter-ok ${table[1].left.value} ${table[1].right.value}"

━━━━━━━━━━━━━━━━━━━━
+ Cell is Struct

+ left GCNode
+ right GCNode

━━━━━━━━━━━━━━━━━━━━
+ GCNode

+ value = 0

────────────────────
+ This
    - value Int
```

```output
scatter-ok 30 40
```

### Pointerful range copy emits barriers and preserves copied objects.

```yumi
/core/table

/BxCore/BxCore.h
/BxGC/BxGCTest.h

. source = Table'Cell(3) • with Cell(GCNode((each + 1) as Int), GCNode((each + 10) as Int))
. target = Table'Cell(3)

. barrier-count-before = C:brixGCBarrierCount()
C:brixGCStartConcurrentMark()
target[0 .. < 2] = source[1 .. < 3]
C:brixGCFinishConcurrentMark()
. barrier-count-after = C:brixGCBarrierCount()
C:brixCollect()

if barrier-count-after <= barrier-count-before
    -- "range-copy-barrier-missing"
else if !C:brixGCVerifyObject(target[0].left as Pointer)
    -- "target-left-missing"
else if !C:brixGCVerifyObject(target[1].right as Pointer)
    -- "target-right-missing"
else
    -- "range-copy-ok ${target[0].left.value} ${target[1].right.value}"

━━━━━━━━━━━━━━━━━━━━
+ Cell is Struct

+ left GCNode
+ right GCNode

━━━━━━━━━━━━━━━━━━━━
+ GCNode

+ value = 0

────────────────────
+ This
    - value Int
```

```output
range-copy-ok 2 12
```

### Table field reads are invalidated by stores through aliased table values.

```yumi
/core/table

. table = Table'Point(1)
table[0] = Point(1, 2)

. other = table
. b = table[0].x
other[0].x = 7
. a = table[0].x

-- "$b $a ${other[0].x}"

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
1 7 7
```

### Growing a table does not reuse a column base read before the growth.

```yumi
/core/table

/core/arena

. arena = Arena(8192)
do finally arena.release()
. rows = Table'Point(arena, 2)
rows.append(Point(1, 2))
rows.append(Point(3, 4))
. b = rows[0].x
rows.append(Point(5, 6))
rows.append(Point(7, 8))
rows.append(Point(9, 10))
rows[0].x = 70
. column = rows.x

-- "$b ${rows[0].x} ${column[0]}"

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
1 70 70
```

### A call that grows a table does not leave the caller with a stale column base.

```yumi
/core/table

/core/arena

. arena = Arena(8192)
do finally arena.release()
. rows = Table'Point(arena, 1)
rows.append(Point(1, 2))
. b = rows[0].x
grow(rows)
rows[0].x = 70
. column = rows.x

-- "$b ${rows[0].x} ${column[0]} ${rows.length}"

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32

━━━━━━━━━━━━━━━━━━━━
+ do grow
    - rows Table'Point

for index in 0 .. < 20
    rows.append(Point(index as I32, 0))
```

```output
1 70 70 21
```

### Growing a table through an aliased value does not leave a stale column base.

```yumi
/core/table

/core/arena

. arena = Arena(8192)
do finally arena.release()
. rows = Table'Point(arena, 1)
rows.append(Point(1, 2))
. other = rows
. b = rows[0].x
other.append(Point(3, 4))
other.append(Point(5, 6))
rows[0].x = 70
. column = other.x

-- "$b ${rows[0].x} ${column[0]}"

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
1 70 70
```

### A method that grows a table held by another object does not leave a stale column base.

```yumi
/core/table

/core/arena

. holder = Holder()
do finally holder.arena.release()
. adder = Adder(holder)
for index U32 in 0 .. < 7
    adder.add(index)
. pass = Pass(adder)
pass.run()
-- "${holder.rows[0].count} ${holder.rows[4].count} ${holder.rows.length}"

━━━━━━━━━━━━━━━━━━━━
+ Row is Struct

+ name U32
+ count U32

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ arena Arena
+ rows Table'Row

────────────────────
+ This

.. rows = Table'Row(arena, 8)

━━━━━━━━━━━━━━━━━━━━
+ Adder

+ holder Holder

────────────────────
+ This
    - holder Holder

────────────────────
+ do add U32
    - name U32

. row = holder.rows.length as U32
holder.rows.append(Row(name, 1))
= row

━━━━━━━━━━━━━━━━━━━━
+ Pass

_ adder Adder
_ holder Holder

────────────────────
+ This
    - adder Adder

.. holder = adder.holder

────────────────────
+ do run

for position U32 in 0 .. < 7
    . previous = holder.rows[position].count
    adder.add(100 + position)
    adder.add(200 + position)
    holder.rows[position].count = 0
    bump(position, previous + 4)

────────────────────
_ do bump
    - row U32
    - amount U32

holder.rows[row].count = holder.rows[row].count + amount
```

```output
5 5 21
```

### Captured row aliases materialize before entering closures.

```yumi
/core/table

. table = Table'Point(2) • with Point(each as I32, (each + 10) as I32)

for row in table
    . fn = => row as Point
    row.x += 20
    -- fn()

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
(20, 10)
(21, 11)
```

### Captured indexed row aliases materialize with the current loop index.

```yumi
/core/table

. table = Table'Point(2) • with Point(each as I32, (each + 10) as I32)

for i in 0 .. < table.length
    . row = table[i]
    . fn = => row as Point
    row.x += 30
    -- fn()

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
(30, 10)
(31, 11)
```

### Table union rows use common columns, inline and external payloads, live narrowing, and snapshots.

```yumi
/core/table

. nodes = Table'Node(3) • with :: index => do
    if index == 0
        = Node.literal(10, 42)
    = Node.call(index as U32, 7, 8, 9, 10)

nodes[2] = Literal(30, 100)

-- nodes.layout
-- ",".join(nodes.source)

for node in nodes
    match node
        | .literal(literal)
            literal.value += 1
        | .call(call)
            call.arg2 += 100

-- nodes[0] as Node
-- nodes[1] as Node
-- nodes[2] as Node

. snapshot Node = nodes[1] as Node
nodes.source[1] = 50
match nodes[1]
    | .literal(literal)
        literal.value += 0
    | .call(call)
        call.arg2 += 1

-- match snapshot
    | .literal(literal)
        "snapshot literal ${literal.source} ${literal.value}"
    | .call(call)
        "snapshot call ${call.source} ${call.arg0} ${call.arg2}"
-- match nodes[1] as Node
    | .literal(literal)
        "current literal ${literal.source} ${literal.value}"
    | .call(call)
        "current call ${call.source} ${call.arg0} ${call.arg2}"

-- type-of nodes[0]
-- type-of nodes.source

━━━━━━━━━━━━━━━━━━━━
+ Syntax is Tuple

+ source U32

━━━━━━━━━━━━━━━━━━━━
+ Literal is Syntax

+ value I64

━━━━━━━━━━━━━━━━━━━━
+ Call is Syntax

+ callee U32
+ arg0 U64
+ arg1 U64
+ arg2 U64

━━━━━━━━━━━━━━━━━━━━
+ Node is Union

| literal Literal
| call Call
```

```output
Table'Node
packing automatic budget=16
tag column[0] U8
common column[1] source U32
inline column[2] stride=16 align=8
spill-handles column[3] U32 offset+1
external arena stride=16 align=8
case 0 literal Literal
  source U32 common[1]
  value I64 inline+0
case 1 call Call
  source U32 common[1]
  callee U32 inline+0
  arg0 U64 inline+8
  arg1 U64 external+0
  arg2 U64 external+8
10,1,30
(10, 43)
(1, 7, 8, 9, 110)
(30, 101)
snapshot call 1 8 110
current call 50 8 111
[TypeInfo TableRow'Node]
[TypeInfo TableField'U32]
```

### The inline payload column is only as wide as the widest case when every case fits under the budget.

```yumi
/core/table

. nodes = Table'Node(2)
nodes[0] = Node.leaf(1, 7)
nodes[1] = Node.pair(2, 3, 4)
-- nodes.layout
-- nodes[0] as Node
-- nodes[1] as Node

. flags = Table'Flag(2)
flags[1] = Flag.on(5, true)
-- flags.layout
-- flags[0] as Flag
-- flags[1] as Flag

━━━━━━━━━━━━━━━━━━━━
+ Base is Tuple

+ id U32

━━━━━━━━━━━━━━━━━━━━
+ Leaf is Base

+ value U64

━━━━━━━━━━━━━━━━━━━━
+ Pair is Base

+ start U32
+ count U32

━━━━━━━━━━━━━━━━━━━━
+ Node is Union

| leaf Leaf
| pair Pair

━━━━━━━━━━━━━━━━━━━━
+ Off is Base

━━━━━━━━━━━━━━━━━━━━
+ On is Base

+ is-lit Bool

━━━━━━━━━━━━━━━━━━━━
+ Flag is Union

| off Off
| on On
```

```output
Table'Node
packing automatic budget=16
tag column[0] U8
common column[1] id U32
inline column[2] stride=8 align=8
case 0 leaf Leaf
  id U32 common[1]
  value U64 inline+0
case 1 pair Pair
  id U32 common[1]
  start U32 inline+0
  count U32 inline+4
(1, 7)
(2, 3, 4)
Table'Flag
packing automatic budget=16
tag column[0] U8
common column[1] id U32
inline column[2] stride=1 align=1
case 0 off Off
  id U32 common[1]
case 1 on On
  id U32 common[1]
  is-lit Bool inline+0
(0)
(5, true)
```

### @hot switches a complete union to controlled packing while preserving common columns, canonical field order, wide aligned inline layouts, cold-only cases, typed mutation, and gathering.

```yumi
/core/table

. rows = Table'Node(3)
rows[0] = Node.literal(10, 40, 7)
rows[1] = Node.call(20, 1, 2, 3, 4, 5)
rows[2] = Node.cold(30, 9, 1)

-- rows.layout

for row in rows
    match row
        | .literal(literal)
            literal.value += 1
        | .call(call)
            call.callee += 10
        | .cold(cold)
            cold.payload += 1000

for row in rows
    match row
        | .literal(literal)
            literal.text-id += 10
        | .call(call)
            call.diagnostic-id += 100
        | .cold(cold)
            cold.cold-id += 100

for row in rows
    -- row as Node

rows[0] = Node.call(50, 6, 7, 8, 9, 10)
-- rows[0] as Node

━━━━━━━━━━━━━━━━━━━━
+ Syntax is Tuple

+ source U32

━━━━━━━━━━━━━━━━━━━━
+ HotSyntax is Syntax

+ @hot value U64

━━━━━━━━━━━━━━━━━━━━
+ Literal is HotSyntax

+ text-id U32

━━━━━━━━━━━━━━━━━━━━
+ Call is Syntax

+ @hot callee U64
+ @hot first-argument U64
+ @hot argument-count U16
+ flags U16
+ diagnostic-id U32

━━━━━━━━━━━━━━━━━━━━
+ Cold is Syntax

+ payload U64
+ cold-id U32

━━━━━━━━━━━━━━━━━━━━
+ Node is Union

| literal Literal
| call Call
| cold Cold
```

```output
Table'Node
packing controlled @hot
tag column[0] U8
common column[1] source U32
inline column[2] stride=24 align=8
case 0 literal Literal
  source U32 common[1]
  value U64 inline+0 @hot
  text-id U32 inline+8
case 1 call Call
  source U32 common[1]
  callee U64 inline+0 @hot
  first-argument U64 inline+8 @hot
  argument-count U16 inline+16 @hot
  flags U16 inline+18
  diagnostic-id U32 inline+20
case 2 cold Cold
  source U32 common[1]
  payload U64 inline+0
  cold-id U32 inline+8
(10, 41, 17)
(20, 11, 2, 3, 4, 105)
(30, 1009, 101)
(50, 6, 7, 8, 9, 10)
```

```yumi
/core/table

. rows = Table'Choice(1)
-- rows.layout

━━━━━━━━━━━━━━━━━━━━
+ Hot is Tuple

+ @hot key U32
+ wide U64
+ left U32
+ right U32

━━━━━━━━━━━━━━━━━━━━
+ Cold is Tuple

+ first U64
+ second U64

━━━━━━━━━━━━━━━━━━━━
+ Choice is Union

| hot Hot
| cold Cold
```

```output
Table'Choice
packing controlled @hot
tag column[0] U8
inline column[1] stride=16 align=8
spill-handles column[2] U32 offset+1
external arena stride=8 align=8
case 0 hot Hot
  key U32 inline+0 @hot
  wide U64 external+0
  left U32 inline+4
  right U32 inline+8
case 1 cold Cold
  first U64 inline+0
  second U64 inline+8
```

### Marking every case-specific field removes spill storage, while @hot remains inert for struct-backed tables and ordinary typed field access.

```yumi
/core/table

. values = Table'Value(2)
values[0] = Value.alpha(1, 2)
values[1] = Value.beta(3)
-- values.layout
-- values[0] as Value
-- values[1] as Value

. points = Table'Point(1)
points[0] = Point(4, 5)
points[0].x += 1
-- points.layout
-- points[0] as Point

━━━━━━━━━━━━━━━━━━━━
+ Alpha is Tuple

+ @hot x I32
+ @hot y I64

━━━━━━━━━━━━━━━━━━━━
+ Beta is Tuple

+ @hot z U16

━━━━━━━━━━━━━━━━━━━━
+ Value is Union

| alpha Alpha
| beta Beta

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ @hot x I32
+ y I32
```

```output
Table'Value
packing controlled @hot
tag column[0] U8
inline column[1] stride=16 align=8
case 0 alpha Alpha
  x I32 inline+0 @hot
  y I64 inline+8 @hot
case 1 beta Beta
  z U16 inline+0 @hot
(1, 2)
(3)
Table'Point
storage fixed
(5, 5)
```

### Arena-backed controlled unions use the same placement descriptor through growth and rewind, with compact external storage for unmarked fields.

```yumi
/core/table

/core/arena

. arena = Arena(16384)
do finally arena.release()
. nodes = Table'Node(arena, 1)

nodes.append(Node.small(10, 1, 2))
. mark = nodes.mark
nodes.append(Node.wide(20, 3, 4, 5, 6))
nodes.reserve(8)
nodes.append(Node.small(30, 7, 8))
nodes.append(Node.wide(40, 9, 10, 11, 12))
nodes.append(Node.wide(50, 13, 14, 15, 16))

match nodes[1]
    | .small(small)
        small.value += 0
        small.cold += 0
    | .wide(wide)
        wide.inline-a += 100
        wide.external-b += 1000

-- nodes.layout
-- "grown ${nodes.length}/${nodes.capacity}"
-- nodes[1] as Node

nodes.rewind(mark)
nodes.append(Node.wide(60, 17, 18, 19, 20))
-- "rewound ${nodes.length}/${nodes.capacity}"
-- nodes[0] as Node
-- nodes[1] as Node

━━━━━━━━━━━━━━━━━━━━
+ Syntax is Tuple

+ source U32

━━━━━━━━━━━━━━━━━━━━
+ Small is Syntax

+ @hot value U64
+ cold U32

━━━━━━━━━━━━━━━━━━━━
+ Wide is Syntax

+ @hot inline-a U64
+ @hot inline-b U64
+ external-a U64
+ external-b U64

━━━━━━━━━━━━━━━━━━━━
+ Node is Union

| small Small
| wide Wide
```

```output
Table'Node
storage arena-growable
row-layout union
packing controlled @hot
growth shared geometric
region column[0] tag width=1 align=1
region column[1] common source width=4 align=4
region column[2] inline width=16 align=8
region column[3] spill-handles width=4 align=4
compact external column[4] offset+1 U32 limit=4294967295 align=8
case 0 small Small
  source U32 common[1]
  value U64 inline+0 @hot
  cold U32 inline+8
case 1 wide Wide
  source U32 common[1]
  inline-a U64 inline+0 @hot
  inline-b U64 inline+8 @hot
  external-a U64 external+0
  external-b U64 external+8
grown 5/8
(20, 103, 4, 5, 1006)
rewound 2/8
(10, 1, 2)
(60, 17, 18, 19, 20)
```

```yumi
/core/table

/core/arena

. arena = Arena(4096)
do finally arena.release()
. rows = Table'Node(arena, 1)
. valid = true
for index in 0 .. < 64
    rows.append(Node.item(index as U64, SpanData(index as U32, 1), null))
for index in 0 .. < rows.length
    match rows[index]
        | .item(row)
            do
                rows[index] = Node.item(row.name, row.range, 305419896u64 + index as U64)
                valid = valid && row.range.start == index as U32 && row.range.count == 1
            0
        ??
            0
for index in 0 .. < rows.length
    match rows[index]
        | .item(row)
            do
                valid = valid && row.range.start == index as U32 && row.range.count == 1
                valid = valid && row.owner! == 305419896u64 + index as U64
            0
        ??
            0
-- valid

━━━━━━━━━━━━━━━━━━━━
+ SpanData is Tuple

+ start U32
+ count U32

━━━━━━━━━━━━━━━━━━━━
+ Item is Tuple

+ @hot name U64
+ range SpanData
+ @hot owner U64?

━━━━━━━━━━━━━━━━━━━━
+ Other is Tuple

+ value U8

━━━━━━━━━━━━━━━━━━━━
+ Node is Union

| item Item
| other Other
```

```output
true
```

### @hot on a field that becomes common is rejected when the union-table descriptor is formed.

```yumi crash
/core/table

. rows = Table'Value(1)

━━━━━━━━━━━━━━━━━━━━
+ Base is Tuple

+ @hot id U32

━━━━━━━━━━━━━━━━━━━━
+ Left is Base

+ left I32

━━━━━━━━━━━━━━━━━━━━
+ Right is Base

+ right I32

━━━━━━━━━━━━━━━━━━━━
+ Value is Union

| left Left
| right Right
```

```output
Exception: Table union 'Value' common field 'id' cannot be @hot because it already has a dedicated common column
<no file:9>
    + @hot id U32
    ^^^^^^^^^^^^^
```

### Managed case-specific fields remain illegal even when @hot requests inline placement.

```yumi crash
/core/table

. rows = Table'Value(1)

━━━━━━━━━━━━━━━━━━━━
+ Managed is Tuple

+ @hot text String

━━━━━━━━━━━━━━━━━━━━
+ Plain is Tuple

+ value I32

━━━━━━━━━━━━━━━━━━━━
+ Value is Union

| managed Managed
| plain Plain
```

```output
Exception: Table union case 'managed' field 'text' contains managed pointers through String
<no file:9>
    + @hot text String
    ^^^^^^^^^^^^^^^^^^
```

### @hot is rejected outside eager tuple storage.

```yumi crash
. box = Box()

━━━━━━━━━━━━━━━━━━━━
+ Box

+ @hot value I32 = 1
```

```output
Exception: Property annotation "@hot" requires an eager stored field on a tuple-layout type
<no file:7>
    + @hot value I32 = 1
    ^^^^^^^^^^^^^^^^^^^^
```

### @hot is rejected on lazy tuple properties.

```yumi crash
. value = LazyValue()

━━━━━━━━━━━━━━━━━━━━
+ LazyValue is Tuple

+ @hot value := 1
```

```output
Exception: Property annotation "@hot" requires an eager stored field on a tuple-layout type
<no file:7>
    + @hot value := 1
    ^^^^^^^^^^^^^^^^^
```

### @hot is rejected on tuple getters instead of being discarded by the seed parser.

```yumi crash
. value = GetterValue()

━━━━━━━━━━━━━━━━━━━━
+ GetterValue is Tuple

+ @hot value I32 => 1
```

```output
Exception: Property annotation "@hot" requires an eager stored field on a tuple-layout type
<no file:7>
    + @hot value I32 => 1
           ^^^^^^^^^
```

### @hot is rejected on globals.

```yumi crash
+ @hot value I32 = 1
-- value
```

```output
Exception: Property annotation "@hot" requires an eager stored field on a tuple-layout type
<no file:2>
    + @hot value I32 = 1
    ^^^^^^^^^^^^^^^^^^^^
```

### @hot is rejected on delegated tuple storage.

```yumi crash
. value = DelegatingValue(Delegate())

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ act

━━━━━━━━━━━━━━━━━━━━
+ Delegate
    is Proto

────────────────────
+ act

━━━━━━━━━━━━━━━━━━━━
+ DelegatingValue is Tuple

as Proto for
+ @hot delegate Delegate
```

```output
Exception: Property annotation "@hot" requires an eager stored field on a tuple-layout type
<no file:21>
    + @hot delegate Delegate
    ^^^^^^^^^^^^^^^^^^^^^^^^
```

### Unknown non-style property annotations name the supported tuple annotation.

```yumi crash
. value = Value(1)

━━━━━━━━━━━━━━━━━━━━
+ Value is Tuple

+ @cold field I32
```

```output
Exception: Unknown property annotation "@cold"; only "@hot" is supported on eager tuple fields
<no file:7>
    + @cold field I32
    ^^^^^^^^^^^^^^^^^
```

### @hot is rejected on interface properties.

```yumi crash
. proto Proto = Thing()
-- proto.value

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

+ @hot value I32

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Proto

+ value I32 = 1
```

```output
Exception: Property annotation "@hot" requires an eager stored field on a tuple-layout type
<no file:8>
    + @hot value I32
    ^^^^^^^^^^^^^^^^
```

### @hot is rejected on style properties while @inherited retains the style-specific path.

```yumi crash
/core/style
/core/markup
/core/tests/support/markup-fixtures

<BatchDemo demo
    <Label
        ink = 9

━━━━━━━━━━━━━━━━━━━━
+ BatchDemo is Box
    is MarkupRootElement
    using TextStyle

+ markup-batch-queue := MarkupBatchQueue(this)

────────────────────
+ do markup-batch-did-flush
    - flush MarkupBatchFlush

━━━━━━━━━━━━━━━━━━━━
+ TextStyle is Style

+ @hot ink I64 = 7

━━━━━━━━━━━━━━━━━━━━
+ Label is Box
    using TextStyle
```

```output
Exception: Property annotation "@hot" requires an eager stored field on a tuple-layout type
<no file:24>
    + @hot ink I64 = 7
    ^^^^^^^^^^^^^^^^^^
```

### Size-only union tables initialize every row to the first declared case's default value.

```yumi
/core/table

. nodes = Table'Node(2)
-- (nodes[0] as Node).case
-- nodes[0] as Node
-- nodes[1] as Node

━━━━━━━━━━━━━━━━━━━━
+ Literal is Tuple

+ source U32 = 12
+ value I64 = 34

━━━━━━━━━━━━━━━━━━━━
+ Call is Tuple

+ source U32
+ callee U32

━━━━━━━━━━━━━━━━━━━━
+ Node is Union

| literal Literal
| call Call
```

```output
0
(12, 34)
(12, 34)
```

### Whole-row union assignment changes cases repeatedly and clears or restores spill handles.

```yumi
/core/table

. nodes = Table'Node(1)
nodes[0] = Call(1, 2, 3, 4, 5)
-- nodes[0] as Node
nodes[0] = Literal(6, 7)
-- nodes[0] as Node
nodes[0] = Node.call(8, 9, 10, 11, 12)
-- nodes[0] as Node

. row = nodes[0]
match row
    | .literal(literal)
        literal.value += 0
    | .call(call)
        call.arg1 += 100
-- row as Node

━━━━━━━━━━━━━━━━━━━━
+ Syntax is Tuple

+ source U32

━━━━━━━━━━━━━━━━━━━━
+ Literal is Syntax

+ value I64

━━━━━━━━━━━━━━━━━━━━
+ Call is Syntax

+ callee U32
+ arg0 U64
+ arg1 U64
+ arg2 U64

━━━━━━━━━━━━━━━━━━━━
+ Node is Union

| literal Literal
| call Call
```

```output
(1, 2, 3, 4, 5)
(6, 7)
(8, 9, 10, 11, 12)
(8, 9, 10, 111, 12)
```

### Equal field names on unrelated cases do not become common columns.

```yumi crash
/core/table

. rows = Table'Value(1)
-- rows.layout
-- rows[0].value

━━━━━━━━━━━━━━━━━━━━
+ Left is Tuple

+ value I32

━━━━━━━━━━━━━━━━━━━━
+ Right is Tuple

+ value I32

━━━━━━━━━━━━━━━━━━━━
+ Value is Union

| left Left
| right Right
```

```output
Exception: TableRow'Value has no property "value"
<no file:6>
    -- rows[0].value
       ^^^^^^^^^^^^^
```

### Union table spans support slicing, common-column projection, indexing, and iteration.

```yumi
/core/table

. rows = Table'Value(4)
rows[0] = Value.left(10, 1)
rows[1] = Value.right(20, 2)
rows[2] = Value.left(30, 3)
rows[3] = Value.right(40, 4)

. span = rows[1 .. < 4]
. nested = span[1 .. < 3]
-- type-of span
-- "${span.length}/${span.size}"
-- ",".join(span.source)
-- nested[0] as Value

for row in span
    -- row as Value

━━━━━━━━━━━━━━━━━━━━
+ Base is Tuple

+ source U32

━━━━━━━━━━━━━━━━━━━━
+ Left is Base

+ value I32

━━━━━━━━━━━━━━━━━━━━
+ Right is Base

+ value I64

━━━━━━━━━━━━━━━━━━━━
+ Value is Union

| left Left
| right Right
```

```output
[TypeInfo TableSpan'Value]
3/3
20,30,40
(30, 3)
(20, 2)
(30, 3)
(40, 4)
```

### Managed pointers remain valid in common typed columns but are rejected in byte payload regions.

```yumi crash
/core/table

. rows = Table'Value(1)

━━━━━━━━━━━━━━━━━━━━
+ Base is Tuple

+ source U32

━━━━━━━━━━━━━━━━━━━━
+ TextValue is Base

+ text String

━━━━━━━━━━━━━━━━━━━━
+ NumberValue is Base

+ number I64

━━━━━━━━━━━━━━━━━━━━
+ Value is Union

| text TextValue
| number NumberValue
```

```output
Exception: Table union case 'text' field 'text' contains managed pointers through String
<no file:14>
    + text String
    ^^^^^^^^^^^^^
```

### Common managed fields retain typed-column storage and survive union scatter and gather.

```yumi
/core/table

. rows = Table'Value(2) • with :: index => do
    if index == 0
        = Value.left("first", 10)
    = Value.right("second", 20)

-- ", ".join(rows.name)
rows.name[1] = "changed"
-- rows[0] as Value
-- rows[1] as Value

━━━━━━━━━━━━━━━━━━━━
+ Base is Tuple

+ name String

━━━━━━━━━━━━━━━━━━━━
+ Left is Base

+ value I32

━━━━━━━━━━━━━━━━━━━━
+ Right is Base

+ value I64

━━━━━━━━━━━━━━━━━━━━
+ Value is Union

| left Left
| right Right
```

```output
first, second
(first, 10)
(changed, 20)
```

### Whole-row union view assignment copies the active logical value and keeps later mutations independent.

```yumi
/core/table

. rows = Table'Value(2)
rows[0] = Value.left(7)
rows[1] = Value.right(8)
rows[1] = rows[0]
rows[0] = Value.right(9)
rows[1] = rows[1]

-- rows[0] as Value
-- rows[1] as Value

━━━━━━━━━━━━━━━━━━━━
+ Left is Tuple

+ value I32

━━━━━━━━━━━━━━━━━━━━
+ Right is Tuple

+ value I64

━━━━━━━━━━━━━━━━━━━━
+ Value is Union

| left Left
| right Right
```

```output
(9)
(7)
```

### Controlled union row and range copies preserve active inline and spill fields, overlap order, and independent immutable-table clones.

```yumi
/core/table

. rows = Table'Node(5)
rows[0] = Node.small(10, 1, 100, 1000)
rows[1] = Node.wide(20, 2, 200, 2000, 2001)
rows[2] = Node.small(30, 3, 300, 3000)
rows[3] = Node.wide(40, 4, 400, 4000, 4001)
rows[4] = Node.small(50, 5, 500, 5000)

. span = rows[1 .. < 4]
. sources, flags = span
-- ",".join(sources)
-- ",".join(flags)

span[1] = rows[0]
rows[4] = span[0]
rows[1 .. < 5] = rows[0 .. < 4]

for row in rows
    -- row as Node

. clone = rows.copy()
rows[2] = Node.small(99, 9, 900, 9000)
-- clone[2] as Node

. lower = Table'Node(5)
lower[0 .. < 5] = clone[0 .. < 5]
lower[0 .. < 4] = lower[1 .. < 5]
for row in lower
    -- row as Node

━━━━━━━━━━━━━━━━━━━━
+ Syntax is Tuple

+ source U32
+ flags U16

━━━━━━━━━━━━━━━━━━━━
+ Small is Syntax

+ @hot value U64
+ cold U32

━━━━━━━━━━━━━━━━━━━━
+ Wide is Syntax

+ @hot value U64
+ external-a U64
+ external-b U64

━━━━━━━━━━━━━━━━━━━━
+ Node is Union

| small Small
| wide Wide
```

```output
20,30,40
2,3,4
(10, 1, 100, 1000)
(10, 1, 100, 1000)
(20, 2, 200, 2000, 2001)
(10, 1, 100, 1000)
(40, 4, 400, 4000, 4001)
(20, 2, 200, 2000, 2001)
(10, 1, 100, 1000)
(20, 2, 200, 2000, 2001)
(10, 1, 100, 1000)
(40, 4, 400, 4000, 4001)
(40, 4, 400, 4000, 4001)
```

### Returning a union row cursor preserves a live base view that observes later case changes.

```yumi
/core/table

. rows = Table'Value(1)
rows[0] = Value.left(3)
. row = first-row(rows)
rows[0] = Value.right(4)
-- row as Value

━━━━━━━━━━━━━━━━━━━━
+ Left is Tuple

+ value I32

━━━━━━━━━━━━━━━━━━━━
+ Right is Tuple

+ value I64

━━━━━━━━━━━━━━━━━━━━
+ Value is Union

| left Left
| right Right

━━━━━━━━━━━━━━━━━━━━
+ do first-row TableRow'Value
    - rows Table'Value

= rows[0]
```

```output
(4)
```

### A captured union row alias remains live and rematches the current active case at use.

```yumi
/core/table

. rows = Table'Value(1)
rows[0] = Value.left(5)
. row = rows[0]
. getter = => row
rows[0] = Value.right(6)
-- getter() as Value

━━━━━━━━━━━━━━━━━━━━
+ Left is Tuple

+ value I32

━━━━━━━━━━━━━━━━━━━━
+ Right is Tuple

+ value I64

━━━━━━━━━━━━━━━━━━━━
+ Value is Union

| left Left
| right Right
```

```output
(6)
```

### Union base row views pass, return, yield, capture, and store as live cursors, while ordinary-union and narrowed-case boundaries gather snapshots.

```yumi
/core/table

. rows = Table'Node(3)
rows[0] = Node.left(10, 100)
rows[1] = Node.right(20, 200)
rows[2] = Node.left(30, 300)

. direct = row-at(rows, 0)
. passed = pass-row(rows[1])
. stream = yielded-rows(rows)
. yielded-direct = stream()!
. yielded-loop = stream()!
. tuple = (rows[0], rows[2])
. tuple-first, tuple-last = tuple
. array = [rows[0], rows[1]]
. list = List'(TableRow'Node)()
list += rows[1]
. holder = Holder(rows[2])
. captured = rows[2]
. getter = => captured
. lazy-getter = make-lazy-getter(rows)
. case-snapshot = snapshot-left(rows[0])
. widened = widen-left(rows[0])
. union-snapshot = pass-value(rows[2])

rows[0] = Node.right(11, 110)
rows[1] = Node.left(21, 210)
rows[2] = Node.right(31, 310)

-- direct as Node
-- passed as Node
-- yielded-direct as Node
-- yielded-loop as Node
-- tuple-first as Node
-- tuple-last as Node
-- array[0] as Node
-- array[1] as Node
-- list[0] as Node
-- holder.row as Node
-- getter() as Node
-- lazy-getter() as Node
-- case-snapshot
-- widened as Node
-- union-snapshot

━━━━━━━━━━━━━━━━━━━━
+ Base is Tuple

+ source U32

━━━━━━━━━━━━━━━━━━━━
+ Left is Base

+ value I32

━━━━━━━━━━━━━━━━━━━━
+ Right is Base

+ value I64

━━━━━━━━━━━━━━━━━━━━
+ Node is Union

| left Left
| right Right

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ row TableRow'Node

────────────────────
+ This
    - row TableRow'Node

━━━━━━━━━━━━━━━━━━━━
+ row-at TableRow'Node
    - rows Table'Node
    - index Index

rows[index]

━━━━━━━━━━━━━━━━━━━━
+ pass-row TableRow'Node
    - row TableRow'Node

row

━━━━━━━━━━━━━━━━━━━━
+ do yielded-rows
    - rows Table'Node

<= rows[1]
for row in rows
    <= row

━━━━━━━━━━━━━━━━━━━━
+ do make-lazy-getter Getter'(TableRow'Node)
    - rows Table'Node

. index = 0
.. row = rows[index]
. getter = => row
index = 1
= getter

━━━━━━━━━━━━━━━━━━━━
+ snapshot-left Left
    - row TableRow'Node

match row
    | .left(left)
        left
    | .right
        Left(0, 0)

━━━━━━━━━━━━━━━━━━━━
+ widen-left TableRow'Node
    - row TableRow'Node

match row
    | .left(left)
        left as TableRow'Node
    | .right
        row

━━━━━━━━━━━━━━━━━━━━
+ pass-value Node
    - node Node

node
```

```output
(11, 110)
(21, 210)
(21, 210)
(11, 110)
(11, 110)
(31, 310)
(11, 110)
(21, 210)
(21, 210)
(31, 310)
(31, 310)
(21, 210)
(10, 100)
(11, 110)
(30, 300)
```

### Row field reads inside closures do not drive table loop cursor planning.

```yumi
/core/table

. table = Table'Point(2) • with Point(each as Size, (each + 10) as Size)

for row in table
    . fn = => row.x + row.x
    row.x += 5
    -- fn()

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Size
+ y Size
```

```output
10
12
```

### Arena-backed Table grows while aliases and pre-growth views keep resolving current storage.

```yumi
/core/table

/core/arena

. arena = Arena(8192)
do finally arena.release()

. table = Table'Point(arena, 1)
. alias = table
-- "initial ${table.length}/${table.capacity}"

. first = table.append(Point(1, 10))
. row = table[first]
. xs = table.x
. span = table[0 .. < 1]

table.reserve(8)
. more = Array'Point([Point(2, 20), Point(3, 30)])
table.extend(more)
-- "grown ${alias.length}/${alias.capacity}"
-- row as Point
-- xs[0]
-- span[0] as Point
for current in alias
    -- "loop ${current as Point}"

. mark = table.mark
table.append(Point(4, 40))
-- "appended ${table.length}/${table.capacity}"
table.rewind(mark)
-- "rewound ${table.length}/${table.capacity}"
-- table[2] as Point

table.truncate(2)
-- "truncated ${table.length}/${table.capacity}"
table.clear()
-- "cleared ${alias.length}/${alias.capacity}"

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
initial 0/4
grown 3/8
(1, 10)
1
(1, 10)
loop (1, 10)
loop (2, 20)
loop (3, 30)
appended 4/8
rewound 3/8
(3, 30)
truncated 2/8
cleared 0/8
```

### Arena-backed union tables share row growth and recycle compact spill storage through table marks.

```yumi
/core/table

/core/arena

. arena = Arena(16384)
do finally arena.release()
. nodes = Table'Node(arena, 1)

nodes.append(Node.literal(10, 42))
. mark = nodes.mark
. first = nodes[0]
nodes.append(Node.call(20, 1, 2, 3, 4))
nodes.append(Node.literal(30, 43))
nodes.append(Node.call(40, 5, 6, 7, 8))
nodes.append(Node.call(50, 9, 10, 11, 12))

-- "grown ${nodes.length}/${nodes.capacity}"
-- first as Node
-- nodes[1] as Node
-- nodes[4] as Node

nodes.rewind(mark)
-- "rewound ${nodes.length}/${nodes.capacity}"
nodes.append(Node.call(60, 13, 14, 15, 16))
-- nodes[0] as Node
-- nodes[1] as Node

━━━━━━━━━━━━━━━━━━━━
+ Syntax is Tuple

+ source U32

━━━━━━━━━━━━━━━━━━━━
+ Literal is Syntax

+ value I64

━━━━━━━━━━━━━━━━━━━━
+ Call is Syntax

+ callee U32
+ arg0 U64
+ arg1 U64
+ arg2 U64

━━━━━━━━━━━━━━━━━━━━
+ Node is Union

| literal Literal
| call Call
```

```output
grown 5/8
(10, 42)
(20, 1, 2, 3, 4)
(50, 9, 10, 11, 12)
rewound 1/8
(10, 42)
(60, 13, 14, 15, 16)
```

### Fixed-size constructors keep their source-compatible shape and reject grow-only operations.

```yumi
/core/table

. table = Table'Point(1)
. attempt = do new
    table.append(Point(1, 2))
catch failure
    -- failure
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
Table.append requires an Arena-backed growable table
```

### Arena reset and table shrink invalidate every previously captured table, row, span, and field view.

```yumi
/core/table

/core/arena

. arena = Arena(8192)
do finally arena.release()
. table = Table'Point(arena)
table.append(Point(1, 2))
. row = table[0]
. span = table[0 .. < 1]
. field = table.x

arena.reset()
show-stale-table(table)
show-stale-row(row)
show-stale-span(span)
show-stale-field(field)

. current = Table'Point(arena)
current.append(Point(3, 4))
. current-row = current[0]
current.truncate(0)
show-stale-row(current-row)

━━━━━━━━━━━━━━━━━━━━
+ do show-stale-table
    - table Table'Point

. attempt = do new
    -- table.length
catch failure
    -- failure
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ do show-stale-row
    - row TableRow'Point

. attempt = do new
    -- row.x
catch failure
    -- failure
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ do show-stale-span
    - span TableSpan'Point

. attempt = do new
    -- span.length
catch failure
    -- failure
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ do show-stale-field
    - field TableField'I32

. attempt = do new
    -- field[0]
catch failure
    -- failure
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
Table is stale after Arena reset
TableRow is stale after Arena reset
TableSpan is stale after Arena reset
TableField is stale after Arena reset
TableRow was invalidated by table shrink
```

### A table of a released arena is stale, and stays stale once a new arena takes the slot.

```yumi
/core/table

/core/arena

. arena = Arena(8192)
. table = Table'Point(arena)
table.append(Point(1, 2))
. row = table[0]
arena.release()
show-stale-table(table)
show-stale-row(row)

. again = Arena(8192)
do finally again.release()
. fresh = Table'Point(again)
fresh.append(Point(5, 6))
show-stale-table(table)
show-stale-row(row)
-- fresh[0].x
again.reset()
show-stale-table(fresh)
. later = Table'Point(again)
later.append(Point(7, 8))
-- later[0].y

━━━━━━━━━━━━━━━━━━━━
+ do show-stale-table
    - table Table'Point

. attempt = do new
    -- table.length
catch failure
    -- failure
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ do show-stale-row
    - row TableRow'Point

. attempt = do new
    -- row.x
catch failure
    -- failure
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
Table is stale after Arena reset
TableRow is stale after Arena reset
Table is stale after Arena reset
TableRow is stale after Arena reset
5
Table is stale after Arena reset
8
```

### Table marks reject foreign tables and in-place row mutations without disturbing either table.

```yumi
/core/table

/core/arena

. arena = Arena(8192)
do finally arena.release()
. left = Table'Point(arena)
. right = Table'Point(arena)
left.append(Point(1, 2))
right.append(Point(3, 4))

. foreign = left.mark
show-rejected-rewind(right, foreign)

. mutated = left.mark
left[0].x = 9
show-rejected-rewind(left, mutated)

-- left[0] as Point
-- right[0] as Point

━━━━━━━━━━━━━━━━━━━━
+ do show-rejected-rewind
    - table Table'Point
    - mark TableMark

. attempt = do new
    table.rewind(mark)
catch failure
    -- failure
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
Table mark is stale or belongs to another table
Table mark is stale or belongs to another table
(9, 2)
(3, 4)
```

### Arena-backed tables accept ordinary String leaves with developer-managed Arena lifetime.

```yumi
/core/table

/core/arena

. arena = Arena(1024)
do finally arena.release()
. rows = Table'Managed(arena)
rows.append(Managed(arena.copy-string("arena text")))
-- rows[0] as Managed

━━━━━━━━━━━━━━━━━━━━
+ Managed is Struct

+ text String
```

```output
(arena text)
```

### Arena-backed tables reject arbitrary managed containers.

```yumi crash
/core/table

/core/arena

. arena = Arena(1024)
do finally arena.release()
. rows = Table'ManagedList(arena)

━━━━━━━━━━━━━━━━━━━━
+ ManagedList is Struct

+ values List'String
```

```output
Exception: Arena type ManagedList contains managed pointers through ManagedList.values
<no file:11>
    + ManagedList is Struct
    ^^^^^^^^^^^^^^^^^^^^^^
```

### Storage checkpoint validation rejects foreign and pre-mark-mutated tables before Arena rewind.

```yumi
/core/table

/core/arena

. arena = Arena(8192)
do finally arena.release()
. left = Table'Point(arena)
. right = Table'Point(arena)
left.append(Point(1, 2))
right.append(Point(3, 4))
. mark = arena.mark
. checkpoint = left.storage-checkpoint(arena, mark)
. retained = arena.allocate'U64(1)
retained[0] = 77u64

show-rejected-storage-restore(right, checkpoint, arena, mark)
left[0].x = 9
show-rejected-storage-restore(left, checkpoint, arena, mark)
-- left[0] as Point
-- right[0] as Point
-- retained[0]

━━━━━━━━━━━━━━━━━━━━
+ do show-rejected-storage-restore
    - table Table'Point
    - checkpoint TableStorageCheckpoint
    - arena Arena
    - mark ArenaMark

. attempt = do new
    table.validate-storage-restore(checkpoint, arena, mark)
catch failure
    -- failure
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
Table storage checkpoint is stale or belongs to another table
Table storage checkpoint is stale or belongs to another table
(9, 2)
(3, 4)
77
```

### Fixed column-zipped spans retain their read path inside optimized loops.

```yumi
/core/table

. table = Table'Point(2) • with Point((each + 1) as I32, (each + 10) as I32)
. points TableSpan'Point = table.x + table.y
for point in points
    -- point.x
    -- point.y

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
1
10
2
11
```

### Controlled union rows copy between Arena-backed and fixed tables, fixed clones outlive Arena reset, and a shrink invalidates previously captured union row views.

```yumi
/core/table

/core/arena

. source-arena = Arena(16384)
do finally source-arena.release()
. source = Table'Node(source-arena, 1)
source.append(Node.wide(10, 1, 100, 1000, 1001))
source.append(Node.small(20, 2, 200, 2000))
. live = source[0]
source.reserve(8)
-- live as Node

. fixed = source.copy()
source-arena.reset()
-- fixed[0] as Node
-- fixed[1] as Node

. target-arena = Arena(16384)
do finally target-arena.release()
. target = Table'Node(target-arena, 1)
target.append(Node.small(0, 0, 0, 0))
target.append(Node.small(0, 0, 0, 0))
target[0 .. < 2] = fixed[0 .. < 2]
fixed[0] = Node.small(99, 9, 900, 9000)
-- target[0] as Node
-- target[1] as Node

. target-fixed = target.copy()
. stale = target[0]
target.truncate(1)
show-stale-node(stale)
target-arena.reset()
-- target-fixed[0] as Node
-- target-fixed[1] as Node

━━━━━━━━━━━━━━━━━━━━
+ do show-stale-node
    - row TableRow'Node

. attempt = do new
    -- row as Node
catch failure
    -- failure
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ Syntax is Tuple

+ source U32
+ flags U16

━━━━━━━━━━━━━━━━━━━━
+ Small is Syntax

+ @hot value U64
+ cold U32

━━━━━━━━━━━━━━━━━━━━
+ Wide is Syntax

+ @hot value U64
+ external-a U64
+ external-b U64

━━━━━━━━━━━━━━━━━━━━
+ Node is Union

| small Small
| wide Wide
```

```output
(10, 1, 100, 1000, 1001)
(10, 1, 100, 1000, 1001)
(20, 2, 200, 2000)
(10, 1, 100, 1000, 1001)
(20, 2, 200, 2000)
TableRow was invalidated by table shrink
(10, 1, 100, 1000, 1001)
(20, 2, 200, 2000)
```

### Union, shared-base, and narrowed-case extensions retain live Table row storage while ordinary union, common-base, and concrete-case results snapshot it.

```yumi
/core/table

/core/arena

. rows = Table'Node(2)
rows[0] = Node.small(10, 1, 100)
rows[1] = Node.wide(20, 2, 3, 200, 300)

. union-snapshot = rows[0].snapshot()
. common-snapshot = rows[0].common-snapshot()
. returned = rows[0].live()
. assigned TableRow'Node = rows[0].live()
. captured = rows[0].live()
. getter = => captured
. stored = List'(TableRow'Node)()
stored += rows[0].live()

do match rows[0]
    | .small(small)
        . case-snapshot = small.case-snapshot()
        small.lower(5)
        -- case-snapshot
        -- "bumped"
    | .wide
        -- "unexpected"

rows[0].bump-source(1)
rows[0].lower(10)
rows[1].lower(20)

-- union-snapshot
-- common-snapshot
-- returned as Node
-- assigned as Node
-- getter() as Node
-- stored[0] as Node
-- rows[1] as Node

. arena = Arena(8192)
do finally arena.release()
. arena-rows = Table'Node(arena, 1)
arena-rows.append(Node.wide(30, 4, 5, 400, 500))
. arena-live = arena-rows[0].live()
arena-rows.reserve(8)
arena-rows[0].lower(7)
-- arena-live as Node

. stale = arena-rows[0].live()
arena.reset()
show-stale(stale)

━━━━━━━━━━━━━━━━━━━━
+ Syntax is Tuple

+ source U32

━━━━━━━━━━━━━━━━━━━━
+ Small is Syntax

+ @hot value U64
+ cold U32

━━━━━━━━━━━━━━━━━━━━
+ Wide is Syntax

+ @hot inline-a U64
+ @hot inline-b U64
+ spill-a U64
+ spill-b U64

━━━━━━━━━━━━━━━━━━━━
+ Node is Union

| small Small
| wide Wide

━━━━━━━━━━━━━━━━━━━━
+ extend Node

────────────────────
+ do lower
    - delta U64

match this
    | .small(small)
        small.lower(delta)
    | .wide(wide)
        wide.lower(delta)

────────────────────
+ snapshot Node

this

────────────────────
+ common-snapshot Syntax

this

────────────────────
+ live TableRow'Node

this

────────────────────
+ describe U32

match this
    | .small(small)
        small.source
    | .wide(wide)
        wide.source

━━━━━━━━━━━━━━━━━━━━
+ extend Syntax

────────────────────
+ do bump-source
    - delta U32

this.source += delta

━━━━━━━━━━━━━━━━━━━━
+ extend Small

────────────────────
+ do lower
    - delta U64

this.source += delta as U32
this.value += delta
this.cold += delta as U32

────────────────────
+ case-snapshot Small

this

━━━━━━━━━━━━━━━━━━━━
+ extend Wide

────────────────────
+ do lower
    - delta U64

this.source += delta as U32
this.inline-a += delta
this.spill-a += delta

━━━━━━━━━━━━━━━━━━━━
+ do show-stale
    - row TableRow'Node

. attempt = do new
    -- row.describe()
catch failure
    -- failure
attempt.wait()
```

```output
(10, 1, 100)
bumped
(10, 1, 100)
(10)
(26, 16, 115)
(26, 16, 115)
(26, 16, 115)
(26, 16, 115)
(40, 22, 3, 220, 300)
(37, 11, 5, 407, 500)
TableRow is stale after Arena reset
```

### Arena table and side-builder checkpoints restore String-bearing union storage in constant-size steps.

```yumi
/core/table

/core/arena
/core/array-builder

. arena = Arena(65536)
do finally arena.release()
. rows = Table'Node(arena, 1)
. side = ArrayBuilder'String(arena)
rows.append(Node.literal(arena.copy-string("root"), arena.copy-string("token"), arena.copy-string("detail")))
side.append(arena.copy-string("kept"))

. arena-mark = arena.mark
. rows-checkpoint = rows.storage-checkpoint(arena, arena-mark)
. side-checkpoint = side.storage-checkpoint(arena, arena-mark)
rows.append(Node.call(arena.copy-string("post"), arena.copy-string("callee"), arena.copy-string("diagnostic")))
. post-row = rows[1]
. post-span = rows[1 .. < 2]
. post-field = rows.source
. post-id = 1
for index in 0 .. < 100
    rows.append(Node.call(arena.copy-string("source-$index"), arena.copy-string("call-$index"), arena.copy-string("diagnostic-$index")))
    side.append(arena.copy-string("side-$index"))

rows.validate-storage-restore(rows-checkpoint, arena, arena-mark)
side.validate-storage-restore(side-checkpoint, arena, arena-mark)
arena.rewind(arena-mark)
rows.restore-storage(rows-checkpoint, arena, arena-mark)
side.restore-storage(side-checkpoint, arena, arena-mark)

-- rows[0] as Node
show-restored-row(post-row)
show-restored-span(post-span)
show-restored-field(post-field)
show-restored-id(rows, post-id)

rows.append(Node.call(arena.copy-string("again"), arena.copy-string("regrown"), null))
side.append(arena.copy-string("regrown"))
-- rows[1] as Node
. restored-side = side.finish-immutable()
-- "${restored-side[0]}/${restored-side[1]}"

. standalone-mark = rows.mark
. standalone-capacity = rows.capacity
rows.reserve(256)
rows.append(Node.literal(arena.copy-string("temporary"), arena.copy-string("temporary"), null))
rows.rewind(standalone-mark)
-- "${rows.length}/${rows.capacity > standalone-capacity}"

. operations = 0
for round in 0 .. < 100
    . repeat-arena-mark = arena.mark
    . repeat-rows-checkpoint = rows.storage-checkpoint(arena, repeat-arena-mark)
    rows.append(Node.call(arena.copy-string("repeat"), arena.copy-string("repeat"), null))
    rows.validate-storage-restore(repeat-rows-checkpoint, arena, repeat-arena-mark)
    arena.rewind(repeat-arena-mark)
    rows.restore-storage(repeat-rows-checkpoint, arena, repeat-arena-mark)
    operations += 1
-- "repeat $operations ${rows.length}/${restored-side.length}"

. empty-rows = Table'Node(arena)
. empty-side = ArrayBuilder'String(arena)
. empty-mark = arena.mark
. empty-rows-checkpoint = empty-rows.storage-checkpoint(arena, empty-mark)
. empty-side-checkpoint = empty-side.storage-checkpoint(arena, empty-mark)
empty-rows.append(Node.literal(arena.copy-string("discard"), arena.copy-string("discard"), null))
empty-side.append(arena.copy-string("discard"))
empty-rows.validate-storage-restore(empty-rows-checkpoint, arena, empty-mark)
empty-side.validate-storage-restore(empty-side-checkpoint, arena, empty-mark)
arena.rewind(empty-mark)
empty-rows.restore-storage(empty-rows-checkpoint, arena, empty-mark)
empty-side.restore-storage(empty-side-checkpoint, arena, empty-mark)
-- "empty ${empty-rows.length}/${empty-side.length}"

━━━━━━━━━━━━━━━━━━━━
+ Syntax is Tuple

+ source String

━━━━━━━━━━━━━━━━━━━━
+ Literal is Syntax

+ @hot token String
+ detail String?

━━━━━━━━━━━━━━━━━━━━
+ Call is Syntax

+ @hot callee String
+ diagnostic String?

━━━━━━━━━━━━━━━━━━━━
+ Node is Union

| literal Literal
| call Call

━━━━━━━━━━━━━━━━━━━━
+ do show-restored-row
    - row TableRow'Node

. attempt = do new
    -- row.source
catch failure
    -- failure
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ do show-restored-span
    - span TableSpan'Node

. attempt = do new
    -- span.length
catch failure
    -- failure
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ do show-restored-field
    - field TableField'String

. attempt = do new
    -- field[0]
catch failure
    -- failure
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ do show-restored-id
    - rows Table'Node
    - index Index

. attempt = do new
    -- rows[index].source
catch failure
    -- failure
attempt.wait()
```

```output
(root, token, detail)
TableRow was invalidated by table shrink
TableSpan was invalidated by table shrink
TableField was invalidated by table shrink
Table index is out of bounds
(again, regrown, unknown)
kept/regrown
2/true
repeat 100 2/2
empty 0/0
```

```yumi
/core/table

/BxGC/BxGC.h

. table = Table'Point(4) • with Point(each as I32, (each + 10) as I32)
. ids = Array'U32([3u32, 1u32, 1u32, 0u32])
. selected = table[ids]
-- type-of selected
-- selected.length
-- selected[0] as Point
-- selected[3] as Point
for row in selected
    -- row as Point

. middle = selected[1 .. < 3]
-- middle.length
for row in middle
    -- row as Point

ids[0] = 2u32
-- selected[0] as Point
selected[1].y = 77
-- table[1] as Point

. span = IndexSpan'U32(ids, 1 .. < 3)
-- type-of span
-- span.length
-- span[0]
for index in span
    -- index
for row in table[span]
    -- row as Point

. sequence = ids.immutable
for row in table[sequence[0 .. < 2]]
    -- row as Point

-- table[Array'U32()].length
-- table[Array'I8([3 as I8])][0] as Point
-- table[Array'U8([3 as U8])][0] as Point
-- table[Array'I16([2 as I16])][0] as Point
-- table[Array'U16([2 as U16])][0] as Point
-- table[Array'I32([1 as I32])][0] as Point
-- table[Array'I64([0 as I64])][0] as Point
-- table[Array'U64([0 as U64])][0] as Point

show-invalid-i32(table[Array'I32([-1 as I32])])
show-invalid-u32(table[Array'U32([4u32])])
show-invalid-span(ids)
ids = Array'U32()
C:brixGCCollect()
-- selected[0] as Point

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32

━━━━━━━━━━━━━━━━━━━━
+ do show-invalid-i32
    - selection TableSelection'Point'I32

. attempt = do new
    -- selection[0].x
catch failure
    -- failure
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ do show-invalid-u32
    - selection TableSelection'Point'U32

. attempt = do new
    -- selection[0].x
catch failure
    -- failure
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ do show-invalid-span
    - ids Array'U32

. attempt = do new
    . invalid = IndexSpan'U32(ids, 0 .. < 5)
    -- invalid.length
catch failure
    -- failure
attempt.wait()
```

```output
[TypeInfo TableSelection'Point'U32]
4
(3, 13)
(0, 10)
(3, 13)
(1, 11)
(1, 11)
(0, 10)
2
(1, 11)
(1, 11)
(2, 12)
(1, 77)
[TypeInfo IndexSpan'U32]
2
1
1
1
(1, 77)
(1, 77)
(2, 12)
(1, 77)
0
(3, 13)
(3, 13)
(2, 12)
(2, 12)
(1, 77)
(0, 10)
(0, 10)
TableSelection index is out of bounds
Table index is out of bounds
IndexSpan range is out of bounds
(2, 12)
```

```yumi
/core/table

/core/arena
/core/array-builder

. table = Table'Point(4) • with Point(each as I32, (each + 10) as I32)
. arena = Arena(8192)
do finally arena.release()

. growing = ArrayBuilder'U32(arena)
growing.append(0u32)
growing.append(2u32)
. growing-selection = table[growing]
for value in 0 .. < 100
    growing.append((value % 4) as U32)
-- growing-selection.length
for row in growing-selection
    -- row as Point
growing[0] = 3u32
-- growing-selection[0] as Point

. moving = ArrayBuilder'U32(arena)
moving.append(0u32)
moving.append(1u32)
. moving-selection = table[moving]
. moving-position = 0
for row in moving-selection
    -- row as Point
    if moving-position == 0
        for value in 0 .. < 100
            moving.append((value % 4) as U32)
        moving[1] = 3u32
    moving-position += 1

. empty-builder = ArrayBuilder'U32(arena)
. empty-selection = table[empty-builder]
-- empty-selection.length
empty-builder.append(1u32)
-- empty-selection.length

. growable-table = Table'Point(arena)
growable-table.append(Point(5, 15))
growable-table.append(Point(6, 16))
. growable-selection = growable-table[Array'U32([1u32])]
for value in 0 .. < 32
    growable-table.append(Point(value as I32, (value + 20) as I32))
-- growable-selection[0] as Point
growable-table.truncate(1)
show-stale-selection(growable-selection)

. rewind-builder = ArrayBuilder'U32(arena)
rewind-builder.append(1u32)
. rewind-selection = table[rewind-builder]
. rewind-mark = rewind-builder.mark
rewind-builder.append(2u32)
rewind-builder.rewind(rewind-mark)
show-stale-selection(rewind-selection)

. finish-builder = ArrayBuilder'U32(arena)
finish-builder.append(2u32)
. finish-selection = table[finish-builder]
. finished = finish-builder.finish-immutable()
-- finished[0]
show-stale-selection(finish-selection)

. arena-mark = arena.mark
. rewind-arena-builder = ArrayBuilder'U32(arena)
rewind-arena-builder.append(0u32)
. rewind-arena-selection = table[rewind-arena-builder]
arena.rewind(arena-mark)
show-stale-selection(rewind-arena-selection)

. reset-builder = ArrayBuilder'U32(arena)
reset-builder.append(1u32)
. reset-selection = table[reset-builder]
arena.reset()
show-stale-selection(reset-selection)

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32

━━━━━━━━━━━━━━━━━━━━
+ do show-stale-selection
    - selection TableSelection'Point'U32

. attempt = do new
    -- selection[0].x
catch failure
    -- failure
attempt.wait()
```

```output
2
(0, 10)
(2, 12)
(3, 13)
(0, 10)
(3, 13)
0
0
(6, 16)
Table index is out of bounds
IndexSpan was invalidated by ArrayBuilder mutation
2
IndexSpan was invalidated by ArrayBuilder mutation
IndexSpan was invalidated by Arena rewind
IndexSpan is stale after Arena reset
```

```yumi
/core/table

. rows = Table'Node(4)
rows[0] = Node.small(10, 100)
rows[1] = Node.wide(20, 1, 2, 200, 300)
rows[2] = Node.small(30, 700)
rows[3] = Node.wide(40, 3, 4, 800, 900)
. ids = Array'U32([2u32, 1u32, 2u32, 3u32])
. selected = rows[ids]

for row in selected
    -- match row
        | .small(small)
            small.value
        | .wide(wide)
            wide.inline-a + wide.external-b

for row in selected
    match row
        | .small(small)
            small.value += 1
        | .wide(wide)
            wide.external-a = wide.external-a + wide.external-b

-- rows[2] as Node
-- rows[1] as Node
-- rows[3] as Node

━━━━━━━━━━━━━━━━━━━━
+ Syntax is Tuple

+ source U32

━━━━━━━━━━━━━━━━━━━━
+ Small is Syntax

+ @hot value U64

━━━━━━━━━━━━━━━━━━━━
+ Wide is Syntax

+ @hot inline-a U64
+ @hot inline-b U64
+ external-a U64
+ external-b U64

━━━━━━━━━━━━━━━━━━━━
+ Node is Union

| small Small
| wide Wide
```

```output
700
301
700
903
(30, 702)
(20, 1, 2, 500, 300)
(40, 3, 4, 1700, 900)
```

```yumi
/core/table
/core/arena
/core/array-builder

. arena = Arena(8192)
do finally arena.release()
. rows = Table'Node(arena, 2)
rows.append(Node.small((1, 2)))
rows.append(Node.wide((3, 4), 4, 5, 6, 7))
-- snapshot(NodeStore(rows))

━━━━━━━━━━━━━━━━━━━━
+ NodeStore

+ nodes Table'Node

────────────────────
+ This
    - nodes Table'Node

━━━━━━━━━━━━━━━━━━━━
_ do snapshot
    - store NodeStore

. parts = ArrayBuilder'String()
for node in store.nodes
    parts.append
        match node
            | .small(row)
                "row: $row"
            | .wide(row)
                "row: $row"

= "\n".join(parts.finish-immutable())

━━━━━━━━━━━━━━━━━━━━
+ Syntax is Tuple

+ source (start U64, end U64)

━━━━━━━━━━━━━━━━━━━━
+ Small is Syntax

━━━━━━━━━━━━━━━━━━━━
+ Wide is Syntax

+ @hot inline-a U64
+ @hot inline-b U64
+ external-a U64
+ external-b U64

━━━━━━━━━━━━━━━━━━━━
+ Node is Union

| small Small
| wide Wide
```

```output
row: ((1, 2))
row: ((3, 4), 4, 5, 6, 7)
```

### copy-from replaces an Arena-backed table's rows with another table's rows, growing, shrinking, and copying union spill storage.

```yumi
/core/arena
/core/table

. arena = Arena(8192)
do finally arena.release()
. live = Table'Point(arena, 2)
. saved = Table'Point(arena, 1)
live.append(Point(1, 10))
live.append(Point(2, 20))
live.append(Point(3, 30))
saved.copy-from(live)
live[1].y = 99
live.append(Point(4, 40))
-- "saved=${saved.length} live=${live.length} y=${saved[1].y}"
live.copy-from(saved)
-- "restored=${live.length} y=${live[1].y} x=${live[2].x}"
live.copy-from(live)
-- "self=${live.length}"
. fixed = Table'Point(1) • with Point(each + 7, 70)
saved.copy-from(fixed)
-- "fixed=${saved.length} ${saved[0] as Point}"

. nodes = Table'Node(arena, 1)
. backup = Table'Node(arena, 1)
nodes.append(Node.small(1))
nodes.append(Node.wide(2, 3, 4, 5, 6))
backup.copy-from(nodes)
match nodes[1]
    | .small(small)
        small.source += 0
    | .wide(wide)
        wide.external-a = 500
nodes.copy-from(backup)
-- nodes[1] as Node
-- backup[0] as Node

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I64
+ y I64

━━━━━━━━━━━━━━━━━━━━
+ Syntax is Tuple

+ source U64

━━━━━━━━━━━━━━━━━━━━
+ Small is Syntax

━━━━━━━━━━━━━━━━━━━━
+ Wide is Syntax

+ @hot inline-a U64
+ @hot inline-b U64
+ external-a U64
+ external-b U64

━━━━━━━━━━━━━━━━━━━━
+ Node is Union

| small Small
| wide Wide
```

```output
saved=3 live=4 y=20
restored=3 y=20 x=3
self=3
fixed=1 (7, 70)
(2, 3, 4, 5, 6)
(1)
```

### A table loop's row variable shadows a local of the same name.

```yumi
/core/table

. table = Table'ShadowRow(3) • with ShadowRow((each + 5) as U32)
-- row-sum(table)
-- index-sum(table)

━━━━━━━━━━━━━━━━━━━━
+ ShadowRow is Struct

+ value U32

━━━━━━━━━━━━━━━━━━━━
+ do row-sum U32
    - rows Table'ShadowRow

. total U32 = 0
. row = rows[0]
for row in rows
    total += row.value
= total + row.value * 100

━━━━━━━━━━━━━━━━━━━━
+ do index-sum U32
    - rows Table'ShadowRow

. total U32 = 0
. index U32 = 9
for index in 0 .. < rows.length
    total += rows[index].value
= total + index * 100
```

```output
518
918
```

### A row read through an index does not outlive an assignment to the index.

```yumi
/core/table

. table = Table'StepRow(3) • with StepRow((each * 2 + 7) as I32)
-- step(table)
-- walk(table, 3)

━━━━━━━━━━━━━━━━━━━━
+ StepRow is Struct

+ offset I32

━━━━━━━━━━━━━━━━━━━━
+ do step I32
    - rows Table'StepRow

. index U32 = 0
. first = rows[index].offset
index += 1
. second = rows[index].offset
index = index + 1
= first * 10000 + second * 100 + rows[index].offset

━━━━━━━━━━━━━━━━━━━━
+ do walk I32
    - rows Table'StepRow
    - count U32

. total I32 = 0
. index U32 = 0
while index < count
    . first = rows[index].offset
    index += 1
    if index < count
        total += rows[index].offset - first
= total
```

```output
70911
4
```

```yumi
/core/table

. rows = Table'Node(2)
rows[0] = Node.low(10, Pair(1, 2))
rows[1] = Node.high(30, Pair(3, 4))
. reader = Reader()
reader.offset = 100
do match rows[0]
    | .low(low)
        -- reader.total(low)
    ??
        -- -1
-- reader.pick(rows)
-- free-total(rows)

━━━━━━━━━━━━━━━━━━━━
+ do free-total I64
    - rows Table'Node

do match? rows[0]
    | .low(low)
        = sum(low)
= -1

━━━━━━━━━━━━━━━━━━━━
+ sum I64
    - base Low

base.value + base.pair.left

━━━━━━━━━━━━━━━━━━━━
+ Reader

+ offset I64

────────────────────
+ total I64
    - base Low

offset + base.value + base.pair.right

────────────────────
+ do pick I64
    - rows Table'Node

do match? rows[1]
    | .high(high)
        = total-high(high)
= -1

────────────────────
_ total-high I64
    - base High

offset + base.value + base.pair.left

━━━━━━━━━━━━━━━━━━━━
+ Pair is Tuple

+ left I64
+ right I64

━━━━━━━━━━━━━━━━━━━━
+ Base is Tuple

+ value I64
+ pair Pair

━━━━━━━━━━━━━━━━━━━━
+ Low is Base

━━━━━━━━━━━━━━━━━━━━
+ High is Base

━━━━━━━━━━━━━━━━━━━━
+ Node is Union

| low Low
| high High
```

```output
112
133
11
```

### Cases with the same field shape stay distinct when a table row is matched, and positional case fields read from the row.

```yumi
/core/arena
/core/table

. arena = Arena()
do finally arena.release()
. build = Table'Shape(arena, 4)
build.append(Shape.wide(1, 7))
build.append(Shape.tall(2, 9))
build.append(Shape.flat(3, 1))
for index in 0 .. < build.length
    -- describe(build, index)
. shapes = build.finish()
for index in 0 .. < shapes.length
    -- name(shapes, index)
-- pair(shapes)

━━━━━━━━━━━━━━━━━━━━
_ do describe String
    - shapes Table'Shape
    - index Index

do match shapes[index]
    | .tall(_, size)
        = "tall $size"
    | .wide(_, size)
        = "wide $size"
    | .flat(id, _)
        = "flat $id"

━━━━━━━━━━━━━━━━━━━━
_ name String
    - shapes ImmutableTable'Shape
    - index Index

match shapes[index .. < index + 1]
    | [.tall(id, _)]
        "tall $id"
    | [.wide(id, _)]
        "wide $id"
    ??
        "other"

━━━━━━━━━━━━━━━━━━━━
_ pair String
    - shapes ImmutableTable'Shape

match shapes[0 .. < shapes.length]
    | [.tall(_, first), .wide(_, second), ...]
        "tall $first then wide $second"
    | [.wide(_, first), .tall(_, second), ...]
        "wide $first then tall $second"
    ??
        "other"

━━━━━━━━━━━━━━━━━━━━
+ Shape is Union

| wide(id U32, size U32)
| tall(id U32, size U32)
| flat(id U32, depth U8)
```

```output
wide 7
tall 9
flat 3
wide 1
tall 2
other
wide 7 then tall 9
```

### A view reads the rows appended so far without sealing the table, and stays readable after the table grows.

```yumi
/core/arena
/core/table

. arena = Arena()
do finally arena.release()
. table = Table'Entry(arena, 2)
table.append(Entry(1, 10))
table.append(Entry(2, 20))
. early = table.view()
for count in 0 .. < 40
    table.append(Entry(3 + (count as U32), 30 + (count as U32)))
. late = table.view()
-- "${early.length} ${late.length} ${table.length}"
-- "${early[1].key} ${early[1].value}"
-- "${late[1].key} ${late[41].key} ${late[41].value}"
table.append(Entry(99, 990))
-- "${late.length} ${table.view().length} ${table.view()[42].value}"

━━━━━━━━━━━━━━━━━━━━
+ Entry is Struct

+ key U32
+ value U32
```

```output
2 42 42
2 20
2 42 69
42 43 990
```

### An immutable row reports its position, whether it was indexed, iterated, or bound by a pattern.

```yumi
/core/arena
/core/table

. arena = Arena()
do finally arena.release()
. build = Table'Part(arena, 4)
build.append(Part.leaf(1, 7))
build.append(Part.leaf(2, 9))
build.append(Part.pair(3, 0, 2))
. parts = build.finish()
-- "${parts[0].index} ${parts[2].index}"
for part in parts[1 .. < 3]
    -- part.index
-- second(parts, parts[2])

━━━━━━━━━━━━━━━━━━━━
_ second String
    - parts ImmutableTable'Part
    - part ImmutableTableRow'Part

match part
    | .pair(_, start, count)
        match parts[start .. < start + count]
            | [_, held, ...]
                "second child is row ${held.index}"
            ??
                "short"
    ??
        "leaf"

━━━━━━━━━━━━━━━━━━━━
+ Part is Union

| leaf(id U32, value U32)
| pair(id U32, start U32, count U32)
```

```output
0 2
1
2
second child is row 1
```

### A row taken from a loop can be passed, stored, assigned, or returned where an optional row is expected.

```yumi
/core/arena
/core/table

. arena = Arena()
do finally arena.release()
. build = Table'Entry(arena, 4)
build.append(Entry(1, 2))
build.append(Entry(3, 4))
build.append(Entry(5, 6))
. entries = build.finish()
for entry in entries[0 .. < 2]
    show(entry)
. kept [ImmutableTableRow'Entry?]
for entry in entries
    kept += entry
for entry in kept
    show(entry)
show(last-below(entries, 4))
show(last-below(entries, 0))
show(first-above(entries, 3))
show(first-above(entries, 9))

━━━━━━━━━━━━━━━━━━━━
_ do show
    - entry ImmutableTableRow'Entry?

if entry
    -- "${entry.key} ${entry.value}"
else
    -- "none"

━━━━━━━━━━━━━━━━━━━━
_ do last-below ImmutableTableRow'Entry?
    - entries ImmutableTable'Entry
    - limit U32

. found ImmutableTableRow'Entry? = null
for entry in entries
    if entry.key < limit
        found = entry
= found

━━━━━━━━━━━━━━━━━━━━
_ do first-above ImmutableTableRow'Entry?
    - entries ImmutableTable'Entry
    - limit U32

for entry in entries
    =? entry if entry.key > limit
= null

━━━━━━━━━━━━━━━━━━━━
+ Entry is Struct

+ key U32
+ value U32
```

```output
1 2
3 4
1 2
3 4
5 6
3 4
none
5 6
none
```

```yumi
/core/table

. rows = Table'Point(1) • with Point((each + 7) as I32, (each + 9) as I32)
for row in rows
    -- sum-row(row, true)
    -- sum-row(row, false)

━━━━━━━━━━━━━━━━━━━━
+ do sum-row I32
    - row TableRow'Point
    - use-x Bool

. first = if use-x then row.x else row.y
= first + row.y

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
16
18
```

```yumi
/core/arena
/core/table
/core/array-builder

. arena = Arena(8192)
do finally arena.release()
. rows = Table'Point(arena)
. values = ArrayBuilder'U64(arena)
rows.append(Point(7u64, 9u64))
values.append(11u64)
fail-rows(rows, Size.max)
fail-rows(rows, 1152921504606846976 as Size)
fail-values(values, Size.max)
fail-values(values, 1152921504606846976 as Size)
rows.append(Point(13u64, 15u64))
values.append(17u64)
-- rows[0] as Point
-- rows[1] as Point
-- values[0], values[1]
-- rows.length, values.length

━━━━━━━━━━━━━━━━━━━━
+ do fail-rows
    - rows Table'Point
    - needed Size

. attempt = do new
    rows.reserve(needed)
catch failure
    -- "${failure.message}: ${failure.source?.line}:${failure.source?.column}"
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ do fail-values
    - values ArrayBuilder'U64'ArenaBacking
    - needed Size

. attempt = do new
    values.reserve(needed)
catch failure
    -- "${failure.message}: ${failure.source?.line}:${failure.source?.column}"
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x U64
+ y U64
```

```output
Table x capacity overflow: 29:5
Arena allocation failed while growing Table: 29:5
ArrayBuilder capacity overflow: 40:5
Arena allocation failed while growing ArrayBuilder: 40:5
(7, 9)
(13, 15)
(11, 17)
(2, 2)
```

```yumi crash
/core/arena
/core/table
/core/array-builder

. arena = Arena(8192)
do finally arena.release()
. rows = Table'Point(arena)
. values = ArrayBuilder'U64(arena)
rows.append(Point(7u64, 9u64))
values.append(11u64)
fail-rows(rows, Size.max)
fail-rows(rows, 1152921504606846976 as Size)
fail-values(values, Size.max)
fail-values(values, 1152921504606846976 as Size)
rows.append(Point(13u64, 15u64))
values.append(17u64)
-- rows[0] as Point
-- rows[1] as Point
-- values[0], values[1]
-- rows.length, values.length

━━━━━━━━━━━━━━━━━━━━
+ do fail-rows
    - rows Table'Point
    - needed Size

rows.reserve(needed)

━━━━━━━━━━━━━━━━━━━━
+ do fail-values
    - values ArrayBuilder'U64'ArenaBacking
    - needed Size

values.reserve(needed)

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x U64
+ y U64
```

```stderr
Unhandled panic: Table x capacity overflow
<no file:28>

Task 1

Stack trace:
  0  __source__:fail-rows no file:28:1
  1  __source__:main no file:12:1
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

```yumi
/BxGC/BxGC.h
/core/table

. rows = Table'Node(1) • with :: index => Node.wide(1, 10, 20, 30, 40)

match rows[0]
    | .small(small)
        -- small.value
    | .wide(wide)
        -- collect-table-case(wide)

━━━━━━━━━━━━━━━━━━━━
+ do collect-table-case U64
    - wide Wide

C:brixGCCollect()
= wide.external-a

━━━━━━━━━━━━━━━━━━━━
+ Syntax is Tuple

+ source U64

━━━━━━━━━━━━━━━━━━━━
+ Small is Syntax

+ value U64

━━━━━━━━━━━━━━━━━━━━
+ Wide is Syntax

+ @hot inline-a U64
+ @hot inline-b U64
+ external-a U64
+ external-b U64

━━━━━━━━━━━━━━━━━━━━
+ Node is Union

| small Small
| wide Wide
```

```output
30
```

```yumi crash
/core/table

. rows = Table'Node(2)
rows[0] = Node.small(10, 2)
do match rows[0]
    | .small(small)
        -- small.pick(9)
    | .wide
        -- "no"

━━━━━━━━━━━━━━━━━━━━
+ Syntax is Tuple

+ source U32

━━━━━━━━━━━━━━━━━━━━
+ Small is Syntax

+ value U32

━━━━━━━━━━━━━━━━━━━━
+ Wide is Syntax

+ target U32

━━━━━━━━━━━━━━━━━━━━
+ Node is Union

| small Small
| wide Wide

━━━━━━━━━━━━━━━━━━━━
+ extend Small

────────────────────
+ do pick String
    - at U32

. part = "${this.value}"
?! Panic("too far $at")
    if at > 5
= "$part ${this.source}"
```

```stderr
Unhandled panic: too far 9
<no file:41>

Task 1

Stack trace:
  0  __source__:pick no file:41:1
  1  __source__:pick-table-row-receiver-bridge no file:37:1
  2  __source__:main no file:8:12
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

```yumi
/core/table

. rows = Table'Node(2)
rows[0] = Node.small(10, 2)
rows[1] = Node.wide(20, 3)
-- rows[0] is .small
-- rows[1] is .small
. held TableRow'Node? = rows[1]
-- held is .wide
if held
    -- held is .wide

━━━━━━━━━━━━━━━━━━━━
+ Syntax is Tuple

+ source U32

━━━━━━━━━━━━━━━━━━━━
+ Small is Syntax

+ value U32

━━━━━━━━━━━━━━━━━━━━
+ Wide is Syntax

+ target U32

━━━━━━━━━━━━━━━━━━━━
+ Node is Union

| small Small
| wide Wide
```

```output
true
false
false
true
```

```yumi
/core/table

. rows = Table'Node(2)
rows[0] = Node.small(10, 2)
rows[1] = Node.wide(20, 3)
for row in rows
    do match? row
        | .small(small)
            -- small.pick(rows)

━━━━━━━━━━━━━━━━━━━━
+ do overwrite
    - rows Table'Node

rows[0] = Node.small(99, 77)

━━━━━━━━━━━━━━━━━━━━
+ extend Small

────────────────────
+ do pick String
    - rows Table'Node

. first = this.value
overwrite(rows)
= "$first ${this.value}"

━━━━━━━━━━━━━━━━━━━━
+ Syntax is Tuple

+ source U32

━━━━━━━━━━━━━━━━━━━━
+ Small is Syntax

+ value U32

━━━━━━━━━━━━━━━━━━━━
+ Wide is Syntax

+ target U32

━━━━━━━━━━━━━━━━━━━━
+ Node is Union

| small Small
| wide Wide
```

```output
2 2
```

```yumi crash
/core/table
/core/arena

. arena = Arena(8192)
do finally arena.release()
. rows = Table'Node(arena, 1)
rows.append(Node.small(10, 2))
. row = rows[0]
arena.reset()
-- row.cheap

━━━━━━━━━━━━━━━━━━━━
+ extend Syntax

────────────────────
+ get cheap U32

7

━━━━━━━━━━━━━━━━━━━━
+ Syntax is Tuple

+ source U32

━━━━━━━━━━━━━━━━━━━━
+ Small is Syntax

+ value U32

━━━━━━━━━━━━━━━━━━━━
+ Wide is Syntax

+ target U32

━━━━━━━━━━━━━━━━━━━━
+ Node is Union

| small Small
| wide Wide
```

```stderr
Unhandled panic: TableRow is stale after Arena reset
<no file:11>

Task 1

Stack trace:
  0  __source__:union-dispatch-1 no file:11:4
  1  __source__:main no file:11:4
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

```yumi crash
/core/table

. rows = Table'Node(2)
rows[0] = Node.small(10, 2)
rows[1] = Node.wide(20, 3)
-- rows[0].check(1)
rows[0].bump(3)
-- rows[0] as Node
-- rows[1].check(9)

━━━━━━━━━━━━━━━━━━━━
+ extend Small

────────────────────
+ do bump
    - by I32

value += by

────────────────────
+ do check String
    - limit I32

= "small $value"

━━━━━━━━━━━━━━━━━━━━
+ extend Wide

────────────────────
+ do bump
    - by I32

target += by

────────────────────
+ do check String
    - limit I32

?! Panic("over $limit")
    if limit > 5
= "wide $target"

━━━━━━━━━━━━━━━━━━━━
+ Syntax is Tuple

+ source U32

━━━━━━━━━━━━━━━━━━━━
+ Small is Syntax

+ value I32

━━━━━━━━━━━━━━━━━━━━
+ Wide is Syntax

+ target I32

━━━━━━━━━━━━━━━━━━━━
+ Node is Union

| small Small
| wide Wide
```

```stderr
Unhandled panic: over 9
<no file:40>

Task 1

Stack trace:
  0  __source__:check no file:40:1
  1  __source__:check-table-row-receiver-bridge no file:37:1
  2  __source__:union-dispatch-1 no file:7:4
  3  __source__:main no file:10:4
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

```yumi crash
/core/table

. rows = Table'Node(2)
rows[0] = Node.small(10, 2)
do match? rows[0]
    | .small(small)
        -- small.pick(9)

━━━━━━━━━━━━━━━━━━━━
+ extend Small

────────────────────
+ do pick String
    - at U32

. first = this.value
. made = Small(1, 2)
?! Panic("p $at ${made.value}")
    if at > 5
= "$first ${this.source}"

━━━━━━━━━━━━━━━━━━━━
+ Syntax is Tuple

+ source U32

━━━━━━━━━━━━━━━━━━━━
+ Small is Syntax

+ value U32

━━━━━━━━━━━━━━━━━━━━
+ Wide is Syntax

+ target U32

━━━━━━━━━━━━━━━━━━━━
+ Node is Union

| small Small
| wide Wide
```

```stderr
Unhandled panic: p 9 2
<no file:19>

Task 1

Stack trace:
  0  __source__:pick no file:19:1
  1  __source__:pick-table-row-receiver-bridge no file:14:1
  2  __source__:main no file:8:12
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

```yumi crash
/core/table

. filled = Table'Point(3) • with :: index => do
    ?! Panic("bad $index")
        if index == 2
    = Point(index as I32, 0)
-- filled.length

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```stderr
Unhandled panic: bad 2
<no file:5>

Task 1

Stack trace:
  0  __source__:filled no file:5:5
  1  __source__:filledCasting no file:4:26
  2  core/table:Table:Table core/table.yumi
  3  __source__:main no file:4:12
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

```yumi
/core/table

store.rows[0] = Node.small(10, 1)
store.rows[1] = Node.wide(20, 2)
-- store.rows[0].twice
store.rows[0] = Node.small(30, 3)
do match? store.rows[0]
    | .small(small)
        -- small.again()

+ store = Store()

━━━━━━━━━━━━━━━━━━━━
+ Store

+ rows = Table'Node(2)

━━━━━━━━━━━━━━━━━━━━
+ do mutate U32

store.rows[0] = Node.small(99, 9)
= 0

━━━━━━━━━━━━━━━━━━━━
+ extend Syntax

────────────────────
+ get twice String

"${this.source} ${mutate()} ${this.source}"

────────────────────
+ do again String

= "${this.source} ${mutate()} ${this.source}"

━━━━━━━━━━━━━━━━━━━━
+ Syntax is Tuple

+ source U32

━━━━━━━━━━━━━━━━━━━━
+ Small is Syntax

+ value U64

━━━━━━━━━━━━━━━━━━━━
+ Wide is Syntax

+ target U64

━━━━━━━━━━━━━━━━━━━━
+ Node is Union

| small Small
| wide Wide
```

```output
10 0 99
30 0 99
```

### A table an arena holds may have union rows whose cases hold managed fields, and the `ImmutableTable` it finishes as is made over its storage, which is not laid out again.

```yumi
/core/arena
/core/table


. arena = use Arena()
. table = Table'Value(arena)
table.append(Plain(4))
table.append(Named("five"))
. rows = table.finish()
-- "rows ${rows.length}"

━━━━━━━━━━━━━━━━━━━━
+ Named is Tuple

+ text String

━━━━━━━━━━━━━━━━━━━━
+ Plain is Tuple

+ value I32

━━━━━━━━━━━━━━━━━━━━
+ Value is Union

| named Named
| plain Plain
```

```output
rows 2
```

### An optional field of the case of a row a match finds, once a test finds it holds a value, is read as what it holds.

```yumi
/core/arena
/core/table


. arena = use Arena()
. table = Table'Shape(arena)
table.append(Square("box"))
table.append(Square(null))
table.append(Dot(1))
. rows = table.finish()
-- "${is-box(rows, 0)} ${is-box(rows, 1)} ${is-box(rows, 2)}"

━━━━━━━━━━━━━━━━━━━━
+ is-box Bool
    - rows ImmutableTable'Shape
    - index U32

match rows[index]
    | .square(row)
        |? !row.name
            false
        ??
            row.name == "box"
    ??
        false

━━━━━━━━━━━━━━━━━━━━
+ Square is Tuple

+ name String?

━━━━━━━━━━━━━━━━━━━━
+ Dot is Tuple

+ size U32

━━━━━━━━━━━━━━━━━━━━
+ Shape is Union

| square Square
| dot Dot
```

```output
true false false
```

### A field of a row named as a private member of the table's row is read from the row, as no module but the table's may use that member.

```yumi
/core/arena
/core/table


. arena = use Arena()
. table = Table'Grid(arena)
table.append(Grid(3, 4))
table.append(Grid(5, 6))
-- "${table[0].columns} ${table[1].columns} ${table[1].rows}"

━━━━━━━━━━━━━━━━━━━━
+ Grid is Struct

+ columns U32
+ rows U32
```

```output
3 5 6
```

### An assignment to a field of a table's row gives the value stored, so it may be an arm's value in a match or a choice.

```yumi
/core/table


. table = Table'Flags(1)
set-flag(table, .flag-a)
set-flag(table, .flag-c)
-- "${table[0].a} ${table[0].b} ${table[0].c}"
-- "${pick(table, true)} ${pick(table, false)} ${table[0].count}"

━━━━━━━━━━━━━━━━━━━━
+ Flags is Struct

+ a Bool
+ b Bool
+ c Bool
+ count U32

━━━━━━━━━━━━━━━━━━━━
+ Flag is U32

| flag-a = 0
| flag-b = 1
| flag-c = 2

━━━━━━━━━━━━━━━━━━━━
+ do set-flag
    - table Table'Flags
    - flag Flag

match flag
    |= .flag-a
        table[0].a = true
    |= .flag-b
        table[0].b = true
    ??
        table[0].c = true

━━━━━━━━━━━━━━━━━━━━
+ pick U32
    - table Table'Flags
    - wide Bool

|? wide
    table[0].count = 40
??
    table[0].count = 2
```

```output
true false true
40 2 2
```

### A table a property holds is finished as a local one is, and is empty after it.

```yumi
/core/table
/core/arena

. arena = Arena(8192)
do finally arena.release()
. keeper = Keeper(arena)
keeper.rows.append(Point(1, 2))
keeper.rows.append(Point(3, 4))
. fixed = keeper.rows.finish()
-- "${fixed.length} ${keeper.rows.length}"
keeper.add(5)
. again = keeper.seal()
-- "${again.length} ${keeper.rows.length} ${again[0].x}"

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int

━━━━━━━━━━━━━━━━━━━━
+ Keeper

+ rows Table'Point

────────────────────
+ This
    - arena Arena

.. rows = Table'Point(arena, 1)

────────────────────
+ do add
    - x Int

rows = Table'Point(Arena(1024), 1)
rows.append(Point(x, x))

────────────────────
+ do seal ImmutableTable'Point

= rows.finish()
```

```output
2 0
1 0 5
```

### A loop over a range of a table's rows reads the rows the range named while the loop appends more.

```yumi
/core/arena
/core/table

. arena = Arena()
. cases = Table'Case(arena, 4)
cases.append(Case(1, 2))
cases.append(Case(3, 4))
cases.append(Case(5, 6))
. total = 0
for fact in cases[1 .. < 3]
    total += fact.a
    cases.append(Case(7, 8))
-- "direct $total ${cases.length}"

━━━━━━━━━━━━━━━━━━━━
+ Case is Struct

+ a Int
+ b Int
```

```output
direct 8 5
```

### An optional field of a table's row is tested and read as a local's is.

```yumi
/core/arena
/core/atom-pool
/core/table

. pool = AtomPool()
. arena = Arena()
. cases = Table'Case(arena, 4)
cases.append(Case(pool.store("first"), 1))
cases.append(Case(null, 2))
for offset in 0 .. < cases.length
    . fact = cases[offset]
    if fact.name && fact.name! == pool.store("first")
        -- "match ${fact.value}"
    . name = if fact.name then fact.name!.text else "-"
    -- name

━━━━━━━━━━━━━━━━━━━━
+ Case is Struct

+ name Atom?
+ value Int
```

```output
match 1
first
-
```

### A method called on a field of a union case of a table row is called on that field.

```yumi
/core/arena
/core/atom-pool
/core/table

-- check()

━━━━━━━━━━━━━━━━━━━━
+ Base is Tuple

+ id U32

━━━━━━━━━━━━━━━━━━━━
+ Word is Base

+ text String

━━━━━━━━━━━━━━━━━━━━
+ Group is Base

+ count U32

━━━━━━━━━━━━━━━━━━━━
+ Node is Union

| atom Word
| list Group

━━━━━━━━━━━━━━━━━━━━
_ do check String

. arena = Arena()
. names = AtomPool()
. table = Table'Node(arena, 16)
table.append(Node.atom(1, names.store("@x").text))
table.append(Node.list(2, 3))
table.append(Node.atom(3, names.store("y").text))
. nodes = table.view()
. shown [String]
for position in 0 .. < 3
    shown += "${is-attribute(nodes[position])}"
= " ".join(shown)

━━━━━━━━━━━━━━━━━━━━
_ is-attribute Bool
    - node ImmutableTableRow'Node

match node
    | .atom(_, text)
        text.starts-with("@")
    ??
        false
```

```output
true false false
```

### A getter of a union case that a row of a table holds is the case's own extension's, over its base's.

```yumi
/core/table

. rows = Table'Piece(3)
rows[0] = Piece.plain(1)
rows[1] = Piece.stop(2)
for position in 0 .. < 2
    print(rows[position].label)
do match rows[1]
    | .stop(stop)
        print(stop.label)
    | .plain
        print("no")

━━━━━━━━━━━━━━━━━━━━
+ Empty is Tuple

+ body U32

━━━━━━━━━━━━━━━━━━━━
+ Stop is Empty

━━━━━━━━━━━━━━━━━━━━
+ Piece is Union

| plain Empty
| stop Stop

━━━━━━━━━━━━━━━━━━━━
+ extend Empty

────────────────────
+ get label String

"empty $body"

━━━━━━━━━━━━━━━━━━━━
+ extend Stop

────────────────────
+ get label String

"stop $body"
```

```output
empty 1
stop 2
stop 2
```

### A member with a loop that may move a row's storage is run through the case gathered from the row.

```yumi crash
/core/table

. rows = Table'Node(2)
rows[0] = Node.small(10, 2)
do match rows[0]
    | .small(small)
        print(small.pick(9))
    | .wide
        print("no")

━━━━━━━━━━━━━━━━━━━━
+ Syntax is Tuple

+ source U32

━━━━━━━━━━━━━━━━━━━━
+ Small is Syntax

+ value U32

━━━━━━━━━━━━━━━━━━━━
+ Wide is Syntax

+ target U32

━━━━━━━━━━━━━━━━━━━━
+ Node is Union

| small Small
| wide Wide

━━━━━━━━━━━━━━━━━━━━
+ extend Small

────────────────────
+ do pick String
    - at U32

. parts [String]
for index in 0 .. < at
    parts += "${this.value + index}"
?! Panic("too far $at")
    if at > 5
= "${parts.length} ${this.source}"
```

```stderr
Unhandled panic: too far 9
<no file:43>

Task 1

Stack trace:
  0  __source__:pick no file:43:1
  1  __source__:pick-table-row-receiver-bridge no file:37:1
  2  __source__:main no file:8:15
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### A loop over a table whose body grows it reads and writes the table's storage as it is after the growth, and visits only the rows the table had when the loop began.

```yumi
/core/table
/core/arena

. arena = Arena(16384)
do finally arena.release()
. rows = Table'Point(arena, 2)
rows.append(Point(1, 10))
rows.append(Point(2, 20))
. turns = 0
for row in rows
    turns += 1
    rows.append(Point(9, 90))
    rows.reserve(64 * turns)
    row.x += 100
-- "turns $turns length ${rows.length}"
for row in rows
    -- row as Point

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
turns 2 length 4
(101, 10)
(102, 20)
(9, 90)
(9, 90)
```

### A column of a table row read in one branch is read again in the other branch, not reused.

```yumi
/core/table

. rows = Table'Point(1) • with Point((each + 7) as I32, (each + 9) as I32)
for row in rows
    -- sum-row(row, true)
    -- sum-row(row, false)

━━━━━━━━━━━━━━━━━━━━
+ do sum-row I32
    - row TableRow'Point
    - use-x Bool

. first = if use-x then row.x else row.y
= first + row.y

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
16
18
```

### An unsuffixed number in one branch takes the type of a union table row's common field in the other.

```yumi
/core/table
/core/arena

. arena = Arena(8192)
do finally arena.release()
. rows = Table'Node(arena)
rows.append(Node.word(Word(5, 7)))
. position = 1
. above U32 = if position > 0 then rows[0].id else 0
-- above

━━━━━━━━━━━━━━━━━━━━
+ Base is Tuple

+ id U32

━━━━━━━━━━━━━━━━━━━━
+ Word is Base

+ text U32

━━━━━━━━━━━━━━━━━━━━
+ Group is Base

+ count U32

━━━━━━━━━━━━━━━━━━━━
+ Node is Union

| word Word
| group Group
```

```output
5
```

### A row of a table passed to a function beside a call that takes the same row is passed whole.

```yumi
/core/arena
/core/table

. arena = Arena()
do finally arena.release()
. rows = Table'Point(arena, 2)
rows.append(Point(1))
rows.append(Point(2))
show(rows.view()[0 .. < 2])

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x U32

━━━━━━━━━━━━━━━━━━━━
+ do show
    - span ImmutableTableSpan'Point

named(span[1], same(span[1], span))

━━━━━━━━━━━━━━━━━━━━
+ same ImmutableTableSpan'Point
    - point ImmutableTableRow'Point
    - span ImmutableTableSpan'Point

span

━━━━━━━━━━━━━━━━━━━━
+ do named
    - point ImmutableTableRow'Point
    - span ImmutableTableSpan'Point

-- "${point.x}:${span.length}"
```

```output
2:2
```

### A comprehension over a span of a table yields the rows of the span.

```yumi
/core/table

. table = Table'LoopRow(3) • with LoopRow((each + 5) as U32, 2)
-- spanned(table).length

━━━━━━━━━━━━━━━━━━━━
+ LoopRow is Struct

+ value U32
+ count U32

━━━━━━━━━━━━━━━━━━━━
+ spanned [U32]
    - table Table'LoopRow

each.value in table[1 .. < 3]
```

```output
2
```

### A comprehension over a table finds its rows where its loop begins, not where it is written.

```yumi
/core/table
/core/arena

. arena = Arena(8192)
do finally arena.release()
. rows = Table'Point(arena, 2)
rows.append(Point(1, 10))
rows.append(Point(2, 20))
. xs = each.x in rows
. ys = each.y in rows[1 .. < 2]
rows.append(Point(3, 30))
rows.reserve(64)
-- List(xs)
-- List(ys)

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
```

```output
1, 2, 3
20
```
