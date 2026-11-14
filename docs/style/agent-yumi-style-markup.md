# Yumi Markup Style Rules For Agents

Use these rules to review working Yumi markup. Cite violations by rule ID, such as `YM-04`.
These are style rules, not syntax requirements; justified exceptions are allowed when the rule's
tradeoff does not fit the code.

## YM-01: Keep Markup Declarative

Represent UI structure with markup nodes, loops, conditionals, style properties, and style sheets.
Do not replace them with manual node construction, layout, text flattening, or painting unless the
renderer or layout system itself is the subject. If the declarative system cannot express required
behavior, fix the responsible component, compiler feature, or stdlib API instead of bypassing it.

Incorrect:

```text
content.clear()
for row in rows
    content += Label(row.name)
content.arrange-bounds(bounds)
```

Correct:

```text
<Box
    for row in #rows keyed by row.id
        <Label row.name
```

## YM-02: Bind Markup To Reactive State

Use tracked reads and reactive streams to keep rendered values synchronized with their source.
Prefer `#` and `##latest` dependencies over polling, request-animation bridges, or imperative
assignments whose only purpose is to copy state into a node.

Incorrect:

```text
status-label.text = status-message
request-animation-frame(this.sync-status)
```

Correct:

```text
<Label status-label
    text = #status-message
```

## YM-03: Keep Interaction With Its Owner

Put routing, selection, activation, and event propagation in the component that owns that behavior.
Repeated leaf rows and status displays should render data and delegate actions rather than reach
through the tree to mutate application state directly.

Incorrect:

```text
<Row
    item = item
    for event on click
        root.selected-item = item
        root.open-selection()
```

Correct:

```text
<Box
    for item in #items keyed by item.id
        <Row
            item = item
            for event on click
                select(item)
                event.cancel()
```

## YM-04: Prefer Style Shorthands

Put supported style shorthands after `•`. Use long property assignments inside shorthand or style
definitions and for properties that have no shorthand. The Yumi style-review procedure covers the
mechanically detectable cases.

Incorrect:

```text
<Box
    display = Display.flex
    flex-direction = FlexDirection.column
    flex = 1
    background-color = black
    padding = 1
```

Correct:

```text
<Box • flex-col flex:1 bg:black p:1
```

Common shorthand families include:

```text
bg: text:
flex-col flex-row flow-col flow-row grid:
absolute flex:
m: mx: my: mt: mr: mb: ml:
p: px: py: pt: pr: pb: pl:
border border-solid border-none border-rounded border-double border-heavy border-dashed
w: h: center: cover: inset:
```

## YM-05: Use Intended Style State

Consume style and runtime state through public style properties and `$is-*` accessors, using their
tracked `#is-*` form in markup. Do not couple application markup to focus managers, imperative
sidecars, or core-style implementation helpers.

Incorrect:

```text
background-color = if focus-manager.focused-node == this then blue else black
```

Correct:

```text
background-color = if #is-focused then blue else black
```

## Related Rules

For windows, controls, pointer and focus behavior, and graphical rendering, read
`docs/style/agent-yumi-style-gui.md`. For keyboard commands, focus scopes, terminal UI ownership,
and terminal rendering, read `docs/style/agent-yumi-style-tui.md`. For ordinary expression, lazy,
collection, type, unit, and hot-path style, read `docs/style/agent-yumi-style.md`.
