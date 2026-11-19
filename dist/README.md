# Razer Mouse Lite

Compacte muisinstellingen in C++17 en Qt 6 Widgets, met afzonderlijke modules voor
Windows en Linux. Geen accounts, advertenties, webview, Python-runtime of service.

## Gebruik

Windows: start `dist/razer-mouse.exe`. De bedrade Razer Cobra wordt automatisch
herkend (USB 1532:00A3). Profielen bevatten DPI, pollingfrequentie, desktopaanwijzer,
verlichtingskleur en helderheid. De Cobra ondersteunt 100–8500 DPI en
125/500/1000 Hz. De ingebouwde databank bevat 114 USB-muismodellen; andere modellen
zijn niet allemaal op echte hardware getest. Bluetooth wordt niet ondersteund.

Klik op een profiel voor direct toepassen. Bewerk een profiel met **Opslaan** of
**Opslaan en toepassen**. Bewaar wijzigingen voordat je een ander profiel kiest.
**Behouden** laat de huidige pollingfrequentie of verlichting staan.

Een linksklik op het trayicoon kiest het volgende profiel, dubbelklikken opent het
venster en rechtsklikken opent het profielmenu. Het kruisje verbergt het venster;
**Afsluiten** in het traymenu sluit de app. Op desktops zonder trayondersteuning
blijft het venster beschikbaar en sluit het kruisje de app af.

**Start bij aanmelden** regelt automatisch starten in de tray. Windows gebruikt
de Run-sleutel van de huidige gebruiker. Linux gebruikt een XDG-autostartbestand
in `~/.config/autostart/mouse-lite.desktop`. Plaats de executable op de definitieve
plek voordat je dit aanzet. Na verplaatsen: schakel de optie uit en weer in.
**Actief profiel herstellen** past het laatste actieve profiel toe bij starten en
opnieuw aansluiten. De optie staat aanvankelijk uit.

Gebruik één muisbeheerprogramma tegelijk. Een profiel wordt pas actief gemarkeerd
nadat de muis de commando's heeft bevestigd. Bij een fout kunnen eerdere
hardwarecommando's al zijn uitgevoerd; hardware-instellingen worden niet
teruggedraaid.

Macro's, knoptoewijzing, muismatkalibratie, RGB-animaties en DPI-stages op de fysieke
DPI-knop zijn niet geïmplementeerd. Elk app-profiel stelt één DPI-waarde in voor
beide assen. De Cobra-verlichtingszone heet in de referentie `logo`.

## Talen

De interface, foutmeldingen en traymenu's zijn vertaald in Engels, Nederlands,
Duits, Deens, Noors (Bokmål), Zweeds, Frans en Spaans. Standaard volgt de app de
systeemtaal; voor andere talen wordt Engels gebruikt. Kies een andere taal onderaan
het venster en herstart de app. Bestaande profielnamen blijven behouden.
Native systeemdialogen volgen eventueel de taal van het besturingssysteem.

Voor een eenmalige taalkeuze: `razer-mouse --language de`. Geldige codes:
`en`, `nl`, `de`, `da`, `nb`, `sv`, `fr`, `es`. `no` wordt herkend als `nb`.

## Linux

De Linux-backend gebruikt `libudev` en `hidraw` voor apparaatdetectie en HID feature
reports. OpenRazer of een Python-daemon is niet nodig. De muis moet via de gewone
HID-driver als `/dev/hidraw*` beschikbaar zijn. Een andere driver die de interface
claimt, of andere software die tegelijk commando's stuurt, kan de werking verhinderen.

De meegeleverde udev-regel geeft de actieve lokale desktopsessie toegang tot de
bekende Razer-muismodellen. Je hoeft de app niet als root te starten:

```sh
sudo install -m 644 packaging/linux/70-mouse-lite.rules /etc/udev/rules.d/70-mouse-lite.rules
sudo udevadm control --reload-rules
# Trek de muis los en sluit hem opnieuw aan.
```

Op X11 kan de app via het optionele programma `xinput` snelheid en acceleratie van
de geselecteerde libinput-muis instellen. Op Wayland worden deze desktopinstellingen
door de compositor beheerd; de app schakelt die bediening uit en meldt de beperking.
DPI, pollingfrequentie, verlichting en profielen blijven daarvan onafhankelijk.
Op Windows gelden de aanwijzerinstellingen voor alle muizen. Games met raw input
kunnen desktopaanwijzerinstellingen negeren.

## Bouwen en testen

Benodigd: CMake 3.21+, C++17-compiler en Qt 6.4+ met Widgets, Network, Concurrent en
LinguistTools. Linux vereist daarnaast `libudev-dev` en `pkg-config`.

Windows met Visual Studio 2022 en de lokaal geïnstalleerde Qt:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1
```

`build.ps1` heeft parameters voor Qt en CMake en maakt `dist/` met Qt en de
MSVC-runtime. De app en Linux-backend zijn volledig C++; PowerShell is alleen
bouwgereedschap. De ingebouwde databank staat al in de broncode.

Ubuntu 24.04 / Debian met Qt 6.4 of nieuwer:

```sh
sudo apt install build-essential cmake ninja-build pkg-config libudev-dev \
    qt6-base-dev qt6-tools-dev qt6-tools-dev-tools qt6-l10n-tools
# Optioneel voor desktopaanwijzerinstellingen op X11:
sudo apt install xinput
cmake -S . -B build/linux -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/linux --parallel
ctest --test-dir build/linux --output-on-failure
./build/linux/razer-mouse
# Optioneel: installatie in je eigen gebruikersmap
cmake --install build/linux --prefix "$HOME/.local"
```

Andere besturingssystemen worden bewust door CMake geweigerd totdat er een eigen
platformimplementatie is. `.github/workflows/build.yml` bouwt Windows en Linux
apart en voert de protocoltests, vertaalcontroles en acht venstertests uit.
De workflow wordt pas uitgevoerd zodra dit project in een GitHub-repository staat.

```sh
razer-mouse --self-test --language en
razer-mouse --probe
razer-mouse --smoke-test preview.png --language fr
razer-mouse --tray
```

De tests veranderen geen muis-, aanwijzer- of opstartinstellingen. `--probe` vraagt
alleen de huidige DPI van een bedrade Cobra op. De smoke-test past geen profiel toe.
De zelftest controleert onder meer alle 458 verlichtingpakketten, exacte
Cobra-commando's, responsevalidatie, HID-descriptors, profielmigratie, atomische
opslag, XDG-opstartbestanden en alle vertalingen en placeholders.

## Bronstructuur en portabiliteit

- `src/core/`: profielen, HID-descriptorparser en Razer-protocol; alleen Qt Core/Gui.
- `src/platform/`: de platforminterface, met aparte Windows- en Linux-backends.
- `src/device/`: gedeelde commandoafhandeling en toepassen van profielen.
- `src/ui/`: Qt Widgets-interface en taalbeheer; geen Windows- of Linux-headers.
- `src/app/`: starten en één actieve applicatie-instantie.
- `translations/`: bewerkbare Qt Linguist TS-bestanden; QM-bestanden worden ingebed.
- `tests/`: controles op protocollen, opslag, platformhulpfuncties en vertalingen.
- `packaging/linux/`: desktopbestand en udev-regels.

De vier modules zijn zelfstandige CMake-targets met expliciete afhankelijkheden.
Een nieuwe backend implementeert `src/platform/platform.h`; de gedeelde code
hoeft daarvoor geen native OS-headers te importeren.

De opslag gebruikt Qt AppConfigLocation:
Windows `%LOCALAPPDATA%/MouseLite/MouseLite/profiles.json`, Linux doorgaans
`~/.config/MouseLite/MouseLite/profiles.json`. Profielbestanden zijn overdraagbaar;
het apparaatpad is lokaal en wordt opnieuw gedetecteerd. Het oude veld
`windowsSettings` wordt bij het lezen gemigreerd naar `pointerSettings`.
Een beschadigd profielbestand wordt niet automatisch overschreven.

Werk vertaalteksten bij met Qt Linguist en `lupdate`. Bij nieuwe bronteksten moeten
alle acht TS-bestanden en `data/messages.json` worden bijgewerkt. De tests weigeren
ontbrekende vertalingen, ongeldige placeholders en verschillen met dit manifest.

Protocol en databank zijn afgeleid van OpenRazer/openrazer-win. Zie
`THIRD_PARTY.md` en `LICENSE` (GPL-2.0-or-later).
