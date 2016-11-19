/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2016  Błażej Szczygieł

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

#include <VAAPI.hpp>
#include <Functions.hpp>
#include <ImgScaler.hpp>
#include <VideoFrame.hpp>
#include <QMPlay2Core.hpp>

#include <va/va_x11.h>
#include <va/va_glx.h>

VAAPI::VAAPI() :
	ok(false),
	VADisp(NULL),
	outW(0), outH(0),
#ifdef HAVE_VPP
	vpp_deint_type(VAProcDeinterlacingNone),
	use_vpp(false),
#endif
	version(0),
	display(NULL)
{
	memset(&nv12ImageFmt, 0, sizeof nv12ImageFmt);
}
VAAPI::~VAAPI()
{
	clr();
	if (VADisp)
		vaTerminate(VADisp);
	if (display)
		XCloseDisplay(display);
}

bool VAAPI::open(bool allowVDPAU, bool &openGL)
{
	clr();

	display = XOpenDisplay(NULL);
	if (!display)
		return false;

	VADisp = openGL ? vaGetDisplayGLX(display) : vaGetDisplay(display);
	if (!VADisp)
	{
		if (openGL)
		{
			VADisp = vaGetDisplay(display);
			openGL = false;
		}
		if (!VADisp)
			return false;
	}

	int major = 0, minor = 0;
	if (vaInitialize(VADisp, &major, &minor) == VA_STATUS_SUCCESS)
	{
		const QString vendor = vaQueryVendorString(VADisp);
		isVDPAU = vendor.contains("VDPAU");
		if (isVDPAU && !allowVDPAU)
			return false;
		isXvBA = vendor.contains("XvBA");

		int numProfiles = vaMaxNumProfiles(VADisp);
		VAProfile profiles[numProfiles];
		if (vaQueryConfigProfiles(VADisp, profiles, &numProfiles) == VA_STATUS_SUCCESS)
		{
			for (int i = 0; i < numProfiles; ++i)
				profileList.push_back(profiles[i]);

			version = (major << 8) | minor;

			int fmtCount = vaMaxNumImageFormats(VADisp);
			VAImageFormat imgFmt[fmtCount];
			if (vaQueryImageFormats(VADisp, imgFmt, &fmtCount) == VA_STATUS_SUCCESS)
				for (int i = 0; i < fmtCount; ++i)
					if (imgFmt[i].fourcc == VA_FOURCC_NV12)
					{
						nv12ImageFmt = imgFmt[i];
						break;
					}

			return true;
		}
	}
	return false;
}

bool VAAPI::init(int width, int height, const char *codecName, bool initFilters)
{
	VAProfile p = (VAProfile)-1; //VAProfileNone
	if (!qstrcmp(codecName, "h264"))
	{
		if (profileList.contains(VAProfileH264High))
			p = VAProfileH264High;
		else if (profileList.contains(VAProfileH264Main))
			p = VAProfileH264Main;
		else if (profileList.contains(VAProfileH264Baseline))
			p = VAProfileH264Baseline;
	}
#if VA_VERSION_HEX >= 0x230000 // 1.3.0
	else if (!qstrcmp(codecName, "vp8")) //Not supported in FFmpeg (06.12.2015)
	{
		if (profileList.contains(VAProfileVP8Version0_3))
			p = VAProfileVP8Version0_3;
	}
#endif
#if VA_VERSION_HEX >= 0x250000 // 1.5.0
	else if (!qstrcmp(codecName, "hevc"))
	{
		if (profileList.contains(VAProfileHEVCMain))
			p = VAProfileHEVCMain;
	}
#endif
#if VA_VERSION_HEX >= 0x260000 // 1.6.0
	else if (!qstrcmp(codecName, "vp9")) //Supported in FFmpeg >= 3.0, not tested
	{
		if (profileList.contains(VAProfileVP9Profile0))
			p = VAProfileVP9Profile0;
	}
#endif
	else if (!qstrcmp(codecName, "mpeg2video"))
	{
		if (profileList.contains(VAProfileMPEG2Main))
			p = VAProfileMPEG2Main;
		else if (profileList.contains(VAProfileMPEG2Simple))
			p = VAProfileMPEG2Simple;
	}
	else if (!qstrcmp(codecName, "mpeg4"))
	{
		if (profileList.contains(VAProfileMPEG4Main))
			p = VAProfileMPEG4Main;
		else if (profileList.contains(VAProfileMPEG4Simple))
			p = VAProfileMPEG4Simple;
	}
	else if (!qstrcmp(codecName, "vc1"))
	{
		if (profileList.contains(VAProfileVC1Advanced))
			p = VAProfileVC1Advanced;
		else if (profileList.contains(VAProfileVC1Main))
			p = VAProfileVC1Main;
		else if (profileList.contains(VAProfileVC1Simple))
			p = VAProfileVC1Simple;
	}
	else if (!qstrcmp(codecName, "h263"))
	{
		if (profileList.contains(VAProfileH263Baseline))
			p = VAProfileH263Baseline;
	}

	if (!ok || profile != p || outW != width || outH != height)
	{
		clr();

		profile = p;
		outW = width;
		outH = height;

		if (!vaapiCreateSurfaces(surfaces, surfacesCount, false))
			return false;
		surfacesCreated = true;

		if (!vaCreateConfigAndContext())
			return false;

		if (initFilters)
			init_vpp();

		ok = true;
	}
	else
	{
#ifdef HAVE_VPP
		forward_reference = VA_INVALID_SURFACE;
		vpp_second = false;
#endif
		if (isVDPAU)
		{
			if (context)
			{
				vaDestroyContext(VADisp, context);
				context = 0;
			}
			if (config)
			{
				vaDestroyConfig(VADisp, config);
				config = 0;
			}
			if (!vaCreateConfigAndContext())
				return false;
		}
	}

	return ok;
}

SurfacesQueue VAAPI::getSurfacesQueue() const
{
	SurfacesQueue surfacesQueue;
	for (int i = 0; i < surfacesCount; ++i)
		surfacesQueue.enqueue((QMPlay2SurfaceID)surfaces[i]);
	return surfacesQueue;
}

inline bool VAAPI::vaCreateConfigAndContext()
{
	return vaCreateConfig(VADisp, profile, VAEntrypointVLD, NULL, 0, &config) == VA_STATUS_SUCCESS && vaCreateContext(VADisp, config, outW, outH, VA_PROGRESSIVE, surfaces, surfacesCount, &context) == VA_STATUS_SUCCESS;
}
bool VAAPI::vaapiCreateSurfaces(VASurfaceID *surfaces, int surfacesCount, bool useAttr)
{
#ifdef NEW_CREATESURFACES
	VASurfaceAttrib attrib, *attribs = NULL;
	if (useAttr)
	{
		attrib.type = VASurfaceAttribPixelFormat;
		attrib.flags = VA_SURFACE_ATTRIB_SETTABLE;
		attrib.value.type = VAGenericValueTypeInteger;
		attrib.value.value.i = VA_FOURCC_NV12;
		attribs = &attrib;
	}
	return vaCreateSurfaces(VADisp, VA_RT_FORMAT_YUV420, outW, outH, surfaces, surfacesCount, attribs, attribs ? 1 : 0) == VA_STATUS_SUCCESS;
#else
	Q_UNUSED(useAttr)
	return vaCreateSurfaces(VADisp, outW, outH, VA_RT_FORMAT_YUV420, surfacesCount, surfaces) == VA_STATUS_SUCCESS;
#endif
}

void VAAPI::init_vpp()
{
#ifdef HAVE_VPP
	use_vpp = true;
	if
	(
		vaCreateConfig(VADisp, (VAProfile)-1, VAEntrypointVideoProc, NULL, 0, &config_vpp) == VA_STATUS_SUCCESS &&
		vaCreateContext(VADisp, config_vpp, 0, 0, 0, NULL, 0, &context_vpp) == VA_STATUS_SUCCESS &&
		vaapiCreateSurfaces(&id_vpp, 1, true)
	)
	{
		unsigned num_filters = VAProcFilterCount;
		VAProcFilterType filters[VAProcFilterCount];
		if (vaQueryVideoProcFilters(VADisp, context_vpp, filters, &num_filters) != VA_STATUS_SUCCESS)
			num_filters = 0;
		if (num_filters)
		{
			/* Creating dummy filter (some drivers/api versions (1.6.x and Ivy Bridge) crashes without any filter) - this is unreachable now */
			VAProcFilterParameterBufferBase none_params = {VAProcFilterNone};
			if (vaCreateBuffer(VADisp, context_vpp, VAProcFilterParameterBufferType, sizeof none_params, 1, &none_params, &vpp_buffers[VAProcFilterNone]) != VA_STATUS_SUCCESS)
				vpp_buffers[VAProcFilterNone] = VA_INVALID_ID;
			/* Searching deinterlacing filter */
			if (vpp_deint_type != VAProcDeinterlacingNone)
				for (unsigned i = 0; i < num_filters; ++i)
					if (filters[i] == VAProcFilterDeinterlacing)
					{
						VAProcFilterCapDeinterlacing deinterlacing_caps[VAProcDeinterlacingCount];
						unsigned num_deinterlacing_caps = VAProcDeinterlacingCount;
						if (vaQueryVideoProcFilterCaps(VADisp, context_vpp, VAProcFilterDeinterlacing, &deinterlacing_caps, &num_deinterlacing_caps) != VA_STATUS_SUCCESS)
							num_deinterlacing_caps = 0;
						bool vpp_deint_types[2] = {false};
						for (unsigned j = 0; j < num_deinterlacing_caps; ++j)
						{
							switch (deinterlacing_caps[j].type)
							{
								case VAProcDeinterlacingMotionAdaptive:
									vpp_deint_types[0] = true;
									break;
								case VAProcDeinterlacingMotionCompensated:
									vpp_deint_types[1] = true;
									break;
								default:
									break;
							}
						}
						if (vpp_deint_type == VAProcDeinterlacingMotionCompensated && !vpp_deint_types[1])
						{
							QMPlay2Core.log(tr("Not supported deinterlacing algorithm") + " - Motion compensated", ErrorLog | LogOnce);
							vpp_deint_type = VAProcDeinterlacingMotionAdaptive;
						}
						if (vpp_deint_type == VAProcDeinterlacingMotionAdaptive && !vpp_deint_types[0])
						{
							QMPlay2Core.log(tr("Not supported deinterlacing algorithm") + " - Motion adaptive", ErrorLog | LogOnce);
							vpp_deint_type = VAProcDeinterlacingNone;
						}
						if (vpp_deint_type != VAProcDeinterlacingNone)
						{
							VAProcFilterParameterBufferDeinterlacing deint_params = {VAProcFilterDeinterlacing, vpp_deint_type, 0};
							if (vaCreateBuffer(VADisp, context_vpp, VAProcFilterParameterBufferType, sizeof deint_params, 1, &deint_params, &vpp_buffers[VAProcFilterDeinterlacing]) != VA_STATUS_SUCCESS)
								vpp_buffers[VAProcFilterDeinterlacing] = VA_INVALID_ID;
						}
						break;
					}
			return;
		}
	}
	if (vpp_deint_type != VAProcDeinterlacingNone) //Show error only when filter is required
		QMPlay2Core.log("VA-API :: " + tr("Cannot open video filters"), ErrorLog | LogOnce);
	clr_vpp();
#endif
}

void VAAPI::clr_vpp()
{
#ifdef HAVE_VPP
	if (use_vpp)
	{
		for (int i = 0; i < VAProcFilterCount; ++i)
			if (vpp_buffers[i] != VA_INVALID_ID)
				vaDestroyBuffer(VADisp, vpp_buffers[i]);
		if (id_vpp != VA_INVALID_SURFACE)
			vaDestroySurfaces(VADisp, &id_vpp, 1);
		if (context_vpp)
			vaDestroyContext(VADisp, context_vpp);
		if (config_vpp)
			vaDestroyConfig(VADisp, config_vpp);
		use_vpp = false;
	}
	id_vpp = forward_reference = VA_INVALID_SURFACE;
	for (int i = 0; i < VAProcFilterCount; ++i)
		vpp_buffers[i] = VA_INVALID_ID;
	vpp_second = false;
	context_vpp = 0;
	config_vpp = 0;
#endif
}
void VAAPI::clr()
{
	clr_vpp();
	if (VADisp)
	{
		if (surfacesCreated)
			vaDestroySurfaces(VADisp, surfaces, surfacesCount);
		if (context)
			vaDestroyContext(VADisp, context);
		if (config)
			vaDestroyConfig(VADisp, config);
	}
	surfacesCreated = ok = false;
	profile = (VAProfile)-1; //VAProfileNone
	context = 0;
	config = 0;
}

void VAAPI::applyVideoAdjustment(int brightness, int contrast, int saturation, int hue)
{
	int num_attribs = vaMaxNumDisplayAttributes(VADisp);
	VADisplayAttribute attribs[num_attribs];
	if (!vaQueryDisplayAttributes(VADisp, attribs, &num_attribs))
	{
		for (int i = 0; i < num_attribs; ++i)
		{
			switch (attribs[i].type)
			{
				case VADisplayAttribHue:
					attribs[i].value = Functions::scaleEQValue(hue, attribs[i].min_value, attribs[i].max_value);
					break;
				case VADisplayAttribSaturation:
					attribs[i].value = Functions::scaleEQValue(saturation, attribs[i].min_value, attribs[i].max_value);
					break;
				case VADisplayAttribBrightness:
					attribs[i].value = Functions::scaleEQValue(brightness, attribs[i].min_value, attribs[i].max_value);
					break;
				case VADisplayAttribContrast:
					attribs[i].value = Functions::scaleEQValue(contrast, attribs[i].min_value, attribs[i].max_value);
					break;
				default:
					break;
			}
		}
		vaSetDisplayAttributes(VADisp, attribs, num_attribs);
	}
}

bool VAAPI::filterVideo(const VideoFrame &videoFrame, VASurfaceID &id, int &field)
{
	const VASurfaceID curr_id = videoFrame.surfaceId;
#ifdef HAVE_VPP
	const bool do_vpp_deint = (field != 0) && vpp_buffers[VAProcFilterDeinterlacing] != VA_INVALID_ID;
	if (use_vpp && !do_vpp_deint)
	{
		forward_reference = VA_INVALID_SURFACE;
		vpp_second = false;
	}
	if (use_vpp && (do_vpp_deint || version <= 0x0023))
	{
		bool vpp_ok = false;

		if (do_vpp_deint && forward_reference == VA_INVALID_SURFACE)
			forward_reference = curr_id;
		if (!vpp_second && forward_reference == curr_id)
			return false;

		if (do_vpp_deint)
		{
			VAProcFilterParameterBufferDeinterlacing *deint_params = NULL;
			if (vaMapBuffer(VADisp, vpp_buffers[VAProcFilterDeinterlacing], (void **)&deint_params) == VA_STATUS_SUCCESS)
			{
				if (version > 0x0025 || !vpp_second)
					deint_params->flags = (field == VA_TOP_FIELD) ? 0 : VA_DEINTERLACING_BOTTOM_FIELD;
				vaUnmapBuffer(VADisp, vpp_buffers[VAProcFilterDeinterlacing]);
			}
		}

		VABufferID pipeline_buf;
		if (vaCreateBuffer(VADisp, context_vpp, VAProcPipelineParameterBufferType, sizeof(VAProcPipelineParameterBuffer), 1, NULL, &pipeline_buf) == VA_STATUS_SUCCESS)
		{
			VAProcPipelineParameterBuffer *pipeline_param = NULL;
			if (vaMapBuffer(VADisp, pipeline_buf, (void **)&pipeline_param) == VA_STATUS_SUCCESS)
			{
				memset(pipeline_param, 0, sizeof *pipeline_param);
				pipeline_param->surface = curr_id;
				pipeline_param->output_background_color = 0xFF000000;

				pipeline_param->num_filters = 1;
				if (!do_vpp_deint)
					pipeline_param->filters = &vpp_buffers[VAProcFilterNone]; //Sometimes it can prevent crash, but sometimes it can produce green image, so it is disabled for newer VA-API versions which don't crash without VPP
				else
				{
					pipeline_param->filters = &vpp_buffers[VAProcFilterDeinterlacing];
					pipeline_param->num_forward_references = 1;
					pipeline_param->forward_references = &forward_reference;
				}

				vaUnmapBuffer(VADisp, pipeline_buf);

				if (vaBeginPicture(VADisp, context_vpp, id_vpp) == VA_STATUS_SUCCESS)
				{
					vpp_ok = vaRenderPicture(VADisp, context_vpp, &pipeline_buf, 1) == VA_STATUS_SUCCESS;
					vaEndPicture(VADisp, context_vpp);
				}
			}
			if (!vpp_ok)
				vaDestroyBuffer(VADisp, pipeline_buf);
		}

		if (vpp_second)
			forward_reference = curr_id;
		if (do_vpp_deint)
			vpp_second = !vpp_second;

		if ((ok = vpp_ok))
		{
			id = id_vpp;
			if (do_vpp_deint)
				field = 0;
			return true;
		}

		return false;
	}
	else
#endif
		id = curr_id;
	return true;
}

quint8 *VAAPI::getNV12Image(VAImage &image, VASurfaceID surfaceID) const
{
	if (nv12ImageFmt.fourcc == VA_FOURCC_NV12)
	{
		VAImageFormat imgFmt = nv12ImageFmt;
		if (vaCreateImage(VADisp, &imgFmt, outW, outH, &image) == VA_STATUS_SUCCESS)
		{
			quint8 *data;
			if
			(
				vaSyncSurface(VADisp, surfaceID) == VA_STATUS_SUCCESS &&
				vaGetImage(VADisp, surfaceID, 0, 0, outW, outH, image.image_id) == VA_STATUS_SUCCESS &&
				vaMapBuffer(VADisp, image.buf, (void **)&data) == VA_STATUS_SUCCESS
			) return data;
			vaDestroyImage(VADisp, image.image_id);
		}
	}
	return NULL;
}
bool VAAPI::getImage(const VideoFrame &videoFrame, void *dest, ImgScaler *nv12ToRGB32) const
{
	VAImage image;
	quint8 *vaData = getNV12Image(image, videoFrame.surfaceId);
	if (vaData)
	{
		const void *data[2] = {
			vaData + image.offsets[0],
			vaData + image.offsets[1]
		};
		nv12ToRGB32->scale(data, (const int *)image.pitches, dest);
		vaUnmapBuffer(VADisp, image.buf);
		vaDestroyImage(VADisp, image.image_id);
		return true;
	}
	return false;
}
