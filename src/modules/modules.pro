TEMPLATE = subdirs
SUBDIRS = FFMpeg Inputs Modplug Playlists Subtitles QPainter Extensions Visualizations AudioFilters VideoFilters OpenGL
linux*: SUBDIRS += ALSA
else: SUBDIRS += PortAudio
unix:!macx: SUBDIRS += XVideo PulseAudio
win32: SUBDIRS += FileAssociation DirectX
