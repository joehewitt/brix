#!/usr/bin/env python3
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.realpath(__file__))))

from common import ROOT, Failure, run_main, say, shown

NODES = os.path.join(ROOT, 'stdlib', 'lang', 'ast', 'ast-nodes.bx')
CHILDREN = os.path.join(ROOT, 'stdlib', 'lang', 'ast', 'ast-children.bx')
UNION = '(type + Syntax (is Union)'
BASE = 'SyntaxNode'
FUNCTION = '(fn-do + gather-children _'
CASE = re.compile(r'^  \(variant (node-[a-z0-9-]+) (AST[A-Za-z0-9]+)\)$')
STRUCT = re.compile(r'^\(type \+ (AST[A-Za-z0-9]+) \(is (SyntaxNode|AST[A-Za-z0-9]+)\)')
FIELD = re.compile(r'^  \(prop \+ ([a-z][a-z0-9-]*) (SyntaxId|\(of RowRange SyntaxId\))[ )]')
CHILD = 'SyntaxId'
HEADER = [
    '(import ./ast)',
    '(import ./ast-nodes)',
    '(import ./operator)',
    '(import ./row-range)',
    '(import /core/arena)',
    '(import /core/array-builder)',
    '(import /core/table)',
    '',
    FUNCTION,
    '  (param nodes (of ImmutableTable Syntax))',
    '  (param children (of ImmutableArray SyntaxId ArenaBacking))',
    '  (param id SyntaxId)',
    '  (param links ChildLinks)',
    '  (match? ([] nodes id)',
]
FOOTER = ['  )', ')']


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
    lines = current_text.split('\n')
    if FUNCTION not in lines:
        raise Failure('%s must declare gather-children first and keep its hand-written declarations after it' % shown(CHILDREN))
    end = lines.index(')', lines.index(FUNCTION))
    return '\n'.join(HEADER + children_arms(nodes_text) + FOOTER + lines[end + 1:])


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
            fields = section(lines, index + 1) if not line.endswith('))') else []
            structs[head.group(1)] = [match.groups() for match in map(FIELD.match, fields) if match]
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
        children = [(field, type) for name in chain for field, type in structs.get(name, [])]
        if not children:
            continue
        arms.append('    (case (call .%s row)' % kind)
        for field, type in children:
            if type == CHILD:
                arms.append('      (call (. links one) (. row %s))' % field)
            else:
                arms.append('      (call (. links all) ([] children (. row %s)))' % field)
        arms.append('    )')
    return arms


def section(lines, start):
    for line in lines[start:]:
        if line == ')':
            return
        yield line


if __name__ == '__main__':
    sys.exit(run_main(main))
