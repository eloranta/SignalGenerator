#include "mainwindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QComboBox>     // ✨ add
#include <QLabel>
#include <QGroupBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    setupAudio();
}

void MainWindow::setupUi() {
    auto *central = new QWidget(this);
    auto *root = new QVBoxLayout(central);

    auto *row1 = new QHBoxLayout;
    m_btnStart = new QPushButton("Start", central);
    m_btnStop  = new QPushButton("Stop", central);
    m_btnStop->setEnabled(false);
    row1->addWidget(m_btnStart);
    row1->addWidget(m_btnStop);
    row1->addStretch(1);

    auto *group = new QGroupBox("Signal Controls", central);
    auto *glyt  = new QVBoxLayout(group);

    // Frequency
    auto *rowFreq = new QHBoxLayout;
    auto *lblF = new QLabel("Frequency (Hz):", group);
    m_spinFreq = new QSpinBox(group);
    m_spinFreq->setRange(20, 20000);
    m_spinFreq->setValue(1000);
    m_spinFreq->setSingleStep(10);
    rowFreq->addWidget(lblF);
    rowFreq->addWidget(m_spinFreq, 1);

    // Volume
    auto *rowVol = new QHBoxLayout;
    auto *lblV = new QLabel("Volume (%):", group);
    m_sliderVol = new QSlider(Qt::Horizontal, group);
    m_sliderVol->setRange(0, 100);
    m_sliderVol->setValue(80);
    rowVol->addWidget(lblV);
    rowVol->addWidget(m_sliderVol, 1);

    // ✨ Channel selection
    auto *rowCh = new QHBoxLayout;
    auto *lblC = new QLabel("Output Channel:", group);
    m_comboChannel = new QComboBox(group);
    m_comboChannel->addItem("Left");
    m_comboChannel->addItem("Right");
    m_comboChannel->addItem("Both");
    m_comboChannel->setCurrentIndex(2); // Both by default
    rowCh->addWidget(lblC);
    rowCh->addWidget(m_comboChannel, 1);

    glyt->addLayout(rowFreq);
    glyt->addLayout(rowVol);
    glyt->addLayout(rowCh);  // ✨ add

    m_lblStatus = new QLabel("Ready.", central);

    root->addLayout(row1);
    root->addWidget(group);
    root->addWidget(m_lblStatus);

    setCentralWidget(central);
    setWindowTitle("Qt6 Signal Generator (48 kHz)");
    resize(520, 220);

    connect(m_btnStart, &QPushButton::clicked, this, &MainWindow::startAudio);
    connect(m_btnStop,  &QPushButton::clicked, this, &MainWindow::stopAudio);
    connect(m_spinFreq, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::freqChanged);
    connect(m_sliderVol, &QSlider::valueChanged, this, &MainWindow::volumeChanged);
    connect(m_comboChannel, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::channelModeChanged); // ✨ add
}

void MainWindow::setupAudio() {
    m_device = QMediaDevices::defaultAudioOutput();

    m_format.setSampleRate(48000);
    m_format.setChannelCount(2);                   // ✨ stereo
    m_format.setSampleFormat(QAudioFormat::Float); // 32-bit float

    if (!m_device.isFormatSupported(m_format)) {
        m_format.setSampleFormat(QAudioFormat::Int16);
        if (!m_device.isFormatSupported(m_format)) {
            m_format = m_device.preferredFormat();
        }
        // Ensure we still end up with 2 channels if possible
        if (m_format.channelCount() < 2)
            m_format.setChannelCount(2);
    }

    // Prepare generator with current format & freq
    m_gen.configure(m_format.sampleRate(),
                    m_format.channelCount(),
                    m_format.sampleFormat());
    m_gen.setFrequency(m_spinFreq->value());
    channelModeChanged(m_comboChannel->currentIndex()); // ✨ apply initial mode

    m_lblStatus->setText(
        QString("Device: %1 | %2 Hz, %3 ch, fmt=%4")
            .arg(m_device.description())
            .arg(m_format.sampleRate())
            .arg(m_format.channelCount())
            .arg(int(m_format.sampleFormat())));
}

void MainWindow::startAudio() {
    if (m_sink) return;
    m_sink = new QAudioSink(m_device, m_format, this);
    connect(m_sink, &QAudioSink::stateChanged, this, [this](QAudio::State s){
        m_lblStatus->setText(
            QString("State: %1, error=%2 | %3 Hz, %4 ch")
                .arg(int(s)).arg(int(m_sink->error()))
                .arg(m_format.sampleRate())
                .arg(m_format.channelCount()));
    });

    m_gen.start();
    m_sink->setVolume(m_sliderVol->value() / 100.0f);
    m_sink->start(&m_gen);

    m_btnStart->setEnabled(false);
    m_btnStop->setEnabled(true);
}

void MainWindow::stopAudio() {
    if (!m_sink) return;
    m_sink->stop();
    m_gen.stop();
    m_sink->deleteLater();
    m_sink = nullptr;

    m_btnStart->setEnabled(true);
    m_btnStop->setEnabled(false);
    m_lblStatus->setText("Stopped.");
}

void MainWindow::freqChanged(int hz) {
    m_gen.setFrequency(hz);
}

void MainWindow::volumeChanged(int volPercent) {
    if (m_sink) m_sink->setVolume(volPercent / 100.0f);
}

// ✨ New: map combo index to generator mode
void MainWindow::channelModeChanged(int idx) {
    switch (idx) {
    case 0: m_gen.setChannelMode(SineGenerator::ChannelMode::LeftOnly);  break;
    case 1: m_gen.setChannelMode(SineGenerator::ChannelMode::RightOnly); break;
    default: m_gen.setChannelMode(SineGenerator::ChannelMode::Both);     break;
    }
}
