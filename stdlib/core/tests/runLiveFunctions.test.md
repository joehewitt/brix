# Live functions

### A live function keeps a state across the runs of the value that calls it. Two calls in one value keep a state each, in the order they are called, and a state that runs its action again runs the value again.

```brix
(import /core/tracker)
(blank)
(prop _ last-count (? Count))
(blank)

(main
  (let meter (call Meter))
  (print (str (. meter shown) " | " (. meter both)))
  (= (. meter value) 3)
  (print (str (. meter shown) " | " (. meter both)))
  (call (?. last-count poke))
  (print (str (. meter shown) " | " (. meter both)))
)

(type + Meter
  (prop + value _ (= 1))
  (prop + shown _ (= (call counted #value true)))
  (prop + both _ (= (str (call counted (* #value 10) false) " and " (call counted (* #value 100) false))))
)

(fn-do + counted String @live
  (param goal Int)
  (param remembers Bool)
  (let found (call (of live-state Count)))
  (let count (?? found (call Count)))
  (if (not found)
    (call keep-live-state count)
    (if remembers
      (= last-count count)
    )
  )
  (+= (. count runs) 1)
  (return (str goal " run " (. count runs)))
)

(type + Count
  (implements Tracker)
  (prop + runs _ (= 0))
  (prop + action (? LiveAction))
  (fn-do + poke _
    (if action
      (call action)
    )
  )
  (fn-do + attach _
    (param action LiveAction)
    (= (. this action) action)
  )
  (fn-do + detach _
    (param action LiveAction)
    (= (. this action) null)
  )
)
```

```output
1 run 1 | 10 run 1 and 100 run 1
3 run 2 | 30 run 2 and 300 run 2
3 run 3 | 30 run 2 and 300 run 2
```

### A live function called by the value of a markup attribute keeps its state across the runs of the attribute.

```brix
(import /core/tracker)
(blank)

(main
  (<> Foo (id: a)
    (<> Foo (id: b)
      (= name "B")
    )
    (<> Foo (id: c)
      (= name (call tagged (. b #name)))
    )
  )
  (blank)
  (print (. c name))
  (= (. b name) "BB")
  (print (. c name))
)

(type + Foo
  (prop + name String)
  (prop + contents (list Foo) (= (call (of List Foo))))
  (fn-do + (infix this += (param child Foo)) _
    (+= contents child)
  )
)

(fn-do + tagged String @live
  (param text String)
  (let found (call (of live-state Count)))
  (let count (?? found (call Count)))
  (if (not found)
    (call keep-live-state count)
  )
  (+= (. count runs) 1)
  (return (str text " " (. count runs)))
)

(type + Count
  (implements Tracker)
  (prop + runs _ (= 0))
  (fn-do + attach _
    (param action LiveAction)
  )
  (fn-do + detach _
    (param action LiveAction)
  )
)
```

```output
B 1
BB 2
```

### A live function is called only where a value is read live.

```brix compile-error
(main
  (print (call twice 4))
)

(fn-expr + twice Int @live
  (param value Int)
  (* value 2)
)
```

```output
Exception: The live function "twice" is called only where a value is read live
<no file:3>
      (print (call twice 4))
             ^^^^^^^^^^^^^^
```

### A live function that fails with an error leaves its live scope, so a value read after it is not read live.

```brix
(import /core/tracker)
(blank)

(main
  (let meter (call Meter))
  (print (. meter shown))
  (= (. meter value) 2)
  (print (. meter shown))
  (call probe-outside)
)

(fn-do + probe-outside _
  (let attempt
    (do-new
      (call keep-live-state (call Count))
      (print "leaked")
      (on-failure failure
        (let panic (? Panic) failure)
        (if panic
          (print (str "outside: " panic))
        )
      )
    )
  )
  (call (. attempt wait))
)

(type + Failure (is Error))

(type + Meter
  (prop + value _ (= 1))
  (prop + shown _ (= (str (catch (call risky #value)))))
)

(fn-do + risky String (?! Failure) @live
  (param goal Int)
  (if (== (% goal 2) 1)
    (throw (call Failure (str "odd " goal)))
  )
  (return (str "even " goal))
)

(type + Count
  (implements Tracker)
  (fn-do + attach _
    (param action LiveAction)
  )
  (fn-do + detach _
    (param action LiveAction)
  )
)
```

```output
odd 1
even 2
outside: Only a live function keeps a live state
```

### A live function that panics leaves its live scope before the panic is recovered.

```brix
(import /core/tracker)
(blank)

(main
  (call make)
  (call probe-outside)
)

(fn-do + make _
  (let attempt
    (do-new
      (let meter (call Meter))
      (print (. meter shown))
      (on-failure failure
        (let panic (? Panic) failure)
        (if panic
          (print (str "made: " panic))
        )
      )
    )
  )
  (call (. attempt wait))
)

(fn-do + probe-outside _
  (let attempt
    (do-new
      (call keep-live-state (call Count))
      (print "leaked")
      (on-failure failure
        (let panic (? Panic) failure)
        (if panic
          (print (str "outside: " panic))
        )
      )
    )
  )
  (call (. attempt wait))
)

(type + Meter
  (prop + value _ (= 1))
  (prop + shown _ (= (call risky #value)))
)

(fn-do + risky String @live
  (param goal Int)
  (throw (call Panic (str "odd " goal)) (if (== (% goal 2) 1)))
  (return (str "even " goal))
)

(type + Count
  (implements Tracker)
  (fn-do + attach _
    (param action LiveAction)
  )
  (fn-do + detach _
    (param action LiveAction)
  )
)
```

```output
made: odd 1
outside: Only a live function keeps a live state
```
