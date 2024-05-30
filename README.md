# Manuals

Browse and Search developer documentation

## Installation

Currently you need to install by grabbing a Flatpak artifact from CI.

You can install documentation manually using the `org.gnome.Sdk.Docs`
runtimes (or similar) or use the application to install them.

```sh
flatpak install --user gnome-nightly org.gnome.Sdk.Docs//master
flatpak install --user flathub org.gnome.Sdk.Docs//45
```

## Dependencies

 * GLib/GObject/Gio/etc
 * Flatpak
 * libdex-1
 * gom-1.0 (currently from git)

## How it works

Manuals will scan your host operating system, flatpak runtimes, and jhbuild
installation for documentation. Currently, the devhelp2 format is supported
but additional formats may be added in the future.

The documentation is indexed in SQLite using GNOME/gom.

If the etag for any documentation has changed during startup, Manuals will
purge the existing indexed contents and re-index that specific documentation.

## Future Work

 * I'd love to see idexing of manpages such as POSIX headers.
 * Sphinx documentation format used by Builder and GNOME HIG
 * Indexing online-based documentation

## Screenshots

![Empty](https://gitlab.gnome.org/chergert/manuals/-/raw/main/data/screenshots/empty.png)

![Browse](https://gitlab.gnome.org/chergert/manuals/-/raw/main/data/screenshots/browse.png)

![Search](https://gitlab.gnome.org/chergert/manuals/-/raw/main/data/screenshots/search.png)

![Installing SDKs](https://gitlab.gnome.org/chergert/manuals/-/raw/main/data/screenshots/install.png)

![Dark](https://gitlab.gnome.org/chergert/manuals/-/raw/main/data/screenshots/dark.png)
