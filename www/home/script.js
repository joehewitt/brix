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
    const buttons = [...document.querySelectorAll('[data-source-toggle]')];
    if (!buttons.length) return;
    const apply = (mode) => {
        root.dataset.codeView = mode;
        for (const button of buttons) button.setAttribute('aria-pressed', String(mode === 'forms'));
    };
    let stored = null;
    try {
        stored = view.localStorage.getItem(codeViewKey);
    } catch {}
    apply(stored === 'forms' ? 'forms' : 'rows');
    for (const button of buttons) {
        button.hidden = false;
        button.addEventListener('click', () => {
            const mode = root.dataset.codeView === 'forms' ? 'rows' : 'forms';
            apply(mode);
            try {
                view.localStorage.setItem(codeViewKey, mode);
            } catch {}
        });
    }
}

function initializeFeatures(document) {
    const view = document.defaultView;
    const section = document.querySelector('[data-features]');
    if (!section) return;
    const card = section.querySelector('[data-feature-card]');
    const nameLabel = card.querySelector('[data-feature-name]');
    const categoryLabel = card.querySelector('[data-feature-category]');
    const body = card.querySelector('[data-feature-body]');
    const sheet = view.matchMedia('(max-width: 480px), (max-width: 760px) and (pointer: coarse)');
    const legend = new Map([...section.querySelectorAll('.feature-legend [data-category]')].map((item) => [item.dataset.category, item.textContent]));
    const templates = new Map([...section.querySelectorAll('[data-feature-template]')].map((template) => [template.dataset.featureTemplate, template]));
    const gap = 10;
    let current = null;

    const place = () => {
        body.style.removeProperty('max-height');
        if (!current || sheet.matches) {
            card.style.removeProperty('left');
            card.style.removeProperty('top');
            return;
        }
        const box = section.getBoundingClientRect();
        const term = current.getBoundingClientRect();
        const width = card.offsetWidth;
        const center = term.left + term.width / 2 - box.left;
        const left = Math.max(0, Math.min(center - width / 2, box.width - width));
        const below = view.innerHeight - term.bottom - gap * 2;
        const above = term.top - gap * 2;
        const isAbove = card.offsetHeight > below && above > below;
        const room = isAbove ? above : below;
        if (card.offsetHeight > room) body.style.maxHeight = `${Math.max(120, room - card.offsetHeight + body.offsetHeight)}px`;
        const height = card.offsetHeight;
        card.dataset.side = isAbove ? 'above' : 'below';
        card.style.left = `${left}px`;
        card.style.top = `${isAbove ? term.top - box.top - height - gap : term.bottom - box.top + gap}px`;
        card.style.setProperty('--arrow-x', `${center - left}px`);
    };

    const close = (shouldFocus = false) => {
        if (!current) return;
        const term = current;
        current = null;
        term.setAttribute('aria-expanded', 'false');
        card.hidden = true;
        section.removeAttribute('data-card-open');
        if (shouldFocus) term.focus();
    };

    const open = (term) => {
        if (current === term) {
            close();
            return;
        }
        if (current) current.setAttribute('aria-expanded', 'false');
        const template = templates.get(term.dataset.feature);
        current = term;
        term.setAttribute('aria-expanded', 'true');
        card.dataset.category = template.dataset.category;
        nameLabel.textContent = template.dataset.name;
        categoryLabel.textContent = template.dataset.size ? legend.get(template.dataset.category) ?? '' : '';
        body.replaceChildren(template.content.cloneNode(true));
        card.dataset.view = body.querySelector('.feature-code')?.dataset.view ?? 'rows';
        card.hidden = false;
        section.toggleAttribute('data-card-open', true);
        body.scrollTo(0, 0);
        place();
    };

    document.addEventListener('click', (event) => {
        const term = event.target.closest('[data-feature]');
        if (!term || !templates.has(term.dataset.feature)) return;
        event.preventDefault();
        open(term);
    });
    card.addEventListener('click', (event) => {
        if (event.target.closest('[data-feature-close], [data-example-link]')) close(event.target.closest('[data-feature-close]') !== null);
    });
    document.addEventListener('pointerdown', (event) => {
        if (!current || card.contains(event.target) || event.target.closest('[data-feature]')) return;
        close();
    });
    document.addEventListener('keydown', (event) => {
        if (event.key === 'Escape' && current) close(true);
    });
    view.addEventListener('resize', place);
    sheet.addEventListener('change', place);
}

function initializeExamples(document) {
    const view = document.defaultView;
    const browser = document.querySelector('[data-examples]');
    if (!browser) return;
    const tree = browser.querySelector('.example-tree');
    const file = browser.querySelector('.example-file');
    const body = browser.querySelector('[data-example-body]');
    const filesToggle = browser.querySelector('[data-files-toggle]');
    const stacked = view.matchMedia('(max-width: 480px), (max-width: 760px) and (pointer: coarse)');
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
            if (stacked.matches) document.getElementById('examples').scrollIntoView({ block: 'start' });
            else view.scrollTo({ top: document.documentElement.scrollHeight });
        }
        for (let folder = link.closest('details'); folder; folder = folder.parentElement.closest('details')) folder.open = true;
        reveal(link);
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
        body.scrollTo(0, 0);
        if (file.getBoundingClientRect().top < 0) file.scrollIntoView({ block: 'start' });
    };

    browser.addEventListener('wheel', (event) => {
        if (stacked.matches || event.ctrlKey || event.deltaY <= 0) return;
        const rest = browser.getBoundingClientRect().top - parseFloat(view.getComputedStyle(browser).top);
        if (rest < 1) return;
        event.preventDefault();
        const unit = [1, 16, view.innerHeight][event.deltaMode];
        view.scrollBy(0, Math.min(event.deltaY * unit, rest));
    }, { passive: false });

    const hashName = () => {
        try {
            return decodeURIComponent(view.location.hash.slice(1));
        } catch {
            return '';
        }
    };
    document.addEventListener('click', (event) => {
        const link = event.target.closest('[data-example-link]');
        if (!link || event.button !== 0 || event.metaKey || event.ctrlKey || event.shiftKey || event.altKey) return;
        const name = link.dataset.exampleLink;
        if (!links.has(name)) return;
        event.preventDefault();
        view.history.pushState(null, '', `#${name}`);
        select(name, true);
    });
    browser.dataset.kind = kindOf(first);
    view.addEventListener('hashchange', () => select(hashName() || first));
    select(hashName(), true);
}

function initializeHome(document) {
    initializeCopyButtons(document);
    initializeCodeView(document);
    initializeFeatures(document);
    initializeExamples(document);
}

initializeHome(document);
