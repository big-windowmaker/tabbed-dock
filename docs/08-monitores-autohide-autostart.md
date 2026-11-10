# 08 — Monitores, ocultar, reservar espaço, inicialização

## Monitores

- A ação `monitors` publica os monitores ligados (`id\trótulo`, do GDK) e
  muda quando um monitor entra ou sai; o menu **Monitor** é montado dela na
  hora de abrir.
- `monitor`: `primary`, `all` (espelhado), `all-independent` ou um conector.
  Quando um monitor entra ou sai, o dock se reposiciona (`monitor-added` /
  `monitor-removed`, comportamento que já existia).
- No wlmaker, o controlador grava a lista em
  `~/.config/wlmaker/big-wm-tabbed-dock-dockapp.monitors.plist`, que o menu
  do ladrilho do wlmaker usa (`ChoicesFile`).

**Não testado com vários monitores reais nem com conexão/desconexão a
quente.** O Xvfb e a sessão aninhada têm uma saída só (no wlmaker, `X11-1`).

## Ocultar automaticamente / reservar espaço

Estados `autohide` e `reserve-space`, no menu e no `--set`. O efeito é o do
dock (strut no X11, zona exclusiva da layer-shell no Wayland). Testado pelo
D-Bus e no menu (X11).

## Inicialização

- O dock: autostart `big-wm-tabbed-dock.desktop` (só `WindowMaker;wlmaker;`).
- O controlador: entra no Dock do Window Maker com `AutoLaunch = YES` (WMState
  padrão) e no wlmaker pelo `big-wm-dockapps` (XWayland).
- Menu **Inicialização**: iniciar, parar e reiniciar o dock agora, e os modos
  da próxima sessão (Dock com Abas e Dock clássico, só um, só o outro,
  restaurar o padrão) — os mesmos da seção Inicialização das Preferências.
