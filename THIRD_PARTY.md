# Third-party notices

Razer Mouse Lite is distributed under GPL-2.0-or-later. See LICENSE.

The HID report layout, command builders and embedded mouse database are adapted
from the locally supplied openrazer-win 1.6.6 reference and upstream OpenRazer.

- openrazer-win: Copyright Nikita Davidchuk and contributors, GPL-2.0-or-later.
  https://github.com/Ar4ikov/openrazer-win
  Reference files: openrazer_win/protocol/report.py, protocol/chroma.py,
  devices/data/devices.json, devices/data/recipes.json, core/transport.py.
- OpenRazer: Copyright the OpenRazer contributors, GPL-2.0-or-later.
  https://github.com/openrazer/openrazer

FLTK 1.4.5 is used under LGPL 2.1 with the FLTK linking exception.
Source: https://github.com/fltk/fltk/tree/release-1.4.5
License and exception: licenses/FLTK-COPYING.txt.
FLTK's bundled PNG/JPEG/zlib libraries retain their upstream notices in its
source tree. CMake uses system image libraries on Linux when available.

fltk-css 0.1.0 is used under LGPL 2.1. CMake FetchContent retrieves the source
from https://github.com/hdijkema/fltk-css at the pinned revision
5b936cca8ea009d5a82cfb90ae9ad6d0d4f66b78.
licenses/fltk-css-LGPL-2.1.txt contains its license.
The application links both toolkits statically and provides its build sources.

nlohmann/json 3.12.0 is a single vendored header under the MIT license.
Source: https://github.com/nlohmann/json/tree/v3.12.0
License: licenses/nlohmann-json-MIT.txt.

Linux links the system libudev and libcurl libraries. The optional tray backend
loads Ayatana AppIndicator GTK3 (or the compatible libappindicator3) and its GTK,
GLib and GObject runtime dependencies. System appearance reads the desktop portal
via optional GIO. These system libraries are not bundled with the Windows app.
https://github.com/AyatanaIndicators/libayatana-appindicator
https://curl.se/libcurl/

The platform-specific tray design follows the author's local racket-tray project:
Windows Shell_NotifyIconW, Linux AppIndicator/GTK3 loaded dynamically, and a future
macOS AppKit status item backend. No Racket runtime or Racket source is bundled.

Windows API documentation consulted:
https://learn.microsoft.com/en-us/windows-hardware/drivers/hid/sending-hid-reports
https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-systemparametersinfow

Linux and cross-platform documentation consulted:
https://www.kernel.org/doc/html/latest/hid/hidraw.html
https://specifications.freedesktop.org/autostart/latest/
https://www.fltk.org/doc-1.4/

The Linux backend links to the system's libudev. X11 pointer configuration uses
the optional system xinput utility; neither is bundled with the Windows app.

The Synapse icon in assets/razer-synapse.ico is copied unchanged from the user's
local Razer Synapse installation (C:/ProgramData/Razer/Razer Synapse/Synapse.ico).
The icon and Razer/Synapse trademarks belong to Razer and are not covered by this
project's GPL license. Razer Mouse Lite is an independent application.

The application uses the user-supplied assets/razer-badge.svg. Its generated
assets/razer-mouse-lite.ico and assets/razer-mouse-lite.png are used for Windows
resources/installer and Linux desktops. Razer owns the logo and trademark; these
assets are not covered by the project's GPL license. Window and tray icons use
the same SVG badge with its background in the active profile color.
