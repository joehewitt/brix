# Yumi Probes For Agents

Probes are Yumi's instrumentation. A probe is a line that records what the program did, and
`--probes` compiles probes in, runs the program, and keeps its trace to query afterward. Reach for
probes before `--` prints, before timers written by hand, and whenever a test or benchmark total
cannot explain what happened. Probe output goes to stderr and to the trace, so stdout stays the
program's own.

## Writing Probes

| Write | Records |
| --- | --- |
| `// paint frame $frame took $ms` | A log. Each `$value` keeps its name (`frame`, `ms`) for queries. |
| `//gui.paint// painted $count` | A log under a key, which selectors and `--key` can pick. |
| `//+gui.frame// frame $n` … `//-gui.frame// done` | A group: its begin, its end and the time between. Groups nest. |
| `//+ setup $name` … `//- setup done` | A group with no key. |
| `//+parse.pass` … `//-parse.pass` | A group with no text, kept only for its time. |
| `//parse.tokens ++`, `//parse.bytes += size` | A counter. |
| `//net.request -> url, status` | An event with values. |
| `//status = value`, `//request:status = value` | A property set, of the object before `:`. |
| `// queued $!item` | A log with a snapshot of `item`. `yumi probes` counts snapshots but does not show them. |
| `//!parse.debug -- dump(tree)` | A statement that runs only where the probe compiles. |

Code inside a probe, and code indented under one, runs only when that probe is compiled in. Never
put work the program needs in a probe. Write probes where they explain the work, not gathered in
one place.

A probe with no key compiles into every `--probes` run of the program, so remove the ones you added
only to look before you hand off. Keep a probe only where it explains work someone will want to see
again, and give it a key under the part it explains. A keyed probe outside the file being run
compiles in only when a selector names it, so it costs nothing otherwise.

## Running With Probes

```text
yumi x.yumi [-o build/x] --probes [selectors] [--timeout 30s] [--quiet-probes] [-- args]
```

- **Without `-o`**, `--probes` builds the executable at `.tmp/build/probes/bin/x` and runs it.
  `--compile` builds without running and needs `-o`.
- **Without selectors**, `--probes` compiles every probe in the files named on the command line and
  every probe with no key anywhere. Selectors replace that default.
- **Selectors** name keys and the namespaces inside them:
  - `gui.frame` and `gui.frame.*` select that key and every key inside it;
  - `log:` selects logs with no key, and `log:net` selects keyed logs;
  - `counter:*` selects every counter, and `gc` the collector's own counters;
  - `profile:parse` keeps the counters and groups without text under `parse` as metrics, with no
    records, and `profile:*` does this for every one, the standard library's included;
  - `*` selects everything.

  Separate several selectors with commas or spaces, quoted as one argument: `--probes 'gc app'`.
  `--probes` takes only the argument after it, so put the program's own arguments after `--`. A
  selector that matches no probe gets a warning.
- **While it runs**, the program keeps its terminal. Each record prints to stderr with its time, a
  log with its `file:line`, and groups nest with their durations. `--quiet-probes` keeps long runs
  quiet.
- **At exit**, a footer starting `── probes ──` gives the exit status, the counters, groups by total
  time, the busiest sites, warnings and the trace path. The exit status is the program's own, or 124
  after `--timeout`.

Traces are kept in `.tmp/build/probes/` under the project root, which is the nearest folder at or
above the current one that holds `.git`, or else a `.yumi` folder, or else the current folder. The
20 most recent stay, and the latest run is always `last`.

### The Probe Runner

`yumi probes run` runs an executable that is already built and records its trace:

```text
yumi probes run [--timeout 30s] [--quiet] [--trace path.trace] -- build/x args
```

The executable must have been built with probes, as `yumi x.yumi -o build/x --compile --probes`
builds it. Each build with `-o build/x` also writes the catalog `build/x.probes.json`, which names
each site's fields. Use the runner for a program you build once and run many times, for an app the
user runs, and for a run whose trace should go somewhere other than `.tmp/build/probes/`.

A probed executable contains the string `PROBES_PATH`, so `strings build/x | grep PROBES_PATH` tells
you whether it has probes. When the runner warns that there is no catalog, or the footer says the
program wrote no trace, the executable has no probes compiled in: rebuild it.

## Querying A Trace

`yumi probes <command> [trace]` reads `last` unless given a `.trace` file. Text output is capped,
and it says how to page with `--offset`. `--format jsonl` gives every record as JSON.

| Command | Answers |
| --- | --- |
| `summary` | What ran: records, groups by time, counters, busiest sites and their fields. |
| `sites` | Which probes exist, with fields such as `paint frame {frame} took {ms}`. |
| `log` | What happened, in order. |
| `around 120`, `around 1.5s` | What happened just before and after a record or a moment. |
| `agg --field ms p50,p95,max --by frame` | Statistics of a field, optionally per value of another. |
| `durations` | Count, total, mean, p50, p95 and max per group. |
| `tree` | Total and self time per nested group path. |
| `counters` | Counter values and profile metrics. |
| `timeline [--bucket 10ms] [--field ms]` | When records happened, as sparklines. |
| `outliers [--field ms]` | The slowest groups, or largest values, against their median. |
| `diff old.trace last` | What changed between two runs. |

Filters work with every command that reads records:

- `--key gui.*`;
- `--where 'ms > 4 && frame < 10'`, over fields and `duration`, `time`, `depth`, `index`, `key`,
  `label`, `text` and `location`, with durations written as `5ms`;
- `--text`;
- `--source file.yumi:12`;
- `--since 100ms` and `--until 2s`;
- `--kind log,event,group,open`, where `open` is a group that never ended.

## Debugging

**Seeing values.** Write `// state $state at $index` where you would have written `--`, then run
with `--probes`. The lines carry times and locations, and `yumi probes log --text state` finds them
again after the run.

**Why a check fails.** Put the failing code in a program of its own, add probes where its behavior
is in doubt, and run it with `--probes`. Read `log`, then `around` the first record that looks
wrong.

**A hang.** Put groups around the waits that might not return and run with `--timeout 10s`.
`yumi probes log --kind open` lists the groups that never ended, which is where the program was
stuck, and `yumi probes around <index>` shows what led up to it.

**A bug only the user can reproduce.** Build the program with the probes you need into a folder of
its own, so another build cannot replace it:

```text
yumi app.yumi -o .tmp/build/app-probes/app --compile --probes 'app.sync app.save'
```

For an app with a bundle, this also builds the bundle and the catalog `app.probes.json` beside the
executable. Check that the build has probes, then give the user a command that runs the bundle's
executable under the probe runner, with the trace beside the catalog:

```text
yumi probes run --quiet --trace .tmp/build/app-probes/app.trace -- .tmp/build/app-probes/App.app/Contents/MacOS/App
```

They reproduce the bug and quit the app. Read the trace like any other, as in
`yumi probes summary .tmp/build/app-probes/app.trace`.

## Profiling

**Where the time goes.** Put a group around one unit of work and log the values that vary:

```yumi
/time/duration

for frame in 0 .. < 20
    //+app.frame// frame $frame
    . ms = 1 + frame % 3
    // paint frame $frame took $ms
    (ms as Milliseconds).wait()
    //-app.frame// frame $frame done
```

Then:

- `yumi probes durations` gives the spread;
- `yumi probes outliers` lists the slowest frames with what happened inside them;
- `yumi probes agg --field ms --by frame` breaks a value down per frame;
- `yumi probes tree` shows where nested time goes;
- `yumi probes timeline` shows when the slow ones happened.

**A hot loop.** Count the operations inside it with `//parse.tokens ++` and
`//parse.bytes += size`, time each pass with `//+parse.pass` and `//-parse.pass`, and read
`yumi probes counters`. When the loop is too hot for a record per pass, run with
`--probes profile:parse`. The counters and the pass time are then kept as metrics, such as
`parse.pass 20,000× 3.4ms`, and no records are written.

**Collector pressure.** Count the operation and select the collector's counters too, as in
`--probes 'gc app'`. Then compare `gc.cycles`, `gc.pauses` and `gc.assist.nanos` with your counter in
`yumi probes counters`. The collector runs rarely, so a small run can show no cycles.

**Before and after a change.** Keep the trace path from the first run's footer, change the code,
rerun with the same probes, then run `yumi probes diff <first>.trace last`.

Probes take time to record, so compare only runs that share the same probes. A trace explains one
run; a claim that code got faster needs a benchmark, which
[the benchmarks guide](agent-yumi-benchmarks.md) describes.

## What A Trace Keeps

A panic, an unhandled error, `request-process-termination`, `--timeout`, and SIGTERM or SIGHUP all
flush and close the trace, unless the program handles that signal itself. A run killed by SIGKILL,
or by SIGINT from Ctrl-C, loses its counters and its last few milliseconds of records.
