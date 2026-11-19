# Variables

Errors about locals name the local, where it was declared, and how to declare or assign it

### a local declared twice in one scope

```brix crash
(main
  (let count 5)
  (let count 6)
)
```

```output
Exception: "count" is already declared on line 3 in this scope; to change its value, assign it without the ".", as in "count = value", or choose another name
<no file:4>
      (let count 6)
           ^^^^^
```

### an assignment to a name nothing declares

```brix crash
(main
  (= total 1)
)
```

```output
Exception: Variable "total" is not declared; declare it with "(let total 1)"
<no file:3>
      (= total 1)
      ^^^^^^^^^^^
```

### an assignment to a misspelled local

```brix crash
(main
  (let value 3)
  (= valeu 4)
)
```

```output
Exception: Variable "valeu" is not declared; did you mean "value"?
<no file:4>
      (= valeu 4)
      ^^^^^^^^^^^
```

### a local called as a function

```brix crash
(main
  (let size 3)
  (print (call size 2))
)
```

```output
Exception: "size" is a local I64, not a function, so it cannot be called
<no file:4>
      (print (call size 2))
             ^^^^^^^^^^^^^
```

### a value returned from the top level of a module

```brix crash
(main
  (return 1)
)
```

```output
Exception: The top level of a module cannot return a value with "return"; remove the "return", or print the value with "print"
<no file:3>
      (return 1)
      ^^^^^^^^^^
```
