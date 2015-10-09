TEMPLATE = subdirs
SUBDIRS = FFmpeg Inputs Modplug Playlists Subtitles QPainter Extensions Visualizations AudioFilters VideoFilters
!contains(QT_CONFIG, opengles1): SUBDIRS += OpenGL2
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
}
win32: SUBDIRS += FileAssociation DirectX
