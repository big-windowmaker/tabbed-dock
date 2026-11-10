# SPDX-License-Identifier: GPL-2.0-or-later
test_env = environment()
test_env.set('G_TEST_SRCDIR', meson.current_source_dir())
test_env.set('G_TEST_BUILDDIR', meson.current_build_dir())
test_env.set('G_DEBUG', 'gc-friendly,fatal-criticals')
test_env.set('MALLOC_CHECK_', '2')
test_env.set('LC_ALL', 'C.UTF-8')
# LANGUAGE vale mais que LC_ALL para o gettext e o GLib: sem isto, uma
# sessão em pt_BR traduz os nomes dos .desktop dos testes.
test_env.set('LANGUAGE', '')
test_env.set('GIO_USE_VFS', 'local')
test_env.set('GSETTINGS_BACKEND', 'memory')

foreach t : ['model', 'config', 'desktop', 'coexist', 'launch', 'wmattrs', 'geometry']
  exe = executable('test-' + t, 'test-@0@.c'.format(t), dependencies: core_dep)
  test(t, exe, env: test_env, protocol: 'tap', args: ['--tap'])
endforeach

# O controlador (dockapp), sem X11: ordem das setas, regiões, preferências,
# --set e os arquivos do menu do wlmaker.
if get_option('dock') and get_option('dockapp') and have_dockapp
  exe = executable('test-dockapp', 'test-dockapp.c', dependencies: core_dep, link_with: dockapp_state_lib,
                   include_directories: include_directories('../src/dockapp'))
  test('dockapp', exe, env: test_env, protocol: 'tap', args: ['--tap'])
  # API de controle por D-Bus e --set do controlador (Xvfb e barramento
  # próprios; pulado sem eles)
  test('control', find_program('test-control.sh'),
       args: [dock_exe, meson.project_build_root() / 'src' / 'big-wm-tabbed-dock-dockapp'],
       protocol: 'tap', timeout: 60, is_parallel: false)
endif

# Largura total no X11 com os ícones do Window Maker em volta (appicons em
# cima da prateleira, colunas do Dock e do Clip); Xvfb e barramento próprios.
if get_option('dock') and have_x11
  x11_tile = executable('x11-tile', 'tools/x11-tile.c', dependencies: x11)
  test('fullwidth', find_program('test-fullwidth.sh'), args: [dock_exe, x11_tile],
       protocol: 'tap', timeout: 60, is_parallel: false)
endif

# Regras estáticas: autostart, sem shell, catálogos, caminhos instalados
if get_option('dock')
  test('static', find_program('test-static.sh'),
       args: [meson.project_source_root(), meson.project_build_root()], protocol: 'tap', is_parallel: false)
endif

# Precisa do binário do dock (opção dock); ver o comentário do script.
if get_option('dock')
  test('sigterm-startup', find_program('test-sigterm-startup.sh'), args: [dock_exe],
       protocol: 'tap', timeout: 30)
endif
