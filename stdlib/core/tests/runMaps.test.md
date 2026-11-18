# Maps

```yumi
. map = Map'String'Int()
-- map["foo"]
```

```output
unknown
```

```yumi
. present Int? = 7
. absent Int?
. property = "strokeJoin"
. map = Map'String'Int
    •? "strokeWidth" present
    •? "strokeCap" absent
    •? (property) present
-- map.length
-- map["strokeWidth"]
-- map has "strokeCap"
-- map["strokeJoin"]
```

```output
2
7
false
7
```

```yumi
/BxGC/BxGC.h

. done SyncChannel'String
do new
    . prior-goal = C:brixGCHeapGoal()
    C:brixGCSetHeapGoal(C:brixGCLiveByteCount() + 1)
    . property = "strokeJoin-${41 + 1}"
    . present String? = "managed-${20 + 2}"
    . map = Map'String'String
        •? "strokeWidth" present
        •? (property) present
    C:brixGCSetHeapGoal(prior-goal)
    C:brixCollect()
    done <- "${map["strokeWidth"]!},${map[property]!}"
-- <- done
```

```output
managed-22,managed-22
```

```yumi
. map = Map'String'Int()
map["foo"] = 42
-- map["foo"]
```

```output
42
```

```yumi
. map = Map'String'Int()
map["a"] = 1
map["b"] = 2
-- map["a"]
-- map["b"]
```

```output
1
2
```

```yumi
. map = Map'String'Int()
-- map.length
map["foo"] = 42
-- map.length
```

```output
0
1
```

```yumi
. map = Map'String'Int()
map["foo"] = 1
map["bar"] = 2
-- map.length
-- map["foo"]
-- map["bar"]
```

```output
2
1
2
```

```yumi
. map = Map'String'Int()
map["c"] = 3
map["a"] = 1
map["b"] = 2
. copy = Map'String'Int()
for key, value in map
    copy[key] = value
-- copy.length
-- copy["a"]
-- copy["b"]
-- copy["c"]
```

```output
3
1
2
3
```

```yumi
. map = Map'String'Int()
map["a"] = 1
map["b"] = 2
map["c"] = 3
-= map["a"]
-- map.length
-- map has "a"
-- map["b"]
-- map["c"]
```

```output
2
false
2
3
```

```yumi
. map1 = Map'String'Int()
map1["a"] = 1
map1["c"] = 3
. map2 = Map'String'Int()
map2["b"] = 2
map2["c"] = 4
. combined = map1 + map2
-- combined.length
-- combined["a"]
-- combined["b"]
-- combined["c"]
```

```output
3
1
2
4
```

```yumi
. map1 = Map'String'Int()
map1["a"] = 1
map1["b"] = 2
map1["c"] = 3
. map2 = Map'String'Int()
map2["b"] = 2
map2["c"] = 4
. difference = map1 - map2
-- difference.length
-- difference["a"]
-- difference has "b"
-- difference has "c"
```

```output
1
1
false
false
```

### In-place merge and subtraction with the map itself keep or empty it.

```yumi
. map = Map'String'Int()
map["a"] = 1
map["b"] = 2
. alias = map
map += alias
-- map.length
-- alias["b"]
map -= alias
-- map.length
-- alias.length
```

```output
2
2
0
0
```

```yumi
. map = Map'String'Int()
map["a"] = 1
map["b"] = 2
map["c"] = 2
. found = Map'String'Bool()
for key in map.find(2)
    found[key] = true
-- found.length
-- found has "a"
-- found has "b"
-- found has "c"
```

```output
2
false
true
true
```

```yumi
. map [String = Int]
-- type-of map
```

```output
[TypeInfo Map'String'I64'GCBacking]
```

```yumi
-- Map
    • baz 42
-- Map
    • ("foo") 42
-- Map
    • ("foo") 1
    • baz 2
-- Map
    • baz 2
    • ("foo") 1
```

```output
[baz = 42]
[foo = 42]
[baz = 2, foo = 1]
[baz = 2, foo = 1]
```

```yumi
-- Map
    • ("foo") 42
    • ("bar") x
    • baz 42
    .. x = 10
```

```output
[baz = 42, bar = 10, foo = 42]
```

### A map constructor keeps its map result when contextualized as one case of a union.

```yumi
. root [String = MapConstructorValue] = Map'String'MapConstructorValue()
root["stream_options"] = Map
    • "include_usage" true
. stream-options = root["stream_options"] as [String = MapConstructorValue]
-- stream-options["include_usage"] as Bool

━━━━━━━━━━━━━━━━━━━━
+ MapConstructorValue is Union

| Bool
| [String = MapConstructorValue]
```

```output
true
```

### Map literals can mix identifier keys with explicit string keys.

```yumi
. content MapLiteralValue = Map'String'MapLiteralValue
    • type "image_url"
    • "image_url" Map'String'MapLiteralValue
        • url "https://example.com/image.png"
. root = content as [String = MapLiteralValue]
. image = root["image_url"] as [String = MapLiteralValue]
-- root["type"] as String
-- image["url"] as String

━━━━━━━━━━━━━━━━━━━━
+ MapLiteralValue is Union

| String
| [String = MapLiteralValue]
```

```output
image_url
https://example.com/image.png
```

### Optional map entries omit absent values and unwrap present values.

```yumi
. present String? = "yes"
. absent String?
. map = Map'String'String
    • type "image"
    •? present present
    •? absent absent
-- map has "present"
-- map has "absent"
-- map["present"]
```

```output
true
false
yes
```

### Full-width numeric keys remain distinct.

```yumi
. map = Map'U64'Int()
map[0u64] = 1
map[4294967296u64] = 2
-- map[0u64]
-- map[4294967296u64]
-- map.length
```

```output
1
2
2
```

### UTF-8 prefixes and equal 32-bit string hashes do not alias keys.

```yumi
. map = Map'String'Int()
map["éa"] = 1
map["éb"] = 2
map["zsycr9pu60"] = 3
map["hngah7hg7b"] = 4
-- map["éa"]
-- map["éb"]
-- map["zsycr9pu60"]
-- map["hngah7hg7b"]
-- map.length
```

```output
1
2
3
4
4
```

### Deleting a same-bucket head or tail preserves the rest of the chain.

```yumi
. delete-head = Map'U32'Int()
delete-head[0u32] = 0
delete-head[8u32] = 8
-= delete-head[0u32]
-- delete-head has 0u32
-- delete-head[8u32]
-- delete-head.length

. delete-tail = Map'U32'Int()
delete-tail[0u32] = 0
delete-tail[8u32] = 8
-= delete-tail[8u32]
-- delete-tail[0u32]
-- delete-tail has 8u32
-- delete-tail.length
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

```yumi
. map = Map'U32'Int()
map[0u32] = 0
map[8u32] = 8
map[16u32] = 16
-= map[8u32]

. key-count = 0
for key in map.keys
    key-count += 1
. value-count = 0
for value in map.values
    value-count += 1
. item-count = 0
for item in map.items
    item-count += 1
. iteration-count = 0
for key, value in map
    iteration-count += 1

-- map.length
-- !map
-- map has 0u32
-- map has 8u32
-- map has 16u32
-- (key-count, value-count, item-count, iteration-count)

-= map[24u32]
-- map.length
-- map has 0u32
-- map has 8u32
-- map has 16u32

-= map[0u32]
-= map[16u32]
-- map.length
-- !map
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

```yumi
. count = 80000
. map = Map'U64'U64()
. expected-key-checksum = 0u64
. expected-value-checksum = 0u64
for i in 0 .. < count
    . key = (i as U64) + ((i % 17) as U64) * 4294967296u64
    map[key] = i as U64
    expected-key-checksum += key
    expected-value-checksum += i as U64

. lookup-ok = true
for i in 0 .. < count
    . key = (i as U64) + ((i % 17) as U64) * 4294967296u64
    if map[key]! != i as U64
        lookup-ok = false

. key-count = 0
. key-checksum = 0u64
for key in map.keys
    key-count += 1
    key-checksum += key

. value-count = 0
. value-checksum = 0u64
for value in map.values
    value-count += 1
    value-checksum += value

. item-count = 0
. item-key-checksum = 0u64
. item-value-checksum = 0u64
for item in map.items
    item-count += 1
    item-key-checksum += item.key
    item-value-checksum += item.value

. iteration-count = 0
. iteration-key-checksum = 0u64
. iteration-value-checksum = 0u64
for key, value in map
    iteration-count += 1
    iteration-key-checksum += key
    iteration-value-checksum += value

-- map.length
-- lookup-ok
-- (key-count, value-count, item-count, iteration-count)
-- key-checksum == expected-key-checksum
-- value-checksum == expected-value-checksum
-- item-key-checksum == expected-key-checksum
-- item-value-checksum == expected-value-checksum
-- iteration-key-checksum == expected-key-checksum
-- iteration-value-checksum == expected-value-checksum
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

```yumi
. map = Map'String'Bool()
map["f"] = false
-- map has "f"
-- map has "missing"
```

```output
true
false
```

### Plain reference keys use stable object identity.

```yumi
. first = Thing()
. second = Thing()
. map = Map'Thing'String()
map[first] = "first"
map[second] = "second"
-- map[first]
-- map[second]
-- map.length

━━━━━━━━━━━━━━━━━━━━
+ Thing
```

```output
first
second
2
```

### Open-addressed capacity uses a 75% occupancy limit; reserve and clear retain storage.

```yumi
. map = Map'U32'U32()
-- map.capacity
for key in 0u32 .. < 6u32
    map[key] = key
-- map.capacity
map[6u32] = 6u32
-- map.capacity
. length-before-update = map.length
map[3u32] = 33u32
-- map.length == length-before-update
-- map[3u32]
map.reserve(24)
-- map.capacity
map.reserve(25)
-- map.capacity
map.clear()
-- map.length
-- map.capacity
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

```yumi
/core/arena

. arena = Arena()
do finally
    arena.release()

. map = Map'U32'U32(arena)
for key in 0u32 .. < 6u32
    map[key] = key * 10u32
. active-bytes = arena.active-bytes
for round in 0 .. < 4
    -= map[3u32]
    map[3u32] = 30u32
-- arena.active-bytes == active-bytes
-- map.length, map.capacity
-- map[0u32], map[3u32], map[5u32]
```

```output
true
(6, 8)
(0, 30, 50)
```

### Clearing a map with live entries and tombstones releases its values and keeps it usable.

```yumi
/core/gc

/BxGC/BxGC.h

. map = Map'U32'MapClearBox()
. weaks = fill-cleared(map)
map.clear()
C:brixCollect()
-- map.length
. live = 0
for weak in weaks
    if weak.get
        live += 1
-- live
map[2u32] = MapClearBox(20)
-- map[2u32]!.value
-- map[1u32] == null

━━━━━━━━━━━━━━━━━━━━
+ do fill-cleared List'(Weak'MapClearBox)
    - map Map'U32'MapClearBox

. weaks = List'(Weak'MapClearBox)()
for index in 0u32 .. < 4u32
    . box = MapClearBox(index as Int)
    map[index] = box
    weaks.append(Weak(box))
-= map[1u32]
= weaks

━━━━━━━━━━━━━━━━━━━━
+ MapClearBox

+ value = 0

────────────────────
+ This
    - v Int

value = v
```

```output
0
0
20
true
```

### Fully colliding keys survive deletion, tombstone reuse, and repeated churn.

```yumi
. map = Map'MapCollisionKey'U32()
for round in 0 .. < 12
    for index in 0 .. < 96
        map[MapCollisionKey(index as U32)] = (index + round * 1000) as U32
    for index in 0 .. < 96 by 3
        -= map[MapCollisionKey(index as U32)]
    for index in 0 .. < 96 by 3
        map[MapCollisionKey(index as U32)] = (index + round * 1000 + 1) as U32

. valid = map.length == 96
for index in 0 .. < 96
    . expected = (index + 11000) as U32
    if index % 3 == 0
        expected += 1u32
    if map[MapCollisionKey(index as U32)]! != expected
        valid = false

for index in 0 .. < 96
    -= map[MapCollisionKey(index as U32)]
-- valid
-- map.length
-- map has MapCollisionKey(500u32)

━━━━━━━━━━━━━━━━━━━━
+ MapCollisionKey is Struct
    is Hashable

+ identity U32

────────────────────
+ this == other MapCollisionKey

identity == other.identity

────────────────────
+ get hash Hash

0u32
```

```output
true
0
false
```

### Iteration visits every live entry exactly once without defining an order.

```yumi
. count = 128
. map = Map'U32'U32()
for index in 0 .. < count
    map[index as U32] = (index * 7) as U32

. seen = Bool[count]
. valid = true
. visited = 0
for key, value in map
    if key >= count as U32 || seen[key as Index] || value != key * 7u32
        valid = false
    else
        seen[key as Index] = true
    visited += 1

for index in 0 .. < count
    if !seen[index]
        valid = false
-- valid
-- visited
```

```output
true
128
```

### Deterministic mixed operations agree with a simple optional-array model.

```yumi
. key-count = 257
. map = Map'U32'I32()
. model = I32?[key-count]
. valid = true
for step in 0 .. < 20000
    . key = ((step * 73 + step / 11) % key-count) as U32
    . action = step % 5
    if action < 3
        . value = (step * 17 - 9000) as I32
        map[key] = value
        model[key as Index] = value
    else if action == 3
        -= map[key]
        model[key as Index] = null
    else if map[key] != model[key as Index]
        valid = false

for key in 0 .. < key-count
    if map[key as U32] != model[key]
        valid = false
-- valid
```

```output
true
```

### Optional and large reference-bearing values survive growth and deletion.

```yumi
/BxGC/BxGC.h

. optional = Map'U32'(Optional'String)()
. absent Optional'String
optional[1u32] = absent
optional[2u32] = "present"

. references = Map'ReferenceMapThing'ReferenceMapThing()
. roots [ReferenceMapThing]
for key in 0 .. < 2000
    . key-object = ReferenceMapThing(key as U32)
    . value-object = ReferenceMapThing((key + 2000) as U32)
    roots += key-object
    roots += value-object
    references[key-object] = value-object
for key in 0 .. < 1000
    -= references[roots[key * 2]]
roots.clear()
C:brixGCCollect()

. reference-checksum = 0u64
for key, value in references
    reference-checksum += key.id as U64
    reference-checksum += value.id as U64

. large = Map'U32'LargeMapValue()
for key in 0 .. < 2000
    large[key as U32] = LargeMapValue("value-$key", key as U64, (key + 1) as U64, (key + 2) as U64, (key + 3) as U64)
for key in 0 .. < 1000
    -= large[key as U32]

-- optional has 1u32
-- optional has 2u32
-- optional.length
-- references.length
-- reference-checksum
-- large.length
. final-value = large[1999u32]!
-- final-value.label
-- final-value.d

━━━━━━━━━━━━━━━━━━━━
+ LargeMapValue is Struct

+ label String
+ a U64
+ b U64
+ c U64
+ d U64

━━━━━━━━━━━━━━━━━━━━
+ ReferenceMapThing

+ id U32

────────────────────
+ This
    - id
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

```yumi
/core/arena

. arena = Arena()
do finally
    arena.release()

. map = Map'MapArenaCollisionKey'U32(arena)
. reserved = Map'MapArenaCollisionKey'U32(arena, 24)
. from-pairs = Map'MapArenaCollisionKey'U32(arena, KeyValuePair(MapArenaCollisionKey(300u32), 3u32))

for index in 0 .. < 96
    map[MapArenaCollisionKey(index as U32)] = index as U32
map[MapArenaCollisionKey(5u32)] = 5000u32
for index in 0 .. < 96 by 3
    -= map[MapArenaCollisionKey(index as U32)]
for index in 0 .. < 96 by 3
    map[MapArenaCollisionKey(index as U32)] = (index + 1000) as U32

. valid = map.length == 96
. visited = 0
. checksum = 0u64
for key, value in map
    visited += 1
    checksum += value as U64
    if map[key]! != value
        valid = false

. extra = Map'MapArenaCollisionKey'U32(arena)
extra[MapArenaCollisionKey(200u32)] = 200u32
. clone = map + extra
. removed = clone - extra

-- type-of Map'MapArenaCollisionKey'U32()
-- type-of map
-- type-of clone
-- type-of removed
-- reserved.capacity
-- from-pairs[MapArenaCollisionKey(300u32)]
-- valid, visited, checksum
-- map[MapArenaCollisionKey(5u32)], map[MapArenaCollisionKey(6u32)]
-- clone.length, clone[MapArenaCollisionKey(200u32)]
-- removed.length, removed has MapArenaCollisionKey(200u32)
-- GCBacking.Context.size, ArenaBacking.Context.size
-- Map'U32'U32.size, Map'U32'U32'ArenaBacking.size

. retained-capacity = map.capacity
map.clear()
map[MapArenaCollisionKey(7u32)] = 77u32
-- map.length, map.capacity, retained-capacity, map[MapArenaCollisionKey(7u32)]

━━━━━━━━━━━━━━━━━━━━
+ MapArenaCollisionKey is Struct
    is Hashable

+ identity U32

────────────────────
+ this == other MapArenaCollisionKey

identity == other.identity

────────────────────
+ get hash Hash

0u32
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

```yumi crash
/core/arena

. arena = Arena()
do finally
    arena.release()
. map = Map'U32'U32(arena, map-maximum-capacity)
```

### Mutating a map during active iteration is detected.

```yumi crash
. map = Map'U32'U32()
map[1u32] = 1u32
map[2u32] = 2u32
for key in map.keys
    map[3u32] = 3u32
```

### Impossible reservations fail explicitly before allocating.

```yumi crash
. map = Map'U32'U32(map-maximum-capacity)
```

```yumi
/core/atom-pool

. atoms = AtomPool()
. others = AtomPool()
do finally
    atoms.release()
    others.release()

. alpha = atoms.store("alpha")
. alpha-again = atoms.store("alpha")
. beta = atoms.store("beta")
. other-alpha = others.store("alpha")
. map = Map'Atom'String()
map[alpha] = "first"
map[alpha-again] = "updated"
map[beta] = "second"
map[other-alpha] = "other"

-- map.length
-- map[alpha]
-- map[beta]
-- map[other-alpha]
-- map has alpha-again
-- alpha == alpha-again
-- alpha != other-alpha
-- hash-for(alpha) == hash-for(alpha.address)
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

```yumi
. map =
    * "alpha"
    * "beta"
    *> (each, each)
    |> Map

-- type-of map
-- (map.length, map["alpha"], map["beta"])
```

```output
[TypeInfo Map'String'String'GCBacking]
(2, alpha, beta)
```

```yumi
. map = Map(pairs(), 24)

-- type-of map
-- map.capacity
-- (map.length, map["a"], map["b"])

━━━━━━━━━━━━━━━━━━━━
+ pairs

* ("a", 1)
* ("b", 2)
* ("a", 3)
```

```output
[TypeInfo Map'String'I64'GCBacking]
32
(2, 3, 2)
```

```yumi
/core/arena

. arena = Arena()
do finally
    arena.release()
. map = Map(arena, pairs(), 24)

-- type-of map
-- map.capacity
-- (map.length, map["a"], map["b"])

━━━━━━━━━━━━━━━━━━━━
+ pairs

* ("a", 1)
* ("b", 2)
* ("a", 3)
```

```output
[TypeInfo Map'String'I64'ArenaBacking]
32
(2, 3, 2)
```

### Map bullets preserve string keys when their values widen into a union.

```yumi
. map = Map'String'Value
    • "include_usage" true

-- map["include_usage"] as Bool

━━━━━━━━━━━━━━━━━━━━
+ Value is Union

| Null
| Bool
| Int
| String
| [String = Value]
```

```output
true
```

### A map assigned by a key answers the value assigned, and holds one value for a key.

```yumi
. map [String = Int]
map["foo"] = 42
-- map["foo"]
-- map.length
-- (map["bar"] = 7)
-- map["bar"], map["none"]
map["foo"] = 43
-- map["foo"], map.length
-- map has "foo", map has "zed"
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

```yumi
. one = Map'String'Int(("a", 1))
-- one.length
-- one["a"]
. two = Map'String'Int(("a", 1), ("b", 2))
-- two.length
-- two["b"]
. mixed = Map'String'Value(("flag", true))
-- mixed.length
-- mixed["flag"] as Bool
. listed = Map'String'Value
    • "count" 3
-- listed.length
-- listed["count"] as Int

━━━━━━━━━━━━━━━━━━━━
+ Value is Union

| Null
| Bool
| Int
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

```yumi
. map = build()
-- map.length
-- map has "omitted"
-- map["type"]! as String
-- map["width"]! as Int
-- ", ".join(map["children"]! as [Value])

━━━━━━━━━━━━━━━━━━━━
+ Value is Union

| Null
| Int
| String
| [Value]

━━━━━━━━━━━━━━━━━━━━
+ build ?! Error

Map'String'Value
    • children children
    •? type "layer" if include
    •? width checked(3) if include
    •? omitted checked(4) if !include

.. children =
    ** [1, 2]
    *> checked(each)
    |> List
.. include = true

━━━━━━━━━━━━━━━━━━━━
_ checked ?! Error
    - value Int

value as Value
```

```output
3
false
layer
3
1, 2
```

### A map's length is read with a getter, so it cannot be assigned.

```yumi crash
. map = Map'String'Int()
map["a"] = 1
map.length = 5
```

```output
Exception: Setter not found
<no file:4>
    map.length = 5
    ^^^^^^^^^^
```

### Fractional float keys hash by their bits, and signed zeros share one key.

```yumi
. map = Map'F64'String()
map[0.25] = "quarter"
map[0.5] = "half"
map[0.0] = "positive"
map[-0.0] = "negative"
-= map[0.25]
-- map.length
-- map[0.0]!
-- map[0.5]!
-- hash-for(0.0f32) == hash-for(-0.0f32)
```

```output
2
negative
half
true
```

