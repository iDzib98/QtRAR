// Dialogo de creacion de archivos: el "¿Guardar como" de WinRAR para RAR y ZIP.
//
// Se usa tanto para "Nuevo archivo" como para "Añadir al archivo", que solo
// cambian en el titulo y en el mensaje.
#pragma once

#include <QDialog>
#include <QStringList>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QRadioButton;

namespace qtrar {

class CreateDialog : public QDialog
{
    Q_OBJECT
public:
    enum class Mode { New, Add };

    CreateDialog(const QString &archivePath, Mode mode, QWidget *parent = nullptr);

    /// Ruta del archivo a crear o añadir, tal como la escribe el usuario.
    QString archivePath() const;
    /// Ficheros o carpetas del sistema a incluir, en orden.
    QStringList files() const;
    int compressionLevel() const;
    bool solid() const;
    bool encryptHeaders() const;
    bool hasPassword() const;
    QString password() const;
    QString chosenFormat() const;

    /// Solo para pruebas: rellenan el dialogo como si el usuario lo hubiera
    /// escrito. La validacion de verdad se hace con la interfaz.
    void setFiles(const QStringList &files);
    void setFormat(const QString &format);

    /// Comprueba lo obvio y avisa. Devuelve false si hay algo que corregir.
    bool validate(QString *errorMessage) const;

private:
    /// Anade al combo el formato que corresponde a la extension, y deja el
    /// formato en el desplegable.
    void syncFormatToExtension();

    Mode m_mode;
    QLineEdit *m_pathEdit;
    QLineEdit *m_filesEdit;
    QComboBox *m_formatCombo;
    QComboBox *m_levelCombo;
    QCheckBox *m_solidCheck;
    QCheckBox *m_encryptHeadersCheck;
    QComboBox *m_passwordCombo;
    QLineEdit *m_passwordEdit;
};

} // namespace qtrar
