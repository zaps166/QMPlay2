// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <GUIHeaders.hpp>

class AboutWidget final : public QWidget
{
    Q_OBJECT
public:
    AboutWidget();
private:
    void showEvent(QShowEvent *) override;
    void closeEvent(QCloseEvent *) override;

    QPlainTextEdit *logE, *clE, *auE;
    QPushButton *clrLogB;
    QFileSystemWatcher logWatcher;
private slots:
    void linkActivated(const QString &link);
    void refreshLog();
    void clrLog();
    void currentTabChanged(int);
};
