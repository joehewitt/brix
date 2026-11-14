# Documentation

- `index.md` is the index of what agents read to write Yumi, which `yumi` and `yumi --help` name.
  Each release ships it with every file it reaches by links, so link only to portable material.
- `style/` contains the repository's agent-facing Yumi style guides. Agents may update these only
  when explicitly asked. The ones `index.md` links to ship with releases, so they must not assume
  this repo. This repo's own rules go in `agent-yumi-tests.md`, `agent-yumi-verification.md` and
  `agent-yumi-enforcement.md`.
- `ref/` is reserved for official reference documentation approved by the project owner. Do not
  create or populate it without that approval.
- `agent/` contains unofficial agent notes and working documents. Agents may create material here
  freely, and it may be deleted at any time.

Completed-task reports, verification ledgers, benchmark logs, and workspace histories do not
belong in `docs/`.
