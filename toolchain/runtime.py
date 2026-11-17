#!/usr/bin/env python3
import argparse
import json
import os
import re
import shlex
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.realpath(__file__)))

import deps
from checks import gc_allocation_abi
from common import BUILD, ROOT, RUNTIME_CONFIG, Failure, built_path, is_within, run_main, say, shown, xcrun

CONFIG = RUNTIME_CONFIG
HOST_FAMILY = 'macos'
HOST_TARGETS = {'Linux': 'linux'}
C_EXTENSIONS = ('.c', '.m')
CXX_EXTENSIONS = ('.cpp', '.mm')
C_STANDARD = 'gnu99'
DEBUG_FLAGS = ['-gdwarf-2', '-DDEBUG', '-D_DEBUG']
ARCHIVER = 'libtool'
TARGET_TOOLS = ('sysroot', 'linker', 'archiver')
COLOR_FLAG = '-fcolor-diagnostics'
SANITIZERS = {'tsan': 'thread', 'asan': 'address'}
TRUE_WORDS = ('1', 'true', 'yes', 'on')
FALSE_WORDS = ('', '0', 'false', 'no', 'off')
DEPFILE_WORD = re.compile(r'(?:\\.|[^\s\\])+')
DEPFILE_ESCAPE = re.compile(r'\\(.)')


def main(arguments):
    options = parse_arguments(arguments)
    with open(CONFIG) as file:
        config = json.load(file)
    build = Build(config, options, os.environ)
    if options.action == 'clean':
        build.clean()
    else:
        gc_allocation_abi.check(config)
        build.build()


def target_dependencies(config, target, projects):
    paths = []
    for project in projects:
        paths += config['runtimeProjects'][project].get('includePaths', [])
    for name in external_names(config, projects):
        library = config['externalLibraries'][name]
        paths += [library['path']] + library.get('includePaths', []) + library.get('libraryPaths', [])
    names = sorted({path.split('/')[2] for path in paths if path.startswith('build/deps/')})
    tools = sorted({target[key].split('/')[2] for key in TARGET_TOOLS if key in target})
    return names, tools


def external_names(config, projects):
    runtime = config['runtimeProjects']
    found = []
    pending = [name for project in projects for name in runtime[project].get('requires', [])]
    visited = set()
    while pending:
        name = pending.pop(0)
        if name in visited:
            continue
        visited.add(name)
        if name in runtime:
            pending += runtime[name].get('requires', [])
        elif name in config['externalLibraries']:
            found.append(name)
        else:
            raise Failure('libs/build.json requires unknown library %s' % name)
    return found


def parse_arguments(arguments):
    parser = argparse.ArgumentParser(prog='runtime.py', description='Build the C runtime libraries.')
    parser.add_argument('action', choices=('build', 'clean'))
    parser.add_argument('--build', default=BUILD, help='build directory (default: %s)' % shown(BUILD))
    parser.add_argument('--target', help='a target from libs/build.json (default: this machine\'s)')
    parser.add_argument('--debug', action='store_true', help='add debug information')
    sanitizers = parser.add_mutually_exclusive_group()
    for flag, name in SANITIZERS.items():
        sanitizers.add_argument('--' + flag, dest='sanitizer', action='store_const', const=name,
                                help='build with the %s sanitizer' % name)
    parser.add_argument('-v', '--verbose', action='store_true', help='print each command')
    return parser.parse_args(arguments)


class Project:
    def __init__(self, name, config, build):
        self.name = name
        self.config = config
        self.path = os.path.join(ROOT, config['path'])
        self.archive = os.path.join(build.root, config['archive'])
        self.directory = os.path.dirname(self.archive)
        override = config.get('targets', {}).get(build.family, {})
        self.excluded = set(override.get('exclude', []))

    def sources(self):
        found = []
        for directory, names, file_names in os.walk(self.path):
            names[:] = sorted(name for name in names if not name.startswith('.') and name != 'tests')
            for file_name in file_names:
                if file_name.startswith('.') or file_name in self.excluded:
                    continue
                if os.path.splitext(file_name)[1] in C_EXTENSIONS + CXX_EXTENSIONS:
                    found.append(os.path.join(directory, file_name))
        return sorted(found)

    def object_path(self, source):
        return os.path.splitext(os.path.join(self.directory, os.path.relpath(source, self.path)))[0] + '.o'


class Build:
    def __init__(self, config, options, environment):
        self.config = config
        self.options = options
        self.environment = environment
        self.target = None
        name = options.target or HOST_TARGETS.get(os.uname().sysname)
        if name:
            self.target = config.get('targets', {}).get(name)
            if not self.target:
                raise Failure('libs/build.json has no target %s; choose one of %s'
                              % (name, ', '.join(config.get('targets', {}))))
        self.family = self.target['family'] if self.target else HOST_FAMILY
        self.root = os.path.abspath(options.build)
        self.sanitizer = {}
        if options.sanitizer:
            self.sanitizer = config.get('sanitizers', {}).get(options.sanitizer)
            if not self.sanitizer:
                raise Failure('libs/build.json does not define the %s sanitizer' % options.sanitizer)
            directory = self.sanitizer['buildDirectory']
            if os.path.basename(self.root) != directory:
                self.root = os.path.join(self.root, directory)
        names = [name for name in config['runtimeProjects']
                 if not self.target or name in self.target['components']]
        self.projects = [Project(name, config['runtimeProjects'][name], self) for name in names]
        self.compiler = config['toolchain']['cCompiler']

    def build(self):
        started = time.time()
        self.prepare()
        flags = self.flags()
        identity = self.toolchain_identity()
        rebuilt = sum(1 for project in self.projects if self.build_project(project, flags, identity))
        if rebuilt > 1:
            say('runtime  %d of %d libraries rebuilt  %.1fs' % (rebuilt, len(self.projects), time.time() - started))

    def clean(self):
        removed = 0
        for project in self.projects:
            paths = [project.archive, project.archive + '.cmd']
            for source in project.sources():
                built = project.object_path(source)
                paths += [built, built + '.d', built + '.cmd']
            for path in paths:
                if os.path.isfile(path):
                    os.remove(path)
                    removed += 1
        if removed:
            say('runtime  removed %d files' % removed)

    def prepare(self):
        externals = external_names(self.config, [project.name for project in self.projects])
        if self.target:
            names, tools = target_dependencies(self.config, self.target, [project.name for project in self.projects])
            deps.ensure_dependencies(self.root, self.target, names)
            if tools:
                deps.ensure_dependencies(self.root, None, tools)
        else:
            deps.ensure_dependencies(self.root)
        for project in self.projects:
            for include, shorthand in project.config.get('exports', {}).items():
                deps.link_path(os.path.join(project.directory, shorthand), os.path.join(project.path, include))
        for name in externals:
            library = self.config['externalLibraries'][name]
            for include, shorthand in library.get('exports', {}).items():
                deps.link_path(os.path.join(self.external_directory(name), shorthand),
                               os.path.join(built_path(library['path'], self.root), include))

    def build_project(self, project, flags, identity):
        started = time.time()
        includes = ['-I' + self.local(path) for path in self.include_directories(project)]
        common = flags + project.config.get('compilerFlags', []) + self.late_flags()
        sources = project.sources()
        compiled = 0
        for source in sorted(sources, key=lambda path: os.stat(path).st_mtime_ns, reverse=True):
            built = project.object_path(source)
            command = ([self.compiler, '-std=' + self.standard(source)] + common + includes +
                       ['-MMD', '-MF', self.local(built + '.d'), '-c', '-o', self.local(built), self.local(source)])
            record = '\0'.join([identity] + command)
            if self.current(built, record):
                continue
            os.makedirs(os.path.dirname(built), exist_ok=True)
            color = [COLOR_FLAG] if sys.stderr.isatty() else []
            self.execute(command[:1] + color + command[1:], '%s failed compiling %s' % (project.name, self.local(source)))
            write_text(built + '.cmd', record)
            compiled += 1
        objects = [project.object_path(source) for source in sources]
        command = self.archive_command(project.archive) + [self.local(path) for path in objects]
        record = '\0'.join(command)
        if compiled or not self.archive_current(project.archive, record, objects):
            if os.path.isfile(project.archive):
                os.remove(project.archive)
            self.execute(command, '%s failed archiving %s' % (project.name, self.local(project.archive)))
            write_text(project.archive + '.cmd', record)
            work = '%d compiled' % compiled if compiled else 'archived'
            say('%s  %s  %.1fs' % (project.name, work, time.time() - started))
            return True
        return False

    def archive_command(self, archive):
        if self.target and 'archiver' in self.target:
            return [built_path(self.target['archiver'], self.root), 'rc', self.local(archive)]
        return [ARCHIVER, '-static', '-D', '-o', self.local(archive)]

    def flags(self):
        flags = []
        if self.target and 'sysroot' in self.target:
            sysroot = built_path(self.target['sysroot'], self.root)
            flags += ['-target', self.target['triple'], '--sysroot=' + sysroot,
                      '--gcc-install-dir=' + os.path.join(sysroot, self.target['gccInstallDirectory'])]
        elif self.target:
            flags += ['-target', self.target['triple'], '-isysroot', self.sdk_path()]
        if self.target and 'cpu' in self.target:
            flags.append('-mcpu=' + self.target['cpu'])
        flags.append('-ffile-prefix-map=%s/=' % ROOT)
        if not is_within(self.root, ROOT):
            flags.append('-ffile-prefix-map=%s/=build/' % self.root)
        flags += self.config.get('compile', {}).get('commonFlags', [])
        flags += ['-D%s=%s' % (name, value) for name, value in self.gc_mode().get('defines', {}).items()]
        return flags

    def late_flags(self):
        flags = ['-DBX_ENABLE_BENCH_METRICS=%d' % self.bench_metrics()]
        if self.options.debug:
            flags += DEBUG_FLAGS
        return flags + self.sanitizer.get('compileFlags', [])

    def standard(self, source):
        if os.path.splitext(source)[1] in C_EXTENSIONS:
            return C_STANDARD
        return self.config['compile']['runtimeCxxStandard']

    def include_directories(self, project):
        directories = [project.path, project.directory]
        directories += [built_path(path, self.root) for path in project.config.get('includePaths', [])]
        directories += [other.directory for other in self.projects]
        for name in external_names(self.config, [project.name]):
            library = self.config['externalLibraries'][name]
            directories += [built_path(path, self.root) for path in library.get('includePaths', [])]
            if library.get('exports'):
                directories.append(self.external_directory(name))
        unique = []
        for directory in directories:
            if directory not in unique:
                unique.append(directory)
        return unique

    def external_directory(self, name):
        return os.path.join(self.root, 'external', name)

    def local(self, path):
        return os.path.relpath(path, ROOT) if is_within(path, ROOT) else path

    def current(self, built, record):
        try:
            built_time = os.stat(built).st_mtime_ns
        except FileNotFoundError:
            return False
        if read_text(built + '.cmd') != record:
            return False
        inputs = depfile_inputs(built + '.d')
        if not inputs:
            return False
        for path in inputs:
            try:
                if os.stat(os.path.join(ROOT, path)).st_mtime_ns > built_time:
                    return False
            except FileNotFoundError:
                return False
        return True

    def archive_current(self, archive, record, objects):
        try:
            archived = os.stat(archive).st_mtime_ns
        except FileNotFoundError:
            return False
        if read_text(archive + '.cmd') != record:
            return False
        return all(os.stat(path).st_mtime_ns <= archived for path in objects)

    def execute(self, command, failure):
        if self.options.verbose:
            say(shlex.join(command))
        try:
            result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, errors='replace')
        except FileNotFoundError:
            raise Failure('%s is not installed' % command[0])
        if result.stdout or result.stderr:
            sys.stderr.write(result.stdout + result.stderr)
        if result.returncode != 0:
            raise Failure(failure)

    def toolchain_identity(self):
        try:
            return subprocess.run([self.compiler, '--version'], capture_output=True, text=True).stdout
        except FileNotFoundError:
            raise Failure('%s is not installed' % self.compiler)

    def sdk_path(self):
        return xcrun('--sdk', self.target['sdk'], '--show-sdk-path')

    def gc_mode(self):
        modes = self.config.get('gcModes', {})
        name = self.environment.get('BRIX_GC_MODE', 'normal').strip().lower() or 'normal'
        mode = modes.get(name)
        if not mode:
            raise Failure('BRIX_GC_MODE must be one of %s' % ', '.join(modes))
        if mode.get('benchmarkOnly') and not self.flag('BRIX_BENCH_RUNTIME'):
            raise Failure('BRIX_GC_MODE=%s is benchmark-only; set BRIX_BENCH_RUNTIME=1' % name)
        return mode

    def bench_metrics(self):
        enabled = self.flag('BRIX_BENCH_METRICS')
        if enabled is None:
            return bool(self.flag('BRIX_BENCH_RUNTIME'))
        return enabled

    def flag(self, name):
        value = self.environment.get(name)
        if value is None:
            return None
        value = value.strip().lower()
        if value in TRUE_WORDS:
            return True
        if value in FALSE_WORDS:
            return False
        raise Failure('%s must be 0 or 1' % name)


def depfile_inputs(path):
    text = read_text(path)
    if text is None:
        return None
    _, _, inputs = text.replace('\\\n', ' ').partition(': ')
    return [DEPFILE_ESCAPE.sub(r'\1', word).replace('$$', '$') for word in DEPFILE_WORD.findall(inputs)]


def read_text(path):
    try:
        with open(path) as file:
            return file.read()
    except FileNotFoundError:
        return None


def write_text(path, text):
    with open(path, 'w') as file:
        file.write(text)


if __name__ == '__main__':
    sys.exit(run_main(main))
