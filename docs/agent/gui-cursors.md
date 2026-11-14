# GUI cursors

`Box` exposes an inherited `cursor` style of type `CursorStyle`. The window resolves it from the
deepest hit Box. `automatic` preserves default platform behavior, including the native text cursor
over text inputs. Reactive style changes refresh the cursor without requiring mouse movement.

Vectors painted inside a canvas do not need individual Boxes. Hit-test the canvas-local pointer
position, map the result to a `CursorStyle`, and bind that value to the canvas Box's `cursor` style.
Use the same hit test for pointer actions so the cursor describes the action a click will perform.
Recompute after tool, selection, or geometry changes as well as pointer movement.

## Captured drags

`drag-cursor` is an optional, non-inherited `CursorStyle` style. It takes precedence while that Box
owns pointer capture, including outside the Box or window. Set it from the active gesture, and clear
it to `null` when canceling a gesture. A null value uses ordinary hover resolution. Capture release,
deactivation, and removal of the capture owner retire the override automatically. Hover cursor
resolution resumes afterward.

The style can change during capture, for example from `open-hand` during hover to `closed-hand`
during a drag. Directional transform roles include `resize-horizontal`, `resize-vertical`,
`resize-nwse`, `resize-nesw`, and `rotate`. `move` is a four-way arrow distinct from the hand cursors.

## Custom images

Create `CursorImage(canvas, hotspot)` from an open `RasterCanvas` and select it with
`CursorStyle.custom(image)`. Creation snapshots the drawing; subsequent canvas edits or closing the
source canvas do not change the cursor. Canvas dimensions and the hotspot use logical points, so a
canvas with scale 2 supplies twice the pixels at the same cursor size. The hotspot must be finite
and inside the image bounds; invalid values throw an error.

Cache cursor images and reuse them across pointer events. `CursorImage` implements `Closable` and
also registers reclamation cleanup. Close it when its owner is finished using it. Native views
retain an applied image until they replace it or close; a closed resource should not be reused.

## Modifiers

`Box.modifiers-changed` delivers `ModifiersChangedEvent` with pointer coordinates and modifier
flags, even when the mouse has not moved. It routes through the captured path during a drag, the
hovered path otherwise, or the focused event path when no pointer target is available. It does not
synthesize pointer movement.
