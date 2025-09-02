#pragma once
#include <QMainWindow>
#include <QAudioSink>
#include <QMediaDevices>
#include <QPointer>

class QPushButton;
class QSlider;
class QSpinBox;
class QLabel;
class QComboBox;   // ✨ add

#include "sinegenerator.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void startAudio();
    void stopAudio();
    void freqChanged(int hz);
    void volumeChanged(int volPercent);
    void channelModeChanged(int idx);   // ✨ add

private:
    void setupUi();
    void setupAudio();

    // UI
    QPushButton *m_btnStart = nullptr;
    QPushButton *m_btnStop  = nullptr;
    QSpinBox    *m_spinFreq = nullptr;
    QSlider     *m_sliderVol = nullptr;
    QComboBox   *m_comboChannel = nullptr; // ✨ add
    QLabel      *m_lblStatus = nullptr;

    // Audio
    QAudioDevice m_device;
    QAudioFormat m_format;
    QPointer<QAudioSink> m_sink;
    SineGenerator m_gen;
};
