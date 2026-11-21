# Arrays

```brix
(main
  (let buffer ([] Float 1))
  (print ([] buffer 0))
  (= ([] buffer 0) 1.5)
  (print ([] buffer 0))
)
```

```output
0
1.5
```

```brix
(main
  (let buffer ([] Int 1))
  (print ([] buffer 0))
  (= ([] buffer 0) 1)
  (print ([] buffer 0))
  (= ([] buffer 0) (as 2.5 Int))
  (print ([] buffer 0))
)
```

```output
0
1
2
```

```brix
(main
  (do-new
    (let buffer ([] Float 1))
    (print ([] buffer 1))
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
  (do-new
    (let buffer ([] Float 1))
    (print (= ([] buffer 1) 1))
    (on-failure failure
      (print failure)
    )
  )
)
```

```output
Set out of bounds (1 / 1)
```

```brix
(main
  (let buffer (call (of Array String) 2))
  (= ([] buffer 0) "a")
  (= ([] buffer 1) "b")
  (blank)
  (call try-read buffer -1)
  (call try-write buffer -5)
  (call try-slice buffer -1)
  (print (call (. ", " join) buffer))
)

(fn-do _ try-read _
  (param buffer (of Array String))
  (param index Index)
  (let attempt
    (do-new
      (print ([] buffer index))
      (on-failure failure
        (print (str "read: " failure))
      )
    )
  )
  (call (. attempt wait))
)

(fn-do _ try-write _
  (param buffer (of Array String))
  (param index Index)
  (let attempt
    (do-new
      (= ([] buffer index) "z")
      (on-failure failure
        (print (str "write: " failure))
      )
    )
  )
  (call (. attempt wait))
)

(fn-do _ try-slice _
  (param buffer (of Array String))
  (param start Index)
  (let attempt
    (do-new
      (print (call (. ", " join) ([] buffer (..< start 1))))
      (on-failure failure
        (print (str "slice: " failure))
      )
    )
  )
  (call (. attempt wait))
)
```

```output
read: Index out of bounds (-1 / 2)
write: Set out of bounds (-5 / 2)
slice: Range out of bounds (0 / 2)
a, b
```

## Ranges

```brix
(main
  (let buffer (call (of Array Float) 5))
  (= ([] buffer (.. 2 4)) 3.5)
  (print (call (. ", " join) buffer))
  (blank)
  (print (call (. ", " join) ([] buffer (.. 2 4))))
)
```

```output
0, 0, 3.5, 3.5, 3.5
3.5, 3.5, 3.5
```

```brix
(main
  (do-new
    (let buffer (call (of Array Float) 5))
    (print ([] buffer (.. 0 (. Position end))))
    (on-failure failure
      (print failure)
    )
  )
)
```

```output
Range out of bounds (5 / 5)
```

```brix
(main
  (do-new
    (let buffer (call (of Array Float) 5))
    (= ([] buffer (.. 0 (. Position end))) 1)
    (on-failure failure
      (print failure)
    )
  )
)
```

```output
Set out of bounds (5 / 5)
```

### Range assignment checks source length before copying.

```brix
(main
  (let buffer ([] Byte 5))
  (= ([] buffer 0) 10u8)
  (= ([] buffer 1) 11u8)
  (let source ([] Byte 1))
  (= ([] source 0) 99u8)
  (blank)
  (call copy-too-much buffer source)
  (print (call (. ", " join) buffer))
)

(fn-do + copy-too-much _
  (param buffer ([] Byte))
  (param source ([] Byte))
  (let attempt
    (do-new
      (= ([] buffer (..< 0 5)) source)
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)
```

```output
Source array too short (1 / 5)
10, 11, 0, 0, 0
```

```brix
(main
  (let values (call (of Array Int) (list 1 2 3 2)))
  (print (== values (call (of Array Int) (list 1 2 3 2))))
  (print (== values (call (of Array Int) (list 1 2 3))))
  (print (== values (list 1 2 3 2)))
)
```

```output
true
false
true
```

```brix
(main
  (let values (call (of Array Int) (list 1 2 3 2)))
  (print (has values 2))
  (print (has-not values 9))
  (print (has values 3))
)
```

```output
true
true
true
```

```brix
(main
  (let values (call (of Array Int) (list 1 2 3 2)))
  (print (call (. values find) 2))
  (print (== (call (. values find) 9) null))
)
```

```output
1
true
```

```brix
(main
  (let values (call (of Array Int) (list 1 2 3 2)))
  (print (call (. values find) (call (of Array Int) (list 3 2))))
  (print (call (. values find) (list 2 3)))
)
```

```output
2
1
```

```brix
(main
  (let values (call (of Array Int) (list 1 2 3 2)))
  (print (call (. values starts-with) (list 1 2)))
  (print (call (. values starts-with) (list 2 3)))
)
```

```output
true
false
```

```brix
(main
  (let values (call (of Array Int) (list 1 2 3 2)))
  (print (call (. values ends-with) (list 3 2)))
  (print (call (. values ends-with) (list 1 2)))
)
```

```output
true
false
```

```brix
(main
  (let bytes (call (of Array Byte) (list 0u8 1u8 2u8 253u8 254u8 255u8)))
  (print (as bytes Hex))
)
```

```output
000102fdfeff
```

```brix
(main
  (print (call (. ", " join) (as (call Hex "000102fdfeff") ([] Byte))))
  (print (call (. ", " join) (as (call Hex "0x000102FDFEFF") ([] Byte))))
)
```

```output
0, 1, 2, 253, 254, 255
0, 1, 2, 253, 254, 255
```

```brix
(main
  (print (as (call (of Array Byte) "f") Base64))
  (print (as (call (of Array Byte) "fo") Base64))
  (print (as (call (of Array Byte) "foo") Base64))
)
```

```output
Zg==
Zm8=
Zm9v
```

```brix
(main
  (let decoded (as (call Base64 "Zm9v") ([] Byte)))
  (print (call (. decoded slice-string)))
)
```

```output
foo
```

```brix
(main
  (let decoded (as (call Base64 "") ([] Byte)))
  (print (. decoded length))
)
```

```output
0
```

### Array storage copies Arena handles and allocation pointers without lifetime effects.

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
  (let arenas (call (of Array Arena) (list first)))
  (= ([] arenas 0) second)
  (let first-next (call (of (. first allocate) U32) 1))
  (= ([] first-next 0) 11u32)
  (print ([] first-next 0))
  (= arenas (call (of Array Arena)))
  (let second-next (call (of (. second allocate) U32) 1))
  (= ([] second-next 0) 21u32)
  (print ([] second-next 0))
  (blank)
  (let pointers (call (of Array (of CPointer U32)) (list first-pointer)))
  (= pointers (call (. pointers replace) (..< 0 1) (call (of Array (of CPointer U32)) (list second-pointer))))
  (print ([] first-pointer 0))
  (print ([] ([] pointers 0) 0))
  (blank)
  (call (. first release))
  (call (. second release))
)
```

```output
11
21
10
20
```

```brix
(import /core/arena)
(blank)

(main
  (print (tuple (. (of Array U32) size) (. (of Array U32 ArenaBacking) size)))
)
```

```output
(16, 16)
```

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (blank)
  (let values (call (of Array U32) arena 4))
  (<- values (list 10u32 20u32 30u32 40u32))
  (let slice ([] values (..< 1 3)))
  (= ([] slice 0) 21u32)
  (print (type-of values))
  (print (type-of slice))
  (print (tuple ([] values 0) ([] values 1) ([] values 2) ([] values 3)))
)
```

```output
[TypeInfo Array'U32'ArenaBacking]
[TypeInfo Array'U32'ArenaBacking]
(10, 21, 30, 40)
```

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (blank)
  (let values (call (of Array U32) arena (list 10u32 21u32)))
  (let grown (call (. values resize) 3 arena))
  (= ([] grown 2) 50u32)
  (print (tuple ([] grown 0) ([] grown 1) ([] grown 2)))
)
```

```output
(10, 21, 50)
```

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (blank)
  (let values (call (of Array U32) arena (list 10u32 21u32 50u32)))
  (let copy (call (. values copy) arena))
  (= ([] copy 0) 99u32)
  (print (tuple ([] values 0) ([] copy 0) ([] copy 1) ([] copy 2)))
)
```

```output
(10, 99, 21, 50)
```

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (blank)
  (let source (call (of Array U32) arena (list 99u32 21u32 30u32 50u32)))
  (let destination (call (of Array U32) arena 3))
  (<- destination ([] source (..< 0 3)))
  (print (tuple ([] destination 0) ([] destination 1) ([] destination 2)))
)
```

```output
(99, 21, 30)
```

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (let values (call (of Array U32) arena (list 5u32 6u32)))
  (print (tuple ([] values 0) ([] values 1)))
)
```

```output
(5, 6)
```

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (let values (call (of Array U32) arena 3 (:with (=> (index) (+ (as index U32) 60u32)))))
  (print (tuple ([] values 0) ([] values 1) ([] values 2)))
)
```

```output
(60, 61, 62)
```

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (let values (call (of Array U32) arena (list 60u32 61u32 62u32)))
  (let mapped (call (. values map) arena (:using (=> (item index) (+ item (as index U32))))))
  (print (tuple ([] mapped 0) ([] mapped 1) ([] mapped 2)))
)
```

```output
(60, 62, 64)
```

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (let values (call (of Array U32) arena (list 60u32 62u32 64u32)))
  (let replaced (call (. values replace) (..< 1 2) arena 77u32))
  (print (tuple ([] replaced 0) ([] replaced 1) ([] replaced 2)))
)
```

```output
(60, 77, 64)
```

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (let octets (call (of Array U8) arena (list 8u8)))
  (let words (call (of Array U16) arena (list 16u16)))
  (print (tuple ([] octets 0) ([] words 0)))
)
```

```output
(8, 16)
```

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (let values (call (of Array U32) arena 5))
  (let bytes (as values (of Array Byte ArenaBacking)))
  (print (. bytes size))
)
```

```output
20
```

```brix
(main
  (let octets (call (of Array U8) (list 1u8)))
  (let words (call (of Array U16) (list 2u16)))
  (print (tuple ([] octets 0) ([] words 0)))
)
```

```output
(1, 2)
```

```brix
(import /BxGC/BxGC.h)
(blank)

(main
  (let values (call (of Array BackingNode) 3))
  (for (in index (..< 0 (. values length)))
    (= ([] values index) (call BackingNode (str "node-" index)))
  )
  (= ([] values 1) (call BackingNode "replacement"))
  (= values (call (. values resize) 5))
  (= ([] values 4) (call BackingNode "grown"))
  (call C:brixGCCollect)
  (print (tuple (. ([] values 0) value) (. ([] values 1) value) (. ([] values 4) value)))
)

(type + BackingNode
  (prop + value String)
  (fn-do + This _
    (param text String)
    (= value text)
  )
)
```

```output
(node-0, replacement, grown)
```

```brix
(import /BxGC/BxGC.h)
(blank)

(main
  (let values (call (of Array BackingNode) (list (call BackingNode "node-0") (call BackingNode "replacement"))))
  (= values (call (. values resize) 5))
  (= ([] values 4) (call BackingNode "grown"))
  (let copy (call (. values copy)))
  (= ([] values 0) (call BackingNode "original"))
  (call C:brixGCCollect)
  (print (tuple (. ([] values 0) value) (. ([] copy 0) value) (. ([] copy 1) value) (. ([] copy 4) value)))
)

(type + BackingNode
  (prop + value String)
  (fn-do + This _
    (param text String)
    (= value text)
  )
)
```

```output
(original, node-0, replacement, grown)
```

```brix
(import /core/arena)
(blank)

(main
  (print (call arena-copy-result))
)

(fn-do + arena-copy-result U32 (without gc)
  (let local-arena (call Arena))
  (finally
    (call (. local-arena release))
  )
  (let values (call (of Array U32) local-arena 2))
  (= ([] values 0) 1u32)
  (= ([] values 1) 2u32)
  (let result (call (. values copy) local-arena))
  (return ([] result 1))
)
```

```output
2
```

```brix
(import /core/arena)
(blank)

(main
  (print (call arena-resize-result))
)

(fn-do + arena-resize-result U32 (without gc)
  (let local-arena (call Arena))
  (finally
    (call (. local-arena release))
  )
  (let values (call (of Array U32) local-arena 2))
  (= ([] values 0) 1u32)
  (= ([] values 1) 2u32)
  (let result (call (. values resize) 3 local-arena))
  (= ([] result 2) 3u32)
  (return ([] result 2))
)
```

```output
3
```

### Byte encoding converters report malformed input.

```brix
(main
  (do-new
    (let bytes (as (call Hex "abc") ([] Byte)))
    (on-failure failure
      (print failure)
    )
  )
)
```

```output
Hex byte arrays require an even digit count
```

```brix
(main
  (do-new
    (let bytes (as (call Base64 "abc") ([] Byte)))
    (on-failure failure
      (print failure)
    )
  )
)
```

```output
Invalid Base64 length
```

```brix
(main
  (let values (call (of Array Int) (list 1 2 3 4 5)))
  (= ([] values (..< 1 4)) (list 8 9 10))
  (print (call (. ", " join) values))
)
```

```output
1, 8, 9, 10, 5
```

```brix
(main
  (let values (call (of Array Int) (list 1 2 3)))
  (call (. values fill) 7)
  (print (call (. ", " join) values))
)
```

```output
7, 7, 7
```

```brix
(main
  (let values (call (of Array Int) (list 7 7 7 7 7)))
  (= values (call (. values replace) (..< 1 4) (call (of Array Int) (list 2 3))))
  (print (call (. ", " join) values))
)
```

```output
7, 2, 3, 7
```

```brix
(main
  (let values (call (of Array Int) (list 7 2 3 7)))
  (= values (call (. values replace) (..< 2 2) (list 4 5)))
  (print (call (. ", " join) values))
)
```

```output
7, 2, 4, 5, 3, 7
```

```brix
(main
  (let values (call (of Array Int) (list 7 2 4 5 3 7)))
  (= values (call (. values replace) (..< 0 2) 9))
  (print (call (. ", " join) values))
)
```

```output
9, 4, 5, 3, 7
```

```brix
(main
  (let values (call (of Array Int) (list 9 4 5 3 7)))
  (-= values 5)
  (print (call (. ", " join) values))
)
```

```output
9, 4, 3, 7
```

```brix
(main
  (let values (call (of Array Int) (list 9 4 3 7)))
  (-= ([] values 1))
  (print (call (. ", " join) values))
)
```

```output
9, 3, 7
```

```brix
(main
  (let values (call (of Array Int) (list 9 3 7)))
  (-= ([] values (..< 1 2)))
  (print (call (. ", " join) values))
)
```

```output
9, 7
```

```brix
(main
  (let values (call (of Array Int) (list 9 7)))
  (-= values 42)
  (print (call (. ", " join) values))
)
```

```output
9, 7
```

### Byte slice-string checks the range before reading.

```brix
(main
  (do-new
    (let buffer (call (of Array Byte) "a"))
    (print (call (. buffer slice-string) (..< 0 5)))
    (on-failure failure
      (print failure)
    )
  )
)
```

```output
Range out of bounds (4 / 1)
```

### Char slice-string checks the range before reading.

```brix
(main
  (do-new
    (let buffer (call (of Array Char) "a"))
    (print (call (. buffer slice-string) (..< 0 5)))
    (on-failure failure
      (print failure)
    )
  )
)
```

```output
Range out of bounds (4 / 1)
```

### Oversized buffer allocation panics instead of returning null data with a huge size.

```brix
(main
  (do-new
    (let buffer (call (of Array I64) (+ (/ (. Size max) (. I64 size)) 1)))
    (print (. buffer size))
    (print (. buffer data address))
    (on-failure failure
      (print failure)
    )
  )
)
```

```output
Array size overflow
```

### Oversized buffer resize panics before byte-size arithmetic overflows.

```brix
(main
  (do-new
    (let buffer ([] I64 1))
    (= buffer (call (. buffer resize) (+ (/ (. Size max) (. I64 size)) 1)))
    (print (. buffer size))
    (on-failure failure
      (print failure)
    )
  )
)
```

```output
Array size overflow
```

### Oversized buffer growth panics before size arithmetic overflows.

```brix
(main
  (do-new
    (let buffer ([] Byte 1))
    (= buffer (call (. buffer grow) (. Size max)))
    (print (. buffer size))
    (on-failure failure
      (print failure)
    )
  )
)
```

```output
Array size overflow
```

### Byte buffers constructed from strings use UTF-8 byte length.

```brix
(main
  (let buffer (call (of Array Byte) "é"))
  (print (. buffer size))
  (print (call (. buffer slice-string)))
)
```

```output
2
é
```

### A byte view of a text reads its UTF-8 bytes.

```brix
(main
  (let part "h")
  (let view (call byte-view (str part "é!")))
  (print (. view size))
  (print (call (. ", " join) view))
  (print (call (. ([] view (..< 1 3)) slice-string)))
)
```

```output
4
104, 195, 169, 33
é
```

### A byte view of a text cannot be written.

```brix compile-error
(main
  (let view (call byte-view "ab"))
  (= ([] view 0) 99)
)
```

```output
Exception: ImmutableArray does not support element assignment
<no file:4>
      (= ([] view 0) 99)
         ^^^^^^^^^^^
```

### A slice of a byte view of a text cannot be written.

```brix compile-error
(main
  (let view (call byte-view "ab"))
  (= ([] ([] view (... 1)) 0) 99)
)
```

```output
Exception: ImmutableArray does not support element assignment
<no file:4>
      (= ([] ([] view (... 1)) 0) 99)
         ^^^^^^^^^^^^^^^^^^^^^^^^
```

### Writing a byte copy of a text leaves the text and its byte view as they were.

```brix
(main
  (let part "k")
  (let text (str part "ey"))
  (let copy (call (of Array Byte) text))
  (= ([] copy 0) 109)
  (print (call (. copy slice-string)))
  (print text)
  (print ([] (call byte-view text) 0))
)
```

```output
mey
key
107
```

### Byte-buffer string range assignment writes UTF-8 bytes without recursion.

```brix
(main
  (let buffer ([] Byte 2))
  (= ([] buffer (..< 0 2)) "é")
  (print (. buffer size))
  (print (call (. buffer slice-string)))
)
```

```output
2
é
```

### Byte-buffer string range assignment rejects length mismatches.

```brix
(main
  (do-new
    (let buffer ([] Byte 1))
    (= ([] buffer (..< 0 1)) "é")
    (on-failure failure
      (print failure)
    )
  )
)
```

```output
String range length mismatch (2 / 1)
```

### Byte slice-string rejects invalid UTF-8.

```brix
(main
  (do-new
    (let buffer ([] Byte 1))
    (= ([] buffer 0) 255u8)
    (print (call (. buffer slice-string)))
    (on-failure failure
      (print failure)
    )
  )
)
```

```output
Unable to read UTF8 content
```

## Relative positions

```brix
(main
  (let buffer ([] Float 3))
  (= ([] buffer (. Position first)) 1.5)
  (= ([] buffer (. Position last)) 2.5)
  (print ([] buffer (. Position first)))
  (print ([] buffer (. Position last)))
  (print (call (. ", " join) buffer))
)
```

```output
1.5
2.5
1.5, 0, 2.5
```

```brix
(main
  (let buffer (call (of Array Float) 3))
  (= ([] buffer (. Position middle)) 1.5)
  (print ([] buffer (. Position middle)))
  (print (call (. ", " join) buffer))
  (blank)
  (= buffer (call (of Array Float) 4))
  (= ([] buffer (. Position middle)) 1.5)
  (print ([] buffer (. Position middle)))
  (print (call (. ", " join) buffer))
)
```

```output
1.5
0, 1.5, 0
1.5
0, 0, 1.5, 0
```

```brix
(main
  (let buffer (call (of Array Float) 3))
  (= ([] buffer 0) 0.5)
  (= ([] buffer 1) 1.5)
  (= ([] buffer 2) 2.5)
  (blank)
  (print (call (. ", " join) ([] buffer (..< (. Position first) 2))))
)
```

```output
0.5, 1.5
```

```brix
(main
  (let buffer (call (of Array Float) 5))
  (= ([] buffer 0) 0.5)
  (= ([] buffer 1) 1.5)
  (= ([] buffer 2) 2.5)
  (= ([] buffer 3) 3.5)
  (= ([] buffer 4) 4.5)
  (blank)
  (print (call (. ", " join) ([] buffer (..< 0 5))))
  (print (call (. ", " join) ([] buffer (..< (. Position first) (. Position end)))))
  (print (call (. ", " join) ([] buffer (.. (. Position first) (. Position last)))))
  (blank)
  (print (call (. ", " join) ([] buffer (..< (. Position last) (. Position first)))))
  (print (call (. ", " join) ([] buffer (.. (. Position last) (. Position first)))))
  (blank)
  (print (call (. ", " join) ([] buffer (..< 0 2))))
)
```

```output
0.5, 1.5, 2.5, 3.5, 4.5
0.5, 1.5, 2.5, 3.5, 4.5
0.5, 1.5, 2.5, 3.5, 4.5
1.5, 2.5, 3.5, 4.5
0.5, 1.5, 2.5, 3.5, 4.5
0.5, 1.5
```

### Storing objects in buffers

```brix
(main
  (let buffer ([] Thing 10))
  (= ([] buffer 0) (call Thing 1))
  (= ([] buffer 1) (call Thing 2))
  (print (. ([] buffer 0) a))
  (print (. ([] buffer 1) a))
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
1
2
```

### Indexed Array struct field reads preserve scalar, aggregate, and managed-field values.

```brix
(main
  (let rows (call (of Array Row) (list (call Row 1u8 2u16 3u32 4u64 (call Point 5 6) "seven"))))
  (blank)
  (print (. ([] rows 0) tiny))
  (print (. ([] rows 0) small))
  (print (. ([] rows 0) word))
  (print (. ([] rows 0) wide))
  (print (. ([] rows 0) position x))
  (print (. ([] rows 0) text))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)

(type + Row (is Struct)
  (prop + tiny U8)
  (prop + small U16)
  (prop + word U32)
  (prop + wide U64)
  (prop + position Point)
  (prop + text String)
)
```

```output
1
2
3
4
5
seven
```

### The indexed Array scalar-field fast path evaluates its receiver and index once.

```brix
(main
  (let probe (call Probe (call (of Array Row) (list (call Row 42)))))
  (blank)
  (print (. ([] (call (. probe rows)) (call (. probe next))) value))
  (print (. probe rows-calls))
  (print (. probe next-calls))
)

(type + Row (is Struct)
  (prop + value I32)
)

(type + Probe
  (prop + values (of Array Row))
  (prop + rows-calls _ (= 0))
  (prop + next-calls _ (= 0))
  (fn-do + This _
    (param values (of Array Row))
  )
  (fn-do + rows (of Array Row)
    (+= rows-calls 1)
    (return values)
  )
  (fn-do + next Index
    (+= next-calls 1)
    (return 0)
  )
)
```

```output
42
1
1
```

### An out-of-bounds indexed Array scalar-field read preserves the accessor panic.

```brix
(main
  (do-new
    (let rows (call (of Array Row) (list (call Row 42))))
    (print (. ([] rows 1) value))
    (on-failure failure
      (print failure)
    )
  )
)

(type + Row (is Struct)
  (prop + value I32)
)
```

```output
Index out of bounds (1 / 1)
```

### Canonical scalar Array reads and writes preserve every eligible numeric representation.

```brix
(main
  (let bools (call (of Array Bool) (list false true)))
  (let i8s (call (of Array I8) (list -128i8 127i8)))
  (let u8s (call (of Array U8) (list 0u8 255u8)))
  (let i16s (call (of Array I16) (list -32768i16 32767i16)))
  (let u16s (call (of Array U16) (list 0u16 65535u16)))
  (let i32s (call (of Array I32) (list -2147483648i32 2147483647i32)))
  (let u32s (call (of Array U32) (list 0u32 4294967295u32)))
  (let i64s (call (of Array I64) (list -9223372036854775808i64 9223372036854775807i64)))
  (let u64s (call (of Array U64) (list 0u64 18446744073709551615u64)))
  (let f32s (call (of Array F32) (list 1.25f32 2.5f32)))
  (let f64s (call (of Array F64) (list 3.25 4.5)))
  (blank)
  (print ([] bools 1))
  (print ([] i8s 0))
  (print ([] u8s 1))
  (print ([] i16s 0))
  (print ([] u16s 1))
  (print ([] i32s 0))
  (print ([] u32s 1))
  (print ([] i64s 0))
  (print ([] u64s 1))
  (print ([] f32s 0))
  (print ([] f64s 1))
  (blank)
  (= ([] bools 0) true)
  (= ([] i8s 0) 126i8)
  (= ([] u8s 0) 254u8)
  (= ([] i16s 0) 32766i16)
  (= ([] u16s 0) 65534u16)
  (= ([] i32s 0) 2147483646i32)
  (= ([] u32s 0) 4294967294u32)
  (= ([] i64s 0) 9223372036854775806i64)
  (= ([] u64s 0) 18446744073709551614u64)
  (= ([] f32s 0) 6.25f32)
  (= ([] f64s 0) 7.5)
  (blank)
  (print ([] bools 0))
  (print ([] i8s 0))
  (print ([] u8s 0))
  (print ([] i16s 0))
  (print ([] u16s 0))
  (print ([] i32s 0))
  (print ([] u32s 0))
  (print ([] i64s 0))
  (print ([] u64s 0))
  (print ([] f32s 0))
  (print ([] f64s 0))
)
```

```output
true
-128
255
-32768
65535
-2147483648
4294967295
-9223372036854775808
18446744073709551615
1.25
4.5
true
126
254
32766
65534
2147483646
4294967294
9223372036854775806
18446744073709551614
6.25
7.5
```

### Canonical scalar Array access evaluates receiver, index, and assigned value once in source order.

```brix
(main
  (let probe (call ScalarArrayProbe))
  (= ([] (call (. probe array)) (call (. probe index))) (call (. probe value)))
  (print (. probe order))
  (print ([] (. probe values) 0))
  (blank)
  (= (. probe order) "")
  (print ([] (call (. probe array)) (call (. probe index))))
  (print (. probe order))
)

(type + ScalarArrayProbe
  (prop + values _ (= (call (of Array U8) (list 1u8))))
  (prop + order _ (= ""))
  (fn-do + array (of Array U8)
    (+= order "receiver>")
    (return values)
  )
  (fn-do + index Index
    (+= order "index>")
    (return 0)
  )
  (fn-do + value U8
    (+= order "value>")
    (return 9u8)
  )
)
```

```output
value>receiver>index>
9
9
receiver>index>
```

### Negative canonical scalar Array indices preserve the accessor panic.

```brix
(main
  (do-new
    (let values (call (of Array U8) (list 1u8)))
    (let index -1i8)
    (print ([] values index))
    (on-failure failure
      (print failure)
    )
  )
)
```

```output
Index out of bounds (-1 / 1)
```

```brix
(main
  (let optional (? (of Array U8)) (call (of Array U8) (list 3u8)))
  (= (?[] optional 0) 4u8)
  (print (?[] optional 0))
)
```

```output
4
```

```brix
(main
  (let custom (call ScalarArrayAccessor (call (of Array U8) (list 5u8))))
  (print ([] custom 0))
  (= ([] custom 0) 6u8)
  (print ([] custom 0))
  (print (. custom reads))
  (print (. custom writes))
)

(type + ScalarArrayAccessor
  (prop + values (of Array U8))
  (prop + reads _ (= 0))
  (prop + writes _ (= 0))
  (fn-do + This _
    (param values _)
  )
  (fn-do + (index-get this (param index Index)) U8
    (+= reads 1)
    (return ([] (. values data) index))
  )
  (fn-do + (index-set this (param index Index) (param value U8)) _
    (+= writes 1)
    (= ([] (. values data) index) value)
  )
)
```

```output
5
6
2
1
```

### Array index extensions retain their accessor dispatch.

```brix crash
(main
  (let values (call (of Array U8) (list 5u8)))
  (print ([] values 0))
)

(extend + (of Array U8)
  (fn-expr + (index-get this (param index Index)) U8
    (+ ([] (. this data) index) 10u8)
  )
)
```

```output
Exception: Extension member conflicts with existing member
<no file:4>
      (print ([] values 0))
             ^^^^^^^^^^^^^
```

### Resizing buffers

```brix
(main
  (let buffer ([] Float 2))
  (= ([] buffer 0) 1.5)
  (print (call (. ", " join) buffer))
  (blank)
  (= buffer (call (. buffer resize) 3))
  (print (call (. ", " join) buffer))
  (blank)
  (= ([] buffer 2) 3.5)
  (print (call (. ", " join) buffer))
)
```

```output
1.5, 0
1.5, 0, 0
1.5, 0, 3.5
```

### Pointer math on buffers

```brix
(main
  (let buffer ([] Float 2))
  (= ([] buffer 0) 1.5)
  (= ([] buffer 1) 2.5)
  (= buffer (+ buffer 1))
  (print ([] buffer 0))
)
```

```output
2.5
```

```brix
(main
  (let buffer ([] Float 2))
  (= ([] buffer 0) 1.5)
  (= ([] buffer 1) 2.5)
  (+= buffer 1)
  (print ([] buffer 0))
)
```

```output
2.5
```

```brix
(main
  (let buffer ([] Float 2))
  (+= buffer 1)
  (print (. buffer size))
)
```

```output
1
```

```brix
(main
  (do-new
    (let buffer ([] Float 2))
    (+= buffer 3)
    (on-failure failure
      (print failure)
    )
  )
)
```

```output
Out of bounds 3 / 2
```

```brix
(main
  (do-new
    (let buffer ([] Float 2))
    (+= buffer 2)
    (print ([] buffer 0))
    (on-failure failure
      (print failure)
    )
  )
)
```

```output
Index out of bounds (0 / 0)
```

## Type conversions

### Converting a non-byte buffer to bytes

```brix
(main
  (let buffer ([] I32 2))
  (= ([] buffer 0) 1)
  (= ([] buffer 1) 2)
  (blank)
  (let bytes (as buffer (of Array Byte)))
  (print (. bytes size))
  (print (. bytes byte-size))
)
```

```output
8
8
```

## ChunkedTokenStream

```brix
(import /core/tests/support/byte-source)
(import /core/tokenstream)
(blank)

(main
  (let buffer ([] Byte 9))
  (= ([] buffer 0) 64u8)
  (blank)
  (= ([] buffer 1) 0u8)
  (= ([] buffer 2) 0u8)
  (= ([] buffer 3) 3u8)
  (= ([] buffer 4) 32u8)
  (blank)
  (= ([] buffer 5) 1u8)
  (= ([] buffer 6) 2u8)
  (= ([] buffer 7) 3u8)
  (= ([] buffer 8) 32u8)
  (blank)
  (let buffer-stream (call TestByteSource buffer))
  (let stream (call (of ChunkedTokenStream Byte) buffer-stream))
  (blank)
  (print (call (of (. stream require) Byte)))
  (print (call (of (. stream require) Byte)))
  (print (call (of (. stream require) Byte)))
  (print (as (call (of (. stream require) U16)) Binary))
  (print (call (of (. stream require) U32)))
)
```

```output
64
0
0
00000011 00100000
16909088
```

```brix
(import /core/tests/support/byte-source)
(import /core/tokenstream)
(blank)

(main
  (let buffer ([] Byte 4))
  (= ([] buffer 0) 1u8)
  (= ([] buffer 1) 2u8)
  (= ([] buffer 2) 3u8)
  (= ([] buffer 3) 32u8)
  (blank)
  (let buffer-stream (call TestByteSource buffer))
  (let stream1 (call (of ChunkedTokenStream Byte) buffer-stream))
  (print (as (call (of (. stream1 require) U32)) Binary))
  (blank)
  (call (. buffer-stream rewind))
  (let stream2 (call (of ChunkedTokenStream Byte) buffer-stream))
  (print (as (call (of (. stream2 require) U32) (. Endian big)) Binary))
  (blank)
  (call (. buffer-stream rewind))
  (let stream3 (call (of ChunkedTokenStream Byte) buffer-stream))
  (print (as (call (of (. stream3 require) U32) (. Endian little)) Binary))
)
```

```output
00000001 00000010 00000011 00100000
00000001 00000010 00000011 00100000
00100000 00000011 00000010 00000001
```

```brix
(import /core/tests/support/byte-source)
(import /core/tokenstream)
(blank)

(main
  (let data (call (of TestByteSource Char) (. "abc" codepoints)))
  (let stream (call (of ChunkedTokenStream Char) data))
  (blank)
  (let buf ([] Char 3))
  (let count (call (. stream read) 3 buf (:should-read-once true)))
  (blank)
  (print count)
  (print (call (. "," join) ([] buf (..< 0 count))))
)
```

```output
3
a,b,c
```

```brix
(import /core/tests/support/byte-source)
(import /core/tokenstream)
(blank)

(main
  (let complete (call TestByteSource (call boundary-bytes 3)))
  (= ([] (. complete data) 4095) 226u8)
  (= ([] (. complete data) 4096) 148u8)
  (= ([] (. complete data) 4097) 129u8)
  (= (. complete is-encoded) true)
  (= (. complete chunk-limit) 4096)
  (let text (call (. (call (. (call (of ChunkedTokenStream Char) complete) read)) slice-string)))
  (print (. text length))
  (print (. text size))
  (print ([] text 4094))
  (print ([] text 4095))
  (blank)
  (let attempt
    (do-new
      (let partial (call TestByteSource (call boundary-bytes 2)))
      (= ([] (. partial data) 4095) 226u8)
      (= ([] (. partial data) 4096) 148u8)
      (= (. partial is-encoded) true)
      (= (. partial chunk-limit) 4096)
      (print (. (call (. (call (of ChunkedTokenStream Char) partial) read)) length))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)

(fn-do + boundary-bytes ([] Byte)
  (param extra Size)
  (let data ([] Byte (+ 4095 extra)))
  (for (in i (..< 0 4095))
    (= ([] data i) 97u8)
  )
  (return data)
)
```

```output
4096
4098
a
━
Unable to read UTF8 content
```

```brix
(import /core/tests/support/byte-source)
(import /core/tokenstream)
(blank)

(main
  (let attempt
    (do-new
      (let buffer-stream (call TestByteSource (as "foo;bar;baz" ([] Byte))))
      (let stream (call (of ChunkedTokenStream Byte) buffer-stream))
      (print (call (. ", " join) (call (. stream split) ";")))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)
```

```output
foo, bar, baz
```

### Each chunk a chunked stream yields keeps its tokens after the next chunk is read.

```brix
(import /core/tests/support/byte-source)
(import /core/tokenstream)
(blank)

(main
  (let text (++ (* "a" 2048) "b"))
  (let stream (call (of ChunkedTokenStream Char) (call (of TestByteSource Char) (. text codepoints))))
  (let chunks (of Iterable ([] Char)) stream)
  (let kept (call (of List ([] Char))))
  (for (in chunk chunks)
    (+= kept chunk)
  )
  (let joined "")
  (for (in chunk kept)
    (print (. chunk size))
    (= joined (++ joined (call (. chunk slice-string))))
  )
  (print (== joined text))
)
```

```output
1024
1024
1
true
```

### A delimiter read or a split of a chunked stream panics on an empty delimiter, as a string split does.

```brix
(import /core/tests/support/byte-source)
(import /core/tokenstream)
(blank)

(main
  (for (in text (list "" "a;b"))
    (let split
      (do-new
        (let stream (call (of ChunkedTokenStream Byte) (call TestByteSource (as text ([] Byte)))))
        (print (call (. ", " join) (call (. stream split) "")))
        (on-failure failure
          (print failure)
        )
      )
    )
    (call (. split wait))
    (let read
      (do-new
        (let stream (call (of ChunkedTokenStream Byte) (call TestByteSource (as text ([] Byte)))))
        (print (call (. stream read) ""))
        (on-failure failure
          (print failure)
        )
      )
    )
    (call (. read wait))
  )
  (let stream (call (of ChunkedTokenStream Byte) (call TestByteSource (as "a;;b" ([] Byte)))))
  (print (str "[" (call (. "][" join) (call (. stream split) ";")) "]"))
)
```

```output
Stream delimiter must not be empty
Stream delimiter must not be empty
Stream delimiter must not be empty
Stream delimiter must not be empty
[a][][b]
```

### Arena allocation returns raw pointers and honors raw alignment.

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena 128))
  (finally
    (call (. arena release))
  )
  (blank)
  (let filled (call (of (. arena allocate) U32) 4))
  (for (in index (..< 0 4))
    (= ([] filled index) (as (* index 3) U32))
  )
  (let raw (call (. arena allocate-raw) 24 32))
  (let empty (call (of (. arena allocate) U64) 0))
  (blank)
  (print (type-of filled))
  (print (tuple ([] filled 0) ([] filled 3)))
  (print (% (. raw address) 32))
  (print (!= (. empty address) 0))
)
```

```output
[TypeInfo CPointer'U32]
(0, 9)
0
true
```

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (let value (call (of (. arena allocate) Point) 1))
  (= ([] value 0) (call Point 10 20))
  (print (. ([] value 0) x))
  (= ([] value 0) (call Point 10 30))
  (print ([] value 0))
)

(type + Point (is Tuple)
  (prop + x I32)
  (prop + y I32)
)
```

```output
10
(10, 30)
```

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (let values (call (of (. arena allocate) U32) 4))
  (print ([] values 0))
  (= ([] values 0) 7u32)
  (= ([] values 3) 11u32)
  (print (tuple ([] values 0) ([] values 3)))
)
```

```output
0
(7, 11)
```

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (blank)
  (let values (call (of (. arena allocate) U32) 4))
  (let outer (. arena mark))
  (let scratch (call (of (. arena allocate) Byte) 32))
  (= ([] scratch 0) 99u8)
  (let inner (. arena mark))
  (let extra (call (of (. arena allocate) Point) 1))
  (= ([] extra 0) (call Point 40 50))
  (call (. arena rewind) inner)
  (call (. arena rewind) outer)
  (print (. arena rewind-count))
)

(type + Point (is Tuple)
  (prop + x I32)
  (prop + y I32)
)
```

```output
2
```

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena 64))
  (finally
    (call (. arena release))
  )
  (blank)
  (let values (call (of (. arena allocate) U32) 4))
  (= ([] values 0) 7u32)
  (let allocation-count (. arena system-block-allocation-count))
  (call (. arena reset))
  (let rebuilt (call (of (. arena allocate) U32) 4))
  (= ([] rebuilt 0) 42u32)
  (print ([] rebuilt 0))
  (print (== (. arena system-block-allocation-count) allocation-count))
  (print (. arena reset-count))
)
```

```output
42
true
1
```

### Array'Byte random-access reads and writes.

```brix
(main
  (let bytes ([] Byte 6))
  (= ([] bytes 0) 1u8)
  (= ([] bytes 1) 65u8)
  (call (of (. bytes write) U16) 4660u16 2)
  (call (of (. bytes write) U16) 4660u16 4 (. Endian little))
  (blank)
  (print (. bytes size))
  (print (as (!! (call (of (. bytes read) U16) 2)) Binary))
  (print (as (!! (call (of (. bytes read) U16) 4 (. Endian little))) Binary))
  (blank)
  (call (of (. bytes write) U32) 16909088u32 1)
  (print (tuple ([] bytes 0) ([] bytes 1) ([] bytes 2) ([] bytes 3) ([] bytes 4) ([] bytes 5)))
  (print (== (call (of (. bytes read) U64) 100) null))
)
```

```output
6
00010010 00110100
00010010 00110100
(1, 1, 2, 3, 32, 18)
true
```

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena 128))
  (finally
    (call (. arena release))
  )
  (blank)
  (let dirty (call (of (. arena allocate) U32) 4))
  (for (in index (..< 0 4))
    (= ([] dirty index) (as (+ index 10) U32))
  )
  (call (. arena reset))
  (let zeroed (call (of (. arena allocate) U32) 4))
  (print (tuple ([] zeroed 0) ([] zeroed 3)))
  (print (. arena system-block-allocation-count))
)
```

```output
(0, 0)
1
```

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (blank)
  (let values (call (of (. arena allocate) U32) 4))
  (for (in index (..< 0 4))
    (= ([] values index) (as (+ index 1) U32))
  )
  (let middle (+ values 1))
  (print (tuple ([] middle 0) ([] middle 1) (+ ([] middle 0) ([] middle 1))))
  (print (call sum-arena-pointer middle 2))
)

(fn-do + sum-arena-pointer U32
  (param pointer (of CPointer U32))
  (param length Size)
  (let sum 0u32)
  (for (in index (..< 0 length))
    (+= sum ([] pointer index))
  )
  (return sum)
)
```

```output
(2, 3, 5)
5
```

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (let values (call (of (. arena allocate) U32) 4))
  (let initial-bytes (. arena active-bytes))
  (print (== (call (. arena try-allocate-raw) 8 3) null))
  (print (== (. arena active-bytes) initial-bytes))
)
```

```output
true
true
```

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (finally
    (call (. arena release))
  )
  (let values (call (of (. arena allocate) U32) 4))
  (let initial-bytes (. arena active-bytes))
  (print (== (call (of (. arena try-allocate) U64) (. Size max)) null))
  (print (== (. arena active-bytes) initial-bytes))
)
```

```output
true
true
```

```brix crash
(main
  (let items (. (call (of Array Point) 1) immutable))
  (= (. ([] (. items rows) 0) x) 1)
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
Exception: Cannot write through a read-only array view
<no file:4>
      (= (. ([] (. items rows) 0) x) 1)
         ^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix crash
(main
  (let items (. (call (of Array Point) 1) immutable))
  (= ([] (. items fields x) 0) 1)
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
Exception: Cannot write through a read-only array view
<no file:4>
      (= ([] (. items fields x) 0) 1)
         ^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix crash
(main
  (let items (. (call (of Array Point) 1) immutable))
  (= ([] ([] (. items rows) 0) 0) 1)
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
Exception: Cannot write through a read-only array view
<no file:4>
      (= ([] ([] (. items rows) 0) 0) 1)
         ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix crash
(main
  (let items (. (call (of Array Point) 1) immutable))
  (let row ([] (. items rows) 0))
  (= (. row x) 1)
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
Exception: Cannot write through a read-only array view
<no file:5>
      (= (. row x) 1)
         ^^^^^^^^^
```

```brix crash
(main
  (let items (. (call (of Array Point) 1) immutable))
  (let range ([] (. items rows) (..< 0 1)))
  (= ([] range 0) (call Point 1 2))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
Exception: Cannot write through a read-only array view
<no file:5>
      (= ([] range 0) (call Point 1 2))
         ^^^^^^^^^^^^
```

```brix crash
(main
  (let items (. (call (of Array Point) 1) immutable))
  (let fields (. items fields x))
  (= ([] fields (..< 0 1)) fields)
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
Exception: Cannot write through a read-only array view
<no file:5>
      (= ([] fields (..< 0 1)) fields)
         ^^^^^^^^^^^^^^^^^^^^^
```

```brix crash
(main
  (let items (. (call (of Array Point) 1) immutable))
  (call write (. items rows))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)

(fn-do + write _
  (param rows (of ArrayRows Point ArrayWriteAccess))
  (= (. ([] rows 0) x) 1)
)
```

```output
Exception: No function "write" accepts (ArrayRows'Point'ArrayReadAccess); it is declared as "write(rows ArrayRows'Point'ArrayWriteAccess)"
<no file:4>
      (call write (. items rows))
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix crash
(main
  (let items (. (call (of Array Point) 1) immutable))
  (let writable (as (. items rows) (of ArrayRows Point ArrayWriteAccess)))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
Exception: ArrayRows'Point'ArrayReadAccess cannot be cast to ArrayRows'Point'ArrayWriteAccess; there is no conversion between them
<no file:4>
      (let writable (as (. items rows) (of ArrayRows Point ArrayWriteAccess)))
                    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix crash
(main
  (let items (. (call (of Array Point) 1) immutable))
  (let writable (as ([] (. items rows) 0) (of ArrayRow Point ArrayWriteAccess)))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
Exception: ArrayRow'Point'ArrayReadAccess cannot be cast to ArrayRow'Point'ArrayWriteAccess; there is no conversion between them
<no file:4>
      (let writable (as ([] (. items rows) 0) (of ArrayRow Point ArrayWriteAccess)))
                    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix crash
(main
  (let items (. (call (of Array Point) 1) immutable))
  (let writable (as ([] (. items rows) (..< 0 1)) (of ArrayRowRange Point ArrayWriteAccess)))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
Exception: ArrayRowRange'Point'ArrayReadAccess cannot be cast to ArrayRowRange'Point'ArrayWriteAccess; there is no conversion between them
<no file:4>
      (let writable (as ([] (. items rows) (..< 0 1)) (of ArrayRowRange Point ArrayWriteAccess)))
                    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix crash
(main
  (let items (. (call (of Array Point) 1) immutable))
  (let writable (as (. items fields x) (of ArrayField Int ArrayWriteAccess)))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
Exception: ArrayField'I64'ArrayReadAccess cannot be cast to ArrayField'I64'ArrayWriteAccess; there is no conversion between them
<no file:4>
      (let writable (as (. items fields x) (of ArrayField Int ArrayWriteAccess)))
                    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix crash
(main
  (let items ([] Int 1))
  (print (. items rows))
)
```

```output
Exception: Array views require a fixed-layout product aggregate
<no file:4>
      (print (. items rows))
             ^^^^^^^^^^^^^^
```

```brix crash
(main
  (let items (call (of Array (of Array Int)) 1))
  (print (. items fields))
)
```

```output
Exception: Array views require a fixed-layout product aggregate
<no file:4>
      (print (. items fields))
             ^^^^^^^^^^^^^^^^
```

```brix crash
(main
  (let items (call (of Array Variant) 1))
  (print (. items rows))
)

(type + Variant (is Union)
  (variant point Point)
  (variant empty)
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
Exception: Array views require a fixed-layout product aggregate
<no file:4>
      (print (. items rows))
             ^^^^^^^^^^^^^^
```

```brix
(main
  (let items (call (of Array Point) (list (call Point 3 4) (call Point 5 6))))
  (let rows (. items rows))
  (= (. ([] rows 0) x) 9)
  (print (. ([] items 0) x))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
9
```

```brix
(main
  (let items (call (of Array Point) (list (call Point 3 4) (call Point 5 6))))
  (let fields (. items fields x))
  (= ([] fields 1) 12)
  (print (. ([] items 1) x))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
12
```

```brix
(main
  (let items (call (of Array Point) (list (call Point 3 4) (call Point 5 6))))
  (let snapshot (as ([] (. items rows) 0) Point))
  (= (. ([] (. items rows) 0) x) 9)
  (print (. snapshot x))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
3
```

```brix
(main
  (let items (call (of Array Point) (list (call Point 3 4) (call Point 5 6))))
  (let rows (. items rows))
  (for (in row rows)
    (print (. row x))
  )
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
3
5
```

```brix
(main
  (let items (call (of Array Point) (list (call Point 3 4) (call Point 5 6))))
  (print (. ([] (call (. ([] (. items rows) (..< 0 1)) to-array)) 0) x))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
3
```

```brix
(main
  (let items (call (of Array Point) (list (call Point 3 4) (call Point 5 6))))
  (print ([] (call (. items fields x to-immutable-array)) 1))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
5
```

```brix
(main
  (let items (call (of Array Point) (list (call Point 3 4) (call Point 5 6))))
  (print (call first-x (. items rows)))
  (print (call first-x (. items immutable rows)))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)

(fn-expr + first-x _
  (param rows (of ArrayRows Point ArrayReadAccess))
  (. ([] rows 0) x)
)
```

```output
3
3
```

```brix
(main
  (let items (call (of Array Point) (list (call Point 3 4) (call Point 5 6))))
  (let detached (call (. items rows to-array)))
  (= (. ([] (. items rows) 0) x) 21)
  (print (. ([] detached 0) x))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
3
```

```brix
(main
  (let items (call (of Array Point) (list (call Point 3 4) (call Point 5 6))))
  (let rows (. items immutable rows))
  (= (. ([] (. items rows) 0) x) 21)
  (print (. ([] rows 0) x))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
21
```

```brix
(main
  (let items (call (of Array Point) (list (call Point 3 4) (call Point 5 6))))
  (let value ([] items 0))
  (= (. value x) 99)
  (print (. ([] items 0) x))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
3
```

```brix
(main
  (let items (call (of Array Point) (list (call Point 3 4) (call Point 5 6))))
  (for (in value items)
    (= (. value x) 100)
  )
  (print (. ([] items 0) x))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
3
```

```brix
(main
  (let items (call (of Array Point) (list (call Point 3 4) (call Point 5 6))))
  (let rows (. items rows))
  (= ([] rows (..< 0 1)) ([] rows (..< 1 2)))
  (print (. ([] items 0) x))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
5
```

```brix
(main
  (let items (call (of Array Point) (list (call Point 1 2) (call Point 3 4) (call Point 5 6))))
  (let field (. items fields x))
  (= ([] field (..< 1 3)) ([] field (..< 0 2)))
  (print (call (. ", " join) (call (. field to-array))))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
1, 1, 3
```

```brix
(main
  (let items (call (of Array Pair) (list (call Pair 7 8))))
  (= ([] ([] (. items rows) 0) 1) 10)
  (print ([] ([] (. items fields) 1) 0))
  (print (. ([] (. items rows) 0) left))
)

(type + Pair (is Tuple)
  (prop + left Int)
  (prop + right Int)
)
```

```output
10
7
```

```brix
(main
  (let items (call (of Array Point)))
  (print (tuple (. items rows size) (. ([] (. items rows) (..< 0 0)) size) (. items fields x size)))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
(0, 0, 0)
```

```brix
(import /BxCore/BxCore.h)
(blank)

(main
  (let rows (call make-rows))
  (call C:brixCollect)
  (print (. ([] rows 0) x))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)

(fn-do + make-rows (of ArrayRows Point ArrayWriteAccess)
  (let local (call (of Array Point) (list (call Point 41 42))))
  (return (. local rows))
)
```

```output
41
```

```brix
(main
  (do-new
    (let items (call (of Array Point) (list (call Point 3 4) (call Point 5 6))))
    (let view ([] (. items rows) 0))
    (let grown (call (. items resize) 4))
    (print (. grown rows size))
    (print (. view x))
    (on-failure failure
      (print failure)
    )
  )
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
4
Stale array view
```

```brix
(import /core/arena)
(blank)

(main
  (let pool (call Arena))
  (finally
    (call (. pool release))
  )
  (let attempt
    (do-new
      (let items (call (of Array Point) pool (list (call Point 31 32))))
      (let view ([] (. items rows) 0))
      (call (. pool reset))
      (print (. view x))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
Stale array view
```

```brix
(main
  (do-new
    (let items (call (of Array Point) (list (call Point 3 4) (call Point 5 6))))
    (let view ([] (. items rows) (..< 0 1)))
    (let grown (call (. items resize) 4))
    (print (. grown rows size))
    (for (in row view)
      (print "unexpected loop body")
    )
    (on-failure failure
      (print failure)
    )
  )
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
4
Stale array view
```

```brix
(import /core/arena)
(blank)

(main
  (let pool (call Arena))
  (finally
    (call (. pool release))
  )
  (let attempt
    (do-new
      (let items (call (of Array Point) pool (list (call Point 31 32))))
      (let view ([] (. items rows) (..< 0 1)))
      (call (. pool reset))
      (for (in row view)
        (print "unexpected loop body")
      )
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
Stale array view
```

```brix
(main
  (do-new
    (let items (call (of Array Point) (list (call Point 3 4) (call Point 5 6))))
    (let view (. items fields x))
    (let grown (call (. items resize) 4))
    (print (. grown rows size))
    (print ([] view 0))
    (on-failure failure
      (print failure)
    )
  )
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
4
Stale array view
```

```brix
(import /core/arena)
(blank)

(main
  (let pool (call Arena))
  (finally
    (call (. pool release))
  )
  (let attempt
    (do-new
      (let items (call (of Array Point) pool (list (call Point 31 32))))
      (let view (. items fields x))
      (call (. pool reset))
      (print ([] view 0))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
Stale array view
```

```brix
(main
  (do-new
    (let items (call (of Array Point) (list (call Point 3 4) (call Point 5 6))))
    (print (. ([] (. items rows) 2) x))
    (on-failure failure
      (print failure)
    )
  )
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
Array view index out of bounds
```

```brix
(main
  (do-new
    (let items (call (of Array Point) (list (call Point 3 4) (call Point 5 6))))
    (print (. ([] (. items rows) (..< 0 3)) size))
    (on-failure failure
      (print failure)
    )
  )
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
Array view range out of bounds
```

```brix
(main
  (do-new
    (let items (call (of Array Point) (list (call Point 51 52))))
    (let row ([] (. items rows) 0))
    (= items (call (of Array Point) (list (call Point 61 62))))
    (print (. ([] (. items rows) 0) x))
    (print (. row x))
    (on-failure failure
      (print failure)
    )
  )
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
61
Stale array view
```

```brix
(main
  (do-new
    (let items (call (of Array Point) (list (call Point 71 72) (call Point 73 74))))
    (let field (. items fields x))
    (let smaller (call (. items resize) 1))
    (print (. ([] (. smaller rows) 0) x))
    (print ([] field 0))
    (on-failure failure
      (print failure)
    )
  )
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
71
Stale array view
```

```brix
(main
  (do-new
    (let items (call (of Array (of Array Point)) 1))
    (= ([] items 0) (call (of Array Point) (list (call Point 81 82))))
    (let row ([] (. ([] items 0) rows) 0))
    (= ([] items 0) (call (of Array Point) (list (call Point 91 92))))
    (print (. ([] (. ([] items 0) rows) 0) x))
    (print (. row x))
    (on-failure failure
      (print failure)
    )
  )
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
91
Stale array view
```

```brix
(main
  (do-new
    (let holder (call Holder (call (of Array Point) (list (call Point 101 102)))))
    (let row ([] (. holder items rows) 0))
    (= (. holder items) (call (of Array Point) (list (call Point 111 112))))
    (print (. ([] (. holder items rows) 0) x))
    (print (. row x))
    (on-failure failure
      (print failure)
    )
  )
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)

(type + Holder
  (prop + items (of Array Point))
  (fn-do + This _
    (param items (of Array Point))
  )
)
```

```output
111
Stale array view
```

```brix
(main
  (let holder (call Holder (call (of Array Point) (list (call Point 111 112)))))
  (let row ([] (. holder items rows) 0))
  (= (. holder items) (. holder items))
  (= ([] (. holder items) 0) (call Point 121 122))
  (print (. row x))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)

(type + Holder
  (prop + items (of Array Point))
  (fn-do + This _
    (param items (of Array Point))
  )
)
```

```output
121
```

```brix
(main
  (let items (call (of Array Point)))
  (print (tuple (. (call (. items rows to-array)) size) (. (call (. items immutable fields x to-immutable-array)) size)))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
(0, 0)
```

```brix
(main
  (let items (call (of Array Point) (list (call Point 3 4) (call Point 5 6))))
  (print (. ([] (. items rows) (.. (. Position first) (. Position last))) size))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
2
```

```brix
(main
  (let items (call (of Array Point) (list (call Point 3 4) (call Point 5 6))))
  (let field (. items fields x))
  (for (in x ([] field (..< 0 1)))
    (print x)
  )
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
3
```

```brix
(import /core/arena)
(blank)

(main
  (let pool (call Arena))
  (finally
    (call (. pool release))
  )
  (let kept (call (of Array Point) pool (list (call Point 129 130))))
  (let row ([] (. kept rows) 0))
  (let boundary (. pool mark))
  (let transient (call (of Array Point) pool (list (call Point 131 132))))
  (call (. pool rewind) boundary)
  (print (. row x))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
129
```

```brix
(import /core/arena)
(blank)

(main
  (let pool (call Arena))
  (finally
    (call (. pool release))
  )
  (let attempt
    (do-new
      (let kept (call (of Array Point) pool (list (call Point 129 130))))
      (let boundary (. pool mark))
      (let transient (call (of Array Point) pool (list (call Point 131 132))))
      (let row ([] (. transient rows) 0))
      (call (. pool rewind) boundary)
      (print (. row x))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
Stale array view
```

```brix
(import /core/arena)
(blank)

(main
  (let attempt
    (do-new
      (let pool (call Arena))
      (let items (call (of Array Point) pool (list (call Point 141 142))))
      (let row ([] (. items rows) 0))
      (call (. pool release))
      (print (. row x))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
Stale array view
```

```brix
(main
  (let values (call (of Array Point) (list (call Point 1 2) (call Point 3 4))))
  (print (tuple (call scan (. values rows)) (call scan (. values immutable rows))))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)

(fn-do + (of scan Access) _
  (param rows (of ArrayRows Point Access))
  (let total 0)
  (for (in row rows)
    (+= total (+ (. row x) (. row y)))
  )
  (return total)
)
```

```output
(10, 10)
```

```brix
(main
  (let values (call (of Array Point) (list (call Point 1 2) (call Point 3 4))))
  (for (in row (. values rows))
    (+= (. row x) (. row y))
  )
  (print (tuple (. ([] values 0) x) (. ([] values 1) x)))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
(3, 7)
```

```brix
(main
  (let values (call (of Array Point) (list (call Point 1 2) (call Point 3 4))))
  (let rows (. values rows))
  (for (in index (..< 0 (. rows length)))
    (+= (. ([] rows index) x) (. ([] rows index) y))
  )
  (print (tuple (. ([] values 0) x) (. ([] values 1) x)))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
(3, 7)
```

```brix
(main
  (let values (call (of Array Point) (list (call Point 1 2) (call Point 3 4))))
  (let saved (call (of Array (of ArrayRow Point ArrayWriteAccess)) 2))
  (let position 0)
  (for (in row (. values rows))
    (= ([] saved position) row)
    (+= position 1)
  )
  (= (. ([] saved 0) x) 20)
  (print (. ([] values 0) x))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
20
```

```brix
(main
  (let values (call (of Array Point) (list (call Point 1 2) (call Point 3 4))))
  (let row (call first (. values rows)))
  (= (. row x) 30)
  (print (. ([] values 0) x))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)

(fn-do + first (of ArrayRow Point ArrayWriteAccess)
  (param rows (of ArrayRows Point ArrayWriteAccess))
  (for (in row rows)
    (return row)
  )
  (throw (call Panic "empty"))
)
```

```output
30
```

```brix
(main
  (let values (call (of Array Point) (list (call Point 1 2) (call Point 3 4))))
  (let snapshot (call snapshot-first (. values rows)))
  (= (. ([] (. values rows) 0) x) 40)
  (print (. snapshot x))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)

(fn-do + snapshot-first Point
  (param rows (of ArrayRows Point ArrayReadAccess))
  (for (in row rows)
    (return (as row Point))
  )
  (throw (call Panic "empty"))
)
```

```output
1
```

```brix
(main
  (let values (call (of Array Point) (list (call Point 1 2) (call Point 3 4))))
  (let read-later (call capture-first (. values rows)))
  (= (. ([] (. values rows) 0) x) 50)
  (print (call read-later))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)

(fn-do + capture-first Reader
  (param rows (of ArrayRows Point ArrayReadAccess))
  (for (in row rows)
    (return (=> () (. row x)))
  )
  (throw (call Panic "empty"))
)

(type + Reader (is (fn-type Int)))
```

```output
50
```

```brix
(import /core/arena)
(blank)

(main
  (let pool (call Arena))
  (finally
    (call (. pool release))
  )
  (let attempt
    (do-new
      (let values (call (of Array Point) pool (list (call Point 8 9))))
      (for (in row (. values rows))
        (call (. pool reset))
        (print (. row x))
      )
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
Stale array view
```

```brix
(main
  (let values (call (of Array Point) (list (call Point 1 2) (call Point 3 4))))
  (call shrink-during-indexed values)
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)

(fn-do + shrink-during-indexed _
  (param values (of Array Point))
  (let attempt
    (do-new
      (let rows (. values rows))
      (for (in index (..< 0 (. rows length)))
        (= values (call (. values resize) 1))
        (print (. ([] rows index) x))
      )
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)
```

```output
Stale array view
```

```brix
(main
  (do-new
    (let values (call (of Array Point) (list (call Point 1 2) (call Point 3 4))))
    (let rows (. values rows))
    (let resized (call (. values resize) 1))
    (let count 0)
    (for (in row rows)
      (+= count 1)
      (print (. row x))
    )
    (print count)
    (on-failure failure
      (print failure)
    )
  )
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
Stale array view
```

```brix
(import /BxCore/BxCore.h)
(import /BxGC/BxGC.h)
(blank)

(main
  (let storage (call (of Array Cell) 2 (:with (call Cell (call GCNode (as (+ each 1) Int)) (call GCNode (as (+ each 10) Int))))))
  (let table (. storage rows))
  (call C:brixCollect)
  (blank)
  (if (not (call C:brixGCVerifyObject (as (. ([] table 0) left) Pointer)))
    (print "array-row-left-missing")
    (else-if (not (call C:brixGCVerifyObject (as (. ([] table 1) right) Pointer)))
      (print "array-row-right-missing")
    )
    (else
      (print (str "array-row-ref-ok " (. ([] table 0) left value) " " (. ([] table 1) right value)))
    )
  )
)

(type + Cell (is Struct)
  (prop + left GCNode)
  (prop + right GCNode)
)

(type + GCNode
  (prop + value _ (= 0))
  (fn-do + This _
    (param value Int)
  )
)
```

```output
array-row-ref-ok 1 11
```

```brix
(main
  (let items (call (of Array Entity) 3 (:with (call Entity (as each I32) (call Point (as each I32) (as (+ each 10) I32))))))
  (print (call (. ", " join) (. items fields id)))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)

(type + Entity (is Struct)
  (prop + id I32)
  (prop + position Point)
)
```

```output
0, 1, 2
```

```brix
(main
  (let items (call (of Array Entity) 3 (:with (call Entity (as each I32) (call Point (as each I32) (as (+ each 10) I32))))))
  (print (call (. ", " join) (. items fields position)))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)

(type + Entity (is Struct)
  (prop + id I32)
  (prop + position Point)
)
```

```output
(0, 10), (1, 11), (2, 12)
```

```brix
(main
  (let items (call (of Array Entity) 3 (:with (call Entity (as each I32) (call Point (as each I32) (as (+ each 10) I32))))))
  (= ([] (. items fields id) 1) 42)
  (print (as ([] (. items rows) 1) Entity))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)

(type + Entity (is Struct)
  (prop + id I32)
  (prop + position Point)
)
```

```output
(42, (1, 11))
```

```brix
(main
  (let items (call (of Array Entity) 3 (:with (call Entity (as each I32) (call Point (as each I32) (as (+ each 10) I32))))))
  (= ([] (. items fields position) 2) (call Point 7 8))
  (print (as ([] (. items rows) 2) Entity))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)

(type + Entity (is Struct)
  (prop + id I32)
  (prop + position Point)
)
```

```output
(2, (7, 8))
```

```brix
(import /core/array)
(blank)

(main
  (let attempt
    (do-new
      (let values (call (of Array InlineRecord) 2 (:with (call InlineRecord "kept" (as each Int)))))
      (let snapshot ([] values 0))
      (= ([] values 0) (call InlineRecord "changed" 7))
      (print (tuple (. snapshot text) (. snapshot number)))
      (print (tuple (. ([] values 0) text) (. ([] values 0) number)))
      (print (tuple (. ([] values 1) text) (. ([] values 1) number)))
      (print (. ([] values 2) text))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)

(type + InlineRecord (is Tuple)
  (prop + text String)
  (prop + number Int)
)
```

```output
(kept, 0)
(changed, 7)
(kept, 1)
Index out of bounds (2 / 2)
```

```brix
(import /core/array)
(blank)

(main
  (let values (call (of Array InlineMutable) 2 (:with (call InlineMutable (as each Int) (as (+ each 10) Int)))))
  (let loaded ([] values 0))
  (= (. loaded x) 99)
  (print (tuple (. ([] values 0) x) (. loaded x)))
  (= ([] values 1) loaded)
  (= (. loaded y) 88)
  (print (tuple (. ([] values 1) x) (. ([] values 1) y) (. loaded y)))
)

(type + InlineMutable (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
(0, 99)
(99, 10, 88)
```

```brix crash
(main
  (let values (. (call (of Array I32) (list 1)) immutable))
  (= ([] values 0) 2)
)
```

```output
Exception: ImmutableArray does not support element assignment
<no file:4>
      (= ([] values 0) 2)
         ^^^^^^^^^^^^^
```

```brix crash
(main
  (let values (. (call (of Array I32) (list 1)) immutable))
  (call (. values resize) 4)
)
```

```output
Exception: ImmutableArray'I32'GCBacking has no method "resize"; did you mean "size"?
<no file:4>
      (call (. values resize) 4)
      ^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix crash
(main
  (let values (. (call (of Array I32) (list 1)) immutable))
  (= ([] (. values data) 0) 2)
)
```

```output
Exception: Property "data" not found
<no file:4>
      (= ([] (. values data) 0) 2)
         ^^^^^^^^^^^^^^^^^^^^^^
```

```brix crash
(main
  (let values (. (call (of Array I32) (list 1)) immutable))
  (= (. values size) 4)
)
```

```output
Exception: Setter not found
<no file:4>
      (= (. values size) 4)
         ^^^^^^^^^^^^^^^
```

```brix
(main
  (print (call accept (call (of Array I32) (list 1 2))))
)

(fn-expr + accept Size
  (param values (of ImmutableArray I32))
  (. values length)
)
```

```output
2
```

### An array of one item type is not an array of another, whose items it would read as the other.

```brix crash
(main
  (call accept (. (call (of Array I32) (list 1)) immutable))
)

(fn-expr + accept _
  (param values (of ImmutableArray String))
  (. values length)
)
```

```output
Exception: No function "accept" accepts (ImmutableArray'I32'GCBacking); it is declared as "accept(values ImmutableArray'String)"
<no file:3>
      (call accept (. (call (of Array I32) (list 1)) immutable))
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### `as` reads an array as one of another item type of the same size.

```brix
(main
  (let codes (call (of Array U32) (list 104u32 105u32)))
  (let chars (as codes (of Array Char)))
  (print (. chars length))
  (print ([] chars 1))
)
```

```output
2
i
```

### An array cast to one whose struct items are of another size panics, as the count it keeps would reach past its storage.

```brix
(main
  (let attempt
    (do-new
      (let points ([] Point 2))
      (let triples (as points (of Array Triple)))
      (print (. triples length))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)

(type + Triple (is Struct)
  (prop + a I32)
  (prop + b I32)
  (prop + c I32)
)
```

```output
An array of 8-byte items cannot be cast to one of 12-byte items
```

```brix
(import /core/array-builder)
(blank)

(main
  (let builder (call (of ArrayBuilder Node)))
  (let values (call (. builder finish-immutable)))
  (let direct (list Node) values)
  (let result (call fallback values))
  (print (. result length))
)

(fn-expr + fallback (list Node)
  (param optional (? (of ImmutableArray Node)))
  (?? optional empty)
  (where
    (lazy empty (list Node))
  )
)

(type + Node (is Union)
  (variant items (list Node))
  (variant number I32)
)
```

```output
0
```

### Byte arrays read and write floating-point values by their bit patterns in either byte order.

```brix
(main
  (let bytes ([] Byte 12))
  (call (of (. bytes write) F64) 259.25 0 .little)
  (call (of (. bytes write) F32) -82.5f32 8)
  (print (!! (call (of (. bytes read) F64) 0 .little)))
  (print (!! (call (of (. bytes read) F32) 8)))
  (print (tuple ([] bytes 7) ([] bytes 8)))
)
```

```output
259.25
-82.5
(64, 194)
```

### An array of text, of objects or of structs that hold text keeps its items across collections.

```brix
(import /BxGC/BxGC.h)
(blank)

(main
  (let kept (call (of Array String) 4))
  (= ([] kept 0) (str "zero" (. kept size)))
  (= ([] kept 1) (str "one" (. kept size)))
  (= ([] kept 2) (str "two" (. kept size)))
  (= ([] kept 3) (str "three" (. kept size)))
  (let boxes (call (of Array Box) 2))
  (= ([] boxes 0) (call Box (str "left" (. kept size))))
  (= ([] boxes 1) (call Box (str "right" (. kept size))))
  (let pairs (call (of Array Pair) 2))
  (= ([] pairs 1) (call Pair 2 (str "two" (. kept size))))
  (call C:brixGCCollect)
  (let total 0)
  (for (in round (..< 0 2000))
    (let text (str "junk" round))
    (+= total (. text size))
    (let more (call (of Array String) 2))
    (= ([] more 1) text)
    (+= total (. ([] more 1) size))
  )
  (call C:brixGCCollect)
  (print total)
  (print (tuple ([] kept 0) ([] kept 1) ([] kept 2) ([] kept 3)))
  (print (tuple (. ([] boxes 0) name) (. ([] boxes 1) name)))
  (print (tuple (. ([] pairs 1) name) (. ([] pairs 1) id)))
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
29780
(zero4, one4, two4, three4)
(left4, right4)
(two4, 2)
```

### An array held by an arena stores and reads numbers, structs, tuples and text, and its copy is apart from it.

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (let values (call (of Array U32) arena 3))
  (= ([] values 0) 10u32)
  (= ([] values 1) 21u32)
  (print (tuple ([] values 0) ([] values 1) ([] values 2)))
  (let copy (call (. values copy) arena))
  (= ([] copy 0) 99u32)
  (print (tuple ([] values 0) ([] copy 0) ([] copy 1)))
  (let points (call (of Array Point) arena 2))
  (= ([] points 1) (call Point 3 4))
  (print ([] points 0))
  (print ([] points 1))
  (print (. ([] points 1) y))
  (let tuples (call (of Array (tuple Int Int)) arena 2))
  (= ([] tuples 1) (tuple 7 8))
  (print ([] tuples 1))
  (let names (call (of Array String) arena 2))
  (= ([] names 0) "kept")
  (print ([] names 0))
  (call (. arena release))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
(10, 21, 0)
(10, 99, 21)
(0, 0)
(3, 4)
4
(7, 8)
kept
```

### An array held by an arena is resized into the arena, larger and smaller, and keeps its items.

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (let values (call (of Array U32) arena 2))
  (= ([] values 0) 10u32)
  (= ([] values 1) 21u32)
  (let grown (call (. values resize) 4 arena))
  (= ([] grown 3) 5u32)
  (print (tuple ([] grown 0) ([] grown 1) ([] grown 2) ([] grown 3) (. grown size)))
  (let less (call (. grown resize) 1 arena))
  (print (tuple ([] less 0) (. less size)))
  (let texts (call (of Array String) arena 1))
  (= ([] texts 0) "kept")
  (let more (call (. texts resize) 3 arena))
  (= ([] more 2) "new")
  (print (tuple ([] more 0) ([] more 2)))
  (call (. arena release))
)
```

```output
(10, 21, 0, 5, 4)
(10, 1)
(kept, new)
```

### An array of objects held by an arena may have no items.

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (let lines (call (of Array Line) arena 0))
  (print (. lines size))
  (let after (call (of Array Line) arena 1))
  (print (. after size))
  (call (. arena release))
)

(type + Line
  (prop + name String)
  (prop + parts (of ImmutableArray Int))
)
```

```output
0
1
```

### A loop over an array made with a length.

```brix
(main
  (let items (call (of Array Int) 3))
  (= ([] items 1) 5)
  (for (in item items)
    (print item)
  )
)
```

```output
0
5
0
```

### A range of the items of an array is assigned one value.

```brix
(main
  (let buffer (call (of Array Int) 5))
  (= ([] buffer (..< 1 3)) 7)
  (for (in item buffer)
    (print item)
  )
)
```

```output
0
7
7
0
0
```

### An array is sliced by ranges of positions, of a number and a position, and by one with its end left open.

```brix
(main
  (let buffer (call (of Array Float) 4))
  (= ([] buffer 0) 0.5)
  (= ([] buffer 1) 1.5)
  (= ([] buffer 2) 2.5)
  (= ([] buffer 3) 3.5)
  (print (call (. ", " join) ([] buffer (..< (. Position first) 2))))
  (print (call (. ", " join) ([] buffer (..< 1 (. Position end)))))
  (print (call (. ", " join) ([] buffer (.. (. Position middle) (. Position last)))))
  (print (call (. ", " join) ([] buffer (... 1))))
)
```

```output
0.5, 1.5
1.5, 2.5, 3.5
2.5, 3.5
1.5, 2.5, 3.5
```

### An empty slice from the end of a full array or list points inside its storage, not at the next object.

```brix
(main
  (let numbers (call (of Array Int) 4))
  (let tail ([] numbers (... 4)))
  (print (tuple (. tail length) (call inside (. tail data) (. numbers data) 4)))
  (let list (list 1 2 3 4))
  (let rest ([] list (... 4)))
  (let storage (. list buffer-pointer))
  (print (tuple (. rest length) (call inside (. rest buffer-pointer) storage (. list capacity))))
)

(fn-do + inside Bool
  (param at (of CPointer Int))
  (param start (of CPointer Int))
  (param count Size)
  (let end (+ (. start address) (* count (. Int size))))
  (return (and (>= (. at address) (. start address)) (< (. at address) end)))
)
```

```output
(0, true)
(0, true)
```

### An empty view from the end of a full array's rows or fields does not keep the next object alive.

```brix
(import /BxGC/BxGCTest.h)
(blank)

(main
  (print (call rows-tail))
  (print (call x-tail))
  (print (call y-tail))
)

(fn-do + rows-tail String
  (call C:brixCollect)
  (let items (call (of Array Point) 2))
  (let start (. (. items data) address))
  (let adjacent (call dropped-neighbor start))
  (let witness (call dropped-address))
  (let tail ([] (. items rows) (..< 2 2)))
  (let kept (call neighbor-kept start witness))
  (return (str "rows " (. tail length) " " (or (not adjacent) (not kept))))
)

(fn-do + x-tail String
  (call C:brixCollect)
  (let items (call (of Array Point) 2))
  (let start (. (. items data) address))
  (let adjacent (call dropped-neighbor start))
  (let witness (call dropped-address))
  (let tail ([] (. items fields x) (..< 2 2)))
  (let kept (call neighbor-kept start witness))
  (return (str "x " (. tail length) " " (or (not adjacent) (not kept))))
)

(fn-do + y-tail String
  (call C:brixCollect)
  (let items (call (of Array Point) 2))
  (let start (. (. items data) address))
  (let adjacent (call dropped-neighbor start))
  (let witness (call dropped-address))
  (let tail ([] (. items fields y) (..< 2 2)))
  (let kept (call neighbor-kept start witness))
  (return (str "y " (. tail length) " " (or (not adjacent) (not kept))))
)

(fn-do + neighbor-kept Bool
  (param start U64)
  (param witness U64)
  (call C:brixGCStartConcurrentMark)
  (call C:brixGCFinishConcurrentMark)
  (let kept (call C:brixGCTestObjectMarked (as (+ start 32) Pointer)))
  (let white (not (call C:brixGCTestObjectMarked (as witness Pointer))))
  (call C:brixCollect)
  (return (and kept white))
)

(fn-do + dropped-address U64
  (let other (call (of Array Point) 2))
  (return (. (. other data) address))
)

(fn-do + dropped-neighbor Bool
  (param start U64)
  (for (in attempt (..< 0 16))
    (let other (call (of Array Point) 2))
    (return-if true (if (== (. (. other data) address) (+ start 32))))
  )
  (return false)
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
rows 0 true
x 0 true
y 0 true
```

### Ranges with a side left open assign one value to the items of an array.

```brix
(main
  (let buffer (call (of Array Int) 5))
  (= ([] buffer (... 3)) 7)
  (= ([] buffer (... _ 1)) 9)
  (for (in item buffer)
    (print item)
  )
)
```

```output
9
0
0
7
7
```

### A stepped range ending at the last item assigns one value in either direction, and one past the end changes nothing.

```brix
(main
  (let ahead (call (of Array Int) 5))
  (= ([] ahead (.. 0 4 (by: 2))) 1)
  (print (call (. ", " join) ahead))
  (let back (call (of Array Int) 5))
  (= ([] back (.. 4 0 (by: 2))) 1)
  (print (call (. ", " join) back))
  (do-new
    (= ([] back (.. 0 6 (by: 2))) 7)
    (on-failure failure
      (print failure)
      (print (call (. ", " join) back))
    )
  )
)
```

```output
1, 0, 1, 0, 1
1, 0, 1, 0, 1
Set out of bounds (6 / 5)
1, 0, 1, 0, 1
```

```brix
(main
  (let large (call (of Array U8) 4098))
  (= ([] large 4097) 7u8)
  (print (call read-large large))
  (print ([] large 4096))
)

(fn-expr + read-large U8
  (param values (of Array U8))
  ([] values 4097)
)
```

```output
7
0
```

```brix crash
(main
  (let items (. (call (of Array Point) 1) immutable))
  (for (in row (. items rows))
    (= (. row x) 1)
  )
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
Exception: Cannot write through a read-only array view
<no file:5>
        (= (. row x) 1)
           ^^^^^^^^^
```

### An element dropped when resize shrinks an array is released.

```brix
(import /core/gc)
(blank)
(import /BxGC/BxGC.h)
(blank)

(main
  (let resized (call shrunk-buffer))
  (call C:brixCollect)
  (print (. ([] resized 0) size))
  (print (== (. ([] resized 1) get) null))
)

(fn-do + shrunk-buffer _
  (let buffer (call (of Array GCNode) 8))
  (for (in i (..< 0 8))
    (= ([] buffer i) (call GCNode i))
  )
  (blank)
  (let tail (call Weak ([] buffer 7)))
  (= buffer (call (. buffer resize) 2))
  (return (tuple buffer tail))
)

(type + GCNode
  (prop + value _ (= 0))
  (fn-do + This _
    (param v Int)
    (= value v)
  )
)
```

```output
2
true
```

### Resizing an array leaves an array sharing its storage whole.

```brix
(import /BxGC/BxGC.h)
(blank)

(main
  (let alias (call shrunk-alias))
  (call C:brixCollect)
  (print (tuple (. alias size) (. ([] alias 0) value) (. ([] alias 7) value)))
)

(fn-do + shrunk-alias _
  (let buffer (call (of Array GCNode) 8))
  (for (in i (..< 0 8))
    (= ([] buffer i) (call GCNode i))
  )
  (let alias buffer)
  (= buffer (call (. buffer resize) 2))
  (= ([] buffer 0) (call GCNode 9))
  (return alias)
)

(type + GCNode
  (prop + value _ (= 0))
  (fn-do + This _
    (param v Int)
    (= value v)
  )
)
```

```output
(8, 0, 7)
```

```brix
(main
  (let parent (call (of Array Int) (list 1 2 3 4)))
  (let grown (call (. ([] parent (..< 0 2)) resize) 3))
  (print (tuple ([] parent 0) ([] parent 2) ([] grown 2)))
)
```

```output
(1, 3, 0)
```

```brix
(import /BxGC/BxGC.h)
(blank)

(main
  (let anchor ([] Byte 1048576))
  (= ([] anchor 0) 7u8)
  (= ([] anchor 1048575) 19u8)
  (call dirty-megabytes)
  (call C:brixGCCollect)
  (call C:brixGCScavenge)
  (print (tuple ([] anchor 0) ([] anchor 1048575)))
  (let fresh ([] Byte 1048576))
  (let sum (as 0 Size))
  (for (in index (..< 0 (. fresh length)))
    (+= sum (as ([] fresh index) Size))
  )
  (print sum)
)

(fn-do + dirty-megabytes _
  (for (in allocation (..< 0 16))
    (let bytes ([] Byte 1048576))
    (for (in index (..< 0 (. bytes length)))
      (= ([] bytes index) 255u8)
    )
  )
)
```

```output
(7, 19)
0
```

```brix crash
(main
  (let values (call (of Array Point) (list (call Point 1 2) (call Point 3 4) (call Point 5 6))))
  (let row ([] (. values rows) 2))
  (-= ([] values 0))
  (print (. values length))
  (print (. row x))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```stderr
Unhandled panic: Stale array view
<no file:7>

Task 1

Stack trace:
  0  __source__:main no file:7:10
  1  __source__:init.__source__ no file
  2  __source__:brix.main no file
```

### An array tested as a condition, alone or beside another, is true where it is not empty.

```brix
(main
  (let empty (. (call (of Array U32) 0) immutable))
  (let full (. (call (of Array U32) 2) immutable))
  (print (call check empty))
  (print (call check full))
  (print (call both empty true))
  (print (call both full true))
  (print (call either empty false))
  (print (call either full false))
  (let mutable (call (of Array U32) 0))
  (print (if-expr mutable "mutable" "none"))
)

(fn-do + check String
  (param params _)
  (return-if "truthy" (if params))
  (return "falsy")
)

(fn-do + both String
  (param params _)
  (param flag Bool)
  (return-if "both" (if (and params flag)))
  (return "not both")
)

(fn-do + either String
  (param params _)
  (param flag Bool)
  (return-if "either" (if (or params flag)))
  (return "neither")
)
```

```output
falsy
truthy
not both
both
neither
either
none
```

### Compound assignment to an item past a pointer reads the item and stores the answer in the item's type.

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (finally
    (inline (call (. arena release)))
  )
  (let counts (call (of (. arena allocate) U32) 4))
  (let bytes (call (of (. arena allocate) U8) 2))
  (let reals (call (of (. arena allocate) F64) 1))
  (= ([] counts 1) 10)
  (+= ([] counts 1) 5)
  (let place 2)
  (= ([] counts (+ place 1)) 3)
  (*= ([] counts (+ place 1)) 4)
  (= ([] bytes 0) 200u8)
  (+= ([] bytes 0) 50u8)
  (= ([] bytes 1) 7u8)
  (*= ([] bytes 1) 2)
  (= ([] reals 0) 1.5)
  (*= ([] reals 0) 3)
  (/= ([] reals 0) 2)
  (print (str ([] counts 1) " " ([] counts 3) " " ([] bytes 0) " " ([] bytes 1) " " ([] reals 0)))
)
```

```output
15 12 250 14 2.25
```

```brix
(main
  (let count 16777216)
  (let values ([] Int count))
  (for (in index (..< 0 count))
    (= ([] values index) (% index 7))
  )
  (let total 0)
  (for (in index (..< 0 count))
    (+= total ([] values index))
  )
  (print total)
)
```

```output
50331645
```

### A value the array's index operator does not take is refused, not stored in the array's own fields

```brix compile-error
(main
  (let codes ([] Char 2))
  (let unit U32 65)
  (= ([] codes 0) unit)
)
```

```output
Exception: Illegal types for operation: Array'Char and U32
<no file:5>
      (= ([] codes 0) unit)
         ^^^^^^^^^^^^
```

### A value converted to the array's item type is stored

```brix
(main
  (let codes ([] Char 2))
  (let unit U32 65)
  (= ([] codes 0) (as unit Char))
  (= ([] codes 1) 'B')
  (print (call (. ([] codes (..< 0 2)) slice-string)))
)
```

```output
AB
```

### An array of a type made with a count that becomes a Size only when asked is refused, not made empty

```brix compile-error
(main
  (let count U64 3)
  (print (. ([] Byte count) length))
)
```

```output
Exception: Expected Size here, but this is U64, which becomes Size only when asked; write "(as count Size)"
<no file:4>
      (print (. ([] Byte count) length))
                         ^^^^^
```

### An index assigned as a value gives what the item then holds, a value of another number converted.

```brix
(main
  (let wide ([] I64 2))
  (let n I32 3)
  (print (= ([] wide 0) n))
  (let halves ([] Float 1))
  (print (= ([] halves 0) 1))
  (let list (list Float) (list 0.5))
  (print (= ([] list 0) 2))
)
```

```output
3
1
2
```

### An array of bytes made of a text holds a copy of its bytes, so writing the array leaves the text as it was.

```brix
(main
  (let text "hello")
  (let bytes (call (of Array Byte) text))
  (= ([] bytes 0) 74)
  (print text)
  (print (call (. bytes slice-string)))
)
```

```output
hello
Jello
```

### A text cast to bytes gives a copy of its bytes, so writing them leaves the text, a literal here, as it was.

```brix
(main
  (let text "hello")
  (let bytes (as text ([] Byte)))
  (= ([] bytes 0) 74)
  (print text)
  (print (call (. bytes slice-string)))
)
```

```output
hello
Jello
```
