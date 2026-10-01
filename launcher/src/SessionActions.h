#pragma once
#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QSet>
#include <QHash>
#include <QUrl>
#include <functional>
class SessionActions : public QObject {
 Q_OBJECT
 Q_PROPERTY(QString userName READ userName CONSTANT)
 Q_PROPERTY(QString displayName READ displayName CONSTANT)
 Q_PROPERTY(QUrl avatar READ avatar CONSTANT)
 Q_PROPERTY(QString sessionLabel READ sessionLabel NOTIFY changed)
 Q_PROPERTY(QVariantList actions READ actions NOTIFY changed)
 Q_PROPERTY(bool busy READ busy NOTIFY changed)
public:
 explicit SessionActions(bool preview=false,QObject *parent=nullptr);
 QString userName() const{return m_user;}
 QString displayName() const{return m_name;}
 QUrl avatar() const{return m_avatar;}
 QString sessionLabel() const{return m_sessionLabel;}
 QVariantList actions() const;
 bool busy() const{return m_busy;}
 Q_INVOKABLE void refresh();
 Q_INVOKABLE QVariantMap action(const QString &key) const;
 Q_INVOKABLE void request(const QString &key);
 Q_INVOKABLE void confirm(const QString &key);
 Q_INVOKABLE void cancel(){m_pending.clear();}
signals:
 void changed();
 void confirmationRequested(const QString &key,const QString &title,const QString &detail);
 void failure(const QString &message);
 void succeeded(const QString &key);
private:
 QString m_user,m_name,m_sessionLabel,m_sid,m_seat,m_dmSeat,m_pending;
 QUrl m_avatar;
 bool m_preview=false,m_busy=false,m_kde=false,m_icewm=false,m_validSession=false,m_remote=true;
 int m_generation=0;
 QSet<QString> m_services;
 QHash<QString,QString> m_power;
 void updateDisplayManager(int generation);
 void execute(const QString &key);
 void complete(const QString &key,const QString &error={});
 void call(bool system,const QString &service,const QString &path,const QString &interface,const QString &method,const QVariantList &args,std::function<void(const QVariantList&,const QString&)> done);
};
