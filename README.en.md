# QtRAR

[Español](README.md) · [English](README.en.md)

RAR and ZIP archive manager for Linux with the WinRAR interface, written in
C++20 and Qt 6. QtRAR does not implement the RAR format: it drives RARLAB's
official binaries (`unrar` and `rar`) and, for ZIP, 7-Zip.

![License: GPL-3.0](https://img.shields.io/badge/license-GPLv3-blue.svg)
![Qt](https://img.shields.io/badge/Qt-6.4%2B-41cd52.svg)
![C++](https://img.shields.io/badge/C%2B%2B-20-f34b7d.svg)

> **Status: 0.1.0 (alpha).** The application is usable every day for listing,
> testing and extracting, but things are still missing and the roadmap is open.
> If something breaks, [open an issue](CONTRIBUTING.md) with the exact terminal
> output: that is the most useful thing you can send.

## Why it exists

On Linux, WinRAR is not available as a native application, and the alternatives
are either command line tools (`unar`, `file-roller`) or have a very different
interface. QtRAR wants to be the obvious answer for people coming from Windows:
the same layout, the same buttons, the same shortcuts, compiled natively and
without Wine.

## What it does

- **WinRAR-style navigation**: file browser on the left, archive contents on the
  right, with an address bar, back/forward, history, favourites and a `..` row
  to go up one level. When an archive is opened, the left pane switches to the
  internal tree of the RAR or ZIP.
- **Toolbar that matches WinRAR's** (32 px, short label under the icon), with the
  right-hand block of buttons that changes depending on whether an archive is
  open. It can be rearranged from *Customise toolbar…*.
- **Extraction**: everything, the archive's folder, the selection, or a single
  member with a double click. Dialog with *General* / *Advanced* / *Options*
  tabs, destination folder tree, update and overwrite modes.
- **Creating** RAR and ZIP archives: compression level, solid archives, password
  encryption, **file name encryption**, multi-volume sets and self-extractors.
- **Maintenance**: test integrity, rename, delete, protect, recover damaged
  headers, read and write the archive comment.
- **Passwords**: requested when needed and remembered for the session (or asked
  every time).
- **Wizard** for step-by-step creation, and a **finder** for files inside the
  archive with wildcards (`*`, `?`).
- **Themes** `original` (the classic WinRAR look) and `system`, with light and
  dark icons, following the desktop's colour scheme.
- **Desktop integration**: *Extract here*, *Extract to…*, *Test* and *Add to
  archive…* actions in Dolphin and Nautilus, MIME type registration, and a
  command line for scripts.
- **Internationalisation** in Spanish and English, including Qt's own
  translations.

## Requirements

| | |
|---|---|
| Qt | 6.4 or newer (Core, Gui, Widgets, Network, LinguistTools, Test) |
| CMake | 3.21 or newer |
| Compiler | C++20 (GCC 12+, Clang 15+, or whatever your distro ships) |
| RAR | `unrar` to list, test and extract; `rar` to create and modify |
| ZIP | 7-Zip (`7z`, or `7za` / `7zz` / `7zr` as alternatives) |

On Debian/Ubuntu:

```sh
sudo apt install build-essential cmake qt6-base-dev qt6-tools-dev-tools \
                 qt6-l10n-tools libgl1-mesa-dev
```

## Installation

Binaries are published in the [GitHub releases](https://github.com/iDzib98/QtRAR/releases).

| Package | Who it is for |
| --- | --- |
| `QtRAR-<version>-x86_64.AppImage` | Any 64-bit Linux. No Qt or anything else needed: `chmod +x` and run. |
| `qtrar_<version>_amd64.deb` | Debian, Ubuntu and derivatives. |

### AppImage

```sh
chmod +x QtRAR-0.1.0-x86_64.AppImage
./QtRAR-0.1.0-x86_64.AppImage
```

Qt is bundled inside, so it works on a machine without Qt installed. The
libraries live in `usr/lib/`: if one of them ever misbehaves, you can delete it
and drop in your own build of the same version (see
[RAR binaries and licensing](#rar-binaries-and-licensing)).

### Debian package

Download the published `.deb` and run:

```sh
sudo apt install ./qtrar_0.1.0_amd64.deb
```

The package includes QtRAR, `unrar` (the only RAR component its licence allows
redistributing), the translations and the Dolphin and Nautilus integrations. It
does **not** include `rar` or any licence key; see
[RAR binaries and licensing](#rar-binaries-and-licensing).

To build the package yourself, see [`packaging/README-deb.md`](packaging/README-deb.md).

### From sources

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/QtRAR
```

The first configure step tries to download RARLAB's official binaries into
`bin/` with `tools/fetch-rar.sh` (the `QTRAR_FETCH_RAR` option, on by default).
If you would rather not download them:

```sh
cmake -S . -B build -DQTRAR_FETCH_RAR=OFF
```

To install under your own user, no root needed:

```sh
cmake --install build --prefix ~/.local
```

Then refresh the application database so MIME types and context menus show up:

```sh
update-desktop-database
```

The CMake options that matter:

| Option | Default | What it does |
|---|---|---|
| `QTRAR_FETCH_RAR` | `ON` | Downloads `rar`/`unrar` into `bin/` if missing. Development only: `bin/` is not tracked. |
| `QTRAR_BUNDLE_UNRAR` | `OFF` | Installs `unrar` next to QtRAR. This is the redistribution RARLAB's EULA allows (clause 3.a). |
| `BUILD_TESTING` | `ON` | Builds `tst_core` and `tst_ui` and registers them with CTest. |

File manager integration is described in
[`packaging/README-context-menu.md`](packaging/README-context-menu.md).

## Command line

QtRAR takes files and actions, so it can be called from scripts or context menus:

```sh
QtRAR archive.rar                    # open the archive
QtRAR --test archive.rar             # test integrity and exit
QtRAR --extract-here archive.rar     # extract next to the archive and exit
QtRAR --extract-to /dest a.rar       # extract into the given folder and exit
QtRAR --extract-to-dialog archive.rar  # open the extraction dialog
QtRAR --add-to-archive files...      # open the creation dialog
QtRAR -l en --theme system           # language and theme
QtRAR --help
```

The application is single-instance: if a window is already open, a second
launch hands the file over and exits (can be turned off in *Options*).

### Where preferences live

In `~/.config/QtRAR/QtRAR.conf` (`QSettings` INI format). Keys in use:

| Key | Meaning |
|---|---|
| `ui/language` | Interface language (`es`, `en`). |
| `ui/theme` | `0` = original theme, `1` = system theme. |
| `ui/icons` | Icon theme. |
| `app/singleInstance` | Single instance. |
| `paths/extractTo` | Default extraction folder; empty means "the archive's folder". |
| `files/confirmOverwrite` | Ask before overwriting. |
| `files/createVolumes` | Create volumes by default. |
| `files/compressionLevel` | Default compression level. |
| `window/rememberGeometry` | Remember window size and position. |
| `tools/rarPath` | Path to the `rar` binary, settable from *Configure RAR binary…*. |

## RAR binaries and licensing

This is the part worth reading before redistributing QtRAR.

QtRAR **does not** include the RAR algorithm or RARLAB's binaries in the
repository. The project needs a `rar` and an `unrar` on the system, or downloaded
into `bin/` for development:

```sh
tools/fetch-rar.sh          # downloads rarlinux-x64-7.23 into bin/, verifying the SHA-256
```

According to RARLAB's licence (`licenses/license.txt`, clause 3):

- `rar` is a trial version and **may not be distributed inside another software
  package**. That is why `bin/` is in `.gitignore` and the Debian package does
  not include it. Every user installs it themselves, or uses 7-Zip, which reads
  and extracts RAR but cannot create it.
- `unrar` may be redistributed separately; it is the only thing QtRAR packages
  (`QTRAR_BUNDLE_UNRAR=ON`).
- Licence keys (`rarreg.key`) must never enter the repository. QtRAR does not
  read, copy or write the key: it only reports whether the `rar` binary finds it
  and whether it is in the 40-day evaluation mode. Validation is RAR's job.

QtRAR's own code is free software: **GPL-3.0-or-later** (see
[`LICENSE`](LICENSE)). `licenses/license.txt` and `licenses/acknow.txt` are
copies of RARLAB's texts and are *not* this project's licence; for the
third-party components, see
[`licenses/THIRD-PARTY-NOTICES.md`](licenses/THIRD-PARTY-NOTICES.md).

## Development

Short version; the details are in [`CONTRIBUTING.md`](CONTRIBUTING.md).

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j

ctest --test-dir build --output-on-failure     # core and UI tests
python3 tools/check-i18n.py                    # strings in the translation catalog
python3 tools/check-icons.py build             # icons requested by the code
bash tools/e2e-test.sh                         # end-to-end (needs bin/rar)
```

The UI tests run with `QT_QPA_PLATFORM=offscreen`, so they need no display. The
end-to-end tests need the real RAR binaries and cannot run in continuous
integration without redistributing them.

## Project layout

```
src/core/     Widget-free core: process execution, listing parser, archive
              service, binary discovery, diagnostics.
src/ui/       Main window, toolbar, dialogs, themes, search.
src/model/    Archive tree model (virtual folders, icons, `..`).
src/app/      Application bootstrap, headless probe, CLI.
res/          SVG icons, original theme stylesheet, .ts translations.
packaging/    .desktop files, Nautilus scripts, DEB packaging recipe.
tests/        tst_core, tst_ui, captured real unrar output.
tools/        RAR download, i18n and icon checks, E2E.
docs/         Maintainer-oriented documentation.
```

One design detail worth knowing before touching anything: the code is split into
two libraries, `qtrar_core` (no `QtWidgets`) and `qtrar_ui`, and the executable
only bootstraps. That way listing parsing and filters are tested without building
a window. See [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md).

## Documentation

| Document | Contents |
|---|---|
| [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | Layers, classes, operation flow, where to change what. |
| [`docs/TRANSLATING.md`](docs/TRANSLATING.md) | How to add or fix a translation. |
| [`CONTRIBUTING.md`](CONTRIBUTING.md) | How to contribute, code style, tests, review process. |
| [`CHANGELOG.md`](CHANGELOG.md) | Version history. |
| [`docs/RELEASING.md`](docs/RELEASING.md) | How a release is published (tag, workflow, packages). |
| [`packaging/README-deb.md`](packaging/README-deb.md) | Building the Debian package. |
| [`packaging/README-context-menu.md`](packaging/README-context-menu.md) | Context menus and MIME types. |
| [`tests/fixtures/README.md`](tests/fixtures/README.md) | How the fixtures are captured and what they are for. |
| [`licenses/THIRD-PARTY-NOTICES.md`](licenses/THIRD-PARTY-NOTICES.md) | Third-party licences (RARLAB, Qt, 7-Zip) and what may be redistributed. |

## Contributing

Fixes, ideas and translations are all welcome. Start with
[`CONTRIBUTING.md`](CONTRIBUTING.md), but in short:

1. Open an issue before a large change, so the approach can be discussed instead
   of the work being wasted.
2. Branch from `main` and open a pull request describing what changed and why.
3. Add or adjust tests: `tst_core` for the core, `tst_ui` for the UI,
   `tools/e2e-test.sh` for behaviour with real archives.
4. Keep the toolbar order and the wording: they are deliberate.

Where help is most useful right now:

- Falling back to ZIP handling when only `unrar` is present and no 7-Zip.
- Extracting a partially damaged archive.
- Native scanning, real CLNesh integration, clipboard support.
- More translations.
- Packaging for other distributions (RPM, Arch, Flatpak).

## Licence

QtRAR is **GPL-3.0-or-later**; see [`LICENSE`](LICENSE) for the full text. The
RARLAB binaries used at runtime are **not** distributed with this project and
carry their own licence.

## Acknowledgements

- To [RARLAB](https://www.rarlab.com/), for `rar` and `unrar`, and for the
  licence that lets a free project use and redistribute `unrar`.
- To the [Qt](https://www.qt.io/) project and whoever maintains the Fusion style,
  which the classic interface look is built on.
- To [7-Zip](https://www.7-zip.org/), for ZIP coverage.
- To Intel, for the slicing-by-8 code credited in `licenses/acknow.txt`.
- To whoever sends a fix, a translation or an idea. You go first.
