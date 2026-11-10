/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Control API of the running dock: stateful GActions of the GtkApplication,
 * exported by GApplication on the session bus (org.gtk.Actions at the
 * application's object path). The dock-app controller
 * (big-wm-tabbed-dock-dockapp) and scripts use them; the states follow every
 * change, wherever it comes from (the preferences window, the dock's own
 * menu, a reload of the configuration), so a client never polls.
 *
 *   alignment      s  "fill", "right", "center" or "left"
 *   alignment-step i  next (+1) or previous (-1) alignment, in the order
 *                     fill → right → center → left → fill
 *   edge           s  "top" or "bottom"
 *   monitor        s  "primary", "all", "all-independent", or a monitor name
 *   monitors       as the choices, "id\tlabel" (read only; follows hotplug)
 *   style          s  "classic", "tiles" or "modern"
 *   autohide       b
 *   reserve-space  b
 *   visible        b  the shelf unfolded (FALSE: folded to the strip)
 *   preferences       opens (or raises) the preferences window
 *   quit              saves and quits
 */
#pragma once

#include "btd-dock.h"

G_BEGIN_DECLS

/* Adds the actions to the application, before it runs (so they are on the
 * bus from the registration on). */
void btd_control_install(GtkApplication *app);
/* The dock the actions act on (the first one; the others follow the saved
 * configuration). NULL while there is none. */
void btd_control_attach(BtdDock *dock);
/* Brings the action states in line with the dock (after any change). */
void btd_control_sync(void);

G_END_DECLS
