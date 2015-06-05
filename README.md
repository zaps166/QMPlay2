#QMPlay2 - QT Media Player 2

>QMPlay2 is a video and audio player. It can play all formats supported by ffmpeg, libmodplug (including J2B and SFX). It also supports Audio CD, raw files and Rayman 2 music. It contains YouTube and Prostopleer browser.

##Compilation and instalation

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

- Install all required MinGW packages (I recommend ArchLinux unofficial MinGW repository).
- Some libraries are incompatible, uses unneeded libraries or doesn't exists in repository - you must built them on your own.
- Notice that QMPlay2 uses static linking for some libraries.
- Run "./compile_win_cross".

####Other information

- You can also compile it on Windows, but you must build toolchain for your own!
- I'm using my own PKGBUILDs for many MinGW libraries.
- Visual Studio can't compile QMPlay2.

###Linux/BSD:

- If you don't want to open Xterm or Konsole - "export NOTERM=1".
- If you want to prepare *.desktop files for system use - "export SYSTEM_BUILD=1".
- If you want to compile with qt suffix (for example "qmake-qt5") - "export QT_SUFFIX=-qt5".
- Run "./compile_unix".
- QMPlay2 is in "app" directory, you can move its contents into /usr directory if $SYSTEM_BUILD == 1.

Building package RPM, DEB or any other - look at Arch Linux PKGBUILD:
>http://aur.archlinux.org/packages/qm/qmplay2-git/PKGBUILD

QMPlay2 uses the external software - "youtube-dl", so "youtube-dl" should be as optional package in package manager.

####Easy installation on openSUSE

- Just run the following commands:
```sh
$ sudo zypper ar http://packman.inode.at/suse/openSUSE_13.2 Packman
$ sudo zypper in QMPlay2 QMPlay2-kde-integration
```

####Easy installation on Ubuntu

- Just run the following commands:
```sh
$ sudo add-apt-repository ppa:samrog131/ppa
$ sudo apt-get update
$ sudo apt-get install qmplay2
```
- If you get dependency issues, do:
```sh
$ sudo apt-get install -f
$ sudo apt-get install qmplay2
```
