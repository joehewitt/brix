# Io

```yumi
Receiver() <-* [1, 2, 3]

━━━━━━━━━━━━━━━━━━━━
+ Receiver

────────────────────
+ do this <- item Int

-- "got $item"

━━━━━━━━━━━━━━━━━━━━
+ do river

<= 10
<= 20
<= 30
```

```output
got 1
got 2
got 3
```

```yumi
Receiver() <-* river()

━━━━━━━━━━━━━━━━━━━━
+ Receiver

────────────────────
+ do this <- item Int

-- "got $item"

━━━━━━━━━━━━━━━━━━━━
+ do river

<= 10
<= 20
<= 30
```

```output
got 10
got 20
got 30
```

```yumi
-- <- (101 .. < 103 as Iterable)
```

```output
101
```
