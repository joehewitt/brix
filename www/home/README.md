# The home page

`home/build.bx` writes the home page of brix-lang.com: the name, the byline, the install command,
a prompt that asks an agent to install Brix and write an app, a link to the reference, and a
browser of `examples`.

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

On a wide screen the tree stays in view beside the file as the page scrolls, and the file's bar,
with its path and the Source button, stays at the top of the window. Below 760px the page is one
column, the code runs to the edges of the screen, and a Files button in the bar opens the tree as
a sheet from the bottom. Without JavaScript the page shows the tree and the first file.

## Files

- `build.bx`: the builder, and the page's text.
- `style.css`: the page's style, after `rows/rows.css` in `home.css`.
- `script.js`: the copy buttons, the Source button and the examples browser.
- `icon.png` and `robots.txt`: copied as they are. Until launch, `robots.txt` and the page's
  `noindex` keep search engines out.
