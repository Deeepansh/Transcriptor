#include "playercontrols.h"

#include <QBoxLayout>
#include <QSlider>
#include <QStyle>
#include <QToolButton>
#include <QComboBox>
#include <QAudio>

#include"mediaplayer.h"

//----------------------------
#define AUDIBLE_RANGE_START 20
#define AUDIBLE_RANGE_END   20000 /* very optimistic */
#define NUM_SAMPLES 96000  //96000
#define SAMPLE_FREQ 48000  //48000
//----------------------------

PlayerControls::PlayerControls(QWidget *parent)
    : QWidget(parent)
{
    m_playButton = new QToolButton(this);
    m_playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));

    connect(m_playButton, &QAbstractButton::clicked, this, &PlayerControls::playClicked);

    m_stopButton = new QToolButton(this);
    m_stopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    m_stopButton->setEnabled(false);

    connect(m_stopButton, &QAbstractButton::clicked, this, &PlayerControls::stop);

    m_seekForwardButton = new QToolButton(this);
    m_seekForwardButton->setIcon(style()->standardIcon(QStyle::SP_MediaSeekForward));

    connect(m_seekForwardButton, &QAbstractButton::clicked, this, &PlayerControls::seekForward);

    m_seekBackwardButton = new QToolButton(this);
    m_seekBackwardButton->setIcon(style()->standardIcon(QStyle::SP_MediaSeekBackward));

    connect(m_seekBackwardButton, &QAbstractButton::clicked, this, &PlayerControls::seekBackward);

    m_muteButton = new QToolButton(this);
    m_muteButton->setIcon(style()->standardIcon(QStyle::SP_MediaVolume));

    connect(m_muteButton, &QAbstractButton::clicked, this, &PlayerControls::muteClicked);

    m_volumeSlider = new QSlider(Qt::Horizontal, this);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(100);

    connect(m_volumeSlider, &QSlider::valueChanged, this, &PlayerControls::onVolumeSliderValueChanged);

    m_rateBox = new QComboBox(this);

    m_rateBox->addItem("0.5x", QVariant(0.5));
    m_rateBox->addItem("0.7x", QVariant(0.7));
    m_rateBox->addItem("1.0x", QVariant(1.0));
    m_rateBox->addItem("1.5x", QVariant(1.5));
    m_rateBox->addItem("2.0x", QVariant(2.0));
    m_rateBox->setCurrentIndex(2);

    //---------------------------------------------------------
    waveWidget = new QCustomPlot(this);

    waveWidget->yAxis->setRange(-1.0, 1.0);
    waveWidget->xAxis->setRange(0, 8);
    waveWidget->clearGraphs();
    waveWidget->addGraph();
    waveWidget->yAxis->setVisible(false);
    waveWidget->xAxis->setVisible(true);


    waveWidget->graph()->setPen(QPen(Qt::black));

    waveWidget->setVisible(false);
    waveWidget->graph()->setVisible(true);

    waveWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    waveWidget->setMinimumSize(0, 100);

    connect(m_rateBox, QOverload<int>::of(&QComboBox::activated), this, &PlayerControls::updateRate);

    QBoxLayout *layout = new QHBoxLayout;

    QBoxLayout *layout2 = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_stopButton);
    layout->addWidget(m_seekBackwardButton);
    layout->addWidget(m_playButton);
    layout->addWidget(m_seekForwardButton);
    layout->addWidget(m_muteButton);
    layout->addWidget(m_volumeSlider);
    layout->addWidget(m_rateBox);

    //layout2->addLayout(layout);
    //layout2->addWidget(waveWidget);
    //-----------------

    //setLayout(layout);

    setLayout(layout);
    //--------------------------------------------
    for (int i = 0; i < NUM_SAMPLES; i ++) {
        mIndices.append((double)i);
        mSamples.append(0);
    }

    double freqStep = (double)SAMPLE_FREQ / (double)NUM_SAMPLES;
    double f = AUDIBLE_RANGE_START;
    while (f < AUDIBLE_RANGE_END) {
        mFftIndices.append(f);
        f += freqStep;
    }

    /* Set up FFT plan */
    mFftIn  = fftw_alloc_real(NUM_SAMPLES);
    mFftOut = fftw_alloc_real(NUM_SAMPLES);
    mFftPlan = fftw_plan_r2r_1d(NUM_SAMPLES, mFftIn, mFftOut, FFTW_R2HC,FFTW_ESTIMATE);
    //-------------------------------------------
}

PlayerControls::~PlayerControls()
{
    fftw_free(mFftIn);
    fftw_free(mFftOut);
    fftw_destroy_plan(mFftPlan);
}

QMediaPlayer::State PlayerControls::state() const
{
    return m_playerState;
}

void PlayerControls::setState(QMediaPlayer::State state)
{
    if (state != m_playerState) {
        m_playerState = state;

        switch (state) {
        case QMediaPlayer::StoppedState:
            m_stopButton->setEnabled(false);
            m_playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
            break;
        case QMediaPlayer::PlayingState:
            m_stopButton->setEnabled(true);
            m_playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
            break;
        case QMediaPlayer::PausedState:
            m_stopButton->setEnabled(true);
            m_playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
            break;
        }
    }
}

int PlayerControls::volume() const
{
    qreal linearVolume =  QAudio::convertVolume(m_volumeSlider->value() / qreal(100),
                                               QAudio::LogarithmicVolumeScale,
                                               QAudio::LinearVolumeScale);

    return qRound(linearVolume * 100);
}

void PlayerControls::setVolume(int volume)
{
    qreal logarithmicVolume = QAudio::convertVolume(volume / qreal(100),
                                                    QAudio::LinearVolumeScale,
                                                    QAudio::LogarithmicVolumeScale);

    m_volumeSlider->setValue(qRound(logarithmicVolume * 100));
}

bool PlayerControls::isMuted() const
{
    return m_playerMuted;
}

void PlayerControls::setMuted(bool muted)
{
    if (muted != m_playerMuted) {
        m_playerMuted = muted;

        m_muteButton->setIcon(style()->standardIcon(muted
                                                        ? QStyle::SP_MediaVolumeMuted
                                                        : QStyle::SP_MediaVolume));
    }
}

void PlayerControls::playClicked()
{
    switch (m_playerState) {
    case QMediaPlayer::StoppedState:
    case QMediaPlayer::PausedState:
        emit play();
        break;
    case QMediaPlayer::PlayingState:
        emit pause();
        break;
    }
}

void PlayerControls::muteClicked()
{
    emit changeMuting(!m_playerMuted);
}

qreal PlayerControls::playbackRate() const
{
    return m_rateBox->itemData(m_rateBox->currentIndex()).toDouble();
}

void PlayerControls::setPlaybackRate(float rate)
{
    for (int i = 0; i < m_rateBox->count(); ++i) {
        if (qFuzzyCompare(rate, float(m_rateBox->itemData(i).toDouble()))) {
            m_rateBox->setCurrentIndex(i);
            return;
        }
    }

    m_rateBox->addItem(QString("%1x").arg(rate), QVariant(rate));
    m_rateBox->setCurrentIndex(m_rateBox->count() - 1);
}

void PlayerControls::updateRate()
{
    emit changeRate(playbackRate());
}

void PlayerControls::onVolumeSliderValueChanged()
{
    emit changeVolume(volume());
}


//-----------------------------------------------
void PlayerControls::processBuffer(QBuffer& audioBuffer)
{
    mInputBuffer.open(QIODevice::ReadWrite);
    audioBuffer.open(QIODevice::ReadOnly);

    while (!audioBuffer.atEnd()) {
        QByteArray chunk = audioBuffer.read(1024); // Read 1024 bytes at a time
        mInputBuffer.write(chunk);
    }
    mInputBuffer.close();
    audioBuffer.close();
    processAudioIn();
}

void PlayerControls::processAudioIn()
{
    mInputBuffer.open(QIODevice::ReadOnly);
    mInputBuffer.seek(0);
    QByteArray ba = mInputBuffer.readAll();

    int num_samples = ba.length() / 2;
    int b_pos = 0;
    for (int i = 0; i < num_samples; i ++) {
        int16_t s;
        s = ba.at(b_pos++);
        s |= ba.at(b_pos++) << 8;
        if (s != 0) {
            mSamples.append((double)s / 32768.0);
        } else {
            mSamples.append(0);
        }
    }
    mInputBuffer.buffer().clear();
    mInputBuffer.seek(0);
    mInputBuffer.close();
    samplesUpdated();
}
/*
void PlayerControls::samplesUpdated()
{
    int n = mSamples.length();
    if (n > 96000) mSamples = mSamples.mid(n - NUM_SAMPLES,-1);

    memcpy(mFftIn, mSamples.data(), NUM_SAMPLES*sizeof(double));

    fftw_execute(mFftPlan);

    QVector<double> fftVec;

    for (int i = (NUM_SAMPLES/SAMPLE_FREQ)*AUDIBLE_RANGE_START;
         i < (NUM_SAMPLES/SAMPLE_FREQ)*AUDIBLE_RANGE_END;
         i ++) {
        fftVec.append(abs(mFftOut[i]));
    }

    //---------------------------------
    for (int i = 0; i < mSamples.length(); ++i) {
        mSamples[i] *= 1.5; // Normalize to range [-1.0, 1.0]
    }
    //---------------------------------
    waveWidget->graph(0)->setData(mIndices, mSamples);
    waveWidget->xAxis->rescale();
    waveWidget->replot();

}
*/

void PlayerControls::getDuration(double total_time)
{
    total_dur = total_time;
}
void PlayerControls::samplesUpdated()
{

    int n = mSamples.length();
    if (n > 96000) mSamples = mSamples.mid(n - NUM_SAMPLES, -1);

    memcpy(mFftIn, mSamples.data(), NUM_SAMPLES * sizeof(double));

    fftw_execute(mFftPlan);

    QVector<double> fftVec;

    double totalDuration = total_dur/1000.0;
    //qInfo()<<"total duration is: "<<totalDuration;

    // Create a vector for time values from 0 to totalDuration
    QVector<double> timeValues;
    double timeStep = totalDuration/(NUM_SAMPLES - 1);

    //qInfo()<<"total duration is: "<<totalDuration<<"\n";

    for (int i = 0; i < NUM_SAMPLES; i++) {
        timeValues.append(i * timeStep);
    }

    /*for (int i = (NUM_SAMPLES / SAMPLE_FREQ) * AUDIBLE_RANGE_START;
         i < (NUM_SAMPLES / SAMPLE_FREQ) * AUDIBLE_RANGE_END;
         i++) {
        fftVec.append(abs(mFftOut[i]));
    }*/

    for (int i = 0; i < NUM_SAMPLES; i++) {
        fftVec.append(abs(mFftOut[i]));
    }

    //---------------------------------
    for (int i = 0; i < mSamples.length(); ++i) {
        mSamples[i] *= 1.5; // Normalize to range [-1.0, 1.0]
    }
    //---------------------------------

    waveWidget->graph(0)->setData(timeValues, mSamples);
    waveWidget->xAxis->rescale();
    waveWidget->replot();
}
