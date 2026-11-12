# 20 — Polimento e desempenho

## Polimento (revisão nas capturas reais, 1× e 4×)

- *Hover* tímido demais na primeira versão: face +0,10 (claro) / +0,09
  (escuro) — perceptível, ainda discreto.
- Soquete da lâmpada apagada no tema claro parecia um quadrado branco
  vazado: a aresta de baixo/direita passou a cinza claro.
- Prateleira e ícone: o ícone desceu 1 px para ficar entre as faixas da
  prateleira sem tocar nelas (verificado pelo teste `icon`).

## Desempenho

Mesmo laboratório (Xvfb 1024×700 + Window Maker + dock rodando), mesmo
build (`debugoptimized`), antes = `main` (`96b9f0f`), depois = este ramo;
duas rodadas alternadas cada; CLK_TCK = 100. Script `perf.sh`: início até
o ladrilho adotado, memória, CPU parado 60 s, CPU sob o roteiro de estresse
(*hover* pelos botões, cliques, roda, troca de tema e de visibilidade) de 30
e depois mais 150 rodadas.

| Medida | Antes | Depois |
|---|---|---|
| Início até o ladrilho no Dock | 44–45 ms | 43 ms |
| RSS / PSS parado | 30,1–30,6 MB / 5,8 MB | 30,3 MB / 5,7 MB |
| CPU parado, 60 s | 0 tiques | 0 tiques |
| CPU, 30 rodadas de estresse | 23 tiques | 14 tiques |
| CPU, 150 rodadas de estresse | 112–113 tiques | 67 tiques |
| RSS após 30 → 180 rodadas | +130 kB | +90 kB |

Sob uso o controlador gasta **~40 % menos CPU**, embora agora redesenhe no
*hover*: o SVG é rasterizado uma vez ao iniciar, em vez de a cada redesenho.
Parado continua em zero (nenhum *polling*, nenhum temporizador novo). A
memória não cresce com o uso além do aquecimento do GTK (dicas, menu), igual
antes e depois.

Medidas de referência num computador, não garantias.
