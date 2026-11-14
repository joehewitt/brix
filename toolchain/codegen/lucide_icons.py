#!/usr/bin/env python3
import decimal
import json
import math
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.realpath(__file__))))

import deps
from common import ROOT, Failure, run_main, say, shown

PACKAGE = 'lucide-static'
OUTPUT = os.path.join(ROOT, 'stdlib', 'gui', 'icons', 'lucide')
TAGS_OUTPUT = os.path.join(ROOT, 'stdlib', 'gui', 'icons', 'lucide-tags.yumi')
GENERATOR = 'toolchain/codegen/lucide_icons.py'
MANIFEST = 'manifest.json'
LICENSE = 'LICENSE.txt'
PROBLEM_LIMIT = 20
MAJOR_DIVIDER = '━' * 20
MINOR_DIVIDER = '─' * 20
ROOT_ATTRIBUTES = {
    'class', 'fill', 'height', 'stroke', 'stroke-linecap', 'stroke-linejoin', 'stroke-width', 'viewBox', 'width',
    'xmlns',
}
ROOT_VALUES = (('fill', 'none'), ('stroke', 'currentColor'), ('stroke-linecap', 'round'), ('stroke-linejoin', 'round'))
STYLE_ATTRIBUTES = {'fill', 'fill-rule', 'stroke', 'stroke-linecap', 'stroke-linejoin', 'stroke-width'}
GEOMETRY_ATTRIBUTES = {
    'circle': {'cx', 'cy', 'r'},
    'ellipse': {'cx', 'cy', 'rx', 'ry'},
    'line': {'x1', 'x2', 'y1', 'y2'},
    'path': {'d'},
    'polygon': {'points'},
    'polyline': {'points'},
    'rect': {'height', 'rx', 'ry', 'width', 'x', 'y'},
}
REQUIRED_GEOMETRY = {
    'circle': ('cx', 'cy', 'r'),
    'ellipse': ('cx', 'cy', 'rx', 'ry'),
    'line': ('x1', 'x2', 'y1', 'y2'),
    'path': ('d',),
    'polygon': ('points',),
    'polyline': ('points',),
    'rect': ('height', 'width'),
}
LINE_CAPS = ('butt', 'round', 'square')
LINE_JOINS = ('bevel', 'miter', 'round')
FILL_RULES = ('even-odd', 'nonzero')
GLYPH_VIEW_BOX = [0, 0, 24, 24]
GLYPH_STROKE_WIDTH = 2
SHAPE_KINDS = {'path': 0, 'circle': 1, 'ellipse': 2, 'line': 3, 'rectangle': 4}
OPERATION_CODES = {'M': 0, 'L': 1, 'C': 2, 'Q': 3, 'A': 4, 'Z': 5}
PARAMETER_COUNTS = {'A': 7, 'C': 6, 'H': 1, 'L': 2, 'M': 2, 'Q': 4, 'S': 4, 'T': 2, 'V': 1}
COMMANDS = 'AaCcHhLlMmQqSsTtVvZz'
COMMENT = re.compile(r'<!--[\s\S]*?-->')
SVG_ROOT = re.compile(r'<svg\b([^<>]*)>([\s\S]*)</svg>')
ELEMENT = re.compile(r'<([a-z]+)\b([^<>]*)/>')
ATTRIBUTE = re.compile(r'([A-Za-z_:][A-Za-z0-9_.:-]*)\s*=\s*"([^"]*)"')
WHITESPACE = re.compile(r'\s+')
SEPARATORS = re.compile(r'[\s,]+')
NUMBER = re.compile(r'[+-]?(?:(?:[0-9]+\.[0-9]*|\.[0-9]+|[0-9]+)(?:[eE][+-]?[0-9]+)?)')
ICON_NAME = re.compile(r'[a-z0-9]+(?:-[a-z0-9]+)*')
TAG_PUNCTUATION = set(" .'/&+-")
PRECISION = decimal.Decimal('1e-12')
CONTEXT = decimal.Context(prec=64)
SMALLEST_PLAIN = 1e-6


def main(arguments):
    if arguments not in ([], ['--check']):
        raise Failure('usage: lucide_icons.py [--check]')
    prefix = deps.ensure_dependency(PACKAGE)
    files = generated_files(prefix, deps.read_recipe(PACKAGE)['version'])
    tags = render_tags(prefix, files)
    if arguments:
        check(files)
        check_tags(tags)
    else:
        write(files)
        write_tags(tags)


def generated_files(prefix, version):
    icons = os.path.join(prefix, 'icons')
    names = sorted(name for name in os.listdir(icons) if name.endswith('.svg'))
    if not names:
        raise Failure('No Lucide SVG files found in %s' % icons)
    files = {}
    owners = {}
    for source_name in names:
        icon_name = source_name[:-len('.svg')]
        type_name = lucide_type_name(icon_name)
        if type_name in owners:
            raise source_error(source_name, 'generated type %s also belongs to %s' % (type_name, owners[type_name]))
        owners[type_name] = source_name
        with open(os.path.join(icons, source_name), encoding='utf-8-sig') as file:
            icon = parse_svg(source_name, file.read())
        files[icon_name + '.yumi'] = render_icon(icon_name, type_name, icon)
    with open(os.path.join(prefix, 'LICENSE'), encoding='utf-8') as file:
        license = file.read()
    files[LICENSE] = license if license.endswith('\n') else license + '\n'
    files[MANIFEST] = render_manifest(version, files)
    return files


def check(files):
    try:
        present = sorted(os.listdir(OUTPUT))
    except FileNotFoundError:
        raise Failure('The generated Lucide directory %s is missing' % shown(OUTPUT))
    problems = ['unexpected %s' % name for name in present if managed(name) and name not in files]
    for name, content in files.items():
        existing = read_text(os.path.join(OUTPUT, name))
        if existing is None:
            problems.append('missing %s' % name)
        elif existing != content:
            problems.append('stale %s' % name)
    if problems:
        listed = problems[:PROBLEM_LIMIT]
        if len(problems) > PROBLEM_LIMIT:
            listed.append('...and %d more' % (len(problems) - PROBLEM_LIMIT))
        raise Failure('%s\nThe Lucide icons are not current; run %s' % ('\n'.join(listed), GENERATOR))


def write(files):
    os.makedirs(OUTPUT, exist_ok=True)
    listed = generated_names()
    removed = 0
    for name in sorted(os.listdir(OUTPUT)):
        if managed(name) and name not in files:
            guard(name, listed)
            os.remove(os.path.join(OUTPUT, name))
            removed += 1
    written = 0
    for name, content in files.items():
        path = os.path.join(OUTPUT, name)
        existing = read_text(path)
        if existing is not None:
            guard(name, listed)
        if existing != content:
            with open(path, 'w', encoding='utf-8') as file:
                file.write(content)
            written += 1
    if written or removed:
        say('Wrote %d and removed %d files in %s' % (written, removed, shown(OUTPUT)))


def render_tags(prefix, files):
    with open(os.path.join(prefix, 'tags.json'), encoding='utf-8') as file:
        listed = json.load(file)
    names = [name[:-len('.yumi')] for name in files if name.endswith('.yumi')]
    lines = [
        MAJOR_DIVIDER,
        '+ ¶ lucide-tag-lines',
        '',
    ]
    for name in names:
        kept = [tag for tag in listed.get(name, []) if plain_tag(tag)]
        lines.append('%s|%s' % (name, ','.join(kept)) if kept else name)
    return '\n'.join(lines) + '\n'


def plain_tag(tag):
    return bool(tag) and all(character.isalnum() or character in TAG_PUNCTUATION for character in tag)


def check_tags(content):
    if read_text(TAGS_OUTPUT) != content:
        raise Failure('%s is not current; run %s' % (shown(TAGS_OUTPUT), GENERATOR))


def write_tags(content):
    if read_text(TAGS_OUTPUT) != content:
        with open(TAGS_OUTPUT, 'w', encoding='utf-8') as file:
            file.write(content)
        say('Wrote %s' % shown(TAGS_OUTPUT))


def managed(name):
    return name.endswith('.yumi') or name in (LICENSE, MANIFEST)


def generated_names():
    text = read_text(os.path.join(OUTPUT, MANIFEST))
    return set() if text is None else set(json.loads(text)['icons'])


def guard(name, listed):
    if name.endswith('.yumi') and name not in listed:
        raise Failure('Refusing to replace %s, which %s does not list' % (shown(os.path.join(OUTPUT, name)), MANIFEST))


def read_text(path):
    try:
        with open(path, encoding='utf-8') as file:
            return file.read()
    except FileNotFoundError:
        return None


def parse_svg(source_name, text):
    root = SVG_ROOT.fullmatch(COMMENT.sub('', text).strip())
    if not root:
        raise source_error(source_name, 'expected one svg root')
    attributes = parse_attributes(source_name, 'svg', root.group(1))
    known(source_name, 'svg', attributes, ROOT_ATTRIBUTES)
    for attribute, expected in ROOT_VALUES:
        actual = required(source_name, 'svg', attributes, attribute)
        if actual != expected:
            raise source_error(source_name, '<svg> requires %s=%s, received %s'
                               % (attribute, quoted(expected), quoted(actual)))
    view_box = number_tokens(source_name, 'svg viewBox', required(source_name, 'svg', attributes, 'viewBox'))
    if len(view_box) != 4:
        raise source_error(source_name, 'svg viewBox requires 4 numbers, received %d' % len(view_box))
    stroke_width = number(source_name, 'svg stroke-width', required(source_name, 'svg', attributes, 'stroke-width'))
    nodes = parse_nodes(source_name, root.group(2))
    if not nodes:
        raise source_error(source_name, 'icon has no vector nodes')
    return view_box, stroke_width, nodes


def parse_nodes(source_name, source):
    nodes = []
    cursor = 0
    while cursor < len(source):
        whitespace = WHITESPACE.match(source, cursor)
        if whitespace:
            cursor = whitespace.end()
        if cursor == len(source):
            break
        element = ELEMENT.match(source, cursor)
        if not element:
            raise source_error(source_name, 'unsupported SVG content near %s' % quoted(source[cursor:cursor + 80]))
        tag = element.group(1)
        if tag not in GEOMETRY_ATTRIBUTES:
            raise source_error(source_name, 'unsupported SVG element <%s>' % tag)
        attributes = parse_attributes(source_name, tag, element.group(2))
        known(source_name, tag, attributes, GEOMETRY_ATTRIBUTES[tag] | STYLE_ATTRIBUTES)
        for attribute in REQUIRED_GEOMETRY[tag]:
            required(source_name, tag, attributes, attribute)
        nodes.append(parse_node(source_name, tag, attributes))
        cursor = element.end()
    return nodes


def parse_node(source_name, tag, attributes):
    styles = parse_styles(source_name, tag, attributes)

    def value(attribute):
        return number(source_name, '%s %s' % (tag, attribute), required(source_name, tag, attributes, attribute))

    if tag == 'path':
        return 'path', styles, PathData(source_name, required(source_name, tag, attributes, 'd')).operations()
    if tag in ('polyline', 'polygon'):
        points = parse_points(source_name, tag, required(source_name, tag, attributes, 'points'))
        operations = [('M', list(points[0]))] + [('L', list(point)) for point in points[1:]]
        if tag == 'polygon':
            operations.append(('Z', []))
        return 'path', styles, operations
    if tag == 'circle':
        return 'circle', styles, list(point(value('cx'), value('cy'))) + [value('r')]
    if tag == 'ellipse':
        return 'ellipse', styles, list(point(value('cx'), value('cy'))) + list(point(value('rx'), value('ry')))
    if tag == 'line':
        return 'line', styles, list(point(value('x1'), value('y1'))) + list(point(value('x2'), value('y2')))
    return parse_rectangle(source_name, attributes, styles)


def parse_rectangle(source_name, attributes, styles):
    def optional(attribute, fallback):
        text = attributes.get(attribute)
        return fallback if text is None else number(source_name, 'rect %s' % attribute, text)

    x = optional('x', 0.0)
    y = optional('y', 0.0)
    width = number(source_name, 'rect width', required(source_name, 'rect', attributes, 'width'))
    height = number(source_name, 'rect height', required(source_name, 'rect', attributes, 'height'))
    if width < 0 or height < 0:
        raise source_error(source_name, 'rect width and height must be nonnegative')
    raw_rx = optional('rx', None)
    raw_ry = optional('ry', None)
    if raw_rx is None and raw_ry is None:
        return 'rectangle', styles, [x, y, width, height]
    rx = normalize(min(raw_ry if raw_rx is None else raw_rx, width / 2))
    ry = normalize(min(raw_rx if raw_ry is None else raw_ry, height / 2))
    if rx < 0 or ry < 0:
        raise source_error(source_name, 'rect corner radii must be nonnegative')
    if rx == 0 or ry == 0:
        return 'rectangle', styles, [x, y, width, height]
    right = add(x, width)
    bottom = add(y, height)
    return 'path', styles, [
        ('M', [add(x, rx), y]),
        ('L', [add(right, -rx), y]),
        arc(rx, ry, 0.0, 0.0, 1.0, right, add(y, ry)),
        ('L', [right, add(bottom, -ry)]),
        arc(rx, ry, 0.0, 0.0, 1.0, add(right, -rx), bottom),
        ('L', [add(x, rx), bottom]),
        arc(rx, ry, 0.0, 0.0, 1.0, x, add(bottom, -ry)),
        ('L', [x, add(y, ry)]),
        arc(rx, ry, 0.0, 0.0, 1.0, add(x, rx), y),
        ('Z', []),
    ]


def parse_styles(source_name, tag, attributes):
    styles = []
    if 'fill' in attributes:
        styles.append(('fill', paint(source_name, tag, 'fill', attributes['fill'])))
    if 'stroke' in attributes:
        styles.append(('stroke', paint(source_name, tag, 'stroke', attributes['stroke'])))
    if 'stroke-width' in attributes:
        width = number(source_name, '%s stroke-width' % tag, attributes['stroke-width'])
        styles.append(('stroke-width', format_number(width)))
    if 'stroke-linecap' in attributes:
        cap = choice(source_name, tag, 'stroke-linecap', attributes['stroke-linecap'], LINE_CAPS)
        styles.append(('stroke-cap', 'LineCap' + cap))
    if 'stroke-linejoin' in attributes:
        join = choice(source_name, tag, 'stroke-linejoin', attributes['stroke-linejoin'], LINE_JOINS)
        styles.append(('stroke-join', 'LineJoin' + join))
    if 'fill-rule' in attributes:
        rule = 'even-odd' if attributes['fill-rule'] == 'evenodd' else attributes['fill-rule']
        styles.append(('fill-rule', 'FillRule' + choice(source_name, tag, 'fill-rule', rule, FILL_RULES)))
    return styles


def paint(source_name, tag, attribute, value):
    if value == 'currentColor':
        return 'Fill.stroke' if attribute == 'fill' else 'Stroke.inherit'
    if value == 'none':
        return '%s.none' % ('Fill' if attribute == 'fill' else 'Stroke')
    raise source_error(source_name, '<%s> has unsupported %s=%s' % (tag, attribute, quoted(value)))


def choice(source_name, tag, attribute, value, accepted):
    if value not in accepted:
        raise source_error(source_name, '<%s> has unsupported %s=%s' % (tag, attribute, quoted(value)))
    return '.' + value


def parse_attributes(source_name, tag, source):
    attributes = {}
    cursor = 0
    while cursor < len(source):
        whitespace = WHITESPACE.match(source, cursor)
        if whitespace:
            cursor = whitespace.end()
        if cursor == len(source):
            break
        attribute = ATTRIBUTE.match(source, cursor)
        if not attribute:
            raise source_error(source_name, '<%s> has invalid attributes near %s'
                               % (tag, quoted(source[cursor:cursor + 60])))
        name = attribute.group(1)
        if name in attributes:
            raise source_error(source_name, '<%s> repeats attribute %s' % (tag, name))
        attributes[name] = attribute.group(2)
        cursor = attribute.end()
    return attributes


def known(source_name, tag, attributes, allowed):
    for attribute in attributes:
        if attribute not in allowed:
            raise source_error(source_name, '<%s> has unsupported attribute %s' % (tag, attribute))


def required(source_name, tag, attributes, attribute):
    if attribute not in attributes:
        raise source_error(source_name, '<%s> is missing %s' % (tag, attribute))
    return attributes[attribute]


def number(source_name, label, source):
    values = number_tokens(source_name, label, source)
    if len(values) != 1:
        raise source_error(source_name, '%s requires one number' % label)
    return values[0]


def parse_points(source_name, tag, source):
    values = number_tokens(source_name, '%s points' % tag, source)
    if len(values) < 2 or len(values) % 2:
        raise source_error(source_name, '<%s> points require one or more x/y pairs' % tag)
    return [point(values[index], values[index + 1]) for index in range(0, len(values), 2)]


def number_tokens(source_name, label, source):
    values = []
    cursor = 0
    while cursor < len(source):
        separators = SEPARATORS.match(source, cursor)
        if separators:
            cursor = separators.end()
        if cursor == len(source):
            break
        if source[cursor] in COMMANDS:
            raise source_error(source_name, '%s contains unexpected command %s' % (label, source[cursor]))
        match = NUMBER.match(source, cursor)
        if not match:
            raise source_error(source_name, '%s contains unsupported content near %s'
                               % (label, quoted(source[cursor:cursor + 40])))
        values.append(finite(source_name, label, match.group()))
        cursor = match.end()
    return values


def path_tokens(source_name, source):
    tokens = []
    cursor = 0
    while cursor < len(source):
        separators = SEPARATORS.match(source, cursor)
        if separators:
            cursor = separators.end()
        if cursor == len(source):
            break
        if source[cursor] in COMMANDS:
            tokens.append(source[cursor])
            cursor += 1
            continue
        match = NUMBER.match(source, cursor)
        if not match:
            raise source_error(source_name, 'path d contains unsupported content near %s'
                               % quoted(source[cursor:cursor + 40]))
        tokens.append(number_token(source_name, match.group()))
        cursor = match.end()
    return tokens


def number_token(source_name, text):
    match = NUMBER.match(text)
    if not match or match.end() != len(text):
        raise source_error(source_name, 'path d contains an invalid number %s' % quoted(text))
    return text, finite(source_name, 'path d', text)


def finite(source_name, label, text):
    value = float(text)
    if not math.isfinite(value):
        raise source_error(source_name, '%s contains a non-finite number' % label)
    return normalize(value)


class PathData:
    def __init__(self, source_name, source):
        self.source_name = source_name
        self.tokens = path_tokens(source_name, source)
        self.index = 0
        self.command = None

    def operations(self):
        operations = []
        current = subpath = (0.0, 0.0)
        previous = None
        cubic = quadratic = None
        while self.index < len(self.tokens):
            if self.at_command():
                self.command = self.tokens[self.index]
                self.index += 1
            elif self.command is None:
                raise self.error('path data starts with numbers instead of a command')
            kind = self.command.upper()
            relative = self.command != kind
            if kind == 'Z':
                operations.append(('Z', []))
                current = subpath
                previous = 'Z'
                cubic = quadratic = None
                self.command = None
                continue
            group = 0
            while self.index < len(self.tokens) and not self.at_command():
                if kind == 'A':
                    values = [self.number(), self.number(), self.number(), self.flag('large-arc'), self.flag('sweep'),
                              self.number(), self.number()]
                else:
                    values = [self.number() for _ in range(PARAMETER_COUNTS[kind])]
                effective = 'L' if kind == 'M' and group else kind
                base = current
                if effective in ('M', 'L', 'H', 'V', 'A'):
                    if effective == 'H':
                        current = point(add(base[0], values[0]) if relative else values[0], base[1])
                    elif effective == 'V':
                        current = point(base[0], add(base[1], values[0]) if relative else values[0])
                    else:
                        current = absolute(values[-2], values[-1], relative, base)
                    if effective == 'M':
                        subpath = current
                        operations.append(('M', list(current)))
                    elif effective == 'A':
                        operations.append(arc(values[0], values[1], values[2], values[3], values[4], *current))
                    else:
                        operations.append(('L', list(current)))
                    previous = effective
                    cubic = quadratic = None
                elif effective in ('C', 'S'):
                    if effective == 'C':
                        first = absolute(values[0], values[1], relative, base)
                    else:
                        first = reflected(cubic, base) if previous in ('C', 'S') else base
                    second = absolute(values[-4], values[-3], relative, base)
                    current = absolute(values[-2], values[-1], relative, base)
                    operations.append(('C', list(first + second + current)))
                    previous = effective
                    cubic = second
                    quadratic = None
                else:
                    if effective == 'Q':
                        control = absolute(values[0], values[1], relative, base)
                    else:
                        control = reflected(quadratic, base) if previous in ('Q', 'T') else base
                    current = absolute(values[-2], values[-1], relative, base)
                    operations.append(('Q', list(control + current)))
                    previous = effective
                    cubic = None
                    quadratic = control
                group += 1
            if not group:
                raise self.error('path command %s has no parameters' % self.command)
        if not operations:
            raise self.error('path data contains no operations')
        return operations

    def at_command(self):
        return isinstance(self.tokens[self.index], str)

    def parameter(self):
        if self.index >= len(self.tokens) or self.at_command():
            raise self.error('path command %s has too few parameters' % self.command)
        return self.tokens[self.index]

    def number(self):
        value = self.parameter()[1]
        self.index += 1
        return value

    def flag(self, label):
        text = self.parameter()[0]
        if text[0] not in ('0', '1'):
            raise self.error('path command %s has invalid %s flag %s' % (self.command, label, quoted(text)))
        if len(text) > 1:
            self.tokens[self.index] = number_token(self.source_name, text[1:])
        else:
            self.index += 1
        return float(text[0])

    def error(self, message):
        return source_error(self.source_name, message)


def absolute(x, y, relative, base):
    return point(add(base[0], x), add(base[1], y)) if relative else point(x, y)


def reflected(control, around):
    return point(add(around[0] * 2, -control[0]), add(around[1] * 2, -control[1]))


def point(x, y):
    return normalize(x), normalize(y)


def arc(rx, ry, rotation, large_arc, sweep, x, y):
    return 'A', [rx, ry, rotation, large_arc, sweep, x, y]


def add(left, right):
    return normalize(left + right)


def normalize(value):
    rounded = float(decimal.Decimal(value).quantize(PRECISION, rounding=decimal.ROUND_HALF_UP, context=CONTEXT))
    return 0.0 if rounded == 0 else rounded


def format_number(value):
    value = normalize(value)
    if value == 0:
        return '0'
    if abs(value) < SMALLEST_PLAIN:
        fixed = decimal.Decimal(value).quantize(PRECISION, rounding=decimal.ROUND_HALF_UP, context=CONTEXT)
        return format(fixed, 'f').rstrip('0').rstrip('.')
    sign, digits, exponent = decimal.Decimal(repr(value)).normalize(CONTEXT).as_tuple()
    digits = ''.join(map(str, digits))
    position = len(digits) + exponent
    if position >= len(digits):
        text = digits + '0' * (position - len(digits))
    elif position > 0:
        text = digits[:position] + '.' + digits[position:]
    else:
        text = '0.' + '0' * -position + digits
    return '-' + text if sign else text


def render_icon(icon_name, type_name, icon):
    lines = [
        '/graphics/primitives',
        '/gui/icon',
        '/gui/icons/lucide-icon',
        '/gui/theme',
        '',
        MAJOR_DIVIDER,
        '• extend GUIDefaultTheme',
        '',
        '• icon-%s' % icon_name,
        '    • own icon-host',
        '        <LucideIcon (first-child) • icon',
        '            glyph = lucide-glyph-program()',
        '',
        MAJOR_DIVIDER,
        '+ %s is LucideIcon' % type_name,
        '',
        MINOR_DIVIDER,
        '+ This',
        '',
        'glyph = lucide-glyph-program()',
        '',
        MAJOR_DIVIDER,
        '_ lucide-glyph-program [Coord]',
        '',
        '[%s]' % ', '.join(map(format_number, glyph_program(icon_name, icon))),
    ]
    return '\n'.join(lines) + '\n'


def glyph_program(icon_name, icon):
    source_name = icon_name + '.svg'
    view_box, stroke_width, nodes = icon
    if view_box != GLYPH_VIEW_BOX:
        raise source_error(source_name, 'unsupported viewBox %s' % ' '.join(map(format_number, view_box)))
    if stroke_width != GLYPH_STROKE_WIDTH:
        raise source_error(source_name, 'unsupported stroke-width %s' % format_number(stroke_width))
    program = []
    for kind, styles, values in nodes:
        program += [SHAPE_KINDS[kind], glyph_filled(source_name, styles)]
        if kind == 'path':
            operations = [code for command, arguments in values for code in [OPERATION_CODES[command]] + arguments]
            program += [len(values), len(operations)] + operations
        else:
            program += values
    return program


def glyph_filled(source_name, styles):
    if not styles:
        return 0
    if styles == [('fill', 'Fill.stroke')]:
        return 1
    raise source_error(source_name, 'unsupported node styles %s' % ' '.join('%s=%s' % style for style in styles))


def render_manifest(version, files):
    manifest = {
        'generator': GENERATOR,
        'source': '%s v%s' % (PACKAGE, version),
        'license': 'ISC, in %s' % LICENSE,
        'icons': [name for name in files if name.endswith('.yumi')],
    }
    return json.dumps(manifest, indent=4, ensure_ascii=False) + '\n'


def lucide_type_name(icon_name):
    if not ICON_NAME.fullmatch(icon_name):
        raise Failure('Unsupported Lucide icon name: %s' % icon_name)
    parts = icon_name.split('-')
    return 'Lucide' + ''.join('N' + part if part[0].isdigit() else part[0].upper() + part[1:] for part in parts)


def quoted(text):
    return json.dumps(text, ensure_ascii=False)


def source_error(source_name, message):
    return Failure('%s: %s' % (source_name, message))


if __name__ == '__main__':
    sys.exit(run_main(main))
