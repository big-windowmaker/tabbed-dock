/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Where the full-width shelf lies next to Window Maker's Dock and Clip.
 *
 * The numbers are those measured on a real session (2026-11-10): middle
 * monitor 3440x1440 at x=3440, IconSize 48, shelf at the bottom with a
 * 112-pixel body (window 165 tall, y=1275), the Clip's column on the left
 * (y 0..624), the Dock's on the right (y 0..816) and the Terminal's appicon
 * at (3440,1280), on top of the shelf: it took 48 pixels off "full width".
 */
#include "btd-geometry.h"

#define MON_X 3440
#define MON_W 3440
#define BODY_H 112
#define BODY_Y (1440 - BODY_H)

static const BtdRect body = { MON_X, BODY_Y, MON_W, BODY_H };

static void
test_appicon_on_top(void)
{
    /* the Clip (top-left), the Dock (top-right) and an appicon resting on
     * the shelf: none enters the body */
    BtdRect tiles[] = {
        { MON_X, 0, 48, 48 },
        { MON_X, 576, 48, 48 },
        { MON_X + MON_W - 48, 0, 48, 48 },
        { MON_X + MON_W - 48, 768, 48, 48 },
        { MON_X, 1280, 48, 48 }, /* appicon: bottom at 1328 = body top */
    };
    int l = -1, r = -1;
    btd_wm_dock_margins(&body, tiles, G_N_ELEMENTS(tiles), &l, &r);
    g_assert_cmpint(l, ==, 0);
    g_assert_cmpint(r, ==, 0);
}

static void
test_several_appicons(void)
{
    /* a row of appicons (IconPosition "bottom left, horizontal") and one
     * more row above: still full width */
    BtdRect tiles[8];
    for (int i = 0; i < 8; i++)
        tiles[i] = (BtdRect){ MON_X + (i % 4) * 48, 1280 - (i / 4) * 48, 48, 48 };
    int l, r;
    btd_wm_dock_margins(&body, tiles, G_N_ELEMENTS(tiles), &l, &r);
    g_assert_cmpint(l, ==, 0);
    g_assert_cmpint(r, ==, 0);
}

static void
test_dock_column_reaches_the_shelf(void)
{
    /* the Dock grows down into the shelf's body: its column stays free */
    BtdRect tiles[] = {
        { MON_X + MON_W - 64, 1344, 64, 64 },
        { MON_X + MON_W - 64, 1408, 64, 64 },
    };
    int l, r;
    btd_wm_dock_margins(&body, tiles, G_N_ELEMENTS(tiles), &l, &r);
    g_assert_cmpint(l, ==, 0);
    g_assert_cmpint(r, ==, 64);
}

static void
test_clip_in_the_corner(void)
{
    /* the Clip moved to the bottom-left corner, plus an appicon above it */
    BtdRect tiles[] = {
        { MON_X, 1440 - 48, 48, 48 },
        { MON_X, 1280, 48, 48 },
    };
    int l, r;
    btd_wm_dock_margins(&body, tiles, G_N_ELEMENTS(tiles), &l, &r);
    g_assert_cmpint(l, ==, 48);
    g_assert_cmpint(r, ==, 0);
}

static void
test_shelf_on_top(void)
{
    /* edge = top: the Dock's first tiles are in the body */
    BtdRect top = { MON_X, 0, MON_W, BODY_H };
    BtdRect tiles[] = {
        { MON_X + MON_W - 64, 0, 64, 64 },
        { MON_X + MON_W - 64, 64, 64, 64 },
        { MON_X + MON_W - 64, 128, 64, 64 }, /* below the body */
        { MON_X, 112, 64, 64 },              /* touching the body's bottom */
    };
    int l, r;
    btd_wm_dock_margins(&top, tiles, G_N_ELEMENTS(tiles), &l, &r);
    g_assert_cmpint(l, ==, 0);
    g_assert_cmpint(r, ==, 64);
}

static void
test_not_tiles(void)
{
    /* not square, too small, too big, in the middle, on another monitor */
    BtdRect tiles[] = {
        { MON_X, 1400, 300, 40 },
        { MON_X, 1420, 16, 16 },
        { MON_X, 1300, 200, 200 },
        { MON_X + 1000, 1392, 48, 48 },
        { 0, 1392, 48, 48 },
        { MON_X + MON_W, 1392, 48, 48 },
    };
    int l, r;
    btd_wm_dock_margins(&body, tiles, G_N_ELEMENTS(tiles), &l, &r);
    g_assert_cmpint(l, ==, 0);
    g_assert_cmpint(r, ==, 0);
    g_assert_true(btd_wm_tile_like(48, 48));
    g_assert_true(btd_wm_tile_like(24, 24));
    g_assert_false(btd_wm_tile_like(48, 49));
    g_assert_false(btd_wm_tile_like(161, 161));
}

int
main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/geometry/appicon-on-top", test_appicon_on_top);
    g_test_add_func("/geometry/several-appicons", test_several_appicons);
    g_test_add_func("/geometry/dock-column-reaches-the-shelf", test_dock_column_reaches_the_shelf);
    g_test_add_func("/geometry/clip-in-the-corner", test_clip_in_the_corner);
    g_test_add_func("/geometry/shelf-on-top", test_shelf_on_top);
    g_test_add_func("/geometry/not-tiles", test_not_tiles);
    return g_test_run();
}
