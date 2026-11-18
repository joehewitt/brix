# Probes

```yumi
. name = "world"
. items = [1, 2, 3]

// hello $name
// plain list $items
//status = "ready"
//items:status = "ready"
//+ setup $name with $!items
// inside
//- goodbye $name with $!items
// list $!items

-- "ok"
```

```output
ok
```

```yumi
. count = 0

//http.value -> value
    .. value = tick("outer") ++ nested
        .. nested = tick("inner")

-- count

━━━━━━━━━━━━━━━━━━━━
+ do tick
    - label String

-- "tick $label"
= label
```

```output
0
```

### A probe emits nothing where probes are not asked for: nothing of it is evaluated.

```yumi
. count = 0

//demo.calls ++
//demo.total += noisy()
//demo.value -> noisy()
//demo.state = noisy()
// said ${noisy()}
count += 1
-- count
-- quiet(3)

━━━━━━━━━━━━━━━━━━━━
+ do noisy Int

-- "noisy"
= 1

━━━━━━━━━━━━━━━━━━━━
+ do quiet Int
    - n Int

//demo.quiet ++
//demo.quiet-value -> noisy()
= n + 1
```

```output
1
4
```

### What is indented under a probe belongs to the probe: none of it runs where probes are not asked for.

```yumi
. count = 0
//demo.calls ++
    count += 1
//demo.state = count
    -- "under a probe"
//demo.value -> shown
    .. shown = noisy()
-- count
-- f(1)

━━━━━━━━━━━━━━━━━━━━
+ do f Int
    - n Int

//demo.calls ++
    = 99
= n

━━━━━━━━━━━━━━━━━━━━
+ do noisy Int

-- "noisy"
= 1
```

```output
0
1
```

### A counter adds a count of any integer type, such as a `Size`, with no `as`.

```yumi probes=demo
_ calls Size = 0

for index in 0 .. < 3
    calls += 1
//demo.calls += calls
. pairs Size = calls * 2
//demo.pairs += pairs
-- calls
```

```output
3
```

### Where probes are asked for, what is indented under a probe runs with it, in order.

```yumi probes=demo
. count = 0
//demo.calls ++
    count += 1
//demo.state = count
    -- "under a probe"
//demo.value -> shown
    .. shown = noisy()
-- count
-- f(1)

━━━━━━━━━━━━━━━━━━━━
+ do f Int
    - n Int

//demo.calls ++
    = 99
= n

━━━━━━━━━━━━━━━━━━━━
+ do noisy Int

-- "noisy"
= 1
```

```output
under a probe
noisy
1
99
```

```yumi probes=snapshot.array
. values = U8[3]
values[0] = 7
//snapshot.array// values $!values
-- values[0]
```

```output
7
```
