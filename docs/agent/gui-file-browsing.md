# GUI File Browsing

`stdlib/gui/file-browsing` provides three views of a folder on disk and the model they share. Import
it with `/gui/file-browsing`; `/gui` does not re-export it.

## Pieces

| Concern | Where | Notes |
| --- | --- | --- |
| Model | `file-tree.bx` | `FileTree`, `FileNode`, `FileNameFilter`, `file-order` |
| Tree view | `file-tree-box.bx` | `FileTreeBox`, a `TreeBox` of the model |
| Column view | `file-column-tree.bx` | `FileColumnTree`, a `ColumnTree` with a detail pane |
| Palette | `file-palette.bx` | `FilePalette`, a `Palette` that browses one folder at a time |
| Find in files | `file-text-palette.bx` | `FileTextPalette`, a `Palette` of the lines under `folder` that match its query, found by `FileTextSearch` |
| Shared behavior | `file-view.bx` | `FileView`: rename, create, trash, move, context menu, shortcuts |
| Coordination | `file-session.bx` | `FileSession`: the current item and the events views publish |
| Rows and icons | `file-row.bx`, `file-icon.bx` | `FileRow`, `FileRenameInput`, `FileIcon`, `FileGlyph` |
| Detail pane | `file-info.bx` | `FileInfoPane`, the column view's detail |

## The model is passed in

A view never creates its `FileTree`. The application creates one with `use` and gives it to every
view that should show that folder. The app runs where main's statements end, and the `use` closes
the tree, which stops its watcher, once the app quits.

```brix
(let files (use (call FileTree (call Path root 0))))
(<> App
  (<> Window
    (<> FileTreeBox (• flex 1)
      (= files files)
    )
  )
)
```

Because several views can show one tree, everything a view asks the tree to keep is counted:

- `retain(node)` and `release(node)` hold a listed folder. A folder that changes on disk while
  nothing holds it is unloaded and listed again when it is next asked for. A folder something holds
  is listed again in place.
- `observe(node)` and `unobserve(node)` ask for `modified` events about one item. The tree publishes
  `modified` for an item as long as any view still observes it.

A view lets go of what it holds when it is unmounted, so removing one view never unloads a folder or
ends an observation another view still needs.

`FileTree` lists folders lazily through `children-of(node)`, keeps a node's identity across a rename
or a move by matching inodes, and publishes `changed` with the folders it listed again.
`hides-name` decides which names are left out. `find(path)` and `key-path(path)` resolve a path
under the root, and `contains(path)` says whether a path is under the root at all.
`file-entries(location, hides-name)` lists one folder as `FileEntry`s without loading it into a
tree; the palette uses it.

## Standalone or together

Each view works with only `files` set. A `FileView` has its own `FileSession` unless one is passed
in, so a single view reports through `view.session`:

```brix
(<> FileTreeBox (id: tree) (• flex 1)
  (= files files)
)
(for (on node (. tree session opened))
  (call open-document (. node path))
)
(for (on message (. tree session failed))
  (call (. toaster post) message)
)
```

Views that should act as one browser share a session as well as a tree:

```brix
(prop + session _ (:= (call FileSession)))

(<> FileTreeBox (• w 260)
  (= files files)
  (= session session)
)
(<> FileColumnTree (• flex 1)
  (= files files)
  (= session session)
)
```

- `session.current` is the selected item. A view sets it when the user selects a row, and every
  view highlights it whenever it changes, including a view mounted later. Setting it does not
  scroll or focus a view.
- `(<- (. session revealed) node)` makes `node` current and asks every view on the session to show
  it and take focus.
- `session.opened` publishes an item a view activated.
- `session.failed` publishes a message when a rename, create, move, or trash fails.
- `session.expanded` lists the expanded folders as paths relative to the root. `FileTreeBox`
  updates it and publishes `session.expansion-changed` when a folder opens or closes, and expands
  those folders again when it is mounted. An application that keeps a session across views keeps
  the open folders with it, and can save and restore the list.

`FilePalette` takes the tree for its starting folder and its `hides-name` filter. It lists each
folder itself and watches only the one it shows, so it loads nothing into the tree. It can browse
above the tree's root unless `is-confined` is set, which stops leaving, `..` and a leading `/` at
the root. It publishes `chosen` with a `FilePaletteItem`; the application decides what
choosing means. `examples/gui/file-browser/file-browser-panel.bx` turns a choice under the root
into `session.revealed` and reports the others.

## Marks

`marks` on any view maps a path relative to the tree root to a list of `FileMark`s. A row shows
its marks after the name, each a label in the mark's color. `FileNode.relative-path` gives the key:
names joined by "/", and empty for the root. Assign a new map to change the marks; the view does not
watch the map itself.

## Layout and styles

`FileTreeBox` is only the tree. Put it in a `Sidebar` and `SidebarSection` for sidebar chrome, as
the file browser example does. It scrolls itself when it has a bounded height and follows a
scrolling ancestor otherwise, like `TreeBox`.

Each module extends `GUIDefaultTheme` with its own rules. The style names a theme can target are
`file-icon` and `file-glyph-icon` on icons, `file-mark` on marks, the `file-info` family on the detail pane, and
`file-palette-chevron` in the palette. `FileColumnTree` sets `row-height` to 28 and gives the
detail pane a minimum width.
