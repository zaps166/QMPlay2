// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QMPlay2Headers.hpp>

#ifndef Q_MOC_RUN

# include <QAbstractItemModel>
# include <QCryptographicHash>
# include <QRegularExpression>
# include <QLoggingCategory>
# include <QStringListModel>
# include <QXmlStreamReader>
# include <QJsonParseError>
# include <QDeadlineTimer>
# include <QJsonDocument>
# include <QMimeDatabase>
# include <QJsonObject>
# include <QStringList>
# include <QJsonArray>
# include <QJsonValue>
# include <QDateTime>
# include <QMetaEnum>
# include <QMimeData>
# include <QUrlQuery>
# include <QProcess>
# include <QBuffer>
# include <QLocale>
# include <QThread>
# include <QDebug>
# include <QTimer>
# include <QUrl>

# include <QDesktopServices>
# include <QTextDocument>
# include <QActionGroup>
# include <QClipboard>
# include <QTextFrame>
# include <QPainter>
# include <QAction>
# include <QScreen>
# include <QWindow>
# include <QImage>

# include <QDialogButtonBox>
# include <QApplication>
# include <QInputDialog>
# include <QProgressBar>
# include <QDockWidget>
# include <QFileDialog>
# include <QFormLayout>
# include <QGridLayout>
# include <QHBoxLayout>
# include <QHeaderView>
# include <QListWidget>
# include <QMessageBox>
# include <QPushButton>
# include <QSpacerItem>
# include <QToolButton>
# include <QTreeWidget>
# include <QVBoxLayout>
# include <QCompleter>
# include <QScrollBar>
# include <QCheckBox>
# include <QComboBox>
# include <QGroupBox>
# include <QSplitter>
# include <QTextEdit>
# include <QTreeView>
# include <QSpinBox>
# include <QWidget>
# include <QLabel>
# include <QMenu>

# ifdef USE_MPRIS2
#  include <QDBusAbstractAdaptor>
#  include <QDBusConnection>
#  include <QDBusObjectPath>
#  include <QDBusMessage>
# endif

# ifdef USE_MEDIABROWSER
#  include <QJSEngine>
#  include <QJSValue>
# endif

# include <tuple>

#endif
