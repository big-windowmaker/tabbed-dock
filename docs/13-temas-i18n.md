# 13 — Tema e tradução

## Tema

Paletas clara e escura desenhadas no ladrilho; a escolha segue
`~/.config/big-wm/theme` (o interruptor claro/escuro do big-wm), relida na
hora por `GFileMonitor`. O menu e as Preferências seguem o tema GTK.

## Tradução

Domínio `big-wm-tabbed-dock`, o mesmo de antes. Entraram no `POTFILES.in`:
`src/ui/btd-control.c`, `src/dockapp/*.c` e o `.desktop` do controlador. Os
títulos do menu do wlmaker estão marcados com `N_()` em
`btd-dockapp-state.c` e o plist do `Dockapps.d` aponta o `TextDomain`.

`po/pt_BR.po`: 208 mensagens, todas traduzidas (verificado por
`tests/test-static.sh`, que falha com mensagem não traduzida ou *fuzzy*).

Vocabulário seguido no Big WindowMaker: "Ocultar", "Largura total",
"Em cima/Embaixo", "Dock com Abas".
