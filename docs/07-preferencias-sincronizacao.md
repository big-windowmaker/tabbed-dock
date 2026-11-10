# 07 — Preferências e sincronização

Três entradas para as mesmas preferências: a janela de Preferências, o
controlador e o arquivo. Uma fonte de verdade: o estado dentro do dock.

| Mudança feita em | Chega em |
|---|---|
| controlador (seta, menu, `--set`) | dock (na hora), Preferências abertas (os controles mudam), arquivo (gravação agendada) |
| Preferências | dock, controlador (estado da ação → redesenho), arquivo |
| arquivo, à mão, com o dock parado | dock no próximo início; controlador lê o arquivo enquanto não há dock |

`btd_prefs_dialog_sync()` atualiza os controles da janela aberta sem disparar
os sinais dela (sem eco). `btd_control_sync()` só emite `StateChanged` para o
que mudou.

Testado em `tests/test-control.sh` (12 verificações TAP, Xvfb + barramento
próprio): sem o dock, `--set` grava o `dock.conf` e mantém as abas padrão; o
dock começa com o alinhamento do arquivo; ▶ ▶ e ◀ (`alignment-step`); ↑;
borda inválida recusada; a mudança é salva; `--set alignment` e `--set edge`
chegam ao dock rodando; o estado está no barramento; tudo salvo ao sair. O
arquivo corrompido (guardado de lado, a cópia `.bak` usada, nada gravado por
cima) é do núcleo e está em `tests/test-config.c`; o caso do controlador sem
o dock diante de um arquivo ilegível não tem teste automático. Visto também em captura: o alinhamento trocado no ladrilho aparece
na janela de Preferências aberta.
