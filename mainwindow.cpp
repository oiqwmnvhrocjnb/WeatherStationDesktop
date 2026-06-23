#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "controlSum.h"
#include <QDateTime>
#include <QRandomGenerator>
#include <QApplication>
#include <QStyle>
#include <QStyleFactory>
#include <QColorDialog>
#include <QListWidget>
#include <QMetaType>
#include <QDockWidget>
#include <QMenu>
#include <cstddef>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setDockOptions(QMainWindow::AnimatedDocks | QMainWindow::AllowTabbedDocks | QMainWindow::AllowNestedDocks);

    QDockWidget *dockValues = new QDockWidget("Текущие показания", this);
    dockValues->setObjectName("dockValues");
    dockValues->setWidget(ui->currentValuesGroup);
    this->addDockWidget(Qt::LeftDockWidgetArea, dockValues);

    QDockWidget *dockControls = new QDockWidget("Отображение графиков", this);
    dockControls->setObjectName("dockControls");
    dockControls->setWidget(ui->graphControlGroup);
    this->addDockWidget(Qt::LeftDockWidgetArea, dockControls);

    QDockWidget *dockSettings = new QDockWidget("Настройки графика", this);
    dockSettings->setObjectName("dockSettings");
    dockSettings->setWidget(ui->graphSettingsGroup);
    this->addDockWidget(Qt::LeftDockWidgetArea, dockSettings);

    QDockWidget *dockNotif = new QDockWidget("Уведомления", this);
    dockNotif->setObjectName("dockNotif");
    dockNotif->setWidget(ui->notificationsGroup);
    this->addDockWidget(Qt::BottomDockWidgetArea, dockNotif);

    QDockWidget *dockGlobal = new QDockWidget("Глобальные настройки", this);
    dockGlobal->setObjectName("dockGlobal");
    dockGlobal->setWidget(ui->settingsTab);
    this->addDockWidget(Qt::RightDockWidgetArea, dockGlobal);

    QDockWidget *dockAbout = new QDockWidget("О программе", this);
    dockAbout->setObjectName("dockAbout");
    dockAbout->setWidget(ui->aboutTab);
    this->addDockWidget(Qt::RightDockWidgetArea, dockAbout);

    ui->graphsWidget->setParent(this);
    this->setCentralWidget(ui->graphsWidget);

    auto showContextMenu = [this](const QPoint &pos, QWidget *widget) {
        QMenu *menu = this->createPopupMenu();
        if (menu) {
            menu->exec(widget->mapToGlobal(pos));
            delete menu;
        }
    };

    this->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, [this, showContextMenu](const QPoint &pos) {
        showContextMenu(pos, this);
    });

    ui->graphsWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->graphsWidget, &QWidget::customContextMenuRequested, this, [this, showContextMenu](const QPoint &pos) {
        showContextMenu(pos, ui->graphsWidget);
    });

    QList<QCustomPlot*> allPlots = {
        ui->plotTemp, ui->plotPressure, ui->plotHumidity,
        ui->plotGas, ui->plotWater, ui->plotLight, ui->plotNoise
    };

    for (QCustomPlot *plot : allPlots) {
        plot->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(plot, &QWidget::customContextMenuRequested, this, [this, showContextMenu, plot](const QPoint &pos) {
            showContextMenu(pos, plot);
        });
    }

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation));
    trayIcon->show();

    qRegisterMetaType<DataPacket>("DataPacket");

    connect(this, &MainWindow::dataReceived, this, &MainWindow::processIncomingData, Qt::QueuedConnection);

    QColor colTemp(255, 50, 50);
    QColor colPressure(50, 200, 50);
    QColor colHumidity(50, 50, 255);
    QColor colGas(50, 50, 50);
    QColor colWater(50, 200, 255);
    QColor colLight(255, 165, 0);
    QColor colNoise(200, 50, 50);

    initGraphSettings(ui->plotTemp, "Температура", -10.0, 40.0, 0.0, 30.0);
    initGraphSettings(ui->plotPressure, "Давление", 100000.0, 102000.0, 100500.0, 101500.0);
    initGraphSettings(ui->plotHumidity, "Влажность", 0.0, 100.0, 30.0, 70.0);
    initGraphSettings(ui->plotGas, "CO2", 300.0, 1000.0, 0.0, 800.0);
    initGraphSettings(ui->plotWater, "Уровень воды", 0.0, 100.0, 10.0, 90.0);
    initGraphSettings(ui->plotLight, "Освещенность", -0.2, 1.2, 0.0, 1.0);
    initGraphSettings(ui->plotNoise, "Уровень шума", -0.2, 1.2, 0.0, 1.0);

    setupGraph(ui->plotTemp, "Температура, °C", colTemp);
    setupGraph(ui->plotPressure, "Давление, Па", colPressure);
    setupGraph(ui->plotHumidity, "Влажность, %", colHumidity);
    setupGraph(ui->plotGas, "CO2, ppm", colGas);
    setupGraph(ui->plotWater, "Уровень воды, %", colWater);
    setupGraph(ui->plotLight, "Освещенность (0/1)", colLight);
    setupGraph(ui->plotNoise, "Шум (0/1)", colNoise);

    bindGraphControls(ui->plotTemp, ui->chkShowTemp, ui->btnColorTemp, ui->comboStyleTemp, ui->btnSetTemp, colTemp);
    bindGraphControls(ui->plotPressure, ui->chkShowPressure, ui->btnColorPressure, ui->comboStylePressure, ui->btnSetPressure, colPressure);
    bindGraphControls(ui->plotHumidity, ui->chkShowHumidity, ui->btnColorHumidity, ui->comboStyleHumidity, ui->btnSetHumidity, colHumidity);
    bindGraphControls(ui->plotGas, ui->chkShowGas, ui->btnColorGas, ui->comboStyleGas, ui->btnSetGas, colGas);
    bindGraphControls(ui->plotWater, ui->chkShowWater, ui->btnColorWater, ui->comboStyleWater, ui->btnSetWater, colWater);
    bindGraphControls(ui->plotLight, ui->chkShowLight, ui->btnColorLight, ui->comboStyleLight, ui->btnSetLight, colLight);
    bindGraphControls(ui->plotNoise, ui->chkShowNoise, ui->btnColorNoise, ui->comboStyleNoise, ui->btnSetNoise, colNoise);

    connect(ui->chkAutoScale, &QCheckBox::toggled, this, &MainWindow::onSettingsChanged);
    connect(ui->spinMinY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onSettingsChanged);
    connect(ui->spinMaxY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onSettingsChanged);
    connect(ui->chkMinAlert, &QCheckBox::toggled, this, &MainWindow::onSettingsChanged);
    connect(ui->spinMinAlert, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onSettingsChanged);
    connect(ui->chkMaxAlert, &QCheckBox::toggled, this, &MainWindow::onSettingsChanged);
    connect(ui->spinMaxAlert, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onSettingsChanged);

    connect(ui->themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        applyTheme(index == 1);
    });
    connect(ui->timeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
        graphWindowSeconds = value;
    });

    applyTheme(false);

    loadSettings();

    startTime = QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000.0;

    dataTimer = new QTimer(this);
    connect(dataTimer, &QTimer::timeout, this, &MainWindow::generateRandomData);

    dataTimer->start(QRandomGenerator::global()->bounded(900, 1101));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    QMainWindow::closeEvent(event);
}


void MainWindow::saveSettings()
{
    QSettings settings(QCoreApplication::applicationDirPath() + "/settings.ini", QSettings::IniFormat);

    settings.setValue("geometry", saveGeometry());

    settings.setValue("windowState", saveState());

    settings.setValue("theme", ui->themeCombo->currentIndex());
    settings.setValue("timeWindow", ui->timeSpin->value());

    saveGraphState(settings, "Temp", ui->plotTemp, ui->chkShowTemp, ui->comboStyleTemp);
    saveGraphState(settings, "Pressure", ui->plotPressure, ui->chkShowPressure, ui->comboStylePressure);
    saveGraphState(settings, "Humidity", ui->plotHumidity, ui->chkShowHumidity, ui->comboStyleHumidity);
    saveGraphState(settings, "Gas", ui->plotGas, ui->chkShowGas, ui->comboStyleGas);
    saveGraphState(settings, "Water", ui->plotWater, ui->chkShowWater, ui->comboStyleWater);
    saveGraphState(settings, "Light", ui->plotLight, ui->chkShowLight, ui->comboStyleLight);
    saveGraphState(settings, "Noise", ui->plotNoise, ui->chkShowNoise, ui->comboStyleNoise);
}

void MainWindow::loadSettings()
{
    QSettings settings(QCoreApplication::applicationDirPath() + "/settings.ini", QSettings::IniFormat);

    if (settings.contains("geometry")) restoreGeometry(settings.value("geometry").toByteArray());

    if (settings.contains("windowState")) restoreState(settings.value("windowState").toByteArray());

    if (settings.contains("theme")) ui->themeCombo->setCurrentIndex(settings.value("theme").toInt());
    if (settings.contains("timeWindow")) ui->timeSpin->setValue(settings.value("timeWindow").toInt());

    loadGraphState(settings, "Temp", ui->plotTemp, ui->chkShowTemp, ui->btnColorTemp, ui->comboStyleTemp);
    loadGraphState(settings, "Pressure", ui->plotPressure, ui->chkShowPressure, ui->btnColorPressure, ui->comboStylePressure);
    loadGraphState(settings, "Humidity", ui->plotHumidity, ui->chkShowHumidity, ui->btnColorHumidity, ui->comboStyleHumidity);
    loadGraphState(settings, "Gas", ui->plotGas, ui->chkShowGas, ui->btnColorGas, ui->comboStyleGas);
    loadGraphState(settings, "Water", ui->plotWater, ui->chkShowWater, ui->btnColorWater, ui->comboStyleWater);
    loadGraphState(settings, "Light", ui->plotLight, ui->chkShowLight, ui->btnColorLight, ui->comboStyleLight);
    loadGraphState(settings, "Noise", ui->plotNoise, ui->chkShowNoise, ui->btnColorNoise, ui->comboStyleNoise);
}

void MainWindow::saveGraphState(QSettings &set, const QString &key, QCustomPlot *plot, QCheckBox *chk, QComboBox *comboStyle)
{
    GraphSettings &gSet = graphSettings[plot];

    set.beginGroup(key);
    set.setValue("visible", chk->isChecked());
    set.setValue("color", plot->graph(0)->pen().color().name());
    set.setValue("lineStyle", comboStyle->currentIndex());

    set.setValue("autoScale", gSet.autoScale);
    set.setValue("minY", gSet.minY);
    set.setValue("maxY", gSet.maxY);

    set.setValue("minAlertEnabled", gSet.minAlertEnabled);
    set.setValue("minAlert", gSet.minAlert);

    set.setValue("maxAlertEnabled", gSet.maxAlertEnabled);
    set.setValue("maxAlert", gSet.maxAlert);
    set.endGroup();
}

void MainWindow::loadGraphState(QSettings &set, const QString &key, QCustomPlot *plot, QCheckBox *chk, QPushButton *btnColor, QComboBox *comboStyle)
{
    set.beginGroup(key);

    if (set.contains("visible")) {
        bool isVisible = set.value("visible").toBool();
        chk->setChecked(isVisible);
        plot->setVisible(isVisible);
    }

    if (set.contains("color")) {
        QColor color(set.value("color").toString());

        QString style = QString("background-color: %1; border: 1px solid gray; border-radius: 3px;").arg(color.name());
        btnColor->setStyleSheet(style);

        QPen pen = plot->graph(0)->pen();
        pen.setColor(color);
        plot->graph(0)->setPen(pen);

        pen = plot->graph(1)->pen();
        pen.setColor(color);
        plot->graph(1)->setPen(pen);
    }

    if (set.contains("lineStyle")) {
        comboStyle->setCurrentIndex(set.value("lineStyle").toInt());
    }

    GraphSettings &gSet = graphSettings[plot];
    if (set.contains("autoScale")) gSet.autoScale = set.value("autoScale").toBool();
    if (set.contains("minY")) gSet.minY = set.value("minY").toDouble();
    if (set.contains("maxY")) gSet.maxY = set.value("maxY").toDouble();
    if (set.contains("minAlertEnabled")) gSet.minAlertEnabled = set.value("minAlertEnabled").toBool();
    if (set.contains("minAlert")) gSet.minAlert = set.value("minAlert").toDouble();
    if (set.contains("maxAlertEnabled")) gSet.maxAlertEnabled = set.value("maxAlertEnabled").toBool();
    if (set.contains("maxAlert")) gSet.maxAlert = set.value("maxAlert").toDouble();

    set.endGroup();

    if (currentSettingsGraph == plot) {
        openSettings(plot);
    }
}

void MainWindow::initGraphSettings(QCustomPlot* plot, const QString& name, double defMinY, double defMaxY, double defMinAlert, double defMaxAlert)
{
    GraphSettings set;
    set.name = name;
    set.autoScale = true;
    set.minY = defMinY;
    set.maxY = defMaxY;
    set.minAlertEnabled = false;
    set.minAlert = defMinAlert;
    set.maxAlertEnabled = false;
    set.maxAlert = defMaxAlert;

    graphSettings[plot] = set;
}

void MainWindow::bindGraphControls(QCustomPlot *plot, QCheckBox *chk, QPushButton *btnColor, QComboBox *comboStyle, QPushButton *btnSettings, QColor initialColor)
{
    connect(chk, &QCheckBox::toggled, plot, &QCustomPlot::setVisible);

    QString style = QString("background-color: %1; border: 1px solid gray; border-radius: 3px;").arg(initialColor.name());
    btnColor->setStyleSheet(style);

    connect(btnColor, &QPushButton::clicked, this, [this, plot, btnColor]() {
        QColor currentColor = plot->graph(0)->pen().color();
        QColor newColor = QColorDialog::getColor(currentColor, this, "Выберите цвет графика");

        if (newColor.isValid()) {
            QString newStyle = QString("background-color: %1; border: 1px solid gray; border-radius: 3px;").arg(newColor.name());
            btnColor->setStyleSheet(newStyle);

            QPen pen = plot->graph(0)->pen();
            pen.setColor(newColor);
            plot->graph(0)->setPen(pen);

            pen = plot->graph(1)->pen();
            pen.setColor(newColor);
            plot->graph(1)->setPen(pen);

            plot->replot();
        }
    });

    connect(comboStyle, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [plot](int index) {
        QPen pen = plot->graph(0)->pen();
        switch (index) {
            case 0: pen.setStyle(Qt::SolidLine); break;
            case 1: pen.setStyle(Qt::DashLine); break;
            case 2: pen.setStyle(Qt::DotLine); break;
            case 3: pen.setStyle(Qt::DashDotLine); break;
            default: pen.setStyle(Qt::SolidLine); break;
        }
        plot->graph(0)->setPen(pen);
        plot->replot();
    });

    connect(btnSettings, &QPushButton::clicked, this, [this, plot]() {
        openSettings(plot);
    });
}

void MainWindow::openSettings(QCustomPlot *plot)
{
    currentSettingsGraph = plot;
    if (!plot) return;

    GraphSettings &set = graphSettings[plot];

    ui->graphSettingsGroup->setEnabled(true);
    ui->graphSettingsGroup->setTitle(QString("Настройки: %1").arg(set.name));

    if (QDockWidget *dock = this->findChild<QDockWidget*>("dockSettings")) {
        dock->raise();
    }

    ui->chkAutoScale->blockSignals(true);
    ui->spinMinY->blockSignals(true);
    ui->spinMaxY->blockSignals(true);
    ui->chkMinAlert->blockSignals(true);
    ui->spinMinAlert->blockSignals(true);
    ui->chkMaxAlert->blockSignals(true);
    ui->spinMaxAlert->blockSignals(true);

    ui->chkAutoScale->setChecked(set.autoScale);
    ui->spinMinY->setValue(set.minY);
    ui->spinMaxY->setValue(set.maxY);
    ui->chkMinAlert->setChecked(set.minAlertEnabled);
    ui->spinMinAlert->setValue(set.minAlert);
    ui->chkMaxAlert->setChecked(set.maxAlertEnabled);
    ui->spinMaxAlert->setValue(set.maxAlert);

    ui->spinMinY->setEnabled(!set.autoScale);
    ui->spinMaxY->setEnabled(!set.autoScale);
    ui->spinMinAlert->setEnabled(set.minAlertEnabled);
    ui->spinMaxAlert->setEnabled(set.maxAlertEnabled);

    ui->chkAutoScale->blockSignals(false);
    ui->spinMinY->blockSignals(false);
    ui->spinMaxY->blockSignals(false);
    ui->chkMinAlert->blockSignals(false);
    ui->spinMinAlert->blockSignals(false);
    ui->chkMaxAlert->blockSignals(false);
    ui->spinMaxAlert->blockSignals(false);
}

void MainWindow::onSettingsChanged()
{
    if (!currentSettingsGraph) return;

    GraphSettings &set = graphSettings[currentSettingsGraph];
    set.autoScale = ui->chkAutoScale->isChecked();
    set.minY = ui->spinMinY->value();
    set.maxY = ui->spinMaxY->value();

    set.minAlertEnabled = ui->chkMinAlert->isChecked();
    set.minAlert = ui->spinMinAlert->value();

    set.maxAlertEnabled = ui->chkMaxAlert->isChecked();
    set.maxAlert = ui->spinMaxAlert->value();

    ui->spinMinY->setEnabled(!set.autoScale);
    ui->spinMaxY->setEnabled(!set.autoScale);
    ui->spinMinAlert->setEnabled(set.minAlertEnabled);
    ui->spinMaxAlert->setEnabled(set.maxAlertEnabled);

    currentSettingsGraph->replot();
}

void MainWindow::addNotification(const QString &message, bool isError)
{
    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss");
    QListWidgetItem *item = new QListWidgetItem(QString("[%1] %2").arg(timeStr, message));

    item->setForeground(isError ? Qt::red : QColor(0, 128, 0));

    ui->listNotifications->insertItem(0, item);

    if (ui->listNotifications->count() > 100) {
        delete ui->listNotifications->takeItem(100);
    }

    if (trayIcon && trayIcon->isVisible()) {
        QSystemTrayIcon::MessageIcon iconType = isError ? QSystemTrayIcon::Warning : QSystemTrayIcon::Information;

        trayIcon->showMessage("Метеостанция", message, iconType, 5000);
    }
}

void MainWindow::applyTheme(bool isDark)
{
    qApp->setStyle(QStyleFactory::create("Fusion"));

    QPalette p = qApp->palette();
    if (isDark) {
        p.setColor(QPalette::Window, QColor(45, 45, 45));
        p.setColor(QPalette::WindowText, Qt::white);
        p.setColor(QPalette::Base, QColor(30, 30, 30));
        p.setColor(QPalette::AlternateBase, QColor(45, 45, 45));
        p.setColor(QPalette::ToolTipBase, Qt::white);
        p.setColor(QPalette::ToolTipText, Qt::white);
        p.setColor(QPalette::Text, Qt::white);
        p.setColor(QPalette::Button, QColor(53, 53, 53));
        p.setColor(QPalette::ButtonText, Qt::white);
        p.setColor(QPalette::BrightText, Qt::red);
        p.setColor(QPalette::Link, QColor(42, 130, 218));
        p.setColor(QPalette::Highlight, QColor(42, 130, 218));
        p.setColor(QPalette::HighlightedText, Qt::black);
    } else {
        p = qApp->style()->standardPalette();
    }
    qApp->setPalette(p);


    QList<QCustomPlot*> plots = {
        ui->plotTemp, ui->plotPressure, ui->plotHumidity,
        ui->plotGas, ui->plotWater, ui->plotLight, ui->plotNoise
    };

    for (QCustomPlot* plot : plots) {
        if (isDark) {
            plot->setBackground(QBrush(QColor(45, 45, 45)));
            plot->xAxis->setTickLabelColor(Qt::white);
            plot->xAxis->setLabelColor(Qt::white);
            plot->xAxis->setBasePen(QPen(Qt::white));
            plot->xAxis->setTickPen(QPen(Qt::white));
            plot->xAxis->setSubTickPen(QPen(Qt::white));
            plot->yAxis->setTickLabelColor(Qt::white);
            plot->yAxis->setLabelColor(Qt::white);
            plot->yAxis->setBasePen(QPen(Qt::white));
            plot->yAxis->setTickPen(QPen(Qt::white));
            plot->yAxis->setSubTickPen(QPen(Qt::white));
        } else {
            plot->setBackground(QBrush(Qt::white));
            plot->xAxis->setTickLabelColor(Qt::black);
            plot->xAxis->setLabelColor(Qt::black);
            plot->xAxis->setBasePen(QPen(Qt::black));
            plot->xAxis->setTickPen(QPen(Qt::black));
            plot->xAxis->setSubTickPen(QPen(Qt::black));
            plot->yAxis->setTickLabelColor(Qt::black);
            plot->yAxis->setLabelColor(Qt::black);
            plot->yAxis->setBasePen(QPen(Qt::black));
            plot->yAxis->setTickPen(QPen(Qt::black));
            plot->yAxis->setSubTickPen(QPen(Qt::black));
        }
        plot->replot();
    }
}

void MainWindow::setupGraph(QCustomPlot *plot, const QString &yAxisLabel, QColor color)
{
    plot->addGraph();
    QPen pen;
    pen.setColor(color);
    pen.setWidth(2);
    pen.setStyle(Qt::SolidLine);
    plot->graph(0)->setPen(pen);

    plot->addGraph();
    QPen avgPen;
    avgPen.setColor(color);
    avgPen.setWidth(1);
    avgPen.setStyle(Qt::DashLine);
    plot->graph(1)->setPen(avgPen);
    plot->graph(1)->setName("Среднее");

    plot->addGraph();
    QPen minPen(Qt::red, 1, Qt::DotLine);
    plot->graph(2)->setPen(minPen);
    plot->graph(2)->setName("Мин. порог");
    plot->graph(2)->setVisible(false);

    plot->addGraph();
    QPen maxPen(Qt::red, 1, Qt::DotLine);
    plot->graph(3)->setPen(maxPen);
    plot->graph(3)->setName("Макс. порог");
    plot->graph(3)->setVisible(false);

    QSharedPointer<QCPAxisTickerDateTime> timeTicker(new QCPAxisTickerDateTime);
    timeTicker->setDateTimeFormat("hh:mm:ss");
    plot->xAxis->setTicker(timeTicker);

    plot->xAxis->setLabel("Время");
    plot->yAxis->setLabel(yAxisLabel);
}

void MainWindow::updatePlot(QCustomPlot *plot, QVector<double> &timeData, QVector<double> &valueData, double newValue, double currentTime)
{
    GraphSettings &settings = graphSettings[plot];

    if (settings.minAlertEnabled) {
        if (newValue < settings.minAlert && !settings.minAlertActive) {
            settings.minAlertActive = true;
            addNotification(QString("⚠️ %1: Значение упало ниже %2! (Текущее: %3)")
                            .arg(settings.name).arg(settings.minAlert).arg(newValue), true);
        } else if (newValue >= settings.minAlert && settings.minAlertActive) {
            settings.minAlertActive = false;
            addNotification(QString("✅ %1: Значение нормализовалось (> %2)")
                            .arg(settings.name).arg(settings.minAlert), false);
        }
    } else {
        settings.minAlertActive = false;
    }

    if (settings.maxAlertEnabled) {
        if (newValue > settings.maxAlert && !settings.maxAlertActive) {
            settings.maxAlertActive = true;
            addNotification(QString("⚠️ %1: Превышен предел %2! (Текущее: %3)")
                            .arg(settings.name).arg(settings.maxAlert).arg(newValue), true);
        } else if (newValue <= settings.maxAlert && settings.maxAlertActive) {
            settings.maxAlertActive = false;
            addNotification(QString("✅ %1: Значение нормализовалось (< %2)")
                            .arg(settings.name).arg(settings.maxAlert), false);
        }
    } else {
        settings.maxAlertActive = false;
    }

    timeData.append(currentTime);
    valueData.append(newValue);

    while (!timeData.isEmpty() && timeData.first() < currentTime - graphWindowSeconds) {
        timeData.removeFirst();
        valueData.removeFirst();
    }

    double sum = 0;
    for (double v : valueData) sum += v;
    double avg = valueData.isEmpty() ? 0 : sum / valueData.size();

    plot->graph(0)->setData(timeData, valueData);

    double minX = currentTime - graphWindowSeconds;
    double maxX = currentTime + 1;
    plot->xAxis->setRange(minX, maxX);

    if (!timeData.isEmpty()) {
        QVector<double> ax = {minX, maxX};

        plot->graph(1)->setData(ax, {avg, avg});

        if (settings.minAlertEnabled) {
            plot->graph(2)->setData(ax, {settings.minAlert, settings.minAlert});
            plot->graph(2)->setVisible(true);
        } else {
            plot->graph(2)->setVisible(false);
        }

        if (settings.maxAlertEnabled) {
            plot->graph(3)->setData(ax, {settings.maxAlert, settings.maxAlert});
            plot->graph(3)->setVisible(true);
        } else {
            plot->graph(3)->setVisible(false);
        }
    }

    if (settings.autoScale) {
        plot->yAxis->rescale();
        QCPRange yRange = plot->yAxis->range();
        double padding = yRange.size() * 0.1;
        if (padding == 0.0) padding = 0.5;
        plot->yAxis->setRange(yRange.lower - padding, yRange.upper + padding);
    } else {

        plot->yAxis->setRange(qMin(settings.minY, settings.maxY), qMax(settings.minY, settings.maxY));
    }

    plot->replot();
}

void MainWindow::generateRandomData()
{
    int nextInterval = QRandomGenerator::global()->bounded(900, 1101);
    dataTimer->start(nextInterval);

    DataPacket packet;

    packet.syncMark = 0xDADA;

    packet.temperature = 20.0f + (QRandomGenerator::global()->bounded(100) / 10.0f) - 5.0f;
    packet.pressure = 101300.0f + (QRandomGenerator::global()->bounded(1000) - 500);
    packet.humidity = 40.0f + (QRandomGenerator::global()->bounded(200) / 10.0f);
    packet.gasConcentration = 400.0f + QRandomGenerator::global()->bounded(100);
    packet.waterLevel = QRandomGenerator::global()->bounded(101);
    packet.isLight = QRandomGenerator::global()->bounded(2) == 1;
    packet.isNoisy = QRandomGenerator::global()->bounded(10) > 8;

    unsigned char dataLength = static_cast<unsigned char>(offsetof(DataPacket, controlSum));
    packet.controlSum = countCrc(reinterpret_cast<unsigned char*>(&packet), dataLength);

    emit dataReceived(packet);
}

void MainWindow::processIncomingData(const DataPacket &packet)
{
    unsigned char dataLength = static_cast<unsigned char>(offsetof(DataPacket, controlSum));

    unsigned short calculatedCrc = countCrc(reinterpret_cast<unsigned char*>(const_cast<DataPacket*>(&packet)), dataLength);

    if (calculatedCrc != packet.controlSum)
    {
        return;
    }

    ui->lblTemp->setText(QString("Температура: %1 °C").arg(packet.temperature, 0, 'f', 1));
    ui->lblPressure->setText(QString("Давление: %1 Па").arg(packet.pressure, 0, 'f', 0));
    ui->lblHumidity->setText(QString("Влажность: %1 %").arg(packet.humidity, 0, 'f', 1));
    ui->lblGas->setText(QString("CO2: %1 ppm").arg(packet.gasConcentration, 0, 'f', 0));
    ui->lblWater->setText(QString("Вода: %1 %").arg(packet.waterLevel));

    ui->lblLight->setText(packet.isLight ? "Свет: СВЕТЛО" : "Свет: ТЕМНО");
    ui->lblLight->setStyleSheet(packet.isLight ? "color: orange;" : "color: gray;");

    ui->lblNoise->setText(packet.isNoisy ? "Шум: ШУМНО" : "Шум: ТИХО");
    ui->lblNoise->setStyleSheet(packet.isNoisy ? "color: red;" : "color: green;");

    QString controlSumString = QString("%1").arg(packet.controlSum, 4, 16, QChar('0')).toUpper();

    ui->lblControlSum->setText(QString("Контрольная сумма: 0x%1").arg(controlSumString));

    double currentTime = QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000.0;

    updatePlot(ui->plotTemp, timeTemp, tempMap, packet.temperature, currentTime);
    updatePlot(ui->plotPressure, timePressure, pressureMap, packet.pressure, currentTime);
    updatePlot(ui->plotHumidity, timeHumidity, humidityMap, packet.humidity, currentTime);
    updatePlot(ui->plotGas, timeGas, gasMap, packet.gasConcentration, currentTime);
    updatePlot(ui->plotWater, timeWater, waterMap, packet.waterLevel, currentTime);

    updatePlot(ui->plotLight, timeLight, lightMap, packet.isLight ? 1.0 : 0.0, currentTime);
    updatePlot(ui->plotNoise, timeNoise, noiseMap, packet.isNoisy ? 1.0 : 0.0, currentTime);
}
