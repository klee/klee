/* $Id$ $Revision$ */
/* vim:set shiftwidth=4 ts=8: */

/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: See CVS logs. Details at http://www.graphviz.org/
 *************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gvconfig.h"

#include	<string.h>

#ifdef ENABLE_LTDL
#include	<sys/types.h>
#ifdef WIN32
#include <windows.h>
#define GLOB_NOSPACE    1   /* Ran out of memory.  */
#define GLOB_ABORTED    2   /* Read error.  */
#define GLOB_NOMATCH    3   /* No matches found.  */
#define GLOB_NOSORT     4
#define DMKEY "Software\\Microsoft" //key to look for library dir
#include "regex_win32.h"
//#include        <regex_win32.c>
typedef struct {
    int gl_pathc;           /* count of total paths so far */
    int gl_matchc;          /* count of paths matching pattern */
    int gl_offs;            /* reserved at beginning of gl_pathv */
    int gl_flags;           /* returned flags */
    char **gl_pathv;        /* list of paths matching pattern */
} glob_t;
static void globfree (glob_t* pglob);
static int glob (GVC_t * gvc, char*, int, int (*errfunc)(const char *, int), glob_t*);
#else
#include        <regex.h>
#include	<glob.h>
#endif 
#include	<sys/stat.h>
#ifdef HAVE_UNISTD_H
#include	<unistd.h>
#endif
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include        "memory.h"
#include        "const.h"
#include        "types.h"

#include	"gvplugin.h"
#include	"gvcjob.h"
#include	"gvcint.h"
#include        "gvcproc.h"

/*
    A config for gvrender is a text file containing a
    list of plugin librariess and their capabilities using a tcl-like
    syntax

    Lines beginning with '#' are ignored as comments

    Blank lines are allowed and ignored.

    plugin_library_path packagename {
	plugin_api {
	    plugin_type plugin_quality
	    ...
	}
	...
    ...

    e.g.

	/usr/lib/graphviz/libgvplugin_cairo.so cairo {renderer {x 0 png 10 ps -10}}
	/usr/lib/graphviz/libgvplugin_gd.so gd {renderer {png 0 gif 0 jpg 0}}

    Internally the config is maintained as lists of plugin_types for each plugin_api.
    If multiple plugins of the same type are found then the highest quality wins.
    If equal quality then the last-one-installed wins (thus giving preference to
    external plugins over internal builtins).

 */

static gvplugin_package_t * gvplugin_package_record(GVC_t * gvc, char *path, char *name)
{
    gvplugin_package_t *package = gmalloc(sizeof(gvplugin_package_t));
    package->path = (path) ? strdup(path) : NULL;
    package->name = strdup(name);
    package->next = gvc->packages;
    gvc->packages = package;
    return package;
}

#ifdef ENABLE_LTDL
/*
  separator - consume all non-token characters until next token.  This includes:
	comments:   '#' ... '\n'
	nesting:    '{'
	unnesting:  '}'
	whitespace: ' ','\t','\n'

	*nest is changed according to nesting/unnesting processed
 */
static void separator(int *nest, char **tokens)
{
    char c, *s;

    s = *tokens;
    while ((c = *s)) {
	/* #->eol = comment */
	if (c == '#') {
	    s++;
	    while ((c = *s)) {
		s++;
		if (c == '\n')
		    break;
	    }
	    continue;
	}
	if (c == '{') {
	    (*nest)++;
	    s++;
	    continue;
	}
	if (c == '}') {
	    (*nest)--;
	    s++;
	    continue;
	}
	if (c == ' ' || c == '\n' || c == '\t') {
	    s++;
	    continue;
	}
	break;
    }
    *tokens = s;
}

/* 
  token - capture all characters until next separator, then consume separator,
	return captured token, leave **tokens pointing to next token.
 */
static char *token(int *nest, char **tokens)
{
    char c, *s, *t;

    s = t = *tokens;
    while ((c = *s)) {
	if (c == '#'
	    || c == ' ' || c == '\t' || c == '\n' || c == '{' || c == '}')
	    break;
	s++;
    }
    *tokens = s;
    separator(nest, tokens);
    *s = '\0';
    return t;
}

static int gvconfig_plugin_install_from_config(GVC_t * gvc, char *s)
{
    char *path, *name, *api;
    const char *type;
    api_t gv_api;
    int quality, rc;
    int nest = 0;
    gvplugin_package_t *package;

    separator(&nest, &s);
    while (*s) {
	path = token(&nest, &s);
	if (nest == 0)
	    name = token(&nest, &s);
        else
	    name = "x";
        package = gvplugin_package_record(gvc, path, name);
	do {
	    api = token(&nest, &s);
	    gv_api = gvplugin_api(api);
	    if (gv_api == -1) {
		agerr(AGERR, "invalid api in config: %s %s\n", path, api);
		return 0;
	    }
	    do {
		if (nest == 2) {
		    type = token(&nest, &s);
		    if (nest == 2)
		        quality = atoi(token(&nest, &s));
		    else
		        quality = 0;
		    rc = gvplugin_install (gvc, gv_api,
				    type, quality, package, NULL);
		    if (!rc) {
		        agerr(AGERR, "config error: %s %s %s\n", path, api, type);
		        return 0;
		    }
		}
	    } while (nest == 2);
	} while (nest == 1);
    }
    return 1;
}
#endif

void gvconfig_plugin_install_from_library(GVC_t * gvc, char *path, gvplugin_library_t *library)
{
    gvplugin_api_t *apis;
    gvplugin_installed_t *types;
    gvplugin_package_t *package;
    int i;

    package = gvplugin_package_record(gvc, path, library->packagename);
    for (apis = library->apis; (types = apis->types); apis++) {
	for (i = 0; types[i].type; i++) {
	    gvplugin_install(gvc, apis->api, types[i].type,
			types[i].quality, package, &types[i]);
        }
    }
}

static void gvconfig_plugin_install_builtins(GVC_t * gvc)
{
    const lt_symlist_t *s;
    const char *name;

    if (gvc->common.builtins == NULL) return;

    for (s = gvc->common.builtins; (name = s->name); s++)
	if (name[0] == 'g' && strstr(name, "_LTX_library")) 
	    gvconfig_plugin_install_from_library(gvc, NULL,
		    (gvplugin_library_t *)(s->address));
}

#ifdef ENABLE_LTDL
static void gvconfig_write_library_config(GVC_t *gvc, char *path, gvplugin_library_t *library, FILE *f)
{
    gvplugin_api_t *apis;
    gvplugin_installed_t *types;
    int i;

    fprintf(f, "%s %s {\n", path, library->packagename);
    for (apis = library->apis; (types = apis->types); apis++) {
        fprintf(f, "\t%s {\n", gvplugin_api_name(apis->api));
	for (i = 0; types[i].type; i++) {
	    /* verify that dependencies are available */
            if (! (gvplugin_load(gvc, apis->api, types[i].type)))
		fprintf(f, "#FAILS");
	    fprintf(f, "\t\t%s %d\n", types[i].type, types[i].quality);
	}
	fputs ("\t}\n", f);
    }
    fputs ("}\n", f);
}

#define BSZ 1024
#define DOTLIBS "/.libs"
#define STRLEN(s) (sizeof(s)-1)

char * gvconfig_libdir(GVC_t * gvc)
{
    static char line[BSZ];
    static char *libdir;
    static boolean dirShown = 0; 
    char *tmp;

    if (!libdir) {
        libdir=getenv("GVBINDIR");
	if (!libdir) {
#ifdef WIN32
	    int r;
	    char* s;
		
		MEMORY_BASIC_INFORMATION mbi;
		if (VirtualQuery (&gvconfig_libdir, &mbi, sizeof(mbi)) == 0) {
		agerr(AGERR,"failed to get handle for executable.\n");
		return 0;
	    }
	    r = GetModuleFileName ((HMODULE)mbi.AllocationBase, line, BSZ);
	    if (!r || (r == BSZ)) {
		agerr(AGERR,"failed to get path for executable.\n");
		return 0;
	    }
	    s = strrchr(line,'\\');
	    if (!s) {
		agerr(AGERR,"no slash in path %s.\n", line);
		return 0;
	    }
	    *s = '\0';
	    libdir = line;
#else
	    libdir = GVLIBDIR;	    
#ifdef __APPLE__
	    uint32_t i, c = _dyld_image_count();
	    size_t len, ind;
	    const char* path;
	    for (i = 0; i < c; ++i) {
		path = _dyld_get_image_name(i);
		tmp = strstr(path, "/libgvc.");
		if (tmp) {
		    if (tmp > path) {
			/* Check for real /lib dir. Don't accept pre-install /.libs */
			char* s = tmp-1;
			/* back up to previous slash (or head of string) */
			while ((*s != '/') && (s > path)) s--;
			if (strncmp (s, DOTLIBS, STRLEN(DOTLIBS)) == 0);
			    continue;
		    }

		    ind = tmp - path;  /* byte offset */
		    len = ind + sizeof("/graphviz");
		    if (len < BSZ)
			libdir = line;
		    else
		        libdir = gmalloc(len);
		    bcopy (path, libdir, ind);
		    /* plugins are in "graphviz" subdirectory */
		    strcpy(libdir+ind, "/graphviz");  
		    break;
		}
	    }
#else
	    FILE* f = fopen ("/proc/self/maps", "r");
	    char* path;
	    if (f) {
		while (!feof (f)) {
		    if (!fgets (line, sizeof (line), f))
			continue;
		    if (!strstr (line, " r-xp "))
			continue;
		    path = strchr (line, '/');
		    if (!path)
		        continue;
		    tmp = strstr (path, "/libgvc.");
		    if (tmp) {
			*tmp = 0;
			/* Check for real /lib dir. Don't accept pre-install /.libs */
			if (strcmp(strrchr(path,'/'), "/.libs") == 0)
			    continue;
			strcpy(line, path);  /* use line buffer for result */
			strcat(line, "/graphviz");  /* plugins are in "graphviz" subdirectory */
			libdir = line;
			break;
		    }
		}
		fclose (f);
	    }
#endif
#endif
	}
    }
    if (gvc->common.verbose && !dirShown) {
	fprintf (stderr, "libdir = \"%s\"\n", (libdir ? libdir : "<null>"));
	dirShown = 1;
    }
    return libdir;
}
#endif

#ifdef ENABLE_LTDL
static void config_rescan(GVC_t *gvc, char *config_path)
{
    FILE *f = NULL;
    glob_t globbuf;
    char *config_glob, *config_re, *path, *libdir;
    int i, rc, re_status;
    gvplugin_library_t *library;
    regex_t re;
#ifndef WIN32
    char *plugin_glob = "libgvplugin_*";
#endif
#if defined(DARWIN_DYLIB)
    char *plugin_re_beg = "[^0-9]\\.";
    char *plugin_re_end = "\\.dylib$";
#elif defined(__MINGW32__)
	char *plugin_glob = "libgvplugin_*";
	char *plugin_re_beg = "[^0-9]-";
    char *plugin_re_end = "\\.dll$"; 
#elif defined(__CYGWIN__)
    plugin_glob = "cyggvplugin_*";
    char *plugin_re_beg = "[^0-9]-";
    char *plugin_re_end = "\\.dll$"; 
#elif defined(WIN32)
    char *plugin_glob = "gvplugin_*";
    char *plugin_re_beg = "[^0-9]";
    char *plugin_re_end = "\\.dll$"; 
#elif ((defined(__hpux__) || defined(__hpux)) && !(defined(__ia64)))
    char *plugin_re_beg = "\\.sl\\.";
    char *plugin_re_end = "$"; 
#else
    /* Everyone else */
    char *plugin_re_beg = "\\.so\\.";
    char *plugin_re_end= "$";
#endif

    if (config_path) {
	f = fopen(config_path,"w");
	if (!f) {
	    agerr(AGERR,"failed to open %s for write.\n", config_path);
	    exit(1);
	}

	fprintf(f, "# This file was generated by \"dot -c\" at time of install.\n\n");
	fprintf(f, "# You may temporarily disable a plugin by removing or commenting out\n");
	fprintf(f, "# a line in this file, or you can modify its \"quality\" value to affect\n");
	fprintf(f, "# default plugin selection.\n\n");
	fprintf(f, "# Manual edits to this file **will be lost** on upgrade.\n\n");
    }

    libdir = gvconfig_libdir(gvc);

    config_re = gmalloc(strlen(plugin_re_beg) + 20 + strlen(plugin_re_end) + 1);

#if defined(WIN32) && !defined(__MINGW32__) && !defined(__CYGWIN__)
    sprintf(config_re,"%s%s", plugin_re_beg, plugin_re_end);
#elif defined(GVPLUGIN_VERSION)
    sprintf(config_re,"%s%d%s", plugin_re_beg, GVPLUGIN_VERSION, plugin_re_end);
#else
    sprintf(config_re,"%s[0-9]+%s", plugin_re_beg, plugin_re_end);
#endif

    if (regcomp(&re, config_re, REG_EXTENDED|REG_NOSUB) != 0) {
	agerr(AGERR,"cannot compile regular expression %s", config_re);
    }

    config_glob = gmalloc(strlen(libdir) + 1 + strlen(plugin_glob) + 1);
    strcpy(config_glob, libdir);
	strcat(config_glob, DIRSEP);
    strcat(config_glob, plugin_glob);

    /* load all libraries even if can't save config */

#if defined(WIN32)
    rc = glob(gvc, config_glob, GLOB_NOSORT, NULL, &globbuf);
#else
    rc = glob(config_glob, GLOB_NOSORT, NULL, &globbuf);
#endif
    if (rc == 0) {
	for (i = 0; i < globbuf.gl_pathc; i++) {
	    re_status = regexec(&re, globbuf.gl_pathv[i], (size_t) 0, NULL, 0);
	    if (re_status == 0) {
		library = gvplugin_library_load(gvc, globbuf.gl_pathv[i]);
		if (library) {
		    gvconfig_plugin_install_from_library(gvc, globbuf.gl_pathv[i], library);
		}
	    }
	}
	/* rescan with all libs loaded to check cross dependencies */
	for (i = 0; i < globbuf.gl_pathc; i++) {
	    re_status = regexec(&re, globbuf.gl_pathv[i], (size_t) 0, NULL, 0);
	    if (re_status == 0) {
		library = gvplugin_library_load(gvc, globbuf.gl_pathv[i]);
		if (library) {
		    path = strrchr(globbuf.gl_pathv[i],DIRSEP[0]);
		    if (path)
			path++;
		    if (f && path)
			gvconfig_write_library_config(gvc, path, library, f);
		}
	    }
	}
    }
    regfree(&re);
    globfree(&globbuf);
    free(config_glob);
    free(config_re);
    if (f)
	fclose(f);
}
#endif

/*
  gvconfig - parse a config file and install the identified plugins
 */
void gvconfig(GVC_t * gvc, boolean rescan)
{
#if 0
    gvplugin_library_t **libraryp;
#endif
#ifdef ENABLE_LTDL
    int sz, rc;
    struct stat config_st, libdir_st;
    FILE *f = NULL;
    char *config_text = NULL;
    char *libdir;
    char *config_file_name = GVPLUGIN_CONFIG_FILE;

#define MAX_SZ_CONFIG 100000
#endif
    
    /* builtins don't require LTDL */
    gvconfig_plugin_install_builtins(gvc);
   
    gvc->config_found = FALSE;
#ifdef ENABLE_LTDL
    if (gvc->common.demand_loading) {
        /* see if there are any new plugins */
        libdir = gvconfig_libdir(gvc);
        rc = stat(libdir, &libdir_st);
        if (rc == -1) {
    	    /* if we fail to stat it then it probably doesn't exist so just fail silently */
	    return;
        }
    
        if (! gvc->config_path) {
            gvc->config_path = gmalloc(strlen(libdir) + 1 + strlen(config_file_name) + 1);
            strcpy(gvc->config_path, libdir);
            strcat(gvc->config_path, DIRSEP);
            strcat(gvc->config_path, config_file_name);
        }
    	
        if (rescan) {
    	    config_rescan(gvc, gvc->config_path);
    	    gvc->config_found = TRUE;
    	    return;
        }
    
        /* load in the cached plugin library data */
    
        rc = stat(gvc->config_path, &config_st);
        if (rc == -1) {
    	    /* silently return without setting gvc->config_found = TRUE */
    	    return;
        }
        else if (config_st.st_size > MAX_SZ_CONFIG) {
    	    agerr(AGERR,"%s is bigger than I can handle.\n", gvc->config_path);
        }
        else {
    	    f = fopen(gvc->config_path,"r");
    	    if (!f) {
    	        agerr (AGERR,"failed to open %s for read.\n", gvc->config_path);
		return;
    	    }
    	    else {
    	        config_text = gmalloc(config_st.st_size + 1);
    	        sz = fread(config_text, 1, config_st.st_size, f);
    	        if (sz == 0) {
    		    agerr(AGERR,"%s is zero sized, or other read error.\n", gvc->config_path);
    		    free(config_text);
    	        }
    	        else {
    	            gvc->config_found = TRUE;
    	            config_text[sz] = '\0';  /* make input into a null terminated string */
    	            rc = gvconfig_plugin_install_from_config(gvc, config_text);
    		    /* NB. config_text not freed because we retain char* into it */
    	        }
    	    }
    	    if (f) {
    	        fclose(f);
	    }
        }
    }
#endif
    gvtextlayout_select(gvc);   /* choose best available textlayout plugin immediately */
}

#ifdef ENABLE_LTDL
#ifdef WIN32

/* Emulating windows glob */

/* glob:
 * Assumes only GLOB_NOSORT flag given. That is, there is no offset,
 * and no previous call to glob.
 */

static int
glob (GVC_t* gvc, char* pattern, int flags, int (*errfunc)(const char *, int), glob_t *pglob)
{
    char* libdir;
    WIN32_FIND_DATA wfd;
    HANDLE h;
    char** str=0;
    int arrsize=0;
    int cnt = 0;
    
    pglob->gl_pathc = 0;
    pglob->gl_pathv = NULL;
    
    h = FindFirstFile (pattern, &wfd);
    if (h == INVALID_HANDLE_VALUE) return GLOB_NOMATCH;
    libdir = gvconfig_libdir(gvc);
    do {
      if (cnt >= arrsize-1) {
        arrsize += 512;
        if (str) str = (char**)realloc (str, arrsize*sizeof(char*));
        else str = (char**)malloc (arrsize*sizeof(char*));
        if (!str) return GLOB_NOSPACE;
      }
      str[cnt] = (char*)malloc (strlen(libdir)+1+strlen(wfd.cFileName)+1);
      if (!str[cnt]) return GLOB_NOSPACE;
      strcpy(str[cnt],libdir);
      strcat(str[cnt],DIRSEP);
      strcat(str[cnt],wfd.cFileName);
      cnt++;
    } while (FindNextFile (h, &wfd));
    str[cnt] = 0;

    pglob->gl_pathc = cnt;
    pglob->gl_pathv = (char**)realloc(str, (cnt+1)*sizeof(char*));
    
    return 0;
}

static void
globfree (glob_t* pglob)
{
    int i;
    for (i = 0; i < pglob->gl_pathc; i++)
      free (pglob->gl_pathv[i]);
    if (pglob->gl_pathv)
		free (pglob->gl_pathv);
}
#endif
#endif
