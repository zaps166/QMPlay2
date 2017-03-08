TEMPLATE = subdirs
SUBDIRS = FFmpeg Inputs Modplug Playlists Subtitles QPainter Extensions Visualizations AudioFilters VideoFilters CUVID

!contains(QT_CONFIG, opengles1): SUBDIRS += OpenGL2
else: message("OpenGL doesn't work with OpenGL|ES 1.0")

!android: SUBDIRS += AudioCD Notify

linux*: {
	!android: SUBDIRS += ALSA
	else: SUBDIRS += OpenSLES
}
else {
	SUBDIRS += PortAudio
}

unix:!android {
	SUBDIRS += XVideo

	packagesExist(libpulse-simple):	SUBDIRS += PulseAudio
	else: message("PulseAudio will not be compiled, because libpulse-simple doesn't exist")
}

packagesExist(libsidplayfp)|packagesExist(libgme): SUBDIRS += Chiptune
else: message("Chiptune will not be compiled, because libsidplayfp and libgme don't exist")
