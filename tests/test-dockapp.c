/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * The dock-app controller without X11: the arrows' order, the tile's
 * regions, the preferences it changes, --set, and the files of wlmaker's
 * menu.
 */
#include "btd-dockapp.h"

#include <glib/gstdio.h>

static void
test_align_order(void)
{
    /* ▶: fill → right → center → left → fill */
    const BtdAlign next[] = { BTD_ALIGN_RIGHT, BTD_ALIGN_CENTER, BTD_ALIGN_LEFT, BTD_ALIGN_FILL };
    BtdAlign a = BTD_ALIGN_FILL;
    for (guint i = 0; i < G_N_ELEMENTS(next); i++) {
        a = btd_align_step(a, 1);
        g_assert_cmpint(a, ==, next[i]);
    }
    /* ◀: the other way round */
    const BtdAlign prev[] = { BTD_ALIGN_LEFT, BTD_ALIGN_CENTER, BTD_ALIGN_RIGHT, BTD_ALIGN_FILL };
    for (guint i = 0; i < G_N_ELEMENTS(prev); i++) {
        a = btd_align_step(a, -1);
        g_assert_cmpint(a, ==, prev[i]);
    }
    /* the examples of the specification */
    g_assert_cmpint(btd_align_step(BTD_ALIGN_FILL, 1), ==, BTD_ALIGN_RIGHT);
    g_assert_cmpint(btd_align_step(BTD_ALIGN_FILL, -1), ==, BTD_ALIGN_LEFT);
    g_assert_cmpint(btd_align_step(BTD_ALIGN_CENTER, 1), ==, BTD_ALIGN_LEFT);
    g_assert_cmpint(btd_align_step(BTD_ALIGN_CENTER, -1), ==, BTD_ALIGN_RIGHT);
    g_assert_cmpint(btd_align_step(BTD_ALIGN_LEFT, 5), ==, BTD_ALIGN_FILL);
    g_assert_cmpint(btd_align_step((BtdAlign)42, 1), ==, BTD_ALIGN_RIGHT); /* unknown: from fill */
}

static void
test_strings(void)
{
    BtdAlign a = BTD_ALIGN_CENTER;
    BtdEdge e = BTD_EDGE_BOTTOM;
    g_assert_true(btd_align_from_string("fill", &a) && a == BTD_ALIGN_FILL);
    g_assert_true(btd_align_from_string("RIGHT", &a) && a == BTD_ALIGN_RIGHT);
    g_assert_false(btd_align_from_string("diagonal", &a));
    g_assert_cmpint(a, ==, BTD_ALIGN_RIGHT); /* untouched */
    g_assert_true(btd_edge_from_string("top", &e) && e == BTD_EDGE_TOP);
    g_assert_false(btd_edge_from_string("", &e));
    g_assert_false(btd_edge_from_string(NULL, &e));
}

static void
test_regions(void)
{
    /* the display (the icon) on top; under it ◀ ▶, the lamp, ▲ ▼ */
    g_assert_cmpint(btd_dockapp_region_at(32, 24), ==, BTD_REGION_ICON);
    g_assert_cmpint(btd_dockapp_region_at(5, 5), ==, BTD_REGION_ICON);
    g_assert_cmpint(btd_dockapp_region_at(59, 44), ==, BTD_REGION_ICON);
    g_assert_cmpint(btd_dockapp_region_at(9, 53), ==, BTD_REGION_LEFT);
    g_assert_cmpint(btd_dockapp_region_at(21, 53), ==, BTD_REGION_RIGHT);
    g_assert_cmpint(btd_dockapp_region_at(41, 53), ==, BTD_REGION_TOP);
    g_assert_cmpint(btd_dockapp_region_at(53, 53), ==, BTD_REGION_BOTTOM);
    /* the margin, the gap under the display and the lamp: no control (a
     * click there does nothing, a drag moves the tile) */
    g_assert_cmpint(btd_dockapp_region_at(1, 1), ==, BTD_REGION_NONE);
    g_assert_cmpint(btd_dockapp_region_at(32, 45), ==, BTD_REGION_NONE);
    g_assert_cmpint(btd_dockapp_region_at(32, 46), ==, BTD_REGION_NONE);
    g_assert_cmpint(btd_dockapp_region_at(32, 53), ==, BTD_REGION_NONE);
    g_assert_cmpint(btd_dockapp_region_at(63, 63), ==, BTD_REGION_NONE);
    /* every region inside the tile's 4 px margin, none overlapping, buttons
     * at least 12 px each way; the display is most of the tile */
    const BtdRegion all[] = { BTD_REGION_TOP, BTD_REGION_BOTTOM, BTD_REGION_LEFT, BTD_REGION_RIGHT, BTD_REGION_ICON };
    for (guint i = 0; i < G_N_ELEMENTS(all); i++) {
        BtdRect r;
        btd_dockapp_region_rect(all[i], &r);
        g_assert_cmpint(r.x, >=, 4);
        g_assert_cmpint(r.y, >=, 4);
        g_assert_cmpint(r.x + r.width, <=, BTD_DOCKAPP_SIZE - 4);
        g_assert_cmpint(r.y + r.height, <=, BTD_DOCKAPP_SIZE - 4);
        g_assert_cmpint(MIN(r.width, r.height), >=, 12);
        for (int y = r.y; y < r.y + r.height; y++)
            for (int x = r.x; x < r.x + r.width; x++)
                g_assert_cmpint(btd_dockapp_region_at(x, y), ==, all[i]);
    }
    BtdRect w, ir;
    btd_dockapp_region_rect(BTD_REGION_ICON, &w);
    g_assert_cmpint(w.width * w.height, >=, 56 * 40);
    /* the icon, 44 px wide (it was 22), inside the display */
    btd_dockapp_icon_rect(&ir);
    g_assert_cmpint(ir.width, >=, 42);
    g_assert_cmpint(ir.x, >=, w.x);
    g_assert_cmpint(ir.y, >=, w.y);
    g_assert_cmpint(ir.x + ir.width, <=, w.x + w.width);
    g_assert_cmpint(ir.y + ir.height, <=, w.y + w.height);
}

static void
test_bar(void)
{
    int x, w;
    btd_dockapp_bar_geometry(BTD_ALIGN_FILL, 10, 30, &x, &w);
    g_assert_cmpint(x, ==, 10);
    g_assert_cmpint(w, ==, 30);
    btd_dockapp_bar_geometry(BTD_ALIGN_LEFT, 10, 30, &x, &w);
    g_assert_cmpint(x, ==, 10);
    btd_dockapp_bar_geometry(BTD_ALIGN_RIGHT, 10, 30, &x, &w);
    g_assert_cmpint(x + w, ==, 40);
    btd_dockapp_bar_geometry(BTD_ALIGN_CENTER, 10, 30, &x, &w);
    g_assert_cmpint(x - 10, ==, 40 - (x + w));
}

static void
test_apply_to_prefs(void)
{
    BtdPrefs p;
    btd_prefs_init_defaults(&p);
    g_assert_true(btd_dockapp_apply_to_prefs(&p, "alignment", g_variant_new_string("left")));
    g_assert_cmpint(p.align, ==, BTD_ALIGN_LEFT);
    g_assert_true(btd_dockapp_apply_to_prefs(&p, "edge", g_variant_new_string("top")));
    g_assert_cmpint(p.edge, ==, BTD_EDGE_TOP);
    g_assert_true(btd_dockapp_apply_to_prefs(&p, "autohide", g_variant_new_boolean(TRUE)));
    g_assert_true(p.autohide);
    g_assert_true(btd_dockapp_apply_to_prefs(&p, "reserve-space", g_variant_new_boolean(FALSE)));
    g_assert_false(p.reserve_space);
    g_assert_true(btd_dockapp_apply_to_prefs(&p, "monitor", g_variant_new_string("HDMI-A-1")));
    g_assert_cmpstr(p.monitor, ==, "HDMI-A-1");
    g_assert_true(btd_dockapp_apply_to_prefs(&p, "style", g_variant_new_string("tiles")));
    g_assert_cmpint(p.style, ==, BTD_STYLE_TILES);
    /* invalid values and read-only states leave the preferences alone */
    g_assert_false(btd_dockapp_apply_to_prefs(&p, "alignment", g_variant_new_string("diagonal")));
    g_assert_false(btd_dockapp_apply_to_prefs(&p, "edge", g_variant_new_boolean(TRUE)));
    g_assert_false(btd_dockapp_apply_to_prefs(&p, "monitor", g_variant_new_string("")));
    g_assert_false(btd_dockapp_apply_to_prefs(&p, "visible", g_variant_new_boolean(FALSE)));
    g_assert_false(btd_dockapp_apply_to_prefs(&p, "nonsense", g_variant_new_string("x")));
    g_assert_cmpint(p.align, ==, BTD_ALIGN_LEFT);
    g_assert_cmpint(p.edge, ==, BTD_EDGE_TOP);
    g_assert_cmpstr(p.monitor, ==, "HDMI-A-1");
    btd_prefs_clear(&p);
}

static void
test_parse_setting(void)
{
    const struct { const char *in, *key; gboolean ok; } cases[] = {
        { "alignment=center", "alignment", TRUE }, { "edge=top", "edge", TRUE },
        { "autohide=yes", "autohide", TRUE },      { "reserve-space=0", "reserve-space", TRUE },
        { "monitor=DP-2", "monitor", TRUE },       { "style=modern", "style", TRUE },
        { "alignment=up", NULL, FALSE },           { "autohide=maybe", NULL, FALSE },
        { "=center", NULL, FALSE },                { "alignment", NULL, FALSE },
        { "monitor=", NULL, FALSE },               { "colour=red", NULL, FALSE },
    };
    for (guint i = 0; i < G_N_ELEMENTS(cases); i++) {
        char *key = NULL;
        GVariant *v = NULL;
        gboolean ok = btd_dockapp_parse_setting(cases[i].in, &key, &v);
        g_assert_cmpint(ok, ==, cases[i].ok);
        if (ok) {
            g_assert_cmpstr(key, ==, cases[i].key);
            g_free(key);
            g_variant_unref(v);
        }
    }
}

static void
test_wlmaker_files(void)
{
    BtdDockappState st = { .align = BTD_ALIGN_RIGHT, .edge = BTD_EDGE_TOP, .style = BTD_STYLE_CLASSIC,
                           .autohide = FALSE, .reserve_space = TRUE };
    st.monitor = g_strdup("DP-2");
    char *text = btd_dockapp_wlm_prefs_text(&st);
    g_assert_nonnull(strstr(text, "Alignment = right;"));
    g_assert_nonnull(strstr(text, "Edge = top;"));
    g_assert_nonnull(strstr(text, "Monitor = DP-2;"));
    g_assert_nonnull(strstr(text, "ReserveSpace = Yes;"));
    /* what we wrote: nothing to change */
    GPtrArray *c = btd_dockapp_wlm_changes(&st, text);
    g_assert_cmpuint(c->len, ==, 0);
    g_ptr_array_unref(c);
    /* what wlmaker writes after a pick (it re-sorts and may quote) */
    const char *picked = "{\n  Alignment = center;\n  AutoHide = Yes;\n  Edge = top;\n"
                         "  Monitor = \"HDMI-A-1\";\n  ReserveSpace = Yes;\n  Style = classic;\n}\n";
    c = btd_dockapp_wlm_changes(&st, picked);
    g_assert_cmpuint(c->len, ==, 3);
    gboolean saw_align = FALSE, saw_hide = FALSE, saw_mon = FALSE;
    for (guint i = 0; i < c->len; i++) {
        BtdDockappSetting *s = c->pdata[i];
        if (g_str_equal(s->key, "alignment"))
            saw_align = g_str_equal(g_variant_get_string(s->value, NULL), "center");
        else if (g_str_equal(s->key, "autohide"))
            saw_hide = g_variant_get_boolean(s->value);
        else if (g_str_equal(s->key, "monitor"))
            saw_mon = g_str_equal(g_variant_get_string(s->value, NULL), "HDMI-A-1");
    }
    g_assert_true(saw_align && saw_hide && saw_mon);
    g_ptr_array_unref(c);
    /* garbage is ignored */
    c = btd_dockapp_wlm_changes(&st, "{ Alignment = diagonal; Edge = ; nonsense }");
    g_assert_cmpuint(c->len, ==, 0);
    g_ptr_array_unref(c);

    /* the monitors: the dock's list, and the chosen one even when unplugged */
    char *choices = btd_dockapp_wlm_choices_text(&st);
    g_assert_nonnull(strstr(choices, "(primary, \"Primary monitor\")"));
    g_assert_nonnull(strstr(choices, "(DP-2, DP-2)"));
    g_free(choices);
    const char *const list[] = { "primary\tPrimary monitor", "eDP-1\teDP-1 (1920×1080)", "DP-2\tDP-2 (2560×1440)",
                                 NULL };
    st.monitors = g_strdupv((char **)list);
    char **mc = btd_dockapp_monitor_choices(&st);
    g_assert_cmpuint(g_strv_length(mc), ==, 3);
    g_strfreev(mc);
    g_free(text);
    btd_dockapp_state_clear(&st);
}

/* Persistence: what the arrows set survives a save and a load (as when the
 * controller or the dock restarts), and a bad value falls back safely. */
static void
test_persistence(void)
{
    char *dir = g_dir_make_tmp("btd-dockapp-XXXXXX", NULL);
    char *path = g_build_filename(dir, "dock.conf", NULL);
    BtdModel *m = btd_model_new();
    btd_model_reset_builtin_tabs(m);
    BtdPrefs p;
    btd_prefs_init_defaults(&p);
    g_assert_true(btd_dockapp_apply_to_prefs(&p, "alignment", g_variant_new_string("left")));
    g_assert_true(btd_dockapp_apply_to_prefs(&p, "edge", g_variant_new_string("top")));
    g_assert_true(btd_config_save(path, m, &p, NULL));
    BtdModel *m2 = btd_model_new();
    BtdPrefs p2;
    btd_prefs_init_defaults(&p2);
    g_assert_cmpint(btd_config_load(path, m2, &p2, NULL), ==, BTD_LOAD_OK);
    g_assert_cmpint(p2.align, ==, BTD_ALIGN_LEFT);
    g_assert_cmpint(p2.edge, ==, BTD_EDGE_TOP);
    g_assert_cmpuint(btd_model_n_tabs(m2), ==, btd_model_n_tabs(m));
    /* an unknown value: the default for it, the rest kept */
    char *data = NULL;
    g_assert_true(g_file_get_contents(path, &data, NULL, NULL));
    GString *s = g_string_new(data);
    g_string_replace(s, "Alignment=left", "Alignment=diagonal", 0);
    g_assert_true(g_file_set_contents(path, s->str, -1, NULL));
    btd_prefs_clear(&p2);
    btd_prefs_init_defaults(&p2);
    btd_model_free(m2);
    m2 = btd_model_new();
    btd_config_load(path, m2, &p2, NULL);
    BtdPrefs def;
    btd_prefs_init_defaults(&def);
    g_assert_cmpint(p2.align, ==, def.align);
    g_assert_cmpint(p2.edge, ==, BTD_EDGE_TOP);
    btd_prefs_clear(&def);
    g_string_free(s, TRUE);
    g_free(data);
    btd_prefs_clear(&p);
    btd_prefs_clear(&p2);
    btd_model_free(m);
    btd_model_free(m2);
    g_unlink(path);
    char *bak = g_strconcat(path, ".bak", NULL);
    g_unlink(bak);
    g_free(bak);
    g_rmdir(dir);
    g_free(path);
    g_free(dir);
}

int
main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/dockapp/align-order", test_align_order);
    g_test_add_func("/dockapp/strings", test_strings);
    g_test_add_func("/dockapp/regions", test_regions);
    g_test_add_func("/dockapp/bar", test_bar);
    g_test_add_func("/dockapp/apply-to-prefs", test_apply_to_prefs);
    g_test_add_func("/dockapp/parse-setting", test_parse_setting);
    g_test_add_func("/dockapp/wlmaker-files", test_wlmaker_files);
    g_test_add_func("/dockapp/persistence", test_persistence);
    return g_test_run();
}
