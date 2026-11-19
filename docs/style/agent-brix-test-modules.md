# Brix Test Modules For Agents

A test module is a Brix program in a `tests` folder that imports `/test` and checks what a library
or app does. Use these rules to write and review test modules. Cite violations by rule ID, such as
`YTEST-02`. These are style rules, not syntax requirements; justified exceptions are allowed when
the rule's tradeoff does not fit the test.

## YTEST-01: Use Declarative Tests For Independent Facts

Use the declarative form when each assertion stands alone, setup fits in small lazy dependencies,
and failure of one row should not prevent later rows from running. Each `(tuple expected actual)`
row checks equality, and a bare `Bool` checks truth.

Incorrect:

```brix
(fn-do + tests TestRender
  (let cases (call TestCase "Rect"))
  (call (. cases run) rect-tests)
  (return cases)
)
```

Correct:

```brix
(import /test)
(import /graphics/point)
(import /graphics/primitives)

(main
  (call tests)
)

(fn-expr + tests TestRender
  (header (call test "Rect"))
  (tuple (call Point 25 40) (. (call Rect 10 20 30 40) center))
  (tuple 60 (. (call Rect 10 20 30 40) bottom))
  (tuple true (call (. (call Rect 10 20 30 40) contains) (tuple 20 30)))
)
```

## YTEST-02: Isolate Imperative Cases Behind Runner Boundaries

Use imperative cases for mutation, loops, early exits, multi-step setup, or assertions interleaved
with state changes. Put each case in a separate `fn-do` function that takes and returns a
`TestCase`, then call it through `run` or `then`. The runner runs each case in a task of its own, so
a panic ends only that case, and the runner continues with the next.

Incorrect:

```brix
(fn-do + tests TestRender
  (let path (call Path))
  (call (. path move-to) 0 0)
  (call (. t expect) "after move" 1 (. path count))
  (return t)
)
```

Correct:

```brix
(import /test)
(import /graphics/path)
(import /graphics/point)

(main
  (call tests)
)

(fn-do + tests TestRender
  (let cases (call TestCase "Path Mutation"))
  (call (. cases run) path-mutation-tests)
  (return cases)
)

(fn-do + path-mutation-tests TestCase
  (param t TestCase)
  (let path (call Path))
  (call (. t expect) "starts empty" 0 (. path count))
  (call (. path move-to) 0 0)
  (call (. t expect) "after move" 1 (. path count))
  (return t)
)
```

## YTEST-03: Use The Assertion That Describes The Fact

Use `t.expect` for equality and `t.check` for Boolean facts. Pass the actual side as a lazy
expression when it is expensive or may return a recoverable error, so the assertion can attach its
label before evaluation. Successful assertions increment `passed`, mismatches increment `failed`,
and errors a case lets out, or panics that end its task, increment `caught`.

Incorrect:

```brix
(call (. t check) "count" (== (. path count) 2))
(call (. t expect) "first curve" expected-curve (call (. path get-curve) 0))
```

Correct:

```brix
(call (. t expect) "count" 2 (. path count))
(call (. t expect) "first curve" expected-curve (=> () (call (. path get-curve) 0)))
(call (. t check) "closed" (. path is-closed))
```

## YTEST-04: Keep Cases Small And Named

Prefer several focused cases with clear labels over one large case that stops at its first panic.
Start with declarative rows for compact value conversions, parsers, pure functions, and geometry
facts. Switch to imperative cases when sequencing, mutation, or repeated setup makes the declarative
form artificial.

Incorrect:

```brix
(let cases (call TestCase "All Path Behavior"))
(call (. cases run) all-path-tests)
```

Correct:

```brix
(let cases (call TestCase "Path Mutation"))
(call (. cases run) path-mutation-tests)
(= cases (call (. cases then) "Path Recovery" path-recovery-tests))
(= cases (call (. cases then) "Path Cache Invalidation" path-cache-tests))
```

## YTEST-05: Lay Out A Module And Its Fixtures The Same Way

Every test module imports `/test`, calls `tests` from its `main`, and defines a public `tests`
function that returns a `TestRender`. Put modules directly in a `tests` folder beside the code they
test, and nothing else there. Put shared helpers in `tests/support/`, and put a module's fixture
files in `tests/fixtures/<module name>/`, which a test run uses as its working folder.

Incorrect:

```text
stdlib/graphics/tests/support.bx
stdlib/graphics/tests/text-fixtures/noto-sans/NotoSans-Regular.ttf
```

Correct:

```text
stdlib/graphics/tests/support/geometry.bx
stdlib/gui/tests/fixtures/tight-paragraph/noto-sans/NotoSans-Regular.ttf
```

## Running Test Modules

`brix test run` builds and runs a project's test modules in parallel and reports each one. The
project root is the nearest folder at or above the current one that holds `.git`, or else a
`.brix` folder, or else the current folder, and `--root` names another. A project with no
`.brix/tests.json` has one suite, `tests`, which holds every module in a `tests` folder under the
project root.

- `brix test run` runs every suite, `brix test run parser/tests/tokens.bx` runs one module by its
  path from the project root, and `brix test list --tests suite:tests` lists the modules of a suite.
- Each module builds into `.tmp/build/tests/` and runs in a folder of its own, which starts as a
  copy of its fixtures folder when it has one.
- A module passes when it builds, prints no `RED` line and exits 0. It fails when it prints a `RED`
  line, and the report shows what it printed. It crashes when it does not build, or exits nonzero
  without a `RED` line, and the report shows the compiler's errors or the module's output.

To split modules into suites, write `.brix/tests.json` at the project root:

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
- `brix`, beside `suites`, is the command, as an array, that runs a suite of modules. It is
  `["brix"]` by default, the `brix` on `PATH`.

A folder under the project root can hold a `.brix/tests.json` of its own, so an app keeps its suites
beside its code. Its suites join the root's, and no two suites anywhere may share an `id`. Its `./`
paths start from its own folder. It takes `brix` and `env` from the nearest config above it, adds
its own `env` after them, and replaces `brix` if it names one. Test ids and paths stay relative to
the project root.

To run one module by hand, build and run it with `brix tests/path.bx -o .tmp/build/tests/path`.
When it has fixtures, build it with `--compile` and run the executable from `tests/fixtures/path/`.
A failed assertion prints its label with the expected and actual values, and one whose actual side
raises prints `caught:` with the error's message and, where the error has one, its source. At the
end, each case prints a line such as `Path GREEN (5 passed)`, or `RED` with its passed, failed and
caught counts, and a module with a failed case exits nonzero.
