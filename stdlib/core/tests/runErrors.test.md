# Errors

```yumi crash
read()

━━━━━━━━━━━━━━━━━━━━
+ do read ?! IOError

?! IOError("bad")
```

```stderr
Unhandled error: bad
<no file:7>

Task 1

Stack trace:
  0  __source__:read no file:7:1
  1  __source__:main no file:2:1
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

### GC-free exceptional paths retain normal Error propagation, cleanup, and caught continuation.

```yumi
-- gc-free-propagate(true)
-- catch gc-free-propagate(false)
-- gc-free-cleanups
-- gc-free-catch(false)
-- gc-free-catch(true)
-- catch gc-free-catch-replace(false)
-- gc-free-catch-replace(true)

_ gc-free-cleanups = 0

━━━━━━━━━━━━━━━━━━━━
+ GCFreeFailure is Error

━━━━━━━━━━━━━━━━━━━━
+ GCFreeReplacement is Error

━━━━━━━━━━━━━━━━━━━━
+ do gc-free-fallible Int ?! GCFreeFailure without gc
    - succeeds Bool

if succeeds
    = 7
?! GCFreeFailure("failed")

━━━━━━━━━━━━━━━━━━━━
+ do gc-free-propagate Int ?! GCFreeFailure without gc
    - succeeds Bool

do finally
    gc-free-cleanups += 1
= gc-free-fallible(succeeds) + 1

━━━━━━━━━━━━━━━━━━━━
+ do gc-free-catch Int without gc
    - succeeds Bool

. value = gc-free-fallible(succeeds)
    catch GCFreeFailure
        40
= value + 2

━━━━━━━━━━━━━━━━━━━━
+ do gc-free-catch-replace Int ?! GCFreeReplacement without gc
    - succeeds Bool

. value = gc-free-fallible(succeeds)
    catch GCFreeFailure
        ?! GCFreeReplacement("replaced")
= value
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

```yumi
-- inspect-joined(false)
-- inspect-joined(true)

━━━━━━━━━━━━━━━━━━━━
+ Config

+ value String

────────────────────
+ This
    - value

━━━━━━━━━━━━━━━━━━━━
+ FileNotFound is Error

━━━━━━━━━━━━━━━━━━━━
+ select-joined
    - fail Bool

match fail
    |= true
        FileNotFound("missing")
    ??
        Config("loaded")

━━━━━━━━━━━━━━━━━━━━
+ do inspect-joined String
    - fail Bool

. joined = catch select-joined(fail)

= match joined
    | config Config
        config.value
    | FileNotFound
        "file error"
```

```output
loaded
file error
```

### An unheld match join auto-propagates its inferred Error channel.

```yumi
-- catch? inspect-joined(false) ?? "propagated"
-- catch? inspect-joined(true) ?? "propagated"

━━━━━━━━━━━━━━━━━━━━
+ Config

+ value String

────────────────────
+ This
    - value

━━━━━━━━━━━━━━━━━━━━
+ FileNotFound is Error

━━━━━━━━━━━━━━━━━━━━
+ do inspect-joined String ?! FileNotFound
    - fail Bool

. config = match fail
    |= true
        FileNotFound("missing")
    ??
        Config("loaded")
= config.value
```

```output
loaded
propagated
```

### Handling every original error with ordinary values produces an ordinary expression type.

```yumi
. config = read-config(2)
    catch FileNotFound
        Config("file default")
    catch PermissionDenied
        Config("permission default")
-- type-of config
-- config.value

━━━━━━━━━━━━━━━━━━━━
+ Config

+ value String

────────────────────
+ This
    - value

━━━━━━━━━━━━━━━━━━━━
+ FileNotFound is Error

━━━━━━━━━━━━━━━━━━━━
+ PermissionDenied is Error

━━━━━━━━━━━━━━━━━━━━
+ do read-config Config ?! FileNotFound ?! PermissionDenied
    - outcome Int

if outcome == 1
    ?! FileNotFound("missing")
if outcome == 2
    ?! PermissionDenied("denied")
= Config("loaded")
```

```output
[TypeInfo Config]
permission default
```

### A Result-valued arm flattens into the inferred match Result.

```yumi
-- inspect-flattened(true, false)
-- inspect-flattened(false, true)
-- inspect-flattened(false, false)

━━━━━━━━━━━━━━━━━━━━
+ do read Int ?! IOError
    - ok Bool

if ok
    = 22
else
    ?! IOError("disk")

━━━━━━━━━━━━━━━━━━━━
+ select-flattened
    - outer Bool
    - inner Bool

match outer
    |= true
        11
    ??
        catch read(inner)

━━━━━━━━━━━━━━━━━━━━
+ do inspect-flattened String
    - outer Bool
    - inner Bool

. joined = catch select-flattened(outer, inner)

= match joined
    | value Int
        "$value"
    | IOError
        "io error"
```

```output
11
22
io error
```

### Bare-Error-only joins use Void success and need only Error arms when held.

```yumi
-- inspect-errors(false)
-- inspect-errors(true)

━━━━━━━━━━━━━━━━━━━━
+ FileNotFound is Error

━━━━━━━━━━━━━━━━━━━━
+ PermissionDenied is Error

━━━━━━━━━━━━━━━━━━━━
+ do select-error ?! FileNotFound ?! PermissionDenied
    - denied Bool

match denied
    |= true
        PermissionDenied("denied")
    ??
        FileNotFound("missing")

━━━━━━━━━━━━━━━━━━━━
+ do inspect-errors String
    - denied Bool

. joined = catch select-error(denied)

= match joined
    | FileNotFound
        "file error"
    | PermissionDenied
        "permission error"
```

```output
file error
permission error
```

### A ?! arm returns immediately and contributes no type to the join.

```yumi
-- catch? inspect-return(false) ?? "returned"
-- catch? inspect-return(true) ?? "returned"

━━━━━━━━━━━━━━━━━━━━
+ Config

+ value String

────────────────────
+ This
    - value

━━━━━━━━━━━━━━━━━━━━
+ PermissionDenied is Error

━━━━━━━━━━━━━━━━━━━━
+ do inspect-return String ?! PermissionDenied
    - fail Bool

. selected = match fail
    |= true
        ?! PermissionDenied("denied")
    ??
        Config("loaded")
-- type-of selected
= selected.value
```

```output
[TypeInfo Config]
loaded
returned
```

### Error results propagate through collection expressions and relayed generators.

```yumi
. list-result = catch list-values()
-- "list error ${list-result is StreamProblem}"

. stream-result = catch stream-values()
-- "stream error ${stream-result is StreamProblem}"

━━━━━━━━━━━━━━━━━━━━
+ StreamProblem is Error

━━━━━━━━━━━━━━━━━━━━
_ do encode-value Int ?! StreamProblem
    - value Int

if value == 0
    = value
?! StreamProblem("bad")

━━━━━━━━━━━━━━━━━━━━
_ list-values [Int] ?! StreamProblem

encode-value(each) in [1]

━━━━━━━━━━━━━━━━━━━━
_ do failing-items <Int>

<= 1
?! StreamProblem("bad")

━━━━━━━━━━━━━━━━━━━━
_ do relayed-items <Int>

for item in failing-items()
    <= item

━━━━━━━━━━━━━━━━━━━━
_ do stream-values [Int] ?! StreamProblem

. values [Int] = List'Int()
for item in relayed-items()
    values += item
= values
```

```output
list error true
stream error true
```

### A fallible feed pipeline can be collected through a named tuple stream.

```yumi
. values = Map'String'Float
    • one 1.0
    • zero 0.0
. answer = ProbabilityAnswer
    - require-probabilities(values)

-- answer.probabilities.length
-- answer.probabilities["one"]
-- answer.probabilities["zero"]

━━━━━━━━━━━━━━━━━━━━
+ ProbabilityError is Error

━━━━━━━━━━━━━━━━━━━━
+ ProbabilityAnswer is Tuple

+ probabilities [String = Float]

━━━━━━━━━━━━━━━━━━━━
_ require-probability Float ?! ProbabilityError
    - value Float

?! ProbabilityError("invalid")
    if value < 0 || value > 1

value

━━━━━━━━━━━━━━━━━━━━
_ require-probabilities ?! ProbabilityError
    - values [String = Float]

** values
*> each[0], require-probability(each[1])
|> Map
```

```output
2
1
0
```

### Error results propagate when a collection expression becomes a union's list case.

```yumi
. success = union-list-values(1)
-- "union list success ${success is [I32]}"
. values = success as [I32]
-- "union list length ${values.length}"
-- "union list first ${values[0]}"

. result = catch union-list-values(0)
-- "union list error ${result is UnionListProblem}"

━━━━━━━━━━━━━━━━━━━━
+ UnionListProblem is Error

━━━━━━━━━━━━━━━━━━━━
+ UnionListPayload is Union

| number I32
| values [I32]

━━━━━━━━━━━━━━━━━━━━
_ do union-list-value I32 ?! UnionListProblem
    - value I32

if value == 0
    ?! UnionListProblem("bad")
= value

━━━━━━━━━━━━━━━━━━━━
_ union-list-values UnionListPayload ?! UnionListProblem
    - value I32

union-list-value(each) in [value]
```

```output
union list success true
union list length 1
union list first 1
union list error true
```

### Returning a held Result fuses its success and Error into the caller-visible Result.

```yumi
-- catch relay(true)
-- catch relay(false)

━━━━━━━━━━━━━━━━━━━━
+ do read Int ?! IOError
    - ok Bool

if ok
    = 7
else
    ?! IOError("read failed")

━━━━━━━━━━━━━━━━━━━━
+ do relay
    - ok Bool

= catch read(ok)
```

```output
7
read failed
```

### Ordinary, direct-Error, and held-Result returns share one outgoing Result.

```yumi
-- inspect-relay(0)
-- inspect-relay(1)
-- inspect-relay(2)

━━━━━━━━━━━━━━━━━━━━
+ OuterError is Error

━━━━━━━━━━━━━━━━━━━━
+ do read Int ?! IOError

?! IOError("inner")

━━━━━━━━━━━━━━━━━━━━
+ do relay
    - mode Int

if mode == 0
    = 42
if mode == 1
    ?! OuterError("outer")
= catch read()

━━━━━━━━━━━━━━━━━━━━
+ do inspect-relay String
    - mode Int

= match catch relay(mode)
    | value Int
        "$value"
    | OuterError
        "outer error"
    | IOError
        "inner error"
```

```output
42
outer error
inner error
```

### Deferred cleanup runs exactly once for either case of a returned held Result.

```yumi
_ cleanups = 0

-- catch relay(true)
-- cleanups
-- catch relay(false)
-- cleanups

━━━━━━━━━━━━━━━━━━━━
+ do read Int ?! IOError
    - ok Bool

if ok
    = 7
else
    ?! IOError("read failed")

━━━━━━━━━━━━━━━━━━━━
+ do relay
    - ok Bool

do finally
    cleanups += 1
= catch read(ok)
```

```output
7
1
read failed
2
```

### A declared success type validates against the fused Result success channel.

```yumi
-- catch relay(true)
-- catch relay(false)

━━━━━━━━━━━━━━━━━━━━
+ do read Int ?! IOError
    - ok Bool

if ok
    = 7
else
    ?! IOError("read failed")

━━━━━━━━━━━━━━━━━━━━
+ do relay Int ?! IOError
    - ok Bool

= catch read(ok)
```

```output
7
read failed
```

```yumi
-- catch foo(10)
-- catch foo(0)

-- type-of foo(10)
-- type-of catch foo(10)

━━━━━━━━━━━━━━━━━━━━
+ foo
    - n

?! Error("Divide by zero")
    if n == 0

n / 2
```

```output
5
Divide by zero
[TypeInfo I64]
[TypeInfo Union]
```

```yumi
-- catch? foo(10)
-- catch? foo(0)

-- type-of foo(10)
-- type-of catch? foo(10)

━━━━━━━━━━━━━━━━━━━━
+ foo
    - n

?! Error("Divide by zero")
    if n == 0

n / 2
```

```output
5
unknown
[TypeInfo I64]
[TypeInfo Optional'I64]
```

```yumi
catch? foo(0)
catch? foo(10)

━━━━━━━━━━━━━━━━━━━━
+ do foo
    - n

?! Error("Divide by zero")
    if n == 0

-- n / 2
```

```output
5
```

```yumi
_ hits = 0

catch bump()
-- hits

━━━━━━━━━━━━━━━━━━━━
+ do bump

hits += 1
```

```output
1
```

```yumi
_ hits = 0

catch bump()
-- hits

━━━━━━━━━━━━━━━━━━━━
+ do bump ?! IOError

hits += 1
```

```output
1
```

```yumi
--
    |? catch? foo(0)
        1
    |? catch? foo(10)
        2
    ??
        3

━━━━━━━━━━━━━━━━━━━━
+ foo
    - n

?! Error("Divide by zero")
    if n == 0

n / 2
```

```output
2
```

```yumi
-- catch? foo(1) ?? -1
-- catch? foo(0) ?? -1

━━━━━━━━━━━━━━━━━━━━
+ do foo
    - n

if n > 0
    = 42
else
    ?! Error("Bad news")
```

```output
42
-1
```

```yumi
-- catch? set-mode(1, false)
-- catch? set-mode(0, true)

━━━━━━━━━━━━━━━━━━━━
+ set-mode
    - code
    - is-on

?! Error("bad")
    if code == 0

is-on
```

```output
false
unknown
```

```yumi
do new
    . foo Foo = Bar()
    -- foo as Baz
catch failure
    -- failure

+ Foo
+ Bar is Foo
+ Baz is Bar
+ Other is Foo
```

```output
Invalid cast to [Object __source__:Baz]
```

```yumi crash
. thing = Thing()
. p = thing as Proto
-- p.foo()

━━━━━━━━━━━━━━━━━━━━
+ Proto is Interface

────────────────────
+ foo Int

━━━━━━━━━━━━━━━━━━━━
+ Thing
    is Proto

────────────────────
+ foo

?! Error("fooey")
```

```output
Exception: "foo" can fail with Error, but the interface method it implements cannot; add "?! Error" to that method, or catch the error inside "foo"
<no file:17>
    + foo
    ^^^^
Interface:
<no file:10>
    + foo Int
```

```yumi
-- catch read(true)
-- catch read(false)
-- type-of read(true)
-- type-of catch read(true)

━━━━━━━━━━━━━━━━━━━━
+ do read Int ?! IOError
    - ok Bool

if ok
    = 7
else
    ?! IOError("read failed")
```

```output
7
read failed
[TypeInfo I64]
[TypeInfo Union]
```

```yumi
-- caller(true)
-- catch caller(false)

━━━━━━━━━━━━━━━━━━━━
+ do read Int ?! IOError
    - ok Bool

if ok
    = 40
else
    ?! IOError("disk missing")

━━━━━━━━━━━━━━━━━━━━
+ caller Int ?! IOError
    - ok Bool

do finally
    -- "cleanup"

read(ok) + 2
```

```output
cleanup
42
cleanup
disk missing
```

```yumi
. result = catch load()
. app = result as AppError
-- app
-- app.previous

━━━━━━━━━━━━━━━━━━━━
+ AppError is Error

━━━━━━━━━━━━━━━━━━━━
+ do read Int ?! IOError

?! IOError("disk")

━━━━━━━━━━━━━━━━━━━━
+ do load Int ?! AppError

?! AppError("load failed")
    catch err IOError

= read()
```

```output
load failed
disk
```

```yumi
. result = catch load()
. app = result as AppError
. config = app.previous! as ConfigError
-- app
-- config
-- config.previous

━━━━━━━━━━━━━━━━━━━━
+ ConfigError is Error

━━━━━━━━━━━━━━━━━━━━
+ AppError is Error

━━━━━━━━━━━━━━━━━━━━
+ do read Int ?! IOError

?! IOError("disk")

━━━━━━━━━━━━━━━━━━━━
+ do parse Int ?! ConfigError

?! ConfigError("config")
    catch err IOError

= read()

━━━━━━━━━━━━━━━━━━━━
+ do load Int ?! AppError

?! AppError("from io")
    catch err ConfigError <- io IOError

?! AppError("from config")
    catch err ConfigError

= parse()
```

```output
from io
config
disk
```

```yumi crash
?! 42
```

```output
Exception: I64 cannot be returned with ?!; return an Error subtype or use ?! Panic(...) to end the task.
<no file:2>
    ?! 42
    ^^^^^
```

```yumi crash
foo()

━━━━━━━━━━━━━━━━━━━━
+ foo Int ?! Int

1
```

```output
Exception: I64 is not a recoverable error type; only subclasses of Error may appear after ?!.
<no file:5>
    + foo Int ?! Int
    ^^^^^^^^^^^^^^^
```

```yumi crash
foo()

━━━━━━━━━━━━━━━━━━━━
+ foo Int ?! Panic

1
```

```output
Exception: Panic cannot be returned as a recoverable error; use ?! Panic(...) to end the task, or convert it explicitly.
<no file:5>
    + foo Int ?! Panic
    ^^^^^^^^^^^^^^^^^
```

```yumi crash
foo()

━━━━━━━━━━━━━━━━━━━━
+ do foo Int ?! TypeError

?! IOError("disk")
```

```output
Exception: IOError can leave "foo", which does not declare it; declare it, as in "+ do foo Int ?! TypeError ?! IOError"
<no file:7>
    ?! IOError("disk")
    ^^^^^^^^^^^^^^^^^^
```

```yumi crash
foo()

━━━━━━━━━━━━━━━━━━━━
+ AppError is Error

━━━━━━━━━━━━━━━━━━━━
+ do read String ?! IOError

?! IOError("disk")

━━━━━━━━━━━━━━━━━━━━
+ do foo Int ?! AppError

?! AppError(read())
    catch err IOError

?! IOError("source")
```

```output
Exception: Error transformer expressions must not call result-returning functions
<no file:15>
    ?! AppError(read())
                ^^^^^^
```

```yumi crash
foo()

━━━━━━━━━━━━━━━━━━━━
+ AppError is Error

━━━━━━━━━━━━━━━━━━━━
+ do foo Int ?! AppError

?! AppError("one")
    catch err IOError

?! AppError("two")
    catch other IOError

?! IOError("source")
```

```output
Exception: Ambiguous error transformer for IOError; only one transformer may match the same error chain.
<no file:14>
        catch other IOError
        ^^^^^^^^^^^^^^^^^^^
```

### A transformer names the error it replaces: a name alone, or a type that is no error, is an error.

```yumi compile-error
-- f()

━━━━━━━━━━━━━━━━━━━━
+ AppError is Error

━━━━━━━━━━━━━━━━━━━━
+ do f Int ?! AppError

?! AppError("one")
    catch err
= 1
```

```output
Exception: Error transformer catch requires an error type
<no file:11>
        catch err
        ^^^^^^^^^
```

### Inferred errors cannot retroactively change the ABI of a recursive function.

```yumi crash
-- recurse(3)

━━━━━━━━━━━━━━━━━━━━
+ do recurse Bool
    - n I64

?! IOError("disk")
    if n == 99

if n > 0
    = recurse(n - 1)
= true
```

```output
Exception: IOError can leave "recurse", which calls itself, so its errors cannot be inferred; declare them on it, as in "+ do recurse Bool ?! IOError", or catch them here
<no file:8>
    ?! IOError("disk")
    ^^^^^^^^^^^^^^^^^
```

### Inferred errors cannot retroactively change the ABI of a recursive Void function either.

```yumi crash
-- "result=${catch? walk(99) ?? false}"

━━━━━━━━━━━━━━━━━━━━
+ do walk Bool
    - n I64

visit(n)
= true

━━━━━━━━━━━━━━━━━━━━
+ do visit
    - n I64

?! IOError("disk")
    if n == 99
if n > 0
    visit(n - 1)
```

```output
Exception: IOError can leave "visit", which calls itself, so its errors cannot be inferred; declare them on it, as in "+ do visit ?! IOError", or catch them here
<no file:15>
    ?! IOError("disk")
    ^^^^^^^^^^^^^^^^^
```

### A recursive Void function with declared errors propagates them to its callers.

```yumi
-- "direct=${catch? walk(99) ?? false}"
-- "nested=${catch? walk(100) ?? false}"
-- "clean=${catch? walk(3) ?? false}"

━━━━━━━━━━━━━━━━━━━━
+ do walk Bool ?! IOError
    - n I64

visit(n)
= true

━━━━━━━━━━━━━━━━━━━━
+ do visit ?! IOError
    - n I64

?! IOError("disk")
    if n == 99
if n > 0
    visit(n - 1)
```

```output
direct=false
nested=false
clean=true
```

### A recursive function with declared errors returns correct values through the result ABI.

```yumi
-- "result=${catch? recurse(3) ?? false}"
-- "raised=${catch? recurse(99) ?? false}"

━━━━━━━━━━━━━━━━━━━━
+ do recurse Bool ?! IOError
    - n I64

?! IOError("disk")
    if n == 99

if n > 0
    = recurse(n - 1)
= true
```

```output
result=true
raised=false
```

### Recursive local grammars tell authors to declare errors on the enclosing function.

```yumi crash
-- catch? parse("a!") ?? "error"

━━━━━━━━━━━━━━━━━━━━
+ ParserError is Error

━━━━━━━━━━━━━━━━━━━━
+ parse String
    - text String

match text
    ~ \items
    ?! ParserError("invalid")

    .. items String ~
        ~ head \item tail \items?
            head ++ (tail ?? "")
        ~ ""
            ""

    .. item String ~
        ~ "a"
            "a"
        ~ "!"
            bad-item()

━━━━━━━━━━━━━━━━━━━━
+ do bad-item String ?! ParserError

?! ParserError("bang")
```

```output
Exception: ParserError can leave recursive function items before its errors are inferred; For a local '.. name ~' grammar, declare ?! ParserError on the enclosing '+ name ~ ...' function so generated pattern helpers inherit the result ABI.
<no file:16>
            ~ head \item tail \items?
                   ^^^^^
```

### Recursive local grammars inherit declared result errors from the enclosing function.

```yumi
-- catch? parse("aaa") ?? "error"
-- catch? parse("a!") ?? "error"

━━━━━━━━━━━━━━━━━━━━
+ ParserError is Error

━━━━━━━━━━━━━━━━━━━━
+ parse String ?! ParserError
    - text String

match text
    ~ \items
    ?! ParserError("invalid")

    .. items String ~
        ~ head \item tail \items?
            head ++ (tail ?? "")
        ~ ""
            ""

    .. item String ~
        ~ "a"
            "a"
        ~ "!"
            bad-item()

━━━━━━━━━━━━━━━━━━━━
+ do bad-item String ?! ParserError

?! ParserError("bang")
```

```output
aaa
error
```

### Acyclic local stream helpers preserve the enclosing function's declared result-error ABI.

```yumi
-- catch? parse-pair("ok") ?? "error"
-- catch? parse-pair("o!") ?? "error"

━━━━━━━━━━━━━━━━━━━━
+ PairError is Error

━━━━━━━━━━━━━━━━━━━━
+ parse-pair String ?! PairError
    - text String

match text
    ~ left \prefix right \suffix
        left ++ right
    ?! PairError("invalid")

    .. prefix String ~
        ~ "o"

    .. suffix String ~
        ~ "k"
        ~ "!"
            bad-suffix()

━━━━━━━━━━━━━━━━━━━━
+ do bad-suffix String ?! PairError

?! PairError("bad suffix")
```

```output
ok
error
```

### Attached handlers recover selected errors and propagate unhandled original errors.

```yumi
-- inspect-attached(0)
-- inspect-attached(1)
-- catch? inspect-attached(2) ?? "propagated"

━━━━━━━━━━━━━━━━━━━━
+ Config

+ value String

────────────────────
+ This
    - value

━━━━━━━━━━━━━━━━━━━━
+ FileNotFound is Error

━━━━━━━━━━━━━━━━━━━━
+ PermissionDenied is Error

━━━━━━━━━━━━━━━━━━━━
+ do read-config Config ?! FileNotFound ?! PermissionDenied
    - outcome Int

if outcome == 1
    ?! FileNotFound("missing")
if outcome == 2
    ?! PermissionDenied("denied")
= Config("loaded")

━━━━━━━━━━━━━━━━━━━━
+ do inspect-attached String ?! PermissionDenied
    - outcome Int

. config = read-config(outcome)
    catch FileNotFound
        Config("default")
= config.value
```

```output
loaded
default
propagated
```

### An error-bearing module combinator preserves success and propagated error results while sharing the active frame through its static pattern argument.

```yumi
-- catch? parse-combinator("ok?") ?? "error"
-- catch? parse-combinator("ok!") ?? "error"

━━━━━━━━━━━━━━━━━━━━
+ CombinatorError is Error

━━━━━━━━━━━━━━━━━━━━
+ guarded-pattern'Result ~ Char ?! CombinatorError
    - pattern PatternFunc'Char'Result

~ value $pattern "?"
    value
~ $pattern "!"
    fail-combinator()

━━━━━━━━━━━━━━━━━━━━
+ parse-combinator String ?! CombinatorError
    - text String

match text
    ~ \guarded-pattern(~ [a-z]+)
    ?! CombinatorError("invalid")

━━━━━━━━━━━━━━━━━━━━
+ do fail-combinator String ?! CombinatorError

?! CombinatorError("guarded failure")
```

```output
ok
error
```

### A direct static module rule can use the cursor/result ABI with declared errors.

```yumi
-- catch? parse-module-token("ok") ?? "error"
-- catch? parse-module-token("!") ?? "error"

━━━━━━━━━━━━━━━━━━━━
+ ModuleTokenError is Error

━━━━━━━━━━━━━━━━━━━━
+ module-token ~ Char ?! ModuleTokenError

~ "ok"
~ "!"
    bad-module-token()

━━━━━━━━━━━━━━━━━━━━
+ parse-module-token String ?! ModuleTokenError
    - text String

match text
    ~ \module-token
    ?! ModuleTokenError("invalid")

━━━━━━━━━━━━━━━━━━━━
+ do bad-module-token String ?! ModuleTokenError

?! ModuleTokenError("bad module token")
```

```output
ok
error
```

### Prefix holding disables automatic propagation and requires every original Error case.

```yumi crash
. config = catch read-config()
    catch FileNotFound
        Config("default")
-- config.value

━━━━━━━━━━━━━━━━━━━━
+ Config

+ value String

────────────────────
+ This
    - value

━━━━━━━━━━━━━━━━━━━━
+ FileNotFound is Error

━━━━━━━━━━━━━━━━━━━━
+ PermissionDenied is Error

━━━━━━━━━━━━━━━━━━━━
+ do read-config Config ?! FileNotFound ?! PermissionDenied

?! PermissionDenied("denied")
```

```output
Exception: This catch does not handle PermissionDenied; add a handler for it, or one for Error
<no file:2>
    . config = catch read-config()
               ^^^^^^^^^^^^^^^^^^
```

### A handler-produced bare Error stays in the expression, and an explicit return fuses it later.

```yumi
-- catch relay-attached(0)
-- catch relay-attached(1)
-- catch relay-attached(2)
-- catch relay-held()

━━━━━━━━━━━━━━━━━━━━
+ ConvertedError is Error

━━━━━━━━━━━━━━━━━━━━
+ FileNotFound is Error

━━━━━━━━━━━━━━━━━━━━
+ PermissionDenied is Error

━━━━━━━━━━━━━━━━━━━━
+ do read-text String ?! FileNotFound ?! PermissionDenied
    - outcome Int

if outcome == 1
    ?! FileNotFound("missing")
if outcome == 2
    ?! PermissionDenied("denied")
= "loaded"

━━━━━━━━━━━━━━━━━━━━
+ do relay-attached
    - outcome Int

. handled = read-text(outcome)
    catch err FileNotFound
        ConvertedError("converted ${err.message}")
    catch PermissionDenied
        "permission default"
-- "after handler"
= handled

━━━━━━━━━━━━━━━━━━━━
+ do relay-held String ?! FileNotFound

. handled = catch read-text(1)
    catch err FileNotFound
        ?! err
    catch PermissionDenied
        "permission default"
= handled
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

```yumi
_ attached-head-count = 0

-- inspect-once(0)
-- inspect-once(1)
-- inspect-once(2)
-- attached-head-count

━━━━━━━━━━━━━━━━━━━━
+ Config

+ value String

────────────────────
+ This
    - value

━━━━━━━━━━━━━━━━━━━━
+ FileNotFound is Error

━━━━━━━━━━━━━━━━━━━━
+ PermissionDenied is Error

━━━━━━━━━━━━━━━━━━━━
+ do counted-read Config ?! FileNotFound ?! PermissionDenied
    - outcome Int

attached-head-count += 1
-- "head $attached-head-count"
if outcome == 1
    ?! FileNotFound("missing")
if outcome == 2
    ?! PermissionDenied("denied")
= Config("loaded")

━━━━━━━━━━━━━━━━━━━━
+ do inspect-once String
    - outcome Int

. config = counted-read(outcome)
    catch FileNotFound
        Config("file default")
    catch PermissionDenied
        Config("permission default")
= config.value
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

```yumi
exercise-void-handler()
-- "after"

━━━━━━━━━━━━━━━━━━━━
+ do void-result ?! IOError

━━━━━━━━━━━━━━━━━━━━
+ do exercise-void-handler

void-result()
    catch IOError
        ?! Panic("converted")
```

```output
after
```

### Attached-handler inference resolves implicit instance properties inside the Result head.

```yumi
-- Holder(2).handled().value

━━━━━━━━━━━━━━━━━━━━
+ Config

+ value String

────────────────────
+ This
    - value

━━━━━━━━━━━━━━━━━━━━
+ FileNotFound is Error

━━━━━━━━━━━━━━━━━━━━
+ do read-holder-config Config ?! FileNotFound
    - outcome Int

if outcome == 1
    ?! FileNotFound("missing")
= Config("loaded $outcome")

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ outcome Int

────────────────────
+ This
    - outcome

────────────────────
+ do handled Config

. handled = read-holder-config(outcome)
    catch FileNotFound
        Config("default")
= handled
```

```output
loaded 2
```

### Constructor Result declarations participate in attached-handler inference and lowering.

```yumi
-- type-of Fallible(false)
-- type-of catch Fallible(false)
-- inspect-held-constructor(true)
-- inspect-constructor(false)
-- inspect-constructor(true)

━━━━━━━━━━━━━━━━━━━━
+ Fallible

+ label String

────────────────────
+ This

label = "recovered"

────────────────────
+ This ?! IOError
    - fail Bool

if fail
    ?! IOError("failed")
label = "loaded"

━━━━━━━━━━━━━━━━━━━━
+ do inspect-constructor String
    - fail Bool

. value = Fallible(fail)
    catch IOError
        Fallible()
= value.label

━━━━━━━━━━━━━━━━━━━━
+ do inspect-held-constructor String
    - fail Bool

. held = catch Fallible(fail)
= match held
    | value Fallible
        value.label
    | IOError
        "held recovered"
```

```output
[TypeInfo Fallible]
[TypeInfo Union]
held recovered
loaded
recovered
```

### Overlapping handlers use source order, while a narrow handler retains a broad residual.

```yumi
-- inspect-overlap(true)
-- inspect-overlap(false)
-- inspect-residual(true)
-- catch? inspect-residual(false) ?? "propagated"

━━━━━━━━━━━━━━━━━━━━
+ ParentError is Error

━━━━━━━━━━━━━━━━━━━━
+ ChildError is ParentError

━━━━━━━━━━━━━━━━━━━━
+ do fail-parent String ?! ParentError
    - child Bool

if child
    ?! ChildError("child")
?! ParentError("parent")

━━━━━━━━━━━━━━━━━━━━
+ do inspect-overlap String
    - child Bool

. value = fail-parent(child)
    catch ChildError
        "child first"
    catch ParentError
        "parent second"
= value

━━━━━━━━━━━━━━━━━━━━
+ do inspect-residual String ?! ParentError
    - child Bool

. value = fail-parent(child)
    catch ChildError
        "child only"
= value
```

```output
child first
parent second
child only
propagated
```

### Attached handler bodies may explicitly propagate replacement errors.

```yumi
-- catch? replace-error() ?? "replacement propagated"

━━━━━━━━━━━━━━━━━━━━
+ SourceError is Error

━━━━━━━━━━━━━━━━━━━━
+ ReplacementError is Error

━━━━━━━━━━━━━━━━━━━━
+ do fail-source String ?! SourceError

?! SourceError("source")

━━━━━━━━━━━━━━━━━━━━
+ do replace-error String ?! ReplacementError

. value = fail-source()
    catch SourceError
        ?! ReplacementError("replacement")
= value
```

```output
replacement propagated
```

### An optional error becomes text by the converter of its object's class, which may make text of its own.

```yumi
. made Error? = AppError("x")
. none Error? = made!.previous
-- made
-- "${none}"

━━━━━━━━━━━━━━━━━━━━
+ AppError is Error

────────────────────
+ to String

"AppError: $message"
```

```output
AppError: x
unknown
```

### Prefix catch over a value that raises nothing gives the value, as it does over a call that cannot fail.

```yumi
. x = 3
-- catch 5
-- catch? 5
-- catch x + 1
-- catch? x + 1
```

```output
5
5
4
4
```

### Attached handlers reject infallible heads with a direct Result diagnostic.

```yumi crash
. value = 42
    catch IOError
        0
-- value
```

```output
Exception: "42" cannot fail, so the catch attached to it has nothing to handle; remove the catch, or attach it to a call that can fail
<no file:2>
    . value = 42
              ^
```

### Attached handlers hold only their head call; fallible arguments unwrap and propagate.

```yumi
-- rejects(3)
-- rejects(20)
. escaped = catch? rejects(-1)
-- escaped == null

━━━━━━━━━━━━━━━━━━━━
+ Box

+ value Int

────────────────────
+ This
    - value

━━━━━━━━━━━━━━━━━━━━
+ do make Box ?! Error
    - value Int

if value < 0
    ?! Error("negative")
= Box(value)

━━━━━━━━━━━━━━━━━━━━
+ do consume ?! Error
    - box Box
    - label String

if box.value > 10
    ?! Error("large")
-- "$label ${box.value}"

━━━━━━━━━━━━━━━━━━━━
+ do rejects Bool ?! Error
    - value Int

consume(make(value), "consumed")
    catch Error
        = true
= false
```

```output
consumed 3
false
true
true
```

### A call with a missing argument reports the missing function even when a catch handler is attached.

```yumi crash
. probe = Probe()
probe.run()
    catch err Error
        ?! Panic(err as String)

━━━━━━━━━━━━━━━━━━━━
+ Probe

────────────────────
+ do run ?! Error
    - value Int

-- value
```

```output
Exception: No method "run" of Probe accepts (); it is declared as "run(value Int)"
<no file:3>
    probe.run()
    ^^^^^^^^^^
```

### A raise inside a loop branch of a fallible method without a result propagates to the caller.

```yumi
. probe = Probe()
probe.run(["a", "b", "a"])
    catch err Error
        -- "caught $err"
-- probe.count

━━━━━━━━━━━━━━━━━━━━
+ Probe

+ count = 0

────────────────────
+ do run ?! Error
    - kinds [String]

for kind in kinds
    if kind == "a"
        count += 1
    else
        ?! Error("unknown $kind")
```

```output
caught unknown b
1
```

### An attached handler may call a function without a result when the guarded call has no result.

```yumi
. probe = Probe()
probe.run(true)
-- probe.note
probe.run(false)
-- probe.note

━━━━━━━━━━━━━━━━━━━━
+ Probe

+ note = "none"

────────────────────
+ do run
    - fails Bool

work(fails)
    catch err Error
        record("$err")

────────────────────
+ do record
    - message String

note = message

────────────────────
+ do work ?! Error
    - fails Bool

?! Error("broken")
    if fails
note = "worked"
```

```output
broken
worked
```

### A handler that gives nothing may take the error of a call through a type parameter bound to a held Result with no value: returning the held Result raises its error, and the call has no value.

```yumi
relay(catch run(true))
-- "passed"
relay(catch run(false))
    catch failure IOError
        do -- "raised ${failure.message}"

━━━━━━━━━━━━━━━━━━━━
+ do run ?! IOError
    - ok Bool

?! IOError("run failed")
    if !ok

━━━━━━━━━━━━━━━━━━━━
+ do relay'Value Value
    - held Value

= held
```

```output
passed
raised run failed
```

### A raise after an early return in a fallible method without a result propagates to the caller.

```yumi
. probe = Probe()
probe.run(false, ["a", "b", "a"])
    catch err Error
        -- "caught $err"
-- probe.count

━━━━━━━━━━━━━━━━━━━━
+ Probe

+ count = 0

────────────────────
+ do run ?! Error
    - skip Bool
    - kinds [String]

=? if skip
for kind in kinds
    if kind == "a"
        count += 1
    else
        ?! Error("unknown $kind")
```

```output
caught unknown b
1
```

### A raise inside a loop branch still propagates when a finally block is pending.

```yumi
. probe = Probe()
probe.run(["a", "b", "a"])
    catch err Error
        -- "caught $err"
-- probe.count
-- probe.finished

━━━━━━━━━━━━━━━━━━━━
+ Probe

+ count = 0
+ finished = 0

────────────────────
+ do run ?! Error
    - kinds [String]

do finally finished += 1
for kind in kinds
    if kind == "a"
        count += 1
    else
        ?! Error("unknown $kind")
```

```output
caught unknown b
1
1
```

### An error raised while evaluating a constructor argument propagates instead of constructing the object.

```yumi
-- describe(true)
-- describe(false)

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ items [String] = List'String()

────────────────────
+ This

────────────────────
+ This
    - items

━━━━━━━━━━━━━━━━━━━━
+ do make [String] ?! Error
    - fails Bool

?! Error("broken")
    if fails
. result [String]
result += "made"
= result

━━━━━━━━━━━━━━━━━━━━
+ do build Holder ?! Error
    - fails Bool

. holder = Holder(make(fails))
= holder

━━━━━━━━━━━━━━━━━━━━
+ do describe String
    - fails Bool

. holder = build(fails)
    catch err Error
        = "caught $err"
= "built ${holder.items.length}"
```

```output
caught broken
built 1
```

### A raise inside a loop over a fallible iterator propagates through a recursive fallible method.

```yumi
. probe = Probe()
probe.run(["a", "b", "a"], true)
    catch err Error
        -- "caught $err"
-- probe.count

━━━━━━━━━━━━━━━━━━━━
+ Probe

+ count = 0

────────────────────
+ do run ?! Error
    - kinds [String]
    - nested Bool

for kind in checked(kinds)
    if kind == "a"
        count += 1
    else if kind == "nested" && nested
        run(kinds, false)
    else
        ?! Error("unknown $kind")

────────────────────
+ do checked [String] ?! Error
    - kinds [String]

?! Error("empty")
    if !kinds.length
= kinds
```

```output
caught unknown b
1
```

```yumi
-- describe("Green")
-- describe("Purple")

━━━━━━━━━━━━━━━━━━━━
+ Shade is Union

| red
| green

━━━━━━━━━━━━━━━━━━━━
+ shade-named ?! Error
    - name String

match name
    |= "Red"
        Shade.red
    |= "Green"
        Shade.green
    ?! Error("Unknown shade '$name'")

━━━━━━━━━━━━━━━━━━━━
+ do describe String
    - name String

. shade = shade-named(name)
    catch err Error
        = "unknown"
= if shade == Shade.green then "green" else "red"
```

```output
green
unknown
```

### A caught optional result matches its value arm only when a value is present.

```yumi
-- spot-text(true)
-- spot-text(false)
-- spot-text-failing()

━━━━━━━━━━━━━━━━━━━━
+ Spot is Tuple

+ number F64
+ count F64

━━━━━━━━━━━━━━━━━━━━
+ do spot-found Spot? ?! Error
    - present Bool
    - failing = false

?! Error("lost")
    if failing
=? Spot(2, 5) if present
= null

━━━━━━━━━━━━━━━━━━━━
+ do spot-text String
    - present Bool

. result = catch spot-found(present)
do match? result
    | error Error
        = "error"
    | spot Spot
        = "spot ${spot.number} of ${spot.count}"
= "none"

━━━━━━━━━━━━━━━━━━━━
+ do spot-text-failing String

. result = catch spot-found(true, true)
do match? result
    | error Error
        = "error ${error.message}"
    | spot Spot
        = "spot ${spot.number}"
= "none"
```

```output
spot 2 of 5
none
error lost
```

### A caught optional result from an interface method matches its value arm only when present.

```yumi
-- located(Locator(true))
-- located(Locator(false))
-- located(Fallback())

━━━━━━━━━━━━━━━━━━━━
+ Place is Tuple

+ number F64
+ count F64

━━━━━━━━━━━━━━━━━━━━
+ PlaceError is Error

━━━━━━━━━━━━━━━━━━━━
+ Finder is Interface

────────────────────
+ do place Place? ?! PlaceError
    - wanted Int

= null

━━━━━━━━━━━━━━━━━━━━
+ Locator
    is Finder

+ present Bool

────────────────────
+ This
    - present

────────────────────
+ do place Place? ?! PlaceError
    - wanted Int

=? null if !present
= Place(wanted as F64, 5)

━━━━━━━━━━━━━━━━━━━━
+ Fallback
    is Finder

━━━━━━━━━━━━━━━━━━━━
+ do located String
    - finder Finder

. result = catch finder.place(3)
do match? result
    | error PlaceError
        = "error"
    | place Place
        = "place ${place.number} of ${place.count}"
= "none"
```

```output
place 3 of 5
none
none
```

### A string literal converts in an override whose base raises no errors

```yumi
. base Base = Child()
-- base.code().width

━━━━━━━━━━━━━━━━━━━━
+ Code is Tuple

+ width = 0
+ height = 0

────────────────────
+ do from text String

?! SyntaxError("Invalid code")
    if text.length != 3
= Code(text.length, 1)

━━━━━━━━━━━━━━━━━━━━
+ Base

────────────────────
+ code Code

Code(0, 0)

━━━━━━━━━━━━━━━━━━━━
+ Child is Base

────────────────────
+ code Code

"abc"
```

```output
3
```

### An explicit conversion of a string literal raises no errors

```yumi
. base Base = Child()
-- base.code().height

━━━━━━━━━━━━━━━━━━━━
+ Code is Tuple

+ width = 0
+ height = 0

────────────────────
+ do from text String

?! SyntaxError("Invalid code")
    if text.length != 3
= Code(text.length, 1)

━━━━━━━━━━━━━━━━━━━━
+ Base

────────────────────
+ code Code

Code(0, 0)

━━━━━━━━━━━━━━━━━━━━
+ Child is Base

────────────────────
+ code Code

Code("xyz")
```

```output
1
```

### A string literal that fails its conversion panics

```yumi
do new
    -- broken().width
catch failure
    -- failure

━━━━━━━━━━━━━━━━━━━━
+ Code is Tuple

+ width = 0
+ height = 0

────────────────────
+ do from text String

?! SyntaxError("Invalid code")
    if text.length != 3
= Code(text.length, 1)

━━━━━━━━━━━━━━━━━━━━
+ broken Code

"toolong"
```

```output
Invalid code
```

### A runtime string that fails its conversion raises the conversion error

```yumi
. text = "toolong"
-- described(text)

━━━━━━━━━━━━━━━━━━━━
+ Code is Tuple

+ width = 0
+ height = 0

────────────────────
+ do from text String

?! SyntaxError("Invalid code")
    if text.length != 3
= Code(text.length, 1)

━━━━━━━━━━━━━━━━━━━━
+ do parse Code
    - text String

= text

━━━━━━━━━━━━━━━━━━━━
+ do described String
    - text String

. result = catch parse(text)
do match? result
    | error SyntaxError
        = "error ${error.message}"
    | code Code
        = "code ${code.width}"
= "none"
```

```output
error Invalid code
```

### A conditional coalesce converts a string literal branch to the other branch's type

```yumi
. base Base = Child()
-- base.code().width

━━━━━━━━━━━━━━━━━━━━
+ Code is Tuple

+ width = 0
+ height = 0

────────────────────
+ do from text String

?! SyntaxError("Invalid code")
    if text.length != 3
= Code(text.length, 1)

━━━━━━━━━━━━━━━━━━━━
+ Base

────────────────────
+ code Code

Code(0, 0)

━━━━━━━━━━━━━━━━━━━━
+ Child is Base

────────────────────
+ do code Code

. fallback = Code(0, 0)
= if fallback.width == 0 then "abc" else fallback
```

```output
3
```

### A raw catch holds a value or its error, and catch? a value or nothing.

```yumi
. result = catch check(3)
-- match result
    | Int
        "value $it"
    | IOError
        "io ${it.message}"
. other = catch check(-1)
-- match other
    | Int
        "value $it"
    | IOError
        "io ${it.message}"
. maybe = catch? check(-1)
-- maybe ?? 99
. sure = catch? check(5)
-- sure ?? 99

━━━━━━━━━━━━━━━━━━━━
+ do check Int
    - value Int

?! IOError("negative")
    if value < 0
= value + 1
```

```output
value 4
io negative
99
6
```

### What a raw catch holds has no truth, even over a call that answers nothing: a condition over it is an error that says to test it with `is`.

```yumi compile-error
. failure = catch check(-1)
-- if failure then "failed" else "worked"

━━━━━━━━━━━━━━━━━━━━
+ do check ?! IOError
    - value Int

?! IOError("negative") if value < 0
```

```output
Exception: A value that may be an error, as "catch" gives, has no truth; test it with "is Error", or take it apart with "match"
<no file:3>
    -- if failure then "failed" else "worked"
          ^^^^^^^
```

### An error that leaves main is reported and ends the program.

```yumi crash
-- "before"
check(3)
-- "middle"
check(-1)
-- "after"

━━━━━━━━━━━━━━━━━━━━
+ do check Int
    - value Int

?! IOError("negative")
    if value < 0
= value + 1
```

```output
before
middle
```

### An error propagates through its callers to main.

```yumi crash
-- outer(3)
-- outer(-1)
-- "after"

━━━━━━━━━━━━━━━━━━━━
+ do outer Int
    - value Int

. inner = check(value)
-- "inner $inner"
= inner * 2

━━━━━━━━━━━━━━━━━━━━
+ do check Int
    - value Int

?! IOError("negative")
    if value < 0
= value + 1
```

```output
inner 4
8
```

### Results of nothing, text, a tuple and a float, with one error or two, and a relay that infers its errors.

```yumi
-- catch save(1)
-- catch save(-1)
-- catch? name(2)
-- catch? name(-2)
-- (catch? pair(3)) ?? (0, "none")
-- (catch? pair(-3)) ?? (0, "none")
. kept = catch wide(5)
-- match kept
    | Float
        "float $it"
    | IOError
        "io"
    | SyntaxError
        "syntax"
-- match catch wide(-5)
    | Float
        "float"
    | IOError
        "io ${it.message}"
    | SyntaxError
        "syntax ${it.message}"
-- match catch wide(0)
    | Float
        "float"
    | IOError
        "io ${it.message}"
    | SyntaxError
        "syntax ${it.message}"
-- match catch relay(0)
    | Float
        "float"
    | Error
        "error ${it.message}"
-- catch? relay(4)

━━━━━━━━━━━━━━━━━━━━
+ do save ?! IOError
    - value Int

?! IOError("cannot save")
    if value < 0
-- "saved $value"

━━━━━━━━━━━━━━━━━━━━
+ do name String ?! IOError
    - value Int

?! IOError("no name")
    if value < 0
= "name$value"

━━━━━━━━━━━━━━━━━━━━
+ do pair (Int, String) ?! SyntaxError
    - value Int

?! SyntaxError("no pair")
    if value < 0
= (value, "pair")

━━━━━━━━━━━━━━━━━━━━
+ do wide Float ?! IOError ?! SyntaxError
    - value Int

?! IOError("io")
    if value < 0
?! SyntaxError("syntax")
    if value == 0
= (value as Float) + 0.5

━━━━━━━━━━━━━━━━━━━━
+ do relay Float
    - value Int

. first = wide(value + 1)
= first + wide(value)
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

```yumi crash
-- catch? outer(1)
-- catch? outer(-1)
-- total
outer(-7)
-- "unreached"

_ total = 0

━━━━━━━━━━━━━━━━━━━━
+ do outer Int
    - value Int

do finally
    total += 1
    -- "outer done"
. first = inner(value)
. second = inner(value + 1) + inner(value + 2)
= first + second

━━━━━━━━━━━━━━━━━━━━
+ do inner Int
    - value Int

do finally
    total += 10
?! IOError("inner $value")
    if value < 0
= value * 2
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

```yumi crash
. box = Box(3)
-- catch? box.take(2)
-- catch? box.take(5)
-- match catch box.take(9)
    | Int
        "took $it"
    | BoxError
        "box ${it.message} ${it.wanted}"
-- catch? box.drain()
box.take(100)
-- "unreached"

━━━━━━━━━━━━━━━━━━━━
+ BoxError is Error

+ wanted Int

────────────────────
+ This
    - count Int

.. message = "empty"
.. wanted = count

━━━━━━━━━━━━━━━━━━━━
+ Box

+ held Int

────────────────────
+ This
    - count Int

.. held = count

────────────────────
+ do take Int
    - count Int

?! BoxError(count)
    if count > held
held -= count
= held

────────────────────
+ do drain Int

= take(held) + take(1)
```

```output
1
unknown
box empty 9
unknown
```

### An error of a function that states Error is held as the case of Error.

```yumi crash
-- catch? stated(1)
-- match catch stated(-1)
    | Int
        "value"
    | Error
        "error ${it.message}"
-- match catch stated(0)
    | Int
        "value"
    | Error
        "error ${it.message}"
. kept = catch chain(-1)
-- kept is Error
-- kept is Int
stated(-1)

━━━━━━━━━━━━━━━━━━━━
+ do stated Int ?! Error
    - value Int

?! IOError("io")
    if value < 0
?! SyntaxError("syntax")
    if value == 0
= value

━━━━━━━━━━━━━━━━━━━━
+ do chain Int ?! Error
    - value Int

= stated(value) + 1
```

```output
1
error io
error syntax
true
false
```

### Errors leave from inside blocks and loops, with the cleanups of each.

```yumi crash
-- catch? deep(3)
-- catch? deep(-3)
deep(-1)

━━━━━━━━━━━━━━━━━━━━
+ do deep Int
    - value Int

if value > 0
    do finally
        -- "leaving block"
    . inner = check(value)
    = inner
while true
    do finally
        -- "leaving loop"
    check(value)
    break
= 0

━━━━━━━━━━━━━━━━━━━━
+ do check Int
    - value Int

?! IOError("negative")
    if value < 0
= value + 1
```

```output
leaving block
4
leaving loop
unknown
leaving loop
```

### A held Result is copied, tested, passed to a function and named Union.

```yumi
. first = catch check(1)
. second = catch check(-1)
-- type-of first
-- first
-- second
. third = second
-- third is IOError
show(first)
show(second)

━━━━━━━━━━━━━━━━━━━━
+ do show
    - result

-- match result
    | Int
        "int $it"
    | IOError
        "error"

━━━━━━━━━━━━━━━━━━━━
+ do check Int
    - value Int

?! IOError("negative")
    if value < 0
= value + 1
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

```yumi crash
-- named(1)
-- named(-1)
-- named(0)
-- broad(-1)
-- broad(0)
-- catch? narrow(-1)
-- catch? narrow(0)
-- catch? narrow(5)
-- total
stated(0)
    catch IOError
        do -- "main io"
        0
-- "main after"
stated(-1)
    catch SyntaxError
        do -- "main syntax"
        0
-- "unreached"

_ total = 0

━━━━━━━━━━━━━━━━━━━━
+ do stated Int ?! Error
    - value Int

?! IOError("io $value")
    if value < 0
?! SyntaxError("syntax $value")
    if value == 0
= value

━━━━━━━━━━━━━━━━━━━━
+ do bump Int
    - by Int

total += by
= 0 - by

━━━━━━━━━━━━━━━━━━━━
+ do named String
    - value Int

. got = stated(value)
    catch failure IOError
        bump(failure.message.length)
    catch other Error
        bump(10)
= "got $got"

━━━━━━━━━━━━━━━━━━━━
+ do broad String
    - value Int

do finally
    total += 100
. text = stated(value)
    catch failure Error
        failure.message.length
= "broad $text"

━━━━━━━━━━━━━━━━━━━━
+ do narrow Int
    - value Int

do finally
    total += 1000
. got = stated(value)
    catch SyntaxError
        relay(value)
= got + 1

━━━━━━━━━━━━━━━━━━━━
+ do relay Int
    - value Int

= stated(value - 1)
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

```yumi
-- catch? pick(1)
-- catch? pick(2)
-- "after"

━━━━━━━━━━━━━━━━━━━━
+ do pick String
    - kind Int

. chosen = match kind
    |= 1
        IOError("one")
    ??
        SyntaxError("other")
-- type-of chosen
= "picked"
```

```output
unknown
unknown
after
```

### An arm that gives an error beside arms that give values raises it at the arm.

```yumi crash
-- catch? pick(1)
-- catch? pick(2)
pick(1)
-- "after"

━━━━━━━━━━━━━━━━━━━━
+ do pick String
    - kind Int

. chosen = match kind
    |= 1
        IOError("one")
    ??
        "plain"
-- type-of chosen
= chosen
```

```output
unknown
[TypeInfo String]
plain
```

### An arm that gives an error its declared type holds gives the error as a value, and an arm whose error the declared type cannot hold raises it.

```yumi
-- by-match(0)?.message ?? "none"
-- by-match(1)?.message ?? "none"
-- by-table(-1)?.message ?? "none"
-- by-table(1)?.message ?? "none"
-- named(0).message
-- named(1).message
-- catch? text(0) ?? "raised"
-- catch? text(1) ?? "raised"

━━━━━━━━━━━━━━━━━━━━
+ Oops is Error

━━━━━━━━━━━━━━━━━━━━
+ by-match Oops?
    - flag Int

match flag
    |= 0
        Oops("zero")
    ??
        null

━━━━━━━━━━━━━━━━━━━━
+ by-table Error?
    - value Int

|? value < 0
    Oops("below")
??
    null

━━━━━━━━━━━━━━━━━━━━
+ named Oops
    - flag Int

match flag
    |= 0
        Oops("zero")
    ??
        Oops("other")

━━━━━━━━━━━━━━━━━━━━
+ text String
    - flag Int

match flag
    |= 0
        Oops("zero")
    ??
        "plain"
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

```yumi
-- describe(3, false)
-- describe(3, true)
-- describe(200, false)

━━━━━━━━━━━━━━━━━━━━
+ f Int ?! Error
    - n Int
    - flag Bool

|? flag
    1
?! Error("too big") if n > 100
??
    n * 2

━━━━━━━━━━━━━━━━━━━━
+ describe String
    - n Int
    - flag Bool

match catch f(n, flag)
    | value Int
        "$value"
    | error Error
        error.message
```

```output
6
1
too big
```

### An arm of a table with no subject panics where its condition holds.

```yumi crash
-- f(3, true)
-- f(200, false)

━━━━━━━━━━━━━━━━━━━━
+ f Int
    - n Int
    - flag Bool

|? flag
    1
?! Panic("too big: $n") if n > 100
??
    n * 2
```

```stderr
Unhandled panic: too big: 200
<no file:12>

Task 1

Stack trace:
  0  __source__:f no file:12:1
  1  __source__:main no file:3:4
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

### An arm of a table with no subject whose body ends in a raise leaves the function where it is taken.

```yumi
-- describe(1)
-- describe(20)
-- describe(5)

━━━━━━━━━━━━━━━━━━━━
+ f Int ?! Error
    - n Int

|? n > 10
    ?! Error("big $doubled")

    .. doubled = n * 2
|? n < 3
    n
??
    ?! Error("bad $tripled")

    .. tripled = n * 3

━━━━━━━━━━━━━━━━━━━━
+ describe String
    - n Int

match catch f(n)
    | value Int
        "$value"
    | error Error
        error.message
```

```output
1
big 40
bad 15
```

### A choice between an error and a text has no type in common.

```yumi crash
-- catch? pick(1)
-- catch? pick(2)
-- "after"

━━━━━━━━━━━━━━━━━━━━
+ do pick String
    - kind Int

. chosen = if kind == 1 then IOError("one") else "plain"
-- type-of chosen
= chosen
```

```output
Exception: The two sides of this "if" are IOError and String, which have no common type; convert them to one type
<no file:10>
    . chosen = if kind == 1 then IOError("one") else "plain"
               ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

### An error constructed as a statement or held in a local is not raised.

```yumi
-- catch? pick(1)
-- "after"

━━━━━━━━━━━━━━━━━━━━
+ do pick String
    - kind Int

. made = IOError("one")
-- type-of made
IOError("dropped")
-- "still here"
= made.message
```

```output
[TypeInfo IOError]
still here
one
after
```

### An arm that raises an error, and a match on conditions whose arms give errors.

```yumi crash
-- catch? shade("Red")
-- catch? shade("Blue")
-- catch? level(1)
-- catch? level(5)
-- catch? level(-5)
shade("Blue")

━━━━━━━━━━━━━━━━━━━━
+ shade ?! Error
    - name String

match name
    |= "Red"
        1
    |= "Green"
        2
    ?! Error("Unknown shade '$name'")

━━━━━━━━━━━━━━━━━━━━
+ level
    - value Int

|? value < 0
    IOError("below")
|? value > 3
    SyntaxError("above")
??
    value
```

```output
1
unknown
1
unknown
unknown
```

### A returned Result returns the error it holds in its place.

```yumi crash
-- catch relay(true)
-- catch relay(false)
-- catch? widen(true)
-- catch? widen(false)
-- catch? guarded(true)
-- catch? guarded(false)
-- total
relay(false)

_ total = 0

━━━━━━━━━━━━━━━━━━━━
+ do read I32 ?! IOError
    - ok Bool

if ok
    = 7
else
    ?! IOError("read failed")

━━━━━━━━━━━━━━━━━━━━
+ do relay Int ?! IOError
    - ok Bool

= catch read(ok)

━━━━━━━━━━━━━━━━━━━━
+ do widen Float ?! Error
    - ok Bool

= catch read(ok)

━━━━━━━━━━━━━━━━━━━━
+ do guarded Int
    - ok Bool

do finally
    total += 1
. kept = catch read(ok)
= kept
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

```yumi
-- catch? chain(1)
-- catch? chain(-1)
-- catch? looped(3)
-- catch? looped(-3)
-- catch? early(0)
-- catch? early(4)
. raw = catch nested(2)
-- match raw
    | String
        "text $it"
    | IOError
        "io ${it.message}"
    | SyntaxError
        "syntax"
-- match catch nested(-2)
    | String
        "text $it"
    | IOError
        "io ${it.message}"
    | SyntaxError
        "syntax"
-- match catch nested(0)
    | String
        "text $it"
    | IOError
        "io ${it.message}"
    | SyntaxError
        "syntax ${it.message}"

━━━━━━━━━━━━━━━━━━━━
+ do check Int
    - value Int

?! IOError("negative $value")
    if value < 0
= value + 1

━━━━━━━━━━━━━━━━━━━━
+ do parse String
    - value Int

?! SyntaxError("zero")
    if value == 0
= "parsed $value"

━━━━━━━━━━━━━━━━━━━━
+ do chain Int
    - value Int

= check(check(check(value)))

━━━━━━━━━━━━━━━━━━━━
+ do looped Int
    - count Int

. total = 0
. index = 0
while index < 3
    total += check(count + index)
    index += 1
= total

━━━━━━━━━━━━━━━━━━━━
+ do early Int
    - value Int

if value == 0
    = check(-9)
. doubled = check(value) * 2
=? doubled if doubled > 100
= check(doubled)

━━━━━━━━━━━━━━━━━━━━
+ do nested String
    - value Int

. checked = check(value)
= parse(checked - 1)
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

```yumi
-- catch pick(0)
-- catch pick(1)
-- catch pick(2)
-- catch pick(3)
. kept = catch? pick(1)
-- kept == null
. held = catch? pick(0)
-- held == null

━━━━━━━━━━━━━━━━━━━━
+ Missing is Error

━━━━━━━━━━━━━━━━━━━━
+ Denied is Error

━━━━━━━━━━━━━━━━━━━━
+ Wrapped is Error

━━━━━━━━━━━━━━━━━━━━
+ do read Int ?! Missing ?! Denied ?! IOError
    - outcome Int

if outcome == 1
    ?! Missing("missing")
if outcome == 2
    ?! Denied("denied")
if outcome == 3
    ?! IOError("io")
= 7

━━━━━━━━━━━━━━━━━━━━
+ do pick
    - outcome Int

. handled = read(outcome)
    catch err Missing
        Wrapped("wrapped ${err.message}")
    catch Denied
        40 + 2
-- "after $outcome"
= handled
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

```yumi
-- catch both(0)
-- catch both(1)
-- catch both(2)

━━━━━━━━━━━━━━━━━━━━
+ Missing is Error

━━━━━━━━━━━━━━━━━━━━
+ Denied is Error

━━━━━━━━━━━━━━━━━━━━
+ First is Error

━━━━━━━━━━━━━━━━━━━━
+ Second is Error

━━━━━━━━━━━━━━━━━━━━
+ do read String ?! Missing ?! Denied
    - outcome Int

if outcome == 1
    ?! Missing("missing")
if outcome == 2
    ?! Denied("denied")
= "read"

━━━━━━━━━━━━━━━━━━━━
+ do both
    - outcome Int

. handled = read(outcome)
    catch err Missing
        First("first ${err.message}")
    catch err Denied
        Second("second ${err.message}")
-- "after"
= handled
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

```yumi crash
all(0)
. seen = catch all(1)
do match seen
    | err Wrapped
        -- "wrapped: ${err.message}"
    | Void
        -- "nothing"
. other = catch? all(1)
-- other == null
all(1)
-- "unreached"

━━━━━━━━━━━━━━━━━━━━
+ Missing is Error

━━━━━━━━━━━━━━━━━━━━
+ Wrapped is Error

━━━━━━━━━━━━━━━━━━━━
+ do read ?! Missing
    - outcome Int

if outcome == 1
    ?! Missing("missing")
-- "read ran"

━━━━━━━━━━━━━━━━━━━━
+ do all
    - outcome Int

. handled = read(outcome)
    catch err Missing
        Wrapped("wrapped ${err.message}")
-- "after"
= handled
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

```yumi crash
-- outer(1)
-- outer(20)
. got = catch? outer(-5)
-- got == null
. other = catch outer(-1)
do match other
    | err Small
        -- "main ${err.message}"
    | number Int
        -- number
-- outer(-1)

━━━━━━━━━━━━━━━━━━━━
+ Small is Error

━━━━━━━━━━━━━━━━━━━━
+ Large is Error

━━━━━━━━━━━━━━━━━━━━
+ do make Int ?! Small
    - value Int

if value < 0
    ?! Small("small $value")
= value * 2

━━━━━━━━━━━━━━━━━━━━
+ do take Int ?! Large
    - value Int
    - extra Int

if value > 10
    ?! Large("large $value")
= value + extra

━━━━━━━━━━━━━━━━━━━━
+ do outer Int ?! Small
    - value Int

. held = catch take(make(value), make(1))
do match held
    | err Large
        -- "caught ${err.message}"
        = -1
    | number Int
        = number
```

```output
4
caught large 40
-1
true
main small -1
```

### An argument of a method call under an optional catch raises past the catch

```yumi crash
-- run(1)
-- run(20)
. lost = catch? run(-1)
-- lost == null
-- run(-1)

━━━━━━━━━━━━━━━━━━━━
+ Small is Error

━━━━━━━━━━━━━━━━━━━━
+ Large is Error

━━━━━━━━━━━━━━━━━━━━
+ Box

+ value Int

────────────────────
+ This
    - value

────────────────────
+ do grow Int ?! Large
    - by Int

if value + by > 10
    ?! Large("large")
= value + by

━━━━━━━━━━━━━━━━━━━━
+ do make Int ?! Small
    - value Int

if value < 0
    ?! Small("small")
= value

━━━━━━━━━━━━━━━━━━━━
+ do run Int ?! Small
    - value Int

. box = Box(1)
. grown = catch? box.grow(make(value))
if grown == null
    = -1
= 0
```

```output
0
-1
true
```

### Handlers in order where a later class is the base of an earlier one, joined with a number of another kind

```yumi
-- catch chain(0)
-- catch chain(1)
-- catch chain(2)

━━━━━━━━━━━━━━━━━━━━
+ Missing is Error

━━━━━━━━━━━━━━━━━━━━
+ Denied is Missing

━━━━━━━━━━━━━━━━━━━━
+ Wrapped is Error

━━━━━━━━━━━━━━━━━━━━
+ Deep is Wrapped

━━━━━━━━━━━━━━━━━━━━
+ do read Float ?! Missing
    - outcome Int

if outcome == 1
    ?! Missing("missing")
if outcome == 2
    ?! Denied("denied")
= 1.5

━━━━━━━━━━━━━━━━━━━━
+ do chain
    - outcome Int

. handled = read(outcome)
    catch err Denied
        Deep("deep ${err.message}")
    catch err Missing
        3i32
-- "after"
= handled
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

```yumi
. first = inner(1)
    catch err Missing
        Wrapped("top ${err.message}")
do match first
    | err Wrapped
        -- "is wrapped ${err.message}"
    | number Int
        -- "is number $number"
. second = inner(0)
    catch err Missing
        Wrapped("top ${err.message}")
do match second
    | err Wrapped
        -- "is wrapped ${err.message}"
    | number Int
        -- "is number $number"

━━━━━━━━━━━━━━━━━━━━
+ Missing is Error

━━━━━━━━━━━━━━━━━━━━
+ Wrapped is Error

━━━━━━━━━━━━━━━━━━━━
+ do inner Int ?! Missing
    - outcome Int

if outcome == 1
    ?! Missing("missing")
= 9
```

```output
is wrapped top missing
is number 9
```

### A pattern and a test of a class on a caught value that may be null or an error

```yumi
-- name(0)
-- name(1)
-- name(2)
-- size(0)
-- size(1)
-- size(2)

━━━━━━━━━━━━━━━━━━━━
+ Box

+ label String

────────────────────
+ This
    - label

━━━━━━━━━━━━━━━━━━━━
+ do find Box? ?! IOError
    - kind Int

if kind == 0
    ?! IOError("io")
if kind == 1
    = Box("held")
= null

━━━━━━━━━━━━━━━━━━━━
+ do name String
    - kind Int

. held = catch find(kind)
do match? held
    | box Box
        = box.label
    | err IOError
        = "error ${err.message}"
= "empty"

━━━━━━━━━━━━━━━━━━━━
+ do size Int
    - kind Int

. held = catch find(kind)
if held is Box
    = 1
= 0
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

```yumi crash
-- made(3)
-- made(-3)
-- kept(4)
-- kept(-4)
. lost = catch? Pair(-1)
-- lost == null
. found = catch? Pair(6)
-- found == null
-- Pair(-9).left
-- "unreached"

━━━━━━━━━━━━━━━━━━━━
+ Pair is Tuple

+ left Int
+ right Int

────────────────────
+ This ?! IOError
    - seed Int

if seed < 0
    ?! IOError("negative $seed")
left = seed
right = seed * 2

━━━━━━━━━━━━━━━━━━━━
+ do made Int
    - seed Int

. pair = Pair(seed)
    catch IOError
        Pair(1)
= pair.left + pair.right

━━━━━━━━━━━━━━━━━━━━
+ do kept String
    - seed Int

. held = catch Pair(seed)
= match held
    | pair Pair
        "pair ${pair.right}"
    | err IOError
        "error ${err.message}"
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

```yumi crash
-- built(2)
-- built(-2)
-- built(50)
-- deep(1)
-- deep(-1)
Holder(Part(-7))
-- "unreached"

━━━━━━━━━━━━━━━━━━━━
+ Part

+ size Int

────────────────────
+ This ?! IOError ?! SyntaxError
    - wanted Int

if wanted < 0
    ?! IOError("negative")
check(wanted)
size = wanted

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ part Part
+ note = "none"

────────────────────
+ This
    - part

note = "holds ${part.size}"

━━━━━━━━━━━━━━━━━━━━
+ do check ?! SyntaxError
    - wanted Int

if wanted > 10
    ?! SyntaxError("too large")

━━━━━━━━━━━━━━━━━━━━
+ do built String
    - wanted Int

. part = Part(wanted)
    catch err IOError
        = "io ${err.message}"
    catch err SyntaxError
        = "syntax ${err.message}"
= "size ${part.size}"

━━━━━━━━━━━━━━━━━━━━
+ do deep String
    - wanted Int

. holder = catch? Holder(Part(wanted))
=? "lost" if holder == null
= holder.note
```

```output
size 2
io negative
syntax too large
holds 1
```

### Constructors that raise beside a return of nothing and a value of their own

```yumi
-- count(1)
-- count(-1)
-- own(2)
-- own(-2)

━━━━━━━━━━━━━━━━━━━━
+ Counted

+ label = "plain"

────────────────────
+ This ?! IOError
    - seed Int

if seed < 0
    ?! IOError("negative")
if seed > 100
    =
label = "seeded"

━━━━━━━━━━━━━━━━━━━━
+ Code is Tuple

+ width = 0
+ height = 0

────────────────────
+ This ?! IOError
    - seed Int

if seed < 0
    ?! IOError("no code")
= Code(seed, seed + 1)

━━━━━━━━━━━━━━━━━━━━
+ do count String
    - seed Int

. held = catch Counted(seed)
= match held
    | counted Counted
        counted.label
    | err IOError
        err.message

━━━━━━━━━━━━━━━━━━━━
+ do own String
    - seed Int

. code = Code(seed)
    catch err IOError
        = err.message
= "code ${code.width} ${code.height}"
```

```output
seeded
negative
code 2 3
no code
```

### A value converted by a function that raises: the error leaves from a declaration, a return and a cast, and a text written in source panics

```yumi
-- width("abc")
-- declared(true)
-- held("toolong")
-- held("abc")
. lost = catch? parse("x")
-- lost == null
-- cast("abc")
. attempt = do new
    -- declared(false)
    -- "unreached"
catch failure
    -- failure
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ Code is Tuple

+ width = 0
+ height = 0

────────────────────
+ do from text String

?! SyntaxError("Invalid code $text")
    if text.length != 3
= Code(text.length, 1)

━━━━━━━━━━━━━━━━━━━━
+ do width Int
    - text String

. code Code = text
= code.width

━━━━━━━━━━━━━━━━━━━━
+ do cast Int
    - text String

= (text as Code).width

━━━━━━━━━━━━━━━━━━━━
+ do declared Int
    - good Bool

if good
    . code Code = "abc"
    = code.width
. code Code = "toolong"
= code.height

━━━━━━━━━━━━━━━━━━━━
+ do parse Code
    - text String

= text

━━━━━━━━━━━━━━━━━━━━
+ do held String
    - text String

. result = catch parse(text)
= match result
    | err SyntaxError
        "error ${err.message}"
    | code Code
        "code ${code.width}"
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

```yumi crash
-- size("abc")
-- taken("abc")
-- described("abc")
-- described("toolong")
-- made("abc")
-- made("no")
-- size("toolong")
-- "unreached"

━━━━━━━━━━━━━━━━━━━━
+ Code is Tuple

+ width = 0
+ height = 0

────────────────────
+ do from text String

?! SyntaxError("Invalid code $text")
    if text.length != 3
= Code(text.length, 1)

━━━━━━━━━━━━━━━━━━━━
+ do take Int
    - code Code

= code.width + code.height

━━━━━━━━━━━━━━━━━━━━
+ do size Int
    - text String

= take(text)

━━━━━━━━━━━━━━━━━━━━
+ do taken Int
    - text String

. code Code = Code(0, 0)
code = text
= take(code)

━━━━━━━━━━━━━━━━━━━━
+ do described String
    - text String

. result = catch size(text)
= match result
    | err SyntaxError
        "error ${err.message}"
    | total Int
        "total $total"

━━━━━━━━━━━━━━━━━━━━
+ do made String
    - text String

. result = catch Code(text)
= match result
    | err SyntaxError
        "error ${err.message}"
    | code Code
        "code ${code.width}"
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

```yumi crash
-- direct("abc")
-- optional("abc")
-- optional("toolong")
-- literal()
-- direct("toolong")
-- "unreached"

━━━━━━━━━━━━━━━━━━━━
+ Code is Tuple

+ width = 0
+ height = 0

────────────────────
+ do from text String

?! SyntaxError("Invalid code $text")
    if text.length != 3
= Code(text.length, 1)

━━━━━━━━━━━━━━━━━━━━
+ do direct Int
    - text String

. code = Code(text)
= code.width

━━━━━━━━━━━━━━━━━━━━
+ do optional Int
    - text String

. code = catch? Code(text)
=? -1 if code == null
= code.width

━━━━━━━━━━━━━━━━━━━━
+ do literal String

. result = catch Code("toolong")
= match result
    | err SyntaxError
        "error ${err.message}"
    | code Code
        "code ${code.width}"
```

```output
3
3
-1
error Invalid code toolong
```

### A join of a text written in source with another text, both converted by a function that raises

```yumi
-- mixed(true, "abc").width
-- mixed(false, "abc").width

━━━━━━━━━━━━━━━━━━━━
+ Code is Tuple

+ width = 0
+ height = 0

────────────────────
+ do from text String

?! SyntaxError("Invalid code $text")
    if text.length != 3
= Code(text.length, 1)

━━━━━━━━━━━━━━━━━━━━
+ do mixed Code
    - good Bool
    - text String

= if good then "abc" else text
```

```output
3
3
```

### A text written in source on either side of a conditional panics where its conversion fails

```yumi
-- joined(false).width
-- other(true).width
. attempt = do new
    -- joined(true).width
    -- "unreached"
catch failure
    -- failure
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ Code is Tuple

+ width = 0
+ height = 0

────────────────────
+ do from text String

?! SyntaxError("Invalid code $text")
    if text.length != 3
= Code(text.length, 1)

━━━━━━━━━━━━━━━━━━━━
+ do joined Code
    - bad Bool

= if bad then "toolong" else Code(9, 9)

━━━━━━━━━━━━━━━━━━━━
+ do other Code
    - good Bool

= if good then Code(8, 8) else "toolong"
```

```output
9
8
Invalid code toolong
```

### A text written in source as an argument, a property's value and an assignment, converted by a function that raises

```yumi
-- take("abc")
-- stored().width
. attempt = do new
    -- take("toolong")
    -- "unreached"
catch failure
    -- failure
attempt.wait()

━━━━━━━━━━━━━━━━━━━━
+ Code is Tuple

+ width = 0
+ height = 0

────────────────────
+ do from text String

?! SyntaxError("Invalid code $text")
    if text.length != 3
= Code(text.length, 1)

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ code Code = "abc"

━━━━━━━━━━━━━━━━━━━━
+ do sum Int
    - code Code

= code.width + code.height

━━━━━━━━━━━━━━━━━━━━
+ do take Int
    - label String

if label == "abc"
    = sum("abc")
= sum("toolong")

━━━━━━━━━━━━━━━━━━━━
+ do stored Code

. holder = Holder()
holder.code = "xyz"
= holder.code
```

```output
4
3
Invalid code toolong
```

### A call of a type with a text written in source panics where the conversion fails and nothing recovers

```yumi crash
-- made().width
-- broken().width
-- "unreached"

━━━━━━━━━━━━━━━━━━━━
+ Code is Tuple

+ width = 0
+ height = 0

────────────────────
+ do from text String

?! SyntaxError("Invalid code $text")
    if text.length != 3
= Code(text.length, 1)

━━━━━━━━━━━━━━━━━━━━
+ made Code

Code("abc")

━━━━━━━━━━━━━━━━━━━━
+ do broken Code

. code = Code("toolong")
= code
```

```output
3
```

### A function whose value is a raise, and a table of conditions that ends in one

```yumi
-- catch? fail() ?? "caught"
-- catch? digit(10) ?? -1
-- catch? digit(40) ?? -1
-- catch? digit(99) ?? -1
-- catch? later(1) ?? "caught"
-- catch? later(2) ?? "caught"

━━━━━━━━━━━━━━━━━━━━
+ Broken is Error

━━━━━━━━━━━━━━━━━━━━
+ fail String ?! Broken

?! Broken("always")

━━━━━━━━━━━━━━━━━━━━
+ digit Int ?! Broken
    - code Int

|? code >= 0 && code <= 25
    code + 100
|? code >= 26 && code <= 51
    code + 200
?! Broken("no digit")

━━━━━━━━━━━━━━━━━━━━
+ later String ?! Broken
    - step Int

-- "step $step"
?! Broken("after printing")
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

```yumi
run()
-- "after"

━━━━━━━━━━━━━━━━━━━━
+ do run

fail()
    catch error IOError
        note(error)

━━━━━━━━━━━━━━━━━━━━
+ do note
    - error Error

-- "noted ${error.message}"

━━━━━━━━━━━━━━━━━━━━
+ do fail

?! IOError("bad")
```

```output
noted bad
after
```

### unhandled-error prints an error as an unhandled error prints, and the program goes on.

```yumi crash
report(fail)
-- "after"
?! IOError("last")

━━━━━━━━━━━━━━━━━━━━
+ do report
    - action Doer

action()
    catch error Error
        unhandled-error(error)

━━━━━━━━━━━━━━━━━━━━
+ do fail

?! IOError("first")
```

```stderr
Unhandled error: first
<no file:17>

Task 1

Stack trace:
  0  __source__:fail no file:17:1
  1  __source__:failCasting no file:2:8
  2  __source__:report no file:10:1
  3  __source__:main no file:2:1
  4  __source__:init.__source__ no file
  5  __source__:brix.main no file

Unhandled error: last
<no file:4>

Task 1

Stack trace:
  0  __source__:main no file:4:1
  1  __source__:init.__source__ no file
  2  __source__:brix.main no file
```

### A program replaces unhandled-error, and an error handed to it goes there.

```yumi
. count = 0
unhandled-error = :: error => do
    count += 1
    -- "handled ${error.message}"
report(fail)
report(succeed)
-- count

━━━━━━━━━━━━━━━━━━━━
+ do report
    - action Doer

action()
    catch error Error
        unhandled-error(error)

━━━━━━━━━━━━━━━━━━━━
+ do fail

?! IOError("failed")

━━━━━━━━━━━━━━━━━━━━
+ do succeed

-- "succeeded"
```

```output
handled failed
succeeded
1
```

### An index that a function answers and that raises lets its error out, or a catch holds it.

```yumi
. t = Indexed()
-- catch? t[2] ?? -1
-- catch? t[8] ?? -1
. r = catch t[9]
-- r is Problem
-- read(t, 1)
-- catch read(t, 5)

━━━━━━━━━━━━━━━━━━━━
+ Problem is Error

━━━━━━━━━━━━━━━━━━━━
+ Indexed

────────────────────
+ do this[index Int] Int ?! Problem

if index > 3
    ?! Problem("index $index")
= index * 2

━━━━━━━━━━━━━━━━━━━━
+ do read Int ?! Problem
    - t Indexed
    - i Int

= t[i] + 1
```

```output
4
-1
true
3
index 5
```

### An error an index raises that nothing handles names the index function in its trace.

```yumi crash
. t = Indexed()
-- t[1]
-- t[9]

━━━━━━━━━━━━━━━━━━━━
+ Problem is Error

━━━━━━━━━━━━━━━━━━━━
+ Indexed

────────────────────
+ do this[index Int] Int ?! Problem

if index > 3
    ?! Problem("index $index")
= index * 2
```

```stderr
Unhandled error: index 9
<no file:16>

Task 1

Stack trace:
  0  __source__:Indexed:[] no file:16:5
  1  __source__:main no file:4:4
  2  __source__:init.__source__ no file
  3  __source__:brix.main no file
```

### A handler given as a value is a block of its own: a use read in it ends where it does.

```yumi
. r = make(true)
    catch err Error
        t.n + 10
        .. t = use Thing(1)
-- "r $r"
. s = make(false)
    catch err Error
        t.n
        .. t = use Thing(2)
-- "s $s"

━━━━━━━━━━━━━━━━━━━━
+ do make Int ?! Error
    - fails Bool

if fails
    ?! Error("bad")
= 5

━━━━━━━━━━━━━━━━━━━━
+ Thing

+ n = 0

────────────────────
+ This
    - n

-- "hello $n"

────────────────────
+ begin-using

this

────────────────────
+ do end-using

-- "bye $n"
```

```output
hello 1
bye 1
r 11
s 5
```

### A cast out of a union that may hold an error takes the case it holds.

```yumi
. a = catch List'Int(items(true))
-- a is Problem
. b = catch List'Int(items(false))
-- (b as [Int]).length
. c = catch first(items(true))
-- c

━━━━━━━━━━━━━━━━━━━━
+ Problem is Error

━━━━━━━━━━━━━━━━━━━━
+ do items
    - fail Bool

<= 1
if fail
    ?! Problem("bad")
<= 2

━━━━━━━━━━━━━━━━━━━━
+ do first Int
    - stream <Int>

for item in stream
    = item
= 0
```

```output
true
2
1
```

### A transformer whose catcher names a chain of `previous` takes only an error whose chain matches, and sets the `previous` of what it raises.

```yumi
. a = catch load(0)
-- a
-- (a as AppError).previous
. b = catch load(1)
-- b

━━━━━━━━━━━━━━━━━━━━
+ AppError is Error

━━━━━━━━━━━━━━━━━━━━
+ ConfigError is Error

━━━━━━━━━━━━━━━━━━━━
+ do read Int ?! IOError
    - n Int

if n == 0
    ?! IOError("disk")
= n

━━━━━━━━━━━━━━━━━━━━
+ do parse Int ?! ConfigError
    - n Int

?! ConfigError("config")
    catch err IOError

if n == 1
    ?! ConfigError("plain")
= read(n)

━━━━━━━━━━━━━━━━━━━━
+ do load Int ?! AppError ?! ConfigError
    - n Int

?! AppError("from io")
    catch err ConfigError <- io IOError

= parse(n)
```

```output
from io
config
plain
```

### Transformers are tried in the order written: one whose chain matches, else one that takes the error whole.

```yumi
. a = catch load(0)
-- a
. b = catch load(1)
-- b

━━━━━━━━━━━━━━━━━━━━
+ AppError is Error

━━━━━━━━━━━━━━━━━━━━
+ ConfigError is Error

━━━━━━━━━━━━━━━━━━━━
+ do read Int ?! IOError
    - n Int

if n == 0
    ?! IOError("disk")
= n

━━━━━━━━━━━━━━━━━━━━
+ do parse Int ?! ConfigError
    - n Int

?! ConfigError("config")
    catch err IOError

if n == 1
    ?! ConfigError("plain")
= read(n)

━━━━━━━━━━━━━━━━━━━━
+ do load Int ?! AppError
    - n Int

?! AppError("wrapped ${err.message}")
    catch err ConfigError <- io IOError

?! AppError("plain ${other.message}")
    catch other ConfigError

= parse(n)
```

```output
wrapped config
plain plain
```

### A transformer takes the errors raised after it, and not those raised before it.

```yumi
-- catch load(0)
-- catch load(7)

━━━━━━━━━━━━━━━━━━━━
+ AppError is Error

━━━━━━━━━━━━━━━━━━━━
+ ConfigError is Error

━━━━━━━━━━━━━━━━━━━━
+ do read Int ?! IOError
    - n Int

if n == 0
    ?! IOError("disk")
= n

━━━━━━━━━━━━━━━━━━━━
+ do parse Int ?! ConfigError
    - n Int

?! ConfigError("config")
    catch err IOError

if n == 1
    ?! ConfigError("plain")
= read(n)

━━━━━━━━━━━━━━━━━━━━
+ do load Int ?! AppError ?! IOError
    - n Int

if n == 7
    ?! IOError("early")
?! AppError("late")
    catch err IOError

if n == 0
    ?! IOError("raised")
= n
```

```output
late
early
```

### A transformer makes its exception before the cleanups open where the error leaves run.

```yumi
-- catch load(0)

━━━━━━━━━━━━━━━━━━━━
+ AppError is Error

━━━━━━━━━━━━━━━━━━━━
+ ConfigError is Error

━━━━━━━━━━━━━━━━━━━━
+ do read Int ?! IOError
    - n Int

if n == 0
    ?! IOError("disk")
= n

━━━━━━━━━━━━━━━━━━━━
+ do parse Int ?! ConfigError
    - n Int

?! ConfigError("config")
    catch err IOError

if n == 1
    ?! ConfigError("plain")
= read(n)

━━━━━━━━━━━━━━━━━━━━
+ do note String
    - text String

-- text
= text

━━━━━━━━━━━━━━━━━━━━
+ do load Int ?! AppError
    - n Int

do finally
    -- "cleanup"
?! AppError(note("making"))
    catch err IOError

= read(n)
```

```output
making
cleanup
making
```

### A transformer in a method takes the error of a call and the error a raise gives where it is written.

```yumi
-- catch Loader().load(0)
-- catch Loader().load(4)

━━━━━━━━━━━━━━━━━━━━
+ AppError is Error

━━━━━━━━━━━━━━━━━━━━
+ do read Int ?! IOError
    - n Int

if n == 0
    ?! IOError("disk")
= n

━━━━━━━━━━━━━━━━━━━━
+ Loader

────────────────────
+ do load Int ?! AppError
    - n Int

?! AppError("method ${err.message}")
    catch err IOError

?! IOError("conditional")
    if n == 4
= read(n)
```

```output
method disk
method conditional
```

### A transformer in a generator takes the error an item raises.

```yumi
-- catch collect()

━━━━━━━━━━━━━━━━━━━━
+ AppError is Error

━━━━━━━━━━━━━━━━━━━━
+ do read Int ?! IOError
    - n Int

if n == 0
    ?! IOError("disk")
= n

━━━━━━━━━━━━━━━━━━━━
+ do numbers <Int> ?! AppError

?! AppError("gen")
    catch err IOError

<= 1
<= read(0)
<= 3

━━━━━━━━━━━━━━━━━━━━
+ do collect [Int] ?! AppError

. values [Int] = List'Int()
for value in numbers()
    values += value
= values
```

```output
gen
```

### A named handler for a base error type covers every error type of its call.

```yumi
. text = "none"
risky(1)
    catch failure Error
        text = "$failure"
-- text
risky(2)
    catch failure Error
        text = "$failure"
-- text
risky(3)
    catch failure Error
        text = "$failure"
-- text

━━━━━━━━━━━━━━━━━━━━
+ FirstError is Error

━━━━━━━━━━━━━━━━━━━━
+ SecondError is Error

━━━━━━━━━━━━━━━━━━━━
+ do risky ?! FirstError ?! SecondError
    - value Int

?! FirstError("first")
    if value == 1
?! SecondError("second")
    if value == 2
```

```output
first
second
second
```

### A named handler for a base error type can update a local inside a method.

```yumi
. holder = Holder()
holder.run(2)
-- holder.text

━━━━━━━━━━━━━━━━━━━━
+ FirstError is Error

━━━━━━━━━━━━━━━━━━━━
+ SecondError is Error

━━━━━━━━━━━━━━━━━━━━
+ Holder

+ text = "none"

────────────────────
+ do run
    - value Int

. problem String?
risky(value)
    catch failure Error
        problem = "$failure"
text = problem ?? "fine"

────────────────────
_ do risky ?! FirstError ?! SecondError
    - value Int

?! FirstError("first")
    if value == 1
?! SecondError("second")
    if value == 2
```

```output
second
```

### A held result can be matched with one named pattern for a base error type.

```yumi
. result = catch risky(2)
-- match result
    | value Int
        "value $value"
    | failure Error
        "failed $failure"

━━━━━━━━━━━━━━━━━━━━
+ FirstError is Error

━━━━━━━━━━━━━━━━━━━━
+ SecondError is Error

━━━━━━━━━━━━━━━━━━━━
+ do risky Int ?! FirstError ?! SecondError
    - value Int

?! FirstError("first")
    if value == 1
?! SecondError("second")
    if value == 2
= value
```

```output
failed second
```

### A named handler for a base error type can replace the value of its call.

```yumi
. text = risky(2)
    catch failure Error
        "failed $failure"
-- text

━━━━━━━━━━━━━━━━━━━━
+ FirstError is Error

━━━━━━━━━━━━━━━━━━━━
+ SecondError is Error

━━━━━━━━━━━━━━━━━━━━
+ do risky String ?! FirstError ?! SecondError
    - value Int

?! FirstError("first")
    if value == 1
?! SecondError("second")
    if value == 2
= "fine"
```

```output
failed second
```

### An action of a match that may panic raises an error its function states.

```yumi
for input in ["12", "9", "x"]
    -- catch parse(input)

━━━━━━━━━━━━━━━━━━━━
+ parse Int ?! SyntaxError
    - text String

match text
    ~ digits [0-9]+
        check(digits.codepoints[0]) + digits.length
    ?? 0

━━━━━━━━━━━━━━━━━━━━
+ check Int ?! SyntaxError
    - char Char

if char != '9' then (char as Int) else reject("Nine")

━━━━━━━━━━━━━━━━━━━━
+ reject Int ?! SyntaxError
    - message String

?! SyntaxError(message)
```

```output
51
Nine
0
```

### Statement-level calls with attached guards each bind their own `it`.

```yumi
-- checked(2)
-- catch checked(-1)

━━━━━━━━━━━━━━━━━━━━
+ do checked String ?! Error
    - second Int

step(1)
    ?! Error("first")
        if it == -1
step(second)
    ?! Error("second")
        if it == -1
= "ok"

━━━━━━━━━━━━━━━━━━━━
+ step
    - value Int

value
```

```output
ok
second
```

### An attached catch handles errors from a throwing getter as it does from a call.

```yumi
. box = Box()
. value = box.text
    catch error IOError
        "caught $error"
-- value

━━━━━━━━━━━━━━━━━━━━
+ Box

────────────────────
+ do get text String

?! IOError("boom")
```

```output
caught boom
```

### An attached catch passes through a null returned by a fallible function with an optional result.

```yumi
. found = find(false)
    catch Error
        null
-- "${found == null}"

━━━━━━━━━━━━━━━━━━━━
+ Box

━━━━━━━━━━━━━━━━━━━━
+ do find Box? ?! Error
    - fails Bool

?! Error("lost")
    if fails
= null
```

```output
true
```

### A call through an optional chain may fail, and a catch attached to it takes its errors. Where the chain makes no call nothing is raised, and the chain gives nothing.

```yumi
. gate Gate? = Gate()
. none Gate? = null
gate?.shut()
    catch failure Error
        -- "caught ${failure.message}"
none?.shut()
    catch failure Error
        -- "caught ${failure.message}"
. count = gate?.count()
    catch Error
        -1
-- "$count"
. missing = none?.count()
    catch Error
        -1
-- "${missing == null}"

━━━━━━━━━━━━━━━━━━━━
+ Gate

────────────────────
+ do shut ?! Error

?! Error("stuck")

────────────────────
+ do count Int ?! Error

?! Error("bad")
```

```output
caught stuck
-1
true
```

### A function that states an error may let out an error of a class above it, which converts to the error it states.

```yumi
. found = catch open-it(3)
-- found is I64

━━━━━━━━━━━━━━━━━━━━
+ BadPath is Error

━━━━━━━━━━━━━━━━━━━━
+ do open-it I64 ?! BadPath
    - seed I64

. found = risky(seed)
= found + 1

━━━━━━━━━━━━━━━━━━━━
+ do risky I64
    - seed I64

if seed < 0
    ?! Error("negative")
= seed
```

```output
true
```

### A handler whose class can match neither the value nor an error of the call is passed over.

```yumi
for n in events(false)
    -- n
. failed = catch first-event(true)
-- failed is Wrapped

━━━━━━━━━━━━━━━━━━━━
+ Wrapped is Error

━━━━━━━━━━━━━━━━━━━━
+ do fetch Int ?! Wrapped
    - fail Bool

?! Wrapped("bad")
    if fail
= 4

━━━━━━━━━━━━━━━━━━━━
+ do first-event Int ?! Wrapped
    - fail Bool

for n in events(fail)
    = n
= 0

━━━━━━━━━━━━━━━━━━━━
+ do events <Int>
    - fail Bool

. response = fetch(fail)
    catch canceled TaskCancelationError
        ?! Wrapped("$canceled")
<= response
<= response + 1
```

```output
4
5
true
```

### An error of a class above the one a function states is cast to that one as it leaves, once the cleanups open have run.

```yumi
do new
    -- catch? risky(1) ?? -1
    -- catch? risky(0) ?? -1
catch failure
    -- failure

━━━━━━━━━━━━━━━━━━━━
+ Failed is Error

━━━━━━━━━━━━━━━━━━━━
+ do risky I32 ?! Failed
    - count I32

do finally
    -- "cleanup ${count}"
if count == 0
    ?! Error("plain")
= count
```

```output
cleanup 1
1
cleanup 0
Invalid cast to [Object __source__:Failed]
```

### A function that raises errors may return an error as its value.

```yumi
-- catch? describe(3) ?? "failed"
-- catch? describe(-1) ?? "failed"
-- catch? describe(0) ?? "failed"

━━━━━━━━━━━━━━━━━━━━
+ CodeError is Error

+ code I64

────────────────────
+ This
    - code

message = "code $code"

━━━━━━━━━━━━━━━━━━━━
_ count-of I64 ?! CodeError
    - n I64

?! CodeError(n) if n < 0
n * 2

━━━━━━━━━━━━━━━━━━━━
_ make ?! CodeError
    - n I64

CodeError
    - count-of(n)

━━━━━━━━━━━━━━━━━━━━
_ do describe String ?! CodeError
    - n I64

. made = make(n)
= "${made.message} ${made.code}"
```

```output
code 6 6
failed
code 0 0
```

### An error of a class above those a function raises leaves it cast to the first of them below it, in their order, not as written.

```yumi
-- describe(0)
-- describe(1)

━━━━━━━━━━━━━━━━━━━━
+ AErr is Error

━━━━━━━━━━━━━━━━━━━━
+ BErr is Error

━━━━━━━━━━━━━━━━━━━━
_ do raises ?! Error
    - n I64

if n == 1
    ?! AErr("a")

━━━━━━━━━━━━━━━━━━━━
_ do relay String ?! BErr ?! AErr
    - n I64

raises(n)
= "ok $n"

━━━━━━━━━━━━━━━━━━━━
_ do describe String
    - n I64

. got = catch relay(n)
= match got
    | AErr
        "A"
    | BErr
        "B"
    | text String
        text
```

```output
ok 0
A
```

### A conditional return of null from a function that returns a union with a Null case converts the null to the union.

```yumi
-- describe(pick(0))
-- describe(pick(1))
-- describe(pick(2))

━━━━━━━━━━━━━━━━━━━━
+ Reading is Union

| Null
| I64
| String

━━━━━━━━━━━━━━━━━━━━
_ do pick Reading
    - n I64

=? null if n == 0
=? "text" if n == 1
= n

━━━━━━━━━━━━━━━━━━━━
_ describe String
    - value Reading

|? value is Null
    "nothing"
|? value is String
    "text ${value as String}"
??
    "number ${value as I64}"
```

```output
nothing
text text
number 2
```

### A raise guarded by a condition is typed under what the condition finds: an optional error the condition tests is raised as the error it holds.

```yumi
. outcome = catch checked(Error("boom"))
-- outcome is Error
-- checked(null)

━━━━━━━━━━━━━━━━━━━━
_ do checked I64 ?! Error
    - failure Error?

?! failure
    if failure
= 1
```

```output
true
1
```

### A handler whose body is a block of two statements that give values, such as an assignment followed by a value, gives no value, so it can not stand for an expression that gives one.

```yumi crash
. problem = ""
. value = parse(-1)
    catch failure Error
        problem = failure.message
        0
-- "$value $problem"

━━━━━━━━━━━━━━━━━━━━
_ do parse I64 ?! Error
    - n I64

?! Error("negative")
    if n < 0
= n
```

```output
Exception: Illegal statement
<no file:4>
        catch failure Error
```

### A handler names the error type it takes, so a name alone is not a handler

```yumi crash
-- run(true)

━━━━━━━━━━━━━━━━━━━━
+ do run String
    - fails Bool

. items = read(fails)
    catch other
        = "other ${other.message}"
= "read ${items.length}"

━━━━━━━━━━━━━━━━━━━━
+ do read [Int] ?! Error
    - fails Bool

?! Error("nope")
    if fails
= [1, 2]
```

```output
Exception: Attached catch handler type must be an Error subtype
<no file:9>
        catch other
              ^^^^^
```

### A getter can declare an error it raises, which a read of the property handles.

```yumi
-- Box(-1).size
    catch error TypeError
        -1
-- Box(3).size
    catch error TypeError
        -1

━━━━━━━━━━━━━━━━━━━━
+ Box

+ raw Int

────────────────────
+ This
    - raw Int

────────────────────
+ do get size Int ?! TypeError

?! TypeError() if raw < 0
= raw
```

```output
-1
3
```
