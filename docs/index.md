# Writing Yumi

This index lists what to read to write Yumi and Brix. `yumi --help` prints where it is. Its links
are relative to this file. The folder above it is the toolchain root, which holds `bin/yumi`, the
standard library and these docs.

## Before Writing Code

- [Correctness](style/agent-yumi-correctness.md): the rules that keep Yumi compiling, and what to
  do when syntax surprises you. Read it before writing any Yumi.
- [Style](style/agent-yumi-style.md): how Yumi is written, rule by rule.
  - For a GUI app, read [GUI style](style/agent-yumi-style-gui.md).
  - For markup, read [markup style](style/agent-yumi-style-markup.md).
  - For a terminal app, read [TUI style](style/agent-yumi-style-tui.md).
- [Brix](style/agent-brix.md): the S-expression form of Yumi, written in `.bx` files. It lists each
  form beside the Yumi it stands for.

## Learning The Language

- [The language reference](ref/README.md) gives each part of the language a chapter, from source
  text and literals through types, patterns, streams, errors, concurrency, markup, styles and
  native code. Read the chapter for the feature you are about to use.
- Its last pages are a symbol index and a form index, which point to the entry that explains each
  operator, mark, name and Brix form.

## The Core Tests

[`stdlib/core/tests`](../stdlib/core/tests) holds about 5,300 cases in 103 files. Each case is a
whole program with the output it prints, or the error the compiler gives it. They record what the
language accepts and what each construct does, so they answer "is this legal, and what does it
print?". They are kept short to test one thing each, so take their constructs, not their style.

- Each file covers one topic, such as `runLoops.test`, `runOptionals.test`, `runStreamMatch.test`
  or `runStateMachines.test`. To find cases that use a construct, search the files for it.
- [The test format](style/agent-yumi-test-format.md) explains a case's parts. Its source runs from
  `:====` to `:----`, and its expected output runs from there to `;====`. Lines such as
  `%compile-error` change what is expected.
- Some cases import helpers from [`stdlib/core/tests/support`](../stdlib/core/tests/support).
- To try a case, copy its source into a file and run it with `yumi case.yumi -o case`.

## Example Programs

[`examples`](../examples) holds whole programs, all written in Brix, which each release builds and
ships. Unlike the core tests, they show how complete programs are written.

- [`examples/basics`](../examples/basics): programs that print, named for what they do, such
  as `lisp.bx`, `vending.bx`, `json-parser.bx`, `adventure.bx`, `sqlite.bx` and `diff.bx`.
- [`examples/net`](../examples/net): an HTTP client and server, a WebSocket chat server for
  browsers with a client that joins it, and an MCP server. `chatgpt-mock-server.bx` stands in for
  the OpenAI API that `chatgpt.bx` talks to.
- [`examples/tui`](../examples/tui): terminal programs, from the small `order.bx`, `timer.bx` and
  `todo.bx` to `chromatic`, a pixel animation studio, `pulse`, a system dashboard, and
  `githistory`, a Git history browser with a commit graph, diffs and live updates, plus one program
  per feature, such as `table.bx`, `dialogs.bx` and `scrolling.bx`.
- [`examples/gui`](../examples/gui): macOS apps. `basics` holds small apps of one file each.
  `showcase/showcase-app.bx` gathers panels of components, effects and motion into one app, and
  opens the panel named after `--`, such as `menus` or `scrolling`.
  `showcase/motion/motion-app.bx` is a gallery of the animation system, with a speed control and a
  reduce motion switch. `quakes` is a live earthquake map that loads the USGS feed and pans and
  zooms under glass panels, and `pattern-designer` edits repeating patterns with undo, files and
  SVG export. `terminal` runs your shell in tabs and windows, with menus and key shortcuts.
- [`examples/games`](../examples/games): three macOS games, `resonance`, `prism-break` and
  `clatter-hill`, a marble run with sound.
- [`examples/graphics`](../examples/graphics): drawings written to image files, which need macOS.

`examples/tui/chromatic` also needs macOS, since it draws with the graphics library, and so does
`examples/tui/pulse` unless it runs with `--demo`.

To try one from your project, run `yumi <toolchain root>/examples/tui/timer.bx -o .tmp/build/timer`.

## The Standard Library

[`stdlib`](../stdlib) is the standard library's source. A module's import path is its path under
`stdlib` without `.yumi`, so `/os/path` is `stdlib/os/path.yumi`. Declarations marked `+` are
public. The library has few comments, so read a declaration's signature and body to learn what it
does.

[Text outlines](agent/graphics-text-paths.md) shows how to turn shaped text into editable paths.

| Library | What it holds |
| --- | --- |
| `core` | Built-in types, collections, streams, tasks, markup and style; the basics need no import |
| `os` | Files, paths, processes, the environment, command lines and app bundles |
| `data` | JSON, CSV, plists, protobuf, Markdown, S-expressions, diffs and SQLite databases |
| `stream` | Stream aggregation, filtering, sorting, combining and timing |
| `time` | Clocks, durations, dates, time zones, timers and deadlines |
| `net` | Sockets, HTTP clients and servers, WebSockets, server-sent events and DNS |
| `gui` | Apps, windows, controls and layout |
| `tui` | Terminal apps: layout, input, forms and dialogs |
| `ui` | Commands, key bindings, selections and undo for apps |
| `graphics` | Colors, curves, canvases, images and vector documents |
| `test` | Test cases for a project's own code |
| `probes` | Probe traces and their supervision |
| `math`, `units` | Math functions, easing, and distance units |
| `regex` | Regular expressions |
| `crypto` | Digests, ChaCha20-Poly1305, SRP and random bytes |
| `media` | Audio output, mixing, synthesis and WAV files |
| `git` | Reading and changing Git repositories |
| `script` | An embeddable expression language: parsing, evaluating, solving and rewriting |
| `code` | Code documents: text, syntax, folding and search |
| `ai` | Chat with Claude, Codex and OpenAI models, MCP, planning and image generation |
| `gc` | Cleanups the collector runs when it reclaims an object |
| `lang` | The compiler itself |

## Checking, Testing And Debugging

- `yumi x.yumi` checks a program, and `yumi x.yumi -o x` builds and runs it. `yumi --help` lists
  the rest.
- [Probes](style/agent-yumi-probes.md): writing probes, and using them with `yumi probes` to debug
  and profile. Use probes in place of prints.
- [Test modules](style/agent-yumi-test-modules.md): testing a project's own code with `/test`, and
  running the tests with `yumi test`.
- [Benchmarks](style/agent-yumi-benchmarks.md): writing benchmarks, and measuring and comparing
  them with `yumi bench`.
- `yumi lint --help`: linting. [The lint rules](style/agent-yumi-lint.md) explains each rule.
- [Reviewing](style/agent-yumi-review.md): how to review Yumi for style, when asked to.
- `yumi init`: writes the `AGENTS.md` section and `CLAUDE.md` import that send a project's agents
  to this index.
