#pragma once

#include <QMPlay2Lib.hpp>

#include <IOController.hpp>

#include <QVariantMap>
#include <QJSValue>
#include <QObject>
#include <QMutex>
#include <QHash>

class TreeWidgetItemJS;
class NetworkAccessJS;
class NetworkReply;
class QTimer;

class QMPLAY2SHAREDLIB_EXPORT CommonJS : public QObject
{
    Q_OBJECT

public:
    CommonJS(QObject *parent = nullptr);
    ~CommonJS();

    int insertJSEngine(QJSEngine *engine);
    QJSEngine *getEngine(const int id);

    int insertNetworkReply(NetworkReply *networkReply);
    NetworkReply *getNetworkReply(const int id) const;
    void removeNetworkReply(const int id);

    int insertIOController(IOController<> *ioCtrl);
    IOController<> *getIOController(const int id) const;
    void removeIOController(const int id);

public: // QJSEngine::newQMetaObject() is available since Qt 5.8, so use these methods for Qt 5.6 compatibility
    Q_INVOKABLE QJSValue newNetworkAccess(int engineId);
    Q_INVOKABLE QJSValue newQTreeWidgetItem(int engineId);

public:
    Q_INVOKABLE bool abortNetworkReply(const int id);

    Q_INVOKABLE bool isIOControllerAborted(const int id);

    Q_INVOKABLE int startTimer(const int ms, const bool singleShot, QJSValue onTimeout);
    Q_INVOKABLE void stopTimer(const int id);

    Q_INVOKABLE QString fromHtml(const QString &html);

    Q_INVOKABLE QByteArray toBase64(const QByteArray &data, const int options = 0);
    Q_INVOKABLE QByteArray fromBase64(const QByteArray &data);

    Q_INVOKABLE QByteArray toHex(const QByteArray &data);
    Q_INVOKABLE QByteArray fromHex(const QByteArray &data);

    Q_INVOKABLE QByteArray decryptAes256Cbc(const QByteArray &password, const QByteArray &salt, const QByteArray &ciphered);

    Q_INVOKABLE QVariantMap youTubeDlFixUrl(const QString &url, const int ioControllerId, const bool nameAvail, const bool extensionAvail, const bool errorAvail);

    Q_INVOKABLE QString timeToStr(const double t, const bool decimals = false);

    Q_INVOKABLE void sendMessage(const QString &msg, const QString &title = QString(), int messageIcon = 1, int ms = 2000);

    Q_INVOKABLE void addCookies(const QString &url, const QByteArray &newCookies, const bool removeAfterUse = true);
    Q_INVOKABLE QByteArray getCookies(const QString &url);

    Q_INVOKABLE void addRawHeaders(const QString &url, const QByteArray &data, const bool removeAfterUse = true);
    Q_INVOKABLE QByteArray getRawheaders(const QString &url);

    Q_INVOKABLE void addNameForUrl(const QString &url, const QString &name, const bool removeAfterUse = true);
    Q_INVOKABLE  QString getNameForUrl(const QString &url);

private:
    mutable QMutex m_jsEngineMutex;
    int m_jsEngineId = 0;
    QHash<int, QJSEngine *> m_jsEngines;

    mutable QMutex m_networkReplyMutex;
    int m_networkReplyId = 0;
    QHash<int, NetworkReply *> m_networkReplies;

    mutable QMutex m_ioControllerMutex;
    int m_ioControllerId = 0;
    QHash<int, IOController<> *> m_ioControllers;

    QMutex m_timerMutex;
    int m_timerId = 0;
    QHash<int, QTimer *> m_timers;
};
