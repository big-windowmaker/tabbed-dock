# Relatório final — Big WindowMaker Tabbed Dock 2.0

10 de novembro de 2026. Detalhes em [01](01-auditoria.md) a [15](15-performance.md).

Classificação: **Concluído e testado** (feito e verificado por teste automático
ou no laboratório), **Concluído, teste parcial** (feito; parte verificada),
**Concluído, não testado**, **Pendente**.

## Repositório

| Critério | Situação |
|---|---|
| Repositório `big-windowmaker/tabbed-dock` criado no GitHub | **Concluído e testado** — criado em 10/11/2026 (público); `gh repo view` confirma; `main` enviado, 44 commits no GitHub, mesmo SHA do local |
| Histórico preservado | **Concluído e testado** — `git filter-repo`, autores e datas originais desde 26/10/2026 |
| Licença e créditos | **Concluído** — `LICENSE` (GPL-2.0-or-later), `CREDITS.md` |
| README, docs, testes, build, empacotamento, CI | **Concluído e testado** — CI no GitHub: ver abaixo |
| Histórico do `main` do big-wm intacto | **Concluído** — a remoção é um commit num branch local (`feat/tabbed-dock-repo`), nada enviado ao big-wm |
| Versão embutida removida só depois da validação | **Concluído e testado** — removida depois do repositório novo compilar e passar nos testes; `tests/run.sh` do big-wm passa |

CI do repositório novo: o primeiro run (42748923831) compilou, testou e empacotou, mas o upload recusou o ":" da época no nome do arquivo; corrigido, o run 42749353521 passou (meson test, makepkg, namcap) e publicou o artefato `tabbed-dock-packages`.

## Controlador

| Critério | Situação |
|---|---|
| ◀ ▶ alinhamento (Largura total → Direita → Centro → Esquerda), ↑ ↓ borda | **Concluído e testado** — X11 (Xvfb + Window Maker, cliques no ladrilho), wlmaker aninhado, `test-control.sh` |
| Clique duplo no ícone abre as Preferências (uma janela) | **Concluído e testado** — X11 e wlmaker |
| Menu do botão direito (Mostrar/ocultar, Preferências, Alinhamento, Borda, Monitor, Ocultar automaticamente, Reservar espaço, Inicialização, Aparência, Iniciar/Parar/Reiniciar, Sobre) | **Concluído e testado** — X11 pelo mouse e pelo teclado; wlmaker pelo mouse |
| Menu nativo do Window Maker ao alcance | **Concluído e testado** — "Opções do Window Maker…"; no wlmaker, o menu do ladrilho na borda |
| Sincronização nos dois sentidos com as Preferências | **Concluído e testado** — `test-control.sh` e captura com a janela aberta |
| Monitores | **Concluído, teste parcial** — lista e escolha testadas com uma saída; **vários monitores e conexão a quente não testados** |
| Ocultar automaticamente, reservar espaço | **Concluído e testado** — pelo D-Bus e pelo menu (X11) |
| Inicialização (iniciar/parar/reiniciar o dock, modos da próxima sessão) | **Concluído, não testado** — os modos são os das Preferências (já testados no dock); os itens do controlador não foram exercitados, nem um login real |
| Tema claro/escuro | **Concluído e testado** — X11, troca na hora |
| Tradução pt_BR | **Concluído e testado** — 208/208, verificado pelo `test-static.sh`; `--help` traduzido no binário instalado |
| Teclado no menu pelo XWayland (wlmaker) | **Pendente** — o mouse funciona, o teclado não |

## Posição

| Critério | Situação |
|---|---|
| Logo acima da bandeja, conta nova (X11 e Wayland) | **Concluído e testado** — WMState e State.plist padrão; `tests/run.sh` |
| Migração idempotente que respeita o arranjo do usuário | **Concluído e testado** — 6 testes (`test_dock_controller.py`); ensaio em **cópias** dos arquivos reais deste computador: controlador no 0,12, bandeja no 0,13 (nas duas listas do Dock), uma linha a mais no Dockapps.plist e no State.plist do wlmaker. Os arquivos reais não foram tocados: a migração roda no próximo login |
| Numa sessão real | **Concluído, não testado** |

## Entrega

| Critério | Situação |
|---|---|
| Pacote próprio `big-wm-tabbed-dock` | **Concluído e testado** — 1:0.2.0-1 compilado (`check()` 10/10) e **instalado neste computador**, com o big-wm 26.11.10 e o wlmaker 31; binários instalados testados no Xvfb |
| ISO | **Concluído, não testado** — `iso-profiles` já lista o pacote; `build-iso` busca o artefato do repositório novo (commit local); wlmaker 31 no `packages` (commit local). Nenhuma ISO gerada |
| Documentação 01–15 e este relatório | **Concluído** |
| Desempenho | **Concluído e testado** — controlador PSS 11,6 MB, 0 de CPU em 60 s parado, `--set` aplicado em ~30 ms ([15](15-performance.md)) |

## O que falta decidir ou enviar

- **big-wm**, **packages** e **build-iso**: branches locais em `~/.cache/bwm-wt/audio/` (sobre os da
  rodada do áudio), não enviados. O big-wm só pode ir depois de o pacote `big-wm-tabbed-dock` 1:0.2.0
  estar disponível (o `big-wm-session` depende dele).
- **Tag `v0.2.0`** no repositório novo: o `PKGBUILD` busca `#tag=v$pkgver`; sem a tag, compilar só com
  `TABBED_DOCK_SOURCE`. Não criada (é a decisão de publicar uma versão).
- Teste numa sessão real, com vários monitores e com reinicialização.
