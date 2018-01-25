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

using System;
using System.Drawing;
using System.Drawing.Printing;
using System.Text;

namespace Graphviz
{
	public class TileableImagePrinter
	{
		public TileableImagePrinter(PrintDocument document, Image image)
		{
			_image = image;
			
			/* calculate imageable area = page area - margins */
			Rectangle bounds = document.DefaultPageSettings.Bounds;
			Margins margins = document.DefaultPageSettings.Margins;
			Rectangle pageBounds = new Rectangle(
				bounds.Location + new Size(margins.Left, margins.Right),
				bounds.Size - new Size(margins.Left + margins.Right, margins.Top + margins.Bottom));
			_pageSize = pageBounds.Size;
			_pageRegion = new Region(pageBounds);
			
			/* calculate image size in 1/100 of an inch */
			_imageSize = new SizeF(image.Size.Width * HUNDREDTH_INCH_PER_PIXEL,
				image.Size.Height * HUNDREDTH_INCH_PER_PIXEL);

			/* calculate how many page columns + rows required to tile out the image */
			_pageColumnCount = ((int)(_imageSize.Width / _pageSize.Width)) + 1;
			_pageRowCount = ((int)(_imageSize.Height / _pageSize.Height)) + 1;
			
			/* calculate the offset of the image on the first page, from page margins + 1/2 of difference between image size and total page size */
			_imageOffset = new PointF(
				pageBounds.Left + (_pageColumnCount * _pageSize.Width - _imageSize.Width) / 2.0f,
				pageBounds.Top + (_pageRowCount * _pageSize.Height - _imageSize.Height) / 2.0f);
	
			/* start at page 0 */
			_page = 0;
			
			_printPage = delegate(object sender, PrintPageEventArgs eventArgs)
			{
				/* which page column + row we are printing */
				int pageColumn = _page % _pageColumnCount;
				int pageRow = _page / _pageColumnCount;

				/* clip to the imageable area and draw the offset image into it */
				eventArgs.Graphics.Clip = _pageRegion;
				eventArgs.Graphics.DrawImage(_image, new RectangleF(
					_imageOffset - new SizeF(pageColumn * _pageSize.Width, pageRow * _pageSize.Height),
					_imageSize));
					
				/* only print column * row pages */
				eventArgs.HasMorePages = ++_page < _pageColumnCount * _pageRowCount;
			};
			
			_endPrint = delegate(object sender, PrintEventArgs eventArgs)
			{
				/* detach handlers from the print document */
				PrintDocument printDocument = (PrintDocument)sender;
				printDocument.PrintPage -= _printPage;
				printDocument.EndPrint -= _endPrint;
			};
			
			/* attach handlers to the print document */
			document.PrintPage += _printPage;
			document.EndPrint += _endPrint;
	
		}
		
		private const float HUNDREDTH_INCH_PER_PIXEL = 100.0f / 96.0f;
		
		private readonly Image _image;
		
		private readonly Size _pageSize;
		private readonly Region _pageRegion;
		private readonly SizeF _imageSize;
		private readonly int _pageColumnCount;
		private readonly int _pageRowCount;
		private readonly PointF _imageOffset;
		
		private int _page;
		
		private readonly PrintPageEventHandler _printPage;
		private readonly PrintEventHandler _endPrint;	}
}
