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

#ifndef MATERIALS_H
#define MATERIALS_H

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct _MaterialProp {
	GLfloat ambient[4];
	GLfloat diffuse[4];
	GLfloat specular[4];
	GLfloat shininess;
    } MaterialProp;

    static MaterialProp mat_emerald = {
	{0.0215, 0.1745, 0.0215, 1.0},
	{0.07568, 0.61424, 0.07568, 1.0},
	{0.633, 0.727811, 0.633, 1.0},
	0.6
    };

    static MaterialProp mat_jade = {
	{0.135, 0.2225, 0.1575, 1.0},
	{0.54, 0.89, 0.63, 1.0},
	{0.316228, 0.316228, 0.316228, 1.0},
	0.1
    };

    static MaterialProp mat_obsidian = {
	{0.05375, 0.05, 0.06625, 1.0},
	{0.18275, 0.17, 0.22525, 1.0},
	{0.332741, 0.328634, 0.346435, 1.0},
	0.3
    };

    static MaterialProp mat_pearl = {
	{0.25, 0.20725, 0.20725, 1.0},
	{1.0, 0.829, 0.829, 1.0},
	{0.296648, 0.296648, 0.296648, 1.0},
	0.088
    };

    static MaterialProp mat_ruby = {
	{0.1745, 0.01175, 0.01175, 1.0},
	{0.61424, 0.04136, 0.04136, 1.0},
	{0.727811, 0.626959, 0.626959, 1.0},
	0.6
    };

    static MaterialProp mat_turquoise = {
	{0.1, 0.18725, 0.1745, 1.0},
	{0.396, 0.74151, 0.69102, 1.0},
	{0.297254, 0.30829, 0.306678, 1.0},
	0.1
    };

    static MaterialProp mat_brass = {
	{0.329412, 0.223529, 0.027451, 1.0},
	{0.780392, 0.568627, 0.113725, 1.0},
	{0.992157, 0.941176, 0.807843, 1.0},
	0.21794872
    };

    static MaterialProp mat_bronze = {
	{0.2125, 0.1275, 0.054, 1.0},
	{0.714, 0.4284, 0.18144, 1.0},
	{0.393548, 0.271906, 0.166721, 1.0},
	0.2
    };

    static MaterialProp mat_chrome = {
	{0.25, 0.25, 0.25, 1.0},
	{0.4, 0.4, 0.4, 1.0},
	{0.774597, 0.774597, 0.774597, 1.0},
	0.6
    };

    static MaterialProp mat_copper = {
	{0.19125, 0.0735, 0.0225, 1.0},
	{0.7038, 0.27048, 0.0828, 1.0},
	{0.256777, 0.137622, 0.086014, 1.0},
	0.1
    };

    static MaterialProp mat_gold = {
	{0.24725, 0.1995, 0.0745, 1.0},
	{0.75164, 0.60648, 0.22648, 1.0},
	{0.628281, 0.555802, 0.366065, 1.0},
	0.4
    };

    static MaterialProp mat_silver = {
	{0.19225, 0.19225, 0.19225, 1.0},
	{0.50754, 0.50754, 0.50754, 1.0},
	{0.508273, 0.508273, 0.508273, 1.0},
	0.4
    };

#ifdef __cplusplus
}				/* end extern "C" */
#endif

#endif
