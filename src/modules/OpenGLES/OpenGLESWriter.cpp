#include <OpenGLESWriter.hpp>

#include <QMPlay2_OSD.hpp>
#include <Functions.hpp>

#include <QPainter>

#include <GLES2/gl2.h>
#include <EGL/egl.h>

static const char vShaderYCbCrSrc[] =
	"precision lowp float;"
	"attribute vec4 vPosition;"
	"attribute vec2 aTexCoord;"
	"varying vec2 vTexCoord;"
	"uniform vec2 scale;"
	"void main() {"
		"vTexCoord = aTexCoord;"
		"gl_Position = vPosition * vec4(scale.xy, 1, 1);"
	"}";
static const char fShaderYCbCrSrc[] =
	"precision lowp float;"
	"varying vec2 vTexCoord;"
	"uniform vec4 videoEq;"
	"uniform sampler2D Ytex, Utex, Vtex;"
	"void main() {"
		"float brightness = videoEq[0];"
		"float contrast = videoEq[1];"
		"float saturation = videoEq[2];"
		"float hueAdj = videoEq[3];"
		"vec3 YCbCr = vec3("
			"texture2D(Ytex, vTexCoord)[0] - 0.0625,"
			"texture2D(Utex, vTexCoord)[0] - 0.5,"
			"texture2D(Vtex, vTexCoord)[0] - 0.5"
		");"
		"%1"
		"if (saturation != 1.0)"
			"YCbCr.yz *= saturation;"
		"vec3 rgb = mat3(1.1643, 1.1643, 1.1643, 0.0, -0.39173, 2.017, 1.5958, -0.8129, 0.0) * YCbCr;"
		"if (contrast != 1.0)"
			"rgb *= contrast;"
		"if (brightness != 0.0)"
			"rgb += brightness;"
		"gl_FragColor = vec4(rgb, 1.0);"
	"}";
static const char fShaderYCbCrHUESrc[] =
	"if (hueAdj != 0.0) {"
		"float hue = atan(YCbCr[2], YCbCr[1]) + hueAdj;"
		"float chroma = sqrt(YCbCr[1] * YCbCr[1] + YCbCr[2] * YCbCr[2]);"
		"YCbCr[1] = chroma * cos(hue);"
		"YCbCr[2] = chroma * sin(hue);"
	"}";

static const char vShaderOSDSrc[] =
	"precision lowp float;"
	"attribute vec4 vPosition;"
	"attribute vec2 aTexCoord;"
	"varying vec2 vTexCoord;"
	"void main() {"
		"vTexCoord = aTexCoord;"
		"gl_Position = vPosition;"
	"}";
static const char fShaderOSDSrc[] =
	"precision lowp float;"
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

struct GLPrivate
{
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
};

static quint32 loadShader( quint32 type, const char shaderSrc[] )
{
	quint32 shader = glCreateShader( type );

	glShaderSource( shader, 1, &shaderSrc, NULL );
	glCompileShader( shader );

	qint32 compiled;
	glGetShaderiv( shader, GL_COMPILE_STATUS, &compiled );
	if ( !compiled )
	{
		glDeleteShader( shader );
		return 0;
	}
	return shader;
}
static bool linkShader( quint32 shaderProgram )
{
	glLinkProgram( shaderProgram );

	qint32 linked;
	glGetProgramiv( shaderProgram, GL_LINK_STATUS, &linked );
	if ( !linked )
	{
		glDeleteProgram( shaderProgram );
		return false;
	}
	return true;
}

/**/

GLDrawable::GLDrawable( OpenGLESWriter &writer ) :
	isOK( true ),
	videoFrame( NULL ),
	p( new GLPrivate ),
	writer( writer ),
	hasImage( false ), hasCurrentContext( false ), doReset( true ),
	shaderProgramYCbCr( 0 ), shaderProgramOSD( 0 )
{
	const qint32 configAttribs[] =
	{
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_NONE
	};
	const qint32 contextAttribs[] =
	{
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE,
	};

	/* Widget attributes */
	setAttribute( Qt::WA_OpaquePaintEvent );
	setAttribute( Qt::WA_PaintOnScreen );
	grabGesture( Qt::PinchGesture );
	setMouseTracking( true );

	/* Initialize EGL */
	p->surface = EGL_NO_SURFACE;
	p->context = EGL_NO_CONTEXT;

	p->display = eglGetDisplay( EGL_DEFAULT_DISPLAY );
	if ( !eglInitialize( p->display, NULL, NULL ) )
		return;

	qint32 numConfigs;
	eglChooseConfig( p->display, configAttribs, &eglConfig, 1, &numConfigs );
	if ( numConfigs != 1 )
		return;

	qint32 format;
	eglGetConfigAttrib( p->display, eglConfig, EGL_NATIVE_VISUAL_ID, &format );

	p->context = eglCreateContext( p->display, eglConfig, EGL_NO_CONTEXT, contextAttribs );

	/* Initialize texCoord array */
	texCoordYCbCr[ 0 ] = texCoordYCbCr[ 4 ] = texCoordYCbCr[ 5 ] = texCoordYCbCr[ 7 ] = 0.0f;
	texCoordYCbCr[ 1 ] = texCoordYCbCr[ 3 ] = 1.0f;
}
GLDrawable::~GLDrawable()
{
	if ( p->context != EGL_NO_CONTEXT )
	{
		if ( shaderProgramYCbCr )
			glDeleteProgram( shaderProgramYCbCr );
		if ( shaderProgramOSD )
			glDeleteProgram( shaderProgramOSD );
		eglMakeCurrent( p->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
		eglDestroyContext( p->display, p->context );
	}
	if ( p->surface != EGL_NO_SURFACE )
		eglDestroySurface( p->display, p->surface );
	if ( p->display != EGL_NO_DISPLAY )
		eglTerminate( p->display );
	delete p;
}

void GLDrawable::resizeEvent( QResizeEvent *e )
{
	Functions::getImageSize( writer.aspect_ratio, writer.zoom, width(), height(), W, H, &X, &Y );
	doReset = true;
	if ( !e )
		paintGL();
	else
	{
		if ( hasCurrentContext )
			glViewport( 0, 0, width(), height() );
		QWidget::resizeEvent( e );
	}
}
bool GLDrawable::isContextValid() const
{
	return p->context;
}
bool GLDrawable::makeCurrent()
{
	if ( p->surface == EGL_NO_SURFACE )
		p->surface = eglCreateWindowSurface( p->display, eglConfig, winId(), NULL );
	if ( eglMakeCurrent( p->display, p->surface, p->surface, p->context ) )
	{
		if ( !hasCurrentContext )
		{
			hasCurrentContext = true;
			return isOK = initializeGL();
		}
	}
	return false;
}
void GLDrawable::clr()
{
	hasImage = false;
	osdImg = QImage();
	osd_checksums.clear();
}

void GLDrawable::paintGL()
{
	if ( !hasCurrentContext && !makeCurrent() )
		return;

	if ( lastVSyncState != writer.VSync )
		eglSwapInterval( p->display, lastVSyncState = writer.VSync );

	glClear( GL_COLOR_BUFFER_BIT );

	if ( !videoFrame && !hasImage )
		return;

	if ( videoFrame )
	{
		hasImage = true;

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
	}

	glVertexAttribPointer( positionYCbCrLoc, 2, GL_FLOAT, false, 0, verticesYCbCr[ writer.flip ] );
	glVertexAttribPointer( texCoordYCbCrLoc, 2, GL_FLOAT, false, 0, texCoordYCbCr );
	glEnableVertexAttribArray( positionYCbCrLoc );
	glEnableVertexAttribArray( texCoordYCbCrLoc );

	glUseProgram( shaderProgramYCbCr );
	if ( doReset )
	{
		glUniform2f( scaleLoc, W / ( float )width(), H / ( float )height() );
		glUniform4f( videoEqLoc, Brightness, Contrast, Saturation, Hue );
		doReset = false;
	}
	glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );

	glDisableVertexAttribArray( texCoordYCbCrLoc );
	glDisableVertexAttribArray( positionYCbCrLoc );

	/* OSD */
	osd_mutex.lock();
	if ( !osd_list.isEmpty() )
	{
		glActiveTexture( GL_TEXTURE3 );
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

		glEnableVertexAttribArray( texCoordYCbCrLoc );
		glEnableVertexAttribArray( positionYCbCrLoc );
		glVertexAttribPointer( positionOSDLoc, 2, GL_FLOAT, false, 0, verticesOSD );
		glVertexAttribPointer( texCoordOSDLoc, 2, GL_FLOAT, false, 0, texCoordOSD );

		glEnable( GL_BLEND );
		glUseProgram( shaderProgramOSD );
		glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
		glDisable( GL_BLEND );

		glDisableVertexAttribArray( texCoordOSDLoc );
		glDisableVertexAttribArray( positionOSDLoc );
	}
	osd_mutex.unlock();

	glUseProgram( 0 );
	eglSwapBuffers( p->display, p->surface );
}

bool GLDrawable::initializeGL()
{
	const bool useHUE = !strstr( ( const char * )glGetString( GL_VERSION ), "OpenGL ES 2.0 Mesa" ) || !strstr( ( const char * )glGetString( GL_RENDERER ), "Intel" );

	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glClearColor( 0.0, 0.0, 0.0, 0.0 );
	glDisable( GL_DITHER );

	quint32 vertexShader, fragmentShader;
	shaderProgramYCbCr = glCreateProgram();
	shaderProgramOSD = glCreateProgram();

	/* YCbCr shader */
	vertexShader = loadShader( GL_VERTEX_SHADER, vShaderYCbCrSrc );
	fragmentShader = loadShader( GL_FRAGMENT_SHADER, QString( fShaderYCbCrSrc ).arg( useHUE ? fShaderYCbCrHUESrc : "" ).toLatin1() );
	if ( !vertexShader || !fragmentShader )
	{
		QMPlay2Core.logError( "OpenGLES :: " + tr( "Błąd podczas kompilacji shaderów" ) );
		return false;
	}

	glAttachShader( shaderProgramYCbCr, vertexShader );
	glAttachShader( shaderProgramYCbCr, fragmentShader );
	glDeleteShader( vertexShader );
	glDeleteShader( fragmentShader );

	glBindAttribLocation( shaderProgramYCbCr, 0, "vPosition" );
	glBindAttribLocation( shaderProgramYCbCr, 1, "aTexCoord" );

	if ( !linkShader( shaderProgramYCbCr ) )
	{
		QMPlay2Core.logError( "OpenGLES :: " + tr( "Błąd podczas linkowania shaderów" ) );
		return false;
	}

	scaleLoc = glGetUniformLocation( shaderProgramYCbCr, "scale" );
	videoEqLoc = glGetUniformLocation( shaderProgramYCbCr, "videoEq" );
	texCoordYCbCrLoc = glGetAttribLocation( shaderProgramYCbCr, "aTexCoord" );
	positionYCbCrLoc = glGetAttribLocation( shaderProgramYCbCr, "vPosition" );

	/* Set the texture unit for every plane */
	glUseProgram( shaderProgramYCbCr );
	glUniform1i( glGetUniformLocation( shaderProgramYCbCr, "Ytex" ), 0 );
	glUniform1i( glGetUniformLocation( shaderProgramYCbCr, "Utex" ), 1 );
	glUniform1i( glGetUniformLocation( shaderProgramYCbCr, "Vtex" ), 2 );
	glUseProgram( 0 );


	/* OSD shader */
	vertexShader = loadShader( GL_VERTEX_SHADER, vShaderOSDSrc );
	fragmentShader = loadShader( GL_FRAGMENT_SHADER, fShaderOSDSrc );
	if ( !vertexShader || !fragmentShader )
	{
		QMPlay2Core.logError( "OpenGLES :: " + tr( "Błąd podczas kompilacji shaderów" ) );
		return false;
	}

	glAttachShader( shaderProgramOSD, vertexShader );
	glAttachShader( shaderProgramOSD, fragmentShader );
	glDeleteShader( vertexShader );
	glDeleteShader( fragmentShader );

	glBindAttribLocation( shaderProgramOSD, 0, "vPosition" );
	glBindAttribLocation( shaderProgramOSD, 1, "aTexCoord" );

	if ( !linkShader( shaderProgramOSD ) )
	{
		QMPlay2Core.logError( "OpenGLES :: " + tr( "Błąd podczas linkowania shaderów" ) );
		return false;
	}

	texCoordOSDLoc = glGetAttribLocation( shaderProgramOSD, "aTexCoord" );
	positionOSDLoc = glGetAttribLocation( shaderProgramOSD, "vPosition" );

	/* Set the texture unit */
	glUseProgram( shaderProgramOSD );
	glUniform1i( glGetUniformLocation( shaderProgramOSD, "tex" ), 3 );
	glUseProgram( 0 );


	/* Prepare textures */
	for ( int i = 1 ; i <= 4 ; ++i )
	{
		glBindTexture( GL_TEXTURE_2D, i );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, i == 1 ? GL_NEAREST : GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, i == 1 ? GL_NEAREST : GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	}

	/* Ensures to set VSync on first repaint */
	lastVSyncState = !writer.VSync;

	return true;
}

QPaintEngine *GLDrawable::paintEngine() const
{
	return NULL;
}

void GLDrawable::paintEvent( QPaintEvent * )
{
	paintGL();
}
bool GLDrawable::event( QEvent *e )
{
	/* Pass gesture event to the parent */
	if ( e->type() == QEvent::Gesture )
		return qApp->notify( parent(), e );
	return QWidget::event( e );
}

/**/

OpenGLESWriter::OpenGLESWriter( Module &module ) :
	outW( -1 ), outH( -1 ), W( -1 ), flip( 0 ),
	aspect_ratio( 0.0 ), zoom( 0.0 ),
	VSync( true ),
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
OpenGLESWriter::~OpenGLESWriter()
{
	delete drawable;
}

bool OpenGLESWriter::set()
{
	VSync = sets().getBool( "VSync" );
	return sets().getBool( "Enabled" );
}

bool OpenGLESWriter::readyWrite() const
{
	return drawable && drawable->isOK;
}

bool OpenGLESWriter::processParams( bool * )
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

	return readyWrite();
}

qint64 OpenGLESWriter::write( const QByteArray &arr )
{
	drawable->videoFrame = VideoFrame::fromData( arr );
	drawable->paintGL();
	drawable->videoFrame = NULL;
	VideoFrame::unref( arr );
	return arr.size();
}
void OpenGLESWriter::writeOSD( const QList< const QMPlay2_OSD * > &osds )
{
	drawable->osd_mutex.lock();
	drawable->osd_list = osds;
	drawable->osd_mutex.unlock();
}

QString OpenGLESWriter::name() const
{
	return OpenGLESWriterName;
}

bool OpenGLESWriter::open()
{
	drawable = new GLDrawable( *this );
	return drawable->isContextValid();
}
