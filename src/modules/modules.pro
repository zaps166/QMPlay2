TEMPLATE = subdirs
SUBDIRS = FFMpeg Inputs Modplug Playlists Subtitles QPainter Extensions Visualizations AudioFilters VideoFilters OpenGL
linux*: SUBDIRS += ALSA
else: SUBDIRS += PortAudio
unix:!macx {
	qtHaveModule(x11extras)|!greaterThan(QT_MAJOR_VERSION, 4): SUBDIRS += XVideo
	SUBDIRS += PulseAudio
}
win32: SUBDIRS += FileAssociation DirectX
