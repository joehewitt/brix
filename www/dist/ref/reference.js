function textNodeIndex(code) {
    const walker = code.ownerDocument.createTreeWalker(code, 4);
    const entries = [];
    let offset = 0;
    for (let node = walker.nextNode(); node; node = walker.nextNode()) {
        entries.push({ node, start: offset, end: offset + node.data.length });
        offset += node.data.length;
    }
    return entries;
}

function sourceRangeToDOM(document, entries, start, end) {
    const first = entries.find((entry) => entry.start <= start && start < entry.end);
    const last = entries.find((entry) => entry.start < end && end <= entry.end);
    if (!first || !last || start >= end) throw new Error(`Invalid DOM source range ${start}:${end}`);
    const range = document.createRange();
    range.setStart(first.node, start - first.start);
    range.setEnd(last.node, end - last.start);
    return range;
}

function featuresAtSegment(features, start, end) {
    const matches = features.flatMap((feature) => feature.ranges
        .filter((range) => range.start <= start && range.end >= end)
        .map((range) => ({ id: feature.id, width: range.end - range.start })));
    const width = Math.min(...matches.map((match) => match.width));
    return [...new Set(matches.filter((match) => match.width === width).map((match) => match.id))];
}

function activeFeatureIds(state) {
    if (state.hoverIds?.length) return state.hoverIds;
    if (state.focusId) return [state.focusId];
    return state.pinnedId ? [state.pinnedId] : [];
}

function sourceHighlightBoxes(ranges) {
    const rects = ranges.flatMap((range) => [...range.getClientRects()])
        .filter((rect) => rect.width > 0 && rect.height > 0)
        .map(({ left, right, top, bottom }) => ({ left, right, top, bottom }))
        .sort((a, b) => a.top - b.top || a.left - b.left);
    const boxes = [];
    for (const rect of rects) {
        const previous = boxes.at(-1);
        if (previous && Math.abs(previous.top - rect.top) < 1
            && Math.abs(previous.bottom - rect.bottom) < 1 && rect.left <= previous.right + 1) {
            previous.right = Math.max(previous.right, rect.right);
        } else {
            boxes.push(rect);
        }
    }
    return boxes;
}

function mountCodeFeatures(root, paintRanges = () => false) {
    const document = root.ownerDocument;
    const data = JSON.parse(root.querySelector('[data-code-features]').textContent);
    const code = root.querySelector('.source-yumi');
    if (code.textContent !== data.source) throw new Error('Rendered example differs from annotated source');
    const entries = textNodeIndex(code);
    const domRanges = new Map(data.features.map((feature) => [feature.id, feature.ranges.map(
        (range) => sourceRangeToDOM(document, entries, range.start, range.end)
    )]));
    const links = [...document.querySelectorAll('[data-code-link]')]
        .filter((link) => link.dataset.codeExampleId === root.dataset.codeExample);
    const segments = [...code.querySelectorAll('[data-feature-ids]')];
    const state = { hoverIds: null, focusId: null, pinnedId: null };
    const update = () => {
        const ids = activeFeatureIds(state);
        for (const link of links) {
            link.classList.toggle('is-active', ids.includes(link.dataset.codeFeature));
            link.classList.toggle('is-pinned', link.dataset.codeFeature === state.pinnedId);
        }
        for (const segment of segments) {
            segment.classList.toggle('yumi-feature-active', segment.dataset.featureIds.split(' ').some((id) => ids.includes(id)));
        }
        paintRanges(root, ids.flatMap((id) => domRanges.get(id) ?? []));
    };
    const clearHover = () => {
        state.hoverIds = null;
        update();
    };
    const escape = (event) => {
        if (event.key !== 'Escape') return;
        state.pinnedId = null;
        state.hoverIds = null;
        state.focusId = null;
        update();
    };
    root.addEventListener('pointerover', (event) => {
        const segment = event.target.closest?.('[data-source-range]');
        state.hoverIds = segment
            ? featuresAtSegment(data.features, ...segment.dataset.sourceRange.split(':').map(Number))
            : null;
        update();
    });
    root.addEventListener('pointerleave', clearHover);
    root.addEventListener('keydown', escape);
    for (const link of links) {
        link.addEventListener('pointerenter', () => {
            state.hoverIds = [link.dataset.codeFeature];
            update();
        });
        link.addEventListener('pointerleave', clearHover);
        link.addEventListener('focus', () => {
            state.focusId = link.dataset.codeFeature;
            state.hoverIds = null;
            update();
        });
        link.addEventListener('blur', () => {
            state.focusId = null;
            update();
        });
        link.addEventListener('click', (event) => {
            if (event.ctrlKey || event.metaKey || event.shiftKey || event.altKey) return;
            const id = link.dataset.codeFeature;
            state.pinnedId = state.pinnedId === id ? null : id;
            state.hoverIds = null;
            state.focusId = null;
            update();
        });
        link.addEventListener('keydown', escape);
    }
    update();
    return state;
}

function activeChapterIndex(tops, readingLine, atBottom = false) {
    if (!tops.length) return -1;
    if (atBottom) return tops.length - 1;
    let active = 0;
    for (let index = 0; index < tops.length; index += 1) {
        if (tops[index] <= readingLine) active = index;
        else break;
    }
    return active;
}

function initializeChapterNavigation(document) {
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

function initializeReference(document) {
    initializeChapterNavigation(document);
    for (const sourceRoot of document.querySelectorAll('[data-yumi-source]')) {
        const code = sourceRoot.querySelector('.source-yumi');
        const lineNumbers = document.createElement('span');
        lineNumbers.className = 'yumi-line-numbers';
        lineNumbers.setAttribute('aria-hidden', 'true');
        lineNumbers.textContent = Array.from(
            { length: code.textContent.split('\n').length },
            (_, index) => String(index + 1)
        ).join('\n');
        code.parentElement.prepend(lineNumbers);
    }
    for (const root of document.querySelectorAll('[data-code-example]')) {
        const code = root.querySelector('.source-yumi');
        const layer = document.createElement('span');
        layer.className = 'code-source-highlights';
        layer.setAttribute('aria-hidden', 'true');
        code.parentElement.append(layer);
        let activeRanges = [];
        const repaint = () => {
            const origin = layer.getBoundingClientRect();
            const boxes = sourceHighlightBoxes(activeRanges).map((rect) => {
                const box = document.createElement('span');
                box.className = 'code-source-highlight';
                box.style.left = `${rect.left - origin.left - 5}px`;
                box.style.top = `${rect.top - origin.top - 3}px`;
                box.style.width = `${rect.right - rect.left + 10}px`;
                box.style.height = `${rect.bottom - rect.top + 6}px`;
                return box;
            });
            layer.replaceChildren(...boxes);
        };
        mountCodeFeatures(root, (_, ranges) => {
            activeRanges = ranges;
            repaint();
            return true;
        });
        const view = document.defaultView;
        view.addEventListener('resize', repaint);
        if (view.ResizeObserver) new view.ResizeObserver(repaint).observe(code);
        document.fonts?.ready.then(repaint);
    }
}

if (typeof document !== 'undefined') initializeReference(document);
