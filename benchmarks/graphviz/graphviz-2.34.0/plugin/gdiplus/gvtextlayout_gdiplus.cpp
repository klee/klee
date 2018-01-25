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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "gvplugin_textlayout.h"
#include "gvplugin_gdiplus.h"

static int count = 0;

using namespace Gdiplus;

static int CALLBACK fetch_first_font(
	const LOGFONTA *logFont,
	const TEXTMETRICA *textMetrics,
	DWORD fontType,
	LPARAM lParam)
{
	/* save the first font we see in the font enumeration */
	*((LOGFONTA *)lParam) = *logFont;
	return 0;
}

Layout::Layout(char *fontname, double fontsize, char* string)
{
	/* convert incoming UTF8 string to wide chars */
	/* NOTE: conversion is 1 or more UTF8 chars to 1 wide char */
	int len = strlen(string);
	text.resize(len);
	text.resize(MultiByteToWideChar(CP_UTF8, 0, string, len, &text[0], len));

	/* search for a font with this name. if we can't find it, use the generic serif instead */
	/* NOTE: GDI font search is more comprehensive than GDI+ and will look for variants e.g. Arial Bold */
	DeviceContext reference;
	LOGFONTA font_to_find;
	font_to_find.lfCharSet = ANSI_CHARSET;
	strncpy(font_to_find.lfFaceName, fontname, sizeof(font_to_find.lfFaceName) - 1);
	font_to_find.lfFaceName[sizeof(font_to_find.lfFaceName) - 1] = '\0';
	font_to_find.lfPitchAndFamily = 0;
	LOGFONTA found_font;
	if (EnumFontFamiliesExA(reference.hdc,
		&font_to_find,
		fetch_first_font,
		(LPARAM)&found_font,
		0) == 0) {
		found_font.lfHeight = (LONG)-fontsize;
		found_font.lfWidth = 0;
		font = new Font(reference.hdc, &found_font);
	}
	else
		font = new Font(FontFamily::GenericSerif(), fontsize);
}

Layout::~Layout()
{
	delete font;
}

void gdiplus_free_layout(void *layout)
{
	if (layout)
		delete (Layout*)layout;
};

boolean gdiplus_textlayout(textpara_t *para, char **fontpath)
{
	/* ensure GDI+ is started up: since we get called outside of a job, we can't rely on GDI+ startup then */
	UseGdiplus();
	
	Layout* layout = new Layout(para->fontname, para->fontsize, para->str);
	
	/* measure the text */
	/* NOTE: use TextRenderingHintAntiAlias + GetGenericTypographic to get a layout without extra space at beginning and end */
	RectF boundingBox;
	DeviceContext deviceContext;
	Graphics measureGraphics(deviceContext.hdc);
	measureGraphics.SetTextRenderingHint(TextRenderingHintAntiAlias);
	measureGraphics.MeasureString(
		&layout->text[0],
		layout->text.size(),
		layout->font,
		PointF(0.0f, 0.0f),
		GetGenericTypographic(),
		&boundingBox);
		
	FontFamily fontFamily;
	layout->font->GetFamily(&fontFamily);
	int style = layout->font->GetStyle();
		
	para->layout = (void*)layout;
	para->free_layout = &gdiplus_free_layout;
	para->width = boundingBox.Width;
	para->height = layout->font->GetHeight(&measureGraphics);
	para->yoffset_layout = fontFamily.GetCellAscent(style) * para->fontsize / fontFamily.GetEmHeight(style); /* convert design units to pixels */
	para->yoffset_centerline = 0;
	return TRUE;
};

static gvtextlayout_engine_t gdiplus_textlayout_engine = {
    gdiplus_textlayout
};

gvplugin_installed_t gvtextlayout_gdiplus_types[] = {
    {0, "textlayout", 8, &gdiplus_textlayout_engine, NULL},
    {0, NULL, 0, NULL, NULL}
};
