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
		disp(NULL)
	{}
	inline ~ScreenSaverPriv()
	{
		if (disp)
			XCloseDisplayFunc(disp);
	}

	typedef void *(*XOpenDisplayType)(const char *name);
	typedef int (*XResetScreenSaverType)(void *dpy);
	typedef int (*XCloseDisplayType)(void *dpy);
	typedef int (*XFlushType)(void *dpy);

	XOpenDisplayType XOpenDisplayFunc;
	XResetScreenSaverType XResetScreenSaverFunc;
	XCloseDisplayType XCloseDisplayFunc;
	XFlushType XFlushFunc;

	void *disp;
};

ScreenSaver::ScreenSaver() :
	priv(new ScreenSaverPriv)
{
#if QT_VERSION >= 0x050000
	if (QGuiApplication::platformName() != "xcb")
		return;
#endif
	QLibrary libX11("X11");
	if (libX11.load())
	{
		priv->XOpenDisplayFunc = (ScreenSaverPriv::XOpenDisplayType)libX11.resolve("XOpenDisplay");
		priv->XResetScreenSaverFunc = (ScreenSaverPriv::XResetScreenSaverType)libX11.resolve("XResetScreenSaver");
		priv->XFlushFunc = (ScreenSaverPriv::XFlushType)libX11.resolve("XFlush");
		priv->XCloseDisplayFunc = (ScreenSaverPriv::XCloseDisplayType)libX11.resolve("XCloseDisplay");
		if (priv->XOpenDisplayFunc && priv->XResetScreenSaverFunc && priv->XFlushFunc && priv->XCloseDisplayFunc)
			priv->disp = priv->XOpenDisplayFunc(NULL);
	}
}
ScreenSaver::~ScreenSaver()
{
	delete priv;
}

bool ScreenSaver::isOk() const
{
	return priv->disp;
}

void ScreenSaver::reset()
{
	priv->XResetScreenSaverFunc(priv->disp);
	priv->XFlushFunc(priv->disp);
}

#elif defined(Q_OS_WIN)

#include <QCoreApplication>

#include <Functions.hpp>

class ScreenSaverCtx
{
public:
	inline ScreenSaverCtx() :
		time(0.0),
		blocked(false)
	{}

	double time;
	bool blocked;
};

static inline bool blockScreenSaver(MSG *msg, ScreenSaverCtx &screenSaverCtx)
{
	if (msg->message == WM_SYSCOMMAND && ((msg->wParam & 0xFFF0) == SC_SCREENSAVE || (msg->wParam & 0xFFF0) == SC_MONITORPOWER) && screenSaverCtx.blocked)
	{
		if (Functions::gettime() - screenSaverCtx.time >= 3.0)
			screenSaverCtx.blocked = false;
		return true;
	}
	return false;
}

static inline void screenSaverReset(ScreenSaverCtx &screenSaverCtx)
{
	screenSaverCtx.time = Functions::gettime();
	screenSaverCtx.blocked = true;
}

#if QT_VERSION >= 0x050000
	#include <QAbstractNativeEventFilter>

	class ScreenSaverPriv : public QAbstractNativeEventFilter
	{
	public:
		inline ScreenSaverPriv() :
			blocked(false)
		{}

		ScreenSaverCtx screenSaverCtx;

	private:
		bool nativeEventFilter(const QByteArray &, void *m, long *)
		{
			return blockScreenSaver((MSG *)m, screenSaverCtx);
		}
	};

	ScreenSaver::ScreenSaver() :
		priv(new ScreenSaverPriv)
	{
		qApp->installNativeEventFilter(priv);
	}
	ScreenSaver::~ScreenSaver()
	{
		delete priv;
	}

	void ScreenSaver::reset()
	{
		screenSaverReset(priv->screenSaverCtx);
	}
#else
	static ScreenSaverCtx screenSaverCtx;

	static bool eventFilter(void *m, long *)
	{
		return blockScreenSaver((MSG *)m, screenSaverCtx);
	}

	ScreenSaver::ScreenSaver() :
		priv(NULL)
	{
		qApp->setEventFilter(::eventFilter);
	}
	ScreenSaver::~ScreenSaver()
	{}

	void ScreenSaver::reset()
	{
		screenSaverReset(screenSaverCtx);
	}
#endif

bool ScreenSaver::isOk() const
{
	return true;
}

#else

ScreenSaver::ScreenSaver() :
	priv(NULL)
{}
ScreenSaver::~ScreenSaver()
{}

bool ScreenSaver::isOk() const
{
	return false;
}

void ScreenSaver::reset()
{}

#endif
