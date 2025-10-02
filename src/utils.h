///////////////////////////////////////////////////////////////////
// Utils
// Generally useful definitions, structures, functions, etc.
//
// David Llewellyn-Jones
// http://www.flypig.co.uk
//
// Spring 2008
///////////////////////////////////////////////////////////////////

#ifndef UTILS_H
#define UTILS_H

///////////////////////////////////////////////////////////////////
// Includes

#define _USE_MATH_DEFINES
#include <math.h>
#ifndef M_TWOPI
#define M_TWOPI (M_PI * 2.0)
#endif

#ifdef _WIN32
#include <windows.h>
#endif

#include <GLee.h>
#include <glib.h>
#include <GL/glut.h>
#include <gtk/gtk.h>
#include <stdbool.h>

///////////////////////////////////////////////////////////////////
// Defines

#define SetVector2(SET, X, Y) (SET).fX = (X); (SET).fY = (Y);
#define SetVector3(SET, X, Y, Z) (SET).fX = (X); (SET).fY = (Y); (SET).fZ = (Z);

///////////////////////////////////////////////////////////////////
// Structures and enumerations

typedef struct _Vector2 {
	union {
		struct {
			GLfloat fX;
			GLfloat fY;
		};
		GLfloat afV[2];
	};
} Vector2;

typedef struct _Vector3 {
	union {
		struct {
			GLfloat fX;
			GLfloat fY;
			GLfloat fZ;
		};
		GLfloat afV[3];
	};
} Vector3;

typedef struct _Matrix2 {
	union {
		struct {
			GLfloat fA1;
			GLfloat fA2;
			GLfloat fB1;
			GLfloat fB2;
		};
		GLfloat afM[4];
		GLfloat aafM[2][2];
	};
} Matrix2;

typedef struct _Matrix3 {
	union {
		struct {
			GLfloat fA1;
			GLfloat fA2;
			GLfloat fA3;
			GLfloat fB1;
			GLfloat fB2;
			GLfloat fB3;
			GLfloat fC1;
			GLfloat fC2;
			GLfloat fC3;
		};
		GLfloat afM[9];
		GLfloat aafM[3][3];
	};
} Matrix3;

typedef struct _Matrix4 {
	union {
		struct {
			GLfloat fA1;
			GLfloat fA2;
			GLfloat fA3;
			GLfloat fA4;
			GLfloat fB1;
			GLfloat fB2;
			GLfloat fB3;
			GLfloat fB4;
			GLfloat fC1;
			GLfloat fC2;
			GLfloat fC3;
			GLfloat fC4;
			GLfloat fD1;
			GLfloat fD2;
			GLfloat fD3;
			GLfloat fD4;
		};
		GLfloat afM[16];
		GLfloat aafM[4][4];
	};
} Matrix4;

///////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////
// Function prototypes

float absf (float fValue);
Vector3 Normal (Vector3 * v1, Vector3 * v2);
Vector3 NormalOrUp (Vector3 * v1, Vector3 * v2);
Vector3 AddVectors (Vector3 const * v1, Vector3 const * v2);
Vector3 SubtractVectors (Vector3 const * v1, Vector3 const * v2);
//Vector3 MultiplyVectors (Vector3 const * v1, Vector3 const * v2);
Vector3 ScaleVector (Vector3 const * v1, float fScale);
void ScaleVectorDirect (Vector3 * v1, float fScale);
void Normalise (Vector3 * v1);
void Normalise3f (float * pfX, float * pfY, float * pfZ);
float Length (Vector3 * v1);
Matrix3 Invert (Matrix3 * m1);
float Determinant (Matrix3 * m1);
float DotProdAngle (float fX1, float fY1, float fX2, float fY2);
//float DotProdAngleVector (Vector3 * v1, Vector3 * v2);
//float DotProdVector (Vector3 const * v1, Vector3 const * v2);
Vector3 MultMatrixVector (Matrix3 * m1, Vector3 * v1);
void PrintMatrix (Matrix3 * m1);
void PrintVector (Vector3 * v1);

Vector3 CrossProduct (Vector3 * v1, Vector3 * v2);
//Matrix3 MultMatrixMatrix (Matrix3 * m1, Matrix3 * m2);
//void SetIdentity (Matrix3 * m1);
//Matrix3 RotationBetweenVectors (Vector3 * v1, Vector3 * v2);
//Matrix3 RotationAngleAxis (Vector3 * vAxis, float fAngle);
//Vector3 PerpendicularVector (Vector3 * pvVector);

void Invert4 (Matrix4 * pmResult, Matrix4 const * pm1);
float Determinant4 (Matrix4 const * pm1);
void MultMatrixMatrix4 (Matrix4 * pmResult, Matrix4 const * pm1, Matrix4 const * pm2);

void MatrixCopy4 (Matrix4 * pmTo, Matrix4 * pmFrom);
void MatrixSetIdentity4 (Matrix4 * pm1);
void MatrixTranslate4 (Matrix4 * pm1, Vector3 const * pvTranslate);
void MatrixScale4 (Matrix4 * pm1, Vector3 const * pvScale);
void MatrixRotate4 (Matrix4 * pm1, float fAngle, Vector3 const * pvAxis);
void MultMatrixScalar4 (Matrix4 * pm1, float fScale);

bool MatrixInvert2x2 (Matrix2 * pmOut1, Matrix2 const * pmIn1);
void MultMatrix2x2Vector2 (Vector2 * pvOut1, Matrix2 const * pmIn1, Vector2 const * pvIn1);


///////////////////////////////////////////////////////////////////
// Function definitions

#endif /* UTILS_H */

