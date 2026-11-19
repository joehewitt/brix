# Channels

```brix
(main
  (let numbers (of SyncChannel Int))
  (blank)
  (do-new
    (print (<- numbers))
  )
  (blank)
  (<- numbers 99)
)
```

```output
99
```

### A event escapes as one memoized Broadcastable adapter and keeps insertion order

```brix
(main
  (let source (call ChannelEventSource))
  (let first (of Broadcastable Int) (. source changed))
  (let second (of Broadcastable Int) (. source changed))
  (print (== (as first Pointer) (as second Pointer)))
  (blank)
  (let observer (call ChannelEventObserver first))
  (call install-channel-loop second)
  (blank)
  (call (. source emit) 8)
)

(fn-do + install-channel-loop _
  (param source (of Broadcastable Int))
  (for (on value source)
    (print (str "loop " value))
  )
)

(type + ChannelEventSource
  (event + changed Int)
  (fn-do + emit _
    (param value Int)
    (<- changed value)
  )
)

(type + ChannelEventObserver
  (prop + source (of Broadcastable Int))
  (fn-do + This _
    (param source _)
  )
  (subscribe-for (on value source)
    (print (str "class " value))
  )
)
```

```output
true
loop 8
class 8
```

### A event Broadcastable adapter cannot be written

```brix crash
(main
  (let source (call ChannelEventSource))
  (let adapter (of Broadcastable Int) (. source changed))
  (<- adapter 1)
)

(type + ChannelEventSource
  (event + changed Int)
)
```

```output
Exception: Operator "<-" is not defined for Broadcastable'I64
<no file:5>
      (<- adapter 1)
      ^^^^^^^^^^^^^^
```

### A write binds more tightly than `&&`, so writing a test of two values needs parentheses: the write itself has no value to test.

```brix compile-error
(main
  (let sender (call ChannelSender))
  (call (. sender send) true false)
)

(type + ChannelSender
  (event + requests Bool)
  (fn-do + send _
    (param first Bool)
    (param second Bool)
    (and (<- requests first) second)
  )
)
```

```output
Exception: (and …) cannot be a `row` of (fn-do …), which takes Step. This body runs steps, and each row is a step: a statement form, a `call`, markup, or a value that `pipe` stages after it work on. To return a value, write `(return value)`. If the body is only a value, write `fn-expr`, or remove `:do` from a closure.
<no file:12>
        (and (<- requests first) second)
        ^
```

### A read of one item takes it from a source that is a broadcastable through a property it delegates to.

```brix
(main
  (let ticker (call ChannelTicker))
  (let got (<- ticker))
  (print (str "got " got))
)

(type + ChannelTicker
  (implements (of Usable Void))
  (delegate (of Broadcastable Int))
  (prop _ channel _ (:= (call (of AsyncOnDemandChannel Int) this)))
  (fn-do + begin-using _
    (do-new
      (<- channel 7)
    )
  )
  (fn-do + end-using _)
)
```

```output
got 7
```

### A event endpoint cannot bind as an AsyncChannel

```brix crash
(main
  (let source (call ChannelEventSource))
  (let channel (of AsyncChannel Int) (. source changed))
)

(type + ChannelEventSource
  (event + changed Int)
)
```

```output
Exception: Event endpoints provide Broadcastable'I64, not AsyncChannel'I64
<no file:4>
      (let channel (of AsyncChannel Int) (. source changed))
           ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix
(main
  (let numbers (of SyncChannel Int))
  (blank)
  (do-new
    (for (in n numbers)
      (print n)
    )
  )
  (blank)
  (<-* numbers (..< 0 2))
  (call (. numbers close))
  (print "done")
)
```

```output
0
1
done
```

```brix
(main
  (let worker (call Worker 1))
  (let to-worker (of SyncChannel Int))
  (let from-worker (of SyncChannel Int))
  (blank)
  (do-new (worker: worker)
    (for (in i (..< 0 200))
      (let n (<- to-worker))
      (if (!= n i)
        (throw (call Panic (str "worker received " n " instead of " i)))
      )
      (<- from-worker i)
    )
  )
  (blank)
  (for (in i (..< 0 200))
    (<- to-worker i)
    (let n (<- from-worker))
    (if (!= n i)
      (throw (call Panic (str "main received " n " instead of " i)))
    )
  )
  (blank)
  (print "done")
)
```

```output
done
```

```brix
(main
  (let chan (call (of SyncBufferedChannel Int)))
  (blank)
  (do-new
    (<- chan 43)
    (<- chan 44)
    (call (. chan close))
  )
  (blank)
  (for (in item chan)
    (print item)
  )
)
```

```output
43
44
```

```brix
(type-line + Pair (is (tuple (: a Int) (: b Int))))
(blank)

(main
  (let pairs (of SyncChannel Pair))
  (blank)
  (do-new
    (for (in n pairs)
      (print n)
    )
  )
  (blank)
  (for (in i (..< 0 5))
    (<- pairs (call Pair i (+ i 1)))
  )
  (call (. pairs close))
  (print "done")
)
```

```output
(0, 1)
(1, 2)
(2, 3)
(3, 4)
(4, 5)
done
```

```brix
(main
  (let chan (call (of AsyncChannel Int)))
  (blank)
  (<- chan 43)
  (blank)
  (for (on item chan)
    (print item)
  )
  (blank)
  (<- chan 44)
)

; chan.close()
```

```output
44
```

```brix
(main
  (for (on item (call publisher (call foo 1)))
    (print item)
  )
)

(fn-do + foo _
  (param n _)
  (yield n)
  (yield (* n 2))
)
```

```output
1
2
```

```brix
(main
  (let foo (=> () 2))
  (print (call (of await Int) foo))
)
```

```output
2
```

```brix
(import /core/arena)
(import /BxCore/BxCore.h)
(blank)

(main
  (let reserved-before (call C:brixArenaRuntimeReservedBytes))
  (let output (of SyncBufferedChannel Int))
  (let scratch-pool (of SyncBufferedChannel Arena))
  (let worker (call Worker 4))
  (for (in _ (..< 0 4))
    (<- scratch-pool (call Arena))
  )
  (for (in value (..< 0 128))
    (do-new (worker: worker)
      (let scratch (!! (<- scratch-pool)))
      (finally
        (<- scratch-pool scratch)
      )
      (let buffer (call (of (. scratch allocate) Int) 1))
      (= ([] buffer 0) value)
      (let copied ([] buffer 0))
      (call (. scratch reset))
      (<- output copied)
    )
  )
  (blank)
  (let total 0)
  (for (in _ (..< 0 128))
    (+= total (!! (<- output)))
  )
  (call (. worker stop))
  (call (. scratch-pool close))
  (for (in scratch scratch-pool)
    (call (. scratch release))
  )
  (print (== total 8128))
  (print (== (call C:brixArenaRuntimeReservedBytes) reserved-before))
)
```

```output
true
true
```

### Publishing a finite stream closes the broadcaster once its subscribers get every item

```brix
(main
  (let gate (of SyncBufferedChannel Bool))
  (call drain gate 2)
  (call drain gate 0)
  (for (in n (call publisher (list 3 4)))
    (print n)
  )
  (print "list done")
)

(fn-do + drain _
  (param gate (of SyncBufferedChannel Bool))
  (param count Int)
  (let pub (call publisher (call gated gate count)))
  (let reader (as pub (of Iterable Int)))
  (call (. reader begin-iterating))
  (<- gate true)
  (let item (<- reader))
  (while item
    (print item)
    (= item (<- reader))
  )
  (print (. pub is-closed))
)

(fn-do + gated _
  (param gate (of SyncBufferedChannel Bool))
  (param count Int)
  (<- gate)
  (for (in n (..< 0 count))
    (yield (+ n 1))
  )
)
```

```output
1
2
true
true
3
4
list done
```

### await hands each result across threads, including when the producer posts before the waiter parks.

```brix
(main
  (let total 0)
  (for (in n (.. 1 300))
    (+= total (?? (call (of await Int) (=> () n)) 0))
  )
  (print total)
)
```

```output
45150
```

### A write waiting on a SyncChannel when it closes reaches the next reader

```brix
(main
  (let numbers (of SyncChannel Int))
  (do-new
    (<- numbers 1)
    (print "written")
  )
  (call yield-once)
  (call (. numbers close))
  (for (in n numbers)
    (print (str "read " n))
  )
  (print "end")
)

(fn-do + yield-once _
  (let done (call Semaphore))
  (do-new
    (call (. done post))
  )
  (call (. done wait))
)
```

```output
read 1
end
written
```

### A reader of a SyncBufferedChannel cancelled while it waits leaves the values written before it ends to other readers

```brix
(main
  (let numbers (of SyncBufferedChannel Int))
  (let reader
    (do-new
      (for (in n numbers)
        (print (str "reader took " n))
      )
    )
  )
  (call yield-once)
  (call (. reader cancel-later))
  (<- numbers 1)
  (<- numbers 2)
  (call (. reader wait))
  (call (. numbers close))
  (for (in n numbers)
    (print (str "left " n))
  )
)

(fn-do + yield-once _
  (let done (call Semaphore))
  (do-new
    (call (. done post))
  )
  (call (. done wait))
)
```

```output
left 1
left 2
```

### A write to a SyncChannel passes over a reader cancelled while it waits and goes to the next

```brix
(main
  (let numbers (of SyncChannel Int))
  (let cancelled
    (do-new
      (for (in n numbers)
        (print (str "cancelled reader took " n))
      )
    )
  )
  (call yield-once)
  (let other
    (do-new
      (print (str "other took " (<- numbers)))
    )
  )
  (call yield-once)
  (call (. cancelled cancel-later))
  (<- numbers 1)
  (print "written")
  (call (. numbers close))
  (call (. other wait))
)

(fn-do + yield-once _
  (let done (call Semaphore))
  (do-new
    (call (. done post))
  )
  (call (. done wait))
)
```

```output
other took 1
written
```

### Reads drain what an AsyncBufferedChannel buffered before it closed, and later writes are dropped

```brix
(main
  (let numbers (call (of AsyncBufferedChannel Int)))
  (<- numbers 1)
  (<- numbers 2)
  (call (. numbers close))
  (<- numbers 3)
  (print (<- numbers))
  (print (<- numbers))
  (let rest (<- numbers))
  (print (if-expr (not rest) "drained" (str "left " rest)))
)
```

```output
1
2
drained
```

### An AsyncBufferedChannel releases a message once it is read, past a wrap, and keeps one still buffered

```brix
(import /core/gc)
(blank)
(import /BxGC/BxGC.h)
(blank)

(main
  (let channel (call (of AsyncBufferedChannel Node)))
  (let watched (call read-past-wrap channel))
  (call C:brixCollect)
  (print (== (. ([] watched 0) get) null))
  (print (?. (. ([] watched 1) get) value))
  (call (. channel close))
  (print (?. (<- channel) value))
  (print (if-expr (not (<- channel)) "drained" "left"))
)

(fn-do + read-past-wrap _
  (param channel (of AsyncBufferedChannel Node))
  (for (in i (..< 0 16))
    (<- channel (call Node i))
  )
  (for (in i (..< 0 15))
    (<- channel)
  )
  (let waiting (call Node 17))
  (<- channel (call Node 16))
  (<- channel waiting)
  (<- channel)
  (let delivered (<- channel))
  (return (tuple (call Weak (!! delivered)) (call Weak waiting)))
)

(type + Node
  (prop + value _ (= 0))
  (fn-do + This _
    (param v Int)
    (= value v)
  )
)
```

```output
true
17
17
drained
```

### An event reached through an optional chain takes a write when the chain holds a value, and skips it when it is null

```brix
(main
  (let holder (call PingHolder))
  (= (. holder pinger) (call Pinger))
  (for (on value (. (!! (. holder pinger)) pinged))
    (print (str "pinged " value))
  )
  (<- (?. (. holder pinger) pinged) 3)
  (= (. holder pinger) null)
  (<- (?. (. holder pinger) pinged) 4)
  (print "done")
)

(type + Pinger
  (event + pinged Int)
)

(type + PingHolder
  (prop + pinger (? Pinger))
)
```

```output
pinged 3
done
```
