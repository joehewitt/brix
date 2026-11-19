# Returns

Errors about returns and the errors a function raises name the function and the declaration to write

### a recursive function with no declared return type

```brix crash
(main
  (print (call count-down 3))
)

(fn-do + count-down _
  (param n I32)
  (if (== n 0)
    (return 0)
  )
  (return (call count-down (- n 1)))
)
```

```output
Exception: Function "count-down" calls itself before its return type is known, so the type cannot be inferred; declare it, as in "(fn-do + count-down I64 …)"
<no file:6>
    (fn-do + count-down _
    ^^^^^^^^^^^^^^^^^^^^
```

### returns that have no common type

```brix crash
(main
  (print (call pick 2))
)

(type + Box)

(fn-do + pick _
  (param x I32)
  (if (> x 1)
    (return (call Box))
  )
  (return 2.5)
)
```

```output
Exception: Function "pick" returns Box here and F64 elsewhere, which have no common type; return one type, or declare the return type
<no file:11>
        (return (call Box))
        ^^^^^^^^^^^^^^^^^^^
```

### an error a call raises that the calling function does not declare

```brix crash
(main
  (print (call outer))
)

(fn-do + outer I32 (?! TypeError)
  (return (call load))
)

(fn-do + load I32 (?! IOError)
  (throw (call IOError "disk"))
)
```

```output
Exception: IOError can leave "outer", which does not declare it; declare it, as in "(fn-do + outer I32 (?! TypeError) (?! IOError) …)", or catch it here
<no file:7>
      (return (call load))
              ^^^^^^^^^^^
```

### an error a function raises but does not declare

```brix crash
(main
  (print (call load))
)

(fn-do + load I32 (?! TypeError)
  (throw (call IOError "disk"))
)
```

```output
Exception: IOError can leave "load", which does not declare it; declare it, as in "(fn-do + load I32 (?! TypeError) (?! IOError) …)"
<no file:7>
      (throw (call IOError "disk"))
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### an error a recursive function raises before its errors are inferred

```brix crash
(main
  (print (call walk 3))
)

(fn-do + walk Bool
  (param n I32)
  (if (== n 0)
    (throw (call IOError "disk"))
  )
  (return (call walk (- n 1)))
)
```

```output
Exception: IOError can leave "walk", which calls itself, so its errors cannot be inferred; declare them on it, as in "(fn-do + walk Bool (?! IOError) …)", or catch them here
<no file:9>
        (throw (call IOError "disk"))
        ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### a method that can fail implementing an interface method that cannot

```brix crash
(main
  (let p (as (call Thing) Proto))
  (print (call (. p foo)))
)

(type + Proto (is Interface)
  (fn-expr + foo I32)
)

(type + Thing
  (implements Proto)
  (fn-expr + foo _
    (throw (call Error "fooey"))
  )
)
```

```output
Exception: "foo" can fail with Error, but the interface method it implements cannot; add "?! Error" to that method, or catch the error inside "foo"
<no file:13>
      (fn-expr + foo _
      ^^^^^^^^^^^^^^^
Interface:
<no file:8>
      (fn-expr + foo I32)
```

### a catch attached to an expression that cannot fail

```brix crash
(main
  (let value
    (scope 42
      (on-error Error
        0
      )
    )
  )
  (print value)
)
```

```output
Exception: "42" cannot fail, so the catch attached to it has nothing to handle; remove the catch, or attach it to a call that can fail
<no file:4>
        (scope 42
               ^
```

### a value returned from a class's event handler

```brix crash
(main
  (let spot (call Spot))
  (<- (. spot pings) true)
)

(type + Spot
  (prop + taps Size (= 0))
  (event + pings Bool)
  (subscribe-for (on event pings)
    (return-if (if (not event)))
    (+= taps 1)
  )
)
```

```output
Exception: The event handler for "pings" cannot return with "="; move its body into a method, as in "_ do handle-pings", and call that, or end the handler early with "continue"
<no file:11>
        (return-if (if (not event)))
        ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```
