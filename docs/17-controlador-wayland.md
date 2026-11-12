# 17 — Controlador "travado" no Wayland (10/11/2026)

Sintoma: na sessão Wayland, o ladrilho do Dock com Abas no WMDock não
respondia às setas nem ao duplo clique.

Evidência: não havia processo `big-wm-tabbed-dock-dockapp`. O ladrilho era
o que o wlmaker reserva para o dockapp acoplado (com o ícone dele), sem
programa por trás. O journal do login dizia
`big-wm-tabbed-dock-dockapp já está aberto nesta sessão`.

Causa (fora deste repositório): o `big-wm-dockapps` do big-wm procura um
dockapp "já aberto" com `pgrep -x` nos 15 primeiros caracteres do nome (o
`comm` do kernel). `big-wm-tabbed-dock-dockapp` e `big-wm-tabbed-dock` (a
barra) têm o mesmo `comm`, `big-wm-tabbed-d`: com a barra rodando, o
controlador nunca era iniciado. Corrigido no big-wm (o nome completo é
conferido; teste em `tests/run.sh`).

Este programa não tinha defeito: com o controlador rodando, no wlmaker
aninhado, depois de três trocas de tema,
- ◀ ▶ percorrem Direita, Centro, Esquerda, Largura total;
- ▲ ▼ trocam a borda (cinco vezes seguidas, com o Dock do wlmaker descendo
  e subindo pela reserva de espaço da barra);
- o duplo clique no ícone abre as Preferências; o botão direito, o menu;
- a barra encerrada e iniciada de novo volta a ser comandada (nome vigiado
  no D-Bus, sem polling);
- barra e controlador trocam de tema na hora (monitor de arquivo).

Cuidado para testes: `pgrep -x big-wm-tabbed-d` casa com os dois
programas; e cliques injetados com `xdotool` direto no XWayland (XTEST)
não passam pelo wlmaker, que só ressincroniza as posições X dos dockapps
quando o ponteiro do Wayland entra num ladrilho — teste pelo ponteiro do
compositor.
