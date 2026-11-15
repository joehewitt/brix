# Errors

An error is a value a function raises in place of its result. Yumi has two kinds:

- **Recoverable errors** are part of a function's type. A function declares the errors it can
  raise, and a caller handles them or lets them pass on to its own caller.
- **Panics** are for mistakes, such as an index out of range. They need no declaration, pass
  through every caller, and end the program unless a cleanup recovers them.

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
  `Panic cannot be returned as a recoverable error; use ?! Panic(...) to unwind, or convert it
  explicitly.`
- **Classes cover their derived classes.** Declaring `Error` itself allows every recoverable
  error.
- **Inferred errors.** A function that declares no errors, such as `scaled` above, can raise
  whatever its body raises, and its callers know which. Function values and getters always infer
  their errors.
- **Recursion.** A function that calls itself must declare its errors: `IOError can leave "walk",
  which calls itself, so its errors cannot be inferred; declare them on it, as in "(fn-do + walk
  Bool (?! IOError) …)", or catch them here`.
- **Overrides** may raise fewer errors than the method they override, not more. A function value
  converts to a function type only if the type declares every error the function can raise.

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
value, or `null` for any error.

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
- **The value** of a call with handlers is the join of its own value and the handlers' values.
- **Nothing to handle.** A handler on a call that cannot fail is an error: `"plain()" cannot fail,
  so the catch attached to it has nothing to handle; remove the catch, or attach it to a call
  that can fail`.

### Transforming errors

A `throw` with an `on-error` form and no body raises nothing where it is written. It replaces each
error of the named class that later leaves the function with a new error, and the new error's
`previous` is the original.

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
- **Rules.** The new error's expression may not call a function that raises, and two transformers
  that could take the same error are an error.

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
  `continue`, or when an error or a panic leaves. A cleanup in a loop body runs on every pass.
- **Order.** Cleanups and uses run newest first.
- **Names.** A cleanup can read only the locals declared before it.
- **Return values** are worked out before the cleanups run. A cleanup cannot return a value:
  `Illegal deferred statement`.
- **Errors in cleanups.** An error raised by a cleanup replaces an error that was already leaving.
- **One row.** `(finally (inline (print "done")))`{.brix} holds one step.

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

A panic is an error of the class `Panic` or a class derived from it. `(throw (call Panic message))`{.brix}
raises one, and needs no declaration. The core library panics for mistakes such as these:

| Mistake | Panic |
| --- | --- |
| An index out of range | `List index 5 out of range`, `Index out of bounds (7 / 3)` |
| `!` on `null` | `Optional'core/number:I64 is null` |
| A failed conversion to a class | `Invalid cast to [Object __source__:Dog]` |

A panic passes through `catch` and handlers. It runs every cleanup it passes, and ends the program
with a report unless a cleanup recovers it.

```brix
(main
  (call show-ratio 10 2)
  (call show-ratio 10 0)
  (print "still running")
)

(fn-do + show-ratio _
  (param top Int)
  (param bottom Int)
  (finally
    (let panic (call recover))
    (if panic
      (print (str "recovered: " panic))
    )
  )
  (print (call ratio top bottom))
)

(fn-expr + ratio Int
  (param top Int)
  (param bottom Int)
  (before
    (throw (call Panic "division by zero") (if (== bottom 0)))
  )
  (/ top bottom)
)
```

```output
5
recovered: division by zero
still running
```

- **`recover`.** `(call recover)`{.brix} in a cleanup, or in a function a cleanup calls, takes the
  panic that is leaving and gives it as a `Panic?`. With no panic leaving, it gives `null`.
- **After recovering.** A recovered panic stops at the block whose cleanup recovered it, and the
  code after that block goes on. A function whose own cleanup recovers gives the zero value of its
  result type, such as `0`, `""` or `null`.
- **Uncaught.** A panic that leaves `main` prints `Unhandled panic:` and its text, then the same
  report as an error, and the program exits with status 1.
