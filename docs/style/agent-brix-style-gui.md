# Brix GUI Style Rules For Agents

Use these rules to review working code in `stdlib/gui`, `examples/gui`, and other graphical UIs.
Cite violations by rule ID, such as `YG-03`. These are style rules, not syntax requirements;
justified exceptions are allowed when the rule's tradeoff does not fit the code.

## YG-01: Keep GUI Structure Declarative

Build applications from `(<> App …)`, `(<> Window …)`, controls, markup nodes, style properties,
tracked reads, and reactive streams. Do not replace the GUI tree with direct native-window
construction or manual child management unless the native host itself is the subject.

Incorrect:

```brix
(let window (call NativeWindow "Editor" (tuple 900 600)))
(call (. window add-label) "Ready")
(call (. window show))
```

Correct:

```brix
(<> App
  (<> Window
    (= title "Editor")
    (= size (tuple 900 600))
    (<> Label (text: "Ready"))
  )
)
```

An `App` runs by itself where the main program's statements end, before the values main uses are
closed, so a resource the windows read is opened with `use` in main. Call `app.run()` only where
code has to wait for the app to quit, such as in a function whose own `use` closes what the windows
read when the function returns.

## YG-02: Express Visual States In Style Rules

Use style sheets and tracked state conditions for hover, press, focus, selection, and other visual
states. Do not use event handlers merely to copy interaction state into paint properties.

Incorrect:

```brix
(for (on event focus)
  (= background-color lightblue)
)
(for (on event blur)
  (= background-color white)
)
```

Correct:

```brix
(rule action-button
  (= background-color white)
  (when-style #is-hovered
    (= background-color lightblue)
  )
  (when-style #is-pressed
    (= background-color blue)
  )
)
```

Animate those changes with `motion` declarations in the same rules, not with timers or code that
steps values. `motion` covers every property that only repaints or re-composites,
`motion.<property>` sets one property's motion, and the motion of the state being entered runs. Use
the theme's `gui-motion-quick`, `gui-motion-smooth` and `gui-motion-bouncy`, so reduce motion
applies.

```brix
(rule action-button
  (= motion gui-motion-quick)
  (when-style #is-pressed
    (= background-color blue)
    (= (. motion background-color) instant)
  )
)
```

A value that code computes moves with `animated(target, motion)`, written in the markup attribute
that shows it. State that steps every frame loops `(for (on tick frames) …)` in a state that runs
only while it moves. Neither uses a `Timer`.

```brix
(<> Label (• test-count)
  (= text (str (as (call round (call animated #passed gui-motion-smooth)) Int) " passed"))
)
```

Motion that plays on its own, such as a spinner, a shake or a pulse, is a keyframe animation that a
theme declares and a rule names with `(= animation …)`. Use the theme's `spin`,
`(shape shake distance)`, `pop-in` and `breathe` before declaring another, and declare it in the
theme so reduce motion can restate it. Code plays one with `(call play (shape $shake 4))` and can
`wait` for it.

```brix
(keyframes pulse animation 1200ms
  (= repeat (. Repeat forever))
  (track opacity
    (at 0% 1%%)
    (at 50% 0.2%% (:easing (. Easing ease-in-out)))
    (at 100% 1%% (:easing (. Easing ease-in-out)))
  )
)
(rule find-bar
  (when-style #is-failing
    (rule find-field
      (= animation (shape shake 3))
    )
  )
)
```

A node that appears or goes away animates through `#is-entering` and `#is-leaving` rules. Do not
delay its removal in code. Remove it with `if` or a keyed `for`, or hide it with `display`, and it
stays until its leave has played. The `#is-entering` rule gives the values it starts from and the
`#is-leaving` rule the values it leaves toward. `(= enter-space (. EnterSpace expand))` and
`(= leave-space (. LeaveSpace collapse))` move its siblings with it under `motion.size`.
`motion-stagger` on a container staggers the children that enter together. A popup's
`#anchor-origin` is its transform origin.

```brix
(rule toast
  (= motion gui-motion-smooth)
  (= leave-space (. LeaveSpace collapse))
  (when-style #is-entering
    (= opacity 0%%)
    (= translate (call Point 32 0))
  )
  (when-style #is-leaving
    (= opacity 0%%)
    (= scale 0.96)
  )
)
```

A node that layout moves or resizes animates there under `motion.position`, `motion.size`, or
`motion.layout` for both. Do not step `max-height`, measured heights or offsets in code to imitate
it. `resize-content` says what the content does while the size moves: `reveal`, the default, clips
it to the moving frame, `reflow` lays it out at each size, and `scale` stretches it.
`(= stack-motion (. StackMotion coupled))` on a container keeps its children touching while one of
them grows or shrinks. `(= scroll-anchor (. ScrollAnchor end))` keeps a scroller at its end as its
content changes, and `ScrollAnchor.content` keeps the rows in view still.

```brix
(rule chat-stage (• flex-col)
  (= justify (. Justify end))
  (= scroll-anchor (. ScrollAnchor end))
  (= stack-motion (. StackMotion coupled))
  (rule chat-turn
    (= (. motion size) gui-motion-smooth)
    (= leave-space (. LeaveSpace collapse))
  )
)
```

To move one change that the elements don't declare motion for, write it inside
`(do-use (call motion gui-motion-smooth) …)`. An element's own motion still wins, and
`(call motion instant)` snaps everything the writes move. Writes made while handling a pointer drag
or a `frames` tick are already instant. Under `instant`, nodes still enter and leave with their
`#is-entering` and `#is-leaving` rules. To swap what a view shows with no motion at all, as when
switching projects, make the writes inside `(do-use gui-still-cause …)`.

```brix
(do-use (call motion gui-motion-smooth)
  (= (. files-sidebar is-collapsed) true)
  (= (. dock is-shown) false)
)
```

When one element should travel from where another was, as a thumbnail opens into a detail view, give
both the same `match` key. If one goes and the other arrives in the same frame, the new one flies
from the old one's place under its `motion.position` and `motion.size`, above any scroller or clip,
and the old one hides. `(= match-content (. MatchContent crossfade))` on the old one fades its
content out during the flight. Do not measure frames or fly a copy by hand. A key read from tracked
state is an assignment, since shorthand arguments are read once.

```brix
(rule photo-card
  (= (. motion layout) gui-motion-smooth)
  (= match-content (. MatchContent crossfade))
)

(<> PhotoCard (• photo-card)
  (= match (str "photo-" (. #photo id)))
)
```

To reorder by dragging, carry the item: start the drag with `event.carry(payload)` instead of
`event.begin`. The item itself follows the pointer above clips, `#is-carried` styles it, and a flex
or flow parent slides its siblings apart for the slot under the pointer while a drop target
accepts. On drop, move the model to the item's `carry-slot`, its index among the other children.
It then lands there at the pointer's speed under its `motion.position`. Do not open gaps with
margins, hide the item, or work out drop indexes by hand.

```brix
(rule queue-row
  (= (. motion position) gui-motion-quick)
  (when-style #is-carried
    (= shadow gui-lifted-shadow)
  )
)

(for (on event drag-start)
  (let ignored (call (. event carry) (call QueueDrag this)))
)
(for (on event drop)
  (let row (. (as (. event payload) QueueDrag) row))
  (let index (. row carry-slot))
  (if (!= index null)
    (call (. queue move) (. row item id) index)
  )
)
```

To tie an animation to scrolling or to a value instead of time, give its element a `timeline`.
`(call scroll-timeline (.. 0 80))` plays it over the first 80 points of the nearest scroller's
offset, or of the scroller passed after the range. `(call progress-timeline #amount)` plays it at a
fraction from 0 to 1 and follows tracked state when assigned in markup. Do not watch scrolling or
step opacity, transforms or sizes in code to follow a position.

```brix
(rule title-divider
  (= animation fade-in)
  (= timeline (call scroll-timeline (.. 0 12)))
)

(<> Sheet (• sheet)
  (= animation sheet-open)
  (= timeline (call progress-timeline #open-amount))
)
```

## YG-03: Use Semantic Controls For Ordinary Interaction

Use `Button`, `Slider`, `TextInput`, and other controls for behavior they already own. Handle their
semantic events instead of rebuilding focus, press, drag, keyboard, or native-input behavior with a
plain `Box`. Raw pointer and key events are appropriate for genuinely custom direct manipulation.

Incorrect:

```brix
(<> Box (• action-button)
  (for (on event pointer-down)
    (= is-pressed true)
  )
  (for (on event pointer-up)
    (call save)
    (= is-pressed false)
  )
)
```

Correct:

```brix
(<> Button (text: "Save") (• action-button)
  (for (on event click)
    (call save)
    (call (. event cancel))
  )
)
```

## YG-04: Preserve Component Ownership

Keep selection, activation, focus policy, and event propagation with the component that owns the
behavior. Leaf rows and status views should render their inputs and delegate actions rather than
reach through the tree to mutate a window or application singleton.

Incorrect:

```brix
(<> ResultRow
  (= result result)
  (for (on event click)
    (= (. app main-window selected-result) result)
    (call (. app main-window open-preview))
  )
)
```

Correct:

```brix
(<> ResultsPanel
  (for (in result #results (keyed-by (. result id)))
    (<> ResultRow
      (= result result)
      (for (on event click)
        (call select-result result)
        (call (. event cancel))
      )
    )
  )
)
```

## YG-05: Keep Layout And Scrolling In The Box System

Express ordinary sizing, flow, positioning, clipping, and scrolling through box properties and
style shorthands. Do not manually arrange child bounds or translate paint coordinates to imitate
layout or scrolling. Custom layout and painting belong in the relevant stdlib subsystem.

Incorrect:

```brix
(for (in child children)
  (call (. child arrange-bounds) (call next-row-bounds))
)
(call (. canvas translate) 0 (- scroll-offset))
```

Correct:

```brix
(<> Box (• flex-row) (• flex 1)
  (<> Box (• viewport) (• flex-col) (• flex 1)
    (for (in row #rows (keyed-by (. row id)))
      (<> ResultRow (value: row))
    )
  )
)
```

## YG-06: Keep Custom Painting Behind The Paint Boundary

Use ordinary nodes and style properties for standard visuals. When a component genuinely owns
custom drawing, implement it in `paint-self`, work in content bounds, and preserve canvas state.
Application event handlers must not draw directly into the window canvas.

Incorrect:

```brix
(for (on event value-changed)
  (= (. window canvas fill-color) (. event color))
  (call (. window canvas circle) 20 20 10)
)
```

Correct:

```brix
(type + ColorSwatch (is Box)
  (fn-do + paint-self _
    (param canvas Canvas)
    (call (. canvas push))
    (let content content-bounds)
    (call (. canvas translate) (. content x) (. content y))
    (= (. canvas fill-color) $color)
    (call (. canvas circle) (/ width 2) (/ height 2) (/ (call min width height) 2))
    (call (. canvas pop))
  )
)
```

## YG-07: Preserve Renderer And Native-Peer Boundaries

Keep retained display-scene state, native peer ownership, invalidation, and frame phases inside the
renderer and window infrastructure. Preserve packed bulk submission across the native boundary; do
not move retained state into application nodes or replace bulk encoding with per-item FFI calls.

Incorrect:

```text
Change: cache the display-scene root on every application Box
Change: submit each painted item through a separate native call
```

Correct:

```text
Change: keep DisplayScene renderer-owned and invalidate it through Window
Change: preserve packed scene encoding and bulk native submission
```

Work inside this boundary changes the `gui` library itself. In the Brix repo, where that work
happens, read `docs/agent/gui-display-scene.md` before changing the scene's representation or frame
phases.

## Related Rules

For markup nodes, style sheets, style shorthands, and reactive markup shape, read
`docs/style/agent-brix-style-markup.md`. To see what an app did or where its frame time goes, read
`docs/style/agent-brix-probes.md`.
