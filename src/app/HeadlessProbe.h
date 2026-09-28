// Banco de pruebas sin interfaz, y tambien las operaciones que se piden desde
// la linea de ordenes.
//
// La aplicacion es grafica, pero todo el nucleo (listar, probar, extraer,
// crear) se apoya en procesos externos, y eso se puede comprobar de verdad sin
// pantalla. Estas sondas imprimen el resultado por stdout y devuelven el codigo
// de salida, de modo que sirvan tanto a mano como desde un script.
#pragma once

#include <QStringList>

namespace qtrar {

/// Peticion de una operacion sin interfaz. La construye `main` con lo que ha
/// parseado `Application` mas las variables de entorno de prueba, de forma que
/// aqui no haya que volver a interpretar `argv`: eso duplicaba el criterio de
/// "esto es un valor de opcion, no un archivo" y por el camino se perdian
/// opciones como `--extract-to`.
struct ProbeRequest {
    QString archivePath;

    bool dump = false;       ///< Volcar el listado y salir (`--dump`).
    bool volumes = false;   ///< Comprobar que estan todos los volumenes.
    bool test = false;      ///< Verificar la integridad del archivo.
    bool list = false;      ///< Listar sin mas detalle (comando `v`).

    QString extractTo;      ///< Carpeta destino; vacio = no extraer.
    QStringList extractItems;   ///< Vacio = todo el archivo.

    QString createPath;     ///< Archivo a crear; vacio = no crear.
    QStringList createItems;

    bool readComment = false;  ///< Volcar el comentario del archivo.
    QString commentFile;       ///< Fichero cuyo contenido se pone como comentario.

    QString removeItems;    ///< Int separad por comas, o vacio.
    QString renameFrom;
    QString renameTo;

    QString password;
};

/// Si la peticion pide alguna operacion sin interfaz, la ejecuta, escribe el
/// resultado por stdout, deja el codigo de salida en `exitCode` y devuelve
/// true. Si no hay nada que hacer, devuelve false y `main` sigue con la ventana.
bool runHeadlessProbe(const ProbeRequest &request, int &exitCode);

} // namespace qtrar
