# Stack local edges

```yumi
+ @stack current = 10

do with current = 20
    -- current
-- current
```

```output
20
10
```

```yumi
+ @stack current = 5

-- scoped()
-- current

━━━━━━━━━━━━━━━━━━━━
+ do scoped Int

do with current = 99
    = current
```

```output
99
5
```

```yumi crash
+ @stack current Int

current = 4
```

```output
Exception: Stack-local variable 'current' can only be rebound with do with
<no file:4>
    current = 4
    ^^^^^^^^^^^
```

```yumi crash
+ @stack current Int

-- current
```

```output
Exception: Stack-local variable 'current' is not bound
<no file:4>
    -- current
       ^^^^^^^
```

```yumi crash
+ @stack current Int

-- read-current()

━━━━━━━━━━━━━━━━━━━━
+ read-current Int

current
```

```output
Exception: Stack-local variable 'current' is not bound: main → read-current
<no file:9>
    current
    ^^^^^^^
```

```yumi
+ @stack current Int

. first Getter'Int?
. second Getter'Int?
do with current = 1
    . captured = => current
    first = captured as Getter'Int
do with current = 2
    . captured = => current
    second = captured as Getter'Int

-- (first)()
-- (second)()
```

```output
1
2
```

```yumi
+ @stack current Int
+ @stack unused String

do with current = 73
    do new
        -- current
```

```output
73
```

```yumi
+ @stack current Int

. read Getter'Int?
do with current = 64
    . captured = => read-current()
    read = captured as Getter'Int

-- (read)()

━━━━━━━━━━━━━━━━━━━━
+ read-current Int

current
```

```output
64
```

```yumi
+ @stack current = 5

. read Getter'Int?
do with current = 64
    . captured = => read-current()
    read = captured as Getter'Int

-- (read)()
-- current

━━━━━━━━━━━━━━━━━━━━
+ read-current Int

current
```

```output
64
5
```

```yumi
+ @stack current = 5

. results SyncChannel'Int
do with current = 81
    do new
        results <- read-current()

-- <- results
-- current

━━━━━━━━━━━━━━━━━━━━
+ read-current Int

current
```

```output
81
5
```

```yumi
+ @stack current Int

do with current = 91
    -- odd(3)

━━━━━━━━━━━━━━━━━━━━
+ do odd Int
    - n Int

= even(n - 1)

━━━━━━━━━━━━━━━━━━━━
+ do even Int
    - n Int

if n == 0
    = current
= odd(n - 1)
```

```output
91
```

```yumi compile-error
+ @stack current Int

. reader = Impl() as Reader
-- reader.read()

━━━━━━━━━━━━━━━━━━━━
+ Reader is Interface

────────────────────
+ read Int

━━━━━━━━━━━━━━━━━━━━
+ Impl
    is Reader

────────────────────
+ read Int

current
```

```output
Exception: Stack-local variable 'current' is not bound: main → Reader.read → Impl.read
<no file:20>
    current
    ^^^^^^^
```

```yumi compile-error
+ @stack current Int

. base Base = Derived()
-- base.value()

━━━━━━━━━━━━━━━━━━━━
+ Base

────────────────────
+ This

────────────────────
+ value Int

0

━━━━━━━━━━━━━━━━━━━━
+ Derived is Base

────────────────────
+ This

────────────────────
+ value Int

current
```

```output
Exception: Stack-local variable 'current' is not bound: main → Base.value → Derived.value
<no file:27>
    current
    ^^^^^^^
```

```yumi compile-error
+ @stack current Int

. get Getter'Int = read-current
do with current = 4
    -- get()

━━━━━━━━━━━━━━━━━━━━
+ read-current Int

current
```

```output
Exception: Stack-local variable 'current' cannot be read through a function value: read-current
<no file:4>
    . get Getter'Int = read-current
          ^^^^^^^^^^
```

```yumi compile-error
+ @stack current Int

. get = => current
do with current = 4
    -- get()
```

```output
Exception: Stack-local variable 'current' is not bound: main → (closure)
<no file:4>
    . get = => current
               ^^^^^^^
```

```yumi compile-error
+ @stack current Int

-- made()

━━━━━━━━━━━━━━━━━━━━
+ do made Int

. get = => current
= get()
```

```output
Exception: Stack-local variable 'current' is not bound: main → made → (closure)
<no file:9>
    . get = => current
               ^^^^^^^
```

```yumi
+ @stack current Int

. reader = Impl() as Reader
do with current = 4
    -- reader.read()
    . get = => current + 1
    -- get()
-- made(7)

━━━━━━━━━━━━━━━━━━━━
+ Reader is Interface

────────────────────
+ read Int

━━━━━━━━━━━━━━━━━━━━
+ Impl
    is Reader

────────────────────
+ read Int

current

━━━━━━━━━━━━━━━━━━━━
+ do made Int
    - n Int

do with current = n
    . get = => current * 2
    = get()
= 0
```

```output
4
5
14
```

```yumi compile-error
+ @stack first Int
+ @stack second Int

-- first + second
```

```output
Exception: Stack-local variable 'first' is not bound
<no file:5>
    -- first + second
       ^^^^^

Exception: Stack-local variable 'second' is not bound
<no file:5>
    -- first + second
               ^^^^^^
```
