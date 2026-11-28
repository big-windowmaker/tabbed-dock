/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Bounded LRU cache of rendered icons (cairo surfaces at the monitor scale).
 * Icon names are resolved through the user's GTK icon theme (which includes
 * ~/.local/share/icons, ~/.icons and the Flatpak export dirs), absolute
 * paths are validated before loading, and file items use the content-type
 * icon. The cache is flushed when the theme changes.
 */
#pragma once

#include <gtk/gtk.h>

#include "btd-desktop.h"
#include "btd-model.h"
#include "btd-windows.h"

G_BEGIN_DECLS

typedef struct _BtdIconCache BtdIconCache;

#define BTD_ICON_CACHE_MAX 128

BtdIconCache *btd_icon_cache_new(GtkIconTheme *theme, guint max_entries);
void btd_icon_cache_free(BtdIconCache *cache);
void btd_icon_cache_clear(BtdIconCache *cache);
guint btd_icon_cache_size(BtdIconCache *cache);

/* Returns a borrowed surface (valid until the next lookup may evict it;
 * draw it immediately). Never NULL: falls back to a generic icon. */
cairo_surface_t *btd_icon_cache_lookup_name(BtdIconCache *cache, const char *icon, int size, int scale);
/* Icon of running windows (BtdWindow *) with no .desktop icon: a theme icon
 * named after the app ID, the WM_CLASS class or instance (also in lower
 * case) or the program; else the window's own icon (X11 _NET_WM_ICON);
 * else the generic icon. Never NULL; borrowed like the lookups above. */
cairo_surface_t *btd_icon_cache_lookup_windows(BtdIconCache *cache, GPtrArray *windows, int size, int scale);
cairo_surface_t *btd_icon_cache_lookup_item(BtdIconCache *cache, const BtdItem *item, BtdDesktopIndex *index,
                                            int size, int scale);

G_END_DECLS
