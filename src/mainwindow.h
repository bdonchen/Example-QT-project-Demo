#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
class QComboBox;
class QSlider;
QT_END_NAMESPACE

class DataGenerator;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onConnectClicked();
    void onStartClicked();
    void onStopClicked();

    void handleWaveformSample(double value);
    void handleImageUpdated(const QImage &image);
    void handleLogMessage(const QString &message);

private:
    void setupUi();
    void setupChart();
    void setupWorker();

    // UI
    QWidget *m_centralWidget;
    QLabel *m_statusLabel;
    QPushButton *m_connectButton;
    QPushButton *m_startButton;
    QPushButton *m_stopButton;
    QComboBox *m_modeComboBox;
    QSlider *m_gainSlider;
    QLabel *m_gainValueLabel;
    QChartView *m_chartView;
    QLineSeries *m_series;
    QValueAxis *m_axisX;
    QValueAxis *m_axisY;
    QLabel *m_imageLabel;
    QLabel *m_logLabel;

    // Data / threading
    QThread *m_workerThread;
    DataGenerator *m_generator;
    double m_timeX;
};

#endif // MAINWINDOW_H
