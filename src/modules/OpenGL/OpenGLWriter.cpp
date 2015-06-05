#include <OpenGLWriter.hpp>

#include <QMPlay2_OSD.hpp>
#include <Functions.hpp>
using Functions::getImageSize;
using Functions::aligned;

#include <QGLShaderProgram>

#include <GL/glext.h>

static inline unsigned getPowerOf2( unsigned n )
{
	if ( !( n & ( n - 1 ) ) )
		return n;
	while ( n & ( n - 1 ) )
		n = n & ( n-1 );
	return n << 1;
}
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
	tex_w( 1.0f ),
	maxTextureSize( 0 ),
	noShaders( false ), hasImage( false )
{
	grabGesture( Qt::PinchGesture );
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
void Drawable::VSync()
{
#if defined Q_WS_X11 || defined Q_OS_LINUX || defined Q_OS_FREEBSD
	typedef int ( *GLXSwapIntervalSGI )( int interval );
	GLXSwapIntervalSGI glXSwapIntervalSGI = ( GLXSwapIntervalSGI )context()->getProcAddress( "glXSwapIntervalSGI" );
	if ( glXSwapIntervalSGI )
		glXSwapIntervalSGI( writer.VSync );
#elif defined Q_OS_WIN
	typedef BOOL ( APIENTRY *WGLSwapIntervalEXT )( int interval );
	WGLSwapIntervalEXT wglSwapIntervalEXT = ( WGLSwapIntervalEXT )context()->getProcAddress( "wglSwapIntervalEXT" );
	if ( wglSwapIntervalEXT )
		wglSwapIntervalEXT( writer.VSync );
#endif
	lastVSyncState = writer.VSync;
}
#endif

void Drawable::initializeGL()
{
#ifndef QtVSync
	VSync();
#endif

	bool useHUE = format().openGLVersionFlags() >= QGLFormat::OpenGL_Version_3_0 || !strstr( ( const char * )glGetString( GL_RENDERER ), "Intel" );
	canCreateTexturesNonPowerOf2 = !!strstr( ( const char * )glGetString( GL_EXTENSIONS ), "GL_ARB_texture_non_power_of_two" );

	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glClearColor( 0.0, 0.0, 0.0, 0.0 );
	glEnable( GL_TEXTURE_2D );
	glDisable( GL_DITHER );

	QString error;
	if ( !canCreateTexturesNonPowerOf2 )
	{
		glGetIntegerv( GL_MAX_TEXTURE_SIZE, &maxTextureSize );
		error = tr( "Sterownik grafiki nie obsługuje" ) + "\"GL_ARB_texture_non_power_of_two\". " + tr( "Wydajność będzie niska" ) + ". ";
	}
	else if ( !QGLShader::hasOpenGLShaders( QGLShader::Fragment ) )
		error = tr( "Sterownik nie obsługuje fragment shader (pixel shader)" ) + ". ";
	else if ( writer.useShaders )
	{
		glActiveTexture = ( GLActiveTexture )context()->getProcAddress( "glActiveTexture" );
		if ( glActiveTexture )
		{
			if ( !program )
			{
				const char *shaderHUE = !useHUE ? "" : //Workaround because of BUG in Mesa i915 driver
				"if ( Hue != 0.0 ) {"
					"float HUE = atan( YCbCr[2], YCbCr[1] ) + Hue;"
					"float CHROMA = sqrt( YCbCr[1] * YCbCr[1] + YCbCr[2] * YCbCr[2] );"
					"YCbCr[1] = CHROMA * cos( HUE );"
					"YCbCr[2] = CHROMA * sin( HUE );"
				"}";
				const QString FProgram = QString(
				"uniform float Brightness, Contrast, Saturation, Hue;"
				"uniform sampler2D Ytex, Utex, Vtex;"
				"void main() {"
					"vec3 YCbCr = vec3("
						"texture2D( Ytex, vec2( gl_TexCoord[0].st ) )[0] - 0.0625,"
						"texture2D( Utex, vec2( gl_TexCoord[0].st ) )[0] - 0.5,"
						"texture2D( Vtex, vec2( gl_TexCoord[0].st ) )[0] - 0.5"
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
		if ( glActiveTexture && !program )
			error += tr( "Problem z kompilacją fragment shader" ) + ". ";
	}

	if ( !writer.useShaders || !error.isEmpty() )
	{
		if ( writer.useShaders )
			QMPlay2Core.logError( "OpenGL :: " + error + tr( "Obraz nie będzie konwertowany do RGB poprzez GPU" ), true, true );
		else
			QMPlay2Core.logError( "OpenGL :: " + tr( "Shadery wyłączone w opcjach" ) + ". " + tr( "Obraz nie będzie konwertowany do RGB poprzez GPU" ), true, true );
		noShaders = true;
	}

	glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
	for ( int i = 1 ; i <= ( noShaders ? 2 : 4 ) ; ++i )
	{
		glBindTexture( GL_TEXTURE_2D, i );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, i == 1 ? GL_NEAREST : GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, i == 1 ? GL_NEAREST : GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
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
		VSync();
#endif

	glClear( GL_COLOR_BUFFER_BIT );

	if ( !videoFrame && !hasImage )
		return;

	if ( videoFrame )
		hasImage = true;

	glLoadIdentity();
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
		if ( !videoFrame )
			glBindTexture( GL_TEXTURE_2D, 4 );
		else
		{
			/* Select texture unit 1 as the active unit and bind the U texture. */
			glActiveTexture( GL_TEXTURE1 );
			glBindTexture( GL_TEXTURE_2D, 2 );
			glTexImage2D( GL_TEXTURE_2D, 0, GL_LUMINANCE, videoFrame->linesize[ 1 ], writer.outH >> 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, videoFrame->data[ 1 ] );

			/* Select texture unit 2 as the active unit and bind the V texture. */
			glActiveTexture( GL_TEXTURE2 );
			glBindTexture( GL_TEXTURE_2D, 3 );
			glTexImage2D( GL_TEXTURE_2D, 0, GL_LUMINANCE, videoFrame->linesize[ 2 ], writer.outH >> 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, videoFrame->data[ 2 ] );

			/* Select texture unit 0 as the active unit and bind the Y texture. */
			glActiveTexture( GL_TEXTURE0 );
			glBindTexture( GL_TEXTURE_2D, 4 );
			glTexImage2D( GL_TEXTURE_2D, 0, GL_LUMINANCE, videoFrame->linesize[ 0 ], writer.outH, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, videoFrame->data[ 0 ] );

			if ( writer.outW == videoFrame->linesize[ 0 ] )
				tex_w = 1.0f;
			else
				tex_w = writer.outW / ( videoFrame->linesize[ 0 ] + 1.0f );
		}
		glBegin( GL_QUADS ); {
			glTexCoord2f( 0.0f,  0.0f  ); vertex( 0, writer.flip );
			glTexCoord2f( tex_w, 0.0f  ); vertex( 1, writer.flip );
			glTexCoord2f( tex_w, tex_w ); vertex( 2, writer.flip );
			glTexCoord2f( 0.0f,  tex_w ); vertex( 3, writer.flip );
		} glEnd();
		program->release();
	}
	else
	{
		glBindTexture( GL_TEXTURE_2D, 2 );
		if ( videoFrame )
		{
			if ( canCreateTexturesNonPowerOf2 )
			{
				const int aligned8W = aligned( writer.outW, 8 );
				if ( !imgScaler.array() )
					imgScaler.createArray( aligned8W * writer.outH << 2 );
				if ( imgScaler.create( writer.outW, writer.outH, aligned8W, writer.outH ) )
				{
					imgScaler.scale( videoFrame );
					if ( Contrast != 1.0f || Brightness != 0.0f )
						Functions::ImageEQ( Contrast * 100, Brightness * 256, ( quint8 * )imgScaler.array(), aligned8W * writer.outH << 2 );
					glTexImage2D( GL_TEXTURE_2D, 0, 4, aligned8W, writer.outH, 0, GL_BGRA, GL_UNSIGNED_BYTE, imgScaler.array() );
				}
			}
			else
			{
				int w2 = getPowerOf2( writer.outW ), h2 = ( writer.outH > 1024 && writer.outH < 1100 ) ? 1024 : getPowerOf2( writer.outH );
				if ( w2 > maxTextureSize )
					w2 = maxTextureSize;
				if ( h2 > maxTextureSize )
					h2 = maxTextureSize;
				if ( !imgScaler.array() )
					imgScaler.createArray( w2 * h2 << 2 );
				if ( imgScaler.create( writer.outW, writer.outH, w2, h2 ) )
				{
					imgScaler.scale( videoFrame );
					if ( Contrast != 1.0f || Brightness != 0.0f )
						Functions::ImageEQ( Contrast * 100, Brightness * 256, ( quint8 * )imgScaler.array(), w2 * h2 << 2 );
					glTexImage2D( GL_TEXTURE_2D, 0, 4, w2, h2, 0, GL_BGRA, GL_UNSIGNED_BYTE, imgScaler.array() );
				}
			}
		}
		glBegin( GL_QUADS ); {
			glTexCoord2i( 0, 0 ); vertex( 0, writer.flip );
			glTexCoord2i( 1, 0 ); vertex( 1, writer.flip );
			glTexCoord2i( 1, 1 ); vertex( 2, writer.flip );
			glTexCoord2i( 0, 1 ); vertex( 3, writer.flip );
		} glEnd();
	}

	/* OSD */
	osd_mutex.lock();
	if ( !osd_list.isEmpty() )
	{
		glBindTexture( GL_TEXTURE_2D, 1 );

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
			if ( canCreateTexturesNonPowerOf2 )
				glTexImage2D( GL_TEXTURE_2D, 0, 4, bounds.width(), bounds.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, osdImg.bits() );
			else
			{
				int w2 = getPowerOf2( bounds.width() ), h2 = getPowerOf2( bounds.height() );
				if ( w2 > maxTextureSize )
					w2 = maxTextureSize;
				if ( h2 > maxTextureSize )
					h2 = maxTextureSize;
				glTexImage2D( GL_TEXTURE_2D, 0, 4, w2, h2, 0, GL_BGRA, GL_UNSIGNED_BYTE, osdImg.scaled( w2, h2, Qt::IgnoreAspectRatio, Qt::FastTransformation ).bits() );
			}
		}

		glLoadIdentity();
		glOrtho( 0, width(), height(), 0, -1, 1 );
		glTranslatef( X, Y, 0 );
		glEnable( GL_BLEND );
		glBegin( GL_QUADS ); {
			glTexCoord2i( 0, 0 ); glVertex2i( bounds.left(),      bounds.top()        );
			glTexCoord2i( 1, 0 ); glVertex2i( bounds.right() + 1, bounds.top()        );
			glTexCoord2i( 1, 1 ); glVertex2i( bounds.right() + 1, bounds.bottom() + 1 );
			glTexCoord2i( 0, 1 ); glVertex2i( bounds.left(),      bounds.bottom() + 1 );
		} glEnd();
		glDisable( GL_BLEND );
	}
	osd_mutex.unlock();
}

bool Drawable::event( QEvent *e )
{
	/*
	 * QGLWidget blocks this event forever (tested on Windows 8.1, Qt 4.8.7)
	 * This is workaround: pass gesture event to the parent.
	*/
	if ( e->type() == QEvent::Gesture )
		return qApp->notify( parent(), e );
	return QGLWidget::event( e );
}

/**/

OpenGLWriter::OpenGLWriter( Module &module ) :
	outW( -1 ), outH( -1 ), W( -1 ), flip( 0 ),
	aspect_ratio( 0.0 ), zoom( 0.0 ),
	VSync( true ), useShaders( true ),
	drawable( NULL )
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
		emit QMPlay2Core.dockVideo( drawable );
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
#ifdef QtVSync
	QGLFormat fmt;
	fmt.setSwapInterval( VSync );
	drawable = new Drawable( *this, fmt );
#else
	drawable = new Drawable( *this );
#endif
	return drawable->context()->isValid();
}
