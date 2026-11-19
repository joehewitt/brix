# Brix Test File Format

The language's own tests, such as those in `stdlib/core/tests`, are `.test.md` files. A test file
is Markdown that follows a fixed convention. The runner reads its headings and fences, ignores all
other prose, and finds many independent cases in it.

## A Case

A case is a program fence and the fences of what it should print:

````markdown
### Printing a string

```brix
(main
  (print "hello")
)
```

```output
hello
```
````

A case has these parts, in this order and no other:

1. An optional `##` or `###` heading.
2. Optional prose.
3. The program: a `brix` fence, whose other words are its flags.
4. Any number of extra files, each a fence with `file=`.
5. An optional `stderr` fence, then an optional `output` fence.

Prose may sit between a case's fences. A case ends at the next program fence, at the next heading,
or at the end of the file. Any fence out of order, or an unknown word on a program, file or
expected fence, is an error that names its line.

## A File

A file opens with one `# Title`, then optional prose about the whole file, then its cases. `##`
headings group cases into sections, and `###` headings title them.

A case's title is the nearest `##` or `###` heading above it, and every case below a heading
carries it until the next one. Cases that share a heading are labeled by their position under it,
as in `Printing a string (2)`. A case with no heading above it but the `#` title is labeled by the
start of its program.

Fences in any other language, such as `text`, are illustrations. The runner ignores them, as it
ignores prose.

## The Program

The program is its fence's content, trimmed. The runner writes it to a file after one blank line,
so its first line is line 2 in the diagnostics it gives:

````markdown
```brix crash
(main
  (print (call area "a"))
)

(fn-expr + area I32
  (param width I32)
  (param height I32)
  (* width height)
)
```

```output
Exception: No function "area" accepts (String); it is declared as "area(width I32, height I32)"
<no file:3>
      (print (call area "a"))
             ^^^^^^^^^^^^^^^
```
````

Notes about a case go in prose above its fences. Nothing inside a fence is the runner's own
syntax, so a program may hold any line, and comments in it are the language's own.

## Flags

Flags follow the language on the program fence, separated by spaces:

- `crash`: the program, or the compiler with a diagnostic, may exit with a failure status, which
  otherwise fails the case. The runner still compares stdout with `output`, and stderr with
  `stderr` when the case has one.
- `compile-error`: the compiler is expected to reject the program. Its diagnostic is on stdout, so
  the `output` fence holds the `Exception:` line followed by its location and caret lines. Use it
  instead of `crash` when the case states a compile-time diagnostic.
- `warn`: compile with warnings. Without a `stderr` fence their text is not compared. With one,
  the fence holds the warnings followed by what the program wrote to stderr, and an empty
  `stderr` fence states that there are none. Warnings from the program and its extra files are
  included. The case's temporary folder is removed from them, so relative file names, line
  numbers, source excerpts and carets remain testable. The case runs alone, outside any batch.
- `probes` or `probes=a,b`: compile the probes these selectors name into the program, as
  `--probes` does, and record the run's trace in the case's folder, which is removed after the
  run. A bare `probes` compiles the probes `--probes` compiles with no selectors. The case runs
  alone, outside any batch. Probe records go to the trace, not stdout, so `output` is still what
  the program prints.

A flag with a value is written `name=value`, without spaces.

## Expected Output

- An `output` fence is compared with stdout.
- A `stderr` fence is compared with stderr: what the compiler and the program wrote there.
- Without an `output` fence, stdout must be empty, unless the case has a `stderr` fence; then stdout
  is not compared.

Both are trimmed before they are compared, so blank lines at their start and end do not matter,
but whitespace inside them and the order of their lines do.

A case whose program crashes with a panic states it on stderr:

````markdown
```brix crash
(main
  (do-new
    (throw (call Panic "task boom"))
  )
)
```

```stderr
Unhandled panic: task boom
<no file:4>

Task 2, spawned by task 1 from no file:3:3

Stack trace:
  0  __source__:Fork no file:4:5
```
````

## Extra Files

A fence with `file=path` writes an extra file before the program compiles, for imports or for
the program to read. Its content is written exactly, without trimming. Its first word only sets
the fence's highlighting, so any word works for a file that is not Brix:

````markdown
```brix
(import ./lib/foo)

(main
  (print (call foo))
)
```

```brix file=lib/foo/foo.bx
(fn-expr + foo Int
  24
)
```

```text file=data/input.txt
first line
```

```output
24
```
````

Paths are relative to the case's temporary folder, and the program's relative imports resolve
against it.

## The UBrix Suite

The `ubrix` suite reads the parse suite's files and ignores their expected output. It checks that
each case's Brix converts to UBrix, the text the editor shows, and back to the same text. A case
whose Brix the compiler rejects is skipped. A case passes only when its Brix is written as the
converter writes it.

## Fences

A fence opens with three backticks, or four when its content holds a line that starts with three.
It closes at a line of at least as many backticks and nothing else.

## Selecting And Running Cases

Cases are numbered from 1 in each file. `stdlib/core/tests/runArrays.test.md:340` selects the
case that holds line 340, and `test:core:stdlib/core/tests/runArrays.test.md:12` the twelfth case,
naming its suite.

To run a case's program by itself, copy its program into `case.bx`, write each extra file to its
path beside it, and run it with `brix case.bx -o case`. Its stdout should match the `output`
fence.

## Editing Tests

- Preserve whitespace inside expected output exactly.
- For diagnostic cases, use the failure text the runner reports.
- Prefer adding a case over changing an unrelated one.
- Put prose and headings between cases, never inside a fence.

## Common Mistakes

- Putting a `file=` fence after `stderr` or `output`, or `stderr` after `output`.
- Writing an `output` fence for a `warn` case's warnings. They go in `stderr`.
- Leaving out the `output` fence of a case that prints, which then fails because stdout is not
  empty.
- Closing a four-backtick fence with three.
- Assigning a script-level variable from inside a `(do-new …)` task. Task bodies capture a by-value
  snapshot of script locals (unlike ordinary closures, which share), so the write mutates the task's
  private copy — live/style actions re-evaluate against the unchanged main-task value and appear
  stale. Drive all script-variable updates from the main task, and use a `Semaphore` plus
  `(do-new …)` blocks only for checkpoint/observation, as in `runStyleAuthoring.test.md`. (`lazy`
  script variables first touched inside a task now materialize at the spawn site; see the "Capture
  scope" tests in `runTasks.test.md`.)
