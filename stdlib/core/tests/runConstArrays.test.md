# Const arrays

### ConstArray values can be declared as global constants and indexed by position.

```brix
(prop + values _
  (=
    (call (of ConstArray U16)
      (block
        (arg 0)
        (arg 8)
        (arg 16)
      )
    )
  )
)
(blank)
(prop + flags _
  (=
    (call (of ConstArray U8)
      (block
        (arg 3)
        (arg 5)
      )
    )
  )
)
(blank)

(main
  (print ([] values 0))
  (print ([] values 1))
  (print ([] values 2))
  (print ([] flags 0))
  (print ([] flags 1))
)
```

```output
0
8
16
3
5
```

### ConstArray values keep their own type identity and are read-only.

```brix
(prop + values _
  (=
    (call (of ConstArray U8)
      (block
        (arg 65)
        (arg 66)
      )
    )
  )
)
(blank)

(main
  (print (type-of values))
  (print (. (type-of values) read-only))
  (print ([] values 0))
  (print ([] values 1))
)
```

```output
[TypeInfo ConstArray'U8]
true
65
66
```

### Inline constructor arguments produce a ConstArray too.

```brix
(main
  (let values (call (of ConstArray U16) 3 5 8))
  (blank)
  (print (type-of values))
  (print ([] values 0))
  (print ([] values 1))
  (print ([] values 2))
)
```

```output
[TypeInfo ConstArray'U16]
3
5
8
```

### Numeric items are cast to the declared item type before emission.

```brix
(prop + bytes _
  (=
    (call (of ConstArray U8)
      (block
        (arg (as 258 U8))
        (arg (as -1 U8))
      )
    )
  )
)
(blank)
(prop + shorts _ (= (call (of ConstArray U16) (as 65537 U16) (as -1 U16))))
(blank)

(main
  (print ([] bytes 0))
  (print ([] bytes 1))
  (print ([] shorts 0))
  (print ([] shorts 1))
)
```

```output
2
255
1
65535
```

### ConstArray values support pointer-sized items such as strings.

```brix
(prop + words _
  (=
    (call (of ConstArray String)
      (block
        (arg "alpha")
        (arg "beta")
      )
    )
  )
)
(blank)

(main
  (print ([] words 0))
  (print ([] words 1))
)
```

```output
alpha
beta
```

### Multiple ConstArrays in one program get independent storage.

```brix
(prop + left _ (= (call (of ConstArray U8) 1 2)))
(prop + right _ (= (call (of ConstArray U8) 10 20)))
(blank)

(main
  (print ([] left 0))
  (print ([] left 1))
  (print ([] right 0))
  (print ([] right 1))
)
```

```output
1
2
10
20
```

### Functions can accept ConstArrays without converting them to mutable pointers.

```brix
(prop + values _ (= (call (of ConstArray U8) 4 5)))
(blank)

(main
  (print (call first values))
)

(fn-expr + first _
  (param data (of ConstArray U8))
  ([] data 0)
)
```

```output
4
```

### ConstArray values are read-only through a global binding.

```brix compile-error
(prop + values _ (= (call (of ConstArray U8) 1 2)))
(blank)

(main
  (= ([] values 0) 9)
)
```

```output
Exception: ConstArray values are read-only
<no file:6>
      (= ([] values 0) 9)
         ^^^^^^^^^^^^^
```

### ConstArray values are read-only through a local binding.

```brix compile-error
(main
  (let values (call (of ConstArray U8) 1 2))
  (blank)
  (= ([] values 0) 9)
)
```

```output
Exception: ConstArray values are read-only
<no file:5>
      (= ([] values 0) 9)
         ^^^^^^^^^^^^^
```

### Incremental assignment is also rejected.

```brix compile-error
(main
  (let values (call (of ConstArray U8) 1 2))
  (blank)
  (+= ([] values 0) 1)
)
```

```output
Exception: ConstArray values are read-only
<no file:5>
      (+= ([] values 0) 1)
          ^^^^^^^^^^^^^
```

### ConstArray values cannot be cast to mutable C pointers.

```brix compile-error
(prop + values _ (= (call (of ConstArray U8) 1 2)))
(blank)

(main
  (let pointer (as values (of CPointer U8)))
)
```

```output
Exception: ConstArray'U8 cannot be cast to CPointer'U8; there is no conversion between them
<no file:6>
      (let pointer (as values (of CPointer U8)))
                   ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### ConstArray literals require exactly one item type.

```brix compile-error
(main
  (print (call ConstArray))
)
```

```output
Exception: ConstArray requires one item type
<no file:3>
      (print (call ConstArray))
             ^^^^^^^^^^^^^^^^^
```

### ConstArray items must be positional values.

```brix compile-error
(main
  (print
    (call (of ConstArray U8)
      (block
        (:first 1)
      )
    )
  )
)
```

```output
Exception: Illegal ConstArray item
<no file:6>
            (:first 1)
            ^^^^^^^^^^
```

### ConstArray items must have a constant representation.

```brix compile-error
(prop + values _ (= (call (of ConstArray U8) (call value))))
(blank)

(main
  (print ([] values 0))
)

(fn-expr + value U8
  42
)
```

```output
Exception: ConstArray item must be constant
<no file:2>
    (prop + values _ (= (call (of ConstArray U8) (call value))))
                                                 ^^^^^^^^^^^^
```
