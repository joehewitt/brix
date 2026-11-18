# Struct mutation

```yumi
. item = Item(1)
item[0] = 7
-- item.x
━━━━━━━━━━━━━━━━━━━━
+ Item is Struct

+ x Int
────────────────────
+ do this[index] = value

x = value
= this
```

```output
7
```

```yumi
. holder = Holder(Item(1))
holder.item[0] = 7
-- holder.item.x
━━━━━━━━━━━━━━━━━━━━
+ Item is Struct

+ x Int
────────────────────
+ do this[index] = value

x = value
= this
━━━━━━━━━━━━━━━━━━━━
+ Holder

+ item Item
────────────────────
+ This
    - item
```

```output
7
```

```yumi
. items = Array'Item([Item(2)])
. copy = items[0]
items[0][0] = 9
-- items[0].x
-- copy.x
━━━━━━━━━━━━━━━━━━━━
+ Item is Struct

+ x Int
────────────────────
+ do this[index] = value

x = value
= this
```

```output
9
2
```

```yumi
. items = Array'Item([Item(2)])
. probe = Probe(items)
probe.items()[probe.index()][probe.index()] = 9
-- items[0].x
-- probe.calls
-- probe.indices
━━━━━━━━━━━━━━━━━━━━
+ Item is Struct

+ x Int
────────────────────
+ do this[index] = value

x = value
= this
━━━━━━━━━━━━━━━━━━━━
+ Probe

+ values Array'Item
+ calls = 0
+ indices = 0
────────────────────
+ This
    - values
────────────────────
+ do items

calls += 1
= values
────────────────────
+ do index

indices += 1
= 0
```

```output
9
1
2
```

```yumi
. p = Point(1, 2)
p = p.move(5)
-- p
. q = p
q = q.move(1)
-- p
-- q
━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x I32
+ y I32
────────────────────
+ do move
    - dx I32

x = x + dx
= this
```

```output
(6, 2)
(6, 2)
(7, 2)
```

### A method of a struct that assigns a field changes its own copy of the struct, whatever its size.

```yumi
. item = Big(1, 2, 3, 4, 5)
item.set(7)
-- item.a
-- item.bump(), item.a

━━━━━━━━━━━━━━━━━━━━
+ Big is Struct

+ a I64
+ b I64
+ c I64
+ d I64
+ e I64

────────────────────
+ do set
    - value

a = value

────────────────────
+ do bump I64

a = a + 10
= a
```

```output
1
(11, 1)
```

### What a struct's []= answers is stored where the struct is held: a local, a property, a global, a field, an argument

```yumi
+ kept = Item(1)

. item = Item(1)
item[0] = 7
-- item.x
. holder = Holder(Item(1), Pair(Item(2), 3))
holder.item[0] = 8
-- holder.item.x
kept[0] = 9
-- kept.x
. pair = Pair(Item(4), 5)
pair.item[0] = 10
-- pair.item.x, pair.count
-- change(Item(11)).x
holder.set(12)
-- holder.item.x
. made = Item(1)
made[0] = made.x + 40
-- made.x

━━━━━━━━━━━━━━━━━━━━
+ do change Item
    - given Item

given[0] = 20
= given

━━━━━━━━━━━━━━━━━━━━
+ Item is Struct

+ x Int

────────────────────
+ do this[index] = value

x = value
= this

━━━━━━━━━━━━━━━━━━━━
+ Pair is Struct

+ item Item
+ count Int

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ item Item
+ pair Pair

────────────────────
+ This
    - item
    - pair

────────────────────
+ do set
    - value Int

item[0] = value
```

```output
7
8
9
(10, 5)
20
12
41
```

### What []= answers for a struct an index gave is stored through that index

```yumi
+ kept = Array'Item([Item(1), Item(2)])

. items = Array'Item([Item(1), Item(2), Item(3)])
items[1][0] = 7
-- items[0].x, items[1].x, items[2].x
. holder = Holder(Array'Item([Item(4), Item(5)]))
holder.items[1][0] = 8
-- holder.items[0].x, holder.items[1].x
kept[0][0] = 9
-- kept[0].x, kept[1].x
holder.set(1, 12)
-- holder.items[1].x
-- change(items, 2)
-- items[2].x
. position = 0
items[position][0] = items[position + 1].x + 40
-- items[0].x
. list = [Item(1), Item(2)]
list[1][0] = 30
-- list[1].x, list[0].x
. grid = Array'(Array'Item)([Array'Item([Item(1), Item(2)]), Array'Item([Item(3)])])
grid[0][1][0] = 50
-- grid[0][1].x, grid[0][0].x, grid[1][0].x

━━━━━━━━━━━━━━━━━━━━
+ do change Int
    - given Array'Item
    - place Index

given[place][0] = 20
= given[place].x

━━━━━━━━━━━━━━━━━━━━
+ Item is Struct

+ x Int

────────────────────
+ do this[index] = value

x = value
= this

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ items Array'Item

────────────────────
+ This
    - items

────────────────────
+ do set
    - place Index
    - value Int

items[place][0] = value
```

```output
(1, 7, 3)
(4, 8)
(9, 2)
12
20
20
47
(30, 1)
(50, 1, 3)
```

### An index of an index assigned evaluates the value, the object and each key once, and raises what either function raises

```yumi
. items = Array'Item([Item(1), Item(2), Item(3)])
. seen = Seen()
items[seen.next()][seen.next()] = seen.next()
-- items[0].x, items[1].x, items[2].x, seen.count
store(items, 2, 1)
-- items[2].x
store(items, 2, 5)
-- items[2].x
store(items, 7, 1)

━━━━━━━━━━━━━━━━━━━━
+ do store
    - items Array'Item
    - place Index
    - field Int

. attempt = do new
    items[place][field] = 40
    -- "stored"
catch failure
    . panic Panic? = failure
    if panic
        -- "recovered $panic"
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ Item is Struct

+ x Int

────────────────────
+ do this[index Int] = value Int

?! Panic("No field $index")
    if index > 2
x = value + index
= this

━━━━━━━━━━━━━━━━━━━━
+ Seen

+ count = 0

────────────────────
+ do next

count += 1
-- "next $count"
= count - 1
```

```output
next 1
next 2
next 3
(1, 2, 3, 3)
stored
41
recovered No field 5
41
recovered Index out of bounds (7 / 3)
```

### The value of an index of an index assigned is the value written, which is converted for the store

```yumi
. items = Array'Item([Item(1), Item(2)])
. taken = (items[1][0] = 7)
-- taken, items[1].x
. wide = Array'Wide([Wide(1, 2), Wide(3, 4)])
wide[1][0] = 2
-- wide[1].a, wide[1].b, wide[0].a
. other = wide
other[0][1] = 9
-- wide[0].b

━━━━━━━━━━━━━━━━━━━━
+ Item is Struct

+ x Int

────────────────────
+ do this[index] = value

x = value
= this

━━━━━━━━━━━━━━━━━━━━
+ Wide is Struct

+ a I64
+ b I64

────────────────────
+ do this[index Int] = value I64

if index == 0
    a = value
else
    b = value
= this
```

```output
(7, 7)
(2, 4, 1)
9
```

### A struct indexed for a store is read where its function is called, after the key replaced it

```yumi
. stack = Stack()
stack.push(5)
stack.push(6)
stack.push(7)
-- stack.values[0], stack.values[1], stack.values[2]

━━━━━━━━━━━━━━━━━━━━
+ Stack

+ values = Array'Int(1)
+ count = 0

────────────────────
+ do push
    - value Int

values[slot()] = value

────────────────────
+ do slot Int

if count == values.size
    values = values.resize(count * 2)
count += 1
= count - 1
```

```output
(5, 6, 7)
```

### A struct indexed is read where its function is called, after the key assigned it

```yumi
. item = Item(1)
-- item[(item = Item(50)).x]
. numbers = Array'Int([1, 2, 3])
-- numbers[(numbers = Array'Int([7, 8, 9])).size - 2]

━━━━━━━━━━━━━━━━━━━━
+ Item is Struct

+ x Int

────────────────────
+ this[index Int] Int

x + index
```

```output
100
8
```

### What an index store answers is stored where the object was read from, though the key names another since

```yumi
. a = Holder()
. b = Holder()
. h = a
h.item[(h = b).zero] = 7
-- a.item.x, b.item.x

━━━━━━━━━━━━━━━━━━━━
+ Item is Struct

+ x Int

────────────────────
+ do this[index Int] = value Int

x = value
= this

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ item = Item(1)
+ zero = 0
```

```output
(7, 1)
```

### What an index store answers is stored where the object was read from, though the function names another since

```yumi
+ current = Holder()
+ other = Holder()

. first = current
current.item[0] = 7
-- first.item.x, other.item.x, current == other

━━━━━━━━━━━━━━━━━━━━
+ Item is Struct

+ x Int

────────────────────
+ do this[index Int] = value Int

current = other
x = value
= this

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ item = Item(1)
```

```output
(7, 1, true)
```

### A number an index store answers is stored where the object was read from

```yumi
. a = Holder()
. b = Holder()
. h = a
h.count[(h = b).zero] = 7u8
-- a.count as U8, b.count as U8

━━━━━━━━━━━━━━━━━━━━
+ Count'Integer is Integer

────────────────────
+ this[index Index] = value Integer

((this as Integer) + value) as This

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ count = 1 as Count'U8
+ zero Index = 0
```

```output
(8, 1)
```

### A field of a struct an index names is stored where the struct lies: in a list, past a pointer, in a list a property holds, and in a struct inside either.

```yumi
/core/arena

. arena = Arena()
do finally arena.release()
. l [S]
l += S(1, 2)
l[0].a = 10
l[0].b += 5
. al = List'S(arena)
al += S(1, 2)
al[0].a = 11
. p = arena.allocate'Outer(2)
p[1] = Outer(S(1, 2), 3)
p[1].inner.b = 20
p[1].inner.a += 4
. w = Wrapper()
w.items[0].a = 15
. nested [Outer]
nested += Outer(S(1, 2), 3)
nested[0].inner.a = 16
nested[0].c -= 1
. named [N]
named += N(1, "one")
named[0].name = "uno"
named[0].a *= 3
-- "${l[0].a} ${l[0].b} ${al[0].a}"
-- "${p[1].inner.a} ${p[1].inner.b} ${p[1].c}"
-- "${w.items[0].a} ${nested[0].inner.a} ${nested[0].c} ${named[0].a} ${named[0].name}"

━━━━━━━━━━━━━━━━━━━━
+ S is Struct

+ a Int
+ b Int

━━━━━━━━━━━━━━━━━━━━
+ Outer is Struct

+ inner S
+ c Int

━━━━━━━━━━━━━━━━━━━━
+ N is Struct

+ a Int
+ name String

━━━━━━━━━━━━━━━━━━━━
+ Wrapper

+ items [S]

────────────────────
+ This

items += S(1, 2)
```

```output
10 7 11
5 20 3
15 16 2 3 uno
```

### A field of a struct an array's item holds is stored in the item, also inside a struct it holds and with an assigning operator.

```yumi
. arr = Array'S(2)
arr[0] = S(1, 2)
arr[1] = S(3, 4)
arr[0].a = 12
arr[1].b += 5
. slot = 1
arr[slot].a *= 2
-- "${arr[0].a} ${arr[0].b} ${arr[1].a} ${arr[1].b}"
. outer = Array'Outer(1)
outer[0] = Outer(S(1, 2), 3)
outer[0].inner.a = 7
outer[0].c -= 1
-- "${outer[0].inner.a} ${outer[0].inner.b} ${outer[0].c}"

━━━━━━━━━━━━━━━━━━━━
+ S is Struct

+ a Int
+ b Int

━━━━━━━━━━━━━━━━━━━━
+ Outer is Struct

+ inner S
+ c Int
```

```output
12 2 6 9
7 2 2
```
