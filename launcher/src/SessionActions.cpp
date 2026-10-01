#include "SessionActions.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QSysInfo>
#ifdef Q_OS_UNIX
#include <pwd.h>
#include <unistd.h>
#endif
#ifdef SESSION_ACTIONS_DBUS
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusObjectPath>
#include <QDBusArgument>
#include <QDBusVariant>
#endif
namespace {
QString executable(const char *name){return QStandardPaths::findExecutable(QString::fromLatin1(name));}
QString displayId(QString value){return value.section('.',0,0);}
}
SessionActions::SessionActions(bool preview,QObject *parent):QObject(parent),m_preview(preview){
 m_user=qEnvironmentVariable("USER",qEnvironmentVariable("USERNAME"));m_name=m_user;
#ifdef Q_OS_UNIX
 if(auto *user=getpwuid(getuid())){m_user=QString::fromLocal8Bit(user->pw_name);m_name=QString::fromLocal8Bit(user->pw_gecos).section(',',0,0);}
#endif
 if(m_name.isEmpty())m_name=m_user;
 for(const auto &path:{QDir::homePath()+"/.face",QDir::homePath()+"/.face.icon",QString("/var/lib/AccountsService/icons/")+m_user})
  if(QFileInfo(path).isReadable()){m_avatar=QUrl::fromLocalFile(path);break;}
 const auto desktop=qEnvironmentVariable("XDG_CURRENT_DESKTOP").toUpper();m_kde=desktop.contains("KDE");m_icewm=desktop.contains("ICEWM");
 m_sessionLabel=QSysInfo::machineHostName()+" · "+(desktop.isEmpty()?QStringLiteral("桌面会话"):desktop);
 refresh();
}
QVariantList SessionActions::actions() const {
 const bool kde=m_kde&&m_services.contains("org.kde.ksmserver")&&m_services.contains("org.kde.Shutdown");
 const bool lock=m_services.contains("org.freedesktop.ScreenSaver")||!executable("xsecurelock").isEmpty();
 const bool logout=kde||(m_icewm&&!executable("icesh").isEmpty())||m_validSession;
 const bool login=m_remote?logout:!m_dmSeat.isEmpty()&&m_services.contains("org.freedesktop.ScreenSaver");
 QVariantList result;
 const auto add=[&](QString key,QString text,bool available,bool destructive,QString detail,QString reason){
  result.append(QVariantMap{{"key",key},{"text",text},{"enabled",available&&!m_preview&&!m_busy},{"destructive",destructive},{"detail",detail},{"reason",m_preview?QStringLiteral("预览中不执行会话操作"):m_busy?QStringLiteral("正在处理…"):available?QString():reason}});
 };
 add("lock","锁定屏幕",lock,false,"","当前会话没有可用的锁屏服务，请安装并配置 xsecurelock。");
 add("login","登录 / 切换用户",login,m_remote,m_remote?"将注销当前远程桌面。请在远程桌面客户端使用所需账号重新连接。":"保留当前桌面，锁屏后进入登录界面。","当前会话没有独立登录界面或锁屏服务。");
 add("logout","注销",logout,true,"将退出当前桌面，请先保存正在编辑的内容。远程连接可能随之断开。","无法确定当前图形会话，注销不可用。");
 for(const auto &key:{QStringLiteral("reboot"),QStringLiteral("poweroff")}){
  const auto permission=m_power.value(key);const bool allowed=permission=="yes"||permission=="challenge";
  add(key,key=="reboot"?"重启主机":"关闭主机",allowed,true,
      key=="reboot"?"将重启整台主机，并断开所有远程会话。请先保存工作。":"将关闭整台主机，并断开所有远程会话。请先保存工作。",
      permission=="no"?"当前用户没有执行此操作的权限。":"当前环境未提供可用的电源管理服务。");
 }
 return result;
}
QVariantMap SessionActions::action(const QString &key) const{for(const auto &item:actions()){const auto map=item.toMap();if(map.value("key")==key)return map;}return {};}
void SessionActions::call(bool system,const QString &service,const QString &path,const QString &interface,const QString &method,const QVariantList &args,std::function<void(const QVariantList&,const QString&)> done){
#ifdef SESSION_ACTIONS_DBUS
 auto message=QDBusMessage::createMethodCall(service,path,interface,method);message.setArguments(args);
 auto *watcher=new QDBusPendingCallWatcher((system?QDBusConnection::systemBus():QDBusConnection::sessionBus()).asyncCall(message,5000),this);
 connect(watcher,&QDBusPendingCallWatcher::finished,this,[watcher,done]{auto reply=watcher->reply();watcher->deleteLater();done(reply.arguments(),reply.type()==QDBusMessage::ErrorMessage?reply.errorMessage():QString());});
#else
 Q_UNUSED(system);Q_UNUSED(service);Q_UNUSED(path);Q_UNUSED(interface);Q_UNUSED(method);Q_UNUSED(args);done({},"当前平台尚未提供此会话操作。");
#endif
}
void SessionActions::refresh(){
 if(m_busy)return;
 const int generation=++m_generation;m_services.clear();m_power.clear();m_validSession=false;m_dmSeat.clear();
#ifdef SESSION_ACTIONS_DBUS
 for(const auto &method:{QStringLiteral("ListNames"),QStringLiteral("ListActivatableNames")})
  call(false,"org.freedesktop.DBus","/org/freedesktop/DBus","org.freedesktop.DBus",method,{},[this,generation](const QVariantList &values,const QString &error){
   if(generation!=m_generation||!error.isEmpty()||values.isEmpty())return;
   for(const auto &name:qdbus_cast<QStringList>(values.first()))m_services.insert(name);emit changed();
  });
 for(const auto &key:{QStringLiteral("reboot"),QStringLiteral("poweroff")})
  call(true,"org.freedesktop.login1","/org/freedesktop/login1","org.freedesktop.login1.Manager",key=="reboot"?"CanReboot":"CanPowerOff",{},[this,generation,key](const QVariantList &values,const QString &error){
   if(generation!=m_generation)return;m_power[key]=error.isEmpty()&&!values.isEmpty()?values.first().toString():QString();emit changed();
  });
 const auto sid=qEnvironmentVariable("XDG_SESSION_ID");
 // Only the caller's validated graphical session may be terminated. Never use TerminateUser.
 if(!sid.isEmpty())call(true,"org.freedesktop.login1","/org/freedesktop/login1","org.freedesktop.login1.Manager","GetSession",{sid},[this,generation,sid](const QVariantList &values,const QString &error){
  if(generation!=m_generation||!error.isEmpty()||values.isEmpty())return;
  const auto path=qdbus_cast<QDBusObjectPath>(values.first()).path();
  call(true,"org.freedesktop.login1",path,"org.freedesktop.DBus.Properties","GetAll",{"org.freedesktop.login1.Session"},[this,generation,sid](const QVariantList &values,const QString &error){
   if(generation!=m_generation||!error.isEmpty()||values.isEmpty())return;
   const auto properties=qdbus_cast<QVariantMap>(values.first());const auto type=properties.value("Type").toString();
   const auto user=qvariant_cast<QDBusArgument>(properties.value("User"));quint32 uid=0;QDBusObjectPath userPath;user.beginStructure();user>>uid>>userPath;user.endStructure();
   const auto display=properties.value("Display").toString();
   m_validSession=uid==quint32(getuid())&&(type=="x11"||type=="wayland")&&
    (qEnvironmentVariable("DISPLAY").isEmpty()?type=="wayland":displayId(display)==displayId(qEnvironmentVariable("DISPLAY")));
   if(!m_validSession){emit changed();return;}
   m_sid=sid;m_remote=properties.value("Remote").toBool();
   const auto seat=qvariant_cast<QDBusArgument>(properties.value("Seat"));QDBusObjectPath seatPath;seat.beginStructure();seat>>m_seat>>seatPath;seat.endStructure();
   m_sessionLabel=QSysInfo::machineHostName()+" · "+(m_remote?"远程桌面":"本地桌面")+" · "+qEnvironmentVariable("XDG_CURRENT_DESKTOP");
   if(!m_remote&&!m_seat.isEmpty())updateDisplayManager(generation);emit changed();
  });
 });
#endif
 emit changed();
}
void SessionActions::updateDisplayManager(int generation){
#ifdef SESSION_ACTIONS_DBUS
 call(true,"org.freedesktop.DisplayManager","/org/freedesktop/DisplayManager","org.freedesktop.DBus.Properties","Get",{"org.freedesktop.DisplayManager","Seats"},[this,generation](const QVariantList &values,const QString &error){
  if(generation!=m_generation||!error.isEmpty()||values.isEmpty())return;
  const auto paths=qdbus_cast<QList<QDBusObjectPath>>(qdbus_cast<QDBusVariant>(values.first()).variant());
  const auto preferred=qEnvironmentVariable("XDG_SEAT_PATH");QString path;
  for(const auto &candidate:paths)if(candidate.path()==preferred)path=preferred;
  if(path.isEmpty()&&paths.size()==1)path=paths.first().path();if(path.isEmpty())return;
  call(true,"org.freedesktop.DisplayManager",path,"org.freedesktop.DBus.Properties","Get",{"org.freedesktop.DisplayManager.Seat","CanSwitch"},[this,generation,path](const QVariantList &values,const QString &error){
   if(generation==m_generation&&error.isEmpty()&&!values.isEmpty()&&qdbus_cast<QDBusVariant>(values.first()).variant().toBool()){m_dmSeat=path;emit changed();}
  });
 });
#else
 Q_UNUSED(generation);
#endif
}
void SessionActions::request(const QString &key){
 const auto item=action(key);if(!item.value("enabled").toBool()){emit failure(item.value("reason","此操作不可用。").toString());return;}
 if(item.value("destructive").toBool()){m_pending=key;emit confirmationRequested(key,item.value("text").toString(),item.value("detail").toString());}else execute(key);
}
void SessionActions::confirm(const QString &key){
 if(key!=m_pending||m_busy)return;m_pending.clear();const auto item=action(key);
 if(!item.value("enabled").toBool()){emit failure("会话状态已改变，请重新打开菜单。");return;}execute(key);
}
void SessionActions::complete(const QString &key,const QString &error){m_busy=false;emit changed();if(error.isEmpty())emit succeeded(key);else emit failure("操作未完成："+error);}
void SessionActions::execute(const QString &key){
 m_busy=true;emit changed();
 const auto done=[this,key](const QVariantList &,const QString &error){complete(key,error);};
 const auto command=[this,key](const QString &program,const QStringList &args){
  auto *process=new QProcess(this);process->setProgram(program);process->setArguments(args);
  connect(process,&QProcess::errorOccurred,this,[this,process,key](QProcess::ProcessError){complete(key,process->errorString());process->deleteLater();});
  connect(process,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this,process,key](int code,QProcess::ExitStatus status){complete(key,code==0&&status==QProcess::NormalExit?QString():QString("命令退出失败（%1）：%2").arg(code).arg(QString::fromUtf8(process->readAllStandardError()).trimmed().left(300)));process->deleteLater();});process->start();
 };
 if(key=="lock"){
  if(m_services.contains("org.freedesktop.ScreenSaver"))call(false,"org.freedesktop.ScreenSaver","/ScreenSaver","org.freedesktop.ScreenSaver","Lock",{},done);
  else command(executable("xsecurelock"),{});return;
 }
 if(key=="login"&&!m_remote){
  if(!m_services.contains("org.freedesktop.ScreenSaver")){complete(key,"请先手动锁定屏幕，再切换用户。");return;}
  call(false,"org.freedesktop.ScreenSaver","/ScreenSaver","org.freedesktop.ScreenSaver","Lock",{},[this,key](const QVariantList &,const QString &error){
   if(!error.isEmpty()){complete(key,error);return;}
   call(true,"org.freedesktop.DisplayManager",m_dmSeat,"org.freedesktop.DisplayManager.Seat","SwitchToGreeter",{},[this,key](const QVariantList &,const QString &error){complete(key,error);});
  });return;
 }
 if(m_kde&&m_services.contains("org.kde.ksmserver")&&m_services.contains("org.kde.Shutdown")){
  const QString method=key=="reboot"?"logoutAndReboot":key=="poweroff"?"logoutAndShutdown":"logout";
  call(false,"org.kde.Shutdown","/Shutdown","org.kde.Shutdown",method,{},done);return;
 }
 if(key=="logout"||key=="login"){
  if(m_icewm&&!executable("icesh").isEmpty()){command(executable("icesh"),{"logout"});return;}
  if(m_validSession){call(true,"org.freedesktop.login1","/org/freedesktop/login1","org.freedesktop.login1.Manager","TerminateSession",{m_sid},done);return;}
 }
 if(key=="reboot"||key=="poweroff")call(true,"org.freedesktop.login1","/org/freedesktop/login1","org.freedesktop.login1.Manager",key=="reboot"?"Reboot":"PowerOff",{true},done);
 else complete(key,"当前环境不支持此操作。");
}
