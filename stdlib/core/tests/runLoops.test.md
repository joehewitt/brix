# Loops

```brix
(main
  (let a 3)
  (while (> a 1)
    (print a)
    (= a (- a 1))
  )
)
```

```output
3
2
```

```brix
(main
  (let a 4)
  (while (> a 1)
    (print a)
    (if (== a 3)
      (break)
    )
    (-= a 1)
  )
  (print "done")
)
```

```output
4
3
done
```

```brix
(main
  (let a 4)
  (while (> a 1)
    (if (== a 3)
      (-= a 1)
      (continue)
    )
    (blank)
    (print a)
    (-= a 1)
  )
  (print "done")
)
```

```output
4
2
done
```

```brix
(main
  (let r (..< 0 3))
  (print (. r length))
)
```

```output
3
```

```brix
(main
  (for (in i (..< 0 3))
    (print i)
  )
  (print "done")
)
```

```output
0
1
2
done
```

### A `for` with no body runs its loop for what reading its items does

```brix
(main
  (let counter (call Counter))
  (for (in _ (call ticks counter 3)))
  (for (in item (list 1 2)))
  (for (in i (..< 0 2)))
  (print (. counter value))
)

(type + Counter
  (prop + value _ (= 0))
)

(fn-do + ticks (stream Int)
  (param counter Counter)
  (param n Int)
  (for (in i (..< 0 n))
    (+= (. counter value) 1)
    (yield i)
  )
)
```

```output
3
```

```brix
(main
  (for (in i (..< 0 4))
    (print
      (scope value
        (lazy value (* i 2))
      )
    )
  )
)
```

```output
0
2
4
6
```

```brix crash
(main
  (for (in item 42)
    (print item)
  )
)
```

```output
Exception: A for loop cannot iterate over I64; to count, loop over a range, as in "(..< 0 42)"
<no file:3>
      (for (in item 42)
                    ^^
```

### Cursor iterables keep traversal state outside immutable Brix values

```brix
(main
  (let values (call CursorRange 3))
  (let total 0)
  (for (in value values)
    (print value)
    (+= total value)
  )
  (print total)
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
3
6
```

```brix
(main
  (for (in i (..< 0 3))
    (print i)
    (if (== i 1)
      (break)
    )
  )
  (print "done")
)
```

```output
0
1
done
```

```brix
(main
  (for (in i (..< 0 3))
    (if (== i 1)
      (continue)
    )
    (print i)
  )
  (print "done")
)
```

```output
0
2
done
```

## Clauses

```brix
(main
  (for (in i (..< 0 10) (when (or (== i 3) (== i 5))))
    (print i)
  )
)
```

```output
3
5
```

```brix
(main
  (for (in i (..< 0 10) (while (< i 3)))
    (print i)
  )
)
```

```output
0
1
2
```

```brix
(main
  (for (in i (..< 0 10) (until (== i 3)))
    (print i)
  )
)
```

```output
0
1
2
```

## Combination iterators

```brix
(main
  (for (in i (..< 0 3))
    (and-for (in j (..< 10 13))
      (print (tuple i j))
    )
  )
)
```

```output
(0, 10)
(1, 11)
(2, 12)
```

```brix
(main
  (for (in i (..< 0 10) (when (> i 5)))
    (and-for (in j (..< 10 13))
      (print (tuple i j))
    )
  )
)
```

```output
(6, 10)
(7, 11)
(8, 12)
```

```brix
(main
  (for (in i (..< 0 3))
    (and-for (in j (..< 10 20) (when (> j 15)))
      (print (tuple i j))
    )
  )
)
```

```output
(0, 16)
(1, 17)
(2, 18)
```

```brix
(main
  (for (in i (..< 0 10) (when (> i 5)))
    (and-for (in j (..< 10 20) (when (> j 15)))
      (print (tuple i j))
    )
  )
)
```

```output
(6, 16)
(7, 17)
(8, 18)
(9, 19)
```

### Show that loop breaks when either of streams end

```brix
(main
  (for (in i (..< 0 10))
    (and-for (in j (..< 10 13))
      (print (tuple i j))
    )
  )
)
```

```output
(0, 10)
(1, 11)
(2, 12)
```

```brix
(main
  (for (in pair (call zip (list 1 2 3) (list 4 5 6)))
    (print pair)
  )
)

(fn-do + (of zip T1 T2) _
  (param stream1 (stream T1))
  (param stream2 (stream T2))
  (for (in item1 stream1)
    (and-for (in item2 stream2)
      (yield (tuple item1 item2))
    )
  )
)
```

```output
(1, 4)
(2, 5)
(3, 6)
```

## Iterator expressions

```brix
(main
  (let iter
    (case-each (in n (.. 1 3))
      (* n 2)
    )
  )
  (for (in i iter)
    (print i)
  )
)
```

```output
2
4
6
```

```brix
(main
  (let iter
    (case-each (in n (.. 1 6) (when (== (% n 2) 0)))
      (* n 2)
    )
  )
  (for (in i iter)
    (print i)
  )
)
```

```output
4
8
12
```

```brix
(main
  (let iter
    (case-each (in n (.. 1 6) (while (< n 3)))
      (* n 2)
    )
  )
  (for (in i iter)
    (print i)
  )
)
```

```output
2
4
```

```brix
(main
  (let iter
    (case-each (in n (.. 1 6) (until (== n 3)))
      (* n 2)
    )
  )
  (for (in i iter)
    (print i)
  )
)
```

```output
2
4
```

## Iterator closures

```brix
(main
  (let iter (=> (ns) (in (* each 2) ns)))
  (for (in i (call iter (list 1 2 3)))
    (print i)
  )
)
```

```output
2
4
6
```

```brix
(main
  (let iter (=> (ns) (in each ns (when (% each 2)))))
  (for (in n (call iter (..< 0 8)))
    (print n)
  )
)
```

```output
1
3
5
7
```

```brix
(main
  (for (in i (..< 0 3))
    (finally
      (print "iter")
    )
    (blank)
    (print i)
    (blank)
    (if (== i 1)
      (break)
    )
  )
  (print "done")
)
```

```output
0
iter
1
iter
done
```

```brix
(main
  (let a 4)
  (while (> a 1)
    (finally
      (print "iter")
    )
    (blank)
    (if (== a 3)
      (-= a 1)
      (continue)
    )
    (blank)
    (print a)
    (-= a 1)
  )
  (print "done")
)
```

```output
4
iter
iter
2
iter
done
```

```brix
(main
  (print (call List (inline (in (* each 2) (..< 0 5)))))
)
```

```output
0, 2, 4, 6, 8
```

### A loop header that names a derived class does not narrow its items: match them, or write as.

```brix crash
(main
  (for (in (: a Foo2) (list (call Foo) (call Foo2) (call Foo)))
    (print a)
  )
)

(blank)
(type-line + Foo)
(type-line + Foo2 (is Foo))
```

```output
Exception: Expected Foo2 here, but this is Foo, which is not always Foo2; match it with "(case (: name Foo2) …)", or write "(as Foo2 Foo2)"
<no file:3>
      (for (in (: a Foo2) (list (call Foo) (call Foo2) (call Foo)))
                    ^^^^
```

```brix
(main
  (for (in (| (: a Foo2)) (list (call Foo) (call Foo2) (call Foo)))
    (print a)
  )
)

(blank)
(type-line + Foo)
(type-line + Foo2 (is Foo))
```

```output
[Object __source__:Foo2]
```

```brix
(main
  (for (in (| (: a ({} (>= it 2)))) (list 1 2 3 1 2))
    (print a)
  )
)
```

```output
2
3
2
```

```brix
(main
  (let a (case-each (in ({} (% it 2)) (..< 0 10))))
  (print (call ", " a))
)
```

```output
1, 3, 5, 7, 9
```

### An operand that does not convert is reported at the operand, even when it reads the loop's `it`.

```brix crash
(main
  (print (call nested (list (list 1 2) (list 3))))
)

(fn-do + nested I32
  (param rows (list (list I32)))
  (let total 0)
  (for (in row rows)
    (for (in cell row)
      (+= total (+ cell it))
    )
  )
  (return total)
)
```

```output
Exception: Expected I32 here, but this is [I32]?, and there is no conversion between them
<no file:11>
          (+= total (+ cell it))
                            ^^
```

### A range loop's variable shadows a local of the same name.

```brix
(main
  (print (call sum 4))
)

(fn-do + sum U32
  (param limit U32)
  (let total U32 0)
  (let index limit)
  (while (> index 2)
    (-= index 1)
  )
  (for (in index (..< 0 limit))
    (+= total index)
  )
  (return (+ total (* index 100)))
)
```

```output
206
```

### A range loop's variable shadows a member of the same name.

```brix
(main
  (print (call (. (call Counter 7) sum) 4))
)

(type + Counter
  (prop + index U32)
  (fn-do + This _
    (param index U32)
  )
  (fn-do + sum U32
    (param limit U32)
    (let total U32 0)
    (for (in index (..< 0 limit))
      (+= total index)
    )
    (return (+ total (* index 100)))
  )
)
```

```output
706
```

### The item of a loop is not known after the loop.

```brix crash
(main
  (for (in i (..< 0 2))
    (print i)
  )
  (print i)
)
```

```output
Exception: Name "i" not found
<no file:6>
      (print i)
             ^
```

### `while` runs its body for as long as its condition holds. `continue` goes to the next pass and `break` leaves the nearest loop.

```brix
(main
  (let a 0)
  (let s 0)
  (while (< a 10)
    (+= a 1)
    (if (== (% a 2) 0)
      (continue)
    )
    (if (> a 7)
      (break)
    )
    (+= s a)
  )
  (print s)
  (print a)
  (let b 0)
  (while (< a 14)
    (= b 0)
    (while true
      (+= b 1)
      (if (> b 2)
        (break)
      )
    )
    (+= a b)
  )
  (print a)
)
```

```output
16
9
15
```

### A `while` tests a text as its condition.

```brix
(main
  (let s "abc")
  (let n 0)
  (while s
    (+= n 1)
    (if (== n 3)
      (= s "")
    )
  )
  (print n)
)
```

```output
3
```

### What follows `break` in its block never runs.

```brix
(main
  (for (in i (..< 0 3))
    (print i)
    (break)
    (print "never")
  )
  (print "end")
)
```

```output
0
end
```

### A loop polls on every pass, however long it runs.

```brix
(main
  (let a 0)
  (while (< a 100000)
    (+= a 1)
  )
  (print a)
  (let s I64 0)
  (for (in i (..< 0 400))
    (for (in j (..< 0 400))
      (+= s j)
    )
  )
  (print s)
)
```

```output
100000
31920000
```

### A local that holds text is kept through the collections a long loop causes.

```brix
(main
  (let s "")
  (for (in i (..< 0 200000))
    (= s (++ "a" "b"))
    (++= s "c")
  )
  (print s)
)
```

```output
abc
```

### A condition that joins texts with && or ||, or that a call makes, is tested as each text is.

```brix
(main
  (let s "a")
  (let t "")
  (let ok true)
  (while (and s ok)
    (print s)
    (= ok false)
  )
  (let n 0)
  (while (and (++ t "x") (< n 2))
    (+= n 1)
  )
  (print n)
  (for (in i (..< 0 3) (when (or s t)))
    (print i)
  )
  (for (in i (..< 0 3) (until (or t (== i 1))))
    (print i)
  )
  (print "done")
)
```

```output
a
2
0
1
2
0
done
```

### A clause that is a text is tested as the text is: when, until and while.

```brix
(main
  (let s "a")
  (let t "")
  (for (in i (..< 0 3) (when t))
    (print i)
  )
  (for (in i (..< 0 3) (when s))
    (print i)
  )
  (for (in i (..< 0 3) (until s))
    (print i)
  )
  (for (in i (..< 0 3) (while t))
    (print i)
  )
  (print "done")
)
```

```output
0
1
2
done
```

### A clause of a loop tests whether an optional holds a value, also one a test found to hold one.

```brix
(main
  (call count 0)
  (call count 5)
  (call count null)
  (call words "")
  (call words "x")
)

(fn-do + count _
  (param value (? Int))
  (if (not value)
    (print "none")
    (return)
  )
  (let turns 0)
  (for (in at-turn (..< 0 3) (while value))
    (+= turns 1)
  )
  (print (str "while " turns))
  (= turns 0)
  (for (in at-turn (..< 0 3) (when value))
    (+= turns 1)
  )
  (print (str "when " turns))
  (= turns 0)
  (for (in at-turn (..< 0 3) (until value))
    (+= turns 1)
  )
  (print (str "until " turns))
)

(fn-do + words _
  (param value (? String))
  (if (not value)
    (return)
  )
  (let turns 0)
  (for (in at-turn (..< 0 3) (while value))
    (+= turns 1)
  )
  (print (str "while " turns))
  (= turns 0)
  (for (in at-turn (..< 0 3) (when value))
    (+= turns 1)
  )
  (print (str "when " turns))
  (= turns 0)
  (for (in at-turn (..< 0 3) (until value))
    (+= turns 1)
  )
  (print (str "until " turns))
)
```

```output
while 3
when 3
until 0
while 3
when 3
until 0
none
while 3
when 3
until 0
while 3
when 3
until 0
```

### A clause of a loop over an optional no test narrowed tests whether it holds a value.

```brix
(main
  (call count 0)
  (call count 5)
  (call count null)
)

(fn-do + count _
  (param value (? Int))
  (let turns 0)
  (for (in at-turn (..< 0 3) (while value))
    (+= turns 1)
  )
  (print (str "while " turns))
  (= turns 0)
  (for (in at-turn (..< 0 3) (when value))
    (+= turns 1)
  )
  (print (str "when " turns))
  (= turns 0)
  (for (in at-turn (..< 0 3) (until value))
    (+= turns 1)
  )
  (print (str "until " turns))
)
```

```output
while 3
when 3
until 0
while 3
when 3
until 0
while 0
when 0
until 3
```

### A loop over a list: of texts, of tuples, and one that is empty.

```brix
(main
  (let names (list "ann" "bob" "cy"))
  (for (in name names)
    (print name)
  )
  (let pairs (list (tuple 1 "one") (tuple 2 "two")))
  (for (in pair pairs)
    (print ([] pair 0))
    (print ([] pair 1))
  )
  (let empty (list Int))
  (for (in item empty)
    (print item)
  )
  (print "done")
)
```

```output
ann
bob
cy
1
one
2
two
done
```

### A loop over a list with `continue` and `break` in its body, and with `when`, `while` and `until` in its header.

```brix
(main
  (let items (list 1 2 3 4 5 6))
  (for (in item items)
    (if (== item 2)
      (continue)
    )
    (if (== item 5)
      (break)
    )
    (print item)
  )
  (for (in item items (when (== (% item 2) 0)))
    (print item)
  )
  (for (in item items (while (< item 3)))
    (print item)
  )
  (for (in item items (until (== item 4)))
    (print item)
  )
)
```

```output
1
3
4
2
4
6
1
2
1
2
3
```

### A list that grows while a loop reads it is read to the length it had, and two loops nested over one list.

```brix
(main
  (let items (list 1 2 3))
  (for (in item items)
    (if (== item 1)
      (+= items 9)
    )
    (print item)
  )
  (print (. items length))
  (let total 0)
  (for (in a items)
    (for (in b items)
      (+= total (* a b))
    )
  )
  (print total)
)
```

```output
1
2
3
4
225
```

### A loop over a list a function is given, over one a property holds, and in a getter.

```brix
(main
  (print (call sum (list 1 2 3)))
  (print (call longest (list "a" "abc" "ab")))
  (let box (call Box (list 4 5)))
  (print (. box total))
  (call (. box show))
)

(fn-do + sum Int
  (param items (list Int))
  (let total 0)
  (for (in item items)
    (+= total item)
  )
  (return total)
)

(fn-do + longest String
  (param names (list String))
  (let best "")
  (for (in name names)
    (if (> (. name length) (. best length))
      (= best name)
    )
  )
  (return best)
)

(type + Box
  (prop + items (list Int))
  (fn-do + This _
    (param items (list Int))
  )
  (fn-expr + total Int :get
    (call sum items)
  )
  (fn-do + show _
    (for (in item items)
      (print item)
    )
  )
)
```

```output
6
abc
9
4
5
```

### A loop over a list of structs, of optionals, of objects and of floats.

```brix
(main
  (let points (list (call Point 1 2) (call Point 3 4)))
  (for (in point points)
    (print (+ (. point x) (. point y)))
  )
  (let maybe (list (? Int)) (list 1 null 3))
  (for (in item maybe)
    (if item
      (print item)
      (else
        (print "none")
      )
    )
  )
  (let things (list (call Thing "a") (call Thing "b")))
  (for (in thing things)
    (print (. thing name))
  )
  (let floats (list 1.5 2.5))
  (for (in f floats)
    (print f)
  )
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)

(type + Thing
  (prop + name String)
  (fn-do + This _
    (param name String)
  )
)
```

```output
3
7
1
none
3
a
b
1.5
2.5
```

### A list emptied while a loop reads it panics at the next read.

```brix crash
(main
  (let items (list 1 2 3))
  (for (in item items)
    (if (== item 2)
      (call (. items clear))
    )
    (print item)
  )
)
```

```output
1
2
```

### A loop over the characters of a text, with `when`, and nested in a loop over a range.

```brix
(main
  (let text "héllo")
  (for (in char text)
    (print char)
  )
  (for (in char text (when (!= char 'l')))
    (print char)
  )
  (let count 0)
  (for (in c "abc")
    (+= count 1)
  )
  (print count)
  (for (in i (..< 0 3))
    (for (in c "xy")
      (print (str i c))
    )
  )
)
```

```output
h
é
l
l
o
h
é
o
3
0x
0y
1x
1y
2x
2y
```

### A loop over what a call gives, over a list written in its header, and over a slice of a list.

```brix
(main
  (let items (list 3 1 2))
  (for (in item (call make))
    (print item)
  )
  (for (in item (list 10 20))
    (print item)
  )
  (for (in item ([] items (..< 1 3)))
    (print item)
  )
)

(fn-do + make (list String)
  (print "made")
  (return (list "x" "y"))
)
```

```output
made
x
y
10
20
1
2
```

### An error raised in the body of a loop over a list leaves the function.

```brix crash
(main
  (call run (list 1 0 3))
)

(fn-do + run _
  (param items (list Int))
  (for (in item items)
    (print item)
    (call check item)
  )
)

(fn-do + check _
  (param item Int)
  (throw (call Error "zero") (if (== item 0)))
)
```

```output
1
0
```

### A loop over a list of unions matched in its body, of texts with a loop over each, and of optional texts.

```brix
(main
  (let shapes (list (call (. Shape circle) 2) (call (. Shape square) 3) (. Shape none)))
  (for (in shape shapes)
    (match shape
      (case (call .circle r)
        (print (str "circle " r))
      )
      (case (call .square side)
        (print (str "square " side))
      )
      (case .none
        (print "none")
      )
    )
  )
  (let words (list "a" "bb"))
  (for (in word words)
    (let doubled (str word word))
    (print doubled)
    (for (in letter doubled)
      (print letter)
    )
  )
  (let opts (list (? String)) (list "x" null))
  (for (in opt opts)
    (print (?? opt "nothing"))
    (if opt
      (print (. opt length))
    )
  )
)

(type + Shape (is Union)
  (variant circle Int)
  (variant square Int)
  (variant none)
)
```

```output
circle 2
square 3
none
aa
a
a
bbbb
b
b
b
b
x
1
nothing
```

### An error a stream raises where a loop begins to read it is a panic at the loop.

```brix crash
(main
  (let bad (of Iterable Int) (call Bad))
  (call show bad)
)

(fn-do + show _
  (param stream (stream Int))
  (for (in item stream)
    (print item)
  )
)

(type + Bad
  (implements (of Iterable Int))
  (fn-expr + (prefix <- this) (? Int)
    null
  )
  (fn-do + begin-iterating _ (?! Error)
    (throw (call Error "no start"))
  )
  (fn-do + end-iterating _ (?! Error)
    (print "ended")
  )
)
```

```stderr
Unhandled panic: no start
<no file:9>

Task 1

Stack trace:
  0  __source__:show no file:9:8
  1  __source__:main no file:4:3
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

### An error a stream raises where a loop has read it to its end is a panic at the loop.

```brix crash
(main
  (let feed (of Iterable Int) (call Feed))
  (call show feed)
)

(fn-do + show _
  (param stream (stream Int))
  (for (in item stream)
    (print item)
  )
  (print "after")
)

(type + Feed
  (implements (of Iterable Int))
  (prop _ left _ (= 3))
  (fn-do + (prefix <- this) (? Int)
    (if (== left 0)
      (return null)
    )
    (-= left 1)
    (return left)
  )
  (fn-do + begin-iterating _ (?! Error)
    (print "begun")
  )
  (fn-do + end-iterating _ (?! Error)
    (print "ended")
    (throw (call Error "no end"))
  )
)
```

```stderr
Unhandled panic: no end
<no file:9>

Task 1

Stack trace:
  0  __source__:show no file:9:8
  1  __source__:main no file:4:3
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

### A loop whose pattern takes a tuple apart: written in parentheses and without, with a type written for a name, and with a name left out.

```brix
(main
  (let pairs (list (tuple 1i32 "a") (tuple 2i32 "b")))
  (for (in (tuple n s) pairs)
    (print n)
    (print s)
  )
  (for (in (tuple n s) pairs)
    (print (str n " " s))
  )
  (for (in (tuple (: n Float) s) pairs)
    (print n)
  )
  (for (in (tuple _ s) pairs)
    (print s)
  )
)
```

```output
1
a
2
b
1 a
2 b
1
2
a
b
```

### A loop that takes apart the pairs of a map, a list of tuples a function is given, and what a generator yields.

```brix
(main
  (let ages (call (of Map String Int)))
  (= ([] ages "ann") 3)
  (= ([] ages "bob") 5)
  (for (in (tuple key value) ages)
    (print (str key " " value))
  )
  (for (in (tuple key value) ages)
    (print key)
  )
  (let total 0)
  (for (in (tuple _ value) ages)
    (+= total value)
  )
  (print total)
  (print (call pairs-of (list (tuple 1 2) (tuple 3 4))))
  (for (in (tuple a b) (call walk))
    (print (+ a b))
  )
)

(fn-do _ pairs-of Int
  (param list (list (tuple Int Int)))
  (let sum 0)
  (for (in (tuple a b) list)
    (+= sum (* a b))
  )
  (return sum)
)

(fn-do _ walk (stream (tuple Int Int))
  (for (in i (.. 1 3))
    (yield (tuple i (* i 10)))
  )
)
```

```output
ann 3
bob 5
ann
bob
8
14
11
22
33
```

### A loop that names fewer fields than the tuple has, one over tuples of three, and one whose field is itself a tuple.

```brix
(main
  (let triples (list (tuple 1 2.5 "x") (tuple 2 3.5 "y")))
  (for (in (tuple a b) triples)
    (print (+ a b))
  )
  (for (in (tuple a b c) triples)
    (print (str a " " b " " c))
  )
  (let kept 0)
  (for (in (tuple (: a Int64) _ c) triples)
    (+= kept a)
    (print c)
  )
  (print kept)
  (let nested (list (tuple (tuple 1 2) 3)))
  (for (in (tuple p q) nested)
    (print p)
    (print q)
  )
)
```

```output
3.5
5.5
1 2.5 x
2 3.5 y
x
y
3
(1, 2)
3
```

### A loop that takes apart tuples of objects and of structs.

```brix
(main
  (let boxes (list (tuple (call Box 1) 2) (tuple (call Box 3) 4)))
  (for (in (tuple box n) boxes)
    (+= (. box size) n)
    (print (. box size))
  )
  (for (in (tuple box n) boxes)
    (print (* (. box size) n))
  )
  (let points (list (tuple (call Point 1 2) "p") (tuple (call Point 3 4) "q")))
  (for (in (tuple point name) points)
    (print (+ (. point x) (. point y)))
    (print name)
  )
)

(type + Box
  (prop + size Int)
  (fn-do + This _
    (param size Int)
  )
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)
```

```output
3
7
6
28
3
p
7
q
```

### A loop that takes a tuple apart with `break` and `continue`, with a type written for a name, and nested in another over a generator.

```brix
(main
  (let total 0)
  (for (in (tuple a b) (call three))
    (+= total a)
    (if (== b "stop")
      (break)
    )
    (if (== a 1)
      (continue)
    )
    (print b)
  )
  (print total)
  (print (call sums (list (tuple 1 2.5) (tuple 2 4.5))))
  (let words (list "one" "two"))
  (for (in (tuple index word) (call tagged words))
    (for (in (tuple inner other) (call tagged words))
      (print (str index inner " " word " " other))
    )
  )
)

(fn-do _ three (stream (tuple Int String))
  (yield (tuple 1 "a"))
  (yield (tuple 2 "b"))
  (yield (tuple 3 "stop"))
  (yield (tuple 4 "never"))
)

(fn-do _ sums Float
  (param rows (list (tuple I32 Float)))
  (let sum 0.0)
  (for (in (tuple (: count Float) each-value) rows)
    (+= sum (* count each-value))
  )
  (return sum)
)

(fn-do _ tagged (stream (tuple Int String))
  (param items (list String))
  (for (in index (..< 0 (. items length)))
    (yield (tuple index ([] items index)))
  )
)
```

```output
b
6
11.5
00 one one
01 one two
10 two one
11 two two
```

### A loop that takes a tuple apart in a function that yields, and one with a clause that tests a name it takes.

```brix
(main
  (let pairs (list (tuple 1 "a") (tuple 2 "b")))
  (for (in (tuple n s) pairs)
    (print (+ n 1))
    (print s)
  )
  (for (in (tuple n s) (call each))
    (print (str n s))
  )
  (for (in (tuple n s) pairs (when (> n 1)))
    (print s)
  )
)

(fn-do _ each (stream (tuple Int String))
  (let pairs (list (tuple 1 "a") (tuple 2 "b")))
  (for (in (tuple n s) pairs)
    (yield (tuple (* n 2) s))
    (yield (tuple (* n 3) (+ s s)))
  )
)
```

```output
2
a
3
b
2a
3aa
4b
6bb
b
```

### Generators of tuples, read by loops that take each apart: one that pairs two lists, one that numbers a list.

```brix
(main
  (for (in (tuple a b) (call zip (list 1 2 3) (list "x" "y" "z")))
    (print (str a b))
  )
  (for (in (tuple index item) (call numbered (list "p" "q")))
    (print (str index ":" item))
  )
)

(fn-do _ zip (stream (tuple Int String))
  (param left (list Int))
  (param right (list String))
  (for (in i (..< 0 (. left length)))
    (let (tuple l r) (tuple ([] left i) ([] right i)))
    (yield (tuple l r))
  )
)

(fn-do _ numbered (stream (tuple Int String))
  (param items (list String))
  (let index 0)
  (for (in item items)
    (yield (tuple index item))
    (+= index 1)
  )
)
```

```output
1x
2y
3z
0:p
1:q
```

### A panic under a loop that takes a tuple apart.

```brix crash
(main
  (let pairs (list (tuple 1 1) (tuple 2 0)))
  (for (in (tuple n d) pairs)
    (print (/ n (call check d)))
  )
)

(fn-do _ check Int
  (param d Int)
  (if (== d 0)
    (throw (call Panic "zero"))
  )
  (return d)
)
```

```stderr
Unhandled panic: zero
<no file:12>

Task 1

Stack trace:
  0  __source__:check no file:12:5
  1  __source__:main no file:5:17
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

### A loop over the characters of a text, and over a map by its entries and by key and value.

```brix
(main
  (for (in ch "héy")
    (print ch)
  )
  (let map (call (of Map String Int)))
  (= ([] map "a") 1)
  (= ([] map "b") 2)
  (for (in entry map)
    (print entry)
  )
  (for (in (tuple key value) map)
    (print (str key "=" value))
  )
)
```

```output
h
é
y
(b, 2)
(a, 1)
b=2
a=1
```

### A loop over an array held in a local reads the local at each item: an array assigned to it in the body is read from there on.

```brix
(main
  (let arr (call (of Array Int) 3))
  (let other (call (of Array Int) 3))
  (= ([] other 0) 7)
  (= ([] other 1) 8)
  (= ([] other 2) 9)
  (for (in a arr)
    (print a)
    (= arr other)
  )
)
```

```output
0
8
9
```

### A loop over a range of floats held in a local steps through the range the local holds at each item.

```brix
(main
  (let r (.. 0.0 1.0 (by: 0.5)))
  (for (in x r)
    (print x)
    (= r (.. 10.0 11.0 (by: 0.5)))
  )
)
```

```output
0
10.5
11
```

### A loop that steps a cursor through a struct sees a field of the struct assigned in its body.

```brix
(main
  (let c (call Cur 2))
  (for (in v c)
    (print v)
    (= (. c limit) 4)
  )
)

(type + Cur (is Struct)
  (implements (of CursorIterable Int Int))
  (prop + limit Int)
  (fn-expr + initial-iterator-state Int :get
    0
  )
  (fn-do + next-iterator (tuple Int (? Int))
    (param state Int)
    (let none (? Int))
    (if (>= state limit)
      (return (tuple state none))
    )
    (return (tuple (+ state 1) (* state 10)))
  )
)
```

```output
0
10
20
30
```

### A loop over a struct that is a property of an object sees the property's field assigned, and the property assigned whole.

```brix
(main
  (let box (call Holder (call Cur 2)))
  (for (in v (. box cur))
    (print v)
    (= (. box cur limit) 4)
  )
  (let other (call Holder (call Cur 2)))
  (for (in v (. other cur))
    (print v)
    (= (. other cur) (call Cur 4))
  )
)

(type + Cur (is Struct)
  (implements (of CursorIterable Int Int))
  (prop + limit Int)
  (fn-expr + initial-iterator-state Int :get
    0
  )
  (fn-do + next-iterator (tuple Int (? Int))
    (param state Int)
    (let none (? Int))
    (if (>= state limit)
      (return (tuple state none))
    )
    (return (tuple (+ state 1) (* state 10)))
  )
)

(type + Holder
  (prop + cur Cur)
  (fn-do + This _
    (param cur Cur)
  )
)
```

```output
0
10
20
30
0
10
20
30
```

### A loop that counts through a tuple reads the tuple assigned in its body, with the length it had when the loop began.

```brix
(main
  (let s (call Span 2))
  (for (in item s)
    (print item)
    (= s (call Span 4))
  )
)

(type + Span (is Tuple)
  (implements (of Indexable Int))
  (prop + size Int)
  (fn-expr + length Size :get
    size
  )
  (fn-expr + (index-get this (param index Index)) Int
    (+ (* index 10) size)
  )
  (fn-expr + (index-get this (param range (of Range Index))) (of Indexable Int)
    this
  )
)
```

```output
2
14
```

### A loop over an array that is an argument reads the argument assigned in its body.

```brix
(main
  (print (call run (call (of Array Int) 3)))
)

(fn-do + run Int
  (param arr (of Array Int))
  (let other (call (of Array Int) 4))
  (= ([] other 1) 8)
  (= ([] other 2) 9)
  (= ([] other 3) 10)
  (let sum 0)
  (for (in a arr)
    (+= sum a)
    (= arr other)
  )
  (return sum)
)
```

```output
17
```

### A loop over a range of whole numbers held in a local is not changed by an assignment to the local.

```brix
(main
  (let r (.. 0 2))
  (for (in x r)
    (print x)
    (= r (.. 10 12))
  )
  (let q (..< 0 3))
  (for (in x q)
    (print x)
    (= q (..< 5 9))
  )
)
```

```output
0
1
2
0
1
2
```

### A loop over an array a class stores, read by its name alone and through `this`, reads the property assigned in its body.

```brix
(main
  (let box (call Box))
  (call (. box run))
  (call (. box swap))
)

(type + Box
  (prop + items _ (= (call make 1)))
  (fn-do + run _
    (for (in item items)
      (print item)
      (= items (call make 7))
    )
  )
  (fn-do + swap _
    (for (in item (. this items))
      (print item)
      (= (. this items) (call make 20))
    )
  )
)

(fn-do + make (of Array Int)
  (param first Int)
  (let made (call (of Array Int) 3))
  (= ([] made 0) first)
  (= ([] made 1) (+ first 1))
  (= ([] made 2) (+ first 2))
  (return made)
)
```

```output
1
8
9
7
21
22
```

### A loop over an array of another object reads the property assigned in its body, and stays with the object it began with where the local is assigned another.

```brix
(main
  (let box (call Box))
  (for (in item (. box items))
    (print item)
    (= (. box items) (call make 7))
  )
  (let second (call Box))
  (for (in item (. second items))
    (print item)
    (= second (call Box))
    (= (. second items) (call make 30))
  )
)

(type + Box
  (prop + items _ (= (call make 1)))
)

(fn-do + make (of Array Int)
  (param first Int)
  (let made (call (of Array Int) 3))
  (= ([] made 0) first)
  (= ([] made 1) (+ first 1))
  (= ([] made 2) (+ first 2))
  (return made)
)
```

```output
1
8
9
1
2
3
```

### A loop over an array that is a field of a struct sees the field assigned, and the struct assigned whole.

```brix
(main
  (let s (call Wrap (call make 1) 0))
  (for (in item (. s items))
    (print item)
    (= (. s items) (call make 7))
  )
  (let t (call Wrap (call make 1) 0))
  (for (in item (. t items))
    (print item)
    (= t (call Wrap (call make 40) 1))
  )
)

(type + Wrap (is Struct)
  (prop + items (of Array Int))
  (prop + tag Int)
)

(fn-do + make (of Array Int)
  (param first Int)
  (let made (call (of Array Int) 3))
  (= ([] made 0) first)
  (= ([] made 1) (+ first 1))
  (= ([] made 2) (+ first 2))
  (return made)
)
```

```output
1
8
9
1
41
42
```

### A loop over an array a closure assigns reads what the closure stored.

```brix
(main
  (call run)
)

(fn-do + run _
  (let arr (call make 1))
  (let set
    (=> () :do
      (block
        (= arr (call make 60))
      )
    )
  )
  (for (in item arr)
    (print item)
    (call set)
  )
)

(fn-do + make (of Array Int)
  (param first Int)
  (let made (call (of Array Int) 3))
  (= ([] made 0) first)
  (= ([] made 1) (+ first 1))
  (= ([] made 2) (+ first 2))
  (return made)
)
```

```output
1
61
62
```

### A loop over an optional argument known to hold an array reads the argument assigned in its body.

```brix
(main
  (call run (call make 1))
)

(fn-do + run _
  (param arr (? (of Array Int)))
  (if arr
    (for (in item arr)
      (print item)
      (= arr (call make 80))
    )
  )
)

(fn-do + make (of Array Int)
  (param first Int)
  (let made (call (of Array Int) 3))
  (= ([] made 0) first)
  (= ([] made 1) (+ first 1))
  (= ([] made 2) (+ first 2))
  (return made)
)
```

```output
1
81
82
```

### A loop over a text or a list held in a local is not changed by an assignment to the local.

```brix
(main
  (let text "abc")
  (for (in ch text)
    (print ch)
    (= text "xyz")
  )
  (let list (list 1 2 3))
  (for (in item list)
    (print item)
    (= list (list 7 8 9))
  )
)
```

```output
a
b
c
1
2
3
```

### In a function that yields, a loop over an array keeps the array it began with.

```brix
(main
  (for (in v (call gen))
    (print v)
  )
)

(fn-do + gen (stream Int)
  (let arr (call make 1))
  (for (in item arr)
    (yield item)
    (= arr (call make 90))
  )
)

(fn-do + make (of Array Int)
  (param first Int)
  (let made (call (of Array Int) 3))
  (= ([] made 0) first)
  (= ([] made 1) (+ first 1))
  (= ([] made 2) (+ first 2))
  (return made)
)
```

```output
1
2
3
```

### A loop in a closure's body over the closure's own array reads the local assigned in its body.

```brix
(main
  (let run
    (=> () :do
      (block
        (let arr (call make 1))
        (for (in item arr)
          (print item)
          (= arr (call make 100))
        )
      )
    )
  )
  (call run)
)

(fn-do + make (of Array Int)
  (param first Int)
  (let made (call (of Array Int) 3))
  (= ([] made 0) first)
  (= ([] made 1) (+ first 1))
  (= ([] made 2) (+ first 2))
  (return made)
)
```

```output
1
101
102
```

### A loop over an array changed in place, and over an array that is a field of a tuple assigned whole in the body.

```brix
(main
  (let arr (call make 1))
  (for (in item arr)
    (print item)
    (= ([] arr 2) 99)
  )
  (let pair (tuple (call make 1) 5))
  (for (in item ([] pair 0))
    (print item)
    (= pair (tuple (call make 30) 6))
  )
)

(fn-do + make (of Array Int)
  (param first Int)
  (let made (call (of Array Int) 3))
  (= ([] made 0) first)
  (= ([] made 1) (+ first 1))
  (= ([] made 2) (+ first 2))
  (return made)
)
```

```output
1
2
99
1
31
32
```

### A loop over a cast reads the array the cast gives.

```brix
(main
  (let text "a\nb")
  (let count 0)
  (for (in byte (as text ([] Byte)))
    (if (== byte 10u8)
      (+= count 1)
    )
  )
  (print count)
)
```

```output
1
```

### Loops over arrays one in another, each assigned in the inner body.

```brix
(main
  (let arr (call make 1))
  (let other (call make 10))
  (for (in item arr (when (> item 0)))
    (print item)
    (for (in inner other)
      (print inner)
      (= other (call make 20))
    )
    (= arr other)
  )
)

(fn-do + make (of Array Int)
  (param first Int)
  (let made (call (of Array Int) 3))
  (= ([] made 0) first)
  (= ([] made 1) (+ first 1))
  (= ([] made 2) (+ first 2))
  (return made)
)
```

```output
1
10
21
22
21
20
21
22
22
20
21
22
```

### A loop over an array of an object with `continue` and `break`, refilled by a method in its body, and loops over a property of an object a call makes, which is called once.

```brix
(main
  (let box (call Box))
  (for (in item (. box items))
    (if (== item 9)
      (continue)
    )
    (print item)
    (call (. box refill) 8)
    (if (== item 10)
      (break)
    )
  )
  (for (in item (. (call fresh) items))
    (print item)
  )
  (for (in item (. (call fresh) wrap items))
    (print item)
  )
)

(type + Box
  (prop + items _ (= (call make 1)))
  (prop + wrap _ (= (call Wrap (call make 100) 0)))
  (fn-do + refill _
    (param first Int)
    (= items (call make first))
  )
)

(type + Wrap (is Struct)
  (prop + items (of Array Int))
  (prop + tag Int)
)

(fn-do + fresh Box
  (print "made a box")
  (return (call Box))
)

(fn-do + make (of Array Int)
  (param first Int)
  (let made (call (of Array Int) 3))
  (= ([] made 0) first)
  (= ([] made 1) (+ first 1))
  (= ([] made 2) (+ first 2))
  (return made)
)
```

```output
1
10
made a box
1
2
3
made a box
100
101
102
```

### A loop over an array of an object the local no longer names, while its body allocates enough for collections to run.

```brix
(main
  (let second (call Box))
  (let total 0)
  (for (in item (. second items))
    (+= total item)
    (= second (call Box))
    (= (. second items) (call make 30))
    (for (in round (..< 0 2000))
      (let junk (list round item total))
      (+= total (- (. junk length) 3))
    )
    (call force-collection)
  )
  (print total)
)

(fn-do + force-collection _
  (let keep (list (list Int)))
  (for (in n (..< 0 3000))
    (+= keep (list n n n n))
  )
)

(type + Box
  (prop + items _ (= (call make 1)))
  (prop + wrap _ (= (call Wrap (call make 100) 0)))
  (fn-do + refill _
    (param first Int)
    (= items (call make first))
  )
)

(type + Wrap (is Struct)
  (prop + items (of Array Int))
  (prop + tag Int)
)

(fn-do + fresh Box
  (print "made a box")
  (return (call Box))
)

(fn-do + make (of Array Int)
  (param first Int)
  (let made (call (of Array Int) 3))
  (= ([] made 0) first)
  (= ([] made 1) (+ first 1))
  (= ([] made 2) (+ first 2))
  (return made)
)
```

```output
6
```

### A loop that names the fields of pairs in an array, with a clause, reads the array assigned in its body.

```brix
(main
  (let pairs (call (of Array (tuple Int String)) 2))
  (= ([] pairs 0) (tuple 1 "one"))
  (= ([] pairs 1) (tuple 2 "two"))
  (let other (call (of Array (tuple Int String)) 2))
  (= ([] other 0) (tuple 7 "seven"))
  (= ([] other 1) (tuple 8 "eight"))
  (for (in (tuple number name) pairs (when (> number 0)))
    (print (str number " " name))
    (= pairs other)
  )
)
```

```output
1 one
8 eight
```

### A closure in a function that yields loops over an array of that function and reads it assigned.

```brix
(main
  (for (in v (call gen))
    (print v)
  )
)

(fn-do + gen (stream Int)
  (let arr (call make 1))
  (let each-one
    (=> () :do
      (block
        (let total 0)
        (for (in item arr)
          (+= total item)
          (= arr (call make 10))
        )
        (return total)
      )
    )
  )
  (yield (call each-one))
  (yield ([] arr 0))
)

(fn-do + make (of Array Int)
  (param first Int)
  (let made (call (of Array Int) 3))
  (= ([] made 0) first)
  (= ([] made 1) (+ first 1))
  (= ([] made 2) (+ first 2))
  (return made)
)
```

```output
24
10
```

### A loop over a lazy local that one read evaluates keeps what the read gave: the body assigns the local it was declared with, and the loop does not see it.

```brix
(main
  (call run)
)

(fn-do + run _
  (let other (call make 1))
  (lazy arr other)
  (for (in item arr)
    (print item)
    (= other (call make 70))
  )
)

(fn-do + make (of Array Int)
  (param first Int)
  (let made (call (of Array Int) 3))
  (= ([] made 0) first)
  (= ([] made 1) (+ first 1))
  (= ([] made 2) (+ first 2))
  (return made)
)
```

```output
1
2
3
```

### A loop over a lazy local the body assigns reads what was assigned at the next item.

```brix
(main
  (call run)
)

(fn-do + run _
  (lazy arr (call make 1))
  (for (in item arr)
    (print item)
    (= arr (call make 70))
  )
  (print ([] arr 0))
)

(fn-do + make (of Array Int)
  (param first Int)
  (let made (call (of Array Int) 3))
  (= ([] made 0) first)
  (= ([] made 1) (+ first 1))
  (= ([] made 2) (+ first 2))
  (return made)
)
```

```output
1
71
72
70
```

### A loop over a lazy local declared in a loop and read twice keeps the value its first read stored.

```brix
(main
  (call run)
)

(fn-do + run _
  (for (in turn (..< 0 2))
    (let other (call make (* turn 10)))
    (lazy arr other)
    (for (in item arr)
      (print item)
      (= other (call make 70))
    )
    (print ([] arr 0))
  )
)

(fn-do + make (of Array Int)
  (param first Int)
  (let made (call (of Array Int) 3))
  (= ([] made 0) first)
  (= ([] made 1) (+ first 1))
  (= ([] made 2) (+ first 2))
  (return made)
)
```

```output
0
1
2
0
10
11
12
10
```

### A loop over a lazy local a closure assigns reads what the closure stored.

```brix
(main
  (call run)
)

(fn-do + run _
  (lazy arr (call make 1))
  (let change (=> () (= arr (call make 70))))
  (for (in item arr)
    (print item)
    (call change)
  )
)

(fn-do + make (of Array Int)
  (param first Int)
  (let made (call (of Array Int) 3))
  (= ([] made 0) first)
  (= ([] made 1) (+ first 1))
  (= ([] made 2) (+ first 2))
  (return made)
)
```

```output
1
71
72
```

### A loop over a lazy local declared as a property of an object keeps the value: the body assigns the property, and the loop does not see it.

```brix
(main
  (call run)
)

(fn-do + run _
  (let holder (call Holder))
  (lazy arr (. holder items))
  (for (in item arr)
    (print item)
    (= (. holder items) (call make 70))
  )
)

(type + Holder
  (prop + items _ (= (call make 1)))
)

(fn-do + make (of Array Int)
  (param first Int)
  (let made (call (of Array Int) 3))
  (= ([] made 0) first)
  (= ([] made 1) (+ first 1))
  (= ([] made 2) (+ first 2))
  (return made)
)
```

```output
1
2
3
```

### A loop over a lazy range of numbers the body assigns keeps the range it began with.

```brix
(main
  (call run)
)

(fn-do + run _
  (lazy r (..< 0 3))
  (for (in x r)
    (print x)
    (= r (..< 5 9))
  )
  (print (. r start))
)
```

```output
0
1
2
5
```

### A loop over a lazy local read again after it evaluates the value once.

```brix
(main
  (call run)
)

(fn-do + run _
  (let count 0)
  (lazy arr (call noisy 1))
  (for (in item arr)
    (print item)
  )
  (print ([] arr 1))
)

(fn-do + noisy (of Array Int)
  (param first Int)
  (print "made")
  (return (call make first))
)

(fn-do + make (of Array Int)
  (param first Int)
  (let made (call (of Array Int) 3))
  (= ([] made 0) first)
  (= ([] made 1) (+ first 1))
  (= ([] made 2) (+ first 2))
  (return made)
)
```

```output
made
1
2
3
2
```

### A loop over a lazy local declared in a loop evaluates the value once in each turn.

```brix
(main
  (call run)
)

(fn-do + run _
  (for (in turn (..< 0 2))
    (lazy arr (call noisy (* turn 10)))
    (for (in item arr)
      (print item)
    )
    (print ([] arr 1))
  )
)

(fn-do + noisy (of Array Int)
  (param first Int)
  (print "made")
  (return (call make first))
)

(fn-do + make (of Array Int)
  (param first Int)
  (let made (call (of Array Int) 3))
  (= ([] made 0) first)
  (= ([] made 1) (+ first 1))
  (= ([] made 2) (+ first 2))
  (return made)
)
```

```output
made
0
1
2
1
made
10
11
12
11
```

### A lazy local whose value panics is evaluated where a loop over it begins, and the program ends by the panic.

```brix crash
(main
  (call run)
)

(fn-do + run _
  (lazy arr (call boom 1))
  (for (in item arr)
    (print item)
  )
  (print ([] arr 0))
)

(fn-do + boom (of Array Int)
  (param first Int)
  (let items (list 1 2 3))
  (print ([] items 5))
  (return (call make first))
)

(fn-do + make (of Array Int)
  (param first Int)
  (let made (call (of Array Int) 3))
  (= ([] made 0) first)
  (= ([] made 1) (+ first 1))
  (= ([] made 2) (+ first 2))
  (return made)
)
```

### A loop over a lazy local a closure reads evaluates the value once, for the loop and the closure.

```brix
(main
  (call run)
)

(fn-do + run _
  (lazy arr (call noisy 1))
  (let peek (=> () ([] arr 1)))
  (for (in item arr)
    (print item)
    (print (call peek))
  )
  (print (call peek))
)

(fn-do + noisy (of Array Int)
  (param first Int)
  (print "made")
  (return (call make first))
)

(fn-do + make (of Array Int)
  (param first Int)
  (let made (call (of Array Int) 3))
  (= ([] made 0) first)
  (= ([] made 1) (+ first 1))
  (= ([] made 2) (+ first 2))
  (return made)
)
```

```output
made
1
2
2
2
3
2
2
```

### A loop over a lazy local read before it finds the value evaluated, and reads what its body assigns.

```brix
(main
  (call run)
)

(fn-do + run _
  (lazy arr (call noisy 1))
  (print ([] arr 0))
  (for (in item arr)
    (print item)
    (= arr (call make 30))
  )
  (print ([] arr 0))
)

(fn-do + noisy (of Array Int)
  (param first Int)
  (print "made")
  (return (call make first))
)

(fn-do + make (of Array Int)
  (param first Int)
  (let made (call (of Array Int) 3))
  (= ([] made 0) first)
  (= ([] made 1) (+ first 1))
  (= ([] made 2) (+ first 2))
  (return made)
)
```

```output
made
1
1
31
32
30
```

### A loop over a lazy local whose body reads and assigns it.

```brix
(main
  (call run)
)

(fn-do + run _
  (lazy arr (call make 1))
  (for (in item arr)
    (print item)
    (print ([] arr 2))
    (if (== item 1)
      (= arr (call make 30))
    )
  )
)

(fn-do + make (of Array Int)
  (param first Int)
  (let made (call (of Array Int) 3))
  (= ([] made 0) first)
  (= ([] made 1) (+ first 1))
  (= ([] made 2) (+ first 2))
  (return made)
)
```

```output
1
3
31
32
32
32
```

### A loop over a lazy optional local a test found a value in reads what its body assigns.

```brix
(main
  (call run)
)

(fn-do + run _
  (lazy arr (? (of Array Int)) (call make 1))
  (if arr
    (for (in item arr)
      (print item)
      (= arr (call make 80))
    )
  )
)

(fn-do + make (of Array Int)
  (param first Int)
  (let made (call (of Array Int) 3))
  (= ([] made 0) first)
  (= ([] made 1) (+ first 1))
  (= ([] made 2) (+ first 2))
  (return made)
)
```

```output
1
81
82
```

### Two loops over one lazy local, one in the other, each read what the outer body assigns.

```brix
(main
  (call run)
)

(fn-do + run _
  (lazy arr (call make 1))
  (for (in item arr)
    (for (in other arr)
      (print (+ (* item 100) other))
    )
    (= arr (call make 5))
  )
)

(fn-do + make (of Array Int)
  (param first Int)
  (let made (call (of Array Int) 3))
  (= ([] made 0) first)
  (= ([] made 1) (+ first 1))
  (= ([] made 2) (+ first 2))
  (return made)
)
```

```output
101
102
103
605
606
607
705
706
707
```

### A loop over a lazy local declared as a property keeps its value where the property is assigned, and reads what is assigned to the local.

```brix
(main
  (let box (call Box))
  (call (. box run))
)

(type + Box
  (prop + items _ (= (call make 1)))
  (fn-do + run _
    (lazy mine items)
    (for (in item mine)
      (print item)
      (= items (call make 20))
    )
    (lazy kept items)
    (for (in item kept)
      (print item)
      (= kept (call make 90))
    )
  )
)

(fn-do + make (of Array Int)
  (param first Int)
  (let made (call (of Array Int) 3))
  (= ([] made 0) first)
  (= ([] made 1) (+ first 1))
  (= ([] made 2) (+ first 2))
  (return made)
)
```

```output
1
2
3
20
91
92
```

### A loop over a lazy local a branch may have read evaluates the value where it was not.

```brix
(main
  (call run true)
  (call run false)
)

(fn-do + run _
  (param early Bool)
  (lazy arr (call noisy 1))
  (if early
    (print ([] arr 0))
  )
  (for (in item arr)
    (print item)
    (= arr (call make 30))
  )
)

(fn-do + noisy (of Array Int)
  (param first Int)
  (print "made")
  (return (call make first))
)

(fn-do + make (of Array Int)
  (param first Int)
  (let made (call (of Array Int) 3))
  (= ([] made 0) first)
  (= ([] made 1) (+ first 1))
  (= ([] made 2) (+ first 2))
  (return made)
)
```

```output
made
1
1
31
32
made
1
31
32
```

### A second header with a clause of its own, over a list and a range

```brix
(main
  (let names (list "a" "b" "c"))
  (for (in name names (when (!= name "b")))
    (and-for (in count (.. 5 9) (when (!= count 6)))
      (print (str name " " count))
    )
  )
)
```

```output
a 5
c 7
```

### Three headers with continue and break, a range that counts down, and clauses that end the loop

```brix
(main
  (for (in i (..< 0 6))
    (and-for (in j (..< 10 20)))
    (and-for (in k (list "a" "b" "c" "d" "e"))
      (if (== i 1)
        (continue)
      )
      (if (== i 4)
        (break)
      )
      (print (str i " " j " " k))
    )
  )
  (print "done")
  (for (in i (.. 5 1))
    (and-for (in j (.. 1 3))
      (print (tuple i j))
    )
  )
  (for (in i (..< 0 4) (until (== i 2)))
    (and-for (in j (..< 0 9) (while (< j 5)))
      (print (tuple i j))
    )
  )
)
```

```output
0 10 a
2 12 c
3 13 d
done
(5, 1)
(4, 2)
(3, 3)
(0, 0)
(1, 1)
```

### What a loop of two headers reads of its containers, and in what order

```brix
(main
  (let loud (call Loud "a" (list 1 2 3)))
  (let other (call Loud "b" (list 10 20 30 40)))
  (for (in x loud)
    (and-for (in y other (when (!= y 20)))
      (print (str x " " y))
    )
  )
  (print "end")
)

(type + Loud
  (implements (of Indexable Int))
  (prop _ name String)
  (prop _ values (of List Int))
  (fn-do + This _
    (param name _)
    (param items (list Int))
    (= values items)
  )
  (fn-expr + length Size :get
    (before
      (print (str name " length"))
    )
    (. values length)
  )
  (fn-expr + (index-get this (param index Index)) Int
    (before
      (print (str name " read " index))
    )
    ([] values index)
  )
  (fn-expr + (index-get this (param range (of Range Index))) Loud
    (call Loud name ([] values range))
  )
)
```

```output
a length
b length
a read 0
b read 0
1 10
a read 1
b read 1
b read 2
2 30
a read 2
b read 3
3 40
end
```

### A loop of two headers over streams begins and ends each in order

```brix
(main
  (let first (call Counter "a" 3))
  (let second (call Counter "b" 5))
  (for (in x first)
    (and-for (in y second)
      (print (str x " " y))
      (if (== x 2)
        (break)
      )
    )
  )
  (print "after")
  (for (in x (call Counter "c" 2))
    (and-for (in y (list 7 8 9))
      (print (str x " " y))
    )
  )
  (print "end")
)

(type + Counter
  (implements (of Iterable Int))
  (prop _ name String)
  (prop _ limit Int)
  (prop _ at _ (= 0))
  (fn-do + This _
    (param name _)
    (param limit _)
  )
  (fn-do + begin-iterating _
    (print (str name " begins"))
  )
  (fn-do + end-iterating _
    (print (str name " ends"))
  )
  (fn-do + (prefix <- this) _
    (if (< at limit)
      (+= at 1)
      (return at)
    )
  )
)
```

```output
a begins
b begins
1 1
2 2
a ends
b ends
after
c begins
1 7
2 8
c ends
end
```

### Closures, a tuple taken apart and the item unnamed in loops of two headers

```brix
(main
  (let made (call (of List (of Getter String))))
  (for (in name (list "a" "b" "c"))
    (and-for (in count (list 1 2 3) (when (!= count 2)))
      (+= made (=> () (str name count)))
    )
  )
  (for (in make made)
    (print (call make))
  )
  (let pairs (list (tuple 1 "x") (tuple 2 "y")))
  (for (in letter (list "p" "q"))
    (and-for (in (tuple number word) pairs)
      (print (str letter " " number " " word))
    )
  )
  (for (in (list "u" "v"))
    (and-for (in n (..< 0 5))
      (print (str each " " it " " n))
    )
  )
)
```

```output
a1
b3
p 1 x
q 2 y
u u 0
v v 1
```

### Loops that match pass over what does not fit: a class, an optional, a tuple, a list

```brix
(main
  (let mixed (list Shape) (list (as (call Circle 1) Shape) (as (call Square 2) Shape) (as (call Circle 3) Shape)))
  (for (in (| (: c Circle)) mixed)
    (print (. c radius))
  )
  (for (in (| (: s Square)) mixed (when (> (. s side) 1)))
    (print (. s side))
  )
  (let maybe (list (? Int)) (list 1 null 3))
  (for (in (| (: n Int)) maybe)
    (print n)
  )
  (let pairs (list (tuple 1 "a") (tuple 2 "b") (tuple 3 "c")))
  (for (in (| (tuple (: n ({} (!= it 2))) word)) pairs)
    (print (str n " " word))
  )
  (let lists (list (list 1 2) (list 3) (call (of List Int)) (list 4 5 6)))
  (for (in (| (list first ...rest)) lists)
    (print (str first " " (. rest length)))
  )
  (for (in (| (list _ second ...)) lists)
    (and-for (in tag (list "x" "y" "z"))
      (print (str second " " tag))
    )
  )
)

(type + Shape (is Interface))

(type + Circle
  (implements Shape)
  (prop + radius Int)
  (fn-do + This _
    (param radius _)
  )
)

(type + Square
  (implements Shape)
  (prop + side Int)
  (fn-do + This _
    (param side _)
  )
)
```

```output
1
3
2
1
3
1 a
3 c
1 1
3 0
4 2
2 x
5 y
```

### Loops of two headers and loops that match in functions that yield

```brix
(main
  (for (in total (call sums (list 1 2 3) (list 10 20 30 40)))
    (print total)
  )
  (for (in word (call picked (list "a" "bb" "ccc" "dd")))
    (print word)
  )
  (print (call (. ", " join) (in (* each 2) (list 1 2 3) (when (!= each 2)))))
)

(fn-do + sums (stream Int)
  (param left (list Int))
  (param right (list Int))
  (for (in a left)
    (and-for (in b right (when (!= b 20)))
      (yield (+ a b))
    )
  )
)

(fn-do + picked (stream String)
  (param words (list String))
  (for (in (| (: word ({} (== (. it length) 2)))) words)
    (yield word)
  )
)
```

```output
11
32
43
bb
dd
2, 6
```

### Closures made in a loop of two headers and in a loop that matches keep their turn

```brix
(main
  (let seen (call (of List String)))
  (let callbacks (call (of List Doer)))
  (for (in item (list "a" "b" "c"))
    (and-for (in size (list 1 2 3))
      (+= callbacks (=> () :do (+= seen (str item size))))
    )
  )
  (for (in callback callbacks)
    (call callback)
  )
  (print (call (. ", " join) seen))
  (let shapes (list Shape) (list (as (call Circle 1) Shape) (as (call Square 2) Shape) (as (call Circle 3) Shape)))
  (let getters (call (of List (of Getter Int))))
  (for (in (| (: c Circle)) shapes)
    (+= getters (=> () (. c radius)))
  )
  (for (in getter getters)
    (print (call getter))
  )
)

(type + Shape (is Interface))

(type + Circle
  (implements Shape)
  (prop + radius Int)
  (fn-do + This _
    (param radius _)
  )
)

(type + Square
  (implements Shape)
  (prop + side Int)
  (fn-do + This _
    (param side _)
  )
)
```

```output
a1, b2, c3
1
3
```

### A header over nothing ends a loop of two, and headers of other integer types

```brix
(main
  (let empty (call (of List Int)))
  (for (in a empty)
    (and-for (in b (list 1 2))
      (print "never")
    )
  )
  (for (in a (list 1 2))
    (and-for (in b empty)
      (print "never")
    )
  )
  (let words (list "x" "y"))
  (for (in (: i U8) (..< 0 3))
    (and-for (in (: j I64) (.. 5 7))
      (print (tuple (+ i 0) j))
    )
  )
  (for (in word words)
    (and-for (in index (..< 0 (. words length)))
      (print (str index " " word))
    )
  )
  (print "end")
)
```

```output
(0, 5)
(1, 6)
(2, 7)
0 x
1 y
end
```

### The trace of a cast in a loop to a class an item is not of

```brix crash
(main
  (for (in item (list (call Foo2) (call Foo) (call Foo2)))
    (print (as item Foo2))
  )
)

(blank)
(type-line + Foo)
(type-line + Foo2 (is Foo))
```

```stderr
Unhandled panic: Invalid cast to [Object __source__:Foo2]
<no file:4>

Task 1

Stack trace:
  0  __source__:main no file:4:12
  1  __source__:init.__source__ no file
  2  __source__:brix.main no file
```

### A comprehension that keeps the items a pattern fits gives the items themselves

```brix
(main
  (let odd (case-each (in ({} (% it 2)) (..< 0 10))))
  (print (type-of odd))
  (print (call (. ", " join) odd))
  (let big (case-each (in ({} (> it 2)) (list 1 2 3 4))))
  (print (type-of big))
  (for (in item big)
    (print item)
  )
  (let words (case-each (in ({} (> (. it length) 1)) (list "a" "bb" "" "ccc"))))
  (print (type-of words))
  (print (call (. "/" join) words))
  (let none (case-each (in ({} (> it 9)) (list 1 2 3))))
  (print (call (. ", " join) none))
  (print "end")
)
```

```output
[TypeInfo Generator'I64]
1, 3, 5, 7, 9
[TypeInfo Generator'I64]
3
4
[TypeInfo Generator'String]
bb/ccc

end
```

### A comprehension that keeps what fits, written over lines, in a function that yields, and read by a loop

```brix
(main
  (let kept
    (iterate (in (list 5 1 7 2 9))
      (block
        (<* ({} (> it 4)))
      )
    )
  )
  (print (type-of kept))
  (let total 0)
  (for (in item kept)
    (+= total item)
  )
  (print total)
  (let names
    (iterate (in (list "ann" "bo" "cy" "dee"))
      (block
        (<* ({} (== (. it length) 3)))
      )
    )
  )
  (print (call (. ", " join) names))
  (print (call (. ", " join) (call picked (list 3 8 4 10) 5)))
)

(fn-do + picked (stream Int)
  (param items (list Int))
  (param floor Int)
  (let passing (case-each (in ({} (> it floor)) items)))
  (for (in item passing)
    (yield (* item 2))
  )
)
```

```output
[TypeInfo Generator'I64]
21
ann, dee
16, 20
```

### Loop over a base class begins and ends by the derived class

```brix
(main
  (let base Base (call Derived))
  (for (in a base)
    (print a)
  )
)

(type + Base
  (implements (of Iterable Int))
  (prop _ at _ (= 0))
  (fn-do + begin-iterating _
    (print "base begins")
  )
  (fn-do + end-iterating _
    (print "base ends")
  )
  (fn-do + (prefix <- this) (? Int)
    (if (< at 2)
      (+= at 1)
      (return at)
    )
    (return null)
  )
)

(type + Derived (is Base)
  (fn-do + begin-iterating _
    (print "derived begins")
  )
  (fn-do + end-iterating _
    (print "derived ends")
  )
)
```

```output
derived begins
1
2
derived ends
```

### Cursor loop over a base class steps by the derived class

```brix
(main
  (let base Base (call Derived))
  (for (in a base)
    (print a)
  )
)

(type + Base
  (implements (of CursorIterable Int Int))
  (fn-expr + initial-iterator-state Int :get
    0
  )
  (fn-do + next-iterator (tuple Int (? Int))
    (param state Int)
    (let none (? Int))
    (if (>= state 2)
      (return (tuple state none))
    )
    (return (tuple (+ state 1) (+ state 1)))
  )
)

(type + Derived (is Base)
  (fn-do + next-iterator (tuple Int (? Int))
    (param state Int)
    (let none (? Int))
    (if (>= state 3)
      (return (tuple state none))
    )
    (return (tuple (+ state 1) (* state 100)))
  )
)
```

```output
0
100
200
```

### Index loop over a base class reads by the derived class

```brix
(main
  (let base Base (call Derived))
  (for (in a base)
    (print a)
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
5
105
```

### each and it are the first header's item

```brix
(main
  (for (in (list "u" "v"))
    (and-for (in (list 1 2))
      (print (str each " " it))
    )
  )
)
```

```output
u u
v v
```

### each and it of the first of three headers

```brix
(main
  (for (in (list "u" "v"))
    (and-for (in b (list 1 2)))
    (and-for (in (list true false))
      (print (str each " " it " " b))
    )
  )
)
```

```output
u u 1
v v 2
```

### A panic under the read of a loop's second header is placed at the first header, where the loop reads every header.

```brix crash
(main
  (call drain)
)

(fn-do + a _
  (yield 1)
  (yield 2)
  (yield 3)
)

(fn-do + b _
  (yield 10)
  (throw (call Panic "zip panic"))
)

(fn-do + drain _
  (for (in x (call a))
    (and-for (in y (call b))
      (print (+ x y))
    )
  )
)
```

```stderr
Unhandled panic: zip panic
<no file:14>

Task 1

Stack trace:
  0  __source__:b no file:14:3
  1  core/stream:Generator:<-read core/stream.bx
  2  __source__:drain no file:18:8
  3  __source__:main no file:3:3
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### An error raised under the read of a loop's second header leaves the loop at the first header.

```brix crash
(main
  (call drain)
)

(type + Problem (is Error))

(fn-do + b _
  (yield 10)
  (throw (call Problem "zip same line"))
)

(fn-do + drain _
  (for (in x (..< 0 5))
    (and-for (in y (call b))
      (print (+ x y))
    )
  )
)
```

```stderr
Unhandled error: zip same line
<no file:10>

Task 1

Stack trace:
  0  __source__:b no file:10:3
  1  core/stream:Generator:<-read core/stream.bx
  2  __source__:drain no file:14:8
  3  __source__:main no file:3:3
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### A generator read by a loop's second header that panics on a bad index is placed at the first header.

```brix crash
(main
  (call drain)
)

(fn-do + b _
  (yield 10)
  (let list (list 1))
  (yield ([] list 3))
)

(fn-do + drain _
  (for (in x (..< 0 5))
    (and-for (in y (call b))
      (print (+ x y))
    )
  )
)
```

```stderr
Unhandled panic: List index 3 out of range
<core/list.bx>

Task 1

Stack trace:
  0  core/list:List:absent core/list.bx
  1  __source__:b no file:9:10
  2  core/stream:Generator:<-read core/stream.bx
  3  __source__:drain no file:13:8
  4  __source__:main no file:3:3
  5  __source__:init.__source__ no file
  6  __source__:brix.main no file
```

```brix
(main
  (let input (list 1 2 3))
  (let items-0 (list Int) (case-each (in value input (when (> value 0)))))
  (print (. items-0 length))
  (let items-1 (list Int) (case-each (in value input (when (> value 1)))))
  (print (. items-1 length))
  (let items-2 (list Int) (case-each (in value input (when (> value 2)))))
  (print (. items-2 length))
  (let items-3 (list Int) (case-each (in value input (when (> value 3)))))
  (print (. items-3 length))
)
```

```output
3
2
1
0
```

```brix
(main
  (call sequence (list 1 2 3))
)

(fn-do + sequence _
  (param input (list Int))
  (let items-0 (list Int)
    (case-each (in value input)
      (call checked value)
    )
  )
  (print (. items-0 length))
  (let items-1 (list Int)
    (case-each (in value input)
      (call checked value)
    )
  )
  (print (. items-1 length))
  (let items-2 (list Int)
    (case-each (in value input)
      (call checked value)
    )
  )
  (print (. items-2 length))
  (let items-3 (list Int)
    (case-each (in value input)
      (call checked value)
    )
  )
  (print (. items-3 length))
)

(fn-do + checked Int
  (param value Int)
  (if (< value 0)
    (throw (call Error "negative"))
  )
  (return value)
)
```

```output
3
3
3
3
```

```brix crash
(main
  (call sequence (list -1))
)

(fn-do + sequence _
  (param input (list Int))
  (let items-0 (list Int)
    (case-each (in value input)
      (call checked value)
    )
  )
  (print (. items-0 length))
  (let items-1 (list Int)
    (case-each (in value input)
      (call checked value)
    )
  )
  (print (. items-1 length))
  (let items-2 (list Int)
    (case-each (in value input)
      (call checked value)
    )
  )
  (print (. items-2 length))
  (let items-3 (list Int)
    (case-each (in value input)
      (call checked value)
    )
  )
  (print (. items-3 length))
)

(fn-do + checked Int
  (param value Int)
  (if (< value 0)
    (throw (call Error "negative"))
  )
  (return value)
)
```

```stderr
Unhandled error: negative
<no file:37>

Task 1

Stack trace:
  0  __source__:checked no file:37:5
  1  __source__:sequence no file:10:7
  2  __source__:main no file:3:3
  3  __source__:init.__source__ no file
  4  __source__:brix.main no file
```

```brix
(main
  (for (in i (..< 0 5))
    (let v
      (match i
        (case= 1
          (print "skip")
          (continue)
        )
        (case= 3
          (break)
        )
        (default
          (* i 10)
        )
      )
    )
    (print v)
  )
)
```

```output
0
skip
20
```

### A loop's pattern that names `each` binds the name itself, not the loop's item.

```brix
(main
  (let pairs (list (tuple String Stored)))
  (+= pairs (tuple "a" (call Stored "first" 1)))
  (for (in (tuple key each) pairs)
    (print (str key " " (. each label) " " (. each count)))
  )
  (let stored (map String Stored))
  (= ([] stored "b") (call Stored "second" 2))
  (for (in (tuple key each) stored)
    (print (str key " " (. each label)))
  )
)

(type + Stored (is Tuple)
  (prop + label String)
  (prop + count I64)
)
```

```output
a first 1
b second
```

### The clauses of a loop are tested in the order written, and the first that decides wins

```brix
(main
  (for (in i (..< 0 10) (when (== (% i 2) 0)) (while (< i 7)))
    (print (str "range " i))
  )
  (for (in i (..< 0 6) (when (!= i 2)) (until (== i 2)))
    (print (str "skip " i))
  )
  (let names (list "a" "bb" "ccc" "dddd" "e"))
  (for (in name names (while (< (. name length) 4)) (when (!= (. name length) 2)))
    (print (str "list " name))
  )
)
```

```output
range 0
range 2
range 4
range 6
skip 0
skip 1
skip 3
skip 4
skip 5
list a
list ccc
```

### A `break` outside a loop is an error

```brix crash
(main
  (break)
  (print 1)
)
```

```output
Exception: `break` outside a loop
<no file:3>
      (break)
      ^^^^^^^
```

### A `continue` outside a loop is an error

```brix crash
(main
  (print (call f))
)

(fn-do + f Int
  (continue)
  (return 7)
)
```

```output
Exception: `continue` outside a loop
<no file:7>
      (continue)
      ^^^^^^^^^^
```

### A function written in a loop is a function: a `break` in it is outside a loop

```brix crash
(main
  (for (in i (..< 0 3))
    (let leave
      (=> () :do
        (block
          (break)
        )
      )
    )
    (call leave)
  )
)
```

```output
Exception: `break` outside a loop
<no file:7>
              (break)
              ^^^^^^^
```
