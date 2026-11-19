# Amanuensis

Amanuensis is an idiosyncratic static site generator. When conditions are favorable, it produces exactly what it is designed to produce.

## Layout

Amanuensis is, for better or worse, mostly one C file, [`pages.c`](amanuensis/pages.c). It parses source files written in [Monkdown](https://github.com/chrischitren/monkdown), my ascetic dialect of Markdown.

To build a site, Amanuensis searches a source directory for `.md` files and parses these with Monkdown. It determines what pages are to be written, analyzes the structure of links among these pages, and finds attachments requested by them. It resolves broken links by assuming that every file has a unique name and searching the source directory.

Outgoing and incoming links between pages naturally organize the site into a directed graph. Momentarily ignoring this directedness, pages which are in any way linked are considered adjacent, and Amanuensis divides the site into connected components. To ensure the connectedness of the whole site without enforcing any strict hierarchy [Note 1], an index page is generated that lists these components.

For each page, its outgoing and incoming links are used to generate a navigation box. This nav box, a header, the main content of the page, and a footer are used to fill a [template file](config/_template.html) which is written to the output directory. Everything in [`config/etc`](config/etc) is copied to the output directory. Attachments that were requested by pages are copied to the output directory.

Following a successful build, a flat, monolithic output directory is constructed which can be navigated as hypertext.
