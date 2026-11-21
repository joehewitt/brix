# Maps

```brix
(main
  (let map (call (of Map String Int)))
  (print ([] map "foo"))
)
```

```output
unknown
```

```brix
(main
  (let present (? Int) 7)
  (let absent (? Int))
  (let property "strokeJoin")
  (let map
    (call (of Map String Int)
      (block
        (entry? "strokeWidth" present)
        (entry? "strokeCap" absent)
        (entry? property present)
      )
    )
  )
  (print (. map length))
  (print ([] map "strokeWidth"))
  (print (has map "strokeCap"))
  (print ([] map "strokeJoin"))
)
```

```output
2
7
false
7
```

```brix
(import /BxGC/BxGC.h)
(blank)

(main
  (let done (of SyncChannel String))
  (do-new
    (let prior-goal (call C:brixGCHeapGoal))
    (call C:brixGCSetHeapGoal (+ (call C:brixGCLiveByteCount) 1))
    (let property (str "strokeJoin-" (+ 41 1)))
    (let present (? String) (str "managed-" (+ 20 2)))
    (let map
      (call (of Map String String)
        (block
          (entry? "strokeWidth" present)
          (entry? property present)
        )
      )
    )
    (call C:brixGCSetHeapGoal prior-goal)
    (call C:brixCollect)
    (<- done (str (!! ([] map "strokeWidth")) "," (!! ([] map property))))
  )
  (print (<- done))
)
```

```output
managed-22,managed-22
```

```brix
(main
  (let map (call (of Map String Int)))
  (= ([] map "foo") 42)
  (print ([] map "foo"))
)
```

```output
42
```

```brix
(main
  (let map (call (of Map String Int)))
  (= ([] map "a") 1)
  (= ([] map "b") 2)
  (print ([] map "a"))
  (print ([] map "b"))
)
```

```output
1
2
```

```brix
(main
  (let map (call (of Map String Int)))
  (print (. map length))
  (= ([] map "foo") 42)
  (print (. map length))
)
```

```output
0
1
```

```brix
(main
  (let map (call (of Map String Int)))
  (= ([] map "foo") 1)
  (= ([] map "bar") 2)
  (print (. map length))
  (print ([] map "foo"))
  (print ([] map "bar"))
)
```

```output
2
1
2
```

```brix
(main
  (let map (call (of Map String Int)))
  (= ([] map "c") 3)
  (= ([] map "a") 1)
  (= ([] map "b") 2)
  (let copy (call (of Map String Int)))
  (for (in (tuple key value) map)
    (= ([] copy key) value)
  )
  (print (. copy length))
  (print ([] copy "a"))
  (print ([] copy "b"))
  (print ([] copy "c"))
)
```

```output
3
1
2
3
```

```brix
(main
  (let map (call (of Map String Int)))
  (= ([] map "a") 1)
  (= ([] map "b") 2)
  (= ([] map "c") 3)
  (-= ([] map "a"))
  (print (. map length))
  (print (has map "a"))
  (print ([] map "b"))
  (print ([] map "c"))
)
```

```output
2
false
2
3
```

```brix
(main
  (let map1 (call (of Map String Int)))
  (= ([] map1 "a") 1)
  (= ([] map1 "c") 3)
  (let map2 (call (of Map String Int)))
  (= ([] map2 "b") 2)
  (= ([] map2 "c") 4)
  (let combined (+ map1 map2))
  (print (. combined length))
  (print ([] combined "a"))
  (print ([] combined "b"))
  (print ([] combined "c"))
)
```

```output
3
1
2
4
```

```brix
(main
  (let map1 (call (of Map String Int)))
  (= ([] map1 "a") 1)
  (= ([] map1 "b") 2)
  (= ([] map1 "c") 3)
  (let map2 (call (of Map String Int)))
  (= ([] map2 "b") 2)
  (= ([] map2 "c") 4)
  (let difference (- map1 map2))
  (print (. difference length))
  (print ([] difference "a"))
  (print (has difference "b"))
  (print (has difference "c"))
)
```

```output
1
1
false
false
```

### In-place merge and subtraction with the map itself keep or empty it.

```brix
(main
  (let map (call (of Map String Int)))
  (= ([] map "a") 1)
  (= ([] map "b") 2)
  (let alias map)
  (+= map alias)
  (print (. map length))
  (print ([] alias "b"))
  (-= map alias)
  (print (. map length))
  (print (. alias length))
)
```

```output
2
2
0
0
```

```brix
(main
  (let map (call (of Map String Int)))
  (= ([] map "a") 1)
  (= ([] map "b") 2)
  (= ([] map "c") 2)
  (let found (call (of Map String Bool)))
  (for (in key (call (. map find) 2))
    (= ([] found key) true)
  )
  (print (. found length))
  (print (has found "a"))
  (print (has found "b"))
  (print (has found "c"))
)
```

```output
2
false
true
true
```

```brix
(main
  (let map (map String Int))
  (print (type-of map))
)
```

```output
[TypeInfo Map'String'I64'GCBacking]
```

```brix
(main
  (print
    (call Map
      (block
        (:baz 42)
      )
    )
  )
  (print
    (call Map
      (block
        (entry "foo" 42)
      )
    )
  )
  (print
    (call Map
      (block
        (entry "foo" 1)
        (:baz 2)
      )
    )
  )
  (print
    (call Map
      (block
        (:baz 2)
        (entry "foo" 1)
      )
    )
  )
)
```

```output
[baz = 42]
[foo = 42]
[baz = 2, foo = 1]
[baz = 2, foo = 1]
```

```brix
(main
  (print
    (call Map
      (block
        (entry "foo" 42)
        (entry "bar" x)
        (:baz 42)
        (lazy x 10)
      )
    )
  )
)
```

```output
[baz = 42, bar = 10, foo = 42]
```

### A map constructor keeps its map result when contextualized as one case of a union.

```brix
(main
  (let root (map String MapConstructorValue) (call (of Map String MapConstructorValue)))
  (= ([] root "stream_options")
    (call Map
      (block
        (entry "include_usage" true)
      )
    )
  )
  (let stream-options (as ([] root "stream_options") (map String MapConstructorValue)))
  (print (as ([] stream-options "include_usage") Bool))
)

(type + MapConstructorValue (is Union)
  (variant Bool)
  (variant (map String MapConstructorValue))
)
```

```output
true
```

### Map literals can mix identifier keys with explicit string keys.

```brix
(main
  (let content MapLiteralValue
    (call (of Map String MapLiteralValue)
      (block
        (:type "image_url")
        (entry "image_url"
          (call (of Map String MapLiteralValue)
            (block
              (:url "https://example.com/image.png")
            )
          )
        )
      )
    )
  )
  (let root (as content (map String MapLiteralValue)))
  (let image (as ([] root "image_url") (map String MapLiteralValue)))
  (print (as ([] root "type") String))
  (print (as ([] image "url") String))
)

(type + MapLiteralValue (is Union)
  (variant String)
  (variant (map String MapLiteralValue))
)
```

```output
image_url
https://example.com/image.png
```

### Optional map entries omit absent values and unwrap present values.

```brix
(main
  (let present (? String) "yes")
  (let absent (? String))
  (let map
    (call (of Map String String)
      (block
        (:type "image")
        (:present? present)
        (:absent? absent)
      )
    )
  )
  (print (has map "present"))
  (print (has map "absent"))
  (print ([] map "present"))
)
```

```output
true
false
yes
```

### Full-width numeric keys remain distinct.

```brix
(main
  (let map (call (of Map U64 Int)))
  (= ([] map 0u64) 1)
  (= ([] map 4294967296u64) 2)
  (print ([] map 0u64))
  (print ([] map 4294967296u64))
  (print (. map length))
)
```

```output
1
2
2
```

### UTF-8 prefixes and equal 32-bit string hashes do not alias keys.

```brix
(main
  (let map (call (of Map String Int)))
  (= ([] map "éa") 1)
  (= ([] map "éb") 2)
  (= ([] map "zsycr9pu60") 3)
  (= ([] map "hngah7hg7b") 4)
  (print ([] map "éa"))
  (print ([] map "éb"))
  (print ([] map "zsycr9pu60"))
  (print ([] map "hngah7hg7b"))
  (print (. map length))
)
```

```output
1
2
3
4
4
```

### Deleting a same-bucket head or tail preserves the rest of the chain.

```brix
(main
  (let delete-head (call (of Map U32 Int)))
  (= ([] delete-head 0u32) 0)
  (= ([] delete-head 8u32) 8)
  (-= ([] delete-head 0u32))
  (print (has delete-head 0u32))
  (print ([] delete-head 8u32))
  (print (. delete-head length))
  (blank)
  (let delete-tail (call (of Map U32 Int)))
  (= ([] delete-tail 0u32) 0)
  (= ([] delete-tail 8u32) 8)
  (-= ([] delete-tail 8u32))
  (print ([] delete-tail 0u32))
  (print (has delete-tail 8u32))
  (print (. delete-tail length))
)
```

```output
false
8
1
0
false
1
```

### Existing and missing deletes keep length, emptiness, and every iteration surface consistent.

```brix
(main
  (let map (call (of Map U32 Int)))
  (= ([] map 0u32) 0)
  (= ([] map 8u32) 8)
  (= ([] map 16u32) 16)
  (-= ([] map 8u32))
  (blank)
  (let key-count 0)
  (for (in key (. map keys))
    (+= key-count 1)
  )
  (let value-count 0)
  (for (in value (. map values))
    (+= value-count 1)
  )
  (let item-count 0)
  (for (in item (. map items))
    (+= item-count 1)
  )
  (let iteration-count 0)
  (for (in (tuple key value) map)
    (+= iteration-count 1)
  )
  (blank)
  (print (. map length))
  (print (not map))
  (print (has map 0u32))
  (print (has map 8u32))
  (print (has map 16u32))
  (print (tuple key-count value-count item-count iteration-count))
  (blank)
  (-= ([] map 24u32))
  (print (. map length))
  (print (has map 0u32))
  (print (has map 8u32))
  (print (has map 16u32))
  (blank)
  (-= ([] map 0u32))
  (-= ([] map 16u32))
  (print (. map length))
  (print (not map))
)
```

```output
2
false
true
false
true
(2, 2, 2, 2)
2
true
false
true
0
true
```

### Growth preserves 80,000 mixed-width keys across lookup and every iteration surface.

```brix
(main
  (let count 80000)
  (let map (call (of Map U64 U64)))
  (let expected-key-checksum 0u64)
  (let expected-value-checksum 0u64)
  (for (in i (..< 0 count))
    (let key (+ (as i U64) (* (as (% i 17) U64) 4294967296u64)))
    (= ([] map key) (as i U64))
    (+= expected-key-checksum key)
    (+= expected-value-checksum (as i U64))
  )
  (blank)
  (let lookup-ok true)
  (for (in i (..< 0 count))
    (let key (+ (as i U64) (* (as (% i 17) U64) 4294967296u64)))
    (if (!= (!! ([] map key)) (as i U64))
      (= lookup-ok false)
    )
  )
  (blank)
  (let key-count 0)
  (let key-checksum 0u64)
  (for (in key (. map keys))
    (+= key-count 1)
    (+= key-checksum key)
  )
  (blank)
  (let value-count 0)
  (let value-checksum 0u64)
  (for (in value (. map values))
    (+= value-count 1)
    (+= value-checksum value)
  )
  (blank)
  (let item-count 0)
  (let item-key-checksum 0u64)
  (let item-value-checksum 0u64)
  (for (in item (. map items))
    (+= item-count 1)
    (+= item-key-checksum (. item key))
    (+= item-value-checksum (. item value))
  )
  (blank)
  (let iteration-count 0)
  (let iteration-key-checksum 0u64)
  (let iteration-value-checksum 0u64)
  (for (in (tuple key value) map)
    (+= iteration-count 1)
    (+= iteration-key-checksum key)
    (+= iteration-value-checksum value)
  )
  (blank)
  (print (. map length))
  (print lookup-ok)
  (print (tuple key-count value-count item-count iteration-count))
  (print (== key-checksum expected-key-checksum))
  (print (== value-checksum expected-value-checksum))
  (print (== item-key-checksum expected-key-checksum))
  (print (== item-value-checksum expected-value-checksum))
  (print (== iteration-key-checksum expected-key-checksum))
  (print (== iteration-value-checksum expected-value-checksum))
)
```

```output
80000
true
(80000, 80000, 80000, 80000)
true
true
true
true
true
true
```

### Membership is based on key presence rather than value truthiness.

```brix
(main
  (let map (call (of Map String Bool)))
  (= ([] map "f") false)
  (print (has map "f"))
  (print (has map "missing"))
)
```

```output
true
false
```

### Plain reference keys use stable object identity.

```brix
(main
  (let first (call Thing))
  (let second (call Thing))
  (let map (call (of Map Thing String)))
  (= ([] map first) "first")
  (= ([] map second) "second")
  (print ([] map first))
  (print ([] map second))
  (print (. map length))
)

(type + Thing)
```

```output
first
second
2
```

### Open-addressed capacity uses a 75% occupancy limit; reserve and clear retain storage.

```brix
(main
  (let map (call (of Map U32 U32)))
  (print (. map capacity))
  (for (in key (..< 0u32 6u32))
    (= ([] map key) key)
  )
  (print (. map capacity))
  (= ([] map 6u32) 6u32)
  (print (. map capacity))
  (let length-before-update (. map length))
  (= ([] map 3u32) 33u32)
  (print (== (. map length) length-before-update))
  (print ([] map 3u32))
  (call (. map reserve) 24)
  (print (. map capacity))
  (call (. map reserve) 25)
  (print (. map capacity))
  (call (. map clear))
  (print (. map length))
  (print (. map capacity))
)
```

```output
8
8
32
true
33
32
128
0
128
```

### Reinserting a removed key reuses its tombstone instead of rebuilding a map at its occupancy limit.

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (blank)
  (let map (call (of Map U32 U32) arena))
  (for (in key (..< 0u32 6u32))
    (= ([] map key) (* key 10u32))
  )
  (let active-bytes (. arena active-bytes))
  (for (in round (..< 0 4))
    (-= ([] map 3u32))
    (= ([] map 3u32) 30u32)
  )
  (print (== (. arena active-bytes) active-bytes))
  (print (tuple (. map length) (. map capacity)))
  (print (tuple ([] map 0u32) ([] map 3u32) ([] map 5u32)))
)
```

```output
true
(6, 8)
(0, 30, 50)
```

### Clearing a map with live entries and tombstones releases its values and keeps it usable.

```brix
(import /core/gc)
(blank)
(import /BxGC/BxGC.h)
(blank)

(main
  (let map (call (of Map U32 MapClearBox)))
  (let weaks (call fill-cleared map))
  (call (. map clear))
  (call C:brixCollect)
  (print (. map length))
  (let live 0)
  (for (in weak weaks)
    (if (. weak get)
      (+= live 1)
    )
  )
  (print live)
  (= ([] map 2u32) (call MapClearBox 20))
  (print (. (!! ([] map 2u32)) value))
  (print (== ([] map 1u32) null))
)

(fn-do + fill-cleared (of List (of Weak MapClearBox))
  (param map (of Map U32 MapClearBox))
  (let weaks (call (of List (of Weak MapClearBox))))
  (for (in index (..< 0u32 4u32))
    (let box (call MapClearBox (as index Int)))
    (= ([] map index) box)
    (call (. weaks append) (call Weak box))
  )
  (-= ([] map 1u32))
  (return weaks)
)

(type + MapClearBox
  (prop + value _ (= 0))
  (fn-do + This _
    (param v Int)
    (= value v)
  )
)
```

```output
0
0
20
true
```

### A map value deleted or cleared during a mark survives that cycle.

```brix
(import /BxGC/BxGCTest.h)
(blank)

(main
  (let deleted-from (call filled-map 9))
  (call C:brixGCStartConcurrentMark)
  (let deleted (!! ([] deleted-from 1u32)))
  (-= ([] deleted-from 1u32))
  (call C:brixGCFinishConcurrentMark)
  (call C:brixCollect)
  (print (call survivor deleted))
  (let cleared-from (call filled-map 10))
  (call C:brixGCStartConcurrentMark)
  (let cleared (!! ([] cleared-from 1u32)))
  (call (. cleared-from clear))
  (call C:brixGCFinishConcurrentMark)
  (call C:brixCollect)
  (print (call survivor cleared))
)

(fn-do + filled-map (of Map U32 MapClearBox)
  (param value Int)
  (let map (call (of Map U32 MapClearBox)))
  (= ([] map 1u32) (call MapClearBox value))
  (return map)
)

(fn-do + survivor String
  (param box MapClearBox)
  (return (if-expr (call C:brixGCVerifyObject (as box Pointer)) (str "alive " (. box value)) "lost"))
)

(type + MapClearBox
  (prop + value _ (= 0))
  (fn-do + This _
    (param v Int)
    (= value v)
  )
)
```

```output
alive 9
alive 10
```

### Fully colliding keys survive deletion, tombstone reuse, and repeated churn.

```brix
(main
  (let map (call (of Map MapCollisionKey U32)))
  (for (in round (..< 0 12))
    (for (in index (..< 0 96))
      (= ([] map (call MapCollisionKey (as index U32))) (as (+ index (* round 1000)) U32))
    )
    (for (in index (..< 0 96 (by: 3)))
      (-= ([] map (call MapCollisionKey (as index U32))))
    )
    (for (in index (..< 0 96 (by: 3)))
      (= ([] map (call MapCollisionKey (as index U32))) (as (+ index (* round 1000) 1) U32))
    )
  )
  (blank)
  (let valid (== (. map length) 96))
  (for (in index (..< 0 96))
    (let expected (as (+ index 11000) U32))
    (if (== (% index 3) 0)
      (+= expected 1u32)
    )
    (if (!= (!! ([] map (call MapCollisionKey (as index U32)))) expected)
      (= valid false)
    )
  )
  (blank)
  (for (in index (..< 0 96))
    (-= ([] map (call MapCollisionKey (as index U32))))
  )
  (print valid)
  (print (. map length))
  (print (has map (call MapCollisionKey 500u32)))
)

(type + MapCollisionKey (is Struct)
  (implements Hashable)
  (prop + identity U32)
  (fn-expr + (infix this == (param other MapCollisionKey)) _
    (== identity (. other identity))
  )
  (fn-expr + hash Hash :get
    0u32
  )
)
```

```output
true
0
false
```

### Iteration visits every live entry exactly once without defining an order.

```brix
(main
  (let count 128)
  (let map (call (of Map U32 U32)))
  (for (in index (..< 0 count))
    (= ([] map (as index U32)) (as (* index 7) U32))
  )
  (blank)
  (let seen ([] Bool count))
  (let valid true)
  (let visited 0)
  (for (in (tuple key value) map)
    (if (or (>= key (as count U32)) ([] seen (as key Index)) (!= value (* key 7u32)))
      (= valid false)
      (else
        (= ([] seen (as key Index)) true)
      )
    )
    (+= visited 1)
  )
  (blank)
  (for (in index (..< 0 count))
    (if (not ([] seen index))
      (= valid false)
    )
  )
  (print valid)
  (print visited)
)
```

```output
true
128
```

### Deterministic mixed operations agree with a simple optional-array model.

```brix
(main
  (let key-count 257)
  (let map (call (of Map U32 I32)))
  (let model ([] (? I32) key-count))
  (let valid true)
  (for (in step (..< 0 20000))
    (let key (as (% (+ (* step 73) (/ step 11)) key-count) U32))
    (let action (% step 5))
    (if (< action 3)
      (let value (as (- (* step 17) 9000) I32))
      (= ([] map key) value)
      (= ([] model (as key Index)) value)
      (else-if (== action 3)
        (-= ([] map key))
        (= ([] model (as key Index)) null)
      )
      (else-if (!= ([] map key) ([] model (as key Index)))
        (= valid false)
      )
    )
  )
  (blank)
  (for (in key (..< 0 key-count))
    (if (!= ([] map (as key U32)) ([] model key))
      (= valid false)
    )
  )
  (print valid)
)
```

```output
true
```

### Optional and large reference-bearing values survive growth and deletion.

```brix
(import /BxGC/BxGC.h)
(blank)

(main
  (let optional (call (of Map U32 (of Optional String))))
  (let absent (of Optional String))
  (= ([] optional 1u32) absent)
  (= ([] optional 2u32) "present")
  (blank)
  (let references (call (of Map ReferenceMapThing ReferenceMapThing)))
  (let roots (list ReferenceMapThing))
  (for (in key (..< 0 2000))
    (let key-object (call ReferenceMapThing (as key U32)))
    (let value-object (call ReferenceMapThing (as (+ key 2000) U32)))
    (+= roots key-object)
    (+= roots value-object)
    (= ([] references key-object) value-object)
  )
  (for (in key (..< 0 1000))
    (-= ([] references ([] roots (* key 2))))
  )
  (call (. roots clear))
  (call C:brixGCCollect)
  (blank)
  (let reference-checksum 0u64)
  (for (in (tuple key value) references)
    (+= reference-checksum (as (. key id) U64))
    (+= reference-checksum (as (. value id) U64))
  )
  (blank)
  (let large (call (of Map U32 LargeMapValue)))
  (for (in key (..< 0 2000))
    (= ([] large (as key U32)) (call LargeMapValue (str "value-" key) (as key U64) (as (+ key 1) U64) (as (+ key 2) U64) (as (+ key 3) U64)))
  )
  (for (in key (..< 0 1000))
    (-= ([] large (as key U32)))
  )
  (blank)
  (print (has optional 1u32))
  (print (has optional 2u32))
  (print (. optional length))
  (print (. references length))
  (print reference-checksum)
  (print (. large length))
  (let final-value (!! ([] large 1999u32)))
  (print (. final-value label))
  (print (. final-value d))
)

(type + LargeMapValue (is Struct)
  (prop + label String)
  (prop + a U64)
  (prop + b U64)
  (prop + c U64)
  (prop + d U64)
)

(type + ReferenceMapThing
  (prop + id U32)
  (fn-do + This _
    (param id _)
  )
)
```

```output
true
true
2
1000
4999000
1000
value-1999
2002
```

### Arena-backed Maps share one storage context across all slot arrays and preserve their backing.

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (blank)
  (let map (call (of Map MapArenaCollisionKey U32) arena))
  (let reserved (call (of Map MapArenaCollisionKey U32) arena 24))
  (let from-pairs (call (of Map MapArenaCollisionKey U32) arena (call KeyValuePair (call MapArenaCollisionKey 300u32) 3u32)))
  (blank)
  (for (in index (..< 0 96))
    (= ([] map (call MapArenaCollisionKey (as index U32))) (as index U32))
  )
  (= ([] map (call MapArenaCollisionKey 5u32)) 5000u32)
  (for (in index (..< 0 96 (by: 3)))
    (-= ([] map (call MapArenaCollisionKey (as index U32))))
  )
  (for (in index (..< 0 96 (by: 3)))
    (= ([] map (call MapArenaCollisionKey (as index U32))) (as (+ index 1000) U32))
  )
  (blank)
  (let valid (== (. map length) 96))
  (let visited 0)
  (let checksum 0u64)
  (for (in (tuple key value) map)
    (+= visited 1)
    (+= checksum (as value U64))
    (if (!= (!! ([] map key)) value)
      (= valid false)
    )
  )
  (blank)
  (let extra (call (of Map MapArenaCollisionKey U32) arena))
  (= ([] extra (call MapArenaCollisionKey 200u32)) 200u32)
  (let clone (+ map extra))
  (let removed (- clone extra))
  (blank)
  (print (type-of (call (of Map MapArenaCollisionKey U32))))
  (print (type-of map))
  (print (type-of clone))
  (print (type-of removed))
  (print (. reserved capacity))
  (print ([] from-pairs (call MapArenaCollisionKey 300u32)))
  (print (tuple valid visited checksum))
  (print (tuple ([] map (call MapArenaCollisionKey 5u32)) ([] map (call MapArenaCollisionKey 6u32))))
  (print (tuple (. clone length) ([] clone (call MapArenaCollisionKey 200u32))))
  (print (tuple (. removed length) (has removed (call MapArenaCollisionKey 200u32))))
  (print (tuple (. GCBacking Context size) (. ArenaBacking Context size)))
  (print (tuple (. (of Map U32 U32) size) (. (of Map U32 U32 ArenaBacking) size)))
  (blank)
  (let retained-capacity (. map capacity))
  (call (. map clear))
  (= ([] map (call MapArenaCollisionKey 7u32)) 77u32)
  (print (tuple (. map length) (. map capacity) retained-capacity ([] map (call MapArenaCollisionKey 7u32))))
)

(type + MapArenaCollisionKey (is Struct)
  (implements Hashable)
  (prop + identity U32)
  (fn-expr + (infix this == (param other MapArenaCollisionKey)) _
    (== identity (. other identity))
  )
  (fn-expr + hash Hash :get
    0u32
  )
)
```

```output
[TypeInfo Map'MapArenaCollisionKey'U32'GCBacking]
[TypeInfo Map'MapArenaCollisionKey'U32'ArenaBacking]
[TypeInfo Map'MapArenaCollisionKey'U32'ArenaBacking]
[TypeInfo Map'MapArenaCollisionKey'U32'ArenaBacking]
32
3
(true, 96, 41555)
(5000, 1006)
(97, 200)
(96, false)
(0, 16)
(8, 8)
(1, 128, 128, 77)
```

### Arena-backed Map reservation failures use the normal Map error path.

```brix crash
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (let map (call (of Map U32 U32) arena map-maximum-capacity))
)
```

### Mutating a map during active iteration is detected.

```brix crash
(main
  (let map (call (of Map U32 U32)))
  (= ([] map 1u32) 1u32)
  (= ([] map 2u32) 2u32)
  (for (in key (. map keys))
    (= ([] map 3u32) 3u32)
  )
)
```

### Impossible reservations fail explicitly before allocating.

```brix crash
(main
  (let map (call (of Map U32 U32) map-maximum-capacity))
)
```

```brix
(import /core/atom-pool)
(blank)

(main
  (let atoms (call AtomPool))
  (let others (call AtomPool))
  (finally
    (call (. atoms release))
    (call (. others release))
  )
  (blank)
  (let alpha (call (. atoms store) "alpha"))
  (let alpha-again (call (. atoms store) "alpha"))
  (let beta (call (. atoms store) "beta"))
  (let other-alpha (call (. others store) "alpha"))
  (let map (call (of Map Atom String)))
  (= ([] map alpha) "first")
  (= ([] map alpha-again) "updated")
  (= ([] map beta) "second")
  (= ([] map other-alpha) "other")
  (blank)
  (print (. map length))
  (print ([] map alpha))
  (print ([] map beta))
  (print ([] map other-alpha))
  (print (has map alpha-again))
  (print (== alpha alpha-again))
  (print (!= alpha other-alpha))
  (print (== (call hash-for alpha) (call hash-for (. alpha address))))
)
```

```output
3
updated
second
other
true
true
true
true
```

```brix
(main
  (let map
    (block
      (item "alpha")
      (item "beta")
      (pipe *> (tuple each each))
      (pipe |> Map)
    )
  )
  (blank)
  (print (type-of map))
  (print (tuple (. map length) ([] map "alpha") ([] map "beta")))
)
```

```output
[TypeInfo Map'String'String'GCBacking]
(2, alpha, beta)
```

```brix
(main
  (let map (call Map (call pairs) 24))
  (blank)
  (print (type-of map))
  (print (. map capacity))
  (print (tuple (. map length) ([] map "a") ([] map "b")))
)

(fn-expr + pairs _
  (item (tuple "a" 1))
  (item (tuple "b" 2))
  (item (tuple "a" 3))
)
```

```output
[TypeInfo Map'String'I64'GCBacking]
32
(2, 3, 2)
```

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (let map (call Map arena (call pairs) 24))
  (blank)
  (print (type-of map))
  (print (. map capacity))
  (print (tuple (. map length) ([] map "a") ([] map "b")))
)

(fn-expr + pairs _
  (item (tuple "a" 1))
  (item (tuple "b" 2))
  (item (tuple "a" 3))
)
```

```output
[TypeInfo Map'String'I64'ArenaBacking]
32
(2, 3, 2)
```

### Map bullets preserve string keys when their values widen into a union.

```brix
(main
  (let map
    (call (of Map String Value)
      (block
        (entry "include_usage" true)
      )
    )
  )
  (blank)
  (print (as ([] map "include_usage") Bool))
)

(type + Value (is Union)
  (variant Null)
  (variant Bool)
  (variant Int)
  (variant String)
  (variant (map String Value))
)
```

```output
true
```

### A map assigned by a key answers the value assigned, and holds one value for a key.

```brix
(main
  (let map (map String Int))
  (= ([] map "foo") 42)
  (print ([] map "foo"))
  (print (. map length))
  (print (= ([] map "bar") 7))
  (print (tuple ([] map "bar") ([] map "none")))
  (= ([] map "foo") 43)
  (print (tuple ([] map "foo") (. map length)))
  (print (tuple (has map "foo") (has map "zed")))
)
```

```output
42
1
7
(7, unknown)
(43, 2)
(true, false)
```

### A map made from one pair, from two, and from a bullet, with a value that is a number and one that is a union.

```brix
(main
  (let one (call (of Map String Int) (tuple "a" 1)))
  (print (. one length))
  (print ([] one "a"))
  (let two (call (of Map String Int) (tuple "a" 1) (tuple "b" 2)))
  (print (. two length))
  (print ([] two "b"))
  (let mixed (call (of Map String Value) (tuple "flag" true)))
  (print (. mixed length))
  (print (as ([] mixed "flag") Bool))
  (let listed
    (call (of Map String Value)
      (block
        (entry "count" 3)
      )
    )
  )
  (print (. listed length))
  (print (as ([] listed "count") Int))
)

(type + Value (is Union)
  (variant Null)
  (variant Bool)
  (variant Int)
)
```

```output
1
1
2
2
1
true
1
3
```

```brix
(main
  (let map (call build))
  (print (. map length))
  (print (has map "omitted"))
  (print (as (!! ([] map "type")) String))
  (print (as (!! ([] map "width")) Int))
  (print (call (. ", " join) (as (!! ([] map "children")) (list Value))))
)

(type + Value (is Union)
  (variant Null)
  (variant Int)
  (variant String)
  (variant (list Value))
)

(fn-expr + build _ (?! Error)
  (call (of Map String Value)
    (block
      (:children children)
      (:type? (only-if "layer" (if include)))
      (:width? (only-if (call checked 3) (if include)))
      (:omitted? (only-if (call checked 4) (if (not include))))
    )
  )
  (where
    (lazy children
      (block
        (spread (list 1 2))
        (pipe *> (call checked each))
        (pipe |> List)
      )
    )
    (lazy include true)
  )
)

(fn-expr _ checked _ (?! Error)
  (param value Int)
  (as value Value)
)
```

```output
3
false
layer
3
1, 2
```

### A map's length is read with a getter, so it cannot be assigned.

```brix crash
(main
  (let map (call (of Map String Int)))
  (= ([] map "a") 1)
  (= (. map length) 5)
)
```

```output
Exception: Setter not found
<no file:5>
      (= (. map length) 5)
         ^^^^^^^^^^^^^^
```

### Fractional float keys hash by their bits, and signed zeros share one key.

```brix
(main
  (let map (call (of Map F64 String)))
  (= ([] map 0.25) "quarter")
  (= ([] map 0.5) "half")
  (= ([] map 0.0) "positive")
  (= ([] map -0.0) "negative")
  (-= ([] map 0.25))
  (print (. map length))
  (print (!! ([] map 0.0)))
  (print (!! ([] map 0.5)))
  (print (== (call hash-for 0.0f32) (call hash-for -0.0f32)))
)
```

```output
2
negative
half
true
```
