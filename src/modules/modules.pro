TEMPLATE = subdirs
SUBDIRS = FFMpeg Inputs Modplug Playlists Subtitles QPainter Extensions Visualizations AudioFilters VideoFilters OpenGL
linux*: SUBDIRS += ALSA
else: SUBDIRS += PortAudio
unix:!macx {
	!greaterThan(QT_MAJOR_VERSION, 4)|qtHaveModule(x11extras): SUBDIRS += XVideo
	packagesExist(libpulse-simple):	SUBDIRS += PulseAudio
}
win32: SUBDIRS += FileAssociation DirectX
