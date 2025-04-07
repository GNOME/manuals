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
  'manuals-utils.c',
  'manuals-window.c',
  'main.c',
]

manuals_deps = [
  glib_dep,
  dex_dep,
  gom_dep,
  flatpak_dep,
  gtk_dep,
  adw_dep,
  libpanel_dep,
  webkit_dep,

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
