# Lists

```brix
(main
  (let list (call (of List I32)))
  (print (. list length))
  (blank)
  (call (. list add) 10)
  (print (. list length))
  (print ([] list 0))
  (blank)
  (call (. list add) 12)
  (print (. list length))
  (print ([] list 1))
)
```

```output
0
1
10
2
12
```

```brix
(main
  (let list (call (of List I32)))
  (print (. list length))
  (blank)
  (+= list 10)
  (print (. list length))
  (print ([] list 0))
  (blank)
  (+= list 12)
  (print (. list length))
  (print ([] list 1))
)
```

```output
0
1
10
2
12
```

```brix
(main
  (let list (call (of List Float)))
  (print (. list length))
  (blank)
  (call (. list add) 10.5)
  (print (. list length))
  (print ([] list 0))
  (blank)
  (call (. list add) 12.5)
  (print (. list length))
  (print ([] list 1))
  (blank)
  (= ([] list 1) 15.5)
  (print ([] list 1))
)
```

```output
0
1
10.5
2
12.5
15.5
```

```brix
(main
  (let thing (call Thing 10))
  (let list (call (of List Thing)))
  (print (. list length))
  (call (. list add) thing)
  (print (. list length))
  (print (. ([] list 0) a))
)

(type + Thing
  (prop + a _ (= 0))
  (fn-do + This _
    (param v _)
    (= a v)
  )
)
```

```output
0
1
10
```

```brix
(main
  (let list (list 1 2 3))
  (print ([] list 0))
  (print ([] list 1))
  (print ([] list 2))
)
```

```output
1
2
3
```

```brix
(main
  (let list (list true false true))
  (print ([] list 0))
  (print ([] list 1))
  (print ([] list 2))
)
```

```output
true
false
true
```

### One float in the list makes the whole list floats

```brix
(main
  (let list (list 1 2.5 3))
  (print ([] list 0))
  (print ([] list 1))
  (print ([] list 2))
)
```

```output
1
2.5
3
```

```brix
(main
  (let list (list (call ListSourceA) (call ListSourceB)))
  (print (. ([] list 0) value))
  (print (. ([] list 1) value))
)

(type + ListSourceA
  (fn-to-expr + ListConverted
    (call ListConverted 17)
  )
)

(type + ListSourceB
  (fn-to-expr + ListConverted
    (call ListConverted 29)
  )
)

(type + ListConverted
  (prop + value I32)
  (fn-do + This _
    (param value _)
  )
)
```

```output
17
29
```

```brix
(main
  (let seed (call list-runtime-value))
  (let list (list (+ seed 3) (* seed 2) (- seed 4)))
  (print (. list length))
  (print ([] list 0))
  (print ([] list 1))
  (print ([] list 2))
)

(fn-expr _ list-runtime-value I32
  11
)
```

```output
3
14
22
7
```

```brix
(main
  (let list (list (call Sub1) (call Sub2) (call Base)))
  (print ([] list 0))
  (print (call (. ([] list 0) a)))
  (print ([] list 1))
  (print (call (. ([] list 1) a)))
  (print ([] list 2))
  (print (call (. ([] list 2) a)))
)

(type + Base
  (fn-expr + a _
    0
  )
)

(type + Sub1 (is Base)
  (fn-expr + a _
    1
  )
)

(type + Sub2 (is Base)
  (fn-expr + a _
    2
  )
)
```

```output
[Object __source__:Base]
1
[Object __source__:Base]
2
[Object __source__:Base]
0
```

```brix
(main
  (let list (list (call Sub1) (call Sub2)))
  (print ([] list 0))
  (print (call (. ([] list 0) a)))
  (print ([] list 1))
  (print (call (. ([] list 1) a)))
)

(type + Base1
  (fn-expr + a _
    0
  )
)

(type + Base2 (is Base1)
  (fn-expr + a _
    1
  )
)

(type + Sub1 (is Base2)
  (fn-expr + a _
    2
  )
)

(type + Sub2 (is Base2)
  (fn-expr + a _
    3
  )
)
```

```output
[Object __source__:Base2]
2
[Object __source__:Base2]
3
```

```brix
(main
  (let list (list (call Sub1) (call Sub2) (call Base1)))
  (print ([] list 0))
  (print (call (. ([] list 0) a)))
  (print ([] list 1))
  (print (call (. ([] list 1) a)))
  (print ([] list 2))
  (print (call (. ([] list 2) a)))
)

(type + Base1
  (fn-expr + a _
    0
  )
)

(type + Base2 (is Base1)
  (fn-expr + a _
    1
  )
)

(type + Sub1 (is Base2)
  (fn-expr + a _
    2
  )
)

(type + Sub2 (is Base2)
  (fn-expr + a _
    3
  )
)
```

```output
[Object __source__:Base1]
2
[Object __source__:Base1]
3
[Object __source__:Base1]
0
```

```brix
(main
  (let list (list 1 2))
  (print (not list))
  (= list (call (of List I32)))
  (print (not list))
)
```

```output
false
true
```

```brix
(main
  (for (in a (list 1 2 3))
    (print a)
  )
)
```

```output
1
2
3
```

```brix
(main
  (let iter (as (list 1 2 3) (stream Int)))
  (for (in a iter)
    (print a)
  )
)
```

```output
1
2
3
```

```brix
(main
  (let l (list 1))
  (+= l 2)
  (print l)
)
```

```output
1, 2
```

```brix
(main
  (let l (list 1))
  (<-* l (list 2 3))
  (print l)
)
```

```output
1, 2, 3
```

```brix
(main
  (let l (list 1 2 3 4))
  (print (call (. l find) 3))
)
```

```output
2
```

```brix
(main
  (let l (list 1 2 3 4))
  (print (has l 3))
  (print (has-not l 3))
  (print (has l 5))
  (print (has-not l 5))
)
```

```output
true
false
false
true
```

```brix
(main
  (let list (list 1 2 3 4))
  (call (. list slide) -1)
  (print list)
  (blank)
  (= list (list 1 2 3 4))
  (call (. list slide) -3)
  (print list)
  (blank)
  (= list (list 1 2 3 4))
  (call (. list slide) 1)
  (print list)
)
```

```output
2, 3, 4
4
1, 1, 2, 3, 4
```

```brix
(main
  (let list1 (list 1 2 3))
  (let list2 (call List list1))
  (print list2)
)
```

```output
1, 2, 3
```

```brix
(main
  (let list (list 1 2 3 4 5 6))
  (= ([] list (tuple 0 4 5)) 8)
  (print list)
)
```

```output
8, 2, 3, 4, 8, 8
```

```brix
(main
  (let x (list 1 2 3 4 5 6))
  (-= ([] x (..< 3 (. Position end))))
  (print x)
)
```

```output
1, 2, 3
```

### Assigning past the end of a list panics, as reading past it does, and leaves no empty slots.

```brix
(main
  (let names (list "a"))
  (let attempt
    (do-new
      (= ([] names 2) "c")
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
  (print names)
)
```

```output
List index 2 out of range
a
```

```brix
(main
  (let attempt
    (do-new
      (let list (list 1 2 3))
      (print ([] list 3))
      (on-failure failure
        (let err (? Panic) failure)
        (print (str "Caught '" err "'"))
      )
    )
  )
  (call (. attempt wait))
)
```

```output
Caught 'List index 3 out of range'
```

```brix
(main
  (let list (list "a" "b"))
  (blank)
  (call try-read list -1)
  (call try-write list -5)
  (call try-replace list -1)
  (call try-insert list -1)
  (call try-remove list -1)
  (print list)
)

(fn-do _ try-read _
  (param list (list String))
  (param index Index)
  (let attempt
    (do-new
      (print ([] list index))
      (on-failure failure
        (print (str "read: " failure))
      )
    )
  )
  (call (. attempt wait))
)

(fn-do _ try-write _
  (param list (list String))
  (param index Index)
  (let attempt
    (do-new
      (= ([] list index) "z")
      (on-failure failure
        (print (str "write: " failure))
      )
    )
  )
  (call (. attempt wait))
)

(fn-do _ try-replace _
  (param list (list String))
  (param index Index)
  (let attempt
    (do-new
      (call (. list replace) index "z")
      (on-failure failure
        (print (str "replace: " failure))
      )
    )
  )
  (call (. attempt wait))
)

(fn-do _ try-insert _
  (param list (list String))
  (param index Index)
  (let attempt
    (do-new
      (call (. list insert) "z" index)
      (on-failure failure
        (print (str "insert: " failure))
      )
    )
  )
  (call (. attempt wait))
)

(fn-do _ try-remove _
  (param list (list String))
  (param index Index)
  (let attempt
    (do-new
      (-= ([] list index))
      (on-failure failure
        (print (str "remove: " failure))
      )
    )
  )
  (call (. attempt wait))
)
```

```output
read: List index -1 out of range
write: List index -5 out of range
replace: List range at index -1 with count 1 out of range
insert: List index -1 out of range
remove: List range at index -1 with count 1 out of range
a, b
```

```brix
(main
  (let list (list 1 2 3))
  (blank)
  (call try-counted-remove list)
  (call try-range-remove list)
  (print list)
)

(fn-do + try-counted-remove _
  (param list (list Int))
  (let attempt
    (do-new
      (call (. list remove) 1 5)
      (on-failure failure
        (print (str "counted: " failure))
      )
    )
  )
  (call (. attempt wait))
)

(fn-do + try-range-remove _
  (param list (list Int))
  (let attempt
    (do-new
      (-= ([] list (..< 2 5)))
      (on-failure failure
        (print (str "range: " failure))
      )
    )
  )
  (call (. attempt wait))
)
```

```output
counted: List range at index 1 with count 5 out of range
range: List range at index 2 with count 3 out of range
1, 2, 3
```

### Removing a range removes the items reading it selects, and reports where they started.

```brix
(import /core/collection)
(blank)

(main
  (let recorder (call Recorder))
  (let list (list 0 1 2 3 4))
  (call observe-collection list (. recorder record))
  (-= ([] list (.. 2 1)))
  (print list)
  (print (. recorder last))
  (let attempt
    (do-new
      (-= ([] list (.. 0 2 (by: 2))))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
  (print list)
  (print (. recorder count))
  (call unobserve-collection list (. recorder record))
  (let named (list 0 1 2 3 4))
  (call (. named remove) (.. 2 1))
  (print named)
  (let ascending (list 0 1 2 3 4))
  (-= ([] ascending (..< 1 3)))
  (print ascending)
  (let exclusive (list 0 1 2 3 4))
  (-= ([] exclusive (..< 3 1)))
  (print exclusive)
  (let positions (list 0 1 2 3 4))
  (-= ([] positions (.. (. Position last) (. Position middle))))
  (print positions)
  (let empty (list 0 1 2))
  (-= ([] empty (..< 3 3)))
  (print empty)
)

(type + Recorder
  (prop + count Size (= 0))
  (prop + last String (= ""))
  (fn-do + record Void
    (param mutation CollectionMutation)
    (+= count 1)
    (= last (str (. mutation kind) ":" (. mutation index) ":" (. mutation count)))
  )
)
```

```output
0, 3, 4
remove:1:2
Only increments of 1 are currently supported
0, 3, 4
1
0, 3, 4
0, 3, 4
0, 1, 4
0, 1
0, 1, 2
```

```brix
(main
  (let list (list 1 2 3))
  (blank)
  (call (. list remove) 3 0)
  (call (. list move) 3 3 0)
  (call try-zero-remove list)
  (call try-zero-move list)
  (print list)
)

(fn-do + try-zero-remove _
  (param list (list Int))
  (let attempt
    (do-new
      (call (. list remove) 99 0)
      (on-failure failure
        (print (str "remove: " failure))
      )
    )
  )
  (call (. attempt wait))
)

(fn-do + try-zero-move _
  (param list (list Int))
  (let attempt
    (do-new
      (call (. list move) 99 99 0)
      (on-failure failure
        (print (str "move: " failure))
      )
    )
  )
  (call (. attempt wait))
)
```

```output
remove: List range at index 99 with count 0 out of range
move: List range at index 99 with count 0 out of range
1, 2, 3
```

```brix
(main
  (print (is (list 1 2 3) (of List Float)))
  (print (is! (list 1 2 3) (of List Float)))
  (print (is~ (list 1 2 3) (of List Float)))
)
```

```output
false
false
true
```

### List capacity policy

```brix
(main
  (let empty (call (of List I32)))
  (print (. empty length))
  (print (. empty capacity))
  (blank)
  (let ints (call (of List I32)))
  (+= ints 1)
  (print (. ints capacity))
  (+= ints 2)
  (+= ints 3)
  (+= ints 4)
  (print (. ints capacity))
  (+= ints 5)
  (print (. ints capacity))
  (blank)
  (let large (call (of List I32)))
  (for (in (: i I32) (..< 0 1025))
    (+= large i)
  )
  (print (. large capacity))
  (blank)
  (let bytes (call (of List Byte)))
  (+= bytes 1u8)
  (print (. bytes capacity))
  (blank)
  (let one (list 42))
  (print (. one capacity))
  (blank)
  (let three (list 1 2 3))
  (print (. three capacity))
  (blank)
  (let many (list 1 2 3 4 5 6))
  (print (. many capacity))
  (blank)
  (let copy (call List three))
  (print copy)
  (print (. copy capacity))
)
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

```brix
(import /core/arena)
(blank)

(main
  (let first (call Arena))
  (let second (call Arena))
  (let first-pointer (call (of (. first allocate) U32) 1))
  (= ([] first-pointer 0) 10u32)
  (let second-pointer (call (of (. second allocate) U32) 1))
  (= ([] second-pointer 0) 20u32)
  (blank)
  (let arenas (call (of List Arena)))
  (+= arenas first)
  (+= arenas second)
  (call (. arenas replace) 0 second)
  (call (. arenas remove) 1)
  (call (. arenas clear))
  (let first-next (call (of (. first allocate) U32) 1))
  (= ([] first-next 0) 11u32)
  (print ([] first-next 0))
  (let second-next (call (of (. second allocate) U32) 1))
  (= ([] second-next 0) 21u32)
  (print ([] second-next 0))
  (blank)
  (let pointers (call (of List (of CPointer U32))))
  (for (in index (..< 0 16))
    (+= pointers first-pointer)
  )
  (call (. pointers replace) 0 second-pointer)
  (call (. pointers remove) 1 (- (. pointers length) 1))
  (print ([] ([] pointers 0) 0))
  (call (. pointers clear))
  (print ([] first-pointer 0))
  (blank)
  (call (. first release))
  (call (. second release))
)
```

```output
11
21
20
10
```

### Reserving capacity preserves contents and does not notify observers.

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
  (+= (. demo items) "a")
  (+= (. demo items) "b")
  (blank)
  (let recorder (call Recorder))
  (call observe-collection (. demo items) (. recorder record))
  (call (. demo items reserve) 1)
  (call (. demo items reserve) 16)
  (print (. demo items length))
  (print (. demo items capacity))
  (print (. demo items))
  (print (. recorder count))
  (+= (. demo items) "c")
  (print (. recorder count))
  (print (. recorder last))
  (call unobserve-collection (. demo items) (. recorder record))
)

(type + Demo (is Box)
  (prop + items (list String))
)

(type + Recorder
  (prop + count Size (= 0))
  (prop + last String (= ""))
  (fn-do + record Void
    (param mutation CollectionMutation)
    (+= count 1)
    (= last (str (. mutation kind) ":" (. mutation index) ":" (. mutation count)))
  )
)
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

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (blank)
  (let list (call (of List U32) arena))
  (for (in index (..< 0 10))
    (+= list (as index U32))
  )
  (blank)
  (let alias list)
  (= ([] alias 0) 100u32)
  (+= alias 10u32)
  (blank)
  (let slice ([] list (..< 2 5)))
  (= ([] slice 0) 22u32)
  (let plus (+ list 11u32))
  (let joined (++ list (list 11u32 12u32)))
  (blank)
  (let copy (call List list))
  (= ([] copy 0) 200u32)
  (let from-items (call (of List U32) arena (list 7u32 8u32)))
  (blank)
  (print (type-of (call (of List U32))))
  (print (type-of list))
  (print (type-of slice))
  (print (type-of plus))
  (print (type-of joined))
  (print (type-of copy))
  (print (type-of from-items))
  (print (tuple (. list length) ([] list 0) ([] list 2) (. list tail)))
  (print (tuple (. plus length) (. plus tail)))
  (print (tuple (. joined length) (. joined tail)))
  (print (tuple ([] copy 0) ([] list 0)))
  (print (tuple ([] from-items 0) ([] from-items 1)))
  (blank)
  (call (. list remove) 0 (. list length))
  (+= list 42u32)
  (let sum 0u32)
  (for (in item list)
    (+= sum item)
  )
  (print sum)
  (blank)
  (call (. list release))
  (print (tuple (. list length) (. list capacity)))
  (+= list 5u32)
  (print ([] list 0))
  (print (tuple (. GCBacking Context size) (. ArenaBacking Context size)))
)
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

```brix
(main
  (let entries (call (of List QueueEntry)))
  (+= entries (tuple (as 41 Size) 9u32))
  (print (. entries length))
  (if entries
    (print (. ([] entries 0) address))
    (print (. ([] entries 0) generation))
  )
  (blank)
  (let stack (list (tuple (: uid U32) (: class-type U16))) (list (tuple 0 0)))
  (let uid EntryUID 1010)
  (let class-type EntryClassId 17)
  (+= stack (tuple uid class-type))
  (print (. stack length))
  (print (tuple (. ([] stack 1) uid) (. ([] stack 1) class-type)))
)

(type + QueueEntry (is (tuple (: address Size) (: generation U32))))

(type + EntryUID (is U32))

(type + EntryClassId (is U16))
```

```output
1
41
9
2
(1010, 17)
```

### += and append add a collection as one item.

```brix
(main
  (let nested (call (of List (list Int))))
  (+= nested (list 1 2))
  (call (. nested append) (list 3))
  (print (. nested length))
  (print ([] nested 0))
  (blank)
  (let values (call (of List Value)))
  (let inner (call (of List Value)))
  (+= inner 1)
  (+= inner 2)
  (+= values inner)
  (call (. values append) inner)
  (print (. values length))
  (blank)
  (let entries (call (of List Value)))
  (let entry
    (call (of Map String Value)
      (block
        (entry "count" 1)
      )
    )
  )
  (+= entries entry)
  (print (. entries length))
)

(type + Value (is Union)
  (variant Bool)
  (variant Int)
  (variant (list Value))
  (variant (map String Value))
)
```

```output
2
1, 2
2
1
```

### ++= and append-all add each item in place.

```brix
(main
  (let items (list 1))
  (let alias items)
  (++= items (list 2 3))
  (call (. items append-all) (.. 4 5))
  (print alias)
  (blank)
  (let values (call (of List Value)))
  (let inner (call (of List Value)))
  (+= inner 1)
  (+= inner 2)
  (++= values inner)
  (call (. values append-all) inner)
  (print (. values length))
)

(type + Value (is Union)
  (variant Bool)
  (variant Int)
  (variant (list Value))
)
```

```output
1, 2, 3, 4, 5
4
```

### reversed yields items last to first and nothing for an empty collection.

```brix
(main
  (let items (list 1 2 3))
  (print (call (. ", " join) (call (. items reversed))))
  (let empty (call (of List Int)))
  (print (. (call (of List Int) (call (. empty reversed))) length))
  (let buffer ([] Int 3))
  (= ([] buffer 0) 4)
  (= ([] buffer 2) 6)
  (print (call (. ", " join) (call (. buffer reversed))))
  (let none ([] Int 0))
  (print (. (call (of List Int) (call (. none reversed))) length))
)
```

```output
3, 2, 1
0
6, 0, 4
0
```

### A list written holds its items in the order written, each converted to the type they join in.

```brix
(main
  (let a (list 1 2 3))
  (print (. a size))
  (print (tuple ([] a 0) ([] a 1) ([] a 2)))
  (let b (list 1.5 2 3))
  (print ([] b 1))
  (let small U8 4)
  (let c (list small 300))
  (print (tuple ([] c 0) ([] c 1)))
  (let one (list 7))
  (print (tuple (. one size) ([] one 0)))
)
```

```output
3
(1, 2, 3)
2
(4, 300)
(1, 7)
```

### A list of text, of tuples, of structs or of objects gives each item by its index, and the members of an item read that way.

```brix
(main
  (let names (list "one" "two" "three"))
  (print (tuple (. names size) ([] names 1)))
  (let pairs (list (tuple 1 "a") (tuple 2 "b")))
  (print ([] pairs 1))
  (let points (list (call Point 1 2) (call Point 5 6)))
  (print (tuple (. ([] points 1) x) (. ([] points 0) y)))
  (let boxes (list (call Box "left") (call Box "right")))
  (print (. ([] boxes 1) name))
  (print (. ([] boxes 0) name size))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)

(type + Box
  (prop + name String)
  (fn-do + This _
    (param name String)
  )
)
```

```output
(3, two)
(2, b)
(5, 2)
right
4
```

### The items of a list written are evaluated in the order written.

```brix
(main
  (let list (list (call note "first" 1) (call note "second" 2) (call note "third" 3)))
  (print (tuple ([] list 0) ([] list 1) ([] list 2)))
  (let texts (list (str "a" (call note "fourth" 4)) (str "b" (call note "fifth" 5))))
  (print (tuple ([] texts 1) ([] texts 0)))
)

(fn-expr + note Int
  (param text String)
  (param n Int)
  (before
    (print text)
  )
  n
)
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

```brix
(import /BxGC/BxGC.h)
(blank)
(prop + counted _ (= 0))
(blank)

(main
  (let names (list (str "a" (call note)) (str "b" (call note))))
  (let boxes (list (call Box (str "left" (call note))) (call Box (str "right" (call note)))))
  (let pairs (list (call Pair 1 (str "one" (call note))) (call Pair 2 (str "two" (call note)))))
  (call C:brixGCCollect)
  (let total 0)
  (for (in round (..< 0 2000))
    (let made (list (str "x" round) (str "y" round) (str "z" round)))
    (+= total (. ([] made 1) size))
    (let more (list (call Pair round (str "p" round))))
    (+= total (. ([] more 0) name size))
    (let box (call Box (str "b" round)))
    (+= total (. box name size))
  )
  (call C:brixGCCollect)
  (print total)
  (print (tuple ([] names 0) ([] names 1)))
  (print (tuple (. ([] boxes 0) name) (. ([] boxes 1) name) (. boxes size)))
  (print (tuple (. ([] pairs 0) name) (. ([] pairs 1) name) (. ([] pairs 1) id)))
)

(fn-do + note Int
  (+= counted 1)
  (return counted)
)

(type + Box
  (prop + name String)
  (fn-do + This _
    (param name String)
  )
)

(type + Pair (is Struct)
  (prop + id Int)
  (prop + name String)
)
```

```output
26670
(a1, b2)
(left3, right4, 2)
(one5, two6, 2)
```

### An index of a list is read with the list evaluated before the key, and assigned with the value evaluated before both. The assignment has the value assigned.

```brix
(prop + a _ (= (list 10 20 30)))
(blank)

(main
  (print ([] a (call next)))
  (print (tuple ([] a 0) ([] a 2)))
  (= ([] a (call key "key")) (call value "value"))
  (print (tuple ([] a 0) ([] a 1) ([] a 2)))
  (print (= ([] a 1) 44))
  (print ([] a 1))
)

(fn-do + next Int
  (= a (list 7 8 9))
  (return 1)
)

(fn-expr + key Int
  (param text String)
  (before
    (print text)
  )
  0
)

(fn-expr + value Int
  (param text String)
  (before
    (print text)
  )
  55
)
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

```brix crash
(main
  (let list (list (call Thing) (call Other)))
  (print (. list size))
)

(type + Thing)

(type + Other)
```

```output
Exception: This list mixes Thing and Other, which have no common type; convert the items to one type
<no file:3>
      (let list (list (call Thing) (call Other)))
                ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A list written with no items is refused, whatever is expected of it

```brix crash
(main
  (let list (list Int) (list))
  (print (. list size))
)
```

```output
Exception: Incompatible types in list
<no file:3>
      (let list (list Int) (list))
                           ^^^^^^
```

### A list of a struct and a number is refused

```brix crash
(main
  (let list (list (call Point 1 2) 5))
  (print (. list size))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
Exception: This list mixes Point and I64, which have no common type; convert the items to one type
<no file:3>
      (let list (list (call Point 1 2) 5))
                ^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A list printed whole: one that is empty, of one item, of texts, of floats, of lists, of objects and of tuples, and in a text.

```brix
(main
  (let empty (list Int))
  (print empty)
  (let one (list 7))
  (print one)
  (let words (list "a" "b" "c"))
  (print words)
  (print (list 1.5 2.5))
  (let nested (list (list 1 2) (list 3)))
  (print nested)
  (print (str "list: " words))
  (let things (list (call Thing 1) (call Thing 2)))
  (print things)
  (let mixed (list (tuple 1 "a") (tuple 2 "b")))
  (print mixed)
)

(type + Thing
  (prop + n Int)
  (fn-do + This _
    (param n _)
  )
  (fn-to-expr + String
    (str "T" n)
  )
)
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

```brix
(main
  (let list (list 1 2 3 4 5))
  (print ([] list (... 1)))
  (print ([] list (... _ 2)))
  (print ([] list (.. 1 3)))
  (print ([] list (..< 1 3)))
  (let tail ([] list (... 2)))
  (print (. tail length))
  (for (in item tail)
    (print item)
  )
  (let total 0)
  (for (in item list)
    (+= total item)
  )
  (print total)
)
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

### A slice ends at most at the list's last item, whatever room the list has reserved or kept after removing items.

```brix
(main
  (let empty (call (of List Int)))
  (call (. empty reserve) 4)
  (print (. ([] empty (..< 0 0)) length))
  (call try-slice empty (..< 0 2))
  (let list (list 1 2 3))
  (call (. list reserve) 8)
  (print (. ([] list (..< 3 3)) length))
  (print ([] list (..< 1 3)))
  (print ([] list (.. 2 0)))
  (call try-slice list (..< 1 4))
  (call try-slice list (.. 4 2))
  (call (. list pop))
  (print ([] list (..< 0 2)))
  (call try-slice list (..< 0 3))
  (call try-slice list (.. 3 1))
)

(fn-do + try-slice _
  (param list (list Int))
  (param range (of Range Int))
  (let attempt
    (do-new
      (print ([] list range))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)
```

```output
0
Range out of bounds (1 / 0)
0
2, 3
1, 2, 3
Range out of bounds (3 / 3)
Range out of bounds (4 / 3)
1, 2
Range out of bounds (2 / 2)
Range out of bounds (3 / 2)
```

### Adding to a list whose storage a slice shares never writes over the other's items, even after either has removed items.

```brix
(main
  (let one (list 1))
  (let first ([] one (..< 0 1)))
  (+= first 2)
  (+= one 9)
  (print first)
  (let list (list 1 2 3))
  (let head ([] list (..< 0 2)))
  (call (. head pop))
  (+= head 7)
  (print list)
  (let middle (list 1 2 3 4))
  (let inner ([] middle (..< 1 3)))
  (call (. inner pop))
  (+= inner 8)
  (print middle)
  (let parent (list 1 2))
  (let kept ([] parent (..< 0 2)))
  (call (. parent pop))
  (+= parent 9)
  (print kept)
  (let cleared (list 1 2 3))
  (let tail ([] cleared (..< 1 3)))
  (call (. cleared clear))
  (+= cleared 5)
  (print tail)
)
```

```output
1, 2
1, 2, 3
1, 2, 3, 4
1, 2
2, 3
```

### A list as a stream: looped over, joined, and given to a function that takes a stream of any item.

```brix
(main
  (let words (list "x" "y"))
  (let stream (as words (stream String)))
  (for (in word stream)
    (print word)
  )
  (print (call (. ", " join) words))
  (print (call (. "-" join) (list 1 2 3)))
  (call show words)
  (call show (list 1 2))
)

(fn-do + (of show T) _
  (param items (stream T))
  (for (in item items)
    (print (str "item " item))
  )
)
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

```brix
(main
  (let list (list 3 4 5))
  (let items (of Iterable Int) list)
  (for (in item items)
    (print item)
  )
  (let more (of Indexable Int) list)
  (for (in item more)
    (print item)
  )
  (print (tuple (. more length) ([] more 1)))
  (let sum 0)
  (for (in item (as list (of Iterable Int)))
    (+= sum item)
  )
  (print sum)
)
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

```brix
(main
  (let list (list 3 4 5))
  (print (tuple (call total list) (call total (list 1.5 2.5))))
)

(fn-do + (of total T) T
  (param items (stream T))
  (let sum T)
  (for (in item items)
    (+= sum item)
  )
  (return sum)
)
```

```output
(12, 4)
```

### A list a property holds, filled in a loop, printed, sliced and read in reverse.

```brix
(main
  (let box (call Box))
  (print (. box size))
  (call (. box fill) 4)
  (print (. box size))
  (print (. box items))
  (let slice ([] (. box items) (.. 1 2)))
  (print slice)
  (for (in item (call (. box items reversed)))
    (print item)
  )
)

(type + Box
  (prop + items (list Int))
  (fn-expr + size _ :get
    (. items length)
  )
  (fn-do + fill _
    (param n Int)
    (for (in i (..< 0 n))
      (+= items (* i i))
    )
  )
)
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

```brix
(import /core)
(blank)

(main
  (let log (call Log))
  (let counter (call Counter))
  (print (call literal-has "using"))
  (print (call literal-has "zeta"))
  (print (call literal-has-not "using"))
  (print (call literal-has-not "zeta"))
  (print (call literal-is-in "using"))
  (print (call literal-is-in "zeta"))
  (print (call literal-not-in "using"))
  (print (call literal-not-in "zeta"))
  (print (call number-has 8))
  (print (call number-has 9))
  (print (call folded-has))
  (print (call computed-has log "b"))
  (print (. log text))
  (print (call key-has counter 2))
  (print (. counter count))
  (print (call key-has counter 9))
  (print (. counter count))
  (print (call listed-has counter 2))
  (print (. counter count))
  (print (call tag-has (call Tag "b")))
  (print (call constant-has "using"))
  (print (call constant-has "zeta"))
  (print (call named-has "entry"))
  (print (call named-has "trace"))
  (print (call named-has "zeta"))
  (print (call named-is-in 7))
  (print (call named-is-in 8))
  (print (call measured-has "zeta"))
  (print (call grown-has "zeta"))
  (print (call count-below 4))
  (print (call count-toward 4))
  (print (call count-toward -3))
)

(blank)
(prop _ heads _ (= (list "param" "constraint" "iterates" "implements" "using")))

(type + Log
  (prop + text _ (= ""))
)

(type + Counter
  (prop + count _ (= 0))
)

(type + Key
  (prop + counter Counter)
  (prop + value Int)
  (fn-do + This _
    (param counter _)
    (param value _)
  )
  (fn-do + (infix this == (param other Key)) _
    (+= (. counter count) 1)
    (return (== value (. other value)))
  )
)

(type + Tag
  (prop + name String)
  (fn-do + This _
    (param name _)
  )
)

(extend + Tag
  (fn-expr + (infix this == (param other Tag)) _
    (== name (. other name))
  )
)

(fn-expr _ literal-has Bool
  (param word String)
  (has (list "param" "constraint" "iterates" "implements" "using") word)
)

(fn-expr _ literal-has-not Bool
  (param word String)
  (has-not (list "param" "constraint" "iterates" "implements" "using") word)
)

(fn-expr _ literal-is-in Bool
  (param word String)
  (is-in word (list "param" "using"))
)

(fn-expr _ literal-not-in Bool
  (param word String)
  (not-in word (list "param" "using"))
)

(fn-expr _ number-has Bool
  (param number Int)
  (has (list 3 5 8 13 21) number)
)

(fn-expr _ folded-has Bool
  (has (list 3 5 8) 5)
)

(fn-expr _ computed-has Bool
  (param log Log)
  (param word String)
  (has (list (call tick log "a") (call tick log "b") (call tick log "c")) (call tick log word))
)

(fn-expr _ key-has Bool
  (param counter Counter)
  (param value Int)
  (has (list (call Key counter 1) (call Key counter 2) (call Key counter 3)) (call Key counter value))
)

(fn-do _ listed-has Bool
  (param counter Counter)
  (param value Int)
  (let keys (list (call Key counter 1) (call Key counter 2) (call Key counter 3)))
  (return (has keys (call Key counter value)))
)

(fn-expr _ tag-has Bool
  (param tag Tag)
  (has (list (call Tag "a") (call Tag "b")) tag)
)

(fn-expr _ constant-has Bool
  (param word String)
  (has heads word)
)

(fn-expr _ named-has Bool
  (param head String)
  (and (or (has heads head) (has probes head)) (has-not probes "zeta"))
  (where
    (lazy heads (list "prop" "entry" "return"))
    (lazy probes (list "probe" "trace"))
  )
)

(fn-expr _ named-is-in Bool
  (param number Int)
  (is-in number primes)
  (where
    (lazy primes (list 2 3 5 7))
  )
)

(fn-expr _ measured-has Bool
  (param word String)
  (or (has words word) (== (. words length) 2))
  (where
    (lazy words (list "a" "b"))
  )
)

(fn-do _ grown-has Bool
  (param word String)
  (+= words word)
  (return (has words word))
  (blank)
  (lazy words (list "a" "b"))
)

(fn-do _ count-below Int
  (param limit Size)
  (let total 0)
  (for (in index (..< 0 limit))
    (+= total 1)
  )
  (return total)
)

(fn-do _ count-toward Int
  (param limit Int)
  (let total 0)
  (for (in index (..< 0 limit))
    (+= total 1)
  )
  (return total)
)

(fn-do _ tick String
  (param log Log)
  (param name String)
  (= (. log text) (str (. log text) name))
  (return name)
)
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

```brix
(import /core)
(blank)

(main
  (print (call loose-has-not "zeta"))
  (print (call loose-has "using"))
)

(extend + (of List ''Item ''Backing)
  (fn-do + (infix this has-not (param item Item)) _
    (return false)
  )
)

(fn-expr _ loose-has-not Bool
  (param word String)
  (has-not (list "param" "using") word)
)

(fn-expr _ loose-has Bool
  (param word String)
  (has (list "param" "using") word)
)
```

```output
false
true
```

### `has not` and `not in` take an integer written with no suffix as `has` does, as the item type of a list of a narrower integer type.

```brix
(main
  (let bytes (list 1u8 2u8))
  (print (has-not bytes 3))
  (print (has-not bytes 2))
  (print (not-in 3 bytes))
  (print (has-not (list 1u8 2u8) 3))
  (print (not-in 2 (list 1u8 2u8)))
)
```

```output
true
false
true
true
false
```

### A class of the program named List leaves the list type `[I64]` and `List'I64` the core one's, since it takes no type arguments.

```brix
(main
  (let xs (list I64) (list 1 2))
  (let ys (call (of List I64)))
  (call (. ys add) 3)
  (print ([] xs 1))
  (print ([] ys 0))
  (print (. (call Thing) size))
)

(type + List
  (prop + size I64 (= 4))
)

(type + Thing (is List))
```

```output
2
3
4
```

### An item given a value its operator makes from the one it holds is found by a field of a tuple local, read once.

```brix
(main
  (print (call count (list 1 2 1 3 1)))
)

(type + Item (is Tuple)
  (prop + crossing Index)
)

(fn-do + count String
  (param values (list Index))
  (let items (list Item) (call (of List Item)))
  (for (in value values)
    (+= items (call Item value))
  )
  (let counts (list Size) (call (of List Size)))
  (for (in item items)
    (while (<= (. counts length) (. item crossing))
      (+= counts 0)
    )
    (+= ([] counts (. item crossing)) 1)
  )
  (return (str counts))
)
```

```output
0, 3, 1, 1
```

### A key that calls something is evaluated for the read and again for the store.

```brix
(main
  (let counter (call Counter))
  (let values (list Index) (list 10 20 30))
  (+= ([] values (- (. values length) 1)) 1)
  (print values)
  (+= ([] values (call (. counter next))) 5)
  (print values)
  (print (. counter count))
)

(type + Counter
  (prop + count Index (= 0))
  (fn-do + next Index
    (+= count 1)
    (return (- count 1))
  )
)
```

```output
10, 20, 31
10, 15, 31
2
```

### A key cast to the type of the list's index is evaluated for the read and again for the store.

```brix
(main
  (let counts (list Int) (list 0 0))
  (let position 1u16)
  (+= ([] counts (as position Index)) 1)
  (print counts)
)
```

```output
0, 1
```

### An item of a list given more by `+=`: a list item takes it by its own `+=`, and nothing is stored; an object item that answers only `+` is given the new object by `[]=`.

```brix
(main
  (let groups (list (list Index)) (list (call (of List Index)) (call (of List Index))))
  (let first (? Index) 1)
  (+= ([] groups (!! first)) 7)
  (+= ([] groups 0) 3)
  (+= ([] groups (!! first)) 8)
  (print ([] groups 0))
  (print ([] groups 1))
  (let tallies (list Tally) (list (call Tally 1) (call Tally 2)))
  (+= ([] tallies 1) 5)
  (print (tuple (. ([] tallies 0) count) (. ([] tallies 1) count)))
)

(type + Tally
  (prop + count Index)
  (fn-do + This _
    (param count _)
  )
  (fn-expr + (infix this + (param more Index)) _
    (call Tally (+ count more))
  )
)
```

```output
3
7, 8
(1, 7)
```

### A list of pointers keeps its storage across a collection: the collector follows the storage of an array on the collected heap whatever its items are, so lists made after the collection do not take the storage over.

```brix
(import /BxGC/BxGC.h)
(blank)

(main
  (let registry (call Registry))
  (let kept (list Thing))
  (for (in index (..< 0 64))
    (let thing (call Thing index))
    (+= kept thing)
    (call (. registry register) thing)
  )
  (call C:brixGCCollect)
  (let fillers (list (list U64)))
  (for (in round (..< 0 200))
    (let filler (list U64))
    (for (in index (..< 0 64))
      (+= filler 7)
    )
    (+= fillers filler)
  )
  (print (call (. registry matching) kept))
)

(type + Thing
  (prop + value Size)
  (fn-do + This _
    (param value _)
  )
)

(type + Registry
  (prop _ identities (list Pointer) (= (call (of List Pointer))))
  (fn-do + register _
    (param thing Thing)
    (+= identities (as thing Pointer))
  )
  (fn-do + matching Size
    (param kept (list Thing))
    (let found Size 0)
    (for (in identity identities)
      (for (in thing kept)
        (if (== (as thing Pointer) identity)
          (+= found 1)
        )
      )
    )
    (return found)
  )
)
```

```output
64
```

### A list written with null alone is a list of Null, which is iterated, and converted to a list of optionals.

```brix
(main
  (let nulls (list null))
  (for (in item nulls)
    (print (== item null))
  )
  (let names (list (? String)) nulls)
  (+= names "a")
  (print (. names length))
)
```

```output
true
2
```

### A list of a union that can hold a list is made from a list or a stream of its items, one item for each, rather than holding the whole list as one item.

```brix
(main
  (let numbers (list 1 2))
  (let listed (call (of List Value) numbers))
  (print (. listed length))
  (let streamed (call (of List Value) (in each numbers)))
  (print (. streamed length))
)

(type + Value (is Union)
  (variant Int)
  (variant (list Value))
)
```

```output
2
2
```

### An integer written with no suffix that is appended to a list of a named integer type is appended in place, so another name for the list sees it.

```brix
(main
  (let ids (list Id))
  (let alias ids)
  (+= ids 7)
  (print (. alias length))
  (print ([] alias 0))
)

(type + Id (is U32))
```

```output
1
7
```

### An unsuffixed number among a variadic list's items takes the type of the other items.

```brix
(main
  (let value F64 2.5)
  (let items (call (of List F64) value 7))
  (print items)
)
```

```output
2.5, 7
```

### An index below zero is out of range, for a list and for the array under it, as is the last item of an empty list.

```brix
(main
  (let list (list 1 2 3))
  (let below Index -1)
  (call try-read list below)
  (call try-write list below)
  (call try-last (call (of List Int)))
  (call try-array (call (of Array Int) 3) below)
  (print list)
)

(fn-do + try-read _
  (param list (list Int))
  (param index Index)
  (let attempt
    (do-new
      (print ([] list index))
      (on-failure failure
        (print (str "read: " failure))
      )
    )
  )
  (call (. attempt wait))
)

(fn-do + try-write _
  (param list (list Int))
  (param index Index)
  (let attempt
    (do-new
      (= ([] list index) 9)
      (on-failure failure
        (print (str "write: " failure))
      )
    )
  )
  (call (. attempt wait))
)

(fn-do + try-last _
  (param list (list Int))
  (let attempt
    (do-new
      (print ([] list (. Position last)))
      (on-failure failure
        (print (str "last: " failure))
      )
    )
  )
  (call (. attempt wait))
)

(fn-do + try-array _
  (param array (of Array Int))
  (param index Index)
  (let attempt
    (do-new
      (= ([] array index) 9)
      (on-failure failure
        (print (str "array: " failure))
      )
    )
  )
  (call (. attempt wait))
)
```

```output
read: List index -1 out of range
write: List index -1 out of range
last: List index -1 out of range
array: Set out of bounds (-1 / 3)
1, 2, 3
```

### Popping an item off an empty list panics, and leaves the list empty.

```brix
(main
  (let list (list 1))
  (print (call (. list pop)))
  (call try-pop list)
  (+= list 2)
  (print list)
)

(fn-do + try-pop _
  (param list (list Int))
  (let attempt
    (do-new
      (call (. list pop))
      (on-failure failure
        (print (str "pop: " failure))
      )
    )
  )
  (call (. attempt wait))
)
```

```output
1
pop: Can't pop the last item of an empty list
2
```

### The head and tail of a list are its first and last items, never items cleared out of it.

```brix
(main
  (let list (list 1 2 3))
  (call (. list pop))
  (print (str (. list head) " " (. list tail)))
  (call (. list clear))
  (call try-head list)
  (call try-tail list)
)

(fn-do + try-head _
  (param list (list Int))
  (let attempt
    (do-new
      (print (. list head))
      (on-failure failure
        (print (str "head: " failure))
      )
    )
  )
  (call (. attempt wait))
)

(fn-do + try-tail _
  (param list (list Int))
  (let attempt
    (do-new
      (print (. list tail))
      (on-failure failure
        (print (str "tail: " failure))
      )
    )
  )
  (call (. attempt wait))
)
```

```output
1 2
head: List index 0 out of range
tail: List index -1 out of range
```

### A list releases the items it removes unless a slice sharing its storage still holds them.

```brix
(import /core/gc)
(blank)
(import /BxGC/BxGC.h)
(blank)

(main
  (let emptied (call emptied-list))
  (let sliced (call sliced-list))
  (call C:brixCollect)
  (print (tuple (. ([] emptied 0) length) (== (. ([] emptied 1) get) null) (== (. ([] emptied 2) get) null) (== (. ([] emptied 3) get) null)))
  (print (tuple (. ([] sliced 0) length) (. ([] ([] sliced 1) 0) value)))
)

(fn-do + emptied-list _
  (let list (call (of List Cell)))
  (for (in i (..< 0 4))
    (+= list (call Cell i))
  )
  (let popped (call Weak ([] list 3)))
  (let ranged (call Weak ([] list 2)))
  (let cleared (call Weak ([] list 0)))
  (call (. list pop))
  (call (. list remove) (..< 1 3))
  (call (. list clear))
  (return (tuple list popped ranged cleared))
)

(fn-do + sliced-list _
  (let list (call (of List Cell)))
  (+= list (call Cell 1))
  (+= list (call Cell 2))
  (let slice ([] list (..< 1 2)))
  (call (. list clear))
  (return (tuple list slice))
)

(type + Cell
  (prop + value _ (= 0))
  (fn-do + This _
    (param v Int)
    (= value v)
  )
)
```

```output
(0, true, true, true)
(0, 2)
```

### A list item popped during a mark survives that cycle.

```brix
(import /BxGC/BxGCTest.h)
(blank)

(main
  (let list (call filled-list 7))
  (call C:brixGCStartConcurrentMark)
  (let popped (call (. list pop)))
  (call C:brixGCFinishConcurrentMark)
  (call C:brixCollect)
  (print (if-expr (call C:brixGCVerifyObject (as popped Pointer)) (str "alive " (. popped value)) "lost"))
)

(fn-do + filled-list (of List Cell)
  (param value Int)
  (let list (call (of List Cell)))
  (+= list (call Cell value))
  (return list)
)

(type + Cell
  (prop + value _ (= 0))
  (fn-do + This _
    (param v Int)
    (= value v)
  )
)
```

```output
alive 7
```

### A string in a tuple popped from a list during a mark survives that cycle.

```brix
(import /BxGC/BxGCTest.h)
(blank)

(main
  (let list (call filled-list 9))
  (call C:brixGCStartConcurrentMark)
  (let popped (call (. list pop)))
  (call C:brixGCFinishConcurrentMark)
  (call C:brixCollect)
  (let text ([] popped 1))
  (let start (as (- (. (as text Pointer) address) 16) Pointer))
  (print (if-expr (call C:brixGCVerifyObject start) (str "alive " text) "lost"))
)

(fn-do + filled-list (list (tuple Int String))
  (param value Int)
  (let list (list (tuple Int String)))
  (+= list (tuple value (str "item " value)))
  (return list)
)
```

```output
alive item 9
```
