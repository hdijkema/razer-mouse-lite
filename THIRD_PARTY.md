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

Qt 6 is used under its applicable open source licenses. Qt is dynamically linked.
Qt licensing: https://www.qt.io/licensing/open-source-lgpl-obligations
Qt source: https://download.qt.io/official_releases/qt/
The Qt LGPL-3.0 and GPL-3.0 license texts are included in licenses/.

Windows API documentation consulted:
https://learn.microsoft.com/en-us/windows-hardware/drivers/hid/sending-hid-reports
https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-systemparametersinfow

Linux and cross-platform documentation consulted:
https://www.kernel.org/doc/html/latest/hid/hidraw.html
https://specifications.freedesktop.org/autostart/latest/
https://doc.qt.io/qt-6/qtlinguist-cmake-qt-add-translations.html

The Linux backend links to the system's libudev. X11 pointer configuration uses
the optional system xinput utility; neither is bundled with the Windows app.

The Synapse icon in assets/razer-synapse.ico is copied unchanged from the user's
local Razer Synapse installation (C:/ProgramData/Razer/Razer Synapse/Synapse.ico).
assets/razer-mouse-lite.png is the embedded 256x256 PNG copied from that icon for Linux desktop use.
The icon and Razer/Synapse trademarks belong to Razer and are not covered by this
project's GPL license. Razer Mouse Lite is an independent application.
