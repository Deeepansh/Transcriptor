#pragma once

#include <QWidget>
#include <QMediaPlayer>
#include <QFileDialog>
#include <QStandardPaths>
#include <QTime>
#include <QSettings>
#include<QBuffer>
#include<QMultimedia>
#include<QAudioFormat>
#include<QMediaMetaData>
#include"fftw3.h"
#include"playercontrols.h"
#include"qcustomplot.h"

class MediaPlayer : public QMediaPlayer
{
    Q_OBJECT
public:
    explicit MediaPlayer(QWidget *parent = nullptr);

    QTime elapsedTime();
    QTime durationTime();
    void setPositionToTime(const QTime& time);
    QString getMediaFileName();
    QString getPositionInfo();

public slots:
    void open();
    void seek(int seconds);
    void togglePlayback();

signals:
    void message(QString text, int timeout = 5000);
    //-------------------------------
    void sendBuffer(QBuffer& audio);
    void sendDuration(double total_duration);

private:
    static QTime getTimeFromPosition(const qint64& position);
    QString m_mediaFileName;
    QUrl m_fileUrl;

    //------------------------------------
    QBuffer audioBuffer;
    QMediaPlayer *p;


};
