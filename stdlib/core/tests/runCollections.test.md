# Collections

```brix
(main
  (call notify-collection-demo (call side-effect))
)

(fn-do + notify-collection-demo _
  (param text String)
  (print text)
)

(fn-do + side-effect String
  (print "arg")
  (return "called")
)
```

```output
arg
called
```

```brix
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (let items (call (of List String) (list "a")))
  (let calls 0)
  (blank)
  (<> Box (id: box)
    (for (in item #items)
      (<> TextBox (value: item))
    )
  )
  (blank)
  (do @feature-collection-observers
    (= calls 1)
  )
  (print calls)
)
```

```output
1
```

```brix
(import /core)
(import /core/collection)
(import /core/tests/support/markup-fixtures)
(blank)
(prop _ warmed _ (= (call warm-list-mutation)))
(blank)

(main
  (<> Demo (id: demo)
    (for (in item (. demo #items))
      (<> TextBox (value: item))
    )
  )
  (blank)
  (let recorder (call Recorder))
  (call observe-collection (. demo items) (. recorder record))
  (++= (. demo items) (call (of List String) (list "a" "b" "c")))
  (call (. demo items insert) (call (of List String) (list "x" "y")) 1)
  (= ([] (. demo items) 0) "z")
  (call (. demo items replace) 0 "r")
  (call (. demo items remove) 2 2)
  (for (in mutation (. recorder seen))
    (print mutation)
  )
  (call unobserve-collection (. demo items) (. recorder record))
)

(type + Recorder
  (prop + seen (list String))
  (fn-do + record Void
    (param mutation CollectionMutation)
    (+= seen (str (. mutation kind) ":" (. mutation index) ":" (. mutation from-index) ":" (. mutation to-index) ":" (. mutation count)))
  )
)

(fn-do + warm-list-mutation Size
  (let items (call (of List String)))
  (+= items "warm")
  (return (. items length))
)

(type + Demo (is Box)
  (prop + items (list String))
)
```

```output
append:0:0:0:3
insert:1:0:0:2
set:0:0:0:1
replace:0:0:0:1
remove:2:0:0:2
```

```brix
(import /core)
(import /core/collection)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo)
    (for (in item (. demo #items))
      (<> TextBox (value: item))
    )
  )
  (blank)
  (let recorder (call Recorder))
  (call observe-collection (. demo items) (. recorder record))
  (call (. demo items clear))
  (for (in mutation (. recorder seen))
    (print mutation)
  )
  (call unobserve-collection (. demo items) (. recorder record))
)

(type + Recorder
  (prop + seen (list String))
  (fn-do + record Void
    (param mutation CollectionMutation)
    (+= seen (str (. mutation kind) ":" (. mutation index) ":" (. mutation from-index) ":" (. mutation to-index) ":" (. mutation count)))
  )
)

(type + Demo (is Box)
  (prop + items (list String))
  (fn-do + This _
    (= items (call (of List String) (list "a" "b" "c")))
  )
)
```

```output
remove:0:0:0:3
```

```brix
(import /core)
(import /core/collection)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo)
    (for (in item (. demo #items))
      (<> TextBox (value: item))
    )
  )
  (blank)
  (let recorder (call Recorder (. demo items)))
  (call observe-collection (. demo items) (!! (. recorder first-observer)))
  (call observe-collection (. demo items) (!! (. recorder second-observer)))
  (call observe-collection (. demo items) (!! (. recorder third-observer)))
  (+= (. demo items) "c")
  (+= (. demo items) "d")
  (for (in call (. recorder seen))
    (print call)
  )
  (call unobserve-collection (. demo items) (!! (. recorder second-observer)))
  (call unobserve-collection (. demo items) (!! (. recorder third-observer)))
)

(type + Recorder
  (prop + items (list String))
  (prop + seen (list String))
  (prop _ first-observer (? CollectionObserver))
  (prop _ second-observer (? CollectionObserver))
  (prop _ third-observer (? CollectionObserver))
  (fn-do + This _
    (param items (list String))
    (= first-observer (as (. this first) CollectionObserver))
    (= second-observer (as (. this second) CollectionObserver))
    (= third-observer (as (. this third) CollectionObserver))
  )
  (fn-do + first Void
    (param mutation CollectionMutation)
    (+= seen "first")
    (call unobserve-collection items (!! first-observer))
  )
  (fn-do + second Void
    (param mutation CollectionMutation)
    (+= seen "second")
  )
  (fn-do + third Void
    (param mutation CollectionMutation)
    (+= seen "third")
  )
)

(type + Demo (is Box)
  (prop + items (list String))
  (fn-do + This _
    (= items (call (of List String) (list "a" "b")))
  )
)
```

```output
first
second
third
second
third
```

```brix
(import /core)
(import /core/collection)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo)
    (for (in item (. demo #items))
      (<> TextBox (value: item))
    )
  )
  (blank)
  (let recorder (call Recorder (. demo items)))
  (call observe-collection (. demo items) (!! (. recorder first-observer)))
  (call observe-collection (. demo items) (!! (. recorder second-observer)))
  (call observe-collection (. demo items) (!! (. recorder third-observer)))
  (+= (. demo items) "c")
  (+= (. demo items) "d")
  (for (in call (. recorder seen))
    (print call)
  )
  (call unobserve-collection (. demo items) (!! (. recorder first-observer)))
  (call unobserve-collection (. demo items) (!! (. recorder third-observer)))
  (call unobserve-collection (. demo items) (!! (. recorder fourth-observer)))
)

(type + Recorder
  (prop + items (list String))
  (prop + seen (list String))
  (prop + did-mutate _ (= false))
  (prop _ first-observer (? CollectionObserver))
  (prop _ second-observer (? CollectionObserver))
  (prop _ third-observer (? CollectionObserver))
  (prop _ fourth-observer (? CollectionObserver))
  (fn-do + This _
    (param items (list String))
    (= first-observer (as (. this first) CollectionObserver))
    (= second-observer (as (. this second) CollectionObserver))
    (= third-observer (as (. this third) CollectionObserver))
    (= fourth-observer (as (. this fourth) CollectionObserver))
  )
  (fn-do + first Void
    (param mutation CollectionMutation)
    (+= seen "first")
    (if (not did-mutate)
      (= did-mutate true)
      (call unobserve-collection items (!! second-observer))
      (call observe-collection items (!! fourth-observer))
    )
  )
  (fn-do + second Void
    (param mutation CollectionMutation)
    (+= seen "second")
  )
  (fn-do + third Void
    (param mutation CollectionMutation)
    (+= seen "third")
  )
  (fn-do + fourth Void
    (param mutation CollectionMutation)
    (+= seen "fourth")
  )
)

(type + Demo (is Box)
  (prop + items (list String))
  (fn-do + This _
    (= items (call (of List String) (list "a" "b")))
  )
)
```

```output
first
third
first
third
fourth
```

```brix
(import /core)
(import /core/collection)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Demo (id: demo)
    (for (in item (. demo #items))
      (<> TextBox (value: item))
    )
  )
  (blank)
  (let recorder (call Recorder (. demo items)))
  (call observe-collection (. demo items) (!! (. recorder first-observer)))
  (call observe-collection (. demo items) (!! (. recorder second-observer)))
  (call observe-collection (. demo items) (!! (. recorder third-observer)))
  (+= (. demo items) "c")
  (+= (. demo items) "d")
  (for (in call (. recorder seen))
    (print call)
  )
  (call unobserve-collection (. demo items) (!! (. recorder first-observer)))
  (call unobserve-collection (. demo items) (!! (. recorder second-observer)))
  (call unobserve-collection (. demo items) (!! (. recorder fourth-observer)))
)

(type + Recorder
  (prop + items (list String))
  (prop + seen (list String))
  (prop + did-mutate _ (= false))
  (prop _ first-observer (? CollectionObserver))
  (prop _ second-observer (? CollectionObserver))
  (prop _ third-observer (? CollectionObserver))
  (prop _ fourth-observer (? CollectionObserver))
  (fn-do + This _
    (param items (list String))
    (= first-observer (as (. this first) CollectionObserver))
    (= second-observer (as (. this second) CollectionObserver))
    (= third-observer (as (. this third) CollectionObserver))
    (= fourth-observer (as (. this fourth) CollectionObserver))
  )
  (fn-do + first Void
    (param mutation CollectionMutation)
    (+= seen "first")
    (if (not did-mutate)
      (= did-mutate true)
      (call unobserve-collection items (!! third-observer))
      (call observe-collection items (!! fourth-observer))
    )
  )
  (fn-do + second Void
    (param mutation CollectionMutation)
    (+= seen "second")
  )
  (fn-do + third Void
    (param mutation CollectionMutation)
    (+= seen "third")
  )
  (fn-do + fourth Void
    (param mutation CollectionMutation)
    (+= seen "fourth")
  )
)

(type + Demo (is Box)
  (prop + items (list String))
  (fn-do + This _
    (= items (call (of List String) (list "a" "b")))
  )
)
```

```output
first
second
first
second
fourth
```

### Collection observer active count cools when the observed collection is collected.

```brix
(import /core)
(import /core/collection)
(import /BxGC/BxGC.h)
(blank)

(main
  (call C:brixGCCollect)
  (do
    (inline (call C:brixGCDrainCleanups))
  )
  (let observer-lane (call C:brixGCCollectionObserversSidePayloadLane))
  (let payloads-before (call C:brixGCObjectSidePayloadCount observer-lane))
  (call make-observed-list)
  (print (call C:brixCollectionObserverActiveCount))
  (print (- (call C:brixGCObjectSidePayloadCount observer-lane) payloads-before))
  (call C:brixGCCollect)
  (do
    (inline (call C:brixGCDrainCleanups))
  )
  (print (call C:brixCollectionObserverActiveCount))
  (print (- (call C:brixGCObjectSidePayloadCount observer-lane) payloads-before))
)

(fn-do + make-observed-list Void
  (let items (call (of List String) (list "a")))
  (call observe-collection items ignore-mutation)
)

(fn-do + ignore-mutation _
  (param mutation CollectionMutation)
)
```

```output
1
1
0
0
```

### ArrayBuilder detaches finished storage, supports rollback, and leaves ImmutableArray immutable.

```brix
(import /core)
(import /core/array-builder)
(blank)

(main
  (print (. (call (of ImmutableArray I64)) length))
  (let array (call (of Array I64) (list 5i64 6i64)))
  (let array-view (. array immutable))
  (print ([] array-view 0))
  (= ([] array 0) 7i64)
  (print ([] array-view 0))
  (let builder (call (of ArrayBuilder I64)))
  (call (. builder reserve) 8)
  (call (. builder append) 1)
  (call (. builder append-all) (call (of List I64) (list 2 2)))
  (let mark (. builder mark))
  (call (. builder append) 3)
  (print (. builder length))
  (call (. builder rewind) mark)
  (let sequence (call (. builder finish-immutable)))
  (print (. sequence length))
  (print ([] sequence 0))
  (print ([] sequence (. Position last)))
  (print ([] sequence (..< 1 3)))
  (print (has sequence 2))
  (print (== sequence ([] sequence (..< 0 (. sequence length)))))
  (blank)
  (call (. builder append) 9)
  (print sequence)
  (print (call (. builder finish-immutable)))
  (print (. (call (. builder finish-immutable)) length))
  (blank)
  (let words (call (of ArrayBuilder String)))
  (call (. words append) "kept")
  (let words-mark (. words mark))
  (call (. words append) "discarded")
  (call (. words rewind) words-mark)
  (print (call (. words finish-immutable)))
  (call (. words append) "cleared")
  (call (. words clear))
  (print (. (call (. words finish-immutable)) length))
  (blank)
  (let mutable (list I64) sequence)
  (+= mutable 4)
  (print sequence)
  (print mutable)
  (let pair-builder (call (of ArrayBuilder (of KeyValuePair String I64))))
  (call (. pair-builder append) (call KeyValuePair "answer" 42i64))
  (let mapped (as (call (. pair-builder finish-immutable)) (map String I64)))
  (print ([] mapped "answer"))
)
```

```output
0
5
7
4
3
1
2
2, 2
true
true
1, 2, 2
9
0
kept
0
1, 2, 2
1, 2, 2, 4
42
```

### ArrayBuilder canonicalizes empty finishes and finish-compacted detaches exact logical content.

```brix
(import /core)
(import /core/arena)
(import /core/array-builder)
(blank)

(main
  (let builder (call (of ArrayBuilder String)))
  (call (. builder reserve) 64)
  (call (. builder append) "alpha")
  (call (. builder append) "beta")
  (let compacted (call (. builder finish-immutable-compacted)))
  (call (. builder append) "reused")
  (let reused (call (. builder finish-immutable-compacted)))
  (let empty-builder (call (of ArrayBuilder String)))
  (call (. empty-builder reserve) 64)
  (let empty (call (. empty-builder finish-immutable)))
  (let arena (call Arena))
  (let arena-builder (call (of ArrayBuilder U32) arena))
  (call (. arena-builder reserve) 32)
  (call (. arena-builder append) 7u32)
  (let arena-compacted (call (. arena-builder finish-immutable-compacted)))
  (print compacted)
  (print reused)
  (print compacted)
  (print (. empty length))
  (print (. (call (. empty-builder finish-immutable-compacted)) length))
  (print arena-compacted)
  (call (. arena release))
)
```

```output
alpha, beta
reused
alpha, beta
0
0
7
```

### ArrayBuilder uses its provider's Array backing and finished views survive builder reuse.

```brix
(import /core)
(import /core/arena)
(import /core/array-builder)
(blank)

(main
  (let arena (call Arena))
  (let builder (call (of ArrayBuilder U32) arena))
  (call (. builder append) 10u32)
  (call (. builder append) 20u32)
  (let first (call (. builder finish-immutable)))
  (call (. builder append) 30u32)
  (let second (call (. builder finish-immutable)))
  (let default-builder (call (of ArrayBuilder U32)))
  (call (. default-builder append) 10u32)
  (call (. default-builder append) 20u32)
  (print first)
  (print second)
  (print ([] first 1))
  (print (== first (call (. default-builder finish-immutable))))
  (call (. arena release))
)
```

```output
10, 20
30
20
true
```

```brix
(import /core/arena)
(import /core/array-builder)
(import /BxGC/BxGC.h)
(blank)

(main
  (let arena (call Arena))
  (let builder (call (of ArrayBuilder ArenaNode) arena))
  (for (in index (..< 0 8))
    (call (. builder append) (call ArenaNode index))
    (call churn-managed-nodes index)
    (call C:brixGCCollect)
  )
  (print (. ([] builder 7) value))
  (let nodes (call (. builder finish-immutable)))
  (call churn-managed-nodes 100)
  (call C:brixGCCollect)
  (print (tuple (. ([] nodes 0) value) (. ([] nodes 7) value)))
  (call (. arena release))
)

(type + ArenaNode
  (prop + value Size)
  (fn-do + This _
    (param value _)
  )
)

(fn-do + churn-managed-nodes _
  (param seed Size)
  (let garbage (list ArenaNode))
  (for (in offset (..< 0 256))
    (+= garbage (call ArenaNode (+ seed offset)))
  )
)
```

```output
7
(0, 7)
```

### GC-backed ImmutableArray storage traces pointerful elements after the builder is gone.

```brix
(import /core/array-builder)
(import /BxGC/BxGC.h)
(blank)

(main
  (let items (call make-sequence-gc-items))
  (call C:brixGCCollect)
  (print (. items length))
  (print (. ([] items 0) value))
)

(type + SequenceGCItem
  (prop + value String)
  (fn-do + This _
    (param value _)
  )
)

(fn-do + make-sequence-gc-items (of ImmutableArray SequenceGCItem)
  (let builder (call (of ArrayBuilder SequenceGCItem)))
  (call (. builder append) (call SequenceGCItem "kept"))
  (return (call (. builder finish-immutable)))
)
```

```output
1
kept
```

### Managed objects and their collections do not retain or release Arena identities.

```brix
(import /core/arena)
(blank)

(main
  (let first (call Arena))
  (let second (call Arena))
  (let first-pointer (call (of (. first allocate) U32) 1))
  (= ([] first-pointer 0) 30u32)
  (let second-pointer (call (of (. second allocate) U32) 1))
  (= ([] second-pointer 0) 40u32)
  (let holder (call ArenaCollectionHolder (call (of List Arena) (list first second)) (list first-pointer second-pointer)))
  (blank)
  (call (. holder arenas clear))
  (call (. holder pointers clear))
  (print ([] first-pointer 0))
  (print ([] second-pointer 0))
  (blank)
  (+= (. holder arenas) first)
  (+= (. holder pointers) first-pointer)
  (= ([] (. holder arenas) 0) second)
  (= ([] (. holder pointers) 0) second-pointer)
  (let first-next (call (of (. first allocate) U32) 1))
  (= ([] first-next 0) 31u32)
  (print ([] first-next 0))
  (let second-next (call (of (. second allocate) U32) 1))
  (= ([] second-next 0) 41u32)
  (print ([] second-next 0))
  (blank)
  (call (. first release))
  (call (. second release))
)

(type + ArenaCollectionHolder
  (prop + arenas (list Arena))
  (prop + pointers (list (of CPointer U32)))
  (fn-do + This _
    (param arenas _)
    (param pointers _)
  )
)
```

```output
30
40
31
41
```

### Explicit collection observer detach deactivates once before later owner reclamation.

```brix
(import /core)
(import /core/collection)
(import /BxGC/BxGC.h)
(blank)

(main
  (call C:brixGCCollect)
  (do
    (inline (call C:brixGCDrainCleanups))
  )
  (let active-before (call C:brixCollectionObserverActiveCount))
  (call make-explicitly-detached-list)
  (print (- (call C:brixCollectionObserverActiveCount) active-before))
  (call C:brixGCCollect)
  (do
    (inline (call C:brixGCDrainCleanups))
  )
  (print (- (call C:brixCollectionObserverActiveCount) active-before))
)

(fn-do + make-explicitly-detached-list Void
  (let items (call (of List String) (list "a")))
  (call observe-collection items ignore-mutation)
  (call unobserve-collection items ignore-mutation)
)

(fn-do + ignore-mutation _
  (param mutation CollectionMutation)
)
```

```output
0
0
```

### Arena ArrayBuilder storage checkpoints restore prior buffers without retaining speculative storage.

```brix
(import /core/arena)
(import /core/array-builder)
(blank)

(main
  (let arena (call Arena 8192))
  (finally
    (inline (call (. arena release)))
  )
  (let builder (call (of ArrayBuilder String) arena))
  (let alias builder)
  (call (. builder append) (call (. arena copy-string) "kept"))
  (let arena-mark (. arena mark))
  (let checkpoint (call (. builder storage-checkpoint) arena arena-mark))
  (call (. builder reserve) 64)
  (call (. builder append) (call (. arena copy-string) "discarded"))
  (call (. builder validate-storage-restore) checkpoint arena arena-mark)
  (call (. arena rewind) arena-mark)
  (call (. builder restore-storage) checkpoint arena arena-mark)
  (call (. builder append) (call (. arena copy-string) "regrown"))
  (let shared-length (. alias length))
  (let shared-first ([] alias 0))
  (let length-after-index (. builder length))
  (let restored (call (. builder finish-immutable)))
  (print (str shared-length "/" shared-first "/" length-after-index "/" (. restored length) "/" ([] restored 0) "/" ([] restored 1)))
  (blank)
  (let empty (call (of ArrayBuilder String) arena))
  (let empty-mark (. arena mark))
  (let empty-checkpoint (call (. empty storage-checkpoint) arena empty-mark))
  (let wrapped-checkpoint (? RestoreCheckpoint) (call RestoreCheckpoint empty-mark empty-checkpoint))
  (call (. empty append) (call (. arena copy-string) "discarded"))
  (call (. empty validate-storage-restore) (. (!! wrapped-checkpoint) side) arena (. (!! wrapped-checkpoint) arena))
  (call (. arena rewind) (. (!! wrapped-checkpoint) arena))
  (call (. empty restore-storage) (. (!! wrapped-checkpoint) side) arena (. (!! wrapped-checkpoint) arena))
  (call (. empty append) (call (. arena copy-string) "regrown"))
  (print (str (. empty length) "/" ([] empty 0)))
)

(type + RestoreCheckpoint (is Tuple)
  (prop + arena ArenaMark)
  (prop + side (of ArrayBuilderStorageCheckpoint String ArenaBacking))
)
```

```output
2/kept/2/2/kept/regrown
1/regrown
```

### ArrayBuilder checkpoint validation rejects foreign and destructively mutated builders before Arena rewind.

```brix
(import /core/arena)
(import /core/array-builder)
(blank)

(main
  (let arena (call Arena 8192))
  (finally
    (inline (call (. arena release)))
  )
  (let first (call (of ArrayBuilder U32) arena))
  (let second (call (of ArrayBuilder U32) arena))
  (call (. first append) 1u32)
  (call (. second append) 2u32)
  (let arena-mark (. arena mark))
  (let checkpoint (call (. first storage-checkpoint) arena arena-mark))
  (let retained (call (of (. arena allocate) U64) 1))
  (= ([] retained 0) 77u64)
  (blank)
  (call show-rejected-storage-restore second checkpoint arena arena-mark)
  (call (. first clear))
  (call show-rejected-storage-restore first checkpoint arena arena-mark)
  (call (. first append) 3u32)
  (let first-values (call (. first finish-immutable)))
  (let second-values (call (. second finish-immutable)))
  (print (str ([] first-values 0) "/" ([] second-values 0) "/" ([] retained 0)))
)

(fn-do + show-rejected-storage-restore _
  (param builder (of ArrayBuilder U32 ArenaBacking))
  (param checkpoint (of ArrayBuilderStorageCheckpoint U32 ArenaBacking))
  (param arena Arena)
  (param mark ArenaMark)
  (let attempt
    (do-new
      (call (. builder validate-storage-restore) checkpoint arena mark)
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)
```

```output
ArrayBuilder storage checkpoint is stale or belongs to another builder
ArrayBuilder storage checkpoint is stale or belongs to another builder
3/2/77
```

```brix
(import /core/array-builder)
(blank)

(main
  (let builder (call (of ArrayBuilder I32)))
  (call (. builder reserve) 8)
  (call (. builder append) 1)
  (call (. builder append) 2)
  (let mutable (call (. builder finish-mutable)))
  (print (. builder length))
  (let immutable (. mutable immutable))
  (let readable (of Indexable I32) mutable)
  (print (is readable (of ImmutableArray I32)))
  (print (is readable (of Array I32)))
  (let read-only (of Indexable I32) immutable)
  (print (is read-only (of ImmutableArray I32)))
  (print (is read-only (of Array I32)))
  (let recovered (as readable (of Array I32)))
  (print ([] recovered 1))
  (call reject-immutable readable)
  (print ([] readable 1))
  (= ([] mutable 0) 9)
  (print ([] immutable 0))
  (print ([] ([] immutable (..< 1 2)) 0))
  (call (. builder append) 3)
  (let detached (call (. builder finish-immutable)))
  (print (. builder length))
  (print ([] detached 0))
  (print ([] mutable 0))
  (call (. builder append) 4)
  (let compacted (call (. builder finish-mutable-compacted)))
  (= ([] compacted 0) 5)
  (print ([] compacted 0))
  (print (. builder length))
  (print (. (call (. builder finish-mutable)) length))
  (let total 0)
  (for (in item immutable)
    (+= total item)
  )
  (for (in item mutable)
    (+= total item)
  )
  (print total)
)

(fn-do + reject-immutable _
  (param values (of Indexable I32))
  (let attempt
    (do-new
      (let wrong (as values (of ImmutableArray I32)))
      (print (. wrong length))
      (on-failure failure
        (print (!= failure null))
      )
    )
  )
  (call (. attempt wait))
)
```

```output
0
false
true
true
false
2
true
2
9
2
0
3
9
5
0
0
22
```

```brix
(import /core/arena)
(import /core/array-builder)
(import /BxGC/BxGC.h)
(blank)

(main
  (let arena (call Arena 8192))
  (finally
    (inline (call (. arena release)))
  )
  (let boxes (call (of ArrayBuilder Box) arena))
  (let pairs (call (of ArrayBuilder Pair) arena))
  (for (in index (..< 0 80))
    (call (. boxes append) (call Box (str "box " index)))
    (call (. pairs append) (call Pair (call Box (str "pair " index)) (call Box (str "tail " index))))
    (call C:brixGCCollect)
  )
  (print (. ([] boxes 0) text))
  (print (. ([] boxes 79) text))
  (print (tuple (. ([] pairs 0) left text) (. ([] pairs 79) right text)))
  (print (tuple (. boxes length) (. pairs length)))
)

(type + Box
  (prop + text String)
  (fn-do + This _
    (param text String)
  )
)

(type + Pair (is Tuple)
  (prop + left Box)
  (prop + right Box)
)
```

```output
box 0
box 79
(pair 0, tail 79)
(80, 80)
```

### Tracked reads of a list follow its mutations as well as its replacement

```brix
(main
  (let bag (call Bag))
  (let count 0)
  (= count (. bag #items size))
  (print (tuple count (. bag size)))
  (+= (. bag items) "c")
  (print (tuple count (. bag size)))
  (= (. bag items) (call (of List String) (list "x")))
  (print (tuple count (. bag size)))
  (+= (. bag items) "y")
  (print (tuple count (. bag size)))
)

(type + Bag
  (prop + items _ (= (call (of List String) (list "a" "b"))))
  (prop + size _ (= (. #items size)))
)
```

```output
(2, 2)
(3, 3)
(1, 1)
(2, 2)
```

### An observer attached directly sees list mutations though nothing reads the list tracked

```brix
(import /core)
(import /core/collection)
(blank)

(main
  (let items (call (of List String)))
  (let recorder (call Recorder))
  (call observe-collection items (. recorder record))
  (+= items "a")
  (call (. items insert) "b" 0)
  (= ([] items 1) "c")
  (call (. items remove) 0)
  (for (in mutation (. recorder seen))
    (print mutation)
  )
  (call unobserve-collection items (. recorder record))
)

(type + Recorder
  (prop + seen (list String))
  (fn-do + record Void
    (param mutation CollectionMutation)
    (+= seen (str (. mutation kind) ":" (. mutation index) ":" (. mutation count)))
  )
)
```

```output
append:0:1
insert:0:1
set:1:1
remove:0:1
```

### Removing a map's entry and rewinding an ArrayBuilder make no value of the type they hold, which a field may give none of.

```brix
(import /core/arena)
(import /core/array-builder)
(import /core/table)
(blank)

(main
  (let arena (call Arena))
  (let held (map String Holder))
  (= ([] held "a") (call Holder (call (. (call (of Table Point) arena) finish))))
  (= ([] held "b") (call Holder (call (. (call (of Table Point) arena) finish))))
  (-= ([] held "a"))
  (print (str (. held length) " " (== ([] held "a") null) " " (. (!! ([] held "b")) points length)))
  (let built (call (of ArrayBuilder Holder)))
  (call (. built append) (!! ([] held "b")))
  (call (. built clear))
  (call (. built append) (!! ([] held "b")))
  (print (. built length))
)

(type + Point (is Struct)
  (prop + x I32)
)

(type + Holder
  (prop + points (of ImmutableTable Point))
  (fn-do + This _
    (param points _)
  )
)
```

```output
1 true 0
1
```
