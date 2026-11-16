# Yumi TUI Style Rules For Agents

Use these rules to review working code in `stdlib/tui`, `examples/tui`, and other terminal UIs.
Cite violations by rule ID, such as `YT-02`. These are style rules, not syntax requirements;
justified exceptions are allowed when the rule's tradeoff does not fit the code.

## YT-01: Keep TUI Structure Declarative

Build TUI applications from `<App>`, `<Window>`, markup nodes, style properties, tracked reads,
and reactive streams. Do not replace rows, styles, DOM nodes, or compositing behavior with direct
terminal drawing or custom paint loops unless the renderer itself is the subject.

Incorrect:

```text
terminal.move-cursor(0, 0)
terminal.write(status-message)
terminal.clear-to-end()
```

Correct:

```text
<App
    <Window • flex-col bg:black
        <StatusBar
            message = #status-message
```

An `App` runs by itself where the main program's statements end, before the values main uses are
closed, so a resource the window reads is opened with `use` in main. It runs until its windows
close, by `window.close()` or `app.quit()`, and the program ends once main's cleanups have run,
without waiting for tasks still pending. Call `app.run()` only where code has to wait for the app
to quit, such as in a function whose own `use` closes what the window reads when the function
returns. Inside a window's body, `app` is the window's own app, so give an element there another
name.

## YT-02: Use Shortcut Nodes For Commands

Register ordinary keyboard commands with declarative `<Shortcut>` nodes near the UI that owns the
command. Use raw `key-press` or `key-input` handlers only for text editing, low-level controls, key
repeats or sequences, cancellation-only behavior, or raw terminal data that shortcuts cannot
represent.

Incorrect:

```text
for event on key-press
    if event.key == Key.escape
        close()
        event.cancel()
```

Correct:

```text
<Shortcut "Close"
    key = .escape
    action = => do close()
```

Preferred key forms include `.escape`, `.enter`, `.space`, `.arrow-up`, `'q'`, `"Ctrl+K"`, and
`"Shift+F4"`. Give a shortcut `action` for a local closure, or `command` for a shared `/ui`
`Command`; a `command` shortcut takes its title and availability from the command.

## YT-03: Scope Shortcuts With Focus Ownership

Set `focus-mode => FocusMode.keys` on a component whose shortcuts should be active only while that
component owns keyboard focus. Mount and unmount conditional shortcuts with markup `if` branches.
Keep genuinely global or window-level shortcuts beside the app node.

Incorrect:

```text
<Window
    <Shortcut "Delete selected row"
        key = .delete
        action = => do rows.delete-selection()
```

Correct:

```text
━━━━━━━━━━━━━━━━━━━━
+ RowList is Box

+ focus-mode => FocusMode.keys

────────────────────
<This
    <Shortcut "Delete selected row"
        key = .delete
        action = => do delete-selection()
```

## YT-04: Preserve Component Ownership

Keep selection, activation, and event propagation with the parent component that owns the behavior.
Leaf rows and status displays should remain presentation-oriented unless the design explicitly
makes them independent controls.

Incorrect:

```text
<ResultRow
    result = result
    for event on click
        root.selected-index = result.index
        root.preview.open(result)
```

Correct:

```text
<ResultsPanel
    for result in #results keyed by result.id
        <ResultRow
            result = result
            for event on click
                select-result(result)
                event.cancel()
```

## YT-05: Use Public Style And Runtime State

Consume state through intended style properties and `$is-*` accessors. Do not read or mutate core
layout, paint-invalidation, or sidecar state from TUI application code. Follow the markup guide for
ordinary style-shorthand and reactive-markup rules.

Incorrect:

```text
if row.$is-layout-invalid
    row.invalidate-paint()
```

Correct:

```text
• This
    background-color = if #is-focused then blue else black
```

## YT-06: Preserve Exact Rendering Semantics

Rendering and performance changes must preserve colors, cell coverage, repaint safety, focus, and
interaction behavior. Do not narrow dirty spans, skip repaint work, or weaken acceptance targets
merely to improve benchmark numbers.

Incorrect:

```text
Change: omit background-only cells from repaint spans
Evidence: frame time decreased
```

Correct:

```text
Change: coalesce adjacent dirty spans without dropping changed cells
Evidence: identical screen buffer and interaction behavior, with lower frame time
```

## Related Rules

For markup nodes, style sheets, style shorthands, and reactive markup shape, read
`docs/style/agent-yumi-style-markup.md`. To see what an app did or where its frame time goes, read
`docs/style/agent-yumi-probes.md`.

Check a TUI change by driving the built executable in a real PTY, and verify it with probes or the
screen buffer. A compile or a non-interactive test is not visual evidence.
