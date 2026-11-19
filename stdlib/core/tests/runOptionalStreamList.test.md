# Optional stream list

```brix
(main
  (let a (call Entry "a"))
  (let b (call Entry "b"))
  (let missing (? Entry))
  (let source (list (? Entry)) (list a missing b))
  (for (in item source)
    (print (if-expr item "present" "absent"))
  )
  (let entries (call collect source))
  (print (. entries length))
  (for (in entry entries)
    (print (. entry text))
  )
)

(type + Entry
  (prop + text String)
  (fn-do + This _
    (param text String)
  )
)

(fn-expr + collect _
  (param source (stream (? Entry)))
  (spread source)
  (pipe ?*> each)
  (pipe |> List)
)
```

```output
present
absent
present
2
a
b
```

```brix
(main
  (let mapped
    (block
      (spread (call values))
      (pipe *> (?? each -1))
      (pipe |> List)
    )
  )
  (let filtered
    (block
      (spread (call values))
      (pipe ?> (!= each null))
      (pipe *> (?? each -1))
      (pipe |> List)
    )
  )
  (print (call (. "," join) mapped))
  (print (call (. "," join) filtered))
)

(fn-do + values _ @inline
  (let missing (? I32))
  (yield 2)
  (yield missing)
  (yield 4)
)
```

```output
2,-1,4
2,4
```

### A stream of optional objects passes the ones that hold nothing to a loop

```brix
(main
  (let boxes (list (? Box)) (list (call Box 1) null (call Box 3)))
  (let count 0)
  (for (in box boxes)
    (+= count 1)
    (print (if-expr box (. box value) 0))
  )
  (print count)
  (print (call total boxes))
  (print (call total (call some boxes)))
)

(type + Box
  (prop + value Int)
  (fn-do + This _
    (param value _)
  )
)

(fn-do + some (stream (? Box))
  (param boxes (list (? Box)))
  (for (in box boxes)
    (yield box)
  )
  (yield null)
)

(fn-do + total Int
  (param boxes (stream (? Box)))
  (let sum 0)
  (for (in box boxes)
    (if box
      (+= sum (. box value))
      (else
        (+= sum 100)
      )
    )
  )
  (return sum)
)
```

```output
1
0
3
3
104
204
```

### A map-filter whose body is never null drops no item, and asks for a map.

```brix crash
(main
  (print (call List (?*> (list 1 2 3) (if-expr (> each 1) each null))))
  (print (call List (?*> (list 1 2 3) (* each 2))))
)
```

```output
Exception: The body of this "?*>" gives I64, which is never null, so it drops no item; write "*>"
<no file:4>
      (print (call List (?*> (list 1 2 3) (* each 2))))
                        ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```
