# If

```yumi
-- if true then 1 else 42
```

```output
1
```

```yumi
. x = 2
if x > 1
    -- "yes"
else
    -- "no"
-- "done"
```

```output
yes
done
```

```yumi
check(1)
check(11)
check(3)

━━━━━━━━━━━━━━━━━━━━
+ do check
    - x I32

if x > 10
    -- "a"
else if x >= 2
    -- "b"
else
    -- "c"
-- "done"
```

```output
c
done
a
done
b
done
```

```yumi
. x = 6
-- if x > 10 then 3 else 2 if x > 5
```

```output
2
```

```yumi
. x = 1
-- 1 if x > 10

x = 11
-- 1 if x > 10

x = 1
-- if x > 10 then 1 else 3

x = 11
-- if x > 10 then 1 else 3

x = 1
-- if x > 10 then 1 else 2 if x > 5

x = 6
-- if x > 10 then 1 else 2 if x > 5

x = 11
-- if x > 10 then 1 else if x > 5 then 2 else 3

x = 6
-- if x > 10 then 1 else if x > 5 then 2 else 3

x = 1
-- if x > 10 then 1 else if x > 5 then 2 else 3
```

```output
unknown
1
3
1
unknown
2
1
2
3
```

```yumi
. x Int?
-- x ?? 2
x = 1
-- x ?? 2
```

```output
2
1
```

```yumi
. x = 0
. y = 1
-- x && x
-- x && y
-- y && y
-- y && x
```

```output
false
false
true
false
```

```yumi
. x = 0
. y = 1
-- x || x
-- x || y
-- y || y
-- y || x
```

```output
false
true
true
true
```

### If blocks have their own scope

```yumi
. a = 1
if true
    . a = 2
    -- a
if false
    -- "never"
else
    . a = 2
    -- a
-- a
```

```output
2
2
1
```

```yumi crash
. x =
    if 1 > 2
        1
    else
        2
-- x
```

```output
Exception: Illegal statement
<no file:2>
    . x =
```

```yumi
-- 1 if true
```

```output
1
```

```yumi
-- 1 if false
```

```output
unknown
```

```yumi
--
    |? true
        1
    ??
        2
```

```output
1
```

```yumi
-- test()
-- test(2)

━━━━━━━━━━━━━━━━━━━━
+ test
    - t Int? = null

|? 0 > 1
    1
|? t
?? 3
```

```output
3
2
```

```yumi
--
    |? bar
        foo
        .. foo = 2
    .. bar = 1
```

```output
2
```

```yumi
. a Int? = 42
if a
    -- a
    -- type-of a
-- type-of a
```

```output
42
[TypeInfo I64]
[TypeInfo Optional'I64]
```

### `else if` is tried where the branches before it are not taken, and `else` where none is.

```yumi
. x = 5
if x == 1
    -- 1
else if x == 2
    -- 2
else if x == 5
    -- 5
else
    -- 0
if x > 9
    -- "big"
else
    -- "small"
-- "end"
```

```output
5
small
end
```

### A condition is a Bool, a number that is not zero, or a text that is not empty.

```yumi
. n = 3
. z = 0.0
. s = "a"
. e = ""
if n
    -- "number"
if z
    -- "zero"
else
    -- "no zero"
if 5u8
    -- "byte"
if s
    -- "text"
if e
    -- "empty"
else if !e
    -- "no text"
if s && !e
    -- "both"
if e || z
    -- "either"
else
    -- "neither"
```

```output
number
no zero
byte
text
no text
both
neither
```

### A local assigned in a branch keeps what the branch gave it.

```yumi
. x = 3
if x > 1
    x = 10
    if x > 5
        x += 1
else
    x = 20
-- x
```

```output
11
```

### A text tested as the condition of a value given only where it holds is asked whether it is empty

```yumi
-- show("a")
-- show("")

━━━━━━━━━━━━━━━━━━━━
+ do show
    - event String

. x = "event" if event
= x ?? "none"
```

```output
event
none
```

### A choice between a text and a truth value has no type in common.

```yumi crash
. flag = true
. chosen = if flag then "picked" else false
-- chosen
```

```output
Exception: The two sides of this "if" are String and Bool, which have no common type; convert them to one type
<no file:3>
    . chosen = if flag then "picked" else false
               ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A choice between two numbers neither of which holds every value of the other has no type in common, though either converts to the other with `as`.

```yumi crash
. flag = true
. small I32 = 3
. ratio F32 = 1.5
-- if flag then small else ratio
```

```output
Exception: The two sides of this "if" are I32 and F32, which have no common type; convert them to one type
<no file:5>
    -- if flag then small else ratio
       ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A choice whose declared type is a union holds each side as its own case instead of joining them as text.

```yumi
. flag = true
-- describe(chosen(flag))
-- describe(chosen(!flag))

━━━━━━━━━━━━━━━━━━━━
_ Shape is Union

| String
| [String]

━━━━━━━━━━━━━━━━━━━━
_ chosen Shape
    - flag Bool

if flag then ["a", "b"] else "c"

━━━━━━━━━━━━━━━━━━━━
_ describe String
    - shape Shape

match shape
    | text String
        "text $text"
    | items [String]
        "items ${items.length}"
```

```output
items 2
text c
```
