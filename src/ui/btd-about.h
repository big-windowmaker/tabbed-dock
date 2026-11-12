/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * The Tabbed Dock's About window, the same from the dock's menu and from
 * the controller's.
 */
#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

/* Shows the About window (one at a time; raised if already open). */
void btd_about_show(GtkWindow *parent);

G_END_DECLS
