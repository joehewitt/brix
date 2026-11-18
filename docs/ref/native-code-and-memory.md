# Native code and memory

A Brix program can call C directly. It imports a C header as it imports a module, calls the
header's functions by their C names, and passes numbers, text, structs, pointers and callbacks
across. This chapter covers C headers and calls, how C's types appear in Brix, pointers, and memory
that the collector does not manage: memory from C, arenas, constant data, and code that runs
without the collector.

## Importing C

```brix
(import /ctype.h)
(import /errno.h)
(import /limits.h)
(import /stdlib.h)
(import /string.h)

(main
  (let phrase "native code")
  (print (call C:strlen phrase))
  (print (call C:strstr phrase "code"))
  (print (call C:strerror C:ENOENT))
  (print (call C:atoi "42 apples"))
  (print (as (call C:toupper 'y') Char))
  (print C:INT_MAX)
  (print (type-of (call C:strlen phrase)))
  (print (?? (call C:getenv "BRIX_REF_UNSET") "unset"))
)
```

```output
11
code
No such file or directory
42
Y
2147483647
[TypeInfo U64]
unset
```

`(import /string.h)`{.brix} imports a C header, and `C:strlen` names a function it declares. In rows
the import is `/string.h`{.rows} and the call `C:strlen(phrase)`{.rows}.

- **Where headers come from.** A path that names a header is looked up as a header before it is
  looked up as a module.
  - A path whose first folder is one of the toolchain's libraries, such as `/sqlite/sqlite3.h` or
    `/BxCore/BxCore.h`, names that library's header.
  - `/c/unistd.h` names a header of the C library explicitly.
  - Any other path is looked up among the target's C library headers, subfolders included, as
    `/sys/stat.h` and `/arpa/inet.h` are.
  - A header beside the program, a framework's header, or a missing header is a missing module:
    `Module "./mylib.h" not found`.
- **What becomes a `C:` name.** Functions with C names whose parameter and result types all have a
  Brix type, and integer constants: macros whose value is an integer, and the cases of enums. A
  header's own includes count too, so `/string.h` also gives the functions of `/stdlib.h`.
- **What does not.** Functions that take a variable number of arguments, such as `printf` and
  `open`, are left out, and so are functions that take or return a union. Floating and text
  macros, macros with parameters, global variables and type names are not `C:` names either: `C:M_PI`
  gives `Function "M_PI" not found`.
- **Per module.** A header's names are visible in the module that imports it, as a module's are.
  A module that uses them without the import gets `C function "abs" is not declared by any header
  this module imports`. `(export /stdlib.h)`{.brix} passes them on, as an export of a module does.
- **Only headers declare C names.** A program cannot declare one of its own:
  `C symbol names belong to header modules`.

### Linking

A program is linked with the runtime, with each toolchain library whose header it imports, and with
the target's system libraries: on macOS, libSystem (the C library, math and threads), libc++ and
zlib. There is no way to link another library. A header of the SDK whose library is not linked, such
as `/curl/curl.h`, passes the check, and the build fails at the link with `Undefined symbols`.

## Calling C

- **Arguments** convert as `as` would convert them to the parameter's type, as in C: a `Float`
  given for an `int` is truncated, and an `Int` keeps its low bits. A literal that does not fit is
  an error: `Integer literal -1 does not fit U64`. A number, a `Bool` or a `Char` never becomes a
  pointer, and `null` is the null pointer.
- **Mismatches** name the declarations: `No C function "strlen" accepts (I64); it is declared as
  "strlen(CPointer'Char8)"`.
- **Text to C.** A `String` is given where C takes a `char *`, and C sees its UTF-8 bytes ending in
  a zero byte. Slices and joined texts arrive the same way. An optional `String?` holding nothing is
  a null pointer.
- **Text from C.** A `char *` result is an `(of CPointer Char8)`. It prints as the text it points
  at, converts to a `String` where one is wanted, and becomes text inside `str`. Each conversion
  copies the text, so the `String` does not change when C's memory does. A null `char *` becomes
  `""`, and `(?? pointer "fallback")`{.brix} gives the fallback text for a null pointer.
- **Results.** An `int` is an `I32`, a `size_t` a `U64`, a `double` an `F64`, a pointer a
  `CPointer`, and a struct returned by value an anonymous struct; see [C types](#c-types).
- **C functions as values.** A C function is a value of a function type, so
  `(let measure Measure C:strlen)`{.brix} works where `Measure` is
  `(fn-type U64 (param text String))`, and a call with some arguments left out gives a binding.

### Out parameters

```brix
(import /stdlib.h)

(main
  (let text "12 34 56 left")
  (let cursor (of CPointer Char8) text)
  (let next (of CPointer Char8))
  (let total 0)
  (while true
    (let value (call C:strtol cursor &next 10))
    (if (== next cursor)
      (break)
    )
    (+= total value)
    (= cursor next)
  )
  (print total)
  (print (str "rest: \"" cursor "\""))
)
```

```output
102
rest: " left"
```

`&next` is a pointer to the local `next`, which C writes through. Only a name can follow `&`: a
local, a parameter, or a struct local. A pointer local declared with no value starts as `null`.

### Callbacks

```brix
(import /stdlib.h)

(main
  (let values (call (of Array I32) (list 42 7 19 3 25)))
  (call C:qsort values (. values length) (. I32 size) (as ascending Compare))
  (print (call ", " values))
  (call C:qsort values (. values length) (. I32 size) (as descending Compare))
  (print (call ", " values))
)

(type + Compare (is (c-fn-type I32 (param a (of CPointer I32)) (param b (of CPointer I32)))))

(fn-expr + ascending I32
  (param a (of CPointer I32))
  (param b (of CPointer I32))
  (- ([] a 0) ([] b 0))
)

(fn-expr + descending I32
  (param a (of CPointer I32))
  (param b (of CPointer I32))
  (- ([] b 0) ([] a 0))
)
```

```output
3, 7, 19, 25, 42
42, 25, 19, 7, 3
```

- **The type.** `(c-fn-type Result (param name Type) …)` declares the type of a C function. In
  rows it is `(*C) I32`{.rows} with its parameters below. Its pointer parameters may be more
  specific than the header's, as `(of CPointer I32)` is for `const void *` here.
- **The function** is a function declared at the top level of a module, given with `as`. A method
  is refused: `Illegal to cast this function to CFunction`. A type that does not fit is refused at
  the call.
- **Context.** To give a callback an object, pass `(as tally Pointer)` where C takes a `void *`,
  and turn it back with `(call (of gc-object-from-pointer Tally) context)`, from
  `/core/intrinsics`. The object must stay referenced from Brix while C holds it; see
  [What the collector follows](#what-the-collector-follows).
- **Panics** in a callback end the task as anywhere else.
- **Threads C creates.** C may call a callback on a thread of its own, with no task. The
  collector does not start or finish a step while such a callback runs, so a long one, or one that
  blocks, holds the collector until it returns.

### How a C call runs

- **Blocking.** A C call may block, as `usleep` and `read` do, without stopping the collector: the
  task leaves the collector's view for the call, and the collector does not wait for it. The call
  does hold its thread, so the other tasks on that thread run only after it returns, where a wait
  in Brix, such as `(call (. 50ms wait))`{.brix}, lets them run. Make a long blocking call in a
  task on the pool, with `do-new-thread`.
- **Short calls.** Functions that cannot reach the collector, such as `strlen`, `strcmp` and the
  math functions, are called directly. The toolchain's own headers mark such a function
  `(without gc)`, and one called between two of the collector's checks `(without blocking)`. A
  program cannot mark a C function itself.
- **`errno`** is not a `C:` name. `(call C:brixGetErrNo)`, from `/BxCore/BxCore.h`, reads it on
  every platform.
- **Output.** `print` writes to C's `stdout` and flushes it, so output from `puts` and `print`
  comes out in the order it was written.

## C types

| C | Brix |
| --- | --- |
| `void` | `Void` |
| `bool` | `Bool` |
| `char` | `Char8` |
| `signed char`, `unsigned char` | `I8`, `U8` |
| `short`, `unsigned short` | `I16`, `U16` |
| `int`, `unsigned int`, `wchar_t` | `I32`, `U32`, `I32` |
| `long`, `long long` | `I64` |
| `unsigned long`, `unsigned long long`, `size_t` | `U64` |
| `float`, `double` | `F32`, `F64` |
| An enum | Its integer type |
| `T *` | `(of CPointer T)`, and `(of CPointer (of CPointer Char8))` for `char **` |
| A pointer to a struct, a union or an unknown type | `(of CPointer I8)` |
| A struct, by value | An anonymous struct of its fields |
| A function pointer | A C function type, `(c-fn-type …)` |

- **`const`** is dropped, so a `const char *` is an `(of CPointer Char8)`.
- **Constants** are integers typed as an integer literal is: `C:INT_MAX` is an `I64`, and fits a
  narrower type where its value does, as in `(let small I8 C:INT8_MIN)`{.brix}. An enum may take C
  constants as its values, and be passed to C.
- **Sizes.** `(. Tm size)`{.brix} gives a type's size in bytes, and `(. I32 bit-width)`{.brix} its
  width in bits. There is no alignment property.

### Structs

```brix
(import /time.h)

(main
  (let moment I64 951782400)
  (let fields (as (call C:gmtime &moment) (of CPointer Tm)))
  (let date ([] fields 0))
  (print (str (+ (. date year) 1900) "-" (+ (. date month) 1) "-" (. date day)))
  (print (. Tm size))
  (let buffer ([] Char8 32))
  (let length (call C:strftime buffer (. buffer length) "%A %d %B %Y" fields))
  (print length)
  (print (as (. buffer data) String))
  (let midnight (call Tm 0 0 0 1 0 100 0 0 0 0 null))
  (print (call C:timegm &midnight))
  (print (. midnight weekday))
)

(type + Tm (is Struct)
  (prop + second I32)
  (prop + minute I32)
  (prop + hour I32)
  (prop + day I32)
  (prop + month I32)
  (prop + year I32)
  (prop + weekday I32)
  (prop + yearday I32)
  (prop + dst I32)
  (prop + offset I64)
  (prop + zone (of CPointer Char8))
)
```

```output
2000-2-29
56
24
Tuesday 29 February 2000
946684800
6
```

- **Layout.** A struct type, `(type + Tm (is Struct) …)`, has C's layout: its fields in
  order, each aligned to its size. So a struct declared with the fields of C's `struct tm` reads and
  writes one, through a pointer converted with `as`, or by `&` of a local as `timegm` takes it.
- **Anonymous structs.** `(c-struct (: bits U32))` is a struct type with no name, as a C function
  returns one. A named type can be one, as `Address` is below.
- **By value.** Structs pass to and from C by value, and assigning one copies it. Reading an item
  through a pointer gives a copy, while assigning a field of `([] fields 0)` writes C's memory.
- **No unions.** C unions have no Brix type, and a header's struct cannot be named in Brix: declare
  a struct with the same fields.
- **Objects are not structs.** An object of a class reaches C as a pointer to the object, whose
  layout is the runtime's. Give C a struct, not an object, when C reads the fields.

```brix
(import /arpa/inet.h)
(import /stdlib.h)

(main
  (let split (call C:ldiv 17 5))
  (print split)
  (print (type-of split))
  (print (str (. split quot) " remainder " (. split rem)))
  (let loopback (call Address 0x0100007f))
  (print (call C:inet_ntoa loopback))
  (print (. Address size))
)

(type + Address (is (c-struct (: bits U32))))
```

```output
(3, 2)
[TypeInfo struct(I64, I64)]
3 remainder 2
127.0.0.1
4
```

## Pointers

```brix
(import /stdlib.h)
(import /string.h)

(main
  (let count 5)
  (let cells (as (call C:calloc count (. I32 size)) (of CPointer I32)))
  (for (in i (..< 0 count))
    (= ([] cells i) (as (* i i) I32))
  )
  (let third (+ cells 2))
  (print ([] third 0))
  (print (- (. third address) (. cells address)))
  (let view (call (of Array I32) cells count))
  (print (call ", " view))
  (call C:memset cells 0 (* count (. I32 size)))
  (print (call ", " view))
  (call C:free cells)
)
```

```output
4
8
0, 1, 4, 9, 16
0, 0, 0, 0, 0
```

- **Types.** `(of CPointer T)` points at values of type `T`. `Pointer` is a pointer to bytes,
  `(of CPointer Byte)`.
- **Making one.** `&name` points at a local, a parameter or a struct local. `(. array data)` points
  at an array's items. `(call Pointer)`{.brix} is the null pointer, and `(as 4096 Pointer)`{.brix}
  a pointer to an address.
- **Reading and writing.** `([] p i)`{.brix} reads the item `i` places on, and
  `(= ([] p i) value)`{.brix} writes it. Nothing checks the index.
- **Moving.** `(+ p n)`{.brix} moves a pointer by `n` items, back when `n` is negative. The
  difference of two pointers is refused; compare their addresses, `(. p address)`, which are
  `U64`s.
- **Comparing.** `==`, `!=`, `<` and the others compare addresses, and a pointer compares with
  `null`. A pointer is a condition, true when it is not null.
- **Printing.** A pointer to `Char8` prints the text it points at. Any other prints its type, as
  `[CPointer core/number:I32*]`.
- **Arrays over C memory.** `(call (of Array I32) pointer count)` is an array over memory it does
  not own. Writes go through to the memory, and indexing is checked, as with
  `Index out of bounds (4 / 4)` for the fifth item of four.

### Pointer conversions

| On its own | Only with `as` |
| --- | --- |
| A pointer to the same type, or to `Void` | A pointer to another type |
| A `(of CPointer Void)` to any pointer | A number to a pointer |
| A pointer to `Char8`, `I8` or `U8` to a pointer to another of those | An array or an object to a pointer of another type |
| An array to a pointer to its items, or to `Void` | A pointer to its address, as `U64` or `Int` |
| A text to a pointer to its bytes | A pointer to `Bool`: whether it is not null |
| `null` to any pointer | |
| Any other object to a pointer to `Void` | |

A conversion that needs `as` says so: `Expected CPointer'I32 here, but this is CPointer'U8, which
becomes CPointer'I32 only when asked; write "as CPointer'I32"`.

## Memory outside the collector

The collector is the default so that objects can refer to one another freely, through parent
links, callbacks and several views of one model, with no rule about who frees what. Leave it only
where a lifetime or a pause matters: an arena for many values that one piece of work makes and can
drop together, memory from C for what a C API owns, and `(without gc)` for code that must never
pause.
The collector frees memory, not resources: close a file, a socket or a subscription with `use` or
`close`, since its object may be reclaimed much later.

### What the collector follows

The collector finds the objects a program can still reach and frees the rest. It does not move
objects, so an object's address stays the same while it lives.

- **Pointers it follows.** A pointer to values, such as a `Pointer` or an `(of CPointer I32)`, in
  a local, a field or an array the collector owns, keeps the object it points into alive.
- **Pointers it does not.** A pointer to pointers, and memory from C, such as `malloc`'s, are not
  looked into. An object referenced only from there is freed. So an object given to C, as a
  callback's context, must also stay referenced from Brix for as long as C may use it.
- **`/gc`** can run the collector, `(call collect-garbage)`, and run code when an object is freed,
  `on-reclaim`; see the Standard library.

### Memory from C

`(call C:malloc n)`, `calloc`, `realloc` and `free` give and take back memory the collector does
not manage, as in the example in [Pointers](#pointers). Nothing tracks it: there is no report of
memory never freed, and no check of memory used after `free`. An array over it gives checked
indexing.

### Arenas

```brix
(import /core/arena)

(main
  (let arena (use (call Arena)))
  (let squares (call (of Array I64) arena 4))
  (for (in i (..< 0 4))
    (= ([] squares i) (* i i))
  )
  (print (call ", " squares))
  (print (type-of squares))
  (let mark (. arena mark))
  (let scratch (call (of (. arena allocate) I64) 1000))
  (= ([] scratch 999) 1)
  (print (> (. arena active-bytes) 8000))
  (call (. arena rewind) mark)
  (print (< (. arena active-bytes) 8000))
  (let name (call (. arena copy-string) "kept in the arena"))
  (print name)
  (print (call (. arena owns) (as name Pointer)))
)
```

```output
0, 1, 4, 9
[TypeInfo Array'I64'ArenaBacking]
true
true
kept in the arena
true
```

An `Arena`, from `/core/arena`, holds memory that is freed all at once.

- **Making one.** `(call Arena)` makes one, and `(call Arena bytes)` gives it a starting size.
  `(use (call Arena))` releases it when the block ends; otherwise call its `release`.
- **Allocating.** `(call (of (. arena allocate) I64) 4)` gives a pointer to 4 zeroed `I64`s, and
  `try-allocate` gives `null` when there is no memory. `allocate-raw` takes a byte count and an
  alignment, and does not zero. `copy-string` copies a text into the arena.
- **Storage in an arena.** Arrays, array builders, tables, bitmaps and `Scratch` stacks take an
  arena in place of the collector, as `(call (of Array I64) arena 4)` does; see Collections. A table
  in an arena cannot hold objects, except texts: `Arena type Holder contains managed pointers
  through Holder.box`.
- **Freeing.** `mark` gives a point to come back to, and `rewind` frees what was allocated after
  it. `reset` frees everything and keeps the memory for reuse. `release` gives it all back.
- **Questions.** `owns` says whether a pointer is in the arena, and `active-bytes`,
  `reserved-bytes`, `peak-active-bytes` and others report its use.
- **Mistakes that are checked.** Rewinding to another arena's mark panics: `Arena mark is stale or
  belongs to another Arena`. So does `mark` after `release`, `Arena is already released`, and
  allocating after it. A view of an arena's array after a `reset` or `rewind` panics with `Stale
  array view`.
- **Mistakes that are not.** Reading an arena's storage after `release`, `reset` or `rewind` is not
  checked, and reads whatever the memory holds. That includes a text the arena holds, and a text
  made from one by an operation that gives back what it was given, such as a slice of the whole
  text, an interpolation of it alone, or a join of it alone.
- **Keeping a text.** `(. text copied)`{.brix} gives a copy the collector owns, which outlives the
  arena.

### Constant data

```brix
(import /string.h)

(prop + month-days _ (= (call (of ConstArray U8) 31 28 31 30 31 30 31 31 30 31 30 31)))

(prop + greeting _ (= (call (of ConstArray Char8) 104 105 0)))

(main
  (let total 0)
  (for (in month (..< 0 12))
    (+= total ([] month-days month))
  )
  (print total)
  (print (type-of month-days))
  (print (call days-in 1))
  (print (call C:strlen greeting))
)

(fn-expr + days-in U8
  (param month Int)
  ([] month-days month)
)
```

```output
365
[TypeInfo ConstArray'U8]
28
2
```

`(call (of ConstArray U8) 31 28 …)` is data written into the program when it is built, as C's
`static const` arrays are.

- **Items** are constants, converted to the item type as the program is built. Anything else is
  refused: `ConstArray item must be constant`.
- **Reading.** `([] table i)`{.brix} reads an item, with no check of the index. A `ConstArray` has
  no `length`.
- **Read-only.** Assigning an item is refused: `ConstArray values are read-only`.
- **To C**, a `ConstArray` is a pointer to its items, as `greeting` is for `strlen`.

### Code that runs without the collector

```brix
(main
  (let readings (call (of Array F64) (list 2.5 4.0 3.5)))
  (print (call mean readings))
  (let total 0)
  (do (without gc)
    (for (in i (..< 1 5))
      (+= total (* i i))
    )
  )
  (print total)
)

(fn-do + mean F64 (without gc)
  (param values (of Array F64))
  (let sum 0.0)
  (for (in value values)
    (+= sum value)
  )
  (return (/ sum (as (. values length) F64)))
)
```

```output
3.3333333333333335
30
```

`(without gc)`, after a function's annotations or on a `(do …)` block, makes code that never meets
the collector: it allocates nothing the collector manages, never pauses for it, and never suspends
its task. The core library's arrays, tables and arenas use it for their inner loops. Each part of
it can be asked for alone, as in `(fn-do + mean F64 (without heap suspend) …)`; see
Functions.

| Name | Refuses |
| --- | --- |
| `heap` | Making objects, arrays, lists and text; closures, tasks and `(do (without gc) …)` blocks that capture locals; a struct or other value that is no object made a value of an interface; a view of an array's rows or fields; calling a generator; a method bound to its object made a function value; and reading a global written with `(:= …)`. |
| `safepoints` | Pausing for the collector. |
| `suspend` | Suspending the task, calling function values, and reading a global written with `(:= …)`. |
| `gc` | All three. |

- **Messages.** Making an object gives `allocation is not allowed in a GC-free zone (entered at …)`,
  storing an object anywhere but the zone's own locals gives `storing a managed pointer is not
  allowed in a GC-free zone`, and waiting gives `cannot suspend inside a GC-free zone`. Each is
  reported where it happens, with the calls that reach it from where the restriction is placed,
  as in `mean (without gc) → helper → Box()`. Code a restriction reaches is checked under it however
  it is called: directly, through an interface or a class's method, or through a conversion.
  Reading a global written with `(:= …)` gives `cannot read lazy global 'name' inside …, since its
  first read runs its value`, as that read may run any code and wait while another task runs it.
- **What may run.** Numbers, structs, pointers, arrays made outside the code, method calls, C calls,
  and an arena's methods, which are themselves `(without gc)`. A C function marked
  `(without blocking)` may reach the collector, so under `(without heap)` or `(without suspend)`
  one counts as allocating and as suspending unless the compiler knows it does neither.

## What is checked

- **When compiling:** that a `C:` name is declared by an imported header; the number and types of
  a C call's arguments; pointer conversions; constants that do not fit; that a C callback is a
  module function; writes to `ConstArray` items; what an arena's table holds; and what code
  without the collector does.
- **When running:** array indexes, including arrays over C memory; arena marks, rewinds and
  allocation after release; and stale views.
- **Not at all:** pointer indexes, null pointers, memory used after `free`, an arena's storage used
  after it is freed, memory never freed, and objects referenced only from C memory. Reading through
  a null pointer ends the program with status 139, with no report.

## Platforms

- **Headers differ.** macOS and iOS builds read the SDK's headers, and Linux builds the sysroot's,
  so a function can have a different declaration on each. `qsort_r` takes its context first on
  macOS and last on Linux. Put calls that differ in platform modules; see Modules.
- **Libraries differ.** macOS links libSystem, libc++, zlib and two system frameworks, iOS libc++
  and zlib, and Linux glibc, libm and static libstdc++, without zlib.
- **Toolchain libraries differ.** A header of a toolchain library that is not built for the target
  is refused, as `… requires … , which the runtime build config does not build for linux`. See
  Platforms.
