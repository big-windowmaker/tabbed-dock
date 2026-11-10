/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * big-wm-tabbed-dock-dockapp — the Tabbed Dock's controller, a classic
 * Window Maker dock-app (64×64 tile, OPENSTEP buttons):
 *
 *        ┌──────────── ↑ ────────────┐   ↑ / ↓   edge: top / bottom
 *        │ ◀ │  tabbed-dock.svg  │ ▶ │   ◀ / ▶   alignment, in the order
 *        └──────────── ↓ ────────────┘           fill → right → center → left
 *   double click on the icon: Preferences; right button: menu.
 *
 * An X11 dock-app (withdrawn leader with an icon window): Window Maker docks
 * it, and wlmaker docks it through XWayland. It drives the running dock
 * through its D-Bus actions (btd-control.h) and follows their states, so
 * the preferences window and the dock-app always show the same thing. With
 * no dock running, it edits the configuration file through the same core
 * library (the dock reads it when it starts).
 */
#include <cairo-xlib.h>
#include <gdk/gdkx.h>
#include <gio/gdesktopappinfo.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <librsvg/rsvg.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>

#include "btd-coexist.h"
#include "btd-config.h"
#include "btd-defaults.h"
#include "btd-dockapp.h"

#define DOCK_APP_ID "io.github.big_windowmaker.TabbedDock"
#define DOCK_PATH "/io/github/big_windowmaker/TabbedDock"
#define SELF_APP_ID "io.github.big_windowmaker.TabbedDock.DockApp"
#define NAME "big-wm-tabbed-dock-dockapp"
#define SIZE 64
#define DRAG_THRESHOLD 4
#define TIP_DELAY_MS 650
#define NOTE_MS 1600

typedef struct {
    GtkApplication *app;
    Display *dpy;
    Window root, leader, icon;
    GdkWindow *gicon;
    int depth;
    Visual *visual;
    Pixmap pixmap;
    RsvgHandle *svg;
    gboolean dark;
    GFileMonitor *theme_monitor;

    /* the running dock (D-Bus) */
    GDBusConnection *bus;
    guint watch_id;
    GDBusActionGroup *dock;
    gboolean running;
    BtdDockappState st; /* what is shown: the dock's state, or the file's */

    /* pointer */
    BtdRegion press_region, hover_region;
    int press_x, press_y, press_xr, press_yr;
    gboolean dragged;
    guint32 last_icon_click;
    guint tip_id, note_id;
    GtkWidget *tip, *tip_label;
    int tip_x, tip_y;       /* the tooltip's side next to the tile, and its top */
    gboolean tip_to_left;
    GtkWidget *menu;
    gboolean menu_pending;
    guint32 menu_time;
    int menu_x, menu_y;

    /* wlmaker: its right-click menu shows our preferences from a file */
    gboolean wlmaker;
    char *wlm_prefs, *wlm_choices, *wlm_written;
    GFileMonitor *wlm_monitor;
} DockApp;

static DockApp app_state;
static gboolean debug_events;

static void redraw(DockApp *d);
static void start_dock_cb(DockApp *d);
static void on_restore_mode(GtkMenuItem *item, DockApp *d);
static void sync_from_dock(DockApp *d);
static void wlm_write(DockApp *d);

/* ---- state ---------------------------------------------------------------- */

static const char *
dock_binary(void)
{
    const char *b = g_getenv("BTD_DOCK_BINARY"); /* tests */
    return b && *b ? b : "big-wm-tabbed-dock";
}

static const char *
dock_app_id(void)
{
    const char *id = g_getenv("BTD_APPLICATION_ID"); /* tests run private instances */
    return id && g_application_id_is_valid(id) ? id : DOCK_APP_ID;
}

static char *
dock_object_path(void)
{
    if (g_str_equal(dock_app_id(), DOCK_APP_ID))
        return g_strdup(DOCK_PATH);
    char *p = g_strdup_printf("/%s", dock_app_id());
    for (char *c = p; *c; c++)
        if (*c == '.')
            *c = '/';
        else if (*c == '-')
            *c = '_';
    return p;
}

/* No dock running: the configuration file is the state. */
static void
load_from_file(DockApp *d)
{
    char *path = btd_config_default_path();
    BtdModel *model = btd_model_new();
    BtdPrefs prefs;
    btd_prefs_init_defaults(&prefs);
    btd_config_load(path, model, &prefs, NULL);
    btd_dockapp_state_from_prefs(&d->st, &prefs);
    d->st.visible = TRUE;
    btd_prefs_clear(&prefs);
    btd_model_free(model);
    g_free(path);
}

/* Changes the file (no dock running), keeping the tabs and every other
 * preference: the core library's load and atomic save, with backup. */
static gboolean
save_to_file(const char *key, GVariant *value)
{
    char *path = btd_config_default_path();
    BtdModel *model = btd_model_new();
    BtdPrefs prefs;
    btd_prefs_init_defaults(&prefs);
    BtdLoadResult r = btd_config_load(path, model, &prefs, NULL);
    gboolean ok = FALSE;
    /* A file that could not be read at all: left alone (the dock keeps a
     * copy of it and starts from the defaults). */
    if (r != BTD_LOAD_CORRUPT_DEFAULTS && btd_dockapp_apply_to_prefs(&prefs, key, value)) {
        if (r == BTD_LOAD_MISSING) {
            /* no file yet: the dock's defaults, so nothing is lost */
            char **dirs = btd_desktop_data_dirs();
            BtdDesktopIndex *index = btd_desktop_index_new((const char *const *)dirs);
            g_strfreev(dirs);
            btd_defaults_apply(model, index);
            btd_desktop_index_free(index);
        }
        GError *err = NULL;
        ok = btd_config_save(path, model, &prefs, &err);
        if (!ok) {
            g_warning("%s: %s", path, err ? err->message : "?");
            g_clear_error(&err);
        }
    }
    btd_prefs_clear(&prefs);
    btd_model_free(model);
    g_free(path);
    return ok;
}

static void
dock_action_state(DockApp *d, const char *name, GVariant *value)
{
    g_variant_take_ref(value); /* floating or a reference of the caller: ours now */
    if (d->running && d->dock) {
        g_action_group_change_action_state(G_ACTION_GROUP(d->dock), name, value);
    } else if (save_to_file(name, value)) {
        load_from_file(d);
        redraw(d);
        wlm_write(d);
    }
    g_variant_unref(value);
}

static void
dock_activate(DockApp *d, const char *name, GVariant *param)
{
    if (d->running && d->dock)
        g_action_group_activate_action(G_ACTION_GROUP(d->dock), name, param);
    else if (param)
        g_variant_unref(g_variant_ref_sink(param));
}

static gboolean
start_dock(const char *extra)
{
    GError *err = NULL;
    gboolean ok = FALSE;
    if (!extra && !g_getenv("BTD_DOCK_BINARY")) {
        /* as the session starts it (systemd scope, environment) */
        GDesktopAppInfo *info = g_desktop_app_info_new("big-wm-tabbed-dock.desktop");
        if (info) {
            ok = g_app_info_launch(G_APP_INFO(info), NULL, NULL, &err);
            g_object_unref(info);
            if (ok)
                return TRUE;
            g_clear_error(&err);
        }
    }
    char *argv[] = { (char *)dock_binary(), (char *)extra, NULL };
    ok = g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH | G_SPAWN_STDOUT_TO_DEV_NULL, NULL, NULL, NULL, &err);
    if (!ok) {
        g_warning("%s: %s", dock_binary(), err ? err->message : "?");
        g_clear_error(&err);
    }
    return ok;
}

/* ---- the running dock ----------------------------------------------------- */

static void
on_dock_state_changed(GActionGroup *g, const char *name, GVariant *value, DockApp *d)
{
    sync_from_dock(d);
}

static void
on_dock_action_added(GActionGroup *g, const char *name, DockApp *d)
{
    sync_from_dock(d);
}

static void
sync_from_dock(DockApp *d)
{
    if (!d->dock)
        return;
    GActionGroup *g = G_ACTION_GROUP(d->dock);
    static const char *names[] = { "alignment", "edge", "monitor", "monitors", "autohide", "reserve-space", "visible",
                                   "style" };
    for (guint i = 0; i < G_N_ELEMENTS(names); i++) {
        GVariant *v = g_action_group_get_action_state(g, names[i]);
        if (v) {
            btd_dockapp_state_set(&d->st, names[i], v);
            g_variant_unref(v);
        }
    }
    redraw(d);
    wlm_write(d);
}

static void
on_dock_appeared(GDBusConnection *c, const char *name, const char *owner, DockApp *d)
{
    /* found already (a command asked the bus first): the same dock */
    if (d->dock && g_strcmp0(g_object_get_data(G_OBJECT(d->dock), "owner"), owner) == 0)
        return;
    g_clear_object(&d->dock);
    d->running = TRUE;
    char *path = dock_object_path();
    d->dock = g_dbus_action_group_get(c, owner, path);
    g_object_set_data_full(G_OBJECT(d->dock), "owner", g_strdup(owner), g_free);
    g_free(path);
    g_signal_connect(d->dock, "action-state-changed", G_CALLBACK(on_dock_state_changed), d);
    g_signal_connect(d->dock, "action-added", G_CALLBACK(on_dock_action_added), d);
    /* the first call asks the dock for its actions and states (async) */
    g_strfreev(g_action_group_list_actions(G_ACTION_GROUP(d->dock)));
    redraw(d);
}

static void
on_dock_vanished(GDBusConnection *c, const char *name, DockApp *d)
{
    d->running = FALSE;
    g_clear_object(&d->dock);
    load_from_file(d);
    redraw(d);
    wlm_write(d);
}

/* ---- drawing -------------------------------------------------------------- */

typedef struct {
    double face[3], hi[3], sh[3], outer[3], arrow[3], accent[3], well[3], well_sh[3], well_hi[3];
} Palette;

static void
palette(DockApp *d, Palette *p)
{
    /* OPENSTEP grey (Window Maker's buttons), and its dark variant */
    static const Palette light = {
        { 0.73, 0.73, 0.73 }, { 1, 1, 1 }, { 0.38, 0.38, 0.38 }, { 0, 0, 0 },
        { 0, 0, 0 }, { 0.0, 0.43, 0.42 }, { 0.62, 0.62, 0.62 }, { 0.33, 0.33, 0.33 }, { 0.92, 0.92, 0.92 } };
    static const Palette dark = {
        { 0.30, 0.30, 0.31 }, { 0.47, 0.47, 0.48 }, { 0.14, 0.14, 0.15 }, { 0, 0, 0 },
        { 0.90, 0.90, 0.90 }, { 0.24, 0.80, 0.78 }, { 0.20, 0.20, 0.21 }, { 0.08, 0.08, 0.08 }, { 0.36, 0.36, 0.37 } };
    *p = d->dark ? dark : light;
}

static void
rgb(cairo_t *cr, const double *c)
{
    cairo_set_source_rgb(cr, c[0], c[1], c[2]);
}

/* A Window Maker button: white top/left, grey and black bottom/right;
 * pressed, the other way round. */
static void
bevel(cairo_t *cr, const Palette *p, const BtdRect *r, gboolean pressed)
{
    double x = r->x, y = r->y, w = r->width, h = r->height;
    rgb(cr, p->face);
    cairo_rectangle(cr, x, y, w, h);
    cairo_fill(cr);
    cairo_set_line_width(cr, 1);
    rgb(cr, pressed ? p->outer : p->hi);
    cairo_move_to(cr, x + 0.5, y + h - 0.5);
    cairo_line_to(cr, x + 0.5, y + 0.5);
    cairo_line_to(cr, x + w - 0.5, y + 0.5);
    cairo_stroke(cr);
    rgb(cr, pressed ? p->hi : p->outer);
    cairo_move_to(cr, x + w - 0.5, y + 0.5);
    cairo_line_to(cr, x + w - 0.5, y + h - 0.5);
    cairo_line_to(cr, x + 0.5, y + h - 0.5);
    cairo_stroke(cr);
    rgb(cr, pressed ? p->face : p->sh);
    cairo_move_to(cr, x + w - 1.5, y + 1.5);
    cairo_line_to(cr, x + w - 1.5, y + h - 1.5);
    cairo_line_to(cr, x + 1.5, y + h - 1.5);
    cairo_stroke(cr);
}

static void
arrow(cairo_t *cr, const BtdRect *r, BtdRegion dir, gboolean pressed, const double *color)
{
    double cx = r->x + r->width / 2.0 + (pressed ? 1 : 0), cy = r->y + r->height / 2.0 + (pressed ? 1 : 0);
    const double a = 4.5, b = 3; /* half base, height from the centre */
    switch (dir) {
    case BTD_REGION_TOP:
        cairo_move_to(cr, cx, cy - b);
        cairo_line_to(cr, cx + a, cy + b);
        cairo_line_to(cr, cx - a, cy + b);
        break;
    case BTD_REGION_BOTTOM:
        cairo_move_to(cr, cx, cy + b);
        cairo_line_to(cr, cx + a, cy - b);
        cairo_line_to(cr, cx - a, cy - b);
        break;
    case BTD_REGION_LEFT:
        cairo_move_to(cr, cx - b, cy);
        cairo_line_to(cr, cx + b, cy - a);
        cairo_line_to(cr, cx + b, cy + a);
        break;
    default:
        cairo_move_to(cr, cx + b, cy);
        cairo_line_to(cr, cx - b, cy - a);
        cairo_line_to(cr, cx - b, cy + a);
        break;
    }
    cairo_close_path(cr);
    cairo_set_source_rgb(cr, color[0], color[1], color[2]);
    cairo_fill(cr);
}

static void
draw(DockApp *d, cairo_t *cr)
{
    Palette p;
    palette(d, &p);
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_paint(cr); /* outside the shape: never shown */

    static const BtdRegion buttons[] = { BTD_REGION_TOP, BTD_REGION_BOTTOM, BTD_REGION_LEFT, BTD_REGION_RIGHT };
    for (guint i = 0; i < G_N_ELEMENTS(buttons); i++) {
        BtdRect r;
        btd_dockapp_region_rect(buttons[i], &r);
        gboolean pressed = d->press_region == buttons[i] && !d->dragged;
        bevel(cr, &p, &r, pressed);
        /* the arrow of the current edge in the accent colour */
        gboolean current = (buttons[i] == BTD_REGION_TOP && d->st.edge == BTD_EDGE_TOP) ||
                           (buttons[i] == BTD_REGION_BOTTOM && d->st.edge == BTD_EDGE_BOTTOM);
        arrow(cr, &r, buttons[i], pressed, current ? p.accent : p.arrow);
    }

    /* the well with the icon, sunken */
    BtdRect w;
    btd_dockapp_region_rect(BTD_REGION_ICON, &w);
    rgb(cr, &p.well[0]);
    cairo_rectangle(cr, w.x, w.y, w.width, w.height);
    cairo_fill(cr);
    cairo_set_line_width(cr, 1);
    rgb(cr, p.well_sh);
    cairo_move_to(cr, w.x + 0.5, w.y + w.height - 0.5);
    cairo_line_to(cr, w.x + 0.5, w.y + 0.5);
    cairo_line_to(cr, w.x + w.width - 0.5, w.y + 0.5);
    cairo_stroke(cr);
    rgb(cr, p.well_hi);
    cairo_move_to(cr, w.x + w.width - 0.5, w.y + 0.5);
    cairo_line_to(cr, w.x + w.width - 0.5, w.y + w.height - 0.5);
    cairo_line_to(cr, w.x + 0.5, w.y + w.height - 0.5);
    cairo_stroke(cr);

    /* the icon; dimmed while the dock is not running */
    const int is = BTD_DOCKAPP_ICON;
    double ix = w.x + (w.width - is) / 2.0, iy = w.y + (w.height - is) / 2.0;
    iy += d->st.edge == BTD_EDGE_TOP ? 1 : -1; /* room for the bar */
    if (d->svg) {
        RsvgRectangle vp = { ix, iy, is, is };
        cairo_push_group(cr);
        rsvg_handle_render_document(d->svg, cr, &vp, NULL);
        cairo_pop_group_to_source(cr);
        cairo_paint_with_alpha(cr, d->running ? 1.0 : 0.45);
    }

    /* a small preview of the shelf: at its edge, with its alignment */
    if (d->running) {
        int bx, bw;
        btd_dockapp_bar_geometry(d->st.align, w.x + 2, w.width - 4, &bx, &bw);
        int by = d->st.edge == BTD_EDGE_TOP ? w.y + 2 : w.y + w.height - 4;
        rgb(cr, p.accent);
        cairo_rectangle(cr, bx, by, bw, 2);
        cairo_fill(cr);
    }
}

static void
apply_shape(DockApp *d)
{
    XRectangle rects[5];
    static const BtdRegion all[] = { BTD_REGION_TOP, BTD_REGION_BOTTOM, BTD_REGION_LEFT, BTD_REGION_RIGHT,
                                     BTD_REGION_ICON };
    for (guint i = 0; i < G_N_ELEMENTS(all); i++) {
        BtdRect r;
        btd_dockapp_region_rect(all[i], &r);
        rects[i] = (XRectangle){ r.x, r.y, r.width, r.height };
    }
    /* the gaps between the buttons and around them show the tile (and so
     * the theme of the dock) */
    XShapeCombineRectangles(d->dpy, d->icon, ShapeBounding, 0, 0, rects, 5, ShapeSet, Unsorted);
}

static void
redraw(DockApp *d)
{
    if (!d->icon)
        return;
    if (!d->pixmap)
        d->pixmap = XCreatePixmap(d->dpy, d->icon, SIZE, SIZE, d->depth);
    cairo_surface_t *s = cairo_xlib_surface_create(d->dpy, d->pixmap, d->visual, SIZE, SIZE);
    cairo_t *cr = cairo_create(s);
    draw(d, cr);
    cairo_destroy(cr);
    cairo_surface_flush(s);
    cairo_surface_destroy(s);
    /* the window keeps it as background: no flicker, nothing to do on Expose */
    XSetWindowBackgroundPixmap(d->dpy, d->icon, d->pixmap);
    XClearWindow(d->dpy, d->icon);
    XFlush(d->dpy);
}

/* ---- theme (light/dark, as Main Menu > Appearance switches it) ------------ */

static gboolean
read_dark(void)
{
    char *path = g_build_filename(g_get_user_config_dir(), "big-wm", "theme", NULL);
    char *data = NULL;
    gboolean dark = FALSE;
    if (g_file_get_contents(path, &data, NULL, NULL))
        dark = g_str_has_prefix(g_strstrip(data), "dark");
    g_free(data);
    g_free(path);
    return dark;
}

static void
on_theme_changed(GFileMonitor *m, GFile *f, GFile *o, GFileMonitorEvent e, DockApp *d)
{
    gboolean dark = read_dark();
    if (dark != d->dark) {
        d->dark = dark;
        redraw(d);
    }
}

/* ---- tooltips and notes --------------------------------------------------- */

static void
tip_hide(DockApp *d)
{
    if (d->tip_id) {
        g_source_remove(d->tip_id);
        d->tip_id = 0;
    }
    if (d->note_id) {
        g_source_remove(d->note_id);
        d->note_id = 0;
    }
    if (d->tip)
        gtk_widget_hide(d->tip);
}

/* A tooltip beside the tile, toward the middle of the screen. It never
 * covers the tile (anchored by its side next to the tile, whatever the
 * length of the text) and never takes a click (empty input shape): a
 * second click right after a change goes to the arrow, not to the note. */
/* Placed once its size is known (a popup window gets no gravity from the
 * window manager): next to the tile, never over it. */
static void
on_tip_allocate(GtkWidget *w, GdkRectangle *alloc, DockApp *d)
{
    int x = d->tip_to_left ? d->tip_x - alloc->width : d->tip_x;
    int cx = 0, cy = 0;
    gtk_window_get_position(GTK_WINDOW(w), &cx, &cy);
    if (cx != x || cy != d->tip_y)
        gtk_window_move(GTK_WINDOW(w), MAX(0, x), d->tip_y);
}

static void
tip_show(DockApp *d, const char *text, BtdRegion region)
{
    if (!text)
        return;
    if (!d->tip) {
        d->tip = gtk_window_new(GTK_WINDOW_POPUP);
        gtk_window_set_type_hint(GTK_WINDOW(d->tip), GDK_WINDOW_TYPE_HINT_TOOLTIP);
        gtk_style_context_add_class(gtk_widget_get_style_context(d->tip), "tooltip");
        d->tip_label = gtk_label_new(NULL);
        gtk_widget_set_margin_start(d->tip_label, 6);
        gtk_widget_set_margin_end(d->tip_label, 6);
        gtk_widget_set_margin_top(d->tip_label, 4);
        gtk_widget_set_margin_bottom(d->tip_label, 4);
        gtk_container_add(GTK_CONTAINER(d->tip), d->tip_label);
        gtk_widget_show(d->tip_label);
        g_signal_connect(d->tip, "size-allocate", G_CALLBACK(on_tip_allocate), d);
        gtk_widget_realize(d->tip);
        cairo_region_t *none = cairo_region_create();
        gtk_widget_input_shape_combine_region(d->tip, none);
        cairo_region_destroy(none);
    }
    gtk_label_set_text(GTK_LABEL(d->tip_label), text);
    gtk_window_resize(GTK_WINDOW(d->tip), 1, 1);
    int tx = 0, ty = 0;
    Window child;
    XTranslateCoordinates(d->dpy, d->icon, d->root, 0, 0, &tx, &ty, &child);
    BtdRect r;
    btd_dockapp_region_rect(region == BTD_REGION_NONE ? BTD_REGION_ICON : region, &r);
    GtkRequisition req;
    gtk_widget_get_preferred_size(d->tip, NULL, &req);
    int sw = WidthOfScreen(DefaultScreenOfDisplay(d->dpy));
    gboolean to_left = tx + SIZE / 2 > sw / 2;
    d->tip_to_left = to_left;
    d->tip_x = to_left ? tx - 6 : tx + SIZE + 6;
    d->tip_y = MAX(0, ty + r.y + r.height / 2 - req.height / 2);
    gtk_window_move(GTK_WINDOW(d->tip), MAX(0, to_left ? d->tip_x - req.width : d->tip_x), d->tip_y);
    gtk_widget_show(d->tip);
}

static gboolean
tip_timeout(gpointer data)
{
    DockApp *d = data;
    d->tip_id = 0;
    if (d->hover_region != BTD_REGION_NONE && d->press_region == BTD_REGION_NONE)
        tip_show(d, btd_dockapp_region_tooltip(d->hover_region), d->hover_region);
    return G_SOURCE_REMOVE;
}

static gboolean
note_timeout(gpointer data)
{
    DockApp *d = data;
    d->note_id = 0;
    if (d->tip)
        gtk_widget_hide(d->tip);
    return G_SOURCE_REMOVE;
}

/* A short note after a change ("Alignment: Center"). */
static void
note(DockApp *d, char *text, BtdRegion region)
{
    tip_hide(d);
    tip_show(d, text, region);
    d->note_id = g_timeout_add(NOTE_MS, note_timeout, d);
    g_free(text);
}

/* ---- actions ---------------------------------------------------------------- */

static void
do_align_step(DockApp *d, int delta, BtdRegion region)
{
    BtdAlign next = btd_align_step(d->st.align, delta);
    d->st.align = next; /* shown at once; the dock's state confirms it */
    redraw(d);
    if (d->running && d->dock)
        dock_activate(d, "alignment-step", g_variant_new_int32(delta));
    else
        dock_action_state(d, "alignment", g_variant_new_string(btd_align_to_string(next)));
    note(d, g_strdup_printf(_("Alignment: %s"), btd_dockapp_align_label(next)), region);
}

static void
do_edge(DockApp *d, BtdEdge edge, BtdRegion region)
{
    d->st.edge = edge;
    redraw(d);
    dock_action_state(d, "edge", g_variant_new_string(btd_edge_to_string(edge)));
    note(d, g_strdup_printf(_("Edge: %s"), btd_dockapp_edge_label(edge)), region);
}

static void
do_preferences(DockApp *d)
{
    tip_hide(d);
    if (d->running && d->dock)
        dock_activate(d, "preferences", NULL);
    else
        start_dock("--preferences");
}

static void
do_toggle_visible(DockApp *d)
{
    if (!d->running) {
        start_dock(NULL);
        return;
    }
    dock_action_state(d, "visible", g_variant_new_boolean(!d->st.visible));
}

static void
do_quit_dock(DockApp *d)
{
    dock_activate(d, "quit", NULL);
}

static gboolean
restart_when_gone(gpointer data)
{
    DockApp *d = data;
    if (d->running)
        return G_SOURCE_CONTINUE; /* still quitting */
    start_dock(NULL);
    return G_SOURCE_REMOVE;
}

static void
do_restart_dock(DockApp *d)
{
    if (!d->running) {
        start_dock(NULL);
        return;
    }
    do_quit_dock(d);
    g_timeout_add(200, restart_when_gone, d);
}

/* ---- Window Maker's own menu of the icon ------------------------------------ */

/* The Window Maker icon (Dock, Clip, drawer or loose icon) our icon window
 * sits in; None when no window manager adopted it (wlmaker shows its own
 * menu on the right button and never passes it). */
static Window
wm_parent(DockApp *d)
{
    Window root, parent = None, *kids = NULL;
    unsigned nk = 0;
    if (!XQueryTree(d->dpy, d->icon, &root, &parent, &kids, &nk))
        return None;
    if (kids)
        XFree(kids);
    return parent == d->root ? None : parent;
}

/* Hands a press to Window Maker, as it expects from a dock-app that takes
 * its clicks: a synthetic press on the icon, after giving up our implicit
 * grab. Button 1: it moves the icon; button 3: its menu. */
static gboolean
wm_forward(DockApp *d, unsigned button, int x, int y, int xr, int yr)
{
    Window parent = wm_parent(d), child;
    if (parent == None)
        return FALSE;
    int px = x, py = y;
    XTranslateCoordinates(d->dpy, d->icon, parent, x, y, &px, &py, &child);
    XUngrabPointer(d->dpy, CurrentTime);
    XEvent ev = { 0 };
    ev.xbutton.type = ButtonPress;
    ev.xbutton.display = d->dpy;
    ev.xbutton.window = parent;
    ev.xbutton.root = d->root;
    ev.xbutton.time = CurrentTime;
    ev.xbutton.x = px;
    ev.xbutton.y = py;
    ev.xbutton.x_root = xr;
    ev.xbutton.y_root = yr;
    ev.xbutton.button = button;
    ev.xbutton.same_screen = True;
    XSendEvent(d->dpy, parent, False, ButtonPressMask, &ev);
    XFlush(d->dpy);
    return TRUE;
}

/* (connected swapped, as every item of menu_item()) */
static void
on_wm_options(DockApp *d)
{
    int xr = 0, yr = 0;
    Window child;
    XTranslateCoordinates(d->dpy, d->icon, d->root, SIZE / 2, SIZE / 2, &xr, &yr, &child);
    wm_forward(d, Button3, SIZE / 2, SIZE / 2, xr, yr);
}

/* ---- the menu (X11; wlmaker shows its own, see btd-dockapp-wlm) ----------- */

static void
on_menu_align(GtkCheckMenuItem *item, DockApp *d)
{
    if (debug_events)
        g_printerr("menu alignment %s active=%d\n", (char *)g_object_get_data(G_OBJECT(item), "value"),
                   gtk_check_menu_item_get_active(item));
    if (!gtk_check_menu_item_get_active(item))
        return;
    dock_action_state(d, "alignment", g_variant_new_string(g_object_get_data(G_OBJECT(item), "value")));
}

static void
on_menu_edge(GtkCheckMenuItem *item, DockApp *d)
{
    if (!gtk_check_menu_item_get_active(item))
        return;
    dock_action_state(d, "edge", g_variant_new_string(g_object_get_data(G_OBJECT(item), "value")));
}

static void
on_menu_monitor(GtkCheckMenuItem *item, DockApp *d)
{
    if (!gtk_check_menu_item_get_active(item))
        return;
    dock_action_state(d, "monitor", g_variant_new_string(g_object_get_data(G_OBJECT(item), "value")));
}

static void
on_menu_style(GtkCheckMenuItem *item, DockApp *d)
{
    if (!gtk_check_menu_item_get_active(item))
        return;
    dock_action_state(d, "style", g_variant_new_string(g_object_get_data(G_OBJECT(item), "value")));
}

static void
on_menu_autohide(GtkCheckMenuItem *item, DockApp *d)
{
    dock_action_state(d, "autohide", g_variant_new_boolean(gtk_check_menu_item_get_active(item)));
}

static void
on_menu_reserve(GtkCheckMenuItem *item, DockApp *d)
{
    dock_action_state(d, "reserve-space", g_variant_new_boolean(gtk_check_menu_item_get_active(item)));
}

static void
on_menu_mode(GtkCheckMenuItem *item, DockApp *d)
{
    if (!gtk_check_menu_item_get_active(item))
        return;
    BtdDockMode mode = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), "value"));
    BtdCoexistPaths paths;
    btd_coexist_paths_init(&paths);
    GError *err = NULL;
    if (btd_coexist_set_mode(&paths, mode, &err))
        note(d, g_strdup(_("The docks change at the next login.")), BTD_REGION_ICON);
    else
        note(d, g_strdup_printf(_("Could not change the docks: %s"), err ? err->message : "?"), BTD_REGION_ICON);
    g_clear_error(&err);
    btd_coexist_paths_clear(&paths);
}

static void
debug_deactivate(GtkMenuShell *m, gpointer data)
{
    g_printerr("menu deactivate\n");
}

static GtkWidget *
menu_item(GtkWidget *menu, const char *label, GCallback cb, DockApp *d)
{
    GtkWidget *i = gtk_menu_item_new_with_mnemonic(label);
    if (cb)
        g_signal_connect_swapped(i, "activate", cb, d);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), i);
    return i;
}

static GtkWidget *
submenu(GtkWidget *menu, const char *label)
{
    GtkWidget *i = gtk_menu_item_new_with_mnemonic(label);
    GtkWidget *sub = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(i), sub);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), i);
    return sub;
}

static GSList *
radio(GtkWidget *menu, GSList *group, const char *label, const char *value, gboolean active, GCallback cb,
      DockApp *d)
{
    GtkWidget *i = gtk_radio_menu_item_new_with_label(group, label);
    g_object_set_data_full(G_OBJECT(i), "value", g_strdup(value), g_free);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(i), active);
    g_signal_connect(i, "toggled", cb, d);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), i);
    return gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(i));
}

static void
on_about(DockApp *d)
{
    gtk_show_about_dialog(NULL, "program-name", _("Tabbed Dock"), "version", BTD_VERSION, "logo-icon-name",
                          "big-wm-tabbed-dock", "comments",
                          _("Controller of the Tabbed Dock: alignment, edge, monitor and startup, from the Dock."),
                          "license-type", GTK_LICENSE_GPL_2_0, "website",
                          "https://github.com/big-windowmaker/tabbed-dock", "authors",
                          (const char *[]){ "Rafael Ruscher <rruscher@gmail.com>", "Big WindowMaker", NULL }, NULL);
}

static void
show_menu(DockApp *d, GdkEvent *trigger)
{
    tip_hide(d);
    if (d->menu)
        gtk_widget_destroy(d->menu);
    GtkWidget *m = gtk_menu_new();
    d->menu = m;
    g_object_add_weak_pointer(G_OBJECT(m), (gpointer *)&d->menu);
    /* destroyed after the chosen item ran (on "deactivate" the item would
     * be gone before its activation) */
    g_signal_connect(m, "selection-done", G_CALLBACK(gtk_widget_destroy), NULL);
    if (debug_events)
        g_signal_connect(m, "deactivate", G_CALLBACK(debug_deactivate), NULL);

    GtkWidget *title = gtk_menu_item_new_with_label(_("Tabbed Dock"));
    gtk_widget_set_sensitive(title, FALSE);
    gtk_menu_shell_append(GTK_MENU_SHELL(m), title);

    GtkWidget *show = gtk_check_menu_item_new_with_mnemonic(_("_Show the Tabbed Dock"));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(show), d->running && d->st.visible);
    g_signal_connect_swapped(show, "toggled", G_CALLBACK(do_toggle_visible), d);
    gtk_menu_shell_append(GTK_MENU_SHELL(m), show);
    menu_item(m, _("_Preferences…"), G_CALLBACK(do_preferences), d);
    gtk_menu_shell_append(GTK_MENU_SHELL(m), gtk_separator_menu_item_new());

    GtkWidget *sub = submenu(m, _("_Alignment"));
    GSList *g = NULL;
    static const BtdAlign aligns[] = { BTD_ALIGN_FILL, BTD_ALIGN_RIGHT, BTD_ALIGN_CENTER, BTD_ALIGN_LEFT };
    for (guint i = 0; i < G_N_ELEMENTS(aligns); i++)
        g = radio(sub, g, btd_dockapp_align_label(aligns[i]), btd_align_to_string(aligns[i]),
                  d->st.align == aligns[i], G_CALLBACK(on_menu_align), d);

    sub = submenu(m, _("_Edge"));
    g = NULL;
    g = radio(sub, g, btd_dockapp_edge_label(BTD_EDGE_BOTTOM), "bottom", d->st.edge == BTD_EDGE_BOTTOM,
              G_CALLBACK(on_menu_edge), d);
    g = radio(sub, g, btd_dockapp_edge_label(BTD_EDGE_TOP), "top", d->st.edge == BTD_EDGE_TOP,
              G_CALLBACK(on_menu_edge), d);

    sub = submenu(m, _("_Monitor"));
    g = NULL;
    char **choices = btd_dockapp_monitor_choices(&d->st);
    for (char **c = choices; c && *c; c++) {
        char **pair = g_strsplit(*c, "\t", 2);
        if (pair[0] && pair[1])
            g = radio(sub, g, pair[1], pair[0], g_strcmp0(pair[0], d->st.monitor) == 0,
                      G_CALLBACK(on_menu_monitor), d);
        g_strfreev(pair);
    }
    g_strfreev(choices);

    GtkWidget *ah = gtk_check_menu_item_new_with_mnemonic(_("Hide _automatically"));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(ah), d->st.autohide);
    g_signal_connect(ah, "toggled", G_CALLBACK(on_menu_autohide), d);
    gtk_menu_shell_append(GTK_MENU_SHELL(m), ah);
    GtkWidget *rs = gtk_check_menu_item_new_with_mnemonic(_("_Reserve screen space"));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(rs), d->st.reserve_space);
    g_signal_connect(rs, "toggled", G_CALLBACK(on_menu_reserve), d);
    gtk_menu_shell_append(GTK_MENU_SHELL(m), rs);

    sub = submenu(m, _("Appea_rance"));
    g = NULL;
    static const BtdStyle styles[] = { BTD_STYLE_CLASSIC, BTD_STYLE_TILES, BTD_STYLE_MODERN };
    for (guint i = 0; i < G_N_ELEMENTS(styles); i++)
        g = radio(sub, g, btd_dockapp_style_label(styles[i]), btd_style_to_string(styles[i]),
                  d->st.style == styles[i], G_CALLBACK(on_menu_style), d);

    sub = submenu(m, _("Start_up"));
    GtkWidget *i;
    i = menu_item(sub, _("_Start the Tabbed Dock"), G_CALLBACK(start_dock_cb), d);
    gtk_widget_set_sensitive(i, !d->running);
    i = menu_item(sub, _("S_top the Tabbed Dock"), G_CALLBACK(do_quit_dock), d);
    gtk_widget_set_sensitive(i, d->running);
    menu_item(sub, _("_Restart the Tabbed Dock"), G_CALLBACK(do_restart_dock), d);
    gtk_menu_shell_append(GTK_MENU_SHELL(sub), gtk_separator_menu_item_new());
    GtkWidget *hdr = gtk_menu_item_new_with_label(_("At the next login:"));
    gtk_widget_set_sensitive(hdr, FALSE);
    gtk_menu_shell_append(GTK_MENU_SHELL(sub), hdr);
    BtdCoexistPaths paths;
    btd_coexist_paths_init(&paths);
    BtdDockMode mode = btd_coexist_get_mode(&paths, d->wlmaker);
    btd_coexist_paths_clear(&paths);
    static const struct {
        BtdDockMode mode;
        const char *label;
    } modes[] = {
        { BTD_MODE_BOTH, N_("Tabbed Dock and classic Dock") },
        { BTD_MODE_TABBED, N_("Only the Tabbed Dock") },
        { BTD_MODE_CLASSIC, N_("Only the classic Dock") },
    };
    g = NULL;
    for (guint k = 0; k < G_N_ELEMENTS(modes); k++) {
        GtkWidget *r = gtk_radio_menu_item_new_with_label(g, _(modes[k].label));
        g = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(r));
        g_object_set_data(G_OBJECT(r), "value", GINT_TO_POINTER(modes[k].mode));
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(r), mode == modes[k].mode);
        g_signal_connect(r, "toggled", G_CALLBACK(on_menu_mode), d);
        gtk_menu_shell_append(GTK_MENU_SHELL(sub), r);
    }
    i = gtk_menu_item_new_with_mnemonic(_("Restore the _default"));
    g_object_set_data(G_OBJECT(i), "value", GINT_TO_POINTER(BTD_MODE_BOTH));
    g_signal_connect(i, "activate", G_CALLBACK(on_restore_mode), d);
    gtk_menu_shell_append(GTK_MENU_SHELL(sub), i);

    gtk_menu_shell_append(GTK_MENU_SHELL(m), gtk_separator_menu_item_new());
    if (wm_parent(d) != None)
        menu_item(m, _("_Window Maker Options…"), G_CALLBACK(on_wm_options), d);
    menu_item(m, _("A_bout"), G_CALLBACK(on_about), d);

    gtk_widget_show_all(m);
    gtk_menu_popup_at_pointer(GTK_MENU(m), trigger);
}

static void
start_dock_cb(DockApp *d)
{
    start_dock(NULL);
}

static void
on_restore_mode(GtkMenuItem *item, DockApp *d)
{
    BtdCoexistPaths paths;
    btd_coexist_paths_init(&paths);
    GError *err = NULL;
    if (btd_coexist_set_mode(&paths, BTD_MODE_BOTH, &err))
        note(d, g_strdup(_("The docks change at the next login.")), BTD_REGION_ICON);
    else
        note(d, g_strdup_printf(_("Could not change the docks: %s"), err ? err->message : "?"), BTD_REGION_ICON);
    g_clear_error(&err);
    btd_coexist_paths_clear(&paths);
}

static gboolean
show_menu_idle(gpointer data)
{
    DockApp *d = data;
    /* the trigger: a press on the root window (a window of GDK, unlike
     * the dock-app's own) where the pointer is, with the seat's pointer */
    GdkEvent *trigger = gdk_event_new(GDK_BUTTON_PRESS);
    GdkWindow *root = gdk_get_default_root_window();
    trigger->button.window = g_object_ref(root);
    trigger->button.button = 3;
    trigger->button.time = d->menu_time;
    trigger->button.x = trigger->button.x_root = d->menu_x;
    trigger->button.y = trigger->button.y_root = d->menu_y;
    GdkSeat *seat = gdk_display_get_default_seat(gdk_display_get_default());
    if (seat)
        gdk_event_set_device(trigger, gdk_seat_get_pointer(seat));
    show_menu(d, trigger);
    gdk_event_free(trigger);
    return G_SOURCE_REMOVE;
}

/* ---- pointer --------------------------------------------------------------- */

static void
on_press(DockApp *d, XButtonEvent *ev)
{
    if (debug_events)
        g_printerr("press %u at %d,%d region %d\n", ev->button, ev->x, ev->y, btd_dockapp_region_at(ev->x, ev->y));
    tip_hide(d);
    BtdRegion r = btd_dockapp_region_at(ev->x, ev->y);
    if (ev->button == Button4 || ev->button == Button5) {
        /* the wheel steps through the alignments, as the side arrows do */
        do_align_step(d, ev->button == Button4 ? -1 : 1, BTD_REGION_ICON);
        return;
    }
    if (ev->button == Button3) {
        /* the menu opens on the release: the press holds the pointer (our
         * implicit grab) and the menu's own grab would fail */
        d->menu_pending = TRUE;
        return;
    }
    if (ev->button != Button1)
        return;
    d->press_region = r;
    d->press_x = ev->x;
    d->press_y = ev->y;
    d->press_xr = ev->x_root;
    d->press_yr = ev->y_root;
    d->dragged = FALSE;
    if (r != BTD_REGION_ICON && r != BTD_REGION_NONE)
        redraw(d); /* the button goes down */
}

static void
on_motion(DockApp *d, XMotionEvent *ev)
{
    BtdRegion r = btd_dockapp_region_at(ev->x, ev->y);
    if (d->press_region != BTD_REGION_NONE && !d->dragged &&
        (abs(ev->x - d->press_x) > DRAG_THRESHOLD || abs(ev->y - d->press_y) > DRAG_THRESHOLD)) {
        /* a drag: the window manager moves the tile (Window Maker) */
        d->dragged = TRUE;
        redraw(d);
        if (wm_forward(d, Button1, d->press_x, d->press_y, d->press_xr, d->press_yr))
            d->press_region = BTD_REGION_NONE;
        return;
    }
    if (r != d->hover_region) {
        d->hover_region = r;
        if (d->tip_id)
            g_source_remove(d->tip_id);
        d->tip_id = 0;
        if (!d->note_id && d->tip)
            gtk_widget_hide(d->tip);
        if (r != BTD_REGION_NONE && d->press_region == BTD_REGION_NONE)
            d->tip_id = g_timeout_add(TIP_DELAY_MS, tip_timeout, d);
    }
}

static void
on_release(DockApp *d, XButtonEvent *ev)
{
    if (debug_events)
        g_printerr("release %u at %d,%d region %d (pressed %d, dragged %d)\n", ev->button, ev->x, ev->y,
                   btd_dockapp_region_at(ev->x, ev->y), d->press_region, d->dragged);
    if (ev->button == Button3 && d->menu_pending) {
        d->menu_pending = FALSE;
        d->menu_time = ev->time;
        d->menu_x = ev->x_root;
        d->menu_y = ev->y_root;
        g_idle_add(show_menu_idle, d);
        return;
    }
    if (ev->button != Button1)
        return;
    BtdRegion pressed = d->press_region;
    gboolean dragged = d->dragged;
    d->press_region = BTD_REGION_NONE;
    d->dragged = FALSE;
    redraw(d);
    if (dragged || pressed == BTD_REGION_NONE || btd_dockapp_region_at(ev->x, ev->y) != pressed)
        return;
    switch (pressed) {
    case BTD_REGION_LEFT:
        do_align_step(d, -1, pressed);
        break;
    case BTD_REGION_RIGHT:
        do_align_step(d, 1, pressed);
        break;
    case BTD_REGION_TOP:
        do_edge(d, BTD_EDGE_TOP, pressed);
        break;
    case BTD_REGION_BOTTOM:
        do_edge(d, BTD_EDGE_BOTTOM, pressed);
        break;
    case BTD_REGION_ICON: {
        /* double click on the icon only: two quick clicks on the arrows
         * are two steps, never the preferences */
        int dbl = 400;
        g_object_get(gtk_settings_get_default(), "gtk-double-click-time", &dbl, NULL);
        if (d->last_icon_click && ev->time - d->last_icon_click <= (guint32)dbl) {
            d->last_icon_click = 0;
            do_preferences(d);
        } else {
            d->last_icon_click = ev->time;
            note(d, btd_dockapp_summary(&d->st, d->running), BTD_REGION_ICON);
        }
        break;
    }
    default:
        break;
    }
    if (pressed != BTD_REGION_ICON)
        d->last_icon_click = 0;
}

/* The dock-app's own window is a foreign X window for GDK: its pointer
 * events are taken here, before GDK (which would log a warning for each
 * click on a non-toplevel); GTK only gets the menu's and the tooltip's. */
static GdkFilterReturn
x_filter(GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
    DockApp *d = data;
    XEvent *ev = xevent;
    switch (ev->type) {
    case ButtonPress:
        on_press(d, &ev->xbutton);
        return GDK_FILTER_REMOVE;
    case ButtonRelease:
        on_release(d, &ev->xbutton);
        return GDK_FILTER_REMOVE;
    case MotionNotify:
        on_motion(d, &ev->xmotion);
        return GDK_FILTER_REMOVE;
    case LeaveNotify:
        d->hover_region = BTD_REGION_NONE;
        if (d->tip_id) {
            g_source_remove(d->tip_id);
            d->tip_id = 0;
        }
        if (!d->note_id && d->tip)
            gtk_widget_hide(d->tip);
        return GDK_FILTER_REMOVE;
    case EnterNotify:
        return GDK_FILTER_REMOVE;
    default:
        return GDK_FILTER_CONTINUE;
    }
}

/* ---- the dock-app windows --------------------------------------------------- */

static void
create_windows(DockApp *d, int argc, char **argv)
{
    GdkDisplay *gdisp = gdk_display_get_default();
    d->dpy = GDK_DISPLAY_XDISPLAY(gdisp);
    int scr = DefaultScreen(d->dpy);
    d->root = RootWindow(d->dpy, scr);
    d->depth = DefaultDepth(d->dpy, scr);
    d->visual = DefaultVisual(d->dpy, scr);

    XSetWindowAttributes a = { 0 };
    a.background_pixel = BlackPixel(d->dpy, scr);
    d->leader = XCreateWindow(d->dpy, d->root, 0, 0, 1, 1, 0, CopyFromParent, InputOutput, CopyFromParent,
                              CWBackPixel, &a);
    d->icon = XCreateWindow(d->dpy, d->root, 0, 0, SIZE, SIZE, 0, CopyFromParent, InputOutput, CopyFromParent,
                            CWBackPixel, &a);
    XClassHint ch = { NAME, "DockApp" };
    XSetClassHint(d->dpy, d->leader, &ch);
    XSetClassHint(d->dpy, d->icon, &ch);
    XStoreName(d->dpy, d->leader, _("Tabbed Dock"));
    XSizeHints sh = { 0 };
    sh.flags = PMinSize | PMaxSize | USPosition;
    sh.min_width = sh.max_width = SIZE;
    sh.min_height = sh.max_height = SIZE;
    XSetWMNormalHints(d->dpy, d->leader, &sh);
    XWMHints wh = { 0 };
    wh.flags = StateHint | IconWindowHint | WindowGroupHint;
    wh.initial_state = WithdrawnState; /* a dock-app: only the icon window shows */
    wh.icon_window = d->icon;
    wh.window_group = d->leader;
    XSetWMHints(d->dpy, d->leader, &wh);
    XSetCommand(d->dpy, d->leader, argv, argc); /* what Window Maker stores as the Command */
    XSelectInput(d->dpy, d->icon,
                 ExposureMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | EnterWindowMask |
                     LeaveWindowMask | StructureNotifyMask);
    apply_shape(d);
    redraw(d);
    XMapWindow(d->dpy, d->leader);
    XFlush(d->dpy);

    d->gicon = gdk_x11_window_foreign_new_for_display(gdisp, d->icon);
    gdk_window_add_filter(d->gicon, x_filter, d);
}

/* ---- wlmaker: its right-click menu (Dockapps.d schema) -------------------- */

/* wlmaker shows the dock-app's preferences from
 * ~/.config/wlmaker/big-wm-tabbed-dock-dockapp.plist and its monitor
 * choices from a file next to it: we keep both in line with the dock, and
 * apply what the user picks there. */
static void
wlm_write(DockApp *d)
{
    if (!d->wlmaker)
        return;
    char *prefs = btd_dockapp_wlm_prefs_text(&d->st);
    if (g_strcmp0(prefs, d->wlm_written) != 0) {
        GError *err = NULL;
        if (g_file_set_contents(d->wlm_prefs, prefs, -1, &err)) {
            g_free(d->wlm_written);
            d->wlm_written = g_strdup(prefs);
        } else {
            g_warning("%s", err->message);
            g_clear_error(&err);
        }
    }
    g_free(prefs);
    char *choices = btd_dockapp_wlm_choices_text(&d->st);
    char *old = NULL;
    if (!g_file_get_contents(d->wlm_choices, &old, NULL, NULL) || g_strcmp0(old, choices) != 0)
        g_file_set_contents(d->wlm_choices, choices, -1, NULL);
    g_free(old);
    g_free(choices);
}

static void
on_wlm_prefs_changed(GFileMonitor *m, GFile *f, GFile *o, GFileMonitorEvent e, DockApp *d)
{
    if (e != G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT && e != G_FILE_MONITOR_EVENT_CREATED &&
        e != G_FILE_MONITOR_EVENT_RENAMED && e != G_FILE_MONITOR_EVENT_MOVED_IN)
        return;
    char *text = NULL;
    if (!g_file_get_contents(d->wlm_prefs, &text, NULL, NULL))
        return;
    if (g_strcmp0(text, d->wlm_written) == 0) {
        g_free(text); /* our own write */
        return;
    }
    /* what the user picked in wlmaker's menu */
    GPtrArray *changes = btd_dockapp_wlm_changes(&d->st, text);
    for (guint i = 0; i < changes->len; i++) {
        BtdDockappSetting *c = changes->pdata[i];
        dock_action_state(d, c->key, g_variant_ref(c->value));
    }
    g_ptr_array_unref(changes);
    g_free(text);
}

static void
wlm_init(DockApp *d)
{
    char *dir = g_build_filename(g_get_user_config_dir(), "wlmaker", NULL);
    g_mkdir_with_parents(dir, 0700);
    d->wlm_prefs = g_build_filename(dir, NAME ".plist", NULL);
    d->wlm_choices = g_build_filename(dir, NAME ".monitors.plist", NULL);
    g_free(dir);
    GFile *f = g_file_new_for_path(d->wlm_prefs);
    d->wlm_monitor = g_file_monitor_file(f, G_FILE_MONITOR_WATCH_MOVES, NULL, NULL);
    g_object_unref(f);
    if (d->wlm_monitor)
        g_signal_connect(d->wlm_monitor, "changed", G_CALLBACK(on_wlm_prefs_changed), d);
    wlm_write(d);
}

/* ---- command line ------------------------------------------------------------ */

/* A command right after starting: the asynchronous watch may not know yet
 * whether the dock runs. Ask the bus now, so a running dock gets the change
 * (and the file is never written behind its back). */
static void
find_dock_now(DockApp *d)
{
    if (d->running || !d->bus)
        return;
    GVariant *r = g_dbus_connection_call_sync(d->bus, "org.freedesktop.DBus", "/org/freedesktop/DBus",
                                              "org.freedesktop.DBus", "GetNameOwner",
                                              g_variant_new("(s)", dock_app_id()), G_VARIANT_TYPE("(s)"),
                                              G_DBUS_CALL_FLAGS_NONE, 2000, NULL, NULL);
    if (!r)
        return; /* no owner: no dock */
    const char *owner = NULL;
    g_variant_get(r, "(&s)", &owner);
    on_dock_appeared(d->bus, dock_app_id(), owner, d);
    g_variant_unref(r);
}

/* --toggle, --start, --stop, --restart, --preferences, --set KEY=VALUE: the
 * items of wlmaker's menu (and scripts) run these. */
static int
run_command(DockApp *d, GVariantDict *opts, GApplicationCommandLine *cl)
{
    gboolean any = g_variant_dict_contains(opts, "toggle") || g_variant_dict_contains(opts, "start") ||
                   g_variant_dict_contains(opts, "stop") || g_variant_dict_contains(opts, "restart") ||
                   g_variant_dict_contains(opts, "preferences") || g_variant_dict_contains(opts, "set");
    if (any)
        find_dock_now(d);
    const char *set = NULL;
    gboolean done = FALSE;
    if (g_variant_dict_lookup(opts, "toggle", "b", &done) && done)
        do_toggle_visible(d);
    else if (g_variant_dict_lookup(opts, "start", "b", &done) && done)
        start_dock(NULL);
    else if (g_variant_dict_lookup(opts, "stop", "b", &done) && done)
        do_quit_dock(d);
    else if (g_variant_dict_lookup(opts, "restart", "b", &done) && done)
        do_restart_dock(d);
    else if (g_variant_dict_lookup(opts, "preferences", "b", &done) && done)
        do_preferences(d);
    else if (g_variant_dict_lookup(opts, "set", "&s", &set)) {
        char *key = NULL;
        GVariant *value = NULL;
        if (!btd_dockapp_parse_setting(set, &key, &value)) {
            g_application_command_line_printerr(cl, "%s: --set alignment|edge|monitor|style=VALUE, "
                                                    "autohide|reserve-space=true|false\n", NAME);
            return 2;
        }
        dock_action_state(d, key, value); /* takes the reference */
        g_free(key);
        done = TRUE;
    }
    return done ? 0 : -1;
}

static gboolean
quit_idle(gpointer app)
{
    if (app_state.bus)
        g_dbus_connection_flush_sync(app_state.bus, NULL, NULL); /* the call is out */
    g_application_release(G_APPLICATION(app));
    return G_SOURCE_REMOVE;
}

static int
on_command_line(GApplication *gapp, GApplicationCommandLine *cl, DockApp *d)
{
    GVariantDict *opts = g_application_command_line_get_options_dict(cl);
    int r = run_command(d, opts, cl);
    if (r >= 0) {
        if (!d->icon) {
            /* only a command, no dock-app: give D-Bus a moment, then go */
            g_application_hold(gapp);
            g_timeout_add(r == 0 ? 600 : 0, quit_idle, gapp);
        }
        return r;
    }
    if (!d->icon) {
        gchar **argv = g_application_command_line_get_arguments(cl, NULL);
        create_windows(d, (int)g_strv_length(argv), argv);
        g_strfreev(argv);
        g_application_hold(gapp); /* the dock-app has no GtkWindow */
        if (d->wlmaker)
            wlm_init(d);
    }
    return 0;
}

static void
on_startup(GApplication *gapp, DockApp *d)
{
    d->dark = read_dark();
    char *path = g_build_filename(g_get_user_config_dir(), "big-wm", "theme", NULL);
    GFile *f = g_file_new_for_path(path);
    d->theme_monitor = g_file_monitor_file(f, G_FILE_MONITOR_WATCH_MOVES, NULL, NULL);
    if (d->theme_monitor)
        g_signal_connect(d->theme_monitor, "changed", G_CALLBACK(on_theme_changed), d);
    g_object_unref(f);
    g_free(path);
    char *svg = btd_dockapp_icon_path();
    if (svg)
        d->svg = rsvg_handle_new_from_file(svg, NULL);
    g_free(svg);
    load_from_file(d);
    d->bus = g_application_get_dbus_connection(gapp);
    if (d->bus)
        d->watch_id = g_bus_watch_name_on_connection(d->bus, dock_app_id(), G_BUS_NAME_WATCHER_FLAGS_NONE,
                                                     (GBusNameAppearedCallback)on_dock_appeared,
                                                     (GBusNameVanishedCallback)on_dock_vanished, d, NULL);
}

int
main(int argc, char **argv)
{
    setlocale(LC_ALL, "");
    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);
    DockApp *d = &app_state;
    debug_events = g_getenv("BTD_DOCKAPP_DEBUG") != NULL;
    d->press_region = d->hover_region = BTD_REGION_NONE;
    /* under wlmaker (Wayland session) it runs on XWayland, and wlmaker's own
     * menu offers its options */
    d->wlmaker = g_getenv("WAYLAND_DISPLAY") != NULL;
    /* a dock-app is an X11 window (Window Maker, XWayland) */
    g_setenv("GDK_BACKEND", "x11", TRUE);
    /* Core X events, as the dock-app's own window gets them: the menu's
     * grab then takes the pointer and the keyboard with the click that
     * opened it (with XInput 2 it showed but got neither). */
    g_setenv("GDK_CORE_DEVICE_EVENTS", "1", TRUE);
    g_set_prgname(NAME);
    gdk_set_program_class("DockApp");

    const char *self_id = g_getenv("BTD_DOCKAPP_ID");
    d->app = gtk_application_new(self_id && g_application_id_is_valid(self_id) ? self_id : SELF_APP_ID,
                                 G_APPLICATION_HANDLES_COMMAND_LINE);
    const GOptionEntry entries[] = {
        { "toggle", 0, 0, G_OPTION_ARG_NONE, NULL, N_("Show or hide the Tabbed Dock (start it if needed)"), NULL },
        { "start", 0, 0, G_OPTION_ARG_NONE, NULL, N_("Start the Tabbed Dock"), NULL },
        { "stop", 0, 0, G_OPTION_ARG_NONE, NULL, N_("Stop the Tabbed Dock"), NULL },
        { "restart", 0, 0, G_OPTION_ARG_NONE, NULL, N_("Restart the Tabbed Dock"), NULL },
        { "preferences", 'p', 0, G_OPTION_ARG_NONE, NULL, N_("Open the Tabbed Dock preferences"), NULL },
        { "set", 0, 0, G_OPTION_ARG_STRING, NULL, N_("Change a preference, e.g. alignment=center"), "KEY=VALUE" },
        { NULL },
    };
    g_application_add_main_option_entries(G_APPLICATION(d->app), entries);
    g_signal_connect(d->app, "startup", G_CALLBACK(on_startup), d);
    g_signal_connect(d->app, "command-line", G_CALLBACK(on_command_line), d);
    int status = g_application_run(G_APPLICATION(d->app), argc, argv);
    g_object_unref(d->app);
    return status;
}
