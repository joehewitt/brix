# Graphics Image Resources And Raster Cache

`VectorGeometryTable` is the renderer-owned resource table for both immutable vector geometry and
immutable raster pixels. Consumers hold small `(index U32, generation U32)` handles rather than
resource objects. Reusing a table slot advances its generation, so a stale handle cannot resolve a
replacement resource accidentally.

## Image resources

`ImageResource` owns a frozen `RasterCanvas` pixel buffer after construction and records its
pixel width, pixel height, and intended display scale. Ownership of the canvas transfers to the
resource table; producers must create a new resource instead of mutating or replacing those pixels.
Releasing the final handle closes the backend buffer and advances the slot generation. Image decode
and file IO are intentionally outside this layer.

CoreGraphics exposes one raster entry point, `brixGraphicsDrawImage`. The source canvas keeps a
lazy `CGImage` snapshot, invalidated by every pixel-mutating operation. Ordinary resources draw the
snapshot directly. The entry point also supports template-alpha resources by clipping through
their alpha and filling with the consumer's current color. Destination edges are rounded to
integral device pixels before lookup and drawing so premultiplied pixels are not resampled across
fractional offsets.

## CoreGraphics vector raster cache

`VectorGeometryTable.paint-vector-raster` observes draws keyed by:

- geometry handle and generation;
- snapped device width and height;
- backing scale and effective stroke width;
- palette mode, either template alpha or a baked-color palette fingerprint.

A matching key replays geometry for its first three draws and promotes an `ImageResource` on the
fourth. CoreGraphics vector entries are baked and keyed by palette, because template-alpha
clipping measured slower. Recoloring leaves existing entries valid and observes a separate palette
key.
Geometry with inherited paint, one-off keys, and entries larger than 1 MiB remain on replay.

Promoted entries are least-recently-used and share an 8 MiB RGBA byte budget. The observation list
is separately bounded so a stream of one-off sizes cannot grow without limit. Evicting a geometry
generation also removes its raster observations and releases their image handles. Atlas metrics
report only promoted images, not pre-promotion observations. Cache hits draw their already-fitted
destination before the replay-only canvas push and clip; fallback replay retains the original
state-isolated transform and clipping path.

## Metal

The Metal renderer (`libs/applegui/BxMetalRenderer.mm`) keeps its own atlases: four 1024-pixel
A8 pages for glyphs and two BGRA pages for images. Image entries are keyed by handle index and
generation, device size, and template-alpha mode, and are evicted by last use. Canvas images can
use their own textures instead, tiled when large. The renderer-facing vocabulary stays the
`(index, generation)` handle; texture packing and uploads stay inside the renderer.
