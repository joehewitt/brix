# Struct mutation

```brix
(main
  (let item (call Item 1))
  (= ([] item 0) 7)
  (print (. item x))
)

(type + Item (is Struct)
  (prop + x Int)
  (fn-do + (index-set this (param index _) (param value _)) _
    (= x value)
    (return this)
  )
)
```

```output
7
```

```brix
(main
  (let holder (call Holder (call Item 1)))
  (= ([] (. holder item) 0) 7)
  (print (. holder item x))
)

(type + Item (is Struct)
  (prop + x Int)
  (fn-do + (index-set this (param index _) (param value _)) _
    (= x value)
    (return this)
  )
)

(type + Holder
  (prop + item Item)
  (fn-do + This _
    (param item _)
  )
)
```

```output
7
```

```brix
(main
  (let items (call (of Array Item) (list (call Item 2))))
  (let copy ([] items 0))
  (= ([] ([] items 0) 0) 9)
  (print (. ([] items 0) x))
  (print (. copy x))
)

(type + Item (is Struct)
  (prop + x Int)
  (fn-do + (index-set this (param index _) (param value _)) _
    (= x value)
    (return this)
  )
)
```

```output
9
2
```

```brix
(main
  (let items (call (of Array Item) (list (call Item 2))))
  (let probe (call Probe items))
  (= ([] ([] (call (. probe items)) (call (. probe index))) (call (. probe index))) 9)
  (print (. ([] items 0) x))
  (print (. probe calls))
  (print (. probe indices))
)

(type + Item (is Struct)
  (prop + x Int)
  (fn-do + (index-set this (param index _) (param value _)) _
    (= x value)
    (return this)
  )
)

(type + Probe
  (prop + values (of Array Item))
  (prop + calls _ (= 0))
  (prop + indices _ (= 0))
  (fn-do + This _
    (param values _)
  )
  (fn-do + items _
    (+= calls 1)
    (return values)
  )
  (fn-do + index _
    (+= indices 1)
    (return 0)
  )
)
```

```output
9
1
2
```

```brix
(main
  (let p (call Point 1 2))
  (= p (call (. p move) 5))
  (print p)
  (let q p)
  (= q (call (. q move) 1))
  (print p)
  (print q)
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
  (fn-do + move _
    (param dx I32)
    (= x (+ x dx))
    (return this)
  )
)
```

```output
(6, 2)
(6, 2)
(7, 2)
```

### A method of a struct that assigns a field changes its own copy of the struct, whatever its size.

```brix
(main
  (let item (call Big 1 2 3 4 5))
  (call (. item set) 7)
  (print (. item a))
  (print (tuple (call (. item bump)) (. item a)))
)

(type + Big (is Struct)
  (prop + a I64)
  (prop + b I64)
  (prop + c I64)
  (prop + d I64)
  (prop + e I64)
  (fn-do + set _
    (param value _)
    (= a value)
  )
  (fn-do + bump I64
    (= a (+ a 10))
    (return a)
  )
)
```

```output
1
(11, 1)
```

### What a struct's []= answers is stored where the struct is held: a local, a property, a global, a field, an argument

```brix
(prop + kept _ (= (call Item 1)))
(blank)

(main
  (let item (call Item 1))
  (= ([] item 0) 7)
  (print (. item x))
  (let holder (call Holder (call Item 1) (call Pair (call Item 2) 3)))
  (= ([] (. holder item) 0) 8)
  (print (. holder item x))
  (= ([] kept 0) 9)
  (print (. kept x))
  (let pair (call Pair (call Item 4) 5))
  (= ([] (. pair item) 0) 10)
  (print (tuple (. pair item x) (. pair count)))
  (print (. (call change (call Item 11)) x))
  (call (. holder set) 12)
  (print (. holder item x))
  (let made (call Item 1))
  (= ([] made 0) (+ (. made x) 40))
  (print (. made x))
)

(fn-do + change Item
  (param given Item)
  (= ([] given 0) 20)
  (return given)
)

(type + Item (is Struct)
  (prop + x Int)
  (fn-do + (index-set this (param index _) (param value _)) _
    (= x value)
    (return this)
  )
)

(type + Pair (is Struct)
  (prop + item Item)
  (prop + count Int)
)

(type + Holder
  (prop + item Item)
  (prop + pair Pair)
  (fn-do + This _
    (param item _)
    (param pair _)
  )
  (fn-do + set _
    (param value Int)
    (= ([] item 0) value)
  )
)
```

```output
7
8
9
(10, 5)
20
12
41
```

### What []= answers for a struct an index gave is stored through that index

```brix
(prop + kept _ (= (call (of Array Item) (list (call Item 1) (call Item 2)))))
(blank)

(main
  (let items (call (of Array Item) (list (call Item 1) (call Item 2) (call Item 3))))
  (= ([] ([] items 1) 0) 7)
  (print (tuple (. ([] items 0) x) (. ([] items 1) x) (. ([] items 2) x)))
  (let holder (call Holder (call (of Array Item) (list (call Item 4) (call Item 5)))))
  (= ([] ([] (. holder items) 1) 0) 8)
  (print (tuple (. ([] (. holder items) 0) x) (. ([] (. holder items) 1) x)))
  (= ([] ([] kept 0) 0) 9)
  (print (tuple (. ([] kept 0) x) (. ([] kept 1) x)))
  (call (. holder set) 1 12)
  (print (. ([] (. holder items) 1) x))
  (print (call change items 2))
  (print (. ([] items 2) x))
  (let position 0)
  (= ([] ([] items position) 0) (+ (. ([] items (+ position 1)) x) 40))
  (print (. ([] items 0) x))
  (let list (list (call Item 1) (call Item 2)))
  (= ([] ([] list 1) 0) 30)
  (print (tuple (. ([] list 1) x) (. ([] list 0) x)))
  (let grid (call (of Array (of Array Item)) (list (call (of Array Item) (list (call Item 1) (call Item 2))) (call (of Array Item) (list (call Item 3))))))
  (= ([] ([] ([] grid 0) 1) 0) 50)
  (print (tuple (. ([] ([] grid 0) 1) x) (. ([] ([] grid 0) 0) x) (. ([] ([] grid 1) 0) x)))
)

(fn-do + change Int
  (param given (of Array Item))
  (param place Index)
  (= ([] ([] given place) 0) 20)
  (return (. ([] given place) x))
)

(type + Item (is Struct)
  (prop + x Int)
  (fn-do + (index-set this (param index _) (param value _)) _
    (= x value)
    (return this)
  )
)

(type + Holder
  (prop + items (of Array Item))
  (fn-do + This _
    (param items _)
  )
  (fn-do + set _
    (param place Index)
    (param value Int)
    (= ([] ([] items place) 0) value)
  )
)
```

```output
(1, 7, 3)
(4, 8)
(9, 2)
12
20
20
47
(30, 1)
(50, 1, 3)
```

### An index of an index assigned evaluates the value, the object and each key once, and raises what either function raises

```brix
(main
  (let items (call (of Array Item) (list (call Item 1) (call Item 2) (call Item 3))))
  (let seen (call Seen))
  (= ([] ([] items (call (. seen next))) (call (. seen next))) (call (. seen next)))
  (print (tuple (. ([] items 0) x) (. ([] items 1) x) (. ([] items 2) x) (. seen count)))
  (call store items 2 1)
  (print (. ([] items 2) x))
  (call store items 2 5)
  (print (. ([] items 2) x))
  (call store items 7 1)
)

(fn-do + store _
  (param items (of Array Item))
  (param place Index)
  (param field Int)
  (let attempt
    (do-new
      (= ([] ([] items place) field) 40)
      (print "stored")
      (on-failure failure
        (let panic (? Panic) failure)
        (if panic
          (print (str "recovered " panic))
        )
      )
    )
  )
  (call (. attempt wait))
)

(type + Item (is Struct)
  (prop + x Int)
  (fn-do + (index-set this (param index Int) (param value Int)) _
    (throw (call Panic (str "No field " index)) (if (> index 2)))
    (= x (+ value index))
    (return this)
  )
)

(type + Seen
  (prop + count _ (= 0))
  (fn-do + next _
    (+= count 1)
    (print (str "next " count))
    (return (- count 1))
  )
)
```

```output
next 1
next 2
next 3
(1, 2, 3, 3)
stored
41
recovered No field 5
41
recovered Index out of bounds (7 / 3)
```

### The value of an index of an index assigned is the value written, which is converted for the store

```brix
(main
  (let items (call (of Array Item) (list (call Item 1) (call Item 2))))
  (let taken (= ([] ([] items 1) 0) 7))
  (print (tuple taken (. ([] items 1) x)))
  (let wide (call (of Array Wide) (list (call Wide 1 2) (call Wide 3 4))))
  (= ([] ([] wide 1) 0) 2)
  (print (tuple (. ([] wide 1) a) (. ([] wide 1) b) (. ([] wide 0) a)))
  (let other wide)
  (= ([] ([] other 0) 1) 9)
  (print (. ([] wide 0) b))
)

(type + Item (is Struct)
  (prop + x Int)
  (fn-do + (index-set this (param index _) (param value _)) _
    (= x value)
    (return this)
  )
)

(type + Wide (is Struct)
  (prop + a I64)
  (prop + b I64)
  (fn-do + (index-set this (param index Int) (param value I64)) _
    (if (== index 0)
      (= a value)
      (else
        (= b value)
      )
    )
    (return this)
  )
)
```

```output
(7, 7)
(2, 4, 1)
9
```

### A struct indexed for a store is read where its function is called, after the key replaced it

```brix
(main
  (let stack (call Stack))
  (call (. stack push) 5)
  (call (. stack push) 6)
  (call (. stack push) 7)
  (print (tuple ([] (. stack values) 0) ([] (. stack values) 1) ([] (. stack values) 2)))
)

(type + Stack
  (prop + values _ (= (call (of Array Int) 1)))
  (prop + count _ (= 0))
  (fn-do + push _
    (param value Int)
    (= ([] values (call slot)) value)
  )
  (fn-do + slot Int
    (if (== count (. values size))
      (= values (call (. values resize) (* count 2)))
    )
    (+= count 1)
    (return (- count 1))
  )
)
```

```output
(5, 6, 7)
```

### A struct indexed is read where its function is called, after the key assigned it

```brix
(main
  (let item (call Item 1))
  (print ([] item (. (= item (call Item 50)) x)))
  (let numbers (call (of Array Int) (list 1 2 3)))
  (print ([] numbers (- (. (= numbers (call (of Array Int) (list 7 8 9))) size) 2)))
)

(type + Item (is Struct)
  (prop + x Int)
  (fn-expr + (index-get this (param index Int)) Int
    (+ x index)
  )
)
```

```output
100
8
```

### What an index store answers is stored where the object was read from, though the key names another since

```brix
(main
  (let a (call Holder))
  (let b (call Holder))
  (let h a)
  (= ([] (. h item) (. (= h b) zero)) 7)
  (print (tuple (. a item x) (. b item x)))
)

(type + Item (is Struct)
  (prop + x Int)
  (fn-do + (index-set this (param index Int) (param value Int)) _
    (= x value)
    (return this)
  )
)

(type + Holder
  (prop + item _ (= (call Item 1)))
  (prop + zero _ (= 0))
)
```

```output
(7, 1)
```

### What an index store answers is stored where the object was read from, though the function names another since

```brix
(prop + current _ (= (call Holder)))
(prop + other _ (= (call Holder)))
(blank)

(main
  (let first current)
  (= ([] (. current item) 0) 7)
  (print (tuple (. first item x) (. other item x) (== current other)))
)

(type + Item (is Struct)
  (prop + x Int)
  (fn-do + (index-set this (param index Int) (param value Int)) _
    (= current other)
    (= x value)
    (return this)
  )
)

(type + Holder
  (prop + item _ (= (call Item 1)))
)
```

```output
(7, 1, true)
```

### A number an index store answers is stored where the object was read from

```brix
(main
  (let a (call Holder))
  (let b (call Holder))
  (let h a)
  (= ([] (. h count) (. (= h b) zero)) 7u8)
  (print (tuple (as (. a count) U8) (as (. b count) U8)))
)

(type + (of Count Integer) (is Integer)
  (fn-expr + (index-set this (param index Index) (param value Integer)) _
    (as (+ (as this Integer) value) This)
  )
)

(type + Holder
  (prop + count _ (= (as 1 (of Count U8))))
  (prop + zero Index (= 0))
)
```

```output
(8, 1)
```

### A field of a struct an index names is stored where the struct lies: in a list, past a pointer, in a list a property holds, and in a struct inside either.

```brix
(import /core/arena)
(blank)

(main
  (let arena (call Arena))
  (finally
    (inline (call (. arena release)))
  )
  (let l (list S))
  (+= l (call S 1 2))
  (= (. ([] l 0) a) 10)
  (+= (. ([] l 0) b) 5)
  (let al (call (of List S) arena))
  (+= al (call S 1 2))
  (= (. ([] al 0) a) 11)
  (let p (call (of (. arena allocate) Outer) 2))
  (= ([] p 1) (call Outer (call S 1 2) 3))
  (= (. ([] p 1) inner b) 20)
  (+= (. ([] p 1) inner a) 4)
  (let w (call Wrapper))
  (= (. ([] (. w items) 0) a) 15)
  (let nested (list Outer))
  (+= nested (call Outer (call S 1 2) 3))
  (= (. ([] nested 0) inner a) 16)
  (-= (. ([] nested 0) c) 1)
  (let named (list N))
  (+= named (call N 1 "one"))
  (= (. ([] named 0) name) "uno")
  (*= (. ([] named 0) a) 3)
  (print (str (. ([] l 0) a) " " (. ([] l 0) b) " " (. ([] al 0) a)))
  (print (str (. ([] p 1) inner a) " " (. ([] p 1) inner b) " " (. ([] p 1) c)))
  (print (str (. ([] (. w items) 0) a) " " (. ([] nested 0) inner a) " " (. ([] nested 0) c) " " (. ([] named 0) a) " " (. ([] named 0) name)))
)

(type + S (is Struct)
  (prop + a Int)
  (prop + b Int)
)

(type + Outer (is Struct)
  (prop + inner S)
  (prop + c Int)
)

(type + N (is Struct)
  (prop + a Int)
  (prop + name String)
)

(type + Wrapper
  (prop + items (list S))
  (fn-do + This _
    (+= items (call S 1 2))
  )
)
```

```output
10 7 11
5 20 3
15 16 2 3 uno
```

### A field of a struct an array's item holds is stored in the item, also inside a struct it holds and with an assigning operator.

```brix
(main
  (let arr (call (of Array S) 2))
  (= ([] arr 0) (call S 1 2))
  (= ([] arr 1) (call S 3 4))
  (= (. ([] arr 0) a) 12)
  (+= (. ([] arr 1) b) 5)
  (let slot 1)
  (*= (. ([] arr slot) a) 2)
  (print (str (. ([] arr 0) a) " " (. ([] arr 0) b) " " (. ([] arr 1) a) " " (. ([] arr 1) b)))
  (let outer (call (of Array Outer) 1))
  (= ([] outer 0) (call Outer (call S 1 2) 3))
  (= (. ([] outer 0) inner a) 7)
  (-= (. ([] outer 0) c) 1)
  (print (str (. ([] outer 0) inner a) " " (. ([] outer 0) inner b) " " (. ([] outer 0) c)))
)

(type + S (is Struct)
  (prop + a Int)
  (prop + b Int)
)

(type + Outer (is Struct)
  (prop + inner S)
  (prop + c Int)
)
```

```output
12 2 6 9
7 2 2
```
