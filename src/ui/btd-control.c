/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Control API of the running dock (see btd-control.h). Every change goes
 * through the same path as the preferences window: the dock's preferences,
 * btd_dock_apply_prefs() and a queued save (atomic, with backup), so the
 * configuration file stays the only source of truth.
 */
#include "btd-control.h"

#include <glib/gi18n.h>

static GtkApplication *control_app;
static BtdDock *control_dock;
static gboolean syncing;

static GSimpleAction *
action(const char *name)
{
    return control_app ? G_SIMPLE_ACTION(g_action_map_lookup_action(G_ACTION_MAP(control_app), name)) : NULL;
}

static void
set_state(const char *name, GVariant *value)
{
    GSimpleAction *a = action(name);
    if (!a) {
        g_variant_unref(g_variant_ref_sink(value));
        return;
    }
    GVariant *old = g_action_get_state(G_ACTION(a));
    g_variant_ref_sink(value);
    if (!old || !g_variant_equal(old, value))
        g_simple_action_set_state(a, value); /* emits the change on the bus */
    if (old)
        g_variant_unref(old);
    g_variant_unref(value);
}

/* The choices of the Monitor preference, as the preferences window lists
 * them: id and label. */
static GVariant *
monitors_variant(void)
{
    GVariantBuilder b;
    g_variant_builder_init(&b, G_VARIANT_TYPE("as"));
    char *s;
    s = g_strdup_printf("primary\t%s", _("Primary monitor"));
    g_variant_builder_add(&b, "s", s);
    g_free(s);
    s = g_strdup_printf("all\t%s", _("All monitors, mirrored"));
    g_variant_builder_add(&b, "s", s);
    g_free(s);
    s = g_strdup_printf("all-independent\t%s", _("All monitors, each with its own windows"));
    g_variant_builder_add(&b, "s", s);
    g_free(s);
    GdkDisplay *d = gdk_display_get_default();
    for (int i = 0; d && i < gdk_display_get_n_monitors(d); i++) {
        GdkMonitor *m = gdk_display_get_monitor(d, i);
        GdkRectangle g;
        gdk_monitor_get_geometry(m, &g);
        const char *name = btd_monitor_name(m);
        char *id = name && *name ? g_strdup(name) : g_strdup_printf("%d", i);
        s = g_strdup_printf("%s\t%s (%d×%d)", id, name && *name ? name : "?", g.width, g.height);
        g_variant_builder_add(&b, "s", s);
        g_free(s);
        g_free(id);
    }
    return g_variant_builder_end(&b);
}

void
btd_control_sync(void)
{
    if (!control_app || !control_dock || syncing)
        return;
    syncing = TRUE;
    BtdPrefs *p = &control_dock->prefs;
    set_state("alignment", g_variant_new_string(btd_align_to_string(p->align)));
    set_state("edge", g_variant_new_string(btd_edge_to_string(p->edge)));
    set_state("monitor", g_variant_new_string(p->monitor ? p->monitor : "primary"));
    set_state("monitors", monitors_variant());
    set_state("style", g_variant_new_string(btd_style_to_string(p->style)));
    set_state("autohide", g_variant_new_boolean(p->autohide));
    set_state("reserve-space", g_variant_new_boolean(p->reserve_space));
    set_state("visible", g_variant_new_boolean(!control_dock->hidden));
    syncing = FALSE;
}

/* After a change from the bus: as the preferences window does. */
static void
commit(void)
{
    btd_dock_apply_prefs(control_dock);
    btd_dock_queue_save(control_dock);
    if (control_dock->prefs_dialog)
        btd_prefs_dialog_sync(control_dock->prefs_dialog);
    btd_control_sync();
}

static void
change_alignment(GSimpleAction *a, GVariant *value, gpointer data)
{
    BtdAlign align;
    if (!control_dock || !btd_align_from_string(g_variant_get_string(value, NULL), &align))
        return;
    if (align != control_dock->prefs.align) {
        control_dock->prefs.align = align;
        commit();
    }
}

static void
activate_alignment_step(GSimpleAction *a, GVariant *param, gpointer data)
{
    if (!control_dock || !param)
        return;
    control_dock->prefs.align = btd_align_step(control_dock->prefs.align, g_variant_get_int32(param));
    commit();
}

static void
change_edge(GSimpleAction *a, GVariant *value, gpointer data)
{
    BtdEdge edge;
    if (!control_dock || !btd_edge_from_string(g_variant_get_string(value, NULL), &edge))
        return;
    if (edge != control_dock->prefs.edge) {
        control_dock->prefs.edge = edge;
        commit();
    }
}

static void
change_monitor(GSimpleAction *a, GVariant *value, gpointer data)
{
    const char *m = g_variant_get_string(value, NULL);
    if (!control_dock || !m || !*m || strlen(m) > 128)
        return;
    if (g_strcmp0(m, control_dock->prefs.monitor) != 0) {
        g_free(control_dock->prefs.monitor);
        control_dock->prefs.monitor = g_strdup(m);
        commit();
    }
}

static void
change_style(GSimpleAction *a, GVariant *value, gpointer data)
{
    static const BtdStyle all[] = { BTD_STYLE_CLASSIC, BTD_STYLE_MODERN, BTD_STYLE_TILES };
    const char *v = g_variant_get_string(value, NULL);
    for (guint i = 0; control_dock && i < G_N_ELEMENTS(all); i++)
        if (g_strcmp0(v, btd_style_to_string(all[i])) == 0 && control_dock->prefs.style != all[i]) {
            control_dock->prefs.style = all[i];
            commit();
        }
}

static void
change_read_only(GSimpleAction *a, GVariant *value, gpointer data)
{
    /* "monitors" only reports; a request to change it is ignored */
}

static void
change_bool(GSimpleAction *a, GVariant *value, gpointer data)
{
    if (!control_dock)
        return;
    gboolean on = g_variant_get_boolean(value);
    const char *name = g_action_get_name(G_ACTION(a));
    gboolean *field = g_str_equal(name, "autohide") ? &control_dock->prefs.autohide
                                                    : &control_dock->prefs.reserve_space;
    if (*field != on) {
        *field = on;
        commit();
    }
}

static void
change_visible(GSimpleAction *a, GVariant *value, gpointer data)
{
    if (!control_dock)
        return;
    btd_dock_set_folded(control_dock, !g_variant_get_boolean(value));
    btd_control_sync();
}

static void
activate_preferences(GSimpleAction *a, GVariant *param, gpointer data)
{
    if (control_dock)
        btd_dock_show_preferences(control_dock);
}

static void
activate_quit(GSimpleAction *a, GVariant *param, gpointer data)
{
    if (control_dock)
        btd_dock_save_now(control_dock);
    g_application_quit(G_APPLICATION(control_app));
}

static void
on_monitors_changed(GdkDisplay *display, GdkMonitor *monitor, gpointer data)
{
    btd_control_sync();
}

void
btd_control_install(GtkApplication *app)
{
    static const GActionEntry entries[] = {
        { "alignment", NULL, NULL, "'fill'", change_alignment },
        { "alignment-step", activate_alignment_step, "i", NULL, NULL },
        { "edge", NULL, NULL, "'bottom'", change_edge },
        { "monitor", NULL, NULL, "'primary'", change_monitor },
        { "monitors", NULL, NULL, "@as []", change_read_only },
        { "style", NULL, NULL, "'classic'", change_style },
        { "autohide", NULL, NULL, "false", change_bool },
        { "reserve-space", NULL, NULL, "false", change_bool },
        { "visible", NULL, NULL, "true", change_visible },
        { "preferences", activate_preferences, NULL, NULL, NULL },
        { "quit", activate_quit, NULL, NULL, NULL },
    };
    control_app = app;
    g_action_map_add_action_entries(G_ACTION_MAP(app), entries, G_N_ELEMENTS(entries), NULL);
}

void
btd_control_attach(BtdDock *dock)
{
    control_dock = dock;
    if (!dock)
        return;
    GdkDisplay *display = gdk_display_get_default();
    static gboolean connected;
    if (display && !connected) {
        g_signal_connect(display, "monitor-added", G_CALLBACK(on_monitors_changed), NULL);
        g_signal_connect(display, "monitor-removed", G_CALLBACK(on_monitors_changed), NULL);
        connected = TRUE;
    }
    btd_control_sync();
}
