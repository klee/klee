/* $Id: VisioText.h,v 1.4 2011/01/25 16:30:51 ellson Exp $ $Revision: 1.4 $ */
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

#ifndef VISIOTEXT_H
#define VISIOTEXT_H

#include "types.h"

namespace Visio
{
	/* Para VDX element */
	
	class Para
	{
	public:
		enum HorzAlign
		{
			horzLeft = 0,
			horzCenter = 1,
			horzRight = 2,
			horzJustify = 3,
			horzForce = 4
		};
		
		Para(HorzAlign horzAlign);
		
		/* output the para details */
		void Print(GVJ_t* job) const;
		
	private:
		HorzAlign _horzAlign;
	};
	
	/* Char VDX element */
	
	class Char
	{
	public:
		Char(double size, unsigned char red, unsigned char green, unsigned char blue);
		
		/* output the char details */
		void Print(GVJ_t* job) const;
		
	private:
		double _size;
		unsigned char _red;
		unsigned char _green;
		unsigned char _blue;
	};
	
	/* para marker + char marker + text */
	
	class Run
	{
	public:
		Run(boxf bounds, char* text);
		~Run();
		
		boxf GetBounds() const;		/* bounding box -- used by text logic */
		
		/* given the text index, output the run */
		void Print(GVJ_t* job, unsigned int index) const;
		
	private:
		boxf _bounds;
		char* _text;
	};
	
	/* Para, Char and Run details for each Graphviz text */
	
	class Text
	{
	public:
		static Text* CreateText(GVJ_t* job, pointf p, textpara_t* para);
		
		~Text();
		
		boxf GetBounds() const;
		
		void Print(GVJ_t* job) const;
		void PrintRun(GVJ_t* job, unsigned int index) const;
		
	private:
		Text(Para* para, Char* chars, Run* run);
		
		Para* _para;
		Char* _chars;
		Run* _run;
	};
	
	/* Hyperlink VDX element */
	
	class Hyperlink
	{
	public:
		static Hyperlink* CreateHyperlink(GVJ_t* job, char* url, char* tooltip, char* target, char* id);
		
		Hyperlink(char* description, char* address, char* frame);
		~Hyperlink();
		
		/* given the id, whether it is default, output the hyperlink */
		void Print(GVJ_t* job, unsigned int id, bool isDefault) const;
		
	private:
		char* _description;
		char* _address;
		char* _frame;
	};
}

#endif