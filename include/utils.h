#ifndef UTILS_H
#define UTILS_H

#include <QtMath>
#include <QVector>
#include <QSize>
#include <QGuiApplication>
#include <QScreen>

namespace utils {
//
static QChar letters[6][6] = {
    {'A','B','C','D','E','F'},
    {'G','H','I','J','K','L'},
    {'M','N','O','Q','P','R'},
    {'S','T','U','V','W','X'},
    {'Y','Z','0','1','2','3'},
    {'4','5','6','7','8','9'},
};

// static QString ArabicLetters = "أبتثجحخدذرزسشصضطظغفقكلمنهوي";
static QString ArabicLetters = "دخحجثتبأطضصشسزرذنملكقفغظيوه";
static QString numbers = "0123456789<_>";

// rectangle flickering
QVector<double> gen_flick(double freq, int refreshRate, float length, quint8 stimMode, double phase=0.0);

// rectangle flickering : ON/OFF
QVector<double> gen_flick_on_off(double freq, int refreshRate, float length);

// sampled sinusoidal flickering // joint frequency-phase modulation (JFPM)
QVector<double> gen_flick_sin(double freq, int refreshRate, float length, double phase);

void wait(int millisecondsToWait);

// joint frequency-phase modulation
// TODO
//
qint8 sign(double value);

//
QSize getScreenSize();

}
//
namespace paradigm_type{
const quint8 ERP = 0;
const quint8 SSVEP = 1;
const quint8 HYBRID = 2;
}
//
namespace flashing_mode {
const quint8 SC = 0;
const quint8 RC = 1;
}
namespace speller_type {
// speller types
// stimulation type
const quint8 FLASHING_SPELLER = 0;
const quint8 FACES_SPELLER = 1;
const quint8 INVERTED_FACE = 2;
const quint8 COLORED_FACE = 3;
const quint8 INVERTED_COLORED_FACE = 4;
const quint8 SSVEP = 5;
const quint8 ARABIC_SPELLER = 6;
const quint8 MULTI_STIM = 7;
const quint8 DUAL_STIM = 8;
//const quint8 MOTION_BAR = 2;
//const quint8 MOTION_FACE = 3;
//const quint8 MOVING_FACE = 4;
}

namespace control_mode {
// control mode : synchronous / asynchronous
const quint8 SYNC = 0;
const quint8 ASYNC = 1;
}

namespace operation_mode {
// operation modes
const quint8 CALIBRATION = 0;
const quint8 COPY_MODE  = 1;
const quint8 FREE_MODE = 2;
const quint8 SSVEP_SINGLE = 3;
}

namespace trial_state {
// trial states
const quint8 PRE_TRIAL = 0;
const quint8 STIMULUS = 1;
const quint8 POST_STIMULUS = 2;
const quint8 FEEDBACK = 3;
const quint8 POST_TRIAL = 4;
}
//
namespace frequency_stimulation {
const quint8 ON_OFF = 0;
const quint8 SIN = 1; // only frequency
const quint8 SIN_FP = 2; // joint frequency-phase modulation
}

namespace config {
//
const int start_port = 54321;
//
const uint64_t OVTK_StimulationLabel_Base = 0x00008100;
const int REFRESH_RATE = 60;
// const double PHASE = 0.35*M_PI;
const double PHASE = M_PI;
}

namespace external_comm {
const quint8 DISABLED = 0;
const quint8 ENABLED = 1;
}

namespace robot_state {
const quint8 MOVING = 0;
const quint8 READY = 1;
}

#endif // UTILS_H
