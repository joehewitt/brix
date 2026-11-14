# Batteries included

Yumi includes libraries for data, files, networks, graphics, interfaces, and the language itself. The useful part is how they connect: HTTP bodies supply streams, patterns recognize records, units describe time and geometry, and reactive values feed interfaces.

This is a capability map, not an inventory of every function.

## At a glance

| Area | Capabilities | Modules |
| --- | --- | --- |
| Collections and streams | Lists, maps, arrays, builders, tables, bitmaps; selection, aggregation, combination, buffering, sorting | `/core`, `/stream` |
| Files and storage | Files, databases, persistent lists, B-trees | `/os/file`, `/file/db`, `/file/dblist`, `/file/btree` |
| Host environment | Arguments, environment, paths, processes, signals | `/os/cli`, `/os/env`, `/os/path`, `/os/process`, `/os/signal` |
| Time and quantities | Durations, timestamps, dates, clocks, timers, zones, mathematics, units | `/time`, `/math`, `/units` |
| Formats | JSON, CSV, Markdown, binary input | `/data/json`, `/data/csv`, `/data/markdown`, `/data/stream` |
| Networking | DNS, TCP/TLS sockets, URLs, HTTP, WebSockets, server-sent events, API helpers, JSON-RPC | `/net`, `/api`, `/data/jsonrpc` |
| AI | Models, conversations, media, streaming responses, OpenAI integration | `/ai`, `/ai/model`, `/ai/chat`, `/ai/media`, `/ai/openai/completions` |
| Graphics | Geometry, colors, paths, curves, drawing, text, layers | `/graphics` |
| Terminal UI | Layout, controls, editors, dialogs, focus, shortcuts, themes | `/tui` |
| Graphical UI | Applications, windows, controls, input, scenes, drawing integration | `/gui` |
| Testing and observation | Test cases, result presentation, probes, inspection, counters, profiling | `/test`, `/core/probes` |
| Language tooling | Source, syntax trees, declarations, types, lint, compiler representations | `/lang` |

## Collections and stream processing

Stored data and ongoing work use the same sequence vocabulary. A consumer can filter records, retain a running total, or collect a finite result without prescribing where the items came from.

:::example-pair

- [Focused import](#source-running-total--import): Bring in the aggregation operations.
- [Running result](#source-running-total--scan): Start from zero and emit each updated accumulator.
- [Combination](#source-running-total--combine): Add each input. Collecting the results produces `2, 5, 9`.

```yumi #running-total
⟦import⟧/stream/aggregate⟦/import⟧

. totals = ⟦scan⟧scan(0, [2, 3, 4])⟦/scan⟧ • using ⟦combine⟧:: total, item => total + item⟦/combine⟧
-- List(totals)
```

:::

- **Storage:** lists, maps, ranges, arrays, immutable storage, builders, bitmaps, and tables.
- **Processing:** aggregate, filter, combine, coincidence, inspect, router, and sort groups under `/stream`.
- **Choose deliberately:** some operations need the whole input; others retain only an accumulator or nearby items.

See [Collections and data layout](collections-and-data-layout.md) and [Stream pipelines](stream-pipelines.md).

## Files and persistent storage

| Need | Building block |
| --- | --- |
| Read or write a document | File access and byte/text input |
| Decode file contents | Format libraries or stream patterns |
| Retain application data across runs | Database and persistent-list facilities |
| Organize lookup and updates | B-tree support |

File streams and persistent structures solve different problems: one supplies input to a computation; the other retains and organizes data for later use.

## Operating-system integration

A command-line tool can work with arguments, environment variables, paths, processes, and signals. The [C bridge](modules-native-code-and-memory.md#the-c-bridge-reaches-existing-native-libraries) reaches native facilities beyond those wrappers.

- Parse the command's inputs with `/os/cli`.
- Read environment and resolve paths with `/os/env` and `/os/path`.
- Coordinate other processes and host signals with `/os/process` and `/os/signal`.

## Time, mathematics, and units

Time facilities distinguish elapsed quantities from calendar concepts. Durations, timestamps, dates, clocks, timers, and zones have different jobs.

:::example-pair

- [Typed time](#source-duration--conversion): The connected units convert milliseconds to seconds.
- [Include the unit](#source-duration--display): Format the value with its canonical suffix.

```yumi #duration
/time/duration

⟦conversion⟧. timeout Seconds = 1500ms⟦/conversion⟧
-- ⟦display⟧timeout as UnitString⟦/display⟧
```

:::

Mathematics and unit families also support angles, distances, graphics coordinates, and layout. See [Numerical units](numerical-units.md) for conversion rules and their limits.

## Text, structured data, and binary input

| Format or task | Capability |
| --- | --- |
| JSON | Exchange API payloads and structured files |
| CSV | Move tabular data into ordinary collections |
| Markdown | Consume formatted text |
| Binary streams | Decode fields whose byte order and representation matter |
| Custom format | Use [stream patterns](stream-pattern-matching.md) to write a grammar |
| Decoded values | Use [structural patterns](structural-pattern-matching.md) to recognize fields and shapes |

Each format module defines its actual coverage; the presence of a format name does not imply every variant is supported. For authored output, [string templates](string-templates.md) keep the document visible while expressions supply its changing parts.

## Networking and APIs

Networking spans request-and-response work and longer-lived connections:

- **Connect:** DNS, URLs, and TCP or TLS-backed sockets.
- **Request:** HTTP and JSON API helpers.
- **Follow arrivals:** WebSockets and server-sent events.
- **Exchange structured calls:** JSON-RPC facilities.

Waiting passes through ordinary helpers. This fragment assumes imports from `/net/http` and a `URL` named `url`:

:::example-pair

- [Call the helper](#source-request--caller): Use ordinary call syntax.
- [Declare the helper](#source-request--helper): There is no `async` marker.
- [Wait inside it](#source-request--io): The request can suspend for network IO without adding `await` at either call.

```yumi #request
⟦caller⟧-- page-body(url)⟦/caller⟧

━━━━━━━━━━━━━━━━━━━━
⟦helper⟧+ page-body String⟦/helper⟧
    - url URL

page.text

.. page = ⟦io⟧http-get(url)⟦/io⟧
```

:::

Client/server roles, transport options, and protocol coverage are contracts of the particular module. See [Concurrency and reactivity](concurrency-and-reactivity.md) for waiting and task behavior.

## AI applications

The AI libraries connect models, conversations, media, and streaming responses. The supplied OpenAI integration implements provider-specific behavior.

A chat interface brings together:

1. A conversation model and request.
2. Incremental response data.
3. A live display that updates as results arrive.

Shared abstractions do not imply identical provider support for every media or response feature.

## Geometry and graphics

| Area | Capabilities |
| --- | --- |
| Geometry | Points, vectors, matrices, shapes, curves, intersections, polygons |
| Path operations | Boolean operations, offsets, strokes, transformations |
| Rendering | Colors, canvases, drawing, text, layers, artwork |

Geometry can be useful in a data-processing program on its own. Rendering adds platform integration. Units and operator overloads let domain relationships appear directly in expressions.

## Terminal applications

The `/tui` family supplies controls with layout, input, and state:

- **Structure:** boxes, forms, dialogs, lists, trees, tables.
- **Content and editing:** labels, buttons, text editing, progress displays, status areas.
- **Interaction:** keyboard shortcuts, focus, scrolling.
- **Presentation:** themes and styles.

Components use markup and reactive state. See [Markup and stylesheets](markup-and-stylesheets.md) for the shared declarative model.

## Graphical applications

The `/gui` family works with `/graphics` to provide applications, windows, controls, layout, input, styles, scenes, and drawing integration.

The same object-tree model carries over from terminal applications. Concrete controls and rendering remain library-specific. The current implementation includes macOS-native and Metal-related facilities; platform support is part of evaluating what you can build today.

## Testing and observation

| Goal | Support |
| --- | --- |
| State expected behavior | Test cases and result presentation in `/test` |
| Explain a particular run | Events, values, object inspection, counters, and profiling in `/core/probes` |

Observations can retain the object and task context needed to understand channels and UI state. See [Probes and observability](probes-and-observability.md).

## Language tooling

The `/lang` family works with source, syntax trees, declarations, types, lint, and compiler representations. It underpins the project's self-hosted language work.

These facilities make Yumi itself a domain for Yumi programs. They include internal machinery; not every compiler representation is a stable extension API.

## The value of including the pieces together

A network response can feed a stream grammar. Its records can become union values, pass through a pipeline, and update a declarative interface. Probes can follow the objects through that work.

The shared models matter as much as the breadth: values, sequences, state, and behavior remain recognizable across library boundaries.
