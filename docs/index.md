# Writing Brix

This index lists what to read to write Brix. `brix --help` prints where it is. Its links
are relative to this file. The folder above it is the toolchain root, which holds `bin/brix`, the
standard library and these docs.

## Before Writing Code

- [Correctness](style/agent-brix-correctness.md): the rules that keep Brix compiling, and what to
  do when syntax surprises you. Read it before writing any Brix.
- [Style](style/agent-brix-style.md): how Brix is written, rule by rule.
  - For a GUI app, read [GUI style](style/agent-brix-style-gui.md).
  - For markup, read [markup style](style/agent-brix-style-markup.md).
  - For a terminal app, read [TUI style](style/agent-brix-style-tui.md).
- [Brix](style/agent-brix.md): Brix's S-expression form, written in `.bx` files. It lists each
  form with its signature.

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

- Each file covers one topic, such as `runLoops.test.md`, `runOptionals.test.md`,
  `runStreamMatch.test.md` or `runStateMachines.test.md`. To find cases that use a construct,
  search the files for it.
- [The test format](style/agent-brix-test-format.md) explains a case's parts. Each is Markdown: its
  program is a `brix` fence, and the `output` or `stderr` fences after it hold what it
  prints. Words on the program fence, such as `compile-error`, change what is expected.
- Some cases import helpers from [`stdlib/core/tests/support`](../stdlib/core/tests/support).
- To try a case, copy its program into a file and run it with `brix case.bx -o case`.

## Example Programs

[`examples`](../examples) holds whole programs, all written in Brix, which each release builds and
ships. Unlike the core tests, they show how complete programs are written.

- [`examples/basics`](../examples/basics): programs that print, in a folder for each part of the
  language they show, such as `patterns`, `grammars`, `pipelines` and `state-machines`, and named
  for what they show and what they do, as `patterns/tuples-fizzbuzz.bx` is. `classics` holds
  familiar programs, such as `lisp-interpreter.bx`, `game-of-life.bx` and `text-adventure.bx`.
- [`examples/net`](../examples/net): an HTTP client and server, a WebSocket chat server for
  browsers with a client that joins it, and an MCP server.
- [`examples/tui`](../examples/tui): terminal programs. `basics` holds small programs of one file
  each, such as `order.bx`, `timer.bx` and `todo.bx`, and one program per feature, such as
  `table.bx`, `dialogs.bx` and `scrolling.bx`. `chromatic` is a pixel animation studio, `pulse` a
  system dashboard, and `githistory` a Git history browser with a commit graph, diffs and live
  updates. `chat` talks to OpenAI, or, with no key, to the stand-in for its API in
  `chat/chat-mock.bx`, served inside the program under a fake network.
- [`examples/gui`](../examples/gui): macOS apps. `basics` holds small apps of one file each.
  `showcase/showcase-app.bx` gathers panels of components, effects and motion into one app, and
  opens the panel named after `--`, such as `menus` or `scrolling`.
  `showcase/motion/motion-app.bx` is a gallery of the animation system, with a speed control and a
  reduce motion switch. `quakes` is a live earthquake map that loads the USGS feed and pans and
  zooms under glass panels, and `pattern-designer` edits repeating patterns with undo, files and
  SVG export. `terminal` runs your shell in tabs and windows, with menus and key shortcuts.
  `control-field` zooms from one animated control card out to half a million on an endless canvas,
  simulating every card on worker threads and drawing them as quad batches. `page-board` pans and
  zooms a board of up to 4,000 mock pages of text, icons and charts drawn as vector paths and
  glyphs, with a panel showing what each frame costs. `herbarium` animates up to 4,000 illustrated
  botanical specimens—ferns, blossoms, vines, seed pods and drifting pollinators—built from cubic
  Bézier fills and strokes, with toggles for animated share, instanced transforms vs. live path
  deformation, paint tiers and split static/dynamic layers. `transit-atlas` renders a zoomable
  vector transit map of cubic metro loops, long diagonal trunk lines, dashed express and ferry
  lanes, aligned double tracks and up to 4,000 animated vehicles with click-triggered reroute
  waves. `clockwork` fills a canvas with watch movements of up to 4,000 meshing gears, balance
  wheels and linkages, turned as stamps under rotating matrices or re-traced each frame, with
  controls for segments per tooth, cutout fill rule, and strokes, bevels, gradients, blends,
  shadows and path clips one at a time.
- [`examples/games`](../examples/games): three macOS games, `resonance`, `prism-break` and
  `clatter-hill`, a marble run with sound.
- [`examples/graphics`](../examples/graphics): macOS windows that draw with the graphics library.
  `vector-flower.bx` builds a drawing with vector markup, `vector-chart.bx` styles one with a style
  sheet, and `truchet.bx` generates a tiling again as the window resizes. `boolean-shapes.bx`,
  `lettering.bx` and `offset-contours.bx` make new paths from paths with booleans, strokes and
  offsets, and `path-crossings.bx` finds where paths cross and which of their points is nearest.
  `circle-packing.bx` packs circles into a shape, `woven-knot.bx` weaves strands over and under
  each other, and `stained-glass.bx` finds the panes that crossing leads close off. `ink-sketch.bx`
  draws tapered ink strokes, and `natural-media.bx` paints a landscape in watercolor, gouache,
  crayon, charcoal, ink, marker and pencil.

`examples/tui/chromatic` also needs macOS, since it draws with the graphics library, and so does
`examples/tui/pulse` unless it runs with `--demo`.

To try one from your project, run
`brix <toolchain root>/examples/tui/basics/timer.bx -o .tmp/build/timer`.

## The Standard Library

[`stdlib`](../stdlib) is the standard library's source. A module's import path is its path under
`stdlib` without `.bx`, so `/os/path` is `stdlib/os/path.bx`. Declarations marked `+` are
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

- `brix x.bx` checks a program, and `brix x.bx -o x` builds and runs it. `brix --help` lists
  the rest.
- [Probes](style/agent-brix-probes.md): writing probes, and using them with `brix probes` to debug
  and profile. Use probes in place of prints.
- [Test modules](style/agent-brix-test-modules.md): testing a project's own code with `/test`, and
  running the tests with `brix test`.
- [Benchmarks](style/agent-brix-benchmarks.md): writing benchmarks, and measuring and comparing
  them with `brix bench`.
- `brix lint --help`: linting. [The lint rules](style/agent-brix-lint.md) explains each rule.
- [Reviewing](style/agent-brix-review.md): how to review Brix for style, when asked to.
- `brix init`: writes the `AGENTS.md` section and `CLAUDE.md` import that send a project's agents
  to this index.
