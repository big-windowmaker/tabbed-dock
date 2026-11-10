# 02 — Migração para o repositório próprio

Origem: `big-windowmaker/big-wm`. Destino: `big-windowmaker/tabbed-dock`
(diretório local `big-wm-tabbed-dock/`).

## Como

`git filter-repo` num clone descartável do big-wm (o repositório de trabalho
não foi tocado), guardando só o que é do dock e levando cada caminho para o
lugar novo:

```sh
git clone --no-local big-wm tabbed-dock-split && cd tabbed-dock-split
git filter-repo \
  --path tabbed-dock/ --path docs/tabbed-dock/ --path docs/screenshots/tabbed-dock/ \
  --path themes/icons/svg/tabbed-dock.svg \
  --path-rename tabbed-dock/: \
  --path-rename docs/tabbed-dock/:docs/ \
  --path-rename docs/screenshots/tabbed-dock/:docs/screenshots/ \
  --path-rename themes/icons/svg/tabbed-dock.svg:data/icons/tabbed-dock.svg
```

Resultado: 43 commits (de 26/10/2026 em diante) com autor, data e mensagem
originais, mais os desta rodada por cima. Base registrada no big-wm com a tag
local `pre-tabbed-dock-split-2026-11-09` (`5d1a3ca`).

## O que foi preservado

- Histórico (`git log --follow` funciona nos arquivos movidos).
- Licença: GPL-2.0-or-later (`LICENSE`, a mesma do big-wm).
- Créditos ([CREDITS.md](../CREDITS.md)): autores, a origem do ícone e do
  protocolo `wlr-foreign-toplevel` (MIT).
- Nomes instalados: `big-wm-tabbed-dock`, `big-wm-tabbed-dock-preferences`,
  `big-wm-appicons`, o domínio de tradução `big-wm-tabbed-dock` e o arquivo de
  configuração não mudaram: quem atualiza não perde nada.

## O que é novo no repositório

`README.md`, `CREDITS.md`, `.gitignore`, `pkgbuild/PKGBUILD` (pacote próprio),
`.github/workflows/ci.yml` (meson test + makepkg + namcap, artefato
`tabbed-dock-packages`), `tests/test-static.sh` (as regras que estavam no
`tests/run.sh` do big-wm), e estes documentos.

## O big-wm depois

Ver [11](11-remocao-versao-embutida.md). O histórico do big-wm não foi
reescrito: a remoção é um commit normal num branch.
