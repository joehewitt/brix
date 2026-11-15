# Collections

A collection holds many values. Yumi's core library has five kinds:

| Kind | Type | Holds |
| --- | --- | --- |
| [List](#lists) | `(list Int)`{.brix} | Items in order, growing and shrinking as items are added and removed. |
| [Map](#maps) | `(map String Int)`{.brix} | Values found by key. |
| [Array](#arrays) | `([] Int)`{.brix} | A fixed number of items in one block of memory. |
| [Table](#tables) | `(of Table Point)`{.brix} | Rows of a struct type, stored as one array for each field. |
| [Bitmap](#bitmaps) | `(of Bitmap U8)`{.brix} | Bits packed into an integer or a buffer. |

Lists, maps and arrays need no import. Ranges, which are values as well as loop headers, are in
[Ranges as values](#ranges-as-values). There is no set type.

Streams, which give their items one at a time, are in Streams, and so are the interfaces a type
implements to be looped over. List patterns are in Patterns.

## Lists

```brix
(main
  (let primes (list 2 3 5))
  (print primes)
  (print (. primes length))
  (print ([] primes 0))
  (print ([] primes .last))
  (+= primes 7)
  (++= primes (list 11 13))
  (print primes)
  (print (has primes 5))
  (print (is-in 4 primes))
  (print ([] primes (..< 1 3)))
  (print (+ primes 17))
  (print (. primes length))
)
```

```output
2, 3, 5
3
2
5
2, 3, 5, 7, 11, 13
true
false
3, 5
2, 3, 5, 7, 11, 13, 17
6
```

- **Literals.** `(list 1 2 3)`{.brix} makes a list. Its item type is the join of its items, so
  `(list 1 2.5)`{.brix} is a list of `Float`; see Conversions. Items with no common type are an
  error: `This list mixes Thing and Other, which have no common type; convert the items to one
  type`. The items are evaluated in the order written.
- **Types.** `(list Int)`{.brix} is a list of `Int`, also written `(of List Int)`{.brix}.
- **Empty lists.** A list literal must have an item: `(list)`{.brix} is an error, `Incompatible
  types in list`. `(call (of List Int))`{.brix} makes an empty list, and so does a declaration
  with a type and no value, `(let empty (list Int))`{.brix}. A property of a list type starts
  empty.
- **Declared item types.** A literal written where a list type is declared makes each item that
  type, so `(let values (list Float) (list 1 2))`{.brix} holds `Float` items, and a list of a
  union may mix its cases; see Unions.
- **Conversions.** A list converts on its own to a list of another item type when its items do,
  as `(list I32)`{.brix} does to `(list Int)`{.brix}, and the result is a copy.
- **Printing** writes the items separated by `, `. A nested list prints its items the same way,
  so `(list (list 1 2) (list 3))`{.brix} prints as `1, 2, 3`.
- **Conditions.** A list is true when it is not empty.

### Reading items

| Operation | Value |
| --- | --- |
| `([] xs index)`{.brix} | The item at `index`, counting from 0. |
| `([] xs .last)`{.brix} | The item at a `Position`: `.first`, `.middle`, `.last` or `(call .index n)`{.brix}. |
| `([] xs range)`{.brix} | A list of the items in a range; see [Slices](#slices). |
| `(?[] xs index)`{.brix} | `null` when `xs` is `null`, and the item otherwise; see Optionals. |
| `(. xs length)`{.brix} | The number of items, a `Size`. |
| `(call (. xs find) item)`{.brix} | The index of the first equal item, or `null`. |
| `(has xs item)`{.brix}, `(is-in item xs)`{.brix} | Whether an equal item is in the list. `has-not` and `not-in` are their opposites. |
| `(call (. xs reversed))`{.brix} | A stream of the items from last to first. |
| `(== xs ys)`{.brix} | Whether the lists have equal items in the same order. |

- **Out of range.** An index below zero, or at or past the length, panics, when reading and when
  assigning: `List index 3 out of range`. A list grows only by adding items.
- **Index types.** An index may be any signed integer type, or `U8`, `U16` or `U32`.
- **Equality** compares items with their type's `==`, including one a type declares.

### Changing a list

```brix
(main
  (let items (list "a" "b" "c"))
  (call (. items append) "d")
  (call (. items insert) "x" 1)
  (print items)
  (= ([] items 0) "A")
  (-= ([] items 1))
  (print items)
  (print (-= items "c"))
  (print (-= items "q"))
  (print (call (. items pop)))
  (print items)
  (<- items "e")
  (call items "f")
  (print items)
  (call (. items remove) 0 2)
  (print items)
  (call (. items clear))
  (print (. items length))
)
```

```output
a, x, b, c, d
A, b, c, d
2
unknown
d
A, b
A, b, e, f
e, f
0
```

| Operation | Effect |
| --- | --- |
| `(+= xs item)`{.brix}, `(call (. xs append) item)`{.brix}, `(<- xs item)`{.brix}, `(call xs item)`{.brix} | Adds one item at the end. |
| `(++= xs items)`{.brix}, `(call (. xs append-all) items)`{.brix}, `(<-* xs items)`{.brix} | Adds each item of a stream at the end. |
| `(call (. xs insert) item index)`{.brix} | Inserts one item before `index`; the item comes first. |
| `(= ([] xs index) item)`{.brix} | Replaces the item at `index`. |
| `(-= ([] xs index))`{.brix} | Removes the item at `index`. |
| `(-= ([] xs range))`{.brix} | Removes the items in a range. |
| `(-= xs item)`{.brix} | Removes the first equal item, and gives the index it had, or `null`. |
| `(call (. xs remove) index count)`{.brix} | Removes `count` items from `index`; `count` defaults to 1. |
| `(call (. xs pop))`{.brix}, `(call (. xs pop-first))`{.brix} | Removes and gives the last or the first item. |
| `(call (. xs move) from to count)`{.brix} | Moves `count` items from one index to another. |
| `(call (. xs clear))`{.brix} | Removes every item, keeping the storage. |
| `(call (. xs reserve) count)`{.brix} | Makes room for `count` items. |

- **One item or many.** `+=`, `append`, `<-` and calling the list add their value as one item,
  even when it is a list. `++=`, `append-all` and `<-*` add each item of their value.
- **Calling a list** with an optional adds the value only when it is not `null`, and with a
  stream adds each item.
- **The value** of `(-= ([] xs index))`{.brix} and of `<-` is the list, and the value of an
  assignment is the item assigned.
- **Bounds.** Removing or inserting at an index past the end panics, as in `List range at index 3
  with count 1 out of range`.

### Sharing and copying

A list is shared, not copied: assigning a list or passing it to a function gives another name for
the same list, and a change through one name shows through every name.

```brix
(main
  (let first (list 1 2))
  (let second first)
  (+= second 3)
  (print first)
  (let copied (call List first))
  (+= copied 4)
  (print first)
  (print copied)
  (let longer (+ first 9))
  (let joined (++ first (list 7 8)))
  (print first)
  (print longer)
  (print joined)
)
```

```output
1, 2, 3
1, 2, 3
1, 2, 3, 4
1, 2, 3
1, 2, 3, 9
1, 2, 3, 7, 8
```

These make a new list and leave the old one alone: `(call List xs)`{.brix},
`(call (of List Int) xs)`{.brix}, `(+ xs item)`{.brix}, `(++ xs items)`{.brix}, and a conversion
to another item type. `+` adds one item, and `++` adds each item of any stream.

### Slices

`([] xs range)`{.brix} gives a list of the items in the range, which may use `Position`s and open
ends, as in `([] xs (... 1))`{.brix}. A range that counts down gives the same items as the one
that counts up. A range past the end panics: `Range out of bounds (9 / 3)`. A range with a step
other than 1 panics: `Only increments of 1 are currently supported`.

A slice starts out sharing the list's storage, so assigning an item of the slice changes the
list. Adding to the slice gives it storage of its own.

### Sorting and other stream functions

Lists have no sort of their own. `/stream` has `sort`, which takes any stream and gives a new
list, and many other functions that take a stream, such as `sum`, `min`, `max`, `distinct` and
`zip`. See Streams and the standard library.

```brix
(import /stream)

(main
  (let scores (list 30 10 20))
  (print (call sort scores))
  (print (call sort scores (:is-reverse true)))
  (print scores)
  (print (call sum scores))
)
```

```output
10, 20, 30
30, 20, 10
30, 10, 20
60
```

## Maps

```brix
(main
  (let ages (map String Int))
  (= ([] ages "ada") 36)
  (= ([] ages "alan") 41)
  (print ([] ages "ada"))
  (print ([] ages "grace"))
  (print (?? ([] ages "grace") 0))
  (print (has ages "alan"))
  (print (. ages length))
  (-= ([] ages "alan"))
  (print (has ages "alan"))
  (print ages)
  (let sizes (call Map (block (:small 1) (:large 3) (entry "extra large" 4))))
  (print (!! ([] sizes "extra large")))
)
```

```output
36
unknown
0
true
2
false
[ada = 36]
4
```

- **Types.** `(map String Int)`{.brix} is a map from `String` keys to `Int` values, also written
  `(of Map String Int)`{.brix}. `(call (of Map String Int))`{.brix} makes an empty map, and so
  does a declaration with a type and no value.
- **Literals.** A call to `Map` whose arguments are entries makes a map, as `sizes` shows. A named
  argument such as
  `(:small 1)`{.brix} uses the name as a `String` key. `(entry key value)`{.brix} takes any key,
  and `(entry? key value)`{.brix} adds its entry only when the value is not `null`. The key and
  value types are the joins of the entries'.
- **Pairs.** `(call (of Map String Int) (tuple "a" 1) (tuple "b" 2))`{.brix} makes a map from
  pairs, and `(call Map pairs)`{.brix} from a stream of them. A later pair with the same key wins.
- **Reading** a key gives an optional value, `null` when the key is missing; see Optionals.
- **Writing** a key adds the entry or replaces its value. `(-= ([] m key))`{.brix} removes an
  entry, and does nothing when the key is missing.
- **Printing** writes `[key = value, …]`. The order of the entries is not defined.

| Operation | Value |
| --- | --- |
| `([] m key)`{.brix} | The value for `key`, or `null`. |
| `(= ([] m key) value)`{.brix} | Stores the value. |
| `(-= ([] m key))`{.brix} | Removes the entry. |
| `(has m key)`{.brix}, `(is-in key m)`{.brix} | Whether the key is present, whatever its value. |
| `(. m length)`{.brix} | The number of entries, a `U32`. |
| `(. m keys)`{.brix}, `(. m values)`{.brix}, `(. m items)`{.brix} | Streams of the keys, the values, and the entries as `KeyValuePair`s. |
| `(call (. m find) value)`{.brix} | A stream of the keys whose value equals `value`. |
| `(call (. m get-or-init) key)`{.brix} | The value for `key`, storing a zero value first when the key is missing. |
| `(+ m other)`{.brix} | A new map with the entries of both; `other` wins where both have a key. |
| `(- m other)`{.brix} | A new map without the keys of `other`. |
| `(+= m other)`{.brix}, `(-= m other)`{.brix} | The same, changing `m`. `(+= m (tuple key value))`{.brix} adds one entry. |
| `(call (. m clear))`{.brix} | Removes every entry. |
| `(not m)`{.brix} | `true` when the map is empty. |

### Looping over a map

```brix
(import /stream)

(main
  (let stock (call Map (:apples 3) (:pears 5) (:plums 2)))
  (let total 0)
  (for (in (tuple name count) stock)
    (+= total count)
  )
  (print total)
  (print (call sort (. stock keys)))
  (let plenty 0)
  (for (in pair stock (when (> (. pair value) 2)))
    (+= plenty 1)
  )
  (print plenty)
)
```

```output
10
apples, pears, plums
2
```

- **Entries.** A loop over a map gives each entry as a `KeyValuePair`, with `key` and `value`,
  which a tuple pattern takes apart.
- **Order** is not defined, and is not the order the entries were added. Sort the keys to visit
  them in order.
- **No changes while looping.** Changing a map while a loop goes through it panics, even when
  only a value changes: `Map mutated during iteration`.

### Keys

- **Key types.** Text, atoms, every number type, `Bool`, `Char` and C pointers can be keys.
- **Classes** are keys by identity: two objects are different keys even when `==` says they are
  equal.
- **Hashable.** Any type can be a key by implementing `Hashable`: a `hash` property of type `Hash`
  and an `==`. The core function `hash-for` hashes text, and helps build one.

```brix
(main
  (let seen (map Point String))
  (= ([] seen (call Point 1 2)) "first")
  (print ([] seen (call Point 1 2)))
)

(type + Point (is (tuple (: x Int) (: y Int))) (implements Hashable)
  (fn-expr + hash Hash :get
    (+ (* (as x Hash) 31) (as y Hash))
  )
  (fn-expr + (infix this == (param other Point)) Bool
    (and (== x (. other x)) (== y (. other y)))
  )
)
```

```output
first
```

### Sharing and equality

A map is shared like a list: assigning it gives another name for the same map. `+` and `-` make
new maps, and `(call Map (. m items))`{.brix} copies one.

Maps have no `==` of their own, so two maps are equal only when they are the same map.

## Arrays

An array is a fixed number of items stored together in one block of memory. Arrays are for
large amounts of data, binary formats and native code; a list suits most other uses.

```brix
(main
  (let counts ([] Int 4))
  (= ([] counts 1) 5)
  (+= ([] counts 3) 9)
  (print (call ", " counts))
  (print counts)
  (let squares (call (of Array Int) 4 (:with (* each each))))
  (print (call ", " squares))
  (let primes (call (of Array Int) (list 2 3 5 7 11)))
  (let middle ([] primes (..< 1 3)))
  (= ([] middle 0) 30)
  (print (call ", " primes))
  (print (has primes 7))
  (print (call (. primes find) 11))
  (let listed (list Int) primes)
  (print listed)
)
```

```output
0, 5, 0, 9
I64[4]
0, 1, 4, 9
2, 30, 5, 7, 11
true
4
2, 30, 5, 7, 11
```

- **Types.** `([] Int)`{.brix} is an array of `Int`, also written `(of Array Int)`{.brix}.
- **Making one.** `([] Int 4)`{.brix} makes four zero items, as does
  `(call (of Array Int) 4)`{.brix}. With a named argument `with`, each item is the value of a
  function of its index, read as `each`. `(call (of Array Int) items)`{.brix} copies a list,
  `(call (of Array Char) text)`{.brix} holds the characters of a text, and
  `(call (of Array Byte) text)`{.brix} its UTF-8 bytes.
- **Fixed length.** An array never grows. `resize` and `grow` give a new array, and `+=` does not
  add an item.
- **Sharing.** Assigning an array shares its storage. `(call (. a copy))`{.brix} makes an array
  with storage of its own.
- **Casting.** `(as a ([] U32))`{.brix} reads the same storage as items of another type, so
  writing one writes the other. A cast keeps the count of items, so their sizes must match:
  `Array'I32 cannot be cast to Array'I64; their items are 4 and 8 bytes, and a cast keeps the
  count of items`.
- **Out of range** panics: `Index out of bounds (4 / 4)` when reading, and `Set out of bounds (4 /
  4)` when writing.
- **Printing** an array writes its type and length, as in `I64[4]`. Join it to print its items,
  as in `(call ", " counts)`{.brix}.
- **Slices** are arrays over the same storage, so writing one writes the other.
- **Lists.** An array converts to a list on its own, as a copy. A list becomes an array only by
  the constructor: `(as (list 1 2) ([] Int))`{.brix} is an error.

| Operation | Value |
| --- | --- |
| `([] a index)`{.brix}, `(= ([] a index) value)`{.brix} | Reads or writes an item. A `Position` also works. |
| `(= ([] a range) value)`{.brix} | Writes a value, an array or a list into a range. |
| `(. a length)`{.brix} | The number of items. |
| `(has a item)`{.brix}, `(is-in item a)`{.brix} | Whether an equal item is in the array. |
| `(call (. a find) item)`{.brix} | The index of the first equal item, or of an array or list of items, or `null`. |
| `(call (. a starts-with) items)`{.brix}, `(call (. a ends-with) items)`{.brix} | Whether the array begins or ends with those items. |
| `(call (. a fill) value)`{.brix} | Sets every item, and gives the array. |
| `(call (. a copy))`{.brix} | An array with its own storage. |
| `(call (. a resize) length)`{.brix}, `(call (. a grow) count)`{.brix} | A new array of another length, keeping the items that fit. |
| `(call (. a map) function)`{.brix} | A new array of the function's values, given each item and its index. |
| `(-= ([] a index))`{.brix}, `(-= a item)`{.brix} | Removes an item, storing a new, shorter array. |
| `(+ a count)`{.brix} | The array without its first `count` items, over the same storage. |
| `(. a immutable)`{.brix} | A read-only view of the array. |
| `(== a b)`{.brix} | Whether the items are equal, against another array or a list. |

### Byte arrays

An array of `Byte` reads and writes numbers in binary formats, and converts to and from text.

```brix
(main
  (let bytes ([] Byte 4))
  (call (of (. bytes write) U16) 513 0)
  (print (call (of (. bytes read) U16) 0))
  (print (as bytes Hex))
  (let text (as "hé" ([] Byte)))
  (print (. text length))
  (print (call (. text slice-string)))
  (print (call (. (as (as "aGk=" Base64) ([] Byte)) slice-string)))
)
```

```output
513
02010000
3
hé
hi
```

- **`read`** reads a number of the type given after it at a byte offset, as in
  `(call (of (. bytes read) U16) 0)`{.brix}, and gives `null` past the end. **`write`** writes one. Both take `.little` or `.big` as a third argument;
  the default is the machine's order.
- **Text.** `(as text ([] Byte))`{.brix} gives a copy of the UTF-8 bytes of a text, and
  `slice-string` reads bytes as text. `as Hex` and `as Base64` write the bytes as text, and
  `(as (as text Hex) ([] Byte))`{.brix} reads them back.

### Immutable arrays and builders

`(. a immutable)`{.brix} is an `ImmutableArray`: a view of the same storage that cannot be
written, so later writes to `a` show through it. Writing an item through it is an error:
`ImmutableArray does not support element assignment`.

An `ArrayBuilder`, from `/core/array-builder`, collects items and then gives them up as an array.
That module also lets `Array` and `ImmutableArray` be made from a stream.

```brix
(import /core/array-builder)

(main
  (let builder (call (of ArrayBuilder Int)))
  (call (. builder append) 1)
  (call (. builder append-all) (list 2 3))
  (print (. builder length))
  (let done (call (. builder finish-immutable)))
  (print done)
  (print (. builder length))
  (print (call ", " (call Array (*> (.. 1 3) (* each 10)))))
)
```

```output
3
1, 2, 3
0
10, 20, 30
```

- **Finishing.** `finish-immutable` and `finish-mutable` give the items as an `ImmutableArray` or
  an `Array`, and leave the builder empty for reuse. Their `-compacted` forms also trim unused
  room.
- **Members.** `append`, `append-all`, `reserve`, `length`, `([] b index)`{.brix}, `clear`, and
  `mark` and `rewind` to go back to an earlier length.

### Views of struct arrays

An array of a struct or tuple type has two views. `rows` gives each item as a row that reads and
writes the array, and `fields` gives each field as a column.

```brix
(main
  (let points ([] Point 2))
  (= ([] points 0) (call Point 1 2))
  (for (in row (. points rows))
    (+= (. row x) 10)
  )
  (print (call ", " (. points fields x)))
  (= ([] (. points fields y) 1) 40)
  (print (as ([] (. points rows) 1) Point))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
11, 10
(10, 40)
```

- **Live.** A row or a column reads and writes the array itself. `(as row Point)`{.brix} gathers
  a copy.
- **Stale views.** A view panics with `Stale array view` once the array has moved: after a
  resize, after `-=`, or after the variable that held it is assigned another array.
- **Read-only.** The views of an immutable array cannot be written: `Cannot write through a
  read-only array view`.
- **Copies of items.** An item read from an array is a copy, so changing a field of it leaves the
  array alone. Write through `rows` or assign the whole item.

`ConstArray` holds constant data compiled into the program, for native code; see Native code and
memory.

## Tables

A table, from `/core/table`, holds rows of a struct type, and stores each field as its own array.
Reading one field of every row reads one block of memory.

```brix
(import /core/arena)
(import /core/table)

(main
  (let points (call (of Table Point) 3 (:with (call Point (as each I32) (as (* each 10) I32)))))
  (print (. points length))
  (print (as ([] points 1) Point))
  (= (. ([] points 1) x) 7)
  (= ([] points 2) (call Point 9 90))
  (print (call ", " (. points x)))
  (for (in row ([] points (..< 1 3)))
    (print (. row y))
  )
  (let arena (call Arena))
  (finally (call (. arena release)))
  (let growing (call (of Table Point) arena))
  (print (call (. growing append) (call Point 1 1)))
  (print (call (. growing append) (call Point 2 2)))
  (let done (call (. growing finish)))
  (print (. done length))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
3
(1, 10)
0, 7, 9
10
90
0
1
2
```

- **Row types** must be struct types with stored fields, or unions; tuples are refused: `Table row
  types must be concrete fixed-layout structs`.
- **Making one.** `(call (of Table Point) count)`{.brix} makes `count` zero rows, and with a
  named argument `with`, each row is the function's value for its index. A table made with an
  arena from `/core/arena`, `(call (of Table Point) arena)`{.brix}, grows: `append` adds a row and gives its index,
  and `extend`, `reserve`, `truncate` and `clear` change its length. Only these tables grow:
  `Table.append requires an Arena-backed growable table`.
- **Rows.** `([] t index)`{.brix} is a row that reads and writes the table, and
  `(as ([] t index) Point)`{.brix} gathers a copy. Assigning a struct to `([] t index)`{.brix}
  writes every field.
- **Columns.** `(. t x)`{.brix} is the column of field `x`, which reads and writes the table.
- **Spans.** `([] t range)`{.brix} is a span of rows, indexed from the start of the range.
- **Finishing.** `finish` gives an `ImmutableTable` with the rows, and leaves the table sealed:
  using it afterwards panics with `Table is sealed`. `view` reads the rows so far without
  sealing.
- **Sharing.** Assigning a table shares it. `copy` makes one with storage of its own.

## Bitmaps

A bitmap, from `/core/bitmap`, holds bits. `(of Bitmap U8)`{.brix} packs eight bits into a `U8`
and is a value, like the integer. A `BitmapBuffer` holds any number of bits in memory of its own,
and is shared like a list.

```brix
(import /core/bitmap)

(main
  (let flags (call (of Bitmap U8)))
  (= ([] flags 0) true)
  (= ([] flags 3) true)
  (print (as flags U8))
  (print ([] flags 3))
  (print (call (. flags count) true))
  (print (call (. flags find) false))
  (print (call ", " (call (. flags indices) true)))
  (let many (call BitmapBuffer 100))
  (= ([] many 99) true)
  (print (call (. many count) true))
)
```

```output
9
true
2
1
0, 3
1
```

| Operation | Value |
| --- | --- |
| `([] b index)`{.brix}, `(= ([] b index) value)`{.brix} | Reads or writes a bit as a `Bool`. |
| `(. b capacity)`{.brix} | The number of bits. |
| `(call (. b count) value)`{.brix} | How many bits are `value`. |
| `(call (. b find) value)`{.brix} | The index of the first bit that is `value`, or `null`. |
| `(call (. b find-consecutive) count value)`{.brix} | The index of the first run of `count` such bits, or `null`. |
| `(call (. b indices) value)`{.brix} | A stream of the indexes of the bits that are `value`. |
| `(call (. b fill) value)`{.brix} | Every bit set to `value`. A `Bitmap` gives a new bitmap, and a `BitmapBuffer` changes itself. |

- **Bit types.** The type after `Bitmap` must be an integer type. `(of Bitmap Char)`{.brix} has
  32 bits.
- **Out of range** panics: `Bitmap index out of bounds`.
- **Looping** over a bitmap gives each bit as a `Bool`, from bit 0.

## Ranges as values

A range, such as `(.. 0 10 (by: 2))`{.brix}, is a value as well as a loop header. Its type is
`Range'T`, a tuple of its `start`, its `end`, its `increment` and whether it includes its end,
`is-through`.

```brix
(main
  (let evens (.. 2 8 (by: 2)))
  (print evens)
  (print (. evens length))
  (print ([] evens 1))
  (print (call List evens))
  (print (. (..< 5 5) length))
  (print (call List (.. 3 1)))
  (print (call List (.. 0.0 1.0 (by: 0.5))))
  (print ([] "abcdef" (... 2)))
)
```

```output
[Range 2 thru 8 by 2]
4
4
2, 4, 6, 8
0
3, 2, 1
0, 0.5, 1
cdef
```

- **Ends.** `(.. a b)`{.brix} includes `b`, and `(..< a b)`{.brix} stops before it. A range from a
  larger number to a smaller one counts down.
- **Steps.** A range steps toward its end by the size of its step, whatever the step's sign. A
  step of 0 panics: `A range cannot step by 0`.
- **Open ranges.** `([] xs (... a))`{.brix}, from `a` to the end, and `([] xs (... _ b))`{.brix},
  from the start to before `b`, index collections. An open range is a `Range'Position`.
- **Members.** `length`, `start`, `end`, `increment`, `is-through`, and `([] r index)`{.brix}
  for an item. An index past the last item panics: `Range index 4 out of range`.
- **Streams.** A range is a stream of its items: `(call List r)`{.brix} lists them, and a function
  that takes a stream takes a range.
- **Printing** writes `[Range 2 thru 8 by 2]`, or `to` for a range that stops before its end.
- **Equality** compares the four fields, so `(.. 0 3)`{.brix} and `(..< 0 4)`{.brix} are not equal. A range has
  no `is in`.

### Positions

A `Position` names a place in a collection without knowing its length. Its cases are `.first`,
`.middle` (half the length), `.last`, `.end` (the length, one past the last item), `.infinity`
and `(call .index n)`{.brix}. An integer converts to a position on its own. Lists, arrays, text
and tables take positions and ranges of positions wherever they take indexes.

## Collection interfaces

These interfaces from the core library describe collections, so a function can take any type
that implements one:

| Interface | Has |
| --- | --- |
| `Indexable'Item` | `length` and `([] this index)`{.brix}; see Streams. |
| `Collection'Item` | The same as `Indexable`. A type implements it by name. |
| `MutableCollection'Item` | `Collection` with assignment, `append`, `insert`, `remove`, `move`, `replace` and `clear`. `List` implements it. |
| `Hashable` | A `hash` property; see [Keys](#keys). |
