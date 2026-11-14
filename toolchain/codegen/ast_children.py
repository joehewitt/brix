#!/usr/bin/env python3
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.realpath(__file__))))

from common import ROOT, Failure, run_main, say, shown

NODES = os.path.join(ROOT, 'stdlib', 'lang', 'ast', 'ast-nodes.yumi')
CHILDREN = os.path.join(ROOT, 'stdlib', 'lang', 'ast', 'ast-children.yumi')
THICK_DIVIDER = '━' * 20
DIVIDER_LINE = '\n%s\n' % THICK_DIVIDER
SECTION_END = '━'
UNION = '+ Syntax is Union'
BASE = 'SyntaxNode'
CASE = re.compile(r'^\| (node-[a-z0-9-]+) (AST[A-Za-z0-9]+)$')
STRUCT = re.compile(r'^\+ (AST[A-Za-z0-9]+) is (SyntaxNode|AST[A-Za-z0-9]+)$')
FIELD = re.compile(r"^\+ (?:@hot )?([a-z][a-z0-9-]*) ([A-Za-z']+)(?: = .*)?$")
CHILD = 'SyntaxId'
CHILD_RANGE = "RowRange'SyntaxId"
HEADER = [
    './ast',
    './ast-nodes',
    './operator',
    './row-range',
    '/core/arena',
    '/core/array-builder',
    '/core/table',
    '',
    THICK_DIVIDER,
    '+ do gather-children',
    "    - nodes ImmutableTable'Syntax",
    "    - children ImmutableArray'SyntaxId'ArenaBacking",
    '    - id SyntaxId',
    '    - links ChildLinks',
    '',
    'do match? nodes[id]',
]


def main(arguments):
    if arguments not in ([], ['--check']):
        raise Failure('usage: ast_children.py [--check]')
    current = read(CHILDREN)
    generated = children_file(read(NODES), current)
    if generated == current:
        return
    if arguments:
        raise Failure('%s is stale; run toolchain/codegen/ast_children.py' % shown(CHILDREN))
    with open(CHILDREN, 'w', encoding='utf-8') as file:
        file.write(generated)
    say('Generated %s' % shown(CHILDREN))


def read(path):
    with open(path, encoding='utf-8') as file:
        return file.read()


def children_file(nodes_text, current_text):
    sections = current_text.split(DIVIDER_LINE)
    if len(sections) < 3:
        raise Failure('%s must keep its hand-written declarations after the second thick divider' % shown(CHILDREN))
    return '\n'.join(HEADER + children_arms(nodes_text) + ['', THICK_DIVIDER, DIVIDER_LINE.join(sections[2:])])


def children_arms(nodes_text):
    lines = nodes_text.split('\n')
    if UNION not in lines:
        raise Failure('The Syntax union is missing from %s' % shown(NODES))
    cases = [match.groups() for match in map(CASE.match, section(lines, lines.index(UNION) + 1)) if match]
    structs = {}
    bases = {}
    for index, line in enumerate(lines):
        head = STRUCT.match(line)
        if head:
            structs[head.group(1)] = [match.groups() for match in map(FIELD.match, section(lines, index + 1)) if match]
            bases[head.group(1)] = head.group(2)
    arms = []
    for kind, struct in cases:
        if struct not in structs:
            raise Failure('%s names %s, which is not declared as a %s' % (kind, struct, BASE))
        chain = []
        name = struct
        while name and name != BASE:
            chain.insert(0, name)
            name = bases.get(name)
        children = [(field, type) for name in chain for field, type in structs.get(name, [])
                    if type in (CHILD, CHILD_RANGE)]
        if not children:
            continue
        arms.append('    | .%s(row)' % kind)
        for field, type in children:
            if type == CHILD:
                arms.append('        links.one(row.%s)' % field)
            else:
                arms.append('        links.all(children[row.%s])' % field)
    return arms


def section(lines, start):
    for line in lines[start:]:
        if line.startswith(SECTION_END):
            return
        yield line


if __name__ == '__main__':
    sys.exit(run_main(main))
