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
#include <QPixmap>
#include <QGraphicsRectItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsItemAnimation>
#include <QTimeLine>
#include <QBrush>
//
#include "mvepspeller.h"
#include "ui_mvepspeller.h"
#include "motionitem.h"
#include "configpanel.h"
#include "ovtk_stimulations.h"
#include "randomflashsequence.h"

static QChar letters[6][6] = {
    {'A','B','C','D','E','F'},
    {'G','H','I','J','K','L'},
    {'M','N','O','Q','P','R'},
    {'S','T','U','V','W','X'},
    {'Y','Z','0','1','2','3'},
    {'4','5','6','7','8','9'},
};

const quint8 PRE_TRIAL = 0;
const quint8 STIMULUS = 1;
const quint8 POST_STIMULUS = 2;
const quint8 FEEDBACK = 3;
const quint8 POST_TRIAL = 4;

const quint8 FLASHING_SPELLER = 0;
const quint8 FACES_SPELLER = 1;
const quint8 MOTION_BAR = 2;
const quint8 MOTION_FACE = 3;

const quint8 CALIBRATION = 0;
const quint8 COPY_MODE  = 1;
const quint8 FREE_MODE = 2;

const uint64_t OVTK_StimulationLabel_Base = 0x00008100;

mVEPSpeller::mVEPSpeller(quint8 spellerType, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::mVEPSpeller)
{
    qDebug()<< Q_FUNC_INFO;

    ui->setupUi(this);
    this->show();
    this->windowHandle()->setScreen(qApp->screens().last());
    this->showFullScreen();

    speller_type = spellerType;
    animation = new QGraphicsItemAnimation;

    create_layout();

    stimTimer = new QTimer(this);
    isiTimer = new QTimer(this);
    preTrialTimer = new QTimer(this);
    animTimer = new QTimeLine();

    animation->setTimeLine(animTimer);

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
//    connect( animTimer, SIGNAL(finished()), this, SLOT(pauseFlashing()));

    feedback_socket = new QUdpSocket(this);
    feedback_socket->bind(QHostAddress::LocalHost, feedback_port);

    state = PRE_TRIAL;
}

void mVEPSpeller::startTrial()
{
    qDebug()<< "[TRIAL START]" << Q_FUNC_INFO;

    if (state == PRE_TRIAL)
    {
        pre_trial();
    }

    if (state == STIMULUS)
    {
        startFlashing();
    }
    else
        if (state == POST_STIMULUS)
        {
            pauseFlashing();
        }
}

void mVEPSpeller::pre_trial()
{
    qDebug()<< Q_FUNC_INFO;

    if (pre_trial_count == 0)
    {
        flashingSequence = new RandomFlashSequence(nr_elements, nr_sequence);
        sendMarker(OVTK_StimulationId_TrialStart);

        if (spelling_mode == CALIBRATION || spelling_mode == COPY_MODE)
        {
            //            highlightTarget();
            text_row += desired_phrase[currentLetter];
            textRow->setText(text_row);
        }
        else if(spelling_mode == FREE_MODE)
        {
            //            highlightTarget();
        }
    }

    preTrialTimer->start();
    pre_trial_count++;

    if (pre_trial_count > pre_trial_wait)
    {
        //        refreshTarget();
        preTrialTimer->stop();
        pre_trial_count = 0;
        state = STIMULUS;
         if(speller_type==MOTION_FACE)
             distance  = itemsList[flashingSequence->sequence[currentStimulation]]->boundingRect().width() -
                         image_stimuli->boundingRect().width();
         else
              distance  = itemsList[flashingSequence->sequence[currentStimulation]]->boundingRect().width();
        //        qDebug()<<"Let's do stimulus!;
    }
}

void mVEPSpeller::startFlashing()
{
    qDebug()<< Q_FUNC_INFO;
    //    " Current Iteration"<<currentStimulation;

    //    stimTimer->start();

    sendMarker(OVTK_StimulationId_VisualStimulationStart);
    sendMarker(OVTK_StimulationLabel_Base + flashingSequence->sequence[currentStimulation]);

    // send target marker
    if (spelling_mode == CALIBRATION || spelling_mode == COPY_MODE)
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

    if(speller_type == MOTION_FACE)
    {
        qreal y = itemsList[flashingSequence->sequence[currentStimulation]]->y();
        qreal x = itemsList[flashingSequence->sequence[currentStimulation]]->x() +
                itemsList[flashingSequence->sequence[currentStimulation]]->boundingRect().width() -
                image_stimuli->boundingRect().width();

        image_stimuli->setPos(x, y);

        // inverted face
        //        image_stimuli->setPos(x+50, y + 100);
        //        image_stimuli->setRotation(180);
        //
        image_stimuli->show();
//        distance -= image_stimuli->boundingRect().width();
        distance = 20;
        for (int i = 0; i < distance; i+=2)
        {
            animation->setPosAt(i / qreal(distance), QPointF(x - i, y));
        }
    }
    else
        if(speller_type == MOTION_BAR)
        {
            qreal y = itemsList[flashingSequence->sequence[currentStimulation]]->y();
            qreal x = itemsList[flashingSequence->sequence[currentStimulation]]->x() +
                    itemsList[flashingSequence->sequence[currentStimulation]]->boundingRect().width() -
                    bar_stimuli->boundingRect().width();
            //            int distance = (itemsList[flashingSequence->sequence[currentStimulation]]->boundingRect().width() / 2);
            //            animation->setItem(itemsList[flashingSequence->sequence[currentStimulation]]);
            //            int distance = 40;

            bar_stimuli->setPos(x, y);
            //            animation->reset();
            bar_stimuli->show();
            distance = 20;

            for (int i = 0; i < distance; i+=2)
            {
                animation->setPosAt(i / (qreal(distance)), QPointF(x - i, y));
            }
            //            animTimer->setFrameRange(1,10);

        }
    qDebug()<<"Anim Pos List: "<<animation->posList().count();
    stimTimer->start();

    //    sendMarker(OVTK_StimulationId_VisualStimulationStart);
    //    sendMarker(OVTK_StimulationLabel_Base + flashingSequence->sequence[currentStimulation]);
//    animTimer->start();
    isiTimer->stop();
    state = POST_STIMULUS;
}

void mVEPSpeller::pauseFlashing()
{
    qDebug()<< Q_FUNC_INFO;
//     sendMarker(OVTK_StimulationId_VisualStimulationStop);

    if(speller_type == MOTION_BAR)
    {
        bar_stimuli->hide();
    }
    else
    {
        image_stimuli->hide();
    }

    animTimer->stop();
    stimTimer->stop();
    isiTimer->start();

    currentStimulation++;
    state = STIMULUS;

    if (currentStimulation >= flashingSequence->sequence.count())
    {
        currentLetter++;
        //                qDebug("STOPPED: isi Timer & Stim timer");
        isiTimer->stop();
        stimTimer->stop();

        //        if(currentLetter >= desired_phrase.length())
        //        {
        //            qDebug("Experiment End");

        //        }
        //        else
        //        {
        sendMarker(OVTK_StimulationId_TrialStop);
        state = FEEDBACK;

        if(spelling_mode == COPY_MODE || spelling_mode == FREE_MODE)
        {
            feedback();
        }
        else if(spelling_mode == CALIBRATION)
        {
            post_trial();
        }
    }
}
void mVEPSpeller::feedback()
{

    qDebug() << Q_FUNC_INFO;

    receiveFeedback();
    textRow->setText(text_row);
    if (spelling_mode == COPY_MODE)
    {

        if(text_row[currentLetter - 1] == desired_phrase[currentLetter - 1])
        {
            //            this->layout()->itemAt(currentTarget)->
            //                    widget()->setStyleSheet("QLabel { color : green; font: 40pt }");
        }
        else
        {
            //            this->layout()->itemAt(currentTarget)->
            //                    widget()->setStyleSheet("QLabel { color : blue; font: 40pt }");
        }

    }
    post_trial();
}

void mVEPSpeller::post_trial()
{
    qDebug()<< Q_FUNC_INFO;


    if(speller_type == MOTION_BAR)
        bar_stimuli->hide();
    else
        image_stimuli->hide();

    currentStimulation = 0;
    state = PRE_TRIAL;
    // wait
    wait(1000);

    //    refreshTarget();

    if (currentLetter >= desired_phrase.length() && (spelling_mode == COPY_MODE || spelling_mode == CALIBRATION)){
        qDebug()<< "Experiment End";
        sendMarker(OVTK_StimulationId_ExperimentStop);
        wait(2000);
        this->close();
    }
    else{
        startTrial();
    }
}

void mVEPSpeller::receiveFeedback()
{
    // wait for OV python script to write in UDP feedback socket
    wait(200);
    QHostAddress sender;
    quint16 senderPort;
    QByteArray *buffer = new QByteArray();

    buffer->resize(feedback_socket->pendingDatagramSize());
    qDebug() << "buffer size" << buffer->size();

    feedback_socket->readDatagram(buffer->data(), buffer->size(), &sender, &senderPort);
    feedback_socket->waitForBytesWritten();
    text_row += buffer->data();
    qDebug()<< "Feedback Data" << buffer->data();
}

void mVEPSpeller::create_layout()
{

    qDebug()<< Q_FUNC_INFO;
    // speller settings
    rows = 3;
    cols = 3;
    nr_elements = rows * cols;
    matrix_width = 800;//1300;
    matrix_height = 600;//700;
    this->setStyleSheet("background-color:white");
    //    this->setGeometry(0, 0, matrix_width, matrix_height);

    QGridLayout *layout = new QGridLayout();

    textRow = new QLabel(this);
    textRow->setText(desired_phrase);
    textRow->setStyleSheet("font:40pt; color:gray; border-color:white;");
    textRow->setAlignment(Qt::AlignCenter);
    //    textRow->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(textRow,0,0,1,0);

    view = new QGraphicsView(this);
    view->setStyleSheet("border:transparent");
    QBrush brush(Qt::black);
    view->setBackgroundBrush(brush);
    layout->addWidget(view);

    QGraphicsScene *scene = new QGraphicsScene(this);

    //    view->setAlignment(Qt::AlignTop);
    view->setScene(scene);



    Mlayout = new MatrixLayout(qMakePair(matrix_width, matrix_height), rows, cols);
    //    qDebug()<< view->width() <<" "<< view->height();
    //    qDebug()<< Mlayout->positions;

    int k=0;
    for (int row=0; row<rows; row++)
    {
        for(int col=0; col<cols; col++)
        {
            //            MotionItem *item = new MotionItem(letters[row][col]);
            MotionItem *item = new MotionItem(QString::number(k+1));

            item->setPos(Mlayout->positions[k].first, Mlayout->positions[k].second);
            scene->addItem(item);
            //            presented_letters.append(letters[row][col]);
            presented_letters.append(QString::number(k+1));
            k++;
        }
    }

    qDebug()<< Q_FUNC_INFO << "speller type "<< speller_type;
//    QBrush brush(Qt::black);
    scene->setBackgroundBrush(brush);
//    view->setBackgroundBrush(brush);
    switch(speller_type)
    {
    case MOTION_BAR :
    {
        QBrush b(Qt::red);
        bar_stimuli = new QGraphicsRectItem(0, 0, 8, 75);
        bar_stimuli->hide();
        bar_stimuli->setBrush(b);
        bar_stimuli->setPos(0, 0);
        scene->addItem(bar_stimuli);
        animation->setItem(bar_stimuli);
        qDebug()<< Q_FUNC_INFO << "Setting the bar stimuli";
        break;
    }
    case MOTION_FACE :
    {

        face_stimuli = QImage(":/images/assets/bennabi_face.png");

//        face_stimuli = face_stimuli.scaled(50, 75, Qt::IgnoreAspectRatio);
        image_stimuli = new QGraphicsPixmapItem(QPixmap::fromImage(face_stimuli));

        image_stimuli->hide();
        image_stimuli->setPos(0, 0);
        scene->addItem(image_stimuli);
        animation->setItem(image_stimuli);
        qDebug()<< Q_FUNC_INFO << "Setting the face stimuli";
        break;
    }
    }

    itemsList = scene->items();
    this->setLayout(layout);

}

void mVEPSpeller::refresh_layout()
{

}

bool mVEPSpeller::isTarget()
{
    //    int row, column;
    int index = flashingSequence->sequence[currentStimulation] - 1;
    //    row = index / nr_elements;
    //    column = index % nr_elements;

    if(desired_phrase[currentLetter]==presented_letters[index][0])
    {
        //                qDebug()<< "letter : " << letters[row][column];
        qDebug()<< "desired letter: " << desired_phrase[currentLetter];
        //        qDebug()<< "flashing: "<< flashingSequence->sequence[currentStimulation];
        qDebug()<< "presented letter:" << presented_letters[index];
        //        qDebug()<< "row: " << row << " column: "<< column;
        return true;
    }
    else
    {
        return false;
    }

}

void mVEPSpeller::highlightTarget()
{

}

void mVEPSpeller::refreshTarget()
{

}

void mVEPSpeller::wait(int millisecondsToWait)
{

    qDebug()<< Q_FUNC_INFO;
    // from stackoverflow question:
    // http://stackoverflow.com/questions/3752742/how-do-i-create-a-pause-wait-function-using-qt
    QTime dieTime = QTime::currentTime().addMSecs( millisecondsToWait );
    while( QTime::currentTime() < dieTime )
    {
        QCoreApplication::processEvents( QEventLoop::AllEvents, 100 );
    }
}

/* Setters */
void mVEPSpeller::setSpeller_type(quint16 value)
{
    speller_type = value;
}

void mVEPSpeller::setFeedbackPort(quint16 value)
{
    feedback_port = value;
}

void mVEPSpeller::setAnimTimeUpdateIntervale(int value)
{
    // 20 ?
    animTimer->setUpdateInterval(value-20);
}

void mVEPSpeller::setStimulation_duration(int value)
{
    stimulation_duration = value;
    stimTimer->setInterval(stimulation_duration);
    animTimer->setDuration(value);
}

void mVEPSpeller::setDesired_phrase(const QString &value)
{

    desired_phrase = value;

}

void mVEPSpeller::setSpelling_mode(int value)
{
    spelling_mode = value;
}

void mVEPSpeller::setNr_trials(int value)
{
    nr_trials = value;
}

void mVEPSpeller::setNr_sequence(int value)
{
    nr_sequence = value;
}

void mVEPSpeller::setIsi(int value)
{
    isi = value;
    isiTimer->setInterval(isi);
}

mVEPSpeller::~mVEPSpeller()
{
    delete ui;
}

