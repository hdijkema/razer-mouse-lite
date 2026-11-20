# Building Razer Mouse Lite

The repository root is the `razer-mouse-lite` folder. Open its `CMakeLists.txt`
in Qt Creator, or open the folder in VS Code. The `openrazer-win` reference
checkout is not needed: the device database is already embedded in this project.

## Requirements

- CMake 3.21 or later and a C++17 compiler.
- Qt 6.4 or later: Widgets, Network, Concurrent, and LinguistTools.
- Windows: Visual Studio 2022 C++ tools and Windows SDK, x64 Qt built for MSVC.
- Linux: `pkg-config` and `libudev-dev`; optional `xinput` for X11 pointer settings.

## Windows

With the local default Qt and CMake paths:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1
```

For another SDK or CMake installation:

```powershell
.\build.ps1 -Qt C:\Qt\6.8.3\msvc2022_64 -CMake cmake
```

The script configures a Release build, runs CTest, stages a clean application
folder, deploys Qt and the MSVC runtime, tests the staged app with development
paths removed, and creates a portable ZIP under `release/`. `dist/` contains the
local portable app. The executable stays named `razer-mouse.exe`.

### Inno Setup

Install the pinned compiler into the ignored local build-tools directory, then
build and test the installer:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\install-inno-setup.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1 -InnoCompiler .\build\tools\inno-setup\ISCC.exe -TestInstaller
```

The helper verifies the compiler download's SHA-256 checksum. The installer
supports current-user and all-users installation, with optional desktop shortcut.
Autostart remains an opt-in setting in the app. Exit the existing app through the
tray menu before upgrading; the installer does not force-close it.

The installer test uses a private AppId, a directory under `build/`, and a separate
registry fixture. It tests installation, application startup, repair by reinstall,
uninstallation, retained untracked data, and removal of the correct startup entry.
An entry pointing at another copy must be retained. Tests explicitly use
current-user mode and do not install the app into Program Files.

## Linux

Ubuntu 24.04, Debian 13, or a distribution providing Qt 6.4 or later:

```sh
sudo apt install build-essential cmake ninja-build pkg-config libudev-dev file dpkg-dev \
    qt6-base-dev qt6-qpa-plugins qt6-tools-dev qt6-tools-dev-tools qt6-l10n-tools
# Optional, for X11 pointer settings:
sudo apt install xinput
cmake -S . -B build/linux -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/linux --parallel
ctest --test-dir build/linux --output-on-failure
./build/linux/razer-mouse
```

Install the udev rule, reload the rules, and reconnect the mouse:

```sh
sudo install -m 644 packaging/linux/70-mouse-lite.rules /etc/udev/rules.d/70-mouse-lite.rules
sudo udevadm control --reload-rules
```

For an installed DEB, the rule is supplied at
`/usr/share/mouse-lite/70-mouse-lite.rules`. Copy that file to the same
`/etc/udev/rules.d/` destination and reload/reconnect as above.
The device must be available through `/dev/hidraw*`. Another driver claiming the
interface, or another configuration program issuing commands, can interfere.

Optional installation in your user directory:

```sh
cmake --install build/linux --prefix "$HOME/.local"
```

Create a distribution-specific DEB using CPack:

```sh
cmake -S . -B build/linux -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build/linux --parallel
cd build/linux
cpack -G DEB
```

The package dependencies are derived by `dpkg-shlibdeps` from the build system.
A package built on Ubuntu 24.04 is not a promise of compatibility with older
Ubuntu or Debian releases. Other operating systems are rejected by CMake until a
platform implementation exists.

To reproduce the Debian 13 CI package, run the build inside Debian 13 (or the
official `debian:13` container) and give CPack a distribution-specific filename:

```sh
cpack --config build/linux/CPackConfig.cmake -G DEB -B release \
    -D CPACK_PACKAGE_FILE_NAME=razer-mouse-lite-0.3.0-debian13-amd64
sudo apt install ./release/razer-mouse-lite-0.3.0-debian13-amd64.deb
```

Use the version you configured when building. Renaming an Ubuntu package does
not turn it into a Debian build: dependencies must be generated in Debian 13.

## Tests and diagnostics

```sh
razer-mouse --self-test --language en
razer-mouse --probe
razer-mouse --smoke-test preview.png --language fr
razer-mouse --tray
```

`--probe` reads the current DPI of a wired Cobra. The self-test checks 458 generated
lighting packets, exact Cobra packets, response validation, HID descriptors,
profile migration, atomic saves, XDG autostart encoding, and all eight translation
catalogues and their placeholders. CTest also checks module boundaries and renders
each language's settings window. Screenshot tests use fresh sample profiles.
These commands do not apply mouse or desktop pointer settings, or enable autostart.

## GitHub Actions

`.github/workflows/build.yml` runs on pushes, pull requests, and manual requests.

| Job | Result |
| --- | --- |
| version | Validated numeric version and release intent; version resolver tests |
| windows | Release build, CTest, portable startup test, installer lifecycle test; ZIP and setup EXE artifacts |
| linux | Release build, CTest, translation checks; Ubuntu 24.04 x86-64 DEB artifact |
| debian13 | Native Debian 13 container build, CTest, DEB installation and installed-app checks; amd64 DEB artifact |
| release | Attaches all four packages to a GitHub Release after all builds pass |

Branch pushes and pull requests only build and test. To publish:

- Push a tag matching `X.Y.Z` or `vX.Y.Z`; or
- Open **Actions → Build and release → Run workflow** and supply `version`.

Leave the manual version field empty for a build without publishing. Versions
must contain three numeric components, no leading zeroes, and values up to 65535.
The release version is passed through CMake into the executable and installer.
The default version is declared in `project(... VERSION ...)` in `CMakeLists.txt`.
A requested version must match a selected tag. Existing release assets are not
overwritten, and an existing tag must point at the checked-out commit.

GitHub runners download their build dependencies. The installed application
does not need internet access for mouse settings. Its optional device-definition
update button fetches this repository's `main/data/mice.json` over HTTPS from
`raw.githubusercontent.com`; no check runs at startup or in the background.
Workflow logs and screenshots are uploaded separately
from distributable packages.

## Source layout

- `src/core/`: profiles, descriptor parsing, protocol; Qt Core/Gui only.
- `src/platform/`: native Windows and Linux implementations of `platform.h`.
- `src/device/`: shared command exchange and profile application.
- `src/ui/`: Qt Widgets and language management; no native OS headers.
- `src/app/`: startup and single-instance handling.
- `translations/`: editable Qt Linguist TS files; compiled QM files are embedded.
- `tests/`: protocol, persistence, translations, module boundaries, packaging.
- `packaging/`: Linux integration files and Windows Inno Setup definition.

The four modules are separate CMake targets with explicit dependencies. When
adding UI text, update every TS catalogue with Qt Linguist/lupdate and refresh
`data/messages.json`. Tests reject unfinished translations, mismatched source
entries, and lost placeholders. Existing profile files using `windowsSettings`
are migrated when read to the portable `pointerSettings` field.

## Device definition updates

`tools/import-reference.ps1` converts the locally supplied openrazer-win device
and recipe data into our compact `data/mice.json`. OpenRazer's original definitions
are C driver code and Python device classes, not this JSON format. Review changes
to the importer when updating the reference: recipe indices and protocol behaviour
can change. Run the complete tests before publishing database changes to `main`.
The C++ validator rejects protocol functions it does not implement, malformed
recipes, duplicate product IDs, invalid ranges and downloads larger than 2 MiB.

The app uses Qt Network asynchronously, with a 20-second deadline, HTTPS
certificate verification and no redirects. A validated download is saved using
`QSaveFile` as `device-definitions.json` beside the profile data. The cache records
the app version; a different app version or invalid cache falls back to the bundled
definitions. Reinstalling or upgrading the app therefore cannot accidentally use
definitions cached for a different engine version. Tests use temporary directories
and the bundled database rather than a user's saved definitions.
