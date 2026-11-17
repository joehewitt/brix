import os
import subprocess
import sys
import unittest

from common import ROOT
from testing import support

import stress


def check_profile(case, flag, sanitizer, symbol):
    support.run([sys.executable, os.path.join(ROOT, 'toolchain', 'runtime.py'), 'build', flag], 180)
    executable = os.path.join(support.scratch(case), 'sanitizer-runtime')
    environment = support.environment(
        BRIX_SANITIZER=sanitizer,
        TSAN_OPTIONS='halt_on_error=1',
        ASAN_OPTIONS='halt_on_error=1:detect_leaks=0',
    )
    support.compile_brix(stress.fixture('sanitizer-runtime'), executable, environment)
    symbols = subprocess.run(['nm', executable], stdout=subprocess.PIPE, text=True, check=True).stdout
    case.assertIn(symbol, symbols, 'The fixture has no %s instrumentation' % sanitizer)
    case.assertIn('gc-sanitizer-ok 2096128', support.run([executable], 120, environment))
    conservative = dict(environment, BRIX_GC_ALWAYS_CONSERVATIVE_STACKS='1')
    case.assertIn('gc-sanitizer-ok 2096128', support.run([executable], 120, conservative))
    stress.run_memory_cases(case, environment)


class GCSanitizers(unittest.TestCase):
    def test_thread_sanitizer_instruments_and_runs_the_runtime(self):
        check_profile(self, '--tsan', 'thread', '__tsan_')

    def test_address_sanitizer_instruments_and_runs_the_runtime(self):
        check_profile(self, '--asan', 'address', '__asan_')
