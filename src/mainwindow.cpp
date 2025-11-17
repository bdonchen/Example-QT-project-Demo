#include "mainwindow.h"
#include "datagenerator.h"

#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QSlider>
#include <QImage>
#include <QPixmap>
#include <QDebug>
#include <QMetaObject>
#include <QtCharts/QChart>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_centralWidget(new QWidget(this)),
      m_statusLabel(nullptr),
      m_connectButton(nullptr),
      m_startButton(nullptr),
      m_stopButton(nullptr),
      m_modeComboBox(nullptr),
      m_gainSlider(nullptr),
      m_gainValueLabel(nullptr),
      m_chartView(nullptr),
      m_series(nullptr),
      m_axisX(nullptr),
      m_axisY(nullptr),
      m_imageLabel(nullptr),
      m_logLabel(nullptr),
      m_workerThread(new QThread(this)),
      m_generator(nullptr),
      m_timeX(0.0)
{
    setupUi();
    setupChart();
    setupWorker();
}

MainWindow::~MainWindow()
{
    if (m_workerThread->isRunning()) {
        m_workerThread->quit();
        m_workerThread->wait();
    }
}

void MainWindow::setupUi()
{
    setWindowTitle("Auditory Imaging Controller (Qt Demo)");
    setCentralWidget(m_centralWidget);

    auto *mainLayout = new QHBoxLayout;
    m_centralWidget->setLayout(mainLayout);

    // ----- Left control panel -----
    auto *controlBox = new QGroupBox("Imaging Controls");
    auto *controlLayout = new QVBoxLayout;
    controlBox->setLayout(controlLayout);

    m_statusLabel = new QLabel("Status: Disconnected");
    controlLayout->addWidget(m_statusLabel);

    m_connectButton = new QPushButton("Connect to Device");
    controlLayout->addWidget(m_connectButton);

    m_modeComboBox = new QComboBox;
    m_modeComboBox->addItems({"Idle", "Live View", "Scan", "Record"});
    controlLayout->addWidget(new QLabel("Mode:"));
    controlLayout->addWidget(m_modeComboBox);

    m_gainSlider = new QSlider(Qt::Horizontal);
    m_gainSlider->setRange(0, 100);
    m_gainSlider->setValue(50);
    m_gainValueLabel = new QLabel("Gain: 50");

    auto *gainLayout = new QHBoxLayout;
    gainLayout->addWidget(new QLabel("Gain:"));
    gainLayout->addWidget(m_gainSlider);
    gainLayout->addWidget(m_gainValueLabel);
    controlLayout->addLayout(gainLayout);

    m_startButton = new QPushButton("Start Scan");
    m_stopButton = new QPushButton("Stop");
    m_stopButton->setEnabled(false);

    controlLayout->addWidget(m_startButton);
    controlLayout->addWidget(m_stopButton);

    m_logLabel = new QLabel("Log: Ready.");
    m_logLabel->setWordWrap(true);
    controlLayout->addWidget(m_logLabel);

    controlLayout->addStretch();

    // ----- Right visualization area -----
    auto *vizLayout = new QVBoxLayout;

    // Waveform
    auto *waveformBox = new QGroupBox("Waveform");
    auto *waveformLayout = new QVBoxLayout;
    waveformBox->setLayout(waveformLayout);

    m_chartView = new QChartView;
    m_chartView->setRenderHint(QPainter::Antialiasing);
    waveformLayout->addWidget(m_chartView);

    // Image
    auto *imageBox = new QGroupBox("Scan Image");
    auto *imageLayout = new QVBoxLayout;
    imageBox->setLayout(imageLayout);

    m_imageLabel = new QLabel;
    m_imageLabel->setMinimumSize(256, 256);
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setStyleSheet("background-color: black;");
    m_imageLabel->setScaledContents(true);
    imageLayout->addWidget(m_imageLabel);

    vizLayout->addWidget(waveformBox, 2);
    vizLayout->addWidget(imageBox, 3);

    mainLayout->addWidget(controlBox, 0);
    mainLayout->addLayout(vizLayout, 1);

    // --- Connections ---
    connect(m_connectButton, &QPushButton::clicked,
            this, &MainWindow::onConnectClicked);
    connect(m_startButton, &QPushButton::clicked,
            this, &MainWindow::onStartClicked);
    connect(m_stopButton, &QPushButton::clicked,
            this, &MainWindow::onStopClicked);

    connect(m_gainSlider, &QSlider::valueChanged, this, [this](int value) {
        const double gain = static_cast<double>(value) / 50.0; // 50 -> 1.0x
        m_gainValueLabel->setText(QString("Gain: %1 (%2x)").arg(value).arg(gain, 0, 'f', 2));

        if (m_generator) {
            QMetaObject::invokeMethod(m_generator, "setGain",
                                      Qt::QueuedConnection,
                                      Q_ARG(double, gain));
        }
    });

    // Initialize gain label to match the current slider position
    const int initialGainValue = m_gainSlider->value();
    const double initialGain = static_cast<double>(initialGainValue) / 50.0;
    m_gainValueLabel->setText(QString("Gain: %1 (%2x)")
                                  .arg(initialGainValue)
                                  .arg(initialGain, 0, 'f', 2));
}

void MainWindow::setupChart()
{
    auto *chart = new QChart;
    m_series = new QLineSeries;

    chart->addSeries(m_series);
    m_axisX = new QValueAxis;
    m_axisY = new QValueAxis;

    m_axisX->setRange(0, 5); // time window (seconds)
    m_axisX->setTitleText("Time (a.u.)");

    m_axisY->setRange(-2, 2);
    m_axisY->setTitleText("Amplitude");

    chart->addAxis(m_axisX, Qt::AlignBottom);
    chart->addAxis(m_axisY, Qt::AlignLeft);
    m_series->attachAxis(m_axisX);
    m_series->attachAxis(m_axisY);

    chart->setTitle("Simulated Auditory Signal");
    m_chartView->setChart(chart);
}

void MainWindow::setupWorker()
{
    m_generator = new DataGenerator;
    m_generator->moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::finished,
            m_generator, &QObject::deleteLater);

    // Connect control signals
    connect(this, &MainWindow::destroyed, m_generator, &DataGenerator::stop);

    // Data connections
    connect(m_generator, &DataGenerator::waveformSampleReady,
            this, &MainWindow::handleWaveformSample);
    connect(m_generator, &DataGenerator::imageUpdated,
            this, &MainWindow::handleImageUpdated);
    connect(m_generator, &DataGenerator::logMessage,
            this, &MainWindow::handleLogMessage);

    // Start thread
    m_workerThread->start();

    // Push initial gain value to the generator
    const double gain = static_cast<double>(m_gainSlider->value()) / 50.0;
    QMetaObject::invokeMethod(m_generator, "setGain",
                              Qt::QueuedConnection,
                              Q_ARG(double, gain));
}

void MainWindow::onConnectClicked()
{
    // In a real app, you'd open hardware / check drivers, etc.
    m_statusLabel->setText("Status: Connected (simulated)");
    m_logLabel->setText("Log: Device connection simulated.");
}

void MainWindow::onStartClicked()
{
    if (!m_workerThread->isRunning()) {
        m_workerThread->start();
    }

    QMetaObject::invokeMethod(m_generator, "start", Qt::QueuedConnection);

    m_startButton->setEnabled(false);
    m_stopButton->setEnabled(true);
    m_statusLabel->setText("Status: Scanning...");
}

void MainWindow::onStopClicked()
{
    QMetaObject::invokeMethod(m_generator, "stop", Qt::QueuedConnection);

    m_startButton->setEnabled(true);
    m_stopButton->setEnabled(false);
    m_statusLabel->setText("Status: Connected (idle)");
}

void MainWindow::handleWaveformSample(double value)
{
    // Append new point, increment time
    m_timeX += 0.03; // match DataGenerator timer ~30ms

    m_series->append(m_timeX, value);

    // Limit visible points
    const int maxPoints = 400;
    if (m_series->count() > maxPoints) {
        m_series->removePoints(0, m_series->count() - maxPoints);
    }

    // Keep X axis window sliding
    m_axisX->setRange(m_timeX - 4.0, m_timeX + 1.0);
}

void MainWindow::handleImageUpdated(const QImage &image)
{
    m_imageLabel->setPixmap(QPixmap::fromImage(image));
}

void MainWindow::handleLogMessage(const QString &message)
{
    m_logLabel->setText("Log: " + message);
    qDebug() << "LOG:" << message;
}
