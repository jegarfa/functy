///////////////////////////////////////////////////////////////////
// Functy
// 3D graph drawing utility
//
// David Llewellyn-Jones
// http://www.flypig.co.uk
//
// Spring 2012
///////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////
// Includes

#include <stdint.h>

#include "vis.h"
#include "curve.h"
#include "function.h"
#include "function_private.h"
#include "vecsym.h"
#include "exportsvx.h"

#include <symbolic.h>

///////////////////////////////////////////////////////////////////
// Defines

#define FUNCTION0 "1"
#define INFLECTION_ZEROCHECK (0.00001f)
#define RADIUSSTEP_MIN (0.0001)

///////////////////////////////////////////////////////////////////
// Structures and enumerations

struct _CurvePersist {
	// Function details
	VecSym3 * pvCurve;
	VecSym3 * pvCurveDeriv;
	VecSym3 * pvCurveDeriv2;
	Operation * psRadiusDerivA;
	Operation * psRadiusDerivP;

	// Handy data to help with the UI
	GString * szXFunction;
	GString * szYFunction;
	GString * szZFunction;

	GString * szXCentre;
	GString * szYCentre;
	GString * szZCentre;

	// Centre information
	float fXCentre;
	float fYCentre;
	float fZCentre;
	Operation * psXCentre;
	Operation * psYCentre;
	Operation * psZCentre;
	Variable * psCentreVariables;
	Variable * psCentreVariableT;
	bool boCentreTimeDependent;

	// The following relate to screen coordinates
	double fAccuracyRadius;

	// Storage for the OpenGL vertex and normal data
	int nPieces;
	int nSegments;

	// Shader related variables
	GString * szShaderCurveX;
	GString * szShaderCurveY;
	GString * szShaderCurveZ;
	GString * szShaderDiffWrtAX;
	GString * szShaderDiffWrtAY;
	GString * szShaderDiffWrtAZ;
	GString * szShaderDiff2WrtAX;
	GString * szShaderDiff2WrtAY;
	GString * szShaderDiff2WrtAZ;
	GString * szShaderDiffRWrtA;
	GString * szShaderDiffRWrtP;
};

///////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////
// Function prototypes

void CurvePopulateVerticesColour (FuncPersist * psFuncData);
void CurvePopulateVerticesNoColour (FuncPersist * psFuncData);
void CurveResetVertices (FuncPersist * psFuncData);
void CurveRecalculateCentre (CurvePersist * psCurveData);
void CurveVertex (Vector3 * pvVertex, Vector3 * pvNormal, double fStep, double fTheta, bool boScreenCoords, double fMultiplier, double fScale, FuncPersist const * psFuncData);
void CurveFilledCuboidSlice (unsigned char * pcData, int nResolution, int nChannels, Vector3 * avCorners, float fZSlice);
void CurveFilledTetrahedronSlice (unsigned char * pcData, int nResolution, int nChannels, Vector3 * avVertices, float fZSlice);
void CurveRasteriseTriangle (unsigned char * pcData, int nResolution, int nChannels, Vector3 const * pvV1, Vector3 const * pvV2, Vector3 const * pvV3);
void CurveReorderQuadVertices (Vector3 * avBounds, int nVertices);
void CurveGetSpine (Vector3 * pvPos, double * pfRadius, Vector3 * pvOffset, float fP, FuncPersist * psFuncData);

///////////////////////////////////////////////////////////////////
// Function definitions

CurvePersist * NewCurvePersist () {
	CurvePersist * psCurveData;

	psCurveData = g_new0 (CurvePersist, 1);

	psCurveData->nPieces = 0;
	psCurveData->nSegments = 0;

	psCurveData->szXFunction = g_string_new ("a");
	psCurveData->szYFunction = g_string_new ("0");
	psCurveData->szZFunction = g_string_new ("0");

	psCurveData->fAccuracyRadius = 1.0f / 50.0f;

	psCurveData->pvCurve = NULL;
	psCurveData->pvCurveDeriv = NULL;
	psCurveData->pvCurveDeriv2 = NULL;
	psCurveData->psRadiusDerivA = NULL;
	psCurveData->psRadiusDerivP = NULL;

	psCurveData->fXCentre = 0.0;
	psCurveData->fYCentre = 0.0;
	psCurveData->fZCentre = 0.0;
	psCurveData->psXCentre = NULL;
	psCurveData->psYCentre = NULL;
	psCurveData->psZCentre = NULL;
	psCurveData->psCentreVariables = NULL;
	psCurveData->psCentreVariableT = NULL;
	psCurveData->boCentreTimeDependent = FALSE;

	psCurveData->szXCentre = g_string_new ("0.0");
	psCurveData->szYCentre = g_string_new ("0.0");
	psCurveData->szZCentre = g_string_new ("0.0");

	psCurveData->szShaderCurveX = g_string_new ("a");
	psCurveData->szShaderCurveY = g_string_new ("1.0");
	psCurveData->szShaderCurveZ = g_string_new ("1.0");
	psCurveData->szShaderDiffWrtAX = g_string_new ("1.0");
	psCurveData->szShaderDiffWrtAY = g_string_new ("1.0");
	psCurveData->szShaderDiffWrtAZ = g_string_new ("1.0");
	psCurveData->szShaderDiff2WrtAX = g_string_new ("1.0");
	psCurveData->szShaderDiff2WrtAY = g_string_new ("1.0");
	psCurveData->szShaderDiff2WrtAZ = g_string_new ("1.0");
	psCurveData->szShaderDiffRWrtA = g_string_new ("1.0");
	psCurveData->szShaderDiffRWrtP = g_string_new ("1.0");

	return psCurveData;
}

void DeleteCurvePersist (CurvePersist * psCurveData) {
	// Free up the function data
	if (psCurveData->szXFunction) {
		g_string_free (psCurveData->szXFunction, TRUE);
	}
	if (psCurveData->szYFunction) {
		g_string_free (psCurveData->szYFunction, TRUE);
	}
	if (psCurveData->szZFunction) {
		g_string_free (psCurveData->szZFunction, TRUE);
	}
	if (psCurveData->pvCurve) {
		DeleteVecSym3 (psCurveData->pvCurve);
		psCurveData->pvCurve = NULL;
	}
	if (psCurveData->pvCurveDeriv) {
		DeleteVecSym3 (psCurveData->pvCurveDeriv);
		psCurveData->pvCurveDeriv = NULL;
	}
	if (psCurveData->pvCurveDeriv2) {
		DeleteVecSym3 (psCurveData->pvCurveDeriv2);
		psCurveData->pvCurveDeriv2 = NULL;
	}
	if (psCurveData->psRadiusDerivA) {
		FreeRecursive (psCurveData->psRadiusDerivA);
		psCurveData->psRadiusDerivA = NULL;
	}
	if (psCurveData->psRadiusDerivP) {
		FreeRecursive (psCurveData->psRadiusDerivP);
		psCurveData->psRadiusDerivP = NULL;
	}
	if (psCurveData->szXCentre) {
		g_string_free (psCurveData->szXCentre, TRUE);
	}
	if (psCurveData->szYCentre) {
		g_string_free (psCurveData->szYCentre, TRUE);
	}
	if (psCurveData->szZCentre) {
		g_string_free (psCurveData->szZCentre, TRUE);
	}
	if (psCurveData->szShaderCurveX) {
		g_string_free (psCurveData->szShaderCurveX, TRUE);
	}
	if (psCurveData->szShaderCurveY) {
		g_string_free (psCurveData->szShaderCurveY, TRUE);
	}
	if (psCurveData->szShaderCurveZ) {
		g_string_free (psCurveData->szShaderCurveZ, TRUE);
	}
	if (psCurveData->szShaderDiffWrtAX) {
		g_string_free (psCurveData->szShaderDiffWrtAX, TRUE);
	}
	if (psCurveData->szShaderDiffWrtAY) {
		g_string_free (psCurveData->szShaderDiffWrtAY, TRUE);
	}
	if (psCurveData->szShaderDiffWrtAZ) {
		g_string_free (psCurveData->szShaderDiffWrtAZ, TRUE);
	}
	if (psCurveData->szShaderDiff2WrtAX) {
		g_string_free (psCurveData->szShaderDiff2WrtAX, TRUE);
	}
	if (psCurveData->szShaderDiff2WrtAY) {
		g_string_free (psCurveData->szShaderDiff2WrtAY, TRUE);
	}
	if (psCurveData->szShaderDiff2WrtAZ) {
		g_string_free (psCurveData->szShaderDiff2WrtAZ, TRUE);
	}
	if (psCurveData->szShaderDiffRWrtA) {
		g_string_free (psCurveData->szShaderDiffRWrtA, TRUE);
	}
	if (psCurveData->szShaderDiffRWrtP) {
		g_string_free (psCurveData->szShaderDiffRWrtP, TRUE);
	}

	// Free up the centre data
	FreeRecursive (psCurveData->psXCentre);
	FreeRecursive (psCurveData->psYCentre);
	FreeRecursive (psCurveData->psZCentre);

	psCurveData->psCentreVariables = FreeVariables (psCurveData->psCentreVariables);

	g_free (psCurveData);
}

void CurveSetFunction (char const * const szXFunction, char const * const szYFunction, char const * const szZFunction, char const * const szRadius, FuncPersist * psFuncData) {
	CurvePersist * psCurveData = psFuncData->Func.psCurveData;
	int nUserAssigned = 0;

	// Create the symbolic function vector if it doesn't already exit
	if (psCurveData->pvCurve == NULL) {
		psCurveData->pvCurve = CreateVecSym3 (NULL, NULL, NULL);
	}

	if (szXFunction) {
		// Free the previous version of the function
		if (psCurveData->pvCurve->psX) {
			FreeRecursive (psCurveData->pvCurve->psX);
		}
		// Set the new function
		psCurveData->pvCurve->psX = StringToOperation (szXFunction);
		nUserAssigned += AssignAllUserFuncs (psCurveData->pvCurve->psX, psFuncData->psUserFuncs);
		psCurveData->pvCurve->psX = UberSimplifyOperation (psCurveData->pvCurve->psX);
		// Set up the handy UI-related data
		g_string_assign (psCurveData->szXFunction, szXFunction);
	}

	if (szYFunction) {
		// Free the previous version of the function
		if (psCurveData->pvCurve->psY) {
			FreeRecursive (psCurveData->pvCurve->psY);
		}
		// Set the new function
		psCurveData->pvCurve->psY = StringToOperation (szYFunction);
		nUserAssigned += AssignAllUserFuncs (psCurveData->pvCurve->psY, psFuncData->psUserFuncs);
		psCurveData->pvCurve->psY = UberSimplifyOperation (psCurveData->pvCurve->psY);
		// Set up the handy UI-related data
		g_string_assign (psCurveData->szYFunction, szYFunction);
	}

	if (szZFunction) {
		// Free the previous version of the function
		if (psCurveData->pvCurve->psZ) {
			FreeRecursive (psCurveData->pvCurve->psZ);
		}
		// Set the new function
		psCurveData->pvCurve->psZ = StringToOperation (szZFunction);
		nUserAssigned += AssignAllUserFuncs (psCurveData->pvCurve->psZ, psFuncData->psUserFuncs);
		psCurveData->pvCurve->psZ = UberSimplifyOperation (psCurveData->pvCurve->psZ);
		// Set up the handy UI-related data
		g_string_assign (psCurveData->szZFunction, szZFunction);
	}

	if (szRadius) {
		// Free the previous version of the function
		if (psFuncData->psFunction) {
			FreeRecursive (psFuncData->psFunction);
		}
		// Set the new function
		psFuncData->psFunction = StringToOperation (szRadius);
		nUserAssigned += AssignAllUserFuncs (psFuncData->psFunction, psFuncData->psUserFuncs);
		psFuncData->psFunction = UberSimplifyOperation (psFuncData->psFunction);
		// Set up the handy UI-related data
		g_string_assign (psFuncData->szFunction, szRadius);
	}

	CurveDeriveFunctions (psFuncData);

	//psFuncData->boTimeDependent |= (nUserAssigned > 0);
	SETBIT (psFuncData->uTimeDependent, (nUserAssigned > 0), 1);
}


void CurveDeriveFunctions (FuncPersist * psFuncData) {
	CurvePersist * psCurveData = psFuncData->Func.psCurveData;
	Operation * psDerivA;
	Operation * psDerivP;
	char * szShaderFunction;
	int nFunctionLen;

	// Free up previous functions
	if (psCurveData->pvCurveDeriv) {
		DeleteVecSym3 (psCurveData->pvCurveDeriv);
		psCurveData->pvCurveDeriv = NULL;
	}

	if (psCurveData->pvCurveDeriv2) {
		DeleteVecSym3 (psCurveData->pvCurveDeriv2);
		psCurveData->pvCurveDeriv2 = NULL;
	}

	if (psCurveData->psRadiusDerivA) {
		FreeRecursive (psCurveData->psRadiusDerivA);
		psCurveData->psRadiusDerivA = NULL;
	}

	if (psCurveData->psRadiusDerivP) {
		FreeRecursive (psCurveData->psRadiusDerivP);
		psCurveData->psRadiusDerivP = NULL;
	}

	if (psFuncData->psVariables) {
		psFuncData->psVariables = FreeVariables (psFuncData->psVariables);
	}

	// Calculate derivatives and related functions that we'll need later
	psDerivA = CreateVariable ("a");
	psDerivP = CreateVariable ("p");

	psFuncData->psVariables = CreateVariables (psDerivA, psFuncData->psVariables);
	psFuncData->psVariables = CreateVariables (psDerivP, psFuncData->psVariables);
	psFuncData->psVariables = CreateVariables (psFuncData->psFunction, psFuncData->psVariables);
	psFuncData->psVariables = CreateVariablesVecSym (psCurveData->pvCurve, psFuncData->psVariables);

	psCurveData->pvCurveDeriv = DifferentiateVecSym (psCurveData->pvCurve, psDerivA);
	psCurveData->pvCurveDeriv = UberSimplifyVecSym (psCurveData->pvCurveDeriv);
	psFuncData->psVariables = CreateVariablesVecSym (psCurveData->pvCurveDeriv, psFuncData->psVariables);

	psCurveData->pvCurveDeriv2 = DifferentiateVecSym (psCurveData->pvCurveDeriv, psDerivA);
	psCurveData->pvCurveDeriv2 = UberSimplifyVecSym (psCurveData->pvCurveDeriv2);
	psFuncData->psVariables = CreateVariablesVecSym (psCurveData->pvCurveDeriv2, psFuncData->psVariables);

	psCurveData->psRadiusDerivA = DifferentiateOperation (psFuncData->psFunction, psDerivA);
	psCurveData->psRadiusDerivA = UberSimplifyOperation (psCurveData->psRadiusDerivA);
	psFuncData->psVariables = CreateVariables (psCurveData->psRadiusDerivA, psFuncData->psVariables);

	psCurveData->psRadiusDerivP = DifferentiateOperation (psFuncData->psFunction, psDerivP);
	psCurveData->psRadiusDerivP = UberSimplifyOperation (psCurveData->psRadiusDerivP);
	psFuncData->psVariables = CreateVariables (psCurveData->psRadiusDerivP, psFuncData->psVariables);

	// Check whether the curve is time-dependent
	psFuncData->psVariables = FreeVariables (psFuncData->psVariables);
	psFuncData->psVariableT = FindVariable (psFuncData->psVariables, "t");

	// Figure out if the functions are time dependent
	//psFuncData->boTimeDependent = (psFuncData->psVariableT != NULL);
	SETBIT (psFuncData->uTimeDependent, (psFuncData->psVariableT != NULL), 0);

	FreeRecursive (psDerivA);
	FreeRecursive (psDerivP);

	// Transfer the function details to the shader
	nFunctionLen = OperationToStringCLength (psFuncData->psFunction);
	szShaderFunction = (char *)malloc (nFunctionLen + 1);
	OperationToStringC (psFuncData->psFunction, szShaderFunction, nFunctionLen + 1);
	g_string_assign (psFuncData->szShaderFunction, szShaderFunction);
	free (szShaderFunction);

	nFunctionLen = OperationToStringCLength (psCurveData->pvCurve->psX);
	szShaderFunction = (char *)malloc (nFunctionLen + 1);
	OperationToStringC (psCurveData->pvCurve->psX, szShaderFunction, nFunctionLen + 1);
	g_string_assign (psCurveData->szShaderCurveX, szShaderFunction);
	free (szShaderFunction);

	nFunctionLen = OperationToStringCLength (psCurveData->pvCurve->psY);
	szShaderFunction = (char *)malloc (nFunctionLen + 1);
	OperationToStringC (psCurveData->pvCurve->psY, szShaderFunction, nFunctionLen + 1);
	g_string_assign (psCurveData->szShaderCurveY, szShaderFunction);
	free (szShaderFunction);

	nFunctionLen = OperationToStringCLength (psCurveData->pvCurve->psZ);
	szShaderFunction = (char *)malloc (nFunctionLen + 1);
	OperationToStringC (psCurveData->pvCurve->psZ, szShaderFunction, nFunctionLen + 1);
	g_string_assign (psCurveData->szShaderCurveZ, szShaderFunction);
	free (szShaderFunction);

	nFunctionLen = OperationToStringCLength (psCurveData->pvCurveDeriv->psX);
	szShaderFunction = (char *)malloc (nFunctionLen + 1);
	OperationToStringC (psCurveData->pvCurveDeriv->psX, szShaderFunction, nFunctionLen + 1);
	g_string_assign (psCurveData->szShaderDiffWrtAX, szShaderFunction);
	free (szShaderFunction);

	nFunctionLen = OperationToStringCLength (psCurveData->pvCurveDeriv->psY);
	szShaderFunction = (char *)malloc (nFunctionLen + 1);
	OperationToStringC (psCurveData->pvCurveDeriv->psY, szShaderFunction, nFunctionLen + 1);
	g_string_assign (psCurveData->szShaderDiffWrtAY, szShaderFunction);
	free (szShaderFunction);

	nFunctionLen = OperationToStringCLength (psCurveData->pvCurveDeriv->psZ);
	szShaderFunction = (char *)malloc (nFunctionLen + 1);
	OperationToStringC (psCurveData->pvCurveDeriv->psZ, szShaderFunction, nFunctionLen + 1);
	g_string_assign (psCurveData->szShaderDiffWrtAZ, szShaderFunction);
	free (szShaderFunction);

	nFunctionLen = OperationToStringCLength (psCurveData->pvCurveDeriv2->psX);
	szShaderFunction = (char *)malloc (nFunctionLen + 1);
	OperationToStringC (psCurveData->pvCurveDeriv2->psX, szShaderFunction, nFunctionLen + 1);
	g_string_assign (psCurveData->szShaderDiff2WrtAX, szShaderFunction);
	free (szShaderFunction);

	nFunctionLen = OperationToStringCLength (psCurveData->pvCurveDeriv2->psY);
	szShaderFunction = (char *)malloc (nFunctionLen + 1);
	OperationToStringC (psCurveData->pvCurveDeriv2->psY, szShaderFunction, nFunctionLen + 1);
	g_string_assign (psCurveData->szShaderDiff2WrtAY, szShaderFunction);
	free (szShaderFunction);

	nFunctionLen = OperationToStringCLength (psCurveData->pvCurveDeriv2->psZ);
	szShaderFunction = (char *)malloc (nFunctionLen + 1);
	OperationToStringC (psCurveData->pvCurveDeriv2->psZ, szShaderFunction, nFunctionLen + 1);
	g_string_assign (psCurveData->szShaderDiff2WrtAZ, szShaderFunction);
	free (szShaderFunction);

	nFunctionLen = OperationToStringCLength (psCurveData->psRadiusDerivA);
	szShaderFunction = (char *)malloc (nFunctionLen + 1);
	OperationToStringC (psCurveData->psRadiusDerivA, szShaderFunction, nFunctionLen + 1);
	g_string_assign (psCurveData->szShaderDiffRWrtA, szShaderFunction);
	free (szShaderFunction);

	nFunctionLen = OperationToStringCLength (psCurveData->psRadiusDerivP);
	szShaderFunction = (char *)malloc (nFunctionLen + 1);
	OperationToStringC (psCurveData->psRadiusDerivP, szShaderFunction, nFunctionLen + 1);
	g_string_assign (psCurveData->szShaderDiffRWrtP, szShaderFunction);
	free (szShaderFunction);
}

void CurveGenerateVertices (FuncPersist * psFuncData) {
	CurvePersist * psCurveData = psFuncData->Func.psCurveData;
	int nPieces;
	int nSegments;
	int nIndex;

	nPieces = floor (1.0 / psFuncData->fAccuracy) + 1;
	nSegments = floor (1.0 / psCurveData->fAccuracyRadius) + 1;

	psCurveData->nPieces = nPieces;
	psCurveData->nSegments = nSegments;

	FreeVertexBuffers (psFuncData);
	psFuncData->afVertices = g_new0 (GLfloat, nPieces * nSegments * 3);
	psFuncData->afNormals = g_new0 (GLfloat, nPieces * nSegments * 3);
	psFuncData->auIndices = g_new (GLushort, nSegments * 2);
	psFuncData->afColours = g_new0 (GLfloat, nPieces * nSegments * 4);
	psFuncData->afTextureCoords = g_new0 (GLfloat, nPieces * nSegments * 2);

	// Generate the index data
	for (nIndex = 0; nIndex < nSegments; nIndex++) {
		psFuncData->auIndices[(nIndex * 2) + 0] = nIndex;
		psFuncData->auIndices[(nIndex * 2) + 1] = nIndex + nSegments;
	}
	
	CurveResetVertices (psFuncData);
}

void CurveResetVertices (FuncPersist * psFuncData) {
	CurvePersist * psCurveData = psFuncData->Func.psCurveData;
	int nIndex;
	int nPiece;
	int nSegment;
	float fStep;
	float fTexTheta;
	float fTheta;

	// These don't need to be generated for curve coordinates
	// They're all populated dynamically
	// However, we store the a and p values for the benefit of the shader

	for (nPiece = 0; nPiece < psCurveData->nPieces; nPiece++) {
		fStep = ((float)(nPiece)) / ((float)(psCurveData->nPieces - 1.0f));

		for (nSegment = 0; nSegment < psCurveData->nSegments; nSegment++) {
			nIndex = (nPiece * psCurveData->nSegments) + nSegment;
			fTexTheta = ((float)nSegment) / ((float)psCurveData->nSegments - 1.0f);
			fTheta = fTexTheta * (2.0 * M_PI);

			psFuncData->afNormals[(nIndex * 3) + 0] = 0.0;
			psFuncData->afNormals[(nIndex * 3) + 1] = 0.0;
			psFuncData->afNormals[(nIndex * 3) + 2] = 1.0;

			psFuncData->afVertices[(nIndex * 3) + 0] = fStep;
			psFuncData->afVertices[(nIndex * 3) + 1] = fTheta;
			psFuncData->afVertices[(nIndex * 3) + 2] = 0.0;
		}
	}
}

void CurvePopulateVertices (FuncPersist * psFuncData) {
	bool boUseShader;

	boUseShader = GetShaderActive (psFuncData->psShaderData);

	if (boUseShader) {
		// Reset the vertices for the shader
		CurveResetVertices (psFuncData);
	}
	else{
		if (psFuncData->boColourFunction) {
			CurvePopulateVerticesColour (psFuncData);
		}
		else {
			CurvePopulateVerticesNoColour (psFuncData);
		}
	}
}

void CurvePopulateVerticesNoColour (FuncPersist * psFuncData) {
	CurvePersist * psCurveData = psFuncData->Func.psCurveData;
	int nPiece;
	int nSegment;
	Vector3 vPos;
	Vector3 vOffset;
	float fStep;
	float fTexTheta;
	float fTheta;
	int nIndex;
	Vector3 vTangent;
	Vector3 vNormal;
	Vector3 vBinormal;
	Vector3 vSecondDeriv;
	Vector3 vTwist1;
	Vector3 vTwist2;
	Vector3 vUp;
	float fSin;
	float fCos;
	Variable * psA;
	Variable * psP;
	Vector3 vA;
	Vector3 vP;
	float fRadius;
	float fRadiusDerivA;
	float fRadiusDerivP;
	Vector3 vN;
	double fTexXOffset = 0.0;
	double fTexYOffset = 0.0;
	double fTexXScale = 1.0;
	double fTexYScale = 1.0;
	double fXScreen;
	double fYScreen;
	double fZScreen;
	float fCurveLength;

	SetVector3 (vUp, 0.01f, 0.1f, 1.0f);
	Normalise (& vUp);

	sscanf (psFuncData->szTexXOffset->str, "%lf", & fTexXOffset);
	sscanf (psFuncData->szTexYOffset->str, "%lf", & fTexYOffset);
	sscanf (psFuncData->szTexXScale->str, "%lf", & fTexXScale);
	sscanf (psFuncData->szTexYScale->str, "%lf", & fTexYScale);

	// Create the parametric functions
	psA = FindVariable (psFuncData->psVariables, "a");
	psP = FindVariable (psFuncData->psVariables, "p");

	for (nPiece = 0; nPiece < psCurveData->nPieces; nPiece++) {
		// Calculate the translation due to the parametric curve 
		fStep = ((float)(nPiece)) / ((float)(psCurveData->nPieces - 1.0f));
		if (psA) {
			SetVariable (psA, fStep);
		}

		vPos = ApproximateVecSym (psCurveData->pvCurve);
		// Calculate the tangent vector
		vTangent = ApproximateVecSym (psCurveData->pvCurveDeriv);
		// Calculate the second derivative for the binormal calculation
		vSecondDeriv = ApproximateVecSym (psCurveData->pvCurveDeriv2);

		// Normalise the tangent vector
		fCurveLength = Length (& vTangent);
		Normalise (& vTangent);

		// Calculate the Frenet frame binormal vector
		vBinormal = CrossProduct (& vTangent, & vSecondDeriv);

		// Tackle inflection points and straight lines
		if (Length (& vBinormal) < INFLECTION_ZEROCHECK) {
			vBinormal = CrossProduct (& vTangent, & vUp);
		}
		Normalise (& vBinormal);

		// Calculate the Frenet frame normal vector
		vNormal = CrossProduct (& vTangent, & vBinormal);

		for (nSegment = 0; nSegment < psCurveData->nSegments; nSegment++) {
			nIndex = (nPiece * psCurveData->nSegments) + nSegment;
			fTexTheta = ((float)nSegment) / ((float)psCurveData->nSegments - 1.0f);
			fTheta = fTexTheta * (2.0 * M_PI);
			if (psP) {
				SetVariable (psP, fTheta);
			}
			fSin = sin (fTheta);
			fCos = cos (fTheta);

			// Transform the position in relation to the Frenet frame
			// vOffset = (vNormal * fSin) + (fBinormal * fCos);
			vTwist1 = ScaleVector (& vNormal, fCos);
			vTwist2 = ScaleVector (& vBinormal, fSin);
			vOffset = AddVectors (& vTwist1, & vTwist2);
			
			fRadius = ApproximateOperation (psFuncData->psFunction);
			fRadiusDerivA = ApproximateOperation (psCurveData->psRadiusDerivA);
			fRadiusDerivP = ApproximateOperation (psCurveData->psRadiusDerivP);

			// Set the curve position
			fXScreen = (((vPos.fX + (fRadius * vOffset.fX)) / psFuncData->fXWidth) * AXIS_XSIZE) - AXIS_XHSIZE;
			fYScreen = (((vPos.fY + (fRadius * vOffset.fY)) / psFuncData->fYWidth) * AXIS_YSIZE) - AXIS_YHSIZE;
			fZScreen = (((vPos.fZ + (fRadius * vOffset.fZ)) / psFuncData->fZWidth) * AXIS_ZSIZE) - AXIS_ZHSIZE;

			psFuncData->afVertices[(nIndex * 3) + 0] = fXScreen;
			psFuncData->afVertices[(nIndex * 3) + 1] = fYScreen;
			psFuncData->afVertices[(nIndex * 3) + 2] = fZScreen;

			// Calculate two perpendicular tangents to the curve
			vA.fX = fRadiusDerivA * fCos;
			vA.fY = fRadiusDerivA * fSin;
			vA.fZ = fCurveLength;

			vP.fX = ((fRadiusDerivP * fCos) - (fRadius * fSin));
			vP.fY = ((fRadiusDerivP * fSin) + (fRadius * fCos));
			vP.fZ = 0.0f;

			// Calculate the normal to the untransformed curve
			vN = Normal (& vP, & vA);

			// Set the curve normal transformed in relation to the Frenet frame
			psFuncData->afNormals[(nIndex * 3) + 0] = (vNormal.fX * vN.fX) + (vBinormal.fX * vN.fY) + (vTangent.fX * vN.fZ);
			psFuncData->afNormals[(nIndex * 3) + 1] = (vNormal.fY * vN.fX) + (vBinormal.fY * vN.fY) + (vTangent.fY * vN.fZ);
			psFuncData->afNormals[(nIndex * 3) + 2] = (vNormal.fZ * vN.fX) + (vBinormal.fZ * vN.fY) + (vTangent.fZ * vN.fZ);

			// Calculate vertex colour
			/*
			fColour = ApproximateOperation (psFuncData->psRed);
			psFuncData->afColours[(nIndex * 4) + 0] = fColour;
			fColour = ApproximateOperation (psFuncData->psGreen);
			psFuncData->afColours[(nIndex * 4) + 1] = fColour;
			fColour = ApproximateOperation (psFuncData->psBlue);
			psFuncData->afColours[(nIndex * 4) + 2] = fColour;
			fColour = ApproximateOperation (psFuncData->psAlpha);
			psFuncData->afColours[(nIndex * 4) + 3] = fColour;
			*/

			// Calculate vertex texture coordinate
			psFuncData->afTextureCoords[(nIndex * 2) + 0] = (fStep * fTexXScale) + fTexXOffset;
			psFuncData->afTextureCoords[(nIndex * 2) + 1] = (fTexTheta * fTexYScale) + fTexYOffset;
		}
	}
}

void CurvePopulateVerticesColour (FuncPersist * psFuncData) {
	CurvePersist * psCurveData = psFuncData->Func.psCurveData;
	int nPiece;
	int nSegment;
	Vector3 vPos;
	Vector3 vOffset;
	float fStep;
	float fTexTheta;
	float fTheta;
	int nIndex;
	Vector3 vTangent;
	Vector3 vNormal;
	Vector3 vBinormal;
	Vector3 vSecondDeriv;
	Vector3 vTwist1;
	Vector3 vTwist2;
	Vector3 vUp;
	float fSin;

	float fCos;
	Variable * psA;
	Variable * psP;
	Vector3 vA;
	Vector3 vP;
	float fRadius;
	float fRadiusDerivA;
	float fRadiusDerivP;
	Vector3 vN;
	double fTexXOffset = 0.0;
	double fTexYOffset = 0.0;
	double fTexXScale = 1.0;
	double fTexYScale = 1.0;
	float fColour;
	double fXScreen;
	double fYScreen;
	double fZScreen;
	float fCurveLength;

	SetVector3 (vUp, 0.01f, 0.1f, 1.0f);
	Normalise (& vUp);

	sscanf (psFuncData->szTexXOffset->str, "%lf", & fTexXOffset);
	sscanf (psFuncData->szTexYOffset->str, "%lf", & fTexYOffset);
	sscanf (psFuncData->szTexXScale->str, "%lf", & fTexXScale);
	sscanf (psFuncData->szTexYScale->str, "%lf", & fTexYScale);

	// Create the parametric functions
	psA = FindVariable (psFuncData->psVariables, "a");
	psP = FindVariable (psFuncData->psVariables, "p");

	for (nPiece = 0; nPiece < psCurveData->nPieces; nPiece++) {
		// Calculate the translation due to the parametric curve 
		fStep = ((float)(nPiece)) / ((float)(psCurveData->nPieces - 1.0f));
		if (psA) {
			SetVariable (psA, fStep);
		}

		vPos = ApproximateVecSym (psCurveData->pvCurve);
		// Calculate the tangent vector
		vTangent = ApproximateVecSym (psCurveData->pvCurveDeriv);
		// Calculate the second derivative for the binormal calculation
		vSecondDeriv = ApproximateVecSym (psCurveData->pvCurveDeriv2);

		// Normalise the tangent vector
		fCurveLength = Length (& vTangent);
		Normalise (& vTangent);

		// Calculate the Frenet frame binormal vector
		vBinormal = CrossProduct (& vTangent, & vSecondDeriv);

		// Tackle inflection points and straight lines
		if (Length (& vBinormal) < INFLECTION_ZEROCHECK) {
			vBinormal = CrossProduct (& vTangent, & vUp);
		}
		Normalise (& vBinormal);

		// Calculate the Frenet frame normal vector
		vNormal = CrossProduct (& vTangent, & vBinormal);

		for (nSegment = 0; nSegment < psCurveData->nSegments; nSegment++) {
			nIndex = (nPiece * psCurveData->nSegments) + nSegment;
			fTexTheta = ((float)nSegment) / ((float)psCurveData->nSegments - 1.0f);
			fTheta = fTexTheta * (2.0 * M_PI);
			if (psP) {
				SetVariable (psP, fTheta);
			}
			fSin = sin (fTheta);
			fCos = cos (fTheta);

			// Transform the position in relation to the Frenet frame
			// vOffset = (vNormal * fSin) + (fBinormal * fCos);
			vTwist1 = ScaleVector (& vNormal, fCos);
			vTwist2 = ScaleVector (& vBinormal, fSin);
			vOffset = AddVectors (& vTwist1, & vTwist2);
			
			fRadius = ApproximateOperation (psFuncData->psFunction);
			fRadiusDerivA = ApproximateOperation (psCurveData->psRadiusDerivA);
			fRadiusDerivP = ApproximateOperation (psCurveData->psRadiusDerivP);

			// Set the curve position
			fXScreen = (((vPos.fX + (fRadius * vOffset.fX)) / psFuncData->fXWidth) * AXIS_XSIZE) - AXIS_XHSIZE;
			fYScreen = (((vPos.fY + (fRadius * vOffset.fY)) / psFuncData->fYWidth) * AXIS_YSIZE) - AXIS_YHSIZE;
			fZScreen = (((vPos.fZ + (fRadius * vOffset.fZ)) / psFuncData->fZWidth) * AXIS_ZSIZE) - AXIS_ZHSIZE;

			psFuncData->afVertices[(nIndex * 3) + 0] = fXScreen;
			psFuncData->afVertices[(nIndex * 3) + 1] = fYScreen;
			psFuncData->afVertices[(nIndex * 3) + 2] = fZScreen;

			// Calculate two perpendicular tangents to the curve
			vA.fX = fRadiusDerivA * fCos;
			vA.fY = fRadiusDerivA * fSin;
			vA.fZ = fCurveLength;

			vP.fX = ((fRadiusDerivP * fCos) - (fRadius * fSin));
			vP.fY = ((fRadiusDerivP * fSin) + (fRadius * fCos));
			vP.fZ = 0.0f;

			// Calculate the normal to the untransformed curve
			vN = Normal (& vP, & vA);

			// Set the curve normal transformed in relation to the Frenet frame
			psFuncData->afNormals[(nIndex * 3) + 0] = (vNormal.fX * vN.fX) + (vBinormal.fX * vN.fY) + (vTangent.fX * vN.fZ);
			psFuncData->afNormals[(nIndex * 3) + 1] = (vNormal.fY * vN.fX) + (vBinormal.fY * vN.fY) + (vTangent.fY * vN.fZ);
			psFuncData->afNormals[(nIndex * 3) + 2] = (vNormal.fZ * vN.fX) + (vBinormal.fZ * vN.fY) + (vTangent.fZ * vN.fZ);

			// Calculate vertex colour
			fColour = ApproximateOperation (psFuncData->psRed);
			psFuncData->afColours[(nIndex * 4) + 0] = fColour;
			fColour = ApproximateOperation (psFuncData->psGreen);
			psFuncData->afColours[(nIndex * 4) + 1] = fColour;
			fColour = ApproximateOperation (psFuncData->psBlue);
			psFuncData->afColours[(nIndex * 4) + 2] = fColour;
			fColour = ApproximateOperation (psFuncData->psAlpha);
			psFuncData->afColours[(nIndex * 4) + 3] = fColour;

			// Calculate vertex texture coordinate
			psFuncData->afTextureCoords[(nIndex * 2) + 0] = (fStep * fTexXScale) + fTexXOffset;
			psFuncData->afTextureCoords[(nIndex * 2) + 1] = (fTexTheta * fTexYScale) + fTexYOffset;
		}
	}
}

void CurveSetFunctionColours (char const * const szRed, char const * const szGreen, char const * const szBlue, char const * const szAlpha, FuncPersist * psFuncData) {
	//CurvePersist * psCurveData = psFuncData->Func.psCurveData;
	double fApproximate;
	int nFunctionLen;
	char * szShaderFunction;
	int nUserAssigned = 0;

	// Free up any previous function
	FreeRecursive (psFuncData->psRed);
	FreeRecursive (psFuncData->psGreen);
	FreeRecursive (psFuncData->psBlue);
	FreeRecursive (psFuncData->psAlpha);
	psFuncData->psVariables = FreeVariables (psFuncData->psVariables);

	psFuncData->psRed = StringToOperation (szRed);
	nUserAssigned += AssignAllUserFuncs (psFuncData->psRed, psFuncData->psUserFuncs);
	psFuncData->psRed = UberSimplifyOperation (psFuncData->psRed);
	psFuncData->psGreen = StringToOperation (szGreen);
	nUserAssigned += AssignAllUserFuncs (psFuncData->psGreen, psFuncData->psUserFuncs);
	psFuncData->psGreen = UberSimplifyOperation (psFuncData->psGreen);
	psFuncData->psBlue = StringToOperation (szBlue);
	nUserAssigned += AssignAllUserFuncs (psFuncData->psBlue, psFuncData->psUserFuncs);
	psFuncData->psBlue = UberSimplifyOperation (psFuncData->psBlue);
	psFuncData->psAlpha = StringToOperation (szAlpha);
	nUserAssigned += AssignAllUserFuncs (psFuncData->psAlpha, psFuncData->psUserFuncs);
	psFuncData->psAlpha = UberSimplifyOperation (psFuncData->psAlpha);

	// Check if we can avoid recalculating the colour for every vertex
	psFuncData->boColourFunction = FALSE;
	fApproximate = ApproximateOperation (psFuncData->psRed);
	psFuncData->fRed = fApproximate;
	if (isnan (fApproximate)) {
		psFuncData->boColourFunction = TRUE;
	}
	fApproximate = ApproximateOperation (psFuncData->psGreen);
	psFuncData->fGreen = fApproximate;
	if (isnan (fApproximate)) {
		psFuncData->boColourFunction = TRUE;
	}
	fApproximate = ApproximateOperation (psFuncData->psBlue);
	psFuncData->fBlue = fApproximate;
	if (isnan (fApproximate)) {
		psFuncData->boColourFunction = TRUE;
	}
	fApproximate = ApproximateOperation (psFuncData->psAlpha);
	psFuncData->fAlpha = fApproximate;
	if (isnan (fApproximate)) {
		psFuncData->boColourFunction = TRUE;
	}
	if (nUserAssigned > 0) {
		psFuncData->boColourFunction = TRUE;
	}

	// Set up the variables
	psFuncData->psVariables = CreateVariables (psFuncData->psRed, psFuncData->psVariables);
	psFuncData->psVariables = CreateVariables (psFuncData->psGreen, psFuncData->psVariables);
	psFuncData->psVariables = CreateVariables (psFuncData->psBlue, psFuncData->psVariables);
	psFuncData->psVariables = CreateVariables (psFuncData->psAlpha, psFuncData->psVariables);

	// Free up any unused variables
	psFuncData->psVariables	= FreeVariables (psFuncData->psVariables);

	// Set up the handy UI-related data
	g_string_assign (psFuncData->szRed, szRed);
	g_string_assign (psFuncData->szGreen, szGreen);
	g_string_assign (psFuncData->szBlue, szBlue);
	g_string_assign (psFuncData->szAlpha, szAlpha);

	// Figure out if the functions are time dependent
	//psFuncData->boTimeDependent = (psFuncData->psVariableT != NULL) || (nUserAssigned > 0);
	SETBIT (psFuncData->uTimeDependent, (psFuncData->psVariableT != NULL), 0);
	SETBIT (psFuncData->uTimeDependent, (nUserAssigned > 0), 2);

	// Transfer the colour details to the shader
	nFunctionLen = OperationToStringCLength (psFuncData->psRed);
	szShaderFunction = (char *)malloc (nFunctionLen + 1);
	OperationToStringC (psFuncData->psRed, szShaderFunction, nFunctionLen + 1);
	g_string_assign (psFuncData->szShaderRed, szShaderFunction);
	free (szShaderFunction);

	nFunctionLen = OperationToStringCLength (psFuncData->psGreen);
	szShaderFunction = (char *)malloc (nFunctionLen + 1);
	OperationToStringC (psFuncData->psGreen, szShaderFunction, nFunctionLen + 1);
	g_string_assign (psFuncData->szShaderGreen, szShaderFunction);
	free (szShaderFunction);

	nFunctionLen = OperationToStringCLength (psFuncData->psBlue);
	szShaderFunction = (char *)malloc (nFunctionLen + 1);
	OperationToStringC (psFuncData->psBlue, szShaderFunction, nFunctionLen + 1);
	g_string_assign (psFuncData->szShaderBlue, szShaderFunction);
	free (szShaderFunction);

	nFunctionLen = OperationToStringCLength (psFuncData->psAlpha);
	szShaderFunction = (char *)malloc (nFunctionLen + 1);
	OperationToStringC (psFuncData->psAlpha, szShaderFunction, nFunctionLen + 1);
	g_string_assign (psFuncData->szShaderAlpha, szShaderFunction);
	free (szShaderFunction);
}

void CurveGetVertexDimensions (int * pnPieces, int * pnSegments, FuncPersist * psFuncData) {
	CurvePersist * psCurveData = psFuncData->Func.psCurveData;

	if (pnPieces) {
		* pnPieces = psCurveData->nPieces;
	}
	if (pnSegments) {
		* pnSegments = psCurveData->nSegments;
	}
}

char const * CurveGetXFunctionString (FuncPersist * psFuncData) {
	CurvePersist * psCurveData = psFuncData->Func.psCurveData;

	return psCurveData->szXFunction->str;
}

char const * CurveGetYFunctionString (FuncPersist * psFuncData) {
	CurvePersist * psCurveData = psFuncData->Func.psCurveData;

	return psCurveData->szYFunction->str;
}

char const * CurveGetZFunctionString (FuncPersist * psFuncData) {
	CurvePersist * psCurveData = psFuncData->Func.psCurveData;

	return psCurveData->szZFunction->str;
}

char const * CurveGetRadiusString (FuncPersist * psFuncData) {
	return psFuncData->szFunction->str;
}

char const * CurveGetXCentreString (FuncPersist * psFuncData) {
	CurvePersist * psCurveData = psFuncData->Func.psCurveData;

	return psCurveData->szXCentre->str;
}

char const * CurveGetYCentreString (FuncPersist * psFuncData) {
	CurvePersist * psCurveData = psFuncData->Func.psCurveData;

	return psCurveData->szYCentre->str;
}

char const * CurveGetZCentreString (FuncPersist * psFuncData) {
	CurvePersist * psCurveData = psFuncData->Func.psCurveData;

	return psCurveData->szZCentre->str;
}

void CurveSetFunctionCentre (char const * const szXCentre, char const * const szYCentre, char const * const szZCentre, FuncPersist * psFuncData) {
	CurvePersist * psCurveData = psFuncData->Func.psCurveData;
	int nUserAssigned = 0;

	// Free up any previous function
	FreeRecursive (psCurveData->psXCentre);
	FreeRecursive (psCurveData->psYCentre);
	FreeRecursive (psCurveData->psZCentre);
	psCurveData->psCentreVariables = FreeVariables (psCurveData->psCentreVariables);

	psCurveData->psXCentre = StringToOperation (szXCentre);
	nUserAssigned += AssignAllUserFuncs (psCurveData->psXCentre, psFuncData->psUserFuncs);
	psCurveData->psXCentre = UberSimplifyOperation (psCurveData->psXCentre);
	psCurveData->psYCentre = StringToOperation (szYCentre);
	nUserAssigned += AssignAllUserFuncs (psCurveData->psYCentre, psFuncData->psUserFuncs);
	psCurveData->psYCentre = UberSimplifyOperation (psCurveData->psYCentre);
	psCurveData->psZCentre = StringToOperation (szZCentre);
	nUserAssigned += AssignAllUserFuncs (psCurveData->psZCentre, psFuncData->psUserFuncs);
	psCurveData->psZCentre = UberSimplifyOperation (psCurveData->psZCentre);

	// Set up the variables
	// Note that we keep these variables separate from the main function
	// in order to keep the animations separate
	psCurveData->psCentreVariables = CreateVariables (psCurveData->psXCentre, psCurveData->psCentreVariables);
	psCurveData->psCentreVariables = CreateVariables (psCurveData->psYCentre, psCurveData->psCentreVariables);
	psCurveData->psCentreVariables = CreateVariables (psCurveData->psZCentre, psCurveData->psCentreVariables);

	// Find the t variable if it exists
	psCurveData->psCentreVariableT = FindVariable (psCurveData->psCentreVariables, "t");

	// Free up any unused variables
	psCurveData->psCentreVariables = FreeVariables (psCurveData->psCentreVariables);

	// Set up the handy UI-related data
	g_string_assign (psCurveData->szXCentre, szXCentre);
	g_string_assign (psCurveData->szYCentre, szYCentre);
	g_string_assign (psCurveData->szZCentre, szZCentre);

	// Figure out if the functions are time dependent
	psCurveData->boCentreTimeDependent = (psCurveData->psCentreVariableT != NULL) || (nUserAssigned > 0);

	AddUndefinedControlVars (psFuncData->psVariables, psFuncData->psFnControlData);
	AssignControlVarsToVariables (psCurveData->psCentreVariables, psFuncData->psFnControlData);

	// Check if we can avoid recalculating the centre for every frame
	psCurveData->fXCentre = ApproximateOperation (psCurveData->psXCentre);
	psCurveData->fYCentre = ApproximateOperation (psCurveData->psYCentre);
	psCurveData->fZCentre = ApproximateOperation (psCurveData->psZCentre);
}

bool CurveGetCentreTimeDependent (FuncPersist const * psFuncData) {
	CurvePersist * psCurveData = psFuncData->Func.psCurveData;

	return psCurveData->boCentreTimeDependent;
}

void CurveGetCentre (double * afCentre, FuncPersist const * psFuncData) {
	CurvePersist * psCurveData = psFuncData->Func.psCurveData;

	afCentre[0] = psCurveData->fXCentre;
	afCentre[1] = psCurveData->fYCentre;
	afCentre[2] = psCurveData->fZCentre;
}

void CurveUpdateCentre (FuncPersist * psFuncData) {
	CurvePersist * psCurveData = psFuncData->Func.psCurveData;

	if (psCurveData->boCentreTimeDependent) {
		CurveRecalculateCentre (psCurveData);
	}
}

void CurveRecalculateCentre (CurvePersist * psCurveData) {
	psCurveData->fXCentre = ApproximateOperation (psCurveData->psXCentre);
	psCurveData->fYCentre = ApproximateOperation (psCurveData->psYCentre);
	psCurveData->fZCentre = ApproximateOperation (psCurveData->psZCentre);
}

void CurveSetFunctionTime (double fTime, FuncPersist * psFuncData) {
	CurvePersist * psCurveData = psFuncData->Func.psCurveData;

	if (psCurveData->psCentreVariableT) {
		SetVariable (psCurveData->psCentreVariableT, fTime);
	}
}

void CurveSetFunctionPosition (double fXMin, double fYMin, double fZMin, FuncPersist * psFuncData) {
	Vector3 vPosition;

	psFuncData->fXMin = fXMin;
	psFuncData->fYMin = fYMin;
	psFuncData->fZMin = fZMin;

	vPosition.fX = AXIS_XHSIZE;
	vPosition.fY = AXIS_YHSIZE;
	vPosition.fZ = AXIS_ZHSIZE;
	SetShaderPosition (& vPosition, psFuncData->psShaderData);
	SetShaderPosition (& vPosition, psFuncData->psShaderShadowData);

	// It's not necessary to regenerate the vertices for the curve function
	// unless it's being scaled
	PopulateVertices (psFuncData);
}

char * CurveGenerateVertexShader (FuncPersist * psFuncData) {
	char * szShader;
	CurvePersist * psCurveData = psFuncData->Func.psCurveData;

	if (psFuncData->szShaderVertexSource) {
		szShader = ReplaceTextCopy (psFuncData->szShaderVertexSource, "function", psFuncData->szShaderFunction->str);

		szShader = ReplaceTextMove (szShader, "curveX", psCurveData->szShaderCurveX->str);
		szShader = ReplaceTextMove (szShader, "curveY", psCurveData->szShaderCurveY->str);
		szShader = ReplaceTextMove (szShader, "curveZ", psCurveData->szShaderCurveZ->str);

		szShader = ReplaceTextMove (szShader, "diffAX", psCurveData->szShaderDiffWrtAX->str);
		szShader = ReplaceTextMove (szShader, "diffAY", psCurveData->szShaderDiffWrtAY->str);
		szShader = ReplaceTextMove (szShader, "diffAZ", psCurveData->szShaderDiffWrtAZ->str);

		szShader = ReplaceTextMove (szShader, "diff2AX", psCurveData->szShaderDiff2WrtAX->str);
		szShader = ReplaceTextMove (szShader, "diff2AY", psCurveData->szShaderDiff2WrtAY->str);
		szShader = ReplaceTextMove (szShader, "diff2AZ", psCurveData->szShaderDiff2WrtAZ->str);
	}
	else {
		szShader = NULL;
	}

	return szShader;
}

char * CurveGenerateFragmentShader (FuncPersist * psFuncData) {
	char * szShader;
	CurvePersist * psCurveData = psFuncData->Func.psCurveData;

	if (psFuncData->szShaderFragmentSource) {
		szShader = ReplaceTextCopy (psFuncData->szShaderFragmentSource, "function", psFuncData->szShaderFunction->str);

		szShader = ReplaceTextMove (szShader, "diffAX", psCurveData->szShaderDiffWrtAX->str);
		szShader = ReplaceTextMove (szShader, "diffAY", psCurveData->szShaderDiffWrtAY->str);
		szShader = ReplaceTextMove (szShader, "diffAZ", psCurveData->szShaderDiffWrtAZ->str);

		szShader = ReplaceTextMove (szShader, "diff2AX", psCurveData->szShaderDiff2WrtAX->str);
		szShader = ReplaceTextMove (szShader, "diff2AY", psCurveData->szShaderDiff2WrtAY->str);
		szShader = ReplaceTextMove (szShader, "diff2AZ", psCurveData->szShaderDiff2WrtAZ->str);

		szShader = ReplaceTextMove (szShader, "diffA", psCurveData->szShaderDiffRWrtA->str);
		szShader = ReplaceTextMove (szShader, "diffP", psCurveData->szShaderDiffRWrtP->str);

		szShader = ReplaceTextMove (szShader, "red", psFuncData->szShaderRed->str);
		szShader = ReplaceTextMove (szShader, "green", psFuncData->szShaderGreen->str);
		szShader = ReplaceTextMove (szShader, "blue", psFuncData->szShaderBlue->str);
		szShader = ReplaceTextMove (szShader, "alpha", psFuncData->szShaderAlpha->str);
	}
	else {
		szShader = NULL;
	}

	return szShader;
}

char * CurveGenerateVertexShaderShadow (FuncPersist * psFuncData) {
	char * szShader;
	CurvePersist * psCurveData = psFuncData->Func.psCurveData;

	if (psFuncData->szShaderShadowVertexSource) {
		szShader = ReplaceTextCopy (psFuncData->szShaderShadowVertexSource, "function", psFuncData->szShaderFunction->str);

		szShader = ReplaceTextMove (szShader, "curveX", psCurveData->szShaderCurveX->str);
		szShader = ReplaceTextMove (szShader, "curveY", psCurveData->szShaderCurveY->str);
		szShader = ReplaceTextMove (szShader, "curveZ", psCurveData->szShaderCurveZ->str);

		szShader = ReplaceTextMove (szShader, "diffAX", psCurveData->szShaderDiffWrtAX->str);
		szShader = ReplaceTextMove (szShader, "diffAY", psCurveData->szShaderDiffWrtAY->str);
		szShader = ReplaceTextMove (szShader, "diffAZ", psCurveData->szShaderDiffWrtAZ->str);

		szShader = ReplaceTextMove (szShader, "diff2AX", psCurveData->szShaderDiff2WrtAX->str);
		szShader = ReplaceTextMove (szShader, "diff2AY", psCurveData->szShaderDiff2WrtAY->str);
		szShader = ReplaceTextMove (szShader, "diff2AZ", psCurveData->szShaderDiff2WrtAZ->str);
	}
	else {
		szShader = NULL;
	}

	return szShader;
}

char * CurveGenerateFragmentShaderShadow (FuncPersist * psFuncData) {
	char * szShader;
	//CurvePersist * psCurveData = psFuncData->Func.psCurveData;

	if (psFuncData->szShaderShadowFragmentSource) {
		//szShader = CopyText (psFuncData->szShaderShadowFragmentSource);
		szShader = ReplaceTextCopy (psFuncData->szShaderShadowFragmentSource, "alpha", psFuncData->szShaderAlpha->str);
	}
	else {
		szShader = NULL;
	}

	return szShader;
}

void CurveInitShader (FuncPersist * psFuncData) {
	GString * szPath;

	szPath = g_string_new ("/shaders/curve.vs");
	GenerateDataPath (szPath, psFuncData->psGlobalData);
	LoadVertexShader (szPath->str, psFuncData);

	szPath = g_string_assign (szPath, "/shaders/curve.fs");
	GenerateDataPath (szPath, psFuncData->psGlobalData);
	LoadFragmentShader (szPath->str, psFuncData);

	szPath = g_string_assign (szPath, "/shaders/curve-shadow.vs");
	GenerateDataPath (szPath, psFuncData->psGlobalData);
	LoadVertexShaderShadow (szPath->str, psFuncData);

	szPath = g_string_assign (szPath, "/shaders/curve-shadow.fs");
	GenerateDataPath (szPath, psFuncData->psGlobalData);
	LoadFragmentShaderShadow (szPath->str, psFuncData);

	g_string_free(szPath, TRUE);
	FunctionShadersRegenerate (psFuncData);
}

void CurveSetShaderActive (bool boActive, FuncPersist * psFuncData) {
	SetShaderActive (boActive, psFuncData->psShaderData);

	CurvePopulateVertices (psFuncData);
}

void CurveSetShaderShadowActive (bool boActive, FuncPersist * psFuncData) {
	SetShaderActive (boActive, psFuncData->psShaderShadowData);

	// TODO: Check whether we actually need to do this
	CurvePopulateVertices (psFuncData);
}

void CurveSetFunctionAccuracyRadius (double fAccuracy, FuncPersist * psFuncData) {
	CurvePersist * psCurveData = psFuncData->Func.psCurveData;

	// Don't do anything if the accuracy hasn't changed
	if (psCurveData->fAccuracyRadius != fAccuracy) {
		psCurveData->fAccuracyRadius = fAccuracy;

		FreeVertexBuffers (psFuncData);
		CurveGenerateVertices (psFuncData);
		CurvePopulateVertices (psFuncData);
	}
}

double CurveGetFunctionAccuracyRadius (FuncPersist * psFuncData) {
	CurvePersist * psCurveData = psFuncData->Func.psCurveData;

	return psCurveData->fAccuracyRadius;
}

int CurveOutputStoredVertices (Recall * hFile, bool boBinary, bool boScreenCoords, bool boExportAlpha, double fMultiplier, double fScale, FuncPersist const * psFuncData) {
	CurvePersist * psCurveData = psFuncData->Func.psCurveData;
	int nPiece;
	int nSegment;
	Vector3 vPos;
	Vector3 vOffset;
	float fStep;
	float fTexTheta;
	float fTheta;
	//int nIndex;
	Vector3 vTangent;
	Vector3 vNormal;
	Vector3 vBinormal;
	Vector3 vSecondDeriv;
	Vector3 vTwist1;
	Vector3 vTwist2;
	Vector3 vUp;
	float fSin;

	float fCos;
	Variable * psA;
	Variable * psP;
	Vector3 vA;
	Vector3 vP;
	float fRadius;
	float fRadiusDerivA;
	float fRadiusDerivP;
	Vector3 vN;
	double fTexXOffset = 0.0;
	double fTexYOffset = 0.0;
	double fTexXScale = 1.0;
	double fTexYScale = 1.0;
	float fColour;
	double fXScreen;
	double fYScreen;
	double fZScreen;
	double fXFunc;
	double fYFunc;
	double fZFunc;
	float fCurveLength;
	int nVertices;
	Vector3 vVertex;
	unsigned char ucColour[4];
	Vector3 vNormalOut;
	int nPieces;
	int nSegments;

	nPieces = (int)((float)psCurveData->nPieces * fMultiplier);
	if (nPieces < 2) {
		nPieces = 2;
	}
	nSegments = (int)((float)psCurveData->nSegments * fMultiplier);
	if (nSegments < 3) {
		nSegments = 3;
	}

	SetVector3 (vUp, 0.01f, 0.1f, 1.0f);
	Normalise (& vUp);

	sscanf (psFuncData->szTexXOffset->str, "%lf", & fTexXOffset);
	sscanf (psFuncData->szTexYOffset->str, "%lf", & fTexYOffset);
	sscanf (psFuncData->szTexXScale->str, "%lf", & fTexXScale);
	sscanf (psFuncData->szTexYScale->str, "%lf", & fTexYScale);

	// Create the parametric functions
	psA = FindVariable (psFuncData->psVariables, "a");
	psP = FindVariable (psFuncData->psVariables, "p");

	nVertices = 0;
	for (nPiece = 0; nPiece < nPieces; nPiece++) {
		// Calculate the translation due to the parametric curve 
		fStep = ((float)(nPiece)) / ((float)(nPieces - 1.0f));
		if (psA) {
			SetVariable (psA, fStep);
		}

		vPos = ApproximateVecSym (psCurveData->pvCurve);
		// Calculate the tangent vector
		vTangent = ApproximateVecSym (psCurveData->pvCurveDeriv);
		// Calculate the second derivative for the binormal calculation
		vSecondDeriv = ApproximateVecSym (psCurveData->pvCurveDeriv2);

		// Normalise the tangent vector
		fCurveLength = Length (& vTangent);
		Normalise (& vTangent);

		// Calculate the Frenet frame binormal vector
		vBinormal = CrossProduct (& vTangent, & vSecondDeriv);

		// Tackle inflection points and straight lines
		if (Length (& vBinormal) < INFLECTION_ZEROCHECK) {
			vBinormal = CrossProduct (& vTangent, & vUp);
		}
		Normalise (& vBinormal);

		// Calculate the Frenet frame normal vector
		vNormal = CrossProduct (& vTangent, & vBinormal);

		for (nSegment = 0; nSegment < nSegments; nSegment++) {
			//nIndex = (nPiece * nSegments) + nSegment;
			fTexTheta = ((float)nSegment) / ((float)nSegments - 1.0f);
			fTheta = fTexTheta * (2.0 * M_PI);
			if (psP) {
				SetVariable (psP, fTheta);
			}
			fSin = sin (fTheta);
			fCos = cos (fTheta);

			// Transform the position in relation to the Frenet frame
			// vOffset = (vNormal * fSin) + (fBinormal * fCos);
			vTwist1 = ScaleVector (& vNormal, fCos);
			vTwist2 = ScaleVector (& vBinormal, fSin);
			vOffset = AddVectors (& vTwist1, & vTwist2);
			
			fRadius = ApproximateOperation (psFuncData->psFunction);
			fRadiusDerivA = ApproximateOperation (psCurveData->psRadiusDerivA);
			fRadiusDerivP = ApproximateOperation (psCurveData->psRadiusDerivP);

			// Set the curve position
			fXFunc = (vPos.fX + (fRadius * vOffset.fX));
			fYFunc = (vPos.fY + (fRadius * vOffset.fY));
			fZFunc = (vPos.fZ + (fRadius * vOffset.fZ));

			// Translate the function
			fXFunc += psCurveData->fXCentre;
			fYFunc += psCurveData->fYCentre;
			fZFunc += psCurveData->fZCentre;

			// Scale and translate to screen coordinates
			fXScreen = ((fXFunc / psFuncData->fXWidth) * AXIS_XSIZE) - AXIS_XHSIZE;
			fYScreen = ((fYFunc / psFuncData->fYWidth) * AXIS_YSIZE) - AXIS_YHSIZE;
			fZScreen = ((fZFunc / psFuncData->fZWidth) * AXIS_ZSIZE) - AXIS_ZHSIZE;

			//psFuncData->afVertices[(nIndex * 3) + 0] = fXScreen;
			//psFuncData->afVertices[(nIndex * 3) + 1] = fYScreen;
			//psFuncData->afVertices[(nIndex * 3) + 2] = fZScreen;

			// Calculate two perpendicular tangents to the curve
			vA.fX = fRadiusDerivA * fCos;
			vA.fY = fRadiusDerivA * fSin;
			vA.fZ = fCurveLength;

			vP.fX = ((fRadiusDerivP * fCos) - (fRadius * fSin));
			vP.fY = ((fRadiusDerivP * fSin) + (fRadius * fCos));
			vP.fZ = 0.0f;

			// Calculate the normal to the untransformed curve
			vN = Normal (& vP, & vA);

			// Set the curve normal transformed in relation to the Frenet frame
			vNormalOut.fX = (vNormal.fX * vN.fX) + (vBinormal.fX * vN.fY) + (vTangent.fX * vN.fZ);
			vNormalOut.fY = (vNormal.fY * vN.fX) + (vBinormal.fY * vN.fY) + (vTangent.fY * vN.fZ);
			vNormalOut.fZ = (vNormal.fZ * vN.fX) + (vBinormal.fZ * vN.fY) + (vTangent.fZ * vN.fZ);

			// Calculate vertex colour
			fColour = ApproximateOperation (psFuncData->psRed);
			ucColour[0] = (unsigned char)(fColour * 255.0);
			fColour = ApproximateOperation (psFuncData->psGreen);
			ucColour[1] = (unsigned char)(fColour * 255.0);
			fColour = ApproximateOperation (psFuncData->psBlue);
			ucColour[2] = (unsigned char)(fColour * 255.0);
			fColour = ApproximateOperation (psFuncData->psAlpha);
			ucColour[3] = (unsigned char)(fColour * 255.0);

			// Calculate vertex texture coordinate
			//psFuncData->afTextureCoords[(nIndex * 2) + 0] = (fStep * fTexXScale) + fTexXOffset;
			//psFuncData->afTextureCoords[(nIndex * 2) + 1] = (fTexTheta * fTexYScale) + fTexYOffset;

			// Output the vertex to file
			if (boScreenCoords) {
				SetVector3 (vVertex, fXScreen, fYScreen, fZScreen);
			}
			else {
				SetVector3 (vVertex, fXFunc, fYFunc, fZFunc);
			}
			ScaleVectorDirect (& vVertex, fScale);

			if (boBinary) {
				recwrite (& vVertex, sizeof (float), 3, hFile);
				if (boExportAlpha) {
					recwrite (ucColour, sizeof (unsigned char), 4, hFile);
				}
				else {
					recwrite (ucColour, sizeof (unsigned char), 3, hFile);
				}
				recwrite (& vNormalOut, sizeof (float), 3, hFile);
			}
			else {
				recprintf (hFile, "%f %f %f ", vVertex.fX, vVertex.fY, vVertex.fZ);
				if (boExportAlpha) {
					recprintf (hFile, "%u %u %u %u ", ucColour[0], ucColour[1], ucColour[2], ucColour[3]);
				}
				else {
					recprintf (hFile, "%u %u %u ", ucColour[0], ucColour[1], ucColour[2]);
				}
				recprintf (hFile, "%f %f %f\n", vNormalOut.fX, vNormalOut.fY, vNormalOut.fZ);
			}
			nVertices++;
		}
	}

	return nVertices;
}

int CurveOutputStoredIndices (Recall * hFile, bool boBinary, double fMultiplier, int nOffset, FuncPersist const * psFuncData) {
	CurvePersist * psCurveData = psFuncData->Func.psCurveData;
	int nFaces;
	int anIndex[3];
	unsigned char uVertices;
	int nPiece;
	int nSegment;
	int nPieces;
	int nSegments;

	nPieces = (int)((float)psCurveData->nPieces * fMultiplier);
	if (nPieces < 2) {
		nPieces = 2;
	}
	nSegments = (int)((float)psCurveData->nSegments * fMultiplier);
	if (nSegments < 3) {
		nSegments = 3;
	}

	// Output index buffer identifiers
	uVertices = 3;
	nFaces = 0;
	for (nPiece = 0; nPiece < (nPieces - 1); nPiece++) {
		for (nSegment = 0; nSegment < nSegments; nSegment++) {
			anIndex[0] = ((nPiece + 0) * nSegments) + ((nSegment + 0) % nSegments) + nOffset;
			anIndex[1] = ((nPiece + 1) * nSegments) + ((nSegment + 0) % nSegments) + nOffset;
			anIndex[2] = ((nPiece + 0) * nSegments) + ((nSegment + 1) % nSegments) + nOffset;

			if (boBinary) {
				recwrite (& uVertices, sizeof (unsigned char), 1, hFile);
				recwrite (& anIndex, sizeof (int), 3, hFile);
			}
			else {
				recprintf (hFile, "3 %d %d %d\n", anIndex[0], anIndex[1], anIndex[2]);
			}
			nFaces++;

			anIndex[0] = ((nPiece + 0) * nSegments) + ((nSegment + 1) % nSegments) + nOffset;
			anIndex[1] = ((nPiece + 1) * nSegments) + ((nSegment + 0) % nSegments) + nOffset;
			anIndex[2] = ((nPiece + 1) * nSegments) + ((nSegment + 1) % nSegments) + nOffset;

			if (boBinary) {
				recwrite (& uVertices, sizeof (unsigned char), 1, hFile);
				recwrite (& anIndex, sizeof (int), 3, hFile);
			}
			else {
				recprintf (hFile, "3 %d %d %d\n", anIndex[0], anIndex[1], anIndex[2]);
			}
			nFaces++;
		}
	}

	return nFaces;
}

int CurveCountStoredVertices (double fMultiplier, FuncPersist const * psFuncData) {
	CurvePersist * psCurveData = psFuncData->Func.psCurveData;
	int nVertices;
	int nPieces;
	int nSegments;

	nPieces = (int)((float)psCurveData->nPieces * fMultiplier);
	if (nPieces < 2) {
		nPieces = 2;
	}
	nSegments = (int)((float)psCurveData->nSegments * fMultiplier);
	if (nSegments < 3) {
		nSegments = 3;
	}
	
	nVertices = nPieces * nSegments;
	
	return nVertices;
}

int CurveCountStoredFaces (double fMultiplier, FuncPersist const * psFuncData) {
	CurvePersist * psCurveData = psFuncData->Func.psCurveData;
	int nFaces;
	int nPieces;
	int nSegments;

	nPieces = (int)((float)psCurveData->nPieces * fMultiplier);
	if (nPieces < 2) {
		nPieces = 2;
	}
	nSegments = (int)((float)psCurveData->nSegments * fMultiplier);
	if (nSegments < 3) {
		nSegments = 3;
	}
	
	nFaces = (nPieces - 1) * nSegments * 2;

	return nFaces;
}

void CurveVertex (Vector3 * pvVertex, Vector3 * pvNormal, double fStep, double fTheta, bool boScreenCoords, double fMultiplier, double fScale, FuncPersist const * psFuncData) {
	CurvePersist * psCurveData = psFuncData->Func.psCurveData;
	double fXFunc;
	double fYFunc;
	double fZFunc;
	double fXScreen;
	double fYScreen;
	double fZScreen;
	Vector3 vA;
	Vector3 vP;
	Vector3 vN;
	Vector3 vVertex;

	Vector3 vPos;
	Vector3 vOffset;
	Vector3 vTangent;
	Vector3 vNormal;
	Vector3 vBinormal;
	Vector3 vSecondDeriv;
	Vector3 vTwist1;
	Vector3 vTwist2;
	Vector3 vUp;
	float fSin;

	float fCos;
	Variable * psA;
	Variable * psP;
	float fRadius;
	float fRadiusDerivA;
	float fRadiusDerivP;
	float fCurveLength;
	Vector3 vNormalOut;

	if ((pvVertex != NULL) || (pvNormal != NULL)) {
		SetVector3 (vUp, 0.01f, 0.1f, 1.0f);
		Normalise (& vUp);

		// Create the parametric functions
		psA = FindVariable (psFuncData->psVariables, "a");
		psP = FindVariable (psFuncData->psVariables, "p");

		if (psA) {
			SetVariable (psA, fStep);
		}

		vPos = ApproximateVecSym (psCurveData->pvCurve);
		// Calculate the tangent vector
		vTangent = ApproximateVecSym (psCurveData->pvCurveDeriv);
		// Calculate the second derivative for the binormal calculation
		vSecondDeriv = ApproximateVecSym (psCurveData->pvCurveDeriv2);

		// Normalise the tangent vector
		fCurveLength = Length (& vTangent);
		Normalise (& vTangent);

		// Calculate the Frenet frame binormal vector
		vBinormal = CrossProduct (& vTangent, & vSecondDeriv);

		// Tackle inflection points and straight lines
		if (Length (& vBinormal) < INFLECTION_ZEROCHECK) {
			vBinormal = CrossProduct (& vTangent, & vUp);
		}
		Normalise (& vBinormal);

		// Calculate the Frenet frame normal vector
		vNormal = CrossProduct (& vTangent, & vBinormal);

		if (psP) {
			SetVariable (psP, fTheta);
		}
		fSin = sin (fTheta);
		fCos = cos (fTheta);

		// Transform the position in relation to the Frenet frame
		// vOffset = (vNormal * fSin) + (fBinormal * fCos);
		vTwist1 = ScaleVector (& vNormal, fCos);
		vTwist2 = ScaleVector (& vBinormal, fSin);
		vOffset = AddVectors (& vTwist1, & vTwist2);
		
		fRadius = ApproximateOperation (psFuncData->psFunction);
		fRadiusDerivA = ApproximateOperation (psCurveData->psRadiusDerivA);
		fRadiusDerivP = ApproximateOperation (psCurveData->psRadiusDerivP);

		// Set the curve position
		fXFunc = (vPos.fX + (fRadius * vOffset.fX));
		fYFunc = (vPos.fY + (fRadius * vOffset.fY));
		fZFunc = (vPos.fZ + (fRadius * vOffset.fZ));

		// Translate the function
		fXFunc += psCurveData->fXCentre;
		fYFunc += psCurveData->fYCentre;
		fZFunc += psCurveData->fZCentre;

		// Scale and translate to screen coordinates
		fXScreen = (((fXFunc - psFuncData->fXMin) / psFuncData->fXWidth) * AXIS_XSIZE) - AXIS_XHSIZE;
		fYScreen = (((fYFunc - psFuncData->fYMin) / psFuncData->fYWidth) * AXIS_YSIZE) - AXIS_YHSIZE;
		fZScreen = (((fZFunc - psFuncData->fXMin) / psFuncData->fZWidth) * AXIS_ZSIZE) - AXIS_ZHSIZE;

		// Calculate two perpendicular tangents to the curve
		vA.fX = fRadiusDerivA * fCos;
		vA.fY = fRadiusDerivA * fSin;
		vA.fZ = fCurveLength;

		vP.fX = ((fRadiusDerivP * fCos) - (fRadius * fSin));
		vP.fY = ((fRadiusDerivP * fSin) + (fRadius * fCos));
		vP.fZ = 0.0f;

		// Calculate the normal to the untransformed curve
		vN = Normal (& vP, & vA);

		// Set the curve normal transformed in relation to the Frenet frame
		vNormalOut.fX = (vNormal.fX * vN.fX) + (vBinormal.fX * vN.fY) + (vTangent.fX * vN.fZ);
		vNormalOut.fY = (vNormal.fY * vN.fX) + (vBinormal.fY * vN.fY) + (vTangent.fY * vN.fZ);
		vNormalOut.fZ = (vNormal.fZ * vN.fX) + (vBinormal.fZ * vN.fY) + (vTangent.fZ * vN.fZ);

		// Output the vertex to file
		if (boScreenCoords) {
			SetVector3 (vVertex, fXScreen, fYScreen, fZScreen);
		}
		else {
			SetVector3 (vVertex, fXFunc, fYFunc, fZFunc);
		}
		ScaleVectorDirect (& vVertex, fScale);

		if (pvVertex) {
			*pvVertex = vVertex;
		}
		if (pvNormal) {
			*pvNormal = vNormalOut;
		}
	}
}

int CurveOutputStoredTrianglesSTL (Recall * hFile, bool boBinary, bool boScreenCoords, double fMultiplier, double fScale, FuncPersist const * psFuncData) {
	CurvePersist * psCurveData = psFuncData->Func.psCurveData;
	int nPiece;
	int nPieces;
	int nSegments;
	int nSegment;
	double fStep;
	double fTheta;
	double fTexTheta;
	Vector3 avPos[4];
	int nVertex;
	Vector3 vP;
	Vector3 vS;
	Vector3 vN;
	float afVector[3];
	uint16_t uAttribute = 0;
	int nTriangles = 0;

	nPieces = (int)((float)psCurveData->nPieces * fMultiplier);
	if (nPieces < 2) {
		nPieces = 2;
	}
	nSegments = (int)((float)psCurveData->nSegments * fMultiplier);
	if (nSegments < 3) {
		nSegments = 3;
	}

	for (nPiece = 0; nPiece < (nPieces - 1); nPiece++) {
		for (nSegment = 0; nSegment < (nSegments - 0); nSegment++) {

			for (nVertex = 0; nVertex < 4; nVertex++) {
				fStep = ((double)(((nPiece + (int)(nVertex / 2)) % nPieces))) / ((double)(nPieces - 1.0f));
				fTexTheta = ((double)(((nSegment + (nVertex % 2)) % nPieces))) / ((double)nSegments - 1.0f);
				fTheta = fTexTheta * (2.0 * M_PI);

				CurveVertex (& avPos[nVertex], NULL, fStep, fTheta, boScreenCoords, fMultiplier, fScale, psFuncData);
			}

			if (boBinary) {
				// Calculate the normals
				// Based on the triangles rather than the curves to avoid messy results
				vP = SubtractVectors (& avPos[1], & avPos[0]);
				vS = SubtractVectors (& avPos[2], & avPos[0]);
				vN = NormalOrUp (& vP, & vS);

				afVector[0] = vN.fX;
				afVector[1] = vN.fY;
				afVector[2] = vN.fZ;
				recwrite (afVector, sizeof(float), 3, hFile);

				afVector[0] = avPos[0].fX;
				afVector[1] = avPos[0].fY;
				afVector[2] = avPos[0].fZ;
				recwrite (afVector, sizeof(float), 3, hFile);

				afVector[0] = avPos[2].fX;
				afVector[1] = avPos[2].fY;
				afVector[2] = avPos[2].fZ;
				recwrite (afVector, sizeof(float), 3, hFile);

				afVector[0] = avPos[1].fX;
				afVector[1] = avPos[1].fY;
				afVector[2] = avPos[1].fZ;
				recwrite (afVector, sizeof(float), 3, hFile);

				recwrite (& uAttribute, sizeof(uint16_t), 1, hFile);

				// Calculate the normals
				// Based on the triangles rather than the curves to avoid messy results
				vP = SubtractVectors (& avPos[3], & avPos[1]);
				vS = SubtractVectors (& avPos[2], & avPos[1]);
				vN = NormalOrUp (& vP, & vS);

				afVector[0] = vN.fX;
				afVector[1] = vN.fY;
				afVector[2] = vN.fZ;
				recwrite (afVector, sizeof(float), 3, hFile);

				afVector[0] = avPos[1].fX;
				afVector[1] = avPos[1].fY;
				afVector[2] = avPos[1].fZ;
				recwrite (afVector, sizeof(float), 3, hFile);

				afVector[0] = avPos[2].fX;
				afVector[1] = avPos[2].fY;
				afVector[2] = avPos[2].fZ;
				recwrite (afVector, sizeof(float), 3, hFile);

				afVector[0] = avPos[3].fX;
				afVector[1] = avPos[3].fY;
				afVector[2] = avPos[3].fZ;
				recwrite (afVector, sizeof(float), 3, hFile);

				recwrite (& uAttribute, sizeof(uint16_t), 1, hFile);
				
				nTriangles += 2;
			}
			else {
				// Calculate the normals
				// Based on the triangles rather than the curves to avoid messy results
				vP = SubtractVectors (& avPos[1], & avPos[0]);
				vS = SubtractVectors (& avPos[2], & avPos[0]);
				vN = NormalOrUp (& vP, & vS);

				// Output the triangles
				recprintf (hFile, "facet normal %e %e %e\n", vN.fX, vN.fY, vN.fZ);
				recprintf (hFile, "outer loop\n");
				recprintf (hFile, "\tvertex %e %e %e\n", avPos[0].fX, avPos[0].fY, avPos[0].fZ);
				recprintf (hFile, "\tvertex %e %e %e\n", avPos[2].fX, avPos[2].fY, avPos[2].fZ);
				recprintf (hFile, "\tvertex %e %e %e\n", avPos[1].fX, avPos[1].fY, avPos[1].fZ);
				recprintf (hFile, "endloop\n");
				recprintf (hFile, "endfacet\n");

				// Calculate the normals
				// Based on the triangles rather than the curves to avoid messy results
				vP = SubtractVectors (& avPos[3], & avPos[1]);
				vS = SubtractVectors (& avPos[2], & avPos[1]);
				vN = NormalOrUp (& vP, & vS);

				recprintf (hFile, "facet normal %e %e %e\n", vN.fX, vN.fY, vN.fZ);
				recprintf (hFile, "outer loop\n");
				recprintf (hFile, "\tvertex %e %e %e\n", avPos[1].fX, avPos[1].fY, avPos[1].fZ);
				recprintf (hFile, "\tvertex %e %e %e\n", avPos[2].fX, avPos[2].fY, avPos[2].fZ);
				recprintf (hFile, "\tvertex %e %e %e\n", avPos[3].fX, avPos[3].fY, avPos[3].fZ);
				recprintf (hFile, "endloop\n");
				recprintf (hFile, "endfacet\n");
			}
		}
	}

	return nTriangles;
}

bool CurveAssignControlVarsToFunction (FnControlPersist * psFnControlData, FuncPersist * psFuncData) {
	CurvePersist * psCurveData = psFuncData->Func.psCurveData;
	bool boFound;

	boFound = AssignControlVarsToVariables (psCurveData->psCentreVariables, psFnControlData);
	
	if (boFound) {
		CurveRecalculateCentre (psCurveData);
	}
	
	return boFound;
}

void CurveOutputVoxelSlicePREV (unsigned char * pcData, int nResolution, int nChannels, int nSlice, FuncPersist * psFuncData) {
	CurvePersist * psCurveData = psFuncData->Func.psCurveData;
	int nPiece;
	int nPieces;
	int nSegments;
	int nSegment;
	double fStep;
	double fTheta;
	double fTexTheta;
	Variable * psA;
	Variable * psP;
	int nX;
	int nY;
	int nChannel;
	double fXFunc;
	double fYFunc;
	double fZFunc;
	double fXFuncCentre;
	double fYFuncCentre;
	double fZFuncCentre;
	double fXStep;
	double fYStep;
	double fZStep;
	double fZSlice;

	Vector3 vPos;
	Vector3 vOffset;
	Vector3 vTangent;
	Vector3 vNormal;
	Vector3 vBinormal;
	Vector3 vSecondDeriv;
	Vector3 vTwist1;
	Vector3 vTwist2;
	Vector3 vUp;
	float fSin;
	float fCos;
	float fRadius;
	float fRadiusStart;
	float fRadiusEnd;
	float fRadiusStep;

	fXStep = psFuncData->fXWidth / ((double)nResolution);
	fYStep = psFuncData->fYWidth / ((double)nResolution);
	fZStep = psFuncData->fZWidth / ((double)nResolution);

	SetVector3 (vUp, 0.01f, 0.1f, 1.0f);
	Normalise (& vUp);

	nPieces = floor (((16.0 * nResolution / 256.0) / psFuncData->fAccuracy)) + 1;
	nSegments = floor (((16.0 * nResolution / 256.0) / psCurveData->fAccuracyRadius)) + 1;

	fZSlice = psFuncData->fZMin + (nSlice * fZStep);

	psA = FindVariable (psFuncData->psVariables, "a");
	psP = FindVariable (psFuncData->psVariables, "p");

	for (nPiece = 0; nPiece < (nPieces - 1); nPiece++) {
		for (nSegment = 0; nSegment < (nSegments - 0); nSegment++) {
			fStep = ((double)(nPiece)) / ((double)(nPieces - 1.0f));
			fTexTheta = ((double)(nSegment)) / ((double)nSegments - 1.0f);
			fTheta = fTexTheta * (2.0 * M_PI);

			if (psA) {
				SetVariable (psA, fStep);
			}

			vPos = ApproximateVecSym (psCurveData->pvCurve);
			// Calculate the tangent vector
			vTangent = ApproximateVecSym (psCurveData->pvCurveDeriv);
			// Calculate the second derivative for the binormal calculation
			vSecondDeriv = ApproximateVecSym (psCurveData->pvCurveDeriv2);

			// Normalise the tangent vector
			Normalise (& vTangent);

			// Calculate the Frenet frame binormal vector
			vBinormal = CrossProduct (& vTangent, & vSecondDeriv);
			//printf ("(%f, %f)\n", vPos.fZ, vTangent.fZ);

			// Tackle inflection points and straight lines
			if (Length (& vBinormal) < INFLECTION_ZEROCHECK) {
				vBinormal = CrossProduct (& vTangent, & vUp);
			}
			Normalise (& vBinormal);

			// Calculate the Frenet frame normal vector
			vNormal = CrossProduct (& vTangent, & vBinormal);

			if (psP) {
				SetVariable (psP, fTheta);
			}
			fSin = sin (fTheta);
			fCos = cos (fTheta);

			// Transform the position in relation to the Frenet frame
			// vOffset = (vNormal * fSin) + (fBinormal * fCos);
			vTwist1 = ScaleVector (& vNormal, fCos);
			vTwist2 = ScaleVector (& vBinormal, fSin);
			vOffset = AddVectors (& vTwist1, & vTwist2);
		
			fRadiusEnd = ApproximateOperation (psFuncData->psFunction);

			if (psFuncData->boMaterialFill) {
				fRadiusStart = 0.0f;
			}
			else {
				fRadiusStart = fRadiusEnd - psFuncData->fMaterialThickness;
				if (fRadiusStart < 0.0) {
					fRadiusStart = 0.0;
				}
			}
			fRadiusStep = MIN (fXStep, MIN(fYStep, fZStep));
			if (fRadiusStep <= 0.0) {
				fRadiusStep = RADIUSSTEP_MIN;
			}

			// Translate the function
			fXFuncCentre = vPos.fX + psCurveData->fXCentre;
			fYFuncCentre = vPos.fY + psCurveData->fYCentre;
			fZFuncCentre = vPos.fZ + psCurveData->fZCentre;
			
			// Fill the full area from the inside 
			for (fRadius = fRadiusStart; fRadius <= fRadiusEnd; fRadius += fRadiusStep) {
				// Set the curve position
				fZFunc = (fZFuncCentre + (fRadius * vOffset.fZ));

				// Convert this to a position on the slice
				if ((fZFunc > fZSlice) && (fZFunc < (fZSlice + fZStep))) {
					fXFunc = (fXFuncCentre + (fRadius * vOffset.fX));
					fYFunc = (fYFuncCentre + (fRadius * vOffset.fY));

					nX = (fXFunc - psFuncData->fXMin) / fXStep;
					nY = (fYFunc - psFuncData->fYMin) / fYStep;

					if ((nX > 0) && (nX < nResolution) && (nY > 0) && (nY < nResolution)) {
						for (nChannel = 0; nChannel < nChannels; nChannel++) {
							pcData[(((nX + ((nResolution - nY - 1) * nResolution)) * nChannels) + nChannel)] = 255;
						}
					}
				}
			}
		}
	}
}

void CurveOutputVoxelSlice (unsigned char * pcData, int nResolution, int nChannels, int nSlice, FuncPersist * psFuncData) {
	CurvePersist * psCurveData = psFuncData->Func.psCurveData;
	int nPiece;
	int nPieces;
	int nSegments;
	int nSegment;
	double fStep;
	double fStepNext;
	double fTheta;
	double fTexTheta;
	double fThetaNext;
	Variable * psA;
	Variable * psP;
	double fXStep;
	double fYStep;
	double fZStep;
	///double fZSlice;

	Vector3 avPos[4];
	Vector3 avOffset[4];
	Vector3 vUp;
	double afRadius[4];
	float afRadiusStart[4];
	int nSpine;
	int nSpineInner;
	Vector3 avCorners[8];

	fXStep = psFuncData->fXWidth / ((double)nResolution);
	fYStep = psFuncData->fYWidth / ((double)nResolution);
	fZStep = psFuncData->fZWidth / ((double)nResolution);

	SetVector3 (vUp, 0.01f, 0.1f, 1.0f);
	Normalise (& vUp);

	nPieces = floor ((1.0 / psFuncData->fAccuracy)) + 1;
	nSegments = floor ((1.0 / psCurveData->fAccuracyRadius)) + 1;

	psA = FindVariable (psFuncData->psVariables, "a");
	psP = FindVariable (psFuncData->psVariables, "p");

	for (nPiece = 0; nPiece < (nPieces - 1); nPiece++) {
		for (nSegment = 0; nSegment < (nSegments - 0); nSegment++) {
			fStep = ((double)(nPiece)) / ((double)(nPieces - 1.0f));
			fStepNext = ((double)(nPiece + 1)) / ((double)(nPieces - 1.0f));
			fTexTheta = (((double)(nSegment)) / ((double)nSegments - 1.0f));
			fTheta = fTexTheta * (2.0 * M_PI);
			fThetaNext = (((double)(nSegment + 1)) / ((double)nSegments - 1.0f)) * (2.0 * M_PI);

			if (psA) {
				SetVariable (psA, fStep);
			}
			if (psP) {
				SetVariable (psP, fTheta);
			}
			CurveGetSpine (& avPos[0], & afRadius[0], & avOffset[0], fTheta, psFuncData);

			if (psP) {
				SetVariable (psP, fThetaNext);
			}
			CurveGetSpine (& avPos[1], & afRadius[1], & avOffset[1], fThetaNext, psFuncData);


			if (psA) {
				SetVariable (psA, fStepNext);
			}
			if (psP) {
				SetVariable (psP, fTheta);
			}
			CurveGetSpine (& avPos[3], & afRadius[3], & avOffset[3], fTheta, psFuncData);

			if (psP) {
				SetVariable (psP, fThetaNext);
			}
			CurveGetSpine (& avPos[2], & afRadius[2], & avOffset[2], fThetaNext, psFuncData);

			for (nSpine = 0; nSpine < 4; nSpine++) {
				if (psFuncData->boMaterialFill) {
					afRadiusStart[nSpine] = 0.0f;
				}
				else {
					afRadiusStart[nSpine] = afRadius[nSpine] - psFuncData->fMaterialThickness;
					if (afRadiusStart[nSpine] < 0.0) {
						afRadiusStart[nSpine] = 0.0;
					}
				}

				avCorners[nSpine] = ScaleVector (& avOffset[nSpine], afRadius[nSpine]);
				avCorners[nSpine] = AddVectors (& avCorners[nSpine], & avPos[nSpine]);
				avCorners[nSpine].fX = (avCorners[nSpine].fX - psFuncData->fXMin) / fXStep;
				avCorners[nSpine].fY = (avCorners[nSpine].fY - psFuncData->fYMin) / fYStep;
				avCorners[nSpine].fZ = (avCorners[nSpine].fZ - psFuncData->fZMin) / fZStep;

				nSpineInner = nSpine + 4;
				avCorners[nSpineInner] = ScaleVector (& avOffset[nSpine], afRadiusStart[nSpine]);
				avCorners[nSpineInner] = AddVectors (& avCorners[nSpineInner], & avPos[nSpine]);
				avCorners[nSpineInner].fX = (avCorners[nSpineInner].fX - psFuncData->fXMin) / fXStep;
				avCorners[nSpineInner].fY = (avCorners[nSpineInner].fY - psFuncData->fYMin) / fYStep;
				avCorners[nSpineInner].fZ = (avCorners[nSpineInner].fZ - psFuncData->fZMin) / fZStep;
			}

			// Convert this to a position on the slice
			//for (nSpine = 0; nSpine < 8; nSpine++) {
			//	if (((int)avCorners[nSpine].fZ == nSlice)) {
			//		int nX = (int)avCorners[nSpine].fX;
			//		int nY = (int)avCorners[nSpine].fY;
			//
			//		if ((nX > 0) && (nX < nResolution) && (nY > 0) && (nY < nResolution)) {
			//			int nChannel;
			//			for (nChannel = 0; nChannel < nChannels; nChannel++) {
			//				pcData[(((nX + ((nResolution - nY - 1) * nResolution)) * nChannels) + nChannel)] = 255;
			//			}
			//		}
			//	}
			//}

			// Render the cuboid
			FilledCuboidSliceSVX (pcData, nResolution, nChannels, avCorners, nSlice);
		}
	}
	
	// The commented code renders some example triangles for testing purpose
	//Vector3 avTriangle[3];
	//static float fRot = 0.0f;
	//fRot += 0.01;
	//SetVector3 (avTriangle[0], (256 + 100 * sin (fRot)), (256 + 100 * cos (fRot)), 0.0f);
	//SetVector3 (avTriangle[1], (256 + 100 * sin (fRot + M_PI * (2.0 / 3.0))), (256 + 100 * cos (fRot + M_PI * (2.0 / 3.0))), 0.0f);
	//SetVector3 (avTriangle[2], (256 + 100 * sin (fRot + M_PI * (4.0 / 3.0))), (256 + 100 * cos (fRot + M_PI * (4.0 / 3.0))), 0.0f);
	//CurveRasteriseTriangle (pcData, nResolution, nChannels, & avTriangle[0], & avTriangle[1], & avTriangle[2]);
}

void CurveGetSpine (Vector3 * pvPos, double * pfRadius, Vector3 * pvOffset, float fP, FuncPersist * psFuncData) {
	CurvePersist * psCurveData = psFuncData->Func.psCurveData;
	Vector3 vTangent;
	Vector3 vNormal;
	Vector3 vBinormal;
	Vector3 vSecondDeriv;
	Vector3 vTwist1;
	Vector3 vTwist2;
	Vector3 vUp;
	float fSin;
	float fCos;

	SetVector3 (vUp, 0.01f, 0.1f, 1.0f);
	Normalise (& vUp);

	*pvPos = ApproximateVecSym (psCurveData->pvCurve);
	// Calculate the tangent vector
	vTangent = ApproximateVecSym (psCurveData->pvCurveDeriv);
	// Calculate the second derivative for the binormal calculation
	vSecondDeriv = ApproximateVecSym (psCurveData->pvCurveDeriv2);

	// Normalise the tangent vector
	Normalise (& vTangent);

	// Calculate the Frenet frame binormal vector
	vBinormal = CrossProduct (& vTangent, & vSecondDeriv);
	//printf ("(%f, %f)\n", vPos.fZ, vTangent.fZ);

	// Tackle inflection points and straight lines
	if (Length (& vBinormal) < INFLECTION_ZEROCHECK) {
		vBinormal = CrossProduct (& vTangent, & vUp);
	}
	Normalise (& vBinormal);

	// Calculate the Frenet frame normal vector
	vNormal = CrossProduct (& vTangent, & vBinormal);

	fSin = sin (fP);
	fCos = cos (fP);

	// Transform the position in relation to the Frenet frame
	// vOffset = (vNormal * fSin) + (fBinormal * fCos);
	vTwist1 = ScaleVector (& vNormal, fCos);
	vTwist2 = ScaleVector (& vBinormal, fSin);
	*pvOffset = AddVectors (& vTwist1, & vTwist2);

	*pfRadius = ApproximateOperation (psFuncData->psFunction);

	// Translate the function
	pvPos->fX += psCurveData->fXCentre;
	pvPos->fY += psCurveData->fYCentre;
	pvPos->fZ += psCurveData->fZCentre;
}

