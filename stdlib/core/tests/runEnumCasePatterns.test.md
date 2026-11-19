# Enum case patterns

### Enum members match as values with |=.

```brix
(main
  (call describe (. Kind b))
)

(type + Kind (is U8)
  (variant a)
  (variant b)
)

(fn-do + describe _
  (param kind Kind)
  (match kind
    (case= .a
      (print "a")
    )
    (case= .b
      (print "b")
    )
  )
)
```

```output
b
```

### A union case arm on an enum names the enum value syntax.

```brix crash
(main
  (call describe (. Kind b))
)

(type + Kind (is U8)
  (variant a)
  (variant b)
)

(fn-do + describe _
  (param kind Kind)
  (match kind
    (case .a
      (print "a")
    )
    (case .b
      (print "b")
    )
  )
)
```

```output
Exception: Kind.a is an enum value, not a union case; match it with "(case= .a …)"
<no file:14>
        (case .a
              ^^
```

### A qualified union case arm on an enum names the qualified value syntax.

```brix crash
(main
  (call describe (. Kind b))
)

(type + Kind (is U8)
  (variant a)
  (variant b)
)

(fn-do + describe _
  (param kind Kind)
  (match kind
    (case= (. Kind a)
      (print "a")
    )
    (case (. Kind b)
      (print "b")
    )
  )
)
```

```output
Exception: Kind.b is an enum value, not a union case; match it with "(case= (. Kind b) …)"
<no file:17>
        (case (. Kind b)
              ^^^^^^^^^^
```

### Expression matches report the enum value syntax.

```brix crash
(main
  (print (call describe (. Kind b)))
)

(type + Kind (is U8)
  (variant a)
  (variant b)
)

(fn-expr + describe _
  (param kind Kind)
  (match kind
    (case .a
      "a"
    )
    (case .b
      "b"
    )
  )
)
```

```output
Exception: Kind.a is an enum value, not a union case; match it with "(case= .a …)"
<no file:14>
        (case .a
              ^^
```

### Optional matches report the enum value syntax instead of skipping the arm.

```brix crash
(main
  (let kind (. Kind b))
  (match? kind
    (case (. Kind a)
      (print "a")
    )
    (case (. Kind b)
      (print "b")
    )
  )
  (print "done")
)

(type + Kind (is U32)
  (variant a (= 0))
  (variant b (= 1))
)
```

```output
Exception: Kind.a is an enum value, not a union case; match it with "(case= (. Kind a) …)"
<no file:5>
        (case (. Kind a)
              ^^^^^^^^^^
```

### Matches with a default arm report the enum value syntax.

```brix crash
(main
  (let kind (. Kind b))
  (match kind
    (case .b
      (print "b")
    )
    (case _
      (print "other")
    )
  )
)

(type + Kind (is U32)
  (variant a (= 0))
  (variant b (= 1))
)
```

```output
Exception: Kind.b is an enum value, not a union case; match it with "(case= .b …)"
<no file:5>
        (case .b
              ^^
```

### An is test that names an enum value as a case reports the enum value syntax.

```brix crash
(main
  (let kind (. Kind b))
  (print (is kind .a))
  (print "done")
)

(type + Kind (is U32)
  (variant a (= 0))
  (variant b (= 1))
)
```

```output
Exception: Kind.a is an enum value, not a union case; match it with "(case= .a …)"
<no file:4>
      (print (is kind .a))
                      ^^
```

### An arm that is a name alone binds the name even where the enum has a value of that name.

```brix compile-error
(main
  (call describe (. Kind b))
)

(type + Kind (is U8)
  (variant a)
  (variant b)
)

(fn-do + describe _
  (param kind Kind)
  (match kind
    (case= (. Kind a)
      (print "a")
    )
    (case b
      (print "b")
    )
  )
)
```

```output
Exception: Arm "(case b …)" binds a new name, not the value Kind.b; match the value with "(case= .b …)"
<no file:17>
        (case b
              ^
```

### An arm written `|=` compares the subject with one value, so values listed there for an enum are an error that names a condition, which takes any of several.

```brix compile-error
(main
  (print (call describe (. Kind b)))
)

(type + Kind (is U8)
  (variant a)
  (variant b)
  (variant c)
)

(fn-expr + describe String
  (param kind Kind)
  (match kind
    (case= (tuple .a .b)
      "a or b"
    )
    (default
      "c"
    )
  )
)
```

```output
Exception: An arm written "(case= …)" compares the subject with one value, and this lists several; to take any of several, write a condition, as in "(case? (or (== it .a) (== it .b)) …)"
<no file:15>
        (case= (tuple .a .b)
               ^^^^^^^^^^^^^
```

### A condition arm takes any of several enum values.

```brix
(main
  (print (call describe (. Kind b)))
  (print (call describe (. Kind c)))
)

(type + Kind (is U8)
  (variant a)
  (variant b)
  (variant c)
)

(fn-expr + describe String
  (param kind Kind)
  (match kind
    (case? (or (== it .a) (== it .b))
      "a or b"
    )
    (default
      "c"
    )
  )
)
```

```output
a or b
c
```
