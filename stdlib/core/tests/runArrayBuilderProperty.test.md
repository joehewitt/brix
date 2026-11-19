# Array builder property

```brix
(import /core/array-builder)
(blank)

(main
  (let holder (call Holder))
  (call (. holder append) 7)
  (call (. holder append) 9)
  (print (. holder builder length))
  (let result (call (. holder finish)))
  (print (. result length))
  (print result)
)

(type + Holder
  (prop + builder _ (:= (call (of ArrayBuilder I32))))
  (fn-do + append _
    (param value I32)
    (call (. builder append) value)
  )
  (fn-do + finish (of ImmutableArray I32)
    (return (call (. builder finish-immutable)))
  )
)
```

```output
2
2
7, 9
```

```brix
(import /core/array-builder)
(blank)

(main
  (let holder (as (call Derived) Base))
  (call (. holder builder append) 7)
  (print (. holder builder length))
  (print (call (. holder builder finish-immutable)))
)

(type + Base
  (prop + builder _ (:= (call (of ArrayBuilder I32))))
)

(type + Derived (is Base)
  (prop + builder _ (:= (call seeded)))
)

(fn-do + seeded (of ArrayBuilder I32)
  (let result (call (of ArrayBuilder I32)))
  (call (. result append) 9)
  (return result)
)
```

```output
2
9, 7
```

```brix
(import /core/array-builder)
(blank)

(main
  (let holder (call Holder))
  (call (. holder builder append) 7)
  (let copy (. holder builder))
  (call (. copy append) 9)
  (print (. holder builder length))
  (print (. copy length))
)

(type + Holder
  (prop + builder _ (:= (call (of ArrayBuilder I32))))
)
```

```output
2
2
```

```brix
(import /core/array-builder)
(blank)

(main
  (let holder (call Holder))
  (call (. holder builder reserve) 32)
  (call (. holder builder append) 7)
  (print (. holder builder length))
  (print (call (. holder builder finish-immutable)))
  (print (. holder builder length))
)

(type + Holder
  (prop + builder (of ArrayBuilder I32))
  (fn-do + This _
    (= builder (call (of ArrayBuilder I32)))
  )
)
```

```output
1
7
0
```

```brix
(import /core/array-builder)
(blank)

(main
  (let builder (call (of ArrayBuilder U32)))
  (call (. builder append) 1)
  (call fill builder)
  (print (. builder length))
  (print (call (. builder finish-immutable)))
)

(fn-do _ fill _
  (param output (of ArrayBuilder U32))
  (call (. output append) 2)
  (call (. output append) 3)
)
```

```output
3
1, 2, 3
```

```brix
(import /core/array-builder)
(blank)

(main
  (let builder (call (of ArrayBuilder U32)))
  (call (. builder reserve) 8)
  (call (. builder append) 1)
  (let copy builder)
  (call (. builder append) 2)
  (let frozen (call (. builder finish-immutable)))
  (call (. copy append) 7)
  (print frozen)
  (print (. copy length))
  (print (call (. builder finish-immutable)))
)
```

```output
1, 2
1
7
```
