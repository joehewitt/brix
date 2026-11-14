import os
import shutil
import subprocess
import tempfile

from common import BUILD, ROOT, shown

YUMI = os.path.join(ROOT, 'bin', 'yumi')
COMPILE_SECONDS = 120


def environment(**overrides):
    values = dict(os.environ)
    values['YUMIPATH'] = os.path.join(ROOT, 'stdlib')
    values['YUMI_BUILD_PATH'] = BUILD
    values.update(overrides)
    return values


def scratch(case):
    directory = tempfile.mkdtemp(prefix='yumi-test-')
    case.addCleanup(shutil.rmtree, directory, True)
    return directory


def run(command, seconds, env=None, cwd=ROOT):
    try:
        result = subprocess.run(command, cwd=cwd, env=env or environment(), stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE, text=True, timeout=seconds)
    except subprocess.TimeoutExpired:
        raise AssertionError('%s did not finish within %d seconds' % (shown(command[0]), seconds))
    if result.returncode != 0:
        ending = ('was killed by signal %d' % -result.returncode if result.returncode < 0
                  else 'exited with status %d' % result.returncode)
        output = '\n'.join(text.strip() for text in (result.stdout, result.stderr) if text.strip())
        raise AssertionError('%s %s%s' % (shown(command[0]), ending, '\n' + output if output else ''))
    return result.stdout


def compile_yumi(source, executable, env=None):
    run([YUMI, source, '--compile', '-o', executable], COMPILE_SECONDS, env)
