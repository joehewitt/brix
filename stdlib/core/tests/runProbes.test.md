# Probes

```brix
(main
  (let name "world")
  (let items (list 1 2 3))
  (blank)
  (probe-log _ (text: "hello " name))
  (probe-log _ (text: "plain list " items))
  (probe-assign status "ready")
  (probe-assign (: items status) "ready")
  (probe-group + _ (text: "setup " name " with " (snapshot items)))
  (probe-log _ (text: "inside"))
  (probe-group - _ (text: "goodbye " name " with " (snapshot items)))
  (probe-log _ (text: "list " (snapshot items)))
  (blank)
  (print "ok")
)
```

```output
ok
```

```brix
(main
  (let count 0)
  (blank)
  (probe-log http.value (items: value)
    (block
      (lazy value
        (scope (++ (call tick "outer") nested)
          (lazy nested (call tick "inner"))
        )
      )
    )
  )
  (blank)
  (print count)
)

(fn-do + tick _
  (param label String)
  (print (str "tick " label))
  (return label)
)
```

```output
0
```

### A probe emits nothing where probes are not asked for: nothing of it is evaluated.

```brix
(main
  (let count 0)
  (blank)
  (probe-count demo.calls)
  (probe-count demo.total (call noisy))
  (probe-log demo.value (items: (call noisy)))
  (probe-assign demo.state (call noisy))
  (probe-log _ (text: "said " (call noisy)))
  (+= count 1)
  (print count)
  (print (call quiet 3))
)

(fn-do + noisy Int
  (print "noisy")
  (return 1)
)

(fn-do + quiet Int
  (param n Int)
  (probe-count demo.quiet)
  (probe-log demo.quiet-value (items: (call noisy)))
  (return (+ n 1))
)
```

```output
1
4
```

### What is indented under a probe belongs to the probe: none of it runs where probes are not asked for.

```brix
(main
  (let count 0)
  (probe-count demo.calls
    (block
      (+= count 1)
    )
  )
  (probe-assign demo.state count
    (block
      (print "under a probe")
    )
  )
  (probe-log demo.value (items: shown)
    (block
      (lazy shown (call noisy))
    )
  )
  (print count)
  (print (call f 1))
)

(fn-do + f Int
  (param n Int)
  (probe-count demo.calls
    (block
      (return 99)
    )
  )
  (return n)
)

(fn-do + noisy Int
  (print "noisy")
  (return 1)
)
```

```output
0
1
```

### A counter adds a count of any integer type, such as a `Size`, with no `as`.

```brix probes=demo
(prop _ calls Size (= 0))
(blank)

(main
  (for (in index (..< 0 3))
    (+= calls 1)
  )
  (probe-count demo.calls calls)
  (let pairs Size (* calls 2))
  (probe-count demo.pairs pairs)
  (print calls)
)
```

```output
3
```

### Where probes are asked for, what is indented under a probe runs with it, in order.

```brix probes=demo
(main
  (let count 0)
  (probe-count demo.calls
    (block
      (+= count 1)
    )
  )
  (probe-assign demo.state count
    (block
      (print "under a probe")
    )
  )
  (probe-log demo.value (items: shown)
    (block
      (lazy shown (call noisy))
    )
  )
  (print count)
  (print (call f 1))
)

(fn-do + f Int
  (param n Int)
  (probe-count demo.calls
    (block
      (return 99)
    )
  )
  (return n)
)

(fn-do + noisy Int
  (print "noisy")
  (return 1)
)
```

```output
under a probe
noisy
1
99
```

### A statement under a key compiles wherever a counter of that key does, so the counter can read what it sets.

```brix probes=profile:demo
(main
  (probe demo.start (let start 5))
  (probe-count demo.start (- 7 start))
  (print "counted")
)
```

```output
counted
```

```brix probes=snapshot.array
(main
  (let values ([] U8 3))
  (= ([] values 0) 7)
  (probe-log snapshot.array (text: "values " (snapshot values)))
  (print ([] values 0))
)
```

```output
7
```
