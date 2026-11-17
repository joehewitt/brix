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
LEFT_IN_PLACE = ('.yumi', '.test')
PARSE_SUITE = 'parse'
CASE_SUITES = (
    (PARSE_SUITE, 'stdlib/lang/tests/parse'),
    ('core', 'stdlib/core/tests'),
    ('diagnostics', 'stdlib/lang/tests/diagnostics'),
)
MODULE_SUITES = (('stdlib', 'stdlib'),)
FAILING = ('failed', 'crashed', 'killed')
EXPECTS_DIAGNOSTIC = ('compile-error', 'crash', 'warnings', 'warn')
KINDS = (
    ('shape', 'the converter makes Brix the reader rejects, or reads back differently'),
    ('diagnostic', 'a case that expects a diagnostic, which Brix reports otherwise'),
    ('other', 'other failures'),
)
SHOWN_FAILURES = 30
SHOWN_LINES = 12
WORKERS = os.cpu_count() or 4
HEAD = re.compile(r'::?===+\n?$')
MID = re.compile(r':---+\n$')
TAIL = re.compile(r';===+')
COMMENT = re.compile(r'===+\n$')
IGNORED = re.compile(r'/:===')
NOTE = re.compile(r'#[ \t\r\v\f]')
BRIX_BLOCK = re.compile(r'%brix[ \t]*\n$')
PLACE = re.compile(r'(no file|[\w./+-]+\.(?:yumi|bx)):\d+(?::\d+)?')
SOURCE_NAME = re.compile(r'([\w./+-]+)\.(?:yumi|bx)\b')
PLACE_LINE = re.compile(r'<[^<>]*:\d+>$')
CARET = re.compile(r'\s*\^+\s*$')
LINE_NUMBER = re.compile(r'\bline \d+\b')
BARE_PLACE = re.compile(r'(?<=: )\d+:\d+$')
SKIPPED = '# This case does not convert to Brix.\n'


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
    running = dict(environment, BRIXPATH=os.path.join(TREE, 'stdlib'), BRIX_TEST_BRIX='1')
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
        found += [os.path.relpath(os.path.join(folder, name), ROOT) for name in names if name.endswith('.test')]
    return sorted(found)


class Case:
    def __init__(self, line):
        self.line = line
        self.head = ''
        self.notes = []
        self.code = []
        self.mid = ''
        self.settings = []
        self.expected = []
        self.tail = ''
        self.brix = None
        self.unconverted = False
        self.problems = []
        self.files = {}

    @property
    def source(self):
        return ''.join(self.code)

    def has_option(self, name):
        return any(kind == 'option' and option_name(parts[0]) == name for kind, parts in self.settings)

    def text(self):
        lines = [self.head] + self.notes
        if self.unconverted:
            lines.append(SKIPPED)
        elif self.brix:
            lines.append(self.brix if self.brix.endswith('\n') else self.brix + '\n')
        lines.append(self.mid)
        if self.unconverted and not self.has_option('skip'):
            lines.append('%skip\n')
        for kind, parts in self.settings:
            if kind == 'brix':
                continue
            if kind == 'file':
                header, content, mid = parts
                name = file_name(header)
                if name in self.files:
                    converted = self.files[name]
                    header = '%%file: %s.bx\n' % name[:-len('.yumi')]
                    content = [converted if converted.endswith('\n') else converted + '\n'] if converted else []
                lines += [header] + content + [mid]
            else:
                lines += parts
        return ''.join(lines + self.expected + [self.tail])


class TestFile:
    def __init__(self, suite, path):
        self.suite = suite
        self.path = path
        with open(os.path.join(ROOT, path)) as file:
            self.lines = file.read().splitlines(True)
        self.items = []
        self.cases = []
        self.parse()

    def parse(self):
        lines = self.lines
        index = 0
        while index < len(lines):
            line = lines[index]
            if HEAD.match(line) and self.has_mid(index + 1):
                index = self.parse_case(index)
            elif COMMENT.match(line):
                end = index + 1
                while end < len(lines) and not COMMENT.match(lines[end]):
                    end += 1
                self.items.append(''.join(lines[index:end + 1]))
                index = end + 1
            elif IGNORED.match(line):
                self.items.append(''.join(lines[index:]))
                index = len(lines)
            else:
                self.items.append(line)
                index += 1

    def has_mid(self, index):
        return any(MID.match(line) for line in self.lines[index:])

    def parse_case(self, index):
        lines = self.lines
        case = Case(index + 1)
        case.head = lines[index]
        index += 1
        while not MID.match(lines[index]):
            if NOTE.match(lines[index]):
                case.notes.append(lines[index])
            else:
                case.code.append(lines[index])
            index += 1
        case.mid = lines[index]
        index += 1
        while index < len(lines):
            line = lines[index]
            if line.startswith('%file:') or BRIX_BLOCK.match(line):
                end = index + 1
                while not MID.match(lines[end]):
                    end += 1
                kind = 'brix' if BRIX_BLOCK.match(line) else 'file'
                case.settings.append((kind, (line, lines[index + 1:end], lines[end])))
                index = end + 1
            elif line.startswith('%'):
                case.settings.append(('option', [line]))
                index += 1
            else:
                break
        while index < len(lines) and not TAIL.match(lines[index]):
            case.expected.append(lines[index])
            index += 1
        if index < len(lines):
            case.tail = lines[index]
            index += 1
        self.items.append(case)
        self.cases.append(case)
        return index

    def jobs(self):
        for case in self.cases:
            if self.suite == PARSE_SUITE:
                brix = [parts for kind, parts in case.settings if kind == 'brix']
                if brix:
                    case.brix = ''.join(brix[0][1])
                else:
                    case.unconverted = True
                continue
            yield (case, None, case.source)
            for kind, parts in case.settings:
                if kind == 'file' and file_name(parts[0]).endswith('.yumi'):
                    yield (case, file_name(parts[0]), ''.join(parts[1]).removesuffix('\n'))

    def write(self):
        target = os.path.join(TREE, self.path)
        os.makedirs(os.path.dirname(target), exist_ok=True)
        with open(target, 'w') as file:
            file.write(''.join(item if isinstance(item, str) else item.text() for item in self.items))


def option_name(line):
    return line[1:].split(':', 1)[0].strip()


def file_name(header):
    return header[len('%file:'):].strip()


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
            text = handle.read()
        if any(MID.match(line) or TAIL.match(line) or HEAD.match(line) or NOTE.match(line)
               for line in text.splitlines(True)):
            case.problems.append('its Brix holds a line the test format reads as its own')
            case.unconverted = True
        elif file:
            case.files[file] = text
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
        for number, case in enumerate(test_file.cases):
            cases['test:%s:%s:%d' % (suite, test_file.path, number + 1)] = (test_file, case)
    adopted = []
    for result in results.values():
        found = cases.get(result['id'])
        if result['status'] == 'failed' and found and adoptable(found[1], result):
            adopt(found[1], result['actual'])
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
    if case and any(case.has_option(name) for name in EXPECTS_DIAGNOSTIC):
        return 'diagnostic'
    return 'shape' if case and case.problems else 'other'


def adoptable(case, result):
    if any(line.startswith('?') for line in case.expected):
        return False
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
    text = ''.join(case.expected)
    leading = text[:len(text) - len(text.lstrip())]
    trailing = text[len(text.rstrip()):]
    case.expected = [leading + actual.strip() + trailing]


def show(report, seconds):
    for suite, counts in report['suites'].items():
        say('%s: %s' % (suite, ', '.join('%d %s' % (count, status) for status, count in sorted(counts.items()))))
    say('%d cases have no Brix and are skipped, %d convert with problems, %d took the places their Brix moved' % (
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
