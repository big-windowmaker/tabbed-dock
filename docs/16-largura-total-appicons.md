# 16 — Largura total com aplicativos abertos (0.2.1)

Investigação de 10 de novembro de 2026, na sessão X11 real do
desenvolvedor (três monitores: HDMI-A-0 e DisplayPort-0 3440×1440,
DisplayPort-1 2560×1080; IconSize 48; prateleira embaixo, em Largura total,
no DisplayPort-0).

## Sintoma

Com um aplicativo aberto, a prateleira em **Largura total** deixava, no lado
esquerdo do monitor, uma faixa vazia da largura de um ícone (48 px).

## Medidas

| | antes (0.2.0) | depois (0.2.1) |
|---|---|---|
| monitor | x=3440, 3440×1440 | igual |
| janela da prateleira | 3440×165 em y=1275 (o corpo + a margem transparente acima das abas) | igual |
| strut reservado | 112 px (`_NET_WM_STRUT_PARTIAL` …, 3488, 6879) | 112 px, 3440…6879 |
| `--print-state` | `x11_area offset=48 wm_left=48 wm_right=0`, `geometry=3392x116` | `wm_left=0`, largura total |
| ícone do Terminal (appicon) | 48×48 em (3440, 1280), até y=1328 | igual |

O corpo da prateleira começa em y = 1440 − 112 = 1328: o appicon termina
**exatamente** em cima dela, onde o Window Maker o põe (na área de trabalho,
logo acima do que a prateleira reserva).

## Causa

A 0.2.0 passou a deixar livres as colunas do Dock e do Clip do Window Maker
(ladrilhos override-redirect quadrados nas laterais, que ficariam fora de
alcance debaixo da prateleira). Para achar essas colunas, olhava o
retângulo da **janela inteira** (165 px de altura, por causa da margem
transparente para a sombra e as abas mais altas), não o do **corpo**. Os
appicons e as miniaturas também são ladrilhos override-redirect quadrados na
lateral: o do Terminal, de 1280 a 1328, entrava nos 165 px da janela e era
tomado por uma coluna do Dock.

## Correção

- `src/core/btd-geometry.c`: a regra, sem X — um ladrilho só conta se
  **entra no corpo** da prateleira (abas + prateleira, a mesma faixa do
  strut); encostar na borda não é entrar.
- `src/backend/btd-x11.c`: só coleta os retângulos dos ladrilhos.
- `src/ui/btd-dock.c`: passa o corpo (`tab_h + shelf_h`, encostado na borda)
  em vez da janela.

Nada de compensação fixa: com o Dock ou o Clip de verdade dentro da
prateleira (o Dock descendo até embaixo, o Clip no canto), a coluna continua
livre.

## Testes

- `tests/test-geometry.c` (6): os números da sessão real; vários appicons;
  coluna do Dock na prateleira; Clip no canto; borda de cima; janelas que não
  são ladrilhos.
- `tests/test-fullwidth.sh` (6, Xvfb): com `tests/tools/x11-tile.c` no lugar
  dos ícones do Window Maker. O binário 0.2.0 dá **976 de 1024** com um
  appicon (4 de 6 falham); o 0.2.1 dá 1024 (6 de 6).
- Sessão real, depois de instalar: um aplicativo aberto pelo dock, o appicon
  em cima da prateleira, `wm_left=0`, a prateleira desde x=0.
- Wayland (wlmaker 31 aninhado): 1600 de 1600 com zero, um e dois
  aplicativos (lá quem dá a largura é o compositor, pela layer-shell).

## Também na 0.2.1: o dock que cai volta

Na mesma sessão, o Window Maker matou o dock (`XKillClient` às 04:03) e o
`Restart=on-failure` do drop-in nunca agiu (`NRestarts=0`): o gerador do
autostart cria a unidade com `ExitType=cgroup`, e os filhos que um
aplicativo aberto pelo dock cria antes de ir para o escopo dele (os zygotes
do VSCodium) ficam no cgroup do dock — a unidade seguia "ativa". O drop-in
`data/big-wm.conf` agora tem `ExitType=main` (o `KillMode=process` já
garantia que nenhum aplicativo é fechado). Provado com duas unidades de
teste: `ExitType=cgroup`, 0 reinícios; `ExitType=main`, reiniciada.

Limitação conhecida: a corrida que deixa esses filhos no cgroup do dock (o
escopo é criado depois do `fork`, por D-Bus) continua; com `ExitType=main` e
`KillMode=process` ela não tem mais efeito sobre o dock nem sobre os
aplicativos.
