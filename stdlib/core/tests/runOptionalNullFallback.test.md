# Optional null fallback

```brix
(main
  (let omitted (only-if (call Box) (if (not (call mixed)))))
  (print (call mixed))
  (print (not (call mixed)))
  (print (== omitted null))
  (let context (call Context))
  (call (. context report) omitted)
  (print (== (. context fix) null))
)

(type + Box
  (prop + value Int (= 1))
)

(type + Context
  (prop + fix (? Box))
  (fn-do + report _
    (param fix (? Box) (= null))
    (= (. this fix) fix)
  )
)

(fn-do + mixed Bool
  (return true)
)
```

```output
true
false
true
true
```

```brix
(main
  (let box (call Box))
  (let present (? Box) box)
  (let absent (? Box) null)
  (print (== (?? present null) box))
  (print (== (?? absent null) null))
  (let number (? Int) null)
  (print (== (?? number null) null))
  (= number 7)
  (print (!! (call retain number)))
)

(type + Box
  (prop + value Int (= 1))
)

(fn-expr + retain _
  (param value (? Int))
  (?? value null)
)
```

```output
true
true
true
7
```

```brix
(main
  (let box (call Box))
  (let present (? Box) box)
  (let absent (? Box) null)
  (print (== (?? present (call fallback)) box))
  (print (== (?? absent (call fallback)) null))
)

(type + Box
  (prop + value Int (= 1))
)

(fn-do + fallback _
  (print "fallback")
  (return null)
)
```

```output
true
fallback
true
```

```brix
(main
  (let half F64 7.5)
  (let none F64 0)
  (print (if-expr (not (call is-set)) 0 half))
  (print (if-expr (call is-set) half 0))
  (print (if-expr (not (call is-set)) 0 7.5))
  (print (?? none 2.5))
  (print (?? half 0))
)

(fn-do + is-set Bool
  (return true)
)
```

```output
7.5
7.5
7.5
2.5
7.5
```

### A conditional with a null fallback yields null on every loop iteration where its condition fails.

```brix
(main
  (let values (list 1.5 2.5 3.5))
  (let present (list true false false))
  (for (in index (..< 0 (. values length)))
    (let value (? F64) (only-if ([] values index) (if ([] present index))))
    (print (== value null))
  )
  (for (in index (..< 0 (. values length)))
    (let pair (? Pair) (only-if (call Pair index ([] values index)) (if ([] present index))))
    (print (== pair null))
  )
  (for (in index (..< 0 (. values length)))
    (let inferred (only-if ([] values index) (if ([] present index))))
    (print (== inferred null))
  )
)

(type + Pair (is Tuple)
  (prop + index Index)
  (prop + value F64)
)
```

```output
false
true
true
false
true
true
false
true
true
```

### A declared optional capture keeps null when its initializer converts from a different optional type.

```brix
(main
  (let flag (call off))
  (let a (? F64) (if-expr flag 1.5 null))
  (print (== a null))
  (let b (? F64) (only-if 1.5 (if flag)))
  (print (== b null))
  (let c (? Int) (if-expr flag 3 null))
  (print (== c null))
  (let d (if-expr flag 1.5 null))
  (print (== d null))
  (let e (? F64) (if-expr flag 1.5 (only-if 2.5 (if flag))))
  (print (== e null))
  (let f (? F64) (if-expr flag 1.5 2.5))
  (print (!! f))
)

(fn-do + off Bool
  (return false)
)
```

```output
true
true
true
true
true
2.5
```

### A null branch before a concrete fallback unifies to the fallback's optional type.

```brix
(main
  (let owner U64 7)
  (print (== (call pick true owner) null))
  (print (!! (call pick false owner)))
  (print (== (call pick-do true owner) null))
  (print (!! (call pick-do false owner)))
  (let text (if-expr (== owner 7) null "seven"))
  (print (== text null))
)

(fn-expr + pick (? U64)
  (param hide Bool)
  (param value U64)
  (if-expr hide null value)
)

(fn-do + pick-do (? U64)
  (param hide Bool)
  (param value U64)
  (let kept value)
  (return (if-expr hide null kept))
)
```

```output
true
7
true
7
true
```

### An optional holding zero is present.

```brix
(main
  (let zero (? Index) 0)
  (print (if-expr zero "present" "absent"))
  (print (not zero))
  (let none (? Index) null)
  (print (if-expr none "present" "absent"))
)
```

```output
present
false
absent
```

```brix
(main
  (let three (call Box))
  (= (. three value) 3)
  (let five (call Box))
  (= (. five value) 5)
  (let missing (call Box))
  (= (. missing value) -1)
  (let boxes (call (of List Box)))
  (+= boxes three)
  (+= boxes five)
  (let entries (call (of Map Int (list Box))))
  (= ([] entries 1) boxes)
  (print (. (?? (call cached entries 1 5) missing) value))
  (print (. (?? (call cached entries 1 9) missing) value))
  (print (. (?? (call cached entries 2 5) missing) value))
)

(type + Box
  (prop + value Int (= 1))
)

(fn-do + cached _
  (param entries (of Map Int (list Box)))
  (param id Int)
  (param value Int)
  (let list ([] entries id))
  (return-if null (if (not list)))
  (for (in entry list)
    (if (== (. entry value) value)
      (return entry)
    )
  )
  (return null)
)
```

```output
5
-1
-1
```

```brix
(main
  (let five (call Box))
  (= (. five value) 5)
  (let missing (call Box))
  (= (. missing value) -1)
  (print (. (?? (call loop-only (list five) 5) missing) value))
  (print (. (?? (call loop-only (list five) 9) missing) value))
  (print (. (?? (call null-first (list five) 9) missing) value))
  (print (. (?? (call null-first (call (of List Box)) 5) missing) value))
)

(type + Box
  (prop + value Int (= 1))
)

(fn-do + loop-only _
  (param boxes (list Box))
  (param value Int)
  (for (in entry boxes)
    (if (== (. entry value) value)
      (return entry)
    )
  )
  (return null)
)

(fn-do + null-first _
  (param boxes (list Box))
  (param value Int)
  (return-if null (if (not (. boxes length))))
  (if (== (. ([] boxes 0) value) value)
    (return ([] boxes 0))
  )
  (return null)
)
```

```output
5
-1
-1
-1
```

### null where a condition holds or else a value, which is evaluated only when it is taken

```brix
(main
  (let a (? Int) (if-expr (call flag) null 5))
  (print (?? a -1))
  (let b (if-expr (not (call flag)) null "text"))
  (print (?? b "none"))
  (let c (if-expr (call flag) null (call make)))
  (print (?? c 0))
  (print (?? (if-expr (not (call flag)) null (call make)) 0))
  (let d (if-expr (not (call flag)) null a))
  (print (?? d 7))
  (let e (if-expr (not (call flag)) null (call Point 1 2)))
  (print (. (!! e) y))
  (print (?? (call hidden true) "hidden"))
  (print (?? (call hidden false) "hidden"))
)

(type + Point (is Struct)
  (prop + x I32)
  (prop + y I32)
)

(fn-do + flag Bool
  (return true)
)

(fn-do + make Int
  (print "made")
  (return 9)
)

(fn-expr + hidden (? String)
  (param hide Bool)
  (if-expr hide null "shown")
)
```

```output
-1
text
0
made
9
7
2
hidden
shown
```

### A number that falls back to a number of another type joins with it

```brix
(main
  (let a F64 0)
  (let b I64 0)
  (let c U8 3)
  (let d I32 12)
  (print (?? a 1))
  (print (?? b 2))
  (print (?? c 4))
  (print (?? a d))
  (print (?? d 0.5))
  (print (?? a (call next)))
  (print (?? c (call next)))
)

(fn-do + next I32
  (print "next")
  (return 8)
)
```

```output
1
2
3
12
12
next
8
3
```
