#!/usr/bin/env python3
import base64
import calendar
import configparser
import contextlib
import fcntl
import hashlib
import hmac
import json
import lzma
import os
import re
import shutil
import stat
import sys
import tarfile
import time
import urllib.error
import urllib.parse
import urllib.request

BRIX_HOME = os.path.realpath(os.environ.get('BRIX_HOME') or os.path.expanduser('~/.brix'))
MANIFEST_NAME = '.release.json'
USES = ('toolchain', 'seed')
VERSION_PATTERN = re.compile(r'\d{4}\.\d{2}\.\d{2}-\d+')
FILE_PATTERN = re.compile(r'[A-Za-z0-9][A-Za-z0-9._-]*')
PART_OVERRIDES = ('BRIXPATH', 'BRIX_BUILD_PATH')
CHUNK_SIZE = 1 << 20
RELEASES_BUCKET = 'joehewitt-brix-releases'
RELEASES_REGION = 'us-west-2'
RELEASES_PREFIX = 'releases'
KITS_PREFIX = RELEASES_PREFIX + '/deps'
RELEASES_MIRROR = os.environ.get('BRIX_RELEASES_MIRROR') or None
HOSTS = {('Darwin', 'arm64'): 'arm64-apple-macos', ('Linux', 'aarch64'): 'aarch64-linux-gnu'}
AWS_PROFILE = 'brix'
SIGN_IN = 'aws sso login --profile %s' % AWS_PROFILE
TOKEN_REFRESH_WINDOW = 300
HTTP_TIMEOUT = 120
TIME_FORMAT = '%Y-%m-%dT%H:%M:%SZ'

checked_files = None
aws_session = None


class LaunchError(Exception):
    pass


class S3Error(LaunchError):
    def __init__(self, message, status=None):
        LaunchError.__init__(self, message)
        self.status = status


def main(arguments):
    try:
        executable = launched_compiler(os.getcwd())
    except LaunchError as error:
        sys.stderr.write('brix: %s\n' % error)
        return 1
    environment = {name: value for name, value in os.environ.items() if name not in PART_OVERRIDES}
    os.execve(executable, [executable] + arguments, environment)


def launched_compiler(directory):
    manifest_path = find_manifest(directory)
    if manifest_path is None:
        return current_compiler(directory)
    manifest = read_manifest(manifest_path)
    if manifest['use'] == 'seed':
        return repo_compiler(manifest_path)
    return toolchain_compiler(manifest)


def repo_compiler(manifest_path):
    root = os.path.dirname(os.path.realpath(manifest_path))
    builder = os.path.join(root, 'bin', 'brix')
    if not os.access(builder, os.X_OK):
        raise LaunchError('%s pins a seed, but there is no bin/brix beside it to build with' % manifest_path)
    if not is_trusted(root):
        approve(root)
    return builder


def is_trusted(root):
    for folder in trusted_folders(read_config()):
        folder = os.path.realpath(os.path.expanduser(folder))
        if root == folder or root.startswith(os.path.join(folder, '')):
            return True
    return False


def approve(root):
    question = '%s pins a seed, so brix would run its own bin/brix.' % root
    if not (sys.stdin and sys.stdin.isatty() and sys.stderr.isatty()):
        raise LaunchError('%s Approve it by running brix there in a terminal, or add the folder to "trusted" in %s'
                          % (question, config_path()))
    sys.stderr.write('brix: %s Trust this folder to run its code? [y/N] ' % question)
    sys.stderr.flush()
    if sys.stdin.readline().strip().lower() not in ('y', 'yes'):
        raise LaunchError('%s is not trusted, so nothing ran' % root)
    trust(root)


def trust(root):
    with install_lock():
        config = read_config()
        config['trusted'] = trusted_folders(config) + [root]
        write_json(config_path(), config)


def trusted_folders(config):
    folders = config.get('trusted', [])
    if not isinstance(folders, list) or not all(isinstance(folder, str) for folder in folders):
        raise LaunchError('"trusted" in %s must be a list of folders' % config_path())
    return folders


def config_path():
    return os.path.join(BRIX_HOME, 'config.json')


def read_config():
    try:
        with open(config_path()) as file:
            config = json.load(file)
    except FileNotFoundError:
        return {}
    except (OSError, ValueError) as error:
        raise LaunchError('Cannot read %s: %s' % (config_path(), error))
    if not isinstance(config, dict):
        raise LaunchError('%s is not a JSON object' % config_path())
    return config


def find_manifest(directory):
    directory = os.path.realpath(directory)
    while True:
        path = os.path.join(directory, MANIFEST_NAME)
        if os.path.isfile(path):
            return path
        parent = os.path.dirname(directory)
        if parent == directory:
            return None
        directory = parent


def read_manifest(path):
    try:
        with open(path) as file:
            manifest = json.load(file)
    except (OSError, ValueError) as error:
        raise LaunchError('Cannot read %s: %s' % (path, error))
    if not isinstance(manifest, dict):
        raise LaunchError('%s is not a JSON object' % path)
    for name in ('version', 'release'):
        if not isinstance(manifest.get(name), str) or not manifest[name]:
            raise LaunchError('%s names no %s' % (path, name))
    if not VERSION_PATTERN.fullmatch(manifest['version']):
        raise LaunchError('%s names version "%s"; versions look like 2026.10.23-1' % (path, manifest['version']))
    manifest.setdefault('use', 'toolchain')
    if manifest['use'] not in USES:
        raise LaunchError('%s has "use": "%s"; it must be one of %s' % (path, manifest['use'], ', '.join(USES)))
    return manifest


def current_compiler(directory):
    current = os.path.join(BRIX_HOME, 'current')
    if not os.path.islink(current):
        raise LaunchError('No %s above %s, and %s names no release' % (MANIFEST_NAME, directory, current))
    version = os.path.basename(os.readlink(current))
    release_path = os.path.join(BRIX_HOME, 'archives', version, 'release.json')
    if not os.path.isfile(release_path):
        raise LaunchError('%s names release %s, which is not in %s' % (current, version, archives_path()))
    return toolchain_compiler({'version': version, 'release': file_digest(release_path)})


def toolchain_compiler(manifest):
    release = checked_release(manifest)
    package_path(manifest['version'], release, 'toolchain')
    root = os.path.join(BRIX_HOME, 'releases', manifest['version'])
    if not os.path.isdir(root):
        install_toolchain(manifest['version'], release)
    return checked_binary(root, os.path.join('bin', executable_name(release)), release)


def seed_compiler(manifest):
    release = checked_release(manifest)
    package_path(manifest['version'], release, 'seed')
    directory = os.path.join(BRIX_HOME, 'seeds', manifest['version'])
    if not os.path.isfile(os.path.join(directory, executable_name(release))):
        install_seed(manifest['version'], release)
    return checked_binary(directory, executable_name(release), release)


def checked_binary(directory, name, release):
    record_path = os.path.join(directory, 'relinked', 'relink.json')
    if os.path.isfile(record_path):
        with open(record_path) as file:
            record = json.load(file)
        executable = os.path.join(directory, 'relinked', executable_name(release))
        check_file(executable, record['sha256'])
        return executable
    executable = os.path.join(directory, name)
    check_file(executable, release_host(release)['hashes']['bin/' + executable_name(release)])
    return executable


def executable_name(release):
    return 'yumi' if 'bin/yumi' in release_host(release)['hashes'] else 'brix'


def checked_release(manifest):
    version = manifest['version']
    path = os.path.join(archives_path(), version, 'release.json')
    if not os.path.isfile(path):
        fetch_release(version, manifest['release'])
    check_file(path, manifest['release'])
    with open(path) as file:
        release = json.load(file)
    release_host(release)
    return release


def host_target():
    system = os.uname()
    host = HOSTS.get((system.sysname, system.machine))
    if not host:
        raise LaunchError('Brix runs on arm64 macOS and aarch64 Linux, not %s on %s' % (system.sysname, system.machine))
    return host


def release_host(release, host=None):
    host = host or host_target()
    if 'hosts' not in release:
        raise LaunchError('Release %s has a release.json older than this launcher reads' % release['version'])
    found = release['hosts'].get(host)
    if not found:
        raise LaunchError('Release %s has no build for %s' % (release['version'], host))
    return found


def archives_path():
    return os.path.join(BRIX_HOME, 'archives')


def package_path(version, release, name, host=None):
    for package in release_host(release, host)['packages']:
        if package['name'] == name:
            path = os.path.join(archives_path(), version, package_file(package))
            check_file(path, package['sha256'])
            return path
    raise LaunchError('Release %s has no %s package' % (version, name))


def package_file(package):
    name = package.get('file')
    if not isinstance(name, str) or not FILE_PATTERN.fullmatch(name):
        raise LaunchError('The release names package file %r, which is not a plain file name' % (name,))
    return name


def install_toolchain(version, release):
    releases = os.path.join(BRIX_HOME, 'releases')
    destination = os.path.join(releases, version)
    with install_lock():
        if os.path.isdir(destination):
            return
        staged = staging_directory(releases, version)
        extract(package_path(version, release, 'toolchain'), staged)
        for kit in release_host(release)['kits']:
            link = os.path.join(staged, 'build', 'deps', kit['name'])
            os.makedirs(os.path.dirname(link), exist_ok=True)
            os.symlink(install_kit(kit), link)
        shutil.copy2(os.path.join(archives_path(), version, 'release.json'), os.path.join(staged, 'release.json'))
        publish(staged, destination)


def kit_file(name, key):
    return '%s-%s.tar.xz' % (name, key[:16])


def kit_directory(name, key):
    return os.path.join(BRIX_HOME, 'deps', '%s-%s' % (name, key[:16]))


def kit_archive(file):
    return os.path.join(BRIX_HOME, 'deps', 'kits', file)


def install_kit(kit):
    destination = kit_directory(kit['name'], kit['key'])
    if os.path.isdir(destination):
        return destination
    archive = fetch_kit(kit['file'], kit.get('sha256'))
    staged = '%s.staging-%d' % (destination, os.getpid())
    remove_tree(staged)
    extract(archive, staged, links=True)
    try:
        os.rename(staged, destination)
    except OSError:
        if not os.path.isdir(destination):
            raise
        remove_tree(staged)
    return destination


def fetch_kit(file, expected=None):
    if not FILE_PATTERN.fullmatch(file):
        raise LaunchError('A release names kit %r, which is not a plain file name' % (file,))
    archive = kit_archive(file)
    if os.path.isfile(archive):
        if expected:
            check_file(archive, expected)
        return archive
    os.makedirs(os.path.dirname(archive), exist_ok=True)
    sys.stderr.write('brix: downloading %s\n' % file)
    temporary = '%s.part-%d' % (archive, os.getpid())
    try:
        digest, size = fetch_object('%s/%s' % (KITS_PREFIX, file), temporary)
        if expected and digest != expected:
            raise LaunchError('%s has sha256 %s, not the %s its release names' % (file, digest, expected))
        os.replace(temporary, archive)
    finally:
        if os.path.exists(temporary):
            os.remove(temporary)
    return archive


def fetch_object(key, path=None):
    if not RELEASES_MIRROR:
        return s3_download(key, path)
    source = os.path.join(RELEASES_MIRROR, key)
    if not os.path.isfile(source):
        raise S3Error('%s is not in the mirror %s' % (key, RELEASES_MIRROR), 404)
    with open(source, 'rb') as file:
        digest = stream_digest(file)
    if path:
        shutil.copyfile(source, path)
    return digest, os.path.getsize(source)


def install_seed(version, release):
    seeds = os.path.join(BRIX_HOME, 'seeds')
    destination = os.path.join(seeds, version)
    with install_lock():
        if os.path.isdir(destination):
            return
        staged = staging_directory(seeds, version)
        executable = os.path.join(staged, executable_name(release))
        with lzma.open(package_path(version, release, 'seed'), 'rb') as source, open(executable, 'wb') as output:
            shutil.copyfileobj(source, output, CHUNK_SIZE)
        os.chmod(executable, 0o755)
        publish(staged, destination)


@contextlib.contextmanager
def install_lock():
    os.makedirs(BRIX_HOME, exist_ok=True)
    with open(os.path.join(BRIX_HOME, 'install.lock'), 'w') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        yield


def staging_directory(parent, version):
    os.makedirs(parent, exist_ok=True)
    path = os.path.join(parent, '.%s.staging' % version)
    remove_tree(path)
    os.makedirs(path)
    return path


def publish(staged, destination):
    os.rename(staged, destination)
    make_read_only(destination)


def make_read_only(root):
    for directory, directories, file_names in os.walk(root):
        for file_name in file_names:
            path = os.path.join(directory, file_name)
            mode = stat.S_IMODE(os.lstat(path).st_mode)
            os.chmod(path, mode & ~(stat.S_IWUSR | stat.S_IWGRP | stat.S_IWOTH))
    for directory, directories, file_names in os.walk(root, topdown=False):
        os.chmod(directory, 0o555)


def remove_tree(path):
    if not os.path.lexists(path):
        return
    for directory, directories, file_names in os.walk(path):
        os.chmod(directory, 0o755)
    shutil.rmtree(path)


def extract(package, destination, links=False):
    os.makedirs(destination, exist_ok=True)
    with tarfile.open(package, 'r:xz') as archive:
        members = checked_members(package, archive, links)
        if hasattr(tarfile, 'data_filter'):
            archive.extractall(destination, members, filter='data')
        else:
            archive.extractall(destination, members)


def checked_members(package, archive, links=False):
    members = archive.getmembers()
    for member in members:
        if not (member.isfile() or member.isdir() or (links and member.issym())):
            raise LaunchError('%s holds %s, which is not a plain file or folder' % (package, member.name))
        parts = member.name.split('/')
        if member.name.startswith('/') or '..' in parts:
            raise LaunchError('%s holds %s, which would land outside its folder' % (package, member.name))
        if member.issym():
            target = os.path.normpath(os.path.join(os.path.dirname(member.name), member.linkname))
            if member.linkname.startswith('/') or target == '..' or target.startswith('../'):
                raise LaunchError('%s holds the link %s to %s, outside its folder' % (package, member.name, member.linkname))
        member.mode &= 0o755
    return members


def check_file(path, expected):
    try:
        status = os.stat(path)
    except OSError:
        raise LaunchError('%s is missing' % path)
    checked = read_checked()
    entry = [status.st_size, status.st_mtime_ns, expected]
    if checked.get(path) == entry:
        return
    if file_digest(path) != expected:
        raise LaunchError('%s does not match its release; its sha256 should be %s' % (path, expected))
    checked[path] = entry
    write_checked(checked)


def read_checked():
    global checked_files
    if checked_files is None:
        try:
            with open(os.path.join(BRIX_HOME, 'checked.json')) as file:
                checked_files = json.load(file)
        except (OSError, ValueError):
            checked_files = {}
    return checked_files


def write_checked(checked):
    os.makedirs(BRIX_HOME, exist_ok=True)
    path = os.path.join(BRIX_HOME, 'checked.json')
    temporary = '%s.%d' % (path, os.getpid())
    with open(temporary, 'w') as file:
        json.dump(checked, file)
    os.replace(temporary, path)


def write_json(path, value):
    path = os.path.realpath(path)
    temporary = '%s.%d' % (path, os.getpid())
    with open(temporary, 'w') as file:
        json.dump(value, file, indent=4)
        file.write('\n')
    os.replace(temporary, path)


def file_digest(path):
    with open(path, 'rb') as file:
        return stream_digest(file)


def stream_digest(stream):
    digest = hashlib.sha256()
    for chunk in iter(lambda: stream.read(CHUNK_SIZE), b''):
        digest.update(chunk)
    return digest.hexdigest()


def release_key(version, name):
    return '%s/%s/%s' % (RELEASES_PREFIX, version, name)


def fetch_release(version, expected):
    archives = archives_path()
    destination = os.path.join(archives, version)
    with install_lock():
        if os.path.isdir(destination):
            return
        sys.stderr.write('brix: downloading release %s\n' % version)
        staged = staging_directory(archives, version)
        release_path = os.path.join(staged, 'release.json')
        digest, size = fetch_object(release_key(version, 'release.json'), release_path)
        if digest != expected:
            raise LaunchError('The release.json of %s has sha256 %s, not the pinned %s' % (version, digest, expected))
        with open(release_path) as file:
            release = json.load(file)
        for package in release_host(release)['packages']:
            name = package_file(package)
            found = fetch_object(release_key(version, name), os.path.join(staged, name))
            if found != (package['sha256'], package['size']):
                raise LaunchError('%s does not match its release.json' % name)
        publish(staged, destination)


def s3_download(key, path=None):
    digest = hashlib.sha256()
    size = 0
    output = open(path, 'wb') if path else None
    try:
        with s3_request('GET', key) as response:
            for chunk in iter(lambda: response.read(CHUNK_SIZE), b''):
                digest.update(chunk)
                size += len(chunk)
                if output:
                    output.write(chunk)
    finally:
        if output:
            output.close()
    return digest.hexdigest(), size


def s3_read(key):
    with s3_request('GET', key) as response:
        return response.read(), response.headers.get('ETag')


def s3_head(key, headers=None):
    try:
        with s3_request('HEAD', key, headers=headers) as response:
            return {name.lower(): value for name, value in response.headers.items()}
    except S3Error as error:
        if error.status == 404:
            return None
        raise


def s3_put(key, body, content_type, headers=None):
    headers = dict(headers or {})
    headers['content-type'] = content_type
    headers['x-amz-checksum-sha256'] = base64.b64encode(hashlib.sha256(body).digest()).decode()
    with s3_request('PUT', key, body, headers):
        pass


def s3_request(method, key, body=b'', headers=None):
    host = '%s.s3.%s.amazonaws.com' % (RELEASES_BUCKET, RELEASES_REGION)
    path = '/' + urllib.parse.quote(key, safe='/-_.~')
    headers = dict(headers or {})
    headers['host'] = host
    credentials = aws_credentials()
    if credentials:
        sign(method, path, headers, hashlib.sha256(body).hexdigest(), credentials)
    request = urllib.request.Request(
        'https://%s%s' % (host, path),
        data=body if method == 'PUT' else None,
        headers=headers,
        method=method,
    )
    try:
        return urllib.request.urlopen(request, timeout=HTTP_TIMEOUT)
    except urllib.error.HTTPError as error:
        raise S3Error(s3_error_message(method, key, error, credentials), error.code)
    except urllib.error.URLError as error:
        raise S3Error('Cannot reach S3 for %s: %s' % (key, error.reason))


def s3_error_message(method, key, error, credentials):
    if error.code == 403 and not credentials:
        return ('%s needs AWS credentials: add a "%s" profile to ~/.aws/config and run %s'
                % (key, AWS_PROFILE, SIGN_IN))
    body = error.read().decode('utf-8', 'replace') if method != 'HEAD' else ''
    code = body.split('<Code>', 1)[1].split('</Code>', 1)[0] if '<Code>' in body else error.reason
    return '%s s3://%s/%s failed: %d %s' % (method, RELEASES_BUCKET, key, error.code, code)


def sign(method, path, headers, payload, credentials):
    stamp = time.strftime('%Y%m%dT%H%M%SZ', time.gmtime())
    day = stamp[:8]
    headers['x-amz-date'] = stamp
    headers['x-amz-content-sha256'] = payload
    if credentials.get('token'):
        headers['x-amz-security-token'] = credentials['token']
    values = {name.lower(): ' '.join(str(value).split()) for name, value in headers.items()}
    names = sorted(values)
    signed = ';'.join(names)
    canonical = '\n'.join([method, path, '', ''.join('%s:%s\n' % (name, values[name]) for name in names), signed, payload])
    scope = '%s/%s/s3/aws4_request' % (day, RELEASES_REGION)
    text = '\n'.join(['AWS4-HMAC-SHA256', stamp, scope, hashlib.sha256(canonical.encode()).hexdigest()])
    key = ('AWS4' + credentials['secret']).encode()
    for part in (day, RELEASES_REGION, 's3', 'aws4_request'):
        key = hmac.new(key, part.encode(), hashlib.sha256).digest()
    signature = hmac.new(key, text.encode(), hashlib.sha256).hexdigest()
    headers['authorization'] = 'AWS4-HMAC-SHA256 Credential=%s/%s, SignedHeaders=%s, Signature=%s' % (
        credentials['key'], scope, signed, signature)


def aws_credentials():
    global aws_session
    if aws_session is None:
        aws_session = profile_credentials() or {}
    return aws_session or None


def profile_credentials():
    config = read_ini(os.environ.get('AWS_CONFIG_FILE') or '~/.aws/config')
    shared = read_ini(os.environ.get('AWS_SHARED_CREDENTIALS_FILE') or '~/.aws/credentials')
    profile = {}
    if config.has_section('profile ' + AWS_PROFILE):
        profile.update(config.items('profile ' + AWS_PROFILE))
    if shared.has_section(AWS_PROFILE):
        profile.update(shared.items(AWS_PROFILE))
    if not profile:
        return None
    if profile.get('aws_access_key_id') and profile.get('aws_secret_access_key'):
        return {
            'key': profile['aws_access_key_id'],
            'secret': profile['aws_secret_access_key'],
            'token': profile.get('aws_session_token'),
        }
    if profile.get('sso_session'):
        return sso_credentials(profile, config)
    raise LaunchError('The "%s" AWS profile has neither access keys nor an sso_session' % AWS_PROFILE)


def read_ini(path):
    parser = configparser.RawConfigParser()
    parser.read(os.path.expanduser(path))
    return parser


def sso_credentials(profile, config):
    section = 'sso-session ' + profile['sso_session']
    if not config.has_section(section) or not config.has_option(section, 'sso_region'):
        raise LaunchError('The AWS config has no [%s] with an sso_region' % section)
    region = config.get(section, 'sso_region')
    token = sso_token(profile['sso_session'], region)
    query = urllib.parse.urlencode({'account_id': profile['sso_account_id'], 'role_name': profile['sso_role_name']})
    request = urllib.request.Request(
        'https://portal.sso.%s.amazonaws.com/federation/credentials?%s' % (region, query),
        headers={'x-amz-sso_bearer_token': token},
    )
    try:
        with urllib.request.urlopen(request, timeout=HTTP_TIMEOUT) as response:
            role = json.load(response)['roleCredentials']
    except urllib.error.HTTPError as error:
        if error.code in (401, 403):
            raise LaunchError('The AWS sign-in has expired; run %s' % SIGN_IN)
        raise LaunchError('Getting credentials for the %s role failed: %d' % (profile['sso_role_name'], error.code))
    except urllib.error.URLError as error:
        raise LaunchError('Cannot reach the AWS sign-in service: %s' % error.reason)
    return {'key': role['accessKeyId'], 'secret': role['secretAccessKey'], 'token': role['sessionToken']}


def sso_token(session, region):
    path = os.path.join(os.path.expanduser('~/.aws/sso/cache'), hashlib.sha1(session.encode()).hexdigest() + '.json')
    try:
        with open(path) as file:
            token = json.load(file)
    except (OSError, ValueError):
        raise LaunchError('Not signed in to AWS; run %s' % SIGN_IN)
    now = time.time()
    if parse_time(token.get('expiresAt', '')) - now > TOKEN_REFRESH_WINDOW:
        return token['accessToken']
    if not all(token.get(name) for name in ('refreshToken', 'clientId', 'clientSecret')) or \
            parse_time(token.get('registrationExpiresAt', '')) <= now:
        raise LaunchError('The AWS sign-in has expired; run %s' % SIGN_IN)
    body = json.dumps({
        'clientId': token['clientId'],
        'clientSecret': token['clientSecret'],
        'grantType': 'refresh_token',
        'refreshToken': token['refreshToken'],
    }).encode()
    request = urllib.request.Request(
        'https://oidc.%s.amazonaws.com/token' % region,
        data=body,
        headers={'content-type': 'application/json'},
        method='POST',
    )
    try:
        with urllib.request.urlopen(request, timeout=HTTP_TIMEOUT) as response:
            refreshed = json.load(response)
    except urllib.error.HTTPError:
        raise LaunchError('The AWS sign-in has expired; run %s' % SIGN_IN)
    except urllib.error.URLError as error:
        raise LaunchError('Cannot reach the AWS sign-in service: %s' % error.reason)
    token['accessToken'] = refreshed['accessToken']
    token['expiresAt'] = time.strftime(TIME_FORMAT, time.gmtime(now + refreshed['expiresIn']))
    if refreshed.get('refreshToken'):
        token['refreshToken'] = refreshed['refreshToken']
    temporary = '%s.%d' % (path, os.getpid())
    with open(os.open(temporary, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600), 'w') as file:
        json.dump(token, file)
    os.replace(temporary, path)
    return token['accessToken']


def parse_time(text):
    text = text.replace('UTC', 'Z').replace('+00:00', 'Z')
    if '.' in text:
        text = text.split('.', 1)[0] + 'Z'
    try:
        return calendar.timegm(time.strptime(text, TIME_FORMAT))
    except ValueError:
        return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
