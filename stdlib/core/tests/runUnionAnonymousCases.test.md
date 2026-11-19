# Union anonymous cases

```brix
(main
  (let result
    (match true
      (case? true
        ..ok
      )
      (default
        ..failed
      )
    )
  )
  (blank)
  (print (is result .ok))
  (print (. result name))
)
```

```output
true
ok
```

```brix
(main
  (let result
    (match true
      (case? true
        (call ..valid "email" "password")
      )
      (default
        ..invalid-email
      )
    )
  )
  (blank)
  (print
    (match result
      (case (call .valid email password)
        (str email ":" password)
      )
      (case .invalid-email
        "email"
      )
    )
  )
)
```

```output
email:password
```

```brix
(main
  (let use-same false)
  (let result
    (match use-same
      (case? use-same
        ..same
      )
      (default
        ..same
      )
    )
  )
  (blank)
  (print (. result name))
  (print (. result case))
)
```

```output
same
0
```

```brix
(main
  (let use-int false)
  (let result
    (match use-int
      (case? use-int
        (call ..value 1)
      )
      (default
        (call ..value 1.5)
      )
    )
  )
  (blank)
  (print
    (match result
      (case (call .value x)
        x
      )
    )
  )
)
```

```output
1.5
```

```brix
(main
  (let input "no")
  (let event
    (match input
      (~ "ok"
        ..ok
      )
      (default
        (call ..failed 404)
      )
    )
  )
  (blank)
  (print
    (match event
      (case .ok
        "ok"
      )
      (case (call .failed code)
        (str "failed:" code)
      )
    )
  )
)
```

```output
failed:404
```

```brix
(main
  (let result
    (match true
      (case? true
        ..ok
      )
      (default
        (call ..failed 1)
      )
    )
  )
  (blank)
  (let other (call .failed 2))
  (print (. other name))
  (print
    (match other
      (case (call .failed code)
        code
      )
      (case .ok
        0
      )
    )
  )
)
```

```output
failed
2
```

```brix
(main
  (let result
    (match true
      (case? true
        ..ok
      )
      (default
        ..failed
      )
    )
  )
  (blank)
  (= result
    (match false
      (case? false
        ..failed
      )
      (default
        ..ok
      )
    )
  )
  (blank)
  (print (. result name))
)
```

```output
ok
```

```brix
(prop + submitted (of AsyncChannel Bool))
(blank)

(main
  (lazy login-submission
    (iterate (on submitted)
      (block
        (case? true
          (call ..valid "email" "password")
        )
        (default
          ..invalid-password
        )
      )
    )
  )
  (blank)
  (print (is login-submission .valid))
)
```

```output
false
```

```brix
(main
  (let result (call authenticate "email" "secret"))
  (print (. result name))
  (print
    (match result
      (case (call .authentic email token)
        (str email ":" token)
      )
      (case .incorrect-password
        "bad"
      )
    )
  )
)

(fn-expr + authenticate _
  (param email String)
  (param password String)
  (case? (== password "secret")
    (call ..authentic email "token")
  )
  (default
    ..incorrect-password
  )
)
```

```output
authentic
email:token
```

```brix
(main
  (for (in result (call authenticate "email" "secret"))
    (print (. result name))
    (print
      (match result
        (case (call .authentic email token)
          (str email ":" token)
        )
        (case .incorrect-password
          "bad"
        )
      )
    )
  )
)

(fn-do + authenticate _
  (param email String)
  (param password String)
  (yield
    (block
      (case? (== password "secret")
        (call ..authentic email "token")
      )
      (default
        ..incorrect-password
      )
    )
  )
)
```

```output
authentic
email:token
```

```brix crash
(main
  (print
    (match true
      (case? true
        ..ok
      )
      (default
        ..failed
      )
    )
  )
)
```

```output
Exception: Anonymous match case '..ok' needs a stable result owner
<no file:6>
            ..ok
            ^^^^
```

```brix crash
(main
  (let result
    (match true
      (case? true
        (call ..value 1)
      )
      (default
        (call ..value 1 2)
      )
    )
  )
)
```

```output
Exception: Anonymous case '..value' has conflicting arity
<no file:9>
            (call ..value 1 2)
            ^^^^^^^^^^^^^^^^^^
```

### Anonymous case named again by its symbol, bare and called

```brix
(main
  (let result
    (match true
      (case? true
        ..ok
      )
      (default
        (call ..failed 1)
      )
    )
  )
  (blank)
  (let other .ok)
  (print (. other name))
  (print (. other case))
  (print (is other .ok))
  (print (is result .ok))
  (let again (call .failed 7))
  (print (. again name))
  (print (. again case))
  (print
    (match again
      (case (call .failed code)
        (+ code 1)
      )
      (case .ok
        0
      )
    )
  )
  (print (== other result))
  (print (== again result))
  (print (!= again result))
)
```

```output
ok
0
true
true
failed
1
8
true
false
true
```

### Anonymous cases of a function with several values and an object

```brix
(main
  (let box (call Box 4))
  (print (call describe (call pick 0 box)))
  (print (call describe (call pick 1 box)))
  (print (call describe (call pick 2 box)))
  (print (call describe (call pick 3 box)))
  (print (. (call pick 1 box) name))
  (print (. (call pick 3 box) case))
)

(type + Box
  (prop + x Int)
  (fn-do + This _
    (param x _)
  )
)

(fn-expr + pick _
  (param which Int)
  (param box Box)
  (case? (== which 0)
    ..empty
  )
  (case? (== which 1)
    (call ..triple 1 2.5 "three")
  )
  (case? (== which 2)
    (call ..triple 4 5 "six")
  )
  (default
    (call ..boxed box)
  )
)

(fn-expr + describe String
  (param value _)
  (match value
    (case .empty
      "empty"
    )
    (case (call .triple a b c)
      (str "triple " a " " b " " c)
    )
    (case (call .boxed held)
      (str "boxed " (. held x))
    )
  )
)
```

```output
empty
triple 1 2.5 three
triple 4 5 six
boxed 4
triple
2
```

### Anonymous cases assigned again and by symbol

```brix
(main
  (let count 0)
  (let result
    (match count
      (case? (> count 0)
        (call ..many count)
      )
      (default
        ..none
      )
    )
  )
  (print (. result name))
  (= count 3)
  (= result
    (match count
      (case? (> count 0)
        (call ..many count)
      )
      (default
        ..none
      )
    )
  )
  (print (. result name))
  (print
    (match result
      (case (call .many n)
        (* n 2)
      )
      (case .none
        0
      )
    )
  )
  (= result .none)
  (print (. result name))
  (= result (call .many 9))
  (print
    (match result
      (case (call .many n)
        n
      )
      (case .none
        0
      )
    )
  )
  (print (call step result))
  (= result .none)
  (print (call step result))
)

(fn-do + step String
  (param value _)
  (if (is value .none)
    (return "none")
  )
  (return "many")
)
```

```output
none
many
6
none
9
many
none
```

### Anonymous cases with converted values and matched fields

```brix
(main
  (let shape
    (match 2
      (case? false
        (call ..circle 1.5)
      )
      (default
        (call ..rect 2 3.5)
      )
    )
  )
  (print (call area shape))
  (= shape (call .circle 2))
  (print (call area shape))
  (let label
    (match shape
      (case (call .circle r)
        (call ..round "circle" 2)
      )
      (case (call .rect w h)
        (call ..flat "rect")
      )
    )
  )
  (print (. label name))
  (print
    (match label
      (case (call .round text size)
        (str text " " size)
      )
      (case (call .flat text)
        text
      )
    )
  )
  (let nested
    (match true
      (case? true
        (call ..wrapped 1 (call Box 2))
      )
      (default
        ..bare
      )
    )
  )
  (print
    (match nested
      (case (call .wrapped first box)
        (+ first (. box x))
      )
      (case .bare
        0
      )
    )
  )
)

(type + Box
  (prop + x Int)
  (fn-do + This _
    (param x _)
  )
)

(fn-expr + area Float
  (param shape _)
  (match shape
    (case (call .circle r)
      (* r r 3.0)
    )
    (case (call .rect w h)
      (* w h)
    )
  )
)
```

```output
7
12
round
circle 2
3
```

### Symbol of an anonymous case that two owners have is ambiguous

```brix crash
(main
  (let first
    (match true
      (case? true
        ..ok
      )
      (default
        ..failed
      )
    )
  )
  (blank)
  (let second
    (match true
      (case? true
        ..ok
      )
      (default
        ..other
      )
    )
  )
  (blank)
  (let third .ok)
  (print (. third name))
)
```

```output
Exception: Anonymous case '.ok' is ambiguous between 'first', 'second'
<no file:25>
      (let third .ok)
                 ^^^
```

### Anonymous case with conflicting arity in one owner

```brix crash
(main
  (let result
    (match 1
      (case? false
        (call ..ok 1)
      )
      (default
        (call ..ok 1 2)
      )
    )
  )
  (print (. result name))
)
```

```output
Exception: Anonymous case '..ok' has conflicting arity
<no file:9>
            (call ..ok 1 2)
            ^^^^^^^^^^^^^^^
```

### Anonymous case given a whole tuple converts it to the first field

```brix crash
(main
  (let point (call Point 1 2))
  (let nested
    (match true
      (case? true
        (call ..wrapped point)
      )
      (default
        ..bare
      )
    )
  )
  (print (. nested name))
)

(type + Point (is Tuple)
  (prop + x Int)
  (prop + y Int)
)
```

```output
Exception: Expected I64 here, but this is Point, and there is no conversion between them
<no file:7>
            (call ..wrapped point)
            ^^^^^^^^^^^^^^^^^^^^^^
```

### Anonymous case of a function holds a name a later arm's pattern captures

```brix
(main
  (print (is (call label true) .loaded))
  (print (is (call label false) .failed))
)

(fn-expr + label _
  (param works Bool)
  (match (catch (call risky works))
    (case (: error Error)
      (call ..failed "no")
    )
    (case (: found (list Int))
      (call ..loaded found)
    )
  )
)

(fn-do + risky (list Int) (?! Error)
  (param works Bool)
  (throw (call Error "no") (if (not works)))
  (return (list 1 2))
)
```

```output
true
true
```

### Anonymous case of a function holds its value as known where the case is written

```brix
(main
  (print
    (match (call pick null)
      (case (call .some n)
        (+ n 1)
      )
      (case (call .other n)
        (?? n -1)
      )
    )
  )
)

(fn-do + pick _
  (param value (? Int))
  (if value
    (return (call ..some value))
  )
  (return (call ..other value))
)
```

```output
-1
```
