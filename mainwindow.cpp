#include "mainwindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QGroupBox>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setupUi();
    setupAudio();
}

void MainWindow::setupUi()
{
    auto *central = new QWidget(this);
    auto *root = new QVBoxLayout(central);

    auto *row1 = new QHBoxLayout;
    m_buttonStart = new QPushButton("Start", central);
    m_buttonStop  = new QPushButton("Stop", central);
    m_buttonStop->setEnabled(false);
    row1->addWidget(m_buttonStart);
    row1->addWidget(m_buttonStop);
    row1->addStretch(1);

    auto *group = new QGroupBox("Signal Controls", central);
    auto *layout  = new QVBoxLayout(group);

    // Frequency
    auto *rowFrequency = new QHBoxLayout;
    auto *labelFrequency = new QLabel("Frequency (Hz):", group);
    m_spinFrequency = new QSpinBox(group);
    m_spinFrequency->setRange(20, 20000);
    m_spinFrequency->setValue(1000);
    m_spinFrequency->setSingleStep(10);
    rowFrequency->addWidget(labelFrequency);
    rowFrequency->addWidget(m_spinFrequency, 1);

    // Volume
    auto *rowVolume = new QHBoxLayout;
    auto *labelVolume = new QLabel("Volume (%):", group);
    m_sliderVolume = new QSlider(Qt::Horizontal, group);
    m_sliderVolume->setRange(0, 100);
    m_sliderVolume->setValue(80);
    rowVolume->addWidget(labelVolume);
    rowVolume->addWidget(m_sliderVolume, 1);

    // Channel selection
    auto *rowChannel = new QHBoxLayout;
    auto *labelChannel = new QLabel("Output Channel:", group);
    m_comboChannel = new QComboBox(group);
    m_comboChannel->addItem("Left");
    m_comboChannel->addItem("Right");
    m_comboChannel->addItem("Both");
    m_comboChannel->setCurrentIndex(2); // Both by default
    rowChannel->addWidget(labelChannel);
    rowChannel->addWidget(m_comboChannel, 1);

    layout->addLayout(rowFrequency);
    layout->addLayout(rowVolume);
    layout->addLayout(rowChannel);

    m_labelStatus = new QLabel("Ready.", central);

    root->addLayout(row1);
    root->addWidget(group);
    root->addWidget(m_labelStatus);

    setCentralWidget(central);
    setWindowTitle("Qt6 Signal Generator (48 kHz)");
    resize(520, 220);

    connect(m_buttonStart, &QPushButton::clicked, this, &MainWindow::startAudio);
    connect(m_buttonStop,  &QPushButton::clicked, this, &MainWindow::stopAudio);
    connect(m_spinFrequency, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::frequencyChanged);
    connect(m_sliderVolume, &QSlider::valueChanged, this, &MainWindow::volumeChanged);
    connect(m_comboChannel, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::channelModeChanged);
}

void MainWindow::setupAudio()
{
    m_device = QMediaDevices::defaultAudioOutput();

    m_format.setSampleRate(48000);
    m_format.setChannelCount(2);                   // stereo
    m_format.setSampleFormat(QAudioFormat::Float); // 32-bit float

    if (!m_device.isFormatSupported(m_format))
    {
        m_format.setSampleFormat(QAudioFormat::Int16);
        if (!m_device.isFormatSupported(m_format))
        {
            m_format = m_device.preferredFormat();
        }
        // Ensure we still end up with 2 channels if possible
        if (m_format.channelCount() < 2)
            m_format.setChannelCount(2);
    }

    // Prepare generator with current format & freq
    m_generator.configure(m_format.sampleRate(), m_format.channelCount(), m_format.sampleFormat());
    m_generator.setFrequency(m_spinFrequency->value());
    channelModeChanged(m_comboChannel->currentIndex()); //apply initial mode

    m_labelStatus->setText(
        QString("Device: %1 | %2 Hz, %3 ch, fmt=%4")
            .arg(m_device.description())
            .arg(m_format.sampleRate())
            .arg(m_format.channelCount())
            .arg(int(m_format.sampleFormat())));
}

void MainWindow::startAudio()
{
    if (m_sink) return;
    m_sink = new QAudioSink(m_device, m_format, this);
    connect(m_sink, &QAudioSink::stateChanged, this, [this](QAudio::State s)
    {
        m_labelStatus->setText(
            QString("State: %1, error=%2 | %3 Hz, %4 ch")
                .arg(int(s)).arg(int(m_sink->error()))
                .arg(m_format.sampleRate())
                .arg(m_format.channelCount()));
    });

    m_generator.start();
    m_sink->setVolume(m_sliderVolume->value() / 100.0f);
    m_sink->start(&m_generator);

    m_buttonStart->setEnabled(false);
    m_buttonStop->setEnabled(true);
}

void MainWindow::stopAudio()
{
    if (!m_sink) return;
    m_sink->stop();
    m_generator.stop();
    m_sink->deleteLater();
    m_sink = nullptr;

    m_buttonStart->setEnabled(true);
    m_buttonStop->setEnabled(false);
    m_labelStatus->setText("Stopped.");
}

void MainWindow::frequencyChanged(int frequency)
{
    m_generator.setFrequency(frequency);
}

void MainWindow::volumeChanged(int percent) {
    if (m_sink) m_sink->setVolume(percent / 100.0f);
}

void MainWindow::channelModeChanged(int index)
{
    switch (index)
    {
        case 0:  m_generator.setChannelMode(SineGenerator::ChannelMode::LeftOnly);  break;
        case 1:  m_generator.setChannelMode(SineGenerator::ChannelMode::RightOnly); break;
        default: m_generator.setChannelMode(SineGenerator::ChannelMode::Both);     break;
    }
}
