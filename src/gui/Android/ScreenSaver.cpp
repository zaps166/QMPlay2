#include <ScreenSaver.hpp>

#include <QCoreApplication>
#include <QJniObject>

class ScreenSaverPriv
{
public:
    inline ScreenSaverPriv()
    {
        QJniObject activity = QNativeInterface::QAndroidApplication::context();
        if (activity.isValid())
        {
            QJniObject serviceName = QJniObject::getStaticObjectField<jstring>("android/content/Context", "POWER_SERVICE");
            if (serviceName.isValid())
            {
                QJniObject powerMgr = activity.callObjectMethod("getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;", serviceName.object<jobject>());
                if (powerMgr.isValid())
                {
                    jint levelAndFlags = QJniObject::getStaticField<jint>("android/os/PowerManager", "SCREEN_BRIGHT_WAKE_LOCK");
                    QJniObject tag = QJniObject::fromString(QCoreApplication::applicationName());
                    m_wakeLock = powerMgr.callObjectMethod("newWakeLock", "(ILjava/lang/String;)Landroid/os/PowerManager$WakeLock;", levelAndFlags,tag.object<jstring>());
                }
            }
        }
    }
    inline ~ScreenSaverPriv()
    {}

    inline bool isLoaded() const
    {
        return m_wakeLock.isValid();
    }

    inline void inhibit()
    {
        m_wakeLock.callMethod<void>("acquire", "()V");
    }
    inline void unInhibit()
    {
        m_wakeLock.callMethod<void>("release", "()V");
    }

private:
    QJniObject m_wakeLock;
};

/**/

ScreenSaver::ScreenSaver() :
    m_priv(new ScreenSaverPriv)
{}
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
