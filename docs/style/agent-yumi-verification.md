# Yumi Verification For Agents

Read this before running lint, benchmarks, probes, or interactive checks in this repo. For tests,
read `docs/style/agent-yumi-tests.md` instead; it decides what to write and what to run.

## Commands

- Type checks: `bin/yumi <file>`.
- Builds: `bin/yumi <file> --compile -o <path>` builds a source into an executable without running
  it. `--target` and `--probes` pick the platform and the probes compiled in.
- Tests: `bin/yumi test run <selectors>`. Which selectors to run is decided by
  `docs/style/agent-yumi-tests.md`.
- Lint: `bin/yumi lint <files>`, with the config in `.yumi/lint.json`. `--list-rules` lists the
  rules, `--rule <id>` runs only the rules named, and `--semantic-rules` adds the rules that type the
  program; add those only when requested or necessary.
- Benchmarks: `bin/yumi bench list` shows the profiles and suites,
  `bin/yumi bench run <selectors> --profile <profile>` runs them, and `bin/yumi bench runs` lists
  the saved runs that `--compare` accepts.

## Scope And Method

Report exact selectors, targets, rules, and profiles, and say what you did not check.

Lint Yumi only when the user asks. Follow `docs/style/agent-yumi-lint.md`, or
`docs/style/agent-yumi-review.md` for a full style review.

For performance work, compare the same profile and selectors against a saved pre-change baseline.
Reject gains that change correctness, visual semantics, lifecycle behavior, or the acceptance
target, or add complexity disproportionate to the measured win. Use probes when totals are
insufficient.

The user checks app behavior in the app and reports what is wrong.

## Probes

`docs/style/agent-yumi-probes.md` says how to write probes, run programs with them, and query and
compare traces. In this repo:

- Build a probed app for the user into a folder of its own, so a rebuild cannot replace it, and
  check that it has probes before asking them to run it:

  ```text
  bin/yumi examples/games/resonance/resonance-app.bx -o .tmp/build/resonance-probes/resonance-app --compile --probes 'key.one key.two'
  bin/yumi probes run --quiet --trace .tmp/build/resonance-probes/resonance-app.trace -- .tmp/build/resonance-probes/Resonance.app/Contents/MacOS/Resonance
  ```

- Tests stay the spec: a trace explains one run.

### Inside The Compiler

Write probes in `stdlib/lang` as in any program, keyed under the part of the compiler they explain,
such as `//dispatch.rank// candidate $decl ranks $rank`. Then run
`bin/yumi --compiler-probes dispatch x.yumi`. It builds a copy of the compiler with those probes at
`.tmp/build/tools/yumi-probed`, which is reused until the compiler's sources or the selectors
change. It then runs that compiler on the rest of the command line, as `yumi probes run` would, and
records its trace as `last`. Add `-o` to build and run `x` too. The selectors work as they do for
`--probes`. Without selectors, every probe in `compiler/yumi-cli.yumi` and every probe with no
key compiles in. Keyed probes in the compiler compile out of `bin/yumi` itself, so leaving them in
costs nothing.

## Batch Diagnostics

Keep diagnostic flags inside the shared batch. Split compilation only for unexpected compiler
failures. Runner coverage must assert batch phases and entry counts, not merely passing results
under an execution mode named `batch`.

## Computer Control

Controlling the user's mouse, keyboard, windows, or applications through Computer Use, shell, or
system APIs requires an explicit request for that exact action in the current turn. Tool or reviewer
approval does not count. This protects the user's live desktop from disruption; it does not apply
to isolated subprocesses such as PTYs. Otherwise ask the user to perform the desktop action.

## TUI Debugging

Check a TUI change as `docs/style/agent-yumi-style-tui.md` says, by driving the built executable in
a real PTY.
