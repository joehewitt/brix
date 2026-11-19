# Mem2reg

### mem2reg: else-if should re-read the original argument in the chained branch

```brix
(main
  (call check 1)
  (call check 11)
  (call check 3)
)

(fn-do + check _
  (param x I32)
  (if (> x 10)
    (print "a")
    (else-if (>= x 2)
      (print "b")
    )
    (else
      (print "c")
    )
  )
  (print "done")
)
```

```output
c
done
a
done
b
done
```

### mem2reg: a join with one updated local should not clobber an unrelated local

```brix
(main
  (let cond 1)
  (let a 10)
  (let b 20)
  (if (> cond 0)
    (= a 1)
    (else
      (= a 3)
    )
  )
  (print a)
  (print b)
)
```

```output
1
20
```

### mem2reg: both sides of the same join should preserve the correct values

```brix
(main
  (let cond 0)
  (let a 10)
  (let b 20)
  (if (> cond 0)
    (= a 1)
    (else
      (= a 3)
    )
  )
  (print a)
  (print b)
)
```

```output
3
20
```

### mem2reg: a multi-variable join should keep the branch-specific pair

```brix
(main
  (let cond 1)
  (let a 10)
  (let b 20)
  (if (> cond 0)
    (= a 1)
    (= b 2)
    (else
      (= a 3)
      (= b 4)
    )
  )
  (print a)
  (print b)
)
```

```output
1
2
```

### mem2reg: the false branch of the same multi-variable join should also work

```brix
(main
  (let cond 0)
  (let a 10)
  (let b 20)
  (if (> cond 0)
    (= a 1)
    (= b 2)
    (else
      (= a 3)
      (= b 4)
    )
  )
  (print a)
  (print b)
)
```

```output
3
4
```

### mem2reg: a loop-carried single local should converge to the final value

```brix
(main
  (let a 3)
  (while (> a 0)
    (-= a 1)
  )
  (print a)
)
```

```output
0
```

### mem2reg: multiple loop-carried locals should keep their own values

```brix
(main
  (let a 2)
  (let b 20)
  (while (> a 0)
    (-= a 1)
    (-= b 10)
  )
  (print a)
  (print b)
)
```

```output
0
0
```

### mem2reg: a partially-defined optional local should not crash on the defined path

```brix
(main
  (let cond 1)
  (let a (? I32))
  (if (> cond 0)
    (= a 1)
  )
  (print a)
)
```

```output
1
```

### mem2reg: a partially-defined optional local should not crash on the undefined path

```brix
(main
  (let cond 0)
  (let a (? I32))
  (if (> cond 0)
    (= a 1)
  )
  (print a)
)
```

```output
unknown
```

### mem2reg: a while loop should serialize a two-variable swap as a parallel copy

```brix
(main
  (let a 1)
  (let b 2)
  (let i 0)
  (while (< i 5)
    (let t a)
    (= a b)
    (= b t)
    (+= i 1)
  )
  (print a)
  (print b)
)
```

```output
2
1
```

### mem2reg: a for loop should serialize a two-variable swap as a parallel copy

```brix
(main
  (let a 1)
  (let b 2)
  (for (in i (..< 0 4))
    (let t a)
    (= a b)
    (= b t)
  )
  (print a)
  (print b)
)
```

```output
1
2
```

### mem2reg: a while loop should break a three-variable phi cycle

```brix
(main
  (let a 1)
  (let b 2)
  (let c 3)
  (let i 0)
  (while (< i 4)
    (let t a)
    (= a b)
    (= b c)
    (= c t)
    (+= i 1)
  )
  (print a)
  (print b)
  (print c)
)
```

```output
2
3
1
```

### mem2reg: a for loop should break a three-variable phi cycle

```brix
(main
  (let a 1)
  (let b 2)
  (let c 3)
  (for (in i (..< 0 5))
    (let t a)
    (= a b)
    (= b c)
    (= c t)
  )
  (print a)
  (print b)
  (print c)
)
```

```output
3
1
2
```

### mem2reg: an acyclic phi overwrite chain should retain every pre-edge value

```brix
(main
  (let previous 0)
  (let current 1)
  (let i 0)
  (while (< i 10)
    (let next (+ previous current))
    (= previous current)
    (= current next)
    (+= i 1)
  )
  (print previous)
  (print current)
)
```

```output
55
89
```

### mem2reg: register cycles and constant phi inputs should coexist on one edge

```brix
(main
  (let a 1)
  (let b 2)
  (let state 0)
  (for (in i (..< 0 3))
    (let t a)
    (= a b)
    (= b t)
    (= state 7)
  )
  (print a)
  (print b)
  (print state)
)
```

```output
2
1
7
```

### mem2reg: self-copies and a split conditional-continue edge should not stall serialization

```brix
(main
  (let a 1)
  (let b 2)
  (let swaps 0)
  (for (in i (..< 0 5))
    (if (== (% i 2) 0)
      (continue)
    )
    (blank)
    (let t a)
    (= a b)
    (= b t)
    (+= swaps 1)
  )
  (print a)
  (print b)
  (print swaps)
)
```

```output
1
2
2
```
