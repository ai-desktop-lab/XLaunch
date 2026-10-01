#include "catalog.h"
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <QUrl>
#include <QRegularExpression>
#include <QFutureWatcher>
#include <QtConcurrent>

namespace {
constexpr auto xdockLaunchEventServer = "ai-workspace-lab.xdock.launch-events";

bool notifyXDock(const Application &app, const QString &type = "launched") {
    QLocalSocket socket;
    QString session = qEnvironmentVariable("DISPLAY", qEnvironmentVariable("WAYLAND_DISPLAY", "default"));
    if (!qEnvironmentVariable("DISPLAY").isEmpty()) session = session.section('.', 0, 0);
    session.replace(QRegularExpression("[^a-zA-Z0-9_-]"), "_");
    socket.connectToServer(QString::fromLatin1(xdockLaunchEventServer) + "." + session);
    if (!socket.waitForConnected(150)) return false;
    const auto encodedPath = QString::fromLatin1(QUrl::toPercentEncoding(app.path));
    const QJsonObject event{{"type", type}, {"name", app.name},
                            {"launchId", app.path}, {"icon", "path:" + encodedPath}};
    socket.write(QJsonDocument(event).toJson(QJsonDocument::Compact));
    socket.write("\n");
    socket.waitForBytesWritten(150);
    if (type == "pin") {
        if (!socket.waitForReadyRead(300)) return false;
        return socket.readLine().trimmed() == "ok";
    }
    return true;
}
}

Catalog::Catalog(){refresh();}
void Catalog::refresh(){apps=discoverApplications();++m_iconRevision;filter();}
void Catalog::setQuery(QString q){if(q==m_query)return;m_query=q;filter();}
void Catalog::setCategory(QString c){if(c==m_category)return;m_category=c;filter();}
void Catalog::filter(){
 beginResetModel();visible.clear();
 for(int i=0;i<apps.size();++i) if((m_category=="全部"||apps[i].category==m_category)&&(apps[i].name.contains(m_query.trimmed(),Qt::CaseInsensitive)||QFileInfo(apps[i].path).completeBaseName().contains(m_query.trimmed(),Qt::CaseInsensitive))) visible.append(i);
 endResetModel();++generation;emit filterChanged();
}
QVariant Catalog::data(const QModelIndex &index,int role) const{
 if(!index.isValid()||index.row()<0||index.row()>=visible.size())return {};
 int i=visible[index.row()]; const auto &a=apps[i];
 if(role==Name)return a.name;if(role==Icon)return QString("image://apps/%1/%2").arg(i).arg(m_iconRevision);if(role==Path)return a.path;return {};
}
QHash<int,QByteArray> Catalog::roleNames()const{return {{Name,"appName"},{Icon,"appIcon"},{Path,"appPath"}};}
QVariantList Catalog::page(int start,int size)const{
 QVariantList result;
 for(int row=qMax(0,start);row<qMin(start+size,visible.size());++row)
   result.append(QVariantMap{{"name",data(index(row),Name)},{"icon",data(index(row),Icon)},{"row",row},{"path",data(index(row),Path)}});
 return result;
}
bool Catalog::pin(int row) {
 if (row<0 || row>=visible.size()) return false;
 bool ok=notifyXDock(apps[visible[row]], "pin");
 if (!ok) emit failure("无法驻留：请确认 XDock 正在运行，或应用已经驻留。");
 return ok;
}
bool Catalog::launch(int row){
 if(row<0||row>=visible.size()||m_launching)return false;
 const auto application=apps[visible[row]];
 m_launching=true;emit launchingChanged();
 auto *watcher=new QFutureWatcher<QString>(this);
 connect(watcher,&QFutureWatcher<QString>::finished,this,[this,watcher,application]{
   QString error=watcher->result(); watcher->deleteLater(); m_launching=false; emit launchingChanged();
   if(error.isEmpty()){notifyXDock(application);emit launched();}else emit failure(error);
 });
 watcher->setFuture(QtConcurrent::run([application]{QString error;openApplication(application.path,error);return error;}));
 return true;
}
