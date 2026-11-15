# Errors

Yumi separates recoverable errors from panics. A recoverable error is an alternative to a
function's successful result. It normally propagates through callers until some code chooses
to handle it. A panic ends the task it happens in, after running that task's cleanup blocks,
and is handled, if at all, where the task was started.

Both use `?!` to signal failure, but the type of the value determines which mechanism applies.
An ordinary subtype of `Error` uses the recoverable error path. `Panic` and its subtypes use
the panic path.

## At a glance

| Situation | Mechanism |
| --- | --- |
| A function cannot produce its normal answer | Return a recoverable error with `?!` |
| A caller cannot recover locally | Let the error propagate |
| A caller can supply a replacement value | Attach a `catch` handler |
| A caller needs to inspect success and failure | Hold the result with prefix `catch` |
| Failure should mean absence | Use `catch?` |
| An invariant is broken and the task cannot go on | Raise a `Panic` |
| A failed task should not end the program | Attach a `catch` to its `do new` |
| Work must happen when a scope exits | Register `do finally` cleanup |

## Declaring and producing errors

Define an error by deriving a class from `Error`. A function signature can name its successful
result followed by the recoverable errors it may return:

:::example-pair

- [Error type](#source-fallible-function--error): Define a recoverable failure with a message and any domain-specific fields.
- [Result contract](#source-fallible-function--contract): Return an integer on success or `OddNumber` on failure.
- [Conditional failure](#source-fallible-function--guard): Return the error when the condition is true; skip the division.
- [Hold the outcome](#source-fallible-function--hold): Allow the error to become a printable value. The calls print `5` and `Expected an even number`.

```yumi #fallible-function
-- half(10)
-- ⟦hold⟧catch half(3)⟦/hold⟧

━━━━━━━━━━━━━━━━━━━━
⟦error⟧+ OddNumber is Error⟦/error⟧

━━━━━━━━━━━━━━━━━━━━
⟦contract⟧+ half Int ?! OddNumber⟦/contract⟧
    - value Int

⟦guard⟧?! OddNumber("Expected an even number")
    if value % 2 != 0⟦/guard⟧

value / 2
```

:::

`Error` provides a message, an optional source location, and an optional `previous` error for
preserving a chain of causes. Its ordinary string conversion displays the message. Custom
error classes can add data that callers need to distinguish or explain a failure.

Multiple error types are written as repeated declarations. This signature fragment describes
a successful `String` or either of two errors:

```yumi
━━━━━━━━━━━━━━━━━━━━
+ read-config String ?! FileNotFound ?! PermissionDenied
    - path String
```

Only recoverable `Error` subtypes belong in these declarations. `?! Int` is invalid, and
`?! Panic` cannot be declared as a recoverable error channel.

## Automatic propagation

At an ordinary call site, use the successful result directly. If the call produces an error,
the containing computation exits through its error path instead:

```yumi #propagation
-- adjusted-count(true)
-- catch adjusted-count(false)

━━━━━━━━━━━━━━━━━━━━
+ CountUnavailable is Error

━━━━━━━━━━━━━━━━━━━━
+ do read-count Int ?! CountUnavailable
    - available Bool

if available
    = 41
⟦failure⟧?! CountUnavailable("Count unavailable")⟦/failure⟧

━━━━━━━━━━━━━━━━━━━━
⟦contract⟧+ adjusted-count Int ?! CountUnavailable⟦/contract⟧
    - available Bool

⟦call⟧read-count(available) + 1⟦/call⟧
```

- [Produce an error](#source-propagation--failure): The unavailable branch cannot supply the count.
- [Propagate automatically](#source-propagation--call): Add only on success; an error exits the computation without a call-site propagation marker.
- [Boundary contract](#source-propagation--contract): Declare the error that may escape. The calls print `42` and `Count unavailable`.

Propagation does not undo earlier work. If a function changes state before a later operation
fails, those changes remain unless the program explicitly compensates for them. Cleanup is
a separate responsibility, described below.

Yumi can infer recoverable errors in functions without explicit error declarations. When a
function declares its error channels, the compiler checks that escaping errors fit them.
An unhandled error outside that contract must be declared, handled, or transformed.

- **API boundaries:** explicit error declarations communicate what callers should handle.
- **Recursion:** an error channel may need a declaration before recursive calls establish the return convention.
- **Interfaces:** implementations must respect the interface's error contract.

## Holding a result with `catch`

Prefix `catch` turns off automatic propagation for the expression it holds. The result retains
both possibilities: a successful value or an error. This held result is often called a `Result`.
Match it to decide what to do:

:::example-pair

- [Retain both outcomes](#source-held-result--hold): Prefix `catch` disables automatic propagation for the held expression.
- [Success arm](#source-held-result--success): Bind the string directly; no `Ok` wrapper is needed.
- [Error arm](#source-held-result--error): Choose a policy using the typed error. Here the output is `Cannot greet: No name was supplied`.

```yumi #held-result
. result = ⟦hold⟧catch read-name(false)⟦/hold⟧
-- match result
    ⟦success⟧| name String⟦/success⟧
        "Hello, $name"
    ⟦error⟧| error NameUnavailable⟦/error⟧
        "Cannot greet: ${error.message}"

━━━━━━━━━━━━━━━━━━━━
+ NameUnavailable is Error

━━━━━━━━━━━━━━━━━━━━
+ do read-name String ?! NameUnavailable
    - available Bool

if available
    = "Yumi"
?! NameUnavailable("No name was supplied")
```

:::

An ordinary match must cover the result's success and error possibilities, either explicitly
or through a suitable fallback. Handling one error type does not cover other error types.
A broad `Error` pattern can cover recoverable error alternatives when they deliberately share
one policy.

Holding an error is not itself recovery. It keeps the decision in the current computation.
That decision might be to log it, use a fallback, retry, or explicitly propagate it with
`?! error`.

## Recovering with attached handlers

An attached `catch` handles selected errors from the expression directly above it. Its body
can supply the value to use instead of the failed expression's result:

```yumi #handler
-- display-name(false)

━━━━━━━━━━━━━━━━━━━━
+ NameUnavailable is Error

━━━━━━━━━━━━━━━━━━━━
+ do read-name String ?! NameUnavailable
    - available Bool

if available
    = "Yumi"
?! NameUnavailable("No name was supplied")

━━━━━━━━━━━━━━━━━━━━
+ display-name String
    - available Bool

⟦call⟧read-name(available)
    catch NameUnavailable
        "Guest"⟦/call⟧
```

- [Attached recovery](#source-handler--call): Keep a successful string or replace this specific failure with `"Guest"`.

- **Inspect the error:** write `catch error NameUnavailable` to give it a local name.
- **Distinguish failures:** attach additional handlers for additional types.
- **Order overlapping handlers:** put a specific subtype before a broad handler when their policies differ.

Errors not handled by an attached handler still propagate automatically. Handling
`FileNotFound`, for example, does not silently discard `PermissionDenied`.

This is different from prefix `catch`. Prefix `catch` holds the result and disables that
automatic propagation. If you combine prefix holding with attached handlers, the original
error alternatives must all be accounted for; unhandled alternatives are not implicitly
sent to the caller.

A handler can also use `?! ReplacementError("...")` to propagate a different error immediately.
The explicit `?!` matters: constructing an error object as an ordinary handler value does
not by itself mean “leave this function now.”

## Turning failure into absence with `catch?`

Use `catch?` when the error details are intentionally irrelevant. It keeps a successful value
and converts a recoverable error into absence:

```yumi #absence
-- ⟦success⟧catch? read-count(true) ?? 0⟦/success⟧
-- ⟦failure⟧catch? read-count(false) ?? 0⟦/failure⟧

━━━━━━━━━━━━━━━━━━━━
+ CountUnavailable is Error

━━━━━━━━━━━━━━━━━━━━
+ do read-count Int ?! CountUnavailable
    - available Bool

if available
    = 41
?! CountUnavailable("Count unavailable")
```

- [Keep success](#source-absence--success): A successful call prints `41`.
- [Discard failure details](#source-absence--failure): Convert the error into absence, then use `0`. A successful zero or `false` would still be present.

This is concise, but it discards the error's identity and explanation. Use an attached handler
or a held result when different failures require different treatment. A fallback should be a
deliberate policy, not merely a way to make an error disappear.

Neither `catch` nor `catch?` is the panic-recovery mechanism. They operate on recoverable
error results.

## Translating errors and retaining their cause

A function may want to expose a domain-specific error instead of the lower-level error it
encounters. An error transformer declares that mapping in the function body:

:::example-pair

- [Translate at the boundary](#source-translation--transformer): Map an escaping `IOError` to `LoadError`; this declaration does not fail unconditionally.
- [Retain the cause](#source-translation--cause): The original error stays available through `previous`. The output is `Could not load settings: Disk unavailable`.

```yumi #translation
. result = catch load()
-- match result
    | value Int
        "Loaded $value"
    | error LoadError
        "${error.message}: ⟦cause⟧${error.previous}⟦/cause⟧"

━━━━━━━━━━━━━━━━━━━━
+ LoadError is Error

━━━━━━━━━━━━━━━━━━━━
+ do read-value Int ?! IOError

?! IOError("Disk unavailable")

━━━━━━━━━━━━━━━━━━━━
+ do load Int ?! LoadError

⟦transformer⟧?! LoadError("Could not load settings")
    catch error IOError⟦/transformer⟧

= read-value()
```

:::

Transformation lets each layer describe a failure in its own terms without losing the cause.
The resulting chain can be inspected through `previous`. A transformer can also match a
cause chain; this fragment matches a `ConfigError` whose previous error is an `IOError`:

```text
?! LoadError("Could not load settings")
    catch config ConfigError <- io IOError
```

Transformer expressions must not call functions that return recoverable errors. The mapping
must not introduce another unresolved failure while translating the original one. Unlike
ordinary attached handlers, overlapping transformers for the same error chain are rejected
as ambiguous.

## Cleanup with `do finally`

Register cleanup with `do finally`. Its body runs when the enclosing scope exits, including
normal completion, a return, recoverable error propagation, or a panic ending the task:

:::example-pair

- [Register an exit action](#source-cleanup--register): Run cleanup on normal completion, return, error propagation, or a panic.
- [Successful exit](#source-cleanup--success): Cleanup runs before the caller receives `7`.
- [Error exit](#source-cleanup--failure): Cleanup also runs before the caller receives the error.

```yumi #cleanup
-- perform(true)
-- catch perform(false)

━━━━━━━━━━━━━━━━━━━━
+ WorkFailed is Error

━━━━━━━━━━━━━━━━━━━━
+ do perform Int ?! WorkFailed
    - succeeds Bool

⟦register⟧do finally
    -- "cleanup"⟦/register⟧

if succeeds
    ⟦success⟧= 7⟦/success⟧
⟦failure⟧?! WorkFailed("Work failed")⟦/failure⟧
```

:::

The output is:

```text
cleanup
7
cleanup
Work failed
```

Cleanup runs before the caller receives either outcome. Multiple cleanup blocks in the same
scope run in reverse registration order. A cleanup registered inside a nested block belongs
to that block's scope, rather than necessarily waiting for the entire function to finish.

Place cleanup registration after the resource it protects has been acquired. `do finally`
provides an exit action; it does not automatically roll back a transaction or restore state
changed earlier in the function.

## Panics

Use `?! Panic("...")` for a failure the task should not survive. Runtime failures such as
out-of-bounds access also raise panic subtypes. Although `Panic` inherits from `Error`, it is
not a recoverable error alternative in a function's result contract.

A panic ends the task it happens in. Its `do finally` blocks and `use` cleanups still run,
innermost first, and nothing between the failure and the start of the task can stop it. A
`catch` written after a `do new` block handles the failure where the task was started, after
the task's cleanups have run:

:::example-pair

- [Start the work](#source-panic--start): The failure stays inside this task.
- [Handle it where it started](#source-panic--handle): Runs after the task's cleanups, with the panic itself.
- [Cleanup still runs](#source-panic--cleanup): `Cleanup ran` is printed first.
- [Fail](#source-panic--raise): Ends the task; ordinary `catch` is not its handler.
- [No resumption](#source-panic--skip): Nothing after the failure in the task runs.

```yumi #panic
. worker = ⟦start⟧do new⟦/start⟧
    run()
⟦handle⟧catch failure⟦/handle⟧
    -- "Task failed: $failure"
worker.wait()
-- "Caller continued"

━━━━━━━━━━━━━━━━━━━━
+ do run

⟦cleanup⟧do finally
    -- "Cleanup ran"⟦/cleanup⟧

⟦raise⟧?! Panic("Broken invariant")⟦/raise⟧
⟦skip⟧-- "This line is not reached"⟦/skip⟧
```

:::

The output is:

```text
Cleanup ran
Task failed: Broken invariant
Caller continued
```

Start the work that may fail in its own task at a boundary that can decide what a failure
means, such as one connection of a server or one case of a test runner. The task's state is
gone when the `catch` runs, so state it shared with others is as the failure left it. A narrower
type, as in `catch failure OutOfBoundsError`, handles only that kind of failure. A cleanup can
read the panic ending its task with `Task().failure`.

A panic that no `catch` handles, including one in the main task, prints its message and stack
trace and then crashes the process, which exits with status 134. On macOS the crash leaves a
report in `~/Library/Logs/DiagnosticReports`, and the message and trace also go to the unified
log as a fault, where Console shows them for an app whose output goes nowhere. A stack overflow
crashes the process the same way, and its report shows the overflowing stack. An unhandled
recoverable error reaching the top-level execution boundary is printed the same way, but ends
the process with status 1 rather than a crash, since it is an expected failure such as a missing
file. Automatic propagation is not automatic success and does not silently discard an unhandled
error.

## Choosing an error policy

Use a recoverable error when callers should be able to make an ordinary decision about a
failure. Use a panic for a failure the task cannot continue past, such as a violated
invariant. Choose based on how callers should respond, rather than simply how serious the
message sounds.

At each boundary:

1. Choose whether to propagate, replace the value, inspect the result, or translate the error.
2. Keep error types specific enough to support that decision.
3. Use `catch?` only when losing the explanation is acceptable.
4. Arrange cleanup independently of the chosen error policy.
