# Amanuensis

Amanuensis is a static site generator. It is idiosyncratic and is best understood as a learning exercise. 

## Design

A "site" is a collection of `.md` files and attachments, all residing in a directory. The directory's structure may or may not be meaningful. The `.md` files can link to each other and to attachments.

The `.md` nature of the sources is chosen for its human-readability and -writability. HTML is difficult to read through the syntax of, and it requires a lot of extraneous keystrokes and cognitive load to write by hand.

The final site is flat. It has no directory structure, and its navigability is dependent on the placement of links. It should be imagined as a directed graph.

Within this graph, pages may reside in connected components. To bridge otherwise-isolated islands, an index page is generated that lists these connected components, serving as a sort of panopticon from which any of a set of isolated subgraphs may be entered. Pages which are better connected within their components produce higher contrast links, with outgoing links being given twice the weight of incoming links in this ranking.

(An interesting alternative to the monochromatic weighting of links on the index page would be a plot, with one axis being the indegree and the other being the outdegree.)

Pages are prepended with a navigation box that contains links to outgoing and incoming pages, arranged in five rows. The central row shows the name of the current page. Rows above the center contain links reachable by "backward paths" (pages that link to the current one, pages that link to those pages), and rows below contain links reachable by "forward paths." More distant pages are on the first and last row; adjacent to the center are immediately outgoing or incoming links.

## Methodology

Source files are written in my dialect of Markdown, called [Monkdown](https://github.com/chrischitren/monkdown), the purpose of which is to enable just enough expressivity to not feel frustrated when writing documents, but to always remain dysfunctional enough that the most convenient decision to make when composing a document is to explain oneself in natural language. Unfettered by styles, typefaces, whitespace, figures, colors... these are the domain of advertising. What I seek to produce is [communication](bandwidth_reciprocity.md). The project is mostly a didactic exercise.

Parsed as they are by Monkdown, documents arrive as a tree of `Node`s, structs containing content and metadata about the style of an element. `Node`s are intended to be isomorphic to HTML elements: they possess a type (the tag), content (the inner HTML), attributes (the same), parents, children, and so forth. A tree of `Node`s can be output as HTML with a simple recursive function that writes sequentially.

Most of the meat of Amanuensis is a pipeline that takes in a directory and spits out a blob of HTML files which are (hopefully) semantically interlinked in such a way as to give the illusion of place, when navigated as hypertext. This pipeline first reads the source directory to a `Filenode` tree. (This is an attempt at portability; hopefully, the `Filenode` struct is self-contained except for the function to build a tree by spelunking with system calls. A rewrite of this function which produces the same data structure is easier than a refactoring of disparate components which all happened to need to know something about the source directory.) Having retrieved our phone book, we can begin making calls, first to Monkdown to parse anything we hope is a source file. The `Node` trees we receive are placed into `Page` structs, which are the first things we've encountered that aren't arranged into a tree! A `Page` is a vertex in a directed graph. It contains lists of incoming and outgoing links as pointers to other `Page` structs.

Amanuensis builds a list of all the `Page`s so that it doesn't lose track of orphans and wind up with memory leaks. It removes these orphans anyway, since there is little point in using hypertext for a site without hyperlinks. All the remaining `Page`s are processed to include a navigation box, header, and footer. Using the list of pointers to `Page` vertices, we analyze the graph and split it into connected components. An index page is built with this information.


