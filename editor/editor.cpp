#include "editor.h"
#include <qclipboard.h>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include <QPainter>
#include <QTextBlock>
#include <QFileDialog>
#include <QInputDialog>
#include <QStandardPaths>
#include <QAbstractItemView>
#include <QScrollBar>
#include <QStringListModel>
#include <QMessageBox>
#include <QMenu>
#include <algorithm>
#include <QEventLoop>
#include <QDebug>
#include <QUndoStack>
#include<QPrinter>

// New Includes
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSet>

#include <QMediaRecorder>
#include <QAudioProbe>
#include <QAudioRecorder>
#include <QAudioEncoderSettings>

#include <QPushButton>
#include <QHBoxLayout>
#include <QApplication>

#include <QAudioInput>
#include <QBuffer>
#include <QNetworkRequest>


Editor::Editor(QWidget *parent)
    : TextEditor(parent),
    m_speakerCompleter(makeCompleter()), m_textCompleter(makeCompleter()), m_transliterationCompleter(makeCompleter()),
    m_dictionary(listFromFile(":/wordlists/english.txt")), m_transcriptLang("english"),
    timeStampExp(QRegularExpression(R"(\[(\d?\d:)?[0-5]?\d:[0-5]?\d(\.\d\d?\d?)?])")),
    speakerExp(QRegularExpression(R"(\[.*]:)")),
    m_saveTimer(new QTimer(this))
{
    connect(this->document(), &QTextDocument::contentsChange, this, &Editor::contentChanged);
    connect(this, &Editor::cursorPositionChanged, this, &Editor::updateWordEditor);
    connect(this, &Editor::cursorPositionChanged, this,
            [&]()
            {
                if (!m_blocks.isEmpty() && textCursor().blockNumber() < m_blocks.size())
                    emit refreshTagList(m_blocks[textCursor().blockNumber()].tagList);
            });

    m_textCompleter->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    m_transliterationCompleter->setModel(new QStringListModel);

    loadDictionary();

    connect(m_speakerCompleter, QOverload<const QString &>::of(&QCompleter::activated),
            this, &Editor::insertSpeakerCompletion);
    connect(m_textCompleter, QOverload<const QString &>::of(&QCompleter::activated),
            this, &Editor::insertTextCompletion);
    connect(m_transliterationCompleter, QOverload<const QString &>::of(&QCompleter::activated),
            this, &Editor::insertTransliterationCompletion);


    connect(m_saveTimer, &QTimer::timeout, this, [this](){
        if (m_autoSave && m_transcriptUrl.isValid())
            transcriptSave();
    });
    m_saveTimer->start(m_saveInterval * 1000);

    m_blocks.append(fromEditor(0));

    m_audioRecorder = new QAudioRecorder();
    m_probe = new QAudioProbe();

    //    undoStack =  new QUndoStack(this);
}


int countwords = 0;
int speakercount = 0;
int modifiedwords = 0;
QSet<QString> mySet;
QSet<QString> modset;

void Editor::setEditorFont(const QFont& font)
{
    document()->setDefaultFont(font);
    m_textCompleter->popup()->setFont(font);
    m_speakerCompleter->popup()->setFont(font);
    m_transliterationCompleter->popup()->setFont(font);
    setLineNumberAreaFont(font);
}

void Editor::setMoveAlongTimeStamps()
{
    if(moveAlongTimeStamps==true)
        moveAlongTimeStamps=false;
    else
        moveAlongTimeStamps=true;
    qInfo()<<moveAlongTimeStamps;
}



void Highlighter::highlightBlock(const QString& text)
{

    if (invalidBlockNumbers.contains(currentBlock().blockNumber())) {
        QTextCharFormat format;
        format.setForeground(Qt::red);
        setFormat(0, text.size(), format);
        return;
    }
    else if (taggedBlockNumbers.contains(currentBlock().blockNumber())) {
        QTextCharFormat format;
        format.setForeground(Qt::blue);
        setFormat(0, text.size(), format);
        return;
    }
    if (invalidWords.contains(currentBlock().blockNumber())) {
        auto invalidWordNumbers = invalidWords.values(currentBlock().blockNumber());
        auto speakerEnd = 0;
        auto speakerMatch = QRegularExpression(R"(\[.*]:)").match(text);
        if (speakerMatch.hasMatch())
            speakerEnd = speakerMatch.capturedEnd();

        auto words = text.mid(speakerEnd + 1).split(" ");

        int start = speakerEnd;

        QTextCharFormat format;
        format.setFontUnderline(true);
        format.setUnderlineColor(Qt::red);
        format.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
        //to remove highlight of timestamp
        /*if(Editor::showTimeStamp){
            qInfo()<<"changed to true";
            for (int i = 0; i < words.size() -1 ; i++) {
                if (!invalidWordNumbers.contains(i))
                    continue;
                for (int j = 0; j < i; j++) start += (words[j].size() + 1);
                int count = words[i].size();
                setFormat(start + 1, count, format);
                start = speakerEnd;
            }
        }else*/ {
            for (int i = 0; i < words.size() ; i++) {
                if (!invalidWordNumbers.contains(i))
                    continue;
                for (int j = 0; j < i; j++) start += (words[j].size() + 1);
                int count = words[i].size();
                setFormat(start + 1, count, format);
                start = speakerEnd;
            }}
    }
    if (taggedWords.contains(currentBlock().blockNumber())) {
        auto invalidWordNumbers = taggedWords.values(currentBlock().blockNumber());
        auto speakerEnd = 0;
        auto speakerMatch = QRegularExpression(R"(\[.*]:)").match(text);
        if (speakerMatch.hasMatch())
            speakerEnd = speakerMatch.capturedEnd();

        auto words = text.mid(speakerEnd + 1).split(" ");

        int start = speakerEnd;

        QTextCharFormat format;
        format.setForeground(Qt::blue);
        //to remove highlight of timestamp
        /*if(Editor::showTimeStamp){
            qInfo()<<"changed to true";
            for (int i = 0; i < words.size() -1 ; i++) {
                if (!invalidWordNumbers.contains(i))
                    continue;
                for (int j = 0; j < i; j++) start += (words[j].size() + 1);
                int count = words[i].size();
                setFormat(start + 1, count, format);
                start = speakerEnd;
            }
        }else*/ {
            for (int i = 0; i < words.size() ; i++) {
                if (!invalidWordNumbers.contains(i))
                    continue;
                for (int j = 0; j < i; j++) start += (words[j].size() + 1);
                int count = words[i].size();
                setFormat(start + 1, count, format);
                start = speakerEnd;
            }}
    }
    if (blockToHighlight == -1)
        return;
    else if (currentBlock().blockNumber() == blockToHighlight) {
        int speakerEnd = 0;
        int lineEnd=text.length();
        auto speakerMatch = QRegularExpression(R"(\[.*]:)").match(text);
        if (speakerMatch.hasMatch())
            speakerEnd = speakerMatch.capturedEnd();

        int timeStampStart = QRegularExpression(R"(\[(\d?\d:)?[0-5]?\d:[0-5]?\d(\.\d\d?\d?)?])").match(text).capturedStart();
        qInfo()<<timeStampStart;
        QTextCharFormat format;

        format.setForeground(QColor(Qt::blue).lighter(120));
        setFormat(0, speakerEnd, format);
        format. setFontWeight(QFont::Bold);
        setFormat(0, speakerEnd, format);


        format.setForeground(Qt::black);
        setFormat(speakerEnd, lineEnd, format);
        format. setFontWeight(QFont::Bold);
        setFormat(speakerEnd, lineEnd, format);

        //        if (invalidWords.contains(currentBlock().blockNumber())) {
        //            qInfo()<<"in";
        //            auto invalidWordNumbers = invalidWords.values(currentBlock().blockNumber());
        //            auto speakerEnd = 0;
        //            auto speakerMatch = QRegularExpression(R"(\[.*]:)").match(text);
        //            if (speakerMatch.hasMatch())
        //                speakerEnd = speakerMatch.capturedEnd();

        //            auto words = text.mid(speakerEnd + 1).split(" ");
        //            int start = speakerEnd;

        //            QTextCharFormat format;
        //            format.setForeground(Qt::black);
        //            setFormat(speakerEnd, lineEnd, format);
        //            format. setFontWeight(QFont::Bold);
        //            setFormat(speakerEnd, lineEnd, format);
        //            format.setFontUnderline(true);
        //            format.setUnderlineColor(Qt::red);
        //            format.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);

        //            for (int i = 0; i < words.size() ; i++) {
        //                if (invalidWordNumbers.contains(i)){
        //                    for (int j = 0; j < i; j++) start += (words[j].size() + 1);
        //                    int count = words[i].size();
        //                    setFormat(start + 1, count, format);
        //                    start = speakerEnd;
        //                }
        //            }
        //        }
        //        if (taggedWords.contains(currentBlock().blockNumber())) {
        //            auto invalidWordNumbers = taggedWords.values(currentBlock().blockNumber());
        //            auto speakerEnd = 0;
        //            auto speakerMatch = QRegularExpression(R"(\[.*]:)").match(text);
        //            if (speakerMatch.hasMatch())
        //                speakerEnd = speakerMatch.capturedEnd();

        //            auto words = text.mid(speakerEnd + 1).split(" ");
        //            int start = speakerEnd;

        //            QTextCharFormat format;
        //            format.setForeground(Qt::black);
        //            setFormat(speakerEnd, lineEnd, format);
        //            format. setFontWeight(QFont::Bold);
        //            setFormat(speakerEnd, lineEnd, format);
        //            format.setForeground(Qt::blue);

        //            for (int i = 0; i < words.size() ; i++) {
        //                if (invalidWordNumbers.contains(i)){
        //                    for (int j = 0; j < i; j++) start += (words[j].size() + 1);
        //                    int count = words[i].size();
        //                    setFormat(start + 1, count, format);
        //                    start = speakerEnd;
        //                }
        //            }
        //        }
        if (taggedWords.contains(currentBlock().blockNumber())||invalidWords.contains(currentBlock().blockNumber())) {
            auto taggedWordNumbers = taggedWords.values(currentBlock().blockNumber());
            auto invalidWordNumbers = invalidWords.values(currentBlock().blockNumber());
            auto speakerEnd = 0;
            auto speakerMatch = QRegularExpression(R"(\[.*]:)").match(text);
            if (speakerMatch.hasMatch())
                speakerEnd = speakerMatch.capturedEnd();

            auto words = text.mid(speakerEnd + 1).split(" ");
            int start = speakerEnd;


            QTextCharFormat format1;
            format1.setForeground(Qt::black);
            format1. setFontWeight(QFont::Bold);
            format1.setFontUnderline(true);
            format1.setUnderlineColor(Qt::red);
            format1.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);

            for (int i = 0; i < words.size() ; i++) {
                if (invalidWordNumbers.contains(i)){
                    for (int j = 0; j < i; j++) start += (words[j].size() + 1);
                    int count = words[i].size();
                    setFormat(start + 1, count, format1);
                    start = speakerEnd;
                    qInfo()<<"in";
                }
            }


            QTextCharFormat format2;
            format2.setForeground(Qt::black);
            format2. setFontWeight(QFont::Bold);
            format2.setForeground(Qt::blue);

            for (int i = 0; i < words.size() ; i++) {
                if (taggedWordNumbers.contains(i)){
                    for (int j = 0; j < i; j++) start += (words[j].size() + 1);
                    int count = words[i].size();
                    setFormat(start + 1, count, format2);
                    start = speakerEnd;
                }
            }




            QTextCharFormat format;
            format.setForeground(Qt::black);
            format. setFontWeight(QFont::Bold);
            format.setForeground(Qt::blue);
            format.setFontUnderline(true);
            format.setUnderlineColor(Qt::red);
            format.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);

            for (int i = 0; i < words.size() ; i++) {
                if (taggedWordNumbers.contains(i)&&invalidWordNumbers.contains(i))
                {
                    for (int j = 0; j < i; j++) start += (words[j].size() + 1);
                    int count = words[i].size();
                    setFormat(start + 1, count, format);
                    start = speakerEnd;
                }
            }
        }
        format.setForeground(Qt::red);
        setFormat(timeStampStart, text.size(), format);
        format. setFontWeight(QFont::Light);
        setFormat(timeStampStart, text.size(), format);

        auto words = text.mid(speakerEnd + 1).split(" ");

        if (wordToHighlight != -1 && wordToHighlight < words.size()) {
            int start = speakerEnd;
            for (int i=0; i < wordToHighlight; i++) start += (words[i].size() + 1);
            int count = words[wordToHighlight].size();

            format.setFontUnderline(true);
            format.setUnderlineColor(Qt::green);
            format.setUnderlineStyle(QTextCharFormat::DashUnderline);
            format.setForeground(Qt::green);
            setFormat(start + 1, count, format);
        }
    }
}



void Editor::mousePressEvent(QMouseEvent *e)
{
    QPlainTextEdit::mousePressEvent(e);
    if (e->modifiers() == Qt::ControlModifier && !m_blocks.isEmpty())
        helpJumpToPlayer();
}

void Editor::keyPressEvent(QKeyEvent *event)
{
    if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_R)
        createChangeSpeakerDialog();
    else if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_T)
        createTimePropagationDialog();
    else if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_M){
        QString blockText = textCursor().block().text();
        QString textTillCursor = blockText.left(textCursor().positionInBlock());

        const bool containsSpeakerBraces = blockText.leftRef(blockText.indexOf(" ")).contains("]:");
        const bool containsTimeStamp = blockText.trimmed() != "" && blockText.trimmed().back() == ']';
        bool isAWordUnderCursor = false;
        int wordNumber = 0;

        if ((containsSpeakerBraces && textTillCursor.count(" ") > 0) || !containsSpeakerBraces) {
            if (m_blocks.size() > textCursor().blockNumber() &&
                !(containsTimeStamp && textTillCursor.count(" ") == blockText.count(" "))) {
                isAWordUnderCursor = true;

                if (containsSpeakerBraces) {
                    auto textWithoutSpeaker = textTillCursor.split("]:").last();
                    wordNumber = textWithoutSpeaker.trimmed().count(" ");
                }
                else
                    wordNumber = textTillCursor.trimmed().count(" ");
            }
        }

        markWordAsCorrect(textCursor().blockNumber(), wordNumber);
    }
    else if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_I){
        qInfo()<<"doubtful";
        int blocknumber=textCursor().blockNumber(), wordNumber=textCursor().block().text().left(textCursor().positionInBlock()).trimmed().count(" ");
        qInfo()<<blocknumber;
        qInfo()<<wordNumber;
        if(m_blocks[blocknumber].words[wordNumber-1].tagList.empty()){
            m_blocks[blocknumber].words[wordNumber-1].tagList.append("InvW");
            qInfo()<<"marked";
        }

        QTextBlock block = document()->findBlockByNumber(blocknumber);
        QTextCursor cursor(document()->findBlockByNumber(blocknumber));
        setContent();
        cursor.setPosition(block.position());
        cursor.movePosition(QTextCursor::NextWord, QTextCursor::MoveAnchor, wordNumber - 1);
        setTextCursor(cursor);
        centerCursor();
    }
    qInfo()<<event;

    auto checkPopupVisible = [](QCompleter* completer) {
        return completer && completer->popup()->isVisible();
    };

    if (checkPopupVisible(m_textCompleter)
        || checkPopupVisible(m_speakerCompleter)
        || checkPopupVisible(m_transliterationCompleter)
        )
    {

        // The following keys are forwarded by the completer to the widget
        switch (event->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_Escape:
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
            //       case Qt::Key_Space:
            event->ignore();
            return; // let the completer do default behavior
        default:
            break;
        }
    }

    TextEditor::keyPressEvent(event);

    QString blockText = textCursor().block().text();
    QString textTillCursor = blockText.left(textCursor().positionInBlock());
    QString completionPrefix;

    const bool shortcutPressed = (event->key() == Qt::Key_N && event->modifiers() == Qt::ControlModifier);
    const bool hasModifier = (event->modifiers() != Qt::NoModifier);
    const bool containsSpeakerBraces = blockText.leftRef(blockText.indexOf(" ")).contains("]:");

    static QString eow("~!@#$%^&*()_+{}|:\"<>?,./;'[]\\-="); // end of word

    if (!shortcutPressed && (hasModifier || event->text().isEmpty() || eow.contains(event->text().right(1)))) {
        m_speakerCompleter->popup()->hide();
        m_textCompleter->popup()->hide();
        m_transliterationCompleter->popup()->hide();
        return;
    }

    QCompleter *m_completer = nullptr;

    if (!textTillCursor.count(" ") && !textTillCursor.contains("]:") && !textTillCursor.contains("]")
        && textTillCursor.size() && containsSpeakerBraces) {
        // Complete speaker

        m_completer = m_speakerCompleter;

        completionPrefix = blockText.left(blockText.indexOf(" "));
        completionPrefix = completionPrefix.mid(1, completionPrefix.size() - 3);

        QList<QString> speakers;
        for (auto& a_block: qAsConst(m_blocks))
            if (!speakers.contains(a_block.speaker) && a_block.speaker != "")
                speakers.append(a_block.speaker);

        m_speakerCompleter->setModel(new QStringListModel(speakers, m_speakerCompleter));
    }
    else {
        if(!showTimeStamp){
            QString blockText = textCursor().block().text();
            QString textTillCursor = blockText.left(textCursor().positionInBlock());

            const bool containsSpeakerBraces = blockText.leftRef(blockText.indexOf(" ")).contains("]:");
            const bool containsTimeStamp = blockText.trimmed() != "" && blockText.trimmed().back() == ']';
            bool isAWordUnderCursor = false;
            int wordNumber = 0;

            if ((containsSpeakerBraces && textTillCursor.count(" ") > 0) || !containsSpeakerBraces) {
                if (m_blocks.size() > textCursor().blockNumber() &&
                    !(containsTimeStamp && textTillCursor.count(" ") == blockText.count(" "))) {
                    isAWordUnderCursor = true;

                    if (containsSpeakerBraces) {
                        auto textWithoutSpeaker = textTillCursor.split("]:").last();
                        wordNumber = textWithoutSpeaker.trimmed().count(" ");
                    }
                    else
                        wordNumber = textTillCursor.trimmed().count(" ");
                }
            }
            if(m_blocks[textCursor().block().blockNumber()].words[wordNumber].text.size()>2){
                QString text = m_blocks[textCursor().blockNumber()].words[wordNumber].text.toLower();
                QString text2 = m_blocks[textCursor().blockNumber()].words[wordNumber].text;
                text=text.trimmed();
                text2=text.trimmed();
                //            qInfo()<<text;
                QString val;
                QFile file;
                file.setFileName("replacedTextDictonary.json");
                file.open(QIODevice::ReadOnly | QIODevice::Text);
                val = file.readAll();
                file.close();
                QJsonDocument d = QJsonDocument::fromJson(val.toUtf8());
                QJsonObject sett2 = d.object();
                QJsonArray value = sett2[text].toArray();
                QStringList allSuggestions;
                if(value.size()>0){
                    for( auto i : value){
                        allSuggestions<< i.toString();
                    }
                    //                qInfo()<<allSuggestions;
                    QMenu *sugg=new QMenu;
                    for(auto i:allSuggestions ){
                        auto readmeJson = new QAction;
                        readmeJson->setText(i);
                        connect(readmeJson, &QAction::triggered, this,[this,i]()
                                {
                                    suggest(i);
                                });
                        sugg->addAction(readmeJson);
                    }
                    //                 sugg->setMaximumHeight(50);
                    sugg->setStyleSheet("QMenu { menu-scrollable: 1; }");


                    QPoint x; x.setX(QCursor::pos().rx()+2);x.setY(QCursor::pos().ry());
                    sugg->exec(x);
                    //                delete sugg;
                }
            }
        }
        if(showTimeStamp)
            if (m_blocks[textCursor().blockNumber()].timeStamp.isValid()
                && textTillCursor.count(" ") == blockText.count(" "))
                return;

        int index = textTillCursor.count(" ");
        completionPrefix = blockText.split(" ")[index];

        if (completionPrefix.isEmpty()){
            m_textCompleter->popup()->hide();
            m_transliterationCompleter->popup()->hide();
            return;
        }

        if (completionPrefix.size() < 2 && !m_transliterate) {
            m_textCompleter->popup()->hide();
            return;
        }

        // Complete text
        if (!m_transliterate)
            m_completer = m_textCompleter;
        else
            m_completer = m_transliterationCompleter;
    }

    if (!m_completer)
        return;

    if (m_completer == m_transliterationCompleter) {
        QTimer replyTimer;
        replyTimer.setSingleShot(true);
        QEventLoop loop;
        connect(this, &Editor::replyCame, &loop, &QEventLoop::quit);
        connect(&replyTimer, &QTimer::timeout, &loop,
                [&]() {
                    emit message("Reply Timeout, Network Connection is slow or inaccessible", 2000);
                    loop.quit();
                });

        sendRequest(completionPrefix, m_transliterateLangCode);
        replyTimer.start(1000);
        loop.exec();

        dynamic_cast<QStringListModel*>(m_completer->model())->setStringList(m_lastReplyList);
    }


    if (m_completer != m_transliterationCompleter && completionPrefix != m_completer->completionPrefix()) {
        m_completer->setCompletionPrefix(completionPrefix);
    }
    m_completer->popup()->setCurrentIndex(m_completer->completionModel()->index(0, 0));

    QRect cr = cursorRect();
    cr.setWidth(m_completer->popup()->sizeHintForColumn(0)
                + m_completer->popup()->verticalScrollBar()->sizeHint().width());
    m_completer->complete(cr);
}

void Editor::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = createStandardContextMenu();

    QString blockText = textCursor().block().text();
    QString textTillCursor = blockText.left(textCursor().positionInBlock());

    const bool containsSpeakerBraces = blockText.leftRef(blockText.indexOf(" ")).contains("]:");
    const bool containsTimeStamp = blockText.trimmed() != "" && blockText.trimmed().back() == ']';
    bool isAWordUnderCursor = false;
    int wordNumber = 0;

    if ((containsSpeakerBraces && textTillCursor.count(" ") > 0) || !containsSpeakerBraces) {
        if (m_blocks.size() > textCursor().blockNumber() &&
            !(containsTimeStamp && textTillCursor.count(" ") == blockText.count(" "))) {
            isAWordUnderCursor = true;

            if (containsSpeakerBraces) {
                auto textWithoutSpeaker = textTillCursor.split("]:").last();
                wordNumber = textWithoutSpeaker.trimmed().count(" ");
            }
            else
                wordNumber = textTillCursor.trimmed().count(" ");
        }
    }

    if (isAWordUnderCursor) {


        auto markAsCorrectAction = new QAction;
        markAsCorrectAction->setText("Mark As Correct");

        connect(markAsCorrectAction, &QAction::triggered, this,
                [this, wordNumber]()
                {
                    markWordAsCorrect(textCursor().blockNumber(), wordNumber);
                });
        menu->addAction(markAsCorrectAction);
        //added suggestions
        QString text = m_blocks[textCursor().blockNumber()].words[wordNumber].text.toLower();
        QString text2 = m_blocks[textCursor().blockNumber()].words[wordNumber].text;
        text=text.trimmed();
        text2=text.trimmed();
        //        qInfo()<<text;
        QString val;
        QFile file;
        file.setFileName("replacedTextDictonary.json");
        file.open(QIODevice::ReadOnly | QIODevice::Text);
        val = file.readAll();
        file.close();
        QJsonDocument d = QJsonDocument::fromJson(val.toUtf8());
        QJsonObject sett2 = d.object();
        QJsonArray value = sett2[text].toArray();
        QStringList allSuggestions;
        for( auto i : value){
            allSuggestions<< i.toString();
        }

        auto AddToClipBoard = new QAction;
        AddToClipBoard->setText("Add to clipboard");

        connect(AddToClipBoard, &QAction::triggered, this,
                [this, text2]()
                {
                    allClips.append(text2);
                });

        menu->addAction(AddToClipBoard);




        //        QTextCursor cursor = textCursor();
        //        if(cursor.hasSelection())
        //        {qInfo()<<"inside text cursor to upper";
        //            cursor.insertText("hello");
        //        }






        QMenu *sugg=menu->addMenu(tr("&Suggestions"));
        for(auto i:allSuggestions ){
            auto readmeJson = new QAction;
            readmeJson->setText(i);
            connect(readmeJson, &QAction::triggered, this,[this,i]()
                    {
                        suggest(i);
                    });
            sugg->addAction(readmeJson);
        }

    }
    QMenu *clip=menu->addMenu(tr("&Clipboard"));
    for(auto i:allClips ){
        auto readmeJson = new QAction;
        readmeJson->setText(i);
        connect(readmeJson, &QAction::triggered, this,[this,i]()
                {
                    suggest(i);
                });
        clip->addAction(readmeJson);
    }


    menu->exec(event->globalPos());
    delete menu;
}

void Editor::transcriptOpen()
{
    QFileDialog fileDialog(this);
    fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
    fileDialog.setWindowTitle(tr("Open File"));
    if(QSettings().value("transcriptDir").toString()=="")
        fileDialog.setDirectory(QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).value(0, QDir::homePath()));
    else
        fileDialog.setDirectory(QSettings().value("transcriptDir").toString());
    if (fileDialog.exec() == QDialog::Accepted) {
        QUrl *fileUrl = new QUrl(fileDialog.selectedUrls().constFirst());
        m_transcriptUrl = *fileUrl;
        QFile transcriptFile(fileUrl->toLocalFile());
        QFileInfo filedir(transcriptFile);
        QString dirInString=filedir.dir().path();
        QSettings().setValue("transcriptDir",dirInString);
        if (!transcriptFile.open(QIODevice::ReadOnly)) {
            emit message(transcriptFile.errorString());
            return;
        }

        m_saveTimer->stop();

        loadTranscriptData(transcriptFile);

        if (m_transcriptLang == "")
            m_transcriptLang = "english";

        //        loadDictionary();

        setContent();
        //*********************Inserting into file********************************
        QFile initial(fileBeforeSave);
        if(!initial.open(QIODevice::OpenModeFlag::WriteOnly)){
            QMessageBox::critical(this,"Error",initial.errorString());
            return;
        }
        QString x("");
        for (auto& a_block: qAsConst(m_blocks)) {
            auto blockText =  a_block.text + " " ;
            x.append(blockText + "\n");
        }

        QTextStream out(&initial);
        out <<x;
        initial.close();

        //*********************************************************

        if (m_transcriptLang != "")
            emit message("Opened transcript " + fileUrl->fileName() + " Language: " + m_transcriptLang);
        else
            emit message("Opened transcript " + fileUrl->fileName());

        m_saveTimer->start(m_saveInterval * 1000);
    }
}

void Editor::transcriptSave()
{


    if (m_transcriptUrl.isEmpty())
        transcriptSaveAs();
    else {
        auto *file = new QFile(m_transcriptUrl.toLocalFile());
        if (!file->open(QIODevice::WriteOnly | QFile::Truncate)) {
            emit message(file->errorString());
            return;
        }
        saveXml(file);
        emit message("File Saved " + m_transcriptUrl.toLocalFile());
    }


    //QStringList list = str.split(QRegExp("\\s+"), QString::SkipEmptyParts);


    QFile final(fileAfterSave);
    if(!final.open(QIODevice::OpenModeFlag::WriteOnly)){
        QMessageBox::critical(this,"Error",final.errorString());
        return;
    }
    QString x("");
    for (auto& a_block: qAsConst(m_blocks)) {
        auto blockText = a_block.text + " " ;
        //            qInfo()<<a_block.text;
        x.append(blockText + "\n");
    }
    QTextStream outer(&final);
    outer<<x;
    final.close();

    if(!QFile::exists("myalgn.py")){
        QFile mapper("myalign.py");
        QFileInfo mapperFileInfo(mapper);
        QFile aligner(":/alignment.py");
        if(!aligner.open(QIODevice::OpenModeFlag::ReadOnly)){
            QMessageBox::critical(this,"Error",aligner.errorString());
            return;
        }
        aligner.seek(0);
        QString cp=aligner.readAll();
        aligner.close();

        if(!mapper.open(QIODevice::OpenModeFlag::WriteOnly|QIODevice::Truncate)){
            QMessageBox::critical(this,"Error",mapper.errorString());
            return;
        }
        mapper.write(QByteArray(cp.toUtf8()));
        mapper.close();
        std::string makingexec="chmod +x "+mapperFileInfo.absoluteFilePath().replace(" ", "\\ ").toStdString();
        int result = system(makingexec.c_str());
        qInfo()<<result;
    }
    if(!QFile::exists("replacedTextDictonary.json")){
        QFile repDict("replacedTextDictonary.json");
        if(!repDict.open(QIODevice::OpenModeFlag::WriteOnly|QIODevice::Truncate)){
            QMessageBox::critical(this,"Error",repDict.errorString());
            return;
        }
        QString init="{}";
        repDict.write(QByteArray(init.toUtf8()));
        repDict.close();
    }
    QFile mapper("myalign.py");
    QFileInfo mapperFileInfo(mapper);
    QFile repDict("replacedTextDictonary.json");
    QFileInfo repDictFileInfo(repDict);
    QFile finalFile(fileAfterSave);
    QFileInfo finalFileInfo(finalFile);
    QFile initialFile(fileBeforeSave);
    QFileInfo initialDictFileInfo(initialFile);
    int result;
    std::string alignmentstr=" python3 " +mapperFileInfo.absoluteFilePath().replace(" ", "\\ ").toStdString()
                               +" -cae "+initialDictFileInfo.absoluteFilePath().replace(" ", "\\ ").toStdString()
                               +" "+finalFileInfo.absoluteFilePath().replace(" ", "\\ ").toStdString()
                               +" "+repDictFileInfo.absoluteFilePath().replace(" ", "\\ ").toStdString();

    if(!realTimeDataSaver){
        result = system(alignmentstr.c_str());
    }
    qInfo()<<result;

}

void Editor::transcriptSaveAs()
{
    QFileDialog fileDialog(this);
    fileDialog.setAcceptMode(QFileDialog::AcceptSave);
    fileDialog.setWindowTitle(tr("Save Transcript"));
    fileDialog.setDirectory(QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).value(0, QDir::homePath()));
    fileDialog.setDefaultSuffix("xml");

    if (fileDialog.exec() == QDialog::Accepted) {
        auto fileUrl = QUrl(fileDialog.selectedUrls().constFirst());

        if (!document()->isEmpty()) {
            QString filePath = fileUrl.toLocalFile();
            if (!filePath.endsWith(".xml", Qt::CaseInsensitive)) {
                filePath += ".xml";
            }
            auto *file = new QFile(fileUrl.toLocalFile());
            if (!file->open(QIODevice::WriteOnly)) {
                emit message(file->errorString());
                return;
            }
            saveXml(file);
            emit message("File Saved " + fileUrl.toLocalFile());
        }
    }
}

void Editor::transcriptClose()
{
    if (m_transcriptUrl.isEmpty()) {
        emit message("No file open");
        return;
    }


    emit message("Closing file " + m_transcriptUrl.toLocalFile());
    m_transcriptUrl.clear();
    m_blocks.clear();
    m_transcriptLang = "english";

    loadDictionary();
    clear();
}

void Editor::showBlocksFromData()
{
    for (auto& m_block: qAsConst(m_blocks)) {
        qDebug() << m_block.timeStamp << m_block.speaker << m_block.text << m_block.tagList;
        for (auto& m_word: qAsConst(m_block.words)) {
            qDebug() << "   " << m_word.timeStamp << m_word.text << m_word.tagList;
        }
    }
}

void Editor::highlightTranscript(const QTime& elapsedTime)
{
    int blockToHighlight = -1;
    int wordToHighlight = -1;

    if (!m_blocks.isEmpty()) {

        for (int i=0; i < m_blocks.size(); i++) {

            if(m_blocks[i].timeStamp.isValid()){if (m_blocks[i].timeStamp > elapsedTime) {


                    QTextBlock tb = this->document()->findBlockByNumber(i);//block number
                    QString s = tb.text();
                    blockToHighlight=tb.blockNumber();
                    break;
                }}

        }

    }
    //qInfo()<<blockToHighlight;
    if (blockToHighlight != highlightedBlock ) {
        highlightedBlock = blockToHighlight;
        if (!m_highlighter)
            m_highlighter = new Highlighter(document());

        m_highlighter->setBlockToHighlight(blockToHighlight);

        if(moveAlongTimeStamps){
            QTextCursor cursor(this->document()->findBlockByNumber(blockToHighlight));
            this->setTextCursor(cursor);
        }


        if (blockToHighlight == -1)
            return;
    }
    if (blockToHighlight == -1)
        return;
    for (int i = 0; i < m_blocks[blockToHighlight].words.size(); i++) {
        if (m_blocks[blockToHighlight].words[i].timeStamp > elapsedTime) {
            wordToHighlight = i;
            break;
        }
    }

    if (wordToHighlight != highlightedWord) {
        highlightedWord = wordToHighlight;
        m_highlighter->setWordToHighlight(wordToHighlight);
    }

}

void Editor::addCustomDictonary()
{
    QString temp=QFileDialog::getOpenFileName(this,"Open Custom Dictonary",QString("/"),"Text Files (*txt)");
    if(temp.isEmpty()) return;
    m_customDictonaryPath=temp;
    QFile file(m_customDictonaryPath);
    if(!file.open(QIODevice::ReadOnly)){
        QMessageBox::critical(this,"Error",file.errorString());
        return;
    }

    loadDictionary();
}

QTime Editor::getTime(const QString& text)
{
    if (text.contains(".")) {
        if (text.count(":") == 2) return QTime::fromString(text, "h:m:s.z");
        return QTime::fromString(text, "m:s.z");
    }
    else {
        if (text.count(":") == 2) return QTime::fromString(text, "h:m:s");
        return QTime::fromString(text, "m:s");
    }
}

word Editor::makeWord(const QTime& t, const QString& s, const QStringList& tagList)
{
    word w = {t, s, tagList};
    return w;
}

QCompleter* Editor::makeCompleter()
{
    auto completer = new QCompleter(this);
    completer->setWidget(this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setWrapAround(false);
    completer->setCompletionMode(QCompleter::PopupCompletion);

    return completer;
}

block Editor::fromEditor(qint64 blockNumber) const
{
    QTime timeStamp;
    QVector<word> words;
    QString text, speaker, blockText(document()->findBlockByNumber(blockNumber).text());

    QRegularExpressionMatch match = timeStampExp.match(blockText);
    if (match.hasMatch()) {
        QString matchedTimeStampString = match.captured();
        if (blockText.mid(match.capturedEnd()).trimmed() == "") {
            // Get timestamp for string after removing the enclosing []
            timeStamp = getTime(matchedTimeStampString.mid(1,matchedTimeStampString.size() - 2));
            text = blockText.split(matchedTimeStampString)[0];
        }
    }else{
        text = blockText.trimmed();
    }

    match = speakerExp.match(blockText);
    if (match.hasMatch()) {
        speaker = match.captured();
        if (text != "")
            text = text.split(speaker)[1];
        speaker = speaker.left(speaker.size() - 2);
        speaker = speaker.right(speaker.size() - 1);
    }

    if (text == "")
        text = blockText.trimmed();
    else
        text = text.trimmed();

    auto list = text.split(" ");
    for (auto& m_word: qAsConst(list)) {
        words.append(makeWord(QTime(), m_word, QStringList()));
    }

    block b = {timeStamp, text, speaker, QStringList(), words};
    return b;
}

void Editor::loadTranscriptData(QFile& file)
{
    countwords = 0;
    speakercount = 0;
    mySet.clear();
    modset.clear();
    QXmlStreamReader reader(&file);
    m_transcriptLang = "";
    m_blocks.clear();
    if (reader.readNextStartElement()) {
        if (reader.name() == "transcript") {
            m_transcriptLang = reader.attributes().value("lang").toString();

            while(reader.readNextStartElement()) {
                if(reader.name() == "line") {
                    QString t1=reader.attributes().value("timestamp").toString();
                    QStringList tl=t1.split(":");
                    auto blockTimeStamp = getTime(reader.attributes().value("timestamp").toString());

                    if(!blockTimeStamp.isValid()){
                        QString t2="";
                        if(t1.count(":")==1){
                            int hr=tl[0].toInt()/60;
                            if(hr<10){
                                t2+="0";
                                t2+=QString::number(hr);
                            }
                            else
                                t2+=QString::number(hr);

                            t2+=":";
                            t2+=QString::number(tl[0].toInt()%60);
                            t2+=":";
                            t2+=tl[1];
                            blockTimeStamp=getTime(t2);
                        }
                        else if(t1.count(":")==2){
                            int hr=(tl[1].toInt()/60)+tl[0].toInt();

                            if(hr<10){
                                t2+="0";
                                t2+=QString::number(hr);
                            }
                            else
                                t2+=QString::number(hr);

                            t2+=":";
                            t2+=QString::number(tl[1].toInt()%60);
                            t2+=":";
                            t2+=tl[2];
                            blockTimeStamp=getTime(t2);
                        }
                    }

                    auto blockText = QString("");
                    auto blockSpeaker = reader.attributes().value("speaker").toString();
                    mySet.insert(blockSpeaker);
                    modset.insert(blockSpeaker);
                    speakercount+=1;
                    auto tagString = reader.attributes().value("tags").toString();
                    QStringList tagList;
                    if (tagString != "")
                        tagList = tagString.split(",");

                    struct block line = {blockTimeStamp, "", blockSpeaker, tagList, QVector<word>()};
                    while(reader.readNextStartElement()){
                        if(reader.name() == "word"){
                            auto wordTimeStamp  = getTime(reader.attributes().value("timestamp").toString());
                            auto wordTagString  = reader.attributes().value("tags").toString();
                            auto wordText       = reader.readElementText();
                            QStringList wordTagList;
                            if (wordTagString != "")
                                wordTagList = wordTagString.split(",");

                            blockText += (wordText + " ");
                            countwords+=1;
                            line.words.append(makeWord(wordTimeStamp, wordText, wordTagList));
                        }
                        else
                            reader.skipCurrentElement();
                    }
                    line.text = blockText.trimmed();
                    m_blocks.append(line);
                }
                else
                    reader.skipCurrentElement();
            }
        }
        else
            reader.raiseError(QObject::tr("Incorrect file"));
    }
    modifiedwords = countwords;
}

void Editor::saveXml(QFile* file)
{
    QXmlStreamWriter writer(file);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement("transcript");

    if (m_transcriptLang != "")
        writer.writeAttribute("lang", m_transcriptLang);

    for (auto& a_block: qAsConst(m_blocks)) {
        if (a_block.text != "") {
            qDebug() << a_block.text;
            auto timeStamp = a_block.timeStamp;
            QString timeStampString = timeStamp.toString("hh:mm:ss.zzz");
            auto speaker = a_block.speaker;

            if (!a_block.tagList.isEmpty())
                writer.writeAttribute("tags", a_block.tagList.join(","));

            writer.writeStartElement("line");
            writer.writeAttribute("timestamp", timeStampString);
            writer.writeAttribute("speaker", speaker);
            for (auto& a_word: qAsConst(a_block.words)) {
                writer.writeStartElement("word");
                writer.writeAttribute("timestamp", a_word.timeStamp.toString("hh:mm:ss.zzz"));

                if (!a_word.tagList.isEmpty())
                    writer.writeAttribute("tags", a_word.tagList.join(","));

                writer.writeCharacters(a_word.text);
                writer.writeEndElement();

                if(a_word.text.contains('.')==true)
                {
                    break;
                }
            }
            writer.writeEndElement();
        }

    }
    writer.writeEndElement();
    file->close();
    delete file;
}

void Editor::helpJumpToPlayer()
{
    auto currentBlockNumber = textCursor().blockNumber();
    auto timeToJump = QTime(0, 0);

    if (m_blocks[currentBlockNumber].timeStamp.isNull())
        return;

    int positionInBlock = textCursor().positionInBlock();
    auto blockText = textCursor().block().text();
    auto textBeforeCursor = blockText.left(positionInBlock);
    int wordNumber = textBeforeCursor.count(" ");
    if (m_blocks[currentBlockNumber].speaker != "" || textCursor().block().text().contains("[]:"))
        wordNumber--;

    for (int i = currentBlockNumber - 1; i >= 0; i--) {
        if (m_blocks[i].timeStamp.isValid()) {

            timeToJump = m_blocks[i].timeStamp;
            break;
        }
    }

    // If we can jump to a word, then do so
    if (wordNumber >= 0 &&
        wordNumber < m_blocks[currentBlockNumber].words.size() &&
        m_blocks[currentBlockNumber].words[wordNumber].timeStamp.isValid()
        ) {
        for (int i = wordNumber - 1; i >= 0; i--) {
            if (m_blocks[currentBlockNumber].words[i].timeStamp.isValid()) {
                timeToJump = m_blocks[currentBlockNumber].words[i].timeStamp;
                emit jumpToPlayer(timeToJump);
                return;
            }
        }
    }
    qInfo()<<timeToJump;
    emit jumpToPlayer(timeToJump);
}

QStringList getGoogleSuggestions(const QString& word);

void Editor::loadDictionary()
{
    m_correctedWords.clear();
    m_dictionary.clear();
    if(QFile::exists("Dictonaries/"+m_transcriptLang+"/"+m_transcriptLang+"combined.txt")){
        auto dictionaryFileName = "Dictonaries/"+m_transcriptLang+"/"+m_transcriptLang+"combined.txt";
        m_dictionary = listFromFile(dictionaryFileName);
        //        qInfo()<<m_dictionary;

    }
    else{
        auto dictionaryFileName = QString(":/wordlists/%1.txt").arg(m_transcriptLang);
        m_dictionary = listFromFile(dictionaryFileName);
    }
    if(m_customDictonaryPath!=NULL){
        auto customdictionaryFileName = QString(m_customDictonaryPath);
        auto wordsFromCustomDictonary=listFromFile(customdictionaryFileName);

        for (auto& word : wordsFromCustomDictonary) {
            word = word.toLower();
        }

        auto combined_dictionary=m_dictionary;
        combined_dictionary.append(wordsFromCustomDictonary);
        combined_dictionary.sort();
        QDir dictonaryFolder("Dictonaries");
        if(!dictonaryFolder.exists()){
            dictonaryFolder.mkpath(".");
        }
        QDir languageFolder("Dictonaries/"+m_transcriptLang);
        if(!languageFolder.exists()){
            languageFolder.mkpath(".");
        }
        QDir dir("Dictonaries/"+m_transcriptLang+"/"+m_transcriptLang+"combined.txt");

        QFile file2(dir.absolutePath());
        if(!file2.open(QIODevice::OpenModeFlag::WriteOnly|QIODevice::Truncate)){
            QMessageBox::critical(this,"Error",file2.errorString());
            return;
        }
        QString a=dir.absolutePath();
        QString x("");
        for(auto i: combined_dictionary){
            x.append(i+"\n");
        }
        file2.write(QByteArray(x.toUtf8()));
        file2.flush();
        file2.close();
        auto combinedCustomdictionaryFileName = QString(a);
        m_dictionary=listFromFile(combinedCustomdictionaryFileName);
        //         qInfo()<<a<<'\n';
    }
    // qInfo()<<m_dictionary<<'\n';
    auto correctedWordsList = listFromFile(QString("corrected_words_%1.txt").arg(m_transcriptLang));
    if (!correctedWordsList.isEmpty()) {
        std::copy(correctedWordsList.begin(),
                  correctedWordsList.end(),
                  std::inserter(m_correctedWords, m_correctedWords.begin()));

        for (auto& a_word: m_correctedWords) {
            m_dictionary.insert
                (
                    std::upper_bound(m_dictionary.begin(), m_dictionary.end(), a_word),
                    a_word
                    );
        }
    }
    m_textCompleter->setModel(new QStringListModel(m_dictionary, m_textCompleter));

    if (!m_highlighter)
        return;

    QMultiMap<int, int> invalidWords;
    for (int i = 0; i < m_blocks.size(); i++) {

        for (int j = 0; j < m_blocks[i].words.size(); j++) {
            auto wordText = m_blocks[i].words[j].text.toLower();

            if (wordText != "" && m_punctuation.contains(wordText.back()))
                wordText = wordText.left(wordText.size() - 1);

            //            if (!std::binary_search(m_dictionary.begin(),
            //                                    m_dictionary.end(),
            //                                    wordText)
            //                )
            //                invalidWords.insert(i, j);
            // Use Google Suggest API to get suggested corrections
            QStringList suggestions = getGoogleSuggestions(wordText);
            if (!suggestions.isEmpty()) {
                // If suggestions are available, replace the invalid word with the first suggestion
                m_blocks[i].words[j].text = suggestions.first();
            } else {
                // If no suggestions are available, consider the word as invalid
                invalidWords.insert(i, j);
            }
        }
    }
    m_highlighter->setInvalidWords(invalidWords);
    m_highlighter->rehighlight();
}

QStringList Editor::getGoogleSuggestions(const QString& word)
{
    QString apiUrl = "https://suggestqueries.google.com/complete/search";
    QNetworkAccessManager manager;
    QNetworkRequest request(apiUrl);

    QUrlQuery query;
    query.addQueryItem("client", "firefox");
    query.addQueryItem("q", word);
    request.setUrl(QUrl(apiUrl + "?" + query.query()));

    QNetworkReply* reply = manager.get(request);
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    QStringList suggestions;

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
        QJsonArray suggestionsArray = jsonDoc[1].toArray();

        for (const QJsonValue& suggestion : suggestionsArray) {
            suggestions.append(suggestion.toString());
        }
    }

    reply->deleteLater();

    return suggestions;
}

QStringList Editor::listFromFile(const QString& fileName)
{
    QStringList words;

    QFile file(fileName);
    if (!file.open(QFile::ReadOnly))
        return {};

    while (!file.atEnd()) {
        QByteArray line = file.readLine();
        if (!line.isEmpty())
            words << QString::fromUtf8(line.trimmed());
    }

    return words;
}



void Editor::setShowTimeStamp()
{


    if(showTimeStamp){
        showTimeStamp=false;}
    else{
        showTimeStamp=true;
    }

    setContent();
    QTextCursor cursorx(this->document()->findBlockByNumber(highlightedBlock));
    this->setTextCursor(cursorx);

    qInfo()<<"size of m_blocks is: "<<m_blocks.size();
    qInfo()<<"\ntime stamp of m_block 1 is: "<<m_blocks[1].timeStamp;

    QVector<QTime> timevec;
    QTime t;
    for(int i = 0; i < m_blocks.size(); ++i)
    {
        t = m_blocks[i].timeStamp;
        timevec.append(t);
    }

    emit sendBlockTime(timevec);

}

void Editor::setContent()
{
    if (!settingContent) {
        settingContent = true;

        if (m_highlighter)
            delete m_highlighter;

        QString content_with_time_stamp("");
        QString content_without_time_stamp("");
        QString content_without_speaker("");
        for (auto& a_block: qAsConst(m_blocks)) {
            auto blockText = a_block.text + " [" + a_block.timeStamp.toString("hh:mm:ss.zzz") + "]";
            content_without_speaker.append(blockText + "\n");
        }

        for (auto& a_block: qAsConst(m_blocks)) {
            auto blockText = "[" + a_block.speaker + "]: " + a_block.text + " [" + a_block.timeStamp.toString("hh:mm:ss.zzz") + "]";
            content_with_time_stamp.append(blockText + "\n");
        }

        for (auto& a_block: qAsConst(m_blocks)) {
            auto blockText = "[" + a_block.speaker + "]: " + a_block.text ;
            content_without_time_stamp.append(blockText + "\n");
        }

        if(removespeakerbool){
            setPlainText(content_without_speaker.trimmed());
            removespeakerbool = false;
        }
        else if(showTimeStamp){
            setPlainText(content_with_time_stamp.trimmed());
            showTimeStamp = false;
        }
        else if(removetimebool){
            qInfo()<<"Entered";
            setPlainText(content_without_time_stamp.trimmed());
        }
        else{
            setPlainText(content_without_time_stamp.trimmed());
        }

        m_highlighter = new Highlighter(document());

        QList<int> invalidBlocks;
        QList<int> taggedBlocks;
        QMultiMap<int, int> invalidWords;
        QMultiMap<int, int>  taggedWords;
        for (int i = 0; i < m_blocks.size(); i++) {
            if (m_blocks[i].timeStamp.isNull())
                invalidBlocks.append(i);
            else if(!m_blocks[i].tagList.isEmpty()){
                taggedBlocks.append(i);
            }
            else {

                for (int j = 0; j < m_blocks[i].words.size(); j++) {
                    auto wordText = m_blocks[i].words[j].text.toLower();

                    if (wordText != "" && m_punctuation.contains(wordText.back()))
                        wordText = wordText.left(wordText.size() - 1);

                    if (wordText != "" && wordText[0]=='\"'){

                        QString text="";
                        for(int i=1;i<wordText.size();i++){
                            text+=wordText[i];
                        }
                        wordText=text;
                    }
                    if (wordText != "" && wordText[wordText.size()-1]=='\"'){
                        wordText = wordText.left(wordText.size() - 1);

                    }
                    if (wordText != "" && wordText[0]=='('){

                        QString text="";
                        for(int i=1;i<wordText.size();i++){
                            text+=wordText[i];
                        }
                        wordText=text;
                    }
                    if (wordText != "" && wordText[wordText.size()-1]==')'){
                        wordText = wordText.left(wordText.size() - 1);

                    }
                    if (wordText != "" && wordText[0]=='['){

                        QString text="";
                        for(int i=1;i<wordText.size();i++){
                            text+=wordText[i];
                        }
                        wordText=text;
                    }
                    if (wordText != "" && wordText[wordText.size()-1]==']'){
                        wordText = wordText.left(wordText.size() - 1);
                        qInfo()<<wordText;
                    }
                    if (wordText != "" && wordText[0]=='{'){
                        QString text="";
                        for(int i=1;i<wordText.size();i++){
                            text+=wordText[i];
                        }
                        wordText=text;
                    }
                    if (wordText != "" &&wordText[wordText.size()-1]=='}'){
                        wordText = wordText.left(wordText.size() - 1);

                    }
                    if (wordText != "" && wordText[0]=="\'"){

                        QString text="";
                        for(int i=1;i<wordText.size();i++){
                            text+=wordText[i];
                        }
                        wordText=text;
                    }
                    if (wordText != "" && wordText[wordText.size()-1]=="\'"){
                        wordText = wordText.left(wordText.size() - 1);

                    }
                    if (wordText != "" && wordText[0]=="<"){

                        QString text="";
                        for(int i=1;i<wordText.size();i++){
                            text+=wordText[i];
                        }
                        wordText=text;
                    }
                    if (wordText != "" && wordText[wordText.size()-1]==">"){
                        wordText = wordText.left(wordText.size() - 1);

                    }

                    if (wordText != "" && wordText[wordText.size()-1]=="?"){
                        wordText = wordText.left(wordText.size() - 1);

                    }
                    if (wordText != "" && wordText[wordText.size()-1]=="!"){
                        wordText = wordText.left(wordText.size() - 1);

                    }
                    if (wordText != "" && wordText[wordText.size()-1]==","){
                        wordText = wordText.left(wordText.size() - 1);

                    }
                    QRegExp regex("([0-1][0-9]|2[0-3]):([0-5][0-9]):([0-5][0-9])(\\.[0-9]+)?");
                    bool match = regex.exactMatch(wordText);
                    if (match) {
                        continue;
                        // the string is a valid time in the format "HH:MM:SS.f"
                    }

                    if (!std::binary_search(m_dictionary.begin(),
                                            m_dictionary.end(),
                                            wordText)
                        ){

                        invalidWords.insert(i, j);
                    }
                    if(!m_blocks[i].words[j].tagList.empty()){
                        taggedWords.insert(i,j);
                    }
                }
            }
        }

        m_highlighter->setInvalidBlocks(invalidBlocks);
        m_highlighter->setTaggedBlocks(taggedBlocks);
        m_highlighter->setInvalidWords(invalidWords);
        m_highlighter->setTaggedWords(taggedWords);
        m_highlighter->setBlockToHighlight(highlightedBlock);
        m_highlighter->setWordToHighlight(highlightedWord);

        settingContent = false;
    }
}

bool Editor::timestampVisibility()
{
    return showTimeStamp;
}

void Editor::contentChanged(int position, int charsRemoved, int charsAdded)
{
    // If chars aren't added or deleted then return
    if (!(charsAdded || charsRemoved) || settingContent)
        return;
    else if (m_blocks.isEmpty()) { // If block data is empty (i.e. no file opened) just fill them from editor
        for (int i = 0; i < document()->blockCount(); i++)
            m_blocks.append(fromEditor(i));
        return;
    }

    delete m_highlighter;
    m_highlighter = new Highlighter(this->document());

    int currentBlockNumber = textCursor().blockNumber();

    if(m_blocks.size() != blockCount()) {
        auto blocksChanged = m_blocks.size() - blockCount();
        if (blocksChanged > 0) { // Blocks deleted
            qInfo() << "[Lines Deleted]" << QString("%1 lines deleted").arg(QString::number(blocksChanged));
            for (int i = 1; i <= blocksChanged; i++)
                m_blocks.removeAt(currentBlockNumber + 1);
        }
        else { // Blocks added
            qInfo() << "[Lines Inserted]" << QString("%1 lines inserted").arg(QString::number(-blocksChanged));
            for (int i = 1; i <= -blocksChanged; i++) {
                if (document()->findBlockByNumber(currentBlockNumber + blocksChanged).text().trimmed() == "")
                    m_blocks.insert(currentBlockNumber + blocksChanged, fromEditor(currentBlockNumber - i));
                else
                    m_blocks.insert(currentBlockNumber + blocksChanged + 1, fromEditor(currentBlockNumber - i + 1));
            }
        }
    }

    auto currentBlockFromEditor = fromEditor(currentBlockNumber);
    auto& currentBlockFromData = m_blocks[currentBlockNumber];

    if (currentBlockFromData.speaker != currentBlockFromEditor.speaker) {
        qInfo() << "[Speaker Changed]"
                << QString("line number: %1").arg(QString::number(currentBlockNumber + 1))
                << QString("initial: %1").arg(currentBlockFromData.speaker)
                << QString("final: %1").arg(currentBlockFromEditor.speaker);

        currentBlockFromData.speaker = currentBlockFromEditor.speaker;
        modset.insert(currentBlockFromEditor.speaker);
    }
    if(showTimeStamp){
        if (currentBlockFromData.timeStamp != currentBlockFromEditor.timeStamp) {

            currentBlockFromData.timeStamp = currentBlockFromEditor.timeStamp;
            qInfo() << "[TimeStamp Changed]"
                    << QString("line number: %1, %2").arg(QString::number(currentBlockNumber + 1), currentBlockFromEditor.timeStamp.toString("hh:mm:ss.zzz"));

        }
    }
    if (currentBlockFromData.text != currentBlockFromEditor.text) {
        qInfo() << "[Text Changed]"
                << QString("line number: %1").arg(QString::number(currentBlockNumber + 1))
                << QString("initial: %1").arg(currentBlockFromData.text)
                << QString("final: %1").arg(currentBlockFromEditor.text);

        currentBlockFromData.text = currentBlockFromEditor.text;
        auto tagList = currentBlockFromData.tagList;

        auto& wordsFromEditor = currentBlockFromEditor.words;
        auto& wordsFromData = currentBlockFromData.words;



        int wordsDifference = wordsFromEditor.size() - wordsFromData.size();

        modifiedwords = modifiedwords+wordsDifference;

        int diffStart{-1}, diffEnd{-1};

        for (int i = 0; i < wordsFromEditor.size() && i < wordsFromData.size(); i++)
            if (wordsFromEditor[i].text != wordsFromData[i].text) {
                diffStart = i;
                break;
            }

        if (diffStart == -1)
            diffStart = wordsFromEditor.size() - 1;
        for (int i = 0; i <= diffStart; i++)
            if (i < wordsFromData.size()){
                wordsFromEditor[i].timeStamp = wordsFromData[i].timeStamp;
                //                wordsFromEditor[i].tagList = wordsFromData[i].tagList;
            }

        if (!wordsDifference) {
            for (int i = diffStart; i < wordsFromEditor.size(); i++){
                wordsFromEditor[i].timeStamp = wordsFromData[i].timeStamp;
                wordsFromEditor[i].tagList = wordsFromData[i].tagList;
            }
        }

        if (wordsDifference > 0) {
            qInfo()<<"exceeds";
            for (int i = wordsFromEditor.size() - 1, j = wordsFromData.size() - 1; j > diffStart; i--, j--)
                if (wordsFromEditor[i].text == wordsFromData[j].text){
                    wordsFromEditor[i].timeStamp = wordsFromData[j].timeStamp;
                    //                    wordsFromEditor[i].tagList = wordsFromData[j].tagList;
                }
            for (int i=wordsFromData.size()-1;i>=0;i--){
                if(!wordsFromData[i].tagList.empty()){
                    for (int j=wordsFromEditor.size()-1;j>=0;j--){
                        if (wordsFromEditor[j].text == wordsFromData[i].text){
                            if(wordsFromEditor[j].tagList.empty())
                                wordsFromEditor[j].tagList = wordsFromData[i].tagList;
                        }
                    }
                }
            }
        }
        else if (wordsDifference < 0) {
            for (int i = wordsFromEditor.size() - 1, j = wordsFromData.size() - 1; i > diffStart; i--, j--)
                if (wordsFromEditor[i].text == wordsFromData[j].text){
                    wordsFromEditor[i].timeStamp = wordsFromData[j].timeStamp;
                    //                    wordsFromEditor[i].tagList = wordsFromData[j].tagList;
                }
            for (int i=wordsFromData.size()-1;i>=0;i--){
                if(!wordsFromData[i].tagList.empty()){
                    for (int j=wordsFromEditor.size()-1;j>=0;j--){
                        if (wordsFromEditor[j].text == wordsFromData[i].text){
                            if(wordsFromEditor[j].tagList.empty())
                                wordsFromEditor[j].tagList = wordsFromData[i].tagList;
                        }
                    }
                }
            }
        }

        //currentBlockFromData = currentBlockFromEditor;
        if(showTimeStamp)
            currentBlockFromData = currentBlockFromEditor;
        else{
            currentBlockFromData.speaker = currentBlockFromEditor.speaker;
            currentBlockFromData.text = currentBlockFromEditor.text;
            currentBlockFromData.words = currentBlockFromEditor.words;
            currentBlockFromData.tagList = currentBlockFromEditor.tagList;
        }

        currentBlockFromData.tagList = tagList;
    }

    m_highlighter->setBlockToHighlight(highlightedBlock);
    m_highlighter->setWordToHighlight(highlightedWord);

    QList<int> invalidBlocks;
    QList<int> taggedBlocks;
    QMultiMap<int, int> invalidWords;
    QMultiMap<int, int>  taggedWords;
    for (int i = 0; i < m_blocks.size(); i++) {
        if (m_blocks[i].timeStamp.isNull())
            invalidBlocks.append(i);
        else if(!m_blocks[i].tagList.isEmpty()){
            taggedBlocks.append(i);
        }
        else {
            for (int j = 0; j < m_blocks[i].words.size(); j++) {
                auto wordText = m_blocks[i].words[j].text.toLower();

                if (wordText != "" && m_punctuation.contains(wordText.back()))
                    wordText = wordText.left(wordText.size() - 1);

                if (wordText != "" && wordText[0]=='\"'){

                    QString text="";
                    for(int i=1;i<wordText.size();i++){
                        text+=wordText[i];
                    }
                    wordText=text;
                }
                if (wordText != "" && wordText[wordText.size()-1]=='\"'){
                    wordText = wordText.left(wordText.size() - 1);

                }
                if (wordText != "" && wordText[0]=='('){

                    QString text="";
                    for(int i=1;i<wordText.size();i++){
                        text+=wordText[i];
                    }
                    qInfo()<<text;
                    wordText=text;
                }
                if (wordText != "" && wordText[wordText.size()-1]==')'){
                    wordText = wordText.left(wordText.size() - 1);

                }
                if (wordText != "" && wordText[0]=='['){

                    QString text="";
                    for(int i=1;i<wordText.size();i++){
                        text+=wordText[i];
                    }
                    wordText=text;
                }
                if (wordText != "" && wordText[wordText.size()-1]==']'){
                    wordText = wordText.left(wordText.size() - 1);

                }
                if (wordText != "" && wordText[0]=='{'){
                    QString text="";
                    for(int i=1;i<wordText.size();i++){
                        text+=wordText[i];
                    }
                    wordText=text;
                }
                if (wordText != "" &&wordText[wordText.size()-1]=='}'){
                    wordText = wordText.left(wordText.size() - 1);

                }
                if (wordText != "" && wordText[0]=="\'"){

                    QString text="";
                    for(int i=1;i<wordText.size();i++){
                        text+=wordText[i];
                    }
                    wordText=text;
                }
                if (wordText != "" && wordText[wordText.size()-1]=="\'"){
                    wordText = wordText.left(wordText.size() - 1);

                }
                if (wordText != "" && wordText[0]=="<"){

                    QString text="";
                    for(int i=1;i<wordText.size();i++){
                        text+=wordText[i];
                    }
                    wordText=text;
                }
                if (wordText != "" && wordText[wordText.size()-1]==">"){
                    wordText = wordText.left(wordText.size() - 1);

                }

                if (wordText != "" && wordText[wordText.size()-1]=="?"){
                    wordText = wordText.left(wordText.size() - 1);

                }
                if (wordText != "" && wordText[wordText.size()-1]=="!"){
                    wordText = wordText.left(wordText.size() - 1);

                }
                if (wordText != "" && wordText[wordText.size()-1]==","){
                    wordText = wordText.left(wordText.size() - 1);

                }
                QRegExp regex("([0-1][0-9]|2[0-3]):([0-5][0-9]):([0-5][0-9])(\\.[0-9]+)?");
                bool match = regex.exactMatch(wordText);
                if (match) {
                    continue;
                    // the string is a valid time in the format "HH:MM:SS.f"
                }

                if (!std::binary_search(m_dictionary.begin(),
                                        m_dictionary.end(),
                                        wordText)
                    )
                    invalidWords.insert(i, j);
                if(!m_blocks[i].words[j].tagList.empty()){
                    taggedWords.insert(i,j);
                }
            }

        }
    }
    m_highlighter->setInvalidBlocks(invalidBlocks);
    m_highlighter->setTaggedBlocks(taggedBlocks);
    m_highlighter->setInvalidWords(invalidWords);
    m_highlighter->setTaggedWords(taggedWords);
    updateWordEditor();
    if(realTimeDataSaver){
        transcriptSave();
    }
}

void Editor::jumpToHighlightedLine()
{
    if (highlightedBlock == -1)
        return;
    QTextCursor cursor(document()->findBlockByNumber(highlightedBlock));
    setTextCursor(cursor);
}

void Editor::splitLine(const QTime& elapsedTime)
{
    //    auto cursor = textCursor();
    //    if (cursor.blockNumber() != highlightedBlock)
    //        return;

    //    int positionInBlock = cursor.positionInBlock();
    //    auto blockText = cursor.block().text();
    //    auto textBeforeCursor = blockText.left(positionInBlock);
    //    auto textAfterCursor = blockText.right(blockText.size() - positionInBlock);
    //    auto cutWordLeft = textBeforeCursor.split(" ").last();
    //    auto cutWordRight = textAfterCursor.split(" ").first();
    //    int wordNumber = textBeforeCursor.count(" ");

    //    if (m_blocks[highlightedBlock].speaker != "" || blockText.contains("[]:"))
    //        wordNumber--;
    //    if (wordNumber < 0 || wordNumber >= m_blocks[highlightedBlock].words.size())
    //        return;

    //    if (textBeforeCursor.contains("]:"))
    //        textBeforeCursor = textBeforeCursor.split("]:").last();
    //    if (textAfterCursor.contains("["))
    //        textAfterCursor = textAfterCursor.split("[").first();

    //    auto timeStampOfCutWord = m_blocks[highlightedBlock].words[wordNumber].timeStamp;
    //    auto tagsOfCutWord = m_blocks[highlightedBlock].words[wordNumber].tagList;
    //    QVector<word> words;
    //    int sizeOfWordsAfter = m_blocks[highlightedBlock].words.size() - wordNumber - 1;

    //    if (cutWordRight != "")
    //        words.append(makeWord(timeStampOfCutWord, cutWordRight, tagsOfCutWord));
    //    for (int i = 0; i < sizeOfWordsAfter; i++) {
    //        words.append(m_blocks[highlightedBlock].words[wordNumber + 1]);
    //        m_blocks[highlightedBlock].words.removeAt(wordNumber + 1);
    //    }

    //    if (cutWordLeft == "")
    //        m_blocks[highlightedBlock].words.removeAt(wordNumber);
    //    else {
    //        m_blocks[highlightedBlock].words[wordNumber].text = cutWordLeft;
    //        m_blocks[highlightedBlock].words[wordNumber].timeStamp = elapsedTime;
    //    }

    //    block blockToInsert = {m_blocks[highlightedBlock].timeStamp,
    //                           textAfterCursor.trimmed(),
    //                           m_blocks[highlightedBlock].speaker,
    //                           m_blocks[highlightedBlock].tagList,
    //                           words};
    //    m_blocks.insert(highlightedBlock + 1, blockToInsert);

    //    m_blocks[highlightedBlock].text = textBeforeCursor.trimmed();
    //    m_blocks[highlightedBlock].timeStamp = elapsedTime;

    //    setContent();
    //    updateWordEditor();

    //    QTextCursor cursorx(this->document()->findBlockByNumber(highlightedBlock));
    //    this->setTextCursor(cursorx);

    //    qInfo() << "[Line Split]"
    //            << QString("line number: %1").arg(QString::number(highlightedBlock + 1))
    //            << QString("word number: %1, %2").arg(QString::number(wordNumber + 1), cutWordLeft + cutWordRight);

    auto cursor = textCursor();

    int positionInBlock = cursor.positionInBlock();
    auto blockText = cursor.block().text();
    auto textBeforeCursor = blockText.left(positionInBlock);
    auto textAfterCursor = blockText.right(blockText.size() - positionInBlock);
    auto cutWordLeft = textBeforeCursor.split(" ").last();
    auto cutWordRight = textAfterCursor.split(" ").first();
    int wordNumber = textBeforeCursor.count(" ");

    if (m_blocks[cursor.blockNumber()].speaker != "" || blockText.contains("[]:"))
        wordNumber--;
    if (wordNumber < 0 || wordNumber >= m_blocks[cursor.blockNumber()].words.size())
        return;

    if (textBeforeCursor.contains("]:"))
        textBeforeCursor = textBeforeCursor.split("]:").last();
    if (textAfterCursor.contains("["))
        textAfterCursor = textAfterCursor.split("[").first();

    auto timeStampOfCutWord = m_blocks[cursor.blockNumber()].words[wordNumber].timeStamp;
    auto tagsOfCutWord = m_blocks[cursor.blockNumber()].words[wordNumber].tagList;
    QVector<word> words;
    int sizeOfWordsAfter = m_blocks[cursor.blockNumber()].words.size() - wordNumber - 1;

    if (cutWordRight != "")
        words.append(makeWord(timeStampOfCutWord, cutWordRight, tagsOfCutWord));
    for (int i = 0; i < sizeOfWordsAfter; i++) {
        words.append(m_blocks[cursor.blockNumber()].words[wordNumber + 1]);
        m_blocks[cursor.blockNumber()].words.removeAt(wordNumber + 1);
    }

    if (cutWordLeft == "")
        m_blocks[cursor.blockNumber()].words.removeAt(wordNumber);
    else {
        m_blocks[cursor.blockNumber()].words[wordNumber].text = cutWordLeft;
        m_blocks[cursor.blockNumber()].words[wordNumber].timeStamp = elapsedTime;
    }

    block blockToInsert = {m_blocks[cursor.blockNumber()].timeStamp,
                           textAfterCursor.trimmed(),
                           m_blocks[cursor.blockNumber()].speaker,
                           m_blocks[cursor.blockNumber()].tagList,
                           words};
    m_blocks.insert(cursor.blockNumber() + 1, blockToInsert);

    m_blocks[cursor.blockNumber()].text = textBeforeCursor.trimmed();
    m_blocks[cursor.blockNumber()].timeStamp = elapsedTime;

    setContent();
    updateWordEditor();

    qInfo() << "[Line Split]"
            << QString("line number: %1").arg(QString::number(highlightedBlock + 1))
            << QString("word number: %1, %2").arg(QString::number(wordNumber + 1), cutWordLeft + cutWordRight);
}

void Editor::mergeUp()
{
    auto blockNumber = textCursor().blockNumber();
    auto previousBlockNumber = blockNumber - 1;

    if (m_blocks.isEmpty() || blockNumber == 0 || m_blocks[blockNumber].speaker != m_blocks[previousBlockNumber].speaker)
        return;

    auto currentWords = m_blocks[blockNumber].words;

    m_blocks[previousBlockNumber].words.append(currentWords);                 // Add current words to previous block
    m_blocks[previousBlockNumber].timeStamp = m_blocks[blockNumber].timeStamp;  // Update time stamp of previous block
    m_blocks[previousBlockNumber].text.append(" " + m_blocks[blockNumber].text);// Append text to previous block

    m_blocks.removeAt(blockNumber);
    setContent();
    updateWordEditor();

    QTextCursor cursor(document()->findBlockByNumber(previousBlockNumber));
    setTextCursor(cursor);
    centerCursor();

    qInfo() << "[Merge Up]"
            << QString("line number: %1, %2").arg(QString::number(previousBlockNumber + 1), QString::number(blockNumber + 1))
            << QString("final line: %1, %2").arg(QString::number(previousBlockNumber + 1), m_blocks[previousBlockNumber].text);
    //    qInfo()<<(undoS);
    qDebug() << this->findChildren<QUndoStack*>();

}

void Editor::mergeDown()
{
    auto blockNumber = textCursor().blockNumber();
    auto nextBlockNumber = blockNumber + 1;

    if (m_blocks.isEmpty() || blockNumber == m_blocks.size() - 1 || m_blocks[blockNumber].speaker != m_blocks[nextBlockNumber].speaker)
        return;

    auto currentWords = m_blocks[blockNumber].words;

    auto temp = m_blocks[nextBlockNumber].words;
    m_blocks[nextBlockNumber].words = currentWords;
    m_blocks[nextBlockNumber].words.append(temp);

    auto tempText = m_blocks[nextBlockNumber].text;
    m_blocks[nextBlockNumber].text = m_blocks[blockNumber].text;
    m_blocks[nextBlockNumber].text.append(" " + tempText);

    m_blocks.removeAt(blockNumber);
    setContent();
    updateWordEditor();

    QTextCursor cursor(document()->findBlockByNumber(blockNumber));
    setTextCursor(cursor);
    centerCursor();

    qInfo() << "[Merge Down]"
            << QString("line number: %1, %2").arg(QString::number(blockNumber + 1), QString::number(nextBlockNumber + 1))
            << QString("final line: %1, %2").arg(QString::number(blockNumber + 1), m_blocks[nextBlockNumber].text);
}

void Editor::createChangeSpeakerDialog()
{
    if (m_blocks.isEmpty())
        return;

    m_changeSpeaker = new ChangeSpeakerDialog(this);
    m_changeSpeaker->setModal(true);
    m_changeSpeaker->setAttribute(Qt::WA_DeleteOnClose);

    QSet<QString> speakers;
    for (auto& a_block: qAsConst(m_blocks))
        speakers.insert(a_block.speaker);

    m_changeSpeaker->addItems(speakers.values());
    m_changeSpeaker->setCurrentSpeaker(m_blocks.at(textCursor().blockNumber()).speaker);

    connect(m_changeSpeaker,
            &ChangeSpeakerDialog::accepted,
            this,
            [&]() {
                changeSpeaker(m_changeSpeaker->speaker(), m_changeSpeaker->replaceAll());
            }
            );
    m_changeSpeaker->show();
}

void Editor::createTimePropagationDialog()
{
    if (m_blocks.isEmpty())
        return;

    m_propagateTime = new TimePropagationDialog(this);
    m_propagateTime->setModal(true);
    m_propagateTime->setAttribute(Qt::WA_DeleteOnClose);

    m_propagateTime->setBlockRange(textCursor().blockNumber() + 1, blockCount());

    connect(m_propagateTime,
            &TimePropagationDialog::accepted,
            this,
            [&]() {
                propagateTime(m_propagateTime->time(),
                              m_propagateTime->blockStart(),
                              m_propagateTime->blockEnd(),
                              m_propagateTime->negateTime());
            }
            );
    m_propagateTime->show();
}

void Editor::createTagSelectionDialog()
{
    if (m_blocks.isEmpty())
        return;

    m_selectTag = new TagSelectionDialog(this);
    m_selectTag->setModal(true);
    m_selectTag->setAttribute(Qt::WA_DeleteOnClose);

    m_selectTag->markExistingTags(m_blocks[textCursor().blockNumber()].tagList);

    connect(m_selectTag,
            &TagSelectionDialog::accepted,
            this,
            [&]() {
                selectTags(m_selectTag->tagList());
            }
            );
    m_selectTag->show();
}

void Editor::insertTimeStamp(const QTime& elapsedTime)
{
    auto blockNumber = textCursor().blockNumber();

    if (m_blocks.size() <= blockNumber)
        return;

    m_blocks[blockNumber].timeStamp = elapsedTime;

    dontUpdateWordEditor = true;
    setContent();
    QTextCursor cursor(document()->findBlockByNumber(blockNumber));
    cursor.movePosition(QTextCursor::EndOfBlock);
    setTextCursor(cursor);
    centerCursor();
    dontUpdateWordEditor = false;

    qInfo() << "[Inserted TimeStamp from Player]"
            << QString("line number: %1, timestamp: %2").arg(QString::number(blockNumber), elapsedTime.toString("hh:mm:ss.zzz"));

}

void Editor::changeTranscriptLang()
{
    auto newLang = QInputDialog::getText(this, "Change Transcript Language", "Current Language: " + m_transcriptLang);
    m_transcriptLang = newLang.toLower();

    loadDictionary();
}

void Editor::speakerWiseJump(const QString& jumpDirection)
{
    auto& blockNumber = highlightedBlock;

    if (blockNumber == -1) {
        emit message("Highlighted block not present");
        return;
    }

    auto speakerName = m_blocks[blockNumber].speaker;
    int blockToJump{-1};

    if (jumpDirection == "up") {
        for (int i = blockNumber - 1; i >= 0; i--)
            if (speakerName == m_blocks[i].speaker) {
                blockToJump = i;
                break;
            }
    }
    else if (jumpDirection == "down") {
        for (int i = blockNumber + 1; i < m_blocks.size(); i++)
            if (speakerName == m_blocks[i].speaker) {
                blockToJump = i;
                break;
            }
    }

    if (blockToJump == -1) {
        emit message("Couldn't find a block to jump");
        return;
    }

    QTime timeToJump(0, 0);

    for (int i = blockToJump - 1; i >= 0; i--) {
        if (m_blocks[i].timeStamp.isValid()) {
            timeToJump = m_blocks[i].timeStamp;
            break;
        }
    }

    emit jumpToPlayer(timeToJump);
}

void Editor::wordWiseJump(const QString& jumpDirection)
{
    auto& wordNumber = highlightedWord;

    if (highlightedBlock == -1 || wordNumber == -1) {
        emit message("Highlighted block or word not present");
        return;
    }

    auto& highlightedBlockWords = m_blocks[highlightedBlock].words;
    QTime timeToJump;
    int wordToJump{-1};

    if (jumpDirection == "left")
        wordToJump = wordNumber - 1;
    else if (jumpDirection == "right")
        wordToJump = wordNumber + 1;

    if (wordToJump < 0 || wordToJump >= highlightedBlockWords.size()) {
        emit message("Can't jump, end of block reached!", 2000);
        return;
    }

    if (jumpDirection == "left") {
        if (wordToJump == 0){
            timeToJump = QTime(0, 0);
            for (int i = highlightedBlock - 1; i >= 0; i--) {
                if (m_blocks[i].timeStamp.isValid()) {
                    timeToJump = m_blocks[i].timeStamp;
                    break;
                }
            }
        }
        else {
            for (int i = wordToJump - 1; i >= 0; i--)
                if (highlightedBlockWords[i].timeStamp.isValid()) {
                    timeToJump = highlightedBlockWords[i].timeStamp;
                    break;
                }
        }
    }

    if (jumpDirection == "right")
        timeToJump = highlightedBlockWords[wordToJump - 1].timeStamp;

    if (timeToJump.isNull()) {
        emit message("Couldn't find a word to jump to");
        return;
    }

    emit jumpToPlayer(timeToJump);
}


void Editor::blockWiseJump(const QString& jumpDirection)
{
    if (highlightedBlock == -1)
        return;

    int blockToJump{-1};

    if (jumpDirection == "up")
        blockToJump = highlightedBlock - 1;
    else if (jumpDirection == "down")
        blockToJump = highlightedBlock + 1;

    if (blockToJump == -1 || blockToJump == blockCount())
        return;

    QTime timeToJump;

    if (jumpDirection == "up") {
        timeToJump = QTime(0, 0);
        for (int i = blockToJump - 1; i >= 0; i--) {
            if (m_blocks[i].timeStamp.isValid()) {
                timeToJump = m_blocks[i].timeStamp;
                break;
            }
        }
    }
    else if (jumpDirection == "down")
        timeToJump = m_blocks[highlightedBlock].timeStamp;

    emit jumpToPlayer(timeToJump);

}

void Editor::useTransliteration(bool value, const QString& langCode)
{
    m_transliterate = value;
    m_transliterateLangCode = langCode;
}

void Editor::suggest(QString suggest)
{

    QTextCursor cursor = textCursor();

    //        cursor.insertText(suggest);

    cursor.movePosition(QTextCursor::StartOfWord);

    cursor.select(QTextCursor::WordUnderCursor);
    cursor.removeSelectedText();
    cursor.insertText(suggest);


}

void Editor::realTimeDataSavingToggle()
{
    if(realTimeDataSaver){
        realTimeDataSaver=false;
    }
    else if(!realTimeDataSaver){
        realTimeDataSaver=true;
    }
}

void Editor::saveAsPDF()
{

    QFile final("pdf.txt");
    if(!final.open(QIODevice::OpenModeFlag::WriteOnly)){
        QMessageBox::critical(this,"Error",final.errorString());
        return;
    }
    QString initialhtm="<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta http-equiv='X-UA-Compatible' content='IE=edge'><meta name='viewport' content='width= 290 , initial-scale=1.0'><title>Document</title></head><body>\n";
    QString finalhtml="</body></html>";
    QString content_with_time_stamp("");
    QString content_without_time_stamp("");
    QString htmlpart(initialhtm);



    for (auto& a_block: qAsConst(m_blocks)) {
        auto blockText = "<p>[" + a_block.speaker + "]: " + a_block.text + " [" + a_block.timeStamp.toString("hh:mm:ss.zzz") + "]<p>";
        content_with_time_stamp.append(blockText + "\n\n");
    }

    for (auto& a_block: qAsConst(m_blocks)) {
        auto blockText = "<p>[" + a_block.speaker + "]: " + a_block.text+"<p>" ;
        content_without_time_stamp.append(blockText + "\n\n");
    }

    if(showTimeStamp){
        htmlpart.append(content_with_time_stamp);
    }
    else{
        htmlpart.append(content_without_time_stamp);
    }
    htmlpart.append(finalhtml);



    auto pdfSaveLocation = QFileDialog::getSaveFileName(this, "Export PDF", QString("/"), "*.pdf");
    if(pdfSaveLocation!=""){
        if (QFileInfo(pdfSaveLocation).suffix().isEmpty()) { pdfSaveLocation.append(".pdf"); }
        qInfo()<<pdfSaveLocation;

        QPrinter printer(QPrinter::PrinterResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setPaperSize(QPrinter::A4);
        printer.setOutputFileName(pdfSaveLocation);
        QTextDocument doc;
        doc.setHtml(htmlpart);
        doc.setPageSize(printer.pageRect().size()); // This is necessary if you want to hide the page number
        doc.print(&printer);
    }
}

void Editor::saveAsTXT()    // save the transcript as a text file
{
    QString txtContent;

    for (auto& a_block : qAsConst(m_blocks)) {
        auto blockText = "[" + a_block.speaker + "]: " + a_block.text + " [" + a_block.timeStamp.toString("hh:mm:ss.zzz") + "]\n";
        txtContent.append(blockText + "\n");
    }

    QString txtSaveLocation = QFileDialog::getSaveFileName(this, "Export TXT", QString("/"), "*.txt");
    if (txtSaveLocation != "") {
        if (QFileInfo(txtSaveLocation).suffix().isEmpty()) {
            txtSaveLocation.append(".txt");
        }
        qInfo() << txtSaveLocation;

        QFile txtFile(txtSaveLocation);
        if (txtFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&txtFile);
            out << txtContent;
            txtFile.close();
        } else {
            QMessageBox::critical(this, "Error", txtFile.errorString());
        }
    }
}

void Editor::updateWordEditor()
{
    if (!m_wordEditor || dontUpdateWordEditor)
        return;

    updatingWordEditor = true;

    auto blockNumber = textCursor().blockNumber();

    if (blockNumber >= m_blocks.size()) {
        m_wordEditor->clear();
        return;
    }

    m_wordEditor->refreshWords(m_blocks[blockNumber].words);

    updatingWordEditor = false;
}

void Editor::wordEditorChanged()
{
    auto editorBlockNumber = textCursor().blockNumber();

    if (document()->isEmpty() || m_blocks.isEmpty())
        m_blocks.append(fromEditor(0));

    if (settingContent || updatingWordEditor || editorBlockNumber >= m_blocks.size())
        return;

    auto& block = m_blocks[editorBlockNumber];
    if (block.words.isEmpty()) {
        block.words = m_wordEditor->currentWords();
        return;
    }

    auto& words = block.words;
    words.clear();

    QString blockText;
    words = m_wordEditor->currentWords();
    for (auto& a_word: words)
        blockText += a_word.text + " ";
    block.text = blockText.trimmed();

    dontUpdateWordEditor = true;
    setContent();
    QTextCursor cursor(document()->findBlockByNumber(editorBlockNumber));
    setTextCursor(cursor);
    centerCursor();
    dontUpdateWordEditor = false;
}

void Editor::changeSpeaker(const QString& newSpeaker, bool replaceAllOccurrences)
{
    if (m_blocks.isEmpty())
        return;
    auto blockNumber = textCursor().blockNumber();
    auto blockSpeaker = m_blocks[blockNumber].speaker;

    if (!replaceAllOccurrences)
        m_blocks[blockNumber].speaker = newSpeaker;
    else {
        for (auto& a_block: m_blocks){
            if (a_block.speaker == blockSpeaker)
                a_block.speaker = newSpeaker;
        }
    }

    setContent();
    QTextCursor cursor(document()->findBlockByNumber(blockNumber));
    setTextCursor(cursor);
    centerCursor();

    qInfo() << "[Speaker Changed]"
            << QString("line number: %1").arg(QString::number(blockNumber + 1))
            << QString("initial: %1").arg(blockSpeaker)
            << QString("final: %1").arg(newSpeaker);
}

void Editor::propagateTime(const QTime& time, int start, int end, bool negateTime)
{
    if (time.isNull()) {
        QMessageBox errorBox(QMessageBox::Critical, "Error", "Invalid Time Selected", QMessageBox::Ok);
        errorBox.exec();
        return;
    }
    else if (start < 1 || end > blockCount() || start > end) {
        QMessageBox errorBox(QMessageBox::Critical, "Error", "Invalid Block Range Selected", QMessageBox::Ok);
        errorBox.exec();
        return;
    }

    for (int i = start - 1; i < end; i++) {
        auto& currentTimeStamp = m_blocks[i].timeStamp;

        if (currentTimeStamp.isNull())
            currentTimeStamp = QTime(0,0, 0, 0);

        int secondsToAdd = time.hour() * 3600 + time.minute() * 60 + time.second();
        int msecondsToAdd = time.msec();

        if (negateTime) {
            secondsToAdd = -secondsToAdd;
            msecondsToAdd = -msecondsToAdd;
        }

        currentTimeStamp = currentTimeStamp.addMSecs(msecondsToAdd);
        currentTimeStamp = currentTimeStamp.addSecs(secondsToAdd);

    }

    int blockNumber = textCursor().blockNumber();

    setContent();
    QTextCursor cursor(document()->findBlockByNumber(blockNumber));
    setTextCursor(cursor);
    centerCursor();

    qInfo() << "[Time propagated]"
            << QString("block range: %1 - %2").arg(QString::number(start), QString::number(end))
            << QString("time: %1 %2").arg(negateTime? "-" : "+", time.toString("hh:mm:ss.zzz"));
}

void Editor::selectTags(const QStringList& newTagList)
{
    m_blocks[textCursor().blockNumber()].tagList = newTagList;

    emit refreshTagList(newTagList);

    qInfo() << "[Tags Selected]"
            << "new tags: " << newTagList;
    setContent();

}

void Editor::markWordAsCorrect(int blockNumber, int wordNumber)
{
    auto textToInsert = m_blocks[blockNumber].words[wordNumber].text.toLower();

    if (textToInsert.trimmed() == "")
        return;

    if (std::binary_search(m_dictionary.begin(),
                           m_dictionary.end(),
                           textToInsert))
    {
        emit message("Word is already correct.");
        return;
    }

    m_dictionary.insert
        (
            std::upper_bound(m_dictionary.begin(), m_dictionary.end(), textToInsert),
            textToInsert
            );

    static_cast<QStringListModel*>(m_textCompleter->model())->setStringList(m_dictionary);
    m_correctedWords.insert(textToInsert);

    QMultiMap<int, int> invalidWords;
    for (int i = 0; i < m_blocks.size(); i++) {
        //        qInfo()<<"\n \n m_blocks["<<i<<"].words : ";
        for (int j = 0; j < m_blocks[i].words.size(); j++) {
            auto wordText = m_blocks[i].words[j].text.toLower();

            if (wordText != "" && m_punctuation.contains(wordText.back()))
                wordText = wordText.left(wordText.size() - 1);

            if (!std::binary_search(m_dictionary.begin(),
                                    m_dictionary.end(),
                                    wordText)
                )
                invalidWords.insert(i, j);
        }
    }
    m_highlighter->setInvalidWords(invalidWords);
    m_highlighter->rehighlight();

    QFile correctedWords(QString("corrected_words_%1.txt").arg(m_transcriptLang));

    if (!correctedWords.open(QFile::WriteOnly | QFile::Truncate))
        emit message("Couldn't write corrected words to file.");
    else {
        for (auto& a_word: m_correctedWords)
            correctedWords.write(QString(a_word + "\n").toStdString().c_str());
    }

    qInfo() << "[Mark As Correct]"
            << QString("text: %1").arg(textToInsert);
}

void Editor::insertSpeakerCompletion(const QString& completion)
{
    if (m_speakerCompleter->widget() != this)
        return;
    QTextCursor tc = textCursor();
    int extra = completion.length() - m_speakerCompleter->completionPrefix().length();

    if (m_speakerCompleter->completionPrefix().length()) {
        tc.movePosition(QTextCursor::Left);
        tc.movePosition(QTextCursor::EndOfWord);
    }

    tc.insertText(completion.right(extra));
    setTextCursor(tc);
}

void Editor::insertTextCompletion(const QString& completion)
{
    if (m_textCompleter->widget() != this)
        return;
    QTextCursor tc = textCursor();
    int extra = completion.length() - m_textCompleter->completionPrefix().length();

    tc.movePosition(QTextCursor::Left);
    tc.movePosition(QTextCursor::EndOfWord);
    tc.insertText(completion.right(extra));

    setTextCursor(tc);
}

void Editor::insertTransliterationCompletion(const QString& completion)
{
    if (m_textCompleter->widget() != this)
        return;

    QTextCursor tc = textCursor();
    tc.select(QTextCursor::WordUnderCursor);
    tc.insertText(completion);

    setTextCursor(tc);
}

void Editor::handleReply()
{
    QStringList tokens;

    QString replyString = m_reply->readAll();

    if (replyString.split("[\"").size() < 4)
        return;

    tokens = replyString.split("[\"")[3].split("]").first().split("\",\"");

    auto lastString = tokens[tokens.size() - 1];
    tokens[tokens.size() - 1] = lastString.left(lastString.size() - 1);

    m_lastReplyList = tokens;
}

void Editor::sendRequest(const QString& input, const QString& langCode)
{
    if (m_reply) {
        m_reply->abort();
        delete m_reply;
        m_reply = nullptr;
    }

    QString url = QString("http://inputtools.google.com/request?text=%1&itc=%2-t-i0-und&num=10&cp=0&cs=1&ie=utf-8&oe=utf-8&app=test").arg(input, langCode);

    QNetworkRequest request(url);
    m_reply = m_manager.get(request);

    connect(m_reply, &QNetworkReply::finished, this, [this] () {
        if (m_reply->error() == QNetworkReply::NoError)
            handleReply();
        else if (m_reply->error() != QNetworkReply::OperationCanceledError)
            emit message(m_reply->errorString());
        emit replyCame();
    });
}

void Editor::pushbutton(const QTime& elapsedTime)
{
    //    player = new MediaPlayer(this);
    //    const QTime elapsedTime = player->elapsedTime();
    auto blockNumber = textCursor().blockNumber();
    QTextBlock highlightedTb = this->document()->findBlockByNumber(blockNumber);
    QString highlightedLine = highlightedTb.text();
    QString timestamp = elapsedTime.toString("hh:mm:ss.zzz");
    //    QString timestamp = m_blocks[blockNumber].timeStamp.toString("hh:mm:ss.zzz");
    QString lineWithTimestamp = highlightedLine + "[" + timestamp + "] ";
    lineWithTimestamp.remove(0, 12);
    m_blocks[blockNumber].text = lineWithTimestamp;
    dontUpdateWordEditor = true;
    setContent();
    QTextCursor cursor(document()->findBlockByNumber(blockNumber));
    setTextCursor(cursor);
    centerCursor();
    dontUpdateWordEditor = false;
}

void Editor::removespeaker()
{
    auto blockNumber = textCursor().blockNumber();
    removespeakerbool = true;
    dontUpdateWordEditor = true;
    setContent();
    QTextCursor cursor(document()->findBlockByNumber(blockNumber));
    setTextCursor(cursor);
    centerCursor();
    dontUpdateWordEditor = false;
}

void Editor::showwordcount(){
    QDialog dialog(this);
    QFormLayout form(&dialog);
    form.addRow(new QLabel("Word Count", this));

    QLineEdit *total_words = new QLineEdit(&dialog);
    QLineEdit *speaker_count = new QLineEdit(&dialog);
    QLineEdit *modified_words = new QLineEdit(&dialog);
    QLineEdit *modified_speaker_count = new QLineEdit(&dialog);
    QString str1 = QString::number(countwords);
    QString str2 = QString::number(mySet.size());
    QString str3 = QString::number(modifiedwords);
    QString str4 = QString::number(modset.size()==1?modset.size():modset.size()-1);

    total_words->setText(str1);
    total_words->setReadOnly(true);
    form.addRow("Total Words",total_words);
    speaker_count->setText(str2);
    speaker_count->setReadOnly(true);
    form.addRow("Speaker Count",speaker_count);
    modified_words->setText(str3);
    modified_words->setReadOnly(true);
    form.addRow("Modified count",modified_words);
    modified_speaker_count->setText(str4);
    modified_speaker_count->setReadOnly(true);
    form.addRow("Modified Speaker count",modified_speaker_count);
    dialog.exec();
}

void Editor::on_actionword_count_triggered()
{
    showwordcount();
}

void Editor::on_actionLink_triggered()
{
    QDialog dialog(this);
    QFormLayout form(&dialog);      // Use a layout allowing to have a label next to each field
    form.addRow(new QLabel("Enter the url and the placeholder", this));

    //! Add the lineEdits with their respective labels
    QLineEdit *link = new QLineEdit(&dialog);
    QLineEdit *text = new QLineEdit(&dialog);
    form.addRow("url", link);
    form.addRow("title", text);

    //! Add some standard buttons (Cancel/Ok) at the bottom of the dialog
    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);
    QObject::connect(&buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    QObject::connect(&buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));

    //! Show the dialog as modal
    if (dialog.exec() == QDialog::Accepted)
    {

        QTextCursor cursor = textCursor();
        cursor.insertHtml("<a href=\"" + link->text() + "\">" + text->text() + "</a>");
        auto blockNumber = textCursor().blockNumber();
        QTextBlock highlightedTb = this->document()->findBlockByNumber(blockNumber);
        QString highlightedLine = highlightedTb.text();
        m_blocks[blockNumber].text = highlightedLine;
        //        setContent();

    }
}

//void Editor::on_actionVoice_triggered()
//{
//    QDialog dialog;
//    dialog.setWindowTitle("Recording Dialog");

//    QLabel label("Start Recording", &dialog);
//    QPushButton startButton("Start", &dialog);

//    QHBoxLayout layout;
//    layout.addWidget(&label);
//    layout.addWidget(&startButton);
//    dialog.setLayout(&layout);

//    QObject::connect(&startButton, &QPushButton::clicked, [&label, &startButton]() {
//        if (startButton.text() == "Start")
//        {
//            startButton.setText("Stop");
//            label.setText("Recording");
//        }
//        else if (startButton.text() == "Stop")
//        {
//            startButton.setText("Start");
//            label.setText("Start Recording");
//        }
//    });

//    dialog.exec();
//}

void Editor::on_actionVoice_triggered()
{
    QDialog dialog;
    dialog.setWindowTitle("Recording Dialog");

    QLabel label("Start Recording", &dialog);
    QPushButton startButton("Start", &dialog);

    QHBoxLayout layout;
    layout.addWidget(&label);
    layout.addWidget(&startButton);
    dialog.setLayout(&layout);

    // Declare QAudioFormat and QBuffer for audio capture
    QAudioFormat format;
    format.setSampleRate(44100);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    QBuffer buffer;
    buffer.open(QIODevice::ReadWrite);

    // Connect the button click to the audio capture and conversion slot
    QObject::connect(&startButton, &QPushButton::clicked, [&label, &startButton, this, &buffer, format]() {
        if (startButton.text() == "Start")
        {
            // Start audio capture
            m_audioInput = new QAudioInput(format);
            m_audioInput->start(&buffer);

            startButton.setText("Stop");
            label.setText("Recording");
        }
        else if (startButton.text() == "Stop")
        {
            // Stop audio capture
            m_audioInput->stop();
            delete m_audioInput;
            m_audioInput = nullptr;

            startButton.setText("Start");
            label.setText("Start Recording");

            // Retrieve the audio data from the buffer
            m_audioData = buffer.buffer();

            // Code to send m_audioData to your custom API for speech-to-text conversion
            QNetworkAccessManager manager;
            QNetworkRequest request(QUrl("https://3.6.185.140:5555/transcript"));
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
            QNetworkReply *reply = manager.post(request, m_audioData);
            qInfo()<<"Sendind data to convert to text\n";
            qDebug() << reply->readAll();
            // Connect the signal to handle the API response
            QObject::connect(reply, &QNetworkReply::finished, [&reply]() {
                qDebug() << "Entered";
                if (reply->error() == QNetworkReply::NoError)
                {
                    QString response = reply->readAll();
                    // Process the response, assuming it contains the converted text
                    qDebug() << "API Response: " << response;
                }
                else
                {
                    qDebug() << "API Error: " << reply->errorString();
                }

                reply->deleteLater();
            });
        }
    });

    dialog.exec();
}
void Editor:: removetimestamp()
{
    qInfo()<<"Hello";
    auto blockNumber = textCursor().blockNumber();
    removetimebool = true;
    dontUpdateWordEditor = true;
    setContent();
    qInfo()<<"Again";
    QTextCursor cursor(document()->findBlockByNumber(blockNumber));
    setTextCursor(cursor);
    centerCursor();
    removetimebool = false;
    dontUpdateWordEditor = false;
}

// void Editor:: showversion()
// {
//     QDialog dialog(this);
//     QFormLayout form(&dialog);
//     form.addRow(new QLabel("Vāgyojaka - Version 1.0.3\n"
//                            "This framework is developed by IIT Bombay.\n"
//                            "This work is licensed under Creative Commons Attribution-NonCommercial 4.0 International License", this));
//     dialog.exec();
// }
