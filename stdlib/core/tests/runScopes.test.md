# Scopes

```yumi crash
. a I32 = 1
. a I32 = 2
```

```output
Exception: "a" is already declared on line 2 in this scope; to change its value, assign it without the ".", as in "a = value", or choose another name
<no file:3>
    . a I32 = 2
      ^^^^^
```

```yumi crash
a() = 1
```

```output
Exception: Illegal assignment
<no file:2>
    a() = 1
    ^^^^^^^
```

```yumi
. a I32 = 1
do
    . a Float = 2.0
    -- a
-- a
```

```output
2
1
```

```yumi
. a I32 = 1
do
    -- a
    . a Float = 2.0
```

```output
1
```

```yumi
. a I32 = 1
do
    . a Float = a
    -- a
```

```output
1
```

```yumi crash
do
    . a Float = 2.0
-- a
```

```output
Exception: Name "a" not found
<no file:4>
    -- a
       ^
```

```yumi
. a = 2
do
    a = 4
-- a
```

```output
4
```

```yumi
. thing = Thing()

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ a = 10

────────────────────
+ This

do
    . a = 1
    -- a
-- a
```

```output
1
10
```

```yumi
. foo Int, bar String, baz Float
-- foo
-- bar
-- baz
```

```output
0

0
```

```yumi
.. foo Int, bar String, baz Float = (1, "hi", 2)
-- foo
-- bar
-- baz
```

```output
1
hi
2
```

```yumi crash
. foo Int, bar String, baz Float = (1, "hi")
```

```output
Exception: Pattern has no possible matches
<no file:2>
    . foo Int, bar String, baz Float = (1, "hi")
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```yumi
.. foo Int = 1
-- foo
```

```output
1
```

```yumi
.. foo = bar
    .. bar = 1
-- foo
```

```output
1
```

```yumi
--
    |? false
        a
    ?? a

.. a = noisy(10)

━━━━━━━━━━━━━━━━━━━━
+ do noisy Int
    - n Int

-- n
= n
```

```output
10
10
```

### A name nothing declares cannot be assigned.

```yumi crash
a = 1
```

```output
Exception: Variable "a" is not declared; declare it with ". a = 1"
<no file:2>
    a = 1
    ^^^^^
```

### A name a function declares cannot be assigned.

```yumi crash
helper = 5

━━━━━━━━━━━━━━━━━━━━
+ helper U32

3
```

```output
Exception: Function "helper" cannot be assigned
<no file:2>
    helper = 5
    ^^^^^^^^^^
```

### A local takes its type from its value, or the type written for it, which the value converts to.

```yumi
. a = 5
. b I64 = 5
. c Float = 1
. d F32 = 1
. e I64 = 5u8
. f U8 = 7
. g = "text"
. h = true
-- a
-- b
-- c
-- d
-- e
-- f
-- g
-- h
```

```output
5
5
1
1
5
7
text
true
```

### A local declared with no value has the zero of its type, and one statement declares several.

```yumi
. x I32
. f Float
. b Bool
. s String
. m I32, n Float
-- x
-- f
-- b
-- s ++ "!"
m = 2
n = m
-- m
-- n
```

```output
0
0
false
!
2
2
```

### A local declared in a loop is declared anew on each pass.

```yumi
for i in 0 .. < 3
    . z Int
    . s String
    z += i
    s ++= "k"
    -- z
    -- s
```

```output
0
k
1
k
2
k
```

### A local hides another of its name in a scope within, and the outer one is read until it is declared.

```yumi
. a I32 = 1
do
    . a Float = a
    a += 0.5
    -- a
-- a
do
    a = 2
    do
        . a = 3
        -- a
    -- a
-- a
```

```output
1.5
1
3
2
2
```

### A local declared in a branch is not the one declared after it.

```yumi
. x = 1
if x
    . y = "in"
    -- y
. y = 2.5
-- y
```

```output
in
2.5
```

### Assignment converts the value to the local's type.

```yumi
. x I64 = 5
x = 6i32
-- x
. f = 1.5
f = 2
-- f
. n = 3
n = 2i64
-- n
. u U8 = 5
u = 200
-- u
. b = true
b = 1 > 2
-- b
```

```output
6
2
2
200
false
```

### A value cannot stand alone as a step.

```yumi crash
. x = 1
x + 1
-- x
```

```output
Exception: A value cannot stand alone as a step; use it, or remove it
<no file:3>
    x + 1
    ^^^^^
```

### A local may have a name that JavaScript objects inherit.

```yumi
. total = 0
for index in 0 .. < 3
    . constructor = index * 2
    total += constructor
-- total
```

```output
6
```

### Past a test that finds an optional local holds a value, a local may be declared again.

```yumi
run(3)
run(0)

━━━━━━━━━━━━━━━━━━━━
+ do run
    - limit U64

. kept = limit + 1
-- "early $kept"
. path = limit if limit > 1
=? if !path
. kept = "late ${path}"
-- kept
```

```output
early 4
late 3
early 1
```

### Inside the body of a test that finds an optional local holds a value, a local is declared once.

```yumi crash
. path U64? = 3
if path
    . kept = path! + 1
    -- kept
    . kept = path! + 2
    -- kept
```

```output
Exception: "kept" is already declared on line 4 in this scope; to change its value, assign it without the ".", as in "kept = value", or choose another name
<no file:6>
        . kept = path! + 2
          ^^^^
```
