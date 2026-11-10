# 12 — Wayland (wlmaker)

O controlador é um dockapp X11; no wlmaker roda pelo XWayland
(`GDK_BACKEND=x11`), como os outros dockapps clássicos do Big WindowMaker. O
dock em si é nativo (layer-shell).

## O que o wlmaker ganhou (pacote 31, branch `fix/dockapp-menu-overlay`)

- Menus de um dockapp (janelas X filhas do ladrilho) desenhados na camada de
  cima: antes ficavam atrás do Dock.
- A forma do ladrilho reaplicada quando o nó da cena muda.
- `Dockapps.d/<nome>.plist`: um pacote pode trazer o menu e as preferências do
  seu dockapp para o menu do ladrilho do wlmaker, com `TextDomain` (tradução
  pelo gettext do próprio pacote) e `ChoicesFile` (escolhas lidas de um
  arquivo, relativo ao diretório do plist).

## Espelho das preferências

O controlador grava `~/.config/wlmaker/big-wm-tabbed-dock-dockapp.plist` (os
valores atuais) e `.monitors.plist`, observa o arquivo e aplica no dock o que
o usuário escolheu no menu do wlmaker.

## Verificado (wlmaker 31 aninhado)

Ladrilho adotado; as quatro setas; clique duplo nas Preferências; o menu do
controlador no conteúdo; o menu do wlmaker na borda com as ações e
"Configurações…" com as preferências do dock; Alinhamento > Centro escolhido
ali muda o dock.

## Pendente

Teclado no menu GTK pelo XWayland (mouse funciona).
