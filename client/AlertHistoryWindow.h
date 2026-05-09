#ifndef ALERTHISTORYWINDOW_H
#define ALERTHISTORYWINDOW_H

#include <QDialog>
#include <QTableView>
#include <QStandardItemModel>
#include <QMap>
#include "MultiColumnFilterProxyModel.h"
#include "FilterPopup.h"
#include "FilterHeader.h"

class AlertHistoryWindow : public QDialog {
    Q_OBJECT
public:
    explicit AlertHistoryWindow(QWidget* parent = nullptr);

private slots:
    void onFilterOpened(int column, QPointF pos);

private:
    QTableView* m_view;
    QStandardItemModel* m_sourceModel;
    MultiColumnFilterProxyModel* m_proxyModel;
    QMap<int, ColumnFilter> m_currentFilters;

    void loadAlerts();
};

#endif // ALERTHISTORYWINDOW_H