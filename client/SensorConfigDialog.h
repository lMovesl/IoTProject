#ifndef SENSORCONFIGDIALOG_H
#define SENSORCONFIGDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QPushButton>

class SensorConfigDialog : public QDialog {
    Q_OBJECT
public:
    explicit SensorConfigDialog(int sensorId, QWidget* parent = nullptr);

private slots:
    void saveAndClose();
    void toggleSpinBoxes();

private:
    int m_sensorId;

    QCheckBox* m_chkMin;
    QCheckBox* m_chkMax;
    QCheckBox* m_chkRate;

    QDoubleSpinBox* m_spinMin;
    QDoubleSpinBox* m_spinMax;
    QDoubleSpinBox* m_spinRate;

    QPushButton* m_btnSave;
    QPushButton* m_btnCancel;
};

#endif // SENSORCONFIGDIALOG_H