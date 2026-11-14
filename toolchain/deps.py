import hashlib
import json
import os
import shutil
import subprocess
import tempfile

import launcher
from common import ROOT, RUNTIME_CONFIG, YUMI_HOME, Failure, file_digest, lock, log_tail, say, xcrun

RECIPES = os.path.join(ROOT, 'toolchain', 'deps')
CACHE = os.path.join(YUMI_HOME, 'deps')
DOWNLOADS = os.path.join(CACHE, 'downloads')
LOGS = os.path.join(CACHE, 'logs')
HOST_TARGETS = {'Linux': 'linux'}


def ensure_dependencies(build_root, target=None, names=None):
    target = target or host_target()
    keys = {}
    for name in default_recipe_names(target) if names is None else names:
        link_path(os.path.join(build_root, 'deps', name), ensure_recipe(name, keys, target))


def ensure_dependency(name, target=None):
    return ensure_recipe(name, {}, target or host_target())


def host_target():
    name = HOST_TARGETS.get(os.uname().sysname)
    if not name:
        return None
    with open(RUNTIME_CONFIG) as file:
        return json.load(file)['targets'][name]


def dependency_kit(name, target=None):
    keys = {}
    prefix = ensure_recipe(name, keys, target)
    return prefix, keys[name]


def dependency_prefix(name, keys):
    return os.path.join(CACHE, '%s-%s' % (name, keys[name][:16]))


def recipe_names():
    return sorted(name for name in os.listdir(RECIPES)
                  if os.path.isfile(os.path.join(RECIPES, name, 'recipe.json')))


def default_recipe_names(target=None):
    names = []
    for name in recipe_names():
        recipe = read_recipe(name)
        if not recipe.get('optional') and (not target or recipe.get('targetable') or recipe.get('portable')):
            names.append(name)
    return names


def recipe_order(wanted):
    names = recipe_names()
    ordered = []
    visiting = set()

    def visit(name):
        if name in ordered:
            return
        if name in visiting:
            raise Failure('Dependency recipes require each other through %s' % name)
        visiting.add(name)
        for required in read_recipe(name).get('requires', []):
            if required not in names:
                raise Failure('Dependency %s requires unknown %s' % (name, required))
            visit(required)
        visiting.remove(name)
        ordered.append(name)

    for name in wanted:
        if name not in names:
            raise Failure('No dependency recipe named %s' % name)
        visit(name)
    return ordered


def read_recipe(name):
    with open(os.path.join(RECIPES, name, 'recipe.json')) as file:
        return json.load(file)


def recipe_key(name, keys, target=None):
    digest = hashlib.sha256()
    if target:
        digest.update(('target=%s\0' % target['triple']).encode())
        if 'sysroot' in target:
            digest.update(('sysroot=%s\0' % recipe_key(sysroot_name(target), {})).encode())
    recipe = os.path.join(RECIPES, name)
    for directory, names, file_names in os.walk(recipe):
        names.sort()
        for file_name in sorted(file_names):
            path = os.path.join(directory, file_name)
            digest.update(os.path.relpath(path, recipe).encode() + b'\0')
            with open(path, 'rb') as file:
                digest.update(file.read() + b'\0')
    for required in read_recipe(name).get('requires', []):
        digest.update(('%s=%s\0' % (required, keys[required])).encode())
    return digest.hexdigest()


def ensure_recipe(name, keys, target=None):
    built = built_target(name, target)
    if built and not read_recipe(name).get('targetable'):
        raise Failure('Dependency %s cannot be built for %s' % (name, built['triple']))
    for recipe_name in recipe_order([name]):
        if recipe_name not in keys:
            keys[recipe_name] = recipe_key(recipe_name, keys, built_target(recipe_name, target))
    prefix = dependency_prefix(name, keys)
    if os.path.isdir(prefix):
        return prefix
    os.makedirs(CACHE, exist_ok=True)
    with open(os.path.join(CACHE, '.%s.lock' % os.path.basename(prefix)), 'w') as file:
        lock(file)
        if not os.path.isdir(prefix):
            unfetched = fetch_kit(name, keys[name])
            if unfetched:
                for required in read_recipe(name).get('requires', []):
                    ensure_recipe(required, keys, target)
                try:
                    build_recipe(name, keys, prefix, built)
                except Failure as error:
                    raise Failure('%s\nNo kit for it could be fetched either: %s' % (error, unfetched))
    return prefix


def built_target(name, target):
    if not target:
        return None
    if read_recipe(name).get('portable') or ('sysroot' in target and name == sysroot_name(target)):
        return None
    return target


def fetch_kit(name, key):
    try:
        launcher.install_kit({'name': name, 'key': key, 'file': launcher.kit_file(name, key)})
    except launcher.LaunchError as error:
        return str(error)
    return None


def build_recipe(name, keys, prefix, target=None):
    recipe = read_recipe(name)
    work = tempfile.mkdtemp(prefix='.build-%s-' % name, dir=CACHE)
    try:
        sources = os.path.join(work, 'sources')
        for source in recipe['sources']:
            archive = download(source)
            extracted = os.path.join(sources, source['name'])
            os.makedirs(extracted)
            run(['tar', '-xf', archive, '-C', extracted, '--strip-components', '1'],
                'Unpacking %s failed' % archive)
        staging = os.path.join(work, 'prefix')
        os.makedirs(staging)
        environment = dict(os.environ)
        environment['SOURCES'] = sources
        environment['PREFIX'] = staging
        environment['RECIPE'] = os.path.join(RECIPES, name)
        environment['JOBS'] = str(os.cpu_count() or 4)
        if target:
            environment['TARGET_TRIPLE'] = target['triple']
            if 'sysroot' in target:
                sysroot = ensure_dependency(sysroot_name(target))
                environment['TARGET_SYSROOT'] = sysroot
                environment['TARGET_GCC_INSTALL_DIR'] = os.path.join(sysroot, target['gccInstallDirectory'])
                environment['TARGET_LINKER'] = host_tool_path(target['linker'])
            else:
                environment['SDKROOT'] = xcrun('--sdk', target['sdk'], '--show-sdk-path')
        for required in recipe.get('requires', []):
            environment['%s_PREFIX' % required.upper().replace('-', '_')] = dependency_prefix(required, keys)
        os.makedirs(LOGS, exist_ok=True)
        log = os.path.join(LOGS, '%s-%s.log' % (name, keys[name][:16]))
        say('Building %s %s' % (name, recipe['version']))
        with open(log, 'w') as output:
            result = subprocess.run(['/bin/bash', os.path.join(RECIPES, name, 'build.sh')],
                                    cwd=work, env=environment, stdout=output, stderr=subprocess.STDOUT)
        if result.returncode != 0:
            raise Failure('%s\nBuilding %s failed; the full log is %s' % (log_tail(log), name, log))
        os.replace(staging, prefix)
    finally:
        shutil.rmtree(work, ignore_errors=True)


def sysroot_name(target):
    return target['sysroot'].split('/')[2]


def host_tool_path(path):
    parts = path.split('/')
    return os.path.join(ensure_dependency(parts[2]), *parts[3:])


def download(source):
    expected = source['sha256']
    path = os.path.join(DOWNLOADS, expected)
    if os.path.isfile(path) and file_digest(path) == expected:
        return path
    os.makedirs(DOWNLOADS, exist_ok=True)
    temporary = '%s.part-%d' % (path, os.getpid())
    say('Downloading %s' % source['url'])
    run(['curl', '-fsSL', '-o', temporary, source['url']], 'Downloading %s failed' % source['url'])
    found = file_digest(temporary)
    if found != expected:
        os.remove(temporary)
        raise Failure('%s has sha256 %s, expected %s' % (source['url'], found, expected))
    os.replace(temporary, path)
    return path


def run(command, message):
    result = subprocess.run(command, capture_output=True, text=True)
    if result.returncode != 0:
        output = (result.stderr or result.stdout).strip()
        raise Failure('%s\n%s' % (output, message) if output else message)
    return result.stdout


def link_path(path, target):
    if os.path.islink(path) and os.readlink(path) == target:
        return
    os.makedirs(os.path.dirname(path), exist_ok=True)
    temporary = '%s.link-%d' % (path, os.getpid())
    os.symlink(target, temporary)
    os.replace(temporary, path)
