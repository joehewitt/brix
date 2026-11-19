import json
import os
import shutil
import subprocess
import tempfile
import unittest

from common import ROOT
from testing import support

LINTER = [os.path.join(ROOT, 'bin', 'brix'), 'lint']
RULE = 'style/no-redundant-literal-suffix'
LINT_SECONDS = 300

FIXTURE = '''(main
  (let values (list "a" "b"))
  (let scalar U32 (as (. values length) U32))
  (let byte U8 7)
  (let masked (/and/ scalar 255u32))
  (let small (< scalar 65536u32))
  (let moved (/lshift/ scalar 3u32))
  (let shifted (/lshift/ 1u64 scalar))
  (let owner 0u32)
  (let typed U32 7u32)
  (let ratio F64 2.5f64)
  (let total U32 0)
  (+= total 1u32)
  (let pair (+ 1u32 2u32))
  (let widened (+ byte 3u32))
  (let unfit (+ byte 300u32))
  (let hexed (/and/ scalar 0x0000ffff))
  (print masked)
  (print small)
  (print moved)
  (print shifted)
  (print owner)
  (print typed)
  (print ratio)
  (print total)
  (print pair)
  (print widened)
  (print unfit)
  (print hexed)
  (print (call takes 3u32))
  (print (call pick 4u32))
  (print (call flag true))
  (print (call bumped values))
)

(fn-expr + takes U32
  (param value U32)
  value
)

(fn-expr + pick String
  (param value U32)
  "u32"
)

(fn-expr + pick String
  (param value Int)
  "int"
)

(fn-expr + flag U64
  (param c Bool)
  (if-expr c 1u64 0u64)
)

(fn-expr + (of bumped T) U32
  (param items (list T))
  (+ (as (. items length) U32) 1u32)
)
'''

REPORTED = [
    ('  (let masked (/and/ scalar 255u32))', '255u32'),
    ('  (let small (< scalar 65536u32))', '65536u32'),
    ('  (let moved (/lshift/ scalar 3u32))', '3u32'),
    ('  (let typed U32 7u32)', '7u32'),
    ('  (let ratio F64 2.5f64)', '2.5f64'),
    ('  (+= total 1u32)', '1u32'),
    ('  (print (call takes 3u32))', '3u32'),
    ('  (if-expr c 1u64 0u64)', '1u64'),
    ('  (if-expr c 1u64 0u64)', '0u64'),
]

KEPT = [
    '  (let shifted (/lshift/ 1u64 scalar))',
    '  (let owner 0u32)',
    '  (let pair (+ 1u32 2u32))',
    '  (let widened (+ byte 3u32))',
    '  (let unfit (+ byte 300u32))',
    '  (let hexed (/and/ scalar 0x0000ffff))',
    '  (print (call pick 4u32))',
    '  (+ (as (. items length) U32) 1u32)',
]


def position_of(text, literal):
    lines = FIXTURE.split('\n')
    row = lines.index(text)
    return row + 1, lines[row].index(literal) + 1


class LiteralSuffix(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        directory = tempfile.mkdtemp(prefix='brix-test-')
        cls.addClassCleanup(shutil.rmtree, directory, True)
        cls.source = os.path.join(directory, 'fixture.bx')
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

    def test_the_fix_drops_only_the_suffix(self):
        fixed = FIXTURE.encode('utf-16-le')
        for text, literal in REPORTED:
            diagnostic = self.found[position_of(text, literal)]
            self.assertEqual(diagnostic['status'], 'actionable')
            edits = diagnostic['fix']['edits']
            self.assertEqual(len(edits), 1)
            edit = edits[0]
            number = literal[:len(literal) - len(literal.lstrip('0123456789.'))]
            removed = fixed[edit['start'] * 2:edit['end'] * 2].decode('utf-16-le')
            self.assertEqual((removed, edit['replacement']), (literal, number))


if __name__ == '__main__':
    unittest.main()
