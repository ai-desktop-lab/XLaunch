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
#include <QScreen>
#include <QQuickStyle>
#include <QSettings>
int main(int argc,char **argv){
 QQuickStyle::setStyle("Basic");
 QApplication app(argc,argv); app.setApplicationName("XLaunch"); app.setOrganizationName("AI Workspace Lab");
 Catalog catalog;
 if(app.arguments().contains("--self-test")){
   int total=catalog.rowCount(); catalog.setQuery("___missing_application___");
   if(catalog.rowCount()!=0||catalog.launch(-1))return 1;
   catalog.setQuery("");if(catalog.rowCount()!=total)return 2;
   int paged=0;for(int start=0;start<total;start+=35)paged+=catalog.page(start,35).size();
   if(paged!=total||!catalog.page(total+1,35).isEmpty())return 4;
   qInfo()<<"Catalog filtering and bounds passed; applications:"<<total;return total>0?0:3;
 }
 QQmlApplicationEngine engine; engine.rootContext()->setContextProperty("catalog",&catalog);engine.addImageProvider("apps",new Icons(&catalog));
 engine.loadFromModule("XLaunch","Main");if(engine.rootObjects().isEmpty())return 1;
 auto window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());
 window->setProperty("appLaunchMode",app.arguments().contains("--fullscreen")||app.arguments().contains("--screenshot"));
 // Keep the compact Menu attached to the lower-left corner of the usable
 // work area. IceWM/XDock publish the Dock strut, so availableGeometry ends
 // exactly at the top edge of the Dock while full-screen mode remains global.
 const auto positionCompactMenu = [window] {
   if (window->property("appLaunchMode").toBool()) return;
   auto *screen = window->screen();
   if (!screen) return;
   const QRect area = screen->availableGeometry();
   window->setPosition(area.left(), area.bottom() - window->height() + 1);
 };
 QTimer::singleShot(0, window, positionCompactMenu);
 QObject::connect(window,&QWindow::visibilityChanged,[window,positionCompactMenu](QWindow::Visibility){
   QTimer::singleShot(0, window, positionCompactMenu);
 });
 QObject::connect(window,&QWindow::heightChanged,[window,positionCompactMenu](int){
   QTimer::singleShot(0, window, positionCompactMenu);
 });
 QObject::connect(window,&QWindow::screenChanged,[window,positionCompactMenu](QScreen *){
   QTimer::singleShot(0, window, positionCompactMenu);
 });
 QSettings settings;
 window->setProperty("systemTheme",app.arguments().contains("--system-theme")||settings.value("theme/system",false).toBool());
 QObject::connect(&app,&QGuiApplication::applicationStateChanged,[&](Qt::ApplicationState state){if(state==Qt::ApplicationActive && !window->isVisible()){window->show();window->raise();window->requestActivate();}});
 QMenu menu; menu.addAction("打开 XLaunch",[&]{window->show();window->raise();window->requestActivate();});menu.addAction("刷新应用",&catalog,&Catalog::refresh);
 auto themeAction=menu.addAction("跟随系统主题");themeAction->setCheckable(true);themeAction->setChecked(window->property("systemTheme").toBool());
 QObject::connect(themeAction,&QAction::toggled,[&](bool checked){window->setProperty("systemTheme",checked);settings.setValue("theme/system",checked);});
 menu.addSeparator();menu.addAction("退出",&app,&QApplication::quit);
 QSystemTrayIcon tray(QIcon::fromTheme("application-x-executable",app.style()->standardIcon(QStyle::SP_ComputerIcon)));tray.setToolTip("XLaunch");tray.setContextMenu(&menu);tray.show();
 QObject::connect(&catalog,&Catalog::launched,window,&QWindow::hide);
 QObject::connect(&tray,&QSystemTrayIcon::activated,[&](auto reason){if(reason==QSystemTrayIcon::Trigger){window->show();window->raise();window->requestActivate();}});
 app.setQuitOnLastWindowClosed(false);
 if(app.arguments().contains("--screenshot")) {
   window->showFullScreen();
   QTimer::singleShot(2000,[&]{window->grabWindow().save("xlaunch-preview.png");app.quit();});
 }
 return app.exec();
}
