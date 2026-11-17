## Writing Brix

`docs/index.md` lists the guides for writing Brix, which releases also ship to agents outside this
repo. Before writing code in the Brix language, read `docs/style/agent-brix-correctness.md`.

Before writing or editing a `.bx` file, also read `docs/style/agent-brix.md`.

Before adding or changing a Brix diagnostic or rule, read `docs/style/agent-brix-enforcement.md`.

When reviewing Brix code for style, follow `docs/style/agent-brix-review.md`. Do this review only when explicitly asked.

Do not lint brix files automatically as part of verification. Use the linter only when asked to do so explicitly.

Brix is not production yet; treat all authoring as compiler/language work. Expect rough edges; fix compiler/language bugs instead of employing workarounds.

Preserve architecture boundaries: generic compiler code must not hardcode stdlib classes outside of core unless explicitly told to.

## Tests

Tests here are a spec, not a safety net. Source code is the record of how libraries and apps behave.
`stdlib/core/tests` and `stdlib/lang/tests/parse` are the record of how the language behaves.

Before creating, editing, or running tests, read `docs/style/agent-brix-tests.md`.

Writing:

- Add a permanent test only as a `stdlib/core/tests` or `stdlib/lang/tests/parse` case, or when the
  user asks for one.
- To check your own change, write a scratch case in `.tmp/scratch` that exercises only the changed
  behavior. Scratch cases are git-ignored. Delete them when they pass.
- A scratch case's prints are what it checks. To see what a program does or why a case fails, run a
  scratch program with probes instead of adding prints.
- If a scratch case exposes a compiler or core-library bug, reduce it to the smallest program that
  shows the bug and add that to `stdlib/core/tests`. Nothing else is promoted.
- Do not add cases to an existing library test module unless the user asks.
- Do not add tests for code under `examples/`, Python or JavaScript tests of compiler internals,
  or tests asserting that an optimization happened. Performance claims are benchmarks.

Running:

- Run the file or single case for what you changed, and nothing broader.
- After a compiler or runtime change, run `suite:core` once before handoff. After a grammar or
  parser change, also run `suite:parse` once.
- After a change to the Yumi grammar, the Brix reader, printer, converter or editor grammar, or the
  form catalog, run `bin/brix brix-sweep` once. It converts every `.yumi` file to Brix, checks that
  each reads back the same, and builds the compiler from its Brix.
- Never run any other `suite:` selector, except `suite:scratch`, unless the user names it.
- After a failure, rerun only the failing selector.
- If test runs for one task pass about two minutes in total, not counting the handoff run, stop
  and tell the user.
- Handing off without running a test is acceptable for a local change you have read. Say what you
  ran and what you did not.

## Verification

Before running lint, benchmarks, or probes, or controlling the user's computer, read
`docs/style/agent-brix-verification.md`.

To see what a Brix program does, write a probe such as `// state $state` instead of a `--` print,
and run it with `--probes`; `brix probes` then filters and aggregates what it recorded.
`docs/style/agent-brix-probes.md` says how. Before handoff, remove the probes you added only to
look.

## Performance Work

For performance-related tasks, use `bin/brix bench`. Save a baseline before edits by running the
selectors under a profile with a name
(`bin/brix bench run <selectors> --profile <profile> --save <name>`), then rerun the same profile
and selectors with `--compare <name>` after edits. `bin/brix bench list` shows the profiles and
suites.


