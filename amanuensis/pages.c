#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "../monkdown/nodes.h"
#include "../monkdown/parse.h"
#include "../monkdown/write.h"
#include "fnodes.h"

#define MAX_PAGES 256
#define MAX_URLSZ 256

#define TITLE_PREFIX "chrcht"

typedef struct page Page;
struct page {
	Filenode *fnode;
	char outfilename[MAX_URLSZ];
	char displayname[MAX_URLSZ];

	Node *nodeheader;
	Node *nodenav;
	Node *nodemain;
	Node *nodefooter;
	
	Page *outlinks[MAX_PAGES];
	Page *backlinks[MAX_PAGES];
};

enum linktypes { LINKTYPE_WEB = 0, LINKTYPE_PAGE,
				LINKTYPE_FRAGMENT, LINKTYPE_OTHER };

/* WRITING */
int writepagehtml(Page *pg, char *writedir, char *templatedir);
int copyfile(char *dest, char *src);

/* INDEX PAGE: SUBGRAPHS */
Page *createindexpage(Page *pglist[MAX_PAGES], int *warnings);

/* PAGE AMENDMENTS */
int makepageheader (Page *pg);
int makepagenav(Page *pg);
int getpagesubgraph(Page *subgraph[MAX_PAGES], int d[MAX_PAGES], Page *pg);
int makepagefooter(Page *pg);

/* PAGE HANDLING FUNCTIONS */
Page *pageinit(Filenode *fnode);
Page *pageinitempty(); 
void pagefree(Page *pg);
void pageprint(Page *pg);

/* HELPER FUNCTIONS */
int guesslink(char *linkstr);

/* DEPRECATED MISCELLANEA */
void getcleannodetext(char *dest, int destsz, Node *n, int toplevel);
void nodeprintlinks(Node *n);


/* A Page is a collection of the things required to write a page (an HTML
   document) in situ. Thus it must possess information about links, backlinks,
   where the file it references is, etc. */

/* A Node is a syntax tree describing the formatting of text and objects in a
   document. It does not contain metadata about the document. */

/* A Filenode is an element of a tree whose structure is representative of an
   existing or hypothetical directory structure. It knows nothing of Nodes or
   Pages; it is an interface to the filesystem. */

/* The default behavior for resolving broken links should be to search for the
   *name* given in the offending file. This should be robust enough that if we
   want to lean on it as a crutch forever, giving everything a unique name and
   only ever referring to these names, we should be able to without issue. */

/* To preserve link integrity, duplicate page names are not tolerated, and no
   reference is made to the directory structure in the output folder. All
   apparent structuring of the site is illusory and for the benefit of a user
   navigating the site alone. As a consequence, internal links do not have to
   be well-formed, nor do they have to be repaired if the directory structure
   is altered. */

/* Good software depends on good specification. The implementation details of
   how character arrays are manipulated is always uncertain and should remain
   up to the discretion of someone who knows the hardware well enough to write
   efficient, reliable machine code. The deciding factor for portability and
   reliability is that the *behaviors* of routines like strcpy and strrchr, as
   well as the behaviors of any routines they depend on, are well-defined, and
   that any implementation of such standard routines exactly replicates this
   behavior. */



int main() {
	char *sourcedir = "../../misc_writings";
	char *outputdir = "../output";
	char *configdir = "../config";

	Filenode *fnode_sources;
	Filenode *fnode_config;
	
	int i, j, k;
	Filenode *fnodelistmd[MAX_PAGES];
	Page *pagelist[MAX_PAGES];

	Filenode *fnodelistother[MAX_PAGES];

	Node *nptr;
	Page *pgptr;
	Filenode *fnptrbuf[MAX_PAGES];
	Filenode *fnptr;

	Page *indexpage;

	char buf[256];
	char buf2[256];
	char *p;
	int exactmatchfound = 0;

	int warnings = 0;
	int orphans  = 0;
	int deadends = 0;
	int isolates = 0;
	int numpages = 0;


/*	char *outdir = "../output";*/

	/* Initialize empty lists of Pages and Filenodes */
	for (i = 0; i < MAX_PAGES; i++) {
		fnodelistmd[i] = NULL;
		pagelist[i] = NULL;
		fnodelistother[i] = NULL;
	}
	
	/* Open the "config" directory */
	fnode_config = fnodeinit(NULL, configdir, FTYPE_DIR);
	fnodebuildtree(fnode_config, fnode_config->path);
	fnodeprint(fnode_config, 0);

	printf("\n");
	
	/* Open the source directory */
	fnode_sources = fnodeinit(NULL, sourcedir, FTYPE_DIR);
	fnodebuildtree(fnode_sources, fnode_sources->path);
	fnodeprint(fnode_sources, 0);
	
	printf("\n");
	
	/* Search the source directory for .md files */
	printf("Found %d results for \"%s\" in \"%s\"\n",
	fnodesearch(fnode_sources, ".md", fnodelistmd, MAX_PAGES), ".md",
				fnode_sources->path);

	printf("\n");
		
	/* Make a Page for each source file */
	for (i = 0; i < MAX_PAGES; i++) {
		if (fnodelistmd[i] != NULL
				&& *(strstr(fnodelistmd[i]->path, ".md")+3) == '\0') {
		/*	
			printf("Parsing source file \"%s\"\n", fnodelistmd[i]->path);
			fnoderelativepath(buf, 256, fnodelistmd[i], fnode_sources);
			printf("(relative path: \"\x1b[32m%s\x1b[0m/%s\")\n", outputdir, buf);
		*/	
			fnoderelativepath(buf, 256, fnodelistmd[i], fnode_sources);
			printf("Parsing source file \"%s\"\n", buf);
			
			if ((pagelist[i] = pageinit(fnodelistmd[i])) == NULL) {
				printf("\x1b[31m[pages] "
						"received NULL Page for path %s\x1b[0m\n", 
						fnodelistmd[i]->path);
			}
			
			numpages++;
			/*printf("\n");*/
		}
	}
	
	printf("\n");
	
	/* Get the links in every page. We need to store them... */
	printf("\x1b[4mProcessing links:\x1b[24m\n");	
	for (i = 0; i < MAX_PAGES; i++) {
		if (pagelist[i] == NULL) {
			continue;
		}
		
/*		pageprint(pagelist[i]);*/
		strcpy(buf, pagelist[i]->fnode->name);
		if ((p = strrchr(buf, '/')) != NULL) {
			memmove(buf, p+1, strlen(buf)+1);
		}
		if ((p = strrchr(buf, '.')) != NULL) {
			*p = '\0';
		}
	
		nptr = pagelist[i]->nodemain;
	
		if (nextnodewithattributekey(nptr, "href") == NULL
			&& nextnodewithattributekey(nptr, "src") == NULL) {
		} else {
			printf("Page \"%s\" at \"%s\" \n", buf,	pagelist[i]->fnode->path);
		}
		
		/* Find all the hrefs */
		while ((nptr = nextnodewithattributekey(nptr, "href")) != NULL) {
			strcpy(buf, getattributebykey(nptr, "href")->value);
			if (guesslink(buf) == LINKTYPE_PAGE) {
				if ((p = strrchr(buf, '/')) != NULL) {
					memmove(buf, p+1, strlen(buf)+1);
				}
				if ((p = strrchr(buf, '.')) != NULL) {
					*p = '\0';
				}
				printf("    Link to \"%s\", searching for page \"%s\"\n",
						getattributebykey(nptr, "href")->value, buf);
				pgptr = NULL;
				exactmatchfound = 0;
				/* Search all the Pages for one with name containing the link
				   referent's filename, stopping if we find an exact match. */
				for (j = 0; j < MAX_PAGES; j++) {
					if (i == j || pagelist[j] == NULL) {
						continue;
					}
					strcpy(buf2, pagelist[j]->fnode->name);
					if ((p = strrchr(buf2, '.')) != NULL) {
						*p = '\0';
					}
					if (strstr(buf2, buf) != NULL) {
						if (strcmp(buf2, buf) == 0) {
							printf("      %c\x1b[32m page at "
									"%s (\"%s\")\x1b[0m\n",
								(exactmatchfound==0) ? '>' : ' ',
								pagelist[j]->fnode->path, buf2);
							if (exactmatchfound == 1) {
								printf("        \x1b[31mWARNING! Multiple "
										"exact matches found!\x1b[0m\n");
								warnings++;
							} else {
								exactmatchfound = 1;
								pgptr = pagelist[j];
							}
						} else if (exactmatchfound == 0) {
							printf("      ?\x1b[33m page at "
										"%s (\"%s\")?\x1b[0m\n",
								pagelist[j]->fnode->path, buf2);
							pgptr = pagelist[j];
						}
					}
				}
				/* If we found a match for the referent, */
				if (pgptr != NULL) {
					if (exactmatchfound == 0) {
						printf("        \x1b[31mWARNING! "
								"Approximate match at \"%s\" used!\x1b[0m\n",
								pgptr->fnode->path);
						warnings++;
					}
					/* Update Node's Attribute to conform to the output
					   directory structure */
					strcpy(buf2, pgptr->fnode->name);
					if ((p = strrchr(buf2, '/')) != NULL) {
						memmove(buf2, p+1, strlen(buf)+1);
					}
					if ((p = strrchr(buf2, '.')) != NULL) {
						*p = '\0';
					}
					strcat(buf2, ".html");
/*					printf("        Updating Node's Attribute value for key "
							"'href' to \"%s\"\n", buf2);*/
					removeattribute(nptr, "href");
					addattribute(nptr, "href",
							buf2, strlen(buf2));
					/* Store pointer to referent's Page in outlinks */
					for (j = 0; j < MAX_PAGES; j++) {
						if (pagelist[i]->outlinks[j] == NULL) {
							pagelist[i]->outlinks[j] = pgptr;
							break;
						} else {
							if (pagelist[i]->outlinks[j] == pgptr) {
								break;
							}
						}
					}
					if (j == MAX_PAGES) {
						printf("\x1b[31mWARNING! Page %s"
								" has full outlink list\x1b[0m\n",
								pagelist[i]->fnode->name);
						warnings++;
					}
					/* Store pointer to the Page we're checking in the ref-
					   erent's list of backlinks ('pages that link here') */
					for (j = 0; j < MAX_PAGES; j++) {
						if (pgptr->backlinks[j] == NULL) {
							pgptr->backlinks[j] = pagelist[i];
							break;
						} else {
							if (pgptr->backlinks[j] == pagelist[i]) {
								break;
							}
						}
					}
					if (j == MAX_PAGES) {
						printf("\x1b[31mWARNING! Page %s"
								" has full backlink list\x1b[0m\n",
								pgptr->fnode->name);
						warnings++;
					}
				/* Warn if we could not find even an approximate match */
				} else {
					printf("        \x1b[31mWARNING! "
							"No match found!\x1b[0m\n");
					warnings++;
				}
			} else if (guesslink(buf) == LINKTYPE_WEB) {
				printf("    Link to \"%s\" - web\n", buf);
				/* TODO Perhaps use libcurl to check for dead links */
			} else if (guesslink(buf) == LINKTYPE_FRAGMENT) {
				printf("    Link to \"%s\" - fragment\n", buf);
			} else if (guesslink(buf) == LINKTYPE_OTHER) {
				printf("    Link to \"%s\" - attachment?\n", buf);
				k = fnodesearch(fnode_sources, buf, fnptrbuf, MAX_PAGES);
				exactmatchfound = 0;
				for (j = 0; j < k; j++) {
					if (strcmp(buf, fnptrbuf[j]->name) == 0) {
						printf("      %c \x1b[32m%s\x1b[0m\n",
							(exactmatchfound==0) ? '>' : ' ',
							fnptrbuf[j]->path);
						if (exactmatchfound == 1) {
							printf("        \x1b[31mWARNING! Multiple "
									"exact matches found!\x1b[0m\n");
							warnings++;
						} else {
							exactmatchfound = 1;
							fnptr = fnptrbuf[j];
						}
					} else if (exactmatchfound == 0) {
						printf("      ?\x1b[33m %s\x1b[0m\n",
								fnptrbuf[j]->path);
						fnptr = fnptrbuf[j];
					}
				}
				if (k > 0) {
					/* TODO Update link in Node; store link/file info. */
					if (exactmatchfound == 0) {
						printf("        \x1b[31mWARNING! "
								"Approximate match at \"%s\" used!\x1b[0m\n",
								fnptr->path);
						warnings++;
					}
			/*		printf("        Updating Node's Attribute \"href\" to"
							" \"%s\"\n", fnptr->name);*/
					removeattribute(nptr, "href");
					addattribute(nptr, "href",
									fnptr->name, strlen(fnptr->name));
					/* Add the referent's path to a list of attachments to be
					   copied to the output directory. */
					for (k = 0; k < MAX_PAGES; k++) {
						if (fnodelistother[k] == NULL) {
							fnodelistother[k] = fnptr;
							break;
						} else {
							if (fnodelistother[k] == fnptr) {
								break;
							}
						}
					}
					if (k == MAX_PAGES) {
						printf("\x1b[31mWARNING! "
								"Too many attachments!\x1b[0m\n");
						warnings++;
					}
					for (j = 0; j < nptr->numchildren; j++) {
						if (nptr->children[j]->type == TYPE_IMG) {
							printf("        Link has image requesting %s\n",
									getattributebykey(nptr->children[j],
									"src")->value);
							
							/* TODO Image handling!!! */
				/*			printf("        Updating Node's Attribute \"src\""
									" to \"%s\"\n", fnptr->name);*/
							removeattribute(nptr->children[j], "src");
							addattribute(nptr->children[j], "src",
										fnptr->name, strlen(fnptr->name));
						}
					}
				} else {
					printf("        \x1b[31mWARNING! "
							"No match found!\x1b[0m\n");
					warnings++;
				}
			} else {
				printf("\x1b[31mWARNING! Unidentifiable link type: "
						"\"%s\"\x1b[0m\n", buf);
				warnings++;
			}
		}
	}
	
	printf("\n");
	
	/* Remove isolated pages and record the number of orphans and dead ends */
	printf("\x1b[4mRemoving isolated pages:\x1b[24m\n");
	for (i = 0; i < MAX_PAGES; i++) {
		if (pagelist[i] != NULL) {
			if (pagelist[i]->backlinks[0] == NULL
					&& pagelist[i]->outlinks[0] == NULL) {
				printf("\x1b[31m%s\x1b[0m ",
						pagelist[i]->fnode->name);
				pagefree(pagelist[i]);
				pagelist[i] = NULL;
				numpages--;
				isolates++;
				continue;
			}
			if (pagelist[i]->outlinks[0] == NULL) {
				deadends++;
			}
			if (pagelist[i]->backlinks[0] == NULL) {
				/*printf("\x1b[31m(orphan) \x1b[0m");*/
				orphans++;
			}
			/*
			printf("%s\x1b[0m\n", pagelist[i]->fnode->name);
			printf("    outgoing links: ");
			for (j = 0; j < MAX_PAGES; j++) {
				if (pagelist[i]->outlinks[j] != NULL) {
					printf("%s ",
							pagelist[i]->outlinks[j]->fnode->name);
				}
			}
			printf("\n");
			printf("    incoming links: ");
			for (j = 0; j < MAX_PAGES; j++) {
				if (pagelist[i]->backlinks[j] != NULL) {
					printf("\x1b[93m%s\x1b[0m ",
							pagelist[i]->backlinks[j]->fnode->name);
				}
			}
			printf("\n");*/
		}
	}	
	if (isolates == 0) {
		printf("\x1b[32mNo isolated pages!\x1b[0m\n");
	} else {
		printf("\n");
	}

	printf("\n");
	
	printf("\x1b[4mMaking index page:\x1b[24m\n");
	indexpage = createindexpage(pagelist, &warnings);
	
	/* Make headers and footers */
	printf("\x1b[4mMaking headers and footers:\x1b[24m\n");
	for (i = 0; i < MAX_PAGES; i++) {
		if (pagelist[i] != NULL) {
			warnings += makepageheader(pagelist[i]);
			warnings += makepagenav(pagelist[i]);
			warnings += makepagefooter(pagelist[i]);
		}
	}
	
	printf("\n");

	/* Inform the user about how badly things went */	
	if (warnings > 0) {
		printf("\x1b[31mLoading completed with %d warning%s; "
				"see logs for details\x1b[0m\n", warnings,
				(warnings==1) ? "" : "s");
	} else {
		printf("Loading completed with no warnings\n");
	}
	if (isolates > 0) {	
		printf("\x1b[31m%d of %d pages were isolated"
					" and have been removed\x1b[0m\n",
					isolates, numpages+isolates);
	}
	if (orphans > 0) {	
		printf("\x1b[31m%d of %d pages are orphans\x1b[0m\n",
					orphans, numpages);
	}
	if (deadends > 0) {	
		printf("\x1b[31m%d of %d pages are dead ends\x1b[0m\n",
					deadends, numpages);
	}

	printf("\n");

	printf("Continue to writing? (y/N) ");
	if (getchar() == 'y') {
		
		printf("\n");

		/* Write the index page first so it can't get overwritten */
		strcpy(buf, outputdir);
		strcat(buf, "/index.html");
		printf("\x1b[4mWriting index page:\x1b[24m\n");
		writepagehtml(indexpage, buf, "../config/_template.html");
	
		printf("\n");
	
		/* Print all the attachments to be copied */
		printf("\x1b[4mCopying attachments to \"%s\":\x1b[24m\n",
				outputdir);
		for (i = 0; i < MAX_PAGES; i++) {
			if (fnodelistother[i] != NULL) {
				strcpy(buf, outputdir);
				strcat(buf, "/");
				strcat(buf, fnodelistother[i]->name);
				printf("%s -> %s... ",
						fnodelistother[i]->name, buf);
				if (copyfile(buf, fnodelistother[i]->path) == 0) {
					printf("\x1b[32msuccess!\x1b[0m\n");
				}
			}
		}

		printf("\n");

		printf("\x1b[4mWriting %d pages with template \"%s\":\x1b[24m\n",
				numpages, "../config/_template.html");	
		for (i = 0; i < MAX_PAGES; i++) {
			if (pagelist[i] != NULL) {
				strcpy(buf, outputdir);
				strcat(buf, "/");
	
				strcpy(buf2, pagelist[i]->fnode->name);
				if ((p = strrchr(buf2, '.')) != NULL) {
					*p = '\0';
				}
				strcat(buf2, ".html");
	
				strcat(buf, buf2);
				
				writepagehtml(pagelist[i], buf, "../config/_template.html");
			}
		}
	}

	printf("\n");

	/* Clean up all our allocated things */
	for (i = 0; i < MAX_PAGES; i++) {
		if (pagelist[i] != NULL) {
/*			printf("freeing page at %p\n", (void*) pagelist[i]);*/
			pagefree(pagelist[i]);
		}
	}
	pagefree(indexpage);
	fnodefree(fnode_sources);
	fnodefree(fnode_config);
	
	return(0);
}


/****************************************************************************/
/* INDEX PAGE: SUBGRAPHS                                                    */
/****************************************************************************/

/* Who handles writing pages? Any method can initialize or modify them, but
   when do we have the authority to write to the disk? Call functions like 
   writepagehtml() or copyfile()? 

   The original way that I conceived of this question had a simple answer:
   Pages exist as data structures merely containing all the necessary data to
   write a Page in any context, namely the Nodes that comprise it, its name,
   its source's location on the disk, etc. The value of this way of handling
   Pages is that everything can be orchestrated without committing anything to
   the output directory, and hopefully we can wind up with a big blob that is
   simply and uneventfully passed to the writing-handler. This philosophy is
   probably still a good one to uphold.

   Then, in this view, where does the index page live? What if we decided to
   create subgraph-specific indices? These should all be built to the Node's
   specification, then wrapped in a Page... but they have no associated files,
   and the page has no source. The problem is that we're trying to use the
   program to accomplish something that wasn't in my initial conception of its
   scope: build Pages programmatically from scratch! Pages are supposed to be
   linked to source files, and the architecture of amanuensis is currently a
   pipeline that expects exactly one input: a directory of source files, and
   produces exactly one output: a directory of html files. Injecting into this
   pipeline algorithmically described outputs breaks the bijection I was imag-
   ining was fundamental.

   What can we do about this? We have been abusing the Node handling routines
   to produce the navigation sections, which has worked okay. We can probably
   do the same. We realize, then, that we should not be creating a Page, but
   merely a Node which will be used as a page body in the same pipeline that
   the rest of the pages go through. */

Page *createindexpage(Page *pglist[MAX_PAGES], int *warnings) {
	int i, j, k;
	Page *subgraphs[32][MAX_PAGES];
	int sgsortbuf[32];
	int sgdists[MAX_PAGES];
	int sortbuf[MAX_PAGES];
	int sortint = 0;
	
	Node *nodemain;
	Node *nptr;
	Node *nptr2;

	Page *pgptr;
	Page *p;
	
	char buf[1024];
	int pgcount = 0;
	int sgcount = 0;
	
	int indegree = 0;
	int outdegree = 0;
	
	/* Initialize an empty list of subgraph-lists */
	for (i = 0; i < MAX_PAGES; i++) {
		for (j = 0; j < 32; j++) {
			subgraphs[j][i] = NULL;
		}
		sortbuf[i] = 0;
	}
	for (i = 0; i < 32; i++) {
		sgsortbuf[i] = 0;
	}
	
	/* Find all the subgraphs */
	for (i = 0; i < MAX_PAGES; i++) {
		if (pglist[i] != NULL) {
			pgcount++;
			/* check all the subgraphs we've found */
			for (j = 0; j < 32; j++) {
				for (k = 0; k < MAX_PAGES; k++) {
					if (subgraphs[j][k] != NULL) {
						if (subgraphs[j][k] == pglist[i]) {
							k = -1;
							break;
						}
					}
				}
				if (k == -1) {
					break;
				}
			}
			if (k != -1) {
				for (j = 0; j < 32; j++) {
					if (subgraphs[j][0] == NULL) {
						break;
					}
				}
				if (j == 32) {
					j = 31;
					(*warnings)++;
					printf("\x1b[31m WARNING! More than 32 subgraphs,"
							" some lost\x1b[0m\n");
				}
				printf("Found subgraph of size %d:\n", 
					(sgsortbuf[j] =
						getpagesubgraph(subgraphs[j], sgdists, pglist[i]))
				);
				sgcount++;
				for (k = 0; k < MAX_PAGES; k++) {
					if (subgraphs[j][k] != NULL) {
						printf("    %s\n", subgraphs[j][k]->fnode->name);
					}
				}
				printf("\n");
			}
		}
	}
	
	/* Sort the pages within each subgraph, using (2*outlinks + backlinks) as
	   the key */
	for (i = 0; i < 32; i++) {
		for (j = 0; j < MAX_PAGES; j++) {
			sortbuf[j] = 0;
		}
		for (j = 0; j < MAX_PAGES; j++) {
			if (subgraphs[i][j] == NULL) {
				continue;
			}
			for (k = 0; k < MAX_PAGES; k++) {
				if (subgraphs[i][j]->outlinks[k] != NULL) {
					sortbuf[j] += 2;
				}
				if (subgraphs[i][j]->backlinks[k] != NULL) {
					sortbuf[j] += 1;
				}
			}
		}
		j = 1;
		k = 0;
		while (j < MAX_PAGES) {
			k = j;
			while (k > 0 && sortbuf[k] > sortbuf[k-1]) {
				sortint = sortbuf[k-1];
				sortbuf[k-1] = sortbuf[k];
				sortbuf[k] = sortint;
				pgptr = subgraphs[i][k-1];
				subgraphs[i][k-1] = subgraphs[i][k];
				subgraphs[i][k] = pgptr;
				k--;
			}
			j++;
		}
	}
	
	/* Sort the subgraph lists as wholes according to their cardinality. */
	for (i = 1; i < 32; i++) {
		j = i;
		while (j > 0 && sgsortbuf[j] > sgsortbuf[j-1]) {
			sortint = sgsortbuf[j-1];
			sgsortbuf[j-1] = sgsortbuf[j];
			sgsortbuf[j] = sortint;
			for (k = 0; k < MAX_PAGES; k++) {
				pgptr = subgraphs[j-1][k];
				subgraphs[j-1][k] = subgraphs[j][k];
				subgraphs[j][k] = pgptr;
			}
			j--;
		}
		i++;
	}
	
	sprintf(buf, "This site consists of %d pages which reside within %d"
		" connected component%s.", 
			pgcount, sgcount, (sgcount == 1) ? "" : "s");
	
	/* Initialize a page to be the index page */
	nodemain = createnode("", 0, NULL, TYPE_MAXTYPE);
	
	nptr = createnode("", 0, nodemain, TYPE_P);
	createnode(buf, strlen(buf), nptr, TYPE_PLAIN);
	
	/* TODO better formatting */
	
	for (i = 0; i < sgcount; i++) {
		nptr = createnode("", 0, nodemain, TYPE_P);
		addattribute(nptr, "class", "subgraphbox", strlen("subgraphbox"));
		for (k = 0; k < MAX_PAGES; k++) {
			if (subgraphs[i][k] != NULL) {
				indegree = 0;
				outdegree = 0;
				for (j = 0; j < MAX_PAGES; j++) {
					if (subgraphs[i][k]->outlinks[j] != NULL) {
						outdegree++;
					}
					if (subgraphs[i][k]->backlinks[j] != NULL) {
						indegree++;
					}
				}
				sortint = indegree + 2*outdegree;
				
				nptr2 = createnode(subgraphs[i][k]->displayname,
						strlen(subgraphs[i][k]->displayname),
						nptr, TYPE_A);
				addattribute(nptr2,	"href",
						subgraphs[i][k]->outfilename,
						strlen(subgraphs[i][k]->outfilename));

				if (((float)sortint)/((float)sgsortbuf[i]) >= 1.0f) {
					addattribute(nptr2, "class", "sg3", 3);
				} else if (((float)sortint)/((float)sgsortbuf[i]) >= 0.5f) {
					addattribute(nptr2, "class", "sg2", 3);
				} else if (((float)sortint)/((float)sgsortbuf[i]) >= 0.25) {
					addattribute(nptr2, "class", "sg1", 3);
				} else {	
					addattribute(nptr2, "class", "sg0", 3);
				}
			}
		}
	}
	
	p = pageinitempty(); 
	strcpy(p->displayname, "home");
	strcpy(p->outfilename, "index.html");

	p->nodemain = nodemain;
/*
	p->nodenav = createnode("", 0, NULL, TYPE_MAXTYPE);

	nptr = createnode("", 0, p->nodenav, TYPE_P);
	addattribute(nptr, "class", "row3", 4);

	nptr = createnode("", 0, nptr, TYPE_STRONG);
	createnode("home", 4, nptr, TYPE_EM);*/

	makepagenav(p);
	makepageheader(p);
	
	
	return(p);
}


/****************************************************************************/
/* PAGE AMENDMENTS                                                          */
/****************************************************************************/

int makepageheader (Page *pg) {
	Node *nodeheader;
	
	nodeheader = createnode("", 0, NULL, TYPE_MAXTYPE);
	pg->nodeheader = nodeheader;

	nodeheader = createnode("", 0, nodeheader, TYPE_A);
	addattribute(nodeheader, "href", "/", strlen("/"));
/*	nodeheader = createnode("", 0, nodeheader, TYPE_IMG);*/
/*	addattribute(nodeheader, "src", "calipers.png", strlen("calipers.png"));*/


	return(0);
}

/* Apparently this ought to live in what is known in the biz as a <nav> tag
   so TODO update all the "header" content to "nav" content. Then we can use
   the <header> tag to hold the title and whatnot as intended. */
int makepagenav(Page *pg) {
	Node *nodenav;
	int i;
	Page *currentsubgraph[MAX_PAGES];
	int depths[MAX_PAGES];
	Node *nodeback2;
	Node *nodeback1;
	Node *nodeself;
	Node *nodeout1;
	Node *nodeout2;
	Node *nptr;
	
	nodenav = createnode("", 0, NULL, TYPE_MAXTYPE);
	pg->nodenav = nodenav;

	nodeback2 = createnode("", 0, nodenav, TYPE_P);
	addattribute(nodeback2, "class", "row1", 4);
	nodeback1 = createnode("", 0, nodenav, TYPE_P);
	addattribute(nodeback1, "class", "row2", 4);

	nodeself = createnode("", 0, nodenav, TYPE_P);
	addattribute(nodeself, "class", "row3", 4);

	nodeout1 = createnode("", 0, nodenav, TYPE_P);
	addattribute(nodeout1, "class", "row4", 4);
	nodeout2 = createnode("", 0, nodenav, TYPE_P);
	addattribute(nodeout2, "class", "row5", 4);
	
	getpagesubgraph(currentsubgraph, depths, pg);
	
/*	printf("\x1b[93m%s lives in a subgraph of %d pages:\x1b[0m\n",
			pg->fnode->name, k);*/
	for (i = 0; i < MAX_PAGES; i++) {
		if (currentsubgraph[i] == NULL) {
			break;
		} else {
/*			printf("%d - %s\n", depths[i], currentsubgraph[i]->fnode->name);*/
			switch (depths[i]) {
				case -2: nptr = nodeback2; break;
				case -1: nptr = nodeback1; break;
				case 0: nptr = nodeself; break;
				case 1: nptr = nodeout1; break;
				case 2: nptr = nodeout2; break;
				default: nptr = NULL;
			}
			if (nptr != NULL) {
				if (depths[i] != 0) {
					nptr = createnode(currentsubgraph[i]->displayname,
							strlen(currentsubgraph[i]->displayname),
							nptr, TYPE_A);
					addattribute(nptr, "href",
							currentsubgraph[i]->outfilename,
							strlen(currentsubgraph[i]->outfilename));
				} else {
					nptr = createnode("", 0, nptr, TYPE_STRONG);
					nptr = createnode(currentsubgraph[i]->displayname,
							strlen(currentsubgraph[i]->displayname),
							nptr, TYPE_EM);
				}
			}
		}
	}
	return(0);
}

int getpagesubgraph(Page *subgraph[MAX_PAGES], int d[MAX_PAGES], Page *pg) {

	int i, j, k = 0;
	int depth = 0;
	Page *pgptr = pg;

	for (i = 0; i < MAX_PAGES; i++) {
		d[i] = 0;
		subgraph[i] = NULL;
	}

	d[0] = 0;
	subgraph[0] = pg;

	/* Get new neighbors and add them to "subgraph" */
	while (k < MAX_PAGES-1) {
		for (i = 0; i < MAX_PAGES; i++) {
			depth = d[k];
			if(pgptr->outlinks[i] != NULL) {
				for (j = 0; j < MAX_PAGES; j++) {
					if (subgraph[j] == pgptr->outlinks[i]) {
						break;
					} else if (subgraph[j] == NULL) {
						if (depth == 0) {
							d[j] = 1;
						} else {
							d[j] = (depth > 0) ? depth+1 : depth-3;
						}
						subgraph[j] = pgptr->outlinks[i];
						break;
					}
				}
			}
			if(pgptr->backlinks[i] != NULL) {
				for (j = 0; j < MAX_PAGES; j++) {
					if (subgraph[j] == pgptr->backlinks[i]) {
						break;
					} else if (subgraph[j] == NULL) {
						if (depth == 0) {
							d[j] = -1;
						} else {
							d[j] = (depth > 0) ? depth+3 : depth-1;
						}
						subgraph[j] = pgptr->backlinks[i];
						break;
					}
				}
			}
		}
		k++;
		if (subgraph[k] == NULL) {
			break;
		} else {
			pgptr = subgraph[k];			
		}
	}
	
	return(k);
}

int makepagefooter(Page *pg) {
	int i;
	Node *nodefooter;
	Node *nptr;
	char namebuf[MAX_URLSZ];
	char linkbuf[MAX_URLSZ];
	char *emptymsg = "There are no pages that link here.";
	char *notemptymsg = "Pages that link here: ";
	char *p;
	
	nodefooter = createnode("", 0, NULL, TYPE_MAXTYPE);
	pg->nodefooter = nodefooter;

	if (pg->backlinks[0] == NULL) {
		createnode(emptymsg, strlen(emptymsg), nodefooter, TYPE_P);
		return(0);
	} else {	
		nodefooter = createnode(notemptymsg, strlen(notemptymsg),
					nodefooter, TYPE_P);
	}

	for (i = 0; i < MAX_PAGES; i++) {
		if (pg->backlinks[i] != NULL) {
			/* Make a name */
			strcpy(namebuf, pg->backlinks[i]->fnode->name);
			if ((p = strrchr(namebuf, '.')) != NULL) {
				*p = '\0';
			}
			while ((p = strchr(namebuf, '_')) != NULL) {
				*p = ' ';
			}

			/* TODO this ought to be stored in each Page methinks, since we
			   keep needing it in various contexts and having to rebuild it
			   every time... */
			strcpy(linkbuf, pg->backlinks[i]->fnode->name);
			if ((p = strrchr(linkbuf, '.')) != NULL) {
				*p = '\0';
			}
			strcat(linkbuf, ".html");
			
			nptr = createnode(namebuf, strlen(namebuf), nodefooter, TYPE_A);
			addattribute(nptr, "href", linkbuf, strlen(linkbuf));
			nptr = createnode(" ", 1, nodefooter, TYPE_PLAIN);
		}
	}
	
	return(0);
}

/****************************************************************************/
/* WRITING                                                                  */
/****************************************************************************/

int writepagehtml(Page *pg, char *writedir, char *templatedir) {

	FILE *writefp = NULL;	
	FILE *templatefp = NULL;
	
	struct stat filestat;
	
	char title[128];
	char *p;

	if (pg->fnode != NULL) {
		strcpy(title, TITLE_PREFIX);
		strcat(title, " - ");
		strcat(title, pg->displayname);
	} else {
		strcpy(title, TITLE_PREFIX);
		strcat(title, " - home");
	}

	if (stat(writedir, &filestat) == 0) {
		printf("\x1b[31mWARNING! Refusing to overwrite \"%s\"\x1b[0m\n",
			writedir);
		return(1);
	}
	if ((writefp = fopen(writedir, "w")) == NULL) {
		printf("\x1b[31m[pages] [mdtohtml] "
				"failed to open for writing: \"%s\"\x1b[0m\n", writedir);
		return(1);
	}
	if ((templatefp = fopen(templatedir, "r")) == NULL) {
		printf("\x1b[31m[pages] [mdtohtml] "
				"failed to open template: \"%s\"\x1b[0m\n", templatedir);
		fclose(writefp);
		return(1);
	}
	
	printf("Writing %s... ", writedir);
	if (writehtmlfromtemplate(writefp, templatefp, title,
			pg->nodeheader, pg->nodenav, pg->nodemain, pg->nodefooter) == 0) {
		fclose(writefp);
		stat(writedir, &filestat);
		printf("\x1b[32mwrote %ld bytes\x1b[0m\n",
				(long int) filestat.st_size);
	} else {
		fclose(writefp);
		fclose(templatefp);
		printf("\x1b[31mfailed to write\x1b[0m\n");
		return(1);
	}

	/* Close any files we might have left open */
	fclose(templatefp);
	
	return(0);
}

int copyfile(char *dest, char *src) {
	FILE *srcfp;
	FILE *destfp;
	
	int readsz = 0;
	
	char buf[4096];

	struct stat filestat;
	
	/* Ask user for permission to overwrite file at writedir if it exists */	
	if (stat(dest, &filestat) == 0) {
		printf("\x1b[31m[pages] [copyfile] "
				"refusing to overwrite \"%s\"\x1b[0m\n", dest);
		return(1);
	}
	if ((destfp = fopen(dest, "wb")) == NULL) {
		printf("\x1b[31m[pages] [copyfile] "
				"failed to open for writing: \"%s\"\x1b[0m\n", dest);
		return(1);
	}
	if ((srcfp = fopen(src, "rb")) == NULL) {
		printf("\x1b[31m[pages] [copyfile] "
				"failed to open for reading: \"%s\"\x1b[0m\n", src);
		fclose(destfp);
		return(1);
	}
	
	while ((readsz = fread(buf, sizeof(char), 4096, srcfp)) > 0) {
		fwrite(buf, sizeof(char), readsz, destfp);
	}
	
	fclose(destfp);
	fclose(srcfp);
	
	return(0);
}

/****************************************************************************/
/* HELPER FUNCTIONS                                                         */
/****************************************************************************/

int guesslink(char *linkstr) {
	if ( (strncmp(linkstr, "https://", 8) == 0)
			|| (strncmp(linkstr, "http://", 7) == 0)
			|| (strstr(linkstr, "//") != NULL)
			|| (strncmp(linkstr, "www.", 4) == 0) ) {
		return(LINKTYPE_WEB);
	}
	if (linkstr[0] == '#') {
		return(LINKTYPE_FRAGMENT);
	}
	if ( (strstr(linkstr, ".md") != NULL)
			|| (strstr(linkstr, ".html") != NULL)
			|| (strchr(linkstr, '.') == NULL) ) {
		return(LINKTYPE_PAGE);
	}
	return(LINKTYPE_OTHER);
}


/****************************************************************************/
/* PAGE HANDLING FUNCTIONS                                                  */
/****************************************************************************/

/* Initialize a Page from a .md source file at *fnode by opening the file,
   calling parsemd, and setting metadata. */
Page *pageinit(Filenode *fnode) {
	Page *pg;
	FILE *readfp = NULL;
	int i;
	char *p;

	if (fnode == NULL) {
		printf("\x1b[31m[pages] [pageinit] "
				"received NULL Filenode, cannot create page\x1b[0m\n");
		return(NULL);
	}

	pg = malloc(sizeof(Page));
	
	pg->fnode = fnode;

	strcpy(pg->outfilename, fnode->name);
	strcpy(pg->displayname, fnode->name);

	if ((p = strrchr(pg->outfilename, '.')) != NULL) {
		strcpy(p, ".html");
	} else {
		strcat(pg->outfilename, ".html");
	}
	
	if ((p = strrchr(pg->displayname, '.')) != NULL) {
		*p = '\0';
	}
	while ((p = strchr(pg->displayname, '_')) != NULL) {
		*p = ' ';
	}

	pg->nodeheader = NULL;
	pg->nodenav = NULL;
	pg->nodefooter = NULL;

	for (i = 0; i < MAX_PAGES; i++) {
		pg->outlinks[i] = NULL;
		pg->backlinks[i] = NULL;
	}

	/* Open file pointers to relevant files */	
	if ((readfp = fopen(fnode->path, "r")) == NULL) {
		printf("\x1b[31m[pages] [pageinit] "
				"failed to open for reading: \"%s\"\x1b[0m\n", fnode->path);
		return(NULL);
	}
	if ((pg->nodemain = parsemd(readfp)) == NULL) {
		printf("\x1b[31m[pages] [pageinit] "
				"failed to parse: \"%s\"\x1b[0m\n", fnode->path);
		return(NULL);
	}
	
	fclose(readfp);
	return(pg);	
}

Page *pageinitempty() {
	Page *pg;
	int i;

	pg = malloc(sizeof(Page));
	
	pg->fnode = NULL;

	pg->nodeheader = NULL;
	pg->nodenav = NULL;
	pg->nodefooter = NULL;

	for (i = 0; i < MAX_PAGES; i++) {
		pg->outlinks[i] = NULL;
		pg->backlinks[i] = NULL;
	}
	
	return(pg);	
	
}

/* Free a Page's Nodes (if they exist) and then free the Page itself */
void pagefree(Page *pg) {
	if (pg->nodeheader != NULL) {
		freenodes(pg->nodeheader);
	}
	if (pg->nodenav != NULL) {
		freenodes(pg->nodenav);
	}
	if (pg->nodemain != NULL) {
		freenodes(pg->nodemain);
	}
	if (pg->nodefooter != NULL) {
		freenodes(pg->nodefooter);
	}
	free(pg);
}


/* Print all the data of a Page for debugging */
void pageprint(Page *pg) {
	printf("\x1b[93m------ PAGE ------ (%p)\n", (void *)pg);

	printf("fnode->path: \"%s\"\n", pg->fnode->path);
	printf("fnode->name: \"%s\"\n", pg->fnode->name);
	printf("nodenav: %4d nodes (%p)\n", countnodes(pg->nodenav),
			(void *)pg->nodenav);
	printf("nodemain:   %4d nodes (%p)\n", countnodes(pg->nodemain),
			(void *)pg->nodemain);
	printf("nodefooter: %4d nodes (%p)\n", countnodes(pg->nodefooter),
			(void *)pg->nodefooter);
	printf("outlinks: ?\n"); /* TODO link printing */
	printf("backlinks: ?\n"); /* TODO backlink printing */

	printf("------------------\x1b[0m\n");
}

/****************************************************************************/
/* DEPRECATED MISCELLANEA                                                   */
/****************************************************************************/

void nodeprintlinks(Node *n) {
	int i = 0;
	char buf[MAX_ATTRIBUTESZ];
	
	buf[0] = '\0';
	
	if (n == NULL) {
		return;
	}
	
	if (n->attributes != NULL) {
		if (n->type == TYPE_A) {
			writeattributebykey(buf, MAX_ATTRIBUTESZ, n, "href");
		}
		if (n->type == TYPE_IMG) {
			writeattributebykey(buf, MAX_ATTRIBUTESZ, n, "src");
		}
	}
	if (buf[0] != '\0') {
		printf("\x1b[33m%s\x1b[0m", buf);
		switch (guesslink(buf)) {
			case LINKTYPE_WEB:
				printf(" (web)\n");
				break;
			case LINKTYPE_PAGE:
				printf(" (page)\n");
				break;
			case LINKTYPE_FRAGMENT:
				printf(" (fragment)\n");
				break;
			case LINKTYPE_OTHER:
				printf(" (other)\n");
				break;
			default:
				break;
		}
	}
	while (i < n->numchildren) {
		if (n->children[i] != NULL) {
			nodeprintlinks(n->children[i]);
		}
		i++;
	}
}

void getcleannodetext(char *dest, int destsz, Node *n, int toplevel) {
	int i = 0;
	if (destsz <= 0) {
		return;
	}
	if (toplevel == 1) {
		strncpy(dest, n->content, destsz);
	} else {
		strncat(dest, n->content, destsz);
	}
	while (i < n->numchildren) {
		if (n->children[i] != NULL) {
			getcleannodetext(dest+strlen(dest), destsz-strlen(dest)-1, n->children[i], 0);
		}
		i++;
	}
}
