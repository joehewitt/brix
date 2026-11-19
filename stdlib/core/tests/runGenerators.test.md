# Generators

```brix
(main
  (let gen (call stuff 42))
  (print gen)
)

(fn-do + stuff _
  (param a _)
  (yield a)
)
```

```output
[Function Generator'I64]
```

```brix
(main
  (let source (call EntrySource))
  (let names (in (. each text) (. source entries) (when (!= (. each text) ""))))
  (print (call tally names))
)

(type + Entry
  (prop + text String)
  (fn-do + This _
    (param text String)
  )
)

(type + EntryProblem (is Error))

(type + EntrySource
  (fn-do + entries (stream Entry) :get
    (throw (call EntryProblem "failed") (if false))
    (yield (call Entry "a"))
  )
)

(fn-do + (of tally Item) Int
  (param stream (of Iterable Item))
  (let count 0)
  (for (in item stream)
    (+= count 1)
  )
  (return count)
)
```

```output
1
```

### A raising generator passed as an Iterable panics when it raises, since the interface carries no error type.

```brix crash
(main
  (let source (call EntrySource))
  (print (call tally (. source entries)))
)

(type + EntryProblem (is Error))

(type + EntrySource
  (fn-do + entries (stream String) :get
    (yield "a")
    (throw (call EntryProblem "failed"))
  )
)

(fn-do + (of tally Item) Int
  (param stream (of Iterable Item))
  (let count 0)
  (for (in item stream)
    (print item)
    (+= count 1)
  )
  (return count)
)
```

```output
a
```

### A generator can publish an event between yields.

```brix
(main
  (let source (call PublishingGeneratorSource))
  (for (on value (. source changed))
    (print (str "got " value))
  )
  (for (in n (call (. source numbers)))
    (print (str "n " n))
  )
  (print "done")
)

(type + PublishingGeneratorSource
  (event + changed Int)
  (fn-do + numbers (stream Int)
    (<- changed 1)
    (yield 10)
    (<- changed 2)
    (yield 20)
  )
)
```

```output
got 1
n 10
got 2
n 20
done
```

### A generator survives erased event publications between yields.

```brix
(main
  (let source (call ErasedPublishingGeneratorSource))
  (for (in n (call (. source numbers)))
    (print (str "n " n))
  )
  (print "done")
)

(type + ErasedPublishingGeneratorSource
  (event + changed Int)
  (fn-do + numbers (stream Int)
    (<- changed 1)
    (yield 10)
    (<- changed 2)
    (yield 20)
  )
)
```

```output
n 10
n 20
done
```

### Virtual generator overrides preserve the base method's error-result ABI.

```brix
(prop _ generator-cleanups _ (= 0))
(blank)

(main
  (let source EventSource (call QuietEventSource))
  (let result (catch (call collect-events source)))
  (if (is result (list Int))
    (let values (as result (list Int)))
    (print (str "count " (. values length)))
    (print (str "first " ([] values 0)))
    (else
      (print "unexpected error")
    )
  )
  (blank)
  (let failed (catch (call collect-events (call EventSource))))
  (print (str "base error " (is failed EventProblem)))
  (blank)
  (for (in value (call cleanup-items))
    (break)
  )
  (print (str "cleanup count " generator-cleanups))
)

(type + EventProblem (is Error))

(type + EventSource
  (fn-do + events (stream Int) (?! EventProblem)
    (yield 1)
    (throw (call EventProblem "base"))
  )
)

(type + QuietEventSource (is EventSource)
  (fn-do + events (stream Int)
    (yield 2)
  )
)

(fn-do _ collect-events (list Int) (?! EventProblem)
  (param source EventSource)
  (let values (list Int) (call (of List Int)))
  (for (in value (call (. source events)))
    (+= values value)
  )
  (return values)
)

(fn-do _ cleanup-items (stream Int)
  (finally
    (+= generator-cleanups 1)
  )
  (yield 1)
  (yield 2)
)
```

```output
count 1
first 2
base error true
cleanup count 1
```

```brix
(main
  (let gen (call stuff 42))
  (print gen)
  (print (call gen))
  (print (call gen))
  (print (call gen))
  (print (call gen))
)

(fn-do + stuff _
  (param a _)
  (yield a)
  (print "hi")
  (yield (+ a 1))
  (print "bye")
)
```

```output
[Function Generator'I64]
42
hi
43
bye
unknown
unknown
```

```brix
(main
  (let gen (call stuff))
  (print (call gen))
  (print (call gen))
  (print (call gen))
)

(fn-do + stuff _
  (let a 1)
  (let n (+ 10 a))
  (yield n)
  (= n (+ 20 a))
  (yield n)
)
```

```output
11
21
unknown
```

```brix
(main
  (let mapper (call map 3 (:cb (* each 2))))
  (print (call mapper))
  (print (call mapper))
  (print (call mapper))
  (print (call mapper))
)

(fn-do + map _
  (param n _)
  (param cb _)
  (let index 0)
  (while (< index n)
    (yield (call cb index))
    (+= index 1)
  )
)
```

```output
0
2
4
unknown
```

```brix
(main
  (for (in a (call gen))
    (print a)
  )
)

(fn-do + gen _
  (yield 1)
  (yield 2)
  (yield 3)
)
```

```output
1
2
3
```

```brix
(main
  (let genny (call gen))
  (print (type-of genny))
  (for (in a (call gen))
    (print a)
  )
)

(fn-do + gen (stream Int)
  (yield 1)
  (yield 2)
  (yield 3)
)
```

```output
[TypeInfo Iterable'I64]
1
2
3
```

```brix
(main
  (for (in a (call gen) (while (< a 3)))
    (print a)
  )
)

(fn-do + gen _
  (yield 1)
  (yield 2)
  (yield 3)
)
```

```output
1
2
```

```brix
(main
  (for (in a (call gen) (when (!= a 2)))
    (print a)
  )
)

(fn-do + gen _
  (yield 1)
  (yield 2)
  (yield 3)
)
```

```output
1
3
```

```brix
(main
  (for (in a (call gen2))
    (print a)
  )
)

(fn-do + gen1 _
  (yield 1)
  (yield 2)
  (yield 3)
)

(fn-do + gen2 _
  (for (in a (call gen1))
    (yield (+ a 1))
  )
)
```

```output
2
3
4
```

```brix
(main
  (for (in a (call foo (call bar)))
    (print a)
  )
)

(fn-do + foo _
  (param stream _)
  (let accum 1)
  (for (in item stream)
    (yield 1)
  )
  (= accum 2)
)

(fn-do + bar _
  (yield 1)
)
```

```output
1
```

### Generator as method

```brix
(main
  (let thing (call Thing))
  (for (in a (call (. thing foo) 1))
    (print a)
  )
)

(type + Thing
  (fn-do + foo _
    (param n _)
    (yield (+ n 1))
    (print this)
    (yield (+ n 2))
  )
)
```

```output
2
[Object __source__:Thing]
3
```

### Generator as closure

```brix
(main
  (let outer 1)
  (let gen
    (=> () :do
      (block
        (yield outer)
        (yield outer)
      )
    )
  )
  (blank)
  (for (in a (call gen))
    (print a)
    (+= outer 1)
  )
)
```

```output
1
2
```

```brix
(main
  (let outer 1)
  (let gen
    (=> (n) :do
      (block
        (yield (+ outer n))
      )
    )
  )
  (blank)
  (for (in a (call gen 1))
    (print a)
  )
)
```

```output
2
```

```brix
(main
  (let gen
    (=> (iterable) :do
      (block
        (for (in i iterable)
          (yield i)
        )
      )
    )
  )
  (blank)
  (for (in a (call gen (list 1 2 3)))
    (print a)
  )
)
```

```output
1
2
3
```

### Generator with do finally

```brix
(main
  (for (in a (call foo))
    (print a)
  )
)

(fn-do + foo _
  (finally
    (print "bye")
  )
  (blank)
  (yield 1)
  (yield 2)
)
```

```output
1
2
bye
```

### Generator with explicit return

```brix
(main
  (for (in a (call foo))
    (print a)
  )
)

(fn-do + foo _
  (finally
    (print "bye")
  )
  (blank)
  (yield 1)
  (return)
  (yield 3)
)
```

```output
1
bye
```

### Generator can return early and return expression is evaluated but discarded

```brix
(main
  (for (in n (call foo))
    (print n)
  )
)

(fn-do + foo _
  (yield 1)
  (return (call say))
)

(fn-do + say _
  (print "say")
  (return 42)
)
```

```output
1
say
```

### Generator with explicit return in nested scope

```brix
(main
  (for (in a (call foo 1))
    (print a)
  )
)

(fn-do + foo _
  (param a _)
  (finally
    (print "bye")
  )
  (yield 1)
  (if a
    (return)
  )
  (yield 3)
)
```

```output
1
bye
```

```brix
(main
  (let f (call foo))
  (let g (as f (of Iterable Int)))
  (for (in n g)
    (print n)
  )
)

(fn-do + foo _
  (yield 1)
)
```

```output
1
```

```brix
(main
  (let thing (call (of Thing Int)))
  (for (in n thing)
    (print n)
  )
)

(type + (of Thing T)
  (fn-to-do + (of Iterable T)
    (yield 1)
  )
)
```

```output
1
```

```brix
(main
  (let thing (call (of Thing Int) 1))
  (for (in n thing)
    (print n)
  )
)

(type + (of Thing T) (is Tuple)
  (prop + start T)
  (fn-to-do + (of Iterable T)
    (yield (. this start))
  )
)
```

```output
1
```

```brix
(main
  (for (in i (call foo (..< 10 15)))
    (print i)
  )
)

(fn-do + (of foo T) _
  (param stream (stream T))
  (yield 1)
  (yield-all stream)
)
```

```output
1
10
11
12
13
14
```

```brix
(main
  (let names (list "a" "b" "c"))
  (let sizes (list 1 0 3))
  (let labels
    (iterate (in names) (and-for (in size sizes))
      (block
        (str each "=" size)
      )
    )
  )
  (for (in label labels)
    (print label)
  )
)
```

```output
a=1
b=0
c=3
```

### A generator read by two loops, the first left early, and one read by `<-` past its end.

```brix
(main
  (let g (call gen))
  (for (in a g)
    (print a)
    (break)
  )
  (for (in a g)
    (print a)
  )
  (print "end")
  (let h (call gen))
  (print (<- h))
  (print (<- h))
  (print (<- h))
  (print (<- h))
)

(fn-do + gen _
  (print "start")
  (yield 1)
  (print "middle")
  (yield 2)
  (print "last")
)
```

```output
start
1
end
start
1
middle
2
last
unknown
unknown
```

### A local of a generator keeps its value from one yield to the next.

```brix
(main
  (for (in a (call gen))
    (print a)
  )
)

(fn-do + gen _
  (let count 3)
  (yield 1)
  (+= count 1)
  (yield count)
)
```

```output
1
4
```

### Generators that yield in loops: over a range up and down, over a list, over a text, and in two loops nested.

```brix
(main
  (for (in a (call count 3))
    (print a)
  )
  (for (in a (call down 3))
    (print a)
  )
  (for (in a (call evens (list 1 2 3 4 5 6)))
    (print a)
  )
  (for (in a (call letters "héy"))
    (print a)
  )
  (for (in a (call nested 2))
    (print a)
  )
  (let total 0)
  (for (in a (call count 1000))
    (+= total a)
  )
  (print total)
)

(fn-do + count _
  (param limit Int)
  (for (in i (..< 0 limit))
    (yield i)
  )
)

(fn-do + down _
  (param from Int)
  (for (in i (.. from 1))
    (yield i)
  )
  (yield 0)
)

(fn-do + evens _
  (param items (list Int))
  (for (in item items)
    (if (== (% item 2) 0)
      (yield item)
    )
  )
)

(fn-do + letters _
  (param text String)
  (for (in char text)
    (yield char)
  )
)

(fn-do + nested _
  (param size Int)
  (for (in i (..< 0 size))
    (for (in j (..< 0 size))
      (yield (+ (* i 10) j))
    )
  )
)
```

```output
0
1
2
3
2
1
0
2
4
6
h
é
y
0
1
10
11
499500
```

### A generator that reads another, one that stops reading early, a member that yields, and generators of texts and of tuples.

```brix
(main
  (for (in a (call doubled (call count 3)))
    (print a)
  )
  (for (in a (call firsts (call count 100) 2))
    (print a)
  )
  (let c (call Counter 2))
  (for (in a (call (. c steps)))
    (print a)
  )
  (for (in a (call (. c steps)))
    (print a)
  )
  (print (. c made))
  (for (in word (call words))
    (print word)
  )
  (for (in pair (call pairs))
    (print ([] pair 0))
    (print ([] pair 1))
  )
)

(fn-do + count _
  (param limit Int)
  (let i 0)
  (while (< i limit)
    (yield i)
    (+= i 1)
  )
)

(fn-do + doubled _
  (param source (of Generator Int))
  (for (in item source)
    (yield (* item 2))
  )
)

(fn-do + firsts _
  (param source (of Generator Int))
  (param limit Int)
  (let taken 0)
  (for (in item source)
    (if (== taken limit)
      (break)
    )
    (+= taken 1)
    (yield item)
  )
)

(fn-do + words _
  (yield "one")
  (yield "two")
)

(fn-do + pairs _
  (yield (tuple 1 "a"))
  (yield (tuple 2 "b"))
)

(type + Counter
  (prop + limit Int)
  (prop + made _ (= 0))
  (fn-do + This _
    (param limit Int)
  )
  (fn-do + steps _
    (for (in i (..< 0 limit))
      (+= made 1)
      (yield (+ (* made 100) i))
    )
  )
)
```

```output
0
2
4
0
1
100
201
300
401
4
one
two
1
a
2
b
```

### A panic raised between two yields of a generator leaves the loop that reads it.

```brix crash
(main
  (call run)
)

(fn-do + run _
  (for (in a (call gen 2))
    (print a)
  )
)

(fn-do + gen _
  (param limit Int)
  (yield 1)
  (call check limit)
  (yield 2)
)

(fn-do + check _
  (param n Int)
  (throw (call Panic "bad") (if (== n 2)))
)
```

```output
1
```

### A generator left by `break` goes on where it stopped; one read by `<-` and then by a loop; a return from inside a loop over one.

```brix
(main
  (let g (call count 5))
  (for (in a g)
    (print a)
    (if (== a 1)
      (break)
    )
  )
  (print "between")
  (for (in a g)
    (print a)
  )
  (print "after")
  (let h (call count 3))
  (print (<- h))
  (for (in a h)
    (print a)
  )
  (print (call outer))
)

(fn-do + count _
  (param limit Int)
  (for (in i (..< 0 limit))
    (yield i)
  )
)

(fn-do + outer Int
  (for (in a (call count 10))
    (if (== a 3)
      (return (* a 2))
    )
  )
  (return 0)
)
```

```output
0
1
between
after
0
1
2
6
```

### A generator given to a function that takes a stream, read by a loop there, left early, and made a list.

```brix
(main
  (call show (call gen 3))
  (print (call total (call gen 4)))
  (let list (call (of List Int) (call gen 3)))
  (print (tuple (. list length) ([] list 0) ([] list 2)))
)

(fn-do + show _
  (param stream (stream Int))
  (for (in item stream)
    (print item)
  )
)

(fn-do + total Int
  (param stream (stream Int))
  (let sum 0)
  (for (in item stream)
    (if (== item 3)
      (break)
    )
    (+= sum item)
  )
  (return sum)
)

(fn-do + gen (stream Int)
  (param n Int)
  (let i 0)
  (while (< i n)
    (yield i)
    (+= i 1)
  )
)
```

```output
0
1
2
3
(3, 0, 2)
```

### A loop over a stream in a constructor, left early.

```brix
(main
  (let bag (call Bag (call gen 4)))
  (print (tuple (. bag count) (. bag sum)))
  (let other (call Bag (call gen 0)))
  (print (tuple (. other count) (. other sum)))
  (let third (call Bag (call gen 9)))
  (print (tuple (. third count) (. third sum)))
)

(type + Bag
  (prop + count Int)
  (prop + sum Int)
  (fn-do + This _
    (param stream (stream Int))
    (for (in item stream)
      (if (== item 5)
        (break)
      )
      (+= count 1)
      (+= sum item)
    )
  )
)

(fn-do + gen (stream Int)
  (param n Int)
  (let i 0)
  (while (< i n)
    (yield i)
    (+= i 1)
  )
)
```

```output
(4, 6)
(0, 0)
(5, 10)
```

### A panic in the body of a loop over a generator runs the cleanup the generator has open.

```brix crash
(main
  (let bag (call Bag (call gen 4)))
  (print (. bag count))
)

(type + Bag
  (prop + count Int)
  (fn-do + This _
    (param stream (stream Int))
    (for (in item stream)
      (+= count (call check item))
    )
  )
)

(fn-do + check Int
  (param item Int)
  (if (== item 2)
    (throw (call Panic "two"))
  )
  (return 1)
)

(fn-do + gen (stream Int)
  (param n Int)
  (finally
    (print "closed")
  )
  (let i 0)
  (while (< i n)
    (yield i)
    (+= i 1)
  )
)
```

```output
closed
```

### A generator an interface declares, run for an object through a value of the interface and read by `<-`.

```brix
(main
  (let dog Pet (call Dog "rex"))
  (for (in line (call (. dog lines) 2))
    (print line)
  )
  (let cat (call Dog "tom"))
  (for (in line (call (. cat lines) 1))
    (print line)
  )
  (let lines (call (. dog lines) 3))
  (print (<- lines))
  (print (<- lines))
)

(type + Pet (is Interface)
  (fn-expr + name String :get)
  (fn-do + lines (stream String)
    (param n Int)
    (let i 0)
    (while (< i n)
      (yield (str name " " i))
      (+= i 1)
    )
  )
)

(type + Dog
  (implements Pet)
  (prop _ called String)
  (fn-do + This _
    (param called _)
  )
  (fn-expr + name _ :get
    called
  )
)
```

```output
rex 0
rex 1
tom 0
rex 0
rex 1
```

### A panic in a generator an interface declares.

```brix crash
(main
  (let dog Pet (call Dog "rex"))
  (for (in line (call (. dog lines) 4))
    (print line)
  )
)

(type + Pet (is Interface)
  (fn-expr + name String :get)
  (fn-do + lines (stream String)
    (param n Int)
    (let i 0)
    (while (< i n)
      (if (== i 2)
        (throw (call Panic "at two"))
      )
      (yield (str name " " i))
      (+= i 1)
    )
  )
)

(type + Dog
  (implements Pet)
  (prop _ called String)
  (fn-do + This _
    (param called _)
  )
  (fn-expr + name _ :get
    called
  )
)
```

```output
rex 0
rex 1
```

### A stream read by `<-` in a function that does nothing else, once and twice in a tuple.

```brix
(main
  (let g (call gen))
  (print (call first g))
  (print (call first g))
  (print (call both g))
)

(fn-do + first (? Int)
  (param stream (stream Int))
  (return (<- stream))
)

(fn-expr + both (tuple (? Int) (? Int))
  (param stream (stream Int))
  (tuple (<- stream) (<- stream))
)

(fn-do + gen (stream Int)
  (yield 1)
  (yield 2)
  (yield 3)
)
```

```output
1
2
(3, unknown)
```

### A yield of an optional, `<=?`: only a value that is there is yielded.

```brix
(main
  (for (in each (call evens (list 1 2 3 4 5 6)))
    (print each)
  )
  (for (in each (call firsts (list "ab" "" "cd")))
    (print each)
  )
  (for (in each (call both 3))
    (print each)
  )
)

(fn-do _ evens (stream Int)
  (param items (list Int))
  (for (in item items)
    (yield? (call even item))
  )
)

(fn-do _ even (? Int)
  (param n Int)
  (if (== (% n 2) 0)
    (return n)
  )
  (return null)
)

(fn-do _ firsts (stream String)
  (param words (list String))
  (for (in word words)
    (yield? (call head word))
    (yield "-")
  )
)

(fn-do _ head (? String)
  (param word String)
  (if (== (. word length) 0)
    (return null)
  )
  (return word)
)

(fn-do _ both (stream Int)
  (param n Int)
  (yield? (call even n))
  (yield n)
  (yield? (call even (+ n 1)))
)
```

```output
2
4
6
ab
-
-
cd
-
3
4
```

### A yield converted to the type of the generator's items: a whole number to a float and to a wider one, and a tuple.

```brix
(main
  (for (in each (call halves))
    (print each)
  )
  (for (in each (call wide))
    (print each)
  )
  (for (in (tuple index word) (call tagged (list "one" "two")))
    (print (str index " " word))
  )
)

(fn-do _ halves (stream Float)
  (yield 1)
  (yield 2.5)
  (let n I32 3)
  (yield n)
)

(fn-do _ wide (stream Int64)
  (let n 7)
  (yield n)
  (yield 8)
)

(fn-do _ tagged (stream (tuple Int String))
  (param items (list String))
  (for (in index (..< 0 (. items length)))
    (yield (tuple index ([] items index)))
  )
)
```

```output
1
2.5
3
7
8
0 one
1 two
```

### A yield of every item of a stream, `<<=`: of a generator and of a list.

```brix
(main
  (for (in each (call all))
    (print each)
  )
  (for (in each (call twice (list 1 2)))
    (print each)
  )
)

(fn-do _ evens (stream Int)
  (param items (list Int))
  (for (in item items)
    (if (== (% item 2) 0)
      (yield item)
    )
  )
)

(fn-do _ all (stream Int)
  (yield 0)
  (yield-all (call evens (list 10 11 12)))
  (yield-all (list 20 21))
  (yield 99)
)

(fn-do _ twice (stream Int)
  (param items (list Int))
  (yield-all items)
  (yield-all items)
)
```

```output
0
10
12
20
21
99
1
2
1
2
```

### A generator declared to return a stream that yields every item of a call to itself.

```brix
(main
  (for (in value (call leaves 3))
    (print (str value))
  )
)

(fn-do _ leaves (stream Int)
  (param depth Int)
  (if (> depth 0)
    (yield-all (call leaves (- depth 1)))
  )
  (yield depth)
)
```

```output
0
1
2
3
```

### A method generator that yields every item of a call to itself, first reached by a bare call inside its class and then called on an object.

```brix
(main
  (let node (call Node "a"))
  (+= (. node kids) (call Node "b"))
  (print (?? (?. (call (. node first)) id) "none"))
  (for (in each (call (. node all)))
    (print (. each id))
  )
)

(type + Node
  (prop + id String)
  (prop + kids (list Node))
  (fn-do + This _
    (param id _)
  )
  (fn-do + all (stream Node)
    (yield this)
    (for (in kid kids)
      (yield-all (call (. kid all)))
    )
  )
  (fn-expr + first (? Node)
    (<- (call all))
  )
)
```

```output
a
a
b
```

### A generator that yields every item of another generator, in a program that asks to inline nothing.

```brix
(main
  (for (in each (call outer))
    (print each)
  )
)

(fn-do _ inner (stream String)
  (yield "a")
)

(fn-do _ outer (stream String)
  (yield-all (call inner))
)
```

```output
a
```

### A comprehension written as a block, over a range: with a name for the item, and with `when`, `while` and `until`.

```brix
(main
  (let iter
    (case-each (in n (.. 1 3))
      (* n 2)
    )
  )
  (for (in each iter)
    (print each)
  )
  (let evens
    (case-each (in n (.. 1 6) (when (== (% n 2) 0)))
      n
    )
  )
  (for (in each evens)
    (print each)
  )
  (let early
    (case-each (in n (.. 1 6) (while (< n 3)))
      (+ n 100)
    )
  )
  (for (in each early)
    (print each)
  )
  (let late
    (case-each (in n (.. 1 6) (until (== n 3)))
      (- n 100)
    )
  )
  (for (in each late)
    (print each)
  )
)
```

```output
2
4
6
2
4
6
101
102
-99
-98
```

### A comprehension over a range that reads a local of the function that writes it, one in a function with a clause, and one of floats.

```brix
(main
  (let k 10)
  (for (in each (in (* each k) (.. 1 3)))
    (print each)
  )
  (print (call total 4))
  (let scaled (in (+ it 0.5) (..< 0 3)))
  (for (in each scaled)
    (print each)
  )
)

(fn-do _ total Int
  (param n Int)
  (let sum 0)
  (for (in each (in (* each each) (.. 1 n) (when (== (% each 2) 0))))
    (+= sum each)
  )
  (return sum)
)
```

```output
10
20
30
20
0.5
1.5
2.5
```

### A comprehension a method returns, which reads the receiver and an argument.

```brix
(main
  (let box (call Box 3))
  (for (in each (call (. box steps)))
    (print each)
  )
  (for (in each (call (. box doubled) 2))
    (print each)
  )
)

(type + Box
  (prop + size Int)
  (fn-do + This _
    (param size Int)
  )
  (fn-do + steps (stream Int)
    (return (in (+ each size) (..< 0 size)))
  )
  (fn-do + doubled (stream Int)
    (param by Int)
    (return (in (* each by size) (.. 1 2)))
  )
)
```

```output
3
4
5
6
12
```

### A comprehension reads a local as it is when the items are read; one never read, one left by `break`, one given to a function as a stream, and an empty one.

```brix
(main
  (let k 2)
  (let later (in (* each k) (.. 1 3)))
  (= k 5)
  (for (in each later)
    (print each)
  )
  (let unread (in each (.. 1 3)))
  (for (in each (in (+ each 1) (.. 1 10)))
    (if (> each 3)
      (break)
    )
    (print each)
  )
  (print (call sum-of (in (* each each) (.. 1 4))))
  (print (call count (in it (..< 0 0))))
)

(fn-do _ sum-of Int
  (param items (stream Int))
  (let sum 0)
  (for (in item items)
    (+= sum item)
  )
  (return sum)
)

(fn-do _ count Int
  (param items (stream Int))
  (let n 0)
  (for (in item items)
    (+= n 1)
  )
  (return n)
)
```

```output
5
10
15
2
3
30
0
```

### A comprehension in a function that yields, one a nested function returns, and one read inside the loop of another.

```brix
(main
  (for (in each (call outer 3))
    (print each)
  )
  (let twice (=> (n) (in (* each n) (.. 1 2))))
  (for (in each (call twice 7))
    (print each)
  )
  (for (in row (in (* each 10) (.. 1 2)))
    (for (in cell (in (+ each 100) (.. 1 2)))
      (print (+ cell row))
    )
  )
)

(fn-do _ outer (stream Int)
  (param n Int)
  (for (in each (in (* each n) (.. 1 2)))
    (yield each)
  )
  (yield-all (in (- each n) (.. 1 2)))
)
```

```output
3
6
-2
-1
7
14
111
112
121
122
```

### A generator given where an iterable of another type of item is wanted, each item converted: from a function that yields and from a comprehension.

```brix
(main
  (print (call total (call walk)))
  (print (call wide (call walk)))
  (print (call total (in (* each 2) (.. 1i32 3))))
  (let kept (of Iterable Float) (call walk))
  (for (in each kept)
    (print each)
  )
)

(fn-do _ walk (stream I32)
  (yield 1)
  (yield 2)
)

(fn-do _ total Float
  (param items (of Iterable Float))
  (let sum 0.0)
  (for (in item items)
    (+= sum item)
  )
  (return sum)
)

(fn-do _ wide Int64
  (param items (of Iterable Int64))
  (let sum 0i64)
  (for (in item items)
    (+= sum item)
  )
  (return sum)
)
```

```output
3
3
12
1
2
```

### A panic under a comprehension written in the header of a loop.

```brix crash
(main
  (let items (list 1 0))
  (for (in each (in (/ 10 (call check each)) items))
    (print each)
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

```output
10
```

### A panic under a comprehension kept in a local and read by a loop of the same function.

```brix crash
(main
  (print (call total))
)

(fn-do _ total Int
  (let items (list 1 0))
  (let sum 0)
  (let halves (in (/ 10 (call check each)) items))
  (for (in each halves)
    (+= sum each)
  )
  (return sum)
)

(fn-do _ check Int
  (param d Int)
  (if (== d 0)
    (throw (call Panic "zero"))
  )
  (return d)
)
```

### A panic under a comprehension read through `join`.

```brix crash
(main
  (let items (list 1 0))
  (let halves (in (/ 10 (call check each)) items))
  (print (call (. ", " join) halves))
)

(fn-do _ check Int
  (param d Int)
  (if (== d 0)
    (throw (call Panic "zero"))
  )
  (return d)
)
```

### A panic under the second of two comprehensions over one name.

```brix crash
(main
  (call run 2)
)

(fn-do + run _
  (param bad Int)
  (let items (list 1 2 3))
  (let first (in (call check each 9) items))
  (for (in item first)
    (print item)
  )
  (let second (in (call check each bad) items))
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

### A panic under a comprehension over a range, written in the header of a loop.

```brix crash
(main
  (call run 2)
)

(fn-do + run _
  (param bad Int)
  (for (in item (in (call check each bad) (.. 1 3)))
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
```

### A panic under the second of two comprehensions over a property, read through `join`.

```brix crash
(main
  (call run 2)
)

(fn-do + run _
  (param bad Int)
  (let box (call Box))
  (print (call (. ", " join) (in (call check each 9) (. box items))))
  (print (call (. ", " join) (in (call check each bad) (. box items))))
)

(type + Box
  (prop + items _ (= (list 1 2 3)))
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
1, 2, 3
```

### A class read by `<-` through a value of an interface panics in a later read: the loop that reads it ends, and the program ends by the panic.

```brix crash
(main
  (let f (of Iterable Int) (call Feed))
  (let sum 0)
  (let item (<- f))
  (while item
    (+= sum item)
    (= item (<- f))
  )
  (print sum)
)

(type + Feed
  (implements (of Iterable Int))
  (prop _ n _ (= 0))
  (fn-do + (prefix <- this) (? Int)
    (+= n 1)
    (if (== n 2)
      (throw (call Panic "feed"))
    )
    (return n)
  )
)
```

### Yields of two tuples, neither the wider: the items have the tuple of what the fields at each place join to.

```brix
(main
  (for (in item (call gen))
    (print item)
  )
)

(fn-do + gen _
  (yield (tuple 1 "a"))
  (yield (tuple 2.5 "b"))
)
```

```output
(1, a)
(2.5, b)
```

### Three yields of pairs with a float on either side: every item has a float at both places.

```brix
(main
  (for (in item (call gen))
    (print item)
  )
)

(fn-do + gen _
  (yield (tuple 2.5 1))
  (yield (tuple 1 2.5))
  (yield (tuple 7 7))
)
```

```output
(2.5, 1)
(1, 2.5)
(7, 7)
```

### Yields of tuples that hold tuples join place by place, within the tuples they hold too.

```brix
(main
  (for (in item (call gen))
    (print item)
  )
)

(fn-do + gen _
  (yield (tuple 1 (tuple 2 3)))
  (yield (tuple 1.5 (tuple 2.5 3)))
  (yield (tuple 1 (tuple 2 3.5)))
)
```

```output
(1, (2, 3))
(1.5, (2.5, 3))
(1, (2, 3.5))
```

### Yields of two lists give a list both convert to.

```brix
(main
  (for (in item (call gen))
    (print item)
  )
)

(fn-do + gen _
  (yield (list 1 2))
  (yield (list 2.5))
)
```

```output
1, 2
2.5
```

### A struct yielded before a tuple of its fields: the tuple is converted to the struct.

```brix
(main
  (for (in item (call gen))
    (print item)
  )
)

(fn-do + gen _
  (yield (call P 1 2))
  (yield (tuple 3 4))
)

(type + P (is Struct)
  (prop + x Int)
  (prop + y Int)
  (fn-to-expr + String
    (str "P" x y)
  )
)
```

```output
P12
P34
```

### A generator of optional items yields an item that holds nothing without ending

```brix
(main
  (for (in item (call gaps))
    (print (?? item -1))
  )
  (print "collected")
  (let all (call List (call gaps)))
  (print (. all length))
  (let texts (call List (call named)))
  (print (. texts length))
  (for (in text (call named))
    (print (?? text "(none)"))
  )
  (let kept
    (block
      (spread (call gaps))
      (pipe ?> (!= each null))
      (pipe *> (?? each 0))
      (pipe |> List)
    )
  )
  (print (call (. ", " join) kept))
)

(fn-do + gaps _
  (let missing (? Int))
  (yield 1)
  (yield missing)
  (yield 3)
  (yield missing)
)

(fn-do + named (stream (? String))
  (let missing (? String))
  (yield "a")
  (yield missing)
  (yield "c")
)
```

```output
1
-1
3
-1
collected
4
3
a
(none)
c
1, 3
```

### A function that yields and is declared to return a list returns the list of what it yields

```brix
(main
  (let shelf (call Shelf))
  (call (. shelf add) "b")
  (call (. shelf add) "a")
  (let titles (. shelf titles))
  (print (type-of titles))
  (print (. titles length))
  (print (call (. ", " join) titles))
  (call (. shelf add) "c")
  (print (. titles length))
  (print (. shelf titles length))
  (print (call squares 4))
  (print (. (call squares 0) length))
  (print (type-of (call pairs)))
  (for (in (tuple n name) (call pairs))
    (print (str n "=" name))
  )
  (let things (. shelf things))
  (print (type-of things))
  (for (in thing things)
    (print (. thing label))
  )
)

(type + Labelled (is Interface)
  (fn-expr + label String :get)
)

(type + Book
  (implements Labelled)
  (prop + label String)
  (fn-do + This _
    (param label _)
  )
)

(type + Shelf
  (implements Labelled)
  (prop + label _ (= "shelf"))
  (prop _ books (list Book))
  (fn-do + add _
    (param title String)
    (+= books (call Book title))
  )
  (fn-do + titles (list String) :get
    (for (in book books)
      (yield (. book label))
    )
  )
  (fn-do + things (list Labelled) :get
    (yield this)
    (for (in book books)
      (yield book)
    )
  )
)

(fn-do + squares (list Int)
  (param count Int)
  (for (in n (..< 0 count))
    (yield (* n n))
  )
)

(fn-do + pairs (list (tuple Int String))
  (yield (tuple 1 "one"))
  (yield (tuple 2 "two"))
)
```

```output
[TypeInfo List'String'GCBacking]
2
b, a
2
3
0, 1, 4, 9
0
[TypeInfo List'Tuple'GCBacking]
1=one
2=two
[TypeInfo List'Labelled'GCBacking]
shelf
b
a
c
```

### A function that yields cannot be declared to return what no generator converts to

```brix crash
(main
  (print (call bad))
)

(fn-do + bad Int
  (yield 1)
)
```

```output
Exception: Generator return type does not match declared return type
<no file:6>
    (fn-do + bad Int
    ^^^^^^^^^^^^^^^
```

### A generator that is made and never stepped runs nothing of its body

```brix
(main
  (let unused (call counted 2))
  (print "made")
  (let held (call counted 3))
  (print held)
  (call show (call counted 1))
  (print (call kind (call counted 5)))
  (let first (call held))
  (print (?? first -1))
)

(fn-do + counted _
  (param count Int)
  (print "started")
  (for (in n (..< 0 count))
    (yield n)
  )
)

(fn-do + (of show T) _
  (param items (stream T))
  (print (call T))
)

(fn-expr + kind (stream Int)
  (param thing _)
  thing
)
```

```output
made
[Function Generator'I64]
0
[Interface core/stream:Iterable'core/number:I64]
started
0
```

### A return in a function that yields evaluates its value, runs the cleanups and finishes

```brix
(main
  (for (in n (call until 3))
    (print n)
  )
  (for (in n (call until 0))
    (print n)
  )
  (for (in n (call nested))
    (print n)
  )
  (for (in n (call tidy))
    (print n)
  )
)

(fn-do + until _
  (param stop Int)
  (for (in n (..< 0 10))
    (if (== n stop)
      (return (call announce (str "stop at " n)))
    )
    (yield n)
  )
  (print "never")
)

(fn-do + nested _
  (for (in a (.. 1 3))
    (for (in b (.. 1 3))
      (if (== (* a b) 4)
        (return (call announce "found"))
      )
      (yield (+ (* a 10) b))
    )
  )
)

(fn-do + tidy _
  (finally
    (print "tidied")
  )
  (yield 1)
  (return (call announce "leaving"))
)

(fn-do + announce Int
  (param text String)
  (print text)
  (return 0)
)
```

```output
0
1
2
stop at 3
stop at 0
11
12
13
21
found
1
leaving
tidied
```

### A member of a tuple that yields keeps the tuple it was called on

```brix
(main
  (let counter (call Counter 3))
  (for (in n (. counter steps))
    (print n)
    (= counter (call Counter 10))
  )
  (print (. counter count))
  (let second (call Counter 2))
  (let held (. second steps))
  (= second (call Counter 5))
  (for (in n held)
    (print n)
  )
  (print (. second count))
  (for (in n (call (. (call Counter 2) scaled) 7))
    (print n)
  )
  (for (in text (call Pair "a" 2))
    (print text)
  )
)

(type + Counter (is Tuple)
  (prop + count Int)
  (fn-do + steps _ :get
    (for (in index (..< 0 count))
      (yield (+ (* index 100) count))
    )
  )
  (fn-do + (of scaled T) _
    (param by T)
    (for (in index (..< 0 count))
      (yield (tuple by index))
    )
  )
)

(type + (of Pair T) (is Tuple)
  (prop + item T)
  (prop + times Int)
  (fn-to-do + (of Iterable T)
    (for (in index (..< 0 times))
      (yield item)
    )
  )
)
```

```output
3
103
203
10
2
102
5
(7, 0)
(7, 1)
a
a
```

### List pattern declared in a function that yields

```brix
(main
  (for (in n (call gen))
    (print n)
  )
)

(fn-do + gen _
  (let (list a b) (list 1 2))
  (yield (+ a b))
)
```

```output
3
```

### List pattern with a named rest declared in a function that yields

```brix
(main
  (for (in n (call gen))
    (print n)
  )
)

(fn-do + gen _
  (let (list a ...rest) (list 1 2 3))
  (yield a)
  (yield (. rest length))
  (yield (+ a ([] rest 0)))
)
```

```output
1
2
3
```

### List pattern declared in a closure that yields

```brix
(main
  (let g
    (=> (items) :do
      (block
        (let (list a b) items)
        (yield a)
        (yield b)
      )
    )
  )
  (for (in n (call g (list 7 8)))
    (print n)
  )
)
```

```output
7
8
```

### List pattern declared in a function that yields a list

```brix
(main
  (print (call squares))
)

(fn-do + squares (list Int)
  (let (list a b) (list 3 4))
  (yield (* a a))
  (yield (* b b))
)
```

```output
9, 16
```

### each and it of the first header in a function that yields

```brix
(main
  (for (in text (call walk))
    (print text)
  )
)

(fn-do + walk _
  (for (in (list "u" "v"))
    (and-for (in (list 1 2))
      (yield (str each " " it))
    )
  )
)
```

```output
u u
v v
```

### A step that raises is read by <- as a call that raises: the error leaves, or catch holds it.

```brix
(main
  (let gen (call items true))
  (print gen)
  (let first (<- gen))
  (print first)
  (let second (catch (<- gen)))
  (print (is second Problem))
)

(type + Problem (is Error))

(fn-do + items _
  (param fail Bool)
  (yield 1)
  (if fail
    (throw (call Problem "bad"))
  )
  (yield 2)
)
```

```output
[Function Generator'I64]
1
true
```

### A loop over a generator that raises leaves by the error, running the cleanups of the loop's body and of the generator; one that does not raise runs to the end.

```brix
(prop _ cleanups _ (= 0))
(blank)

(main
  (let r (catch (call total (call items true))))
  (print (is r Problem))
  (print (call total (call items false)))
  (print (str "cleanups " cleanups))
)

(type + Problem (is Error))

(fn-do + items _
  (param fail Bool)
  (finally
    (+= cleanups 1)
  )
  (yield 1)
  (if fail
    (throw (call Problem "bad"))
  )
  (yield 2)
)

(fn-do + total Int
  (param stream (stream Int))
  (let sum 0)
  (for (in item stream)
    (finally
      (print (str "after " item))
    )
    (+= sum item)
  )
  (return sum)
)
```

```output
after 1
true
after 1
after 2
3
cleanups 2
```

### A generator that loops over one that raises raises too, and so does a comprehension over it.

```brix
(main
  (let r (catch (call sum-all)))
  (print (is r Problem))
  (let s (catch (call outer)))
  (print (is s Problem))
)

(type + Problem (is Error))

(fn-do + inner _
  (yield 1)
  (throw (call Problem "inner"))
)

(fn-do + relay _
  (for (in x (call inner))
    (yield (* x 10))
  )
)

(fn-do + outer Int
  (let n 0)
  (for (in x (call relay))
    (print x)
    (+= n x)
  )
  (return n)
)

(fn-do + sum-all Int
  (let values (in (+ each 1) (call relay)))
  (let n 0)
  (for (in v values)
    (print v)
    (+= n v)
  )
  (return n)
)
```

```output
11
true
10
true
```

### An override of a generator whose base states errors carries them, raising or not, and a loop through the base's table leaves by the error.

```brix
(main
  (let src Source (call Loud))
  (let r (catch (call drain src)))
  (print (is r Problem))
  (let q (catch (call drain (call Source))))
  (print q)
)

(type + Problem (is Error))

(type + Source
  (fn-do + numbers (stream Int) (?! Problem)
    (yield 1)
    (yield 2)
  )
)

(type + Loud (is Source)
  (fn-do + numbers (stream Int)
    (yield 5)
    (throw (call Problem "loud"))
  )
)

(fn-do + drain Int (?! Problem)
  (param source Source)
  (let n 0)
  (for (in x (call (. source numbers)))
    (print x)
    (+= n x)
  )
  (return n)
)
```

```output
5
true
1
2
3
```

### An error raised by a generator leaves nested loops at once.

```brix
(main
  (let r (catch (call run)))
  (print (is r Problem))
  (print "end")
)

(type + Problem (is Error))

(fn-do + items _
  (yield "a")
  (throw (call Problem "x"))
)

(fn-do + run _
  (for (in a (call items))
    (for (in b (..< 0 2))
      (print (str a b))
    )
  )
)
```

```output
a0
a1
true
end
```

### A list of feeds whose feed raises is a generator that raises.

```brix
(main
  (let r (catch (call run)))
  (print (is r Problem))
)

(type + Problem (is Error))

(fn-do + check Int (?! Problem)
  (param n Int)
  (if (> n 2)
    (throw (call Problem "big"))
  )
  (return n)
)

(fn-do + run _
  (let values
    (block
      (item 1)
      (item (call check 2))
      (item (call check 3))
    )
  )
  (for (in v values)
    (print v)
  )
)
```

```output
1
2
true
```

### A comprehension that raises, read by a loop whose turns make closures.

```brix
(main
  (let r (catch (call run)))
  (print (is r Problem))
)

(type + Problem (is Error))

(fn-do + check Int (?! Problem)
  (param n Int)
  (if (> n 2)
    (throw (call Problem "big"))
  )
  (return n)
)

(fn-do + run _
  (let callbacks (call (of List Doer)))
  (for (in v (in (call check each) (list 1 2 3)))
    (+= callbacks (=> () :do (print (str "cb " v))))
    (print v)
  )
  (for (in c callbacks)
    (call c)
  )
)
```

```output
1
2
true
```

```brix crash
(main
  (print "start")
  (for (in x (call items))
    (print x)
  )
)

(type + Problem (is Error))

(type + Other (is Error))

(fn-do + items (stream Int) (?! Problem)
  (yield 1)
  (throw (call Other "bad"))
)
```

```output
Exception: Other can leave generator items, which does not declare it; declare it with ?! Other, or catch it here
<no file:15>
      (throw (call Other "bad"))
      ^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix crash
(main
  (print (call describe (call items)))
  (let g (call items))
  (print "made")
)

(type + Problem (is Error))

(fn-do + items _
  (yield 1)
  (throw (call Problem "bad"))
)

(fn-expr + describe String
  (param g (of Generator Int))
  "gen"
)
```

```output
Exception: No function "describe" accepts (Generator'I64); it is declared as "describe(g Generator'Int)"
<no file:3>
      (print (call describe (call items)))
             ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A generator that raises, made and never read, is made.

```brix
(main
  (let gen (call items))
  (print "made")
)

(type + Problem (is Error))

(fn-do + items _
  (yield 1)
  (throw (call Problem "text"))
)
```

```output
made
```

### A generator that raises, kept as an Iterable'Int that is never read, is made.

```brix
(main
  (let it (of Iterable Int) (call items))
  (print "as iterable only")
)

(type + Problem (is Error))

(fn-do + items _
  (yield 1)
  (throw (call Problem "text"))
)
```

```output
as iterable only
```

### catch? of a step of a generator that raises gives the item, or nothing where the step raised.

```brix
(main
  (let gen (call items))
  (let a (catch? (<- gen)))
  (print (?? a -1))
  (let b (catch? (<- gen)))
  (print (?? b -2))
)

(type + Problem (is Error))

(fn-do + items _
  (yield 1)
  (throw (call Problem "bad"))
)
```

```output
1
-2
```

### A generator reads a name a match pattern binds after it yields.

```brix
(main
  (for (in x (call gen (list (call (. Item num) 1) (call (. Item word) "b") (call (. Item pair) (tuple 3 "c")))))
    (print x)
  )
)

(fn-do + gen (of Iterable String)
  (param items (list Item))
  (for (in item items)
    (match item
      (case (call .num n)
        (yield (str "n " n))
        (yield (str "n again " n))
      )
      (case (call .word w)
        (yield (str "w " w))
        (yield (str "w again " w))
      )
      (case (call .pair a b)
        (yield (str "p " a))
        (yield (str "p " b))
      )
    )
  )
)

(type + Item (is Union)
  (variant num Int)
  (variant word String)
  (variant pair (tuple Int String))
)
```

```output
n 1
n again 1
w b
w again b
p 3
p c
```

```brix
(import /BxGC/BxGC.h)
(blank)

(main
  (let source (call kept-values "live"))
  (print (call source))
  (call churn)
  (print (call source))
  (call churn)
  (print (call source))
)

(blank)
(type-line + TextPair (is (tuple (: left String) (: right String))))

(fn-do _ kept-values _
  (param seed String)
  (let pair (call TextPair (str seed "-left") (str seed "-right")))
  (finally
    (print (str "cleanup " (. pair right)))
  )
  (for (in value (list (. pair left) (. pair right)))
    (yield value)
  )
)

(fn-do _ churn _
  (for (in index (..< 0 8))
    (let pair (call TextPair (str "garbage-" index) (str "discard-" index)))
    (call C:brixCollect)
  )
)
```

```output
live-left
live-right
cleanup live-right
unknown
```

```brix
(import /BxGC/BxGC.h)
(blank)

(main
  (for (in value (call kept-values "early"))
    (print value)
    (call churn)
    (break)
  )
)

(blank)
(type-line + TextPair (is (tuple (: left String) (: right String))))

(fn-do _ kept-values _
  (param seed String)
  (let pair (call TextPair (str seed "-left") (str seed "-right")))
  (finally
    (print (str "cleanup " (. pair right)))
  )
  (for (in value (list (. pair left) (. pair right)))
    (yield value)
  )
)

(fn-do _ churn _
  (for (in index (..< 0 8))
    (let pair (call TextPair (str "garbage-" index) (str "discard-" index)))
    (call C:brixCollect)
  )
)
```

```output
early-left
cleanup early-right
```

```brix
(import /BxGC/BxGC.h)
(blank)

(main
  (let left (call kept-iterator 2))
  (let right (call kept-iterator 5))
  (print (call left))
  (print (call right))
  (call churn)
  (print (call left))
  (print (call right))
  (call churn)
  (print (call left))
  (print (call right))
  (print (call left))
)

(blank)
(type-line + TextPair (is (tuple (: left String) (: right String))))

(fn-do _ kept-iterator _
  (param seed Int)
  (for (in value (list seed (+ seed 1) (+ seed 2)))
    (yield value)
  )
)

(fn-do _ churn _
  (for (in index (..< 0 8))
    (let pair (call TextPair (str "garbage-" index) (str "discard-" index)))
    (call C:brixCollect)
  )
)
```

```output
2
5
3
6
4
7
unknown
```

```brix
(import /BxGC/BxGC.h)
(blank)

(main
  (let captured (call capture-generator 10))
  (print (call captured))
  (call churn)
  (print (call captured))
  (print (call captured))
)

(blank)
(type-line + TextPair (is (tuple (: left String) (: right String))))

(fn-do _ capture-generator _
  (param seed Int)
  (let count seed)
  (let produce
    (=> () :do
      (block
        (yield count)
        (+= count 1)
        (yield count)
      )
    )
  )
  (return (call produce))
)

(fn-do _ churn _
  (for (in index (..< 0 8))
    (let pair (call TextPair (str "garbage-" index) (str "discard-" index)))
    (call C:brixCollect)
  )
)
```

```output
10
11
unknown
```

```brix
(import /BxGC/BxGC.h)
(blank)

(main
  (let result (catch (call collect-failing)))
  (print (is result GeneratorTrouble))
)

(blank)
(type-line + TextPair (is (tuple (: left String) (: right String))))

(type + GeneratorTrouble (is Error))

(fn-do _ failing-values _
  (param seed String)
  (let value (str seed " value"))
  (finally
    (print (str "cleanup " value))
  )
  (yield value)
  (throw (call GeneratorTrouble "expected"))
)

(fn-do _ collect-failing Int (?! GeneratorTrouble)
  (for (in value (call failing-values "failure"))
    (print value)
    (call churn)
  )
  (return 0)
)

(fn-do _ churn _
  (for (in index (..< 0 8))
    (let pair (call TextPair (str "garbage-" index) (str "discard-" index)))
    (call C:brixCollect)
  )
)
```

```output
failure value
cleanup failure value
true
```

```brix
(import /BxGC/BxGC.h)
(blank)

(main
  (let total 0)
  (for (in value (call borrowed-values))
    (+= (. value count) 10)
    (call churn)
    (+= total (. value count))
  )
  (print total)
)

(blank)
(type-line + TextPair (is (tuple (: left String) (: right String))))

(type + YieldRecord (is Struct)
  (prop + count Int)
)

(fn-do _ borrowed-values _
  (let value (call YieldRecord 3))
  (yield value)
  (+= (. value count) 1)
  (yield value)
)

(fn-do _ churn _
  (for (in index (..< 0 8))
    (let pair (call TextPair (str "garbage-" index) (str "discard-" index)))
    (call C:brixCollect)
  )
)
```

```output
27
```

```brix
(main
  (let source (call EntrySource))
  (let names (in (. each text) (. source entries) (when (!= (. each text) ""))))
  (let collected (list String) (call collect names))
  (print (str (. collected length) " " (call (. " " join) collected)))
)

(type + Entry
  (prop + text String)
  (fn-do + This _
    (param text String)
  )
)

(type + EntrySource
  (fn-do + entries (stream Entry) :get
    (throw (call Error "failed") (if false))
    (yield (call Entry "a.txt"))
    (yield (call Entry ""))
    (yield (call Entry "b.txt"))
  )
)

(fn-expr + (of collect Item) _
  (param stream (stream Item))
  (call List stream)
)
```

```output
2 a.txt b.txt
```

```brix
(main
  (for (in x (call pairs "a1"))
    (print x)
  )
)

(fn-do + pairs (stream String)
  (param text (of Indexable Char))
  (yield "start")
  (let got
    (match text
      (~ (seq (: w (chars "a-z")) (: n (chars "0-9")))
        (str n w)
      )
    )
  )
  (yield (?? got "none"))
)
```

```output
start
1a
```

### A function that yields reads a lazy local declared under the value it yields.

```brix
(main
  (for (in line (call format 3))
    (print line)
  )
  (print (call (. "\n" join) (call format 0)))
)

(fn-do + format _
  (param width U32)
  (yield "first")
  (if width
    (yield
      (scope (str "    " indent "^")
        (lazy indent (* "-" width))
      )
    )
  )
)
```

```output
first
    ---^
first
```

### A return written with only its condition ends what a function that yields makes.

```brix
(main
  (for (in n (call numbers 0))
    (print n)
  )
  (for (in n (call numbers 3))
    (print n)
  )
)

(fn-do + numbers (stream Int)
  (param count Int)
  (yield 1)
  (return-if (if (== count 0)))
  (yield count)
)
```

```output
1
1
3
```

### An override that yields and states no errors raises those of the base's generator when called through the base: an error of a class above them is cast to one as it leaves.

```brix
(main
  (do-new
    (let sources (list Source) (list (call Source) (call Counter 2) (call Counter 0)))
    (for (in source sources)
      (print (?? (catch? (call total source)) -1))
    )
    (on-failure failure
      (print failure)
    )
  )
)

(fn-do + total Int (?! Error)
  (param source Source)
  (let sum 0)
  (for (in value (call (. source values)))
    (+= sum value)
  )
  (return sum)
)

(type + Failed (is Error))

(type + Source
  (fn-do + values (stream Int)
    (if false
      (yield 0)
    )
    (throw (call Failed "not here"))
  )
)

(type + Counter (is Source)
  (prop + count Int)
  (fn-do + This _
    (param count _)
    (call super)
  )
  (fn-do + values (stream Int)
    (for (in step (..< 0 3))
      (yield step)
    )
    (if (== count 0)
      (throw (call Error "plain"))
    )
    (if (== count 1)
      (throw (call Failed "counted"))
    )
  )
)
```

```output
-1
3
Invalid cast to [Object __source__:Failed]
```

### A `use` in a generator ends where the generator ends, and where a loop leaves it after a yield.

```brix
(main
  (for (in c (call letters))
    (print (str "first " c))
    (break)
  )
  (print "mid")
  (for (in c (call letters))
    (print (str "all " c))
  )
  (print "after")
)

(fn-do + letters (stream Char)
  (let text (use (call Holder "xy")))
  (for (in c text)
    (yield c)
  )
)

(type + Holder
  (implements (of Usable String))
  (prop + text String)
  (fn-do + This _
    (param text String)
  )
  (fn-do + begin-using String
    (print "begin")
    (return text)
  )
  (fn-do + end-using _
    (print "end")
  )
)
```

```output
begin
first x
end
mid
begin
all x
all y
end
after
```

### A method that yields and raises nothing, in the place of a base's whose generator raises, takes the base's errors.

```brix
(main
  (let providers (list Provider) (list (call Provider) (call Demo)))
  (for (in provider providers)
    (print (?? (catch? (call run provider)) "failed"))
  )
)

(type + ProviderError (is Error))

(type + Provider
  (fn-do + stream (stream String)
    (param n I64)
    (if false
      (yield "never")
    )
    (throw (call ProviderError "not implemented"))
  )
)

(type + Demo (is Provider)
  (fn-do + stream (stream String)
    (param n I64)
    (yield (str "one " n))
    (yield (str "two " n))
  )
)

(fn-do _ run String (?! ProviderError)
  (param provider Provider)
  (let text "")
  (for (in word (call (. provider stream) 3))
    (+= text (str word ","))
  )
  (return text)
)
```

```output
failed
one 3,two 3,
```

### A comprehension over an index span.

```brix
(import /core/arena)
(import /core/array-builder)
(import /core/table)
(blank)

(main
  (let arena (call Arena))
  (let items (call (of ArrayBuilder U32) arena))
  (call (. items append) 4)
  (call (. items append) 5)
  (call (. items append) 6)
  (let ids (call (of IndexSpan U32) items (..< 1 3)))
  (print (call (. ", " join) (in (str each) ids)))
)
```

```output
5, 6
```

### A yield keeps what a test found of a property.

```brix
(main
  (for (in line (call lines (call Release "1.2")))
    (print line)
  )
)

(fn-do _ lines _
  (param release Release)
  (if (. release version)
    (yield "version")
    (yield (str (. release version length)))
  )
)

(type + Release (is Tuple)
  (prop + version (? String))
)
```

```output
version
3
```

### A generator's variadic tail with a default takes no items, one item, or several.

```brix
(main
  (for (in n (call count))
    (print n)
  )
  (for (in n (call count 10))
    (print n)
  )
  (for (in n (call count 10 20))
    (print n)
  )
)

(fn-do + count (stream Size)
  (param ... items (list Int) (= (call (of List Int))))
  (yield (. items length))
)
```

```output
0
1
2
```

### A stream has no truth to test, so a generator of one Bool does not become that Bool, with or without `as`, and a condition over it is an error too.

```brix compile-error
(main
  (print (as (call found (list 1 2 3) 2) Bool))
)

(fn-do + found (stream Bool)
  (param items (list Int))
  (param wanted Int)
  (yield (and (> (. items length) 0) (== ([] items 0) wanted)))
)
```

```output
Exception: Iterable'Bool cannot become Bool: it has no truth to test, and "as" does not read the values it gives; read them instead
<no file:3>
      (print (as (call found (list 1 2 3) 2) Bool))
             ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix compile-error
(main
  (let answer Bool (call found (list 1 2 3) 2))
  (print answer)
)

(fn-do + found (stream Bool)
  (param items (list Int))
  (param wanted Int)
  (yield (and (> (. items length) 0) (== ([] items 0) wanted)))
)
```

```output
Exception: Iterable'Bool cannot become Bool: it has no truth to test, and "as" does not read the values it gives; read them instead
<no file:3>
      (let answer Bool (call found (list 1 2 3) 2))
                  ^^^^
```

```brix compile-error
(main
  (print (if-expr (call found (list 1 2 3) 2) "found" "missing"))
)

(fn-do + found (stream Bool)
  (param items (list Int))
  (param wanted Int)
  (yield (and (> (. items length) 0) (== ([] items 0) wanted)))
)
```

```output
Exception: Iterable'Bool has no truth to test; read the values it gives instead
<no file:3>
      (print (if-expr (call found (list 1 2 3) 2) "found" "missing"))
                      ^^^^^^^^^^^^^^^^^^^^^^^^^^^
```
