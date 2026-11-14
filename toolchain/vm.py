#!/usr/bin/env python3
import argparse
import json
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.realpath(__file__)))

from common import TMP, Failure, run_main, say

HOME = os.path.expanduser('~')
LIMA_TEMPLATES = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'lima')
CONTAINER_IMAGE = 'amazonlinux:2023'
DISTROS = {
    'amazon': {'template': os.path.join(LIMA_TEMPLATES, 'amazon-2023.yaml'), 'desktop': False},
    'debian': {'template': 'template:debian-13', 'desktop': True},
    'ubuntu': {'template': 'template:ubuntu-24.04', 'desktop': False},
    'fedora': {'template': 'template:fedora-44', 'desktop': False},
}
DEBIAN_DESKTOP = r'''set -eu
export DEBIAN_FRONTEND=noninteractive
apt-get update -q
apt-get install -y -q linux-image-arm64 gnome-core dconf-cli mesa-vulkan-drivers vulkan-tools
echo 'linux-base linux-base/removing-running-kernel boolean false' | debconf-set-selections
cloud=$(dpkg -l 'linux-image-*cloud*' 2>/dev/null | awk '/^ii/ {print $2}')
if [ -n "$cloud" ]; then
    apt-get purge -y -q $cloud
fi
sed -i "s/^#\? *AutomaticLoginEnable *=.*/AutomaticLoginEnable = true/; s/^#\? *AutomaticLogin *=.*/AutomaticLogin = $1/" /etc/gdm3/daemon.conf
mkdir -p /etc/dconf/profile /etc/dconf/db/local.d
printf 'user-db:user\nsystem-db:local\n' > /etc/dconf/profile/user
printf '[org/gnome/desktop/screensaver]\nlock-enabled=false\n\n[org/gnome/desktop/session]\nidle-delay=uint32 0\n' > /etc/dconf/db/local.d/00-yumi
dconf update
systemctl set-default graphical.target
apt-get clean
'''


def main(arguments):
    options = parse_arguments(arguments)
    if options.action == 'list':
        return list_machines()
    if options.action == 'stop':
        return stop(options)
    name = machine_name(options)
    if options.clean:
        return clean_action(name, options)
    if options.action == 'start':
        start(name, options)
    elif options.action == 'provision':
        ready(name, existing(name))
        provision(name, options)
    elif options.action == 'shell':
        shell(name, options)
    else:
        raise Failure('vm.py: %s is only for a clean VM; add --clean' % options.action)
    return 0


def clean_action(name, options):
    if options.action == 'start':
        start_clean(name, options.distro, options.fresh, options.cpus, options.memory)
    elif options.action == 'delete':
        delete(name)
    elif options.action == 'copy':
        copy_into(name, options.paths[:-1], options.paths[-1])
    elif options.action == 'shell':
        existing(name)
        arguments = ['limactl', 'shell', '--workdir', '/', name] + (['--'] + options.command if options.command else [])
        os.execvp('limactl', arguments)
    else:
        raise Failure('vm.py: a clean VM has no %s' % options.action)
    return 0


def parse_arguments(arguments):
    parser = argparse.ArgumentParser(
        prog='vm.py',
        description='Create, start and stop the Lima VMs that run Linux builds. Each VM mounts the '
                    'Mac home directory read-only and this worktree\'s .tmp writable, at the same paths. '
                    'A clean VM, named yumi-DISTRO-clean, mounts nothing of the Mac; files reach it by copy.')
    actions = parser.add_subparsers(dest='action', required=True)
    actions.add_parser('list', help='list the yumi VMs')
    start_parser = actions.add_parser('start', help='start a VM, creating and provisioning it if missing')
    add_machine_arguments(start_parser)
    start_parser.add_argument('--fresh', action='store_true', help='with --clean, delete the VM first and create it again')
    start_parser.add_argument('--cpus', type=int, default=8, help='CPUs for a new VM (default: 8)')
    start_parser.add_argument('--memory', type=int, default=8, help='GiB of memory for a new VM (default: 8)')
    start_parser.add_argument('--disk', type=int, default=100, help='GiB of disk for a new VM (default: 100)')
    stop_parser = actions.add_parser('stop', help='stop a VM')
    add_machine_arguments(stop_parser)
    stop_parser.add_argument('--all', action='store_true', help='stop every running yumi VM')
    provision_parser = actions.add_parser('provision', help='run a VM\'s provisioning again')
    add_machine_arguments(provision_parser)
    shell_parser = actions.add_parser('shell', help='run a command in a VM, or open a shell')
    add_machine_arguments(shell_parser)
    shell_parser.add_argument('--container', nargs='?', const=CONTAINER_IMAGE, metavar='IMAGE',
                              help='run in a container in the VM (default image: %s)' % CONTAINER_IMAGE)
    shell_parser.add_argument('command', nargs=argparse.REMAINDER, help='the command, after --')
    delete_parser = actions.add_parser('delete', help='delete a clean VM')
    add_machine_arguments(delete_parser)
    copy_parser = actions.add_parser('copy', help='copy files into a clean VM')
    add_machine_arguments(copy_parser)
    copy_parser.add_argument('paths', nargs='+', metavar='path', help='the files to copy, then the folder in the VM')
    options = parser.parse_args(arguments)
    if getattr(options, 'command', None) and options.command[0] == '--':
        options.command = options.command[1:]
    return options


def add_machine_arguments(parser):
    parser.add_argument('--distro', choices=sorted(DISTROS), default='debian', help='the distribution (default: debian)')
    parser.add_argument('--desktop', action='store_true', help='the GNOME desktop VM, not the server VM')
    parser.add_argument('--clean', action='store_true', help='the VM that mounts nothing of the Mac')
    parser.add_argument('--name', help='a VM name other than yumi-DISTRO or yumi-DISTRO-desktop')


def machine_name(options):
    if options.desktop and not DISTROS[options.distro]['desktop']:
        raise Failure('vm.py: a desktop VM is set up only for debian')
    if options.clean and options.desktop:
        raise Failure('vm.py: a clean VM has no desktop')
    if options.name:
        return options.name
    if options.clean:
        return clean_name(options.distro)
    return 'yumi-%s%s' % (options.distro, '-desktop' if options.desktop else '')


def clean_name(distro):
    return 'yumi-%s-clean' % distro


def machines():
    result = subprocess.run(['limactl', 'list', '--json'], capture_output=True, text=True)
    if result.returncode != 0:
        raise Failure('vm.py: limactl list failed\n%s' % result.stderr.strip())
    found = {}
    for line in result.stdout.splitlines():
        if line.strip():
            machine = json.loads(line)
            found[machine['name']] = machine
    return found


def existing(name):
    machine = machines().get(name)
    if not machine:
        raise Failure('vm.py: no VM named %s; create it with vm.py start' % name)
    return machine


def list_machines():
    for name, machine in sorted(machines().items()):
        if not name.startswith('yumi-'):
            continue
        writable = [shortened(mount['location']) for mount in machine['config'].get('mounts', []) if mount.get('writable')]
        say('%-22s %-8s %2d CPUs  %3d GiB memory  %4d GiB disk  writable: %s' % (
            name, machine['status'], machine['cpus'], machine['memory'] >> 30, machine['disk'] >> 30,
            ', '.join(writable) or 'none'))
    return 0


def shortened(path):
    return '~' + path[len(HOME):] if path == HOME or path.startswith(HOME + '/') else path


def start(name, options):
    machine = machines().get(name)
    if not machine:
        create(name, options)
        limactl('start', name)
        provision(name, options)
    elif machine['status'] == 'Running' and has_mount(machine):
        say('%s is running' % name)
    else:
        ready(name, machine)


def ready(name, machine):
    if not has_mount(machine):
        add_mount(name, machine)
    elif machine['status'] != 'Running':
        limactl('start', name)


def create(name, options):
    template = DISTROS[options.distro]['template']
    say('Creating %s from %s' % (name, shortened(template)))
    flags = ['--name=' + name, '--vm-type=vz', '--cpus=%d' % options.cpus,
             '--memory=%d' % options.memory, '--disk=%d' % options.disk, '--set', mount_expression()]
    flags.append('--video' if options.desktop else '--containerd=user')
    os.makedirs(TMP, exist_ok=True)
    limactl('create', *flags, template)


def provision(name, options):
    if options.desktop:
        user = guest(name, ['id', '-un'], capture=True).strip()
        say('Installing GNOME and the standard kernel in %s' % name)
        guest(name, ['sudo', 'sh', '-s', user], stdin=DEBIAN_DESKTOP)
        say('Restarting %s on the standard kernel' % name)
        limactl('stop', name)
        limactl('start', name)
    else:
        say('Pulling %s in %s' % (CONTAINER_IMAGE, name))
        guest(name, ['nerdctl', 'pull', '--quiet', CONTAINER_IMAGE])


def has_mount(machine):
    return any(mount['location'] == TMP and mount.get('writable') for mount in machine['config'].get('mounts', []))


def mount_expression():
    return '.mounts += [%s]' % json.dumps({'location': TMP, 'writable': True})


def add_mount(name, machine):
    running = machine['status'] == 'Running'
    if running:
        say('Restarting %s to mount %s writable' % (name, shortened(TMP)))
        limactl('stop', name)
    else:
        say('Mounting %s writable in %s' % (shortened(TMP), name))
    os.makedirs(TMP, exist_ok=True)
    limactl('edit', name, '--set', mount_expression())
    limactl('start', name)


def start_clean(name, distro, fresh=False, cpus=8, memory=8):
    machine = machines().get(name)
    if machine and fresh:
        delete(name)
        machine = None
    if not machine:
        template = DISTROS[distro]['template']
        say('Creating %s from %s, with nothing of the Mac mounted' % (name, shortened(template)))
        limactl('create', '--name=' + name, '--vm-type=vz', '--plain', '--cpus=%d' % cpus,
                '--memory=%d' % memory, '--disk=40', template)
    if (machines().get(name) or {}).get('status') != 'Running':
        limactl('start', name)
    return name


def delete(name):
    existing(name)
    say('Deleting %s' % name)
    limactl('delete', '--force', name)


def copy_into(name, paths, destination):
    existing(name)
    if run_in(name, ['mkdir', '-p', destination])[0] != 0:
        raise Failure('vm.py: making %s in %s failed' % (destination, name))
    for path in paths:
        if subprocess.run(['limactl', 'copy', '--recursive', path, '%s:%s' % (name, destination)]).returncode != 0:
            raise Failure('vm.py: copying %s into %s failed' % (path, name))


def run_in(name, command, stdin=None, capture=False):
    result = subprocess.run(['limactl', 'shell', '--workdir', '/', name, '--'] + command, input=stdin, text=True,
                            stdout=subprocess.PIPE if capture else None,
                            stderr=subprocess.STDOUT if capture else None)
    return result.returncode, result.stdout


def stop(options):
    found = machines()
    if options.all:
        names = [name for name, machine in sorted(found.items())
                 if name.startswith('yumi-') and machine['status'] == 'Running']
    else:
        names = [machine_name(options)]
        if names[0] not in found:
            raise Failure('vm.py: no VM named %s' % names[0])
    for name in names:
        if found[name]['status'] == 'Running':
            limactl('stop', name)
        else:
            say('%s is not running' % name)
    return 0


def shell(name, options):
    ready(name, existing(name))
    command = list(options.command)
    if options.container:
        flags = ['-it'] if sys.stdin.isatty() else ['-i']
        command = ['nerdctl', 'run', '--rm'] + flags + [
            '-v', '%s:%s:ro' % (HOME, HOME), '-v', '%s:%s' % (TMP, TMP), '-w', os.getcwd(),
            options.container] + (command or ['sh'])
    arguments = ['limactl', 'shell', name] + (['--'] + command if command else [])
    os.execvp('limactl', arguments)


def guest(name, command, stdin=None, capture=False):
    result = subprocess.run(['limactl', 'shell', name, '--'] + command, input=stdin, text=True,
                            stdout=subprocess.PIPE if capture else None)
    if result.returncode != 0:
        raise Failure('vm.py: %s failed in %s' % (' '.join(command), name))
    return result.stdout


def limactl(action, *arguments):
    if subprocess.run(['limactl', action, '--tty=false'] + list(arguments)).returncode != 0:
        raise Failure('vm.py: limactl %s failed' % ' '.join([action] + list(arguments)))


if __name__ == '__main__':
    sys.exit(run_main(main))
