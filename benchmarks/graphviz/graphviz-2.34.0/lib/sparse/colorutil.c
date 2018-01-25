/* $Id$Revision: */
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

#include "general.h"
#include "colorutil.h"

static void r2hex(float r, char *h){
  /* convert a number in [0,1] to 0 to 255 then to a hex */
  static char hex[] = "0123456789abcdef";
  int i = (int)(255*r+0.5);
  int j = i%16;
  int k = i/16;
  h[0] = hex[k];
  h[1] = hex[j];
}

void rgb2hex(float r, float g, float b, char *cstring, char *opacity){
  cstring[0] = '#';
  r2hex(r, &(cstring[1]));
  r2hex(g, &(cstring[3]));
  r2hex(b, &(cstring[5]));
  //set to semitransparent for multiple sets vis
  if (opacity && strlen(opacity) >= 2){
    cstring[7] = opacity[0];
    cstring[8] = opacity[1];
    cstring[9]='\0';
  } else {
    cstring[7] = '\0';
  }
}

real Hue2RGB(real v1, real v2, real H) {
  if(H < 0.0) H += 1.0;
  if(H > 1.0) H -= 1.0;
  if((6.0*H) < 1.0) return (v1 + (v2 - v1) * 6.0 * H);
  if((2.0*H) < 1.0) return v2;
  if((3.0*H) < 2.0) return (v1 + (v2 - v1) * ((2.0/3.0) - H) * 6.0);
  return v1;
}

char *hex[16]={"0","1","2","3","4","5","6","7","8","9","a","b","c","d","e","f"};

char * hue2rgb(real hue, char *color){
  real v1, v2, lightness = .5, saturation = 1;
  int red, blue, green;

  if(lightness < 0.5) 
    v2 = lightness * (1.0 + saturation);
  else
    v2 = (lightness + saturation) - (saturation * lightness);

  v1 = 2.0 * lightness - v2;

  red =   (int)(255.0 * Hue2RGB(v1, v2, hue + (1.0/3.0)) + 0.5);
  green = (int)(255.0 * Hue2RGB(v1, v2, hue) + 0.5);
  blue =  (int)(255.0 * Hue2RGB(v1, v2, hue - (1.0/3.0)) + 0.5);
  color[0] = '#';
  sprintf(color+1,"%s",hex[red/16]);
  sprintf(color+2,"%s",hex[red%16]);
  sprintf(color+3,"%s",hex[green/16]);
  sprintf(color+4,"%s",hex[green%16]);
  sprintf(color+5,"%s",hex[blue/16]);
  sprintf(color+6,"%s",hex[blue%16]);
  color[7] = '\0';
  return color;
}


void hue2rgb_real(real hue, real *color){
  real v1, v2, lightness = .5, saturation = 1;
  int red, blue, green;

  if(lightness < 0.5) 
    v2 = lightness * (1.0 + saturation);
  else
    v2 = (lightness + saturation) - (saturation * lightness);

  v1 = 2.0 * lightness - v2;

  red =   (int)(255.0 * Hue2RGB(v1, v2, hue + (1.0/3.0)) + 0.5);
  green = (int)(255.0 * Hue2RGB(v1, v2, hue) + 0.5);
  blue =  (int)(255.0 * Hue2RGB(v1, v2, hue - (1.0/3.0)) + 0.5);


  color[0] = red/255.;
  color[1] = green/255.;
  color[2] = blue/255.;
		  
}
