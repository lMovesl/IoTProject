#ifndef MEASUREMENTHISTORYWINDOW_H
#define MEASUREMENTHISTORYWINDOW_H

#include <QDialog>
#include <QTableView>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QDateTime>
#include <QStandardItemModel>

#include "MultiColumnFilterProxyModel.h"

class MeasurementHistoryWindow : public QDialog {
    Q_OBJECT
public:
    explicit MeasurementHistoryWindow(QWidget* parent = nullptr);

private slots:
    void exportToCsv();
private:
    void onFilterOpened(int column, QPointF pos);
    void loadHistory(); 

    QTableView* m_tableView;
    QStandardItemModel* m_sourceModel;
    MultiColumnFilterProxyModel* m_proxyModel;

    QMap<int, ColumnFilter> m_currentFilters;
};

#endif