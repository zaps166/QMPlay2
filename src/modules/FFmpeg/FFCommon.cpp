// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <FFCommon.hpp>

#ifdef FIND_HWACCEL_DRIVERS_PATH
void FFCommon::setDriversPath(const QString &dirName, const QByteArray &envVar)
{
    if (qEnvironmentVariableIsSet(envVar))
        return;

    static const auto libDirs = [] {
        QStringList dirsWithWordSize, dirs;

        QProcess whereis;
        whereis.start("whereis", {"-l"});
        if (whereis.waitForStarted() && whereis.waitForFinished() && whereis.exitCode() == 0)
        {
            const auto wordSizeStr = QByteArray::number(QSysInfo::WordSize);
            for (const auto &entry : whereis.readAllStandardOutput().split('\n'))
            {
                if (!entry.startsWith("bin: "))
                    continue;

                const auto path = entry.mid(5);
                if (path.contains(QByteArray("/lib" + wordSizeStr)))
                    dirsWithWordSize.append(path);
                else if (path.contains("/lib/") || path.endsWith("/lib"))
                    dirs.append(path);
            }
        }

#ifdef Q_OS_LINUX
        QFile maps("/proc/self/maps");
        if (maps.open(QFile::ReadOnly | QFile::Text))
        {
            const auto lines = maps.readAll().split('\n');
            for (auto &&line : lines)
            {
                const auto dir = QRegularExpression(R"((\/usr\/lib\/\S+-linux-gnu)\/.+\.so($|\.))").match(line).captured(1).toUtf8();
                if (!dir.isEmpty())
                {
                    if (!dirs.contains(dir))
                        dirs.append(dir);
                    break;
                }
            }
        }
#endif

        return dirsWithWordSize + dirs;
    }();

    for (auto &&dir : libDirs)
    {
        const QString path = dir + "/" + dirName;
        if (QDir(path).entryList({"*.so*"}, QDir::Files).count() > 0)
        {
            qputenv(envVar, path.toLocal8Bit());
            qDebug().noquote() << "Set" << envVar << "to" << path;
            break;
        }
    }
}
#endif
