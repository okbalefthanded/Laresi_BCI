//
#include <QWindow>
#include <QDebug>
#include <QLabel>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QTime>
#include <QElapsedTimer>
#include <windows.h>
#include <QtMath>
#include <Qapplication>
//
#include "ssvepgl.h"
#include "ovtk_stimulations.h"
#include "utils.h"
#include "glutils.h"
//
SsvepGL::SsvepGL(SSVEP paradigm)
{

    qDebug()<< Q_FUNC_INFO;

    m_nrElements = paradigm.nrElements();
    setControlMode(paradigm.controlMode());
    setFrequencies(paradigm.frequencies());
    setStimulationDuration(paradigm.stimulationDuration());
    setBreakDuration(paradigm.breakDuration());
    setSequence(paradigm.nrSequences());
    setFlickeringMode(paradigm.experimentMode());
    setFeedbackPort(12345);
    setStimulationMode(paradigm.stimulationMode());

    // set vertices, vertices indeices & colors
    initElements();

    m_preTrialTimer = new QTimer(this);
    m_preTrialTimer->setTimerType(Qt::PreciseTimer);
    m_preTrialTimer->setInterval(1000);
    m_preTrialTimer->setSingleShot(true);

    connect(m_preTrialTimer, SIGNAL(timeout()), this, SLOT(startTrial()) );

    m_feedbackSocket = new QUdpSocket(this);
    m_feedbackSocket->bind(QHostAddress::LocalHost, m_feedbackPort);

    m_state = trial_state::PRE_TRIAL;

}

void SsvepGL::initializeGL()
{

    qDebug()<< Q_FUNC_INFO;

    // Initialize OpenGL Backend
    initializeOpenGLFunctions();
    m_index = 0;
    // Set global information
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_FRAMEBUFFER_SRGB);


    if(  QGuiApplication::screens().size() == 2)
    {
        this->setScreen(QGuiApplication::screens().last());
        this->showFullScreen();
    }

    // set vertices, vertices indeices & colors
    // initElements();

    // static const int samplesLength = REFRESH_RATE * (stimulationDuration);

    m_flicker.resize(m_frequencies.size());
    for (int i=0; i < m_frequencies.size(); ++i)
    {
        m_flicker[i] = utils::gen_flick(m_frequencies[i], config::REFRESH_RATE, m_stimulationDuration, m_stimulationMode);
    }

    // Application-specific initialization
    {
        // Create shaders (Do not release until VAO is created)
        m_programShader = new QOpenGLShaderProgram();
        m_programShader->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/src/shaders/sh_v.vert");
        m_programShader->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/src/shaders/sh_f.frag");
        m_programShader->link();
        m_programShader->bind();

        // Create buffer (Do not release until VAO is created)
        m_vertexBuffer.create();
        m_vertexBuffer.bind();
        m_vertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
        m_vertexBuffer.allocate(m_vertices.data(), m_vertices.count() * sizeof(QVector3D)); //

        m_colorBuffer.create();
        m_colorBuffer.bind();
        m_colorBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
        m_colorBuffer.allocate(m_colors.data(), m_colors.count() * sizeof(QVector3D));

        // Create Vertex Array Object
        m_vaObject.create();

        m_vaObject.bind();
        m_vertexBuffer.bind();
        m_programShader->enableAttributeArray(0);
        m_programShader->setAttributeBuffer(0, GL_FLOAT, 0, glUtils::TUPLESIZE, 0);
        m_colorBuffer.bind();
        m_programShader->enableAttributeArray(1);
        m_programShader->setAttributeBuffer(1, GL_FLOAT, 0, glUtils::TUPLESIZE, 0);
        m_vaObject.release();

        // Release (unbind) all
        m_vaObject.release();
        m_vertexBuffer.release();
        m_colorBuffer.release();
        m_programShader->release();
    }
}

void SsvepGL::resizeGL(int w, int h)
{
    //TODO
    //    (void)w;
    //    (void)h;
    initElements();
}

void SsvepGL::paintGL()
{
    //    qDebug()<< Q_FUNC_INFO;
    // clear
    glClear(GL_COLOR_BUFFER_BIT);
    int nVertices = glUtils::VERTICES_PER_TRIANGLE * (m_nrElements) * glUtils::TRIANGLES_PER_SQUARE;
    // Render using our shader
    m_programShader->bind();
    {
        m_vaObject.bind();
        // nr_triangles = 3 * Nr_elments * 2 (3 : vertices per triangle), (2: 2 triangles per rectangle)
        glDrawElements(GL_TRIANGLES, nVertices, GL_UNSIGNED_INT, m_vindices.data());
        m_vaObject.release();
    }
    m_programShader->release();

}

void SsvepGL::startTrial()
{

    qDebug()<< "[TRIAL START]" << Q_FUNC_INFO;

    if (m_state == trial_state::PRE_TRIAL)
    {
        preTrial();
    }
    if (m_state == trial_state::STIMULUS)
    {
        Flickering();
    }
    if (m_state == trial_state::POST_TRIAL)
    {
        postTrial();
    }
}

void SsvepGL::preTrial()
{

    qDebug()<< Q_FUNC_INFO;

    if(m_firstRun)
    {
        //  qDebug()<< nr_elements << stimulationSequence << stimulationSequence / nr_elements;
        m_flickeringSequence = new RandomFlashSequence(m_nrElements, m_stimulationSequence / m_nrElements);
        qDebug()<<"sequence"<<m_flickeringSequence->sequence;
        m_firstRun = false;
    }

    if (m_preTrialCount == 1)
    {

        sendMarker(OVTK_StimulationId_TrialStart);

        if (m_flickeringMode == operation_mode::CALIBRATION ||
                m_flickeringMode == operation_mode::COPY_MODE)
        {
            //            qDebug()<< "highlightTarget";
            highlightTarget();
            //            text_row += desired_phrase[currentLetter];
            //            textRow->setText(text_row);
        }
        //        else if(m_flickeringMode == operation_mode::COPY_MODE)
        //        {
        //            highlightTarget();
        //        }
    }
    else if(m_preTrialCount == 3)
    {
        refreshTarget();
    }
    //    qDebug()<< "Pre trial timer start";

    m_preTrialTimer->start();
    m_preTrialCount++;

    if (m_preTrialCount > m_preTrialWait)
    {
        refreshTarget();
        m_preTrialTimer->stop();
        m_preTrialCount = 0;
        m_state = trial_state::STIMULUS;

    }

}

void SsvepGL::feedback()
{
    qDebug()<< Q_FUNC_INFO;

    receiveFeedback();

    if(m_flickeringMode == operation_mode::COPY_MODE)
    {
        //
        qDebug()<< Q_FUNC_INFO << "current flicker" << m_flickeringSequence->sequence[m_currentFlicker];
        qDebug()<< Q_FUNC_INFO << "current feddback " << m_sessionFeedback[m_currentFlicker-1].digitValue();

        if(m_sessionFeedback[m_currentFlicker] == m_flickeringSequence->sequence[m_currentFlicker])
        {

            highlightFeedback(glColors::red, m_flickeringSequence->sequence[m_currentFlicker]);
        }
        else
        {
            highlightFeedback(glColors::orange, m_sessionFeedback[m_currentFlicker-1].digitValue());
        }
    }
    else if(m_flickeringMode == operation_mode::FREE_MODE)
    {
        //
        highlightFeedback(glColors::red, m_sessionFeedback[m_currentFlicker-1].digitValue());
    }
}

void SsvepGL::postTrial()
{

    //    sendMarker(OVTK_StimulationId_TrialStop);
    //    qDebug()<< Q_FUNC_INFO << "disconnecting frameswapped to update "<<"index"<< index;
    disconnect(this, SIGNAL(frameSwapped()), this, SLOT(update()));
    initElements();
    //    currentStimulation = 0;

    m_index = 0;
    m_state = trial_state::PRE_TRIAL;

    // feedback
    if(m_flickeringMode == operation_mode::COPY_MODE || m_flickeringMode == operation_mode::FREE_MODE)
    {

        qDebug() << Q_FUNC_INFO << "lets do feedback";
        feedback();
    }

    // feedback for 1 sec & refresh
    wait(1000);
    refresh(m_sessionFeedback[m_currentFlicker-1].digitValue());

    // wait
    int waitMillisec = m_breakDuration - m_preTrialWait * 1000;
    wait(waitMillisec);

    //    refreshTarget();

    if (m_currentFlicker >= m_flickeringSequence->sequence.size() &&
            (m_flickeringMode == operation_mode::COPY_MODE || m_flickeringMode == operation_mode::CALIBRATION))
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

void SsvepGL::Flickering()
{
    qDebug()<< Q_FUNC_INFO;

    if(m_index == 0)
    {
        // qDebug()<< Q_FUNC_INFO << "connecting frameswapped to update" << "index " << index;
        connect(this, SIGNAL(frameSwapped()), this, SLOT(update()));
        //        qDebug()<< Q_FUNC_INFO << "[update ]Index (first): "<< m_index << "current time: " << QTime::currentTime().msec();
    }

    //    sendMarker(config::OVTK_StimulationLabel_Base + m_flickeringSequence->sequence[m_currentFlicker]);
    //    sendMarker(OVTK_StimulationId_VisualSteadyStateStimulationStart);
    //    qDebug()<< Q_FUNC_INFO << "markers sent" << "current time: " << QTime::currentTime().msec();


    qDebug()<<"Stimulation "<<m_flickeringSequence->sequence[m_currentFlicker];

    while(m_index < m_flicker[0].size())
    {
        QCoreApplication::processEvents(QEventLoop::AllEvents);
    }

    sendMarker(OVTK_StimulationId_TrialStop);
    //    qDebug()<< Q_FUNC_INFO << "[update ]Index (last) : "<< m_index << "current time: " << QTime::currentTime().msec();
    ++m_currentFlicker;
    m_state = trial_state::POST_TRIAL;
}

void SsvepGL::receiveFeedback()
{
    qDebug()<< Q_FUNC_INFO;

    // wait for OV python script to write in UDP feedback socket
    wait(200);
    QHostAddress sender;
    quint16 senderPort;
    QByteArray *buffer = new QByteArray();

    buffer->resize(m_feedbackSocket->pendingDatagramSize());
    qDebug() << "buffer size" << buffer->size();

    m_feedbackSocket->readDatagram(buffer->data(), buffer->size(), &sender, &senderPort);
    //    feedback_socket->waitForBytesWritten();
    m_sessionFeedback += buffer->data();
    qDebug()<< Q_FUNC_INFO << "session feedback" << m_sessionFeedback;
    qDebug()<< Q_FUNC_INFO << "Feedback Data" << buffer->data();
}

void SsvepGL::wait(int millisecondsToWait)
{

    qDebug()<< Q_FUNC_INFO;

    // from stackoverflow question:
    // http://stackoverflow.com/questions/3752742/how-do-i-create-a-pause-wait-function-using-qt
    QTime dieTime = QTime::currentTime().addMSecs( millisecondsToWait );
    while( QTime::currentTime() < dieTime )
    {
        //        qDebug()<<"Flickering"<<QTime::currentTime();
        QCoreApplication::processEvents( QEventLoop::AllEvents, 100);

    }

}

void SsvepGL::initElements()
{

    qDebug()<< Q_FUNC_INFO;
    double dx = 0.2;
    double dy = 0.25;
    int isNullX = 0, isNullY = 0, sx=1;

    if(m_nrElements == 1)
    {
        m_vertices.resize(glUtils::POINTS_PER_SQUARE);
        m_vertices[0] = refPoints::topPoints[0];
        m_colors.resize(glUtils::POINTS_PER_SQUARE);
        m_colors[0] = glColors::white;
        for(int i=1; i<glUtils::POINTS_PER_SQUARE; ++i)
        {
            // init points
            isNullX = i % 2;
            isNullY = (i+1) % 2;
            m_vertices[i].setX(m_vertices[i-1].x() + (dx * isNullX *sx));
            m_vertices[i].setY(m_vertices[i-1].y() - (dy * isNullY));
            m_vertices[i].setZ(refPoints::topPoints[0].z());
            sx--;
            // init colors
            m_colors[i] = glColors::white;
        }

    }
    else
    {
        // init vectors
        int vectorsSize = m_nrElements * glUtils::POINTS_PER_SQUARE;
        m_vertices.resize(vectorsSize);
        initRects();
        initColors();


        /*
        // synchrounous mode : no idle state
        if(m_controlMode == control_mode::SYNC)
        {
            initRects();
            initColors();
        }
        // asynchrounous mode : with idle state
        else{

            //            for(int i=0; i<m_vertices.count(); i+=glUtils::POINTS_PER_SQUARE)
            //            {
            //                m_vertices[i] = refPoints::topPoints[i/glUtils::POINTS_PER_SQUARE];
            //                sx = 1;
            //                for(int j=i+1; j<i+glUtils::POINTS_PER_SQUARE; ++j)
            //                {
            //                    isNullX = j % 2;
            //                    isNullY = (j+1) % 2;
            //                    m_vertices[j].setX(m_vertices[j-1].x() + (dx * isNullX * sx));
            //                    m_vertices[j].setY(m_vertices[j-1].y() - (dy * isNullY));
            //                    m_vertices[j].setZ(refPoints::topPoints[0].z());
            //                    sx--;
            //                }
            //            }
            // init colors
            m_colors = {glColors::gray, glColors::gray, glColors::gray, glColors::gray};
            m_colors.resize(vectorsSize);
            for (int i=glUtils::POINTS_PER_SQUARE; i<m_colors.count(); i++)
            {
                m_colors[i] = glColors::white;
            }
        } */
    }

    // init indices
    m_vindices.resize(m_nrElements*glUtils::INDICES_PER_SQUARE);
    int k=0; int val = 0;
    for(int i=0; i<(m_nrElements*glUtils::INDICES_PER_SQUARE); i+=glUtils::INDICES_PER_SQUARE)
    {
        val = 2*k;
        m_vindices[i] =  val;
        m_vindices[i+1] = val + 1;
        m_vindices[i+2] = val + 2;
        m_vindices[i+3] = m_vindices[i];
        m_vindices[i+4] = m_vindices[i+2];
        m_vindices[i+5] = val + 3;
        k +=2;
    }

    m_vaObject.bind();
    m_colorBuffer.bind();
    m_colorBuffer.write(0, m_colors.data(), m_colors.count() * sizeof(QVector3D)); // number of vertices to avoid * sizeof QVector3D
    m_vaObject.release();
    m_colorBuffer.release();

    QOpenGLWindow::update();
}

void SsvepGL::initRects()
{
    double dx = 0.2;
    double dy = 0.25;
    int isNullX = 0, isNullY = 0, sx=1;

    for(int i=0; i<m_vertices.count(); i+=glUtils::POINTS_PER_SQUARE)
    {
        m_vertices[i] = refPoints::topPoints[i/glUtils::POINTS_PER_SQUARE];
        sx = 1;
        for(int j=i+1; j<i+glUtils::POINTS_PER_SQUARE; ++j)
        {
            isNullX = j % 2;
            isNullY = (j+1) % 2;
            m_vertices[j].setX(m_vertices[j-1].x() + (dx * isNullX * sx));
            m_vertices[j].setY(m_vertices[j-1].y() - (dy * isNullY));
            m_vertices[j].setZ(refPoints::topPoints[0].z());
            sx--;
        }
    }
}

void SsvepGL::initColors()
{
    qDebug()<< Q_FUNC_INFO;

    int vectorsSize = m_nrElements * glUtils::POINTS_PER_SQUARE;
    m_colors.resize(vectorsSize);

    if(m_controlMode == control_mode::SYNC)
    {
        qDebug()<< "[control mode] : SYNC" ;
        qDebug()<< "[colors : ]" << m_colors.count();
        for (int i=0; i<m_colors.count(); i++)
        {
            m_colors[i] = glColors::white;
        }
    }
    else
    {
        qDebug()<< "[control mode] : ASYNC" ;
        // init colors
        m_colors = {glColors::gray, glColors::gray, glColors::gray, glColors::gray};
        // m_colors.resize(vectorsSize);
        for (int i=glUtils::POINTS_PER_SQUARE; i<m_colors.count(); i++)
        {
            m_colors[i] = glColors::white;
        }
    }

}

void SsvepGL::highlightTarget()
{

    qDebug()<< Q_FUNC_INFO;

    if(m_nrElements == 1)
    {
        m_colors = {glColors::green, glColors::green, glColors::green, glColors::green};
    }
    else
    {
        int tmp = m_flickeringSequence->sequence[m_currentFlicker]-1;
        int squareIndex = tmp + (glUtils::VERTICES_PER_TRIANGLE*tmp);
        m_colors[squareIndex] = glColors::green;
        m_colors[squareIndex + 1] = glColors::green;
        m_colors[squareIndex + 2] = glColors::green;
        m_colors[squareIndex + 3] = glColors::green;
    }

    m_vaObject.bind();
    m_colorBuffer.bind();
    m_colorBuffer.write(0, m_colors.data(), m_colors.count() * sizeof(QVector3D)); // number of vertices to avoid * sizeof QVector3D
    m_vaObject.release();
    m_colorBuffer.release();
    //     Schedule a redraw
    QOpenGLWindow::update();
}

void SsvepGL::refreshTarget()
{

    qDebug()<< Q_FUNC_INFO;

    if(m_nrElements == 1)
    {
        m_colors = {glColors::white, glColors::white, glColors::white, glColors::white};
    }
    else
    {
        if(m_controlMode == control_mode::SYNC)
        {
            int tmp = m_flickeringSequence->sequence[m_currentFlicker]-1;
            //            int squareIndex = tmp+(glUtils::VERTICES_PER_TRIANGLE*tmp);
            int squareIndex = tmp;
            m_colors[squareIndex] = glColors::white;
            m_colors[squareIndex + 1] = glColors::white;
            m_colors[squareIndex + 2] = glColors::white;
            m_colors[squareIndex + 3] = glColors::white;
        }
        else
        {

            if(m_flickeringSequence->sequence[m_currentFlicker] == 1)
            {
                // center rectangle for idle state
                m_colors[0] = glColors::gray;
                m_colors[1] = glColors::gray;
                m_colors[2] = glColors::gray;
                m_colors[3] = glColors::gray;
            }
            else
            {
                int tmp = m_flickeringSequence->sequence[m_currentFlicker]-1;
                int squareIndex = tmp+(glUtils::VERTICES_PER_TRIANGLE*tmp);
                m_colors[squareIndex] = glColors::white;
                m_colors[squareIndex + 1] = glColors::white;
                m_colors[squareIndex + 2] = glColors::white;
                m_colors[squareIndex + 3] = glColors::white;
            }

        }
    }
    m_vaObject.bind();
    m_colorBuffer.bind();
    m_colorBuffer.write(0, m_colors.data(), m_colors.count() * sizeof(QVector3D)); // number of vertices to avoid * sizeof QVector3D
    m_vaObject.release();
    m_colorBuffer.release();
    //     Schedule a redraw
    QOpenGLWindow::update();
}

void SsvepGL::highlightFeedback(QVector3D feedbackColor, int feebdackIndex)
{

    qDebug()<< Q_FUNC_INFO << "[icon to highlight] "<< feebdackIndex;
    //int tmp = feebdackIndex;
    int squareIndex = feebdackIndex + (glUtils::VERTICES_PER_TRIANGLE*feebdackIndex);
    m_colors[squareIndex] = feedbackColor;
    m_colors[squareIndex + 1] = feedbackColor;
    m_colors[squareIndex + 2] = feedbackColor;
    m_colors[squareIndex + 3] = feedbackColor;

    m_vaObject.bind();
    m_colorBuffer.bind();
    m_colorBuffer.write(0, m_colors.data(), m_colors.count() * sizeof(QVector3D)); // number of vertices to avoid * sizeof QVector3D
    m_vaObject.release();
    m_colorBuffer.release();
    //     Schedule a redraw
    QOpenGLWindow::update();
}

void SsvepGL::refresh(int feedbackIndex)
{
    qDebug()<< Q_FUNC_INFO << "[icon to highlight] "<< feedbackIndex;

    int squareIndex = feedbackIndex + (glUtils::VERTICES_PER_TRIANGLE*feedbackIndex);
    m_colors[squareIndex] = glColors::white;
    m_colors[squareIndex + 1] = glColors::white;
    m_colors[squareIndex + 2] = glColors::white;
    m_colors[squareIndex + 3] = glColors::white;

    m_vaObject.bind();
    m_colorBuffer.bind();
    m_colorBuffer.write(0, m_colors.data(), m_colors.count() * sizeof(QVector3D)); // number of vertices to avoid * sizeof QVector3D
    m_vaObject.release();
    m_colorBuffer.release();
    //     Schedule a redraw
    QOpenGLWindow::update();
}



void SsvepGL::setControlMode(quint8 t_controlMode)
{
    m_controlMode = t_controlMode;
}

void SsvepGL::setStimulationMode(quint8 t_stimulationMode)
{
    m_stimulationMode = t_stimulationMode;
}

void SsvepGL::update()
{

    //    qDebug()<< "[update ]Index : "<< m_index << "current time: " << QTime::currentTime().msec();


    if(m_index == 0)
    {
        sendMarker(config::OVTK_StimulationLabel_Base + m_flickeringSequence->sequence[m_currentFlicker]);
        sendMarker(OVTK_StimulationId_VisualSteadyStateStimulationStart);
    }

    int k;

    if(m_nrElements == 1)
    {
        k = 0;
    }
    else
    {
        if (m_controlMode == control_mode::SYNC)
        {
            k = 0;
        }
        else
        {
            k = glUtils::POINTS_PER_SQUARE;
        }
    }
    for(int i = 0; i<m_flicker.size() ;++i)
    {
        m_colors[k]   = QVector3D(m_flicker[i][m_index], m_flicker[i][m_index], m_flicker[i][m_index]);
        m_colors[k+1] = QVector3D(m_flicker[i][m_index], m_flicker[i][m_index], m_flicker[i][m_index]);
        m_colors[k+2] = QVector3D(m_flicker[i][m_index], m_flicker[i][m_index], m_flicker[i][m_index]);
        m_colors[k+3] = QVector3D(m_flicker[i][m_index], m_flicker[i][m_index], m_flicker[i][m_index]);
        k += glUtils::POINTS_PER_SQUARE;
    }

    m_vaObject.bind();
    m_colorBuffer.bind();
    m_colorBuffer.write(0, m_colors.data(), m_colors.count() * sizeof(QVector3D)); // number of vertices to avoid * sizeof QVector3D
    m_vaObject.release();
    m_colorBuffer.release();
    ++m_index;
    // Schedule a redraw
    QOpenGLWindow::update();
}


// Setters
void SsvepGL::setFrequencies(QString freqs)
{
    //    qDebug()<< Q_FUNC_INFO;

    QStringList freqsList = freqs.split(',');

    if (m_nrElements == 1)
    {

        //        frequencies[0] = freqsList[0].toDouble();
        m_frequencies.append(freqsList[0].toDouble());
    }
    else
    {
        foreach(QString str, freqsList)
        {

            m_frequencies.append(str.toDouble());
        }
    }

}

void SsvepGL::setFlickeringMode(int t_mode)
{
    m_flickeringMode = t_mode;
}

void SsvepGL::setStimulationDuration(float t_stimDuration)
{
    m_stimulationDuration = t_stimDuration;
}

void SsvepGL::setBreakDuration(int t_brkDuration)
{
    m_breakDuration = t_brkDuration;
    m_preTrialWait = t_brkDuration;
}

void SsvepGL::setSequence(int t_sequence)
{
    m_stimulationSequence = t_sequence;
}

void SsvepGL::setFeedbackPort(int t_port)
{
    m_feedbackPort = t_port;
}

SsvepGL::~SsvepGL()
{
    makeCurrent();
}
