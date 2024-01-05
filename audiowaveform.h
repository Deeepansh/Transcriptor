#ifndef AUDIOWAVEFORM_H
#define AUDIOWAVEFORM_H

#include <QWidget>

#include <QMediaPlayer>
#include <QAbstractButton>
#include <QAbstractSlider>
#include <QComboBox>
#include <QLabel>

//---------------------------------------
#include"mediaplayer/qcustomplot.h"
#include<QVector>
#include"mediaplayer/fftw3.h"

namespace Ui {
class AudioWaveForm;
}

class AudioWaveForm : public QWidget
{
    Q_OBJECT

public:
    explicit AudioWaveForm(QWidget *parent = nullptr);
    ~AudioWaveForm();

public slots:
    //----------------------
    void processBuffer(QBuffer& audioBuffer);
    void getDuration(double total_time);
    void onMousePress(QMouseEvent *event);
    void getTimeArray(QVector<QTime> timeArray);

private slots:
    void processAudioIn();

    void onAfterReplot();
    void onXAxisRangeChanged(const QCPRange& newRange);
    void onScrollBarValueChanged(int value);

    void onMouseMove(QMouseEvent *event);
    //void onSelectionChanged(bool selected);
private:
    Ui::AudioWaveForm *ui;
    //-------------------------------------------
    QCustomPlot *waveWidget;
    void samplesUpdated();
    void plotLines();
    void deselectLines(QVector<QCPItemLine*> &lines, int index, int num_of_lines);

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
    int flag1 = 1;
    int num_of_blocks;
    QVector<int> blocktime;
    QVector<QTime> endTime;

    QVector<QCPItemLine*> startLine;
    QVector<QCPItemLine*> endLine;
    QVector<QCPItemText*> utteranceNumbers;

    QScrollBar* horizontalScrollBar;
};

#endif // AUDIOWAVEFORM_H
