project('manuals', 'c',
          version: '47.alpha',
    meson_version: '>= 1.0.0',
  default_options: [ 'warning_level=2', 'werror=false', 'c_std=gnu11', ],
)

ABI_MAJOR = 1
ABI_MINOR = 0
ABI_MICRO = 0

glib_req_version = '2.78'
dex_req_version = '0.4'
gom_req_version = '0.4'
flatpak_req_version = '1.0'

glib_req = '>= @0@'.format(glib_req_version)
dex_req = '>= @0@'.format(dex_req_version)
gom_req = '>= @0@'.format(gom_req_version)
flatpak_req = '>= @0@'.format(flatpak_req_version)

glib_dep = dependency('glib-2.0', version: glib_req)
dex_dep = dependency('libdex-1', version: dex_req)
gom_dep = dependency('gom-1.0', version: gom_req)
flatpak_dep = dependency('flatpak', version: flatpak_req)

i18n = import('i18n')
gnome = import('gnome')
cc = meson.get_compiler('c')

add_project_arguments(['-I' + meson.project_build_root()], language: 'c')

config_h = configuration_data()

if get_option('development')
  config_h.set10('DEVELOPMENT_BUILD', true)
  if get_option('builder')
    app_id = 'org.gnome.Builder.Devel.Manuals'
  else
    app_id = 'app.devsuite.Manuals.Devel'
  endif
else
  if get_option('builder')
    app_id = 'org.gnome.Builder.Manuals'
  else
    app_id = 'app.devsuite.Manuals'
  endif
endif

config_h.set_quoted('APP_ID', app_id)
config_h.set_quoted('PACKAGE_VERSION', meson.project_version())
config_h.set_quoted('GETTEXT_PACKAGE', 'manuals')
config_h.set_quoted('LOCALEDIR', get_option('prefix') / get_option('localedir'))

project_c_args = []
test_c_args = [
  '-Wcast-align',
  '-Wdeclaration-after-statement',
  '-Werror=address',
  '-Werror=array-bounds',
  '-Werror=empty-body',
  '-Werror=implicit',
  '-Werror=implicit-function-declaration',
  '-Werror=incompatible-pointer-types',
  '-Werror=init-self',
  '-Werror=int-conversion',
  '-Werror=int-to-pointer-cast',
  '-Werror=main',
  '-Werror=misleading-indentation',
  '-Werror=missing-braces',
  '-Werror=missing-include-dirs',
  '-Werror=nonnull',
  '-Werror=overflow',
  '-Werror=parenthesis',
  '-Werror=pointer-arith',
  '-Werror=pointer-to-int-cast',
  '-Werror=redundant-decls',
  '-Werror=return-type',
  '-Werror=sequence-point',
  '-Werror=shadow',
  '-Werror=strict-prototypes',
  '-Werror=trigraphs',
  '-Werror=undef',
  '-Werror=write-strings',
  '-Wformat-nonliteral',
  '-Wignored-qualifiers',
  '-Wimplicit-function-declaration',
  '-Wlogical-op',
  '-Wmissing-declarations',
  '-Wmissing-format-attribute',
  '-Wmissing-include-dirs',
  '-Wmissing-noreturn',
  '-Wnested-externs',
  '-Wno-cast-function-type',
  '-Wno-dangling-pointer',
  '-Wno-missing-field-initializers',
  '-Wno-sign-compare',
  '-Wno-unused-parameter',
  '-Wold-style-definition',
  '-Wpointer-arith',
  '-Wredundant-decls',
  '-Wstrict-prototypes',
  '-Wswitch-default',
  '-Wswitch-enum',
  '-Wundef',
  '-Wuninitialized',
  '-Wunused',
  '-fno-strict-aliasing',
  ['-Werror=format-security', '-Werror=format=2'],
]
if get_option('buildtype') != 'plain'
  test_c_args += '-fstack-protector-strong'
endif
foreach arg: test_c_args
  if cc.has_multi_arguments(arg)
    project_c_args += arg
  endif
endforeach
add_project_arguments(project_c_args, language: 'c')

# Various optimizations for non-debug builds including disabling of
# cast checks, asserts, and additional link options.
release_link_args = []
test_link_args = [
  '-Wl,-z,defs',   # Detect and reject underlinking
  '-Wl,-z,now',    # Disable lazy binding
  '-Wl,-z,relro',  # Read-only segments after relocation
]
if not get_option('buildtype').startswith('debug')
  add_global_arguments([
    '-DG_DISABLE_ASSERT',
    '-DG_DISABLE_CAST_CHECKS',
  ], language: 'c')
  test_link_args += ['-Wl,-Bsymbolic', '-fno-plt']
endif
foreach link_arg: test_link_args
  if cc.has_link_argument(link_arg)
    release_link_args += link_arg
  endif
endforeach

configure_file(
         output: 'config.h',
  configuration: config_h,
)

subdir('data')
subdir('src')
subdir('po')

gnome.post_install(
  # glib_compile_schemas: true,
    gtk_update_icon_cache: true,
  update_desktop_database: true,
)
