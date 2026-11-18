# Brix Tests For Agents

Read this before creating, editing, or running tests.

## What Tests Are For

A test is worth keeping only when it is a spec: a statement of how the system is meant to behave
that the source code does not already make.

- `stdlib/lang/tests/parse` and `stdlib/core/tests` are the spec of the language. They are
  permanent.
- Library and app source is its own record. Read it to learn what a library does. A test that
  restates what the code plainly does is not a spec, and a suite that passes is not evidence that
  a change you have not read is correct.
- Library and app tests have mostly caught compiler bugs by accident. A compiler bug belongs in
  `stdlib/core/tests` as a small program, not in a library test that happens to trip over it.

## Writing Tests

### Permanent Tests

Add a permanent test only in these cases:

1. Accepted syntax, rejected syntax, or parser AST shape: a case in `stdlib/lang/tests/parse`.
2. Language semantics, compiler behavior visible from Brix, core runtime behavior, or `stdlib/core`:
   a case in `stdlib/core/tests`.
3. The wording of a compiler diagnostic: a case in `stdlib/lang/tests/diagnostics`.
4. The user asks for a test. Put it where they say. For a library test without a stated location,
   write a native module under `stdlib/<area>/tests` and follow
   `docs/style/agent-brix-test-modules.md`.

Do not add any of these unless the user asks:

- a case in an existing library test module;
- a test for code under `examples/`;
- a Python or JavaScript test of compiler internals such as IR, MIR, inlining decisions, register
  allocation, or object layout;
- a test asserting that an optimization happened, or that something is fast. Performance claims
  are benchmarks;
- a stress test.

A compiler change does not justify a script test. If Brix source plus expected output or
diagnostics can show the behavior, it is a `parse` or `core` case. Python tests exist only for what
Brix cannot observe, such as the benchmark runner and the C runtime under sanitizers. Each is a `unittest` file in the `tests` folder of the code it checks,
listed in a suite of `.brix/tests.json`. Add one when the user asks, or to fix a repeatable
linter false positive: reduce it to a focused linter test and fix the rule.

The `core` import boundary is strict: a `core` case may import only modules under `/core`. Use local
declarations or `file=` fixtures instead of pulling in a non-core module as a helper.

### Scratch Cases

To check your own change, write a scratch case. Scratch cases live in `.tmp/scratch/*.test.md`, use
the format in `docs/style/agent-brix-test-format.md`, may import any module, and are git-ignored.

- Write the smallest program that exercises the behavior you changed, and print the values whose
  expected output shows the change works. One or two cases is normal.
- Name the file for the task, such as `.tmp/scratch/path-offset.test.md`, so parallel work does not
  collide.
- Run it with `file:.tmp/scratch/path-offset.test.md`.
- Delete the file when it passes. Do not move it into a library test module.
- A case's prints are what it checks, not a way to look inside. To see what a program does, or why
  a case fails, put the program in `.tmp/scratch/<task>.yumi`, which is git-ignored and is not a
  test, add probes where the behavior is in doubt, and run it with
  `bin/brix .tmp/scratch/<task>.yumi --probes`. `docs/style/agent-brix-probes.md` says how to
  write and query them. Delete the program with the case.

A scratch case costs what its imports cost to compile. Import the narrowest module that reaches the
changed code, not the whole library or an app.

### Promoting A Scratch Case

If a scratch case exposes a compiler or core-library bug, reduce it to the smallest program that
shows the bug using only `/core` imports, add that to the matching `stdlib/core/tests` file, and fix
the bug there. This is the only way a scratch case becomes permanent. A library bug is fixed in the
library and its scratch case is deleted.

To find where the compiler goes wrong, probe it rather than reading or guessing: write
keyed probes in `stdlib/lang` where it decides, and run `bin/brix --compiler-probes <key>` on the
reduced program.

## Running Tests

Run tests with `bin/brix test run <selectors>`. A failure prints its expected output, actual output
and errors in the run, trimmed to 60 lines. `bin/brix test list <selectors>` shows what a selection
holds without running it.

### Selectors

| Selector | Runs |
| --- | --- |
| `file:stdlib/core/tests/runArrays.test.md` | Every case in one file |
| `test:core:stdlib/core/tests/runArrays.test.md:12` | The twelfth case in the file |
| `stdlib/core/tests/runArrays.test.md:340` | The case containing line 340 |
| `file:stdlib/graphics/tests/path.yumi` | One native test module |
| `suite:core` | A whole suite |

A path followed by a bare number is a line number, not a case number. A native module is one test
however many cases it holds. The `stdlib` suites run every `.yumi` file directly inside a `tests`
folder, from a copy of its `tests/fixtures/<module name>/` folder. To run one case of a `.test.md`
file, select it by its number or a line in it.

### Scope

- Run the file or single case for what you changed, and nothing broader.
- After a compiler or runtime change, run `suite:core` once before handoff. After a grammar or
  parser change, also run `suite:parse` once.
- Never run any other `suite:` selector, except `suite:scratch`, unless the user names it. This
  includes `suite:stdlib`, `suite:parity`, and `suite:other`, which holds the Python tests and the
  slow GC sanitizer runs.
- When you change a library that has a test module, you may run that one module's file selector.
- `suite:parity` holds opt-in checks that two paths agree, kept beside their code in `parity/`
  folders, such as GUI render parity. They are not specs. Run one only when the user asks.
- After a failure, rerun only the failing selector.
- If test runs for one task pass about two minutes in total, not counting the handoff run, stop
  and tell the user what you have run and what you still want to run.
- Handing off without running a test is acceptable for a local change you have read. Do not run a
  suite to feel safe.

### When A Test Fails

- A `parse` or `core` failure is a spec violation. Fix the code, or tell the user that the change
  alters the spec and update the case only with their agreement.
- A library or app test that fails because you deliberately changed the behavior it describes is
  stale. Update or delete the case and say so at handoff.
- A library or app test that fails for a reason unrelated to your change is an orthogonal failure.
  Reduce it to a scratch case, then follow the promotion rule above.
- When the cause is not plain from the code, look inside with probes, as Scratch Cases describes,
  instead of adding prints or rerunning with guesses.

### Reporting

At handoff, list the exact selectors you ran and their results, and say plainly what you did not
run.

## Format

- For `.test.md` cases, fixtures, and expected output, read `docs/style/agent-brix-test-format.md`.
- For native library test modules, read `docs/style/agent-brix-test-modules.md`.
