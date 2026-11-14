/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * The window tracker keeps a window that left the table alive until the
 * changed callback has run: the dock's groups point to it until then, and a
 * frame drawn in between used to read freed memory (crash in g_str_hash
 * under the draw handler).
 */
#include "btd-windows.h"

typedef struct {
    int calls;
    guint listed;
    gboolean retired_readable;
} Seen;

static BtdWindow *retired_window;

static void
on_changed(BtdWindowTracker *t, gpointer data)
{
    Seen *seen = data;
    seen->calls++;
    GList *l = btd_window_tracker_list(t);
    seen->listed = g_list_length(l);
    g_list_free(l);
    /* the window that left is not listed, but still intact for the groups
     * built before this callback */
    seen->retired_readable = retired_window && g_strcmp0(retired_window->wm_class, "calamares") == 0;
}

static void
test_window_outlives_removal_until_changed(void)
{
    BtdWindowTracker *t = g_new0(BtdWindowTracker, 1);
    btd_window_tracker_init_base(t, "test", 0);
    Seen seen = { 0 };
    btd_window_tracker_set_changed_func(t, on_changed, &seen);

    BtdWindow *keep = btd_window_new(t, 1);
    btd_window_set_string(&keep->wm_class, "xterm", NULL);
    BtdWindow *gone = btd_window_new(t, 2);
    btd_window_set_string(&gone->wm_class, "calamares", NULL);
    retired_window = gone;

    guint64 id = 2;
    g_hash_table_remove(t->windows, &id); /* destroyed, or WM_CLASS re-read */
    g_assert_cmpuint(g_hash_table_size(t->windows), ==, 1);
    g_assert_cmpuint(t->retired->len, ==, 1);
    g_assert_cmpstr(gone->wm_class, ==, "calamares"); /* a redraw now is safe */
    g_assert_cmpuint(t->idle_id, !=, 0);              /* the dock will be told */

    while (t->idle_id)
        g_main_context_iteration(NULL, TRUE);
    g_assert_cmpint(seen.calls, ==, 1);
    g_assert_cmpuint(seen.listed, ==, 1);
    g_assert_true(seen.retired_readable);
    g_assert_cmpuint(t->retired->len, ==, 0); /* freed after the callback */
    retired_window = NULL;

    btd_window_tracker_free(t);
}

int
main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/windows/outlives-removal-until-changed", test_window_outlives_removal_until_changed);
    return g_test_run();
}
