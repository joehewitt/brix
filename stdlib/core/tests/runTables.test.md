# Tables

Table rows

These tests specify the v1 fixed-size struct-of-arrays Table'T / TableRow'T /
TableSpan'T / TableField'T behavior from JOE-52.

### Table(size) zero-initializes stored fields and exposes row/span/column view types.

```brix
(import /core/table)
(blank)

(main
  (let table (call (of Table Particle) 3))
  (print (. table size))
  (print (. table length))
  (print (. ([] table 0) id))
  (print (. ([] table 1) mass))
  (print (. ([] table 2) alive))
  (blank)
  (= (. ([] table 1) id) 7)
  (= (. ([] table 1) mass) 2.5)
  (= (. ([] table 1) alive) true)
  (print (as ([] table 1) Particle))
  (blank)
  (print (type-of ([] table 1)))
  (print (type-of ([] table (..< 0 2))))
  (print (type-of (. table id)))
)

(type + Particle (is Struct)
  (prop + id I32)
  (prop + mass Float)
  (prop + alive Bool)
)
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

```brix
(import /core/table)
(blank)

(main
  (let table (call (of Table LoopRow) 3 (:with (call LoopRow (as (+ each 5) U32) 2))))
  (let store (call LoopTableStore table))
  (blank)
  (for (in offset (..< 0 2u32))
    (let row ([] (. store rows) (as (+ 1u32 offset) Index)))
    (print (. row value))
  )
  (blank)
  (let cursor ([] (. store rows) 0))
  (for (in offset (..< 0 (. cursor count)))
    (let row ([] (. store rows) (as (- (+ (. cursor value) offset) 4) Index)))
    (print (. row value))
  )
)

(type + LoopRow (is Struct)
  (prop + value U32)
  (prop + count U32)
)

(type + LoopTableStore
  (prop + rows (of Table LoopRow))
  (fn-do + This _
    (param rows (of Table LoopRow))
  )
)
```

```output
6
7
6
7
```

```brix
(import /core/table)
(blank)
(import /core/arena)
(blank)

(main
  (let arena (call Arena 16384))
  (finally
    (inline (call (. arena release)))
  )
  (let rows (call (of Table Node) arena 2))
  (call (. rows append) (call (. Node wide) 1 10 20 30 40))
  (call (. rows append) (call (. Node wide) 2 50 60 70 80))
  (let take-external false)
  (blank)
  (match ([] rows 0)
    (case (call .small small)
      (call show-scalar-spill (. small value))
    )
    (case (call .wide wide)
      (call show-scalar-spill (+ (. wide external-a) (. wide external-b)))
      (+= (. wide external-a) 5)
      (call show-scalar-spill (+ (. wide external-a) (. wide external-b)))
      (call show-scalar-spill (+ (. wide inline-a) (. wide inline-b)))
      (if take-external
        (call show-scalar-spill (. wide external-a))
      )
    )
  )
  (blank)
  (match ([] rows 0)
    (case (call .small small)
      (call show-scalar-spill (. small value))
    )
    (case (call .wide wide)
      (let first (. wide external-a))
      (call (. rows reserve) 32)
      (call show-scalar-spill (+ first (. wide external-b)))
    )
  )
  (blank)
  (match ([] rows 0)
    (case (call .small small)
      (call show-scalar-spill (. small value))
    )
    (case (call .wide wide)
      (call show-scalar-spill (. wide external-a))
    )
  )
  (blank)
  (match ([] rows 1)
    (case (call .small small)
      (call show-scalar-spill (. small value))
    )
    (case (call .wide wide)
      (call show-scalar-spill (. wide external-b))
    )
  )
)

(fn-do + show-scalar-spill _
  (param value U64)
  (print value)
)

(type + Syntax (is Tuple)
  (prop + source U64)
)

(type + Small (is Syntax)
  (prop + value U64)
)

(type + Wide (is Syntax)
  (prop + inline-a U64 @hot)
  (prop + inline-b U64 @hot)
  (prop + external-a U64)
  (prop + external-b U64)
)

(type + Node (is Union)
  (variant small Small)
  (variant wide Wide)
)
```

```output
70
75
30
75
35
80
```

```brix
(import /core/table)
(blank)
(import /core/arena)
(blank)

(main
  (let arena (call Arena 8192))
  (finally
    (inline (call (. arena release)))
  )
  (let rows (call (of Table Node) arena 1))
  (call (. rows append) (call (. Node wide) 1 10 20 30 40))
  (blank)
  (match ([] rows 0)
    (case (call .small small)
      (call show-table-case-call (. small value))
    )
    (case (call .wide wide)
      (call show-table-case-call (call pair-table-cases wide wide))
      (call show-table-case-call (call unsafe-table-case wide rows))
    )
  )
)

(fn-do + show-table-case-call _
  (param value U64)
  (print value)
)

(fn-expr + pair-table-cases U64
  (param left Wide)
  (param right Wide)
  (+ (. left external-a) (. right external-b))
)

(fn-expr + unsafe-table-case U64
  (param wide Wide)
  (param rows (of Table Node))
  (before
    (do
      (call mutate-table-case-source rows)
    )
  )
  (. wide external-a)
)

(fn-do + mutate-table-case-source _
  (param rows (of Table Node))
  (= ([] rows 0) (call (. Node wide) 2 100 200 300 400))
)

(type + Syntax (is Tuple)
  (prop + source U64)
)

(type + Small (is Syntax)
  (prop + value U64)
)

(type + Wide (is Syntax)
  (prop + inline-a U64 @hot)
  (prop + inline-b U64 @hot)
  (prop + external-a U64)
  (prop + external-b U64)
)

(type + Node (is Union)
  (variant small Small)
  (variant wide Wide)
)
```

```output
70
30
```

```brix
(import /core/table)
(blank)
(import /core/arena)
(blank)

(main
  (let arena (call Arena 4096))
  (finally
    (inline (call (. arena release)))
  )
  (let rows (call (of Table Node) arena 1))
  (call (. rows append) (call (. Node wide) 10 1 2 200 300))
  (print (call read-wide ([] rows 0) false))
)

(type + Syntax (is Tuple)
  (prop + source U32)
)

(type + Wide (is Syntax)
  (prop + inline-a U64 @hot)
  (prop + inline-b U64 @hot)
  (prop + external-a U64)
  (prop + external-b U64)
)

(type + Small (is Syntax)
  (prop + value U8)
)

(type + Node (is Union)
  (variant small Small)
  (variant wide Wide)
)

(fn-do + read-wide U64
  (param row (of TableRow Node))
  (param include-first Bool)
  (return
    (match row
      (case .small
        0u64
      )
      (case (call .wide wide)
        (let first 0u64)
        (if include-first
          (= first (. wide external-a))
        )
        (+ first (. wide external-b))
      )
    )
  )
)
```

```output
300
```

```brix compile-error
(import /core/table)
(blank)
(import /core/arena)
(blank)

(main
  (let arena (call Arena 8192))
  (finally
    (inline (call (. arena release)))
  )
  (let rows (call (of Table Point) arena 1))
  (call (. rows append) (call Point 1 2))
  (let fixed (call (. rows finish)))
  (= ([] ([] fixed (..< 0 1)) 0) (call Point 3 4))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
Exception: ImmutableTableSpan is read-only
<no file:15>
      (= ([] ([] fixed (..< 0 1)) 0) (call Point 3 4))
         ^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix compile-error
(import /core/table)
(blank)
(import /core/arena)
(blank)

(main
  (let arena (call Arena 8192))
  (finally
    (inline (call (. arena release)))
  )
  (let rows (call (of Table Point) arena 1))
  (call (. rows append) (call Point 1 2))
  (let fixed (call (. rows finish)))
  (= ([] (. fixed x) 0) 3)
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
Exception: ImmutableTableField is read-only
<no file:15>
      (= ([] (. fixed x) 0) 3)
         ^^^^^^^^^^^^^^^^^^
```

```brix compile-error
(import /core/table)
(blank)

(main
  (let fixed (call (of ImmutableTable Point)))
  (print (. fixed length))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
Exception: No constructor of ImmutableTable matches these arguments; its constructors are private, declared with _, so only its own module can call them
<no file:6>
      (let fixed (call (of ImmutableTable Point)))
                           ^^^^^^^^^^^^^^
```

```brix
(import /core/table)
(blank)
(import /core/arena)
(blank)

(main
  (let arena (call Arena 8192))
  (finally
    (inline (call (. arena release)))
  )
  (let rows (call (of Table Point) arena 1))
  (call (. rows append) (call Point 10 20))
  (call (. rows append) (call Point 30 40))
  (let fixed (call (. rows finish)))
  (blank)
  (print (type-of fixed))
  (print (type-of ([] fixed 0)))
  (print (type-of ([] fixed (..< 0 1))))
  (print (type-of (. fixed x)))
  (print (str (. fixed length) "/" (. fixed size)))
  (print (as ([] fixed 0) Point))
  (print (as ([] fixed 1) Point))
  (print (call (. "," join) (. fixed x)))
  (print (as ([] ([] fixed (..< 1 2)) 0) Point))
  (blank)
  (let total 0)
  (for (in row fixed)
    (+= total (+ (. row x) (. row y)))
  )
  (print total)
  (blank)
  (let empty (call (. (call (of Table Point) arena) finish)))
  (print (str (. empty length) "/" (. empty size)))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
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

```brix
(import /core/table)
(blank)
(import /core/arena)
(import /core/array-builder)
(blank)

(main
  (let arena (call Arena 8192))
  (finally
    (inline (call (. arena release)))
  )
  (let rows (call (of Table Point) arena 2))
  (call (. rows append) (call Point 10 20))
  (call (. rows append) (call Point 30 40))
  (call (. rows append) (call Point 50 60))
  (let fixed (call (. rows finish)))
  (let ids (call (of Array U32) (list 2u32 0u32 2u32)))
  (let selected ([] fixed ids))
  (print (type-of selected))
  (print (type-of ([] selected 0)))
  (print (. selected length))
  (for (in row selected)
    (print (as row Point))
  )
  (let middle ([] selected (..< 1 3)))
  (for (in row middle)
    (print (as row Point))
  )
  (let sequence (. ids immutable))
  (print (as ([] ([] fixed sequence) 1) Point))
  (let builder (call (of ArrayBuilder U32) arena))
  (call (. builder append) 1u32)
  (call (. builder append) 0u32)
  (for (in row ([] fixed builder))
    (print (as row Point))
  )
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
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

```brix compile-error
(import /core/table)
(blank)
(import /core/arena)
(blank)

(main
  (let arena (call Arena 8192))
  (finally
    (inline (call (. arena release)))
  )
  (let rows (call (of Table Point) arena 1))
  (call (. rows append) (call Point 1 2))
  (let fixed (call (. rows finish)))
  (= (. ([] ([] fixed (call (of Array U32) (list 0u32))) 0) x) 3)
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
Exception: ImmutableTableRow is read-only
<no file:15>
      (= (. ([] ([] fixed (call (of Array U32) (list 0u32))) 0) x) 3)
         ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix
(import /core/table)
(blank)
(import /core/arena)
(blank)

(main
  (let arena (call Arena 8192))
  (finally
    (inline (call (. arena release)))
  )
  (let rows (call (of Table Point) arena 1))
  (call (. rows append) (call Point 1 2))
  (let alias rows)
  (let old-row ([] rows 0))
  (let old-span ([] rows (..< 0 1)))
  (let old-field (. rows x))
  (let fixed (call (. rows finish)))
  (blank)
  (call show-sealed-table alias)
  (call show-sealed-row old-row)
  (call show-sealed-span old-span)
  (call show-sealed-field old-field)
  (call show-sealed-finish alias)
  (print (as ([] fixed 0) Point))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)

(fn-do + show-sealed-table _
  (param table (of Table Point))
  (let attempt
    (do-new
      (call (. table append) (call Point 3 4))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)

(fn-do + show-sealed-row _
  (param row (of TableRow Point))
  (let attempt
    (do-new
      (= (. row x) 3)
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)

(fn-do + show-sealed-span _
  (param span (of TableSpan Point))
  (let attempt
    (do-new
      (= ([] span 0) (call Point 3 4))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)

(fn-do + show-sealed-field _
  (param field (of TableField I32))
  (let attempt
    (do-new
      (= ([] field 0) 3)
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)

(fn-do + show-sealed-finish _
  (param table (of Table Point))
  (let attempt
    (do-new
      (let discarded (call (. table finish)))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)
```

```output
Table is sealed
Table is sealed
Table is sealed
Table is sealed
Table is sealed
(1, 2)
```

```brix compile-error
(import /core/table)
(blank)
(import /core/arena)
(blank)

(main
  (let arena (call Arena 8192))
  (finally
    (inline (call (. arena release)))
  )
  (let rows (call (of Table Point) arena 1))
  (call (. rows append) (call Point 1 2))
  (let fixed (call (. rows finish)))
  (= ([] fixed 0) (call Point 3 4))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
Exception: ImmutableTable is read-only
<no file:15>
      (= ([] fixed 0) (call Point 3 4))
         ^^^^^^^^^^^^
```

```brix compile-error
(import /core/table)
(blank)
(import /core/arena)
(blank)

(main
  (let arena (call Arena 8192))
  (finally
    (inline (call (. arena release)))
  )
  (let rows (call (of Table Point) arena 1))
  (call (. rows append) (call Point 1 2))
  (let fixed (call (. rows finish)))
  (= (. ([] fixed 0) x) 3)
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
Exception: ImmutableTableRow is read-only
<no file:15>
      (= (. ([] fixed 0) x) 3)
         ^^^^^^^^^^^^^^^^^^
```

```brix
(import /core/table)
(blank)
(import /core/arena)
(blank)

(main
  (let arena (call Arena 8192))
  (finally
    (inline (call (. arena release)))
  )
  (let rows (call (of Table Node) arena 1))
  (call (. rows append) (call (. Node small) 10 100))
  (call (. rows append) (call (. Node wide) 20 1 2 300 400))
  (let fixed (call (. rows finish)))
  (blank)
  (print (type-of fixed))
  (print (type-of (. fixed source)))
  (print (call (. "," join) (. fixed source)))
  (print (as ([] fixed 0) Node))
  (print (as ([] ([] fixed (..< 1 2)) 0) Node))
  (blank)
  (for (in row fixed)
    (print
      (match row
        (case (call .small small)
          (. small value)
        )
        (case (call .wide wide)
          (+ (. wide inline-a) (. wide external-b))
        )
      )
    )
  )
)

(type + Syntax (is Tuple)
  (prop + source U32)
)

(type + Small (is Syntax)
  (prop + value U64 @hot)
)

(type + Wide (is Syntax)
  (prop + inline-a U64 @hot)
  (prop + inline-b U64 @hot)
  (prop + external-a U64)
  (prop + external-b U64)
)

(type + Node (is Union)
  (variant small Small)
  (variant wide Wide)
)
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

```brix
(import /core/table)
(blank)

(main
  (let rows (call (of Table Node) 4))
  (= ([] rows 0) (call (. Node low) 10 (call Pair 1 2)))
  (= ([] rows 1) (call (. Node middle) 20 (call Pair 2 3)))
  (= ([] rows 2) (call (. Node high) 30 (call Pair 3 4)))
  (= ([] rows 3) (call (. Node other) 40 (call Pair 4 5)))
  (blank)
  (print
    (match ([] rows 0)
      (case (call .low low)
        (. low value)
      )
      (case (call .middle middle)
        (. middle value)
      )
      (case (call .high high)
        (. high value)
      )
      (default
        -1
      )
    )
  )
  (blank)
  (print
    (match ([] rows 1)
      (case (call .low low)
        (. low value)
      )
      (case (all (call .middle middle) ({} (== (. middle pair right) 3)))
        (+ (. middle value) (. middle pair right))
      )
      (case (call .high high)
        (. high value)
      )
      (default
        -1
      )
    )
  )
  (blank)
  (let span ([] rows (..< 1 4)))
  (for (in row span)
    (print
      (match row
        (case (call .low low)
          (. low value)
        )
        (case (call .middle middle)
          (+ (. middle value) (. middle pair right))
        )
        (case (call .high high)
          (. high value)
        )
        (default
          -1
        )
      )
    )
  )
  (blank)
  (let selected 0)
  (match ([] rows 2)
    (case (call .low low)
      (= selected (. low value))
    )
    (case (call .middle middle)
      (= selected (. middle value))
    )
    (case (call .high high)
      (= selected (. high value))
    )
    (default
      (= selected -1)
    )
  )
  (print selected)
)

(type + Pair (is Tuple)
  (prop + left I64)
  (prop + right I64)
)

(type + Base (is Tuple)
  (prop + value I64)
  (prop + pair Pair)
)

(type + Low (is Base))

(type + Middle (is Base))

(type + High (is Base))

(type + Other (is Base))

(type + Node (is Union)
  (variant low Low)
  (variant middle Middle)
  (variant high High)
  (variant other Other)
)
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

```brix
(import /core/table)
(blank)

(main
  (let calls "")
  (let table
    (call (of Table Point) 4
      (:with
        (=> (each) :do
          (block
            (+= calls (str each))
            (return (call Point (as each I32) (as (* each each) I32)))
          )
        )
      )
    )
  )
  (blank)
  (print calls)
  (for (in i (..< 0 (. table length)))
    (print (as ([] table i) Point))
  )
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
0123
(0, 0)
(1, 1)
(2, 4)
(3, 9)
```

### A stored TableRow is a live row cursor; row as T gathers a stable value snapshot.

```brix
(import /core/table)
(blank)

(main
  (let table (call (of Table Point) 1))
  (= ([] table 0) (call Point 1 2))
  (blank)
  (let row ([] table 0))
  (let snapshot (as row Point))
  (= (. row x) 9)
  (= (. snapshot y) 8)
  (blank)
  (print (as ([] table 0) Point))
  (print (as row Point))
  (print snapshot)
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
(9, 2)
(9, 2)
(1, 8)
```

### Assigning table[i] scatters a real T into every column and copies the source value.

```brix
(import /core/table)
(blank)

(main
  (let table (call (of Table Particle) 2))
  (let value (call Particle 1 2 3))
  (= ([] table 0) value)
  (= (. value x) 99)
  (= ([] table 1) (call Particle 4 5 6))
  (blank)
  (print (as ([] table 0) Particle))
  (print (as ([] table 1) Particle))
  (print value)
)

(type + Particle (is Struct)
  (prop + x I32)
  (prop + y I32)
  (prop + z I32)
)
```

```output
(1, 2, 3)
(4, 5, 6)
(99, 2, 3)
```

### Struct-valued fields are column values; assigning the field column writes the table.

```brix
(import /core/table)
(blank)

(main
  (let table (call (of Table Entity) 1))
  (= ([] table 0) (call Entity 1 (call Point 2 3) (call Point 4 5)))
  (blank)
  (let copied-position (. ([] table 0) position))
  (= (. copied-position y) 30)
  (print (as ([] table 0) Entity))
  (print copied-position)
  (blank)
  (= ([] (. table position) 0) (call Point 7 8))
  (print (as ([] table 0) Entity))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)

(type + Entity (is Struct)
  (prop + id I32)
  (prop + position Point)
  (prop + velocity Point)
)
```

```output
(1, (2, 3), (4, 5))
(2, 30)
(1, (7, 8), (4, 5))
```

### table.field returns a writable TableField over one contiguous field buffer.

```brix
(import /core/table)
(blank)

(main
  (let table (call (of Table Point) 3 (:with (call Point (as each I32) (as (+ each 10) I32)))))
  (print (call (. ", " join) (. table x)))
  (print (call (. ", " join) (. table y)))
  (print (. table x length))
  (blank)
  (= ([] (. table x) 1) 42)
  (call (. table y fill) 7)
  (print (as ([] table 0) Point))
  (print (as ([] table 1) Point))
  (print (as ([] table 2) Point))
  (print (call (. ", " join) (. table y)))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
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

```brix
(import /core/table)
(blank)

(main
  (let table (call (of Table Point) 3 (:with (call Point (as (+ each 1) I32) (as (+ each 10) I32)))))
  (let xs (. table x data))
  (let ys (. table y data))
  (blank)
  (print ([] xs 0))
  (print ([] xs 1))
  (print ([] xs 2))
  (print ([] ys 0))
  (print ([] ys 1))
  (print ([] ys 2))
  (blank)
  (= ([] xs 1) 42)
  (= ([] ys 2) 77)
  (print (as ([] table 1) Point))
  (print (as ([] table 2) Point))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
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

```brix
(import /core/table)
(blank)

(main
  (let table (call (of Table Point) 3 (:with (call Point (as (+ each 1) I32) (as (+ each 10) I32)))))
  (let points (of TableSpan Point) (+ (. table x) (. table y)))
  (blank)
  (print (. points length))
  (print (type-of points))
  (print (as ([] points 0) Point))
  (print (as ([] points 2) Point))
  (blank)
  (= (. ([] points 1) x) 42)
  (= (. ([] points 2) y) 77)
  (print (as ([] table 1) Point))
  (print (as ([] table 2) Point))
  (blank)
  (for (in point points)
    (+= (. point y) (. point x))
  )
  (blank)
  (print (as ([] table 0) Point))
  (print (as ([] table 1) Point))
  (print (as ([] table 2) Point))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
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

```brix
(import /core/table)
(blank)

(main
  (let table (call (of Table Point) 5 (:with (call Point (as (+ each 1) I32) (as (+ each 10) I32)))))
  (let middle (of TableSpan Point) (+ ([] (. table x) (..< 1 4)) ([] (. table y) (..< 1 4))))
  (blank)
  (print (. middle length))
  (print (as ([] middle 0) Point))
  (print (as ([] middle 2) Point))
  (blank)
  (= (. ([] middle 0) x) 22)
  (= (. ([] middle 2) y) 99)
  (print (as ([] table 0) Point))
  (print (as ([] table 1) Point))
  (print (as ([] table 3) Point))
  (print (as ([] table 4) Point))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
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

```brix
(import /core/table)
(blank)

(main
  (let table (call (of Table Particle) 2 (:with (call Particle (as (+ each 1) I32) (as (+ each 10) I32) (as (+ each 100) I32)))))
  (let particles (of TableSpan Particle) (+ (. table id) (. table x) (. table y)))
  (blank)
  (print (. particles length))
  (print (as ([] particles 0) Particle))
  (print (as ([] particles 1) Particle))
  (blank)
  (= (. ([] particles 0) y) 500)
  (print (as ([] table 0) Particle))
)

(type + Particle (is Struct)
  (prop + id I32)
  (prop + x I32)
  (prop + y I32)
)
```

```output
2
(1, 10, 100)
(2, 11, 101)
(1, 10, 500)
```

### Destructuring TableSpan and Table unzips live field views.

```brix
(import /core/table)
(blank)

(main
  (let table (call (of Table Point) 4 (:with (call Point (as (+ each 1) I32) (as (+ each 10) I32)))))
  (let span ([] table (..< 1 3)))
  (let (tuple xs ys) span)
  (blank)
  (print (type-of xs))
  (print (. xs length))
  (print (call (. ", " join) xs))
  (print (call (. ", " join) ys))
  (blank)
  (= ([] xs 0) 40)
  (= ([] ys 1) 90)
  (print (as ([] table 1) Point))
  (print (as ([] table 2) Point))
  (blank)
  (let (tuple all-x all-y) table)
  (= ([] all-x 0) 70)
  (= ([] all-y 3) 120)
  (print (as ([] table 0) Point))
  (print (as ([] table 3) Point))
  (blank)
  (let (tuple (: typed-x (of TableField I32)) (: typed-y (of TableField I32))) span)
  (= ([] typed-x 0) 400)
  (= ([] typed-y 1) 900)
  (print (type-of typed-y))
  (print (as ([] table 1) Point))
  (print (as ([] table 2) Point))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
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

```brix crash
(import /core/table)
(blank)

(main
  (let table (call (of Table Point) 4 (:with (call Point (as (+ each 1) I32) (as (+ each 10) I32)))))
  (let bad (of TableSpan Point) (+ ([] (. table x) (..< 0 2)) ([] (. table y) (..< 0 3))))
  (print (. bad length))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

### TableField zip rejects columns of a table that grows, whose storage a span could not follow.

```brix
(import /core/table)
(blank)
(import /core/arena)
(blank)

(main
  (let arena (call Arena 8192))
  (finally
    (inline (call (. arena release)))
  )
  (blank)
  (let table (call (of Table Point) arena 1))
  (call (. table append) (call Point 1 10))
  (let zipped
    (do-new
      (let points (of TableSpan Point) (+ (. table x) (. table y)))
      (print (. points length))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. zipped wait))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
Table column zip requires columns of tables that do not grow
```

### table[range] returns a span; span rows and span columns are relative to the range.

```brix
(import /core/table)
(blank)

(main
  (let table (call (of Table Point) 5 (:with (call Point (as each I32) (as (+ each 10) I32)))))
  (let span ([] table (..< 1 4)))
  (blank)
  (print (. span length))
  (print (as ([] span 0) Point))
  (print (as ([] span 2) Point))
  (print (call (. ", " join) (. span x)))
  (blank)
  (= ([] (. span x) 1) 99)
  (= (. ([] span 2) y) 77)
  (print (as ([] table 1) Point))
  (print (as ([] table 2) Point))
  (print (as ([] table 3) Point))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
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

```brix
(import /core/table)
(blank)

(main
  (let table (call (of Table Point) 4 (:with (call Point (as (+ each 1) I32) (as (+ each 10) I32)))))
  (let full ([] table (..< (. Position first) (. Position end))))
  (let tail ([] full (.. (. Position middle) (. Position last))))
  (let xs ([] (. table x) (.. (. Position first) (. Position last))))
  (blank)
  (print (. full length))
  (print (as ([] full 0) Point))
  (print (as ([] full 3) Point))
  (print (. tail length))
  (print (as ([] tail 0) Point))
  (print (as ([] tail 1) Point))
  (print (. xs length))
  (print ([] xs 2))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
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

```brix
(import /core/table)
(blank)

(main
  (let table (call (of Table Point) 4 (:with (call Point (as (+ each 1) I32) (as (+ each 10) I32)))))
  (let rows (of Indexable (of TableRow Point)) table)
  (let span-rows (of Indexable (of TableRow Point)) ([] table (..< 1 4)))
  (let xs (of Indexable I32) (. table x))
  (blank)
  (print (as ([] rows 1) Point))
  (print (as ([] ([] rows (..< 1 3)) 0) Point))
  (print (as ([] ([] span-rows (.. (. Position first) (. Position last))) 2) Point))
  (print ([] ([] xs (..< 1 3)) 1))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
(2, 11)
(2, 11)
(4, 13)
3
```

### Column range assignment and table span assignment copy selected columns/rows.

```brix
(import /core/table)
(blank)

(main
  (let source (call (of Table Point) 4 (:with (call Point (as (+ each 1) I32) (as (+ each 10) I32)))))
  (let target (call (of Table Point) 4))
  (blank)
  (= ([] (. target x) (..< 1 3)) ([] (. source x) (..< 0 2)))
  (= ([] target (..< 0 2)) ([] source (..< 2 4)))
  (blank)
  (for (in i (..< 0 (. target length)))
    (print (as ([] target i) Point))
  )
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
(3, 12)
(4, 13)
(2, 0)
(0, 0)
```

### A table range must step by one and lie within the table, span or column it indexes.

```brix
(import /core/table)
(blank)

(main
  (let table (call (of Table Point) 5 (:with (call Point (as each I32) (as (+ each 10) I32)))))
  (let target (call (of Table Point) 3))
  (print (as ([] ([] table (.. 3 1)) 0) Point))
  (print (. ([] table (..< 5 5)) length))
  (call attempt (=> () (. ([] table (..< 0 4 (by: 2))) length)))
  (call attempt (=> () (. ([] ([] table (..< 1 3)) (..< 0 3)) length)))
  (call attempt (=> () (. ([] ([] (. table x) (..< 1 3)) (..< 2 4)) length)))
  (call attempt (=> () (. ([] table (..< -1 2)) length)))
  (let stored
    (do-new
      (= ([] target (..< 2 4)) ([] table (..< 0 2)))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. stored wait))
  (print (as ([] target 2) Point))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)

(fn-do + attempt _
  (param count (fn-type Size))
  (let task
    (do-new
      (print (call count))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. task wait))
)
```

```output
(1, 11)
0
Table requires a contiguous range
TableSpan range is out of bounds
TableField range is out of bounds
Table range is out of bounds
Table range is out of bounds
(0, 0)
```

### A range assignment needs a source at least as long as the range, and copies a prefix of a longer one.

```brix
(import /core/table)
(blank)

(main
  (let source (call (of Table Point) 5 (:with (call Point (as each I32) (as (+ each 10) I32)))))
  (let target (call (of Table Point) 3))
  (let values (call (of Table Value) 3))
  (= ([] values 0) (call (. Value left) (call Left 1)))
  (= ([] values 1) (call (. Value right) (call Left 2)))
  (let rows
    (do-new
      (= ([] target (..< 0 3)) ([] ([] source (..< 1 5)) (..< 0 2)))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. rows wait))
  (let items
    (do-new
      (= ([] (. target x) (..< 0 3)) ([] (. source x) (..< 0 2)))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. items wait))
  (let cases
    (do-new
      (= ([] values (..< 0 3)) ([] ([] values (..< 1 3)) (..< 0 1)))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. cases wait))
  (print (as ([] target 0) Point))
  (print (as ([] values 0) Value))
  (= ([] (. target y) (..< 1 3)) (. source y))
  (print (as ([] target 2) Point))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)

(type + Left (is Struct)
  (prop + value I32)
)

(type + Value (is Union)
  (variant left Left)
  (variant right Left)
)
```

```output
Table range source is too short
TableField range source is too short
Table range source is too short
(0, 0)
(1)
(0, 11)
```

### Iterating a table yields live row cursors without forcing row struct materialization.

```brix
(import /core/table)
(blank)

(main
  (let table (call (of Table Point) 3 (:with (call Point (as each I32) (as (+ each 1) I32)))))
  (blank)
  (for (in row table)
    (+= (. row x) 10)
    (print (as row Point))
  )
  (blank)
  (for (in i (..< 0 (. table length)))
    (print (as ([] table i) Point))
  )
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
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

```brix
(import /core/table)
(blank)

(main
  (let table (call (of Table Point) 4 (:with (call Point (as each Size) (as (+ each 10) Size)))))
  (let row-sum (as 0 Size))
  (let index-sum (as 0 Size))
  (blank)
  (for (in row table)
    (+= row-sum (+ (. row x) (. row x) (. row y)))
  )
  (blank)
  (for (in i (..< 0 (. table length)))
    (+= index-sum (+ (. ([] table i) x) (. ([] table i) x) (. ([] table i) y)))
  )
  (blank)
  (print (str row-sum " " index-sum))
)

(type + Point (is Struct)
  (prop + x Size)
  (prop + y Size)
)
```

```output
58 58
```

### Iterating a span yields live row cursors with span-relative indexes.

```brix
(import /core/table)
(blank)

(main
  (let table (call (of Table Point) 4 (:with (call Point (as each I32) (as (+ each 10) I32)))))
  (let span ([] table (..< 1 3)))
  (blank)
  (for (in row span)
    (+= (. row y) 100)
    (print (as row Point))
  )
  (blank)
  (for (in i (..< 0 (. table length)))
    (print (as ([] table i) Point))
  )
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
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

```brix
(import /core/table)
(blank)

(main
  (let first (call (of Table Point) 2 (:with (call Point (as (+ each 1) I32) (as (+ each 10) I32)))))
  (let second (call (of Table Point) 2 (:with (call Point (as (+ each 101) I32) (as (+ each 110) I32)))))
  (let table first)
  (blank)
  (for (in i (..< 0 2))
    (if (== i 1)
      (= table second)
    )
    (print (. ([] table i) x))
  )
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
1
102
```

### A table held by a loop variable is read after the loop binds it, never from before the loop.

```brix
(import /core/table)
(blank)

(main
  (let one (call (of Table Point) 1 (:with (call Point (as (+ each 1) I32) (as (+ each 10) I32)))))
  (let two (call (of Table Point) 2 (:with (call Point (as (+ each 21) I32) (as (+ each 30) I32)))))
  (let three (call (of Table Point) 3 (:with (call Point (as (+ each 41) I32) (as (+ each 50) I32)))))
  (let tables (list (of Table Point)))
  (+= tables one)
  (+= tables two)
  (+= tables three)
  (blank)
  (for (in table tables)
    (print (str (. table length) " " (. ([] table 0) x)))
  )
  (print (call (. ", " join) (in (str (. each length)) tables)))
  (print (call total-rows tables))
)

(fn-do _ total-rows Size
  (param tables (list (of Table Point)))
  (let total Size 0)
  (for (in table tables)
    (+= total (. table length))
  )
  (return total)
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
1 1
2 21
3 41
1, 2, 3
6
```

### copy creates independent fixed-size column storage.

```brix
(import /core/table)
(blank)

(main
  (let original (call (of Table Point) 2 (:with (call Point (as (+ each 1) I32) (as (+ each 10) I32)))))
  (let copied (call (. original copy)))
  (blank)
  (= (. ([] copied 0) x) 99)
  (= (. ([] original 1) y) 88)
  (blank)
  (print (as ([] original 0) Point))
  (print (as ([] original 1) Point))
  (print (as ([] copied 0) Point))
  (print (as ([] copied 1) Point))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
(1, 10)
(2, 88)
(99, 10)
(2, 11)
```

### Function arguments materialize TableRow cursors as live row views.

```brix
(import /core/table)
(blank)

(main
  (let table (call (of Table Point) 4 (:with (call Point (as (+ each 1) I32) (as (+ each 10) I32)))))
  (blank)
  (print (call bump ([] table 1)))
  (for (in row table (when (== (. row x) 1)))
    (print (call bump row))
  )
  (for (in table (when (== (. each x) 3)))
    (print (call bump each))
  )
  (for (in table (when (== (. it x) 4)))
    (print (call bump it))
  )
  (blank)
  (for (in i (..< 0 (. table length)))
    (print (as ([] table i) Point))
  )
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)

(fn-do + bump Point
  (param row (of TableRow Point))
  (+= (. row x) 10)
  (return (as row Point))
)
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

```brix
(import /core/table)
(blank)

(main
  (let table (call (of Table Point) 2 (:with (call Point (as (+ each 1) I32) (as (+ each 10) I32)))))
  (blank)
  (print (call move ([] table 0)))
  (for (in row table (when (== (. row x) 2)))
    (print (call move row))
  )
  (blank)
  (print (as ([] table 0) Point))
  (print (as ([] table 1) Point))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)

(fn-do + move Point
  (param point Point)
  (+= (. point x) 100)
  (+= (. point y) 1000)
  (return point)
)
```

```output
(101, 1010)
(102, 1011)
(1, 10)
(2, 11)
```

### Escaped TableRows stay live, while gathered row values are snapshots.

```brix
(import /core/table)
(blank)

(main
  (let table (call (of Table Point) 1))
  (= ([] table 0) (call Point 1 2))
  (blank)
  (let row ([] table 0))
  (let snapshot (as ([] table 0) Point))
  (= (. ([] table 0) x) 7)
  (= (. row y) 8)
  (= (. snapshot x) 9)
  (blank)
  (print (as row Point))
  (print snapshot)
  (print (as ([] table 0) Point))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
(7, 8)
(9, 2)
(7, 8)
```

### Returning table[i] or a loop row materializes a live TableRow value.

```brix
(import /core/table)
(blank)

(main
  (let table (call (of Table Point) 2 (:with (call Point (as (+ each 1) I32) (as (+ each 10) I32)))))
  (let direct (call row-at table 1))
  (let first (call first-row table))
  (blank)
  (= (. ([] table 1) x) 50)
  (= (. direct y) 60)
  (= (. first x) 70)
  (blank)
  (print (as direct Point))
  (print (as first Point))
  (print (as ([] table 0) Point))
  (print (as ([] table 1) Point))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)

(fn-expr + row-at (of TableRow Point)
  (param table (of Table Point))
  (param index Index)
  ([] table index)
)

(fn-do + first-row (of TableRow Point)
  (param table (of Table Point))
  (for (in row table)
    (return row)
  )
  (throw (call Panic "Expected a non-empty table"))
)
```

```output
(50, 60)
(70, 10)
(70, 10)
(50, 60)
```

### Yielding table[i] and row materializes live TableRow values.

```brix
(import /core/table)
(blank)

(main
  (let table (call (of Table Point) 2 (:with (call Point (as (+ each 1) I32) (as (+ each 10) I32)))))
  (let stream (call rows table))
  (let direct (!! (call stream)))
  (let looped (!! (call stream)))
  (blank)
  (= (. ([] table 1) x) 50)
  (= (. direct y) 60)
  (= (. looped x) 70)
  (blank)
  (print (as direct Point))
  (print (as looped Point))
  (print (as ([] table 0) Point))
  (print (as ([] table 1) Point))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)

(fn-do + rows _
  (param table (of Table Point))
  (yield ([] table 1))
  (for (in row table)
    (yield row)
  )
)
```

```output
(50, 60)
(70, 10)
(70, 10)
(50, 60)
```

### Arrays, lists, and object fields can store live TableRow values.

```brix
(import /core/table)
(blank)

(main
  (let table (call (of Table Point) 2 (:with (call Point (as (+ each 1) I32) (as (+ each 10) I32)))))
  (let array (list ([] table 0) ([] table 1)))
  (let rows (call (of List (of TableRow Point))))
  (+= rows ([] table 0))
  (+= rows ([] table 1))
  (let holder (call Holder ([] table 0)))
  (blank)
  (= (. ([] table 0) x) 30)
  (= (. ([] array 1) y) 40)
  (= (. ([] rows 0) y) 50)
  (= (. holder row x) 60)
  (blank)
  (print (as ([] array 0) Point))
  (print (as ([] array 1) Point))
  (print (as ([] rows 0) Point))
  (print (as ([] rows 1) Point))
  (print (as (. holder row) Point))
  (print (as ([] table 0) Point))
  (print (as ([] table 1) Point))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)

(type + Holder
  (prop + row (of TableRow Point))
  (fn-do + This _
    (param row (of TableRow Point))
  )
)
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

```brix
(import /core/table)
(blank)

(main
  (let table (call (of Table Point) 2 (:with (call Point (as (+ each 1) I32) (as (+ each 10) I32)))))
  (let getter (call make-getter table))
  (blank)
  (= (. ([] table 0) x) 8)
  (= (. ([] table 1) x) 9)
  (print (as (call getter) Point))
  (= (. (call getter) y) 77)
  (print (as ([] table 0) Point))
  (print (as ([] table 1) Point))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)

(fn-do + make-getter (of Getter (of TableRow Point))
  (param table (of Table Point))
  (let index 0)
  (lazy row ([] table index))
  (let getter (=> () row))
  (= index 1)
  (return getter)
)
```

```output
(9, 11)
(8, 10)
(9, 77)
```

### Whole-row address-taking cannot pass table[i] as a contiguous row struct pointer.

```brix crash
(import /core/table)
(blank)
(import /JaCore/tests.h)
(blank)

(main
  (let table (call (of Table Thing) 1))
  (call C:ctest15 ([] &table 0))
)

(type + Thing (is Struct)
  (prop + a Int)
  (prop + b String)
)
```

```output
Exception: No C function "ctest15" accepts (Table'Thing); it is declared as "ctest15((of CPointer I8))"
<no file:9>
      (call C:ctest15 ([] &table 0))
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Table row types must be concrete fixed-layout structs, not class/object types.

```brix crash
(import /core/table)
(blank)

(main
  (let table (call (of Table Thing) 1))
  (print (. table length))
)

(type + Thing
  (prop + value I32 (= 0))
)
```

```output
Exception: Table row types must be concrete fixed-layout structs
<no file:10>
    (type + Thing
    ^^^^^^^^^^^^
```

### Table row types cannot include getter or lazy row fields in v1.

```brix crash
(import /core/table)
(blank)

(main
  (let getter-table (call (of Table GetterRow) 1))
  (let lazy-table (call (of Table LazyRow) 1))
  (print (+ (. getter-table length) (. lazy-table length)))
)

(type + GetterRow (is Struct)
  (prop + stored I32)
  (fn-expr + derived _ :get
    (+ stored 1)
  )
)

(type + LazyRow (is Struct)
  (prop + stored I32)
  (prop + cached _ (:= (+ stored 1)))
)
```

```output
Exception: Table row fields must be eager stored fields
<no file:13>
      (fn-expr + derived _ :get
      ^^^^^^^^^^^^^^^^^^^^^^^^
```

### Table row types cannot rely on delegated interface-shaped behavior in v1.

```brix crash
(import /core/table)
(blank)

(main
  (let table (call (of Table DelegatingRow) 1))
  (print (. table length))
)

(type + Proto (is Interface)
  (fn-expr + act _)
)

(type + Delegate
  (fn-expr + act _)
)

(type + DelegatingRow (is Struct)
  (delegate Proto)
  (prop + delegate Delegate)
)
```

```output
Exception: Table row types cannot use delegated interface behavior
<no file:18>
    (type + DelegatingRow (is Struct)
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Table row types must be named structs, not tuple aliases.

```brix crash
(import /core/table)
(blank)

(main
  (let table (call (of Table Thing) 1))
  (print (. table length))
)

(type + Thing (is (tuple (: a Int) (: b String))))
```

```output
Exception: Table row types must be concrete fixed-layout structs
<no file:10>
    (type + Thing (is (tuple (: a Int) (: b String))))
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Pointerful scalar row field stores emit barriers during concurrent marking.

```brix
(import /core/table)
(blank)
(import /BxCore/BxCore.h)
(import /BxGC/BxGCTest.h)
(blank)

(main
  (let table (call (of Table Cell) 1))
  (= ([] table 0) (call Cell (call GCNode 1) (call GCNode 2)))
  (blank)
  (let barrier-count-before (call C:brixGCBarrierCount))
  (call C:brixGCStartConcurrentMark)
  (= (. ([] table 0) left) (call GCNode 3))
  (call C:brixGCFinishConcurrentMark)
  (let barrier-count-after (call C:brixGCBarrierCount))
  (call C:brixCollect)
  (blank)
  (if (<= barrier-count-after barrier-count-before)
    (print "barrier-missing")
    (else-if (not (call C:brixGCVerifyObject (as (. ([] table 0) left) Pointer)))
      (print "left-missing")
    )
    (else
      (print (str "barrier-ok " (. ([] table 0) left value)))
    )
  )
)

(type + Cell (is Struct)
  (prop + left GCNode)
  (prop + right GCNode)
)

(type + GCNode
  (prop + value _ (= 0))
  (fn-do + This _
    (param value Int)
  )
)
```

```output
barrier-ok 3
```

### Pointerful row scatter keeps newly stored row objects alive.

```brix
(import /core/table)
(blank)
(import /BxCore/BxCore.h)
(import /BxGC/BxGCTest.h)
(blank)

(main
  (let table (call (of Table Cell) 2))
  (= ([] table 0) (call Cell (call GCNode 10) (call GCNode 20)))
  (blank)
  (let barrier-count-before (call C:brixGCBarrierCount))
  (call C:brixGCStartConcurrentMark)
  (= ([] table 1) (call Cell (call GCNode 30) (call GCNode 40)))
  (call C:brixGCFinishConcurrentMark)
  (let barrier-count-after (call C:brixGCBarrierCount))
  (blank)
  (call C:brixCollect)
  (blank)
  (if (<= barrier-count-after barrier-count-before)
    (print "scatter-barrier-missing")
    (else-if (not (call C:brixGCVerifyObject (as (. ([] table 1) left) Pointer)))
      (print "left-missing")
    )
    (else-if (not (call C:brixGCVerifyObject (as (. ([] table 1) right) Pointer)))
      (print "right-missing")
    )
    (else
      (print (str "scatter-ok " (. ([] table 1) left value) " " (. ([] table 1) right value)))
    )
  )
)

(type + Cell (is Struct)
  (prop + left GCNode)
  (prop + right GCNode)
)

(type + GCNode
  (prop + value _ (= 0))
  (fn-do + This _
    (param value Int)
  )
)
```

```output
scatter-ok 30 40
```

### Pointerful range copy emits barriers and preserves copied objects.

```brix
(import /core/table)
(blank)
(import /BxCore/BxCore.h)
(import /BxGC/BxGCTest.h)
(blank)

(main
  (let source (call (of Table Cell) 3 (:with (call Cell (call GCNode (as (+ each 1) Int)) (call GCNode (as (+ each 10) Int))))))
  (let target (call (of Table Cell) 3))
  (blank)
  (let barrier-count-before (call C:brixGCBarrierCount))
  (call C:brixGCStartConcurrentMark)
  (= ([] target (..< 0 2)) ([] source (..< 1 3)))
  (call C:brixGCFinishConcurrentMark)
  (let barrier-count-after (call C:brixGCBarrierCount))
  (call C:brixCollect)
  (blank)
  (if (<= barrier-count-after barrier-count-before)
    (print "range-copy-barrier-missing")
    (else-if (not (call C:brixGCVerifyObject (as (. ([] target 0) left) Pointer)))
      (print "target-left-missing")
    )
    (else-if (not (call C:brixGCVerifyObject (as (. ([] target 1) right) Pointer)))
      (print "target-right-missing")
    )
    (else
      (print (str "range-copy-ok " (. ([] target 0) left value) " " (. ([] target 1) right value)))
    )
  )
)

(type + Cell (is Struct)
  (prop + left GCNode)
  (prop + right GCNode)
)

(type + GCNode
  (prop + value _ (= 0))
  (fn-do + This _
    (param value Int)
  )
)
```

```output
range-copy-ok 2 12
```

### Table field reads are invalidated by stores through aliased table values.

```brix
(import /core/table)
(blank)

(main
  (let table (call (of Table Point) 1))
  (= ([] table 0) (call Point 1 2))
  (blank)
  (let other table)
  (let b (. ([] table 0) x))
  (= (. ([] other 0) x) 7)
  (let a (. ([] table 0) x))
  (blank)
  (print (str b " " a " " (. ([] other 0) x)))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
1 7 7
```

### Growing a table does not reuse a column base read before the growth.

```brix
(import /core/table)
(blank)
(import /core/arena)
(blank)

(main
  (let arena (call Arena 8192))
  (finally
    (inline (call (. arena release)))
  )
  (let rows (call (of Table Point) arena 2))
  (call (. rows append) (call Point 1 2))
  (call (. rows append) (call Point 3 4))
  (let b (. ([] rows 0) x))
  (call (. rows append) (call Point 5 6))
  (call (. rows append) (call Point 7 8))
  (call (. rows append) (call Point 9 10))
  (= (. ([] rows 0) x) 70)
  (let column (. rows x))
  (blank)
  (print (str b " " (. ([] rows 0) x) " " ([] column 0)))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
1 70 70
```

### A call that grows a table does not leave the caller with a stale column base.

```brix
(import /core/table)
(blank)
(import /core/arena)
(blank)

(main
  (let arena (call Arena 8192))
  (finally
    (inline (call (. arena release)))
  )
  (let rows (call (of Table Point) arena 1))
  (call (. rows append) (call Point 1 2))
  (let b (. ([] rows 0) x))
  (call grow rows)
  (= (. ([] rows 0) x) 70)
  (let column (. rows x))
  (blank)
  (print (str b " " (. ([] rows 0) x) " " ([] column 0) " " (. rows length)))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)

(fn-do + grow _
  (param rows (of Table Point))
  (for (in index (..< 0 20))
    (call (. rows append) (call Point (as index I32) 0))
  )
)
```

```output
1 70 70 21
```

### Growing a table through an aliased value does not leave a stale column base.

```brix
(import /core/table)
(blank)
(import /core/arena)
(blank)

(main
  (let arena (call Arena 8192))
  (finally
    (inline (call (. arena release)))
  )
  (let rows (call (of Table Point) arena 1))
  (call (. rows append) (call Point 1 2))
  (let other rows)
  (let b (. ([] rows 0) x))
  (call (. other append) (call Point 3 4))
  (call (. other append) (call Point 5 6))
  (= (. ([] rows 0) x) 70)
  (let column (. other x))
  (blank)
  (print (str b " " (. ([] rows 0) x) " " ([] column 0)))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
1 70 70
```

### A method that grows a table held by another object does not leave a stale column base.

```brix
(import /core/table)
(blank)
(import /core/arena)
(blank)

(main
  (let holder (call Holder))
  (finally
    (inline (call (. holder arena release)))
  )
  (let adder (call Adder holder))
  (for (in (: index U32) (..< 0 7))
    (call (. adder add) index)
  )
  (let pass (call Pass adder))
  (call (. pass run))
  (print (str (. ([] (. holder rows) 0) count) " " (. ([] (. holder rows) 4) count) " " (. holder rows length)))
)

(type + Row (is Struct)
  (prop + name U32)
  (prop + count U32)
)

(type + Holder
  (prop + arena Arena)
  (prop + rows (of Table Row))
  (fn-do + This _
    (lazy rows (call (of Table Row) arena 8))
  )
)

(type + Adder
  (prop + holder Holder)
  (fn-do + This _
    (param holder Holder)
  )
  (fn-do + add U32
    (param name U32)
    (let row (as (. holder rows length) U32))
    (call (. holder rows append) (call Row name 1))
    (return row)
  )
)

(type + Pass
  (prop _ adder Adder)
  (prop _ holder Holder)
  (fn-do + This _
    (param adder Adder)
    (lazy holder (. adder holder))
  )
  (fn-do + run _
    (for (in (: position U32) (..< 0 7))
      (let previous (. ([] (. holder rows) position) count))
      (call (. adder add) (+ 100 position))
      (call (. adder add) (+ 200 position))
      (= (. ([] (. holder rows) position) count) 0)
      (call bump position (+ previous 4))
    )
  )
  (fn-do _ bump _
    (param row U32)
    (param amount U32)
    (= (. ([] (. holder rows) row) count) (+ (. ([] (. holder rows) row) count) amount))
  )
)
```

```output
5 5 21
```

### Captured row aliases materialize before entering closures.

```brix
(import /core/table)
(blank)

(main
  (let table (call (of Table Point) 2 (:with (call Point (as each I32) (as (+ each 10) I32)))))
  (blank)
  (for (in row table)
    (let fn (=> () (as row Point)))
    (+= (. row x) 20)
    (print (call fn))
  )
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
(20, 10)
(21, 11)
```

### Captured indexed row aliases materialize with the current loop index.

```brix
(import /core/table)
(blank)

(main
  (let table (call (of Table Point) 2 (:with (call Point (as each I32) (as (+ each 10) I32)))))
  (blank)
  (for (in i (..< 0 (. table length)))
    (let row ([] table i))
    (let fn (=> () (as row Point)))
    (+= (. row x) 30)
    (print (call fn))
  )
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
(30, 10)
(31, 11)
```

### Table union rows use common columns, inline and external payloads, live narrowing, and snapshots.

```brix
(import /core/table)
(blank)

(main
  (let nodes
    (call (of Table Node) 3
      (:with
        (=> (index) :do
          (block
            (if (== index 0)
              (return (call (. Node literal) 10 42))
            )
            (return (call (. Node call) (as index U32) 7 8 9 10))
          )
        )
      )
    )
  )
  (blank)
  (= ([] nodes 2) (call Literal 30 100))
  (blank)
  (print (. nodes layout))
  (print (call (. "," join) (. nodes source)))
  (blank)
  (for (in node nodes)
    (match node
      (case (call .literal literal)
        (+= (. literal value) 1)
      )
      (case (call .call call)
        (+= (. call arg2) 100)
      )
    )
  )
  (blank)
  (print (as ([] nodes 0) Node))
  (print (as ([] nodes 1) Node))
  (print (as ([] nodes 2) Node))
  (blank)
  (let snapshot Node (as ([] nodes 1) Node))
  (= ([] (. nodes source) 1) 50)
  (match ([] nodes 1)
    (case (call .literal literal)
      (+= (. literal value) 0)
    )
    (case (call .call call)
      (+= (. call arg2) 1)
    )
  )
  (blank)
  (print
    (match snapshot
      (case (call .literal literal)
        (str "snapshot literal " (. literal source) " " (. literal value))
      )
      (case (call .call call)
        (str "snapshot call " (. call source) " " (. call arg0) " " (. call arg2))
      )
    )
  )
  (print
    (match (as ([] nodes 1) Node)
      (case (call .literal literal)
        (str "current literal " (. literal source) " " (. literal value))
      )
      (case (call .call call)
        (str "current call " (. call source) " " (. call arg0) " " (. call arg2))
      )
    )
  )
  (blank)
  (print (type-of ([] nodes 0)))
  (print (type-of (. nodes source)))
)

(type + Syntax (is Tuple)
  (prop + source U32)
)

(type + Literal (is Syntax)
  (prop + value I64)
)

(type + Call (is Syntax)
  (prop + callee U32)
  (prop + arg0 U64)
  (prop + arg1 U64)
  (prop + arg2 U64)
)

(type + Node (is Union)
  (variant literal Literal)
  (variant call Call)
)
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

```brix
(import /core/table)
(blank)

(main
  (let nodes (call (of Table Node) 2))
  (= ([] nodes 0) (call (. Node leaf) 1 7))
  (= ([] nodes 1) (call (. Node pair) 2 3 4))
  (print (. nodes layout))
  (print (as ([] nodes 0) Node))
  (print (as ([] nodes 1) Node))
  (blank)
  (let flags (call (of Table Flag) 2))
  (= ([] flags 1) (call (. Flag on) 5 true))
  (print (. flags layout))
  (print (as ([] flags 0) Flag))
  (print (as ([] flags 1) Flag))
)

(type + Base (is Tuple)
  (prop + id U32)
)

(type + Leaf (is Base)
  (prop + value U64)
)

(type + Pair (is Base)
  (prop + start U32)
  (prop + count U32)
)

(type + Node (is Union)
  (variant leaf Leaf)
  (variant pair Pair)
)

(type + Off (is Base))

(type + On (is Base)
  (prop + is-lit Bool)
)

(type + Flag (is Union)
  (variant off Off)
  (variant on On)
)
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

```brix
(import /core/table)
(blank)

(main
  (let rows (call (of Table Node) 3))
  (= ([] rows 0) (call (. Node literal) 10 40 7))
  (= ([] rows 1) (call (. Node call) 20 1 2 3 4 5))
  (= ([] rows 2) (call (. Node cold) 30 9 1))
  (blank)
  (print (. rows layout))
  (blank)
  (for (in row rows)
    (match row
      (case (call .literal literal)
        (+= (. literal value) 1)
      )
      (case (call .call call)
        (+= (. call callee) 10)
      )
      (case (call .cold cold)
        (+= (. cold payload) 1000)
      )
    )
  )
  (blank)
  (for (in row rows)
    (match row
      (case (call .literal literal)
        (+= (. literal text-id) 10)
      )
      (case (call .call call)
        (+= (. call diagnostic-id) 100)
      )
      (case (call .cold cold)
        (+= (. cold cold-id) 100)
      )
    )
  )
  (blank)
  (for (in row rows)
    (print (as row Node))
  )
  (blank)
  (= ([] rows 0) (call (. Node call) 50 6 7 8 9 10))
  (print (as ([] rows 0) Node))
)

(type + Syntax (is Tuple)
  (prop + source U32)
)

(type + HotSyntax (is Syntax)
  (prop + value U64 @hot)
)

(type + Literal (is HotSyntax)
  (prop + text-id U32)
)

(type + Call (is Syntax)
  (prop + callee U64 @hot)
  (prop + first-argument U64 @hot)
  (prop + argument-count U16 @hot)
  (prop + flags U16)
  (prop + diagnostic-id U32)
)

(type + Cold (is Syntax)
  (prop + payload U64)
  (prop + cold-id U32)
)

(type + Node (is Union)
  (variant literal Literal)
  (variant call Call)
  (variant cold Cold)
)
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

```brix
(import /core/table)
(blank)

(main
  (let rows (call (of Table Choice) 1))
  (print (. rows layout))
)

(type + Hot (is Tuple)
  (prop + key U32 @hot)
  (prop + wide U64)
  (prop + left U32)
  (prop + right U32)
)

(type + Cold (is Tuple)
  (prop + first U64)
  (prop + second U64)
)

(type + Choice (is Union)
  (variant hot Hot)
  (variant cold Cold)
)
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

```brix
(import /core/table)
(blank)

(main
  (let values (call (of Table Value) 2))
  (= ([] values 0) (call (. Value alpha) 1 2))
  (= ([] values 1) (call (. Value beta) 3))
  (print (. values layout))
  (print (as ([] values 0) Value))
  (print (as ([] values 1) Value))
  (blank)
  (let points (call (of Table Point) 1))
  (= ([] points 0) (call Point 4 5))
  (+= (. ([] points 0) x) 1)
  (print (. points layout))
  (print (as ([] points 0) Point))
)

(type + Alpha (is Tuple)
  (prop + x I32 @hot)
  (prop + y I64 @hot)
)

(type + Beta (is Tuple)
  (prop + z U16 @hot)
)

(type + Value (is Union)
  (variant alpha Alpha)
  (variant beta Beta)
)

(type + Point (is Struct)
  (prop + x I32 @hot)
  (prop + y I32)
)
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

```brix
(import /core/table)
(blank)
(import /core/arena)
(blank)

(main
  (let arena (call Arena 16384))
  (finally
    (inline (call (. arena release)))
  )
  (let nodes (call (of Table Node) arena 1))
  (blank)
  (call (. nodes append) (call (. Node small) 10 1 2))
  (let mark (. nodes mark))
  (call (. nodes append) (call (. Node wide) 20 3 4 5 6))
  (call (. nodes reserve) 8)
  (call (. nodes append) (call (. Node small) 30 7 8))
  (call (. nodes append) (call (. Node wide) 40 9 10 11 12))
  (call (. nodes append) (call (. Node wide) 50 13 14 15 16))
  (blank)
  (match ([] nodes 1)
    (case (call .small small)
      (+= (. small value) 0)
      (+= (. small cold) 0)
    )
    (case (call .wide wide)
      (+= (. wide inline-a) 100)
      (+= (. wide external-b) 1000)
    )
  )
  (blank)
  (print (. nodes layout))
  (print (str "grown " (. nodes length) "/" (. nodes capacity)))
  (print (as ([] nodes 1) Node))
  (blank)
  (call (. nodes rewind) mark)
  (call (. nodes append) (call (. Node wide) 60 17 18 19 20))
  (print (str "rewound " (. nodes length) "/" (. nodes capacity)))
  (print (as ([] nodes 0) Node))
  (print (as ([] nodes 1) Node))
)

(type + Syntax (is Tuple)
  (prop + source U32)
)

(type + Small (is Syntax)
  (prop + value U64 @hot)
  (prop + cold U32)
)

(type + Wide (is Syntax)
  (prop + inline-a U64 @hot)
  (prop + inline-b U64 @hot)
  (prop + external-a U64)
  (prop + external-b U64)
)

(type + Node (is Union)
  (variant small Small)
  (variant wide Wide)
)
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

```brix
(import /core/table)
(blank)
(import /core/arena)
(blank)

(main
  (let arena (call Arena 4096))
  (finally
    (inline (call (. arena release)))
  )
  (let rows (call (of Table Node) arena 1))
  (let valid true)
  (for (in index (..< 0 64))
    (call (. rows append) (call (. Node item) (as index U64) (call SpanData (as index U32) 1) null))
  )
  (for (in index (..< 0 (. rows length)))
    (match ([] rows index)
      (case (call .item row)
        (do
          (= ([] rows index) (call (. Node item) (. row name) (. row range) (as (+ 305419896u64 index) U64)))
          (= valid (and valid (== (. row range start) (as index U32)) (== (. row range count) 1)))
        )
        0
      )
      (default
        0
      )
    )
  )
  (for (in index (..< 0 (. rows length)))
    (match ([] rows index)
      (case (call .item row)
        (do
          (= valid (and valid (== (. row range start) (as index U32)) (== (. row range count) 1)))
          (= valid (and valid (== (!! (. row owner)) (as (+ 305419896u64 index) U64))))
        )
        0
      )
      (default
        0
      )
    )
  )
  (print valid)
)

(type + SpanData (is Tuple)
  (prop + start U32)
  (prop + count U32)
)

(type + Item (is Tuple)
  (prop + name U64 @hot)
  (prop + range SpanData)
  (prop + owner (? U64) @hot)
)

(type + Other (is Tuple)
  (prop + value U8)
)

(type + Node (is Union)
  (variant item Item)
  (variant other Other)
)
```

```output
true
```

### @hot on a field that becomes common is rejected when the union-table descriptor is formed.

```brix crash
(import /core/table)
(blank)

(main
  (let rows (call (of Table Value) 1))
)

(type + Base (is Tuple)
  (prop + id U32 @hot)
)

(type + Left (is Base)
  (prop + left I32)
)

(type + Right (is Base)
  (prop + right I32)
)

(type + Value (is Union)
  (variant left Left)
  (variant right Right)
)
```

```output
Exception: Table union 'Value' common field 'id' cannot be @hot because it already has a dedicated common column
<no file:10>
      (prop + id U32 @hot)
      ^^^^^^^^^^^^^^^^^^^^
```

### Managed case-specific fields remain illegal even when @hot requests inline placement.

```brix crash
(import /core/table)
(blank)

(main
  (let rows (call (of Table Value) 1))
)

(type + Managed (is Tuple)
  (prop + text String @hot)
)

(type + Plain (is Tuple)
  (prop + value I32)
)

(type + Value (is Union)
  (variant managed Managed)
  (variant plain Plain)
)
```

```output
Exception: Table union case 'managed' field 'text' contains managed pointers through String
<no file:10>
      (prop + text String @hot)
      ^^^^^^^^^^^^^^^^^^^^^^^^^
```

### @hot is rejected outside eager tuple storage.

```brix crash
(main
  (let box (call Box))
)

(type + Box
  (prop + value I32 @hot (= 1))
)
```

```output
Exception: Property annotation "@hot" requires an eager stored field on a tuple-layout type
<no file:7>
      (prop + value I32 @hot (= 1))
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### @hot is rejected on lazy tuple properties.

```brix crash
(main
  (let value (call LazyValue))
)

(type + LazyValue (is Tuple)
  (prop + value _ @hot (:= 1))
)
```

```output
Exception: Property annotation "@hot" requires an eager stored field on a tuple-layout type
<no file:7>
      (prop + value _ @hot (:= 1))
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### @hot is rejected on tuple getters instead of being discarded by the seed parser.

```brix crash
(main
  (let value (call GetterValue))
)

(type + GetterValue (is Tuple)
  (prop + value I32 @hot (=> 1))
)
```

```output
Exception: Property annotation "@hot" requires an eager stored field on a tuple-layout type
<no file:7>
      (prop + value I32 @hot (=> 1))
      ^^^^^^^^^^^^^^^^^
```

### @hot is rejected on globals.

```brix crash
(prop + value I32 @hot (= 1))

(main
  (print value)
)
```

```output
Exception: Property annotation "@hot" requires an eager stored field on a tuple-layout type
<no file:2>
    (prop + value I32 @hot (= 1))
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### @hot is rejected on delegated tuple storage.

```brix crash
(main
  (let value (call DelegatingValue (call Delegate)))
)

(type + Proto (is Interface)
  (fn-expr + act _)
)

(type + Delegate
  (implements Proto)
  (fn-expr + act _)
)

(type + DelegatingValue (is Tuple)
  (delegate Proto)
  (prop + delegate Delegate @hot)
)
```

```output
Exception: Property annotation "@hot" requires an eager stored field on a tuple-layout type
<no file:17>
      (prop + delegate Delegate @hot)
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Unknown non-style property annotations name the supported tuple annotation.

```brix crash
(main
  (let value (call Value 1))
)

(type + Value (is Tuple)
  (prop + field I32 @cold)
)
```

```output
Exception: Unknown property annotation "@cold"; only "@hot" is supported on eager tuple fields
<no file:7>
      (prop + field I32 @cold)
      ^^^^^^^^^^^^^^^^^^^^^^^^
```

### @hot is rejected on interface properties.

```brix crash
(main
  (let proto Proto (call Thing))
  (print (. proto value))
)

(type + Proto (is Interface)
  (prop + value I32 @hot)
)

(type + Thing
  (implements Proto)
  (prop + value I32 (= 1))
)
```

```output
Exception: Property annotation "@hot" requires an eager stored field on a tuple-layout type
<no file:8>
      (prop + value I32 @hot)
      ^^^^^^^^^^^^^^^^^^^^^^^
```

### @hot is rejected on style properties while @inherited retains the style-specific path.

```brix crash
(import /core/style)
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> BatchDemo (id: demo)
    (<> Label
      (= ink 9)
    )
  )
)

(type + BatchDemo (is Box)
  (implements MarkupRootElement)
  (using TextStyle)
  (prop + markup-batch-queue _ (:= (call MarkupBatchQueue this)))
  (fn-do + markup-batch-did-flush _
    (param flush MarkupBatchFlush)
  )
)

(type + TextStyle (is Style)
  (prop + ink I64 @hot (= 7))
)

(type + Label (is Box)
  (using TextStyle)
)
```

```output
Exception: Property annotation "@hot" requires an eager stored field on a tuple-layout type
<no file:25>
      (prop + ink I64 @hot (= 7))
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Size-only union tables initialize every row to the first declared case's default value.

```brix
(import /core/table)
(blank)

(main
  (let nodes (call (of Table Node) 2))
  (print (. (as ([] nodes 0) Node) case))
  (print (as ([] nodes 0) Node))
  (print (as ([] nodes 1) Node))
)

(type + Literal (is Tuple)
  (prop + source U32 (= 12))
  (prop + value I64 (= 34))
)

(type + Call (is Tuple)
  (prop + source U32)
  (prop + callee U32)
)

(type + Node (is Union)
  (variant literal Literal)
  (variant call Call)
)
```

```output
0
(12, 34)
(12, 34)
```

### Whole-row union assignment changes cases repeatedly and clears or restores spill handles.

```brix
(import /core/table)
(blank)

(main
  (let nodes (call (of Table Node) 1))
  (= ([] nodes 0) (call Call 1 2 3 4 5))
  (print (as ([] nodes 0) Node))
  (= ([] nodes 0) (call Literal 6 7))
  (print (as ([] nodes 0) Node))
  (= ([] nodes 0) (call (. Node call) 8 9 10 11 12))
  (print (as ([] nodes 0) Node))
  (blank)
  (let row ([] nodes 0))
  (match row
    (case (call .literal literal)
      (+= (. literal value) 0)
    )
    (case (call .call call)
      (+= (. call arg1) 100)
    )
  )
  (print (as row Node))
)

(type + Syntax (is Tuple)
  (prop + source U32)
)

(type + Literal (is Syntax)
  (prop + value I64)
)

(type + Call (is Syntax)
  (prop + callee U32)
  (prop + arg0 U64)
  (prop + arg1 U64)
  (prop + arg2 U64)
)

(type + Node (is Union)
  (variant literal Literal)
  (variant call Call)
)
```

```output
(1, 2, 3, 4, 5)
(6, 7)
(8, 9, 10, 11, 12)
(8, 9, 10, 111, 12)
```

### Equal field names on unrelated cases do not become common columns.

```brix crash
(import /core/table)
(blank)

(main
  (let rows (call (of Table Value) 1))
  (print (. rows layout))
  (print (. ([] rows 0) value))
)

(type + Left (is Tuple)
  (prop + value I32)
)

(type + Right (is Tuple)
  (prop + value I32)
)

(type + Value (is Union)
  (variant left Left)
  (variant right Right)
)
```

```output
Exception: TableRow'Value has no property "value"
<no file:8>
      (print (. ([] rows 0) value))
             ^^^^^^^^^^^^^^^^^^^^^
```

### Union table spans support slicing, common-column projection, indexing, and iteration.

```brix
(import /core/table)
(blank)

(main
  (let rows (call (of Table Value) 4))
  (= ([] rows 0) (call (. Value left) 10 1))
  (= ([] rows 1) (call (. Value right) 20 2))
  (= ([] rows 2) (call (. Value left) 30 3))
  (= ([] rows 3) (call (. Value right) 40 4))
  (blank)
  (let span ([] rows (..< 1 4)))
  (let nested ([] span (..< 1 3)))
  (print (type-of span))
  (print (str (. span length) "/" (. span size)))
  (print (call (. "," join) (. span source)))
  (print (as ([] nested 0) Value))
  (blank)
  (for (in row span)
    (print (as row Value))
  )
)

(type + Base (is Tuple)
  (prop + source U32)
)

(type + Left (is Base)
  (prop + value I32)
)

(type + Right (is Base)
  (prop + value I64)
)

(type + Value (is Union)
  (variant left Left)
  (variant right Right)
)
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

```brix crash
(import /core/table)
(blank)

(main
  (let rows (call (of Table Value) 1))
)

(type + Base (is Tuple)
  (prop + source U32)
)

(type + TextValue (is Base)
  (prop + text String)
)

(type + NumberValue (is Base)
  (prop + number I64)
)

(type + Value (is Union)
  (variant text TextValue)
  (variant number NumberValue)
)
```

```output
Exception: Table union case 'text' field 'text' contains managed pointers through String
<no file:14>
      (prop + text String)
      ^^^^^^^^^^^^^^^^^^^^
```

### Common managed fields retain typed-column storage and survive union scatter and gather.

```brix
(import /core/table)
(blank)

(main
  (let rows
    (call (of Table Value) 2
      (:with
        (=> (index) :do
          (block
            (if (== index 0)
              (return (call (. Value left) "first" 10))
            )
            (return (call (. Value right) "second" 20))
          )
        )
      )
    )
  )
  (blank)
  (print (call (. ", " join) (. rows name)))
  (= ([] (. rows name) 1) "changed")
  (print (as ([] rows 0) Value))
  (print (as ([] rows 1) Value))
)

(type + Base (is Tuple)
  (prop + name String)
)

(type + Left (is Base)
  (prop + value I32)
)

(type + Right (is Base)
  (prop + value I64)
)

(type + Value (is Union)
  (variant left Left)
  (variant right Right)
)
```

```output
first, second
(first, 10)
(changed, 20)
```

### Whole-row union view assignment copies the active logical value and keeps later mutations independent.

```brix
(import /core/table)
(blank)

(main
  (let rows (call (of Table Value) 2))
  (= ([] rows 0) (call (. Value left) 7))
  (= ([] rows 1) (call (. Value right) 8))
  (= ([] rows 1) ([] rows 0))
  (= ([] rows 0) (call (. Value right) 9))
  (= ([] rows 1) ([] rows 1))
  (blank)
  (print (as ([] rows 0) Value))
  (print (as ([] rows 1) Value))
)

(type + Left (is Tuple)
  (prop + value I32)
)

(type + Right (is Tuple)
  (prop + value I64)
)

(type + Value (is Union)
  (variant left Left)
  (variant right Right)
)
```

```output
(9)
(7)
```

### Controlled union row and range copies preserve active inline and spill fields, overlap order, and independent immutable-table clones.

```brix
(import /core/table)
(blank)

(main
  (let rows (call (of Table Node) 5))
  (= ([] rows 0) (call (. Node small) 10 1 100 1000))
  (= ([] rows 1) (call (. Node wide) 20 2 200 2000 2001))
  (= ([] rows 2) (call (. Node small) 30 3 300 3000))
  (= ([] rows 3) (call (. Node wide) 40 4 400 4000 4001))
  (= ([] rows 4) (call (. Node small) 50 5 500 5000))
  (blank)
  (let span ([] rows (..< 1 4)))
  (let (tuple sources flags) span)
  (print (call (. "," join) sources))
  (print (call (. "," join) flags))
  (blank)
  (= ([] span 1) ([] rows 0))
  (= ([] rows 4) ([] span 0))
  (= ([] rows (..< 1 5)) ([] rows (..< 0 4)))
  (blank)
  (for (in row rows)
    (print (as row Node))
  )
  (blank)
  (let clone (call (. rows copy)))
  (= ([] rows 2) (call (. Node small) 99 9 900 9000))
  (print (as ([] clone 2) Node))
  (blank)
  (let lower (call (of Table Node) 5))
  (= ([] lower (..< 0 5)) ([] clone (..< 0 5)))
  (= ([] lower (..< 0 4)) ([] lower (..< 1 5)))
  (for (in row lower)
    (print (as row Node))
  )
)

(type + Syntax (is Tuple)
  (prop + source U32)
  (prop + flags U16)
)

(type + Small (is Syntax)
  (prop + value U64 @hot)
  (prop + cold U32)
)

(type + Wide (is Syntax)
  (prop + value U64 @hot)
  (prop + external-a U64)
  (prop + external-b U64)
)

(type + Node (is Union)
  (variant small Small)
  (variant wide Wide)
)
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

```brix
(import /core/table)
(blank)

(main
  (let rows (call (of Table Value) 1))
  (= ([] rows 0) (call (. Value left) 3))
  (let row (call first-row rows))
  (= ([] rows 0) (call (. Value right) 4))
  (print (as row Value))
)

(type + Left (is Tuple)
  (prop + value I32)
)

(type + Right (is Tuple)
  (prop + value I64)
)

(type + Value (is Union)
  (variant left Left)
  (variant right Right)
)

(fn-do + first-row (of TableRow Value)
  (param rows (of Table Value))
  (return ([] rows 0))
)
```

```output
(4)
```

### A captured union row alias remains live and rematches the current active case at use.

```brix
(import /core/table)
(blank)

(main
  (let rows (call (of Table Value) 1))
  (= ([] rows 0) (call (. Value left) 5))
  (let row ([] rows 0))
  (let getter (=> () row))
  (= ([] rows 0) (call (. Value right) 6))
  (print (as (call getter) Value))
)

(type + Left (is Tuple)
  (prop + value I32)
)

(type + Right (is Tuple)
  (prop + value I64)
)

(type + Value (is Union)
  (variant left Left)
  (variant right Right)
)
```

```output
(6)
```

### Union base row views pass, return, yield, capture, and store as live cursors, while ordinary-union and narrowed-case boundaries gather snapshots.

```brix
(import /core/table)
(blank)

(main
  (let rows (call (of Table Node) 3))
  (= ([] rows 0) (call (. Node left) 10 100))
  (= ([] rows 1) (call (. Node right) 20 200))
  (= ([] rows 2) (call (. Node left) 30 300))
  (blank)
  (let direct (call row-at rows 0))
  (let passed (call pass-row ([] rows 1)))
  (let stream (call yielded-rows rows))
  (let yielded-direct (!! (call stream)))
  (let yielded-loop (!! (call stream)))
  (let tuple (tuple ([] rows 0) ([] rows 2)))
  (let (tuple tuple-first tuple-last) tuple)
  (let array (list ([] rows 0) ([] rows 1)))
  (let list (call (of List (of TableRow Node))))
  (+= list ([] rows 1))
  (let holder (call Holder ([] rows 2)))
  (let captured ([] rows 2))
  (let getter (=> () captured))
  (let lazy-getter (call make-lazy-getter rows))
  (let case-snapshot (call snapshot-left ([] rows 0)))
  (let widened (call widen-left ([] rows 0)))
  (let union-snapshot (call pass-value ([] rows 2)))
  (blank)
  (= ([] rows 0) (call (. Node right) 11 110))
  (= ([] rows 1) (call (. Node left) 21 210))
  (= ([] rows 2) (call (. Node right) 31 310))
  (blank)
  (print (as direct Node))
  (print (as passed Node))
  (print (as yielded-direct Node))
  (print (as yielded-loop Node))
  (print (as tuple-first Node))
  (print (as tuple-last Node))
  (print (as ([] array 0) Node))
  (print (as ([] array 1) Node))
  (print (as ([] list 0) Node))
  (print (as (. holder row) Node))
  (print (as (call getter) Node))
  (print (as (call lazy-getter) Node))
  (print case-snapshot)
  (print (as widened Node))
  (print union-snapshot)
)

(type + Base (is Tuple)
  (prop + source U32)
)

(type + Left (is Base)
  (prop + value I32)
)

(type + Right (is Base)
  (prop + value I64)
)

(type + Node (is Union)
  (variant left Left)
  (variant right Right)
)

(type + Holder
  (prop + row (of TableRow Node))
  (fn-do + This _
    (param row (of TableRow Node))
  )
)

(fn-expr + row-at (of TableRow Node)
  (param rows (of Table Node))
  (param index Index)
  ([] rows index)
)

(fn-expr + pass-row (of TableRow Node)
  (param row (of TableRow Node))
  row
)

(fn-do + yielded-rows _
  (param rows (of Table Node))
  (yield ([] rows 1))
  (for (in row rows)
    (yield row)
  )
)

(fn-do + make-lazy-getter (of Getter (of TableRow Node))
  (param rows (of Table Node))
  (let index 0)
  (lazy row ([] rows index))
  (let getter (=> () row))
  (= index 1)
  (return getter)
)

(fn-expr + snapshot-left Left
  (param row (of TableRow Node))
  (match row
    (case (call .left left)
      left
    )
    (case .right
      (call Left 0 0)
    )
  )
)

(fn-expr + widen-left (of TableRow Node)
  (param row (of TableRow Node))
  (match row
    (case (call .left left)
      (as left (of TableRow Node))
    )
    (case .right
      row
    )
  )
)

(fn-expr + pass-value Node
  (param node Node)
  node
)
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

```brix
(import /core/table)
(blank)

(main
  (let table (call (of Table Point) 2 (:with (call Point (as each Size) (as (+ each 10) Size)))))
  (blank)
  (for (in row table)
    (let fn (=> () (+ (. row x) (. row x))))
    (+= (. row x) 5)
    (print (call fn))
  )
)

(type + Point (is Struct)
  (prop + x Size)
  (prop + y Size)
)
```

```output
10
12
```

### Arena-backed Table grows while aliases and pre-growth views keep resolving current storage.

```brix
(import /core/table)
(blank)
(import /core/arena)
(blank)

(main
  (let arena (call Arena 8192))
  (finally
    (inline (call (. arena release)))
  )
  (blank)
  (let table (call (of Table Point) arena 1))
  (let alias table)
  (print (str "initial " (. table length) "/" (. table capacity)))
  (blank)
  (let first (call (. table append) (call Point 1 10)))
  (let row ([] table first))
  (let xs (. table x))
  (let span ([] table (..< 0 1)))
  (blank)
  (call (. table reserve) 8)
  (let more (call (of Array Point) (list (call Point 2 20) (call Point 3 30))))
  (call (. table extend) more)
  (print (str "grown " (. alias length) "/" (. alias capacity)))
  (print (as row Point))
  (print ([] xs 0))
  (print (as ([] span 0) Point))
  (for (in current alias)
    (print (str "loop " (as current Point)))
  )
  (blank)
  (let mark (. table mark))
  (call (. table append) (call Point 4 40))
  (print (str "appended " (. table length) "/" (. table capacity)))
  (call (. table rewind) mark)
  (print (str "rewound " (. table length) "/" (. table capacity)))
  (print (as ([] table 2) Point))
  (blank)
  (call (. table truncate) 2)
  (print (str "truncated " (. table length) "/" (. table capacity)))
  (call (. table clear))
  (print (str "cleared " (. alias length) "/" (. alias capacity)))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
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

```brix
(import /core/table)
(blank)
(import /core/arena)
(blank)

(main
  (let arena (call Arena 16384))
  (finally
    (inline (call (. arena release)))
  )
  (let nodes (call (of Table Node) arena 1))
  (blank)
  (call (. nodes append) (call (. Node literal) 10 42))
  (let mark (. nodes mark))
  (let first ([] nodes 0))
  (call (. nodes append) (call (. Node call) 20 1 2 3 4))
  (call (. nodes append) (call (. Node literal) 30 43))
  (call (. nodes append) (call (. Node call) 40 5 6 7 8))
  (call (. nodes append) (call (. Node call) 50 9 10 11 12))
  (blank)
  (print (str "grown " (. nodes length) "/" (. nodes capacity)))
  (print (as first Node))
  (print (as ([] nodes 1) Node))
  (print (as ([] nodes 4) Node))
  (blank)
  (call (. nodes rewind) mark)
  (print (str "rewound " (. nodes length) "/" (. nodes capacity)))
  (call (. nodes append) (call (. Node call) 60 13 14 15 16))
  (print (as ([] nodes 0) Node))
  (print (as ([] nodes 1) Node))
)

(type + Syntax (is Tuple)
  (prop + source U32)
)

(type + Literal (is Syntax)
  (prop + value I64)
)

(type + Call (is Syntax)
  (prop + callee U32)
  (prop + arg0 U64)
  (prop + arg1 U64)
  (prop + arg2 U64)
)

(type + Node (is Union)
  (variant literal Literal)
  (variant call Call)
)
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

### Arena-backed union rows keep their latest case and fields when replaced repeatedly.

```brix
(import /core/table)
(blank)
(import /core/arena)
(blank)

(main
  (let arena (call Arena 16384))
  (finally
    (inline (call (. arena release)))
  )
  (let nodes (call (of Table Node) arena 4))
  (call (. nodes append) (call (. Node call) 1 2 3 4 5))
  (call (. nodes append) (call (. Node literal) 6 7))
  (call (. nodes append) (call (. Node call) 8 9 10 11 12))
  (for (in pass (..< 0 3))
    (= ([] nodes 0) (call (. Node call) (as pass U32) 20 21 22 23))
    (= ([] nodes 2) (call (. Node literal) (as pass U32) 30))
    (= ([] nodes 2) (call (. Node call) (as pass U32) 40 41 42 43))
  )
  (= ([] nodes 1) (as ([] nodes 0) Node))
  (= ([] nodes 0) (call (. Node literal) 50 51))
  (for (in position (..< 0 (. nodes length)))
    (print (as ([] nodes position) Node))
  )
)

(type + Syntax (is Tuple)
  (prop + source U32)
)

(type + Literal (is Syntax)
  (prop + value I64)
)

(type + Call (is Syntax)
  (prop + callee U32)
  (prop + arg0 U64)
  (prop + arg1 U64)
  (prop + arg2 U64)
)

(type + Node (is Union)
  (variant literal Literal)
  (variant call Call)
)
```

```output
(50, 51)
(2, 20, 21, 22, 23)
(2, 40, 41, 42, 43)
```

### Fixed-size constructors keep their source-compatible shape and reject grow-only operations.

```brix
(import /core/table)
(blank)

(main
  (let table (call (of Table Point) 1))
  (let attempt
    (do-new
      (call (. table append) (call Point 1 2))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
Table.append requires an Arena-backed growable table
```

### Arena reset and table shrink invalidate every previously captured table, row, span, and field view.

```brix
(import /core/table)
(blank)
(import /core/arena)
(blank)

(main
  (let arena (call Arena 8192))
  (finally
    (inline (call (. arena release)))
  )
  (let table (call (of Table Point) arena))
  (call (. table append) (call Point 1 2))
  (let row ([] table 0))
  (let span ([] table (..< 0 1)))
  (let field (. table x))
  (blank)
  (call (. arena reset))
  (call show-stale-table table)
  (call show-stale-row row)
  (call show-stale-span span)
  (call show-stale-field field)
  (blank)
  (let current (call (of Table Point) arena))
  (call (. current append) (call Point 3 4))
  (let current-row ([] current 0))
  (call (. current truncate) 0)
  (call show-stale-row current-row)
)

(fn-do + show-stale-table _
  (param table (of Table Point))
  (let attempt
    (do-new
      (print (. table length))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)

(fn-do + show-stale-row _
  (param row (of TableRow Point))
  (let attempt
    (do-new
      (print (. row x))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)

(fn-do + show-stale-span _
  (param span (of TableSpan Point))
  (let attempt
    (do-new
      (print (. span length))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)

(fn-do + show-stale-field _
  (param field (of TableField I32))
  (let attempt
    (do-new
      (print ([] field 0))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
Table is stale after Arena reset
TableRow is stale after Arena reset
TableSpan is stale after Arena reset
TableField is stale after Arena reset
TableRow was invalidated by table shrink
```

### A table of a released arena is stale, and stays stale once a new arena takes the slot.

```brix
(import /core/table)
(blank)
(import /core/arena)
(blank)

(main
  (let arena (call Arena 8192))
  (let table (call (of Table Point) arena))
  (call (. table append) (call Point 1 2))
  (let row ([] table 0))
  (call (. arena release))
  (call show-stale-table table)
  (call show-stale-row row)
  (blank)
  (let again (call Arena 8192))
  (finally
    (inline (call (. again release)))
  )
  (let fresh (call (of Table Point) again))
  (call (. fresh append) (call Point 5 6))
  (call show-stale-table table)
  (call show-stale-row row)
  (print (. ([] fresh 0) x))
  (call (. again reset))
  (call show-stale-table fresh)
  (let later (call (of Table Point) again))
  (call (. later append) (call Point 7 8))
  (print (. ([] later 0) y))
)

(fn-do + show-stale-table _
  (param table (of Table Point))
  (let attempt
    (do-new
      (print (. table length))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)

(fn-do + show-stale-row _
  (param row (of TableRow Point))
  (let attempt
    (do-new
      (print (. row x))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
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

```brix
(import /core/table)
(blank)
(import /core/arena)
(blank)

(main
  (let arena (call Arena 8192))
  (finally
    (inline (call (. arena release)))
  )
  (let left (call (of Table Point) arena))
  (let right (call (of Table Point) arena))
  (call (. left append) (call Point 1 2))
  (call (. right append) (call Point 3 4))
  (blank)
  (let foreign (. left mark))
  (call show-rejected-rewind right foreign)
  (blank)
  (let mutated (. left mark))
  (= (. ([] left 0) x) 9)
  (call show-rejected-rewind left mutated)
  (blank)
  (print (as ([] left 0) Point))
  (print (as ([] right 0) Point))
)

(fn-do + show-rejected-rewind _
  (param table (of Table Point))
  (param mark TableMark)
  (let attempt
    (do-new
      (call (. table rewind) mark)
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
Table mark is stale or belongs to another table
Table mark is stale or belongs to another table
(9, 2)
(3, 4)
```

### Arena-backed tables accept ordinary String leaves with developer-managed Arena lifetime.

```brix
(import /core/table)
(blank)
(import /core/arena)
(blank)

(main
  (let arena (call Arena 1024))
  (finally
    (inline (call (. arena release)))
  )
  (let rows (call (of Table Managed) arena))
  (call (. rows append) (call Managed (call (. arena copy-string) "arena text")))
  (print (as ([] rows 0) Managed))
)

(type + Managed (is Struct)
  (prop + text String)
)
```

```output
(arena text)
```

### Arena-backed tables reject arbitrary managed containers.

```brix crash
(import /core/table)
(blank)
(import /core/arena)
(blank)

(main
  (let arena (call Arena 1024))
  (finally
    (inline (call (. arena release)))
  )
  (let rows (call (of Table ManagedList) arena))
)

(type + ManagedList (is Struct)
  (prop + values (of List String))
)
```

```output
Exception: Arena type ManagedList contains managed pointers through ManagedList.values
<no file:15>
    (type + ManagedList (is Struct)
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Storage checkpoint validation rejects foreign and pre-mark-mutated tables before Arena rewind.

```brix
(import /core/table)
(blank)
(import /core/arena)
(blank)

(main
  (let arena (call Arena 8192))
  (finally
    (inline (call (. arena release)))
  )
  (let left (call (of Table Point) arena))
  (let right (call (of Table Point) arena))
  (call (. left append) (call Point 1 2))
  (call (. right append) (call Point 3 4))
  (let mark (. arena mark))
  (let checkpoint (call (. left storage-checkpoint) arena mark))
  (let retained (call (of (. arena allocate) U64) 1))
  (= ([] retained 0) 77u64)
  (blank)
  (call show-rejected-storage-restore right checkpoint arena mark)
  (= (. ([] left 0) x) 9)
  (call show-rejected-storage-restore left checkpoint arena mark)
  (print (as ([] left 0) Point))
  (print (as ([] right 0) Point))
  (print ([] retained 0))
)

(fn-do + show-rejected-storage-restore _
  (param table (of Table Point))
  (param checkpoint TableStorageCheckpoint)
  (param arena Arena)
  (param mark ArenaMark)
  (let attempt
    (do-new
      (call (. table validate-storage-restore) checkpoint arena mark)
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
Table storage checkpoint is stale or belongs to another table
Table storage checkpoint is stale or belongs to another table
(9, 2)
(3, 4)
77
```

### Fixed column-zipped spans retain their read path inside optimized loops.

```brix
(import /core/table)
(blank)

(main
  (let table (call (of Table Point) 2 (:with (call Point (as (+ each 1) I32) (as (+ each 10) I32)))))
  (let points (of TableSpan Point) (+ (. table x) (. table y)))
  (for (in point points)
    (print (. point x))
    (print (. point y))
  )
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
1
10
2
11
```

### Controlled union rows copy between Arena-backed and fixed tables, fixed clones outlive Arena reset, and a shrink invalidates previously captured union row views.

```brix
(import /core/table)
(blank)
(import /core/arena)
(blank)

(main
  (let source-arena (call Arena 16384))
  (finally
    (inline (call (. source-arena release)))
  )
  (let source (call (of Table Node) source-arena 1))
  (call (. source append) (call (. Node wide) 10 1 100 1000 1001))
  (call (. source append) (call (. Node small) 20 2 200 2000))
  (let live ([] source 0))
  (call (. source reserve) 8)
  (print (as live Node))
  (blank)
  (let fixed (call (. source copy)))
  (call (. source-arena reset))
  (print (as ([] fixed 0) Node))
  (print (as ([] fixed 1) Node))
  (blank)
  (let target-arena (call Arena 16384))
  (finally
    (inline (call (. target-arena release)))
  )
  (let target (call (of Table Node) target-arena 1))
  (call (. target append) (call (. Node small) 0 0 0 0))
  (call (. target append) (call (. Node small) 0 0 0 0))
  (= ([] target (..< 0 2)) ([] fixed (..< 0 2)))
  (= ([] fixed 0) (call (. Node small) 99 9 900 9000))
  (print (as ([] target 0) Node))
  (print (as ([] target 1) Node))
  (blank)
  (let target-fixed (call (. target copy)))
  (let stale ([] target 0))
  (call (. target truncate) 1)
  (call show-stale-node stale)
  (call (. target-arena reset))
  (print (as ([] target-fixed 0) Node))
  (print (as ([] target-fixed 1) Node))
)

(fn-do + show-stale-node _
  (param row (of TableRow Node))
  (let attempt
    (do-new
      (print (as row Node))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)

(type + Syntax (is Tuple)
  (prop + source U32)
  (prop + flags U16)
)

(type + Small (is Syntax)
  (prop + value U64 @hot)
  (prop + cold U32)
)

(type + Wide (is Syntax)
  (prop + value U64 @hot)
  (prop + external-a U64)
  (prop + external-b U64)
)

(type + Node (is Union)
  (variant small Small)
  (variant wide Wide)
)
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

```brix
(import /core/table)
(blank)
(import /core/arena)
(blank)

(main
  (let rows (call (of Table Node) 2))
  (= ([] rows 0) (call (. Node small) 10 1 100))
  (= ([] rows 1) (call (. Node wide) 20 2 3 200 300))
  (blank)
  (let union-snapshot (call (. ([] rows 0) snapshot)))
  (let common-snapshot (call (. ([] rows 0) common-snapshot)))
  (let returned (call (. ([] rows 0) live)))
  (let assigned (of TableRow Node) (call (. ([] rows 0) live)))
  (let captured (call (. ([] rows 0) live)))
  (let getter (=> () captured))
  (let stored (call (of List (of TableRow Node))))
  (+= stored (call (. ([] rows 0) live)))
  (blank)
  (match ([] rows 0)
    (case (call .small small)
      (let case-snapshot (call (. small case-snapshot)))
      (call (. small lower) 5)
      (print case-snapshot)
      (print "bumped")
    )
    (case .wide
      (print "unexpected")
    )
  )
  (blank)
  (call (. ([] rows 0) bump-source) 1)
  (call (. ([] rows 0) lower) 10)
  (call (. ([] rows 1) lower) 20)
  (blank)
  (print union-snapshot)
  (print common-snapshot)
  (print (as returned Node))
  (print (as assigned Node))
  (print (as (call getter) Node))
  (print (as ([] stored 0) Node))
  (print (as ([] rows 1) Node))
  (blank)
  (let arena (call Arena 8192))
  (finally
    (inline (call (. arena release)))
  )
  (let arena-rows (call (of Table Node) arena 1))
  (call (. arena-rows append) (call (. Node wide) 30 4 5 400 500))
  (let arena-live (call (. ([] arena-rows 0) live)))
  (call (. arena-rows reserve) 8)
  (call (. ([] arena-rows 0) lower) 7)
  (print (as arena-live Node))
  (blank)
  (let stale (call (. ([] arena-rows 0) live)))
  (call (. arena reset))
  (call show-stale stale)
)

(type + Syntax (is Tuple)
  (prop + source U32)
)

(type + Small (is Syntax)
  (prop + value U64 @hot)
  (prop + cold U32)
)

(type + Wide (is Syntax)
  (prop + inline-a U64 @hot)
  (prop + inline-b U64 @hot)
  (prop + spill-a U64)
  (prop + spill-b U64)
)

(type + Node (is Union)
  (variant small Small)
  (variant wide Wide)
)

(extend + Node
  (fn-do + lower _
    (param delta U64)
    (match this
      (case (call .small small)
        (call (. small lower) delta)
      )
      (case (call .wide wide)
        (call (. wide lower) delta)
      )
    )
  )
  (fn-expr + snapshot Node
    this
  )
  (fn-expr + common-snapshot Syntax
    this
  )
  (fn-expr + live (of TableRow Node)
    this
  )
  (fn-expr + describe U32
    (match this
      (case (call .small small)
        (. small source)
      )
      (case (call .wide wide)
        (. wide source)
      )
    )
  )
)

(extend + Syntax
  (fn-do + bump-source _
    (param delta U32)
    (+= (. this source) delta)
  )
)

(extend + Small
  (fn-do + lower _
    (param delta U64)
    (+= (. this source) (as delta U32))
    (+= (. this value) delta)
    (+= (. this cold) (as delta U32))
  )
  (fn-expr + case-snapshot Small
    this
  )
)

(extend + Wide
  (fn-do + lower _
    (param delta U64)
    (+= (. this source) (as delta U32))
    (+= (. this inline-a) delta)
    (+= (. this spill-a) delta)
  )
)

(fn-do + show-stale _
  (param row (of TableRow Node))
  (let attempt
    (do-new
      (print (call (. row describe)))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)
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

```brix
(import /core/table)
(blank)
(import /core/arena)
(import /core/array-builder)
(blank)

(main
  (let arena (call Arena 65536))
  (finally
    (inline (call (. arena release)))
  )
  (let rows (call (of Table Node) arena 1))
  (let side (call (of ArrayBuilder String) arena))
  (call (. rows append) (call (. Node literal) (call (. arena copy-string) "root") (call (. arena copy-string) "token") (call (. arena copy-string) "detail")))
  (call (. side append) (call (. arena copy-string) "kept"))
  (blank)
  (let arena-mark (. arena mark))
  (let rows-checkpoint (call (. rows storage-checkpoint) arena arena-mark))
  (let side-checkpoint (call (. side storage-checkpoint) arena arena-mark))
  (call (. rows append) (call (. Node call) (call (. arena copy-string) "post") (call (. arena copy-string) "callee") (call (. arena copy-string) "diagnostic")))
  (let post-row ([] rows 1))
  (let post-span ([] rows (..< 1 2)))
  (let post-field (. rows source))
  (let post-id 1)
  (for (in index (..< 0 100))
    (call (. rows append) (call (. Node call) (call (. arena copy-string) (str "source-" index)) (call (. arena copy-string) (str "call-" index)) (call (. arena copy-string) (str "diagnostic-" index))))
    (call (. side append) (call (. arena copy-string) (str "side-" index)))
  )
  (blank)
  (call (. rows validate-storage-restore) rows-checkpoint arena arena-mark)
  (call (. side validate-storage-restore) side-checkpoint arena arena-mark)
  (call (. arena rewind) arena-mark)
  (call (. rows restore-storage) rows-checkpoint arena arena-mark)
  (call (. side restore-storage) side-checkpoint arena arena-mark)
  (blank)
  (print (as ([] rows 0) Node))
  (call show-restored-row post-row)
  (call show-restored-span post-span)
  (call show-restored-field post-field)
  (call show-restored-id rows post-id)
  (blank)
  (call (. rows append) (call (. Node call) (call (. arena copy-string) "again") (call (. arena copy-string) "regrown") null))
  (call (. side append) (call (. arena copy-string) "regrown"))
  (print (as ([] rows 1) Node))
  (let restored-side (call (. side finish-immutable)))
  (print (str ([] restored-side 0) "/" ([] restored-side 1)))
  (blank)
  (let standalone-mark (. rows mark))
  (let standalone-capacity (. rows capacity))
  (call (. rows reserve) 256)
  (call (. rows append) (call (. Node literal) (call (. arena copy-string) "temporary") (call (. arena copy-string) "temporary") null))
  (call (. rows rewind) standalone-mark)
  (print (str (. rows length) "/" (> (. rows capacity) standalone-capacity)))
  (blank)
  (let operations 0)
  (for (in round (..< 0 100))
    (let repeat-arena-mark (. arena mark))
    (let repeat-rows-checkpoint (call (. rows storage-checkpoint) arena repeat-arena-mark))
    (call (. rows append) (call (. Node call) (call (. arena copy-string) "repeat") (call (. arena copy-string) "repeat") null))
    (call (. rows validate-storage-restore) repeat-rows-checkpoint arena repeat-arena-mark)
    (call (. arena rewind) repeat-arena-mark)
    (call (. rows restore-storage) repeat-rows-checkpoint arena repeat-arena-mark)
    (+= operations 1)
  )
  (print (str "repeat " operations " " (. rows length) "/" (. restored-side length)))
  (blank)
  (let empty-rows (call (of Table Node) arena))
  (let empty-side (call (of ArrayBuilder String) arena))
  (let empty-mark (. arena mark))
  (let empty-rows-checkpoint (call (. empty-rows storage-checkpoint) arena empty-mark))
  (let empty-side-checkpoint (call (. empty-side storage-checkpoint) arena empty-mark))
  (call (. empty-rows append) (call (. Node literal) (call (. arena copy-string) "discard") (call (. arena copy-string) "discard") null))
  (call (. empty-side append) (call (. arena copy-string) "discard"))
  (call (. empty-rows validate-storage-restore) empty-rows-checkpoint arena empty-mark)
  (call (. empty-side validate-storage-restore) empty-side-checkpoint arena empty-mark)
  (call (. arena rewind) empty-mark)
  (call (. empty-rows restore-storage) empty-rows-checkpoint arena empty-mark)
  (call (. empty-side restore-storage) empty-side-checkpoint arena empty-mark)
  (print (str "empty " (. empty-rows length) "/" (. empty-side length)))
)

(type + Syntax (is Tuple)
  (prop + source String)
)

(type + Literal (is Syntax)
  (prop + token String @hot)
  (prop + detail (? String))
)

(type + Call (is Syntax)
  (prop + callee String @hot)
  (prop + diagnostic (? String))
)

(type + Node (is Union)
  (variant literal Literal)
  (variant call Call)
)

(fn-do + show-restored-row _
  (param row (of TableRow Node))
  (let attempt
    (do-new
      (print (. row source))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)

(fn-do + show-restored-span _
  (param span (of TableSpan Node))
  (let attempt
    (do-new
      (print (. span length))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)

(fn-do + show-restored-field _
  (param field (of TableField String))
  (let attempt
    (do-new
      (print ([] field 0))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)

(fn-do + show-restored-id _
  (param rows (of Table Node))
  (param index Index)
  (let attempt
    (do-new
      (print (. ([] rows index) source))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)
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

```brix
(import /core/table)
(blank)
(import /BxGC/BxGC.h)
(blank)

(main
  (let table (call (of Table Point) 4 (:with (call Point (as each I32) (as (+ each 10) I32)))))
  (let ids (call (of Array U32) (list 3u32 1u32 1u32 0u32)))
  (let selected ([] table ids))
  (print (type-of selected))
  (print (. selected length))
  (print (as ([] selected 0) Point))
  (print (as ([] selected 3) Point))
  (for (in row selected)
    (print (as row Point))
  )
  (blank)
  (let middle ([] selected (..< 1 3)))
  (print (. middle length))
  (for (in row middle)
    (print (as row Point))
  )
  (blank)
  (= ([] ids 0) 2u32)
  (print (as ([] selected 0) Point))
  (= (. ([] selected 1) y) 77)
  (print (as ([] table 1) Point))
  (blank)
  (let span (call (of IndexSpan U32) ids (..< 1 3)))
  (print (type-of span))
  (print (. span length))
  (print ([] span 0))
  (for (in index span)
    (print index)
  )
  (for (in row ([] table span))
    (print (as row Point))
  )
  (blank)
  (let sequence (. ids immutable))
  (for (in row ([] table ([] sequence (..< 0 2))))
    (print (as row Point))
  )
  (blank)
  (print (. ([] table (call (of Array U32))) length))
  (print (as ([] ([] table (call (of Array I8) (list (as 3 I8)))) 0) Point))
  (print (as ([] ([] table (call (of Array U8) (list (as 3 U8)))) 0) Point))
  (print (as ([] ([] table (call (of Array I16) (list (as 2 I16)))) 0) Point))
  (print (as ([] ([] table (call (of Array U16) (list (as 2 U16)))) 0) Point))
  (print (as ([] ([] table (call (of Array I32) (list (as 1 I32)))) 0) Point))
  (print (as ([] ([] table (call (of Array I64) (list (as 0 I64)))) 0) Point))
  (print (as ([] ([] table (call (of Array U64) (list (as 0 U64)))) 0) Point))
  (blank)
  (call show-invalid-i32 ([] table (call (of Array I32) (list (as -1 I32)))))
  (call show-invalid-u32 ([] table (call (of Array U32) (list 4u32))))
  (call show-invalid-span ids)
  (= ids (call (of Array U32)))
  (call C:brixGCCollect)
  (print (as ([] selected 0) Point))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)

(fn-do + show-invalid-i32 _
  (param selection (of TableSelection Point I32))
  (let attempt
    (do-new
      (print (. ([] selection 0) x))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)

(fn-do + show-invalid-u32 _
  (param selection (of TableSelection Point U32))
  (let attempt
    (do-new
      (print (. ([] selection 0) x))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)

(fn-do + show-invalid-span _
  (param ids (of Array U32))
  (let attempt
    (do-new
      (let invalid (call (of IndexSpan U32) ids (..< 0 5)))
      (print (. invalid length))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)
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

```brix
(import /core/table)
(blank)
(import /core/arena)
(import /core/array-builder)
(blank)

(main
  (let table (call (of Table Point) 4 (:with (call Point (as each I32) (as (+ each 10) I32)))))
  (let arena (call Arena 8192))
  (finally
    (inline (call (. arena release)))
  )
  (blank)
  (let growing (call (of ArrayBuilder U32) arena))
  (call (. growing append) 0u32)
  (call (. growing append) 2u32)
  (let growing-selection ([] table growing))
  (for (in value (..< 0 100))
    (call (. growing append) (as (% value 4) U32))
  )
  (print (. growing-selection length))
  (for (in row growing-selection)
    (print (as row Point))
  )
  (= ([] growing 0) 3u32)
  (print (as ([] growing-selection 0) Point))
  (blank)
  (let moving (call (of ArrayBuilder U32) arena))
  (call (. moving append) 0u32)
  (call (. moving append) 1u32)
  (let moving-selection ([] table moving))
  (let moving-position 0)
  (for (in row moving-selection)
    (print (as row Point))
    (if (== moving-position 0)
      (for (in value (..< 0 100))
        (call (. moving append) (as (% value 4) U32))
      )
      (= ([] moving 1) 3u32)
    )
    (+= moving-position 1)
  )
  (blank)
  (let empty-builder (call (of ArrayBuilder U32) arena))
  (let empty-selection ([] table empty-builder))
  (print (. empty-selection length))
  (call (. empty-builder append) 1u32)
  (print (. empty-selection length))
  (blank)
  (let growable-table (call (of Table Point) arena))
  (call (. growable-table append) (call Point 5 15))
  (call (. growable-table append) (call Point 6 16))
  (let growable-selection ([] growable-table (call (of Array U32) (list 1u32))))
  (for (in value (..< 0 32))
    (call (. growable-table append) (call Point (as value I32) (as (+ value 20) I32)))
  )
  (print (as ([] growable-selection 0) Point))
  (call (. growable-table truncate) 1)
  (call show-stale-selection growable-selection)
  (blank)
  (let rewind-builder (call (of ArrayBuilder U32) arena))
  (call (. rewind-builder append) 1u32)
  (let rewind-selection ([] table rewind-builder))
  (let rewind-mark (. rewind-builder mark))
  (call (. rewind-builder append) 2u32)
  (call (. rewind-builder rewind) rewind-mark)
  (call show-stale-selection rewind-selection)
  (blank)
  (let finish-builder (call (of ArrayBuilder U32) arena))
  (call (. finish-builder append) 2u32)
  (let finish-selection ([] table finish-builder))
  (let finished (call (. finish-builder finish-immutable)))
  (print ([] finished 0))
  (call show-stale-selection finish-selection)
  (blank)
  (let arena-mark (. arena mark))
  (let rewind-arena-builder (call (of ArrayBuilder U32) arena))
  (call (. rewind-arena-builder append) 0u32)
  (let rewind-arena-selection ([] table rewind-arena-builder))
  (call (. arena rewind) arena-mark)
  (call show-stale-selection rewind-arena-selection)
  (blank)
  (let reset-builder (call (of ArrayBuilder U32) arena))
  (call (. reset-builder append) 1u32)
  (let reset-selection ([] table reset-builder))
  (call (. arena reset))
  (call show-stale-selection reset-selection)
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)

(fn-do + show-stale-selection _
  (param selection (of TableSelection Point U32))
  (let attempt
    (do-new
      (print (. ([] selection 0) x))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)
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

```brix
(import /core/table)
(blank)

(main
  (let rows (call (of Table Node) 4))
  (= ([] rows 0) (call (. Node small) 10 100))
  (= ([] rows 1) (call (. Node wide) 20 1 2 200 300))
  (= ([] rows 2) (call (. Node small) 30 700))
  (= ([] rows 3) (call (. Node wide) 40 3 4 800 900))
  (let ids (call (of Array U32) (list 2u32 1u32 2u32 3u32)))
  (let selected ([] rows ids))
  (blank)
  (for (in row selected)
    (print
      (match row
        (case (call .small small)
          (. small value)
        )
        (case (call .wide wide)
          (+ (. wide inline-a) (. wide external-b))
        )
      )
    )
  )
  (blank)
  (for (in row selected)
    (match row
      (case (call .small small)
        (+= (. small value) 1)
      )
      (case (call .wide wide)
        (= (. wide external-a) (+ (. wide external-a) (. wide external-b)))
      )
    )
  )
  (blank)
  (print (as ([] rows 2) Node))
  (print (as ([] rows 1) Node))
  (print (as ([] rows 3) Node))
)

(type + Syntax (is Tuple)
  (prop + source U32)
)

(type + Small (is Syntax)
  (prop + value U64 @hot)
)

(type + Wide (is Syntax)
  (prop + inline-a U64 @hot)
  (prop + inline-b U64 @hot)
  (prop + external-a U64)
  (prop + external-b U64)
)

(type + Node (is Union)
  (variant small Small)
  (variant wide Wide)
)
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

```brix
(import /core/table)
(import /core/arena)
(import /core/array-builder)
(blank)

(main
  (let arena (call Arena 8192))
  (finally
    (inline (call (. arena release)))
  )
  (let rows (call (of Table Node) arena 2))
  (call (. rows append) (call (. Node small) (tuple 1 2)))
  (call (. rows append) (call (. Node wide) (tuple 3 4) 4 5 6 7))
  (print (call snapshot (call NodeStore rows)))
)

(type + NodeStore
  (prop + nodes (of Table Node))
  (fn-do + This _
    (param nodes (of Table Node))
  )
)

(fn-do _ snapshot _
  (param store NodeStore)
  (let parts (call (of ArrayBuilder String)))
  (for (in node (. store nodes))
    (call (. parts append)
      (block
        (match node
          (case (call .small row)
            (str "row: " row)
          )
          (case (call .wide row)
            (str "row: " row)
          )
        )
      )
    )
  )
  (blank)
  (return (call (. "\n" join) (call (. parts finish-immutable))))
)

(type + Syntax (is Tuple)
  (prop + source (tuple (: start U64) (: end U64)))
)

(type + Small (is Syntax))

(type + Wide (is Syntax)
  (prop + inline-a U64 @hot)
  (prop + inline-b U64 @hot)
  (prop + external-a U64)
  (prop + external-b U64)
)

(type + Node (is Union)
  (variant small Small)
  (variant wide Wide)
)
```

```output
row: ((1, 2))
row: ((3, 4), 4, 5, 6, 7)
```

### copy-from replaces an Arena-backed table's rows with another table's rows, growing, shrinking, and copying union spill storage.

```brix
(import /core/arena)
(import /core/table)
(blank)

(main
  (let arena (call Arena 8192))
  (finally
    (inline (call (. arena release)))
  )
  (let live (call (of Table Point) arena 2))
  (let saved (call (of Table Point) arena 1))
  (call (. live append) (call Point 1 10))
  (call (. live append) (call Point 2 20))
  (call (. live append) (call Point 3 30))
  (call (. saved copy-from) live)
  (= (. ([] live 1) y) 99)
  (call (. live append) (call Point 4 40))
  (print (str "saved=" (. saved length) " live=" (. live length) " y=" (. ([] saved 1) y)))
  (call (. live copy-from) saved)
  (print (str "restored=" (. live length) " y=" (. ([] live 1) y) " x=" (. ([] live 2) x)))
  (call (. live copy-from) live)
  (print (str "self=" (. live length)))
  (let fixed (call (of Table Point) 1 (:with (call Point (+ each 7) 70))))
  (call (. saved copy-from) fixed)
  (print (str "fixed=" (. saved length) " " (as ([] saved 0) Point)))
  (blank)
  (let nodes (call (of Table Node) arena 1))
  (let backup (call (of Table Node) arena 1))
  (call (. nodes append) (call (. Node small) 1))
  (call (. nodes append) (call (. Node wide) 2 3 4 5 6))
  (call (. backup copy-from) nodes)
  (match ([] nodes 1)
    (case (call .small small)
      (+= (. small source) 0)
    )
    (case (call .wide wide)
      (= (. wide external-a) 500)
    )
  )
  (call (. nodes copy-from) backup)
  (print (as ([] nodes 1) Node))
  (print (as ([] backup 0) Node))
)

(type + Point (is Struct)
  (prop + x I64)
  (prop + y I64)
)

(type + Syntax (is Tuple)
  (prop + source U64)
)

(type + Small (is Syntax))

(type + Wide (is Syntax)
  (prop + inline-a U64 @hot)
  (prop + inline-b U64 @hot)
  (prop + external-a U64)
  (prop + external-b U64)
)

(type + Node (is Union)
  (variant small Small)
  (variant wide Wide)
)
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

```brix
(import /core/table)
(blank)

(main
  (let table (call (of Table ShadowRow) 3 (:with (call ShadowRow (as (+ each 5) U32)))))
  (print (call row-sum table))
  (print (call index-sum table))
)

(type + ShadowRow (is Struct)
  (prop + value U32)
)

(fn-do + row-sum U32
  (param rows (of Table ShadowRow))
  (let total U32 0)
  (let row ([] rows 0))
  (for (in row rows)
    (+= total (. row value))
  )
  (return (+ total (* (. row value) 100)))
)

(fn-do + index-sum U32
  (param rows (of Table ShadowRow))
  (let total U32 0)
  (let index U32 9)
  (for (in index (..< 0 (. rows length)))
    (+= total (. ([] rows index) value))
  )
  (return (+ total (* index 100)))
)
```

```output
518
918
```

### A row read through an index does not outlive an assignment to the index.

```brix
(import /core/table)
(blank)

(main
  (let table (call (of Table StepRow) 3 (:with (call StepRow (as (+ (* each 2) 7) I32)))))
  (print (call step table))
  (print (call walk table 3))
)

(type + StepRow (is Struct)
  (prop + offset I32)
)

(fn-do + step I32
  (param rows (of Table StepRow))
  (let index U32 0)
  (let first (. ([] rows index) offset))
  (+= index 1)
  (let second (. ([] rows index) offset))
  (= index (+ index 1))
  (return (+ (* first 10000) (* second 100) (. ([] rows index) offset)))
)

(fn-do + walk I32
  (param rows (of Table StepRow))
  (param count U32)
  (let total I32 0)
  (let index U32 0)
  (while (< index count)
    (let first (. ([] rows index) offset))
    (+= index 1)
    (if (< index count)
      (+= total (- (. ([] rows index) offset) first))
    )
  )
  (return total)
)
```

```output
70911
4
```

```brix
(import /core/table)
(blank)

(main
  (let rows (call (of Table Node) 2))
  (= ([] rows 0) (call (. Node low) 10 (call Pair 1 2)))
  (= ([] rows 1) (call (. Node high) 30 (call Pair 3 4)))
  (let reader (call Reader))
  (= (. reader offset) 100)
  (match ([] rows 0)
    (case (call .low low)
      (print (call (. reader total) low))
    )
    (default
      (print -1)
    )
  )
  (print (call (. reader pick) rows))
  (print (call free-total rows))
)

(fn-do + free-total I64
  (param rows (of Table Node))
  (match? ([] rows 0)
    (case (call .low low)
      (return (call sum low))
    )
  )
  (return -1)
)

(fn-expr + sum I64
  (param base Low)
  (+ (. base value) (. base pair left))
)

(type + Reader
  (prop + offset I64)
  (fn-expr + total I64
    (param base Low)
    (+ offset (. base value) (. base pair right))
  )
  (fn-do + pick I64
    (param rows (of Table Node))
    (match? ([] rows 1)
      (case (call .high high)
        (return (call total-high high))
      )
    )
    (return -1)
  )
  (fn-expr _ total-high I64
    (param base High)
    (+ offset (. base value) (. base pair left))
  )
)

(type + Pair (is Tuple)
  (prop + left I64)
  (prop + right I64)
)

(type + Base (is Tuple)
  (prop + value I64)
  (prop + pair Pair)
)

(type + Low (is Base))

(type + High (is Base))

(type + Node (is Union)
  (variant low Low)
  (variant high High)
)
```

```output
112
133
11
```

### Cases with the same field shape stay distinct when a table row is matched, and positional case fields read from the row.

```brix
(import /core/arena)
(import /core/table)
(blank)

(main
  (let arena (call Arena))
  (finally
    (inline (call (. arena release)))
  )
  (let build (call (of Table Shape) arena 4))
  (call (. build append) (call (. Shape wide) 1 7))
  (call (. build append) (call (. Shape tall) 2 9))
  (call (. build append) (call (. Shape flat) 3 1))
  (for (in index (..< 0 (. build length)))
    (print (call describe build index))
  )
  (let shapes (call (. build finish)))
  (for (in index (..< 0 (. shapes length)))
    (print (call name shapes index))
  )
  (print (call pair shapes))
)

(fn-do _ describe String
  (param shapes (of Table Shape))
  (param index Index)
  (match ([] shapes index)
    (case (call .tall _ size)
      (return (str "tall " size))
    )
    (case (call .wide _ size)
      (return (str "wide " size))
    )
    (case (call .flat id _)
      (return (str "flat " id))
    )
  )
)

(fn-expr _ name String
  (param shapes (of ImmutableTable Shape))
  (param index Index)
  (match ([] shapes (..< index (+ index 1)))
    (case (list (call .tall id _))
      (str "tall " id)
    )
    (case (list (call .wide id _))
      (str "wide " id)
    )
    (default
      "other"
    )
  )
)

(fn-expr _ pair String
  (param shapes (of ImmutableTable Shape))
  (match ([] shapes (..< 0 (. shapes length)))
    (case (list (call .tall _ first) (call .wide _ second) ...)
      (str "tall " first " then wide " second)
    )
    (case (list (call .wide _ first) (call .tall _ second) ...)
      (str "wide " first " then tall " second)
    )
    (default
      "other"
    )
  )
)

(type + Shape (is Union)
  (variant wide (tuple (: id U32) (: size U32)))
  (variant tall (tuple (: id U32) (: size U32)))
  (variant flat (tuple (: id U32) (: depth U8)))
)
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

```brix
(import /core/arena)
(import /core/table)
(blank)

(main
  (let arena (call Arena))
  (finally
    (inline (call (. arena release)))
  )
  (let table (call (of Table Entry) arena 2))
  (call (. table append) (call Entry 1 10))
  (call (. table append) (call Entry 2 20))
  (let early (call (. table view)))
  (for (in count (..< 0 40))
    (call (. table append) (call Entry (+ 3 (as count U32)) (+ 30 (as count U32))))
  )
  (let late (call (. table view)))
  (print (str (. early length) " " (. late length) " " (. table length)))
  (print (str (. ([] early 1) key) " " (. ([] early 1) value)))
  (print (str (. ([] late 1) key) " " (. ([] late 41) key) " " (. ([] late 41) value)))
  (call (. table append) (call Entry 99 990))
  (print (str (. late length) " " (. (call (. table view)) length) " " (. ([] (call (. table view)) 42) value)))
)

(type + Entry (is Struct)
  (prop + key U32)
  (prop + value U32)
)
```

```output
2 42 42
2 20
2 42 69
42 43 990
```

### An immutable row reports its position, whether it was indexed, iterated, or bound by a pattern.

```brix
(import /core/arena)
(import /core/table)
(blank)

(main
  (let arena (call Arena))
  (finally
    (inline (call (. arena release)))
  )
  (let build (call (of Table Part) arena 4))
  (call (. build append) (call (. Part leaf) 1 7))
  (call (. build append) (call (. Part leaf) 2 9))
  (call (. build append) (call (. Part pair) 3 0 2))
  (let parts (call (. build finish)))
  (print (str (. ([] parts 0) index) " " (. ([] parts 2) index)))
  (for (in part ([] parts (..< 1 3)))
    (print (. part index))
  )
  (print (call second parts ([] parts 2)))
)

(fn-expr _ second String
  (param parts (of ImmutableTable Part))
  (param part (of ImmutableTableRow Part))
  (match part
    (case (call .pair _ start count)
      (match ([] parts (..< start (+ start count)))
        (case (list _ held ...)
          (str "second child is row " (. held index))
        )
        (default
          "short"
        )
      )
    )
    (default
      "leaf"
    )
  )
)

(type + Part (is Union)
  (variant leaf (tuple (: id U32) (: value U32)))
  (variant pair (tuple (: id U32) (: start U32) (: count U32)))
)
```

```output
0 2
1
2
second child is row 1
```

### A row taken from a loop can be passed, stored, assigned, or returned where an optional row is expected.

```brix
(import /core/arena)
(import /core/table)
(blank)

(main
  (let arena (call Arena))
  (finally
    (inline (call (. arena release)))
  )
  (let build (call (of Table Entry) arena 4))
  (call (. build append) (call Entry 1 2))
  (call (. build append) (call Entry 3 4))
  (call (. build append) (call Entry 5 6))
  (let entries (call (. build finish)))
  (for (in entry ([] entries (..< 0 2)))
    (call show entry)
  )
  (let kept (list (? (of ImmutableTableRow Entry))))
  (for (in entry entries)
    (+= kept entry)
  )
  (for (in entry kept)
    (call show entry)
  )
  (call show (call last-below entries 4))
  (call show (call last-below entries 0))
  (call show (call first-above entries 3))
  (call show (call first-above entries 9))
)

(fn-do _ show _
  (param entry (? (of ImmutableTableRow Entry)))
  (if entry
    (print (str (. entry key) " " (. entry value)))
    (else
      (print "none")
    )
  )
)

(fn-do _ last-below (? (of ImmutableTableRow Entry))
  (param entries (of ImmutableTable Entry))
  (param limit U32)
  (let found (? (of ImmutableTableRow Entry)) null)
  (for (in entry entries)
    (if (< (. entry key) limit)
      (= found entry)
    )
  )
  (return found)
)

(fn-do _ first-above (? (of ImmutableTableRow Entry))
  (param entries (of ImmutableTable Entry))
  (param limit U32)
  (for (in entry entries)
    (return-if entry (if (> (. entry key) limit)))
  )
  (return null)
)

(type + Entry (is Struct)
  (prop + key U32)
  (prop + value U32)
)
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

```brix
(import /core/table)
(blank)

(main
  (let rows (call (of Table Point) 1 (:with (call Point (as (+ each 7) I32) (as (+ each 9) I32)))))
  (for (in row rows)
    (print (call sum-row row true))
    (print (call sum-row row false))
  )
)

(fn-do + sum-row I32
  (param row (of TableRow Point))
  (param use-x Bool)
  (let first (if-expr use-x (. row x) (. row y)))
  (return (+ first (. row y)))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
16
18
```

```brix
(import /core/arena)
(import /core/table)
(import /core/array-builder)
(blank)

(main
  (let arena (call Arena 8192))
  (finally
    (inline (call (. arena release)))
  )
  (let rows (call (of Table Point) arena))
  (let values (call (of ArrayBuilder U64) arena))
  (call (. rows append) (call Point 7u64 9u64))
  (call (. values append) 11u64)
  (call fail-rows rows (. Size max))
  (call fail-rows rows (as 1152921504606846976 Size))
  (call fail-values values (. Size max))
  (call fail-values values (as 1152921504606846976 Size))
  (call (. rows append) (call Point 13u64 15u64))
  (call (. values append) 17u64)
  (print (as ([] rows 0) Point))
  (print (as ([] rows 1) Point))
  (print (tuple ([] values 0) ([] values 1)))
  (print (tuple (. rows length) (. values length)))
)

(fn-do + fail-rows _
  (param rows (of Table Point))
  (param needed Size)
  (let attempt
    (do-new
      (call (. rows reserve) needed)
      (on-failure failure
        (print (str (. failure message) ": " (?. (. failure source) line) ":" (?. (. failure source) column)))
      )
    )
  )
  (call (. attempt wait))
)

(fn-do + fail-values _
  (param values (of ArrayBuilder U64 ArenaBacking))
  (param needed Size)
  (let attempt
    (do-new
      (call (. values reserve) needed)
      (on-failure failure
        (print (str (. failure message) ": " (?. (. failure source) line) ":" (?. (. failure source) column)))
      )
    )
  )
  (call (. attempt wait))
)

(type + Point (is Struct)
  (prop + x U64)
  (prop + y U64)
)
```

```output
Table x capacity overflow: 33:7
Arena allocation failed while growing Table: 33:7
ArrayBuilder capacity overflow: 47:7
Arena allocation failed while growing ArrayBuilder: 47:7
(7, 9)
(13, 15)
(11, 17)
(2, 2)
```

```brix crash
(import /core/arena)
(import /core/table)
(import /core/array-builder)
(blank)

(main
  (let arena (call Arena 8192))
  (finally
    (inline (call (. arena release)))
  )
  (let rows (call (of Table Point) arena))
  (let values (call (of ArrayBuilder U64) arena))
  (call (. rows append) (call Point 7u64 9u64))
  (call (. values append) 11u64)
  (call fail-rows rows (. Size max))
  (call fail-rows rows (as 1152921504606846976 Size))
  (call fail-values values (. Size max))
  (call fail-values values (as 1152921504606846976 Size))
  (call (. rows append) (call Point 13u64 15u64))
  (call (. values append) 17u64)
  (print (as ([] rows 0) Point))
  (print (as ([] rows 1) Point))
  (print (tuple ([] values 0) ([] values 1)))
  (print (tuple (. rows length) (. values length)))
)

(fn-do + fail-rows _
  (param rows (of Table Point))
  (param needed Size)
  (call (. rows reserve) needed)
)

(fn-do + fail-values _
  (param values (of ArrayBuilder U64 ArenaBacking))
  (param needed Size)
  (call (. values reserve) needed)
)

(type + Point (is Struct)
  (prop + x U64)
  (prop + y U64)
)
```

```stderr
Unhandled panic: Table x capacity overflow
<no file:31>

Task 1

Stack trace:
  0  __source__:fail-rows no file:31:3
  1  __source__:main no file:16:3
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

```brix
(import /BxGC/BxGC.h)
(import /core/table)
(blank)

(main
  (let rows (call (of Table Node) 1 (:with (=> (index) (call (. Node wide) 1 10 20 30 40)))))
  (blank)
  (match ([] rows 0)
    (case (call .small small)
      (print (. small value))
    )
    (case (call .wide wide)
      (print (call collect-table-case wide))
    )
  )
)

(fn-do + collect-table-case U64
  (param wide Wide)
  (call C:brixGCCollect)
  (return (. wide external-a))
)

(type + Syntax (is Tuple)
  (prop + source U64)
)

(type + Small (is Syntax)
  (prop + value U64)
)

(type + Wide (is Syntax)
  (prop + inline-a U64 @hot)
  (prop + inline-b U64 @hot)
  (prop + external-a U64)
  (prop + external-b U64)
)

(type + Node (is Union)
  (variant small Small)
  (variant wide Wide)
)
```

```output
30
```

```brix crash
(import /core/table)
(blank)

(main
  (let rows (call (of Table Node) 2))
  (= ([] rows 0) (call (. Node small) 10 2))
  (match ([] rows 0)
    (case (call .small small)
      (print (call (. small pick) 9))
    )
    (case .wide
      (print "no")
    )
  )
)

(type + Syntax (is Tuple)
  (prop + source U32)
)

(type + Small (is Syntax)
  (prop + value U32)
)

(type + Wide (is Syntax)
  (prop + target U32)
)

(type + Node (is Union)
  (variant small Small)
  (variant wide Wide)
)

(extend + Small
  (fn-do + pick String
    (param at U32)
    (let part (str (. this value)))
    (throw (call Panic (str "too far " at)) (if (> at 5)))
    (return (str part " " (. this source)))
  )
)
```

```stderr
Unhandled panic: too far 9
<no file:39>

Task 1

Stack trace:
  0  __source__:pick no file:39:5
  1  __source__:pick-table-row-receiver-bridge no file:36:3
  2  __source__:main no file:10:14
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

```brix
(import /core/table)
(blank)

(main
  (let rows (call (of Table Node) 2))
  (= ([] rows 0) (call (. Node small) 10 2))
  (= ([] rows 1) (call (. Node wide) 20 3))
  (print (is ([] rows 0) .small))
  (print (is ([] rows 1) .small))
  (let held (? (of TableRow Node)) ([] rows 1))
  (print (is held .wide))
  (if held
    (print (is held .wide))
  )
)

(type + Syntax (is Tuple)
  (prop + source U32)
)

(type + Small (is Syntax)
  (prop + value U32)
)

(type + Wide (is Syntax)
  (prop + target U32)
)

(type + Node (is Union)
  (variant small Small)
  (variant wide Wide)
)
```

```output
true
false
false
true
```

```brix
(import /core/table)
(blank)

(main
  (let rows (call (of Table Node) 2))
  (= ([] rows 0) (call (. Node small) 10 2))
  (= ([] rows 1) (call (. Node wide) 20 3))
  (for (in row rows)
    (match? row
      (case (call .small small)
        (print (call (. small pick) rows))
      )
    )
  )
)

(fn-do + overwrite _
  (param rows (of Table Node))
  (= ([] rows 0) (call (. Node small) 99 77))
)

(extend + Small
  (fn-do + pick String
    (param rows (of Table Node))
    (let first (. this value))
    (call overwrite rows)
    (return (str first " " (. this value)))
  )
)

(type + Syntax (is Tuple)
  (prop + source U32)
)

(type + Small (is Syntax)
  (prop + value U32)
)

(type + Wide (is Syntax)
  (prop + target U32)
)

(type + Node (is Union)
  (variant small Small)
  (variant wide Wide)
)
```

```output
2 2
```

```brix crash
(import /core/table)
(import /core/arena)
(blank)

(main
  (let arena (call Arena 8192))
  (finally
    (inline (call (. arena release)))
  )
  (let rows (call (of Table Node) arena 1))
  (call (. rows append) (call (. Node small) 10 2))
  (let row ([] rows 0))
  (call (. arena reset))
  (print (. row cheap))
)

(extend + Syntax
  (fn-expr + cheap U32 :get
    7
  )
)

(type + Syntax (is Tuple)
  (prop + source U32)
)

(type + Small (is Syntax)
  (prop + value U32)
)

(type + Wide (is Syntax)
  (prop + target U32)
)

(type + Node (is Union)
  (variant small Small)
  (variant wide Wide)
)
```

```stderr
Unhandled panic: TableRow is stale after Arena reset
<no file:15>

Task 1

Stack trace:
  0  __source__:union-dispatch-1 no file:15:10
  1  __source__:main no file:15:10
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

```brix crash
(import /core/table)
(blank)

(main
  (let rows (call (of Table Node) 2))
  (= ([] rows 0) (call (. Node small) 10 2))
  (= ([] rows 1) (call (. Node wide) 20 3))
  (print (call (. ([] rows 0) check) 1))
  (call (. ([] rows 0) bump) 3)
  (print (as ([] rows 0) Node))
  (print (call (. ([] rows 1) check) 9))
)

(extend + Small
  (fn-do + bump _
    (param by I32)
    (+= value by)
  )
  (fn-do + check String
    (param limit I32)
    (return (str "small " value))
  )
)

(extend + Wide
  (fn-do + bump _
    (param by I32)
    (+= target by)
  )
  (fn-do + check String
    (param limit I32)
    (throw (call Panic (str "over " limit)) (if (> limit 5)))
    (return (str "wide " target))
  )
)

(type + Syntax (is Tuple)
  (prop + source U32)
)

(type + Small (is Syntax)
  (prop + value I32)
)

(type + Wide (is Syntax)
  (prop + target I32)
)

(type + Node (is Union)
  (variant small Small)
  (variant wide Wide)
)
```

```stderr
Unhandled panic: over 9
<no file:33>

Task 1

Stack trace:
  0  __source__:check no file:33:5
  1  __source__:check-table-row-receiver-bridge no file:31:3
  2  __source__:union-dispatch-1 no file:9:10
  3  __source__:main no file:12:10
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

```brix crash
(import /core/table)
(blank)

(main
  (let rows (call (of Table Node) 2))
  (= ([] rows 0) (call (. Node small) 10 2))
  (match? ([] rows 0)
    (case (call .small small)
      (print (call (. small pick) 9))
    )
  )
)

(extend + Small
  (fn-do + pick String
    (param at U32)
    (let first (. this value))
    (let made (call Small 1 2))
    (throw (call Panic (str "p " at " " (. made value))) (if (> at 5)))
    (return (str first " " (. this source)))
  )
)

(type + Syntax (is Tuple)
  (prop + source U32)
)

(type + Small (is Syntax)
  (prop + value U32)
)

(type + Wide (is Syntax)
  (prop + target U32)
)

(type + Node (is Union)
  (variant small Small)
  (variant wide Wide)
)
```

```stderr
Unhandled panic: p 9 2
<no file:20>

Task 1

Stack trace:
  0  __source__:pick no file:20:5
  1  __source__:pick-table-row-receiver-bridge no file:16:3
  2  __source__:main no file:10:14
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

```brix crash
(import /core/table)
(blank)

(main
  (let filled
    (call (of Table Point) 3
      (:with
        (=> (index) :do
          (block
            (throw (call Panic (str "bad " index)) (if (== index 2)))
            (return (call Point (as index I32) 0))
          )
        )
      )
    )
  )
  (print (. filled length))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```stderr
Unhandled panic: bad 2
<no file:11>

Task 1

Stack trace:
  0  __source__:filled no file:11:13
  1  __source__:filledCasting no file:8:7
  2  core/table:Table:Table core/table.bx
  3  __source__:main no file:7:5
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

```brix
(import /core/table)
(blank)

(main
  (= ([] (. store rows) 0) (call (. Node small) 10 1))
  (= ([] (. store rows) 1) (call (. Node wide) 20 2))
  (print (. ([] (. store rows) 0) twice))
  (= ([] (. store rows) 0) (call (. Node small) 30 3))
  (match? ([] (. store rows) 0)
    (case (call .small small)
      (print (call (. small again)))
    )
  )
)

(blank)
(prop + store _ (= (call Store)))

(type + Store
  (prop + rows _ (= (call (of Table Node) 2)))
)

(fn-do + mutate U32
  (= ([] (. store rows) 0) (call (. Node small) 99 9))
  (return 0)
)

(extend + Syntax
  (fn-expr + twice String :get
    (str (. this source) " " (call mutate) " " (. this source))
  )
  (fn-do + again String
    (return (str (. this source) " " (call mutate) " " (. this source)))
  )
)

(type + Syntax (is Tuple)
  (prop + source U32)
)

(type + Small (is Syntax)
  (prop + value U64)
)

(type + Wide (is Syntax)
  (prop + target U64)
)

(type + Node (is Union)
  (variant small Small)
  (variant wide Wide)
)
```

```output
10 0 99
30 0 99
```

### A table an arena holds may have union rows whose cases hold managed fields, and the `ImmutableTable` it finishes as is made over its storage, which is not laid out again.

```brix
(import /core/arena)
(import /core/table)
(blank)

(main
  (let arena (use (call Arena)))
  (let table (call (of Table Value) arena))
  (call (. table append) (call Plain 4))
  (call (. table append) (call Named "five"))
  (let rows (call (. table finish)))
  (print (str "rows " (. rows length)))
)

(type + Named (is Tuple)
  (prop + text String)
)

(type + Plain (is Tuple)
  (prop + value I32)
)

(type + Value (is Union)
  (variant named Named)
  (variant plain Plain)
)
```

```output
rows 2
```

### An optional field of the case of a row a match finds, once a test finds it holds a value, is read as what it holds.

```brix
(import /core/arena)
(import /core/table)
(blank)

(main
  (let arena (use (call Arena)))
  (let table (call (of Table Shape) arena))
  (call (. table append) (call Square "box"))
  (call (. table append) (call Square null))
  (call (. table append) (call Dot 1))
  (let rows (call (. table finish)))
  (print (str (call is-box rows 0) " " (call is-box rows 1) " " (call is-box rows 2)))
)

(fn-expr + is-box Bool
  (param rows (of ImmutableTable Shape))
  (param index U32)
  (match ([] rows index)
    (case (call .square row)
      (case? (not (. row name))
        false
      )
      (default
        (== (. row name) "box")
      )
    )
    (default
      false
    )
  )
)

(type + Square (is Tuple)
  (prop + name (? String))
)

(type + Dot (is Tuple)
  (prop + size U32)
)

(type + Shape (is Union)
  (variant square Square)
  (variant dot Dot)
)
```

```output
true false false
```

### A field of a row named as a private member of the table's row is read from the row, as no module but the table's may use that member.

```brix
(import /core/arena)
(import /core/table)
(blank)

(main
  (let arena (use (call Arena)))
  (let table (call (of Table Grid) arena))
  (call (. table append) (call Grid 3 4))
  (call (. table append) (call Grid 5 6))
  (print (str (. ([] table 0) columns) " " (. ([] table 1) columns) " " (. ([] table 1) rows)))
)

(type + Grid (is Struct)
  (prop + columns U32)
  (prop + rows U32)
)
```

```output
3 5 6
```

### An assignment to a field of a table's row gives the value stored, so it may be an arm's value in a match or a choice.

```brix
(import /core/table)
(blank)

(main
  (let table (call (of Table Flags) 1))
  (call set-flag table .flag-a)
  (call set-flag table .flag-c)
  (print (str (. ([] table 0) a) " " (. ([] table 0) b) " " (. ([] table 0) c)))
  (print (str (call pick table true) " " (call pick table false) " " (. ([] table 0) count)))
)

(type + Flags (is Struct)
  (prop + a Bool)
  (prop + b Bool)
  (prop + c Bool)
  (prop + count U32)
)

(type + Flag (is U32)
  (variant flag-a (= 0))
  (variant flag-b (= 1))
  (variant flag-c (= 2))
)

(fn-do + set-flag _
  (param table (of Table Flags))
  (param flag Flag)
  (match flag
    (case= .flag-a
      (= (. ([] table 0) a) true)
    )
    (case= .flag-b
      (= (. ([] table 0) b) true)
    )
    (default
      (= (. ([] table 0) c) true)
    )
  )
)

(fn-expr + pick U32
  (param table (of Table Flags))
  (param wide Bool)
  (case? wide
    (= (. ([] table 0) count) 40)
  )
  (default
    (= (. ([] table 0) count) 2)
  )
)
```

```output
true false true
40 2 2
```

### A table a property holds is finished as a local one is, and is empty after it.

```brix
(import /core/table)
(import /core/arena)
(blank)

(main
  (let arena (call Arena 8192))
  (finally
    (inline (call (. arena release)))
  )
  (let keeper (call Keeper arena))
  (call (. keeper rows append) (call Point 1 2))
  (call (. keeper rows append) (call Point 3 4))
  (let fixed (call (. keeper rows finish)))
  (print (str (. fixed length) " " (. keeper rows length)))
  (call (. keeper add) 5)
  (let again (call (. keeper seal)))
  (print (str (. again length) " " (. keeper rows length) " " (. ([] again 0) x)))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)

(type + Keeper
  (prop + rows (of Table Point))
  (fn-do + This _
    (param arena Arena)
    (lazy rows (call (of Table Point) arena 1))
  )
  (fn-do + add _
    (param x Int)
    (= rows (call (of Table Point) (call Arena 1024) 1))
    (call (. rows append) (call Point x x))
  )
  (fn-do + seal (of ImmutableTable Point)
    (return (call (. rows finish)))
  )
)
```

```output
2 0
1 0 5
```

### A loop over a range of a table's rows reads the rows the range named while the loop appends more.

```brix
(import /core/arena)
(import /core/table)
(blank)

(main
  (let arena (call Arena))
  (let cases (call (of Table Case) arena 4))
  (call (. cases append) (call Case 1 2))
  (call (. cases append) (call Case 3 4))
  (call (. cases append) (call Case 5 6))
  (let total 0)
  (for (in fact ([] cases (..< 1 3)))
    (+= total (. fact a))
    (call (. cases append) (call Case 7 8))
  )
  (print (str "direct " total " " (. cases length)))
)

(type + Case (is Struct)
  (prop + a Int)
  (prop + b Int)
)
```

```output
direct 8 5
```

### An optional field of a table's row is tested and read as a local's is.

```brix
(import /core/arena)
(import /core/atom-pool)
(import /core/table)
(blank)

(main
  (let pool (call AtomPool))
  (let arena (call Arena))
  (let cases (call (of Table Case) arena 4))
  (call (. cases append) (call Case (call (. pool store) "first") 1))
  (call (. cases append) (call Case null 2))
  (for (in offset (..< 0 (. cases length)))
    (let fact ([] cases offset))
    (if (and (. fact name) (== (!! (. fact name)) (call (. pool store) "first")))
      (print (str "match " (. fact value)))
    )
    (let name (if-expr (. fact name) (. (!! (. fact name)) text) "-"))
    (print name)
  )
)

(type + Case (is Struct)
  (prop + name (? Atom))
  (prop + value Int)
)
```

```output
match 1
first
-
```

### A method called on a field of a union case of a table row is called on that field.

```brix
(import /core/arena)
(import /core/atom-pool)
(import /core/table)
(blank)

(main
  (print (call check))
)

(type + Base (is Tuple)
  (prop + id U32)
)

(type + Word (is Base)
  (prop + text String)
)

(type + Group (is Base)
  (prop + count U32)
)

(type + Node (is Union)
  (variant atom Word)
  (variant list Group)
)

(fn-do _ check String
  (let arena (call Arena))
  (let names (call AtomPool))
  (let table (call (of Table Node) arena 16))
  (call (. table append) (call (. Node atom) 1 (. (call (. names store) "@x") text)))
  (call (. table append) (call (. Node list) 2 3))
  (call (. table append) (call (. Node atom) 3 (. (call (. names store) "y") text)))
  (let nodes (call (. table view)))
  (let shown (list String))
  (for (in position (..< 0 3))
    (+= shown (str (call is-attribute ([] nodes position))))
  )
  (return (call (. " " join) shown))
)

(fn-expr _ is-attribute Bool
  (param node (of ImmutableTableRow Node))
  (match node
    (case (call .atom _ text)
      (call (. text starts-with) "@")
    )
    (default
      false
    )
  )
)
```

```output
true false false
```

### A getter of a union case that a row of a table holds is the case's own extension's, over its base's.

```brix
(import /core/table)
(blank)

(main
  (let rows (call (of Table Piece) 3))
  (= ([] rows 0) (call (. Piece plain) 1))
  (= ([] rows 1) (call (. Piece stop) 2))
  (for (in position (..< 0 2))
    (call print (. ([] rows position) label))
  )
  (match ([] rows 1)
    (case (call .stop stop)
      (call print (. stop label))
    )
    (case .plain
      (call print "no")
    )
  )
)

(type + Empty (is Tuple)
  (prop + body U32)
)

(type + Stop (is Empty))

(type + Piece (is Union)
  (variant plain Empty)
  (variant stop Stop)
)

(extend + Empty
  (fn-expr + label String :get
    (str "empty " body)
  )
)

(extend + Stop
  (fn-expr + label String :get
    (str "stop " body)
  )
)
```

```output
empty 1
stop 2
stop 2
```

### A member with a loop that may move a row's storage is run through the case gathered from the row.

```brix crash
(import /core/table)
(blank)

(main
  (let rows (call (of Table Node) 2))
  (= ([] rows 0) (call (. Node small) 10 2))
  (match ([] rows 0)
    (case (call .small small)
      (call print (call (. small pick) 9))
    )
    (case .wide
      (call print "no")
    )
  )
)

(type + Syntax (is Tuple)
  (prop + source U32)
)

(type + Small (is Syntax)
  (prop + value U32)
)

(type + Wide (is Syntax)
  (prop + target U32)
)

(type + Node (is Union)
  (variant small Small)
  (variant wide Wide)
)

(extend + Small
  (fn-do + pick String
    (param at U32)
    (let parts (list String))
    (for (in index (..< 0 at))
      (+= parts (str (+ (. this value) index)))
    )
    (throw (call Panic (str "too far " at)) (if (> at 5)))
    (return (str (. parts length) " " (. this source)))
  )
)
```

```stderr
Unhandled panic: too far 9
<no file:42>

Task 1

Stack trace:
  0  __source__:pick no file:42:5
  1  __source__:pick-table-row-receiver-bridge no file:36:3
  2  __source__:main no file:10:19
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### A loop over a table whose body grows it reads and writes the table's storage as it is after the growth, and visits only the rows the table had when the loop began.

```brix
(import /core/table)
(import /core/arena)
(blank)

(main
  (let arena (call Arena 16384))
  (finally
    (inline (call (. arena release)))
  )
  (let rows (call (of Table Point) arena 2))
  (call (. rows append) (call Point 1 10))
  (call (. rows append) (call Point 2 20))
  (let turns 0)
  (for (in row rows)
    (+= turns 1)
    (call (. rows append) (call Point 9 90))
    (call (. rows reserve) (* 64 turns))
    (+= (. row x) 100)
  )
  (print (str "turns " turns " length " (. rows length)))
  (for (in row rows)
    (print (as row Point))
  )
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
turns 2 length 4
(101, 10)
(102, 20)
(9, 90)
(9, 90)
```

### A column of a table row read in one branch is read again in the other branch, not reused.

```brix
(import /core/table)
(blank)

(main
  (let rows (call (of Table Point) 1 (:with (call Point (as (+ each 7) I32) (as (+ each 9) I32)))))
  (for (in row rows)
    (print (call sum-row row true))
    (print (call sum-row row false))
  )
)

(fn-do + sum-row I32
  (param row (of TableRow Point))
  (param use-x Bool)
  (let first (if-expr use-x (. row x) (. row y)))
  (return (+ first (. row y)))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
16
18
```

### An unsuffixed number in one branch takes the type of a union table row's common field in the other.

```brix
(import /core/table)
(import /core/arena)
(blank)

(main
  (let arena (call Arena 8192))
  (finally
    (inline (call (. arena release)))
  )
  (let rows (call (of Table Node) arena))
  (call (. rows append) (call (. Node word) (call Word 5 7)))
  (let position 1)
  (let above U32 (if-expr (> position 0) (. ([] rows 0) id) 0))
  (print above)
)

(type + Base (is Tuple)
  (prop + id U32)
)

(type + Word (is Base)
  (prop + text U32)
)

(type + Group (is Base)
  (prop + count U32)
)

(type + Node (is Union)
  (variant word Word)
  (variant group Group)
)
```

```output
5
```

### A row of a table passed to a function beside a call that takes the same row is passed whole.

```brix
(import /core/arena)
(import /core/table)
(blank)

(main
  (let arena (call Arena))
  (finally
    (inline (call (. arena release)))
  )
  (let rows (call (of Table Point) arena 2))
  (call (. rows append) (call Point 1))
  (call (. rows append) (call Point 2))
  (call show ([] (call (. rows view)) (..< 0 2)))
)

(type + Point (is Struct)
  (prop + x U32)
)

(fn-do + show _
  (param span (of ImmutableTableSpan Point))
  (call named ([] span 1) (call same ([] span 1) span))
)

(fn-expr + same (of ImmutableTableSpan Point)
  (param point (of ImmutableTableRow Point))
  (param span (of ImmutableTableSpan Point))
  span
)

(fn-do + named _
  (param point (of ImmutableTableRow Point))
  (param span (of ImmutableTableSpan Point))
  (print (str (. point x) ":" (. span length)))
)
```

```output
2:2
```

### A comprehension over a span of a table yields the rows of the span.

```brix
(import /core/table)
(blank)

(main
  (let table (call (of Table LoopRow) 3 (:with (call LoopRow (as (+ each 5) U32) 2))))
  (print (. (call spanned table) length))
)

(type + LoopRow (is Struct)
  (prop + value U32)
  (prop + count U32)
)

(fn-expr + spanned (list U32)
  (param table (of Table LoopRow))
  (in (. each value) ([] table (..< 1 3)))
)
```

```output
2
```

### A comprehension over a table finds its rows where its loop begins, not where it is written.

```brix
(import /core/table)
(import /core/arena)
(blank)

(main
  (let arena (call Arena 8192))
  (finally
    (inline (call (. arena release)))
  )
  (let rows (call (of Table Point) arena 2))
  (call (. rows append) (call Point 1 10))
  (call (. rows append) (call Point 2 20))
  (let xs (in (. each x) rows))
  (let ys (in (. each y) ([] rows (..< 1 2))))
  (call (. rows append) (call Point 3 30))
  (call (. rows reserve) 64)
  (print (call List xs))
  (print (call List ys))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
1, 2, 3
20
```
