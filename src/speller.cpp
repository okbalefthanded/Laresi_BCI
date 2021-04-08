//
#include <QWindow>
#include <QMessageBox>
#include <QGenericMatrix>
#include <QLabel>
#include <QGridLayout>
#include <QTimer>
#include <QHBoxLayout>
#include <QThread>
#include <QTime>
#include <QPropertyAnimation>
#include <QDateTime>
#include <QPixmap>
#include <QDir>
#include <QPainter>
//
#include "speller.h"
//#include "ui_speller.h"
#include "configpanel.h"
#include "ovtk_stimulations.h"
#include "randomflashsequence.h"
#include "utils.h"
//
Speller::Speller(QWidget *parent) : QWidget(parent)
  //ui(new Ui::Speller)
{

    // qDebug()<< Q_FUNC_INFO;
    // ui->setupUi(this);
    setupUi(this);
    this->setProperty("windowTitle", "ERP Speller");
    this->show();

    showWindow();
    createLayout();
    initTimers();
    initFeedbackSocket();

    m_state = trial_state::PRE_TRIAL;

}

Speller::Speller(int i)
{
    // qDebug()<< Q_FUNC_INFO;
}

void Speller::startTrial()
{
    // qDebug()<< "[TRIAL START]" << Q_FUNC_INFO;

    if (m_state == trial_state::PRE_TRIAL)
    {
        preTrial();
    }

    if (m_state == trial_state::STIMULUS)
    {
        startFlashing();
    }
    else if (m_state == trial_state::POST_STIMULUS)
    {
        pauseFlashing();
    }
}

void Speller::startFlashing(){}

void Speller::pauseFlashing()
{

    // sendMarker(OVTK_StimulationId_VisualStimulationStop);
    // qDebug() << Q_FUNC_INFO << QTime::currentTime().msec();

    //   this->layout()->itemAt(m_flashingSequence->sequence[m_currentStimulation])->
    //                widget()->setStyleSheet("QLabel { color : gray; font: 40pt }");

    m_element = new QLabel();
    m_element->setPixmap(m_icons[m_flashingSequence->sequence[m_currentStimulation] - 1]);
    m_element->setAlignment(Qt::AlignCenter);

    this->layout()->replaceWidget(this->
                                  layout()->
                                  itemAt(m_flashingSequence->sequence[m_currentStimulation]-1)->
                                  widget(),
                                  m_element,
                                  Qt::FindDirectChildrenOnly);

    switchStimulationTimers();
    ++m_currentStimulation;

    trialEnd();
}

void Speller::preTrial()
{
    // qDebug()<< Q_FUNC_INFO;

    if(m_trials == 0)
    {
        sendMarker(OVTK_StimulationId_ExperimentStart);
        //        initLogger();
    }

    if (m_preTrialCount == 0)
    {
        sendMarker(OVTK_StimulationId_TrialStart);
        m_flashingSequence = new RandomFlashSequence(m_nrElements, m_ERP->nrSequences());

        // qDebug() << Q_FUNC_INFO << m_flashingSequence->sequence;

        if (m_ERP->experimentMode() == operation_mode::CALIBRATION)
        {
            highlightTarget();
            m_text += m_desiredPhrase[m_currentLetter];
            m_textRow->setText(m_text);

        }
        else if(m_ERP->experimentMode() == operation_mode::COPY_MODE)
        {
            highlightTarget();
            // m_textRow->setText(m_text);
        }
        else if(m_ERP->experimentMode() == operation_mode::FREE_MODE)
        {
            utils::wait(500);
        }
    }

    m_preTrialTimer->start();
    ++m_preTrialCount;
    // qDebug() << Q_FUNC_INFO << m_preTrialCount;
    endPreTrial();
}

void Speller::feedback()
{
    receiveFeedback();
    m_textRow->setText(m_text);

    // qDebug()<< Q_FUNC_INFO << "setting TEXT ROW with "<< m_text;
    // qDebug()<< "m text row "<< m_textRow->text();

    // Presenting Feedback
    if (m_presentFeedback)
    {
        //receiveFeedback();
        //m_textRow->setText(m_text);

        if (m_text[m_text.length()-1] != "#")
        {
            if (m_ERP->experimentMode() == operation_mode::COPY_MODE)
            {
                int id = m_text[m_text.length()-1].digitValue();
                QPixmap map = m_icons[id-1];
                // feedback: green correct selection, highlight the target icon
                //           blue incorrect selection, highlight the selected icon
                if( m_text[m_text.length()-1] == m_desiredPhrase[m_currentLetter - 1])
                {
                    //            this->layout()->itemAt(m_currentTarget)->
                    //                    widget()->setStyleSheet("QLabel { color : green; font: 40pt }");
                    map.fill(Qt::green);
                    isCorrect = true;
                    ++m_correct;
                }
                else
                {
                    // this->layout()->itemAt(m_currentTarget)->
                    //         widget()->setStyleSheet("QLabel { color : blue; font: 40pt }");
                    map.fill(Qt::blue);
                    isCorrect = false;
                }
                // qDebug()<< Q_FUNC_INFO << "element id: "<< id;
                m_element = new QLabel();
                m_element->setPixmap(map);
                m_element->setAlignment(Qt::AlignCenter);

                this->layout()->replaceWidget(this->
                                              layout()->
                                              itemAt(id-1)->
                                              widget(),
                                              m_element,
                                              Qt::FindDirectChildrenOnly);
            }

            else if (m_ERP->experimentMode() == operation_mode::FREE_MODE)
            {
                int id = m_text[m_text.length()-1].digitValue();
                QPixmap map = m_icons[id-1];
                map.fill(Qt::blue);
                m_element = new QLabel();
                m_element->setPixmap(map);
                m_element->setAlignment(Qt::AlignCenter);

                this->layout()->replaceWidget(this->
                                              layout()->
                                              itemAt(id-1)->
                                              widget(),
                                              m_element,
                                              Qt::FindDirectChildrenOnly);

            }
        }
    }
    postTrial();
}

void Speller::postTrial()
{
    // qDebug()<< Q_FUNC_INFO;

    ++m_trials;
    // m_currentStimulation = 0;
    // m_state = trial_state::PRE_TRIAL;
    // wait
    // utils::wait(1000);
    // utils::wait(500);
    // utils::wait(250); // showing feedback for 0.25 sec
    // refreshTarget();

    if (m_presentFeedback)
    {
        // utils::wait(1000);
        // utils::wait(500);
        // utils::wait(250); // showing feedback for 0.25 sec
        utils::wait(100);
        if (m_text[m_text.length()-1] != "#")
        {
            if (m_ERP->experimentMode() == operation_mode::COPY_MODE ||
                    m_ERP->experimentMode() == operation_mode::FREE_MODE)
            {
                int id = m_text[m_text.length()-1].digitValue();
                // qDebug()<< Q_FUNC_INFO << "element ID" << id;
                QPixmap map = m_icons[id-1];
                m_element = new QLabel();
                m_element->setPixmap(map);
                m_element->setAlignment(Qt::AlignCenter);

                this->layout()->replaceWidget(this->
                                              layout()->
                                              itemAt(id-1)->
                                              widget(),
                                              m_element,
                                              Qt::FindDirectChildrenOnly);


            }
            else if(m_ERP->experimentMode() == operation_mode::CALIBRATION)
            {
                refreshTarget();
            }
        }
    }
    // m_textRow->setText(m_text);
    // m_textRow->show();

    // Send and Recieve feedback to/from Robot if external communication is enabled
    externalCommunication();

    m_currentStimulation = 0;
    m_state = trial_state::PRE_TRIAL;
    //
    if (m_currentLetter >= m_desiredPhrase.length() &&
            m_desiredPhrase.length() != 1 &&
            (m_ERP->experimentMode() == operation_mode::COPY_MODE ||
             m_ERP->experimentMode() == operation_mode::CALIBRATION)
            )
    {
        m_correct = (m_correct / m_desiredPhrase.length()) * 100;
        qDebug()<< "Accuracy on ERP session: " << m_correct;
        qDebug()<< "Experiment End, closing speller";
        sendMarker(OVTK_StimulationId_ExperimentStop);
        utils::wait(2000);
        // emit(slotTerminated());
        // this->close();
    }
    else if(m_desiredPhrase.length() <= 1)
    {
        // qDebug() << "[POST TRIAL 1]" << Q_FUNC_INFO;
        m_currentLetter = 0;
        emit(slotTerminated());
        return;
    }
    else
    {
        this->layout()->update();
        startTrial();
    }

}

void Speller::receiveFeedback()
{
    // qDebug() << Q_FUNC_INFO;
    // qDebug() << "FEEDBACK SOCKET PORT" << m_feedbackPort;
    // wait for OV python script to write in UDP feedback socket
    // utils::wait(500);
    // utils::wait(250);

    // utils::wait(200);
    // utils::wait(80);
    // m_feedbackSocket->waitForReadyRead(90);
    // qDebug()<< QTime::currentTime();
    m_feedbackSocket->waitForReadyRead();
    QHostAddress sender;
    quint16 senderPort;
    QByteArray *buffer = new QByteArray();

    buffer->resize(m_feedbackSocket->pendingDatagramSize());
    // qDebug() << "buffer size" << buffer->size();

    //m_feedbackSocket->readDatagram(buffer->data(), buffer->size(), &sender, &senderPort);
    while(m_feedbackSocket->hasPendingDatagrams())
    {
        m_feedbackSocket->readDatagram(buffer->data(), buffer->size(), &sender, &senderPort);
    }
    //  feedback_socket->waitForBytesWritten();
    // qDebug()<< "Received: "<< QString(buffer->data());
    m_text += QString(buffer->data());
}

bool Speller::isTarget()
{
    int index = m_flashingSequence->sequence[m_currentStimulation] - 1;

    if(m_desiredPhrase[m_currentLetter] == m_presentedLetters[index][0])
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool Speller::isTarget(int t_stim)
{
    if (m_desiredPhrase[m_currentLetter] == m_presentedLetters[t_stim-1][0])
        return true;
    else
        return false;
}

void Speller::highlightTarget()
{
    int idx = 0;

    for (int i=0; i<m_rows; i++)
    {
        for (int j=0; j<m_cols; j++)
        {
            //            idx++;
            //            if (desired_phrase[m_currentLetter] == letters[i][j]){
            //                currentTarget = idx;
            //                break;
            if(m_desiredPhrase[m_currentLetter] == m_presentedLetters[idx][0])
            {
                m_currentTarget = idx + 1;
                break;
            }
            idx++;
        }
    }

    // qDebug()<< Q_FUNC_INFO << "current tg "<< m_currentTarget << "current letter " << m_desiredPhrase[m_currentLetter];
    QPixmap map = m_icons[m_currentTarget - 1];
    map.fill(Qt::yellow);

    m_element = new QLabel();
    m_element->setPixmap(map);
    m_element->setAlignment(Qt::AlignCenter);

    this->layout()->replaceWidget(this->
                                  layout()->
                                  itemAt(m_currentTarget-1)->
                                  widget(),
                                  m_element,
                                  Qt::FindDirectChildrenOnly);

}

void Speller::refreshTarget()
{

    //   this->layout()->itemAt(m_currentTarget)->
    //           widget()->setStyleSheet("QLabel { color : gray; font: 40pt }");
    // qDebug()<< Q_FUNC_INFO << m_currentTarget;

    m_element = new QLabel();
    QPixmap map = m_icons[m_currentTarget - 1];
    m_element->setPixmap(map);
    m_element->setAlignment(Qt::AlignCenter);
    /*
    this->layout()->replaceWidget(this->
                                  layout()->
                                  itemAt(m_currentTarget)->
                                  widget(),
                                  m_element,
                                  Qt::FindDirectChildrenOnly);
                                  */

    // qDebug()<< Q_FUNC_INFO << "[current target] "<< m_currentTarget <<"[map] "<< m_currentTarget-1;

}

void Speller::sendStimulationInfo()
{
    sendMarker(OVTK_StimulationId_VisualStimulationStart);
    sendMarker(config::OVTK_StimulationLabel_Base + m_flashingSequence->sequence[m_currentStimulation]);

    // send target marker
    if (m_ERP->experimentMode() == operation_mode::CALIBRATION ||
            m_ERP->experimentMode() == operation_mode::COPY_MODE)
    {
        if (isTarget())
        {
            // qDebug()<< desired_phrase[currentLetter];
            sendMarker(OVTK_StimulationId_Target);
        }
        else
        {
            sendMarker(OVTK_StimulationId_NonTarget);
        }
    }
}

void Speller::switchStimulationTimers()
{
    if(m_state == trial_state::STIMULUS)
    {
        m_stimTimer->start();
        m_isiTimer->stop();
        m_state = trial_state::POST_STIMULUS;
    }
    else if(trial_state::POST_STIMULUS)
    {
        m_stimTimer->stop();
        m_isiTimer->start();
        m_state = trial_state::STIMULUS;
    }
}

void Speller::startPreTrial()
{
    // qDebug() << Q_FUNC_INFO;

    if (m_preTrialCount == 0)
    {
        sendMarker(OVTK_StimulationId_TrialStart);
        m_flashingSequence = new RandomFlashSequence(m_nrElements, m_ERP->nrSequences());
        // qDebug() << "about to test mode";
        if (m_ERP->experimentMode() == operation_mode::CALIBRATION)
        {
            highlightTarget();
            m_text += m_desiredPhrase[m_currentLetter];
            m_textRow->setText(m_text);
        }
        else if(m_ERP->experimentMode() == operation_mode::COPY_MODE)
        {
            highlightTarget();
        }
        else if(m_ERP->experimentMode() == operation_mode::FREE_MODE)
        {
            utils::wait(500);
        }
    }
    else return;
}

void Speller::endPreTrial()
{
    // qDebug() << Q_FUNC_INFO;
    if (m_preTrialCount > m_preTrialWait || m_ERP->experimentMode() == operation_mode::FREE_MODE)
    {
        if(m_ERP->experimentMode() == operation_mode::COPY_MODE ||
                m_ERP->experimentMode() == operation_mode::CALIBRATION)
        {

            refreshTarget();
        }
        m_preTrialTimer->stop();
        m_preTrialCount = 0;
        m_state = trial_state::STIMULUS;
    }
    else return;
}

void Speller::trialEnd()
{
    if (m_currentStimulation >= m_flashingSequence->sequence.count())
    {
        ++m_currentLetter;
        m_isiTimer->stop();
        m_stimTimer->stop();

        // utils::wait(1000); // time window after last epoch/stim
        utils::wait(500);
        // utils::wait(700); // 700 ms == epoch time windows
        sendMarker(OVTK_StimulationId_TrialStop);
        m_state = trial_state::FEEDBACK;

        if(m_ERP->experimentMode() == operation_mode::COPY_MODE || m_ERP->experimentMode() == operation_mode::FREE_MODE)
        {
            feedback();
        }
        else if(m_ERP->experimentMode() == operation_mode::CALIBRATION)
        {
            postTrial();
        }
    }
}

void Speller::externalCommunication()
{
    // Send and Recieve feedback to/from Robot if external communication is enabled
    // m_hybridCommand = m_text[m_text.length()-1] + "2";

    m_hybridCommand = m_text[m_text.length()-1];

    if(m_ERP->externalComm() == external_comm::ENABLED)
    {
        qDebug() << "Sending Feedback to Robot";
        if (!m_robotSocket->isOpen())
        {
            qDebug()<< "Not sending Feedback to Robot : Cannot send feedback socket is not open";
        }

        try
        {
            // m_hybridCommand = "12";
            std::string str = m_hybridCommand.toStdString();
            const char* p = str.c_str();
            // qDebug()<< "command to send to Robot: " << m_hybridCommand;
            QByteArray byteovStimulation;
            QDataStream streamovs(&byteovStimulation, QIODevice::WriteOnly);
            streamovs.setByteOrder(QDataStream::LittleEndian);
            streamovs.writeRawData(p, m_hybridCommand.length());
            m_robotSocket->write(byteovStimulation);
            m_robotSocket->waitForBytesWritten();
        }
        catch(...)
        {
            qDebug() <<"Send Feedback to Robot, Issue With writting Data";
        }

        qDebug() << "Recieve State from Robot";
        m_robotSocket->waitForReadyRead();

        QByteArray robotState = m_robotSocket->readAll();

        quint8 rState = robotState.toUInt();
        qDebug()<< Q_FUNC_INFO << "Robot State recieved " << rState;
        if(rState == robot_state::READY)
        {
            qDebug()<< "Correct State";
            m_state = trial_state::PRE_TRIAL;
        }

    }
}

void Speller::initLogger()
{

    // qDebug() << Q_FUNC_INFO;

    QDir logsDir(QCoreApplication::applicationDirPath() + "/logs");
    if(!logsDir.exists())
    {
        logsDir.mkdir(logsDir.path());
    }
    QString fileName;
    if(m_ERP->experimentMode() == operation_mode::CALIBRATION)
    {

        fileName = logsDir.filePath("erp_calib_" +QDateTime::currentDateTime().toString("dd_MM_yyyy_hh_mm_ss")+".txt");
    }
    else
    {
        fileName =  logsDir.filePath("erp_online_" +QDateTime::currentDateTime().toString("dd_MM_yyyy_hh_mm_ss")+".txt");
    }

    log = new Logger(this, fileName);
}

void Speller::showWindow()
{
    this->show();

    if(qApp->screens().count() == 2)
    {
        this->windowHandle()->setScreen(qApp->screens().last());
        this->showFullScreen();
    }
    else
    {
        this->showMaximized();
    }

    this->setStyleSheet("background-color : black");
}

void Speller::initTimers()
{
    m_stimTimer = new QTimer(this);
    m_isiTimer = new QTimer(this);
    m_preTrialTimer = new QTimer(this);

    m_stimTimer->setTimerType(Qt::PreciseTimer);
    m_stimTimer->setSingleShot(true);
    m_stimTimer->setInterval(100); //default value

    m_isiTimer->setTimerType(Qt::PreciseTimer);
    m_isiTimer->setSingleShot(true);
    m_isiTimer->setInterval(100); //default value

    m_preTrialTimer->setTimerType(Qt::PreciseTimer);
    // m_preTrialTimer->setInterval(1000);
    m_preTrialTimer->setInterval(500);
    // m_preTrialTimer->setInterval(200);
    m_preTrialTimer->setSingleShot(true);

    connect( m_stimTimer, SIGNAL(timeout()), this, SLOT(pauseFlashing()) );
    connect( m_isiTimer, SIGNAL(timeout()), this, SLOT(startFlashing()) );
    connect( m_preTrialTimer, SIGNAL(timeout()), this, SLOT(startTrial()) );
}

void Speller::initFeedbackSocket()
{
    m_feedbackSocket = new QUdpSocket(this);
    m_feedbackSocket->bind(QHostAddress::LocalHost, m_feedbackPort);
}

ERP *Speller::erp() const
{
    return m_ERP;
}

void Speller::setERP(ERP *erp)
{
    m_ERP = erp;
    setTimers(m_ERP->stimulationDuration(), m_ERP->breakDuration());
    setDesiredPhrase(m_ERP->desiredPhrase());
    m_textRow->setText(m_desiredPhrase);
    // external comm
    // a temporary hack
    // qDebug()<< "Lets see external comm" <<m_ERP->externalComm();
    if(m_ERP->externalComm() == external_comm::ENABLED)
    {
        qDebug()<< "External Comm is enabled;";
        m_robotSocket = new QTcpSocket();
        //    m_robotSocket->connectToHost(QHostAddress("10.3.66.5"), m_robotPort);
        // 10.3.66.5 / 10.6.65.128 /10.3.64.92
        // m_robotSocket->connectToHost(QHostAddress("10.3.64.92"), m_robotPort);
        m_robotSocket->connectToHost(QHostAddress(m_ERP->externalAddress()), m_robotPort);

        if(m_robotSocket->waitForConnected())
        {
            qDebug() << "Robot Connection : State Connected";
        }
        else
        {
            qDebug() << "Robot Connection : State Not Connected";
        }
    }
}

bool Speller::presentFeedback() const
{
    return m_presentFeedback;
}

void Speller::setTimers(int t_stimulation, int t_isi)
{
    m_stimTimer->setInterval(t_stimulation);
    m_isiTimer->setInterval(t_isi);
}

void Speller::setPresentFeedback(bool t_do)
{
    m_presentFeedback = t_do;
}

void Speller::showFeedback(QString command, bool correct)
{
    int id = 0;
    QPixmap map;
    if (command[0] != '#')
    {
        id = command[0].digitValue();
        QString speed = command.at(1);
        // qDebug()<< "speed "<< speed << command;
        // present feedbck
        if (m_ERP->experimentMode() == operation_mode::COPY_MODE)
        {
            map = m_icons[id-1];
            // feedback: green correct selection, highlight the target icon
            //           blue incorrect selection, highlight the selected icon
            if( m_text[m_text.length()-1] == m_desiredPhrase[m_desiredPhrase.length() - 1])
            {
                map.fill(Qt::green);
                isCorrect = true;
                ++m_correct;
            }
            else
            {
                map.fill(Qt::blue);
                isCorrect = false;
            }
            QColor color;
            if (correct)
            {
                color = Qt::red;
            }
            else
            {
                color = Qt::black;
            }
            QPainter painter(&map);
            // painter.setPen(Qt::white);
            painter.setPen(color);
            painter.setFont(QFont("Arial", 30));
            painter.drawText(QPoint(25, 50), speed);

            m_element = new QLabel();
            m_element->setPixmap(map);
            m_element->setAlignment(Qt::AlignCenter);

            this->layout()->replaceWidget(this->
                                          layout()->
                                          itemAt(id-1)->
                                          widget(),
                                          m_element,
                                          Qt::FindDirectChildrenOnly);
        }

        else if (m_ERP->experimentMode() == operation_mode::FREE_MODE)
        {
            map = m_icons[id-1];
            map.fill(Qt::blue);
            m_element = new QLabel();
            m_element->setPixmap(map);
            m_element->setAlignment(Qt::AlignCenter);

            this->layout()->replaceWidget(this->
                                          layout()->
                                          itemAt(id-1)->
                                          widget(),
                                          m_element,
                                          Qt::FindDirectChildrenOnly);

        }
        //
        utils::wait(500);

        // refreshtarget
        if (m_ERP->experimentMode() == operation_mode::COPY_MODE ||
                m_ERP->experimentMode() == operation_mode::FREE_MODE)
        {
            map = m_icons[id-1];
            m_element = new QLabel();
            m_element->setPixmap(map);
            m_element->setAlignment(Qt::AlignCenter);

            this->layout()->replaceWidget(this->
                                          layout()->
                                          itemAt(id-1)->
                                          widget(),
                                          m_element,
                                          Qt::FindDirectChildrenOnly);


        }
    }
}

int Speller::getCurrentTarget()
{
    int idx = 0;

    for (int i=0; i<m_rows; i++)
    {
        for (int j=0; j<m_cols; j++)
        {
            if(m_desiredPhrase[m_currentLetter] == m_presentedLetters[idx][0])
            {
                m_currentTarget = idx + 1;
                break;
            }
            idx++;
        }
    }

    return m_currentTarget;
}

void Speller::setDesiredPhrase(const QString &t_desiredPhrase)
{
    m_desiredPhrase = t_desiredPhrase;
}

void Speller::createLayout()
{
    // qDebug()<< Q_FUNC_INFO;
    // speller settings
    m_rows = 3;
    m_cols = 3;
    m_nrElements = m_rows * m_cols;
    //    m_icons = QList<QImage>();
    //    m_icons = QList<QLabel>();
    //    m_icons = QList<QLabel*>();
    m_icons = QList<QPixmap>();
    m_element = new QLabel();
    m_element->setAlignment(Qt::AlignCenter);
    //    this->matrix_height = 640;
    //    this->matrix_width = 480;
    //    this->setGeometry(0, 0, matrix_width, matrix_height);
    //    this->Mlayout = new MatrixLayout(qMakePair(6,6), 6, 6);
    //    QHBoxLayout *genLayout = new QHBoxLayout();
    QGridLayout *layout = new QGridLayout();

    m_textRow = new QLabel(this);
    m_textRow->setStyleSheet("font:30pt; color:gray; border-color:white;");
    m_textRow->setAlignment(Qt::AlignLeft);
    m_textRow->setObjectName("Desired Phrase Row");
    //    textRow->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // layout->addWidget(m_textRow, 0, 0, 1, 0);

    layout->setHorizontalSpacing(400);
    layout->setVerticalSpacing(100);

    m_textRow->setText(m_desiredPhrase);
    // qDebug()<< Q_FUNC_INFO<< m_desiredPhrase;

    int k = 1;
    QString stimName;
    QPixmap pic;
    QImage iconImage;
    int label_h, label_w;
    // add speller ellements
    for(int i=1; i<m_rows+1; i++)
    {
        for(int j=0; j<m_cols; j++)
        {
            QLabel *element = new QLabel(this);
            label_h = element->height() + 40;
            label_w = element->width() + 40;

            //            QLabel element;
            //            label_h = element.height() + 40;
            //            label_w = element.width() + 40;

            stimName = ":/images/" + QString::number(k) + ".png"; // directions images
            pic = QPixmap(stimName);

            //            element.setPixmap(pic.scaled(label_w, label_h, Qt::KeepAspectRatio));
            //            element.setAlignment(Qt::AlignCenter);

            // element->setText(letters[i-1][j]);

            // element->setText(QString::number(k));
            // element->setStyleSheet("font: 40pt; color:gray");

            // stimName ="image: url(:/images/" + QString::number(k) + ".png)";

            element->setPixmap(pic.scaled(label_w, label_h, Qt::KeepAspectRatio));
            element->setAlignment(Qt::AlignCenter);

            // element->setStyleSheet(stimName);

            //            iconImage = QImage(stimName);
            //            iconImage.scaled(label_w, label_h, Qt::KeepAspectRatio);
            m_icons.append(pic.scaled(label_w, label_h, Qt::KeepAspectRatio));

            //            m_icons->append(element);

            //            m_icons.append(element);
            layout->addWidget(element, i, j);

            m_presentedLetters.append(QString::number(k));
            k++;
            // presented_letters.append(letters[i-1][j]);
        }
    }

    this->setLayout(layout);
}

void Speller::refreshLayout()
{
    // TODO
}

Speller::~Speller()
{
    // delete ui;
}
