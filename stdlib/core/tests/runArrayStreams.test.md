# Array streams

```brix
(import /core/array-builder)

(main
  (let mutable (call Array (*> (.. 1 3) (* each 2))))
  (let immutable (call ImmutableArray (*> (.. 1 3) (* each 3)) 20))
  (print (type-of mutable))
  (print (type-of immutable))
  (print (call (. ", " join) mutable))
  (print (call (. ", " join) immutable))
  (let empty (call Array (?> (.. 1 3) false) 100))
  (print (tuple (. empty length) (. empty data address)))
)
```

```output
[TypeInfo Array'I64]
[TypeInfo ImmutableArray'I64'GCBacking]
2, 4, 6
3, 6, 9
(0, 0)
```

```brix
(import /core/array-builder)
(import /core/arena)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (let initial-bytes (. arena active-bytes))
  (let empty (call Array arena (?> (.. 1 3) false) 100))
  (let frozen (call ImmutableArray arena (?> (.. 1 3) false) 100))
  (print (tuple (. empty length) (. empty data address) (. frozen length)))
  (print (== (. arena active-bytes) initial-bytes))
  (let mutable (call Array arena (*> (.. 1 3) (* each 2))))
  (let immutable (call ImmutableArray arena (*> (.. 1 3) (* each 3)) 20))
  (print (type-of mutable))
  (print (type-of immutable))
  (print (call (. ", " join) mutable))
  (print (call (. ", " join) immutable))
)
```

```output
(0, 0, 0)
true
[TypeInfo Array'I64'ArenaBacking]
[TypeInfo ImmutableArray'I64'ArenaBacking]
2, 4, 6
3, 6, 9
```

```brix
(import /core/array-builder)

(main
  (let values (call Array (*> (..< 1 1) each) (. Size max)))
  (let frozen (call ImmutableArray (*> (..< 1 1) each) (. Size max)))
  (print (tuple (. values length) (. values data address) (. frozen length)))
  (for (in item values)
    (print "unexpected")
  )
  (for (in item frozen)
    (print "unexpected")
  )
  (print (tuple (. ([] values (..< 0 0)) length) (. ([] frozen (..< 0 0)) length)))
  (print (. (call (. values copy)) length))
  (print (. values immutable length))
  (print (. (as values (of Array Byte)) length))
  (let grown (call (. values resize) 2))
  (= ([] grown 0) 7)
  (= ([] grown 1) 9)
  (print (call (. ", " join) grown))
)
```

```output
(0, 0, 0)
(0, 0)
0
0
0
7, 9
```

```brix
(import /core/array-builder)
(import /core/arena)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (let values (call Array arena (*> (..< 1 1) each) (. Size max)))
  (let frozen (call ImmutableArray arena (*> (..< 1 1) each) (. Size max)))
  (print (tuple (. values length) (. values data address) (. frozen length)))
  (for (in item values)
    (print "unexpected")
  )
  (for (in item frozen)
    (print "unexpected")
  )
  (print (tuple (. ([] values (..< 0 0)) length) (. ([] frozen (..< 0 0)) length)))
  (print (. (call (. values copy) arena) length))
  (print (. values immutable length))
  (print (. (as values (of Array Byte ArenaBacking)) length))
  (let grown (call (. values resize) 2 arena))
  (= ([] grown 0) 7)
  (= ([] grown 1) 9)
  (print (call (. ", " join) grown))
)
```

```output
(0, 0, 0)
(0, 0)
0
0
0
7, 9
```

```brix
(import /core/array-builder)

(main
  (for (in capacity (list 0 1 65 256))
    (let mutable (call Array (call source) (as capacity Size)))
    (let frozen (call ImmutableArray (call source) (as capacity Size)))
    (print (tuple (. mutable length) (. frozen length)))
    (let total 0)
    (for (in item mutable)
      (+= total item)
    )
    (for (in item frozen)
      (+= total item)
    )
    (print (tuple total ([] mutable 64) ([] frozen 64)))
  )
  (let singleton (call Array (*> (.. 7 7) each)))
  (print (tuple (. singleton length) ([] singleton 0)))
)

(fn-do + source _ @inline
  (for (in i (..< 0 65))
    (yield i)
  )
)
```

```output
(65, 65)
(4160, 64, 64)
(65, 65)
(4160, 64, 64)
(65, 65)
(4160, 64, 64)
(65, 65)
(4160, 64, 64)
(1, 7)
```

```brix
(import /core/array-builder)
(import /core/arena)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (for (in capacity (list 0 1 65 256))
    (let mutable (call Array arena (call source) (as capacity Size)))
    (let frozen (call ImmutableArray arena (call source) (as capacity Size)))
    (print (tuple (. mutable length) (. frozen length)))
    (let total 0)
    (for (in item mutable)
      (+= total item)
    )
    (for (in item frozen)
      (+= total item)
    )
    (print (tuple total ([] mutable 64) ([] frozen 64)))
  )
  (let singleton (call Array arena (*> (.. 7 7) each)))
  (print (tuple (. singleton length) ([] singleton 0)))
)

(fn-do + source _ @inline
  (for (in i (..< 0 65))
    (yield i)
  )
)
```

```output
(65, 65)
(4160, 64, 64)
(65, 65)
(4160, 64, 64)
(65, 65)
(4160, 64, 64)
(65, 65)
(4160, 64, 64)
(1, 7)
```

```brix
(import /core/array-builder)
(import /BxGC/BxGC.h)
(import /core/arena)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (let allocated (call C:brixGCAllocatedObjectCount))
  (let a (call Array (*> (..< 1 1) each) (. Size max)))
  (let b (call ImmutableArray (?> (.. 1 3) false) (. Size max)))
  (let c (call Array arena (*> (..< 1 1) each) (. Size max)))
  (let d (call ImmutableArray arena (?> (.. 1 3) false) (. Size max)))
  (let delta (- (call C:brixGCAllocatedObjectCount) allocated))
  (print delta)
  (print (tuple (. arena active-bytes) (. arena system-block-allocation-count)))
  (print (tuple (. a length) (. b length) (. c length) (. d length)))
)
```

```output
0
(0, 0)
(0, 0, 0, 0)
```

```brix
(import /core/array-builder)

(main
  (let mutable
    (block
      (spread (call source))
      (pipe *> (* each 2))
      (pipe ?> (> each 2))
      (pipe |> Array)
    )
  )
  (let frozen
    (block
      (spread (call source))
      (item 5)
      (pipe |> ImmutableArray)
    )
  )
  (print (call (. ", " join) mutable))
  (print (call (. ", " join) frozen))
)

(fn-do + source _ @inline
  (finally
    (print "closed")
  )
  (for (in i (.. 1 3))
    (print (str "yield " i))
    (yield i)
  )
)
```

```output
yield 1
yield 2
yield 3
closed
yield 1
yield 2
yield 3
closed
4, 6
1, 2, 3, 5
```

```brix
(import /core/array-builder)
(import /BxGC/BxGC.h)

(main
  (let first (call ArrayStreamBox "first"))
  (let second (call ArrayStreamBox "second"))
  (let mutable (call Array (call source first second) 256))
  (let frozen (call ImmutableArray (call source first second) 256))
  (call C:brixGCCollect)
  (= (. ([] mutable 0) text) "changed")
  (print (tuple (. ([] frozen 0) text) (. first text)))
  (print (tuple (. ([] mutable 1) text) (. ([] frozen 1) text)))
)

(type + ArrayStreamBox
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(fn-do + source _ @inline
  (param first ArrayStreamBox)
  (param second ArrayStreamBox)
  (yield first)
  (call C:brixGCCollect)
  (yield second)
)
```

```output
(changed, changed)
(second, second)
```

```brix
(import /core/array-builder)
(import /BxGC/BxGC.h)
(import /core/arena)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (let first (call ArrayStreamBox "first"))
  (let second (call ArrayStreamBox "second"))
  (let mutable (call Array arena (call source first second) 256))
  (let frozen (call ImmutableArray arena (call source first second) 256))
  (call C:brixGCCollect)
  (= (. ([] mutable 0) text) "changed")
  (print (tuple (. ([] frozen 0) text) (. first text)))
  (print (tuple (. ([] mutable 1) text) (. ([] frozen 1) text)))
)

(type + ArrayStreamBox
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(fn-do + source _ @inline
  (param first ArrayStreamBox)
  (param second ArrayStreamBox)
  (yield first)
  (call C:brixGCCollect)
  (yield second)
)
```

```output
(changed, changed)
(second, second)
```

```brix
(import /core/array-builder)

(main
  (let result (catch (call collect)))
  (print (is result ArrayStreamFailure))
)

(type + ArrayStreamFailure (is Error))

(fn-do + collect Int (?! ArrayStreamFailure)
  (let values (call Array (call source)))
  (return (as (. values length) Int))
)

(fn-do + source (stream Int) (?! ArrayStreamFailure) @inline
  (finally
    (print "closed")
  )
  (yield 1)
  (print "fail")
  (throw (call ArrayStreamFailure "failed"))
  (yield 2)
)
```

```output
fail
closed
true
```

```brix
(import /core/array-builder)

(main
  (let result (catch (call collect)))
  (print (is result ArrayStreamFailure))
)

(type + ArrayStreamFailure (is Error))

(fn-do + collect Int (?! ArrayStreamFailure)
  (let values (call ImmutableArray (call source)))
  (return (as (. values length) Int))
)

(fn-do + source (stream Int) (?! ArrayStreamFailure) @inline
  (finally
    (print "closed")
  )
  (yield 1)
  (print "fail")
  (throw (call ArrayStreamFailure "failed"))
  (yield 2)
)
```

```output
fail
closed
true
```

```brix
(import /core/array-builder)

(main
  (let attempt
    (do-new
      (let values (call Array (*> (..< 1 1) each)))
      (print ([] values 0))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)
```

```output
Index out of bounds (0 / 0)
```

```brix
(import /core/array-builder)

(main
  (let attempt
    (do-new
      (let values (call Array (*> (..< 1 1) each)))
      (print (= ([] values 0) 1))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)
```

```output
Set out of bounds (0 / 0)
```

```brix
(import /core/array-builder)

(main
  (let attempt
    (do-new
      (let values (call Array (*> (..< 1 1) each)))
      (print ([] values (.. 0 1)))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)
```

```output
Range out of bounds (1 / 0)
```

```brix
(import /core/array-builder)
(import /core/arena)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (let attempt
    (do-new
      (let values (call Array arena (*> (..< 1 1) each)))
      (print ([] values 0))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)
```

```output
Index out of bounds (0 / 0)
```

```brix
(import /core/array-builder)
(import /core/arena)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (let attempt
    (do-new
      (let values (call Array arena (*> (..< 1 1) each)))
      (print (= ([] values 0) 1))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)
```

```output
Set out of bounds (0 / 0)
```

```brix
(import /core/array-builder)
(import /core/arena)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (let attempt
    (do-new
      (let values (call Array arena (*> (..< 1 1) each)))
      (print ([] values (.. 0 1)))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)
```

```output
Range out of bounds (1 / 0)
```

```brix
(import /core/array-builder)
(import /core/arena)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (let mutable
    (block
      (spread (.. 1 3))
      (pipe *> (* each 2))
      (pipe |> (call Array arena ...))
    )
  )
  (let frozen
    (block
      (spread (.. 1 3))
      (pipe *> (* each 3))
      (pipe |> (call ImmutableArray arena ...))
    )
  )
  (print (call (. ", " join) mutable))
  (print (call (. ", " join) frozen))
)
```

```output
2, 4, 6
3, 6, 9
```

```brix
(import /core/array-builder)
(import /BxGC/BxGC.h)

(main
  (let mutable (call Array (call source)))
  (let frozen (call ImmutableArray (call source) 256))
  (call C:brixGCCollect)
  (print (tuple (. mutable length) (. frozen length)))
  (print (tuple (. ([] mutable 0) text) (. ([] frozen 64) text)))
)

(type + ArrayStreamBox
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(fn-do + source _ @inline
  (for (in i (..< 0 65))
    (yield (call ArrayStreamBox (str "item " i)))
    (call C:brixGCCollect)
  )
)
```

```output
(65, 65)
(item 0, item 64)
```

```brix
(import /core/array-builder)
(import /core/arena)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (let result (catch (call collect arena)))
  (print (is result ArrayStreamFailure))
)

(type + ArrayStreamFailure (is Error))

(fn-do + collect Int (?! ArrayStreamFailure)
  (param arena Arena)
  (let values (call Array arena (call source) 20))
  (return (as (. values length) Int))
)

(fn-do + source (stream Int) (?! ArrayStreamFailure) @inline
  (finally
    (print "closed")
  )
  (yield 1)
  (print "fail")
  (throw (call ArrayStreamFailure "failed"))
  (yield 2)
)
```

```output
fail
closed
true
```

```brix
(import /core/array-builder)
(import /core/arena)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (let result (catch (call collect arena)))
  (print (is result ArrayStreamFailure))
)

(type + ArrayStreamFailure (is Error))

(fn-do + collect Int (?! ArrayStreamFailure)
  (param arena Arena)
  (let values (call ImmutableArray arena (call source) 20))
  (return (as (. values length) Int))
)

(fn-do + source (stream Int) (?! ArrayStreamFailure) @inline
  (finally
    (print "closed")
  )
  (yield 1)
  (print "fail")
  (throw (call ArrayStreamFailure "failed"))
  (yield 2)
)
```

```output
fail
closed
true
```

```brix
(import /core/array-builder)
(import /BxGC/BxGC.h)

(main
  (for (in capacity (list 65 256))
    (let input (call source))
    (let initial-count (call C:brixGCAllocatedObjectCount))
    (let initial-bytes (call C:brixGCRequestedByteCount))
    (let values (call Array input (as capacity Size)))
    (let count (- (call C:brixGCAllocatedObjectCount) initial-count))
    (let bytes (- (call C:brixGCRequestedByteCount) initial-bytes))
    (print (tuple count bytes (. values length) ([] values 64)))
  )
)

(fn-do + source _
  (for (in i (..< 0 65))
    (yield i)
  )
)
```

```output
(2, 552, 65, 64)
(2, 2080, 65, 64)
```

```brix
(import /core/array-builder)
(import /BxGC/BxGC.h)

(main
  (for (in capacity (list 65 256))
    (let input (call source))
    (let initial-count (call C:brixGCAllocatedObjectCount))
    (let initial-bytes (call C:brixGCRequestedByteCount))
    (let values (call ImmutableArray input (as capacity Size)))
    (let count (- (call C:brixGCAllocatedObjectCount) initial-count))
    (let bytes (- (call C:brixGCRequestedByteCount) initial-bytes))
    (print (tuple count bytes (. values length) ([] values 64)))
  )
)

(fn-do + source _
  (for (in i (..< 0 65))
    (yield i)
  )
)
```

```output
(2, 552, 65, 64)
(2, 2080, 65, 64)
```
