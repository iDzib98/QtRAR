// Panel inferior de WinRAR: a la izquierda el recuento de ficheros y carpetas
// con los totales, a la derecha el indicador de espacio libre del destino.
#pragma once

#include <QString>
#include <QWidget>

class QLabel;
class QProgressBar;

namespace qtrar {

class StatusPanel : public QWidget
{
    Q_OBJECT
public:
    explicit StatusPanel(QWidget *parent = nullptr);

    void setCounts(int files, int folders, qint64 bytes, qint64 packed);
    void setSelectionInfo(const QString &text);
    void setLicenseText(const QString &text);
    void setLicenseWarning(bool warning);

    /// Muestra el espacio libre de la carpeta de destino.
    void setFreeSpace(const QString &path, qint64 total, qint64 free);

    /// Texto tal cual se ve en la barra: recuento, seleccion y licencia.
    QString countsText() const;
    void clearFreeSpace();

private:
    QLabel *m_countsLabel;
    QLabel *m_selectionLabel;
    QLabel *m_licenseLabel;
    QLabel *m_freeSpaceLabel;
    QProgressBar *m_freeSpaceBar;
};

} // namespace qtrar
