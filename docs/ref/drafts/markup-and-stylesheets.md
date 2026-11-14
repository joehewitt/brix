# Markup and stylesheets

Yumi can describe an object tree directly in source. Markup names the objects, their properties,
and their children; stylesheets collect rules for their appearance and layout; reactive
expressions connect them to changing state.

This makes interfaces part of the language's expression vocabulary. The program can describe
a tree in the same source language as its data model, event handlers, and computations.

## At a glance

| Capability | What it gives you |
| --- | --- |
| Tree structure | `<` creates nodes; indentation groups children. |
| Properties | Assign ordinary values and expressions. |
| Content composition | Slots accept caller-supplied content; branches and repetition follow data. |
| Styles | `•` introduces styling; named rules collect shared decisions. |
| Live behavior | Tracked state, events, and subscriptions connect the tree to the application. |
| Targets | Terminal and graphical libraries supply concrete controls and rendering. |

## The shape of the source follows the tree

This fragment uses the terminal library's boxes and labels:

:::example-pair

- [Object node](#source-tree--parent): Construct a box with identity, properties, and behavior.
- [Layout and spacing](#source-tree--style): Introduce style information beside the node.
- [Children](#source-tree--children): Indentation places two labels inside the box; their text is a constructor argument.

```yumi #tree
⟦parent⟧<Box⟦/parent⟧ ⟦style⟧• flex-col p:1⟦/style⟧
⟦children⟧    <Label "Account"
    <Label "Ready" • text:green⟦/children⟧
```

:::

Markup constructs objects. A node can have identity, expose properties, participate in events,
and own resources. This is distinct from a text template that produces a string containing
tags. A node's type remains connected to ordinary Yumi declarations and behavior.

The tree form removes much of the visual overhead of allocating objects one at a time and
then connecting parent and child references. It also makes the nesting visible where a reader
is deciding how the interface is composed.

## Properties remain expressions

Property assignments can use ordinary Yumi values and expressions. This fragment illustrates
the graphical library's window markup:

:::example-pair

- [Property assignment](#source-properties--title): Supply an ordinary string value.
- [Structured value](#source-properties--size): Properties use Yumi expressions, including tuples.
- [Nested object](#source-properties--child): The window also owns child content.

```yumi #properties
<Window
    ⟦title⟧title = "Report"⟦/title⟧
    ⟦size⟧size = (720, 480)⟦/size⟧
    background = "#f7f7f9"

    ⟦child⟧<Box • absolute cover:.parent⟦/child⟧
```

:::

The markup is not a separate configuration language with its own limited calculation rules.
A property's value can come from the same functions and types used elsewhere in the program.
Likewise, behavior can live in ordinary methods instead of becoming a large inline block
inside the tree.

## Composition includes content

| Composition feature | What it lets a component do |
| --- | --- |
| Slots | Accept caller-supplied content, such as a dialog body and actions |
| Slot forwarding | Pass content through an intermediate component |
| Conditional markup | Choose the branch that matches application state |
| Repeated markup | Construct children from a collection of domain values |

The component owns the surrounding structure, layout, and behavior. The caller supplies the pieces that vary.

## Styles collect decisions that belong together

Style shorthands make common combinations concise:

```yumi #styles
<Box • ⟦layout⟧flex-row p:1⟦/layout⟧ ⟦border⟧border-rounded border:aqua⟦/border⟧
<Label "Ready" • ⟦color⟧bg:blue text:white⟦/color⟧
```

- [Arrangement](#source-styles--layout): Keep layout and spacing close to the affected node.
- [Border](#source-styles--border): Combine shorthands for shape and color.
- [Colors](#source-styles--color): Style the label's background and text.

Here layout, spacing, border, and color decisions remain close to the relevant node. Reusable
rules can move repeated choices into a stylesheet. This declaration fragment defines a
named style in a context that supplies the corresponding properties and shorthands:

```text
• status-panel • flex-col p:1 bg:black
```

A node can then refer to `status-panel` in its style list. Stylesheets can group and extend
rules, while selectors and state-dependent rules let appearance follow the role and state
of nodes in the tree.

A rule can hold other rules for the nodes beneath the one it is applied to. A nested rule
reaches every match, including content written in the node's body or given to its slots. A
nested rule written with `own` reaches only the node's own content: the markup its class and
the classes it extends declare. An icon rule can then fill a button's icon host without
reaching the icon host of a menu item in the button's menu:

```text
• icon-ellipsis
    • own icon-host
        <LucideIcon (first-child) • icon
            glyph = lucide-glyph-program()
```

The advantage grows when the same components appear throughout an application. A common
style can express a shared decision once, while a particular component can retain the
properties that are specific to its own behavior.

A stylesheet can also declare an animation as one of its values. Each track names a property
the animation moves, and a rule plays the animation by naming it:

```text
• pop-in animation 220ms
    opacity = 0%% ..
    translate = Point(0, 6) .. Point(0, 0)

• toast
    animation = pop-in
```

The compiler checks each track's property and value type against the styles the sheet uses. A
variant of the sheet, such as the one for reduced motion, restates the animation as it restates
any other value.

Arrival and departure are states too. A node starts in `#is-entering` and moves to its own values
once it has appeared. When a branch or a collection drops it, it enters `#is-leaving` and stays in
the tree until its leave has played:

```text
• toast
    motion = gui-motion-smooth

    when #is-entering
        opacity = 0%%

    when #is-leaving
        opacity = 0%%
```

Layout changes move too. A node with `motion.size` or `motion.position` travels from the size and
place it had to the ones layout gives it, and the code that caused the change doesn't measure
anything. Code can also move a change that the nodes don't declare motion for by making it inside
`do use motion(...)`.

Two nodes that show the same thing in different places can share a `match` key. When one goes and
the other arrives in the same frame, the new node flies from where the old one was to its own
place.

A dragged node can be carried. It follows the pointer while its siblings make room for it, and
when it's dropped it settles into its new place at the speed it was thrown.

An animation can follow something other than time. With a `timeline`, a scroller's offset or a
tracked amount says how far along it is, so a divider can fade in as a list scrolls away from its
top, or a sheet can open as far as it is dragged.

## State can stay connected to the declaration

An interface needs to change after it is created. Tracked reads identify the values that a
live expression depends on. For example, a property assignment can read `#title` so that the
declaration participates in changes to the title, rather than merely taking an initial copy.

Streams can supply changing values too. Latest-value expressions make the current value of
a stream available to a live calculation. That connects network results, application events,
and state changes to the same expression system.

This does not remove the need to decide who owns state. A component should have a clear
relationship to the values it changes and the subscriptions it creates. The payoff is that
much of the display can remain a declaration of how that state should appear.

[Concurrency and reactivity](concurrency-and-reactivity.md) explains the tracked-value and
subscription model separately from its use in a user interface.

[State machines](state-machines.md) organize behavior around named phases and the events
that move between them. Their state tests and case projections can drive tracked properties
and styles, keeping a component's appearance connected to its current phase.

## Events connect the tree to behavior

Nodes can publish events and subscribe to other nodes' events. A parent component can own
selection, command routing, or dialog behavior while its children focus on displaying data
and exposing interactions.

The UI libraries also provide declarative shortcut components. A command's key binding and
availability can therefore live in the tree with the controls it affects, instead of requiring
a second, disconnected description of the interface.

**Things to decide:**

- Which component owns state and may change it?
- Who owns each event subscription?
- What stops an obsolete node from influencing state after removal?

Markup composition, reactive ownership, and resource cleanup belong to the same component design.

## Terminal and graphical applications

Yumi's terminal and graphical libraries share these language ideas while supplying different
rendering and input facilities. Terminal components include forms, lists, trees, tables,
editors, and dialogs. Graphical components provide windows, controls, layout, and drawing.

Sharing a declarative vocabulary does not imply that every control or platform feature is
identical. It means the reader can carry the same model of object trees, state, styles, and
events between application domains.

The broader library tour is in [Batteries included](batteries-included.md). The distinctive
language feature is the connection: the tree's structure, its changing values, and the code
that reacts to it all belong to Yumi.
