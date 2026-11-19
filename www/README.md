# The website

`www/build`, run from anywhere, builds the whole site into `.tmp/site`, which git ignores:

- the home page, its examples browser, `robots.txt` and the icon, from `www/home`.
  `www/home/README.md` says how it is made.
- the language reference, from `docs/ref`, into `.tmp/site/ref`. `www/reference/README.md` says
  how it is made.

Both show Brix code as rows with `www/rows`, which `www/rows/README.md` describes.

It needs only the toolchain that `.release.json` pins, and it starts from an empty `.tmp/site`
each time.

Every link in the site is relative, so the same files work under any path. The examples browser
loads each file as it is chosen, so look at the site over HTTP, as with
`python3 -m http.server --directory .tmp/site`, not by opening `index.html` as a file.
