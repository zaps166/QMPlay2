/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2016  Błażej Szczygieł

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published
	by the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <LineEdit.hpp>
#include <QMPlay2Core.hpp>

#include <QResizeEvent>

LineEditButton::LineEditButton()
{
	setToolTip(tr("Clear"));
	setPixmap(QMPlay2Core.getIconFromTheme("edit-clear").pixmap(16, 16));
	resize(pixmap()->size());
	setCursor(Qt::ArrowCursor);
}

void LineEditButton::mousePressEvent(QMouseEvent *e)
{
	if (e->button() & Qt::LeftButton)
		emit clicked();
}

/**/

LineEdit::LineEdit(QWidget *parent) : QLineEdit(parent)
{
	connect(this, SIGNAL(textChanged(const QString &)), this, SLOT(textChangedSlot(const QString &)));
	connect(&b, SIGNAL(clicked()), this, SLOT(clear_text()));
	setMinimumWidth(b.width() * 2.5);
	setTextMargins(0, 0, b.width() * 1.5, 0);
	b.setParent(this);
	b.hide();
}

void LineEdit::resizeEvent(QResizeEvent *e)
{
	b.move(e->size().width() - b.width() * 1.5, e->size().height() / 2 - b.height() / 2);
}
void LineEdit::mousePressEvent(QMouseEvent *e)
{
	if (!b.underMouse())
		QLineEdit::mousePressEvent(e);
}
void LineEdit::mouseMoveEvent(QMouseEvent *e)
{
	if (!b.underMouse())
		QLineEdit::mouseMoveEvent(e);
}

void LineEdit::textChangedSlot(const QString &str)
{
	b.setVisible(!str.isEmpty());
}
void LineEdit::clear_text()
{
	clear();
	emit clearButtonClicked();
}
