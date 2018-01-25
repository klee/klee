/* $Id$ $Revision$ */
/* vim:set shiftwidth=4 ts=8: */

/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: See CVS logs. Details at http://www.graphviz.org/
 *************************************************************************/

#include "gvplugin.h"

#include "gvplugin_gdiplus.h"

extern gvplugin_installed_t gvrender_gdiplus_types[];
extern gvplugin_installed_t gvtextlayout_gdiplus_types[];
extern gvplugin_installed_t gvloadimage_gdiplus_types[];
extern gvplugin_installed_t gvdevice_gdiplus_types[];
extern gvplugin_installed_t gvdevice_gdiplus_types_for_cairo[];

using namespace std;
using namespace Gdiplus;

/* class id corresponding to each format_type */
static GUID format_id [] = {
	GUID_NULL,
	GUID_NULL,
	ImageFormatBMP,
	ImageFormatEMF,
	ImageFormatEMF,
	ImageFormatGIF,
	ImageFormatJPEG,
	ImageFormatPNG,
	ImageFormatTIFF
};

static ULONG_PTR _gdiplusToken = NULL;

static void UnuseGdiplus()
{
	GdiplusShutdown(_gdiplusToken);
}

void UseGdiplus()
{
	/* only startup once, and ensure we get shutdown */
	if (!_gdiplusToken)
	{
		GdiplusStartupInput input;
		GdiplusStartup(&_gdiplusToken, &input, NULL);
		atexit(&UnuseGdiplus);
	}
}

const Gdiplus::StringFormat* GetGenericTypographic()
{
	const Gdiplus::StringFormat* format = StringFormat::GenericTypographic();
	return format;
}

void SaveBitmapToStream(Bitmap &bitmap, IStream *stream, int format)
{
	/* search the encoders for one that matches our device id, then save the bitmap there */
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	UINT encoderNum;
	UINT encoderSize;
	GetImageEncodersSize(&encoderNum, &encoderSize);
	vector<char> codec_buffer(encoderSize);
	ImageCodecInfo *codecs = (ImageCodecInfo *)&codec_buffer.front();
	GetImageEncoders(encoderNum, encoderSize, codecs);
	for (UINT i = 0; i < encoderNum; ++i)
		if (memcmp(&(format_id[format]), &codecs[i].FormatID, sizeof(GUID)) == 0) {
			bitmap.Save(stream, &codecs[i].Clsid, NULL);
			break;
		}
}

static gvplugin_api_t apis[] = {
    {API_render, gvrender_gdiplus_types},
	{API_textlayout, gvtextlayout_gdiplus_types},
	{API_loadimage, gvloadimage_gdiplus_types},
    {API_device, gvdevice_gdiplus_types},
	{API_device, gvdevice_gdiplus_types_for_cairo},
    {(api_t)0, 0},
};

#ifdef WIN32_DLL /*visual studio*/
#ifndef GVPLUGIN_GDIPLUS_EXPORTS
__declspec(dllimport) gvplugin_library_t gvplugin_gdiplus_LTX_library = { "gdiplus", apis };
#else
__declspec(dllexport) gvplugin_library_t gvplugin_gdiplus_LTX_library = { "gdiplus", apis };
#endif
#else /*end visual studio*/
#ifdef GVDLL
__declspec(dllexport) gvplugin_library_t gvplugin_gdiplus_LTX_library = { "gdiplus", apis };
#else
extern "C" gvplugin_library_t gvplugin_gdiplus_LTX_library = { "gdiplus", apis };
#endif
#endif

