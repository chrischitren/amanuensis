# Amanuensis

Amanuensis is an idiosyncratic static site generator. When conditions are favorable, it produces exactly what it is designed to produce.

## Overview

Amanuensis is, for better or worse, mostly one C file, [`pages.c`](src/pages.c). It parses source files written in [Monkdown](https://github.com/chrischitren/monkdown), my ascetic dialect of Markdown.

To build a site, Amanuensis searches a source directory for `.md` files and parses these with Monkdown. It determines what pages are to be written, analyzes the structure of links among these pages, and finds attachments requested by them. It resolves broken links by assuming that every file has a unique name and searching the source directory.

Outgoing and incoming links between pages naturally organize the site into a directed graph. Momentarily ignoring this directedness, pages which are in any way linked are considered adjacent, and Amanuensis divides the site into connected components. To ensure the connectedness of the whole site, an index page is generated that contains a link to every page.

For each page, its outgoing and incoming links are used to generate a navigation box. This nav box, a header, the main content of the page, and a footer are used to fill a [template file](config/_template.html) which is written to the output directory. Everything in [`config/etc`](config/etc) is copied to the output directory. Attachments that were requested by pages are copied to the output directory.

Following a successful build, a flat, monolithic output directory is constructed which can be navigated as hypertext.

## Layout

[`src`](src) contains the aforementioned `pages.c` which is essentially a sequence of commands. It also contains `fnodes.c` which defines some utilities for storing directory structures, searching for files, etc.

[`build`](build) contains the scripts [`compile.sh`](build/compile.sh), which is pretty self explanatory, and [`buildsite.sh`](build/buildsite.sh), which cleans up the output directory and executes the compiled binary to build the site.

[`config`](config) contains the template file [`_template.html`](config/_template.html). The data in this file is used, but the file itself is kept outside of the subdirectory [`config/etc`](config/etc), the contents of which are to be present in the final site.

[`input`](input) contains the input blob (`.md` files & attachments).

[`output`](output) contains the output blob (`.html` files & attachments).
