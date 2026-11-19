# Strings

```brix
(main
  (print "foo")
)
```

```output
foo
```

```brix
(main
  (print (. "foo" length))
)
```

```output
3
```

```brix
(main
  (print (. "foo" display-width))
  (print (. "é" display-width))
  (print (. "📁" length))
  (print (. "📁" display-width))
  (print (. "📁x" display-width))
)
```

```output
3
1
1
2
3
```

```brix
(main
  (print (+ "foo" "bar"))
)
```

```output
foobar
```

### A number added to a text makes no text of it: write it inside the text.

```brix crash
(main
  (print (+ "foo" 3))
)
```

```output
Exception: Operator "+" is not defined for String and I64
<no file:3>
      (print (+ "foo" 3))
             ^^^^^^^^^^^
```

```brix
(main
  (let n "bar")
  (print (str "foo" n))
)
```

```output
foobar
```

```brix
(main
  (let n 3)
  (print (str "foo" n))
)
```

```output
foo3
```

```brix
(main
  (let n 3)
  (print (type-of (str n)))
  (print (str n))
)
```

```output
[TypeInfo String]
3
```

```brix
(main
  (let n 3.5f64)
  (print (str "foo" n))
)
```

```output
foo3.5
```

```brix crash
(main
  (print (- "foo" 4))
)
```

```output
Exception: Operator "-" is not defined for String and I64
<no file:3>
      (print (- "foo" 4))
             ^^^^^^^^^^^
```

```brix
(main
  (print (++ "foo" "bar"))
)
```

```output
foobar
```

```brix
(main
  (let a "foo")
  (++= a "bar")
  (print a)
)
```

```output
foobar
```

```brix
(main
  (let a "foo")
  (+= a "bar")
  (print a)
)
```

```output
foobar
```

```brix
(main
  (let a "ABC")
  (print ([] a 0))
  (print (as ([] a 0) Int))
)
```

```output
A
65
```

```brix
(main
  (let a "")
  (print (not a))
  (let b "x")
  (print (not b))
)
```

```output
true
false
```

### String representation metadata for literals and all UTF-8 scalar widths.

```brix
(import /BxCore/BxString.h)
(blank)

(main
  (let empty "")
  (let ascii "hello")
  (let scalars "Aé漢😀")
  (print (. empty size))
  (print (. empty length))
  (print (call C:brixStringGetFlags empty))
  (print (. ascii size))
  (print (. ascii length))
  (print (call C:brixStringGetFlags ascii))
  (print (. scalars size))
  (print (. scalars length))
  (print (call C:brixStringGetFlags scalars))
)
```

```output
0
0
3
5
5
3
10
4
2
```

### String literals keep NUL bytes across metadata and concatenation.

```brix
(main
  (let value "a\x00b")
  (print (. value size))
  (print (. value length))
  (print (. (++ "x" value) size))
)
```

```output
3
3
4
```

### Char conversion, interpolation, comparison, search, iteration, indexing, case mapping, slicing, and map keys use the byte length, not the first NUL.

```brix
(main
  (let value "x\x00y")
  (let nul (call String (as 0 Char)))
  (print (. nul size))
  (print (. (str "a" (as 0 Char) "b") length))
  (print (. (++ "a" nul "b") length))
  (print (== value "x"))
  (print (== value "x\x00y"))
  (print (< "x" value))
  (print (> value "x"))
  (print (call (. value find) "y"))
  (let count 0)
  (for (in char value)
    (+= count 1)
  )
  (print count)
  (print (as ([] value 1) U32))
  (print (. value uppercased size))
  (print (. ([] value (..< 1 3)) size))
  (let map (call (of Map String Int)))
  (= ([] map "x") 1)
  (= ([] map "x\x00y") 2)
  (print ([] map "x"))
  (print ([] map "x\x00y"))
)
```

```output
1
3
3
false
true
true
true
2
3
0
3
2
1
2
```

### A character literal is a Char whatever a module declares, even a class named Char.

```brix
(main
  (print 'a')
  (print (< 'a' 'b'))
)

(type + Char
  (prop + x _ (= 1))
)
```

```output
a
true
```

### An empty search String matches at the start.

```brix
(main
  (print (call (. "abc" find) ""))
  (print (call (. "abc" starts-with) ""))
  (print (call (. "" find) ""))
)
```

```output
0
true
0
```

### Runtime construction paths initialize equivalent metadata.

```brix
(import /BxCore/BxString.h)
(blank)

(main
  (let concatenated (++ "ab" "é"))
  (let sliced ([] "Aé漢😀" (.. 1 2)))
  (let formatted (str "value=" 42))
  (let repeated (* "é" 3))
  (let upper (. "aé" uppercased))
  (let lower (. "AÉ" lowercased))
  (print (. concatenated size))
  (print (. concatenated length))
  (print (call C:brixStringGetFlags concatenated))
  (print sliced)
  (print (. sliced size))
  (print (. sliced length))
  (print (. formatted size))
  (print (. formatted length))
  (print (call C:brixStringGetFlags formatted))
  (print (. repeated size))
  (print (. repeated length))
  (print (. upper length))
  (print (. lower length))
)
```

```output
4
3
2
é漢
5
2
8
8
3
6
3
2
2
```

### Codepoint slices preserve exact-fit ASCII, scalar widths, embedded NUL, and emptiness.

```brix
(import /BxCore/BxString.h)
(blank)

(main
  (let ascii (call (of Array Char) "exact"))
  (let scalars (call (of Array Char) "Aé漢😀"))
  (let embedded (call (of Array Char) (list ([] "a" 0) (as 0 Char) ([] "b" 0))))
  (let empty ([] Char 0))
  (let empty-string String (call C:brixStringFromUtf8Codepoints (. empty data) 0 0))
  (print (call (. ascii slice-string)))
  (print (call (. scalars slice-string)))
  (print (as ([] (call (. embedded slice-string)) 1) U32))
  (print (. (call (. embedded slice-string)) size))
  (print (== empty-string ""))
  (print (. empty-string size))
)
```

```output
exact
Aé漢😀
0
3
true
0
```

### Foreign adoption preserves NUL termination and never marks malformed UTF-8 as ASCII.

```brix
(import /JaCore/tests.h)
(import /BxCore/BxString.h)
(blank)

(main
  (let foreign (as (call C:ctest4d) String))
  (let embedded-bytes ([] Byte 4))
  (= ([] embedded-bytes 0) 97u8)
  (= ([] embedded-bytes 1) 0u8)
  (= ([] embedded-bytes 2) 98u8)
  (= ([] embedded-bytes 3) 0u8)
  (let embedded (as (as (. embedded-bytes data) (of CPointer Char8)) String))
  (let malformed-bytes ([] Byte 2))
  (= ([] malformed-bytes 0) 255u8)
  (= ([] malformed-bytes 1) 0u8)
  (let malformed (as (as (. malformed-bytes data) (of CPointer Char8)) String))
  (let malformed-empty ([] malformed (..< 0 0)))
  (print foreign)
  (print (. foreign size))
  (print (. foreign length))
  (print (call C:brixStringGetFlags foreign))
  (print embedded)
  (print (. embedded size))
  (print (. embedded length))
  (print (call C:brixStringGetFlags embedded))
  (print (. malformed size))
  (print (. malformed length))
  (print (call C:brixStringGetFlags malformed))
  (print (. malformed-empty size))
)
```

```output
foo
3
3
3
a
1
1
3
1
0
0
0
```

### Large ASCII metadata stays exact and powers byte-offset find/slice paths.

```brix
(import /BxCore/BxString.h)
(blank)

(main
  (let large (* "0123456789abcdef" 65536))
  (print (. large size))
  (print (. large length))
  (print (call C:brixStringGetFlags large))
  (print (call (. large find) "abcdef" 1048560))
  (print ([] large (..< 1048560 1048576)))
)
```

```output
1048576
1048576
3
1048570
0123456789abcdef
```

```brix
(main
  (let a "ABC")
  (print (as ([] a 2) Int))
)
```

```output
67
```

### ASCII indexing preserves signed positive indexes and rejects signed negative indexes.

```brix
(main
  (do-new
    (print (call read-character "abc" 1))
    (print (call read-character "abc" (- 0 1)))
    (on-failure failure
      (print failure)
    )
  )
)

(fn-expr + read-character Char
  (param text String)
  (param index I64)
  ([] text index)
)
```

```output
b
Index out of bounds (-1 / 3)
```

```brix
(main
  (do-new
    (let a "ABC")
    (print (as ([] a 3) Int))
    (on-failure failure
      (print failure)
    )
  )
)
```

```output
Index out of bounds (3 / 3)
```

```brix
(main
  (do-new
    (let a "é")
    (print (as ([] a 1) Int))
    (on-failure failure
      (print failure)
    )
  )
)
```

```output
Index out of bounds (1 / 1)
```

```brix
(main
  (print 65c)
  (print (as 65c Int))
)
```

```output
A
65
```

```brix
(main
  (print 'A')
  (print (as 'A' Int))
  (print (as (char "\n") Int))
  (print (as (char "€") Int))
)
```

```output
A
65
10
8364
```

```brix
(main
  (let a "foo")
  (let b (str a "+"))
  (print (== b "foo+"))
  (print (== b "foo-"))
)
```

```output
true
false
```

```brix
(main
  (let a "bar")
  (print ([] a (. Position first)))
  (print ([] a (. Position middle)))
  (print ([] a (. Position last)))
)
```

```output
b
a
r
```

```brix
(main
  (let a "foobarbaz")
  (print ([] a (..< 0 5)))
  (print ([] a (.. 0 5)))
  (print ([] a (.. 5 0)))
)
```

```output
fooba
foobar
foobar
```

```brix
(main
  (let a "foo bar baz")
  (print ([] a (..< (. Position first) (. Position end))))
  (print ([] a (.. (. Position last) (. Position first))))
  (print ([] a (..< 0 (. Position middle))))
)
```

```output
foo bar baz
foo bar baz
foo b
```

```brix
(main
  (let a "bar")
  (print (= ([] a 1) (call Char 65)))
)
```

```output
bAr
```

```brix
(main
  (let a "foo bar baz")
  (print (= ([] a (..< 4 7)) "BAR"))
)
```

```output
foo BAR baz
```

```brix
(main
  (let a "foo bar baz")
  (print (= ([] a (.. 4 6)) "BAR"))
)
```

```output
foo BAR baz
```

### An index assignment stores the setter's new string back into its variable, property or element.

```brix
(main
  (let a "hello")
  (= ([] a 0) "J")
  (= ([] a .last) "!")
  (= ([] a (.. .first 1)) "Y")
  (print a)
  (let h (call Holder "abc"))
  (= ([] (. h name) 1) "X")
  (print (. h name))
  (let names (list "abc" "def"))
  (= ([] ([] names 1) 0) "Z")
  (print ([] names 1))
)

(type + Holder
  (prop + name String)
  (fn-do + This _
    (param name _)
  )
)
```

```output
Yll!
aXc
Zef
```

```brix
(main
  (print (* "=" 5))
  (print (* "foo" 5))
)
```

```output
=====
foofoofoofoofoo
```

```brix
(main
  (print (call text))
)

(blank)
(prop + bar _ (= "BAR"))

(fn-expr + text _
  (paragraph
    (str "Foo " bar " baz")
  )
)
```

```output
Foo BAR baz
```

### Lines that start with a backtick inside a paragraph body are text, not comments.

```brix
(main
  (print (call fenced))
)

(fn-expr + fenced _
  (paragraph
    "```yumi"
    "`quoted`"
    "```"
  )
)
```

````output
```yumi
`quoted`
```
````

### A text block is a text written on the rows below a row that ends in a quote that opens it, one line for each row, joined with line feeds. A blank row between lines is an empty line, and blank rows after the last line belong to the code below. Spaces past the block's margin are kept, and inserts and escapes work as in a text body.

```brix
(main
  (let name "Ann")
  (let letter
    (paragraph
      (str "Dear " name ",")
      (blank)
      (str "  Your total is \$" (+ 2 3) ".")
      "` not a comment"
      "━━━━━━━━━━━━━━━━━━━━"
    )
  )
  (let empty (paragraph))
  (print letter)
  (print (str "[" empty "]"))
  (print
    (call shout
      (block
        (arg
          (paragraph
            (blank)
            "two"
          )
        )
      )
    )
  )
  (print
    (++ "joined: "
      (paragraph
        "a"
        "b"
      )
    )
  )
)

(fn-expr + shout String
  (param text String)
  (str "<" text ">")
)
```

```output
Dear Ann,

  Your total is $5.
` not a comment
━━━━━━━━━━━━━━━━━━━━
[]
<
two>
joined: a
b
```

### A paragraph with no body is empty, and the member after its divider stays a member.

```brix
(main
  (print (str "[" (call (. (call Base) text)) "]"))
  (print (str "[" (call (. (call Sub) text)) "]"))
  (print (call (. (call Base) other)))
)

(type + Base
  (fn-expr + text _
    (paragraph)
  )
  (fn-expr + other Int
    1
  )
)

(type + Sub (is Base)
  (fn-expr + text _
    (paragraph
      "sub text"
    )
  )
)
```

```output
[]
[sub text]
1
```

```brix
(main
  (print
    (block
      "ñßéåøñţğşž"
      (pipe *> (. each uppercase))
      (pipe |> join)
    )
  )
)
```

```output
ÑẞÉÅØÑŢĞŞŽ
```

```brix
(main
  (print
    (block
      "ÑßÉÅØÑŢĞŞŽ"
      (pipe *> (. each lowercase))
      (pipe |> join)
    )
  )
)
```

```output
ñßéåøñţğşž
```

```brix
(main
  (print (. "ñßéåøñţğşž" uppercased))
)
```

```output
ÑẞÉÅØÑŢĞŞŽ
```

```brix
(main
  (print (. "ÑẞÉÅØÑŢĞŞŽ" lowercased))
)
```

```output
ñßéåøñţğşž
```

```brix
(main
  (print (== (. "ñßéåøñţğşž" decomposed) "ñßéåøñţğşž"))
)
```

```output
false
```

```brix
(main
  (print (== (. "ñßéåøñţğşž" decomposed normalized) "ñßéåøñţğşž"))
)
```

```output
true
```

```brix
(main
  (print
    (block
      (. "ñéåñţğşž" decomposed)
      (pipe ?> (!= (. each category) .mark-nonspacing))
      (pipe |> join)
    )
  )
)
```

```output
neantgsz
```

```brix
(main
  (print ([] "ñßéåøñţğşž" (.. 1 3)))
)
```

```output
ßéå
```

```brix
(main
  (print ([] "━a━a━━b" (.. 1 5)))
)
```

```output
a━a━━
```

```brix
(main
  (let n (call (. "a.b.c" find-reverse) "."))
  (print n)
)
```

```output
3
```

```brix
(main
  (print (== (. "é" normalized) "é"))
  (print (== (. "ắ" decomposed normalized) "ắ"))
)
```

```output
true
true
```

```brix
(main
  (print (== (call (. "" trim)) ""))
  (print (== (call (. " \t\n" trim)) ""))
  (print (call (. "\t hello \n" trim)))
)
```

```output
true
true
hello
```

```brix
(main
  (print (call (. "|" join) (call (. "a--b--" split) "--")))
)
```

```output
a|b|
```

```brix
(main
  (do-new
    (print (call (. "," join) (call (. "abc" split) "")))
    (on-failure failure
      (print failure)
    )
  )
)
```

```output
String split delimiter must not be empty
```

```brix
(main
  (do-new
    (let data ([] Byte 1))
    (= ([] data 0) 195u8)
    (let output ([] Char 1))
    (let (tuple decoded bytes) (call decode-utf8-buffer data (. data size) output))
    (print decoded)
    (on-failure failure
      (print failure)
    )
  )
)
```

```output
Unable to read UTF8 content
```

```brix
(main
  (let count 0)
  (for (in char "")
    (+= count 1)
  )
  (print count)
)
```

```output
0
```

```brix
(main
  (let source "Aé漢😀é")
  (let count 0)
  (let checksum (as 0 Size))
  (for (in char source)
    (+= count 1)
    (+= checksum (as char Size))
  )
  (print count)
  (print checksum)
)
```

```output
6
158130
```

```brix
(main
  (let source (* "Aé漢😀é" 100))
  (let count 0)
  (let checksum (as 0 Size))
  (for (in char source)
    (+= count 1)
    (+= checksum (as char Size))
  )
  (print count)
  (print checksum)
)
```

```output
600
15813000
```

```brix
(main
  (let source "Aé😀")
  (let first (as 0 Size))
  (let second (as 0 Size))
  (for (in char source)
    (+= first (as char Size))
  )
  (for (in char source)
    (+= second (as char Size))
  )
  (print first)
  (print second)
)
```

```output
128810
128810
```

```brix
(main
  (let pairs 0)
  (let checksum (as 0 Size))
  (for (in outer "Aé")
    (for (in inner "漢😀")
      (+= pairs 1)
      (+= checksum (+ (as outer Size) (as inner Size)))
    )
  )
  (print pairs)
  (print checksum)
)
```

```output
4
314520
```

```brix
(main
  (let count 0)
  (for (in char "abcdef")
    (+= count 1)
    (if (== count 3)
      (break)
    )
  )
  (print count)
  (blank)
  (let repeated 0)
  (for (in char "abcdef")
    (+= repeated 1)
  )
  (print repeated)
)
```

```output
3
6
```

```brix
(main
  (let count 0)
  (let checksum (as 0 Size))
  (for (in char (as "Aé😀" (of Iterable Char)))
    (+= count 1)
    (+= checksum (as char Size))
  )
  (print count)
  (print checksum)
)
```

```output
3
128810
```

```brix
(main
  (let source "Aé😀")
  (let checksum (as 0 Size))
  (for (in char source)
    (+= checksum (as char Size))
  )
  (print checksum)
  (print (as ([] source 0) Size))
  (print (as ([] source 1) Size))
  (print (as ([] source 2) Size))
)
```

```output
128810
65
233
128512
```

```brix
(main
  (do-new
    (let source "Aé😀")
    (for (in char source)
      (let seen char)
    )
    (print ([] source 3))
    (on-failure failure
      (print failure)
    )
  )
)
```

```output
Index out of bounds (3 / 3)
```

### Arena String copies preserve the 16-byte String ABI, UTF-8 metadata, NUL termination, and ordinary String behavior while allocating only from the selected Arena.

```brix
(import /core/arena)
(import /BxCore/BxCore.h)
(import /BxCore/BxString.h)
(import /string.h)
(blank)

(main
  (let empty-arena (call Arena 64))
  (let ascii-arena (call Arena 64))
  (let scalar-arena (call Arena 64))
  (let managed-before (call C:brixArenaManagedAllocationCount))
  (let empty (call (. empty-arena copy-string) ""))
  (let ascii (call (. ascii-arena copy-string) "hello"))
  (let scalars (call (. scalar-arena copy-string) "Aé漢😀"))
  (let managed-after (call C:brixArenaManagedAllocationCount))
  (blank)
  (print (== empty ""))
  (print (. empty size))
  (print (. empty length))
  (print (call C:brixStringGetFlags empty))
  (print (call C:strlen empty))
  (print (. empty-arena active-bytes))
  (print (call (. empty-arena owns) (as empty Pointer)))
  (print (== (call C:brixStringHash empty (. empty size)) (call C:brixStringHash "" 0)))
  (print (== ascii "hello"))
  (print (. ascii size))
  (print (. ascii length))
  (print (call C:brixStringGetFlags ascii))
  (print (call C:strlen ascii))
  (print (. ascii-arena active-bytes))
  (print (call (. ascii-arena owns) (as ascii Pointer)))
  (print (== (call C:brixStringHash ascii (. ascii size)) (call C:brixStringHash "hello" 5)))
  (print (== scalars "Aé漢😀"))
  (print (. scalars size))
  (print (. scalars length))
  (print (call C:brixStringGetFlags scalars))
  (print (call C:strlen scalars))
  (print (. scalar-arena active-bytes))
  (print (call (. scalar-arena owns) (as scalars Pointer)))
  (print (== (call C:brixStringHash scalars (. scalars size)) (call C:brixStringHash "Aé漢😀" 10)))
  (print ([] scalars (.. 1 2)))
  (print (str "copy=" scalars))
  (print (== managed-after managed-before))
  (blank)
  (let checksum (as 0 Size))
  (for (in char scalars)
    (+= checksum (as char Size))
  )
  (print checksum)
  (blank)
  (call (. empty-arena release))
  (call (. ascii-arena release))
  (call (. scalar-arena release))
)
```

```output
true
0
0
3
0
17
true
true
true
5
5
3
5
22
true
true
true
10
4
2
10
27
true
true
é漢
copy=Aé漢😀
true
157260
```

### Arena codepoint-span materialization writes ordinary Strings directly from scalar arrays without creating a managed String.

```brix
(import /core/arena)
(import /BxCore/BxCore.h)
(import /BxCore/BxString.h)
(import /string.h)
(blank)

(main
  (let source (. "zAé漢😀q" codepoints))
  (let arena (call Arena 64))
  (let managed-before (call C:brixArenaManagedAllocationCount))
  (let empty (call (. ([] source (..< 0 0)) slice-string) arena))
  (let scalars (call (. ([] source (..< 1 5)) slice-string) arena))
  (let managed-after (call C:brixArenaManagedAllocationCount))
  (blank)
  (print (== empty ""))
  (print (. empty size))
  (print (. empty length))
  (print (call C:brixStringGetFlags empty))
  (print (call C:strlen empty))
  (print (call (. arena owns) (as empty Pointer)))
  (print (== scalars "Aé漢😀"))
  (print (. scalars size))
  (print (. scalars length))
  (print (call C:brixStringGetFlags scalars))
  (print (call C:strlen scalars))
  (print (call (. arena owns) (as scalars Pointer)))
  (print (== (call C:brixStringHash scalars (. scalars size)) (call C:brixStringHash "Aé漢😀" 10)))
  (print ([] scalars (.. 1 2)))
  (print (str "direct=" scalars))
  (print (== managed-after managed-before))
  (print (== (call (. arena copy-span) source 1 5) scalars))
  (print (== (call (. ([] source (..< 1 5)) slice-string)) scalars))
  (print (== (call (. ([] (as "zAé漢😀q" ([] Byte)) (..< 1 11)) slice-string)) scalars))
  (print (== (call (. ([] source (..< 0 6)) slice-string) (..< 1 5)) scalars))
  (= ([] source 2) (as 0 Char))
  (let truncated (call (. ([] source (..< 1 5)) slice-string) arena))
  (print (== truncated "A"))
  (print (== (. truncated length) 1))
  (print (== (call C:brixStringGetFlags truncated) 3))
  (= ([] source 1) (as 0 Char))
  (print (== (call (. ([] source (..< 1 5)) slice-string) arena) ""))
  (print (== scalars "Aé漢😀"))
  (blank)
  (call (. arena release))
)
```

```output
true
0
0
3
0
true
true
10
4
2
10
true
true
é漢
direct=Aé漢😀
true
true
true
true
true
true
true
true
true
true
```

### Arena codepoint-span materialization rejects reversed bounds.

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena 64))
  (finally
    (call (. arena release))
  )
  (let attempt
    (do-new
      (let source (. "abc" codepoints))
      (let invalid (call (. arena copy-span) source 2 1))
      (on-failure failure
        (print (str "error: " failure))
      )
    )
  )
  (call (. attempt wait))
)
```

```output
error: Arena codepoint span materialization failed
```

### Arena codepoint-span materialization rejects non-scalar Char values.

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena 64))
  (finally
    (call (. arena release))
  )
  (let attempt
    (do-new
      (let source (call (of Array Char) (list (as 55296 Char))))
      (let invalid (call (. arena copy-span) source 0 1))
      (on-failure failure
        (print (str "error: " failure))
      )
    )
  )
  (call (. attempt wait))
)
```

```output
error: Arena codepoint span materialization failed
```

### Interpolation converts each piece to String before evaluating the next piece.

```brix
(main
  (let t (call Tag))
  (print (str t (call next) " x"))
)

(type + Tag
  (fn-to-do + String
    (print "converting tag")
    (return "tag")
  )
)

(fn-do + next String
  (print "evaluating next")
  (return "next")
)
```

```output
converting tag
evaluating next
tagnext x
```

```brix
(main
  (print "toString")
  (print "constructor")
  (print "__proto__")
  (print "hasOwnProperty")
  (print "valueOf")
)
```

```output
toString
constructor
__proto__
hasOwnProperty
valueOf
```

### An operator on two strings is answered by a function of String, or by the opposite of one.

```brix
(main
  (print (== "a" "a"))
  (print (== "a" "b"))
  (print (!= "a" "a"))
  (print (!= "a" "b"))
  (print (< "a" "b"))
  (print (< "b" "a"))
  (print (> "a" "b"))
  (print (> "b" "a"))
  (print (not "x"))
  (print (not ""))
  (print (not (not "x")))
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
false
true
true
```

### `++=` joins a value onto the text of a local.

```brix
(main
  (let s "a")
  (++= s "b")
  (++= s 5)
  (print s)
  (let e String)
  (++= e "x")
  (print e)
)
```

```output
ab5
x
```

### A text literal keeps a NUL byte: in its size, its length and what follows it.

```brix
(import /string.h)
(blank)

(main
  (let value "a\x00b")
  (print (. value size))
  (print (. value length))
  (print (. (++ "x" value) size))
  (print (== (++ value "c") "a\x00bc"))
  (print (== value "a"))
  (print (!= value "a\x00c"))
  (print (call C:strlen value))
  (print (. "\x00" size))
  (print (. (str value "!") size))
)
```

```output
3
3
4
true
false
true
1
1
4
```

### A line continuation in text adds no character.

```brix
(main
  (let n 4)
  (let a "abcdef")
  (print a)
  (print (. a size))
  (let b (str "x" n ""))
  (print b)
  (print (. b size))
  (let c "")
  (print (. c size))
  (let d (str "pq" n "r"))
  (print d)
)
```

```output
abcdef
6
x4
2
0
pq4r
```

### A line continuation in a paragraph body adds no character.

```brix
(main
  (print (call text))
  (print (. (call text) size))
)

(fn-expr + text _
  (paragraph
    "Foo  baz end"
  )
)
```

```output
Foo  baz end
12
```

### Interpolation makes a piece text by its converter, and never by a `++` its type declares.

```brix
(main
  (let n 4)
  (print (str n " x"))
  (print (str n " x"))
  (print (str n))
  (print (++ n " x"))
)

(extend + Int
  (fn-expr + (infix this ++ (param other String)) _
    "over"
  )
)
```

```output
4 x
4 x
4
over
```

### `++` makes no text of a value that is not text: write it inside a text.

```brix crash
(main
  (let t (call Tag))
  (print (++ t (call next)))
)

(type + Tag
  (fn-to-do + String
    (print "converting tag")
    (return "tag")
  )
)

(fn-do + next String
  (print "evaluating next")
  (return "next")
)
```

```output
Exception: Operator "++" is not defined for Tag and String
<no file:4>
      (print (++ t (call next)))
             ^^^^^^^^^^^^^^^^^^
```

### Interpolation joins a piece whose class is String as text, without the class's `++`; `++` written calls it.

```brix
(main
  (let l Loud "quiet")
  (print (++ l "x"))
  (print (str l "x"))
  (print (str l " x"))
  (print (++ "x" l))
  (print (str "x" l))
)

(type + Loud (is String)
  (fn-expr + (infix this ++ (param other String)) _
    "LOUD"
  )
)
```

```output
LOUD
quietx
quiet x
xquiet
xquiet
```

### A hex escape takes two digits and a unicode escape four.

```brix
(main
  (print "aAb")
  (print (. "aAb" size))
  (print "ABg")
  (print "aAf")
  (print "ée")
  (print (== "a\x00b" "a\x00b"))
  (print (. "a\x00b" size))
  (print (== "a\x00b" "a\x00b"))
)
```

```output
aAb
3
ABg
aAf
ée
true
3
true
```

### Interpolation makes each piece text as it comes.

```brix
(main
  (print (str (call a) (call b)))
  (print "--")
  (print (str "x" (call a) "y" (call b) "z"))
)

(type + TagA (alias ta) (is I32)
  (fn-to-do + String
    (print "conv a")
    (return "<a>")
  )
)

(type + TagB (alias tb) (is I32)
  (fn-to-do + String
    (print "conv b")
    (return "<b>")
  )
)

(fn-do + a TagA
  (print "eval a")
  (return 1ta)
)

(fn-do + b TagB
  (print "eval b")
  (return 1tb)
)
```

```output
eval a
conv a
eval b
conv b
<a><b>
--
eval a
conv a
eval b
conv b
x<a>y<b>z
```

### A text is sliced by a range of positions and by ranges with a side left open.

```brix
(main
  (let text "hello world")
  (print ([] text (..< (. Position first) 5)))
  (print ([] text (... 6)))
  (print ([] text (... _ 5)))
  (print ([] text (..< (. Position middle) (. Position end))))
)
```

```output
hello
world
hello
 world
```

### A slice of a text past its end panics, as a slice of a list does.

```brix
(main
  (let attempt
    (do-new
      (let text "héllo")
      (print ([] text (..< 2 9)))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)
```

```output
Range out of bounds (8 / 5)
```

### A slice of a text that starts below zero panics rather than make a text of any size.

```brix
(main
  (let attempt
    (do-new
      (let text "héllo")
      (let start -1)
      (print ([] text (..< start 2)))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)
```

```output
Range out of bounds (1 / 5)
```

### Items and ranges of a text are assigned: the text is stored anew, a copy made before keeps the old one, and the assignment gives the new text.

```brix
(main
  (let a "hello")
  (let b a)
  (= ([] a 0) "J")
  (print (tuple a b))
  (= ([] a (.. 1 2)) "")
  (print a)
  (= ([] a .last) " world")
  (print (tuple a (. a length)))
  (print (+ (= ([] a 0) "") "!"))
  (print a)
)
```

```output
(Jello, hello)
Jlo
(Jl world, 8)
l world!
l world
```

### An item of a text held by a property is assigned from outside the object and inside it, and one of a local copy of an argument.

```brix
(main
  (let h (call Holder "abc"))
  (= ([] (. h name) 1) "X")
  (call (. h rename))
  (print (. h name))
  (call change "xyz")
)

(type + Holder
  (prop + name String)
  (fn-do + This _
    (param name _)
  )
  (fn-do + rename _
    (= ([] name 0) "Q")
    (print (= ([] name 2) "R"))
  )
)

(fn-do + change _
  (param text String)
  (let copy text)
  (= ([] copy 1) "-")
  (print (tuple text copy))
)
```

```output
QXR
QXR
(xyz, x-z)
```

### An item of a text that is an item of a list, and of one that is a field of a struct, is assigned.

```brix
(main
  (let names (list "abc" "def"))
  (= ([] ([] names 1) 0) "Z")
  (print (tuple ([] names 1) ([] names 0)))
  (let pair (call Pair "one" "two"))
  (= ([] (. pair left) 0) "O")
  (print (tuple (. pair left) (. pair right)))
)

(type + Pair (is Struct)
  (prop + left String)
  (prop + right String)
)
```

```output
(Zef, abc)
(One, two)
```

### Items of a text are assigned in a loop, in a local of each turn and by each index of the text.

```brix
(main
  (let lines (list String))
  (for (in word (list "ab" "cd"))
    (let copy word)
    (= ([] copy 1) "!")
    (+= lines copy)
  )
  (print lines)
  (print (call shout "abc"))
)

(fn-do + shout String
  (param text String)
  (let loud text)
  (for (in index (..< 0 (. loud length)))
    (= ([] loud index) "X")
  )
  (return loud)
)
```

```output
a!, c!
XXX
```

### Assigning an item past the end of a text panics, as reading one does.

```brix
(main
  (let attempt
    (do-new
      (let a "abc")
      (= ([] a 7) "!")
      (print a)
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)
```

```output
Index out of bounds (7 / 3)
```

### An item of a text in a list a property holds, and in a list of lists, is assigned.

```brix
(main
  (let h (call Holder))
  (= ([] ([] (. h items) 1) 1) "!")
  (print (. h items))
  (call (. h mark) 0)
  (print (. h items))
  (let grid (list (list "ab" "cd") (list "ef")))
  (= ([] ([] ([] grid 0) 1) 0) "C")
  (print grid)
)

(type + Holder
  (prop + items _ (= (list "ab" "cd")))
  (fn-do + mark _
    (param at Int)
    (= ([] ([] items at) 0) "#")
  )
)
```

```output
ab, c!
#b, c!
ab, Cd, ef
```

### A text is split by loops, by steps taken in turn from two splits, and where nothing lies between delimiters

```brix
(main
  (for (in part (call (. "a,b,,c" split) ","))
    (print (str "[" part "]"))
  )
  (print (. (call List (call (. "one--two--three" split) "--")) length))
  (let left (call (. "x y z" split) " "))
  (let right (call (. "1-2-3" split) "-"))
  (let a (call left))
  (let b (call right))
  (print (str (?? a "?") (?? b "?")))
  (= a (call left))
  (= b (call right))
  (print (str (?? a "?") (?? b "?")))
  (for (in line (call (. "p=1;q=2" split) ";"))
    (for (in side (call (. line split) "="))
      (print side)
    )
  )
  (print (call (. "|" join) (call (. "" split) ",")))
  (print (call (. "|" join) (call (. "," split) ",")))
)
```

```output
[a]
[b]
[]
[c]
3
x1
y2
p
1
q
2

|
```

```brix
(type-line + Tag (is String))
(blank)

(main
  (let tag Tag "hello")
  (let text String tag)
  (print text)
  (let label Label (as tag String))
  (print (is label String))
  (let named Label text)
  (print (is named String))
  (print (str label "/" named))
)

(type + Label (is Union)
  (variant String)
  (variant Int)
)
```

```output
hello
true
true
hello/hello
```

```brix
(main
  (print (call (. "foobar" find) "bob"))
  (print (== (call (. "foobar" find) "bob") null))
)
```

```output
unknown
true
```

```brix
(main
  (print (call (. (call decode-base64 "Zm9v") slice-string)))
  (print (is (catch (call decode-base64 "abc")) SyntaxError))
  (print (is (catch (call decode-base64 "Zm=v")) SyntaxError))
  (print (catch (call decode-base64 "abc")))
  (print (catch (call decode-base64 "Zm=v")))
)
```

```output
foo
true
true
Invalid Base64 length
Invalid Base64 padding
```

```brix
(main
  (let empty (call DisplayText ""))
  (print (str (. empty is-single-cell) " " (. empty codepoint-count) " " (. empty display-width)))
  (let ascii (call DisplayText "abc"))
  (print (str (. ascii is-single-cell) " " (call (. ascii width-at) 0) " " (call (. ascii width-at) 2)))
  (let latin (call DisplayText "é"))
  (print (str (. latin is-single-cell) " " (. latin codepoint-count) " " (call (. latin width-at) 0)))
  (let mixed (call DisplayText "a界é📁"))
  (print (str (. mixed is-single-cell) " " (. mixed codepoint-count) " " (. mixed display-width)))
  (for (in index (..< 0 (. mixed codepoint-count)))
    (print (call (. mixed width-at) index))
  )
)
```

```output
true 0 0
true 1 1
true 1 1
false 5 6
1
2
1
0
2
```

```brix
(main
  (let narrow (+ (call DisplayText "a") (call DisplayText "b")))
  (print (str (. narrow is-single-cell) " " (call (. narrow width-at) 1)))
  (let wide-last (+ (call DisplayText "a") (call DisplayText "界")))
  (print (str (. wide-last is-single-cell) " " (call (. wide-last width-at) 0) " " (call (. wide-last width-at) 1)))
  (let wide-first (+ (call DisplayText "界") (call DisplayText "a")))
  (print (str (. wide-first is-single-cell) " " (call (. wide-first width-at) 0) " " (call (. wide-first width-at) 1)))
)
```

```output
true 1
false 1 2
false 2 1
```

### A Char that holds half of a surrogate pair or a number past the last character makes no text.

```brix
(main
  (print (. (call (. (call (of Array Char) (list ([] "a" 0) (as 0xD7FF Char) (as 0xE000 Char) (as 0x10FFFF Char))) slice-string)) size))
  (do-new
    (print (str (as 0xD800 Char)))
    (on-failure failure
      (print failure)
    )
  )
  (do-new
    (print (str (as 0x110000 Char)))
    (on-failure failure
      (print failure)
    )
  )
  (do-new
    (print (call (. (call (of Array Char) (list ([] "a" 0) (as 0xDC00 Char) ([] "b" 0))) slice-string)))
    (on-failure failure
      (print failure)
    )
  )
)
```

```output
11
55296 is not a Unicode character
1114112 is not a Unicode character
Array holds a Char that is not a Unicode character
```

### decode-utf8 makes text of UTF-8 bytes, and raises for a stray byte, an overlong form or a surrogate.

```brix
(main
  (print (call decode-utf8 (call (of Array Byte) "héllo")))
  (print (. (call decode-utf8 ([] Byte 0)) length))
  (print (?? (catch? (call decode-utf8 (call (of Array Byte) (list 104u8 255u8)))) "stray byte"))
  (print (?? (catch? (call decode-utf8 (call (of Array Byte) (list 192u8 175u8)))) "overlong"))
  (print (?? (catch? (call decode-utf8 (call (of Array Byte) (list 237u8 160u8 128u8)))) "surrogate"))
  (let failure (catch (call decode-utf8 (call (of Array Byte) (list 255u8)))))
  (if (is failure SyntaxError)
    (print (. (as failure SyntaxError) message))
  )
)
```

```output
héllo
0
stray byte
overlong
surrogate
Invalid UTF-8
```

### A character written in Brix is a text of one character; a text of none is an error.

```brix crash
(import ./lib/glyph)
(blank)

(main
  (print glyph)
)
```

```brix file=lib/glyph.bx
(prop + glyph _ (= (char "")))
```

```output
Exception: (char …) takes a text of one character, as in (char "a"), but this one has 0
<lib/glyph.bx:1>
    (prop + glyph _ (= (char "")))
                       ^^^^^^^^
```

### An escape of half of a surrogate pair in Brix text is no character, and is an error.

```brix crash
(import ./lib/mark)
(blank)

(main
  (print mark)
)
```

```brix file=lib/mark.bx
(prop + mark _ (= "a\uDC00b"))
```

```output
Exception: “\uDC00” at line 1, column 21 is half of a surrogate pair, which is no character by itself. Write the character itself in the text
<lib/mark.bx:1>
    (prop + mark _ (= "a\uDC00b"))
                        ^
```

### An item of a text that no name holds cannot be assigned, since the new text it makes would be lost.

```brix crash
(main
  (let text "abc")
  (= ([] text 1) 'Z')
  (print text)
  (= ([] "abc" 1) 'Z')
)
```

```output
Exception: Assigning an item of a text makes a new text, and nothing holds this one, so the change is lost; assign the item of a text a name holds
<no file:6>
      (= ([] "abc" 1) 'Z')
         ^^^^^^^^^^^^
```

### A copied text is new memory the collector owns, so it outlives the arena that held the original.

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (let kept (call (. arena copy-string) "café"))
  (let copied (. kept copied))
  (print (call (. arena owns) (as kept Pointer)))
  (print (call (. arena owns) (as copied Pointer)))
  (print (== copied kept))
  (print (. copied length))
  (print (. copied size))
  (call (. arena release))
  (print copied)
)
```

```output
true
false
true
4
5
café
```
