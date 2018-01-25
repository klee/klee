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

#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#ifdef HAVE_SYS_INOTIFY_H
#include <sys/inotify.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if 0
#ifdef WITH_CGRAPH
#include <poll.h>
#endif
#endif

#include "gvplugin_device.h"

#include <cairo.h>
#ifdef CAIRO_HAS_XLIB_SURFACE
#include <cairo-xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrender.h>

typedef struct window_xlib_s {
    Window win;
    unsigned long event_mask;
    Pixmap pix;
    GC gc;
    Visual *visual;
    Colormap cmap;
    int depth;
    Atom wm_delete_window_atom;
} window_t;

static void handle_configure_notify(GVJ_t * job, XConfigureEvent * cev)
{
/*FIXME - should allow for margins */
/*	- similar zoom_to_fit code exists in: */
/*	plugin/gtk/callbacks.c */
/*	plugin/xlib/gvdevice_xlib.c */
/*	lib/gvc/gvevent.c */

    job->zoom *= 1 + MIN(
	((double) cev->width - (double) job->width) / (double) job->width,
	((double) cev->height - (double) job->height) / (double) job->height);
    if (cev->width > job->width || cev->height > job->height)
        job->has_grown = 1;
    job->width = cev->width;
    job->height = cev->height;
    job->needs_refresh = 1;
}

static void handle_expose(GVJ_t * job, XExposeEvent * eev)
{
    window_t *window;

    window = (window_t *)job->window;
    XCopyArea(eev->display, window->pix, eev->window, window->gc,
              eev->x, eev->y, eev->width, eev->height, eev->x, eev->y);
}

static void handle_client_message(GVJ_t * job, XClientMessageEvent * cmev)
{
    window_t *window;

    window = (window_t *)job->window;
    if (cmev->format == 32
        && (Atom) cmev->data.l[0] == window->wm_delete_window_atom)
        exit(0);
}

static boolean handle_keypress(GVJ_t *job, XKeyEvent *kev)
{
    
    int i;
    KeyCode *keycodes;

    keycodes = (KeyCode *)job->keycodes;
    for (i=0; i < job->numkeys; i++) {
	if (kev->keycode == keycodes[i])
	    return (job->keybindings[i].callback)(job);
    }
    return FALSE;
}

static Visual *find_argb_visual(Display * dpy, int scr)
{
    XVisualInfo *xvi;
    XVisualInfo template;
    int nvi;
    int i;
    XRenderPictFormat *format;
    Visual *visual;

    template.screen = scr;
    template.depth = 32;
    template.class = TrueColor;
    xvi = XGetVisualInfo(dpy,
                         VisualScreenMask |
                         VisualDepthMask |
                         VisualClassMask, &template, &nvi);
    if (!xvi)
        return 0;
    visual = 0;
    for (i = 0; i < nvi; i++) {
        format = XRenderFindVisualFormat(dpy, xvi[i].visual);
        if (format->type == PictTypeDirect && format->direct.alphaMask) {
            visual = xvi[i].visual;
            break;
        }
    }

    XFree(xvi);
    return visual;
}

static void browser_show(GVJ_t *job)
{
#if defined HAVE_SYS_TYPES_H && defined HAVE_UNISTD_H && defined HAVE_ERRNO_H
   char *exec_argv[3] = {BROWSER, NULL, NULL};
   pid_t pid;
   int err;

   exec_argv[1] = job->selected_href;

   pid = fork();
   if (pid == -1) {
       fprintf(stderr,"fork failed: %s\n", strerror(errno));
   }
   else if (pid == 0) {
       err = execvp(exec_argv[0], exec_argv);
       fprintf(stderr,"error starting %s: %s\n", exec_argv[0], strerror(errno));
   }
#else
   fprintf(stdout,"browser_show: %s\n", job->selected_href);
#endif
}

static int handle_xlib_events (GVJ_t *firstjob, Display *dpy)
{
    GVJ_t *job;
    window_t *window;
    XEvent xev;
    pointf pointer;
    int rc = 0;

    while (XPending(dpy)) {
        XNextEvent(dpy, &xev);

        for (job = firstjob; job; job = job->next_active) {
	    window = (window_t *)job->window;
	    if (xev.xany.window == window->win) {
                switch (xev.xany.type) {
                case ButtonPress:
		    pointer.x = (double)xev.xbutton.x;
		    pointer.y = (double)xev.xbutton.y;
                    (job->callbacks->button_press)(job, xev.xbutton.button, pointer);
		    rc++;
                    break;
                case MotionNotify:
		    if (job->button) { /* only interested while a button is pressed */
		        pointer.x = (double)xev.xbutton.x;
		        pointer.y = (double)xev.xbutton.y;
                        (job->callbacks->motion)(job, pointer);
		        rc++;
		    }
                    break;
                case ButtonRelease:
		    pointer.x = (double)xev.xbutton.x;
		    pointer.y = (double)xev.xbutton.y;
                    (job->callbacks->button_release)(job, xev.xbutton.button, pointer);
		    if (job->selected_href && job->selected_href[0] && xev.xbutton.button == 1)
		        browser_show(job);
		    rc++;
                    break;
                case KeyPress:
		    if (handle_keypress(job, &xev.xkey))
			return -1;  /* exit code */
                    break;
                case ConfigureNotify:
                    handle_configure_notify(job, &xev.xconfigure);
		    rc++;
                    break;
                case Expose:
                    handle_expose(job, &xev.xexpose);
		    rc++;
                    break;
                case ClientMessage:
                    handle_client_message(job, &xev.xclient);
		    rc++;
                    break;
                }
	        break;
	    }
	}
    }
    return rc; 
}

static void update_display(GVJ_t *job, Display *dpy)
{
    window_t *window;
    cairo_surface_t *surface;

    window = (window_t *)job->window;

    if (job->has_grown) {
	XFreePixmap(dpy, window->pix);
	window->pix = XCreatePixmap(dpy, window->win,
			job->width, job->height, window->depth);
	job->has_grown = 0;
	job->needs_refresh = 1;
    }
    if (job->needs_refresh) {
	XFillRectangle(dpy, window->pix, window->gc, 0, 0,
                	job->width, job->height);
	surface = cairo_xlib_surface_create(dpy,
			window->pix, window->visual,
			job->width, job->height);
    	job->context = (void *)cairo_create(surface);
	job->external_context = TRUE;
        (job->callbacks->refresh)(job);
	cairo_surface_destroy(surface);
	XCopyArea(dpy, window->pix, window->win, window->gc,
			0, 0, job->width, job->height, 0, 0);
        job->needs_refresh = 0;
    }
}

static void init_window(GVJ_t *job, Display *dpy, int scr)
{
    int argb = 0;
    const char *base = "";
    XGCValues gcv;
    XSetWindowAttributes attributes;
    XWMHints *wmhints;
    XSizeHints *normalhints;
    XClassHint *classhint;
    unsigned long attributemask = 0;
    char *name;
    window_t *window;
    int w, h;
    double zoom_to_fit;

    window = (window_t *)malloc(sizeof(window_t));
    if (window == NULL) {
	fprintf(stderr, "Failed to malloc window_t\n");
	return;
    }

    w = 480;    /* FIXME - w,h should be set by a --geometry commandline option */
    h = 325;
    
    zoom_to_fit = MIN((double) w / (double) job->width,
			(double) h / (double) job->height);
    if (zoom_to_fit < 1.0) /* don't make bigger */
	job->zoom *= zoom_to_fit;

    job->width  = w;    /* use window geometry */
    job->height = h;

    job->window = (void *)window;
    job->fit_mode = 0;
    job->needs_refresh = 1;

    if (argb && (window->visual = find_argb_visual(dpy, scr))) {
        window->cmap = XCreateColormap(dpy, RootWindow(dpy, scr),
                                    window->visual, AllocNone);
        attributes.override_redirect = False;
        attributes.background_pixel = 0;
        attributes.border_pixel = 0;
        attributes.colormap = window->cmap;
        attributemask = ( CWBackPixel
                          | CWBorderPixel
			  | CWOverrideRedirect
			  | CWColormap );
        window->depth = 32;
    } else {
        window->cmap = DefaultColormap(dpy, scr);
        window->visual = DefaultVisual(dpy, scr);
        attributes.background_pixel = WhitePixel(dpy, scr);
        attributes.border_pixel = BlackPixel(dpy, scr);
        attributemask = (CWBackPixel | CWBorderPixel);
        window->depth = DefaultDepth(dpy, scr);
    }

    window->win = XCreateWindow(dpy, RootWindow(dpy, scr),
                             0, 0, job->width, job->height, 0, window->depth,
                             InputOutput, window->visual,
                             attributemask, &attributes);

    name = malloc(strlen("graphviz: ") + strlen(base) + 1);
    strcpy(name, "graphviz: ");
    strcat(name, base);

    normalhints = XAllocSizeHints();
    normalhints->flags = 0;
    normalhints->x = 0;
    normalhints->y = 0;
    normalhints->width = job->width;
    normalhints->height = job->height;

    classhint = XAllocClassHint();
    classhint->res_name = "graphviz";
    classhint->res_class = "Graphviz";

    wmhints = XAllocWMHints();
    wmhints->flags = InputHint;
    wmhints->input = True;

    Xutf8SetWMProperties(dpy, window->win, name, base, 0, 0,
                         normalhints, wmhints, classhint);
    XFree(wmhints);
    XFree(classhint);
    XFree(normalhints);
    free(name);

    window->pix = XCreatePixmap(dpy, window->win, job->width, job->height,
		window->depth);
    if (argb)
        gcv.foreground = 0;
    else
        gcv.foreground = WhitePixel(dpy, scr);
    window->gc = XCreateGC(dpy, window->pix, GCForeground, &gcv);
    update_display(job, dpy);

    window->event_mask = (
          ButtonPressMask
        | ButtonReleaseMask
        | PointerMotionMask
        | KeyPressMask
        | StructureNotifyMask
        | ExposureMask);
    XSelectInput(dpy, window->win, window->event_mask);
    window->wm_delete_window_atom =
        XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dpy, window->win, &window->wm_delete_window_atom, 1);
    XMapWindow(dpy, window->win);
}

static int handle_stdin_events(GVJ_t *job, int stdin_fd)
{
    int rc=0;

    if (feof(stdin))
	return -1;
    (job->callbacks->read)(job, job->input_filename, job->layout_type);
    
    rc++;
    return rc;
}

#ifdef HAVE_SYS_INOTIFY_H
static int handle_file_events(GVJ_t *job, int inotify_fd)
{
    int avail, ret, len, ln, rc = 0;
    static char *buf;
    char *bf, *p;
    struct inotify_event *event;

    ret = ioctl(inotify_fd, FIONREAD, &avail);
    if (ret < 0) {
	fprintf(stderr,"ioctl() failed\n");
	return -1;;
    }

    if (avail) {
        buf = realloc(buf, avail);
        if (!buf) {
            fprintf(stderr,"problem with realloc(%d)\n", avail);
            return -1;
        }
        len = read(inotify_fd, buf, avail);
        if (len != avail) {
            fprintf(stderr,"avail = %u, len = %u\n", avail, len);
            return -1;
        }
        bf = buf;
        while (len > 0) {
    	    event = (struct inotify_event *)bf;
	    switch (event->mask) {
	    case IN_MODIFY:
		p = strrchr(job->input_filename, '/');
		if (p)
		    p++;
		else 
		    p = job->input_filename;
		if (strcmp((char*)(&(event->name)), p) == 0) {
		    (job->callbacks->read)(job, job->input_filename, job->layout_type);
		    rc++;
		}
		break;

            case IN_ACCESS:
            case IN_ATTRIB:
            case IN_CLOSE_WRITE:
            case IN_CLOSE_NOWRITE:
            case IN_OPEN:
            case IN_MOVED_FROM:
            case IN_MOVED_TO:
            case IN_CREATE:
            case IN_DELETE:
            case IN_DELETE_SELF:
            case IN_MOVE_SELF:
            case IN_UNMOUNT:
            case IN_Q_OVERFLOW:
            case IN_IGNORED:
            case IN_ISDIR:
            case IN_ONESHOT:
		break;
    	    }
	    ln = event->len + 4 * sizeof(int);
            bf += ln;
            len -= ln;
        }
        if (len != 0) {
            fprintf(stderr,"length miscalculation, len = %d\n", len);
            return -1;
        }
    }
    return rc;
}
#endif

static void xlib_initialize(GVJ_t *firstjob)
{
    Display *dpy;
    KeySym keysym;
    KeyCode *keycodes;
    const char *display_name = NULL;
    int i, scr;

    dpy = XOpenDisplay(display_name);
    if (dpy == NULL) {
	fprintf(stderr, "Failed to open XLIB display: %s\n",
		XDisplayName(NULL));
	return;
    }
    scr = DefaultScreen(dpy);

    firstjob->display = (void*)dpy;
    firstjob->screen = scr;

    keycodes = (KeyCode *)malloc(firstjob->numkeys * sizeof(KeyCode));
    if (keycodes == NULL) {
        fprintf(stderr, "Failed to malloc %d*KeyCode\n", firstjob->numkeys);
        return;
    }
    for (i = 0; i < firstjob->numkeys; i++) {
        keysym = XStringToKeysym(firstjob->keybindings[i].keystring);
        if (keysym == NoSymbol)
            fprintf(stderr, "ERROR: No keysym for \"%s\"\n",
		firstjob->keybindings[i].keystring);
        else
            keycodes[i] = XKeysymToKeycode(dpy, keysym);
    }
    firstjob->keycodes = (void*)keycodes;

    firstjob->device_dpi.x = DisplayWidth(dpy, scr) * 25.4 / DisplayWidthMM(dpy, scr);
    firstjob->device_dpi.y = DisplayHeight(dpy, scr) * 25.4 / DisplayHeightMM(dpy, scr);
    firstjob->device_sets_dpi = TRUE;
}

static void xlib_finalize(GVJ_t *firstjob)
{
    GVJ_t *job;
    Display *dpy = (Display *)(firstjob->display);
    int scr = firstjob->screen;
    KeyCode *keycodes= firstjob->keycodes;
    int numfds, stdin_fd=0, xlib_fd, ret, events;
    fd_set rfds;
    boolean watching_stdin_p = FALSE;
#ifdef HAVE_SYS_INOTIFY_H
    int wd=0;
    int inotify_fd=0;
    boolean watching_file_p = FALSE;
    static char *dir;
    char *p, *cwd = NULL;

    inotify_fd = inotify_init();
    if (inotify_fd < 0) {
	fprintf(stderr,"inotify_init() failed\n");
	return;
    }
#endif

    numfds = xlib_fd = XConnectionNumber(dpy);

    if (firstjob->input_filename) {
        if (firstjob->graph_index == 0) {
#ifdef HAVE_SYS_INOTIFY_H
	    watching_file_p = TRUE;

	    if (firstjob->input_filename[0] != '/') {
    	        cwd = getcwd(NULL, 0);
	        dir = realloc(dir, strlen(cwd) + 1 + strlen(firstjob->input_filename) + 1);
	        strcpy(dir, cwd);
	        strcat(dir, "/");
	        strcat(dir, firstjob->input_filename);
	        free(cwd);
	    }
	    else {
	        dir = realloc(dir, strlen(firstjob->input_filename) + 1);
	        strcpy(dir, firstjob->input_filename);
	    }
	    p = strrchr(dir,'/');
	    *p = '\0';
    
    	    wd = inotify_add_watch(inotify_fd, dir, IN_MODIFY );

            numfds = MAX(inotify_fd, numfds);
#endif
	}
    }
    else {
	watching_stdin_p = TRUE;
	stdin_fd = fcntl(STDIN_FILENO, F_DUPFD, 0);
	numfds = MAX(stdin_fd, numfds);
    }

    for (job = firstjob; job; job = job->next_active)
	init_window(job, dpy, scr);

    /* This is the event loop */
    FD_ZERO(&rfds);
    while (1) {
	events = 0;

#ifdef HAVE_SYS_INOTIFY_H
	if (watching_file_p) {
	    if (FD_ISSET(inotify_fd, &rfds)) {
                ret = handle_file_events(firstjob, inotify_fd);
	        if (ret < 0)
	            break;
	        events += ret;
	    }
            FD_SET(inotify_fd, &rfds);
	}
#endif

	if (watching_stdin_p) {
	    if (FD_ISSET(stdin_fd, &rfds)) {
                ret = handle_stdin_events(firstjob, stdin_fd);
	        if (ret < 0)
	            watching_stdin_p = FALSE;
	        events += ret;
	    }
	    if (watching_stdin_p) 
	        FD_SET(stdin_fd, &rfds);
	}

	ret = handle_xlib_events(firstjob, dpy);
	if (ret < 0)
	    break;
	events += ret;
        FD_SET(xlib_fd, &rfds);

	if (events) {
            for (job = firstjob; job; job = job->next_active)
	        update_display(job, dpy);
	    XFlush(dpy);
	}

	ret = select(numfds+1, &rfds, NULL, NULL, NULL);
	if (ret < 0) {
	    fprintf(stderr,"select() failed\n");
	    break;
	}
    }

#ifdef HAVE_SYS_INOTIFY_H
    if (watching_file_p)
	ret = inotify_rm_watch(inotify_fd, wd);
#endif

    XCloseDisplay(dpy);
    free(keycodes);
    firstjob->keycodes = NULL;
}

static gvdevice_features_t device_features_xlib = {
    GVDEVICE_DOES_TRUECOLOR
	| GVDEVICE_EVENTS,      /* flags */
    {0.,0.},                    /* default margin - points */
    {0.,0.},                    /* default page width, height - points */
    {96.,96.},                  /* dpi */
};

static gvdevice_engine_t device_engine_xlib = {
    xlib_initialize,
    NULL,		/* xlib_format */
    xlib_finalize,
};
#endif

gvplugin_installed_t gvdevice_types_xlib[] = {
#ifdef CAIRO_HAS_XLIB_SURFACE
    {0, "xlib:cairo", 0, &device_engine_xlib, &device_features_xlib},
    {0, "x11:cairo", 0, &device_engine_xlib, &device_features_xlib},
#endif
    {0, NULL, 0, NULL, NULL}
};
