// Arranque de la aplicacion: tema, traducciones, instancia unica y
// argumentos de la linea de ordenes.
#pragma once

#include <QApplication>
#include <QCommandLineParser>
#include <QStringList>
#include <QTranslator>

namespace qtrar {

class Application : public QApplication
{
    Q_OBJECT
public:
    Application(int &argc, char **argv);
    ~Application() override;

    /// Procesa la linea de ordenes. Devuelve false si solo se muestra ayuda.
    bool setup(const QStringList &arguments);

    /// Traduce la interfaz segun las preferencias o el idioma del sistema.
    void loadTranslations();

    /// Modo prueba: imprime el listado por stdout en vez de abrir la ventana.
    bool dumpListingRequested() const { return m_dumpListing; }

    /// Carpeta de `--extract-to`. Vacia si no se pidio esa operacion.
    QString extractTo() const { return m_extractTo; }
    bool testRequested() const { return m_test; }
    bool extractToDialogRequested() const { return m_extractToDialog; }
    bool addToArchiveRequested() const { return m_addToArchive; }
    bool hasGuiAction() const { return m_extractToDialog || m_addToArchive; }

    /// Archivos pedidos en la linea de ordenes. Lo decide el parser, no un
    /// recorrido a mano: si no, el valor de una opcion ("-l en") se tomaria
    /// por un nombre de archivo.
    QStringList archiveArguments() const { return m_parser.positionalArguments(); }

    /// Intenta tomar el papel de instancia principal. Devuelve false si ya hay
    /// otra instancia, en cuyo caso le ha enviado la peticion.
    bool claimPrimaryInstance();

signals:
    /// Otra instancia ha pedido abrir este archivo.
    void openArchiveRequested(const QString &path);

private:
    QTranslator m_qtTranslator;
    QTranslator m_appTranslator;
    QCommandLineParser m_parser;
    QString m_language;
    QString m_extractTo;
    bool m_dumpListing = false;
    bool m_test = false;
    bool m_extractToDialog = false;
    bool m_addToArchive = false;
};

} // namespace qtrar
