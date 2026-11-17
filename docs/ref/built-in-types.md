# Built-in types

Every module imports the core library's 18 always-imported modules without writing them; see
Modules. Their public types are the built-in types. Most belong to a chapter of their own, and this
one lists them all and says where each is covered. It covers fully the ones no other chapter does:
the number types as a family, type information, function types, the interfaces the language itself
uses, and a few others. It ends with the names that look like types but are not, and the functions
core makes visible.

## The catalog

### Numbers and truth

| Type | What it is | See |
| --- | --- | --- |
| `I8`, `I16`, `I32`, `I64`, `U8`, `U16`, `U32`, `U64` | Integers of 8 to 64 bits, signed and unsigned. `Int` is `I64`. | [Numbers](#numbers) |
| `F32`, `F64` | Floating-point numbers. `Float` is `F64`. | [Numbers](#numbers) |
| `I1` | A one-bit integer, as a `Bitmap` holds. | [Numbers](#numbers) |
| `Number` | The base of every number type, `Bool` and `Char`. | [Numbers](#numbers) |
| `Size`, `Index`, `Increment` | Counts, positions and steps, built on `I64`. | [Numbers](#numbers) |
| `Hash` | What `hash` gives, built on `U32`. | Collections |
| `Endian` | Byte order: `big` or `little`. | [Numbers](#numbers) |
| `Bool` | `true` or `false`. | Literals |
| `Null` | The type of `null`. | Optionals |
| `Optional'Value` | What `(? T)` means. | Optionals |
| `Void` | No value. | Functions |

### Text

| Type | What it is | See |
| --- | --- | --- |
| `String` | UTF-8 text. | Text |
| `Char`, `Char8` | A code point, and a C `char`. | Text |
| `Atom` | A name kept by its address. | Text |
| `Hex`, `Base64`, `Binary`, `UnitString`, `Abbreviation` | Kinds of text, built on `String`. | Text |
| `UTF8Category` | The Unicode category of a character. | Text |
| `DisplayText` | Text with its width on a terminal measured once. | [Display text](#display-text) |

### Collections, ranges and streams

| Type | What it is | See |
| --- | --- | --- |
| `List`, `Map`, `KeyValuePair` | Lists, maps, and a map's entries. | Collections |
| `Array`, `ImmutableArray` | Arrays of fixed length. | Collections |
| `Collection`, `MutableCollection`, `Hashable` | Interfaces of collections and keys. | Collections |
| `NTuple2` to `NTuple6` | Tuples of 2 to 6 values of one type. | [Other types](#other-types) |
| `CollectionMutation`, `CollectionMutationKind`, `CollectionObserver` | Changes to a collection, for code that watches them. | [Watching a collection](#watching-a-collection) |
| `Range`, `Position` | Ranges as values, and positions in a collection. | Collections |
| `Stream`, `Iterable`, `CursorIterable`, `Indexable`, `Broadcastable`, `Generator` | The stream family. | Streams |
| `Readable`, `Writable`, `Closable`, `Usable` | Values that can be read, written, closed and used. | [Interfaces the language uses](#interfaces-the-language-uses) |
| `Subscriber`, `Subscribable` | Functions and objects a broadcaster delivers to. | [Other types](#other-types) |

### Functions, tasks and events

| Type | What it is | See |
| --- | --- | --- |
| `Doer`, `Getter`, `Mapper`, `Predicate`, `Comparer` | Common function types. | Functions |
| `Statement`, `Selector`, `OptionalMapper`, `Lazy`, `Comparison` | More function types, and what a `Comparer` gives. | [Function types](#function-types) |
| `Task`, `TaskEnd`, `Worker` | Tasks, how they ended, and the threads they run on. | Concurrency |
| `Semaphore`, `Mutex`, `Completion` | Waiting and locking. | Concurrency |
| `SyncChannel`, `SyncBufferedChannel`, `AsyncChannel`, `AsyncBufferedChannel` | Channels. | Concurrency |
| `AsyncOnDemandChannel`, `OneShotChannel` | A broadcaster that starts its source when first heard, and a gate. | [More channels](#more-channels) |
| `RoutedEvent`, `EventNode`, `Tracker`, `LiveAction` | Routed events and tracking. | Reactivity |

### Errors, types and memory

| Type | What it is | See |
| --- | --- | --- |
| `Error`, `Panic` and their kinds | Errors. | Errors |
| `UnhandledErrorHandler` | The type of `unhandled-error`. | [Other types](#other-types) |
| `TypeInfo`, `PropertyList`, `PropertyInfo`, `TypeRelation` | Types as values. | [Type information](#type-information) |
| `Tuple`, `Struct`, `Union`, `Interface` | The bases that make a declaration a tuple, struct, union or interface. | Types |
| `MatchLocation`, `TokenLocation`, `TokenStream` | Where a pattern matched, and resumable sources. | Stream patterns |
| `CPointer`, `Pointer`, `ConstArray`, `CFunction` | Pointers, constant data and C functions. | Native code and memory |
| `GCBacking` | Where a collection's items are kept by default. | Types |

## Numbers

```brix
(main
  (print (str "U8 " (. U8 min) " to " (. U8 max) ", " (. U8 size) " byte"))
  (print (str "I16 " (. I16 min) " to " (. I16 max) ", " (. I16 bit-width) " bits"))
  (print (str "F32 up to " (. F32 max)))
  (print (str (. Int name) " " (. Size name)))
  (print (str (== Int I64) " " (== Size I64)))
  (let count Size 3)
  (let total Int count)
  (print (type-of count))
  (print (type-of (+ count total)))
  (print (as 2.5f32 UnitString))
  (print (call (. 42 pad) 5))
  (print (call (. 2.5 pad) 3))
  (print (call (. 2.5 places) 3))
  (print (as 300u16 Binary))
  (print (as "300" U8))
)
```

```output
U8 0 to 255, 1 byte
I16 -32768 to 32767, 16 bits
F32 up to 3.4028235e+38
I64 Size
true false
[TypeInfo Size]
[TypeInfo I64]
2.5f32
00042
2.500
2.5
00000001 00101100
255
```

| Type | Bytes | Least | Greatest | Suffixes | Other names |
| --- | --- | --- | --- | --- | --- |
| `I1` | 1 | 0 | 1 | `i1` | `Int1` |
| `I8` | 1 | -128 | 127 | `i8` | `Int8` |
| `U8` | 1 | 0 | 255 | `u8` | `UInt8`, `Byte` |
| `I16` | 2 | -32768 | 32767 | `i16` | `Int16`, `Short` |
| `U16` | 2 | 0 | 65535 | `u16` | `UInt16`, `UShort` |
| `I32` | 4 | -2147483648 | 2147483647 | `i32` | `Int32` |
| `U32` | 4 | 0 | 4294967295 | `u32` | `UInt32` |
| `I64` | 8 | -9223372036854775808 | 9223372036854775807 | `i64`, `l`, `i` | `Int64`, `Int`, `Long` |
| `U64` | 8 | 0 | 18446744073709551615 | `u64`, `ul`, `u` | `UInt64`, `UInt`, `ULong` |
| `F32` | 4 | -3.4028235e+38 | 3.4028235e+38 | `f32` | `Float32` |
| `F64` | 8 | -1.7976931348623157e+308 | 1.7976931348623157e+308 | `f64`, `f` | `Float64`, `Float` |
| `Bool` | 1 | `false` | `true` | `b` | |
| `Char` | 4 | | | `c` | |

- **On the type.** `size` is a type's size in bytes, `min` and `max` its least and greatest value,
  and `bit-width` an integer type's width in bits. `is-integer`, `is-signed` and `is-number` say
  what kind of number it is.
- **Other names.** `Int`, `Float`, `Byte` and the others in the last column are the same type under
  another name: `(== Int I64)` is true, and the type prints as `I64`. Programs write `Int`, `Float`
  and `Byte`.
- **`Size`, `Index` and `Increment`** are types of their own built on `I64`, and `Hash` on `U32`.
  They convert to and from their base on their own, arithmetic on them gives the base, and they
  keep their own names: `(== Size I64)` is false.
- **`Number`** is the base of every number type, `Bool` and `Char`. As a constraint,
  `(constraint is Item Number)` admits all of them; as a parameter's type it takes nothing.
- **Text.** `(call (. n pad) 5)` writes an integer with zeros in front, to 5 characters with the
  sign, or a float with exactly that many decimals. `places` writes a float with at most that many,
  dropping zeros at the end. `as Hex`, `as Binary` and `as UnitString` write an integer in hex, in
  bits, or followed by its unit, as `2.5f32` is. Text converts to a number with `as`, and a number
  that does not fit the type is clamped: `(as "300" U8)` is `255`.
- **`I1`** holds 0 or 1, as a `Bitmap'I1` does. Arithmetic on it gives an `I32`.
- **`Endian`** names a byte order, `.big` or `.little`, for reading and writing numbers in a byte
  array; see Collections.

## Type information

```brix
(main
  (let shape Shape (call Square))
  (let info (type-of shape))
  (print info)
  (print (. info name))
  (print (. info concrete-name))
  (print (. Square base))
  (print (str (. Shape size) " " (. Point size)))
  (print (str (. Point is-tuple) " " (. Point read-only) " " (. Shape read-only)))
  (for (in property (. Square properties))
    (print (str (. property name) " " (. property type-name) " at " (. property offset)))
  )
  (print (call (. Square compare) Shape))
  (print (call (. Shape compare) Square))
  (print (call (. I32 compare) F64))
  (print (call (. Square compare-unified) Circle))
  (print (== (type-of shape) Shape))
)

(type + Shape
  (prop + label _ (= "shape"))
)

(type + Square (is Shape)
  (prop + side F64 (= 2.0))
  (prop + center Point (= (call Point 0 0)))
)

(type + Circle (is Shape))

(type + Point (is (tuple (: x F32) (: y F32))))
```

```output
[TypeInfo Shape]
Shape
Square
[TypeInfo Shape]
8 8
true true false
label String at 16
side F64 at 24
center Point at 32
[UpCast Square = Shape]
[DownCast Shape = Square]
[SideCast I32 = F64]
[CommonUpCast Square = Circle]
true
```

A type's name used as a value is a `TypeInfo`, and so is what `(type-of value)`{.brix} gives. It
prints as `[TypeInfo Shape]`, and a generic type with its arguments joined by `'`, as
`[TypeInfo List'I64'GCBacking]`.

| Member | Gives |
| --- | --- |
| `name` | The type's name as it prints: `I64` for `Int`. |
| `key` | The type's full key, with its module, as `Class:core/number:I32`. |
| `concrete-name`, `concrete-key` | For an object, the class it was made as, read when the program runs. |
| `size` | The bytes a value takes where it is stored: 8 for any object. |
| `min`, `max`, `bit-width`, `is-integer`, `is-signed`, `is-number` | What a number type holds; see [Numbers](#numbers). |
| `is-tuple`, `is-struct` | Whether it is a tuple, or a struct. |
| `read-only` | False for a class, a struct and a table column, whose values can be changed in place. |
| `base` | The type it is built on. A type with none is an error: `No base type`. |
| `properties` | Its stored properties, base class first, as a `PropertyList`. |
| `compare(other)` | How a value of this type relates to `other`, as a `TypeRelation`. |
| `compare-unified(other)` | How it relates to the type both have in common. |

- **Equality.** `==` and `!=` compare types: `(== (type-of shape) Shape)`{.brix} is true for the
  declared type, and false for the class the object was made as.
- **`type-of`** works out its operand once, and gives its declared type.
- **Properties.** A `PropertyList` has a `length`, can be indexed and looped over, and finds one
  property with `(call (. list named) "side")`, giving `null` where there is none. Each
  `PropertyInfo` has a `name`, `type-name`, `offset`, `size`, and flags such as `is-float` and
  `is-reference`.
- **Relations.** A `TypeRelation` prints its kind and the two types: `UpCast` to a base class,
  `DownCast` to a derived one, `SideCast` between numbers or into an optional, `Converter` through a
  declared conversion, `Illegal` where there is no conversion, and others. Its `cast` converts a
  value as the relation says.
- **In generic functions**, a type parameter is a `TypeInfo`, so `(. Item name)` names the type a
  call was made with.

## Function types

```brix
(import /stream/sort)

(main
  (let words (list "pear" "fig" "banana"))
  (print (call sort words (:using by-length)))
  (print (call by-length "fig" "kiwi"))
  (let shout (of Selector String String) (=> (word) (. word uppercased)))
  (print (call shout "hey"))
  (let report (of Statement String) (=> (word) :do (print (str "saw " word))))
  (call report "fig")
  (let even (of OptionalMapper Int Int) (=> (n) (only-if n (if (== (% n 2) 0)))))
  (print (call even 3))
  (print (call even 4))
  (let answer (of Lazy Int) (=> () 42))
  (let known Int answer)
  (print known)
  (print (type-of by-length))
  (print (type-of shout))
)

(fn-expr + by-length Comparison
  (param left String)
  (param right String)
  (case? (< a b)
    .lesser
  )
  (case? (> a b)
    .greater
  )
  (default
    .equal
  )
  (where
    (lazy a (. left length))
    (lazy b (. right length))
  )
)
```

```output
fig, pear, banana
lesser
HEY
saw fig
unknown
4
42
[TypeInfo Binding]
[TypeInfo Selector'String'String]
```

| Type | Shape |
| --- | --- |
| `Statement'Item` | `(item Item) Void`: a step for each item. |
| `Selector'Item'Result` | `(item Item) Result`, as `Mapper`. |
| `OptionalMapper'Item'Result` | `(item Item) Result?`, as `?*>` takes. |
| `Lazy'T` | `() T`, which converts to `T` by calling itself. |
| `Comparison` | Not a function: `lesser`, `equal` or `greater`, what a `Comparer` gives. |

- **Names at run time.** A function value prints its type as `Closure`, and a declared function
  used as a value as `Binding`. A value declared with a named function type keeps that name.
- **`Function`** is the marker of function types, not a type a value can be declared with:
  `Expected Function here, but this is closure`.

## Interfaces the language uses

```brix
(main
  (let log (call Log))
  (<- log "start" "middle")
  (let sink (of Writable String) log)
  (<- sink "end")
  (print (. log lines))
  (let countdown (call Countdown 2))
  (print (<- countdown))
  (print (<- countdown))
  (print (<- countdown))
  (do-use (call Door "front")
    (print "inside")
  )
  (do-use (call Session)
    (print "working")
  )
)

(type + Log
  (implements (of Writable String))
  (prop + lines (list String))
  (fn-do + (infix this <- (param item String)) _
    (+= lines item)
  )
)

(type + Countdown
  (implements (of Readable Int))
  (prop + left Int)
  (fn-do + This _
    (param left _)
  )
  (fn-do + (prefix <- this) (? Int)
    (return-if null (if (== left 0)))
    (-= left 1)
    (return left)
  )
)

(type + Door
  (implements Closable)
  (prop + name String)
  (fn-do + This _
    (param name _)
  )
  (fn-do + close _
    (print (str "closed " name))
  )
)

(type + Session
  (implements (of Usable Session))
  (fn-do + begin-using Session
    (print "begin")
    (return this)
  )
  (fn-do + end-using _
    (print "end")
  )
)
```

```output
start, middle, end
1
0
unknown
inside
closed front
begin
working
end
```

Some interfaces give a type a place in the language itself, not only in libraries:

| Interface | What it gives | What a type declares |
| --- | --- | --- |
| `Readable'Message` | Reading with `(<- value)`. | `(prefix <- this)`, giving `Message?`. |
| `Writable'Message` | Writing with `(<- value item)`. | `(infix this <- (param item Message))`. |
| `Closable` | `use` and `do-use` call `close` at the end. | `close`. |
| `Usable'Used` | `use` binds what `begin-using` gives, and calls `end-using` at the end. | `begin-using` and `end-using`. |
| `Iterable`, `CursorIterable`, `Indexable`, `Broadcastable` | Loops and the stream family; see Streams. | Their members. |
| `Collection'Item` | Tracked reads follow the changes it reports; see Reactivity. | Its members, and the notifications. |
| `Hashable` | Use as a map key; see Collections. | `hash` and `==`. |
| `RoutedEvent`, `EventNode` | Routed and rising events; see Reactivity. | Their members. |

- **`use` goes by members.** A type that declares `close`, or `begin-using` and `end-using`, can be
  used whether or not it names the interface. One with neither cannot: `Plain cannot be used, as it
  has no "begin-using" and is not Closable`.
- **Reading is not looping.** A type that is only `Readable` is read with `<-`, not looped over:
  `A for loop cannot iterate over Countdown`.
- **Protocols that are not interfaces.** A type becomes text through a conversion to `String`, and
  converts through its `fn-to` and `fn-from` declarations; see Conversions. It compares through the
  operators it defines; see Operators.

## Display text

```brix
(main
  (let plain (call DisplayText "abc"))
  (let wide (call DisplayText "a漢b"))
  (print (str (. plain display-width) " " (. plain codepoint-count) " " (. plain is-single-cell)))
  (print (str (. wide display-width) " " (. wide codepoint-count) " " (. wide is-single-cell)))
  (print (call (. wide width-at) 1))
  (print (call ", " (. wide widths)))
  (let both (+ plain wide))
  (print (. both text))
  (print (. both display-width))
  (print plain)
)
```

```output
3 3 true
4 3 false
2
1, 2, 1
abca漢b
7
(abc, 3, 3, U8[0])
```

A `DisplayText` holds a text with how many columns of a terminal it takes, measured once when it is
made. `widths` gives each character's width, and is empty when every character takes one column,
as `is-single-cell` says. `width-at` gives one character's width, and `+` joins two. The terminal
libraries keep their text this way.

## More channels

```brix
(main
  (let ticks (call (of AsyncOnDemandChannel Int) (call Ticker)))
  (print "no subscriber yet")
  (for (on t ticks)
    (print (str "tick " t))
    (if (== t 2)
      (break)
    )
  )
  (<- ticks 1)
  (<- ticks 2)
  (let gate (call OneShotChannel))
  (let waiter
    (do-new
      (<- gate)
      (print "gate opened")
    )
  )
  (do-new
    (print (str "waiter end " (. waiter end)))
    (call (. gate done))
  )
  (print "main done")
)

(type + Ticker
  (implements (of Usable Void))
  (fn-do + begin-using _
    (print "first subscriber: start")
  )
  (fn-do + end-using _
    (print "last subscriber gone: stop")
  )
)
```

```output
no subscriber yet
first subscriber: start
main done
tick 1
tick 2
last subscriber gone: stop
waiter end 0
gate opened
```

- **`AsyncOnDemandChannel`** is a broadcaster made with a `Usable`. Its first subscription calls
  `begin-using`, and when the last one ends it calls `end-using`, so a source runs only while
  someone listens. Values written with no subscriber are dropped. Timers and signals in the standard
  library are made this way.
- **`OneShotChannel`** is a gate. `(<- gate)` waits until some task calls `(call (. gate done))`,
  which lets every waiting task go on. A task that reads the gate after that waits for the next
  `done`.
- **`TaskEnd`** prints as a number: 0 while the task runs, as above.

## Watching a collection

```brix
(main
  (let items (list "a" "b"))
  (let watcher CollectionObserver (=> (change) :do (call show change)))
  (call observe-collection items watcher)
  (+= items "c")
  (call (. items insert) "x" 0)
  (= ([] items 1) "A")
  (call (. items move) 0 2)
  (call (. items remove) 0 2)
  (call unobserve-collection items watcher)
  (+= items "unseen")
  (print items)
)

(fn-do + show _
  (param change CollectionMutation)
  (print (str (. change kind) " at " (. change index) ", " (. change from-index) " to "
    (. change to-index) ", count " (. change count)))
)
```

```output
append at 2, 0 to 0, count 1
insert at 0, 0 to 0, count 1
set at 1, 0 to 0, count 1
move at 0, 0 to 2, count 1
remove at 0, 0 to 0, count 2
b, c, unseen
```

`(call observe-collection items watcher)` calls the watcher with a `CollectionMutation` for each
change to the collection, until `unobserve-collection`. A mutation's `kind` is `append`, `insert`,
`set`, `remove`, `replace`, `move` or `reset`, and its `index`, `from-index`, `to-index` and
`count` say where.

## Other types

- **`NTuple2'T` to `NTuple6'T`** are tuples of 2 to 6 values of one type, with fields `a` to `f`:
  `(of NTuple2 Int)` is `(a Int, b Int)`. A list takes one as an index to assign several items:
  `(= ([] xs (tuple 0 2)) 7)` sets the items at 0 and 2.
- **`Subscriber'Message`** is the function type a broadcaster delivers to:
  `(item Message, broadcaster (of Broadcastable Message)) Void`. `(call (. ticks subscribe) printer)`
  subscribes one without a loop, and `unsubscribe` with the same value ends it.
- **`UnhandledErrorHandler`** is the type of `unhandled-error`, which takes an error and gives
  nothing. Assigning `unhandled-error` replaces what is done with errors that have no caller; see
  Errors.
- **Optional members.** An optional's `has-value` says whether it holds a value. Read the value
  with `!!` or a pattern; see Optionals.
- **Type families.** A type with several `is` types and no body, as
  `(type-line + Plain (is Int String))`{.brix}, is a family: as a parameter's type it takes any of
  them, and the function is made once for each. `Stream` is one.

## Names that are not types to write

- **Backings in printed types.** `type-of` shows a collection's backing, as in
  `[TypeInfo List'I64'GCBacking]`. A program writes `(list Int)` and names a backing only to choose
  another, such as an arena's.
- **Names only seen at run time.** `Closure`, `Binding`, and `Tuple` and `Union` for anonymous
  tuples and unions, appear in `type-of` output, but are not types to declare values with.
- **Core's own.** `Function`, `Capture`, `Or`, `GCTypeMap`, `StorageBacking`, the types of array
  views, the trackers, and the pattern runtime's types are public because the core and the code
  the compiler writes use them. A program does not name them.

## Functions core gives

| Function | Does | See |
| --- | --- | --- |
| `print`, `print-all` | Prints a value, or each item of a stream on its own line. | Text |
| `request-process-termination` | Ends the program with a status. | Modules |
| `when-main-ends` | Runs an action when `main` ends. | Concurrency |
| `usable-core-count` | The number of cores a program keeps busy. | Concurrency |
| `get-current-task-id` | The running task's id, 1 in `main`. | Concurrency |
| `await` | Runs a function in the thread pool, and waits for it. | Concurrency |
| `unhandled-error`, `print-unhandled-error` | What is done with an error that has no caller, and the default. | Errors |
| `join` | Joins items into text. | Text |
| `decode-base64` | Base64 text to bytes. | Text |
| `hash-for` | The hash of a key. | Collections |
| `map`, `filter`, `map-filter`, `tap` | The functions pipeline stages `*>`, `?>`, `?*>` and `*\|>` call. | Streams |
| `map-do` | Runs a step for each item now. | Streams |
| `flat-map` | The items of a stream of streams. | Streams |
| `publisher` | A stream as a broadcaster. | Streams |
| `observe-collection`, `unobserve-collection` | Watch a collection's changes. | [Watching a collection](#watching-a-collection) |
| `identity` | Gives its argument. | |
| `type-identity` | A predicate that tests whether a value is of a type. | [Type information](#type-information) |
| `live-state`, `keep-live-state` | State a live function keeps between runs. | Reactivity |

There is no query that lists types or members. `brix app.bx --list-sources` lists the modules a
program loads, core's among them, and `brix app.bx --explain hash-for` finds a function's
declarations.
