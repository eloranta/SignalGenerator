#include "sinegenerator.h"
#include <QtMath>
#include <cstring>

SineGenerator::SineGenerator(QObject *parent) : QIODevice(parent) {}

void SineGenerator::configure(int sampleRate, int channels, QAudioFormat::SampleFormat fmt) {
    QMutexLocker lock(&m_mutex);
    m_sampleRate = sampleRate;
    m_channels   = qMax(1, channels);
    m_sampleFmt  = fmt;
    rebuildBuffer();
}

void SineGenerator::setFrequency(int freqHz) {
    QMutexLocker lock(&m_mutex);
    m_freqHz = qMax(1, freqHz);
    rebuildBuffer();
}

void SineGenerator::start() { open(QIODevice::ReadOnly); }
void SineGenerator::stop()  { close(); }

void SineGenerator::rebuildBuffer() {
    // Use an integer number of samples per period for seamless looping
    const int periodSamples = qMax(1, m_sampleRate / qMax(1, m_freqHz));
    m_posBytes = 0;

    if (m_sampleFmt == QAudioFormat::Float) {
        const int bps = 4;
        m_buffer.resize(periodSamples * m_channels * bps);
        float *dst = reinterpret_cast<float*>(m_buffer.data());
        for (int i = 0; i < periodSamples; ++i) {
            const float s = std::sin(2.0f * float(M_PI) * i / periodSamples);
            for (int ch = 0; ch < m_channels; ++ch) *dst++ = s; // -1..+1
        }
    } else { // Int16 fallback
        const int bps = 2;
        m_buffer.resize(periodSamples * m_channels * bps);
        qint16 *dst = reinterpret_cast<qint16*>(m_buffer.data());
        for (int i = 0; i < periodSamples; ++i) {
            const qint16 s = qint16(32767 * std::sin(2.0 * M_PI * i / periodSamples));
            for (int ch = 0; ch < m_channels; ++ch) *dst++ = s;
        }
    }
}

qint64 SineGenerator::readData(char *data, qint64 maxlen) {
    QMutexLocker lock(&m_mutex);
    if (m_buffer.isEmpty() || maxlen <= 0) return 0;

    qint64 total = 0;
    const int N = m_buffer.size();

    while (total < maxlen) {
        const int chunk = qMin<int>(N - m_posBytes, int(maxlen - total));
        std::memcpy(data + total, m_buffer.constData() + m_posBytes, size_t(chunk));
        total += chunk;
        m_posBytes += chunk;
        if (m_posBytes >= N) m_posBytes = 0; // loop
    }
    return total;
}
