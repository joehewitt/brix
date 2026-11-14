import json
import os
import re
import subprocess
import tempfile

from common import ROOT, Failure

PROBE = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'gc-allocation-abi-probe.cpp')
COMPILER_SOURCES = {
    'stdlib/lang/emit/runtime.yumi': {
        'gc-allocation-buffer-version': 'version',
        'gc-control-phase-mask': 'controlPhaseMask',
    },
    'stdlib/lang/emit/layouts.yumi': {
        'compiler-buffer-count': 'compilerBufferCount',
        'site-key-size': 'siteKeySize',
        'thread-cache-compiler-buffers-offset': 'threadCacheCompilerBuffersOffset',
        'runtime-buffer-size': 'runtimeSize',
    },
    'stdlib/lang/backend/gc-write-barrier-lowering.yumi': {
        'cursor-offset': 'cursorOffset',
        'limit-offset': 'limitOffset',
        'slot-size-offset': 'runtimeSlotSizeOffset',
        'generation-offset': 'runtimeGenerationOffset',
        'handshake-requested-bit': 'controlHandshakeRequestedBit',
        'sweep-generation-shift': 'controlSweepGenerationShift',
    },
}
CONSTANT_PATTERN = re.compile(r'^[_+] ([a-z][a-z0-9-]*) [A-Z]\w*(?: = (0x[0-9a-fA-F]+|\d+))?$', re.M)


def check(config):
    runtime = runtime_fields(config)
    problems = []
    for source, names in COMPILER_SOURCES.items():
        with open(os.path.join(ROOT, source)) as file:
            constants = {name: int(value, 0) if value else 0 for name, value in CONSTANT_PATTERN.findall(file.read())}
        for name, field in names.items():
            if name not in constants:
                problems.append('  %s: missing from %s' % (name, source))
            elif constants[name] != runtime[field]:
                problems.append('  %s in %s is %d; the runtime\'s %s is %d'
                                % (name, source, constants[name], field, runtime[field]))
    if problems:
        raise Failure('\n'.join(['The compiler\'s GC allocation ABI does not match libs/gc:'] + problems))


def runtime_fields(config):
    with tempfile.TemporaryDirectory(prefix='yumi-gc-abi-') as scratch:
        executable = os.path.join(scratch, 'probe')
        compiled = subprocess.run(
            [
                config['toolchain']['cxxCompiler'],
                '-std=' + config['compile']['runtimeCxxStandard'],
                '-I' + os.path.join(ROOT, 'libs', 'gc', 'include'),
                PROBE,
                '-o',
                executable,
            ],
            cwd=ROOT,
            capture_output=True,
            text=True,
        )
        if compiled.returncode != 0:
            raise Failure('%sCompiling the GC allocation ABI probe failed' % compiled.stderr)
        return json.loads(subprocess.check_output([executable], cwd=ROOT, text=True))
