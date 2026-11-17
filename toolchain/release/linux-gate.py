#!/usr/bin/env python3
import concurrent.futures
import hashlib
import json
import lzma
import os
import re
import subprocess
import sys
import tarfile
import tempfile

HOST = 'aarch64-linux-gnu'
BUILD_HOST = 'arm64-apple-macos'
TIMEOUT = 600
TOOLS = ('probes', 'test', 'bench', 'lint')
TOOL_PROGRAM = 'stdlib'
TOOL_PROBE = 'joined 1+2+3'
TEST_RESULT = '1 test  1 passed'
BENCH_RESULT = '1 benchmark  1 passed'
GUIDE_INDEX = 'docs/index.md'
INIT_FILES = (('AGENTS.md', '`brix --help`'), ('CLAUDE.md', '@AGENTS.md'))
EXAMPLES = ('net', 'tui')
MACOS_EXAMPLES = ('examples/tui/chromatic/chromatic-app.bx',)
EXAMPLE_SOURCES = ('.yumi', '.bx')
EXAMPLE_IMPORT = re.compile(r'^\+? ?(\.{1,2}/\S+)$')
BRIX_EXAMPLE_IMPORT = re.compile(r'^\s*\((?:import|export) (\.{1,2}/[^\s)]+)\)$')


class GateFailure(Exception):
    pass


def main(arguments):
    root, version = arguments
    home = os.path.realpath(os.path.expanduser('~/.brix'))
    mirror = os.path.join(root, 'mirror')
    say('Installing %s from %s' % (version, mirror))
    run(['python3', os.path.join(mirror, 'install.py'), '--from', mirror, '--version', version, '--yes',
         '--no-modify-path'])
    brix = os.path.join(home, 'bin', 'brix')
    with open(os.path.join(home, 'archives', version, 'release.json')) as file:
        release = json.load(file)
    host = release['hosts'][HOST]
    work = tempfile.mkdtemp(prefix='brix-gate-')
    sources = os.path.join(root, 'compiler')
    result = {
        'cleanRoom': clean_room(brix, os.path.join(root, 'cleanroom'), work),
        'tools': tools(brix, os.path.join(home, 'releases', version), os.path.join(root, 'cleanroom'), work),
        'examples': examples(brix, os.path.join(home, 'releases', version), work),
        'relink': relinked(home, version, host, work),
        'fixedPoint': fixed_point(brix, sources, host, work),
        'macosObject': macos_object(brix, sources, os.path.join(mirror, 'releases', version),
                                    release['hosts'][BUILD_HOST], work),
    }
    print(json.dumps(result))


def clean_room(brix, programs, work):
    names = sorted(name[:-5] for name in os.listdir(programs) if name.endswith('.yumi'))
    for name in names:
        say('Compiling and running %s' % name)
        executable = os.path.join(work, 'cleanroom', name)
        run([brix, '--compile', '-o', executable, os.path.join(programs, name + '.yumi')], cwd=work)
        actual = run([executable], cwd=work)
        with open(os.path.join(programs, name + '.out')) as file:
            expected = file.read()
        if actual != expected:
            raise GateFailure('%s printed\n%s\ninstead of\n%s' % (name, actual, expected))
    return {'programs': names, 'distribution': distribution(), 'glibc': os.confstr('CS_GNU_LIBC_VERSION')}


def tools(brix, release, programs, work):
    room = os.path.join(work, 'tools')
    os.makedirs(room)
    index = os.path.join(release, GUIDE_INDEX)
    say('Checking that brix --help names %s' % GUIDE_INDEX)
    usage = run([brix, '--help'], cwd=room)
    if index not in usage or not os.path.isfile(index):
        raise GateFailure('brix --help printed\n%s\nwithout %s, or it is missing' % (usage, index))
    for name in TOOLS:
        say('Running brix %s --help' % name)
        run([brix, name, '--help'], cwd=room)
    program = os.path.join(programs, TOOL_PROGRAM + '.yumi')
    say('Recording the probes of %s and reading them back' % TOOL_PROGRAM)
    run([brix, program, '--probes'], cwd=room)
    log = run([brix, 'probes', 'log'], cwd=room)
    if TOOL_PROBE not in log:
        raise GateFailure('brix probes log printed\n%s\nwithout %r' % (log, TOOL_PROBE))
    say('Linting %s with its semantic rules' % TOOL_PROGRAM)
    run([brix, 'lint', '--project', programs, '--semantic-rules', '--semantic-root', program, program], cwd=room)
    project = os.path.join(room, 'project')
    os.makedirs(project)
    say('Running brix init in a new project')
    run([brix, 'init', project], cwd=room)
    for name, expected in INIT_FILES:
        path = os.path.join(project, name)
        text = open(path).read() if os.path.isfile(path) else ''
        if expected not in text:
            raise GateFailure('brix init wrote no %s holding %r' % (name, expected))
    say('Running the test modules of the clean room programs')
    tested = dict(os.environ, PATH='%s:%s' % (os.path.dirname(brix), os.environ.get('PATH', '')))
    output = run([brix, 'test', 'run'], cwd=programs, env=tested)
    if TEST_RESULT not in output:
        raise GateFailure('brix test run printed\n%s\nwithout %r' % (output, TEST_RESULT))
    say('Running the benchmark of the clean room programs')
    output = run([brix, 'bench', 'run', '--iterations', '1'], cwd=programs, env=tested)
    if BENCH_RESULT not in output:
        raise GateFailure('brix bench run printed\n%s\nwithout %r' % (output, BENCH_RESULT))
    return list(TOOLS)


def examples(brix, release, work):
    entries = [entry for entry in example_entries(release) if entry not in MACOS_EXAMPLES]
    say('Building the %d net and tui example programs that run on Linux' % len(entries))
    room = os.path.join(work, 'examples')
    os.makedirs(room)

    def build(entry):
        output = os.path.join(room, os.path.splitext(entry)[0].replace('/', '-'))
        run([brix, os.path.join(release, entry), '--compile', '-o', output], cwd=room)

    with concurrent.futures.ThreadPoolExecutor(max(1, (os.cpu_count() or 2) // 2)) as pool:
        list(pool.map(build, entries))
    return len(entries)


def example_entries(root):
    sources = []
    for name in EXAMPLES:
        for directory, directories, file_names in os.walk(os.path.join(root, 'examples', name)):
            sources += [os.path.join(directory, file_name) for file_name in file_names
                        if file_name.endswith(EXAMPLE_SOURCES)]
    imported = set()
    for source in sources:
        pattern = BRIX_EXAMPLE_IMPORT if source.endswith('.bx') else EXAMPLE_IMPORT
        with open(source) as file:
            for line in file:
                match = pattern.match(line.rstrip())
                if match:
                    stem = os.path.normpath(os.path.join(os.path.dirname(source), match.group(1)))
                    imported.update(stem + extension for extension in EXAMPLE_SOURCES)
    return sorted(os.path.relpath(source, root) for source in sources if source not in imported)


def relinked(home, version, host, work):
    say('Relinking the compiler from its relink kit')
    room = os.path.join(work, 'relink')
    for package in host['packages']:
        if package['name'] in ('toolchain', 'relink'):
            with tarfile.open(os.path.join(home, 'archives', version, package['file']), 'r:xz') as archive:
                archive.extractall(room, filter='data')
    for kit in host['kits']:
        link = os.path.join(room, 'build', 'deps', kit['name'])
        os.makedirs(os.path.dirname(link), exist_ok=True)
        os.symlink(os.path.join(home, 'deps', '%s-%s' % (kit['name'], kit['key'][:16])), link)
    with open(os.path.join(room, 'link.json')) as file:
        link = json.load(file)
    arguments = list(link['arguments'])
    output = os.path.join(room, 'relinked', 'brix')
    os.makedirs(os.path.dirname(output))
    arguments[arguments.index('-o') + 1] = output
    run([os.path.join(room, link['linker'])] + arguments, cwd=room)
    digest = file_digest(output)
    if digest != host['hashes']['bin/brix']:
        raise GateFailure('Relinking gave %s, not bin/brix (%s)' % (digest, host['hashes']['bin/brix']))
    return {'sha256': digest}


def fixed_point(brix, sources, host, work):
    say('Compiling the compiler with the installed compiler')
    executable = os.path.join(work, 'compiler', 'brix')
    run([brix, '--compile', '-o', executable, os.path.join(sources, 'brix-cli.yumi')], cwd=work)
    digest = file_digest(executable + '.o')
    if digest != host['hashes']['obj/brix.o']:
        raise GateFailure('The compiler built on Linux has object %s, not the released %s' % (
            digest, host['hashes']['obj/brix.o']))
    return {'object': digest}


def macos_object(brix, sources, files, macos, work):
    say('Compiling the macOS compiler from the C headers the Mac read')
    package = [package for package in macos['packages'] if package['name'] == 'c-headers'][0]
    snapshot = os.path.join(work, 'macos', 'brix.c-headers')
    os.makedirs(os.path.dirname(snapshot))
    with lzma.open(os.path.join(files, package['file'])) as source, open(snapshot, 'wb') as output:
        output.write(source.read())
    output_path = os.path.join(work, 'macos', 'brix.o')
    run([brix, '--target', 'macos', '--object-only', '--c-headers', snapshot, '-o', output_path,
         os.path.join(sources, 'brix-cli.yumi')], cwd=work)
    digest = file_digest(output_path)
    if digest != macos['hashes']['obj/brix.o']:
        raise GateFailure('The macOS compiler built on Linux has object %s, not the released %s' % (
            digest, macos['hashes']['obj/brix.o']))
    return {'object': digest}


def distribution():
    try:
        with open('/etc/os-release') as file:
            for line in file:
                if line.startswith('PRETTY_NAME='):
                    return line.split('=', 1)[1].strip().strip('"')
    except OSError:
        pass
    return None


def run(command, cwd=None, env=None):
    say('$ %s' % ' '.join(command))
    try:
        result = subprocess.run(command, cwd=cwd, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                universal_newlines=True, timeout=TIMEOUT)
    except subprocess.TimeoutExpired:
        raise GateFailure('%s timed out after %d s' % (command[0], TIMEOUT))
    if result.returncode != 0:
        raise GateFailure('%s failed with status %d:\n%s' % (command[0], result.returncode, result.stdout[-4000:]))
    return result.stdout


def file_digest(path):
    digest = hashlib.sha256()
    with open(path, 'rb') as file:
        for chunk in iter(lambda: file.read(1 << 20), b''):
            digest.update(chunk)
    return digest.hexdigest()


def say(message):
    sys.stderr.write('%s\n' % message)
    sys.stderr.flush()


if __name__ == '__main__':
    try:
        main(sys.argv[1:])
    except GateFailure as error:
        say('Linux gate: %s' % error)
        sys.exit(1)
