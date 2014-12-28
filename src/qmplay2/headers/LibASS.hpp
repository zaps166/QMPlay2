#ifndef LIBASS_HPP
#define LIBASS_HPP

#include <QByteArray>
#include <QList>

class Settings;
class QMPlay2_OSD;
struct ass_style;
struct ass_library;
struct ass_track;
struct ass_event;
struct ass_renderer;

class LibASS
{
public:
	LibASS( Settings & );
	~LibASS();

	void setWindowSize( int, int );
	void setARatio( double );
	void setZoom( double );
	void setFontScale( double );

	void addFont( const QByteArray &name, const QByteArray &data );
	void clearFonts();

	void initOSD();
	void setOSDStyle();
	bool getOSD( QMPlay2_OSD *&, const QByteArray &, double );
	void closeOSD();

	void initASS( const QByteArray &header = QByteArray() );
	inline bool isASS()
	{
		return hasASSData && ass_sub_track && ass_sub_renderer;
	}
	void setASSStyle();
	void addASSEvent( const QByteArray & );
	void addASSEvent( const QByteArray &, double, double );
	void flushASSEvents();
	bool getASS( QMPlay2_OSD *&, double );
	void closeASS();
private:
	void readStyle( const QString &, ass_style * );
	void calcSize();

	Settings &settings;

	ass_library *ass;
	int W, H, winW, winH;
	double zoom, aspect_ratio, fontScale;

	//OSD
	ass_track *osd_track;
	ass_style *osd_style;
	ass_event *osd_event;
	ass_renderer *osd_renderer;

	//ASS subtitles
	ass_track *ass_sub_track;
	ass_renderer *ass_sub_renderer;
	QList< ass_style * > ass_sub_styles_copy;
	bool hasASSData, overridePlayRes;
};

#endif //LIBASS_HPP
