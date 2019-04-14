#pragma once

#include <QJSValue>
#include <QObject>
#include <QMutex>
#include <QFile>

#include <QMPlay2Extensions.hpp>
#include <IOController.hpp>

#include <functional>

class NetworkAccess;
class NetworkReply;
class QTreeWidget;
class QJSEngine;
class CommonJS;
class QAction;

class MediaBrowserJS : public QObject
{
    Q_OBJECT

public:
    using CompleterReadyCallback = std::function<void()>;

    class Description
    {
    public:
        Description() = default;
        inline Description(const QString &descr, NetworkReply *reply) :
            description(descr),
            imageReply(reply)
        {}
        inline Description(NetworkReply *reply) :
            nextReply(reply)
        {}

        QString description;
        NetworkReply *imageReply = nullptr;
        NetworkReply *nextReply = nullptr;
    };

    enum class PagesMode
    {
        Single,
        Multi,
        List,
    };
    Q_ENUM(PagesMode)

    enum class CompleterMode
    {
        None,
        Continuous,
        All
    };
    Q_ENUM(CompleterMode)

public:
    MediaBrowserJS(const QString &commonCode, const int lineNumber, const QString &scriptPath, NetworkAccess &net, QTreeWidget *treeW, QObject *parent = nullptr);
    ~MediaBrowserJS();

    int version() const;

    QString name() const;
    QIcon icon() const;

    void prepareWidget();

    void finalize();

    QString getQMPlay2Url(const QString &text) const;

    NetworkReply *getSearchReply(const QString &text, const qint32 page);
    Description addSearchResults(const QByteArray &reply);

    PagesMode pagesMode() const;
    QStringList getPagesList() const;

    bool hasWebpage() const;
    QString getWebpageUrl(const QString &text) const;

    CompleterMode completerMode() const;
    NetworkReply *getCompleterReply(const QString &text);
    QStringList getCompletions(const QByteArray &reply = QByteArray());
    void setCompleterListCallback(const CompleterReadyCallback &callback);

    QMPlay2Extensions::AddressPrefix addressPrefix(bool img) const;

    bool hasAction() const;

    bool convertAddress(const QString &prefix,
                        const QString &url,
                        const QString &param,
                        QString *streamUrl,
                        QString *name,
                        QIcon *icon,
                        QString *extension,
                        IOController<> *ioCtrl);

public:
    Q_INVOKABLE QJSValue network() const;

    Q_INVOKABLE bool hasCompleterListCallback() const;
    Q_INVOKABLE void resetCompleterListCallback();
    Q_INVOKABLE void completerListCallback();

private:
    QJSValue callJS(const char * const funcInfo, const QJSValueList &args = {}) const;
    QJSValue callJSFunction(const QString &funcName, const QJSValueList &args = {}) const;

private:
    bool toBool(const QJSValue &value) const;
    bool toInt(const QJSValue &value) const;
    QString toString(const QJSValue &value) const;
    QStringList toStringList(const QJSValue &value) const;
    NetworkReply *toNetworkReply(const QJSValue &value) const;
    template<typename E>
    E toEnum(const QJSValue &value) const;

private:
    QString m_name;
    int m_version = 0;
    QIcon m_icon;
    QFile m_iconFile;

    QJSEngine &m_engine;

    CommonJS &m_commonJS;

    QTreeWidget *const m_treeW;

    QJSValue m_script;
    QJSValue m_network;
    QJSValue m_treeWidgetJS;

    QMutex m_mutex;

    CompleterReadyCallback m_completerListCallback;
};
