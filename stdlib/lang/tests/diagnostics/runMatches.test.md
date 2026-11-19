# Matches

A match that leaves cases out names the type in source form, the cases it leaves out, and the arm to add

### a match on Bool without false

```brix crash
(main
  (match true
    (case= true
      (print "true")
    )
  )
)
```

```output
Exception: This match on Bool does not handle false; add "(case= false …)", or "(default …)" for the rest
<no file:4>
        (case= true
        ^^^^^^^^^^
```

### a match on an optional without null

```brix crash
(main
  (let value (? I32) 41)
  (print
    (match value
      (case (: n I32)
        (+ n 1)
      )
    )
  )
)
```

```output
Exception: This match on I32? does not handle null; add "(case= null …)", or "(default …)" for the rest
<no file:6>
          (case (: n I32)
          ^^^^^^^^^^^^^^
```

### a match on an optional that handles only null

```brix crash
(main
  (let value (? I32))
  (print
    (match value
      (case= null
        0
      )
    )
  )
)
```

```output
Exception: This match on I32? handles only null; add a case for a present value, as in "(case (: value I32) …)", or "(default …)" for the rest
<no file:6>
          (case= null
          ^^^^^^^^^^
```

### a match on an enum that leaves out two cases

```brix crash
(main
  (let level (. Level low))
  (print
    (match level
      (case= (. Level low)
        1
      )
    )
  )
)

(type + Level (is U32)
  (variant low (= 0))
  (variant mid (= 1))
  (variant high (= 2))
)
```

```output
Exception: This match on Level does not handle Level.mid and Level.high; add "(case= (. Level mid) …)" and "(case= (. Level high) …)", or "(default …)" for the rest
<no file:6>
          (case= (. Level low)
          ^^^^^^^^^^^^^^^^^^^
```

### an arm that only prints, in a match whose value is kept

```brix crash
(type + Shade (is Union)
  (variant red)
  (variant blue)
)

(main
  (let s Shade .blue)
  (let said (match s (case .red (print "red")) (case .blue "blue")))
  (print said)
)
```

```output
Exception: This arm gives no value, and here a match gives the value of the arm it takes; to run its arms as steps, write the match on a row of its own in a body of steps
<no file:9>
      (let said (match s (case .red (print "red")) (case .blue "blue")))
                         ^^^^^^^^^^^^^^^^^^^^^^^^^
```

### an arm that only calls, in a closure whose body is a value

```brix crash
(fn-do + ping _
  (print "ping")
)

(main
  (let run (=> () (match true (case= true (call ping)) (case= false (call ping)))))
  (call run)
)
```

```output
Exception: This arm gives no value, and this body is a value, so a match gives the value of the arm it takes; to run its arms as steps, write `fn-do`, or `:do` in a closure
<no file:7>
      (let run (=> () (match true (case= true (call ping)) (case= false (call ping)))))
                                  ^^^^^^^^^^^^^^^^^^^^^^^^
```

### in Brix, a match on an enum that leaves out cases names the arms in Brix

```brix crash
(type + Level (is U8)
  (variant low)
  (variant mid)
  (variant high)
)

(main
  (let level Level (. Level low))
  (match level
    (case= .low (print "low"))
  )
)
```

```output
Exception: This match on Level does not handle Level.mid and Level.high; add "(case= (. Level mid) …)" and "(case= (. Level high) …)", or "(default …)" for the rest
<no file:11>
        (case= .low (print "low"))
        ^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### in Brix, a union arm on an enum value names the equality arm in Brix

```brix crash
(type + Kind (is U8)
  (variant a)
  (variant b)
)

(main
  (let kind Kind (. Kind b))
  (match kind
    (case= .a (print "a"))
    (case .b (print "b"))
  )
)
```

```output
Exception: Kind.b is an enum value, not a union case; match it with "(case= .b …)"
<no file:11>
        (case .b (print "b"))
              ^^
```
