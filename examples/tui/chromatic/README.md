# Chromatic

A pixel animation studio written entirely in Brix. It opens with **Lumen**, an eight-frame
bioluminescent jellyfish, ready to edit. The large checkerboard canvas edits one frame while an
independent preview plays the whole loop. A color palette, two editable layers, onion skins,
and a clickable thumbnail timeline surround the canvas.

From the repository root:

```sh
bin/brix examples/tui/chromatic/chromatic-app.bx -o .tmp/build/chromatic
```

Use a terminal with mouse reporting, Unicode, and RGB backgrounds. The minimum size is
**96 × 40**; **132 × 44** gives the preview room to show every pixel. Larger canvases scale up
automatically when there is room. The PNG exporter uses Brix's native graphics library.

Projects and exports go into `chromatic-output` in the working directory. Supply a different
folder as the program's first argument:

```sh
bin/brix examples/tui/chromatic/chromatic-app.bx -o .tmp/build/chromatic -- .tmp/my-sprites
```

If that folder contains `project.chromatic.json`, Chromatic opens it on launch. Otherwise it
starts with Lumen. Saving or exporting creates the folder. Loading a malformed project leaves
the current document intact. Quitting or reloading after an edit offers save, discard, or cancel.

## Making something

Click a palette swatch, then drag on the large canvas. Right-drag erases. Fast strokes fill in
between pointer events, and each stroke is one undo step. The mirror toggle reflects pencil and
eraser strokes across the canvas's vertical center. Flood fill affects a connected area on the
selected layer; the eyedropper samples the visible composite.

`N` inserts a blank frame after the selected one; `D` duplicates both layers. Click a thumbnail
or use `[` and `]` to select a frame. The timeline pages to follow the selection. Pink onion pixels
show the previous frame, teal pixels show the next, and violet marks their overlap. Onion skins
appear only through transparent pixels and never enter the exported image.

Select **Ink** or **Glow** to edit that layer; both can contain arbitrary artwork. Visibility
applies across the animation, and painting a hidden layer is blocked until it is shown. Use the
blank-frame command to start from scratch. Layer visibility,
frame operations, frame rate, and painting all support undo and redo, with 64 steps retained.
`CommandHistory` manages the snapshot entries and saved position: undoing or redoing back to the
saved state clears the unsaved indicator.

The preview uses a monotonic clock and runs independently of the editor. Selecting a frame while
playing leaves playback running; when paused, the preview follows the selected frame. The studio
holds 1–32 frames, each 24 × 20 pixels, with a fixed 16-entry palette including transparency and a
playback rate of 1–24 fps.

## Controls

| Key | Action |
| --- | --- |
| `B` / `E` / `F` / `I` | Pencil / eraser / fill / eyedropper |
| Mouse drag / right-drag | Paint / erase |
| Arrow keys, then `Enter` | Move the pixel cursor and apply the tool |
| `,` / `.` | Previous / next palette color |
| `O` / `G` / `M` | Onion skins / grid / mirrored strokes |
| `1` / `2` / `L` | Select Ink / Glow / toggle selected layer visibility |
| `[` / `]` | Previous / next frame |
| `N` / `D` / `Delete` | New blank frame / duplicate / delete |
| `C` | Clear the selected layer in the current frame |
| `Space` | Play or pause the preview |
| `-` / `+` | Lower / raise frame rate |
| `Z` / `Y` | Undo / redo |
| `S` / `R` | Save / reload project |
| `X` | Export PNGs, metadata, and the editable project |
| `?` / `Esc` | Open / close help |
| `Q` | Quit |

Letter shortcuts use lowercase; the UI capitalizes their labels for readability.

## Output

Export writes the following files into the chosen folder, replacing previous exports there:

- `sheet.png`: all frames in a single horizontal strip, with transparent backgrounds.
- `frame.png`: the selected frame at its native 24 × 20 resolution.
- `sheet.json`: frame dimensions, count, frame rate, horizontal layout, and looping metadata.
- `project.chromatic.json`: every palette index on both layers, their visibility, and frame rate.

PNG export composites only the visible layers. Project save retains hidden pixels too. The project
is written atomically; `S` saves only the project, while `X` also refreshes the PNGs and metadata.

## Source

The workspace uses `GroupPanel`, the footer uses `StatusBar` and active `Shortcut` metadata,
and help and unsaved-change prompts use `ModalDialog` with keyboard-focusable `DialogButton`
actions. Preview playback uses the TUI frame scheduler and stops requesting frames while paused
or covered by a dialog. The pixel canvas, preview, and timeline remain specialized drawing
components.

- `chromatic-app.bx`: declarative workspace, editing actions, history, playback, and file lifecycle.
- `chromatic-document.bx`: sprite model, project validation, flood fill, and the starter animation.
- `chromatic-paint.bx`: theme, RGB pixel canvas, onion skins, and half-block preview painters.
- `chromatic-export.bx`: transparent PNG compositing and export metadata.
