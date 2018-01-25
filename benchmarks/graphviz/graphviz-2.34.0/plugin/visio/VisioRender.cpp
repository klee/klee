/* $Id: VisioRender.cpp,v 1.9 2011/01/25 16:30:51 ellson Exp $ $Revision: 1.9 $ */
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

#ifdef _MSC_VER
#include <cfloat>
#define isfinite _finite
#endif

#ifdef __GNUC__
#include <cmath>
#endif

#ifdef __SUNPRO_CC
#include <ieeefp.h>
#define isfinite(x) finite(x)
#endif

#include "VisioRender.h"

#include "const.h"
#include "macros.h"
#include "gvcjob.h"
#include "gvio.h"

namespace Visio
{
	using namespace std;
	
	static const float INCHES_PER_POINT = 1.0 / 72.0;
	static const float ZERO_ADJUST = 0.125;
	
	enum ConLineRouteExt
	{
		LORouteExtDefault = 0,
		LORouteExtStraight = 1,
		LORouteExtNURBS	= 2
	};
	
	enum ShapeRouteStyle
	{
		LORouteDefault = 0,
		LORouteRightAngle = 1,
		LORouteStraight = 2,
		LORouteCenterToCenter = 16
	};
	
	Render::Render():
		_pageId(0),
		_shapeId(0),
		_hyperlinkId(0),
		_inComponent(false),
		_graphics(),
		_texts(),
		_hyperlinks(),
		_nodeIds()
	{
	}

	Render::~Render()
	{
	}

	void Render::BeginGraph(GVJ_t* job)
	{
		gvputs(job, "<VisioDocument xmlns='http://schemas.microsoft.com/visio/2003/core'>\n");
		gvputs(job, "<Pages>\n");
	}

	void Render::EndGraph(GVJ_t* job)
	{
		gvputs(job, "</Pages>\n");
		gvputs(job, "</VisioDocument>\n");
	}

	void Render::BeginPage(GVJ_t* job)
	{
		gvprintf(job, "<Page ID='%d'>\n", ++_pageId);
		
		gvputs(job, "<PageSheet>\n");
		gvputs(job, "<PageProps>\n");
		gvprintf(job, "<PageWidth>%f</PageWidth>\n", job->width * INCHES_PER_POINT);
		gvprintf(job, "<PageHeight>%f</PageHeight>\n", job->height * INCHES_PER_POINT);
		gvputs(job, "</PageProps>\n");
		gvputs(job, "</PageSheet>\n");
		
		gvputs(job, "<Shapes>");
	}

	void Render::EndPage(GVJ_t* job)
	{
		gvputs(job, "</Shapes>\n");
		gvputs(job, "</Page>\n");
	}

	void Render::BeginNode(GVJ_t* job)
	{
		_inComponent = true;
		ClearGraphicsAndTexts();
	}

	void Render::EndNode(GVJ_t* job)
	{
		_inComponent = false;
		
		unsigned int outerShapeId = 0;
		switch (_graphics.size())
		{
		case 0:
			/* no graphics to render */
			break;
		case 1:
			/* single graphic to render, output as top level shape */
			PrintOuterShape(job, _graphics[0]);
			outerShapeId = _shapeId;
			break;
		default:
			/* multiple graphics to render, output each as a subshape of a group */
				
			/* calculate group bounds */
			boxf outerBounds = _graphics[0]->GetBounds();
			for (Graphics::const_iterator nextGraphic = _graphics.begin() + 1, lastGraphic = _graphics.end();
				 nextGraphic != lastGraphic;
				 ++nextGraphic)
			{
				boxf innerBounds = (*nextGraphic)->GetBounds();
				if (outerBounds.LL.x > innerBounds.LL.x)
					outerBounds.LL.x = innerBounds.LL.x;
				if (outerBounds.LL.y > innerBounds.LL.y)
					outerBounds.LL.y = innerBounds.LL.y;
				if (outerBounds.UR.x < innerBounds.UR.x)
					outerBounds.UR.x = innerBounds.UR.x;
				if (outerBounds.UR.y < innerBounds.UR.y)
					outerBounds.UR.y = innerBounds.UR.y;
			}
			
			gvprintf(job, "<Shape ID='%d' Type='Group'>\n", ++_shapeId);
			outerShapeId = _shapeId;			
			
			gvputs(job, "<XForm>\n");
			gvprintf(job, "<PinX>%f</PinX>\n", (outerBounds.LL.x + outerBounds.UR.x) * 0.5 * INCHES_PER_POINT);
			gvprintf(job, "<PinY>%f</PinY>\n", (outerBounds.LL.y + outerBounds.UR.y) * 0.5 * INCHES_PER_POINT);
			gvprintf(job, "<Width>%f</Width>\n", (outerBounds.UR.x - outerBounds.LL.x) * INCHES_PER_POINT);
			gvprintf(job, "<Height>%f</Height>\n", (outerBounds.UR.y - outerBounds.LL.y) * INCHES_PER_POINT);
			gvputs(job, "</XForm>\n");

			/* output Hyperlink */
			PrintHyperlinks(job);
				
			/* output Para, Char */
			PrintTexts(job);
		
			/* output subshapes */
			gvputs(job, "<Shapes>\n");
			for (Graphics::const_iterator nextGraphic = _graphics.begin(), lastGraphic = _graphics.end(); nextGraphic != lastGraphic; ++nextGraphic)
				PrintInnerShape(job, *nextGraphic, outerShapeId, outerBounds);
			gvputs(job, "</Shapes>\n");
				
			gvputs(job, "</Shape>\n");
			break;
		}
		
		/* save node id for edge logic */
		if (outerShapeId)
			_nodeIds[job->obj->u.n] = outerShapeId;
		ClearGraphicsAndTexts();
	}

	void Render::BeginEdge(GVJ_t* job)
	{
		_inComponent = true;
		ClearGraphicsAndTexts();
	}

	void Render::EndEdge(GVJ_t* job)
	{
		_inComponent = false;
		
		if (_graphics.size() > 0)
		{
			Agedge_t* edge = job->obj->u.e;
			
			/* get previously saved ids for tail and head node; edge type for graph */
			NodeIds::const_iterator beginId = _nodeIds.find(agtail(edge));
			NodeIds::const_iterator endId = _nodeIds.find(aghead(edge));
			
			/* output first connectable shape as an edge shape, all else as regular outer shapes */
			bool firstConnector = true;
			for (Graphics::const_iterator nextGraphic = _graphics.begin(), lastGraphic = _graphics.end(); nextGraphic != lastGraphic; ++nextGraphic)
				if (firstConnector && PrintEdgeShape(job,
					_graphics[0],
					beginId == _nodeIds.end() ? 0 : beginId->second,
					endId == _nodeIds.end() ? 0 : endId->second,
#ifdef WITH_CGRAPH
					EDGE_TYPE(agroot(edge))))
#else
					EDGE_TYPE(edge->head->graph->root)))
#endif
					firstConnector = false;
				else
					PrintOuterShape(job, *nextGraphic);

		}
		ClearGraphicsAndTexts();
	}

	void Render::AddEllipse(GVJ_t* job, pointf* A, bool filled)
	{
		AddGraphic(job, Graphic::CreateEllipse(job, A, filled));
	}

	void Render::AddBezier(GVJ_t* job, pointf* A, int n, bool arrow_at_start, bool arrow_at_end, bool filled)
	{
		AddGraphic(job, Graphic::CreateBezier(job, A, n, arrow_at_start, arrow_at_end, filled));
	}

	void Render::AddPolygon(GVJ_t* job, pointf* A, int n, bool filled)
	{
		AddGraphic(job, Graphic::CreatePolygon(job, A, n, filled));
	}

	void Render::AddPolyline(GVJ_t* job, pointf* A, int n)
	{
		AddGraphic(job, Graphic::CreatePolyline(job, A, n));
	}
	
	void Render::AddText(GVJ_t* job, pointf p, textpara_t *para)
	{
		AddText(job, Text::CreateText(job, p, para));
	}

	void Render::AddAnchor(GVJ_t *job, char *url, char *tooltip, char *target, char *id)
	{
		AddHyperlink(job, Hyperlink::CreateHyperlink(job, url, tooltip, target, id));
	}
	
	void Render::ClearGraphicsAndTexts()
	{
		/* clear graphics */
		for (Graphics::iterator nextGraphic = _graphics.begin(), lastGraphic = _graphics.end(); nextGraphic != lastGraphic; ++nextGraphic)
			delete *nextGraphic;
		_graphics.clear();
		
		/* clear texts */
		for (Texts::iterator nextText = _texts.begin(), lastText = _texts.end(); nextText != lastText; ++nextText)
			delete *nextText;
		_texts.clear();

		/* clear hyperlinks */
		for (Hyperlinks::iterator nextHyperlink = _hyperlinks.begin(), lastHyperlink = _hyperlinks.end(); nextHyperlink != lastHyperlink; ++nextHyperlink)
			delete *nextHyperlink;
		_hyperlinks.clear();
	}
	
	void Render::AddGraphic(GVJ_t* job, const Graphic* graphic)
	{
		if (_inComponent)
			/* if in component, accumulate for end node/edge */
			_graphics.push_back(graphic);
		else
			/* if outside, output immediately */
			PrintOuterShape(job, graphic);		
	}

	void Render::AddText(GVJ_t* job, const Text* text)
	{
		/* if in component, accumulate for end node/edge */
		if (_inComponent)
			_texts.push_back(text);
	}

	void Render::AddHyperlink(GVJ_t* job, const Hyperlink* hyperlink)
	{
		/* if in component, accumulate for end node/edge */
		if (_inComponent)
			_hyperlinks.push_back(hyperlink);
	}
	
	void Render::PrintOuterShape(GVJ_t* job, const Graphic* graphic)
	{
		boxf bounds = graphic->GetBounds();
		
		gvprintf(job, "<Shape ID='%d' Type='Shape'>\n", ++_shapeId);
		
		gvputs(job, "<XForm>\n");
		gvprintf(job, "<PinX>%f</PinX>\n", (bounds.LL.x + bounds.UR.x) * 0.5 * INCHES_PER_POINT);
		gvprintf(job, "<PinY>%f</PinY>\n", (bounds.LL.y + bounds.UR.y) * 0.5 * INCHES_PER_POINT);
		gvprintf(job, "<Width>%f</Width>\n", (bounds.UR.x - bounds.LL.x) * INCHES_PER_POINT);
		gvprintf(job, "<Height>%f</Height>\n", (bounds.UR.y - bounds.LL.y) * INCHES_PER_POINT);
		gvputs(job, "</XForm>\n");
		
		gvputs(job, "<Misc>\n");
		gvputs(job, "<ObjType>1</ObjType>\n");
		gvputs(job, "</Misc>\n");
		
		/* output Hyperlink */
		PrintHyperlinks(job);

		/* output Para, Char, Text */
		PrintTexts(job);
		
		/* output Line, Fill, Geom */
		graphic->Print(job, bounds.LL, bounds.UR, true);
		
		gvputs(job, "</Shape>\n");
	}
	
	void Render::PrintInnerShape(GVJ_t* job, const Graphic* graphic, unsigned int outerId, boxf outerBounds)
	{
		boxf innerBounds = graphic->GetBounds();
		
		/* compute scale. if infinite, scale by 0 instead */
		double xscale = 1.0 / (outerBounds.UR.x - outerBounds.LL.x);
		double yscale = 1.0 / (outerBounds.UR.y - outerBounds.LL.y);
		if (isfinite(xscale) == 0)
			xscale = 0.0;
		if (isfinite(yscale) == 0)
			yscale = 0.0;
		
		gvprintf(job, "<Shape ID='%d' Type='Shape'>\n", ++_shapeId);
		
		/* inner XForm is based on width or height of outer Shape */
		gvputs(job, "<XForm>\n");
		gvprintf(job, "<PinX F='Sheet.%d!Width*%f' />\n", outerId, (((innerBounds.LL.x + innerBounds.UR.x) * 0.5) - outerBounds.LL.x) * xscale);
		gvprintf(job, "<PinY F='Sheet.%d!Height*%f' />\n", outerId, (((innerBounds.LL.y + innerBounds.UR.y) * 0.5) - outerBounds.LL.y) * yscale);
		gvprintf(job, "<Width F='Sheet.%d!Width*%f' />\n", outerId, (innerBounds.UR.x - innerBounds.LL.x) * xscale);
		gvprintf(job, "<Height F='Sheet.%d!Height*%f' />\n", outerId, (innerBounds.UR.y - innerBounds.LL.y) * yscale);
		gvputs(job, "</XForm>\n");
		
		gvputs(job, "<Misc>\n");
		gvputs(job, "<ObjType>1</ObjType>\n");
		gvputs(job, "</Misc>\n");
		
		/* output Line, Fill, Geom */
		graphic->Print(job, innerBounds.LL, innerBounds.UR, true);

		gvputs(job, "</Shape>\n");
	}
	
	bool Render::PrintEdgeShape(GVJ_t* job, const Graphic* graphic, unsigned int beginId, unsigned int endId, int edgeType)
	{
		if (const Connection* connection = graphic->GetConnection())
		{
			pointf first = connection->GetFirst();
			pointf last = connection->GetLast();
			
			bool zeroWidth = first.x == last.x;
			bool zeroHeight = first.y == last.y;

			gvprintf(job, "<Shape ID='%d' Type='Shape'>\n", ++_shapeId);
			
			/* XForm depends on XForm1D */
			gvputs(job, "<XForm>\n");
			gvputs(job, "<PinX F='GUARD((BeginX+EndX)/2)'/>\n");
			gvputs(job, "<PinY F='GUARD((BeginY+EndY)/2)'/>\n");
			if (zeroWidth)
				gvprintf(job, "<Width F='GUARD(%f)'/>\n", 2 * ZERO_ADJUST);	/* if vertical line, expand width to 0.25 inches */
			else
				gvputs(job, "<Width F='GUARD(EndX-BeginX)'/>\n");
			if (zeroHeight)
				gvprintf(job, "<Height F='GUARD(%f)'/>\n", 2 * ZERO_ADJUST);	/* if horizontal line, expand height to 0.25 inches */
			else
				gvputs(job, "<Height F='GUARD(EndY-BeginY)'/>\n");
			gvputs(job, "<Angle F='GUARD(0DA)'/>\n");
			gvputs(job, "</XForm>\n");
				
			/* XForm1D walking glue makes connector attach to its nodes */
			gvputs(job, "<XForm1D>\n");
			gvprintf(job, "<BeginX F='_WALKGLUE(BegTrigger,EndTrigger,WalkPreference)'>%f</BeginX>\n", first.x * INCHES_PER_POINT);
			gvprintf(job, "<BeginY F='_WALKGLUE(BegTrigger,EndTrigger,WalkPreference)'>%f</BeginY>\n", first.y * INCHES_PER_POINT);
			gvprintf(job, "<EndX F='_WALKGLUE(EndTrigger,BegTrigger,WalkPreference)'>%f</EndX>\n", last.x * INCHES_PER_POINT);
			gvprintf(job, "<EndY F='_WALKGLUE(EndTrigger,BegTrigger,WalkPreference)'>%f</EndY>\n", last.y * INCHES_PER_POINT);
			gvputs(job, "</XForm1D>\n");
			
			gvputs(job, "<Protection>\n");
			gvputs(job, "<LockHeight>1</LockHeight>\n");
			gvputs(job, "<LockCalcWH>1</LockCalcWH>\n");
			gvputs(job, "</Protection>\n");
			
			gvputs(job, "<Misc>\n");
			gvputs(job, "<NoAlignBox>1</NoAlignBox>\n");
			gvputs(job, "<DynFeedback>2</DynFeedback>\n");
			gvputs(job, "<GlueType>2</GlueType>\n");
			if (beginId && endId)
			{
				gvprintf(job, "<BegTrigger F='_XFTRIGGER(Sheet.%d!EventXFMod)'/>\n", beginId);
				gvprintf(job, "<EndTrigger F='_XFTRIGGER(Sheet.%d!EventXFMod)'/>\n", endId);
			}
			gvputs(job, "<ObjType>2</ObjType>\n");
			gvputs(job, "</Misc>\n");
			
			gvputs(job, "<Layout>\n");
			gvprintf(job, "<ShapeRouteStyle>%d</ShapeRouteStyle>\n", edgeType == ET_LINE ? LORouteCenterToCenter : LORouteRightAngle);
			gvputs(job, "<ConFixedCode>6</ConFixedCode>\n");
			gvprintf(job, "<ConLineRouteExt>%d</ConLineRouteExt>\n", edgeType == ET_LINE || edgeType == ET_PLINE ? LORouteExtStraight : LORouteExtNURBS);
			gvputs(job, "<ShapeSplittable>1</ShapeSplittable>\n");
			gvputs(job, "</Layout>\n");
			
			/* output Hyperlink */
			PrintHyperlinks(job);

			/* TextXForm depends on custom control */
			gvputs(job, "<TextXForm>\n");
			gvputs(job, "<TxtPinX F='SETATREF(Controls.TextPosition)'/>\n");
			gvputs(job, "<TxtPinY F='SETATREF(Controls.TextPosition.Y)'/>\n");
			gvputs(job, "<TxtWidth F='MAX(TEXTWIDTH(TheText),5*Char.Size)'/>\n");
			gvputs(job, "<TxtHeight F='TEXTHEIGHT(TheText,TxtWidth)'/>\n");
			gvputs(job, "</TextXForm>\n");

			if (zeroWidth)
			{
				first.x -= ZERO_ADJUST;
				last.x += ZERO_ADJUST;
			}
			if (zeroHeight)
			{
				first.y -= ZERO_ADJUST;
				last.y += ZERO_ADJUST;
			}
			
			/* compute center to attach text to. if text has been rendered, use overall bounding box center; if not, use the path center */
			pointf textCenter;
			if (_texts.size() > 0)
			{
				boxf outerTextBounds = _texts[0]->GetBounds();
				
				for (Texts::const_iterator nextText = _texts.begin() + 1, lastText = _texts.end();
					 nextText != lastText;
					 ++nextText)
				{
					boxf innerTextBounds = (*nextText)->GetBounds();
					if (outerTextBounds.LL.x > innerTextBounds.LL.x)
						outerTextBounds.LL.x = innerTextBounds.LL.x;
					if (outerTextBounds.LL.y > innerTextBounds.LL.y)
						outerTextBounds.LL.y = innerTextBounds.LL.y;
					if (outerTextBounds.UR.x < innerTextBounds.UR.x)
						outerTextBounds.UR.x = innerTextBounds.UR.x;
					if (outerTextBounds.UR.y < innerTextBounds.UR.y)
						outerTextBounds.UR.y = innerTextBounds.UR.y;
				}
				textCenter.x = (outerTextBounds.LL.x + outerTextBounds.UR.x) / 2.0;
				textCenter.y = (outerTextBounds.LL.y + outerTextBounds.UR.y) / 2.0;
			}
			else
				textCenter = connection->GetCenter();
			
			/* Control for positioning text */
			gvputs(job, "<Control NameU='TextPosition'>\n");
			gvprintf(job, "<X>%f</X>\n", (textCenter.x - first.x) * INCHES_PER_POINT);
			gvprintf(job, "<Y>%f</Y>\n", (textCenter.y - first.y) * INCHES_PER_POINT);
			gvputs(job, "<XDyn F='Controls.TextPosition'/>\n");
			gvputs(job, "<YDyn F='Controls.TextPosition.Y'/>\n");
			gvputs(job, "<XCon F='IF(OR(STRSAME(SHAPETEXT(TheText),&quot;&quot;),HideText),5,0)'>5</XCon>\n");
			gvputs(job, "<YCon>0</YCon>\n");
			gvputs(job, "</Control>\n");
				
			/* output Para, Char, Text */
			PrintTexts(job);
			
			/* output Line, Fill, Geom */
			graphic->Print(job, first, last, edgeType != ET_LINE && edgeType != ET_PLINE);
			
			gvputs(job, "</Shape>\n");
			return true;
		}
		else
			return false;
	}
	
	void Render::PrintTexts(GVJ_t* job)
	{
		if (_texts.size() > 0)
		{
			/* output Para, Char */
			for (Texts::iterator nextText = _texts.begin(), lastText = _texts.end(); nextText != lastText; ++nextText)
				(*nextText)->Print(job);
			
			/* output Text. each run references above Para + Char */
			gvputs(job, "<Text>");
			for (unsigned int index = 0, count = _texts.size(); index < count; ++index)
				(_texts[index])->PrintRun(job, index);
			gvputs(job, "</Text>");
		}
	}
	
	void Render::PrintHyperlinks(GVJ_t* job)
	{
		if (_hyperlinks.size() > 0)
		{
			_hyperlinks[0]->Print(job, ++_hyperlinkId, true);
			for (Hyperlinks::iterator nextHyperlink = _hyperlinks.begin() + 1, lastHyperlink = _hyperlinks.end(); nextHyperlink != lastHyperlink; ++nextHyperlink)
				(*nextHyperlink)->Print(job, ++_hyperlinkId, false);
		}
	}

}
