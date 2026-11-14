import json
import math
import os
import re
import secrets
import shlex
import shutil
import signal
import subprocess
import sys
import tempfile
import time

from bench import sample
from common import ROOT, TMP, Failure, file_digest, say

SELECTOR = 'bench-gc-runtime'
HARNESS = 'yumi-gc-runtime-v3'
RUNTIME_SETUP = 'yumi-bench-runtime-v8'
SAMPLED_PREFIX = 'parser/compiler-source'
PROCESS_METRICS = (
    'wallMs',
    'cpuMs',
    'userMs',
    'systemMs',
    'instructions',
    'cycles',
    'involuntaryContextSwitches',
    'launches',
)
COMPILE_SECONDS = 180
RUN_SECONDS = 57
GRACE_SECONDS = 1.5
POLL_SECONDS = 0.1
SAMPLE_SECONDS = 50
OUTPUT_LIMIT = 1 << 20
TAIL_LIMIT = 4096
DEADLINE_EXIT_CODE = 124
CLEARED_PREFIXES = (
    'YUMI_',
    'YUMITEST_',
    'PROBES_',
    'ASAN_',
    'TSAN_',
    'UBSAN_',
    'LSAN_',
    'MSAN_',
    'LLVM_PROFILE',
    'Malloc',
)
CLEARED_NAMES = (
    'DYLD_INSERT_LIBRARIES',
    'DYLD_LIBRARY_PATH',
    'DYLD_FRAMEWORK_PATH',
    'LD_PRELOAD',
    'LD_LIBRARY_PATH',
)
SETUP_KEYS = (
    'YUMIPATH',
    'YUMI_BUILD_PATH',
    'YUMI_BENCH_RUNTIME',
    'YUMI_BENCH_METRICS',
    'YUMI_GC_MODE',
    'PROBES_ENABLED',
    'PROBES_PATH',
    'PROBES_CONTROL_PATH',
)
RUNTIME_ARCHIVES = (
    'yumi/libs/core/libyumicore.a',
    'yumi/libs/gc/libyumigc.a',
    'yumi/libs/events/libyumievents.a',
    'yumi/libs/graphics/libyumigraphics.a',
    'yumi/libs/applegui/libyumiapplegui.a',
)
GC_ARCHIVE = 'yumi/libs/gc/libyumigc.a'
SUPPORT_FILES = (
    '.yumi/benchmarks.json',
    'compiler/bench/bench-launch.yumi',
    'libs/build.json',
    'toolchain/bench/compile.py',
    'toolchain/bench/gc_runtime.py',
    'toolchain/bench/runtimes.py',
    'toolchain/bench/sample.py',
    'toolchain/runtime.py',
)
KINDS = ('timing', 'instrumented')
RUNNER = [os.path.join(ROOT, 'bin', 'yumi'), 'bench', 'launch']
LAUNCHER = os.path.join(ROOT, 'bin', 'yumi-bench')
EVIDENCE = os.path.join(TMP, 'bench', 'gc-runtime-evidence')
DIAGNOSTIC_NAME = 'gc-runtime-last-diagnostic.json'
OUTPUT_FLAGS = ('-o', '--o')
ARTIFACT_PLACEHOLDER = '<variant-artifact>'
SOURCE_IDENTITY = re.compile(r'bench-result\s+([^\s"\']+)\s+checksum=')
RESULT_LINE = re.compile(r'^bench-result\s+(\S+)\s+checksum=(\d+)\s*$')
LINE_BREAK = re.compile(r'\r?\n')
SELECTION_SEPARATOR = re.compile(r'[\s,]+')
UNSAFE_NAME = re.compile(r'[^A-Za-z0-9._-]+')


class Variant:
    def __init__(self, key, mode, runtime_mode_id, definitions):
        self.key = key
        self.mode = mode
        self.runtime_mode_id = runtime_mode_id
        self.definitions = definitions


VARIANTS = (
    Variant('noGc', 'no-gc', 0, ['YU_ENABLE_YUMI_GC=0', 'YU_GC_NEVER_COLLECT=0']),
    Variant('neverCollect', 'never-collect', 1, ['YU_ENABLE_YUMI_GC=1', 'YU_GC_NEVER_COLLECT=1']),
    Variant('normal', 'normal', 2, ['YU_ENABLE_YUMI_GC=1', 'YU_GC_NEVER_COLLECT=0']),
)


class Outcome:
    def __init__(self, code=None, signum=None, stdout='', stderr='', timed_out=False, truncated=False):
        self.code = code
        self.signum = signum
        self.stdout = stdout
        self.stderr = stderr
        self.timed_out = timed_out
        self.truncated = truncated

    def passed(self):
        return self.code == 0 and not self.signum and not self.timed_out and not self.truncated

    def label(self):
        if self.timed_out:
            return ' timeout'
        if self.truncated:
            return ' output-limit'
        if self.signum:
            return ' signal=%s' % signal_name(self.signum)
        return ' exit=%s' % ('unknown' if self.code is None else self.code)

    def record(self):
        return {
            'exitCode': self.code,
            'signal': signal_name(self.signum) if self.signum else None,
            'stdout': self.stdout,
            'stderr': self.stderr,
            'timedOut': self.timed_out,
            'outputTruncated': self.truncated,
        }


class Started:
    def __init__(self, process, stdout, stderr):
        self.process = process
        self.stdout = stdout
        self.stderr = stderr


class Harness:
    def __init__(self, seconds):
        self.deadline = time.monotonic() + seconds
        self.children = set()
        self.stopping = 0
        self.expired = False
        self.context = {'phase': 'startup'}
        for signum in (signal.SIGINT, signal.SIGTERM):
            signal.signal(signum, self.interrupt)

    def interrupt(self, signum, frame):
        if self.stopping:
            return
        self.stopping = signum
        if not self.children:
            exit_by_signal(signum)
        for child in list(self.children):
            signal_group(child, signum)

    def start(self, command, environment):
        stdout = tempfile.TemporaryFile()
        stderr = tempfile.TemporaryFile()
        try:
            process = subprocess.Popen(command, cwd=ROOT, env=environment, stdin=subprocess.DEVNULL,
                                       stdout=stdout, stderr=stderr, start_new_session=True)
        except OSError as error:
            stdout.close()
            stderr.close()
            raise Failure('%s did not start: %s' % (command[0], error.strerror))
        self.children.add(process)
        return Started(process, stdout, stderr)

    def execute_all(self, commands):
        if self.deadline - time.monotonic() <= GRACE_SECONDS:
            self.expired = True
            return [Outcome(stderr='gc-runtime harness deadline expired before starting child', timed_out=True)
                    for _ in commands]
        started = [self.start(command, environment) for command, environment in commands]
        return [self.finish(child) for child in started]

    def execute(self, command, environment):
        return self.execute_all([(command, environment)])[0]

    def finish(self, child):
        timed_out = self.wait(child.process)
        self.children.discard(child.process)
        if self.stopping:
            exit_by_signal(self.stopping)
        stdout, stdout_truncated = read_tail(child.stdout)
        stderr, stderr_truncated = read_tail(child.stderr)
        code = child.process.returncode
        return Outcome(code if code >= 0 else None, -code if code < 0 else None, stdout, stderr,
                       timed_out, stdout_truncated or stderr_truncated)

    def wait(self, process):
        timed_out = False
        kill_at = None
        while True:
            try:
                process.wait(timeout=POLL_SECONDS)
                return timed_out
            except subprocess.TimeoutExpired:
                pass
            now = time.monotonic()
            if kill_at is None:
                if self.stopping:
                    kill_at = now + GRACE_SECONDS
                elif now >= self.deadline - GRACE_SECONDS:
                    timed_out = True
                    self.expired = True
                    signal_group(process, signal.SIGINT)
                    kill_at = now + GRACE_SECONDS
            elif now >= kill_at:
                signal_group(process, signal.SIGKILL)
                kill_at = math.inf

    def fail(self, message, outcome=None):
        if outcome:
            write_through(outcome)
        self.diagnose({
            'status': 'failed',
            'context': self.context,
            'message': message,
            'exitCode': outcome.code if outcome else None,
            'signal': signal_name(outcome.signum) if outcome and outcome.signum else None,
            'stdoutTail': outcome.stdout[-TAIL_LIMIT:] if outcome else '',
            'stderrTail': outcome.stderr[-TAIL_LIMIT:] if outcome else '',
            'preservedExecutable': self.preserve_executable(),
        })
        raise Failure(message)

    def diagnose(self, diagnostic):
        build = os.environ.get('YUMI_BUILD_PATH', '')
        if not build:
            return
        try:
            path = os.path.join(os.path.abspath(build), DIAGNOSTIC_NAME)
            os.makedirs(os.path.dirname(path), exist_ok=True)
            write_json(path, dict({'schemaVersion': 1, 'harness': HARNESS}, **diagnostic))
        except OSError:
            pass

    def preserve_executable(self):
        source = self.context.get('executablePath', '')
        build = os.environ.get('YUMI_BUILD_PATH', '')
        mode = self.context.get('variant', '')
        if not source or not build or not mode:
            return None
        try:
            destination = os.path.join(os.path.abspath(build), 'gc-runtime-failure-%s' % mode)
            shutil.copyfile(source, destination)
            os.chmod(destination, 0o755)
            debug = None
            if os.path.exists(source + '.dSYM'):
                shutil.copytree(source + '.dSYM', destination + '.dSYM', dirs_exist_ok=True)
                debug = destination + '.dSYM'
            return {'path': destination, 'sha256': file_digest(destination), 'debugPath': debug}
        except OSError:
            return None


def selected():
    return SELECTOR in SELECTION_SEPARATOR.split(os.environ.get('PROBES_ENABLED', ''))


def compile_variants(arguments, compiler):
    harness = Harness(COMPILE_SECONDS)
    artifact = output_path(arguments)
    if not artifact:
        raise Failure('gc-runtime compile requires an output artifact path')
    source = source_path(arguments)
    if not source:
        raise Failure('gc-runtime compile requires a benchmark source path')
    source_digest = identity_digest(source)
    with open(source) as file:
        identity = benchmark_identity(file.read())
    if not identity:
        raise Failure('gc-runtime source must print one literal bench-result benchmark identity')
    manifest = {
        'schemaVersion': 2,
        'harness': HARNESS,
        'benchmarkId': identity,
        'sourceSha256': source_digest,
        'environmentPolicy': {
            'clearedPrefixes': list(CLEARED_PREFIXES),
            'clearedNames': list(CLEARED_NAMES),
            'clearedAmbientKeys': sorted(name for name in os.environ if cleared(name)),
            'runtimeArchiveSetup': {
                'harness': RUNTIME_SETUP,
                'policy': 'sanitized-environment-with-explicit-build-overlays',
                'explicitKeys': list(SETUP_KEYS),
                'probesDisabled': True,
            },
        },
        'compilerArgs': normalized_arguments(arguments, artifact),
        'producerSupportFiles': [{'path': path, 'sha256': identity_digest(os.path.join(ROOT, path))}
                                 for path in SUPPORT_FILES],
        'variants': {},
    }
    jobs = []
    for variant in VARIANTS:
        manifest['variants'][variant.key] = {
            'mode': variant.mode,
            'definitions': variant.definitions,
            'timing': None,
            'instrumented': None,
        }
        jobs += [(variant, kind) for kind in KINDS]
    build = os.environ.get('YUMI_BUILD_PATH', '')
    probes = os.environ.get('PROBES_ENABLED', '')
    for index in range(0, len(jobs), 2):
        pair = jobs[index:index + 2]
        outcomes = harness.execute_all([compile_command(variant, kind, arguments, artifact, build, probes, compiler)
                                        for variant, kind in pair])
        for (variant, kind), outcome in zip(pair, outcomes):
            if not outcome.passed():
                harness.context = {'phase': '%s-compile' % kind, 'variant': variant.mode}
                harness.fail('%s %s compile failed%s' % (variant.mode, kind, outcome.label()), outcome)
            instrumented = kind == 'instrumented'
            manifest['variants'][variant.key][kind] = {
                'benchmarkMetrics': instrumented,
                'probes': probes if instrumented else '',
                'buildPath': runtime_build_path(build, kind, variant.mode),
                'archives': archive_identities(build, kind, variant.mode),
                'effectiveConfig': {
                    'mode': variant.mode,
                    'benchmarkRuntime': True,
                    'benchmarkMetrics': instrumented,
                    'yumiPath': os.path.join(ROOT, 'stdlib'),
                    'probesEnabled': probes if instrumented else '',
                    'sourceId': identity,
                },
                'executableSha256': identity_digest(executable_path(artifact, kind, variant.mode)),
            }
    write_json(artifact + '.gc-runtime.json', manifest)
    with open(artifact, 'w') as file:
        file.write('#!/bin/sh\nexec %s --run %s\n' % (shlex.quote(LAUNCHER), shlex.quote(artifact)))
    os.chmod(artifact, 0o755)


def compile_command(variant, kind, arguments, artifact, build, probes, compiler):
    timing = kind == 'timing'
    environment = variant_environment(variant.mode, {
        'YUMI_BUILD_PATH': os.path.join(build, 'clean') if timing else build,
        'YUMI_BENCH_RUNTIME': '1',
        'YUMI_BENCH_METRICS': '0' if timing else '1',
        'YUMI_BENCH_METRICS_PATH': '',
        'PROBES_ENABLED': '' if timing else probes,
        'PROBES_PATH': '',
    })
    command = [compiler, '--compile', '-o', executable_path(artifact, kind, variant.mode), source_path(arguments)]
    if not timing and probes:
        command.append('--probes=' + probes)
    return command, environment


def run_variants(arguments):
    harness = Harness(RUN_SECONDS)
    try:
        return measure_variants(harness, arguments)
    except Failure as failure:
        if not harness.expired:
            raise
        say(str(failure))
        return DEADLINE_EXIT_CODE


def measure_variants(harness, arguments):
    if len(arguments) != 1:
        harness.fail('gc-runtime run requires the compiled launcher path')
    artifact = os.path.abspath(arguments[0])
    outer_metrics = os.environ.get('YUMI_BENCH_METRICS_PATH', '')
    if not outer_metrics:
        harness.fail('gc-runtime run requires YUMI_BENCH_METRICS_PATH')
    manifest = read_json(artifact + '.gc-runtime.json')
    if not isinstance(manifest, dict) or manifest.get('schemaVersion') != 2 or manifest.get('harness') != HARNESS:
        harness.fail('gc-runtime manifest is not a supported six-artifact manifest')
    source = next((argument for argument in manifest['compilerArgs'] if argument.lower().endswith('.yumi')), '')
    if not source or digest(source) != manifest['sourceSha256']:
        harness.fail('gc-runtime source changed after the six variant artifacts were compiled')
    identity = manifest['benchmarkId']
    outer_probes = os.environ.get('PROBES_PATH', '')
    run_nonce = secrets.token_hex(16)
    runs = {}
    canonical_line = None
    canonical_checksum = None
    for variant in VARIANTS:
        built = manifest['variants'].get(variant.key) or {}
        timing_executable = executable_path(artifact, 'timing', variant.mode)
        instrumented_executable = executable_path(artifact, 'instrumented', variant.mode)
        if digest(timing_executable) != (built.get('timing') or {}).get('executableSha256'):
            harness.fail('%s timing executable changed after the manifest was written' % variant.mode)
        if digest(instrumented_executable) != (built.get('instrumented') or {}).get('executableSha256'):
            harness.fail('%s instrumented executable changed after the manifest was written' % variant.mode)
        metrics_nonce = '%s-%s' % (run_nonce, variant.mode)
        metrics_path = '%s.%s.json' % (outer_metrics, metrics_nonce)
        timing_environment = variant_environment(variant.mode, {
            'YUMI_BUILD_PATH': built['timing']['buildPath'],
            'YUMI_BENCH_RUNTIME': '1',
            'YUMI_BENCH_METRICS': '0',
            'YUMI_BENCH_METRICS_PATH': '',
            'PROBES_ENABLED': '',
            'PROBES_PATH': '',
        })
        harness.context = {'phase': 'timing-run', 'variant': variant.mode, 'executablePath': timing_executable}
        sample_path = ('%s.%s.sample.txt' % (outer_metrics, metrics_nonce)
                       if identity.startswith(SAMPLED_PREFIX) else '')
        timing, measured, attribution, observed = run_timing(harness, timing_executable, timing_environment,
                                                             sample_path)
        if not timing.passed():
            harness.fail('%s timing run failed%s' % (variant.mode, timing.label()), timing)
        if not measured:
            harness.fail('%s timing runner did not print one valid bench-metrics line' % variant.mode, timing)
        result = bench_result(timing.stdout)
        if not result:
            harness.fail('%s did not print one valid bench-result line' % variant.mode)
        observed_result = bench_result(observed.stdout) if observed else result
        if not observed_result or observed_result['line'] != result['line']:
            harness.fail('%s sample observation did not preserve the benchmark result' % variant.mode)
        if result['id'] != identity:
            harness.fail("%s printed benchmark identity '%s', expected '%s' from the compiled source"
                         % (variant.mode, result['id'], identity))
        if canonical_line is None:
            canonical_line = result['line']
            canonical_checksum = result['checksum']
        elif result['line'] != canonical_line or result['checksum'] != canonical_checksum:
            harness.fail("gc-runtime checksum mismatch: expected '%s', %s printed '%s'"
                         % (canonical_line, variant.mode, result['line']))

        if variant.mode == 'normal':
            probes_path = outer_probes
        else:
            probes_path = '%s.%s' % (outer_probes, variant.mode) if outer_probes else ''
        instrumented_environment = variant_environment(variant.mode, {
            'YUMI_BUILD_PATH': built['instrumented']['buildPath'],
            'YUMI_BENCH_RUNTIME': '1',
            'YUMI_BENCH_METRICS': '1',
            'YUMI_BENCH_METRICS_PATH': metrics_path,
            'YUMI_BENCH_RUN_NONCE': metrics_nonce,
            'PROBES_PATH': probes_path,
        })
        harness.context = {'phase': 'instrumented-run', 'variant': variant.mode,
                           'executablePath': instrumented_executable}
        instrumented = harness.execute([instrumented_executable], instrumented_environment)
        if not instrumented.passed():
            harness.fail('%s instrumented run failed%s' % (variant.mode, instrumented.label()), instrumented)
        instrumented_result = bench_result(instrumented.stdout)
        if not instrumented_result or instrumented_result['line'] != canonical_line:
            harness.fail("gc-runtime instrumented checksum mismatch: expected '%s', %s printed '%s'"
                         % (canonical_line, variant.mode,
                            instrumented_result['line'] if instrumented_result else '<invalid>'))
        metrics = read_json(metrics_path)
        if not valid_sidecar(metrics, variant, metrics_nonce, identity):
            harness.context = dict(harness.context, metricsSidecar=preserve_sidecar(metrics_path, variant.mode))
            harness.fail('%s did not produce a valid schema-v2 benchmark metrics sidecar' % variant.mode)
        runs[variant.key] = dict({'mode': variant.mode}, **measured)
        runs[variant.key].update({
            'sampleAttribution': attribution,
            'rssPeakBytes': metrics.get('rssPeakBytes'),
            'checksum': result['checksum'],
            'resultLine': result['line'],
            'build': built,
            'metrics': metrics,
        })

    merged = dict(runs['normal']['metrics'] or {})
    merged.update({
        'schemaVersion': 2,
        'source': 'gc-runtime-three-variant',
        'variant': 'normal',
        'checksum': canonical_checksum,
        'harnessRunNonce': run_nonce,
        'effectiveConfig': {
            'timingPass': 'clean-runtime-and-compiler',
            'observationPass': 'instrumented-runtime-and-compiler',
            'environmentPolicy': manifest['environmentPolicy'],
        },
        'benchmarkIdentity': {
            'id': identity,
            'sourceSha256': manifest['sourceSha256'],
            'compilerArgs': manifest['compilerArgs'],
        },
        'producerSupportFiles': manifest['producerSupportFiles'],
        'checksumEquivalent': 1,
        'checksumsEquivalent': True,
        'noGcRuntimeMs': runs['noGc']['wallMs'],
        'neverCollectRuntimeMs': runs['neverCollect']['wallMs'],
        'normalRuntimeMs': runs['normal']['wallMs'],
        'gcTaxMechanical': ratio(runs['neverCollect']['wallMs'], runs['noGc']['wallMs']),
        'gcTaxTotal': ratio(runs['normal']['wallMs'], runs['noGc']['wallMs']),
        'gcTaxMechanicalInstructions': ratio(runs['neverCollect']['instructions'], runs['noGc']['instructions']),
        'gcTaxTotalInstructions': ratio(runs['normal']['instructions'], runs['noGc']['instructions']),
        'rssPeakBytes': runs['normal']['rssPeakBytes'],
        'variants': runs,
    })
    merged['evidencePath'] = write_evidence(merged)
    write_json(outer_metrics, merged)
    harness.diagnose({'status': 'passed', 'context': {'phase': 'complete'}, 'checksum': canonical_checksum})
    print(canonical_line)


def run_timing(harness, executable, environment, sample_path):
    timing = harness.execute(RUNNER + [executable], environment)
    measured = process_metrics(timing.stdout) if timing.passed() else None
    if not measured or not sample_path:
        return timing, measured, None, None
    observed_child = harness.start([executable], environment)
    try:
        sampler_child = harness.start(['sample', str(observed_child.process.pid), str(SAMPLE_SECONDS), '-mayDie',
                                       '-f', sample_path], sanitized_environment())
    except Failure:
        signal_group(observed_child.process, signal.SIGKILL)
        harness.finish(observed_child)
        raise
    observed = harness.finish(observed_child)
    sampler = harness.finish(sampler_child)
    if not observed.passed():
        harness.diagnose({
            'status': 'failed',
            'context': dict(harness.context, phase='sample-observation', executablePath=executable),
            'observed': observed.record(),
        })
        lines = LINE_BREAK.split(observed.stderr)
        raise Failure('sample observation run failed%s:\n%s'
                      % (observed.label(), '\n'.join(lines[-10:] + ['... first lines ...'] + lines[:10])))
    try:
        with open(sample_path, errors='replace') as file:
            attribution = sample.analyze(file.read())
    except (OSError, Failure) as error:
        raise Failure('sample produced no usable attribution%s: %s (%s)'
                      % (sampler.label(), sampler.stderr.strip(), error))
    os.remove(sample_path)
    return timing, measured, attribution, observed


def process_metrics(stdout):
    lines = [line for line in LINE_BREAK.split(stdout) if line.startswith('bench-metrics ')]
    if len(lines) != 1:
        return None
    try:
        measured = json.loads(lines[0][len('bench-metrics '):])['process']
    except (ValueError, KeyError, TypeError):
        return None
    if not isinstance(measured, dict):
        return None
    if not all(finite(measured.get(name)) and measured[name] >= 0 for name in PROCESS_METRICS):
        return None
    if not integer(measured['launches']) or measured['launches'] <= 0:
        return None
    return {name: measured[name] for name in PROCESS_METRICS}


def bench_result(stdout):
    lines = [line for line in LINE_BREAK.split(stdout) if line.startswith('bench-result ')]
    if len(lines) != 1:
        return None
    match = RESULT_LINE.match(lines[0])
    if not match:
        return None
    return {'line': lines[0], 'id': match.group(1), 'checksum': match.group(2)}


def valid_sidecar(metrics, variant, nonce, identity):
    if not isinstance(metrics, dict):
        return False
    if not (metrics.get('schemaVersion') == 2
            and metrics.get('variant') == variant.mode
            and metrics.get('runNonce') == nonce
            and metrics.get('runtimeModeId') == variant.runtime_mode_id
            and isinstance(metrics.get('allocationSizeHistogramBytes'), list)
            and metrics.get('allocationSizeHistogramComplete') is True
            and metrics.get('allocationSizeClassHistogramComplete') is True
            and isinstance(metrics.get('gcPauseTrace'), list)
            and isinstance(metrics.get('gcCycleSummaries'), list)
            and number(metrics.get('gcPauseTraceTotalEvents'))
            and number(metrics.get('gcPauseTraceStoredEvents'))
            and number(metrics.get('gcPauseTraceDroppedEvents'))
            and number(metrics.get('rssPeakBytes'))
            and metrics.get('metricsInvariantValid') is True
            and zero(metrics.get('gcPauseTraceDroppedEvents'))
            and metrics.get('gcPauseSummaryComplete') is True
            and finite_nonnegative(metrics)):
        return False
    sites = (isinstance(metrics.get('allocationSiteRates'), list)
             and metrics.get('allocationSiteRatesComplete') is True
             and zero(metrics.get('allocationSiteHashCollisionCount'))
             and zero(metrics.get('allocationSiteTableOverflowCount'))
             and zero(metrics.get('allocationSiteNonAllocatingOperationCount')))
    lifetimes = (metrics.get('allocationLifetimeInvariantValid') is True
                 and isinstance(metrics.get('allocationLifetimeHistogramCycles'), list)
                 and isinstance(metrics.get('allocationLifetimeCensoredHistogramCycles'), list))
    collected_nothing = (zero(metrics.get('gcCycleCount'))
                         and not metrics['gcCycleSummaries']
                         and zero(metrics.get('gcPauseTraceTotalEvents')))
    if not mutator_phases_valid(metrics, identity) or not sites:
        return False
    if variant.mode == 'no-gc':
        return (metrics.get('source') == 'bump-public'
                and null(metrics, 'allocationSizeClassHistogramBytes')
                and metrics.get('allocationSizeClassHistogramBasis') == 'inapplicable-no-gc'
                and null(metrics, 'allocationLifetimeHistogramCycles')
                and collected_nothing)
    if variant.mode == 'never-collect':
        return collected_nothing and lifetimes and size_classes_valid(metrics)
    return (metrics.get('source') == 'gc'
            and size_classes_valid(metrics)
            and cycle_evidence_valid(metrics)
            and lifetimes)


def size_classes_valid(metrics):
    histogram = metrics.get('allocationSizeClassHistogramBytes')
    if not isinstance(histogram, list):
        return False
    total = 0
    for entry in histogram:
        count = entry.get('count') if isinstance(entry, dict) else None
        if not count:
            continue
        if not number(count):
            return False
        total += count
    allocations = metrics.get('allocationCount')
    return (metrics.get('source') == 'gc'
            and metrics.get('allocationSizeClassHistogramBasis') == 'allocatedBytes-v1-size-class'
            and number(allocations)
            and total == allocations
            and number(metrics.get('allocationSizeClassHistogramCount'))
            and metrics['allocationSizeClassHistogramCount'] == allocations)


def cycle_evidence_valid(metrics):
    cycles = metrics.get('gcCycleCount')
    summaries = metrics['gcCycleSummaries']
    trace = metrics['gcPauseTrace']
    stored = metrics.get('gcPauseSummaryStoredEvents')
    if not integer(cycles) or len(summaries) != cycles or not integer(stored):
        return False
    if cycles == 0:
        return (not trace
                and zero(metrics['gcPauseTraceTotalEvents'])
                and zero(metrics['gcPauseTraceStoredEvents'])
                and stored == 0)
    if stored <= 0:
        return False
    kinds = {}
    for event in trace:
        if not isinstance(event, dict) or not integer(event.get('cycleId')) or not integer(event.get('kind')):
            return False
        kinds.setdefault(event['cycleId'], set()).add(event['kind'])
    return all(isinstance(summary, dict)
               and integer(summary.get('cycleId'))
               and {1, 3, 4} <= kinds.get(summary['cycleId'], set())
               for summary in summaries)


def mutator_phases_valid(metrics, identity):
    phases = metrics.get('mutatorPhases')
    if not isinstance(phases, list) or metrics.get('mutatorPhasesComplete') is not True:
        return False
    scope = metrics.get('mutatorPhaseCounterScope')
    scope = scope if isinstance(scope, dict) else {}
    if (scope.get('barrierCalls') != 'process-global'
            or scope.get('barrierSlowEntries') != 'process-global'
            or scope.get('allocationRefills') != 'calling-mutator-published-TLS'
            or scope.get('assists') != 'calling-mutator-published-TLS'):
        return False
    if identity != 'allocation/store-intensive':
        return True
    names = [phase.get('name') if isinstance(phase, dict) else None for phase in phases]
    complete = [isinstance(phase, dict) and phase.get('complete') is True for phase in phases]
    return names == ['idle', 'mark-window'] and all(complete)


def finite_nonnegative(value):
    if isinstance(value, bool):
        return True
    if number(value):
        return math.isfinite(value) and value >= 0
    if isinstance(value, list):
        return all(finite_nonnegative(item) for item in value)
    if isinstance(value, dict):
        return all(finite_nonnegative(item) for item in value.values())
    return True


def number(value):
    return isinstance(value, (int, float)) and not isinstance(value, bool)


def finite(value):
    return number(value) and math.isfinite(value)


def integer(value):
    return finite(value) and float(value).is_integer()


def zero(value):
    return number(value) and value == 0


def null(values, name):
    return name in values and values[name] is None


def ratio(numerator, denominator):
    return round(numerator / denominator, 6) if denominator > 0 else None


def cleared(name):
    return name in CLEARED_NAMES or name.startswith(CLEARED_PREFIXES)


def sanitized_environment():
    return {name: value for name, value in os.environ.items() if not cleared(name)}


def variant_environment(mode, overlay):
    environment = sanitized_environment()
    environment.update({
        'YUMI_GC_MODE': mode,
        'YUMI_BUILD_PATH': os.environ.get('YUMI_BUILD_PATH', ''),
        'YUMIPATH': os.path.join(ROOT, 'stdlib'),
        'YUMI_BENCH_RUNTIME': '1',
        'PROBES_ENABLED': os.environ.get('PROBES_ENABLED', ''),
        'PROBES_CONTROL_PATH': '',
    })
    environment.update(overlay)
    return environment


def runtime_build_path(build, kind, mode):
    root = os.path.join(build, 'clean') if kind == 'timing' else build
    return os.path.join(root, 'bench') if mode == 'normal' else os.path.join(root, mode, 'bench')


def archive_identities(build, kind, mode):
    root = runtime_build_path(build, kind, mode)
    archives = [archive for archive in RUNTIME_ARCHIVES if mode != 'no-gc' or archive != GC_ARCHIVE]
    return [{'path': os.path.join(root, archive), 'sha256': identity_digest(os.path.join(root, archive))}
            for archive in archives]


def output_path(arguments):
    for index, argument in enumerate(arguments):
        if argument in OUTPUT_FLAGS:
            value = arguments[index + 1] if index + 1 < len(arguments) else ''
            return os.path.abspath(value) if value else ''
        flag, equals, value = argument.partition('=')
        if equals and flag in OUTPUT_FLAGS:
            return os.path.abspath(value) if value else ''
    return ''


def source_path(arguments):
    return next((argument for argument in arguments
                 if not argument.startswith('-') and argument.lower().endswith(('.yumi', '.bx'))), '')


def benchmark_identity(source):
    found = SOURCE_IDENTITY.findall(source)
    return found[0] if len(found) == 1 else ''


def replace_output(arguments, replacement):
    result = list(arguments)
    for index, argument in enumerate(result):
        if argument in OUTPUT_FLAGS:
            if index + 1 < len(result):
                result[index + 1] = replacement
            return result
        flag, equals, _ = argument.partition('=')
        if equals and flag in OUTPUT_FLAGS:
            result[index] = '%s=%s' % (flag, replacement)
            return result
    return result


def normalized_arguments(arguments, artifact):
    return [ARTIFACT_PLACEHOLDER if argument == artifact else argument
            for argument in replace_output(arguments, ARTIFACT_PLACEHOLDER)]


def executable_path(artifact, kind, mode):
    return '%s.%s.%s' % (artifact, kind, mode)


def write_evidence(metrics):
    directory = os.path.join(EVIDENCE, UNSAFE_NAME.sub('--', metrics['benchmarkIdentity']['id']))
    os.makedirs(directory, exist_ok=True)
    path = os.path.join(directory, '%s.json' % metrics['harnessRunNonce'])
    write_json(path, {
        'schemaVersion': 1,
        'source': 'gc-runtime-evidence',
        'generatedAt': time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()),
        'metrics': metrics,
    })
    return os.path.relpath(path, ROOT)


def preserve_sidecar(source, mode):
    build = os.environ.get('YUMI_BUILD_PATH', '')
    if not source or not build or not mode:
        return None
    try:
        destination = os.path.join(os.path.abspath(build), 'gc-runtime-failure-%s.metrics.json' % mode)
        shutil.copyfile(source, destination)
        return {'path': destination, 'sha256': file_digest(destination)}
    except OSError:
        return None


def digest(path):
    try:
        return file_digest(path)
    except OSError:
        return None


def identity_digest(path):
    try:
        return file_digest(path)
    except OSError as error:
        raise Failure('%s: %s' % (path, error.strerror))


def read_json(path):
    try:
        with open(path) as file:
            return json.load(file)
    except (OSError, ValueError):
        return None


def write_json(path, value):
    with open(path, 'w') as file:
        file.write(json.dumps(value, separators=(',', ':')) + '\n')


def read_tail(file):
    file.seek(0, os.SEEK_END)
    size = file.tell()
    file.seek(max(0, size - OUTPUT_LIMIT))
    text = file.read().decode(errors='replace')
    file.close()
    return text, size > OUTPUT_LIMIT


def write_through(outcome):
    if outcome.stdout:
        sys.stdout.write(outcome.stdout)
        sys.stdout.flush()
    if outcome.stderr:
        sys.stderr.write(outcome.stderr)


def signal_group(process, signum):
    if process.returncode is not None:
        return
    try:
        os.killpg(process.pid, signum)
    except OSError:
        try:
            process.send_signal(signum)
        except OSError:
            pass


def signal_name(signum):
    try:
        return signal.Signals(signum).name
    except ValueError:
        return str(signum)


def exit_by_signal(signum):
    signal.signal(signum, signal.SIG_DFL)
    os.kill(os.getpid(), signum)
