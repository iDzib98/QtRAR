# Crear el paquete Debian

En Debian/Ubuntu, desde la raíz del proyecto:

```sh
cmake -S . -B build-deb \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=OFF \
  -DQTRAR_FETCH_RAR=OFF \
  -DQTRAR_BUNDLE_UNRAR=ON
cmake --build build-deb -j
cpack --config build-deb/CPackConfig.cmake -G DEB
```

El paquete incluye `unrar` (componente que su EULA permite redistribuir), las
traducciones y las integraciones con Dolphin y Nautilus. No incluye `rar` ni
claves de licencia: se recomienda instalar 7-Zip para ZIP y añadir el binario
`rar` oficial por separado si se necesita crear o modificar RAR.
