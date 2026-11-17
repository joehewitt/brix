import os
import subprocess
import time

from bench import gc_runtime
from common import ROOT, TMP, Failure

BUILDER = os.path.join(ROOT, 'bin', 'brix')
COMPILER_LOG = os.path.join(TMP, 'bench', 'compilers.log')
STRING_FLAGS = ('o', 'probes')
BOOLEAN_WORDS = ('true', 'false')
TIME_FORMAT = '%Y-%m-%dT%H:%M:%SZ'


def main(arguments):
    if arguments[:1] == ['--run']:
        return gc_runtime.run_variants(arguments[1:])
    compiler_probes = os.environ.get('BRIX_BENCH_COMPILER_PROBES', '')
    compiler_trace = os.environ.get('BRIX_BENCH_COMPILER_TRACE', '')
    if compiler_probes and not compiler_trace:
        raise Failure('BRIX_BENCH_COMPILER_PROBES needs BRIX_BENCH_COMPILER_TRACE')
    if gc_runtime.selected():
        compiler = build_compiler(compiler_probes)
        if not compiler:
            return 1
        return gc_runtime.compile_variants(arguments, compiler)
    flags, sources = parse(arguments)
    if flags.get('debug'):
        raise Failure('The compiler has no debug build (--debug)')
    if not flags.get('compile'):
        raise Failure('The bench compiler only compiles (--compile)')
    if not sources or not flags.get('o'):
        raise Failure('usage: brix-bench --compile -o <executable> <source>')
    source = sources[0][1]
    compiler = build_compiler(compiler_probes)
    if not compiler:
        return 1
    record_compiler(compiler, source)
    environment = {name: value for name, value in os.environ.items() if not name.startswith('PROBES_')}
    if compiler_probes:
        if os.path.exists(compiler_trace):
            os.remove(compiler_trace)
        environment['PROBES_ENABLED'] = '1'
        environment['PROBES_PATH'] = compiler_trace
    options = ['--probes=' + flags['probes']] if flags.get('probes') else []
    os.execve(compiler, [compiler, '--compile'] + options + ['-o', flags['o'], source], environment)


def parse(arguments):
    flags = {}
    sources = []
    index = 0
    while index < len(arguments):
        argument = arguments[index]
        if not argument.startswith('-') or argument == '-':
            sources.append((index, argument))
        else:
            name, equals, value = argument.lstrip('-').partition('=')
            if name in STRING_FLAGS:
                if not equals and index + 1 < len(arguments):
                    index += 1
                    value = arguments[index]
                flags[name] = value
            else:
                if not equals and index + 1 < len(arguments) and arguments[index + 1] in BOOLEAN_WORDS:
                    index += 1
                    value = arguments[index]
                flags[name] = value != 'false'
        index += 1
    return flags, sources


def build_compiler(compiler_probes):
    environment = {name: value for name, value in os.environ.items()
                   if not name.startswith('PROBES_') and not name.startswith('BRIX_BENCH_')}
    selection = ['--compiler-probes=' + compiler_probes] if compiler_probes else []
    built = subprocess.run([BUILDER] + selection + ['--build-only'], env=environment,
                           stdout=subprocess.PIPE, text=True)
    if built.returncode != 0:
        return None
    return built.stdout.strip().split('\n')[-1]


def record_compiler(compiler, source):
    status = os.stat(compiler)
    os.makedirs(os.path.dirname(COMPILER_LOG), exist_ok=True)
    with open(COMPILER_LOG, 'a') as file:
        file.write('%s %s %d %s\n' % (time.strftime(TIME_FORMAT, time.gmtime()),
                                      time.strftime(TIME_FORMAT, time.gmtime(status.st_mtime)),
                                      status.st_size, os.path.relpath(source, ROOT)))
