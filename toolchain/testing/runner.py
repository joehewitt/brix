import importlib.util
import json
import os
import re
import sys
import time
import traceback
import unittest

from common import ROOT, Failure

PROTOCOL = 'ometer.testometer.v1'
CLASS = re.compile(r'^class (\w+)\(')
METHOD = re.compile(r'^    def (test_\w+)\(')


class PythonTest:
    def __init__(self, relative_path, source_path, owner, name, line):
        self.relative_path = relative_path
        self.source_path = source_path
        self.owner = owner
        self.name = name
        self.line = line
        self.last_line = line
        self.id = 'test:%s:%d' % (relative_path, line)
        self.label = name[len('test_'):].replace('_', ' ')


def main(arguments):
    if arguments not in (['catalog'], ['run']):
        raise Failure('usage: brixtest-python catalog|run')
    root = os.path.realpath(os.environ.get('OMETER_TESTOMETER_PROJECT_ROOT') or os.getcwd())
    files = os.environ.get('BRIXTEST_PYTHON_FILES', '').split()
    if not files:
        raise Failure('BRIXTEST_PYTHON_FILES must list the test files of the suite')
    os.chdir(ROOT)
    files = [os.path.relpath(os.path.join(ROOT, path), root) for path in files]
    tests = [test for relative_path in files for test in catalog_file(root, relative_path)]
    channel = os.fdopen(os.dup(1), 'w')
    os.dup2(2, 1)
    sys.stdout = sys.stderr
    if arguments == ['catalog']:
        emit(channel, catalog(root, files, tests))
    else:
        run_tests(channel, tests)


def catalog_file(root, relative_path):
    source_path = os.path.join(root, relative_path)
    try:
        with open(source_path) as file:
            lines = file.read().split('\n')
    except OSError as error:
        raise Failure('Cannot read %s: %s' % (relative_path, error.strerror))
    found = []
    owner = None
    for number, text in enumerate(lines, 1):
        start = CLASS.match(text)
        if start:
            owner = start.group(1)
        method = METHOD.match(text)
        if method and owner:
            found.append(PythonTest(relative_path, source_path, owner, method.group(1), number))
    for test, following in zip(found, found[1:] + [None]):
        test.last_line = following.line - 1 if following else len(lines)
    return found


def catalog(root, files, tests):
    return {
        'tests': [{
            'id': test.id,
            'label': test.label,
            'selector': test.id,
            'sourcePath': test.source_path,
            'relativePath': test.relative_path,
            'lineNoStart': test.line,
            'lineNoEnd': test.last_line,
            'selectors': ['file:' + test.relative_path, test.relative_path,
                          '%s:%d' % (test.relative_path, test.line)],
        } for test in tests],
        'suites': [{
            'kind': 'suite',
            'children': [{
                'kind': 'suite',
                'id': 'file:' + relative_path,
                'label': relative_path,
                'selector': 'file:' + relative_path,
                'sourcePath': os.path.join(root, relative_path),
                'relativePath': relative_path,
                'children': [{'kind': 'test', 'id': test.id}
                             for test in tests if test.relative_path == relative_path],
            } for relative_path in files],
        }],
    }


def run_tests(channel, tests):
    wanted = selected_test_ids()
    chosen = [test for test in tests if wanted is None or test.id in wanted]
    emit(channel, event('runStart', count=len(chosen), testIds=[test.id for test in chosen]))
    result = Result(channel, chosen)
    for relative_path in dict.fromkeys(test.relative_path for test in chosen):
        group = [test for test in chosen if test.relative_path == relative_path]
        try:
            module = load_module(group[0].source_path)
        except Exception:
            result.crash(group, import_failure())
            continue
        suite = unittest.TestSuite()
        for test in group:
            owner = getattr(module, test.owner, None)
            if owner is None:
                result.crash([test], '%s has no class %s' % (relative_path, test.owner))
                continue
            suite.addTest(owner(test.name))
        suite.run(result)
    emit(channel, event('runEnd', count=len(chosen), killed=0, **result.counts))


def load_module(source_path):
    directory = os.path.dirname(source_path)
    if directory not in sys.path:
        sys.path.insert(0, directory)
    name = 'brixtest_' + re.sub(r'\W', '_', os.path.relpath(source_path))
    spec = importlib.util.spec_from_file_location(name, source_path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def import_failure():
    kind, value, trace = sys.exc_info()
    frames = [frame for frame in traceback.extract_tb(trace)
              if frame.filename != __file__ and not frame.filename.startswith('<frozen')]
    lines = ['Traceback (most recent call last):\n'] + traceback.format_list(frames)
    return ''.join(lines + traceback.format_exception_only(kind, value)).rstrip('\n')


class Result(unittest.TestResult):
    def __init__(self, channel, tests):
        super().__init__()
        self.channel = channel
        self.tests = {(test.owner, test.name): test for test in tests}
        self.started = {}
        self.counts = {'passed': 0, 'failed': 0, 'skipped': 0, 'crashed': 0}

    def known(self, case):
        return self.tests.get((type(case).__name__, getattr(case, '_testMethodName', '')))

    def startTest(self, case):
        super().startTest(case)
        test = self.known(case)
        if test:
            self.started[test.id] = time.time()
            emit(self.channel, event('testStart', id=test.id))

    def addSuccess(self, case):
        self.finish(case, 'passed')

    def addFailure(self, case, error):
        self.finish(case, 'failed', self.error_text(case, error))

    def addError(self, case, error):
        test = self.known(case)
        if test:
            self.finish(case, 'failed', self.error_text(case, error))
        else:
            emit(self.channel, event('error', message='%s\n%s' % (case, self.error_text(case, error))))

    def addSkip(self, case, reason):
        self.finish(case, 'skipped', reason)

    def error_text(self, case, error):
        if error[0] is AssertionError:
            return str(error[1])
        return self._exc_info_to_string(error, case).rstrip('\n')

    def finish(self, case, status, errors=''):
        test = self.known(case)
        if not test:
            return
        started = self.started.get(test.id, time.time())
        self.counts[status] += 1
        fields = {'errors': errors} if errors else {}
        emit(self.channel, event('testResult', id=test.id, status=status,
                                 duration=(time.time() - started) * 1000, **fields))

    def crash(self, tests, text):
        for test in tests:
            emit(self.channel, event('testStart', id=test.id))
            self.counts['crashed'] += 1
            emit(self.channel, event('testResult', id=test.id, status='crashed', duration=0, errors=text))


def selected_test_ids():
    text = os.environ.pop('OMETER_TESTOMETER_SELECTED_TEST_IDS', '')
    if not text:
        return None
    decoded = json.loads(text)
    if not isinstance(decoded, list) or not all(isinstance(value, str) for value in decoded):
        raise Failure('Selected Python test IDs must be a JSON array of strings')
    return set(decoded)


def event(kind, **fields):
    return dict({'protocol': PROTOCOL, 'version': 1, 'type': kind}, **fields)


def emit(channel, value):
    channel.write(json.dumps(value) + '\n')
    channel.flush()
