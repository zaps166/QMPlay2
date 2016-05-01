#QMPlay2 - Qt Media Player 2

>QMPlay2 is a video and audio player. It can play all formats supported by FFmpeg, libmodplug (including J2B and SFX). It also supports Audio CD, raw files, Rayman 2 music and chiptunes. It contains YouTube and Prostopleer browser.

##Instalation

####Easy installation on Windows (XP or higher)

- Download the newest "*.exe" file from http://sourceforge.net/projects/zaps166/files/QMPlay2 for your CPU architecture and install it:
	- 64-bit Windows: http://zaps166.sf.net/?QMPlay2Download=Win64Installer
	- 32-bit Windows: http://zaps166.sf.net/?QMPlay2Download=Win32Installer

####Easy installation on Arch Linux / Manjaro Linux

- Arch Linux only: Install AUR client (e.g. yaourt),
- run the following command:
```sh
$ yaourt -S qmplay2
```

####Easy installation on openSUSE Leap 42.1

- Just run the following commands:
```sh
$ sudo zypper ar http://packman.inode.at/suse/openSUSE_Leap_42.1 Packman
$ sudo zypper in QMPlay2
```
- Qt5 "multimedia" package may be buggy on Qt5 <= 5.6.1 and its FFmpeg doesn't have all codecs.
- Don't use official package, because it is obsolete.

####Easy installation on openSUSE 13.2

- Just run the following commands:
```sh
$ sudo zypper ar http://packman.inode.at/suse/openSUSE_13.2 Packman
$ sudo zypper in QMPlay2 QMPlay2-kde-integration
```

##YouTube

Some YouTube videos doesn't work without the external "youtube-dl" software. You must install it!

####Windows

- Watch the video: http://zaps166.sf.net/downloads/QMPlay2_youtube-dl.mp4
- If the video doesn't play inside the web browser - pase the address to QMPlay2 (Ctrl+V) and play it!
- Windows XP: I don't know why, but "youtube-dl.exe" needs .NET 3.5.

####Arch Linux / Manjaro Linux

- Install "youtube-dl" from repositories: ```sudo pacman -S youtube-dl```

####Other Linux distributions or other operating system

- Don't use "youtube-dl" from repositories - usually it contains too old package which no longer works. The exception is Arch Linux and Manjaro Linux.
- Download "youtube-dl": http://rg3.github.io/youtube-dl/download.html
- Set the path for the script (you must have Python installed) in QMPlay2 YouTube settings (you can watch the video from Windows installation).
- QMPlay2 can automatically updates the "youtube-dl" if it is not installed from repositories.

##ALSA

If you are using your own ALSA configuration ```asound.conf``` or ```.asoundrc``` you should also append:
```defaults.namehint.!showall on``` to the configuration file. Otherwise devices which were added may not be visible!

##Ubuntu Unity

Tray context menu in Ubuntu Unity doesn't work properly on Qt older than 5.6.1!

##Compilation from sources

###You need (devel packages):

- Qt4 or Qt5 (including QtOpenGL\*, OpenSSL for https, QtDBus on Linux/BSD and QX11Extras for Qt5),
- FFmpeg >= 2.2 (libavcodec, libavformat, libavutil, libswscale, libswresample or libavresample\*\*, libavdevice\*\*\*, OpenSSL for https),
- portaudio (default on non-Linux OS, if you want to compile portaudio module on Linux, change "src/modules/modules.pro"),
- pulseaudio (optional, if you don't want to compile pulseaudio on Linux, remove it from "src/modules/modules.pro"),
- taglib >= 1.7 (>= 1.9 recommended) (you can disable it in "src/gui/gui.pro"),
- libgme and libsidplayfp - for Chiptune module,
- libva (vaapi) and libvdpau - only on X11,
- libasound (for ALSA module on Linux),
- DirectDraw SDK - only on Windows,
- libcdio and libcddb,
- libxv - only on X11,
- libass.

\* QtOpenGL is not used on Qt >= 5.6,<br/>
\*\* libavresample: uncomment last 8 lines in "src/qmplay2/qmplay2.pro",<br/>
\*\*\* libavdevice: only Linux.

###Running compilation script:

####Linux/BSD*:

#####Arch Linux / Manjaro Linux dependencies

- Common packages: ```sudo pacman -S make gcc pkg-config ffmpeg libass libva libxv alsa-lib libcdio taglib libcddb libpulse libgme libsidplayfp```
- Qt5 build (recommend for Qt5 >= 5.6.1): ```sudo pacman -S qt5-base qt5-x11extras qt5-tools```
- Qt4 build: ```sudo pacman -S qt4```

You can also install youtube-dl: ```sudo pacman -S youtube-dl```

#####OpenSUSE dependencies (Qt4 build)

- Add Packman repository for FFmpeg with all codecs:
	- openSUSE Leap 42.1: ```sudo zypper ar http://packman.inode.at/suse/openSUSE_Leap_42.1 Packman```
	- openSUSE 13.2: ```sudo zypper ar http://packman.inode.at/suse/openSUSE_13.2 Packman```
- Install dependencies: ```sudo zypper in libqt4-devel gcc-c++ alsa-devel libpulse-devel libass-devel libtag-devel libcdio-devel libcddb-devel libXv-devel Mesa-devel libsidplayfp-devel libgme-devel libva-devel libvdpau-devel libavcodec-devel libavformat-devel libavutil-devel libswscale-devel libswresample-devel libavdevice-devel```

#####Ubuntu

- Open "Software & Updates" and select the "Universe" repository.
- Ubuntu doesn't provide pkg-config for libgme, so if you want to use Game-Music-Emu for chiptunes, you must install "libgme-dev" and manually modify "src/modules/Chiptune/Chiptune.pro".

#####Ubuntu 16.04 and higher dependencies (Qt4 build)

- Install dependencies from the package manager:
```sudo apt-get install g++ libqt4-dev libasound2-dev libass-dev libcdio-dev libcddb2-dev libsidplayfp-dev libxv-dev libtag1-dev libpulse-dev libva-dev libvdpau-dev libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libswresample-dev libavdevice-dev```

#####Ubuntu 15.04 and 15.10 dependencies (Qt4 build)

- Install dependencies from the package manager:
```sudo apt-get install g++ libqt4-dev libasound2-dev libass-dev libcdio-dev libcddb2-dev libsidplayfp-dev libxv-dev libtag1-dev libpulse-dev libva-dev libvdpau-dev libavcodec-ffmpeg-dev libavformat-ffmpeg-dev libavutil-ffmpeg-dev libswscale-ffmpeg-dev libswresample-ffmpeg-dev libavdevice-ffmpeg-dev```

#####Ubuntu 14.10 and older dependencies (Qt4 build)

Ubuntu <= 14.10 uses old LibAV instead of the new FFmpeg (>= 2.2 is necessary), so the FFmpeg must be compiled from sources and the LibAV development files must be removed!

- Install dependencies from the package manager:
```sudo apt-get install g++ yasm libqt4-dev libasound2-dev libass-dev libcdio-dev libcddb2-dev libsidplayfp-dev libxv-dev libtag1-dev libpulse-dev libssl-dev libva-dev libvdpau-dev```
- Remove LibAV devel files for the compilation time (this is mandatory, otherwise QMPlay2 will link to the old LibAV libraries and it will crash at runtime!):
```sudo apt-get remove libavformat-dev libavcodec-dev libavresample-dev libavdevice-dev libavutil-dev```. You can install it again after compilation.
- Download the newest FFmpeg from http://ffmpeg.org/download.html and unpack it. Then write a command:
```./configure --prefix=/usr/local --enable-shared --disable-static --enable-openssl --disable-avfilter --disable-encoders --disable-muxers --disable-programs && make -j4 && sudo make -j4 install```<br/>
This will compile and install the newest FFmpeg without features that are not supported in QMPlay2.
- Before QMPlay2 compilation please be sure that you have removed LibAV development packages from repositories!

####Compilation

- Install all dependencies using package manager (in devel version) or compile it from sources.
- If you want to compile with qt suffix (for example "qmake-qt5") - "export QT_SUFFIX=-qt5".
- Qt5 is used by default since Qt 5.6.1 version, so "export QT_SUFFIX=-qt4" for Qt4 in this case.
- Compilation only:
	- If you want to prepare *.desktop files for system use - "export SYSTEM_BUILD=1".
	- Run "./compile_unix".
	- QMPlay2 is in "app" directory, you can move its contents into "/usr" directory if "$SYSTEM_BUILD == 1".
- Compilation and installation:
	- Run "./installer_unix install" (it compiles and uses "sudo" to copy files to "/usr").
	- If you want to uninstall, run "./installer_unix uninstall" (it also uses "sudo").

*BSD - not tested

####OS X:

- Download and install Xcode.
- Download and install the newest Qt for Mac.
- Download, compile and install "pkg-config".
- Download and install CMake for Mac (for taglib).
- Download, compile and install all dependencies from sources.
- Add directory containing "qmake" to "PATH".
- Run "./compile_mac n" where "n" is number of threads (4 by default).

####Windows (cross-compilation):

- Install all required MinGW packages (I recommend Arch Linux unofficial MinGW repository).
- Some libraries are incompatible, uses unneeded libraries or doesn't exists in repository - you must built them on your own.
- Notice that QMPlay2 uses static linking for some libraries.
- Run "./compile_win_cross".

#####Other information

- You can also compile it on Windows, but you must build toolchain for your own!
- I'm using my own PKGBUILDs for many MinGW libraries.
- Visual Studio can't compile QMPlay2.

##Building package RPM, DEB or any other

- Look at "installer_unix" script or Arch Linux PKGBUILD: https://aur.archlinux.org/cgit/aur.git/tree/?h=qmplay2
- QMPlay2 sometimes uses the external software - "youtube-dl", so it should be added as optional package.
- QMPlay2 has non-standard MIME types in "src/gui/Unix/x-*.xml", so they should be registered during installing from package.
