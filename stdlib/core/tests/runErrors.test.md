# Errors

```brix crash
(main
  (call read)
)

(fn-do + read _ (?! IOError)
  (throw (call IOError "bad"))
)
```

```stderr
Unhandled error: bad
<no file:7>

Task 1

Stack trace:
  0  __source__:read no file:7:3
  1  __source__:main no file:3:3
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

### GC-free exceptional paths retain normal Error propagation, cleanup, and caught continuation.

```brix
(main
  (print (call gc-free-propagate true))
  (print (catch (call gc-free-propagate false)))
  (print gc-free-cleanups)
  (print (call gc-free-catch false))
  (print (call gc-free-catch true))
  (print (catch (call gc-free-catch-replace false)))
  (print (call gc-free-catch-replace true))
)

(blank)
(prop _ gc-free-cleanups _ (= 0))

(type + GCFreeFailure (is Error))

(type + GCFreeReplacement (is Error))

(fn-do + gc-free-fallible Int (?! GCFreeFailure) (without gc)
  (param succeeds Bool)
  (if succeeds
    (return 7)
  )
  (throw (call GCFreeFailure "failed"))
)

(fn-do + gc-free-propagate Int (?! GCFreeFailure) (without gc)
  (param succeeds Bool)
  (finally
    (+= gc-free-cleanups 1)
  )
  (return (+ (call gc-free-fallible succeeds) 1))
)

(fn-do + gc-free-catch Int (without gc)
  (param succeeds Bool)
  (let value
    (call gc-free-fallible succeeds
      (block
        (on-error GCFreeFailure
          40
        )
      )
    )
  )
  (return (+ value 2))
)

(fn-do + gc-free-catch-replace Int (?! GCFreeReplacement) (without gc)
  (param succeeds Bool)
  (let value
    (call gc-free-fallible succeeds
      (block
        (on-error GCFreeFailure
          (throw (call GCFreeReplacement "replaced"))
        )
      )
    )
  )
  (return value)
)
```

```output
8
failed
2
42
9
replaced
7
```

### A held match join routes bare Errors through a marked Result channel.

```brix
(main
  (print (call inspect-joined false))
  (print (call inspect-joined true))
)

(type + Config
  (prop + value String)
  (fn-do + This _
    (param value _)
  )
)

(type + FileNotFound (is Error))

(fn-expr + select-joined _
  (param fail Bool)
  (match fail
    (case= true
      (call FileNotFound "missing")
    )
    (default
      (call Config "loaded")
    )
  )
)

(fn-do + inspect-joined String
  (param fail Bool)
  (let joined (catch (call select-joined fail)))
  (blank)
  (return
    (match joined
      (case (: config Config)
        (. config value)
      )
      (case FileNotFound
        "file error"
      )
    )
  )
)
```

```output
loaded
file error
```

### An unheld match join auto-propagates its inferred Error channel.

```brix
(main
  (print (?? (catch? (call inspect-joined false)) "propagated"))
  (print (?? (catch? (call inspect-joined true)) "propagated"))
)

(type + Config
  (prop + value String)
  (fn-do + This _
    (param value _)
  )
)

(type + FileNotFound (is Error))

(fn-do + inspect-joined String (?! FileNotFound)
  (param fail Bool)
  (let config
    (match fail
      (case= true
        (call FileNotFound "missing")
      )
      (default
        (call Config "loaded")
      )
    )
  )
  (return (. config value))
)
```

```output
loaded
propagated
```

### Handling every original error with ordinary values produces an ordinary expression type.

```brix
(main
  (let config
    (call read-config 2
      (block
        (on-error FileNotFound
          (call Config "file default")
        )
        (on-error PermissionDenied
          (call Config "permission default")
        )
      )
    )
  )
  (print (type-of config))
  (print (. config value))
)

(type + Config
  (prop + value String)
  (fn-do + This _
    (param value _)
  )
)

(type + FileNotFound (is Error))

(type + PermissionDenied (is Error))

(fn-do + read-config Config (?! FileNotFound) (?! PermissionDenied)
  (param outcome Int)
  (if (== outcome 1)
    (throw (call FileNotFound "missing"))
  )
  (if (== outcome 2)
    (throw (call PermissionDenied "denied"))
  )
  (return (call Config "loaded"))
)
```

```output
[TypeInfo Config]
permission default
```

### A Result-valued arm flattens into the inferred match Result.

```brix
(main
  (print (call inspect-flattened true false))
  (print (call inspect-flattened false true))
  (print (call inspect-flattened false false))
)

(fn-do + read Int (?! IOError)
  (param ok Bool)
  (if ok
    (return 22)
    (else
      (throw (call IOError "disk"))
    )
  )
)

(fn-expr + select-flattened _
  (param outer Bool)
  (param inner Bool)
  (match outer
    (case= true
      11
    )
    (default
      (catch (call read inner))
    )
  )
)

(fn-do + inspect-flattened String
  (param outer Bool)
  (param inner Bool)
  (let joined (catch (call select-flattened outer inner)))
  (blank)
  (return
    (match joined
      (case (: value Int)
        (str value)
      )
      (case IOError
        "io error"
      )
    )
  )
)
```

```output
11
22
io error
```

### Bare-Error-only joins use Void success and need only Error arms when held.

```brix
(main
  (print (call inspect-errors false))
  (print (call inspect-errors true))
)

(type + FileNotFound (is Error))

(type + PermissionDenied (is Error))

(fn-do + select-error _ (?! FileNotFound) (?! PermissionDenied)
  (param denied Bool)
  (match denied
    (case= true
      (call PermissionDenied "denied")
    )
    (default
      (call FileNotFound "missing")
    )
  )
)

(fn-do + inspect-errors String
  (param denied Bool)
  (let joined (catch (call select-error denied)))
  (blank)
  (return
    (match joined
      (case FileNotFound
        "file error"
      )
      (case PermissionDenied
        "permission error"
      )
    )
  )
)
```

```output
file error
permission error
```

### A ?! arm returns immediately and contributes no type to the join.

```brix
(main
  (print (?? (catch? (call inspect-return false)) "returned"))
  (print (?? (catch? (call inspect-return true)) "returned"))
)

(type + Config
  (prop + value String)
  (fn-do + This _
    (param value _)
  )
)

(type + PermissionDenied (is Error))

(fn-do + inspect-return String (?! PermissionDenied)
  (param fail Bool)
  (let selected
    (match fail
      (case= true
        (throw (call PermissionDenied "denied"))
      )
      (default
        (call Config "loaded")
      )
    )
  )
  (print (type-of selected))
  (return (. selected value))
)
```

```output
[TypeInfo Config]
loaded
returned
```

### Error results propagate through collection expressions and relayed generators.

```brix
(main
  (let list-result (catch (call list-values)))
  (print (str "list error " (is list-result StreamProblem)))
  (blank)
  (let stream-result (catch (call stream-values)))
  (print (str "stream error " (is stream-result StreamProblem)))
)

(type + StreamProblem (is Error))

(fn-do _ encode-value Int (?! StreamProblem)
  (param value Int)
  (if (== value 0)
    (return value)
  )
  (throw (call StreamProblem "bad"))
)

(fn-expr _ list-values (list Int) (?! StreamProblem)
  (in (call encode-value each) (list 1))
)

(fn-do _ failing-items (stream Int)
  (yield 1)
  (throw (call StreamProblem "bad"))
)

(fn-do _ relayed-items (stream Int)
  (for (in item (call failing-items))
    (yield item)
  )
)

(fn-do _ stream-values (list Int) (?! StreamProblem)
  (let values (list Int) (call (of List Int)))
  (for (in item (call relayed-items))
    (+= values item)
  )
  (return values)
)
```

```output
list error true
stream error true
```

### A fallible feed pipeline can be collected through a named tuple stream.

```brix
(main
  (let values
    (call (of Map String Float)
      (block
        (:one 1.0)
        (:zero 0.0)
      )
    )
  )
  (let answer
    (call ProbabilityAnswer
      (block
        (arg (call require-probabilities values))
      )
    )
  )
  (blank)
  (print (. answer probabilities length))
  (print ([] (. answer probabilities) "one"))
  (print ([] (. answer probabilities) "zero"))
)

(type + ProbabilityError (is Error))

(type + ProbabilityAnswer (is Tuple)
  (prop + probabilities (map String Float))
)

(fn-expr _ require-probability Float (?! ProbabilityError)
  (param value Float)
  (before
    (throw (call ProbabilityError "invalid") (if (or (< value 0) (> value 1))))
  )
  value
)

(fn-expr _ require-probabilities _ (?! ProbabilityError)
  (param values (map String Float))
  (spread values)
  (pipe *> (tuple ([] each 0) (call require-probability ([] each 1))))
  (pipe |> Map)
)
```

```output
2
1
0
```

### Error results propagate when a collection expression becomes a union's list case.

```brix
(main
  (let success (call union-list-values 1))
  (print (str "union list success " (is success (list I32))))
  (let values (as success (list I32)))
  (print (str "union list length " (. values length)))
  (print (str "union list first " ([] values 0)))
  (blank)
  (let result (catch (call union-list-values 0)))
  (print (str "union list error " (is result UnionListProblem)))
)

(type + UnionListProblem (is Error))

(type + UnionListPayload (is Union)
  (variant number I32)
  (variant values (list I32))
)

(fn-do _ union-list-value I32 (?! UnionListProblem)
  (param value I32)
  (if (== value 0)
    (throw (call UnionListProblem "bad"))
  )
  (return value)
)

(fn-expr _ union-list-values UnionListPayload (?! UnionListProblem)
  (param value I32)
  (in (call union-list-value each) (list value))
)
```

```output
union list success true
union list length 1
union list first 1
union list error true
```

### Returning a held Result fuses its success and Error into the caller-visible Result.

```brix
(main
  (print (catch (call relay true)))
  (print (catch (call relay false)))
)

(fn-do + read Int (?! IOError)
  (param ok Bool)
  (if ok
    (return 7)
    (else
      (throw (call IOError "read failed"))
    )
  )
)

(fn-do + relay _
  (param ok Bool)
  (return (catch (call read ok)))
)
```

```output
7
read failed
```

### Ordinary, direct-Error, and held-Result returns share one outgoing Result.

```brix
(main
  (print (call inspect-relay 0))
  (print (call inspect-relay 1))
  (print (call inspect-relay 2))
)

(type + OuterError (is Error))

(fn-do + read Int (?! IOError)
  (throw (call IOError "inner"))
)

(fn-do + relay _
  (param mode Int)
  (if (== mode 0)
    (return 42)
  )
  (if (== mode 1)
    (throw (call OuterError "outer"))
  )
  (return (catch (call read)))
)

(fn-do + inspect-relay String
  (param mode Int)
  (return
    (match (catch (call relay mode))
      (case (: value Int)
        (str value)
      )
      (case OuterError
        "outer error"
      )
      (case IOError
        "inner error"
      )
    )
  )
)
```

```output
42
outer error
inner error
```

### Deferred cleanup runs exactly once for either case of a returned held Result.

```brix
(prop _ cleanups _ (= 0))
(blank)

(main
  (print (catch (call relay true)))
  (print cleanups)
  (print (catch (call relay false)))
  (print cleanups)
)

(fn-do + read Int (?! IOError)
  (param ok Bool)
  (if ok
    (return 7)
    (else
      (throw (call IOError "read failed"))
    )
  )
)

(fn-do + relay _
  (param ok Bool)
  (finally
    (+= cleanups 1)
  )
  (return (catch (call read ok)))
)
```

```output
7
1
read failed
2
```

### A declared success type validates against the fused Result success channel.

```brix
(main
  (print (catch (call relay true)))
  (print (catch (call relay false)))
)

(fn-do + read Int (?! IOError)
  (param ok Bool)
  (if ok
    (return 7)
    (else
      (throw (call IOError "read failed"))
    )
  )
)

(fn-do + relay Int (?! IOError)
  (param ok Bool)
  (return (catch (call read ok)))
)
```

```output
7
read failed
```

```brix
(main
  (print (catch (call foo 10)))
  (print (catch (call foo 0)))
  (blank)
  (print (type-of (call foo 10)))
  (print (type-of (catch (call foo 10))))
)

(fn-expr + foo _
  (param n _)
  (before
    (throw (call Error "Divide by zero") (if (== n 0)))
  )
  (/ n 2)
)
```

```output
5
Divide by zero
[TypeInfo I64]
[TypeInfo Union]
```

```brix
(main
  (print (catch? (call foo 10)))
  (print (catch? (call foo 0)))
  (blank)
  (print (type-of (call foo 10)))
  (print (type-of (catch? (call foo 10))))
)

(fn-expr + foo _
  (param n _)
  (before
    (throw (call Error "Divide by zero") (if (== n 0)))
  )
  (/ n 2)
)
```

```output
5
unknown
[TypeInfo I64]
[TypeInfo Optional'I64]
```

```brix
(main
  (catch? (call foo 0))
  (catch? (call foo 10))
)

(fn-do + foo _
  (param n _)
  (throw (call Error "Divide by zero") (if (== n 0)))
  (blank)
  (print (/ n 2))
)
```

```output
5
```

```brix
(prop _ hits _ (= 0))
(blank)

(main
  (catch (call bump))
  (print hits)
)

(fn-do + bump _
  (+= hits 1)
)
```

```output
1
```

```brix
(prop _ hits _ (= 0))
(blank)

(main
  (catch (call bump))
  (print hits)
)

(fn-do + bump _ (?! IOError)
  (+= hits 1)
)
```

```output
1
```

```brix
(main
  (print
    (block
      (case? (catch? (call foo 0))
        1
      )
      (case? (catch? (call foo 10))
        2
      )
      (default
        3
      )
    )
  )
)

(fn-expr + foo _
  (param n _)
  (before
    (throw (call Error "Divide by zero") (if (== n 0)))
  )
  (/ n 2)
)
```

```output
2
```

```brix
(main
  (print (?? (catch? (call foo 1)) -1))
  (print (?? (catch? (call foo 0)) -1))
)

(fn-do + foo _
  (param n _)
  (if (> n 0)
    (return 42)
    (else
      (throw (call Error "Bad news"))
    )
  )
)
```

```output
42
-1
```

```brix
(main
  (print (catch? (call set-mode 1 false)))
  (print (catch? (call set-mode 0 true)))
)

(fn-expr + set-mode _
  (param code _)
  (param is-on _)
  (before
    (throw (call Error "bad") (if (== code 0)))
  )
  is-on
)
```

```output
false
unknown
```

```brix
(main
  (do-new
    (let foo Foo (call Bar))
    (print (as foo Baz))
    (on-failure failure
      (print failure)
    )
  )
)

(blank)
(type-line + Foo)
(type-line + Bar (is Foo))
(type-line + Baz (is Bar))
(type-line + Other (is Foo))
```

```output
Invalid cast to [Object __source__:Baz]
```

```brix crash
(main
  (let thing (call Thing))
  (let p (as thing Proto))
  (print (call (. p foo)))
)

(type + Proto (is Interface)
  (fn-expr + foo Int)
)

(type + Thing
  (implements Proto)
  (fn-expr + foo _
    (throw (call Error "fooey"))
  )
)
```

```output
Exception: "foo" can fail with Error, but the interface method it implements cannot; add "(?! Error)" to that method, or catch the error inside "foo"
<no file:14>
      (fn-expr + foo _
      ^^^^^^^^^^^^^^^
Interface:
<no file:9>
      (fn-expr + foo Int)
```

```brix
(main
  (print (catch (call read true)))
  (print (catch (call read false)))
  (print (type-of (call read true)))
  (print (type-of (catch (call read true))))
)

(fn-do + read Int (?! IOError)
  (param ok Bool)
  (if ok
    (return 7)
    (else
      (throw (call IOError "read failed"))
    )
  )
)
```

```output
7
read failed
[TypeInfo I64]
[TypeInfo Union]
```

```brix
(main
  (print (call caller true))
  (print (catch (call caller false)))
)

(fn-do + read Int (?! IOError)
  (param ok Bool)
  (if ok
    (return 40)
    (else
      (throw (call IOError "disk missing"))
    )
  )
)

(fn-expr + caller Int (?! IOError)
  (param ok Bool)
  (before
    (finally
      (print "cleanup")
    )
  )
  (+ (call read ok) 2)
)
```

```output
cleanup
42
cleanup
disk missing
```

```brix
(main
  (let result (catch (call load)))
  (let app (as result AppError))
  (print app)
  (print (. app previous))
)

(type + AppError (is Error))

(fn-do + read Int (?! IOError)
  (throw (call IOError "disk"))
)

(fn-do + load Int (?! AppError)
  (throw (call AppError "load failed") (on-error (: err IOError)))
  (blank)
  (return (call read))
)
```

```output
load failed
disk
```

```brix
(main
  (let result (catch (call load)))
  (let app (as result AppError))
  (let config (as (!! (. app previous)) ConfigError))
  (print app)
  (print config)
  (print (. config previous))
)

(type + ConfigError (is Error))

(type + AppError (is Error))

(fn-do + read Int (?! IOError)
  (throw (call IOError "disk"))
)

(fn-do + parse Int (?! ConfigError)
  (throw (call ConfigError "config") (on-error (: err IOError)))
  (blank)
  (return (call read))
)

(fn-do + load Int (?! AppError)
  (throw (call AppError "from io") (on-error (caught (: err ConfigError) (: io IOError))))
  (blank)
  (throw (call AppError "from config") (on-error (: err ConfigError)))
  (blank)
  (return (call parse))
)
```

```output
from io
config
disk
```

```brix crash
(main
  (throw 42)
)
```

```output
Exception: I64 cannot be returned with ?!; return an Error subtype or use ?! Panic(...) to end the task.
<no file:3>
      (throw 42)
      ^^^^^^^^^^
```

```brix crash
(main
  (call foo)
)

(fn-expr + foo Int (?! Int)
  1
)
```

```output
Exception: I64 is not a recoverable error type; only subclasses of Error may appear after ?!.
<no file:6>
    (fn-expr + foo Int (?! Int)
    ^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix crash
(main
  (call foo)
)

(fn-expr + foo Int (?! Panic)
  1
)
```

```output
Exception: Panic cannot be returned as a recoverable error; use ?! Panic(...) to end the task, or convert it explicitly.
<no file:6>
    (fn-expr + foo Int (?! Panic)
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix crash
(main
  (call foo)
)

(fn-do + foo Int (?! TypeError)
  (throw (call IOError "disk"))
)
```

```output
Exception: IOError can leave "foo", which does not declare it; declare it, as in "(fn-do + foo Int (?! TypeError) (?! IOError) …)"
<no file:7>
      (throw (call IOError "disk"))
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

```brix crash
(main
  (call foo)
)

(type + AppError (is Error))

(fn-do + read String (?! IOError)
  (throw (call IOError "disk"))
)

(fn-do + foo Int (?! AppError)
  (throw (call AppError (call read)) (on-error (: err IOError)))
  (blank)
  (throw (call IOError "source"))
)
```

```output
Exception: Error transformer expressions must not call result-returning functions
<no file:13>
      (throw (call AppError (call read)) (on-error (: err IOError)))
                            ^^^^^^^^^^^
```

```brix crash
(main
  (call foo)
)

(type + AppError (is Error))

(fn-do + foo Int (?! AppError)
  (throw (call AppError "one") (on-error (: err IOError)))
  (blank)
  (throw (call AppError "two") (on-error (: other IOError)))
  (blank)
  (throw (call IOError "source"))
)
```

```output
Exception: Ambiguous error transformer for IOError; only one transformer may match the same error chain.
<no file:11>
      (throw (call AppError "two") (on-error (: other IOError)))
                                   ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A transformer names the error it replaces: a name alone, or a type that is no error, is an error.

```brix compile-error
(main
  (print (call f))
)

(type + AppError (is Error))

(fn-do + f Int (?! AppError)
  (throw (call AppError "one") (on-error err))
  (return 1)
)
```

```output
Exception: Error transformer catch requires an error type
<no file:9>
      (throw (call AppError "one") (on-error err))
                                   ^^^^^^^^^^^^^^
```

### Inferred errors cannot retroactively change the ABI of a recursive function.

```brix crash
(main
  (print (call recurse 3))
)

(fn-do + recurse Bool
  (param n I64)
  (throw (call IOError "disk") (if (== n 99)))
  (blank)
  (if (> n 0)
    (return (call recurse (- n 1)))
  )
  (return true)
)
```

```output
Exception: IOError can leave "recurse", which calls itself, so its errors cannot be inferred; declare them on it, as in "(fn-do + recurse Bool (?! IOError) …)", or catch them here
<no file:8>
      (throw (call IOError "disk") (if (== n 99)))
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Inferred errors cannot retroactively change the ABI of a recursive Void function either.

```brix crash
(main
  (print (str "result=" (?? (catch? (call walk 99)) false)))
)

(fn-do + walk Bool
  (param n I64)
  (call visit n)
  (return true)
)

(fn-do + visit _
  (param n I64)
  (throw (call IOError "disk") (if (== n 99)))
  (if (> n 0)
    (call visit (- n 1))
  )
)
```

```output
Exception: IOError can leave "visit", which calls itself, so its errors cannot be inferred; declare them on it, as in "(fn-do + visit _ (?! IOError) …)", or catch them here
<no file:14>
      (throw (call IOError "disk") (if (== n 99)))
      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A recursive Void function with declared errors propagates them to its callers.

```brix
(main
  (print (str "direct=" (?? (catch? (call walk 99)) false)))
  (print (str "nested=" (?? (catch? (call walk 100)) false)))
  (print (str "clean=" (?? (catch? (call walk 3)) false)))
)

(fn-do + walk Bool (?! IOError)
  (param n I64)
  (call visit n)
  (return true)
)

(fn-do + visit _ (?! IOError)
  (param n I64)
  (throw (call IOError "disk") (if (== n 99)))
  (if (> n 0)
    (call visit (- n 1))
  )
)
```

```output
direct=false
nested=false
clean=true
```

### A recursive function with declared errors returns correct values through the result ABI.

```brix
(main
  (print (str "result=" (?? (catch? (call recurse 3)) false)))
  (print (str "raised=" (?? (catch? (call recurse 99)) false)))
)

(fn-do + recurse Bool (?! IOError)
  (param n I64)
  (throw (call IOError "disk") (if (== n 99)))
  (blank)
  (if (> n 0)
    (return (call recurse (- n 1)))
  )
  (return true)
)
```

```output
result=true
raised=false
```

### Recursive local grammars tell authors to declare errors on the enclosing function.

```brix crash
(main
  (print (?? (catch? (call parse "a!")) "error"))
)

(type + ParserError (is Error))

(fn-expr + parse String
  (param text String)
  (match text
    (~ \items)
    (throw (call ParserError "invalid"))
    (blank)
    (lazy items String
      (pattern
        (~ (seq (: head \item) (: tail (? \items)))
          (++ head (?? tail ""))
        )
        (~ ""
          ""
        )
      )
    )
    (blank)
    (lazy item String
      (pattern
        (~ "a"
          "a"
        )
        (~ "!"
          (call bad-item)
        )
      )
    )
  )
)

(fn-do + bad-item String (?! ParserError)
  (throw (call ParserError "bang"))
)
```

```output
Exception: ParserError can leave recursive function items before its errors are inferred; for a local "(lazy name Type (pattern …))" grammar, declare "(?! ParserError)" on the enclosing function so generated pattern helpers inherit the result ABI.
<no file:16>
            (~ (seq (: head \item) (: tail (? \items)))
                            ^^^^^
```

### Recursive local grammars inherit declared result errors from the enclosing function.

```brix
(main
  (print (?? (catch? (call parse "aaa")) "error"))
  (print (?? (catch? (call parse "a!")) "error"))
)

(type + ParserError (is Error))

(fn-expr + parse String (?! ParserError)
  (param text String)
  (match text
    (~ \items)
    (throw (call ParserError "invalid"))
    (blank)
    (lazy items String
      (pattern
        (~ (seq (: head \item) (: tail (? \items)))
          (++ head (?? tail ""))
        )
        (~ ""
          ""
        )
      )
    )
    (blank)
    (lazy item String
      (pattern
        (~ "a"
          "a"
        )
        (~ "!"
          (call bad-item)
        )
      )
    )
  )
)

(fn-do + bad-item String (?! ParserError)
  (throw (call ParserError "bang"))
)
```

```output
aaa
error
```

### Acyclic local stream helpers preserve the enclosing function's declared result-error ABI.

```brix
(main
  (print (?? (catch? (call parse-pair "ok")) "error"))
  (print (?? (catch? (call parse-pair "o!")) "error"))
)

(type + PairError (is Error))

(fn-expr + parse-pair String (?! PairError)
  (param text String)
  (match text
    (~ (seq (: left \prefix) (: right \suffix))
      (++ left right)
    )
    (throw (call PairError "invalid"))
    (blank)
    (lazy prefix String
      (pattern
        (~ "o")
      )
    )
    (blank)
    (lazy suffix String
      (pattern
        (~ "k")
        (~ "!"
          (call bad-suffix)
        )
      )
    )
  )
)

(fn-do + bad-suffix String (?! PairError)
  (throw (call PairError "bad suffix"))
)
```

```output
ok
error
```

### Attached handlers recover selected errors and propagate unhandled original errors.

```brix
(main
  (print (call inspect-attached 0))
  (print (call inspect-attached 1))
  (print (?? (catch? (call inspect-attached 2)) "propagated"))
)

(type + Config
  (prop + value String)
  (fn-do + This _
    (param value _)
  )
)

(type + FileNotFound (is Error))

(type + PermissionDenied (is Error))

(fn-do + read-config Config (?! FileNotFound) (?! PermissionDenied)
  (param outcome Int)
  (if (== outcome 1)
    (throw (call FileNotFound "missing"))
  )
  (if (== outcome 2)
    (throw (call PermissionDenied "denied"))
  )
  (return (call Config "loaded"))
)

(fn-do + inspect-attached String (?! PermissionDenied)
  (param outcome Int)
  (let config
    (call read-config outcome
      (block
        (on-error FileNotFound
          (call Config "default")
        )
      )
    )
  )
  (return (. config value))
)
```

```output
loaded
default
propagated
```

### An error-bearing module combinator preserves success and propagated error results while sharing the active frame through its static pattern argument.

```brix
(main
  (print (?? (catch? (call parse-combinator "ok?")) "error"))
  (print (?? (catch? (call parse-combinator "ok!")) "error"))
)

(type + CombinatorError (is Error))

(fn-expr + (of guarded-pattern Result) _ (over: Char) (?! CombinatorError)
  (param pattern (of PatternFunc Char Result))
  (~ (seq (: value $pattern) "?")
    value
  )
  (~ (seq $pattern "!")
    (call fail-combinator)
  )
)

(fn-expr + parse-combinator String (?! CombinatorError)
  (param text String)
  (match text
    (~ (call \guarded-pattern (~ (+ (chars "a-z")))))
    (throw (call CombinatorError "invalid"))
  )
)

(fn-do + fail-combinator String (?! CombinatorError)
  (throw (call CombinatorError "guarded failure"))
)
```

```output
ok
error
```

### A direct static module rule can use the cursor/result ABI with declared errors.

```brix
(main
  (print (?? (catch? (call parse-module-token "ok")) "error"))
  (print (?? (catch? (call parse-module-token "!")) "error"))
)

(type + ModuleTokenError (is Error))

(fn-expr + module-token _ (over: Char) (?! ModuleTokenError)
  (~ "ok")
  (~ "!"
    (call bad-module-token)
  )
)

(fn-expr + parse-module-token String (?! ModuleTokenError)
  (param text String)
  (match text
    (~ \module-token)
    (throw (call ModuleTokenError "invalid"))
  )
)

(fn-do + bad-module-token String (?! ModuleTokenError)
  (throw (call ModuleTokenError "bad module token"))
)
```

```output
ok
error
```

### Prefix holding disables automatic propagation and requires every original Error case.

```brix crash
(main
  (let config
    (scope (catch (call read-config))
      (on-error FileNotFound
        (call Config "default")
      )
    )
  )
  (print (. config value))
)

(type + Config
  (prop + value String)
  (fn-do + This _
    (param value _)
  )
)

(type + FileNotFound (is Error))

(type + PermissionDenied (is Error))

(fn-do + read-config Config (?! FileNotFound) (?! PermissionDenied)
  (throw (call PermissionDenied "denied"))
)
```

```output
Exception: This catch does not handle PermissionDenied; add a handler for it, or one for Error
<no file:4>
        (scope (catch (call read-config))
               ^^^^^^^^^^^^^^^^^^^^^^^^^
```

### A handler-produced bare Error stays in the expression, and an explicit return fuses it later.

```brix
(main
  (print (catch (call relay-attached 0)))
  (print (catch (call relay-attached 1)))
  (print (catch (call relay-attached 2)))
  (print (catch (call relay-held)))
)

(type + ConvertedError (is Error))

(type + FileNotFound (is Error))

(type + PermissionDenied (is Error))

(fn-do + read-text String (?! FileNotFound) (?! PermissionDenied)
  (param outcome Int)
  (if (== outcome 1)
    (throw (call FileNotFound "missing"))
  )
  (if (== outcome 2)
    (throw (call PermissionDenied "denied"))
  )
  (return "loaded")
)

(fn-do + relay-attached _
  (param outcome Int)
  (let handled
    (call read-text outcome
      (block
        (on-error (: err FileNotFound)
          (call ConvertedError (str "converted " (. err message)))
        )
        (on-error PermissionDenied
          "permission default"
        )
      )
    )
  )
  (print "after handler")
  (return handled)
)

(fn-do + relay-held String (?! FileNotFound)
  (let handled
    (scope (catch (call read-text 1))
      (on-error (: err FileNotFound)
        (throw err)
      )
      (on-error PermissionDenied
        "permission default"
      )
    )
  )
  (return handled)
)
```

```output
after handler
loaded
after handler
converted missing
after handler
permission default
missing
```

### A side-effecting head evaluates exactly once on success and on each Error path.

```brix
(prop _ attached-head-count _ (= 0))
(blank)

(main
  (print (call inspect-once 0))
  (print (call inspect-once 1))
  (print (call inspect-once 2))
  (print attached-head-count)
)

(type + Config
  (prop + value String)
  (fn-do + This _
    (param value _)
  )
)

(type + FileNotFound (is Error))

(type + PermissionDenied (is Error))

(fn-do + counted-read Config (?! FileNotFound) (?! PermissionDenied)
  (param outcome Int)
  (+= attached-head-count 1)
  (print (str "head " attached-head-count))
  (if (== outcome 1)
    (throw (call FileNotFound "missing"))
  )
  (if (== outcome 2)
    (throw (call PermissionDenied "denied"))
  )
  (return (call Config "loaded"))
)

(fn-do + inspect-once String
  (param outcome Int)
  (let config
    (call counted-read outcome
      (block
        (on-error FileNotFound
          (call Config "file default")
        )
        (on-error PermissionDenied
          (call Config "permission default")
        )
      )
    )
  )
  (return (. config value))
)
```

```output
head 1
loaded
head 2
file default
head 3
permission default
3
```

### Attached handlers pass a successful Void Result through without reading a Void value.

```brix
(main
  (call exercise-void-handler)
  (print "after")
)

(fn-do + void-result _ (?! IOError))

(fn-do + exercise-void-handler _
  (call void-result
    (block
      (on-error IOError
        (throw (call Panic "converted"))
      )
    )
  )
)
```

```output
after
```

### Attached-handler inference resolves implicit instance properties inside the Result head.

```brix
(main
  (print (. (call (. (call Holder 2) handled)) value))
)

(type + Config
  (prop + value String)
  (fn-do + This _
    (param value _)
  )
)

(type + FileNotFound (is Error))

(fn-do + read-holder-config Config (?! FileNotFound)
  (param outcome Int)
  (if (== outcome 1)
    (throw (call FileNotFound "missing"))
  )
  (return (call Config (str "loaded " outcome)))
)

(type + Holder
  (prop + outcome Int)
  (fn-do + This _
    (param outcome _)
  )
  (fn-do + handled Config
    (let handled
      (call read-holder-config outcome
        (block
          (on-error FileNotFound
            (call Config "default")
          )
        )
      )
    )
    (return handled)
  )
)
```

```output
loaded 2
```

### Constructor Result declarations participate in attached-handler inference and lowering.

```brix
(main
  (print (type-of (call Fallible false)))
  (print (type-of (catch (call Fallible false))))
  (print (call inspect-held-constructor true))
  (print (call inspect-constructor false))
  (print (call inspect-constructor true))
)

(type + Fallible
  (prop + label String)
  (fn-do + This _
    (= label "recovered")
  )
  (fn-do + This _ (?! IOError)
    (param fail Bool)
    (if fail
      (throw (call IOError "failed"))
    )
    (= label "loaded")
  )
)

(fn-do + inspect-constructor String
  (param fail Bool)
  (let value
    (call Fallible fail
      (block
        (on-error IOError
          (call Fallible)
        )
      )
    )
  )
  (return (. value label))
)

(fn-do + inspect-held-constructor String
  (param fail Bool)
  (let held (catch (call Fallible fail)))
  (return
    (match held
      (case (: value Fallible)
        (. value label)
      )
      (case IOError
        "held recovered"
      )
    )
  )
)
```

```output
[TypeInfo Fallible]
[TypeInfo Union]
held recovered
loaded
recovered
```

### Overlapping handlers use source order, while a narrow handler retains a broad residual.

```brix
(main
  (print (call inspect-overlap true))
  (print (call inspect-overlap false))
  (print (call inspect-residual true))
  (print (?? (catch? (call inspect-residual false)) "propagated"))
)

(type + ParentError (is Error))

(type + ChildError (is ParentError))

(fn-do + fail-parent String (?! ParentError)
  (param child Bool)
  (if child
    (throw (call ChildError "child"))
  )
  (throw (call ParentError "parent"))
)

(fn-do + inspect-overlap String
  (param child Bool)
  (let value
    (call fail-parent child
      (block
        (on-error ChildError
          "child first"
        )
        (on-error ParentError
          "parent second"
        )
      )
    )
  )
  (return value)
)

(fn-do + inspect-residual String (?! ParentError)
  (param child Bool)
  (let value
    (call fail-parent child
      (block
        (on-error ChildError
          "child only"
        )
      )
    )
  )
  (return value)
)
```

```output
child first
parent second
child only
propagated
```

### Attached handler bodies may explicitly propagate replacement errors.

```brix
(main
  (print (?? (catch? (call replace-error)) "replacement propagated"))
)

(type + SourceError (is Error))

(type + ReplacementError (is Error))

(fn-do + fail-source String (?! SourceError)
  (throw (call SourceError "source"))
)

(fn-do + replace-error String (?! ReplacementError)
  (let value
    (call fail-source
      (block
        (on-error SourceError
          (throw (call ReplacementError "replacement"))
        )
      )
    )
  )
  (return value)
)
```

```output
replacement propagated
```

### An optional error becomes text by the converter of its object's class, which may make text of its own.

```brix
(main
  (let made (? Error) (call AppError "x"))
  (let none (? Error) (. (!! made) previous))
  (print made)
  (print (str none))
)

(type + AppError (is Error)
  (fn-to-expr + String
    (str "AppError: " message)
  )
)
```

```output
AppError: x
unknown
```

### Prefix catch over a value that raises nothing gives the value, as it does over a call that cannot fail.

```brix
(main
  (let x 3)
  (print (catch 5))
  (print (catch? 5))
  (print (catch (+ x 1)))
  (print (catch? (+ x 1)))
)
```

```output
5
5
4
4
```

### Attached handlers reject infallible heads with a direct Result diagnostic.

```brix crash
(main
  (let value
    (scope 42
      (on-error IOError
        0
      )
    )
  )
  (print value)
)
```

```output
Exception: "42" cannot fail, so the catch attached to it has nothing to handle; remove the catch, or attach it to a call that can fail
<no file:4>
        (scope 42
               ^
```

### Attached handlers hold only their head call; fallible arguments unwrap and propagate.

```brix
(main
  (print (call rejects 3))
  (print (call rejects 20))
  (let escaped (catch? (call rejects -1)))
  (print (== escaped null))
)

(type + Box
  (prop + value Int)
  (fn-do + This _
    (param value _)
  )
)

(fn-do + make Box (?! Error)
  (param value Int)
  (if (< value 0)
    (throw (call Error "negative"))
  )
  (return (call Box value))
)

(fn-do + consume _ (?! Error)
  (param box Box)
  (param label String)
  (if (> (. box value) 10)
    (throw (call Error "large"))
  )
  (print (str label " " (. box value)))
)

(fn-do + rejects Bool (?! Error)
  (param value Int)
  (call consume (call make value) "consumed"
    (block
      (on-error Error
        (return true)
      )
    )
  )
  (return false)
)
```

```output
consumed 3
false
true
true
```

### A call with a missing argument reports the missing function even when a catch handler is attached.

```brix crash
(main
  (let probe (call Probe))
  (call (. probe run)
    (block
      (on-error (: err Error)
        (throw (call Panic (as err String)))
      )
    )
  )
)

(type + Probe
  (fn-do + run _ (?! Error)
    (param value Int)
    (print value)
  )
)
```

```output
Exception: No method "run" of Probe accepts (); it is declared as "run(value Int)"
<no file:4>
      (call (. probe run)
      ^^^^^^^^^^^^^^^^^^
```

### A raise inside a loop branch of a fallible method without a result propagates to the caller.

```brix
(main
  (let probe (call Probe))
  (call (. probe run) (list "a" "b" "a")
    (block
      (on-error (: err Error)
        (print (str "caught " err))
      )
    )
  )
  (print (. probe count))
)

(type + Probe
  (prop + count _ (= 0))
  (fn-do + run _ (?! Error)
    (param kinds (list String))
    (for (in kind kinds)
      (if (== kind "a")
        (+= count 1)
        (else
          (throw (call Error (str "unknown " kind)))
        )
      )
    )
  )
)
```

```output
caught unknown b
1
```

### An attached handler may call a function without a result when the guarded call has no result.

```brix
(main
  (let probe (call Probe))
  (call (. probe run) true)
  (print (. probe note))
  (call (. probe run) false)
  (print (. probe note))
)

(type + Probe
  (prop + note _ (= "none"))
  (fn-do + run _
    (param fails Bool)
    (call work fails
      (block
        (on-error (: err Error)
          (call record (str err))
        )
      )
    )
  )
  (fn-do + record _
    (param message String)
    (= note message)
  )
  (fn-do + work _ (?! Error)
    (param fails Bool)
    (throw (call Error "broken") (if fails))
    (= note "worked")
  )
)
```

```output
broken
worked
```

### A handler that gives nothing may take the error of a call through a type parameter bound to a held Result with no value: returning the held Result raises its error, and the call has no value.

```brix
(main
  (call relay (catch (call run true)))
  (print "passed")
  (call relay (catch (call run false))
    (block
      (on-error (: failure IOError)
        (do
          (inline (print (str "raised " (. failure message))))
        )
      )
    )
  )
)

(fn-do + run _ (?! IOError)
  (param ok Bool)
  (throw (call IOError "run failed") (if (not ok)))
)

(fn-do + (of relay Value) Value
  (param held Value)
  (return held)
)
```

```output
passed
raised run failed
```

### A raise after an early return in a fallible method without a result propagates to the caller.

```brix
(main
  (let probe (call Probe))
  (call (. probe run) false (list "a" "b" "a")
    (block
      (on-error (: err Error)
        (print (str "caught " err))
      )
    )
  )
  (print (. probe count))
)

(type + Probe
  (prop + count _ (= 0))
  (fn-do + run _ (?! Error)
    (param skip Bool)
    (param kinds (list String))
    (return-if (if skip))
    (for (in kind kinds)
      (if (== kind "a")
        (+= count 1)
        (else
          (throw (call Error (str "unknown " kind)))
        )
      )
    )
  )
)
```

```output
caught unknown b
1
```

### A raise inside a loop branch still propagates when a finally block is pending.

```brix
(main
  (let probe (call Probe))
  (call (. probe run) (list "a" "b" "a")
    (block
      (on-error (: err Error)
        (print (str "caught " err))
      )
    )
  )
  (print (. probe count))
  (print (. probe finished))
)

(type + Probe
  (prop + count _ (= 0))
  (prop + finished _ (= 0))
  (fn-do + run _ (?! Error)
    (param kinds (list String))
    (finally
      (inline (+= finished 1))
    )
    (for (in kind kinds)
      (if (== kind "a")
        (+= count 1)
        (else
          (throw (call Error (str "unknown " kind)))
        )
      )
    )
  )
)
```

```output
caught unknown b
1
1
```

### An error raised while evaluating a constructor argument propagates instead of constructing the object.

```brix
(main
  (print (call describe true))
  (print (call describe false))
)

(type + Holder
  (prop + items (list String) (= (call (of List String))))
  (fn-do + This _)
  (fn-do + This _
    (param items _)
  )
)

(fn-do + make (list String) (?! Error)
  (param fails Bool)
  (throw (call Error "broken") (if fails))
  (let result (list String))
  (+= result "made")
  (return result)
)

(fn-do + build Holder (?! Error)
  (param fails Bool)
  (let holder (call Holder (call make fails)))
  (return holder)
)

(fn-do + describe String
  (param fails Bool)
  (let holder
    (call build fails
      (block
        (on-error (: err Error)
          (return (str "caught " err))
        )
      )
    )
  )
  (return (str "built " (. holder items length)))
)
```

```output
caught broken
built 1
```

### A raise inside a loop over a fallible iterator propagates through a recursive fallible method.

```brix
(main
  (let probe (call Probe))
  (call (. probe run) (list "a" "b" "a") true
    (block
      (on-error (: err Error)
        (print (str "caught " err))
      )
    )
  )
  (print (. probe count))
)

(type + Probe
  (prop + count _ (= 0))
  (fn-do + run _ (?! Error)
    (param kinds (list String))
    (param nested Bool)
    (for (in kind (call checked kinds))
      (if (== kind "a")
        (+= count 1)
        (else-if (and (== kind "nested") nested)
          (call run kinds false)
        )
        (else
          (throw (call Error (str "unknown " kind)))
        )
      )
    )
  )
  (fn-do + checked (list String) (?! Error)
    (param kinds (list String))
    (throw (call Error "empty") (if (not (. kinds length))))
    (return kinds)
  )
)
```

```output
caught unknown b
1
```

```brix
(main
  (print (call describe "Green"))
  (print (call describe "Purple"))
)

(type + Shade (is Union)
  (variant red)
  (variant green)
)

(fn-expr + shade-named _ (?! Error)
  (param name String)
  (match name
    (case= "Red"
      (. Shade red)
    )
    (case= "Green"
      (. Shade green)
    )
    (throw (call Error (str "Unknown shade '" name "'")))
  )
)

(fn-do + describe String
  (param name String)
  (let shade
    (call shade-named name
      (block
        (on-error (: err Error)
          (return "unknown")
        )
      )
    )
  )
  (return (if-expr (== shade (. Shade green)) "green" "red"))
)
```

```output
green
unknown
```

### A caught optional result matches its value arm only when a value is present.

```brix
(main
  (print (call spot-text true))
  (print (call spot-text false))
  (print (call spot-text-failing))
)

(type + Spot (is Tuple)
  (prop + number F64)
  (prop + count F64)
)

(fn-do + spot-found (? Spot) (?! Error)
  (param present Bool)
  (param failing _ (= false))
  (throw (call Error "lost") (if failing))
  (return-if (call Spot 2 5) (if present))
  (return null)
)

(fn-do + spot-text String
  (param present Bool)
  (let result (catch (call spot-found present)))
  (match? result
    (case (: error Error)
      (return "error")
    )
    (case (: spot Spot)
      (return (str "spot " (. spot number) " of " (. spot count)))
    )
  )
  (return "none")
)

(fn-do + spot-text-failing String
  (let result (catch (call spot-found true true)))
  (match? result
    (case (: error Error)
      (return (str "error " (. error message)))
    )
    (case (: spot Spot)
      (return (str "spot " (. spot number)))
    )
  )
  (return "none")
)
```

```output
spot 2 of 5
none
error lost
```

### A caught optional result from an interface method matches its value arm only when present.

```brix
(main
  (print (call located (call Locator true)))
  (print (call located (call Locator false)))
  (print (call located (call Fallback)))
)

(type + Place (is Tuple)
  (prop + number F64)
  (prop + count F64)
)

(type + PlaceError (is Error))

(type + Finder (is Interface)
  (fn-do + place (? Place) (?! PlaceError)
    (param wanted Int)
    (return null)
  )
)

(type + Locator
  (implements Finder)
  (prop + present Bool)
  (fn-do + This _
    (param present _)
  )
  (fn-do + place (? Place) (?! PlaceError)
    (param wanted Int)
    (return-if null (if (not present)))
    (return (call Place (as wanted F64) 5))
  )
)

(type + Fallback
  (implements Finder)
)

(fn-do + located String
  (param finder Finder)
  (let result (catch (call (. finder place) 3)))
  (match? result
    (case (: error PlaceError)
      (return "error")
    )
    (case (: place Place)
      (return (str "place " (. place number) " of " (. place count)))
    )
  )
  (return "none")
)
```

```output
place 3 of 5
none
none
```

### A string literal converts in an override whose base raises no errors

```brix
(main
  (let base Base (call Child))
  (print (. (call (. base code)) width))
)

(type + Code (is Tuple)
  (prop + width _ (= 0))
  (prop + height _ (= 0))
  (fn-from-do + (: text String)
    (throw (call SyntaxError "Invalid code") (if (!= (. text length) 3)))
    (return (call Code (. text length) 1))
  )
)

(type + Base
  (fn-expr + code Code
    (call Code 0 0)
  )
)

(type + Child (is Base)
  (fn-expr + code Code
    "abc"
  )
)
```

```output
3
```

### An explicit conversion of a string literal raises no errors

```brix
(main
  (let base Base (call Child))
  (print (. (call (. base code)) height))
)

(type + Code (is Tuple)
  (prop + width _ (= 0))
  (prop + height _ (= 0))
  (fn-from-do + (: text String)
    (throw (call SyntaxError "Invalid code") (if (!= (. text length) 3)))
    (return (call Code (. text length) 1))
  )
)

(type + Base
  (fn-expr + code Code
    (call Code 0 0)
  )
)

(type + Child (is Base)
  (fn-expr + code Code
    (call Code "xyz")
  )
)
```

```output
1
```

### A string literal that fails its conversion panics

```brix
(main
  (do-new
    (print (. (call broken) width))
    (on-failure failure
      (print failure)
    )
  )
)

(type + Code (is Tuple)
  (prop + width _ (= 0))
  (prop + height _ (= 0))
  (fn-from-do + (: text String)
    (throw (call SyntaxError "Invalid code") (if (!= (. text length) 3)))
    (return (call Code (. text length) 1))
  )
)

(fn-expr + broken Code
  "toolong"
)
```

```output
Invalid code
```

### A runtime string that fails its conversion raises the conversion error

```brix
(main
  (let text "toolong")
  (print (call described text))
)

(type + Code (is Tuple)
  (prop + width _ (= 0))
  (prop + height _ (= 0))
  (fn-from-do + (: text String)
    (throw (call SyntaxError "Invalid code") (if (!= (. text length) 3)))
    (return (call Code (. text length) 1))
  )
)

(fn-do + parse Code
  (param text String)
  (return text)
)

(fn-do + described String
  (param text String)
  (let result (catch (call parse text)))
  (match? result
    (case (: error SyntaxError)
      (return (str "error " (. error message)))
    )
    (case (: code Code)
      (return (str "code " (. code width)))
    )
  )
  (return "none")
)
```

```output
error Invalid code
```

### A conditional coalesce converts a string literal branch to the other branch's type

```brix
(main
  (let base Base (call Child))
  (print (. (call (. base code)) width))
)

(type + Code (is Tuple)
  (prop + width _ (= 0))
  (prop + height _ (= 0))
  (fn-from-do + (: text String)
    (throw (call SyntaxError "Invalid code") (if (!= (. text length) 3)))
    (return (call Code (. text length) 1))
  )
)

(type + Base
  (fn-expr + code Code
    (call Code 0 0)
  )
)

(type + Child (is Base)
  (fn-do + code Code
    (let fallback (call Code 0 0))
    (return (if-expr (== (. fallback width) 0) "abc" fallback))
  )
)
```

```output
3
```

### A raw catch holds a value or its error, and catch? a value or nothing.

```brix
(main
  (let result (catch (call check 3)))
  (print
    (match result
      (case Int
        (str "value " it)
      )
      (case IOError
        (str "io " (. it message))
      )
    )
  )
  (let other (catch (call check -1)))
  (print
    (match other
      (case Int
        (str "value " it)
      )
      (case IOError
        (str "io " (. it message))
      )
    )
  )
  (let maybe (catch? (call check -1)))
  (print (?? maybe 99))
  (let sure (catch? (call check 5)))
  (print (?? sure 99))
)

(fn-do + check Int
  (param value Int)
  (throw (call IOError "negative") (if (< value 0)))
  (return (+ value 1))
)
```

```output
value 4
io negative
99
6
```

### What a raw catch holds has no truth, even over a call that answers nothing: a condition over it is an error that says to test it with `is`.

```brix compile-error
(main
  (let failure (catch (call check -1)))
  (print (if-expr failure "failed" "worked"))
)

(fn-do + check _ (?! IOError)
  (param value Int)
  (throw (call IOError "negative") (if (< value 0)))
)
```

```output
Exception: A value that may be an error, as "catch" gives, has no truth; test it with "is Error", or take it apart with "match"
<no file:4>
      (print (if-expr failure "failed" "worked"))
                      ^^^^^^^
```

### `not` denies no truth of what a raw catch holds, even one whose value is optional.

```brix compile-error
(main
  (let found (catch (call lookup -1)))
  (if (not found)
    (print "none")
  )
)

(fn-do + lookup (? Int) (?! IOError)
  (param value Int)
  (throw (call IOError "negative") (if (< value 0)))
  (return-if null (if (== value 0)))
  (return value)
)
```

```output
Exception: A value that may be an error, as "catch" gives, has no truth; test it with "is Error", or take it apart with "match"
<no file:4>
      (if (not found)
               ^^^^^
```

### An error that leaves main is reported and ends the program.

```brix crash
(main
  (print "before")
  (call check 3)
  (print "middle")
  (call check -1)
  (print "after")
)

(fn-do + check Int
  (param value Int)
  (throw (call IOError "negative") (if (< value 0)))
  (return (+ value 1))
)
```

```output
before
middle
```

### An error propagates through its callers to main.

```brix crash
(main
  (print (call outer 3))
  (print (call outer -1))
  (print "after")
)

(fn-do + outer Int
  (param value Int)
  (let inner (call check value))
  (print (str "inner " inner))
  (return (* inner 2))
)

(fn-do + check Int
  (param value Int)
  (throw (call IOError "negative") (if (< value 0)))
  (return (+ value 1))
)
```

```output
inner 4
8
```

### Results of nothing, text, a tuple and a float, with one error or two, and a relay that infers its errors.

```brix
(main
  (print (catch (call save 1)))
  (print (catch (call save -1)))
  (print (catch? (call name 2)))
  (print (catch? (call name -2)))
  (print (?? (catch? (call pair 3)) (tuple 0 "none")))
  (print (?? (catch? (call pair -3)) (tuple 0 "none")))
  (let kept (catch (call wide 5)))
  (print
    (match kept
      (case Float
        (str "float " it)
      )
      (case IOError
        "io"
      )
      (case SyntaxError
        "syntax"
      )
    )
  )
  (print
    (match (catch (call wide -5))
      (case Float
        "float"
      )
      (case IOError
        (str "io " (. it message))
      )
      (case SyntaxError
        (str "syntax " (. it message))
      )
    )
  )
  (print
    (match (catch (call wide 0))
      (case Float
        "float"
      )
      (case IOError
        (str "io " (. it message))
      )
      (case SyntaxError
        (str "syntax " (. it message))
      )
    )
  )
  (print
    (match (catch (call relay 0))
      (case Float
        "float"
      )
      (case Error
        (str "error " (. it message))
      )
    )
  )
  (print (catch? (call relay 4)))
)

(fn-do + save _ (?! IOError)
  (param value Int)
  (throw (call IOError "cannot save") (if (< value 0)))
  (print (str "saved " value))
)

(fn-do + name String (?! IOError)
  (param value Int)
  (throw (call IOError "no name") (if (< value 0)))
  (return (str "name" value))
)

(fn-do + pair (tuple Int String) (?! SyntaxError)
  (param value Int)
  (throw (call SyntaxError "no pair") (if (< value 0)))
  (return (tuple value "pair"))
)

(fn-do + wide Float (?! IOError) (?! SyntaxError)
  (param value Int)
  (throw (call IOError "io") (if (< value 0)))
  (throw (call SyntaxError "syntax") (if (== value 0)))
  (return (+ (as value Float) 0.5))
)

(fn-do + relay Float
  (param value Int)
  (let first (call wide (+ value 1)))
  (return (+ first (call wide value)))
)
```

```output
saved 1
core:Void
cannot save
name2
unknown
(3, pair)
(0, none)
float 5.5
io io
syntax syntax
error syntax
10
```

### The cleanups of each function run as an error leaves it.

```brix crash
(main
  (print (catch? (call outer 1)))
  (print (catch? (call outer -1)))
  (print total)
  (call outer -7)
  (print "unreached")
)

(blank)
(prop _ total _ (= 0))

(fn-do + outer Int
  (param value Int)
  (finally
    (+= total 1)
    (print "outer done")
  )
  (let first (call inner value))
  (let second (+ (call inner (+ value 1)) (call inner (+ value 2))))
  (return (+ first second))
)

(fn-do + inner Int
  (param value Int)
  (finally
    (+= total 10)
  )
  (throw (call IOError (str "inner " value)) (if (< value 0)))
  (return (* value 2))
)
```

```output
outer done
12
outer done
unknown
42
outer done
```

### A method raises an error of a class the program declares.

```brix crash
(main
  (let box (call Box 3))
  (print (catch? (call (. box take) 2)))
  (print (catch? (call (. box take) 5)))
  (print
    (match (catch (call (. box take) 9))
      (case Int
        (str "took " it)
      )
      (case BoxError
        (str "box " (. it message) " " (. it wanted))
      )
    )
  )
  (print (catch? (call (. box drain))))
  (call (. box take) 100)
  (print "unreached")
)

(type + BoxError (is Error)
  (prop + wanted Int)
  (fn-do + This _
    (param count Int)
    (lazy message "empty")
    (lazy wanted count)
  )
)

(type + Box
  (prop + held Int)
  (fn-do + This _
    (param count Int)
    (lazy held count)
  )
  (fn-do + take Int
    (param count Int)
    (throw (call BoxError count) (if (> count held)))
    (-= held count)
    (return held)
  )
  (fn-do + drain Int
    (return (+ (call take held) (call take 1)))
  )
)
```

```output
1
unknown
box empty 9
unknown
```

### An error of a function that states Error is held as the case of Error.

```brix crash
(main
  (print (catch? (call stated 1)))
  (print
    (match (catch (call stated -1))
      (case Int
        "value"
      )
      (case Error
        (str "error " (. it message))
      )
    )
  )
  (print
    (match (catch (call stated 0))
      (case Int
        "value"
      )
      (case Error
        (str "error " (. it message))
      )
    )
  )
  (let kept (catch (call chain -1)))
  (print (is kept Error))
  (print (is kept Int))
  (call stated -1)
)

(fn-do + stated Int (?! Error)
  (param value Int)
  (throw (call IOError "io") (if (< value 0)))
  (throw (call SyntaxError "syntax") (if (== value 0)))
  (return value)
)

(fn-do + chain Int (?! Error)
  (param value Int)
  (return (+ (call stated value) 1))
)
```

```output
1
error io
error syntax
true
false
```

### Errors leave from inside blocks and loops, with the cleanups of each.

```brix crash
(main
  (print (catch? (call deep 3)))
  (print (catch? (call deep -3)))
  (call deep -1)
)

(fn-do + deep Int
  (param value Int)
  (if (> value 0)
    (finally
      (print "leaving block")
    )
    (let inner (call check value))
    (return inner)
  )
  (while true
    (finally
      (print "leaving loop")
    )
    (call check value)
    (break)
  )
  (return 0)
)

(fn-do + check Int
  (param value Int)
  (throw (call IOError "negative") (if (< value 0)))
  (return (+ value 1))
)
```

```output
leaving block
4
leaving loop
unknown
leaving loop
```

### A held Result is copied, tested, passed to a function and named Union.

```brix
(main
  (let first (catch (call check 1)))
  (let second (catch (call check -1)))
  (print (type-of first))
  (print first)
  (print second)
  (let third second)
  (print (is third IOError))
  (call show first)
  (call show second)
)

(fn-do + show _
  (param result _)
  (print
    (match result
      (case Int
        (str "int " it)
      )
      (case IOError
        "error"
      )
    )
  )
)

(fn-do + check Int
  (param value Int)
  (throw (call IOError "negative") (if (< value 0)))
  (return (+ value 1))
)
```

```output
[TypeInfo Union]
2
negative
true
int 2
error
```

### Handlers capture the error, are tried in the order written, and let the errors they do not take leave.

```brix crash
(main
  (print (call named 1))
  (print (call named -1))
  (print (call named 0))
  (print (call broad -1))
  (print (call broad 0))
  (print (catch? (call narrow -1)))
  (print (catch? (call narrow 0)))
  (print (catch? (call narrow 5)))
  (print total)
  (call stated 0
    (block
      (on-error IOError
        (do
          (inline (print "main io"))
        )
        0
      )
    )
  )
  (print "main after")
  (call stated -1
    (block
      (on-error SyntaxError
        (do
          (inline (print "main syntax"))
        )
        0
      )
    )
  )
  (print "unreached")
)

(blank)
(prop _ total _ (= 0))

(fn-do + stated Int (?! Error)
  (param value Int)
  (throw (call IOError (str "io " value)) (if (< value 0)))
  (throw (call SyntaxError (str "syntax " value)) (if (== value 0)))
  (return value)
)

(fn-do + bump Int
  (param by Int)
  (+= total by)
  (return (- 0 by))
)

(fn-do + named String
  (param value Int)
  (let got
    (call stated value
      (block
        (on-error (: failure IOError)
          (call bump (. failure message length))
        )
        (on-error (: other Error)
          (call bump 10)
        )
      )
    )
  )
  (return (str "got " got))
)

(fn-do + broad String
  (param value Int)
  (finally
    (+= total 100)
  )
  (let text
    (call stated value
      (block
        (on-error (: failure Error)
          (. failure message length)
        )
      )
    )
  )
  (return (str "broad " text))
)

(fn-do + narrow Int
  (param value Int)
  (finally
    (+= total 1000)
  )
  (let got
    (call stated value
      (block
        (on-error SyntaxError
          (call relay value)
        )
      )
    )
  )
  (return (+ got 1))
)

(fn-do + relay Int
  (param value Int)
  (return (call stated (- value 1)))
)
```

```output
got 1
got -5
got -10
broad 5
broad 8
unknown
unknown
6
3215
```

### A match whose arms all give errors raises the error of the arm taken.

```brix
(main
  (print (catch? (call pick 1)))
  (print (catch? (call pick 2)))
  (print "after")
)

(fn-do + pick String
  (param kind Int)
  (let chosen
    (match kind
      (case= 1
        (call IOError "one")
      )
      (default
        (call SyntaxError "other")
      )
    )
  )
  (print (type-of chosen))
  (return "picked")
)
```

```output
unknown
unknown
after
```

### An arm that gives an error beside arms that give values raises it at the arm.

```brix crash
(main
  (print (catch? (call pick 1)))
  (print (catch? (call pick 2)))
  (call pick 1)
  (print "after")
)

(fn-do + pick String
  (param kind Int)
  (let chosen
    (match kind
      (case= 1
        (call IOError "one")
      )
      (default
        "plain"
      )
    )
  )
  (print (type-of chosen))
  (return chosen)
)
```

```output
unknown
[TypeInfo String]
plain
```

### An arm that gives an error its declared type holds gives the error as a value, and an arm whose error the declared type cannot hold raises it.

```brix
(main
  (print (?? (?. (call by-match 0) message) "none"))
  (print (?? (?. (call by-match 1) message) "none"))
  (print (?? (?. (call by-table -1) message) "none"))
  (print (?? (?. (call by-table 1) message) "none"))
  (print (. (call named 0) message))
  (print (. (call named 1) message))
  (print (?? (catch? (call text 0)) "raised"))
  (print (?? (catch? (call text 1)) "raised"))
)

(type + Oops (is Error))

(fn-expr + by-match (? Oops)
  (param flag Int)
  (match flag
    (case= 0
      (call Oops "zero")
    )
    (default
      null
    )
  )
)

(fn-expr + by-table (? Error)
  (param value Int)
  (case? (< value 0)
    (call Oops "below")
  )
  (default
    null
  )
)

(fn-expr + named Oops
  (param flag Int)
  (match flag
    (case= 0
      (call Oops "zero")
    )
    (default
      (call Oops "other")
    )
  )
)

(fn-expr + text String
  (param flag Int)
  (match flag
    (case= 0
      (call Oops "zero")
    )
    (default
      "plain"
    )
  )
)
```

```output
zero
none
below
none
zero
other
raised
plain
```

### An arm of a table with no subject raises where its condition holds, and the arms after it are tested where it does not.

```brix
(main
  (print (call describe 3 false))
  (print (call describe 3 true))
  (print (call describe 200 false))
)

(fn-expr + f Int (?! Error)
  (param n Int)
  (param flag Bool)
  (case? flag
    1
  )
  (throw (call Error "too big") (if (> n 100)))
  (default
    (* n 2)
  )
)

(fn-expr + describe String
  (param n Int)
  (param flag Bool)
  (match (catch (call f n flag))
    (case (: value Int)
      (str value)
    )
    (case (: error Error)
      (. error message)
    )
  )
)
```

```output
6
1
too big
```

### An arm of a table with no subject panics where its condition holds.

```brix crash
(main
  (print (call f 3 true))
  (print (call f 200 false))
)

(fn-expr + f Int
  (param n Int)
  (param flag Bool)
  (case? flag
    1
  )
  (throw (call Panic (str "too big: " n)) (if (> n 100)))
  (default
    (* n 2)
  )
)
```

```stderr
Unhandled panic: too big: 200
<no file:13>

Task 1

Stack trace:
  0  __source__:f no file:13:3
  1  __source__:main no file:4:10
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

### An arm of a table with no subject whose body ends in a raise leaves the function where it is taken.

```brix
(main
  (print (call describe 1))
  (print (call describe 20))
  (print (call describe 5))
)

(fn-expr + f Int (?! Error)
  (param n Int)
  (case? (> n 10)
    (throw (call Error (str "big " doubled)))
    (blank)
    (lazy doubled (* n 2))
  )
  (case? (< n 3)
    n
  )
  (default
    (throw (call Error (str "bad " tripled)))
    (blank)
    (lazy tripled (* n 3))
  )
)

(fn-expr + describe String
  (param n Int)
  (match (catch (call f n))
    (case (: value Int)
      (str value)
    )
    (case (: error Error)
      (. error message)
    )
  )
)
```

```output
1
big 40
bad 15
```

### A choice between an error and a text has no type in common.

```brix crash
(main
  (print (catch? (call pick 1)))
  (print (catch? (call pick 2)))
  (print "after")
)

(fn-do + pick String
  (param kind Int)
  (let chosen (if-expr (== kind 1) (call IOError "one") "plain"))
  (print (type-of chosen))
  (return chosen)
)
```

```output
Exception: The two sides of this "if" are IOError and String, which have no common type; convert them to one type
<no file:10>
      (let chosen (if-expr (== kind 1) (call IOError "one") "plain"))
                  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### An error constructed as a statement or held in a local is not raised.

```brix
(main
  (print (catch? (call pick 1)))
  (print "after")
)

(fn-do + pick String
  (param kind Int)
  (let made (call IOError "one"))
  (print (type-of made))
  (call IOError "dropped")
  (print "still here")
  (return (. made message))
)
```

```output
[TypeInfo IOError]
still here
one
after
```

### An arm that raises an error, and a match on conditions whose arms give errors.

```brix crash
(main
  (print (catch? (call shade "Red")))
  (print (catch? (call shade "Blue")))
  (print (catch? (call level 1)))
  (print (catch? (call level 5)))
  (print (catch? (call level -5)))
  (call shade "Blue")
)

(fn-expr + shade _ (?! Error)
  (param name String)
  (match name
    (case= "Red"
      1
    )
    (case= "Green"
      2
    )
    (throw (call Error (str "Unknown shade '" name "'")))
  )
)

(fn-expr + level _
  (param value Int)
  (case? (< value 0)
    (call IOError "below")
  )
  (case? (> value 3)
    (call SyntaxError "above")
  )
  (default
    value
  )
)
```

```output
1
unknown
1
unknown
unknown
```

### A returned Result returns the error it holds in its place.

```brix crash
(main
  (print (catch (call relay true)))
  (print (catch (call relay false)))
  (print (catch? (call widen true)))
  (print (catch? (call widen false)))
  (print (catch? (call guarded true)))
  (print (catch? (call guarded false)))
  (print total)
  (call relay false)
)

(blank)
(prop _ total _ (= 0))

(fn-do + read I32 (?! IOError)
  (param ok Bool)
  (if ok
    (return 7)
    (else
      (throw (call IOError "read failed"))
    )
  )
)

(fn-do + relay Int (?! IOError)
  (param ok Bool)
  (return (catch (call read ok)))
)

(fn-do + widen Float (?! Error)
  (param ok Bool)
  (return (catch (call read ok)))
)

(fn-do + guarded Int
  (param ok Bool)
  (finally
    (+= total 1)
  )
  (let kept (catch (call read ok)))
  (return kept)
)
```

```output
7
read failed
7
unknown
7
unknown
2
```

### Errors leave through nested calls, loops and conditional returns.

```brix
(main
  (print (catch? (call chain 1)))
  (print (catch? (call chain -1)))
  (print (catch? (call looped 3)))
  (print (catch? (call looped -3)))
  (print (catch? (call early 0)))
  (print (catch? (call early 4)))
  (let raw (catch (call nested 2)))
  (print
    (match raw
      (case String
        (str "text " it)
      )
      (case IOError
        (str "io " (. it message))
      )
      (case SyntaxError
        "syntax"
      )
    )
  )
  (print
    (match (catch (call nested -2))
      (case String
        (str "text " it)
      )
      (case IOError
        (str "io " (. it message))
      )
      (case SyntaxError
        "syntax"
      )
    )
  )
  (print
    (match (catch (call nested 0))
      (case String
        (str "text " it)
      )
      (case IOError
        (str "io " (. it message))
      )
      (case SyntaxError
        (str "syntax " (. it message))
      )
    )
  )
)

(fn-do + check Int
  (param value Int)
  (throw (call IOError (str "negative " value)) (if (< value 0)))
  (return (+ value 1))
)

(fn-do + parse String
  (param value Int)
  (throw (call SyntaxError "zero") (if (== value 0)))
  (return (str "parsed " value))
)

(fn-do + chain Int
  (param value Int)
  (return (call check (call check (call check value))))
)

(fn-do + looped Int
  (param count Int)
  (let total 0)
  (let index 0)
  (while (< index 3)
    (+= total (call check (+ count index)))
    (+= index 1)
  )
  (return total)
)

(fn-do + early Int
  (param value Int)
  (if (== value 0)
    (return (call check -9))
  )
  (let doubled (* (call check value) 2))
  (return-if doubled (if (> doubled 100)))
  (return (call check doubled))
)

(fn-do + nested String
  (param value Int)
  (let checked (call check value))
  (return (call parse (- checked 1)))
)
```

```output
4
unknown
15
unknown
unknown
11
text parsed 2
io negative -2
syntax zero
```

### A handler that gives an error beside handlers that give values keeps the error as a value

```brix
(main
  (print (catch (call pick 0)))
  (print (catch (call pick 1)))
  (print (catch (call pick 2)))
  (print (catch (call pick 3)))
  (let kept (catch? (call pick 1)))
  (print (== kept null))
  (let held (catch? (call pick 0)))
  (print (== held null))
)

(type + Missing (is Error))

(type + Denied (is Error))

(type + Wrapped (is Error))

(fn-do + read Int (?! Missing) (?! Denied) (?! IOError)
  (param outcome Int)
  (if (== outcome 1)
    (throw (call Missing "missing"))
  )
  (if (== outcome 2)
    (throw (call Denied "denied"))
  )
  (if (== outcome 3)
    (throw (call IOError "io"))
  )
  (return 7)
)

(fn-do + pick _
  (param outcome Int)
  (let handled
    (call read outcome
      (block
        (on-error (: err Missing)
          (call Wrapped (str "wrapped " (. err message)))
        )
        (on-error Denied
          (+ 40 2)
        )
      )
    )
  )
  (print (str "after " outcome))
  (return handled)
)
```

```output
after 0
7
after 1
wrapped missing
after 2
42
io
after 1
true
after 0
false
```

### Two handlers that each give an error of their own class

```brix
(main
  (print (catch (call both 0)))
  (print (catch (call both 1)))
  (print (catch (call both 2)))
)

(type + Missing (is Error))

(type + Denied (is Error))

(type + First (is Error))

(type + Second (is Error))

(fn-do + read String (?! Missing) (?! Denied)
  (param outcome Int)
  (if (== outcome 1)
    (throw (call Missing "missing"))
  )
  (if (== outcome 2)
    (throw (call Denied "denied"))
  )
  (return "read")
)

(fn-do + both _
  (param outcome Int)
  (let handled
    (call read outcome
      (block
        (on-error (: err Missing)
          (call First (str "first " (. err message)))
        )
        (on-error (: err Denied)
          (call Second (str "second " (. err message)))
        )
      )
    )
  )
  (print "after")
  (return handled)
)
```

```output
after
read
after
first missing
after
second denied
```

### A handler that gives an error for a call that gives nothing, returned and left unhandled in the module

```brix crash
(main
  (call all 0)
  (let seen (catch (call all 1)))
  (match seen
    (case (: err Wrapped)
      (print (str "wrapped: " (. err message)))
    )
    (case Void
      (print "nothing")
    )
  )
  (let other (catch? (call all 1)))
  (print (== other null))
  (call all 1)
  (print "unreached")
)

(type + Missing (is Error))

(type + Wrapped (is Error))

(fn-do + read _ (?! Missing)
  (param outcome Int)
  (if (== outcome 1)
    (throw (call Missing "missing"))
  )
  (print "read ran")
)

(fn-do + all _
  (param outcome Int)
  (let handled
    (call read outcome
      (block
        (on-error (: err Missing)
          (call Wrapped (str "wrapped " (. err message)))
        )
      )
    )
  )
  (print "after")
  (return handled)
)
```

```output
read ran
after
after
wrapped: wrapped missing
after
true
after
```

### An argument under catch raises past the catch, which holds only the errors of the call

```brix crash
(main
  (print (call outer 1))
  (print (call outer 20))
  (let got (catch? (call outer -5)))
  (print (== got null))
  (let other (catch (call outer -1)))
  (match other
    (case (: err Small)
      (print (str "main " (. err message)))
    )
    (case (: number Int)
      (print number)
    )
  )
  (print (call outer -1))
)

(type + Small (is Error))

(type + Large (is Error))

(fn-do + make Int (?! Small)
  (param value Int)
  (if (< value 0)
    (throw (call Small (str "small " value)))
  )
  (return (* value 2))
)

(fn-do + take Int (?! Large)
  (param value Int)
  (param extra Int)
  (if (> value 10)
    (throw (call Large (str "large " value)))
  )
  (return (+ value extra))
)

(fn-do + outer Int (?! Small)
  (param value Int)
  (let held (catch (call take (call make value) (call make 1))))
  (match held
    (case (: err Large)
      (print (str "caught " (. err message)))
      (return -1)
    )
    (case (: number Int)
      (return number)
    )
  )
)
```

```output
4
caught large 40
-1
true
main small -1
```

### An argument of a method call under an optional catch raises past the catch

```brix crash
(main
  (print (call run 1))
  (print (call run 20))
  (let lost (catch? (call run -1)))
  (print (== lost null))
  (print (call run -1))
)

(type + Small (is Error))

(type + Large (is Error))

(type + Box
  (prop + value Int)
  (fn-do + This _
    (param value _)
  )
  (fn-do + grow Int (?! Large)
    (param by Int)
    (if (> (+ value by) 10)
      (throw (call Large "large"))
    )
    (return (+ value by))
  )
)

(fn-do + make Int (?! Small)
  (param value Int)
  (if (< value 0)
    (throw (call Small "small"))
  )
  (return value)
)

(fn-do + run Int (?! Small)
  (param value Int)
  (let box (call Box 1))
  (let grown (catch? (call (. box grow) (call make value))))
  (if (== grown null)
    (return -1)
  )
  (return 0)
)
```

```output
0
-1
true
```

### Handlers in order where a later class is the base of an earlier one, joined with a number of another kind

```brix
(main
  (print (catch (call chain 0)))
  (print (catch (call chain 1)))
  (print (catch (call chain 2)))
)

(type + Missing (is Error))

(type + Denied (is Missing))

(type + Wrapped (is Error))

(type + Deep (is Wrapped))

(fn-do + read Float (?! Missing)
  (param outcome Int)
  (if (== outcome 1)
    (throw (call Missing "missing"))
  )
  (if (== outcome 2)
    (throw (call Denied "denied"))
  )
  (return 1.5)
)

(fn-do + chain _
  (param outcome Int)
  (let handled
    (call read outcome
      (block
        (on-error (: err Denied)
          (call Deep (str "deep " (. err message)))
        )
        (on-error (: err Missing)
          3i32
        )
      )
    )
  )
  (print "after")
  (return handled)
)
```

```output
after
1.5
after
3
after
deep denied
```

### A handler in a module that gives an error, matched after it

```brix
(main
  (let first
    (call inner 1
      (block
        (on-error (: err Missing)
          (call Wrapped (str "top " (. err message)))
        )
      )
    )
  )
  (match first
    (case (: err Wrapped)
      (print (str "is wrapped " (. err message)))
    )
    (case (: number Int)
      (print (str "is number " number))
    )
  )
  (let second
    (call inner 0
      (block
        (on-error (: err Missing)
          (call Wrapped (str "top " (. err message)))
        )
      )
    )
  )
  (match second
    (case (: err Wrapped)
      (print (str "is wrapped " (. err message)))
    )
    (case (: number Int)
      (print (str "is number " number))
    )
  )
)

(type + Missing (is Error))

(type + Wrapped (is Error))

(fn-do + inner Int (?! Missing)
  (param outcome Int)
  (if (== outcome 1)
    (throw (call Missing "missing"))
  )
  (return 9)
)
```

```output
is wrapped top missing
is number 9
```

### A pattern and a test of a class on a caught value that may be null or an error

```brix
(main
  (print (call name 0))
  (print (call name 1))
  (print (call name 2))
  (print (call size 0))
  (print (call size 1))
  (print (call size 2))
)

(type + Box
  (prop + label String)
  (fn-do + This _
    (param label _)
  )
)

(fn-do + find (? Box) (?! IOError)
  (param kind Int)
  (if (== kind 0)
    (throw (call IOError "io"))
  )
  (if (== kind 1)
    (return (call Box "held"))
  )
  (return null)
)

(fn-do + name String
  (param kind Int)
  (let held (catch (call find kind)))
  (match? held
    (case (: box Box)
      (return (. box label))
    )
    (case (: err IOError)
      (return (str "error " (. err message)))
    )
  )
  (return "empty")
)

(fn-do + size Int
  (param kind Int)
  (let held (catch (call find kind)))
  (if (is held Box)
    (return 1)
  )
  (return 0)
)
```

```output
error io
held
empty
0
1
0
```

### A constructor of a tuple that raises, handled, held, made optional and left unhandled

```brix crash
(main
  (print (call made 3))
  (print (call made -3))
  (print (call kept 4))
  (print (call kept -4))
  (let lost (catch? (call Pair -1)))
  (print (== lost null))
  (let found (catch? (call Pair 6)))
  (print (== found null))
  (print (. (call Pair -9) left))
  (print "unreached")
)

(type + Pair (is Tuple)
  (prop + left Int)
  (prop + right Int)
  (fn-do + This _ (?! IOError)
    (param seed Int)
    (if (< seed 0)
      (throw (call IOError (str "negative " seed)))
    )
    (= left seed)
    (= right (* seed 2))
  )
)

(fn-do + made Int
  (param seed Int)
  (let pair
    (call Pair seed
      (block
        (on-error IOError
          (call Pair 1)
        )
      )
    )
  )
  (return (+ (. pair left) (. pair right)))
)

(fn-do + kept String
  (param seed Int)
  (let held (catch (call Pair seed)))
  (return
    (match held
      (case (: pair Pair)
        (str "pair " (. pair right))
      )
      (case (: err IOError)
        (str "error " (. err message))
      )
    )
  )
)
```

```output
9
3
pair 8
error negative -4
true
false
```

### A constructor of a class that raises and propagates, as a handled value and as an argument under catch

```brix crash
(main
  (print (call built 2))
  (print (call built -2))
  (print (call built 50))
  (print (call deep 1))
  (print (call deep -1))
  (call Holder (call Part -7))
  (print "unreached")
)

(type + Part
  (prop + size Int)
  (fn-do + This _ (?! IOError) (?! SyntaxError)
    (param wanted Int)
    (if (< wanted 0)
      (throw (call IOError "negative"))
    )
    (call check wanted)
    (= size wanted)
  )
)

(type + Holder
  (prop + part Part)
  (prop + note _ (= "none"))
  (fn-do + This _
    (param part _)
    (= note (str "holds " (. part size)))
  )
)

(fn-do + check _ (?! SyntaxError)
  (param wanted Int)
  (if (> wanted 10)
    (throw (call SyntaxError "too large"))
  )
)

(fn-do + built String
  (param wanted Int)
  (let part
    (call Part wanted
      (block
        (on-error (: err IOError)
          (return (str "io " (. err message)))
        )
        (on-error (: err SyntaxError)
          (return (str "syntax " (. err message)))
        )
      )
    )
  )
  (return (str "size " (. part size)))
)

(fn-do + deep String
  (param wanted Int)
  (let holder (catch? (call Holder (call Part wanted))))
  (return-if "lost" (if (== holder null)))
  (return (. holder note))
)
```

```output
size 2
io negative
syntax too large
holds 1
```

### Constructors that raise beside a return of nothing and a value of their own

```brix
(main
  (print (call count 1))
  (print (call count -1))
  (print (call own 2))
  (print (call own -2))
)

(type + Counted
  (prop + label _ (= "plain"))
  (fn-do + This _ (?! IOError)
    (param seed Int)
    (if (< seed 0)
      (throw (call IOError "negative"))
    )
    (if (> seed 100)
      (return)
    )
    (= label "seeded")
  )
)

(type + Code (is Tuple)
  (prop + width _ (= 0))
  (prop + height _ (= 0))
  (fn-do + This _ (?! IOError)
    (param seed Int)
    (if (< seed 0)
      (throw (call IOError "no code"))
    )
    (return (call Code seed (+ seed 1)))
  )
)

(fn-do + count String
  (param seed Int)
  (let held (catch (call Counted seed)))
  (return
    (match held
      (case (: counted Counted)
        (. counted label)
      )
      (case (: err IOError)
        (. err message)
      )
    )
  )
)

(fn-do + own String
  (param seed Int)
  (let code
    (call Code seed
      (block
        (on-error (: err IOError)
          (return (. err message))
        )
      )
    )
  )
  (return (str "code " (. code width) " " (. code height)))
)
```

```output
seeded
negative
code 2 3
no code
```

### A value converted by a function that raises: the error leaves from a declaration, a return and a cast, and a text written in source panics

```brix
(main
  (print (call width "abc"))
  (print (call declared true))
  (print (call held "toolong"))
  (print (call held "abc"))
  (let lost (catch? (call parse "x")))
  (print (== lost null))
  (print (call cast "abc"))
  (let attempt
    (do-new
      (print (call declared false))
      (print "unreached")
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)

(type + Code (is Tuple)
  (prop + width _ (= 0))
  (prop + height _ (= 0))
  (fn-from-do + (: text String)
    (throw (call SyntaxError (str "Invalid code " text)) (if (!= (. text length) 3)))
    (return (call Code (. text length) 1))
  )
)

(fn-do + width Int
  (param text String)
  (let code Code text)
  (return (. code width))
)

(fn-do + cast Int
  (param text String)
  (return (. (as text Code) width))
)

(fn-do + declared Int
  (param good Bool)
  (if good
    (let code Code "abc")
    (return (. code width))
  )
  (let code Code "toolong")
  (return (. code height))
)

(fn-do + parse Code
  (param text String)
  (return text)
)

(fn-do + held String
  (param text String)
  (let result (catch (call parse text)))
  (return
    (match result
      (case (: err SyntaxError)
        (str "error " (. err message))
      )
      (case (: code Code)
        (str "code " (. code width))
      )
    )
  )
)
```

```output
3
3
error Invalid code toolong
code 3
true
3
Invalid code toolong
```

### An argument and an assignment converted by a function that raises, caught by a caller and left unhandled

```brix crash
(main
  (print (call size "abc"))
  (print (call taken "abc"))
  (print (call described "abc"))
  (print (call described "toolong"))
  (print (call made "abc"))
  (print (call made "no"))
  (print (call size "toolong"))
  (print "unreached")
)

(type + Code (is Tuple)
  (prop + width _ (= 0))
  (prop + height _ (= 0))
  (fn-from-do + (: text String)
    (throw (call SyntaxError (str "Invalid code " text)) (if (!= (. text length) 3)))
    (return (call Code (. text length) 1))
  )
)

(fn-do + take Int
  (param code Code)
  (return (+ (. code width) (. code height)))
)

(fn-do + size Int
  (param text String)
  (return (call take text))
)

(fn-do + taken Int
  (param text String)
  (let code Code (call Code 0 0))
  (= code text)
  (return (call take code))
)

(fn-do + described String
  (param text String)
  (let result (catch (call size text)))
  (return
    (match result
      (case (: err SyntaxError)
        (str "error " (. err message))
      )
      (case (: total Int)
        (str "total " total)
      )
    )
  )
)

(fn-do + made String
  (param text String)
  (let result (catch (call Code text)))
  (return
    (match result
      (case (: err SyntaxError)
        (str "error " (. err message))
      )
      (case (: code Code)
        (str "code " (. code width))
      )
    )
  )
)
```

```output
4
4
total 4
error Invalid code toolong
code 3
error Invalid code no
```

### A call of a type that converts a value raises from the function, and gives its error under catch

```brix crash
(main
  (print (call direct "abc"))
  (print (call optional "abc"))
  (print (call optional "toolong"))
  (print (call literal))
  (print (call direct "toolong"))
  (print "unreached")
)

(type + Code (is Tuple)
  (prop + width _ (= 0))
  (prop + height _ (= 0))
  (fn-from-do + (: text String)
    (throw (call SyntaxError (str "Invalid code " text)) (if (!= (. text length) 3)))
    (return (call Code (. text length) 1))
  )
)

(fn-do + direct Int
  (param text String)
  (let code (call Code text))
  (return (. code width))
)

(fn-do + optional Int
  (param text String)
  (let code (catch? (call Code text)))
  (return-if -1 (if (== code null)))
  (return (. code width))
)

(fn-do + literal String
  (let result (catch (call Code "toolong")))
  (return
    (match result
      (case (: err SyntaxError)
        (str "error " (. err message))
      )
      (case (: code Code)
        (str "code " (. code width))
      )
    )
  )
)
```

```output
3
3
-1
error Invalid code toolong
```

### A join of a text written in source with another text, both converted by a function that raises

```brix
(main
  (print (. (call mixed true "abc") width))
  (print (. (call mixed false "abc") width))
)

(type + Code (is Tuple)
  (prop + width _ (= 0))
  (prop + height _ (= 0))
  (fn-from-do + (: text String)
    (throw (call SyntaxError (str "Invalid code " text)) (if (!= (. text length) 3)))
    (return (call Code (. text length) 1))
  )
)

(fn-do + mixed Code
  (param good Bool)
  (param text String)
  (return (if-expr good "abc" text))
)
```

```output
3
3
```

### A text written in source on either side of a conditional panics where its conversion fails

```brix
(main
  (print (. (call joined false) width))
  (print (. (call other true) width))
  (let attempt
    (do-new
      (print (. (call joined true) width))
      (print "unreached")
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)

(type + Code (is Tuple)
  (prop + width _ (= 0))
  (prop + height _ (= 0))
  (fn-from-do + (: text String)
    (throw (call SyntaxError (str "Invalid code " text)) (if (!= (. text length) 3)))
    (return (call Code (. text length) 1))
  )
)

(fn-do + joined Code
  (param bad Bool)
  (return (if-expr bad "toolong" (call Code 9 9)))
)

(fn-do + other Code
  (param good Bool)
  (return (if-expr good (call Code 8 8) "toolong"))
)
```

```output
9
8
Invalid code toolong
```

### A text written in source as an argument, a property's value and an assignment, converted by a function that raises

```brix
(main
  (print (call take "abc"))
  (print (. (call stored) width))
  (let attempt
    (do-new
      (print (call take "toolong"))
      (print "unreached")
      (on-failure failure
        (print failure)
      )
    )
  )
  (call (. attempt wait))
)

(type + Code (is Tuple)
  (prop + width _ (= 0))
  (prop + height _ (= 0))
  (fn-from-do + (: text String)
    (throw (call SyntaxError (str "Invalid code " text)) (if (!= (. text length) 3)))
    (return (call Code (. text length) 1))
  )
)

(type + Holder
  (prop + code Code (= "abc"))
)

(fn-do + sum Int
  (param code Code)
  (return (+ (. code width) (. code height)))
)

(fn-do + take Int
  (param label String)
  (if (== label "abc")
    (return (call sum "abc"))
  )
  (return (call sum "toolong"))
)

(fn-do + stored Code
  (let holder (call Holder))
  (= (. holder code) "xyz")
  (return (. holder code))
)
```

```output
4
3
Invalid code toolong
```

### A call of a type with a text written in source panics where the conversion fails and nothing recovers

```brix crash
(main
  (print (. (call made) width))
  (print (. (call broken) width))
  (print "unreached")
)

(type + Code (is Tuple)
  (prop + width _ (= 0))
  (prop + height _ (= 0))
  (fn-from-do + (: text String)
    (throw (call SyntaxError (str "Invalid code " text)) (if (!= (. text length) 3)))
    (return (call Code (. text length) 1))
  )
)

(fn-expr + made Code
  (call Code "abc")
)

(fn-do + broken Code
  (let code (call Code "toolong"))
  (return code)
)
```

```output
3
```

### A function whose value is a raise, and a table of conditions that ends in one

```brix
(main
  (print (?? (catch? (call fail)) "caught"))
  (print (?? (catch? (call digit 10)) -1))
  (print (?? (catch? (call digit 40)) -1))
  (print (?? (catch? (call digit 99)) -1))
  (print (?? (catch? (call later 1)) "caught"))
  (print (?? (catch? (call later 2)) "caught"))
)

(type + Broken (is Error))

(fn-expr + fail String (?! Broken)
  (throw (call Broken "always"))
)

(fn-expr + digit Int (?! Broken)
  (param code Int)
  (case? (and (>= code 0) (<= code 25))
    (+ code 100)
  )
  (case? (and (>= code 26) (<= code 51))
    (+ code 200)
  )
  (throw (call Broken "no digit"))
)

(fn-expr + later String (?! Broken)
  (param step Int)
  (before
    (print (str "step " step))
  )
  (throw (call Broken "after printing"))
)
```

```output
caught
110
240
-1
step 1
caught
step 2
caught
```

### A catch attached to a call that answers nothing may call a function that answers nothing.

```brix
(main
  (call run)
  (print "after")
)

(fn-do + run _
  (call fail
    (block
      (on-error (: error IOError)
        (call note error)
      )
    )
  )
)

(fn-do + note _
  (param error Error)
  (print (str "noted " (. error message)))
)

(fn-do + fail _
  (throw (call IOError "bad"))
)
```

```output
noted bad
after
```

### unhandled-error prints an error as an unhandled error prints, and the program goes on.

```brix crash
(main
  (call report fail)
  (print "after")
  (throw (call IOError "last"))
)

(fn-do + report _
  (param action Doer)
  (call action
    (block
      (on-error (: error Error)
        (call unhandled-error error)
      )
    )
  )
)

(fn-do + fail _
  (throw (call IOError "first"))
)
```

```stderr
Unhandled error: first
<no file:20>

Task 1

Stack trace:
  0  __source__:fail no file:20:3
  1  __source__:failCasting no file:3:16
  2  __source__:report no file:10:3
  3  __source__:main no file:3:3
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file

Unhandled error: last
<no file:5>

Task 1

Stack trace:
  0  __source__:main no file:5:3
  1  __source__:init.__source__ no file
  2  __source__:brix.main no file
```

### A program replaces unhandled-error, and an error handed to it goes there.

```brix
(main
  (let count 0)
  (= unhandled-error
    (=> (error) :do
      (block
        (+= count 1)
        (print (str "handled " (. error message)))
      )
    )
  )
  (call report fail)
  (call report succeed)
  (print count)
)

(fn-do + report _
  (param action Doer)
  (call action
    (block
      (on-error (: error Error)
        (call unhandled-error error)
      )
    )
  )
)

(fn-do + fail _
  (throw (call IOError "failed"))
)

(fn-do + succeed _
  (print "succeeded")
)
```

```output
handled failed
succeeded
1
```

### An index that a function answers and that raises lets its error out, or a catch holds it.

```brix
(main
  (let t (call Indexed))
  (print (?? (catch? ([] t 2)) -1))
  (print (?? (catch? ([] t 8)) -1))
  (let r (catch ([] t 9)))
  (print (is r Problem))
  (print (call read t 1))
  (print (catch (call read t 5)))
)

(type + Problem (is Error))

(type + Indexed
  (fn-do + (index-get this (param index Int)) Int (?! Problem)
    (if (> index 3)
      (throw (call Problem (str "index " index)))
    )
    (return (* index 2))
  )
)

(fn-do + read Int (?! Problem)
  (param t Indexed)
  (param i Int)
  (return (+ ([] t i) 1))
)
```

```output
4
-1
true
3
index 5
```

### An error an index raises that nothing handles names the index function in its trace.

```brix crash
(main
  (let t (call Indexed))
  (print ([] t 1))
  (print ([] t 9))
)

(type + Problem (is Error))

(type + Indexed
  (fn-do + (index-get this (param index Int)) Int (?! Problem)
    (if (> index 3)
      (throw (call Problem (str "index " index)))
    )
    (return (* index 2))
  )
)
```

```stderr
Unhandled error: index 9
<no file:13>

Task 1

Stack trace:
  0  __source__:Indexed:[] no file:13:7
  1  __source__:main no file:5:10
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

### A handler given as a value is a block of its own: a use read in it ends where it does.

```brix
(main
  (let r
    (call make true
      (block
        (on-error (: err Error)
          (+ (. t n) 10)
          (lazy t (use (call Thing 1)))
        )
      )
    )
  )
  (print (str "r " r))
  (let s
    (call make false
      (block
        (on-error (: err Error)
          (. t n)
          (lazy t (use (call Thing 2)))
        )
      )
    )
  )
  (print (str "s " s))
)

(fn-do + make Int (?! Error)
  (param fails Bool)
  (if fails
    (throw (call Error "bad"))
  )
  (return 5)
)

(type + Thing
  (prop + n _ (= 0))
  (fn-do + This _
    (param n _)
    (print (str "hello " n))
  )
  (fn-expr + begin-using _
    this
  )
  (fn-do + end-using _
    (print (str "bye " n))
  )
)
```

```output
hello 1
bye 1
r 11
s 5
```

### A cast out of a union that may hold an error takes the case it holds.

```brix
(main
  (let a (catch (call (of List Int) (call items true))))
  (print (is a Problem))
  (let b (catch (call (of List Int) (call items false))))
  (print (. (as b (list Int)) length))
  (let c (catch (call first (call items true))))
  (print c)
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

(fn-do + first Int
  (param stream (stream Int))
  (for (in item stream)
    (return item)
  )
  (return 0)
)
```

```output
true
2
1
```

### A transformer whose catcher names a chain of `previous` takes only an error whose chain matches, and sets the `previous` of what it raises.

```brix
(main
  (let a (catch (call load 0)))
  (print a)
  (print (. (as a AppError) previous))
  (let b (catch (call load 1)))
  (print b)
)

(type + AppError (is Error))

(type + ConfigError (is Error))

(fn-do + read Int (?! IOError)
  (param n Int)
  (if (== n 0)
    (throw (call IOError "disk"))
  )
  (return n)
)

(fn-do + parse Int (?! ConfigError)
  (param n Int)
  (throw (call ConfigError "config") (on-error (: err IOError)))
  (blank)
  (if (== n 1)
    (throw (call ConfigError "plain"))
  )
  (return (call read n))
)

(fn-do + load Int (?! AppError) (?! ConfigError)
  (param n Int)
  (throw (call AppError "from io") (on-error (caught (: err ConfigError) (: io IOError))))
  (blank)
  (return (call parse n))
)
```

```output
from io
config
plain
```

### Transformers are tried in the order written: one whose chain matches, else one that takes the error whole.

```brix
(main
  (let a (catch (call load 0)))
  (print a)
  (let b (catch (call load 1)))
  (print b)
)

(type + AppError (is Error))

(type + ConfigError (is Error))

(fn-do + read Int (?! IOError)
  (param n Int)
  (if (== n 0)
    (throw (call IOError "disk"))
  )
  (return n)
)

(fn-do + parse Int (?! ConfigError)
  (param n Int)
  (throw (call ConfigError "config") (on-error (: err IOError)))
  (blank)
  (if (== n 1)
    (throw (call ConfigError "plain"))
  )
  (return (call read n))
)

(fn-do + load Int (?! AppError)
  (param n Int)
  (throw (call AppError (str "wrapped " (. err message))) (on-error (caught (: err ConfigError) (: io IOError))))
  (blank)
  (throw (call AppError (str "plain " (. other message))) (on-error (: other ConfigError)))
  (blank)
  (return (call parse n))
)
```

```output
wrapped config
plain plain
```

### A transformer takes the errors raised after it, and not those raised before it.

```brix
(main
  (print (catch (call load 0)))
  (print (catch (call load 7)))
)

(type + AppError (is Error))

(type + ConfigError (is Error))

(fn-do + read Int (?! IOError)
  (param n Int)
  (if (== n 0)
    (throw (call IOError "disk"))
  )
  (return n)
)

(fn-do + parse Int (?! ConfigError)
  (param n Int)
  (throw (call ConfigError "config") (on-error (: err IOError)))
  (blank)
  (if (== n 1)
    (throw (call ConfigError "plain"))
  )
  (return (call read n))
)

(fn-do + load Int (?! AppError) (?! IOError)
  (param n Int)
  (if (== n 7)
    (throw (call IOError "early"))
  )
  (throw (call AppError "late") (on-error (: err IOError)))
  (blank)
  (if (== n 0)
    (throw (call IOError "raised"))
  )
  (return n)
)
```

```output
late
early
```

### A transformer makes its exception before the cleanups open where the error leaves run.

```brix
(main
  (print (catch (call load 0)))
)

(type + AppError (is Error))

(type + ConfigError (is Error))

(fn-do + read Int (?! IOError)
  (param n Int)
  (if (== n 0)
    (throw (call IOError "disk"))
  )
  (return n)
)

(fn-do + parse Int (?! ConfigError)
  (param n Int)
  (throw (call ConfigError "config") (on-error (: err IOError)))
  (blank)
  (if (== n 1)
    (throw (call ConfigError "plain"))
  )
  (return (call read n))
)

(fn-do + note String
  (param text String)
  (print text)
  (return text)
)

(fn-do + load Int (?! AppError)
  (param n Int)
  (finally
    (print "cleanup")
  )
  (throw (call AppError (call note "making")) (on-error (: err IOError)))
  (blank)
  (return (call read n))
)
```

```output
making
cleanup
making
```

### A transformer in a method takes the error of a call and the error a raise gives where it is written.

```brix
(main
  (print (catch (call (. (call Loader) load) 0)))
  (print (catch (call (. (call Loader) load) 4)))
)

(type + AppError (is Error))

(fn-do + read Int (?! IOError)
  (param n Int)
  (if (== n 0)
    (throw (call IOError "disk"))
  )
  (return n)
)

(type + Loader
  (fn-do + load Int (?! AppError)
    (param n Int)
    (throw (call AppError (str "method " (. err message))) (on-error (: err IOError)))
    (blank)
    (throw (call IOError "conditional") (if (== n 4)))
    (return (call read n))
  )
)
```

```output
method disk
method conditional
```

### A transformer in a generator takes the error an item raises.

```brix
(main
  (print (catch (call collect)))
)

(type + AppError (is Error))

(fn-do + read Int (?! IOError)
  (param n Int)
  (if (== n 0)
    (throw (call IOError "disk"))
  )
  (return n)
)

(fn-do + numbers (stream Int) (?! AppError)
  (throw (call AppError "gen") (on-error (: err IOError)))
  (blank)
  (yield 1)
  (yield (call read 0))
  (yield 3)
)

(fn-do + collect (list Int) (?! AppError)
  (let values (list Int) (call (of List Int)))
  (for (in value (call numbers))
    (+= values value)
  )
  (return values)
)
```

```output
gen
```

### A named handler for a base error type covers every error type of its call.

```brix
(main
  (let text "none")
  (call risky 1
    (block
      (on-error (: failure Error)
        (= text (str failure))
      )
    )
  )
  (print text)
  (call risky 2
    (block
      (on-error (: failure Error)
        (= text (str failure))
      )
    )
  )
  (print text)
  (call risky 3
    (block
      (on-error (: failure Error)
        (= text (str failure))
      )
    )
  )
  (print text)
)

(type + FirstError (is Error))

(type + SecondError (is Error))

(fn-do + risky _ (?! FirstError) (?! SecondError)
  (param value Int)
  (throw (call FirstError "first") (if (== value 1)))
  (throw (call SecondError "second") (if (== value 2)))
)
```

```output
first
second
second
```

### A named handler for a base error type can update a local inside a method.

```brix
(main
  (let holder (call Holder))
  (call (. holder run) 2)
  (print (. holder text))
)

(type + FirstError (is Error))

(type + SecondError (is Error))

(type + Holder
  (prop + text _ (= "none"))
  (fn-do + run _
    (param value Int)
    (let problem (? String))
    (call risky value
      (block
        (on-error (: failure Error)
          (= problem (str failure))
        )
      )
    )
    (= text (?? problem "fine"))
  )
  (fn-do _ risky _ (?! FirstError) (?! SecondError)
    (param value Int)
    (throw (call FirstError "first") (if (== value 1)))
    (throw (call SecondError "second") (if (== value 2)))
  )
)
```

```output
second
```

### A held result can be matched with one named pattern for a base error type.

```brix
(main
  (let result (catch (call risky 2)))
  (print
    (match result
      (case (: value Int)
        (str "value " value)
      )
      (case (: failure Error)
        (str "failed " failure)
      )
    )
  )
)

(type + FirstError (is Error))

(type + SecondError (is Error))

(fn-do + risky Int (?! FirstError) (?! SecondError)
  (param value Int)
  (throw (call FirstError "first") (if (== value 1)))
  (throw (call SecondError "second") (if (== value 2)))
  (return value)
)
```

```output
failed second
```

### A named handler for a base error type can replace the value of its call.

```brix
(main
  (let text
    (call risky 2
      (block
        (on-error (: failure Error)
          (str "failed " failure)
        )
      )
    )
  )
  (print text)
)

(type + FirstError (is Error))

(type + SecondError (is Error))

(fn-do + risky String (?! FirstError) (?! SecondError)
  (param value Int)
  (throw (call FirstError "first") (if (== value 1)))
  (throw (call SecondError "second") (if (== value 2)))
  (return "fine")
)
```

```output
failed second
```

### An action of a match that may panic raises an error its function states.

```brix
(main
  (for (in input (list "12" "9" "x"))
    (print (catch (call parse input)))
  )
)

(fn-expr + parse Int (?! SyntaxError)
  (param text String)
  (match text
    (~ (: digits (+ (chars "0-9")))
      (+ (call check ([] (. digits codepoints) 0)) (. digits length))
    )
    (default
      (inline 0)
    )
  )
)

(fn-expr + check Int (?! SyntaxError)
  (param char Char)
  (if-expr (!= char '9') (as char Int) (call reject "Nine"))
)

(fn-expr + reject Int (?! SyntaxError)
  (param message String)
  (throw (call SyntaxError message))
)
```

```output
51
Nine
0
```

### Statement-level calls with attached guards each bind their own `it`.

```brix
(main
  (print (call checked 2))
  (print (catch (call checked -1)))
)

(fn-do + checked String (?! Error)
  (param second Int)
  (call step 1
    (block
      (throw (call Error "first") (if (== it -1)))
    )
  )
  (call step second
    (block
      (throw (call Error "second") (if (== it -1)))
    )
  )
  (return "ok")
)

(fn-expr + step _
  (param value Int)
  value
)
```

```output
ok
second
```

### An attached catch handles errors from a throwing getter as it does from a call.

```brix
(main
  (let box (call Box))
  (let value
    (scope (. box text)
      (on-error (: error IOError)
        (str "caught " error)
      )
    )
  )
  (print value)
)

(type + Box
  (fn-do + text String :get
    (throw (call IOError "boom"))
  )
)
```

```output
caught boom
```

### An attached catch passes through a null returned by a fallible function with an optional result.

```brix
(main
  (let found
    (call find false
      (block
        (on-error Error
          null
        )
      )
    )
  )
  (print (str (== found null)))
)

(type + Box)

(fn-do + find (? Box) (?! Error)
  (param fails Bool)
  (throw (call Error "lost") (if fails))
  (return null)
)
```

```output
true
```

### A call through an optional chain may fail, and a catch attached to it takes its errors. Where the chain makes no call nothing is raised, and the chain gives nothing.

```brix
(main
  (let gate (? Gate) (call Gate))
  (let none (? Gate) null)
  (call (?. gate shut)
    (block
      (on-error (: failure Error)
        (print (str "caught " (. failure message)))
      )
    )
  )
  (call (?. none shut)
    (block
      (on-error (: failure Error)
        (print (str "caught " (. failure message)))
      )
    )
  )
  (let count
    (call (?. gate count)
      (block
        (on-error Error
          -1
        )
      )
    )
  )
  (print (str count))
  (let missing
    (call (?. none count)
      (block
        (on-error Error
          -1
        )
      )
    )
  )
  (print (str (== missing null)))
)

(type + Gate
  (fn-do + shut _ (?! Error)
    (throw (call Error "stuck"))
  )
  (fn-do + count Int (?! Error)
    (throw (call Error "bad"))
  )
)
```

```output
caught stuck
-1
true
```

### A function that states an error may let out an error of a class above it, which converts to the error it states.

```brix
(main
  (let found (catch (call open-it 3)))
  (print (is found I64))
)

(type + BadPath (is Error))

(fn-do + open-it I64 (?! BadPath)
  (param seed I64)
  (let found (call risky seed))
  (return (+ found 1))
)

(fn-do + risky I64
  (param seed I64)
  (if (< seed 0)
    (throw (call Error "negative"))
  )
  (return seed)
)
```

```output
true
```

### A handler whose class can match neither the value nor an error of the call is passed over.

```brix
(main
  (for (in n (call events false))
    (print n)
  )
  (let failed (catch (call first-event true)))
  (print (is failed Wrapped))
)

(type + Wrapped (is Error))

(fn-do + fetch Int (?! Wrapped)
  (param fail Bool)
  (throw (call Wrapped "bad") (if fail))
  (return 4)
)

(fn-do + first-event Int (?! Wrapped)
  (param fail Bool)
  (for (in n (call events fail))
    (return n)
  )
  (return 0)
)

(fn-do + events (stream Int)
  (param fail Bool)
  (let response
    (call fetch fail
      (block
        (on-error (: canceled TaskCancelationError)
          (throw (call Wrapped (str canceled)))
        )
      )
    )
  )
  (yield response)
  (yield (+ response 1))
)
```

```output
4
5
true
```

### An error of a class above the one a function states is cast to that one as it leaves, once the cleanups open have run.

```brix
(main
  (do-new
    (print (?? (catch? (call risky 1)) -1))
    (print (?? (catch? (call risky 0)) -1))
    (on-failure failure
      (print failure)
    )
  )
)

(type + Failed (is Error))

(fn-do + risky I32 (?! Failed)
  (param count I32)
  (finally
    (print (str "cleanup " count))
  )
  (if (== count 0)
    (throw (call Error "plain"))
  )
  (return count)
)
```

```output
cleanup 1
1
cleanup 0
Invalid cast to [Object __source__:Failed]
```

### A function that raises errors may return an error as its value.

```brix
(main
  (print (?? (catch? (call describe 3)) "failed"))
  (print (?? (catch? (call describe -1)) "failed"))
  (print (?? (catch? (call describe 0)) "failed"))
)

(type + CodeError (is Error)
  (prop + code I64)
  (fn-do + This _
    (param code _)
    (= message (str "code " code))
  )
)

(fn-expr _ count-of I64 (?! CodeError)
  (param n I64)
  (before
    (throw (call CodeError n) (if (< n 0)))
  )
  (* n 2)
)

(fn-expr _ make _ (?! CodeError)
  (param n I64)
  (call CodeError
    (block
      (arg (call count-of n))
    )
  )
)

(fn-do _ describe String (?! CodeError)
  (param n I64)
  (let made (call make n))
  (return (str (. made message) " " (. made code)))
)
```

```output
code 6 6
failed
code 0 0
```

### An error of a class above those a function raises leaves it cast to the first of them below it, in their order, not as written.

```brix
(main
  (print (call describe 0))
  (print (call describe 1))
)

(type + AErr (is Error))

(type + BErr (is Error))

(fn-do _ raises _ (?! Error)
  (param n I64)
  (if (== n 1)
    (throw (call AErr "a"))
  )
)

(fn-do _ relay String (?! BErr) (?! AErr)
  (param n I64)
  (call raises n)
  (return (str "ok " n))
)

(fn-do _ describe String
  (param n I64)
  (let got (catch (call relay n)))
  (return
    (match got
      (case AErr
        "A"
      )
      (case BErr
        "B"
      )
      (case (: text String)
        text
      )
    )
  )
)
```

```output
ok 0
A
```

### A conditional return of null from a function that returns a union with a Null case converts the null to the union.

```brix
(main
  (print (call describe (call pick 0)))
  (print (call describe (call pick 1)))
  (print (call describe (call pick 2)))
)

(type + Reading (is Union)
  (variant Null)
  (variant I64)
  (variant String)
)

(fn-do _ pick Reading
  (param n I64)
  (return-if null (if (== n 0)))
  (return-if "text" (if (== n 1)))
  (return n)
)

(fn-expr _ describe String
  (param value Reading)
  (case? (is value Null)
    "nothing"
  )
  (case? (is value String)
    (str "text " (as value String))
  )
  (default
    (str "number " (as value I64))
  )
)
```

```output
nothing
text text
number 2
```

### A raise guarded by a condition is typed under what the condition finds: an optional error the condition tests is raised as the error it holds.

```brix
(main
  (let outcome (catch (call checked (call Error "boom"))))
  (print (is outcome Error))
  (print (call checked null))
)

(fn-do _ checked I64 (?! Error)
  (param failure (? Error))
  (throw failure (if failure))
  (return 1)
)
```

```output
true
1
```

### A handler whose body is a block of two statements that give values, such as an assignment followed by a value, gives no value, so it can not stand for an expression that gives one.

```brix crash
(main
  (let problem "")
  (let value
    (call parse -1
      (block
        (on-error (: failure Error)
          (= problem (. failure message))
          0
        )
      )
    )
  )
  (print (str value " " problem))
)

(fn-do _ parse I64 (?! Error)
  (param n I64)
  (throw (call Error "negative") (if (< n 0)))
  (return n)
)
```

```output
Exception: Illegal statement
<no file:1>
    
    ^
```

### A handler names the error type it takes, so a name alone is not a handler

```brix crash
(main
  (print (call run true))
)

(fn-do + run String
  (param fails Bool)
  (let items
    (call read fails
      (block
        (on-error other
          (return (str "other " (. other message)))
        )
      )
    )
  )
  (return (str "read " (. items length)))
)

(fn-do + read (list Int) (?! Error)
  (param fails Bool)
  (throw (call Error "nope") (if fails))
  (return (list 1 2))
)
```

```output
Exception: Attached catch handler type must be an Error subtype
<no file:11>
            (on-error other
                      ^^^^^
```

### A getter can declare an error it raises, which a read of the property handles.

```brix
(main
  (print
    (scope (. (call Box -1) size)
      (on-error (: error TypeError)
        -1
      )
    )
  )
  (print
    (scope (. (call Box 3) size)
      (on-error (: error TypeError)
        -1
      )
    )
  )
)

(type + Box
  (prop + raw Int)
  (fn-do + This _
    (param raw Int)
  )
  (fn-do + size Int :get (?! TypeError)
    (throw (call TypeError) (if (< raw 0)))
    (return raw)
  )
)
```

```output
-1
3
```
