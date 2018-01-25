/* $Id: VisioGraphic.h,v 1.8 2011/01/25 16:30:51 ellson Exp $ $Revision: 1.8 $ */
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

#ifndef VISIOGRAPHIC_H
#define VISIOGRAPHIC_H

#include "types.h"

namespace Visio
{
	/* Fill VDX element */
	
	class Fill
	{
	public:
		Fill(unsigned char red, unsigned char green, unsigned char blue, double transparency);
		
		/* output the fill */
		void Print(GVJ_t* job) const;
		
	private:
		unsigned char _red;
		unsigned char _green;
		unsigned char _blue;
		double _transparency;	/* 0.0 == opaque, 1.0 == transparent */
	};

	/* Line VDX element */
	
	class Line
	{
	public:
		Line(double weight, unsigned char red, unsigned char green, unsigned char blue, unsigned int pattern, unsigned int beginArrow = 0, unsigned int endArrow = 0);
		
		/* output the line */
		void Print(GVJ_t* job) const;
		
	private:
		double _weight;
		unsigned char _red;
		unsigned char _green;
		unsigned char _blue;
		unsigned int _pattern;		/* solid == 1, dashed == 2, dotted == 3 etc. */
		unsigned int _beginArrow;	/* arrow type e.g. 2 is filled arrow head */
		unsigned int _endArrow;		/* arrow type e.g. 2 is filled arrow head */
	};
	
	/* Geom VDX element */
	
	class Connection
	{
	public:
		virtual pointf GetFirst() const = 0;
		virtual pointf GetLast() const = 0;
		virtual pointf GetCenter() const = 0;
	};
	
	class Geom
	{
	public:
		virtual ~Geom();
		
		virtual boxf GetBounds() const = 0;				/* bounding box -- used by node logic */
		virtual const Connection* GetConnection() const = 0;	/* first, last and center points -- used by edge logic */
		
		/* given first (lower left) and last points (upper right), output the geometry */ 
		virtual void Print(GVJ_t* job, pointf first, pointf last, bool allowCurves) const = 0;
	};
	
	class Ellipse: public Geom
	{
	public:
		Ellipse(pointf* points, bool filled);
		
		virtual boxf GetBounds() const;
		virtual const Connection* GetConnection() const;

		void Print(GVJ_t* job, pointf first, pointf last, bool allowCurves) const;

	private:
		bool _filled;
		pointf _points[2];
	};
	
	class Path: public Geom
	{
	public:
		Path(pointf* points, int pointCount);
		~Path();
		
		virtual boxf GetBounds() const;
		
	protected:
		pointf* _points;
		int _pointCount;
	};
		
	class Bezier: public Path, public Connection
	{
	public:
		Bezier(pointf* points, int pointCount, bool filled);
		
		virtual const Connection* GetConnection() const;
		
		virtual pointf GetFirst() const;
		virtual pointf GetLast() const;
		virtual pointf GetCenter() const;		

		virtual void Print(GVJ_t* job, pointf first, pointf last, bool allowCurves) const;
		

	private:
		bool _filled;
	};
	
	class Polygon: public Path
	{
	public:
		Polygon(pointf* points, int pointCount, bool filled);
		
		virtual const Connection* GetConnection() const;
		
		virtual void Print(GVJ_t* job, pointf first, pointf last, bool allowCurves) const;

	private:
		bool _filled;
	};

	class Polyline: public Path
	{
	public:
		Polyline(pointf* points, int pointCount);
		
		virtual const Connection* GetConnection() const;

		void Print(GVJ_t* job, pointf first, pointf last, bool allowCurves) const;

	};
	
	/* Line, Fill and Geom details for each Graphviz graphic */
	
	class Graphic
	{
	public:
		static Graphic* CreateEllipse(GVJ_t* job, pointf* A, bool filled);
		static Graphic* CreateBezier(GVJ_t* job, pointf* A, int n, bool arrow_at_start, bool arrow_at_end, bool filled);
		static Graphic* CreatePolygon(GVJ_t* job, pointf* A, int n, bool filled);
		static Graphic* CreatePolyline(GVJ_t* job, pointf* A, int n);
		
		~Graphic();

		boxf GetBounds() const;
		const Connection* GetConnection() const;

		void Print(GVJ_t* job, pointf first, pointf last, bool allowCurves) const;
		
	private:
		Graphic(Line* line, Fill* fill, Geom* geom);
		
		Line* _line;
		Fill* _fill;
		Geom* _geom;
	};
}

#endif