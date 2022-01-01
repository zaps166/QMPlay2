/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2022  Błażej Szczygieł

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

#include <Appearance.hpp>

#include <ColorButton.hpp>
#include <VideoDock.hpp>
#include <Functions.hpp>
#include <Settings.hpp>
#include <Main.hpp>

#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QApplication>
#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QGridLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QSettings>
#include <QComboBox>
#include <QPainter>
#include <QSlider>
#include <QStyle>
#include <QLabel>
#include <QFile>

class WallpaperW : public QWidget
{
public:
    inline void setPixmap(const QPixmap &pix)
    {
        pixmap = pix;
        update();
    }
    inline const QPixmap &getPixmap() const
    {
        return pixmap;
    }
private:
    void paintEvent(QPaintEvent *) override
    {
        if (!pixmap.isNull())
        {
            QPainter p(this);
            Functions::drawPixmap(p, pixmap, this);
        }
    }

    QPixmap pixmap;
};

/**/

#define DEFAULT_ALPHA  0.3
#define DEFAULT_GRAD1  0xFF000000
#define DEFAULT_GRAD2  0xFF333366
#define DEFAULT_QMPTXT 0xFFFFFFFF

static const QString QMPlay2ColorExtension = ".QMPlay2Color";
static QPalette systemPalette;
static QString colorsDir;

static void setBrush(QPalette &pal, QPalette::ColorRole colorRole, QColor color, double alpha = 1.0)
{
    color.setAlphaF(alpha);
    if (colorRole == QPalette::AlternateBase)
    {
        int r, g, b, a;
        color.getRgb(&r, &g, &b, &a);
        if (color.lightnessF() >= 0.5)
        {
            r -= 8;
            g -= 8;
            b -= 8;
        }
        else
        {
            r += 8;
            g += 8;
            b += 8;
        }
        r = qBound(0, r, 255);
        g = qBound(0, g, 255);
        b = qBound(0, b, 255);
        color.setRgb(r, g, b, a);
    }
    pal.setBrush(QPalette::Active, colorRole, color);
    pal.setBrush(QPalette::Inactive, colorRole, colorRole == QPalette::Highlight ? color.lighter(120) : color);
    pal.setBrush(QPalette::Disabled, colorRole, color.darker(140));
}
static QPixmap getWallpaper(const QByteArray &base64Pixmap)
{
    QByteArray wallpaper_data = QByteArray::fromBase64(base64Pixmap);
    QDataStream ds(wallpaper_data);
    QPixmap wallpaper;
    ds >> wallpaper;
    return wallpaper;
}
static QString getColorSchemePath(const QString &dir, QString name)
{
    name += QMPlay2ColorExtension;
    if (QFile::exists(":/Colors/" + name))
        return ":/Colors/" + name;
    return dir + name;
}

void Appearance::init()
{
    colorsDir = QMPlay2Core.getSettingsDir() + "Colors/";

    systemPalette = QApplication::palette();
    QDir dir(QMPlay2Core.getSettingsDir());
    dir.mkdir("Colors");

    QMPlay2GUI.grad1  = DEFAULT_GRAD1;
    QMPlay2GUI.grad2  = DEFAULT_GRAD2;
    QMPlay2GUI.qmpTxt = DEFAULT_QMPTXT;

    QString colorScheme = QMPlay2Core.getSettings().getString("ColorScheme");
    if (!colorScheme.isEmpty())
    {
        const QString filePath = getColorSchemePath(colorsDir, colorScheme);
        QSettings colorScheme(filePath, QSettings::IniFormat);
        if (colorScheme.value("QMPlay2ColorScheme").toBool())
        {
            bool mustApplyPalette = false;

            if (colorScheme.contains("VideoDock/Grad1"))
                QMPlay2GUI.grad1 = colorScheme.value("VideoDock/Grad1").toUInt();
            if (colorScheme.contains("VideoDock/Grad2"))
                QMPlay2GUI.grad2 = colorScheme.value("VideoDock/Grad2").toUInt();
            if (colorScheme.contains("VideoDock/QmpTxt"))
                QMPlay2GUI.qmpTxt = colorScheme.value("VideoDock/QmpTxt").toUInt();

            QPalette pal = systemPalette, sliderButton_pal = systemPalette;
            if (colorScheme.value("Colors/Use").toBool())
            {
                if (colorScheme.contains("Colors/Button"))
                    setBrush(pal, QPalette::Button, colorScheme.value("Colors/Button").toUInt());
                if (colorScheme.contains("Colors/Window"))
                    setBrush(pal, QPalette::Window, colorScheme.value("Colors/Window").toUInt());
                if (colorScheme.contains("Colors/Shadow"))
                    setBrush(pal, QPalette::Shadow, colorScheme.value("Colors/Shadow").toUInt());
                if (colorScheme.contains("Colors/Highlight"))
                    setBrush(pal, QPalette::Highlight, colorScheme.value("Colors/Highlight").toUInt());
                if (colorScheme.contains("Colors/Base"))
                {
                    setBrush(pal, QPalette::Base, colorScheme.value("Colors/Base").toUInt());
                    setBrush(pal, QPalette::AlternateBase, colorScheme.value("Colors/Base").toUInt());
                }
                if (colorScheme.contains("Colors/Text"))
                {
                    setBrush(pal, QPalette::Text, colorScheme.value("Colors/Text").toUInt());
                    setBrush(pal, QPalette::WindowText, colorScheme.value("Colors/Text").toUInt());
                    setBrush(pal, QPalette::ButtonText, colorScheme.value("Colors/Text").toUInt());
                }
                if (colorScheme.contains("Colors/HighlightedText"))
                    setBrush(pal, QPalette::HighlightedText, colorScheme.value("Colors/HighlightedText").toUInt());
                sliderButton_pal = pal;
                if (colorScheme.contains("Colors/SliderButton"))
                    setBrush(sliderButton_pal, QPalette::Button, colorScheme.value("Colors/SliderButton").toUInt());
                mustApplyPalette = true;
            }

            QPalette mainW_pal = pal;
            if (colorScheme.value("Wallpaper/Use").toBool() && colorScheme.contains("Wallpaper/Image"))
            {
                QPixmap wallpaper = getWallpaper(colorScheme.value("Wallpaper/Image").toByteArray());
                if (!wallpaper.isNull())
                {
                    double alpha = 1.0;
                    if (colorScheme.contains("Wallpaper/Alpha"))
                    {
                        alpha = colorScheme.value("Wallpaper/Alpha").toDouble();
                        setBrush(mainW_pal, QPalette::Button, mainW_pal.brush(QPalette::Button).color(), alpha);
                        setBrush(mainW_pal, QPalette::Base, mainW_pal.brush(QPalette::Base).color(), alpha);
                        setBrush(mainW_pal, QPalette::AlternateBase, mainW_pal.brush(QPalette::AlternateBase).color(), alpha);
                        setBrush(mainW_pal, QPalette::Shadow, mainW_pal.brush(QPalette::Shadow).color(), alpha);
                        setBrush(mainW_pal, QPalette::Highlight, mainW_pal.brush(QPalette::Highlight).color(), alpha);
                    }
                    mainW_pal.setBrush(QPalette::Window, wallpaper);

                    emit QMPlay2Core.wallpaperChanged(true, alpha);
                    mustApplyPalette = true;
                }
            }

            if (mustApplyPalette)
                applyPalette(pal, sliderButton_pal, mainW_pal);
        }
    }
}
void Appearance::applyPalette(const QPalette &pal, const QPalette &sliderButton_pal, const QPalette &mainW_pal)
{
    QApplication::setPalette(pal);
    QMPlay2GUI.mainW->setPalette(mainW_pal);
    for (QWidget *w : QApplication::allWidgets())
    {
        QSlider *s = qobject_cast<QSlider *>(w);
        if (s)
            s->setPalette(sliderButton_pal);
    }
}

Appearance::Appearance(QWidget *p) :
    QDialog(p)
{
    setWindowTitle(tr("Appearance settings"));

    colorSchemesB = new QGroupBox(tr("Manage color schemes"));

    schemesB = new QComboBox;
    reloadSchemes();

    newB = new QPushButton(tr("New"));
    connect(newB, SIGNAL(clicked()), this, SLOT(newScheme()));

    saveB = new QPushButton(tr("Save"));
    connect(saveB, SIGNAL(clicked()), this, SLOT(saveScheme()));

    deleteB = new QPushButton(tr("Remove"));
    connect(deleteB, SIGNAL(clicked()), this, SLOT(deleteScheme()));

    QGridLayout *layout = new QGridLayout(colorSchemesB);
    layout->addWidget(schemesB, 0, 0, 1, 3);
    layout->addWidget(newB, 1, 0, 1, 1);
    layout->addWidget(saveB, 1, 1, 1, 1);
    layout->addWidget(deleteB, 1, 2, 1, 1);
    layout->setMargin(3);


    useColorsB = new QGroupBox(tr("Use custom colors"));
    useColorsB->setCheckable(true);

    QFormLayout *formLayout = new QFormLayout(useColorsB);
    formLayout->addRow(tr("Buttons color") + ":", buttonC = new ColorButton);
    formLayout->addRow(tr("Window color") + ":", windowC = new ColorButton);
    formLayout->addRow(tr("Border color") + ":", shadowC = new ColorButton);
    formLayout->addRow(tr("Highlight color") + ":", highlightC = new ColorButton);
    formLayout->addRow(tr("Base color") + ":", baseC = new ColorButton);
    formLayout->addRow(tr("Text color") + ":", textC = new ColorButton);
    formLayout->addRow(tr("Highlighted text color") + ":", highlightedTextC = new ColorButton);
    formLayout->addRow(tr("Slider button color") + ":", sliderButtonC = new ColorButton);
    formLayout->setMargin(3);


    gradientB = new QGroupBox(tr("Gradient in the video window"));

    formLayout = new QFormLayout(gradientB);
    formLayout->addRow(tr("The color on the top and bottom") + ":", grad1C = new ColorButton);
    formLayout->addRow(tr("Color in the middle") + ":", grad2C = new ColorButton);
    formLayout->addRow(tr("Text color") + ":", qmpTxtC = new ColorButton);


    useWallpaperB = new QGroupBox(tr("Wallpaper in the main window"));
    useWallpaperB->setCheckable(true);

    wallpaperW = new WallpaperW;

    alphaB = new QDoubleSpinBox;
    alphaB->setPrefix(tr("Transparency") + ": ");
    alphaB->setRange(0.0, 1.0);
    alphaB->setSingleStep(0.1);
    alphaB->setDecimals(1);

    wallpaperB = new QPushButton(tr("Select wallpaper"));
    connect(wallpaperB, SIGNAL(clicked()), this, SLOT(chooseWallpaper()));

    layout = new QGridLayout(useWallpaperB);
    layout->addWidget(wallpaperW);
    layout->addWidget(alphaB);
    layout->addWidget(wallpaperB);
    layout->setMargin(3);


    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);
    connect(buttonBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(buttonBoxClicked(QAbstractButton *)));


    layout = new QGridLayout(this);
    layout->addWidget(colorSchemesB, 0, 0, 1, 2);
    layout->addWidget(useColorsB, 1, 0, 1, 1);
    layout->addWidget(gradientB, 2, 0, 1, 1);
    layout->addWidget(useWallpaperB, 1, 1, 2, 1);
    layout->addWidget(buttonBox, 3, 0, 1, 2);
    layout->setMargin(3);


    int pos = schemesB->findText(QMPlay2Core.getSettings().getString("ColorScheme"));
    if (pos >= 2)
        schemesB->setCurrentIndex(pos);
    else if (QApplication::palette() == systemPalette && QMPlay2GUI.mainW->palette() == systemPalette && QMPlay2GUI.grad1 == DEFAULT_GRAD1 && QMPlay2GUI.grad2 == DEFAULT_GRAD2 && QMPlay2GUI.qmpTxt == DEFAULT_QMPTXT)
        schemesB->setCurrentIndex(1);
    else
        loadCurrentPalette();


    connect(buttonC, SIGNAL(colorChanged()), this, SLOT(showReadOnlyWarning()));
    connect(windowC, SIGNAL(colorChanged()), this, SLOT(showReadOnlyWarning()));
    connect(shadowC, SIGNAL(colorChanged()), this, SLOT(showReadOnlyWarning()));
    connect(highlightC, SIGNAL(colorChanged()), this, SLOT(showReadOnlyWarning()));
    connect(baseC, SIGNAL(colorChanged()), this, SLOT(showReadOnlyWarning()));
    connect(textC, SIGNAL(colorChanged()), this, SLOT(showReadOnlyWarning()));
    connect(highlightedTextC, SIGNAL(colorChanged()), this, SLOT(showReadOnlyWarning()));
    connect(sliderButtonC, SIGNAL(colorChanged()), this, SLOT(showReadOnlyWarning()));
    connect(grad1C, SIGNAL(colorChanged()), this, SLOT(showReadOnlyWarning()));
    connect(grad2C, SIGNAL(colorChanged()), this, SLOT(showReadOnlyWarning()));
    connect(qmpTxtC, SIGNAL(colorChanged()), this, SLOT(showReadOnlyWarning()));
}

void Appearance::schemesIndexChanged(int idx)
{
    if (idx == 0)
        loadCurrentPalette();
    else
        loadDefaultPalette();
    if (idx >= 2)
    {
        QSettings colorScheme(getColorSchemePath(colorsDir, schemesB->currentText()), QSettings::IniFormat);
        if (colorScheme.value("QMPlay2ColorScheme").toBool())
        {
            if (colorScheme.contains("VideoDock/Grad1"))
                grad1C->setColor(colorScheme.value("VideoDock/Grad1").toUInt());
            if (colorScheme.contains("VideoDock/Grad2"))
                grad2C->setColor(colorScheme.value("VideoDock/Grad2").toUInt());
            if (colorScheme.contains("VideoDock/QmpTxt"))
                qmpTxtC->setColor(colorScheme.value("VideoDock/QmpTxt").toUInt());

            useColorsB->setChecked(colorScheme.value("Colors/Use", useColorsB->isChecked()).toBool());
            if (colorScheme.value("Colors/Use").toBool())
            {
                if (colorScheme.contains("Colors/Button"))
                    buttonC->setColor(colorScheme.value("Colors/Button").toUInt());
                if (colorScheme.contains("Colors/Window"))
                    windowC->setColor(colorScheme.value("Colors/Window").toUInt());
                if (colorScheme.contains("Colors/Shadow"))
                    shadowC->setColor(colorScheme.value("Colors/Shadow").toUInt());
                if (colorScheme.contains("Colors/Highlight"))
                    highlightC->setColor(colorScheme.value("Colors/Highlight").toUInt());
                if (colorScheme.contains("Colors/Base"))
                    baseC->setColor(colorScheme.value("Colors/Base").toUInt());
                if (colorScheme.contains("Colors/Text"))
                    textC->setColor(colorScheme.value("Colors/Text").toUInt());
                if (colorScheme.contains("Colors/HighlightedText"))
                    highlightedTextC->setColor(colorScheme.value("Colors/HighlightedText").toUInt());
                if (colorScheme.contains("Colors/SliderButton"))
                    sliderButtonC->setColor(colorScheme.value("Colors/SliderButton").toUInt());
            }

            useWallpaperB->setChecked(colorScheme.value("Wallpaper/Use", useWallpaperB->isChecked()).toBool());
            if (colorScheme.value("Wallpaper/Use").toBool())
            {
                if (colorScheme.contains("Wallpaper/Alpha"))
                    alphaB->setValue(colorScheme.value("Wallpaper/Alpha").toDouble());
                if (colorScheme.contains("Wallpaper/Image"))
                    wallpaperW->setPixmap(getWallpaper(colorScheme.value("Wallpaper/Image").toByteArray()));
            }

            if (idx >= rwSchemesIdx)
                saveB->setEnabled(true);
        }
        if (idx >= rwSchemesIdx)
            deleteB->setEnabled(true);
    }
    warned = false;
}

void Appearance::newScheme()
{
    bool ok;
    const QString name = Functions::cleanFileName(QInputDialog::getText(this, tr("Name"), tr("Enter the name for a color scheme"), QLineEdit::Normal, QString(), &ok));
    if (ok && !name.isEmpty())
    {
        for (int i = 2; i < schemesB->count(); ++i)
            if (schemesB->itemText(i) == name)
            {
                QMessageBox::warning(this, tr("Name"), tr("The specified name already exists!"));
                return;
            }
        QSettings colorScheme(colorsDir + name + QMPlay2ColorExtension, QSettings::IniFormat);
        if (colorScheme.isWritable())
        {
            colorScheme.setValue("QMPlay2ColorScheme", true);
            saveScheme(colorScheme);
            colorScheme.sync();

            reloadSchemes();
            int idx = schemesB->findText(name);
            if (idx >= 2)
                schemesB->setCurrentIndex(idx);
        }
    }
}
void Appearance::saveScheme()
{
    const QString filePath = colorsDir + schemesB->currentText() + QMPlay2ColorExtension;
    if (QFile::exists(filePath))
    {
        QSettings colorScheme(filePath, QSettings::IniFormat);
        if (colorScheme.isWritable())
            saveScheme(colorScheme);
    }
}
void Appearance::deleteScheme()
{
    if (schemesB->currentIndex() > 1 && QMessageBox::question(this, tr("Removing"), tr("Do you want to remove") + ": \"" + schemesB->currentText() + "\"?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
    {
        QFile::remove(colorsDir + schemesB->currentText() + QMPlay2ColorExtension);
        schemesB->removeItem(schemesB->currentIndex());
    }
}
void Appearance::chooseWallpaper()
{
    const QString filePath = QFileDialog::getOpenFileName(this, tr("Loading wallpaper"), QMPlay2GUI.getCurrentPth(), tr("Pictures") + " (*.jpg *.jpeg *.png *.gif *.bmp)");
    if (!filePath.isEmpty())
    {
        QFile f(filePath);
        if (f.open(QFile::ReadOnly))
        {
            QPixmap pixmap;
            pixmap.loadFromData(f.readAll());
            wallpaperW->setPixmap(pixmap);
            showReadOnlyWarning();
        }
    }
}
void Appearance::buttonBoxClicked(QAbstractButton *b)
{
    switch (buttonBox->buttonRole(b))
    {
        case QDialogButtonBox::AcceptRole:
            apply();
            if (saveB->isEnabled())
                saveScheme();
            accept();
            break;
        case QDialogButtonBox::RejectRole:
            reject();
            break;
        case QDialogButtonBox::ApplyRole:
            apply();
            break;
        default:
            break;
    }
}
void Appearance::showReadOnlyWarning()
{
    if (!saveB->isEnabled() && !warned)
    {
        QMessageBox::warning(this, tr("Volatile settings"), schemesB->currentText() + " " + tr("is not writable, settings will be lost after restart. Consider creating a new color scheme!"));
        warned = true;
    }
}

void Appearance::saveScheme(QSettings &colorScheme)
{
    colorScheme.setValue("VideoDock/Grad1", grad1C->getColor().rgb());
    colorScheme.setValue("VideoDock/Grad2", grad2C->getColor().rgb());
    colorScheme.setValue("VideoDock/QmpTxt", qmpTxtC->getColor().rgb());

    colorScheme.remove("Colors");
    colorScheme.setValue("Colors/Use", useColorsB->isChecked());
    if (useColorsB->isChecked())
    {
        colorScheme.setValue("Colors/Button", buttonC->getColor().rgb());
        colorScheme.setValue("Colors/Window", windowC->getColor().rgb());
        colorScheme.setValue("Colors/Shadow", shadowC->getColor().rgb());
        colorScheme.setValue("Colors/Highlight", highlightC->getColor().rgb());
        colorScheme.setValue("Colors/Base", baseC->getColor().rgb());
        colorScheme.setValue("Colors/Text", textC->getColor().rgb());
        colorScheme.setValue("Colors/HighlightedText", highlightedTextC->getColor().rgb());
        colorScheme.setValue("Colors/SliderButton", sliderButtonC->getColor().rgb());
    }

    colorScheme.remove("Wallpaper");
    const QPixmap wallpaper = wallpaperW->getPixmap();
    colorScheme.setValue("Wallpaper/Use", !wallpaper.isNull() && useWallpaperB->isChecked());
    if (useWallpaperB->isChecked())
    {
        colorScheme.setValue("Wallpaper/Alpha", alphaB->value());
        if (!wallpaper.isNull())
        {
            QByteArray img_arr;
            QDataStream ds(&img_arr, QIODevice::WriteOnly);
            ds << wallpaper;
            colorScheme.setValue("Wallpaper/Image", img_arr.toBase64());
        }
    }
}
void Appearance::reloadSchemes()
{
    disconnect(schemesB, SIGNAL(currentIndexChanged(int)), this, SLOT(schemesIndexChanged(int)));
    schemesB->clear();
    schemesB->addItems({tr("Current color scheme"), tr("Default color scheme")});
    rwSchemesIdx = 2;
    for (const QString &fName : QDir(":/Colors").entryList())
    {
        schemesB->addItem(fName.left(fName.length() - QMPlay2ColorExtension.length()));
        ++rwSchemesIdx;
    }
    for (const QString &fName : QDir(colorsDir).entryList({"*" + QMPlay2ColorExtension}, QDir::Files, QDir::Name))
        schemesB->addItem(fName.left(fName.length() - QMPlay2ColorExtension.length()));
    connect(schemesB, SIGNAL(currentIndexChanged(int)), this, SLOT(schemesIndexChanged(int)));
    warned = false;
}
void Appearance::loadCurrentPalette()
{
    QPalette currentPalette = QApplication::palette(), sliderPalette, mainW_pal = QMPlay2GUI.mainW->palette();
    for (QWidget *w : QApplication::allWidgets())
    {
        QSlider *s = qobject_cast<QSlider *>(w);
        if (s && s->isEnabled())
        {
            sliderPalette = s->palette();
            break;
        }
    }

    grad1C->setColor(QMPlay2GUI.grad1);
    grad2C->setColor(QMPlay2GUI.grad2);
    qmpTxtC->setColor(QMPlay2GUI.qmpTxt);

    useColorsB->setChecked(currentPalette != systemPalette);
    buttonC->setColor(currentPalette.brush(QPalette::Button).color());
    windowC->setColor(currentPalette.brush(QPalette::Window).color());
    shadowC->setColor(currentPalette.brush(QPalette::Shadow).color());
    highlightC->setColor(currentPalette.brush(QPalette::Highlight).color());
    baseC->setColor(currentPalette.brush(QPalette::Base).color());
    textC->setColor(currentPalette.brush(QPalette::Text).color());
    highlightedTextC->setColor(currentPalette.brush(QPalette::HighlightedText).color());
    sliderButtonC->setColor(sliderPalette.brush(QPalette::Button).color());

    QPixmap wallpaper = mainW_pal.brush(QPalette::Window).texture();
    useWallpaperB->setChecked(!wallpaper.isNull());
    alphaB->setValue(useWallpaperB->isChecked() ? mainW_pal.brush(QPalette::Base).color().alphaF() : DEFAULT_ALPHA);
    wallpaperW->setPixmap(wallpaper);

    saveB->setEnabled(false);
    deleteB->setEnabled(false);
}
void Appearance::loadDefaultPalette()
{
    grad1C->setColor(DEFAULT_GRAD1);
    grad2C->setColor(DEFAULT_GRAD2);
    qmpTxtC->setColor(DEFAULT_QMPTXT);

    useColorsB->setChecked(false);
    buttonC->setColor(systemPalette.brush(QPalette::Button).color());
    windowC->setColor(systemPalette.brush(QPalette::Window).color());
    shadowC->setColor(systemPalette.brush(QPalette::Shadow).color());
    highlightC->setColor(systemPalette.brush(QPalette::Highlight).color());
    baseC->setColor(systemPalette.brush(QPalette::Base).color());
    textC->setColor(systemPalette.brush(QPalette::Text).color());
    highlightedTextC->setColor(systemPalette.brush(QPalette::HighlightedText).color());
    sliderButtonC->setColor(systemPalette.brush(QPalette::Button).color());

    useWallpaperB->setChecked(false);
    alphaB->setValue(DEFAULT_ALPHA);
    wallpaperW->setPixmap(QPixmap());

    saveB->setEnabled(false);
    deleteB->setEnabled(false);
}
void Appearance::apply()
{
    QMPlay2GUI.grad1 = grad1C->getColor();
    QMPlay2GUI.grad2 = grad2C->getColor();
    QMPlay2GUI.qmpTxt = qmpTxtC->getColor();
    QMPlay2GUI.updateInDockW();

    QPalette pal, sliderButton_pal;
    if (!useColorsB->isChecked())
        pal = sliderButton_pal = systemPalette;
    else
    {
        setBrush(pal, QPalette::Button, buttonC->getColor());
        setBrush(pal, QPalette::Window, windowC->getColor());
        setBrush(pal, QPalette::Shadow, shadowC->getColor());
        setBrush(pal, QPalette::Highlight, highlightC->getColor());
        setBrush(pal, QPalette::Base, baseC->getColor());
        setBrush(pal, QPalette::AlternateBase, baseC->getColor());
        setBrush(pal, QPalette::Text, textC->getColor());
        setBrush(pal, QPalette::WindowText, textC->getColor());
        setBrush(pal, QPalette::ButtonText, textC->getColor());
        setBrush(pal, QPalette::HighlightedText, highlightedTextC->getColor());
        sliderButton_pal = pal;
        setBrush(sliderButton_pal, QPalette::Button, sliderButtonC->getColor());
    }

    QPalette mainW_pal = pal;
    bool hasWallpaper = false;
    if (useWallpaperB->isChecked())
    {
        QPixmap pixmap = wallpaperW->getPixmap();
        if (!pixmap.isNull())
        {
            mainW_pal.setBrush(QPalette::Window, pixmap);
            setBrush(mainW_pal, QPalette::Button, buttonC->getColor(), alphaB->value());
            setBrush(mainW_pal, QPalette::Base, baseC->getColor(), alphaB->value());
            setBrush(mainW_pal, QPalette::AlternateBase, baseC->getColor(), alphaB->value());
            setBrush(mainW_pal, QPalette::Shadow, shadowC->getColor(), alphaB->value());
            setBrush(mainW_pal, QPalette::Highlight, highlightC->getColor(), alphaB->value());
            hasWallpaper = true;
        }
    }
    emit QMPlay2Core.wallpaperChanged(hasWallpaper, alphaB->value());

    applyPalette(pal, sliderButton_pal, mainW_pal);

    QMPlay2Core.getSettings().set("ColorScheme", schemesB->currentIndex() >= 2 ? schemesB->currentText() : QString());
}
