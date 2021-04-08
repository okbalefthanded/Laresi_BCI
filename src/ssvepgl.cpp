//
#include <QWindow>
#include <QDebug>
#include <QTime>
#include <QtMath>
#include <Qapplication>
#include <QDateTime>
#include <QDir>
//
#include "ssvepgl.h"
#include "ovtk_stimulations.h"
#include "utils.h"
#include "glutils.h"
//
SsvepGL::SsvepGL(SSVEP *paradigm, int t_port)
{
    m_ssvep = paradigm;
    setFrequencies(m_ssvep->frequencies());
    setFeedbackPort(t_port);
    // m_preTrialWait = m_ssvep->breakDuration();

    m_preTrialWait = 3;
    // qDebug() << "SSVEP stim duration : " << m_ssvep->stimulationDuration();
    // set vertices, vertices indeices & colors
    initElements();

    m_preTrialTimer = new QTimer(this);
    m_preTrialTimer->setTimerType(Qt::PreciseTimer);
    // m_preTrialTimer->setInterval(1000);
    // m_preTrialTimer->setInterval((m_ssvep->breakDuration()*1000) / 2);
    m_preTrialTimer->setInterval(m_ssvep->breakDuration() / 2);
    m_preTrialTimer->setSingleShot(true);

    connect(m_preTrialTimer, SIGNAL(timeout()), this, SLOT(startTrial()) );

    m_feedbackSocket = new QUdpSocket(this);
    m_feedbackSocket->bind(QHostAddress::LocalHost, m_feedbackPort);

    connect(m_feedbackSocket, SIGNAL(readyRead()), this, SLOT(receiveFeedback()));
    //
    initLogger();
    //
    //
    m_state = trial_state::PRE_TRIAL;

}

void SsvepGL::initializeGL()
{
    // Initialize OpenGL Backend
    initializeOpenGLFunctions();
    m_index = -1;
    // Set global information
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_FRAMEBUFFER_SRGB);

    /*
    if(QGuiApplication::screens().size() == 2)
    {
        this->setScreen(QGuiApplication::screens().last());
        this->showFullScreen();
    }
    */

    // set vertices, vertices indeices & colors
    // initElements();

    // static const int samplesLength = REFRESH_RATE * (stimulationDuration);

    m_flicker.resize(m_frequencies.size());
    double phase = 0.0;
    for (int i=0; i < m_frequencies.size(); ++i)
    {
        phase = config::PHASE * ((i+1)%2);
        m_flicker[i] = utils::gen_flick(m_frequencies[i], config::REFRESH_RATE, m_ssvep->stimulationDuration(), m_ssvep->stimulationMode(), phase);
    }
    //qDebug() << "flicker size: "<< m_flicker[0].size();
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

    initExternalSocket();
}

void SsvepGL::resizeGL(int w, int h)
{
    //TODO
    //    (void)w;
    //    (void)h;
    // initElements();
}

void SsvepGL::paintGL()
{

    // clear
    glClear(GL_COLOR_BUFFER_BIT);
    int nVertices = glUtils::VERTICES_PER_TRIANGLE * (m_ssvep->nrElements()) * glUtils::TRIANGLES_PER_SQUARE;
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
    // qDebug()<< "[TRIAL START]" << Q_FUNC_INFO;

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

    // if(m_trials == 0 && m_preTrialCount == 0)
    if(m_trials == 0)
    {
        sendMarker(OVTK_StimulationId_ExperimentStart);
    }

    if(m_firstRun)
    {
        m_flickeringSequence = new RandomFlashSequence(m_ssvep->nrElements(), m_ssvep->nrSequences() / m_ssvep->nrElements());
        // qDebug()<< "sequence "<<m_flickeringSequence->sequence;

        m_firstRun = false;
    }

    if (m_preTrialCount == 1)
    {
        sendMarker(OVTK_StimulationId_TrialStart);

        if (m_ssvep->experimentMode() == operation_mode::CALIBRATION ||
                m_ssvep->experimentMode() == operation_mode::COPY_MODE)
        {
            // qDebug() << "Higlight target SSVEP ";
            highlightTarget();
        }

    }
    else if(m_preTrialCount == 2)
    {
        if (m_ssvep->experimentMode() == operation_mode::CALIBRATION ||
                m_ssvep->experimentMode() == operation_mode::COPY_MODE)
        {
            // qDebug() << "Refresh target SSVEP ";
            refreshTarget();
        }
    }

    m_preTrialTimer->start();
    m_preTrialCount++;


    if (m_preTrialCount > m_preTrialWait)
    {
        // refreshTarget();
        scheduleRedraw();
        m_preTrialTimer->stop();
        // m_preTrialCount = 0;
        m_preTrialCount = 1;
        m_state = trial_state::STIMULUS;
    }
}

void SsvepGL::postTrial()
{

    disconnect(this, SIGNAL(frameSwapped()), this, SLOT(update()));
    initElements();

    m_index = -1;
    m_lostFrames = 0;
    m_state = trial_state::PRE_TRIAL;

    // feedback
    if(m_ssvep->experimentMode() == operation_mode::COPY_MODE || m_ssvep->experimentMode() == operation_mode::FREE_MODE)
    {

        // utils::wait(100); // wait for OV processing
        feedback();  // wait for feedback
        if (m_presentFeedback)
        {
            utils::wait(500);
            // feedback for 1 sec & refresh
            // utils::wait(1000);
            refresh(m_sessionFeedback[m_currentFlicker].digitValue()-1);
        }
    }

    else // calibration mode
    {
        refreshTarget();
    }

    externalCommunication();

    postTrialEnd();

}

void SsvepGL::postTrialEnd()
{
    ++m_currentFlicker;
    ++m_trials;

    if (m_currentFlicker < m_flickeringSequence->sequence.size() &&
            m_flickeringSequence->sequence.length() != 1 &&
            (m_ssvep->experimentMode() == operation_mode::COPY_MODE ||
             m_ssvep->experimentMode() == operation_mode::CALIBRATION))
    {
        startTrial();
    }
    else if (m_flickeringSequence->sequence.length() <= 1)
    {
        m_currentFlicker = 0;
        emit(slotTerminated());
        return;
    }
    else
    {
        m_correct = (m_correct / m_flickeringSequence->sequence.size()) * 100;
        qDebug()<< "Accuracy in SSVEP session: " << m_correct;
        qDebug()<< "Experiment End, closing SSVEP stimulation";
        sendMarker(OVTK_StimulationId_ExperimentStop);
        utils::wait(2000);
        // emit(slotTerminated());
        // this->close();
    }
}

void SsvepGL::Flickering()
{

    if(m_index == -1)
    {
        // qDebug()<< Q_FUNC_INFO << "connecting frameswapped to update" << "index " << index;
        connect(this, SIGNAL(frameSwapped()), this, SLOT(update()));
        // qDebug()<< Q_FUNC_INFO << "[update ]Index (first): "<< m_index << "current time: " << QTime::currentTime().msec();
    }

    //    sendMarker(config::OVTK_StimulationLabel_Base + m_flickeringSequence->sequence[m_currentFlicker]);
    //    sendMarker(OVTK_StimulationId_VisualSteadyStateStimulationStart);
    //        qDebug()<< Q_FUNC_INFO << "markers sent" << "current time: " << QTime::currentTime().msec();
    // qDebug()<< Q_FUNC_INFO << "Stim : " << "current time: " << QTime::currentTime().msec();


    while(m_index <= m_flicker[0].size())
    {
        QCoreApplication::processEvents(QEventLoop::AllEvents);
    }

    // utils::wait(17);
    sendMarker(OVTK_StimulationId_TrialStop);
    //  qDebug()<< Q_FUNC_INFO << "[update ]Index (last) : "<< m_index << "current time: " << QTime::currentTime().msec();

    //++m_currentFlicker;

    m_state = trial_state::POST_TRIAL;
}

void SsvepGL::feedback()
{
    // receiveFeedback
    // m_feedbackSocket->waitForReadyRead(500);
    // qDebug()<< QTime::currentTime();
    // m_feedbackSocket->waitForReadyRead(100);
    // m_feedbackSocket->waitForReadyRead(90);
    m_feedbackSocket->waitForReadyRead();

    if(m_presentFeedback)
    {
        // m_feedbackSocket->waitForReadyRead(100);

        if(m_ssvep->experimentMode() == operation_mode::COPY_MODE)
        {

            if(m_sessionFeedback[m_currentFlicker].digitValue() == m_flickeringSequence->sequence[m_currentFlicker])
            {
                highlightFeedback(glColors::green, m_flickeringSequence->sequence[m_currentFlicker]-1);
                ++m_correct;
            }
            else
            {
                highlightFeedback(glColors::blue, m_sessionFeedback[m_currentFlicker].digitValue()-1);
            }
        }
        else if(m_ssvep->experimentMode() == operation_mode::FREE_MODE)
        {
            highlightFeedback(glColors::red, m_sessionFeedback[m_currentFlicker].digitValue()-1);
        }
    }
}

void SsvepGL::receiveFeedback()
{
    // wait for OV python script to write in UDP feedback socket
    // wait(500);
    // qDebug()<< Q_FUNC_INFO;
    QHostAddress sender;
    quint16 senderPort;
    QByteArray *buffer = new QByteArray();

    buffer->resize(m_feedbackSocket->pendingDatagramSize());

    while(m_feedbackSocket->hasPendingDatagrams())
    {
        m_feedbackSocket->readDatagram(buffer->data(), buffer->size(), &sender, &senderPort);
    }
    log->write(buffer->data());

    // qDebug()<< buffer->data();

    if (m_flickeringSequence->sequence.length() == 1) // Hybrid stimulation mode
    {

        m_sessionFeedback = buffer->data();
    }
    else
    {
        m_sessionFeedback += buffer->data();
    }

    // qDebug() << Q_FUNC_INFO << "SSVEP FEEDBACK "<< m_sessionFeedback;

}

void SsvepGL::initElements()
{
    double dx = 0.2;
    double dy = 0.2;
    int isNullX = 0, isNullY = 0, sx=1;

    if(m_ssvep->nrElements() == 1)
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
        int vectorsSize = m_ssvep->nrElements() * glUtils::POINTS_PER_SQUARE;
        m_vertices.resize(vectorsSize);
        initRects();
        initColors();
    }
    //
    initIndices();
    //
    scheduleRedraw();
}

void SsvepGL::initRects()
{
    // double pixelSize = 250.0; // length of a stimulus vertex in pixels (150x150 square)
    // double pixelSize = 150.0;
    double pixelSize = 200.0;
    // double dx = 0.2;
    // double dy = 0.2;
    QSize screenSize = utils::getScreenSize();
    double dx = pixelSize / screenSize.width();
    double dy = pixelSize / screenSize.height();
    int isNullX = 0, isNullY = 0, sx=1;
    int offset;

    if( m_ssvep->controlMode() == control_mode::SYNC)
    {
        offset = glUtils::POINTS_PER_SQUARE;
    }
    else
    {
        offset = 0;
    }

    for(int i=0;i<m_vertices.count(); i+=glUtils::POINTS_PER_SQUARE)
    {
        m_vertices[i] = refPoints::topPoints[(i+offset)/glUtils::POINTS_PER_SQUARE];
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
    int vectorsSize = m_ssvep->nrElements() * glUtils::POINTS_PER_SQUARE;
    m_colors.resize(vectorsSize);

    if( m_ssvep->controlMode() == control_mode::SYNC)
    {
        for (int i=0; i<m_colors.count(); i++)
        {
            m_colors[i] = glColors::white;
        }
    }
    else
    {
        // init colors
        m_colors = {glColors::gray, glColors::gray, glColors::gray, glColors::gray};
        m_colors.resize(vectorsSize);
        for (int i=glUtils::POINTS_PER_SQUARE; i<m_colors.count(); i++)
        {
            m_colors[i] = glColors::white;
        }
    }
}

void SsvepGL::initIndices()
{
    // init indices
    m_vindices.resize(m_ssvep->nrElements()*glUtils::INDICES_PER_SQUARE);
    int k=0; int val = 0;
    for(int i=0; i<(m_ssvep->nrElements()*glUtils::INDICES_PER_SQUARE); i+=glUtils::INDICES_PER_SQUARE)
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
}

void SsvepGL::externalCommunication()
{
    // Send and Recieve feedback to/from Robot if external communication is enabled
    // qDebug()<< Q_FUNC_INFO << m_sessionFeedback;
    std::string cmd = QString(m_sessionFeedback[m_currentFlicker]).toStdString();

    if(m_ssvep->externalComm() == external_comm::ENABLED)
    {
        qDebug() << "Sending Feedback to Robot";
        if (!m_robotSocket->isOpen())
        {
            qDebug()<< "Not sending Feedback to Robot : Cannot send feedback socket is not open";
        }

        try
        {
            std::string str = cmd;
            const char* p = str.c_str();
            // qDebug()<< "command to send to Robot: " << QString::fromStdString(cmd) << m_sessionFeedback[m_currentFlicker];
            QByteArray byteovStimulation;
            QDataStream streamovs(&byteovStimulation, QIODevice::WriteOnly);
            streamovs.setByteOrder(QDataStream::LittleEndian);
            streamovs.writeRawData(p, str.length());
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

void SsvepGL::highlightTarget()
{

    if(m_ssvep->nrElements() == 1)
    {
        //        m_colors = {glColors::green, glColors::green, glColors::green, glColors::green};
        //        m_colors = {glColors::red, glColors::red, glColors::red, glColors::red};
        m_colors = {glColors::yellow, glColors::yellow, glColors::yellow, glColors::yellow};
    }
    else
    {
        int tmp = m_flickeringSequence->sequence[m_currentFlicker]-1;
        int squareIndex = tmp + (glUtils::VERTICES_PER_TRIANGLE*tmp);
        //        m_colors[squareIndex] = glColors::green;
        //        m_colors[squareIndex + 1] = glColors::green;
        //        m_colors[squareIndex + 2] = glColors::green;
        //        m_colors[squareIndex + 3] = glColors::green;
        //        m_colors[squareIndex] = glColors::red;
        //        m_colors[squareIndex + 1] = glColors::red;
        //        m_colors[squareIndex + 2] = glColors::red;
        //        m_colors[squareIndex + 3] = glColors::red;
        m_colors[squareIndex] = glColors::yellow;
        m_colors[squareIndex + 1] = glColors::yellow;
        m_colors[squareIndex + 2] = glColors::yellow;
        m_colors[squareIndex + 3] = glColors::yellow;
    }

    scheduleRedraw();

}

void SsvepGL::refreshTarget()
{

    if(m_ssvep->nrElements() == 1)
    {
        m_colors = {glColors::white, glColors::white, glColors::white, glColors::white};
    }
    else
    {
        if( m_ssvep->controlMode() == control_mode::SYNC)
        {

            int tmp = m_flickeringSequence->sequence[m_currentFlicker] - 1;
            int squareIndex = tmp+(glUtils::VERTICES_PER_TRIANGLE*tmp);
            // qDebug()<< Q_FUNC_INFO << "squareIdx " << tmp;
            //            int squareIndex = tmp;
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
    scheduleRedraw();
}

void SsvepGL::highlightFeedback(QVector3D feedbackColor, int feebdackIndex)
{
    int squareIndex = feebdackIndex + (glUtils::VERTICES_PER_TRIANGLE*feebdackIndex);
    m_colors[squareIndex] = feedbackColor;
    m_colors[squareIndex + 1] = feedbackColor;
    m_colors[squareIndex + 2] = feedbackColor;
    m_colors[squareIndex + 3] = feedbackColor;

    scheduleRedraw();
}

void SsvepGL::refresh(int feedbackIndex)
{

    int squareIndex = feedbackIndex + (glUtils::VERTICES_PER_TRIANGLE*feedbackIndex);
    m_colors[squareIndex] = glColors::white;
    m_colors[squareIndex + 1] = glColors::white;
    m_colors[squareIndex + 2] = glColors::white;
    m_colors[squareIndex + 3] = glColors::white;

    scheduleRedraw();

}

void SsvepGL::initLogger()
{

    QDir logsDir(QCoreApplication::applicationDirPath() + "/logs");
    if(!logsDir.exists())
    {
        logsDir.mkdir(logsDir.path());
    }
    QString fileName;
    if(m_ssvep->experimentMode() == operation_mode::CALIBRATION)
    {

        fileName = logsDir.filePath("ssvep_calib_" +QDateTime::currentDateTime().toString("dd_MM_yyyy_hh_mm_ss")+".txt");
    }
    else
    {
        fileName =  logsDir.filePath("ssvep_online_" +QDateTime::currentDateTime().toString("dd_MM_yyyy_hh_mm_ss")+".txt");
    }

    log = new Logger(this, fileName);
}

void SsvepGL::scheduleRedraw()
{

    // qDebug()<< "ScheduleRedraw [update ] Index : "<< m_index << "current time: " << QTime::currentTime().msec();
    // QOpenGLWindow::update();

    m_vaObject.bind();
    m_colorBuffer.bind();
    m_colorBuffer.write(0, m_colors.data(), m_colors.count() * sizeof(QVector3D)); // number of vertices to avoid * sizeof QVector3D
    m_vaObject.release();
    m_colorBuffer.release();

    QOpenGLWindow::update();
}

bool SsvepGL::presentFeedback() const
{
    return m_presentFeedback;
}

void SsvepGL::setPresentFeedback(bool presentFeedback)
{
    m_presentFeedback = presentFeedback;
}

SSVEP *SsvepGL::ssvep() const
{
    return m_ssvep;
}

void SsvepGL::setSsvep(SSVEP *ssvep)
{
    m_ssvep = ssvep;
}

bool SsvepGL::isCorrect() const
{
    return m_sessionFeedback[m_sessionFeedback.length()-1].digitValue() == m_flickeringSequence->sequence[m_currentFlicker];
}

void SsvepGL::update()
{
    //   qDebug()<< "[update ] Index : "<< m_lostFrames << " " << m_index << "current time: " << QTime::currentTime().msec();

    // temporary hack
    if (m_lostFrames <= 7)
    {
        //if(m_lostFrames == 0)
        //  qDebug()<< "[update ] first: incorrect" << QTime::currentTime();
    }
    else
    {

        if(m_index == -1)
        {
            //  qDebug()<< "[update ] first correct: " << QTime::currentTime();
            sendMarker(config::OVTK_StimulationLabel_Base + m_flickeringSequence->sequence[m_currentFlicker]);
            sendMarker(OVTK_StimulationId_VisualSteadyStateStimulationStart);
        }

        int k;

        if(m_ssvep->nrElements() == 1)
        {
            k = 0;
        }
        else
        {
            if (m_ssvep->controlMode() == control_mode::SYNC)
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

        ++m_index;
    }

    ++m_lostFrames;
    scheduleRedraw();
}

// Setters
void SsvepGL::setFrequencies(QString freqs)
{

    QStringList freqsList = freqs.split(',');

    if (m_ssvep->nrElements() == 1)
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

void SsvepGL::setFeedbackPort(int t_port)
{
    m_feedbackPort = t_port;
}

void SsvepGL::initExternalSocket()
{
    // qDebug()<< "Lets see external comm" <<m_ssvep->externalComm();
    if(m_ssvep->externalComm() == external_comm::ENABLED)
    {
        qDebug()<< "External Comm is enabled;";
        m_robotSocket = new QTcpSocket();
        //    m_robotSocket->connectToHost(QHostAddress("10.3.66.5"), m_robotPort);
        // 10.3.66.5 / 10.6.65.128 /10.3.64.92
        // m_robotSocket->connectToHost(QHostAddress("10.3.64.92"), m_robotPort);
        m_robotSocket->connectToHost(QHostAddress(m_ssvep->externalAddress()), m_robotPort);

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

SsvepGL::~SsvepGL()
{
    makeCurrent();
}
