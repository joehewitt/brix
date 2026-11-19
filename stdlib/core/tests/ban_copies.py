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
  (return (+ (. items size) (. (as parsed (list JSON)) size) (call (. counter count)) (call doubled 1)))
)

(fn-do + guarded Int (?! Error) BANNED
  (param text String)
  (return (call work text))
)
'''


class BanCopies(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.directory = tempfile.mkdtemp(prefix='brix-test-')
        cls.addClassCleanup(shutil.rmtree, cls.directory, True)

    def functions(self, name, ban):
        folder = os.path.join(self.directory, name)
        os.makedirs(folder)
        source = os.path.join(folder, 'program.bx')
        with open(source, 'w', encoding='utf-8') as file:
            file.write(PROGRAM.replace('BANNED', ban))
        result = subprocess.run([BRIX, source, '--inspect', 'emitted'], cwd=ROOT,
                                env=support.environment(), stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, text=True, timeout=COMPILE_SECONDS)
        found = FUNCTION.findall(result.stdout)
        if result.returncode != 0 or not found:
            raise AssertionError('%s did not compile\n%s' % (name, result.stdout[-4000:]))
        return sorted(found)

    def test_a_ban_compiles_no_copy_of_what_it_reaches(self):
        free = self.functions('free', '')
        banned = self.functions('banned', '(without clock)')
        self.assertEqual(banned, free)


if __name__ == '__main__':
    unittest.main()
