import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest

from common import ROOT
from testing import support

LINTER = [os.path.join(ROOT, 'bin', 'brix'), 'lint']
CONFLICTS = 'names/import-conflicts'
SORT = 'imports/sort'
LINT_SECONDS = 300
ON_MAC = sys.platform == 'darwin'

MODULES = {
    'variant.linux.bx': '(prop + variant-name _ (= "linux"))\n',
    'variant.macos.bx': '(prop + variant-name _ (= "macos"))\n',
    'variant.apple.bx': '(prop + variant-name _ (= "apple"))\n',
}
SOURCES = {
    'platforms.bx': '(import ./variant.macos)\n(import ./variant.linux)\n',
    'families.bx': '(import ./variant.macos)\n(import ./variant.apple)\n',
}
HEADER_SOURCES = {
    'headers.bx': '(import /unistd.h)\n(import /stdlib.h)\n',
    'overloads.bx': '(import /string.h)\n(import /dns_sd.h)\n',
}


def conflict_names(found, file):
    return {diagnostic['message'].split('"')[1]
            for diagnostic in found.get((file, CONFLICTS), [])}


class ImportConflicts(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        directory = tempfile.mkdtemp(prefix='brix-test-')
        cls.addClassCleanup(shutil.rmtree, directory, True)
        sources = dict(SOURCES, **HEADER_SOURCES) if ON_MAC else SOURCES
        for name, text in dict(MODULES, **sources).items():
            with open(os.path.join(directory, name), 'w', encoding='utf-8') as file:
                file.write(text)
        sources = [os.path.join(directory, name) for name in sources]
        cls.found = cls.lint(LINTER, sources)

    @classmethod
    def lint(cls, command, sources):
        arguments = sources + ['--format', 'json-lines', '--rule', CONFLICTS, '--rule', SORT]
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
                key = (os.path.basename(diagnostic['path']), diagnostic['ruleId'])
                found.setdefault(key, []).append(diagnostic)
        if not ended:
            raise AssertionError('%s did not finish\n%s' % (command[-1], result.stderr))
        return found

    def sort_fix(self, found, file):
        diagnostics = found[(file, SORT)]
        self.assertEqual(len(diagnostics), 1)
        return diagnostics[0].get('fix')

    def test_platform_variants_no_build_loads_together_do_not_conflict(self):
        self.assertEqual(conflict_names(self.found, 'platforms.bx'), set())
        self.assertIsNotNone(self.sort_fix(self.found, 'platforms.bx'))

    def test_platform_variants_one_build_loads_together_conflict(self):
        self.assertEqual(conflict_names(self.found, 'families.bx'), {'variant-name'})
        self.assertIsNone(self.sort_fix(self.found, 'families.bx'))

    @unittest.skipUnless(ON_MAC, 'declarations of the macOS SDK')
    def test_agreeing_c_declarations_in_two_headers_are_one_binding(self):
        self.assertEqual(conflict_names(self.found, 'headers.bx'), set())
        self.assertIsNotNone(self.sort_fix(self.found, 'headers.bx'))

    @unittest.skipUnless(ON_MAC, 'declarations of the macOS SDK')
    def test_c_declarations_that_differ_conflict(self):
        self.assertIn('strchr', conflict_names(self.found, 'overloads.bx'))
        self.assertIsNone(self.sort_fix(self.found, 'overloads.bx'))


if __name__ == '__main__':
    unittest.main()
