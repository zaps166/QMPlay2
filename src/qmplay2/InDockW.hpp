// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QMPlay2Headers.hpp>

class QMPLAY2SHAREDLIB_EXPORT InDockW final : public QWidget
{
    Q_OBJECT
public:
    InDockW(const QColor &, const QColor &, const QColor &);

    QWidget *getWidget();
    void setLoseHeight(int lh);
    void setCustomPixmap(const QPixmap &pix = QPixmap());

    void updateSettings();

private:
    const QColor &grad1, &grad2, &qmpTxt;
    Qt::TransformationMode blurredTransformation;
    QPixmap customPixmap, customPixmapBlurred;
    bool m_enlargeCovers = false;
    bool m_hideLogo = false;
    bool hasWallpaper;
    int loseHeight;
    QPointer<QWidget> w;
private slots:
    void wallpaperChanged(bool hasWallpaper, double alpha);
public:
    void setWidget(QWidget *newW);
protected:
    void resizeEvent(QResizeEvent *) override;
    void paintEvent(QPaintEvent *) override;
    void enterEvent(Q_ENTER_EVENT *) override;
    void leaveEvent(QEvent *) override;
    bool event(QEvent *) override;
signals:
    void resized(const QSize &size);
    void itemDropped(const QString &);
    void hasCoverImage(bool);
};
