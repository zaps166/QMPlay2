#include <OpenGLWriter.hpp>

#include <QMPlay2_OSD.hpp>
#include <Functions.hpp>
using Functions::getImageSize;
using Functions::aligned;

#include <QGLShaderProgram>
#include <QDebug>

#include <GL/glext.h>
#ifndef GL_TEXTURE_RECTANGLE_ARB
	#define GL_TEXTURE_RECTANGLE_ARB 0x84F5
#endif

static inline void vertex( int v, int flip )
{
	if ( flip & Qt::Horizontal )
	{
		if ( v & 1 )
			--v;
		else
			++v;
	}
	if ( flip & Qt::Vertical )
		v = 3 - v;
	switch ( v )
	{
		case 0:
			glVertex2i( -1,  1 );
			break;
		case 1:
			glVertex2i(  1,  1 );
			break;
		case 2:
			glVertex2i(  1, -1 );
			break;
		case 3:
			glVertex2i( -1, -1 );
			break;
	}
}

#ifdef QtVSync
Drawable::Drawable( OpenGLWriter &writer, const QGLFormat &format ) :
	QGLWidget( format ),
#else
Drawable::Drawable( OpenGLWriter &writer ) :
#endif
	videoFrame( NULL ),
	writer( writer ),
	program( NULL ),
	noShaders( false ), hasImage( false )
{
	setMouseTracking( true );
}
Drawable::~Drawable()
{
	clr();
	delete program;
}

void Drawable::clr()
{
	hasImage = false;
	imgScaler.destroy();

	osdImg = QImage();
	osd_checksums.clear();
}

void Drawable::resizeEvent( QResizeEvent *e )
{
	getImageSize( writer.aspect_ratio, writer.zoom, width(), height(), W, H, &X, &Y );
	if ( !e )
		updateGL();
	else
		QGLWidget::resizeEvent( e );
}

#ifndef QtVSync
void Drawable::VSync( bool vsync )
{
#if defined Q_WS_X11 || defined Q_OS_LINUX || defined Q_OS_FREEBSD
	typedef int ( *_glXSwapIntervalSGI )( int interval );
	_glXSwapIntervalSGI glXSwapIntervalSGI = ( _glXSwapIntervalSGI )context()->getProcAddress( "glXSwapIntervalSGI" );
	if ( glXSwapIntervalSGI )
		glXSwapIntervalSGI( vsync );
	else
#elif defined Q_OS_WIN
	typedef BOOL ( APIENTRY *_wglSwapIntervalEXT )( int interval );
	_wglSwapIntervalEXT wglSwapIntervalEXT = ( _wglSwapIntervalEXT )context()->getProcAddress( "wglSwapIntervalEXT" );
	if ( wglSwapIntervalEXT )
		wglSwapIntervalEXT( vsync );
	else
#endif
		QMPlay2Core.logError( "OpenGL :: " + tr( "Nie można ustawić VSync" ) );
	lastVSyncState = vsync;
}
#endif

void Drawable::initializeGL()
{
#ifdef QtVSync
	if ( format().swapInterval() != writer.VSync )
		QMPlay2Core.logError( "OpenGL :: " + tr( "Nie można ustawić VSync" ) );
#else
	VSync( writer.VSync );
#endif

	glEnable( GL_TEXTURE_2D );
	glClearColor( 0.0, 0.0, 0.0, 0.0 );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	QString error;
	if ( !QGLShader::hasOpenGLShaders( QGLShader::Fragment ) )
		error = tr( "Sterownik nie obsługuje fragment shader (pixel shader)" ) + ". ";
	else if ( writer.useShaders )
	{
		const bool hasGL_ARB_texture_rectangle = QString( ( char * )glGetString( GL_EXTENSIONS ) ).contains( "GL_ARB_texture_rectangle" );
		glActiveTexture = ( _glActiveTexture )context()->getProcAddress( "glActiveTexture" );
		if ( glActiveTexture && hasGL_ARB_texture_rectangle )
		{
			if ( !program )
			{
				const char *shaderHUE = !writer.useHUE ? "" : //Workaround because of BUG in Mesa i915 driver
				"if ( Hue != 0.0 ) {"
					"float HUE = atan( YCbCr[2], YCbCr[1] ) + Hue;"
					"float CHROMA = sqrt( YCbCr[1] * YCbCr[1] + YCbCr[2] * YCbCr[2] );"
					"YCbCr[1] = CHROMA * cos( HUE );"
					"YCbCr[2] = CHROMA * sin( HUE );"
				"}";
				const QString FProgram = QString(
				"#extension GL_ARB_texture_rectangle : enable\n"
				"uniform float Brightness, Contrast, Saturation, Hue;"
				"uniform sampler2DRect Ytex, Utex, Vtex;"
				"void main() {"
					"vec3 YCbCr = vec3("
						"texture2DRect( Ytex, vec2( gl_TexCoord[0].xy       ) )[0] - 0.0625,"
						"texture2DRect( Utex, vec2( gl_TexCoord[0].xy / 2.0 ) )[0] - 0.5,"
						"texture2DRect( Vtex, vec2( gl_TexCoord[0].xy / 2.0 ) )[0] - 0.5"
					");"
					"%1"
					"YCbCr.yz *= Saturation;"
					"vec3 RGB = mat3( 1.1643, 1.1643, 1.1643, 0.0, -0.39173, 2.017, 1.5958, -0.8129, 0.0 ) * YCbCr * Contrast + Brightness;"
					"gl_FragColor = vec4( RGB, 1.0 );"
				"}" ).arg( shaderHUE );

				program = new QGLShaderProgram( this );
				program->addShaderFromSourceCode( QGLShader::Fragment, FProgram );
			}
			if ( program->bind() )
			{
				program->setUniformValue( "Ytex", 0 );
				program->setUniformValue( "Utex", 1 );
				program->setUniformValue( "Vtex", 2 );
				program->release();
			}
			else
			{
				program->release();
				delete program;
				program = NULL;
			}
		}

		if ( !glActiveTexture )
			error += tr( "Nie można odnaleźć funkcji" ) + ": \"glActiveTexture\". ";
		if ( !hasGL_ARB_texture_rectangle )
			error += tr( "Sterownik grafiki nie obsługuje" ) + ": \"GL_ARB_texture_rectangle\". ";
		if ( glActiveTexture && hasGL_ARB_texture_rectangle && !program )
			error += tr( "Problem z kompilacją fragment shader" ) + ". ";
	}

	if ( !writer.useShaders || !error.isEmpty() )
	{
		if ( writer.useShaders )
			QMPlay2Core.logError( "OpenGL :: " + error + tr( "Obraz nie będzie konwertowany do RGB poprzez GPU" ) );
		else
			QMPlay2Core.logError( "OpenGL :: " + tr( "Shadery wyłączone w opcjach" ) + ". " + tr( "Obraz nie będzie konwertowany do RGB poprzez GPU" ) );
		noShaders = true;
	}
}
void Drawable::resizeGL( int w, int h )
{
	glViewport( 0, 0, w, h );
}
void Drawable::paintGL()
{
#ifndef QtVSync
	if ( lastVSyncState != writer.VSync )
		VSync( writer.VSync );
#endif

	glClear( GL_COLOR_BUFFER_BIT );

	if ( !videoFrame && !hasImage )
		return;

	if ( videoFrame )
		hasImage = true;

	glPushMatrix();
	glScalef( W / ( float )width(), H / ( float )height(), 1.0f );
	if ( !noShaders )
	{
		program->bind();
		if ( setVideoEQ )
		{
			program->setUniformValue( "Contrast", Contrast );
			program->setUniformValue( "Saturation", Saturation );
			program->setUniformValue( "Brightness", Brightness );
			program->setUniformValue( "Hue", Hue );
			setVideoEQ = false;
		}
		if ( videoFrame )
		{
			/* Select texture unit 1 as the active unit and bind the U texture. */
			glActiveTexture( GL_TEXTURE1 );
			glBindTexture( GL_TEXTURE_RECTANGLE_ARB, 1 );
			glTexImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, GL_LUMINANCE, videoFrame->linesize[ 1 ], writer.outH >> 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, videoFrame->data[ 1 ] );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

			/* Select texture unit 2 as the active unit and bind the V texture. */
			glActiveTexture( GL_TEXTURE2 );
			glBindTexture( GL_TEXTURE_RECTANGLE_ARB, 2 );
			glTexImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, GL_LUMINANCE, videoFrame->linesize[ 2 ], writer.outH >> 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, videoFrame->data[ 2 ] );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

			/* Select texture unit 0 as the active unit and bind the Y texture. */
			glActiveTexture( GL_TEXTURE0 );
			glBindTexture( GL_TEXTURE_RECTANGLE_ARB, 3 );
			glTexImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, GL_LUMINANCE, videoFrame->linesize[ 0 ], writer.outH, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, videoFrame->data[ 0 ] );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		}
		glBegin( GL_QUADS ); {
			glTexCoord2i( 0,        0           ); vertex( 0, writer.flip );
			glTexCoord2i( writer.W, 0           ); vertex( 1, writer.flip );
			glTexCoord2i( writer.W, writer.outH ); vertex( 2, writer.flip );
			glTexCoord2i( 0,        writer.outH ); vertex( 3, writer.flip );
		} glEnd();
		program->release();
	}
	else
	{
		glBindTexture( GL_TEXTURE_2D, 4 );
		if ( videoFrame )
		{
			const int aligned8W = aligned( writer.outW, 8 );
			if ( !imgScaler.array() )
				imgScaler.createArray( aligned8W * writer.outH << 2 );
			if ( imgScaler.create( writer.outW, writer.outH, aligned8W, writer.outH ) )
			{
				imgScaler.scale( videoFrame );
				if ( Contrast != 1.0f || Brightness != 0.0f )
					Functions::ImageEQ( Contrast * 100, Brightness * 256, ( quint8 * )imgScaler.array(), aligned8W * writer.outH << 2 );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
				glTexImage2D( GL_TEXTURE_2D, 0, 4, aligned8W, writer.outH, 0, GL_BGRA, GL_UNSIGNED_BYTE, imgScaler.array() );
			}
		}
		glBegin( GL_QUADS ); {
			glTexCoord2i( -1, -1 ); vertex( 0, writer.flip );
			glTexCoord2i(  0, -1 ); vertex( 1, writer.flip );
			glTexCoord2i(  0,  0 ); vertex( 2, writer.flip );
			glTexCoord2i( -1,  0 ); vertex( 3, writer.flip );
		} glEnd();
	}
	glPopMatrix();

	/* OSD */
	osd_mutex.lock();
	if ( !osd_list.isEmpty() )
	{
		glPushMatrix();
		glOrtho( 0, width(), height(), 0, -1, 1 );
		glTranslatef( X, Y, 0 );
		glBindTexture( GL_TEXTURE_2D, 5 );
		glEnable( GL_BLEND );

		QRect bounds;
		const qreal scaleW = ( qreal )W / writer.outW, scaleH = ( qreal )H / writer.outH;
		bool mustRepaint = Functions::mustRepaintOSD( osd_list, osd_checksums, &scaleW, &scaleH, &bounds );
		if ( !mustRepaint )
			mustRepaint = osdImg.size() != bounds.size();
		if ( mustRepaint )
		{
			if ( osdImg.size() != bounds.size() )
				osdImg = QImage( bounds.size(), QImage::Format_ARGB32 );
			osdImg.fill( 0 );
			QPainter p( &osdImg );
			p.translate( -bounds.topLeft() );
			Functions::paintOSD( osd_list, scaleW, scaleH, p, &osd_checksums );
			glTexImage2D( GL_TEXTURE_2D, 0, 4, bounds.width(), bounds.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, osdImg.bits() );
		}
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		glBegin( GL_QUADS ); {
			glTexCoord2i( -1, -1 ); glVertex2i( bounds.left(),      bounds.top()        );
			glTexCoord2i(  0, -1 ); glVertex2i( bounds.right() + 1, bounds.top()        );
			glTexCoord2i(  0,  0 ); glVertex2i( bounds.right() + 1, bounds.bottom() + 1 );
			glTexCoord2i( -1,  0 ); glVertex2i( bounds.left(),      bounds.bottom() + 1 );
		} glEnd();
#if 0
		foreach ( const QMPlay2_OSD *osd, osd_list )
		{
			osd->lock();
			if ( osd->needsRescale() )
			{
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
				glPushMatrix();
				glScalef( ( float )W / writer.outW, ( float )H / writer.outH, 1.0f );
			}
			else
			{
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
			}
			for ( int j = 0 ; j < osd->imageCount() ; j++ )
			{
				const QMPlay2_OSD::Image &img = osd->getImage( j );
				glTexImage2D( GL_TEXTURE_2D, 0, 4, img.rect.width(), img.rect.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, ( uchar * )img.data.data() );
				glBegin( GL_QUADS ); {
					glTexCoord2i( -1, -1 ); glVertex2i( img.rect.left(),      img.rect.top()        );
					glTexCoord2i(  0, -1 ); glVertex2i( img.rect.right() + 1, img.rect.top()        );
					glTexCoord2i(  0,  0 ); glVertex2i( img.rect.right() + 1, img.rect.bottom() + 1 );
					glTexCoord2i( -1,  0 ); glVertex2i( img.rect.left(),      img.rect.bottom() + 1 );
				} glEnd();
			}
			if ( osd->needsRescale() )
				glPopMatrix();
			osd->unlock();
		}
#endif
		glDisable( GL_BLEND );
		glPopMatrix();
	}
	osd_mutex.unlock();
}

/**/

OpenGLWriter::OpenGLWriter( Module &module )
{
	addParam( "W" );
	addParam( "H" );
	addParam( "AspectRatio" );
	addParam( "Zoom" );
	addParam( "Flip" );
	addParam( "Saturation" );
	addParam( "Brightness" );
	addParam( "Contrast" );
	addParam( "Hue" );

	drawable = NULL;
	W = outW = outH = -1;

	SetModule( module );
}

OpenGLWriter::~OpenGLWriter()
{
	delete drawable;
}

bool OpenGLWriter::set()
{
	bool restartPlaying = false;

	bool _VSync = sets().getBool( "VSync" );
	bool _useShaders = sets().getBool( "Use_shaders" );

	if ( _VSync != VSync )
	{
		VSync = _VSync;
#ifdef QtVSync
		restartPlaying = true;
#endif
	}
	if ( _useShaders != useShaders )
	{
		useShaders = _useShaders;
		restartPlaying = true;
	}

	return !restartPlaying && sets().getBool( "Enabled" );
}

bool OpenGLWriter::readyWrite() const
{
	return drawable;
}

bool OpenGLWriter::processParams( bool * )
{
	bool doResizeEvent = false;

	const double _aspect_ratio = getParam( "AspectRatio" ).toDouble();
	const double _zoom = getParam( "Zoom" ).toDouble();
	const int _flip = getParam( "Flip" ).toInt();
	const float Contrast = ( getParam( "Contrast" ).toInt() + 100 ) / 100.0f;
	const float Saturation = ( getParam( "Saturation" ).toInt() + 100 ) / 100.0f;
	const float Brightness = getParam( "Brightness" ).toInt() / 100.0f;
	const float Hue = getParam( "Hue" ).toInt() / -31.831f;
	if ( _aspect_ratio != aspect_ratio || _zoom != zoom || _flip != flip || drawable->Contrast != Contrast || drawable->Brightness != Brightness || drawable->Saturation != Saturation || drawable->Hue != Hue )
	{
		zoom = _zoom;
		aspect_ratio = _aspect_ratio;
		flip = _flip;
		drawable->Contrast = Contrast;
		drawable->Brightness = Brightness;
		drawable->Saturation = Saturation;
		drawable->Hue = Hue;
		doResizeEvent = drawable->isVisible();
		drawable->setVideoEQ = true;
	}

	const int _outW = getParam( "W" ).toInt();
	const int _outH = getParam( "H" ).toInt();
	if ( _outW > 0 && _outH > 0 && ( _outW != outW || _outH != outH ) )
	{
		outW = _outW;
		outH = _outH;
		W = ( outW % 8 ) ? outW-1 : outW;

		drawable->clr();
		QMPlay2Core.dockVideo( drawable );
	}

	if ( doResizeEvent )
		drawable->resizeEvent( NULL );

	return readyWrite();
}

qint64 OpenGLWriter::write( const QByteArray &arr )
{
	drawable->videoFrame = VideoFrame::fromData( arr );
	drawable->updateGL();
	drawable->videoFrame = NULL;
	VideoFrame::unref( arr );
	return arr.size();
}
void OpenGLWriter::writeOSD( const QList< const QMPlay2_OSD * > &osds )
{
	drawable->osd_mutex.lock();
	drawable->osd_list = osds;
	drawable->osd_mutex.unlock();
}

QString OpenGLWriter::name() const
{
	return OpenGLWriterName;
}

bool OpenGLWriter::open()
{
	if ( QGLFormat::openGLVersionFlags() == QGLFormat::OpenGL_Version_None )
		return false;
	useHUE = QGLFormat::openGLVersionFlags() >= QGLFormat::OpenGL_Version_3_0;
#ifdef QtVSync
	QGLFormat fmt;
	fmt.setSwapInterval( VSync );
	drawable = new Drawable( *this, fmt );
#else
	drawable = new Drawable( *this );
#endif
	if ( !drawable->context()->isValid() )
		return false;
	drawable->makeCurrent();
	const char *extensions_str = ( const char * )glGetString( GL_EXTENSIONS );
	bool isOK = extensions_str && strstr( extensions_str, "GL_ARB_texture_non_power_of_two" );
	if ( !isOK )
		QMPlay2Core.logError( "OpenGL :: " + tr( "Nie można tworzyć tekstur o rozmiarach innych niż potęga liczby 2, OpenGL wyłączony..." ), true, false );
	return isOK;
}
