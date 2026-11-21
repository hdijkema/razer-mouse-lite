# Building Razer Mouse Lite

The application uses C++17, FLTK 1.4 and fltk-css. nlohmann/json 3.12.0 is
vendored as a single header. No Qt SDK, plugins, DLLs or translation tools are
used. Windows and Linux have platform implementations.

## Prerequisites

- CMake 3.21+, Git, and a C++17 compiler.
- Windows: Visual Studio 2022 C++ tools and Windows SDK.
- Linux: libudev, libcurl, and the X11/font development libraries below.
- Optional Linux tray runtime: Ayatana AppIndicator GTK3. It is loaded with
  dlopen; its development headers are not required.

fltk-css is fetched from https://github.com/hdijkema/fltk-css.git by CMake
FetchContent, pinned to commit 5b936cca8ea009d5a82cfb90ae9ad6d0d4f66b78.
CI and release builds use that exact revision. Update GIT_TAG in CMakeLists.txt
after publishing and testing a new library revision.

For local library development, use CMake's standard source override:

    cmake -S . -B build/local -DFETCHCONTENT_SOURCE_DIR_FLTK_CSS=/path/to/fltk-css

This uses the existing checkout directly, without downloading or changing it.
Remove the override (or use a separate build directory) to return to the pinned
revision. The former FLTK_CSS_SOURCE_DIR option is no longer used.

CMake finds an installed FLTK 1.4 CONFIG package when available; otherwise
fltk-css fetches the pinned FLTK 1.4.5 source. The first configure needs network
access to fetch fltk-css and, unless installed, FLTK. Use FLTK_DIR to select an
existing FLTK installation. Application builds link the toolkit statically.

## Windows

From a Visual Studio Developer terminal:

    cmake -S . -B build/fltk-release -G "Visual Studio 17 2022" -A x64
    cmake --build build/fltk-release --config Release --target razer-mouse --parallel 4
    ctest --test-dir build/fltk-release -C Release --output-on-failure

Use a new build directory when migrating from the former toolkit. The executable
is build/fltk-release/Release/razer-mouse.exe. Its editable skins folder is copied
beside it on every build. The default configuration can also use a system CMake
or the CMake supplied with your C++ development tools.

The build script builds, tests and stages a portable ZIP:

    .\build.ps1 -CMake cmake

It copies the MSVC runtime, CSS skins, documentation and licenses. The staged
application is tested with only Windows system directories on PATH. There is no
deployment-tool step for a GUI framework. Packaging preserves the old generated
dist directory inside its build fixture before replacing it with the new one,
so old framework DLLs cannot remain in the new distribution.

For an Inno Setup installer:

    .\tools\install-inno-setup.ps1
    .\build.ps1 -CMake cmake -InnoCompiler .\build\tools\inno-setup\ISCC.exe -TestInstaller

The existing installer supports current-user and all-users installation. Tests
use a private application identity, installation directory and registry key.
Profiles remain in their historical user settings location and are retained
across installation, repair and uninstall.

## Linux

For Debian/Ubuntu:

    sudo apt install build-essential cmake ninja-build git pkg-config \
      libudev-dev libcurl4-openssl-dev libx11-dev libxft-dev libxinerama-dev \
      libxcursor-dev libxfixes-dev libxrender-dev libpng-dev libjpeg-dev zlib1g-dev

    cmake -S . -B build/fltk-release -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/usr -DFLTK_BACKEND_WAYLAND=OFF
    cmake --build build/fltk-release --target razer-mouse --parallel 4
    ctest --test-dir build/fltk-release --output-on-failure

For headless rendering tests, install xvfb and xauth and use:

    xvfb-run -a ctest --test-dir build/fltk-release --output-on-failure

The X11 configuration can run under XWayland. Desktop pointer configuration is
disabled on Wayland: the compositor owns that setting. DPI and lighting use
hidraw directly and do not depend on a desktop session.

Install and package:

    sudo cmake --install build/fltk-release
    cd build/fltk-release
    cpack -G DEB

The DEB derives required system-library dependencies and recommends
libayatana-appindicator3-1 for the tray. The optional library is not an ELF
link-time dependency. FLTK/fltk-css development files are not installed into the
application package.

Install the supplied permissions rule to use supported devices as a normal user:

    sudo install -m 644 packaging/linux/70-mouse-lite.rules /etc/udev/rules.d/70-mouse-lite.rules
    sudo udevadm control --reload-rules

Reconnect the mouse afterwards. Do not run the app as root. Optional xinput
provides pointer speed/acceleration on X11; commands are started with an argument
vector and bounded execution time, without a shell.

### Optional tray and system appearance

On Debian/Ubuntu:

    sudo apt install libayatana-appindicator3-1

The Linux tray tries libayatana-appindicator3.so.1, then the compatible
libappindicator3.so.1. GTK/GLib/GObject entry points are resolved dynamically and
checked before use. The backend pumps nonblocking GLib events from FLTK's loop;
native callbacks queue action identifiers for the application thread. Native
objects and menu callbacks are released during shutdown.

The desktop must expose an AppIndicator host. If the library or host is missing,
the settings window stays usable; closing it exits, rather than leaving an
unreachable hidden application. MOUSE_LITE_DISABLE_TRAY=1 disables tray loading.
On older AppIndicator versions, primary activation opens the menu; profile
switching and Next profile are menu actions. Newer versions may expose a direct
activation signal.

Follow system reads org.freedesktop.portal.Settings over optional GIO, using the
org.freedesktop.appearance/color-scheme preference. A missing portal or preference
falls back to light mode. Windows reads AppsUseLightTheme. Changes are checked
asynchronously every two seconds while following the system.

macOS is an extension point, not a completed mouse backend. The same tray API
can use AppKit NSStatusItem/NSMenu, matching the author's racket-tray approach;
HID transport, startup, pointer controls and packaging still need implementation.

## CSS skins

The three templates live in skins/mint, skins/graphite and skins/sand. Each has
a complete light.css and dark.css. They are copied beside the executable and
also embedded as a fallback.

Copy a template to <settings directory>/skins/my-skin. Restart to discover a new
directory, then select it beside Language. Names may contain letters, digits,
hyphens and underscores. Existing directories can be edited and applied with
Reload CSS. An invalid stylesheet retains the last working style and reports
the parser diagnostic.

Lookup order is user skins, executable-adjacent skins, installed shared-data
skins, then embedded templates. Mode and skin are saved in settings.json next
to profiles.json; language updates preserve appearance settings and vice versa.
Existing language preferences are read from the former native registry/INI
location until a JSON preference is saved.

Supported appearance properties include backgrounds, colors, fonts, font sizes,
font weight/style, border width/color/radius, selection color and interaction
states. The styling library's README describes the subset. Widget geometry and
resizing stay in FLTK. Menus/scrollbars use FLTK's matching native drawing palette.

## Tests and diagnostics

    razer-mouse --self-test --language en --catalogues /path/to/translations
    razer-mouse --smoke-test preview.png --language nl --theme dark --skin mint
    razer-mouse --version

The contract suite checks all bundled models, generated lighting packets, exact
Cobra packets, CRC rejection, definition validation, versioned caches, atomic
persistence, Unicode filenames, corrupt-file protection, profile migration,
translation completeness/placeholders, descriptor parsing and startup escaping.
It enumerates attached mice read-only; it never applies settings.

UI tests use isolated sample profiles and a synthetic Cobra. They exercise
editing/validation, read-only behavior, capabilities, all skins, all appearance
modes and simulated system changes before rendering a PNG. They do not create a
tray, modify personal preferences, perform downloads or write to a mouse. A
graphical session is required; use Xvfb on headless Linux.

The --probe option queries current DPI from a wired Cobra without changing
settings. Real application of DPI/lighting/pointer settings requires manual
hardware verification. Packet tests alone do not prove compatibility with every
listed mouse or operating system.

## Structure

- src/core: STL/JSON profiles, persistence API, localization, HID descriptors and protocol.
- src/platform: native APIs, atomic replacement, paths, USB channels, desktop integration, HTTPS.
- src/device: command orchestration and confirmation handling.
- src/ui: FLTK widgets, CSS skins, asynchronous operations and editor.
- src/app: command line, startup and application lifetime.
- translations: eight editable UTF-8 JSON catalogues.
- tests: protocol, persistence, translations, UI, boundaries, release and installer checks.

The boundary check rejects direct native OS headers outside platform and Qt
includes in shared modules. Existing windowsSettings profile fields are migrated
to pointerSettings while reading.

## Device definitions and weekly updates

tools/import-reference.ps1 converts reviewed openrazer-win data and recipes into
data/mice.json. OpenRazer's original data lives in C/Python, rather than this JSON
format. tools/dpi-recipes.json records reviewed recipe fingerprints; unknown
protocol changes require source review.

The update button downloads this project's reviewed JSON using WinHTTP on Windows
and libcurl on Linux. HTTPS certificate verification remains enabled, redirects
are refused, transfers use bounded timeouts and payloads are limited to 2 MiB.
Only validated data is atomically installed. Definition updates never apply a
profile. The cache records the app version and is revalidated when loaded.

The existing weekly workflow still checks OpenRazer on Mondays at 05:17 UTC. It
uses a pinned converter, validates candidate definitions with the C++ tests and
opens a draft PR only for changed definitions. No merge or publication occurs
automatically. The workflow now builds the FLTK app and uses Xvfb for screenshots.

A version tag or explicitly versioned manual release run publishes only after
Windows, Ubuntu and Debian 13 jobs succeed. Normal pushes only build/test.
