# GUI window chrome

`Window.titlebar-visible` defaults to `true`. Set it to `false` to extend GUI content into the
macOS titlebar area and hide the title and titlebar background. The native close, minimize, and
zoom buttons remain available. The window's `title` is retained even when its visual label is hidden.
The `size` property continues to describe the drawable content size.

`Window.position` is an optional `Point` giving the top-left corner of the content area in screen
points, measured from the top-left of the primary display. It defaults to `null`, which leaves
placement to the native window. Setting it before or after the window opens moves the window; a
position that would leave the titlebar off every display centers the window instead. Moving or
resizing the native window updates `position`, so applications can persist it alongside `size`.

`Window.state-key` does that persistence. When the window's `App` has a `state` (an `AppState`
from `stdlib/ui/app-state.bx`), a window with a `state-key` restores its saved position, size and
zoom on open and saves them whenever they change, in the section `window:<state-key>`. `AppState` keeps
one versioned JSON file per app under `~/Library/Application Support/<app id>/`, saves after a
short debounce, and flushes when the app closes. Apps store their own sections with `read` and
`write`, each with a version; a section saved with another version reads as `null`.

`Resizer.state-key` (and `Sidebar.state-key`, which passes it to the sidebar's resizer) saves the
size the user drags to in the section `resizer:<state-key>` when the drag ends, and restores it
when the resizer is mounted.

`Window.stoplight-offset` is an optional `Point`, defaulting to `null`. It translates all three
native window buttons from their native positions without changing their spacing. Coordinates are
logical points: positive X moves right and positive Y moves down. Set it back to `null` to restore
native placement. The offset works with either titlebar setting, and both properties can change
while a window is open.

Illustrative window properties:

```text
titlebar-visible = false
stoplight-offset = Point(12, 10)
```

`window-stoplight-center` is the buttons' native vertical center, so an offset of
`row-height / 2 - window-stoplight-center` centers them on a title row of that height.
`window-stoplight-offset` suits a row of about 42 points.

Reserve space for the buttons in application content. The offset does not add layout padding.
Keep the buttons inside the window bounds so they remain usable.
In fullscreen, AppKit controls button placement; the configured offset resumes upon returning to
windowed mode.

The backend retains the existing native buttons and their actions. An overlay handles positioning
and passes input outside the buttons through to the underlying views. Clearing the offset or
entering fullscreen returns the buttons to their native parents. AppKit moves the buttons back into
its titlebar when it retiles it, such as after a title change; the overlay takes them back at once
for titles set through `Window.title`, and on the next main-queue turn otherwise, keeping the native
frames it recorded when it first adopted them.

## Interface zoom

`Window.zoom` scales everything in the window the way a browser's page zoom does. It defaults to
`1`. The window lays out at `viewport`, which is `size` divided by the zoom, and paints at
`backing-scale`, the display's scale times the zoom, so text and vectors stay sharp. Pointer, scroll,
touch and drop coordinates arrive in layout units. Native peers such as `TextInput` and `WebView`,
popups, drag images and the text-input caret are placed at their layout frames times the zoom, and
native text fields use a font scaled by it. A zoom of zero or less behaves as `1`.

`zoom-in`, `zoom-out` and `zoom-to-actual-size` step through `window-zoom-levels` (50% to 200%).
Users get them only in windows that opt in with `allows-zoom = true`; it defaults to `false`, so
apps whose canvases bind those keys keep them. An opted-in window zooms on ⌘= (or ⌘+), ⌘- and ⌘0
when no key binding on the focus path or in the window claims the key first, and saves its zoom
with its `state-key`. Put `MenuRole.zoom-in`, `MenuRole.zoom-out` and `MenuRole.actual-size` items in
the app's View menu to list them; they are disabled in windows that have not opted in, and the
native menu leaves their keys to the window so focused controls still win. Setting `zoom` directly
works either way.

The traffic lights keep their size and horizontal place under zoom, while their vertical center
follows it so they stay on the same content row. `Window.stoplight-inset` is
`window-stoplight-inset` in the window's layout units, so content that starts there clears the
buttons at any zoom; `TabBar` uses it when `is-beside-stoplights`. Read it from a component's own
`• This` style block (`padding-left = #stoplight-inset`) or from markup; module-level theme rules
cannot read it.

`SurfaceWindow` inherits the same zoom. Its `size` is the host's requested size, and a toy host can
treat `zoom` as a camera scale over `viewport`.

## Draggable content

`Box.window-draggable` is an inherited style, defaulting to `false`. Set it to `true` on a header
or other content region to let a primary mouse press initiate native window dragging. Decorative
descendants inherit the region. Set it to `false` on custom interactive controls or subregions to
exclude them. `Slider`, `TextInput`, `ListBox`, `ListItem`, and `ColumnTree` opt out by default.
Draggable content works with either titlebar setting.

Illustrative component styles:

```text
• Header
    window-draggable = true

• HeaderButton
    window-draggable = false
```

Normal hit testing applies, including clipping, stacking, modal boundaries, and native controls.
Pointer-down handlers run first and can cancel the default drag action. Accepted window drags
clear managed capture and pressed state and do not produce a GUI click. Rejected native requests
retain ordinary pointer behavior. Double-click titlebar actions are not added by this style.

The native callback retains its original mouse-down event and supplies a token valid only for
that callback and window. Release, replacement by another press, deactivation, and closure
invalidate the request. AppKit receives the retained event after managed event handling and
capture cleanup finish; this does not depend on whichever event AppKit is currently processing.
