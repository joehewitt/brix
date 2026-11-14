import json
import os
import shutil
import subprocess
import tempfile
import unittest

from common import ROOT
from testing import support

LINTER = [os.path.join(ROOT, 'bin', 'yumi'), 'lint']
RULE = 'style/no-redundant-literal-suffix'
LINT_SECONDS = 300
DIVIDER = '━' * 20

FIXTURE = f'''. values = ["a", "b"]
. scalar U32 = values.length as U32
. byte U8 = 7
. masked = scalar /and/ 255u32
. small = scalar < 65536u32
. moved = scalar /lshift/ 3u32
. shifted = 1u64 /lshift/ scalar
. owner = 0u32
. typed U32 = 7u32
. ratio F64 = 2.5f64
. total U32 = 0
total += 1u32
. pair = 1u32 + 2u32
. widened = byte + 3u32
. unfit = byte + 300u32
. hexed = scalar /and/ 0x0000ffff
-- masked
-- small
-- moved
-- shifted
-- owner
-- typed
-- ratio
-- total
-- pair
-- widened
-- unfit
-- hexed
-- takes(3u32)
-- pick(4u32)
-- flag(true)
-- bumped(values)

{DIVIDER}
+ takes U32
    - value U32

value

{DIVIDER}
+ pick String
    - value U32

"u32"

{DIVIDER}
+ pick String
    - value Int

"int"

{DIVIDER}
+ flag U64
    - c Bool

if c then 1u64 else 0u64

{DIVIDER}
+ bumped'T U32
    - items [T]

(items.length as U32) + 1u32
'''

REPORTED = [
    ('. masked = scalar /and/ 255u32', '255u32'),
    ('. small = scalar < 65536u32', '65536u32'),
    ('. moved = scalar /lshift/ 3u32', '3u32'),
    ('. typed U32 = 7u32', '7u32'),
    ('. ratio F64 = 2.5f64', '2.5f64'),
    ('total += 1u32', '1u32'),
    ('-- takes(3u32)', '3u32'),
    ('if c then 1u64 else 0u64', '1u64'),
    ('if c then 1u64 else 0u64', '0u64'),
]

KEPT = [
    '. shifted = 1u64 /lshift/ scalar',
    '. owner = 0u32',
    '. pair = 1u32 + 2u32',
    '. widened = byte + 3u32',
    '. unfit = byte + 300u32',
    '. hexed = scalar /and/ 0x0000ffff',
    '-- pick(4u32)',
    '(items.length as U32) + 1u32',
]


def position_of(text, literal):
    lines = FIXTURE.split('\n')
    row = lines.index(text)
    return row + 1, lines[row].index(literal) + 1


class LiteralSuffix(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        directory = tempfile.mkdtemp(prefix='yumi-test-')
        cls.addClassCleanup(shutil.rmtree, directory, True)
        cls.source = os.path.join(directory, 'fixture.yumi')
        with open(cls.source, 'w', encoding='utf-8') as file:
            file.write(FIXTURE)
        cls.found = cls.lint(LINTER)

    @classmethod
    def lint(cls, command):
        arguments = [cls.source, '--semantic-rules', '--semantic-root', cls.source,
                     '--format', 'json-lines', '--rule', RULE]
        result = subprocess.run(command + arguments, cwd=ROOT, env=support.environment(),
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
                                timeout=LINT_SECONDS)
        found = {}
        ended = False
        for line in result.stdout.split('\n'):
            if not line.startswith('{'):
                continue
            event = json.loads(line)
            if event.get('event') == 'end':
                ended = True
            diagnostic = event.get('diagnostic')
            if diagnostic and diagnostic['ruleId'] == RULE:
                found[(diagnostic['line'], diagnostic['column'])] = diagnostic
        if not ended:
            raise AssertionError('%s did not finish\n%s' % (command[-1], result.stderr))
        return found

    def test_a_suffix_the_place_already_gives_is_reported(self):
        expected = {position_of(text, literal) for text, literal in REPORTED}
        self.assertEqual(set(self.found), expected)

    def test_a_suffix_that_selects_a_type_is_kept(self):
        kept = {FIXTURE.split('\n').index(text) + 1 for text in KEPT}
        self.assertEqual({line for line, _ in self.found} & kept, set())

    def test_the_fix_deletes_only_the_suffix(self):
        fixed = FIXTURE.encode('utf-16-le')
        for text, literal in REPORTED:
            diagnostic = self.found[position_of(text, literal)]
            self.assertEqual(diagnostic['status'], 'actionable')
            edits = diagnostic['fix']['edits']
            self.assertEqual(len(edits), 1)
            edit = edits[0]
            unit = literal.lstrip('0123456789.')
            removed = fixed[edit['start'] * 2:edit['end'] * 2].decode('utf-16-le')
            self.assertEqual((removed, edit['replacement']), (unit, ''))


if __name__ == '__main__':
    unittest.main()
