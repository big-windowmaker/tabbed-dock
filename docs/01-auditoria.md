# 01 — Auditoria (antes do 2.0)

Estado do Dock com Abas em 9 de novembro de 2026, antes da separação, no
`big-wm` (tag local `pre-tabbed-dock-split-2026-11-09`, commit `5d1a3ca`).

## Onde estava

| O quê | Onde (no big-wm) |
|---|---|
| Código (GTK 3, C, meson) | `tabbed-dock/src/{core,model,backend,ui}` |
| Dados, `.desktop`, drop-in do big-wm | `tabbed-dock/data/` |
| Traduções (domínio `big-wm-tabbed-dock`) | `tabbed-dock/po/` |
| Testes (C/TAP, scripts de sessão) | `tabbed-dock/tests/` |
| Documentação | `docs/tabbed-dock/` |
| Capturas | `docs/screenshots/tabbed-dock/` |
| Ícone | `themes/icons/svg/tabbed-dock.svg` |
| Pacote | `big-wm-tabbed-dock`, um dos pacotes do `pkgbuild/PKGBUILD` do big-wm (versão por data, 26.11.x) |
| Alvos do Makefile, verificações do `tests/run.sh` (seção 8) | big-wm |

## O que funcionava

- Dock com abas nas duas sessões: X11 (EWMH, forma da janela, `_NET_WM_STRUT_PARTIAL`) e
  Wayland (layer-shell, `wlr-foreign-toplevel`).
- Preferências (janela própria e no Configurações do big-wm): posição (em cima/embaixo),
  alinhamento (largura total, esquerda, centro, direita), monitor, todos os monitores,
  ocultar automaticamente, reservar espaço, estilo, inicialização.
- Configuração em `~/.config/big-tabbed-dock/dock.conf`, escrita atômica e `.bak`.

## O que faltava (objeto desta rodada)

1. Repositório próprio: o dock evoluía preso ao ciclo do big-wm.
2. Nenhum jeito de mudar alinhamento e borda sem abrir as Preferências.
3. Nenhuma API de controle: as Preferências eram a única entrada; outro programa
   só podia editar o arquivo e esperar o dock reler.
4. Nada no Dock do Window Maker / wlmaker que representasse o dock.

## Problemas achados durante a rodada

- **X11, a prateleira cobria a coluna do Dock do Window Maker** (borda de cima +
  largura total, e nos outros alinhamentos quando a prateleira chegava à
  lateral): os cliques no Dock iam para a prateleira. Corrigido: a área usada
  desconta as colunas do Dock/Clip (ver [09](09-posicionamento.md)).
- Um processo antigo do dock sobreviveu a um `quit` com um diálogo modal aberto
  (comportamento anterior a esta rodada; registrado, não corrigido).
