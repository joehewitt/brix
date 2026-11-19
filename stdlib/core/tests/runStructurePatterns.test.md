# Structure patterns

JOE-32 ImmutableArray Patterns

### List patterns cover empty, exact slots, wildcard slots, bare rest, and captured rest.

```brix
(main
  (let empty (call (of List Int)))
  (let numbers (list 1 2 3 4))
  (blank)
  (print
    (match empty
      (case (list)
        "empty"
      )
      (default
        "not empty"
      )
    )
  )
  (blank)
  (print
    (match numbers
      (case (list first _ third ...)
        (str first "/" third)
      )
      (default
        "no"
      )
    )
  )
  (blank)
  (let middle
    (match numbers
      (case (list first ...rest last)
        rest
      )
    )
  )
  (blank)
  (print (type-of middle))
  (print middle)
  (blank)
  (print
    (match (list 1 2)
      (case (list _ _ _)
        "bad"
      )
      (default
        "short"
      )
    )
  )
)
```

```output
empty
1/3
[TypeInfo List'I64'GCBacking]
2, 3
short
```

### Array patterns keep concrete slice types for captured remainders.

```brix
(main
  (let values (call (of Array Int) (list 10 20 30 40)))
  (let tail
    (match values
      (case (list head ...rest)
        rest
      )
    )
  )
  (blank)
  (print (type-of tail))
  (print (. tail length))
  (print (call (. ", " join) tail))
  (blank)
  (print
    (match values
      (case (list 10 ... 40)
        "edge"
      )
      (default
        "miss"
      )
    )
  )
  (blank)
  (print
    (match values
      (case (list 10 99 ...)
        "bad"
      )
      (default
        "mismatch"
      )
    )
  )
)
```

```output
[TypeInfo Array'I64]
3
20, 30, 40
edge
mismatch
```

### Concrete user-defined Indexable patterns lower through static length/index/slice operations.

```brix
(main
  (let window (call IntWindow (list 5 6 7 8)))
  (let rest
    (match window
      (case (list first ...middle last)
        middle
      )
    )
  )
  (blank)
  (print (type-of rest))
  (print (. rest length))
  (print (call (. ", " join) rest))
)

(type + IntWindow
  (implements (of Indexable Int))
  (prop _ values (of List Int))
  (fn-do + This _
    (param items (list Int))
    (= values items)
  )
  (fn-expr + length Size :get
    (. values length)
  )
  (fn-expr + (index-get this (param index Index)) Int
    ([] values index)
  )
  (fn-expr + (index-get this (param range (of Range Index))) IntWindow
    (call IntWindow ([] values range))
  )
)
```

```output
[TypeInfo IntWindow]
2
6, 7
```

### Erased Indexable captures return Indexable'T instead of the concrete source type.

```brix
(main
  (let source (of Indexable Int) (call IntWindow (list 5 6 7 8)))
  (let rest
    (match source
      (case (list _ ...tail)
        tail
      )
    )
  )
  (blank)
  (print (type-of rest))
  (print (. rest length))
  (print (call (. ", " join) rest))
)

(type + IntWindow
  (implements (of Indexable Int))
  (prop _ values (of List Int))
  (fn-do + This _
    (param items (list Int))
    (= values items)
  )
  (fn-expr + length Size :get
    (. values length)
  )
  (fn-expr + (index-get this (param index Index)) Int
    ([] values index)
  )
  (fn-expr + (index-get this (param range (of Range Index))) IntWindow
    (call IntWindow ([] values range))
  )
)
```

```output
[TypeInfo Indexable'I64]
3
6, 7, 8
```

### Table-backed Indexable objects use their concrete slice types too.

```brix
(import /core/table)
(blank)

(main
  (let table (call (of Table Point) 3 (:with (call Point (as (+ each 1) I32) (as (+ each 10) I32)))))
  (let rows
    (match table
      (case (list first ...rest)
        rest
      )
    )
  )
  (blank)
  (let xs
    (match (. table x)
      (case (list _ ...tail)
        tail
      )
    )
  )
  (blank)
  (print (type-of rows))
  (print (. rows length))
  (print (as ([] rows 0) Point))
  (print (type-of xs))
  (print (. xs length))
  (print (call (. ", " join) xs))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
[TypeInfo TableSpan'Point]
2
(2, 11)
[TypeInfo TableField'I32]
2
2, 3
```

### String is excluded from bracket sequence matching; text patterns handle strings instead.

```brix
(main
  (print (is "abc" (list "a" ...)))
)
```

```output
false
```

### match?, is, and declaration contexts use the same sequence profile.

```brix
(main
  (let maybe
    (match? (list 1 2 3)
      (case (list 1 ...tail)
        (. tail length)
      )
    )
  )
  (blank)
  (print (type-of maybe))
  (print (?? maybe 0))
  (blank)
  (print (is (list 1 2 3) (list 1 _ 3)))
  (print (is (list 1 2) (list 1 _ 3)))
  (blank)
  (let (list head _ ...tail) (list 10 20 30 40))
  (print head)
  (print tail)
  (blank)
  (if (= (| (list _ second ...rest)) (list 5 6 7 8))
    (print second)
    (print rest)
    (else
      (print "no")
    )
  )
)
```

```output
[TypeInfo Optional'Size]
2
true
false
10
30, 40
6
7, 8
```

### `is` tests the conditions inside a pattern of items or of keys, each reading `it` as the value it tests.

```brix
(main
  (let counts (call (of Map String Int)))
  (= ([] counts "a") 4)
  (blank)
  (print (is (list 1 2) (list (all Int ({} (> it 0))) ...)))
  (print (is (list 1 2) (list (all Int ({} (> it 1))) ...)))
  (print (is counts (map "a" ({} (> it 3)))))
  (print (is-not counts (map "a" (all Int ({} (> it 4))))))
  (blank)
  (for (in group (list (list 1 5) (list 0 2)))
    (print
      (match group
        (case (list first ...)
          (str first " " (is group (list ({} (> it 0)) ({} (> it 0)))) " " ([] it 1))
        )
      )
    )
  )
)
```

```output
true
false
true
true
1 true 5
0 false 2
```

## JOE-32 Map Patterns

### Map patterns match required keys, allow extra entries, and reject missing or mismatched values.

```brix
(main
  (let settings (call (of Map String Int)))
  (= ([] settings "a") 1)
  (= ([] settings "b") 2)
  (= ([] settings "c") 3)
  (blank)
  (print
    (match settings
      (case (map "a" one)
        (str "a=" one)
      )
      (default
        "no"
      )
    )
  )
  (blank)
  (print
    (match settings
      (case (map "missing" value)
        "bad"
      )
      (default
        "missing"
      )
    )
  )
  (blank)
  (print
    (match settings
      (case (map "a" 9)
        "bad"
      )
      (default
        "mismatch"
      )
    )
  )
  (blank)
  (print
    (match settings
      (case (list (= "a" _) ...)
        "open"
      )
      (default
        "closed"
      )
    )
  )
)
```

```output
a=1
missing
mismatch
open
```

### Map remainder capture returns unmatched entries with the same key/value types.

```brix
(main
  (let settings (call (of Map String Int)))
  (= ([] settings "a") 1)
  (= ([] settings "b") 2)
  (= ([] settings "c") 3)
  (blank)
  (let rest
    (match settings
      (case (list (= "a" _) ...tail)
        tail
      )
    )
  )
  (blank)
  (print (type-of rest))
  (print (. rest length))
  (print ([] rest "a"))
  (print ([] rest "b"))
  (print ([] rest "c"))
)
```

```output
[TypeInfo Map'String'I64'GCBacking]
2
unknown
2
3
```

### Map keys support string, numeric, and existing-key equality lookup forms.

```brix
(main
  (let key "b")
  (let settings (call (of Map String Int)))
  (= ([] settings "a") 1)
  (= ([] settings "b") 2)
  (blank)
  (let numeric (call (of Map Int String)))
  (= ([] numeric 7) "seven")
  (blank)
  (print
    (match settings
      (case (map $key two)
        (str "b=" two)
      )
      (default
        "no"
      )
    )
  )
  (blank)
  (print
    (match numeric
      (case (map 7 word)
        word
      )
      (default
        "no"
      )
    )
  )
  (blank)
  (print (is settings (list (= "a" 1) ...)))
  (print (is settings (list (= "a" 9) ...)))
  (blank)
  (if (= (| (list (= "b" two) ...rest)) settings)
    (print two)
    (print ([] rest "a"))
    (else
      (print "no")
    )
  )
)
```

```output
b=2
seven
true
false
2
1
```

### Nested sequence patterns narrow union-valued fields before typed element captures.

```brix
(main
  (let choice (map String Payload))
  (= ([] choice "content") "hello")
  (let root (map String Payload))
  (= ([] root "choices") (list (as choice Payload)))
  (blank)
  (let found
    (match? root
      (case (list (= "choices" (list (: first (map String Payload)) ...)) ...)
        ([] first "content")
      )
    )
  )
  (blank)
  (print (as (!! found) String))
)

(type + Payload (is Union)
  (variant String)
  (variant (list Payload))
  (variant (map String Payload))
)
```

```output
hello
```

### Nested map patterns narrow optional subjects and union-valued fields in streamed constructors.

```brix
(main
  (let delta (map String String))
  (= ([] delta "content") "hello")
  (let choice (map String Payload))
  (= ([] choice "delta") delta)
  (let maybe-choice (? (map String Payload)) choice)
  (blank)
  (for (in event (call events maybe-choice))
    (print (?? (. event content) "missing"))
    (break)
  )
)

(type + Payload (is Union)
  (variant String)
  (variant (map String String))
)

(type + Event
  (prop + content (? String))
  (fn-do + This _
    (param choice (? (map String Payload)))
    (= content (call event-content choice))
  )
)

(fn-expr _ event-content _
  (param choice (? (map String Payload)))
  (match? choice
    (case (list (= "delta" (list (= "content" content) ...)) ...)
      content
    )
  )
)

(fn-do + events (stream Event)
  (param choice (? (map String Payload)))
  (yield (call Event choice))
)
```

```output
hello
```

### Impossible nested patterns reached through streamed constructors fail during compilation.

```brix crash
(main
  (let choice (map String Payload))
  (= ([] choice "delta") "text")
  (blank)
  (for (in event (call invalid-events choice))
    (print (?? (. event content) "missing"))
    (break)
  )
)

(type + Payload (is Union)
  (variant String)
  (variant (map String String))
)

(type + InvalidEvent
  (prop + content (? String))
  (fn-do + This _
    (param choice (map String Payload))
    (= content (call invalid-event-content choice))
  )
)

(fn-expr _ invalid-event-content _
  (param choice (map String Payload))
  (match? choice
    (case (list (= "delta" (tuple first second)) ...)
      (str first " " second)
    )
  )
)

(fn-do + invalid-events (stream InvalidEvent)
  (param choice (map String Payload))
  (yield (call InvalidEvent choice))
)
```

```output
Exception: Pattern has no possible matches
<no file:28>
        (case (list (= "delta" (tuple first second)) ...)
        ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

## JOE-32 Text Patterns

### Text patterns match whole strings with anchored literals and optional captured gaps.

```brix
(main
  (print
    (match "foobar"
      (case (all "foo" ...middle "bar")
        (. middle length)
      )
      (default
        99
      )
    )
  )
  (blank)
  (print
    (match "fooxbar"
      (case (all (: f "foo") ...middle (: b "bar"))
        (str f "|" middle "|" b)
      )
      (default
        "no"
      )
    )
  )
  (blank)
  (print
    (match "foobaz"
      (case (all "foo" ... "bar")
        "bad"
      )
      (default
        "no"
      )
    )
  )
)
```

```output
0
foo|x|bar
no
```

### Text matching is left-to-right and non-greedy while preserving later anchors.

```brix
(main
  (print
    (match "fooafooend"
      (case (all "foo" ...middle "fooend")
        middle
      )
      (default
        "no"
      )
    )
  )
  (blank)
  (print
    (match "foofooend"
      (case (all "foo" ...middle "fooend")
        middle
      )
      (default
        "no"
      )
    )
  )
)
```

```output
a
```

### Text patterns participate in match?, is, and declaration contexts.

```brix
(main
  (let maybe
    (match? "fooxbar"
      (case (all "foo" ...middle "bar")
        middle
      )
    )
  )
  (blank)
  (print (type-of maybe))
  (print (?? maybe "missing"))
  (blank)
  (print (is "fooxbar" (all "foo" ... "bar")))
  (print (is "foobaz" (all "foo" ... "bar")))
  (blank)
  (let (all "foo" ...decl-middle "bar") "foozbar")
  (print decl-middle)
  (blank)
  (if (= (| (all "foo" ...if-middle "bar")) "fooybar")
    (print if-middle)
    (else
      (print "no")
    )
  )
)
```

```output
[TypeInfo Optional'String]
x
true
false
z
y
```

## Wildcard And Exhaustiveness

### _ is a catchall for closed domains and still binds nothing.

```brix
(main
  (let bool-result
    (match true
      (case _
        1
      )
    )
  )
  (blank)
  (let value (? Int))
  (let optional-result
    (match value
      (case _
        2
      )
    )
  )
  (blank)
  (let choice (. Choice no))
  (let union-result
    (match choice
      (case _
        3
      )
    )
  )
  (blank)
  (print bool-result)
  (print optional-result)
  (print union-result)
)

(type + Choice (is Union)
  (variant yes)
  (variant no)
)
```

```output
1
2
3
```

### Structure patterns are conditional and do not make open sequence domains exhaustive.

```brix
(main
  (let fallback
    (match (list 2)
      (case (list 1 ...)
        1
      )
      (default
        0
      )
    )
  )
  (blank)
  (let maybe
    (match? (list 2)
      (case (list 1 ...)
        1
      )
    )
  )
  (blank)
  (print fallback)
  (print (?? maybe 99))
)
```

```output
0
99
```

## Existing Destructurable Objects

### _ covers one destructured slot in tuple-shaped subjects.

```brix
(main
  (let (tuple first _ third) (tuple 1 2 3))
  (print (tuple first third))
  (blank)
  (let pair (call Pair 4 5))
  (print
    (match pair
      (case (tuple left _)
        left
      )
    )
  )
)

(type + Pair (is (tuple (: left Int) (: right Int))))
```

```output
(1, 3)
4
```

### _ works with Table and TableSpan destructuring.

```brix
(import /core/table)
(blank)

(main
  (let table (call (of Table Point) 2 (:with (call Point (as (+ each 1) I32) (as (+ each 10) I32)))))
  (let (tuple xs _) table)
  (let span ([] table (..< 0 2)))
  (blank)
  (print (type-of xs))
  (print (call (. ", " join) xs))
  (blank)
  (print
    (match span
      (case (tuple _ ys)
        (call (. ", " join) ys)
      )
    )
  )
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)
```

```output
[TypeInfo TableField'I32]
1, 2
10, 11
```

### Old collection type annotations still bind the whole subject, not a one-item pattern.

```brix
(main
  (let values (list 1 2))
  (let request (list Int) values)
  (blank)
  (let source (call (of Map String Int)))
  (= ([] source "a") 1)
  (= ([] source "b") 2)
  (let settings (map String Int) source)
  (blank)
  (print (. request length))
  (print (. settings length))
  (print ([] settings "b"))
)
```

```output
2
2
2
```

### Declarations that take a tuple apart and skip fields

```brix
(main
  (let pair (call Pair 4 "four"))
  (let (tuple a _) pair)
  (let (tuple _ b) (tuple 1 "x"))
  (let (tuple _ _ c) (tuple 1 2 3))
  (print a)
  (print b)
  (print c)
  (print (call sum pair))
)

(fn-do + sum Int
  (param pair Pair)
  (let (tuple count _) pair)
  (let (tuple _ name) pair)
  (return (+ count (. name length)))
)

(type + Pair (is (tuple (: count Int) (: name String))))
```

```output
4
x
3
8
```

### Pattern of items against a text never matches

```brix
(main
  (let text "abc")
  (print (is text (list "a" ...)))
  (print (is-not text (list "a" ...)))
  (print (is "abc" (list first second third)))
  (if (is text (list "a" "b" "c"))
    (print "items")
    (else
      (print "no items")
    )
  )
)
```

```output
false
true
false
no items
```

### Items of a list by place, a rest between two ends, and an end that is a value

```brix
(main
  (let numbers (list 1 2 3 4))
  (print
    (match numbers
      (case (list first _ third ...)
        (str first "/" third)
      )
      (default
        "no"
      )
    )
  )
  (let middle
    (match numbers
      (case (list first ...rest last)
        rest
      )
    )
  )
  (print middle)
  (print
    (match numbers
      (case (list)
        "empty"
      )
      (case (list 7 ... 4)
        "edge"
      )
      (default
        "other"
      )
    )
  )
)
```

```output
1/3
2, 3
other
```

### A pattern of items in an if, and in a function that returns from it

```brix
(main
  (if (= (| (list _ second ...rest)) (list 5 6 7 8))
    (print second)
    (print rest)
    (else
      (print "no")
    )
  )
  (if (= (| (list _ second ...rest)) (list 5))
    (print second)
    (else
      (print "no")
    )
  )
  (let made (call show (list 1 2 3)))
  (print made)
  (print (call show (list 9)))
)

(fn-do + show Int
  (param items (list Int))
  (if (= (| (list first ... last)) items)
    (return (+ first last))
  )
  (return -1)
)
```

```output
6
7, 8
no
4
-1
```

### Patterns of items inside patterns of items

```brix
(main
  (let grid (list (list 1 2) (list 3 4) (list 5)))
  (print
    (match grid
      (case (list (list a b) ... (list last))
        (+ a b last)
      )
      (default
        -1
      )
    )
  )
  (print
    (match grid
      (case (list (list a) ...)
        a
      )
      (default
        -1
      )
    )
  )
  (let (list (list x ...) ...others) grid)
  (print x)
  (print (. others length))
)
```

```output
8
-1
1
2
```

### Items that are values of text and of a float

```brix
(main
  (let words (list "a" "b" "c"))
  (print
    (match words
      (case (list "a" second ...)
        second
      )
      (default
        "no"
      )
    )
  )
  (print
    (match words
      (case (list "z" ...)
        "z"
      )
      (case (list ... "c")
        "ends c"
      )
      (default
        "no"
      )
    )
  )
  (let halves (list 0.5 1.5))
  (print
    (match halves
      (case (list 0.5 other)
        other
      )
      (default
        0.0
      )
    )
  )
)
```

```output
b
ends c
1.5
```

### Arms of none, one, two and many items in a function

```brix
(main
  (print (call kind (call (of List Int))))
  (print (call kind (list 7)))
  (print (call kind (list 7 8)))
  (print (call kind (list 1 2 3 4)))
)

(fn-expr + kind String
  (param items (list Int))
  (match items
    (case (list)
      "none"
    )
    (case (list only)
      (str "one " only)
    )
    (case (list first second)
      (str "two " first " " second)
    )
    (case (list first ...middle last)
      (str "many " first " " (. middle length) " " last)
    )
  )
)
```

```output
none
one 7
two 7 8
many 1 2 4
```

### What a pattern of items reads of a class, and in what order

```brix
(main
  (let loud (call Loud (list 1 2 3 4 5)))
  (print
    (match loud
      (case (list first _ ... last)
        (+ first last)
      )
      (default
        0
      )
    )
  )
  (print "next")
  (let kept
    (match loud
      (case (list _ ...rest 5)
        (. rest length)
      )
      (default
        0
      )
    )
  )
  (print kept)
  (print "next")
  (print (is loud (list 1 ...tail)))
  (print "next")
  (print
    (match loud
      (case (list 9 _ ...)
        "nine"
      )
      (case (list 1 2 3)
        "three"
      )
      (default
        "other"
      )
    )
  )
)

(type + Loud
  (implements (of Indexable Int))
  (prop _ values (of List Int))
  (fn-do + This _
    (param items (list Int))
    (= values items)
  )
  (fn-expr + length Size :get
    (before
      (print "length")
    )
    (. values length)
  )
  (fn-expr + (index-get this (param index Index)) Int
    (before
      (print (str "read " index))
    )
    ([] values index)
  )
  (fn-expr + (index-get this (param range (of Range Index))) Loud
    (before
      (print (str "slice " (. range start) " " (. range end)))
    )
    (call Loud ([] values range))
  )
)
```

```output
length
read 0
read 1
read 4
6
next
length
read 0
read 4
slice 1 4
length
3
next
length
read 0
true
next
length
read 0
length
other
```

### Items that are tuples, typed names and tests

```brix
(main
  (let pairs (list (tuple 1 "a") (tuple 2 "b")))
  (print
    (match pairs
      (case (list (tuple n s) ...)
        (str n " " s)
      )
      (default
        "no"
      )
    )
  )
  (let typed
    (match (list 1 2 3)
      (case (list (: first Int) ...rest)
        (+ first (. rest length))
      )
      (default
        0
      )
    )
  )
  (print typed)
  (print
    (match (list 5 6)
      (case (list ({} (> it 4)) ({} (> it 9)))
        "both"
      )
      (case (list ({} (> it 4)) _)
        "first"
      )
      (default
        "none"
      )
    )
  )
)
```

```output
1 a
3
first
```

### Patterns of items under the keys of a map

```brix
(main
  (let settings (map String (list String)))
  (= ([] settings "mode") (list "a" "b"))
  (= ([] settings "other") (list "c"))
  (print
    (match settings
      (case (list (= "mode" (list first ...)) (= "other" (list only)))
        (str first " " only)
      )
      (default
        "no"
      )
    )
  )
  (print
    (match settings
      (case (list (= "mode" (list _)) ...)
        "one"
      )
      (case (list (= "mode" (list _ second)) ...)
        second
      )
      (default
        "no"
      )
    )
  )
  (print (is settings (list (= "other" (list "c")) ...)))
  (print (is settings (list (= "missing" _) ...)))
)
```

```output
a c
b
true
false
```

### A pattern of items in a generic function

```brix
(main
  (print (call total (list 1 2 3)))
  (print (call total (list "a")))
)

(fn-expr + (of total T) Size
  (param items (list T))
  (match items
    (case (list _ ...rest)
      (. rest length)
    )
    (default
      99
    )
  )
)
```

```output
2
0
```

### A match of items over a text takes the arm that is left

```brix
(main
  (let text "abc")
  (print
    (match text
      (case (list first ...)
        "items"
      )
      (default
        "text"
      )
    )
  )
  (print (is text (list _ _ _)))
)
```

```output
text
false
```

### Items compared with text, whole numbers and floats, and a declared first item

```brix
(main
  (let names (list "ann" "bob"))
  (let counts (list 3 4))
  (print
    (match names
      (case (list "ann" "bob")
        "both"
      )
      (default
        "no"
      )
    )
  )
  (print (is names (list "ann" _)))
  (print (is counts (list 3 4)))
  (print (is counts (list 3 5)))
  (let floats (list 1.5 2.5))
  (print (is floats (list 1.5 _)))
  (let (list n ...) counts)
  (print n)
)
```

```output
both
true
true
false
true
3
```

### A pattern of items over an optional list in a function that may give none

```brix
(main
  (let none (? (list Int)))
  (let some (? (list Int)) (list 4 5))
  (print (?? (call first-of none) -1))
  (print (?? (call first-of some) -1))
  (print (?? (call first-of (call (of List Int))) -1))
)

(fn-expr + first-of (? Int)
  (param items (? (list Int)))
  (match? items
    (case (list first ...)
      first
    )
  )
)
```

```output
-1
4
-1
```

### A pattern of items over the list case of a union

```brix
(main
  (let inner (list Doc))
  (+= inner (as "x" Doc))
  (+= inner (as "y" Doc))
  (print (?? (call size (as inner Doc)) 0))
  (print (?? (call size (as "text" Doc)) 0))
)

(fn-expr + size (? Size)
  (param doc Doc)
  (match? doc
    (case (list _ ...rest)
      (+ (. rest length) 1)
    )
  )
)

(type + Doc (is Union)
  (variant String)
  (variant (list Doc))
)
```

```output
2
0
```

### A pattern of items in an if over an optional list

```brix
(main
  (let some (? (list String)) (list "a" "b"))
  (if (= (| (list first second)) some)
    (print (str first " " second))
    (else
      (print "no")
    )
  )
  (let none (? (list String)))
  (if (= (| (list first ...)) none)
    (print first)
    (else
      (print "no")
    )
  )
)
```

```output
a b
no
```

### Keys of a map that are a local, a number and a text, in a match, an is and an if

```brix
(main
  (let key "b")
  (let settings (map String Int))
  (= ([] settings "a") 1)
  (= ([] settings "b") 2)
  (let numeric (map Int String))
  (= ([] numeric 7) "seven")
  (print
    (match settings
      (case (map $key two)
        (str "b=" two)
      )
      (default
        "no"
      )
    )
  )
  (print
    (match numeric
      (case (map 7 word)
        word
      )
      (default
        "no"
      )
    )
  )
  (print (is settings (list (= "a" 1) ...)))
  (print (is settings (list (= "a" 9) ...)))
  (if (= (| (list (= "b" two) (= "a" one))) settings)
    (print (+ two one))
    (else
      (print "no")
    )
  )
  (if (= (| (map "zz" two)) settings)
    (print two)
    (else
      (print "no")
    )
  )
)
```

```output
b=2
seven
true
false
3
no
```

### Patterns of items over the fields of an object, in its methods

```brix
(main
  (let box (call Box (call (of Array String) (list "x" "y" "z")) (list 1 2)))
  (print (call (. box edges)))
  (print (call (. box second)))
)

(type + Box
  (prop + names (of Array String))
  (prop + counts (list Int))
  (fn-do + This _
    (param names _)
    (param counts _)
  )
  (fn-expr + edges String
    (match names
      (case (list first ... last)
        (str first last)
      )
      (default
        "none"
      )
    )
  )
  (fn-expr + second Int
    (match (. this counts)
      (case (list _ second ...)
        second
      )
      (default
        0
      )
    )
  )
)
```

```output
xz
2
```

### Patterns of items over each list of a loop

```brix
(main
  (let rows (list (list Int)) (list (list 1 2) (list 3) (list 4 5 6) (call (of List Int))))
  (for (in row rows)
    (print
      (match row
        (case (list a b)
          (str "pair " (+ a b))
        )
        (case (list a)
          (str "one " a)
        )
        (case (list a ...rest)
          (str "more " a " " (. rest length))
        )
        (default
          "none"
        )
      )
    )
  )
  (let total 0)
  (for (in row rows)
    (if (= (| (list first ... last)) row)
      (+= total (+ first last))
    )
  )
  (print total)
)
```

```output
pair 3
one 3
more 4 2
none
13
```

### Items of a list of optionals

```brix
(main
  (let maybe (list (? Int)) (list 1 null 3))
  (print
    (match maybe
      (case (list first second ...)
        (str (?? first 0) " " (?? second 0))
      )
      (default
        "no"
      )
    )
  )
  (print
    (match maybe
      (case (list 1 ...)
        "one"
      )
      (default
        "other"
      )
    )
  )
)
```

```output
1 0
one
```

### A rest alone, a rest that is named alone, and a rest before one item

```brix
(main
  (let empty (call (of List Int)))
  (let full (list 1 2))
  (print (is empty (list ...)))
  (print (is full (list ...)))
  (print (is empty (list ... _)))
  (print (is full (list ... _)))
  (let whole
    (match full
      (case (list ...all)
        all
      )
    )
  )
  (print whole)
  (let none
    (match empty
      (case (list ...all)
        (. all length)
      )
    )
  )
  (print none)
)
```

```output
true
true
false
true
1, 2
0
```

### A named rest inside a pattern with a named rest

```brix
(main
  (let grid (list (list 1 2 3) (list 4) (list 5 6)))
  (let found
    (match grid
      (case (list (list _ ...inner) ...outer)
        (+ (. inner length) (. outer length))
      )
      (default
        0
      )
    )
  )
  (print found)
)
```

```output
4
```

### Arms that give the rest they name

```brix
(main
  (let picked (call pick (list 1 2 3)))
  (print picked)
  (print (call pick (list 9)))
)

(fn-expr + pick (list Int)
  (param items (list Int))
  (match items
    (case (list _ ...rest _)
      rest
    )
    (case (list ...all)
      all
    )
  )
)
```

```output
2
9
```

### A declared pattern of items that fails after one that holds

```brix crash
(main
  (let (list head _ ...tail) (list 10 20 30 40))
  (print head)
  (print tail)
  (let (list first second) (list 1))
  (print first)
  (print second)
  (print "after")
)
```

```output
10
30, 40
```

### A declared pattern of two items over three leaves by a panic at the pattern

```brix crash
(main
  (let (list first second) (list 1 2 3))
  (print first)
  (print second)
  (print "after")
)
```

```stderr
Unhandled panic: Pattern match failed
<no file:3>

Task 1

Stack trace:
  0  __source__:main no file:3:8
  1  __source__:init.__source__ no file
  2  __source__:brix.main no file
```

### A panic in what a pattern of items reads of a class

```brix crash
(main
  (let bad (call Bad))
  (print
    (match bad
      (case (list first second)
        (+ first second)
      )
      (default
        0
      )
    )
  )
  (print "after")
)

(type + Bad
  (implements (of Indexable Int))
  (fn-expr + length Size :get
    2
  )
  (fn-expr + (index-get this (param index Index)) Int
    ([] (list 10) index)
  )
)
```

```stderr
Unhandled panic: List index 1 out of range
<core/list.bx>

Task 1

Stack trace:
  0  core/list:List:absent core/list.bx
  1  __source__:Bad:[] no file:23:5
  2  __source__:main no file:6:25
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

### A declaration that names a derived class does not narrow: match the value, or write as.

```brix crash
(main
  (let foos (list (call Foo) (call Foo2)))
  (let b Foo2 ([] foos 1))
  (print b)
  (let a Foo2 ([] foos 0))
  (print a)
)

(blank)
(type-line + Foo)
(type-line + Foo2 (is Foo))
```

```output
Exception: Expected Foo2 here, but this is Foo, which is not always Foo2; match it with "(case (: name Foo2) …)", or write "(as ([] foos 1) Foo2)"
<no file:4>
      (let b Foo2 ([] foos 1))
             ^^^^
```

### The trace of a cast to a class the value is not of

```brix crash
(main
  (let foos (list (call Foo) (call Foo2)))
  (let b (as ([] foos 1) Foo2))
  (print b)
  (let a (as ([] foos 0) Foo2))
  (print a)
)

(blank)
(type-line + Foo)
(type-line + Foo2 (is Foo))
```

```stderr
Unhandled panic: Invalid cast to [Object __source__:Foo2]
<no file:6>

Task 1

Stack trace:
  0  __source__:main no file:6:10
  1  __source__:init.__source__ no file
  2  __source__:brix.main no file
```

### The trace of a cast to a class a value of an interface does not hold

```brix crash
(main
  (let things (list Thing) (list (as (call Foo) Thing) (as (call Bar) Thing)))
  (let b (as ([] things 1) Bar))
  (print b)
  (let a (as ([] things 0) Bar))
  (print a)
)

(type + Thing (is Interface))

(type + Foo
  (implements Thing)
)

(type + Bar
  (implements Thing)
)
```

```stderr
Unhandled panic: Invalid cast to [Object __source__:Bar]
<no file:6>

Task 1

Stack trace:
  0  __source__:main no file:6:10
  1  __source__:init.__source__ no file
  2  __source__:brix.main no file
```

### A one-item list pattern whose item is a case pattern matches by shape, not as a collection type.

```brix
(main
  (let blank (call (. Tree branch) 1 (call (of List Tree) (call (. Tree leaf) 2 "blank"))))
  (let sum (call (. Tree branch) 3 (call (of List Tree) (call (. Tree leaf) 4 "+") (call (. Tree leaf) 5 "a"))))
  (let other (call (. Tree branch) 6 (call (of List Tree) (call (. Tree leaf) 7 "other"))))
  (let empty (call (. Tree branch) 8 (call (of List Tree))))
  (blank)
  (print (str (call only blank) " " (call only sum) " " (call only other) " " (call only empty) " " (call only (call (. Tree leaf) 9 "blank"))))
  (print (str (call is-blank blank) " " (call is-blank sum) " " (call is-blank other) " " (call is-blank empty)))
  (print (str (call is-marked blank) " " (call is-marked sum) " " (call is-marked other) " " (call is-marked empty)))
)

(type + Tree (is Union)
  (variant leaf (tuple (: id Int) (: text String)))
  (variant branch (tuple (: id Int) (: items (list Tree))))
)

(fn-expr _ only String
  (param tree Tree)
  (match tree
    (case (call .branch _ (list (call .leaf _ name)))
      name
    )
    (default
      "-"
    )
  )
)

(fn-expr _ is-blank Bool
  (param tree Tree)
  (match tree
    (case (call .branch _ (list (call .leaf _ "blank")))
      true
    )
    (default
      false
    )
  )
)

(fn-expr _ is-marked Bool
  (param tree Tree)
  (match tree
    (case (call .branch _ (list (call .leaf _ ({} (== it "blank")))))
      true
    )
    (default
      false
    )
  )
)
```

```output
blank - other - -
true false false false
true false false false
```

### Arms that repeat one union case keep their order while sharing its tag, list length, and first item.

```brix
(main
  (let imp (call branch 1 (call (of List Tree) (call leaf "import") (call leaf "a"))))
  (let exp (call branch 2 (call (of List Tree) (call leaf "export") (call leaf "a") (call leaf "b") (call leaf "c"))))
  (let pair (call branch 3 (call (of List Tree) (call leaf "let") (call leaf "x") (call leaf "y"))))
  (let triple (call branch 4 (call (of List Tree) (call leaf "let") (call leaf "x") (call leaf "y") (call leaf "z"))))
  (let lone (call branch 5 (call (of List Tree) (call leaf "let"))))
  (let nested (call branch 6 (call (of List Tree) (call branch 0 (call (of List Tree) (call leaf "in"))) (call leaf "tail"))))
  (let empty (call branch 7 (call (of List Tree))))
  (let other (call branch 8 (call (of List Tree) (call leaf "other") (call leaf "a"))))
  (blank)
  (print (str (call kind imp) " | " (call kind exp) " | " (call kind pair) " | " (call kind triple) " | " (call kind lone)))
  (print (str (call kind nested) " | " (call kind empty) " | " (call kind other) " | " (call kind (call leaf "word"))))
  (print (str (call headed imp) " | " (call headed pair) " | " (call headed lone) " | " (call headed nested) " | " (call headed empty)))
  (print (str (call describe imp) " | " (call describe pair) " | " (call describe other) " | " (call describe (call leaf "word"))))
  (print (str (?? (call target-of imp) "none") " | " (?? (call target-of pair) "none") " | " (?? (call target-of empty) "none")))
  (print (str (call tagged empty) " | " (call tagged imp) " | " (call tagged nested) " | " (call tagged (call leaf "word"))))
)

(type + Tree (is Union)
  (variant leaf (tuple (: id Int) (: text String)))
  (variant branch (tuple (: id Int) (: items (list Tree))))
)

(fn-expr _ leaf Tree
  (param text String)
  (call (. Tree leaf) 0 text)
)

(fn-expr _ branch Tree
  (param id Int)
  (param items (list Tree))
  (call (. Tree branch) id items)
)

(fn-expr _ text-of String
  (param tree Tree)
  (match tree
    (case (call .leaf _ text)
      text
    )
    (default
      "?"
    )
  )
)

(fn-expr _ kind String
  (param tree Tree)
  (match tree
    (case (call .leaf _ text)
      (str "leaf " text)
    )
    (case (call .branch _ (list (call .leaf _ "import") path))
      (str "import " (call text-of path))
    )
    (case (call .branch _ (list (call .leaf _ "export") first ...rest))
      (str "export " (call text-of first) "+" (. rest length))
    )
    (case (call .branch _ (list (call .leaf _ "let") name value))
      (str "let " (call text-of name) "=" (call text-of value))
    )
    (case (call .branch _ (list (call .leaf _ "let") ... last))
      (str "let-rest " (call text-of last))
    )
    (case (call .branch _ (list (call .leaf _ ({} (== (. it length) 3))) ...))
      "three"
    )
    (case (call .branch _ (list (call .branch _ inner) ...))
      (str "nested " (. inner length))
    )
    (case (call .branch _ (list))
      "empty"
    )
    (case (call .branch _ _)
      "other"
    )
  )
)

(fn-expr _ headed String
  (param tree Tree)
  (match tree
    (case (call .branch _ (list (call .leaf _ "import") ...))
      "import"
    )
    (case (call .branch _ (list (call .branch _ (list (call .leaf _ name))) (call .leaf _ tail)))
      (str "nested " name " " tail)
    )
    (case (call .branch _ (list (call .leaf _ "let") _ ...))
      "let+"
    )
    (case (call .branch _ (list (call .leaf _ name)))
      (str "only " name)
    )
    (default
      "-"
    )
  )
)

(fn-do _ describe String
  (param tree Tree)
  (match tree
    (case (call .branch _ (list (call .leaf _ "import") path))
      (return (str "import " (call text-of path)))
    )
    (case (call .branch _ (list (call .leaf _ "let") _ value))
      (return (str "let " (call text-of value)))
    )
    (case (call .leaf _ _)
      (return "leaf")
    )
    (default
      (return "?")
    )
  )
)

(fn-expr _ target-of (? String)
  (param tree Tree)
  (match? tree
    (case (call .branch _ (list (call .leaf _ "import") path))
      (call text-of path)
    )
    (case (call .branch _ (list (call .leaf _ "let") _ path))
      (call text-of path)
    )
  )
)

(fn-expr _ tagged String
  (param tree Tree)
  (match tree
    (case (call .branch 7 (list (call .leaf _ name) ...))
      (str "seven " name)
    )
    (case (call .branch id (list (call .leaf _ "import") ...))
      (str "import#" id)
    )
    (case (call .branch id _)
      (str "branch#" id)
    )
    (case (call .leaf id _)
      (str "leaf#" id)
    )
  )
)
```

```output
import a | export a+2 | let x=y | let-rest z | three
nested 1 | empty | other | leaf word
import | let+ | only let | nested in tail | -
import a | let y | ? | leaf
a | y | none
branch#7 | import#1 | branch#6 | leaf#0
```

### A one-entry map pattern can hold a sequence pattern with a remainder.

```brix
(main
  (let root (map String Payload))
  (= ([] root "items") (list (as "a" Payload) (as "b" Payload)))
  (blank)
  (print
    (match root
      (case (map "items" (list (: first String) ...))
        first
      )
      (default
        "none"
      )
    )
  )
)

(type + Payload (is Union)
  (variant String)
  (variant (list Payload))
  (variant (map String Payload))
)
```

```output
a
```

### A one-entry map pattern can hold another one-entry map pattern.

```brix
(main
  (let inner (map String Payload))
  (= ([] inner "id") "x")
  (let root (map String Payload))
  (= ([] root "message") inner)
  (blank)
  (print
    (match root
      (case (map "message" (map "id" (: id String)))
        id
      )
      (default
        "none"
      )
    )
  )
)

(type + Payload (is Union)
  (variant String)
  (variant (list Payload))
  (variant (map String Payload))
)
```

```output
x
```

### A one-item sequence pattern can hold a sequence pattern with a remainder.

```brix
(main
  (let rows (list (list 1 2 3) (list 4 5 6)))
  (blank)
  (print
    (match ([] rows (..< 0 1))
      (case (list (list head ...))
        head
      )
      (default
        0
      )
    )
  )
)
```

```output
1
```

### A one-entry map pattern can hold a typed map capture inside a sequence pattern.

```brix
(main
  (let choice (map String Payload))
  (= ([] choice "content") "hello")
  (let root (map String Payload))
  (= ([] root "choices") (list (as choice Payload)))
  (blank)
  (print
    (match root
      (case (map "choices" (list (: first (map String Payload)) ...))
        (as (!! ([] first "content")) String)
      )
      (default
        "none"
      )
    )
  )
)

(type + Payload (is Union)
  (variant String)
  (variant (list Payload))
  (variant (map String Payload))
)
```

```output
hello
```

### A one-item sequence pattern with a typed capture requires exactly one item.

```brix
(main
  (let one (list 7))
  (let two (list 7 8))
  (blank)
  (print
    (match one
      (case (list (: only Int))
        only
      )
      (default
        0
      )
    )
  )
  (blank)
  (print
    (match two
      (case (list (: only Int))
        only
      )
      (default
        0
      )
    )
  )
)
```

```output
7
0
```

### A one-entry bracket of plain types still names a collection type.

```brix
(main
  (let value Payload (list (as "a" Payload)))
  (let table Payload (call (of Map String Payload)))
  (blank)
  (print
    (match value
      (case (list Payload)
        "list"
      )
      (case (map String Payload)
        "map"
      )
      (default
        "other"
      )
    )
  )
  (blank)
  (print
    (match table
      (case (list Payload)
        "list"
      )
      (case (map String Payload)
        "map"
      )
      (default
        "other"
      )
    )
  )
)

(type + Payload (is Union)
  (variant String)
  (variant (list Payload))
  (variant (map String Payload))
)
```

```output
list
map
```

### A one-entry bracket of a named tuple still names a collection type.

```brix
(main
  (let stack (list (tuple (: uid U32) (: kind U16))) (list (tuple 1 2)))
  (let uid 7)
  (print (. stack length))
  (print (. ([] stack 0) kind))
  (print uid)
  (blank)
  (let source (map String (tuple (: uid U32) (: kind U16))))
  (let table (map String (tuple (: uid U32) (: kind U16))) source)
  (let kind 9)
  (print (. table length))
  (print kind)
)
```

```output
1
2
7
0
9
```

### A named capture typed as a base class covers every union member that derives from it.

```brix
(main
  (let shape Shape (call Square))
  (print
    (match shape
      (case (exact Null)
        "none"
      )
      (case (: base Base)
        (. base label)
      )
    )
  )
)

(type + Base
  (prop + label _ (= "base"))
)

(type + Circle (is Base))

(type + Square (is Base))

(type + Shape (is Union)
  (variant Null)
  (variant Circle)
  (variant Square)
)
```

```output
base
```

### A match over an optional map can use a map pattern with literal keys.

```brix
(main
  (let present (? (map String Payload)) (call (of Map String Payload)))
  (= ([] (!! present) "kind") "path")
  (= ([] (!! present) "value") "/tmp")
  (let absent (? (map String Payload)))
  (blank)
  (print (call describe present))
  (print (call describe absent))
)

(fn-expr + describe _
  (param fields (? (map String Payload)))
  (match fields
    (case (list (= "kind" "path") (= "value" (: location String)))
      (str "path " location)
    )
    (default
      "other"
    )
  )
)

(type + Payload (is Union)
  (variant String)
  (variant (list Payload))
  (variant (map String Payload))
)
```

```output
path /tmp
other
```

### A match over a union can mix literal arms with map and sequence patterns.

```brix
(main
  (let fields (map String Payload))
  (= ([] fields "kind") "url")
  (= ([] fields "value") "https://example.com")
  (blank)
  (print (call describe "stop"))
  (print (call describe fields))
  (print (call describe (list (as "a" Payload) (as "b" Payload))))
  (print (call describe "else"))
)

(fn-expr + describe _
  (param value Payload)
  (match value
    (case "stop"
      "stopped"
    )
    (case (list (= "kind" "url") (= "value" (: address String)))
      (str "url " address)
    )
    (case (list (: first String) ...)
      (str "list " first)
    )
    (default
      "other"
    )
  )
)

(type + Payload (is Union)
  (variant String)
  (variant (list Payload))
  (variant (map String Payload))
)
```

```output
stopped
url https://example.com
list a
other
```
