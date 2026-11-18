# Lint overloads

```yumi
./one
./two

-- pick(10)
-- pick(10, 20)
. partial = pick(...)
-- partial(10)
-- partial(10, 20)
```

```yumi file=one.yumi
━━━━━━━━━━━━━━━━━━━━
+ pick
    - value Int

1
```

```yumi file=two.yumi
━━━━━━━━━━━━━━━━━━━━
+ pick
    - value Int
    - other Int

2
```

```output
1
2
1
2
```

```yumi
./two
./one

-- pick(10)
-- pick(10, 20)
. partial = pick(...)
-- partial(10)
-- partial(10, 20)
```

```yumi file=one.yumi
━━━━━━━━━━━━━━━━━━━━
+ pick
    - value Int

1
```

```yumi file=two.yumi
━━━━━━━━━━━━━━━━━━━━
+ pick
    - value Int
    - other Int

2
```

```output
1
2
1
2
```
