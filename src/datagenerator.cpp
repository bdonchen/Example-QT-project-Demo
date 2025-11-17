#include "datagenerator.h"
#include <QtMath>
#include <QRandomGenerator>

DataGenerator::DataGenerator(QObject *parent)
    : QObject(parent),
      m_timer(new QTimer(this)),
      m_currentLine(0),
      m_phase(0.0),
      m_gain(1.0)
{
    // 256x256 grayscale image
    m_image = QImage(256, 256, QImage::Format_Grayscale8);
    m_image.fill(0);

    connect(m_timer, &QTimer::timeout, this, &DataGenerator::onTimeout);
}

void DataGenerator::start()
{
    if (!m_timer->isActive()) {
        m_phase = 0.0;
        m_currentLine = 0;
        m_image.fill(0);
        emit logMessage("Scan started.");
        m_timer->start(30); // 30 ms = ~33 Hz update rate
    }
}

void DataGenerator::stop()
{
    if (m_timer->isActive()) {
        m_timer->stop();
        emit logMessage("Scan stopped.");
    }
}

void DataGenerator::setGain(double gain)
{
    m_gain = gain;
}

void DataGenerator::onTimeout()
{
    // --- Simulate waveform ---
    // Base sine + random noise
    m_phase += 0.05;
    double sine = qSin(m_phase * 5.0);        // higher frequency
    double noise = (QRandomGenerator::global()->generateDouble() - 0.5) * 0.3;
    double value = (sine + noise) * m_gain;
    emit waveformSampleReady(value);

    // --- Simulate line-by-line "scan image" ---
    if (m_currentLine < m_image.height()) {
        for (int x = 0; x < m_image.width(); ++x) {
            double nx = static_cast<double>(x) / m_image.width();
            double ny = static_cast<double>(m_currentLine) / m_image.height();

            // Example: some interference pattern + noise
            double v = 0.5
                       + 0.25 * qSin(20 * nx + m_phase)
                       + 0.25 * qCos(15 * ny - m_phase)
                       + (QRandomGenerator::global()->generateDouble() - 0.5) * 0.1;

            int intensity = qBound(0, static_cast<int>(v * 255), 255);
            m_image.setPixel(x, m_currentLine, qRgb(intensity, intensity, intensity));
        }
        m_currentLine++;
    } else {
        // restart scan
        m_currentLine = 0;
        m_image.fill(0);
        emit logMessage("Scan complete. Restarting.");
    }

    emit imageUpdated(m_image);
}
