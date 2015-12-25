#include <OpenGL2Writer.hpp>

#include <QMPlay2_OSD.hpp>
#include <Functions.hpp>

#ifdef USE_NEW_OPENGL_API
	#include <QOpenGLContext>
#endif

#include <QPainter>

#if !defined OPENGL_ES2 && !defined Q_OS_MAC
	#include <GL/glext.h>
#endif

#ifdef OPENGL_ES2
	static const char precisionStr[] = "precision lowp float;";
#else
	static const char precisionStr[] = "";
#endif

static const char vShaderYCbCrSrc[] =
	"%1"
	"attribute vec4 vPosition;"
	"attribute vec2 aTexCoord;"
	"varying vec2 vTexCoord;"
	"uniform vec2 scale;"
	"void main() {"
		"vTexCoord = aTexCoord;"
		"gl_Position = vPosition * vec4(scale.xy, 1, 1);"
	"}";
static const char fShaderYCbCrSrc[] =
	"%1"
	"varying vec2 vTexCoord;"
	"uniform vec4 videoEq;"
	"uniform sampler2D Ytex, Utex, Vtex;"
	"void main() {"
		"float brightness = videoEq[0];"
		"float contrast = videoEq[1];"
		"float saturation = videoEq[2];"
		"vec3 YCbCr = vec3("
			"texture2D(Ytex, vTexCoord)[0] - 0.0625,"
			"texture2D(Utex, vTexCoord)[0] - 0.5,"
			"texture2D(Vtex, vTexCoord)[0] - 0.5"
		");"
		"%2"
		"YCbCr.yz *= saturation;"
		"vec3 rgb = mat3"
		"("
			"1.16430,  1.16430, 1.16430,"
			"0.00000, -0.39173, 2.01700,"
			"1.59580, -0.81290, 0.00000"
		") * YCbCr * contrast + brightness;"
		"gl_FragColor = vec4(rgb, 1.0);"
	"}";
static const char fShaderYCbCrHueSrc[] =
	"float hueAdj = videoEq[3];"
	"if (hueAdj != 0.0) {"
		"float hue = atan(YCbCr[2], YCbCr[1]) + hueAdj;"
		"float chroma = sqrt(YCbCr[1] * YCbCr[1] + YCbCr[2] * YCbCr[2]);"
		"YCbCr[1] = chroma * cos(hue);"
		"YCbCr[2] = chroma * sin(hue);"
	"}";

static const char vShaderOSDSrc[] =
	"%1"
	"attribute vec4 vPosition;"
	"attribute vec2 aTexCoord;"
	"varying vec2 vTexCoord;"
	"void main() {"
		"vTexCoord = aTexCoord;"
		"gl_Position = vPosition;"
	"}";
static const char fShaderOSDSrc[] =
	"%1"
	"varying vec2 vTexCoord;"
	"uniform sampler2D tex;"
	"void main() {"
		"gl_FragColor = texture2D(tex, vTexCoord);"
	"}";

static const float verticesYCbCr[ 4 ][ 8 ] = {
	/* Normal */
	{
		-1.0f, -1.0f, //0. Left-bottom
		+1.0f, -1.0f, //1. Right-bottom
		-1.0f, +1.0f, //2. Left-top
		+1.0f, +1.0f, //3. Right-top
	},
	/* Horizontal flip */
	{
		+1.0f, -1.0f, //1. Right-bottom
		-1.0f, -1.0f, //0. Left-bottom
		+1.0f, +1.0f, //3. Right-top
		-1.0f, +1.0f, //2. Left-top
	},
	/* Vertical flip */
	{
		-1.0f, +1.0f, //2. Left-top
		+1.0f, +1.0f, //3. Right-top
		-1.0f, -1.0f, //0. Left-bottom
		+1.0f, -1.0f, //1. Right-bottom
	},
	/* Rotated */
	{
		+1.0f, +1.0f, //3. Right-top
		-1.0f, +1.0f, //2. Left-top
		+1.0f, -1.0f, //1. Right-bottom
		-1.0f, -1.0f, //0. Left-bottom
	}
};
static const float texCoordOSD[ 8 ] = {
	0.0f, 1.0f,
	1.0f, 1.0f,
	0.0f, 0.0f,
	1.0f, 0.0f,
};

#ifndef USE_NEW_OPENGL_API
	#define NUM_BUFFERS_TO_CLEAR 3 //3 buffers must be cleared when triple-buffer is used
#endif

/**/

#ifndef USE_NEW_OPENGL_API
Drawable::Drawable( OpenGL2Writer &writer, const QGLFormat &fmt ) :
	QGLWidget( fmt ),
#else
Drawable::Drawable( OpenGL2Writer &writer ) :
#endif
	isOK( true ), paused( false ),
#ifndef OPENGL_ES2
	supportsShaders( false ), canCreateNonPowerOfTwoTextures( false ),
	glActiveTexture( NULL ),
#endif
	writer( writer ),
	hasImage( false ),
	texCoordYCbCrLoc( -1 ), positionYCbCrLoc( -1 ), texCoordOSDLoc( -1 ), positionOSDLoc( -1 )
{
	grabGesture( Qt::PinchGesture );
	setMouseTracking( true );

#ifdef Q_OS_WIN
	/*
	 * This property is read by QMPlay2 and it ensures that toolbar will be visible
	 * on fullscreen in Windows Vista and newer on nVidia and AMD drivers.
	*/
	if ( QSysInfo::windowsVersion() >= QSysInfo::WV_6_0 )
		setProperty( "PreventFullscreen", true );
#endif

#ifndef USE_NEW_OPENGL_API
	connect( &QMPlay2Core, SIGNAL( videoDockMoved() ), this, SLOT( resetClearCounter() ) );
#endif

	/* Initialize texCoord array */
	texCoordYCbCr[ 0 ] = texCoordYCbCr[ 4 ] = texCoordYCbCr[ 5 ] = texCoordYCbCr[ 7 ] = 0.0f;
	texCoordYCbCr[ 1 ] = texCoordYCbCr[ 3 ] = 1.0f;
}

#ifndef USE_NEW_OPENGL_API
bool Drawable::testGL()
{
	makeCurrent();
	if ( ( isOK = isValid() ) )
	{
#ifndef OPENGL_ES2
		initGLProc();
		if ( !canCreateNonPowerOfTwoTextures || !supportsShaders || !glActiveTexture )
		{
			showOpenGLMissingFeaturesMessage();
			isOK = false;
		}
		/* Reset variables */
		supportsShaders = canCreateNonPowerOfTwoTextures = false;
		glActiveTexture = NULL;
#endif
	}
	doneCurrent();
	return isOK;
}
#endif

#ifndef OPENGL_ES2
void Drawable::initGLProc()
{
	const char *glExtensions = ( const char * )glGetString( GL_EXTENSIONS );
	if ( glExtensions )
	{
		supportsShaders = !!strstr( glExtensions, "GL_ARB_vertex_shader" ) && !!strstr( glExtensions, "GL_ARB_fragment_shader" ) && !!strstr( glExtensions, "GL_ARB_shader_objects" );
		canCreateNonPowerOfTwoTextures = !!strstr( glExtensions, "GL_ARB_texture_non_power_of_two" );
	}
	glActiveTexture = ( GLActiveTexture )context()->getProcAddress( "glActiveTexture" );
}
#endif

void Drawable::clr()
{
	hasImage = false;
	osdImg = QImage();
	osd_checksums.clear();
}

void Drawable::resizeEvent( QResizeEvent *e )
{
	Functions::getImageSize( writer.aspect_ratio, writer.zoom, width(), height(), W, H, &X, &Y );
	doReset = true;
	if ( e )
		QGLWidget::resizeEvent( e );
	else if ( paused )
	{
#ifndef USE_NEW_OPENGL_API
		updateGL();
#else
		update();
#endif
	}
}

#ifndef USE_NEW_OPENGL_API
void Drawable::resetClearCounter()
{
	doClear = NUM_BUFFERS_TO_CLEAR;
}
#endif

#ifndef OPENGL_ES2
void Drawable::showOpenGLMissingFeaturesMessage()
{
	fprintf
	(
		stderr,
		"GL_ARB_texture_non_power_of_two : %s\n"
		"Vertex & fragment shader: %s\n"
		"glActiveTexture: %s\n",
		canCreateNonPowerOfTwoTextures ? "yes" : "no",
		supportsShaders ? "yes" : "no",
		glActiveTexture ? "yes" : "no"
	);
	QMPlay2Core.logError( "OpenGL 2 :: " + tr( "Sterownik musi obsługiwać multiteksturowanie, shadery oraz tekstury o dowolnym rozmiarze" ), true, true );
}
#endif

void Drawable::initializeGL()
{
	int glMajor = 0, glMinor = 0;
#ifndef OPENGL_ES2
	glGetIntegerv( GL_MAJOR_VERSION, &glMajor );
	glGetIntegerv( GL_MINOR_VERSION, &glMinor );
#endif
	if ( !glMajor )
	{
		const QString glVersionStr = ( const char * )glGetString( GL_VERSION );
		const int dotIdx = glVersionStr.indexOf( '.' );
		if ( dotIdx > 0 )
		{
			const int vIdx = glVersionStr.lastIndexOf( ' ', dotIdx );
			if ( sscanf( glVersionStr.mid( vIdx < 0 ? 0 : vIdx ).toLatin1().data(), "%d.%d", &glMajor, &glMinor ) != 2 )
				glMajor = glMinor = 0;
		}
	}
	if ( glMajor )
		glVer = QString( "%1.%2" ).arg( glMajor ).arg( glMinor );
	else
		glVer = "2";

#ifndef OPENGL_ES2
	initGLProc();
#ifndef USE_NEW_OPENGL_API
	if ( !glActiveTexture ) //Be sure that "glActiveTexture" has valid pointer (don't check "supportsShaders" here)!
#else
	if ( !glActiveTexture || !canCreateNonPowerOfTwoTextures || !supportsShaders ) //"testGL()" doesn't work with "USE_NEW_OPENGL_API", so check features here!
#endif
	{
		showOpenGLMissingFeaturesMessage();
		isOK = false;
		return;
	}
#endif

	/* YCbCr shader */
	if ( shaderProgramYCbCr.shaders().isEmpty() )
	{
		shaderProgramYCbCr.addShaderFromSourceCode( QGLShader::Vertex, QString( vShaderYCbCrSrc ).arg( precisionStr ) );
		/* Use hue only when OpenGL/OpenGL|ES version >= 3.0, because it can be slow on old hardware and/or buggy drivers and may increase CPU usage! */
		shaderProgramYCbCr.addShaderFromSourceCode( QGLShader::Fragment, QString( fShaderYCbCrSrc ).arg( precisionStr ).arg( (glMajor * 10 + glMinor >= 30) ? fShaderYCbCrHueSrc : "" ) );
	}
	if ( shaderProgramYCbCr.bind() )
	{
		const qint32 newTexCoordLoc = shaderProgramYCbCr.attributeLocation( "aTexCoord" );
		const qint32 newPositionLoc = shaderProgramYCbCr.attributeLocation( "vPosition" );
		if ( newTexCoordLoc != newPositionLoc ) //If new locations are invalid, just leave them untouched...
		{
			texCoordYCbCrLoc = newTexCoordLoc;
			positionYCbCrLoc = newPositionLoc;
		}
		shaderProgramYCbCr.setUniformValue( "Ytex", 0 );
		shaderProgramYCbCr.setUniformValue( "Utex", 1 );
		shaderProgramYCbCr.setUniformValue( "Vtex", 2 );
		shaderProgramYCbCr.release();
	}
	else
	{
		QMPlay2Core.logError( tr( "Błąd podczas kompilacji/linkowania shaderów" ) );
		isOK = false;
		return;
	}

	/* OSD shader */
	if ( shaderProgramOSD.shaders().isEmpty() )
	{
		shaderProgramOSD.addShaderFromSourceCode( QGLShader::Vertex, QString( vShaderOSDSrc ).arg( precisionStr ) );
		shaderProgramOSD.addShaderFromSourceCode( QGLShader::Fragment, QString( fShaderOSDSrc ).arg( precisionStr ) );
	}
	if ( shaderProgramOSD.bind() )
	{
		const qint32 newTexCoordLoc = shaderProgramYCbCr.attributeLocation( "aTexCoord" );
		const qint32 newPositionLoc = shaderProgramYCbCr.attributeLocation( "vPosition" );
		if ( newTexCoordLoc != newPositionLoc ) //If new locations are invalid, just leave them untouched...
		{
			texCoordOSDLoc = newTexCoordLoc;
			positionOSDLoc = newPositionLoc;
		}
		shaderProgramOSD.setUniformValue( "tex", 3 );
		shaderProgramOSD.release();
	}
	else
	{
		QMPlay2Core.logError( tr( "Błąd podczas kompilacji/linkowania shaderów" ) );
		isOK = false;
		return;
	}

	/* Set OpenGL parameters */
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glClearColor( 0.0, 0.0, 0.0, 1.0 );
	glClear( GL_COLOR_BUFFER_BIT );
	glDisable( GL_STENCIL_TEST );
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_DITHER );

	/* Prepare textures */
	for ( int i = 1 ; i <= 4 ; ++i )
	{
		glBindTexture( GL_TEXTURE_2D, i );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, i == 1 ? GL_NEAREST : GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, i == 1 ? GL_NEAREST : GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	}

#ifdef VSYNC_SETTINGS
	/* Ensures that the VSync will be set on paintGL() */
	lastVSyncState = !writer.vSync;
#endif

#ifndef USE_NEW_OPENGL_API
	doClear = 0;
#endif
	doReset = true;
}
void Drawable::paintGL()
{
#ifdef VSYNC_SETTINGS
	if ( lastVSyncState != writer.vSync )
	{
		typedef int (APIENTRY *SwapInterval)(int); //BOOL is just normal int in Windows, APIENTRY declares nothing on non-Windows platforms
		SwapInterval swapInterval = NULL;
#ifdef Q_OS_WIN
		swapInterval = ( SwapInterval )context()->getProcAddress( "wglSwapIntervalEXT" );
#else
		swapInterval = ( SwapInterval )context()->getProcAddress( "glXSwapIntervalMESA" );
		if ( !swapInterval )
			swapInterval = ( SwapInterval )context()->getProcAddress( "glXSwapIntervalSGI" );
#endif
		if ( swapInterval )
			swapInterval( writer.vSync );
		else
			QMPlay2Core.logError( tr( "Zarządzanie VSync jest nieobsługiwane" ), true, true );
		lastVSyncState = writer.vSync;
	}
#endif

#if !defined USE_NEW_OPENGL_API
	if ( doReset )
		doClear = NUM_BUFFERS_TO_CLEAR;
	if ( doClear > 0 )
	{
		glClear( GL_COLOR_BUFFER_BIT );
		--doClear;
	}
#elif QT_VERSION < 0x050500
	glClear( GL_COLOR_BUFFER_BIT );
#endif

	if ( videoFrameArr.isEmpty() && !hasImage )
		return;

	bool resetDone = false;

	if ( !videoFrameArr.isEmpty() )
	{
		const VideoFrame *videoFrame = VideoFrame::fromData( videoFrameArr );

		if ( doReset )
		{
			/* Prepare textures */
			glBindTexture( GL_TEXTURE_2D, 2 );
			glTexImage2D( GL_TEXTURE_2D, 0, GL_LUMINANCE, videoFrame->linesize[ 0 ], writer.outH, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL );

			glBindTexture( GL_TEXTURE_2D, 3 );
			glTexImage2D( GL_TEXTURE_2D, 0, GL_LUMINANCE, videoFrame->linesize[ 1 ], writer.outH >> 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL );

			glBindTexture( GL_TEXTURE_2D, 4 );
			glTexImage2D( GL_TEXTURE_2D, 0, GL_LUMINANCE, videoFrame->linesize[ 2 ], writer.outH >> 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL );

			/* Prepare texture coordinates */
			texCoordYCbCr[ 2 ] = texCoordYCbCr[ 6 ] = (videoFrame->linesize[ 0 ] == writer.outW) ? 1.0f : (writer.outW / (videoFrame->linesize[ 0 ] + 1.0f));

			resetDone = true;
		}

		glActiveTexture( GL_TEXTURE0 );
		glBindTexture( GL_TEXTURE_2D, 2 );
		glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, videoFrame->linesize[ 0 ], writer.outH, GL_LUMINANCE, GL_UNSIGNED_BYTE, videoFrame->data[ 0 ] );

		glActiveTexture( GL_TEXTURE1 );
		glBindTexture( GL_TEXTURE_2D, 3 );
		glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, videoFrame->linesize[ 1 ], writer.outH >> 1, GL_LUMINANCE, GL_UNSIGNED_BYTE, videoFrame->data[ 1 ] );

		glActiveTexture( GL_TEXTURE2 );
		glBindTexture( GL_TEXTURE_2D, 4 );
		glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, videoFrame->linesize[ 2 ], writer.outH >> 1, GL_LUMINANCE, GL_UNSIGNED_BYTE, videoFrame->data[ 2 ] );

		videoFrameArr.clear();
		hasImage = true;
	}

	shaderProgramYCbCr.setAttributeArray( positionYCbCrLoc, verticesYCbCr[ writer.flip ], 2 );
	shaderProgramYCbCr.setAttributeArray( texCoordYCbCrLoc, texCoordYCbCr, 2 );
	shaderProgramYCbCr.enableAttributeArray( positionYCbCrLoc );
	shaderProgramYCbCr.enableAttributeArray( texCoordYCbCrLoc );

	shaderProgramYCbCr.bind();
	if ( doReset )
	{
		shaderProgramYCbCr.setUniformValue( "scale", W / ( float )width(), H / ( float )height() );
		shaderProgramYCbCr.setUniformValue( "videoEq", Brightness, Contrast, Saturation, Hue );
		doReset = !resetDone;
	}
	glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
	shaderProgramYCbCr.release();

	shaderProgramYCbCr.disableAttributeArray( texCoordYCbCrLoc );
	shaderProgramYCbCr.disableAttributeArray( positionYCbCrLoc );

	glActiveTexture( GL_TEXTURE3 );

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
			Functions::paintOSD( false, osd_list, scaleW, scaleH, p, &osd_checksums );
			glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, bounds.width(), bounds.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, osdImg.bits() );
		}

		const float left   = ( bounds.left() + X ) * 2.0f / width();
		const float right  = ( bounds.right() + X + 1 ) * 2.0f / width();
		const float top    = ( bounds.top() + Y ) * 2.0f / height();
		const float bottom = ( bounds.bottom() + Y + 1 ) * 2.0f / height();
		const float verticesOSD[ 8 ] = {
			left  - 1.0f, -bottom + 1.0f,
			right - 1.0f, -bottom + 1.0f,
			left  - 1.0f, -top    + 1.0f,
			right - 1.0f, -top    + 1.0f,
		};

		shaderProgramOSD.setAttributeArray( positionOSDLoc, verticesOSD, 2 );
		shaderProgramOSD.setAttributeArray( texCoordOSDLoc, texCoordOSD, 2 );
		shaderProgramOSD.enableAttributeArray( positionOSDLoc );
		shaderProgramOSD.enableAttributeArray( texCoordOSDLoc );

		glEnable( GL_BLEND );
		shaderProgramOSD.bind();
		glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
		shaderProgramOSD.release();
		glDisable( GL_BLEND );

		shaderProgramOSD.disableAttributeArray( texCoordOSDLoc );
		shaderProgramOSD.disableAttributeArray( positionOSDLoc );
	}
	osd_mutex.unlock();

	glBindTexture( GL_TEXTURE_2D, 0 );
}
#ifndef USE_NEW_OPENGL_API
void Drawable::resizeGL( int w, int h )
{
	glViewport( 0, 0, w, h );
}
#endif

#ifndef USE_NEW_OPENGL_API
void Drawable::paintEvent( QPaintEvent *e )
{
	doClear = NUM_BUFFERS_TO_CLEAR;
	QGLWidget::paintEvent( e );
}
#endif
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

OpenGL2Writer::OpenGL2Writer( Module &module ) :
	outW( -1 ), outH( -1 ), W( -1 ), flip( 0 ),
	aspect_ratio( 0.0 ), zoom( 0.0 ),
#ifdef VSYNC_SETTINGS
	vSync( true ),
#endif
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
OpenGL2Writer::~OpenGL2Writer()
{
	delete drawable;
}

bool OpenGL2Writer::set()
{
#ifdef VSYNC_SETTINGS
	vSync = sets().getBool( "VSync" );
#endif
	return sets().getBool( "Enabled" );
}

bool OpenGL2Writer::readyWrite() const
{
	return drawable->isOK;
}

bool OpenGL2Writer::processParams( bool * )
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
	else
		drawable->doReset = true;

	return readyWrite();
}

qint64 OpenGL2Writer::write( const QByteArray &arr )
{
	drawable->paused = false;
	drawable->videoFrameArr = arr;
	VideoFrame::unref( arr );
#ifndef USE_NEW_OPENGL_API
	drawable->updateGL();
#else
	drawable->update();
#endif
	return arr.size();
}
void OpenGL2Writer::writeOSD( const QList< const QMPlay2_OSD * > &osds )
{
	drawable->osd_mutex.lock();
	drawable->osd_list = osds;
	drawable->osd_mutex.unlock();
}

void OpenGL2Writer::pause()
{
	drawable->paused = true;
}

QString OpenGL2Writer::name() const
{
#ifdef OPENGL_ES2
	return "OpenGL|ES " + drawable->glVer;
#else
	return "OpenGL " + drawable->glVer;
#endif
}

bool OpenGL2Writer::open()
{
#ifndef USE_NEW_OPENGL_API
	QGLFormat fmt;
	fmt.setDepth( false );
	fmt.setStencil( false );
	drawable = new Drawable( *this, fmt );
	return drawable->testGL();
#else
	drawable = new Drawable( *this );
	return true;
#endif
}
