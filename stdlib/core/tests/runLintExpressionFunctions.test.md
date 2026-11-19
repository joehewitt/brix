# Lint expression functions

```brix
(prop _ current Int (= 1))
(blank)

(main
  (print (call eager))
  (= current 1)
  (print (call deferred))
)

(fn-do + eager Int
  (let snapshot current)
  (throw (call Panic "unexpected") (if (call advance)))
  (return snapshot)
)

(fn-expr + deferred Int
  (before
    (throw (call Panic "unexpected") (if (call advance)))
  )
  (scope snapshot
    (lazy snapshot current)
  )
)

(fn-do + advance Bool
  (= current 2)
  (return false)
)
```

```output
1
2
```

```brix
(prop _ invocations Int (= 0))
(blank)

(main
  (print (call invoke (call DefaultHook)))
  (print (call invoke (call ActiveHook)))
  (print (call invoke (call ActiveHook)))
)

(type + Hook (is Interface)
  (fn-do + value Int
    (return 0)
  )
)

(type + DefaultHook
  (implements Hook)
)

(type + ActiveHook
  (implements Hook)
  (fn-do + value Int
    (+= invocations 1)
    (return invocations)
  )
)

(fn-do + invoke Int
  (param hook Hook)
  (return (call (. hook value)))
)
```

```output
0
1
2
```

```brix
(prop _ invocations Int (= 0))
(blank)

(main
  (print (call invoke (call DefaultHook)))
  (print (call invoke (call ActiveHook)))
  (print (call invoke (call ActiveHook)))
)

(type + Hook (is Interface)
  (fn-expr + value Int
    0
  )
)

(type + DefaultHook
  (implements Hook)
)

(type + ActiveHook
  (implements Hook)
  (fn-do + value Int
    (+= invocations 1)
    (return invocations)
  )
)

(fn-do + invoke Int
  (param hook Hook)
  (return (call (. hook value)))
)
```

```output
0
1
2
```
