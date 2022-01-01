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

#include <TagEditor.hpp>

#include <Functions.hpp>
#include <Main.hpp>

#define TAGLIB_VERSION ((TAGLIB_MAJOR_VERSION << 8) | TAGLIB_MINOR_VERSION)
#define TAGLIB1B (TAGLIB_VERSION >= 0x10B)

#ifdef TAGLIB_FULL_INCLUDE_PATH
    #include <taglib/taglib.h>
#else
    #include <taglib.h>
#endif

#if TAGLIB_VERSION < 0x109
    #error Taglib 1.9 or newer is needed!
#endif

#ifdef TAGLIB_FULL_INCLUDE_PATH
    #include <taglib/trueaudiofile.h>
    #include <taglib/oggflacfile.h>
    #include <taglib/wavpackfile.h>
    #include <taglib/vorbisfile.h>
    #include <taglib/speexfile.h>
    #include <taglib/aifffile.h>
    #include <taglib/mpegfile.h>
    #include <taglib/flacfile.h>
    #include <taglib/asffile.h>
    #include <taglib/mpcfile.h>
    #include <taglib/mp4file.h>
    #include <taglib/wavfile.h>
    #include <taglib/apefile.h>
    #include <taglib/fileref.h>
    #include <taglib/modfile.h>
    #include <taglib/s3mfile.h>
    #include <taglib/itfile.h>
    #include <taglib/xmfile.h>
    #include <taglib/opusfile.h>
#else
    #include <trueaudiofile.h>
    #include <oggflacfile.h>
    #include <wavpackfile.h>
    #include <vorbisfile.h>
    #include <speexfile.h>
    #include <aifffile.h>
    #include <mpegfile.h>
    #include <flacfile.h>
    #include <asffile.h>
    #include <mpcfile.h>
    #include <mp4file.h>
    #include <wavfile.h>
    #include <apefile.h>
    #include <fileref.h>
    #include <modfile.h>
    #include <s3mfile.h>
    #include <itfile.h>
    #include <xmfile.h>
    #include <opusfile.h>
#endif
using namespace TagLib;

#define instanceOf(p, t) (dynamic_cast<t *>(&p) == &p)
static inline bool isOgg(File &file)
{
    return instanceOf(file, Ogg::Vorbis::File) || instanceOf(file, Ogg::FLAC::File) || instanceOf(file, Ogg::Speex::File) || instanceOf(file, Ogg::Opus::File);
}
static inline Ogg::XiphComment *getXiphComment(File &file)
{
    if (instanceOf(file, Ogg::Vorbis::File))
        return ((Ogg::Vorbis::File &)file).tag();
    else if (instanceOf(file, Ogg::FLAC::File))
        return ((Ogg::FLAC::File &)file).tag();
    else if (instanceOf(file, Ogg::Speex::File))
        return ((Ogg::Speex::File &)file).tag();
    else if (instanceOf(file, Ogg::Opus::File))
        return ((Ogg::Opus::File &)file).tag();
    return nullptr;
}

#include <QImageReader>
#include <QFileDialog>
#include <QPushButton>
#include <QFormLayout>
#include <QBoxLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QPainter>
#include <QLabel>

PictureW::PictureW(ByteVector &picture) :
    picture(picture)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

void PictureW::paintEvent(QPaintEvent *)
{
    if (!picture.isEmpty())
    {
        QPixmap pixmap;
        pixmap.loadFromData((const quint8 *)picture.data(), picture.size());
        if (!pixmap.isNull())
        {
            QPainter p(this);
            Functions::drawPixmap(p, pixmap, this);
        }
    }
}

/**/

static inline Tag &getTag(FileRef &fRef, File &file)
{
    return *(instanceOf(file, RIFF::WAV::File) ? ((RIFF::WAV::File &)file).InfoTag() : fRef.tag());
}

static void removeXiphComment(Ogg::XiphComment *xiphComment)
{
    if (xiphComment)
    {
#if TAGLIB1B
        xiphComment->removeAllPictures();
        xiphComment->removeAllFields();
#else
        const Ogg::FieldListMap &fieldListMap = xiphComment->fieldListMap();
        for (Ogg::FieldListMap::ConstIterator it = fieldListMap.begin(); it != fieldListMap.end(); ++it)
        {
            if (xiphComment->contains(it->first))
                xiphComment->removeField(it->first);
        }
#endif
    }
}

/**/

TagEditor::TagEditor() :
    fRef(nullptr),
    picture(new ByteVector),
    pictureModificated(false), pictureBChecked(false)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setTitle(tr("Add tags"));
    setCheckable(true);

    yearB = new QSpinBox;
    yearB->setRange(0, 32767);
    yearB->setSpecialValueText(tr("None"));

    trackB = new QSpinBox;
    trackB->setRange(0, 32767);
    trackB->setSpecialValueText(tr("None"));

    pictureB = new QGroupBox(tr("Cover"));
    pictureB->setCheckable(true);
    pictureW = new PictureW(*picture);
    loadImgB = new QPushButton(tr("Load cover picture"));
    loadImgB->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    saveImgB = new QPushButton(tr("Save cover picture"));
    saveImgB->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    connect(loadImgB, SIGNAL(clicked()), this, SLOT(loadImage()));
    connect(saveImgB, SIGNAL(clicked()), this, SLOT(saveImage()));

    QVBoxLayout *pictureLayout = new QVBoxLayout(pictureB);
    pictureLayout->addWidget(pictureW);
    pictureLayout->addWidget(loadImgB);
    pictureLayout->addWidget(saveImgB);

    QFormLayout *tagsLayout = new QFormLayout;
    tagsLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    tagsLayout->addRow(tr("Title") + ": ", titleE = new QLineEdit);
    tagsLayout->addRow(tr("Artist") + ": ", artistE = new QLineEdit);
    tagsLayout->addRow(tr("Album") + ": ", albumE = new QLineEdit);
    tagsLayout->addRow(tr("Comment") + ": ", commentE = new QLineEdit);
    tagsLayout->addRow(tr("Genre") + ": ", genreE = new QLineEdit);
    tagsLayout->addRow(tr("Year") + ": ", yearB);
    tagsLayout->addRow(tr("Track") + ": ", trackB);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addLayout(tagsLayout);
    layout->addWidget(pictureB);
}
TagEditor::~TagEditor()
{
    delete fRef;
    delete picture;
}

bool TagEditor::open(const QString &fileName)
{
    clear();
#ifdef Q_OS_WIN
    fRef = new FileRef((const wchar_t *)fileName.utf16(), false);
#else
    fRef = new FileRef(fileName.toLocal8Bit(), false);
#endif

    // TagLib can't load Ogg Opus file if file extension is ".ogg"
    if (fRef->isNull() && fileName.endsWith(".ogg", Qt::CaseInsensitive))
    {
#ifdef Q_OS_WIN
        auto file = new Ogg::Opus::File((const wchar_t *)fileName.utf16(), false);
#else
        auto file = new Ogg::Opus::File(fileName.toLocal8Bit(), false);
#endif
        if (file->isValid())
        {
            delete fRef;
            fRef = new FileRef(file);
        }
        else
        {
            delete file;
        }
    }

    if (!fRef->isNull() && fRef->tag())
    {
        File &file = *fRef->file();

        /* Copy ID3v2 to InfoTag */
        if (instanceOf(file, RIFF::WAV::File))
        {
            const Tag &tag = *fRef->tag();
            RIFF::Info::Tag &infoTag = *((RIFF::WAV::File &)file).InfoTag();
            if (infoTag.isEmpty() && !tag.isEmpty())
            {
                infoTag.setTitle(tag.title());
                infoTag.setArtist(tag.artist());
                infoTag.setAlbum(tag.album());
                infoTag.setComment(tag.comment());
                infoTag.setGenre(tag.genre());
                infoTag.setYear(tag.year());
                infoTag.setTrack(tag.track());
            }
        }

        const Tag &tag = getTag(*fRef, file);
        bool hasTags = !tag.isEmpty();
        setChecked(true);
        if (hasTags)
        {
            titleE->setText(tag.title().toCString(true));
            artistE->setText(tag.artist().toCString(true));
            albumE->setText(tag.album().toCString(true));
            commentE->setText(tag.comment().toCString(true));
            genreE->setText(tag.genre().toCString(true));
            yearB->setValue(tag.year());
            trackB->setValue(tag.track());
        }
        /* Covers */
        if (instanceOf(file, MPEG::File) || instanceOf(file, RIFF::AIFF::File))
        {
            pictureB->setEnabled(true);
            if (hasTags)
            {
                ID3v2::Tag *id3v2 = nullptr;
                if (instanceOf(file, MPEG::File))
                {
                    MPEG::File &mpegF = (MPEG::File &)file;
                    if (mpegF.hasID3v2Tag())
                        id3v2 = mpegF.ID3v2Tag();
                }
                else if (instanceOf(file, RIFF::AIFF::File))
                    id3v2 = ((RIFF::AIFF::File &)file).tag();
                if (id3v2)
                {
                    const ID3v2::FrameList &frameList = id3v2->frameList("APIC");
                    if (!frameList.isEmpty())
                    {
                        ID3v2::AttachedPictureFrame &pictureFrame = *(ID3v2::AttachedPictureFrame *)frameList.front();
                        pictureMimeType = pictureFrame.mimeType().toCString();
                        *picture = pictureFrame.picture();
                        pictureB->setChecked(true);
                        pictureW->update();
                    }
                }
            }
        }
        else if (instanceOf(file, FLAC::File))
        {
            pictureB->setEnabled(true);
            FLAC::File &flacF = (FLAC::File &)file;
            if (!flacF.pictureList().isEmpty())
            {
                FLAC::Picture &flacPicture = *flacF.pictureList().front();
                pictureMimeType = flacPicture.mimeType().toCString();
                *picture = flacPicture.data();
                pictureB->setChecked(true);
                pictureW->update();
                hasTags = true;
            }
        }
        else if (instanceOf(file, MP4::File))
        {
            MP4::ItemListMap &itemListMap = ((MP4::File &)file).tag()->itemListMap();
            MP4::ItemListMap::ConstIterator it = itemListMap.find("covr");
            pictureB->setEnabled(true);
            if (it != itemListMap.end())
            {
                MP4::CoverArtList coverArtList = it->second.toCoverArtList();
                if (!coverArtList.isEmpty())
                {
                    MP4::CoverArt coverArt = coverArtList.front();
                    switch (coverArt.format())
                    {
                        case MP4::CoverArt::JPEG:
                            pictureMimeType = "image/jpeg";
                            break;
                        case MP4::CoverArt::PNG:
                            pictureMimeType = "image/png";
                            break;
                        case MP4::CoverArt::BMP:
                            pictureMimeType = "image/bmp";
                            break;
                        case MP4::CoverArt::GIF:
                            pictureMimeType = "image/gif";
                            break;
                        default:
                            break;
                    }
                    if (!pictureMimeType.isEmpty())
                    {
                        *picture = coverArt.data();
                        pictureB->setChecked(true);
                        pictureW->update();
                        hasTags = true;
                    }
                }
            }
        }
        else if (isOgg(file))
        {
            Ogg::XiphComment *xiphComment = getXiphComment(file);
            if (xiphComment)
            {
                FLAC::Picture *flacPicture = nullptr;
                pictureB->setEnabled(true);
#if TAGLIB1B
                const List<FLAC::Picture *> pictures = xiphComment->pictureList();
                if (!pictures.isEmpty())
                {
                    flacPicture = pictures[0];
                    hasTags = true;
                }
#else
                const Ogg::FieldListMap &fieldListMap = xiphComment->fieldListMap();
                Ogg::FieldListMap::ConstIterator it = fieldListMap.find("METADATA_BLOCK_PICTURE");
                FLAC::Picture tmpFlacPicture;
                if (it != fieldListMap.end() && !it->second.isEmpty())
                {
                    /* OGG picture and FLAC picture are the same except OGG picture is encoded into Base64 */
                    const QByteArray pict_frame_decoded = QByteArray::fromBase64(it->second.front().toCString());
                    if (tmpFlacPicture.parse(ByteVector(pict_frame_decoded.data(), pict_frame_decoded.size())))
                        flacPicture = &tmpFlacPicture;
                }
#endif
                if (flacPicture)
                {
                    pictureMimeType = flacPicture->mimeType().toCString();
                    *picture = flacPicture->data();
                    pictureB->setChecked(true);
                    pictureW->update();
                }
            }
        }
        pictureBChecked = pictureB->isChecked();
        setChecked(hasTags);
        return true;
    }
    delete fRef;
    fRef = nullptr;
    return false;
}
void TagEditor::clear()
{
    if (fRef)
    {
        delete fRef;
        fRef = nullptr;
    }
    setChecked(false);
    clearValues();
}
bool TagEditor::save()
{
    if (fRef)
    {
        bool mustSave = false, result = false;

        if (!isChecked())
            clearValues();

        File &file = *fRef->file();

        Tag &tag = getTag(*fRef, file);
        if (titleE->text() != tag.title().toCString(true))
        {
            tag.setTitle(String(titleE->text().toUtf8().constData(), String::UTF8));
            mustSave = true;
        }
        if (artistE->text() != tag.artist().toCString(true))
        {
            tag.setArtist(String(artistE->text().toUtf8().constData(), String::UTF8));
            mustSave = true;
        }
        if (albumE->text() != tag.album().toCString(true))
        {
            tag.setAlbum(String(albumE->text().toUtf8().constData(), String::UTF8));
            mustSave = true;
        }
        if (commentE->text() != tag.comment().toCString(true))
        {
            tag.setComment(String(commentE->text().toUtf8().constData(), String::UTF8));
            mustSave = true;
        }
        if (genreE->text() != tag.genre().toCString(true))
        {
            tag.setGenre(String(genreE->text().toUtf8().constData(), String::UTF8));
            mustSave = true;
        }
        if ((uint)yearB->value() != tag.year())
        {
            tag.setYear(yearB->value());
            mustSave = true;
        }
        if ((uint)trackB->value() != tag.track())
        {
            tag.setTrack(trackB->value());
            mustSave = true;
        }

        if (isChecked() && (pictureModificated || pictureBChecked != pictureB->isChecked()))
        {
            const bool hasPicture = pictureB->isChecked() && !picture->isEmpty();
            if (instanceOf(file, MPEG::File) || instanceOf(file, RIFF::AIFF::File))
            {
                ID3v2::Tag *id3v2 = nullptr;
                if (instanceOf(file, MPEG::File))
                    id3v2 = ((MPEG::File &)file).ID3v2Tag(hasPicture);
                else if (instanceOf(file, RIFF::AIFF::File))
                    id3v2 = ((RIFF::AIFF::File &)file).tag();
                if (id3v2)
                {
                    id3v2->removeFrames("APIC");
                    if (hasPicture)
                    {
                        ID3v2::AttachedPictureFrame *pictureFrame = new ID3v2::AttachedPictureFrame;
                        pictureFrame->setType(ID3v2::AttachedPictureFrame::FrontCover);
                        pictureFrame->setMimeType(pictureMimeType.constData());
                        pictureFrame->setPicture(*picture);
                        id3v2->addFrame(pictureFrame);
                    }
                    mustSave = true;
                }
            }
            else if (instanceOf(file, FLAC::File))
            {
                FLAC::File &flacF = (FLAC::File &)file;
                flacF.removePictures();
                if (hasPicture)
                {
                    FLAC::Picture *flacPicture = new FLAC::Picture;
                    flacPicture->setMimeType(pictureMimeType.constData());
                    flacPicture->setType(FLAC::Picture::FrontCover);
                    flacPicture->setData(*picture);
                    flacF.addPicture(flacPicture);
                }
                mustSave = true;
            }
            else if (instanceOf(file, MP4::File))
            {
                MP4::ItemListMap &itemListMap = ((MP4::File &)file).tag()->itemListMap();
                if (itemListMap.contains("covr"))
                    itemListMap.erase("covr");
                if (hasPicture)
                {
                    MP4::CoverArt::Format format = (MP4::CoverArt::Format)0;
                    if (pictureMimeType == "image/jpeg")
                        format = MP4::CoverArt::JPEG;
                    else if (pictureMimeType == "image/png")
                        format = MP4::CoverArt::PNG;
                    else if (pictureMimeType == "image/bmp")
                        format = MP4::CoverArt::BMP;
                    else if (pictureMimeType == "image/gif")
                        format = MP4::CoverArt::GIF;
                    if (format)
                    {
                        MP4::CoverArtList coverArtList;
                        coverArtList.append(MP4::CoverArt(format, *picture));
                        itemListMap.insert("covr", coverArtList);
                    }
                }
                mustSave = true;
            }
            else if (isOgg(file))
            {
                Ogg::XiphComment *xiphComment = getXiphComment(file);
                if (xiphComment)
                {
                    FLAC::Picture *flacPicture = nullptr;
                    if (hasPicture)
                    {
                        flacPicture = new FLAC::Picture;
                        flacPicture->setMimeType(pictureMimeType.constData());
                        flacPicture->setType(FLAC::Picture::FrontCover);
                        flacPicture->setData(*picture);
                    }
#if TAGLIB1B
                    xiphComment->removeAllPictures();
                    if (flacPicture)
                        xiphComment->addPicture(flacPicture);
#else
                    xiphComment->removeField("METADATA_BLOCK_PICTURE");
                    if (flacPicture)
                    {
                        const ByteVector pict_data = flacPicture->render();
                        xiphComment->addField("METADATA_BLOCK_PICTURE", QByteArray::fromRawData(pict_data.data(), pict_data.size()).toBase64().data());
                        delete flacPicture;
                    }
#endif
                    mustSave = true;
                }
            }
        }
        else if (!isChecked()) //Usuwanie wszystkich znanych tagów
        {
            mustSave = true;

            if (instanceOf(file, MPEG::File))
                ((MPEG::File &)file).strip();
            else if (instanceOf(file, MPC::File))
                ((MPC::File &)file).strip();
            else if (instanceOf(file, WavPack::File))
                ((WavPack::File &)file).strip();
            else if (instanceOf(file, TrueAudio::File))
                ((TrueAudio::File &)file).strip();
            else if (instanceOf(file, APE::File))
                ((APE::File &)file).strip();
            else if (instanceOf(file, MP4::File))
                ((MP4::File &)file).tag()->itemListMap().clear();
            else if (instanceOf(file, ASF::File))
                ((ASF::File &)file).tag()->attributeListMap().clear();
            else if (isOgg(file))
                removeXiphComment(getXiphComment(file));
            else if (instanceOf(file, FLAC::File))
            {
                FLAC::File &flacF = (FLAC::File &)file;
                flacF.removePictures();
                if (flacF.hasXiphComment())
                    removeXiphComment(flacF.xiphComment());
            }
            else if (instanceOf(file, RIFF::AIFF::File))
            {
                ID3v2::Tag *id3v2 = ((RIFF::AIFF::File &)file).tag();
                if (id3v2)
                {
                    ID3v2::FrameList frameList = id3v2->frameList();
                    for (ID3v2::FrameList::ConstIterator it = frameList.begin(); it != frameList.end(); ++it)
                        id3v2->removeFrame(*it);
                }
            }
            else if (instanceOf(file, Mod::File) || instanceOf(file, S3M::File) || instanceOf(file, IT::File) || instanceOf(file, XM::File))
            {
                Mod::Tag *modTag = nullptr;
                if (instanceOf(file, Mod::File))
                    modTag = ((Mod::File &)file).tag();
                else if (instanceOf(file, S3M::File))
                    modTag = ((S3M::File &)file).tag();
                else if (instanceOf(file, IT::File))
                    modTag = ((IT::File &)file).tag();
                else if (instanceOf(file, XM::File))
                    modTag = ((XM::File &)file).tag();
                if (modTag)
                    modTag->setTrackerName(String::null);
            }
        }

        /* Remove ID3 tags from FLAC::File */
        if (mustSave && instanceOf(file, FLAC::File))
        {
            FLAC::File &flacF = (FLAC::File &)file;
            if (flacF.hasID3v1Tag() || flacF.hasID3v2Tag())
            {
#if TAGLIB1B
                flacF.strip(FLAC::File::ID3v1 | FLAC::File::ID3v2);
#else
    #ifdef Q_OS_WIN
                const FileName fName = file.name(); //Class with "std::string"
    #else
                const QByteArray fName = file.name(); //Raw pointer, so copy it
    #endif
                result = fRef->save();
                delete fRef;
                fRef = nullptr;
                if (result)
                    result = MPEG::File(fName, false).save(MPEG::File::NoTags);
                mustSave = false;
#endif
            }
        }

        /* No ID3v2 in WAV, only InfoTag */
        if (mustSave && instanceOf(file, RIFF::WAV::File))
        {
            RIFF::WAV::File &wavF = (RIFF::WAV::File &)file;
            wavF.save(wavF.InfoTag()->isEmpty() ? RIFF::WAV::File::NoTags : RIFF::WAV::File::Info);
            mustSave = false;
        }

        return mustSave ? fRef->save() : (fRef ? true : result);
    }
    return false;
}

void TagEditor::loadImage()
{
    const QString filePath = QFileDialog::getOpenFileName(this, tr("Loading cover picture"), QMPlay2GUI.getCurrentPth(), tr("Pictures") + " (*.jpg *.jpeg *.png *.gif *.bmp)");
    if (!filePath.isEmpty())
    {
        QFile f(filePath);
        if (f.open(QFile::ReadOnly))
        {
            const QByteArray fmt = QImageReader::imageFormat(&f);
            if (fmt == "jpeg" || fmt == "png" || fmt == "gif" || fmt == "bmp")
            {
                picture->setData(f.readAll().constData(), f.size());
                pictureMimeType = "image/" + fmt;
                pictureModificated = true;
                pictureW->update();
            }
        }
    }
}
void TagEditor::saveImage()
{
    QMPlay2GUI.saveCover(QByteArray::fromRawData(picture->data(), picture->size()));
}

void TagEditor::clearValues()
{
    pictureModificated = pictureBChecked = false;
    pictureMimeType.clear();
    picture->clear();
    titleE->clear();
    artistE->clear();
    albumE->clear();
    commentE->clear();
    genreE->clear();
    yearB->setValue(0);
    trackB->setValue(0);
    pictureB->setChecked(false);
    pictureB->setEnabled(false);
}
