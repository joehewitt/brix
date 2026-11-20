# Collector

### Workers that collect at the same time each return

```brix
(import /core/array-builder)
(import /BxGC/BxGC.h)
(blank)

(main
  (let builder (call (of ArrayBuilder String)))
  (for (in i (..< 0 20000))
    (call (. builder append) (str "live-" i))
  )
  (let live (call (. builder finish-immutable)))
  (let target (+ (call C:brixGCCollectionCount) 300))
  (let worker (call Worker 2))
  (let done (of SyncChannel Bool))
  (blank)
  (do-new (worker: worker)
    (inline (call collect-until target done))
  )
  (do-new (worker: worker)
    (inline (call collect-until target done))
  )
  (blank)
  (<- done)
  (<- done)
  (call (. worker stop))
  (print (. live length))
)

(fn-do + collect-until _
  (param target U64)
  (param done (of SyncChannel Bool))
  (while (< (call C:brixGCCollectionCount) target)
    (call C:brixGCCollect)
  )
  (<- done true)
)
```

```output
20000
```
