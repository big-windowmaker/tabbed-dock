# 06 — Menu do botão direito

```
Dock com Abas                    (título)
☑ Mostrar o Dock com Abas
Preferências…
Alinhamento ▸   ◉ Largura total ○ Direita ○ Centro ○ Esquerda
Borda ▸         ◉ Embaixo ○ Em cima
Monitor ▸       ◉ Principal ○ Todos ○ Todos, independentes ○ <cada monitor ligado>
☐ Ocultar automaticamente
☑ Reservar espaço na tela
Aparência ▸     ◉ Clássico ○ Ladrilhos ○ Moderno
Inicialização ▸ Iniciar / Parar / Reiniciar o Dock com Abas
                Na próxima sessão: ◉ Dock com Abas e Dock clássico
                                   ○ Só o Dock com Abas ○ Só o Dock clássico
                Restaurar o padrão
────────────
Opções do Window Maker…          (só com o ladrilho no Dock do Window Maker)
Sobre
```

Os rótulos exatos estão no catálogo (`po/pt_BR.po`).

- Os itens marcados vêm dos estados das ações do dock (ou do `dock.conf`).
- O menu abre na **soltura** do botão 3 (abrir no clique deixava o próprio
  soltar fechar o menu) e é destruído em `selection-done` (destruir em
  `deactivate` perdia a ativação do item).
- **Opções do Window Maker…** manda ao ladrilho pai do Window Maker um clique
  de botão 3 sintético: o menu nativo do Dock (Manter, Iniciar ao entrar,
  Configurações, Remover...) continua ao alcance.

## No wlmaker

O wlmaker dá o botão 3 ao conteúdo dos dockapps X11 e o menu do próprio
ladrilho na **borda**. O menu do ladrilho traz as ações e as preferências do
controlador por `share/wlmaker/Dockapps.d/big-wm-tabbed-dock-dockapp.plist`
(extensão do wlmaker 31: `TextDomain` para traduzir, `ChoicesFile` para a
lista de monitores). Ver [12](12-wayland-wlmaker.md).

## Pendente

No XWayland, o menu GTK funciona com o mouse; a navegação por **teclado** no
menu não funciona (no X11 puro funciona). Registrado.
