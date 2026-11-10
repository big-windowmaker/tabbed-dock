# 11 — Remoção da versão embutida no big-wm

Feita só depois do repositório novo compilar, passar nos testes e gerar o
pacote. Branch `feat/tabbed-dock-repo` do big-wm (commit normal, sem reescrever
histórico):

- `git rm` de `tabbed-dock/` e `docs/tabbed-dock/`;
- `Makefile`: os alvos do dock saem; entra a instalação do
  `big-wm-dock-controller`;
- `pkgbuild/PKGBUILD`: o pacote `big-wm-tabbed-dock` sai da lista; o
  `big-wm-session` passa a depender dele;
- `tests/run.sh`: a seção 8 vira uma nota (os testes estão aqui); verificações
  novas do controlador;
- `README.md` e documentos: links para `big-windowmaker/tabbed-dock`;
- padrões do Dock (WMState, State.plist), `startbigwm*`, `big-wm-dockapps` e o
  catálogo de dockapps do menu (`big-wm-appmenu`) com o controlador.

O ícone `themes/icons/svg/tabbed-dock.svg` continua no big-wm (o tema usa);
a cópia daqui é a do pacote (`big-wm-tabbed-dock.svg` no hicolor).

`tests/run.sh` do big-wm: todas as verificações passaram depois da remoção.
