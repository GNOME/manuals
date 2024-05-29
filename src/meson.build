subdir('libide-tree')

manuals_sources = [
  'manuals-application.c',
  'manuals-book.c',
  'manuals-devhelp-importer.c',
  'manuals-flatpak.c',
  'manuals-flatpak-importer.c',
  'manuals-flatpak-installer.c',
  'manuals-flatpak-reference.c',
  'manuals-gio.c',
  'manuals-heading.c',
  'manuals-importer.c',
  'manuals-install-button.c',
  'manuals-jhbuild-importer.c',
  'manuals-job.c',
  'manuals-keyword.c',
  'manuals-navigatable.c',
  'manuals-path-bar.c',
  'manuals-path-button.c',
  'manuals-path-element.c',
  'manuals-path-model.c',
  'manuals-progress.c',
  'manuals-purge-missing.c',
  'manuals-repository.c',
  'manuals-sdk.c',
  'manuals-sdk-dialog.c',
  'manuals-sdk-installer.c',
  'manuals-sdk-reference.c',
  'manuals-search-entry.c',
  'manuals-search-model.c',
  'manuals-search-query.c',
  'manuals-search-result.c',
  'manuals-sidebar.c',
  'manuals-system-importer.c',
  'manuals-tab.c',
  'manuals-tag.c',
  'manuals-tree-addin.c',
  'manuals-tree-expander.c',
  'manuals-utils.c',
  'manuals-window.c',
  'main.c',
]

manuals_deps = [
  dependency('flatpak', version: '>= 1.0'),
  dependency('gom-1.0', version: '>= 0.4'),
  dependency('gtk4', version: '>= 4.12'),
  dependency('libadwaita-1', version: '>= 1.5'),
  dependency('libpanel-1', version: '>= 1.6'),
  dependency('libdex-1', version: '>= 0.5'),
  dependency('webkitgtk-6.0', version: '>= 2.42'),

  libide_tree_dep,
]

manuals_sources += gnome.compile_resources('manuals-resources',
  'manuals.gresource.xml',
  c_name: 'manuals'
)

manuals = executable('manuals', manuals_sources,
  dependencies: manuals_deps,
       install: true,
)
