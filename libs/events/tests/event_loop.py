import json
import os
import unittest

from common import ROOT, RUNTIME_CONFIG
from testing import support

EVENTS = os.path.join(ROOT, 'libs', 'events')
FIXTURES = os.path.join(EVENTS, 'tests', 'fixtures')
STUBS = ('YuTask.h', 'YuTaskQueue.h')


def cxx_compiler():
    with open(RUNTIME_CONFIG) as file:
        return json.load(file)['toolchain']['cxxCompiler']


def include_root(directory):
    root = os.path.join(directory, 'include')
    headers = os.path.join(root, 'YuEvents')
    os.makedirs(headers)
    for name in os.listdir(os.path.join(EVENTS, 'include')):
        source = os.path.join(FIXTURES if name in STUBS else os.path.join(EVENTS, 'include'), name)
        os.symlink(source, os.path.join(headers, name))
    os.symlink(os.path.join(ROOT, 'libs', 'gc', 'include'), os.path.join(root, 'YuGC'))
    return root


class EventLoop(unittest.TestCase):
    def test_wakeups_stay_nonblocking_through_boundaries_races_saturation_and_shutdown(self):
        directory = support.scratch(self)
        executable = os.path.join(directory, 'wakeup')
        support.run([
            cxx_compiler(), '-std=c++17', '-O1', '-pthread', '-I', include_root(directory),
            os.path.join(FIXTURES, 'wakeup.cpp'),
            os.path.join(FIXTURES, 'trigger.cpp'),
            os.path.join(EVENTS, 'YuPoll.cpp'),
            os.path.join(EVENTS, 'YuSelectLoop.cpp'),
            os.path.join(EVENTS, 'YuKQueueLoop.cpp'),
            '-o', executable,
        ], 120)
        support.run([executable], 20)
