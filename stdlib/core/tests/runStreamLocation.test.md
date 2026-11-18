# Stream location

```yumi
/core/parsing

-- TokenLocation.size
-- MatchLocation.size
```

```output
8
16
```

### Baseline: successful String input should report each matched line span.

```yumi
match "a\nbc\ndef\n"
    ~ (\line \nl)+

    .. line ~
        ~ [a-z]+
            do -- "line $location"
            ""
```

```output
line ((0, 0), (0, 0))
line ((1, 0), (1, 1))
line ((2, 0), (2, 2))
```

## Direct TokenStream Cursor Rewind

### Rewinding a mark restores the cursor directly, including across newlines.

```yumi
/core/tokenstream

. stream = IndexableTokenStream'Char("ab\nc")
. begin = stream.mark()
. buf = Char[4]
stream.read(4, buf)
stream.rewind(begin)

-- "direct-rewind"
-- stream.location
```

```output
direct-rewind
(0, 0)
```

### Indexable marks support nesting, repeated rewind, EOF, and cached-location reuse.

```yumi
/core/tokenstream

. stream = IndexableTokenStream'Char("a\nb")
. outer = stream.mark()
. first = <- stream
-- stream.cursor
. inner = stream.mark()
. second = <- stream
. third = <- stream
. eof = <- stream
-- stream.cursor

stream.rewind(inner)
. rewound-newline = <- stream
-- rewound-newline! as U32
-- stream.cursor
stream.rewind(outer)
-- stream.location
. first-again = <- stream
stream.rewind(outer)
. first-repeated = <- stream
-- stream.cursor
```

```output
1
3
10
2
(0, 0)
1
```

### A mark cannot be used with another stream instance.

```yumi
/core/tokenstream

. attempt = do new
    . first = IndexableTokenStream'Char("a")
    . second = IndexableTokenStream'Char("b")
    first.rewind(second.mark())
catch failure
    -- failure
attempt.wait()
```

```output
Stream mark belongs to another stream
```

### A large direct cursor displacement rewinds to zero and rereads the first token.

```yumi
/core/tokenstream

. stream = IndexableTokenStream'Char(Char[65537])
. begin = stream.mark()
stream.cursor = 65536
stream.rewind(begin)
-- stream.cursor
-- (<- stream)! as U32
```

```output
0
0
```

### Rewinding an outer mark invalidates nested marks beyond the restored cursor.

```yumi
/core/tokenstream

. attempt = do new
    . stream = IndexableTokenStream'Char("ab")
    . outer = stream.mark()
    . item = <- stream
    . inner = stream.mark()
    stream.rewind(outer)
    stream.rewind(inner)
catch failure
    -- failure
attempt.wait()
```

```output
Stream mark is no longer valid
```

### Lazy locations recognize LF, CRLF, and CR without rescanning cached prefixes after rewind.

```yumi
/core/tokenstream

. stream = IndexableTokenStream'Char("a\r\nb\rc\nλ")
. begin = stream.mark()
-- stream.location
while stream.has-more
    . item = <- stream
    -- stream.location

stream.rewind(begin)
-- stream.location
-- stream.get-error-location()
```

```output
(0, 0)
(0, 1)
(0, 2)
(1, 0)
(1, 1)
(2, 0)
(2, 1)
(3, 0)
(3, 1)
(0, 0)
(3, 1)
```

### A mark restores the exact indexable cursor for single and bulk reads.

```yumi
/core/tokenstream

. stream = IndexableTokenStream'Char("abcd")
. begin = stream.mark()
. bulk = Char[3]
. actual = stream.read(3, bulk)
stream.rewind(begin)
-- stream.cursor
-- (<- stream)!

. second = stream.mark()
. item = (<- stream)!
stream.rewind(second)
-- stream.cursor
-- (<- stream)!
```

```output
0
a
1
b
```

### Cached lazy locations preserve repeated, monotonic, backward, Unicode, and high-water queries.

```yumi
/core/tokenstream

. stream = ArrayTokenStream'Char("a\r\nb\rc\nλ".codepoints)
-- stream.location
-- stream.location
. first = <- stream
-- stream.location
. carriage = <- stream
. line-feed = <- stream
. second-line = <- stream
-- stream.location
. backward = stream.mark()
. second-carriage = <- stream
. third-line = <- stream
. third = <- stream
. final-line-feed = <- stream
. unicode = <- stream
-- stream.location
stream.rewind(backward)
-- stream.location
-- stream.get-error-location()
```

```output
(0, 0)
(0, 0)
(0, 1)
(1, 1)
(3, 1)
(1, 1)
(3, 1)
```

### Chunked iterable streams rewind by cursor while preserving their logical position.

```yumi
/core/tokenstream

. stream = ChunkedTokenStream'Char("abcd")
. begin = stream.mark()
. bulk = Char[3]
. actual = stream.read(3, bulk)
stream.rewind(begin)
-- stream.location
-- stream.read(3, bulk)
-- bulk.slice-string()
```

```output
(0, 0)
3
abc
```

### Chunked byte streams rewind by cursor while preserving their logical position.

```yumi
/core/tests/support/byte-source
/core/tokenstream

. data = TestByteSource'Char("abcd".codepoints)
data.rewind()
. stream = ChunkedTokenStream'Char(data)
. begin = stream.mark()
. bulk = Char[3]
. actual = stream.read(3, bulk)
stream.rewind(begin)
-- stream.location
-- stream.read(3, bulk)
-- bulk.slice-string()
```

```output
(0, 0)
3
abc
```

## Pattern Backtracking

### The first alternative consumes "ab\nc" before failing; the second should restart at (0, 0).

```yumi
match "ab\nc"
    ~ (("a" "b" \nl "x") | ("a" "b" \nl "c"))
        do -- "alternate $location"
        ""
```

```output
alternate ((0, 0), (1, 0))
```

### Positive lookahead consumes "ab\n" and puts it back before the real match.

```yumi
match "ab\nc"
    ~ &("a" "b" \nl) "a" "b" \nl "c"
        do -- "lookahead $location"
        ""
```

```output
lookahead ((0, 0), (1, 0))
```

### A failed optional branch should leave the stream location unchanged for the following match.

```yumi
match "ab\nc"
    ~ ("a" "b" \nl "x")? "a" "b" \nl "c"
        do -- "optional $location"
        ""
```

```output
optional ((0, 0), (1, 0))
```

### A failed static combinator branch restores its caller mark before the location-bearing arm.

```yumi
/core/patterns

match "ab;\ncd"
    ~ ((\delimited(",", ~ [a-z]+) "!") | ("ab" ";" \nl "cd"))
        do -- "combinator $location"
        ""
```

```output
combinator ((0, 0), (1, 1))
```

## Error Location

### A body with no pattern uses TokenStream.error-location; it should point at the next unread token.

```yumi
do new
    match "ok\nbad"
        ~ "ok" \nl
        ?! Panic("extra at $location")
catch failure
    -- failure
```

```output
extra at (1, 0)
```

### A choice that skips every alternative, since none can start with the next item, fails where it began.

```yumi
do new
    match "'+"
        ~ "'" (\upper | \bracketed) !.
        ?! Panic("failed at $location")

        .. upper ~
            ~ [A-Z]

        .. bracketed ~
            ~ "[" [a-z] "]"
catch failure
    -- failure
```

```output
failed at (0, 1)
```

## ChunkedTokenStream Byte Inputs

### Numeric U8 patterns exercise ChunkedTokenStream reads one token at a time.

```yumi
/core/tests/support/byte-source
/core/tokenstream

. data = TestByteSource(Array'U8([97u8, 98u8, 10u8, 99u8]))
data.rewind()

. stream = ChunkedTokenStream'U8(data)
match stream
    ~ .{4}
        do -- "buffer-uint8 $location"
        ""
```

```output
buffer-uint8 ((0, 0), (1, 0))
```

### String patterns exercise ChunkedTokenStream.read(size) bulk reads for Char streams.

```yumi
/core/tests/support/byte-source
/core/tokenstream

. data = TestByteSource'Char("ab\nc".codepoints)
. stream = ChunkedTokenStream'Char(data)
match stream
    ~ "ab\nc"
        do -- "buffer-char $location"
        ""
```

```output
buffer-char ((0, 0), (1, 0))
```

```yumi
/core/tokenstream

. stream = ArrayTokenStream'Char("!é\n東京😀?".codepoints)
. literal = "é\n東京😀"
-- match stream
    ~ "!" span \runtime-location(literal) "?"
        span
-- stream.cursor
-- stream.cursor-extent

━━━━━━━━━━━━━━━━━━━━
+ runtime-location ~ Char
    - literal String

~ $literal
    location
```

```output
((0, 1), (1, 2))
7
7
```

```yumi
/BxGC/BxGC.h

match "xfoo\nxxfooo\nfo\n"
    ~ ("x"* \foo \nl)+

    .. foo ~
        ~ "f" "o"+
            do -- after-collection(location)
            ""

━━━━━━━━━━━━━━━━━━━━
+ do after-collection MatchLocation
    - location MatchLocation

C:brixCollect()
= location
```

```output
((0, 1), (0, 3))
((1, 2), (1, 5))
((2, 0), (2, 1))
```

```yumi
/core/parsing

. result = match ""
    ~ \empty
?! Panic("empty match failed")
    if !result

-- "${result.begin.line}:${result.begin.column}-${result.end.line}:${result.end.column}"

━━━━━━━━━━━━━━━━━━━━
+ empty ~ Char

~ ""
    location
```

```output
0:0-0:0
```

### A function that the expression of a rule's arm makes reads the arm's `location`.

```yumi
/stream/aggregate

for text in ["a,b,c", "a"]
    -- match text
        ~ found \words
            found
        ?? "none"

━━━━━━━━━━━━━━━━━━━━
+ words ~ Char String

~ head \word tail ("," \word)*
    reduce • seed head • stream tail • using :: left, right =>
        "($left $right ${location.end.column})"

.. word String ~
    ~ w [a-z]+
        "<$w>"
```

```output
((<a> <b> 4) <c> 4)
<a>
```
