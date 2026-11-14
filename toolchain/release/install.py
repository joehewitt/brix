#!/usr/bin/env python3
import argparse
import json
import os
import shutil
import subprocess
import sys
import tempfile
import types

VERSION = None
RELEASE_SHA256 = None
LAUNCHER = None

REPOSITORY = 'https://github.com/joehewitt/brix.git'
MINIMUM_MACOS = (15, 0)
MINIMUM_GLIBC = (2, 34)
MINIMUM_CLANG = 16
LINUX_COMPILER_PACKAGES = 'clang %d or later (sudo dnf install clang, or sudo apt install clang)' % MINIMUM_CLANG


class InstallError(Exception):
    pass


def main(arguments):
    options = parse_arguments(arguments)
    try:
        install(options)
    except InstallError as error:
        sys.stderr.write('\nyumi install: %s\n' % error)
        return 1
    except KeyboardInterrupt:
        sys.stderr.write('\nyumi install: stopped\n')
        return 130
    return 0


def parse_arguments(arguments):
    parser = argparse.ArgumentParser(
        prog='install.py',
        description='Install the Yumi compiler and standard library on arm64 macOS or aarch64 Linux.')
    parser.add_argument('--version', dest='release_version', metavar='VERSION',
                        help='the release to install, or latest (default: %s)' % (VERSION or 'latest'))
    parser.add_argument('--from', dest='mirror', metavar='DIR',
                        help='install from a local copy of the releases bucket instead of downloading')
    parser.add_argument('--home', metavar='DIR', help='where Yumi keeps its releases (default: ~/.yumi)')
    parser.add_argument('--source', action='store_true',
                        help='also clone the repo at the release and build the compiler from source')
    parser.add_argument('--checkout', metavar='DIR', help='where --source clones the repo (default: ~/yumi)')
    parser.add_argument('--repository', default=REPOSITORY, help='the repo --source clones (default: %(default)s)')
    parser.add_argument('-y', '--yes', action='store_true', help='answer every question with its default')
    path = parser.add_mutually_exclusive_group()
    path.add_argument('--modify-path', dest='modify_path', action='store_true', default=None,
                      help='put the yumi command on PATH without asking')
    path.add_argument('--no-modify-path', dest='modify_path', action='store_false',
                      help='leave the shell\'s startup files alone')
    return parser.parse_args(arguments)


def install(options):
    host = checked_host()
    home = os.path.realpath(os.path.expanduser(options.home or '~/.yumi'))
    os.environ['YUMI_HOME'] = home
    if options.mirror:
        mirror = os.path.realpath(os.path.expanduser(options.mirror))
        if not os.path.isdir(mirror):
            raise InstallError('%s is not a folder' % options.mirror)
        os.environ['YUMI_RELEASES_MIRROR'] = mirror
    launcher = loaded_launcher()
    version, expected = chosen_release(launcher, options.release_version or VERSION or 'latest', host)
    say('Installing Yumi %s for %s into %s' % (version, host['name'], shown(home)))
    try:
        launcher.fetch_release(version, expected)
        release = launcher.checked_release({'version': version, 'release': expected})
        launcher.install_toolchain(version, release)
    except launcher.LaunchError as error:
        raise InstallError(str(error))
    make_current(home, version)
    command = install_command(launcher, home)
    say('Checking that it compiles and runs a program')
    check_command(command)
    if options.source:
        install_source(launcher, options, host, version, release, command)
    offer_path(options, home)
    say('\nYumi %s is installed. Run yumi --help to start.' % version)


def checked_host():
    system = os.uname()
    if system.sysname == 'Darwin':
        return checked_mac()
    if system.sysname == 'Linux':
        return checked_linux(system.machine)
    raise InstallError('Yumi runs on arm64 macOS and aarch64 Linux, not %s' % system.sysname)


def checked_mac():
    if command_text(['/usr/sbin/sysctl', '-n', 'hw.optional.arm64']) != '1':
        raise InstallError('Yumi runs on Apple silicon Macs, and this Mac has an Intel processor')
    if os.uname().machine != 'arm64':
        raise InstallError('This Python runs under Rosetta; run the installer with an arm64 Python, as in: '
                           'arch -arm64 /usr/bin/python3 install.py')
    version = command_text(['/usr/bin/sw_vers', '-productVersion'])
    if version_tuple(version) < MINIMUM_MACOS:
        raise InstallError('Yumi needs macOS %s or later, and this Mac runs %s' % (dotted(MINIMUM_MACOS), version))
    if command_text(['/usr/bin/xcrun', '--find', 'ld']) is None:
        raise InstallError('Yumi links programs with Apple\'s tools; install them with: xcode-select --install')
    return {'name': 'macOS %s' % version, 'triple': 'arm64-apple-macos', 'family': 'macos'}


def checked_linux(machine):
    if machine != 'aarch64':
        raise InstallError('Yumi runs on aarch64 Linux, and this machine is %s' % machine)
    try:
        libc = os.confstr('CS_GNU_LIBC_VERSION') or ''
    except (ValueError, OSError):
        libc = ''
    if not libc.startswith('glibc '):
        raise InstallError('Yumi needs glibc %s or later, and this Linux has another C library' % dotted(MINIMUM_GLIBC))
    glibc = libc.split()[1]
    if version_tuple(glibc) < MINIMUM_GLIBC:
        raise InstallError('Yumi needs glibc %s or later, and this Linux has %s' % (dotted(MINIMUM_GLIBC), glibc))
    if 'atomics' not in cpu_features():
        raise InstallError('Yumi needs an Armv8.2 processor or later, such as Graviton2, and this one lacks '
                           'its atomic instructions')
    return {'name': 'Linux with glibc %s' % glibc, 'triple': 'aarch64-linux-gnu', 'family': 'linux'}


def cpu_features():
    try:
        with open('/proc/cpuinfo') as file:
            for line in file:
                if line.startswith('Features'):
                    return line.split(':', 1)[1].split()
    except OSError:
        pass
    return []


def loaded_launcher():
    source = LAUNCHER
    path = '<launcher>'
    if source is None:
        path = os.path.join(os.path.dirname(os.path.dirname(os.path.realpath(__file__))), 'launcher.py')
        with open(path) as file:
            source = file.read()
    module = types.ModuleType('yumi_launcher')
    module.__file__ = path
    exec(compile(source, path, 'exec'), module.__dict__)
    return module


def chosen_release(launcher, wanted, host):
    if wanted == VERSION and RELEASE_SHA256:
        return VERSION, RELEASE_SHA256
    try:
        body = fetched_text(launcher, launcher.RELEASES_PREFIX + '/releases.json')
    except launcher.LaunchError as error:
        raise InstallError('Cannot read the list of releases: %s' % error)
    releases = [entry for entry in json.loads(body)['releases'] if host['triple'] in entry.get('hosts', [])]
    if wanted != 'latest':
        releases = [entry for entry in releases if entry['version'] == wanted]
    if not releases:
        raise InstallError('No release %sis built for %s' % ('' if wanted == 'latest' else wanted + ' ', host['name']))
    chosen = max(releases, key=lambda entry: version_key(entry['version']))
    return chosen['version'], chosen['release']


def fetched_text(launcher, key):
    with tempfile.TemporaryDirectory() as directory:
        path = os.path.join(directory, 'object')
        launcher.fetch_object(key, path)
        with open(path) as file:
            return file.read()


def make_current(home, version):
    current = os.path.join(home, 'current')
    link = '%s.%d' % (current, os.getpid())
    os.symlink(os.path.join('releases', version), link)
    os.replace(link, current)


def install_command(launcher, home):
    if LAUNCHER is None:
        with open(launcher.__file__) as file:
            source = file.read()
    else:
        source = LAUNCHER
    directory = os.path.join(home, 'bin')
    os.makedirs(directory, exist_ok=True)
    command = os.path.join(directory, 'yumi')
    temporary = '%s.%d' % (command, os.getpid())
    with open(temporary, 'w') as file:
        file.write(source)
    os.chmod(temporary, 0o755)
    os.replace(temporary, command)
    return command


def check_command(command):
    with tempfile.TemporaryDirectory(prefix='yumi-install-') as directory:
        source = os.path.join(directory, 'hello.yumi')
        with open(source, 'w') as file:
            file.write('-- "Yumi compiled and ran this"\n')
        result = subprocess.run([command, source, '-o', os.path.join(directory, 'hello')], cwd=directory,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, universal_newlines=True)
        if result.returncode != 0 or 'Yumi compiled and ran this' not in result.stdout:
            raise InstallError('The installed compiler could not build a program:\n%s' % result.stdout.strip())


def install_source(launcher, options, host, version, release, command):
    git = shutil.which('git')
    if not git:
        raise InstallError('Building from source needs git')
    if host['family'] == 'linux' and clang_major() < MINIMUM_CLANG:
        raise InstallError('Building from source on Linux needs %s' % LINUX_COMPILER_PACKAGES)
    default = os.path.join(os.path.expanduser('~'), 'yumi')
    checkout = os.path.realpath(os.path.expanduser(
        options.checkout or ask_text('Clone the repo to', shown(default), options.yes)))
    if os.path.exists(checkout) and os.listdir(checkout):
        raise InstallError('%s already exists and is not empty' % checkout)
    tag = 'v' + version
    say('Cloning %s at %s into %s' % (options.repository, tag, shown(checkout)))
    run([git, 'clone', '--quiet', '--branch', tag, options.repository, checkout], 'Cloning failed')
    source = release['source']['commit']
    parent = run([git, '-C', checkout, 'rev-parse', 'HEAD^'], 'Reading the clone failed').strip()
    if parent != source:
        raise InstallError('%s is not the release of %s; its parent is %s' % (tag, source[:12], parent[:12]))
    with open(os.path.join(checkout, launcher.MANIFEST_NAME)) as file:
        manifest = json.load(file)
    if manifest.get('version') != version or manifest.get('release') != launcher.file_digest(
            os.path.join(launcher.archives_path(), version, 'release.json')):
        raise InstallError('%s at %s does not pin release %s' % (launcher.MANIFEST_NAME, tag, version))
    launcher.trust(checkout)
    try:
        for kit in launcher.release_host(release)['sourceKits']:
            launcher.install_kit(kit)
    except launcher.LaunchError as error:
        raise InstallError(str(error))
    say('Building the compiler from source, with release %s as its seed' % version)
    builder = os.path.join(checkout, 'bin', 'yumi')
    run([builder, '-c', '-- "built"'], 'Building the compiler failed', cwd=checkout)
    built = os.path.join(checkout, '.tmp', 'build', 'tools', 'yumi.o')
    expected = launcher.release_host(release)['hashes']['obj/yumi.o']
    if launcher.file_digest(built) == expected:
        say('The compiler built from source is identical to release %s' % version)
    else:
        say('The compiler built from source differs from release %s, whose C headers were read with another '
            'libclang or SDK; it works, but is not the released compiler' % version)
    say('In %s, yumi runs this build; elsewhere it runs release %s' % (shown(checkout), version))


def clang_major():
    found = command_text(['clang', '--version']) or ''
    words = found.split()
    if 'version' not in words or words.index('version') + 1 == len(words):
        return 0
    return (version_tuple(words[words.index('version') + 1]) or (0,))[0]


def offer_path(options, home):
    directory = os.path.join(home, 'bin')
    if directory in [os.path.realpath(part) for part in os.environ.get('PATH', '').split(':') if part]:
        return
    startup, line = shell_startup(directory)
    if not startup:
        say('Add %s to PATH to run yumi' % directory)
        return
    if os.path.isfile(startup):
        with open(startup) as file:
            if line in file.read().splitlines():
                return
    wanted = options.modify_path
    if wanted is None:
        wanted = ask_yes('Add %s to PATH in %s?' % (shown(directory), shown(startup)), True, options.yes)
    if not wanted:
        say('Add %s to PATH to run yumi' % directory)
        return
    os.makedirs(os.path.dirname(startup), exist_ok=True)
    with open(startup, 'a') as file:
        file.write('\n%s\n' % line)
    say('Added to %s:\n    %s\nOpen a new terminal to use it' % (shown(startup), line))


def shell_startup(directory):
    shell = os.path.basename(os.environ.get('SHELL', ''))
    shown_directory = directory.replace(os.path.expanduser('~'), '$HOME', 1)
    if shell == 'zsh':
        return os.path.expanduser('~/.zshrc'), 'export PATH="%s:$PATH"' % shown_directory
    if shell == 'bash':
        startup = '~/.bash_profile' if os.uname().sysname == 'Darwin' else '~/.bashrc'
        return os.path.expanduser(startup), 'export PATH="%s:$PATH"' % shown_directory
    if shell == 'fish':
        return os.path.expanduser('~/.config/fish/conf.d/yumi.fish'), 'fish_add_path %s' % directory
    return None, None


def ask_yes(question, default, assume):
    answer = ask(question, 'Y/n' if default else 'y/N', assume)
    if answer is None or not answer:
        return default
    return answer.lower() in ('y', 'yes')


def ask_text(question, default, assume):
    answer = ask(question, default, assume)
    return answer or default


def ask(question, hint, assume):
    if assume:
        return None
    try:
        terminal = open('/dev/tty', 'r+')
    except OSError:
        return None
    with terminal:
        terminal.write('%s [%s] ' % (question, hint))
        terminal.flush()
        return terminal.readline().strip()


def run(command, message, cwd=None):
    result = subprocess.run(command, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            universal_newlines=True)
    if result.returncode != 0:
        raise InstallError('%s:\n%s' % (message, result.stdout.strip()[-4000:]))
    return result.stdout


def command_text(command):
    try:
        result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, universal_newlines=True)
    except OSError:
        return None
    return result.stdout.strip() if result.returncode == 0 else None


def version_tuple(text):
    parts = []
    for part in (text or '').split('.'):
        digits = ''.join(char for char in part if char.isdigit())
        if not digits:
            break
        parts.append(int(digits))
    return tuple(parts)


def version_key(version):
    date, counter = version.rsplit('-', 1)
    return date, int(counter)


def dotted(version):
    return '.'.join(str(part) for part in version)


def shown(path):
    home = os.path.expanduser('~')
    return '~' + path[len(home):] if path == home or path.startswith(home + '/') else path


def say(message):
    sys.stderr.write('%s\n' % message)
    sys.stderr.flush()


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
