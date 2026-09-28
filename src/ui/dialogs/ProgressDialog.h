// Dialogo de progreso de operaciones largas, con cancelacion.
//
// WinRAR muestra el nombre del fichero actual y un porcentaje. El porcentaje
// se calcula con granularidad de fichero a partir del listado que ya se tiene
// en memoria, porque `unrar` solo emite porcentajes cuando procesa cada
// miembro: se progreso por bytes reales, no inventado.
#pragma once

#include <QDialog>
#include <QStringList>

class QLabel;
class QProgressBar;
class QPushButton;

namespace qtrar {

class ProgressDialog : public QDialog
{
    Q_OBJECT
public:
    ProgressDialog(const QString &title, int totalSteps, QWidget *parent = nullptr);

    void setCurrentFile(const QString &name);
    void setStatusText(const QString &text);
    void setValue(int value);
    void setMaximum(int value);
    /// Marca un paso mas como completado y actualiza el porcentaje.
    void advance();
    int value() const;
    bool wasCancelled() const { return m_cancelled; }

signals:
    void cancelled();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QLabel *m_fileLabel;
    QLabel *m_statusLabel;
    QProgressBar *m_bar;
    QPushButton *m_cancelButton;
    int m_totalSteps = 0;
    bool m_cancelled = false;
};

} // namespace qtrar
