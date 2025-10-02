///////////////////////////////////////////////////////////////////
// Utils
// Generally useful definitions, structures, functions, etc.
//
// David Llewellyn-Jones
// http://www.flypig.co.uk
//
// Spring 2008
///////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////
// Includes

#include "utils.h"

///////////////////////////////////////////////////////////////////
// Defines

#define LENGTH_THRESHOLD	(0.001)

///////////////////////////////////////////////////////////////////
// Structures and enumerations

///////////////////////////////////////////////////////////////////
// Global variables

static Matrix3 mIdMul3 = {{{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}}};
static Matrix4 mIdMul4 = {{{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}}};

///////////////////////////////////////////////////////////////////
// Function prototypes

void RemoveRowColumn4 (Matrix3 * pmResult, Matrix4 const * pm1, int nRow, int nCol);

///////////////////////////////////////////////////////////////////
// Function definitions

float absf (float fValue) {
	if (fValue < 0.0f) fValue = -fValue;
	return fValue;
}

Vector3 Normal (Vector3 * v1, Vector3 * v2) {
	Vector3 vReturn;

	vReturn.fX = (v1->fY * v2->fZ) - (v1->fZ * v2->fY);
	vReturn.fY = (v1->fZ * v2->fX) - (v1->fX * v2->fZ);
	vReturn.fZ = (v1->fX * v2->fY) - (v1->fY * v2->fX);

	Normalise (& vReturn);

	return vReturn;
}

Vector3 NormalOrUp (Vector3 * v1, Vector3 * v2) {
	Vector3 vReturn;

	vReturn.fX = (v1->fY * v2->fZ) - (v1->fZ * v2->fY);
	vReturn.fY = (v1->fZ * v2->fX) - (v1->fX * v2->fZ);
	vReturn.fZ = (v1->fX * v2->fY) - (v1->fY * v2->fX);

	if (Length (& vReturn) < LENGTH_THRESHOLD) {
		SetVector3 (vReturn, 0.0, 0.0, 1.0);
	}
	else {
		Normalise (& vReturn);
	}

	return vReturn;
}

void Normalise (Vector3 * v1) {
	float fLength;

	fLength = sqrt ((v1->fX * v1->fX) + (v1->fY * v1->fY) + (v1->fZ * v1->fZ));

	v1->fX /= fLength;
	v1->fY /= fLength;
	v1->fZ /= fLength;
}

void Normalise3f (float * pfX, float * pfY, float * pfZ) {
	float fLength;

	fLength = sqrt (((*pfX) * (*pfX)) + ((*pfY) * (*pfY)) + ((*pfZ) * (*pfZ)));

	*pfX /= fLength;
	*pfY /= fLength;
	*pfZ /= fLength;
}

Vector3 AddVectors (Vector3 const * v1, Vector3 const * v2) {
	Vector3 vReturn;

	vReturn.fX = (v1->fX + v2->fX);
	vReturn.fY = (v1->fY + v2->fY);
	vReturn.fZ = (v1->fZ + v2->fZ);

	return vReturn;
}

Vector3 SubtractVectors (Vector3 const * v1, Vector3 const * v2) {
	Vector3 vReturn;

	vReturn.fX = (v1->fX - v2->fX);
	vReturn.fY = (v1->fY - v2->fY);
	vReturn.fZ = (v1->fZ - v2->fZ);

	return vReturn;
}

Vector3 ScaleVector (Vector3 const * v1, float fScale) {
	Vector3 vReturn;

	vReturn.fX = (v1->fX * fScale);
	vReturn.fY = (v1->fY * fScale);
	vReturn.fZ = (v1->fZ * fScale);

	return vReturn;
}

void ScaleVectorDirect (Vector3 * v1, float fScale) {
	v1->fX *= fScale;
	v1->fY *= fScale;
	v1->fZ *= fScale;
}

float Length (Vector3 * v1) {
	return sqrt ((v1->fX * v1->fX) + (v1->fY * v1->fY) + (v1->fZ * v1->fZ));
}

Matrix3 Invert (Matrix3 * m1) {
	Matrix3 vReturn;
	float fDet;

	fDet = Determinant (m1);
	if (fDet != 0.0f) {
		fDet = 1 / fDet;

		vReturn.fA1 =   fDet * ((m1->fB2 * m1->fC3) - (m1->fC2 * m1->fB3));
		vReturn.fA2 = - fDet * ((m1->fA2 * m1->fC3) - (m1->fC2 * m1->fA3));
		vReturn.fA3 =   fDet * ((m1->fA2 * m1->fB3) - (m1->fB2 * m1->fA3));

		vReturn.fB1 = - fDet * ((m1->fB1 * m1->fC3) - (m1->fC1 * m1->fB3));
		vReturn.fB2 =   fDet * ((m1->fA1 * m1->fC3) - (m1->fC1 * m1->fA3));
		vReturn.fB3 = - fDet * ((m1->fA1 * m1->fB3) - (m1->fB1 * m1->fA3));

		vReturn.fC1 =   fDet * ((m1->fB1 * m1->fC2) - (m1->fC1 * m1->fB2));
		vReturn.fC2 = - fDet * ((m1->fA1 * m1->fC2) - (m1->fC1 * m1->fA2));
		vReturn.fC3 =   fDet * ((m1->fA1 * m1->fB2) - (m1->fB1 * m1->fA2));
	}
	else {
		vReturn = mIdMul3;
	}

	return vReturn;	
}

void Invert4 (Matrix4 * pmResult, Matrix4 const * pm1) {
	float fDet;
	int nRow;
	int nCol;
	Matrix3 mSubMatrix;
	int nSign;

	fDet = Determinant4 (pm1);
	if (fDet != 0.0f) {
		fDet = 1 / fDet;

		nSign = 1;
		for (nRow = 0; nRow < 4; nRow++) {
			for (nCol = 0; nCol < 4; nCol++) {
				RemoveRowColumn4 (& mSubMatrix, pm1, nRow, nCol);
				
				pmResult->aafM[nRow][nCol] = ((float)nSign) * fDet * Determinant(& mSubMatrix);
				nSign = - nSign;
			}
		}
	}
	else {
		*pmResult = mIdMul4;
	}
}

float Determinant (Matrix3 * m1) {
	return (m1->fA1 * ((m1->fB2 * m1->fC3) - (m1->fB3 * m1->fC2)))
		- (m1->fA2 * ((m1->fB1 * m1->fC3) - (m1->fB3 * m1->fC1)))
		+ (m1->fA3 * ((m1->fB1 * m1->fC2) - (m1->fB2 * m1->fC1)));
}

float DotProdAngle (float fX1, float fY1, float fX2, float fY2) {
	float fAngle;
	float fScaler;

	float fY;
	float fRot;

	fRot = atan2 (fY1, fX1);
	fY = - (fX2 * sin (fRot)) + (fY2 * cos (fRot));

	fScaler = sqrt ((fX1 * fX1) + (fY1 * fY1)) * sqrt ((fX2 * fX2) + (fY2 * fY2));
	fAngle = acos (((fX1 * fX2) + (fY1 * fY2)) / fScaler);

	if (fY < 0) fAngle = -fAngle;

	return fAngle;
}

// Useful for calculating the result of simultaneous equations
// and therefore where the normal to a plane passes through a point
// [ a1 b1 c1 ] [ v1 ]   [ r1 ]   [ (a1*v1) + (b1*v2) + (c1*v3) ]
// [ a2 b2 c2 ] [ v2 ] = [ r3 ] = [ (a2*v1) + (b2*v2) + (c2*v3) ]
// [ a3 b3 c3 ] [ v3 ]   [ r3 ]   [ (a3*v1) + (b3*v2) + (c3*v3) ]
Vector3 MultMatrixVector (Matrix3 * m1, Vector3 * v1) {
	Vector3 vReturn;
	vReturn.fX = (m1->fA1 * v1->fX) + (m1->fB1 * v1->fY) + (m1->fC1 * v1->fZ);
	vReturn.fY = (m1->fA2 * v1->fX) + (m1->fB2 * v1->fY) + (m1->fC2 * v1->fZ);
	vReturn.fZ = (m1->fA3 * v1->fX) + (m1->fB3 * v1->fY) + (m1->fC3 * v1->fZ);

	return vReturn;
}

void PrintMatrix (Matrix3 * m1) {
	printf ("[ %f, \t%f, \t%f \t]\n", m1->fA1, m1->fB1, m1->fC1);
	printf ("[ %f, \t%f, \t%f \t]\n", m1->fA2, m1->fB2, m1->fC2);
	printf ("[ %f, \t%f, \t%f \t]\n", m1->fA3, m1->fB3, m1->fC3);
}

void PrintVector (Vector3 * v1) {
	printf ("[ %f, \t%f, \t%f \t]\n", v1->fX, v1->fY, v1->fZ);
}

Vector3 CrossProduct (Vector3 * v1, Vector3 * v2) {
	Vector3 vResult;
	
	vResult.fX = (v1->fY * v2->fZ) - (v1->fZ * v2->fY);
	vResult.fY = (v1->fZ * v2->fX) - (v1->fX * v2->fZ);
	vResult.fZ = (v1->fX * v2->fY) - (v1->fY * v2->fX);

	return vResult;
}

void RemoveRowColumn4 (Matrix3 * pmResult, Matrix4 const * pm1, int nRow, int nCol) {
	int nColTo;
	int nRowTo;
	int nColFrom;
	int nRowFrom;

	nColFrom = 0;
	for (nColTo = 0; nColTo < 3; nColTo++) {
		if (nColTo == nCol) {
			nColFrom++;
		}
		nRowFrom = 0;
		for (nRowTo = 0; nRowTo < 3; nRowTo++) {
			if (nRowTo == nRow) {
				nRowFrom++;
			}

			pmResult->aafM[nColTo][nRowTo] = pm1->aafM[nColFrom][nRowFrom];

			nRowFrom++;
		}
		nColFrom++;
	}
}

float Determinant4 (Matrix4 const * pm1) {
	Matrix3 mSubMatrix;
	int nColSub;
	int nSign;
	float fDeterminant;

	fDeterminant = 0.0f;
	nSign = 1;
	for (nColSub = 0; nColSub < 4; nColSub++) {
		// Copy the other columns and rows into a 3x3 matrix
		RemoveRowColumn4 (& mSubMatrix, pm1, 0, nColSub);

		// Calculate the contribution from this column
		fDeterminant += ((float)nSign) * pm1->aafM[nColSub][0] * Determinant (& mSubMatrix);
		nSign = -nSign;
	}
	
	return fDeterminant;
}

void MultMatrixMatrix4 (Matrix4 * pmResult, Matrix4 const * pm1, Matrix4 const * pm2) {
	int nCol;
	int nRow;
	int nIndex;
	float fCell;
	
	for (nCol = 0; nCol < 4; nCol++) {
		for (nRow = 0; nRow < 4; nRow++) {
			fCell = 0.0f;
			for (nIndex = 0; nIndex < 4; nIndex++) {
				fCell += pm1->aafM[nIndex][nRow] * pm2->aafM[nCol][nIndex];
			}
			pmResult->aafM[nCol][nRow] = fCell;
		}
	}
}

void MatrixCopy4 (Matrix4 * pmTo, Matrix4 * pmFrom) {
	int nRow;
	int nCol;
	
	for (nRow = 0; nRow < 4; nRow++) {
		for (nCol = 0; nCol < 4; nCol++) {
			pmTo->aafM[nRow][nCol] = pmFrom->aafM[nRow][nCol];
		}
	}
}

void MatrixSetIdentity4 (Matrix4 * pm1) {
	int nRow;
	int nCol;
	
	for (nRow = 0; nRow < 4; nRow++) {
		for (nCol = 0; nCol < 4; nCol++) {
			pm1->aafM[nRow][nCol] = (nRow == nCol);
		}
	}
}

void MatrixTranslate4 (Matrix4 * pm1, Vector3 const * pvTranslate) {
	Matrix4 mTranslate;
	Matrix4 mOriginal;

	MatrixSetIdentity4 (& mTranslate);

	mTranslate.aafM[3][0] = pvTranslate->fX;
	mTranslate.aafM[3][1] = pvTranslate->fY;
	mTranslate.aafM[3][2] = pvTranslate->fZ;

	MatrixCopy4 (& mOriginal, pm1);

	MultMatrixMatrix4 (pm1, & mOriginal, & mTranslate);
}

void MatrixScale4 (Matrix4 * pm1, Vector3 const * pvScale) {
	Matrix4 mScale;
	Matrix4 mOriginal;
	int nRow;
	int nCol;
	
	for (nRow = 0; nRow < 4; nRow++) {
		for (nCol = 0; nCol < 4; nCol++) {
			if (nRow == nCol) {
				if (nRow < 3) {
					mScale.aafM[nRow][nCol] = pvScale->afV[nRow];
				}
				else {
					mScale.aafM[nRow][nCol] = 1.0f;
				}
			}
			else {
				mScale.aafM[nRow][nCol] = 0.0f;
			}
		}
	}

	MatrixCopy4 (& mOriginal, pm1);

	MultMatrixMatrix4 (pm1, & mOriginal, & mScale);
}

void MatrixRotate4 (Matrix4 * pm1, float fAngle, Vector3 const * pvAxis) {
	float fCosine;
	float fSine;
	Vector3 vNormal;
	Matrix4 mRotate;
	Matrix4 mOriginal;
	
	vNormal.fX = pvAxis->fX;
	vNormal.fY = pvAxis->fY;
	vNormal.fZ = pvAxis->fZ;

	Normalise (& vNormal);

	fCosine = cos (fAngle * M_PI / 180.0f);
	fSine = sin (fAngle * M_PI / 180.0f);
	
	mRotate.aafM[0][0] = (vNormal.fX * vNormal.fX * (1.0f - fCosine)) + (fCosine);
	mRotate.aafM[0][1] = (vNormal.fY * vNormal.fX * (1.0f - fCosine)) + (vNormal.fZ * fSine);
	mRotate.aafM[0][2] = (vNormal.fZ * vNormal.fX * (1.0f - fCosine)) - (vNormal.fY * fSine);
	mRotate.aafM[0][3] = 0.0f;

	mRotate.aafM[1][0] = (vNormal.fX * vNormal.fY * (1.0f - fCosine)) - (vNormal.fZ * fSine);
	mRotate.aafM[1][1] = (vNormal.fY * vNormal.fY * (1.0f - fCosine)) + (fCosine);
	mRotate.aafM[1][2] = (vNormal.fZ * vNormal.fY * (1.0f - fCosine)) + (vNormal.fX * fSine);
	mRotate.aafM[1][3] = 0.0f;
	
	mRotate.aafM[2][0] = (vNormal.fX * vNormal.fZ * (1.0f - fCosine)) + (vNormal.fY * fSine);
	mRotate.aafM[2][1] = (vNormal.fY * vNormal.fZ * (1.0f - fCosine)) - (vNormal.fX * fSine);
	mRotate.aafM[2][2] = (vNormal.fZ * vNormal.fZ * (1.0f - fCosine)) + (fCosine);
	mRotate.aafM[2][3] = 0.0f;

	mRotate.aafM[3][0] = 0.0f;
	mRotate.aafM[3][1] = 0.0f;
	mRotate.aafM[3][2] = 0.0f;
	mRotate.aafM[3][3] = 1.0f;

/*	mRotate.aafM[0][0] = vNormal.fX * vNormal.fX * (1.0f - fCosine) + (fCosine);*/
/*	mRotate.aafM[1][0] = vNormal.fY * vNormal.fX * (1.0f - fCosine) + (vNormal.fZ * fSine);*/
/*	mRotate.aafM[2][0] = vNormal.fZ * vNormal.fX * (1.0f - fCosine) - (vNormal.fY * fSine);*/
/*	mRotate.aafM[3][0] = 0.0f;*/

/*	mRotate.aafM[0][1] = vNormal.fX * vNormal.fY * (1.0f - fCosine) - (vNormal.fZ * fSine);*/
/*	mRotate.aafM[1][1] = vNormal.fY * vNormal.fY * (1.0f - fCosine) + (fCosine);*/
/*	mRotate.aafM[2][1] = vNormal.fZ * vNormal.fY * (1.0f - fCosine) + (vNormal.fX * fSine);*/
/*	mRotate.aafM[3][1] = 0.0f;*/
/*	*/
/*	mRotate.aafM[0][2] = vNormal.fX * vNormal.fZ * (1.0f - fCosine) + (vNormal.fY * fSine);*/
/*	mRotate.aafM[1][2] = vNormal.fY * vNormal.fZ * (1.0f - fCosine) - (vNormal.fX * fSine);*/
/*	mRotate.aafM[2][2] = vNormal.fZ * vNormal.fZ * (1.0f - fCosine) + (fCosine);*/
/*	mRotate.aafM[3][2] = 0.0f;*/

/*	mRotate.aafM[0][3] = 0.0f;*/
/*	mRotate.aafM[1][3] = 0.0f;*/
/*	mRotate.aafM[2][3] = 0.0f;*/
/*	mRotate.aafM[3][3] = 1.0f;*/

	MatrixCopy4 (& mOriginal, pm1);

	MultMatrixMatrix4 (pm1, & mOriginal, & mRotate);	
}

void MultMatrixScalar4 (Matrix4 * pm1, float fScale) {
	int nIndex;
	
	for (nIndex = 0; nIndex < 16; nIndex++) {
		pm1->afM[nIndex] *= fScale;
	}
}

bool MatrixInvert2x2 (Matrix2 * pmOut1, Matrix2 const * pmIn1) {
	bool boSuccess;
	float fDeterminant;
	float fA1Out;
	float fB2Out;

	boSuccess = TRUE;
	fDeterminant = (pmIn1->fA1 * pmIn1->fB2) - (pmIn1->fA2 * pmIn1->fB1);
	if (fDeterminant != 0.0f) {
		if (pmOut1 != pmIn1) {
			pmOut1->fA1 = pmIn1->fB2 / fDeterminant;
			pmOut1->fB2 = pmIn1->fA1 / fDeterminant;
			pmOut1->fB1 = - pmIn1->fB1 / fDeterminant;
			pmOut1->fA2 = - pmIn1->fA2 / fDeterminant;
		}
		else {
			fA1Out = pmIn1->fB2 / fDeterminant;
			fB2Out = pmIn1->fA1 / fDeterminant;
			pmOut1->fA1 = fA1Out;
			pmOut1->fB2 = fB2Out;
			pmOut1->fB1 = - pmIn1->fB1 / fDeterminant;
			pmOut1->fA2 = - pmIn1->fA2 / fDeterminant;
		}
	}
	else {
		boSuccess = FALSE;
	}
	
	return boSuccess;
}

// Useful for calculating the result of simultaneous equations
// [ a1 b1 ] [ v1 ] = [ r1 ] = [ (a1*v1) + (b1*v2) ]
// [ a2 b2 ] [ v2 ]   [ r2 ]   [ (a2*v1) + (b2*v2) ]
void MultMatrix2x2Vector2 (Vector2 * pvOut1, Matrix2 const * pmIn1, Vector2 const * pvIn1) {
	Vector2 vOut;
	if (pvOut1 != pvIn1) {
		pvOut1->fX = (pmIn1->fA1 * pvIn1->fX) + (pmIn1->fB1 * pvIn1->fY);
		pvOut1->fY = (pmIn1->fA2 * pvIn1->fX) + (pmIn1->fB2 * pvIn1->fY);
	}
	else {
		vOut.fX = (pmIn1->fA1 * pvIn1->fX) + (pmIn1->fB1 * pvIn1->fY);
		vOut.fY = (pmIn1->fA2 * pvIn1->fX) + (pmIn1->fB2 * pvIn1->fY);
		pvOut1->fX = vOut.fX;
		pvOut1->fY = vOut.fY;
	}
}



