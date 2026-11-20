import collections
import os
import re
import shutil
import subprocess
import tempfile
import unittest

from common import ROOT
from testing import support

BRIX = os.path.join(ROOT, 'bin', 'brix')
COMPILE_SECONDS = 300
FUNCTION = re.compile(r'^function __source__:(\S+?)(?:\.[0-9a-f]{16})+ kind=', re.MULTILINE)

PROGRAM = '''(type-line + Apples (alias apples) (is F64))
(type-line + Pears (alias pears) (is F64))

(type + Halver (is (fn-type F64 (param amount F64))))

(fn-expr + (of larger Num) Num
  (param a Num)
  (param b Num)
  (case? (> b a)
    b
  )
  (default
    a
  )
)

(fn-do + (of largest Num) Num
  (param a Num)
  (param b Num)
  (param c Num)
  (return (call larger (call larger a b) c))
)

(fn-do + (of named Num) String
  (param a Num)
  (return (str Num " " (as a F64)))
)

(fn-expr + (of half Num) Num
  (param amount Num)
  (/ amount 2)
)

(main
  (print (as (call largest 1apples 3apples 2apples) F64))
  (print (as (call largest 4pears 2pears 3pears) F64))
  (print (call named 1apples))
  (print (call named 2pears))
  (let a Halver (call (of half Apples) ...))
  (let b Halver (call (of half Pears) ...))
  (print (== a b))
)
'''
OUTPUT = '''3
4
[TypeInfo Apples] 1
[TypeInfo Pears] 2
false
'''


class FoldedCopies(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.directory = tempfile.mkdtemp(prefix='brix-test-')
        cls.addClassCleanup(shutil.rmtree, cls.directory, True)
        cls.source = os.path.join(cls.directory, 'program.bx')
        with open(cls.source, 'w', encoding='utf-8') as file:
            file.write(PROGRAM)

    def run_brix(self, *arguments):
        return subprocess.run([BRIX, self.source] + list(arguments), cwd=ROOT,
                              env=support.environment(), stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT, text=True, timeout=COMPILE_SECONDS)

    def copies(self):
        result = self.run_brix('--inspect', 'emitted')
        found = FUNCTION.findall(result.stdout)
        if result.returncode != 0 or not found:
            raise AssertionError('program did not compile\n%s' % result.stdout[-4000:])
        return collections.Counter(found)

    def test_copies_that_lower_alike_are_emitted_once(self):
        copies = self.copies()
        self.assertEqual(copies['larger'], 1)
        self.assertEqual(copies['largest'], 1)

    def test_a_copy_that_reads_its_type_or_is_a_value_stays_apart(self):
        copies = self.copies()
        self.assertEqual(copies['named'], 2)
        self.assertEqual(copies['half.adapter'], 2)

    def test_a_folded_program_runs_as_written(self):
        executable = os.path.join(self.directory, 'program')
        result = self.run_brix('-o', executable)
        self.assertEqual(result.returncode, 0, result.stdout[-4000:])
        self.assertEqual(result.stdout, OUTPUT)


if __name__ == '__main__':
    unittest.main()
