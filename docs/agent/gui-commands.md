# GUI commands, history, modes, and key bindings

`stdlib/ui` holds the command model every app shares, whatever its toolkit: the `Command`
protocol, command logs, history, undo targets, edit transactions and valued modes. `stdlib/gui`
adds the markup nodes, key bindings, menus and command palette, and its index re-exports `/ui`. The
pieces are independent: an application can use a closure based `KeyShortcut` by itself, or combine
commands, history, valued modes, a command palette, and persisted key mappings.

## Lightweight shortcuts

`KeyShortcut` runs a closure for a key. Use it when an action needs no command class or history.

```yumi
<KeyShortcut "Refresh"
    key = KeyCombo("Cmd+R")
    stable-key = "example.refresh"
    action = => do refresh()
```

Titled shortcuts are discoverable by a window's command palette. Set `discoverable = false` for
bindings that only make sense inside another control. `enabled`, `when`, `repeat`,
`allow-while-input`, and `allow-while-modal` use the same routing rules as other key bindings.

## Commands

Commands are markup-ready objects implementing `Command` (`stdlib/ui/command.bx`).
`CommandNode` (`stdlib/gui/command.bx`) supplies the live properties that controls and palettes read: `title`,
`description`, `enabled`, `discoverable`, `is-on`, `running`, `stable-key`, and
`unavailable-reason`. A command class inherits `CommandActionNode` and implements `execute`; the
shared `run` checks `enabled`, owns the `running` lifetime, and invokes `execute`.
`ActionCommand` runs a closure. Commands do not record history themselves: a document change is an
invocation recorded in a command log, described next.

## Command logs

`stdlib/ui/command-log.bx` holds the generic, editable record of what changed a document.

- `ParameterizedCommand'Args` is a command whose arguments are one immutable value: `kind`,
  `title(args)`, `sentence(args)` (the parts a person reads and edits), `execute(args)`,
  `encode(args)` and `decode(json)`, and optionally `replacing(args, key, value)` to change one
  parameter, `reverts` and `revert(args)` when it has an inverse, and `defaults` when it can run
  without being asked for arguments.
- `CommandLog'Step` is an ordered list of steps with ids. `perform` executes and records a step,
  `record` records one that already ran, and `insert`, `delete`, `move` and `edit` change the list.
  Every change goes into its `CommandHistory` as one undoable entry. Editing step k rewinds to just
  before k and executes forward: by running inverses when every later step reverts, and otherwise
  through its `CommandLogTarget`'s nearest checkpoint (`restore`). A step that fails while
  replaying is kept with its failure (`step-failure`) instead of stopping the replay. Live edits
  (`ValueChangePhase.began` and `.changed`) replay without recording; `.ended` records one entry.
- `InvocationLog` is a `CommandLog'Invocation` for `ParameterizedCommand`s:
  `log.perform(command, args)` and `log.load(command, json)`.
- `EditableLog` is the read and edit surface a view needs: `length`, `step-id`, `step-title`,
  `step-sentence`, `step-failure`, `edit-parameter(id, key, value, phase)` and `cancel-edit`.

Sentence parts (`CommandSentencePart`) carry typed parameters from
`stdlib/ui/command-parameters.bx`: numbers and points with a `ParameterUnit` (none, degrees,
percent; `display-value` applies its scale), text with optional `options`, references to ids, and
`CommandValueParameter` for app-defined values. A new value for one parameter is a
`CommandParameterValue` (number, point, text, or ids).

`InvocationCommand'Args` (`stdlib/gui/command.bx`) is a `CommandActionNode` that performs a parameterized command with its
`defaults` into a `log`. It is discoverable only when the command has defaults, so a command
palette never has to ask for arguments.

### Edit transactions

`EditTransaction'Step'State` (`stdlib/ui/edit-transaction.bx`) is a gesture that builds one step
while the pointer moves. `begin(label, log)` snapshots its `Snapshottable'State` document; `apply`
or `record` holds the live step; `update(key, value)` replaces a parameter as the gesture moves;
`replay` restores the snapshot and executes the live step again; `run` records it in the log with
the measured `work`, and `cancel` restores the snapshot. Subclasses hook `restored`, `changed`,
`ran` and `revert`.

## History

`CommandHistory` keeps one stack of titled `CommandHistoryEntry`s and a cursor. Recording after an
undo drops the redo entries. A failed undo or redo stays where it was, for retry. `limit` (100)
bounds the stack; `null` leaves it unbounded. `use history.group(title)` collects every entry recorded inside it into one undo
step, nested groups included, and still records what finished when the block raises. An entry recorded while
the history is undoing or redoing is ignored, so side effects of restoring a state cannot record
over the step being replayed.

An entry may implement `coalesce(next)`: when the next entry has the same title, arrives within
`coalescing` (one second; `null` for no time limit) and the run is still open, the history offers it
to the newest entry, which can absorb it. Undo, redo, `mark-saved`, `clear` and `seal()` end the
run, so a text editor seals when the caret moves and records typing untimed. Command logs coalesce repeated edits of one step, so nudging a
number with the arrow keys undoes in one step. `mark-saved` records the save point; `is-modified`
is false exactly at it, and becomes permanently true when the saved state can no longer be reached.
`undo-title`, `redo-title`, `is-modified` and `revision` are trackable fields.

## Undo routing

`UndoTarget` (`stdlib/ui/undo-target.bx`) is anything Undo and Redo can act on: `can-undo`,
`can-redo`, `undo-title`, `redo-title`, `undo` and `redo`. `CommandHistory` is one. A GUI box
offers one through `undo-target`. Cmd+Z and Cmd+Shift+Z go to the first target, from the focused box
outward (from the window itself when nothing has focus), that can undo or redo, before key bindings see them, and the Edit menu's Undo and Redo
items take their titles and enabled state from the same walk (`Window.undo-target-for`,
`Window.perform-undo`). A focused `TextInput` is the innermost target while its native field has
typing to undo; once it has none, Undo reaches the document. `CodeEditor` and `MarkdownEditor`
offer their session's history.

## History view

`HistoryView` (`stdlib/gui/history-view.bx`) shows an `EditableLog` as one row per step, its
sentence laid out with editors for its parameters: `NumberField`s for numbers (in display units)
and points, a `Select` for text with options, and text for the rest. A failed step shows its
failure. Editing a field calls `edit-parameter` with the field's phase, so dragging replays live and
releasing records one undo step. Bind `revision` to the log's revision so rows follow the log.

## Command scopes and key commands

`CommandScope` is an optional markup container. It packages related command declarations and key
bindings without becoming a history owner.

```yumi
_ DeleteCommand is CommandActionNode

+ editor Editor
+ title = editor.#delete-label
+ enabled = editor.#has-selection
+ unavailable-reason = "Select something to delete"
+ stable-key = "editor.delete"

────────────────────
+ do execute

editor.delete-selection()

━━━━━━━━━━━━━━━━━━━━
+ EditorCommands is CommandScope

+ editor Editor
+ delete-command DeleteCommand

────────────────────
<This
    <DeleteCommand delete-command
        editor = editor

    <KeyCommand
        key = .backspace
        stable-key = "editor.delete"
        command = delete-command.id
```

`KeyCommand` resolves the concrete command reference supplied by markup, rechecks its current
availability, and calls the same `run` entry point used by controls and the palette. Duplicate
active matches within one focus scope are consumed without invoking either target. A disabled local
match also consumes the key instead of falling through to an unrelated outer action.

## Valued modes

`Mode'T` has a persistent `value` and an `effective-value`. Temporary activations are independently
owned; the newest live activation wins, and releasing it reveals the next activation or the current
persistent value. `changed` reports persistent assignments, while `effective-changed` reports every
effective-value update caused by either a persistent assignment or a temporary activation.

```yumi
+ tool-mode := Mode'DrawingTool(DrawingTool.pen)
+ independent-handles := Mode'Bool(false)

<KeyMode'Bool
    title = "Independent handles"
    key = KeyCombo(.option)
    stable-key = "editor.independent-handles"
    mode = independent-handles
    value = true
    activation = KeyModeActivation.held
```

Held modes activate once per physical press. Repeat events do not add leases. Release, window
deactivation, unmounting, disabling, and remapping release only the activation owned by that
binding. `ModeAssignmentCommand'T` provides the command and palette form of a persistent mode
change.

`KeyModeActivation.tap-or-hold` activates a temporary lease immediately. Releasing before the
binding's `hold-threshold`, which defaults to 200 ms, commits the value persistently. Releasing
after the threshold closes the lease and restores the preceding effective mode. Forced release
during deactivation, unmounting, disabling, or remapping never commits the value.

## Keys

`Key`, `PointerModifiers` and `PointerButton` (`stdlib/ui/input.bx`) and `KeyCombo`
(`stdlib/ui/key-combo.bx`) are shared by both toolkits. A `KeyCombo` is modifiers plus a trigger:
a named `Key` or typed characters. `matches-key(key, characters, modifiers)` and
`matches-text(text)` are toolkit-neutral; each toolkit extends `KeyCombo` with `matches(event)` for
its own key events. The terminal decoder reports Ctrl+letter as the letter with `control` set and an
uppercase letter with `shift` set, so `KeyCombo("Ctrl+S")` and `KeyCombo("Shift+N")` match in both.
Terminals never report Cmd, so a Cmd combo never fires there.

## Key assignments

`KeyAssignment` (`stdlib/ui/key-assignment.bx`) is what gui `KeyBinding` and tui `Shortcut` share:
a key, a display title, a `stable-key`, an `enabled` flag, the `allow-while-input` and
`allow-while-modal` flags, and the default that a mapping profile restores.
`key-assignment-routable` is the shared routing rule. Outside a modal only `allow-while-modal`
assignments route, Tab never routes, and a key that the focused input owns routes only with
`allow-while-input`. Each toolkit decides which keys an input owns. The gui runs bindings before the
focused box, so an input owns every text and text-editing key. The terminal gives the focused box the
key first, so an input owns only printable keys.

Both toolkits' boxes are `KeyScopeNode`s, and `active-key-assignments` collects the live
assignments in priority order. It walks from the focused box outward and, at each box that is a key
scope (a `keys` or `inside` focus mode, or `press` in the gui), takes the assignments that box owns.
Unscoped assignments follow, those inside the modal before those outside it. An assignment's scope is
its nearest key-scope ancestor. A gui binding with `is-container-wide` skips `press` ancestors, so
one declared inside a button belongs to the container around the button. Each workbench pane holds
its content in an `inside` scope, so a binding declared in a pane acts only while focus is in that
pane, even when the control that declares it takes no focus.

A tui `Shortcut` runs either an `action` closure or a shared `command`. With a `command` it takes its
title and availability from the command, so one `Command` can back a gui control, a palette entry,
and a terminal shortcut.

## Mapping profiles

`KeyMapping`, `KeyMappingProfile`, and `apply-key-mapping-profile` live in `stdlib/ui/keymap.bx`
and act on any list of key assignments. Every persistently remappable `KeyShortcut`, `KeyCommand`,
`KeyMode`, or tui `Shortcut` has an explicit `stable-key`. A versioned `KeyMappingProfile` replaces keys or disables declarations without
serializing commands, modes, contexts, or execution records.

```yumi
. mappings [KeyMapping] = List'KeyMapping()
mappings += KeyMapping("editor.delete", KeyCombo("Cmd+Backspace"))
mappings += KeyMapping("editor.independent-handles", null, true)
. profile = KeyMappingProfile(mappings)
. encoded = profile as String
. loaded = load-key-mapping-profile(encoded)
apply-key-mapping-profile(window.registered-key-bindings, loaded)
```

Loading validates the profile version before applying it. Unknown stable keys remain in the loaded
profile and have no runtime target. Applying a replacement profile restores declarations that are
absent from it to their markup defaults. It first releases live held activations, so a mapping
change cannot leave a mode stuck. Persistence belongs to the application. `profile.json` and
`load-key-mapping-json` embed a profile inside a larger settings document.

## Palettes

`Palette'Item` in `stdlib/gui/search-palette.bx` is the search popup that the command palette and
other pickers are built on: a search field over a virtualized list of results, opened with `show()`.
It owns the typeahead, the highlight, and the keys. Up and Down move the highlight and stop at the
first and last result, Home and End jump to them, Page Up and Page Down move it a page, Enter or a
click chooses, and Escape or an outside press dismisses.
The search field keeps focus the whole time. It hands the keys in `takes-keys` to the palette instead
of moving its caret: Up and Down by default, and a subclass can add Left and Right. `TextInput.passes-keys`
is the underlying switch, and it covers Up, Down, Left, Right, and Return.

A palette gets its results in one of three ways:

- Set `source`, a function from the query to the items. `fuzzy-ranked(items, query, text-of)` in
  `stdlib/data/fuzzy.bx` ranks a list; `match-spans(positions)` turns a `FuzzyQuery`'s matched
  positions into `MatchLabel` highlights.
- Bind `results` in a subclass to a computed list that reads `#query`.
- Override `refresh()`, which runs when the query changes and when the palette opens, and assign
  `results` whenever they are ready. Use this for searches that finish later.

`key-builder` gives items a stable identity. When results change under a highlight the user moved,
the highlight stays on the same item. A new query highlights its first result. `select-key(key)`
highlights an item by its key, now or as soon as results that hold it arrive.

Fill the `row` slot to draw an item; `row.#item` is the item and `row.#is-highlighted` its state.
`prompt` is the search placeholder, `empty-text` shows when there are no results, and `notice`
shows a line under the results until the query changes. `scope` is a
label in front of the search field that says what is being searched, such as a folder's path. It
takes at most 60% of the field, so a long scope is cut short and there is still room to type.
`rewrite(text)` replaces the typed query, for a palette that consumes part of what was typed.

Choosing dismisses the palette and publishes `chosen`. Override `choose(item)` to do something else,
such as staying open for a second step.

A palette's size, position, and row height are styles, so a theme sets them:

```text
• FilePalette • w:498 h:338
    placement = CenteredPlacement(56)
    row-height = 25
```

`row-height` comes from `GUIVirtualRowStyle`. It is inherited, so a rule on any ancestor sets the row
height of the lists inside it. `TreeBox` and `ColumnTree` take their row height the same way.

`stdlib/gui/tests/search-palette.bx` compiles complete examples.
`stdlib/gui/file-browsing/file-palette.bx` is a palette that browses the disk one folder at a
time, starting at a file tree's root: it lists and filters the current folder, shows the end of that
folder's path as its `scope`, steps into a folder on Right, Tab, or a typed `/`, and steps out on
Left, or on Backspace when the query is empty, as far as `/`. It lists each folder itself and watches
only the one it shows, so it is not confined to the tree's root. It takes Left and Right from the
search field, so they never move the caret. `docs/agent/gui-file-browsing.md` covers it with the
other file views.

## Command palette

`CommandPalette` is a `Palette` over the window's discoverable commands. Every command it runs that
has a `stable-key` is remembered in its `recents` (`Recents`, newest first, bounded by `limit`).
With an empty query the palette lists only the remembered commands, in that order. Typing ranks every
command with the fuzzy matcher over its title and description, remembered commands first among
equals, and highlights the matched characters. An application that keeps recents elsewhere assigns
its own `Recents` and listens to its `changed` event.

Each row shows the title, the description (or, for a disabled command, its `unavailable-reason`),
and its key bindings as macOS symbols from `KeyBinding.symbol-label`, such as `⇧⌘P`.
`display-label` stays the textual form (`Cmd+Shift+P`) that key-mapping profiles store and parse.
Choosing a disabled command keeps the palette open and shows why as its `notice`.

A `CommandChooserNode` is a command whose result is a choice. Running it from the palette replaces
the command rows with its `choices` and shows its `prompt` as the search placeholder; choosing one calls `choose` and
dismisses the palette. Running it from a key binding or control opens its `palette` directly on the
choices. Backspace on an empty query returns to the command list, and Escape does too when the
choices were opened from that list. `choices` may fail; a failure shows an empty list.

The complete examples are compiled by `stdlib/ui/tests/commands.bx` and
`stdlib/gui/tests/shortcuts.bx`.
