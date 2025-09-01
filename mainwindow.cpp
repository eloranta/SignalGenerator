#include "mainwindow.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
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

    auto *rowFreq = new QHBoxLayout;
    auto *lblF = new QLabel("Frequency (Hz):", group);
    m_spinFreq = new QSpinBox(group);
    m_spinFreq->setRange(20, 20000);
    m_spinFreq->setValue(1000);
    m_spinFreq->setSingleStep(10);
    rowFreq->addWidget(lblF);
    rowFreq->addWidget(m_spinFreq, 1);

    auto *rowVol = new QHBoxLayout;
    auto *lblV = new QLabel("Volume (%):", group);
    m_sliderVol = new QSlider(Qt::Horizontal, group);
    m_sliderVol->setRange(0, 100);
    m_sliderVol->setValue(80);
    rowVol->addWidget(lblV);
    rowVol->addWidget(m_sliderVol, 1);

    glyt->addLayout(rowFreq);
    glyt->addLayout(rowVol);

    m_lblStatus = new QLabel("Ready.", central);

    root->addLayout(row1);
    root->addWidget(group);
    root->addWidget(m_lblStatus);

    setCentralWidget(central);
    setWindowTitle("Qt6 Signal Generator (48 kHz)");
    resize(480, 200);

    connect(m_btnStart, &QPushButton::clicked, this, &MainWindow::startAudio);
    connect(m_btnStop,  &QPushButton::clicked, this, &MainWindow::stopAudio);
    connect(m_spinFreq, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::freqChanged);
    connect(m_sliderVol, &QSlider::valueChanged, this, &MainWindow::volumeChanged);
}

void MainWindow::setupAudio() {
    // Pick default OUTPUT device and 48 kHz mono float (fallback if needed)
    m_device = QMediaDevices::defaultAudioOutput();

    m_format.setSampleRate(48000);
    m_format.setChannelCount(1);                  // mono
    m_format.setSampleFormat(QAudioFormat::Float);// 32-bit float

    if (!m_device.isFormatSupported(m_format)) {
        // Try Int16 mono @ 48 kHz
        m_format.setSampleFormat(QAudioFormat::Int16);
        if (!m_device.isFormatSupported(m_format)) {
            // fallback to preferred format to guarantee playback
            m_format = m_device.preferredFormat();
        }
    }

    // Prepare generator with current format & freq
    m_gen.configure(m_format.sampleRate(),
                    m_format.channelCount(),
                    m_format.sampleFormat());
    m_gen.setFrequency(m_spinFreq->value());

    m_lblStatus->setText(
        QString("Device: %1 | %2 Hz, %3 ch, fmt=%4")
            .arg(m_device.description())
            .arg(m_format.sampleRate())
            .arg(m_format.channelCount())
            .arg(int(m_format.sampleFormat())));
}

void MainWindow::startAudio() {
    if (m_sink) return; // already running
    m_sink = new QAudioSink(m_device, m_format, this);
    connect(m_sink, &QAudioSink::stateChanged, this, [this](QAudio::State s){
        m_lblStatus->setText(
            QString("State: %1, error=%2 | %3 Hz, %4 ch")
                .arg(int(s)).arg(int(m_sink->error()))
                .arg(m_format.sampleRate())
                .arg(m_format.channelCount()));
    });

    m_gen.start(); // open QIODevice for reading
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
