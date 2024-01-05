#include "mediaplayer.h"

//-----------------------------

#include"playercontrols.h"
//-----------------------------
MediaPlayer::MediaPlayer(QWidget *parent)
    : QMediaPlayer(parent)
{
}

QTime MediaPlayer::elapsedTime()
{
    return getTimeFromPosition(position());
}

QTime MediaPlayer::durationTime()
{
    return getTimeFromPosition(duration());
}

void MediaPlayer::setPositionToTime(const QTime& time)
{
    if (time.isNull())
        return;
    qint64 position = 3600000*time.hour() + 60000*time.minute() + 1000*time.second() + time.msec();
    setPosition(position);
}

QString MediaPlayer::getMediaFileName()
{
    return m_mediaFileName;
}

QString MediaPlayer::getPositionInfo()
{
    QString format = "mm:ss:zzz";
    if (durationTime().hour() != 0)
        format = "hh:mm:ss:zzz";

    return elapsedTime().toString(format) + " / " + durationTime().toString(format);
}

void MediaPlayer::open()
{
    QFileDialog fileDialog;
    fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
    fileDialog.setWindowTitle(tr("Open Media"));
    QStringList supportedMimeTypes = QMediaPlayer::supportedMimeTypes();
    if (!supportedMimeTypes.isEmpty())
        fileDialog.setMimeTypeFilters(supportedMimeTypes);
    if(QSettings().value("mediaDir").toString()=="")
        fileDialog.setDirectory(QStandardPaths::standardLocations(QStandardPaths::MoviesLocation).value(0, QDir::homePath()));
    else
        fileDialog.setDirectory(QSettings().value("mediaDir").toString());
    if (fileDialog.exec() == QDialog::Accepted) {
        QUrl *fileUrl = new QUrl(fileDialog.selectedUrls().constFirst());
        QFile MediaFile(fileUrl->toLocalFile());
        if(!MediaFile.open(QIODevice::ReadOnly))
        {
            qInfo()<<"Not open for readonly\n";
        }
        QFileInfo filedir(MediaFile);
        QString dirInString=filedir.dir().path();
        QSettings().setValue("mediaDir",dirInString);
        m_mediaFileName = fileUrl->fileName();
        setMedia(*fileUrl);
        emit message("Opened file " + fileUrl->fileName());
        //----------------------------------------
        QByteArray audioData = MediaFile.readAll();

        audioBuffer.open(QIODevice::ReadWrite);

        audioBuffer.write(audioData);
        audioBuffer.close();
        //audioBuffer.open(QIODevice::ReadOnly);
        play();

        QString filepath;
        filepath = filedir.absoluteFilePath();

        p = new QMediaPlayer(this);
        p->setMedia(*fileUrl);

        connect(p, &QMediaPlayer::durationChanged, [this]() {
            double tot_duration = p->duration();
            //qInfo() << "Total duration updated: " << tot_duration;
            emit sendDuration(tot_duration);
            emit sendBuffer(audioBuffer);
        });
        //----------------------------------------------
    }
}

void MediaPlayer::seek(int seconds)
{
    if (elapsedTime().addSecs(seconds) > durationTime())
        setPosition(duration());
    else if (elapsedTime().addSecs(seconds).isNull())
        setPosition(0);
    else
        setPositionToTime(elapsedTime().addSecs(seconds));
}

QTime MediaPlayer::getTimeFromPosition(const qint64& position)
{
    auto milliseconds = position % 1000;
    auto seconds = (position/1000) % 60;
    auto minutes = (position/60000) % 60;
    auto hours = (position/3600000) % 24;

    return QTime(hours, minutes, seconds, milliseconds);
}

void MediaPlayer::togglePlayback()
{
    if (state() == MediaPlayer::PausedState || state() == MediaPlayer::StoppedState)
        play();
    else if (state() == MediaPlayer::PlayingState)
        pause();
}

//============================================================
/*QBuffer MediaPlayer::getBuffer()
{
    return audioBuffer;
}
*/
