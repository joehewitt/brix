# Lists

```yumi
. list = List'I32()
-- list.length

list.add(10)
-- list.length
-- list[0]

list.add(12)
-- list.length
-- list[1]
```

```output
0
1
10
2
12
```

```yumi
. list = List'I32()
-- list.length

list += 10
-- list.length
-- list[0]

list += 12
-- list.length
-- list[1]
```

```output
0
1
10
2
12
```

```yumi
. list = List'Float()
-- list.length

list.add(10.5)
-- list.length
-- list[0]

list.add(12.5)
-- list.length
-- list[1]

list[1] = 15.5
-- list[1]
```

```output
0
1
10.5
2
12.5
15.5
```

```yumi
. thing = Thing(10)
. list = List'Thing()
-- list.length
list.add(thing)
-- list.length
-- list[0].a

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ a = 0

────────────────────
+ This
    - v

a = v
```

```output
0
1
10
```

```yumi
. list = [1,2,3]
-- list[0]
-- list[1]
-- list[2]
```

```output
1
2
3
```

```yumi
. list = [true, false, true]
-- list[0]
-- list[1]
-- list[2]
```

```output
true
false
true
```

### One float in the list makes the whole list floats

```yumi
. list = [1, 2.5, 3]
-- list[0]
-- list[1]
-- list[2]
```

```output
1
2.5
3
```

```yumi
. list = [ListSourceA(), ListSourceB()]
-- list[0].value
-- list[1].value

━━━━━━━━━━━━━━━━━━━━
+ ListSourceA

────────────────────
+ to ListConverted

ListConverted(17)

━━━━━━━━━━━━━━━━━━━━
+ ListSourceB

────────────────────
+ to ListConverted

ListConverted(29)

━━━━━━━━━━━━━━━━━━━━
+ ListConverted

+ value I32

────────────────────
+ This
    - value
```

```output
17
29
```

```yumi
. seed = list-runtime-value()
. list = [seed + 3, seed * 2, seed - 4]
-- list.length
-- list[0]
-- list[1]
-- list[2]

━━━━━━━━━━━━━━━━━━━━
_ list-runtime-value I32

11
```

```output
3
14
22
7
```

```yumi
. list = [Sub1(), Sub2(), Base()]
-- list[0]
-- list[0].a()
-- list[1]
-- list[1].a()
-- list[2]
-- list[2].a()

━━━━━━━━━━━━━━━━━━━━
+ Base

────────────────────
+ a

0

━━━━━━━━━━━━━━━━━━━━
+ Sub1 is Base

────────────────────
+ a

1

━━━━━━━━━━━━━━━━━━━━
+ Sub2 is Base

────────────────────
+ a

2
```

```output
[Object __source__:Base]
1
[Object __source__:Base]
2
[Object __source__:Base]
0
```

```yumi
. list = [Sub1(), Sub2()]
-- list[0]
-- list[0].a()
-- list[1]
-- list[1].a()

━━━━━━━━━━━━━━━━━━━━
+ Base1

────────────────────
+ a

0

━━━━━━━━━━━━━━━━━━━━
+ Base2 is Base1

────────────────────
+ a

1

━━━━━━━━━━━━━━━━━━━━
+ Sub1 is Base2

────────────────────
+ a

2

━━━━━━━━━━━━━━━━━━━━
+ Sub2 is Base2

────────────────────
+ a

3
```

```output
[Object __source__:Base2]
2
[Object __source__:Base2]
3
```

```yumi
. list = [Sub1(), Sub2(), Base1()]
-- list[0]
-- list[0].a()
-- list[1]
-- list[1].a()
-- list[2]
-- list[2].a()

━━━━━━━━━━━━━━━━━━━━
+ Base1

────────────────────
+ a

0

━━━━━━━━━━━━━━━━━━━━
+ Base2 is Base1

────────────────────
+ a

1

━━━━━━━━━━━━━━━━━━━━
+ Sub1 is Base2

────────────────────
+ a

2

━━━━━━━━━━━━━━━━━━━━
+ Sub2 is Base2

────────────────────
+ a

3
```

```output
[Object __source__:Base1]
2
[Object __source__:Base1]
3
[Object __source__:Base1]
0
```

```yumi
. list = [1,2]
-- !list
list = List'I32()
-- !list
```

```output
false
true
```

```yumi
for a in [1,2,3]
    -- a
```

```output
1
2
3
```

```yumi
. iter = [1,2,3] as <Int>
for a in iter
    -- a
```

```output
1
2
3
```

```yumi
. l = [1]
l += 2
-- l
```

```output
1, 2
```

```yumi
. l = [1]
l <-* [2,3]
-- l
```

```output
1, 2, 3
```

```yumi
. l = [1,2,3,4]
-- l.find(3)
```

```output
2
```

```yumi
. l = [1,2,3,4]
-- l has 3
-- l has not 3
-- l has 5
-- l has not 5
```

```output
true
false
false
true
```

```yumi
. list = [1,2,3,4]
list.slide(-1)
-- list

list = [1,2,3,4]
list.slide(-3)
-- list

list = [1,2,3,4]
list.slide(1)
-- list
```

```output
2, 3, 4
4
1, 1, 2, 3, 4
```

```yumi
. list1 = [1, 2, 3]
. list2 = List(list1)
-- list2
```

```output
1, 2, 3
```

```yumi
. list = [1, 2, 3, 4, 5, 6]
list[0, 4, 5] = 8
-- list
```

```output
8, 2, 3, 4, 8, 8
```

```yumi
. x = [1,2,3,4,5,6]
-= x[3 .. < Position.end]
-- x
```

```output
1, 2, 3
```

### Assigning past the end of a list panics, as reading past it does, and leaves no empty slots.

```yumi
. names = ["a"]
. attempt = do new
    names[2] = "c"
catch failure
    -- failure
attempt.wait()
-- names
```

```output
List index 2 out of range
a
```

```yumi
. attempt = do new
    . list = [1,2,3]
    -- list[3]
catch failure
    . err Panic? = failure
    -- "Caught '$err'"
attempt.wait()
```

```output
Caught 'List index 3 out of range'
```

```yumi
. list = ["a", "b"]

try-read(list, -1)
try-write(list, -5)
try-replace(list, -1)
try-insert(list, -1)
try-remove(list, -1)
-- list

━━━━━━━━━━━━━━━━━━━━
_ do try-read
    - list [String]
    - index Index

. attempt = do new
    -- list[index]
catch failure
    -- "read: $failure"
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
_ do try-write
    - list [String]
    - index Index

. attempt = do new
    list[index] = "z"
catch failure
    -- "write: $failure"
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
_ do try-replace
    - list [String]
    - index Index

. attempt = do new
    list.replace(index, "z")
catch failure
    -- "replace: $failure"
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
_ do try-insert
    - list [String]
    - index Index

. attempt = do new
    list.insert("z", index)
catch failure
    -- "insert: $failure"
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
_ do try-remove
    - list [String]
    - index Index

. attempt = do new
    -= list[index]
catch failure
    -- "remove: $failure"
attempt.wait()
```

```output
read: List index -1 out of range
write: List index -5 out of range
replace: List range at index -1 with count 1 out of range
insert: List index -1 out of range
remove: List range at index -1 with count 1 out of range
a, b
```

```yumi
. list = [1,2,3]

try-counted-remove(list)
try-range-remove(list)
-- list

━━━━━━━━━━━━━━━━━━━━
+ do try-counted-remove
    - list [Int]

. attempt = do new
    list.remove(1, 5)
catch failure
    -- "counted: $failure"
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ do try-range-remove
    - list [Int]

. attempt = do new
    -= list[2 .. < 5]
catch failure
    -- "range: $failure"
attempt.wait()
```

```output
counted: List range at index 1 with count 5 out of range
range: List range at index 2 with count 3 out of range
1, 2, 3
```

```yumi
. list = [1,2,3]

list.remove(3, 0)
list.move(3, 3, 0)
try-zero-remove(list)
try-zero-move(list)
-- list

━━━━━━━━━━━━━━━━━━━━
+ do try-zero-remove
    - list [Int]

. attempt = do new
    list.remove(99, 0)
catch failure
    -- "remove: $failure"
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ do try-zero-move
    - list [Int]

. attempt = do new
    list.move(99, 99, 0)
catch failure
    -- "move: $failure"
attempt.wait()
```

```output
remove: List range at index 99 with count 0 out of range
move: List range at index 99 with count 0 out of range
1, 2, 3
```

```yumi
-- [1,2,3] is List'Float
-- [1,2,3] is! List'Float
-- [1,2,3] is~ List'Float
```

```output
false
false
true
```

### List capacity policy

```yumi
. empty = List'I32()
-- empty.length
-- empty.capacity

. ints = List'I32()
ints += 1
-- ints.capacity
ints += 2
ints += 3
ints += 4
-- ints.capacity
ints += 5
-- ints.capacity

. large = List'I32()
for i I32 in 0 .. < 1025
    large += i
-- large.capacity

. bytes = List'Byte()
bytes += 1u8
-- bytes.capacity

. one = [42]
-- one.capacity

. three = [1,2,3]
-- three.capacity

. many = [1,2,3,4,5,6]
-- many.capacity

. copy = List(three)
-- copy
-- copy.capacity
```

```output
0
0
4
4
8
1536
8
1
3
6
1, 2, 3
3
```

### List growth, replacement, removal, truncation, and clear only copy or drop Arena pointers.

```yumi
/core/arena

. first = Arena()
. second = Arena()
. first-pointer = first.allocate'U32(1)
first-pointer[0] = 10u32
. second-pointer = second.allocate'U32(1)
second-pointer[0] = 20u32

. arenas = List'Arena()
arenas += first
arenas += second
arenas.replace(0, second)
arenas.remove(1)
arenas.clear()
. first-next = first.allocate'U32(1)
first-next[0] = 11u32
-- first-next[0]
. second-next = second.allocate'U32(1)
second-next[0] = 21u32
-- second-next[0]

. pointers = List'(CPointer'U32)()
for index in 0 .. < 16
    pointers += first-pointer
pointers.replace(0, second-pointer)
pointers.remove(1, pointers.length - 1)
-- pointers[0][0]
pointers.clear()
-- first-pointer[0]

first.release()
second.release()
```

```output
11
21
20
10
```

### Reserving capacity preserves contents and does not notify observers.

```yumi
/core
/core/collection
/core/tests/support/markup-fixtures

<Demo demo
    for item in demo.#items
        <TextBox = item

demo.items += "a"
demo.items += "b"

. recorder = Recorder()
observe-collection(demo.items, recorder.record)
demo.items.reserve(1)
demo.items.reserve(16)
-- demo.items.length
-- demo.items.capacity
-- demo.items
-- recorder.count
demo.items += "c"
-- recorder.count
-- recorder.last
unobserve-collection(demo.items, recorder.record)

━━━━━━━━━━━━━━━━━━━━
+ Demo is Box

+ items [String]

━━━━━━━━━━━━━━━━━━━━
+ Recorder

+ count Size = 0
+ last String = ""

────────────────────
+ do record Void
    - mutation CollectionMutation

count += 1
last = "${mutation.kind}:${mutation.index}:${mutation.count}"
```

```output
2
16
a, b
0
1
append:2:1
```

### Arena-backed Lists retain one algorithm and preserve their backing through results and copies.

```yumi
/core/arena

. arena = Arena()
do finally
    arena.release()

. list = List'U32(arena)
for index in 0 .. < 10
    list += index as U32

. alias = list
alias[0] = 100u32
alias += 10u32

. slice = list[2 .. < 5]
slice[0] = 22u32
. plus = list + 11u32
. joined = list ++ [11u32, 12u32]

. copy = List(list)
copy[0] = 200u32
. from-items = List'U32(arena, [7u32, 8u32])

-- type-of List'U32()
-- type-of list
-- type-of slice
-- type-of plus
-- type-of joined
-- type-of copy
-- type-of from-items
-- list.length, list[0], list[2], list.tail
-- plus.length, plus.tail
-- joined.length, joined.tail
-- copy[0], list[0]
-- from-items[0], from-items[1]

list.remove(0, list.length)
list += 42u32
. sum = 0u32
for item in list
    sum += item
-- sum

list.release()
-- list.length, list.capacity
list += 5u32
-- list[0]
-- GCBacking.Context.size, ArenaBacking.Context.size
```

```output
[TypeInfo List'U32'GCBacking]
[TypeInfo List'U32'ArenaBacking]
[TypeInfo List'U32'ArenaBacking]
[TypeInfo List'U32'ArenaBacking]
[TypeInfo List'U32'ArenaBacking]
[TypeInfo List'U32'ArenaBacking]
[TypeInfo List'U32'ArenaBacking]
(11, 100, 22, 10)
(12, 11)
(13, 12)
(200, 100)
(7, 8)
42
(0, 0)
5
(0, 16)
```

```yumi
. entries = List'QueueEntry()
entries += (41 as Size, 9u32)
-- entries.length
if entries
    -- entries[0].address
    -- entries[0].generation

. stack [(uid U32, class-type U16)] = [(0, 0)]
. uid EntryUID = 1010
. class-type EntryClassId = 17
stack += (uid, class-type)
-- stack.length
-- stack[1].uid, stack[1].class-type

━━━━━━━━━━━━━━━━━━━━
+ QueueEntry is (address Size, generation U32)

━━━━━━━━━━━━━━━━━━━━
+ EntryUID is U32

━━━━━━━━━━━━━━━━━━━━
+ EntryClassId is U16
```

```output
1
41
9
2
(1010, 17)
```

### += and append add a collection as one item.

```yumi
. nested = List'[Int]()
nested += [1, 2]
nested.append([3])
-- nested.length
-- nested[0]

. values = List'Value()
. inner = List'Value()
inner += 1
inner += 2
values += inner
values.append(inner)
-- values.length

. entries = List'Value()
. entry = Map'String'Value
    • "count" 1
entries += entry
-- entries.length

━━━━━━━━━━━━━━━━━━━━
+ Value is Union

| Bool
| Int
| [Value]
| [String = Value]
```

```output
2
1, 2
2
1
```

### ++= and append-all add each item in place.

```yumi
. items = [1]
. alias = items
items ++= [2, 3]
items.append-all(4 .. 5)
-- alias

. values = List'Value()
. inner = List'Value()
inner += 1
inner += 2
values ++= inner
values.append-all(inner)
-- values.length

━━━━━━━━━━━━━━━━━━━━
+ Value is Union

| Bool
| Int
| [Value]
```

```output
1, 2, 3, 4, 5
4
```

### reversed yields items last to first and nothing for an empty collection.

```yumi
. items = [1, 2, 3]
-- ", ".join(items.reversed())
. empty = List'Int()
-- List'Int(empty.reversed()).length
. buffer = Int[3]
buffer[0] = 4
buffer[2] = 6
-- ", ".join(buffer.reversed())
. none = Int[0]
-- List'Int(none.reversed()).length
```

```output
3, 2, 1
0
6, 0, 4
0
```

### A list written holds its items in the order written, each converted to the type they join in.

```yumi
. a = [1, 2, 3]
-- a.size
-- a[0], a[1], a[2]
. b = [1.5, 2, 3]
-- b[1]
. small U8 = 4
. c = [small, 300]
-- c[0], c[1]
. one = [7]
-- one.size, one[0]
```

```output
3
(1, 2, 3)
2
(4, 300)
(1, 7)
```

### A list of text, of tuples, of structs or of objects gives each item by its index, and the members of an item read that way.

```yumi
. names = ["one", "two", "three"]
-- names.size, names[1]
. pairs = [(1, "a"), (2, "b")]
-- pairs[1]
. points = [Point(1, 2), Point(5, 6)]
-- points[1].x, points[0].y
. boxes = [Box("left"), Box("right")]
-- boxes[1].name
-- boxes[0].name.size

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int

━━━━━━━━━━━━━━━━━━━━
+ Box

+ name String

────────────────────
+ This
    - name String
```

```output
(3, two)
(2, b)
(5, 2)
right
4
```

### The items of a list written are evaluated in the order written.

```yumi
. list = [note("first", 1), note("second", 2), note("third", 3)]
-- list[0], list[1], list[2]
. texts = ["a${note("fourth", 4)}", "b${note("fifth", 5)}"]
-- texts[1], texts[0]

━━━━━━━━━━━━━━━━━━━━
+ note Int
    - text String
    - n Int

-- text
n
```

```output
first
second
third
(1, 2, 3)
fourth
fifth
(b5, a4)
```

### A list written keeps its items, and what they hold, across collections.

```yumi
/BxGC/BxGC.h

+ counted = 0

. names = ["a${note()}", "b${note()}"]
. boxes = [Box("left${note()}"), Box("right${note()}")]
. pairs = [Pair(1, "one${note()}"), Pair(2, "two${note()}")]
C:brixGCCollect()
. total = 0
for round in 0 .. < 2000
    . made = ["x$round", "y$round", "z$round"]
    total += made[1].size
    . more = [Pair(round, "p$round")]
    total += more[0].name.size
    . box = Box("b$round")
    total += box.name.size
C:brixGCCollect()
-- total
-- names[0], names[1]
-- boxes[0].name, boxes[1].name, boxes.size
-- pairs[0].name, pairs[1].name, pairs[1].id

━━━━━━━━━━━━━━━━━━━━
+ do note Int

counted += 1
= counted

━━━━━━━━━━━━━━━━━━━━
+ Box

+ name String

────────────────────
+ This
    - name String

━━━━━━━━━━━━━━━━━━━━
+ Pair is Struct

+ id Int
+ name String
```

```output
26670
(a1, b2)
(left3, right4, 2)
(one5, two6, 2)
```

### An index of a list is read with the list evaluated before the key, and assigned with the value evaluated before both. The assignment has the value assigned.

```yumi
+ a = [10, 20, 30]

-- a[next()]
-- a[0], a[2]
a[key("key")] = value("value")
-- a[0], a[1], a[2]
-- (a[1] = 44)
-- a[1]

━━━━━━━━━━━━━━━━━━━━
+ do next Int

a = [7, 8, 9]
= 1

━━━━━━━━━━━━━━━━━━━━
+ key Int
    - text String

-- text
0

━━━━━━━━━━━━━━━━━━━━
+ value Int
    - text String

-- text
55
```

```output
20
(7, 9)
value
key
(55, 8, 9)
44
44
```

### A list of items of two classes that share nothing is refused

```yumi crash
. list = [Thing(), Other()]
-- list.size

━━━━━━━━━━━━━━━━━━━━
+ Thing

━━━━━━━━━━━━━━━━━━━━
+ Other
```

```output
Exception: This list mixes Thing and Other, which have no common type; convert the items to one type
<no file:2>
    . list = [Thing(), Other()]
             ^^^^^^^^^^^^^^^^^^
```

### A list written with no items is refused, whatever is expected of it

```yumi crash
. list [Int] = []
-- list.size
```

```output
Exception: Incompatible types in list
<no file:2>
    . list [Int] = []
                   ^^
```

### A list of a struct and a number is refused

```yumi crash
. list = [Point(1, 2), 5]
-- list.size

━━━━━━━━━━━━━━━━━━━━
+ Point is Struct

+ x Int
+ y Int
```

```output
Exception: This list mixes Point and I64, which have no common type; convert the items to one type
<no file:2>
    . list = [Point(1, 2), 5]
             ^^^^^^^^^^^^^^^^
```

### A list printed whole: one that is empty, of one item, of texts, of floats, of lists, of objects and of tuples, and in a text.

```yumi
. empty [Int]
-- empty
. one = [7]
-- one
. words = ["a", "b", "c"]
-- words
-- [1.5, 2.5]
. nested = [[1, 2], [3]]
-- nested
-- "list: $words"
. things = [Thing(1), Thing(2)]
-- things
. mixed = [(1, "a"), (2, "b")]
-- mixed

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ n Int

────────────────────
+ This
    - n

────────────────────
+ to String

"T$n"
```

```output
7
a, b, c
1.5, 2.5
1, 2, 3
list: a, b, c
T1, T2
(1, a), (2, b)
```

### Slices of a list by ranges with a side left open, printed and looped over.

```yumi
. list = [1, 2, 3, 4, 5]
-- list[1 ...]
-- list[... 2]
-- list[1 .. 3]
-- list[1 .. < 3]
. tail = list[2 ...]
-- tail.length
for item in tail
    -- item
. total = 0
for item in list
    total += item
-- total
```

```output
2, 3, 4, 5
1, 2
2, 3, 4
2, 3
3
3
4
5
15
```

### A list as a stream: looped over, joined, and given to a function that takes a stream of any item.

```yumi
. words = ["x", "y"]
. stream = words as <String>
for word in stream
    -- word
-- ", ".join(words)
-- "-".join([1, 2, 3])
show(words)
show([1, 2])

━━━━━━━━━━━━━━━━━━━━
+ do show'T
    - items <T>

for item in items
    -- "item $item"
```

```output
x
y
x, y
1-2-3
item x
item y
item 1
item 2
```

### A list seen as an iterable and as an indexable, looped over through each.

```yumi
. list = [3, 4, 5]
. items Iterable'Int = list
for item in items
    -- item
. more Indexable'Int = list
for item in more
    -- item
-- more.length, more[1]
. sum = 0
for item in list as Iterable'Int
    sum += item
-- sum
```

```output
3
4
5
3
4
5
(3, 4)
12
```

### A function of a stream of any item, given a list of numbers and one of floats.

```yumi
. list = [3, 4, 5]
-- total(list), total([1.5, 2.5])

━━━━━━━━━━━━━━━━━━━━
+ do total'T T
    - items <T>

. sum T
for item in items
    sum += item
= sum
```

```output
(12, 4)
```

### A list a property holds, filled in a loop, printed, sliced and read in reverse.

```yumi
. box = Box()
-- box.size
box.fill(4)
-- box.size
-- box.items
. slice = box.items[1 .. 2]
-- slice
for item in box.items.reversed()
    -- item

━━━━━━━━━━━━━━━━━━━━
+ Box

+ items [Int]

────────────────────
+ get size

items.length

────────────────────
+ do fill
    - n Int

for i in 0 .. < n
    items += i * i
```

```output
0
4
0, 1, 4, 9
1, 4
9
4
1
0
```

```yumi
/core

. log = Log()
. counter = Counter()
-- literal-has("using")
-- literal-has("zeta")
-- literal-has-not("using")
-- literal-has-not("zeta")
-- literal-is-in("using")
-- literal-is-in("zeta")
-- literal-not-in("using")
-- literal-not-in("zeta")
-- number-has(8)
-- number-has(9)
-- folded-has()
-- computed-has(log, "b")
-- log.text
-- key-has(counter, 2)
-- counter.count
-- key-has(counter, 9)
-- counter.count
-- listed-has(counter, 2)
-- counter.count
-- tag-has(Tag("b"))
-- constant-has("using")
-- constant-has("zeta")
-- named-has("entry")
-- named-has("trace")
-- named-has("zeta")
-- named-is-in(7)
-- named-is-in(8)
-- measured-has("zeta")
-- grown-has("zeta")
-- count-below(4)
-- count-toward(4)
-- count-toward(-3)

_ heads = ["param", "constraint", "iterates", "implements", "using"]

━━━━━━━━━━━━━━━━━━━━
+ Log

+ text = ""

━━━━━━━━━━━━━━━━━━━━
+ Counter

+ count = 0

━━━━━━━━━━━━━━━━━━━━
+ Key

+ counter Counter
+ value Int

────────────────────
+ This
    - counter
    - value

────────────────────
+ do this == other Key

counter.count += 1
= value == other.value

━━━━━━━━━━━━━━━━━━━━
+ Tag

+ name String

────────────────────
+ This
    - name

━━━━━━━━━━━━━━━━━━━━
+ extend Tag

────────────────────
+ this == other Tag

name == other.name

━━━━━━━━━━━━━━━━━━━━
_ literal-has Bool
    - word String

["param", "constraint", "iterates", "implements", "using"] has word

━━━━━━━━━━━━━━━━━━━━
_ literal-has-not Bool
    - word String

["param", "constraint", "iterates", "implements", "using"] has not word

━━━━━━━━━━━━━━━━━━━━
_ literal-is-in Bool
    - word String

word is in (["param", "using"])

━━━━━━━━━━━━━━━━━━━━
_ literal-not-in Bool
    - word String

word not in (["param", "using"])

━━━━━━━━━━━━━━━━━━━━
_ number-has Bool
    - number Int

[3, 5, 8, 13, 21] has number

━━━━━━━━━━━━━━━━━━━━
_ folded-has Bool

[3, 5, 8] has 5

━━━━━━━━━━━━━━━━━━━━
_ computed-has Bool
    - log Log
    - word String

[tick(log, "a"), tick(log, "b"), tick(log, "c")] has tick(log, word)

━━━━━━━━━━━━━━━━━━━━
_ key-has Bool
    - counter Counter
    - value Int

[Key(counter, 1), Key(counter, 2), Key(counter, 3)] has Key(counter, value)

━━━━━━━━━━━━━━━━━━━━
_ do listed-has Bool
    - counter Counter
    - value Int

. keys = [Key(counter, 1), Key(counter, 2), Key(counter, 3)]
= keys has Key(counter, value)

━━━━━━━━━━━━━━━━━━━━
_ tag-has Bool
    - tag Tag

[Tag("a"), Tag("b")] has tag

━━━━━━━━━━━━━━━━━━━━
_ constant-has Bool
    - word String

heads has word

━━━━━━━━━━━━━━━━━━━━
_ named-has Bool
    - head String

(heads has head || probes has head) && probes has not "zeta"

.. heads = ["prop", "entry", "return"]
.. probes = ["probe", "trace"]

━━━━━━━━━━━━━━━━━━━━
_ named-is-in Bool
    - number Int

number is in primes

.. primes = [2, 3, 5, 7]

━━━━━━━━━━━━━━━━━━━━
_ measured-has Bool
    - word String

words has word || words.length == 2

.. words = ["a", "b"]

━━━━━━━━━━━━━━━━━━━━
_ do grown-has Bool
    - word String

words += word
= words has word

.. words = ["a", "b"]

━━━━━━━━━━━━━━━━━━━━
_ do count-below Int
    - limit Size

. total = 0
for index in 0 .. < limit
    total += 1
= total

━━━━━━━━━━━━━━━━━━━━
_ do count-toward Int
    - limit Int

. total = 0
for index in 0 .. < limit
    total += 1
= total

━━━━━━━━━━━━━━━━━━━━
_ do tick String
    - log Log
    - name String

log.text = "${log.text}$name"
= name
```

```output
true
false
false
true
true
false
false
true
true
false
true
true
abcb
true
2
false
5
true
7
false
true
false
true
true
false
true
false
true
true
4
4
3
```

```yumi
/core

-- loose-has-not("zeta")
-- loose-has("using")

━━━━━━━━━━━━━━━━━━━━
+ extend List''Item''Backing

────────────────────
+ do this has not item Item

= false

━━━━━━━━━━━━━━━━━━━━
_ loose-has-not Bool
    - word String

["param", "using"] has not word

━━━━━━━━━━━━━━━━━━━━
_ loose-has Bool
    - word String

["param", "using"] has word
```

```output
false
true
```

### `has not` and `not in` take an integer written with no suffix as `has` does, as the item type of a list of a narrower integer type.

```yumi
. bytes = [1u8, 2u8]
-- bytes has not 3
-- bytes has not 2
-- 3 not in bytes
-- [1u8, 2u8] has not 3
-- 2 not in [1u8, 2u8]
```

```output
true
false
true
true
false
```

### A class of the program named List leaves the list type `[I64]` and `List'I64` the core one's, since it takes no type arguments.

```yumi
. xs [I64] = [1, 2]
. ys = List'I64()
ys.add(3)
-- xs[1]
-- ys[0]
-- Thing().size

━━━━━━━━━━━━━━━━━━━━
+ List

+ size I64 = 4

━━━━━━━━━━━━━━━━━━━━
+ Thing is List
```

```output
2
3
4
```

### An item given a value its operator makes from the one it holds is found by a field of a tuple local, read once.

```yumi
-- count([1, 2, 1, 3, 1])

━━━━━━━━━━━━━━━━━━━━
+ Item is Tuple

+ crossing Index

━━━━━━━━━━━━━━━━━━━━
+ do count String
    - values [Index]

. items [Item] = List'Item()
for value in values
    items += Item(value)
. counts [Size] = List'Size()
for item in items
    while counts.length <= item.crossing
        counts += 0
    counts[item.crossing] += 1
= "$counts"
```

```output
0, 3, 1, 1
```

### A key that calls something is evaluated for the read and again for the store.

```yumi
. counter = Counter()
. values [Index] = [10, 20, 30]
values[values.length - 1] += 1
-- values
values[counter.next()] += 5
-- values
-- counter.count

━━━━━━━━━━━━━━━━━━━━
+ Counter

+ count Index = 0

────────────────────
+ do next Index

count += 1
= count - 1
```

```output
10, 20, 31
10, 15, 31
2
```

### A key cast to the type of the list's index is evaluated for the read and again for the store.

```yumi
. counts [Int] = [0, 0]
. position = 1u16
counts[position as Index] += 1
-- counts
```

```output
0, 1
```

### An item of a list given more by `+=`: a list item takes it by its own `+=`, and nothing is stored; an object item that answers only `+` is given the new object by `[]=`.

```yumi
. groups [[Index]] = [List'Index(), List'Index()]
. first Index? = 1
groups[first!] += 7
groups[0] += 3
groups[first!] += 8
-- groups[0]
-- groups[1]
. tallies [Tally] = [Tally(1), Tally(2)]
tallies[1] += 5
-- tallies[0].count, tallies[1].count

━━━━━━━━━━━━━━━━━━━━
+ Tally

+ count Index

────────────────────
+ This
    - count

────────────────────
+ this + more Index

Tally(count + more)
```

```output
3
7, 8
(1, 7)
```

### A list of pointers keeps its storage across a collection: the collector follows the storage of an array on the collected heap whatever its items are, so lists made after the collection do not take the storage over.

```yumi
/BxGC/BxGC.h

. registry = Registry()
. kept [Thing]
for index in 0 .. < 64
    . thing = Thing(index)
    kept += thing
    registry.register(thing)
C:brixGCCollect()
. fillers [[U64]]
for round in 0 .. < 200
    . filler [U64]
    for index in 0 .. < 64
        filler += 7
    fillers += filler
-- registry.matching(kept)

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ value Size

────────────────────
+ This
    - value

━━━━━━━━━━━━━━━━━━━━
+ Registry

_ identities [Pointer] = List'Pointer()

────────────────────
+ do register
    - thing Thing

identities += thing as Pointer

────────────────────
+ do matching Size
    - kept [Thing]

. found Size = 0
for identity in identities
    for thing in kept
        if thing as Pointer == identity
            found += 1
= found
```

```output
64
```

### A list written with null alone is a list of Null, which is iterated, and converted to a list of optionals.

```yumi
. nulls = [null]
for item in nulls
    -- item == null
. names [String?] = nulls
names += "a"
-- names.length
```

```output
true
2
```

### A list of a union that can hold a list is made from a list or a stream of its items, one item for each, rather than holding the whole list as one item.

```yumi
. numbers = [1, 2]
. listed = List'Value(numbers)
-- listed.length
. streamed = List'Value(each in numbers)
-- streamed.length

━━━━━━━━━━━━━━━━━━━━
+ Value is Union

| Int
| [Value]
```

```output
2
2
```

### An integer written with no suffix that is appended to a list of a named integer type is appended in place, so another name for the list sees it.

```yumi
. ids [Id]
. alias = ids
ids += 7
-- alias.length
-- alias[0]

━━━━━━━━━━━━━━━━━━━━
+ Id is U32
```

```output
1
7
```

### An unsuffixed number among a variadic list's items takes the type of the other items.

```yumi
. value F64 = 2.5
. items = List'F64(value, 7)
-- items
```

```output
2.5, 7
```

### An index below zero is out of range, for a list and for the array under it, as is the last item of an empty list.

```yumi
. list = [1, 2, 3]
. below Index = -1
try-read(list, below)
try-write(list, below)
try-last(List'Int())
try-array(Array'Int(3), below)
-- list

━━━━━━━━━━━━━━━━━━━━
+ do try-read
    - list [Int]
    - index Index

. attempt = do new
    -- list[index]
catch failure
    -- "read: $failure"
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ do try-write
    - list [Int]
    - index Index

. attempt = do new
    list[index] = 9
catch failure
    -- "write: $failure"
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ do try-last
    - list [Int]

. attempt = do new
    -- list[Position.last]
catch failure
    -- "last: $failure"
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ do try-array
    - array Array'Int
    - index Index

. attempt = do new
    array[index] = 9
catch failure
    -- "array: $failure"
attempt.wait()
```

```output
read: List index -1 out of range
write: List index -1 out of range
last: List index -1 out of range
array: Set out of bounds (-1 / 3)
1, 2, 3
```

### Popping an item off an empty list panics, and leaves the list empty.

```yumi
. list = [1]
-- list.pop()
try-pop(list)
list += 2
-- list

━━━━━━━━━━━━━━━━━━━━
+ do try-pop
    - list [Int]

. attempt = do new
    list.pop()
catch failure
    -- "pop: $failure"
attempt.wait()
```

```output
1
pop: Can't pop the last item of an empty list
2
```

### The head and tail of a list are its first and last items, never items cleared out of it.

```yumi
. list = [1, 2, 3]
list.pop()
-- "${list.head} ${list.tail}"
list.clear()
try-head(list)
try-tail(list)

━━━━━━━━━━━━━━━━━━━━
+ do try-head
    - list [Int]

. attempt = do new
    -- list.head
catch failure
    -- "head: $failure"
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ do try-tail
    - list [Int]

. attempt = do new
    -- list.tail
catch failure
    -- "tail: $failure"
attempt.wait()
```

```output
1 2
head: List index 0 out of range
tail: List index -1 out of range
```
