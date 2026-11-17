import json
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.realpath(__file__))))

from common import Failure, run_main

LINE = re.compile(r'^(\s+)([+!:|\s]*)(\d+)\s+(.*?)(?:\s+\(in (.+?)\))?(?:\s+\+\s+[\d,]+)?\s*(\[[^\]]*\])?\s*$')
BLOCKED_LEAVES = {
    '__psynch_cvwait',
    '__psynch_mutexwait',
    '__psynch_mutexdrop',
    '__select',
    'kevent',
    '__semwait_signal',
    'mach_msg2_trap',
    'mach_msg_trap',
    '__ulock_wait',
    'swtch_pri',
    '__wait4',
    'read',
    '__read',
    'write',
    '__write',
    'nanosleep',
    '__sysctl',
}
BLOCKED_SYMBOL = re.compile(r'psynch|ulock|kevent|semwait|mach_msg')
SYSTEM_BINARY = re.compile(r'^(libsystem|libc\+\+|libobjc|dyld|libdispatch)')
CPU_CAUSES = (
    ('gc-write-barrier', re.compile(r'brixGC(Bulk)?WriteBarrier|brixWriteBarrier')),
    ('gc-roots', re.compile(r'brixGC(Add|Remove)Root|GCStackMap|StorageRoot')),
    ('alloc', re.compile(r'brixAlloc|brixTryFastGCAllocation|brixGCResizeBuffer|allocObject|allocDescribed|'
                         r'finishAllocation|AllocBatch|BatchAllocation|BatchReservation|'
                         r'brixRecordGCPublicAllocation|allocSpan')),
    ('gc-pacing-sweep', re.compile(r'sweepDebt|SweepDebt|needsSweepNow|sweepIncremental|GCProgress|AllocationAssist|'
                                   r'rescueSweep|publicCollectorPhase|Safepoint|safepoint|Handshake|GCSpan::sweep|'
                                   r'drainLimit|traceDescriptor')),
    ('gc-ffi-context', re.compile(r'brixGC(Enter|Leave)FFI|PublishBlockingContext|brixContextGet|brixGCRegisterTask|'
                                  r'brixGCUnregisterTask')),
    ('panic-state', re.compile(r'brixPanic')),
    ('string-runtime', re.compile(r'brixString|BxString')),
    ('number-runtime', re.compile(r'brixNumber|BxNumber|ToString')),
    ('scheduler', re.compile(r'BxTaskQueue|BxTask::|brixSchedule|BxThread|BxSemaphore|BxKQueue|BxSelectLoop|'
                             r'brixRunEventLoop|BxEvents')),
)
FRAME_COMMIT = re.compile(r'CA::Transaction::commit|CA::Render::(prepare_image|copy_image)|brixGuiWindowCommitFrame|'
                          r'setCanvasImage')
IMAGE_DATA_LOCK = re.compile(r'CGSImageDataLock')
ICC_CONVERSION = re.compile(r'convert_icc|convert_using_vImageConverter|CGColorTransformConvertUsingCMSConverter')
VIMAGE_CONVERSION = re.compile(r'vImageConverter|vImageConvert_AnyToAny|AnyToAnyBlockInternal')
SYMBOL_LABEL = re.compile(r'[^A-Za-z0-9_]')


def main(arguments):
    if not 1 <= len(arguments) <= 2:
        raise Failure('usage: sample.py <sample.txt> [metrics.json]')
    metrics = {}
    if len(arguments) == 2:
        with open(arguments[1]) as file:
            metrics = json.load(file)
    with open(arguments[0], errors='replace') as file:
        attribution = analyze(file.read(), metrics)
    if attribution is None:
        raise Failure('The sample call graph has no attributable samples')
    print(json.dumps(attribution))


def analyze(text, metrics=None):
    symbols = brix_symbols(metrics or {})
    threads = sample_threads(text)
    causes = {}
    blocked_leaves = {}
    blocking_stacks = {}
    cpu_buckets = {}
    color = {
        'frameCommitSamples': 0,
        'commitSamples': 0,
        'imageDataLockSamples': 0,
        'iccSamples': 0,
        'vImageSamples': 0,
    }
    running_samples = 0
    blocked_samples = 0
    for thread in threads:
        for leaf in thread['leaves']:
            count = leaf['self']
            if leaf['symbol'] in BLOCKED_LEAVES or BLOCKED_SYMBOL.search(leaf['symbol']):
                thread['blocked'] += count
                blocked_samples += count
                cause = blocking_cause(leaf['ancestors'])
                causes[cause] = causes.get(cause, 0) + count
                blocked_leaves[leaf['symbol']] = blocked_leaves.get(leaf['symbol'], 0) + count
                key = (leaf['symbol'], cause)
                blocking_stacks[key] = blocking_stacks.get(key, 0) + count
                continue
            thread['running'] += count
            running_samples += count
            frames = [leaf] + leaf['ancestors'][::-1]
            if any(FRAME_COMMIT.search(frame['symbol']) for frame in frames):
                color['frameCommitSamples'] += count
                image_data_lock = any(IMAGE_DATA_LOCK.search(frame['symbol']) for frame in frames)
                icc_conversion = any(ICC_CONVERSION.search(frame['symbol']) for frame in frames)
                vimage_conversion = any(VIMAGE_CONVERSION.search(frame['symbol']) for frame in frames)
                if image_data_lock:
                    color['imageDataLockSamples'] += count
                if icc_conversion:
                    color['iccSamples'] += count
                if vimage_conversion:
                    color['vImageSamples'] += count
                if image_data_lock or icc_conversion or vimage_conversion:
                    color['commitSamples'] += count
            bucket = cpu_bucket(frames, symbols)
            cpu_buckets[bucket] = cpu_buckets.get(bucket, 0) + count
    total = running_samples + blocked_samples
    if not total:
        return None
    stacks = [{'leaf': leaf, 'cause': cause, 'count': count, 'sharePct': round(count * 100 / total, 6)}
              for (leaf, cause), count in blocking_stacks.items()]
    return {
        'schemaVersion': 2,
        'totalSamples': total,
        'runningSamples': running_samples,
        'blockedSamples': blocked_samples,
        'runningPct': round(running_samples * 100 / total, 6),
        'blockedPct': round(blocked_samples * 100 / total, 6),
        'threads': [{name: thread[name] for name in ('name', 'total', 'running', 'blocked', 'main')}
                    for thread in threads if thread['running'] + thread['blocked'] > 0],
        'blockingCauses': causes,
        'blockedLeafSymbols': blocked_leaves,
        'blockingStacks': sorted(stacks, key=lambda stack: -stack['count']),
        'cpuBuckets': cpu_buckets,
        'colorConversion': color,
    }


def brix_symbols(metrics):
    records = (metrics.get('mir') or {}).get('topFunctions')
    if records is None:
        records = metrics.get('mirTopFunctions') or []
    symbols = {}
    for record in records:
        if isinstance(record, dict) and record.get('symbol'):
            label = SYMBOL_LABEL.sub('_', record['symbol'])
            symbols.setdefault(label, {'name': record.get('name'), 'loc': record.get('loc') or ''})
    return symbols


def sample_threads(text):
    start = text.find('Call graph:')
    if start < 0:
        raise Failure('The sample output has no call graph')
    end = text.find('\nTotal number in stack')
    lines = text[start:end if end >= 0 else len(text)].split('\n')[1:]
    threads = []
    current = None
    chain = []

    def finish_node():
        node = chain.pop()
        if current is None or node['symbol'].startswith('Thread_'):
            return
        count = node['count'] - node['children']
        if count > 0:
            current['leaves'].append({
                'symbol': node['symbol'],
                'binary': node['binary'],
                'self': count,
                'ancestors': node['ancestors'],
            })

    for raw in lines:
        if not raw.strip():
            continue
        match = LINE.match(raw)
        if not match:
            continue
        depth = len(match.group(1)) + len(match.group(2))
        count = int(match.group(3))
        symbol = match.group(4).strip()
        binary = match.group(5) or ''
        if symbol.startswith('Thread_'):
            while chain:
                finish_node()
            current = {
                'name': symbol.split()[0],
                'total': count,
                'running': 0,
                'blocked': 0,
                'main': 'main-thread' in raw,
                'leaves': [],
            }
            threads.append(current)
            continue
        if current is None:
            continue
        while chain and chain[-1]['depth'] >= depth:
            finish_node()
        if chain:
            chain[-1]['children'] += count
        chain.append({
            'depth': depth,
            'count': count,
            'children': 0,
            'symbol': symbol,
            'binary': binary,
            'ancestors': [{'symbol': entry['symbol'], 'binary': entry['binary']} for entry in chain],
        })
    while chain:
        finish_node()
    return threads


def blocking_cause(ancestors):
    for ancestor in reversed(ancestors):
        if not SYSTEM_BINARY.search(ancestor['binary']) and not ancestor['symbol'].startswith('Thread_'):
            return ancestor['symbol']
    return '(unknown)'


def cpu_bucket(frames, symbols):
    for frame in frames:
        for bucket, pattern in CPU_CAUSES:
            if pattern.search(frame['symbol']):
                return bucket
        found = symbols.get(frame['symbol']) or symbols.get(re.sub(r'^_', '', frame['symbol']))
        if found:
            location = found['loc']
            if '/stdlib/' in location:
                return 'brix:' + re.sub(r'\.yumi.*', '', location.split('/stdlib/')[1])
            if '/bench/' in location:
                return 'brix:bench-app'
            return 'brix:other'
    return 'other-cpp-or-system'


if __name__ == '__main__':
    sys.exit(run_main(main))
