#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Static rules (they came with the dock from big-wm's tests/run.sh):
# the autostart stays in the Big WindowMaker sessions, nothing launches
# applications through a shell, the catalogs are valid and complete, and
# the installation only writes where it should. TAP.
#
#   test-static.sh SOURCE_DIR BUILD_DIR
set -u
src=$1 build=$2
n=0 failed=0
ok() { n=$((n + 1)); echo "ok $n - $1"; }
not_ok() { n=$((n + 1)); failed=1; echo "not ok $n - $1"; }
echo "1..5"

f=$src/data/big-wm-tabbed-dock.desktop.in
if grep -q '^OnlyShowIn=' "$f" && ! grep -q '^X-GNOME-Autostart-Phase=' "$f" &&
	! grep '^OnlyShowIn=' "$f" | grep -q -v -E '^OnlyShowIn=((WindowMaker|wlmaker);)+$'; then
	ok "autostart only in WindowMaker/wlmaker, no X-GNOME-Autostart-Phase"
else
	not_ok "autostart: OnlyShowIn must list only WindowMaker/wlmaker"
fi

if grep -n -E '\b(system|popen|execl|g_spawn_command_line_(async|sync))[[:space:]]*\(|"(/bin/)?sh"[[:space:]]*,[[:space:]]*"-c"' \
	"$src"/src/*/*.c; then
	not_ok "no shell to launch applications"
else
	ok "no shell to launch applications"
fi

bad=0
for po in "$src"/po/*.po; do
	msgfmt --check -o /dev/null "$po" || bad=1
	stats=$(LC_ALL=C msgfmt --statistics -o /dev/null "$po" 2>&1)
	case $stats in *untranslated* | *fuzzy*) echo "# $po: $stats"; bad=1 ;; esac
done
[ "$bad" = 0 ] && ok "catalogs valid and complete" || not_ok "catalogs valid and complete"

dest=$(mktemp -d)
trap 'rm -rf "$dest"' EXIT
if meson install -C "$build" --destdir "$dest" --no-rebuild >/dev/null 2>&1; then
	# any prefix (/usr in the package, /usr/local by default)
	allowed='^/usr(/local)?/(bin|lib/systemd/user|share/(applications|icons/hicolor|locale|doc|wlmaker/Dockapps\.d))/|^(/usr/local)?/etc/xdg/autostart/'
	out=$(find "$dest" \( -type f -o -type l \) | sed "s#^$dest##" | grep -v -E "$allowed")
	if [ -z "$out" ]; then ok "installed only in the expected paths"; else not_ok "installed elsewhere: $out"; fi
	has() { [ -n "$(find "$dest" -path "*$1" -print -quit)" ]; }
	if has /share/locale/pt_BR/LC_MESSAGES/big-wm-tabbed-dock.mo &&
		has /share/icons/hicolor/scalable/apps/big-wm-tabbed-dock.svg &&
		has /share/wlmaker/Dockapps.d/big-wm-tabbed-dock-dockapp.plist &&
		has /bin/big-wm-tabbed-dock-dockapp; then
		ok "translation, icon and wlmaker menu installed"
	else
		not_ok "translation, icon or wlmaker menu missing"
	fi
else
	not_ok "meson install"
	not_ok "translation, icon and wlmaker menu installed"
fi
exit $failed
