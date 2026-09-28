# Integración con el explorador de archivos

QtRAR instala varias integraciones de escritorio:

- El archivo `qtrar.desktop` registra RAR y ZIP para **Abrir con QtRAR** y ofrece
  las acciones rápidas *Extraer aquí*, *Extraer en…* y *Comprobar*.
- Dolphin muestra un submenú **QtRAR** para archivos comprimidos. En archivos y
  carpetas ofrece *Añadir a un archivo…*.
- Nautilus (Archivos de GNOME) muestra las acciones bajo **Scripts → QtRAR**.

Después de instalar QtRAR, actualiza la base de datos de aplicaciones para que
los nuevos tipos MIME aparezcan inmediatamente:

```sh
update-desktop-database
```

Si quieres que QtRAR sea el abridor predeterminado:

```sh
xdg-mime default qtrar.desktop application/vnd.rar
xdg-mime default qtrar.desktop application/zip
```

En una compilación local se instalan junto al ejecutable con:

```sh
cmake --install build --prefix ~/.local
```

Las acciones de Nautilus se instalan en el directorio de scripts del sistema;
si Nautilus ya estaba abierto, reinícialo para que recargue el submenú.
