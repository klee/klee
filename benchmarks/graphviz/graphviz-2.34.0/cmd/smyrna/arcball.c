/* $Id$Revision: */
/* vim:set shiftwidth=4 ts=8: */

/*************************************************************************************/
/**                                                                                 **/
/** Copyright (c) 1999-2009 Tatewake.com                                            **/
/**                                                                                 **/
/** Permission is hereby granted, free of charge, to any person obtaining a copy    **/
/** of this software and associated documentation files (the "Software"), to deal   **/
/** in the Software without restriction, including without limitation the rights    **/
/** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell       **/
/** copies of the Software, and to permit persons to whom the Software is           **/
/** furnished to do so, subject to the following conditions:                        **/
/**                                                                                 **/
/** The above copyright notice and this permission notice shall be included in      **/
/** all copies or substantial portions of the Software.                             **/
/**                                                                                 **/
/** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR      **/
/** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,        **/
/** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE     **/
/** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER          **/
/** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   **/
/** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN       **/
/** THE SOFTWARE.                                                                   **/
/**                                                                                 **/
/*************************************************************************************/

#include "glcompdefs.h"
#define ARCBALL_C
#include "smyrnadefs.h"
#include "arcball.h"

static void setBounds(ArcBall_t * a, GLfloat NewWidth, GLfloat NewHeight)
{
    assert((NewWidth > 1.0f) && (NewHeight > 1.0f));
    //Set adjustment factor for width/height
    a->AdjustWidth = 1.0f / ((NewWidth - 1.0f) * 0.5f);
    a->AdjustHeight = 1.0f / ((NewHeight - 1.0f) * 0.5f);
}

static void mapToSphere(ArcBall_t * a, const Point2fT * NewPt,
			Vector3fT * NewVec)
{
    Point2fT TempPt;
    GLfloat length;

    //Copy paramter into temp point
    TempPt = *NewPt;

    //Adjust point coords and scale down to range of [-1 ... 1]
    TempPt.s.X = (TempPt.s.X * a->AdjustWidth) - 1.0f;
    TempPt.s.Y = 1.0f - (TempPt.s.Y * a->AdjustHeight);

    //Compute the square of the length of the vector to the point from the center
    length = (TempPt.s.X * TempPt.s.X) + (TempPt.s.Y * TempPt.s.Y);

    //If the point is mapped outside of the sphere... (length > radius squared)
    if (length > 1.0f) {
	GLfloat norm;

	//Compute a normalizing factor (radius / sqrt(length))
	norm = 1.0f / FuncSqrt(length);

	//Return the "normalized" vector, a point on the sphere
	NewVec->s.X = TempPt.s.X * norm;
	NewVec->s.Y = TempPt.s.Y * norm;
	NewVec->s.Z = 0.0f;
    } else			//Else it's on the inside
    {
	//Return a vector to a point mapped inside the sphere sqrt(radius squared - length)
	NewVec->s.X = TempPt.s.X;
	NewVec->s.Y = TempPt.s.Y;
	NewVec->s.Z = FuncSqrt(1.0f - length);
    }
}

static Matrix4fT Transform = { {1.0f, 0.0f, 0.0f, 0.0f,	// NEW: Final Transform
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f}
};

static Matrix3fT LastRot = { {1.0f, 0.0f, 0.0f,	// NEW: Last Rotation
			      0.0f, 1.0f, 0.0f,
			      0.0f, 0.0f, 1.0f}
};

static Matrix3fT ThisRot = { {1.0f, 0.0f, 0.0f,	// NEW: This Rotation
			      0.0f, 1.0f, 0.0f,
			      0.0f, 0.0f, 1.0f}
};

//Create/Destroy
void init_arcBall(ArcBall_t * a, GLfloat NewWidth, GLfloat NewHeight)
{
    a->Transform = Transform;
    a->LastRot = LastRot;
    a->ThisRot = ThisRot;
    //Clear initial values
    a->StVec.s.X =
	a->StVec.s.Y =
	a->StVec.s.Z = a->EnVec.s.X = a->EnVec.s.Y = a->EnVec.s.Z = 0.0f;

    //Set initial bounds
    setBounds(a, NewWidth, NewHeight);

    a->isClicked = 0;
    a->isRClicked = 0;
    a->isDragging = 0;
}

//Mouse down
static void click(ArcBall_t * a, const Point2fT * NewPt)
{
    //Map the point to the sphere
    mapToSphere(a, NewPt, &a->StVec);
}

//Mouse drag, calculate rotation
static void drag(ArcBall_t * a, const Point2fT * NewPt, Quat4fT * NewRot)
{
    //Map the point to the sphere
    mapToSphere(a, NewPt, &a->EnVec);

    //Return the quaternion equivalent to the rotation
    if (NewRot) {
	Vector3fT Perp;

	//Compute the vector perpendicular to the begin and end vectors
	Vector3fCross(&Perp, &a->StVec, &a->EnVec);

	//Compute the length of the perpendicular vector
	if (Vector3fLength(&Perp) > Epsilon)	//if its non-zero
	{
	    //We're ok, so return the perpendicular vector as the transform after all
	    NewRot->s.X = Perp.s.X;
	    NewRot->s.Y = Perp.s.Y;
	    NewRot->s.Z = Perp.s.Z;
	    //In the quaternion values, w is cosine (theta / 2), where theta is rotation angle
	    NewRot->s.W = Vector3fDot(&a->StVec, &a->EnVec);
	} else			//if its zero
	{
	    //The begin and end vectors coincide, so return an identity transform
	    NewRot->s.X = NewRot->s.Y = NewRot->s.Z = NewRot->s.W = 0.0f;
	}
    }
}

#ifdef UNUSED
static void arcmouseRClick(ViewInfo * v)
{
    Matrix3fSetIdentity(&view->arcball->LastRot);	// Reset Rotation
    Matrix3fSetIdentity(&view->arcball->ThisRot);	// Reset Rotation
    Matrix4fSetRotationFromMatrix3f(&view->arcball->Transform, &view->arcball->ThisRot);	// Reset Rotation

}
#endif

void arcmouseClick(ViewInfo * v)
{
    view->arcball->isDragging = 1;	// Prepare For Dragging
    view->arcball->LastRot = view->arcball->ThisRot;	// Set Last Static Rotation To Last Dynamic One
    click(view->arcball, &view->arcball->MousePt);
//    printf ("arcmouse click \n");

}

void arcmouseDrag(ViewInfo * v)
{
    Quat4fT ThisQuat;
    drag(view->arcball, &view->arcball->MousePt, &ThisQuat);
    Matrix3fSetRotationFromQuat4f(&view->arcball->ThisRot, &ThisQuat);	// Convert Quaternion Into Matrix3fT
    Matrix3fMulMatrix3f(&view->arcball->ThisRot, &view->arcball->LastRot);	// Accumulate Last Rotation Into This One
    Matrix4fSetRotationFromMatrix3f(&view->arcball->Transform, &view->arcball->ThisRot);	// Set Our Final Transform's Rotation From This One

}

#ifdef UNUSED
void Update(ViewInfo * view)
{

    if (view->arcball->isRClicked)	// If Right Mouse Clicked, Reset All Rotations
    {
	Matrix3fSetIdentity(&view->arcball->LastRot);	// Reset Rotation
	Matrix3fSetIdentity(&view->arcball->ThisRot);	// Reset Rotation
	Matrix4fSetRotationFromMatrix3f(&view->arcball->Transform, &view->arcball->ThisRot);	// Reset Rotation
    }

    if (!view->arcball->isDragging)	// Not Dragging
    {
	if (view->arcball->isClicked)	// First Click
	{
	    view->arcball->isDragging = 1;	// Prepare For Dragging
	    view->arcball->LastRot = view->arcball->ThisRot;	// Set Last Static Rotation To Last Dynamic One
	    click(view->arcball, &view->arcball->MousePt);
	}
    } else {
	if (view->arcball->isClicked)	// Still Clicked, So Still Dragging
	{
	    Quat4fT ThisQuat;

	    drag(view->arcball, &view->arcball->MousePt, &ThisQuat);
	    Matrix3fSetRotationFromQuat4f(&view->arcball->ThisRot, &ThisQuat);	// Convert Quaternion Into Matrix3fT
	    Matrix3fMulMatrix3f(&view->arcball->ThisRot, &view->arcball->LastRot);	// Accumulate Last Rotation Into This One
	    Matrix4fSetRotationFromMatrix3f(&view->arcball->Transform, &view->arcball->ThisRot);	// Set Our Final Transform's Rotation From This One
	} else			// No Longer Dragging
	    view->arcball->isDragging = 0;
    }
}
#endif
