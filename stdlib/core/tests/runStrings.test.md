# Strings

```yumi
-- "foo"
```

```output
foo
```

```yumi
-- "foo".length
```

```output
3
```

```yumi
-- "foo".display-width
-- "é".display-width
-- "📁".length
-- "📁".display-width
-- "📁x".display-width
```

```output
3
1
1
2
3
```

```yumi
-- "foo" + "bar"
```

```output
foobar
```

### A number added to a text makes no text of it: write it inside the text.

```yumi crash
-- "foo" + 3
```

```output
Exception: Operator "+" is not defined for String and I64
<no file:2>
    -- "foo" + 3
       ^^^^^^^^^
```

```yumi
. n = "bar"
-- "foo$n"
```

```output
foobar
```

```yumi
. n = 3
-- "foo$n"
```

```output
foo3
```

```yumi
. n = 3
-- type-of "$n"
-- "$n"
```

```output
[TypeInfo String]
3
```

```yumi
. n = 3.5f64
-- "foo$n"
```

```output
foo3.5
```

```yumi crash
-- "foo" - 4
```

```output
Exception: Operator "-" is not defined for String and I64
<no file:2>
    -- "foo" - 4
       ^^^^^^^^^
```

```yumi
-- "foo" ++ "bar"
```

```output
foobar
```

```yumi
. a = "foo"
a ++= "bar"
-- a
```

```output
foobar
```

```yumi
. a = "foo"
a += "bar"
-- a
```

```output
foobar
```

```yumi
. a = "ABC"
-- a[0]
-- a[0] as Int
```

```output
A
65
```

```yumi
. a = ""
-- !a
. b = "x"
-- !b
```

```output
true
false
```

### String representation metadata for literals and all UTF-8 scalar widths.

```yumi
/BxCore/BxString.h

. empty = ""
. ascii = "hello"
. scalars = "Aé漢😀"
-- empty.size
-- empty.length
-- C:brixStringGetFlags(empty)
-- ascii.size
-- ascii.length
-- C:brixStringGetFlags(ascii)
-- scalars.size
-- scalars.length
-- C:brixStringGetFlags(scalars)
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

```yumi
. value = "a\0b"
-- value.size
-- value.length
-- ("x" ++ value).size
```

```output
3
3
4
```

### Char conversion, interpolation, comparison, search, iteration, indexing, case mapping, slicing, and map keys use the byte length, not the first NUL.

```yumi
. value = "x\0y"
. nul = String(0 as Char)
-- nul.size
-- "a${0 as Char}b".length
-- ("a" ++ nul ++ "b").length
-- value == "x"
-- value == "x\0y"
-- "x" < value
-- value > "x"
-- value.find("y")
. count = 0
for char in value
    count += 1
-- count
-- (value[1] as U32)
-- value.uppercased.size
-- value[1 .. < 3].size
. map = Map'String'Int()
map["x"] = 1
map["x\0y"] = 2
-- map["x"]
-- map["x\0y"]
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

```yumi
-- 'a'
-- 'a' < 'b'

━━━━━━━━━━━━━━━━━━━━
+ Char

+ x = 1
```

```output
a
true
```

### An empty search String matches at the start.

```yumi
-- "abc".find("")
-- "abc".starts-with("")
-- "".find("")
```

```output
0
true
0
```

### Runtime construction paths initialize equivalent metadata.

```yumi
/BxCore/BxString.h

. concatenated = "ab" ++ "é"
. sliced = "Aé漢😀"[1 .. 2]
. formatted = "value=${42}"
. repeated = "é" * 3
. upper = "aé".uppercased
. lower = "AÉ".lowercased
-- concatenated.size
-- concatenated.length
-- C:brixStringGetFlags(concatenated)
-- sliced
-- sliced.size
-- sliced.length
-- formatted.size
-- formatted.length
-- C:brixStringGetFlags(formatted)
-- repeated.size
-- repeated.length
-- upper.length
-- lower.length
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

```yumi
/BxCore/BxString.h

. ascii = Array'Char("exact")
. scalars = Array'Char("Aé漢😀")
. embedded = Array'Char(["a"[0], 0 as Char, "b"[0]])
. empty = Char[0]
. empty-string String = C:brixStringFromUtf8Codepoints(empty.data, 0, 0)
-- ascii.slice-string()
-- scalars.slice-string()
-- (embedded.slice-string()[1] as U32)
-- embedded.slice-string().size
-- empty-string == ""
-- empty-string.size
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

```yumi
/JaCore/tests.h
/BxCore/BxString.h

. foreign = C:ctest4d() as String
. embedded-bytes = Byte[4]
embedded-bytes[0] = 97u8
embedded-bytes[1] = 0u8
embedded-bytes[2] = 98u8
embedded-bytes[3] = 0u8
. embedded = (embedded-bytes.data as CPointer'Char8) as String
. malformed-bytes = Byte[2]
malformed-bytes[0] = 255u8
malformed-bytes[1] = 0u8
. malformed = (malformed-bytes.data as CPointer'Char8) as String
. malformed-empty = malformed[0 .. < 0]
-- foreign
-- foreign.size
-- foreign.length
-- C:brixStringGetFlags(foreign)
-- embedded
-- embedded.size
-- embedded.length
-- C:brixStringGetFlags(embedded)
-- malformed.size
-- malformed.length
-- C:brixStringGetFlags(malformed)
-- malformed-empty.size
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

```yumi
/BxCore/BxString.h

. large = "0123456789abcdef" * 65536
-- large.size
-- large.length
-- C:brixStringGetFlags(large)
-- large.find("abcdef", 1048560)
-- large[1048560 .. < 1048576]
```

```output
1048576
1048576
3
1048570
0123456789abcdef
```

```yumi
. a = "ABC"
-- a[2] as Int
```

```output
67
```

### ASCII indexing preserves signed positive indexes and rejects signed negative indexes.

```yumi
do new
    -- read-character("abc", 1)
    -- read-character("abc", 0 - 1)
catch failure
    -- failure

━━━━━━━━━━━━━━━━━━━━
+ read-character Char
    - text String
    - index I64

text[index]
```

```output
b
Index out of bounds (-1 / 3)
```

```yumi
do new
    . a = "ABC"
    -- a[3] as Int
catch failure
    -- failure
```

```output
Index out of bounds (3 / 3)
```

```yumi
do new
    . a = "é"
    -- a[1] as Int
catch failure
    -- failure
```

```output
Index out of bounds (1 / 1)
```

```yumi
-- 65c
-- 65c as Int
```

```output
A
65
```

```yumi
-- 'A'
-- 'A' as Int
-- '\n' as Int
-- '\u20AC' as Int
```

```output
A
65
10
8364
```

```yumi
. a = "foo"
. b = "$a+"
-- b == "foo+"
-- b == "foo-"
```

```output
true
false
```

```yumi
. a = "bar"
-- a[Position.first]
-- a[Position.middle]
-- a[Position.last]
```

```output
b
a
r
```

```yumi
. a = "foobarbaz"
-- a[0 .. < 5]
-- a[0 .. 5]
-- a[5 .. 0]
```

```output
fooba
foobar
foobar
```

```yumi
. a = "foo bar baz"
-- a[Position.first .. < Position.end]
-- a[Position.last .. Position.first]
-- a[0 .. < Position.middle]
```

```output
foo bar baz
foo bar baz
foo b
```

```yumi
. a = "bar"
-- a[1] = Char(65)
```

```output
bAr
```

```yumi
. a = "foo bar baz"
-- a[4 .. < 7] = "BAR"
```

```output
foo BAR baz
```

```yumi
. a = "foo bar baz"
-- a[4 .. 6] = "BAR"
```

```output
foo BAR baz
```

### An index assignment stores the setter's new string back into its variable, property or element.

```yumi
. a = "hello"
a[0] = "J"
a[.last] = "!"
a[.first .. 1] = "Y"
-- a
. h = Holder("abc")
h.name[1] = "X"
-- h.name
. names = ["abc", "def"]
names[1][0] = "Z"
-- names[1]

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ name String
────────────────────
+ This
    - name
```

```output
Yll!
aXc
Zef
```

```yumi
-- "=" * 5
-- "foo" * 5
```

```output
=====
foofoofoofoofoo
```

```yumi
-- text()

+ bar = "BAR"

━━━━━━━━━━━━━━━━━━━━
+ ¶ text

Foo $bar baz
```

```output
Foo BAR baz
```

### Lines that start with a backtick inside a paragraph body are text, not comments.

````yumi
-- fenced()

━━━━━━━━━━━━━━━━━━━━
+ ¶ fenced

```yumi
`quoted`
```
````

````output
```yumi
`quoted`
```
````

### A paragraph with no body is empty, and the member after its divider stays a member.

```yumi
-- "[${Base().text()}]"
-- "[${Sub().text()}]"
-- Base().other()

━━━━━━━━━━━━━━━━━━━━
+ Base

────────────────────
+ ¶ text

────────────────────
+ other Int

1

━━━━━━━━━━━━━━━━━━━━
+ Sub is Base

────────────────────
+ ¶ text

sub text
```

```output
[]
[sub text]
1
```

```yumi
--
    "ñßéåøñţğşž"
    *> each.uppercase
    |> join
```

```output
ÑẞÉÅØÑŢĞŞŽ
```

```yumi
--
    "ÑßÉÅØÑŢĞŞŽ"
    *> each.lowercase
    |> join
```

```output
ñßéåøñţğşž
```

```yumi
-- "ñßéåøñţğşž".uppercased
```

```output
ÑẞÉÅØÑŢĞŞŽ
```

```yumi
-- "ÑẞÉÅØÑŢĞŞŽ".lowercased
```

```output
ñßéåøñţğşž
```

```yumi
-- "ñßéåøñţğşž".decomposed == "ñßéåøñţğşž"
```

```output
false
```

```yumi
-- "ñßéåøñţğşž".decomposed.normalized == "ñßéåøñţğşž"
```

```output
true
```

```yumi
--
    "ñéåñţğşž".decomposed
    ?> each.category != .mark-nonspacing
    |> join
```

```output
neantgsz
```

```yumi
-- "ñßéåøñţğşž"[1 .. 3]
```

```output
ßéå
```

```yumi
-- "━a━a━━b"[1 .. 5]
```

```output
a━a━━
```

```yumi
. n = "a.b.c".find-reverse(".")
-- n
```

```output
3
```

```yumi
-- "e\u0301".normalized == "\u00E9"
-- "\u1EAF".decomposed.normalized == "\u1EAF"
```

```output
true
true
```

```yumi
-- "".trim() == ""
-- " \t\n".trim() == ""
-- "\t hello \n".trim()
```

```output
true
true
hello
```

```yumi
-- "|".join("a--b--".split("--"))
```

```output
a|b|
```

```yumi
do new
    -- ",".join("abc".split(""))
catch failure
    -- failure
```

```output
String split delimiter must not be empty
```

```yumi
do new
    . data = Byte[1]
    data[0] = 195u8
    . output = Char[1]
    . decoded, bytes = decode-utf8-buffer(data, data.size, output)
    -- decoded
catch failure
    -- failure
```

```output
Unable to read UTF8 content
```

```yumi
. count = 0
for char in ""
    count += 1
-- count
```

```output
0
```

```yumi
. source = "Aé漢😀e\u0301"
. count = 0
. checksum = 0 as Size
for char in source
    count += 1
    checksum += char as Size
-- count
-- checksum
```

```output
6
158130
```

```yumi
. source = "Aé漢😀e\u0301" * 100
. count = 0
. checksum = 0 as Size
for char in source
    count += 1
    checksum += char as Size
-- count
-- checksum
```

```output
600
15813000
```

```yumi
. source = "Aé😀"
. first = 0 as Size
. second = 0 as Size
for char in source
    first += char as Size
for char in source
    second += char as Size
-- first
-- second
```

```output
128810
128810
```

```yumi
. pairs = 0
. checksum = 0 as Size
for outer in "Aé"
    for inner in "漢😀"
        pairs += 1
        checksum += (outer as Size) + (inner as Size)
-- pairs
-- checksum
```

```output
4
314520
```

```yumi
. count = 0
for char in "abcdef"
    count += 1
    if count == 3
        break
-- count

. repeated = 0
for char in "abcdef"
    repeated += 1
-- repeated
```

```output
3
6
```

```yumi
. count = 0
. checksum = 0 as Size
for char in ("Aé😀" as Iterable'Char)
    count += 1
    checksum += char as Size
-- count
-- checksum
```

```output
3
128810
```

```yumi
. source = "Aé😀"
. checksum = 0 as Size
for char in source
    checksum += char as Size
-- checksum
-- source[0] as Size
-- source[1] as Size
-- source[2] as Size
```

```output
128810
65
233
128512
```

```yumi
do new
    . source = "Aé😀"
    for char in source
        . seen = char
    -- source[3]
catch failure
    -- failure
```

```output
Index out of bounds (3 / 3)
```

### Arena String copies preserve the 16-byte String ABI, UTF-8 metadata, NUL termination, and ordinary String behavior while allocating only from the selected Arena.

```yumi
/core/arena
/BxCore/BxCore.h
/BxCore/BxString.h
/string.h

. empty-arena = Arena(64)
. ascii-arena = Arena(64)
. scalar-arena = Arena(64)
. managed-before = C:brixArenaManagedAllocationCount()
. empty = empty-arena.copy-string("")
. ascii = ascii-arena.copy-string("hello")
. scalars = scalar-arena.copy-string("Aé漢😀")
. managed-after = C:brixArenaManagedAllocationCount()

-- empty == ""
-- empty.size
-- empty.length
-- C:brixStringGetFlags(empty)
-- C:strlen(empty)
-- empty-arena.active-bytes
-- empty-arena.owns(empty as Pointer)
-- C:brixStringHash(empty, empty.size) == C:brixStringHash("", 0)
-- ascii == "hello"
-- ascii.size
-- ascii.length
-- C:brixStringGetFlags(ascii)
-- C:strlen(ascii)
-- ascii-arena.active-bytes
-- ascii-arena.owns(ascii as Pointer)
-- C:brixStringHash(ascii, ascii.size) == C:brixStringHash("hello", 5)
-- scalars == "Aé漢😀"
-- scalars.size
-- scalars.length
-- C:brixStringGetFlags(scalars)
-- C:strlen(scalars)
-- scalar-arena.active-bytes
-- scalar-arena.owns(scalars as Pointer)
-- C:brixStringHash(scalars, scalars.size) == C:brixStringHash("Aé漢😀", 10)
-- scalars[1 .. 2]
-- "copy=$scalars"
-- managed-after == managed-before

. checksum = 0 as Size
for char in scalars
    checksum += char as Size
-- checksum

empty-arena.release()
ascii-arena.release()
scalar-arena.release()
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

```yumi
/core/arena
/BxCore/BxCore.h
/BxCore/BxString.h
/string.h

. source = "zAé漢😀q".codepoints
. arena = Arena(64)
. managed-before = C:brixArenaManagedAllocationCount()
. empty = source[0 .. < 0].slice-string(arena)
. scalars = source[1 .. < 5].slice-string(arena)
. managed-after = C:brixArenaManagedAllocationCount()

-- empty == ""
-- empty.size
-- empty.length
-- C:brixStringGetFlags(empty)
-- C:strlen(empty)
-- arena.owns(empty as Pointer)
-- scalars == "Aé漢😀"
-- scalars.size
-- scalars.length
-- C:brixStringGetFlags(scalars)
-- C:strlen(scalars)
-- arena.owns(scalars as Pointer)
-- C:brixStringHash(scalars, scalars.size) == C:brixStringHash("Aé漢😀", 10)
-- scalars[1 .. 2]
-- "direct=$scalars"
-- managed-after == managed-before
-- arena.copy-span(source, 1, 5) == scalars
-- source[1 .. < 5].slice-string() == scalars
-- ("zAé漢😀q" as Byte[])[1 .. < 11].slice-string() == scalars
-- source[0 .. < 6].slice-string(1 .. < 5) == scalars
source[2] = 0 as Char
. truncated = source[1 .. < 5].slice-string(arena)
-- truncated == "A"
-- truncated.length == 1
-- C:brixStringGetFlags(truncated) == 3
source[1] = 0 as Char
-- source[1 .. < 5].slice-string(arena) == ""
-- scalars == "Aé漢😀"

arena.release()
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

```yumi
/core/arena

. arena = Arena(64)
do finally
    arena.release()
. attempt = do new
    . source = "abc".codepoints
    . invalid = arena.copy-span(source, 2, 1)
catch failure
    -- "error: $failure"
attempt.wait()
```

```output
error: Arena codepoint span materialization failed
```

### Arena codepoint-span materialization rejects non-scalar Char values.

```yumi
/core/arena

. arena = Arena(64)
do finally
    arena.release()
. attempt = do new
    . source = Array'Char([55296 as Char])
    . invalid = arena.copy-span(source, 0, 1)
catch failure
    -- "error: $failure"
attempt.wait()
```

```output
error: Arena codepoint span materialization failed
```

### Interpolation converts each piece to String before evaluating the next piece.

```yumi
. t = Tag()
-- "${t}${next()} x"

━━━━━━━━━━━━━━━━━━━━
+ Tag

────────────────────
+ do to String

-- "converting tag"
= "tag"

━━━━━━━━━━━━━━━━━━━━
+ do next String

-- "evaluating next"
= "next"
```

```output
converting tag
evaluating next
tagnext x
```

```yumi
-- "toString"
-- "constructor"
-- "__proto__"
-- "hasOwnProperty"
-- "valueOf"
```

```output
toString
constructor
__proto__
hasOwnProperty
valueOf
```

### An operator on two strings is answered by a function of String, or by the opposite of one.

```yumi
-- "a" == "a"
-- "a" == "b"
-- "a" != "a"
-- "a" != "b"
-- "a" < "b"
-- "b" < "a"
-- "a" > "b"
-- "b" > "a"
-- !"x"
-- !""
-- !!"x"
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

```yumi
. s = "a"
s ++= "b"
s ++= 5
-- s
. e String
e ++= "x"
-- e
```

```output
ab5
x
```

### A text literal keeps a NUL byte: in its size, its length and what follows it.

```yumi
/string.h

. value = "a\0b"
-- value.size
-- value.length
-- ("x" ++ value).size
-- value ++ "c" == "a\0bc"
-- value == "a"
-- value != "a\0c"
-- C:strlen(value)
-- "\0".size
-- "${value}!".size
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

```yumi
. n = 4
. a = "abc\
def"
-- a
-- a.size
. b = "\
x${n}\
"
-- b
-- b.size
. c = "\
"
-- c.size
. d = "p\
\
q${n}\
r"
-- d
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

```yumi
-- text()
-- text().size

━━━━━━━━━━━━━━━━━━━━
+ ¶ text

Foo \
\
 baz \
end
```

```output
Foo  baz end
12
```

### Interpolation makes a piece text by its converter, and never by a `++` its type declares.

```yumi
. n = 4
-- "${n} x"
-- "$n x"
-- "${n}"
-- n ++ " x"

━━━━━━━━━━━━━━━━━━━━
+ extend Int

────────────────────
+ this ++ other String

"over"
```

```output
4 x
4 x
4
over
```

### `++` makes no text of a value that is not text: write it inside a text.

```yumi crash
. t = Tag()
-- t ++ next()

━━━━━━━━━━━━━━━━━━━━
+ Tag

────────────────────
+ do to String

-- "converting tag"
= "tag"

━━━━━━━━━━━━━━━━━━━━
+ do next String

-- "evaluating next"
= "next"
```

```output
Exception: Operator "++" is not defined for Tag and String
<no file:3>
    -- t ++ next()
       ^^^^^^^^^^^
```

### Interpolation joins a piece whose class is String as text, without the class's `++`; `++` written calls it.

```yumi
. l Loud = "quiet"
-- l ++ "x"
-- "${l}x"
-- "$l x"
-- "x" ++ l
-- "x${l}"

━━━━━━━━━━━━━━━━━━━━
+ Loud is String

────────────────────
+ this ++ other String

"LOUD"
```

```output
LOUD
quietx
quiet x
xquiet
xquiet
```

### A hex escape takes two digits and a unicode escape four.

```yumi
-- "a\x41b"
-- "a\x41b".size
-- "\x41\x42g"
-- "a\u0041f"
-- "\u00e9e"
-- "a\x00b" == "a\0b"
-- "a\x00b".size
-- "a\u0000b" == "a\0b"
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

```yumi
-- "${a()}${b()}"
-- "--"
-- "x${a()}y${b()}z"

━━━━━━━━━━━━━━━━━━━━
+ TagA (ta) is I32

────────────────────
+ do to String

-- "conv a"
= "<a>"

━━━━━━━━━━━━━━━━━━━━
+ TagB (tb) is I32

────────────────────
+ do to String

-- "conv b"
= "<b>"

━━━━━━━━━━━━━━━━━━━━
+ do a TagA

-- "eval a"
= 1ta

━━━━━━━━━━━━━━━━━━━━
+ do b TagB

-- "eval b"
= 1tb
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

```yumi
. text = "hello world"
-- text[Position.first .. < 5]
-- text[6 ...]
-- text[... 5]
-- text[Position.middle .. < Position.end]
```

```output
hello
world
hello
 world
```

### A slice of a text past its end panics, as a slice of a list does.

```yumi
. attempt = do new
    . text = "héllo"
    -- text[2 .. < 9]
catch failure
    -- failure
attempt.wait()
```

```output
Range out of bounds (8 / 5)
```

### A slice of a text that starts below zero panics rather than make a text of any size.

```yumi
. attempt = do new
    . text = "héllo"
    . start = -1
    -- text[start .. < 2]
catch failure
    -- failure
attempt.wait()
```

```output
Range out of bounds (1 / 5)
```

### Items and ranges of a text are assigned: the text is stored anew, a copy made before keeps the old one, and the assignment gives the new text.

```yumi
. a = "hello"
. b = a
a[0] = "J"
-- a, b
a[1 .. 2] = ""
-- a
a[.last] = " world"
-- a, a.length
-- (a[0] = "") + "!"
-- a
```

```output
(Jello, hello)
Jlo
(Jl world, 8)
l world!
l world
```

### An item of a text held by a property is assigned from outside the object and inside it, and one of a local copy of an argument.

```yumi
. h = Holder("abc")
h.name[1] = "X"
h.rename()
-- h.name
change("xyz")

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ name String

────────────────────
+ This
    - name

────────────────────
+ do rename

name[0] = "Q"
-- name[2] = "R"

━━━━━━━━━━━━━━━━━━━━
+ do change
    - text String

. copy = text
copy[1] = "-"
-- text, copy
```

```output
QXR
QXR
(xyz, x-z)
```

### An item of a text that is an item of a list, and of one that is a field of a struct, is assigned.

```yumi
. names = ["abc", "def"]
names[1][0] = "Z"
-- names[1], names[0]
. pair = Pair("one", "two")
pair.left[0] = "O"
-- pair.left, pair.right

━━━━━━━━━━━━━━━━━━━━
+ Pair is Struct

+ left String
+ right String
```

```output
(Zef, abc)
(One, two)
```

### Items of a text are assigned in a loop, in a local of each turn and by each index of the text.

```yumi
. lines [String]
for word in ["ab", "cd"]
    . copy = word
    copy[1] = "!"
    lines += copy
-- lines
-- shout("abc")

━━━━━━━━━━━━━━━━━━━━
+ do shout String
    - text String

. loud = text
for index in 0 .. < loud.length
    loud[index] = "X"
= loud
```

```output
a!, c!
XXX
```

### Assigning an item past the end of a text panics, as reading one does.

```yumi
. attempt = do new
    . a = "abc"
    a[7] = "!"
    -- a
catch failure
    -- failure
attempt.wait()
```

```output
Index out of bounds (7 / 3)
```

### An item of a text in a list a property holds, and in a list of lists, is assigned.

```yumi
. h = Holder()
h.items[1][1] = "!"
-- h.items
h.mark(0)
-- h.items
. grid = [["ab", "cd"], ["ef"]]
grid[0][1][0] = "C"
-- grid

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ items = ["ab", "cd"]

────────────────────
+ do mark
    - at Int

items[at][0] = "#"
```

```output
ab, c!
#b, c!
ab, Cd, ef
```

### A text is split by loops, by steps taken in turn from two splits, and where nothing lies between delimiters

```yumi
for part in "a,b,,c".split(",")
    -- "[$part]"
-- List("one--two--three".split("--")).length
. left = "x y z".split(" ")
. right = "1-2-3".split("-")
. a = left()
. b = right()
-- "${a ?? "?"}${b ?? "?"}"
a = left()
b = right()
-- "${a ?? "?"}${b ?? "?"}"
for line in "p=1;q=2".split(";")
    for side in line.split("=")
        -- side
-- "|".join("".split(","))
-- "|".join(",".split(","))
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

```yumi
+ Tag is String

. tag Tag = "hello"
. text String = tag
-- text
. label Label = tag as String
-- label is String
. named Label = text
-- named is String
-- "${label}/${named}"

━━━━━━━━━━━━━━━━━━━━
+ Label is Union

| String
| Int
```

```output
hello
true
true
hello/hello
```

```yumi
-- "foobar".find("bob")
-- "foobar".find("bob") == null
```

```output
unknown
true
```

```yumi
-- decode-base64("Zm9v").slice-string()
-- (catch decode-base64("abc")) is SyntaxError
-- (catch decode-base64("Zm=v")) is SyntaxError
-- catch decode-base64("abc")
-- catch decode-base64("Zm=v")
```

```output
foo
true
true
Invalid Base64 length
Invalid Base64 padding
```

```yumi
. empty = DisplayText("")
-- "${empty.is-single-cell} ${empty.codepoint-count} ${empty.display-width}"
. ascii = DisplayText("abc")
-- "${ascii.is-single-cell} ${ascii.width-at(0)} ${ascii.width-at(2)}"
. latin = DisplayText("\u00E9")
-- "${latin.is-single-cell} ${latin.codepoint-count} ${latin.width-at(0)}"
. mixed = DisplayText("a界e\u0301📁")
-- "${mixed.is-single-cell} ${mixed.codepoint-count} ${mixed.display-width}"
for index in 0 .. < mixed.codepoint-count
    -- mixed.width-at(index)
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

```yumi
. narrow = DisplayText("a") + DisplayText("b")
-- "${narrow.is-single-cell} ${narrow.width-at(1)}"
. wide-last = DisplayText("a") + DisplayText("界")
-- "${wide-last.is-single-cell} ${wide-last.width-at(0)} ${wide-last.width-at(1)}"
. wide-first = DisplayText("界") + DisplayText("a")
-- "${wide-first.is-single-cell} ${wide-first.width-at(0)} ${wide-first.width-at(1)}"
```

```output
true 1
false 1 2
false 2 1
```

### An escape of half of a surrogate pair is no character, and is a syntax error.

```yumi crash
-- "a\u00e9b"
-- "a\uD800b"
```

```output
Exception: "\uD800" is half of a surrogate pair, which is no character by itself; write the character itself in the text
<no file:3>
    -- "a\uD800b"
           ^
```

### A Char that holds half of a surrogate pair or a number past the last character makes no text.

```yumi
-- Array'Char(["a"[0], 0xD7FF as Char, 0xE000 as Char, 0x10FFFF as Char]).slice-string().size
do new
    -- "${0xD800 as Char}"
catch failure
    -- failure
do new
    -- "${0x110000 as Char}"
catch failure
    -- failure
do new
    -- Array'Char(["a"[0], 0xDC00 as Char, "b"[0]]).slice-string()
catch failure
    -- failure
```

```output
11
55296 is not a Unicode character
1114112 is not a Unicode character
Array holds a Char that is not a Unicode character
```

### decode-utf8 makes text of UTF-8 bytes, and raises for a stray byte, an overlong form or a surrogate.

```yumi
-- decode-utf8(Array'Byte("héllo"))
-- decode-utf8(Byte[0]).length
-- (catch? decode-utf8(Array'Byte([104u8, 255u8]))) ?? "stray byte"
-- (catch? decode-utf8(Array'Byte([192u8, 175u8]))) ?? "overlong"
-- (catch? decode-utf8(Array'Byte([237u8, 160u8, 128u8]))) ?? "surrogate"
. failure = catch decode-utf8(Array'Byte([255u8]))
if failure is SyntaxError
    -- (failure as SyntaxError).message
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

```yumi crash
./lib/glyph

-- glyph
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

```yumi crash
./lib/mark

-- mark
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

```yumi crash
. text = "abc"
text[1] = 'Z'
-- text
"abc"[1] = 'Z'
```

```output
Exception: Assigning an item of a text makes a new text, and nothing holds this one, so the change is lost; assign the item of a text a name holds
<no file:5>
    "abc"[1] = 'Z'
    ^^^^^^^^
```

### A copied text is new memory the collector owns, so it outlives the arena that held the original.

```yumi
/core/arena

. arena = Arena()
. kept = arena.copy-string("café")
. copied = kept.copied
-- arena.owns(kept as Pointer)
-- arena.owns(copied as Pointer)
-- copied == kept
-- copied.length
-- copied.size
arena.release()
-- copied
```

```output
true
false
true
4
5
café
```
