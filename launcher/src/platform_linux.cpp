#include "platform.h"
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QIcon>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QSet>
#include <QScreen>
#include <QGuiApplication>
#include <QApplication>
#include <QStyle>
#include <QLocale>
#include <algorithm>
#include <utility>

namespace {
bool enabledValue(const QString &value)
{
    return value.compare("true", Qt::CaseInsensitive) == 0 || value == "1";
}

bool desktopVisible(const QSettings &desktop)
{
    if (enabledValue(desktop.value("Desktop Entry/Hidden").toString()) ||
        enabledValue(desktop.value("Desktop Entry/NoDisplay").toString()))
        return false;

    const auto currentDesktop = qEnvironmentVariable("XDG_CURRENT_DESKTOP");
    const auto onlyShowIn = desktop.value("Desktop Entry/OnlyShowIn").toString()
        .split(';', Qt::SkipEmptyParts);
    const auto notShowIn = desktop.value("Desktop Entry/NotShowIn").toString()
        .split(';', Qt::SkipEmptyParts);
    if (!onlyShowIn.isEmpty() && !currentDesktop.isEmpty()) {
        bool allowed = false;
        for (const auto &desktopName : currentDesktop.split(':'))
            allowed |= onlyShowIn.contains(desktopName, Qt::CaseInsensitive);
        if (!allowed) return false;
    }
    for (const auto &desktopName : currentDesktop.split(':'))
        if (notShowIn.contains(desktopName, Qt::CaseInsensitive)) return false;

    const auto tryExec = desktop.value("Desktop Entry/TryExec").toString();
    return tryExec.isEmpty() || !QStandardPaths::findExecutable(tryExec).isEmpty();
}

QString categoryFor(const QStringList &categories)
{
    for (const auto &category : categories) {
        if (category.compare("Development", Qt::CaseInsensitive) == 0 ||
            category.compare("IDE", Qt::CaseInsensitive) == 0) return "开发";
        if (category.compare("Game", Qt::CaseInsensitive) == 0) return "游戏";
        if (category.compare("Graphics", Qt::CaseInsensitive) == 0 ||
            category.compare("Photography", Qt::CaseInsensitive) == 0) return "图形";
        if (category.compare("AudioVideo", Qt::CaseInsensitive) == 0 ||
            category.compare("Audio", Qt::CaseInsensitive) == 0 ||
            category.compare("Video", Qt::CaseInsensitive) == 0) return "多媒体";
        if (category.compare("Office", Qt::CaseInsensitive) == 0 ||
            category.compare("Finance", Qt::CaseInsensitive) == 0) return "办公";
        if (category.compare("Network", Qt::CaseInsensitive) == 0 ||
            category.compare("WebBrowser", Qt::CaseInsensitive) == 0) return "网络";
        if (category.compare("System", Qt::CaseInsensitive) == 0 ||
            category.compare("Settings", Qt::CaseInsensitive) == 0) return "系统";
    }
    return "工具";
}

QString desktopName(const QSettings &desktop, const QFileInfo &file)
{
    const auto name = desktop.value("Desktop Entry/Name[" + QLocale().name() + "]", desktop.value("Desktop Entry/Name")).toString().trimmed();
    return name.isEmpty() ? file.completeBaseName() : name;
}
}

QList<Application> discoverApplications()
{
    QList<Application> apps;
    QSet<QString> seen;
    const auto roots = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
    for (const auto &root : roots) {
        QDirIterator files(root, {"*.desktop"}, QDir::Files, QDirIterator::Subdirectories);
        while (files.hasNext()) {
            const QFileInfo file(files.next());
            QString desktopId = QDir(root).relativeFilePath(file.filePath()); desktopId.replace('/', '-');
            if (seen.contains(desktopId)) continue;
            seen.insert(desktopId); // First XDG entry, including Hidden, overrides lower priority roots.
            QSettings desktop(file.absoluteFilePath(), QSettings::IniFormat);
            if (desktop.value("Desktop Entry/Type").toString() != "Application" ||
                !desktopVisible(desktop)) continue;

            Application app;
            app.name = desktopName(desktop, file);
            app.path = file.absoluteFilePath();
            app.category = categoryFor(desktop.value("Desktop Entry/Categories").toString()
                .split(';', Qt::SkipEmptyParts));
            apps.append(std::move(app));
        }
    }
    std::sort(apps.begin(), apps.end(), [](const auto &left, const auto &right) {
        return left.name.localeAwareCompare(right.name) < 0;
    });
    return apps;
}

QImage applicationIcon(const QString &path)
{
    QSettings desktop(path, QSettings::IniFormat);
    const auto iconName = desktop.value("Desktop Entry/Icon").toString().trimmed();
    QIcon icon;
    if (QFileInfo::exists(iconName)) icon = QIcon(iconName);
    if (icon.isNull() && !iconName.isEmpty()) icon = QIcon::fromTheme(iconName);
    if (icon.isNull()) icon = QIcon::fromTheme("application-x-executable");
    if (icon.isNull()) icon = QApplication::style()->standardIcon(QStyle::SP_FileIcon);
    return icon.pixmap(QSize(96, 96)).toImage();
}

bool openApplication(const QString &path, QString &error)
{
    const auto gio = QStandardPaths::findExecutable("gio");
    if (gio.isEmpty()) {
        error = "缺少 gio，请安装 GLib 工具后重试。";
        return false;
    }
    QProcess process;
    process.start(gio, {"launch", path});
    if (!process.waitForStarted(1500)) {
        error = "无法启动 gio。";
        return false;
    }
    if (!process.waitForFinished(5000)) {
        process.kill();
        process.waitForFinished();
        error = "应用启动超时，请从系统启动器重试。";
        return false;
    }
    if (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0) return true;
    error = QStringLiteral("启动失败：%1")
        .arg(QString::fromLocal8Bit(process.readAllStandardError()).trimmed());
    return false;
}
