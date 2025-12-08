#include "config.h"


QAtomicPointer<AppConfig> AppConfig::m_instance = nullptr;
QMutex                    AppConfig::m_mutex;


//单例实现
AppConfig* AppConfig::instance(const QString &iniPath)
{
    AppConfig *p = m_instance.loadAcquire();
    if (!p) {
        QMutexLocker locker(&m_mutex);
        p = m_instance.loadAcquire();
        if (!p) {
            Q_ASSERT(!iniPath.isEmpty() && "第一次调用必须给 iniPath");//断言检测path
            p = new AppConfig(iniPath);
            m_instance.storeRelease(p);
        }
    }
    return p;
}

AppConfig::AppConfig(const QString &iniPath, QObject *parent)
    : QObject(parent), m_iniPath(iniPath){}

//调用接口

void AppConfig::load()
{
    QSettings s(m_iniPath, QSettings::IniFormat);
    // 设置属性字段 与默认值设置
    version = s.value("version","1.0").toString();
    userName = s.value("userName", "Manager").toString();
    //QString s_Path = m_iniPath.left(m_iniPath.size()-11);
    storagePath = s.value("storagePath", m_iniPath.left(m_iniPath.size()-11)).toString();
}

void AppConfig::save()
{
    QSettings s(m_iniPath, QSettings::IniFormat);

    s.setValue("version", version);
    s.setValue("userName", userName);
    s.setValue("storagePath", storagePath);

    s.sync();
}
