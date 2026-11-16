const themes = [
    {
        name: 'Yumi Dark',
        type: 'dark',
        colors: { editorBackground: '#111111', editorForeground: '#c9d1d9', lineNumberForeground: '#6e7681' },
        tokenColors: [
            { scope: ['comment', 'punctuation.definition.comment', 'string.comment'], settings: { foreground: '#8b949e' } },
            { scope: ['constant.other.placeholder', 'constant.character'], settings: { foreground: '#ec8e2c' } },
            { scope: ['constant', 'entity.name.constant', 'variable.other.constant', 'variable.other.enummember', 'variable.language', 'entity'], settings: { foreground: '#79c0ff' } },
            { scope: ['entity.name', 'meta.export.default', 'meta.definition.variable'], settings: { foreground: '#ffffff' } },
            { scope: ['variable.parameter.function', 'meta.block', 'meta.object.member', 'meta.embedded.expression'], settings: { foreground: '#c9d1d9' } },
            { scope: ['variable.parameter.type.yumi', 'meta.definition.variable.yumi'], settings: { foreground: '#fdac54' } },
            { scope: 'entity.name.function', settings: { foreground: '#d2a8ff' } },
            { scope: ['entity.name.tag', 'support.class.component'], settings: { foreground: '#a5d6ff' } },
            { scope: 'keyword', settings: { foreground: '#ec8e2c' } },
            { scope: ['storage', 'storage.type'], settings: { foreground: '#ec8e2c' } },
            { scope: 'string', settings: { foreground: '#a5d6ff' } },
            { scope: 'string.quoted.double.yumi constant.character.escape.yumi', settings: { foreground: '#a5d6ff' } },
            { scope: 'support', settings: { foreground: '#79c0ff' } },
            { scope: 'variable', settings: { foreground: '#ffffff' } },
            { scope: 'variable.other', settings: { foreground: '#c9d1d9' } },
            { scope: 'string variable', settings: { foreground: '#79c0ff' } },
            { scope: ['source.regexp', 'string.regexp'], settings: { foreground: '#a5d6ff' } },
            { scope: ['support.constant', 'support.variable', 'meta.module-reference'], settings: { foreground: '#79c0ff' } },
            { scope: 'punctuation.section.embedded', settings: { foreground: '#ec8e2c' } },
            { scope: 'string.embedded.yumi', settings: { foreground: '#b8a94d', fontStyle: 'bold' } },
            { scope: 'divider.class.yumi', settings: { foreground: '#c4c371' } },
            { scope: 'divider.function.yumi', settings: { foreground: '#6d6c48' } },
            { scope: ['bullet.probe.yumi', 'entity.name.function.probe.yumi'], settings: { foreground: '#a37bae', fontStyle: 'bold' } },
            { scope: 'header.probe.yumi', settings: { foreground: '#a37bae', fontStyle: 'italic' } },
            { scope: ['probe.embedded.yumi', 'probe.embedded.snapshot.yumi', 'probe.embedded.preview.yumi'], settings: { foreground: '#5b9cda', fontStyle: 'bold' } },
            { scope: 'divider.header.yumi', settings: { foreground: '#476ebd' } },
            { scope: 'name.class.yumi', settings: { foreground: '#c4c371', fontStyle: 'bold' } },
            { scope: 'name.function.solo.yumi', settings: { foreground: '#d2d2d2', fontStyle: 'bold' } },
            { scope: 'entity.name.type.yumi', settings: { foreground: '#5095db' } },
            { scope: 'definition.yumi', settings: { foreground: '#edbcf3' } },
            { scope: ['entity.name.ordinal.yumi', 'entity.name.variable.special.yumi'], settings: { foreground: '#d17277' } },
            { scope: ['pooftest.comment', 'yumitest.comment', 'yumitest.separator.comment'], settings: { foreground: '#d17277' } },
            { scope: ['yumitest.separator.start', 'yumitest.separator.middle', 'yumitest.separator.end'], settings: { foreground: '#58a6ff', fontStyle: 'bold' } },
            { scope: 'pooftest.separator.start', settings: { foreground: '#c4c371', fontStyle: 'bold' } },
            { scope: 'pooftest.separator.middle', settings: { foreground: '#4a4d40', fontStyle: 'bold' } },
            { scope: 'pooftest.separator.end', settings: { foreground: '#292c33' } }
        ]
    },
    {
        name: 'Carbon Garden',
        type: 'dark',
        colors: { editorBackground: '#111312', editorForeground: '#e7ebe8', lineNumberForeground: '#748078' },
        tokenColors: [
            { scope: 'comment.yumi', settings: { foreground: '#748078', fontStyle: 'italic' } },
            { scope: ['divider.class.yumi', 'divider.function.yumi', 'divider.header.yumi'], settings: { foreground: '#70877a' } },
            { scope: 'keyword', settings: { foreground: '#ff9b73' } },
            { scope: 'variable.parameter.type.yumi', settings: { foreground: '#ff9b73' } },
            { scope: ['entity.name.type.yumi', 'storage.type.function.yumi'], settings: { foreground: '#80c7f2' } },
            { scope: ['name.function.solo.yumi', 'entity.name.function.stream.yumi'], settings: { foreground: '#4fd1a1' } },
            { scope: ['string.quoted.double.yumi', 'constant.character.yumi'], settings: { foreground: '#e5c07b' } },
            { scope: ['constant.numeric.integer.decimal.yumi', 'constant.numeric.float.yumi', 'constant.language.yumi'], settings: { foreground: '#c7a0f5' } },
            { scope: ['operator.infix.yumi', 'operator.assign.yumi', 'operator.bullet.yumi', 'operator.printing.yumi'], settings: { foreground: '#c5d0ca' } }
        ]
    },
    {
        name: 'Paper Signal',
        type: 'light',
        colors: { editorBackground: '#fbfcfa', editorForeground: '#202522', lineNumberForeground: '#7f8983' },
        tokenColors: [
            { scope: 'comment.yumi', settings: { foreground: '#68726c', fontStyle: 'italic' } },
            { scope: ['divider.class.yumi', 'divider.function.yumi', 'divider.header.yumi'], settings: { foreground: '#9a5b38' } },
            { scope: 'keyword', settings: { foreground: '#a13c2f' } },
            { scope: 'variable.parameter.type.yumi', settings: { foreground: '#a13c2f' } },
            { scope: ['entity.name.type.yumi', 'storage.type.function.yumi'], settings: { foreground: '#205fa6' } },
            { scope: ['name.function.solo.yumi', 'entity.name.function.stream.yumi'], settings: { foreground: '#08765c' } },
            { scope: ['string.quoted.double.yumi', 'constant.character.yumi'], settings: { foreground: '#7a5814' } },
            { scope: ['constant.numeric.integer.decimal.yumi', 'constant.numeric.float.yumi', 'constant.language.yumi'], settings: { foreground: '#7544a0' } },
            { scope: ['operator.infix.yumi', 'operator.assign.yumi', 'operator.bullet.yumi', 'operator.printing.yumi'], settings: { foreground: '#3f4b45' } }
        ]
    },
    {
        name: 'Terminal Contrast',
        type: 'dark',
        colors: { editorBackground: '#090909', editorForeground: '#ffffff', lineNumberForeground: '#858585' },
        tokenColors: [
            { scope: 'comment.yumi', settings: { foreground: '#a8a8a8' } },
            { scope: ['divider.class.yumi', 'divider.function.yumi', 'divider.header.yumi'], settings: { foreground: '#ffcf40' } },
            { scope: 'keyword', settings: { foreground: '#ff745f', fontStyle: 'bold' } },
            { scope: 'variable.parameter.type.yumi', settings: { foreground: '#ffcf40' } },
            { scope: ['entity.name.type.yumi', 'storage.type.function.yumi'], settings: { foreground: '#58b9ff' } },
            { scope: ['name.function.solo.yumi', 'entity.name.function.stream.yumi'], settings: { foreground: '#59e391' } },
            { scope: ['string.quoted.double.yumi', 'constant.character.yumi'], settings: { foreground: '#ffe072' } },
            { scope: ['constant.numeric.integer.decimal.yumi', 'constant.numeric.float.yumi', 'constant.language.yumi'], settings: { foreground: '#d9a0ff' } }
        ]
    }
];

const selector = document.querySelector('#theme-select');
const specificNode = document.querySelector('#specific-node');
const nodeStack = document.querySelector('#node-stack');
const sourceContexts = [...document.querySelectorAll('[data-yumi-source]')].map((sourceRoot) => {
    const ast = JSON.parse(sourceRoot.querySelector('[data-yumi-ast]').textContent);
    const runs = [...sourceRoot.querySelectorAll('[data-yumi-run]')];
    const nodes = new Map(ast.nodes.map((node) => [node.id, node]));
    const code = sourceRoot.querySelector('.source-yumi');
    const lineNumbers = document.createElement('span');
    lineNumbers.className = 'yumi-line-numbers';
    lineNumbers.setAttribute('aria-hidden', 'true');
    lineNumbers.textContent = Array.from(
        { length: code.textContent.split('\n').length },
        (_, index) => String(index + 1)
    ).join('\n');
    code.parentElement.prepend(lineNumbers);
    return { sourceRoot, runs, nodes };
});
const runs = sourceContexts.flatMap((context) => context.runs);

for (const run of runs) {
    if (run.dataset.divider && run.textContent) {
        run.dataset.dividerFill = run.textContent[0].repeat(512);
        run.style.setProperty('--divider-source-width', `${run.textContent.length}ch`);
    }
}

for (const [index, theme] of themes.entries()) {
    const option = document.createElement('option');
    option.value = String(index);
    option.textContent = theme.name;
    selector.append(option);
}

function scopeMatches(rule, actual) {
    return actual === rule || actual.startsWith(`${rule}.`);
}

function selectorMatch(selector, scopes) {
    const parts = selector.trim().split(/\s+/);
    let scopeIndex = scopes.length - 1;
    let depth = -1;
    for (let partIndex = parts.length - 1; partIndex >= 0; partIndex -= 1) {
        while (scopeIndex >= 0 && !scopeMatches(parts[partIndex], scopes[scopeIndex])) {
            scopeIndex -= 1;
        }
        if (scopeIndex < 0) return null;
        if (depth < 0) depth = scopeIndex;
        scopeIndex -= 1;
    }
    return {
        depth,
        ancestors: parts.length,
        specificity: parts.reduce((total, part) => total + part.split('.').length, 0)
    };
}

function strongerMatch(left, right) {
    if (!right) return true;
    if (left.depth !== right.depth) return left.depth > right.depth;
    if (left.ancestors !== right.ancestors) return left.ancestors > right.ancestors;
    if (left.specificity !== right.specificity) return left.specificity > right.specificity;
    return left.order > right.order;
}

function tokenSettings(theme, scopes) {
    const selected = new Map();
    for (const [order, tokenColor] of theme.tokenColors.entries()) {
        const selectors = Array.isArray(tokenColor.scope) ? tokenColor.scope : [tokenColor.scope];
        for (const selector of selectors) {
            const match = selectorMatch(selector, scopes);
            if (!match) continue;
            const ranked = { ...match, order };
            for (const [key, value] of Object.entries(tokenColor.settings)) {
                if (strongerMatch(ranked, selected.get(key)?.rank)) {
                    selected.set(key, { rank: ranked, value });
                }
            }
        }
    }
    return Object.fromEntries([...selected].map(([key, item]) => [key, item.value]));
}

function applyTheme(theme) {
    document.documentElement.style.colorScheme = theme.type;
    document.documentElement.style.setProperty('--code-bg', theme.colors.editorBackground);
    document.documentElement.style.setProperty('--text', theme.colors.editorForeground);
    document.documentElement.style.setProperty('--gutter-text', theme.colors.lineNumberForeground);
    for (const run of runs) {
        const scopes = run.dataset.scopes.split(' ');
        const settings = tokenSettings(theme, scopes);
        const styles = settings.fontStyle?.split(' ') ?? [];
        run.style.color = settings.foreground ?? theme.colors.editorForeground;
        run.style.backgroundColor = settings.background ?? '';
        run.style.fontStyle = styles.includes('italic') ? 'italic' : 'normal';
        run.style.fontWeight = styles.includes('bold') ? '700' : '400';
        run.style.textDecoration = styles.includes('underline')
            ? 'underline'
            : styles.includes('strikethrough')
              ? 'line-through'
              : 'none';
    }
}

function clearInspection() {
    for (const run of runs) run.classList.remove('is-inspected');
}

function resetInspection() {
    clearInspection();
    specificNode.textContent = 'No selection';
    nodeStack.replaceChildren();
}

function showNodeStack(context, ids) {
    nodeStack.replaceChildren();
    const visible = ids.map((id) => context.nodes.get(id)).filter(Boolean);
    const mostSpecific = visible.at(-1);
    specificNode.textContent = mostSpecific ? `${mostSpecific.kind} #${mostSpecific.id}` : 'No source-backed node';
    for (const node of visible) {
        const row = document.createElement('div');
        row.className = `node-row${node === mostSpecific ? ' is-specific' : ''}`;
        const title = document.createElement('div');
        title.className = 'node-title';
        const kind = document.createElement('strong');
        kind.textContent = `${node.kind} #${node.id}`;
        const range = document.createElement('span');
        range.textContent = `${node.range[0]}:${node.range[1]}`;
        title.append(kind, range);
        row.append(title);
        const entries = Object.entries(node.fields);
        if (entries.length) {
            const fields = document.createElement('pre');
            fields.className = 'node-fields';
            fields.textContent = entries.map(([key, value]) => `${key}: ${value}`).join('\n');
            row.append(fields);
        }
        nodeStack.append(row);
    }
    clearInspection();
    if (mostSpecific) {
        for (const run of context.runs) {
            const [start, end] = run.dataset.range.split(':').map(Number);
            if (start < mostSpecific.range[1] && end > mostSpecific.range[0]) run.classList.add('is-inspected');
        }
    }
}

selector.addEventListener('change', () => applyTheme(themes[Number(selector.value)]));
for (const context of sourceContexts) {
    context.sourceRoot.addEventListener('pointerover', (event) => {
        const run = event.target.closest('[data-yumi-run]');
        if (!run) {
            resetInspection();
            return;
        }
        showNodeStack(context, run.dataset.nodes.split(',').filter(Boolean).map(Number));
    });
}

document.addEventListener('pointerover', (event) => {
    if (!event.target.closest?.('[data-yumi-run]')) resetInspection();
});
document.documentElement.addEventListener('pointerleave', resetInspection);
window.addEventListener('blur', resetInspection);

applyTheme(themes[0]);
