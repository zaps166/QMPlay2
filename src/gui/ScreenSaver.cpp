#include <ScreenSaver.hpp>

#if defined(Q_WS_X11) || (QT_VERSION >= 0x050000 && defined(Q_OS_UNIX) && !defined(Q_OS_MAC) && !defined(Q_OS_ANDROID))

#if QT_VERSION >= 0x050000
	#include <QGuiApplication>
#endif
#include <QLibrary>

class ScreenSaverPriv
{
public:
	inline ScreenSaverPriv() :
		m_disp(NULL)
	{}
	inline ~ScreenSaverPriv()
	{
		if (m_disp)
			XCloseDisplayFunc(m_disp);
	}

	inline void block()
	{
		int interval, preferBlanking, allowExposures;
		XGetScreenSaverFunc(m_disp, m_timeout, interval, preferBlanking, allowExposures);
		XSetScreenSaverFunc(m_disp, 0, interval, preferBlanking, allowExposures);
		XFlushFunc(m_disp);
	}
	inline void unblock()
	{
		int tmpTimeout, interval, preferBlanking, allowExposures;
		XGetScreenSaverFunc(m_disp, tmpTimeout, interval, preferBlanking, allowExposures);
		XSetScreenSaverFunc(m_disp, m_timeout, interval, preferBlanking, allowExposures);
		XFlushFunc(m_disp);
	}

	typedef void *(*XOpenDisplayType)(const char *name);
	typedef int (*XGetScreenSaverType)(void *dpy, int &timeout, int &interval, int &preferBlanking, int &allowExposures);
	typedef int (*XSetScreenSaverType)(void *dpy, int timeout, int interval, int prefer_blanking, int allow_exposures);
	typedef int (*XCloseDisplayType)(void *dpy);
	typedef int (*XFlushType)(void *dpy);

	XOpenDisplayType XOpenDisplayFunc;
	XGetScreenSaverType XGetScreenSaverFunc;
	XSetScreenSaverType XSetScreenSaverFunc;
	XCloseDisplayType XCloseDisplayFunc;
	XFlushType XFlushFunc;

	void *m_disp;
	int m_timeout;
};

ScreenSaver::ScreenSaver() :
	m_priv(new ScreenSaverPriv),
	m_ref(0)
{
#if QT_VERSION >= 0x050000
	if (QGuiApplication::platformName() != "xcb")
		return;
#endif
	QLibrary libX11("X11");
	if (libX11.load())
	{
		m_priv->XOpenDisplayFunc = (ScreenSaverPriv::XOpenDisplayType)libX11.resolve("XOpenDisplay");
		m_priv->XGetScreenSaverFunc = (ScreenSaverPriv::XGetScreenSaverType)libX11.resolve("XGetScreenSaver");
		m_priv->XSetScreenSaverFunc = (ScreenSaverPriv::XSetScreenSaverType)libX11.resolve("XSetScreenSaver");
		m_priv->XFlushFunc = (ScreenSaverPriv::XFlushType)libX11.resolve("XFlush");
		m_priv->XCloseDisplayFunc = (ScreenSaverPriv::XCloseDisplayType)libX11.resolve("XCloseDisplay");
		if (m_priv->XOpenDisplayFunc && m_priv->XGetScreenSaverFunc && m_priv->XSetScreenSaverFunc && m_priv->XFlushFunc && m_priv->XCloseDisplayFunc)
			m_priv->m_disp = m_priv->XOpenDisplayFunc(NULL);
	}
}
ScreenSaver::~ScreenSaver()
{
	if (m_ref > 0)
		m_priv->unblock();
	delete m_priv;
}

void ScreenSaver::block()
{
	if (m_priv->m_disp && m_ref++ == 0)
		m_priv->block();
}
void ScreenSaver::unblock()
{
	if (m_priv->m_disp && --m_ref == 0)
		m_priv->unblock();
}

#elif defined(Q_OS_WIN)

#include <QCoreApplication>

#include <windows.h>

static inline bool blockScreenSaver(MSG *msg, bool &blocked)
{
	return (blocked && msg->message == WM_SYSCOMMAND && ((msg->wParam & 0xFFF0) == SC_SCREENSAVE || (msg->wParam & 0xFFF0) == SC_MONITORPOWER));
}

#if QT_VERSION >= 0x050000
	#include <QAbstractNativeEventFilter>

	class ScreenSaverPriv : public QAbstractNativeEventFilter
	{
	public:
		inline ScreenSaverPriv() :
			blocked(false)
		{}

		bool blocked;

	private:
		bool nativeEventFilter(const QByteArray &, void *m, long *)
		{
			return blockScreenSaver((MSG *)m, blocked);
		}
	};

	ScreenSaver::ScreenSaver() :
		m_priv(new ScreenSaverPriv),
		m_ref(0)
	{
		qApp->installNativeEventFilter(m_priv);
	}
	ScreenSaver::~ScreenSaver()
	{
		delete m_priv;
	}

	void ScreenSaver::block()
	{
		if (m_ref++ == 0)
			m_priv->blocked = true;
	}
	void ScreenSaver::unblock()
	{
		if (--m_ref == 0)
			m_priv->blocked = false;
	}
#else
	static bool blocked = false;

	static bool eventFilter(void *m, long *)
	{
		return blockScreenSaver((MSG *)m, blocked);
	}

	ScreenSaver::ScreenSaver() :
		m_priv(NULL),
		m_ref(0)
	{
		qApp->setEventFilter(::eventFilter);
	}
	ScreenSaver::~ScreenSaver()
	{}

	void ScreenSaver::block()
	{
		if (m_ref++ == 0)
			blocked = true;
	}
	void ScreenSaver::unblock()
	{
		if (--m_ref == 0)
			blocked = false;
	}
#endif

#else

ScreenSaver::ScreenSaver() :
	m_priv(NULL),
	m_ref(0)
{}
ScreenSaver::~ScreenSaver()
{}

void ScreenSaver::block()
{}
void ScreenSaver::unblock()
{}

#endif
