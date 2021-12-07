/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2021  Błażej Szczygieł

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

#include <Main.hpp>

#include <EventFilterWorkarounds.hpp>
#include <PanGestureEventFilter.hpp>
#include <ScreenSaver.hpp>
#include <Frame.hpp>
#include <MainWidget.hpp>
#include <PlayClass.hpp>
#include <Functions.hpp>
#include <VideoDock.hpp>
#include <Settings.hpp>
#include <Version.hpp>
#include <Module.hpp>
#include <IPC.hpp>

#include <QCommandLineParser>
#include <QDesktopWidget>
#include <QApplication>
#include <QImageReader>
#include <QTreeWidget>
#include <QMessageBox>
#include <QFileDialog>
#include <QPainter>
#include <QBuffer>
#include <QFile>
#include <QDir>
#ifdef CHECK_FOR_EGL
    #include <QLibrary>
#endif
#ifdef Q_OS_MACOS
    #include <QSurfaceFormat>
#endif

#include <clocale>
#include <csignal>
#include <ctime>

static ScreenSaver *g_screenSaver = nullptr;
static bool g_useGui = true;

/**/

QMPlay2GUIClass &QMPlay2GUIClass::instance()
{
    static QMPlay2GUIClass qmplay2Gui;
    return qmplay2Gui;
}

QString QMPlay2GUIClass::getPipe()
{
#if defined Q_OS_WIN
    return "\\\\.\\pipe\\QMPlay2";
#elif defined Q_OS_MACOS
    return "/tmp/QMPlay2." + QString(getenv("USER"));
#else
    return QDir::tempPath() + "/QMPlay2." + QString(getenv("USER"));
#endif
}
void QMPlay2GUIClass::saveCover(QByteArray cover)
{
    QBuffer buffer(&cover);
    if (buffer.open(QBuffer::ReadOnly))
    {
        const QString fileExtension = QImageReader::imageFormat(&buffer);
        buffer.close();
        if (!fileExtension.isEmpty())
        {
            QString fileName = QFileDialog::getSaveFileName(QMPlay2GUI.mainW, tr("Saving cover picture"), QMPlay2GUI.getCurrentPth(), fileExtension.toUpper() + " (*." + fileExtension + ')');
            if (!fileName.isEmpty())
            {
                if (!fileName.endsWith('.' + fileExtension))
                    fileName += '.' + fileExtension;
                QFile f(fileName);
                if (f.open(QFile::WriteOnly))
                {
                    f.write(cover);
                    QMPlay2GUI.setCurrentPth(Functions::filePath(fileName));
                }
            }
        }
    }
}

void QMPlay2GUIClass::setTreeWidgetItemIcon(QTreeWidgetItem *tWI, const QIcon &icon, const int column, QTreeWidget *treeWidget)
{
    bool setDefaultIcon = icon.name().isEmpty();
    if (!setDefaultIcon)
    {
        // Icons from theme may not be in correct size in tree widget, so do a workaround.
        if (!treeWidget)
            treeWidget = tWI->treeWidget();
        if (treeWidget)
            tWI->setData(column, Qt::DecorationRole, icon.pixmap(treeWidget->window()->windowHandle(), treeWidget->iconSize()));
        else
            setDefaultIcon = true;
    }
    if (setDefaultIcon)
        tWI->setIcon(column, icon);
}

#ifdef UPDATER
void QMPlay2GUIClass::runUpdate(const QString &UpdateFile)
{
    settings->set("UpdateFile", QString("remove:" + UpdateFile));
    connect(qApp, &QApplication::destroyed, [=] {
        ShellExecuteW(nullptr, L"open", (const wchar_t *)UpdateFile.utf16(), L"--Auto", nullptr, SW_SHOWNORMAL);
    });
}
#endif

void QMPlay2GUIClass::setStyle()
{
    QString defaultStyle;
#if defined Q_OS_ANDROID
    defaultStyle = "fusion"; //Android style is awful in Qt (tested on Qt 5.4 and Qt 5.5)
#endif
    QApplication::setStyle(settings->getString("Style", defaultStyle));
}

void QMPlay2GUIClass::loadIcons()
{
    deleteIcons();

    // QtSvg doesn't support effects. This workaround generates drop shadow effect similar to
    // drop shadow in SVG file. Remove / modify this workaround if QMPlay2 SVG icon changes!
    const QSize size(128, 128);
    const QPixmap pixmap = QIcon(":/QMPlay2.svgz").pixmap(size);
    const qreal sizeRatio = (((qreal)pixmap.size().width()  / (qreal)size.width()) + ((qreal)pixmap.size().height() / (qreal)size.height())) / 2.0;
    qmplay2Icon = new QIcon(Functions::applyDropShadow(pixmap, 10.0 * sizeRatio, QPointF(3.0 * sizeRatio, 3.0 * sizeRatio), QColor(0, 0, 0, 127)));

    groupIcon = new QIcon(getIconFromTheme("folder-orange"));
    mediaIcon = new QIcon(getIconFromTheme("applications-multimedia"));
    folderIcon = new QIcon(getIconFromTheme("folder"));
}
void QMPlay2GUIClass::deleteIcons()
{
    delete qmplay2Icon;
    delete groupIcon;
    delete mediaIcon;
    delete folderIcon;
    qmplay2Icon = groupIcon = mediaIcon = folderIcon = nullptr;
}

QString QMPlay2GUIClass::getCurrentPth(QString pth, bool leaveFilename)
{
    if (pth.startsWith("file://"))
        pth.remove(0, 7);
    if (!leaveFilename)
        pth = Functions::filePath(pth);
    if (!QFileInfo::exists(pth))
        pth = settings->getString("currPth");
    return pth;
}
void QMPlay2GUIClass::setCurrentPth(const QString &pth)
{
    settings->set("currPth", Functions::cleanPath(pth));
}

void QMPlay2GUIClass::restoreGeometry(const QString &pth, QWidget *w, const int defaultSizePercent)
{
    const QRect geo = settings->getRect(pth);
    QDesktopWidget *desktop = QApplication::desktop();
    if (geo.isValid())
    {
        w->setGeometry(geo);
        if (!desktop->screenGeometry(w).contains(w->pos()))
            w->move(desktop->width() / 2 - w->width() / 2, desktop->height() / 2 - w->height() / 2);
    }
    else
    {
        const QSize defaultSize = desktop->availableGeometry(w).size() * defaultSizePercent / 100;
        w->move(desktop->width() / 2 - defaultSize.width() / 2, desktop->height() / 2 - defaultSize.height() / 2);
        w->resize(defaultSize);
    }
}

void QMPlay2GUIClass::updateInDockW()
{
    qobject_cast<MainWidget *>(mainW)->videoDock->updateIDW();
}

const QWidget *QMPlay2GUIClass::getVideoDock() const
{
    return qobject_cast<MainWidget *>(mainW)->videoDock;
}
const QWidget *QMPlay2GUIClass::getMainWindow() const
{
    return mainW;
}

QMPlay2GUIClass::QMPlay2GUIClass() :
    groupIcon(nullptr), mediaIcon(nullptr), folderIcon(nullptr),
    mainW(nullptr),
    screenSaver(nullptr),
    shortcutHandler(nullptr)
{}
QMPlay2GUIClass::~QMPlay2GUIClass()
{}

/**/

static QCommandLineParser *createCmdParser(bool descriptions)
{
    static constexpr const char *translations[] = {
        QT_TRANSLATE_NOOP("Help", "Open and play specified <url>."),
        QT_TRANSLATE_NOOP("Help", "Open new QMPlay2 instance and play specified <url>."),
        QT_TRANSLATE_NOOP("Help", "Add specified <url> to playlist."),
        QT_TRANSLATE_NOOP("Help", "Start the application with given <profile name>."),
        QT_TRANSLATE_NOOP("Help", "Don't play after run (bypass \"Remember playback position\" option)."),
        QT_TRANSLATE_NOOP("Help", "Toggle playback."),
        QT_TRANSLATE_NOOP("Help", "Start playback."),
        QT_TRANSLATE_NOOP("Help", "Stop playback."),
        QT_TRANSLATE_NOOP("Help", "Ensure that the window will be visible if the application is running."),
        QT_TRANSLATE_NOOP("Help", "Toggle fullscreen."),
        QT_TRANSLATE_NOOP("Help", "Set specified volume."),
        QT_TRANSLATE_NOOP("Help", "Set specified playback speed."),
        QT_TRANSLATE_NOOP("Help", "Seek to the specified value."),
        QT_TRANSLATE_NOOP("Help", "Play next entry on playlist."),
        QT_TRANSLATE_NOOP("Help", "Play previous entry on playlist."),
        QT_TRANSLATE_NOOP("Help", "Terminate the application."),
        QT_TRANSLATE_NOOP("Help", "Display this help."),
    };

    const auto maybeGetTranslatedText = [&](const char *text) {
        if (descriptions)
            return QCoreApplication::translate("Help", text);
        return QString();
    };

    QCommandLineParser *parser = new QCommandLineParser;
    parser->addPositionalArgument("<url>", maybeGetTranslatedText(translations[0]), "[url]");
    parser->addOptions({
        {"open", maybeGetTranslatedText(translations[0]), "url"},
        {"opennew", maybeGetTranslatedText(translations[1]), "url"},
        {"enqueue", maybeGetTranslatedText(translations[2]), "url"},
        {"profile", maybeGetTranslatedText(translations[3]), "profile name"},
        {"noplay", maybeGetTranslatedText(translations[4])},
        {"toggle", maybeGetTranslatedText(translations[5])},
        {"play", maybeGetTranslatedText(translations[6])},
        {"stop", maybeGetTranslatedText(translations[7])},
        {"show", maybeGetTranslatedText(translations[8])},
        {"fullscreen", maybeGetTranslatedText(translations[9])},
        {"volume", maybeGetTranslatedText(translations[10]), "0..100"},
        {"speed", maybeGetTranslatedText(translations[11]), "0.05..100.0"},
        {"seek", maybeGetTranslatedText(translations[12]), "s"},
        {"next", maybeGetTranslatedText(translations[13])},
        {"prev", maybeGetTranslatedText(translations[14])},
        {"quit", maybeGetTranslatedText(translations[15])},
        {{"h", "help"}, maybeGetTranslatedText(translations[16])},
    });

    return parser;
}
static QString fileArg(const QString &arg)
{
    if (!arg.contains("://"))
    {
        const QFileInfo argInfo(arg);
        if (!argInfo.isAbsolute())
            return argInfo.absoluteFilePath();
    }
    return arg;
}
static QList<QPair<QString, QString>> parseArguments(const QCommandLineParser &parser)
{
    QList<QPair<QString, QString>> arguments;
    for (const QString &option : parser.optionNames())
    {
        QString value = parser.value(option);
        if (option == "open" || option == "enqueue")
            value = fileArg(value);
        arguments += {option, value};
    }

    QString urlLines;
    for (const QString &url : parser.positionalArguments())
        urlLines += fileArg(url) + "\n";
    urlLines.chop(1);
    if (!urlLines.isEmpty())
    {
        bool found = false;
        for (int i = arguments.count() - 1; i >= 0; --i)
        {
            if (arguments.at(i).first == "open" || arguments.at(i).first == "enqueue")
            {
                arguments[i].second += "\n" + urlLines;
                found = true;
                break;
            }
        }
        if (!found)
            arguments += {"open", urlLines};
    }

    return arguments;
}

static bool writeToSocket(IPCSocket &socket, QList<QPair<QString, QString>> &arguments)
{
    bool ret = false;

    for (auto &&argument : arguments)
    {
        if (argument.first == "noplay" || argument.first == "profile")
            continue;

        if (argument.first == "open" || argument.first == "enqueue")
        {
            if (!argument.second.isEmpty())
                argument.second = Functions::Url(argument.second);
#ifdef Q_OS_WIN
            if (argument.second.startsWith("file://"))
                argument.second.remove(0, 7);
#endif
        }
        socket.write(QString(argument.first + '\t' + argument.second).toUtf8() + '\0');
        ret = true;
    }

    return ret;
}

static inline void exitProcedure()
{
    delete g_screenSaver;
    g_screenSaver = nullptr;
}

#ifndef Q_OS_WIN
    #include <csetjmp>
    static jmp_buf env;
    static bool qAppOK;
    static bool canDeleteApp = true;
#endif

static inline void forceKill()
{
#ifdef Q_OS_WIN
    const int s = SIGBREAK;
#else
    const int s = SIGKILL;
#endif
    exitProcedure();
    raise(s);
}
static inline void callDefaultSignalHandler(int s)
{
    exitProcedure();
    signal(s, SIG_DFL);
    raise(s);
}
static void signal_handler(int s)
{
    const char *crashSignal = nullptr;
    switch (s)
    {
        case SIGINT:
        case SIGTERM:
            if (!qApp)
                forceKill();
            else
            {
                static bool first = true;
                QWidget *modalW = QApplication::activeModalWidget();
                if (!modalW && QMPlay2GUI.mainW && first)
                {
                    QMPlay2GUI.mainW->close();
                    first = false;
                }
                else
                {
                    forceKill();
                }
            }
            break;
        case SIGABRT:
#ifndef Q_OS_WIN
            if (!qAppOK && g_useGui)
            {
                canDeleteApp = g_useGui = false;
                longjmp(env, 1);
            }
#endif
            QMPlay2Core.log("QMPlay2 has been aborted (SIGABRT)", ErrorLog | AddTimeToLog | (qApp ? SaveLog : DontShowInGUI));
            callDefaultSignalHandler(s);
            break;
        case SIGILL:
            crashSignal = "SIGILL";
            break;
        case SIGFPE:
            crashSignal = "SIGFPE";
            break;
#ifndef Q_OS_WIN
        case SIGBUS:
            crashSignal = "SIGBUS";
            break;
#endif
        case SIGSEGV:
            crashSignal = "SIGSEGV";
            break;
    }
    if (crashSignal)
    {
        QMPlay2Core.log(QString("QMPlay2 crashed (%1)").arg(crashSignal), ErrorLog | AddTimeToLog | (qApp ? SaveLog : DontShowInGUI));
        callDefaultSignalHandler(s);
    }
}

#ifdef Q_OS_WIN
static LRESULT CALLBACK MMKeysHookProc(int code, WPARAM wparam, LPARAM lparam)
{
    if (code == HC_ACTION && wparam == WM_KEYDOWN)
    {
        PKBDLLHOOKSTRUCT kbd = (PKBDLLHOOKSTRUCT)lparam;
        if (kbd)
        {
            switch (kbd->vkCode)
            {
                case VK_MEDIA_NEXT_TRACK:
                    emit QMPlay2Core.processParam("next");
                    return 1;
                case VK_MEDIA_PREV_TRACK:
                    emit QMPlay2Core.processParam("prev");
                    return 1;
                case VK_MEDIA_STOP:
                    emit QMPlay2Core.processParam("stop");
                    return 1;
                case VK_MEDIA_PLAY_PAUSE:
                    emit QMPlay2Core.processParam("toggle");
                    return 1;
            }
        }
    }
    return CallNextHookEx(nullptr, code, wparam, lparam);
}
#endif

static QtMessageHandler g_defaultMsgHandler = nullptr;
static QMutex g_messageHandlerMutex;
static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
#ifdef USE_VULKAN
    if (type == QtWarningMsg && context.category && qstrcmp(context.category, "default") == 0 && message.startsWith("failed to load vulkan", Qt::CaseInsensitive))
        return;
#endif
    bool qmplay2Log = false;
    if (QCoreApplication::instance())
    {
        // Use QMPlay2 logger only when we have a "QApplication" instance (we're still executing "main()"),
        // so any static data including "QSystemLocaleSingleton" and "QMPlay2CoreClass" are still valid.
        switch (type)
        {
            case QtWarningMsg:
            case QtCriticalMsg:
            case QtFatalMsg:
                g_messageHandlerMutex.lock();
                QMPlay2Core.logError(qFormatLogMessage(type, context, message), false);
                g_messageHandlerMutex.unlock();
                qmplay2Log = true;
                break;
            case QtDebugMsg:
            case QtInfoMsg:
                if (type != QtDebugMsg || qstrcmp(context.category, "js") == 0)
                {
                    g_messageHandlerMutex.lock();
                    QMPlay2Core.logInfo(qFormatLogMessage(type, context, message), false);
                    g_messageHandlerMutex.unlock();
                    qmplay2Log = true;
                }
                break;
            default:
                break;
        }
    }
    if (!qmplay2Log)
    {
#ifdef Q_OS_ANDROID
        if (g_defaultMsgHandler)
            g_defaultMsgHandler(type, context, message);
#else
        g_messageHandlerMutex.lock();
        fprintf(stderr, "%s\n", qFormatLogMessage(type, context, message).toLocal8Bit().constData());
        fflush(stderr);
        g_messageHandlerMutex.unlock();
#endif
    }
}

#ifdef CHECK_FOR_EGL
static void checkForEGL()
{
    // FIXME: Non-X11 environment must also get the device file.

    if (!qEnvironmentVariableIsSet("DISPLAY"))
        return;

    QLibrary libX11("libX11.so.6");
    QLibrary libEGL("libEGL.so.1");
    if (!libX11.load() || !libEGL.load())
        return;

    using XOpenDisplayType = void *(*)(const char *name);
    using XCloseDisplayType = int (*)(void *display);

    auto XOpenDisplayFunc = (XOpenDisplayType)libX11.resolve("XOpenDisplay");
    auto XCloseDisplayFunc = (XCloseDisplayType)libX11.resolve("XCloseDisplay");
    if (!XOpenDisplayFunc || !XCloseDisplayFunc)
        return;

    using eglGetProcAddressType = void *(*)(const char *);
    using eglGetDisplayType = void *(*)(void *);
    using eglInitializeType = unsigned (*)(void *, int *, int *);
    using eglQueryStringType = const char *(*)(void *, int);
    using eglQueryDisplayAttribEXTType = unsigned (*)(void *, int, void *);
    using eglTerminateType = unsigned (*)(void *);

    auto eglGetProcAddress = (eglGetProcAddressType)libEGL.resolve("eglGetProcAddress");
    auto eglGetDisplayFunc = (eglGetDisplayType)libEGL.resolve("eglGetDisplay");
    auto eglInitializeFunc = (eglInitializeType)libEGL.resolve("eglInitialize");
    auto eglQueryStringFunc = (eglQueryStringType)libEGL.resolve("eglQueryString");
    auto eglTerminateFunc = (eglTerminateType)libEGL.resolve("eglTerminate");
    if (!eglGetProcAddress || !eglGetDisplayFunc || !eglInitializeFunc || !eglQueryStringFunc || !eglTerminateFunc)
        return;

    auto dpy = XOpenDisplayFunc(nullptr);
    if (!dpy)
        return;

    QByteArray cardFilePath;

    auto eglDpy = eglGetDisplayFunc(dpy);
    if (eglDpy && eglInitializeFunc(eglDpy, nullptr, nullptr))
    {
        constexpr int EGLVendor = 0x3053;
        const QByteArray eglVendor = eglQueryStringFunc(eglDpy, EGLVendor);
        if (eglVendor == "Mesa Project") // Don't allow to run Qt over EGL on NVIDIA
        {
            constexpr int EGLExtensions = 0x3055;
            const bool hasDeviceQuery = QByteArray(eglQueryStringFunc(nullptr, EGLExtensions)).contains("EGL_EXT_device_query");

            auto eglQueryDisplayAttribEXTFunc = (eglQueryDisplayAttribEXTType)eglGetProcAddress("eglQueryDisplayAttribEXT");
            auto eglQueryDeviceStringEXTFunc = (eglQueryStringType)eglGetProcAddress("eglQueryDeviceStringEXT");

            if (hasDeviceQuery && eglQueryDisplayAttribEXTFunc && eglQueryDeviceStringEXTFunc)
            {
                constexpr int EGLDeviceExt = 0x322C;
                constexpr int EGLDrmDeviceFileExt = 0x3233;
                void *eglDev = nullptr;
                if (eglQueryDisplayAttribEXTFunc(eglDpy, EGLDeviceExt, &eglDev) && eglDev)
                {
                    if (const char *file = eglQueryDeviceStringEXTFunc(eglDev, EGLDrmDeviceFileExt))
                        cardFilePath = file;
                    else
                        cardFilePath = "";
                }
            }
        }
        eglTerminateFunc(eglDpy);
    }

    XCloseDisplayFunc(dpy);

    if (!cardFilePath.isNull())
    {
        if (!qEnvironmentVariableIsSet("QT_XCB_GL_INTEGRATION"))
            qputenv("QT_XCB_GL_INTEGRATION", "xcb_egl");
        if (!qEnvironmentVariableIsSet("QMPLAY2_EGL_CARD_FILE_PATH") && !cardFilePath.isEmpty())
            qputenv("QMPLAY2_EGL_CARD_FILE_PATH", cardFilePath);
    }
}
#endif

#ifdef Q_OS_ANDROID
Q_DECL_EXPORT
#endif
int main(int argc, char *argv[])
{
    signal(SIGINT, signal_handler);
    signal(SIGABRT, signal_handler);
    signal(SIGILL, signal_handler);
    signal(SIGFPE, signal_handler);
#ifndef Q_OS_WIN
    signal(SIGBUS, signal_handler);
#endif
    signal(SIGSEGV, signal_handler);
    signal(SIGTERM, signal_handler);
    atexit(exitProcedure);

    qputenv("QT_QPA_UPDATE_IDLE_TIME", "0");

    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#ifndef USE_OPENGL
    QGuiApplication::setAttribute(Qt::AA_ForceRasterWidgets);
#endif

#ifdef Q_OS_MACOS
    auto fmt = QSurfaceFormat::defaultFormat();
    fmt.setColorSpace(QSurfaceFormat::sRGBColorSpace);
    QSurfaceFormat::setDefaultFormat(fmt);
#endif

#ifndef Q_OS_WIN
    if (!setjmp(env))
#endif
    new QApplication(argc, argv);

    // Qt sets this to system locale. Mesa Radeon VA-API driver lowers the image quality
    // on some locales in some cases. Reset it for all platforms for consistency.
    std::setlocale(LC_NUMERIC, "C");

#ifndef Q_OS_WIN
    qAppOK = true;
#endif
    QCoreApplication::setApplicationName("QMPlay2");

    QMPlay2GUIClass &qmplay2Gui = QMPlay2GUI; //Create "QMPlay2GUI" instance
    g_defaultMsgHandler = qInstallMessageHandler(messageHandler);

    QCommandLineParser *parser = createCmdParser(false);
    parser->setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser->process(*qApp);
    QList<QPair<QString, QString>> arguments = parseArguments(*parser);
    const bool help = parser->isSet("help");
    QString cmdLineProfile = parser->value("profile");
    delete parser;

    if (!help)
    {
        bool useSocket = true;

        for (auto &&argument : arguments)
        {
            if (argument.first == "opennew")
            {
                argument.first = "open";
                useSocket = false;
                break;
            }
        }

        if (useSocket)
        {
            IPCSocket socket(qmplay2Gui.getPipe());
            if (socket.open(IPCSocket::WriteOnly))
            {
                if (writeToSocket(socket, arguments))
                    g_useGui = false;
                socket.close();
            }
#ifndef Q_OS_WIN
            else if (QFile::exists(qmplay2Gui.getPipe()))
            {
                QFile::remove(qmplay2Gui.getPipe());
                arguments.append({"noplay", QString()});
            }
#endif
        }

        if (!g_useGui)
        {
#ifndef Q_OS_WIN
            if (canDeleteApp)
#endif
                delete qApp;
            return 0;
        }
    }

#ifdef CHECK_FOR_EGL
    if (!help)
        checkForEGL();
#endif

    qmplay2Gui.cmdLineProfile = std::move(cmdLineProfile);

    QString libPath, sharePath = QCoreApplication::applicationDirPath();
    bool cmakeBuildFound = false;
#ifdef Q_OS_MACOS
    if (QDir(sharePath).exists("../../../CMakeFiles/QMPlay2.dir"))
        sharePath += "/../../.."; //Probably CMake not-installed build in Bundle
#endif
    if (QDir(sharePath).exists("CMakeFiles/QMPlay2.dir"))
    {
        //We're in CMake not-installed build
        libPath = sharePath + "/.."; //Here is "modules" directory
        sharePath += "/../.."; //Here is "lang" directory
        if (QDir(libPath).exists("modules") || QDir(sharePath).exists("lang"))
            cmakeBuildFound = true;
    }
    if (!cmakeBuildFound)
    {
#if !defined Q_OS_WIN && !defined Q_OS_MACOS && !defined Q_OS_ANDROID
        sharePath = QCoreApplication::applicationDirPath() + "/../share/qmplay2";
        libPath = QMPlay2CoreClass::getLibDir();
        if (libPath.isEmpty() || !QDir(libPath).exists("qmplay2"))
        {
            libPath += "/../";
            if (sizeof(void *) == 8 && QDir(libPath).exists("lib64/qmplay2"))
                libPath += "lib64";
            else if (sizeof(void *) == 4 && QDir(libPath).exists("lib32/qmplay2"))
                libPath += "lib32";
            else
                libPath += "lib";
        }
        libPath += "/qmplay2";
#elif defined Q_OS_MACOS
        libPath = QCoreApplication::applicationDirPath();
        sharePath += "/../share/qmplay2";
#else
        libPath = sharePath;
#endif
    }

    qRegisterMetaType<AVRational>("AVRational");
    qRegisterMetaType<Frame>("Frame");

    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);

    QDir::setCurrent(QCoreApplication::applicationDirPath()); //Is it really needed?

    if (g_useGui)
    {
        qmplay2Gui.screenSaver = g_screenSaver = new ScreenSaver;
        QApplication::setQuitOnLastWindowClosed(false);
        qApp->installEventFilter(new EventFilterWorkarounds(qApp));
        PanGestureEventFilter::install();
    }

#ifdef Q_OS_WIN
    HHOOK keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, MMKeysHookProc, GetModuleHandle(nullptr), 0);
#endif

    qsrand(time(nullptr));

    do
    {
        /* QMPlay2GUI musi być stworzone już wcześniej */
        QMPlay2Core.init(!help, cmakeBuildFound, libPath, sharePath, qmplay2Gui.cmdLineProfile);

        if (help)
        {
            parser = createCmdParser(true);
            parser->setApplicationDescription(QString("QMPlay2 - Qt Media Player 2 (%1)").arg((QString)Version::get()));
            printf("%s", parser->helpText().toLocal8Bit().constData());
            fflush(stdout);
            delete parser;
            break;
        }

        if (!qmplay2Gui.cmdLineProfile.isEmpty() && QMPlay2Core.getSettingsProfile() == "/")
            qmplay2Gui.cmdLineProfile = QMPlay2Core.getSettingsProfile(); // Default profile

        Settings &settings = QMPlay2Core.getSettings();
        QByteArray lastVer = settings.getByteArray("Version", Version::get());
        settings.set("Version", Version::get());
        settings.set("LastQMPlay2Path", QCoreApplication::applicationDirPath());

        if (Functions::parseVersion(lastVer) < QDate(2015, 10, 9))
        {
            QFile::remove(QMPlay2Core.getSettingsDir() + "OpenGL.ini");
            settings.remove("audioWriters");
            settings.remove("videoWriters");
        }
        if (settings.contains("Volume"))
        {
            const int vol = settings.getInt("Volume");
            settings.set("VolumeL", vol);
            settings.set("VolumeR", vol);
            settings.remove("Volume");
        }

        qmplay2Gui.loadIcons();
        {
            const QIcon svgIcon = QIcon(":/QMPlay2.svgz");
            if (svgIcon.availableSizes().size() == 1)
                QMessageBox::warning(nullptr, QString(), QObject::tr("QtSvg icon engine plugin doesn't exist.\nQMPlay2 will not scale up icons!"));
        }

        qmplay2Gui.pipe = new IPCServer(qmplay2Gui.getPipe());
        if
        (
#ifdef Q_OS_WIN
            WaitNamedPipeW((const wchar_t *)qmplay2Gui.getPipe().utf16(), NMPWAIT_USE_DEFAULT_WAIT) ||
#endif
            !qmplay2Gui.pipe->listen()
        )
        {
            delete qmplay2Gui.pipe;
            qmplay2Gui.pipe = nullptr;
#ifdef QMPLAY2_ALLOW_ONLY_ONE_INSTANCE
            if (settings.getBool("AllowOnlyOneInstance"))
            {
                QMPlay2Core.quit();
                IPCSocket socket(qmplay2Gui.getPipe());
                if (socket.open(IPCSocket::WriteOnly))
                {
                    socket.write(QByteArray("show\t") + '\0');
                    socket.close();
                }
                break;
            }
#endif
        }

        QApplication::setWindowIcon(QMPlay2Core.getIconFromTheme("QMPlay2", QMPlay2Core.getQMPlay2Icon()));
        qmplay2Gui.setStyle();

#ifdef UPDATER
        QString UpdateFile = settings.getString("UpdateFile");
        if (UpdateFile.startsWith("remove:"))
        {
            UpdateFile.remove(0, 7);
            if (lastVer != Version::get())
            {
                const QString updateString = QObject::tr("QMPlay2 has been updated to version") + " " + Version::get();
                QMPlay2Core.logInfo(updateString);
                QMessageBox::information(nullptr, QCoreApplication::applicationName(), updateString);
                settings.remove("UpdateVersion");
            }
            else
            {
                const QString message = QObject::tr("QMPlay2 hasn't been updated. Do you want to run the update (recommended)?");
                if (QMessageBox::question(nullptr, QCoreApplication::applicationName(), message, QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
                {
                    qmplay2Gui.runUpdate(UpdateFile);
                    QMPlay2Core.quit();
                    break;
                }
            }
            QFile::remove(UpdateFile);
            settings.remove("UpdateFile");
            settings.flush();
        }
        else if (QFileInfo(UpdateFile).size())
        {
            qmplay2Gui.runUpdate(UpdateFile);
            QMPlay2Core.quit();
            break;
        }
        UpdateFile.clear();
#endif

        lastVer.clear();

        qmplay2Gui.restartApp = qmplay2Gui.removeSettings = qmplay2Gui.noAutoPlay = false;
        qmplay2Gui.newProfileName.clear();
        new MainWidget(arguments);
        do
        {
            QCoreApplication::exec();
            // Go back to event queue and allow to close the main window (and playback) if
            // QCoreApplication exits too early. Reproducible on macOS when closing from dock.
        } while (qmplay2Gui.mainW);

        const QString settingsDir = QMPlay2Core.getSettingsDir();
        const QString profile = QMPlay2Core.getSettingsProfile();

        QMPlay2Core.quit();

        if (qmplay2Gui.removeSettings)
        {
            const QString settingsDirProfile = settingsDir + profile;
            for (const QString &fName : QDir(settingsDirProfile).entryList({"*.ini"}))
                QFile::remove(settingsDirProfile + fName);
            if (profile != "/")
                QDir(settingsDir).rmdir(profile);
        }
        else if (!qmplay2Gui.newProfileName.isEmpty())
        {
            const QString srcDir = settingsDir + profile;
            const QString dstDir = settingsDir + "Profiles/" + qmplay2Gui.newProfileName + "/";
            for (const QString &fName : QDir(srcDir).entryList({"*.ini"}, QDir::Files))
            {
                if (fName != "Profile.ini")
                    QFile::copy(srcDir + fName, dstDir + fName);
            }
        }

        if (qmplay2Gui.noAutoPlay)
            arguments.append({"noplay", QString()});

        delete qmplay2Gui.pipe;
    } while (qmplay2Gui.restartApp);

    qmplay2Gui.deleteIcons();

#ifdef Q_OS_WIN
    UnhookWindowsHookEx(keyboardHook);
#endif

    exitProcedure();

#ifndef Q_OS_WIN
    if (canDeleteApp)
#endif
        delete qApp;
    return 0;
}
