#include "tool.h"
#include "./ui_tool.h"
#include "editor/utilities/keyboardshortcutguide.h"
#include <QProgressBar>

//----------------------------------
#include<transcriptgenerator.h>
#include"./ui_audiowaveform.h"
//----------------------------------

#include <QFontDialog>
#include <QMessageBox>

#include <QStyle>
#include <QIcon>

Tool::Tool(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Tool)
{
    ui->setupUi(this);

    player = new MediaPlayer(this);
    player->setVideoOutput(ui->m_videoWidget);

    //ui->tabWidget->addTab(new QWidget(), "wave");


    QBoxLayout *wavetablayout = new QVBoxLayout;
    wavetablayout->addWidget(ui->waveWidget);
    ui->tab_3->setLayout(wavetablayout);
    //setLayout(wavetablayout);

    ui->splitter_tool->setCollapsible(0, false);
    ui->splitter_tool->setCollapsible(1, false);
    ui->splitter_tool->setSizes(QList<int>({static_cast<int>(0.15 * sizeHint().height()),
                                            static_cast<int>(0.85 * sizeHint().height())}));

    ui->splitter_editor->setCollapsible(0, false);
    ui->splitter_editor->setCollapsible(1, false);
    ui->splitter_editor->setSizes(QList<int>({static_cast<int>(0.7 * sizeHint().height()),
                                              static_cast<int>(0.3 * sizeHint().height())}));

    ui->m_wordEditor->setHidden(true);

    setTransliterationLangCodes();
    //Upload_and_generate_Transcript
    connect(ui->close, &QAction::triggered, this, &QMainWindow::close);

    //ToolBar icons
    //    connect(ui->add_video, &QAction::clicked, player, &MediaPlayer::open);


    // Connect Player Controls and Media Player
    connect(ui->player_open, &QAction::triggered, player, &MediaPlayer::open);
    connect(ui->player_togglePlay, &QAction::triggered, player, &MediaPlayer::togglePlayback);
    connect(ui->player_seekForward, &QAction::triggered, player, [&]() {player->seek(5);});
    connect(ui->player_seekBackward, &QAction::triggered, player, [&]() {player->seek(-5);});
    connect(ui->m_playerControls, &PlayerControls::play, player, &QMediaPlayer::play);
    connect(ui->m_playerControls, &PlayerControls::pause, player, &QMediaPlayer::pause);
    connect(ui->m_playerControls, &PlayerControls::stop, player, &QMediaPlayer::stop);
    connect(ui->m_playerControls, &PlayerControls::seekForward, player, [&]() {player->seek(5);});
    connect(ui->m_playerControls, &PlayerControls::seekBackward, player, [&]() {player->seek(-5);});
    connect(ui->m_playerControls, &PlayerControls::changeVolume, player, &QMediaPlayer::setVolume);
    connect(ui->m_playerControls, &PlayerControls::changeMuting, player, &QMediaPlayer::setMuted);
    connect(ui->m_playerControls, &PlayerControls::changeRate, player, &QMediaPlayer::setPlaybackRate);
    connect(ui->m_playerControls, &PlayerControls::stop, ui->m_videoWidget, QOverload<>::of(&QVideoWidget::update));
    connect(player, &QMediaPlayer::stateChanged, ui->m_playerControls, &PlayerControls::setState);
    connect(player, &QMediaPlayer::volumeChanged, ui->m_playerControls, &PlayerControls::setVolume);
    connect(player, &QMediaPlayer::mutedChanged, ui->m_playerControls, &PlayerControls::setMuted);
    connect(player, &MediaPlayer::message, this->statusBar(), &QStatusBar::showMessage);
    connect(player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this, &Tool::handleMediaPlayerError);

    //--------------------
    //Connect Mediaplayer to waveform widgets
    connect(player, &MediaPlayer::sendBuffer, ui->m_playerControls, &PlayerControls::processBuffer);
    connect(player, &MediaPlayer::sendDuration, ui->m_playerControls, &PlayerControls::getDuration);

    connect(player, &MediaPlayer::sendBuffer, ui->waveWidget, &AudioWaveForm::processBuffer);
    connect(player, &MediaPlayer::sendDuration, ui->waveWidget, &AudioWaveForm::getDuration);

    connect(ui->m_editor, &Editor::sendBlockTime, ui->waveWidget, &AudioWaveForm::getTimeArray);

    //--------------------

    // Connect components dependent on Player's position change to player
    connect(player, &QMediaPlayer::positionChanged, this,
            [&]()
            {
                ui->slider_position->setValue(player->position());
                ui->label_position->setText(player->getPositionInfo());
                ui->m_editor->highlightTranscript(player->elapsedTime());
            }
            );

    connect(player, &QMediaPlayer::durationChanged, this,
            [&]()
            {
                ui->slider_position->setRange(0, player->duration());
                ui->label_position->setText(player->getPositionInfo());
            }
            );

    // Connect edit menu actions
    connect(ui->edit_undo, &QAction::triggered, ui->m_editor, &Editor::undo);
    connect(ui->edit_redo, &QAction::triggered, ui->m_editor, &Editor::redo);
    connect(ui->edit_cut, &QAction::triggered, ui->m_editor, &Editor::cut);
    connect(ui->edit_copy, &QAction::triggered, ui->m_editor, &Editor::copy);
    connect(ui->edit_paste, &QAction::triggered, ui->m_editor, &Editor::paste);
    connect(ui->edit_findReplace, &QAction::triggered, ui->m_editor, &Editor::findReplace);

    // Connect view menu actions
    connect(ui->view_incFont, &QAction::triggered, this, [&]() {changeFontSize(+1);});
    connect(ui->view_decFont, &QAction::triggered, this, [&]() {changeFontSize(-1);});
    connect(ui->view_font, &QAction::triggered, this, &Tool::changeFont);
    connect(ui->view_toggleTagList, &QAction::triggered, this, [&]() {ui->m_tagListDisplay->setVisible(!ui->m_tagListDisplay->isVisible());});

    // Connect Editor menu actions and editor controls
    ui->m_editor->setWordEditor(ui->m_wordEditor);
    connect(ui->Save_as_PDF,&QAction::triggered,ui->m_editor,&Editor::saveAsPDF);
    connect(ui->actionSave_as_Text,&QAction::triggered,ui->m_editor,&Editor::saveAsTXT);
    connect(ui->Real_Time_Data_Saver,&QAction::triggered,ui->m_editor,&Editor::realTimeDataSavingToggle);
    connect(ui->Add_Custom_Dictonary, &QAction::triggered, ui->m_editor, &Editor::addCustomDictonary);
    connect(ui->actionShowLineTimeStamp,&QAction::triggered,ui->m_editor,[&]() {ui->m_editor->pushbutton(player->elapsedTime());});      // newly added
    connect(ui->actionRemove_Speaker,&QAction::triggered,ui->m_editor,&Editor::removespeaker);                                      // newly added
    connect(ui->actionword_count,&QAction::triggered,ui->m_editor,&Editor::on_actionword_count_triggered);
    connect(ui->actionRemove_Time_Stamp,&QAction::triggered,ui->m_editor,&Editor::removetimestamp);
    connect(ui->actionLink,&QAction::triggered,ui->m_editor,&Editor::on_actionLink_triggered);
    connect(ui->actionVoice_Typing_2,&QAction::triggered,ui->m_editor,&Editor::on_actionVoice_triggered);

    //    connect(ui->editor_openTranscript, &QAction::triggered, ui->m_editor, &Editor::transcriptOpen);
    connect(ui->editor_debugBlocks, &QAction::triggered, ui->m_editor, &Editor::showBlocksFromData);
    connect(ui->editor_save, &QAction::triggered, ui->m_editor, &Editor::transcriptSave);
    connect(ui->editor_saveAs, &QAction::triggered, ui->m_editor, &Editor::transcriptSaveAs);
    connect(ui->editor_close, &QAction::triggered, ui->m_editor, &Editor::transcriptClose);
    connect(ui->editor_jumpToLine, &QAction::triggered, ui->m_editor, &Editor::jumpToHighlightedLine);
    connect(ui->editor_splitLine, &QAction::triggered, ui->m_editor, [&]() {ui->m_editor->splitLine(player->elapsedTime());});
    connect(ui->editor_mergeUp, &QAction::triggered, ui->m_editor, &Editor::mergeUp);
    connect(ui->editor_mergeDown, &QAction::triggered, ui->m_editor, &Editor::mergeDown);
    connect(ui->editor_toggleWords, &QAction::triggered, ui->m_wordEditor, [&](){ui->m_wordEditor->setVisible(!ui->m_wordEditor->isVisible());});
    connect(ui->editor_changeLang, &QAction::triggered, ui->m_editor, &Editor::changeTranscriptLang);
    connect(ui->editor_changeSpeaker, &QAction::triggered, ui->m_editor, &Editor::createChangeSpeakerDialog);
    connect(ui->editor_propagateTime, &QAction::triggered, ui->m_editor, &Editor::createTimePropagationDialog);
    connect(ui->editor_editTags, &QAction::triggered, ui->m_editor, &Editor::createTagSelectionDialog);
    connect(ui->editor_autoSave, &QAction::triggered, ui->m_editor, [this](){ui->m_editor->useAutoSave(ui->editor_autoSave->isChecked());});
    connect(ui->m_editor, &Editor::message, this->statusBar(), &QStatusBar::showMessage);
    connect(ui->m_editor, &Editor::jumpToPlayer, player, &MediaPlayer::setPositionToTime);
    connect(ui->m_editor, &Editor::refreshTagList, ui->m_tagListDisplay, &TagListDisplayWidget::refreshTags);
    connect(ui->Show_Time_Stamps, &QAction::triggered, ui->m_editor,&Editor::setShowTimeStamp );
    connect(ui->move_along_timestamps, &QAction::triggered, ui->m_editor,&Editor::setMoveAlongTimeStamps );

    //Real time data saver
    //connect(ui->m_editor, &Editor::contentChanged, ui->m_editor, &Editor::transcriptSave);


    connect(ui->Open_File_in_Editor1, &QAction::triggered, ui->m_editor_2, &Editor::transcriptOpen);
    //    connect(ui->editor_debugBlocks, &QAction::triggered, ui->m_editor_2, &Editor::showBlocksFromData);
    connect(ui->editor_save2, &QAction::triggered, ui->m_editor_2, &Editor::transcriptSave);
    //    connect(ui->editor_saveAs, &QAction::triggered, ui->m_editor_2, &Editor::transcriptSaveAs);
    //    connect(ui->editor_close, &QAction::triggered, ui->m_editor_2, &Editor::transcriptClose);
    //    connect(ui->editor_jumpToLine, &QAction::triggered, ui->m_editor_2, &Editor::jumpToHighlightedLine);
    //    connect(ui->editor_splitLine, &QAction::triggered, ui->m_editor_2, [&]() {ui->m_editor_2->splitLine(player->elapsedTime());});
    //    connect(ui->editor_mergeUp, &QAction::triggered, ui->m_editor_2, &Editor::mergeUp);
    //    connect(ui->editor_mergeDown, &QAction::triggered, ui->m_editor_2, &Editor::mergeDown);
    //    connect(ui->editor_changeLang, &QAction::triggered, ui->m_editor_2, &Editor::changeTranscriptLang);
    //    connect(ui->editor_changeSpeaker, &QAction::triggered, ui->m_editor_2, &Editor::createChangeSpeakerDialog);
    //    connect(ui->editor_propagateTime, &QAction::triggered, ui->m_editor_2, &Editor::createTimePropagationDialog);
    //    connect(ui->editor_editTags, &QAction::triggered, ui->m_editor_2, &Editor::createTagSelectionDialog);
    //    connect(ui->editor_autoSave, &QAction::triggered, ui->m_editor_2, [this](){ui->m_editor_2->useAutoSave(ui->editor_autoSave->isChecked());});
    //    connect(ui->m_editor_2, &Editor::message, this->statusBar(), &QStatusBar::showMessage);
    //    connect(ui->m_editor_2, &Editor::jumpToPlayer, player, &MediaPlayer::setPositionToTime);
    //    connect(ui->m_editor_2, &Editor::refreshTagList, ui->m_tagListDisplay, &TagListDisplayWidget::refreshTags);
    //    connect(ui->Show_Time_Stamps, &QAction::triggered, ui->m_editor_2,&Editor::setShowTimeStamp );


    connect(ui->Open_File_in_Editor2, &QAction::triggered, ui->m_editor_3, &Editor::transcriptOpen);
    //    connect(ui->editor_debugBlocks, &QAction::triggered, ui->m_editor_2, &Editor::showBlocksFromData);
    connect(ui->editor_save3, &QAction::triggered, ui->m_editor_3, &Editor::transcriptSave);
    //    connect(ui->editor_saveAs, &QAction::triggered, ui->m_editor_2, &Editor::transcriptSaveAs);
    //    connect(ui->editor_close, &QAction::triggered, ui->m_editor_2, &Editor::transcriptClose);
    //    connect(ui->editor_jumpToLine, &QAction::triggered, ui->m_editor_2, &Editor::jumpToHighlightedLine);
    //    connect(ui->editor_splitLine, &QAction::triggered, ui->m_editor_2, [&]() {ui->m_editor_2->splitLine(player->elapsedTime());});
    //    connect(ui->editor_mergeUp, &QAction::triggered, ui->m_editor_2, &Editor::mergeUp);
    //    connect(ui->editor_mergeDown, &QAction::triggered, ui->m_editor_2, &Editor::mergeDown);
    //    connect(ui->editor_changeLang, &QAction::triggered, ui->m_editor_2, &Editor::changeTranscriptLang);
    //    connect(ui->editor_changeSpeaker, &QAction::triggered, ui->m_editor_2, &Editor::createChangeSpeakerDialog);
    //    connect(ui->editor_propagateTime, &QAction::triggered, ui->m_editor_2, &Editor::createTimePropagationDialog);
    //    connect(ui->editor_editTags, &QAction::triggered, ui->m_editor_2, &Editor::createTagSelectionDialog);
    //    connect(ui->editor_autoSave, &QAction::triggered, ui->m_editor_2, [this](){ui->m_editor_2->useAutoSave(ui->editor_autoSave->isChecked());});
    //    connect(ui->m_editor_2, &Editor::message, this->statusBar(), &QStatusBar::showMessage);
    //    connect(ui->m_editor_2, &Editor::jumpToPlayer, player, &MediaPlayer::setPositionToTime);
    //    connect(ui->m_editor_2, &Editor::refreshTagList, ui->m_tagListDisplay, &TagListDisplayWidget::refreshTags);
    //    connect(ui->Show_Time_Stamps, &QAction::triggered, ui->m_editor_2,&Editor::setShowTimeStamp );


    auto useTransliterationMenu = new QMenu("Use Transliteration", ui->menuEditor);
    auto group = new QActionGroup(this);
    auto langs = m_transliterationLang.keys();

    auto none = new QAction("None");
    none->setActionGroup(group);
    none->setCheckable(true);
    none->setChecked(true);
    none->setActionGroup(group);
    useTransliterationMenu->addAction(none);

    for(int i = 0; i < langs.size(); i++) {
        auto action = new QAction(langs[i]);
        action->setCheckable(true);
        action->setActionGroup(group);
        useTransliterationMenu->addAction(action);
    }
    group->setExclusive(true);
    ui->menuEditor->addMenu(useTransliterationMenu);

    connect(group, &QActionGroup::triggered, this, &Tool::transliterationSelected);


    // Connect keyboard shortcuts guide to help action
    connect(ui->help_keyboardShortcuts, &QAction::triggered, this, &Tool::createKeyboardShortcutGuide);

    // Connect position slider change to player position
    connect(ui->slider_position, &QSlider::sliderMoved, player, &MediaPlayer::setPosition);

    font = QFont( "Monospace", 10 );
    setFontForElements();
}

Tool::~Tool()
{
    delete player;
    delete ui;
}

void Tool::handleMediaPlayerError()
{
    const QString errorString = player->errorString();
    QString message = "Error: ";
    if (errorString.isEmpty())
        message += " #" + QString::number(int(player->error()));
    else
        message += errorString;
    statusBar()->showMessage(message);
}

void Tool::keyPressEvent(QKeyEvent *event)
{

    if (event->key() == Qt::Key_Up && event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier))
        ui->m_editor->speakerWiseJump("up");
    else if (event->key() == Qt::Key_Down && event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier))
        ui->m_editor->speakerWiseJump("down");
    else if (event->key() == Qt::Key_Left && event->modifiers() == Qt::AltModifier)
        ui->m_editor->wordWiseJump("left");
    else if (event->key() == Qt::Key_Right && event->modifiers() == Qt::AltModifier)
        ui->m_editor->wordWiseJump("right");
    else if (event->key() == Qt::Key_Up && event->modifiers() == Qt::AltModifier )
        ui->m_editor->blockWiseJump("up");
    else if (event->key() == Qt::Key_Down && event->modifiers() == Qt::AltModifier)
        ui->m_editor->blockWiseJump("down");
    else if (event->key() == Qt::Key_I && event->modifiers() == Qt::ControlModifier) {
        if (ui->m_editor->hasFocus())
            ui->m_editor->insertTimeStamp(player->elapsedTime());
        else if(ui->m_wordEditor->hasFocus())
            ui->m_wordEditor->insertTimeStamp(player->elapsedTime());
    }
    else
        QMainWindow::keyPressEvent(event);
}

void Tool::createKeyboardShortcutGuide()
{
    auto help_keyshortcuts = new KeyboardShortcutGuide(this);

    help_keyshortcuts->setAttribute(Qt::WA_DeleteOnClose);
    help_keyshortcuts->show();
}

void Tool::changeFont()
{
    bool fontSelected;
    font = QFontDialog::getFont(&fontSelected, QFont( font.family(), font.pointSize() ), this, tr("Pick a font") );

    if (fontSelected)
        setFontForElements();
}

void Tool::changeFontSize(int change)
{
    font.setPointSize(font.pointSize() + change);
    setFontForElements();
}

void Tool::setFontForElements()
{
    ui->m_editor->setEditorFont(font);
    ui->m_editor_2->setEditorFont(font);
    ui->m_editor_3->setEditorFont(font);
    ui->m_wordEditor->setFont(font);
    ui->m_wordEditor->fitTableContents();

    ui->m_tagListDisplay->setFont(font);
}

void Tool::setTransliterationLangCodes()
{
    QStringList languages = QString("ENGLISH AMHARIC ARABIC BENGALI CHINESE GREEK GUJARATI HINDI KANNADA MALAYALAM MARATHI NEPALI ORIYA PERSIAN PUNJABI RUSSIAN SANSKRIT SINHALESE SERBIAN TAMIL TELUGU TIGRINYA URDU").split(" ");
    QStringList langCodes = QString("en am ar bn zh el gu hi kn ml mr ne or fa pa ru sa si sr ta te ti ur").split(" ");

    for (int i = 0; i < languages.size(); i++)
        m_transliterationLang.insert(languages[i], langCodes[i]);
}

void Tool::transliterationSelected(QAction* action)
{
    if (action->text() == "None") {
        ui->m_editor->useTransliteration(false);
        return;
    }


    auto langCode = m_transliterationLang[action->text()];
    qDebug() << langCode;
    ui->m_editor->useTransliteration(true, langCode);
}



void Tool::on_Upload_and_generate_Transcript_triggered()
{

    QFileDialog fileDialog(this);
    fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
    fileDialog.setWindowTitle(tr("Open File"));
    fileDialog.setDirectory(QStandardPaths::standardLocations(
                                QStandardPaths::DocumentsLocation).value(0, QDir::homePath()));


    if (fileDialog.exec() == QDialog::Accepted) {
        QUrl *fileUrl = new QUrl(fileDialog.selectedUrls().constFirst());
        TranscriptGenerator tr(fileUrl);
        qInfo()<<QThread::currentThread();
        //running transcript generator on another thread (parallel processing)
        //QtConcurrent::run(&tr, &TranscriptGenerator::Upload_and_generate_Transcript);
        tr.Upload_and_generate_Transcript();


        QFile myfile(fileUrl->toLocalFile());
        QFileInfo fileInfo(myfile);
        QString filename(fileInfo.fileName());
        QString filepaths=fileInfo.dir().path();

        qInfo()<<filepaths+"/transcript.xml";

        bool fileExists = QFileInfo::exists(filepaths+"/transcript.xml") && QFileInfo(filepaths+"/transcript.xml").isFile();
        while(!fileExists){
            fileExists = QFileInfo::exists(filepaths+"/transcript.xml") && QFileInfo(filepaths+"/transcript.xml").isFile();
        }

        QFile transcriptFile(filepaths+"/transcript.xml");
        if (!transcriptFile.open(QIODevice::ReadOnly)) {
            qInfo()<<(transcriptFile.errorString());
            return;
        }
        ui->m_editor->loadTranscriptData(transcriptFile);
        ui->m_editor->setContent();
        transcriptFile.close();

        QFile transcriptFile2(filepaths+"/transcript.xml");
        if (!transcriptFile2.open(QIODevice::ReadOnly)) {
            qInfo()<<(transcriptFile2.errorString());
            return;
        }
        ui->m_editor_2->loadTranscriptData(transcriptFile2);
        ui->m_editor_2->setContent();
    /*
    QFileDialog fileDialog(this);
    fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
    fileDialog.setWindowTitle(tr("Open File"));
    fileDialog.setDirectory(QStandardPaths::standardLocations(
                                QStandardPaths::DocumentsLocation).value(0, QDir::homePath()));


    if (fileDialog.exec() == QDialog::Accepted) {
        QUrl *fileUrl = new QUrl(fileDialog.selectedUrls().constFirst());
        TranscriptGenerator tr(fileUrl);
        tr.Upload_and_generate_Transcript();

        //----------------------------
        QFile myfile(fileUrl->toLocalFile());
        QFileInfo fileInfo(myfile);
        QString filename(fileInfo.fileName());
        QString filepaths=fileInfo.dir().path();

        qInfo()<<filepaths+"/transcript.xml";

        bool fileExists = QFileInfo::exists(filepaths+"/transcript.xml") && QFileInfo(filepaths+"/transcript.xml").isFile();
        while(!fileExists){
            fileExists = QFileInfo::exists(filepaths+"/transcript.xml") && QFileInfo(filepaths+"/transcript.xml").isFile();
        }

        QFile transcriptFile(filepaths+"/transcript.xml");
        if (!transcriptFile.open(QIODevice::ReadOnly)) {
            qInfo()<<(transcriptFile.errorString());
            return;
        }
        ui->m_editor->loadTranscriptData(transcriptFile);
        ui->m_editor->setContent();
        transcriptFile.close();

        QFile transcriptFile2(filepaths+"/transcript.xml");
        if (!transcriptFile2.open(QIODevice::ReadOnly)) {
            qInfo()<<(transcriptFile2.errorString());
            return;
        }
        ui->m_editor_2->loadTranscriptData(transcriptFile2);
        ui->m_editor_2->setContent();
        //----------------------------
        */
    }



}


void Tool::on_btn_translate_clicked()
{

    QProgressBar progressBar;
    progressBar.setMinimum(0);
    progressBar.setMaximum(100);
    progressBar.setValue(10);
    progressBar.show();
    progressBar.raise();
    progressBar.activateWindow();

    if(!QFile::exists("Translate.py")){
        QFile mapper("Translate.py");
        QFileInfo mapperFileInfo(mapper);
        QFile aligner(":/Translate.py");
        if(!aligner.open(QIODevice::OpenModeFlag::ReadOnly)){
            return;
        }
        aligner.seek(0);
        QString cp=aligner.readAll();
        aligner.close();

        if(!mapper.open(QIODevice::OpenModeFlag::WriteOnly|QIODevice::Truncate)){

            return;
        }
        mapper.write(QByteArray(cp.toUtf8()));
        mapper.close();
        std::string makingexec="chmod +x "+mapperFileInfo.absoluteFilePath().replace(" ", "\\ ").toStdString();
        int result = system(makingexec.c_str());
        qInfo()<<result;
    }




    QFile translate("toTranslate.txt");
    if(!translate.open(QIODevice::OpenModeFlag::WriteOnly)){
        QMessageBox::critical(this,"Error",translate.errorString());
        return;
    }
    QString x("");
    for (auto& a_block : qAsConst(ui->m_editor_2->m_blocks)) {
        auto blockText = a_block.text + " " ;
        //            qInfo()<<a_block.text;
        x.append(blockText + "\n");
    }
    QTextStream outer(&translate);
    outer<<x;
    translate.close();

    QFileInfo TranslateFile(translate);
    QString Translatefilepaths=TranslateFile.dir().path();

    QFile myfile(ui->m_editor_2->m_transcriptUrl.toLocalFile());

    QFileInfo fileInfo(myfile);
    QString filename(fileInfo.fileName());
    QString filepaths=fileInfo.dir().path();
    QString translatedFile=filepaths+"/HindiTranslated.xml";
    QString filepaths2=filepaths;
    qInfo()<<filepaths2;



    QFile mapper("Translate.py");
    QFileInfo mapperFileInfo(mapper);

    QFile finalFile(translatedFile);
    QFileInfo finalFileInfo(finalFile);
    QFile HindiTranslated("HindiTranslated.txt");
    QFileInfo HindiTranslate(HindiTranslated);
    QFileInfo initialDictFileInfo(translate);
    int result;
    QFile transcriptFileToTranslate(ui->m_editor_2->m_transcriptUrl.toLocalFile());
    QFileInfo FromTranscriptFileToTranslate(transcriptFileToTranslate);

    QFile tempXML("temp.xml");
    QFileInfo tempXMLinfo(tempXML);

    std::string translatorStr="python3 "  +mapperFileInfo.absoluteFilePath().toStdString()
                                +" "+ '\"'+initialDictFileInfo.absoluteFilePath().toStdString()+ '\"'
                                +" "+ '\"'+HindiTranslate.absoluteFilePath().toStdString()+ '\"'
                                +" "+ '\"'+finalFileInfo.absoluteFilePath().toStdString()+ '\"'
                                +" "+ '\"'+FromTranscriptFileToTranslate.absoluteFilePath().toStdString()+ '\"'
                                +" "+ '\"'+tempXMLinfo.absoluteFilePath().toStdString()+ '\"';

    qInfo()<<translatorStr.c_str();

    int result2 = system(translatorStr.c_str());
    qInfo()<<result2;

    qInfo()<<"Save Pressed";
    bool fileExists = QFileInfo::exists(filepaths2+"/HindiTranslated.xml") && QFileInfo(filepaths2+"/HindiTranslated.xml").isFile();
    while(!fileExists){
        fileExists = QFileInfo::exists(filepaths2+"/HindiTranslated.xml") && QFileInfo(filepaths2+"/HindiTranslated.xml").isFile();
    }
    qInfo()<<"path exists now";
    QFile transcriptFile3(filepaths2+"/HindiTranslated.xml");
    if (!transcriptFile3.open(QIODevice::ReadOnly)) {
        qInfo()<<(transcriptFile3.errorString());
        return;
    }
    ui->m_editor_3->loadTranscriptData(transcriptFile3);
    ui->m_editor_3->setContent();
    qInfo()<<"translated";
    progressBar.setValue(100);
    progressBar.hide();


}


void Tool::on_editor_openTranscript_triggered()
{
    ui->m_editor->m_transcriptUrl.clear();
    ui->m_editor->transcriptOpen();
    ui->m_editor->setContent();
    //    ui->m_editor_2->m_transcriptUrl=ui->m_editor->m_transcriptUrl;
    //    QFile transcriptFile(ui->m_editor->m_transcriptUrl.toLocalFile());
    //    if (!transcriptFile.open(QIODevice::ReadOnly)) {
    //        qInfo()<<(transcriptFile.errorString());
    //        return;
    //    }
    //    ui->m_editor_2->loadTranscriptData(transcriptFile);
    //    ui->m_editor_2->setContent();
}





void Tool::on_add_video_clicked()
{
    player->open();
}


void Tool::on_open_transcript_clicked()
{
    Tool::on_editor_openTranscript_triggered();
}


void Tool::on_new_transcript_clicked()
{
    ui->m_editor->close();
}


void Tool::on_save_transcript_clicked()
{
    ui->m_editor->transcriptSave();
}


void Tool::on_save_as_transcript_clicked()
{
    ui->m_editor->transcriptSaveAs();
}


void Tool::on_load_a_custom_dict_clicked()
{
    ui->m_editor->addCustomDictonary();
}


void Tool::on_get_PDF_clicked()
{
    ui->m_editor->saveAsPDF();
}


void Tool::on_decreseFontSize_clicked()
{
    this->changeFontSize(-1);
}


void Tool::on_increaseFontSize_clicked()
{
    this->changeFontSize(+1);

}


void Tool::on_toggleWordEditor_clicked()
{
    ui->m_wordEditor->setVisible(!ui->m_wordEditor->isVisible());
}


void Tool::on_keyboard_shortcuts_clicked()
{
    this->createKeyboardShortcutGuide();
}


void Tool::on_fontComboBox_currentFontChanged(const QFont &f)
{
    //    bool fontSelected;
    font = f;

    //    if (fontSelected)
    setFontForElements();}

