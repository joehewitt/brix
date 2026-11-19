# Comprehension indexing

```brix
(main
  (let items (call (of List String) "a" "b" "c"))
  (let picked (call (of List String) (in ([] items each) (..< 1 3))))
  (print (call (. " " join) (in each picked)))
)
```

```output
b c
```

```brix
(main
  (let bag (call Bag))
  (+= (. bag items) "a")
  (+= (. bag items) "b")
  (+= (. bag items) "c")
  (print (call (. " " join) (in each (call (. bag middle) 0 2))))
)

(type + Bag
  (prop + items (list String) (= (call (of List String))))
  (fn-do + middle (list String)
    (param start Index)
    (param end Index)
    (return (call (of List String) (in ([] items each) (..< start end))))
  )
)
```

```output
a b
```

```brix
(main
  (let bag (call Bag))
  (+= (. bag items) "a")
  (+= (. bag items) "b")
  (+= (. bag items) "c")
  (print (call (. " " join) (in each (call (. bag upto) 2))))
)

(type + Bag
  (prop + items (list String) (= (call (of List String))))
  (fn-do + upto (list String)
    (param index Index)
    (return (call (of List String) (in ([] items each) (..< 0 index))))
  )
)
```

```output
a b
```

```brix
(main
  (let bag (call Bag))
  (+= (. bag items) "a")
  (+= (. bag items) "b")
  (+= (. bag items) "c")
  (print (call (. " " join) (in each (call (. bag upto) 2))))
)

(type + Bag
  (prop + items (list String) (= (call (of List String))))
  (fn-do + upto (list String)
    (param count Index)
    (for (in item items)
      (if (> (. item length) 0)
        (return (call (of List String) (in ([] items each) (..< 0 count))))
      )
    )
    (return (call (of List String)))
  )
)
```

```output
a b
```

```brix
(main
  (let bag (call Bag))
  (+= (. bag items) "a")
  (+= (. bag items) "b")
  (+= (. bag items) "c")
  (+= (. bag ranges) (call Stretch 1 3))
  (print (call (. " " join) (in each (call (. bag stretch) 0))))
)

(type + Stretch (is Tuple)
  (prop + start Index)
  (prop + end Index)
)

(type + Bag
  (prop + items (list String) (= (call (of List String))))
  (prop + ranges (list Stretch) (= (call (of List Stretch))))
  (fn-do + stretch (list String)
    (param index Index)
    (for (in range ranges)
      (if (> (. range end) 0)
        (let picked (call (of List String) (in ([] items each) (..< (. ([] ranges index) start) (. ([] ranges index) end)))))
        (+= picked "!")
        (return picked)
      )
    )
    (return (call (of List String)))
  )
)
```

```output
b c !
```
