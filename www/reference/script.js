export function activeChapterIndex(tops, readingLine, atBottom = false) {
    if (!tops.length) return -1;
    if (atBottom) return tops.length - 1;
    let active = 0;
    for (let index = 0; index < tops.length; index += 1) {
        if (tops[index] <= readingLine) active = index;
        else break;
    }
    return active;
}

export function initializeChapterNavigation(document) {
    const navigation = document.querySelector('[data-chapter-navigation]');
    if (!navigation) return;
    const view = document.defaultView;
    const panel = navigation.querySelector('[data-chapter-toc]');
    const rail = navigation.querySelector('[data-chapter-rail]');
    const close = navigation.querySelector('[data-chapter-close]');
    const links = [...navigation.querySelectorAll('[data-chapter-link]')];
    const chapters = [...document.querySelectorAll('[data-chapter]')];
    let touchOpens = false;
    const setOpen = (open) => {
        panel.hidden = !open;
    };
    const dismiss = () => {
        touchOpens = false;
        setOpen(false);
    };
    navigation.addEventListener('pointerenter', (event) => {
        if (event.pointerType !== 'touch') setOpen(true);
    });
    navigation.addEventListener('pointerleave', () => {
        if (!navigation.contains(document.activeElement)) setOpen(false);
    });
    navigation.addEventListener('focusin', () => setOpen(true));
    navigation.addEventListener('focusout', (event) => {
        if (!navigation.contains(event.relatedTarget)) dismiss();
    });
    navigation.addEventListener('pointerdown', (event) => {
        touchOpens = event.pointerType === 'touch' && panel.hidden && rail.contains(event.target);
    });
    const closeAndFocus = () => {
        const current = links.find((link) => rail.contains(link) && link.getAttribute('aria-current') === 'location') ?? links[0];
        current?.focus({ preventScroll: true });
        dismiss();
    };
    close.addEventListener('click', closeAndFocus);
    navigation.addEventListener('keydown', (event) => {
        if (event.key !== 'Escape') return;
        event.preventDefault();
        closeAndFocus();
    });
    document.addEventListener('pointerdown', (event) => {
        if (!navigation.contains(event.target)) dismiss();
    });
    document.addEventListener('keydown', (event) => {
        if (event.key === 'Escape' && !panel.hidden) dismiss();
    });
    for (const link of links) link.addEventListener('click', (event) => {
        if (event.ctrlKey || event.metaKey || event.shiftKey || event.altKey) return;
        if (touchOpens && rail.contains(link)) {
            event.preventDefault();
            touchOpens = false;
            setOpen(true);
            return;
        }
        document.getElementById(link.dataset.chapterLink)?.focus({ preventScroll: true });
        dismiss();
    });
    let scheduled = false;
    let current = null;
    const update = () => {
        scheduled = false;
        const atBottom = view.scrollY > 0 && view.scrollY + view.innerHeight >= document.documentElement.scrollHeight - 2;
        const index = activeChapterIndex(chapters.map((chapter) => chapter.getBoundingClientRect().top), Math.min(180, view.innerHeight * 0.25), atBottom);
        const id = chapters[index]?.id;
        if (!id || id === current) return;
        current = id;
        for (const link of links) {
            if (link.dataset.chapterLink === id) link.setAttribute('aria-current', 'location');
            else link.removeAttribute('aria-current');
        }
    };
    const schedule = () => {
        if (scheduled) return;
        scheduled = true;
        view.requestAnimationFrame(update);
    };
    view.addEventListener('scroll', schedule, { passive: true });
    view.addEventListener('resize', schedule);
    view.addEventListener('hashchange', schedule);
    view.addEventListener('load', schedule);
    document.fonts?.ready.then(schedule);
    if (view.ResizeObserver) new view.ResizeObserver(schedule).observe(document.documentElement);
    update();
}

export function initializeCodeView(document) {
    const root = document.documentElement;
    const view = document.defaultView;
    const key = 'brix-code-view';
    const buttons = [...document.querySelectorAll('[data-brix-code]')].map((block) => {
        const button = document.createElement('button');
        button.type = 'button';
        button.className = 'code-view-toggle';
        button.textContent = 'Source';
        button.title = 'Show code as Brix source';
        block.append(button);
        return button;
    });
    const apply = (mode) => {
        root.dataset.codeView = mode;
        for (const button of buttons) button.setAttribute('aria-pressed', String(mode === 'forms'));
    };
    let stored = null;
    try {
        stored = view.localStorage.getItem(key);
    } catch {}
    apply(stored === 'forms' ? 'forms' : 'rows');
    for (const button of buttons) {
        button.addEventListener('click', () => {
            const before = button.parentElement.getBoundingClientRect().top;
            const mode = root.dataset.codeView === 'forms' ? 'rows' : 'forms';
            apply(mode);
            try {
                view.localStorage.setItem(key, mode);
            } catch {}
            const shift = button.parentElement.getBoundingClientRect().top - before;
            view.scrollBy({ top: shift, behavior: 'instant' });
        });
    }
}

export function initializeReference(document) {
    initializeChapterNavigation(document);
    initializeCodeView(document);
}

if (typeof document !== 'undefined') initializeReference(document);
