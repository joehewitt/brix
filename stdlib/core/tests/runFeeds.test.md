# Feeds

```brix
(main
  (print
    (block
      1
      (pipe |> double)
    )
  )
)

(fn-expr + double _
  (param n _)
  (* n 2)
)
```

```output
2
```

```brix
(main
  (print
    (call (. ", " join)
      (block
        (item 1)
        (item 2)
      )
    )
  )
)
```

```output
1, 2
```

```brix
(main
  (print
    (call (. ", " join)
      (block
        (spread (..< 1 5))
      )
    )
  )
)
```

```output
1, 2, 3, 4
```

```brix
(main
  (print
    (call (. ", " join)
      (block
        (item 1)
        (spread (.. 2 4))
        (item 5)
      )
    )
  )
)
```

```output
1, 2, 3, 4, 5
```

```brix
(main
  (print
    (call (. ", " join)
      (block
        (spread (.. 1 10))
        (pipe ?> (% each 2))
      )
    )
  )
)
```

```output
1, 3, 5, 7, 9
```

```brix
(main
  (print
    (call (. ", " join)
      (block
        (spread (.. 1 10))
        (pipe ?> (call is-even each))
      )
    )
  )
)

(fn-expr + is-even _
  (param n _)
  (== (% n 2) 0)
)
```

```output
2, 4, 6, 8, 10
```

```brix
(main
  (print
    (call List
      (block
        (spread (.. 1 5))
        (pipe *> (* each 2))
      )
    )
  )
)
```

```output
2, 4, 6, 8, 10
```

```brix
(main
  (print
    (call List
      (block
        (spread (.. 1 5))
        (pipe ?*> (only-if (* each 2) (if (% each 2))))
      )
    )
  )
)
```

```output
2, 6, 10
```

```brix
(main
  (print
    (call List
      (block
        (spread (.. 1 5))
        (pipe ?*> (call double-even each))
      )
    )
  )
)

(fn-expr + double-even _
  (param n _)
  (only-if (* n 2) (if (% n 2)))
)
```

```output
2, 6, 10
```

```brix
(main
  (print
    (call List
      (block
        (spread (.. 1 5))
        (pipe *> (call triple each))
      )
    )
  )
)

(fn-expr + triple _
  (param item _)
  (* item 3)
)
```

```output
3, 6, 9, 12, 15
```

```brix
(main
  (print
    (call List
      (block
        (spread (.. 1 5))
        (pipe |> (call double (:where (== (% each 2) 1))))
      )
    )
  )
)

(fn-do + (of double Item) _
  (param items (stream Item))
  (param where (of Predicate Item))
  (for (in item items)
    (if (call where item)
      (yield (* item 2))
    )
  )
)
```

```output
2, 6, 10
```

### Optional chaining in feed

```brix
(main
  (let foo (only-if (list 1 2 3) (if (> 5 6))))
  (let bar (only-if (list 4 5 6) (if (> 5 3))))
  (print
    (call (. ", " join)
      (block
        (item 0)
        (spread? foo)
        (spread? bar)
      )
    )
  )
)
```

```output
0, 4, 5, 6
```

```brix
(main
  (print (call foo))
)

(fn-expr + foo _
  (header (call Thing))
  1
  10
  20
)

(type + Thing
  (prop + count _ (= 0))
  (fn-to-expr + String
    (str "Thing: " count)
  )
  (fn-do + with _
    (param next _)
    (+= count next)
    (return this)
  )
)
```

```output
Thing: 31
```

```brix
(main
  1
  (pipe |> foo)
)

(fn-do + foo _
  (param a _)
  (print a)
  (return a)
)
```

```output
1
```

```brix
(main
  (print (call List (call filtered)))
)

(fn-expr + filtered _
  (call candidates)
  (pipe ?*>
    (scope (only-if (tuple name value) (if (== value 1)))
      (lazy name ([] each 0))
      (lazy value ([] each 1))
    )
  )
)

(fn-expr + candidates _
  (item (tuple "missing" 0))
  (item (tuple "present" 1))
)
```

```output
(present, 1)
```

```brix
(main
  (print (call List (call filtered)))
)

(fn-expr + filtered _
  (call candidates)
  (pipe ?*>
    (scope (only-if current (if (and (>= index 1) (!= current "/"))))
      (lazy (tuple current (tuple index part)) each)
    )
  )
)

(fn-expr + candidates _
  (item (tuple "/" (tuple 0 "")))
  (item (tuple "/Users" (tuple 1 "Users")))
  (item (tuple "/Users/joe" (tuple 2 "joe")))
)
```

```output
/Users, /Users/joe
```

```brix
(main
  (print
    (block
      1
      (pipe |=> (* it 5))
    )
  )
)
```

```output
5
```

```brix
(main
  (for (in item (.. 1 3))
    (call action item)
  )
)

(fn-do + action _
  (param item _)
  (print (str "got " item))
)
```

```output
got 1
got 2
got 3
```

```brix
(main
  (let foo (call Foo))
  (blank)
  (.. 1 3)
  (pipe *-> foo)
)

(type + Foo
  (fn-do + (infix this <- (param n _)) _
    (print (str "hit " n))
  )
)
```

```output
hit 1
hit 2
hit 3
```

```brix
(main
  (let foo (call Foo))
  (blank)
  (.. 1 3)
  (pipe -> foo)
)

(type + Foo
  (fn-do + (infix this <- (param n _)) _
    (print (str "hit all " n))
  )
)
```

```output
hit all [Range 1 thru 3 by 1]
```

```brix
(main
  (print
    (call List
      (block
        (spread (.. 1 3))
        (pipe *> 7)
      )
    )
  )
  (print
    (call List
      (block
        (spread (.. 1 3))
        (pipe ?> true)
        (pipe ?*> (only-if 9 (if true)))
      )
    )
  )
)
```

```output
7, 7, 7
9, 9, 9
```

```brix
(main
  (let functions
    (block
      (spread (.. 1 2))
      (pipe *> double)
    )
  )
  (for (in fn functions)
    (print (call fn 3))
  )
  (let partials
    (block
      (spread (.. 1 2))
      (pipe *> (call add each ...))
    )
  )
  (for (in fn partials)
    (print (call fn 10))
  )
)

(fn-expr + double Int
  (param value Int)
  (* value 2)
)

(fn-expr + add Int
  (param left Int)
  (param right Int)
  (+ left right)
)
```

```output
6
6
11
12
```

```brix
(main
  (let values
    (block
      (spread (.. 1 3))
      (pipe *> (call make))
    )
  )
  (let empty
    (block
      (spread (..< 1 1))
      (pipe *> (call make))
    )
  )
  (let unused
    (block
      (spread (.. 1 3))
      (pipe *> (call make))
    )
  )
  (print (str "created " count))
  (let pulled 0)
  (for (in fn values)
    (print (call fn 3))
    (+= pulled 1)
    (if (== pulled 2)
      (break)
    )
  )
  (for (in fn empty)
    (print (call fn 3))
  )
  (print (str "finished " count))
)

(blank)
(prop _ count _ (= 0))

(type + Unary (is (fn-type Int (param value Int))))

(fn-do + make Unary
  (+= count 1)
  (print (str "factory " count))
  (return double)
)

(fn-expr + double Int
  (param value Int)
  (* value 2)
)
```

```output
created 0
factory 1
6
factory 2
6
finished 2
```

```brix
(main
  (let rows
    (block
      (spread (.. 1 2))
      (pipe *>
        (block
          (call List
            (block
              (spread (.. 1 2))
              (pipe *> (+ (* each 10) outer))
            )
          )
          (lazy outer each)
        )
      )
    )
  )
  (for (in row rows)
    (print row)
  )
)
```

```output
11, 21
12, 22
```

```brix
(main
  (let cells (list Int)
    (block
      (..< 0 3)
      (pipe *>
        (block
          (item low)
          (item high)
          (lazy (tuple low high) (tuple each (* each 10)))
        )
      )
      (pipe |> flat-map)
    )
  )
  (print cells)
)
```

```output
0, 0, 1, 10, 2, 20
```

```brix
(main
  (for (in value (.. 1 2))
    (print (call cached value))
  )
)

(fn-expr + cached Int @inline
  (param value Int)
  (call read (=> (value) result))
  (where
    (lazy result value)
  )
)

(type + Getter (is (fn-type Int (param ignored Int))))

(fn-expr + read Int @inline
  (param fn Getter)
  (call fn 0)
)
```

```output
1
2
```

```brix
(main
  (let context (call MatchContext))
  (let value
    (match "7"
      (~ (: text (+ (chars "0-9")))
        (call (. context capture) text location)
      )
      (throw (call Panic "integer did not match"))
    )
  )
  (print value)
)

(type + MatchContext
  (fn-expr + capture String
    (param text String)
    (param location MatchLocation)
    (str "integer " text)
  )
)
```

```output
integer 7
```

```brix
(main
  (let values
    (block
      (spread (.. 1 4))
      (pipe *|> (call record each))
      (pipe *> (* each 10))
      (pipe ?> (>= each 20))
    )
  )
  (print "created")
  (for (in value values)
    (print (str "out " value))
    (if (== value 30)
      (break)
    )
  )
  (print "done")
)

(fn-do + record _
  (param value Int)
  (print (str "tap " value))
)
```

```output
created
tap 1
tap 2
out 20
tap 3
out 30
done
```

```brix
(main
  (let unused
    (block
      (spread (.. 1 2))
      (pipe *|> (call make))
    )
  )
  (spread (.. 1 2))
  (pipe *|> (call make))
  (let empty
    (block
      (spread (..< 1 1))
      (pipe *|> (call make))
    )
  )
  (for (in item empty)
    (print item)
  )
  (print "created")
  (let values
    (block
      (spread (.. 1 2))
      (pipe *|> (call make))
      (pipe *|> action)
    )
  )
  (for (in item values)
    (print (str "out " item))
  )
)

(fn-do + make _
  (print "factory")
  (return action)
)

(fn-do + action _
  (param value Int)
  (print (str "called " value))
)
```

```output
created
factory
out 1
factory
out 2
```

```brix
(main
  (let values (call tap-escaping 10))
  (print "created")
  (for (in value values)
    (print (str "out " value))
    (if (== value 2)
      (break)
    )
  )
)

(fn-expr + tap-escaping _
  (param offset Int)
  (spread (.. 1 3))
  (pipe *|> (call record (+ each offset)))
)

(fn-do + record _
  (param value Int)
  (print (str "tap " value))
)
```

```output
created
tap 11
out 1
tap 12
out 2
```

```brix
(import /BxGC/BxGC.h)
(blank)

(main
  (let first (call TapBox "one"))
  (let second (call TapBox "two"))
  (let values
    (block
      (item first)
      (item second)
      (pipe *|> (call update each))
    )
  )
  (call C:brixGCCollect)
  (for (in item values)
    (call C:brixGCCollect)
    (print (. item text))
  )
  (print (. first text))
  (print (. second text))
)

(type + TapBox
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(fn-do + update _
  (param item TapBox)
  (= (. item text) (+ (. item text) "!"))
)
```

```output
one!
two!
one!
two!
```

```brix
(main
  (let values (list Int)
    (block
      (spread (.. 1 3))
      (pipe *|> (call record each))
      (pipe *|> 99)
    )
  )
  (print values)
)

(fn-do + record _
  (param value Int)
  (print (str "tap " value))
)
```

```output
tap 1
tap 2
tap 3
1, 2, 3
```

```brix
(main
  (let result (catch (call tap-failure)))
  (print (is result TapFailure))
)

(type + TapFailure (is Error))

(fn-do + tap-failure Int (?! TapFailure)
  (let values
    (block
      (spread (call tap-source))
      (pipe *|> (call tap-action each))
    )
  )
  (for (in value values)
    (print (str "out " value))
  )
  (return 0)
)

(fn-do + tap-source _ @inline
  (finally
    (print "closed")
  )
  (yield 1)
  (yield 2)
  (yield 3)
)

(fn-do + tap-action _ (?! TapFailure)
  (param value Int)
  (print (str "tap " value))
  (if (== value 2)
    (throw (call TapFailure "failed"))
  )
)
```

```output
tap 1
out 1
tap 2
closed
true
```

```brix
(main
  (print (call tap-partial))
)

(fn-do + tap-partial Int
  (let factor 2)
  (let values
    (block
      (spread (call tap-source))
      (pipe *|> (call record (* each factor)))
    )
  )
  (= factor 3)
  (print "created")
  (for (in value values)
    (print (str "out " value))
    (if (== value 2)
      (return 17)
    )
  )
  (return 0)
)

(fn-do + tap-source _ @inline
  (finally
    (print "closed")
  )
  (yield 1)
  (yield 2)
  (yield 3)
)

(fn-do + record _
  (param value Int)
  (print (str "tap " value))
)
```

```output
created
tap 3
out 1
tap 6
out 2
closed
17
```

```brix
(main
  (let rows
    (block
      (spread (.. 1 2))
      (pipe *>
        (block
          (call List
            (block
              (spread (.. 1 2))
              (pipe *|> (call record (+ (* outer 10) each)))
            )
          )
          (lazy outer each)
        )
      )
    )
  )
  (for (in row rows)
    (print row)
  )
)

(fn-do + record _
  (param value Int)
  (print (str "tap " value))
)
```

```output
tap 11
tap 12
1, 2
tap 21
tap 22
1, 2
```

```brix
(main
  (let result (catch (call invoke fail)))
  (print (is result CallbackFailure))
)

(type + CallbackFailure (is Error))

(fn-do + (of invoke Output) _
  (param callback (of Mapper Int Output))
  (call callback 1)
)

(fn-do + fail _ (?! CallbackFailure)
  (param value Int)
  (throw (call CallbackFailure "failed"))
)
```

```output
true
```

```brix
(main
  (print
    (call List
      (block
        (spread (.. 1 1))
        (pipe *> (call nested-values each))
        (pipe |> flatten-values)
        (pipe |> List)
      )
    )
  )
)

(fn-expr + nested-values _ @inline
  (param value Int)
  (item? (only-if 0 (if (== value 0))))
  (spread (.. value value))
)

(fn-do + flatten-values (stream Int)
  (param streams (stream (stream Int)))
  (for (in stream streams)
    (for (in value stream)
      (yield value)
    )
  )
)
```

```output
1
```

```brix
(main
  (for (in item (call named-items))
    (print (. item name))
  )
)

(type + Named (is Interface)
  (fn-expr + name String :get)
)

(type + First (is Tuple)
  (implements Named)
  (prop + value Int)
  (fn-expr + name _ :get
    (str "first " value)
  )
)

(type + Second (is Tuple)
  (implements Named)
  (prop + label String)
  (fn-expr + name _ :get
    (str "second " label)
  )
)

(fn-expr + named-items (list Named)
  (call (of List Named)
    (block
      (item (call First 1))
      (item (call Second "b"))
    )
  )
)
```

```output
first 1
second b
```

```brix
(main
  (for (in item (call named-items))
    (print (. item name))
  )
)

(type + Named (is Interface)
  (fn-expr + name String :get)
)

(type + First (is Tuple)
  (implements Named)
  (prop + value Int)
  (fn-expr + name _ :get
    (str "first " value)
  )
)

(type + Second (is Tuple)
  (implements Named)
  (prop + label String)
  (fn-expr + name _ :get
    (str "second " label)
  )
)

(fn-expr + named-items (list Named)
  (item (call First 1))
  (item (call Second "b"))
)
```

```output
first 1
second b
```

```brix
(main
  (for (in item (call named-items))
    (print (. item name))
  )
)

(type + Named (is Interface)
  (fn-expr + name String :get)
)

(type + First (is Tuple)
  (implements Named)
  (prop + value Int)
  (fn-expr + name _ :get
    (str "first " value)
  )
)

(type + Second (is Tuple)
  (implements Named)
  (prop + label String)
  (fn-expr + name _ :get
    (str "second " label)
  )
)

(fn-expr + named-items (list Named)
  (call (of List Named)
    (block
      (item (as (call First 1) Named))
      (item (as (call Second "b") Named))
    )
  )
)
```

```output
first 1
second b
```

### A list of feeds kept in a local and read by a loop, one of a single feed, one read in a function, and one a function returns, read by `<-`.

```brix
(main
  (let source
    (block
      (item 1)
      (item 2)
      (item 3)
    )
  )
  (for (in item source)
    (print item)
  )
  (let one
    (block
      (item "a")
    )
  )
  (for (in item one)
    (print item)
  )
  (print (call total))
  (print (<- (call pair)))
  (let p (call pair))
  (print (<- p))
  (print (<- p))
  (print (<- p))
)

(fn-do + total _
  (let sum 0)
  (let items
    (block
      (item 10)
      (item 20)
    )
  )
  (for (in item items)
    (+= sum item)
  )
  (return sum)
)

(fn-do + pair (stream String)
  (return
    (block
      (item "x")
      (item "y")
    )
  )
)
```

```output
1
2
3
a
30
x
x
y
unknown
```

### A list of feeds a method returns, which reads the receiver and an argument, and one that reads a local as it is when the items are read.

```brix
(main
  (let box (call Box 3))
  (for (in item (call (. box items) 10))
    (print item)
  )
  (for (in item (call scaled 2))
    (print item)
  )
  (let k 5)
  (let late
    (block
      (item k)
      (item (+ k 1))
    )
  )
  (= k 50)
  (for (in item late)
    (print item)
  )
)

(fn-do + scaled (stream Int)
  (param by-much Int)
  (let base 100)
  (return
    (block
      (item base)
      (item (* base by-much))
      (spread (.. 1 by-much))
    )
  )
)

(type + Box
  (prop + n Int)
  (fn-do + This _
    (param n Int)
    (= (. this n) n)
  )
  (fn-do + items (stream Int)
    (param extra Int)
    (return
      (block
        (item n)
        (item (+ n extra))
        (spread (.. 1 n))
      )
    )
  )
)
```

```output
3
13
1
2
3
100
200
1
2
50
51
```

### A list of feeds given to a function as a block: joined, with a range among the feeds, and made a list.

```brix
(main
  (print
    (call (. ", " join)
      (block
        (item 1)
        (item 2)
      )
    )
  )
  (print
    (call (. ", " join)
      (block
        (item 1)
        (spread (.. 2 4))
        (item 5)
      )
    )
  )
  (print
    (call List
      (block
        (item 1)
        (item 2)
      )
    )
  )
  (let list
    (call List
      (block
        (spread (.. 1 3))
        (item 9)
      )
    )
  )
  (print (tuple (. list length) ([] list 3)))
)
```

```output
1, 2
1, 2, 3, 4, 5
1, 2
(4, 9)
```

### Spreads of a range: up, down, empty, with ends that are arguments, two in a row, and of bytes.

```brix
(main
  (let source
    (block
      (item 1)
      (spread (.. 2 4))
      (item 5)
    )
  )
  (for (in item source)
    (print item)
  )
  (let down
    (block
      (spread (.. 3 1))
    )
  )
  (for (in item down)
    (print item)
  )
  (let none
    (block
      (spread (..< 1 1))
      (spread (..< 5 5))
    )
  )
  (for (in item none)
    (print item)
  )
  (print (call (. ", " join) (call spans 2 5)))
  (print (call (. ", " join) (call spans 5 2)))
  (let twice
    (block
      (spread (.. 1 2))
      (spread (.. 1 2))
    )
  )
  (for (in item twice)
    (print item)
  )
  (let bytes
    (block
      (spread (.. 250u8 255u8))
    )
  )
  (let sum 0)
  (for (in b bytes)
    (+= sum b)
  )
  (print sum)
)

(fn-do + spans (stream Int)
  (param low Int)
  (param high Int)
  (return
    (block
      (spread (..< low high))
      (item 0)
      (spread (.. low high))
    )
  )
)
```

```output
1
2
3
4
5
3
2
1
2, 3, 4, 0, 2, 3, 4, 5
5, 4, 3, 0, 5, 4, 3, 2
1
2
1
2
1515
```

### Feeds that give what an optional holds, `*?`: where it holds a value, where it holds none, and alone in a list that yields nothing.

```brix
(main
  (let source
    (block
      (item 0)
      (item? (call maybe 1))
      (item? (call maybe -1))
      (item? (call maybe 2))
    )
  )
  (for (in item source)
    (print item)
  )
  (let only
    (block
      (item? (call maybe -5))
    )
  )
  (for (in item only)
    (print item)
  )
  (print "none")
  (let names
    (block
      (item? (call named 1))
      (item "b")
      (item? (call named 0))
    )
  )
  (for (in name names)
    (print name)
  )
)

(fn-do + maybe (? Int)
  (param n Int)
  (if (> n 0)
    (return (* n 10))
  )
  (return null)
)

(fn-do + named (? String)
  (param n Int)
  (if (> n 0)
    (return "a")
  )
  (return null)
)
```

```output
0
10
20
none
a
b
```

### Feeds of several types yield the type they unify to, and feeds written where a list of a type is wanted are converted to it.

```brix
(main
  (let floats
    (block
      (item 1)
      (item 2.5)
    )
  )
  (for (in f floats)
    (print f)
  )
  (let spread
    (block
      (spread (.. 1 2))
      (item 2.5)
    )
  )
  (for (in f spread)
    (print f)
  )
  (let later
    (block
      (item 0.5)
      (item 1)
      (spread (.. 2 3))
    )
  )
  (for (in f later)
    (print f)
  )
  (let wide (of List Float)
    (call (of List Float)
      (block
        (item 1)
        (item 2)
      )
    )
  )
  (print (tuple (. wide length) (+ ([] wide 0) 0.5)))
  (let texts (of List String)
    (call (of List String)
      (block
        (item "a")
        (item "b")
      )
    )
  )
  (print (tuple (. texts length) ([] texts 1)))
  (print (call typed))
)

(fn-do + typed (list Float)
  (return
    (block
      (item 1)
      (item 2)
    )
  )
)
```

```output
1
2.5
1
2
2.5
0.5
1
2
3
(2, 1.5)
(2, b)
1, 2
```

### A list written after `**` where a list of a type is wanted is a list of that type.

```brix
(main
  (print (call (. ", " join) (call spread)))
)

(fn-expr + spread (list Float)
  (spread (list 1 2))
  (spread (list 3))
)
```

```output
1, 2, 3
```

### A list of feeds left by `break` has finished, one never read runs nothing, and one read by `<-` past its end gives nothing.

```brix
(main
  (let source
    (block
      (item 1)
      (item 2)
      (item 3)
    )
  )
  (for (in item source)
    (if (== item 2)
      (break)
    )
    (print item)
  )
  (for (in item source)
    (print item)
  )
  (let unused
    (block
      (item 1)
      (spread (.. 1 3))
    )
  )
  (let gen
    (block
      (spread (.. 1 2))
    )
  )
  (print (<- gen))
  (print (<- gen))
  (print (<- gen))
  (print (call count-of source))
  (let fresh
    (block
      (item 4)
      (item 5)
    )
  )
  (print (call count-of fresh))
)

(fn-do + count-of Int
  (param items (stream Int))
  (let n 0)
  (for (in item items)
    (+= n 1)
  )
  (return n)
)
```

```output
1
1
2
unknown
0
2
```

### A list of feeds piped through a function over each item: mapped, filtered, mapped where a condition holds, and through several pipes in a row.

```brix
(main
  (let doubled
    (block
      (spread (.. 1 3))
      (pipe *> (* each 2))
    )
  )
  (for (in item doubled)
    (print item)
  )
  (let big
    (block
      (spread (.. 1 6))
      (pipe ?> (> each 3))
    )
  )
  (for (in item big)
    (print item)
  )
  (let odd
    (block
      (spread (.. 1 6))
      (pipe ?*> (only-if (* each 10) (if (== (% each 2) 1))))
    )
  )
  (for (in item odd)
    (print item)
  )
  (let chained
    (block
      (spread (.. 1 6))
      (pipe ?> (> each 2))
      (pipe *> (* each 100))
      (pipe ?> (< each 600))
    )
  )
  (for (in item chained)
    (print item)
  )
)
```

```output
2
4
6
4
5
6
10
30
50
300
400
500
```

### A pipe whose function reads a local as it is when the items are read, a tap, a filter in a function that reads an argument, a value piped whole as `it`, and a pipe never read.

```brix
(main
  (let k 10)
  (let scaled
    (block
      (spread (.. 1 3))
      (pipe *> (* each k))
    )
  )
  (= k 100)
  (for (in item scaled)
    (print item)
  )
  (let taps
    (block
      (spread (.. 1 3))
      (pipe *|> (call bump each))
    )
  )
  (for (in item taps)
    (print item)
  )
  (print count)
  (print (call pick 3))
  (print
    (block
      5
      (pipe |=> (* it 5))
    )
  )
  (let unused
    (block
      (spread (.. 1 3))
      (pipe *|> (call bump each))
    )
  )
  (print count)
)

(prop _ count _ (= 0))

(fn-do + bump _
  (param n Int)
  (+= count n)
)

(fn-do + pick _
  (param limit Int)
  (let kept 0)
  (let source
    (block
      (spread (.. 1 10))
      (pipe ?> (<= each limit))
    )
  )
  (for (in item source)
    (+= kept item)
  )
  (return kept)
)
```

```output
100
200
300
1
2
3
6
6
25
6
```

### Pipes a method returns, whose functions read the receiver and an argument, and pipes that map to a float, a truth and a pair.

```brix
(main
  (let box (call Box 3))
  (for (in item (call (. box scaled)))
    (print item)
  )
  (for (in item (call (. box above) 1))
    (print item)
  )
  (print (call (. ", " join) (call evens 6)))
  (let halves
    (block
      (item 1)
      (item 2)
      (item 3)
      (pipe *> (/ each 2.0))
    )
  )
  (for (in h halves)
    (print h)
  )
  (let flags
    (block
      (spread (.. 1 3))
      (pipe *> (> each 1))
    )
  )
  (for (in f flags)
    (print f)
  )
  (let pairs
    (block
      (spread (.. 1 2))
      (pipe *> (tuple each (* each each)))
    )
  )
  (for (in (tuple a b) pairs)
    (print (+ a b))
  )
)

(fn-do + evens (stream Int)
  (param limit Int)
  (return
    (block
      (spread (.. 1 limit))
      (pipe ?> (== (% each 2) 0))
    )
  )
)

(type + Box
  (prop + n Int)
  (fn-do + This _
    (param n Int)
    (= (. this n) n)
  )
  (fn-do + scaled (stream Int)
    (return
      (block
        (spread (.. 1 n))
        (pipe *> (* each n))
      )
    )
  )
  (fn-do + above (stream Int)
    (param low Int)
    (return
      (block
        (spread (.. 1 n))
        (pipe ?> (> each low))
        (pipe *> (+ each n))
      )
    )
  )
)
```

```output
3
6
9
5
6
2, 4, 6
0.5
1
1.5
false
true
true
2
6
```

### A pipe made a list, given to a function as a block, left by `break`, read by `<-` past its end, joined, and over a list that yields nothing.

```brix
(main
  (let list
    (call List
      (block
        (spread (.. 1 4))
        (pipe *> (* each each))
      )
    )
  )
  (print (tuple (. list length) ([] list 3)))
  (print
    (call total-of
      (block
        (spread (.. 1 4))
        (pipe ?> (!= each 2))
      )
    )
  )
  (let early
    (block
      (spread (.. 1 100))
      (pipe *> (* each 3))
    )
  )
  (for (in item early)
    (if (> item 9)
      (break)
    )
    (print item)
  )
  (let nested
    (block
      (spread (.. 1 2))
      (pipe *> (* each 10))
      (pipe *> (+ each 1))
      (pipe *> (* each 2))
    )
  )
  (print (<- nested))
  (print (<- nested))
  (print (<- nested))
  (let picked
    (block
      (spread (.. 1 6))
      (pipe ?*> (only-if (* each 10) (if (== (% each 3) 0))))
    )
  )
  (print (call (. ", " join) picked))
  (let none
    (block
      (spread (..< 1 1))
      (pipe *> (* each 2))
    )
  )
  (for (in item none)
    (print item)
  )
  (print "end")
)

(fn-do + total-of Int
  (param items (stream Int))
  (let sum 0)
  (for (in item items)
    (+= sum item)
  )
  (return sum)
)
```

```output
(4, 16)
8
3
6
9
22
42
unknown
30, 60
end
```

### A pipe piped forward into a function, a tap whose function gives a value that is dropped, and a tap that runs as each item is read.

```brix
(main
  (let more
    (block
      (spread (.. 1 4))
      (pipe *> (* each 2))
      (pipe |> total-of)
    )
  )
  (print more)
  (let discarded
    (block
      (spread (.. 1 3))
      (pipe *|> (* each 2))
    )
  )
  (for (in item discarded)
    (print item)
  )
  (let counted
    (block
      (spread (.. 1 3))
      (pipe *|> (call touch each))
    )
  )
  (print touched)
  (for (in item counted)
    (print touched)
  )
)

(prop _ touched _ (= 0))

(fn-do + touch _
  (param n Int)
  (+= touched 1)
)

(fn-do + total-of Int
  (param items (stream Int))
  (let sum 0)
  (for (in item items)
    (+= sum item)
  )
  (return sum)
)
```

```output
20
1
2
3
0
1
2
3
```

### Feeds of texts piped through a function that reads a property of each.

```brix
(main
  (let words
    (block
      (item "a")
      (item "bb")
      (pipe *> (. each length))
    )
  )
  (for (in n words)
    (print n)
  )
  (let kept
    (block
      (item "a")
      (item "bb")
      (item "ccc")
      (pipe ?> (> (. each length) 1))
    )
  )
  (for (in w kept)
    (print w)
  )
)
```

```output
1
2
bb
ccc
```

### A list of several feeds piped forward into a function, alone and after other pipes.

```brix
(main
  (let mixed
    (block
      (item 1)
      (spread (.. 2 4))
      (pipe ?> (!= each 3))
      (pipe *> (* each 10))
      (pipe |> total-of)
    )
  )
  (print mixed)
  (let three
    (block
      (item 1)
      (item 2)
      (item 3)
      (pipe |> total-of)
    )
  )
  (print three)
)

(fn-do + total-of Int
  (param items (stream Int))
  (let sum 0)
  (for (in item items)
    (+= sum item)
  )
  (return sum)
)
```

```output
70
6
```

### A pipe and a list of feeds written in a turn of a loop read the item of that turn.

```brix
(main
  (for (in round (.. 1 2))
    (let scaled
      (block
        (spread (.. 1 2))
        (pipe *> (* each round))
      )
    )
    (for (in item scaled)
      (print item)
    )
    (let listed
      (block
        (item round)
        (spread (.. 1 round))
      )
    )
    (for (in item listed)
      (print item)
    )
  )
)
```

```output
1
2
1
1
2
4
2
1
2
```

### A pipe written on one line with what it reads: over a list, over a range kept in a local, over a range in parentheses, and a filter over a list.

```brix
(main
  (call run)
)

(fn-do + run _
  (let list (list 1 2 3))
  (let mapped (*> list (* each 2)))
  (for (in item mapped)
    (print item)
  )
  (let range (.. 1 3))
  (let scaled (*> range (* each 10)))
  (for (in item scaled)
    (print item)
  )
  (let direct (*> (.. 4 6) (+ each 1)))
  (for (in item direct)
    (print item)
  )
  (let kept (?> (list 1 2 3 4) (== (% each 2) 0)))
  (for (in item kept)
    (print item)
  )
)
```

```output
2
4
6
10
20
30
5
6
7
2
4
```

### Pipes on one line kept in locals and piped again, a filter and a map in a row, and a pipe joined whose function makes a text.

```brix
(main
  (call run)
)

(fn-do + run _
  (let list (list 1 2 3 4 5 6))
  (let evens (?> list (== (% each 2) 0)))
  (let doubled (*> evens (* each 2)))
  (for (in item doubled)
    (print item)
  )
  (let chained (*> (?> list (> each 2)) (* each 10)))
  (for (in item chained)
    (print item)
  )
  (print (call (. ", " join) (*> list (str "<" each ">"))))
)
```

```output
4
8
12
30
40
50
60
<1>, <2>, <3>, <4>, <5>, <6>
```

### A pipe that maps where a value is given, and pipes over what a generator function gives: mapped to floats, filtered from a local, and tapped.

```brix
(main
  (call run)
)

(fn-do + run _
  (let halves (?*> (list 1 2 3 4) (call half each)))
  (for (in item halves)
    (print item)
  )
  (let more (*> (call counted 3) (+ each 0.5)))
  (for (in item more)
    (print item)
  )
  (let source (call counted 4))
  (let kept (?> source (> each 2)))
  (for (in item kept)
    (print item)
  )
  (let seen (*|> (call counted 2) (call show each)))
  (for (in item seen)
    (print (* item 2))
  )
)

(fn-do + half (? Int)
  (param n Int)
  (if (== (% n 2) 0)
    (return (/ n 2))
  )
  (return null)
)

(fn-do + show _
  (param n Int)
  (print (str "saw " n))
)

(fn-do + counted (stream Int)
  (param n Int)
  (for (in i (.. 1 n))
    (yield i)
  )
)
```

```output
1
2
1.5
2.5
3.5
3
4
saw 1
2
saw 2
4
```

### A value piped whole to an expression that reads it as `it`: a number, a text and a list.

```brix
(main
  (call run)
)

(fn-do + run _
  (let total (|=> 5 (+ (* it 2) 1)))
  (print total)
  (let text (|=> "abc" (str it it)))
  (print text)
  (let list (list 3 1 2))
  (let size (|=> list (. it length)))
  (print size)
)
```

```output
11
abcabc
3
```

### A pipe written in a turn of a loop that reads the loop's item, a pipe over texts, and a pipe read by the header of a loop.

```brix
(main
  (call run)
)

(fn-do + run _
  (for (in round (.. 1 2))
    (let scaled (*> (.. 1 3) (* each round)))
    (for (in item scaled)
      (print item)
    )
  )
  (let words (list "a" "bb" "ccc"))
  (let sizes (*> words (. each length)))
  (for (in size sizes)
    (print size)
  )
  (let total 0)
  (for (in n (*> words (. each length)))
    (+= total n)
  )
  (print total)
)
```

```output
1
2
3
2
4
6
1
2
3
6
```

### A comprehension piped through a function, that pipe piped forward into a function, and a comprehension over a range filtered.

```brix
(main
  (call run)
)

(fn-do + run _
  (let list (list 1 2 3))
  (let doubled (in (* each 2) list))
  (let more (*> doubled (+ each 1)))
  (let text (|> more (. ", " join)))
  (print text)
  (let picked (?> (in each (.. 1 6)) (== (% each 3) 0)))
  (for (in item picked)
    (print item)
  )
)
```

```output
3, 5, 7
3
6
```

### A panic under a list of feeds read by a loop of the same function.

```brix crash
(main
  (call run)
)

(fn-do + run _
  (let source
    (block
      (item 1)
      (item (call check 2))
      (item 3)
    )
  )
  (for (in item source)
    (print item)
  )
)

(fn-do + check Int
  (param n Int)
  (if (== n 2)
    (throw (call Panic "no"))
  )
  (return n)
)
```

```output
1
```

### A panic under a list of feeds read through `join`.

```brix crash
(main
  (call run)
)

(fn-do + run _
  (let source
    (block
      (item 1)
      (item (call check 2))
      (item 3)
    )
  )
  (print (call (. ", " join) source))
)

(fn-do + check Int
  (param n Int)
  (if (== n 2)
    (throw (call Panic "no"))
  )
  (return n)
)
```

### A panic under a list of feeds a function returns, read by a loop.

```brix crash
(main
  (call run)
)

(fn-do + run _
  (let source (call numbers))
  (for (in item source)
    (print item)
  )
)

(fn-do + numbers (stream Int)
  (return
    (block
      (item 1)
      (item (call check 2))
      (item 3)
    )
  )
)

(fn-do + check Int
  (param n Int)
  (if (== n 2)
    (throw (call Panic "no"))
  )
  (return n)
)
```

```output
1
```

### A panic in the function of a pipe over a list of feeds, read by a loop of the same function.

```brix crash
(main
  (call run)
)

(fn-do + run _
  (let doubled
    (block
      (spread (.. 1 3))
      (pipe *> (call check each))
    )
  )
  (for (in item doubled)
    (print item)
  )
)

(fn-do + check Int
  (param n Int)
  (if (== n 2)
    (throw (call Panic "no"))
  )
  (return n)
)
```

```output
1
```

### A panic in the function of a pipe a function returns, read through `join`.

```brix crash
(main
  (call run)
)

(fn-do + run _
  (print (call (. ", " join) (call numbers)))
)

(fn-do + numbers (stream Int)
  (return
    (block
      (spread (.. 1 3))
      (pipe *> (call check each))
    )
  )
)

(fn-do + check Int
  (param n Int)
  (if (== n 2)
    (throw (call Panic "no"))
  )
  (return n)
)
```

### A panic in the function of a pipe over a range kept in a local.

```brix crash
(main
  (call run)
)

(fn-do + run _
  (let source (.. 1 3))
  (let mapped (*> source (call check each)))
  (for (in item mapped)
    (print item)
  )
)

(fn-do + check Int
  (param n Int)
  (if (== n 2)
    (throw (call Panic "no"))
  )
  (return n)
)
```

```output
1
```

### A panic in the function of the third pipe a function writes, a filter.

```brix crash
(main
  (call run 2)
)

(fn-do + run _
  (param bad Int)
  (let first
    (block
      (spread (.. 1 3))
      (pipe *> (call check each 9))
      (pipe ?> (> (call check each 9) 0))
    )
  )
  (for (in item first)
    (print item)
  )
  (let second
    (block
      (spread (.. 1 3))
      (pipe ?> (> (call check each bad) 0))
    )
  )
  (for (in item second)
    (print item)
  )
)

(fn-do + check Int
  (param n Int)
  (param bad Int)
  (if (== n bad)
    (throw (call Panic "no"))
  )
  (return n)
)
```

```output
1
2
3
1
```

### A panic in the function of the third pipe written on one line, read through a fourth by `join`.

```brix crash
(main
  (call run)
)

(fn-do + run _
  (let list (list 1 2 3))
  (let first (*> list (+ each 1)))
  (let second (?> list (!= each 9)))
  (let third (*> list (call check each)))
  (for (in item first)
    (print item)
  )
  (for (in item second)
    (print item)
  )
  (print (call (. ", " join) (*> third (str each))))
)

(fn-do + check Int
  (param n Int)
  (if (== n 2)
    (throw (call Panic "no"))
  )
  (return n)
)
```

```output
2
3
4
1
2
3
```

### A panic in the function of a filter that follows a map, after a list of feeds that calls a closure has been read.

```brix crash
(main
  (call run)
)

(fn-do + run _
  (let list (list 1 2 3))
  (let fn (=> (n) (+ n 1)))
  (let source
    (block
      (item (call fn 1))
      (item (call check 2))
      (item 3)
    )
  )
  (for (in item source)
    (print item)
  )
  (let piped (?> (*> list (+ each 1)) (> (call check each) 0)))
  (for (in item piped)
    (print item)
  )
)

(fn-do + check Int
  (param n Int)
  (if (== n 4)
    (throw (call Panic "no"))
  )
  (return n)
)
```

```output
2
2
3
2
3
```

### A panic in the function of the second pipe a function writes over one list.

```brix crash
(main
  (call run)
)

(fn-do + run _
  (let list (list 1 2 3))
  (let first (*> list (+ each 1)))
  (for (in item first)
    (print item)
  )
  (let second (*> list (call check each)))
  (for (in item second)
    (print item)
  )
  (let third (in (call check each) list))
  (for (in item third)
    (print item)
  )
)

(fn-do + check Int
  (param n Int)
  (if (== n 3)
    (throw (call Panic "no"))
  )
  (return n)
)
```

```output
2
3
4
1
2
```

### A panic in the function of a pipe over what a generator function gives.

```brix crash
(main
  (call run)
)

(fn-do + run _
  (let mapped (*> (call counted 3) (call check each)))
  (for (in item mapped)
    (print item)
  )
)

(fn-do + counted (stream Int)
  (param n Int)
  (for (in i (.. 1 n))
    (yield (- (call check (+ i 10)) 10))
  )
)

(fn-do + check Int
  (param n Int)
  (if (== n 13)
    (throw (call Panic "no"))
  )
  (return n)
)
```

```output
1
2
```

### Feeds of two pairs, neither the wider: the items have the pair of what the fields at each place join to.

```brix
(main
  (let a
    (block
      (item (tuple 1 "a"))
      (item (tuple 2.5 "b"))
    )
  )
  (for (in (tuple n s) a)
    (print (str n " " s))
  )
)
```

```output
1 a
2.5 b
```

### Feeds of pairs with a float on either side have a float at both places.

```brix
(main
  (let c
    (block
      (item (tuple 2.5 1))
      (item (tuple 1 2.5))
    )
  )
  (for (in item c)
    (print item)
  )
)
```

```output
(2.5, 1)
(1, 2.5)
```

### A feed written `**` gives every item of a list, between two items.

```brix
(main
  (let items (list 4 5 6))
  (let all
    (block
      (item 1)
      (spread items)
      (item 9)
    )
  )
  (for (in item all)
    (print item)
  )
)
```

```output
1
4
5
6
9
```

### Two feeds written `**` over generators, with an item between them.

```brix
(main
  (let all
    (block
      (spread (call counted 3))
      (item 10)
      (spread (call counted 2))
    )
  )
  (for (in item all)
    (print item)
  )
)

(fn-do + counted (stream Int)
  (param limit Int)
  (for (in index (..< 0 limit))
    (yield index)
  )
)
```

```output
0
1
2
10
0
1
```

### `**?` gives the items of a list an optional holds, and nothing where it holds none.

```brix
(main
  (let foo (only-if (list 1 2 3) (if (> 5 6))))
  (let bar (only-if (list 4 5 6) (if (> 5 3))))
  (print
    (call (. ", " join)
      (block
        (item 0)
        (spread? foo)
        (spread? bar)
      )
    )
  )
)
```

```output
0, 4, 5, 6
```

### `**` over a range and over a list of floats: the items have the wider type.

```brix
(main
  (let items (list 4.5 5.5))
  (let all
    (block
      (spread (.. 1 3))
      (spread items)
      (item 9)
    )
  )
  (for (in item all)
    (print item)
  )
)
```

```output
1
2
3
4.5
5.5
9
```

### Three feeds written `**` over lists, one list twice.

```brix
(main
  (let words (list "a" "b"))
  (let more (list "c"))
  (let all
    (block
      (spread words)
      (spread more)
      (spread words)
    )
  )
  (print (call (. ", " join) all))
)
```

```output
a, b, c, a, b
```

### `**` over a generator with a cleanup, left early: the cleanup runs before what follows the loop.

```brix
(main
  (let all
    (block
      (spread (call counted 5))
      (item 100)
    )
  )
  (for (in item all)
    (print item)
    (if (== item 1)
      (break)
    )
  )
  (print "after")
)

(fn-do + counted (stream Int)
  (param limit Int)
  (finally
    (print "closed")
  )
  (for (in index (..< 0 limit))
    (yield index)
  )
)
```

```output
0
1
closed
after
```

### `**` over a generator with a cleanup, read to its end: the cleanup runs before the item after it.

```brix
(main
  (let all
    (block
      (spread (call counted 5))
      (item 100)
    )
  )
  (for (in item all)
    (print item)
  )
  (print "after")
)

(fn-do + counted (stream Int)
  (param limit Int)
  (finally
    (print "closed")
  )
  (for (in index (..< 0 limit))
    (yield index)
  )
)
```

```output
0
1
2
3
4
closed
100
after
```

### `**` over a generator left early: the item after it is never given.

```brix
(main
  (let all
    (block
      (spread (call counted 5))
      (item 100)
    )
  )
  (for (in item all)
    (print item)
    (if (== item 1)
      (break)
    )
  )
  (print "after")
)

(fn-do + counted (stream Int)
  (param limit Int)
  (for (in index (..< 0 limit))
    (yield index)
  )
)
```

```output
0
1
after
```

### A single `**` feed over a list of values of an interface, mapped by a pipe and made a list.

```brix
(main
  (let nodes (list Named) (call (of List Named)))
  (+= nodes (call Leaf "a"))
  (+= nodes (call Leaf "c"))
  (let kept
    (block
      (spread nodes)
      (pipe *> (. each label))
      (pipe |> List)
    )
  )
  (print ([] kept 1))
  (print (. kept length))
)

(type + Named (is Interface)
  (fn-expr + label String :get)
)

(type + Leaf
  (implements Named)
  (prop + label String)
  (fn-do + This _
    (param label _)
  )
)
```

```output
c
2
```

### `**?` over a generator an optional holds, and over one that holds none.

```brix
(main
  (let maybe (only-if (call counted 2) (if true)))
  (let none (only-if (call counted 2) (if false)))
  (let all
    (block
      (item 7)
      (spread? maybe)
      (spread? none)
    )
  )
  (for (in item all)
    (print item)
  )
)

(fn-do + counted (stream Int)
  (param limit Int)
  (for (in index (..< 0 limit))
    (yield index)
  )
)
```

```output
7
0
1
```

### `**` over a property and over what a method answers.

```brix
(main
  (let holder (call Holder))
  (let all
    (block
      (spread (. holder items))
      (spread (call (. holder more)))
    )
  )
  (for (in item all)
    (print item)
  )
)

(type + Holder
  (prop + items _ (= (list 1 2)))
  (fn-do + more (list Int)
    (print "more")
    (return (list 3))
  )
)
```

```output
1
2
more
3
```

### `**` over a value that iterates by a cursor.

```brix
(main
  (let all
    (block
      (item 0)
      (spread (call CursorRange 3))
      (item 9)
    )
  )
  (for (in item all)
    (print item)
  )
)

(type + CursorRange (is Struct)
  (implements (of CursorIterable I32 I32))
  (prop + limit I32)
  (fn-expr + initial-iterator-state I32 :get
    0
  )
  (fn-do + next-iterator (tuple I32 (? I32))
    (param state I32)
    (let none (? I32))
    (if (>= state limit)
      (return (tuple state none))
    )
    (blank)
    (return (tuple (+ state 1) (+ state 1)))
  )
)
```

```output
0
1
2
3
9
```

### Two `**` feeds over cursors: each begins from its own state.

```brix
(main
  (let all
    (block
      (spread (call CursorRange 2))
      (spread (call CursorRange 3))
    )
  )
  (for (in item all)
    (print item)
  )
)

(type + CursorRange (is Struct)
  (implements (of CursorIterable I32 I32))
  (prop + limit I32)
  (fn-expr + initial-iterator-state I32 :get
    0
  )
  (fn-do + next-iterator (tuple I32 (? I32))
    (param state I32)
    (let none (? I32))
    (if (>= state limit)
      (return (tuple state none))
    )
    (blank)
    (return (tuple (+ state 1) (+ state 1)))
  )
)
```

```output
1
2
1
2
3
```

### `**` over a text gives its characters.

```brix
(main
  (let text "héy")
  (let all
    (block
      (item '<')
      (spread text)
      (item '>')
    )
  )
  (for (in item all)
    (print item)
  )
)
```

```output
<
h
é
y
>
```

### `**` over the keys of a map.

```brix
(main
  (let ages (call (of Map String Int)))
  (= ([] ages "ann") 3)
  (= ([] ages "bob") 5)
  (let all
    (block
      (spread (. ages keys))
      (item "zed")
    )
  )
  (for (in item all)
    (print item)
  )
)
```

```output
ann
bob
zed
```

### `**` over an array.

```brix
(main
  (let cells (call (of Array Int) 3))
  (= ([] cells 0) 4)
  (= ([] cells 2) 6)
  (let all
    (block
      (item 1)
      (spread cells)
    )
  )
  (for (in item all)
    (print item)
  )
)
```

```output
1
4
0
6
```

### `**` over a range held in a local.

```brix
(main
  (let span (..< 2 5))
  (let all
    (block
      (item 1)
      (spread span)
      (item 9)
    )
  )
  (for (in item all)
    (print item)
  )
)
```

```output
1
2
3
4
9
```

### `**?` over an optional argument, given none and given a list.

```brix
(main
  (print (call total null))
  (print (call total (list 4 5)))
)

(fn-do + total Int
  (param more (? (list Int)))
  (let all
    (block
      (item 1)
      (spread? more)
      (item 2)
    )
  )
  (let sum 0)
  (for (in item all)
    (+= sum item)
  )
  (return sum)
)
```

```output
3
12
```

### `**` over a stream a function is given.

```brix
(main
  (call show (call counts 3))
)

(fn-do + counts (stream Int)
  (param limit Int)
  (for (in place (..< 0 limit))
    (yield (* place 2))
  )
)

(fn-do + show _
  (param source (stream Int))
  (let all
    (block
      (item -1)
      (spread source)
      (item -2)
    )
  )
  (for (in item all)
    (print item)
  )
)
```

```output
-1
0
2
4
-2
```

### A function whose body is two `**` feeds answers a list.

```brix
(main
  (let picked (call both (list 1 2) (list 3)))
  (print (. picked length))
  (print ([] picked 2))
)

(fn-expr + both (list Int)
  (param first (list Int))
  (param second (list Int))
  (spread first)
  (spread second)
)
```

```output
3
3
```

### A method whose body is feeds reads a property by its name and through `this`.

```brix
(main
  (let box (call Box (list 5 6)))
  (for (in item (call (. box all)))
    (print item)
  )
  (print (call (. box sum)))
)

(type + Box
  (prop + items (list Int))
  (fn-do + This _
    (param items _)
  )
  (fn-expr + all (stream Int)
    (item 0)
    (spread items)
    (spread (. this items))
  )
  (fn-do + sum Int
    (let total 0)
    (for (in item (call all))
      (+= total item)
    )
    (return total)
  )
)
```

```output
0
5
6
5
6
22
```

### `**` in the body of a loop, over the item of each turn.

```brix
(main
  (let rows (list (list Int)) (list (list 1 2) (list 3)))
  (for (in row rows)
    (let line
      (block
        (item 0)
        (spread row)
      )
    )
    (let count 0)
    (for (in item line)
      (+= count item)
    )
    (print count)
  )
)
```

```output
3
3
```

### `**` over a list of tuples, read by a loop that names their fields.

```brix
(main
  (let pairs (list (tuple 1 "a") (tuple 2 "b")))
  (let all
    (block
      (spread pairs)
      (item (tuple 3 "c"))
    )
  )
  (for (in (tuple count name) all)
    (print (str count " " name))
  )
)
```

```output
1 a
2 b
3 c
```

### `**` over a list of floats after an integer.

```brix
(main
  (let floats (list 1.5 2.5))
  (let all
    (block
      (item 1)
      (spread floats)
    )
  )
  (for (in item all)
    (print item)
  )
)
```

```output
1
1.5
2.5
```

### Two `**` feeds over generators, mapped by a pipe.

```brix
(main
  (let all
    (block
      (spread (call letters "ab"))
      (spread (call letters "c"))
      (pipe *> (str each "!"))
    )
  )
  (for (in item all)
    (print item)
  )
)

(fn-do + letters (stream String)
  (param text String)
  (for (in letter text)
    (yield (str letter))
  )
)
```

```output
a!
b!
c!
```

### A single `**` feed over a list written there, mapped and made a list.

```brix
(main
  (let all
    (block
      (spread (list 1 2 3))
      (pipe *> (* each 2))
      (pipe |> List)
    )
  )
  (print (. all length))
  (print ([] all 0))
)
```

```output
3
2
```

### Feeds with a `**` given to a function that takes a stream.

```brix
(main
  (let names (list "b" "a"))
  (print
    (call count
      (block
        (spread names)
        (item "c")
      )
    )
  )
)

(fn-do + count Int
  (param items (stream String))
  (let total 0)
  (for (in item items)
    (+= total (. item length))
  )
  (return total)
)
```

```output
3
```

### A function whose body is feeds, with a list written between two items.

```brix
(main
  (for (in item (call outer))
    (print item)
  )
)

(fn-expr + outer (stream Int)
  (item 1)
  (spread (list 2 3))
  (item 4)
)
```

```output
1
2
3
4
```

### A `**` feed between two items reads a generator that panics: the items before are printed.

```brix crash
(main
  (let all
    (block
      (item 5)
      (spread (call picks (list 0 7)))
      (item 6)
    )
  )
  (for (in item all)
    (print item)
  )
)

(fn-do + picks (stream Int)
  (param places (list Int))
  (let items (list 10 20 30))
  (for (in place places)
    (yield ([] items place))
  )
)
```

```output
5
10
```

### The first feed of a list is a `**` over a generator that panics on its second item.

```brix crash
(main
  (let all
    (block
      (spread (call picks (list 0 7)))
      (blank)
      (item 6)
    )
  )
  (for (in item all)
    (print item)
  )
)

(fn-do + picks (stream Int)
  (param places (list Int))
  (let items (list 10 20 30))
  (for (in place places)
    (yield ([] items place))
  )
)
```

```output
10
```

### A single `**` feed over a generator that panics, mapped by a pipe.

```brix crash
(main
  (let all
    (block
      (spread (call picks (list 0 7)))
      (pipe *> (+ each 1))
    )
  )
  (for (in item all)
    (print item)
  )
)

(fn-do + picks (stream Int)
  (param places (list Int))
  (let items (list 10 20 30))
  (for (in place places)
    (yield ([] items place))
  )
)
```

```output
11
```

### A `**` feed after an item, mapped by a pipe, over a generator that panics.

```brix crash
(main
  (let all
    (block
      (item 4)
      (spread (call picks (list 1 7)))
      (pipe *> (+ each 1))
    )
  )
  (for (in item all)
    (print item)
  )
)

(fn-do + picks (stream Int)
  (param places (list Int))
  (let items (list 10 20 30))
  (for (in place places)
    (yield ([] items place))
  )
)

(fn-expr + pick Int
  (param place Int)
  ([] (list 10 20 30) place)
)
```

```output
5
21
```

### A stack trace through the first feed of a list, a `**` over a generator that panics: the frame of the list is on the line under the feed.

```brix crash
(main
  (let spread-first
    (block
      (spread (call names (list 0 7)))
    )
  )
  (blank)
  (print (call (. ", " join) spread-first))
  (print "after")
)

(fn-do + names (stream String)
  (param places (list Int))
  (let items (list "a" "b" "c"))
  (for (in place places)
    (yield ([] items place))
  )
)
```

```stderr
Unhandled panic: List index 7 out of range
<core/list.bx>

Task 1

Stack trace:
  0  core/list:List:absent core/list.bx
  1  __source__:names no file:17:12
  2  core/stream:Generator:<-read core/stream.bx
  3  core/stream:<-read(wrapped) core/stream.bx
  4  __source__:spread-first no file:6:7
  5  core/stream:Generator:<-read core/stream.bx
  6  core/stream:<-read(wrapped) core/stream.bx
  7  core/string:String:join core/string.bx
  8  __source__:main no file:9:10
  9  __source__:init.__source__ no file
  10  __source__:brix.main no file
```

### A stack trace through a `**` feed after an item: the frame of the list is at the feed.

```brix crash
(main
  (let spread-second
    (block
      (item "z")
      (spread (call names (list 1 7)))
    )
  )
  (print (call (. ", " join) spread-second))
  (print "after")
)

(fn-do + names (stream String)
  (param places (list Int))
  (let items (list "a" "b" "c"))
  (for (in place places)
    (yield ([] items place))
  )
)
```

```stderr
Unhandled panic: List index 7 out of range
<core/list.bx>

Task 1

Stack trace:
  0  core/list:List:absent core/list.bx
  1  __source__:names no file:17:12
  2  core/stream:Generator:<-read core/stream.bx
  3  core/stream:<-read(wrapped) core/stream.bx
  4  __source__:spread-second no file:6:7
  5  core/stream:Generator:<-read core/stream.bx
  6  core/stream:<-read(wrapped) core/stream.bx
  7  core/string:String:join core/string.bx
  8  __source__:main no file:9:10
  9  __source__:init.__source__ no file
  10  __source__:brix.main no file
```

### A stack trace through the first `**` feed of a list in a function, with a line left empty under it.

```brix crash
(main
  (print (call gathered 1))
  (print (call gathered 7))
)

(fn-do + gathered String
  (param place Int)
  (let spread-gathered
    (block
      (spread (call names (list 0 place)))
      (blank)
      (item "z")
    )
  )
  (return (call (. ", " join) spread-gathered))
)

(fn-do + names (stream String)
  (param places (list Int))
  (let items (list "a" "b" "c"))
  (for (in place places)
    (yield ([] items place))
  )
)
```

```stderr
Unhandled panic: List index 7 out of range
<core/list.bx>

Task 1

Stack trace:
  0  core/list:List:absent core/list.bx
  1  __source__:names no file:23:12
  2  core/stream:Generator:<-read core/stream.bx
  3  core/stream:<-read(wrapped) core/stream.bx
  4  __source__:spread-gathered no file:12:7
  5  core/stream:Generator:<-read core/stream.bx
  6  core/stream:<-read(wrapped) core/stream.bx
  7  core/string:String:join core/string.bx
  8  __source__:gathered no file:16:11
  9  __source__:main no file:4:10
  10  __source__:init.__source__ no file
  11  __source__:brix.main no file
```

### A stack trace through the second of two `**` feeds over streams held in a list.

```brix crash
(main
  (let spread-held (list (call names (list 0)) (call names (list 7))))
  (let spread-each
    (block
      (item "z")
      (spread ([] spread-held 0))
      (spread ([] spread-held 1))
    )
  )
  (print (call (. ", " join) spread-each))
  (print "after")
)

(fn-do + names (stream String)
  (param places (list Int))
  (let items (list "a" "b" "c"))
  (for (in place places)
    (yield ([] items place))
  )
)
```

```stderr
Unhandled panic: List index 7 out of range
<core/list.bx>

Task 1

Stack trace:
  0  core/list:List:absent core/list.bx
  1  __source__:names no file:19:12
  2  core/stream:Generator:<-read core/stream.bx
  3  core/stream:<-read(wrapped) core/stream.bx
  4  __source__:spread-each no file:8:7
  5  core/stream:Generator:<-read core/stream.bx
  6  core/stream:<-read(wrapped) core/stream.bx
  7  core/string:String:join core/string.bx
  8  __source__:main no file:11:10
  9  __source__:init.__source__ no file
  10  __source__:brix.main no file
```

### A value is written whole to what a pipe names, on one line or the next, and the pipe gives what it wrote to

```brix
(main
  (let log (call Log "first"))
  (.. 1 3)
  (pipe -> log)
  (-> (list 4 5) log)
  (-> "text" log)
  (-> (call value "left") (call made "right"))
  (let same (-> 7 log))
  (print (. same name))
  (print (. log count))
  (let tally (call Tally 0))
  (= tally (-> 5 tally))
  (= tally (-> 6 tally))
  (print (. tally total))
)

(type + Log
  (prop + name String)
  (prop + count _ (= 0))
  (fn-do + This _
    (param name _)
  )
  (fn-do + (infix this <- (param n _)) _
    (+= count 1)
    (print (str name " got " n))
  )
)

(type + Tally (is Tuple)
  (prop + total Int)
  (fn-do + (infix this <- (param n Int)) _
    (print (str "tally " total " sees " n))
  )
)

(fn-do + value String
  (param text String)
  (print (str "value " text))
  (return text)
)

(fn-do + made Log
  (param text String)
  (print (str "made " text))
  (return (call Log text))
)
```

```output
first got [Range 1 thru 3 by 1]
first got 4, 5
first got text
value left
made right
right got left
first got 7
first
4
tally 0 sees 5
tally 0 sees 6
0
```

### A list of one feed is written whole to a fixed array, and the pipe gives the array.

```brix
(main
  (let values (list I32))
  (+= values 4)
  (+= values 7)
  (let collected
    (block
      (spread values)
      (pipe -> ([] I32 (. values length)))
    )
  )
  (print ([] collected 0))
  (print ([] collected 1))
)
```

```output
4
7
```

### Spread over a base class reads by the derived class

```brix
(main
  (let base Base (call Derived))
  (let all
    (block
      (spread base)
    )
  )
  (print (call (. ", " join) all))
  (for (in a base)
    (and-for (in b base)
      (print (+ a b))
    )
  )
)

(type + Base
  (implements (of Indexable Int))
  (fn-expr + length Size :get
    2
  )
  (fn-expr + (index-get this (param index Index)) Int
    (+ index 1)
  )
  (fn-expr + (index-get this (param range (of Range Index))) Base
    this
  )
)

(type + Derived (is Base)
  (fn-expr + (index-get this (param index Index)) Int
    (+ (* index 100) 5)
  )
)
```

```output
5, 105
10
210
```

### `*->` writes each item of a list to an object by its `<-`, and gives what it wrote to.

```brix
(main
  (let sink (call Sink))
  (let back (*-> (list 10 20 30) sink))
  (print (. back total))
)

(type + Sink
  (prop + total _ (= 0))
  (fn-do + (infix this <- (param n _)) _
    (+= total n)
    (print (str "got " n))
  )
)
```

```output
got 10
got 20
got 30
60
```

### `*->` writes each item a generator gives.

```brix
(main
  (let sink (call Sink))
  (*-> (call items) sink)
  (print (. sink total))
)

(type + Sink
  (prop + total _ (= 0))
  (fn-do + (infix this <- (param n _)) _
    (+= total n)
    (print (str "got " n))
  )
)

(fn-do + items _
  (yield 1)
  (yield 2)
  (yield 3)
)
```

```output
got 1
got 2
got 3
6
```

### `*->` writes each item of a range that counts down, and of a list, to a list.

```brix
(main
  (let list (list Int))
  (.. 3 1)
  (pipe *-> list)
  (print list)
  (let more (*-> (list 4 5) list))
  (print more)
)
```

```output
3, 2, 1
3, 2, 1, 4, 5
```

### `*->` reads a call's result once.

```brix
(main
  (let sink (call Sink))
  (*-> (call make) sink)
  (print (. sink total))
)

(type + Sink
  (prop + total _ (= 0))
  (fn-do + (infix this <- (param n _)) _
    (+= total n)
  )
)

(fn-do + make (list Int)
  (print "made")
  (return (list 5 6))
)
```

```output
made
11
```

### `*->` whose left side is another `*->`, written to first.

```brix
(main
  (let a (call Sink))
  (let b (call Sink))
  (= (. b name) "b")
  (*-> (. (*-> (list 1 2) a) seen) b)
)

(type + Sink
  (prop + name _ (= "a"))
  (prop + seen (list Int))
  (fn-do + (infix this <- (param n _)) _
    (call (. seen append) (* n 10))
    (print (str name " " n))
  )
)
```

```output
a 1
a 2
b 10
b 20
```

### `*->` over a list in a function that yields.

```brix
(main
  (for (in n (call nums))
    (print n)
  )
)

(fn-do + nums _
  (let sink (call Sink))
  (*-> (list 7 8) sink)
  (yield (. sink total))
)

(type + Sink
  (prop + total _ (= 0))
  (fn-do + (infix this <- (param n _)) _
    (+= total n)
  )
)
```

```output
15
```

### `*->` over a generator in a function that yields.

```brix
(main
  (for (in n (call nums))
    (print n)
  )
)

(fn-do + nums _
  (yield 0)
  (let sink (call Sink))
  (*-> (call items) sink)
  (yield (. sink total))
  (yield 9)
)

(type + Sink
  (prop + total _ (= 0))
  (fn-do + (infix this <- (param n _)) _
    (+= total n)
  )
)

(fn-do + items _
  (yield 1)
  (yield 2)
  (yield 4)
)
```

```output
0
7
9
```

### A write that raises leaves `*->` and the function at once.

```brix
(main
  (let sink (call Sink))
  (let failed (catch (call run sink)))
  (print (is failed Problem))
  (print (. sink total))
)

(type + Problem (is Error))

(type + Sink
  (prop + total _ (= 0))
  (fn-do + (infix this <- (param n _)) _
    (if (== n 2)
      (throw (call Problem "two"))
    )
    (+= total n)
  )
)

(fn-do + run _ (?! Problem)
  (param sink Sink)
  (*-> (list 1 2 3) sink)
)
```

```output
true
1
```

### `*->` reading a generator that raises leaves by its error after the items before it.

```brix
(main
  (let sink (call Sink))
  (let failed (catch (call run sink)))
  (print (is failed Problem))
  (print (. sink total))
)

(type + Problem (is Error))

(type + Sink
  (prop + total _ (= 0))
  (fn-do + (infix this <- (param n _)) _
    (+= total n)
  )
)

(fn-do + run _ (?! Problem)
  (param sink Sink)
  (*-> (call items) sink)
)

(fn-do + items _
  (yield 1)
  (yield 2)
  (throw (call Problem "bad"))
)
```

```output
true
3
```

### A list of one feed piped into a function whose items are of another type.

```brix
(main
  (print
    (call List
      (block
        (spread (.. 1 3))
        (pipe |> names)
      )
    )
  )
)

(fn-do + names _
  (param items (stream Int))
  (for (in i items)
    (yield (str "n" i))
  )
)
```

```output
n1, n2, n3
```

### A list of one feed piped into a function, its value declared.

```brix
(main
  (let total
    (block
      (item 4)
      (pipe |> sum)
    )
  )
  (print total)
)

(fn-do + sum Int
  (param items (stream Int))
  (let t 0)
  (for (in i items)
    (+= t i)
  )
  (return t)
)
```

```output
4
```

### A list of one feed piped into a parameter of a list type.

```brix
(main
  (let n
    (block
      (spread (.. 1 4))
      (pipe |> count)
    )
  )
  (print n)
)

(fn-do + count Int
  (param items (list Int))
  (return (. items length))
)
```

```output
4
```

### A list of one optional feed piped into a function.

```brix
(main
  (let maybe (? Int) 5)
  (print
    (call List
      (block
        (item? maybe)
        (pipe |> names)
      )
    )
  )
)

(fn-do + names _
  (param items (stream Int))
  (for (in i items)
    (yield (str "n" i))
  )
)
```

```output
n5
```

### A list of one feed piped into a function given a closure, in a function's body.

```brix
(main
  (print (call List (call evens)))
)

(fn-expr + evens _
  (spread (.. 1 6))
  (pipe |> (call double (:where (== (% each 2) 0))))
)

(fn-do + (of double Item) _
  (param items (stream Item))
  (param where (of Predicate Item))
  (for (in item items)
    (if (call where item)
      (yield (* item 2))
    )
  )
)
```

```output
4, 8, 12
```

### A map-filter pipe whose clause declares a lazy local.

```brix
(main
  (print (call List (call doubled)))
)

(fn-expr + doubled _
  (call nums)
  (pipe ?*>
    (scope (only-if twice (if (> each 1)))
      (lazy twice (* each 2))
    )
  )
)

(fn-expr + nums _
  (item 1)
  (item 2)
  (item 3)
)
```

```output
4, 6
```

### A map-filter pipe whose clause takes each item apart lazily.

```brix
(main
  (print (call List (call pairs)))
)

(fn-expr + pairs _
  (call nums)
  (pipe ?*>
    (scope (only-if name (if (> n 1)))
      (lazy (tuple n name) each)
    )
  )
)

(fn-expr + nums _
  (item (tuple 1 "a"))
  (item (tuple 2 "b"))
  (item (tuple 3 "c"))
)
```

```output
b, c
```

### Lists of feeds as properties' defaults: each object gets its own list; `*?` and `**`.

```brix
(main
  (let a (call App))
  (let b (call App))
  (+= (. a values) 3)
  (print (tuple (. a values length) (. b values length)))
  (print (. a names))
  (print (. b more))
)

(type + App
  (prop + values (list Int)
    (block
      (item 1)
      (item 2)
    )
  )
  (prop + names (list String)
    (block
      (item "x")
      (item? (call maybe))
      (item? (call never))
    )
  )
  (prop + more (list Int)
    (block
      (spread (.. 1 3))
      (item 9)
    )
  )
)

(fn-expr + maybe (? String)
  "y"
)

(fn-expr + never (? String)
  null
)
```

```output
(3, 2)
x, y
1, 2, 3, 9
```

### A derived class's feed default calls a function; the base's default is kept.

```brix
(main
  (let b (call Derived))
  (print (tuple (. b values) (. b extra)))
)

(type + Base
  (prop + values (list Int)
    (block
      (item 4)
    )
  )
)

(type + Derived (is Base)
  (prop + extra (list Int)
    (block
      (item (call total))
      (item 5)
    )
  )
)

(fn-expr + total Int
  (+ 40 2)
)
```

```output
(4, 42, 5)
```

### A constructor argument replaces a feed default; its own default is a list.

```brix
(main
  (let a (call App (list 7)))
  (print (. a values))
  (let b (call App))
  (print (. b values))
)

(type + App
  (prop + values (list Int)
    (block
      (item 1)
    )
  )
  (fn-do + This _
    (param values (list Int) (= (list 2)))
  )
)
```

```output
7
2
```

### A lazy dependency written between two headers leaves the first header in the chain

```brix
(main
  (print (call foo))
)

(fn-expr + foo _
  (header (call Thing))
  1
  base
  (blank)
  (lazy base 10)
  (header (call Thing))
  20
  more
  (blank)
  (lazy more 300)
)

(type + Thing
  (prop + count _ (= 0))
  (fn-to-expr + String
    (str "Thing: " count)
  )
  (fn-do + with _
    (param next Int)
    (+= count next)
    (return this)
  )
  (fn-do + with _
    (param next Thing)
    (+= (. next count) count)
    (return next)
  )
)
```

```output
Thing: 331
```

```brix
(main
  (print (call feed-deep))
)

(fn-do + feed-deep Int
  (let values
    (block
      (item 1)
      (pipe *> (+ each 1))
      (pipe *> (+ each 1))
      (pipe *> (+ each 1))
      (pipe *> (+ each 1))
      (pipe *> (+ each 1))
      (pipe *> (+ each 1))
      (pipe *> (+ each 1))
      (pipe *> (+ each 1))
      (pipe *> (+ each 1))
      (pipe *> (+ each 1))
      (pipe *> (+ each 1))
      (pipe *> (+ each 1))
      (pipe *> (+ each 1))
      (pipe *> (+ each 1))
      (pipe *> (+ each 1))
      (pipe *> (+ each 1))
      (pipe *> (+ each 1))
      (pipe *> (+ each 1))
      (pipe *> (+ each 1))
      (pipe *> (+ each 1))
      (pipe *> (+ each 1))
      (pipe *> (+ each 1))
      (pipe *> (+ each 1))
      (pipe *> (+ each 1))
      (pipe *> (+ each 1))
      (pipe *> (+ each 1))
      (pipe *> (+ each 1))
      (pipe *> (+ each 1))
      (pipe *> (+ each 1))
      (pipe *> (+ each 1))
      (pipe *> (+ each 1))
      (pipe *> (+ each 1))
      (pipe *> (+ each 1))
      (pipe *> (+ each 1))
      (pipe *> (+ each 1))
      (pipe *> (+ each 1))
    )
  )
  (let total 0)
  (for (in value values)
    (+= total value)
  )
  (return total)
)
```

```output
37
```

```brix
(import /BxGC/BxGC.h)
(blank)

(main
  (let first (call EscapingTapBox "one"))
  (let second (call EscapingTapBox "two"))
  (let values (call tap-managed-escaping first second))
  (call C:brixGCCollect)
  (for (in item values)
    (call C:brixGCCollect)
    (print (. item text))
  )
  (print (. first text))
  (print (. second text))
)

(type + EscapingTapBox
  (prop + text String)
  (fn-do + This _
    (param text _)
  )
)

(fn-do + update _
  (param item EscapingTapBox)
  (= (. item text) (+ (. item text) "!"))
)

(fn-expr + tap-managed-escaping _
  (param first EscapingTapBox)
  (param second EscapingTapBox)
  (item first)
  (item second)
  (pipe *|> (call update each))
)
```

```output
one!
two!
one!
two!
```

```brix
(main
  (let result (catch (call tap-failure)))
  (print (is result EscapingTapFailure))
)

(type + EscapingTapFailure (is Error))

(fn-do + tap-failure Int (?! EscapingTapFailure)
  (let values (call tap-error-escaping))
  (for (in value values)
    (print (str "out " value))
  )
  (return 0)
)

(fn-expr + tap-error-escaping _
  (spread (call tap-source))
  (pipe *|> (call tap-action each))
)

(fn-do + tap-source _ @inline
  (finally
    (print "closed")
  )
  (yield 1)
  (yield 2)
  (yield 3)
)

(fn-do + tap-action _ (?! EscapingTapFailure)
  (param value Int)
  (print (str "tap " value))
  (if (== value 2)
    (throw (call EscapingTapFailure "failed"))
  )
)
```

```output
tap 1
out 1
tap 2
closed
true
```

### A list of feeds written in a lazy local that a generator reads first is given the environment that holds the local.

```brix
(main
  (for (in line (call request (list "a" "b")))
    (print line)
  )
)

(fn-expr + request _
  (param names (list String))
  (item prompt)
  (spread names)
  (pipe |> List)
  (where
    (lazy prompt
      (call (. "" join)
        (block
          (item "You are here. ")
          (item "Answer briefly.")
        )
      )
    )
  )
)
```

```output
You are here. Answer briefly.
a
b
```

### Spreads whose items have different types each convert their items to the list's item type, and the list keeps them in the order they are written.

```brix
(main
  (let scopes (list (call Scope 1u32)))
  (print (call (. ", " join) (call boundaries "a\nb" scopes)))
)

(type + Scope (is Tuple)
  (prop + start U32)
)

(fn-expr + boundaries (list Size)
  (param source String)
  (param scopes (list Scope))
  (spread
    (block
      scopes
      (pipe *> (. each start))
    )
  )
  (spread
    (block
      (..< 0 (. source length))
      (pipe ?> (== ([] source each) (char "\n")))
    )
  )
  (spread
    (block
      (..< 0 (. source length))
      (pipe *> (+ each 1))
    )
  )
)
```

```output
1, 1, 1, 2, 3
```

### A range of integers written with no suffix that a spread gives holds the type of the list's items.

```brix
(main
  (print (call bytes))
)

(fn-expr + bytes (of List U8)
  (item 1)
  (spread (.. 2 4))
)
```

```output
1, 2, 3, 4
```

### A row a constructor takes from feeds whose value converts only when asked is reported where the value is written.

```brix compile-error
(main
  (let big 9u64)
  (let m
    (call (of Map String Number)
      (block
        (entry "kind" 1)
        (entry "serial" big)
      )
    )
  )
  (print (. m length))
)

(type + Number (is Union)
  (variant Int)
  (variant Float)
)
```

```output
Exception: Expected Number here, but this is U64, which becomes Number only when asked; write "(as big Number)"
<no file:8>
            (entry "serial" big)
                            ^^^
```

### A List given feeds whose items raise: an error leaves the function, and a catch holds it. A global made so takes its items as its module starts.

```brix
(prop + labels (list String)
  (=
    (call List
      (block
        (item (call label "a"))
        (item (call label "b"))
      )
    )
  )
)
(blank)

(main
  (print (call totals 1))
  (let failed (catch (call totals -1)))
  (print
    (match failed
      (case (list Int)
        (str "list " it)
      )
      (case IOError
        (str "io " (. it message))
      )
    )
  )
  (let held (catch? (call totals -3)))
  (print (?? held (list 0)))
  (print labels)
)

(fn-do + totals (list Int)
  (param start Int)
  (return
    (call List
      (block
        (item (call check start))
        (item (call check (+ start 1)))
      )
    )
  )
)

(fn-do + check Int
  (param value Int)
  (throw (call IOError "negative") (if (< value 0)))
  (return (+ value 1))
)

(fn-do + label String
  (param text String)
  (throw (call IOError "empty") (if (== (. text length) 0)))
  (return (str "<" text ">"))
)
```

```output
2, 3
io negative
0
<a>, <b>
```

### A List given feeds builds its items where it is written, as one given them as arguments does: the value of a property reads the object's members and what they track, and runs again as they change, as does an assignment that reads something tracked.

```brix
(main
  (lazy meter (call Meter))
  (lazy shown (call Shown))
  (blank)
  (= (. shown flags)
    (call (of List Int)
      (block
        (item (. meter #value))
        (item (* (. meter #other) 10))
      )
    )
  )
  (print (str (. meter flags) " | " (. meter plain) " | " (. shown flags)))
  (= (. meter value) 3)
  (print (str (. meter flags) " | " (. meter plain) " | " (. shown flags)))
  (= (. meter other) 5)
  (print (str (. meter flags) " | " (. meter plain) " | " (. shown flags)))
)

(type + Meter
  (prop + value _ (= 1))
  (prop + other _ (= 2))
  (prop + flags _
    (=
      (call (of List Int)
        (block
          (item #value)
          (item (* #other 10))
          (item (+ #value #other))
        )
      )
    )
  )
  (prop + maybe (? Int) (= 4))
  (prop + plain _
    (=
      (block
        (item value)
        (item? maybe)
        (item #other)
        (pipe |> List)
      )
    )
  )
)

(type + Shown
  (prop + flags _ (= (call (of List Int))))
)
```

```output
1, 20, 3 | 1, 4, 2 | 1, 20
3, 20, 5 | 1, 4, 2 | 3, 20
3, 50, 8 | 3, 4, 5 | 3, 50
```

### A List given feeds in the value of a property gives every item of what a spread writes: as the constructor gives the value, as the value runs again on a change it tracks, and once for a lazy property, which keeps it.

```brix
(main
  (let meter (call Meter))
  (print (str (. meter first) " | " (. meter live) " | " (. meter later)))
  (= (. meter others) (call (of List Int) 7 8))
  (= (. meter value) 5)
  (print (str (. meter first) " | " (. meter live) " | " (. meter later)))
)

(type + Meter
  (prop + value _ (= 1))
  (prop + others _ (= (call (of List Int) 2 3)))
  (prop + first _
    (=
      (call (of List Int)
        (block
          (item value)
          (spread others)
        )
      )
    )
  )
  (prop + live _
    (=
      (call (of List Int)
        (block
          (item #value)
          (spread #others)
          (spread (.. 10 11))
        )
      )
    )
  )
  (prop + later _
    (:=
      (call (of List Int)
        (block
          (spread others)
          (item value)
        )
      )
    )
  )
)
```

```output
1, 2, 3 | 1, 2, 3, 10, 11 | 2, 3, 1
1, 2, 3 | 5, 7, 8, 10, 11 | 2, 3, 1
```
