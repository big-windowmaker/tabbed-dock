# 04 — O ladrilho

Um dockapp clássico: janela líder *withdrawn* com `WM_CLASS
big-wm-tabbed-dock-dockapp.DockApp` e uma *icon window* de 64×64, que o
Window Maker (e o wlmaker, pelo XWayland) adota no Dock.

```
 ┌────────────────────────────┐
 │        ▲  (4,4 56×12)      │   ↑  borda de cima
 │ ◀ ┌──────────────────┐ ▶   │   ◀ ▶ (4,17 / 48,17, 12×30)
 │   │  tabbed-dock.svg │     │   ícone (17,17 30×30)
 │   └──────────────────┘     │
 │        ▼  (4,48 56×12)     │   ↓  borda de baixo
 └────────────────────────────┘
```

- Fundo desenhado com Cairo num pixmap do X (`cairo-xlib`), botões em relevo
  no estilo OPENSTEP. Uma **forma** (XShape) deixa as margens e os vãos
  transparentes: o ladrilho do tema aparece por trás.
- Estado visível: a seta da borda atual em destaque e uma prateleira em
  miniatura na posição do alinhamento atual; dock oculto = ícone esmaecido.
- Dicas (*tooltips*) ao lado do ladrilho, nunca por cima (uma dica por cima
  roubava o clique seguinte — corrigido posicionando a janela no
  `size-allocate`).
- Avisos curtos (1,6 s) depois de cada ação ("Centro", "Em cima"...).
- Tema claro/escuro: segue `~/.config/big-wm/theme` (`GFileMonitor`), troca na
  hora.

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
