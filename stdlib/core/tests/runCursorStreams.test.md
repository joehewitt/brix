# Cursor streams

```brix
(main
  (let values (.. 1 3))
  (for (in item values)
    (print item)
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
  (print (call List (call Steps 3)))
)

(type + Steps (is Tuple)
  (implements (of CursorIterable Int Int))
  (prop + limit Int)
  (fn-expr + initial-iterator-state Int :get @inline
    0
  )
  (fn-do + next-iterator (tuple Int (? Int)) @inline
    (param state Int)
    (let none (? Int))
    (if (>= state limit)
      (return (tuple state none))
    )
    (return (tuple (+ state 1) (+ 2 (* state 3))))
  )
)
```

```output
2, 5, 8
```

```brix
(main
  (print (. (..< 0 0) length))
  (print (. (.. 0 0) length))
  (print (. (..< 0 6 (by: 3)) length))
  (print (. (.. 0 6 (by: 3)) length))
  (print (. (..< 0 7 (by: 3)) length))
  (print (. (.. 0 7 (by: 3)) length))
  (print (. (..< 7 0 (by: 3)) length))
  (print (. (.. 7 0 (by: 3)) length))
  (print (. (..< -5 5 (by: 3)) length))
  (print (. (.. -5 5 (by: 3)) length))
  (print (. (.. -120i8 120i8 (by: 3i8)) length))
  (print (. (..< 250u8 0u8 (by: 3u8)) length))
  (print (. (..< -2147483648 2147483647) length))
  (print (. (.. -2147483648 2147483647) length))
  (print (. (..< 0.0 1.0 (by: 0.25)) length))
  (print (. (.. 0.0 1.0 (by: 0.25)) length))
  (print (. (..< 0.0 1.0 (by: 0.3)) length))
  (print (. (.. 0.0 1.0 (by: 0.3)) length))
)
```

```output
0
1
2
3
3
3
3
3
4
4
81
84
4294967295
4294967296
4
5
4
4
```

### A loop over a struct that steps a cursor.

```brix
(main
  (let values (call CursorRange 3))
  (for (in value values)
    (print value)
  )
)

(type + CursorRange (is Struct)
  (implements (of CursorIterable I32 I32))
  (prop + limit I32)
  (fn-expr + initial-iterator-state I32 :get
    0
  )
  (fn-do + next-iterator (tuple I32 (? I32))
    (param state I32)
    (let none (? I32))
    (if (>= state limit)
      (return (tuple state none))
    )
    (return (tuple (+ state 1) (+ state 1)))
  )
)
```

```output
1
2
3
```

### An error a cursor raises as it steps leaves the function looping over it, as a call's does.

```brix crash
(main
  (call show (call CursorRange 3))
)

(fn-do + show _
  (param values CursorRange)
  (for (in value values)
    (print value)
  )
)

(type + CursorRange (is Struct)
  (implements (of CursorIterable I32 I32))
  (prop + limit I32)
  (fn-expr + initial-iterator-state I32 :get
    0
  )
  (fn-do + next-iterator (tuple I32 (? I32)) (?! Error)
    (param state I32)
    (if (>= state 2)
      (throw (call Error "stop"))
    )
    (return (tuple (+ state 1) (+ state 1)))
  )
)
```

```stderr
Unhandled error: stop
<no file:22>

Task 1

Stack trace:
  0  __source__:CursorRange:next-iterator no file:22:7
  1  __source__:show no file:8:8
  2  __source__:main no file:3:3
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```
