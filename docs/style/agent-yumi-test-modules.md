# Yumi Test Modules For Agents

A test module is a Yumi program in a `tests` folder that imports `/test` and checks what a library
or app does. Use these rules to write and review test modules. Cite violations by rule ID, such as
`YTEST-02`. These are style rules, not syntax requirements; justified exceptions are allowed when
the rule's tradeoff does not fit the test.

## YTEST-01: Use Declarative Tests For Independent Facts

Use the declarative form when each assertion stands alone, setup fits in small lazy dependencies,
and failure of one row should not prevent later rows from running. Each `expected, actual` row checks
equality, and a bare `Bool` checks truth.

Incorrect:

```text
━━━━━━━━━━━━━━━━━━━━
+ do tests TestRender

. cases = TestCase("Rect")
cases.run(rect-tests)
= cases
```

Correct:

```yumi
/test
/graphics/point
/graphics/primitives

tests()

━━━━━━━━━━━━━━━━━━━━
+ tests TestRender

════════════════════
test("Rect")

Point(25, 40), Rect(10, 20, 30, 40).center
60, Rect(10, 20, 30, 40).bottom
true, Rect(10, 20, 30, 40).contains((20, 30))
```

## YTEST-02: Isolate Imperative Cases Behind Runner Boundaries

Use imperative cases for mutation, loops, early exits, multi-step setup, or assertions interleaved
with state changes. Put each case in a separate `do` function that takes and returns a `TestCase`,
then call it through `run` or `then`. This boundary lets the runner recover from a panic and continue
with the next case.

Incorrect:

```text
━━━━━━━━━━━━━━━━━━━━
+ do tests TestRender

. path = Path()
path.move-to(0, 0)
t.expect("after move", 1, path.count)
= t
```

Correct:

```yumi
/test
/graphics/path
/graphics/point

tests()

━━━━━━━━━━━━━━━━━━━━
+ do tests TestRender

. cases = TestCase("Path Mutation")
cases.run(path-mutation-tests)
= cases

━━━━━━━━━━━━━━━━━━━━
+ do path-mutation-tests TestCase
    - t TestCase

. path = Path()
t.expect("starts empty", 0, path.count)

path.move-to(0, 0)
t.expect("after move", 1, path.count)

= t
```

## YTEST-03: Use The Assertion That Describes The Fact

Use `t.expect` for equality and `t.check` for Boolean facts. Pass the actual side as a lazy
expression when it is expensive or may return a recoverable error, so the assertion can attach its
label before evaluation. Successful assertions increment `passed`, mismatches increment `failed`,
and recovered errors or panics increment `caught`.

Incorrect:

```text
t.check("count", path.count == 2)
t.expect("first curve", expected-curve, path.get-curve(0))
```

Correct:

```text
t.expect("count", 2, path.count)
t.expect("first curve", expected-curve, => path.get-curve(0))
t.check("closed", path.is-closed)
```

## YTEST-04: Keep Cases Small And Named

Prefer several focused cases with clear labels over one large case that stops at its first panic.
Start with declarative rows for compact value conversions, parsers, pure functions, and geometry
facts. Switch to imperative cases when sequencing, mutation, or repeated setup makes the declarative
form artificial.

Incorrect:

```text
. cases = TestCase("All Path Behavior")
cases.run(all-path-tests)
```

Correct:

```text
. cases = TestCase("Path Mutation")
cases.run(path-mutation-tests)
cases = cases.then("Path Recovery", path-recovery-tests)
cases = cases.then("Path Cache Invalidation", path-cache-tests)
```

## YTEST-05: Lay Out A Module And Its Fixtures The Same Way

Every test module imports `/test`, calls `tests()` at module scope, and defines
`+ tests TestRender`. Put modules directly in a `tests` folder beside the code they test, and
nothing else there. Put shared helpers in `tests/support/`, and put a module's fixture files in
`tests/fixtures/<module name>/`, which a test run uses as its working folder.

Incorrect:

```text
stdlib/graphics/tests/support.yumi
stdlib/graphics/tests/text-fixtures/noto-sans/NotoSans-Regular.ttf
```

Correct:

```text
stdlib/graphics/tests/support/geometry.yumi
stdlib/gui/tests/fixtures/tight-paragraph/noto-sans/NotoSans-Regular.ttf
```

## Running Test Modules

`yumi test run` builds and runs a project's test modules in parallel and reports each one. The
project root is the nearest folder at or above the current one that holds `.git`, or else a
`.yumi` folder, or else the current folder, and `--root` names another. A project with no
`.yumi/tests.json` has one suite, `tests`, which holds every module in a `tests` folder under the
project root.

- `yumi test run` runs every suite, `yumi test run parser/tests/tokens.yumi` runs one module by its
  path from the project root, and `yumi test list --tests suite:tests` lists the modules of a suite.
- Each module builds into `.tmp/build/tests/` and runs in a folder of its own, which starts as a
  copy of its fixtures folder when it has one.
- A module passes when it builds, prints no `RED` line and exits 0. It fails when it prints a `RED`
  line, and the report shows what it printed. It crashes when it does not build, or exits nonzero
  without a `RED` line, and the report shows the compiler's errors or the module's output.

To split modules into suites, write `.yumi/tests.json` at the project root:

```json
{
    "suites": [
        { "id": "parser", "modules": "./parser" },
        { "id": "app", "modules": "./app", "timeout": 600, "env": { "APP_MODE": "test" } }
    ]
}
```

- `modules` names the folder whose `tests` folders hold the suite's modules.
- `env` sets environment variables for a suite's runs, and an `env` beside `suites` sets them for
  every suite.
- `timeout` is how many seconds a run may go without reporting a result before it is stopped, 300
  by default.
- `yumi`, beside `suites`, is the command, as an array, that runs a suite of modules. It is
  `["yumi"]` by default, the `yumi` on `PATH`.

A folder under the project root can hold a `.yumi/tests.json` of its own, so an app keeps its suites
beside its code. Its suites join the root's, and no two suites anywhere may share an `id`. Its `./`
paths start from its own folder. It takes `yumi` and `env` from the nearest config above it, adds
its own `env` after them, and replaces `yumi` if it names one. Test ids and paths stay relative to
the project root.

To run one module by hand, build and run it with `yumi tests/path.yumi -o .tmp/build/tests/path`.
When it has fixtures, build it with `--compile` and run the executable from `tests/fixtures/path/`.
A failed assertion prints its label with the expected and actual values. At the end, each case
prints a line such as `Path GREEN (5 passed)`, or `RED` with its passed, failed and caught counts,
and a module with a failed case exits nonzero.
