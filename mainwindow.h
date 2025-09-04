#pragma once
#include <QMainWindow>
#include <QAudioSink>
#include <QMediaDevices>
#include <QPointer>

class QPushButton;
class QSlider;
class QSpinBox;
class QLabel;
class QComboBox;

#include "sinegenerator.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;
private slots:
    void startAudio();
    void stopAudio();
    void frequencyChanged(int frequency);
    void volumeChanged(int percent);
    void channelModeChanged(int index);
private:
    void setupUi();
    void setupAudio();
// UI
    QPushButton *m_buttonStart = nullptr;
    QPushButton *m_buttonStop  = nullptr;
    QSpinBox    *m_spinFrequency = nullptr;
    QSlider     *m_sliderVolume = nullptr;
    QComboBox   *m_comboChannel = nullptr;
    QLabel      *m_labelStatus = nullptr;
// Audio
    QAudioDevice m_device;
    QAudioFormat m_format;
    QPointer<QAudioSink> m_sink;
    SineGenerator m_generator;
};
