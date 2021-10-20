#include <CommonJS.hpp>

#include <NetworkAccess.hpp>
#include <QMPlay2Core.hpp>
#include <Functions.hpp>
#include <YouTubeDL.hpp>

#include <QTextDocumentFragment>
#include <QLoggingCategory>
#include <QJSEngine>
#include <QTimer>

Q_LOGGING_CATEGORY(js, "JS")

CommonJS::CommonJS(QObject *parent)
    : QObject(parent)
{}
CommonJS::~CommonJS()
{}

int CommonJS::insertNetworkReply(NetworkReply *networkReply)
{
    if (!networkReply)
        return 0;

    QMutexLocker locker(&m_networkReplyMutex);
    const auto id = ++m_networkReplyId;
    m_networkReplies[id] = networkReply;

    connect(networkReply, &NetworkReply::destroyed,
            this, [=] {
        removeNetworkReply(id);
    });

    return id;
}
NetworkReply *CommonJS::getNetworkReply(const int id) const
{
    QMutexLocker locker(&m_networkReplyMutex);
    return m_networkReplies[id];
}
void CommonJS::removeNetworkReply(const int id)
{
    QMutexLocker locker(&m_networkReplyMutex);
    m_networkReplies.remove(id);
}

int CommonJS::insertIOController(IOController<> *ioCtrl)
{
    if (!ioCtrl)
        return 0;

    QMutexLocker locker(&m_ioControllerMutex);
    const auto id = ++m_ioControllerId;
    m_ioControllers[id] = ioCtrl;
    return id;
}
IOController<> *CommonJS::getIOController(const int id) const
{
    QMutexLocker locker(&m_ioControllerMutex);
    return m_ioControllers[id];
}
void CommonJS::removeIOController(const int id)
{
    QMutexLocker locker(&m_ioControllerMutex);
    m_ioControllers.remove(id);
}

bool CommonJS::abortNetworkReply(const int id)
{
    auto networkReply = getNetworkReply(id);
    if (networkReply)
    {
        networkReply->abort();
        return true;
    }
    return false;
}

bool CommonJS::isIOControllerAborted(const int id)
{
    const auto ioCtrl = getIOController(id);
    return (!ioCtrl || ioCtrl->isAborted());
}

int CommonJS::startTimer(const int ms, const bool singleShot, QJSValue onTimeout)
{
    if (!onTimeout.isCallable())
        return 0;

    auto timer = new QTimer(this);
    timer->setSingleShot(singleShot);
    timer->start(ms);

    QMutexLocker locker(&m_timerMutex);
    const auto id = ++m_timerId;
    m_timers[id] = timer;

    connect(timer, &QTimer::timeout,
            this, [=]() mutable {
        onTimeout.call();
        if (timer->isSingleShot())
            stopTimer(id);
    });

    return id;
}
void CommonJS::stopTimer(const int id)
{
    QMutexLocker locker(&m_timerMutex);
    auto it = m_timers.find(id);
    if (it != m_timers.end())
    {
        delete it.value();
        m_timers.erase(it);
    }
}

QString CommonJS::fromHtml(const QString &html)
{
    return QTextDocumentFragment::fromHtml(html).toPlainText();
}

QByteArray CommonJS::toBase64(const QByteArray &data, const int options)
{
    return data.toBase64(static_cast<QByteArray::Base64Options>(qBound(0, options, 2)));
}
QByteArray CommonJS::fromBase64(const QByteArray &data)
{
    return QByteArray::fromBase64(data);
}

QByteArray CommonJS::toHex(const QByteArray &data)
{
    return data.toHex();
}
QByteArray CommonJS::fromHex(const QByteArray &data)
{
    return QByteArray::fromHex(data);
}

QByteArray CommonJS::decryptAes256Cbc(const QByteArray &password, const QByteArray &salt, const QByteArray &ciphered)
{
    return Functions::decryptAes256Cbc(password, salt, ciphered);
}

QVariantMap CommonJS::youTubeDlFixUrl(const QString &url, const int ioControllerId, const bool nameAvail, const bool extensionAvail, const bool errorAvail)
{
    if (auto ioCtrl = getIOController(ioControllerId))
    {
        QString outUrl, name, extension, error;
        const bool ok = YouTubeDL::fixUrl(
            url,
            outUrl,
            ioCtrl,
            nameAvail ? &name : nullptr,
            extensionAvail ? &extension : nullptr,
            errorAvail ? &error : nullptr
        );
        return {
            {"ok", ok},
            {"url", outUrl},
            {"name", name},
            {"extension", extension},
            {"error", error},
        };
    }
    return {
        {"ok", false}
    };
}

QString CommonJS::timeToStr(const double t, const bool decimals)
{
    return Functions::timeToStr(t, decimals);
}

void CommonJS::sendMessage(const QString &msg, const QString &title, int messageIcon, int ms)
{
    QMPlay2Core.sendMessage(msg, title, messageIcon, ms);
}

void CommonJS::addCookies(const QString &url, const QByteArray &newCookies, const bool removeAfterUse)
{
    QMPlay2Core.addCookies(url, newCookies, removeAfterUse);
}
QByteArray CommonJS::getCookies(const QString &url)
{
    return QMPlay2Core.getCookies(url);
}

void CommonJS::addRawHeaders(const QString &url, const QByteArray &data, const bool removeAfterUse)
{
    QMPlay2Core.addRawHeaders(url, data, removeAfterUse);
}
QByteArray CommonJS::getRawheaders(const QString &url)
{
    return QMPlay2Core.getRawHeaders(url);
}

void CommonJS::addNameForUrl(const QString &url, const QString &name, const bool removeAfterUse)
{
    QMPlay2Core.addNameForUrl(url, name, removeAfterUse);
}
QString CommonJS::getNameForUrl(const QString &url)
{
    return QMPlay2Core.getNameForUrl(url);
}
