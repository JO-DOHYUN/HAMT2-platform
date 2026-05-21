#include "FrameFilterProxyModel.h"

#include "FrameListModel.h"

#include <QAbstractItemModel>
#include <QRegularExpression>

namespace {
bool parseTokenToId(const QString& token, quint32* out) {
    if (!out) return false;
    const QString t = token.trimmed();
    if (t.isEmpty()) return false;
    bool ok = false;
    quint32 value = 0;
    if (t.startsWith(QStringLiteral("0x"), Qt::CaseInsensitive)) value = t.mid(2).toUInt(&ok, 16);
    else value = t.toUInt(&ok, 10);
    if (!ok) return false;
    *out = value;
    return true;
}
}

FrameFilterProxyModel::FrameFilterProxyModel(QObject* parent) : QSortFilterProxyModel(parent) {
    setDynamicSortFilter(true);
    connect(this, &QSortFilterProxyModel::modelReset, this, &FrameFilterProxyModel::countChanged);
    connect(this, &QSortFilterProxyModel::rowsInserted, this, &FrameFilterProxyModel::countChanged);
    connect(this, &QSortFilterProxyModel::rowsRemoved, this, &FrameFilterProxyModel::countChanged);
    connect(this, &QSortFilterProxyModel::layoutChanged, this, &FrameFilterProxyModel::countChanged);
}

void FrameFilterProxyModel::setIdFilter(const QString& text) {
    const QString normalized = text.trimmed();
    if (m_idFilter == normalized) return;
    m_idFilter = normalized;
    emit idFilterChanged();
    invalidate();
    emit countChanged();
}

bool FrameFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
    if (m_idFilter.trimmed().isEmpty()) return true;
    const QAbstractItemModel* src = sourceModel();
    if (!src) return true;

    const QModelIndex idx = src->index(sourceRow, 0, sourceParent);
    const QString idText = src->data(idx, FrameListModel::IdTextRole).toString();
    const quint32 idValue = src->data(idx, FrameListModel::IdRole).toUInt();
    const QString normalizedIdText = idText.toUpper();

    const QStringList tokens = m_idFilter.split(QRegularExpression(QStringLiteral("[,;\\s]+")), Qt::SkipEmptyParts);
    for (const QString& rawToken : tokens) {
        const QString token = rawToken.trimmed();
        if (token.isEmpty()) continue;
        quint32 tokenId = 0;
        if (parseTokenToId(token, &tokenId) && tokenId == idValue) return true;
        if (normalizedIdText.contains(token.toUpper(), Qt::CaseInsensitive)) return true;
    }
    return false;
}
