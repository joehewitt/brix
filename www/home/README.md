# The home page

`home/build.bx` writes the home page of brix-lang.com: the name, the byline, links to the
reference and the GitHub repository, the install command, a prompt that asks an agent to install
Brix and write an app, a note that the reference is written for agents, and a browser of
`examples`.

Run it from the repository root with
`bin/brix www/home/build.bx -o .tmp/build/build-home -- --output .tmp/site`. `--examples` names
another folder of examples.

## The examples browser

The left pane is a tree of every `.bx` and `.md` file below `examples`, folders first, each level
sorted by name. Names that start with a dot are left out. The right pane shows one file.

- Each Brix file is shown as rows, the way the editor draws it, with its forms beside them. The
  Source button switches between the two and the browser remembers the choice, which the reference
  shares.
- Each Markdown file is shown as rendered Markdown.
- The pane takes the editor's light or dark colors to match the page.
- The build writes every file's pane to `examples/<path>.html`, and the page loads one when its file
  is chosen. The page holds the pane of `home-first-example`, `basics/life.bx`, so it shows code
  before its script runs.
- The URL's fragment names the file shown, as in `#tui/todo.bx`, so a file can be linked. A link
  to one opens only that file's folders and scrolls to the browser.
- A file whose forms cannot be shown as rows fails the build, with its path.

On a wide screen the browser is one panel as tall as the window less the page's gutter above and
below it, and it stays in place once the page has scrolled to it. The tree and the file scroll
within it. A wheel over the panel scrolls the page until the panel is in place, then the pane under
the pointer. Below 760px the page is one column that scrolls as a whole, the code runs to the edges
of the screen, the file's bar stays at the top of the window, and a Files button in the bar opens
the tree as a sheet from the bottom. Without JavaScript the page shows the tree and the first file.

## Files

- `build.bx`: the builder, and the page's text.
- `style.css`: the page's style, after `rows/rows.css` in `home.css`.
- `script.js`: the copy buttons, the Source button and the examples browser.
- `icon.png` and `robots.txt`: copied as they are. Until launch, `robots.txt` and the page's
  `noindex` keep search engines out.
