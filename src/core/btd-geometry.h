/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Big Tabbed Dock — where the shelf may lie next to Window Maker's Dock and
 * Clip (X11).
 *
 * The Dock and the Clip are columns of square override-redirect tiles on
 * the sides of a monitor, and Window Maker puts them where it likes,
 * ignoring the space the shelf reserves: a tile inside the shelf would be
 * out of reach under it, so the shelf leaves those columns free.
 *
 * The application icons (appicons) and miniwindows look just the same
 * (square, override-redirect, on the side of the screen), but Window Maker
 * places them inside the work area, that is, outside the space the shelf
 * reserves. So only a tile that enters the shelf's body (the opaque strip
 * that is also the reserved strut, without the transparent margin above
 * it) counts: an appicon resting on top of the shelf does not take a tile's
 * width away from "full width".
 *
 * Only plain integers here, so it is unit-tested without X.
 */
#pragma once

#include <glib.h>

G_BEGIN_DECLS

typedef struct {
    int x, y, width, height;
} BtdRect;

/* The smallest and largest Window Maker icon sizes (IconSize). */
#define BTD_WM_TILE_MIN 24
#define BTD_WM_TILE_MAX 160

/* Is a viewable override-redirect window of this size a Window Maker tile? */
gboolean btd_wm_tile_like(int width, int height);

/*
 * The widths to leave free on each side of `body` (the shelf's strip on its
 * monitor, full monitor width) for the tiles in `tiles` that enter it.
 */
void btd_wm_dock_margins(const BtdRect *body, const BtdRect *tiles, guint n_tiles, int *left, int *right);

G_END_DECLS
