#ifndef CONFIGUREDEVICEDIALOG_H
#define CONFIGUREDEVICEDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include "DatabaseManager.h"

class ConfigureDeviceDialog : public QDialog {
    Q_OBJECT

public:
    explicit ConfigureDeviceDialog(int deviceId, QWidget* parent = nullptr);

private slots:
    void onSaveClicked();

private:
    int m_deviceId;

    QLineEdit* m_nameEdit;
    QComboBox* m_roomCombo;
    QLabel* m_idLabel;

    void setupUI();
    void loadData();
};

#endif // CONFIGUREDEVICEDIALOG_H