<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS []>
<TS version="2.1" language="es_ES" sourcelanguage="en_US">
    <context>
        <name>Backend</name>
        <message>
            <location filename="../src/device/mousebackend.cpp" line="17" />
            <source>HID write failed: %1</source>
            <translation>Error al escribir HID: %1</translation>
        </message>
        <message>
            <location filename="../src/device/mousebackend.cpp" line="21" />
            <source>HID read failed: %1</source>
            <translation>Error al leer HID: %1</translation>
        </message>
        <message>
            <location filename="../src/device/mousebackend.cpp" line="27" />
            <source>Mouse rejected command %1:%2 (status %3). Close other mouse software and try again.</source>
            <translation>El ratón rechazó el comando %1:%2 (estado %3). Cierra otros programas del ratón e inténtalo de nuevo.</translation>
        </message>
        <message>
            <location filename="../src/device/mousebackend.cpp" line="31" />
            <source>No confirmation from the mouse. Close other mouse software and try again.</source>
            <translation>El ratón no ha confirmado el comando. Cierra otros programas del ratón e inténtalo de nuevo.</translation>
        </message>
        <message>
            <location filename="../src/device/mousebackend.cpp" line="40" />
            <source>This diagnostic requires a wired Razer Cobra.</source>
            <translation>Este diagnóstico requiere un Razer Cobra con cable.</translation>
        </message>
        <message>
            <location filename="../src/device/mousebackend.cpp" line="50" />
            <source>Razer Cobra reports DPI X=%1, Y=%2. No settings changed.</source>
            <translation>Razer Cobra indica PPP X=%1, Y=%2. No se ha cambiado ningún ajuste.</translation>
        </message>
        <message>
            <location filename="../src/device/mousebackend.cpp" line="58" />
            <source>No supported mouse connected. Connect the mouse using USB and click Refresh.</source>
            <translation>No hay ningún ratón compatible conectado. Conéctalo por USB y pulsa Actualizar.</translation>
        </message>
        <message>
            <location filename="../src/device/mousebackend.cpp" line="63" />
            <source>Polling rate is not supported for this model. Select Keep current.</source>
            <translation>Este modelo no admite cambiar la frecuencia de sondeo. Selecciona Mantener actual.</translation>
        </message>
        <message>
            <location filename="../src/device/mousebackend.cpp" line="74" />
            <source>This lighting mode is not supported.</source>
            <translation>Este modo de iluminación no es compatible.</translation>
        </message>
        <message>
            <location filename="../src/device/mousebackend.cpp" line="81" />
            <source>Could not open mouse: %1</source>
            <translation>No se pudo abrir el ratón: %1</translation>
        </message>
        <message>
            <location filename="../src/device/mousebackend.cpp" line="87" />
            <source>Profile '%1' applied · %2 DPI</source>
            <translation>Perfil '%1' aplicado · %2 PPP</translation>
        </message>
        <message>
            <location filename="../src/device/mousebackend.cpp" line="89" />
            <source> Earlier hardware commands have not been rolled back.</source>
            <translation> Los comandos de hardware anteriores no se han revertido.</translation>
        </message>
    </context>
    <context>
        <name>Platform</name>
        <message>
            <location filename="../src/platform/linux/linuxplatform.cpp" line="46" />
            <location filename="../src/platform/linux/linuxplatform.cpp" line="55" />
            <source>Incomplete HID transfer.</source>
            <translation>Transferencia HID incompleta.</translation>
        </message>
        <message>
            <location filename="../src/platform/linux/linuxplatform.cpp" line="148" />
            <source>Cannot access %1: %2. Install the supplied udev rule and reconnect the mouse.</source>
            <translation>No se puede acceder a %1: %2. Instala la regla udev incluida y vuelve a conectar el ratón.</translation>
        </message>
        <message>
            <location filename="../src/platform/linux/linuxplatform.cpp" line="155" />
            <source>Mouse identity changed. Click Refresh and try again.</source>
            <translation>La identificación del ratón ha cambiado. Pulsa Actualizar e inténtalo de nuevo.</translation>
        </message>
        <message>
            <location filename="../src/platform/linux/linuxplatform.cpp" line="170" />
            <location filename="../src/platform/linux/linuxplatform.cpp" line="182" />
            <location filename="../src/platform/windows/windowsplatform.cpp" line="107" />
            <source>Could not update automatic startup.</source>
            <translation>No se pudo cambiar el inicio automático.</translation>
        </message>
        <message>
            <location filename="../src/platform/linux/linuxplatform.cpp" line="195" />
            <source>Applies to the selected X11/libinput mouse. Games using raw input may ignore these settings.</source>
            <translation>Se aplica al ratón X11/libinput seleccionado. Los juegos con entrada directa pueden ignorar estos ajustes.</translation>
        </message>
        <message>
            <location filename="../src/platform/linux/linuxplatform.cpp" line="196" />
            <source>Pointer speed and acceleration are managed by your Wayland desktop, or require xinput on X11. DPI and lighting still work.</source>
            <translation>La velocidad y la aceleración del puntero las gestiona tu escritorio Wayland, o requieren xinput en X11. Los PPP y la iluminación siguen funcionando.</translation>
        </message>
        <message>
            <location filename="../src/platform/linux/linuxplatform.cpp" line="203" />
            <source>Could not find the selected mouse in X11.</source>
            <translation>No se encontró el ratón seleccionado en X11.</translation>
        </message>
        <message>
            <location filename="../src/platform/linux/linuxplatform.cpp" line="227" />
            <source>Could not change the selected mouse's X11/libinput pointer settings.</source>
            <translation>No se pudieron cambiar los ajustes del puntero X11/libinput del ratón seleccionado.</translation>
        </message>
        <message>
            <location filename="../src/platform/windows/windowsplatform.cpp" line="159" />
            <source>Applies to all mice. Games using raw input may ignore these settings.</source>
            <translation>Se aplica a todos los ratones. Los juegos con entrada directa pueden ignorar estos ajustes.</translation>
        </message>
        <message>
            <location filename="../src/platform/windows/windowsplatform.cpp" line="165" />
            <source>Could not read desktop pointer settings.</source>
            <translation>No se pudieron leer los ajustes del puntero.</translation>
        </message>
        <message>
            <location filename="../src/platform/windows/windowsplatform.cpp" line="171" />
            <source>Could not change pointer acceleration.</source>
            <translation>No se pudo cambiar la aceleración del puntero.</translation>
        </message>
        <message>
            <location filename="../src/platform/windows/windowsplatform.cpp" line="176" />
            <source>Could not change pointer speed.</source>
            <translation>No se pudo cambiar la velocidad del puntero.</translation>
        </message>
    </context>
    <context>
        <name>Profiles</name>
        <message>
            <location filename="../src/core/profile.cpp" line="23" />
            <source>Profile</source>
            <translation>Perfil</translation>
        </message>
        <message>
            <location filename="../src/core/profile.cpp" line="53" />
            <source>Invalid profile file. The file will not be overwritten.</source>
            <translation>Archivo de perfiles no válido. El archivo no se sobrescribirá.</translation>
        </message>
        <message>
            <location filename="../src/core/profile.cpp" line="73" />
            <source>Everyday</source>
            <translation>Diario</translation>
        </message>
        <message>
            <location filename="../src/core/profile.cpp" line="73" />
            <source>Precision</source>
            <translation>Precisión</translation>
        </message>
        <message>
            <location filename="../src/core/profile.cpp" line="73" />
            <source>Fast</source>
            <translation>Rápido</translation>
        </message>
        <message>
            <location filename="../src/core/profile.cpp" line="86" />
            <source>Could not create the settings folder.</source>
            <translation>No se pudo crear la carpeta de ajustes.</translation>
        </message>
    </context>
    <context>
        <name>Protocol</name>
        <message>
            <location filename="../src/core/razerprotocol.cpp" line="45" />
            <source>DPI is outside this mouse's supported range.</source>
            <translation>Los PPP están fuera del rango admitido por este ratón.</translation>
        </message>
        <message>
            <location filename="../src/core/razerprotocol.cpp" line="49" />
            <source>This mouse supports DPI in steps of 100.</source>
            <translation>Este ratón admite PPP en incrementos de 100.</translation>
        </message>
        <message>
            <location filename="../src/core/razerprotocol.cpp" line="80" />
            <source>Unknown protocol argument.</source>
            <translation>Argumento de protocolo desconocido.</translation>
        </message>
        <message>
            <location filename="../src/core/razerprotocol.cpp" line="113" />
            <source>Unknown protocol function: %1</source>
            <translation>Función de protocolo desconocida: %1</translation>
        </message>
        <message>
            <location filename="../src/core/razerprotocol.cpp" line="118" />
            <source>Invalid protocol index.</source>
            <translation>Índice de protocolo no válido.</translation>
        </message>
        <message>
            <location filename="../src/core/razerprotocol.cpp" line="122" />
            <source>Invalid protocol step.</source>
            <translation>Paso de protocolo no válido.</translation>
        </message>
    </context>
    <context>
        <name>RazerMouseWindow</name>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="86" />
            <source>No supported USB mouse found</source>
            <translation>No se encontró ningún ratón USB compatible</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="157" />
            <location filename="../src/ui/razermousewindow.cpp" line="434" />
            <source>No profile applied yet</source>
            <translation>Ningún perfil aplicado todavía</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="161" />
            <source>Your mouse. Your settings. Done.</source>
            <translation>Tu ratón. Tus ajustes. Listo.</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="168" />
            <source>Refresh</source>
            <translation>Actualizar</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="171" />
            <source>Looking for USB mice…</source>
            <translation>Buscando ratones USB…</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="183" />
            <source>PROFILES</source>
            <translation>PERFILES</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="190" />
            <source>Click a profile to apply
it immediately.</source>
            <translation>Pulsa un perfil para
aplicarlo al instante.</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="194" />
            <source>+ New</source>
            <translation>+ Nuevo</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="195" />
            <location filename="../src/ui/razermousewindow.cpp" line="297" />
            <source>Delete</source>
            <translation>Eliminar</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="210" />
            <source>Profile name</source>
            <translation>Nombre del perfil</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="212" />
            <source>Sensor</source>
            <translation>Sensor</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="224" />
            <source>Sensitivity</source>
            <translation>Sensibilidad</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="228" />
            <location filename="../src/ui/razermousewindow.cpp" line="254" />
            <source>Keep current</source>
            <translation>Mantener actual</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="230" />
            <source>Polling rate</source>
            <translation>Frecuencia de sondeo</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="232" />
            <source>Desktop pointer</source>
            <translation>Puntero del escritorio</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="234" />
            <source>Include desktop pointer settings in this profile</source>
            <translation>Incluir ajustes del puntero en este perfil</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="240" />
            <source>Speed</source>
            <translation>Velocidad</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="241" />
            <source>Enable pointer acceleration</source>
            <translation>Activar la aceleración del puntero</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="249" />
            <source>Lighting</source>
            <translation>Iluminación</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="252" />
            <source>Static color</source>
            <translation>Color fijo</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="253" />
            <source>Off</source>
            <translation>Apagado</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="258" />
            <source>Effect / color</source>
            <translation>Efecto / color</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="263" />
            <source>Brightness</source>
            <translation>Brillo</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="267" />
            <source>Profile color</source>
            <translation>Color del perfil</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="272" />
            <source>Save</source>
            <translation>Guardar</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="273" />
            <source>Save and apply</source>
            <translation>Guardar y aplicar</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="277" />
            <source>Save changes before switching profiles.</source>
            <translation>Guarda los cambios antes de cambiar de perfil.</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="285" />
            <source>Profile saved. Click the profile to apply it.</source>
            <translation>Perfil guardado. Pulsa el perfil para aplicarlo.</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="290" />
            <source>New profile</source>
            <translation>Nuevo perfil</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="296" />
            <source>Delete profile</source>
            <translation>Eliminar perfil</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="296" />
            <source>Delete profile '%1'?</source>
            <translation>¿Eliminar el perfil '%1'?</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="298" />
            <source>Cancel</source>
            <translation>Cancelar</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="307" />
            <source>Start at login</source>
            <translation>Iniciar al acceder a la sesión</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="309" />
            <source>Restore active profile on startup / reconnect</source>
            <translation>Restaurar el perfil activo al iniciar / reconectar</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="315" />
            <source>Language (restart required)</source>
            <translation>Idioma (requiere reiniciar)</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="317" />
            <source>System language</source>
            <translation>Idioma del sistema</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="325" />
            <source>Language saved. Restart Razer Mouse Lite to apply it.</source>
            <translation>Idioma guardado. Reinicia Razer Mouse Lite para aplicarlo.</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="326" />
            <source>Could not save the language preference.</source>
            <translation>No se pudo guardar el idioma elegido.</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="335" />
            <source>Choose a profile or edit its settings.</source>
            <translation>Elige un perfil o modifica sus ajustes.</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="390" />
            <source>Enter a profile name.</source>
            <translation>Introduce un nombre para el perfil.</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="408" />
            <source>Could not save: %1</source>
            <translation>No se pudo guardar: %1</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="417" />
            <source>No supported mouse connected. Connect the mouse using USB and click Refresh.</source>
            <translation>No hay ningún ratón compatible conectado. Conéctalo por USB y pulsa Actualizar.</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="426" />
            <source>Applying profile '%1'…</source>
            <translation>Aplicando el perfil '%1'…</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="444" />
            <source>Razer Mouse Lite · %1
Click: next profile · Double-click: settings</source>
            <translation>Razer Mouse Lite · %1
Clic: siguiente perfil · Doble clic: ajustes</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="446" />
            <source>Open settings</source>
            <translation>Abrir ajustes</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="447" />
            <source>Quit</source>
            <translation>Salir</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="468" />
            <source>Detected automatically · 100–%1 DPI · </source>
            <translation>Detectado automáticamente · 100–%1 PPP · </translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="469" />
            <source>No RGB control for this model</source>
            <translation>Sin control RGB para este modelo</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="469" />
            <source>DPI and lighting over USB</source>
            <translation>PPP e iluminación por USB</translation>
        </message>
        <message>
            <location filename="../src/ui/razermousewindow.cpp" line="470" />
            <source>Connect your Razer mouse using USB. You can save profiles in advance.</source>
            <translation>Conecta tu ratón Razer por USB. Puedes guardar perfiles de antemano.</translation>
        </message>
    </context>
</TS>