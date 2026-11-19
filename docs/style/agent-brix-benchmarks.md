# Brix Benchmarks For Agents

A benchmark is a Brix program that does a fixed amount of work, and `brix bench` builds it, launches
it until its timing settles, and reports what the launches measured. Use benchmarks for any claim
that code got faster or uses less, and probes (`agent-brix-probes.md`) to explain why.

## Writing A Benchmark

Put each benchmark in a suite folder inside a `bench` folder beside the code it measures, such as
`parser/bench/tokens/split-words.bx`. Each folder inside a `bench` folder is a suite named after
it, so `tokens` here, except `bench/fixtures`, which holds code and data the benchmarks share. Two
suites cannot have the same name.

```brix
; benchmark.meta description: Splits a fixed sentence into words.
; benchmark.meta timing: recommended
; benchmark.meta profiles: timing

(main
  (let text (* "the quick brown fox jumps over the lazy dog " 200))
  (let total 0)
  (for (in _ (..< 0 200))
    (for (in word (call (. text split) " "))
      (+= total (. word length))
    )
  )
  (print (str "bench-result tokens/split-words checksum=" total))
)
```

- **The header.** `benchmark.meta description` is required. `timing` is `recommended`, or
  `not-recommended` when the benchmark's evidence is its counters rather than its time. `profiles`
  lists the profiles that suit it, as advice to whoever runs it.
- **The result line.** Print exactly one line `bench-result <suite>/<name> checksum=<digits>`, where
  the checksum is a whole number computed from the work, so the work cannot be optimized away and
  a change that alters the result shows.
- **Fixed work.** Do not time the work yourself, read files that change, use the network or depend
  on clocks. Build inputs in the source, or from fixed templates.
- **Scale.** Each sample launches the program many times, so start small. A benchmark that runs for
  hundreds of milliseconds gives steadier times than one that runs for a few.

## Running Benchmarks

`brix bench` finds benchmarks under the project root, which is the nearest folder at or above the
current one that holds `.git`, or else a `.brix` folder, or else the current folder. `--root`
names another.

```text
brix bench list
brix bench list tokens --benchmarks
brix bench run tokens/split-words
brix bench run tokens --profile gc
```

A selector is a suite (`tokens`), one benchmark of a suite (`tokens/split-words`), a benchmark name
in every suite that has it, or a benchmark's source file. With no selector, a run takes the
profile's `selectors`, or every benchmark when it has none.

A profile chooses what a run measures, and a run uses one profile:

- `timing`, the default, reports run time, CPU time, instructions and cycles.
- `gc` builds the benchmarks with the collector's probes and reports run time with the collections,
  pauses and assist time those probes count.

`--iterations` and `--warmup` set the measured and discarded samples of a profile with a timing
metric, and `--metrics all` shows every metric the profile has.

## What A Run Measures

Each sample builds the benchmark, launches it once to discard that launch, then launches it again
until the measured launches total 250 ms or number 100, and reports the median of each figure:

- `runMs` is wall time from process creation to exit, so it includes startup and teardown.
- `runCpuMs` is user plus system CPU time, summed over threads.
- `runInstructions` and `runCycles` count instructions retired and cycles.

`runInstructions` is the steadiest figure: repeated runs agree within about half a percent, while
the run time of a benchmark that runs for a few milliseconds can drift by 10%. Read instructions for
short benchmarks, or give them more work. When samples vary, report the range rather than a
favorable median.

## Comparing Runs

Save a baseline before a change, then rerun the same selectors and profile and compare:

```text
brix bench run tokens --save before
brix bench run tokens --compare before
```

Each figure then shows its change from the baseline. Saved runs are kept in `.tmp/results/bench`,
and `brix bench runs` lists them. A faster median is not a win if another figure, the checksum or
the program's behavior gets worse.

## Measuring Your Own Counters

A profile can report a program's own probe counters. Count the work with a probe in the code, such
as `//tokens.words ++`, then define a metric and a profile in `.brix/benchmarks.json` at the project
root:

```json
{
    "metrics": [
        {
            "id": "words",
            "title": "Words",
            "description": "Words the benchmark split.",
            "format": "count",
            "source": "run",
            "value": "/metrics/probeCounters/tokens.words"
        }
    ],
    "profiles": [
        {
            "id": "tokens",
            "description": "Run time and words split.",
            "probes": ["tokens"],
            "metrics": ["runMs", "words"]
        }
    ]
}
```

Then run `brix bench run --profile tokens`. A profile's `probes` are the probe selectors the
benchmarks are built and launched with, and its `metrics` can name the built-in metrics too. Probes
take time to record, so compare runs only within one profile.

## The Config

`.brix/benchmarks.json` is optional, and adds to what `brix bench` has built in. Every key is
optional:

- `suites` lists the suites in place of the ones found in `bench` folders. Each has an `id`, a
  `path` from the project root, and optionally a `description` and `extensions`, which is
  `[".bx"]` by default.
- `metrics` adds metrics. Each has an `id`, `title`, `description`, `format` (`duration` in
  milliseconds, `count`, `bytes`, `percent` or `rate`), `source` (`run`) and a `value`, which is a
  JSON pointer into what a launch measured. `sum` adds several pointers instead, and `ratio`
  divides a `numerator` by a `denominator` and multiplies by `scale`.
- `profiles` adds profiles. Each has an `id`, `description` and `metrics`, and optionally `probes`,
  `selectors` (what a run takes when given none) and `default`, which makes it the default profile.
- `env` sets environment variables for every build and launch.
- `brix` is the command, as an array, that builds and launches benchmarks. It is `["brix"]` by
  default, the `brix` on `PATH`.

The built-in metrics are `runMs`, `runCpuMs`, `runInstructions`, `runCycles`, `collectorCycles`,
`collectorPauses` and `collectorAssistNanos`, and the built-in profiles are `timing` and `gc`. A
config cannot define them again.

A folder under the project root can hold a `.brix/benchmarks.json` of its own, so an app keeps its
benchmarks beside its code. Its suites, metrics, profiles and commands join the root's, and each
name is defined once across all of them. A suite's `path`, and a command's `./` paths, `${root}`
and `${build}`, start from the folder of the config that defines them, and a nested config's `env`
applies to the commands it defines. A profile with no `description` adds its `metrics` to the
profile of that `id` that another config defines. Suites listed in any config replace the ones
found in `bench` folders.
