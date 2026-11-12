# 04 — O ladrilho

Um dockapp clássico: janela líder *withdrawn* com `WM_CLASS
big-wm-tabbed-dock-dockapp.DockApp` e uma *icon window* de 64×64, que o
Window Maker (e o wlmaker, pelo XWayland) adota no Dock.

Desde o redesenho de 11/2026 ([16](16-auditoria-visual-dockapp.md) a
[21](21-relatorio-final-redesign-dockapp.md)): um painel de instrumentos
OPENSTEP, na linha do YAWMPPP e do WMXRES.

```
 4                                                  60
 ┌──────────────────── visor (4,4 56×41) ───────────┐   clique duplo: Preferências
 │  ▬▬▬  prateleira em miniatura (borda, alinhamento) │   clique: estado; roda: ◀ ▶
 │            tabbed-dock.svg 44×33 (10,9)           │
 └───────────────────────────────────────────────────┘ 45
 [◀ 4,47][▶ 16,47]  ● 29,50  [▲ 36,47][▼ 48,47]       botões 12×13
```

- Desenho em `src/dockapp/btd-dockapp-draw.c` (Cairo, sem X11, testado em
  superfícies de imagem por `tests/test-dockapp-draw.c`); geometria e regiões
  em `btd-dockapp-state.c`; o X11 só copia o pixmap para a janela.
- O SVG é rasterizado **uma vez**, no tamanho do visor, ao iniciar; cada
  redesenho só compõe a imagem guardada.
- Botões em relevo do Window Maker; a borda atual fica **afundada** (botão de
  estado), com a seta na cor de destaque. Passar o ponteiro clareia o botão;
  apertar o afunda e desloca a seta 1 px.
- Lâmpada: verde-água com o dock rodando, âmbar com ele oculto, apagada com
  ele parado (o visor escurece, como uma tela sem sinal).
- Uma **forma** (XShape) deixa a margem, o vão sob o visor e o redor da
  lâmpada transparentes: o ladrilho do tema aparece por trás.
- Dicas (*tooltips*) ao lado do ladrilho, nunca por cima (uma dica por cima
  roubava o clique seguinte — corrigido posicionando a janela no
  `size-allocate`).
- Avisos curtos (1,6 s) depois de cada ação ("Centro", "Em cima"...).
- Tema claro/escuro: segue `~/.config/big-wm/theme` (`GFileMonitor`), troca na
  hora. O visor é escuro nos dois temas; a moldura e os botões são cinza
  metálico no claro e grafite no escuro.

## Entrada

O GDK não entrega eventos de uma *icon window* que ele não criou do jeito
dele; o controlador lê os eventos num filtro do GDK (`GDK_FILTER_REMOVE`) e
usa `GDK_CORE_DEVICE_EVENTS=1` para o menu GTK receber cliques e teclas no X
puro.

- Botão 1: ação da região (seta, ícone). Clique duplo no ícone: Preferências.
  Dois cliques rápidos numa seta são dois passos, não um clique duplo.
- Roda: ◀ ▶.
- Botão 3: o menu (ver [06](06-menu-contexto.md)).
- Arrastar com o botão 1 fora das setas: repassado ao Window Maker (o ladrilho
  continua arrastável no Dock).
