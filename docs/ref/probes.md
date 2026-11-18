# Probes

A probe records what a program does while it runs: a line of text with values in it, a value that
changed, a count, or how long a stretch of work took. Probes are written in the source, but they
are not part of the program. A build leaves them out unless it is asked for them with `--probes`,
and a build that has them writes what they record to a trace file, not to the program's output.
`brix` shows the records on stderr while the program runs, and `brix probes` reads a trace
afterward: it filters records, times groups, sums counters and compares runs.

This chapter covers the kinds of probe and what each records, the code a probe can hold, where
probes can be written, how a build chooses its probes, the console and the trace, the commands that
read a trace, and how probes differ from prints.

## A first probe

```brix
(main
  (let names (list "Ada" "Grace" "Edsger"))
  (for (in name names)
    (probe-log _ (text: "greeting " name))
    (print (str "Hello, " name "!"))
  )
  (probe-log greet.done (text: "greeted " (. names length) " people"))
)
```

```output
Hello, Ada!
Hello, Grace!
Hello, Edsger!
```

The output is the same with probes or without them. Saved as `greet.bx` and run with
`brix greet.bx --probes`, the program prints those three lines to stdout, and `brix` writes what the
probes recorded to stderr:

```text
·      0ns  greeting Ada  greet.bx:4
·   16.0µs  greeting Grace  greet.bx:4
·   20.0µs  greeting Edsger  greet.bx:4
·   23.0µs  [greet.done] greeted 3 people  greet.bx:7
── probes ── exit 0 · 246.7ms · 4 records
busiest: greeting … 3× greet.bx:4
records: 4 logs over 23.0µs
trace: …/.tmp/build/probes/greet-1794767186696.trace
query: brix probes summary · log · agg --field F · durations · tree · timeline · outliers (brix probes --help)
```

Each line is one record: the time since the run's first record, the text with its values written
in, and the file and line of the probe. The first probe has no key, which `_` stands for; the
second has the key `greet.done`, shown in brackets. The lines below the rule are the footer, which
sums up the run. Times differ from run to run, and so do the times in every console output in this
chapter.

In rows, a probe is a row that starts with `//`. The two probes above are
`// greeting $name`{.rows} and `//greet.done// greeted ${names.length} people`{.rows}. In rows,
`//` never starts a comment; see Source text.

## Kinds of probe

There are five probe forms. Each is a step, and each can be written where a step can, with the
exceptions in [Where a probe can be written](#where-a-probe-can-be-written).

| Form | Rows | What it records |
| --- | --- | --- |
| `(probe-log _ (text: "paint " frame))`{.brix} | `// paint $frame`{.rows} | A **log**: a line of text, with each value kept as a field. |
| `(probe-log gui.paint (text: "painted " n))`{.brix} | `//gui.paint// painted $n`{.rows} | A log under a key. |
| `(probe-log net.request (items: url status))`{.brix} | `//net.request -> url, status`{.rows} | An **event**: a key and values, with no text. |
| `(probe-log tui.box.root)`{.brix} | `//tui.box.root`{.rows} | An event with no values. |
| `(probe-assign status value)`{.brix} | `//status = value`{.rows} | A **property set**: an event whose fields are `property` and `value`. |
| `(probe-count parse.tokens)`{.brix} | `//parse.tokens ++`{.rows} | A **counter**, plus one. |
| `(probe-count parse.bytes size)`{.brix} | `//parse.bytes += size`{.rows} | A counter, plus an amount. |
| `(probe-group + app.frame (text: "frame " n))`{.brix} | `//+app.frame// frame $n`{.rows} | The start of a **group**, with text. |
| `(probe-group - app.frame (text: "done"))`{.brix} | `//-app.frame// done`{.rows} | The end of the group, with text. |
| `(probe-group + _ (text: "setup"))`{.brix} | `//+ setup`{.rows} | The start of a group with no key. |
| `(probe-group + parse.pass)`{.brix} | `//+parse.pass`{.rows} | The start of a group with no text, kept for its time. |
| `(probe-group +- tui.layout (text: "layout " box))`{.brix} | `//+-tui.layout// layout $box`{.rows} | A group that ends where the block it is written in is left. |
| `(probe gui.frame.objects (let start0 5))`{.brix} | `//!gui.frame.objects . start0 = 5`{.rows} | Nothing: a **statement probe**, a step that runs only in a build that has it. |

A probe of an object writes `(: object key)` in place of the key; see
[Probes of an object](#probes-of-an-object).

### Logs

A log is a line of text. Its `text:` part is strings and values, as in `str`. Each value is kept in
the record as a field of its own, with its type, so the tools can filter and add up values, not
only read the text. A log with a key shows the key in brackets before its text.

### Events and property sets

```brix
(main
  (let orders (list (tuple "apple" 3) (tuple "pear" 1) (tuple "apple" 5) (tuple "fig" 2)))
  (let total 0)
  (for (in (tuple sku quantity) orders)
    (probe-log shop.order (items: sku quantity))
    (+= total quantity)
    (probe-assign shop.total total)
  )
  (print total)
)
```

```output
11
```

An event has a key and values but no text. In rows it is `//shop.order -> sku, quantity`{.rows}.
A property set records that a value changed, as `//shop.total = total`{.rows}; it is an event
whose fields are `property` and `value`. With `--probes`, the console shows:

```text
·      0ns  shop.order: apple, 3
·      0ns  shop.total = 3
·      0ns  shop.order: pear, 1
·      0ns  shop.total = 4
·    1.0µs  shop.order: apple, 5
·    1.0µs  shop.total = 9
·    1.0µs  shop.order: fig, 2
·    1.0µs  shop.total = 11
── probes ── exit 0 · 245.8ms · 8 records
busiest: shop.order 4× · shop.total 4×
records: 8 events over 1.0µs
```

The console shows no file and line for events; `brix probes log` shows them. Events suit values
that the tools should add up: `brix probes agg --field quantity sum,count,max --by sku` sums the
orders above by product. See [Reading a trace](#reading-a-trace).

### Counters

```brix
(main
  (let values (list 72 15 98 41 63 8 55))
  (call sort values)
  (print values)
)

(fn-do + sort _
  (param values (list Int))
  (for (in next (..< 1 (. values length)))
    (let at next)
    (while (and (> at 0) (> ([] values (- at 1)) ([] values at)))
      (probe-count sort.swaps)
      (let moved ([] values at))
      (= ([] values at) ([] values (- at 1)))
      (= ([] values (- at 1)) moved)
      (-= at 1)
    )
    (probe-count sort.passes)
  )
)
```

```output
8, 15, 41, 55, 63, 72, 98
```

A counter adds one, or the amount it is given, to a number kept for the whole run. It writes no
record each time: counters are summed in memory and written once, when the program ends, so they
appear only in the footer and in `brix probes counters`. With `--probes`:

```text
── probes ── exit 0 · 244.5ms · 0 records
counters: sort.passes 6 · sort.swaps 13
```

- **The amount** is converted to `U64` as `as` converts it, so `1.5` adds 1 and a negative amount
  wraps around. Text is refused.
- **One number per run.** A counter is not a series. To see a value change over time, record it
  with an event or a property set.
- **A counter of an object** is kept for each object: `(probe-count (: this parse.reads))`{.brix}.

### Groups

```brix
(main
  (let words (list "pear" "fig" "plum"))
  (probe-group + shop.load (text: "loading " (. words length) " items"))
  (for (in word words)
    (probe-group +- shop.item (text: "item " word))
    (probe-log _ (text: "weighing " word))
  )
  (probe-group - shop.load (text: "loaded"))
  (print (call find words "fig"))
)

(fn-do + find Int
  (param words (list String))
  (param wanted String)
  (probe-group +- shop.find (text: "finding " wanted))
  (for (in at (..< 0 (. words length)))
    (return-if at (if (== ([] words at) wanted)))
  )
  (return -1)
)
```

```output
1
```

A group marks a stretch of work. `+` begins it, `-` ends it, and what is recorded in between nests
inside it. With `--probes`:

```text
·      0ns  + loading 3 items  shop.bx:3
·    1.0µs    + item pear  shop.bx:5
·    1.0µs      weighing pear  shop.bx:6
·    1.0µs    - item pear 0ns  shop.bx:5
·    2.0µs    + item fig  shop.bx:5
·    2.0µs      weighing fig  shop.bx:6
·    2.0µs    - item fig 0ns  shop.bx:5
·    2.0µs    + item plum  shop.bx:5
·    2.0µs      weighing plum  shop.bx:6
·    2.0µs    - item plum 0ns  shop.bx:5
·    3.0µs  - loaded 3.0µs  shop.bx:8
·    3.0µs  + finding fig  shop.bx:15
·    3.0µs  - finding fig 0ns  shop.bx:15
── probes ── exit 0 · 205.2ms · 14 records
groups: shop.load 3.0µs · shop.find 0ns · shop.item 3× 0ns (max 0ns)
busiest: weighing … 3× shop.bx:6
records: 14 logs over 3.0µs
```

- **The end line** shows the end's text and how long the group took.
- **`+-`** begins a group that ends where the block it is written in is left, by any way out: at
  the end of each turn of a loop, and at a `return`, as in `find` above. In a value body's
  `before`, a `+-` group also spans the value.
- **A group with no text** shows its key, as `+ app.load`. It is kept for its time, which is what
  `brix probes durations`, `tree` and `outliers` read.
- **An end closes the innermost open group** of its task, whatever its key. Keys on an end name it
  for the reader; they are not matched.
- **An end with no group open** stops the program with a panic, in a build that has the probe.
- **Tasks.** Each task has its own groups; see [Tasks and threads](#tasks-and-threads).

### Statement probes

```brix probes
(main
  (let items (list 3 1 2))
  (probe app.check
    (if (!= (. items length) 3)
      (print "wrong count")
    )
  )
  (probe _ (print (str "items: " items)))
  (print "ok")
)
```

```output
items: 3, 1, 2
ok
```

`probe` holds one step, which runs only in a build that has the probe, and records nothing. It is
for work that only looking needs, such as a check or a print while finding a fault. In rows it is
`//!` followed by a key and the step. Run without `--probes`, the program above prints only `ok`.

## Text and values

The text of a log is strings and values. In rows, values are written into the text with `$`:

| Rows | Form | Meaning |
| --- | --- | --- |
| `$name`, `$a.b`, `$a?.b` | `name`, `(. a b)`, `(?. a b)` | A value, written as text. |
| `${expr}` | the expression | Any expression. |
| `$?name`, `$?{expr}` | `(preview name)` | A **preview**: an object or array by its id, not as text. |
| `$!name`, `$!{expr}` | `(snapshot name)` | A **snapshot**: the object's contents, copied into the trace. |
| `\$`, `$$` | `"$"` | A dollar sign. |

How a value is kept:

- **Numbers, `Bool`, `Char` and `String`** are kept as they are, each a field of its own type.
- **In a log**, any other value is written as its text, made as a print makes it: a list as
  `1, 2, 3`, and an object of a class with no conversion to `String` as `[Object module:Type]`.
- **In an event**, an object of a class (a `List` is one), an `Array`, or an `Inspectable` tuple is
  written by its id, as `List#1002`. Other values are written as text.
- **A preview** writes an object or array by its id even in a log. A preview of a value that has no
  id writes the name of its type in brackets, as `[Tuple]`.
- **A snapshot** of an `Inspectable` object runs its `inspectable-snapshot` and records what that
  writes; of an array, it copies its bytes; of any other object, it writes only the id. An event's
  values in rows take previews but not snapshots. `brix probes` counts snapshots but does not show
  what they hold; an inspector attached to the running program does.
- **Ids** come from a counter in the runtime and change when the program changes. Don't depend on
  them.

Each value is a field, and each field has a name, which the tools use, as in `--where 'ms > 2'` or
`agg --field quantity`:

| Value | Field name |
| --- | --- |
| A name | The name: `frame`. |
| A chain of members | Its path: `values.length`. |
| A call | The function's name: `noisy`. |
| A preview or snapshot | The name inside it. |
| Anything else | `value` and its place among the values: `value1`. |
| A property set | `property` and `value`. |

Names bound by a tuple pattern are names too, so `(for (in (tuple sku quantity) orders) …)` gives
the fields `sku` and `quantity`.

## Probes of an object

A probe of an object writes `(: object key)` in place of the key, as
`(probe-assign (: items status) "ready")`{.brix}, in rows `//items:status = "ready"`{.rows}. Its
record belongs to the object, and the console shows it with the object's id, as
`List#1002.status = ready`. In a method, the object is `this`:
`(probe-count (: this parse.reads))`{.brix}.

- **The object** must be an object of a class, an `Array` or an `Inspectable` value. Any other
  value is an error where the probe compiles: `Not supported`.
- **A `+-` group** cannot be of an object.

## Code under a probe

Rows indented under a probe belong to it. In Brix they are the probe form's last item, a
`(block …)`. They run only in a build that has the probe, before it records, so they can work out a
value for the probe alone:

```brix probes
(main
  (let scores (list 12 40 7 33))
  (probe-log stats.best (items: best)
    (block
      (lazy best (call highest scores))
    )
  )
  (print (. scores length))
)

(fn-do + highest Int
  (param scores (list Int))
  (print "looking for the best score")
  (let best 0)
  (for (in score scores)
    (if (> score best)
      (= best score)
    )
  )
  (return best)
)
```

```output
looking for the best score
4
```

In rows the probe is `//stats.best -> best`{.rows}, with `.. best = highest(scores)` under it. The
build above has the probe, so `highest` runs and prints. Without `--probes` the program prints only
`4`: none of a probe is compiled, neither its values nor the code under it.

```brix
(main
  (let count 0)
  (probe-count demo.calls)
  (probe-count demo.total (call noisy))
  (probe-log demo.value (items: (call noisy)))
  (probe-assign demo.state (call noisy))
  (probe-log _ (text: "said " (call noisy)))
  (probe-count demo.more
    (block
      (= count 99)
    )
  )
  (+= count 1)
  (print count)
)

(fn-do + noisy Int
  (print "noisy")
  (return 1)
)
```

```output
1
```

Built with `--probes`, the same program prints `noisy` four times and then `100`: each value is
worked out, and the block under `demo.more` runs. So never put work the program needs in a probe.
What the program needs belongs in ordinary code, where it runs in every build.

## Where a probe can be written

| Place | |
| --- | --- |
| A step body: `main`, `fn-do`, a loop, an `if`, a `match` arm, a `do` block | Yes |
| A constructor or method, including probes of `this` | Yes |
| The `before` of a value body | Yes |
| The `block` of a function value, before its answer | Yes |
| An arm of a decision table, before the arm's result | Yes |
| A value body without `before`, or after its value | No |
| Between the feeds of a list body | No |
| A `where` | No |
| The value of a `let`, or any other value | No |
| The members of a type | No |
| The top level of a file | No |

A value body has one value, so a probe goes in its `before`:

```brix compile-error
(main
  (print (call double 4))
)

(fn-expr + double Int
  (param n Int)
  (* n 2)
  (probe-log _ (text: "doubling " n))
)
```

```output
Exception: A `fn-expr` body has one value, and (probe-log …) follows the value (* …). Put checks, prints, probes and `do` forms in `(before …)`, other steps in a `(do …)` there, and lazies in `(where …)`, or write `fn-do` and return with `(return value)`.
<no file:9>
      (probe-log _ (text: "doubling " n))
      ^
```

```brix
(main
  (print (call double 4))
)

(fn-expr + double Int
  (param n Int)
  (before
    (probe-log _ (text: "doubling " n))
  )
  (* n 2)
)
```

```output
8
```

At the top of a file, a probe is refused like any other step:

```brix compile-error
(probe-log _ (text: "loading"))

(main
  (print "ok")
)
```

```output
Exception: (probe-log …) cannot be a form at the top level of a file, which takes Top. A file holds declarations at its top level. Code that runs goes inside `(main …)`.
<no file:2>
    (probe-log _ (text: "loading"))
    ^
```

## Keys

A key is one or more names joined by dots, each a lowercase letter followed by letters, digits and
hyphens, as in `tui.box.root` or `net.request-2`. `_` stands for no key. A key does two things:

- **It names what the probe explains.** Records, groups and counters are listed by key.
- **It selects.** `--probes` chooses probes by key, and `brix probes` filters by key. Give each
  probe a key under the part of the program it explains, such as `parse.tokens` in a parser, so a
  selector can take the part whole.

A probe with no key is for looking at one thing now. Remove such probes when the looking is done,
and keep the ones that explain work someone will want to see again.

## Choosing probes

### Without `--probes`

A build without `--probes` leaves every probe out:

- **Nothing runs.** No value of a probe is worked out and no code under it runs, as the `noisy`
  example above shows.
- **Nothing is built.** The executable has no probe runtime and no catalog. A function that only a
  probe calls is not built.
- **Names are still checked.** A probe that reads a name that does not exist is an error in every
  build:

```brix compile-error
(main
  (probe-log _ (text: "missing " nowhere))
  (print "ok")
)
```

```output
Exception: Name "nowhere" not found
<no file:3>
      (probe-log _ (text: "missing " nowhere))
                                     ^^^^^^^
```

The other checks of a probe are made only where it compiles: a counter given text, or a probe of
a value that is not an object, builds without `--probes` and is an error with it.

### `--probes` and selectors

`brix app.bx --probes` builds the program with probes, runs it, and records its trace. Without
`-o`, the executable is built at `.tmp/build/probes/bin/app`; with `-o PATH` it is built there.
`--compile` builds without running, and needs `-o`.

| Option | Meaning |
| --- | --- |
| `--probes [SELECTORS]` | Compile probes in, run, and record the trace. |
| `--timeout D` | Stop the program after `D` (`500ms`, `30s`, `2m`) and exit with 124. |
| `--trace PATH` | Write the trace at `PATH`. |
| `--quiet-probes` | Print only the footer, not each record. |

`--trace` and `--quiet-probes` need `--probes`. `--probes` and `--timeout` need a target that runs
on this machine.

Without selectors, `--probes` compiles every probe in the program's own file, and, in every other
file, each log, group and statement probe with no key. Selectors choose instead. They are joined by
commas or spaces in one argument:

| Selector | Chooses |
| --- | --- |
| `key`, `key.*` | The probes with that key and the keys inside it. |
| `log:` | Logs with no key. |
| `log:key` | Logs under a key. |
| `log` | Every log, with a key or not. |
| `counter:*`, `counter:key` | Every counter, or the counters under a key. |
| `event:key`, `data:key`, `statement:key` | Events, groups with no text, or statement probes under a key. |
| `profile:key` | Counters and groups with no text under a key, kept as metrics: no records, only totals. |
| `gc` | The collector's counters: `gc.cycles`, `gc.pauses` and `gc.assist.nanos`. |
| `*`, `all` | Every probe, the core library's too, and the collector's counters. |

```brix
(main
  (let words (list "one" "two" "three"))
  (probe-log parse.words (text: "parsing " (. words length) " words"))
  (for (in word words)
    (probe-count parse.tokens)
    (probe-log _ (text: "token " word))
  )
  (print (. words length))
)
```

```output
3
```

What each selection records, from the console, without its footer's last lines:

```text
$ brix words.bx --probes
·      0ns  [parse.words] parsing 3 words  words.bx:3
·    1.0µs  token one  words.bx:6
·    2.0µs  token two  words.bx:6
·    2.0µs  token three  words.bx:6
counters: parse.tokens 3
$ brix words.bx --probes parse
·      0ns  [parse.words] parsing 3 words  words.bx:3
counters: parse.tokens 3
$ brix words.bx --probes parse.tokens
counters: parse.tokens 3
$ brix words.bx --probes log:
·      0ns  token one  words.bx:6
·    1.0µs  token two  words.bx:6
·    1.0µs  token three  words.bx:6
$ brix words.bx --probes log:parse
·      0ns  [parse.words] parsing 3 words  words.bx:3
$ brix words.bx --probes nothing
warning: --probes selector nothing matched no probe sites
```

- **No key.** With selectors, a log with no key needs `log:` or `log`. A statement probe with no key
  compiles into every build with `--probes`, whatever the selectors.
- **No match.** A selector that matches no probe is a warning, and the program still runs.
- **What a selector is.** Selectors are letters, digits, spaces and `. * : , - _`. A word after
  `--probes` with other characters is not taken as selectors: it goes to the program as its first
  argument, and the build compiles the probes it would compile with no selectors.
- **Metrics.** With `profile:`, the footer has a `metrics:` line in place of records, as
  `metrics: lib.calls 1 · lib.pass 1× 333ns`.

### What probes cost

A probe left out costs nothing. A probe compiled in costs the time to record it, and a build with
probes also checks, at a **probe safepoint** in each loop, whether the program should pass on what it
has recorded, unless every selector is a `profile:` one. A function written
`(without probe-safepoints)` holds none; see Functions. Compare times only between runs built with
the same probes.

## The console

`brix` runs the program, follows its trace as it grows, and prints each record to **stderr**. The
program's stdout stays its own. The console reads the trace every few milliseconds, so where its
lines fall among the program's own output means nothing.

- **A line** is `·`, the time since the run's first record, two spaces for each group the record is
  in, and the record. Logs and groups end with their file and line.
- **At most 400 lines** are printed. The footer then says how many were not shown; the trace has
  them all.
- **The footer** begins with a rule that gives the exit status, how long the program ran, and the
  number of records. Then come lines for what the run has: `counters:`, `metrics:`, `groups:` (each
  key's count, total time, and longest), `busiest:` (the sites that wrote two or more records),
  `records:` (by kind, and the time they span), `trace:` (the trace's path) and `query:` (the
  commands that read it). Each list shows eight entries and then `+N more`.
- **Warnings** in the footer: a group that never ended, a selector that matched nothing, and a
  program that wrote no trace because no probes were compiled in.

## The trace

### Where it is kept

Traces are kept in `.tmp/build/probes/` under the project's root, the nearest folder up that holds
`.git`. Each run leaves three files, named after the program and a stamp:

| File | Holds |
| --- | --- |
| `name-stamp.trace` | The records. |
| `name-stamp.probes.json` | The catalog: each probe's kind, key, text, field names, file and line. |
| `name-stamp.run.json` | The program, its arguments, its exit status and whether it timed out. |

`last.json` names the latest run, which `brix probes` reads by default. The 20 latest runs are
kept. A build with `-o build/app` and `--probes` also writes the catalog beside the executable, as
`build/app.probes.json`.

### What a record holds

- **Its site**: the probe that wrote it, and so its kind, key, text, file and line.
- **Its values**, each with its type.
- **Its time**: the clock time, in nanoseconds. Tools show it as the time since the run's first
  record.
- **Its owner**: the object it is of, or else the innermost group open in its task. Owners are how
  records nest.

A record does not hold its task or thread. The trace describes itself: the program declares each
probe at the start, so `brix probes` can read a trace with no catalog. Without the catalog, fields
are named `value1`, `value2`, and so on.

### How a run ends

- **The exit status** is the program's own. After `--timeout`, it is 124, and the footer's rule
  says `timed out after 300.0ms` in place of `exit 124`.
- **A panic** keeps its status, and the trace keeps what was recorded before it.
- **Signals.** A panic, an unhandled error, a request to end the process, `--timeout`, SIGTERM and
  SIGHUP all finish the trace. SIGKILL and Ctrl-C lose the counters and the last few milliseconds
  of records.

### Running a probed program yourself

A program built with probes runs as any other program when it is run directly, and records
nothing. Two environment variables turn recording on:

| Variable | Meaning |
| --- | --- |
| `PROBES_PATH` | The trace file. Without it, nothing is recorded. |
| `PROBES_ENABLED` | Recording starts when it is set and not empty. Its value is not read: which probes record is chosen when the program is built. |
| `PROBES_CONTROL_PATH` | A channel through which an inspector asks the running program about live objects. |

`brix app.bx --probes` and `brix probes run` set these themselves and clear any others starting
with `PROBES_`. Others tune the buffers: `PROBES_SEGMENT_SIZE`, `PROBES_SEGMENT_POOL`,
`PROBES_SEGMENT_TABLE_CAPACITY`, `PROBES_TASK_BUFFER_SIZE`, `PROBES_FLUSH_THRESHOLD` and
`PROBES_FLUSH_INTERVAL_MS`.

## Reading a trace

`brix probes COMMAND [TRACE]` reads a trace. `TRACE` is `last` by default, or a `.trace` file, or
a `.run.json` file.

| Command | Prints |
| --- | --- |
| `summary` | Records, groups by time, counters and the busiest sites. |
| `sites` | Each probe with its fields, location and number of records. |
| `log` | Records in order, filtered. |
| `around INDEX\|TIME` | Records around a record's index (`120`) or a time (`1.5s`). |
| `agg` | Statistics of a field, as `agg --field ms p50,p95,max --by frame`. |
| `durations` | Groups by key: count, total, mean, p50, p95 and longest. |
| `tree` | Nested groups as a tree, with total and own time. |
| `counters` | Counters and metrics. |
| `timeline` | Records over time, as bars. |
| `outliers` | The slowest groups, or the largest values of a field, against the median. |
| `diff A B` | Changes between two traces: records per site, group times and counters. |
| `run COMMAND…` | Runs a program built with probes and records its trace. |

Filters, for the commands that list records:

| Filter | Keeps |
| --- | --- |
| `--key K[,K]…` | Records whose key matches a glob, as `gui.*` or `*.frame`. A plain key also takes the keys inside it. |
| `--where EXPR` | Records whose fields compare as asked, joined by `&&`: `'ms > 4 && frame < 10'`. The operators are `==`, `!=`, `<`, `<=`, `>`, `>=` and `~`. `duration`, `time`, `depth`, `index`, `key`, `label`, `text` and `location` can be compared too. |
| `--text S` | Records whose text contains `S`. |
| `--source F[:L]` | Records written at a file, or a file and line. |
| `--since D`, `--until D` | Records in a span of time from the first record: `250us`, `100ms`, `1.5s`. |
| `--kind K[,K]…` | `log`, `event`, `group`, `begin`, `end`, `system`, or `open` for groups that never ended. |

`--limit N` and `--offset N` page through what a command prints; text is capped, and says how to
see more. `--format json` prints one JSON value and `--format jsonl` one JSON object per line.
`--catalog PATH` names the catalog when it is not beside the trace.

### An example

The outputs below read the trace of this program, saved as `frames.bx`:

```brix
(import /time/duration)

(main
  (for (in frame (..< 0 6))
    (probe-group + app.frame (text: "frame " frame))
    (let ms (+ 1 (% frame 3)))
    (probe-log _ (text: "paint frame " frame " took " ms))
    (probe-count app.paints)
    (call (. (as ms Milliseconds) wait))
    (probe-group - app.frame (text: "frame " frame " done"))
  )
  (print "done")
)
```

```output
done
```

`summary`:

```text
trace    …/.tmp/build/probes/frames-1794767200079.trace · exit 0
program  …/.tmp/build/probes/bin/frames
records  18 over 13.6ms: 6 logs, 6 groups

groups by total time
  group      count   total   mean    p50    p95    max  location
  app.frame      6  13.6ms  2.2ms  2.2ms  3.4ms  3.4ms  frames.bx:5

counters
  counter     value
  app.paints      6

busiest sites
  records  site                           fields     location
        6  paint frame {frame} took {ms}  frame, ms  frames.bx:7
```

`sites`, where a group's end with text is two sites, a log of its text and the end itself:

```text
id  kind     records  site                            fields     location
 1  begin          6  [app.frame] frame {frame}       frame      frames.bx:5
 2  log            6  paint frame {frame} took {ms}   frame, ms  frames.bx:7
 3  counter        1  app.paints                                 frames.bx:8
 4  log            6  [app.frame] frame {frame} done  frame      frames.bx:10
 5  end            6  app.frame                                  frames.bx:10
```

`log`, with each record's index:

```text
#0         0ns  + frame 0  frames.bx:5
#1         0ns    paint frame 0 took 1  frames.bx:7
#2       1.1ms  - frame 0 done 1.1ms  frames.bx:10
#3       1.1ms  + frame 1  frames.bx:5
#4       1.1ms    paint frame 1 took 2  frames.bx:7
#5       3.4ms  - frame 1 done 2.2ms  frames.bx:10
…
#17     13.6ms  - frame 5 done 3.4ms  frames.bx:10
```

Filters and `around`:

```text
$ brix probes log --text paint --where 'ms > 2'
#7       3.4ms    paint frame 2 took 3  frames.bx:7
#16     10.2ms    paint frame 5 took 3  frames.bx:7
$ brix probes log --source frames.bx:7 --limit 3
#1         0ns    paint frame 0 took 1  frames.bx:7
#4       1.1ms    paint frame 1 took 2  frames.bx:7
#7       3.4ms    paint frame 2 took 3  frames.bx:7
… +3 more records (--offset 3, --limit N, or --format jsonl for all)
$ brix probes around 7 --context 2
  #5       3.4ms  - frame 1 done 2.2ms  frames.bx:10
  #6       3.4ms  + frame 2  frames.bx:5
→ #7       3.4ms    paint frame 2 took 3  frames.bx:7
  #8       6.8ms  - frame 2 done 3.3ms  frames.bx:10
  #9       6.8ms  + frame 3  frames.bx:5
```

`agg` takes statistics of a field: `count`, `sum`, `min`, `max`, `mean`, `median` and `pNN`, as
words after the field or with `--stats`. `--by` splits them by another field:

```text
$ brix probes agg --field ms
field  count  min  mean  p50  p95  max
ms         6    1     2    2    3    3
$ brix probes agg --field ms p50,max,count --by frame
frame  p50  max  count
0        1    1      1
1        2    2      1
2        3    3      1
3        1    1      1
4        2    2      1
5        3    3      1
```

`durations`, `counters`, and `outliers`, which lists groups against the median with what happened
inside each:

```text
$ brix probes durations
group      count   total   mean    p50    p95    max  location
app.frame      6  13.6ms  2.2ms  2.2ms  3.4ms  3.4ms  frames.bx:5
$ brix probes counters
counter     value
app.paints      6
$ brix probes outliers
1.5× median (3.4ms vs 2.2ms)  app.frame  #15 at 10.2ms  frames.bx:5
    #16     10.2ms    paint frame 5 took 3  frames.bx:7
1.5× median (3.3ms vs 2.2ms)  app.frame  #6 at 3.4ms  frames.bx:5
    #7       3.4ms    paint frame 2 took 3  frames.bx:7
…
```

`timeline` draws records per stretch of time, or, with `--field`, a statistic of a field:

```text
$ brix probes timeline --bucket 2ms
each column is 2.0ms over 13.6ms; bars show records per column
                      records  peak
app.frame                   6     2  │█▅ █ ▅ │
paint frame … took …        6     2  │█▅ █ ▅ │
$ brix probes timeline --field ms --bucket 2ms
each column is 2.0ms over 13.6ms; bars show max of ms per column
                      records  peak
paint frame … took …        6     3  │▆█ ▆ █ │
```

`tree` nests groups, here for a program with a group `app.parse` inside a group `app.load`:

```text
total   self  count  group
3.4ms  1.1ms      1  app.load
2.2ms  2.2ms      1    app.parse
```

`diff` compares two runs. Here the same `frames.bx` ran again with 4 frames in place of 6:

```text
$ brix probes diff …/frames-1794767200079.trace last
a  …/.tmp/build/probes/frames-1794767200079.trace  13.6ms · 18 records
b  …/.tmp/build/probes/frames-1794767274433.trace  7.9ms · 12 records
             name                                   a      b  change
group p50    app.frame                          2.2ms  1.1ms  -49.1%
group total  app.frame                         13.6ms  7.9ms  -41.8%
counter      app.paints                             6      4  -33.3%
records      paint frame … took … frames.bx:7       6      4  -33.3%
records      app.frame frames.bx:5                  6      4  -33.3%
```

Sites are matched by file and line, so in a renamed file every site is new.

### Values the tools read

`--format jsonl` shows a record as the tools see it:

```text
{"label": "shop.order", "depth": 0, "key": "shop.order", "text": "shop.order: apple, 3", "fields": {"quantity": 3, "sku": "apple"}, "location": "orders.bx:5", "time_ns": 0, "index": 0, "kind": "event"}
```

A record of an object adds its owner, as `"owner": "List#1002"`. For the orders program in
[Events and property sets](#events-and-property-sets):

```text
$ brix probes agg --field quantity sum,count,max --by sku
sku    sum  count  max
apple    8      2    5
fig      2      1    2
pear     1      1    1
$ brix probes log --where 'quantity > 2'
#0         0ns  shop.order: apple, 3  orders.bx:5
#4         0ns  shop.order: apple, 5  orders.bx:5
```

### A program built once

`brix probes run` runs a program that was built with probes, as `brix` runs one with `--probes`:

```text
$ brix frames.bx -o build/frames --compile --probes
$ brix probes run --quiet --trace build/frames.trace -- build/frames
```

`--quiet` prints only the footer and `--trace` names the trace. The catalog is found beside the
executable. Run on a program built without probes, it warns that there is no catalog and that the
program wrote no trace.

## Probes and other features

### Tasks and threads

Each task has its own groups, so a record nests inside the group open in its own task. The
records of every task and thread go to one trace, in the order of their times. A record does not
say which task wrote it.

```brix
(import /time/duration)

(main
  (let a
    (do-new
      (probe-group + work.task (text: "task a"))
      (call (. 2ms wait))
      (probe-log _ (text: "a inside"))
      (probe-group - work.task (text: "a done"))
    )
  )
  (let b
    (do-new
      (probe-group + work.task (text: "task b"))
      (call (. 1ms wait))
      (probe-log _ (text: "b inside"))
      (probe-group - work.task (text: "b done"))
    )
  )
  (call (. a wait))
  (call (. b wait))
  (print "done")
)
```

```output
done
```

With `--probes`:

```text
·      0ns  + task a  tasks.bx:6
·    8.0µs  + task b  tasks.bx:14
·    1.1ms    b inside  tasks.bx:16
·    1.1ms  - b done 1.1ms  tasks.bx:17
·    2.1ms    a inside  tasks.bx:8
·    2.1ms  - a done 2.1ms  tasks.bx:9
── probes ── exit 0 · 204.5ms · 8 records
groups: work.task 2× 3.2ms (max 2.1ms)
records: 8 logs over 2.1ms
```

Task b's group began while a's was open, but it is not inside it. To find where a program hangs,
put groups around its waits and run it with `--timeout`: `brix probes log --kind open` lists the
groups that never ended.

### Tests and benchmarks

- **A case checks prints.** What a test compares is what the program prints, so a probe never
  changes a case's expected output, unless code under it prints. To see why a case fails, run its
  program with `--probes`.
- **`probes=selectors`** on a `.test.md` case's program fence compiles those probes into its
  program, and a bare `probes` compiles what `--probes` compiles with no selectors. The case runs alone, and its trace
  is removed with the case's directory.
- **Test modules** run by `brix test` have no probes, and `brix test` clears any `PROBES_`
  variables for the programs it runs.
- **Benchmarks.** A benchmark profile can name `probes` selectors to build with, and read counters
  as metrics.

### Tools

- **`brix app.bx --list-probes`** prints one JSON line for each probe in every module the program
  loads, the core library's included, with its key, file, kind and line, for a tool that lets
  someone choose keys:

  ```text
  {"key": "io.dispatch", "path": "…/stdlib/core/_io.yumi", "kind": "log", "line": 220}
  ```

- **An inspector** can attach to a running program through `PROBES_CONTROL_PATH`, follow its trace
  and ask it about live objects, such as the contents of a snapshot.
- **The compiler's own probes.** In the Brix repository, `bin/brix --compiler-probes [SELECTORS]`
  builds a copy of the compiler with those probes, runs it on the rest of the command line, and
  records its trace. Without selectors, it compiles the probes of the compiler's command line and
  every probe with no key. Its keys include `compiler.phase`, `typing.phase` and `emit.walks`.

## Probes and prints

A print is part of the program. A probe looks at it.

| | Print | Probe |
| --- | --- | --- |
| Compiled | Always. | Only with `--probes`, when a selector chooses it. |
| Runs | Always. | Only where it is compiled. |
| Writes to | stdout. | The trace, which `brix` shows on stderr. |
| Keeps | Text. | Each value with its type, the time, the file and line, its group or object. |
| Read by | Whoever runs the program, and its tests. | `brix probes`, which filters, adds up, times and compares. |

- **To see what a program does**, write a probe, not a print, and run it with `--probes`.
- **Output the program owes its user** is a print, and so is anything a test should check.
- **A probe and a print on one row.** In rows, a probe cannot follow a print on the same row:
  `-- // done`{.rows} is a syntax error.
