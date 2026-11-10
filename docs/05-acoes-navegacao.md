# 05 — Setas: alinhamento e borda

| Botão | Efeito |
|---|---|
| ▶ | próximo alinhamento: **Largura total → Direita → Centro → Esquerda → Largura total** |
| ◀ | o mesmo ciclo ao contrário |
| ↑ | prateleira na borda de cima |
| ↓ | prateleira na borda de baixo |
| clique duplo no ícone | Preferências (uma janela; se já aberta, vem para a frente) |
| clique simples no ícone | mostra o estado atual |

O ciclo está num lugar só (`btd_align_step` em `src/core/btd-config.c`) e é
o mesmo para o controlador, a ação `alignment-step` e o menu.

Com o dock rodando, cada clique é uma ativação de ação no D-Bus; o dock
aplica, salva e devolve o novo estado, que o ladrilho redesenha. Sem o dock,
o controlador grava o `dock.conf` e o próximo início usa.

Verificado no X11 (Xvfb + Window Maker, cliques por `xdotool` no ladrilho
adotado): ▶ ×4 e ◀ ×4 voltam ao início passando pelos quatro; ↓ ↑; dois ▶
rápidos são dois passos e não abrem as Preferências; clique duplo abre uma
janela só. No wlmaker (sessão aninhada): as quatro setas e o clique duplo.
