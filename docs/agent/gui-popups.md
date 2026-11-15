# GUI Popups

A popup is an ordinary `Box` declared where it logically belongs. While open it is a presentation
layer: laid out, painted, and hit-tested by its `Window` instead of by its parent, and rendered into
its own native child panel. Styles, inherited properties, key bindings, and event routing still
follow the logical tree, so a popup inherits its owner's theme.

## Pieces

| Concern | Where | Notes |
| --- | --- | --- |
| Behavior | `stdlib/gui/popup.yumi` | `Popup`, `GUIPopupStyle`, `PopupAnchor`, `DismissReason` |
| Placement | `stdlib/gui/placement.yumi` | `Placement` subclasses place a popup from a `PlacementRequest` |
| Layer stack | `stdlib/gui/window.yumi` | `popups`, `modal-popup`, hit testing, light dismiss, focus |
| Native surface | `YuAppleGUI.mm` `yumiGuiPopup*` | borderless child `NSPanel` per open popup |
| Shapes | `stdlib/gui/shape.yumi`, `stdlib/graphics/outline.yumi` | `BoxShape`, `CalloutShape`, `Outline`, `Shadow` |
| Components | `menu`, `tooltip`, `popover`, `select`, `dialog`, `toast`, `command-palette` | built only on the public model |

`Box.is-presentation-layer` makes rendered-child traversal skip a popup, so base paint, hit testing,
layout, focus walks, and the base `DisplayScene` exclude it. Paint and content-size invalidation
stop at the layer. Each popup has its own `DisplayScene` with an `origin` offset.

## Shapes and shadows

`GUIBoxPaint` has `shape` and `shadow`, so any box can use them, popup or not.

- `shadow` is a `Shadow(color, blur, x, y, spread)`. Blur follows CSS: sigma is half the blur.
  It paints outside the shape only, so translucent boxes stay clean.
- `shape` is a `BoxShape`. A custom shape overrides only `path(node, rect, canvas)`. Fill,
  stroke, clip, and shadow come from that path, and path shapes draw through CoreGraphics. Override
  `outsets` if the path reaches past the bounds.
- `OutlineShape` describes its geometry as an `Outline` (rounded rect plus optional arrow), which
  every renderer draws natively. It is the default shape, built from `corner-radius`.
  `CalloutShape` adds an arrow. A box points it with `callout-side`/`callout-offset`; `Popup`
  answers from its placement.
- Canvases draw `Outline` and `Shadow` natively. The display recorder turns a covered fill or
  an inset stroke under an outline clip into a GPU item. Metal draws rounded rects, arrows,
  inner borders, and blurred shadows analytically in one quad each. Paths, path clips, and
  custom paint under a shape clip replay through CoreGraphics into a cached tile. The recorder
  tracks path bounds so path ink and shadows are in the tile and in repaint damage.
- `Box.ink-outsets` is how far arrows and shadows reach past the bounds. Popup surfaces grow by
  it, and anchored placement adds `BoxShape.anchor-gap` so arrows don't overlap anchors.
- Popups draw through Metal when their window does, with one `CAMetalLayer` per popup panel.

## Behavior styles

Popup behavior lives in `GUIPopupStyle`, so class defaults, themes, and instances can all set it:
`placement`, `modal`, `pointer-transparent`, `popup-stacking` (`auto`, `hint`, `manual`),
`popup-focus`, `outside-press-dismiss`, `outside-press-passes`, `escape-dismiss`,
`focus-loss-dismiss`, `deactivate-dismiss`, `anchor-hidden-dismiss`, `hover-trigger`,
`hover-delay`, and `context-trigger`. Shorthands include `below`, `above-center`, `beside`,
`at-pointer`, `follow-pointer`, `cover-anchor`, `centered`, `fill-window`, `corner`, `modal`, and
`passive`.

## Stacking rules

- A popup's parent popup is its nearest `Popup` ancestor in the logical tree.
- Opening an `auto` popup closes other non-`manual` popups that are not its ancestors and do not
  contain its anchor; `hint` popups only replace other hints.
- An outside press is a press outside the popup, its descendants, and its anchor box. Presses on
  native peers such as a `TextInput` count too, though the peer still receives them.
- Escape dismisses the topmost popup that allows it; closing a popup closes its descendants.
- Focus returns to the popup's return focus when focus was inside it, or when focus was empty and
  the popup closed by escape, choice, or program.
- `PopupFocus.first` targets the first box in the popup that accepts focus, even if it is not ready
  yet. A native `TextInput` is ready only once its control is shown in the popup's surface; until
  then the window holds the request as pending focus instead of skipping to a later box.

## Triggers

The window routes secondary or Control presses to the nearest `context-trigger` popup, and hover to
the nearest `hover-trigger` popup with warm-up, instant switching while warm, and a short grace on
exit. While an `auto` popup is open, hover popups appear only for boxes inside the topmost one. A
popup opened during a press is chosen by releasing over it after moving.

## Verification

`stdlib/gui/tests/popups.yumi` states anchored, point and cover placement, snapping above the
anchor, re-placement when the anchor scrolls, the modal boundary, and the popup stack.
`examples/gui/showcase/showcase-app.bx` is the interactive showcase; run it with `-- menus` to open
its popups.
