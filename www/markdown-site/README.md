# Markdown site builder

`site.yumi` is the reusable core for turning an indexed directory of Markdown documents into one HTML document.

The entry document defaults to `README.md`. Its Markdown links establish document order, and every Markdown file below the source root must appear in that index, except the files a site names as `unlisted`. Loading produces typed `MarkdownSite` and `MarkdownSiteDocument` values containing the source text and parsed Markdown tree.

`render-markdown-site-document` scopes every heading and local link to the document ID. Optional fenced-code, inline-code and link renderers let a site add domain-specific presentation without reparsing rendered HTML.

The language reference in `www/reference/build.yumi` is the complete example. It supplies chapter IDs, Brix code shown as rows, annotated Yumi source rendering, navigation, page chrome, static assets, atomic output writes, and recursive watch mode while leaving document discovery, ordering, Markdown rendering, and link scoping in the reusable module.

Run the reference builder once with:

```sh
bin/brix www/reference/build.yumi -o .tmp/build/build-reference
```

Keep it running and rebuild after Markdown changes with:

```sh
bin/brix www/reference/build.yumi -o .tmp/build/build-reference --watch
```

The builder also accepts `--source` and `--output`, which makes fixture builds and alternate copies of the guide independent of repository paths.
