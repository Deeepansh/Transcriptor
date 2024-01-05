#pragma once

//-------------------------
//#include"qcustomplot.h"
//-------------------------
#include <QMainWindow>
#include "mediaplayer/mediaplayer.h"
#include "editor/texteditor.h"
#include<QThread>
#include<QtConcurrent/QtConcurrent>
#include "./transcriptgenerator.h"
#include"audiowaveform.h"

QT_BEGIN_NAMESPACE
    namespace Ui { class Tool; }
QT_END_NAMESPACE

    class Tool final : public QMainWindow
{
    Q_OBJECT

        public:
                 explicit Tool(QWidget *parent = nullptr);
    ~Tool() final;

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void handleMediaPlayerError();
    void createKeyboardShortcutGuide();
    void changeFont();
    void changeFontSize(int change);
    void transliterationSelected(QAction* action);

    void on_Upload_and_generate_Transcript_triggered();
    //    void on_editor_openTranscript_triggered();

    void on_btn_translate_clicked();

    void on_editor_openTranscript_triggered();



    void on_add_video_clicked();

    void on_open_transcript_clicked();

    void on_new_transcript_clicked();

    void on_save_transcript_clicked();

    void on_save_as_transcript_clicked();

    void on_load_a_custom_dict_clicked();

    void on_get_PDF_clicked();

    void on_decreseFontSize_clicked();

    void on_increaseFontSize_clicked();

    void on_toggleWordEditor_clicked();

    void on_keyboard_shortcuts_clicked();

    void on_fontComboBox_currentFontChanged(const QFont &f);

private:
    void setFontForElements();
    void setTransliterationLangCodes();

    MediaPlayer *player = nullptr;
    Ui::Tool *ui;
    QFont font;
    QMap<QString, QString> m_transliterationLang;

};

