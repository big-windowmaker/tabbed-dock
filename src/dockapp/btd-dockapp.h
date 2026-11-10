/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * The dock-app controller: geometry, state and texts, without X11 or GTK
 * (unit-tested in tests/test-dockapp.c).
 */
#pragma once

#include "btd-config.h"

G_BEGIN_DECLS

#define BTD_DOCKAPP_SIZE 64
#define BTD_DOCKAPP_ICON 22 /* tabbed-dock.svg, in the well */

typedef enum {
    BTD_REGION_NONE,
    BTD_REGION_TOP,    /* ↑ edge: top */
    BTD_REGION_BOTTOM, /* ↓ edge: bottom */
    BTD_REGION_LEFT,   /* ◀ previous alignment */
    BTD_REGION_RIGHT,  /* ▶ next alignment */
    BTD_REGION_ICON,   /* double click: preferences */
} BtdRegion;

typedef struct {
    int x, y, width, height;
} BtdRect;

/* What the dock-app shows: the running dock's action states, or the
 * configuration file. */
typedef struct {
    BtdAlign align;
    BtdEdge edge;
    BtdStyle style;
    char *monitor;
    char **monitors; /* "id\tlabel", from the dock; NULL: not known */
    gboolean autohide, reserve_space, visible;
} BtdDockappState;

typedef struct {
    char *key;      /* action name: alignment, edge, monitor, style, autohide, reserve-space */
    GVariant *value;
} BtdDockappSetting;

void btd_dockapp_region_rect(BtdRegion region, BtdRect *rect);
BtdRegion btd_dockapp_region_at(int x, int y);
/* The little shelf in the well: x and width of a bar in [x0, x0 + width). */
void btd_dockapp_bar_geometry(BtdAlign align, int x0, int width, int *bx, int *bw);

void btd_dockapp_state_clear(BtdDockappState *st);
void btd_dockapp_state_from_prefs(BtdDockappState *st, const BtdPrefs *prefs);
/* An action state of the dock ("alignment", "monitors"...). FALSE: unknown
 * name or type, the state is untouched. */
gboolean btd_dockapp_state_set(BtdDockappState *st, const char *name, GVariant *value);
/* The same change on preferences (no dock running: the file). */
gboolean btd_dockapp_apply_to_prefs(BtdPrefs *prefs, const char *name, GVariant *value);

const char *btd_dockapp_align_label(BtdAlign align);
const char *btd_dockapp_edge_label(BtdEdge edge);
const char *btd_dockapp_style_label(BtdStyle style);
const char *btd_dockapp_region_tooltip(BtdRegion region);
char *btd_dockapp_summary(const BtdDockappState *st, gboolean running);
/* "id\tlabel" choices of the Monitor menu (the dock's list, or the
 * generic ones plus the current choice). */
char **btd_dockapp_monitor_choices(const BtdDockappState *st);

/* --set KEY=VALUE */
gboolean btd_dockapp_parse_setting(const char *text, char **key, GVariant **value);

/* wlmaker's right-click menu (schema in Dockapps.d): the values it shows,
 * the monitor choices, and what the user picked there. */
char *btd_dockapp_wlm_prefs_text(const BtdDockappState *st);
char *btd_dockapp_wlm_choices_text(const BtdDockappState *st);
GPtrArray *btd_dockapp_wlm_changes(const BtdDockappState *st, const char *text); /* of BtdDockappSetting* */

/* tabbed-dock.svg (BTD_DOCKAPP_ICON_FILE for tests) */
char *btd_dockapp_icon_path(void);

G_END_DECLS
