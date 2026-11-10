/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "btd-geometry.h"

gboolean
btd_wm_tile_like(int width, int height)
{
    return width == height && width >= BTD_WM_TILE_MIN && width <= BTD_WM_TILE_MAX;
}

void
btd_wm_dock_margins(const BtdRect *body, const BtdRect *tiles, guint n_tiles, int *left, int *right)
{
    *left = *right = 0;
    if (!body || body->width <= 0 || body->height <= 0)
        return;
    for (guint i = 0; i < n_tiles; i++) {
        const BtdRect *t = &tiles[i];
        if (!btd_wm_tile_like(t->width, t->height))
            continue;
        /* must enter the body: touching its edge is not entering it */
        if (t->y + t->height <= body->y || t->y >= body->y + body->height)
            continue;
        if (t->x + t->width <= body->x || t->x >= body->x + body->width)
            continue;
        /* a column on a side (a couple of pixels of slack for borders) */
        if (t->x <= body->x + 2)
            *left = MAX(*left, t->x + t->width - body->x);
        else if (t->x + t->width >= body->x + body->width - 2)
            *right = MAX(*right, body->x + body->width - t->x);
    }
}
