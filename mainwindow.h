#pragma once

#include <QMainWindow>
#include <QTimer>
#include <QVector>
#include <QCheckBox>
#include <QPushButton>
#include <QComboBox>
#include <QMap>
#include <QSystemTrayIcon>
#include <QSettings>
#include <QCloseEvent>
#include "qcustomplot.h"
#include "packet.h"

namespace Ui {
class MainWindow;
}

struct GraphSettings {
    QString name;

    bool autoScale = true;
    double minY = 0.0;
    double maxY = 100.0;

    bool minAlertEnabled = false;
    double minAlert = 0.0;
    bool minAlertActive = false;

    bool maxAlertEnabled = false;
    double maxAlert = 100.0;
    bool maxAlertActive = false;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

signals:
    void dataReceived(const DataPacket &packet);

private slots:
    void generateRandomData();
    void processIncomingData(const DataPacket &packet);
    void openSettings(QCustomPlot *plot);
    void onSettingsChanged();

private:
    Ui::MainWindow *ui;
    QSystemTrayIcon *trayIcon;

    void setupGraph(QCustomPlot *plot, const QString &yAxisLabel, QColor color);
    void updatePlot(QCustomPlot *plot, QVector<double> &timeData, QVector<double> &valueData, double newValue, double currentTime);
    void applyTheme(bool isDark);

    void initGraphSettings(QCustomPlot* plot, const QString& name, double defMinY, double defMaxY, double defMinAlert, double defMaxAlert);

    void addNotification(const QString &message, bool isError);

    void bindGraphControls(QCustomPlot *plot, QCheckBox *chk, QPushButton *btnColor, QComboBox *comboStyle, QPushButton *btnSettings, QColor initialColor);

    void saveSettings();
    void loadSettings();
    void saveGraphState(QSettings &set, const QString &key, QCustomPlot *plot, QCheckBox *chk, QComboBox *comboStyle);
    void loadGraphState(QSettings &set, const QString &key, QCustomPlot *plot, QCheckBox *chk, QPushButton *btnColor, QComboBox *comboStyle);

    QTimer *dataTimer;
    double startTime;

    QMap<QCustomPlot*, GraphSettings> graphSettings;
    QCustomPlot* currentSettingsGraph = nullptr;

    QVector<double> timeTemp;
    QVector<double> tempMap;

    QVector<double> timePressure;
    QVector<double> pressureMap;

    QVector<double> timeHumidity;
    QVector<double> humidityMap;

    QVector<double> timeGas;
    QVector<double> gasMap;

    QVector<double> timeWater;
    QVector<double> waterMap;

    QVector<double> timeLight;
    QVector<double> lightMap;

    QVector<double> timeNoise;
    QVector<double> noiseMap;

    int graphWindowSeconds = 60;
};
