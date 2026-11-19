# Match exhaustiveness

```brix
(main
  (let result
    (match true
      (case= true
        1
      )
      (case= false
        0
      )
    )
  )
  (print (type-of result))
  (print (+ result 1))
)
```

```output
[TypeInfo I64]
2
```

```brix crash
(main
  (match true
    (case= true
      (print "true")
    )
  )
)
```

```output
Exception: This match on Bool does not handle false; add "(case= false …)", or "(default …)" for the rest
<no file:4>
        (case= true
        ^^^^^^^^^^
```

### Held Results require every Error channel.

```brix crash
(main
  (let result
    (match (catch (call load 0))
      (case (: config Config)
        (. config value)
      )
      (case (: err FileNotFound)
        "not found"
      )
    )
  )
  (print result)
)

(type + Config
  (prop + value String)
  (fn-do + This _
    (param value _)
  )
)

(type + FileNotFound (is Error))

(type + PermissionDenied (is Error))

(fn-do + load Config (?! FileNotFound) (?! PermissionDenied)
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
Exception: This match on Result does not handle PermissionDenied; add a case for it, or "(default …)" for the rest
<no file:5>
          (case (: config Config)
          ^^^^^^^^^^^^^^^^^^^^^^
```

### Held Results require their success channel.

```brix crash
(main
  (let result
    (match (catch (call load 1))
      (case (: err FileNotFound)
        "not found"
      )
      (case (: err PermissionDenied)
        "denied"
      )
    )
  )
  (print result)
)

(type + Config
  (prop + value String)
  (fn-do + This _
    (param value _)
  )
)

(type + FileNotFound (is Error))

(type + PermissionDenied (is Error))

(fn-do + load Config (?! FileNotFound) (?! PermissionDenied)
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
Exception: This match on Result does not handle Config; add a case for it, or "(default …)" for the rest
<no file:5>
          (case (: err FileNotFound)
          ^^^^^^^^^^^^^^^^^^^^^^^^^
```

### Exhaustive held Results evaluate only the selected success or Error arm.

```brix
(main
  (print (call inspect 0))
  (print (call inspect 2))
)

(type + Config
  (prop + value String)
  (fn-do + This _
    (param value _)
  )
)

(type + FileNotFound (is Error))

(type + PermissionDenied (is Error))

(fn-do + load Config (?! FileNotFound) (?! PermissionDenied)
  (param outcome Int)
  (if (== outcome 1)
    (throw (call FileNotFound "missing"))
  )
  (if (== outcome 2)
    (throw (call PermissionDenied "denied"))
  )
  (return (call Config "loaded"))
)

(fn-expr + inspect String
  (param outcome Int)
  (match (catch (call load outcome))
    (case (: config Config)
      (. config value)
    )
    (case (: err FileNotFound)
      "not found"
    )
    (case (: err PermissionDenied)
      "permission denied"
    )
  )
)
```

```output
loaded
permission denied
```

### A typed arm that propagates its Error still covers that Result channel.

```brix
(main
  (print (catch (call inspect 1)))
)

(type + Config
  (prop + value String)
  (fn-do + This _
    (param value _)
  )
)

(type + FileNotFound (is Error))

(type + PermissionDenied (is Error))

(fn-do + load Config (?! FileNotFound) (?! PermissionDenied)
  (param outcome Int)
  (if (== outcome 1)
    (throw (call FileNotFound "missing"))
  )
  (if (== outcome 2)
    (throw (call PermissionDenied "denied"))
  )
  (return (call Config "loaded"))
)

(fn-do + inspect String (?! FileNotFound)
  (param outcome Int)
  (return
    (match (catch (call load outcome))
      (case (: config Config)
        (. config value)
      )
      (case (: err FileNotFound)
        (throw err)
      )
      (case (: err PermissionDenied)
        "permission denied"
      )
    )
  )
)
```

```output
missing
```

### Predicate-only arms still check the held Result domain.

```brix crash
(main
  (let result
    (match (catch (call load))
      (case? false
        "never"
      )
    )
  )
  (print result)
)

(type + Config)

(type + FileNotFound (is Error))

(type + PermissionDenied (is Error))

(fn-do + load Config (?! FileNotFound) (?! PermissionDenied)
  (return (call Config))
)
```

```output
Exception: This match on Result does not handle Config, FileNotFound and PermissionDenied; add a case for each, or "(default …)" for the rest
<no file:5>
          (case? false
          ^^^^^^^^^^^
```

### Error supertypes, unconditional remainders, and optional matches retain their coverage rules.

```brix
(main
  (print (call inspect-with-supertype 2))
  (print (call inspect-with-remainder 1))
  (let partial
    (match? (catch (call load 2))
      (case (: config Config)
        (. config value)
      )
    )
  )
  (print (?? partial "none"))
)

(type + Config
  (prop + value String)
  (fn-do + This _
    (param value _)
  )
)

(type + FileNotFound (is Error))

(type + PermissionDenied (is Error))

(fn-do + load Config (?! FileNotFound) (?! PermissionDenied)
  (param outcome Int)
  (if (== outcome 1)
    (throw (call FileNotFound "missing"))
  )
  (if (== outcome 2)
    (throw (call PermissionDenied "denied"))
  )
  (return (call Config "loaded"))
)

(fn-expr + inspect-with-supertype String
  (param outcome Int)
  (match (catch (call load outcome))
    (case (: config Config)
      (. config value)
    )
    (case Error
      "error"
    )
  )
)

(fn-expr + inspect-with-remainder String
  (param outcome Int)
  (match (catch (call load outcome))
    (case (: config Config)
      (. config value)
    )
    (default
      "remainder"
    )
  )
)
```

```output
error
remainder
none
```

### A Result success union remains one channel, but its alternatives can be covered together.

```brix
(main
  (print (call inspect-choice false))
  (print (call inspect-choice true))
)

(type + Config
  (prop + value String)
  (fn-do + This _
    (param value _)
  )
)

(type + Choice (is Union)
  (variant Config)
  (variant String)
)

(type + PermissionDenied (is Error))

(fn-do + load-choice Choice (?! PermissionDenied)
  (param as-text Bool)
  (if as-text
    (return "text")
  )
  (return (call Config "config"))
)

(fn-expr + inspect-choice String
  (param as-text Bool)
  (match (catch (call load-choice as-text))
    (case (: config Config)
      (. config value)
    )
    (case (: text String)
      text
    )
    (case (: err PermissionDenied)
      "denied"
    )
  )
)
```

```output
config
text
```

### One alternative does not cover an entire union-valued success channel.

```brix crash
(main
  (let result
    (match (catch (call load-choice))
      (case (: config Config)
        (. config value)
      )
      (case (: err PermissionDenied)
        "denied"
      )
    )
  )
  (print result)
)

(type + Config
  (prop + value String)
  (fn-do + This _
    (param value _)
  )
)

(type + Choice (is Union)
  (variant Config)
  (variant String)
)

(type + PermissionDenied (is Error))

(fn-do + load-choice Choice (?! PermissionDenied)
  (return (call Config "config"))
)
```

```output
Exception: This match on Result does not handle Choice; add a case for it, or "(default …)" for the rest
<no file:5>
          (case (: config Config)
          ^^^^^^^^^^^^^^^^^^^^^^
```

```brix crash
(main
  (let result
    (match true
      (case= true
        1
      )
    )
  )
  (print result)
)
```

```output
Exception: This match on Bool does not handle false; add "(case= false …)", or "(default …)" for the rest
<no file:5>
          (case= true
          ^^^^^^^^^^
```

```brix
(main
  (let value (? Int) 41)
  (let result
    (match value
      (case (: n Int)
        (+ n 1)
      )
      (case= null
        0
      )
    )
  )
  (print (type-of result))
  (print (+ result 1))
)
```

```output
[TypeInfo I64]
43
```

```brix
(main
  (let value (? Int))
  (let result
    (match value
      (case (: n Int)
        (+ n 1)
      )
      (case= null
        0
      )
    )
  )
  (print result)
)
```

```output
0
```

```brix crash
(main
  (let value (? Int) 41)
  (let result
    (match value
      (case (: n Int)
        (+ n 1)
      )
    )
  )
  (print result)
)
```

```output
Exception: This match on I64? does not handle null; add "(case= null …)", or "(default …)" for the rest
<no file:6>
          (case (: n Int)
          ^^^^^^^^^^^^^^
```

```brix crash
(main
  (let value (? Int))
  (let result
    (match value
      (case= null
        0
      )
    )
  )
  (print result)
)
```

```output
Exception: This match on I64? handles only null; add a case for a present value, as in "(case (: value I64) …)", or "(default …)" for the rest
<no file:6>
          (case= null
          ^^^^^^^^^^
```

```brix
(main
  (let value (call (. Result good) 41))
  (let result
    (match value
      (case (call .good n)
        (+ n 1)
      )
      (case .bad
        0
      )
    )
  )
  (print (type-of result))
  (print (+ result 1))
)

(type + Result (is Union)
  (variant good (tuple (: n Int)))
  (variant bad)
)
```

```output
[TypeInfo I64]
43
```

```brix crash
(main
  (let value (call (. Result good) 41))
  (let result
    (match value
      (case (call .good n)
        (+ n 1)
      )
    )
  )
  (print result)
)

(type + Result (is Union)
  (variant good (tuple (: n Int)))
  (variant bad)
)
```

```output
Exception: This match on Result does not handle .bad; add a case for it, or "(default …)" for the rest
<no file:6>
          (case (call .good n)
          ^^^^^^^^^^^^^^^^^^^
```

```brix
(main
  (let values
    (call List
      (block
        (item (call (. Result good) 1))
        (item (. Result bad))
        (pipe ?*>
          (match? each
            (case (call .good n)
              n
            )
          )
        )
      )
    )
  )
  (print values)
)

(type + Result (is Union)
  (variant good (tuple (: n Int)))
  (variant bad)
)
```

```output
1
```

```brix
(main
  (let event
    (match true
      (case= true
        (call ..ok 41)
      )
      (default
        ..failed
      )
    )
  )
  (let result
    (match event
      (case (call .ok n)
        (+ n 1)
      )
      (case .failed
        0
      )
    )
  )
  (print (type-of result))
  (print (+ result 1))
)
```

```output
[TypeInfo I64]
43
```

```brix
(main
  (let value Choice 41)
  (let result
    (match value
      (case Int
        1
      )
      (case String
        2
      )
    )
  )
  (print (type-of result))
  (print (+ result 1))
)

(type + Choice (is Union)
  (variant Int)
  (variant String)
)
```

```output
[TypeInfo I64]
2
```

```brix
(main
  (let value Mode (. Mode beta))
  (let result
    (match value
      (case= (. Mode alpha)
        1
      )
      (case= (. Mode beta)
        2
      )
    )
  )
  (print (type-of result))
  (print (+ result 1))
)

(type + Mode (is Int)
  (variant alpha)
  (variant beta)
)
```

```output
[TypeInfo I64]
3
```

```brix
(main
  (let value "x")
  (let result
    (match value
      (case= "x"
        1
      )
    )
  )
  (print (type-of result))
  (print (+ result 1))
)
```

```output
[TypeInfo I64]
2
```

```brix
(main
  (let value "x")
  (let result
    (match? value
      (case= "x"
        1
      )
    )
  )
  (print (type-of result))
  (print (?? result 0))
)
```

```output
[TypeInfo Optional'I64]
1
```

```brix
(main
  (let result
    (match? true
      (case= true
        1
      )
    )
  )
  (print (type-of result))
  (print (?? result 0))
)
```

```output
[TypeInfo Optional'I64]
1
```

```brix
(main
  (let result
    (match true
      (case= true
        1
      )
      (default
        0
      )
    )
  )
  (print (type-of result))
  (print (+ result 1))
)
```

```output
[TypeInfo I64]
2
```

### List shape patterns nest inside union case patterns.

```brix
(main
  (print (call describe (call (. Tree branch) "let" (list (call (. Tree leaf) "total") (call (. Tree leaf) "Int")))))
  (print (call describe (call (. Tree branch) "set" (list (call (. Tree leaf) "total")))))
  (print (call describe (call (. Tree branch) "run" (call (of List Tree)))))
  (print (call describe (call (. Tree leaf) "total")))
)

(type + Tree (is Union)
  (variant leaf String)
  (variant branch (tuple (: name String) (: items (list Tree))))
)

(fn-expr + describe String
  (param tree Tree)
  (match tree
    (case (call .branch "let" (list (call .leaf name) ...rest))
      (str "let " name " with " (. rest length) " more")
    )
    (case (call .branch name (list only))
      (str name " with one item")
    )
    (case (call .branch name (list))
      (str name " alone")
    )
    (case (call .leaf text)
      (str "leaf " text)
    )
    (default
      "other"
    )
  )
)
```

```output
let total with 1 more
set with one item
run alone
leaf total
```

### A remainder-only list pattern covers its union case.

```brix
(main
  (print (call count (call (. Tree branch) "let" (list (call (. Tree leaf) "total") (call (. Tree leaf) "Int")))))
  (print (call count (call (. Tree leaf) "total")))
)

(type + Tree (is Union)
  (variant leaf String)
  (variant branch (tuple (: name String) (: items (list Tree))))
)

(fn-expr + count Size
  (param tree Tree)
  (match tree
    (case (call .branch _ (list ...items))
      (. items length)
    )
    (case (call .leaf _)
      0
    )
  )
)
```

```output
2
0
```

### A list shape pattern with required items does not cover its union case.

```brix crash
(main
  (print (call count (call (. Tree branch) "let" (list (call (. Tree leaf) "total")))))
)

(type + Tree (is Union)
  (variant leaf String)
  (variant branch (tuple (: name String) (: items (list Tree))))
)

(fn-expr + count Size
  (param tree Tree)
  (match tree
    (case (call .branch _ (list first ...))
      1
    )
    (case (call .leaf _)
      0
    )
  )
)
```

```output
Exception: This match on Tree does not handle .branch; add a case for it, or "(default …)" for the rest
<no file:14>
        (case (call .branch _ (list first ...))
        ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### An interface pattern checks coverage of a Null case without compiling its converter.

```brix
(main
  (let text Body "x")
  (print (call bar-of text))
  (let empty Body null)
  (print (call bar-of empty))
  (let other Body (call Other))
  (print (call bar-of other))
)

(type + Proto (is Interface)
  (fn-expr + bar Int :get)
)

(type + Other
  (implements Proto)
  (fn-expr + bar _ :get
    7
  )
)

(type + Body (is Union)
  (variant Null)
  (variant String)
  (variant Proto)
)

(fn-expr + bar-of Int
  (param body Body)
  (match body
    (case (: proto Proto)
      (. proto bar)
    )
    (default
      0
    )
  )
)
```

```output
0
0
7
```

### A type written in brackets in an arm of a match on a result covers the list the result holds.

```brix
(main
  (print (call chosen 2))
  (print (call chosen 0))
)

(fn-do + chosen String
  (param id U64)
  (let result (catch (call copies-of id)))
  (match result
    (case (: error Error)
      (return "error")
    )
    (case (: found (list U64))
      (return (str "found " (. found length)))
    )
  )
)

(fn-do + copies-of (list U64) (?! Error)
  (param id U64)
  (throw (call Error "none") (if (== id 0)))
  (return (call (of List U64) id id id))
)
```

```output
found 3
error
```

### Arms of a case that holds a list take the case together when one takes some count of items and a rest and others take each smaller count.

```brix
(main
  (let empty (list Int))
  (print (call count (call .many empty)))
  (print (call count (call .many (list 5 6))))
  (print (call count .none))
)

(fn-expr + count Int
  (param bag Bag)
  (match bag
    (case .none
      0
    )
    (case (call .many (list))
      -1
    )
    (case (call .many (list first ...rest))
      (+ first (. rest length))
    )
  )
)

(type + Bag (is Union)
  (variant none)
  (variant many (list Int))
)
```

```output
-1
6
0
```

### Arms of a list that leave out a count of items do not take the case.

```brix crash
(main
  (print (call count (call .many (list 1))))
)

(fn-expr + count Int
  (param bag Bag)
  (match bag
    (case .none
      0
    )
    (case (call .many (list))
      -1
    )
    (case (call .many (list first second ...rest))
      first
    )
  )
)

(type + Bag (is Union)
  (variant none)
  (variant many (list Int))
)
```

```output
Exception: This match on Bag does not handle .many; add a case for it, or "(default …)" for the rest
<no file:9>
        (case .none
        ^^^^^^^^^^
```

### `true` and `false` in a pattern are literals, so they match an equal value, as `|= true` does.

```brix
(main
  (let flag false)
  (let result
    (match flag
      (case true
        "yes"
      )
      (case false
        "no"
      )
    )
  )
  (print result)
  (print (call classify false true))
  (print (call classify true false))
  (print (call classify true true))
  (print (call named 3))
)

(fn-expr _ classify String
  (param valid Bool)
  (param many Bool)
  (match (tuple valid many)
    (case (tuple false _)
      "invalid"
    )
    (case (tuple _ false)
      "few"
    )
    (default
      "many"
    )
  )
)

(fn-expr _ named String
  (param true-count Int)
  (match true-count
    (case falsey
      (str "n=" falsey)
    )
  )
)
```

```output
no
invalid
few
many
n=3
```

```brix crash
(main
  (match true
    (case true
      (print "true")
    )
  )
)
```

```output
Exception: This match on Bool does not handle false; add "(case= false …)", or "(default …)" for the rest
<no file:4>
        (case true
        ^^^^^^^^^
```
