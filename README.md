QMPlay2 - Qt Media Player 2
===========================

![Screenshot](http://zaps166.sourceforge.net/gh-imgs/QMPlay2-screenshot.png)

QMPlay2 is a video and audio player. It can play all formats supported by FFmpeg, libmodplug (including J2B and SFX). It also supports Audio CD, raw files, Rayman 2 music and chiptunes. It contains YouTube and Prostopleer browser.

Table of Contents
=================

* [Instalation](#instalation)
* [YouTube](#youtube)
* [Spherical view](#spherical-view)
* [ALSA](#alsa)
* [Ubuntu Unity](#ubuntu-unity)
* [Hardware acceleration](#hardware-acceleration)
* [Deinterlacing](#deinterlacing)
* [Hidden features](#hidden-features)
* [Instalation from sources](#instalation-from-sources)
* [Building package RPM, DEB or any other](#building-package-rpm-deb-or-any-other)
* [Other information](#other-information)

##Instalation

####Easy installation on Windows (XP or higher)

- Download the newest "*.exe" file from http://sourceforge.net/projects/zaps166/files/QMPlay2 for your CPU architecture and install it:
	- 64-bit Windows: http://zaps166.sf.net/?QMPlay2Download=Win64Installer
	- 32-bit Windows: http://zaps166.sf.net/?QMPlay2Download=Win32Installer

####Easy installation on Arch Linux / Manjaro Linux

- Arch Linux only: Install AUR client (e.g. yaourt),
- run the following command:
```
$ yaourt -S qmplay2
```

####Easy installation on openSUSE Leap 42.1

- Run the following commands:
```
$ sudo zypper ar http://packman.inode.at/suse/openSUSE_Leap_42.1 Packman
$ sudo zypper in QMPlay2
```
- The "multimedia" repository uses Qt5 for QMPlay2, so it may be buggy on Qt5 <= 5.6.1 and its FFmpeg doesn't have all codecs.
- Don't use official package, because it is obsolete.
- Don't mix FFmpeg from different repositories!

####Easy installation on openSUSE 13.2

- Run the following commands:
```
$ sudo zypper ar http://packman.inode.at/suse/openSUSE_13.2 Packman
$ sudo zypper in QMPlay2 QMPlay2-kde-integration
```
- Don't mix FFmpeg from different repositories!

##YouTube

Some YouTube videos don't work without external "youtube-dl" software. You must install it!

####Windows

- Watch the video: http://zaps166.sf.net/downloads/QMPlay2_youtube-dl.mp4
- If the video doesn't play inside the web browser - paste the address to QMPlay2 (Ctrl+V) and play it!
- Currently (as of 30.06.2016) "youtube-dl.exe" needs Visual Studio 2010 Redistributable Package.

####Arch Linux / Manjaro Linux

- Install "youtube-dl" from repositories: `sudo pacman -S youtube-dl`

####Other Linux distributions or other operating system

- Don't use "youtube-dl" from repositories - usually it contains too old package which no longer works. The exception is Arch Linux and Manjaro Linux.
- Download "youtube-dl": http://rg3.github.io/youtube-dl/download.html
- Set the path for the script (you must have Python installed) in QMPlay2 YouTube settings (you can watch the video from Windows installation).
- QMPlay2 can automatically updates the "youtube-dl" if it is not installed from repositories.

##Spherical view

QMPlay2 supports spherical view on OpenGL video optput. You can watch e.g. YouTube spherical videos by pressing "Ctrl+3". You can also enable it from the menu: "Playback->Video filters->Spherical view".

##ALSA

If you are using your own ALSA configuration `asound.conf` or `.asoundrc` you should also append:
`defaults.namehint.!showall on` to the configuration file. Otherwise devices which were added may not be visible!

##Ubuntu Unity

Tray context menu in Ubuntu Unity doesn't work properly on Qt older than 5.6.1!

##Hardware acceleration

QMPlay2 supports hardware video decoding. Currently it works only on X11 (Linux/BSD) via VDPAU and VA-API. Hardware acceleration is disabled by default.
You can enable it in "Settings->Playback settings" - move proper decoder on decoders list to the top and then apply settings.

VDPAU and VA-API decoder uses its own video output, so OpenGL features and CPU filters won't work.

VDPAU and VA-API has its own deinterlacing filters. These settings are mixed together with "FFmpeg" module settings.

H.264 lossless movies (CRF 0 or QP 0) are not properly decoded.

##Deinterlacing

Video interlacing is automatically detected by QMPlay2. Go to "Settings->Video filters" for more options.
If you have fast CPU (or low video resolution) you can use "Yadif 2x" deinterlacing filter for better quality.

You can enable deinterlacing filter on non-interlaced video if necessary (some interlaced videos may not have interlacing data),
but remember to revert this settings on any other video! Otherwise the video quality will be worse and performance will be worse!

Hardware accelerated video decoding uses its own video filtering, so the CPU deinterlacing method (e.g. "Yadif 2x") does nothing in this case.
Of course you can adjust other deinterlacing settings in case of hardware acceleration.

Chroma plane if pixel format is not YUV420 when XVideo or DirectDraw is used as video output might not be properly deinterlaced.

##Hidden features

###Audio balance

Right click on volume slider and select "Split channels".

###Last.fm scrobbling

Go to "Options->Modules settings" and click "Extensions" on the list. Find "LastFM" group box, select "Scrobble", type your login and password and then press "Apply".

##Instalation from sources

###CMake requirements

For CMake build be sure that you have correct CMake version:
- CMake 2.8.11 or higher is recommended,
- CMake 2.8.9 is the lowest version for Qt5,
- CMake 2.8.6 is the lowest version for Qt4.

###You need devel packages:

####Necessary:
- Qt4 >= 4.7.0 (4.8.x recommended) or Qt5 >= 5.0.0 (>= 5.6.1 recommended):
	- QtOpenGL - not used since Qt 5.6.0,
	- QtDBus - Linux/BSD only,
	- OpenSSL for https support.
- FFmpeg >= 2.2:
	- libavformat - for FFmpeg module only,
	- libavcodec - for FFmpeg module only,
	- libswscale,
	- libavutil,
	- libswresample or libavresample - libswresample is default,
	- libavdevice - for FFmpeg module only, optional (enabled on Linux as default),
	- OpenSSL for https.

####Important:
- TagLib >= 1.7 (>= 1.9 recommended),
- libass - for OSD and every subtitles.

####For modules (some of them can be automatically not used if not found):
- FFmpeg (necessary module): libva (VA-API) and libvdpau (VDPAU) - only on X11,
- Chiptune: libgme (kode54 version is recommended) and libsidplayfp,
- DirectX (Windows only): DirectDraw SDK (included in mingw-w64),
- AudioCD: libcdio and libcddb,
- ALSA (Linux only): libasound,
- PulseAudio - libpulse-simple,
- PortAudio: portaudio (v19),
- XVideo (X11 only): libxv.

###Running the compilation:

#####Arch Linux / Manjaro Linux dependencies

- Common packages:
```
$ sudo pacman -S cmake make gcc pkg-config ffmpeg libass libva libxv alsa-lib libcdio taglib libcddb libpulse libgme libsidplayfp xdg-utils
```
- Qt:
	- for Qt5 build (recommend for Qt5 >= 5.6.1): `sudo pacman -S qt5-base qt5-tools`,
	- for Qt4 build: `sudo pacman -S qt4`.

You can also install youtube-dl: `sudo pacman -S youtube-dl`

#####OpenSUSE dependencies (for Qt4 build)

- Add Packman repository for FFmpeg with all codecs (don't mix FFmpeg from different repositories!):
	- openSUSE Leap 42.1: `sudo zypper ar http://packman.inode.at/suse/openSUSE_Leap_42.1 Packman`
	- openSUSE 13.2: `sudo zypper ar http://packman.inode.at/suse/openSUSE_13.2 Packman`
- Install dependencies:
```
$ sudo zypper in cmake libqt4-devel gcc-c++ alsa-devel libpulse-devel libass-devel libtag-devel libcdio-devel libcddb-devel libXv-devel Mesa-devel libsidplayfp-devel libgme-devel libva-devel libvdpau-devel libavcodec-devel libavformat-devel libavutil-devel libswscale-devel libswresample-devel libavdevice-devel xdg-utils
```

#####Ubuntu

- Open "Software & Updates" and select the "Universe" repository.

#####Ubuntu 16.04 and higher dependencies (Qt4 build)

- Install dependencies from the package manager:
```
$ sudo apt-get install cmake g++ libqt4-dev libasound2-dev libass-dev libcdio-dev libcddb2-dev libsidplayfp-dev libgme-dev libxv-dev libtag1-dev libpulse-dev libva-dev libvdpau-dev libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libswresample-dev libavdevice-dev xdg-utils
```

#####Ubuntu 15.04 and 15.10 dependencies (Qt4 build)

- Install dependencies from the package manager:
```
$ sudo apt-get install cmake g++ libqt4-dev libasound2-dev libass-dev libcdio-dev libcddb2-dev libsidplayfp-dev libgme-dev libxv-dev libtag1-dev libpulse-dev libva-dev libvdpau-dev libavcodec-ffmpeg-dev libavformat-ffmpeg-dev libavutil-ffmpeg-dev libswscale-ffmpeg-dev libswresample-ffmpeg-dev libavdevice-ffmpeg-dev xdg-utils
```

#####Ubuntu 14.10 and older dependencies (Qt4 build)

Ubuntu <= 14.10 uses old LibAV instead of the new FFmpeg (>= 2.2 is necessary), so the FFmpeg must be compiled from sources and the LibAV development files must be removed!

- Install dependencies from the package manager:
```
$ sudo apt-get install cmake g++ yasm libqt4-dev libasound2-dev libass-dev libcdio-dev libcddb2-dev libsidplayfp-dev libgme-dev libxv-dev libtag1-dev libpulse-dev libssl-dev libva-dev libvdpau-dev xdg-utils
```
- Remove LibAV devel files for the compilation time (this is mandatory, otherwise QMPlay2 will link to the old LibAV libraries and it will crash at runtime!):
```
$ sudo apt-get remove libavformat-dev libavcodec-dev libavresample-dev libavdevice-dev libavutil-dev
```
You can install it again after compilation.
- Download the newest FFmpeg from http://ffmpeg.org/download.html and unpack it. Then write a command:
```
$ ./configure --prefix=/usr/local --enable-shared --disable-static --enable-openssl --disable-avfilter --disable-encoders --disable-muxers --disable-programs
$ make -j4
$ sudo make -j4 install
```
This will compile and install the newest FFmpeg without features that are not supported in QMPlay2.
- Run: `sudo ldconfig`
- Before QMPlay2 compilation please be sure that you have removed LibAV development packages from repositories!

####Linux/BSD using CMake:

- Install all needed packages and dependencies (in devel version) using package manager or compile it from sources.
- You can use `cmake-gui` for graphical configuration. Otherwise follow below instructions:
	- create a "build" directory and go to it: `mkdir build && cd build`,
	- run CMake (also you can run with arguments which you want): `cmake ..`,
	- check the summary - which features are enabled - you can set/force them manually,
	- if CMake finishes wihout errors, run: `make -j4` (replace 4 with numbers of CPU threads),
	- if compiling finishes wihout errors, install it: `sudo make -j4 install`.

CMake options (option - default value: description):
- CMake options and the default settings:
	- `CMAKE_INSTALL_PREFIX` - mostly it is `/usr/local`: instalation directory.
	- `CMAKE_BUILD_TYPE` - `Release`.
	- `LANGUAGES` - `All` - a space-seperated list of translations to compile into QMPlay2.
	- `SOLID_ACTIONS_INSTALL_PATH` - Linux/BSD only, autodetect: you can specify the path manually.
	- `SET_INSTALL_RPATH` - non-Windows only, `ON` on OS X, `OFF` anywhere else: sets RPATH after installation.
	- `USE_QT5` - autodetect: if Qt >= 5.6.1 is found then it uses Qt5, otherwise it uses Qt4.
	- `USE_FFMPEG` - ON: enable/disable FFmpeg module.
	- `USE_FFMPEG_VAAPI`: autodetect: enabled on X11 if libva and libva-x11 exist.
	- `USE_FFMPEG_VDPAU`: autodetect: enabled on X11 if libvdpau exist.
	- `USE_FFMPEG_AVDEVICE` - autodetect on Linux, `OFF` on non-Linux OS: it allows to use e.g. V4L2 devices.
	- `USE_INPUTS` - ON: enable/disable Inputs module.
	- `USE_MODPLUG` - ON: enable/disable Modplug module.
	- `USE_EXTENSIONS` - ON: enable/disable Extensions module.
	- `USE_MPRIS2` - Linux/BSD only, `ON`: enable/disable MPRIS2 in Extensions module.
	- `USE_VISUALIZATIONS` - ON: enable/disable Visualizations module.
	- `USE_AUDIOFILTERS` - ON: enable/disable AudioFilters module.
	- `USE_VIDEOFILTERS` - ON: enable/disable VideoFilters module.
	- `USE_OPENGL2` - `ON`: enable/disable OpenGL2 module.
	- `USE_AUDIOCD` - autodetect: enabled if libcdio and libcddb exist: enable/disable AudioCD module.
	- `USE_ALSA` - `ON` on Linux: enable/disable ALSA module.
	- `USE_PORTAUDIO` - `ON` on non-Linux OS: enable/disable PortAudio module.
	- `USE_PULSEAUDIO` - autodetect on Linux/BSD, `OFF` anywhere else: enable/disable PulseAudio module.
	- `USE_XVIDEO` - autodetect on X11: enabled if libxv exists: enable/disable XVideo module.
	- `USE_CHIPTUNE_GME` - autodetect: enabled if libgme exists.
	- `USE_CHIPTUNE_SID` - autodetect: enabled if libsidplayfp exists.
	- `USE_TAGLIB` - `ON`: allows to disable tag editor.
	- `USE_AVRESAMPLE` - `OFF`: uses libavresample instead of libswresample.
	- `USE_OPENGL_FOR_VISUALIZATIONS` - Qt >= 5.6.0 feature, `OFF`: it allows to use "QOpenGLWidget" for visualizations.
	- `USE_JEMALLOC` - `OFF`: it links to jemalloc memory allocator which can reduce memory usage. Remember to unset `LDFLAGS` if it contains `--as-needed` flag!
	- `USE_CMD` - Windows only, `OFF`.
	- `USE_PROSTOPLEER` - `ON`.
	- `USE_LIBASS` - `ON`.

Using other Qt installation using CMake:
- Qt4: `QT_QMAKE_EXECUTABLE`: path to the `qmake` executable from Qt4.
- Qt5: `Qt5Widgets_DIR`: path to the Qt5Widgets cmake directory (e.g. `~/qtbase/lib/cmake/Qt5Widgets`).

Every CMake option must be prepended with `-D` and new value is set after `=`.

You can strip binaries during installation to save disk space: `sudo make -j4 install/strip`.

Example commands (execute it in QMPlay2 directory with source code):

- Simple installation (rely on autodetection, `strip` reduces size but it makes that debugging is impossible):

```sh
$ mkdir build
$ cd build
$ cmake .. -DCMAKE_INSTALL_PREFIX=/usr
$ make -j8
$ sudo make -j8 install/strip
```

- Use manually-specified install prefix, force Qt4, disable SID, enable jemalloc, use Polish language only and use manually-specified Solid actions path:

```sh
$ mkdir build
$ cd build
$ cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DUSE_QT5=NO -DUSE_CHIPTUNE_SID=OFF -DUSE_JEMALLOC=ON -DLANGUAGES="pl" -DSOLID_ACTIONS_INSTALL_PATH="/usr/share/solid/actions"
$ make -j8
$ sudo make -j8 install
```

- Uninstallation (in `build` directory):

```sh
$ sudo make uninstall
```

####Mac OS X:

- Download and install Xcode.
- Download and install the newest Qt5 for OS X.
- Download, compile and install `pkg-config`.
- Download and install CMake for OS X (mainly for taglib).
- Download, compile and install all dependencies from sources.
- Add directory containing `qmake` to `PATH` (use `export PATH=/path/to/dir`).
- Run: `./compile_unix n` where `n` is number of threads (4 as default).

You can also use `cmake`, but it doesn't create application bundle.

####Windows (cross-compilation):

- Install all required MinGW packages (I recommend Arch Linux unofficial MinGW repository).
- Some libraries are incompatible, uses unneeded dependencies or doesn't exist in repository - you must built them on your own.
- Run `cmake` from cross-compilation toolchain.

#####Other information for Windows

- You can also compile it on Windows, but you must build toolchain for your own!
- I'm using my own PKGBUILDs for many MinGW libraries.
- Visual Studio can't compile QMPlay2.

##Building package RPM, DEB or any other

- QMPlay2 sometimes uses the external software at runtime - "youtube-dl", so it should be added as an optional package.
- Use CMake.

##Other information

- QMPlay2 uses Oxygen icons which are under LGPLv3 license (https://techbase.kde.org/Projects/Oxygen).
- QMPlay2 contains modified libmodplug sources which are used by Modplug module.
