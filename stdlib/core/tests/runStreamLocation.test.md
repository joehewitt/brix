# Stream location

```brix
(import /core/parsing)
(blank)

(main
  (print (. TokenLocation size))
  (print (. MatchLocation size))
)
```

```output
8
16
```

### Baseline: successful String input should report each matched line span.

```brix
(main
  (match "a\nbc\ndef\n"
    (~ (+ (seq \line \nl)))
    (blank)
    (lazy line
      (pattern
        (~ (+ (chars "a-z"))
          (do
            (inline (print (str "line " location)))
          )
          ""
        )
      )
    )
  )
)
```

```output
line ((0, 0), (0, 0))
line ((1, 0), (1, 1))
line ((2, 0), (2, 2))
```

## Direct TokenStream Cursor Rewind

### Rewinding a mark restores the cursor directly, including across newlines.

```brix
(import /core/tokenstream)
(blank)

(main
  (let stream (call (of IndexableTokenStream Char) "ab\nc"))
  (let begin (call (. stream mark)))
  (let buf ([] Char 4))
  (call (. stream read) 4 buf)
  (call (. stream rewind) begin)
  (blank)
  (print "direct-rewind")
  (print (. stream location))
)
```

```output
direct-rewind
(0, 0)
```

### Indexable marks support nesting, repeated rewind, EOF, and cached-location reuse.

```brix
(import /core/tokenstream)
(blank)

(main
  (let stream (call (of IndexableTokenStream Char) "a\nb"))
  (let outer (call (. stream mark)))
  (let first (<- stream))
  (print (. stream cursor))
  (let inner (call (. stream mark)))
  (let second (<- stream))
  (let third (<- stream))
  (let eof (<- stream))
  (print (. stream cursor))
  (blank)
  (call (. stream rewind) inner)
  (let rewound-newline (<- stream))
  (print (as (!! rewound-newline) U32))
  (print (. stream cursor))
  (call (. stream rewind) outer)
  (print (. stream location))
  (let first-again (<- stream))
  (call (. stream rewind) outer)
  (let first-repeated (<- stream))
  (print (. stream cursor))
)
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

```brix
(import /core/tokenstream)
(blank)

(main
  (let attempt
    (do-new
      (let first (call (of IndexableTokenStream Char) "a"))
      (let second (call (of IndexableTokenStream Char) "b"))
      (call (. first rewind) (call (. second mark)))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)
```

```output
Stream mark belongs to another stream
```

### A large direct cursor displacement rewinds to zero and rereads the first token.

```brix
(import /core/tokenstream)
(blank)

(main
  (let stream (call (of IndexableTokenStream Char) ([] Char 65537)))
  (let begin (call (. stream mark)))
  (= (. stream cursor) 65536)
  (call (. stream rewind) begin)
  (print (. stream cursor))
  (print (as (!! (<- stream)) U32))
)
```

```output
0
0
```

### Rewinding an outer mark invalidates nested marks beyond the restored cursor.

```brix
(import /core/tokenstream)
(blank)

(main
  (let attempt
    (do-new
      (let stream (call (of IndexableTokenStream Char) "ab"))
      (let outer (call (. stream mark)))
      (let item (<- stream))
      (let inner (call (. stream mark)))
      (call (. stream rewind) outer)
      (call (. stream rewind) inner)
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)
```

```output
Stream mark is no longer valid
```

### Lazy locations recognize LF, CRLF, and CR without rescanning cached prefixes after rewind.

```brix
(import /core/tokenstream)
(blank)

(main
  (let stream (call (of IndexableTokenStream Char) "a\r\nb\rc\nλ"))
  (let begin (call (. stream mark)))
  (print (. stream location))
  (while (. stream has-more)
    (let item (<- stream))
    (print (. stream location))
  )
  (blank)
  (call (. stream rewind) begin)
  (print (. stream location))
  (print (call (. stream get-error-location)))
)
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

```brix
(import /core/tokenstream)
(blank)

(main
  (let stream (call (of IndexableTokenStream Char) "abcd"))
  (let begin (call (. stream mark)))
  (let bulk ([] Char 3))
  (let actual (call (. stream read) 3 bulk))
  (call (. stream rewind) begin)
  (print (. stream cursor))
  (print (!! (<- stream)))
  (blank)
  (let second (call (. stream mark)))
  (let item (!! (<- stream)))
  (call (. stream rewind) second)
  (print (. stream cursor))
  (print (!! (<- stream)))
)
```

```output
0
a
1
b
```

### Cached lazy locations preserve repeated, monotonic, backward, Unicode, and high-water queries.

```brix
(import /core/tokenstream)
(blank)

(main
  (let stream (call (of ArrayTokenStream Char) (. "a\r\nb\rc\nλ" codepoints)))
  (print (. stream location))
  (print (. stream location))
  (let first (<- stream))
  (print (. stream location))
  (let carriage (<- stream))
  (let line-feed (<- stream))
  (let second-line (<- stream))
  (print (. stream location))
  (let backward (call (. stream mark)))
  (let second-carriage (<- stream))
  (let third-line (<- stream))
  (let third (<- stream))
  (let final-line-feed (<- stream))
  (let unicode (<- stream))
  (print (. stream location))
  (call (. stream rewind) backward)
  (print (. stream location))
  (print (call (. stream get-error-location)))
)
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

```brix
(import /core/tokenstream)
(blank)

(main
  (let stream (call (of ChunkedTokenStream Char) "abcd"))
  (let begin (call (. stream mark)))
  (let bulk ([] Char 3))
  (let actual (call (. stream read) 3 bulk))
  (call (. stream rewind) begin)
  (print (. stream location))
  (print (call (. stream read) 3 bulk))
  (print (call (. bulk slice-string)))
)
```

```output
(0, 0)
3
abc
```

### Chunked byte streams rewind by cursor while preserving their logical position.

```brix
(import /core/tests/support/byte-source)
(import /core/tokenstream)
(blank)

(main
  (let data (call (of TestByteSource Char) (. "abcd" codepoints)))
  (call (. data rewind))
  (let stream (call (of ChunkedTokenStream Char) data))
  (let begin (call (. stream mark)))
  (let bulk ([] Char 3))
  (let actual (call (. stream read) 3 bulk))
  (call (. stream rewind) begin)
  (print (. stream location))
  (print (call (. stream read) 3 bulk))
  (print (call (. bulk slice-string)))
)
```

```output
(0, 0)
3
abc
```

### A prefix release keeps a later mark on its own token and rejects a mark it discarded.

```brix
(import /core/tests/support/byte-source)
(import /core/tokenstream)
(blank)

(main
  (let data (call (of TestByteSource Char) (. "abcd" codepoints)))
  (call (. data rewind))
  (let stream (call (of ChunkedTokenStream Char) data))
  (let lost (call (. stream mark)))
  (<- stream)
  (<- stream)
  (let kept (call (. stream mark)))
  (call (. stream pattern-release))
  (<- stream)
  (<- stream)
  (print (str (!! (call (. stream capture-span) kept))))
  (call (. stream rewind) kept)
  (print (!! (<- stream)))
  (print (. stream location))
  (let attempt
    (do-new
      (call (. stream rewind) lost)
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)
```

```output
cd
c
(0, 3)
Stream mark is no longer valid
```

### A bulk read whose refill releases as much as the source gives still reads its full count.

```brix
(import /core/tests/support/byte-source)
(import /core/tokenstream)
(blank)

(main
  (let data (call (of TestByteSource Byte) (as (* "x" 8192) ([] Byte))))
  (= (. data chunk-limit) 1024)
  (let stream (call (of ChunkedTokenStream Byte) data))
  (print (call (. stream read) 1024 ([] Byte 1024)))
  (print (call (. stream read) 2048 ([] Byte 2048)))
  (print (call (. stream read) 3072 ([] Byte 3072)))
)
```

```output
1024
2048
3072
```

## Pattern Backtracking

### The first alternative consumes "ab\nc" before failing; the second should restart at (0, 0).

```brix
(main
  (match "ab\nc"
    (~ (| (seq "a" "b" \nl "x") (seq "a" "b" \nl "c"))
      (do
        (inline (print (str "alternate " location)))
      )
      ""
    )
  )
)
```

```output
alternate ((0, 0), (1, 0))
```

### Positive lookahead consumes "ab\n" and puts it back before the real match.

```brix
(main
  (match "ab\nc"
    (~ (seq (& (seq "a" "b" \nl)) "a" "b" \nl "c")
      (do
        (inline (print (str "lookahead " location)))
      )
      ""
    )
  )
)
```

```output
lookahead ((0, 0), (1, 0))
```

### A failed optional branch should leave the stream location unchanged for the following match.

```brix
(main
  (match "ab\nc"
    (~ (seq (? (seq "a" "b" \nl "x")) "a" "b" \nl "c")
      (do
        (inline (print (str "optional " location)))
      )
      ""
    )
  )
)
```

```output
optional ((0, 0), (1, 0))
```

### A failed static combinator branch restores its caller mark before the location-bearing arm.

```brix
(import /core/patterns)
(blank)

(main
  (match "ab;\ncd"
    (~ (| (seq (call \delimited "," (~ (+ (chars "a-z")))) "!") (seq "ab" ";" \nl "cd"))
      (do
        (inline (print (str "combinator " location)))
      )
      ""
    )
  )
)
```

```output
combinator ((0, 0), (1, 1))
```

## Error Location

### A body with no pattern uses TokenStream.error-location; it should point at the next unread token.

```brix
(main
  (do-new
    (match "ok\nbad"
      (~ (seq "ok" \nl))
      (throw (call Panic (str "extra at " location)))
    )
    (on-failure failure
      (print failure)
    )
  )
)
```

```output
extra at (1, 0)
```

### A choice that skips every alternative, since none can start with the next item, fails where it began.

```brix
(main
  (do-new
    (match "'+"
      (~ (seq "'" (| \upper \bracketed) (! .)))
      (throw (call Panic (str "failed at " location)))
      (blank)
      (lazy upper
        (pattern
          (~ (chars "A-Z"))
        )
      )
      (blank)
      (lazy bracketed
        (pattern
          (~ (seq "[" (chars "a-z") "]"))
        )
      )
    )
    (on-failure failure
      (print failure)
    )
  )
)
```

```output
failed at (0, 1)
```

## ChunkedTokenStream Byte Inputs

### Numeric U8 patterns exercise ChunkedTokenStream reads one token at a time.

```brix
(import /core/tests/support/byte-source)
(import /core/tokenstream)
(blank)

(main
  (let data (call TestByteSource (call (of Array U8) (list 97u8 98u8 10u8 99u8))))
  (call (. data rewind))
  (blank)
  (let stream (call (of ChunkedTokenStream U8) data))
  (match stream
    (~ ({} . 4)
      (do
        (inline (print (str "buffer-uint8 " location)))
      )
      ""
    )
  )
)
```

```output
buffer-uint8 ((0, 0), (1, 0))
```

### String patterns exercise ChunkedTokenStream.read(size) bulk reads for Char streams.

```brix
(import /core/tests/support/byte-source)
(import /core/tokenstream)
(blank)

(main
  (let data (call (of TestByteSource Char) (. "ab\nc" codepoints)))
  (let stream (call (of ChunkedTokenStream Char) data))
  (match stream
    (~ "ab\nc"
      (do
        (inline (print (str "buffer-char " location)))
      )
      ""
    )
  )
)
```

```output
buffer-char ((0, 0), (1, 0))
```

```brix
(import /core/tokenstream)
(blank)

(main
  (let stream (call (of ArrayTokenStream Char) (. "!é\n東京😀?" codepoints)))
  (let literal "é\n東京😀")
  (print
    (match stream
      (~ (seq "!" (: span (call \runtime-location literal)) "?")
        span
      )
    )
  )
  (print (. stream cursor))
  (print (. stream cursor-extent))
)

(fn-expr + runtime-location _ (over: Char)
  (param literal String)
  (~ $literal
    location
  )
)
```

```output
((0, 1), (1, 2))
7
7
```

```brix
(import /BxGC/BxGC.h)
(blank)

(main
  (match "xfoo\nxxfooo\nfo\n"
    (~ (+ (seq (* "x") \foo \nl)))
    (blank)
    (lazy foo
      (pattern
        (~ (seq "f" (+ "o"))
          (do
            (inline (print (call after-collection location)))
          )
          ""
        )
      )
    )
  )
)

(fn-do + after-collection MatchLocation
  (param location MatchLocation)
  (call C:brixCollect)
  (return location)
)
```

```output
((0, 1), (0, 3))
((1, 2), (1, 5))
((2, 0), (2, 1))
```

```brix
(import /core/parsing)
(blank)

(main
  (let result
    (match ""
      (~ \empty)
    )
  )
  (throw (call Panic "empty match failed") (if (not result)))
  (blank)
  (print (str (. result begin line) ":" (. result begin column) "-" (. result end line) ":" (. result end column)))
)

(fn-expr + empty _ (over: Char)
  (~ ""
    location
  )
)
```

```output
0:0-0:0
```

### A function that the expression of a rule's arm makes reads the arm's `location`.

```brix
(import /stream/aggregate)
(blank)

(main
  (for (in text (list "a,b,c" "a"))
    (print
      (match text
        (~ (: found \words)
          found
        )
        (default
          (inline "none")
        )
      )
    )
  )
)

(fn-expr + words String (over: Char)
  (~ (seq (: head \word) (: tail (* (seq "," \word))))
    (call reduce (:seed head) (:stream tail)
      (:using
        (=> (left right)
          (block
            (str "(" left " " right " " (. location end column) ")")
          )
        )
      )
    )
  )
  (where
    (lazy word String
      (pattern
        (~ (: w (+ (chars "a-z")))
          (str "<" w ">")
        )
      )
    )
  )
)
```

```output
((<a> <b> 4) <c> 4)
<a>
```
