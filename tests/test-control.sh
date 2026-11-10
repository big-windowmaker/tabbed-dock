#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Control API of the dock (D-Bus actions, btd-control.c) and the dock-app
# controller's command line (--set), on a private X server and session bus:
# the arrows' order, the edge, persistence in dock.conf, an invalid value,
# and the configuration edited with no dock running. TAP; skipped without
# Xvfb or dbus-daemon.
#
#   test-control.sh DOCK DOCKAPP
set -u
dock=$1 dockapp=$2
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
cleanup() {
	for p in "${pids[@]}"; do kill "$p" 2>/dev/null; done
	wait 2>/dev/null
	rm -rf "$tmp"
}
trap cleanup EXIT

exec 3< <(Xvfb -displayfd 1 -screen 0 1024x768x24 -nolisten tcp -noreset 2>/dev/null & echo "PID $!")
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
export BTD_APPLICATION_ID=io.github.big_windowmaker.TabbedDockTest
export BTD_DOCKAPP_ID=io.github.big_windowmaker.TabbedDockTest.DockApp
export BTD_DOCK_BINARY=$dock
unset WAYLAND_DISPLAY
mkdir -p "$XDG_RUNTIME_DIR" && chmod 700 "$XDG_RUNTIME_DIR"
conf=$XDG_CONFIG_HOME/big-tabbed-dock/dock.conf
path=/io/github/big_windowmaker/TabbedDockTest
call() { gdbus call --session --dest "$BTD_APPLICATION_ID" --object-path "$path" --method "org.gtk.Actions.$1" "${@:2}" >/dev/null; }
state() { "$dock" --print-state 2>/dev/null | grep -o 'edge=[a-z]* align=[a-z]*' | head -n1; }
key() { sed -n "s/^$1=//p" "$conf" 2>/dev/null; }
wait_for() { for _ in $(seq 50); do eval "$1" && return 0; sleep 0.1; done; return 1; }

echo "1..12"
# No dock: the controller edits the file (the dock's defaults, nothing lost)
"$dockapp" --set alignment=left
wait_for '[ "$(key Alignment)" = left ]'
check "no dock: --set alignment writes dock.conf" "$(key Alignment)" left
check "no dock: the default tabs are kept" "$(grep -c '^\[Tab ' "$conf")" 4

"$dock" >"$tmp/dock.log" 2>&1 &
pids+=($!)
wait_for 'gdbus call --session --dest "$BTD_APPLICATION_ID" --object-path "$path" --method org.gtk.Actions.List >/dev/null 2>&1' ||
	{ not_ok "dock on the bus"; exit 1; }
check "the dock starts with the file's alignment" "$(state)" "edge=bottom align=left"
call Activate alignment-step "[<int32 1>]" "{}"
call Activate alignment-step "[<int32 1>]" "{}"
check "▶ ▶ from left: right" "$(state)" "edge=bottom align=right"
call Activate alignment-step "[<int32 -1>]" "{}"
check "◀ from right: fill" "$(state)" "edge=bottom align=fill"
call SetState edge "<'top'>" "{}"
check "↑: top" "$(state)" "edge=top align=fill"
call SetState edge "<'diagonal'>" "{}"
check "an invalid edge is refused" "$(state)" "edge=top align=fill"
wait_for '[ "$(key Position)" = top ]'
check "the change is saved (Position)" "$(key Position)" top
# The controller's command line talks to the running dock
"$dockapp" --set alignment=center
wait_for '[ "$(state)" = "edge=top align=center" ]'
check "--set alignment=center reaches the dock" "$(state)" "edge=top align=center"
"$dockapp" --set edge=bottom
wait_for '[ "$(state)" = "edge=bottom align=center" ]'
check "--set edge=bottom reaches the dock" "$(state)" "edge=bottom align=center"
desc=$(gdbus call --session --dest "$BTD_APPLICATION_ID" --object-path "$path" --method org.gtk.Actions.Describe alignment)
check "the alignment state is on the bus" "$(echo "$desc" | grep -o "'center'")" "'center'"
"$dock" --quit
wait_for '! "$dock" --print-state >/dev/null 2>&1'
check "saved on quit" "$(key Alignment)/$(key Position)" "center/bottom"
exit $failed
