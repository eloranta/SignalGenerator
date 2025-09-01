#pragma once
#include <QIODevice>
#include <QAudioFormat>
#include <QByteArray>
#include <QMutex>

class SineGenerator : public QIODevice {
    Q_OBJECT
public:
    explicit SineGenerator(QObject *parent = nullptr);

    // Must be called before start(): defines audio frame format
    void configure(int sampleRate, int channels, QAudioFormat::SampleFormat fmt);

    // Change frequency on the fly (Hz). Rebuilds the internal loop buffer.
    void setFrequency(int freqHz);

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
    void rebuildBuffer(); // rebuild one-period buffer for current freq/format

    QByteArray m_buffer;     // interleaved one-period buffer
    int        m_posBytes = 0;

    int m_sampleRate = 48000;
    int m_channels   = 1;
    QAudioFormat::SampleFormat m_sampleFmt = QAudioFormat::Float;
    int m_freqHz     = 1000;   // default tone

    QMutex m_mutex; // protects buffer rebuild / pos
};
