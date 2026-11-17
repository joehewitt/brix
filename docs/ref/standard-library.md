# The Standard Library

The standard library comes with the compiler. Its source is in the `stdlib` folder of the
toolchain, one folder for each library, and it is the reference for what each module declares:
a declaration marked `+` is public.

## Importing

A module's import path is its path under `stdlib`, without the file's extension: `/os/path` is the
module in `stdlib/os/path`. An import, `(import /os/path)`{.brix}, makes the module's public names
usable in the file that imports it. Modules covers imports in full.

A folder's module of the same name gathers its library: `/tui` is `stdlib/tui/tui`, which passes on
the public names of the library's modules. Importing it imports the whole library; importing one
module, such as `/tui/table`, imports only that module and what it passes on.

Some modules are imported into every module without being named:

- **Always**: `core` and the core modules for errors, numbers, strings, types, backing storage,
  arrays, function types, collections, lists, maps, ranges, streams, channels, events, tracked
  values, parsing and tasks.
- **In a module that writes stream patterns**: `core/tokenstream` and `core/patterns`.
- **In a module that writes markup**: `core/markup`.
- **In a module that declares or extends a style sheet**: `core/style`.
- **In a program's entry module, and in a module that writes probes, when probes are compiled
  in**: `core/probes`.

Modules whose names start with `_` are the libraries' own internals.

## Language support

### core

The types and runtime the language itself is built on. Most of it is imported into every module;
see [Importing](#importing). The Built-in types chapter covers what those modules declare: numbers,
`String`, `List`, `Map`, `Array`, ranges, errors, streams, channels, tasks and function types.

These core modules must be imported by name:

| Module | Holds |
| --- | --- |
| `/core/table` | `Table` and `ImmutableTable`: rows of a struct type stored as columns. See Collections. |
| `/core/bitmap` | `Bitmap` and `BitmapBuffer`: bits read and set by index. See Collections. |
| `/core/arena` | `Arena`: memory that is freed all at once, outside the collector. See Native code and memory. |
| `/core/array-builder` | `ArrayBuilder`: an array that grows as items are added. See Collections. |
| `/core/scratch` | A stack of scratch memory in an arena. |
| `/core/atom-pool` | Interning: one shared copy of each text. |
| `/core/gc` | `Weak`: a reference that does not keep its object alive. |
| `/core/animatable` | `Animatable`: the interface of values that motion can move between. |
| `/core/list-cursor` | `ListCursor`: a position in a list that steps and wraps. |
| `/core/dom` | `Node`: a generic tree of content nodes. |
| `/core/pattern-helpers` | Helpers for stream patterns that keep state while they match. |
| `/core/intrinsics` | Low-level operations, such as counting leading zero bits. |

### stream

Operations on streams. `/stream` gathers all but one of its modules.

| Module | Holds |
| --- | --- |
| `/stream/aggregate` | `reduce`, `scan`, `sum`, `count`, `first`, `last`, `all`, `some`. |
| `/stream/filter` | `take`, `skip`, `distinct`, `every`. |
| `/stream/combine` | `zip`, `product`, `indexed`, `repeat`, and `or`, which merges two streams as their items arrive. |
| `/stream/coincidence` | `buffer`, `window`, `pairwise`, `partition`. |
| `/stream/inspect` | `any`, `contains`, `reversed`, `forever`. |
| `/stream/sort` | `sort` and `merge-sort`. |
| `/stream/router` | `Router` and `EventDispatcher`: sending each item to the handlers it matches. |
| `/stream/timing` | Streams of numbers driven by time. Not part of `/stream`. |

### gc

Control over the garbage collector. `/gc` passes on `Weak` from `/core/gc`, and adds `on-reclaim`,
which runs a cleanup after an object is collected, as well as `collect-garbage` and
`return-free-memory`. Modules that hold native resources, such as open files, use it to release
them. `gc-memory` reports the collector's memory: live bytes, the heap goal, committed bytes and how
much of them spans hold, free committed bytes and how scattered they are, peak demand, the retention
target, the free bytes not yet returned, footprint and the collection count. `set-minimum-heap-goal`
sets the smallest heap goal, the size the heap may reach before a collection starts, which is 32 MB
by default. A program that would rather spend memory than run more collections, such as the
compiler, sets a larger one.

## Values and data

### time

Durations, dates, clocks and timers. `/time` gathers every module except `/time/timer` and
`/time/fake-clock`.

- `/time/duration`: `Duration`, the units from `ns` to `milleniums`, `wait` and `timeout`.
- `/time/clock`: `ClockTime`, `measure`, and the `time` effect, whose doors every read of the clock
  and every timed wait go through; see Functions.
- `/time/datetime`: `DateTime`, `Date`, `Time`, `Month`, `now` and `today`.
- `/time/zone`: `TimeZone`, `utc` and `local`.
- `/time/timestamp`: reading and writing ISO 8601, RFC 2822 and RFC 3339 timestamps.
- `/time/deadline`: `Deadline`.
- `/time/debounced`: `debounced` and `throttled` streams.
- `/time/flicks`: `Flicks`, a unit of 1/705,600,000 of a second, chosen so that a frame at common
  video and audio rates lasts a whole number of flicks.
- `/time/timer`: `Timer`, `TimerLoop` and `AnimationLoop`.
- `/time/fake-clock`: `FakeClock`, a fake for `time` whose clock moves only when `advance` moves it.

### math

- `/math`: `π`, angles in radians and degrees (`rad`, `deg`, `°`), `abs`, `round`, `floor`,
  `sqrt`, trigonometry, `random`, `min`, `max` and `clamp`. `Random` is a generator of its own:
  `(call Random 42u)` gives the same numbers on every run, and `split` makes another from it.
- `/math/easing`: `Easing` and `CubicBezier`.
- `/math/motion`: motions that move a value over time: `spring`, `ease-in-out`, `linear`, `steps`,
  `decay` and `keyframes`.

### units

`/units` declares `Percent` (`%`), `Ratio` and scaling, and passes on `/units/distance`, the
lengths from `nm` to `miles`. Pixels are declared in `/graphics/primitives` and durations in
`/time/duration`. Units explains how unit families are declared and converted.

### data

Data formats. `/data` itself declares only the interfaces of readable and writable data; each
format is its own module.

| Module | Holds |
| --- | --- |
| `/data/json` | `JSON`, `parse-json` and `to-json`. |
| `/data/jsonl` | JSON Lines. |
| `/data/jsonrpc` | JSON-RPC requests and responses. |
| `/data/csv` | A stream pattern for CSV files. |
| `/data/markdown` | Parsing Markdown into a tree of nodes. |
| `/data/sexp` | S-expressions. |
| `/data/source-tree`, `/data/source-sexp` | An editable tree of source text. |
| `/data/plist` | Binary property lists. |
| `/data/protobuf` | Reading and writing protocol buffers. |
| `/data/binary` | `ByteReader` and `ByteWriter`. |
| `/data/stream` | `DataStream`. |
| `/data/sqlite` | `SQLite` databases, statements and rows. |
| `/data/diff` | Line diffs. |
| `/data/fuzzy` | Fuzzy matching and edit distance. |

### regex

`/regex`: `Regex`, which matches, finds, replaces and splits text, and `RegexMatch`.

### crypto

`/crypto` gathers `ChaCha20Poly1305` encryption, SHA-1 and SHA-512 digests, HMAC and HKDF,
`random-bytes`, and an SRP client for password logins.

## The system

### os

Files, processes and the environment. There is no `/os` module; import each one.

- **Files and paths:** `/os/path` (`Path`, with joining by `/`, file information, directory
  listings and change notifications), `/os/file` (`File`, standard input and output, and reading
  and writing whole files), `/os/file-tree` and `/os/file-session` (watched trees of files).
- **The environment:** `/os/env` (environment variables and the working directory), `/os/cli`
  (command lines: `CommandLine`, subcommands, flags, options and arguments), `/os/shell` (the
  login shell's environment), `/os/bundle` and `/os/app-support` (an app's bundle and its
  application support folder).
- **Processes:** `/os/process` (`Process`, and pipes to and from processes), `/os/pty`,
  `/os/signal`, `/os/memory-pressure` (`memory-pressure`, which sends a warning or critical level
  when macOS or iOS runs short of memory), `/os/tty`, `/os/shared-memory`, `/os/message-channel`,
  `/os/outbox` and `/os/mach-port`.
- **Terminals:** `/os/terminal` gathers a terminal emulator, a shell session in a pseudo-terminal,
  and terminal paths.

### net

Networking. There is no `/net` module; import each one.

| Module | Holds |
| --- | --- |
| `/net/url` | `URL` and query strings. |
| `/net/dns` | Addresses, ports and name resolution. |
| `/net/socket` | TCP and TLS connections and listeners. |
| `/net/http` | `http-get`, `http-post`, `http-fetch`, requests and responses. |
| `/net/http-server` | `HTTPServer` and its routes, which may answer WebSocket upgrades. |
| `/net/sse` | Server-sent events. |
| `/net/websocket` | `WebSocket`, either end of a WebSocket connection. |
| `/net/dns-sd` | Finding services on the local network. |

### git

`/git` reads and changes Git repositories by running the `git` command: status, logs, diffs,
commits, merges, fetching and pushing, worktrees, and watching a worktree for changes. It needs
`git` to be installed.

## Interfaces

### ui

What graphical and terminal interfaces share. `/ui` gathers commands and their history, undo, key
combinations and key maps, input, selection, modes, recent items and type-to-select. Import
`/ui/panes` for pane layouts, `/ui/virtual`, `/ui/virtual-layout` and `/ui/virtual-recycler` for
lists that create only the rows in view, and `/ui/app-state` for state saved between runs.

### gui

Native desktop and mobile apps, written as markup and styled with style sheets. `/gui` gathers 78
modules and `/ui`:

- **Apps and layout:** `App`, `Window`, `Box`, flex, flow and grid layouts, positions, themes and
  style shorthands.
- **Text and images:** `Text`, `Label`, `TextInput`, images, vector images and icons.
- **Controls:** buttons, check boxes, switches, toggles, sliders, number and color fields, color
  pickers, selects, combo boxes, segmented controls, progress bars and disclosures.
- **Collections:** list boxes, tree boxes, column trees, thumbnail grids, virtual lists, tab bars,
  sidebars, toolbars and timelines.
- **Overlays:** popups, popovers, menus, dialogs, tooltips, toasts and command palettes.
- **Motion and gestures:** animation, keyframes, swiping, pinching, dragging, zooming and resizing.
- **The system:** the clipboard, file dialogs and status items.

Other modules are imported by path: `/gui/desktop` (the trash and opening URLs),
`/gui/web-view`, `/gui/terminal-view`, `/gui/workbench`, and views of UIs that another process
renders. `/gui/markdown`, `/gui/code-editor`, `/gui/file-browsing` and `/gui/chat` each gather a
larger component. Icons from the Lucide set are each a module of their own, such as
`/gui/icons/lucide/star`.

### tui

Terminal apps, written as markup with the same model as `gui`. `/tui` gathers apps, boxes, windows,
layouts, canvases, borders, styles, themes, colors, events and shortcuts, text and a text editor,
tables, buttons, dialogs, forms, panels, progress bars, spinners and status bars, as well as
`/ui/input`, `/ui/key-combo` and `/units`. Import `/tui/listbox`, `/tui/treebox`,
`/tui/columntree`, `/tui/virtual` and `/tui/terminal` separately. `/tui/code-editor` and
`/tui/file-browsing` each gather a larger component.

### code

The model behind code editors. `/code` gathers the text of a document, an editing session with
selections and undo, a document backed by a file, folding and comment toggling. Import
`/code/syntax` for syntax coloring, `/code/code-find` for find and replace, `/code/file-names` and
`/code/file-text` for finding files by name and by content, and `/code/structure` for editing Brix
source by its structure.

## Graphics and media

### graphics

Drawing, text, shaders and vector geometry. There is no `/graphics` module; import each one.

- **Values:** `/graphics/primitives` (`Coord` in `px`, `Rect`, `Dimensions`, `Insets` and drawing
  settings), `/graphics/point`, `/graphics/color` (`RGBA`, `HSVA` and named colors),
  `/graphics/matrix` and `/graphics/gradient`.
- **Drawing:** `/graphics/canvas` (`Canvas`, drawing to images, SVG and PNG), `/graphics/image`,
  `/graphics/text` (fonts and text layout), `/graphics/outline`, `/graphics/surface` and
  `/graphics/movie`.
- **Shaders:** `/graphics/shader` and filters such as CRT, pixelate, wave, halftone and glitch.
- **Geometry:** `/graphics/path`, `/graphics/vector` (path commands and shapes), SVG paths, curves
  and curve fitting, intersections, polygons, offsets, strokes and boolean operations, and vector
  networks.
- **Documents:** a scene graph of graphic nodes, paint styles, and vector documents with their
  rendering.

### media

Sound. There is no `/media` module. `/media/synth` declares sound patches in `hz`,
`/media/mixer` mixes voices, `/media/audio-output` plays them, `/media/spectrum` analyzes them,
and `/media/wave-file` writes WAV files.

## AI

### ai

Conversations with AI models. `/ai` gathers the model of a conversation (messages, tools and
events), branching chats, chat sessions and their storage in SQLite, API keys and sign-ins, and
image generation. Each provider is imported on its own: `/ai/claude` and `/ai/codex` drive those
command-line tools, `/ai/openai/completions`, `/ai/openai/responses` and `/ai/openai/images` call
the OpenAI API, and `/ai/mcp` serves tools over MCP. `/ai/services` connects the default setup.

## Embedding

### script

`/script` is a small expression language for formulas that a program's users write: reading,
parsing, printing and evaluating them, rewriting them, and solving them for a value. Its values
include numbers, points, colors and ranges.

## Tools

These libraries serve the toolchain rather than programs.

- **lang** is the Brix compiler. Programs use one part of it: `/lang/product/mac-app` and
  `/lang/product/ios-app` declare the app bundle a program builds into, with its name, id, icon
  and document types.
- **probes** reads the records that probes write, for `brix probes`. Writing probes needs no
  import; see Probes.
- **test** is the library test modules use: `/test` declares test cases and how they report. See
  `brix test --help`.
