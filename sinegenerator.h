#pragma once
#include <QIODevice>
#include <QAudioFormat>
#include <QByteArray>
#include <QMutex>

class SineGenerator : public QIODevice {
    Q_OBJECT
public:
    enum class ChannelMode { LeftOnly, RightOnly, Both }; // ✨

    explicit SineGenerator(QObject *parent = nullptr);

    void configure(int sampleRate, int channels, QAudioFormat::SampleFormat fmt);
    void setFrequency(int freqHz);

    // ✨ New: select output channel(s)
    void setChannelMode(ChannelMode mode);

    void start();
    void stop();

protected:
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char*, qint64) override { return 0; }
    bool isSequential() const override { return true; }
    qint64 bytesAvailable() const override {
        return m_buffer.size() + QIODevice::bytesAvailable();
    }

private:
    void rebuildBuffer(); // rebuild one-period buffer

    QByteArray m_buffer;     // interleaved one-period buffer
    int        m_posBytes = 0;

    int m_sampleRate = 48000;
    int m_channels   = 2; // ✨ stereo default
    QAudioFormat::SampleFormat m_sampleFmt = QAudioFormat::Float;
    int m_freqHz     = 1000;

    ChannelMode m_mode = ChannelMode::Both; // ✨
    QMutex m_mutex;
};
