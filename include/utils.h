#ifndef UTILS_H
#define UTILS_H

#include <QVector>
#include <QSize>
#include <QGuiApplication>
#include <QScreen>

namespace utils {

// rectangle flickering : ON/OFF
QVector<int> gen_flick(double freq, int refreshRate, float length);

// sampled sinusoidal flickering
QVector<double> gen_flick_sin(double freq, int refreshRate, int length);

//
qint8 sign(double value);

//
QSize getScreenSize();

}


namespace speller_type {
// speller types
const quint8 FLASHING_SPELLER = 0;
const quint8 FACES_SPELLER = 1;
const quint8 MOTION_BAR = 2;
const quint8 MOTION_FACE = 3;
const quint8 MOVING_FACE = 4;
const quint8 SSVEP = 5;
const quint8 INVERTED_FACE = 6;
const quint8 COLORED_FACE = 7;
const quint8 INVERTED_COLORED_FACE = 8;
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

namespace config {
//
const int start_port = 54321;
//
const uint64_t OVTK_StimulationLabel_Base = 0x00008100;
}

#endif // UTILS_H