# Ranges

```brix
(main
  (let range (..< 0 1))
  (print (. range start))
  (print (. range end))
  (print (. range increment))
  (print (. range is-through))
)
```

```output
0
1
1
false
```

```brix
(main
  (let range (..< 0 1 (by: 0.25)))
  (print (. range start))
  (print (. range end))
  (print (. range increment))
  (print (. range is-through))
)
```

```output
0
1
0.25
false
```

```brix
(main
  (let range (.. 0 1))
  (print (. range start))
  (print (. range end))
  (print (. range increment))
  (print (. range is-through))
)
```

```output
0
1
1
true
```

```brix
(main
  (for (in i (..< 0 3))
    (print i)
  )
)
```

```output
0
1
2
```

```brix
(main
  (for (in i (.. 0 3))
    (print i)
  )
)
```

```output
0
1
2
3
```

```brix
(main
  (for (in i (..< 3 0))
    (print i)
  )
)
```

```output
3
2
1
```

```brix
(main
  (for (in i (.. 3 0))
    (print i)
  )
)
```

```output
3
2
1
0
```

```brix
(main
  (for (in i (.. 126i8 127i8))
    (print i)
  )
)
```

```output
126
127
```

```brix
(main
  (for (in i (.. -127i8 -128i8))
    (print i)
  )
)
```

```output
-127
-128
```

```brix
(main
  (print
    (call (. ", " join)
      (block
        (spread (..< 1 5))
      )
    )
  )
)
```

```output
1, 2, 3, 4
```

```brix
(main
  (for (in i (..< 0 1 (by: 0.25)))
    (print i)
  )
)
```

```output
0
0.25
0.5
0.75
```

```brix
(main
  (for (in i (.. 0 1 (by: 0.25)))
    (print i)
  )
)
```

```output
0
0.25
0.5
0.75
1
```

```brix
(main
  (for (in i (.. 1 0 (by: 0.25)))
    (print i)
  )
)
```

```output
1
0.75
0.5
0.25
0
```

### A range runs from its start toward its end, so a step written with a sign counts by its size.

```brix
(main
  (for (in n (.. 10 0 (by: -5)))
    (print n)
  )
  (let rising (.. 0 10 (by: -5)))
  (print (. rising length))
  (for (in x (.. 1.0 0.0 (by: -0.5)))
    (print x)
  )
)
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

```brix
(main
  (let range (..< 3 0))
  (for (in i (call (. range forward)))
    (print i)
  )
)
```

```output
1
2
3
```

```brix
(main
  (let range (.. 3 0))
  (for (in i (call (. range forward)))
    (print i)
  )
)
```

```output
0
1
2
3
```

```brix
(main
  (let range (.. 1 0 (by: 0.25)))
  (for (in i (call (. range forward)))
    (print i)
  )
)
```

```output
0
0.25
0.5
0.75
1
```

```brix
(main
  (let range (..< 1 0 (by: 0.25)))
  (for (in i (call (. range forward)))
    (print i)
  )
)
```

```output
0.25
0.5
0.75
1
```

```brix
(main
  (let iter (as (..< 0 3) (stream Int)))
  (for (in a iter)
    (print a)
  )
)
```

```output
0
1
2
```

## Relative positions

```brix
(main
  (let range (..< (. Position first) (. Position end)))
  (for (in i (call (. range normalize) (..< 0 3)))
    (print i)
  )
)
```

```output
0
1
2
```

```brix
(main
  (let range (..< (. Position first) (. Position last)))
  (for (in i (call (. range normalize) (..< 0 3)))
    (print i)
  )
)
```

```output
0
1
```

```brix
(main
  (let range (.. (. Position first) (. Position last)))
  (for (in i (call (. range normalize) (..< 0 3)))
    (print i)
  )
)
```

```output
0
1
2
```

```brix
(main
  (let range (..< (. Position first) (. Position end)))
  (for (in i (call (. range normalize) (..< 0 3)))
    (print i)
  )
)
```

```output
0
1
2
```

```brix
(main
  (let range (.. (. Position first) (. Position end)))
  (for (in i (call (. range normalize) (..< 0 3)))
    (print i)
  )
)
```

```output
0
1
2
3
```

```brix
(main
  (let range (.. (. Position first) (. Position end) (by: 2)))
  (for (in i (call (. range normalize) (..< 0 5)))
    (print i)
  )
)
```

```output
0
2
4
```

```brix
(main
  (let base (..< 0 1 (by: 0.25)))
  (let range (.. (. Position end) (. Position first)))
  (for (in i (call (. range normalize) base))
    (print i)
  )
)
```

```output
4
3
2
1
0
```

```brix
(main
  (let base (..< 0 1 (by: 0.25)))
  (let range (..< (. Position end) (. Position first)))
  (for (in i (call (. range normalize) base))
    (print i)
  )
)
```

```output
4
3
2
1
```

```brix
(main
  (let base (..< 0 1 (by: 0.25)))
  (let range (.. (. Position first) (. Position middle)))
  (for (in i (call (. range normalize) base))
    (print ([] base i))
  )
)
```

```output
0
0.25
0.5
```

```brix
(main
  (print (. (..< 0 17 (by: 80)) length))
  (print (call List (..< 0 17 (by: 80))))
  (print (. (..< 0 17 (by: 8)) length))
  (print (call List (..< 0 17 (by: 8))))
  (print (call List (..< 17 0 (by: 8))))
  (print (. (..< 0 16 (by: 8)) length))
  (print (call List (..< 0 16 (by: 8))))
  (print (call List (.. 0 16 (by: 8))))
  (print (call List (.. 0 17 (by: 8))))
  (print (. (..< 0 0 (by: 8)) length))
  (print (call List (.. 0 0 (by: 8))))
  (print (. (.. 0u64 (- (. U64 max) 1) (by: (. U64 max))) length))
  (print (call List (..< 0.0 1.125 (by: 0.5))))
  (print (call List (.. 0.0 1.125 (by: 0.5))))
  (print (call List (.. 0.0 1.0 (by: 0.5))))
  (print (call List (..< 1.125 0.0 (by: 0.5))))
)
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

```brix crash
(main
  (for (in i (..< 0 3))
    (= i 5)
  )
)
```

```output
Exception: Illegal to update const variables
<no file:4>
        (= i 5)
        ^^^^^^^
```

### The item of a loop over a range cannot be assigned by an operator.

```brix crash
(main
  (for (in i (..< 0 3))
    (+= i 5)
  )
)
```

```output
Exception: Illegal to update const variables
<no file:4>
        (+= i 5)
        ^^^^^^^^
```

### A range loop counts up or down, by the order of its ends. `..` takes its end and `.. <` stops before it.

```brix
(main
  (for (in i (.. 0 3))
    (print i)
  )
  (for (in i (.. 3 0))
    (print i)
  )
  (for (in i (..< 0 3))
    (print i)
  )
  (for (in i (..< 3 0))
    (print i)
  )
  (for (in i (.. 5 5))
    (print i)
  )
  (for (in i (..< 5 5))
    (print i)
  )
  (for (in i (.. -2 1))
    (print i)
  )
  (print "end")
)
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

```brix
(main
  (for (in i (.. 126i8 127i8))
    (print i)
  )
  (for (in i (.. -127i8 -128i8))
    (print i)
  )
  (for (in i (.. 254u8 255u8))
    (print i)
  )
  (for (in i (.. 1u8 0u8))
    (print i)
  )
  (for (in i (.. 2147483646 2147483647))
    (print i)
  )
  (for (in i (.. 18446744073709551614u64 18446744073709551615u64))
    (print i)
  )
  (for (in i (.. 1u64 0u64))
    (print i)
  )
)
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

```brix
(main
  (let n 3)
  (for (in i (..< 0 n))
    (= n 10)
    (print i)
  )
  (print n)
  (for (in i (..< 0u8 2))
    (print i)
  )
  (for (in j (..< 5000000000 5000000002))
    (print j)
  )
  (for (in k (.. 1i8 2u8))
    (print k)
  )
  (let a 2)
  (for (in i (.. (* a 2) (* a 3)))
    (print i)
  )
)
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

```brix
(main
  (let n 3000000000u32)
  (let passes 0)
  (for (in i (..< 0 n))
    (print i)
    (+= passes 1)
    (if (== passes 3)
      (break)
    )
  )
  (let span (..< 0 n))
  (print (. span end))
)
```

```output
0
1
2
3000000000
```

### The clauses of a range loop: `when` skips a pass, and `while` and `until` end the loop.

```brix
(main
  (for (in i (..< 0 10) (when (== (% i 3) 0)))
    (print i)
  )
  (for (in i (.. 0 5) (while (< i 2)))
    (print i)
  )
  (for (in i (.. 0 5) (until (== i 3)))
    (print i)
  )
  (for (in i (.. 0 3) (when (< i 3)))
    (print i)
  )
  (print "end")
)
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

```brix
(main
  (for (in i (.. 0 3))
    (if (== i 3)
      (continue)
    )
    (print i)
  )
  (for (in i (..< 0 3))
    (for (in j (..< 0 3))
      (if (== j 2)
        (break)
      )
      (if (== i 1)
        (continue)
      )
      (print (+ (* i 10) j))
    )
  )
  (print "end")
)
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

```brix
(main
  (let i 9)
  (for (in i (..< 0 2))
    (print i)
  )
  (print i)
  (for (in i (.. 100i8 101i8))
    (let t (+ i 100))
    (print t)
  )
)
```

```output
0
1
9
-56
-55
```

### Ranges written as values: their ends, step and length, with a step written and counted down.

```brix
(main
  (let len 5)
  (let span (..< 0 len))
  (print (. span start))
  (print (. span end))
  (print (. span increment))
  (print (. span is-through))
  (print (. span length))
  (call show (.. 1 3))
  (call show (..< (call note 4) (call note 9)))
  (let wide (.. 2u64 10u64))
  (print (. wide length))
  (let real (.. 0.5 2.5))
  (print (. real start))
  (print (. real increment))
  (let back (.. 9 3))
  (print (. back length))
  (print ([] back 1))
  (let stepped (.. 0 10 (by: 5)))
  (print (. stepped increment))
  (print (. stepped length))
)

(fn-do + note Int
  (param n Int)
  (print (str "note " n))
  (return n)
)

(fn-do + show _
  (param range (of Range Int))
  (print (. range start))
  (print (. range end))
  (print (. range is-through))
  (print (. range length))
)
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

```brix
(main
  (let holder (call Holder (.. 2 6)))
  (print (tuple (. holder span start) (. holder span end) (. holder span is-through)))
  (print (. holder span length))
  (= (. holder span) (..< 1 4))
  (print (. holder span length))
  (let down (..< 5 1))
  (print (tuple (. down start) (. down end) (. down length)))
  (print (tuple ([] down 0) ([] down 3)))
  (let stepped (.. 10 0 (by: 5)))
  (print (tuple (. stepped length) ([] stepped 1)))
  (let bytes (.. 1u8 3u8))
  (print (. bytes length))
  (print (call total (.. 0 4)))
)

(type + Holder
  (prop + span (of Range Int))
  (fn-do + This _
    (param span _)
  )
)

(fn-do + total Int
  (param range (of Range Int))
  (let sum 0)
  (let i 0)
  (while (< i (. range length))
    (+= sum ([] range i))
    (+= i 1)
  )
  (return sum)
)
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

```brix
(main
  (let range (..< 0 3))
  (for (in i range)
    (print i)
  )
)
```

```output
0
1
2
```

### A range held in a local: given to a function that takes a stream, converted to a stream, and looped over.

```brix
(main
  (let range (.. 1 4))
  (print (call total range))
  (for (in n (as range (stream Int)))
    (print n)
  )
  (for (in n range)
    (print n)
  )
)

(fn-do + (of total T) T
  (param items (stream T))
  (let sum T)
  (for (in item items)
    (+= sum item)
  )
  (return sum)
)
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

```brix
(main
  (let range (... _ 3))
  (for (in i (call (. range normalize) (..< 0 5)))
    (print i)
  )
  (let open (... 2))
  (for (in i (call (. open normalize) (..< 0 5)))
    (print i)
  )
)
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

```brix
(main
  (let head (... _ 2))
  (call show head)
  (let tail (... 1))
  (call show tail)
  (call show (.. (. Position first) (. Position last)))
  (call show (.. (. Position last) (. Position first)))
  (call show (..< (. Position middle) (. Position end)))
)

(fn-do + show _
  (param range (of Range Position))
  (let items (list "a" "b" "c" "d"))
  (print (call (. ", " join) ([] items range)))
)
```

```output
a, b
b, c, d
a, b, c, d
a, b, c, d
c, d
```

```brix
(main
  (let text "hello world")
  (let count 5)
  (let spot (call Position))
  (print ([] text (... _ count)))
  (print ([] text (... (. spot column))))
  (print ([] (list 1 2 3 4) (... _ 2)))
  (print ([] (list 1 2 3 4) (... 2)))
)

(type + Position
  (prop + column Int (= 6))
)
```

```output
hello
world
1, 2
3, 4
```

### A range made forward holds the same items from the lowest up, whatever its step and direction.

```brix
(main
  (print (call List (call (. (.. 0 10 (by: 3)) forward))))
  (print (call List (call (. (..< 0 10 (by: 3)) forward))))
  (print (call List (call (. (.. 10 0 (by: 3)) forward))))
  (print (call List (call (. (..< 10 0 (by: 3)) forward))))
  (print (call List (call (. (.. 0.0 1.0 (by: 0.25)) forward))))
  (print (call List (call (. (..< 2 2) forward))))
)
```

```output
0, 3, 6, 9
0, 3, 6, 9
1, 4, 7, 10
1, 4, 7, 10
0, 0.25, 0.5, 0.75, 1
```

### A range made forward keeps its items when its highest is the item type's maximum.

```brix
(main
  (print (call List (call (. (.. 253u8 255u8) forward))))
  (print (call List (call (. (.. 255u8 253u8) forward))))
  (print (call List (call (. (.. 255u8 255u8) forward))))
  (print (call List (call (. (.. 250u8 255u8 (by: 3)) forward))))
  (print (call List (call (. (.. 127i8 123i8 (by: 2)) forward))))
  (print (call List (call (. (.. (- (. Int max) 2) (. Int max)) forward))))
)
```

```output
253, 254, 255
253, 254, 255
255
250, 253
123, 125, 127
9223372036854775805, 9223372036854775806, 9223372036854775807
```

### An index into a range is one of its items or out of range, and a slice by index holds the items at those places.

```brix
(main
  (let r (.. 0 10 (by: 3)))
  (print ([] r 3))
  (print (call List ([] r (..< 1 3))))
  (print (call List ([] r (.. 0 3 (by: 2)))))
  (print (call List ([] (.. 0 10) (..< 2 4))))
  (print (call List ([] (.. 10 0 (by: 2)) (.. 1 3))))
  (print (call List ([] (.. 0.0 1.0 (by: 0.25)) (..< 1 3))))
  (call try-index r 4)
  (call try-index r -1)
  (call try-slice r)
)

(fn-do + try-index _
  (param r (of Range Int))
  (param i Index)
  (let attempt
    (do-new
      (print ([] r i))
      (on-failure failure
        (print (str "index: " failure))
      )
    )
  )
  (call (. attempt wait))
)

(fn-do + try-slice _
  (param r (of Range Int))
  (let attempt
    (do-new
      (print (call List ([] r (..< 2 5))))
      (on-failure failure
        (print (str "slice: " failure))
      )
    )
  )
  (call (. attempt wait))
)
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

```brix
(main
  (call try-list)
  (call try-loop)
)

(fn-do + try-list _
  (let attempt
    (do-new
      (let r (.. 0 5 (by: 0)))
      (print (call List r))
      (on-failure failure
        (print (str "list: " failure))
      )
    )
  )
  (call (. attempt wait))
)

(fn-do + try-loop _
  (let attempt
    (do-new
      (for (in i (.. 0 5 (by: 0)))
        (print i)
      )
      (on-failure failure
        (print (str "loop: " failure))
      )
    )
  )
  (call (. attempt wait))
)
```

```output
list: A range cannot step by 0
loop: A range cannot step by 0
```

### reversed yields a range's items last to first and nothing for an empty range.

```brix
(main
  (print (. (call (of List Int) (call (. (..< 0 0) reversed))) length))
  (print (call (. ", " join) (call (. (.. 5 5) reversed))))
  (print (call (. ", " join) (call (. (.. 1 4) reversed))))
  (print (call (. ", " join) (call (. (.. 4 1) reversed))))
)
```

```output
0
5
4, 3, 2, 1
1, 2, 3, 4
```
