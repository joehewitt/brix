import fcntl
import hashlib
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
TMP = os.path.join(ROOT, '.tmp')
BUILD = os.path.join(TMP, 'build')
RUNTIME_CONFIG = os.path.join(ROOT, 'libs', 'build.json')
BRIX_HOME = os.path.realpath(os.environ.get('BRIX_HOME') or os.path.expanduser('~/.brix'))
CHUNK_SIZE = 1 << 20
LOG_TAIL = 20
PASSED_ENVIRONMENT_NAMES = (
    'DEVELOPER_DIR',
    'HOME',
    'LANG',
    'LC_ALL',
    'LOGNAME',
    'SDKROOT',
    'SHELL',
    'TMPDIR',
    'USER',
)


class Failure(Exception):
    pass


def repository_root():
    directory = ROOT
    while not os.path.exists(os.path.join(directory, '.git')):
        parent = os.path.dirname(directory)
        if parent == directory:
            return ROOT
        directory = parent
    return directory


def take_folders(arguments, flag='--with'):
    folders = []
    rest = []
    index = 0
    while index < len(arguments):
        if arguments[index] != flag:
            rest.append(arguments[index])
            index += 1
            continue
        if index + 1 >= len(arguments):
            raise Failure('%s takes a folder' % flag)
        folder = os.path.abspath(arguments[index + 1])
        if not os.path.isdir(folder):
            raise Failure('%s is not a folder' % arguments[index + 1])
        folders.append(folder)
        index += 2
    return folders, rest


def say(message):
    sys.stderr.write('%s\n' % message)
    sys.stderr.flush()


def shown(path):
    relative = os.path.relpath(path, ROOT)
    return path if relative == '..' or relative.startswith('../') else relative


def built_path(path, root=BUILD):
    if path == 'build' or path.startswith('build/'):
        return os.path.join(root, os.path.relpath(path, 'build'))
    return os.path.join(ROOT, path)


def is_within(path, directory):
    directory = os.path.abspath(directory)
    return os.path.commonpath([os.path.abspath(path), directory]) == directory


def file_digest(path):
    digest = hashlib.sha256()
    with open(path, 'rb') as file:
        for chunk in iter(lambda: file.read(CHUNK_SIZE), b''):
            digest.update(chunk)
    return digest.hexdigest()


def log_tail(path):
    with open(path, errors='replace') as file:
        return ''.join(file.readlines()[-LOG_TAIL:]).rstrip('\n')


def lock(file, wait=True):
    try:
        fcntl.flock(file, fcntl.LOCK_EX if wait else fcntl.LOCK_EX | fcntl.LOCK_NB)
    except BlockingIOError:
        return False
    return True


def passed_environment():
    return {name: os.environ[name] for name in PASSED_ENVIRONMENT_NAMES if name in os.environ}


def build_environment():
    environment = passed_environment()
    environment['BRIXPATH'] = os.path.join(ROOT, 'stdlib')
    environment['BRIX_BUILD_PATH'] = BUILD
    environment['BRIX_DEBUG_LOG'] = ''
    return environment


def command_output(command):
    try:
        result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    except FileNotFoundError:
        raise Failure('%s is not installed' % command[0])
    output = result.stdout.strip()
    if result.returncode != 0:
        failed = '%s failed' % ' '.join(command)
        raise Failure('%s\n%s' % (output, failed) if output else failed)
    return output


def xcrun(*arguments):
    return command_output(['/usr/bin/xcrun'] + list(arguments))


def run_main(main):
    try:
        return main(sys.argv[1:]) or 0
    except Failure as failure:
        say(str(failure))
        return 1
    except KeyboardInterrupt:
        return 130
