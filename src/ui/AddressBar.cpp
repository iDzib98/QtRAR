#include "AddressBar.h"

#include "ThemeManager.h"

#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLineEdit>

namespace qtrar {

AddressBar::AddressBar(QWidget *parent) : QWidget(parent)
{
    m_backButton = new QToolButton(this);
    m_backButton->setText(QStringLiteral("◀"));
    m_backButton->setToolTip(tr("Atras"));

    m_forwardButton = new QToolButton(this);
    m_forwardButton->setText(QStringLiteral("▶"));
    m_forwardButton->setToolTip(tr("Adelante"));

    m_upButton = new QToolButton(this);
    m_upButton->setObjectName(QStringLiteral("upButton"));
    m_upButton->setIcon(ThemeManager::icon(QStringLiteral("up")));
    m_upButton->setToolTip(tr("Subir un nivel"));

    m_combo = new QComboBox(this);
    m_combo->setObjectName(QStringLiteral("addressBar"));
    m_combo->setEditable(true);
    m_combo->setInsertPolicy(QComboBox::NoInsert);
    m_combo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_combo->setToolTip(tr("Ruta actual"));
    m_combo->setMaxVisibleItems(16);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(1);
    layout->addWidget(m_backButton);
    layout->addWidget(m_forwardButton);
    layout->addWidget(m_upButton);
    layout->addWidget(m_combo, 1);

    connect(m_combo, &QComboBox::activated, this, &AddressBar::onActivated);
    if (auto *edit = m_combo->lineEdit()) {
        connect(edit, &QLineEdit::returnPressed, this, &AddressBar::onEditActivated);
    }
    connect(m_upButton, &QToolButton::clicked, this, &AddressBar::upRequested);
    connect(m_backButton, &QToolButton::clicked, this, &AddressBar::goBack);
    connect(m_forwardButton, &QToolButton::clicked, this, &AddressBar::goForward);
}

QString AddressBar::displayText() const
{
    if (m_archivePath.isEmpty())
        return QDir::toNativeSeparators(m_currentPath);
    if (m_currentPath.isEmpty())
        return tr("raiz");
    return m_currentPath;
}

void AddressBar::setArchivePath(const QString &archivePath, const QString &currentPath)
{
    m_internalChange = true;
    m_archivePath = archivePath;
    m_history.clear();
    m_historyPos = -1;
    m_combo->clear();
    m_combo->addItem(tr("raiz"));
    m_internalChange = false;
    setInternalPath(currentPath);
}

void AddressBar::setFileSystemPath(const QString &path)
{
    const QString clean = QDir::cleanPath(QDir(path).absolutePath());
    const bool returnedFromArchive = !m_archivePath.isEmpty();
    m_internalChange = true;
    m_archivePath.clear();
    if (returnedFromArchive) {
        m_history.clear();
        m_historyPos = -1;
    }
    m_currentPath = clean;
    m_combo->clear();

    QStringList ancestors;
    QString current = clean;
    while (!current.isEmpty()) {
        ancestors.prepend(current);
        const QString parent = QFileInfo(current).absolutePath();
        if (parent == current)
            break;
        current = parent;
    }
    for (const QString &ancestor : ancestors)
        m_combo->addItem(QDir::toNativeSeparators(ancestor));
    const int idx = m_combo->findText(QDir::toNativeSeparators(clean));
    m_combo->setCurrentIndex(idx >= 0 ? idx : m_combo->count() - 1);
    if (m_combo->lineEdit())
        m_combo->setCurrentText(QDir::toNativeSeparators(clean));

    if (m_history.value(m_historyPos) != clean) {
        while (m_history.size() > m_historyPos + 1)
            m_history.removeLast();
        m_history.append(clean);
        m_historyPos = m_history.size() - 1;
    }
    m_backButton->setEnabled(m_historyPos > 0);
    m_forwardButton->setEnabled(m_historyPos >= 0 && m_historyPos < m_history.size() - 1);
    m_upButton->setEnabled(QFileInfo(clean).absolutePath() != clean);
    m_internalChange = false;
}

void AddressBar::setInternalPath(const QString &currentPath)
{
    m_internalChange = true;
    m_currentPath = currentPath;

    // El desplegable ofrece los ancestros, como hace WinRAR.
    m_combo->clear();
    m_combo->addItem(tr("raiz"));
    QString acc;
    const QStringList parts = currentPath.split(u'/', Qt::SkipEmptyParts);
    for (int i = 0; i < parts.size(); ++i) {
        acc += (acc.isEmpty() ? QString() : QStringLiteral("/")) + parts.at(i);
        m_combo->addItem(acc);
    }
    const int idx = m_combo->findText(currentPath.isEmpty() ? tr("raiz") : currentPath);
    m_combo->setCurrentIndex(idx >= 0 ? idx : 0);
    if (m_combo->lineEdit())
        m_combo->setCurrentText(displayText());

    if (m_history.value(m_historyPos) != currentPath) {
        while (m_history.size() > m_historyPos + 1)
            m_history.removeLast();
        m_history.append(currentPath);
        m_historyPos = m_history.size() - 1;
    }
    m_backButton->setEnabled(m_historyPos > 0);
    m_forwardButton->setEnabled(m_historyPos >= 0 && m_historyPos < m_history.size() - 1);
    // En la raiz de un archivo, "subir" vuelve a la carpeta local que lo
    // contiene, como en WinRAR.
    m_upButton->setEnabled(!currentPath.isEmpty() || !m_archivePath.isEmpty());
    m_internalChange = false;
}

bool AddressBar::goBack()
{
    if (!canGoBack())
        return false;
    --m_historyPos;
    const QString path = m_history.at(m_historyPos);
    if (m_archivePath.isEmpty())
        setFileSystemPath(path);
    else
        setInternalPath(path);
    emit pathActivated(m_currentPath);
    return true;
}

bool AddressBar::goForward()
{
    if (!canGoForward())
        return false;
    ++m_historyPos;
    const QString path = m_history.at(m_historyPos);
    if (m_archivePath.isEmpty())
        setFileSystemPath(path);
    else
        setInternalPath(path);
    emit pathActivated(m_currentPath);
    return true;
}

void AddressBar::setEnabledAddress(bool enabled)
{
    m_combo->setEnabled(enabled);
    m_backButton->setEnabled(enabled && canGoBack());
    m_forwardButton->setEnabled(enabled && canGoForward());
    m_upButton->setEnabled(enabled
                           && (!m_currentPath.isEmpty() || !m_archivePath.isEmpty()));
}

void AddressBar::onActivated(int index)
{
    if (m_internalChange || index < 0)
        return;
    if (m_archivePath.isEmpty()) {
        emit pathActivated(QDir::fromNativeSeparators(m_combo->itemText(index)));
        return;
    }
    if (index == 0) {
        emit pathActivated(QString());
        return;
    }
    emit pathActivated(m_combo->itemText(index));
}

void AddressBar::onEditActivated()
{
    if (m_internalChange || !m_combo->lineEdit())
        return;
    const QString text = m_combo->lineEdit()->text().trimmed();
    if (!m_archivePath.isEmpty() && text == tr("raiz"))
        emit pathActivated(QString());
    else
        emit pathActivated(QDir::fromNativeSeparators(text));
}

} // namespace qtrar
