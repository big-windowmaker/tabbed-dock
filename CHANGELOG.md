# Changelog

Formato livre, uma seção por versão; datas no fuso de Brasília. O pacote
`big-wm-tabbed-dock` usa `epoch=1` desde 0.2.0 (antes, vinha no big-wm com
versão por data).

## 0.3.3 — 2026-11-27

- Aplicativo aberto sem `.desktop` (zenity, um script, um jogo, um
  programa do Wine) aparecia com o ícone genérico "exec", enquanto o
  appicon do Window Maker ao lado mostrava o dele. Agora vem primeiro um
  ícone do tema com o nome da janela (app ID, classe ou instância do
  `WM_CLASS`, também em minúsculas, ou o programa) e, no X11, o ícone da
  própria janela (`_NET_WM_ICON`). No Wayland vale o do tema (o protocolo
  não traz os pixels). Teste `tests/test-icons.c`.

## 0.3.2 — 2026-11-20

- Aplicativo instalado com o dock já rodando: a janela aberta aparecia sem
  ícone e o menu do botão direito não trazia “Manter em” nem “Adicionar à
  aba” até o dock ser reiniciado (visto com o Big Remote Play). O
  `GAppInfoMonitor` do GIO só observa as pastas de aplicativos depois de uma
  consulta ao banco de aplicativos do GIO, e para depois de cada aviso até a
  próxima consulta; o dock lê os `.desktop` por conta própria e nunca
  consultava. Agora consulta ao iniciar e a cada aviso, e refaz o índice a
  cada instalação, remoção ou atualização.

## 0.3.1 — 2026-11-14

- Abas sem os "triângulos" de papel de parede entre elas: as abas vizinhas
  se sobrepõem por uma inclinação (a da esquerda na frente, como abas de
  pasta), nos estilos clássico e tiles. Lado a lado, as laterais inclinadas
  deixavam um vão em V entre cada duas abas. O clique segue o que está
  desenhado na frente.
- Corrigida a queda do dock (SIGSEGV em `g_str_hash` dentro do desenho)
  quando uma janela sumia ou trocava de `WM_CLASS` enquanto o dock se
  redesenhava, por exemplo ao abrir e fechar o instalador na ISO live: a
  janela era liberada antes de o dock refazer seus grupos. Agora o
  rastreador só a libera depois do aviso (`tests/test-windows.c`).

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
