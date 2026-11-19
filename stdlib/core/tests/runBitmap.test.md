# Bitmap

```brix
(main
  (let value (as 7 (of Value U64)))
  (= ([] value 0) 9ul)
  (print (as value U64))
)

(type + (of Value Integer) (is Integer)
  (fn-expr + (index-get this (param index Index)) _
    (+ (as this Integer) (as index Integer))
  )
  (fn-expr + (index-set this (param index Index) (param value Integer)) _
    (as value This)
  )
)
```

```output
9
```

```brix
(main
  (let value (as 7 (of Value I8)))
  (let copy value)
  (= ([] value 0) (as -8 I8))
  (print (as value I8))
  (print (as copy I8))
)

(type + (of Value Integer) (is Integer)
  (fn-expr + (index-get this (param index Index)) _
    (+ (as this Integer) (as index Integer))
  )
  (fn-expr + (index-set this (param index Index) (param value Integer)) _
    (as value This)
  )
)
```

```output
-8
7
```

```brix
(main
  (let holder (call Holder (as 7 (of Value U64))))
  (= ([] (. holder value) 0) 9ul)
  (print (as (. holder value) U64))
)

(type + (of Value Integer) (is Integer)
  (fn-expr + (index-get this (param index Index)) _
    (+ (as this Integer) (as index Integer))
  )
  (fn-expr + (index-set this (param index Index) (param value Integer)) _
    (as value This)
  )
)

(type + Holder
  (prop + value (of Value U64))
  (fn-do + This _
    (param value _)
  )
)
```

```output
9
```

```brix
(main
  (let values (call (of Array (of Value U64)) (list (as 7 (of Value U64)))))
  (= ([] ([] values 0) 0) 9ul)
  (print (as ([] values 0) U64))
)

(type + (of Value Integer) (is Integer)
  (fn-expr + (index-get this (param index Index)) _
    (+ (as this Integer) (as index Integer))
  )
  (fn-expr + (index-set this (param index Index) (param value Integer)) _
    (as value This)
  )
)
```

```output
9
```

```brix
(main
  (let probe (call Probe))
  (= ([] ([] (call (. probe values)) (call (. probe index))) (call (. probe index))) 9ul)
  (print (as ([] (. probe items) 0) U64))
  (print (. probe calls))
  (print (. probe indices))
)

(type + (of Value Integer) (is Integer)
  (fn-expr + (index-get this (param index Index)) _
    (+ (as this Integer) (as index Integer))
  )
  (fn-expr + (index-set this (param index Index) (param value Integer)) _
    (as value This)
  )
)

(type + Probe
  (prop + items _ (= (call (of Array (of Value U64)) (list (as 7 (of Value U64))))))
  (prop + calls _ (= 0))
  (prop + indices _ (= 0))
  (fn-do + values _
    (+= calls 1)
    (return items)
  )
  (fn-do + index _
    (+= indices 1)
    (return 0)
  )
)
```

```output
9
1
2
```

### What []= answers for a number an index gave is stored through that index

```brix
(prop + kept _ (= (call (of Array (of Count U8)) (list (as 0 (of Count U8))))))
(blank)

(main
  (let counts (call (of Array (of Count U8)) (list (as 0 (of Count U8)) (as 0 (of Count U8)))))
  (= ([] ([] counts 1) 3) 10u8)
  (= ([] ([] counts 1) 1) 20u8)
  (= ([] ([] counts 0) 7) 1u8)
  (print (tuple (as ([] counts 0) U8) (as ([] counts 1) U8)))
  (= ([] ([] kept 0) 2) 5u8)
  (print (as ([] kept 0) U8))
  (let longs (call (of Array (of Count I64)) (list (as 5 (of Count I64)))))
  (let copy ([] longs 0))
  (= ([] ([] longs 0) 1) -8)
  (print (tuple (as ([] longs 0) I64) (as copy I64)))
)

(type + (of Count Integer) (is Integer)
  (fn-expr + (index-set this (param index Index) (param value Integer)) _
    (as (+ (as this Integer) value (as index Integer)) This)
  )
)
```

```output
(8, 34)
7
(-2, 5)
```

### What a number's []= answers is stored where the number is held: a local, a property, an argument

```brix
(main
  (let value (as 1 (of Count U8)))
  (= ([] value 2) 10u8)
  (print (as value U8))
  (let holder (call Holder (as 3 (of Count U8))))
  (= ([] (. holder count) 1) 4u8)
  (print (as (. holder count) U8))
  (call (. holder add) 5u8)
  (print (as (. holder count) U8))
  (print (tuple (as (call raise value) U8) (as value U8)))
)

(fn-do + raise (of Count U8)
  (param given (of Count U8))
  (= ([] given 0) 100u8)
  (return given)
)

(type + (of Count Integer) (is Integer)
  (fn-expr + (index-set this (param index Index) (param value Integer)) _
    (as (+ (as this Integer) value (as index Integer)) This)
  )
)

(type + Holder
  (prop + count (of Count U8))
  (fn-do + This _
    (param count _)
  )
  (fn-do + add _
    (param value U8)
    (= ([] count 0) value)
  )
)
```

```output
13
8
13
(113, 13)
```

```brix
(import /core/bitmap)
(blank)

(main
  (for (in capacity (list 0 1 63 64 65 130))
    (let bits (call BitmapBuffer capacity))
    (print (str (. bits capacity) " " (call (. bits count) false) " " (call (. bits count) true)))
  )
)
```

```output
0 0 0
1 1 0
63 63 0
64 64 0
65 65 0
130 130 0
```

```brix
(import /core/bitmap)
(blank)

(main
  (call read (call (of Bitmap U8)) 8)
  (call read (call (of Bitmap U8)) (. Size max))
  (call write (call (of Bitmap U8)) 8)
  (call write (call (of Bitmap U8)) (. Size max))
  (call read (call BitmapBuffer 8) 8)
  (call read (call BitmapBuffer 8) (. Size max))
  (call write (call BitmapBuffer 8) 8)
  (call write (call BitmapBuffer 8) (. Size max))
  (call read (call BitmapBuffer 0) 0)
  (call read (call BitmapBuffer 0) (. Size max))
  (call write (call BitmapBuffer 0) 0)
  (call write (call BitmapBuffer 0) (. Size max))
)

(fn-do _ (of read Bits) _
  (param bits Bits)
  (param index Index)
  (let attempt
    (do-new
      (let value ([] bits index))
      (on-failure failure
        (let panic (? Panic) failure)
        (print (str "read " index ": " (is panic OutOfBoundsError) " " panic))
      )
    )
  )
  (call (. attempt wait))
)

(fn-do _ (of write Bits) _
  (param bits Bits)
  (param index Index)
  (let attempt
    (do-new
      (= ([] bits index) true)
      (on-failure failure
        (let panic (? Panic) failure)
        (print (str "write " index ": " (is panic OutOfBoundsError) " " panic))
      )
    )
  )
  (call (. attempt wait))
)
```

```output
read 8: true Bitmap index out of bounds
read 9223372036854775807: true Bitmap index out of bounds
write 8: true Bitmap index out of bounds
write 9223372036854775807: true Bitmap index out of bounds
read 8: true Bitmap index out of bounds
read 9223372036854775807: true Bitmap index out of bounds
write 8: true Bitmap index out of bounds
write 9223372036854775807: true Bitmap index out of bounds
read 0: true Bitmap index out of bounds
read 9223372036854775807: true Bitmap index out of bounds
write 0: true Bitmap index out of bounds
write 9223372036854775807: true Bitmap index out of bounds
```

```brix
(import /core/bitmap)
(blank)

(main
  (call read (call (of Bitmap U8)) -1)
  (call write (call (of Bitmap U8)) -1)
  (call read (call BitmapBuffer 8) -1)
  (call write (call BitmapBuffer 8) -1)
)

(fn-do _ (of read Bits) _
  (param bits Bits)
  (param index Index)
  (let attempt
    (do-new
      (let value ([] bits index))
      (on-failure failure
        (print (str "read " index ": " (is failure OutOfBoundsError) " " failure))
      )
    )
  )
  (call (. attempt wait))
)

(fn-do _ (of write Bits) _
  (param bits Bits)
  (param index Index)
  (let attempt
    (do-new
      (= ([] bits index) true)
      (on-failure failure
        (print (str "write " index ": " (is failure OutOfBoundsError) " " failure))
      )
    )
  )
  (call (. attempt wait))
)
```

```output
read -1: true Bitmap index out of bounds
write -1: true Bitmap index out of bounds
read -1: true Bitmap index out of bounds
write -1: true Bitmap index out of bounds
```

```brix
(import /core/bitmap)
(blank)

(main
  (for (in index (..< 0 8))
    (let raw (/lshift/ (as 1 I8) index))
    (let bits (call (of Bitmap I8) raw))
    (print (str raw " " (as bits I8) " " ([] bits index) " " (call (. bits count) true)))
  )
  (let one (call (of Bitmap I1) (as 1 I1)))
  (print (str (as one I1) " " ([] one 0)))
  (let top (call (of Bitmap I64) (/lshift/ (as 1 I64) 63)))
  (print (str (as top I64) " " ([] top 63) " " (call (. top count) true)))
  (let high (call (of Bitmap U64) 9223372036854775809ul))
  (print (str (as high U64) " " ([] high 63) " " ([] high 0) " " (call (. high count) true)))
  (let ones (call (of Bitmap I64) -1l))
  (print (str (as ones I64) " " (call (. ones count) true)))
)
```

```output
1 1 true 1
2 2 true 1
4 4 true 1
8 8 true 1
16 16 true 1
32 32 true 1
64 64 true 1
-128 -128 true 1
1 true
-9223372036854775808 true 1
9223372036854775809 true true 2
-1 64
```

```brix
(import /core/bitmap)
(blank)

(main
  (let bits (call BitmapBuffer 65))
  (let alias bits)
  (= ([] alias 64) true)
  (print ([] bits 64))
  (call (. alias fill) true)
  (print (call (. bits count) true))
  (let copy (call (. bits copy)))
  (= ([] bits 64) false)
  (print ([] copy 64))
  (= ([] copy 0) false)
  (print ([] bits 0))
  (print (. copy capacity))
  (let empty (call (. (call BitmapBuffer 0) copy)))
  (print (str (. empty capacity) " " (call (. empty count) true)))
)
```

```output
true
65
true
true
65
0 0
```

```brix
(import /core/arena)
(import /core/bitmap)
(blank)

(main
  (let source-arena (call Arena))
  (let target-arena (call Arena))
  (let bits (call BitmapBuffer source-arena 65))
  (= ([] bits 64) true)
  (let copy (call (. bits copy) target-arena))
  (call (. source-arena release))
  (print ([] copy 64))
  (= ([] copy 0) true)
  (print (call (. copy count) true))
  (call (. target-arena release))
)
```

```output
true
2
```

```brix
(import /core/bitmap)
(blank)

(main
  (let empty (call (of Bitmap I1)))
  (let full (call (. empty fill) true))
  (print (str (call (. full count) true) " " (call (. full count) false) " " (call (. empty count) false) " " (as (call (. full fill) false) I1)))
  (let wide-empty (call (of Bitmap U64)))
  (let wide-full (call (. wide-empty fill) true))
  (print (str (call (. wide-full count) true) " " (call (. wide-full count) false) " " (call (. wide-empty count) false)))
  (print (as wide-full U64))
  (print (as (call (. wide-full fill) false) U64))
  (let sparse (call (of Bitmap U8)))
  (for (in index (list 0 3 7))
    (= ([] sparse index) true)
  )
  (print (str (call (. sparse count) true) " " (call (. sparse count) false) " " (as sparse U8)))
)
```

```output
1 0 1 0
64 0 64
18446744073709551615
0
3 5 137
```

```brix
(import /core/bitmap)
(blank)

(main
  (for (in capacity (list 0 1 64 65 130))
    (let bits (call BitmapBuffer capacity))
    (call (. bits fill) true)
    (let set (call (. bits count) true))
    (let clear (call (. bits count) false))
    (call (. bits fill) false)
    (print (str set " " clear " " (call (. bits count) false) " " (. bits capacity)))
  )
  (let sparse (call BitmapBuffer 8))
  (for (in index (list 0 3 7))
    (= ([] sparse index) true)
  )
  (print (str (call (. sparse count) true) " " (call (. sparse count) false)))
)
```

```output
0 0 0 0
1 0 1 1
64 0 64 64
65 0 65 65
130 0 130 130
3 5
```

```brix
(import /core/bitmap)
(blank)

(main
  (let bits (call (of Bitmap I8)))
  (= ([] bits 7) true)
  (print (str ([] bits 7) " " (call (. bits count) true) " " (as bits I8)))
  (= ([] bits 7) false)
  (print (as bits I8))
  (let flags (call (of Bitmap U8)))
  (= ([] flags 3) true)
  (= ([] flags 3) true)
  (print (str ([] flags 3) " " (call (. flags count) true)))
  (= ([] flags 4) true)
  (= ([] flags 3) false)
  (= ([] flags 3) false)
  (print (str ([] flags 3) " " ([] flags 4) " " (call (. flags count) true)))
)
```

```output
true 1 -128
0
true 1
false true 1
```

```brix
(import /core/bitmap)
(blank)

(main
  (let bits (call BitmapBuffer 130))
  (for (in index (list 0 63 64 129))
    (= ([] bits index) true)
    (print (str ([] bits index) " " (call (. bits count) true) " " (!! (call (. bits find) true))))
    (= ([] bits index) false)
    (print (call (. bits count) true))
  )
)
```

```output
true 1 0
0
true 1 63
0
true 1 64
0
true 1 129
0
```

```brix
(import /core/bitmap)
(blank)

(main
  (let bits (call (of Bitmap U8)))
  (let iterator (as bits (of Iterable Bool)))
  (= bits (call (. bits fill) true))
  (let clear 0)
  (for (in bit iterator)
    (if (not bit)
      (+= clear 1)
    )
  )
  (print clear)
  (print (call (. bits count) true))
  (let changing (call (of Bitmap U8)))
  (let seen 0)
  (for (in bit changing)
    (= changing (call (. changing fill) true))
    (if (not bit)
      (+= seen 1)
    )
  )
  (print seen)
  (let source (call (of Bitmap U8)))
  (let indices (call (. source indices) false))
  (= source (call (. source fill) true))
  (let count 0)
  (for (in index indices)
    (+= count 1)
  )
  (print count)
)
```

```output
8
8
8
8
```

```brix
(import /core/bitmap)
(blank)

(main
  (let bits (call BitmapBuffer 3))
  (let iterator (as bits (of Iterable Bool)))
  (= ([] bits 2) true)
  (for (in bit iterator)
    (print (str "before " bit))
  )
  (let live (call BitmapBuffer 3))
  (for (in bit live)
    (print (str "during " bit))
    (= ([] live 2) true)
  )
  (let source (call BitmapBuffer 3))
  (let snapshot (as (call (. source copy)) (of Iterable Bool)))
  (call (. source fill) true)
  (for (in bit snapshot)
    (print (str "copy " bit))
  )
)
```

```output
before false
before false
before true
during false
during false
during true
copy false
copy false
copy false
```

```brix
(import /core/bitmap)
(blank)

(main
  (let packed (call (of Bitmap U8) (as 66 U8)))
  (for (in index (call (. packed indices) true))
    (print (str "packed true " index))
  )
  (for (in index (call (. packed indices) false))
    (print (str "packed false " index))
  )
  (let buffer (call BitmapBuffer 8))
  (= ([] buffer 1) true)
  (= ([] buffer 6) true)
  (for (in index (call (. buffer indices) true))
    (print (str "buffer true " index))
  )
  (for (in index (call (. buffer indices) false))
    (print (str "buffer false " index))
  )
  (let empty (call BitmapBuffer 0))
  (for (in bit empty)
    (print "empty bit")
  )
  (for (in index (call (. empty indices) false))
    (print "empty index")
  )
  (print "done")
)
```

```output
packed true 1
packed true 6
packed false 0
packed false 2
packed false 3
packed false 4
packed false 5
packed false 7
buffer true 1
buffer true 6
buffer false 0
buffer false 2
buffer false 3
buffer false 4
buffer false 5
buffer false 7
done
```

```brix
(import /BxGC/BxGCTest.h)
(import /core/bitmap)
(blank)

(main
  (let iterator (call retained-iterator))
  (call C:brixCollect)
  (let seen 0)
  (for (in bit iterator)
    (if bit
      (+= seen 1)
    )
  )
  (print seen)
)

(fn-do _ retained-iterator _
  (let bits (call BitmapBuffer 257))
  (call (. bits fill) true)
  (return (as bits (of Iterable Bool)))
)
```

```output
257
```

```brix
(import /core/bitmap)
(blank)

(main
  (let bits (call (of Bitmap U8)))
  (print (call (. bits find) true))
  (print (call (. (call (. bits fill) true) find) false))
  (= ([] bits 6) true)
  (= ([] bits 2) true)
  (print (call (. bits find) true))
  (print (call (. bits find) false))
  (= ([] bits 0) true)
  (= ([] bits 1) true)
  (print (call (. bits find) false))
)
```

```output
unknown
unknown
2
0
3
```

```brix
(import /core/bitmap)
(blank)

(main
  (let bits (call (of Bitmap U8)))
  (print (call (. bits find-consecutive) 0 false))
  (print (call (. bits find-consecutive) 8 false))
  (print (call (. bits find-consecutive) 9 false))
  (print (call (. bits find-consecutive) (. Size max) false))
  (for (in index (list 1 2 5 6 7))
    (= ([] bits index) true)
  )
  (print (call (. bits find-consecutive) 2 true))
  (print (call (. bits find-consecutive) 3 true))
  (print (call (. bits find-consecutive) 4 true))
  (print (call (. bits find-consecutive) 2 false))
  (print (call (. bits find-consecutive) 3 false))
  (let ends (call (of Bitmap U8)))
  (for (in index (list 0 1 6 7))
    (= ([] ends index) true)
  )
  (print (call (. ends find-consecutive) 3 true))
  (print (call (. ends find-consecutive) 4 false))
  (let alternating (call (of Bitmap U8)))
  (for (in index (list 0 2 4 6))
    (= ([] alternating index) true)
  )
  (print (call (. alternating find-consecutive) 2 true))
  (print (call (. alternating find-consecutive) 2 false))
)
```

```output
unknown
0
unknown
unknown
1
5
unknown
3
unknown
unknown
2
unknown
unknown
```

```brix
(import /core/bitmap)
(blank)

(main
  (let bits (call BitmapBuffer 193))
  (for (in index (..< 60 140))
    (= ([] bits index) true)
  )
  (print (call (. bits find-consecutive) 80 true))
  (print (call (. bits find-consecutive) 81 true))
  (let word (call BitmapBuffer 130))
  (for (in index (..< 62 66))
    (= ([] word index) true)
  )
  (print (call (. word find-consecutive) 4 true))
  (print (call (. word find-consecutive) 5 true))
  (let padded (call BitmapBuffer 65))
  (call (. padded fill) true)
  (print (call (. padded find) false))
  (print (call (. padded find-consecutive) 1 false))
  (print (call (. padded find-consecutive) 65 true))
  (let empty (call BitmapBuffer 0))
  (print (call (. empty find) true))
  (print (call (. empty find) false))
  (print (call (. empty find-consecutive) 1 false))
)
```

```output
60
unknown
62
unknown
unknown
unknown
0
unknown
unknown
unknown
```

### BitmapBuffer rejects a negative capacity and counts the bits of a partial final word.

```brix
(import /core/arena)
(import /core/bitmap)
(blank)

(main
  (let arena (call Arena))
  (for (in capacity (list -1 -64))
    (let gc
      (do-new
        (let bits (call BitmapBuffer capacity))
        (on-failure failure
          (print (str "gc " capacity ": " failure))
        )
      )
    )
    (call (. gc wait))
    (let arena-attempt
      (do-new
        (let bits (call BitmapBuffer arena capacity))
        (on-failure failure
          (print (str "arena " capacity ": " failure))
        )
      )
    )
    (call (. arena-attempt wait))
  )
  (call (. arena release))
  (for (in capacity (list 63 64 65))
    (let bits (call BitmapBuffer capacity))
    (call (. bits fill) true)
    (print (str (. bits capacity) " " ([] bits (- capacity 1)) " " (call (. bits count) true) " " (call (. bits count) false)))
  )
)
```

```output
gc -1: BitmapBuffer capacity is negative
arena -1: BitmapBuffer capacity is negative
gc -64: BitmapBuffer capacity is negative
arena -64: BitmapBuffer capacity is negative
63 true 63 0
64 true 64 0
65 true 65 0
```
