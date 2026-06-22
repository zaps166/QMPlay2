/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2026  Błażej Szczygieł

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

#include <OSDSettingsW.hpp>

#include <Settings.hpp>
#include <LibASS.hpp>

#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFontComboBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QSpacerItem>
#include <QLabel>
#include <QSpinBox>
#include <QGuiApplication>
#include <QRadioButton>
#include <QCoreApplication>

#include "ColorButton.hpp"

void OSDSettingsW::init(const QString &prefix, int a, int b, int c, int d, int e, int f, double g, double h, const QColor &i, const QColor &j, const QColor &k, bool l, bool m)
{
    Settings &QMPSettings = QMPlay2Core.getSettings();
    QMPSettings.init(prefix + "/FontName", QGuiApplication::font().family());
    QMPSettings.init(prefix + "/FontSize", a);
    QMPSettings.init(prefix + "/Spacing", b);
    QMPSettings.init(prefix + "/Bold", l);
    QMPSettings.init(prefix + "/LeftMargin", c);
    QMPSettings.init(prefix + "/RightMargin", d);
    QMPSettings.init(prefix + "/VMargin", e);
    QMPSettings.init(prefix + "/Alignment", f);
    QMPSettings.init(prefix + "/Outline", g);
    QMPSettings.init(prefix + "/Shadow", h);
    QMPSettings.init(prefix + "/Background", m);
    QMPSettings.init(prefix + "/TextColor", i);
    QMPSettings.init(prefix + "/OutlineColor", j);
    QMPSettings.init(prefix + "/ShadowColor", k);
    QMPSettings.init(prefix + "/BackgroundColor", QColor(k.red(), k.green(), k.blue(), 0x88));
    const int align = QMPSettings.getInt(prefix + "/Alignment");
    if (align < 0 || align > 8)
        QMPSettings.set(prefix + "/Alignment", f);
}

OSDSettingsW::OSDSettingsW(const QString &prefix) :
    prefix(prefix)
{
    setWidgetResizable(true);
    setFrameShape(QFrame::NoFrame);

    auto widget = new QWidget;

    setupUi(widget);

    readSettings();

    setDisabled(LibASS::isDummy());

    setWidget(widget);
}
OSDSettingsW::~OSDSettingsW()
{
}

void OSDSettingsW::setupUi(QWidget *OSDSettings)
{
    QGridLayout *gridLayout = new QGridLayout(OSDSettings);
    gridLayout->setContentsMargins(1, 1, 1, 1);

    QSpacerItem *horizontalSpacer = new QSpacerItem(89, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);
    gridLayout->addItem(horizontalSpacer, 0, 1, 1, 1);

    m_layout = new QVBoxLayout();
    QHBoxLayout *horizontalLayout = new QHBoxLayout();

    QGroupBox *fontGB = new QGroupBox(OSDSettings);
    QFormLayout *formLayout = new QFormLayout(fontGB);
    m_fontCB = new QFontComboBox(fontGB);
    formLayout->setWidget(0, QFormLayout::ItemRole::SpanningRole, m_fontCB);

    QLabel *fontSizeL = new QLabel(fontGB);
    formLayout->setWidget(1, QFormLayout::ItemRole::LabelRole, fontSizeL);
    m_fontSizeB = new QSpinBox(fontGB);
    formLayout->setWidget(1, QFormLayout::ItemRole::FieldRole, m_fontSizeB);

    QLabel *spacingL = new QLabel(fontGB);
    formLayout->setWidget(2, QFormLayout::ItemRole::LabelRole, spacingL);
    m_spacingB = new QSpinBox(fontGB);
    m_spacingB->setMinimum(-20);
    m_spacingB->setMaximum(20);
    formLayout->setWidget(2, QFormLayout::ItemRole::FieldRole, m_spacingB);

    m_boldCB = new QCheckBox(fontGB);
    formLayout->setWidget(3, QFormLayout::ItemRole::SpanningRole, m_boldCB);

    horizontalLayout->addWidget(fontGB);

    QGroupBox *marginsGB = new QGroupBox(OSDSettings);
    QFormLayout *formLayout2 = new QFormLayout(marginsGB);
    QLabel *leftMarginL = new QLabel(marginsGB);
    formLayout2->setWidget(0, QFormLayout::ItemRole::LabelRole, leftMarginL);
    m_leftMarginB = new QSpinBox(marginsGB);
    formLayout2->setWidget(0, QFormLayout::ItemRole::FieldRole, m_leftMarginB);

    QLabel *rightMarginL = new QLabel(marginsGB);
    formLayout2->setWidget(1, QFormLayout::ItemRole::LabelRole, rightMarginL);
    m_rightMarginB = new QSpinBox(marginsGB);
    formLayout2->setWidget(1, QFormLayout::ItemRole::FieldRole, m_rightMarginB);

    QLabel *vMarginL = new QLabel(marginsGB);
    formLayout2->setWidget(2, QFormLayout::ItemRole::LabelRole, vMarginL);
    m_vMarginB = new QSpinBox(marginsGB);
    formLayout2->setWidget(2, QFormLayout::ItemRole::FieldRole, m_vMarginB);

    horizontalLayout->addWidget(marginsGB);
    m_layout->addLayout(horizontalLayout);

    QHBoxLayout *horizontalLayout2 = new QHBoxLayout();
    QGroupBox *alignGB = new QGroupBox(OSDSettings);
    horizontalLayout2->addWidget(alignGB);

    QGroupBox *frameGB = new QGroupBox(OSDSettings);
    QFormLayout *formLayout3 = new QFormLayout(frameGB);
    m_backgroundCB = new QCheckBox(frameGB);
    formLayout3->setWidget(0, QFormLayout::ItemRole::SpanningRole, m_backgroundCB);
    QLabel *outlineL = new QLabel(frameGB);
    formLayout3->setWidget(1, QFormLayout::ItemRole::LabelRole, outlineL);
    m_outlineB = new QDoubleSpinBox(frameGB);
    m_outlineB->setMaximum(4.0);
    m_outlineB->setSingleStep(0.1);
    formLayout3->setWidget(1, QFormLayout::ItemRole::FieldRole, m_outlineB);
    m_shadowL = new QLabel(frameGB);
    formLayout3->setWidget(3, QFormLayout::ItemRole::LabelRole, m_shadowL);
    m_shadowB = new QDoubleSpinBox(frameGB);
    m_shadowB->setMaximum(4.0);
    m_shadowB->setSingleStep(0.1);
    formLayout3->setWidget(3, QFormLayout::ItemRole::FieldRole, m_shadowB);

    horizontalLayout2->addWidget(frameGB);

    QGroupBox *colorsGB = new QGroupBox(OSDSettings);
    QFormLayout *formLayout4 = new QFormLayout(colorsGB);
    QLabel *textColorL = new QLabel(colorsGB);
    formLayout4->setWidget(0, QFormLayout::ItemRole::LabelRole, textColorL);
    m_textColorB = new ColorButton(colorsGB);
    formLayout4->setWidget(0, QFormLayout::ItemRole::FieldRole, m_textColorB);

    QLabel *outlineColorL = new QLabel(colorsGB);
    formLayout4->setWidget(1, QFormLayout::ItemRole::LabelRole, outlineColorL);
    m_outlineColorB = new ColorButton(colorsGB);
    formLayout4->setWidget(1, QFormLayout::ItemRole::FieldRole, m_outlineColorB);

    m_shadowColorL = new QLabel(colorsGB);
    formLayout4->setWidget(2, QFormLayout::ItemRole::LabelRole, m_shadowColorL);
    m_shadowColorB = new ColorButton(colorsGB);
    m_shadowColorB->setAlphaAllowed(true);
    formLayout4->setWidget(2, QFormLayout::ItemRole::FieldRole, m_shadowColorB);

    horizontalLayout2->addWidget(colorsGB);
    m_layout->addLayout(horizontalLayout2);

    gridLayout->addLayout(m_layout, 0, 0, 1, 1);

    QSpacerItem *verticalSpacer = new QSpacerItem(20, 121, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);
    gridLayout->addItem(verticalSpacer, 1, 0, 1, 1);

    // Alignment GroupBox
    auto *alignL = new QGridLayout(alignGB);
    for (int align = 0; align < 9; ++align)
    {
        alignB[align] = new QRadioButton;
        alignL->addWidget(alignB[align], align / 3, align % 3);
    }

    connect(m_backgroundCB, &QCheckBox::toggled, this, [this, shadowText = m_shadowColorL->text()](bool checked) {
        m_shadowL->setEnabled(!checked);
        m_shadowB->setEnabled(!checked);
        m_shadowColorL->setText(checked ? tr("Background") : shadowText);
        setShadowColorB();
    });

    fontGB->setTitle(tr("Font"));
    fontSizeL->setText(tr("Size: "));
    spacingL->setText(tr("Spacing: "));
    m_boldCB->setText(tr("Bold"));
    marginsGB->setTitle(tr("Margins"));
    leftMarginL->setText(tr("Left: "));
    rightMarginL->setText(tr("Right: "));
    vMarginL->setText(tr("Vertical: "));
    alignGB->setTitle(tr("Subtitles alignment"));
    frameGB->setTitle(tr("Border"));
    outlineL->setText(tr("Outline: "));
    m_shadowL->setText(tr("Shadow: "));
    m_backgroundCB->setText(tr("Background"));
    colorsGB->setTitle(tr("Colors"));
    textColorL->setText(tr("Text: "));
    outlineColorL->setText(tr("Border: "));
    m_shadowColorL->setText(tr("Shadow: "));
}

void OSDSettingsW::addWidget(QWidget *w)
{
    m_layout->addWidget(w);
}

void OSDSettingsW::writeSettings()
{
    Settings &QMPSettings = QMPlay2Core.getSettings();
    QMPSettings.set(prefix + "/FontName", m_fontCB->currentText());
    QMPSettings.set(prefix + "/FontSize", m_fontSizeB->value());
    QMPSettings.set(prefix + "/Spacing", m_spacingB->value());
    QMPSettings.set(prefix + "/Bold", m_boldCB->isChecked());
    QMPSettings.set(prefix + "/LeftMargin", m_leftMarginB->value());
    QMPSettings.set(prefix + "/RightMargin", m_rightMarginB->value());
    QMPSettings.set(prefix + "/VMargin", m_vMarginB->value());
    for (int align = 0; align < 9; ++align)
        if (alignB[align]->isChecked())
        {
            QMPSettings.set(prefix + "/Alignment", align);
            break;
        }
    QMPSettings.set(prefix + "/Outline", m_outlineB->value());
    QMPSettings.set(prefix + "/Shadow", m_shadowB->value());
    QMPSettings.set(prefix + "/Background", m_backgroundCB->isChecked());
    QMPSettings.set(prefix + "/TextColor", m_textColorB->getColor());
    QMPSettings.set(prefix + "/OutlineColor", m_outlineColorB->getColor());
    QMPSettings.set(prefix + (m_backgroundCB->isChecked() ? "/BackgroundColor" : "/ShadowColor"), m_shadowColorB->getColor());
}

void OSDSettingsW::readSettings()
{
    Settings &QMPSettings = QMPlay2Core.getSettings();
    const int idx = m_fontCB->findText(QMPSettings.getString(prefix + "/FontName"));
    if (idx > -1)
        m_fontCB->setCurrentIndex(idx);
    m_fontSizeB->setValue(QMPSettings.getInt(prefix + "/FontSize"));
    m_spacingB->setValue(QMPSettings.getInt(prefix + "/Spacing"));
    m_boldCB->setChecked(QMPSettings.getBool(prefix + "/Bold"));
    m_leftMarginB->setValue(QMPSettings.getInt(prefix + "/LeftMargin"));
    m_rightMarginB->setValue(QMPSettings.getInt(prefix + "/RightMargin"));
    m_vMarginB->setValue(QMPSettings.getInt(prefix + "/VMargin"));
    alignB[QMPSettings.getInt(prefix + "/Alignment")]->setChecked(true);
    m_outlineB->setValue(QMPSettings.getDouble(prefix + "/Outline"));
    m_shadowB->setValue(QMPSettings.getDouble(prefix + "/Shadow"));
    m_backgroundCB->setChecked(QMPSettings.getBool(prefix + "/Background"));
    m_textColorB->setColor(QMPSettings.getColor(prefix + "/TextColor"));
    m_outlineColorB->setColor(QMPSettings.getColor(prefix + "/OutlineColor"));
    if (!m_backgroundCB->isChecked()) // If checked, it's triggered automatically here
        setShadowColorB();
}

inline void OSDSettingsW::setShadowColorB()
{
    m_shadowColorB->setColor(QMPlay2Core.getSettings().getColor(prefix + (m_backgroundCB->isChecked() ? "/BackgroundColor" : "/ShadowColor")));
}
