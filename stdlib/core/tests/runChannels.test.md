# Channels

```yumi
. numbers SyncChannel'Int

do new
    -- <- numbers

numbers <- 99
```

```output
99
```

### A event escapes as one memoized Broadcastable adapter and keeps insertion order

```yumi
. source = ChannelEventSource()
. first Broadcastable'Int = source.changed
. second Broadcastable'Int = source.changed
-- first as Pointer == second as Pointer

. observer = ChannelEventObserver(first)
install-channel-loop(second)

source.emit(8)

━━━━━━━━━━━━━━━━━━━━
+ do install-channel-loop
    - source Broadcastable'Int

for value on source
    -- "loop $value"

━━━━━━━━━━━━━━━━━━━━
+ ChannelEventSource

+ event changed Int

────────────────────
+ do emit
    - value Int

changed <- value

━━━━━━━━━━━━━━━━━━━━
+ ChannelEventObserver

+ source Broadcastable'Int

────────────────────
+ This
    - source

────────────────────
for value on source

-- "class $value"
```

```output
true
loop 8
class 8
```

### A event Broadcastable adapter cannot be written

```yumi crash
. source = ChannelEventSource()
. adapter Broadcastable'Int = source.changed
adapter <- 1

━━━━━━━━━━━━━━━━━━━━
+ ChannelEventSource

+ event changed Int
```

```output
Exception: Operator "<-" is not defined for Broadcastable'I64
<no file:4>
    adapter <- 1
    ^^^^^^^^^^^^
```

### A write binds more tightly than `&&`, so writing a test of two values needs parentheses: the write itself has no value to test.

```yumi compile-error
. sender = ChannelSender()
sender.send(true, false)

━━━━━━━━━━━━━━━━━━━━
+ ChannelSender

+ event requests Bool

────────────────────
+ do send
    - first Bool
    - second Bool

requests <- first && second
```

```output
Exception: Operator "and" is not defined for Void and Bool
<no file:15>
    requests <- first && second
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A read of one item takes it from a source that is a broadcastable through a property it delegates to.

```yumi
. ticker = ChannelTicker()
. got = <- ticker
-- "got $got"

━━━━━━━━━━━━━━━━━━━━
+ ChannelTicker
    is Usable'Void

as Broadcastable'Int for
_ channel := AsyncOnDemandChannel'Int(this)

────────────────────
+ do begin-using

do new
    channel <- 7

────────────────────
+ do end-using
```

```output
got 7
```

### A event endpoint cannot bind as an AsyncChannel

```yumi crash
. source = ChannelEventSource()
. channel AsyncChannel'Int = source.changed

━━━━━━━━━━━━━━━━━━━━
+ ChannelEventSource

+ event changed Int
```

```output
Exception: Event endpoints provide Broadcastable'I64, not AsyncChannel'I64
<no file:3>
    . channel AsyncChannel'Int = source.changed
      ^^^^^^^^^^^^^^^^^^^^^^^^
```

```yumi
. numbers SyncChannel'Int

do new
    for n in numbers
        -- n

numbers <-* 0 .. < 2
numbers.close()
-- "done"
```

```output
0
1
done
```

```yumi
. worker = Worker(1)
. to-worker SyncChannel'Int
. from-worker SyncChannel'Int

do (worker) new
    for i in 0 .. < 200
        . n = <- to-worker
        if n != i
            ?! Panic("worker received $n instead of $i")
        from-worker <- i

for i in 0 .. < 200
    to-worker <- i
    . n = <- from-worker
    if n != i
        ?! Panic("main received $n instead of $i")

-- "done"
```

```output
done
```

```yumi
. chan = SyncBufferedChannel'Int()

do new
    chan <- 43
    chan <- 44
    chan.close()

for item in chan
    -- item
```

```output
43
44
```

```yumi
+ Pair is (a Int, b Int)

. pairs SyncChannel'Pair

do new
    for n in pairs
        -- n

for i in 0 .. < 5
    pairs <- Pair(i, i + 1)
pairs.close()
-- "done"
```

```output
(0, 1)
(1, 2)
(2, 3)
(3, 4)
(4, 5)
done
```

```yumi
. chan = AsyncChannel'Int()

chan <- 43

for item on chan
    -- item

chan <- 44
`chan.close()
```

```output
44
```

```yumi
for item on publisher(foo(1))
    -- item

━━━━━━━━━━━━━━━━━━━━
+ do foo
    - n

<= n
<= n * 2
```

```output
1
2
```

```yumi
. foo = => 2
-- await'Int(foo)
```

```output
2
```

```yumi
/core/arena
/BxCore/BxCore.h

. reserved-before = C:brixArenaRuntimeReservedBytes()
. output SyncBufferedChannel'Int
. scratch-pool SyncBufferedChannel'Arena
. worker = Worker(4)
for _ in 0 .. < 4
    scratch-pool <- Arena()
for value in 0 .. < 128
    do (worker) new
        . scratch = (<- scratch-pool)!
        do finally
            scratch-pool <- scratch
        . buffer = scratch.allocate'Int(1)
        buffer[0] = value
        . copied = buffer[0]
        scratch.reset()
        output <- copied

. total = 0
for _ in 0 .. < 128
    total += (<- output)!
worker.stop()
scratch-pool.close()
for scratch in scratch-pool
    scratch.release()
-- total == 8128
-- C:brixArenaRuntimeReservedBytes() == reserved-before
```

```output
true
true
```

### Publishing a finite stream closes the broadcaster once its subscribers get every item

```yumi
. gate SyncBufferedChannel'Bool
drain(gate, 2)
drain(gate, 0)
for n in publisher([3, 4])
    -- n
-- "list done"

━━━━━━━━━━━━━━━━━━━━
+ do drain
    - gate SyncBufferedChannel'Bool
    - count Int

. pub = publisher(gated(gate, count))
. reader = pub as Iterable'Int
reader.begin-iterating()
gate <- true
. item = <- reader
while item
    -- item
    item = <- reader
-- pub.is-closed

━━━━━━━━━━━━━━━━━━━━
+ do gated
    - gate SyncBufferedChannel'Bool
    - count Int

<- gate
for n in 0 .. < count
    <= n + 1
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

```yumi
. total = 0
for n in 1 .. 300
    total += await'Int(=> n) ?? 0
-- total
```

```output
45150
```

### A write waiting on a SyncChannel when it closes reaches the next reader

```yumi
. numbers SyncChannel'Int
do new
    numbers <- 1
    -- "written"
yield-once()
numbers.close()
for n in numbers
    -- "read $n"
-- "end"

━━━━━━━━━━━━━━━━━━━━
+ do yield-once

. done = Semaphore()
do new
    done.post()
done.wait()
```

```output
read 1
end
written
```

### A reader of a SyncBufferedChannel cancelled while it waits leaves the values written before it ends to other readers

```yumi
. numbers SyncBufferedChannel'Int
. reader = do new
    for n in numbers
        -- "reader took $n"
yield-once()
reader.cancel-later()
numbers <- 1
numbers <- 2
reader.wait()
numbers.close()
for n in numbers
    -- "left $n"

━━━━━━━━━━━━━━━━━━━━
+ do yield-once

. done = Semaphore()
do new
    done.post()
done.wait()
```

```output
left 1
left 2
```

### Reads drain what an AsyncBufferedChannel buffered before it closed, and later writes are dropped

```yumi
. numbers = AsyncBufferedChannel'Int()
numbers <- 1
numbers <- 2
numbers.close()
numbers <- 3
-- <- numbers
-- <- numbers
. rest = <- numbers
-- if !rest then "drained" else "left $rest"
```

```output
1
2
drained
```

### An AsyncBufferedChannel releases a message once it is read, past a wrap, and keeps one still buffered

```yumi
/core/gc

/BxGC/BxGC.h

. channel = AsyncBufferedChannel'Node()
. watched = read-past-wrap(channel)
C:brixCollect()
-- watched[0].get == null
-- watched[1].get?.value
channel.close()
-- (<- channel)?.value
-- if !(<- channel) then "drained" else "left"

━━━━━━━━━━━━━━━━━━━━
+ do read-past-wrap
    - channel AsyncBufferedChannel'Node

for i in 0 .. < 16
    channel <- Node(i)
for i in 0 .. < 15
    <- channel
. waiting = Node(17)
channel <- Node(16)
channel <- waiting
<- channel
. delivered = <- channel
= (Weak(delivered!), Weak(waiting))

━━━━━━━━━━━━━━━━━━━━
+ Node

+ value = 0

────────────────────
+ This
    - v Int

value = v
```

```output
true
17
17
drained
```

### An event reached through an optional chain takes a write when the chain holds a value, and skips it when it is null

```yumi
. holder = PingHolder()
holder.pinger = Pinger()
for value on holder.pinger!.pinged
    -- "pinged $value"
holder.pinger?.pinged <- 3
holder.pinger = null
holder.pinger?.pinged <- 4
-- "done"

━━━━━━━━━━━━━━━━━━━━
+ Pinger

+ event pinged Int

━━━━━━━━━━━━━━━━━━━━
+ PingHolder

+ pinger Pinger?
```

```output
pinged 3
done
```
