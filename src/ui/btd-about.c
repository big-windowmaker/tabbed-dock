/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * The Tabbed Dock's About window: the icon, the name and the installed
 * version, what it is, the maintainers of Big WindowMaker, the projects it
 * builds on, the license and the repository. GTK's About window, which
 * follows the GTK theme (light or dark, X11 or Wayland).
 */
#include "btd-about.h"

#include <glib/gi18n.h>

static GtkWidget *about_window;

void
btd_about_show(GtkWindow *parent)
{
    if (about_window) {
        gtk_window_present(GTK_WINDOW(about_window));
        return;
    }
    static const char *maintainers[] = { "Rafael Ruscher <rruscher@gmail.com>",
                                         "Barnabé Di Kartola <barnabedikartola@gmail.com>", NULL };
    /* "Name URL": GTK shows the name as a link */
    static const char *thanks[] = { "Alfredo Kojima — Window Maker https://www.windowmaker.org/",
                                    "Philipp Kaeser — wlmaker https://github.com/phkaeser/wlmaker",
                                    "Bruno Gonçalves — BigLinux https://www.biglinux.com.br/",
                                    "Tales A. Mendonça — BigCommunity", NULL };

    GtkAboutDialog *a = GTK_ABOUT_DIALOG(gtk_about_dialog_new());
    about_window = GTK_WIDGET(a);
    g_object_add_weak_pointer(G_OBJECT(a), (gpointer *)&about_window);
    if (parent)
        gtk_window_set_transient_for(GTK_WINDOW(a), parent);
    gtk_about_dialog_set_program_name(a, _("Tabbed Dock"));
    gtk_about_dialog_set_version(a, BTD_VERSION);
    gtk_about_dialog_set_logo_icon_name(a, "big-wm-tabbed-dock");
    gtk_about_dialog_set_comments(
        a, _("A horizontal dock with tabs for Window Maker (X11) and wlmaker (Wayland), after the shelf of "
             "OPENSTEP 4.0 beta, and its controller on the Dock.\nPart of Big WindowMaker."));
    gtk_about_dialog_set_copyright(a, "© 2026 Big WindowMaker");
    gtk_about_dialog_set_license_type(a, GTK_LICENSE_GPL_2_0);
    gtk_about_dialog_set_website(a, "https://github.com/big-windowmaker/tabbed-dock");
    gtk_about_dialog_set_website_label(a, _("Repository on GitHub"));
    gtk_about_dialog_add_credit_section(a, _("Maintainers of Big WindowMaker"), maintainers);
    gtk_about_dialog_add_credit_section(a, _("Special thanks"), thanks);
    g_signal_connect(a, "response", G_CALLBACK(gtk_widget_destroy), NULL);
    gtk_widget_show(GTK_WIDGET(a));
}
