// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "OpenGLWindow.hpp"

#include <QMPlay2Core.hpp>

#include <QGuiApplication>
#include <QOpenGLContext>
#include <QDockWidget>

OpenGLWindow::OpenGLWindow()
    : m_platformName(QGuiApplication::platformName())
    , m_passEventsToParent(
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
          !m_platformName.contains(QStringLiteral("wayland")) &&
          m_platformName != QStringLiteral("windows") &&
#endif
          m_platformName != QStringLiteral("xcb") && m_platformName != QStringLiteral("android"))
{
    connect(&updateTimer, SIGNAL(timeout()), this, SLOT(doUpdateGL()));

    if (!m_passEventsToParent)
        setFlags(Qt::WindowTransparentForInput);

    m_widget = QWidget::createWindowContainer(this);
    if (!m_platformName.contains(QStringLiteral("wayland")) && m_platformName != QStringLiteral("android"))
        m_widget->setAttribute(Qt::WA_NativeWindow);
    m_widget->installEventFilter(this);
    m_widget->setAcceptDrops(false);

    connect(&QMPlay2Core, SIGNAL(videoDockVisible(bool)), this, SLOT(videoVisible(bool)));
}
OpenGLWindow::~OpenGLWindow()
{
    makeCurrent();
}

void OpenGLWindow::deleteMe()
{
    delete m_widget;
}

bool OpenGLWindow::makeContextCurrent()
{
    if (!context())
        return false;

    makeCurrent();
    return true;
}
void OpenGLWindow::doneContextCurrent()
{
    doneCurrent();
}

void OpenGLWindow::setVSync(bool enable)
{
    QSurfaceFormat fmt = format();
    if (!handle())
    {
        fmt.setSwapBehavior(QSurfaceFormat::DoubleBuffer); //Probably it doesn't work
        fmt.setSwapInterval(enable);
        setFormat(fmt);
    }
    else if (enable != fmt.swapInterval())
    {
        fmt.setSwapInterval(enable);
        destroy();
        setFormat(fmt);
        create();
        setVisible(true);
    }
    vSync = enable;
}
void OpenGLWindow::updateGL(bool requestDelayed)
{
    if (m_visible && isExposed())
        QMetaObject::invokeMethod(this, "doUpdateGL", Qt::QueuedConnection, Q_ARG(bool, requestDelayed));
}

void OpenGLWindow::initializeGL()
{
    connect(context(), SIGNAL(aboutToBeDestroyed()), this, SLOT(aboutToBeDestroyed()), Qt::DirectConnection);
    OpenGLCommon::initializeGL();
}
void OpenGLWindow::paintGL()
{
    if (isExposed())
    {
        glClear(GL_COLOR_BUFFER_BIT);
        OpenGLCommon::paintGL();
    }
}

void OpenGLWindow::doUpdateGL(bool queued)
{
    if (queued)
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest), Qt::LowEventPriority);
    else
    {
        //sendEvent() doesn't enqueue the event here
        QEvent updateEvent(QEvent::UpdateRequest);
        QCoreApplication::sendEvent(this, &updateEvent);
    }
}
void OpenGLWindow::aboutToBeDestroyed()
{
    makeCurrent();
    contextAboutToBeDestroyed();
    doneCurrent();
}
void OpenGLWindow::videoVisible(bool v)
{
    m_visible = v && (m_widget->visibleRegion() != QRegion() || QMPlay2Core.getVideoDock()->visibleRegion() != QRegion());
}

bool OpenGLWindow::eventFilter(QObject *o, QEvent *e)
{
    if (o == m_widget)
        dispatchEvent(e, m_widget->parent());
    return false;
}

bool OpenGLWindow::event(QEvent *e)
{
    switch (e->type())
    {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseMove:
        case QEvent::FocusIn:
        case QEvent::FocusOut:
        case QEvent::FocusAboutToChange:
        case QEvent::Enter:
        case QEvent::Leave:
        case QEvent::TabletMove:
        case QEvent::TabletPress:
        case QEvent::TabletRelease:
        case QEvent::TabletEnterProximity:
        case QEvent::TabletLeaveProximity:
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
        case QEvent::TouchEnd:
        case QEvent::InputMethodQuery:
        case QEvent::TouchCancel:
            if (m_passEventsToParent)
                return QCoreApplication::sendEvent(parent(), e);
        case QEvent::Wheel:
            if (m_passEventsToParent)
                return QCoreApplication::sendEvent(const_cast<QWidget *>(QMPlay2Core.getVideoDock()), e);
        default:
            break;
    }
    return QOpenGLWindow::event(e);
}
