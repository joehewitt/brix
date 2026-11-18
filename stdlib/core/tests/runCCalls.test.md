# Ccalls

```yumi
/JaCore/tests.h

C:ctest1()
```

```output
received!
```

```yumi
/JaCore/tests.h

C:ctest2a(42)
```

```output
received 42
```

```yumi
/JaCore/tests.h

C:ctest2a(true)
```

```output
received 1
```

```yumi crash
/JaCore/tests.h

C:ctest2a((1,2))
```

```output
Exception: No C function "ctest2a" accepts ((I64, I64)); it is declared as "ctest2a(I32)"
<no file:4>
    C:ctest2a((1,2))
    ^^^^^^^^^^^^^^^^
```

```yumi crash
/JaCore/tests.h

C:ctest2a • with 1
```

```output
Exception: No C function "ctest2a" accepts with I64; it is declared as "ctest2a(I32)"
<no file:4>
    C:ctest2a • with 1
    ^^^^^^^^^^^^^^^^^^
```

```yumi
/JaCore/tests.h

C:ctest2a(42)
```

```output
received 42
```

```yumi
/JaCore/tests.h

C:ctest2b(true)
```

```output
received 1
```

```yumi
/JaCore/tests.h

C:ctest2c(42.1234f)
```

```output
received 42.123400
```

```yumi
/JaCore/tests.h

C:ctest2d(9223372036854775807i64)
```

```output
received 9223372036854775807
```

```yumi
/JaCore/tests.h

C:ctest2e("foo")
```

```output
received 'foo'
```

### An optional text given to a C pointer is the text it holds, or a null pointer where it holds none.

```yumi
/JaCore/tests.h

give("foo")
give(null)

━━━━━━━━━━━━━━━━━━━━
+ do give
    - text String?

C:ctest2g(text)
```

```output
received 'foo'
received null
```

```yumi
/JaCore/tests.h

. thing = Thing()
C:ctest2f(thing)

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ a I32 = 48
```

```output
received 48
```

```yumi
/JaCore/tests.h

. thing = Thing()
C:ctest2f(thing)

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ a I32 = 48
+ doubled = #a * 2
```

```output
received 48
```

```yumi
/JaCore/tests.h

-- C:ctest4a()
```

```output
42
```

```yumi
/JaCore/tests.h

-- C:ctest4b()
```

```output
9223372036854775807
```

```yumi
/JaCore/tests.h

-- C:ctest4c()
```

```output
42.1234
```

## Casting C functions to function types

```yumi
/JaCore/tests.h

. f2 = C:ctest2a as Foo
f2(42)

━━━━━━━━━━━━━━━━━━━━
+ Foo is () Void
    - a Int
```

```output
received 42
```

```yumi
/JaCore/tests.h

. f1 = C:ctest2a as Foo
f1(42)

. f2 = C:ctest2aa as Foo
f2(43)

━━━━━━━━━━━━━━━━━━━━
+ Foo is () Void
    - a Int
```

```output
received 42received aa 43
```

```yumi crash
/JaCore/tests.h

. f1 = C:ctest2a as Foo
f1(1, 2)

━━━━━━━━━━━━━━━━━━━━
+ Foo is () Void
    - a Int
```

```output
Exception: Wrong number of arguments
<no file:5>
    f1(1, 2)
    ^^^^^^^^
```

### Cast C function to another function with different return type

```yumi
/JaCore/tests.h

. f1 = C:ctest4a as Foo
-- f1()

━━━━━━━━━━━━━━━━━━━━
+ Foo is () Float
```

```output
42
```

```yumi
/JaCore/tests.h

. f1 = C:ctest2a as Foo
f1(42.5)

━━━━━━━━━━━━━━━━━━━━
+ Foo is () Void
    - a Float
```

```output
received 42
```

```yumi
/JaCore/tests.h

. f1 = C:ctest2a as Foo
f1(Thing())

━━━━━━━━━━━━━━━━━━━━
+ Thing

────────────────────
+ to I32

42

━━━━━━━━━━━━━━━━━━━━
+ Foo is () Void
    - a Thing
```

```output
received 42
```

### Cast C function to another function with incompatible return type

```yumi crash
/JaCore/tests.h

. f1 = C:ctest2a as Foo
f1(Thing())

━━━━━━━━━━━━━━━━━━━━
+ Thing

━━━━━━━━━━━━━━━━━━━━
+ Foo is () Void
    - a Thing
```

```output
Exception: function ctest2a cannot be cast to Foo; there is no conversion between them
<no file:4>
    . f1 = C:ctest2a as Foo
           ^^^^^^^^^^^^^^^^
```

### Multiple arguments

```yumi
/JaCore/tests.h

C:ctest13(Thing(), "foo")

━━━━━━━━━━━━━━━━━━━━
+ Thing
```

```output
received 1 foo
```

### Multiple arguments with one missing

```yumi
/JaCore/tests.h

-- C:ctest13(Thing(), ...)

━━━━━━━━━━━━━━━━━━━━
+ Thing
```

```output
[Binding ctest13]
```

### Multiple arguments with one extra

```yumi crash
/JaCore/tests.h

C:ctest13(Thing(), "foo", "bar")

━━━━━━━━━━━━━━━━━━━━
+ Thing
```

```output
Exception: No C function "ctest13" accepts (Thing, String, String); it is declared as "ctest13(CPointer'I8, CPointer'Char8)"
<no file:4>
    C:ctest13(Thing(), "foo", "bar")
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Returning null from C to string

```yumi
/JaCore/tests.h

. called = C:ctest14()
-- "'$called'"
```

```output
''
```

```yumi
/JaCore/tests.h

. thing Thing
C:ctest15(&thing)
-- thing.a
-- thing.b as String

━━━━━━━━━━━━━━━━━━━━
+ Thing is (a Int, b CPointer'Char8)
```

```output
42
hi there
```

### A header read as C++ declares `abs` once for each type; here it is one function.

```yumi
/stdlib.h

-- C:abs(-5)
-- C:llabs(-6)
```

```output
5
6
```

### A C function that may block is called from the main and from a function.

```yumi
/unistd.h
/string.h

-- C:usleep(10)
-- C:getpid() > 0
-- own-pid() > 0
-- length("hello")

━━━━━━━━━━━━━━━━━━━━
+ do own-pid Int

= C:getpid()

━━━━━━━━━━━━━━━━━━━━
+ length U64
    - text String

C:strlen(text)
```

```output
0
true
true
5
```

### null is given where C takes a function pointer.

```yumi
/stdlib.h

-- C:bsearch(null, null, 0, 8, null) == null
```

```output
true
```

### A pointer from C is held, added to, and passed back. An integer is given where C takes a pointer.

```yumi
/stdlib.h
/string.h

. home = C:getenv("HOME")
. rest = home + 1
-- C:strlen(home) - C:strlen(rest)
-- C:strtoull("31", null, 10)
```

```output
1
31
```

### A pointer equals `null` when it holds no address.

```yumi
/string.h

. found = C:strstr("hello", "x")
-- found == null
-- null == found
-- null != found
. held = C:strstr("hello", "l")
-- held == null
-- held != null
-- null == held
-- null != held
. empty Pointer = null
-- empty == null
```

```output
true
true
false
false
true
false
true
true
```

### The address of a local is given to C, which writes through it.

```yumi
/time.h

. t I64
C:time(&t)
-- t > 0
```

```output
true
```

### A pointer prints as the text it leads to when that is C chars, and as its type when it is not.

```yumi
/string.h

. cell I64 = 7
. p CPointer'I64 = &cell
-- p
-- "at $p"
-- C:strstr("hello world", "wor")
. q = C:strstr("hello world", "lo")
-- q
-- "found $q"
```

```output
[CPointer core/number:I64*]
at [CPointer core/number:I64*]
world
lo world
found lo world
```

### A pointer to C chars becomes text where text is wanted, with `as` and without.

```yumi
/string.h

. s String = C:strstr("hello world", "wor")
-- s
-- s.size
. t = C:strstr("hello world", "lo") as String
-- t ++ "!"
-- shout(C:strstr("abc", "b"))
-- (C:strstr("héllo ✓", "llo") as String).length
-- s == "world"

━━━━━━━━━━━━━━━━━━━━
+ shout String
    - text String

text.uppercased
```

```output
world
5
lo world!
BC
5
true
```

### A class that is a pointer to C chars is the text it leads to, and a class that is another pointer prints as that pointer.

```yumi
/stdlib.h

+ CStr is CPointer'Char8
+ Buf'T is CPointer'U8
+ Ref'T is CPointer'T
+ Fixed is CPointer'F64
+ Deep is Fixed

. p CPointer'Char8 = "hello"
. c CStr = p as CStr
-- c
-- "$c!"
. raw = C:malloc(64)
-- raw as Buf'I64
-- raw as Ref'I64
-- raw as Fixed
-- raw as Deep
C:free(raw)
```

```output
hello
hello!
[CPointer core/number:U8*]
[CPointer core/number:I64*]
[CPointer core/number:F64*]
[CPointer core/number:F64*]
```

### A pointer to C chars becomes a class that is String the way it becomes String.

```yumi
-- hexof("ff")
-- hexof("ff") ++ "!"
. h Hex = hexof("0a")
-- h

━━━━━━━━━━━━━━━━━━━━
+ do hexof Hex
    - p CPointer'Char8

= p
```

```output
ff
ff!
0a
```

### A C function as a value of a function type, kept in a local and given to a function.

```yumi
/JaCore/tests.h

. f Take = C:ctest2a
f(1)
run(f)
run(C:ctest2aa)
-- ""

━━━━━━━━━━━━━━━━━━━━
+ Take is () Void
    - a I32

━━━━━━━━━━━━━━━━━━━━
+ do run
    - take Take

take(7)
```

```output
received 1received 7received aa 7
```

### C functions that answer, as values of function types.

```yumi
/JaCore/tests.h

. add = C:ctest5 as Add
-- add(2, 3)
. get = C:ctest4a as Get
-- get()
-- get() + add(1, 1)

━━━━━━━━━━━━━━━━━━━━
+ Add is () I32
    - a I32
    - b I32

━━━━━━━━━━━━━━━━━━━━
+ Get is () I32
```

```output
5
42
44
```

### A C function given one argument, as a value of a function type that takes the other.

```yumi
/JaCore/tests.h

. two I32 = 2
. one = C:ctest5(two, ...) as One
-- one(5)
-- apply(one, 9)
-- apply(C:ctest5(two, ...), 1)

━━━━━━━━━━━━━━━━━━━━
+ One is () I32
    - b I32

━━━━━━━━━━━━━━━━━━━━
+ apply
    - one One
    - n I32

one(n)
```

```output
7
11
3
```

### A C function kept in an optional property of a function type.

```yumi
/JaCore/tests.h

. box = Box()
box.take = C:ctest2a
box.run(3)
box.take = C:ctest2aa
box.run(4)
-- ""

━━━━━━━━━━━━━━━━━━━━
+ Take is () Void
    - a I32

━━━━━━━━━━━━━━━━━━━━
+ Box

+ take Take?

────────────────────
+ do run
    - n I32

. held = take
if held
    held(n)
```

```output
received 3received aa 4
```

### C function values chosen in a loop, and one given where a function is.

```yumi
/JaCore/tests.h

. f Take = C:ctest2a
. g Take = C:ctest2aa
. i I32 = 0
while i < 3
    . pick = if i == 1 then f else g
    pick(i)
    i += 1
-- ""
-- run(C:ctest4a)
-- run(four)

━━━━━━━━━━━━━━━━━━━━
+ Take is () Void
    - a I32

━━━━━━━━━━━━━━━━━━━━
+ Get is () I32

━━━━━━━━━━━━━━━━━━━━
+ run
    - get Get

get() + 1

━━━━━━━━━━━━━━━━━━━━
+ four I32

4
```

```output
received aa 0received 1received aa 2
43
5
```

### Narrow C arguments on the stack

```yumi
/JaCore/tests.h

. small U8 = 7
. other U8 = 9
. wide I16 = -300
C:ctestStackPacked(1, 2, 3, 4, 5, 6, 7, 8, small, other, wide, 123456)
```

```output
h 8 i 7 j 9 k -300 l 123456
```

### A pointer from C that may point at nothing gives its text where it points at some, and the other side where it does not

```yumi
/stdlib.h

. home = C:getenv("HOME") ?? ""
. missing = C:getenv("BRIX_NO_SUCH_VARIABLE") ?? "none"
-- home.length > 0
-- missing
```

```output
true
none
```

### A pointer moved on by a signed count moves back when the count is negative, whatever its width

```yumi
/string.h

. q = C:strstr("hello world", "wor")
. back I32 = -2
. ahead I8 = 1
. step I16 = -1
-- q + back
-- q + ahead
-- q + step
-- q[back] as Int
```

```output
o world
orld
 world
111
```

### A C `unsigned long long` is a U64

```yumi
/stdlib.h

. most = C:strtoull("18446744073709551615", null, 10)
-- most
-- most > 0
```

```output
18446744073709551615
true
```

### A call of `brixBenchCollect` outside the benchmark runtime does nothing

```yumi
/BxCore/BxCore.h

C:brixBenchCollect()
-- "collected"
```

```output
collected
```

### A pointer to a local, kept in another local, writes the local it points to

```yumi
. cell I64 = 7
. pointer CPointer'I64 = &cell
pointer[0] = 9
-- cell
```

```output
9
```

### A constant a C header defines is read by its C name as the number it is written as, and takes a type beside or in place of a value of another integer type as an integer written with no suffix does. A union's value may be one.

```yumi
/stdio.h
/stdlib.h

-- C:EXIT_FAILURE
-- C:EOF
. offset I16 = C:SEEK_END
-- offset + 1
. small U8 = 254
-- small + C:SEEK_END
-- Ending.failure as I32

━━━━━━━━━━━━━━━━━━━━
+ Ending is I32

| success = C:EXIT_SUCCESS
| failure = C:EXIT_FAILURE
```

```output
1
-1
3
0
1
```

### A number becomes a pointer only when asked, so a tuple with a pointer field given one is refused

```yumi compile-error
. address U64 = 5
-- Holder(address).data.address

━━━━━━━━━━━━━━━━━━━━
+ Holder is Tuple

+ data CPointer'Byte
```

```output
Exception: No constructor of Holder accepts (U64); its declared constructors are "Holder(data CPointer'Byte)"
<no file:3>
    -- Holder(address).data.address
       ^^^^^^
```

### A number asked to become a pointer is the pointer at that address

```yumi
. address U64 = 5
-- Holder(address as CPointer'Byte).data.address

━━━━━━━━━━━━━━━━━━━━
+ Holder is Tuple

+ data CPointer'Byte
```

```output
5
```

### A pointer becomes a pointer to another type only when asked, or by way of a pointer to `Void`

```yumi
. bytes = Byte[8]
bytes[0] = 7
. words = bytes.data as CPointer'I32
. opaque CPointer'Void = bytes.data
. back CPointer'Byte = opaque
-- words[0]
-- back[0]
```

```output
7
7
```

### A pointer to one type is not a pointer to another unasked

```yumi compile-error
. bytes = Byte[8]
. words CPointer'I32 = bytes.data
-- words[0]
```

```output
Exception: Expected CPointer'I32 here, but this is CPointer'U8, which becomes CPointer'I32 only when asked; write "as CPointer'I32"
<no file:3>
    . words CPointer'I32 = bytes.data
            ^^^^^^^^^^^^
```

### An object becomes a pointer only to `Void` unasked

```yumi compile-error
. box = Box()
. opaque CPointer'Void = box
. bytes CPointer'Byte = box
-- bytes.address == opaque.address

+ Box
```

```output
Exception: Expected CPointer'U8 here, but this is Box, which becomes CPointer'U8 only when asked; write "as CPointer'U8"
<no file:4>
    . bytes CPointer'Byte = box
            ^^^^^^^^^^^^^
```

### An array becomes a pointer to its own items or to `Void` unasked, and to others only when asked

```yumi
. bytes = Byte[16]
bytes[0] = 3
. own CPointer'Byte = bytes
. opaque CPointer'Void = bytes
. words = bytes as CPointer'I64
-- own[0]
-- words[0]
-- opaque.address == own.address
```

```output
3
3
true
```

### An array is not a pointer to another type unasked

```yumi compile-error
. bytes = Byte[16]
. words CPointer'I64 = bytes
-- words[0]
```

```output
Exception: Expected CPointer'I64 here, but this is Array'U8, which becomes CPointer'I64 only when asked; write "as CPointer'I64"
<no file:3>
    . words CPointer'I64 = bytes
            ^^^^^^^^^^^^
```

### A C function takes a number for a pointer parameter only when asked

```yumi compile-error
/string.h

. address U64 = 5
-- C:strlen(address)
```

```output
Exception: No C function "strlen" accepts (U64); it is declared as "strlen(CPointer'Char8)"
<no file:5>
    -- C:strlen(address)
       ^^^^^^^^^^^^^^^^^
```

### A C function's parameter of an enum type takes a number, as the enum's integer type does

```yumi
/time.h

. spec = Spec[1]
-- C:clock_gettime(C:CLOCK_REALTIME, spec)
-- spec[0].sec > 0

━━━━━━━━━━━━━━━━━━━━
+ Spec is (sec I64, nsec I64)
```

```output
0
true
```

### A collection waits while a thread C created runs a callback, after a callback nested in it has returned, and not once it has returned

```yumi
/pthread.h
/stdlib.h
/unistd.h
/BxGC/BxGC.h

. flags = C:calloc(3, 4) as CPointer'I32
. handle = C:calloc(1, 8) as CPointer'Pointer
C:pthread_create(handle as Pointer, null, run as Start, flags as Pointer)
while flags[0] == 0
    C:usleep(1000)
C:brixCollect()
-- flags[1] == 1
C:pthread_join(handle[0], null)
-- flags[2]
C:brixCollect()

━━━━━━━━━━━━━━━━━━━━
_ Start is (*C) Pointer
    - context Pointer

━━━━━━━━━━━━━━━━━━━━
_ Compare is (*C) I32
    - a CPointer'I32
    - b CPointer'I32

━━━━━━━━━━━━━━━━━━━━
_ ascending I32
    - a CPointer'I32
    - b CPointer'I32

a[0] - b[0]

━━━━━━━━━━━━━━━━━━━━
_ do run Pointer
    - context Pointer

. flags = context as CPointer'I32
. kept = [5, 6, 7]
. values = Array'I32([3, 1, 2])
C:qsort(values, values.length, I32.size, ascending as Compare)
C:brixCollect()
flags[0] = 1
C:usleep(30000)
flags[2] = (kept[0] + kept[1] + kept[2] + values[0]) as I32
flags[1] = 1
= null
```

```output
true
19
```
