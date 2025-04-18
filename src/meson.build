manuals_sources = [
  'main.c',
  'manuals-application.c',
  'manuals-bundle-dialog.c',
  'manuals-install-button.c',
  'manuals-path-bar.c',
  'manuals-path-button.c',
  'manuals-path-element.c',
  'manuals-path-model.c',
  'manuals-search-entry.c',
  'manuals-tab.c',
  'manuals-tag.c',
  'manuals-tree-expander.c',
  'manuals-window.c',
  'manuals-wrapped-model.c',
]

manuals_deps = [
  dependency('gtk4'),
  dependency('libadwaita-1', version: '>= 1.4'),
  dependency('libpanel-1', version: '>= 1.6'),
  dependency('libfoundry-1', version: '>= 1.0'),
  dependency('webkitgtk-6.0', version: '>= 2.42'),
]

manuals_sources += gnome.compile_resources('manuals-resources',
  'manuals.gresource.xml',
  c_name: 'manuals'
)

executable('manuals', manuals_sources,
  dependencies: manuals_deps,
       install: true,
)
