import json
import os
import shutil
import subprocess
import tempfile
import unittest

from common import ROOT
from testing import support

LINTER = [os.path.join(ROOT, 'bin', 'yumi'), 'lint']
RULES = ('style/no-redundant-context-cast', 'style/prefer-constructor-initializer')
LINT_SECONDS = 300
DIVIDER = '━' * 20
MEMBER = '─' * 20

FIXTURE = f'''. values = ["a", "b"]
. start U32 = values.length as U32
. wide U64 = start as U64
. small U8 = 5 as U8
-- start
-- wide
-- small
-- Holder'Byte(7).wide()
-- Holder'U32(9).item
-- Steps(2).first.value

{DIVIDER}
+ Holder'Item

+ item Item

{MEMBER}
+ This
    - item

{MEMBER}
+ wide U64

takes-byte(item as Byte) as U64

{DIVIDER}
_ takes-byte Byte
    - value Byte

value

{DIVIDER}
+ Steps

+ first Text
+ label Text
+ count Int

{MEMBER}
+ This
    - size Int

label = Text("$size")
first = add-step("Check")

{MEMBER}
+ do add-step Text
    - title String

count += 1
= Text(title)

{DIVIDER}
+ Text

+ value String

{MEMBER}
+ This
    - value
'''


def line_of(text):
    return FIXTURE.split('\n').index(text) + 1


class SemanticRules(unittest.TestCase):
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
                     '--format', 'json-lines']
        for rule in RULES:
            arguments += ['--rule', rule]
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
            if diagnostic:
                key = (diagnostic['ruleId'], diagnostic['line'])
                found.setdefault(key, set()).add(diagnostic['status'])
        if not ended:
            raise AssertionError('%s did not finish\n%s' % (command[-1], result.stderr))
        return found

    def statuses(self, rule, text):
        return self.found.get((rule, line_of(text)), set())

    def test_a_cast_that_narrows_is_not_redundant(self):
        rule = 'style/no-redundant-context-cast'
        self.assertEqual(self.statuses(rule, '. start U32 = values.length as U32'), set())
        self.assertEqual(self.statuses(rule, '. wide U64 = start as U64'), {'actionable'})
        self.assertEqual(self.statuses(rule, '. small U8 = 5 as U8'), {'actionable'})

    def test_a_cast_in_generic_code_is_for_review(self):
        rule = 'style/no-redundant-context-cast'
        self.assertEqual(self.statuses(rule, 'takes-byte(item as Byte) as U64'), {'review'})

    def test_a_constructor_value_that_calls_a_method_is_for_review(self):
        rule = 'style/prefer-constructor-initializer'
        self.assertEqual(self.statuses(rule, 'label = Text("$size")'), {'actionable'})
        self.assertEqual(self.statuses(rule, 'first = add-step("Check")'), {'review'})


if __name__ == '__main__':
    unittest.main()
