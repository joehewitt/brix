# The website

`www/build`, run from anywhere, builds the whole site into `www/dist`, which git ignores:

- the home page and `robots.txt`, from `www/src/prelaunch`;
- the language reference, from `docs/ref`, into `www/dist/ref`. `www/src/reference/README.md`
  says how it is made.

It needs only the toolchain that `.release.json` pins. It builds into `.tmp/site` and replaces
`www/dist` only when everything built, so a failed build leaves the last one in place.

Every link in the site is relative, so the same files work under any path.
