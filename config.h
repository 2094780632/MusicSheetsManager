#pragma once
#include <QObject>
#include <QSettings>
#include <QAtomicPointer>
#include <QMutex>

#include <QStandardPaths>
#include <QDir>
#include <QRegularExpression>

//全局单例Config设置

class AppConfig : public QObject
{
    Q_OBJECT
public:
    static AppConfig* instance(const QString &iniPath = QString());         // 单例入口
    void load();                                                            // 读设置
    void save();                                                            // 存设置

    // 设置属性字段
    QString version;
    QString userName;
    QString storagePath;

    //读
    QString getVersion()   const { return version; }
    QString getUserName()  const { return userName; }
    QString getStoragePath()const { return storagePath; }

    //写
    void setVersion(const QString &v)   { version = v; }
    void setUserName(const QString &n)  { userName = n; }
    void setStoragePath(const QString &p){ storagePath = p; }


private:
    explicit AppConfig(const QString &iniPath, QObject *parent = nullptr);
    Q_DISABLE_COPY(AppConfig)

    static QAtomicPointer<AppConfig> m_instance;
    static QMutex                    m_mutex;
    QString                          m_iniPath;
};
