TEMPLATE = subdirs
SUBDIRS = FFmpeg Inputs Modplug Playlists Subtitles QPainter Extensions Visualizations AudioFilters VideoFilters

!contains(QT_CONFIG, opengles1): SUBDIRS += OpenGL2
else: message("OpenGL doesn't work with OpenGL|ES 1.0")

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
	else: message("XVideo will not be compiled, because QX11Extras doesn't exist")

	packagesExist(libpulse-simple):	SUBDIRS += PulseAudio
	else: message("PulseAudio will not be compiled, because libpulse-simple doesn't exist")
}

win32: SUBDIRS += FileAssociation DirectX

macx: QT_CONFIG -= no-pkg-config
win32|packagesExist(libsidplayfp)|packagesExist(libgme): SUBDIRS += Chiptune
else: message("Chiptune will not be compiled, because libsidplayfp and libgme don't exist")
