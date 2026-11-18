# Variables

Errors about locals name the local, where it was declared, and how to declare or assign it

### a local declared twice in one scope

```yumi crash
. count = 5
. count = 6
```

```output
Exception: "count" is already declared on line 2 in this scope; to change its value, assign it without the ".", as in "count = value", or choose another name
<no file:3>
    . count = 6
      ^^^^^
```

### an assignment to a name nothing declares

```yumi crash
total = 1
```

```output
Exception: Variable "total" is not declared; declare it with ". total = 1"
<no file:2>
    total = 1
    ^^^^^^^^^
```

### an assignment to a misspelled local

```yumi crash
. value = 3
valeu = 4
```

```output
Exception: Variable "valeu" is not declared; did you mean "value"?
<no file:3>
    valeu = 4
    ^^^^^^^^^
```

### a local called as a function

```yumi crash
. size = 3
-- size(2)
```

```output
Exception: "size" is a local I64, not a function, so it cannot be called
<no file:3>
    -- size(2)
       ^^^^^^^
```

### a value returned from the top level of a module

```yumi crash
= 1
```

```output
Exception: The top level of a module cannot return a value with "="; remove the "=", or print the value with "--"
<no file:2>
    = 1
    ^^^
```
