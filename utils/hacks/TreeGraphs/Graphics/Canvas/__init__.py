# ===-- __init__.py -------------------------------------------------------===##
# 
#                      The KLEE Symbolic Virtual Machine
# 
#  This file is distributed under the University of Illinois Open Source
#  License. See LICENSE.TXT for details.
# 
# ===----------------------------------------------------------------------===##


from __future__ import division

###

import math, os, random

from Graphics.Geometry import vec2

from reportlab.pdfgen import canvas
#from reportlab.graphics import shapes
from reportlab.pdfbase import pdfmetrics

"""
class Canvas:
	def startDrawing(self, (w,h)):
	def endDrawing(self):
		
	def getAspect(self):
		
	def setColor(self, r, g, b):
	def setLineWidth(self, width):
	def drawOutlineBox(self, x0, y0, x1, y1):
	def drawFilledBox(self, x0, y0, x1, y1):
	def drawFilledPolygon(self, pts):
	def drawOutlineCircle(self, x, y, r):
	def startDrawPoints(self):
	def endDrawPoints(self):
	def drawPoint(self, x, y):
	def setPointSize(self, size):

	def drawLine(self, a, b):
	def drawLines(self, ptPairs):
	def drawLineStrip(self, pts):

	def pushTransform(self):
	def popTransform(self):
	def translate(self, x, y):
	def scale(self, x, y):
	def setFontSize(self, size):
	def drawString(self, (x, y), text):
"""

class BaseCanvas:
	def drawPoints(self, pts):
		self.startDrawPoints()
		for pt in pts:
			self.drawPoint(pt)
		self.endDrawPoints()
		
	def drawStringCentered(self, boxLL, boxUR, text):
		ll,ur = self.getStringBBox(text)
		stringSize = vec2.sub(ur,ll)
		boxSize = vec2.sub(boxUR,boxLL)
		deltaSize = vec2.sub(boxSize, stringSize)
		halfDeltaSize = vec2.mulN(deltaSize, .5)
		
		self.drawString(vec2.add(boxLL,halfDeltaSize), text)

	def getStringSize(self, string):
		ll,ur = self.getStringBBox(string)
		return vec2.sub(ur,ll)
	
class PdfCanvas(BaseCanvas):
	def __init__(self, name, basePos=(300,400), baseScale=(250,250), pageSize=None):
		self._font = 'Times-Roman'
		self.c = canvas.Canvas(name, pagesize=pageSize)
		self.pointSize = 1
		self.scaleX = self.scaleY = 1
		self.state = []
		self.basePos = tuple(basePos)
		self.baseScale = tuple(baseScale)
		self.lastFontSizeSet = None
		
		self.kLineScaleFactor = 1.95
		
	def getAspect(self):
		return 1.0,1.0
		
	def startDrawing(self):
		self.pointSize = 1
		self.scaleX = self.scaleY = 1
		
		self.translate((self.basePos[0] + self.baseScale[0], self.basePos[1] + self.baseScale[1]))
		self.scale(self.baseScale)

		self.setColor(0,0,0)
		self.setLineWidth(1)
		self.setPointSize(1)

	def finishPage(self):
		self.c.showPage()

		self.pointSize = 1
		self.scaleX = self.scaleY = 1
		
		self.translate((self.basePos[0] + self.baseScale[0], self.basePos[1] + self.baseScale[1]))
		self.scale(self.baseScale)

		self.setColor(0,0,0)
		self.setLineWidth(1)
		self.setPointSize(1)
		
		if self.lastFontSizeSet is not None:
			self.setFontSize(self.lastFontSizeSet)
			
	def endDrawing(self):
		self.c.showPage()
		self.c.save()
		
	def setColor(self, r, g, b):
		self.c.setStrokeColorRGB(r,g,b)
		self.c.setFillColorRGB(r,g,b)
	def setLineWidth(self, width):
		avgScale = (self.scaleX+self.scaleY)/2
		self.c.setLineWidth(width/(self.kLineScaleFactor*avgScale))
	def setPointSize(self, size):
		avgScale = (self.scaleX+self.scaleY)/2
		self.pointSize = size/(4*avgScale)

	def drawOutlineBox(self, (x0, y0), (x1, y1)):
		self.c.rect(x0, y0, x1-x0, y1-y0, stroke=1, fill=0)
	def drawFilledBox(self, (x0, y0), (x1, y1)):
		self.c.rect(x0, y0, x1-x0, y1-y0, stroke=0, fill=1)
	def drawOutlineCircle(self, (x, y), r):
		self.c.circle(x, y, r, stroke=1, fill=0)
	def drawFilledCircle(self, (x, y), r):
		self.c.circle(x, y, r, stroke=0, fill=1)
	def drawFilledPolygon(self, pts):
		p = self.c.beginPath()
		p.moveTo(*pts[-1])
		for x,y in pts:
			p.lineTo(x,y)
		self.c.drawPath(p, fill=1, stroke=0)
	def drawOutlinePolygon(self, pts):
		p = self.c.beginPath()
		p.moveTo(* pts[0])
		for x,y in pts[1:]:
			p.lineTo(x,y)
		p.close()
		self.c.drawPath(p, fill=0, stroke=1)
	def startDrawPoints(self):
		pass
	def endDrawPoints(self):
		pass
	def drawPoint(self, (x, y)):
		self.c.circle(x, y, self.pointSize, stroke=0, fill=1)

	def drawLine(self, a, b):
		self.drawLines([(a,b)])
	def drawLines(self, ptPairs):
		p = self.c.beginPath()
		for a,b in ptPairs:
			p.moveTo(a[0],a[1])
			p.lineTo(b[0],b[1])
		self.c.drawPath(p)
	def drawLineStrip(self, pts):
		p = self.c.beginPath()
		p.moveTo(pts[0][0],pts[0][1])
		for pt in pts[1:]:
			p.lineTo(pt[0],pt[1])
		self.c.drawPath(p)
	def drawBezier(self, (p0,p1,p2,p3)):
		self.c.bezier(*(p0+p1+p2+p3))
		
	def pushTransform(self):
		self.state.append( (self.scaleX,self.scaleY) )
		self.c.saveState()
	def popTransform(self):
		self.c.restoreState()
		self.scaleX,self.scaleY = self.state.pop()
	def translate(self, (x, y)):
		self.c.translate(x, y)
	def rotate(self, angle):
		self.c.rotate(angle*180/math.pi)
	def scale(self, (x, y)):
		self.scaleX *= x
		self.scaleY *= y
		self.c.scale(x, y)

	def setFont(self, fontName):
		self._font = {"Symbol":"Symbol",
					 "Times":"Times-Roman"}.get(fontName,fontName)
	def setFontSize(self, size):
		self.lastFontSizeSet = size
		avgScale = (self.scaleX+self.scaleY)/2
		self.fontSize = size/(2*avgScale)
		self.c.setFont(self._font, self.fontSize)
#		self.c.setFont("Times-Roman", size/(2*avgScale))
	def drawString(self, (x, y), text):
		self.c.drawString(x, y, text)
		
	def drawOutlineString(self, (x,y), text):
		t = self.c.beginText(x, y)
		t.setTextRenderMode(1)
		t.textLine(text)
		t.setTextRenderMode(0)
		self.c.drawText(t)

	def getStringBBox(self, text):
		font = pdfmetrics.getFont(self._font)
		width = pdfmetrics.stringWidth(text, self._font, self.fontSize)
		ll = (0,0)
		ur = (width, (1.0 - font.face.ascent/2048.)*self.fontSize)
		ur = (width, (1.0 - font.face.ascent/2048.)*self.fontSize)
		return ll,ur
