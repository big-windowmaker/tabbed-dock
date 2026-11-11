/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * The dock-app's face, drawn with Cairo, without X11 (tested in
 * tests/test-dockapp-draw.c on image surfaces).
 */
#pragma once

#include <cairo.h>
#include <librsvg/rsvg.h>

#include "btd-dockapp.h"

G_BEGIN_DECLS

typedef struct {
    const BtdDockappState *st;
    gboolean dark;    /* Big WindowMaker's dark theme */
    gboolean running; /* the dock runs (else the file is shown) */
    BtdRegion pressed; /* the button held down */
    BtdRegion hover;   /* the button under the pointer */
} BtdDockappLook;

/* tabbed-dock.svg rendered once at its size in the display
 * (BTD_DOCKAPP_ICON_W × _H, ARGB); NULL without the SVG. */
cairo_surface_t *btd_dockapp_icon_new(RsvgHandle *svg);

/* The whole 64×64 face. Outside the shape (btd_dockapp_shape) it paints
 * black, never shown. icon may be NULL. */
void btd_dockapp_draw(cairo_t *cr, const BtdDockappLook *look, cairo_surface_t *icon);

G_END_DECLS
