import json
import os
import re
import shutil
import subprocess
import time
from concurrent.futures import ThreadPoolExecutor

import brix_sweep
import compiler
from common import BUILD, ROOT, Failure, say, take_folders

WORK = os.path.join(BUILD, 'brix-tests')
TREE = os.path.join(WORK, 'tree')
SOURCES = os.path.join(WORK, 'sources')
CONVERTED = os.path.join(WORK, 'converted')
REPORT = os.path.join(WORK, 'report.json')
TREE_FOLDERS = ('stdlib/',)
SPEC_EXTENSION = '.test.md'
LEFT_IN_PLACE = ('.yumi', SPEC_EXTENSION)
PARSE_SUITE = 'parse'
CASE_SUITES = (
    (PARSE_SUITE, 'stdlib/lang/tests/parse'),
    ('core', 'stdlib/core/tests'),
    ('diagnostics', 'stdlib/lang/tests/diagnostics'),
)
MODULE_SUITES = (('stdlib', 'stdlib'),)
FAILING = ('failed', 'crashed', 'killed')
PROGRAMS = ('yumi', 'brix')
EXPECTS_DIAGNOSTIC = ('compile-error', 'crash', 'warn')
KINDS = (
    ('shape', 'the converter makes Brix the reader rejects, or reads back differently'),
    ('diagnostic', 'a case that expects a diagnostic, which Brix reports otherwise'),
    ('other', 'other failures'),
)
SHOWN_FAILURES = 30
SHOWN_LINES = 12
WORKERS = os.cpu_count() or 4
OPENING = re.compile(r'(````|```(?!`))')
CLOSING = re.compile(r'```+[ \t\r\v\f]*\n?$')
LONG_CLOSING = re.compile(r'````+[ \t\r\v\f]*\n?$')
HEADING = re.compile(r'#+ ')
PLACE = re.compile(r'(no file|[\w./+-]+\.(?:yumi|bx)):\d+(?::\d+)?')
SOURCE_NAME = re.compile(r'([\w./+-]+)\.(?:yumi|bx)\b')
PLACE_LINE = re.compile(r'<[^<>]*:\d+>$')
CARET = re.compile(r'\s*\^+\s*$')
LINE_NUMBER = re.compile(r'\bline \d+\b')
BARE_PLACE = re.compile(r'(?<=: )\d+:\d+$')


def main(arguments):
    folders, arguments = take_folders(arguments)
    module_suites = MODULE_SUITES + tuple((os.path.basename(folder), os.path.basename(folder)) for folder in folders)
    names = [suite for suite, _ in CASE_SUITES + module_suites]
    unknown = [argument for argument in arguments if argument not in names]
    if unknown:
        raise Failure('brix brix-tests takes suite names, from %s, and --with FOLDER to add a folder of '
                      'test modules as a suite; not %s' % (', '.join(names), ', '.join(unknown)))
    chosen = set(arguments or names)
    started = time.time()
    compiler.ensure_compiler(compiler.seed_name())
    converter = compiler.ensure_tool('brix')
    suite_runner = compiler.ensure_tool('test-suite')
    module_runner = compiler.ensure_tool('test')
    environment = compiler.compiler_environment()
    shutil.rmtree(WORK, ignore_errors=True)
    os.makedirs(TREE)
    sources = [path for path in brix_sweep.brix_sources() if path.startswith(TREE_FOLDERS)]
    say('Converting %d library files to Brix' % len(sources))
    brix_sweep.convert(converter, sources, TREE, environment)
    copy_resources(ROOT, TREE_FOLDERS)
    for folder in folders:
        parent, name = os.path.split(folder)
        added = brix_sweep.brix_sources(parent, name)
        say('Converting %d files in %s to Brix' % (len(added), name))
        brix_sweep.convert(converter, added, TREE, environment, parent)
        copy_resources(parent, (name + '/',))
    files = []
    for suite, root in CASE_SUITES:
        if suite in chosen:
            files += [TestFile(suite, path) for path in test_paths(root)]
    say('Converting the cases of %d test files to Brix' % len(files))
    convert_cases(converter, files, environment)
    for test_file in files:
        test_file.write()
    report = {'suites': {}, 'unconverted': [], 'problems': [], 'adopted': [], 'failures': []}
    for test_file in files:
        report['unconverted'] += ['%s:%d' % (test_file.path, case.line) for case in test_file.cases if case.unconverted]
        report['problems'] += ['%s:%d: %s' % (test_file.path, case.line, problem)
                               for case in test_file.cases for problem in case.problems]
    running = dict(environment, BRIXPATH=os.path.join(TREE, 'stdlib'))
    for suite, root in CASE_SUITES:
        if suite in chosen:
            suite_files = [test_file for test_file in files if test_file.suite == suite]
            run_case_suite(suite, root, suite_files, suite_runner, running, report)
    for suite, folder in module_suites:
        if suite in chosen:
            run_module_suite(suite, folder, module_runner, running, report)
    with open(REPORT, 'w') as file:
        json.dump(report, file, indent=2)
    show(report, time.time() - started)
    return 1 if report['failures'] else 0


def copy_resources(root, folders):
    listed = subprocess.run(
        ['git', 'ls-files', '-z', '--cached', '--others', '--exclude-standard', '--'] + list(folders),
        cwd=root,
        stdout=subprocess.PIPE,
        check=True,
    ).stdout.decode()
    for path in listed.split('\0'):
        if not path or path.endswith(LEFT_IN_PLACE) or not os.path.isfile(os.path.join(root, path)):
            continue
        target = os.path.join(TREE, path)
        os.makedirs(os.path.dirname(target), exist_ok=True)
        shutil.copy2(os.path.join(root, path), target)


def test_paths(root):
    found = []
    for folder, _, names in os.walk(os.path.join(ROOT, root)):
        found += [os.path.relpath(os.path.join(folder, name), ROOT) for name in names if name.endswith(SPEC_EXTENSION)]
    return sorted(found)


class Fence:
    def __init__(self, line, opening, content, closing):
        self.line = line
        self.opening = opening
        self.content = content
        self.closing = closing
        self.words = opening.lstrip('`').split()
        self.replaced = None
        self.dropped = False

    @property
    def kind(self):
        return self.words[0] if self.words else ''

    @property
    def file(self):
        return next((word[len('file='):] for word in self.words if word.startswith('file=')), None)

    @property
    def body(self):
        return ''.join(self.content).removesuffix('\n')

    def replace(self, words, body):
        self.replaced = (words, body)

    def text(self):
        if self.dropped:
            return ''
        if self.replaced is None:
            return self.opening + ''.join(self.content) + self.closing
        words, body = self.replaced
        ticks = fence_ticks(body)
        lines = [ticks + ' '.join(words) + '\n']
        if body:
            lines.append(body + '\n')
        return ''.join(lines + [ticks + '\n'])


class Case:
    def __init__(self, program):
        self.program = program
        self.twin = None
        self.files = []
        self.errors = None
        self.output = None
        self.brix = None
        self.converted = {}
        self.unconverted = False
        self.problems = []

    @property
    def line(self):
        return self.program.line

    def has_flag(self, name):
        return name in self.program.words[1:]

    def fences(self):
        return [fence for fence in [self.program, self.twin, self.errors, self.output] + self.files if fence]

    def settle(self):
        if self.unconverted:
            for fence in self.fences():
                fence.dropped = True
            return
        if self.twin:
            self.twin.dropped = True
        if self.brix is not None:
            self.program.replace(['brix'] + self.program.words[1:], self.brix)
        for fence in self.files:
            if fence.file in self.converted:
                fence.replace(['brix', 'file=%s.bx' % fence.file[:-len('.yumi')]], self.converted[fence.file])


class TestFile:
    def __init__(self, suite, path):
        self.suite = suite
        self.path = path
        with open(os.path.join(ROOT, path)) as file:
            self.lines = file.read().splitlines(True)
        self.items = []
        self.cases = []
        self.parse()

    @property
    def kept(self):
        return [case for case in self.cases if not case.unconverted]

    def parse(self):
        lines = self.lines
        current = None
        stage = None
        index = 0
        while index < len(lines):
            line = lines[index]
            opening = OPENING.match(line)
            if opening:
                closing = LONG_CLOSING if opening.group(1) == '````' else CLOSING
                end = index + 1
                while end < len(lines) and not closing.match(lines[end]):
                    end += 1
                if end == len(lines):
                    raise Failure('%s:%d: The fence is never closed' % (self.path, index + 1))
                fence = Fence(index + 1, line, lines[index + 1:end], lines[end])
                self.items.append(fence)
                current, stage = self.take(fence, current, stage)
                index = end + 1
                continue
            if HEADING.match(line):
                current = None
                stage = None
            self.items.append(line)
            index += 1

    def take(self, fence, current, stage):
        if fence.file is not None:
            current.files.append(fence)
            return current, 'file'
        if fence.kind in PROGRAMS:
            if current and stage == 'program' and current.program.kind != fence.kind:
                current.twin = fence
                return current, 'twin'
            case = Case(fence)
            self.cases.append(case)
            return case, 'program'
        if fence.kind == 'stderr':
            current.errors = fence
            return current, 'errors'
        if fence.kind == 'output':
            current.output = fence
            return current, 'output'
        return current, stage

    def jobs(self):
        for case in self.cases:
            if case.program.kind == 'yumi':
                if case.twin:
                    case.brix = case.twin.body.strip()
                elif self.suite == PARSE_SUITE:
                    case.unconverted = True
                    continue
                else:
                    yield (case, None, case.program.body)
            for fence in case.files:
                if fence.file.endswith('.yumi'):
                    yield (case, fence.file, fence.body)

    def write(self):
        for case in self.cases:
            case.settle()
        target = os.path.join(TREE, self.path)
        os.makedirs(os.path.dirname(target), exist_ok=True)
        with open(target, 'w') as file:
            file.write(''.join(item if isinstance(item, str) else item.text() for item in self.items))


def fence_ticks(body):
    lines = body.split('\n')
    if any(line.startswith('````') for line in lines):
        return None
    return '````' if any(line.startswith('```') for line in lines) else '```'


def convert_cases(converter, files, environment):
    jobs = [job for test_file in files for job in test_file.jobs()]
    os.makedirs(SOURCES)
    names = []
    for number, (_, _, source) in enumerate(jobs):
        name = '%d.yumi' % number
        with open(os.path.join(SOURCES, name), 'w') as file:
            file.write(source)
        names.append(name)
    count = max(1, min(WORKERS, len(names)))
    chunks = [names[index::count] for index in range(count)]
    writing = dict(environment, BRIX_OUT=CONVERTED)
    with ThreadPoolExecutor(count) as pool:
        results = list(pool.map(lambda chunk: convert_chunk(converter, chunk, writing), chunks))
    messages = {}
    for lines in results:
        for line in lines:
            name = line.split(':', 1)[0]
            messages.setdefault(name, []).append(line[len(name):].lstrip(':0123456789 '))
    for number, (case, file, _) in enumerate(jobs):
        name = '%d.yumi' % number
        converted = os.path.join(CONVERTED, '%d.bx' % number)
        case.problems += messages.get(name, [])
        if not os.path.isfile(converted):
            case.unconverted = True
            continue
        with open(converted) as handle:
            text = handle.read().strip('\n')
        if fence_ticks(text) is None:
            case.problems.append('its Brix holds a line that would close its fence')
            case.unconverted = True
        elif file:
            case.converted[file] = text
        else:
            case.brix = text


def convert_chunk(converter, chunk, environment):
    result = subprocess.run(
        [converter, '--sweep'] + chunk,
        cwd=SOURCES,
        env=environment,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        universal_newlines=True,
    )
    lines = result.stdout.splitlines()
    if not any(line.startswith(brix_sweep.SUMMARY) for line in lines):
        raise Failure('The converter stopped with status %d before it finished:\n%s' % (
            result.returncode, '\n'.join(lines[-20:])))
    return [line for line in lines if not line.startswith(brix_sweep.SUMMARY)]


def suite_environment(environment, suite, root):
    return dict(
        environment,
        OMETER_TESTOMETER_PROJECT_ROOT=TREE,
        OMETER_TESTOMETER_SUITE_NAME=suite,
        OMETER_TESTOMETER_CATALOG_START_INDEX='0',
        BRIX_TEST_SUITE=suite,
        BRIX_TEST_ROOT=os.path.join(TREE, root),
        BRIX_TEST_COMPILER=compiler.COMPILER,
    )


def run_case_suite(suite, root, files, runner, environment, report):
    say('Running %s from Brix' % suite)
    variables = suite_environment(environment, suite, root)
    results = run_suite(suite, [runner, 'catalog'], [runner, 'run'], variables)
    cases = {}
    for test_file in files:
        for number, case in enumerate(test_file.kept):
            cases['test:%s:%s:%d' % (suite, test_file.path, number + 1)] = (test_file, case)
    adopted = []
    for result in results.values():
        found = cases.get(result['id'])
        if result['status'] == 'failed' and found and adoptable(result) and adopt(found[1], result['actual']):
            adopted.append(result['id'])
    if adopted:
        for test_file in set(cases[identifier][0] for identifier in adopted):
            test_file.write()
        say('Running the %d cases of %s whose places in their source moved' % (len(adopted), suite))
        rerun = run_suite(suite, None, [runner, 'run'], variables, adopted)
        results.update(rerun)
    report['adopted'] += ['%s:%d' % (cases[identifier][0].path, cases[identifier][1].line) for identifier in adopted]
    record(suite, results, report, cases)


def run_module_suite(suite, folder, runner, environment, report):
    say('Running %s from Brix' % suite)
    variables = suite_environment(environment, suite, folder)
    results = run_suite(suite, [runner, 'catalog-modules', folder], [runner, 'run-modules', folder], variables)
    record(suite, results, report, {})


def run_suite(suite, catalog_command, run_command, environment, identifiers=None):
    if identifiers is None:
        listed = subprocess.run(catalog_command, cwd=TREE, env=environment, stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE, universal_newlines=True)
        if listed.returncode != 0:
            raise Failure('Listing %s from Brix failed:\n%s' % (suite, listed.stderr.strip()))
        identifiers = [test['id'] for test in json.loads(listed.stdout).get('tests', [])]
        if not identifiers:
            raise Failure('%s lists no tests in its Brix' % suite)
    selected = dict(environment, OMETER_TESTOMETER_SELECTED_TEST_IDS=json.dumps(identifiers))
    ran = subprocess.run(run_command, cwd=TREE, env=selected, stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE, universal_newlines=True)
    results = {}
    for line in ran.stdout.splitlines():
        if not line.startswith('{'):
            continue
        try:
            event = json.loads(line)
        except ValueError:
            continue
        if event.get('type') == 'testResult':
            results[event['id']] = event
    for identifier in identifiers:
        results.setdefault(identifier, {
            'id': identifier, 'status': 'killed', 'errors': ran.stderr.strip()[-2000:]})
    return results


def record(suite, results, report, cases):
    counts = {}
    for result in results.values():
        counts[result['status']] = counts.get(result['status'], 0) + 1
        if result['status'] in FAILING:
            found = cases.get(result['id'])
            place = '%s:%d' % (found[0].path, found[1].line) if found else result['id']
            case = found[1] if found else None
            report['failures'].append({
                'suite': suite,
                'place': place,
                'kind': failure_kind(case),
                'problems': case.problems if case else [],
                'status': result['status'],
                'expected': result.get('expected', ''),
                'actual': result.get('actual', ''),
                'errors': result.get('errors', ''),
            })
    report['suites'][suite] = counts


def failure_kind(case):
    if case and any(case.has_flag(name) for name in EXPECTS_DIAGNOSTIC):
        return 'diagnostic'
    return 'shape' if case and case.problems else 'other'


def adoptable(result):
    return shape(result.get('expected', '')) == shape(result.get('actual', ''))


def shape(text):
    tokens = []
    quoting = False
    for line in text.strip().split('\n'):
        if quoting and (line.startswith('    ') or CARET.match(line)):
            if tokens[-1] != ('quote',):
                tokens.append(('quote',))
            continue
        quoting = bool(PLACE_LINE.match(line.strip()))
        placed = BARE_PLACE.sub('L', PLACE.sub(placeless, LINE_NUMBER.sub('line L', line)))
        tokens.append(('line', SOURCE_NAME.sub(r'\1.src', placed)))
    return tokens


def placeless(match):
    name = match.group(1)
    return '%s:L' % SOURCE_NAME.sub(r'\1.src', name)


def adopt(case, actual):
    whole = actual.strip()
    if case.errors and case.output:
        printed = case.output.body.strip()
        if not whole.endswith('\n' + printed):
            return False
        whole = whole[:-len(printed) - 1].rstrip()
    fence = case.errors or case.output
    if not fence or fence_ticks(whole) is None:
        return False
    fence.replace([fence.kind], whole)
    return True


def show(report, seconds):
    for suite, counts in report['suites'].items():
        say('%s: %s' % (suite, ', '.join('%d %s' % (count, status) for status, count in sorted(counts.items()))))
    say('%d cases have no Brix and are left out, %d convert with problems, %d took the places their Brix moved' % (
        len(report['unconverted']), len(report['problems']), len(report['adopted'])))
    for kind, label in KINDS:
        count = len([failure for failure in report['failures'] if failure['kind'] == kind])
        if count:
            say('%d failures: %s' % (count, label))
    shown = [failure for failure in report['failures'] if failure['kind'] == 'other']
    for failure in report['failures']:
        if failure['kind'] == 'shape':
            say('  %s: %s' % (failure['place'], failure['problems'][0][:160]))
    for failure in shown[:SHOWN_FAILURES]:
        say('\n%s %s %s' % (failure['status'], failure['suite'], failure['place']))
        for label in ('expected', 'actual', 'errors'):
            lines = failure[label].strip().split('\n')
            if failure[label].strip():
                say('  %s\n%s' % (label, '\n'.join('    ' + line for line in lines[:SHOWN_LINES])))
    more = len(shown) - SHOWN_FAILURES
    if more > 0:
        say('\n…and %d more failures' % more)
    say('\n%d failures in %.1f s; the report is %s' % (len(report['failures']), seconds, os.path.relpath(REPORT)))
