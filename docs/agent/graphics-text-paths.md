# Text outlines

Import `/graphics/text` to turn text into an editable `/graphics/path` `Path`:

```brix
(import /graphics/text)

(main
  (let lettering (call text-path "RESONANCE" (call Font "Avenir Next" 78 (. FontWeight heavy))))
  (print (. lettering bounding-box))
)
```

`text-path(text, font)` creates and closes a temporary text layout. The font defaults to
`system-font()`. Keep the returned path when drawing the same lettering repeatedly.

For styled text, wrapping, truncation, or line spacing, construct a `TextLayout` with those
settings and call `layout.to-path()` before closing it. Extraction uses the layout's shaped glyph
runs and their resolved fonts, preserving kerning, ligatures, combining marks, bidirectional text,
font fallback, and line baselines. Each line is left-aligned in layout coordinates: x increases
rightward and y downward from the layout's top. Outline extraction preserves those coordinates;
it does not shift the ink bounds to zero or clip contours to a drawing rectangle.

The returned path contains closed glyph contours. Quadratic font curves become equivalent cubic
curves, as elsewhere in `Path`. Use `path.outline(canvas)` to paint with the canvas's current fill
and stroke, or use the existing path transforms, clipping, and geometry operations. Glyph holes
retain their winding and work with the default nonzero fill rule.

Each call returns an independent mutable path that remains valid after the layout closes.
The native layout caches extracted contours until it closes. Calling `to-path()` on a closed
layout raises `TextLayoutRequestPanic`.

Empty text and whitespace produce empty paths. Inline objects and glyphs without vector contours,
such as bitmap-only emoji, contribute no contours; their layout advances still position the other
glyphs. A path contains geometry, so per-run colors and text decorations are not carried into it.

Resonance's `resonance-title.bx` uses `text-path` once, centers the resulting ink bounds, and
paints the path into a Metal layer. Its separate `LayerShader` distorts that rendered layer in
response to notes. Outline extraction does not require a raster canvas or an intermediate SVG.
