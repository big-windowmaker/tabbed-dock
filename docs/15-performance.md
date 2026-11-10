# 15 — Desempenho

Medido em 10/11/2026 neste computador (Manjaro, kernel 6.18), Xvfb 1280×1024,
barramento de sessão próprio, sem gerenciador de janelas, binários de
`build/` (meson, `buildtype` padrão). Script: dock + controlador, depois 5
`--set alignment=…` e 60 s parados.

| Medida | Controlador | Dock |
|---|---|---|
| Início (até a janela / o nome no barramento) | 43 ms | 19 ms |
| Memória RSS | 51,7 MB | 64,9 MB |
| Memória PSS (descontadas as bibliotecas compartilhadas) | 11,6 MB | 16,5 MB |
| CPU em 60 s parado | 0 tiques (CLK_TCK=100) | 2 tiques |
| Threads / descritores | 5 / 14 | — |

`--set alignment=…` com o dock rodando: o comando termina em 14–15 ms e o
estado novo já aparece no D-Bus em 29–31 ms (medido de fora, com um `gdbus`
por consulta; o tempo real é menor). Um clique no ladrilho segue o mesmo
caminho sem iniciar processo.

O controlador não faz *polling*: redesenha só quando um estado muda, um
arquivo observado muda ou o mouse passa; o resto do tempo fica bloqueado no
laço do GLib.

Uma medida só, sem repetição estatística; números de referência, não de
desempenho garantido.
