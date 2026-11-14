import os
import unittest

from testing import support

FIXTURES = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'fixtures')
STRESS_SETTINGS = (
    'YUMI_GC_STRESS',
    'YUMI_GC_STRESS_BYTES',
    'YUMI_GC_REPLAY',
    'YUMI_GC_REPLAY_SEED',
    'YUMI_GC_REPLAY_INTERVAL',
)
MEMORY_CASES = (
    ('tiny-heap-wave', 'tiny-heap-wave-ok', {
        'YUMI_GC_MIN_HEAP_GOAL': str(128 * 1024),
        'YUMI_GC_HEAP_MIN_GROWTH': str(64 * 1024),
        'YUMI_GC_VERIFY_EACH_SWEEP': '1',
    }),
    ('resize-shrink-tail', 'resize-shrink-tail-ok 0,1', {'YUMI_GC_VERIFY_EACH_SWEEP': '1'}),
    ('markdown-refresh', 'markdown-refresh-ok', {'YUMI_GC_MIN_HEAP_GOAL': str(1024 * 1024)}),
    ('ffi-panic-cleanup', 'ffi-panic-cleanup-ok', {}),
)


def fixture(name):
    return os.path.join(FIXTURES, name + '.yumi')


def run_memory_cases(case, environment):
    directory = support.scratch(case)
    for name, expected, settings in MEMORY_CASES:
        executable = os.path.join(directory, name)
        support.compile_yumi(fixture(name), executable, environment)
        output = support.run([executable], 90, dict(environment, **settings))
        case.assertIn(expected, output, name)


class GCStress(unittest.TestCase):
    def test_memory_stress(self):
        run_memory_cases(self, support.environment())

    def test_memory_stress_under_checkmark(self):
        environment = support.environment(YUMI_GC_CHECKMARK='1')
        for name in STRESS_SETTINGS:
            environment.pop(name, None)
        run_memory_cases(self, environment)

    def test_string_iterator_concurrency(self):
        executable = os.path.join(support.scratch(self), 'string-concurrency')
        environment = support.environment(YUMI_GC_VALIDATE_STACKMAPS='1')
        support.compile_yumi(fixture('string-concurrency'), executable, environment)
        output = support.run([executable], 20, environment)
        self.assertEqual(output.strip(), 'gc-string-iterator-concurrency-ok')
