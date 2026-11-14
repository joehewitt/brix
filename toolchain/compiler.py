import json
import os
import shutil
import subprocess
import sys
import tempfile
import time

import launcher
from common import BUILD, ROOT, RUNTIME_CONFIG, Failure, build_environment, built_path, file_digest, lock, xcrun

MANIFEST = os.path.join(ROOT, launcher.MANIFEST_NAME)
TOOLS = os.path.join(BUILD, 'tools')
COMPILER = os.path.join(TOOLS, 'yumi')
OBJECT = COMPILER + '.o'
INPUTS = COMPILER + '.inputs.json'
LOCK = COMPILER + '.lock'
SOURCE = os.path.join(ROOT, 'compiler', 'yumi-cli.yumi')
RUNTIME_BUILDER = os.path.join(ROOT, 'toolchain', 'runtime.py')
INPUTS_VERSION = 5
BUILDER_FILES = (
    'libs/build.json',
    'toolchain/common.py',
    'toolchain/compiler.py',
    'toolchain/launcher.py',
)
TOOL_SEED = 'compiler'
TOOL_SOURCES = {
    'probes': os.path.join(ROOT, 'compiler', 'probes', 'probe-cli.yumi'),
    'test': os.path.join(ROOT, 'compiler', 'test', 'test-cli.yumi'),
    'test-suite': os.path.join(ROOT, 'compiler', 'test', 'test-suite.yumi'),
    'bench': os.path.join(ROOT, 'compiler', 'bench', 'bench-cli.yumi'),
    'lint': os.path.join(ROOT, 'compiler', 'lint', 'lint-cli.yumi'),
    'brix': os.path.join(ROOT, 'compiler', 'brix', 'brix-cli.yumi'),
}
PROBED_COMPILER = os.path.join(TOOLS, 'yumi-probed')
SELECTOR_CHARACTERS = set('abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.*:,-_ ')
NANOSECONDS = 1000000000


def main(arguments):
    os.environ.setdefault('YUMI_BUILD_PATH', BUILD)
    if arguments and arguments[0] == 'brix-sweep':
        import brix_sweep
        return brix_sweep.main(arguments[1:])
    if arguments and arguments[0] == 'brix-tests':
        import brix_tests
        return brix_tests.main(arguments[1:])
    if arguments and arguments[0] in TOOL_SOURCES:
        ensure_compiler(seed_name())
        tool = ensure_tool(arguments[0])
        os.execv(tool, [tool] + arguments[1:])
    options = arguments[:arguments.index('--')] if '--' in arguments else arguments
    build_only = '--build-only' in options
    if build_only:
        position = options.index('--build-only')
        arguments = arguments[:position] + arguments[position + 1:]
    selectors, arguments = compiler_probes(arguments)
    ensure_compiler(seed_name())
    if selectors is None:
        if build_only:
            print(COMPILER)
            return 0
        os.execv(COMPILER, [COMPILER] + arguments)
    probed = ensure_probed_compiler(selectors)
    if build_only:
        print(probed)
        return 0
    tool = ensure_tool('probes')
    os.execv(tool, [tool, 'run', '--', probed] + arguments)


def compiler_probes(arguments):
    end = arguments.index('--') if '--' in arguments else len(arguments)
    for position in range(end):
        argument = arguments[position]
        if argument.startswith('--compiler-probes='):
            return argument[len('--compiler-probes='):], arguments[:position] + arguments[position + 1:]
        if argument == '--compiler-probes':
            following = arguments[position + 1] if position + 1 < end else ''
            if is_selector_list(following):
                return following, arguments[:position] + arguments[position + 2:]
            return '', arguments[:position] + arguments[position + 1:]
    return None, arguments


def is_selector_list(text):
    if not text or text.startswith('-') or text.endswith('.yumi'):
        return False
    return all(character in SELECTOR_CHARACTERS for character in text)


def seed_name():
    manifest = monorepo_manifest()
    return '%s@%s' % (manifest['version'], manifest['release'])


def monorepo_manifest():
    if not os.path.isfile(MANIFEST):
        fail('%s is missing; it names the seed that builds the compiler.' % MANIFEST)
    try:
        manifest = launcher.read_manifest(MANIFEST)
    except launcher.LaunchError as error:
        fail(str(error))
    if manifest['use'] != 'seed':
        fail('%s must pin a seed ("use": "seed") to build the compiler' % MANIFEST)
    return manifest


def seed_command():
    try:
        return launcher.seed_compiler(monorepo_manifest())
    except launcher.LaunchError as error:
        fail(str(error))


def fail(message):
    raise Failure('Building the compiler failed:\n%s' % message)


def ensure_compiler(seed):
    record = read_record()
    status = record_status(record, seed)
    if status == 'fresh':
        return
    os.makedirs(TOOLS, exist_ok=True)
    with open(LOCK, 'w') as file:
        if status == 'touched':
            if lock(file, wait=False):
                refresh_record(record)
            return
        lock(file)
        record = read_record()
        status = record_status(record, seed)
        if status == 'touched':
            refresh_record(record)
        elif status != 'fresh':
            build(seed)


def ensure_tool(name):
    executable = os.path.join(TOOLS, 'yumi-' + name)
    ensure_executable(executable, TOOL_SOURCES[name], TOOL_SEED, [], 'Building yumi %s failed' % name)
    return executable


def ensure_probed_compiler(selectors):
    flag = '--probes=' + selectors if selectors else '--probes'
    key = '%s %s' % (TOOL_SEED, flag)
    ensure_executable(PROBED_COMPILER, SOURCE, key, [flag], 'Building the probed compiler failed')
    return PROBED_COMPILER


def ensure_executable(executable, source, key, flags, label):
    inputs = executable + '.inputs.json'
    record = read_record(inputs)
    status = record_status(record, key)
    if status == 'fresh':
        return
    with open(executable + '.lock', 'w') as file:
        if status == 'touched':
            if lock(file, wait=False):
                refresh_record(record, inputs)
            return
        lock(file)
        record = read_record(inputs)
        status = record_status(record, key)
        if status == 'touched':
            refresh_record(record, inputs)
        elif status != 'fresh':
            build_executable(executable, inputs, source, key, flags, label)


def build_executable(executable, inputs, source, key, flags, label):
    started = time.time_ns()
    temporary = tempfile.mkdtemp(prefix='.build-%s-' % os.path.basename(executable), dir=TOOLS)
    try:
        environment = compiler_environment()
        built = os.path.join(temporary, os.path.basename(executable))
        run([COMPILER, '--compile', '-o', built] + flags + [source], environment, label)
        listed = run([COMPILER, '--list-sources'] + flags + [source], environment, label)
        sources = [line for line in listed.splitlines() if line]
        record = input_record(sources, started, key, COMPILER)
        outputs = [executable]
        os.replace(built, executable)
        if flags:
            os.replace(built + '.probes.json', executable + '.probes.json')
            outputs.append(executable + '.probes.json')
        record['outputs'] = [file_entry(path) for path in outputs]
        write_record(record, inputs)
    finally:
        shutil.rmtree(temporary, ignore_errors=True)


def read_record(inputs=INPUTS):
    try:
        with open(inputs) as file:
            record = json.load(file)
    except (OSError, ValueError):
        return None
    if record.get('version') != INPUTS_VERSION:
        return None
    return record


def record_status(record, seed):
    if record is None:
        return 'missing'
    if record.get('seed') != seed:
        return 'stale'
    touched = False
    for entry in record['outputs'] + record['files']:
        path, size, seconds, nanoseconds, digest = entry
        if not digest:
            return 'stale'
        try:
            stat = os.stat(path)
        except OSError:
            return 'stale'
        if stat.st_size != size:
            return 'stale'
        if modified(stat) != [seconds, nanoseconds]:
            if file_digest(path) != digest:
                return 'stale'
            touched = True
    for path, names in record['directories']:
        try:
            listing = directory_listing(path)
        except OSError:
            return 'stale'
        if listing != names:
            return 'stale'
    return 'touched' if touched else 'fresh'


def refresh_record(record, inputs=INPUTS):
    record['outputs'] = [file_entry(entry[0]) for entry in record['outputs']]
    record['files'] = [refreshed_entry(entry) for entry in record['files']]
    write_record(record, inputs)


def refreshed_entry(entry):
    path, size, seconds, nanoseconds, digest = entry
    stat = os.stat(path)
    return [path, stat.st_size] + modified(stat) + [digest]


def build(seed):
    started = time.time_ns()
    compiler = seed_command()
    temporary = tempfile.mkdtemp(prefix='.build-yumi-', dir=TOOLS)
    try:
        environment = compiler_environment()
        build_runtime(environment)
        bootstrap = os.path.join(temporary, 'yumi-bootstrap')
        executable = os.path.join(temporary, 'yumi')
        run([compiler, '--compile', '-o', bootstrap, SOURCE], environment)
        run([bootstrap, '--compile', '-o', executable, SOURCE], environment)
        listed = run([executable, '--list-sources', SOURCE], environment)
        sources = [line for line in listed.splitlines() if line]
        record = input_record(sources, started, seed, compiler)
        os.replace(executable + '.o', OBJECT)
        os.replace(executable, COMPILER)
        archives = [file_entry(path) for path in runtime_archives(sources)]
        record['outputs'] = [file_entry(COMPILER), file_entry(OBJECT)] + archives
        write_record(record)
    finally:
        shutil.rmtree(temporary, ignore_errors=True)
    remove_file(COMPILER + '.key')


def run(command, environment, label='Building the compiler failed'):
    result = subprocess.run(
        command,
        cwd=ROOT,
        env=environment,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        universal_newlines=True,
    )
    if result.returncode != 0:
        raise Failure('%s:\n%s' % (label, (result.stderr or result.stdout).rstrip('\n')))
    return result.stdout


def build_runtime(environment):
    result = subprocess.run(
        [sys.executable, RUNTIME_BUILDER, 'build', '--build', BUILD],
        cwd=ROOT,
        env=environment,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        universal_newlines=True,
    )
    if result.returncode != 0:
        fail(result.stderr.rstrip('\n'))


def compiler_environment():
    environment = build_environment()
    if 'PATH' in os.environ:
        environment['PATH'] = os.environ['PATH']
    return environment


def input_record(sources, started, seed, compiler):
    files = set()
    directories = set()
    build_root = BUILD + os.sep
    for path in sources:
        if not path.startswith(build_root):
            files.add(path)
        if path.startswith(ROOT + os.sep) and path.endswith('.yumi'):
            directories.add(os.path.dirname(path))
    for name in BUILDER_FILES:
        files.add(os.path.join(ROOT, name))
    files.add(MANIFEST)
    files.add(compiler)
    for path in runtime_inputs(sources):
        if os.path.isdir(path):
            add_tree(path, files, directories)
        else:
            files.add(path)
    for path in toolchain_inputs():
        files.add(path)
    entries = []
    for path in sorted(files):
        entry = file_entry(path)
        if entry[2] * NANOSECONDS + entry[3] > started:
            entry[4] = ''
        entries.append(entry)
    return {
        'version': INPUTS_VERSION,
        'seed': seed,
        'outputs': [],
        'files': entries,
        'directories': [[path, directory_listing(path)] for path in sorted(directories)],
    }


def add_tree(root, files, directories):
    for directory, names, file_names in os.walk(root):
        names[:] = [name for name in names if not name.startswith('.')]
        directories.add(directory)
        for name in file_names:
            if not name.startswith('.'):
                files.add(os.path.join(directory, name))


def runtime_inputs(sources):
    projects, libraries, names = linked_runtime(sources)
    inputs = []
    for name in names:
        if name in projects:
            inputs.append(built_path(projects[name]['path']))
        elif name in libraries:
            inputs.extend(built_path(path) for path in libraries[name].get('libraryPaths', []))
    return inputs


def runtime_archives(sources):
    projects, _, names = linked_runtime(sources)
    return [os.path.join(BUILD, projects[name]['archive']) for name in names if name in projects]


def linked_runtime(sources):
    with open(RUNTIME_CONFIG) as file:
        config = json.load(file)
    projects = config.get('runtimeProjects', {})
    libraries = config.get('externalLibraries', {})
    wanted = list(config.get('runtimeLink', {}).get('always', []))
    for name, project in projects.items():
        built = os.path.join(BUILD, 'yumi', project['path']) + os.sep
        if any(path.startswith(built) for path in sources):
            wanted.append(name)
    names = []
    while wanted:
        name = wanted.pop()
        if name in names:
            continue
        names.append(name)
        if name in projects:
            wanted.extend(projects[name].get('requires', []))
    return projects, libraries, names


def toolchain_inputs():
    if os.uname().sysname == 'Linux':
        with open(RUNTIME_CONFIG) as file:
            target = json.load(file)['targets']['linux']
        clang = shutil.which('clang')
        if not clang:
            fail('Building the runtime on Linux needs clang')
        return [clang] + [built_path(target[name]) for name in ('linker', 'libclang')]
    clang = xcrun('--find', 'clang')
    linker = xcrun('--find', 'ld')
    sdk = xcrun('--show-sdk-path')
    libclang = os.path.join(os.path.dirname(os.path.dirname(clang)), 'lib', 'libclang.dylib')
    return [clang, linker, libclang, os.path.join(sdk, 'SDKSettings.json')]


def file_entry(path):
    stat = os.stat(path)
    return [path, stat.st_size] + modified(stat) + [file_digest(path)]


def modified(stat):
    return list(divmod(stat.st_mtime_ns, NANOSECONDS))


def directory_listing(path):
    return sorted(name for name in os.listdir(path) if not name.startswith('.'))


def write_record(record, inputs=INPUTS):
    temporary = inputs + '.tmp-%d' % os.getpid()
    with open(temporary, 'w') as file:
        json.dump(record, file)
    os.replace(temporary, inputs)


def remove_file(path):
    try:
        os.remove(path)
    except FileNotFoundError:
        pass
