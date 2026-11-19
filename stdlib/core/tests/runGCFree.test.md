# Gcfree

```brix
(import /BxEvents/BxGCHandshake.h)
(import /BxGC/BxGC.h)
(blank)

(main
  (print (call C:brixGCActiveTaskCanUseFastAllocation))
  (print (call zone-state 4))
  (print (call C:brixGCActiveTaskCanUseFastAllocation))
  (call zone-collection-boundary)
  (print (call C:brixGCActiveTaskCanUseFastAllocation))
)

(fn-do + zone-state Bool (without gc)
  (param count Int)
  (let total 0)
  (for (in index (..< 0 count))
    (+= total index)
  )
  (return (call C:brixGCActiveTaskCanUseFastAllocation))
)

(fn-do + zone-collection-boundary _ (without gc)
  (call C:brixCollect)
)
```

```output
true
false
true
true
```

```brix
(import /BxEvents/BxGCHandshake.h)
(blank)

(main
  (print (catch (call gc-free-propagate false)))
  (print (call C:brixGCActiveTaskCanUseFastAllocation))
  (print (call gc-free-catch false))
  (print (call C:brixGCActiveTaskCanUseFastAllocation))
  (print (catch (call gc-free-catch-replace false)))
  (print (call C:brixGCActiveTaskCanUseFastAllocation))
)

(type + GCFreeFailure (is Error))

(type + GCFreeReplacement (is Error))

(fn-do + gc-free-fallible Int (?! GCFreeFailure) (without gc)
  (param succeeds Bool)
  (if succeeds
    (return 7)
  )
  (throw (call GCFreeFailure "failed"))
)

(fn-do + gc-free-propagate Int (?! GCFreeFailure) (without gc)
  (param succeeds Bool)
  (return (+ (call gc-free-fallible succeeds) 1))
)

(fn-do + gc-free-catch Int (without gc)
  (param succeeds Bool)
  (let value
    (call gc-free-fallible succeeds
      (block
        (on-error GCFreeFailure
          40
        )
      )
    )
  )
  (return (+ value 2))
)

(fn-do + gc-free-catch-replace Int (?! GCFreeReplacement) (without gc)
  (param succeeds Bool)
  (let value
    (call gc-free-fallible succeeds
      (block
        (on-error GCFreeFailure
          (throw (call GCFreeReplacement "replaced"))
        )
      )
    )
  )
  (return value)
)
```

```output
failed
true
42
true
replaced
true
```

```brix
(import /BxEvents/BxGCHandshake.h)
(blank)

(main
  (print (call managed-catch-then-allocate true))
  (print (call C:brixGCActiveTaskCanUseFastAllocation))
  (print (call managed-catch-value false))
  (print (call C:brixGCActiveTaskCanUseFastAllocation))
)

(type + ManagedFailure (is Error))

(fn-do + managed-fallible Int (?! ManagedFailure)
  (param succeeds Bool)
  (if succeeds
    (return 7)
  )
  (throw (call ManagedFailure "failed"))
)

(fn-do + managed-catch-then-allocate Size
  (param succeeds Bool)
  (let value
    (call managed-fallible succeeds
      (block
        (on-error ManagedFailure
          (throw (call Panic "managed failure"))
        )
      )
    )
  )
  (let values (list value))
  (return (. values length))
)

(fn-expr + managed-catch-value Int
  (param succeeds Bool)
  (call managed-fallible succeeds
    (block
      (on-error ManagedFailure
        3
      )
    )
  )
)
```

```output
1
true
3
true
```

```brix
(main
  (let seed 5)
  (do (without gc)
    (let value (+ seed 7))
    (+= value 1)
  )
  (print seed)
)
```

```output
5
```

```brix crash
(main
  (let seed 5)
  (do (without gc)
    (let box (call Box))
  )
  (print seed)
)

(type + Box
  (prop + value Int (= 0))
)
```

```output
Exception: allocation is not allowed in a GC-free zone (entered at no file:4:3): main (do without gc) → (call Box)
<no file:5>
        (let box (call Box))
                 ^^^^^^^^^^
```

```brix crash
(main
  (print (call allocate-in-zone))
)

(type + Box
  (prop + value Int (= 0))
)

(fn-do + allocate-in-zone Int (without gc)
  (let box (call Box))
  (return (. box value))
)
```

```output
Exception: allocation is not allowed in a GC-free zone (entered at no file:10:1): allocate-in-zone (without gc) → (call Box)
<no file:11>
      (let box (call Box))
               ^^^^^^^^^^
```

```brix crash
(main
  (let holder (call Holder))
  (call (. holder replace) "after")
)

(type + Holder
  (prop + value String (= "before"))
  (fn-do + replace _ (without gc)
    (param next String)
    (= value next)
  )
)
```

```output
Exception: storing a managed pointer is not allowed in a GC-free zone (entered at no file:9:3); compute into a zone buffer and publish after the zone exits: Holder.replace (without gc) → (= value next)
<no file:11>
        (= value next)
        ^^^^^^^^^^^^^^
```

```brix crash
(main
  (let generator (call suspended-values))
  (print (call generator))
)

(fn-do + suspended-values _ (without gc)
  (yield 1)
)
```

```output
Exception: allocation is not allowed in a GC-free zone (entered at no file:7:1): suspended-values (without gc)
<no file:7>
    (fn-do + suspended-values _ (without gc)
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Exception: cannot suspend inside a GC-free zone (entered at no file:7:1): suspended-values (without gc) → (yield 1)
<no file:8>
      (yield 1)
      ^^^^^^^^^
```

```brix crash
(main
  (call conflict)
)

(fn-expr + conflict _ (without gc heap))
```

```output
Exception: Conflicting compilation effect 'no-heap'
<no file:6>
    (fn-expr + conflict _ (without gc heap))
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix crash
(main
  (let generator (call suspended-values))
  (print (call generator))
)

(fn-do + suspended-values _ (without safepoints)
  (yield 1)
)
```

```output
Exception: cannot suspend under 'without safepoints': suspended-values (without safepoints) → (yield 1)
<no file:8>
      (yield 1)
      ^^^^^^^^^
```

```brix
(import /core/arena)
(import /core/table)
(blank)

(main
  (let span-source (. "span-owned" codepoints))
  (print (call arena-cycle span-source))
)

(fn-do + arena-cycle U64 (without gc)
  (param span-source ([] Char))
  (let arena (call Arena 4096))
  (finally
    (call (. arena release))
  )
  (let alias arena)
  (let checksum 0u64)
  (let copied (call arena-copy-string alias))
  (+= checksum (as (. copied length) U64))
  (let copied-span (call arena-copy-codepoint-span alias span-source))
  (+= checksum (as (. copied-span length) U64))
  (for (in index (..< 0 16))
    (let value (call (of (. alias allocate) U64) 1))
    (= ([] value 0) (as index U64))
    (+= checksum ([] value 0))
  )
  (let mark (. alias mark))
  (let discarded (call (of (. alias allocate) U64) 1))
  (= ([] discarded 0) 99u64)
  (call (. alias rewind) mark)
  (let rows (call (of Table ArenaBenchRow) arena 1))
  (call (. rows append) (call ArenaBenchRow 10u64 20u64))
  (let table-mark (. rows mark))
  (call (. rows append) (call ArenaBenchRow 30u64 40u64))
  (+= checksum (+ (. ([] rows 0) left) (. ([] rows 1) right)))
  (call (. rows rewind) table-mark)
  (call (. rows clear))
  (call (. arena reset))
  (return checksum)
)

(fn-do + arena-copy-string String (without gc)
  (param arena Arena)
  (return (call (. arena copy-string) "arena-owned"))
)

(fn-do + arena-copy-codepoint-span String (without gc)
  (param arena Arena)
  (param source ([] Char))
  (return (call (. arena copy-span) source 0 (. source length)))
)

(type + ArenaBenchRow (is Struct)
  (prop + left U64)
  (prop + right U64)
)
```

```output
191
```

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena 256))
  (finally
    (inline (call (. arena release)))
  )
  (print (call arena-cycle arena))
)

(type + (of ManagedBacking Value)
  (prop + marker Value)
)

(type + (of ArenaBacking Value) (is Tuple)
  (prop + arena Arena)
)

(type + (of Storage Value Backing) (is Tuple)
  (constraint = Backing (of ManagedBacking Value))
  (prop + storage Backing)
  (prop + value Value)
  (fn-do + This _
    (param value Value)
    (= storage (call (of ManagedBacking Value)))
  )
  (fn-do + This _
    (param arena Arena)
    (param value Value)
    (constraint = Backing (of ArenaBacking Value))
    (= storage (call (of ArenaBacking Value) arena))
  )
)

(fn-do + arena-cycle U32 (without gc)
  (param arena Arena)
  (let storage (of Storage U32 (of ArenaBacking U32)) (call (of Storage U32) arena 41u32))
  (return (. storage value))
)
```

```output
41
```

```brix
(import /core/arena)
(import /core/table)
(blank)

(main
  (print (call controlled-union-cycle))
)

(fn-do + controlled-union-cycle U64 (without gc)
  (let arena (call Arena 8192))
  (finally
    (call (. arena release))
  )
  (let rows (call (of Table Node) arena 1))
  (call (. rows append) (call (. Node small) 1u32 1u64 2u32))
  (call (. rows reserve) 8)
  (call (. rows append) (call (. Node wide) 2u32 3u64 4u32 5u32))
  (call (. rows append) (call (. Node small) 3u32 6u64 7u32))
  (blank)
  (call (. ([] rows 0) lower) 2u64)
  (call (. ([] rows 1) lower) 3u64)
  (let extension-checksum (+ (as (. ([] rows 0) source-plus) U64) (call (. ([] rows 0) score)) (call (. ([] rows 1) score))))
  (blank)
  (match ([] rows 1)
    (case (call .small small)
      (+= (. small value) 1u64)
    )
    (case (call .wide wide)
      (+= (. wide hot) 1u64)
    )
  )
  (match ([] rows 1)
    (case (call .small small)
      (+= (. small cold) 1u32)
    )
    (case (call .wide wide)
      (+= (. wide cold) 1u32)
    )
  )
  (blank)
  (let live ([] rows 1))
  (= ([] rows 0) live)
  (let span ([] rows (..< 0 2)))
  (= ([] rows (..< 1 3)) span)
  (blank)
  (let snapshot Node (as ([] rows 1) Node))
  (let checksum
    (match snapshot
      (case (call .small small)
        (+ (. small value) (as (. small cold) U64))
      )
      (case (call .wide wide)
        (+ (. wide hot) (as (. wide cold) U64) (as (. wide other) U64))
      )
    )
  )
  (call (. rows clear))
  (return (+ checksum extension-checksum))
)

(type + Syntax (is Tuple)
  (prop + source U32)
)

(type + Small (is Syntax)
  (prop + value U64 @hot)
  (prop + cold U32)
)

(type + Wide (is Syntax)
  (prop + hot U64 @hot)
  (prop + cold U32)
  (prop + other U32)
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
  (fn-expr + score U64
    (match this
      (case (call .small small)
        (+ (. small value) (as (. small cold) U64))
      )
      (case (call .wide wide)
        (+ (. wide hot) (as (. wide cold) U64) (as (. wide other) U64))
      )
    )
  )
)

(extend + Small
  (fn-do + lower _
    (param delta U64)
    (+= (. this value) delta)
    (+= (. this cold) (as delta U32))
    (+= (. this source) (as delta U32))
  )
)

(extend + Wide
  (fn-do + lower _
    (param delta U64)
    (+= (. this hot) delta)
    (+= (. this cold) (as delta U32))
    (+= (. this source) (as delta U32))
  )
)

(extend + Syntax
  (fn-expr + source-plus U64 :get
    (as (. this source) U64)
  )
)
```

```output
48
```

```brix crash
(main
  (let value (as (call Pair 1 2) View))
  (print (call equal value value))
)

(type + View (is Interface))

(type + Pair (is (tuple (: first I32) (: second I32)))
  (implements View)
)

(fn-do + equal _ (without gc)
  (param left View)
  (param right View)
  (return (== left right))
)
```

```output
Exception: Interface equality requires unrestricted effects: equal (without gc) → (== left right)
<no file:16>
      (return (== left right))
              ^^^^^^^^^^^^^^^
```

```brix crash
(main
  (let value (as (call Pair 1 2) View))
  (print (call equal value value))
)

(type + View (is Interface))

(type + Pair (is (tuple (: first I32) (: second I32)))
  (implements View)
)

(fn-do + equal _ (without heap)
  (param left View)
  (param right View)
  (return (== left right))
)
```

```output
Exception: Interface equality requires unrestricted effects: equal (without heap) → (== left right)
<no file:16>
      (return (== left right))
              ^^^^^^^^^^^^^^^
```

```brix crash
(main
  (let value (as (call Pair 1 2) View))
  (print (call equal value value))
)

(type + View (is Interface))

(type + Pair (is (tuple (: first I32) (: second I32)))
  (implements View)
)

(fn-do + equal _ (without safepoints)
  (param left View)
  (param right View)
  (return (== left right))
)
```

```output
Exception: Interface equality requires unrestricted effects: equal (without safepoints) → (== left right)
<no file:16>
      (return (== left right))
              ^^^^^^^^^^^^^^^
```

```brix crash
(main
  (let value (as (call Pair 1 2) View))
  (print (call equal value value))
)

(type + View (is Interface))

(type + Pair (is (tuple (: first I32) (: second I32)))
  (implements View)
)

(fn-do + equal _ (without suspend)
  (param left View)
  (param right View)
  (return (== left right))
)
```

```output
Exception: Interface equality requires unrestricted effects: equal (without suspend) → (== left right)
<no file:16>
      (return (== left right))
              ^^^^^^^^^^^^^^^
```

```brix
(main
  (let shape Shape (call Square 3))
  (let base Base (call Derived))
  (print (call measure shape base))
)

(type + Shape (is Interface)
  (fn-do + area Int)
)

(type + Square
  (implements Shape)
  (prop + side Int)
  (fn-do + This _
    (param side _)
  )
  (fn-do + area Int
    (return (* side side))
  )
)

(type + Base
  (fn-do + This _)
  (fn-do + area Int
    (return 0)
  )
)

(type + Derived (is Base)
  (fn-do + This _)
  (fn-do + area Int
    (return 7)
  )
)

(fn-do + measure Int (without gc)
  (param shape Shape)
  (param base Base)
  (return (+ (call (. shape area)) (call (. base area))))
)
```

```output
16
```

```brix crash
(main
  (let shape Shape (call Square 3))
  (print (call measure shape))
)

(type + Shape (is Interface)
  (fn-do + area Int)
)

(type + Square
  (implements Shape)
  (prop + side Int)
  (fn-do + This _
    (param side _)
  )
  (fn-do + area Int
    (let box (call Box))
    (return (* side side))
  )
)

(type + Box
  (prop + value Int (= 0))
)

(fn-do + measure Int (without gc)
  (param shape Shape)
  (return (call (. shape area)))
)
```

```output
Exception: allocation is not allowed in a GC-free zone (entered at no file:27:1): measure (without gc) → Shape.area → Square.area → (call Box)
<no file:18>
        (let box (call Box))
                 ^^^^^^^^^^
```

```brix crash
(main
  (let base Base (call Derived))
  (print (call measure base))
)

(type + Base
  (fn-do + This _)
  (fn-do + area Int
    (return 0)
  )
)

(type + Derived (is Base)
  (fn-do + This _)
  (fn-do + area Int
    (let box (call Box))
    (return 7)
  )
)

(type + Box
  (prop + value Int (= 0))
)

(fn-do + measure Int (without gc)
  (param base Base)
  (return (call (. base area)))
)
```

```output
Exception: allocation is not allowed in a GC-free zone (entered at no file:26:1): measure (without gc) → Base.area → Derived.area → (call Box)
<no file:17>
        (let box (call Box))
                 ^^^^^^^^^^
```

```brix crash
(main
  (print (call run (=> ((: n Int)) (+ n 1))))
)

(fn-do + run Int (without suspend)
  (param action (fn-type Int (param _ Int)))
  (return (call action 4))
)
```

```output
Exception: Function value call requires unrestricted effects: run (without suspend) → (call action 4)
<no file:8>
      (return (call action 4))
              ^^^^^^^^^^^^^^^
```

### A GC-free block assigns a local declared outside it, at the top level and in a function

```brix
(main
  (let seed 5)
  (let total 0)
  (do (without gc)
    (= total (+ seed 8))
  )
  (print total)
  (print (call run 5))
)

(fn-do + run I64
  (param seed I64)
  (let total 0)
  (do (without gc)
    (= total (+ seed 8))
  )
  (return total)
)
```

```output
13
13
```

### A GC-free block assigns fields of an outer struct local

```brix
(main
  (let point (call Point 1 2))
  (do (without gc)
    (= (. point x) 10)
    (+= (. point y) 5)
  )
  (print (. point x))
  (print (. point y))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
10
7
```

### A loop in a GC-free block, and a nested GC-free block, update outer locals

```brix
(main
  (let sum 0)
  (do (without gc)
    (for (in index (..< 0 5))
      (+= sum index)
    )
  )
  (print sum)
  (let outer 1)
  (do (without gc)
    (+= outer 10)
    (do (without gc)
      (+= outer 100)
    )
  )
  (print outer)
)
```

```output
10
111
```

### A GC-free block assigns a global, arguments, and locals taken apart into

```brix
(prop + counter _ (= 1))
(blank)

(main
  (= counter 2)
  (do (without gc)
    (+= counter 40)
  )
  (print counter)
  (print (call bump 5))
  (print (call shift (call Point 1 2)))
  (let left 1)
  (let right 2)
  (do (without gc)
    (= (tuple left right) (tuple right left))
  )
  (print left)
  (print right)
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)

(fn-do + bump I64
  (param seed I64)
  (do (without gc)
    (+= seed 10)
  )
  (return seed)
)

(fn-do + shift Int
  (param point Point)
  (do (without gc)
    (+= (. point x) 100)
  )
  (return (. point x))
)
```

```output
42
15
101
2
1
```

### A GC-free block narrows outer locals as a plain block does

```brix
(main
  (let known (? Int) 4)
  (let seen 0)
  (if known
    (do (without gc)
      (= seen (+ known 1))
      (= known 9)
    )
    (print seen)
    (print (+ known 1))
  )
  (let unset (? Int) null)
  (do (without gc)
    (= unset 7)
  )
  (print (+ unset 1))
  (let cleared (? I64) 3)
  (if cleared
    (do (without gc)
      (= cleared null)
    )
    (print (?? cleared -1))
  )
)
```

```output
5
10
8
-1
```

### A GC-free block leaves the fields of an outer struct it assigns as a plain block does

```brix
(main
  (let holder (call Holder 3))
  (if (. holder value)
    (do (without gc)
      (= (. holder value) null)
    )
    (print (?? (. holder value) -1))
  )
  (do (without gc)
    (= (. holder value) 7)
  )
  (print (?? (. holder value) -1))
)

(type + Holder (is Struct)
  (prop + value (? Int))
)
```

```output
-1
7
```

```brix crash
(main
  (let name "before")
  (let other "after")
  (do (without gc)
    (= name other)
  )
  (print name)
)
```

```output
Exception: storing a managed pointer is not allowed in a GC-free zone (entered at no file:5:3); compute into a zone buffer and publish after the zone exits: main (do without gc) → (= name other)
<no file:6>
        (= name other)
        ^^^^^^^^^^^^^^
```

### A runtime function that suspends the task, called where the code may not suspend

```brix compile-error
(import /BxEvents/BxEvents.h)
(blank)

(main
  (print (call direct (call C:brixNewMutex)))
)

(fn-do + direct Int (without suspend)
  (param native (of CPointer Char8))
  (call lock native)
  (return 1)
)

(fn-do + lock _
  (param native (of CPointer Char8))
  (call C:brixLockMutex native)
)
```

```output
Exception: cannot suspend under 'without suspend': direct (without suspend) → lock → (call C:brixLockMutex native)
<no file:17>
      (call C:brixLockMutex native)
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### An interface member whose implementation suspends the task, called where the code may not suspend

```brix compile-error
(import /BxEvents/BxEvents.h)
(blank)

(main
  (let source (as (call Impl) Source))
  (print (call direct source))
)

(type + Source (is Interface)
  (fn-do + take Int)
)

(type + Impl
  (implements Source)
  (prop _ native (of CPointer Char8) (= (call C:brixNewMutex)))
  (fn-do + take Int
    (call C:brixLockMutex native)
    (return 1)
  )
)

(fn-do + direct Int (without suspend)
  (param source Source)
  (return (call (. source take)))
)
```

```output
Exception: cannot suspend under 'without suspend': direct (without suspend) → Source.take → Impl.take → (call C:brixLockMutex native)
<no file:18>
        (call C:brixLockMutex native)
        ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A runtime function that makes text allocates

```brix compile-error
(import /BxCore/BxNumber.h)
(blank)

(main
  (print (call outer 3))
)

(fn-do + outer Int (without heap)
  (param n Int)
  (let text (call C:brixIntToString n 0))
  (return n)
)
```

```output
Exception: allocation is not allowed under 'without heap': outer (without heap) → (call C:brixIntToString n 0)
<no file:11>
      (let text (call C:brixIntToString n 0))
                ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A lambda that captures a local allocates its environment

```brix compile-error
(main
  (print (call outer 3))
)

(fn-do + apply Int
  (param action _)
  (return (call action))
)

(fn-do + outer Int (without heap)
  (param n Int)
  (return (call apply (=> () (+ n 1))))
)
```

```output
Exception: allocation is not allowed under 'without heap': outer (without heap) → (=> () (+ n 1))
<no file:13>
      (return (call apply (=> () (+ n 1))))
                          ^^^^^^^^^^^^^^^
```

### A converter an implicit conversion runs is typed under the caller's restriction

```brix compile-error
(main
  (let label (call Label 3))
  (print (call outer label))
)

(type + Box
  (prop + value Int (= 0))
)

(type + Label
  (prop + count Int)
  (fn-do + This _
    (param count _)
  )
  (fn-to-expr + Box
    (call Box)
  )
)

(fn-do + outer Int (without heap)
  (param label Label)
  (let box (as label Box))
  (return (. label count))
)
```

```output
Exception: allocation is not allowed under 'without heap': outer (without heap) → the converter of Label → (call Box)
<no file:17>
        (call Box)
        ^^^^^^^^^^
```

### Every allocation is reported, each with the chain from the restricted function

```brix compile-error
(main
  (print (call outer 3))
)

(type + Box
  (prop + value Int (= 0))
)

(fn-do + helper Int
  (param n Int)
  (let box (call Box))
  (return (+ (. box value) n))
)

(fn-do + outer Int (without heap)
  (param n Int)
  (let first (call Box))
  (let second (call Box))
  (return (+ (call helper n) (. first value) (. second value)))
)
```

```output
Exception: allocation is not allowed under 'without heap': outer (without heap) → helper → (call Box)
<no file:12>
      (let box (call Box))
               ^^^^^^^^^^

Exception: allocation is not allowed under 'without heap': outer (without heap) → (call Box)
<no file:18>
      (let first (call Box))
                 ^^^^^^^^^^

Exception: allocation is not allowed under 'without heap': outer (without heap) → (call Box)
<no file:19>
      (let second (call Box))
                  ^^^^^^^^^^
```

### A struct made a value of an interface is boxed; an enum is not

```brix compile-error
(main
  (print (call outer 3))
)

(type + Weighted (is Interface)
  (fn-expr + weight Int :get)
)

(type + Fill (is U32)
  (implements Weighted)
  (variant empty (= 0))
  (variant full (= 1))
  (fn-expr + weight Int :get
    1
  )
)

(type + Pair (is Struct)
  (implements Weighted)
  (prop + a Int)
  (fn-expr + weight Int :get
    a
  )
)

(fn-do + weigh Int
  (param item Weighted)
  (return (. item weight))
)

(fn-do + outer Int (without heap)
  (param n Int)
  (let total (call weigh (. Fill full)))
  (return (+ total (call weigh (call Pair n))))
)
```

```output
Exception: allocation is not allowed under 'without heap': outer (without heap) → (call Pair n)
<no file:35>
      (return (+ total (call weigh (call Pair n))))
                                   ^^^^^^^^^^^^^
```

### A view of an array's rows makes the owner the runtime tracks it by

```brix compile-error
(main
  (let items (call (of Array Point) 2))
  (print (call outer items))
)

(type + Point (is Struct)
  (prop + x Int)
)

(fn-do + outer Int (without heap)
  (param items (of Array Point))
  (let rows (. items rows))
  (return 0)
)
```

```output
Exception: allocation is not allowed under 'without heap': outer (without heap) → (. items rows)
<no file:13>
      (let rows (. items rows))
                ^^^^^^^^^^^^^^
```

### A generator makes its frame where it is called

```brix compile-error
(main
  (let made (call outer 3))
  (print made)
)

(fn-do + counted _
  (param n Int)
  (yield n)
)

(fn-do + outer (of Generator Int) (without heap)
  (param n Int)
  (return (call counted n))
)
```

```output
Exception: allocation is not allowed under 'without heap': outer (without heap) → counted
<no file:7>
    (fn-do + counted _
    ^^^^^^^^^^^^^^^^^
```

### A task or a GC-free block that captures a local is given a capture the collector owns

```brix compile-error
(main
  (print (call outer 3))
)

(fn-do + outer Int (without heap)
  (param n Int)
  (let total n)
  (do-new
    (+= total 1)
  )
  (do (without gc)
    (+= total 1)
  )
  (return total)
)
```

```output
Exception: allocation is not allowed under 'without heap': outer (without heap) → (do-new …)
<no file:9>
      (do-new
      ^^^^^^

Exception: allocation is not allowed under 'without heap': outer (without heap) → (do (without gc) …)
<no file:12>
      (do (without gc)
      ^^^^^^^^^^^^^^^
```

### A method bound to its object and made a function value is copied; a function is not

```brix compile-error
(main
  (let made (call outer (call Adder)))
  (print (call made 2))
)

(type + Maker (is (fn-type Int (param n Int))))

(type + Adder
  (prop + base Int (= 1))
  (fn-do + add Int
    (param n Int)
    (return (+ base n))
  )
)

(fn-do + twice Int
  (param n Int)
  (return (* n 2))
)

(fn-do + outer Maker (without heap)
  (param adder Adder)
  (let plain Maker twice)
  (return (. adder add))
)
```

```output
Exception: allocation is not allowed under 'without heap': outer (without heap) → (. adder add)
<no file:25>
      (return (. adder add))
              ^^^^^^^^^^^^^
```

### A lazy global's first read runs its value, and may wait for another task; a plain global is set when its module starts

```brix compile-error
(main
  (print (call heavy 3))
  (print (call waits 1))
)

(blank)
(prop _ names (list String) (:= (list "a" "b")))
(prop _ plain (list String) (= (list "c")))

(fn-do + heavy Int (without heap)
  (param n Int)
  (return (+ (. names length) (. plain length) n))
)

(fn-do + waits Int (without suspend)
  (param n Int)
  (return (+ (. names length) n))
)
```

```output
Exception: cannot read lazy global 'names' under 'without heap', since its first read runs its value: heavy (without heap) → names
<no file:13>
      (return (+ (. names length) (. plain length) n))
                    ^^^^^

Exception: cannot read lazy global 'names' under 'without suspend', since its first read runs its value: waits (without suspend) → names
<no file:18>
      (return (+ (. names length) n))
                    ^^^^^
```

### A C function that polls the collector and has no claim is taken to suspend; slicing text allocates

```brix compile-error
(import /BxCore/BxCore.h)
(import /BxCore/BxString.h)
(blank)

(main
  (print (call idle 3))
  (print (call slices "abc"))
)

(fn-do + idle Int (without suspend)
  (param n Int)
  (let collected (call C:brixGCRuntimeIdleCollect 0u64))
  (return n)
)

(fn-do + slices String (without heap)
  (param text String)
  (return (as (call C:brixStringSlice text 0 1) String))
)
```

```output
Exception: cannot suspend under 'without suspend': idle (without suspend) → (call C:brixGCRuntimeIdleCollect 0u64)
<no file:13>
      (let collected (call C:brixGCRuntimeIdleCollect 0u64))
                     ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Exception: allocation is not allowed under 'without heap': slices (without heap) → (call C:brixStringSlice text 0 1)
<no file:19>
      (return (as (call C:brixStringSlice text 0 1) String))
                  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A task that captures nothing is spawned with no allocation from the collector

```brix
(main
  (print (call outer 3))
)

(fn-do + outer Int (without heap)
  (param n Int)
  (do-new
    (let unused 1)
  )
  (return n)
)
```

```output
3
```

### A task's handler takes the panic it handles into the collector's heap

```brix compile-error
(main
  (print (call outer 3))
)

(fn-do + outer Int (without heap)
  (param n Int)
  (do-new
    (let unused 1)
    (on-failure Panic
      (let handled 2)
    )
  )
  (return n)
)
```

```output
Exception: allocation is not allowed under 'without heap': outer (without heap) → (do-new …)
<no file:8>
      (do-new
      ^^^^^^
```

### A without section restricts a block or a function's header, and code that keeps to it runs.

```brix
(main
  (let seed 5)
  (do (without gc)
    (let value (+ seed 7))
  )
  (print (call total 3))
)

(fn-do + total Int (without heap suspend)
  (param count Int)
  (let sum 0)
  (for (in value (.. 1 count))
    (+= sum value)
  )
  (return sum)
)
```

```output
6
```

### A do block kept from the heap, from suspending, from safepoints or from the probes runs in place.

```brix
(main
  (let count 4)
  (let sum 0)
  (do (without heap)
    (for (in value (.. 1 count))
      (+= sum value)
    )
  )
  (print sum)
  (do (without suspend)
    (+= sum (call total count))
  )
  (print sum)
  (do (without safepoints probe-safepoints)
    (for (in value (.. 1 count))
      (+= sum value)
    )
  )
  (print sum)
  (do (without heap suspend)
    (+= sum (call total 2))
  )
  (print sum)
)

(fn-do + total Int
  (param count Int)
  (let sum 0)
  (for (in value (.. 1 count))
    (+= sum value)
  )
  (return sum)
)
```

```output
10
20
30
33
```

### A cleanup opened outside a do block runs as it was opened, where the block is left.

```brix
(main
  (print (call early 1))
  (print (call early 0))
)

(fn-do + early Int
  (param flag Int)
  (finally
    (let task
      (do-new
        (print flag)
      )
    )
    (call (. task wait))
  )
  (do (without heap)
    (if (== flag 1)
      (return 3)
    )
  )
  (return 4)
)
```

```output
1
3
0
4
```

### Waiting for a task suspends

```brix compile-error
(import /BxEvents/BxEvents.h)
(blank)

(main
  (do (without suspend)
    (call C:brixWaitForTask 0)
  )
)
```

```output
Exception: cannot suspend under 'without suspend': main (do without suspend) → (call C:brixWaitForTask 0)
<no file:7>
        (call C:brixWaitForTask 0)
        ^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Cancelling a task yields to it, so it suspends

```brix compile-error
(import /BxEvents/BxEvents.h)
(blank)

(main
  (do (without suspend)
    (call C:brixCancelTask 0)
  )
)
```

```output
Exception: cannot suspend under 'without suspend': main (do without suspend) → (call C:brixCancelTask 0)
<no file:7>
        (call C:brixCancelTask 0)
        ^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix compile-error
(main
  (let sum 0)
  (do (without heap)
    (= sum (. (call Box) value))
  )
  (print sum)
)

(type + Box
  (prop + value Int (= 0))
)
```

```output
Exception: allocation is not allowed under 'without heap': main (do without heap) → (call Box)
<no file:5>
        (= sum (. (call Box) value))
                  ^^^^^^^^^^
```

```brix compile-error
(main
  (print (call boxed))
)

(fn-do + boxed Int (without heap)
  (return (. (call Box) value))
)

(type + Box
  (prop + value Int (= 0))
)
```

```output
Exception: allocation is not allowed under 'without heap': boxed (without heap) → (call Box)
<no file:7>
      (return (. (call Box) value))
                 ^^^^^^^^^^
```

```brix compile-error
(main
  (do (without heap network)
    (let one 1)
  )
)
```

```output
Exception: Unknown name 'network' after without
<no file:3>
      (do (without heap network)
      ^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix compile-error
(main
  (print 1)
)

(fn-do + run _ (without cancel)
  (print 2)
)
```

```output
Exception: 'without cancel' is only for a do block
<no file:6>
    (fn-do + run _ (without cancel)
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A task an object owns allocates the record the object keeps of it, so a method under without heap cannot start one, though it can start a task marked @detached that captures nothing

```brix compile-error
(main
  (let holder (call Holder))
  (call (. holder start))
)

(type + Holder
  (fn-do + start _ (without heap)
    (do-new
      (print "owned")
    )
    (do-new @detached
      (print "detached")
    )
  )
  (fn-do + close _)
)
```

```output
Exception: allocation is not allowed under 'without heap': Holder.start (without heap) → (do-new …)
<no file:9>
        (do-new
        ^^^^^^
```
