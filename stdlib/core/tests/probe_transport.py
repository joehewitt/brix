import json
import os
import unittest

from common import ROOT
from testing import support

BRIX = os.path.join(ROOT, 'bin', 'brix')
SEGMENTS = {'PROBES_ENABLED': 'log', 'PROBES_SEGMENT_SIZE': '16384', 'PROBES_SEGMENT_POOL': '1'}
TAIL_PRODUCERS = 64
ROLLOVER_PRODUCERS = 32
ROLLOVER_RECORDS = 120
RECORD_LIMIT = 1000000

TAIL_SOURCE = '''
for producer-id in 0 .. < %d
    do new thread
        // worker tail $producer-id

-- "done"
''' % TAIL_PRODUCERS

ROLLOVER_SOURCE = '''
for producer-id in 0 .. < %d
    do new thread
        for i in 0 .. < %d
            // producer $producer-id item $i

-- "done"
''' % (ROLLOVER_PRODUCERS, ROLLOVER_RECORDS)


def recorded_fields(case, source, settings):
    directory = support.scratch(case)
    trace = os.path.join(directory, 'probes.trace')
    environment = support.environment(PROBES_ENABLED='1', PROBES_PATH=trace)
    environment.update(SEGMENTS, **settings)
    program = os.path.join(directory, 'program')
    support.run([BRIX, '-c', source, '--compile', '--probes', '-o', program], support.COMPILE_SECONDS)
    output = support.run([program], support.COMPILE_SECONDS, environment)
    case.assertEqual(output.strip(), 'done')
    log = support.run([BRIX, 'probes', 'log', trace, '--format', 'jsonl', '--limit', str(RECORD_LIMIT)], 300)
    return [json.loads(line)['fields'] for line in log.splitlines()]


class ProbeTransport(unittest.TestCase):
    def test_parallel_tasks_flush_their_tail_records(self):
        fields = recorded_fields(self, TAIL_SOURCE, {
            'PROBES_TASK_BUFFER_SIZE': '65536',
            'PROBES_FLUSH_THRESHOLD': '65536',
            'PROBES_FLUSH_INTERVAL_MS': '10000',
        })
        self.assertEqual(sorted(field['value1'] for field in fields), list(range(TAIL_PRODUCERS)))

    def test_parallel_tasks_roll_over_segments_without_losing_records(self):
        fields = recorded_fields(self, ROLLOVER_SOURCE, {'PROBES_TASK_BUFFER_SIZE': '256'})
        expected = [(producer, item) for producer in range(ROLLOVER_PRODUCERS) for item in range(ROLLOVER_RECORDS)]
        self.assertEqual(sorted((field['value1'], field['value2']) for field in fields), expected)
