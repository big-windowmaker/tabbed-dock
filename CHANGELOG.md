# Changelog

Formato livre, uma seção por versão; datas no fuso de Brasília. O pacote
`big-wm-tabbed-dock` usa `epoch=1` desde 0.2.0 (antes, vinha no big-wm com
versão por data).

## Não lançado

- Abas sem os "triângulos" de papel de parede entre elas: as abas vizinhas
  se sobrepõem por uma inclinação (a da esquerda na frente, como abas de
  pasta), nos estilos clássico e tiles. Lado a lado, as laterais inclinadas
  deixavam um vão em V entre cada duas abas. O clique segue o que está
  desenhado na frente.

## 0.3.0 — 2026-11-11

- Controlador: novo visual, um painel de instrumentos OPENSTEP. O
  `tabbed-dock.svg` passa a 44 px (eram 22) num visor escuro, com a prateleira
  em miniatura na borda e no alinhamento atuais; botões `[◀][▶] ● [▲][▼]`; a
  borda atual fica afundada; lâmpada de estado (rodando, oculto, parado).
  Desenho em `btd-dockapp-draw.c`, testado sem X11 (`dockapp-draw`); o SVG é
  rasterizado uma vez (≈ 40 % menos CPU sob uso). Docs 16 a 21.
- Janela **Sobre** no menu do dock (nova) e no do controlador: versão,
  mantenedores (Rafael Ruscher, Barnabé Di Kartola), agradecimentos (Window
  Maker, wlmaker, BigLinux, BigCommunity), licença e repositório.
- PKGBUILD: mantenedores.

## 0.2.1 — 2026-11-10

- Largura total: um appicon do Window Maker sobre a prateleira não tira mais
  a largura de um ícone.
- Autostart: o dock que cai volta mesmo com aplicativos abertos
  (`ExitType=main`).
- Docs: o controlador que nunca iniciava no Wayland (nome de 15 caracteres
  do processo).

## 0.2.0 — 2026-11-09

- Repositório próprio (antes dentro do big-wm), com README, LICENSE, CREDITS,
  PKGBUILD e CI.
- Controlador DockApp: alinhamento, borda, monitor e inicialização pelo Dock
  do Window Maker e do wlmaker; API de controle por D-Bus.
