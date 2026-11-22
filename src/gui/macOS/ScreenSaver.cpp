// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <ScreenSaver.hpp>

#include <IOKit/pwr_mgt/IOPMLib.h>

#define QMPLAY2_MEDIA_PLAYBACK CFSTR("QMPlay2 media playback")

class ScreenSaverPriv
{
public:
    inline ~ScreenSaverPriv()
    {
        unInhibit();
    }

    inline void inhibit()
    {
        m_okDisp = (IOPMAssertionCreateWithName(kIOPMAssertPreventUserIdleDisplaySleep, kIOPMAssertionLevelOn, QMPLAY2_MEDIA_PLAYBACK, &m_idDisp) == kIOReturnSuccess);
        m_okSys  = (IOPMAssertionCreateWithName(kIOPMAssertPreventUserIdleSystemSleep,  kIOPMAssertionLevelOn, QMPLAY2_MEDIA_PLAYBACK, &m_idSys)  == kIOReturnSuccess);
    }
    inline void unInhibit()
    {
        if (m_okDisp)
        {
            IOPMAssertionRelease(m_idDisp);
            m_okDisp = false;
        }
        if (m_okSys)
        {
            IOPMAssertionRelease(m_idSys);
            m_okSys = false;
        }
    }

private:
    IOPMAssertionID m_idDisp;
    IOPMAssertionID m_idSys;
    bool m_okDisp = false;
    bool m_okSys  = false;
};

/**/

ScreenSaver::ScreenSaver() :
    m_priv(new ScreenSaverPriv)
{}
ScreenSaver::~ScreenSaver()
{
    delete m_priv;
}

void ScreenSaver::inhibit(int context)
{
    if (inhibitHelper(context))
        m_priv->inhibit();
}
void ScreenSaver::unInhibit(int context)
{
    if (unInhibitHelper(context))
        m_priv->unInhibit();
}
