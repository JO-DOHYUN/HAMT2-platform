#pragma once

#include <QSortFilterProxyModel>

class FrameFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
    Q_PROPERTY(QString idFilter READ idFilter WRITE setIdFilter NOTIFY idFilterChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
public:
    explicit FrameFilterProxyModel(QObject* parent = nullptr);

    QString idFilter() const { return m_idFilter; }
    void setIdFilter(const QString& text);
    int count() const { return rowCount(); }

signals:
    void idFilterChanged();
    void countChanged();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
    QString m_idFilter;
};
