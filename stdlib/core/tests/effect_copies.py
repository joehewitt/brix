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
FUNCTION = re.compile(r'^function (\S+) kind=', re.MULTILINE)

PROGRAM = '''(import /data/json)
(import /data/markdown)
(import /time/clock)

(main
  (print (call work "[1, 2, 3]"))
  (print (call guarded "[4, 5]"))
)

(effect + world)

(fn-do + roll Int (door world)
  (return 10)
)

(fn-do + real-roll Int
  (let items (list 4 5))
  (+= items 6)
  (return (+ (call roll) (. items size)))
)

(type + Stub
  (fn-do + roll Int
    (return (+ (call real-roll) 1))
  )
)

(type + Counter (is Interface)
  (fn-do + count Int)
)

(type + Fixed
  (implements Counter)
  (fn-do + count Int
    (return 2)
  )
)

(fn-do + work Int (?! Error)
  (param text String)
  (let items (list 1 2))
  (+= items 3)
  (let parsed (call parse-json text))
  (let page (call Markdown "# title"))
  (let counter (as (call Fixed) Counter))
  (let doubled (=> ((: n Int)) (* n 2)))
  (return (+ (. items size) (. (as parsed (list JSON)) size) (call (. counter count)) (call doubled 1) (call roll)))
)

(fn-do + guarded Int (?! Error) GUARD
  (param text String)
  (return BODY)
)
'''
SHARED = '(call work text)'
TRIVIAL = '(. text size)'
BINDING = '(with world (call Stub))'


class EffectCopies(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.directory = tempfile.mkdtemp(prefix='brix-test-')
        cls.addClassCleanup(shutil.rmtree, cls.directory, True)

    def functions(self, name, guard, body):
        folder = os.path.join(self.directory, name)
        os.makedirs(folder)
        source = os.path.join(folder, 'program.bx')
        with open(source, 'w', encoding='utf-8') as file:
            file.write(PROGRAM.replace('GUARD', guard).replace('BODY', body))
        result = subprocess.run([BRIX, source, '--inspect', 'emitted'], cwd=ROOT,
                                env=support.environment(), stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, text=True, timeout=COMPILE_SECONDS)
        found = FUNCTION.findall(result.stdout)
        if result.returncode != 0 or not found:
            raise AssertionError('%s did not compile\n%s' % (name, result.stdout[-4000:]))
        return sorted(found)

    def test_a_ban_compiles_no_copy_of_what_it_reaches(self):
        free = self.functions('free', '', SHARED)
        banned = self.functions('banned', '(without clock)', SHARED)
        self.assertEqual(banned, free)

    def test_a_binding_compiles_no_copy_of_what_it_reaches(self):
        apart = self.functions('apart', BINDING, TRIVIAL)
        around = self.functions('around', BINDING, SHARED)
        self.assertEqual(around, apart)


if __name__ == '__main__':
    unittest.main()
