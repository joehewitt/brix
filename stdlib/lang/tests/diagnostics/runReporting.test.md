# Reporting

A program's errors are all reported, one per line and at most ten, each once

### errors in several functions, in line order

```yumi crash
-- f(1)
-- gone-left + gone-right
-- gone-below
━━━━━━━━━━━━━━━━━━━━
+ f I32
    - a I32
a.zz
```

```output
Exception: Name "gone-left" not found
<no file:3>
    -- gone-left + gone-right
       ^^^^^^^^^

Exception: Name "gone-below" not found
<no file:4>
    -- gone-below
       ^^^^^^^^^^

Exception: I32 has no property "zz"
<no file:8>
    a.zz
    ^^^^
```

### a name missing twice in one function

```yumi crash
-- f(1)
━━━━━━━━━━━━━━━━━━━━
+ do f I32
    - a I32
-- gone-name
-- gone-name
= a
```

```output
Exception: Name "gone-name" not found
<no file:6>
    -- gone-name
       ^^^^^^^^^
```

### one mistake that two passes report

```yumi crash
-- test()
━━━━━━━━━━━━━━━━━━━━
+ test I32
= 42
```

```output
Exception: Function "test" is not declared with do, so its body is one expression and cannot return with "="; write "+ do test I32"
<no file:5>
    = 42
    ^^^^
```

### more than ten errors

```yumi crash
-- gone-one
-- gone-two
-- gone-three
-- gone-four
-- gone-five
-- gone-six
-- gone-seven
-- gone-eight
-- gone-nine
-- gone-ten
-- gone-eleven
```

```output
Exception: Name "gone-one" not found
<no file:2>
    -- gone-one
       ^^^^^^^^

Exception: Name "gone-two" not found
<no file:3>
    -- gone-two
       ^^^^^^^^

Exception: Name "gone-three" not found
<no file:4>
    -- gone-three
       ^^^^^^^^^^

Exception: Name "gone-four" not found
<no file:5>
    -- gone-four
       ^^^^^^^^^

Exception: Name "gone-five" not found
<no file:6>
    -- gone-five
       ^^^^^^^^^

Exception: Name "gone-six" not found
<no file:7>
    -- gone-six
       ^^^^^^^^

Exception: Name "gone-seven" not found
<no file:8>
    -- gone-seven
       ^^^^^^^^^^

Exception: Name "gone-eight" not found
<no file:9>
    -- gone-eight
       ^^^^^^^^^^

Exception: Name "gone-nine" not found
<no file:10>
    -- gone-nine
       ^^^^^^^^^

Exception: Name "gone-ten" not found
<no file:11>
    -- gone-ten
       ^^^^^^^^

1 more error is not shown
```
