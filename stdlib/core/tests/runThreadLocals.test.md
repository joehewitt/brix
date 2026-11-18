# Thread locals

```yumi
+ @thread scratch = 10

. worker = Worker(1)
. results SyncChannel'String

do (worker) new
    scratch += 1
    results <- "first $scratch"

-- <- results

do (worker) new
    scratch += 1
    results <- "second $scratch"

-- <- results
```

```output
first 11
second 12
```

```yumi
+ @thread parallel-scratch = 41

. results SyncChannel'Int

do new thread
    parallel-scratch += 1
    results <- parallel-scratch

-- <- results
```

```output
42
```

```yumi
+ @thread scratch = 0

. worker-a = Worker(1)
. worker-b = Worker(1)
. results SyncChannel'String

do (worker-a) new
    scratch += 1
    results <- "a $scratch"

-- <- results

do (worker-b) new
    scratch += 1
    results <- "b $scratch"

-- <- results

do (worker-a) new
    scratch += 1
    results <- "a $scratch"

-- <- results
```

```output
a 1
b 1
a 2
```

```yumi
+ @thread thread-value = 0

. worker-a = Worker(1)
. worker-b = Worker(1)
. results SyncChannel'Int

do (worker-a) new
    thread-value = 101
    results <- thread-value

-- <- results

do (worker-b) new
    thread-value = 202
    results <- thread-value

-- <- results

do (worker-a) new
    results <- thread-value

-- <- results

do (worker-b) new
    results <- thread-value

-- <- results
```

```output
101
202
101
202
```

```yumi
+ @task logical = 0
+ @thread physical = 0
+ Result is (label String, logical Int, physical Int)

. worker = Worker(1)
. results SyncChannel'Result

do (worker) new
    logical += 1
    physical += 1
    results <- ("task 1", logical, physical)

-- <- results

do (worker) new
    logical += 1
    physical += 1
    results <- ("task 2", logical, physical)

-- <- results
```

```output
(task 1, 1, 1)
(task 2, 1, 2)
```

```yumi
+ @thread inferred = (1, 2)
+ @thread explicit Int = 5

-- inferred, explicit
inferred = (3, 4)
explicit = 8
-- inferred, explicit
```

```output
((1, 2), 5)
((3, 4), 8)
```

```yumi
./lib/threadlocals

-- remote-count
remote-count = 7
-- remote-count
```

```yumi file=lib/threadlocals.yumi

+ @thread remote-count = 3

```

```output
3
7
```

### A global may read a thread local declared after it.

```yumi
-- later
-- th

+ later = th + 1
+ @thread th = 5
```

```output
6
5
```

### A thread-local's value may read one declared before it.

```yumi
-- later

+ @thread early = 5
+ @thread later = early + 1
```

```output
6
```

### A thread-local's value cannot read one declared after it, which takes its value later.

```yumi crash
-- early

+ @thread early = later + 1
+ @thread later = 5
```

```output
Exception: Global 'later' is read before it is initialized
<no file:4>
    + @thread early = later + 1
                      ^^^^^
```

### A panic while a thread-local takes its value is reported, and ends the process.

```yumi crash
-- label

+ @thread label = pick()

━━━━━━━━━━━━━━━━━━━━
+ do pick String

?! Panic("armed")
= "ok"
```

```stderr
Unhandled panic: armed
<no file:9>

Stack trace:
  <no frames>
```
