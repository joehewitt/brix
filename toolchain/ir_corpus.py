import concurrent.futures
import glob
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile
import threading
import time

sys.path.insert(0, os.path.dirname(os.path.realpath(__file__)))

import compiler as builder
from common import BUILD, ROOT, TMP, Failure, repository_root, run_main, say, take_folders

STATE = os.path.join(TMP, 'restructure')
CORPUS = os.path.join(STATE, 'corpus')
REVISION = os.path.join(STATE, 'corpus-revision')
BASELINE_COMPILER = os.path.join(STATE, 'baseline-brix')
BASELINE = os.path.join(STATE, 'baseline')
CURRENT = os.path.join(STATE, 'current')
CURRENT_COMPILER = os.path.join(STATE, 'current-brix')
SOURCES = os.path.join(STATE, 'sources')
FOLDERS = os.path.join(STATE, 'corpus-folders')
TOP = repository_root()
PREFIX = os.path.relpath(ROOT, TOP)


def toolchain_path(*parts):
    return os.path.normpath(os.path.join(PREFIX, *parts))


SNAPSHOT_PATHS = tuple(toolchain_path(path) for path in ('stdlib', 'apps/compiler', 'examples'))
COMPILER_PATHS = tuple(toolchain_path(path) for path in ('stdlib', 'apps/compiler'))
COMPILER_SOURCE = toolchain_path('apps', 'compiler', 'brix-cli.yumi')
STDLIB = toolchain_path('stdlib')
APP_PATTERNS = ('*-app.yumi', '*-app.bx')
FULL = 'ir'
QUICK = 'emitted'
EXAMPLE_PATTERNS = tuple(toolchain_path('examples', pattern) for pattern in (
    '*/*.yumi',
    '*/*.bx',
    '*/*/*-app.yumi',
    '*/*/*-app.bx',
))
FUNCTION_START = re.compile(rb'^function (.+?) kind=', re.MULTILINE)
FUNCTION_LINE = b'function '
NEWLINE_FUNCTION = b'\n' + FUNCTION_LINE
READ_SIZE = 1 << 22
LEADING_KEPT = 1 << 14
JOBS = max(2, (os.cpu_count() or 4) // 4)
TIMINGS = os.path.join(STATE, 'timings')
TIMING_RUNS = 3
ALTERNATED_RUNS = 5
TIMING_LINE = re.compile(r'^([a-z][a-z-]*) ([0-9.]+) ?(µs|us|ms|s)?$')
MILLISECONDS = {'µs': 0.001, 'us': 0.001, 'ms': 1.0, 's': 1000.0, None: 0.001}
SHOWN_CHANGES = 20
USAGE = '''usage: python3 toolchain/ir_corpus.py COMMAND [--with FOLDER] [--quick] [--jobs N]

Checks that a change to the compiler leaves the IR it emits identical. The corpus is a snapshot of
stdlib, the compiler, examples and the folders named with --with, so the compiler's own sources can
change without changing what it compiles. Functions are matched by name: the order the compiler
prints them in varies from run to run, and is not compared.

Each command that needs a compiler first copies the compiler's sources, then builds from the copy,
so the sources can be edited again as soon as it says it copied them.

  snapshot [REVISION]   snapshot the corpus at REVISION (HEAD by default); each --with FOLDER adds
                        a folder, whose *-app programs join the corpus
  baseline [REVISION]   keep the compiler built from the current sources, or from REVISION's, and
                        record its IR both ways
  check                 build the current compiler, record its IR and compare it with the baseline
  diff PROGRAM FUNCTION print how one function's IR differs between the baseline and the compiler
                        the last check built
  timing save NAME      time cold builds of the compiler and the first program a --with folder
                        added, from the corpus, and save them
  timing compare NAME   time them again, compare with the timings saved as NAME, and save as last
  timing baseline       time the baseline compiler and the current one in alternating runs, so
                        that a change in load reaches both, and compare them
  programs              list the corpus programs

  --quick               compare only the IR, not the backend's stages after it (--inspect emitted)
  --jobs N              record N programs at once (%d by default)
''' % JOBS


def main(arguments):
    if not arguments or arguments[0] in ('-h', '--help'):
        sys.stdout.write(USAGE)
        return 0 if arguments else 1
    quick = '--quick' in arguments
    jobs = JOBS
    if '--jobs' in arguments:
        position = arguments.index('--jobs')
        if position + 1 >= len(arguments) or not arguments[position + 1].isdigit():
            raise Failure(USAGE.rstrip())
        jobs = int(arguments[position + 1])
        arguments = arguments[:position] + arguments[position + 2:]
    arguments = [argument for argument in arguments if argument != '--quick']
    found, arguments = take_folders(arguments)
    folders = [repository_path(folder) for folder in found]
    if not arguments:
        raise Failure(USAGE.rstrip())
    command, rest = arguments[0], arguments[1:]
    if command == 'snapshot':
        return snapshot(rest[0] if rest else 'HEAD', folders)
    if folders:
        raise Failure('Only snapshot takes --with; the corpus keeps the folders it was taken with')
    if command == 'programs':
        for program in programs():
            print(program)
        return 0
    if command == 'baseline' and len(rest) <= 1:
        return baseline(rest[0] if rest else None, jobs)
    if command == 'check':
        return check(QUICK if quick else FULL, jobs)
    if command == 'diff' and len(rest) == 2:
        return diff(rest[0], rest[1])
    if command == 'timing' and len(rest) == 2 and rest[0] in ('save', 'compare'):
        return timing(rest[0], rest[1])
    if command == 'timing' and rest == ['baseline']:
        return against_baseline()
    raise Failure(USAGE.rstrip())


def repository_path(folder):
    relative = os.path.relpath(folder, TOP)
    if relative == os.pardir or relative.startswith(os.pardir + os.sep):
        raise Failure('%s is outside the repository at %s' % (folder, TOP))
    return relative


def snapshot(revision, folders):
    resolved = git('rev-parse', revision).strip()
    shutil.rmtree(CORPUS, ignore_errors=True)
    paths = SNAPSHOT_PATHS + tuple(folders)
    extract(resolved, paths, CORPUS)
    with open(REVISION, 'w') as file:
        file.write(resolved + '\n')
    with open(FOLDERS, 'w') as file:
        json.dump(folders, file)
    say('Snapshot of %s at %s, %d programs' % (', '.join(paths), resolved[:12], len(programs())))
    return 0


def extract(revision, paths, directory):
    os.makedirs(directory, exist_ok=True)
    archive = subprocess.Popen(
        ['git', 'archive', '--format=tar', revision] + list(paths),
        cwd=TOP,
        stdout=subprocess.PIPE,
    )
    try:
        with tarfile.open(fileobj=archive.stdout, mode='r|') as tar:
            tar.extractall(directory, filter='data')
    except tarfile.ReadError as error:
        if archive.wait() == 0:
            raise Failure('git archive %s gave an archive tar cannot read: %s' % (revision, error))
    if archive.wait() != 0:
        raise Failure('git archive %s failed' % revision)


def programs():
    if not os.path.isdir(CORPUS):
        raise Failure('No corpus; run: python3 toolchain/ir_corpus.py snapshot')
    found = [COMPILER_SOURCE] if os.path.isfile(os.path.join(CORPUS, COMPILER_SOURCE)) else []
    found += added_programs()
    for pattern in EXAMPLE_PATTERNS:
        found += sorted(os.path.relpath(path, CORPUS) for path in glob.glob(os.path.join(CORPUS, pattern)))
    return found


def added_programs():
    found = []
    for folder in corpus_folders():
        for pattern in APP_PATTERNS:
            found += sorted(os.path.relpath(path, CORPUS) for path in glob.glob(os.path.join(CORPUS, folder, pattern)))
    return found


def corpus_folders():
    if not os.path.isfile(FOLDERS):
        return []
    with open(FOLDERS) as file:
        return json.load(file)


def timed():
    return [COMPILER_SOURCE] + added_programs()[:1]


def baseline(revision, jobs):
    built_compiler(BASELINE_COMPILER, revision)
    say('Kept the compiler built from %s as the baseline' % (
        git('rev-parse', '--short', revision).strip() if revision else 'the current sources'))
    record_baseline(jobs)
    return 0


def record_baseline(jobs):
    shutil.rmtree(BASELINE, ignore_errors=True)
    for mode in (QUICK, FULL):
        record(BASELINE_COMPILER, BASELINE, mode, jobs)


def check(mode, jobs):
    if not os.path.isfile(BASELINE_COMPILER):
        raise Failure('No baseline; run: python3 toolchain/ir_corpus.py baseline')
    built_compiler(CURRENT_COMPILER)
    listed = programs()
    record(CURRENT_COMPILER, CURRENT, mode, jobs)
    changed = 0
    for program in listed:
        before = read_digests(BASELINE, mode, program)
        after = read_digests(CURRENT, mode, program)
        if before is None:
            say('%s: no baseline' % program)
            changed += 1
            continue
        differences = compare(before, after)
        if differences:
            changed += 1
            say('%s: %d functions differ' % (program, len(differences)))
            for line in differences[:SHOWN_CHANGES]:
                say('  ' + line)
            if len(differences) > SHOWN_CHANGES:
                say('  …and %d more' % (len(differences) - SHOWN_CHANGES))
    if changed:
        say('%d of %d programs emit different IR' % (changed, len(listed)))
        return 1
    say('All %d programs emit identical IR' % len(listed))
    return 0


def diff(program, function):
    if not os.path.isfile(BASELINE_COMPILER):
        raise Failure('No baseline; run: python3 toolchain/ir_corpus.py baseline')
    if not os.path.isfile(CURRENT_COMPILER):
        raise Failure('No compiler from a check; run: python3 toolchain/ir_corpus.py check')
    before = function_text(BASELINE_COMPILER, program, function)
    after = function_text(CURRENT_COMPILER, program, function)
    import difflib
    lines = difflib.unified_diff(
        before.splitlines(keepends=True),
        after.splitlines(keepends=True),
        'baseline/%s' % function,
        'current/%s' % function,
    )
    sys.stdout.writelines(lines)
    return 0


def timing(action, name):
    path = os.path.join(TIMINGS, name + '.json')
    saved = None
    if action == 'compare':
        if not os.path.isfile(path):
            raise Failure('No timings saved as %s' % name)
        with open(path) as file:
            saved = json.load(file)
    measured = measure(built_compiler(CURRENT_COMPILER), TIMING_RUNS)
    os.makedirs(TIMINGS, exist_ok=True)
    with open(path if action == 'save' else os.path.join(TIMINGS, 'last.json'), 'w') as file:
        json.dump(measured, file, indent=2)
    for program, phases in measured.items():
        say(program)
        for phase, milliseconds in phases.items():
            before = saved.get(program, {}).get(phase) if saved else None
            if before:
                say('  %-15s %9.1f ms  %9.1f ms  %+6.1f%%' % (
                    phase, before, milliseconds, (milliseconds - before) * 100 / before))
            else:
                say('  %-15s %9.1f ms' % (phase, milliseconds))
    return 0


def against_baseline():
    if not os.path.isfile(BASELINE_COMPILER):
        raise Failure('No baseline; run: python3 toolchain/ir_corpus.py baseline')
    current = built_compiler(CURRENT_COMPILER)
    order = [BASELINE_COMPILER, current]
    samples = {compiler: {} for compiler in order}
    for run in range(ALTERNATED_RUNS):
        for compiler in order if run % 2 == 0 else order[::-1]:
            for program, phases in measure(compiler, 1).items():
                for phase, milliseconds in phases.items():
                    samples[compiler].setdefault(program, {}).setdefault(phase, []).append(milliseconds)
    say('Medians of %d runs each, baseline then current' % ALTERNATED_RUNS)
    for program in timed():
        say(program)
        for phase, values in samples[current][program].items():
            before = median(samples[BASELINE_COMPILER][program].get(phase, [0]))
            after = median(values)
            change = (after - before) * 100 / before if before else 0
            say('  %-15s %9.1f ms  %9.1f ms  %+6.1f%%' % (phase, before, after, change))
    return 0


def median(values):
    return sorted(values)[len(values) // 2]


def measure(compiler, runs):
    output = os.path.join(STATE, 'timed')
    os.makedirs(output, exist_ok=True)
    measured = timed()
    samples = {program: {} for program in measured}
    for run in range(runs):
        for program in measured:
            name = os.path.splitext(os.path.basename(program))[0]
            result = subprocess.run(
                [compiler, os.path.join(CORPUS, program), '--compile', '-o', os.path.join(output, name), '--timings'],
                cwd=ROOT,
                env=environment(),
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                universal_newlines=True,
            )
            phases = {}
            for line in result.stderr.splitlines():
                found = TIMING_LINE.match(line.strip())
                if found:
                    phases[found.group(1)] = float(found.group(2)) * MILLISECONDS[found.group(3)]
            if result.returncode != 0 or 'total' not in phases:
                raise Failure('%s did not build:\n%s' % (program, (result.stderr + result.stdout).strip()[-2000:]))
            for phase, milliseconds in phases.items():
                samples[program].setdefault(phase, []).append(milliseconds)
            say('[%d/%d] %s: %.0f ms' % (run + 1, runs, program, phases['total']))
    return {program: {phase: median(values) for phase, values in phases.items()}
            for program, phases in samples.items()}


def built_compiler(destination, revision=None):
    copy_sources(revision)
    return build_sources(destination)


def copy_sources(revision):
    started = time.time()
    shutil.rmtree(SOURCES, ignore_errors=True)
    if revision:
        extract(revision, COMPILER_PATHS, SOURCES)
        say('Took the compiler sources from %s' % revision)
        return
    for path in COMPILER_PATHS:
        shutil.copytree(os.path.join(TOP, path), os.path.join(SOURCES, path),
                        ignore=shutil.ignore_patterns('.*'))
    say('Copied the compiler sources in %.1f s; edits from here on are not in this run' % (time.time() - started))


def build_sources(destination):
    started = time.time()
    environment = builder.compiler_environment()
    builder.build_runtime(environment)
    environment['BRIXPATH'] = os.path.join(SOURCES, STDLIB)
    source = os.path.join(SOURCES, COMPILER_SOURCE)
    seed = builder.seed_command()
    temporary = tempfile.mkdtemp(prefix='.build-', dir=STATE)
    try:
        bootstrap = os.path.join(temporary, 'brix-bootstrap')
        executable = os.path.join(temporary, 'brix')
        builder.run([seed, '--compile', '-o', bootstrap, source], environment)
        say('Built the bootstrap compiler in %.0f s' % (time.time() - started))
        builder.run([bootstrap, '--compile', '-o', executable, source], environment)
        os.replace(executable, destination)
    finally:
        shutil.rmtree(temporary, ignore_errors=True)
    say('Built the compiler in %.0f s' % (time.time() - started))
    return destination


def environment():
    passed = dict(os.environ)
    passed['BRIXPATH'] = os.path.join(CORPUS, STDLIB)
    passed['BRIX_BUILD_PATH'] = BUILD
    passed.pop('BRIX_DEBUG_LOG', None)
    return passed


def record(compiler, directory, mode, jobs):
    shutil.rmtree(os.path.join(directory, mode), ignore_errors=True)
    started = time.time()
    listed = programs()
    weights = {program: recorded_size(mode, program) for program in listed}
    ordered = sorted(listed, key=lambda program: -weights[program])
    failed = threading.Event()
    finished = []

    def recorded(program):
        if failed.is_set():
            return
        begun = time.time()
        try:
            digests = dump(compiler, program, mode)
        except Failure:
            failed.set()
            raise
        path = digest_path(directory, mode, program)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, 'w') as file:
            for name, digest in digests:
                file.write('%s\t%s\n' % (name, digest))
        finished.append(program)
        say('[%d/%d] %s: %d functions in %.1f s' % (
            len(finished), len(listed), program, len(digests), time.time() - begun))

    with concurrent.futures.ThreadPoolExecutor(jobs) as pool:
        futures = [pool.submit(recorded, program) for program in ordered]
    failures = [str(future.exception()) for future in futures if future.exception()]
    if failures:
        raise Failure('\n'.join(failures))
    say('Recorded %d programs with --inspect %s in %.0f s, %d at once' % (
        len(listed), mode, time.time() - started, jobs))


def recorded_size(mode, program):
    for recorded in (mode, QUICK, FULL):
        path = digest_path(BASELINE, recorded, program)
        if os.path.isfile(path):
            return os.path.getsize(path)
    return float('inf')


def dump(compiler, program, mode):
    process = subprocess.Popen(
        [compiler, os.path.join(CORPUS, program), '--inspect', mode],
        cwd=ROOT,
        env=environment(),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    digests = []
    seen = {}
    name = None
    digest = None
    leading = b''
    pending = b''
    while True:
        chunk = process.stderr.read(READ_SIZE)
        if chunk:
            data = pending + chunk
            end = data.rfind(b'\n') + 1
            pending = data[end:]
        else:
            data = pending
            end = len(data)
        view = memoryview(data)
        start = 0
        for found in function_starts(data, end):
            if digest is None:
                leading = (leading + data[start:found.start()])[-LEADING_KEPT:]
            else:
                digest.update(view[start:found.start()])
                digests.append((name, digest.hexdigest()))
            base = found.group(1).decode()
            seen[base] = seen.get(base, 0) + 1
            name = base if seen[base] == 1 else '%s#%d' % (base, seen[base])
            digest = hashlib.sha256()
            start = found.start()
        if digest is None:
            leading = (leading + data[start:end])[-LEADING_KEPT:]
        else:
            digest.update(view[start:end])
        view.release()
        if not chunk:
            break
    if name is not None:
        digests.append((name, digest.hexdigest()))
    output = process.stdout.read()
    if process.wait() != 0 or not digests:
        message = (b'\n'.join(leading.splitlines()[-20:]) + b'\n' + output).decode(errors='replace').strip()
        raise Failure('%s did not compile:\n%s' % (program, message))
    return digests


def function_starts(data, end):
    line = 0
    while line < end:
        found = FUNCTION_START.match(data, line, end)
        if found:
            yield found
        line = data.find(NEWLINE_FUNCTION, line, end) + 1
        if not line:
            return


def function_text(compiler, program, function):
    process = subprocess.Popen(
        [compiler, os.path.join(CORPUS, program), '--inspect', FULL],
        cwd=ROOT,
        env=environment(),
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
    )
    kept = []
    base, _, ordinal = function.partition('#')
    wanted = int(ordinal) if ordinal else 1
    count = 0
    inside = False
    for line in process.stderr:
        started = FUNCTION_START.match(line)
        if started:
            inside = False
            if started.group(1).decode() == base:
                count += 1
                inside = count == wanted
        if inside:
            kept.append(line)
    process.wait()
    return b''.join(kept).decode(errors='replace')


def digest_path(directory, mode, program):
    return os.path.join(directory, mode, program + '.digests')


def read_digests(directory, mode, program):
    path = digest_path(directory, mode, program)
    if not os.path.isfile(path):
        return None
    with open(path) as file:
        return [tuple(line.rstrip('\n').split('\t')) for line in file if line.strip()]


def compare(before, after):
    old = grouped(before)
    new = grouped(after)
    lines = []
    for name, digests in old.items():
        if name not in new:
            lines.append('removed %s' % name)
        elif sorted(new[name]) != sorted(digests):
            lines.append('changed %s' % name)
    for name in new:
        if name not in old:
            lines.append('added %s' % name)
    return lines


def grouped(digests):
    groups = {}
    for name, digest in digests:
        groups.setdefault(name.partition('#')[0], []).append(digest)
    return groups


def git(*arguments):
    return subprocess.run(['git'] + list(arguments), cwd=ROOT, stdout=subprocess.PIPE, check=True,
                          universal_newlines=True).stdout


if __name__ == '__main__':
    sys.exit(run_main(main))
