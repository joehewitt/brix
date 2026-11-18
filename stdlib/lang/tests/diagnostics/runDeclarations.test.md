# Declarations

Declaration errors say which rule a function or parameter breaks and how to fix it

### immediate declaration needs do

```yumi crash
-- total()

━━━━━━━━━━━━━━━━━━━━
+ total Int

. x = 1
x
```

```output
Exception: Function "total" is not declared with do, so its body is one expression and cannot hold ". name = value" declarations; write "+ do total Int" and return its value with "= value"
<no file:7>
    . x = 1
    ^^^^^^^
```

### loop needs do

```yumi crash
-- total()

━━━━━━━━━━━━━━━━━━━━
+ total I32

for i in 0 .. < 3
    i
3
```

```output
Exception: Function "total" is not declared with do, so its body is one expression and cannot hold loops; write "+ do total I32" and return its value with "= value"
<no file:7>
    for i in 0 .. < 3
       ^^^^^^^^^^^^^
```

### explicit return needs do

```yumi crash
-- total()

━━━━━━━━━━━━━━━━━━━━
+ total I32

= 3
```

```output
Exception: Function "total" is not declared with do, so its body is one expression and cannot return with "="; write "+ do total I32"
<no file:7>
    = 3
    ^^^
```

### value not returned

```yumi crash
-- total()

━━━━━━━━━━━━━━━━━━━━
+ do total I32

. x = 1
x
```

```output
Exception: Function total ends with a value it does not return; return it with "= value", or remove it
<no file:8>
    x
    ^
```

### required after a default

```yumi crash
-- f(1, 2)

━━━━━━━━━━━━━━━━━━━━
+ f I32
    - a I32 = 1
    - b I32

a + b
```

```output
Exception: Parameter "b" has no default but comes after "a", which has a default; declare required parameters first, or give "b" a default
<no file:7>
        - b I32
          ^^^^^
```

### a call that leaves out a parameter that may be null

```yumi crash
-- f(1)

━━━━━━━━━━━━━━━━━━━━
+ f I32
    - a I32
    - b I32?

a + (b ?? 0)
```

```output
Exception: No function "f" accepts (I64); it is declared as "f(a I32, b I32?)"; "b" may be null but has no default, so a call must give it: pass null, or declare "b I32? = null"
<no file:2>
    -- f(1)
       ^^^^
```

### typed declaration that cannot match

```yumi crash
. n I32 = Pair(1, 2)

━━━━━━━━━━━━━━━━━━━━
+ Pair is Tuple

+ x I32
+ y I32
```

```output
Exception: "n" is declared I32, but its value is Pair, which can never be I32; convert the value, or declare "n" without a type
<no file:2>
    . n I32 = Pair(1, 2)
      ^^^^^
```

### a function written after another function body with no divider

```yumi crash
-- first(1)
━━━━━━━━━━━━━━━━━━━━
+ first I32
    - n I32

n + 1

+ second I32

2
```

```output
Exception: "+ second I32" is read as part of the body of "first", because no divider separates them; put a thick divider (━) on the line above it
<no file:9>
    + second I32
    ^^^^^^^^^^^^
```

### a value written after a function body with no divider

```yumi crash
-- first(1)
━━━━━━━━━━━━━━━━━━━━
+ first I32
    - n I32

n + 1

+ limit = 5
```

```output
Exception: "+ limit = 5" is read as part of the body of "first", because no divider separates them; declare module-level values above the first divider
<no file:9>
    + limit = 5
    ^^^^^^^^^^^
```

### a method written after another method body with no divider

```yumi crash
-- Box().size()
━━━━━━━━━━━━━━━━━━━━
+ Box

────────────────────
+ size I32

2

+ more I32

3
```

```output
Exception: "+ more I32" is read as part of the body of "size", because no divider separates them; put a thin divider (─) on the line above it
<no file:11>
    + more I32
    ^^^^^^^^^^
```

### a declaration with no type written after a function body with no divider

```yumi crash
-- first(1)
━━━━━━━━━━━━━━━━━━━━
+ first I32
    - n I32

n + 1

+ second

-- "second"
```

```output
Exception: "+ second" is read as part of the body of "first", because no divider separates them; put a thick divider (━) on the line above it
<no file:9>
    + second
    ^^^^^^^^
```
