import json
import os
import re
import shutil

import runtime
from checks import gc_allocation_abi
from common import Failure, lock, say

STAMP = 'brix-bench-runtime-v8'
STAMP_NAME = '.brix-bench-runtime'
LOCK_NAME = '.bench-runtime.lock'
KEPT_NAME = 'results'
GC_RUNTIME_SELECTOR = 'bench-gc-runtime'
GC_MODES = ('normal', 'no-gc', 'never-collect')


def main(arguments):
    if len(arguments) != 1:
        say('usage: brix-bench-runtime <benchmark-build-root>')
        return 2
    root = os.path.abspath(arguments[0])
    os.makedirs(root, exist_ok=True)
    with open(runtime.CONFIG) as file:
        config = json.load(file)
    with open(os.path.join(root, LOCK_NAME), 'w') as file:
        lock(file)
        gc_allocation_abi.check(config)
        for kind, mode, build_root in variants(root):
            build_variant(config, kind, mode, build_root)


def variants(root):
    gc_runtime = GC_RUNTIME_SELECTOR in re.split(r'[\s,]+', os.environ.get('PROBES_ENABLED', ''))
    kinds = (('instrumented', root), ('clean', os.path.join(root, 'clean'))) if gc_runtime else (('instrumented', root),)
    for kind, base in kinds:
        for mode in GC_MODES if gc_runtime else ('normal',):
            yield kind, mode, os.path.join(base, 'bench') if mode == 'normal' else os.path.join(base, mode, 'bench')


def build_variant(config, kind, mode, build_root):
    stamp = '%s:%s:%s' % (STAMP, kind, mode)
    stamp_path = os.path.join(build_root, STAMP_NAME)
    if read_stamp(stamp_path) != stamp:
        reset(build_root)
    os.makedirs(build_root, exist_ok=True)
    environment = {
        'BRIX_BENCH_RUNTIME': '1',
        'BRIX_BENCH_METRICS': '0' if kind == 'clean' else '1',
        'BRIX_GC_MODE': mode,
    }
    build = runtime.Build(config, runtime.parse_arguments(['build', '--build', build_root]), environment)
    try:
        build.build()
    except Failure as failure:
        raise Failure('%s for the %s %s benchmark runtime' % (failure, kind, mode))
    with open(stamp_path, 'w') as file:
        file.write(stamp + '\n')


def read_stamp(path):
    try:
        with open(path) as file:
            return file.read().strip()
    except FileNotFoundError:
        return None


def reset(build_root):
    if not os.path.isdir(build_root):
        return
    for name in os.listdir(build_root):
        if name == KEPT_NAME:
            continue
        path = os.path.join(build_root, name)
        if os.path.isdir(path) and not os.path.islink(path):
            shutil.rmtree(path)
        else:
            os.remove(path)
