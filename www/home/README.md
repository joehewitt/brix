# The home page

`home/build.bx` writes the home page of brix-lang.com: the name, the byline, a link to the GitHub
repository, the install command, a prompt that asks an agent to install Brix and write an app, a
note that links the reference and says it is written for agents, a paragraph on the basics, a cloud
of the language's features, and a browser of `examples`.

Run it from the repository root with
`bin/brix www/home/build.bx -o .tmp/build/build-home -- --output .tmp/site`. `--examples` names
another folder of examples.

## The feature cloud

Above the examples, a paragraph sums up what kind of language Brix is, and below it a cloud of
boxed terms names its headline features. Each term is one file in `features`, colored by its
category and sized by its weight. Clicking a term opens a card with a one-sentence summary, a
short program shown as rows, and links to the examples that use the feature most. The card floats
over the page below its term, or above it when it does not fit below and there is more room above,
and its body scrolls when neither side has room for all of it. The card's Source button is the same
switch as the examples browser's. An example link closes the card and opens that file in the
browser.

The page is near black, or near white in light mode, and its color comes from three pops, one
for each category, and a yellow for links and the file shown. A large term is filled with its
category's pop. While a card is open, the other terms turn gray.

A feature with no size is left out of the cloud, and an element with its name in `data-feature`
opens its card from elsewhere on the page. Its card leaves out the category, which only matches a
term to the legend. The paragraph's link on "one syntax for agents and another for humans" opens `two-syntaxes`
that way.

Below 480px, and on a touch screen below 760px, the card is a sheet from the bottom of the window.

A feature file starts with front matter, one `; key: value` line each, and the rest is the program:

- `name`: the term's text.
- `category`: `basics`, `front-end` or `systems`, which sets its color.
- `size`: `large`, `medium` or `small`, or left out to keep the feature out of the cloud.
- `summary`: one sentence of Markdown.
- `view`: `rows`, the default, or `both`, which shows the rows and the forms side by side, each
  labeled, in a wider card with no Source button. The sheet stacks them.
- `examples`: up to three paths below `examples`, separated by `, `.

The build fails on an unknown key, a missing name or summary, a category or size not in those lists,
or an example that is not in the browser. The cloud mixes the categories in proportion to their
counts. Within each category, the three sizes are mixed the same way, and a large term leads.

## The examples browser

The left pane is a tree of every `.bx` and `.md` file below `examples`, folders first, each level
sorted by name. Names that start with a dot are left out. The right pane shows one file.

- Each Brix file is shown as rows, the way the editor draws it, with its forms beside them. The
  pane has no header: its Source button floats at its top right, over the code, and switches
  between the two. The browser remembers the choice, which the reference shares. The tree marks
  the file shown.
- Each Markdown file is shown as rendered Markdown.
- The pane takes the editor's light or dark colors to match the page.
- The build writes every file's pane to `examples/<path>.html`, and the page loads one when its file
  is chosen. The page holds the pane of `home-first-example`, `basics/pipelines/sensor-to-screen.bx`,
  so it shows code before its script runs. It is the one file that takes a value from a channel
  through a grammar, a pipeline and a state machine to markup.
- The URL's fragment names the file shown, as in `#tui/todo.bx`, so a file can be linked. A link
  to one opens only that file's folders and scrolls to the browser.
- A file whose forms cannot be shown as rows fails the build, with its path.

The browser is one panel as tall as the window less the page's gutter above and below it, and it
stays in place once the page has scrolled to it. The tree and the file scroll within it. A wheel
over the panel scrolls the page until the panel is in place, then the pane under the pointer. Below
760px the header and the install boxes stack and the tree narrows, so a narrow desktop window keeps
both columns.

On a touch screen below 760px, and on any screen below 480px, the page is one column that scrolls
as a whole, the code runs to the edges of the screen, and a Files button beside the Source button
opens the tree as a sheet from the bottom. The two buttons stay at the top right of the window
while the file scrolls under them. Without JavaScript the page shows the tree and the first file.

## Files

- `build.bx`: the builder, and the page's text.
- `features.bx`: reads `features` and writes the cloud.
- `features`: one program for each feature.
- `style.css`: the page's style, after `rows/rows.css` in `home.css`.
- `script.js`: the copy buttons, the Source buttons, the feature cloud and the examples browser.
- `robots.txt`: copied as it is. It lets search engines read every page.
