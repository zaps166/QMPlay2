#include <NetworkAccessJS.hpp>

#include <NetworkAccess.hpp>
#include <QMPlay2Core.hpp>
#include <CommonJS.hpp>

#include <QLoggingCategory>

static void getStandardArgs(const QJSValue &args,
                            QString &url,
                            QByteArray &postData,
                            QByteArray &rawHeaders,
                            int *retries = nullptr)
{
    if (args.isString())
    {
        url = args.toString();
        if (retries)
            *retries = -1;
        return;
    }

    const QVariantMap argsMap = args.toVariant().toMap();
    url = argsMap["url"].toString();
    postData = argsMap["post"].toByteArray();
    for (auto &&line : argsMap["headers"].toList())
    {
        rawHeaders.push_back(line.toByteArray());
        if (!rawHeaders.endsWith("\r\n"))
            rawHeaders.push_back("\r\n");
    }
    if (retries)
    {
        bool ok = false;
        *retries = argsMap["retries"].toInt(&ok);
        if (!ok)
            *retries = -1;
    }
}

/**/

NetworkAccessJS::NetworkAccessJS()
    : m_net(new NetworkAccess(this))
{}
NetworkAccessJS::NetworkAccessJS(NetworkAccess &net, QObject *parent)
    : QObject(parent)
    , m_net(&net)
{}
NetworkAccessJS::~NetworkAccessJS()
{}

QString NetworkAccessJS::urlEncoded()
{
    return NetworkAccess::UrlEncoded;
}

void NetworkAccessJS::setCustomUserAgent(const QString &customUserAgent)
{
    m_net->setCustomUserAgent(customUserAgent);
}
void NetworkAccessJS::setMaxDownloadSize(const int maxSize)
{
    m_net->setMaxDownloadSize(maxSize);
}
void NetworkAccessJS::setRetries(const int retries)
{
    return m_net->setRetries(retries);
}

int NetworkAccessJS::getRetries() const
{
    return m_net->getRetries();
}

int NetworkAccessJS::start(QJSValue args, QJSValue onFinished, QJSValue onProgress)
{
    QString url;
    QByteArray postData;
    QByteArray rawHeaders;
    getStandardArgs(args, url, postData, rawHeaders);

    auto reply = m_net->start(url, postData, rawHeaders);
    const auto id = QMPlay2Core.getCommonJS()->insertNetworkReply(reply);

    connect(reply, &NetworkReply::finished,
            this, [=]() mutable {
        if (onFinished.isCallable())
        {
            onFinished.call({
                static_cast<int>(reply->error()),
                QString(reply->readAll()),
                QString(reply->getCookies()),
                id,
            });
        }
        reply->deleteLater();
    });
    if (onProgress.isCallable())
    {
        connect(reply, &NetworkReply::downloadProgress,
                this, [=](int pos, int total) mutable {
            onProgress.call({pos, total});
        });
    }

    return id;
}
QVariantMap NetworkAccessJS::startAndWait(QJSValue args, const int ioControllerId)
{
    if (auto ioCtrl = QMPlay2Core.getCommonJS()->getIOController(ioControllerId))
    {
        QString url;
        QByteArray postData;
        QByteArray rawHeaders;
        int retries;
        getStandardArgs(args, url, postData, rawHeaders, &retries);

        auto &reply = ioCtrl->toRef<NetworkReply>();
        if (m_net->startAndWait(reply, url, postData, rawHeaders, retries))
        {
            return {
                {"ok", true},
                {"reply", QString(reply->readAll())},
                {"cookies", QString(reply->getCookies())},
            };
        }
        reply.reset();
    }
    return {
        {"ok", false}
    };
}
