/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * The dock-app controller without X11 or GTK: the tile's regions, its
 * state, the texts it shows, and the files of wlmaker's right-click menu.
 */
#include "btd-dockapp.h"

#include <glib/gi18n.h>
#include <string.h>

/* ---- geometry ------------------------------------------------------------- *
 *
 *  4     16 17                 47 48     60
 *  ┌─────────────────── ↑ ─────────────────┐ 4
 *  └───────────────────────────────────────┘ 16
 *  ┌──────┐ ┌─────────────────┐ ┌──────┐     17
 *  │  ◀   │ │ tabbed-dock.svg │ │  ▶   │
 *  └──────┘ └─────────────────┘ └──────┘     47
 *  ┌─────────────────── ↓ ─────────────────┐ 48
 *  └───────────────────────────────────────┘ 60
 *
 * 4 px around the tile, like the other dock-apps (the Dock's tile shows
 * there and in the 1 px gaps), buttons 12 px thick: big enough to hit. */
void
btd_dockapp_region_rect(BtdRegion region, BtdRect *r)
{
    switch (region) {
    case BTD_REGION_TOP:
        *r = (BtdRect){ 4, 4, 56, 12 };
        break;
    case BTD_REGION_BOTTOM:
        *r = (BtdRect){ 4, 48, 56, 12 };
        break;
    case BTD_REGION_LEFT:
        *r = (BtdRect){ 4, 17, 12, 30 };
        break;
    case BTD_REGION_RIGHT:
        *r = (BtdRect){ 48, 17, 12, 30 };
        break;
    case BTD_REGION_ICON:
        *r = (BtdRect){ 17, 17, 30, 30 };
        break;
    default:
        *r = (BtdRect){ 0, 0, 0, 0 };
        break;
    }
}

BtdRegion
btd_dockapp_region_at(int x, int y)
{
    static const BtdRegion all[] = { BTD_REGION_TOP, BTD_REGION_BOTTOM, BTD_REGION_LEFT, BTD_REGION_RIGHT,
                                     BTD_REGION_ICON };
    for (guint i = 0; i < G_N_ELEMENTS(all); i++) {
        BtdRect r;
        btd_dockapp_region_rect(all[i], &r);
        if (x >= r.x && x < r.x + r.width && y >= r.y && y < r.y + r.height)
            return all[i];
    }
    return BTD_REGION_NONE;
}

void
btd_dockapp_bar_geometry(BtdAlign align, int x0, int width, int *bx, int *bw)
{
    int third = width / 3;
    switch (align) {
    case BTD_ALIGN_FILL:
        *bx = x0;
        *bw = width;
        break;
    case BTD_ALIGN_LEFT:
        *bx = x0;
        *bw = third;
        break;
    case BTD_ALIGN_RIGHT:
        *bx = x0 + width - third;
        *bw = third;
        break;
    default:
        *bx = x0 + (width - third) / 2;
        *bw = third;
        break;
    }
}

/* ---- state ------------------------------------------------------------------ */

void
btd_dockapp_state_clear(BtdDockappState *st)
{
    g_clear_pointer(&st->monitor, g_free);
    g_clear_pointer(&st->monitors, g_strfreev);
}

void
btd_dockapp_state_from_prefs(BtdDockappState *st, const BtdPrefs *prefs)
{
    st->align = prefs->align;
    st->edge = prefs->edge;
    st->style = prefs->style;
    st->autohide = prefs->autohide;
    st->reserve_space = prefs->reserve_space;
    g_free(st->monitor);
    st->monitor = g_strdup(prefs->monitor ? prefs->monitor : "primary");
}

static gboolean
style_from_string(const char *s, BtdStyle *out)
{
    static const BtdStyle all[] = { BTD_STYLE_CLASSIC, BTD_STYLE_MODERN, BTD_STYLE_TILES };
    for (guint i = 0; s && i < G_N_ELEMENTS(all); i++)
        if (g_ascii_strcasecmp(s, btd_style_to_string(all[i])) == 0) {
            *out = all[i];
            return TRUE;
        }
    return FALSE;
}

gboolean
btd_dockapp_state_set(BtdDockappState *st, const char *name, GVariant *v)
{
    if (!name || !v)
        return FALSE;
    gboolean str = g_variant_is_of_type(v, G_VARIANT_TYPE_STRING);
    gboolean boolean = g_variant_is_of_type(v, G_VARIANT_TYPE_BOOLEAN);
    if (g_str_equal(name, "alignment") && str)
        return btd_align_from_string(g_variant_get_string(v, NULL), &st->align);
    if (g_str_equal(name, "edge") && str)
        return btd_edge_from_string(g_variant_get_string(v, NULL), &st->edge);
    if (g_str_equal(name, "style") && str)
        return style_from_string(g_variant_get_string(v, NULL), &st->style);
    if (g_str_equal(name, "monitor") && str) {
        g_free(st->monitor);
        st->monitor = g_variant_dup_string(v, NULL);
        return TRUE;
    }
    if (g_str_equal(name, "monitors") && g_variant_is_of_type(v, G_VARIANT_TYPE_STRING_ARRAY)) {
        g_strfreev(st->monitors);
        st->monitors = g_variant_dup_strv(v, NULL);
        return TRUE;
    }
    if (boolean && g_str_equal(name, "autohide"))
        st->autohide = g_variant_get_boolean(v);
    else if (boolean && g_str_equal(name, "reserve-space"))
        st->reserve_space = g_variant_get_boolean(v);
    else if (boolean && g_str_equal(name, "visible"))
        st->visible = g_variant_get_boolean(v);
    else
        return FALSE;
    return TRUE;
}

gboolean
btd_dockapp_apply_to_prefs(BtdPrefs *prefs, const char *name, GVariant *v)
{
    BtdDockappState st = { 0 };
    btd_dockapp_state_from_prefs(&st, prefs);
    gboolean ok = btd_dockapp_state_set(&st, name, v) && !g_str_equal(name, "monitors") &&
                  !g_str_equal(name, "visible");
    if (ok && g_str_equal(name, "monitor") && (!st.monitor || !*st.monitor || strlen(st.monitor) > 128))
        ok = FALSE;
    if (ok) {
        prefs->align = st.align;
        prefs->edge = st.edge;
        prefs->style = st.style;
        prefs->autohide = st.autohide;
        prefs->reserve_space = st.reserve_space;
        g_free(prefs->monitor);
        prefs->monitor = g_strdup(st.monitor);
    }
    btd_dockapp_state_clear(&st);
    return ok;
}

/* ---- texts -------------------------------------------------------------------- */

const char *
btd_dockapp_align_label(BtdAlign align)
{
    switch (align) {
    case BTD_ALIGN_FILL:
        return _("Full Width");
    case BTD_ALIGN_RIGHT:
        return _("Right");
    case BTD_ALIGN_LEFT:
        return _("Left");
    default:
        return _("Center");
    }
}

const char *
btd_dockapp_edge_label(BtdEdge edge)
{
    return edge == BTD_EDGE_TOP ? _("Top") : _("Bottom");
}

const char *
btd_dockapp_style_label(BtdStyle style)
{
    switch (style) {
    case BTD_STYLE_MODERN:
        return _("Modern");
    case BTD_STYLE_TILES:
        return _("Window Maker tiles");
    default:
        return _("Classic (OPENSTEP)");
    }
}

const char *
btd_dockapp_region_tooltip(BtdRegion region)
{
    switch (region) {
    case BTD_REGION_TOP:
        return _("Move the Tabbed Dock to the top");
    case BTD_REGION_BOTTOM:
        return _("Move the Tabbed Dock to the bottom");
    case BTD_REGION_LEFT:
        return _("Previous alignment");
    case BTD_REGION_RIGHT:
        return _("Next alignment");
    case BTD_REGION_ICON:
        return _("Open the Tabbed Dock Preferences (double click)");
    default:
        return NULL;
    }
}

static const char *
monitor_label(const BtdDockappState *st)
{
    for (char **m = st->monitors; m && *m; m++) {
        const char *tab = strchr(*m, '\t');
        if (tab && (size_t)(tab - *m) == strlen(st->monitor ? st->monitor : "") &&
            strncmp(*m, st->monitor, (size_t)(tab - *m)) == 0)
            return tab + 1;
    }
    if (!st->monitor || g_str_equal(st->monitor, "primary"))
        return _("Primary monitor");
    if (g_str_equal(st->monitor, "all"))
        return _("All monitors, mirrored");
    if (g_str_equal(st->monitor, "all-independent"))
        return _("All monitors, each with its own windows");
    return st->monitor;
}

char *
btd_dockapp_summary(const BtdDockappState *st, gboolean running)
{
    if (!running)
        return g_strdup(_("The Tabbed Dock is not running.\nDouble click: Preferences. Right button: start it."));
    return g_strdup_printf(_("Tabbed Dock\nAlignment: %s · Edge: %s\nMonitor: %s"),
                           btd_dockapp_align_label(st->align), btd_dockapp_edge_label(st->edge),
                           monitor_label(st));
}

char **
btd_dockapp_monitor_choices(const BtdDockappState *st)
{
    GPtrArray *a = g_ptr_array_new();
    gboolean current = FALSE;
    if (st->monitors && st->monitors[0]) {
        for (char **m = st->monitors; *m; m++) {
            g_ptr_array_add(a, g_strdup(*m));
            const char *tab = strchr(*m, '\t');
            if (tab && st->monitor && strncmp(*m, st->monitor, (size_t)(tab - *m)) == 0 &&
                strlen(st->monitor) == (size_t)(tab - *m))
                current = TRUE;
        }
    } else {
        g_ptr_array_add(a, g_strdup_printf("primary\t%s", _("Primary monitor")));
        g_ptr_array_add(a, g_strdup_printf("all\t%s", _("All monitors, mirrored")));
        g_ptr_array_add(a, g_strdup_printf("all-independent\t%s", _("All monitors, each with its own windows")));
        current = !st->monitor || g_str_equal(st->monitor, "primary") || g_str_equal(st->monitor, "all") ||
                  g_str_equal(st->monitor, "all-independent");
    }
    /* a monitor chosen before and unplugged now: still listed, so the
     * choice shows (the dock uses the primary meanwhile) */
    if (!current && st->monitor && *st->monitor)
        g_ptr_array_add(a, g_strdup_printf("%s\t%s", st->monitor, st->monitor));
    g_ptr_array_add(a, NULL);
    return (char **)g_ptr_array_free(a, FALSE);
}

gboolean
btd_dockapp_parse_setting(const char *text, char **key, GVariant **value)
{
    const char *eq = text ? strchr(text, '=') : NULL;
    if (!eq || eq == text)
        return FALSE;
    char *k = g_strndup(text, (size_t)(eq - text));
    const char *v = eq + 1;
    GVariant *var = NULL;
    if (g_str_equal(k, "autohide") || g_str_equal(k, "reserve-space")) {
        if (!g_ascii_strcasecmp(v, "true") || !g_ascii_strcasecmp(v, "yes") || !g_strcmp0(v, "1") ||
            !g_ascii_strcasecmp(v, "on"))
            var = g_variant_new_boolean(TRUE);
        else if (!g_ascii_strcasecmp(v, "false") || !g_ascii_strcasecmp(v, "no") || !g_strcmp0(v, "0") ||
                 !g_ascii_strcasecmp(v, "off"))
            var = g_variant_new_boolean(FALSE);
    } else if (g_str_equal(k, "alignment")) {
        BtdAlign a;
        if (btd_align_from_string(v, &a))
            var = g_variant_new_string(btd_align_to_string(a));
    } else if (g_str_equal(k, "edge")) {
        BtdEdge e;
        if (btd_edge_from_string(v, &e))
            var = g_variant_new_string(btd_edge_to_string(e));
    } else if (g_str_equal(k, "style")) {
        BtdStyle s;
        if (style_from_string(v, &s))
            var = g_variant_new_string(btd_style_to_string(s));
    } else if (g_str_equal(k, "monitor") && *v && strlen(v) <= 128) {
        var = g_variant_new_string(v);
    }
    if (!var) {
        g_free(k);
        return FALSE;
    }
    *key = k;
    *value = g_variant_ref_sink(var);
    return TRUE;
}

/* ---- wlmaker's menu --------------------------------------------------------------- */

/* A plist string: bare when it can be, else quoted. */
static void
plist_string(GString *s, const char *v)
{
    gboolean bare = *v != '\0';
    for (const char *c = v; *c; c++)
        if (!g_ascii_isalnum(*c) && *c != '_' && *c != '.' && *c != '-')
            bare = FALSE;
    if (bare) {
        g_string_append(s, v);
        return;
    }
    g_string_append_c(s, '"');
    for (const char *c = v; *c; c++) {
        if (*c == '"' || *c == '\\')
            g_string_append_c(s, '\\');
        g_string_append_c(s, *c);
    }
    g_string_append_c(s, '"');
}

char *
btd_dockapp_wlm_prefs_text(const BtdDockappState *st)
{
    GString *s = g_string_new("{\n");
    g_string_append_printf(s, "  Alignment = %s;\n", btd_align_to_string(st->align));
    g_string_append_printf(s, "  AutoHide = %s;\n", st->autohide ? "Yes" : "No");
    g_string_append_printf(s, "  Edge = %s;\n", btd_edge_to_string(st->edge));
    g_string_append(s, "  Monitor = ");
    plist_string(s, st->monitor && *st->monitor ? st->monitor : "primary");
    g_string_append(s, ";\n");
    g_string_append_printf(s, "  ReserveSpace = %s;\n", st->reserve_space ? "Yes" : "No");
    g_string_append_printf(s, "  Style = %s;\n", btd_style_to_string(st->style));
    g_string_append(s, "}\n");
    return g_string_free(s, FALSE);
}

char *
btd_dockapp_wlm_choices_text(const BtdDockappState *st)
{
    char **choices = btd_dockapp_monitor_choices(st);
    GString *s = g_string_new("(\n");
    for (char **c = choices; *c; c++) {
        char **pair = g_strsplit(*c, "\t", 2);
        if (pair[0] && pair[1]) {
            g_string_append(s, c == choices ? "  (" : ",\n  (");
            plist_string(s, pair[0]);
            g_string_append(s, ", ");
            plist_string(s, pair[1]);
            g_string_append(s, ")");
        }
        g_strfreev(pair);
    }
    g_string_append(s, "\n)\n");
    g_strfreev(choices);
    return g_string_free(s, FALSE);
}

static void
setting_free(gpointer p)
{
    BtdDockappSetting *s = p;
    g_free(s->key);
    if (s->value)
        g_variant_unref(s->value);
    g_free(s);
}

static void
add_change(GPtrArray *out, const char *key, GVariant *value)
{
    BtdDockappSetting *s = g_new0(BtdDockappSetting, 1);
    s->key = g_strdup(key);
    s->value = g_variant_ref_sink(value);
    g_ptr_array_add(out, s);
}

GPtrArray *
btd_dockapp_wlm_changes(const BtdDockappState *st, const char *text)
{
    GPtrArray *out = g_ptr_array_new_with_free_func(setting_free);
    GRegex *re = g_regex_new("^\\s*\"?([A-Za-z]+)\"?\\s*=\\s*(\"((?:[^\"\\\\]|\\\\.)*)\"|([^;\\s]*))\\s*;",
                             G_REGEX_MULTILINE, 0, NULL);
    GMatchInfo *mi = NULL;
    g_regex_match(re, text ? text : "", 0, &mi);
    while (g_match_info_matches(mi)) {
        char *key = g_match_info_fetch(mi, 1);
        char *quoted = g_match_info_fetch(mi, 3);
        char *bare = g_match_info_fetch(mi, 4);
        const char *v = quoted && *quoted ? quoted : bare;
        char *val = g_strcompress(v ? v : "");
        gboolean yes = !g_ascii_strcasecmp(val, "yes") || !g_ascii_strcasecmp(val, "true");
        if (g_str_equal(key, "Alignment")) {
            BtdAlign a;
            if (btd_align_from_string(val, &a) && a != st->align)
                add_change(out, "alignment", g_variant_new_string(btd_align_to_string(a)));
        } else if (g_str_equal(key, "Edge")) {
            BtdEdge e;
            if (btd_edge_from_string(val, &e) && e != st->edge)
                add_change(out, "edge", g_variant_new_string(btd_edge_to_string(e)));
        } else if (g_str_equal(key, "Style")) {
            BtdStyle s;
            if (style_from_string(val, &s) && s != st->style)
                add_change(out, "style", g_variant_new_string(btd_style_to_string(s)));
        } else if (g_str_equal(key, "Monitor")) {
            if (*val && strlen(val) <= 128 && g_strcmp0(val, st->monitor) != 0)
                add_change(out, "monitor", g_variant_new_string(val));
        } else if (g_str_equal(key, "AutoHide")) {
            if (yes != st->autohide)
                add_change(out, "autohide", g_variant_new_boolean(yes));
        } else if (g_str_equal(key, "ReserveSpace")) {
            if (yes != st->reserve_space)
                add_change(out, "reserve-space", g_variant_new_boolean(yes));
        }
        g_free(val);
        g_free(key);
        g_free(quoted);
        g_free(bare);
        g_match_info_next(mi, NULL);
    }
    g_match_info_free(mi);
    g_regex_unref(re);
    return out;
}

/* Titles of wlmaker's right-click menu (data/wlmaker/big-wm-tabbed-dock-
 * dockapp.plist, TextDomain big-wm-tabbed-dock): listed here so xgettext
 * puts them in the catalog. */
const char *const btd_dockapp_wlm_titles[] = {
    N_("Show or Hide the Tabbed Dock"), N_("Tabbed Dock Preferences..."), N_("Restart the Tabbed Dock"),
    N_("Stop the Tabbed Dock"), N_("Alignment"), N_("Edge"), N_("Monitor"), N_("Hide automatically"),
    N_("Reserve screen space"), N_("Appearance"), NULL,
};

char *
btd_dockapp_icon_path(void)
{
    const char *env = g_getenv("BTD_DOCKAPP_ICON_FILE");
    if (env && g_file_test(env, G_FILE_TEST_EXISTS))
        return g_strdup(env);
    char *p = g_build_filename(BTD_DATADIR, "big-wm-tabbed-dock", "tabbed-dock.svg", NULL);
    if (g_file_test(p, G_FILE_TEST_EXISTS))
        return p;
    g_free(p);
    p = g_build_filename(BTD_DATADIR, "icons", "hicolor", "scalable", "apps", "big-wm-tabbed-dock.svg", NULL);
    if (g_file_test(p, G_FILE_TEST_EXISTS))
        return p;
    g_free(p);
    return NULL;
}
