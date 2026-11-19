# Ccalls

```brix
(import /JaCore/tests.h)
(blank)

(main
  (call C:ctest1)
)
```

```output
received!
```

```brix
(import /JaCore/tests.h)
(blank)

(main
  (call C:ctest2a 42)
)
```

```output
received 42
```

```brix
(import /JaCore/tests.h)
(blank)

(main
  (call C:ctest2a true)
)
```

```output
received 1
```

```brix crash
(import /JaCore/tests.h)
(blank)

(main
  (call C:ctest2a (tuple 1 2))
)
```

```output
Exception: No C function "ctest2a" accepts ((I64, I64)); it is declared as "ctest2a(I32)"
<no file:6>
      (call C:ctest2a (tuple 1 2))
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix crash
(import /JaCore/tests.h)
(blank)

(main
  (call C:ctest2a (:with 1))
)
```

```output
Exception: No C function "ctest2a" accepts with I64; it is declared as "ctest2a(I32)"
<no file:6>
      (call C:ctest2a (:with 1))
      ^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix
(import /JaCore/tests.h)
(blank)

(main
  (call C:ctest2a 42)
)
```

```output
received 42
```

```brix
(import /JaCore/tests.h)
(blank)

(main
  (call C:ctest2b true)
)
```

```output
received 1
```

```brix
(import /JaCore/tests.h)
(blank)

(main
  (call C:ctest2c 42.1234f)
)
```

```output
received 42.123400
```

```brix
(import /JaCore/tests.h)
(blank)

(main
  (call C:ctest2d 9223372036854775807i64)
)
```

```output
received 9223372036854775807
```

```brix
(import /JaCore/tests.h)
(blank)

(main
  (call C:ctest2e "foo")
)
```

```output
received 'foo'
```

### An optional text given to a C pointer is the text it holds, or a null pointer where it holds none.

```brix
(import /JaCore/tests.h)
(blank)

(main
  (call give "foo")
  (call give null)
)

(fn-do + give _
  (param text (? String))
  (call C:ctest2g text)
)
```

```output
received 'foo'
received null
```

```brix
(import /JaCore/tests.h)
(blank)

(main
  (let thing (call Thing))
  (call C:ctest2f thing)
)

(type + Thing
  (prop + a I32 (= 48))
)
```

```output
received 48
```

```brix
(import /JaCore/tests.h)
(blank)

(main
  (let thing (call Thing))
  (call C:ctest2f thing)
)

(type + Thing
  (prop + a I32 (= 48))
  (prop + doubled _ (= (* #a 2)))
)
```

```output
received 48
```

```brix
(import /JaCore/tests.h)
(blank)

(main
  (print (call C:ctest4a))
)
```

```output
42
```

```brix
(import /JaCore/tests.h)
(blank)

(main
  (print (call C:ctest4b))
)
```

```output
9223372036854775807
```

```brix
(import /JaCore/tests.h)
(blank)

(main
  (print (call C:ctest4c))
)
```

```output
42.1234
```

## Casting C functions to function types

```brix
(import /JaCore/tests.h)
(blank)

(main
  (let f2 (as C:ctest2a Foo))
  (call f2 42)
)

(type + Foo (is (fn-type Void (param a Int))))
```

```output
received 42
```

```brix
(import /JaCore/tests.h)
(blank)

(main
  (let f1 (as C:ctest2a Foo))
  (call f1 42)
  (blank)
  (let f2 (as C:ctest2aa Foo))
  (call f2 43)
)

(type + Foo (is (fn-type Void (param a Int))))
```

```output
received 42received aa 43
```

```brix crash
(import /JaCore/tests.h)
(blank)

(main
  (let f1 (as C:ctest2a Foo))
  (call f1 1 2)
)

(type + Foo (is (fn-type Void (param a Int))))
```

```output
Exception: Wrong number of arguments
<no file:7>
      (call f1 1 2)
      ^^^^^^^^^^^^^
```

### Cast C function to another function with different return type

```brix
(import /JaCore/tests.h)
(blank)

(main
  (let f1 (as C:ctest4a Foo))
  (print (call f1))
)

(type + Foo (is (fn-type Float)))
```

```output
42
```

```brix
(import /JaCore/tests.h)
(blank)

(main
  (let f1 (as C:ctest2a Foo))
  (call f1 42.5)
)

(type + Foo (is (fn-type Void (param a Float))))
```

```output
received 42
```

```brix
(import /JaCore/tests.h)
(blank)

(main
  (let f1 (as C:ctest2a Foo))
  (call f1 (call Thing))
)

(type + Thing
  (fn-to-expr + I32
    42
  )
)

(type + Foo (is (fn-type Void (param a Thing))))
```

```output
received 42
```

### Cast C function to another function with incompatible return type

```brix crash
(import /JaCore/tests.h)
(blank)

(main
  (let f1 (as C:ctest2a Foo))
  (call f1 (call Thing))
)

(type + Thing)

(type + Foo (is (fn-type Void (param a Thing))))
```

```output
Exception: function ctest2a cannot be cast to Foo; there is no conversion between them
<no file:6>
      (let f1 (as C:ctest2a Foo))
              ^^^^^^^^^^^^^^^^^^
```

### Multiple arguments

```brix
(import /JaCore/tests.h)
(blank)

(main
  (call C:ctest13 (call Thing) "foo")
)

(type + Thing)
```

```output
received 1 foo
```

### Multiple arguments with one missing

```brix
(import /JaCore/tests.h)
(blank)

(main
  (print (call C:ctest13 (call Thing) ...))
)

(type + Thing)
```

```output
[Binding ctest13]
```

### Multiple arguments with one extra

```brix crash
(import /JaCore/tests.h)
(blank)

(main
  (call C:ctest13 (call Thing) "foo" "bar")
)

(type + Thing)
```

```output
Exception: No C function "ctest13" accepts (Thing, String, String); it is declared as "ctest13(CPointer'I8, CPointer'Char8)"
<no file:6>
      (call C:ctest13 (call Thing) "foo" "bar")
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Returning null from C to string

```brix
(import /JaCore/tests.h)
(blank)

(main
  (let called (call C:ctest14))
  (print (str "'" called "'"))
)
```

```output
''
```

```brix
(import /JaCore/tests.h)
(blank)

(main
  (let thing Thing)
  (call C:ctest15 &thing)
  (print (. thing a))
  (print (as (. thing b) String))
)

(type + Thing (is (tuple (: a Int) (: b (of CPointer Char8)))))
```

```output
42
hi there
```

### A header read as C++ declares `abs` once for each type; here it is one function.

```brix
(import /stdlib.h)
(blank)

(main
  (print (call C:abs -5))
  (print (call C:llabs -6))
)
```

```output
5
6
```

### A C function that may block is called from the main and from a function.

```brix
(import /unistd.h)
(import /string.h)
(blank)

(main
  (print (call C:usleep 10))
  (print (> (call C:getpid) 0))
  (print (> (call own-pid) 0))
  (print (call length "hello"))
)

(fn-do + own-pid Int
  (return (call C:getpid))
)

(fn-expr + length U64
  (param text String)
  (call C:strlen text)
)
```

```output
0
true
true
5
```

### null is given where C takes a function pointer.

```brix
(import /stdlib.h)
(blank)

(main
  (print (== (call C:bsearch null null 0 8 null) null))
)
```

```output
true
```

### A pointer from C is held, added to, and passed back. An integer is given where C takes a pointer.

```brix
(import /stdlib.h)
(import /string.h)
(blank)

(main
  (let home (call C:getenv "HOME"))
  (let rest (+ home 1))
  (print (- (call C:strlen home) (call C:strlen rest)))
  (print (call C:strtoull "31" null 10))
)
```

```output
1
31
```

### A pointer equals `null` when it holds no address.

```brix
(import /string.h)
(blank)

(main
  (let found (call C:strstr "hello" "x"))
  (print (== found null))
  (print (== null found))
  (print (!= null found))
  (let held (call C:strstr "hello" "l"))
  (print (== held null))
  (print (!= held null))
  (print (== null held))
  (print (!= null held))
  (let empty Pointer null)
  (print (== empty null))
)
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

```brix
(import /time.h)
(blank)

(main
  (let t I64)
  (call C:time &t)
  (print (> t 0))
)
```

```output
true
```

### A pointer prints as the text it leads to when that is C chars, and as its type when it is not.

```brix
(import /string.h)
(blank)

(main
  (let cell I64 7)
  (let p (of CPointer I64) &cell)
  (print p)
  (print (str "at " p))
  (print (call C:strstr "hello world" "wor"))
  (let q (call C:strstr "hello world" "lo"))
  (print q)
  (print (str "found " q))
)
```

```output
[CPointer core/number:I64*]
at [CPointer core/number:I64*]
world
lo world
found lo world
```

### A pointer to C chars becomes text where text is wanted, with `as` and without.

```brix
(import /string.h)
(blank)

(main
  (let s String (call C:strstr "hello world" "wor"))
  (print s)
  (print (. s size))
  (let t (as (call C:strstr "hello world" "lo") String))
  (print (++ t "!"))
  (print (call shout (call C:strstr "abc" "b")))
  (print (. (as (call C:strstr "héllo ✓" "llo") String) length))
  (print (== s "world"))
)

(fn-expr + shout String
  (param text String)
  (. text uppercased)
)
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

```brix
(import /stdlib.h)
(blank)
(type-line + CStr (is (of CPointer Char8)))
(type-line + (of Buf T) (is (of CPointer U8)))
(type-line + (of Ref T) (is (of CPointer T)))
(type-line + Fixed (is (of CPointer F64)))
(type-line + Deep (is Fixed))
(blank)

(main
  (let p (of CPointer Char8) "hello")
  (let c CStr (as p CStr))
  (print c)
  (print (str c "!"))
  (let raw (call C:malloc 64))
  (print (as raw (of Buf I64)))
  (print (as raw (of Ref I64)))
  (print (as raw Fixed))
  (print (as raw Deep))
  (call C:free raw)
)
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

```brix
(main
  (print (call hexof "ff"))
  (print (++ (call hexof "ff") "!"))
  (let h Hex (call hexof "0a"))
  (print h)
)

(fn-do + hexof Hex
  (param p (of CPointer Char8))
  (return p)
)
```

```output
ff
ff!
0a
```

### A C function as a value of a function type, kept in a local and given to a function.

```brix
(import /JaCore/tests.h)
(blank)

(main
  (let f Take C:ctest2a)
  (call f 1)
  (call run f)
  (call run C:ctest2aa)
  (print "")
)

(type + Take (is (fn-type Void (param a I32))))

(fn-do + run _
  (param take Take)
  (call take 7)
)
```

```output
received 1received 7received aa 7
```

### C functions that answer, as values of function types.

```brix
(import /JaCore/tests.h)
(blank)

(main
  (let add (as C:ctest5 Add))
  (print (call add 2 3))
  (let get (as C:ctest4a Get))
  (print (call get))
  (print (+ (call get) (call add 1 1)))
)

(type + Add (is (fn-type I32 (param a I32) (param b I32))))

(type + Get (is (fn-type I32)))
```

```output
5
42
44
```

### A C function given one argument, as a value of a function type that takes the other.

```brix
(import /JaCore/tests.h)
(blank)

(main
  (let two I32 2)
  (let one (as (call C:ctest5 two ...) One))
  (print (call one 5))
  (print (call apply one 9))
  (print (call apply (call C:ctest5 two ...) 1))
)

(type + One (is (fn-type I32 (param b I32))))

(fn-expr + apply _
  (param one One)
  (param n I32)
  (call one n)
)
```

```output
7
11
3
```

### A C function kept in an optional property of a function type.

```brix
(import /JaCore/tests.h)
(blank)

(main
  (let box (call Box))
  (= (. box take) C:ctest2a)
  (call (. box run) 3)
  (= (. box take) C:ctest2aa)
  (call (. box run) 4)
  (print "")
)

(type + Take (is (fn-type Void (param a I32))))

(type + Box
  (prop + take (? Take))
  (fn-do + run _
    (param n I32)
    (let held take)
    (if held
      (call held n)
    )
  )
)
```

```output
received 3received aa 4
```

### C function values chosen in a loop, and one given where a function is.

```brix
(import /JaCore/tests.h)
(blank)

(main
  (let f Take C:ctest2a)
  (let g Take C:ctest2aa)
  (let i I32 0)
  (while (< i 3)
    (let pick (if-expr (== i 1) f g))
    (call pick i)
    (+= i 1)
  )
  (print "")
  (print (call run C:ctest4a))
  (print (call run four))
)

(type + Take (is (fn-type Void (param a I32))))

(type + Get (is (fn-type I32)))

(fn-expr + run _
  (param get Get)
  (+ (call get) 1)
)

(fn-expr + four I32
  4
)
```

```output
received aa 0received 1received aa 2
43
5
```

### Narrow C arguments on the stack

```brix
(import /JaCore/tests.h)
(blank)

(main
  (let small U8 7)
  (let other U8 9)
  (let wide I16 -300)
  (call C:ctestStackPacked 1 2 3 4 5 6 7 8 small other wide 123456)
)
```

```output
h 8 i 7 j 9 k -300 l 123456
```

### A pointer from C that may point at nothing gives its text where it points at some, and the other side where it does not

```brix
(import /stdlib.h)
(blank)

(main
  (let home (?? (call C:getenv "HOME") ""))
  (let missing (?? (call C:getenv "BRIX_NO_SUCH_VARIABLE") "none"))
  (print (> (. home length) 0))
  (print missing)
)
```

```output
true
none
```

### A pointer moved on by a signed count moves back when the count is negative, whatever its width

```brix
(import /string.h)
(blank)

(main
  (let q (call C:strstr "hello world" "wor"))
  (let back I32 -2)
  (let ahead I8 1)
  (let step I16 -1)
  (print (+ q back))
  (print (+ q ahead))
  (print (+ q step))
  (print (as ([] q back) Int))
)
```

```output
o world
orld
 world
111
```

### A C `unsigned long long` is a U64

```brix
(import /stdlib.h)
(blank)

(main
  (let most (call C:strtoull "18446744073709551615" null 10))
  (print most)
  (print (> most 0))
)
```

```output
18446744073709551615
true
```

### A call of `brixBenchCollect` outside the benchmark runtime does nothing

```brix
(import /BxCore/BxCore.h)
(blank)

(main
  (call C:brixBenchCollect)
  (print "collected")
)
```

```output
collected
```

### A pointer to a local, kept in another local, writes the local it points to

```brix
(main
  (let cell I64 7)
  (let pointer (of CPointer I64) &cell)
  (= ([] pointer 0) 9)
  (print cell)
)
```

```output
9
```

### A constant a C header defines is read by its C name as the number it is written as, and takes a type beside or in place of a value of another integer type as an integer written with no suffix does. A union's value may be one.

```brix
(import /stdio.h)
(import /stdlib.h)
(blank)

(main
  (print C:EXIT_FAILURE)
  (print C:EOF)
  (let offset I16 C:SEEK_END)
  (print (+ offset 1))
  (let small U8 254)
  (print (+ small C:SEEK_END))
  (print (as (. Ending failure) I32))
)

(type + Ending (is I32)
  (variant success (= C:EXIT_SUCCESS))
  (variant failure (= C:EXIT_FAILURE))
)
```

```output
1
-1
3
0
1
```

### A number becomes a pointer only when asked, so a tuple with a pointer field given one is refused

```brix compile-error
(main
  (let address U64 5)
  (print (. (call Holder address) data address))
)

(type + Holder (is Tuple)
  (prop + data (of CPointer Byte))
)
```

```output
Exception: No constructor of Holder accepts (U64); its declared constructors are "Holder(data CPointer'Byte)"
<no file:4>
      (print (. (call Holder address) data address))
                      ^^^^^^
```

### A number asked to become a pointer is the pointer at that address

```brix
(main
  (let address U64 5)
  (print (. (call Holder (as address (of CPointer Byte))) data address))
)

(type + Holder (is Tuple)
  (prop + data (of CPointer Byte))
)
```

```output
5
```

### A pointer becomes a pointer to another type only when asked, or by way of a pointer to `Void`

```brix
(main
  (let bytes ([] Byte 8))
  (= ([] bytes 0) 7)
  (let words (as (. bytes data) (of CPointer I32)))
  (let opaque (of CPointer Void) (. bytes data))
  (let back (of CPointer Byte) opaque)
  (print ([] words 0))
  (print ([] back 0))
)
```

```output
7
7
```

### A pointer to one type is not a pointer to another unasked

```brix compile-error
(main
  (let bytes ([] Byte 8))
  (let words (of CPointer I32) (. bytes data))
  (print ([] words 0))
)
```

```output
Exception: Expected CPointer'I32 here, but this is CPointer'U8, which becomes CPointer'I32 only when asked; write "(as (. bytes data) (of CPointer I32))"
<no file:4>
      (let words (of CPointer I32) (. bytes data))
                 ^^^^^^^^^^^^^^^^^
```

### An object becomes a pointer only to `Void` unasked

```brix compile-error
(main
  (let box (call Box))
  (let opaque (of CPointer Void) box)
  (let bytes (of CPointer Byte) box)
  (print (== (. bytes address) (. opaque address)))
)

(blank)
(type-line + Box)
```

```output
Exception: Expected CPointer'U8 here, but this is Box, which becomes CPointer'U8 only when asked; write "(as box (of CPointer U8))"
<no file:5>
      (let bytes (of CPointer Byte) box)
                 ^^^^^^^^^^^^^^^^^^
```

### An array becomes a pointer to its own items or to `Void` unasked, and to others only when asked

```brix
(main
  (let bytes ([] Byte 16))
  (= ([] bytes 0) 3)
  (let own (of CPointer Byte) bytes)
  (let opaque (of CPointer Void) bytes)
  (let words (as bytes (of CPointer I64)))
  (print ([] own 0))
  (print ([] words 0))
  (print (== (. opaque address) (. own address)))
)
```

```output
3
3
true
```

### An array is not a pointer to another type unasked

```brix compile-error
(main
  (let bytes ([] Byte 16))
  (let words (of CPointer I64) bytes)
  (print ([] words 0))
)
```

```output
Exception: Expected CPointer'I64 here, but this is Array'U8, which becomes CPointer'I64 only when asked; write "(as bytes (of CPointer I64))"
<no file:4>
      (let words (of CPointer I64) bytes)
                 ^^^^^^^^^^^^^^^^^
```

### A C function takes a number for a pointer parameter only when asked

```brix compile-error
(import /string.h)
(blank)

(main
  (let address U64 5)
  (print (call C:strlen address))
)
```

```output
Exception: No C function "strlen" accepts (U64); it is declared as "strlen(CPointer'Char8)"
<no file:7>
      (print (call C:strlen address))
             ^^^^^^^^^^^^^^^^^^^^^^^
```

### A C function's parameter of an enum type takes a number, as the enum's integer type does

```brix
(import /time.h)
(blank)

(main
  (let spec ([] Spec 1))
  (print (call C:clock_gettime C:CLOCK_REALTIME spec))
  (print (> (. ([] spec 0) sec) 0))
)

(type + Spec (is (tuple (: sec I64) (: nsec I64))))
```

```output
0
true
```

### A collection waits while a thread C created runs a callback, after a callback nested in it has returned, and not once it has returned

```brix
(import /pthread.h)
(import /stdlib.h)
(import /unistd.h)
(import /BxGC/BxGC.h)
(blank)

(main
  (let flags (as (call C:calloc 3 4) (of CPointer I32)))
  (let handle (as (call C:calloc 1 8) (of CPointer Pointer)))
  (call C:pthread_create (as handle Pointer) null (as run Start) (as flags Pointer))
  (while (== ([] flags 0) 0)
    (call C:usleep 1000)
  )
  (call C:brixCollect)
  (print (== ([] flags 1) 1))
  (call C:pthread_join ([] handle 0) null)
  (print ([] flags 2))
  (call C:brixCollect)
)

(type _ Start (is (c-fn-type Pointer (param context Pointer))))

(type _ Compare (is (c-fn-type I32 (param a (of CPointer I32)) (param b (of CPointer I32)))))

(fn-expr _ ascending I32
  (param a (of CPointer I32))
  (param b (of CPointer I32))
  (- ([] a 0) ([] b 0))
)

(fn-do _ run Pointer
  (param context Pointer)
  (let flags (as context (of CPointer I32)))
  (let kept (list 5 6 7))
  (let values (call (of Array I32) (list 3 1 2)))
  (call C:qsort values (. values length) (. I32 size) (as ascending Compare))
  (call C:brixCollect)
  (= ([] flags 0) 1)
  (call C:usleep 30000)
  (= ([] flags 2) (as (+ ([] kept 0) ([] kept 1) ([] kept 2) ([] values 0)) I32))
  (= ([] flags 1) 1)
  (return null)
)
```

```output
true
19
```
