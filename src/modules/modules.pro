TEMPLATE = subdirs
SUBDIRS = FFmpeg Inputs Modplug Playlists Subtitles QPainter Extensions Visualizations AudioFilters VideoFilters
!android: SUBDIRS += AudioCD
linux*: {
	!android: SUBDIRS += ALSA
	else: SUBDIRS += OpenSLES
}
else {
	SUBDIRS += PortAudio
}
unix:!macx:!android {
	!greaterThan(QT_MAJOR_VERSION, 4)|qtHaveModule(x11extras): SUBDIRS += XVideo
	packagesExist(libpulse-simple):	SUBDIRS += PulseAudio
	!contains(QT_CONFIG, opengles1):!contains(QT_CONFIG, opengles2): SUBDIRS += OpenGL
	packagesExist(egl):packagesExist(glesv2): SUBDIRS += OpenGLES
}
else {
	!android: SUBDIRS += OpenGL
}
win32: SUBDIRS += FileAssociation DirectX
