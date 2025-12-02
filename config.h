#pragma once
#include <QObject>
#include <QSettings>
#include <QAtomicPointer>
#include <QMutex>

//全局单例Config设置

class AppConfig : public QObject
{
    Q_OBJECT
public:
    static AppConfig* instance(const QString &iniPath = QString());          // 单例入口
    void load();                           // 读设置
    void save();                           // 存设置

    // 设置属性字段
    QString version;
    QString userName;
    QString storagePath;

private:
    explicit AppConfig(const QString &iniPath, QObject *parent = nullptr);
    Q_DISABLE_COPY(AppConfig)

    static QAtomicPointer<AppConfig> m_instance;
    static QMutex                    m_mutex;
    QString                          m_iniPath;
};
