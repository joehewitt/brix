# Property changes

```brix
(import ./base)
(blank)

(main
  (let value (call Derived))
  (print (. value result))
  (= (. value input) 7)
  (print (. value result))
)

(type + Derived (is Base))
```

```brix file=base.bx
(type + Base
  (prop + input _ (= 2))
  (prop + result _ (= (call transform #input)))
)

(fn-expr _ transform _
  (param value Int)
  (* value 3)
)
```

```output
6
21
```

```brix
(main
  (let f (call Foo))
  (print (. f foo))
  (blank)
  (= (. f bar) 50)
  (print (. f foo))
  (blank)
  (= (. f bar) 25)
  (print (. f foo))
)

(type + Foo
  (prop + foo _ (= (* #bar 2)))
  (prop + bar _ (= 42))
)
```

```output
84
100
50
```

```brix crash
(main
  (let thing (call Thing))
  (let name (. thing #name))
  (print name)
)

(type + Thing
  (prop + prefix _ (= ""))
  (fn-expr + name _ :get
    (+ "bob" prefix)
  )
)
```

```output
Exception: Tracked getters are not supported; track concrete state instead
<no file:4>
      (let name (. thing #name))
                ^^^^^^^^^^^^^^^
```

### A tracked read in a lazy property's value is reported, as the value is worked out once.

```brix crash
(main
  (let f (call Foo))
  (print (. f later))
)

(type + Foo
  (prop + a _ (= 1))
  (prop + later _ (:= (* #a 3)))
)
```

```output
Exception: A lazy property is worked out once, when it is first read, so it cannot follow "#a"; write "=" in place of ":=" to keep it live, or read "a" without "#"
<no file:9>
      (prop + later _ (:= (* #a 3)))
                             ^^
```

### A tracked read of each item of a pipeline that a call in a derived value works out is followed: the value runs again as an item changes and as items are added and removed, and no longer for an item removed.

```brix
(main
  (let b (call Box))
  (print (. b waiting))
  (= (. ([] (. b items) 0) is-pending) false)
  (print (. b waiting))
  (let added (call Item))
  (+= (. b items) added)
  (print (. b waiting))
  (= (. added is-pending) false)
  (print (. b waiting))
  (let removed ([] (. b items) 1))
  (call (. b items remove) 1)
  (print (. b waiting))
  (= (. removed is-pending) false)
  (print (. b waiting))
)

(type + Item
  (prop + is-pending _ (= true))
)

(type + Box
  (prop + items _ (= (list (call Item) (call Item) (call Item))))
  (prop + waiting _ (= (call noted (call tally (in each #items (when (. each #is-pending)))))))
)

(fn-do + noted Int
  (param n Int)
  (print (str "ran " n))
  (return n)
)

(fn-do + (of tally T) _
  (param stream (stream T))
  (let total 0)
  (for (in item stream)
    (+= total 1)
  )
  (return total)
)
```

```output
ran 3
3
ran 2
2
ran 3
3
ran 2
2
ran 1
1
1
```

### The items a pipeline reads in the value of an object made while another value runs are followed by that object's value alone: the other value runs again only for what it reads itself.

```brix
(main
  (let o (call Outer))
  (print (. o made waiting))
  (= (. ([] (. o made items) 0) is-pending) false)
  (print (. o made waiting))
  (= (. o flag) true)
  (print (. o made waiting))
)

(type + Item
  (prop + is-pending _ (= true))
)

(type + Box
  (prop + items _ (= (list (call Item) (call Item))))
  (prop + waiting _ (= (call noted "box" (call tally (in each #items (when (. each #is-pending)))))))
)

(type + Outer
  (prop + flag _ (= false))
  (prop + made _ (= (call built #flag)))
)

(fn-do + built Box
  (param flag Bool)
  (print (str "outer ran " flag))
  (return (call Box))
)

(fn-do + noted Int
  (param what String)
  (param n Int)
  (print (str what " ran " n))
  (return n)
)

(fn-do + (of tally T) _
  (param stream (stream T))
  (let total 0)
  (for (in item stream)
    (+= total 1)
  )
  (return total)
)
```

```output
outer ran false
box ran 2
2
box ran 1
1
outer ran true
box ran 2
2
```

### A tracked read of a list for each item of a pipeline follows the changes made to each list in place.

```brix
(main
  (let b (call Box))
  (print (. b filled))
  (+= (. ([] (. b groups) 1) members) 3)
  (print (. b filled))
  (call (. ([] (. b groups) 1) members clear))
  (print (. b filled))
)

(type + Group
  (prop + members _ (= (call (of List Int))))
)

(type + Box
  (prop + groups _ (= (list (call Group) (call Group))))
  (prop + filled _ (= (call tally (in each #groups (when (> (. each #members length) 0))))))
)

(fn-do + (of tally T) _
  (param stream (stream T))
  (let total 0)
  (for (in item stream)
    (+= total 1)
  )
  (return total)
)
```

```output
0
1
0
```

### A rule that works out a pipeline follows a tracked read of each item, running as the count of them changes.

```brix
(main
  (let b (call Box))
  (= (. ([] (. b items) 0) is-pending) false)
  (= (. ([] (. b items) 1) is-pending) false)
  (= (. ([] (. b items) 0) is-pending) true)
)

(type + Item
  (prop + is-pending _ (= true))
)

(type + Box
  (prop + items _ (= (list (call Item) (call Item))))
  (whenever-rule (call tally (in each #items (when (. each #is-pending))))
    (print (str "waiting " waiting))
    (blank)
    (lazy waiting (call tally (in each items (when (. each is-pending)))))
  )
)

(fn-do + (of tally T) _
  (param stream (stream T))
  (let total 0)
  (for (in item stream)
    (+= total 1)
  )
  (return total)
)
```

```output
waiting 1
waiting 0
waiting 1
```

### A tracked read of each item of a pipeline that is the whole value is reported: the pipeline is worked out after the value, where nothing follows it.

```brix crash
(main
  (let b (call Box))
  (print (. b waiting))
)

(type + Item
  (prop + is-pending _ (= true))
)

(type + Box
  (prop + items _ (= (list (call Item))))
  (prop + waiting _ (= (in each #items (when (. each #is-pending)))))
)
```

```output
Exception: "#is-pending" is read for each item of a pipeline, which is followed only where the pipeline is given to a call in a live value, as in count(each in #items when each.#is-done); write "is-pending" to read it plainly
<no file:13>
      (prop + waiting _ (= (in each #items (when (. each #is-pending)))))
                                                 ^^^^^^^^^^^^^^^^^^^^
```

### A pipeline given to a function that hands it back, as a generator does, is followed where what it hands back is given to a function that works it through in the same value: the reads happen in that call. One whose loop works it through follows it directly.

```brix
(main
  (let b (call Bag))
  (print (str (. b latest) " " (. b counted)))
  (= (. ([] (. b items) 1) n) 7)
  (print (str (. b latest) " " (. b counted)))
  (= (. ([] (. b items) 0) n) 0)
  (print (str (. b latest) " " (. b counted)))
)

(type + Bag
  (prop + items _ (= (list (call Item) (call Item))))
  (prop + latest _ (= (?? (call last-of (call pass (in (. each n) #items (when (> (. each #n) 1))))) 0)))
  (prop + counted _ (= (call tally (call later (in each #items (when (> (. each #n) 1)))))))
)

(type + Item
  (prop + n _ (= 2))
)

(fn-do + (of tally T) _
  (param stream (stream T))
  (let total 0)
  (for (in item stream)
    (+= total 1)
  )
  (return total)
)

(fn-do + (of last-of T) (? T)
  (param stream (stream T))
  (let found (? T))
  (for (in item stream)
    (= found item)
  )
  (return found)
)

(fn-do + (of pass T) _
  (param stream (stream T))
  (for (in item stream)
    (yield item)
  )
)

(fn-do + (of later T) _
  (param stream (stream T))
  (return stream)
)

(fn-do + hold _
  (param sink (list (of Generator Int)))
  (param stream (of Generator Int))
  (+= sink stream)
  (return 0)
)
```

```output
2 2
7 2
7 1
```

### A pipeline given to a function that keeps it, rather than working it through as it is called, is reported where it reads an item with "#".

```brix crash
(main
  (let b (call Bag))
  (print (. b kept))
)

(type + Bag
  (prop + items _ (= (list (call Item) (call Item))))
  (prop + sink (list (of Generator Int)))
  (prop + kept _ (= (call hold sink (in (. each n) #items (when (> (. each #n) 1))))))
)

(type + Item
  (prop + n _ (= 2))
)

(fn-do + (of tally T) _
  (param stream (stream T))
  (let total 0)
  (for (in item stream)
    (+= total 1)
  )
  (return total)
)

(fn-do + (of last-of T) (? T)
  (param stream (stream T))
  (let found (? T))
  (for (in item stream)
    (= found item)
  )
  (return found)
)

(fn-do + (of pass T) _
  (param stream (stream T))
  (for (in item stream)
    (yield item)
  )
)

(fn-do + (of later T) _
  (param stream (stream T))
  (return stream)
)

(fn-do + hold _
  (param sink (list (of Generator Int)))
  (param stream (of Generator Int))
  (+= sink stream)
  (return 0)
)
```

```output
Exception: "#n" is read for each item of a pipeline that "hold" keeps rather than working through as it is called, so the read would not be followed; give the pipeline to a function that works it through, as count does, or write "n" to read it plainly
<no file:10>
      (prop + kept _ (= (call hold sink (in (. each n) #items (when (> (. each #n) 1))))))
                                                                       ^^^^^^^^^^^
```

### A pipeline that a function hands back is reported where nothing in the value works through what it hands back. A pipeline with no "#" read may be given to any function.

```brix crash
(main
  (let b (call Bag))
  (print (. b kept))
)

(type + Bag
  (prop + items _ (= (list (call Item) (call Item))))
  (prop + sink (list (of Generator Int)))
  (prop + plain _ (= (call hold sink (in (. each n) items (when (> (. each n) 1))))))
  (prop + kept _ (= (call later (in (. each n) #items (when (> (. each #n) 1))))))
)

(type + Item
  (prop + n _ (= 2))
)

(fn-do + (of tally T) _
  (param stream (stream T))
  (let total 0)
  (for (in item stream)
    (+= total 1)
  )
  (return total)
)

(fn-do + (of last-of T) (? T)
  (param stream (stream T))
  (let found (? T))
  (for (in item stream)
    (= found item)
  )
  (return found)
)

(fn-do + (of pass T) _
  (param stream (stream T))
  (for (in item stream)
    (yield item)
  )
)

(fn-do + (of later T) _
  (param stream (stream T))
  (return stream)
)

(fn-do + hold _
  (param sink (list (of Generator Int)))
  (param stream (of Generator Int))
  (+= sink stream)
  (return 0)
)
```

```output
Exception: "#n" is read for each item of a pipeline that "later" hands back without working it through, and nothing works through what it hands back as the value is made; give that to a function that works it through, as count does, or write "n" to read it plainly
<no file:11>
      (prop + kept _ (= (call later (in (. each n) #items (when (> (. each #n) 1))))))
                                                                   ^^^^^^^^^^^
```

### A pipeline given to a method, or by name, or piped with "|>", is followed by the parameter it fills, wherever that parameter is in the list.

```brix
(main
  (let b (call Bag))
  (print (str (. b by-method) " " (. b by-default) " " (. b by-pipe) " " (. b by-named)))
  (= (. ([] (. b items) 0) n) 0)
  (print (str (. b by-method) " " (. b by-default) " " (. b by-pipe) " " (. b by-named)))
)

(type + Bag
  (prop + items _ (= (list (call Item) (call Item))))
  (prop + counter _ (= (call Counter)))
  (prop + by-method _ (= (call (. counter tally) 10 (in (. each n) #items (when (> (. each #n) 1))))))
  (prop + by-default _ (= (call scaled 0 (in (. each n) #items (when (> (. each #n) 1))))))
  (prop + by-pipe _ (= (|> (in (. each n) #items (when (> (. each #n) 1))) (call piped 100))))
  (prop + by-named _
    (=
      (call scaled 0
        (block
          (:scale 3)
          (:stream (in (. each n) #items (when (> (. each #n) 1))))
        )
      )
    )
  )
)

(type + Item
  (prop + n _ (= 2))
)

(type + Counter
  (prop + base _ (= 1))
  (fn-do + tally Int
    (param scale Int)
    (param stream (of Generator Int))
    (let total base)
    (for (in item stream)
      (+= total scale)
    )
    (return total)
  )
)

(fn-do + scaled Int
  (param base Int)
  (param stream (of Generator Int))
  (param scale Int (= 2))
  (let total base)
  (for (in item stream)
    (+= total scale)
  )
  (return total)
)

(fn-do + piped Int
  (param scale Int)
  (param stream (of Generator Int))
  (let total 0)
  (for (in item stream)
    (+= total scale)
  )
  (return total)
)
```

```output
21 4 200 6
11 2 100 3
```

```brix
(main
  (let foo (call Foo))
  (print (. foo foo))
  (blank)
  (= (. foo bar count) 2)
  (blank)
  (print (. foo foo))
  (blank)
  (= (. foo bar) (call Bar 5))
  (blank)
  (print (. foo foo))
)

(type + Foo
  (prop + foo _ (= (* (. bar #count) 10)))
  (prop + bar _ (= (call Bar 1)))
)

(type + Bar
  (prop + count _ (= 1))
  (fn-do + This _
    (param count _)
  )
)
```

```output
10
20
50
```

```brix
(prop + bb _ (= (call Bar 1)))
(blank)

(main
  (let foo (call Foo))
  (print (. foo foo))
  (= (. bb count) 2)
  (print (. foo foo))
  (= (. bb count) 3)
  (print (. foo foo))
)

(type + Foo
  (prop + foo _
    (=
      (scope (* (. bar #count) 10)
        (lazy bar bb)
      )
    )
  )
)

(type + Bar
  (prop + count _ (= 1))
  (fn-do + This _
    (param count _)
  )
)
```

```output
10
20
30
```

### Cross-class tracked setter dispatch is finalized after later reader compilation

```brix
(main
  (let writer (call Writer))
  (call (. writer set) 1)
  (let reader (call Reader writer))
  (print (. reader mirror))
  (call (. writer set) 2)
  (print (. reader mirror))
)

(type + Writer
  (prop + value _ (= 0))
  (fn-do + set _
    (param next Int)
    (= value next)
  )
)

(type + Reader
  (prop + writer Writer)
  (prop + mirror _ (= (* (. writer #value) 10)))
  (fn-do + This _
    (param writer _)
  )
)
```

```output
10
20
```

```brix crash
(prop + bar _ (= 42))
(blank)

(main
  (let foo (call Foo))
)

(type + Foo
  (prop + foo _ (= (* #bar 2)))
)
```

```output
Exception: Untrackable name
<no file:10>
      (prop + foo _ (= (* #bar 2)))
                          ^^^^
```

```brix
(main
  (let foo (call Foo))
  (print (. foo foo))
)

(type + Foo
  (prop + foo _
    (=
      (block
        (let bar 42)
        (* #bar 2)
      )
    )
  )
)
```

```output
84
```

```brix
(main
  (let foo (call Foo))
  (= (. foo bar) 10)
  (= (. foo bar) 5)
  (= (. foo bar) 5)
  (= (. foo bar) 10)
  (= (. foo bar) 10)
  (= (. foo bar) 5)
)

(type + Foo
  (prop + bar _ (= 0))
  (when-rule (< #bar 10)
    (print "lesser")
  )
  (when-rule (>= #bar 10)
    (print "greater")
  )
)
```

```output
lesser
greater
lesser
greater
lesser
```

### whenever observes every value change without an initial fire

```brix
(main
  (let foo (call Foo))
  (= (. foo bar) 1)
  (= (. foo bar) 2)
  (= (. foo bar) 2)
  (= (. foo bar) 0)
  (= (. foo bar) 0)
  (= (. foo bar) 3)
)

(type + Foo
  (prop + bar _ (= 0))
  (whenever-rule #bar
    (print (str "bar " bar))
  )
)
```

```output
bar 1
bar 2
bar 0
bar 3
```

### whenever over an interface-typed property fires when the value switches concrete type

```brix
(main
  (let holder (call ShapeHolder))
  (let round (call RoundShape 1))
  (let square (call SquareShape 2))
  (= (. holder shape) round)
  (= (. holder shape) square)
  (= (. holder shape) square)
  (= (. holder shape) round)
  (= (. holder shape) null)
)

(type + ShapeKind (is Interface)
  (fn-expr + label String :get)
)

(type + RoundShape
  (implements ShapeKind)
  (prop + size I64)
  (fn-do + This _
    (param size _)
  )
  (fn-expr + label String :get
    (str "round " size)
  )
)

(type + SquareShape
  (implements ShapeKind)
  (prop + size I64)
  (fn-do + This _
    (param size _)
  )
  (fn-expr + label String :get
    (str "square " size)
  )
)

(type + ShapeHolder
  (prop + shape (? ShapeKind))
  (whenever-rule #shape
    (print (?? (?. shape label) "none"))
  )
)
```

```output
round 1
square 2
round 1
none
```

### whenever over a list of interface values fires when an element switches concrete type, and equality elsewhere still refuses

```brix
(main
  (let holder (call ShapeList))
  (= (. holder shapes) (call shapes-of (call RoundShape 1)))
  (= (. holder shapes) (call shapes-of (call SquareShape 2)))
  (= (. holder shapes) (call shapes-of (call SquareShape 2)))
  (= (. holder shapes) (call shapes-of (call SquareShape 3)))
  (= (. holder shapes) (call shapes-of (call RoundShape 3)))
  (call mismatch (call RoundShape 1) (call SquareShape 1))
)

(type + ShapeKind (is Interface)
  (fn-expr + label String :get)
)

(type + RoundShape (is (tuple (: size I64)))
  (implements ShapeKind)
  (fn-expr + label String :get
    (str "round " size)
  )
)

(type + SquareShape (is (tuple (: size I64)))
  (implements ShapeKind)
  (fn-expr + label String :get
    (str "square " size)
  )
)

(fn-do + shapes-of (list ShapeKind)
  (param shape ShapeKind)
  (let shapes (list ShapeKind))
  (+= shapes shape)
  (return shapes)
)

(fn-do + mismatch _
  (param left ShapeKind)
  (param right ShapeKind)
  (let attempt
    (do-new
      (print (== left right))
      (on-failure failure
        (let failure (? Panic) failure)
        (if failure
          (print (str failure))
        )
      )
    )
  )
  (call (. attempt wait))
)

(type + ShapeList
  (prop + shapes (list ShapeKind))
  (whenever-rule #shapes
    (print (. ([] shapes 0) label))
    (if (== (. ([] shapes 0) label) "round 3")
      (call mismatch (call RoundShape 1) (call SquareShape 1))
    )
  )
)
```

```output
round 1
square 2
square 3
round 3
Unsupported interface equality: concrete types differ
Unsupported interface equality: concrete types differ
```

### whenever over a value that holds an interface value fires when the held value switches concrete type

```brix
(main
  (let holder (call FrameHolder))
  (= (. holder frame) (call Frame (call RoundShape 1) 1))
  (= (. holder frame) (call Frame (call SquareShape 2) 1))
  (= (. holder frame) (call Frame (call SquareShape 2) 1))
  (= (. holder frame) (call Frame (call RoundShape 2) 1))
)

(type + ShapeKind (is Interface)
  (fn-expr + label String :get)
)

(type + RoundShape (is (tuple (: size I64)))
  (implements ShapeKind)
  (fn-expr + label String :get
    (str "round " size)
  )
)

(type + SquareShape (is (tuple (: size I64)))
  (implements ShapeKind)
  (fn-expr + label String :get
    (str "square " size)
  )
)

(type + Frame (is (tuple (: inner ShapeKind) (: marker I64)))
  (implements ShapeKind)
  (fn-expr + label String :get
    (str "framed " (. inner label))
  )
)

(type + FrameHolder
  (prop + frame (? ShapeKind))
  (whenever-rule #frame
    (print (?? (?. frame label) "none"))
  )
)
```

```output
framed round 1
framed square 2
framed round 2
```

### whenever value triggers require a tracked read

```brix crash
(main
  (let foo (call Foo))
  (= (. foo bar) 1)
)

(type + Foo
  (prop + bar _ (= 0))
  (whenever-rule bar
    (print (str "bar " bar))
  )
)
```

```output
Exception: whenever value triggers must read at least one tracked property (like #bar)
<no file:9>
      (whenever-rule bar
                     ^^^
```

## Dependencies

### Same-value primitive property stores keep the value but do not dispatch to live subscribers

```brix
(main
  (let foo (call Foo))
  (print (. foo value))
  (= (. foo value) 0)
  (print (. foo value))
  (= (. foo value) 1)
  (print (. foo value))
  (= (. foo value) 1)
  (print (. foo value))
)

(type + Foo
  (prop + value _ (= 0))
  (blank)
  (depends changed value)
  (fn-do + changed _
    (print (tuple "changed" value))
  )
)
```

```output
0
0
(changed, 1)
1
1
```

### Primitive store gates compare floating-point bits rather than IEEE equality

```brix
(main
  (let foo (call Foo))
  (= (. foo value) 0.0)
  (= (. foo value) -0.0)
  (= (. foo value) -0.0)
)

(type + Foo
  (prop + value _ (= 0.0))
  (blank)
  (depends changed value)
  (fn-do + changed _
    (print "changed")
  )
)
```

```output
changed
```

### Property stores compare references by identity, not content equality

```brix
(main
  (let first (call Thing "same"))
  (let second (call Thing "same"))
  (let holder (call Holder first))
  (= (. holder thing) first)
  (print (. holder thing name))
  (= (. holder thing) second)
  (print (. holder thing name))
  (= (. holder thing) second)
  (print (. holder thing name))
)

(type + Thing
  (prop + name String)
  (fn-do + This _
    (param name _)
  )
)

(type + Holder
  (prop + thing Thing)
  (blank)
  (depends changed thing)
  (fn-do + This _
    (param thing _)
  )
  (fn-do + changed _
    (print (tuple "changed" (. thing name)))
  )
)
```

```output
same
(changed, same)
same
same
```

### Same-value primitive stores preserve tracked module-local memoization

```brix
(main
  (lazy value 1)
  (lazy generation 0)
  (lazy observed (call IntSnapshot #value generation))
  (blank)
  (print (. observed generation))
  (= generation 1)
  (= value 1)
  (print (. observed generation))
  (= generation 2)
  (= value 2)
  (print (. observed generation))
  (= generation 3)
  (= value 2)
  (print (. observed generation))
)

(type _ IntSnapshot (is (tuple (: value Int) (: generation Int))))
```

```output
0
0
2
2
```

### Tracked module-local reference stores use identity rather than content equality

```brix
(main
  (lazy first (call Thing "same"))
  (lazy second (call Thing "same"))
  (lazy value first)
  (lazy generation 0)
  (lazy observed (call ThingSnapshot #value generation))
  (blank)
  (print (. observed generation))
  (= generation 1)
  (= value first)
  (print (. observed generation))
  (= generation 2)
  (= value second)
  (print (. observed generation))
  (= generation 3)
  (= value second)
  (print (. observed generation))
)

(type + Thing
  (prop + name String)
  (fn-do + This _
    (param name _)
  )
)

(type _ ThingSnapshot (is (tuple (: value Thing) (: generation Int))))
```

```output
0
0
2
2
```

```brix
(main
  (let foo (call Foo))
  (print (. foo x))
  (= (. foo x) 1)
  (= (. foo x) 2)
  (= (. foo x) 3)
)

(type + Foo
  (prop + x _ (= 0))
  (blank)
  (depends bar x)
  (fn-do + bar _
    (print (tuple "bar" x))
  )
)
```

```output
0
(bar, 1)
(bar, 2)
(bar, 3)
```

```brix crash
(main
  (let foo (call Foo))
)

(type + Foo
  (prop + x _ (= 0))
  (blank)
  (depends bar x)
  (fn-do + bar _
    (param nd _)
    (print (tuple "bar" x))
  )
)
```

```output
Exception: A dependency calls "bar" with nothing, but it takes "nd"; give "nd" a default, or name a method that takes nothing
<no file:9>
      (depends bar x)
               ^^^
```

```brix
(main
  (let foo (call Foo))
  (print (. foo x))
  (= (. foo x) 1)
)

(type + Foo
  (prop + x _ (= 0))
  (blank)
  (depends bar x)
  (fn-do + bar _
    (param n _ (= 0))
    (print (tuple "bar" x n))
  )
)
```

```output
0
(bar, 1, 0)
```

```brix
(main
  (let foo (call Bar))
  (print (. foo x))
  (= (. foo x) 1)
  (= (. foo x) 2)
  (= (. foo x) 3)
)

(type + Foo
  (prop + x _ (= 0))
  (blank)
  (depends bar x)
  (fn-do + bar _
    (print (tuple "bar" x))
  )
)

(type + Bar (is Foo))
```

```output
0
(bar, 1)
(bar, 2)
(bar, 3)
```

```brix crash
(main
  (let foo (call Foo))
)

(type + Foo
  (depends bar x)
  (fn-do + bar _
    (print (tuple "bar" x))
  )
)
```

```output
Exception: Property "x" does not exist
<no file:7>
      (depends bar x)
      ^^^^^^^^^^^^^^^
```

```brix crash
(main
  (let foo (call Foo))
)

(type + Foo
  (prop + x _ (= 0))
  (blank)
  (depends bar x)
)
```

```output
Exception: Function "bar" does not exist
<no file:9>
      (depends bar x)
      ^^^^^^^^^^^^^^^
```

```brix
(main
  (lazy foo #bar)
  (lazy bar 1)
  (print foo)
  (= bar 2)
  (print foo)
  (= bar 3)
  (print foo)
)
```

```output
1
2
3
```

```brix
(main
  (lazy foo (call Thing))
  (lazy bar (call Thing))
  (blank)
  (= (. foo name) (. bar #name))
  (print (. foo name))
  (= (. bar name) "tom")
  (print (. foo name))
  (= (. bar name) "fred")
  (print (. foo name))
)

(type + Thing
  (prop + name _ (= "bob"))
)
```

```output
bob
tom
fred
```

```brix
(main
  (lazy foo (call Thing))
  (lazy bar (call Thing))
  (blank)
  (= (. foo name) (. bar #name))
  (print (. foo echo))
  (= (. bar name) "tom")
  (print (. foo echo))
  (= (. bar name) "fred")
  (print (. foo echo))
)

(type + Thing
  (prop + name _ (= "bob"))
  (prop + echo _ (= ""))
  (content
    (= echo #name)
  )
  (fn-do + (infix this += (param child Foo)) _
    (= content child)
  )
)
```

```output
bob
tom
fred
```

Canonical ids, rather than the display name String, retain a stable tracker. Reading one
dependency twice in the same evaluation still emits one membership.

Reordering preserves both tracker objects and their new observation order.

Removing one dependency, adding another, and replacing the owner have exact membership effects.

Runtime names retain a String-equality fallback without colliding with compiler ids.

```brix
(import /core/tracker)
(blank)

(main
  (let owner-a (call DependencyIdentityOwner))
  (let owner-b (call DependencyIdentityOwner))
  (let first-subscribers (list PropertySubscriber))
  (let second-subscribers (list PropertySubscriber))
  (let third-subscribers (list PropertySubscriber))
  (let previous (list Tracker) (call (of List Tracker)))
  (let next (list Tracker) (call (of List Tracker)))
  (blank)
  (let first (call track-property-dependency previous next "first" 1 first-subscribers (as owner-a Pointer)))
  (let repeated (call track-property-dependency previous next "renamed-first" 1 first-subscribers (as owner-a Pointer)))
  (if (or (!= (as first Pointer) (as repeated Pointer)) (!= (. next length) 1))
    (throw (call Panic "canonical property identity did not deduplicate a repeated read"))
  )
  (blank)
  (let second (call track-property-dependency previous next "second" 2 second-subscribers (as owner-a Pointer)))
  (= previous next)
  (= next (call (of List Tracker)))
  (blank)
  (let reordered-second (call track-property-dependency previous next "second" 2 second-subscribers (as owner-a Pointer)))
  (let reordered-first (call track-property-dependency previous next "first" 1 first-subscribers (as owner-a Pointer)))
  (if (or (!= (as reordered-second Pointer) (as second Pointer)) (!= (as reordered-first Pointer) (as first Pointer)))
    (throw (call Panic "reordered dependencies lost their canonical trackers"))
  )
  (if (or (!= (. next length) 2) (!= (as ([] next 0) Pointer) (as second Pointer)) (!= (as ([] next 1) Pointer) (as first Pointer)))
    (throw (call Panic "reordered dependency set did not preserve observation order"))
  )
  (blank)
  (= previous next)
  (= next (call (of List Tracker)))
  (blank)
  (let retained-first (call track-property-dependency previous next "first" 1 first-subscribers (as owner-a Pointer)))
  (let added-third (call track-property-dependency previous next "third" 3 third-subscribers (as owner-a Pointer)))
  (if (or (!= (. next length) 2) (!= (as retained-first Pointer) (as first Pointer)))
    (throw (call Panic "added/removed dependency set did not retain its unchanged membership"))
  )
  (if (or (== (as added-third Pointer) (as first Pointer)) (== (as added-third Pointer) (as second Pointer)))
    (throw (call Panic "new dependency reused an unrelated tracker"))
  )
  (blank)
  (= previous next)
  (= next (call (of List Tracker)))
  (let replaced-owner (call track-property-dependency previous next "first" 1 first-subscribers (as owner-b Pointer)))
  (if (or (== (as replaced-owner Pointer) (as retained-first Pointer)) (!= (. next length) 1))
    (throw (call Panic "replaced dependency owner retained the old owner's tracker"))
  )
  (blank)
  (= previous (call (of List Tracker)))
  (= next (call (of List Tracker)))
  (let dynamic-first (call track-property-dependency previous next "dynamic" first-subscribers (as owner-a Pointer)))
  (= previous next)
  (= next (call (of List Tracker)))
  (let dynamic-reused (call track-property-dependency previous next "dynamic" first-subscribers (as owner-a Pointer)))
  (let dynamic-other (call track-property-dependency previous next "other" first-subscribers (as owner-a Pointer)))
  (if (or (!= (as dynamic-reused Pointer) (as dynamic-first Pointer)) (!= (. next length) 2))
    (throw (call Panic "dynamic property-name fallback did not preserve exact membership"))
  )
  (if (== (as dynamic-other Pointer) (as dynamic-first Pointer))
    (throw (call Panic "different dynamic property names shared one tracker"))
  )
  (blank)
  (print "property-dependency-identity-ok")
)

(type + DependencyIdentityOwner)
```

```output
property-dependency-identity-ok
```

### Live actions compare heterogeneous tracker implementations by identity.

```brix
(import /core/tracker)
(blank)

(main
  (let owner (call TrackerRefreshOwner))
  (let trackers (list Tracker) (call (of List Tracker)))
  (+= trackers (as (call FirstRefreshTracker) Tracker))
  (let callback SubscriberCallback
    (=> () :do
      (block
        (do
          (inline (. owner value))
        )
      )
    )
  )
  (let action (call LiveAction trackers callback))
  (call action)
  (call (. action close))
  (print "heterogeneous-tracker-refresh-ok")
)

(type + FirstRefreshTracker
  (implements Tracker)
  (fn-do + attach _
    (param action LiveAction)
    (do
      (inline action)
    )
  )
  (fn-do + detach _
    (param action LiveAction)
    (do
      (inline action)
    )
  )
)

(type + TrackerRefreshOwner
  (prop + value _ (= 0))
)
```

```output
heterogeneous-tracker-refresh-ok
```

### Property subscriptions retain live actions weakly and release their side-payload lane with the observed object.

```brix
(import /BxGC/BxGC.h)
(blank)
(prop + property-change-observer-reclaimed _ (= false))
(blank)

(main
  (let foo (call Foo))
  (if (call C:brixGCHasObjectSidePayload (as foo Pointer) 0)
    (print "has")
  )
  (call C:brixGCClearObjectSidePayload (as foo Pointer) 0)
  (if (not (call C:brixGCHasObjectSidePayload (as foo Pointer) 0))
    (print "cleared")
  )
  (blank)
  (call C:brixCollect)
  (do
    (inline (call C:brixGCDrainCleanups))
  )
  (let payloads-before (call C:brixGCObjectSidePayloadCount 0))
  (blank)
  (print (call exercise-weak-property-subscription))
  (blank)
  (for (in cycle (..< 0 3))
    (call C:brixCollect)
    (do
      (inline (call C:brixGCDrainCleanups))
    )
  )
  (blank)
  (print (- (call C:brixGCObjectSidePayloadCount 0) payloads-before))
)

(type + Foo
  (prop + bar _ (= 1))
  (prop + doubled _ (= (* #bar 2)))
)

(fn-do + exercise-weak-property-subscription String
  (let source (call WeakPropertySource))
  (let live (call WeakPropertyObserver source))
  (call make-dead-property-observer source)
  (blank)
  (call C:brixCollect)
  (do
    (inline (call C:brixGCDrainCleanups))
  )
  (if (not property-change-observer-reclaimed)
    (return "dead-observer-retained")
  )
  (blank)
  (= (. source value) 9)
  (if (!= (. live mirror) 18)
    (return "live-observer-not-updated")
  )
  (blank)
  (return "weak-property-subscription-ok")
)

(fn-do + make-dead-property-observer _
  (param source WeakPropertySource)
  (let observer (call WeakPropertyObserver source))
  (throw (call Panic "Unable to register dead property observer cleanup") (if (not (call C:brixGCRegisterCleanup (as observer Pointer) (as context Pointer) callback))))
  (lazy callback (as property-observer-did-reclaim PropertyCleanupCallback))
  (lazy context (call PropertyCleanupContext))
  (do
    (inline (. observer mirror))
  )
  (call (. observer close))
)

(type + PropertyCleanupContext)

(type + PropertyCleanupCallback (is (c-fn-type Size (param context Pointer))))

(fn-do + property-observer-did-reclaim Size
  (param context Pointer)
  (do
    (inline context)
  )
  (= property-change-observer-reclaimed true)
  (return 1)
)

(type + WeakPropertySource
  (prop + value _ (= 1))
)

(type + WeakPropertyObserver
  (prop + source WeakPropertySource)
  (prop + mirror _ (= (* (. source #value) 2)))
  (fn-do + This _
    (param source _)
  )
  (fn-do + close _)
)
```

```output
has
cleared
weak-property-subscription-ok
0
```

```brix
(main
  (let source (call Source))
  (= (. source next) 10)
  (call (. source refresh))
  (blank)
  (let observer (call Observer))
  (= (. observer source) source)
  (print (str "0:" (. observer text)))
  (blank)
  (= (. source next) 20)
  (call (. source refresh))
  (print (str "1:" (. observer text)))
  (blank)
  (= (. source value) 30)
  (print (str "2:" (. observer text)))
)

(type + Source
  (prop + value _ (= 0))
  (prop + next _ (= 0))
  (fn-do + refresh _
    (= value next)
  )
)

(type + Observer
  (prop + source (? Source))
  (prop + text _ (= (str (?? (?. source #value) 0))))
)
```

```output
0:10
1:20
2:30
```

```brix
(main
  (let leaf (call NestedOptionalLeaf))
  (let middle (call NestedOptionalMiddle))
  (let root (call NestedOptionalRoot))
  (let observer (call NestedOptionalObserver))
  (print (. observer value))
  (blank)
  (= (. observer root) root)
  (print (. observer value))
  (blank)
  (= (. root middle) middle)
  (print (. observer value))
  (blank)
  (= (. middle leaf) leaf)
  (print (. observer value))
  (blank)
  (= (. leaf value) 7)
  (print (. observer value))
  (blank)
  (= (. middle leaf) null)
  (print (. observer value))
)

(type + NestedOptionalLeaf
  (prop + value _ (= 1))
)

(type + NestedOptionalMiddle
  (prop + leaf (? NestedOptionalLeaf))
)

(type + NestedOptionalRoot
  (prop + middle (? NestedOptionalMiddle))
)

(type + NestedOptionalObserver
  (prop + root (? NestedOptionalRoot))
  (prop + value _ (= (?? (?. (?. (?. #root #middle) #leaf) #value) 0)))
)
```

```output
0
0
0
1
7
0
```

## Stream changes

```brix
(main
  (lazy a (## (list 1 2 3)))
  (let started a)
  (call yield-once)
  (print a)
)

(fn-do + yield-once _
  (let done (call Semaphore))
  (do-new
    (call (. done post))
  )
  (call (. done wait))
)
```

```output
3
```

```brix
(main
  (lazy foo 42)
  (let stream (stream #foo))
  (print stream)
  (print (<- stream))
  (= foo 52)
  (print (<- stream))
)
```

```output
[Object core/channel:AsyncBufferedChannel'core/number:I64]
42
52
```

```brix
(main
  (lazy foo 10)
  (let stream (stream #foo))
  (print stream)
  (blank)
  (do-new
    (for (in item stream)
      (print item)
    )
  )
  (blank)
  (call yield-once)
  (= foo 11)
  (= foo 12)
  (call (. stream close))
)

(fn-do + yield-once _
  (let done (call Semaphore))
  (do-new
    (call (. done post))
  )
  (call (. done wait))
)
```

```output
[Object core/channel:AsyncBufferedChannel'core/number:I64]
10
11
12
```

A live owner retains the scalar entry after its action dies; the target clears and explicit
release removes the entry exactly.

A dead owner removes its entry even while the action remains live.

PropertyTracker detach must release the production scalar subscription immediately.

Compiler-generated markup may share one property dependency tracker between actions. Each
action needs its own scalar handle and indexed slot, and detaching one must leave the other live.

Detaching a live peer leaves an unrelated cleared action for dispatch to prune. Detaching the
cleared action's tracker after that prune must make the stale scalar release a clean no-op.

Every subscriber present when a property changes is queued before any of them runs, so a later
subscriber an earlier one detaches still runs once for that change.

A subscriber added during dispatch waits for the next generation, after every subscriber that
was present at dispatch start. The second dispatch preserves insertion order exactly.

A subscriber can remove itself. A dispatch made while a subscriber runs queues the subscribers
it reaches behind those already queued, once each, and a running subscriber runs again last.

A successful load during concurrent mark shades a target that was otherwise reachable only
through the non-marking registry.

Owner and action dying in the same cycle removes the entry without a surviving scalar handle.

```brix
(import /BxGC/BxGC.h)
(import /BxGC/BxGCTest.h)
(import /core/tracker)
(blank)

(main
  (call C:brixGCSetHeapGoal 1073741824)
  (call C:brixGCInit)
  (call C:brixCollect)
  (let baseline (call C:brixGCTestOwnerWeakRegistrationCount))
  (blank)
  (let live-owner (call OwnerWeakProbe 1))
  (let dead-action-handle (call owner-weak-register-dead-action live-owner))
  (call C:brixCollect)
  (if (call C:brixGCOwnerWeakLoad dead-action-handle)
    (throw (call Panic "dead action survived owner-weak collection"))
  )
  (if (!= (call C:brixGCTestOwnerWeakRegistrationCount) (+ baseline 1))
    (throw (call Panic "cleared owner-weak entry count mismatch"))
  )
  (if (!= (. live-owner value) 1)
    (throw (call Panic "live owner did not survive collection"))
  )
  (if (not (call C:brixGCReleaseOwnerWeak dead-action-handle))
    (throw (call Panic "unable to release cleared owner-weak entry"))
  )
  (if (!= (call C:brixGCTestOwnerWeakRegistrationCount) baseline)
    (throw (call Panic "cleared owner-weak entry survived release"))
  )
  (blank)
  (let live-action (call LiveAction owner-weak-empty-trackers))
  (let dead-owner-handle (call owner-weak-register-dead-owner live-action))
  (call C:brixCollect)
  (if (call C:brixGCOwnerWeakLoad dead-owner-handle)
    (throw (call Panic "owner-reclaimed scalar handle remained loadable"))
  )
  (if (!= (call C:brixGCTestOwnerWeakRegistrationCount) baseline)
    (throw (call Panic "owner-reclaimed scalar handle remained registered"))
  )
  (do
    (inline (. live-action is-closed))
  )
  (blank)
  (let detach-owner (call OwnerWeakProbe 2))
  (let detach-subscribers (list PropertySubscriber))
  (let detach-tracker (call PropertyTracker "value" detach-subscribers))
  (= (. detach-tracker owner) (as detach-owner Pointer))
  (let detach-action (call LiveAction owner-weak-empty-trackers))
  (call (. detach-tracker attach) detach-action)
  (if (!= (call C:brixGCTestOwnerWeakRegistrationCount) (+ baseline 1))
    (throw (call Panic "property tracker attach did not register one scalar handle"))
  )
  (call (. detach-tracker detach) detach-action)
  (if (!= (call C:brixGCTestOwnerWeakRegistrationCount) baseline)
    (throw (call Panic "explicit property tracker detach leaked its scalar handle"))
  )
  (call (. detach-tracker detach) detach-action)
  (let detach-active (call owner-weak-active-subscriber-count detach-subscribers))
  (if (or (!= (call C:brixGCTestOwnerWeakRegistrationCount) baseline) detach-active)
    (throw (call Panic "repeated property tracker detach changed the registry"))
  )
  (blank)
  (let shared-owner (call OwnerWeakProbe 9))
  (let shared-subscribers (list PropertySubscriber))
  (let shared-tracker (call PropertyTracker "value" shared-subscribers))
  (= (. shared-tracker owner) (as shared-owner Pointer))
  (let shared-first-action (call LiveAction owner-weak-empty-trackers))
  (let shared-second-action (call LiveAction owner-weak-empty-trackers))
  (call (. shared-tracker attach) shared-first-action)
  (call (. shared-tracker attach) shared-second-action)
  (let shared-active (call owner-weak-active-subscriber-count shared-subscribers))
  (if (or (!= (call C:brixGCTestOwnerWeakRegistrationCount) (+ baseline 2)) (!= shared-active 2))
    (throw (call Panic "shared property tracker did not register both actions"))
  )
  (call (. shared-tracker detach) shared-first-action)
  (= shared-active (call owner-weak-active-subscriber-count shared-subscribers))
  (if (or (!= (call C:brixGCTestOwnerWeakRegistrationCount) (+ baseline 1)) (!= shared-active 1))
    (throw (call Panic "shared property tracker detached the wrong action"))
  )
  (call (. shared-tracker detach) shared-second-action)
  (= shared-active (call owner-weak-active-subscriber-count shared-subscribers))
  (if (or (!= (call C:brixGCTestOwnerWeakRegistrationCount) baseline) shared-active)
    (throw (call Panic "shared property tracker leaked its final action"))
  )
  (blank)
  (let prune-owner (call OwnerWeakProbe 3))
  (let prune-subscribers (list PropertySubscriber))
  (let pruned-tracker (call owner-weak-attach-dead-action prune-owner prune-subscribers))
  (let prune-live-tracker (call PropertyTracker "value" prune-subscribers))
  (= (. prune-live-tracker owner) (as prune-owner Pointer))
  (let prune-live-action (call LiveAction owner-weak-empty-trackers))
  (call (. prune-live-tracker attach) prune-live-action)
  (call C:brixCollect)
  (if (!= (call C:brixGCTestOwnerWeakRegistrationCount) (+ baseline 2))
    (throw (call Panic "dead action was removed before the prune fixture"))
  )
  (call (. prune-live-tracker detach) prune-live-action)
  (let prune-active (call owner-weak-active-subscriber-count prune-subscribers))
  (if (or (!= (call C:brixGCTestOwnerWeakRegistrationCount) (+ baseline 1)) (!= prune-active 1))
    (throw (call Panic "property tracker detach reaped an unrelated cleared action"))
  )
  (call property-dispatch prune-subscribers "value")
  (= prune-active (call owner-weak-active-subscriber-count prune-subscribers))
  (if (or (!= (call C:brixGCTestOwnerWeakRegistrationCount) baseline) prune-active)
    (throw (call Panic "first property dispatch did not prune the cleared scalar handle"))
  )
  (call (. pruned-tracker detach) prune-live-action)
  (= prune-active (call owner-weak-active-subscriber-count prune-subscribers))
  (if (or (!= (call C:brixGCTestOwnerWeakRegistrationCount) baseline) prune-active)
    (throw (call Panic "detach after dispatch prune changed the empty registry"))
  )
  (call property-dispatch prune-subscribers "value")
  (= prune-active (call owner-weak-active-subscriber-count prune-subscribers))
  (if (or (!= (call C:brixGCTestOwnerWeakRegistrationCount) baseline) prune-active)
    (throw (call Panic "repeated property dispatch changed the empty registry"))
  )
  (blank)
  (let reentrant-owner (call OwnerWeakProbe 8))
  (let reentrant-subscribers (list PropertySubscriber))
  (let reentrant-probe (call OwnerWeakDispatchProbe))
  (let reentrant-first-tracker (call PropertyTracker "value" reentrant-subscribers))
  (= (. reentrant-first-tracker owner) (as reentrant-owner Pointer))
  (let reentrant-later-tracker (call PropertyTracker "value" reentrant-subscribers))
  (= (. reentrant-later-tracker owner) (as reentrant-owner Pointer))
  (let reentrant-first-action (call LiveAction (. reentrant-probe detach-later)))
  (let reentrant-later-action (call LiveAction (. reentrant-probe run-later)))
  (= (. reentrant-probe later-tracker) reentrant-later-tracker)
  (= (. reentrant-probe later-action) reentrant-later-action)
  (call (. reentrant-first-tracker attach) reentrant-first-action)
  (call (. reentrant-later-tracker attach) reentrant-later-action)
  (call property-dispatch reentrant-subscribers "value")
  (if (!= (. reentrant-probe later-runs) 1)
    (throw (call Panic "a subscriber queued for a change did not run once"))
  )
  (let reentrant-active (call owner-weak-active-subscriber-count reentrant-subscribers))
  (if (or (!= (call C:brixGCTestOwnerWeakRegistrationCount) (+ baseline 1)) (!= reentrant-active 1))
    (throw (call Panic "reentrant dispatch left the wrong scalar subscriptions"))
  )
  (call (. reentrant-first-tracker detach) reentrant-first-action)
  (= reentrant-active (call owner-weak-active-subscriber-count reentrant-subscribers))
  (if (or (!= (call C:brixGCTestOwnerWeakRegistrationCount) baseline) reentrant-active)
    (throw (call Panic "reentrant dispatch fixture leaked a scalar subscription"))
  )
  (blank)
  (let add-owner (call OwnerWeakProbe 10))
  (let add-subscribers (list PropertySubscriber))
  (let add-probe (call OwnerWeakDispatchProbe))
  (let add-first-tracker (call PropertyTracker "value" add-subscribers))
  (= (. add-first-tracker owner) (as add-owner Pointer))
  (let add-existing-tracker (call PropertyTracker "value" add-subscribers))
  (= (. add-existing-tracker owner) (as add-owner Pointer))
  (let add-late-tracker (call PropertyTracker "value" add-subscribers))
  (= (. add-late-tracker owner) (as add-owner Pointer))
  (let add-first-action (call LiveAction (. add-probe attach-late)))
  (let add-existing-action (call LiveAction (. add-probe run-existing)))
  (let add-late-action (call LiveAction (. add-probe run-added)))
  (= (. add-probe later-tracker) add-late-tracker)
  (= (. add-probe later-action) add-late-action)
  (call (. add-first-tracker attach) add-first-action)
  (call (. add-existing-tracker attach) add-existing-action)
  (call property-dispatch add-subscribers "value")
  (if (!= (. add-probe order) "fe")
    (throw (call Panic "subscriber added during dispatch ran in the starting generation"))
  )
  (call property-dispatch add-subscribers "value")
  (if (!= (. add-probe order) "fefel")
    (throw (call Panic "next-generation subscriber order was not deterministic"))
  )
  (call (. add-first-tracker detach) add-first-action)
  (call (. add-existing-tracker detach) add-existing-action)
  (call (. add-late-tracker detach) add-late-action)
  (blank)
  (let self-owner (call OwnerWeakProbe 11))
  (let self-subscribers (list PropertySubscriber))
  (let self-probe (call OwnerWeakDispatchProbe))
  (let self-tracker (call PropertyTracker "value" self-subscribers))
  (= (. self-tracker owner) (as self-owner Pointer))
  (let self-action (call LiveAction (. self-probe remove-self)))
  (= (. self-probe self-tracker) self-tracker)
  (= (. self-probe self-action) self-action)
  (call (. self-tracker attach) self-action)
  (call property-dispatch self-subscribers "value")
  (call property-dispatch self-subscribers "value")
  (if (!= (. self-probe self-runs) 1)
    (throw (call Panic "self-removing subscriber ran after detaching itself"))
  )
  (blank)
  (let nested-owner (call OwnerWeakProbe 12))
  (let nested-subscribers (list PropertySubscriber))
  (let nested-probe (call OwnerWeakDispatchProbe))
  (let nested-first-tracker (call PropertyTracker "value" nested-subscribers))
  (= (. nested-first-tracker owner) (as nested-owner Pointer))
  (let nested-second-tracker (call PropertyTracker "value" nested-subscribers))
  (= (. nested-second-tracker owner) (as nested-owner Pointer))
  (let nested-first-action (call LiveAction (. nested-probe run-nested-first)))
  (let nested-second-action (call LiveAction (. nested-probe run-nested-second)))
  (= (. nested-probe nested-subscribers) nested-subscribers)
  (call (. nested-first-tracker attach) nested-first-action)
  (call (. nested-second-tracker attach) nested-second-action)
  (call property-dispatch nested-subscribers "value")
  (if (!= (. nested-probe order) "fsf")
    (throw (call Panic "nested property dispatch corrupted subscriber order"))
  )
  (call (. nested-first-tracker detach) nested-first-action)
  (call (. nested-second-tracker detach) nested-second-action)
  (blank)
  (let mark-owner (call OwnerWeakProbe 4))
  (let mark-target (? OwnerWeakProbe) (call OwnerWeakProbe 5))
  (let mark-target-pointer (as (!! mark-target) Pointer))
  (let mark-handle (call C:brixGCRegisterOwnerWeak (as mark-owner Pointer) mark-target-pointer))
  (call C:brixCollect)
  (= mark-target null)
  (call C:brixGCStartConcurrentMark)
  (let loaded-target (call C:brixGCOwnerWeakLoad mark-handle))
  (if (or (!= loaded-target mark-target-pointer) (not (call C:brixGCTestObjectMarked mark-target-pointer)))
    (throw (call Panic "mark-phase owner-weak load did not shade its target"))
  )
  (if (!= (. mark-owner value) 4)
    (throw (call Panic "mark-phase owner did not survive collection"))
  )
  (call C:brixGCFinishConcurrentMark)
  (call C:brixGCFinishAutomaticCycle)
  (if (not (call C:brixGCReleaseOwnerWeak mark-handle))
    (throw (call Panic "unable to release mark-phase owner-weak handle"))
  )
  (if (!= (call C:brixGCTestOwnerWeakRegistrationCount) baseline)
    (throw (call Panic "mark-phase owner-weak handle survived release"))
  )
  (blank)
  (let same-cycle-handle (call owner-weak-register-dead-pair))
  (call C:brixCollect)
  (if (call C:brixGCOwnerWeakLoad same-cycle-handle)
    (throw (call Panic "same-cycle owner/action handle remained loadable"))
  )
  (if (!= (call C:brixGCTestOwnerWeakRegistrationCount) baseline)
    (throw (call Panic "same-cycle owner/action handle remained registered"))
  )
  (blank)
  (print "owner-weak-property-ok")
)

(type + OwnerWeakProbe
  (prop + value U64)
  (fn-do + This _
    (param value _)
  )
)

(fn-do + owner-weak-empty-trackers (list Tracker)
  (return (call (of List Tracker)))
)

(fn-do + owner-weak-register-dead-action U64
  (param owner OwnerWeakProbe)
  (let action (call LiveAction owner-weak-empty-trackers))
  (return (call C:brixGCRegisterOwnerWeak (as owner Pointer) (as action Pointer)))
)

(fn-do + owner-weak-register-dead-owner U64
  (param action LiveAction)
  (let owner (call OwnerWeakProbe 6))
  (return (call C:brixGCRegisterOwnerWeak (as owner Pointer) (as action Pointer)))
)

(fn-do + owner-weak-attach-dead-action PropertyTracker
  (param owner OwnerWeakProbe)
  (param subscribers (list PropertySubscriber))
  (let tracker (call PropertyTracker "value" subscribers))
  (= (. tracker owner) (as owner Pointer))
  (let action (call LiveAction owner-weak-empty-trackers))
  (call (. tracker attach) action)
  (return tracker)
)

(type + OwnerWeakDispatchProbe
  (prop + later-tracker (? PropertyTracker))
  (prop + later-action (? LiveAction))
  (prop + later-runs Size (= 0))
  (prop + self-tracker (? PropertyTracker))
  (prop + self-action (? LiveAction))
  (prop + self-runs Size (= 0))
  (prop + nested-subscribers (? (list PropertySubscriber)))
  (prop + is-nested _ (= false))
  (prop + order _ (= ""))
  (fn-do + detach-later (list Tracker)
    (if (and later-tracker later-action)
      (call (. later-tracker detach) later-action)
    )
    (return (call (of List Tracker)))
  )
  (fn-do + run-later (list Tracker)
    (+= later-runs 1)
    (return (call (of List Tracker)))
  )
  (fn-do + attach-late _
    (+= order "f")
    (if (and later-tracker later-action (== order "f"))
      (call (. later-tracker attach) later-action)
    )
  )
  (fn-do + run-existing _
    (+= order "e")
  )
  (fn-do + run-added _
    (+= order "l")
  )
  (fn-do + remove-self _
    (+= self-runs 1)
    (if (and self-tracker self-action)
      (call (. self-tracker detach) self-action)
    )
  )
  (fn-do + run-nested-first _
    (+= order "f")
    (if (and (not is-nested) nested-subscribers)
      (= is-nested true)
      (call property-dispatch nested-subscribers "value")
    )
  )
  (fn-do + run-nested-second _
    (+= order "s")
  )
)

(fn-do + owner-weak-register-dead-pair U64
  (let owner (call OwnerWeakProbe 7))
  (let action (call LiveAction owner-weak-empty-trackers))
  (return (call C:brixGCRegisterOwnerWeak (as owner Pointer) (as action Pointer)))
)

(fn-do + owner-weak-active-subscriber-count Size
  (param subscribers (list PropertySubscriber))
  (let count Size 0)
  (for (in subscriber subscribers)
    (if (and (. subscriber slot) (. subscriber handle))
      (+= count 1)
    )
  )
  (return count)
)
```

```output
owner-weak-property-ok
```

```brix crash
(import /BxGC/BxGC.h)
(import /core/tracker)
(blank)

(main
  (call C:brixGCSetHeapGoal 1073741824)
  (call C:brixGCInit)
  (let subscribers (list PropertySubscriber))
  (let action (call LiveAction owner-weak-attach-guard-empty-trackers))
  (let tracker (call PropertyTracker "value" subscribers))
  (= (. tracker owner) (as action Pointer))
  (call (. tracker attach) action)
  (call (. tracker attach) action)
)

(fn-do + owner-weak-attach-guard-empty-trackers (list Tracker)
  (return (call (of List Tracker)))
)
```

### Derived properties and property handlers follow writes to the object's own properties, and stop when the object closes

```brix
(main
  (let f (call Foo))
  (print (. f double))
  (= (. f bar) 50)
  (print (. f double))
  (print (. f label))
  (= (. f bar) 25)
  (print (. f double))
  (print (. f label))
  (print (. f changes))
  (call (. f close))
  (= (. f bar) 7)
  (print (. f double))
  (print (. f changes))
)

(type + Foo
  (prop + bar _ (= 42))
  (prop + double _ (= (* #bar 2)))
  (prop + label _ (= (str "bar is " #bar " and double is " #double)))
  (prop + changes _ (= 0))
  (blank)
  (depends bar-changed bar)
  (fn-do + bar-changed _
    (+= changes 1)
  )
  (fn-do + close _)
)
```

```output
84
100
bar is 50 and double is 100
50
bar is 25 and double is 50
2
50
2
```

### A derived property declared after the property that reads it still follows its own dependencies, whether the earlier property reads it tracked or untracked

```brix
(main
  (let tracked (call Tracked))
  (print (str (. tracked double) " " (. tracked label)))
  (= (. tracked bar) 5)
  (print (str (. tracked double) " " (. tracked label)))
  (blank)
  (let plain (call Plain))
  (print (str (. plain double) " " (. plain label)))
  (= (. plain bar) 5)
  (print (str (. plain double) " " (. plain label)))
)

(type + Tracked
  (prop + bar _ (= 1))
  (prop + label _ (= (str "double is " #double)))
  (prop + double _ (= (* #bar 2)))
)

(type + Plain
  (prop + bar _ (= 1))
  (prop + label _ (= (str "double is " double)))
  (prop + double _ (= (* #bar 2)))
)
```

```output
2 double is 2
10 double is 10
2 double is 2
10 double is 2
```

### A subclass default for an inherited property can read the subclass's own derived properties, and both keep following their dependencies

```brix
(main
  (let item (call Scaled))
  (print (str (. item double) " " (. item total)))
  (= (. item bar) 5)
  (print (str (. item double) " " (. item total)))
)

(type + Totalled
  (prop + total _ (= 0))
)

(type + Scaled (is Totalled)
  (prop + bar _ (= 1))
  (prop + double _ (= (* #bar 2)))
  (prop + total _ (= (+ #double 1)))
)
```

```output
2 3
10 11
```

### A handler of a property reads the derived properties it feeds already updated, on every write, and a derived property no rule reads is evaluated again only when read

```brix
(main
  (let foo (call Foo))
  (let watcher (call Watcher foo))
  (print (str (. watcher seen) " " (call (. "|" join) (. foo log))))
  (blank)
  (= (. foo bar) 5)
  (print (str (. watcher seen) " " (call (. "|" join) (. foo log))))
  (blank)
  (= (. foo bar) 7)
  (print (str (. watcher seen) " " (call (. "|" join) (. foo log))))
)

(type + Foo
  (prop + log (list String) (= (call (of List String))))
  (prop + bar _ (= 42))
  (prop + double _ (= (* #bar 2)))
  (blank)
  (depends bar-changed bar)
  (fn-do + bar-changed _
    (+= log (str "bar:" bar "/" double))
  )
)

(type + Watcher
  (prop + source Foo)
  (prop + seen _ (= (call noted source (+ (. source #bar) (. source #double)))))
  (fn-do + This _
    (param source _)
  )
)

(fn-do + noted Int
  (param foo Foo)
  (param sum Int)
  (+= (. foo log) (str "seen:" sum))
  (return sum)
)
```

```output
126 seen:126
15 seen:126|bar:5/10|seen:15
21 seen:126|bar:5/10|seen:15|bar:7/14|seen:21
```

### A lazy property another initializer reads during construction is constructed once: the object keeps the instance its other properties saw, tracked reads through it follow that instance, and a lazy property nothing reads stays unevaluated

```brix
(main
  (let panel (call Panel))
  (print (str made " " (. panel title)))
  (= (. panel session current) "one")
  (print (str made " " (. panel title) " " (== (as (. panel first) Pointer) (as (. panel session) Pointer))))
  (= (. panel session current) "two")
  (print (str made " " (. panel title)))
  (blank)
  (let idle (call Idle))
  (print (str made))
  (= (. idle session current) "used")
  (print (str made " " (. idle session current)))
)

(blank)
(prop + made _ (= 0))

(type + Session
  (prop + current _ (= "none"))
  (fn-do + This _
    (+= made 1)
  )
)

(type + Panel
  (prop + session _ (:= (call Session)))
  (prop + first Session (= session))
  (prop + title _ (= (. session #current)))
)

(type + Idle
  (prop + session _ (:= (call Session)))
  (prop + title _ (= "idle"))
)
```

```output
1 none
1 one true
1 two
1
2 used
```

### A lazy property read under a condition, or by another lazy property, is still one instance

```brix
(main
  (let shown (call Panel true))
  (= (. shown session current) "one")
  (print (str made " " (. shown title)))
  (blank)
  (let quiet (call Panel false))
  (print (str (. quiet title)))
  (= (. quiet session current) "two")
  (let count made)
  (= (. quiet uses) true)
  (print (str (. quiet title) " " (== made count)))
  (blank)
  (let chain (call Chain))
  (= (. chain inner current) "three")
  (print (str (. chain seen) " " (== (as (. chain outer inner) Pointer) (as (. chain inner) Pointer))))
)

(blank)
(prop + made _ (= 0))

(type + Session
  (prop + current _ (= "none"))
  (fn-do + This _
    (+= made 1)
  )
)

(type + Wrapper
  (prop + inner Session)
  (fn-do + This _
    (param inner _)
  )
)

(type + Panel
  (prop + uses Bool)
  (prop + session _ (:= (call Session)))
  (prop + title _ (= (if-expr #uses (. session #current) "off")))
  (fn-do + This _
    (param uses _)
  )
)

(type + Chain
  (prop + inner _ (:= (call Session)))
  (prop + outer _ (:= (call Wrapper inner)))
  (prop + seen _ (= (. outer inner #current)))
)
```

```output
1 one
off
two true
three true
```

```brix
(main
  (lazy name "bob")
  (let foo (call Foo))
  (blank)
  (= (. foo title) #name)
  (print (. foo title))
  (= name "tom")
  (print (. foo title))
  (call (. foo close))
  (= name "fred")
  (print (. foo title))
)

(type + Foo
  (prop + title _ (= ""))
  (fn-do + close _)
)
```

```output
bob
tom
tom
```

### a when rule stores to a property of its object

```brix
(main
  (let counter (call Counter))
  (call (. counter bump) 3)
  (call (. counter bump) 4)
  (print (str "count " (. counter count) " seen " (. counter seen)))
)

(type + Counter
  (prop + count _ (= 0))
  (prop + seen _ (= 0))
  (when-rule (> #count 5)
    (+= seen 1)
  )
  (fn-do + bump _
    (param value Int)
    (+= count value)
  )
)
```

```output
count 7 seen 1
```

### A tracked optional local set to nothing and back gives again what reads it.

```brix
(main
  (lazy value (? Int) 3)
  (let holder (call Holder))
  (= (. holder text) (str "v=" #value))
  (print (. holder text))
  (= value null)
  (print (. holder text))
  (= value 7)
  (print (. holder text))
)

(type + Holder
  (prop + text _ (= ""))
)
```

```output
v=3
v=unknown
v=7
```

### A return in the body of a `whenever` rule of a class leaves only the body, and the rule runs again on the next change.

```brix
(main
  (let foo (call Foo))
  (= (. foo bar) "x")
  (= (. foo bar) null)
  (= (. foo bar) "y")
  (print (. foo count))
)

(type + Foo
  (prop + bar (? String) (= null))
  (prop + count _ (= 0))
  (whenever-rule #bar
    (return-if (if (== bar null)))
    (+= count 1)
    (print (str "changed " (?? bar "-")))
  )
)
```

```output
changed x
changed y
2
```

### A return in the body of a `when` rule of a class leaves only the body.

```brix
(main
  (let foo (call Foo))
  (= (. foo bar) 3)
  (= (. foo bar) 12)
  (= (. foo bar) 1)
  (= (. foo bar) 20)
  (print (. foo count))
)

(type + Foo
  (prop + bar _ (= 0))
  (prop + count _ (= 0))
  (when-rule (>= #bar 10)
    (return-if (if (> bar 15)))
    (+= count 1)
    (print (str "greater " bar))
  )
)
```

```output
greater 12
1
```

### A `whenever` rule of a base class on a property a class below gives a live value runs each time that value changes.

```brix
(main
  (let sub (call Sub))
  (print (. sub runs))
  (= (. sub message) "a")
  (print (. sub runs))
  (= (. sub message) "b")
  (print (. sub runs))
  (= (. sub message) "c")
  (print (. sub runs))
)

(type + Base
  (prop + text String (= ""))
  (prop + runs _ (= 0))
  (whenever-rule #text
    (+= runs 1)
  )
)

(type + Sub (is Base)
  (prop + message _ (= "start"))
  (prop + text _ (= (str #message "!")))
)
```

```output
0
1
2
3
```

### A `whenever` rule of a class on a property it gives a new value runs where the property is stored through the class above that declares it.

```brix
(main
  (let sub (call Sub))
  (let base Base sub)
  (print (. sub runs))
  (= (. base text) "a")
  (print (. sub runs))
  (call (. base set) "b")
  (print (. sub runs))
)

(type + Base
  (prop + text String (= ""))
  (fn-do + set _
    (param value String)
    (= text value)
  )
)

(type + Sub (is Base)
  (prop + text _ (= "init"))
  (prop + runs _ (= 0))
  (whenever-rule #text
    (+= runs 1)
  )
)
```

```output
0
1
2
```

### A store to a property a later read tracks, inside a test of another property, where nothing after it reads what the test found.

```brix
(main
  (let panel (call Panel))
  (call (. panel reveal) 1)
  (call (. panel reveal) 2)
  (print (call (. panel describe)))
)

(type + Step
  (prop + id U64)
  (prop + is-revealed _ (= false))
  (prop + is-shown _ (= #is-revealed))
  (fn-do + This _
    (param id _)
  )
)

(type + Panel
  (prop _ revealed (? Step))
  (prop _ steps _ (= (list (call Step 1) (call Step 2))))
  (fn-do + reveal _
    (param id U64)
    (if revealed
      (= (. revealed is-revealed) false)
    )
    (= revealed null)
    (for (in step steps)
      (if (== (. step id) id)
        (= (. step is-revealed) true)
        (= revealed step)
      )
    )
  )
  (fn-expr + describe String
    (str (. ([] steps 0) is-revealed) " " (. ([] steps 1) is-revealed))
  )
)
```

```output
false true
```

### A `close` that returns a value closes the live actions the object owns as it returns, at each return.

```brix
(main
  (let source (call Source))
  (let view (call View source))
  (= (. source count) 2)
  (print (. view doubled))
  (print (call (. view close)))
  (= (. source count) 5)
  (print (. view doubled))
  (print (call (. view close)))
)

(type + Source
  (prop + count _ (= 1))
)

(type + View
  (prop + source Source)
  (prop + doubled _ (= (* (. #source #count) 2)))
  (prop + closings _ (= 0))
  (fn-do + This _
    (param source _)
  )
  (fn-do + close Bool
    (+= closings 1)
    (return-if false (if (> closings 1)))
    (return true)
  )
)
```

```output
4
true
4
false
```

### A `whenever` rule whose body loops runs again on each change of what it reads.

```brix
(main
  (let bar (call Bar))
  (= (. bar preview) (call Preview (list "a" "b") (list "b" "c")))
  (print (. bar seen))
  (= (. bar preview) (call Preview (list "x") (list "y")))
  (print (. bar seen))
)

(type + Preview
  (prop + active (list String))
  (prop + empty (list String))
  (fn-do + This _
    (param active _)
    (param empty _)
  )
)

(type + Bar
  (prop + preview (? Preview) (= null))
  (prop + seen _ (= ""))
  (whenever-rule #preview
    (let current preview)
    (if current
      (let open (call (of List String) (. current active)))
      (for (in key (. current empty))
        (if (not (has open key))
          (+= open key)
        )
      )
      (= seen (call (. "," join) open))
    )
  )
)
```

```output
a,b,c
x,y
```

### A `when` rule whose body loops runs it first as the object is made, and again on each change.

```brix
(main
  (let bar (call Bar))
  (print (. bar seen))
  (= (. bar on) false)
  (= (. bar on) true)
  (print (. bar seen))
)

(type + Bar
  (prop + on _ (= true))
  (prop + seen _ (= 0))
  (when-rule #on
    (for (in step (..< 0 3))
      (+= seen step)
    )
  )
)
```

```output
3
6
```

### A tracked value that reads a later property on some paths alone gives that property its value there, and both values follow what they read.

```brix
(main
  (let thing (call Thing))
  (print (. thing shows))
  (= (. thing flag) false)
  (print (. thing shows))
  (= (. thing ready) false)
  (= (. thing flag) true)
  (print (. thing shows))
  (= (. thing ready) true)
  (print (. thing shows))
)

(type + Thing
  (prop + ready _ (= true))
  (prop + shows _ (= (and #ready #hovered)))
  (prop + flag _ (= true))
  (blank)
  (prop _ hovered _ (= #flag))
)
```

```output
true
false
false
true
```

### An error a tracked value raises as it is evaluated again goes to unhandled-error, and the property keeps the value it had.

```brix
(main
  (= unhandled-error
    (=> (error) :do
      (block
        (print (str "handled " (. error message)))
      )
    )
  )
  (blank)
  (let thing (call Thing))
  (print (. thing ok))
  (= (. thing n) 3)
  (print (. thing ok))
  (= (. thing n) 9)
  (print (. thing ok))
  (= (. thing n) 4)
  (print (. thing ok))
)

(type + Thing
  (prop + n I64 (= 0))
  (prop + ok _ (= (and (> #n 0) (call check #n))))
)

(fn-do _ check Bool
  (param n I64)
  (if (> n 5)
    (throw (call Error (str "too big " n)))
  )
  (return (!= n 3))
)
```

```output
false
false
handled too big 9
false
true
```

### A narrowed read stored after a store to a watched property stores what the narrowed read holds.

```brix
(main
  (let doc (call Doc))
  (let watcher (call Watcher doc))
  (let closed (list Closed))
  (+= closed (call Closed 1 (call Jump 4 "x")))
  (+= closed (call Closed 2 null))
  (call reopen closed doc)
  (print (?? (?. (. doc jump) text) "none"))
  (call reopen closed doc)
  (print (?? (?. (. doc jump) text) "none"))
  (print (. watcher seen))
)

(fn-do _ reopen _
  (param closed (list Closed))
  (param doc Doc)
  (while (> (. closed length) 0)
    (let entry (call (. closed pop)))
    (if (> (. entry index) 0)
      (if (. entry jump)
        (= (. doc wants) true)
        (= (. doc jump) (. entry jump))
      )
      (return)
    )
  )
)

(type _ Closed (is (tuple (: index Int) (: jump (? Jump)))))

(type + Jump (is Tuple)
  (prop + line Size)
  (prop + text String)
)

(type + Doc
  (prop + wants _ (= false))
  (prop + jump (? Jump))
)

(type + Watcher
  (prop + doc Doc)
  (prop + seen _ (= 0))
  (fn-do + This _
    (param doc _)
  )
  (whenever-rule (. doc #jump)
    (+= seen 1)
  )
  (whenever-rule (. doc #wants)
    (+= seen 10)
  )
)
```

```output
none
x
11
```

### A narrowed read stored after a store to a property its function tracked before stores what the narrowed read holds.

```brix
(main
  (let doc (call Doc))
  (call reopen (call Closed 1 (call Jump 4 "x")) doc)
  (print (?? (?. (. doc jump) text) "none"))
  (print (. doc wants))
)

(fn-do _ reopen _
  (param entry Closed)
  (param doc Doc)
  (let wanted (. doc wants))
  (if (. entry jump)
    (= (. doc wants) (not wanted))
    (= (. doc jump) (. entry jump))
  )
)

(type _ Closed (is (tuple (: index Int) (: jump (? Jump)))))

(type + Jump (is Tuple)
  (prop + line Size)
  (prop + text String)
)

(type + Doc
  (prop + wants _ (= false))
  (prop + is-wanting _ (= #wants))
  (prop + jump (? Jump))
)
```

```output
x
true
```

### A property a test found to hold a value, forced after a store to a property a value tracks, is what it holds.

```brix
(main
  (let holder (call Holder))
  (= (. holder options) (call Options))
  (call (. holder run))
)

(type + Options
  (prop + name _ (= "x"))
)

(type + Holder
  (prop + options (? Options))
  (prop + flag _ (= false))
  (prop + shown _ (= (str #flag)))
  (fn-do + run _
    (if options
      (= flag true)
      (print (. (!! options) name))
    )
  )
)
```

```output
x
```

### A store to a tracked property runs the rules that track it before the next statement, so a property a test found to hold a value is tested again where it is forced after the store.

```brix crash
(main
  (let holder (call Holder))
  (= (. holder options) (call Options))
  (call (. holder run))
)

(type + Options
  (prop + name _ (= "x"))
)

(type + Holder
  (prop + options (? Options))
  (prop + flag _ (= false))
  (fn-do + run _
    (if options
      (= flag true)
      (print (. (!! options) name))
    )
  )
  (whenever-rule #flag
    (= options null)
  )
)
```

```stderr
Unhandled panic: Optional'[Object __source__:Options] is null
<no file:18>

Task 1

Stack trace:
  0  __source__:Holder:run no file:18:17
  1  __source__:main no file:5:3
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

### A store to a tracked property ends what is known of every property, so a property a test found to hold a value is optional again after it.

```brix crash
(main
  (let holder (call Holder))
  (= (. holder options) (call Options))
  (call (. holder run))
)

(type + Options
  (prop + name _ (= "x"))
)

(type + Holder
  (prop + options (? Options))
  (prop + flag _ (= false))
  (prop + shown _ (= (str #flag)))
  (fn-do + run _
    (if options
      (= flag true)
      (print (. options name))
    )
  )
)
```

```output
Exception: Property "name" is read from Options?, which may be null; unwrap it first with "(!! …)" or "(?. …)"
<no file:19>
          (print (. options name))
                 ^^^^^^^^^^^^^^^^
```

### A `when` rule of a class runs its body as the object is made and on every change while its test holds.

```brix
(main
  (let meter (call Meter))
  (= (. meter level) 5)
  (= (. meter level) 0)
  (= (. meter level) 7)
)

(type + Meter
  (prop + level _ (= 2))
  (when-rule (> #level 0)
    (print (str "level " level))
  )
)
```

```output
level 2
level 5
level 7
```

### A tracked lazy local is evaluated, and starts watching what it read, at whichever of its reads runs first, even where a read written before it does not run.

```brix
(main
  (lazy foo (* #bar 10))
  (lazy bar 1)
  (let flag true)
  (if (not flag)
    (print (str "branch one " foo))
  )
  (if flag
    (print (str "branch two " foo))
  )
  (print (str "plain " foo))
  (= bar 2)
  (print (str "after change " foo))
)
```

```output
branch two 10
plain 10
after change 20
```

### A `when` rule that leaves early on its first run, as the object is made, leaves only its body.

```brix
(main
  (let foo (call Foo))
  (print (str "made " (. foo name)))
  (= (. foo bar) 5)
  (= (. foo bar) 7)
)

(type + Foo
  (prop + bar _ (= 0))
  (prop + name _ (= "none"))
  (fn-do + This _
    (= name "foo")
  )
  (when-rule (< #bar 6)
    (return-if (if (== bar 0)))
    (print (str "lesser " bar))
  )
  (when-rule (> #bar 6)
    (print (str "greater " bar))
  )
)
```

```output
made foo
lesser 5
greater 7
```

### A rule whose nested function reads the object and a local the rule declares, where the constructor runs it first and where what it tests changes.

```brix
(main
  (let holder (call Holder))
  (print (call (. ", " join) (. holder first)))
  (print (call (. ", " join) (. holder later)))
  (= (. holder context) "b")
  (print (call (. ", " join) (. holder first)))
  (print (call (. ", " join) (. holder later)))
  (= (. holder hidden) "z")
  (= (. holder context) "c")
  (print (call (. ", " join) (. holder first)))
  (print (call (. ", " join) (. holder later)))
)

(type + Holder
  (prop + context _ (= "a"))
  (prop + hidden _ (= "x"))
  (prop + first _ (= (call (of List String))))
  (prop + later _ (= (call (of List String) "none")))
  (prop _ names _ (= (call (of List String) "x" "y" "z")))
  (when-rule (!= #context "")
    (let suffix context)
    (= first (call (of List String) (in (str each suffix) names (when (!= each hidden)))))
  )
  (whenever-rule #context
    (let suffix context)
    (= later (call (of List String) (in (str each suffix) names (when (!= each hidden)))))
  )
)
```

```output
ya, za
none
yb, zb
yb, zb
xc, yc
xc, yc
```

### A derived property that reads another one never shows a rule a mix of old and new values.

```brix
(main
  (let f (call Foo))
  (= (. f a) 2)
  (print (call (. "|" join) (. f seen)))
)

(type + Foo
  (prop + a _ (= 1))
  (prop + total _ (= (+ #a #b)))
  (prop + b _ (= (* #a 10)))
  (prop + seen (list String) (= (call (of List String))))
  (whenever-rule #total
    (+= seen (str "total:" total))
  )
)
```

```output
total:22
```

### A derived property nothing subscribes to is evaluated again only when read after changes, once, from fresh values.

```brix
(main
  (let f (call Foo))
  (print (. f d))
  (= (. f a) 2)
  (= (. f a) 3)
  (print (. f d))
)

(type + Foo
  (prop + a _ (= 1))
  (prop + b _ (= (* #a 10)))
  (prop + c _ (= (* #a 100)))
  (prop + d _ (= (call summed #b #c)))
)

(fn-do + summed Int
  (param left Int)
  (param right Int)
  (print (str "run " left " " right))
  (return (+ left right))
)
```

```output
run 10 100
110
run 30 300
330
```

### A class may give a property of its base a value that reads tracked state, read through the base.

```brix
(main
  (let s (call Sub))
  (let b Base s)
  (print (. b x))
  (= (. s y) 5)
  (print (. b x))
)

(type + Base
  (prop + x _ (= 1))
)

(type + Sub (is Base)
  (prop + y _ (= 2))
  (prop + x _ (= (* #y 10)))
)
```

```output
20
50
```

### A value written to a derived property stays until what it reads changes.

```brix
(main
  (let f (call Foo))
  (print (. f total))
  (= (. f total) 99)
  (print (. f total))
  (= (. f a) 3)
  (print (. f total))
)

(type + Foo
  (prop + a _ (= 1))
  (prop + total _ (= (* #a 10)))
)
```

```output
10
99
30
```

### A stream of a derived value gives nothing when the value is evaluated again to the same value.

```brix
(main
  (let f (call Foo))
  (let values (stream (. f #positive)))
  (print (<- values))
  (= (. f a) 2)
  (= (. f a) -1)
  (print (<- values))
  (call (. values close))
)

(type + Foo
  (prop + a _ (= 1))
  (prop + positive _ (= (> #a 0)))
)
```

```output
true
false
```

### A derived value that makes another one it reads stale before it subscribes still sees later changes.

```brix
(main
  (let f (call Foo))
  (print (. f d))
  (= (. f on) true)
  (print (. f d))
  (= (. f src a) 7)
  (print (. f d))
)

(type + Source
  (prop + a _ (= 1))
)

(type + Foo
  (prop + src _ (= (call Source)))
  (prop + on _ (= false))
  (prop + total _ (= (* (. src #a) 10)))
  (prop + d _ (= (if-expr #on (call bumped src #total) 0)))
)

(fn-do + bumped Int
  (param source Source)
  (param value Int)
  (if (== (. source a) 1)
    (= (. source a) 5)
  )
  (return value)
)
```

```output
0
10
70
```

### A rule that reads a list and a derived value of it sees them agree when the list changes.

```brix
(main
  (let f (call Foo))
  (= (. f on) true)
  (print (. f m))
  (+= (. f items) 7)
  (print (call (. "|" join) (. f seen)))
)

(type + Foo
  (prop + items _ (= (call (of List Int))))
  (prop + on _ (= false))
  (prop + m _ (= (if-expr #on (. #items length) 0)))
  (prop + seen (list String) (= (call (of List String))))
  (whenever-rule (str (. #items length) ":" #m)
    (+= seen (str (. items length) ":" m))
  )
)
```

```output
0
1:1
```

### A derived property that settles a stale derived value it reads, and comes out the same, still runs its rules when what it reads changes again.

```brix
(main
  (let source (call Source))
  (let middle (call Middle source))
  (let f (call Foo middle "a"))
  (call (. source pick) "b")
  (call (. source pick) "c")
  (call (. source pick) "a")
)

(type + Source
  (prop + revision U64)
  (prop _ picked _ (= "a"))
  (fn-do + pick _
    (param id String)
    (= picked id)
    (+= revision 1)
  )
  (fn-do + current String
    (return picked)
  )
)

(type + Middle
  (prop + source Source)
  (prop + revision _ (= (. #source #revision)))
  (fn-do + This _
    (param source _)
  )
)

(type + Foo
  (prop + middle Middle)
  (prop + id String)
  (prop + is-picked _ (= (== (call revised (call (. middle source current)) (. middle #revision)) #id)))
  (fn-do + This _
    (param middle _)
    (param id _)
  )
  (whenever-rule #is-picked
    (print (str id " picked " is-picked))
  )
)

(fn-expr _ revised String
  (param value String)
  (param revision U64)
  value
  (where
    (lazy ignored revision)
  )
)
```

```output
a picked false
a picked true
```

### A dependency on a getter is rejected, as a getter is never stored to run it.

```brix crash
(main
  (let foo (call Foo))
)

(type + Foo
  (prop + a _ (= 1))
  (prop + twice _ (=> (* a 2)))
  (blank)
  (depends shown twice)
  (fn-do + shown _
    (print "shown")
  )
)
```

```output
Exception: "twice" is a getter, which is never stored, so a dependency on it never runs; depend on the properties it reads
<no file:10>
      (depends shown twice)
                     ^^^^^
```

### A method called with "#" before its name is rejected, as only a stored property is tracked.

```brix crash
(main
  (let c (call Counter))
  (let h (call Holder))
  (= (. h text) (str (call (. c #size))))
)

(type + Holder
  (prop + text _ (= ""))
)

(type + Counter
  (prop + count _ (= 1))
  (fn-expr + size Int
    count
  )
)
```

```output
Exception: "size" is a method, so "#" before it tracks nothing; track the properties it reads instead
<no file:5>
      (= (. h text) (str (call (. c #size))))
                               ^^^^^^^^^^^
```

### A rule that returns a value with "=" is rejected, as a rule gives none.

```brix crash
(main
  (let f (call Foo))
  (= (. f a) 1)
)

(type + Foo
  (prop + a _ (= 0))
  (whenever-rule #a
    (return 5)
  )
)
```

```output
Exception: A rule gives no value, so its body cannot return one with "="; end it early with "= alone"
<no file:10>
        (return 5)
        ^^^^^^^^^^
```

### A tracked read of a struct's field is rejected, as a struct's values are copied.

```brix crash
(main
  (let h (call Holder))
  (print (. h sum))
)

(type + Point (is Struct)
  (prop + x Int)
  (prop + y Int)
)

(type + Holder
  (prop + p _ (= (call Point 1 2)))
  (prop + sum _ (= (+ (. p #x) 1)))
)
```

```output
Exception: "#x" reads a field of Point, whose values are copied, so it cannot be tracked; track a property of a class
<no file:14>
      (prop + sum _ (= (+ (. p #x) 1)))
                          ^^^^^^^^
```

### A tracked read of "this" is rejected, as only a property is tracked.

```brix crash
(main
  (let c (call Counter))
  (print (. c mirror))
)

(type + Counter
  (prop + count _ (= 1))
  (prop + mirror _ (= (str #this)))
)
```

```output
Exception: "#this" tracks nothing, as only a property is tracked; track the properties the value reads, as in "#count"
<no file:9>
      (prop + mirror _ (= (str #this)))
                               ^^^^^
```

### A function written where a local is declared stores to it, and what reads the local tracked follows.

```brix
(main
  (let count 0)
  (let holder (call Holder))
  (= (. holder text) (str "count " #count))
  (let bump (=> () :do (+= count 1)))
  (call bump)
  (print (. holder text))
  (call bump)
  (print (. holder text))
)

(type + Holder
  (prop + text _ (= ""))
)
```

```output
count 1
count 2
```

### A function written in another, written where a local is declared, stores to the local.

```brix
(main
  (let word "a")
  (let holder (call Holder))
  (= (. holder text) (str "word " #word))
  (let outer
    (=> () :do
      (block
        (let inner (=> () :do (= word "b")))
        (call inner)
      )
    )
  )
  (call outer)
  (print (. holder text))
)

(type + Holder
  (prop + text _ (= ""))
)
```

```output
word b
```

### A function written before the read that tracks a local stores to it before the read and after.

```brix
(main
  (let count 0)
  (let bump (=> () :do (+= count 1)))
  (call bump)
  (let holder (call Holder))
  (= (. holder text) (str "count " #count))
  (print (. holder text))
  (call bump)
  (print (. holder text))
)

(type + Holder
  (prop + text _ (= ""))
)
```

```output
count 1
count 2
```

## Lazies attached to a value

### A lazy attached to the value of a live assignment is part of the value, in "main" or in a function: the assignment stores again as what the lazy reads changes, whether the value reads the lazy plainly or with "#", in a scope or in the block of a call, and a "#" read written in the value itself is still followed. A live lazy's value may have one attached too.

```brix
(main
  (let cart (call Cart))
  (let plain (call Label))
  (let tracked (call Label))
  (let called (call Label))
  (let doubling (call Label))
  (= (. plain text)
    (scope (str q " x " (. cart #price))
      (lazy q (. cart #quantity))
    )
  )
  (= (. tracked text)
    (scope (str #q " items")
      (lazy q (. cart #quantity))
    )
  )
  (call follow cart called)
  (lazy doubled
    (scope (* q 2)
      (lazy q (. cart #quantity))
    )
  )
  (= (. doubling text) (str "doubled " #doubled))
  (call report plain tracked called doubling)
  (= (. cart quantity) 3)
  (call report plain tracked called doubling)
  (= (. cart price) 7)
  (call report plain tracked called doubling)
  (= (. cart quantity) 4)
  (call report plain tracked called doubling)
)

(fn-do + follow _
  (param cart Cart)
  (param label Label)
  (= (. label text)
    (call (. (call padded amount) trim)
      (block (lazy amount (* (. cart #quantity) (. cart #price))))
    )
  )
)

(fn-expr + padded String
  (param n Int)
  (str "  " n "  ")
)

(fn-do + report _
  (param plain Label)
  (param tracked Label)
  (param called Label)
  (param doubling Label)
  (print (str (. plain text) " | " (. tracked text) " | " (. called text) " | " (. doubling text)))
)

(type + Cart
  (prop + quantity _ (= 1))
  (prop + price _ (= 5))
)

(type + Label
  (prop + text _ (= ""))
  (fn-do + close _)
)
```

```output
1 x 5 | 1 items | 5 | doubled 2
3 x 5 | 3 items | 15 | doubled 6
3 x 7 | 3 items | 21 | doubled 6
4 x 7 | 4 items | 28 | doubled 8
```

### A lazy attached to the value of a content row is part of the value, for a row that stores to a property and for one that declares a property.

```brix
(import /core/markup)

(main
  (let cart (call Cart))
  (let card (call Card cart))
  (print (str (. card plain) " | " (. card noted)))
  (= (. cart quantity) 3)
  (print (str (. card plain) " | " (. card noted)))
  (= (. cart price) 7)
  (print (str (. card plain) " | " (. card noted)))
  (= (. cart quantity) 4)
  (print (str (. card plain) " | " (. card noted)))
)

(type + Cart
  (prop + quantity _ (= 1))
  (prop + price _ (= 5))
)

(type + Card (is MarkupNode)
  (prop + cart Cart)
  (prop + plain _ (= ""))
  (fn-do + This _
    (param cart _)
  )
  (content
    (= plain
      (scope (str q " x " (. cart #price))
        (lazy q (. cart #quantity))
      )
    )
    (prop + noted _
      (= (scope (str #q " at " (. cart #price))
          (lazy q (. cart #quantity))
        )
      )
    )
  )
)
```

```output
1 x 5 | 1 at 5
3 x 5 | 3 at 5
3 x 7 | 3 at 7
4 x 7 | 4 at 7
```

### A lazy attached to a live value is evaluated again in each run where the value reads it plainly, reads nothing tracked itself and works on it only with built-in operators: in a function, in a method, and in a content row that stores to a property or declares one.

```brix
(import /core/markup)

(main
  (let cart (call Cart))
  (let label (call Label))
  (let card (call Card cart))
  (call follow cart label)
  (call (. card bind))
  (print (str (. label text) " | " (. card bound) " | " (. card plain) " | " (. card noted)))
  (= (. cart quantity) 3)
  (print (str (. label text) " | " (. card bound) " | " (. card plain) " | " (. card noted)))
  (= (. cart quantity) 4)
  (print (str (. label text) " | " (. card bound) " | " (. card plain) " | " (. card noted)))
)

(fn-do + follow _
  (param cart Cart)
  (param label Label)
  (= (. label text) (scope (str q " items") (lazy q (. cart #quantity))))
)

(type + Cart
  (prop + quantity _ (= 1))
)

(type + Label
  (prop + text _ (= ""))
)

(type + Card (is MarkupNode)
  (prop + cart Cart)
  (prop + bound _ (= 0))
  (prop + plain _ (= ""))
  (fn-do + This _
    (param cart _)
  )
  (fn-do + bind _
    (= bound (scope (* q 2) (lazy q (. cart #quantity))))
  )
  (content
    (= plain (scope (str q " x") (lazy q (. cart #quantity))))
    (prop + noted _ (= (scope (str q " noted") (lazy q (. cart #quantity)))))
  )
)
```

```output
1 items | 2 | 1 x | 1 noted
3 items | 6 | 3 x | 3 noted
4 items | 8 | 4 x | 4 noted
```

### A lazy attached to the value of a derived property is part of the value, whether the value reads it plainly or with "#", in a scope or in the block of a call.

```brix
(main
  (let cart (call Cart))
  (print (str (. cart total) " | " (. cart tracked) " | " (. cart shown)))
  (= (. cart quantity) 3)
  (print (str (. cart total) " | " (. cart tracked) " | " (. cart shown)))
  (= (. cart price) 7)
  (print (str (. cart total) " | " (. cart tracked) " | " (. cart shown)))
  (= (. cart quantity) 4)
  (print (str (. cart total) " | " (. cart tracked) " | " (. cart shown)))
)

(fn-expr + padded String
  (param n Int)
  (str "  " n "  ")
)

(type + Cart
  (prop + quantity _ (= 1))
  (prop + price _ (= 5))
  (prop + total _ (= (scope (* q #price) (lazy q #quantity))))
  (prop + tracked _ (= (scope (* #q 2) (lazy q #quantity))))
  (prop + shown _
    (= (call (. (call padded amount) trim)
        (block (lazy amount (+ #quantity #price)))
      )
    )
  )
)
```

```output
5 | 2 | 6
15 | 6 | 8
21 | 6 | 10
28 | 8 | 11
```

### A lazy attached to a live value is its own, apart from a live lazy of its function with its name: a "#" read of either follows what that one reads.

```brix
(main
  (let cart (call Cart))
  (let priced (call Label))
  (let counted (call Label))
  (lazy shown (. cart #price))
  (= (. priced text) (str "price " #shown))
  (= (. counted text)
    (scope (str #shown " items")
      (lazy shown (. cart #quantity))
    )
  )
  (print (str (. priced text) " | " (. counted text)))
  (= (. cart quantity) 3)
  (print (str (. priced text) " | " (. counted text)))
  (= (. cart price) 7)
  (print (str (. priced text) " | " (. counted text)))
)

(type + Cart
  (prop + quantity _ (= 1))
  (prop + price _ (= 5))
)

(type + Label
  (prop + text _ (= ""))
  (fn-do + close _)
)
```

```output
price 5 | 1 items
price 5 | 3 items
price 7 | 3 items
```

### A lazy attached to a live value may be read in more than one arm of "or", "and", "if-expr" or "??", where the arm that reads it first may not run, and by another lazy one arm reads, in a derived property and in a live assignment.

```brix
(main
  (let form (call Form))
  (let view (call View form))
  (let label (call Label))
  (= (. label text)
    (scope (str (or (?? (?. picked folds) false) (?? (?. picked keeps) false)) " " tip)
      (lazy picked (. form #branch))
      (lazy count (. form #runs length))
      (lazy tip (if-expr (> count 1) many "one run"))
      (lazy many (str count " runs"))
    )
  )
  (call report view label)
  (= (. form branch) (call Branch false true))
  (call report view label)
  (+= (. form runs) "a")
  (+= (. form runs) "b")
  (call report view label)
  (= (. form branch) (call Branch true true))
  (call report view label)
  (= (. form runs) (list "c"))
  (call report view label)
)

(fn-do + report _
  (param view View)
  (param label Label)
  (print (str (. view either) " " (. view both) " " (. view chosen) " " (. view fallback) " " (. view tip) " | " (. label text)))
)

(type + Branch
  (prop + folds Bool)
  (prop + keeps Bool)
  (fn-do + This _
    (param folds _)
    (param keeps _)
  )
)

(type + Form
  (prop + branch (? Branch) (= null))
  (prop + runs (list String))
)

(type + Label
  (prop + text _ (= ""))
)

(type + View
  (prop + form Form)
  (prop + either _
    (=
      (scope (or (?? (?. picked folds) false) (?? (?. picked keeps) false))
        (lazy picked (. #form #branch))
      )
    )
  )
  (prop + both _
    (=
      (scope (and (?? (?. picked folds) false) (?? (?. picked keeps) false))
        (lazy picked (. #form #branch))
      )
    )
  )
  (prop + chosen _
    (=
      (scope (if-expr (> (. #form #runs length) 1) (?? (?. picked folds) false) (?? (?. picked keeps) false))
        (lazy picked (. #form #branch))
      )
    )
  )
  (prop + fallback _
    (=
      (scope (?? (?. picked folds) (?? (?. picked keeps) false))
        (lazy picked (. #form #branch))
      )
    )
  )
  (prop + tip _
    (=
      (scope (if-expr (> count 1) many "one run")
        (lazy count (. #form #runs length))
        (lazy many (str count " runs"))
      )
    )
  )
  (fn-do + This _
    (param form _)
  )
)
```

```output
false false false false one run | false one run
true false true false one run | true one run
true false false false 2 runs | true 2 runs
true true true true 2 runs | true 2 runs
true true true true one run | true one run
```
