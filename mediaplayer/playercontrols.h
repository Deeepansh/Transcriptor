#pragma once

#include <QMediaPlayer>
#include <QWidget>
#include <QAbstractButton>
#include <QAbstractSlider>
#include <QComboBox>
#include <QLabel>

//---------------------------------------
#include"qcustomplot.h"
#include<QVector>
#include"fftw3.h"

//---------------------------------------
class PlayerControls : public QWidget
{
    Q_OBJECT

public:
    explicit PlayerControls(QWidget *parent = nullptr);
    ~PlayerControls();

    QMediaPlayer::State state() const;
    int volume() const;
    bool isMuted() const;
    qreal playbackRate() const;

public slots:
    void setState(QMediaPlayer::State state);
    void setVolume(int volume);
    void setMuted(bool muted);
    void setPlaybackRate(float rate);

    //----------------------
    void processBuffer(QBuffer& audioBuffer);
    void getDuration(double total_time);

signals:
    void play();
    void pause();
    void stop();
    void seekForward();
    void seekBackward();
    void changeVolume(int volume);
    void changeMuting(bool muting);
    void changeRate(qreal rate);


private slots:
    void playClicked();
    void muteClicked();
    void updateRate();
    void onVolumeSliderValueChanged();

    //-----------------------------------
    void processAudioIn();
    //----------------------------------

private:
    QMediaPlayer::State m_playerState = QMediaPlayer::StoppedState;
    bool m_playerMuted = false;
    QAbstractButton *m_playButton = nullptr;
    QAbstractButton *m_stopButton = nullptr;
    QAbstractButton *m_seekForwardButton = nullptr;
    QAbstractButton *m_seekBackwardButton = nullptr;
    QAbstractButton *m_muteButton = nullptr;
    QAbstractSlider *m_volumeSlider = nullptr;
    QComboBox *m_rateBox = nullptr;

    //-------------------------------------------
    QCustomPlot *waveWidget;
    void samplesUpdated();

    QBuffer mInputBuffer;
    double tot_duration;

    //qint64 mDuration;
    QVector<double> mFftIndices;

    fftw_plan mFftPlan;
    double *mFftIn;
    double *mFftOut;

    QVector<double> mSamples;
    QVector<double> mIndices;

    double total_dur;
    //---------------------------------------------

};
