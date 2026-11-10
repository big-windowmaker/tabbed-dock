# 14 — Testes

`meson test -C build`: 10 conjuntos, todos passam.

| Conjunto | O quê |
|---|---|
| config, model, desktop, launch, coexist, wmattrs | os do dock (já existiam) |
| dockapp | lógica do controlador: regiões, ciclo, rótulos, dicas, `chave=valor`, plists do wlmaker (8 subtestes) |
| control | `test-control.sh`: Xvfb + dbus-daemon próprio; ações pelo D-Bus, estado no arquivo e no `--print-state`, controlador com e sem dock (12 TAP) |
| static | `test-static.sh`: autostart só nas sessões certas, nenhum shell, catálogos completos, caminhos instalados (5 TAP) |
| sigterm-startup | SIGTERM durante a inicialização |

No big-wm: `tests/dockapps/test_dock_controller.py` (6) e o `tests/run.sh`
completo (todas as verificações passaram).

No wlmaker: `ctest` 29/29, `dockapp_prefs` 4/4.

## Manual, no laboratório

X11: Xvfb + Window Maker com a configuração do Big WindowMaker, cliques por
`xdotool` no ladrilho adotado, capturas. Wayland: wlmaker 31 aninhado,
HOME descartável, barramento próprio, `systemctl`/`loginctl`/desligamento
bloqueados.

## Não testado

Sessão real (login no gerenciador de sessões), conta real migrada,
vários monitores físicos, conexão de monitor a quente, reinicialização real.
