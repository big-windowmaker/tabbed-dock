/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Stand-ins for Window Maker's icons in the tests: square override-redirect
 * windows (Dock and Clip tiles, appicons) at given places, until killed.
 *
 *   x11-tile SIZE X,Y [X,Y...]
 *
 * Prints "ready" once every tile is mapped.
 */
#include <X11/Xlib.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int
main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "usage: x11-tile SIZE X,Y [X,Y...]\n");
        return 2;
    }
    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "x11-tile: no display\n");
        return 1;
    }
    int size = atoi(argv[1]);
    XSetWindowAttributes a = { 0 };
    a.override_redirect = True;
    a.background_pixel = BlackPixel(dpy, DefaultScreen(dpy));
    for (int i = 2; i < argc; i++) {
        int x, y;
        if (sscanf(argv[i], "%d,%d", &x, &y) != 2)
            return 2;
        Window w = XCreateWindow(dpy, DefaultRootWindow(dpy), x, y, size, size, 0, CopyFromParent, InputOutput,
                                 CopyFromParent, CWOverrideRedirect | CWBackPixel, &a);
        XMapRaised(dpy, w);
    }
    XSync(dpy, False);
    printf("ready\n");
    fflush(stdout);
    for (;;)
        pause();
}
