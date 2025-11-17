#ifndef DATAGENERATOR_H
#define DATAGENERATOR_H

#include <QObject>
#include <QTimer>
#include <QImage>

class DataGenerator : public QObject
{
    Q_OBJECT
public:
    explicit DataGenerator(QObject *parent = nullptr);

public slots:
    void start();
    void stop();
    void setGain(double gain);

signals:
    void waveformSampleReady(double value);
    void imageUpdated(const QImage &image);
    void logMessage(const QString &message);

private slots:
    void onTimeout();

private:
    QTimer *m_timer;
    QImage m_image;
    int m_currentLine;
    double m_phase;
    double m_gain;
};

#endif // DATAGENERATOR_H
