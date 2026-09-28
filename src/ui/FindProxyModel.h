// Filtro de busqueda para la vista de archivos.
//
// WinRAR filtra mientras se escribe, en la propia barra de direcciones. Aqui se
// aplica sobre el proxy, de modo que ordena y filtra a la vez, y solo se deja
// pasar lo que coincide con el texto.
#pragma once

#include <QSortFilterProxyModel>

namespace qtrar {

class FindProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit FindProxyModel(QObject *parent = nullptr);

    /// Texto buscado. Vacio = sin filtro.
    QString searchText() const { return m_text; }
    void setSearchText(const QString &text);

    /// Las wildcards de WinRAR: `*.txt` y `?` funcionan como alli, y ademas se
    /// acepta un patron sin comodines que busca en cualquier parte del nombre.
    /// "a"  ->  coincide si el nombre contiene "a"
    /// "*.a" -> coincide si termina en ".a"
    static bool matchesPattern(const QString &name, const QString &pattern);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    QString m_text;
    /// Cache de lo que sepidio a `entryPath` para no repetir el calculo en
    /// cada fila: Qt pregunta una vez por fila visible, pero tambien al
    /// invalidar.
    mutable QHash<QString, bool> m_cache;
};

} // namespace qtrar
