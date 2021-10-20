QMPlay2 - Qt Media Player 2
===========================

![Screenshot](https://raw.githubusercontent.com/zaps166/GitHubCommonContents/master/Screenshots/QMPlay2.png)

QMPlay2 is a video and audio player. It can play all formats supported by FFmpeg, libmodplug (including J2B and SFX). It also supports Audio CD, raw files, Rayman 2 music and chiptunes. It contains YouTube and MyFreeMP3 browser.

Table of Contents
=================

* [Installation](#installation)
* [YouTube](#youtube)
* [Spherical view](#spherical-view)
* [ALSA](#alsa)
* [Hardware acceleration](#hardware-acceleration)
* [Deinterlacing](#deinterlacing)
* [Hidden features](#hidden-features)
* [Multimedia keys](#multimedia-keys)
* [Installation from sources](#installation-from-sources)
* [Building package RPM, DEB or any other](#building-package-rpm-deb-or-any-other)
* [Other information](#other-information)

## Installation

#### Easy installation on Windows

- [Download](https://github.com/zaps166/QMPlay2/releases/latest) the Windows installer.

#### Easy installation on Linux (AppImage)

- [Download](https://github.com/zaps166/QMPlay2/releases/latest) the Linux AppImage.

#### Easy installation on Arch Linux / Manjaro Linux

- Install AUR client (e.g. yay),
- run the following command: `yay -S qmplay2`

#### Easy installation on openSUSE
- Don't mix FFmpeg from different repositories!
- Run the following commands:
##### openSUSE Leap 15.3
```
sudo zypper ar https://ftp.gwdg.de/pub/linux/misc/packman/suse/openSUSE_Leap_15.3 Packman
sudo zypper ar http://download.opensuse.org/repositories/multimedia:/apps/openSUSE_Leap_15.3
sudo zypper dup --allow-vendor-change --from "Packman"
sudo zypper dup --allow-vendor-change --from http://download.opensuse.org/repositories/multimedia:/apps/openSUSE_Leap_15.3
sudo zypper in QMPlay2
```
##### openSUSE Tumbleweed
```
sudo zypper ar https://ftp.gwdg.de/pub/linux/misc/packman/suse/openSUSE_Tumbleweed Packman
sudo zypper dup --allow-vendor-change --from "Packman"
sudo zypper in QMPlay2
```
#### Easy installation on Fedora
- Run the following commands:
```
sudo dnf install https://download1.rpmfusion.org/free/fedora/rpmfusion-free-release-$(rpm -E %fedora).noarch.rpm https://download1.rpmfusion.org/nonfree/fedora/rpmfusion-nonfree-release-$(rpm -E %fedora).noarch.rpm
sudo dnf groupupdate core
sudo dnf update
sudo dnf groupupdate multimedia --setop="install_weak_deps=False" --exclude=PackageKit-gstreamer-plugin
sudo dnf groupupdate sound-and-video
sudo dnf install qmplay2
```

#### Easy installation on Gentoo Linux

- Run the following command: `emerge --ask media-video/qmplay2`

## YouTube

You can change the default audio and video quality of YouTube contents. Click on the "Settings" icon on the left of the search bar, change the order of audio and/or video quality priorities and apply changes.
If the chosen quality can't be found on YouTube content, QMPlay2 will try using the next entry on the quality list.

YouTube videos don't work without external "youtube-dl" software, so QMPlay2 will download it automatically. You can remove downloaded "youtube-dl" from settings.

#### Windows

- Make sure that antivirus or firewall doesn't block "youtube-dl" and doesn't block executing external applications!

## Spherical view

QMPlay2 supports spherical view on OpenGL and Vulkan video outputs. You can watch e.g. YouTube spherical videos by pressing "Ctrl+3". You can also enable it from the menu: "Playback->Video filters->Spherical view".

## ALSA

If you are using your own ALSA configuration `asound.conf` or `.asoundrc` you should also append:
`defaults.namehint.!showall on` to the configuration file. Otherwise devices which were added may not be visible!

## Hardware acceleration

QMPlay2 supports hardware video decoding: CUVID (NVIDIA only), DXVA2 (Windows Vista and higher), D3D11VA (Vulkan, Windows 8 and higher) VDPAU/VA-API (X11 for VDPAU, Linux/BSD only) and VideoToolBox (macOS only).
Hardware acceleration is disabled by default, but you can enable it in "Settings->Playback settings":
- move hardware accelerated decoder on decoders list to the top,
- apply settings.

### Hardware acceleration important information:
- CUVID, DXVA2, VDPAU and VA-API use OpenGL video output, so OpenGL features are available, but CPU filters won't work.
- CUVID. D3D11VA and VA-API use Vulkan video output, so Vulkan features are available, but CPU filters won't work.
- DXVA2 requires "WGL_NV_DX_interop" extension.
- DXVA2 and VDPAU don't work with Vulkan.
- VDPAU, VA-API, CUVID and DXVA2 have its own deinterlacing filters. Their settings are available in "Settings->Video filters".
- VA-API on Vulkan uses its own deinterlacing filter only on Intel drivers.
- H.264 lossless movies (CRF 0 or QP 0) might not be properly decoded via VDPAU and VA-API.
- VideoToolBox doesn't support deinterlacing.

### VA-API + OpenGL information:

VA-API + OpenGL uses EGL for OpenGL context creation. On X11 QMPlay2 tries to detect if EGL can be used, but the detection can fail. In this case you can try do it manually: `export QT_XCB_GL_INTEGRATION=xcb_glx` and run QMPlay2 from command line. If everything is working properly, you can export this variable globally. This doesn't work on NVIDIA drivers. In case of multiple GPUs installed in system VA-API requires to use the same device as OpenGL. QMPlay2 detects it automatically, but if the detection fails, try to do it manually, e.g.: `export QMPLAY2_EGL_CARD_FILE_PATH=/dev/dri/card1` and run QMPlay2 from command line. If everything is working properly, you can export this variable globally.

## Deinterlacing

Video interlacing is automatically detected by QMPlay2. Go to "Settings->Video filters" for more options.
If you have fast CPU (or low video resolution), you can use "Yadif 2x" deinterlacing filter for better quality.

You can enable deinterlacing filter on non-interlaced video if necessary (some interlaced videos may not have interlacing data),
but remember to revert this setting on any other video! Otherwise the video quality and performance will be worse!

Hardware-accelerated video decoding uses its own video filtering, so the CPU deinterlacing method (e.g. "Yadif 2x") does nothing in this case.
Of course you can adjust other deinterlacing settings in case of hardware acceleration.

Vulkan renderer has Yadif deinterlacing filter which is used by default for CPU decoded videos. You can change this behavior in Vulkan renderer settings. Moreover, Yadif Vulkan filter is used for hardware-decoded videos.

Chroma plane if pixel format is not YUV420 when XVideo or DirectDraw is used as video output may not be properly deinterlaced.

## Hidden features

### Audio balance

Right click on volume slider and select "Split channels".

### Last.fm scrobbling

Go to "Options->Modules settings" and click "Extensions" on the list. Find "LastFM" group box, select "Scrobble", type your login and password and then press "Apply".

### Control files

Empty files in `share` directory (on Windows it is a directory with `QMPlay2.exe`):
- `portable` - runs QMPlay2 in portable mode (settings are stored in applications directory),
- `noautoupdates` - disables auto-updates at first run.

### Still images

Go to "Options->settings" and check "Read and display still images".

### Custom user agent

You can specify a `CustomUserAgent` in `QMPlay2.ini` file in `General` section.

### Single instance

You can force single instance for QMPlay2: set "Allow only one instance" in "Settings->General settings".

## Multimedia keys

In Windows and macOS, multimedia keys should work automatically.

In Linux/BSD, you must associate keys with commands:
- using QMPlay2 binary, see: `QMPlay2 -h`,
- using MPRIS2:
    - Toggle play/pause: `dbus-send --print-reply --dest=org.mpris.MediaPlayer2.QMPlay2 /org/mpris/MediaPlayer2 org.mpris.MediaPlayer2.Player.PlayPause`.
    - Next: `dbus-send --print-reply --dest=org.mpris.MediaPlayer2.QMPlay2 /org/mpris/MediaPlayer2 org.mpris.MediaPlayer2.Player.Next`
    - Prev: `dbus-send --print-reply --dest=org.mpris.MediaPlayer2.QMPlay2 /org/mpris/MediaPlayer2 org.mpris.MediaPlayer2.Player.Previous`
    - Stop: `dbus-send --print-reply --dest=org.mpris.MediaPlayer2.QMPlay2 /org/mpris/MediaPlayer2 org.mpris.MediaPlayer2.Player.Stop`

## Installation from sources

Don't forget to update submodules: `git submodule update --init`.

### CMake requirements

For CMake build be sure that you have CMake 3.9 or higher.

### You need devel packages:

#### Necessary:
- Qt5 >= 5.10:
    - Qt5DBus - Linux/BSD only,
    - Qt5Svg - for SVG icons,
    - Qt5Qml - for MediaBrowser,
    - Qt5WinExtras - for Windows,
- FFmpeg >= 4.0:
    - libavformat - requires OpenSSL or GnuTLS for https support,
    - libavcodec - for FFmpeg module only,
    - libswscale,
    - libavutil,
    - libswresample,
    - libavdevice - for FFmpeg module only, optional (enabled on Linux as default),

#### Important:
- TagLib >= 1.9,
- libass - for OSD and non-graphical subtitles.

#### For modules (some of them can be automatically disabled if not found):
- FFmpeg (necessary module): libva (VA-API) and libvdpau (VDPAU, X11 only),
- Chiptune: libgme (kode54 version is recommended) and libsidplayfp,
- DirectX (Windows only): DirectDraw SDK (included in mingw-w64),
- AudioCD: libcdio and libcddb,
- ALSA (Linux only): libasound,
- PulseAudio - libpulse-simple,
- PortAudio: portaudio (v19),
- XVideo (X11 only): libxv.

### Dependencies:

##### Arch Linux / Manjaro Linux dependencies

- Common packages:
```
$ sudo pacman -S cmake make gcc pkg-config ffmpeg libass libva libxv alsa-lib libcdio taglib libcddb libpulse libgme libsidplayfp qt5-base qt5-tools
```

### Running the compilation for Linux/BSD using CMake:

- Install all needed packages and dependencies (in devel version) using package manager or compile it from sources.
- You can use `cmake-gui` for graphical configuration. Otherwise follow the instructions below:
    - create a "build" directory and go to it: `mkdir build && cd build`,
    - run CMake (also you can run with arguments which you want): `cmake ..`,
    - check the summary - which features are enabled - you can set/force them manually,
    - if CMake finishes without errors, run: `make -j8` (replace 8 with numbers of CPU threads),
    - if compiling finishes without errors, install it: `sudo make -j8 install`.

CMake options (option - default value: description):
- CMake options and the default settings:
    - `CMAKE_INSTALL_PREFIX` - mostly it is `/usr/local`: installation directory.
    - `CMAKE_BUILD_TYPE` - `Release`.
    - `LANGUAGES` - `All` - a space-separated list of translations to compile into QMPlay2.
    - `SOLID_ACTIONS_INSTALL_PATH` - Linux/BSD only, autodetect: you can specify the path manually.
    - `SET_INSTALL_RPATH` - non-Windows only, `ON` on macOS, `OFF` anywhere else: sets RPATH after installation.
    - `USE_FFMPEG` - ON: enable/disable FFmpeg module.
    - `USE_FFMPEG_VAAPI`: autodetect: enabled if libva, libva-drm, and egl exist.
    - `USE_FFMPEG_VDPAU`: autodetect: enabled on X11 if libvdpau exists.
    - `USE_FFMPEG_AVDEVICE` - autodetect on Linux, `OFF` on non-Linux OS: it allows to use e.g. V4L2 devices.
    - `USE_INPUTS` - ON: enable/disable Inputs module.
    - `USE_MODPLUG` - ON: enable/disable Modplug module.
    - `USE_EXTENSIONS` - ON: enable/disable Extensions module.
    - `USE_MPRIS2` - Linux/BSD only, `ON`: enable/disable MPRIS2 in Extensions module.
    - `USE_VISUALIZATIONS` - ON: enable/disable Visualizations module.
    - `USE_AUDIOFILTERS` - ON: enable/disable AudioFilters module.
    - `USE_VIDEOFILTERS` - ON: enable/disable VideoFilters module.
    - `USE_OPENGL` - `ON`: enable/disable OpenGL support.
    - `USE_VULKAN` - autodetect: enable/disable Vulkan support.
    - `USE_GLSLC` - `OFF`: enable/disable GLSL -> SPIR-V shader compilation when building QMPlay2.
    - `USE_AUDIOCD` - autodetect: enabled if libcdio and libcddb exist: enable/disable AudioCD module.
    - `USE_ALSA` - `ON` on Linux: enable/disable ALSA module.
    - `USE_PORTAUDIO` - `ON` on non-Linux OS: enable/disable PortAudio module.
    - `USE_PULSEAUDIO` - autodetect on Linux/BSD, `OFF` anywhere else: enable/disable PulseAudio module.
    - `USE_XVIDEO` - autodetect on X11: enabled if libxv exists: enable/disable XVideo module.
    - `USE_CHIPTUNE_GME` - autodetect: enabled if libgme exists.
    - `USE_CHIPTUNE_SID` - autodetect: enabled if libsidplayfp exists.
    - `USE_TAGLIB` - `ON`: enable/disable tag editor.
    - `USE_CMD` - Windows only, `OFF`.
    - `USE_LASTFM` - `ON`: enable/disable LastFM in Extensions module.
    - `USE_LIBASS` - `ON`: enable/disable libass (subtitles engine) dependency.
    - `USE_CUVID` - `ON`: enable/disable CUVID module.
    - `USE_LYRICS` - `ON`: enable/disable lyrics module.
    - `USE_MEDIABROWSER` - `ON`: enable/disable MediaBrowser module.
    - `USE_RADIO` - `ON`: enable/disable Radio Browser module.
    - `USE_YOUTUBE` - `ON`: enable/disable YouTube module.
    - `USE_ASAN` - `OFF`: enable/disable address sanitizer.
    - `USE_UBSAN` - `OFF`: enable/disable undefined behavior sanitizer.
    - `USE_LINK_TIME_OPTIMIZATION` - `OFF`: enable/disable Link Time Optimization for release builds.
    - `USE_GIT_VERSION` - `ON`: append Git HEAD to QMPlay2 version (if exists).
    - `USE_UPDATES` - `ON`: enable/disable software updates.
    - `FIND_HWACCEL_DRIVERS_PATH` - `OFF`: Find drivers path for hwaccel, useful for universal package.

Using other Qt installation of CMake:
- `Qt5Widgets_DIR`: path to the Qt5Widgets cmake directory (e.g. `~/qtbase/lib/cmake/Qt5Widgets`).
- `Qt5DBus_DIR`: path to the Qt5DBus cmake directory (e.g. `~/qtbase/lib/cmake/Qt5DBus`).
- `Qt5LinguistTools_DIR`: path to the Qt5LinguistTools cmake directory (e.g. `~/qtbase/lib/cmake/Qt5LinguistTools`).
- `Qt5Svg_DIR`: path to the Qt5Svg cmake directory (e.g. `~/qtbase/lib/cmake/Qt5Svg`).
- `Qt5Qml_DIR`: path to the Qt5Qml cmake directory (e.g. `~/qtbase/lib/cmake/Qt5Qml`).

Every CMake option must be prepended with `-D` and new value is set after `=`.

You can strip binaries during installation to save disk space: `sudo make -j8 install/strip`.

Example commands (execute it in QMPlay2 directory with source code):

- Simple installation (rely on autodetection, `strip` reduces size, but it makes debugging unavailable):

```sh
$ mkdir build
$ cd build
$ cmake .. -DCMAKE_INSTALL_PREFIX=/usr
$ make -j8
$ sudo make -j8 install/strip
```

- Use manually-specified install prefix, disable SID, use Polish language only and use manually-specified Solid actions path:

```sh
$ mkdir build
$ cd build
$ cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DUSE_CHIPTUNE_SID=OFF -DLANGUAGES="pl" -DSOLID_ACTIONS_INSTALL_PATH="/usr/share/solid/actions"
$ make -j8
$ sudo make -j8 install
```

- Update icon theme, mime and desktop database (replace `/usr/share/` by your install prefix path):

```sh
$ sudo update-desktop-database
$ sudo update-mime-database /usr/share/mime
$ sudo gtk-update-icon-cache /usr/share/icons/hicolor
```

- Uninstallation (in `build` directory):

```sh
$ sudo make uninstall
```

#### macOS:

- Install [Brew](http://brew.sh) and developer command line tools.
- Download and install Qt5 for macOS (ignore errors about missing XCode if any).
- Download and install CMake for macOS:
 - create symlink for `cmake` CLI (from Bundle) to `/usr/local/bin`.
- Install `pkg-config` and all dependencies from Brew.
 - you can also compile them manually (especially `libass`, `ffmpeg` (w/o most encoders), `fribidi` (w/o `glib`) and `libsidplayfp`).
- Set missing directories for dependencies (QtCreator or CMake GUI are recommended):
 - set install prefix e.g. to Desktop directory (for Bundle),
 - install via `make install` - this creates a Bundle.

#### Windows (cross-compilation):

- Install all required MinGW packages (I recommend AUR MinGW packages).
- Some libraries are incompatible, use unneeded dependencies or don't exist in repository - you should built them on your own.
- Run `cmake` from cross-compilation toolchain.

##### Other information for Windows

- You can compile QMPlay2 on Windows host, but you must prepare toolchain for your own (also with CMake advanced options for libraries and paths):
 - `CMAKE_LIBRARY_PATH` - where to find QMPlay2 dependency libraries,
 - `CMAKE_INCLUDE_PATH`- where to find QMPlay2 dependency includes,
 - `CUSTOM_*_LIBRARIES` - additional custom libraries for linker (useful for static linking),
- I use my own PKGBUILDs for many MinGW libraries.
- Visual Studio can't compile QMPlay2.

## Building package RPM, DEB or any other

You can look at [Arch Linux PKGBUILD](https://aur.archlinux.org/cgit/aur.git/tree/PKGBUILD?h=qmplay2).

## Other information

- QMPlay2 contains modified libmodplug sources which are used by Modplug module.
- QMPlay2 uses Concept icons created by Alexey Varfolomeev.
