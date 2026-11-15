# Errors

An error is a value a function raises in place of its result. Yumi has two kinds:

- **Recoverable errors** are part of a function's type. A function declares the errors it can
  raise, and a caller handles them or lets them pass on to its own caller.
- **Panics** are for mistakes, such as an index out of range. They need no declaration, pass
  through every caller and every handler, and end the task they happen in. Where the task was
  started, its `do-new` form can handle the failure.

This chapter covers error values, declaring and raising errors, how they leave a call, handling
them, cleanup, and panics. Errors in generators and pipelines are also in Streams, and errors in
tasks in Concurrency.

## Error values

An error is an object of the class `Error` or a class derived from it.

```brix
(main
  (let plain (call Error "disk full"))
  (print plain)
  (print (. plain message))
  (let missing (call NotFound "ada"))
  (print missing)
  (print (. missing name))
  (print (is missing Error))
)

(type + NotFound (is Error)
  (prop + name String)
  (fn-do + This _
    (param name _)
    (= message (str "no user named " name))
  )
)
```

```output
disk full
disk full
no user named ada
ada
true
```

| Member | Value |
| --- | --- |
| `message` | The error's text, `""` by default. Converting an error to text gives its message. |
| `source` | Where it was raised, an `ErrorSource` of file, line and column, or `null` before it is raised. |
| `previous` | The error it replaced, or `null`; see [Transforming errors](#transforming-errors). |
| `stack` | The stack it was raised on, as text, as a report prints it. |
| `raised-at` | When it was raised, in seconds since 1970 in UTC, or `null` before it is raised. |

- **Making one.** `(call Error message)`{.brix} makes an error, and so does a constructor of a
  derived class. Making an error does not raise it.
- **Declaring a kind of error.** `(type + NotFound (is Error))`{.brix} declares one. It may add
  properties, and a constructor may set `message`.

The core library declares these kinds of error:

| Class | Kind | Raised for |
| --- | --- | --- |
| `Panic` | Panic | A mistake in the program; the base of every panic. |
| `OutOfBoundsError` | Panic | An index outside a list, an array or a text. |
| `TaskCancelationError` | Panic | A canceled task; see Concurrency. |
| `TaskTerminationError` | Panic | A task ended because the process is ending. |
| `IOError` | Recoverable | A failed read or write. |
| `TimeoutError` | Recoverable | An `IOError` for an operation that took too long. |
| `SizeLimitError` | Recoverable | Input larger than a limit. |
| `SyntaxError` | Recoverable | Text that does not parse. |
| `TypeError` | Recoverable | A value of the wrong type. |
| `UnionError` | Recoverable | A failed conversion of a union; see Unions. |

## Declaring errors

A function names the errors it can raise after its result type, each in a `?!` form:

```brix
(main
  (print (call checked-ratio 6 3))
  (print (catch (call checked-ratio 6 0)))
  (print (call scaled 5 1))
  (print (catch (call scaled 5 0)))
)

(type + ZeroDivisor (is Error))

(fn-expr + checked-ratio Int (?! ZeroDivisor)
  (param top Int)
  (param bottom Int)
  (before
    (throw (call ZeroDivisor (str "cannot divide " top " by zero")) (if (== bottom 0)))
  )
  (/ top bottom)
)

(fn-do + scaled Int
  (param top Int)
  (param bottom Int)
  (let ratio (call checked-ratio top bottom))
  (print (str "ratio is " ratio))
  (return (* ratio 10))
)
```

```output
2
cannot divide 6 by zero
ratio is 5
50
cannot divide 5 by zero
```

- **Where.** `?!` follows the result type of a function, a method, a constructor, a generator or a
  function type, as in `(fn-do + read Int (?! IOError) (?! SyntaxError) body)`{.brix} and
  `(fn-type Int (?! IOError))`{.brix}. A function with no result writes `_`
  in its place.
- **Only errors.** Only `Error` and its derived classes may follow `?!`: `I64 is not a
  recoverable error type; only subclasses of Error may appear after ?!.` A panic is not declared:
  `Panic cannot be returned as a recoverable error; use ?! Panic(...) to end the task, or convert
  it explicitly.`
- **Classes cover their derived classes.** Declaring `Error` itself allows every recoverable
  error.
- **Inferred errors.** A function that declares no errors, such as `scaled` above, can raise
  whatever its body raises, and its callers know which. Function values and getters always infer
  their errors. Declaring a public function's errors anyway states the contract to its callers,
  and a change to its body that would raise another error is then caught where it is made.
- **Recursion.** A function that calls itself must declare its errors: `IOError can leave "walk",
  which calls itself, so its errors cannot be inferred; declare them on it, as in "(fn-do + walk
  Bool (?! IOError) …)", or catch them here`.
- **Overrides** may raise fewer errors than the method they override, not more, and so may a
  method that implements an interface's method: `"read" can fail with IOError, but the interface
  method it implements cannot; add "?! IOError" to that method, or catch the error inside "read"`.
  A function value converts to a function type only if the type declares every error the
  function can raise.

A function that declares its errors cannot let any other error out:

```brix compile-error
(main
  (print (call parse "7"))
)

(fn-expr + parse Int (?! SyntaxError)
  (param text String)
  (before
    (throw (call IOError "unreadable") (if (== text "")))
  )
  (as text Int)
)
```

```output
Exception: IOError can leave "parse", which does not declare it; declare it, as in "(fn-expr + parse Int (?! SyntaxError) (?! IOError) …)"
<no file:9>
        (throw (call IOError "unreadable") (if (== text "")))
        ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```

One exception: an error of a class *above* a declared class, such as a plain `Error` leaving a
function that declares `NotFound`, compiles. It is converted to the declared class as
it leaves, and panics then if it is not one: `Invalid cast to [Object __source__:NotFound]`.

## Raising

`(throw error)`{.brix} raises an error, and `(throw error (if condition))`{.brix} raises it only
when the condition holds.

- **Step bodies** may raise anywhere, including in loops, branches and cleanups.
- **Value bodies** raise in their `before` rows, which run before the value; see Functions. A
  `match` arm may also be a `throw`, and so may the whole body.
- **Narrowing.** With an optional error, `(throw failure (if failure))`{.brix} raises the error
  it holds.
- **Errors as values.** An error that is made and not thrown is an ordinary value: it can be
  stored, passed and returned without being raised.
- **After a call.** A `throw` in the block of a call row reads the call's result as `it`, as in
  `(call step 1 (block (throw (call Error "failed") (if (== it -1)))))`{.brix}.

## How errors leave

An error raised in a function ends the function, and every call it passes through, until
something handles it:

- **A call** whose error is not handled ends its own function the same way. Its cleanups run as it
  leaves; see [Cleanup](#cleanup).
- **Loops and blocks** end with the function that holds them.
- **`main`** may let any error leave. Its cleanups run, then the program prints a report on the
  error stream and exits with status 1.
- **A task's body** may let any recoverable error leave too. The task's cleanups run, and the
  program prints the report, which names the task and the task that started it, and exits with
  status 1.
- **Nothing is undone.** What a function changed before the error stays changed. A cleanup can
  undo it; see [Cleanup](#cleanup).

A call that can fail looks like any other. Most callers can only pass an error on, so the
language does that for them, and handling is written only where a caller does something with the
error. So a call does not show that it can end its function early: the function's declared errors
show it, and `yumi app.bx --explain name` lists each error a function can raise and the call it
comes from.

```brix crash-stderr
(main
  (call read)
  (print "never printed")
)

(fn-do + read _ (?! IOError)
  (throw (call IOError "bad disk"))
)
```

```output
Unhandled error: bad disk
<no file:8>

Task 1

Stack trace:
  0  __source__:read no file:8:3
  1  __source__:main no file:3:3
  2  __source__:init.__source__ no file
  3  __source__:yumi.main no file
```

## Handling errors

### `catch` and `catch?`

`(catch expr)`{.brix} gives a result: the value of `expr`, or the error it raised. The result
prints as either, `is` tests it, and `match` takes it apart. `(catch? expr)`{.brix} gives the
value, or `null` for any error. `catch?` drops which error it was and why, so it suits a place
where having no value is a fine answer; where failures need different treatment, use a handler or
`catch`.

```brix
(main
  (for (in name (list "ada" "bob" ""))
    (print (call describe name))
  )
  (print (?? (catch? (call lookup "ada")) -1))
  (print (?? (catch? (call lookup "bob")) -1))
  (let held (catch (call lookup "")))
  (print (is held BadName))
  (print (is held NotFound))
)

(type + NotFound (is Error)
  (prop + name String)
  (fn-do + This _
    (param name _)
    (= message (str "no user named " name))
  )
)

(type + BadName (is Error))

(fn-do + lookup Int (?! NotFound) (?! BadName)
  (param name String)
  (throw (call BadName "empty name") (if (== name "")))
  (if (== name "ada")
    (return 36)
  )
  (throw (call NotFound name))
)

(fn-expr + describe String
  (param name String)
  (match (catch (call lookup name))
    (case (: age Int)
      (str name " is " age)
    )
    (case (: missing NotFound)
      (str "missing: " (. missing name))
    )
    (case (: bad BadName)
      (str "bad: " (. bad message))
    )
  )
)
```

```output
ada is 36
missing: bob
bad: empty name
36
-1
true
false
```

- **Matching a result** must cover the value and every error the call can raise: `This match on
  Result does not handle IOError; add a case for it, or "??" for the rest`. A
  `(case (: e Error) body)`{.brix} arm covers every error.
- **Only the call's own errors.** `catch` holds the errors of the expression it wraps. An error
  raised by working out a call's arguments still leaves.
- **Panics** pass through `catch` and `catch?`.

### Handlers

A call can carry handlers in its block. Each `on-error` form names a class of error, and gives
the call's value in place of an error of that class.

```brix
(main
  (print (call load 0))
  (print (call load 1))
  (print (call load 2))
  (print (catch (call load 3)))
)

(type + FileMissing (is Error))

(type + AccessDenied (is Error))

(type + Corrupt (is Error))

(fn-do + read-config String (?! FileMissing) (?! AccessDenied) (?! Corrupt)
  (param outcome Int)
  (throw (call FileMissing "no file") (if (== outcome 1)))
  (throw (call AccessDenied "not yours") (if (== outcome 2)))
  (throw (call Corrupt "bad bytes") (if (== outcome 3)))
  (return "custom")
)

(fn-do + load String
  (param outcome Int)
  (let settings
    (call read-config outcome
      (block
        (on-error FileMissing
          "defaults"
        )
        (on-error (: err AccessDenied)
          (do
            (inline (print (str "denied: " (. err message))))
          )
          "guest"
        )
      )
    )
  )
  (return (str "using " settings))
)
```

```output
using custom
using defaults
denied: not yours
using guest
bad bytes
```

- **Naming the error.** `(on-error (: err AccessDenied) body)`{.brix} binds the error to `err`.
- **Order.** Handlers are tried in the order written. An error that no handler takes leaves as it
  would without them.
- **Bodies.** A handler's last row is its value, and a `do` form before it runs steps. A handler
  may also `return` from the function, `break` or `continue` a loop, or raise, including the same
  error again with `(throw err)`{.brix}.
- **Raising a new error.** `(on-error Missing (throw (call LoadFailed "could not load")))`{.brix}
  raises the new error. A handler whose value is an error, `(on-error Missing (call LoadFailed
  "could not load"))`, raises nothing: the call's value is then a result that may hold the error,
  and the function goes on with it.
- **The value** of a call with handlers is the join of its own value and the handlers' values.
- **Nothing to handle.** A handler on a call that cannot fail is an error: `"plain()" cannot fail,
  so the catch attached to it has nothing to handle; remove the catch, or attach it to a call
  that can fail`.

### Transforming errors

A `throw` with an `on-error` form and no body raises nothing where it is written. It replaces each
error of the named class that later leaves the function with a new error, and the new error's
`previous` is the original. So each layer can report a failure in its own terms and keep the
cause.

```brix
(main
  (let result (catch (call load-settings)))
  (print result)
  (print (. (as result SettingsError) previous))
)

(type + SettingsError (is Error))

(fn-do + read-file String (?! IOError)
  (throw (call IOError "disk unavailable"))
)

(fn-do + load-settings String (?! SettingsError)
  (throw (call SettingsError "could not load settings") (on-error (: err IOError)))
  (return (call read-file))
)
```

```output
could not load settings
disk unavailable
```

- **Scope.** A transformer applies to errors that leave after its row. The new error is made
  before the function's cleanups run.
- **Rules.** The new error's expression may not call a function that raises, so that translating
  one error cannot raise another, and two transformers that could take the same error are an
  error.

```brix
(main
  (print (catch (call load 0)))
  (print (catch (call load 1)))
)

(type + AppError (is Error))

(type + ConfigError (is Error))

(fn-do + read Int (?! IOError)
  (param n Int)
  (throw (call IOError "disk") (if (== n 0)))
  (return n)
)

(fn-do + parse Int (?! ConfigError)
  (param n Int)
  (throw (call ConfigError "config") (on-error (: err IOError)))
  (throw (call ConfigError "plain") (if (== n 1)))
  (return (call read n))
)

(fn-do + load Int (?! AppError)
  (param n Int)
  (throw (call AppError (str "from " (. io message))) (on-error (caught (: err ConfigError) (: io IOError))))
  (throw (call AppError (str "only " (. other message))) (on-error (: other ConfigError)))
  (return (call parse n))
)
```

```output
from disk
only plain
```

- **Chains.** `(caught (: err ConfigError) (: io IOError))` takes a `ConfigError` whose `previous`
  is an `IOError`, and names both. In rows it is `catch err ConfigError <- io IOError`{.rows}. A
  plain transformer for the same class after it takes the errors whose chain does not fit.

### Errors with no caller

Some errors have no caller to go to, such as one raised by a subscriber of a channel. These go to
`unhandled-error`, a function the program may replace; by default it prints the report and the
program goes on. See Concurrency.

## Cleanup

A `finally` form holds steps that run when the block it is written in ends, however it ends.

```brix
(main
  (print (call run false))
  (print (catch (call run true)))
)

(type + Failed (is Error))

(type + Resource
  (prop + name String)
  (fn-do + This _
    (param name _)
    (print (str "open " name))
  )
  (fn-expr + begin-using _
    this
  )
  (fn-do + end-using _
    (print (str "close " name))
  )
)

(fn-do + run Int (?! Failed)
  (param fails Bool)
  (finally
    (print "first cleanup")
  )
  (let a (use (call Resource "a")))
  (finally
    (print "second cleanup")
  )
  (let b (use (call Resource "b")))
  (throw (call Failed "work failed") (if fails))
  (print (str "working with " (. a name) " and " (. b name)))
  (return 1)
)
```

```output
open a
open b
working with a and b
close b
second cleanup
close a
first cleanup
1
open a
open b
close b
second cleanup
close a
first cleanup
work failed
```

- **When.** A cleanup runs when its block ends: at the end, at a `return`, `break` or
  `continue`, when an error leaves, or when a panic ends the task. A cleanup in a loop body runs on
  every pass.
- **Order.** Cleanups and uses run newest first.
- **Names.** A cleanup can read only the locals declared before it.
- **Return values** are worked out before the cleanups run. A cleanup cannot return a value:
  `Illegal deferred statement`.
- **Errors in cleanups.** An error raised by a cleanup replaces an error that was already leaving.
- **One row.** `(finally (inline (print "done")))`{.brix} holds one step.

A cleanup is not a rollback: it runs however the block ends, and undoes only what it is written to
undo. To undo partial work, set a local when the work is done, and have the cleanup undo it only
when the local is not set:

```brix
(main
  (let first (list "tea"))
  (print (catch (call add-all first (list "milk" "" "bread"))))
  (print first)
  (let second (list "tea"))
  (print (catch (call add-all-or-none second (list "milk" "" "bread"))))
  (print second)
  (call add-all-or-none second (list "milk" "bread"))
  (print second)
)

(type + BadItem (is Error))

(fn-do + add-all _ (?! BadItem)
  (param cart (list String))
  (param items (list String))
  (for (in item items)
    (throw (call BadItem "an empty item") (if (== item "")))
    (+= cart item)
  )
)

(fn-do + add-all-or-none _ (?! BadItem)
  (param cart (list String))
  (param items (list String))
  (let start (. cart length))
  (let finished false)
  (finally
    (while (and (not finished) (> (. cart length) start))
      (call (. cart pop))
    )
  )
  (for (in item items)
    (throw (call BadItem "an empty item") (if (== item "")))
    (+= cart item)
  )
  (= finished true)
)
```

```output
an empty item
tea, milk
an empty item
tea
tea, milk, bread
```

### `use`

`(use value)`{.brix} uses a resource until the end of the block: it calls the value's
`begin-using`, gives what that returns, and calls `end-using` when the block ends, in order with
the block's cleanups.

- **Resources.** A type is a resource when it declares `begin-using` and `end-using`, or when it
  implements `Closable`, whose `close` is then called at the end.
- **Forms.** `(let file (use (call open path)))`{.brix} binds the resource.
  A `do-use` form uses a resource for the rows of its own block.
- **Null.** `use?` and `do-use?` skip a `null` resource.

## Panics

A panic is an error of the class `Panic` or a class derived from it. It reports a mistake that the
code where it happens cannot handle, such as an index out of range.
`(throw (call Panic message))`{.brix} raises one, and needs no declaration. The core library panics
for mistakes such as these:

| Mistake | Panic |
| --- | --- |
| An index out of range | `List index 5 out of range`, `Index out of bounds (7 / 3)` |
| `(!! value)`{.brix} on `null` | `Optional'core/number:I64 is null` |
| A failed conversion to a class | `Invalid cast to [Object __source__:Dog]` |

A panic ends the task it happens in. The cleanups of the blocks it leaves still run, newest first,
and nothing else in the task does.

```brix crash
(main
  (print "before")
  (call fail)
  (print "after")
)

(fn-do + fail _
  (finally
    (print "cleanup")
  )
  (throw (call Panic "broken invariant"))
)
```

```output
before
cleanup
```

- **Nothing stops it.** `catch`, `catch?` and handlers let a panic pass, and nothing between the
  panic and the start of its task can stop it.
- **In `main`.** A panic in the main task ends the program; see
  [Unhandled panics](#unhandled-panics).
- **In a cleanup.** A panic raised by a cleanup takes the place of the one that was leaving, and
  the cleanups of the blocks around it still run.
- **In a subscriber.** An event delivers each value in the task that publishes it, so a panic in
  one of its subscribers ends the publishing task.

### Panics, exceptions and `recover`

A panic looks like an exception or like Go's `panic`, but it ends differently. No code in the
failing task can stop it, and that task's stack is never resumed.

| | Yumi panic | Exceptions in Java or Python | Go's `panic` | Rust's `panic!` |
| --- | --- | --- | --- | --- |
| What stops it | Nothing; an `on-failure` arm where the task was started handles it after the task ends | An enclosing `catch` | `recover()` in a deferred function | An enclosing `catch_unwind` |
| What runs as it leaves | Cleanups | `finally` blocks | Deferred functions | Destructors |
| Where work goes on | In the task that started the failed one, which was never interrupted | After the `catch`, on the same stack | In the caller of the function that deferred `recover()`, on the same stack | After `catch_unwind` on the same stack, or in the thread that joins the failed one |
| When nothing stops it | The program ends, whichever task failed | The thread ends; in the main thread, the program | The program ends, whichever goroutine failed | The thread ends; in the main thread, the program |

- **Not an exception.** Exceptions use one mechanism for failures a caller expects, such as a
  missing file, and for mistakes, such as reading past the end of a list. A `catch` written for the
  first also stops the second, and the program goes on from whatever state the mistake left. Yumi
  keeps the two apart. Expected failures are recoverable errors, which a function declares and
  `catch` and handlers take. Mistakes are panics, which pass every `catch` and handler, so a
  handler can never hide a bug.
- **Not Go's `recover`.** In Go, any deferred function in the failing goroutine can call
  `recover()`. The panic then stops, and the function that deferred it returns normally to its
  caller, which goes on running on the stack where the mistake happened. In practice `defer` with
  `recover()` becomes try and catch. Yumi's cleanups run as Go's deferred functions do, and a cleanup can read the
  panic with `(. (call Task) failure)`{.brix}, but it cannot stop it.
- **Handled where the work began.** In Go, a goroutine cannot handle a panic in a goroutine it
  started; the started one must recover inside itself, or the program ends. In Yumi, the task that
  started the work handles its failure with `on-failure` arms. That is the place that knows what
  the failure means, such as a server that drops one connection and goes on serving the others.
- **The closest relatives** are Rust's threads, whose panic reaches the thread that joins them, and
  Erlang's processes, which fail whole and are restarted by a supervisor.
  Unlike Rust, Yumi has nothing like `catch_unwind`, and a panic that no arm handles ends the
  program even when it happens in a task other than the main one.

Ending the whole task is the point of the design. Code that just made a mistake is the code whose
assumptions no longer hold, so no row of it runs again. What survives is the code that started
it, which never saw the mistake happen.

### Handling a failed task

`on-failure` arms at the end of a `do-new` form handle a panic that ends the task, where the task
was started. An arm binds the panic and runs its rows in the failed task, after the task's
cleanups.

```brix
(main
  (let worker (call start 3))
  (call (. worker wait))
  (print (== (. worker end) .panicked))
  (call (. (call start -1) wait))
  (call (. (call start 2) wait))
  (print "main goes on")
)

(fn-do + start Task
  (param count Int)
  (let task
    (do-new
      (print (str "sum " (call add-up (list 1 2) count)))
      (on-failure (: failure OutOfBoundsError)
        (print (str "out of bounds: " failure))
      )
      (on-failure failure
        (print (str "failed: " failure))
      )
    )
  )
  (return task)
)

(fn-do + add-up Int
  (param items (list Int))
  (param count Int)
  (finally
    (print (str "cleanup after " count))
  )
  (throw (call Panic "negative count") (if (< count 0)))
  (let sum 0)
  (for (in index (..< 0 count))
    (+= sum ([] items index))
  )
  (return sum)
)
```

```output
cleanup after 3
out of bounds: List index 2 out of range
true
cleanup after -1
failed: negative count
cleanup after 2
sum 3
main goes on
```

- **Kinds.** An arm that names a class handles only that kind of panic, as in
  `(do-new (inline (call work)) (on-failure (: failure OutOfBoundsError) (print failure)))`{.brix}.
  One that names only its binding, as `failure` does in `start` above, handles any. The arms are
  tried in the order written. An arm takes only panics: `A task's handler takes a panic`.
- **No arm fits.** A panic that no arm takes is unhandled, as is one raised in an arm.
- **After a failure.** The failed task's own state is gone, but what it shared with other tasks is
  as the panic left it. Start work that may fail in a task of its own at a point that can decide
  what a failure means, such as one connection of a server.
- **Cancelling** a task runs its cleanups, but not its `on-failure` arms, since it is not a
  failure.
- **The handle.** A `do-new` form gives the task it starts. `(call (. worker wait))`{.brix} waits
  for it to end, and its `end` is `panicked` once a panic has ended it, handled or not. See
  Concurrency.
- **Recoverable errors** that leave a task's body are not handled by `on-failure`; see
  [How errors leave](#how-errors-leave).

### The failure in a cleanup

While a task's cleanups run, `(. (call Task) failure)`{.brix} gives the panic that is ending the
task. Otherwise it gives `null`.

```brix
(main
  (print (str "main: " (call ending)))
  (let worker
    (do-new
      (finally
        (print (str "cleanup: " (call ending)))
      )
      (throw (call Panic "ending"))
      (on-failure failure
        (print (str "handler: " (. failure message) ", now " (call ending)))
      )
    )
  )
  (call (. worker wait))
)

(fn-do + ending String
  (let failure (. (call Task) failure))
  (return (if-expr (!= failure null) (. failure message) "nothing"))
)
```

```output
main: nothing
cleanup: ending
handler: ending, now nothing
```

A cleanup can read the failure to log it or to undo work, but cannot stop it.

### Unhandled panics

A panic that no `on-failure` arm handles, in any task, ends the program. It prints `Unhandled
panic:` and its text, then the same report as an error, and crashes: the process ends with
`SIGABRT`, status 134.

- **Crash reports.** On macOS the crash leaves a report in `~/Library/Logs/DiagnosticReports`, and
  the message and the stack also go to the system log as a fault, where Console shows them for an
  app whose output goes nowhere.
- **Stack overflow.** A call too deep for its task's stack prints `Stack overflow` and the stack,
  with recursive calls folded into one line, and ends the program, even in a task with
  `on-failure` arms. On macOS it dies of `SIGBUS`, status 138.
- **Recoverable errors** that leave `main` or a task's body end the program with status 1 rather
  than a crash, since they are expected failures, such as a missing file.

## Choosing between them

| The situation | Use |
| --- | --- |
| A caller can deal with the failure, such as a missing file or bad input | A recoverable error, declared with `?!` |
| This caller can do nothing about it | Nothing: let it leave |
| The caller has a value to use in its place | An `on-error` handler on the call |
| The caller wants success and failure as one value to look at | `catch`, and a `match` |
| Any failure just means there is no value | `catch?` |
| A layer should report the failure in its own terms | A transformer |
| Something must be released or undone however the block ends | `finally` or `use` |
| A mistake in the program, which no caller can repair, such as a broken invariant | A panic |
| A failure that should end only part of the program | A panic, in a task with `on-failure` arms |

Choose by how callers should respond, not by how serious the failure sounds. A recoverable error
is part of a function's type, so every caller sees it and decides. A panic is not, so it suits a
failure that callers should never have to think about.
