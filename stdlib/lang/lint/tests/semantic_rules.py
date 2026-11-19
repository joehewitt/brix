import json
import os
import shutil
import subprocess
import tempfile
import unittest

from common import ROOT
from testing import support

LINTER = [os.path.join(ROOT, 'bin', 'brix'), 'lint']
RULES = ('style/no-redundant-context-cast', 'style/prefer-constructor-initializer')
LINT_SECONDS = 300

FIXTURE = '''(main
  (let values (list "a" "b"))
  (let start U32 (as (. values length) U32))
  (let wide U64 (as start U64))
  (let small U8 (as 5 U8))
  (print start)
  (print wide)
  (print small)
  (print (call (. (call (of Holder Byte) 7) wide)))
  (print (. (call (of Holder U32) 9) item))
  (print (. (call Steps 2) first value))
)

(type + (of Holder Item)
  (prop + item Item)
  (fn-do + This _
    (param item _)
  )
  (fn-expr + wide U64
    (as (call takes-byte (as item Byte)) U64)
  )
)

(fn-expr _ takes-byte Byte
  (param value Byte)
  value
)

(type + Steps
  (prop + first Text)
  (prop + label Text)
  (prop + count Int)
  (fn-do + This _
    (param size Int)
    (= label (call Text (str size)))
    (= first (call add-step "Check"))
  )
  (fn-do + add-step Text
    (param title String)
    (+= count 1)
    (return (call Text title))
  )
)

(type + Text
  (prop + value String)
  (fn-do + This _
    (param value _)
  )
)
'''


def line_of(text):
    return FIXTURE.split('\n').index(text) + 1


class SemanticRules(unittest.TestCase):
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
        self.assertEqual(self.statuses(rule, '  (let start U32 (as (. values length) U32))'), set())
        self.assertEqual(self.statuses(rule, '  (let wide U64 (as start U64))'), {'actionable'})
        self.assertEqual(self.statuses(rule, '  (let small U8 (as 5 U8))'), {'actionable'})

    def test_a_cast_in_generic_code_is_for_review(self):
        rule = 'style/no-redundant-context-cast'
        self.assertEqual(self.statuses(rule, '    (as (call takes-byte (as item Byte)) U64)'), {'review'})

    def test_a_constructor_value_that_calls_a_method_is_for_review(self):
        rule = 'style/prefer-constructor-initializer'
        self.assertEqual(self.statuses(rule, '    (= label (call Text (str size)))'), {'actionable'})
        self.assertEqual(self.statuses(rule, '    (= first (call add-step "Check"))'), {'review'})


if __name__ == '__main__':
    unittest.main()
