/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2017  Błażej Szczygieł

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published
	by the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <ScreenSaver.hpp>

#if defined(Q_WS_X11) || (QT_VERSION >= 0x050000 && defined(Q_OS_UNIX) && !defined(Q_OS_MAC) && !defined(Q_OS_ANDROID))

#if QT_VERSION >= 0x050000
	#include <QGuiApplication>
#endif
#include <QLibrary>

class ScreenSaverPriv : public QObject
{
public:
	inline ScreenSaverPriv() :
		m_disp(nullptr)
	{}
	inline ~ScreenSaverPriv()
	{
		if (m_disp)
			XCloseDisplayFunc(m_disp);
	}

	inline void load()
	{
		QLibrary libX11("X11");
		if (libX11.load())
		{
			XOpenDisplayFunc = (XOpenDisplayType)libX11.resolve("XOpenDisplay");
			XForceScreenSaverFunc = (XForceScreenSaverType)libX11.resolve("XForceScreenSaver");
			XFlushFunc = (XFlushType)libX11.resolve("XFlush");
			XCloseDisplayFunc = (XCloseDisplayType)libX11.resolve("XCloseDisplay");
			if (XOpenDisplayFunc && XForceScreenSaverFunc && XFlushFunc && XCloseDisplayFunc)
				m_disp = XOpenDisplayFunc(nullptr);
		}
	}
	inline bool isLoaded() const
	{
		return (bool)m_disp;
	}

	inline void inhibit()
	{
		timerEvent(nullptr);
		m_timerID = startTimer(30000);
	}
	inline void unInhibit()
	{
		killTimer(m_timerID);
	}

private:
	void timerEvent(QTimerEvent *)
	{
		XForceScreenSaverFunc(m_disp, 0);
		XFlushFunc(m_disp);
	}

	using XOpenDisplayType = void *(*)(const char *name);
	XOpenDisplayType XOpenDisplayFunc;

	using XForceScreenSaverType = int (*)(void *display, int mode);
	XForceScreenSaverType XForceScreenSaverFunc;

	using XFlushType = int (*)(void *display);
	XFlushType XFlushFunc;

	using XCloseDisplayType = int (*)(void *display);
	XCloseDisplayType XCloseDisplayFunc;

	void *m_disp;
	int m_timerID;
};

/**/

ScreenSaver::ScreenSaver() :
	m_priv(new ScreenSaverPriv)
{
#if QT_VERSION >= 0x050000
	if (QGuiApplication::platformName() != "xcb")
		return;
#endif
	m_priv->load();
}
ScreenSaver::~ScreenSaver()
{
	delete m_priv;
}

void ScreenSaver::inhibit(int context)
{
	if (inhibitHelper(context) && m_priv->isLoaded())
		m_priv->inhibit();
}
void ScreenSaver::unInhibit(int context)
{
	if (unInhibitHelper(context) && m_priv->isLoaded())
		m_priv->unInhibit();
}

#elif defined(Q_OS_WIN)

#include <QCoreApplication>

#include <windows.h>

static inline bool inhibitScreenSaver(MSG *msg, bool &inhibited)
{
	return (inhibited && msg->message == WM_SYSCOMMAND && ((msg->wParam & 0xFFF0) == SC_SCREENSAVE || (msg->wParam & 0xFFF0) == SC_MONITORPOWER));
}

#if QT_VERSION >= 0x050000
	#include <QAbstractNativeEventFilter>

	class ScreenSaverPriv : public QAbstractNativeEventFilter
	{
	public:
		inline ScreenSaverPriv() :
			inhibited(false)
		{}

		bool inhibited;

	private:
		bool nativeEventFilter(const QByteArray &, void *m, long *)
		{
			return inhibitScreenSaver((MSG *)m, inhibited);
		}
	};

	ScreenSaver::ScreenSaver() :
		m_priv(new ScreenSaverPriv)
	{
		qApp->installNativeEventFilter(m_priv);
	}
	ScreenSaver::~ScreenSaver()
	{
		delete m_priv;
	}

	void ScreenSaver::inhibit(int context)
	{
		if (inhibitHelper(context))
			m_priv->inhibited = true;
	}
	void ScreenSaver::unInhibit(int context)
	{
		if (unInhibitHelper(context))
			m_priv->inhibited = false;
	}
#else
	static bool inhibited = false;

	static bool eventFilter(void *m, long *)
	{
		return inhibitScreenSaver((MSG *)m, inhibited);
	}

	ScreenSaver::ScreenSaver() :
		m_priv(nullptr)
	{
		qApp->setEventFilter(::eventFilter);
	}
	ScreenSaver::~ScreenSaver()
	{}

	void ScreenSaver::inhibit(int context)
	{
		if (inhibitHelper(context))
			inhibited = true;
	}
	void ScreenSaver::unInhibit(int context)
	{
		if (unInhibitHelper(context))
			inhibited = false;
	}
#endif

#else

ScreenSaver::ScreenSaver() :
	m_priv(nullptr)
{
	Q_UNUSED(m_priv)
}
ScreenSaver::~ScreenSaver()
{}

void ScreenSaver::inhibit(int)
{}
void ScreenSaver::unInhibit(int)
{}

inline bool ScreenSaver::inhibitHelper(int)
{
	return false;
}
inline bool ScreenSaver::unInhibitHelper(int)
{
	return false;
}

#define NO_INHIBIT_HELPER

#endif

#ifndef NO_INHIBIT_HELPER

inline bool ScreenSaver::inhibitHelper(int context)
{
	if (!m_refs.value(context))
	{
		m_refs[context] = true;
		for (auto it = m_refs.constBegin(), itEnd = m_refs.constEnd(); it != itEnd; ++it)
		{
			if (it.value() && it.key() != context)
				return false;
		}
		return true;
	}
	return false;
}
inline bool ScreenSaver::unInhibitHelper(int context)
{
	if (m_refs.value(context))
	{
		m_refs[context] = false;
		for (auto it = m_refs.constBegin(), itEnd = m_refs.constEnd(); it != itEnd; ++it)
		{
			if (it.value())
				return false;
		}
		return true;
	}
	return false;
}

#endif
