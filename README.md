#QMPlay2 - QT Media Player 2

>QMPlay2 is a video and audio player. It can play all formats supported by FFmpeg, libmodplug (including J2B and SFX). It also supports Audio CD, raw files and Rayman 2 music. It contains YouTube and Prostopleer browser.

##Instalation

####Easy installation on Windows (XP or higher)

- Download the newest "*.exe" file from http://sourceforge.net/projects/zaps166/files/QMPlay2 for your CPU architecture and install it:
	- 64-bit Windows: http://zaps166.sf.net/?QMPlay2Download=Win64Installer
	- 32-bit Windows: http://zaps166.sf.net/?QMPlay2Download=Win32Installer
- If you want to watch YouTube videos and use "youtube-dl", watch this video: http://zaps166.sf.net/downloads/QMPlay2_youtube-dl.mp4

####Easy installation on openSUSE

- Just run the following commands:
```sh
$ sudo zypper ar http://packman.inode.at/suse/openSUSE_13.2 Packman
$ sudo zypper in QMPlay2 QMPlay2-kde-integration
```

##Compilation from sources

###You need (devel packages):

- ffmpeg >= 2.0 (>= 2.4 recommended) (libavcodec, libavformat, libavutil, libswscale, libswresample or libavresample\*, OpenSSL for https),
- portaudio (default on non-Linux OS, if you want to compile portaudio module on Linux, change "src/modules/modules.pro"),
- pulseaudio (optional, if you don't want to compile pulseaudio on Linux, remove it from "src/modules/modules.pro"),
- qt4 or qt5 (including QtOpenGL, OpenSSL for https, QtDBus on Linux/BSD and QX11Extras for Qt5),
- taglib >= 1.7 (>= 1.9 recommended) (you can disable it in "src/gui/gui.pro"),
- libva (vaapi) and libvdpau - only on X11,
- libasound (for ALSA module on Linux),
- DirectDraw SDK - only on Windows,
- libcdio and libcddb,
- libxv - only on X11,
- libass.

\*libavresample: uncomment last eight lines in "src/qmplay2/qmplay2.pro"

##Running compilation script:

###Windows (Cross-compile):

- Install all required MinGW packages (I recommend Arch Linux unofficial MinGW repository).
- Some libraries are incompatible, uses unneeded libraries or doesn't exists in repository - you must built them on your own.
- Notice that QMPlay2 uses static linking for some libraries.
- Run "./compile_win_cross".

####Other information

- You can also compile it on Windows, but you must build toolchain for your own!
- I'm using my own PKGBUILDs for many MinGW libraries.
- Visual Studio can't compile QMPlay2.

###Linux/BSD:

- Install all dependencies using package manager (in devel version) or compile it from sources.
- If you want to compile with qt suffix (for example "qmake-qt5") - "export QT_SUFFIX=-qt5".
- Compilation only:
	- If you don't want to open Xterm or Konsole - "export NOTERM=1".
	- If you want to prepare *.desktop files for system use - "export SYSTEM_BUILD=1".
	- Run "./compile_unix".
	- QMPlay2 is in "app" directory, you can move its contents into "/usr" directory if "$SYSTEM_BUILD == 1".
- Compilation and installation:
	- Run "./installer_unix install" (it compiles and uses "sudo" to copy files to "/usr").
	- If you want to uninstall, run "./installer_unix uninstall" (it also uses "sudo").

###OS X:

- Download and install Xcode.
- Download and install the newest Qt for Mac.
- Download, compile and install "pkg-config".
- Download and install CMake for Mac (for taglib).
- Download, compile and install all dependencies from sources.
- Add directory containing "qmake" to "PATH".
- Run "./compile_mac n" where "n" is number of threads (4 by default).

Building package RPM, DEB or any other - look at Arch Linux PKGBUILD:
>https://aur.archlinux.org/cgit/aur.git/tree/PKGBUILD?h=qmplay2

QMPlay2 uses the external software - "youtube-dl", so "youtube-dl" should be as optional package in package manager.
