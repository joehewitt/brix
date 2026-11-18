# Returns

Errors about returns and the errors a function raises name the function and the declaration to write

### a recursive function with no declared return type

```yumi crash
-- count-down(3)
━━━━━━━━━━━━━━━━━━━━
+ do count-down
    - n I32

if n == 0
    = 0
= count-down(n - 1)
```

```output
Exception: Function "count-down" calls itself before its return type is known, so the type cannot be inferred; declare it, as in "+ do count-down I64"
<no file:4>
    + do count-down
    ^^^^^^^^^^^^^^
```

### returns that have no common type

```yumi crash
-- pick(2)
━━━━━━━━━━━━━━━━━━━━
+ Box
━━━━━━━━━━━━━━━━━━━━
+ do pick
    - x I32

if x > 1
    = Box()
= 2.5
```

```output
Exception: Function "pick" returns Box here and F64 elsewhere, which have no common type; return one type, or declare the return type
<no file:10>
        = Box()
        ^^^^^^^
```

### an error a call raises that the calling function does not declare

```yumi crash
-- outer()
━━━━━━━━━━━━━━━━━━━━
+ do outer I32 ?! TypeError

= load()
━━━━━━━━━━━━━━━━━━━━
+ do load I32 ?! IOError

?! IOError("disk")
```

```output
Exception: IOError can leave "outer", which does not declare it; declare it, as in "+ do outer I32 ?! TypeError ?! IOError", or catch it here
<no file:6>
    = load()
      ^^^^^^
```

### an error a function raises but does not declare

```yumi crash
-- load()
━━━━━━━━━━━━━━━━━━━━
+ do load I32 ?! TypeError

?! IOError("disk")
```

```output
Exception: IOError can leave "load", which does not declare it; declare it, as in "+ do load I32 ?! TypeError ?! IOError"
<no file:6>
    ?! IOError("disk")
    ^^^^^^^^^^^^^^^^^^
```

### an error a recursive function raises before its errors are inferred

```yumi crash
-- walk(3)
━━━━━━━━━━━━━━━━━━━━
+ do walk Bool
    - n I32

if n == 0
    ?! IOError("disk")
= walk(n - 1)
```

```output
Exception: IOError can leave "walk", which calls itself, so its errors cannot be inferred; declare them on it, as in "+ do walk Bool ?! IOError", or catch them here
<no file:8>
        ?! IOError("disk")
        ^^^^^^^^^^^^^^^^^^
```

### a method that can fail implementing an interface method that cannot

```yumi crash
. p = Thing() as Proto
-- p.foo()
━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ foo I32
━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Proto

────────────────────
+ foo

?! Error("fooey")
```

```output
Exception: "foo" can fail with Error, but the interface method it implements cannot; add "?! Error" to that method, or catch the error inside "foo"
<no file:14>
    + foo
    ^^^^
Interface:
<no file:8>
    + foo I32
```

### a catch attached to an expression that cannot fail

```yumi crash
. value = 42
    catch Error
        0
-- value
```

```output
Exception: "42" cannot fail, so the catch attached to it has nothing to handle; remove the catch, or attach it to a call that can fail
<no file:2>
    . value = 42
              ^
```

### a value returned from a class's event handler

```yumi crash
. spot = Spot()
spot.pings <- true
━━━━━━━━━━━━━━━━━━━━
+ Spot

+ taps Size = 0
+ event pings Bool

────────────────────
for event on pings

=? if !event
taps += 1
```

```output
Exception: The event handler for "pings" cannot return with "="; move its body into a method, as in "_ do handle-pings", and call that, or end the handler early with "continue"
<no file:13>
    =? if !event
    ^^^^^^^^^^^^
```
