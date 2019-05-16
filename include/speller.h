#ifndef SPELLER_H
#define SPELLER_H
//
#include <QWidget>
#include <QLabel>
#include <QImage>
#include <QElapsedTimer>
#include <QUdpSocket>
//
#include "randomflashsequence.h"
#include "matrixlayout.h"
#include "erp.h"
#include "utils.h"
//

namespace Ui {
class Speller;
}

class Speller : public QWidget
{
    Q_OBJECT

public:
    explicit Speller(QWidget *parent = 0);
    ~Speller();

    ERP *erp() const;
    void setERP(ERP *erp);
    QString *desiredPhrase() const;
    void setDesiredPhrase(const QString &t_desiredPhrase);
    void setTimers(int t_stimulation, int t_isi);

    friend class Hybrid;
    friend class HybridStimulation;

signals:
    void markerTag(uint64_t t_ovStimulation);
    void slotTerminated();

protected slots:

    void preTrial();
    void startTrial();
    void feedback();
    void postTrial();
    virtual void startFlashing();
    void pauseFlashing();
    void receiveFeedback();
    void createLayout();
    void refreshLayout();

public slots:

    void sendMarker(uint64_t t_ovStimulation){
        emit markerTag(t_ovStimulation);
    }

protected:

    bool isTarget();
    void highlightTarget();
    void refreshTarget();
    void sendStimulationInfo();
    void switchStimulationTimers();

    Ui::Speller *ui;

    int m_rows;
    int m_cols;
    int m_nrElements;
    int m_matrixWidth;
    int m_matrixHeight;
    int m_currentStimulation = 0;
    int m_currentLetter = 0;
    int m_currentTarget = 0;
    int m_trials = 0;

    bool m_stimulus = true;

    bool m_stateFinished = false;
    bool m_running = true;
    int  m_state;
    //
    ERP *m_ERP;
    //
    int m_preTrialWait = 2; // 2 seconds
    int m_preTrialCount = 0;
    quint16 m_feedbackPort = 12345;

    QString m_desiredPhrase= "";
    QString m_text;
    QImage *m_faceStimuli;
    QList<QString> m_presentedLetters;
    MatrixLayout *m_mLayout;
    // Timers
    QTimer *m_preTrialTimer;
    QTimer *m_stimTimer;
    QTimer *m_isiTimer;

    QLabel *m_textRow;
    QUdpSocket *m_feedbackSocket;
    RandomFlashSequence *m_flashingSequence;
};

#endif // SPELLER_H
