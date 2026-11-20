# Panic

```brix crash
(main
  (throw (call Panic "boom"))
)
```

```brix crash
(main
  (call gc-free-panic)
)

(fn-do + gc-free-panic _ (without gc)
  (throw (call Panic "gc-free boom"))
)
```

```stderr
Unhandled panic: gc-free boom
<no file:7>

Task 1

Stack trace:
  0  __source__:gc-free-panic no file:7:3
  1  __source__:main no file:3:3
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

```brix crash
(main
  (call outer)
)

(fn-do + outer _
  (call middle)
)

(fn-do + middle _
  (call inner)
)

(fn-do + inner _
  (throw (call Panic "boom"))
)
```

```stderr
Unhandled panic: boom
<no file:15>

Task 1

Stack trace:
  0  __source__:inner no file:15:3
  1  __source__:middle no file:11:3
  2  __source__:outer no file:7:3
  3  __source__:main no file:3:3
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

```brix crash
(main
  (call foo)
)

(fn-do + foo _
  (finally
    (print "cleanup")
  )
  (blank)
  (throw (call Panic "boom"))
)
```

```output
cleanup
```

```brix
(main
  (let attempt
    (do-new
      (call foo)
      (on-failure (: failure SpecialPanic)
        (print (str "special " failure))
      )
      (on-failure failure
        (print (str "other " failure))
      )
    )
  )
  (call (. attempt wait))
  (print "done")
)

(type + SpecialPanic (is Panic))

(fn-do + foo _
  (throw (call SpecialPanic "typed"))
)
```

```output
special typed
done
```

```brix
(main
  (let attempt
    (do-new
      (call outer)
      (on-failure failure
        (print (str "caught " failure))
      )
    )
  )
  (call (. attempt wait))
  (print "done")
)

(fn-do + outer _
  (call inner)
  (print "after outer")
)

(fn-do + inner _
  (finally
    (print (str "inner cleanup sees " (. (call Task) failure)))
    (throw (call Panic "wrapped"))
  )
  (blank)
  (throw (call Panic "inner"))
  (print "after inner")
)
```

```output
inner cleanup sees inner
caught wrapped
done
```

```brix
(main
  (let attempt
    (do-new
      (call foo)
      (on-failure _
        (print)
      )
    )
  )
  (call (. attempt wait))
  (print "done")
)

(fn-do + foo _
  (finally
    (print "first cleanup")
  )
  (blank)
  (finally
    (print (str "middle cleanup sees " (. (call Task) failure)))
  )
  (blank)
  (finally
    (print "last cleanup")
  )
  (blank)
  (throw (call Panic "boom"))
  (print "after")
)
```

```output
last cleanup
middle cleanup sees boom
first cleanup
done
```

```brix
(main
  (let attempt
    (do-new
      (call outer)
      (on-failure failure
        (print (str "caught " failure))
      )
    )
  )
  (call (. attempt wait))
  (print "done")
)

(fn-do + outer _
  (call middle)
  (print "after outer")
)

(fn-do + middle _
  (call inner)
  (print "after middle")
)

(fn-do + inner _
  (print "before inner")
  (throw (call Panic "deep"))
  (print "after inner")
)
```

```output
before inner
caught deep
done
```

```brix
(main
  (let attempt
    (do-new
      (call outer)
      (on-failure _
        (print)
      )
    )
  )
  (call (. attempt wait))
  (print "done")
)

(fn-do + outer _
  (finally
    (print (str "outer cleanup sees " (. (call Task) failure)))
  )
  (blank)
  (call inner)
)

(fn-do + inner _
  (finally
    (print "inner cleanup 1")
  )
  (blank)
  (finally
    (print "inner cleanup 2")
  )
  (blank)
  (throw (call Panic "inner"))
)
```

```output
inner cleanup 2
inner cleanup 1
outer cleanup sees inner
done
```

```brix
(main
  (let attempt
    (do-new
      (call foo)
      (on-failure failure
        (print (str failure ": " (. failure source)))
      )
    )
  )
  (call (. attempt wait))
  (print "done")
)

(fn-do + foo _
  (throw (call Panic "manual" (call ErrorSource "manual.bx" 99 7)))
)
```

```output
manual: manual.bx:99:7
done
```

### A panic unwinding through an implicit Result return still reaches the caller.

```brix crash
(main
  (call propagating)
  (print "unreachable")
)

(fn-do + read Int (?! IOError)
  (param ok Bool)
  (if ok
    (return 7)
    (else
      (throw (call IOError "disk"))
    )
  )
)

(fn-do + propagating _
  (let value (call read true))
  (print (str "value " value))
  (throw (call Panic "boom after result"))
)
```

```output
value 7
```

```brix
(main
  (call attempt 0)
  (call attempt 1)
  (call attempt 2)
)

(fn-do + attempt _
  (param which Int)
  (let task
    (do-new
      (call choose which)
      (on-failure failure
        (print (str (. failure message) ": " (?. (. failure source) line) ":" (?. (. failure source) column)))
      )
    )
  )
  (call (. task wait))
)

(fn-do + choose _
  (param which Int)
  (if (== which 0)
    (let missing (? Int))
    (print (!! missing))
    (else-if (== which 1)
      (let missing (? Int))
      (print (!! missing))
    )
    (else
      (throw (call Panic "existing"))
    )
  )
)
```

```output
Optional'core/number:I64 is null: 25:12
Optional'core/number:I64 is null: 28:14
existing: 31:7
```

### An unhandled panic is reported with the text its own class converts it to.

```brix crash
(main
  (throw (call MyPanic))
)

(type + MyPanic (is Panic)
  (fn-to-expr + String
    "custom text"
  )
)
```

```stderr
Unhandled panic: custom text
<no file:3>

Task 1

Stack trace:
  0  __source__:main no file:3:3
  1  __source__:init.__source__ no file
  2  __source__:brix.main no file
```

### A core error made with no message has the text of its own class.

```brix
(main
  (print (call Panic))
  (print (call OutOfBoundsError))
  (print (call TaskCancelationError))
  (print (call IOError))
  (print (call TimeoutError))
)
```

```output
Panic
Out of bounds
Task cancelled
IO error
Timed out
```

### A raise under a condition that does not hold raises nothing.

```brix
(main
  (let x 3)
  (throw (call Panic "big") (if (> x 5)))
  (print "after")
)
```

```output
after
```

### A raise under a condition that holds raises.

```brix crash
(main
  (let x 9)
  (throw (call Panic "big") (if (> x 5)))
  (print "never")
)
```

### A panic raised under a text is raised when the text is not empty.

```brix
(main
  (let t "")
  (throw (call Panic "empty") (if t))
  (print "after")
)
```

```output
after
```

### A panic raised below a recursive call leaves through every caller, and nothing after a call runs.

```brix crash
(main
  (call down 3)
  (print "after")
)

(fn-do + down _
  (param n Int)
  (if (> n 0)
    (call down (- n 1))
    (print n)
    (return)
  )
  (throw (call Panic "bottom"))
)
```

```stderr
Unhandled panic: bottom
<no file:14>

Task 1

Stack trace:
  0  __source__:down no file:14:3
  1  __source__:down no file:10:5
  2  __source__:down no file:10:5
  3  __source__:down no file:10:5
  4  __source__:main no file:3:3
  5  __source__:init.__source__ no file
  6  __source__:brix.main no file
```

### A panic leaves through two functions that call each other.

```brix crash
(main
  (call a 2)
  (print "after")
)

(fn-do + a _
  (param n Int)
  (call b n)
  (print (str "a " n))
)

(fn-do + b _
  (param n Int)
  (if (> n 0)
    (call a (- n 1))
    (print (str "b " n))
    (return)
  )
  (throw (call Panic "bottom"))
)
```

```stderr
Unhandled panic: bottom
<no file:20>

Task 1

Stack trace:
  0  __source__:b no file:20:3
  1  __source__:a no file:9:3
  2  __source__:b no file:16:5
  3  __source__:a no file:9:3
  4  __source__:b no file:16:5
  5  __source__:a no file:9:3
  6  __source__:main no file:3:3
  7  __source__:init.__source__ no file
  8  __source__:brix.main no file
```

### A call that raises nothing goes on, and one that raises ends the statement it is in.

```brix crash
(main
  (print (call check 1))
  (print (+ (call check -1) (call check 2)))
  (print "after")
)

(fn-do + check Int
  (param n Int)
  (throw (call Panic "negative") (if (< n 0)))
  (return n)
)
```

```stderr
Unhandled panic: negative
<no file:10>

Task 1

Stack trace:
  0  __source__:check no file:10:3
  1  __source__:main no file:4:13
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

### A panic raised under the constant true ends its function, which needs no return after it.

```brix crash
(main
  (print (call pick))
)

(fn-do + pick Int
  (throw (call Panic "always") (if true))
)
```

```stderr
Unhandled panic: always
<no file:7>

Task 1

Stack trace:
  0  __source__:pick no file:7:3
  1  __source__:main no file:3:10
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

### A cleanup runs once at each way out of its scope: an iteration that ends, a panic, and a return whose value was made before it.

```brix
(main
  (let attempt
    (do-new
      (call foo)
      (on-failure failure
        (print (str "caught " failure))
      )
    )
  )
  (call (. attempt wait))
  (print "done")
  (print (call value))
)

(fn-do + foo _
  (for (in i (..< 0 3))
    (finally
      (print (str "iter " i))
    )
    (if (== i 1)
      (throw (call Panic "boom"))
    )
    (print i)
  )
)

(fn-do + value Int
  (let n 1)
  (finally
    (= n 9)
    (print (str "cleanup " n))
  )
  (blank)
  (if (== n 1)
    (finally
      (print "inner")
    )
    (return (+ n 1))
  )
  (return 0)
)
```

```output
0
iter 0
iter 1
caught boom
done
inner
cleanup 9
2
```

### A return gives the value its expression had before the cleanups ran.

```brix
(main
  (print (call scalar))
  (print (call fresh))
  (print (. (call field) a))
  (print (call text))
)

(type + Holder
  (prop + a _ (= 1))
)

(fn-do + scalar Int
  (let n 1)
  (finally
    (= n 9)
  )
  (return n)
)

(fn-do + fresh (tuple Int Int)
  (let made (tuple 1 2))
  (finally
    (= made (tuple 7 8))
  )
  (return (tuple (+ ([] made 0) 1) (+ ([] made 1) 1)))
)

(fn-do + field Holder
  (let held (call Holder))
  (finally
    (= (. held a) 9)
  )
  (return held)
)

(fn-do + text String
  (let said "before")
  (finally
    (= said "after")
  )
  (return said)
)
```

```output
1
(2, 3)
9
before
```

### A panic raised in a cleanup as its block is left by its end, a `break`, a `continue` or a return leaves the function once the cleanups have run.

```brix
(main
  (call watch 1)
  (call watch 2)
  (call watch 3)
  (call watch 4)
  (call watch 5)
  (print "end")
)

(fn-do + watch _
  (param way Int)
  (let attempt
    (do-new
      (print (str "left " (call leave way)))
      (on-failure failure
        (print (str "way " way ": " failure))
      )
    )
  )
  (call (. attempt wait))
)

(fn-do + leave Int
  (param way Int)
  (let count 0)
  (while (< count 3)
    (+= count 1)
    (finally
      (print (str "cleanup " count))
      (if (== count way)
        (throw (call Panic (str "pass " count)))
      )
    )
    (if (== count 1)
      (continue)
    )
    (if (== count 2)
      (break)
    )
  )
  (if (== way 4)
    (finally
      (throw (call Panic "at the return"))
    )
    (return 4)
  )
  (do
    (finally
      (if (== way 3)
        (throw (call Panic "at the end of a block"))
      )
    )
    (print "block")
  )
  (return count)
)
```

```output
cleanup 1
way 1: pass 1
cleanup 1
cleanup 2
way 2: pass 2
cleanup 1
cleanup 2
block
way 3: at the end of a block
cleanup 1
cleanup 2
way 4: at the return
cleanup 1
cleanup 2
block
left 2
end
```

### A panic raised in a cleanup takes the place of one raised before it, and the cleanups of the blocks around still run.

```brix
(main
  (call run)
  (call watch)
  (print "end")
)

(fn-do + run _
  (let attempt
    (do-new
      (do
        (finally
          (throw (call Panic "one"))
        )
        (finally
          (throw (call Panic "two"))
        )
        (print "body")
      )
      (print "not reached")
      (on-failure failure
        (print (str "run sees " failure))
      )
    )
  )
  (call (. attempt wait))
)

(fn-do + watch _
  (let attempt
    (do-new
      (print (call first true))
      (on-failure failure
        (print (str "watch sees " failure))
      )
    )
  )
  (call (. attempt wait))
)

(fn-do + first Int
  (param deep Bool)
  (finally
    (print "outer cleanup")
  )
  (do
    (finally
      (print "middle cleanup")
      (throw (call Panic "middle"))
    )
    (if deep
      (finally
        (print "inner cleanup")
      )
      (return 5)
    )
  )
  (return 6)
)
```

```output
body
run sees one
inner cleanup
middle cleanup
outer cleanup
watch sees middle
end
```

### A returned value is converted to the type the function returns before the cleanups run, which a panic the conversion raises runs.

```brix
(main
  (print (call wrapped))
  (print (call described))
  (call check)
  (print "end")
)

(fn-do + wrapped Wrapped
  (let s "s")
  (finally
    (print "cleanup")
    (= s "changed")
  )
  (return s)
)

(fn-do + described String
  (let t (call Thing))
  (finally
    (= (. t n) 9)
  )
  (return (str t))
)

(fn-do + check _
  (let attempt
    (do-new
      (print (call held))
      (on-failure failure
        (print (str "check sees " failure))
      )
    )
  )
  (call (. attempt wait))
)

(fn-do + held Int
  (let o (? Int))
  (finally
    (let failure (. (call Task) failure))
    (if failure
      (print (str "cleanup sees " failure))
      (else
        (print "no panic")
      )
    )
  )
  (return (!! o))
)

(type + Wrapped (is (tuple (: text String)))
  (fn-from-expr + (: other String)
    (before
      (print (str "from " other))
    )
    (call Wrapped (str other "!"))
  )
)

(type + Thing
  (prop + n _ (= 1))
  (fn-to-expr + String
    (str "thing " n)
  )
)
```

```output
from s
cleanup
(s!)
thing 1
cleanup sees Optional'core/number:I64 is null
check sees Optional'core/number:I64 is null
end
```

### A panic raised in a cleanup where its function ends is reported when nothing recovers it.

```brix crash
(main
  (call go)
  (print "after")
)

(fn-do + go _
  (finally
    (throw (call Panic "in cleanup"))
  )
  (print "body")
)
```

```output
body
```

### A panic names a tuple with no name by its fields.

```brix
(main
  (call plain)
  (call named)
  (print "end")
)

(fn-do + plain _
  (let attempt
    (do-new
      (let t (? (tuple Int String)))
      (print (!! t))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)

(fn-do + named _
  (let attempt
    (do-new
      (let t (? (tuple (: a Int) (: b (tuple Float Bool)))))
      (print (!! t))
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)
```

```output
Optional'Tuple:(core/number:I64, core/string:String) is null
Optional'Tuple:(a: core/number:I64, b: Tuple:(core/number:F64, core/number:Bool)) is null
end
```

### A panic raised in a closure ends the program after what was printed.

```brix crash
(main
  (let limit 3)
  (let gate-direct
    (=> (amount) :do
      (block
        (throw (call Panic (str "direct past " limit)) (if (> amount limit)))
        (return (* amount 11))
      )
    )
  )
  (print (call gate-direct 2))
  (print (call gate-direct 9))
)
```

```output
22
```

### A panic raised in a closure called through a function value.

```brix crash
(main
  (let limit 4)
  (let gate-valued
    (=> (amount) :do
      (block
        (throw (call Panic (str "valued past " limit)) (if (> amount limit)))
        (return (* amount 12))
      )
    )
  )
  (print (call apply-valued gate-valued 2))
  (let kept ValuedCheck gate-valued)
  (print (call kept 3))
  (print (call apply-valued gate-valued 9))
)

(type + ValuedCheck (is (fn-type Int (param amount Int))))

(fn-do + apply-valued Int
  (param check ValuedCheck)
  (param amount Int)
  (return (+ (call check amount) 100))
)
```

```output
124
36
```

### A panic raised in a function bound and kept as a function value.

```brix crash
(main
  (let check BoundCheck (call limit-bound 5 ...))
  (print (call check 2))
  (print (call apply-bound check 9))
  (print (call settle-bound 1))
)

(type + BoundCheck (is (fn-type Int (param amount Int))))

(fn-do + apply-bound Int
  (param check BoundCheck)
  (param amount Int)
  (return (+ (call check amount) 200))
)

(fn-do + limit-bound Int
  (param limit Int)
  (param amount Int)
  (throw (call Panic (str "bound past " limit)) (if (> amount limit)))
  (return (* amount 13))
)

(fn-do + settle-bound Int
  (param amount Int)
  (return (+ amount 201))
)
```

```output
26
```

### A panic raised under one of three closures passed as function values.

```brix crash
(main
  (let gate-one (=> (amount) (call fail-each amount "one")))
  (let gate-two (=> (amount) (call fail-each (+ amount 1) "two")))
  (let gate-three (=> (amount) (call fail-each (+ amount 2) "three")))
  (print (call apply-each gate-one 2))
  (print (call apply-each gate-two 2))
  (print (call apply-each gate-three 9))
)

(type + EachCheck (is (fn-type Int (param amount Int))))

(fn-do + apply-each Int
  (param check EachCheck)
  (param amount Int)
  (return (+ (call check amount) 300))
)

(fn-do + fail-each Int
  (param amount Int)
  (param text String)
  (throw (call Panic (str "each " text)) (if (> amount 6)))
  (return (* amount 14))
)
```

```output
328
342
```

### A panic raised in a closure written in a closure.

```brix crash
(main
  (let limit 7)
  (let gate-outer
    (=> (amount) :do
      (block
        (let gate-inner
          (=> (value) :do
            (block
              (throw (call Panic (str "inner past " limit)) (if (> value limit)))
              (return (* value 15))
            )
          )
        )
        (return (+ (call gate-inner amount) 500))
      )
    )
  )
  (print (call gate-outer 1))
  (print (call gate-outer 9))
)
```

```output
515
```

### A panic raised in a closure assigned to an optional property of a function type.

```brix crash
(main
  (let holder (call HeldGate))
  (print (call (. holder apply-held) 5))
  (let gate-held
    (=> (amount) :do
      (block
        (throw (call Panic "held past 8") (if (> amount 8)))
        (return (* amount 16))
      )
    )
  )
  (= (. holder action) gate-held)
  (print (call (. holder apply-held) 1))
  (print (call (. holder apply-held) 9))
)

(type + HeldCheck (is (fn-type Int (param amount Int))))

(type + HeldGate
  (prop + action (? HeldCheck))
  (fn-do + apply-held Int
    (param amount Int)
    (let step action)
    (if step
      (return (+ (call step amount) 600))
    )
    (return -1)
  )
)
```

```output
-1
616
```

### A panic raised in a closure a method returns, called after the method.

```brix crash
(main
  (let gate (call PassGate))
  (= (. gate limit) 9)
  (let passed (call (. gate passer)))
  (print (call passed 2))
  (print (call passed 10))
)

(type + PassGate
  (prop + limit _ (= 0))
  (fn-do + passer _
    (let gate-passed
      (=> (amount) :do
        (block
          (throw (call Panic (str "passed past " limit)) (if (> amount limit)))
          (return (* amount 17))
        )
      )
    )
    (return gate-passed)
  )
)
```

```output
34
```

```brix crash
(main
  (call choose 0)
)

(fn-do + choose _
  (param which Int)
  (if (== which 0)
    (let missing (? Int))
    (print (!! missing))
    (else-if (== which 1)
      (let missing (? Int))
      (print (!! missing))
    )
    (else
      (throw (call Panic "existing"))
    )
  )
)
```

```stderr
Unhandled panic: Optional'core/number:I64 is null
<no file:10>

Task 1

Stack trace:
  0  __source__:choose no file:10:12
  1  __source__:main no file:3:3
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

```brix crash
(main
  (call choose 1)
)

(fn-do + choose _
  (param which Int)
  (if (== which 0)
    (let missing (? Int))
    (print (!! missing))
    (else-if (== which 1)
      (let missing (? Int))
      (print (!! missing))
    )
    (else
      (throw (call Panic "existing"))
    )
  )
)
```

```stderr
Unhandled panic: Optional'core/number:I64 is null
<no file:13>

Task 1

Stack trace:
  0  __source__:choose no file:13:14
  1  __source__:main no file:3:3
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

```brix crash
(main
  (call choose 2)
)

(fn-do + choose _
  (param which Int)
  (if (== which 0)
    (let missing (? Int))
    (print (!! missing))
    (else-if (== which 1)
      (let missing (? Int))
      (print (!! missing))
    )
    (else
      (throw (call Panic "existing"))
    )
  )
)
```

```stderr
Unhandled panic: existing
<no file:16>

Task 1

Stack trace:
  0  __source__:choose no file:16:7
  1  __source__:main no file:3:3
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

```brix crash
(import /core/markup)
(import /core/tests/support/markup-fixtures)
(blank)

(main
  (<> Panel (id: panel))
)

(type + Panel (is Box)
  (prop + level I64 (= (call boom)))
  (content
    (<> Box)
  )
)

(fn-do + boom I64
  (throw (call Panic "boom"))
)
```

```stderr
Unhandled panic: boom
<no file:18>

Task 1

Stack trace:
  0  __source__:boom no file:18:3
  1  __source__:Panel:Panel no file:11:24
  2  __source__:Panel:Panel no file:10:1
  3  __source__:main no file:7:3
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```

### A panic raised making a value of an enum that main reads first ends the program before main runs.

```brix crash
(main
  (print "before")
  (print (. Scale minor title))
)

(type + Scale (is (tuple (: title String) (: notes (list Note))))
  (variant minor (= (tuple "Minor" (list (call Note 3)))))
  (variant major (= (tuple "Major" (list (call Note 1)))))
)

(type + Note
  (prop + pitch Int)
  (fn-do + This _
    (param pitch Int)
    (throw (call Panic (str "no pitch " pitch)) (if (> pitch 2)))
    (= (. this pitch) pitch)
  )
)
```

```stderr
Unhandled panic: no pitch 3
<no file:16>

Task 1

Stack trace:
  0  __source__:Note:This no file:16:5
  1  __source__:init.__source__.statics no file:8:42
  2  __source__:brix.main no file
```

### A panic raised by a Readable's read through the interface reaches the caller.

```brix crash
(main
  (call pull (call Boom))
  (print "end")
)

(type + Boom (implements (of Readable Int))
  (prop + count Int)
  (fn-do + (prefix <- this) (? Int)
    (throw (call Panic "boom") (if (== count 0)))
    (return count)
  )
)

(fn-do _ pull _
  (param source (of Readable Int))
  (let got (<- source))
  (print "after pull")
)
```

```stderr
Unhandled panic: boom
<no file:10>

Task 1

Stack trace:
  0  __source__:Boom:<-read no file:10:5
  1  __source__:<-read(wrapped) no file:9:3
  2  __source__:pull no file:17:12
  3  __source__:main no file:3:3
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file
```
