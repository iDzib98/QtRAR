// Barra de direcciones de WinRAR: campo editable con desplegable de historial,
// boton de subir un nivel y navegacion con flechas.
#pragma once

#include <QComboBox>
#include <QToolButton>
#include <QWidget>

class QHBoxLayout;

namespace qtrar {

class AddressBar : public QWidget
{
    Q_OBJECT
public:
    explicit AddressBar(QWidget *parent = nullptr);

    /// Muestra una ruta interna del archivo. La raiz se muestra como "raiz".
    void setArchivePath(const QString &archivePath, const QString &currentPath);
    void setInternalPath(const QString &currentPath);
    /// Muestra una ruta del sistema y sus ancestros en el desplegable.
    void setFileSystemPath(const QString &path);

    QString currentPath() const { return m_currentPath; }
    /// Texto que ve el usuario en la barra (ruta o nombre del archivo).
    QString displayText() const;
    void setEnabledAddress(bool enabled);

    /// Atrás y adelante sobre el historial de carpetas visitadas. La ventana
    /// responde a esto con `goTo()`; aqui solo se mueve el puntero y se avisa.
    bool goBack();
    bool goForward();
    bool canGoBack() const { return m_historyPos > 0; }
    bool canGoForward() const { return m_historyPos >= 0 && m_historyPos < m_history.size() - 1; }

signals:
    /// El usuario ha escrito o elegido una ruta interna.
    void pathActivated(const QString &internalPath);
    void upRequested();

private slots:
    void onActivated(int index);
    void onEditActivated();

private:
    QComboBox *m_combo;
    QToolButton *m_backButton;
    QToolButton *m_forwardButton;
    QToolButton *m_upButton;
    QString m_archivePath;
    QString m_currentPath;
    QStringList m_history;
    int m_historyPos = -1;
    bool m_internalChange = false;
};

} // namespace qtrar
