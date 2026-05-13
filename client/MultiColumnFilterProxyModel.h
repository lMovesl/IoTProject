#ifndef MULTI_COLUMN_FILTER_PROXY_MODEL_H
#define MULTI_COLUMN_FILTER_PROXY_MODEL_H

#include <QSortFilterProxyModel>
#include <QMap>
#include <QDateTime>

enum class FilterType { List, NumericRange, DateRange };

struct ColumnFilter {
    FilterType type = FilterType::List;
    QSet<QString> allowedValues; 
    double minNum = -999999.0, maxNum = 999999.0; 
    QDateTime minDate, maxDate;
    bool isActive = false;
};

class MultiColumnFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit MultiColumnFilterProxyModel(QObject* parent = nullptr) : QSortFilterProxyModel(parent) {}
    void setFilter(int column, const ColumnFilter& filter) {
        m_columnFilters[column] = filter;
        invalidateFilter();
    }
protected:
protected:
    bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override {
        if (source_left.column() == 2 || source_left.column() == 3) {
            QVariant leftData = source_left.data(Qt::UserRole);
            QVariant rightData = source_right.data(Qt::UserRole);

            if (leftData.userType() == QMetaType::Double) {
                return leftData.toDouble() < rightData.toDouble();
            }
            else if (leftData.userType() == QMetaType::QDateTime) {
                return leftData.toDateTime() < rightData.toDateTime();
            }
        }

        return QSortFilterProxyModel::lessThan(source_left, source_right);
    }

    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override {
        for (auto it = m_columnFilters.begin(); it != m_columnFilters.end(); ++it) {
            int col = it.key();
            const ColumnFilter& f = it.value();
            if (!f.isActive) continue;

            QModelIndex index = sourceModel()->index(source_row, col, source_parent);

            if (f.type == FilterType::NumericRange) {
                QVariant data = index.data(Qt::UserRole);
                if (data.isValid() && !data.isNull()) {
                    double val = data.toDouble();
                    if (val < f.minNum || val > f.maxNum) return false;
                }
                else {
                    return false; 
                }
            }
            else if (f.type == FilterType::DateRange) {
                QDateTime rowDate = index.data(Qt::UserRole).toDateTime();
                if (rowDate < f.minDate ||
                    rowDate > f.maxDate) {
                    return false;
                }
            }
            else { // FilterType::List
                QString val = index.data().toString();
                if (!f.allowedValues.isEmpty() && !f.allowedValues.contains(val)) return false;
            }
        }
        return true;
    }

private:
    QMap<int, ColumnFilter> m_columnFilters;
};

#endif //!MULTI_COLUMN_FILTER_PROXY_MODEL_H