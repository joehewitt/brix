# Enum case patterns

### Enum members match as values with |=.

```yumi
describe(Kind.b)

━━━━━━━━━━━━━━━━━━━━
+ Kind is U8

| a
| b

━━━━━━━━━━━━━━━━━━━━
+ do describe
    - kind Kind

match kind
    |= .a
        -- "a"
    |= .b
        -- "b"
```

```output
b
```

### A union case arm on an enum names the enum value syntax.

```yumi crash
describe(Kind.b)

━━━━━━━━━━━━━━━━━━━━
+ Kind is U8

| a
| b

━━━━━━━━━━━━━━━━━━━━
+ do describe
    - kind Kind

match kind
    | .a
        -- "a"
    | .b
        -- "b"
```

```output
Exception: Kind.a is an enum value, not a union case; match it with '|= .a'
<no file:15>
        | .a
          ^^
```

### A qualified union case arm on an enum names the qualified value syntax.

```yumi crash
describe(Kind.b)

━━━━━━━━━━━━━━━━━━━━
+ Kind is U8

| a
| b

━━━━━━━━━━━━━━━━━━━━
+ do describe
    - kind Kind

match kind
    |= Kind.a
        -- "a"
    | Kind.b
        -- "b"
```

```output
Exception: Kind.b is an enum value, not a union case; match it with '|= Kind.b'
<no file:17>
        | Kind.b
          ^^^^^^
```

### Expression matches report the enum value syntax.

```yumi crash
-- describe(Kind.b)

━━━━━━━━━━━━━━━━━━━━
+ Kind is U8

| a
| b

━━━━━━━━━━━━━━━━━━━━
+ describe
    - kind Kind

match kind
    | .a
        "a"
    | .b
        "b"
```

```output
Exception: Kind.a is an enum value, not a union case; match it with '|= .a'
<no file:15>
        | .a
          ^^
```

### Optional matches report the enum value syntax instead of skipping the arm.

```yumi crash
. kind = Kind.b
match? kind
    | Kind.a
        -- "a"
    | Kind.b
        -- "b"
-- "done"

━━━━━━━━━━━━━━━━━━━━
+ Kind is U32

| a = 0
| b = 1
```

```output
Exception: Kind.a is an enum value, not a union case; match it with '|= Kind.a'
<no file:4>
        | Kind.a
          ^^^^^^
```

### Matches with a default arm report the enum value syntax.

```yumi crash
. kind = Kind.b
match kind
    | .b
        -- "b"
    | _
        -- "other"

━━━━━━━━━━━━━━━━━━━━
+ Kind is U32

| a = 0
| b = 1
```

```output
Exception: Kind.b is an enum value, not a union case; match it with '|= .b'
<no file:4>
        | .b
          ^^
```

### An is test that names an enum value as a case reports the enum value syntax.

```yumi crash
. kind = Kind.b
-- kind is .a
-- "done"

━━━━━━━━━━━━━━━━━━━━
+ Kind is U32

| a = 0
| b = 1
```

```output
Exception: Kind.a is an enum value, not a union case; match it with '|= .a'
<no file:3>
    -- kind is .a
               ^^
```

### An arm that is a name alone binds the name even where the enum has a value of that name.

```yumi compile-error
describe(Kind.b)

━━━━━━━━━━━━━━━━━━━━
+ Kind is U8

| a
| b

━━━━━━━━━━━━━━━━━━━━
+ do describe
    - kind Kind

match kind
    |= Kind.a
        -- "a"
    | b
        -- "b"
```

```output
Exception: Arm '| b' binds a new name, not the value Kind.b; match the value with '|= Kind.b'
<no file:17>
        | b
          ^
```

### An arm written `|=` compares the subject with one value, so values listed there for an enum are an error that names a condition, which takes any of several.

```yumi compile-error
-- describe(Kind.b)

━━━━━━━━━━━━━━━━━━━━
+ Kind is U8

| a
| b
| c

━━━━━━━━━━━━━━━━━━━━
+ describe String
    - kind Kind

match kind
    |= .a, .b
        "a or b"
    ??
        "c"
```

```output
Exception: An arm written '|=' compares the subject with one value, and this lists several; to take any of several, write a condition, as in '|? it == .a || it == .b'
<no file:16>
        |= .a, .b
           ^^^^^^
```

### A condition arm takes any of several enum values.

```yumi
-- describe(Kind.b)
-- describe(Kind.c)

━━━━━━━━━━━━━━━━━━━━
+ Kind is U8

| a
| b
| c

━━━━━━━━━━━━━━━━━━━━
+ describe String
    - kind Kind

match kind
    |? it == .a || it == .b
        "a or b"
    ??
        "c"
```

```output
a or b
c
```
