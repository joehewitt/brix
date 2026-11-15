# Streams

A stream gives its items one at a time. Lists, text, ranges and arrays are streams, and so are
generators, comprehensions and channels, which make their items as they are read. A loop reads a
stream, a pipeline transforms one, and a target such as a list or a file takes its items.

This chapter covers the kinds of stream, generators, comprehensions, reading streams, pipelines,
targets, and making a type a stream. Loops are in Control flow, grammars over streams in Stream
patterns, and channels in Concurrency.

## The stream family

`(stream Int)`{.brix} is a stream of `Int` of any kind. A parameter of that type takes a list, a
range, an array, a generator, a comprehension, or a value of any type that implements one of the
stream interfaces.

```brix
(main
  (print (call total (list 1 2 3)))
  (print (call total (.. 1 4)))
  (print (call total (call evens 6)))
  (print (call total (in (* each 10) (list 1 2))))
  (print (call total (call (of Array Int) (list 5 5))))
  (print (call vowels "streams"))
)

(fn-do + total Int
  (param values (stream Int))
  (let sum 0)
  (for (in value values)
    (+= sum value)
  )
  (return sum)
)

(fn-do + evens (stream Int)
  (param limit Int)
  (for (in n (.. 0 limit (by: 2)))
    (yield n)
  )
)

(fn-do + vowels Int
  (param letters (stream Char))
  (let count 0)
  (for (in letter letters (when (is-in letter (list 'a' 'e' 'i' 'o' 'u'))))
    (+= count 1)
  )
  (return count)
)
```

```output
6
10
12
30
10
2
```

`(stream Int)`{.brix} is `Stream'Int`, which is one of these interfaces:

| Interface | Gives its items by | Implemented by |
| --- | --- | --- |
| `Indexable'Item` | `length` and `([] this index)`{.brix}. | `List`, `String`, `Range`, `Array`. |
| `CursorIterable'Item'State` | A state passed from one step to the next. | `String`, `Range`. |
| `Iterable'Item` | Reading the next item, `null` at the end. | Generators, comprehensions, channels. |
| `Broadcastable'Item` | Reading, by many readers at once. | `AsyncChannel`; see Concurrency. |
| `ImmutableArray'Item` | Its items in memory. | `ImmutableArray`; see Collections. |

- **Item conversion.** A stream's items convert to the item type a parameter wants, as a list of
  `I32` does for `(stream Int)`{.brix}.
- **Maps** give their entries as `KeyValuePair`s to a stream parameter; see Collections.
- **Lists.** A stream converts to a list wherever a list type is wanted, as in
  `(let all (list Int) values)`{.brix}, and `(call List values)`{.brix} makes one. Either reads
  the whole stream at once.
- **Readers alone.** A type that can only be read with `<-` is not a stream, and a loop over it is
  an error: `A for loop cannot iterate over Reader; loop over a list, a range or a stream`.

## Generators

A function with a step body that yields is a generator. Calling it makes a `Generator`, a stream
whose items the body makes as they are read.

```brix
(main
  (let numbers (call count-to 2))
  (print "made")
  (for (in n numbers)
    (print (str "got " n))
  )
  (for (in n numbers)
    (print "again")
  )
)

(fn-do + count-to (stream Int)
  (param limit Int)
  (finally
    (print "finished")
  )
  (print "started")
  (for (in n (.. 1 limit))
    (yield n)
    (print (str "after " n))
  )
)
```

```output
made
started
got 1
after 1
got 2
after 2
finished
```

- **Lazy.** Calling a generator runs none of its body. Each read runs the body up to its next
  yield, and the body's locals keep their values between reads.
- **One pass.** A generator gives its items once. A loop that ends, for any reason, finishes it,
  and a later loop over it reads nothing. A list or a range gives its items again to each loop.
- **Ending.** A generator ends when its body finishes, or at a `return` or a `return-if` whose
  condition holds. A value given to `return` is worked out and dropped.
- **Cleanup.** A `finally` form in a generator runs when it finishes: at its end, when a loop over it
  leaves early by `break`, `return` or an error, or when the generator raises.

### Yields

| Form | Gives |
| --- | --- |
| `(yield value)`{.brix} | One item. |
| `(yield? value)`{.brix} | One item, only when `value` is not `null`. |
| `(yield-all items)`{.brix} | Every item of a stream, including a call of the generator itself. |

```brix
(main
  (print (call List (call flatten (list (list 1 2) (list 3)))))
  (print (call List (call present (list 4 null 5))))
  (print (call List (call countdown 9 7)))
)

(fn-do + flatten (stream Int)
  (param groups (list (list Int)))
  (for (in group groups)
    (yield-all group)
  )
)

(fn-do + present (stream Int)
  (param items (list (? Int)))
  (for (in item items)
    (yield? item)
  )
)

(fn-do + countdown (stream Int)
  (param from Int)
  (param stop Int)
  (for (in n (.. from 0))
    (yield n)
    (return-if (if (== n stop)))
  )
)
```

```output
1, 2, 3
4, 5
9, 8, 7
```

### Item types

- **Declared.** A generator declared `(stream Int)`{.brix} converts each yield to `Int`. A yield
  that does not convert is an error: `Expected I64 here, but this is String, and there is no
  conversion between them`.
- **Inferred.** A generator with `_` for its result type gives the join of its yields, so yields
  of `1` and `2.5` make a `Generator'Float`. Yields with no join are an error: `Unable to unify
  generator yield types`.
- **Lists.** A function that yields and is declared a list type, such as `(list Int)`{.brix},
  runs its whole body at the call and returns the list of its yields.
- **Value bodies cannot yield**: `Function "gen" is not declared with do, so its body is one
  expression and cannot yield; write "+ do gen" and return its value with "= value"`.

A function value can yield too, with a step body: `(=> () :do (block (yield 1) (yield 2)))`{.brix}.

### Errors

A generator's errors are raised where its items are read. Its error types are declared as for any
function, after `?!`; see Errors.

```brix
(main
  (let read (catch (call List (call parsed (list "1" "x" "3")))))
  (print (is read Error))
  (for (in n (call parsed (list "4" "5")))
    (print n)
  )
)

(fn-do + parsed (stream Int) (?! Error)
  (param texts (list String))
  (for (in text texts)
    (throw (call Error (str "not a number: " text)) (if (== text "x")))
    (yield (as text Int))
  )
)
```

```output
true
4
5
```

A loop over a generator that raises raises the error, after running the generator's cleanup. A
raising generator passed where an `Iterable` is wanted panics when it raises, since `Iterable`
declares no errors.

## Comprehensions

A comprehension describes a stream by the items it is made from. Every comprehension is a
`Generator`.

```brix
(main
  (print (call List (in (* each 2) (list 1 2 3))))
  (print (call List (in (* each each) (.. 1 5) (when (!= (% each 2) 0)))))
  (print (call List (in each (.. 1 10) (while (< each 4)))))
  (let doubled
    (case-each (in n (.. 1 3))
      (* n 2)
    )
  )
  (print (call List doubled))
  (let sizes
    (iterate (in (list 1 5 10))
      (block
        (case? (> it 7) "big")
        (case? (> it 3) "middle")
        (default "small")
      )
    )
  )
  (print (call List sizes))
  (let pairs
    (iterate (in (list 1 2 3))
      (and-for (in letter (list "a" "b")))
      (block
        (str letter each)
      )
    )
  )
  (print (call List pairs))
)
```

```output
2, 4, 6
1, 9, 25
1, 2, 3
2, 4, 6
small, middle, big
a1, b2
```

| Form | Gives |
| --- | --- |
| `(in value source)`{.brix} | `value` for each item of `source`, which `value` reads as `each` or `it`. |
| `(case-each (in pattern source) body)`{.brix} | `body` for each item that fits `pattern`. Without a body, the items that fit. |
| `(iterate (in source) (block arms))`{.brix} | The value of the first arm that fits each item. Items that no arm takes are left out. |

- **Clauses.** `when`, `while` and `until` work as they do in a loop header: `when` skips items,
  and `while` and `until` end the stream. See Control flow.
- **Patterns.** The pattern of `case-each` binds names, as in a loop, and an item it does not fit
  is skipped. See Patterns.
- **Several sources.** An `and-for` form reads a second source beside the first, as in `pairs`,
  and the comprehension ends with the shorter one.
- **Laziness.** The source is worked out when the comprehension is made. The values are worked
  out as the items are read, and read locals as they are at that moment.
- **Lists.** A comprehension written where a list type is declared becomes a list at once.

## Reading streams

A loop reads a stream from the start; see Control flow. These read it in other ways:

| Form | Gives |
| --- | --- |
| `(<- reader)`{.brix} | The next item, or `null` at the end. |
| `(call List items)`{.brix} | A list of every item. |
| `(call (. ", " join) items)`{.brix}, `(call ", " items)`{.brix} | The items as text, with `", "` between them; see Text. |

```brix
(main
  (let numbers (call two))
  (print (<- numbers))
  (print (<- numbers))
  (print (?? (<- numbers) -1))
)

(fn-do + two (stream Int)
  (yield 1)
  (yield 2)
)
```

```output
1
2
-1
```

- **Readers.** `<-` reads generators, comprehensions, channels and other `Iterable`s. It does not
  read a list or a range, which a loop or an index reads instead.
- **Resuming.** A loop over a generator that has been read with `<-` starts at its next item.

The `/stream` library has functions over streams, such as `sort`, `sum`, `count`, `min`, `max`,
`first`, `distinct`, `take`, `skip`, `zip` and `indexed`. See the standard library.

## Pipelines

A pipeline passes a value through stages. Each stage operator takes the value on its left and an
expression on its right.

```brix
(main
  (let numbers (list 1 2 3 4 5 6))
  (print (call List (*> numbers (* each 10))))
  (print (call List (?> numbers (== (% each 2) 0))))
  (print (call List (?*> numbers (call half each))))
  (print (|> numbers sum-of))
  (print (|=> numbers (. it length)))
  (print (|> (*> (?> numbers (> each 3)) (* each each)) List))
  (print (|> 10 (call minus 3)))
)

(fn-expr + minus Int
  (param a Int)
  (param b Int)
  (- a b)
)

(fn-expr + half (? Int)
  (param n Int)
  (only-if (/ n 2) (if (== (% n 2) 0)))
)

(fn-do + sum-of Int
  (param values (stream Int))
  (let sum 0)
  (for (in value values)
    (+= sum value)
  )
  (return sum)
)
```

```output
10, 20, 30, 40, 50, 60
2, 4, 6
1, 2, 3
21
6
16, 25, 36
-7
```

| Operator | Gives |
| --- | --- |
| `(*> items value)`{.brix} | A stream of `value` for each item, read as `each`. |
| `(?> items condition)`{.brix} | A stream of the items for which `condition` holds. |
| `(?*> items value)`{.brix} | A stream of `value` for each item, leaving out the `null` ones. |
| `(*\|> items step)`{.brix} | A stream of the same items, running `step` for each as it passes. |
| `(\|> value function)`{.brix} | The function called with `value` as its last argument. |
| `(\|=> value expression)`{.brix} | `expression`, which reads the whole `value` as `it`. |
| `(-> value target)`{.brix} | Writes `value` to `target`, and gives the target. |
| `(*-> items target)`{.brix} | Writes each item to `target`, and gives the target. |

- **`each`.** In `*>`, `?>`, `?*>` and `*|>`, the right side is worked out for each item, which
  it reads as `each`. After a stage, `each` is the item the stage gives. `each` where nothing gives
  an item is an error: `Name "each" not found, as nothing here gives an item; "each" reads the item
  of a stage, a loop, a callback or a trigger`.
- **Lazy stages.** `*>`, `?>`, `?*>` and `*|>` make a stream and run nothing until it is read.
  Then each item passes through all the stages before the next item is read. A pipeline of these
  stages that nothing reads never runs.
- **Eager operators.** `|>`, `|=>`, `->` and `*->` run at once.
- **`|>`** adds the piped value after the arguments written in the call:
  `(|> 10 (call minus 3))`{.brix} calls `(call minus 3 10)`{.brix}. The piped value is worked out
  first. The right side may be a function, a call, a type such as `List`, or a method such as
  `(. ", " join)`{.brix}.
- **Chains.** One operator repeated has one form, as in `(*> items (* each 2) (+ each 1))`{.brix}.
  Mixed operators nest from the left.
- **One pass.** A pipeline kept in a local is a `Generator`, so it can be read once. Declare a list
  type to keep the items instead: `(let kept (list Int) (*> items (* each 2)))`{.brix}.
- **Records.** `(~> text \record)`{.brix} is a stage that reads one match of a stream pattern
  after another; see Stream patterns.

### Pipelines in rows of a block

A pipeline can be written as rows of a block: a row with the starting value, then a `pipe` row
for each stage, holding the stage's operator and value.

```brix
(main
  (let made
    (block
      (list 1 2 3 4 5 6)
      (pipe ?> (== (% each 2) 0))
      (pipe *> (* each 10))
      (pipe |> List)
    )
  )
  (print made)
  (let squares
    (block
      (bullet 1)
      (bullet* (.. 2 4))
      (pipe *> (* each each))
      (pipe |> List)
    )
  )
  (print squares)
  (print (call evens-between 1 9))
)

(fn-expr + evens-between (list Int)
  (param low Int)
  (param high Int)
  (.. low high)
  (pipe ?> (== (% each 2) 0))
)
```

```output
20, 40, 60
1, 4, 9, 16
2, 4, 6, 8
```

- **Bullets** start a stream: `(bullet value)`{.brix} gives one item, `(bullet* items)`{.brix}
  every item of a stream, and `(bullet? value)`{.brix} one item when it is not `null`.
- **Stage bodies.** A stage's value can be a `block` with rows of its own, and lazy bindings
  written under a stage are worked out for each item. To read an outer item inside an inner stage,
  bind it to a name in a lazy binding first, since the inner `each` hides it.

### Bullets in calls

A row of the form `(:name value)`{.brix} in a call gives an argument by name; in rows it is
written with a bullet, `•`. A named argument whose value reads `each` becomes a function of
`each`; see Functions.

### Errors in pipelines

A stage raises its errors where its items are read, not where the pipeline is made. A loop or a
`List` that reads the pipeline raises them, and `catch` around the reading catches them. A
pipeline that is never read never raises.

## Targets

A target takes items written to it. A list adds each item, an array fills itself from index 0, a
file writes text, and a channel sends. Any type that declares `<-` with a parameter, as in
`(fn-do + (infix this <- (param item Int)) Sink body)`{.brix}, is a target.

```brix
(main
  (let sink (call Sink))
  (<- sink 1)
  (-> 2 sink)
  (*-> (list 3 4) sink)
  (<-* sink (list 5 6))
  (print (. sink items))
  (let collected (list Int))
  (*-> (*> (.. 1 3) (* each 10)) collected)
  (print collected)
)

(type + Sink
  (prop + items (list Int))
  (fn-do + (infix this <- (param item Int)) Sink
    (+= items item)
    (return this)
  )
)
```

```output
1, 2, 3, 4, 5, 6
10, 20, 30
```

| Form | Writes |
| --- | --- |
| `(<- target value)`{.brix} | One value, and gives the target. |
| `(?<- target value)`{.brix} | One value, when `target` is not `null`. |
| `(<-* target items)`{.brix} | Each item of a stream. It is a step, not a value. |
| `(-> value target)`{.brix} | The whole value, worked out before the target. |
| `(*-> items target)`{.brix} | Each item, with the target worked out before the items. |

- **Item types.** `->` needs a `<-` that takes the value's type, and `*->` one that takes the item
  type. A type can declare both. A target with no fitting `<-` is an error that names the target
  first: `Operator "->" is not defined for I64 and [I64]`.
- **Errors.** A `<-` that raises stops `*->` at that item.

## Making a type a stream

A type becomes a stream by implementing one of the stream interfaces.

```brix
(main
  (for (in n (call Countdown 3))
    (print n)
  )
  (print (call List (call Steps 7)))
  (let squares (call Squares 4))
  (print (call List squares))
  (print ([] squares .last))
  (print (call (of List String) (call Pair "a" 2)))
)

(type + Countdown (implements (of Iterable Int))
  (prop + count Int)
  (fn-do + This _
    (param count _)
  )
  (fn-do + (prefix <- this) (? Int)
    (return-if null (if (== count 0)))
    (-= count 1)
    (return (+ count 1))
  )
)

(type + Steps (is (tuple (: limit Int)))
  (implements (of CursorIterable Int Int))
  (fn-expr + initial-iterator-state Int :get
    0
  )
  (fn-expr + next-iterator (tuple Int (? Int))
    (param state Int)
    (if-expr (> state limit) (tuple state null) (tuple (+ state 3) state))
  )
)

(type + Squares (is (tuple (: count Int)))
  (implements (of Indexable Int))
  (fn-expr + length Size :get
    count
  )
  (fn-expr + (index-get this (param index Index)) Int
    (* index index)
  )
)

(type + Pair (is (tuple (: name String) (: count Int)))
  (fn-to-do + (of Iterable String)
    (for (in n (..< 0 count))
      (yield name)
    )
  )
)
```

```output
3
2
1
0, 3, 6
0, 1, 4, 9
9
a, a
```

- **`Iterable`.** Declare `<-` with no parameter, as in
  `(fn-do + (prefix <- this) (? Int) body)`{.brix}, giving the next item, or `null` at the end. The object keeps its own place, so a second loop goes on where the first stopped.
  `begin-iterating` and `end-iterating`, when declared, run when a loop starts and ends.
- **`CursorIterable`.** Declare `initial-iterator-state` and `next-iterator`, which takes a state
  and gives the next state and the item, or `null` for the item at the end. Each loop starts from
  the initial state.
- **`Indexable`.** Declare `length` and an index, as in
  `(fn-expr + (index-get this (param index Index)) Int body)`{.brix}. A loop reads the length
  once, then each index. Positions such as `.last` and `reversed` come with it.
- **A conversion with yields.** A type that declares a conversion to `Iterable` with a step body,
  as `Pair` does, can be looped over and passed as a stream.
