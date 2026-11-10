# 03 — Arquitetura do controlador

```
 big-wm-tabbed-dock-dockapp ──D-Bus (org.gtk.Actions)──▶ big-wm-tabbed-dock
   (ladrilho no Dock)            /io/github/big_windowmaker/TabbedDock
        │  sem o dock rodando                                │
        └────────────▶ ~/.config/big-tabbed-dock/dock.conf ◀──┘ (salva)
```

## API de controle (`src/ui/btd-control.c`)

O dock é um `GApplication`; as ações exportadas pelo GLib no D-Bus são a API.
Nada de interface D-Bus escrita à mão.

| Ação | Tipo | O que faz |
|---|---|---|
| `alignment` | estado `s` | `fill`, `right`, `center`, `left` |
| `alignment-step` | parâmetro `i` | +1 / −1 no ciclo fill → right → center → left → fill |
| `edge` | estado `s` | `top`, `bottom` |
| `monitor` | estado `s` | `primary`, `all`, `all-independent` ou um conector (`DP-2`...) |
| `monitors` | estado `as` | monitores ligados, `id\trótulo` (só leitura) |
| `style` | estado `s` | estilo da prateleira |
| `autohide`, `reserve-space`, `visible` | estado `b` | |
| `preferences` | — | abre a janela de Preferências (uma só) |
| `quit` | — | encerra |

Toda mudança passa por `commit()`: aplica (`apply_prefs`), agenda a gravação,
atualiza a janela de Preferências aberta (`btd_prefs_dialog_sync`) e os
estados das ações (`btd_control_sync`, que só emite quando o valor muda). O
caminho inverso — mudar nas Preferências — chama o mesmo `btd_control_sync`
no fim de `apply_prefs`. Por isso a sincronização é nos dois sentidos sem
laço.

## O controlador (`src/dockapp/`)

- `btd-dockapp-state.c`: lógica pura (regiões de clique, rótulos, dicas,
  ciclo, leitura de `chave=valor`, o texto dos plists do wlmaker). Testada sem X.
- `btd-dockapp.c`: o ladrilho X11, o desenho (Cairo), o menu (GTK), o vínculo
  com o dock (`GDBusActionGroup` + `g_bus_watch_name`) e a linha de comando.

Sem o dock no barramento, as mudanças vão direto para o `dock.conf` pela
biblioteca do núcleo (a mesma do dock); arquivo ausente recebe os padrões,
arquivo ilegível **não** é sobrescrito. Na linha de comando, o controlador
procura o dock de forma síncrona (`GetNameOwner`) antes de decidir, para não
escrever o arquivo com o dock rodando.

## Linha de comando

```sh
big-wm-tabbed-dock-dockapp                       # o ladrilho
big-wm-tabbed-dock-dockapp --set alignment=center   # edge= monitor= style= autohide= reserve-space=
big-wm-tabbed-dock-dockapp --toggle | --start | --stop | --restart | --preferences
```

O dock é iniciado pelo `.desktop` (`GDesktopAppInfo`), sem shell.
Variáveis de depuração: `BTD_DOCKAPP_DEBUG=1`, `BTD_DOCK_BINARY`, `BTD_DOCKAPP_ICON_FILE`.
