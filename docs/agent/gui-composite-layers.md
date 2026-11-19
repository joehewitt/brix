# GUI composite layers

Paint and hit order normally follow tree order: a later sibling draws over an earlier one and wins
the hit test. Composite layers change that order by name. There is no numeric z-index.

## Declaring layers

`Layer` is a non-rendering markup node. A box declares layers by containing `<Layer>` children.
Layers nest, and at most one should be marked `is-default`.

```text
<ListView
    <Layer "rows"
        is-default = true
    <Layer "indicators"
```

Every `Window` owns the root of the composite tree. It declares these layers, in this order:

```text
underlay
content          default
adornments
    focus
    handles
feedback
```

Layers a window declares in its own markup come after the built-in ones.

## Joining a layer

The non-inherited `layer` style moves a box, with its subtree, into a named layer. Names resolve
upward from the box's parent: the nearest ancestor that declares a layer with that name wins. An
unknown name leaves the box where it was.

```text
• Resizer
    layer = "handles"
```

A box without `layer` stays with its parent. The unassigned children of a box with a default layer
go into that layer. A box only counts as routed when its layer differs from the one it would have
inherited.

## Order

Order is depth-first through the composite tree. A declaring box paints its own background first,
then its layers in order, then its border. Its unassigned children paint in its default layer, or
first if it has no default. Boxes routed into a layer keep tree order among themselves. Boxes routed
into the default layer from deeper in the tree paint after the declarer's own children.

## Scopes

Layers stack inside scopes. A scope is the window, a presentation layer (such as a popup), a box
that clips or scrolls, a translucent or filtered box, or the root of a tree. A routed box never
escapes its nearest scope, so it stays clipped, scrolled, and filtered with its container. Inside the
scope it sits above or below the scope's own content, according to the global composite order. The
scope as a whole occupies one slot in its parent.

Painting or hit-testing a subtree directly, as `Box.snapshot` does, leaves out routed descendants
whose host lies outside that subtree.

## Events

Event routing ignores layers. A pointer event that hits a routed box bubbles through its tree
ancestors as usual.

## Implementation

`composite.bx` keeps a registry of mounted boxes that set `layer`. `composite-refresh` turns it
into a plan, and each entry point refreshes first: `hit-path`, `hit-path-within`, `Box.paint`, and
`DisplayScene` build and update. A refresh does real work only after something invalidated the plan.

- A routed box gets a host: either the box that declares its target layer, or a nearer scope.
- Each host keeps two lists, `below` and `above`. It paints `below`, then its children, then
  `above`, and hit-tests in the reverse order.
- Every `Box` has a flags byte. The `routed` bit makes normal child traversals skip a box. A box
  that takes part in compositing also gets a `CompositeBoxState` record holding its declared
  layers, host and member lists.
- Invalidation is local. A refresh marks every ancestor of a layered box `relevant` and records
  whether it is a scope and whether it is visible. The plan is invalidated only when one of these
  happens:
  - a layer is declared, renamed or removed;
  - a layered box mounts, unmounts or changes its `layer`;
  - a relevant box is attached or detached;
  - a relevant box's scope or visibility actually flips.
- A host always gets a `DisplayGroup`. When a routed box's display entries change, its host
  relinks. A plan whose content changed rebuilds the scene.
