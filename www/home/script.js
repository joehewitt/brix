const codeViewKey = 'brix-code-view';

function initializeCopyButtons(document) {
    const view = document.defaultView;
    for (const button of document.querySelectorAll('[data-copy]')) {
        const source = button.parentElement.querySelector('[data-copy-text]');
        let reset = 0;
        button.hidden = false;
        button.addEventListener('click', async () => {
            try {
                await view.navigator.clipboard.writeText(source.textContent);
                button.textContent = 'Copied';
            } catch {
                const selection = view.getSelection();
                selection.selectAllChildren(source);
                const copied = document.execCommand('copy');
                if (copied) selection.removeAllRanges();
                button.textContent = copied ? 'Copied' : 'Selected';
            }
            view.clearTimeout(reset);
            reset = view.setTimeout(() => {
                button.textContent = 'Copy';
            }, 1600);
        });
    }
}

function initializeCodeView(document) {
    const root = document.documentElement;
    const view = document.defaultView;
    const button = document.querySelector('[data-source-toggle]');
    if (!button) return;
    const apply = (mode) => {
        root.dataset.codeView = mode;
        button.setAttribute('aria-pressed', String(mode === 'forms'));
    };
    let stored = null;
    try {
        stored = view.localStorage.getItem(codeViewKey);
    } catch {}
    apply(stored === 'forms' ? 'forms' : 'rows');
    button.hidden = false;
    button.addEventListener('click', () => {
        const mode = root.dataset.codeView === 'forms' ? 'rows' : 'forms';
        apply(mode);
        try {
            view.localStorage.setItem(codeViewKey, mode);
        } catch {}
    });
}

function initializeExamples(document) {
    const view = document.defaultView;
    const browser = document.querySelector('[data-examples]');
    if (!browser) return;
    const tree = browser.querySelector('.example-tree');
    const file = browser.querySelector('.example-file');
    const body = browser.querySelector('[data-example-body]');
    const label = browser.querySelector('[data-example-path]');
    const filesToggle = browser.querySelector('[data-files-toggle]');
    const links = new Map([...browser.querySelectorAll('[data-example]')].map((link) => [link.dataset.example, link]));
    const first = browser.dataset.current;
    const fragments = new Map([[first, body.innerHTML]]);
    const kindOf = (name) => (name.endsWith('.md') ? 'text' : 'code');
    let current = first;
    let shown = first;
    let request = 0;

    const reveal = (link) => {
        const box = tree.getBoundingClientRect();
        const item = link.getBoundingClientRect();
        if (item.top < box.top || item.bottom > box.bottom) tree.scrollTop += item.top - box.top - box.height / 3;
    };
    const setTreeOpen = (open) => {
        browser.toggleAttribute('data-tree-open', open);
        filesToggle.setAttribute('aria-expanded', String(open));
        if (open) reveal(links.get(current));
    };
    browser.toggleAttribute('data-tree-collapsible', true);
    filesToggle.hidden = false;
    filesToggle.addEventListener('click', () => setTreeOpen(!browser.hasAttribute('data-tree-open')));
    tree.addEventListener('click', (event) => {
        if (event.target.closest('[data-example]')) setTreeOpen(false);
    });
    document.addEventListener('pointerdown', (event) => {
        if (!browser.hasAttribute('data-tree-open')) return;
        if (!tree.contains(event.target) && !filesToggle.contains(event.target)) setTreeOpen(false);
    });
    document.addEventListener('keydown', (event) => {
        if (event.key !== 'Escape' || !browser.hasAttribute('data-tree-open')) return;
        setTreeOpen(false);
        filesToggle.focus();
    });

    const fragment = async (name) => {
        if (fragments.has(name)) return fragments.get(name);
        const response = await view.fetch(`examples/${name.split('/').map(encodeURIComponent).join('/')}.html`);
        if (!response.ok) throw new Error(`${response.status}`);
        const html = await response.text();
        fragments.set(name, html);
        return html;
    };

    const select = async (name, isLanding = false) => {
        const link = links.get(name);
        if (!link || (name === shown && !isLanding)) return;
        current = name;
        for (const other of links.values()) other.removeAttribute('aria-current');
        link.setAttribute('aria-current', 'page');
        if (isLanding) {
            for (const folder of tree.querySelectorAll('details[open]')) folder.open = false;
            document.getElementById('examples').scrollIntoView({ block: 'start' });
        }
        for (let folder = link.closest('details'); folder; folder = folder.parentElement.closest('details')) folder.open = true;
        reveal(link);
        label.textContent = name;
        browser.dataset.kind = kindOf(name);
        if (name === shown) return;
        shown = null;
        const ticket = ++request;
        body.setAttribute('aria-busy', 'true');
        let html;
        let loaded = true;
        try {
            html = await fragment(name);
        } catch {
            html = '<p class="example-error">This example could not be loaded.</p>';
            loaded = false;
        }
        if (ticket !== request) return;
        body.innerHTML = html;
        shown = loaded ? name : null;
        body.removeAttribute('aria-busy');
        if (file.getBoundingClientRect().top < 0) file.scrollIntoView({ block: 'start' });
    };

    const hashName = () => {
        try {
            return decodeURIComponent(view.location.hash.slice(1));
        } catch {
            return '';
        }
    };
    browser.dataset.kind = kindOf(first);
    view.addEventListener('hashchange', () => select(hashName() || first));
    select(hashName(), true);
}

function initializeHome(document) {
    initializeCopyButtons(document);
    initializeCodeView(document);
    initializeExamples(document);
}

initializeHome(document);
