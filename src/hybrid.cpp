//
#include <QtDebug>
//
#include "hybrid.h"
//

Hybrid::Hybrid()
{
    m_type = paradigm_type::HYBRID;

    m_ERPparadigm = new ERP();
    m_SSVEPparadigm = new SSVEP();
}

Hybrid::Hybrid(ERP *erp, SSVEP *ssvep)
{

    m_type = paradigm_type::HYBRID;

    m_ERPparadigm = new ERP(erp->experimentMode(),
                            erp->controlMode(),
                            erp->type(),
                            external_comm::DISABLED,
                            erp->stimulationDuration(),
                            erp->breakDuration(),
                            erp->nrSequences(),
                            erp->desiredPhrase(),
                            m_externalAddress,
                            erp->stimulationType(),
                            erp->flashingMode()
                            );

    m_SSVEPparadigm = new SSVEP(ssvep->experimentMode(),
                                ssvep->controlMode(),
                                ssvep->type(),
                                external_comm::DISABLED,
                                ssvep->stimulationDuration(),
                                ssvep->breakDuration(),
                                ssvep->nrSequences(),
                                ssvep->desiredPhrase(),
                                m_externalAddress,
                                ssvep->nrElements(),
                                ssvep->frequencies(),
                                ssvep->stimulationMode()
                                );
}


QVariant Hybrid::toVariant() const
{
    QVariantMap map;

    map.insert("paradigmType", m_type);
    map.insert("experimentMode", m_experimentMode);
    map.insert("externalComm", m_externalComm);
    map.insert("ip", m_externalAddress);
    // ERP config
    map.insert("ERP_breakDuration", m_ERPparadigm->breakDuration());
    map.insert("ERP_desiredPhrase", m_ERPparadigm->desiredPhrase());
    map.insert("ERP_experimentMode",m_experimentMode);
    // map.insert("ERP_controlMode", m_ERPparadigm->controlMode());
    map.insert("ERP_flashingMode", m_ERPparadigm->flashingMode());
    map.insert("ERP_nrSequences", m_ERPparadigm->nrSequences());
    map.insert("ERP_paradigmType", m_ERPparadigm->type());
    map.insert("ERP_stimulationDuration", m_ERPparadigm->stimulationDuration());
    map.insert("ERP_stimulationType", m_ERPparadigm->stimulationType());
    // SSVEP config
    map.insert("SSVEP_breakDuration", m_SSVEPparadigm->breakDuration());
    map.insert("SSVEP_desiredPhrase", m_SSVEPparadigm->desiredPhrase());
    map.insert("SSVEP_experimentMode", m_experimentMode);
    map.insert("SSVEP_controlMode", m_SSVEPparadigm->controlMode());
    map.insert("SSVEP_frequencies", m_SSVEPparadigm->frequencies());
    map.insert("SSVEP_nrElements", m_SSVEPparadigm->nrElements());
    map.insert("SSVEP_nrSequences", m_SSVEPparadigm->nrSequences());
    map.insert("SSVEP_paradigmType", m_SSVEPparadigm->type());
    map.insert("SSVEP_stimulationDuration", m_SSVEPparadigm->stimulationDuration());
    map.insert("SSVEP_stimulationMode", m_SSVEPparadigm->stimulationMode());

    return map;
}

void Hybrid::fromVariant(const QVariant &variant)
{
    QVariantMap map = variant.toMap();

    m_type = map.value("paradigymType").toUInt();
    m_experimentMode = map.value("experimentMode").toUInt();

    if(map.value("externalComm").isNull())
    {
        m_externalComm = external_comm::DISABLED;
        m_externalAddress = "127.0.0.1"; // home sweet home
    }
    else
    {
        m_externalComm = map.value("externalComm").toInt();
        m_externalAddress = map.value("ip").toString();
    }
    // ERP config
    m_ERPparadigm->setExperimentMode(map.value("experimentMode").toInt());
    m_ERPparadigm->setControlMode(map.value("ERP_controlMode").toInt());
    m_ERPparadigm->setType(map.value("ERP_paradigmType").toInt());
    m_ERPparadigm->setStimulationDuration(map.value("ERP_stimulationDuration").toInt());
    m_ERPparadigm->setBreakDuration(map.value("ERP_breakDuration").toInt());
    m_ERPparadigm->setNrSequences(map.value("ERP_nrSequences").toInt());
    QString str = map.value("ERP_desiredPhrase").toString();

    if(str.isEmpty())
    {
        // RandomFlashSequence *randomPhrase = new RandomFlashSequence(9, 6);
        // RandomFlashSequence *randomPhrase = new RandomFlashSequence(9, 4);
        RandomFlashSequence *randomPhrase = new RandomFlashSequence(9, 8);
        m_ERPparadigm->setDesiredPhrase(randomPhrase->toString());
    }
    else
    {
        m_ERPparadigm->setDesiredPhrase(str);
    }

    m_ERPparadigm->setStimulationType(map.value("ERP_stimulationType").toInt());
    m_ERPparadigm->setFlashingMode(map.value("ERP_flashingMode").toInt());
    // SSVEP config
    m_SSVEPparadigm->setExperimentMode(map.value("experimentMode").toInt());
    m_SSVEPparadigm->setControlMode(map.value("SSVEP_controlMode").toInt());
    m_SSVEPparadigm->setType(map.value("paradigmType").toInt());
    m_SSVEPparadigm->setStimulationDuration(map.value("SSVEP_stimulationDuration").toInt());
    m_SSVEPparadigm->setBreakDuration(map.value("SSVEP_breakDuration").toInt());
    m_SSVEPparadigm->setNrSequences(map.value("SSVEP_nrSequences").toInt());
    m_SSVEPparadigm->setDesiredPhrase(map.value("SSVEP_desiredPhrase").toString());
    m_SSVEPparadigm->setNrElements(map.value("SSVEP_nrElements").toInt());
    m_SSVEPparadigm->setFrequencies(map.value("SSVEP_frequencies").toString());
    m_SSVEPparadigm->setStimulationMode(map.value("SSVEP_stimulationMode").toInt());
}

Hybrid::~Hybrid()
{

}
