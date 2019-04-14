#include <MediaBrowserJS.hpp>

#include <NetworkAccessJS.hpp>
#include <TreeWidgetJS.hpp>
#include <CommonJS.hpp>

#include <QMPlay2Core.hpp>

#include <QRegularExpression>
#include <QCoreApplication>
#include <QLoggingCategory>
#include <QMimeDatabase>
#include <QHeaderView>
#include <QTreeWidget>
#include <QJSEngine>
#include <QFileInfo>
#include <QMetaEnum>
#include <QAction>
#include <QDir>

Q_DECLARE_LOGGING_CATEGORY(mb)

MediaBrowserJS::MediaBrowserJS(const QString &commonCode, const int lineNumber, const QString &scriptPath, NetworkAccess &net, QTreeWidget *treeW, QObject *parent)
    : QObject(parent)
    , m_engine(*(new QJSEngine(this)))
    , m_commonJS(*QMPlay2Core.getCommonJS())
    , m_treeW(treeW)
    , m_network(m_engine.newQObject(new NetworkAccessJS(net, this)))
    , m_treeWidgetJS(m_engine.newQObject(new TreeWidgetJS(m_treeW, this)))
{
    m_engine.installExtensions(QJSEngine::ConsoleExtension);

    auto globalObject = m_engine.globalObject();
    globalObject.setProperty(
        "engine",
        m_commonJS.insertJSEngine(&m_engine)
    );
    globalObject.setProperty(
        "common",
        m_engine.newQObject(&m_commonJS)
    );
    globalObject.setProperty(
        "self",
        m_engine.newQObject(this)
    );

    QFile scriptFile(scriptPath);
    if (scriptFile.open(QFile::ReadOnly))
    {
        m_script = m_engine.evaluate(commonCode.arg(scriptFile.readAll().constData()), QFileInfo(scriptPath).fileName(), lineNumber);
        if (m_script.isError())
        {
            qCWarning(mb).nospace().noquote()
                << m_script.property("fileName").toString()
                << ":"
                << m_script.property("lineNumber").toInt()
                << " "
                << m_script.toString()
            ;
            return;
        }
    }

    const auto map = callJSFunction("getInfo").toVariant().toMap();

    m_name = map["name"].toString();
    if (m_name.simplified().isEmpty())
        return;

    m_version = map["version"].toInt();
    if (m_version < 1)
        return;

    const auto icon = map["icon"].toString();
    if (QFileInfo(icon).isFile())
    {
        m_icon = QIcon(icon);
    }
    else
    {
        const auto iconData = QByteArray::fromBase64(icon.toLatin1());
        if (QMimeDatabase().mimeTypeForData(iconData).name() == "application/gzip")
        {
            m_iconFile.setFileName(QString("%1/QMPlay2.MediaBrowserIcon.%2.%3.svgz").arg(QDir::tempPath(), name()).arg(QCoreApplication::applicationPid()));
            if (m_iconFile.open(QFile::WriteOnly))
            {
                m_iconFile.write(iconData);
                m_iconFile.close();
                m_icon = QIcon(m_iconFile.fileName());
            }
        }
    }
}
MediaBrowserJS::~MediaBrowserJS()
{
    finalize();
    if (!m_iconFile.fileName().isEmpty())
        m_iconFile.remove();
}

int MediaBrowserJS::version() const
{
    return m_version;
}

QString MediaBrowserJS::name() const
{
    return m_name;
}
QIcon MediaBrowserJS::icon() const
{
    return m_icon;
}

void MediaBrowserJS::prepareWidget()
{
    m_treeW->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeW->headerItem()->setHidden(false);
    m_treeW->setSortingEnabled(true);
    m_treeW->setIconSize({22, 22});
    m_treeW->setIndentation(0);

    m_treeW->setColumnCount(1);
    m_treeW->header()->setStretchLastSection(false);
    m_treeW->header()->setSectionResizeMode(0, QHeaderView::Stretch);

    callJS(Q_FUNC_INFO, {m_treeWidgetJS});
}

void MediaBrowserJS::finalize()
{
    callJS(Q_FUNC_INFO);
}

QString MediaBrowserJS::getQMPlay2Url(const QString &text) const
{
    return toString(callJS(Q_FUNC_INFO, {text}));
}

NetworkReply *MediaBrowserJS::getSearchReply(const QString &text, const qint32 page)
{
    return toNetworkReply(callJS(Q_FUNC_INFO, {text, page}));
}
MediaBrowserJS::Description MediaBrowserJS::addSearchResults(const QByteArray &reply)
{
    const auto map = callJS(Q_FUNC_INFO, {QString(reply)}).toVariant().toMap();

    const int count = m_treeW->topLevelItemCount();
    for (int i = 0; i < count; ++i)
        m_treeW->topLevelItem(i)->setIcon(0, icon());

    const auto description = map["description"].toString();
    const auto imageReply = m_commonJS.getNetworkReply(map["imageReply"].toInt());
    const auto nextReply = m_commonJS.getNetworkReply(map["nextReply"].toInt());

    if (!description.isEmpty() && imageReply && !nextReply)
    {
        return {
            description,
            imageReply
        };
    }

    if (description.isEmpty() && !imageReply && nextReply)
    {
        return {
            nextReply
        };
    }

    return {};
}

MediaBrowserJS::PagesMode MediaBrowserJS::pagesMode() const
{
    return toEnum<PagesMode>(callJS(Q_FUNC_INFO));
}
QStringList MediaBrowserJS::getPagesList() const
{
    return toStringList(callJS(Q_FUNC_INFO));
}

bool MediaBrowserJS::hasWebpage() const
{
    return toBool(callJS(Q_FUNC_INFO));
}
QString MediaBrowserJS::getWebpageUrl(const QString &text) const
{
    return toString(callJS(Q_FUNC_INFO, {text}));
}

MediaBrowserJS::CompleterMode MediaBrowserJS::completerMode() const
{
    return toEnum<CompleterMode>(callJS(Q_FUNC_INFO));
}
NetworkReply *MediaBrowserJS::getCompleterReply(const QString &text)
{
    return toNetworkReply(callJS(Q_FUNC_INFO, {text}));
}
QStringList MediaBrowserJS::getCompletions(const QByteArray &reply)
{
    return toStringList(callJS(Q_FUNC_INFO, {QString(reply)}));
}

void MediaBrowserJS::setCompleterListCallback(const CompleterReadyCallback &callback)
{
    m_completerListCallback = callback;
    callJSFunction("completerListCallbackSet");
}

QMPlay2Extensions::AddressPrefix MediaBrowserJS::addressPrefix(bool img) const
{
    return {
        name(),
        img ? icon() : QIcon(),
    };
}

bool MediaBrowserJS::hasAction() const
{
    return toBool(callJS(Q_FUNC_INFO));
}

bool MediaBrowserJS::convertAddress(const QString &prefix,
                                    const QString &url,
                                    const QString &param,
                                    QString *streamUrl,
                                    QString *name,
                                    QIcon *icon,
                                    QString *extension,
                                    IOController<> *ioCtrl)
{
    if (prefix != this->name())
        return false;

    if (icon)
        *icon = this->icon();

    if (!streamUrl)
        return false;

    const int ioCtrlId = m_commonJS.insertIOController(ioCtrl);
    if (!ioCtrlId)
        return false;

    m_mutex.lock();
    const auto map = callJS(Q_FUNC_INFO, {prefix, url, param, !!name, !!extension, ioCtrlId}).toVariant().toMap();
    m_mutex.unlock();

    m_commonJS.removeIOController(ioCtrlId);
    ioCtrl->reset();

    if (!ioCtrl->isAborted())
    {
        if (streamUrl)
        {
            const auto value = map["url"].toString();
            if (!value.isNull())
                *streamUrl = value;
        }
        if (name)
        {
            const auto value = map["name"].toString();
            if (!value.isNull())
                *name = value;
        }
        if (extension)
        {
            const auto value = map["extension"].toString();
            if (!value.isNull())
                *extension = value;
        }
    }

    return true;
}

QJSValue MediaBrowserJS::network() const
{
    return m_network;
}

bool MediaBrowserJS::hasCompleterListCallback() const
{
    return !!m_completerListCallback;
}
void MediaBrowserJS::resetCompleterListCallback()
{
    m_completerListCallback = nullptr;
}
void MediaBrowserJS::completerListCallback()
{
    if (m_completerListCallback)
        m_completerListCallback();
}

QJSValue MediaBrowserJS::callJS(const char *const funcInfo, const QJSValueList &args) const
{
    return callJSFunction(
        QRegularExpression(QString(R"(\ %1::(.+)\()").arg(metaObject()->className())).match(funcInfo).captured(1),
        args
    );
}
QJSValue MediaBrowserJS::callJSFunction(const QString &funcName, const QJSValueList &args) const
{
    const auto value = m_script.property(funcName).call(args);
    if (value.isError())
    {
        qCCritical(mb).nospace().noquote()
            << value.property("fileName").toString()
            << ":"
            << value.property("lineNumber").toInt()
            << " "
            << value.toString()
        ;
        return QJSValue();
    }
    return value;
}

bool MediaBrowserJS::toBool(const QJSValue &value) const
{
    if (value.isBool())
        return value.toBool();
    return false;
}
bool MediaBrowserJS::toInt(const QJSValue &value) const
{
    if (value.isNumber())
        return value.toInt();
    return false;
}
QString MediaBrowserJS::toString(const QJSValue &value) const
{
    if (value.isString())
        return value.toString();
    return QString();
}
QStringList MediaBrowserJS::toStringList(const QJSValue &value) const
{
    QStringList stringList;

    const auto list = value.toVariant().toList();
    for (auto &&item : list)
    {
        if (item.isValid())
            stringList.push_back(item.toString());
    }

    return stringList;
}
NetworkReply *MediaBrowserJS::toNetworkReply(const QJSValue &value) const
{
    if (value.isNumber())
        return m_commonJS.getNetworkReply(value.toInt());
    return nullptr;
}
template<typename E>
E MediaBrowserJS::toEnum(const QJSValue &value) const
{
    const QMetaEnum values = QMetaEnum::fromType<E>();
    if (value.isNumber())
    {
        const int count = values.keyCount();
        const int e = value.toInt();
        for (int i = 0; i < count; ++i)
        {
            if (e == values.value(i))
                return static_cast<E>(e);
        }
    }
    return static_cast<E>(values.value(0));
}
