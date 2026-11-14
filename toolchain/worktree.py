#!/usr/bin/env python3
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.realpath(__file__)))

import compiler
import runtime
from common import BUILD, Failure, run_main, say

FRESH_STATUSES = ('fresh', 'touched')


def main(arguments):
    if arguments:
        raise Failure('usage: worktree.py')
    runtime.main(['build', '--build', BUILD])
    build_compiler()


def build_compiler():
    seed = compiler.seed_name()
    status = compiler.record_status(compiler.read_record(), seed)
    started = time.time()
    compiler.ensure_compiler(seed)
    if status not in FRESH_STATUSES:
        say('compiler  built  %.1fs' % (time.time() - started))


if __name__ == '__main__':
    sys.exit(run_main(main))
