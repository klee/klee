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
using System.Drawing.Drawing2D;
using System.Windows.Forms;

namespace Graphviz
{
	public partial class ScrollableImageControl : ScrollableControl
	{
		public Image Image
		{
			get
			{
				return _image;
			}
			
			set
			{
				_image = value;
				UpdateAutoScroll();
			}
		}
		
		public float Zoom
		{
			get
			{
				return _zoom;
			}
			
			set
			{
				_zoom = value;
				UpdateAutoScroll();
			}
		}
		
		public ScrollableImageControl()
		{
			InitializeComponent();

			SetStyle(ControlStyles.AllPaintingInWmPaint |
				ControlStyles.DoubleBuffer |
				ControlStyles.ResizeRedraw |
				ControlStyles.UserMouse |
				ControlStyles.UserPaint,
				true);
			_image = null;
			_zoom = 1.0f;
			_lastScroll = new Point(0, 0);
		}
		
		public void ActualSize()
		{
			Zoom = 1.0f;
		}
		
		public void ZoomToFit()
		{
			Size imageSize = Image.Size;
			Size controlSize = Size;
			Zoom = Math.Min((float)controlSize.Width / (float)imageSize.Width, (float)controlSize.Height / (float)imageSize.Height);
		}
		
		public void ZoomIn()
		{
			Zoom *= _squareRootOfTwo;
		}
		
		public void ZoomOut()
		{
			Zoom /= _squareRootOfTwo;
		}

		protected override void OnMouseDown(MouseEventArgs e)
		{
			/* if potentially panning with the mouse, record last scroll position */
			/* NOTE: Autoscroll should be set to +ve values but always returns -ve values */
			if (e.Button == MouseButtons.Left)
				_lastScroll = new Point(
					e.X - AutoScrollPosition.X,
					e.Y - AutoScrollPosition.Y);
			base.OnMouseDown(e);
		}

		protected override void OnMouseMove(MouseEventArgs e)
		{
			/* if panning with the mouse, scroll to position */
			/* NOTE: Autoscroll should be set to +ve values but always returns -ve values */
			if (e.Button == MouseButtons.Left)
				AutoScrollPosition = new Point(
					_lastScroll.X - e.X,
					_lastScroll.Y - e.Y);
			base.OnMouseMove(e);
		}

		protected override void OnPaintBackground(PaintEventArgs e)
		{
			/* fill clip rectangle with background color */
			e.Graphics.FillRectangle(new SolidBrush(BackColor), e.ClipRectangle);		
		}
		
		protected override void OnPaint(PaintEventArgs pe)
		{
			if (_image != null) {
				Matrix oldMatrix = pe.Graphics.Transform;
				
				/* if the zoomed image size is smaller than the control size, center the display */
				/* adjust by the scroll position, then scale by the zoom factor */
				pe.Graphics.TranslateTransform(
					Math.Max((Width - _image.Width * _zoom) / 2.0f, 0.0f) + AutoScrollPosition.X,
					Math.Max((Height - _image.Height * _zoom) / 2.0f, 0.0f)	+ AutoScrollPosition.Y);
				pe.Graphics.ScaleTransform(_zoom, _zoom);
			
				/* blit the image onto the graphics */
				pe.Graphics.DrawImage(_image, 0.0f, 0.0f);
				
				pe.Graphics.Transform = oldMatrix;
			}
			base.OnPaint(pe);
			
		}
		
		private void UpdateAutoScroll()
		{
			if (_image == null)
				AutoScrollMinSize = Size;
			else
				AutoScrollMinSize = new Size(
					(int)(_image.Width * _zoom + 0.5f),
					(int)(_image.Height * _zoom + 0.5f));
			Invalidate();
		}
		
		private readonly float _squareRootOfTwo = (float)Math.Sqrt(2.0);
		private Image _image;
		private float _zoom;
		private Point _lastScroll;
	}
}
