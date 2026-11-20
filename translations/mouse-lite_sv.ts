<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS []>
<TS version="2.1" language="sv_SE" sourcelanguage="en_US">
    <context>
        <name>Backend</name>
        <message>
            <location filename="../src/device/mousebackend.cpp" line="17" />
            <source>HID write failed: %1</source>
            <translation>HID-skrivning misslyckades: %1</translation>
        </message>
        <message>
            <location filename="../src/device/mousebackend.cpp" line="21" />
            <source>HID read failed: %1</source>
            <translation>HID-läsning misslyckades: %1</translation>
        </message>
        <message>
            <location filename="../src/device/mousebackend.cpp" line="27" />
            <source>Mouse rejected command %1:%2 (status %3). Close other mouse software and try again.</source>
            <translation>Musen avvisade kommando %1:%2 (status %3). Stäng andra musprogram och försök igen.</translation>
        </message>
        <message>
            <location filename="../src/device/mousebackend.cpp" line="31" />
            <source>No confirmation from the mouse. Close other mouse software and try again.</source>
            <translation>Ingen bekräftelse från musen. Stäng andra musprogram och försök igen.</translation>
        </message>
        <message>
            <location filename="../src/device/mousebackend.cpp" line="40" />
            <source>This diagnostic requires a wired Razer Cobra.</source>
            <translation>Den här diagnosen kräver en kabelansluten Razer Cobra.</translation>
        </message>
        <message>
            <location filename="../src/device/mousebackend.cpp" line="50" />
            <source>Razer Cobra reports DPI X=%1, Y=%2. No settings changed.</source>
            <translation>Razer Cobra rapporterar DPI X=%1, Y=%2. Inga inställningar ändrades.</translation>
        </message>
        <message>
            <location filename="../src/device/mousebackend.cpp" line="58" />
            <source>No supported mouse connected. Connect the mouse using USB and click Refresh.</source>
            <translation>Ingen mus som stöds är ansluten. Anslut musen via USB och klicka på Uppdatera.</translation>
        </message>
        <message>
            <location filename="../src/device/mousebackend.cpp" line="63" />
            <source>Polling rate is not supported for this model. Select Keep current.</source>
            <translation>Avläsningsfrekvensen stöds inte för den här modellen. Välj Behåll aktuell.</translation>
        </message>
        <message>
            <location filename="../src/device/mousebackend.cpp" line="74" />
            <source>This lighting mode is not supported.</source>
            <translation>Det här belysningsläget stöds inte.</translation>
        </message>
        <message>
            <location filename="../src/device/mousebackend.cpp" line="81" />
            <source>Could not open mouse: %1</source>
            <translation>Kunde inte öppna musen: %1</translation>
        </message>
        <message>
            <location filename="../src/device/mousebackend.cpp" line="87" />
            <source>Profile '%1' applied · %2 DPI</source>
            <translation>Profilen '%1' har tillämpats · %2 DPI</translation>
        </message>
        <message>
            <location filename="../src/device/mousebackend.cpp" line="89" />
            <source> Earlier hardware commands have not been rolled back.</source>
            <translation> Tidigare maskinvarukommandon har inte återställts.</translation>
        </message>
    </context>
    <context>
        <name>Platform</name>
        <message>
            <location filename="../src/platform/linux/linuxplatform.cpp" line="46" />
            <location filename="../src/platform/linux/linuxplatform.cpp" line="55" />
            <source>Incomplete HID transfer.</source>
            <translation>Ofullständig HID-överföring.</translation>
        </message>
        <message>
            <location filename="../src/platform/linux/linuxplatform.cpp" line="148" />
            <source>Cannot access %1: %2. Install the supplied udev rule and reconnect the mouse.</source>
            <translation>Kan inte komma åt %1: %2. Installera den medföljande udev-regeln och anslut musen igen.</translation>
        </message>
        <message>
            <location filename="../src/platform/linux/linuxplatform.cpp" line="155" />
            <source>Mouse identity changed. Click Refresh and try again.</source>
            <translation>Musens identitet har ändrats. Klicka på Uppdatera och försök igen.</translation>
        </message>
        <message>
            <location filename="../src/platform/linux/linuxplatform.cpp" line="170" />
            <location filename="../src/platform/linux/linuxplatform.cpp" line="182" />
            <location filename="../src/platform/windows/windowsplatform.cpp" line="107" />
            <source>Could not update automatic startup.</source>
            <translation>Kunde inte ändra automatisk start.</translation>
        </message>
        <message>
            <location filename="../src/platform/linux/linuxplatform.cpp" line="195" />
            <source>Applies to the selected X11/libinput mouse. Games using raw input may ignore these settings.</source>
            <translation>Gäller den valda X11/libinput-musen. Spel med raw input kan ignorera inställningarna.</translation>
        </message>
        <message>
            <location filename="../src/platform/linux/linuxplatform.cpp" line="196" />
            <source>Pointer speed and acceleration are managed by your Wayland desktop, or require xinput on X11. DPI and lighting still work.</source>
            <translation>Pekarhastighet och acceleration hanteras av Wayland-skrivbordet eller kräver xinput på X11. DPI och belysning fungerar fortfarande.</translation>
        </message>
        <message>
            <location filename="../src/platform/linux/linuxplatform.cpp" line="203" />
            <source>Could not find the selected mouse in X11.</source>
            <translation>Kunde inte hitta den valda musen i X11.</translation>
        </message>
        <message>
            <location filename="../src/platform/linux/linuxplatform.cpp" line="227" />
            <source>Could not change the selected mouse's X11/libinput pointer settings.</source>
            <translation>Kunde inte ändra X11/libinput-pekarinställningarna för den valda musen.</translation>
        </message>
        <message>
            <location filename="../src/platform/windows/windowsplatform.cpp" line="159" />
            <source>Applies to all mice. Games using raw input may ignore these settings.</source>
            <translation>Gäller alla möss. Spel med raw input kan ignorera inställningarna.</translation>
        </message>
        <message>
            <location filename="../src/platform/windows/windowsplatform.cpp" line="165" />
            <source>Could not read desktop pointer settings.</source>
            <translation>Kunde inte läsa pekarinställningarna.</translation>
        </message>
        <message>
            <location filename="../src/platform/windows/windowsplatform.cpp" line="171" />
            <source>Could not change pointer acceleration.</source>
            <translation>Kunde inte ändra pekaraccelerationen.</translation>
        </message>
        <message>
            <location filename="../src/platform/windows/windowsplatform.cpp" line="176" />
            <source>Could not change pointer speed.</source>
            <translation>Kunde inte ändra pekarhastigheten.</translation>
        </message>
    </context>
    <context>
        <name>Profiles</name>
        <message>
            <location filename="../src/core/profile.cpp" line="23" />
            <source>Profile</source>
            <translation>Profil</translation>
        </message>
        <message>
            <location filename="../src/core/profile.cpp" line="53" />
            <source>Invalid profile file. The file will not be overwritten.</source>
            <translation>Ogiltig profilfil. Filen skrivs inte över.</translation>
        </message>
        <message>
            <location filename="../src/core/profile.cpp" line="73" />
            <source>Everyday</source>
            <translation>Vardag</translation>
        </message>
        <message>
            <location filename="../src/core/profile.cpp" line="73" />
            <source>Precision</source>
            <translation>Precision</translation>
        </message>
        <message>
            <location filename="../src/core/profile.cpp" line="73" />
            <source>Fast</source>
            <translation>Snabb</translation>
        </message>
        <message>
            <location filename="../src/core/profile.cpp" line="86" />
            <source>Could not create the settings folder.</source>
            <translation>Kunde inte skapa inställningsmappen.</translation>
        </message>
    </context>
    <context>
        <name>Protocol</name>
        <message>
            <location filename="../src/core/razerprotocol.cpp" line="45" />
            <source>DPI is outside this mouse's supported range.</source>
            <translation>DPI ligger utanför det område som musen stöder.</translation>
        </message>
        <message>
            <location filename="../src/core/razerprotocol.cpp" line="49" />
            <source>This mouse supports DPI in steps of 100.</source>
            <translation>Den här musen stöder DPI i steg om 100.</translation>
        </message>
        <message>
            <location filename="../src/core/razerprotocol.cpp" line="80" />
            <source>Unknown protocol argument.</source>
            <translation>Okänt protokollargument.</translation>
        </message>
        <message>
            <location filename="../src/core/razerprotocol.cpp" line="113" />
            <source>Unknown protocol function: %1</source>
            <translation>Okänd protokollfunktion: %1</translation>
        </message>
        <message>
            <location filename="../src/core/razerprotocol.cpp" line="118" />
            <source>Invalid protocol index.</source>
            <translation>Ogiltigt protokollindex.</translation>
        </message>
        <message>
            <location filename="../src/core/razerprotocol.cpp" line="122" />
            <source>Invalid protocol step.</source>
            <translation>Ogiltigt protokollsteg.</translation>
        </message>
    </context>
    <context>
        <name>RazerMouseWindow</name>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="86" />
            <source>No supported USB mouse found</source>
            <translation>Ingen USB-mus som stöds hittades</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="157" />
            <location filename="../src/ui/razermousewindow.cpp" line="434" />
            <source>No profile applied yet</source>
            <translation>Ingen profil har tillämpats ännu</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="161" />
            <source>Your mouse. Your settings. Done.</source>
            <translation>Din mus. Dina inställningar. Klart.</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="168" />
            <source>Refresh</source>
            <translation>Uppdatera</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="171" />
            <source>Looking for USB mice…</source>
            <translation>Söker efter USB-möss…</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="183" />
            <source>PROFILES</source>
            <translation>PROFILER</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="190" />
            <source>Click a profile to apply
it immediately.</source>
            <translation>Klicka på en profil för att
tillämpa den direkt.</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="194" />
            <source>+ New</source>
            <translation>+ Ny</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="195" />
            <location filename="../src/ui/razermousewindow.cpp" line="297" />
            <source>Delete</source>
            <translation>Ta bort</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="210" />
            <source>Profile name</source>
            <translation>Profilnamn</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="212" />
            <source>Sensor</source>
            <translation>Sensor</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="224" />
            <source>Sensitivity</source>
            <translation>Känslighet</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="228" />
            <location filename="../src/ui/razermousewindow.cpp" line="254" />
            <source>Keep current</source>
            <translation>Behåll aktuell</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="230" />
            <source>Polling rate</source>
            <translation>Avläsningsfrekvens</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="232" />
            <source>Desktop pointer</source>
            <translation>Skrivbordspekare</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="234" />
            <source>Include desktop pointer settings in this profile</source>
            <translation>Ta med pekarinställningar i den här profilen</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="240" />
            <source>Speed</source>
            <translation>Hastighet</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="241" />
            <source>Enable pointer acceleration</source>
            <translation>Aktivera pekaracceleration</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="249" />
            <source>Lighting</source>
            <translation>Belysning</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="252" />
            <source>Static color</source>
            <translation>Fast färg</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="253" />
            <source>Off</source>
            <translation>Av</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="258" />
            <source>Effect / color</source>
            <translation>Effekt / färg</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="263" />
            <source>Brightness</source>
            <translation>Ljusstyrka</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="267" />
            <source>Profile color</source>
            <translation>Profilfärg</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="272" />
            <source>Save</source>
            <translation>Spara</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="273" />
            <source>Save and apply</source>
            <translation>Spara och tillämpa</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="277" />
            <source>Save changes before switching profiles.</source>
            <translation>Spara ändringarna innan du byter profil.</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="285" />
            <source>Profile saved. Click the profile to apply it.</source>
            <translation>Profilen har sparats. Klicka på profilen för att tillämpa den.</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="290" />
            <source>New profile</source>
            <translation>Ny profil</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="296" />
            <source>Delete profile</source>
            <translation>Ta bort profil</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="296" />
            <source>Delete profile '%1'?</source>
            <translation>Ta bort profilen '%1'?</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="298" />
            <source>Cancel</source>
            <translation>Avbryt</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="307" />
            <source>Start at login</source>
            <translation>Starta vid inloggning</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="309" />
            <source>Restore active profile on startup / reconnect</source>
            <translation>Återställ aktiv profil vid start / återanslutning</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="315" />
            <source>Language (restart required)</source>
            <translation>Språk (omstart krävs)</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="317" />
            <source>System language</source>
            <translation>Systemspråk</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="325" />
            <source>Language saved. Restart Razer Mouse Lite to apply it.</source>
            <translation>Språket har sparats. Starta om Razer Mouse Lite för att använda det.</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="326" />
            <source>Could not save the language preference.</source>
            <translation>Kunde inte spara språkinställningen.</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="335" />
            <source>Choose a profile or edit its settings.</source>
            <translation>Välj en profil eller redigera dess inställningar.</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="390" />
            <source>Enter a profile name.</source>
            <translation>Ange ett profilnamn.</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="408" />
            <source>Could not save: %1</source>
            <translation>Kunde inte spara: %1</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="417" />
            <source>No supported mouse connected. Connect the mouse using USB and click Refresh.</source>
            <translation>Ingen mus som stöds är ansluten. Anslut musen via USB och klicka på Uppdatera.</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="426" />
            <source>Applying profile '%1'…</source>
            <translation>Tillämpar profilen '%1'…</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="444" />
            <source>Razer Mouse Lite · %1
Click: next profile · Double-click: settings</source>
            <translation>Razer Mouse Lite · %1
Klick: nästa profil · Dubbelklick: inställningar</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="446" />
            <source>Open settings</source>
            <translation>Öppna inställningar</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="447" />
            <source>Quit</source>
            <translation>Avsluta</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="468" />
            <source>Detected automatically · 100–%1 DPI · </source>
            <translation>Identifierad automatiskt · 100–%1 DPI · </translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="469" />
            <source>No RGB control for this model</source>
            <translation>Ingen RGB-styrning för den här modellen</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="469" />
            <source>DPI and lighting over USB</source>
            <translation>DPI och belysning via USB</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="470" />
            <source>Connect your Razer mouse using USB. You can save profiles in advance.</source>
            <translation>Anslut din Razer-mus via USB. Du kan spara profiler i förväg.</translation>
        </message>
        <message>
            <source>About</source>
            <translation>Om</translation>
        </message>
        <message>
            <source>About Razer Mouse Lite</source>
            <translation>Om Razer Mouse Lite</translation>
        </message>
        <message>
            <source>Huge thanks to the OpenRazer and openrazer-win communities for reverse-engineering the device protocols and sharing their work. This app builds on their efforts.</source>
            <translation>Ett stort tack till OpenRazer- och openrazer-win-gemenskaperna för att ha kartlagt enhetsprotokollen och delat sitt arbete. Den här appen bygger på deras insatser.</translation>
        </message>
        <message>
            <source>Want to help support a new device? OpenRazer explains how to capture and analyse USB traffic.</source>
            <translation>Vill du hjälpa till att stödja en ny enhet? OpenRazer förklarar hur du spelar in och analyserar USB-trafik.</translation>
        </message>
        <message>
            <source>USB protocol reverse-engineering guide</source>
            <translation>Guide till reverse engineering av USB-protokollet</translation>
        </message>
        <message>
            <source>Update device definitions</source>
            <translation>Uppdatera enhetsdefinitioner</translation>
        </message>
        <message>
            <source>Checking device definitions…</source>
            <translation>Kontrollerar enhetsdefinitioner…</translation>
        </message>
        <message>
            <source>Could not update device definitions. Check your connection and try again.</source>
            <translation>Det gick inte att uppdatera enhetsdefinitionerna. Kontrollera anslutningen och försök igen.</translation>
        </message>
        <message>
            <source>These device definitions are incompatible or damaged. The current definitions have been kept.</source>
            <translation>Dessa enhetsdefinitioner är inkompatibla eller skadade. De nuvarande definitionerna har behållits.</translation>
        </message>
        <message>
            <source>Device definitions are up to date.</source>
            <translation>Enhetsdefinitionerna är aktuella.</translation>
        </message>
        <message>
            <source>Could not save device definitions: %1</source>
            <translation>Det gick inte att spara enhetsdefinitionerna: %1</translation>
        </message>
        <message>
            <source>Device definitions updated. No mouse settings were changed.</source>
            <translation>Enhetsdefinitionerna har uppdaterats. Inga musinställningar har ändrats.</translation>
        </message>
    </context>
</TS>