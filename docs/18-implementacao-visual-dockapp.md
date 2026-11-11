# 18 — Implementação

```
 4                                                  60
 ┌──────────────────── visor 56×41 ─────────────────┐ 4
 │  ▬▬▬  prateleira em miniatura (borda/alinhamento) │
 │            tabbed-dock.svg 44×33                  │
 └───────────────────────────────────────────────────┘ 45
 [◀][▶]  ●  [▲][▼]   botões 12×13 em y 47, lâmpada 6×6       60
```

## Código

| Arquivo | Mudança |
|---|---|
| `src/dockapp/btd-dockapp-draw.c` / `.h` (novos) | A face inteira em Cairo, sem X11: visor, ícone, prateleira, botões, lâmpada, paletas |
| `src/dockapp/btd-dockapp-state.c`, `btd-dockapp.h` | Regiões novas; `btd_dockapp_led_rect`, `_icon_rect`, `_bar_rect`, `_shape` |
| `src/dockapp/btd-dockapp.c` | Usa o módulo; SVG rasterizado uma vez; forma de `btd_dockapp_shape`; *hover* redesenha só ao entrar/sair de um botão |
| `src/meson.build`, `tests/meson.build` | Biblioteca `btddockappdraw`; teste `dockapp-draw` |

Sem mudança: ações, ciclo de alinhamento, clique duplo, menu, roda,
arraste repassado ao Window Maker, dicas, D-Bus, arquivos do wlmaker,
configuração, traduções (nenhuma frase nova).

## Detalhes de acabamento

- Linhas de 1 px no meio-pixel e `CAIRO_ANTIALIAS_NONE` nas molduras; setas
  desenhadas pixel a pixel (▲▼ 8×4, ◀▶ 4×8), centradas no interior do
  botão, deslocadas 1 px quando o botão afunda.
- Visor afundado 2 px (cinza e preto em cima/esquerda, branco e cinza claro
  embaixo/direita). Prateleira: barra de 2 px "fósforo" com a linha voltada
  para o centro mais clara.
- Ícone: a área do desenho do documento 64×64 (viewBox 4..60 × 12..54, a
  prateleira e a sombra) em 44×33, em posição inteira (10,9). Rasterizado
  por librsvg **uma vez** ao iniciar; cada redesenho só compõe a imagem.
- Estados: normal; *hover* (face mais clara); apertado (afundado, seta
  deslocada); selecionado (borda atual afundada, face mais escura, seta na
  cor de destaque); dock oculto (lâmpada âmbar); parado (lâmpada apagada,
  visor a 42 %). Não há estado "desabilitado": sem o dock, os botões editam
  o arquivo, como antes.
- Temas: o visor é escuro nos dois; claro = cinza metálico do Window Maker,
  escuro = grafite com realces suaves (não é a inversão do claro).

## O SVG

`data/icons/tabbed-dock.svg` **não mudou**: a 44 px a prateleira, as abas e
os três ladrilhos se leem (ver capturas); afinar o desenho não traria ganho
visível e mexeria no ícone do pacote (o mesmo arquivo é o ícone do
aplicativo).
