# Values

Errors about lists, loops and declared values name the types involved and what to write instead

### a list whose items share no type

```yumi crash
. list = [Thing(), Other()]
-- list.size
━━━━━━━━━━━━━━━━━━━━
+ Thing
━━━━━━━━━━━━━━━━━━━━
+ Other
```

```output
Exception: This list mixes Thing and Other, which have no common type; convert the items to one type
<no file:2>
    . list = [Thing(), Other()]
             ^^^^^^^^^^^^^^^^^^
```

### a loop over a number

```yumi crash
for item in 42
    -- item
```

```output
Exception: A for loop cannot iterate over I64; to count, loop over a range, as in "0 .. < 42"
<no file:2>
    for item in 42
                ^^
```

### a loop over a value that cannot be iterated

```yumi crash
. ratio = 0.5
for item in ratio
    -- item
```

```output
Exception: A for loop cannot iterate over F64; loop over a list, a range or a stream
<no file:3>
    for item in ratio
                ^^^^^
```

### an interface local declared with no value

```yumi crash
. p Proto
━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface
```

```output
Exception: "p" is declared Proto with no value, but Proto is an interface, so there is nothing to construct; give it a value, as in ". p Proto = value", or declare it "Proto?" to start as null
<no file:2>
    . p Proto
        ^^^^^
```

### a lazy local defined in terms of itself

```yumi crash
-- total()
━━━━━━━━━━━━━━━━━━━━
+ total

a
    .. a = b
    .. b = a
```

```output
Exception: "a" depends on its own value, directly or through the values it reads; break the cycle
<no file:7>
        .. a = b
           ^
```

### a labeled argument a function value does not take

```yumi crash
-- apply(combine)
━━━━━━━━━━━━━━━━━━━━
+ Op is () I32
    - a I32
    - b I32
━━━━━━━━━━━━━━━━━━━━
+ combine I32
    - a I32
    - b I32

a - b
━━━━━━━━━━━━━━━━━━━━
+ apply I32
    - f Op

f(10) • by 3
```

```output
Exception: "f" has no "by" argument
<no file:17>
    f(10) • by 3
         ^^^^^^^
```
