#include <VideoWriter.hpp>

#include <QCoreApplication>

class OpenGL2Common;

class OpenGL2Writer : public VideoWriter
{
	Q_DECLARE_TR_FUNCTIONS(OpenGL2Writer)
public:
	OpenGL2Writer(Module &);
private:
	~OpenGL2Writer();

	bool set();

	bool readyWrite() const;

	bool processParams(bool *paramsCorrected);

	QMPlay2PixelFormats supportedPixelFormats() const;

	void writeVideo(const VideoFrame &videoFrame);
	void writeOSD(const QList<const QMPlay2_OSD *> &);

	void pause();

	QString name() const;

	bool open();

	/**/

	OpenGL2Common *drawable;
#ifdef OPENGL_NEW_API
	bool forceRtt, useRtt;
#endif
#ifdef VSYNC_SETTINGS
	bool vSync;
#endif
};

#define OpenGL2WriterName "OpenGL 2"
