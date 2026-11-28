/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Icon of a running application with no .desktop (btd_icon_cache_lookup_windows):
 * a theme icon named after the window (class in lower case here), else the
 * window's own icon (X11 _NET_WM_ICON), else the generic icon. Before, every
 * such application showed the generic "exec" icon in the dock.
 */
#include <glib/gstdio.h>

#include "btd-icons.h"

#define SIZE 48

static char *theme_dir;

/* the colour at the centre of a drawn icon, as 0xRRGGBB (opaque pixels) */
static guint32
centre(cairo_surface_t *s)
{
    cairo_surface_flush(s);
    int w = cairo_image_surface_get_width(s), h = cairo_image_surface_get_height(s);
    const guint32 *px =
        (const guint32 *)(cairo_image_surface_get_data(s) + (h / 2) * cairo_image_surface_get_stride(s));
    return px[w / 2] & 0xFFFFFF;
}

static cairo_surface_t *
square(double r, double g, double b, int side)
{
    cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, side, side);
    cairo_t *cr = cairo_create(s);
    cairo_set_source_rgb(cr, r, g, b);
    cairo_paint(cr);
    cairo_destroy(cr);
    return s;
}

static BtdIconCache *
make_cache(void)
{
    GtkIconTheme *theme = gtk_icon_theme_new();
    gtk_icon_theme_set_search_path(theme, (const char *[]){ theme_dir }, 1);
    gtk_icon_theme_set_custom_theme(theme, "teste");
    BtdIconCache *c = btd_icon_cache_new(theme, BTD_ICON_CACHE_MAX);
    g_object_unref(theme);
    return c;
}

static void
test_theme_icon_by_class(void)
{
    BtdIconCache *c = make_cache();
    BtdWindow w = { .id = 1, .wm_class = "MinhaClasse", .wm_instance = "minha" };
    GPtrArray *ws = g_ptr_array_new();
    g_ptr_array_add(ws, &w);
    /* "MinhaClasse" is not in the theme; "minhaclasse" (green) is */
    g_assert_cmphex(centre(btd_icon_cache_lookup_windows(c, ws, SIZE, 1)), ==, 0x00FF00);
    g_ptr_array_unref(ws);
    btd_icon_cache_free(c);
}

static void
test_window_own_icon(void)
{
    BtdIconCache *c = make_cache();
    BtdWindow w = { .id = 2, .wm_class = "SemIcone", .icon = square(1, 0, 0, 16), .icon_serial = 7 };
    GPtrArray *ws = g_ptr_array_new();
    g_ptr_array_add(ws, &w);
    cairo_surface_t *s = btd_icon_cache_lookup_windows(c, ws, SIZE, 2);
    /* scaled to the icon size at the monitor scale */
    g_assert_cmpint(cairo_image_surface_get_width(s), ==, SIZE * 2);
    g_assert_cmphex(centre(s), ==, 0xFF0000);
    /* a new icon on the same window is drawn again, not taken from the cache */
    cairo_surface_destroy(w.icon);
    w.icon = square(0, 0, 1, 16);
    w.icon_serial = 8;
    g_assert_cmphex(centre(btd_icon_cache_lookup_windows(c, ws, SIZE, 2)), ==, 0x0000FF);
    cairo_surface_destroy(w.icon);
    g_ptr_array_unref(ws);
    btd_icon_cache_free(c);
}

static void
test_nothing_falls_back_to_generic(void)
{
    BtdIconCache *c = make_cache();
    BtdWindow w = { .id = 3, .app_id = "nada.por.aqui" };
    GPtrArray *ws = g_ptr_array_new();
    g_ptr_array_add(ws, &w);
    cairo_surface_t *s = btd_icon_cache_lookup_windows(c, ws, SIZE, 1);
    g_assert_nonnull(s);
    g_assert_true(s == btd_icon_cache_lookup_name(c, NULL, SIZE, 1));
    g_assert_cmphex(centre(s), ==, 0x808080);
    g_ptr_array_unref(ws);
    btd_icon_cache_free(c);
}

int
main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);
    theme_dir = g_dir_make_tmp("btd-icons-XXXXXX", NULL);
    char *apps = g_build_filename(theme_dir, "teste", "48x48", "apps", NULL);
    g_mkdir_with_parents(apps, 0700);
    char *index = g_build_filename(theme_dir, "teste", "index.theme", NULL);
    g_file_set_contents(index,
                        "[Icon Theme]\nName=teste\nDirectories=48x48/apps\n\n"
                        "[48x48/apps]\nSize=48\nType=Fixed\n",
                        -1, NULL);
    char *png = g_build_filename(apps, "minhaclasse.png", NULL);
    cairo_surface_t *green = square(0, 1, 0, 48);
    cairo_surface_write_to_png(green, png);
    cairo_surface_destroy(green);
    /* the generic icon (gray), as every real theme has */
    char *generic = g_build_filename(apps, "application-x-executable.png", NULL);
    cairo_surface_t *gray = square(0.5, 0.5, 0.5, 48);
    cairo_surface_write_to_png(gray, generic);
    cairo_surface_destroy(gray);

    g_test_add_func("/icons/theme-icon-by-class", test_theme_icon_by_class);
    g_test_add_func("/icons/window-own-icon", test_window_own_icon);
    g_test_add_func("/icons/nothing-generic", test_nothing_falls_back_to_generic);
    int r = g_test_run();

    g_unlink(png);
    g_unlink(generic);
    g_free(generic);
    g_unlink(index);
    g_rmdir(apps);
    char *size_dir = g_path_get_dirname(apps);
    g_rmdir(size_dir);
    char *teste = g_path_get_dirname(size_dir);
    g_rmdir(teste);
    g_rmdir(theme_dir);
    g_free(teste);
    g_free(size_dir);
    g_free(png);
    g_free(index);
    g_free(apps);
    g_free(theme_dir);
    return r;
}
