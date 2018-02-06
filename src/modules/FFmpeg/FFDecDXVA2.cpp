/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2018  Błażej Szczygieł

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published
	by the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <FFDecDXVA2.hpp>

#include <HWAccelInterface.hpp>
#include <HWAccelHelper.hpp>
#include <VideoWriter.hpp>
#include <StreamInfo.hpp>
#include <ImgScaler.hpp>
#include <FFCommon.hpp>

#include <QMultiMap>
#include <QLibrary>
#include <QDebug>

#include <GL/gl.h>
#include <GL/wglext.h>

using Size = QPair<quint32, quint32>;

namespace DX
{
	using  Direct3DCreate9ExType = HRESULT (WINAPI *)(UINT SDKVersion, IDirect3D9Ex **ppD3D);
	static Direct3DCreate9ExType Direct3DCreate9Ex;

	using  DXVA2CreateDirect3DDeviceManager9Type = HRESULT (WINAPI *)(UINT *pResetToken,IDirect3DDeviceManager9 **ppDXVAManager);
	static DXVA2CreateDirect3DDeviceManager9Type DXVA2CreateDirect3DDeviceManager9;
}

extern "C"
{
	#include <libavcodec/avcodec.h>
	#include <libswscale/swscale.h>
}

#include <initguid.h>

/* GUIDs from FFmpeg code */

DEFINE_GUID(IID_IDirectXVideoDecoderService, 0xfc51a551, 0xd5e7, 0x11d9, 0xaf, 0x55, 0x00, 0x05, 0x4e, 0x43, 0xff, 0x02);

DEFINE_GUID(DXVA2_ModeMPEG2_VLD,             0xee27417f, 0x5e28, 0x4e65, 0xbe, 0xea, 0x1d, 0x26, 0xb5, 0x08, 0xad, 0xc9);
DEFINE_GUID(DXVA2_ModeMPEG2and1_VLD,         0x86695f12, 0x340e, 0x4f04, 0x9f, 0xd3, 0x92, 0x53, 0xdd, 0x32, 0x74, 0x60);
DEFINE_GUID(DXVA2_ModeH264_E,                0x1b81be68, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVA2_ModeH264_F,                0x1b81be69, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVADDI_Intel_ModeH264_E,        0x604F8E68, 0x4951, 0x4C54, 0x88, 0xFE, 0xAB, 0xD2, 0x5C, 0x15, 0xB3, 0xD6);
DEFINE_GUID(DXVA2_ModeVC1_D,                 0x1b81beA3, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVA2_ModeVC1_D2010,             0x1b81beA4, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(DXVA2_ModeHEVC_VLD_Main,         0x5b11d51b, 0x2f4c, 0x4452, 0xbc, 0xc3, 0x09, 0xf2, 0xa1, 0x16, 0x0c, 0xc0);
DEFINE_GUID(DXVA2_ModeHEVC_VLD_Main10,       0x107af0e0, 0xef1a, 0x4d19, 0xab, 0xa8, 0x67, 0xa1, 0x63, 0x07, 0x3d, 0x13);
DEFINE_GUID(DXVA2_ModeVP9_VLD_Profile0,      0x463707f8, 0xa1d0, 0x4585, 0x87, 0x6d, 0x83, 0xaa, 0x6d, 0x60, 0xb8, 0x9e);

DEFINE_GUID(DXVA2_NoEncrypt,                 0x1b81beD0, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);

static QMultiMap<AVCodecID, const GUID *> GUIDToAVCodec;

/**/

class DXVA2Hwaccel : public HWAccelInterface
{
public:
	inline DXVA2Hwaccel(IDirect3DDevice9 *d3d9Device, const bool dontReleaseRenderTarget) :
		m_dontReleaseRenderTarget(dontReleaseRenderTarget),
		m_d3d9Device(d3d9Device),
		m_glHandleD3D(nullptr),
		m_videoDecoder(nullptr),
		m_renderTarget(nullptr),
		m_glHandleSurface(nullptr)
	{
		m_d3d9Device->AddRef();
	}
	~DXVA2Hwaccel() final
	{
		releaseSurfacesAndDecoder();
		if (m_dontReleaseRenderTarget)
		{
			for (auto it = m_renderTargets.constBegin(), itEnd = m_renderTargets.constEnd(); it != itEnd; ++it)
				it.value()->Release();
		}
		m_d3d9Device->Release();
	}

	QString name() const override
	{
		return "DXVA2";
	}

	Format getFormat() const override
	{
		return RGB32;
	}

	bool init(quint32 *textures) override
	{
		using  wglGetProcAddressType = PROC(WINAPI *)(LPCSTR);
		static wglGetProcAddressType wglGetProcAddress = (wglGetProcAddressType)QLibrary::resolve("opengl32", "wglGetProcAddress");
		if (!wglGetProcAddress)
			return false;

		wglDXOpenDeviceNV = (PFNWGLDXOPENDEVICENVPROC)wglGetProcAddress("wglDXOpenDeviceNV");
		wglDXSetResourceShareHandleNV = (PFNWGLDXSETRESOURCESHAREHANDLENVPROC)wglGetProcAddress("wglDXSetResourceShareHandleNV");
		wglDXRegisterObjectNV = (PFNWGLDXREGISTEROBJECTNVPROC)wglGetProcAddress("wglDXRegisterObjectNV");
		wglDXLockObjectsNV = (PFNWGLDXLOCKOBJECTSNVPROC)wglGetProcAddress("wglDXLockObjectsNV");
		wglDXUnlockObjectsNV = (PFNWGLDXUNLOCKOBJECTSNVPROC)wglGetProcAddress("wglDXUnlockObjectsNV");
		wglDXUnregisterObjectNV = (PFNWGLDXUNREGISTEROBJECTNVPROC)wglGetProcAddress("wglDXUnregisterObjectNV");
		wglDXCloseDeviceNV = (PFNWGLDXCLOSEDEVICENVPROC)wglGetProcAddress("wglDXCloseDeviceNV");
		if (!wglDXOpenDeviceNV || !wglDXSetResourceShareHandleNV || !wglDXRegisterObjectNV || !wglDXLockObjectsNV || !wglDXUnlockObjectsNV || !wglDXUnregisterObjectNV || !wglDXCloseDeviceNV)
			return false;

		if (m_dontReleaseRenderTarget)
			m_renderTarget = m_renderTargets.value(m_size);

		if (!m_renderTarget)
		{
			HANDLE sharedHande = nullptr;
			HRESULT hr;

			hr = m_d3d9Device->CreateRenderTarget(m_size.first, m_size.second, D3DFMT_X8R8G8B8, D3DMULTISAMPLE_NONE, 0, false, &m_renderTarget, &sharedHande);
			if (FAILED(hr))
				return false;

			// Don't release render target on Intel drivers, bacause "wglDXSetResourceShareHandleNV()" can fail.
			// Render target must be created here on Radeon drivers, otherwise "wglDXRegisterObjectNV()" will fail.
			if (m_dontReleaseRenderTarget)
				m_renderTargets[m_size] = m_renderTarget;

			if (!wglDXSetResourceShareHandleNV(m_renderTarget, sharedHande))
				return false;
		}

		if (!m_glHandleD3D)
		{
			m_glHandleD3D = wglDXOpenDeviceNV(m_d3d9Device);
			if (!m_glHandleD3D)
				return false;
		}

		m_glHandleSurface = wglDXRegisterObjectNV(m_glHandleD3D, m_renderTarget, *textures, GL_TEXTURE_2D, WGL_ACCESS_READ_ONLY_NV);
		if (!m_glHandleSurface)
			return false;

		if (!wglDXLockObjectsNV(m_glHandleD3D, 1, &m_glHandleSurface))
			return false;

		return true;
	}
	void clear(bool contextChange) override
	{
		if (m_glHandleSurface)
		{
			wglDXUnlockObjectsNV(m_glHandleD3D, 1, &m_glHandleSurface);
			wglDXUnregisterObjectNV(m_glHandleD3D, m_glHandleSurface);
			m_glHandleSurface = nullptr;
		}
		if (contextChange && m_glHandleD3D)
		{
			wglDXCloseDeviceNV(m_glHandleD3D);
			m_glHandleD3D = nullptr;
		}
		if (!m_dontReleaseRenderTarget && m_renderTarget)
		{
			m_renderTarget->Release();
			m_renderTarget = nullptr;
		}
	}

	CopyResult copyFrame(const VideoFrame &videoFrame, Field field) override
	{
		Q_UNUSED(field);

		if (!wglDXUnlockObjectsNV(m_glHandleD3D, 1, &m_glHandleSurface))
			return CopyError;

		IDirect3DSurface9 *surface = (IDirect3DSurface9 *)videoFrame.surfaceId;
		const RECT rect = {0, 0, videoFrame.size.width, videoFrame.size.height};
		HRESULT hr = m_d3d9Device->StretchRect(surface, &rect, m_renderTarget, &rect, D3DTEXF_NONE);

		if (!wglDXLockObjectsNV(m_glHandleD3D, 1, &m_glHandleSurface) || FAILED(hr))
			return CopyError;

		return CopyOk;
	}

	bool getImage(const VideoFrame &videoFrame, void *dest, ImgScaler *nv12ToRGB32) override
	{
		D3DSURFACE_DESC desc;
		D3DLOCKED_RECT lockedRect;
		IDirect3DSurface9 *surface = (IDirect3DSurface9 *)videoFrame.surfaceId;
		if (SUCCEEDED(surface->GetDesc(&desc)) && SUCCEEDED(surface->LockRect(&lockedRect, nullptr, D3DLOCK_READONLY)))
		{
			const void *src[2] = {
				lockedRect.pBits,
				(quint8 *)lockedRect.pBits + (lockedRect.Pitch * desc.Height)
			};
			const int srcLinesize[2] = {
				lockedRect.Pitch,
				lockedRect.Pitch
			};
			nv12ToRGB32->scale(src, srcLinesize, dest);
			surface->UnlockRect();
			return true;
		}
		return false;
	}

	/**/

	inline IDirect3DDevice9 *getD3D9Device() const
	{
		return m_d3d9Device;
	}

	void setNewData(const Size &size, IDirectXVideoDecoder *videoDecoder, FFDecDXVA2::Surfaces &surfaces)
	{
		releaseSurfacesAndDecoder();

		m_videoDecoder = videoDecoder;
		m_surfaces = surfaces;

		m_videoDecoder->AddRef();
		for (IDirect3DSurface9 *surface : *m_surfaces)
			surface->AddRef();

		m_size = size;
	}

private:
	void releaseSurfacesAndDecoder()
	{
		if (m_surfaces)
		{
			for (IDirect3DSurface9 *surface : *m_surfaces)
				surface->Release();
		}
		if (m_videoDecoder)
			m_videoDecoder->Release();
	}

private:
	const bool m_dontReleaseRenderTarget;

	PFNWGLDXOPENDEVICENVPROC wglDXOpenDeviceNV;
	PFNWGLDXSETRESOURCESHAREHANDLENVPROC wglDXSetResourceShareHandleNV;
	PFNWGLDXREGISTEROBJECTNVPROC wglDXRegisterObjectNV;
	PFNWGLDXLOCKOBJECTSNVPROC wglDXLockObjectsNV;
	PFNWGLDXUNLOCKOBJECTSNVPROC wglDXUnlockObjectsNV;
	PFNWGLDXUNREGISTEROBJECTNVPROC wglDXUnregisterObjectNV;
	PFNWGLDXCLOSEDEVICENVPROC wglDXCloseDeviceNV;

	IDirect3DDevice9 *m_d3d9Device;
	HANDLE m_glHandleD3D;

	IDirectXVideoDecoder *m_videoDecoder;
	FFDecDXVA2::Surfaces m_surfaces;

	IDirect3DSurface9 *m_renderTarget;
	HANDLE m_glHandleSurface;

	QMap<Size, IDirect3DSurface9 *> m_renderTargets;
	Size m_size;
};

/**/

bool FFDecDXVA2::loadLibraries()
{
	QLibrary d3d9("d3d9");
	QLibrary dxva2("dxva2");
	if (dxva2.load() && d3d9.load())
	{
		DX::Direct3DCreate9Ex = (DX::Direct3DCreate9ExType)d3d9.resolve("Direct3DCreate9Ex");
		DX::DXVA2CreateDirect3DDeviceManager9 = (DX::DXVA2CreateDirect3DDeviceManager9Type)dxva2.resolve("DXVA2CreateDirect3DDeviceManager9");
		if (DX::Direct3DCreate9Ex && DX::DXVA2CreateDirect3DDeviceManager9)
		{
			GUIDToAVCodec.insert(AV_CODEC_ID_MPEG2VIDEO, &DXVA2_ModeMPEG2_VLD);
			GUIDToAVCodec.insert(AV_CODEC_ID_MPEG2VIDEO, &DXVA2_ModeMPEG2and1_VLD);

			GUIDToAVCodec.insert(AV_CODEC_ID_H264, &DXVA2_ModeH264_F);
			GUIDToAVCodec.insert(AV_CODEC_ID_H264, &DXVA2_ModeH264_E);
			GUIDToAVCodec.insert(AV_CODEC_ID_H264, &DXVADDI_Intel_ModeH264_E);

			GUIDToAVCodec.insert(AV_CODEC_ID_VC1,  &DXVA2_ModeVC1_D2010);
			GUIDToAVCodec.insert(AV_CODEC_ID_WMV3, &DXVA2_ModeVC1_D2010);
			GUIDToAVCodec.insert(AV_CODEC_ID_VC1,  &DXVA2_ModeVC1_D);
			GUIDToAVCodec.insert(AV_CODEC_ID_WMV3, &DXVA2_ModeVC1_D);

			GUIDToAVCodec.insert(AV_CODEC_ID_HEVC, &DXVA2_ModeHEVC_VLD_Main);
			GUIDToAVCodec.insert(AV_CODEC_ID_HEVC, &DXVA2_ModeHEVC_VLD_Main10);

			GUIDToAVCodec.insert(AV_CODEC_ID_VP9, &DXVA2_ModeVP9_VLD_Profile0);

			return true;
		}
	}
	return false;
}

FFDecDXVA2::FFDecDXVA2(QMutex &avcodec_mutex, Module &module) :
	FFDecHWAccel(avcodec_mutex),
	m_copyVideo(Qt::Unchecked),
	m_d3d9Device(nullptr),
	m_devMgr(nullptr),
	m_devHandle(INVALID_HANDLE_VALUE),
	m_decoderService(nullptr),
	m_videoDecoder(nullptr),
	m_swsCtx(nullptr)
{
	SetModule(module);
}
FFDecDXVA2::~FFDecDXVA2()
{
	if (codecIsOpen)
		avcodec_flush_buffers(codec_ctx);

	if (m_surfaces)
		for (IDirect3DSurface9 *surface : *m_surfaces)
			surface->Release();

	if (m_videoDecoder)
		m_videoDecoder->Release();
	if (m_decoderService)
		m_decoderService->Release();
	if (m_devMgr)
	{
		if (m_devHandle != INVALID_HANDLE_VALUE)
			m_devMgr->CloseDeviceHandle(m_devHandle);
		m_devMgr->Release();
	}
	if (m_d3d9Device)
		m_d3d9Device->Release();

	if (m_swsCtx)
		sws_freeContext(m_swsCtx);
}

bool FFDecDXVA2::set()
{
	const Qt::CheckState copyVideo = (Qt::CheckState)sets().getInt("CopyVideoDXVA2");
	if (copyVideo != m_copyVideo)
	{
		m_copyVideo = copyVideo;
		return false;
	}
	return sets().getBool("DecoderDXVA2Enabled");
}

QString FFDecDXVA2::name() const
{
	return "FFmpeg/DXVA2";
}

void FFDecDXVA2::downloadVideoFrame(VideoFrame &decoded)
{
	D3DSURFACE_DESC desc;
	D3DLOCKED_RECT lockedRect;
	IDirect3DSurface9 *surface = (IDirect3DSurface9 *)frame->data[3];
	if (SUCCEEDED(surface->GetDesc(&desc)) && SUCCEEDED(surface->LockRect(&lockedRect, nullptr, D3DLOCK_READONLY)))
	{
		AVBufferRef *dstBuffer[3] = {
			av_buffer_alloc(lockedRect.Pitch * frame->height),
			av_buffer_alloc((lockedRect.Pitch / 2) * ((frame->height + 1) / 2)),
			av_buffer_alloc((lockedRect.Pitch / 2) * ((frame->height + 1) / 2))
		};

		quint8 *srcData[2] = {
			(quint8 *)lockedRect.pBits,
			(quint8 *)lockedRect.pBits + (lockedRect.Pitch * desc.Height)
		};
		qint32 srcLinesize[2] = {
			lockedRect.Pitch,
			lockedRect.Pitch
		};

		uint8_t *dstData[3] = {
			dstBuffer[0]->data,
			dstBuffer[1]->data,
			dstBuffer[2]->data
		};
		qint32 dstLinesize[3] = {
			lockedRect.Pitch,
			lockedRect.Pitch / 2,
			lockedRect.Pitch / 2
		};

		m_swsCtx = sws_getCachedContext(m_swsCtx, frame->width, frame->height, AV_PIX_FMT_NV12, frame->width, frame->height, AV_PIX_FMT_YUV420P, SWS_POINT, nullptr, nullptr, nullptr);
		sws_scale(m_swsCtx, srcData, srcLinesize, 0, frame->height, dstData, dstLinesize);

		decoded = VideoFrame(VideoFrameSize(frame->width, frame->height), dstBuffer, dstLinesize, frame->interlaced_frame, frame->top_field_first);

		surface->UnlockRect();
	}
}

bool FFDecDXVA2::open(StreamInfo &streamInfo, VideoWriter *writer)
{
	if (streamInfo.type != QMPLAY2_TYPE_VIDEO)
		return false;

	AVCodec *codec = init(streamInfo);
	if (!codec || !hasHWAccel("dxva2"))
		return false;

	const QList<const GUID *> codecGUIDs = GUIDToAVCodec.values(codec->id);
	if (codecGUIDs.isEmpty())
		return false;

	DXVA2Hwaccel *dxva2Hwaccel = nullptr;
	if (writer)
	{
		dxva2Hwaccel = dynamic_cast<DXVA2Hwaccel *>(writer->getHWAccelInterface());
		if (dxva2Hwaccel)
		{
			m_d3d9Device = dxva2Hwaccel->getD3D9Device();
			m_d3d9Device->AddRef();
			m_hwAccelWriter = writer;
		}
	}

	QString deviceDescription;
	HRESULT hr;

	if (!m_d3d9Device)
	{
		IDirect3D9Ex *d3d9 = nullptr;

		hr = DX::Direct3DCreate9Ex(D3D_SDK_VERSION, &d3d9);
		if (FAILED(hr))
			return false;

		D3DDISPLAYMODE d3ddm;
		hr = d3d9->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);
		if (FAILED(hr))
		{
			d3d9->Release();
			return false;
		}

		D3DADAPTER_IDENTIFIER9 adapterIdentifier;
		memset(&adapterIdentifier, 0, sizeof adapterIdentifier);
		hr = d3d9->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &adapterIdentifier);
		if (FAILED(hr))
		{
			d3d9->Release();
			return false;
		}

		deviceDescription = adapterIdentifier.Description;

		D3DPRESENT_PARAMETERS d3dpp;
		memset(&d3dpp, 0, sizeof d3dpp);
		d3dpp.Windowed = true;
		d3dpp.BackBufferFormat = d3ddm.Format;
		d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		d3dpp.Flags = D3DPRESENTFLAG_VIDEO;

		hr = d3d9->CreateDevice
		(
			D3DADAPTER_DEFAULT,
			D3DDEVTYPE_HAL,
			GetDesktopWindow(),
			D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE,
			&d3dpp,
			&m_d3d9Device
		);

		d3d9->Release();

		if (FAILED(hr))
			return false;
	}

	quint32 resetToken = 0;
	hr = DX::DXVA2CreateDirect3DDeviceManager9(&resetToken, &m_devMgr);
	if (FAILED(hr))
		return false;

	hr = m_devMgr->ResetDevice(m_d3d9Device, resetToken);
	if (FAILED(hr))
		return false;

	hr = m_devMgr->OpenDeviceHandle(&m_devHandle);
	if (FAILED(hr))
		return false;

	hr = m_devMgr->GetVideoService(m_devHandle, IID_IDirectXVideoDecoderService, (void **)&m_decoderService);
	if (FAILED(hr))
		return false;

	m_devMgr->CloseDeviceHandle(m_devHandle);
	m_devHandle = INVALID_HANDLE_VALUE;

	m_devMgr->Release();
	m_devMgr = nullptr;

	const quint32 requiredFormat = (codec_ctx->pix_fmt == AV_PIX_FMT_YUV420P || codec_ctx->pix_fmt == AV_PIX_FMT_YUVJ420P) ? MAKEFOURCC('N', 'V', '1', '2') : MAKEFOURCC('P', '0', '1', '0');
	D3DFORMAT targetFmt = D3DFMT_UNKNOWN;
	const GUID *deviceGUID = nullptr;

	unsigned codecGUIDCount = 0;
	GUID *codecGUIDList = nullptr;
	hr = m_decoderService->GetDecoderDeviceGuids(&codecGUIDCount, &codecGUIDList);
	if (FAILED(hr))
		return false;

	for (int j = 0; j < codecGUIDs.size(); ++j)
	{
		const GUID *codecGUID = codecGUIDs[j];
		for (unsigned i = 0; i < codecGUIDCount; ++i)
		{
			if (IsEqualGUID(codecGUIDList[i], *codecGUID))
			{
				unsigned d3dFormatCount = 0;
				D3DFORMAT *d3dFmt = nullptr;
				hr = m_decoderService->GetDecoderRenderTargets(*codecGUID, &d3dFormatCount, &d3dFmt);
				if (SUCCEEDED(hr))
				{
					for (unsigned i = 0; i < d3dFormatCount; ++i)
					{
						if (d3dFmt[i] == requiredFormat)
						{
							targetFmt = d3dFmt[i];
							deviceGUID = codecGUID;
							break;
						}
					}
					CoTaskMemFree(d3dFmt);
				}
				break;
			}
		}
		if (deviceGUID)
			break;
	}

	CoTaskMemFree(codecGUIDList);

	if (!deviceGUID)
		return false;

	DXVA2_VideoDesc desc;
	memset(&desc, 0, sizeof desc);
	desc.SampleWidth = codec_ctx->coded_width;
	desc.SampleHeight = codec_ctx->coded_height;
	desc.SampleFormat.NominalRange = DXVA2_NominalRange_0_255; //Does it work?
	desc.Format = targetFmt;

	quint32 configCount = 0;
	DXVA2_ConfigPictureDecode *configList = nullptr;
	hr = m_decoderService->GetDecoderConfigurations(*deviceGUID, &desc, nullptr, &configCount, &configList);
	if (FAILED(hr))
		return false;

	//Detection from FFmpeg code
	quint32 bestScore = 0;
	for (unsigned i = 0; i < configCount; ++i)
	{
		quint32 score = 0;

		if (configList[i].ConfigBitstreamRaw == 1)
			score += 1;
		else if (configList[i].ConfigBitstreamRaw == 2 && codec->id == AV_CODEC_ID_H264)
			score += 2;
		else
			continue;

		if (IsEqualGUID(configList[i].guidConfigBitstreamEncryption, DXVA2_NoEncrypt))
			score += 16;

		if (score > bestScore)
		{
			bestScore = score;
			m_config = configList[i];
		}
	}
	CoTaskMemFree(configList);

	if (!bestScore)
		return false;

	//Values from FFmpeg code
	int surfaceAlignment;
	if (codec->id == AV_CODEC_ID_MPEG2VIDEO)
		surfaceAlignment = 32;
	else if  (codec->id == AV_CODEC_ID_HEVC)
		surfaceAlignment = 128;
	else
		surfaceAlignment = 16;

	//Values from FFmpeg code
	int numSurfaces = 4;
	if (codec->id == AV_CODEC_ID_H264 || codec->id == AV_CODEC_ID_HEVC)
		numSurfaces += 16;
	else if (codec->id == AV_CODEC_ID_VP9)
		numSurfaces += 9;
	else
		numSurfaces += 2;

	const quint32 surfaceW = FFALIGN(codec_ctx->coded_width, surfaceAlignment);
	const quint32 surfaceH = FFALIGN(codec_ctx->coded_height, surfaceAlignment);
	m_surfaces = Surfaces(new QVector<IDirect3DSurface9 *>(numSurfaces));

	hr = m_decoderService->CreateSurface(surfaceW, surfaceH, m_surfaces->count() - 1, targetFmt, D3DPOOL_DEFAULT, 0, DXVA2_VideoDecoderRenderTarget, m_surfaces->data(), nullptr);
	if (FAILED(hr))
	{
		m_surfaces.clear();
		return false;
	}

	hr = m_decoderService->CreateVideoDecoder(*deviceGUID, &desc, &m_config, m_surfaces->data(), m_surfaces->count(), &m_videoDecoder);
	if (FAILED(hr))
		return false;

	m_decoderService->Release();
	m_decoderService = nullptr;

	dxva_context *dxva2Ctx  = (dxva_context *)av_mallocz(sizeof(dxva_context));
	dxva2Ctx->decoder       = m_videoDecoder;
	dxva2Ctx->cfg           = &m_config;
	dxva2Ctx->surface_count = m_surfaces->count();
	dxva2Ctx->surface       = m_surfaces->data();
	if (IsEqualGUID(*deviceGUID, DXVADDI_Intel_ModeH264_E))
		dxva2Ctx->workaround = FF_DXVA2_WORKAROUND_INTEL_CLEARVIDEO;

	SurfacesQueue surfacesQueue;
	for (int i = 0; i < m_surfaces->count(); ++i)
		surfacesQueue.enqueue((QMPlay2SurfaceID)m_surfaces->at(i));
	new HWAccelHelper(codec_ctx, AV_PIX_FMT_DXVA2_VLD, dxva2Ctx, surfacesQueue);

	if (!openCodec(codec))
		return false;

	if (m_copyVideo != Qt::Checked)
	{
		if (!dxva2Hwaccel)
			dxva2Hwaccel = new DXVA2Hwaccel(m_d3d9Device, deviceDescription.contains("Intel", Qt::CaseInsensitive));
		dxva2Hwaccel->setNewData(Size(streamInfo.W, streamInfo.H), m_videoDecoder, m_surfaces);
		if (!m_hwAccelWriter && dxva2Hwaccel)
			m_hwAccelWriter = VideoWriter::createOpenGL2(dxva2Hwaccel);
		if (!m_hwAccelWriter && m_copyVideo == Qt::Unchecked)
			return false;
	}

	time_base = streamInfo.getTimeBase();
	return true;
}
