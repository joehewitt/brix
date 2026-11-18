# Lint expression functions

```yumi
_ current Int = 1

-- eager()
current = 1
-- deferred()

━━━━━━━━━━━━━━━━━━━━
+ do eager Int

. snapshot = current
?! Panic("unexpected")
    if advance()
= snapshot

━━━━━━━━━━━━━━━━━━━━
+ deferred Int

?! Panic("unexpected")
    if advance()
snapshot
    .. snapshot = current

━━━━━━━━━━━━━━━━━━━━
+ do advance Bool

current = 2
= false
```

```output
1
2
```

```yumi
_ invocations Int = 0

-- invoke(DefaultHook())
-- invoke(ActiveHook())
-- invoke(ActiveHook())

━━━━━━━━━━━━━━━━━━━━
+ Hook is Interface

────────────────────
+ do value Int

= 0

━━━━━━━━━━━━━━━━━━━━
+ DefaultHook
    is Hook

━━━━━━━━━━━━━━━━━━━━
+ ActiveHook
    is Hook

────────────────────
+ do value Int

invocations += 1
= invocations

━━━━━━━━━━━━━━━━━━━━
+ do invoke Int
    - hook Hook

= hook.value()
```

```output
0
1
2
```

```yumi
_ invocations Int = 0

-- invoke(DefaultHook())
-- invoke(ActiveHook())
-- invoke(ActiveHook())

━━━━━━━━━━━━━━━━━━━━
+ Hook is Interface

────────────────────
+ value Int

0

━━━━━━━━━━━━━━━━━━━━
+ DefaultHook
    is Hook

━━━━━━━━━━━━━━━━━━━━
+ ActiveHook
    is Hook

────────────────────
+ do value Int

invocations += 1
= invocations

━━━━━━━━━━━━━━━━━━━━
+ do invoke Int
    - hook Hook

= hook.value()
```

```output
0
1
2
```
