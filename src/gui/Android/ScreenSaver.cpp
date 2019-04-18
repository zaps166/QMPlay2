#include <ScreenSaver.hpp>

#include <QAndroidJniObject>
#include <QCoreApplication>

class ScreenSaverPriv
{
public:
    inline ScreenSaverPriv()
    {
        QAndroidJniObject activity = QAndroidJniObject::callStaticObjectMethod("org/qtproject/qt5/android/QtNative", "activity", "()Landroid/app/Activity;");
        if (activity.isValid())
        {
            QAndroidJniObject serviceName = QAndroidJniObject::getStaticObjectField<jstring>("android/content/Context", "POWER_SERVICE");
            if (serviceName.isValid())
            {
                QAndroidJniObject powerMgr = activity.callObjectMethod("getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;", serviceName.object<jobject>());
                if (powerMgr.isValid())
                {
                    jint levelAndFlags = QAndroidJniObject::getStaticField<jint>("android/os/PowerManager", "SCREEN_BRIGHT_WAKE_LOCK");
                    QAndroidJniObject tag = QAndroidJniObject::fromString(QCoreApplication::applicationName());
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
    QAndroidJniObject m_wakeLock;
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
