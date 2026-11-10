# 10 — Empacotamento

Pacote `big-wm-tabbed-dock` (Arch/Manjaro), `pkgbuild/PKGBUILD`:

- `epoch=1`, `pkgver=0.2.0`: o pacote antigo, feito pelo big-wm, tinha versão
  por data (26.11.x), maior que 0.2.0 sem a época.
- Fonte: `git+https://github.com/big-windowmaker/tabbed-dock.git#tag=v$pkgver`,
  ou outra por `TABBED_DOCK_SOURCE` (o CI usa o commit em teste).
- `check()`: `meson test` (precisa de `xorg-server-xvfb` e `dbus`, em
  `checkdepends`).
- `optdepends`: `wlmaker>=1:0.8.r80.g7481afe-31` (menu do ladrilho no
  wlmaker).

Instala:

| Arquivo | |
|---|---|
| `/usr/bin/big-wm-tabbed-dock`, `big-wm-tabbed-dock-dockapp`, `big-wm-appicons` | |
| `/usr/share/applications/big-wm-tabbed-dock-preferences.desktop` | |
| `/usr/share/applications/big-wm-tabbed-dock-dockapp.desktop` | o controlador, `NoDisplay` |
| `/etc/xdg/autostart/big-wm-tabbed-dock.desktop` | só nas sessões do Big WindowMaker |
| `/usr/share/icons/hicolor/scalable/apps/big-wm-tabbed-dock.svg` | |
| `/usr/share/wlmaker/Dockapps.d/big-wm-tabbed-dock-dockapp.plist` | menu no wlmaker |
| `/usr/share/locale/*/LC_MESSAGES/big-wm-tabbed-dock.mo` | |

`tests/test-static.sh` confere que nada é instalado fora disso, para qualquer
prefixo.

## Quem usa

- big-wm: `big-wm-session` depende de `big-wm-tabbed-dock>=1:0.2.0`.
- ISO: `iso-profiles` já lista `big-wm-tabbed-dock` em `Packages-Desktop`;
  `build-iso` busca o artefato `tabbed-dock-packages` do `ci.yml` deste
  repositório (`PACKAGE_SOURCES`) e o pacote está em `EXPECTED_PACKAGES`.
- `packages`: wlmaker 31 (o `Dockapps.d` com `TextDomain` e `ChoicesFile`).
