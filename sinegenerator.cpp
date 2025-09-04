#include "sinegenerator.h"

#include <algorithm>
#include <cstring>
#include <cmath>
#include <QMutexLocker>

SineGenerator::SineGenerator(QObject *parent) : QIODevice(parent) {}

void SineGenerator::configure(int sampleRate, int channels, QAudioFormat::SampleFormat fmt)
{
    QMutexLocker lock(&m_mutex);
    m_sampleRate = sampleRate;
    m_channels   = std::max(1, channels);
    m_sampleFmt  = fmt;
    rebuildBuffer();
}

void SineGenerator::setFrequency(int freqHz)
{
    QMutexLocker lock(&m_mutex);
    m_freqHz = std::max(1, freqHz);
    rebuildBuffer();
}

void SineGenerator::setChannelMode(ChannelMode mode)
{
    QMutexLocker lock(&m_mutex);
    m_mode = mode;
    rebuildBuffer();
}

void SineGenerator::start() { open(QIODevice::ReadOnly); }
void SineGenerator::stop()  { close(); }

void SineGenerator::rebuildBuffer()
{
    // Integer number of samples/period for seamless looping
    const int periodSamples = std::max(1, m_sampleRate / std::max(1, m_freqHz));
    m_posBytes = 0;

    const bool leftOn  = (m_mode == ChannelMode::LeftOnly  || m_mode == ChannelMode::Both);
    const bool rightOn = (m_mode == ChannelMode::RightOnly || m_mode == ChannelMode::Both);

    if (m_sampleFmt == QAudioFormat::Float)
    {
        const int bytesPerSample = 4;
        m_buffer.resize(periodSamples * m_channels * bytesPerSample);
        float *dst = reinterpret_cast<float*>(m_buffer.data());

        for (int i = 0; i < periodSamples; ++i)
        {
            const float s = std::sin(2.0f * float(M_PI) * float(i) / float(periodSamples));

            // Interleave
            for (int ch = 0; ch < m_channels; ++ch)
            {
                float out = 0.0f;
                if (ch == 0 && leftOn)  out = s;
                if (ch == 1 && rightOn) out = s;
                *dst++ = out;
            }
        }
    }
    else
    {   // Int16 fallback
        const int bytesPerSample = 2;
        m_buffer.resize(periodSamples * m_channels * bytesPerSample);
        qint16 *dst = reinterpret_cast<qint16*>(m_buffer.data());

        for (int i = 0; i < periodSamples; ++i)
        {
            const qint16 s = qint16(32767.0 * std::sin(2.0 * M_PI * double(i) / double(periodSamples)));

            for (int ch = 0; ch < m_channels; ++ch)
            {
                qint16 out = 0;
                if (ch == 0 && leftOn)  out = s;
                if (ch == 1 && rightOn) out = s;
                *dst++ = out;
            }
        }
    }
}

qint64 SineGenerator::readData(char *data, qint64 maxlen)
{
    QMutexLocker lock(&m_mutex);
    if (m_buffer.isEmpty() || maxlen <= 0) return 0;

    qint64 total = 0;
    const int N = m_buffer.size();

    while (total < maxlen)
    {
        const int chunk = std::min<int>(N - m_posBytes, int(maxlen - total));
        std::memcpy(data + total, m_buffer.constData() + m_posBytes, size_t(chunk));
        total += chunk;
        m_posBytes += chunk;
        if (m_posBytes >= N) m_posBytes = 0; // loop
    }
    return total;
}
