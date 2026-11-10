# 09 — Posição no Dock e na tela

## O ladrilho logo acima da bandeja

| Sessão | Padrão (conta nova) | Conta existente |
|---|---|---|
| X11 | `WMState`: controlador em `0,2`, bandeja `0,3`, volume `0,4`, música `0,5`... | `big-wm-dock-controller x11` |
| Wayland | `State.plist`: `Dockapps = (wmclock, "big-wm-tabbed-dock-dockapp", "big-wm-tray", wmpulsemixer, wmusic, ...)` | `big-wm-dock-controller wayland` |

`big-wm-dock-controller` (no big-wm, `session/lib/`, chamado pelo
`startbigwm` / `startbigwm-wayland` se o controlador estiver instalado) roda
uma vez por sessão (marca em `~/.config/big-wm/migrations/`):

- X11: o controlador entra no lugar da bandeja e a bandeja e os de baixo
  descem um, até o primeiro lugar vago; a ordem do usuário fica; cada lista do
  Dock por altura de tela é ajustada; se a coluna não cabe na tela, não mexe e
  tenta no próximo login.
- Wayland: logo antes de `big-wm-tray` no `Dockapps.plist` (copiando a
  posição relativa aos lançadores) e no `State.plist`.
- Quem já tem o controlador (Dock, Clip, gaveta), quem o tirou depois da
  migração ou o pôs em `Removed`, e quem não tem a bandeja no Dock: nada muda.

Testes: `tests/dockapps/test_dock_controller.py` no big-wm (6 casos: acima da
bandeja, ordem do usuário com vão, já tem / sem bandeja, tela baixa, Wayland,
Wayland com `Removed`), mais as verificações do `tests/run.sh` do Dock padrão.

**Não verificado numa sessão real nem em arquivos de uma conta real**: só em
arquivos de teste e no laboratório.

## A prateleira e o Dock do Window Maker (X11)

A prateleira desconta as colunas do Dock e do Clip do Window Maker (janelas
override-redirect quadradas de 32 a 160 px, encostadas na lateral, na faixa da
prateleira), em todos os alinhamentos. Recalcula quando uma janela da raiz é
mapeada, desmapeada ou movida (200 ms de espera). `--print-state` mostra
`x11_area offset=… wm_left=… wm_right=…`.
