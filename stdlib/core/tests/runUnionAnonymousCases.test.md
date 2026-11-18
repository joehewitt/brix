# Union anonymous cases

```yumi
. result = match true
    |? true
        ..ok
    ??
        ..failed

-- result is .ok
-- result.name
```

```output
true
ok
```

```yumi
. result = match true
    |? true
        ..valid("email", "password")
    ??
        ..invalid-email

-- match result
    | .valid(email, password)
        "$email:$password"
    | .invalid-email
        "email"
```

```output
email:password
```

```yumi
. use-same = false
. result = match use-same
    |? use-same
        ..same
    ??
        ..same

-- result.name
-- result.case
```

```output
same
0
```

```yumi
. use-int = false
. result = match use-int
    |? use-int
        ..value(1)
    ??
        ..value(1.5)

-- match result
    | .value(x)
        x
```

```output
1.5
```

```yumi
. input = "no"
. event = match input
    ~ "ok"
        ..ok
    ??
        ..failed(404)

-- match event
    | .ok
        "ok"
    | .failed(code)
        "failed:$code"
```

```output
failed:404
```

```yumi
. result = match true
    |? true
        ..ok
    ??
        ..failed(1)

. other = .failed(2)
-- other.name
-- match other
    | .failed(code)
        code
    | .ok
        0
```

```output
failed
2
```

```yumi
. result = match true
    |? true
        ..ok
    ??
        ..failed

result = match false
    |? false
        ..failed
    ??
        ..ok

-- result.name
```

```output
ok
```

```yumi
+ submitted AsyncChannel'Bool

.. login-submission = on submitted
    |? true
        ..valid("email", "password")
    ??
        ..invalid-password

-- login-submission is .valid
```

```output
false
```

```yumi
. result = authenticate("email", "secret")
-- result.name
-- match result
    | .authentic(email, token)
        "$email:$token"
    | .incorrect-password
        "bad"

━━━━━━━━━━━━━━━━━━━━
+ authenticate
    - email String
    - password String

|? password == "secret"
    ..authentic(email, "token")
??
    ..incorrect-password
```

```output
authentic
email:token
```

```yumi
for result in authenticate("email", "secret")
    -- result.name
    -- match result
        | .authentic(email, token)
            "$email:$token"
        | .incorrect-password
            "bad"

━━━━━━━━━━━━━━━━━━━━
+ do authenticate
    - email String
    - password String

<=
    |? password == "secret"
        ..authentic(email, "token")
    ??
        ..incorrect-password
```

```output
authentic
email:token
```

```yumi crash
-- match true
    |? true
        ..ok
    ??
        ..failed
```

```output
Exception: Anonymous match case '..ok' needs a stable result owner
<no file:4>
            ..ok
            ^^^^
```

```yumi crash
. result = match true
    |? true
        ..value(1)
    ??
        ..value(1, 2)
```

```output
Exception: Anonymous case '..value' has conflicting arity
<no file:6>
            ..value(1, 2)
            ^^^^^^^^^^^^^
```

### Anonymous case named again by its symbol, bare and called

```yumi
. result = match true
    |? true
        ..ok
    ??
        ..failed(1)

. other = .ok
-- other.name
-- other.case
-- other is .ok
-- result is .ok
. again = .failed(7)
-- again.name
-- again.case
-- match again
    | .failed(code)
        code + 1
    | .ok
        0
-- other == result
-- again == result
-- again != result
```

```output
ok
0
true
true
failed
1
8
true
false
true
```

### Anonymous cases of a function with several values and an object

```yumi
. box = Box(4)
-- describe(pick(0, box))
-- describe(pick(1, box))
-- describe(pick(2, box))
-- describe(pick(3, box))
-- pick(1, box).name
-- pick(3, box).case

━━━━━━━━━━━━━━━━━━━━
+ Box

+ x Int

────────────────────
+ This
    - x

━━━━━━━━━━━━━━━━━━━━
+ pick
    - which Int
    - box Box

|? which == 0
    ..empty
|? which == 1
    ..triple(1, 2.5, "three")
|? which == 2
    ..triple(4, 5, "six")
??
    ..boxed(box)

━━━━━━━━━━━━━━━━━━━━
+ describe String
    - value

match value
    | .empty
        "empty"
    | .triple(a, b, c)
        "triple $a $b $c"
    | .boxed(held)
        "boxed ${held.x}"
```

```output
empty
triple 1 2.5 three
triple 4 5 six
boxed 4
triple
2
```

### Anonymous cases assigned again and by symbol

```yumi
. count = 0
. result = match count
    |? count > 0
        ..many(count)
    ??
        ..none
-- result.name
count = 3
result = match count
    |? count > 0
        ..many(count)
    ??
        ..none
-- result.name
-- match result
    | .many(n)
        n * 2
    | .none
        0
result = .none
-- result.name
result = .many(9)
-- match result
    | .many(n)
        n
    | .none
        0
-- step(result)
result = .none
-- step(result)

━━━━━━━━━━━━━━━━━━━━
+ do step String
    - value

if value is .none
    = "none"
= "many"
```

```output
none
many
6
none
9
many
none
```

### Anonymous cases with converted values and matched fields

```yumi
. shape = match 2
    |? false
        ..circle(1.5)
    ??
        ..rect(2, 3.5)
-- area(shape)
shape = .circle(2)
-- area(shape)
. label = match shape
    | .circle(r)
        ..round("circle", 2)
    | .rect(w, h)
        ..flat("rect")
-- label.name
-- match label
    | .round(text, size)
        "$text $size"
    | .flat(text)
        text
. nested = match true
    |? true
        ..wrapped(1, Box(2))
    ??
        ..bare
-- match nested
    | .wrapped(first, box)
        first + box.x
    | .bare
        0

━━━━━━━━━━━━━━━━━━━━
+ Box

+ x Int

────────────────────
+ This
    - x

━━━━━━━━━━━━━━━━━━━━
+ area Float
    - shape

match shape
    | .circle(r)
        r * r * 3.0
    | .rect(w, h)
        w * h
```

```output
7
12
round
circle 2
3
```

### Symbol of an anonymous case that two owners have is ambiguous

```yumi crash
. first = match true
    |? true
        ..ok
    ??
        ..failed

. second = match true
    |? true
        ..ok
    ??
        ..other

. third = .ok
-- third.name
```

```output
Exception: Anonymous case '.ok' is ambiguous between 'first', 'second'
<no file:14>
    . third = .ok
              ^^^
```

### Anonymous case with conflicting arity in one owner

```yumi crash
. result = match 1
    |? false
        ..ok(1)
    ??
        ..ok(1, 2)
-- result.name
```

```output
Exception: Anonymous case '..ok' has conflicting arity
<no file:6>
            ..ok(1, 2)
            ^^^^^^^^^^
```

### Anonymous case given a whole tuple converts it to the first field

```yumi crash
. point = Point(1, 2)
. nested = match true
    |? true
        ..wrapped(point)
    ??
        ..bare
-- nested.name

━━━━━━━━━━━━━━━━━━━━
+ Point is Tuple

+ x Int
+ y Int
```

```output
Exception: Expected I64 here, but this is Point, and there is no conversion between them
<no file:5>
            ..wrapped(point)
            ^^^^^^^^^^^^^^^^
```

### Anonymous case of a function holds a name a later arm's pattern captures

```yumi
-- label(true) is .loaded
-- label(false) is .failed

━━━━━━━━━━━━━━━━━━━━
+ label
    - works Bool

match catch risky(works)
    | error Error
        ..failed("no")
    | found [Int]
        ..loaded(found)

━━━━━━━━━━━━━━━━━━━━
+ do risky [Int] ?! Error
    - works Bool

?! Error("no") if !works
= [1, 2]
```

```output
true
true
```

### Anonymous case of a function holds its value as known where the case is written

```yumi
-- match pick(null)
    | .some(n)
        n + 1
    | .other(n)
        n ?? -1

━━━━━━━━━━━━━━━━━━━━
+ do pick
    - value Int?

if value
    = ..some(value)
= ..other(value)
```

```output
-1
```
