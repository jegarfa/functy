///////////////////////////////////////////////////////////////////
// Functy
// 3D graph drawing utility
//
// David Llewellyn-Jones
// http://www.flypig.co.uk
//
// Spring 2009
///////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////
// Includes

#include <stdint.h>

#include "vis.h"
#include "spherical.h"
#include "function.h"
#include "function_private.h"

#include <symbolic.h>

///////////////////////////////////////////////////////////////////
// Defines

#define FUNCTION0 "1"

///////////////////////////////////////////////////////////////////
// Structures and enumerations

struct _SphericalPersist {
	// The actual function
	Operation * psDiffWrtA;
	Operation * psDiffWrtP;
	Variable * psVariableA;
	Variable * psVariableP;
	Variable * psVariableR;

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

	// Handy data to help with the UI
	GString * szXCentre;
	GString * szYCentre;
	GString * szZCentre;

	// Storage for the OpenGL vertex and normal data
	int nAVertices;
	int nPVertices;

	// Shader related variables
	GString * szShaderDiffWrtA;
	GString * szShaderDiffWrtP;
};

///////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////
// Function prototypes

void SphericalPopulateVerticesColour (FuncPersist * psFuncData);
void SphericalPopulateVerticesNoColour (FuncPersist * psFuncData);
void SphericalResetVertices (FuncPersist * psFuncData);
void SphericalRecalculateCentre (SphericalPersist * psSphericalData);
void SphericalVertex (Vector3 * pvVertex, Vector3 * pvNormal, double fAFunc, double fPFunc, bool boScreenCoords, double fMultiplier, double fScale, FuncPersist const * psFuncData);

///////////////////////////////////////////////////////////////////
// Function definitions

SphericalPersist * NewSphericalPersist () {
	SphericalPersist * psSphericalData;

	psSphericalData = g_new0 (SphericalPersist, 1);

	psSphericalData->nAVertices = 0;
	psSphericalData->nPVertices = 0;

	psSphericalData->psDiffWrtA = NULL;
	psSphericalData->psDiffWrtP = NULL;
	psSphericalData->psVariableA = NULL;
	psSphericalData->psVariableP = NULL;
	psSphericalData->psVariableR = NULL;

	psSphericalData->fXCentre = 0.0;
	psSphericalData->fYCentre = 0.0;
	psSphericalData->fZCentre = 0.0;
	psSphericalData->psXCentre = NULL;
	psSphericalData->psYCentre = NULL;
	psSphericalData->psZCentre = NULL;
	psSphericalData->psCentreVariables = NULL;
	psSphericalData->psCentreVariableT = NULL;
	psSphericalData->boCentreTimeDependent = FALSE;

	psSphericalData->szXCentre = g_string_new ("0.0");
	psSphericalData->szYCentre = g_string_new ("0.0");
	psSphericalData->szZCentre = g_string_new ("0.0");

	psSphericalData->szShaderDiffWrtA = g_string_new ("0");
	psSphericalData->szShaderDiffWrtP = g_string_new ("0");

	return psSphericalData;
}

void DeleteSphericalPersist (SphericalPersist * psSphericalData) {
	// Free up the function data
	FreeRecursive (psSphericalData->psDiffWrtA);
	FreeRecursive (psSphericalData->psDiffWrtP);

	if (psSphericalData->szXCentre) {
		g_string_free (psSphericalData->szXCentre, TRUE);
	}
	if (psSphericalData->szYCentre) {
		g_string_free (psSphericalData->szYCentre, TRUE);
	}
	if (psSphericalData->szZCentre) {
		g_string_free (psSphericalData->szZCentre, TRUE);
	}
	if (psSphericalData->szShaderDiffWrtA) {
		g_string_free (psSphericalData->szShaderDiffWrtA, TRUE);
	}
	if (psSphericalData->szShaderDiffWrtP) {
		g_string_free (psSphericalData->szShaderDiffWrtP, TRUE);
	}

	// Free up the centre data
	FreeRecursive (psSphericalData->psXCentre);
	FreeRecursive (psSphericalData->psYCentre);
	FreeRecursive (psSphericalData->psZCentre);

	psSphericalData->psCentreVariables = FreeVariables (psSphericalData->psCentreVariables);

	g_free (psSphericalData);
}

void SphericalSetFunction (char const * const szFunction, FuncPersist * psFuncData) {
	SphericalPersist * psSphericalData = psFuncData->Func.psSphericalData;
	Operation * psWRT;
	char * szShaderFunction;
	int nFunctionLen;
	int nUserAssigned = 0;

	// Free up any previous function
	FreeRecursive (psFuncData->psFunction);
	FreeRecursive (psSphericalData->psDiffWrtA);
	FreeRecursive (psSphericalData->psDiffWrtP);
	psFuncData->psVariables = FreeVariables (psFuncData->psVariables);

	psFuncData->psFunction = StringToOperation (szFunction);
	nUserAssigned += AssignAllUserFuncs (psFuncData->psFunction, psFuncData->psUserFuncs);
	psFuncData->psFunction = UberSimplifyOperation (psFuncData->psFunction);

	// Differentiate with respect to a
	psWRT = CreateVariable ("a");
	psSphericalData->psDiffWrtA = DifferentiateOperation (psFuncData->psFunction, psWRT);
	FreeRecursive (psWRT);
	psSphericalData->psDiffWrtA = UberSimplifyOperation (psSphericalData->psDiffWrtA);

	// Differentiate with respect to p
	psWRT = CreateVariable ("p");
	psSphericalData->psDiffWrtP = DifferentiateOperation (psFuncData->psFunction, psWRT);
	FreeRecursive (psWRT);
	psSphericalData->psDiffWrtP = UberSimplifyOperation (psSphericalData->psDiffWrtP);

	// Set up the variables
	psFuncData->psVariables = CreateVariables (psFuncData->psFunction, psFuncData->psVariables);
	psFuncData->psVariables = CreateVariables (psSphericalData->psDiffWrtA, psFuncData->psVariables);
	psFuncData->psVariables = CreateVariables (psSphericalData->psDiffWrtP, psFuncData->psVariables);

	// Free up any unused variables
	psFuncData->psVariables	= FreeVariables (psFuncData->psVariables);

	// Find the a, p, r and t variables if they exist
	psSphericalData->psVariableA = FindVariable (psFuncData->psVariables, "a");
	psSphericalData->psVariableP = FindVariable (psFuncData->psVariables, "p");
	psSphericalData->psVariableR = FindVariable (psFuncData->psVariables, "r");
	psFuncData->psVariableT = FindVariable (psFuncData->psVariables, "t");

	// Set up the handy UI-related data
	g_string_assign (psFuncData->szFunction, szFunction);

	// Figure out if the functions are time dependent
	//psFuncData->boTimeDependent = (psFuncData->psVariableT != NULL) || (nUserAssigned > 0);
	SETBIT (psFuncData->uTimeDependent, (psFuncData->psVariableT != NULL), 0);
	SETBIT (psFuncData->uTimeDependent, (nUserAssigned > 0), 2);

	// Transfer the function details to the shader
	nFunctionLen = OperationToStringCLength (psFuncData->psFunction);
	szShaderFunction = (char *)malloc (nFunctionLen + 1);
	OperationToStringC (psFuncData->psFunction, szShaderFunction, nFunctionLen + 1);
	g_string_assign (psFuncData->szShaderFunction, szShaderFunction);
	free (szShaderFunction);

	nFunctionLen = OperationToStringCLength (psSphericalData->psDiffWrtA);
	szShaderFunction = (char *)malloc (nFunctionLen + 1);
	OperationToStringC (psSphericalData->psDiffWrtA, szShaderFunction, nFunctionLen + 1);
	g_string_assign (psSphericalData->szShaderDiffWrtA, szShaderFunction);
	free (szShaderFunction);

	nFunctionLen = OperationToStringCLength (psSphericalData->psDiffWrtP);
	szShaderFunction = (char *)malloc (nFunctionLen + 1);
	OperationToStringC (psSphericalData->psDiffWrtP, szShaderFunction, nFunctionLen + 1);
	g_string_assign (psSphericalData->szShaderDiffWrtP, szShaderFunction);
	free (szShaderFunction);
}

void SphericalSetFunctionColours (char const * const szRed, char const * const szGreen, char const * const szBlue, char const * const szAlpha, FuncPersist * psFuncData) {
	SphericalPersist * psSphericalData = psFuncData->Func.psSphericalData;
	double fApproximate;
	char * szShaderFunction;
	int nFunctionLen;
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

	// Find the a, p, r and t variables if they exist
	psSphericalData->psVariableA = FindVariable (psFuncData->psVariables, "a");
	psSphericalData->psVariableP = FindVariable (psFuncData->psVariables, "p");
	psSphericalData->psVariableR = FindVariable (psFuncData->psVariables, "r");
	psFuncData->psVariableT = FindVariable (psFuncData->psVariables, "t");

	// Free up any unused variables
	psFuncData->psVariables = FreeVariables (psFuncData->psVariables);

	// Set up the handy UI-related data
	g_string_assign (psFuncData->szRed, szRed);
	g_string_assign (psFuncData->szGreen, szGreen);
	g_string_assign (psFuncData->szBlue, szBlue);
	g_string_assign (psFuncData->szAlpha, szAlpha);

	// Figure out if the functions are time dependent
	//psFuncData->boTimeDependent = (psFuncData->psVariableT != NULL) || (nUserAssigned > 0);
	SETBIT (psFuncData->uTimeDependent, (psFuncData->psVariableT != NULL), 0);
	SETBIT (psFuncData->uTimeDependent, (nUserAssigned > 0), 1);

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

void SphericalGetVertexDimensions (int * pnAVertices, int * pnPVertices, FuncPersist * psFuncData) {
	SphericalPersist * psSphericalData = psFuncData->Func.psSphericalData;

	if (pnAVertices) {
		* pnAVertices = psSphericalData->nAVertices;
	}
	if (pnPVertices) {
		* pnPVertices = psSphericalData->nPVertices;
	}
}

void SphericalPopulateVertices (FuncPersist * psFuncData) {
	bool boUseShader;

	boUseShader = GetShaderActive (psFuncData->psShaderData);

	if (boUseShader) {
		// Reset the vertices for the shader
		SphericalResetVertices (psFuncData);
	}
	else {
		if (psFuncData->boColourFunction) {
			SphericalPopulateVerticesColour (psFuncData);
		}
		else {
			SphericalPopulateVerticesNoColour (psFuncData);
		}
	}
}

void SphericalPopulateVerticesNoColour (FuncPersist * psFuncData) {
	SphericalPersist * psSphericalData = psFuncData->Func.psSphericalData;
	int nVertex;
	double fA;
	double fP;
	double fAFunc;
	double fPFunc;
	double fRFunc;
	double fXScreen;
	double fYScreen;
	double fZScreen;
	Vector3 vA;
	Vector3 vP;
	Vector3 vN;
	double fEnd;
	double fDiff;
	double fTexXOffset = 0.0;
	double fTexYOffset = 0.0;
	double fTexXScale = 1.0;
	double fTexYScale = 1.0;

	sscanf (psFuncData->szTexXOffset->str, "%lf", & fTexXOffset);
	sscanf (psFuncData->szTexYOffset->str, "%lf", & fTexYOffset);
	sscanf (psFuncData->szTexXScale->str, "%lf", & fTexXScale);
	sscanf (psFuncData->szTexYScale->str, "%lf", & fTexYScale);

	fEnd = 1.0 + (psFuncData->fAccuracy / 2.0);
	nVertex = 0;
	for (fA = 0.0; fA < fEnd; fA += psFuncData->fAccuracy) {

		fAFunc = (fA * 2.0 * M_PI);

		for (fP = 0.0; fP < fEnd; fP += psFuncData->fAccuracy) {
			fPFunc = (fP * 1.0 * M_PI);

			if (psSphericalData->psVariableA) {
				SetVariable (psSphericalData->psVariableA, fAFunc);
			}
			if (psSphericalData->psVariableP) {
				SetVariable (psSphericalData->psVariableP, fPFunc);
			}

			fRFunc = ApproximateOperation (psFuncData->psFunction);

			// Calculate the normal
			fDiff = ApproximateOperation (psSphericalData->psDiffWrtA);

			vA.fX = sin (fPFunc) * ((fDiff * cos (fAFunc)) - (fRFunc * sin (fAFunc)));
			vA.fY = sin (fPFunc) * ((fDiff * sin (fAFunc)) + (fRFunc * cos (fAFunc)));
			vA.fZ = fDiff * cos (fPFunc);

			fDiff = ApproximateOperation (psSphericalData->psDiffWrtP);

			vP.fX = cos (fAFunc) * ((fDiff * sin (fPFunc)) + (fRFunc * cos (fPFunc)));
			vP.fY = sin (fAFunc) * ((fDiff * sin (fPFunc)) + (fRFunc * cos (fPFunc)));
			vP.fZ = (fDiff * cos (fPFunc)) - (fRFunc * sin (fPFunc));

			vN = Normal (& vA, & vP);

			// Since sin(p) is zero when p is zero, we need to fix the normal
			if (fPFunc == 0.0f) {
				vN.fX = 0.0;
				vN.fY = 0.0;
				vN.fZ = -1.0;
			}

			psFuncData->afNormals[(nVertex * 3)] = vN.fX;
			psFuncData->afNormals[(nVertex * 3) + 1] = vN.fY;
			psFuncData->afNormals[(nVertex * 3) + 2] = vN.fZ;

			// Calculate the cartesian position
			fXScreen = (((fRFunc * cos (fAFunc) * sin (fPFunc)) / psFuncData->fXWidth) * AXIS_XSIZE) - AXIS_XHSIZE;
			fYScreen = (((fRFunc * sin (fAFunc) * sin (fPFunc)) / psFuncData->fYWidth) * AXIS_YSIZE) - AXIS_YHSIZE;
			fZScreen = (((fRFunc * cos (fPFunc)) / psFuncData->fZWidth) * AXIS_ZSIZE) - AXIS_ZHSIZE;

			// Calculate vertex texture coordinate
			psFuncData->afTextureCoords[(nVertex * 2)] = (fAFunc * fTexXScale) + fTexXOffset;
			psFuncData->afTextureCoords[(nVertex * 2) + 1] = (fPFunc * fTexYScale) + fTexYOffset;

			psFuncData->afVertices[(nVertex * 3)] = fXScreen;
			psFuncData->afVertices[(nVertex * 3) + 1] = fYScreen;
			psFuncData->afVertices[(nVertex * 3) + 2] = fZScreen;

			nVertex++;
		}
	}
}

void SphericalPopulateVerticesColour (FuncPersist * psFuncData) {
	SphericalPersist * psSphericalData = psFuncData->Func.psSphericalData;
	int nVertex;
	double fA;
	double fP;
	double fAFunc;
	double fPFunc;
	double fRFunc;
	double fXScreen;
	double fYScreen;
	double fZScreen;
	Vector3 vA;
	Vector3 vP;
	Vector3 vN;
	double fEnd;
	float fColour;
	double fDiff;
	double fTexXOffset = 0.0;
	double fTexYOffset = 0.0;
	double fTexXScale = 1.0;
	double fTexYScale = 1.0;

	sscanf (psFuncData->szTexXOffset->str, "%lf", & fTexXOffset);
	sscanf (psFuncData->szTexYOffset->str, "%lf", & fTexYOffset);
	sscanf (psFuncData->szTexXScale->str, "%lf", & fTexXScale);
	sscanf (psFuncData->szTexYScale->str, "%lf", & fTexYScale);

	fEnd = 1.0 + (psFuncData->fAccuracy / 2.0);
	nVertex = 0;
	for (fA = 0.0; fA < fEnd; fA += psFuncData->fAccuracy) {

		fAFunc = (fA * 2.0 * M_PI);

		for (fP = 0.0; fP < fEnd; fP += psFuncData->fAccuracy) {
			fPFunc = (fP * 1.0 * M_PI);

			if (psSphericalData->psVariableA) {
				SetVariable (psSphericalData->psVariableA, fAFunc);
			}
			if (psSphericalData->psVariableP) {
				SetVariable (psSphericalData->psVariableP, fPFunc);
			}

			fRFunc = ApproximateOperation (psFuncData->psFunction);

			// Calculate the normal
			fDiff = ApproximateOperation (psSphericalData->psDiffWrtA);

			vA.fX = sin (fPFunc) * ((fDiff * cos (fAFunc)) - (fRFunc * sin (fAFunc)));
			vA.fY = sin (fPFunc) * ((fDiff * sin (fAFunc)) + (fRFunc * cos (fAFunc)));
			vA.fZ = fDiff * cos (fPFunc);

			fDiff = ApproximateOperation (psSphericalData->psDiffWrtP);

			vP.fX = cos (fAFunc) * ((fDiff * sin (fPFunc)) + (fRFunc * cos (fPFunc)));
			vP.fY = sin (fAFunc) * ((fDiff * sin (fPFunc)) + (fRFunc * cos (fPFunc)));
			vP.fZ = (fDiff * cos (fPFunc)) - (fRFunc * sin (fPFunc));

			vN = Normal (& vA, & vP);

			// Since sin(p) is zero when p is zero, we need to fix the normal
			if (fPFunc == 0.0f) {
				vN.fX = 0.0;
				vN.fY = 0.0;
				vN.fZ = -1.0;
			}

			psFuncData->afNormals[(nVertex * 3)] = vN.fX;
			psFuncData->afNormals[(nVertex * 3) + 1] = vN.fY;
			psFuncData->afNormals[(nVertex * 3) + 2] = vN.fZ;

			// Calculate the cartesian position
			fXScreen = (((fRFunc * cos (fAFunc) * sin (fPFunc)) / psFuncData->fXWidth) * AXIS_ZSIZE) - AXIS_ZHSIZE;
			fYScreen = (((fRFunc * sin (fAFunc) * sin (fPFunc)) / psFuncData->fYWidth) * AXIS_ZSIZE) - AXIS_ZHSIZE;
			fZScreen = (((fRFunc * cos (fPFunc)) / psFuncData->fZWidth) * AXIS_ZSIZE) - AXIS_ZHSIZE;

			if (psSphericalData->psVariableR) {
				SetVariable (psSphericalData->psVariableR, fRFunc);
			}

			// Calculate vertex colour
			fColour = ApproximateOperation (psFuncData->psRed);
			psFuncData->afColours[(nVertex * 4)] = fColour;
			fColour = ApproximateOperation (psFuncData->psGreen);
			psFuncData->afColours[(nVertex * 4) + 1] = fColour;
			fColour = ApproximateOperation (psFuncData->psBlue);
			psFuncData->afColours[(nVertex * 4) + 2] = fColour;
			fColour = ApproximateOperation (psFuncData->psAlpha);
			psFuncData->afColours[(nVertex * 4) + 3] = fColour;

			// Calculate vertex texture coordinate
			psFuncData->afTextureCoords[(nVertex * 2)] = (fA * fTexXScale) + fTexXOffset;
			psFuncData->afTextureCoords[(nVertex * 2) + 1] = (fP * fTexYScale) + fTexYOffset;

			psFuncData->afVertices[(nVertex * 3)] = fXScreen;
			psFuncData->afVertices[(nVertex * 3) + 1] = fYScreen;
			psFuncData->afVertices[(nVertex * 3) + 2] = fZScreen;

			nVertex++;
		}
	}
}

void SphericalGenerateVertices (FuncPersist * psFuncData) {
	SphericalPersist * psSphericalData = psFuncData->Func.psSphericalData;
	int nAVertices;
	int nPVertices;
	int nIndex;
/*
	double fX;
	double fY;
	double fXScreen;
	double fYScreen;
*/

	nAVertices = floor (1.0 / psFuncData->fAccuracy) + 1;
	nPVertices = floor (1.0 / psFuncData->fAccuracy) + 1;

	psSphericalData->nAVertices = nAVertices;
	psSphericalData->nPVertices = nPVertices;

	psFuncData->afVertices = g_new0 (GLfloat, nAVertices * nPVertices * 3);
	psFuncData->afNormals = g_new0 (GLfloat, nAVertices * nPVertices * 3);
	psFuncData->auIndices = g_new (GLushort, nPVertices * 2);
	psFuncData->afColours = g_new0 (GLfloat, nAVertices * nPVertices * 4);
	psFuncData->afTextureCoords = g_new0 (GLfloat, nAVertices * nPVertices * 2);

	// Generate the index data
	for (nIndex = 0; nIndex < nPVertices; nIndex++) {
		psFuncData->auIndices[(nIndex * 2)] = nIndex;
		psFuncData->auIndices[(nIndex * 2) + 1] = nIndex + nPVertices;
	}

	SphericalResetVertices (psFuncData);
}

void SphericalResetVertices (FuncPersist * psFuncData) {
	int nVertex;
	double fEnd;
	double fA;
	double fP;
	double fAScreen;
	double fPScreen;

	// These don't need to be generated for spherical coordinates
	// They're all populated dynamically
	// However we store the a and p values for the benefit of the shader

	fEnd = 1.0 + (psFuncData->fAccuracy / 2.0);
	nVertex = 0;
	for (fA = 0.0; fA < fEnd; fA += psFuncData->fAccuracy) {
		fAScreen = (fA * 2.0f * M_PI);

		for (fP = 0.0; fP < fEnd; fP += psFuncData->fAccuracy) {
			fPScreen = (fP * M_PI);

			psFuncData->afNormals[(nVertex * 3)] = 0.0;
			psFuncData->afNormals[(nVertex * 3) + 1] = 0.0;
			psFuncData->afNormals[(nVertex * 3) + 2] = 1.0;

			psFuncData->afVertices[(nVertex * 3)] = fAScreen;
			psFuncData->afVertices[(nVertex * 3) + 1] = fPScreen;
			psFuncData->afVertices[(nVertex * 3) + 2] = 0.0;

			nVertex++;
		}
	}
}

char const * SphericalGetXCentreString (FuncPersist * psFuncData) {
	SphericalPersist * psSphericalData = psFuncData->Func.psSphericalData;

	return psSphericalData->szXCentre->str;
}

char const * SphericalGetYCentreString (FuncPersist * psFuncData) {
	SphericalPersist * psSphericalData = psFuncData->Func.psSphericalData;

	return psSphericalData->szYCentre->str;
}

char const * SphericalGetZCentreString (FuncPersist * psFuncData) {
	SphericalPersist * psSphericalData = psFuncData->Func.psSphericalData;

	return psSphericalData->szZCentre->str;
}

void SphericalSetFunctionCentre (char const * const szXCentre, char const * const szYCentre, char const * const szZCentre, FuncPersist * psFuncData) {
	SphericalPersist * psSphericalData = psFuncData->Func.psSphericalData;
	int nUserAssigned = 0;

	// Free up any previous function
	FreeRecursive (psSphericalData->psXCentre);
	FreeRecursive (psSphericalData->psYCentre);
	FreeRecursive (psSphericalData->psZCentre);
	psSphericalData->psCentreVariables = FreeVariables (psSphericalData->psCentreVariables);

	psSphericalData->psXCentre = StringToOperation (szXCentre);
	nUserAssigned += AssignAllUserFuncs (psSphericalData->psXCentre, psFuncData->psUserFuncs);
	psSphericalData->psXCentre = UberSimplifyOperation (psSphericalData->psXCentre);
	psSphericalData->psYCentre = StringToOperation (szYCentre);
	nUserAssigned += AssignAllUserFuncs (psSphericalData->psYCentre, psFuncData->psUserFuncs);
	psSphericalData->psYCentre = UberSimplifyOperation (psSphericalData->psYCentre);
	psSphericalData->psZCentre = StringToOperation (szZCentre);
	nUserAssigned += AssignAllUserFuncs (psSphericalData->psZCentre, psFuncData->psUserFuncs);
	psSphericalData->psZCentre = UberSimplifyOperation (psSphericalData->psZCentre);

	// Set up the variables
	// Note that we keep these variables separate from the main function
	// in order to keep the animations separate
	psSphericalData->psCentreVariables = CreateVariables (psSphericalData->psXCentre, psSphericalData->psCentreVariables);
	psSphericalData->psCentreVariables = CreateVariables (psSphericalData->psYCentre, psSphericalData->psCentreVariables);
	psSphericalData->psCentreVariables = CreateVariables (psSphericalData->psZCentre, psSphericalData->psCentreVariables);

	// Find the t variable if it exists
	psSphericalData->psCentreVariableT = FindVariable (psSphericalData->psCentreVariables, "t");

	// Free up any unused variables
	psSphericalData->psCentreVariables = FreeVariables (psSphericalData->psCentreVariables);

	// Set up the handy UI-related data
	g_string_assign (psSphericalData->szXCentre, szXCentre);
	g_string_assign (psSphericalData->szYCentre, szYCentre);
	g_string_assign (psSphericalData->szZCentre, szZCentre);

	// Figure out if the functions are time dependent
	psSphericalData->boCentreTimeDependent = (psSphericalData->psCentreVariableT != NULL) || (nUserAssigned > 0);

	AddUndefinedControlVars (psFuncData->psVariables, psFuncData->psFnControlData);
	AssignControlVarsToVariables (psSphericalData->psCentreVariables, psFuncData->psFnControlData);

	// Check if we can avoid recalculating the centre for every frame
	psSphericalData->fXCentre = ApproximateOperation (psSphericalData->psXCentre);
	psSphericalData->fYCentre = ApproximateOperation (psSphericalData->psYCentre);
	psSphericalData->fZCentre = ApproximateOperation (psSphericalData->psZCentre);
}

bool SphericalGetCentreTimeDependent (FuncPersist const * psFuncData) {
	SphericalPersist * psSphericalData = psFuncData->Func.psSphericalData;

	return psSphericalData->boCentreTimeDependent;
}

void SphericalGetCentre (double * afCentre, FuncPersist const * psFuncData) {
	SphericalPersist * psSphericalData = psFuncData->Func.psSphericalData;

	afCentre[0] = psSphericalData->fXCentre;
	afCentre[1] = psSphericalData->fYCentre;
	afCentre[2] = psSphericalData->fZCentre;
}

void SphericalUpdateCentre (FuncPersist * psFuncData) {
	SphericalPersist * psSphericalData = psFuncData->Func.psSphericalData;

	if (psSphericalData->boCentreTimeDependent) {
		SphericalRecalculateCentre (psSphericalData);
	}
}

void SphericalRecalculateCentre (SphericalPersist * psSphericalData) {
	psSphericalData->fXCentre = ApproximateOperation (psSphericalData->psXCentre);
	psSphericalData->fYCentre = ApproximateOperation (psSphericalData->psYCentre);
	psSphericalData->fZCentre = ApproximateOperation (psSphericalData->psZCentre);
}

void SphericalSetFunctionTime (double fTime, FuncPersist * psFuncData) {
	SphericalPersist * psSphericalData = psFuncData->Func.psSphericalData;

	if (psSphericalData->psCentreVariableT) {
		SetVariable (psSphericalData->psCentreVariableT, fTime);
	}
}

void SphericalSetFunctionPosition (double fXMin, double fYMin, double fZMin, FuncPersist * psFuncData) {
	Vector3 vPosition;

	psFuncData->fXMin = fXMin;
	psFuncData->fYMin = fYMin;
	psFuncData->fZMin = fZMin;

	vPosition.fX = AXIS_XHSIZE;
	vPosition.fY = AXIS_YHSIZE;
	vPosition.fZ = AXIS_ZHSIZE;
	SetShaderPosition (& vPosition, psFuncData->psShaderData);
	SetShaderPosition (& vPosition, psFuncData->psShaderShadowData);

	// It's not necessary to regenerate the vertices for the spherical function
	// unless it's being scaled
	PopulateVertices (psFuncData);
}

char * SphericalGenerateVertexShader (FuncPersist * psFuncData) {
	char * szShader;
	SphericalPersist * psSphericalData = psFuncData->Func.psSphericalData;

	if (psFuncData->szShaderVertexSource) {
		szShader = ReplaceTextCopy (psFuncData->szShaderVertexSource, "function", psFuncData->szShaderFunction->str);
		szShader = ReplaceTextMove (szShader, "diffA", psSphericalData->szShaderDiffWrtA->str);
		szShader = ReplaceTextMove (szShader, "diffP", psSphericalData->szShaderDiffWrtP->str);
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

char * SphericalGenerateFragmentShader (FuncPersist * psFuncData) {
	char * szShader;
	SphericalPersist * psSphericalData = psFuncData->Func.psSphericalData;

	if (psFuncData->szShaderFragmentSource) {
		szShader = ReplaceTextCopy (psFuncData->szShaderFragmentSource, "function", psFuncData->szShaderFunction->str);
		szShader = ReplaceTextMove (szShader, "diffA", psSphericalData->szShaderDiffWrtA->str);
		szShader = ReplaceTextMove (szShader, "diffP", psSphericalData->szShaderDiffWrtP->str);
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

char * SphericalGenerateVertexShaderShadow (FuncPersist * psFuncData) {
	char * szShader;
	//SphericalPersist * psSphericalData = psFuncData->Func.psSphericalData;

	if (psFuncData->szShaderShadowVertexSource) {
		szShader = ReplaceTextCopy (psFuncData->szShaderShadowVertexSource, "function", psFuncData->szShaderFunction->str);
	}
	else {
		szShader = NULL;
	}

	return szShader;
}

char * SphericalGenerateFragmentShaderShadow (FuncPersist * psFuncData) {
	char * szShader;
	//SphericalPersist * psSphericalData = psFuncData->Func.psSphericalData;

	if (psFuncData->szShaderShadowFragmentSource) {
		szShader = CopyText (psFuncData->szShaderShadowFragmentSource);
	}
	else {
		szShader = NULL;
	}

	return szShader;
}

void SphericalInitShader (FuncPersist * psFuncData) {
	GString * szPath;

	szPath = g_string_new ("/shaders/spherical.vs");
	GenerateDataPath (szPath, psFuncData->psGlobalData);
	LoadVertexShader (szPath->str, psFuncData);

	szPath = g_string_assign (szPath, "/shaders/spherical.fs");
	GenerateDataPath (szPath, psFuncData->psGlobalData);
	LoadFragmentShader (szPath->str, psFuncData);
	
	szPath = g_string_assign (szPath, "/shaders/spherical-shadow.vs");
	GenerateDataPath (szPath, psFuncData->psGlobalData);
	LoadVertexShaderShadow (szPath->str, psFuncData);

	szPath = g_string_assign (szPath, "/shaders/spherical-shadow.fs");
	GenerateDataPath (szPath, psFuncData->psGlobalData);
	LoadFragmentShaderShadow (szPath->str, psFuncData);
	
	g_string_free (szPath, TRUE);
	FunctionShadersRegenerate (psFuncData);
}

void SphericalSetShaderActive (bool boActive, FuncPersist * psFuncData) {
	SetShaderActive (boActive, psFuncData->psShaderData);

	SphericalPopulateVertices (psFuncData);
}

void SphericalSetShaderShadowActive (bool boActive, FuncPersist * psFuncData) {
	SetShaderActive (boActive, psFuncData->psShaderShadowData);

	// TODO: Check whether we actually need to do this
	SphericalPopulateVertices (psFuncData);
}

int SphericalOutputStoredVertices (Recall * hFile, bool boBinary, bool boScreenCoords, bool boExportAlpha, double fMultiplier, double fScale, FuncPersist const * psFuncData) {
	SphericalPersist * psSphericalData = psFuncData->Func.psSphericalData;
	int nVertex;
	double fA;
	double fP;
	double fAFunc;
	double fPFunc;
	double fRFunc;
	double fXFunc;
	double fYFunc;
	double fZFunc;
	double fXScreen;
	double fYScreen;
	double fZScreen;
	Vector3 vA;
	Vector3 vP;
	Vector3 vN;
	double fEnd;
	float fColour;
	double fDiff;
	double fTexXOffset = 0.0;
	double fTexYOffset = 0.0;
	double fTexXScale = 1.0;
	double fTexYScale = 1.0;
	Vector3 vVertex;
	unsigned char ucColour[4];
	double fAccuracy;
	
	if (fMultiplier > 0.0) {
		fAccuracy = psFuncData->fAccuracy / fMultiplier;
	}
	else {
		fAccuracy = 0.5;
	}
	if (fAccuracy < ACCURACY_MIN) {
		fAccuracy = ACCURACY_MIN;
	}

	sscanf (psFuncData->szTexXOffset->str, "%lf", & fTexXOffset);
	sscanf (psFuncData->szTexYOffset->str, "%lf", & fTexYOffset);
	sscanf (psFuncData->szTexXScale->str, "%lf", & fTexXScale);
	sscanf (psFuncData->szTexYScale->str, "%lf", & fTexYScale);

	fEnd = 1.0 + (fAccuracy / 2.0);
	nVertex = 0;
	for (fA = 0.0; fA < fEnd; fA += fAccuracy) {

		fAFunc = (fA * 2.0 * M_PI);

		for (fP = 0.0; fP < fEnd; fP += fAccuracy) {
			fPFunc = (fP * 1.0 * M_PI);

			if (psSphericalData->psVariableA) {
				SetVariable (psSphericalData->psVariableA, fAFunc);
			}
			if (psSphericalData->psVariableP) {
				SetVariable (psSphericalData->psVariableP, fPFunc);
			}

			fRFunc = ApproximateOperation (psFuncData->psFunction);

			// Calculate the normal
			fDiff = ApproximateOperation (psSphericalData->psDiffWrtA);

			vA.fX = sin (fPFunc) * ((fDiff * cos (fAFunc)) - (fRFunc * sin (fAFunc)));
			vA.fY = sin (fPFunc) * ((fDiff * sin (fAFunc)) + (fRFunc * cos (fAFunc)));
			vA.fZ = fDiff * cos (fPFunc);

			fDiff = ApproximateOperation (psSphericalData->psDiffWrtP);

			vP.fX = cos (fAFunc) * ((fDiff * sin (fPFunc)) + (fRFunc * cos (fPFunc)));
			vP.fY = sin (fAFunc) * ((fDiff * sin (fPFunc)) + (fRFunc * cos (fPFunc)));
			vP.fZ = (fDiff * cos (fPFunc)) - (fRFunc * sin (fPFunc));

			vN = Normal (& vA, & vP);

			// Since sin(p) is zero when p is zero, we need to fix the normal
			if (fPFunc == 0.0f) {
				vN.fX = 0.0;
				vN.fY = 0.0;
				vN.fZ = -1.0;
			}

			//psFuncData->afNormals[(nVertex * 3)] = vN.fX;
			//psFuncData->afNormals[(nVertex * 3) + 1] = vN.fY;
			//psFuncData->afNormals[(nVertex * 3) + 2] = vN.fZ;

			// Calculate the cartesian position
			fXFunc = (fRFunc * cos (fAFunc) * sin (fPFunc));
			fYFunc = (fRFunc * sin (fAFunc) * sin (fPFunc));
			fZFunc = (fRFunc * cos (fPFunc));

			// Translate the function
			fXFunc += psSphericalData->fXCentre;
			fYFunc += psSphericalData->fYCentre;
			fZFunc += psSphericalData->fZCentre;

			// Scale and translate to screen coordinates
			fXScreen = ((fXFunc / psFuncData->fXWidth) * AXIS_XSIZE) - AXIS_XHSIZE;
			fYScreen = ((fYFunc / psFuncData->fYWidth) * AXIS_YSIZE) - AXIS_YHSIZE;
			fZScreen = ((fZFunc / psFuncData->fZWidth) * AXIS_ZSIZE) - AXIS_ZHSIZE;

			if (psSphericalData->psVariableR) {
				SetVariable (psSphericalData->psVariableR, fRFunc);
			}

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
			//psFuncData->afTextureCoords[(nVertex * 2)] = (fA * fTexXScale) + fTexXOffset;
			//psFuncData->afTextureCoords[(nVertex * 2) + 1] = (fP * fTexYScale) + fTexYOffset;

			//psFuncData->afVertices[(nVertex * 3)] = fXScreen;
			//psFuncData->afVertices[(nVertex * 3) + 1] = fYScreen;
			//psFuncData->afVertices[(nVertex * 3) + 2] = fZScreen;

			// Output the vertex to file
			if (boScreenCoords) {
				SetVector3 (vVertex, fXScreen, fYScreen, fZScreen);
			}
			else {
				SetVector3 (vVertex, fXFunc, fYFunc, fZFunc);
			}
			ScaleVectorDirect (& vVertex, fScale);

			vN.fX = -vN.fX;
			vN.fY = -vN.fY;
			vN.fZ = -vN.fZ;
			if (boBinary) {
				recwrite (& vVertex, sizeof (float), 3, hFile);
				if (boExportAlpha) {
					recwrite (ucColour, sizeof (unsigned char), 4, hFile);
				}
				else {
					recwrite (ucColour, sizeof (unsigned char), 3, hFile);
				}
				recwrite (& vN, sizeof (float), 3, hFile);
			}
			else {
				recprintf (hFile, "%f %f %f ", vVertex.fX, vVertex.fY, vVertex.fZ);
				if (boExportAlpha) {
					recprintf (hFile, "%u %u %u %u ", ucColour[0], ucColour[1], ucColour[2], ucColour[3]);
				}
				else {
					recprintf (hFile, "%u %u %u ", ucColour[0], ucColour[1], ucColour[2]);
				}
				recprintf (hFile, "%f %f %f\n", vN.fX, vN.fY, vN.fZ);
			}
			nVertex++;
		}
	}
	
	return nVertex;
}

int SphericalOutputStoredIndices (Recall * hFile, bool boBinary, double fMultiplier, int nOffset, FuncPersist const * psFuncData) {
	int nIndices;
	int anIndex[3];
	unsigned char uVertices;
	int nAVertices;
	int nPVertices;
	int nAVertex;
	int nPVertex;
	double fAccuracy;
	
	if (fMultiplier > 0.0) {
		fAccuracy = psFuncData->fAccuracy / fMultiplier;
	}
	else {
		fAccuracy = 0.5;
	}
	if (fAccuracy < ACCURACY_MIN) {
		fAccuracy = ACCURACY_MIN;
	}

	nAVertices = floor (1.0 / fAccuracy) + 1;
	nPVertices = floor (1.0 / fAccuracy) + 1;

	// Output index buffer identifiers
	uVertices = 3;
	nIndices = 0;
	for (nAVertex = 0; nAVertex < (nAVertices - 1); nAVertex++) {
		for (nPVertex = 0; nPVertex < nPVertices; nPVertex++) {
			anIndex[0] = ((nAVertex + 0) * nPVertices) + ((nPVertex + 0) % nPVertices) + nOffset;
			anIndex[1] = ((nAVertex + 0) * nPVertices) + ((nPVertex + 1) % nPVertices) + nOffset;
			anIndex[2] = ((nAVertex + 1) * nPVertices) + ((nPVertex + 0) % nPVertices) + nOffset;

			if (boBinary) {
				recwrite (& uVertices, sizeof (unsigned char), 1, hFile);
				recwrite (& anIndex, sizeof (int), 3, hFile);
			}
			else {
				recprintf (hFile, "3 %d %d %d\n", anIndex[0], anIndex[1], anIndex[2]);
			}
			nIndices++;

			anIndex[0] = ((nAVertex + 0) * nPVertices) + ((nPVertex + 1) % nPVertices) + nOffset;
			anIndex[1] = ((nAVertex + 1) * nPVertices) + ((nPVertex + 1) % nPVertices) + nOffset;
			anIndex[2] = ((nAVertex + 1) * nPVertices) + ((nPVertex + 0) % nPVertices) + nOffset;

			if (boBinary) {
				recwrite (& uVertices, sizeof (unsigned char), 1, hFile);
				recwrite (& anIndex, sizeof (int), 3, hFile);
			}
			else {
				recprintf (hFile, "3 %d %d %d\n", anIndex[0], anIndex[1], anIndex[2]);
			}
			nIndices++;
		}
	}

	return nIndices;
}

int SphericalCountStoredVertices (double fMultiplier, FuncPersist const * psFuncData) {
	int nVertices;
	int nAVertices;
	int nPVertices;
	double fAccuracy;
	
	if (fMultiplier > 0.0) {
		fAccuracy = psFuncData->fAccuracy / fMultiplier;
	}
	else {
		fAccuracy = 0.5;
	}
	if (fAccuracy < ACCURACY_MIN) {
		fAccuracy = ACCURACY_MIN;
	}

	nAVertices = floor (1.0 / fAccuracy) + 1;
	nPVertices = floor (1.0 / fAccuracy) + 1;

	nVertices = nAVertices * nPVertices;
	
	return nVertices;
}

int SphericalCountStoredFaces (double fMultiplier, FuncPersist const * psFuncData) {
	int nFaces;
	int nAVertices;
	int nPVertices;
	double fAccuracy;
	
	if (fMultiplier > 0.0) {
		fAccuracy = psFuncData->fAccuracy / fMultiplier;
	}
	else {
		fAccuracy = 0.5;
	}
	if (fAccuracy < ACCURACY_MIN) {
		fAccuracy = ACCURACY_MIN;
	}

	nAVertices = floor (1.0 / fAccuracy) + 1;
	nPVertices = floor (1.0 / fAccuracy) + 1;

	nFaces = (nAVertices - 1) * nPVertices * 2;
	
	return nFaces;
}

void SphericalVertex (Vector3 * pvVertex, Vector3 * pvNormal, double fAFunc, double fPFunc, bool boScreenCoords, double fMultiplier, double fScale, FuncPersist const * psFuncData) {
	SphericalPersist * psSphericalData = psFuncData->Func.psSphericalData;
	double fRFunc;
	double fXFunc;
	double fYFunc;
	double fZFunc;
	double fDiff;
	double fXScreen;
	double fYScreen;
	double fZScreen;
	Vector3 vA;
	Vector3 vP;
	Vector3 vN;
	Vector3 vVertex;

	if ((pvVertex != NULL) || (pvNormal != NULL)) {
		if (psSphericalData->psVariableA) {
			SetVariable (psSphericalData->psVariableA, fAFunc);
		}

		if (psSphericalData->psVariableP) {
			SetVariable (psSphericalData->psVariableP, fPFunc);
		}

		fRFunc = ApproximateOperation (psFuncData->psFunction);

		// Calculate the normal
		fDiff = ApproximateOperation (psSphericalData->psDiffWrtA);

		vA.fX = sin (fPFunc) * ((fDiff * cos (fAFunc)) - (fRFunc * sin (fAFunc)));
		vA.fY = sin (fPFunc) * ((fDiff * sin (fAFunc)) + (fRFunc * cos (fAFunc)));
		vA.fZ = fDiff * cos (fPFunc);

		fDiff = ApproximateOperation (psSphericalData->psDiffWrtP);

		vP.fX = cos (fAFunc) * ((fDiff * sin (fPFunc)) + (fRFunc * cos (fPFunc)));
		vP.fY = sin (fAFunc) * ((fDiff * sin (fPFunc)) + (fRFunc * cos (fPFunc)));
		vP.fZ = (fDiff * cos (fPFunc)) - (fRFunc * sin (fPFunc));

		vN = Normal (& vA, & vP);

		// Since sin(p) is zero when p is zero, we need to fix the normal
		if (fPFunc == 0.0f) {
			vN.fX = 0.0;
			vN.fY = 0.0;
			vN.fZ = -1.0;
		}

		// Calculate the cartesian position
		fXFunc = (fRFunc * cos (fAFunc) * sin (fPFunc));
		fYFunc = (fRFunc * sin (fAFunc) * sin (fPFunc));
		fZFunc = (fRFunc * cos (fPFunc));

		// Translate the function
		fXFunc += psSphericalData->fXCentre;
		fYFunc += psSphericalData->fYCentre;
		fZFunc += psSphericalData->fZCentre;

		// Scale and translate to screen coordinates
		fXScreen = (((fXFunc - psFuncData->fXMin) / psFuncData->fXWidth) * AXIS_XSIZE) - AXIS_XHSIZE;
		fYScreen = (((fYFunc - psFuncData->fYMin) / psFuncData->fYWidth) * AXIS_YSIZE) - AXIS_YHSIZE;
		fZScreen = (((fZFunc - psFuncData->fZMin) / psFuncData->fZWidth) * AXIS_ZSIZE) - AXIS_ZHSIZE;

		if (psSphericalData->psVariableR) {
			SetVariable (psSphericalData->psVariableR, fRFunc);
		}

		// Return the correct vertex position
		if (boScreenCoords) {
			SetVector3 (vVertex, fXScreen, fYScreen, fZScreen);
		}
		else {
			SetVector3 (vVertex, fXFunc, fYFunc, fZFunc);
		}
		ScaleVectorDirect (& vVertex, fScale);

		vN.fX = -vN.fX;
		vN.fY = -vN.fY;
		vN.fZ = -vN.fZ;

		if (pvVertex) {
			*pvVertex = vVertex;
		}
		if (pvNormal) {
			*pvNormal = vN;
		}
	}
}

int SphericalOutputStoredTrianglesSTL (Recall * hFile, bool boBinary, bool boScreenCoords, double fMultiplier, double fScale, FuncPersist const * psFuncData) {
	//SphericalPersist * psSphericalData = psFuncData->Func.psSphericalData;
	int nA;
	int nP;
	double fAFunc;
	double fPFunc;
	int nAVertices;
	int nPVertices;
	Vector3 avPos[4];
	int nVertex;
	Vector3 vA;
	Vector3 vP;
	Vector3 vN;
	double fAccuracy;
	float afVector[3];
	uint16_t uAttribute = 0;
	int nTriangles = 0;

	if (fMultiplier > 0.0) {
		fAccuracy = psFuncData->fAccuracy / fMultiplier;
	}
	else {
		fAccuracy = 0.5;
	}
	if (fAccuracy < ACCURACY_MIN) {
		fAccuracy = ACCURACY_MIN;
	}

	nAVertices = floor (1.0 / fAccuracy) + 1;
	nPVertices = floor (1.0 / fAccuracy) + 1;

	for (nA = 0; nA < (nAVertices - 1); nA++) {
		for (nP = 0; nP < (nPVertices - 0); nP++) {
			for (nVertex = 0; nVertex < 4; nVertex++) {
				fAFunc = (((double)((nA + (int)(nVertex / 2)) % nAVertices)) * fAccuracy) * 2.0 * M_PI;
				fPFunc = (((double)((nP + (nVertex % 2)) % nPVertices)) * fAccuracy) * 1.0 * M_PI;
				SphericalVertex (& avPos[nVertex], NULL, fAFunc, fPFunc, boScreenCoords, fMultiplier, fScale, psFuncData);
			}

			if (boBinary) {
				// Calculate the normals
				// Based on the triangles rather than the curves to avoid messy results
				vA = SubtractVectors (& avPos[1], & avPos[0]);
				vP = SubtractVectors (& avPos[2], & avPos[0]);
				vN = NormalOrUp (& vA, & vP);

				afVector[0] = vN.fX;
				afVector[1] = vN.fY;
				afVector[2] = vN.fZ;
				recwrite (afVector, sizeof(float), 3, hFile);

				afVector[0] = avPos[0].fX;
				afVector[1] = avPos[0].fY;
				afVector[2] = avPos[0].fZ;
				recwrite (afVector, sizeof(float), 3, hFile);

				afVector[0] = avPos[1].fX;
				afVector[1] = avPos[1].fY;
				afVector[2] = avPos[1].fZ;
				recwrite (afVector, sizeof(float), 3, hFile);

				afVector[0] = avPos[2].fX;
				afVector[1] = avPos[2].fY;
				afVector[2] = avPos[2].fZ;
				recwrite (afVector, sizeof(float), 3, hFile);

				recwrite (& uAttribute, sizeof(uint16_t), 1, hFile);

				// Calculate the normals
				// Based on the triangles rather than the curves to avoid messy results
				vA = SubtractVectors (& avPos[3], & avPos[1]);
				vP = SubtractVectors (& avPos[2], & avPos[1]);
				vN = NormalOrUp (& vA, & vP);

				afVector[0] = vN.fX;
				afVector[1] = vN.fY;
				afVector[2] = vN.fZ;
				recwrite (afVector, sizeof(float), 3, hFile);

				afVector[0] = avPos[1].fX;
				afVector[1] = avPos[1].fY;
				afVector[2] = avPos[1].fZ;
				recwrite (afVector, sizeof(float), 3, hFile);

				afVector[0] = avPos[3].fX;
				afVector[1] = avPos[3].fY;
				afVector[2] = avPos[3].fZ;
				recwrite (afVector, sizeof(float), 3, hFile);

				afVector[0] = avPos[2].fX;
				afVector[1] = avPos[2].fY;
				afVector[2] = avPos[2].fZ;
				recwrite (afVector, sizeof(float), 3, hFile);

				recwrite (& uAttribute, sizeof(uint16_t), 1, hFile);
				
				nTriangles += 2;
			}
			else {
				// Calculate the normals
				// Based on the triangles rather than the curves to avoid messy results
				vA = SubtractVectors (& avPos[1], & avPos[0]);
				vP = SubtractVectors (& avPos[2], & avPos[0]);
				vN = NormalOrUp (& vA, & vP);

				// Output the triangles
				recprintf (hFile, "facet normal %e %e %e\n", vN.fX, vN.fY, vN.fZ);
				recprintf (hFile, "outer loop\n");
				recprintf (hFile, "\tvertex %e %e %e\n", avPos[0].fX, avPos[0].fY, avPos[0].fZ);
				recprintf (hFile, "\tvertex %e %e %e\n", avPos[1].fX, avPos[1].fY, avPos[1].fZ);
				recprintf (hFile, "\tvertex %e %e %e\n", avPos[2].fX, avPos[2].fY, avPos[2].fZ);
				recprintf (hFile, "endloop\n");
				recprintf (hFile, "endfacet\n");

				// Calculate the normals
				// Based on the triangles rather than the curves to avoid messy results
				vA = SubtractVectors (& avPos[3], & avPos[1]);
				vP = SubtractVectors (& avPos[2], & avPos[1]);
				vN = NormalOrUp (& vA, & vP);

				recprintf (hFile, "facet normal %e %e %e\n", vN.fX, vN.fY, vN.fZ);
				recprintf (hFile, "outer loop\n");
				recprintf (hFile, "\tvertex %e %e %e\n", avPos[1].fX, avPos[1].fY, avPos[1].fZ);
				recprintf (hFile, "\tvertex %e %e %e\n", avPos[3].fX, avPos[3].fY, avPos[3].fZ);
				recprintf (hFile, "\tvertex %e %e %e\n", avPos[2].fX, avPos[2].fY, avPos[2].fZ);
				recprintf (hFile, "endloop\n");
				recprintf (hFile, "endfacet\n");
			}
		}
	}

	return nTriangles;
}

bool SphericalAssignControlVarsToFunction (FnControlPersist * psFnControlData, FuncPersist * psFuncData) {
	SphericalPersist * psSphericalData = psFuncData->Func.psSphericalData;
	bool boFound;

	boFound = AssignControlVarsToVariables (psSphericalData->psCentreVariables, psFnControlData);
	
	if (boFound) {
		SphericalRecalculateCentre (psSphericalData);
	}
	
	return boFound;
}

void SphericalOutputVoxelSlice (unsigned char * pcData, int nResolution, int nChannels, int nSlice, FuncPersist * psFuncData) {
	SphericalPersist * psSphericalData = psFuncData->Func.psSphericalData;
	int nX;
	int nY;
	int nChannel;
	double fXFunc;
	double fYFunc;
	double fZFunc;
	double fXStep;
	double fYStep;
	double fZStep;
	unsigned char ucFill;
	double fAFunc;
	double fPFunc;
	double fRFunc;
	double fDiag;
	double fAdjacent;
	double fOpposite;
	double fElevation;
	double fDistance;

	fXStep = psFuncData->fXWidth / ((double)nResolution);
	fYStep = psFuncData->fYWidth / ((double)nResolution);
	fZStep = psFuncData->fZWidth / ((double)nResolution);

	fZFunc = psFuncData->fZMin + (nSlice * fZStep);
	for (nX = 0; nX < nResolution; nX++) {
		fXFunc = psFuncData->fXMin + (nX * fXStep);
		for (nY = 0; nY < nResolution; nY++) {
			fYFunc = psFuncData->fYMin + (nY * fYStep);

			// Calculate the angles
			fOpposite = (fXFunc - psSphericalData->fXCentre);
			fAdjacent = (fYFunc - psSphericalData->fYCentre);
			fElevation = (fZFunc - psSphericalData->fZCentre);
			fAFunc = atan2 (fOpposite, fAdjacent);
			fDiag = sqrt ((fAdjacent * fAdjacent) + (fOpposite * fOpposite));
			fPFunc = atan2 (fElevation, fDiag);

			if (psSphericalData->psVariableA) {
				SetVariable (psSphericalData->psVariableA, fAFunc);
			}

			if (psSphericalData->psVariableP) {
				SetVariable (psSphericalData->psVariableP, fPFunc);
			}

			fRFunc = ApproximateOperation (psFuncData->psFunction);
			fDistance = sqrt ((fAdjacent * fAdjacent) + (fOpposite * fOpposite) + (fElevation * fElevation));

			if ((fDistance < fRFunc) && ((psFuncData->boMaterialFill == TRUE) || (fDistance > (fRFunc - psFuncData->fMaterialThickness)))) {
				ucFill = 255u;
			}
			else {
				ucFill = 0u;
			}

			for (nChannel = 0; nChannel < nChannels; nChannel++) {
				if (ucFill != 0) {
					pcData[(((nX + ((nResolution - nY - 1) * nResolution)) * nChannels) + nChannel)] = ucFill;
				}
			}
		}
	}
}


