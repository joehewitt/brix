import os
import shutil
import subprocess
import time
from concurrent.futures import ThreadPoolExecutor

import compiler
from common import BUILD, ROOT, Failure, file_digest, say, take_folders

SWEEP = os.path.join(BUILD, 'brix-sweep')
COMPILER_SOURCE = os.path.join('apps', 'compiler', 'brix-cli.bx')
SUMMARY = 'sweep files '
SHOWN_PROBLEMS = 40
WORKERS = os.cpu_count() or 4


def main(arguments):
    folders, rest = take_folders(arguments)
    if rest:
        raise Failure('brix brix-sweep takes only --with FOLDER, to convert the files of a folder outside the toolchain too')
    started = time.time()
    compiler.ensure_compiler(compiler.seed_name())
    converter = compiler.ensure_tool('brix')
    result = sweep(converter, compiler.COMPILER, SWEEP, compiler.compiler_environment(), folders)
    say('%d files convert to Brix and read back, and the compiler builds itself from them, in %.1f s' % (
        result['files'], time.time() - started))
    return 0


def sweep(converter, builder, directory, environment, folders=()):
    shutil.rmtree(directory, ignore_errors=True)
    tree = os.path.join(directory, 'tree')
    os.makedirs(tree)
    sources = brix_sources()
    say('Converting %d files to Brix' % len(sources))
    convert(converter, sources, tree, environment)
    count = len(sources)
    for folder in folders:
        parent, name = os.path.split(folder)
        added = brix_sources(parent, name)
        say('Converting %d files in %s to Brix' % (len(added), name))
        convert(converter, added, tree, environment, parent)
        count += len(added)
    say('Building the compiler from its Brix')
    digest = build(builder, tree, os.path.join(directory, 'build'), environment)
    return {'files': count, 'object': digest}


def brix_sources(root=ROOT, folder=None):
    listed = subprocess.run(
        ['git', 'ls-files', '-z', '--cached', '--others', '--exclude-standard', '--',
         '*.yumi' if folder is None else folder + '/*.yumi'],
        cwd=root,
        stdout=subprocess.PIPE,
        check=True,
    ).stdout.decode()
    paths = [path for path in listed.split('\0') if path]
    return sorted(set(path for path in paths if os.path.isfile(os.path.join(root, path))))


def convert(converter, sources, tree, environment, root=ROOT):
    if not sources:
        return
    count = max(1, min(WORKERS, len(sources)))
    chunks = [sources[index::count] for index in range(count)]
    writing = dict(environment, BRIX_OUT=tree)
    with ThreadPoolExecutor(count) as pool:
        results = list(pool.map(lambda chunk: convert_chunk(converter, chunk, writing, root), chunks))
    messages = [message for found, lines in results for message in lines]
    problems = sum(found for found, lines in results)
    if problems:
        shown = '\n'.join(messages[:SHOWN_PROBLEMS])
        more = len(messages) - SHOWN_PROBLEMS
        raise Failure('%d problems in %d files converted to Brix:\n%s%s' % (
            problems, len(sources), shown, '\n…and %d more' % more if more > 0 else ''))


def convert_chunk(converter, chunk, environment, root=ROOT):
    result = subprocess.run(
        [converter, '--sweep'] + chunk,
        cwd=root,
        env=environment,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        universal_newlines=True,
    )
    lines = result.stdout.splitlines()
    summary = [line for line in lines if line.startswith(SUMMARY)]
    messages = [line for line in lines if not line.startswith(SUMMARY)]
    if not summary:
        return 1, messages + ['The converter stopped with status %d before it finished' % result.returncode]
    return int(summary[-1].split()[-1]), messages


def build(builder, tree, directory, environment):
    reading = dict(environment, BRIXPATH=os.path.join(tree, 'stdlib'))
    source = os.path.join(tree, COMPILER_SOURCE)
    first = os.path.join(directory, 'first', 'brix')
    second = os.path.join(directory, 'second', 'brix')
    for executable, building in ((first, builder), (second, first)):
        os.makedirs(os.path.dirname(executable))
        result = subprocess.run(
            [building, '--compile', '-o', executable, source],
            cwd=ROOT,
            env=reading,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            universal_newlines=True,
        )
        if result.returncode != 0:
            raise Failure('Building the compiler from Brix with %s failed:\n%s' % (
                building, result.stdout.rstrip('\n')))
    digest = file_digest(first + '.o')
    if digest != file_digest(second + '.o'):
        raise Failure('The compiler built from Brix does not rebuild itself to the same object: %s and %s differ' % (
            first + '.o', second + '.o'))
    return digest
