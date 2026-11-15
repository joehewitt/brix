# Yumi Test File Format

The language's own tests, such as those in `stdlib/core/tests`, are `.test` files. A `.test` file
is a line-oriented file read by the test runner, not Yumi source, and it holds many independent
cases.

## Basic Test Case

A test case has three parts:

```text
:====

-- "hello"

:----

hello
;====
```

Meaning:

- `:====` starts a test.
- Lines after `:====` are the Yumi source under test.
- `:----` separates source from options/expected output.
- Lines after `:----` are expected output unless they are option lines.
- `;====` ends the test.

The runner compares:

```text
stdout.trim() == expected.trim()
```

So leading and trailing blank lines around the whole source and expected blocks do not matter, but internal whitespace and line order do matter.

## Separators

The parser accepts separators with at least three repeated marker characters:

- `:===` or `:====` starts a test.
- `:---` or `:----` separates sections.
- `;===` or `;====` ends a test.

Most existing tests use four characters. Prefer matching the local file style.

## Source Section

The source section is copied into the compiler as the test program.

```text
:====

. x = 42
-- x

:----

42
;====
```

In the source section only, lines beginning with `# ` are ignored by the test parser:

```text
:====
# This parser-level comment is not part of the Yumi source.
-- "ok"
:----
ok
;====
```

Use this sparingly. Most comments inside source should be valid Yumi comments if they are meant to exercise or document source behavior.

## Expected Section

The expected section is the output or diagnostic text to compare against.

Lines beginning with `?` are ignored by the test parser:

```text
:====

-- "hello"

:----
? The next line is the expected stdout.
hello
;====
```

Ignored `?` lines are useful for notes, unstable diagnostic lines, or documenting why an expectation looks unusual.

## Options

Options appear immediately after the first `:----`, before the expected output body.

```text
:====

futureFeature()

:----
%nyi

;====
```

Supported options:

- `%skip`: mark the test skipped.
- `%nyi`: mark the test skipped because behavior is not implemented yet.
- `%crash`: a compiler/runtime crash or nonzero exit is expected; the runner compares stdout to the expected body and keeps stderr as failure diagnostic context.
- `%compile-error`: the compiler is expected to reject the program; the runner compares stdout,
  which holds the diagnostic, to the expected body. Use it instead of `%crash` when the case states
  a compile-time diagnostic: the `Exception:` line followed by its location and caret lines.
- `%crash-stderr`: a compiler/runtime crash or nonzero exit is expected; the runner compares stderr to the expected body.
- `%warn`: run the compiler with warnings enabled without comparing warnings.
- `%warnings`: enable warnings and compare stderr followed by stdout, separated by a newline,
  against the expected body. Diagnostics from the case source and its `%file` modules are
  included. The case's temporary directory prefix is removed; relative file names, line numbers,
  source excerpts, and carets remain testable. With `%crash-stderr`, only stderr is compared, as
  usual.
- `%file: relative/path`: create an extra file before compiling/running this test.
- `%probes: selectors`: compile the probes these selectors name into the program, as `--probes`
  does, and record the run's trace in the case's directory, which is removed after the run. A bare
  `%probes` compiles the probes `--probes` compiles with no selectors. The case runs alone, outside
  any batch. Probe records go to the trace, not stdout, so the expected body is still what the
  program prints.

Options with values use this form:

```text
%name: value
```

Options without values use this form:

```text
%name
```

## `%file` Blocks

Use `%file` to create additional files for imports or runtime file IO.

```text
:====

./lib/foo
-- foo()

:----
%file: lib/foo.yumi

<Yumi declaration divider line>
+ foo

-- "from foo"

:----

from foo
;====
```

The `<Yumi declaration divider line>` placeholder represents the long divider used by existing Yumi source examples before declarations. It is Yumi source content, not test-runner syntax.

Rules:

- `%file: path` starts a file-content block.
- The file content continues until the next `:----`.
- After that `:----`, more options may follow, or the expected body begins.
- Relative paths are written under a per-test temporary directory.
- Relative imports resolve against that temporary directory for tests using `%file`.

A test may have multiple `%file` blocks:

```text
:----
%file: lib/foo.yumi
...
:----
%file: data/input.txt
...
:----
expected output
;====
```

## Focused Tests

A focused test starts with two colons:

```text
::====
```

Focus markers are easy to leave behind, so select a single case by its number instead when the
runner allows it.

## File-Level Comments And Ignored Remainders

At top level, a plain separator starts or ends a comment block:

```text
====
This text is ignored by the test parser.
====
```

At top level, this form ignores the remainder of the file:

```text
/:====
```

Use remainder ignore only for temporary local work. Do not leave it in committed tests unless intentionally disabling the rest of the file.

## Running One Case By Hand

To run a case's program by itself, copy its source section into a file, write each `%file` block
to its path beside that file, and run it with `yumi case.yumi -o case`. Its stdout should match the
expected section.

## Editing Existing Tests

Be careful not to confuse separator lines with Yumi syntax. The separator lines are test-runner syntax and are not part of the compiled source.

When updating expected output:

- Preserve internal whitespace exactly.
- Do not include parser note lines beginning with `?` if they should be compared.
- For diagnostic tests, use the actual failure text the runner reports.
- Prefer adding a new test block over mutating an unrelated existing test.

## Common Mistakes

- Forgetting the final `;====`.
- Putting expected output before all `%file` blocks are closed.
- Adding `%file` content but forgetting the second `:----` before expected output.
- Expecting lines beginning with `?` to be compared.
- Editing the long visual divider lines inside Yumi source as if they were test separators. Only lines like `:----` and `;====` are separators.
- Assigning a script-level variable from inside a `do new` task. Task bodies capture a by-value
  snapshot of script locals (unlike ordinary closures, which share), so the write mutates the
  task's private copy — live/style actions re-evaluate against the unchanged main-task value and
  appear stale. Drive all script-variable updates from the main task, and use a `Semaphore` plus
  `do new` blocks only for checkpoint/observation, as in runStyleAuthoring.test. (Lazy `..`
  script variables first touched inside a task now materialize at the spawn site; see the
  "Capture scope" tests in runTasks.test.)
