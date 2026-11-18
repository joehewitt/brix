import json
import os
import signal
import subprocess
import time
import unittest

from common import ROOT
from testing import support

RUNNER = [os.path.join(ROOT, 'bin', 'brix'), 'bench', 'launch']
RESULT_LINE = 'bench-result fake/case checksum=7'
PID_WAIT_SECONDS = 30
COUNT_LAUNCH = ['echo launch >> "$LAUNCH_LOG"', 'count=$(($(wc -l < "$LAUNCH_LOG")))']
CLEARED = ('PROBES_ENABLED', 'PROBES_PATH', 'BRIX_BENCH_METRICS_PATH')


class FakeBenchmark:
    def __init__(self, case, lines):
        self.root = support.scratch(case)
        self.executable = os.path.join(self.root, 'fake-case')
        self.launch_log = os.path.join(self.root, 'launches.log')
        self.pid_file = os.path.join(self.root, 'pid')
        with open(self.executable, 'w') as file:
            file.write('\n'.join(['#!/bin/sh'] + lines + ['']))
        os.chmod(self.executable, 0o755)

    def environment(self, **overlay):
        values = {name: value for name, value in os.environ.items() if name not in CLEARED}
        values.update(LAUNCH_LOG=self.launch_log, PID_FILE=self.pid_file)
        values.update(overlay)
        return values

    def run(self, **overlay):
        return subprocess.run(RUNNER + [self.executable], env=self.environment(**overlay),
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

    def launches(self):
        with open(self.launch_log) as file:
            return len([line for line in file.read().split('\n') if line])

    def pid(self):
        deadline = time.time() + PID_WAIT_SECONDS
        while time.time() < deadline:
            try:
                with open(self.pid_file) as file:
                    text = file.read().strip()
                if text and int(text) > 0:
                    return int(text)
            except FileNotFoundError:
                pass
            time.sleep(0.01)
        raise AssertionError('The benchmark did not record its process id')


def stdout_lines(result):
    return [line for line in result.stdout.split('\n') if line]


def process_metrics(case, result):
    lines = [line for line in stdout_lines(result) if line.startswith('bench-metrics ')]
    case.assertEqual(len(lines), 1)
    return json.loads(lines[0][len('bench-metrics '):])['process']


def assert_gone(case, pid):
    with case.assertRaises(ProcessLookupError):
        os.kill(pid, 0)


class BenchmarkLaunches(unittest.TestCase):
    def test_reports_warm_launch_process_metrics(self):
        bench = FakeBenchmark(self, COUNT_LAUNCH + [
            'echo "$count" >> "$PROBES_PATH"',
            'echo "{\\"launch\\": $count}" > "$BRIX_BENCH_METRICS_PATH"',
            'echo "%s"' % RESULT_LINE,
        ])
        probes = os.path.join(bench.root, 'fake-case.probes')
        metrics_path = os.path.join(bench.root, 'fake-case.metrics.json')
        result = bench.run(PROBES_ENABLED='grammar', PROBES_PATH=probes, BRIX_BENCH_METRICS_PATH=metrics_path)

        self.assertEqual(result.returncode, 0)
        self.assertEqual(result.stderr, '')
        self.assertEqual([line for line in stdout_lines(result) if line.startswith('bench-result ')], [RESULT_LINE])
        metrics = process_metrics(self, result)
        self.assertEqual(metrics['launches'], bench.launches() - 1)
        self.assertTrue(2 <= metrics['launches'] <= 100)
        for key in ('wallMs', 'cpuMs', 'instructions', 'cycles'):
            self.assertGreater(metrics[key], 0, key)
        self.assertGreaterEqual(metrics['userMs'], 0)
        self.assertGreaterEqual(metrics['systemMs'], 0)
        self.assertIsInstance(metrics['instructions'], int)
        self.assertIsInstance(metrics['cycles'], int)
        self.assertIsInstance(metrics['involuntaryContextSwitches'], int)
        self.assertGreaterEqual(metrics['involuntaryContextSwitches'], 0)
        with open(probes) as file:
            self.assertEqual(file.read(), '2\n')
        with open(metrics_path) as file:
            self.assertEqual(json.load(file), {'launch': 2})
        self.assertEqual(sorted(os.listdir(bench.root)),
                         ['fake-case', 'fake-case.metrics.json', 'fake-case.probes', 'launches.log'])

    def test_measures_a_slow_benchmark_once_after_the_discarded_launch(self):
        bench = FakeBenchmark(self, COUNT_LAUNCH + ['sleep 0.3', 'echo "%s"' % RESULT_LINE])
        result = bench.run()

        self.assertEqual(result.returncode, 0)
        metrics = process_metrics(self, result)
        self.assertEqual(metrics['launches'], 1)
        self.assertEqual(bench.launches(), 2)
        self.assertTrue(300 <= metrics['wallMs'] < 5000)

    def test_propagates_a_failing_discarded_launch_without_metrics(self):
        bench = FakeBenchmark(self, COUNT_LAUNCH + ['echo "early failure" >&2', 'exit 3'])
        result = bench.run()

        self.assertEqual(result.returncode, 3)
        self.assertIn('early failure', result.stderr)
        self.assertEqual(result.stdout, '')
        self.assertEqual(bench.launches(), 1)

    def test_reports_a_failing_measured_launch_with_its_output(self):
        bench = FakeBenchmark(self, COUNT_LAUNCH + [
            'if [ "$count" -ge 3 ]; then',
            '    echo "late failure" >&2',
            '    exit 4',
            'fi',
            'echo "%s"' % RESULT_LINE,
        ])
        result = bench.run()

        self.assertEqual(result.returncode, 4)
        self.assertIn('Measured launch 2 failed with exit code 4', result.stderr)
        self.assertIn('late failure', result.stderr)
        self.assertEqual(stdout_lines(result), [RESULT_LINE])
        self.assertEqual(bench.launches(), 3)

    def test_exits_by_the_signal_that_killed_the_benchmark(self):
        bench = FakeBenchmark(self, COUNT_LAUNCH + ['kill -KILL $$'])
        result = bench.run()

        self.assertEqual(result.returncode, -signal.SIGKILL)
        self.assertEqual(result.stdout, '')
        self.assertEqual(bench.launches(), 1)

    def test_forwards_termination_to_the_running_benchmark(self):
        bench = FakeBenchmark(self, ['echo $$ > "$PID_FILE"', 'exec sleep 30'])
        runner = subprocess.Popen(RUNNER + [bench.executable], env=bench.environment(),
                                  stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        pid = bench.pid()
        runner.send_signal(signal.SIGTERM)
        stdout, _ = runner.communicate()

        self.assertEqual(runner.returncode, -signal.SIGTERM)
        self.assertEqual(stdout, '')
        assert_gone(self, pid)

    def test_kills_the_benchmark_at_its_deadline(self):
        bench = FakeBenchmark(self, ['[ -n "$PID_FILE" ] || exit 0', 'echo $$ > "$PID_FILE"', 'exec sleep 30'])
        first = subprocess.run([bench.executable], env={'PATH': os.environ['PATH']})
        self.assertEqual(first.returncode, 0)
        result = subprocess.run(RUNNER + ['--deadline', '1s', bench.executable],
                                env=bench.environment(), stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

        self.assertEqual(result.returncode, 124)
        self.assertIn('The benchmark launches passed their 1.0s deadline', result.stderr)
        self.assertEqual(result.stdout, '')
        assert_gone(self, bench.pid())

    def test_passes_gc_runtime_launchers_through_unmeasured(self):
        bench = FakeBenchmark(self, COUNT_LAUNCH + ['echo "%s"' % RESULT_LINE])
        result = bench.run(PROBES_ENABLED='bench-gc-runtime,gc')

        self.assertEqual(result.returncode, 0)
        self.assertEqual(stdout_lines(result), [RESULT_LINE])
        self.assertEqual(bench.launches(), 1)
