#include "SessionActions.h"
#include "catalog.h"
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QTimer>
#include <QDebug>
#include <QStyle>
#include <QQuickStyle>
#include <QSettings>
#include <QScreen>
#include <QLocalServer>
#include <QLocalSocket>
#include <QRegularExpression>
#ifdef XLAUNCH_X11
#include <qnativeinterface.h>
#include <xcb/xcb.h>
#include <cstring>
#include <cstdlib>
#endif
int main(int argc,char **argv){
 QQuickStyle::setStyle("Basic");
 QApplication app(argc,argv); app.setApplicationName("XLaunch"); app.setOrganizationName("AI Workspace Lab");
 app.setQuitOnLastWindowClosed(false);
 const bool inspection=app.arguments().contains("--self-test")||app.arguments().contains("--screenshot");
 QString session=qEnvironmentVariable("DISPLAY",qEnvironmentVariable("WAYLAND_DISPLAY","default"));
 if (!qEnvironmentVariable("DISPLAY").isEmpty()) session = session.section('.', 0, 0);
    session.replace(QRegularExpression("[^a-zA-Z0-9_-]"),"_");
 QString endpoint="ai-workspace-lab.xlaunch."+session;
 QLocalServer server;
 if(!inspection){
   QLocalSocket existing; existing.connectToServer(endpoint);
   if(existing.waitForConnected(150)){
     if(app.arguments().contains("--hidden"))return 0;
     existing.write(app.arguments().contains("--toggle")?"toggle\n":"show\n");existing.waitForBytesWritten(200);return 0;
   }
   server.setSocketOptions(QLocalServer::UserAccessOption);
   if(!server.listen(endpoint)) { QLocalServer::removeServer(endpoint); if(!server.listen(endpoint)) return 1; }
 }
 Catalog catalog;
 if(app.arguments().contains("--self-test")){
   int total=catalog.rowCount(); catalog.setQuery("___missing_application___");
   if(catalog.rowCount()!=0||catalog.launch(-1))return 1;
   catalog.setQuery("");if(catalog.rowCount()!=total)return 2;
   int paged=0;for(int start=0;start<total;start+=35)paged+=catalog.page(start,35).size();
   if(paged!=total||!catalog.page(total+1,35).isEmpty())return 4;
   qInfo()<<"Catalog filtering and bounds passed; applications:"<<total;return total>0?0:3;
 }
 SessionActions sessionActions(inspection);
 QQmlApplicationEngine engine; engine.rootContext()->setContextProperty("sessionActions",&sessionActions); engine.rootContext()->setContextProperty("catalog",&catalog);engine.addImageProvider("apps",new Icons(&catalog));
 engine.loadFromModule("XLaunch","Main");if(engine.rootObjects().isEmpty())return 1;
 auto window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());
 window->setProperty("appLaunchMode",app.arguments().contains("--fullscreen")||app.arguments().contains("--screenshot"));
 QSettings settings;
 window->setProperty("systemTheme",app.arguments().contains("--system-theme")||settings.value("theme/system",false).toBool());
 const auto hideFromTaskLists=[window]{
#ifdef XLAUNCH_X11
   if(QGuiApplication::platformName()!="xcb")return;
   if(auto *native=qGuiApp->nativeInterface<QNativeInterface::QX11Application>()){
     auto *connection=native->connection();
     const auto atom=[connection](const char *name){auto *r=xcb_intern_atom_reply(connection,xcb_intern_atom(connection,0,std::strlen(name),name),nullptr);auto value=r?r->atom:0;std::free(r);return value;};
     // Qt rewrites state properties during normal/fullscreen transitions. Ask the WM
     // to retain task-list exclusions after the window has been mapped.
     xcb_client_message_event_t event{};event.response_type=XCB_CLIENT_MESSAGE;event.format=32;
     event.window=window->winId();event.type=atom("_NET_WM_STATE");
     event.data.data32[0]=1;event.data.data32[1]=atom("_NET_WM_STATE_SKIP_TASKBAR");
     event.data.data32[2]=atom("_NET_WM_STATE_SKIP_PAGER");event.data.data32[3]=1;
     auto *tree=xcb_query_tree_reply(connection,xcb_query_tree(connection,window->winId()),nullptr);
     if(!tree)return;const auto root=tree->root;std::free(tree);
     xcb_send_event(connection,0,root,XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT|XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY,reinterpret_cast<const char *>(&event));xcb_flush(connection);
   }
#endif
 };
 const auto position=[window,hideFromTaskLists]{
   if(window->property("appLaunchMode").toBool()){
     const auto screen=window->screen()->geometry();
     window->showFullScreen();window->resize(screen.size());window->setPosition(screen.topLeft());
     QTimer::singleShot(0,window,hideFromTaskLists);return;
   }
   window->showNormal();
   const auto available=window->screen()->availableGeometry();
   QSize size(qMin(520,available.width()),qMin(560,available.height()));
   window->resize(size); window->setPosition(available.left(),available.bottom()-size.height()+1);
   QTimer::singleShot(0,window,hideFromTaskLists);
 };
 const auto showMenu=[window,position]{
   window->setProperty("appLaunchMode",false); position(); window->show(); window->raise();window->requestActivate();
   QMetaObject::invokeMethod(window,"focusMenu");
 };
 auto *modeTimer=new QTimer(window);modeTimer->setInterval(0);modeTimer->setSingleShot(true);
 // QML requests layout when mode changes; availableGeometry includes the dock reservation.
 QObject::connect(window,SIGNAL(layoutRequested()),modeTimer,SLOT(start()));
 QObject::connect(modeTimer,&QTimer::timeout,window,[window,position]{if(window->isVisible()){position();window->raise();window->requestActivate();}});
 QObject::connect(window->screen(),&QScreen::availableGeometryChanged,window,[window,position](const QRect &){if(window->isVisible())position();});
 QObject::connect(&server,&QLocalServer::newConnection,[&]{
   while(server.hasPendingConnections()){
     auto *socket=server.nextPendingConnection();
     QObject::connect(socket,&QLocalSocket::readyRead,window,[socket,window,showMenu]{
       auto command=socket->readAll().trimmed();
       if(command=="toggle" && window->isVisible())window->hide();else showMenu();socket->disconnectFromServer();
     });
     QObject::connect(socket,&QLocalSocket::disconnected,socket,&QObject::deleteLater);
   }
 });
 auto *hideTimer=new QTimer(window);hideTimer->setSingleShot(true);hideTimer->setInterval(250);
 QObject::connect(hideTimer,&QTimer::timeout,window,[window,&app]{if(app.applicationState()!=Qt::ApplicationActive&&!window->property("appLaunchMode").toBool()&&!window->property("sessionOverlayOpen").toBool())window->hide();});
 QObject::connect(&app,&QGuiApplication::applicationStateChanged,window,[hideTimer](Qt::ApplicationState state){if(state==Qt::ApplicationActive)hideTimer->stop();else hideTimer->start();});
 QMenu menu; menu.addAction("打开 XLaunch",showMenu);menu.addAction("刷新应用",&catalog,&Catalog::refresh);
 auto themeAction=menu.addAction("跟随系统主题");themeAction->setCheckable(true);themeAction->setChecked(window->property("systemTheme").toBool());
 QObject::connect(themeAction,&QAction::toggled,[&](bool checked){window->setProperty("systemTheme",checked);settings.setValue("theme/system",checked);});
 menu.addSeparator();menu.addAction("退出",&app,&QApplication::quit);
 QSystemTrayIcon tray(QIcon::fromTheme("application-x-executable",app.style()->standardIcon(QStyle::SP_ComputerIcon)));tray.setToolTip("XLaunch");tray.setContextMenu(&menu);tray.show();
 QObject::connect(&catalog,&Catalog::launched,window,&QWindow::hide);
 QObject::connect(&tray,&QSystemTrayIcon::activated,[&](auto reason){if(reason==QSystemTrayIcon::Trigger)showMenu();});
 app.setQuitOnLastWindowClosed(false);
 if(!app.arguments().contains("--hidden")&&!inspection) {
   if(app.arguments().contains("--fullscreen")){position();window->requestActivate();}
   else showMenu();
 }
 if(app.arguments().contains("--screenshot")) {
   window->showFullScreen();
   QTimer::singleShot(2000,[&]{window->grabWindow().save("xlaunch-preview.png");app.quit();});
 }
 return app.exec();
}
