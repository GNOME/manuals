libide_tree_sources = [
  'ide-tree-addin.c',
  'ide-tree-empty.c',
  'ide-tree-expander.c',
  'ide-tree-node.c',
  'ide-tree.c',
]

libide_tree_enum_headers = [
  'ide-tree-node.h',
]

libide_tree_sources += gnome.mkenums_simple('ide-tree-enums',
         sources: libide_tree_enum_headers,
)

libide_tree_sources += gnome.compile_resources(
  'ide-tree-resources',
  'libide-tree.gresource.xml',
  c_name: 'ide_tree',
)

libide_tree_deps = [
  dependency('libdex-1'),
  dependency('gtk4'),
]

libide_tree = static_library('ide-tree',
  libide_tree_sources,
   dependencies: libide_tree_deps,
         c_args: release_link_args + ['-DIDE_TREE_COMPILATION'],
)

libide_tree_dep = declare_dependency(
         dependencies: libide_tree_deps,
            link_with: libide_tree,
  include_directories: include_directories('.'),
)
