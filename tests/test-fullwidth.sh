#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# "Full width" (X11) with Window Maker's icons around the shelf, on a
# private X server: an application icon (appicon) resting on top of the
# shelf must not take its width off the shelf (it took one icon's width,
# 2026-11-10), while a Dock tile that enters the shelf keeps its column
# free. Window Maker is replaced by square override-redirect windows
# (x11-tile), which is what its icons are. TAP; skipped without Xvfb or
# dbus-daemon.
#
#   test-fullwidth.sh DOCK X11-TILE
set -u
dock=$1 tile=$2
n=0 failed=0
ok() { n=$((n + 1)); echo "ok $n - $1"; }
not_ok() { n=$((n + 1)); failed=1; echo "not ok $n - $1"; }
check() { if [ "$2" = "$3" ]; then ok "$1"; else not_ok "$1 (got '$2', want '$3')"; fi; }

if ! command -v Xvfb >/dev/null || ! command -v dbus-daemon >/dev/null || ! command -v gdbus >/dev/null; then
	echo "1..0 # SKIP Xvfb, dbus-daemon or gdbus missing"
	exit 0
fi
tmp=$(mktemp -d)
pids=()
tiles=()
cleanup() {
	for p in "${tiles[@]}" "${pids[@]}"; do kill "$p" 2>/dev/null; done
	wait 2>/dev/null
	rm -rf "$tmp"
}
trap cleanup EXIT

W=1024 H=768
exec 3< <(Xvfb -displayfd 1 -screen 0 ${W}x${H}x24 -nolisten tcp -noreset 2>/dev/null & echo "PID $!")
read -r line <&3
pids+=("${line#PID }")
read -r -t 10 disp <&3 || { echo "1..0 # SKIP Xvfb did not start"; exit 0; }
cat >"$tmp/bus.conf" <<'EOF'
<!DOCTYPE busconfig PUBLIC "-//freedesktop//DTD D-Bus Bus Configuration 1.0//EN"
 "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
<busconfig><type>session</type><listen>unix:tmpdir=/tmp</listen><auth>EXTERNAL</auth>
<policy context="default"><allow send_destination="*" eavesdrop="true"/><allow eavesdrop="true"/>
<allow own="*"/></policy></busconfig>
EOF
read -r bus bus_pid < <(dbus-daemon --config-file="$tmp/bus.conf" --fork --print-address=1 --print-pid=1 | tr '\n' ' ')
pids+=("$bus_pid")

export DISPLAY=:$disp DBUS_SESSION_BUS_ADDRESS=$bus GDK_BACKEND=x11 NO_AT_BRIDGE=1 GIO_USE_VFS=local
export HOME=$tmp XDG_CONFIG_HOME=$tmp/config XDG_DATA_HOME=$tmp/data XDG_RUNTIME_DIR=$tmp/run
export BTD_APPLICATION_ID=io.github.big_windowmaker.TabbedDockFullWidth
unset WAYLAND_DISPLAY
mkdir -p "$XDG_RUNTIME_DIR" "$XDG_CONFIG_HOME/big-tabbed-dock" && chmod 700 "$XDG_RUNTIME_DIR"
printf '[Dock]\nVersion=1\nPosition=bottom\nAlignment=fill\nReserveSpace=true\nAutoHide=false\n' \
	>"$XDG_CONFIG_HOME/big-tabbed-dock/dock.conf"
path=/io/github/big_windowmaker/TabbedDockFullWidth
wait_for() { for _ in $(seq 50); do eval "$1" && return 0; sleep 0.1; done; return 1; }
area() { "$dock" --print-state 2>/dev/null | grep -o 'wm_left=[0-9]* wm_right=[0-9]*' | head -n1; }
width() { "$dock" --print-state 2>/dev/null | sed -n 's/^geometry=\([0-9]*\)x.*/\1/p' | head -n1; }
height() { "$dock" --print-state 2>/dev/null | sed -n 's/^geometry=[0-9]*x\([0-9]*\).*/\1/p' | head -n1; }
add_tiles() {
	"$tile" "$@" >"$tmp/tile.out" &
	tiles+=($!)
	wait_for 'grep -q ready "$tmp/tile.out"'
}

echo "1..6"
"$dock" >"$tmp/dock.log" 2>&1 &
pids+=($!)
wait_for 'gdbus call --session --dest "$BTD_APPLICATION_ID" --object-path "$path" --method org.gtk.Actions.List >/dev/null 2>&1' ||
	{ not_ok "dock on the bus"; exit 1; }
check "full width with nothing around" "$(width) $(area)" "$W wm_left=0 wm_right=0"

# The shelf's body is its whole height here (no compositor, no shadow).
body=$(height)
# Window Maker puts an application's icon in the bottom-left corner of the
# work area: on top of the shelf. One, then a row of three.
add_tiles 48 "0,$((H - body - 48))"
sleep 0.5 # the dock waits 200 ms after Window Maker's icons move
check "an appicon on top of the shelf: still full width" "$(width) $(area)" "$W wm_left=0 wm_right=0"
add_tiles 48 "48,$((H - body - 48))" "96,$((H - body - 48))" "0,$((H - body - 96))"
sleep 0.5
check "several appicons: still full width" "$(width) $(area)" "$W wm_left=0 wm_right=0"

# The Dock's column reaching down into the shelf: kept free on the right.
add_tiles 64 "$((W - 64)),$((H - 128))" "$((W - 64)),$((H - 64))"
sleep 0.5
check "a Dock column in the shelf: its width left free" "$(width) $(area)" "$((W - 64)) wm_left=0 wm_right=64"

# The column goes away: full width again.
kill "${tiles[-1]}" 2>/dev/null
unset 'tiles[-1]'
sleep 0.5
check "the Dock column gone: full width again" "$(width) $(area)" "$W wm_left=0 wm_right=0"

# The Clip in the bottom-left corner (in the shelf): left free.
add_tiles 64 "0,$((H - 64))"
sleep 0.5
check "the Clip in the corner: its width left free" "$(width) $(area)" "$((W - 64)) wm_left=64 wm_right=0"
exit $failed
