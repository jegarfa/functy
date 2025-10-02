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
#include "cartesian.h"
#include "function.h"
#include "function_private.h"

#include <symbolic.h>

///////////////////////////////////////////////////////////////////
// Defines

#define FUNCTION0 "0"

///////////////////////////////////////////////////////////////////
// Structures and enumerations

struct _CartesianPersist {
	// The actual function
	Operation * psDiffWrtX;
	Operation * psDiffWrtY;
	Variable * psVariableX;
	Variable * psVariableY;
	Variable * psVariableZ;

	// Storage for the OpenGL vertex and normal data
	int nXVertices;
	int nYVertices;

	// Shader related variables
	GString * szShaderDiffWrtX;
	GString * szShaderDiffWrtY;
};

///////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////
// Function prototypes

void CartesianPopulateVerticesColour (FuncPersist * psFuncData);
void CartesianPopulateVerticesNoColour (FuncPersist * psFuncData);
void CartesianResetVertices (FuncPersist * psFuncData);
void CartesianVertex (Vector3 * pvVertex, Vector3 * pvNormal, double fXFunc, double fYFunc, bool boScreenCoords, double fMultiplier, double fScale, FuncPersist const * psFuncData);

///////////////////////////////////////////////////////////////////
// Function definitions

CartesianPersist * NewCartesianPersist () {
	CartesianPersist * psCartesianData;

	psCartesianData = g_new0 (CartesianPersist, 1);

	psCartesianData->nXVertices = 0;
	psCartesianData->nYVertices = 0;

	psCartesianData->psDiffWrtX = NULL;
	psCartesianData->psDiffWrtY = NULL;
	psCartesianData->psVariableX = NULL;
	psCartesianData->psVariableY = NULL;
	psCartesianData->psVariableZ = NULL;

	psCartesianData->szShaderDiffWrtX = g_string_new ("0");
	psCartesianData->szShaderDiffWrtY = g_string_new ("0");

	return psCartesianData;
}

void DeleteCartesianPersist (CartesianPersist * psCartesianData) {
	// Free up the function data
	FreeRecursive (psCartesianData->psDiffWrtX);
	FreeRecursive (psCartesianData->psDiffWrtY);

	if (psCartesianData->szShaderDiffWrtX) {
		g_string_free (psCartesianData->szShaderDiffWrtX, TRUE);
	}
	if (psCartesianData->szShaderDiffWrtY) {
		g_string_free (psCartesianData->szShaderDiffWrtY, TRUE);
	}

	g_free (psCartesianData);
}

void CartesianSetFunction (char const * const szFunction, FuncPersist * psFuncData) {
	CartesianPersist * psCartesianData = psFuncData->Func.psCartesianData;
	Operation * psWRT;
	char * szShaderFunction;
	int nFunctionLen;
	int nUserAssigned = 0;

	// Free up any previous function
	FreeRecursive (psFuncData->psFunction);
	FreeRecursive (psCartesianData->psDiffWrtX);
	FreeRecursive (psCartesianData->psDiffWrtY);
	psFuncData->psVariables = FreeVariables (psFuncData->psVariables);

	psFuncData->psFunction = StringToOperation (szFunction);
	nUserAssigned += AssignAllUserFuncs (psFuncData->psFunction, psFuncData->psUserFuncs);
	psFuncData->psFunction = UberSimplifyOperation (psFuncData->psFunction);

	// Differentiate with respect to x
	psWRT = CreateVariable ("x");
	psCartesianData->psDiffWrtX = DifferentiateOperation (psFuncData->psFunction, psWRT);
	FreeRecursive (psWRT);
	psCartesianData->psDiffWrtX = UberSimplifyOperation (psCartesianData->psDiffWrtX);

	// Differentiate with respect to y
	psWRT = CreateVariable ("y");
	psCartesianData->psDiffWrtY = DifferentiateOperation (psFuncData->psFunction, psWRT);
	FreeRecursive (psWRT);
	psCartesianData->psDiffWrtY = UberSimplifyOperation (psCartesianData->psDiffWrtY);

	// Set up the variables
	psFuncData->psVariables = CreateVariables (psFuncData->psFunction, psFuncData->psVariables);
	psFuncData->psVariables = CreateVariables (psCartesianData->psDiffWrtX, psFuncData->psVariables);
	psFuncData->psVariables = CreateVariables (psCartesianData->psDiffWrtY, psFuncData->psVariables);

	// Free up any unused variables
	psFuncData->psVariables = FreeVariables (psFuncData->psVariables);

	// Find the x, y, z and t variables if they exist
	psCartesianData->psVariableX = FindVariable (psFuncData->psVariables, "x");
	psCartesianData->psVariableY = FindVariable (psFuncData->psVariables, "y");
	psCartesianData->psVariableZ = FindVariable (psFuncData->psVariables, "z");
	psFuncData->psVariableT = FindVariable (psFuncData->psVariables, "t");

	// Set up the handy UI-related data
	g_string_assign (psFuncData->szFunction, szFunction);

	// Figure out if the functions are time dependent
	//psFuncData->boTimeDependent = (psFuncData->psVariableT != NULL) || (nUserAssigned > 0);
	SETBIT (psFuncData->uTimeDependent, (psFuncData->psVariableT != NULL), 0);
	SETBIT (psFuncData->uTimeDependent, (nUserAssigned > 0), 1);

	// Transfer the function details to the shader
	nFunctionLen = OperationToStringCLength (psFuncData->psFunction);
	szShaderFunction = (char *)malloc (nFunctionLen + 1);
	OperationToStringC (psFuncData->psFunction, szShaderFunction, nFunctionLen + 1);
	g_string_assign (psFuncData->szShaderFunction, szShaderFunction);
	free (szShaderFunction);

	nFunctionLen = OperationToStringCLength (psCartesianData->psDiffWrtX);
	szShaderFunction = (char *)malloc (nFunctionLen + 1);
	OperationToStringC (psCartesianData->psDiffWrtX, szShaderFunction, nFunctionLen + 1);
	g_string_assign (psCartesianData->szShaderDiffWrtX, szShaderFunction);
	free (szShaderFunction);

	nFunctionLen = OperationToStringCLength (psCartesianData->psDiffWrtY);
	szShaderFunction = (char *)malloc (nFunctionLen + 1);
	OperationToStringC (psCartesianData->psDiffWrtY, szShaderFunction, nFunctionLen + 1);
	g_string_assign (psCartesianData->szShaderDiffWrtY, szShaderFunction);
	free (szShaderFunction);
}

void CartesianSetFunctionColours (char const * const szRed, char const * const szGreen, char const * const szBlue, char const * const szAlpha, FuncPersist * psFuncData) {
	CartesianPersist * psCartesianData = psFuncData->Func.psCartesianData;
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

	// Find the x, y, z and t variables if they exist
	psCartesianData->psVariableX = FindVariable (psFuncData->psVariables, "x");
	psCartesianData->psVariableY = FindVariable (psFuncData->psVariables, "y");
	psCartesianData->psVariableZ = FindVariable (psFuncData->psVariables, "z");
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

void CartesianGetVertexDimensions (int * pnXVertices, int * pnYVertices, FuncPersist * psFuncData) {
	CartesianPersist * psCartesianData = psFuncData->Func.psCartesianData;

	if (pnXVertices) {
		* pnXVertices = psCartesianData->nXVertices;
	}
	if (pnYVertices) {
		* pnYVertices = psCartesianData->nXVertices;
	}
}

void CartesianPopulateVertices (FuncPersist * psFuncData) {
	bool boUseShader;

	boUseShader = GetShaderActive (psFuncData->psShaderData);

	if (boUseShader) {
		// Reset the vertices for the shader
		// Strictly speaking this isn't necessary for the cartesian case
		// but it may prevent peculiar problems happening later 
		CartesianResetVertices (psFuncData);
	}
	else{
		if (psFuncData->boColourFunction) {
			CartesianPopulateVerticesColour (psFuncData);
		}
		else {
			CartesianPopulateVerticesNoColour (psFuncData);
		}
	}
}

void CartesianPopulateVerticesNoColour (FuncPersist * psFuncData) {
	CartesianPersist * psCartesianData = psFuncData->Func.psCartesianData;
	int nVertex;
	double fX;
	double fY;
	double fXFunc;
	double fYFunc;
	double fZFunc;
	double fZScreen;
	//double x;
	//double y;
	Vector3 vX;
	Vector3 vY;
	Vector3 vN;
	double fEnd;
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
	for (fX = 0.0; fX < fEnd; fX += psFuncData->fAccuracy) {

		fXFunc = (fX * psFuncData->fXWidth) + psFuncData->fXMin;

		for (fY = 0.0; fY < fEnd; fY += psFuncData->fAccuracy) {
			fYFunc = (fY * psFuncData->fYWidth) + psFuncData->fYMin;

			//x = fXFunc;
			//y = fYFunc;

			if (psCartesianData->psVariableX) {
				SetVariable (psCartesianData->psVariableX, fXFunc);
			}
			if (psCartesianData->psVariableY) {
				SetVariable (psCartesianData->psVariableY, fYFunc);
			}

			fZFunc = ApproximateOperation (psFuncData->psFunction);

			vX.fX = 1.0f;
			vX.fY = 0.0f;
			vX.fZ = ApproximateOperation (psCartesianData->psDiffWrtX);

			vY.fX = 0.0f;
			vY.fY = 1.0f;
			vY.fZ = ApproximateOperation (psCartesianData->psDiffWrtY);

			vN = Normal (& vX, & vY);

			psFuncData->afNormals[(nVertex * 3)] = vN.fX;
			psFuncData->afNormals[(nVertex * 3) + 1] = vN.fY;
			psFuncData->afNormals[(nVertex * 3) + 2] = vN.fZ;

			fZScreen = (((fZFunc - psFuncData->fZMin) / psFuncData->fZWidth) * AXIS_ZSIZE) - AXIS_ZHSIZE;

			// Calculate vertex texture coordinate
			psFuncData->afTextureCoords[(nVertex * 2)] = (fXFunc * fTexXScale) + fTexXOffset;
			psFuncData->afTextureCoords[(nVertex * 2) + 1] = (fYFunc * fTexYScale) + fTexYOffset;

			psFuncData->afVertices[(nVertex * 3) + 2] = fZScreen;

			nVertex++;
		}
	}
}

void CartesianPopulateVerticesColour (FuncPersist * psFuncData) {
	CartesianPersist * psCartesianData = psFuncData->Func.psCartesianData;
	int nVertex;
	double fX;
	double fY;
	double fXFunc;
	double fYFunc;
	double fZFunc;
	double fZScreen;
	//double x;
	//double y;
	Vector3 vX;
	Vector3 vY;
	Vector3 vN;
	double fEnd;
	float fColour;
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
	for (fX = 0.0; fX < fEnd; fX += psFuncData->fAccuracy) {

		fXFunc = (fX * psFuncData->fXWidth) + psFuncData->fXMin;

		for (fY = 0.0; fY < fEnd; fY += psFuncData->fAccuracy) {
			fYFunc = (fY * psFuncData->fYWidth) + psFuncData->fYMin;

			//x = fXFunc;
			//y = fYFunc;

			if (psCartesianData->psVariableX) {
				SetVariable (psCartesianData->psVariableX, fXFunc);
			}
			if (psCartesianData->psVariableY) {
				SetVariable (psCartesianData->psVariableY, fYFunc);
			}

			fZFunc = ApproximateOperation (psFuncData->psFunction);

			vX.fX = 1.0f;
			vX.fY = 0.0f;
			vX.fZ = ApproximateOperation (psCartesianData->psDiffWrtX);

			vY.fX = 0.0f;
			vY.fY = 1.0f;
			vY.fZ = ApproximateOperation (psCartesianData->psDiffWrtY);

			vN = Normal (& vX, & vY);

			psFuncData->afNormals[(nVertex * 3)] = vN.fX;
			psFuncData->afNormals[(nVertex * 3) + 1] = vN.fY;
			psFuncData->afNormals[(nVertex * 3) + 2] = vN.fZ;

			fZScreen = (((fZFunc - psFuncData->fZMin) / psFuncData->fZWidth) * AXIS_ZSIZE) - AXIS_ZHSIZE;

			if (psCartesianData->psVariableZ) {
				SetVariable (psCartesianData->psVariableZ, fZFunc);
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
			psFuncData->afTextureCoords[(nVertex * 2)] = (fXFunc * fTexXScale) + fTexXOffset;
			psFuncData->afTextureCoords[(nVertex * 2) + 1] = (fYFunc * fTexYScale) + fTexYOffset;

			psFuncData->afVertices[(nVertex * 3) + 2] = fZScreen;

			nVertex++;
		}
	}
}

void CartesianGenerateVertices (FuncPersist * psFuncData) {
	CartesianPersist * psCartesianData = psFuncData->Func.psCartesianData;
	int nXVertices;
	int nYVertices;
	int nIndex;

	nXVertices = floor (1.0 / psFuncData->fAccuracy) + 1;
	nYVertices = floor (1.0 / psFuncData->fAccuracy) + 1;

	psCartesianData->nXVertices = nXVertices;
	psCartesianData->nYVertices = nYVertices;

	psFuncData->afVertices = g_new0 (GLfloat, nXVertices * nYVertices * 3);
	psFuncData->afNormals = g_new0 (GLfloat, nXVertices * nYVertices * 3);
	psFuncData->auIndices = g_new (GLushort, nYVertices * 2);
	psFuncData->afColours = g_new0 (GLfloat, nXVertices * nYVertices * 4);
	psFuncData->afTextureCoords = g_new0 (GLfloat, nXVertices * nYVertices * 2);

	// Generate the index data
	for (nIndex = 0; nIndex < nYVertices; nIndex++) {
		psFuncData->auIndices[(nIndex * 2)] = nIndex;
		psFuncData->auIndices[(nIndex * 2) + 1] = nIndex + nYVertices;
	}

	CartesianResetVertices (psFuncData);
}

void CartesianResetVertices (FuncPersist * psFuncData) {
	int nVertex;
	double fX;
	double fY;
	double fXScreen;
	double fYScreen;
	double fEnd;

	// Generate the x,y vertex data (z values are populated dynamically)
	fEnd = 1.0 + (psFuncData->fAccuracy / 2.0);
	nVertex = 0;
	for (fX = 0.0; fX < fEnd; fX += psFuncData->fAccuracy) {
		fXScreen = (fX * AXIS_XSIZE) - AXIS_XHSIZE;

		for (fY = 0.0; fY < fEnd; fY += psFuncData->fAccuracy) {
			fYScreen = (fY * AXIS_YSIZE) - AXIS_YHSIZE;

			psFuncData->afNormals[(nVertex * 3)] = 0.0;
			psFuncData->afNormals[(nVertex * 3) + 1] = 0.0;
			psFuncData->afNormals[(nVertex * 3) + 2] = 1.0;

			psFuncData->afVertices[(nVertex * 3)] = fXScreen;
			psFuncData->afVertices[(nVertex * 3) + 1] = fYScreen;
			psFuncData->afVertices[(nVertex * 3) + 2] = 0.0;

			nVertex++;
		}
	}
}

void CartesianSetFunctionPosition (double fXMin, double fYMin, double fZMin, FuncPersist * psFuncData) {
	Vector3 vPosition;

	psFuncData->fXMin = fXMin;
	psFuncData->fYMin = fYMin;
	psFuncData->fZMin = fZMin;

	vPosition.fX = fXMin + ((AXIS_XHSIZE * psFuncData->fXWidth) / AXIS_XSIZE);
	vPosition.fY = fYMin + ((AXIS_YHSIZE * psFuncData->fYWidth) / AXIS_YSIZE);
	vPosition.fZ = fZMin + ((AXIS_ZHSIZE * psFuncData->fZWidth) / AXIS_ZSIZE);
	SetShaderPosition (& vPosition, psFuncData->psShaderData);
	SetShaderPosition (& vPosition, psFuncData->psShaderShadowData);

	PopulateVertices (psFuncData);
}

char * CartesianGenerateVertexShader (FuncPersist * psFuncData) {
	char * szShader;
	CartesianPersist * psCartesianData = psFuncData->Func.psCartesianData;

	if (psFuncData->szShaderVertexSource) {
		szShader = ReplaceTextCopy (psFuncData->szShaderVertexSource, "function", psFuncData->szShaderFunction->str);
		szShader = ReplaceTextMove (szShader, "diffX", psCartesianData->szShaderDiffWrtX->str);
		szShader = ReplaceTextMove (szShader, "diffY", psCartesianData->szShaderDiffWrtY->str);
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

char * CartesianGenerateFragmentShader (FuncPersist * psFuncData) {
	char * szShader;
	CartesianPersist * psCartesianData = psFuncData->Func.psCartesianData;

	if (psFuncData->szShaderFragmentSource) {
		szShader = ReplaceTextCopy (psFuncData->szShaderFragmentSource, "function", psFuncData->szShaderFunction->str);
		szShader = ReplaceTextMove (szShader, "diffX", psCartesianData->szShaderDiffWrtX->str);
		szShader = ReplaceTextMove (szShader, "diffY", psCartesianData->szShaderDiffWrtY->str);
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

char * CartesianGenerateVertexShaderShadow (FuncPersist * psFuncData) {
	char * szShader;
	//CartesianPersist * psCartesianData = psFuncData->Func.psCartesianData;

	if (psFuncData->szShaderShadowVertexSource) {
		szShader = ReplaceTextCopy (psFuncData->szShaderShadowVertexSource, "function", psFuncData->szShaderFunction->str);
	}
	else {
		szShader = NULL;
	}

	return szShader;
}

char * CartesianGenerateFragmentShaderShadow (FuncPersist * psFuncData) {
	char * szShader;
	//CartesianPersist * psCartesianData = psFuncData->Func.psCartesianData;

	if (psFuncData->szShaderShadowFragmentSource) {
		//szShader = CopyText (psFuncData->szShaderShadowFragmentSource);
		szShader = ReplaceTextCopy (psFuncData->szShaderShadowFragmentSource, "alpha", psFuncData->szShaderAlpha->str);
	}
	else {
		szShader = NULL;
	}

	return szShader;
}

void CartesianInitShader (FuncPersist * psFuncData) {
	GString * szPath;

	szPath = g_string_new ("/shaders/cartesian.vs");
	GenerateDataPath (szPath, psFuncData->psGlobalData);
	LoadVertexShader (szPath->str, psFuncData);

	szPath = g_string_assign (szPath, "/shaders/cartesian.fs");
	GenerateDataPath (szPath, psFuncData->psGlobalData);
	LoadFragmentShader (szPath->str, psFuncData);

	szPath = g_string_assign (szPath, "/shaders/cartesian-shadow.vs");
	GenerateDataPath (szPath, psFuncData->psGlobalData);
	LoadVertexShaderShadow (szPath->str, psFuncData);

	szPath = g_string_assign (szPath, "/shaders/cartesian-shadow.fs");
	GenerateDataPath (szPath, psFuncData->psGlobalData);
	LoadFragmentShaderShadow (szPath->str, psFuncData);

	g_string_free (szPath, TRUE);
	FunctionShadersRegenerate (psFuncData);
}

void CartesianSetShaderActive (bool boActive, FuncPersist * psFuncData) {
	SetShaderActive (boActive, psFuncData->psShaderData);

	CartesianPopulateVertices (psFuncData);
}

void CartesianSetShaderShadowActive (bool boActive, FuncPersist * psFuncData) {
	SetShaderActive (boActive, psFuncData->psShaderShadowData);

	// TODO: Check whether we actually need to do this
	CartesianPopulateVertices (psFuncData);
}

void CartesianUpdateCentre (FuncPersist * psFuncData) {
	// Do nothing
}

void CartesianSetFunctionCentre (char const * const szXCentre, char const * const szYCentre, char const * const szZCentre, FuncPersist * psFuncData) {
	// Do nothing
}

void CartesianGetCentre (double * afCentre, FuncPersist const * psFuncData) {
	afCentre[0] = 0.0f;
	afCentre[1] = 0.0f;
	afCentre[2] = 0.0f;
}

char const * CartesianGetXCentreString (FuncPersist * psFuncData) {
	return "0";
}

char const * CartesianGetYCentreString (FuncPersist * psFuncData) {
	return "0";
}

char const * CartesianGetZCentreString (FuncPersist * psFuncData) {
	return "0";
}

int CartesianOutputStoredVertices (Recall * hFile, bool boBinary, bool boExportAlpha, bool boScreenCoords, double fMultiplier, double fScale, FuncPersist const * psFuncData) {
	CartesianPersist * psCartesianData = psFuncData->Func.psCartesianData;
	int nVertex;
	double fX;
	double fY;
	double fXFunc;
	double fYFunc;
	double fZFunc;
	double fXScreen;
	double fYScreen;
	double fZScreen;
	//double x;
	//double y;
	Vector3 vX;
	Vector3 vY;
	Vector3 vN;
	double fEnd;
	float fColour;
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
		fAccuracy = 1.0;
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
	for (fX = 0.0; fX < fEnd; fX += fAccuracy) {

		fXFunc = (fX * psFuncData->fXWidth) + psFuncData->fXMin;

		for (fY = 0.0; fY < fEnd; fY += fAccuracy) {
			fYFunc = (fY * psFuncData->fYWidth) + psFuncData->fYMin;

			//x = fXFunc;
			//y = fYFunc;

			if (psCartesianData->psVariableX) {
				SetVariable (psCartesianData->psVariableX, fXFunc);
			}
			if (psCartesianData->psVariableY) {
				SetVariable (psCartesianData->psVariableY, fYFunc);
			}

			fZFunc = ApproximateOperation (psFuncData->psFunction);

			vX.fX = 1.0f;
			vX.fY = 0.0f;
			vX.fZ = ApproximateOperation (psCartesianData->psDiffWrtX);

			vY.fX = 0.0f;
			vY.fY = 1.0f;
			vY.fZ = ApproximateOperation (psCartesianData->psDiffWrtY);

			vN = Normal (& vX, & vY);

			fXScreen = (fX * AXIS_XSIZE) - AXIS_XHSIZE;
			fYScreen = (fY * AXIS_YSIZE) - AXIS_YHSIZE;
			fZScreen = (((fZFunc - psFuncData->fZMin) / psFuncData->fZWidth) * AXIS_ZSIZE) - AXIS_ZHSIZE;

			if (psCartesianData->psVariableZ) {
				SetVariable (psCartesianData->psVariableZ, fZFunc);
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
			//psFuncData->afTextureCoords[(nVertex * 2)] = (fXFunc * fTexXScale) + fTexXOffset;
			//psFuncData->afTextureCoords[(nVertex * 2) + 1] = (fYFunc * fTexYScale) + fTexYOffset;

			//psFuncData->afVertices[(nVertex * 3) + 2] = fZScreen;

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

int CartesianOutputStoredIndices (Recall * hFile, bool boBinary, double fMultiplier, int nOffset, FuncPersist const * psFuncData) {
	int nIndices;
	int anIndex[3];
	unsigned char uVertices;
	int nXVertex;
	int nYVertex;
	int nXVertices;
	int nYVertices;
	double fAccuracy;
	
	if (fMultiplier > 0.0) {
		fAccuracy = psFuncData->fAccuracy / fMultiplier;
	}
	else {
		fAccuracy = 1.0;
	}
	if (fAccuracy < ACCURACY_MIN) {
		fAccuracy = ACCURACY_MIN;
	}

	nXVertices = floor (1.0 / fAccuracy) + 1;
	nYVertices = floor (1.0 / fAccuracy) + 1;

	// Output index buffer identifiers
	uVertices = 3;
	nIndices = 0;
	for (nXVertex = 0; nXVertex < (nXVertices - 1); nXVertex++) {
		for (nYVertex = 0; nYVertex < (nYVertices - 1); nYVertex++) {
			anIndex[0] = ((nXVertex + 0) * nYVertices) + ((nYVertex + 0) % nYVertices) + nOffset;
			anIndex[1] = ((nXVertex + 1) * nYVertices) + ((nYVertex + 0) % nYVertices) + nOffset;
			anIndex[2] = ((nXVertex + 0) * nYVertices) + ((nYVertex + 1) % nYVertices) + nOffset;

			if (boBinary) {
				recwrite (& uVertices, sizeof (unsigned char), 1, hFile);
				recwrite (& anIndex, sizeof (int), 3, hFile);
			}
			else {
				recprintf (hFile, "3 %d %d %d\n", anIndex[0], anIndex[1], anIndex[2]);
			}
			nIndices++;

			anIndex[0] = ((nXVertex + 0) * nYVertices) + ((nYVertex + 1) % nYVertices) + nOffset;
			anIndex[1] = ((nXVertex + 1) * nYVertices) + ((nYVertex + 0) % nYVertices) + nOffset;
			anIndex[2] = ((nXVertex + 1) * nYVertices) + ((nYVertex + 1) % nYVertices) + nOffset;

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

int CartesianCountStoredVertices (double fMultiplier, FuncPersist const * psFuncData) {
	int nVertices;
	int nXVertices;
	int nYVertices;
	double fAccuracy;
	
	if (fMultiplier > 0.0) {
		fAccuracy = psFuncData->fAccuracy / fMultiplier;
	}
	else {
		fAccuracy = 1.0;
	}
	if (fAccuracy < ACCURACY_MIN) {
		fAccuracy = ACCURACY_MIN;
	}

	nXVertices = floor (1.0 / fAccuracy) + 1;
	nYVertices = floor (1.0 / fAccuracy) + 1;
	
	nVertices = nXVertices * nYVertices;
	
	return nVertices;
}

int CartesianCountStoredFaces (double fMultiplier, FuncPersist const * psFuncData) {
	int nFaces;
	int nXVertices;
	int nYVertices;
	double fAccuracy;
	
	if (fMultiplier > 0.0) {
		fAccuracy = psFuncData->fAccuracy / fMultiplier;
	}
	else {
		fAccuracy = 1.0;
	}
	if (fAccuracy < ACCURACY_MIN) {
		fAccuracy = ACCURACY_MIN;
	}

	nXVertices = floor (1.0 / fAccuracy) + 1;
	nYVertices = floor (1.0 / fAccuracy) + 1;
	
	nFaces = (nXVertices - 1) * (nYVertices - 1) * 2;

	return nFaces;
}

void CartesianVertex (Vector3 * pvVertex, Vector3 * pvNormal, double fXFunc, double fYFunc, bool boScreenCoords, double fMultiplier, double fScale, FuncPersist const * psFuncData) {
	CartesianPersist * psCartesianData = psFuncData->Func.psCartesianData;
	double fZFunc;
	double fXScreen;
	double fYScreen;
	double fZScreen;
	Vector3 vX;
	Vector3 vY;
	Vector3 vN;
	Vector3 vVertex;

	if ((pvVertex != NULL) || (pvNormal != NULL)) {
		if (psCartesianData->psVariableX) {
			SetVariable (psCartesianData->psVariableX, fXFunc);
		}
		if (psCartesianData->psVariableY) {
			SetVariable (psCartesianData->psVariableY, fYFunc);
		}

		fZFunc = ApproximateOperation (psFuncData->psFunction);

		vX.fX = 1.0f;
		vX.fY = 0.0f;
		vX.fZ = ApproximateOperation (psCartesianData->psDiffWrtX);

		vY.fX = 0.0f;
		vY.fY = 1.0f;
		vY.fZ = ApproximateOperation (psCartesianData->psDiffWrtY);

		vN = Normal (& vX, & vY);

		fXScreen = (((fXFunc - psFuncData->fXMin) / psFuncData->fXWidth) * AXIS_XSIZE) - AXIS_XHSIZE;
		fYScreen = (((fYFunc - psFuncData->fYMin) / psFuncData->fYWidth) * AXIS_YSIZE) - AXIS_YHSIZE;
		fZScreen = (((fZFunc - psFuncData->fZMin) / psFuncData->fZWidth) * AXIS_ZSIZE) - AXIS_ZHSIZE;

		if (psCartesianData->psVariableZ) {
			SetVariable (psCartesianData->psVariableZ, fZFunc);
		}

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
			*pvNormal = vN;
		}
	}
}

int CartesianOutputStoredTrianglesSTL (Recall * hFile, bool boBinary, bool boScreenCoords, double fMultiplier, double fScale, FuncPersist const * psFuncData) {
	//CartesianPersist * psCartesianData = psFuncData->Func.psCartesianData;
	int nX;
	int nY;
	double fXFunc;
	double fYFunc;
	int nXVertices;
	int nYVertices;
	Vector3 avPos[4];
	int nVertex;
	Vector3 vX;
	Vector3 vY;
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

	nXVertices = floor (1.0 / fAccuracy) + 1;
	nYVertices = floor (1.0 / fAccuracy) + 1;

	for (nX = 0; nX < (nXVertices - 1); nX++) {
		for (nY = 0; nY < (nYVertices - 1); nY++) {
			for (nVertex = 0; nVertex < 4; nVertex++) {
				fXFunc = ((((double)((nX + (int)(nVertex / 2)) % nXVertices)) * fAccuracy) * psFuncData->fXWidth) + psFuncData->fXMin;
				fYFunc = ((((double)((nY + (nVertex % 2)) % nYVertices)) * fAccuracy) * psFuncData->fYWidth) + psFuncData->fYMin;
				CartesianVertex (& avPos[nVertex], NULL, fXFunc, fYFunc, boScreenCoords, fMultiplier, fScale, psFuncData);
			}

			if (boBinary) {
				// Calculate the normals
				// Based on the triangles rather than the curves to avoid messy results
				vX = SubtractVectors (& avPos[1], & avPos[0]);
				vY = SubtractVectors (& avPos[2], & avPos[0]);
				vN = NormalOrUp (& vX, & vY);

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
				vX = SubtractVectors (& avPos[3], & avPos[1]);
				vY = SubtractVectors (& avPos[2], & avPos[1]);
				vN = NormalOrUp (& vX, & vY);

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
				vX = SubtractVectors (& avPos[1], & avPos[0]);
				vY = SubtractVectors (& avPos[2], & avPos[0]);
				vN = NormalOrUp (& vX, & vY);

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
				vX = SubtractVectors (& avPos[3], & avPos[1]);
				vY = SubtractVectors (& avPos[2], & avPos[1]);
				vN = NormalOrUp (& vX, & vY);

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

bool CartesianAssignControlVarsToFunction (FnControlPersist * psFnControlData, FuncPersist * psFuncData) {
	// Do nothing	
	return FALSE;
}

void CartesianOutputVoxelSlice (unsigned char * pcData, int nResolution, int nChannels, int nSlice, FuncPersist * psFuncData) {
	CartesianPersist * psCartesianData = psFuncData->Func.psCartesianData;
	int nX;
	int nY;
	int nChannel;
	double fXFunc;
	double fYFunc;
	double fZFunc;
	double fXStep;
	double fYStep;
	double fZStep;
	double fZSlice;
	unsigned char ucFill;

	fXStep = psFuncData->fXWidth / ((double)nResolution);
	fYStep = psFuncData->fYWidth / ((double)nResolution);
	fZStep = psFuncData->fZWidth / ((double)nResolution);

	fZSlice = psFuncData->fZMin + (nSlice * fZStep);

	for (nX = 0; nX < nResolution; nX++) {
		fXFunc = psFuncData->fXMin + (nX * fXStep);
		for (nY = 0; nY < nResolution; nY++) {
			fYFunc = psFuncData->fYMin + (nY * fYStep);

			if (psCartesianData->psVariableX) {
				SetVariable (psCartesianData->psVariableX, fXFunc);
			}
			if (psCartesianData->psVariableY) {
				SetVariable (psCartesianData->psVariableY, fYFunc);
			}

			fZFunc = ApproximateOperation (psFuncData->psFunction);

			if ((fZSlice < fZFunc) && ((psFuncData->boMaterialFill == TRUE) || (fZSlice > (fZFunc - psFuncData->fMaterialThickness)))) {
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



