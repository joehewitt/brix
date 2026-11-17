# The website

`www/build`, run from anywhere, builds the whole site into `.tmp/site`, which git ignores:

- the home page and `robots.txt`, from `www/prelaunch`;
- the language reference, from `docs/ref`, into `.tmp/site/ref`. `www/reference/README.md` says
  how it is made.

It needs only the toolchain that `.release.json` pins, and it starts from an empty `.tmp/site`
each time.

Every link in the site is relative, so the same files work under any path.
