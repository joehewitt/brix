# Thread locals

```brix
(prop + scratch _ @thread (= 10))
(blank)

(main
  (let worker (call Worker 1))
  (let results (of SyncChannel String))
  (blank)
  (do-new (worker: worker)
    (+= scratch 1)
    (<- results (str "first " scratch))
  )
  (blank)
  (print (<- results))
  (blank)
  (do-new (worker: worker)
    (+= scratch 1)
    (<- results (str "second " scratch))
  )
  (blank)
  (print (<- results))
)
```

```output
first 11
second 12
```

```brix
(prop + parallel-scratch _ @thread (= 41))
(blank)

(main
  (let results (of SyncChannel Int))
  (blank)
  (do-new-thread
    (+= parallel-scratch 1)
    (<- results parallel-scratch)
  )
  (blank)
  (print (<- results))
)
```

```output
42
```

```brix
(prop + scratch _ @thread (= 0))
(blank)

(main
  (let worker-a (call Worker 1))
  (let worker-b (call Worker 1))
  (let results (of SyncChannel String))
  (blank)
  (do-new (worker: worker-a)
    (+= scratch 1)
    (<- results (str "a " scratch))
  )
  (blank)
  (print (<- results))
  (blank)
  (do-new (worker: worker-b)
    (+= scratch 1)
    (<- results (str "b " scratch))
  )
  (blank)
  (print (<- results))
  (blank)
  (do-new (worker: worker-a)
    (+= scratch 1)
    (<- results (str "a " scratch))
  )
  (blank)
  (print (<- results))
)
```

```output
a 1
b 1
a 2
```

```brix
(prop + thread-value _ @thread (= 0))
(blank)

(main
  (let worker-a (call Worker 1))
  (let worker-b (call Worker 1))
  (let results (of SyncChannel Int))
  (blank)
  (do-new (worker: worker-a)
    (= thread-value 101)
    (<- results thread-value)
  )
  (blank)
  (print (<- results))
  (blank)
  (do-new (worker: worker-b)
    (= thread-value 202)
    (<- results thread-value)
  )
  (blank)
  (print (<- results))
  (blank)
  (do-new (worker: worker-a)
    (<- results thread-value)
  )
  (blank)
  (print (<- results))
  (blank)
  (do-new (worker: worker-b)
    (<- results thread-value)
  )
  (blank)
  (print (<- results))
)
```

```output
101
202
101
202
```

```brix
(prop + logical _ @task (= 0))
(prop + physical _ @thread (= 0))
(type-line + Result (is (tuple (: label String) (: logical Int) (: physical Int))))
(blank)

(main
  (let worker (call Worker 1))
  (let results (of SyncChannel Result))
  (blank)
  (do-new (worker: worker)
    (+= logical 1)
    (+= physical 1)
    (<- results (tuple "task 1" logical physical))
  )
  (blank)
  (print (<- results))
  (blank)
  (do-new (worker: worker)
    (+= logical 1)
    (+= physical 1)
    (<- results (tuple "task 2" logical physical))
  )
  (blank)
  (print (<- results))
)
```

```output
(task 1, 1, 1)
(task 2, 1, 2)
```

```brix
(prop + inferred _ @thread (= (tuple 1 2)))
(prop + explicit Int @thread (= 5))
(blank)

(main
  (print (tuple inferred explicit))
  (= inferred (tuple 3 4))
  (= explicit 8)
  (print (tuple inferred explicit))
)
```

```output
((1, 2), 5)
((3, 4), 8)
```

```brix
(import ./lib/threadlocals)
(blank)

(main
  (print remote-count)
  (= remote-count 7)
  (print remote-count)
)
```

```brix file=lib/threadlocals.bx
(prop + remote-count _ @thread (= 3))
```

```output
3
7
```

### A global may read a thread local declared after it.

```brix
(main
  (print later)
  (print th)
)

(blank)
(prop + later _ (= (+ th 1)))
(prop + th _ @thread (= 5))
```

```output
6
5
```

### A thread-local's value may read one declared before it.

```brix
(main
  (print later)
)

(blank)
(prop + early _ @thread (= 5))
(prop + later _ @thread (= (+ early 1)))
```

```output
6
```

### A thread-local's value cannot read one declared after it, which takes its value later.

```brix crash
(main
  (print early)
)

(blank)
(prop + early _ @thread (= (+ later 1)))
(prop + later _ @thread (= 5))
```

```output
Exception: Global 'later' is read before it is initialized
<no file:7>
    (prop + early _ @thread (= (+ later 1)))
                                  ^^^^^
```

### A panic while a thread-local takes its value is reported, and ends the process.

```brix crash
(main
  (print label)
)

(blank)
(prop + label _ @thread (= (call pick)))

(fn-do + pick String
  (throw (call Panic "armed"))
  (return "ok")
)
```

```stderr
Unhandled panic: armed
<no file:10>

Stack trace:
  <no frames>
```
