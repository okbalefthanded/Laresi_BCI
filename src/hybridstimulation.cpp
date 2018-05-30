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
#include <QPixmap>
//
#include "hybridstimulation.h"
#include "ui_hybridstimulation.h"
#include "configpanel.h"
#include "ovtk_stimulations.h"
#include "randomflashsequence.h"
//
const quint8 PRE_TRIAL = 0;
const quint8 ERPSTIMULUS = 1;
const quint8 SSVEPSTIMULUS = 2;
const quint8 POST_ERPSTIMULUS = 3;
const quint8 FEEDBACK = 4;
const quint8 POST_TRIAL = 5;

const quint8 INVERTED_FACE = 6;
const quint8 COLORED_FACE = 7;
const quint8 INVERTED_COLORED_FACE = 8;

const quint8 CALIBRATION = 0;
const quint8 COPY_MODE  = 1;
const quint8 FREE_MODE = 2;

const uint64_t OVTK_StimulationLabel_Base = 0x00008100;
//
HybridStimulation::HybridStimulation(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::HybridStimulation)
{
    ui->setupUi(this);

    this->show();
    this->setWindowTitle("Hybrid ERP/SSVEP BCI");

    if(qApp->screens().count() == 2){

        this->windowHandle()->setScreen(qApp->screens().last());
        this->showFullScreen();
    }

    QPalette pal = palette();
    pal.setColor(QPalette::Background, Qt::black);
    this->setAutoFillBackground(true);
    this->setPalette(pal);

    create_layout();

    stimTimer = new QTimer(this);
    isiTimer = new QTimer(this);
    preTrialTimer = new QTimer(this);

    stimTimer->setTimerType(Qt::PreciseTimer);
    stimTimer->setSingleShot(true);

    isiTimer->setTimerType(Qt::PreciseTimer);
    isiTimer->setSingleShot(true);

    preTrialTimer->setTimerType(Qt::PreciseTimer);
    preTrialTimer->setInterval(1000);
    preTrialTimer->setSingleShot(true);

    connect( stimTimer, SIGNAL(timeout()), this, SLOT(pauseFlashing()) );
    connect( isiTimer, SIGNAL(timeout()), this, SLOT(startFlashing()) );
    connect( preTrialTimer, SIGNAL(timeout()), this, SLOT(startTrial()) );

    feedbackSocket = new QUdpSocket(this);
    feedbackSocket->bind(QHostAddress::LocalHost, feedbackPort);

    state = PRE_TRIAL;
}

void HybridStimulation::startTrial()
{
    qDebug()<< "[TRIAL START]" << Q_FUNC_INFO;

    if (state == PRE_TRIAL)
    {

        pre_trial();
    }

    if (state == ERPSTIMULUS)
    {
        startFlashing();

    }
    else if (state == POST_ERPSTIMULUS){

        pauseFlashing();

    }
    if(state == SSVEPSTIMULUS)
    {
        Flickering();
    }

}

void HybridStimulation::startFlashing()
{
    //    qDebug()<< Q_FUNC_INFO;

    sendMarker(OVTK_StimulationId_VisualStimulationStart);
    sendMarker(OVTK_StimulationLabel_Base + ERPFlashingSequence->sequence[currentStimulation]);

    // send target marker
    if (operationMode == CALIBRATION || operationMode == COPY_MODE)
    {
        if (isTarget()){sendMarker(OVTK_StimulationId_Target);}
        else{sendMarker(OVTK_StimulationId_NonTarget);}
    }

    if(ERPStimulationType == INVERTED_FACE)
    {
        ERPLayout->itemAt(ERPFlashingSequence->sequence[currentStimulation] - 1)->
                widget()->setStyleSheet("background-image: url(:/images/assets/bennabi_face_inverted.png);"
                                        "background-repeat: no-repeat;"
                                        "background-position: left;"
                                        );
    }

    else if(ERPStimulationType == COLORED_FACE)
    {
        switch (ERPFlashingSequence->sequence[currentStimulation])
        {
        case 1:
        {
            ERPLayout->itemAt(ERPFlashingSequence->sequence[currentStimulation] - 1)->
                    widget()->setStyleSheet("background-image: url(:/images/assets/bennabi_face_blue.png);"
                                            "background-repeat: no-repeat;"
                                            "background-position: left;"
                                            );
            break;
        }
        case 2:
        {
            ERPLayout->itemAt(ERPFlashingSequence->sequence[currentStimulation] - 1)->
                    widget()->setStyleSheet("background-image: url(:/images/assets/bennabi_face_green.png)"
                                            "background-repeat: no-repeat;"
                                            "background-position: left;"
                                            );
            break;
        }
        case 3:
        {
            ERPLayout->itemAt(ERPFlashingSequence->sequence[currentStimulation] - 1)->
                    widget()->setStyleSheet("background-image: url(:/images/assets/bennabi_face_red.png);"
                                            "background-repeat: no-repeat;"
                                            "background-position: left;"
                                            );
            break;
        }
        case 4:
        {
            ERPLayout->itemAt(ERPFlashingSequence->sequence[currentStimulation] - 1)->
                    widget()->setStyleSheet("background-image: url(:/images/assets/bennabi_face_cyan.png);"
                                            "background-repeat: no-repeat;"
                                            "background-position: left;"
                                            );
            break;
        }
        case 5:
        {
            ERPLayout->itemAt(ERPFlashingSequence->sequence[currentStimulation] - 1)->
                    widget()->setStyleSheet("background-image: url(:/images/assets/bennabi_face.png);"
                                            "background-repeat: no-repeat;"
                                            "background-position: left;"
                                            );
            break;
        }
        case 6:
        {
            ERPLayout->itemAt(ERPFlashingSequence->sequence[currentStimulation] - 1)->
                    widget()->setStyleSheet("background-image: url(:/images/assets/bennabi_face_yellow.png)"
                                            "background-repeat: no-repeat;"
                                            "background-position: left;"
                                            );
            break;
        }
        case 7:
        {
            ERPLayout->itemAt(ERPFlashingSequence->sequence[currentStimulation] - 1)->
                    widget()->setStyleSheet("background-image: url(:/images/assets/bennabi_face_magenta.png);"
                                            "background-repeat: no-repeat;"
                                            "background-position: left;"
                                            );
            break;
        }
        case 8:
        {
            ERPLayout->itemAt(ERPFlashingSequence->sequence[currentStimulation] - 1)->
                    widget()->setStyleSheet("background-image: url(:/images/assets/bennabi_face_orange.png);"
                                            "background-repeat: no-repeat;"
                                            "background-position: left;"
                                            );
            break;
        }
            //        case 9:
            //        {
            //            this->layout()->itemAt(ERPFlashingSequence->sequence[currentStimulation])->
            //                    widget()->setStyleSheet("image: url(:/images/assets/bennabi_face_magenta.png)");
            //            break;
            //        }

        }
    }

    else if(ERPStimulationType == INVERTED_COLORED_FACE)
    {
        switch (ERPFlashingSequence->sequence[currentStimulation])
        {
        case 1:
        {
            ERPLayout->itemAt(ERPFlashingSequence->sequence[currentStimulation] - 1)->
                    widget()->setStyleSheet("background-image: url(:/images/assets/bennabi_face_blue_inverted.png);"
                                            "background-repeat: no-repeat;"
                                            "background-position: left;"
                                            );
            break;
        }
        case 2:
        {
            ERPLayout->itemAt(ERPFlashingSequence->sequence[currentStimulation] - 1)->
                    widget()->setStyleSheet("background-image: url(:/images/assets/bennabi_face_green_inverted.png);"
                                            "background-repeat: no-repeat;"
                                            "background-position: left;"
                                            );
            break;
        }
        case 3:
        {
            ERPLayout->itemAt(ERPFlashingSequence->sequence[currentStimulation] - 1)->
                    widget()->setStyleSheet("background-image: url(:/images/assets/bennabi_face_red_inverted.png);"
                                            "background-repeat: no-repeat;"
                                            "background-position: left;"
                                            );
            break;
        }
        case 4:
        {
            ERPLayout->itemAt(ERPFlashingSequence->sequence[currentStimulation] - 1)->
                    widget()->setStyleSheet("background-image: url(:/images/assets/bennabi_face_cyan_inverted.png);"
                                            "background-repeat: no-repeat;"
                                            "background-position: left;"
                                            );
            break;
        }
        case 5:
        {
            ERPLayout->itemAt(ERPFlashingSequence->sequence[currentStimulation] - 1)->
                    widget()->setStyleSheet("background-image: url(:/images/assets/bennabi_face_inverted.png);"
                                            "background-repeat: no-repeat;"
                                            "background-position: left;"
                                            );
            break;
        }
        case 6:
        {
            ERPLayout->itemAt(ERPFlashingSequence->sequence[currentStimulation] - 1)->
                    widget()->setStyleSheet("background-image: url(:/images/assets/bennabi_face_yellow_inverted.png);"
                                            "background-repeat: no-repeat;"
                                            "background-position: left;"
                                            );
            break;
        }
        case 7:
        {
            ERPLayout->itemAt(ERPFlashingSequence->sequence[currentStimulation] - 1)->
                    widget()->setStyleSheet("background-image: url(:/images/assets/bennabi_face_magenta_inverted.png);"
                                            "background-repeat: no-repeat;"
                                            "background-position: left;"
                                            );
            break;
        }
        case 8:
        {
            ERPLayout->itemAt(ERPFlashingSequence->sequence[currentStimulation] - 1)->
                    widget()->setStyleSheet("background-image: url(:/images/assets/bennabi_face_orange_inverted.png);"
                                            "background-repeat: no-repeat;"
                                            "background-position: left;"
                                            );
            break;
        }
            //        case 9:
            //        {
            //            this->layout()->itemAt(ERPFlashingSequence->sequence[currentStimulation])->
            //                    widget()->setStyleSheet("image: url(:/images/assets/bennabi_face_magenta_inverted.png)");
            //            break;
            //        }

        }
    }

    stimTimer->start();
    isiTimer->stop();
    state = POST_ERPSTIMULUS;
}

void HybridStimulation::pauseFlashing()
{
    //    qDebug()<< Q_FUNC_INFO;
    // sendMarker(OVTK_StimulationId_VisualStimulationStop);
    ERPLayout->itemAt(ERPFlashingSequence->sequence[currentStimulation] - 1)->
            widget()->setStyleSheet("QLabel { color : gray; font: 40pt }");

    stimTimer->stop();
    isiTimer->start();
    //    qDebug("Isi Timer started");
    currentStimulation++;
    //    state = ERPSTIMULUS;

    if (currentStimulation >= ERPFlashingSequence->sequence.count())
    {
        currentLetter++;
        //        qDebug("STOPPED: isi Timer & Stim timer");
        isiTimer->stop();
        stimTimer->stop();

        //        if(currentLetter >= desired_phrase.length())
        //        {
        //            qDebug("Experiment End");

        //        }
        //        else
        //        {
        wait(1000);
        sendMarker(OVTK_StimulationId_TrialStop);
        //        state = FEEDBACK;

        state = SSVEPSTIMULUS;
        startTrial();

        //        if(operationMode == COPY_MODE || operationMode == FREE_MODE){
        //            feedback();
        //        }
        //        else if(operationMode == CALIBRATION)
        //        {
        //            post_trial();
        //        }
    }
}

void HybridStimulation::Flickering()
{
    qDebug()<<Q_FUNC_INFO;
    sendMarker(OVTK_StimulationId_VisualSteadyStateStimulationStart);

    QTime dieTime = QTime::currentTime().addMSecs(SSVEPStimulationDuration * 1000);
    QTime elapsedTime = QTime::currentTime();
    QTime elapsedTime1 = elapsedTime;
    QTime elapsedTime2 = elapsedTime;
    QTime elapsedTime3 = elapsedTime;

    int counter = 0;
    bool fl = false;
    bool fl1 = false;
    bool fl2 = false;
    bool fl3 = false;

    while( QTime::currentTime() < dieTime )

    {
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        if( QTime::currentTime() == elapsedTime.addMSecs((1/frequencies[0])*500) )
        {
            elapsedTime = QTime::currentTime() ;
            if (!fl)
            {

                SSVEPLayout->itemAt(0)->widget()->setStyleSheet("background-color: black");
                fl = true;
            }
            else
            {
                SSVEPLayout->itemAt(0)->widget()->setStyleSheet("background-color: white");
                fl = false;
            }
        }
        else if(QTime::currentTime() == elapsedTime1.addMSecs((1/frequencies[1])*500))
        {

            elapsedTime1 = QTime::currentTime() ;
            if (!fl1)
            {
                SSVEPLayout->itemAt(1)->widget()->setStyleSheet("background-color: black");
                fl1 = true;
            }
            else
            {
                SSVEPLayout->itemAt(1)->widget()->setStyleSheet("background-color: white");
                fl1 = false;
            }
        }
        else if(QTime::currentTime() == elapsedTime2.addMSecs((1/frequencies[2])*500))
        {
            elapsedTime2 = QTime::currentTime() ;
            if (!fl2)
            {
                SSVEPLayout->itemAt(2)->widget()->setStyleSheet("background-color: black");
                fl2 = true;
            }
            else
            {
                SSVEPLayout->itemAt(2)->widget()->setStyleSheet("background-color: white");
                fl2 = false;
            }
        }
        else if(QTime::currentTime() == elapsedTime3.addMSecs((1/frequencies[3])*500))
        {
            elapsedTime3 = QTime::currentTime() ;
            if (!fl3)
            {
                SSVEPLayout->itemAt(3)->widget()->setStyleSheet("background-color: black");
                fl3 = true;
            }
            else
            {
                SSVEPLayout->itemAt(3)->widget()->setStyleSheet("background-color: white");
                fl3 = false;
            }
        }

    }

    sendMarker(OVTK_StimulationId_VisualSteadyStateStimulationStop);
    state = POST_TRIAL;

    if(operationMode == COPY_MODE || operationMode == FREE_MODE){
        feedback();
    }
    else if(operationMode == CALIBRATION)
    {
        post_trial();
    }
}

void HybridStimulation::pre_trial()
{
    qDebug()<< Q_FUNC_INFO;

    if (pre_trial_count == 0)
    {
        ERPFlashingSequence = new RandomFlashSequence(nrERPElements, nrERPSequence);

        //        SSVEPFlickeringSequence = new RandomFlashSequence(nrSSVEPElements, 1);
        sendMarker(OVTK_StimulationId_TrialStart);

        if (operationMode == CALIBRATION)
        {
            qDebug()<< "highlightTarget";
            highlightTarget();
            text_row += desiredSequence[currentLetter];
            textRow->setText(text_row);
        }
        else if(operationMode == COPY_MODE)
        {
            highlightTarget();
        }
    }
    //    qDebug()<< "Pre trial timer start";

    preTrialTimer->start();
    pre_trial_count++;

    if (pre_trial_count > pre_trial_wait)
    {
        refreshTarget();
        preTrialTimer->stop();
        pre_trial_count = 0;
        state = ERPSTIMULUS;
    }

}

void HybridStimulation::feedback()
{
    qDebug() << Q_FUNC_INFO;

    receiveFeedback();
    textRow->setText(text_row);
    if (operationMode == COPY_MODE)
    {

        if(text_row[currentLetter - 1] == desiredSequence[currentLetter - 1])
        {
            ERPLayout->layout()->itemAt(currentTarget)->
                    widget()->setStyleSheet("QLabel { color : green; font: 40pt }");
        }
        else
        {
            ERPLayout->layout()->itemAt(currentTarget)->
                    widget()->setStyleSheet("QLabel { color : blue; font: 40pt }");
        }

    }
    post_trial();
}

void HybridStimulation::post_trial()
{
    qDebug()<< Q_FUNC_INFO;


    currentStimulation = 0;
    state = PRE_TRIAL;
    // wait
    wait(1000);

    refreshTarget();

    if (currentLetter >= desiredSequence.length() && (operationMode == COPY_MODE || operationMode == CALIBRATION))
    {
        qDebug()<< "Experiment End";
        sendMarker(OVTK_StimulationId_ExperimentStop);
        wait(2000);
        this->close();
    }
    else
    {
        startTrial();
    }

}

void HybridStimulation::receiveFeedback()
{
    // wait for OV python script to write in UDP feedback socket
    wait(200);
    QHostAddress sender;
    quint16 senderPort;
    QByteArray *buffer = new QByteArray();

    buffer->resize(feedbackSocket->pendingDatagramSize());
    qDebug() << "buffer size" << buffer->size();

    feedbackSocket->readDatagram(buffer->data(), buffer->size(), &sender, &senderPort);
    feedbackSocket->waitForBytesWritten();
    text_row += buffer->data();
    qDebug()<< "Feedback Data" << buffer->data();
}

bool HybridStimulation::isTarget()
{
    //    qDebug()<<Q_FUNC_INFO;
    int index = ERPFlashingSequence->sequence[currentStimulation] - 1;
    if(desiredSequence[currentLetter]==presentedCommands[index][0])
    {
        //                qDebug()<< "letter : " << letters[row][column];
        //        qDebug()<< "desired letter: " << desiredSequence[currentLetter];
        //        qDebug()<< "flashing: "<< flashingSequence->sequence[currentStimulation];
        //        qDebug()<< "presented letter:" << desiredSequence[index];
        //        qDebug()<< "row: " << row << " column: "<< column;
        return true;
    }
    else
    {
        return false;
    }

}

void HybridStimulation::highlightTarget()
{
    int idx = 0;

    for (int i=0; i<nrERPElements; i++)
    {
        if(desiredSequence[currentLetter]==presentedCommands[idx][0])
        {
            currentTarget = idx;
            break;
        }
        idx++;
    }

    if ( (ERPStimulationType == COLORED_FACE) || (ERPStimulationType == INVERTED_COLORED_FACE) )
    {
        switch (currentTarget)
        {
        case 0:
        {
            ERPLayout->itemAt(currentTarget)->widget()->setStyleSheet("QLabel { color : blue; font: 60pt }");
            break;
        }
        case 1:
        {
            ERPLayout->itemAt(currentTarget)->widget()->setStyleSheet("QLabel { color : green; font: 60pt }");
            break;
        }
        case 2:
        {
            ERPLayout->itemAt(currentTarget)->widget()->setStyleSheet("QLabel { color : red; font: 60pt }");
            break;
        }
        case 3:
        {
            ERPLayout->itemAt(currentTarget)->widget()->setStyleSheet("QLabel { color : cyan; font: 60pt }");
            break;
        }
        case 4:
        {
            ERPLayout->itemAt(currentTarget)->widget()->setStyleSheet("QLabel { color : white; font: 60pt }");
            break;
        }
        case 5:
        {
            ERPLayout->itemAt(currentTarget)->widget()->setStyleSheet("QLabel { color : yellow; font: 60pt }");
            break;
        }
        case 6:
        {
            ERPLayout->itemAt(currentTarget)->widget()->setStyleSheet("QLabel { color : magenta; font: 60pt }");
            break;
        }
        case 7:
        {
            ERPLayout->itemAt(currentTarget)->widget()->setStyleSheet("QLabel { color : orange; font: 60pt }");
            break;
        }

        }
    }
    else
        ERPLayout->itemAt(currentTarget)->widget()->setStyleSheet("QLabel { color : red; font: 60pt }");
}

void HybridStimulation::refreshTarget()
{

    ERPLayout->itemAt(currentTarget)->widget()->setStyleSheet("QLabel { color : gray; font: 40pt }");

}

void HybridStimulation::wait(int millisecondsToWait)
{

    qDebug()<< Q_FUNC_INFO;

    // from stackoverflow question:
    // http://stackoverflow.com/questions/3752742/how-do-i-create-a-pause-wait-function-using-qt
    QTime dieTime = QTime::currentTime().addMSecs( millisecondsToWait );
    while( QTime::currentTime() < dieTime )
    {
        //        qDebug()<<"waiting..."<<QTime::currentTime();
        QCoreApplication::processEvents( QEventLoop::AllEvents, 100);

    }
}

/* Setters */
void HybridStimulation::setERPStimulationType(int value)
{
    ERPStimulationType = value;
}

void HybridStimulation::setFrequencies(QString freqs)
{
    QStringList freqsList = freqs.split(',');

    foreach(QString str, freqsList)
    {

        frequencies.append(str.toFloat());
    }
}

void HybridStimulation::setSSVEPStimulationDuration(int stimDuration)
{
    SSVEPStimulationDuration = stimDuration;
}

void HybridStimulation::setFeedbackPort(quint16 value)
{
    feedbackPort = value;
}

void HybridStimulation::setERPStimulationDuration(int value)
{
    ERPstimulationDuration = value;
    stimTimer->setInterval(ERPstimulationDuration);
}

void HybridStimulation::setDesiredSequence(const QString &value)
{
    desiredSequence = value;
}

void HybridStimulation::setOperationMode(int value)
{
    operationMode = value;
}

void HybridStimulation::setERPNrtrials(int value)
{
    nrERPTrials = value;
}

void HybridStimulation::setERPNrSequence(int value)
{
    nrERPSequence = value;
}

void HybridStimulation::setIsi(int value)
{
    isi = value;
    isiTimer->setInterval(isi);
}

void HybridStimulation::create_layout()
{
    nrERPElements = 8;
    nrSSVEPElements = 4;
    ERPLayout = new EllipseLayout("upper");
    SSVEPLayout = new EllipseLayout("lower");
    QGridLayout *layout = new QGridLayout;

    textRow = new QLabel();
    textRow->setText(desiredSequence);
    textRow->setStyleSheet("font:30pt; color:gray; border-color:white;");
    textRow->setAlignment(Qt::AlignCenter);

    for(int i=1; i<=nrERPElements;i++)
    {
        QLabel *lb = new QLabel;
        lb->setText(QString::number(i));
        lb->setStyleSheet("font: 40pt; color:gray");
        ERPLayout->addWidget(lb);
        presentedCommands.append(QString::number(i));
    }

    for(int i=1; i<=nrSSVEPElements; i++)
    {
        QLabel *ssvepElement = new QLabel;
        ssvepElement->setStyleSheet("background-color: rgb(255, 255, 255)");
        ssvepElement->setMaximumSize(QSize(100, 100));
        SSVEPLayout->addWidget(ssvepElement);
    }

    layout->addWidget(textRow,0,0,1,0);
    layout->addLayout(ERPLayout,2,0,2,2);
    layout->addLayout(SSVEPLayout,3,1);
    this->setLayout(layout);
}

void HybridStimulation::refresh_layout()
{

}

HybridStimulation::~HybridStimulation()
{
    delete ui;
}
