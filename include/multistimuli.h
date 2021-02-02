#ifndef MULTISTIMULI_H
#define MULTISTIMULI_H
//
#include<QMap>
//
#include "speller.h"
#include "erp.h"

class MultiStimuli : public Speller
{

protected slots:
    void startFlashing();
    void pauseFlashing();
    void preTrial();
    void createLayout();
    void sendStimulationInfo();
    void updateWidgets(QList<QPixmap> &pics);


private:
    QList<QPixmap> m_stimuli;

    QMap<int, QString> m_stimuliMap{{1, "bennabi_face_red_inverted.png"},
                                    {2, "yin_yang_small_croped.png"},
                                    {3, "whitehouse_small.png"},
                                    {4, "bennabi_face_blue_inverted.png"},
                                    {5, "yin_yang_small_croped.png"},
                                    {6, "whitehouse_small.png"},
                                    {7, "bennabi_face_magenta_inverted.png"},
                                    {8, "yin_yang_small_croped.png"},
                                    {9, "whitehouse_small.png"}
                                   };
    /*
    QMap<int, QString> m_stimuliMap{{1, "bennabi_face_red_inverted.png"},
                                    {2, "whitehouse_small.png"},
                                    {3, "yin_yang_small_croped.png"},
                                    {4, "bennabi_face_blue_inverted.png"},
                                    {5, "whitehouse_small.png"},
                                    {6, "yin_yang_small_croped.png"},
                                    {7, "bennabi_face_magenta_inverted.png"},
                                    {8, "whitehouse_small.png"},
                                    {9, "yin_yang_small_croped.png"},
                                   };*/
};

#endif // MULTISTIMULI_H
