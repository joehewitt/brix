# Benchmark Suites

`docs/style/agent-yumi-benchmarks.md` says how to write a benchmark, run it, read what a run
measures and compare runs. This page covers what is particular to this repo's suites.

`.yumi/benchmarks.json` defines this repo's benchmark suites, profiles, metrics, and the commands
that compile and run them, and `bin/yumi bench` runs them. A folder can keep its suites in a
`.yumi/benchmarks.json` of its own, which joins this one. The configs list their suites, so
`yumi bench` does not look for `bench` folders here. The `runMs`, `runCpuMs`, `runInstructions` and
`runCycles` metrics and the `timing` and `gc` profiles are built into `yumi bench`, and the config
defines the rest. Each benchmark is a normal `.yumi` program in a suite directory. Every source file
starts with benchmark metadata using Yumi's backtick comment syntax:

```yumi
`benchmark.meta description: Exercises simple named stream captures over repeated rows.
`benchmark.meta timing: recommended
`benchmark.meta profiles: stream,compile-timing
```

Every benchmark declares `timing: recommended` or `timing: not-recommended`. The latter marks
benchmarks whose intended evidence is their declared counters or other statistics rather than a
time-based goal.

Benchmarks live beside the code they measure. Each home has a `bench` directory with one directory
per suite, such as `stdlib/core/bench/stream` or `stdlib/gui/bench/gui`. A suite directory holds
only benchmark files. Shared fixtures go in the home's `bench/fixtures`: standard-library cases
import them by module path, such as `/gui/bench/fixtures/scene-bench-fixture`, and other cases by
relative path. The release leaves `bench` directories out of the installed standard library.

Benchmarks may also declare an ordered, comma-separated `profiles` list. These are advisory
recommendations shown by `bin/yumi bench list`; they do not restrict other profiles or
exploratory runs.

The runner strips `benchmark.meta` comment lines before compiling the benchmark. Each benchmark
still prints one stable result line:

```text
bench-result suite/case checksum=12345
```

Benchmark programs must not time themselves and must not read changing repo files, use the network,
or depend on clocks. Put fixed inputs directly in the source or generate them from deterministic
templates.

The compiler benchmarks in `parser` intentionally read the `compiler/yumi-cli.yumi` import closure from
this worktree. Their corpus grows with the compiler. Treat them as practical watchpoints rather
than fixed-input benchmarks:

- `parser/compiler-source`, `parser/compiler-source-1`, and `parser/compiler-source-2`
  measure source loading, import discovery, parsing, syntax-tree finalization, and cleanup with
  eight, one, and two parse workers respectively. They stop at `source-to-syntax`: no C-reference
  collection, header cache or scanner access, generated C declarations, declaration building,
  linking, or IR lowering runs.
- `parser/compiler-pipeline` calls `compile-program` with eight parse workers. It includes
  parsing, C-header materialization, declaration building/linking, type checking, and diagnostic
  rendering/output for every module, including `not-supported` declines. Its checksum is the
  number of checked instances. It stops after type checking, so emission and native code
  generation are not part of it.

C-header materialization collects actual C references from the retained syntax nodes to select
declarations. It does not rescan source bytes, and names in comments or plain string text do not
request declarations.

For both workloads, `runMs` measures the benchmark's executable. `compileMs` measures the compiler
building that executable: the compiler's `compiler.phase` probe groups from parse through merge.
C-header cache state affects the compiler benchmark, and does not affect the parser benchmarks.

## Running Benchmarks

`bin/yumi bench list` shows the profiles and suites, and `bin/yumi bench list <selector>
--benchmarks` lists the benchmarks a selection holds. A selector is a suite (`stream`), one
benchmark in one suite (`stream/capture-spans-simple`), a benchmark name in every suite that has
it, or a benchmark source file. Run the selection with one profile:

```text
bin/yumi bench run stream --profile allocator
bin/yumi bench run stream/capture-spans-simple --profile compile-timing
```

Without `--profile`, the run uses the config's default, `compile-timing`. Use one run per profile
so timing, allocation, compiler metrics, and runtime counters remain separate experiments.
`--iterations` and `--warmup` apply to profiles with a timing metric.

Use focused cases while iterating. Before declaring broad performance work complete, run the full
relevant suite or explain why the focused workload is the whole contract.

## Profiles And Measurements

The default `compile-timing` profile reports compile time, compiler phase timing, and the
executable's run time, CPU time, instructions retired, and cycles, without runtime counters or
allocation sidecars.

Profiles in `.yumi/benchmarks.json` bundle selectors, probe selectors, compiler probe selectors, and
report columns. Metric entries define the column header, a one-line description, and how the value
is computed from compile or run samples using paths, sums, child-object sums, or ratios. Profiles
choose which metric keys are visible and whether compiler probes, benchmark runtime metrics, or
runtime probes are enabled.

Use the `allocator` profile for allocation count and allocated bytes, and `stream` for
stream-pattern counters.
Choose profiles because their measurements can evaluate the claim, not because they are broadly
available.

Time-based profiles such as `compile-timing` and `graphics-fps` use measured samples and warmups.
Non-timing profiles such as `allocator`, `stream`, and `graphics` collect their configured
counters without pretending they are timing experiments.

High dispersion matters. When multiple measured samples vary materially, report the range and
environmental caveats rather than presenting a favorable median as conclusive. A faster median is
not a win if variance, tail latency, allocation, correctness, or another protected metric regresses.

## Run Time

Every sample compiles a fresh executable, and macOS holds the first launch of a new executable
before its first instruction while it scans the file. That hold is usually 100-400 ms and
reaches seconds when other first launches are queued. `bin/yumi bench launch`, the suites' run
command, keeps it out of the measurement:

1. It launches the executable once and discards that launch's timing. This launch absorbs the scan
   and prints the benchmark's result line.
2. It launches the executable again and repeats until the measured launches total 250 ms or number
   100. A benchmark that runs for 250 ms or longer is measured once; a 4 ms benchmark is measured
   about sixty times.
3. It reports the median of each figure across the measured launches of that sample.

Each measured launch is timed from process creation to exit, and its CPU time, instructions
retired, and cycles are read from the exited process's own resource usage:

- `runMs` is wall time. It includes process creation, dynamic loading, runtime startup, the
  benchmark's work, and process teardown. It excludes the first-launch scan, the discarded launch,
  and the time spent in the runner and in `bin/yumi bench`.
- `runCpuMs` is user plus system CPU time, summed over the process's threads.
- `runInstructions` and `runCycles` count instructions retired and CPU cycles, including kernel
  work done on the process's behalf.

Per-frame and per-root ratios such as `graphicsFrameMs` and `temporaryRootNsPerPush` divide the
same wall time.

Choose the figure by how long the benchmark runs. `runInstructions` is the steadiest: repeated runs
on an unchanged tree agree within about 0.4%. `runMs` agrees within about 1.5% for cases that run
200 ms or longer and drifts up to 10% for cases that run 4-5 ms, and `runCycles` moves with it.
Read `runInstructions` for short cases, or give the benchmark enough work to run for hundreds of
milliseconds.

Only the first measured launch writes the benchmark metrics sidecar and the probe file, so counters
are never accumulated across launches. Any launch that fails fails the sample, and all launches of
one sample must finish within 57 seconds. The `gc-runtime` profile's launcher passes through the
run command unmeasured; its harness runs the same command on each GC variant, as described below.

## GC Runtime

The `gc-runtime` profile compiles each benchmark six times: no-GC bump allocation (A),
never-collect GC mechanics (B), and normal collection (C), each as a clean timing executable and as
an instrumented executable. `bin/yumi-bench` runs the variants in that order. For each
variant it runs `bin/yumi bench launch` on the timing executable with probes and benchmark metrics
off, then runs the instrumented executable once to write the metrics sidecar and probe file. The
three variants must print the same result line, and every variant's sidecar must validate.

Variant figures are the runner's figures for the timing executable, defined in Run Time above:

- `gcNoGcRuntimeMs`, `gcNeverCollectRuntimeMs` and `gcNormalRuntimeMs` are median wall times of
  warm launches. They include process creation, dynamic loading, runtime startup, the work, and
  teardown. They exclude the first-launch scan, the discarded launch, the instrumented run, any
  `sample` pass, and time spent in the harness, the runner, and `bin/yumi bench`.
- The `UserMs` and `SystemMs` columns are median CPU times summed over threads, and the
  `InvoluntaryContextSwitches`, `Instructions` and `Cycles` columns are medians of the same
  launches, all read from the exited process's resource usage at its native resolution.
- `gcTaxMechanical` (B/A) and `gcTaxTotal` (C/A) divide those median wall times, so process
  startup and teardown are in both terms and the ratio understates the tax of a very short
  benchmark. `gcTaxMechanicalInstructions` and `gcTaxTotalInstructions` divide median
  instructions retired; read them for short cases, as with `runInstructions`.

The instruction figures and instruction tax ratios repeat within a few percent under load. The
wall-time tax ratios can drift by around 10%, because a variant that runs 350-550 ms gets one
measured launch and contention from other work raises its wall time.

GC pause, CPU, memory, barrier and allocation figures come from the instrumented runs' sidecars,
the unprefixed ones from the normal variant, not from the timing launches. The `parser/compiler-source`
cases also run each timing executable once more under `sample` for the running and blocked
percentages.

All launches of one sample, across the three variants, must finish within the launcher's 57-second
deadline, and compiling the six executables must finish within 180 seconds.

## Baselines And Comparisons

Before performance-affecting edits, save a named baseline using the representative selectors and
smallest profile set that measures the claim. After edits, rerun the same selectors, profile,
workload, warmup, iteration policy, and material environment conditions, comparing against that
baseline:

```text
bin/yumi bench run stream/literals-and-runs --save before-stream-change --reason "baseline before stream change"
bin/yumi bench run stream/literals-and-runs --compare before-stream-change --save after-stream-change
```

Saved runs are kept in `.tmp/results/bench`, and `bin/yumi bench runs` lists them. Report selector
and profile equivalence, sampling policy, absolute values, deltas, expected thresholds, and noise.
A missing or mismatched baseline is missing evidence, not a neutral comparison.

## Allocation Metrics

Allocation count and allocated bytes come from the GC runtime counters. Resize operations are
counted at the Yumi runtime boundary. If a benchmark crashes or exits before the runtime writes its
metrics sidecar, allocation metrics are reported as `null`.

A profile with `benchmarkMetrics` builds its benchmarks against a separate benchmark runtime under
`.tmp/bench`, which the profile's `yumi-bench-runtime` setup builds with the same relative
filenames as the normal runtime build, and which the compiler links when `YUMI_BENCH_RUNTIME=1`.
That runtime writes the metrics sidecar itself: when `YUMI_BENCH_METRICS_PATH` names a file, it
starts counting as the program starts and writes the file when the program exits. Other profiles
use the normal build in `.tmp/build` and do not include allocation hooks.

The compiler records neither write-barrier calls nor allocation sites, so `gcBarrierCalls` and
`gcBarrierSlowEntryPct` read 0 and the sidecar lists no allocation sites. `gcBarrierSlowEntries`
comes from the runtime and is exact.

## Adding A Benchmark

1. Add a `.yumi` file to an existing suite directory, or add a new suite directory to the `bench`
   directory beside the code it measures and declare it in the nearest `.yumi/benchmarks.json`.
2. Add `` `benchmark.meta description: ...``, a `timing` classification, and an advisory
   `` `benchmark.meta profiles: ...`` list near the top of the source.
3. Keep the program deterministic, self-contained, and safe to launch repeatedly; every sample
   launches it more than once.
4. Print exactly one `bench-result suite/case checksum=N` line, where `suite/case` matches the suite
   directory and filename without `.yumi`.
5. Keep the initial scale modest so ordinary smoke evidence remains comfortable.
6. Confirm the new case with `bin/yumi bench list <suite> --benchmarks`, then run its focused
   selector.

Adding a benchmark to an existing suite does not require editing code. Profiles and metric
definitions are part of the `.yumi/benchmarks.json` files.
