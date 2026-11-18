# Documentation

- `index.md` is the index of what agents read to write Brix, which `brix` and `brix --help` name.
  Each release ships it with every file it reaches by links, so link only to portable material.
- `style/` contains the repository's agent-facing Brix style guides. Agents may update these only
  when explicitly asked. The ones `index.md` links to ship with releases, so they must not assume
  this repo. This repo's own rules go in `agent-brix-tests.md`, `agent-brix-verification.md` and
  `agent-brix-enforcement.md`.
- `ref/` is reserved for official reference documentation approved by the project owner. Do not
  create or populate it without that approval. Its examples are test cases: the `docs` suite
  compiles each `brix` block as a program, runs it, and compares what it prints with the `output`
  block that follows. Words after `brix` are the case's options, such as `compile-error` or
  `crash`, as on the program fences of a `.test.md` file. Code in its prose is written as Brix forms,
  tagged as `` `(let name value)`{.brix} ``, which the website shows as rows; inside a table, a
  form's `|` is written `\|`. Only text about the rows view itself is written as rows, tagged
  `{.rows}`. The suite's "Inline forms" case for each document checks that every tagged form
  prints as rows and reads back the same, through `brix-inline-view` in `/lang/brix/inline`.
- `agent/` contains unofficial agent notes and working documents. Agents may create material here
  freely, and it may be deleted at any time.

Completed-task reports, verification ledgers, benchmark logs, and workspace histories do not
belong in `docs/`.
