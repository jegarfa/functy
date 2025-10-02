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

#include "vis.h"
#include "function.h"
#include "function_private.h"
#include "textures.h"
#include "string.h"

///////////////////////////////////////////////////////////////////
// Defines

#define FUNCTION0 "1"

///////////////////////////////////////////////////////////////////
// Structures and enumerations

///////////////////////////////////////////////////////////////////
// Global variables

static GLfloat gMatDiffuse[] = { 1.0, 1.0, 1.0, 1.0 };
static GLfloat gBoardSpecular[] = { 0.7, 0.7, 0.7, 0.9 };
static GLfloat gBoardShininess[] = { 10.0 };

///////////////////////////////////////////////////////////////////
// Function prototypes

void GenerateVertices (FuncPersist * psFuncData);

///////////////////////////////////////////////////////////////////
// Function definitions

FuncPersist * NewFuncPersist (FUNCTYPE eType, GlobalPersist const * psGlobalData) {
	FuncPersist * psFuncData;

	psFuncData = g_new0 (FuncPersist, 1);

	psFuncData->psGlobalData = psGlobalData;

	psFuncData->eType = eType;
	switch (eType) {
	case FUNCTYPE_CARTESIAN:
		psFuncData->Func.psCartesianData = NewCartesianPersist ();
		break;
	case FUNCTYPE_SPHERICAL:
		psFuncData->Func.psSphericalData = NewSphericalPersist ();
		break;
	case FUNCTYPE_CURVE:
		psFuncData->Func.psCurveData = NewCurvePersist ();
		break;
	default:
		psFuncData->Func.psNone = NULL;
		break;
	}

	psFuncData->fXMin = -5.0;
	psFuncData->fYMin = -5.0;
	psFuncData->fZMin = -20.0;
	psFuncData->fXWidth = 10.0;
	psFuncData->fYWidth = 10.0;
	psFuncData->fZWidth = 40.0;

	psFuncData->fAccuracy = ACCURACY_NEW;

	MatrixSetIdentity4 (& psFuncData->mStructureTransform);
	
	psFuncData->afVertices = NULL;
	psFuncData->afNormals = NULL;
	psFuncData->auIndices = NULL;
	psFuncData->afColours = NULL;
	psFuncData->afTextureCoords = NULL;

	psFuncData->psFunction = NULL;
	psFuncData->psVariables = NULL;

	psFuncData->boColourFunction = FALSE;
	psFuncData->fRed = 0.5f;
	psFuncData->fGreen = 0.5f;
	psFuncData->fBlue = 0.5f;
	psFuncData->fAlpha = 0.5f;
	psFuncData->psRed = NULL;
	psFuncData->psGreen = NULL;
	psFuncData->psBlue = NULL;
	psFuncData->psAlpha = NULL;

	psFuncData->uTexture = TEXTURE_NONE;
	psFuncData->szTexFile = g_string_new ("");
	psFuncData->szTexXScale = g_string_new ("1.0");
	psFuncData->szTexYScale = g_string_new ("1.0");
	psFuncData->szTexXOffset = g_string_new ("0.0");
	psFuncData->szTexYOffset = g_string_new ("0.0");

	psFuncData->szName = g_string_new ("");
	psFuncData->szFunction = g_string_new ("");
	psFuncData->szRed = g_string_new ("1.0");
	psFuncData->szGreen = g_string_new ("0.5");
	psFuncData->szBlue = g_string_new ("0.5");
	psFuncData->szAlpha = g_string_new ("0.9");

	psFuncData->uTimeDependent = 0u;
	psFuncData->psShaderData = NULL;
	psFuncData->szShaderVertexSource = NULL;
	psFuncData->szShaderFragmentSource = NULL;

	psFuncData->szShaderFunction = g_string_new ("0");
	psFuncData->szShaderRed = g_string_new ("1");
	psFuncData->szShaderGreen = g_string_new ("1");
	psFuncData->szShaderBlue = g_string_new ("1");
	psFuncData->szShaderAlpha = g_string_new ("1");

	psFuncData->psShaderShadowData = NULL;
	psFuncData->szShaderShadowVertexSource = NULL;
	psFuncData->szShaderShadowFragmentSource = NULL;

	psFuncData->psFnControlData = NULL;

	InitShader (psFuncData);

	SetFunction (FUNCTION0, psFuncData);

	GenerateVertices (psFuncData);
	PopulateVertices (psFuncData);

	// Shadow data
	psFuncData->psShadowData = NULL;

	// Texture data
	psFuncData->psTexData = NULL;

	psFuncData->boMaterialFill = FALSE;
	psFuncData->fMaterialThickness = 0.1f;

	// Audio
	psFuncData->psUserFuncs = NULL;

	return psFuncData;
}

FUNCTYPE GetFunctionType (FuncPersist * psFuncData) {
	return psFuncData->eType;
}

void DeleteFuncPersist (FuncPersist * psFuncData) {
	// Free up the subtype
	switch (psFuncData->eType) {
	case FUNCTYPE_CARTESIAN:
		DeleteCartesianPersist (psFuncData->Func.psCartesianData);
		psFuncData->Func.psCartesianData = NULL;
		break;
	case FUNCTYPE_SPHERICAL:
		DeleteSphericalPersist (psFuncData->Func.psSphericalData);
		psFuncData->Func.psSphericalData = NULL;
		break;
	case FUNCTYPE_CURVE:
		DeleteCurvePersist (psFuncData->Func.psCurveData);
		psFuncData->Func.psCurveData = NULL;
		break;
	default:
		// Do absolutely nothing
		break;
	}

	// Free up the function data
	FreeRecursive (psFuncData->psFunction);

	// Free up the colour data
	FreeRecursive (psFuncData->psRed);
	FreeRecursive (psFuncData->psGreen);
	FreeRecursive (psFuncData->psBlue);

	psFuncData->psVariables = FreeVariables (psFuncData->psVariables);

	// Free up the handy UI-related data
	if (psFuncData->szName) {
		g_string_free (psFuncData->szName, TRUE);
	}
	if (psFuncData->szFunction) {
		g_string_free (psFuncData->szFunction, TRUE);
	}
	if (psFuncData->szRed) {
		g_string_free (psFuncData->szRed, TRUE);
	}
	if (psFuncData->szGreen) {
		g_string_free (psFuncData->szGreen, TRUE);
	}
	if (psFuncData->szBlue) {
		g_string_free (psFuncData->szBlue, TRUE);
	}
	if (psFuncData->szAlpha) {
		g_string_free (psFuncData->szAlpha, TRUE);
	}

	if (psFuncData->szTexFile) {
		g_string_free (psFuncData->szTexFile, TRUE);
	}
	if (psFuncData->szTexXScale) {
		g_string_free (psFuncData->szTexXScale, TRUE);
	}
	if (psFuncData->szTexYScale) {
		g_string_free (psFuncData->szTexYScale, TRUE);
	}
	if (psFuncData->szTexXOffset) {
		g_string_free (psFuncData->szTexXOffset, TRUE);
	}
	if (psFuncData->szTexYOffset) {
		g_string_free (psFuncData->szTexYOffset, TRUE);
	}

	// Free up the OpenGL vertex and normal datas
	FreeVertexBuffers (psFuncData);
	
	// Free up any texture
	if (psFuncData->uTexture != TEXTURE_NONE) {
		UnloadTexture (psFuncData->uTexture);
	}

	// Free the shader data
	if (psFuncData->psShaderData) {
		DeleteShaderPersist (psFuncData->psShaderData);
		psFuncData->psShaderData = NULL;
	}

	if (psFuncData->szShaderVertexSource) {
		g_free (psFuncData->szShaderVertexSource);
		psFuncData->szShaderVertexSource = NULL;
	}

	if (psFuncData->szShaderFragmentSource) {
		g_free (psFuncData->szShaderFragmentSource);
		psFuncData->szShaderFragmentSource = NULL;
	}

	if (psFuncData->szShaderFunction) {
		g_string_free (psFuncData->szShaderFunction, TRUE);
	}
	if (psFuncData->szShaderRed) {
		g_string_free (psFuncData->szShaderRed, TRUE);
	}
	if (psFuncData->szShaderGreen) {
		g_string_free (psFuncData->szShaderGreen, TRUE);
	}
	if (psFuncData->szShaderBlue) {
		g_string_free (psFuncData->szShaderBlue, TRUE);
	}
	if (psFuncData->szShaderAlpha) {
		g_string_free (psFuncData->szShaderAlpha, TRUE);
	}

	if (psFuncData->psShaderShadowData) {
		DeleteShaderPersist (psFuncData->psShaderShadowData);
		psFuncData->psShaderShadowData = NULL;
	}

	if (psFuncData->szShaderShadowVertexSource) {
		g_free (psFuncData->szShaderShadowVertexSource);
		psFuncData->szShaderShadowVertexSource = NULL;
	}

	if (psFuncData->szShaderShadowFragmentSource) {
		g_free (psFuncData->szShaderShadowFragmentSource);
		psFuncData->szShaderShadowFragmentSource = NULL;
	}

	// Clear the shadow data (will be freed in vis)
	psFuncData->psShadowData = NULL;

	// Clear the texture data (will be freed in vis)
	psFuncData->psTexData = NULL;

	// Clear the audio user function data (will be freed in vis)
	psFuncData->psUserFuncs = NULL;

	// Unlink the global data
	psFuncData->psGlobalData = NULL;

	g_free (psFuncData);
}

void SetFunctionTextureData (TexPersist * psTexData, FuncPersist * psFuncData) {
	psFuncData->psTexData = psTexData;
}

void SetFunctionUserFuncs (UserFunc * psUserFuncs, FuncPersist * psFuncData) {
	psFuncData->psUserFuncs = psUserFuncs;
}

void InitShader (FuncPersist * psFuncData) {
	if (psFuncData->psShaderData == NULL) {
		psFuncData->psShaderData = NewShaderPersist ();
		//SetShaderControlVars (psFuncData->psFnControlData, psFuncData->psShaderData);
	}

	if (psFuncData->psShaderShadowData == NULL) {
		psFuncData->psShaderShadowData = NewShaderPersist ();
	}

	switch (psFuncData->eType) {
	case FUNCTYPE_CARTESIAN:
		CartesianInitShader (psFuncData);
		break;
	case FUNCTYPE_SPHERICAL:
		SphericalInitShader (psFuncData);
		break;
	case FUNCTYPE_CURVE:
		CurveInitShader (psFuncData);
		break;
	default:
		// Do absolutely nothing
		break;
	}
}

void SetFunctionControlVars (FnControlPersist * psFnControlData, FuncPersist * psFuncData) {
	psFuncData->psFnControlData = psFnControlData;

	if (psFuncData->psShaderData) {
		SetShaderControlVars (psFnControlData, psFuncData->psShaderData);
	}

	if (psFuncData->psShaderShadowData) {
		SetShaderControlVars (psFnControlData, psFuncData->psShaderShadowData);
	}	
}

void LoadVertexShader (char const * const szFilename, FuncPersist * psFuncData) {
	// Free up any previously loaded shader source
	if (psFuncData->szShaderVertexSource) {
		g_free (psFuncData->szShaderVertexSource);
		psFuncData->szShaderVertexSource = NULL;
	}

	psFuncData->szShaderVertexSource = LoadShaderFile (szFilename);
}

void LoadFragmentShader (char const * const szFilename, FuncPersist * psFuncData) {
	// Free up any previously loaded shader source
	if (psFuncData->szShaderFragmentSource) {
		g_free (psFuncData->szShaderFragmentSource);
		psFuncData->szShaderFragmentSource = NULL;
	}

	psFuncData->szShaderFragmentSource = LoadShaderFile (szFilename);
}

void LoadVertexShaderShadow (char const * const szFilename, FuncPersist * psFuncData) {
	// Free up any previously loaded shadow shader source
	if (psFuncData->szShaderShadowVertexSource) {
		g_free (psFuncData->szShaderShadowVertexSource);
		psFuncData->szShaderShadowVertexSource = NULL;
	}

	psFuncData->szShaderShadowVertexSource = LoadShaderFile (szFilename);
}

void LoadFragmentShaderShadow (char const * const szFilename, FuncPersist * psFuncData) {
	// Free up any previously loaded shadow shader source
	if (psFuncData->szShaderShadowFragmentSource) {
		g_free (psFuncData->szShaderShadowFragmentSource);

		psFuncData->szShaderShadowFragmentSource = NULL;
	}

	psFuncData->szShaderShadowFragmentSource = LoadShaderFile (szFilename);
}

void ActivateFunctionShader (FuncPersist * psFuncData) {
	ActivateShader (psFuncData->psShaderData);
}

void DeactivateFunctionShader (FuncPersist * psFuncData) {
	DeactivateShader (psFuncData->psShaderData);
}

void ActivateFunctionShaderShadow (FuncPersist * psFuncData) {
	ActivateShader (psFuncData->psShaderShadowData);
}

void DeactivateFunctionShaderShadow (FuncPersist * psFuncData) {
	DeactivateShader (psFuncData->psShaderShadowData);
}

void SetFunctionShaderActive (bool boActive, FuncPersist * psFuncData) {
	switch (psFuncData->eType) {
	case FUNCTYPE_CARTESIAN:
		CartesianSetShaderActive (boActive, psFuncData);
		break;
	case FUNCTYPE_SPHERICAL:
		SphericalSetShaderActive (boActive, psFuncData);
		break;
	case FUNCTYPE_CURVE:
		CurveSetShaderActive (boActive, psFuncData);
		break;
	default:
		// Do absolutely nothing
		break;
	}
}

void SetFunctionShaderShadowActive (bool boActive, FuncPersist * psFuncData) {
	switch (psFuncData->eType) {
	case FUNCTYPE_CARTESIAN:
		CartesianSetShaderShadowActive (boActive, psFuncData);
		break;
	case FUNCTYPE_SPHERICAL:
		SphericalSetShaderShadowActive (boActive, psFuncData);
		break;
	case FUNCTYPE_CURVE:
		CurveSetShaderShadowActive (boActive, psFuncData);
		break;
	default:
		// Do absolutely nothing
		break;
	}
}

void SetFunction (char const * const szFunction, FuncPersist * psFuncData) {
	switch (psFuncData->eType) {
	case FUNCTYPE_CARTESIAN:
		CartesianSetFunction (szFunction, psFuncData);
		break;
	case FUNCTYPE_SPHERICAL:
		SphericalSetFunction (szFunction, psFuncData);
		break;
	case FUNCTYPE_CURVE:
		CurveSetFunction ("a", "0", "0", szFunction, psFuncData);
		break;
	default:
		// Do absolutely nothing
		break;
	}

	AddUndefinedControlVars (psFuncData->psVariables, psFuncData->psFnControlData);
}

void SetFunctionName (char const * const szName, FuncPersist * psFuncData) {
	// Set up the handy UI-related data
	g_string_assign (psFuncData->szName, szName);
}

void SetFunctionColours (char const * const szRed, char const * const szGreen, char const * const szBlue, char const * const szAlpha, FuncPersist * psFuncData) {
	switch (psFuncData->eType) {
	case FUNCTYPE_CARTESIAN:
		CartesianSetFunctionColours (szRed, szGreen, szBlue, szAlpha, psFuncData);
		break;
	case FUNCTYPE_SPHERICAL:
		SphericalSetFunctionColours (szRed, szGreen, szBlue, szAlpha, psFuncData);
		break;
	case FUNCTYPE_CURVE:
		CurveSetFunctionColours (szRed, szGreen, szBlue, szAlpha, psFuncData);
		break;
	default:
		// Do absolutely nothing
		break;
	}

	AddUndefinedControlVars (psFuncData->psVariables, psFuncData->psFnControlData);
}

void FunctionShadersRegenerate (FuncPersist * psFuncData) {
	char * szShader;

	szShader = NULL;
	if (psFuncData->psShaderData) {
		// Generate a vertex shader
		switch (psFuncData->eType) {
		case FUNCTYPE_CARTESIAN:
			szShader = CartesianGenerateVertexShader (psFuncData);
			break;
		case FUNCTYPE_SPHERICAL:
			szShader = SphericalGenerateVertexShader (psFuncData);
			break;
		case FUNCTYPE_CURVE:
			szShader = CurveGenerateVertexShader (psFuncData);
			break;
		default:
			// Do absolutely nothing
			break;
		}
		szShader = ShaderAddControlVars (szShader, psFuncData->psShaderData);

		ShaderRegenerateVertex (szShader, psFuncData->psShaderData);
		if (szShader) {
			//Recall * out = recopen ("temp.vs", "w");
			//recwrite (szShader, 1, strlen (szShader), out);
			//recclose (out);

			g_free (szShader);
			szShader = NULL;
		}

		// Generate a fragment shader
		switch (psFuncData->eType) {
		case FUNCTYPE_CARTESIAN:
			szShader = CartesianGenerateFragmentShader (psFuncData);
			break;
		case FUNCTYPE_SPHERICAL:
			szShader = SphericalGenerateFragmentShader (psFuncData);
			break;
		case FUNCTYPE_CURVE:
			szShader = CurveGenerateFragmentShader (psFuncData);
			break;
		default:
			// Do absolutely nothing
			break;
		}
		szShader = ShaderAddControlVars (szShader, psFuncData->psShaderData);

		ShaderRegenerateFragment (szShader, psFuncData->psShaderData);
		if (szShader) {
			//Recall * out = recopen ("temp.fs", "w");
			//recwrite (szShader, 1, strlen (szShader), out);
			//recclose (out);

			g_free (szShader);
			szShader = NULL;
		}
	}

	if (psFuncData->psShaderShadowData) {
		// Generate a vertex shader
		switch (psFuncData->eType) {
		case FUNCTYPE_CARTESIAN:
			szShader = CartesianGenerateVertexShaderShadow (psFuncData);
			break;
		case FUNCTYPE_SPHERICAL:
			szShader = SphericalGenerateVertexShaderShadow (psFuncData);
			break;
		case FUNCTYPE_CURVE:
			szShader = CurveGenerateVertexShaderShadow (psFuncData);
			break;
		default:
			// Do absolutely nothing
			break;
		}
		szShader = ShaderAddControlVars (szShader, psFuncData->psShaderShadowData);

		ShaderRegenerateVertex (szShader, psFuncData->psShaderShadowData);
		if (szShader) {
			g_free (szShader);
			szShader = NULL;
		}

		// Generate a fragment shader
		switch (psFuncData->eType) {
		case FUNCTYPE_CARTESIAN:
			szShader = CartesianGenerateFragmentShaderShadow (psFuncData);
			break;
		case FUNCTYPE_SPHERICAL:
			szShader = SphericalGenerateFragmentShaderShadow (psFuncData);
			break;
		case FUNCTYPE_CURVE:
			szShader = CurveGenerateFragmentShaderShadow (psFuncData);
			break;
		default:
			// Do absolutely nothing
			break;
		}
		szShader = ShaderAddControlVars (szShader, psFuncData->psShaderShadowData);

		ShaderRegenerateFragment (szShader, psFuncData->psShaderShadowData);
		if (szShader) {
			g_free (szShader);
			szShader = NULL;
		}
	}
}

char const * GetFunctionName (FuncPersist * psFuncData) {
	return psFuncData->szName->str;
}

char const * GetFunctionString (FuncPersist * psFuncData) {
	return psFuncData->szFunction->str;
}

char const * GetRedString (FuncPersist * psFuncData) {
	return psFuncData->szRed->str;
}

char const * GetGreenString (FuncPersist * psFuncData) {
	return psFuncData->szGreen->str;
}

char const * GetBlueString (FuncPersist * psFuncData) {
	return psFuncData->szBlue->str;
}

char const * GetAlphaString (FuncPersist * psFuncData) {
	return psFuncData->szAlpha->str;
}

char const * GetTexFileString (FuncPersist * psFuncData) {
	return psFuncData->szTexFile->str;
}

char const * GetTexXOffsetString (FuncPersist * psFuncData) {
	return psFuncData->szTexXOffset->str;
}

char const * GetTexYOffsetString (FuncPersist * psFuncData) {
	return psFuncData->szTexYOffset->str;
}

char const * GetTexXScaleString (FuncPersist * psFuncData) {
	return psFuncData->szTexXScale->str;
}

char const * GetTexYScaleString (FuncPersist * psFuncData) {
	return psFuncData->szTexYScale->str;
}

void SetFunctionRange (double fXMin, double fYMin, double fZMin, double fXWidth, double fYWidth, double fZWidth, FuncPersist * psFuncData) {
	Vector3 vScale;

	psFuncData->fXWidth = fXWidth;
	psFuncData->fYWidth = fYWidth;
	psFuncData->fZWidth = fZWidth;

	vScale.fX = fXWidth / AXIS_XSIZE;
	vScale.fY = fYWidth / AXIS_YSIZE;
	vScale.fZ = fZWidth / AXIS_ZSIZE;
	SetShaderScale (& vScale, psFuncData->psShaderData);
	SetShaderScale (& vScale, psFuncData->psShaderShadowData);

	SetFunctionPosition (fXMin, fYMin, fZMin, psFuncData);
}

void GetFunctionRange (double * afRange, FuncPersist const * psFuncData) {
	if (afRange) {
		afRange[0] = psFuncData->fXMin;
		afRange[1] = psFuncData->fYMin;
		afRange[2] = psFuncData->fZMin;
		afRange[3] = psFuncData->fXWidth;
		afRange[4] = psFuncData->fYWidth;
		afRange[5] = psFuncData->fZWidth;
	}
}

void SetFunctionPosition (double fXMin, double fYMin, double fZMin, FuncPersist * psFuncData) {
	switch (psFuncData->eType) {
	case FUNCTYPE_CARTESIAN:
		CartesianSetFunctionPosition (fXMin, fYMin, fZMin, psFuncData);
		break;
	case FUNCTYPE_SPHERICAL:
		SphericalSetFunctionPosition (fXMin, fYMin, fZMin, psFuncData);
		break;
	case FUNCTYPE_CURVE:
		CurveSetFunctionPosition (fXMin, fYMin, fZMin, psFuncData);
		break;
	default:
		// Do absolutely nothing
		break;
	}
}

void FreeVertexBuffers (FuncPersist * psFuncData) {
	// Free vertex buffer
	if (psFuncData->afVertices) {
		g_free (psFuncData->afVertices);
		psFuncData->afVertices = NULL;
	}
	// Free normal buffer
	if (psFuncData->afNormals) {
		g_free (psFuncData->afNormals);
		psFuncData->afNormals = NULL;
	}
	// Free index buffer
	if (psFuncData->auIndices) {
		g_free (psFuncData->auIndices);
		psFuncData->auIndices = NULL;
	}
	// Free colour buffer
	if (psFuncData->afColours) {
		g_free (psFuncData->afColours);
		psFuncData->afColours = NULL;
	}
	// Free texture coordinate buffer
	if (psFuncData->afTextureCoords) {
		g_free (psFuncData->afTextureCoords);
		psFuncData->afTextureCoords = NULL;
	}
}

void SetFunctionAccuracy (double fAccuracy, FuncPersist * psFuncData) {
	// Don't do anything if the accuracy hasn't changed
	if (psFuncData->fAccuracy != fAccuracy) {
		psFuncData->fAccuracy = fAccuracy;

		FreeVertexBuffers (psFuncData);
		GenerateVertices (psFuncData);
		PopulateVertices (psFuncData);
	}
}

double GetFunctionAccuracy (FuncPersist * psFuncData) {
	return psFuncData->fAccuracy;
}

GLfloat * GetVertices (FuncPersist * psFuncData) {
	return psFuncData->afVertices;
}

GLfloat * GetNormals (FuncPersist * psFuncData) {
	return psFuncData->afNormals;
}

GLushort * GetIndices (FuncPersist * psFuncData) {
	return psFuncData->auIndices;
}

GLfloat * GetColours (FuncPersist * psFuncData) {
	return psFuncData->afColours;
}

GLfloat * GetTextureCoords (FuncPersist * psFuncData) {
	return psFuncData->afTextureCoords;
}

bool GetColour (float * afGraphColour, FuncPersist * psFuncData) {
	if (!psFuncData->boColourFunction) {
		afGraphColour[0] = psFuncData->fRed;
		afGraphColour[1] = psFuncData->fGreen;
		afGraphColour[2] = psFuncData->fBlue;
		afGraphColour[3] = psFuncData->fAlpha;
	}

	return !psFuncData->boColourFunction;
}

void GetVertexDimensions (int * pnV1Vertices, int * pnV2Vertices, FuncPersist * psFuncData) {
	switch (psFuncData->eType) {
	case FUNCTYPE_CARTESIAN:
		CartesianGetVertexDimensions (pnV1Vertices, pnV2Vertices, psFuncData);
		break;
	case FUNCTYPE_SPHERICAL:
		SphericalGetVertexDimensions (pnV1Vertices, pnV2Vertices, psFuncData);
		break;
	case FUNCTYPE_CURVE:
		CurveGetVertexDimensions (pnV1Vertices, pnV2Vertices, psFuncData);
		break;
	default:
		// Do absolutely nothing
		break;
	}
}

void PopulateVertices (FuncPersist * psFuncData) {
	switch (psFuncData->eType) {
	case FUNCTYPE_CARTESIAN:
		CartesianPopulateVertices (psFuncData);
		break;
	case FUNCTYPE_SPHERICAL:
		SphericalPopulateVertices (psFuncData);
		break;
	case FUNCTYPE_CURVE:
		CurvePopulateVertices (psFuncData);
		break;
	default:
		// Do absolutely nothing
		break;
	}
}

void GenerateVertices (FuncPersist * psFuncData) {
	switch (psFuncData->eType) {
	case FUNCTYPE_CARTESIAN:
		CartesianGenerateVertices (psFuncData);
		break;
	case FUNCTYPE_SPHERICAL:
		SphericalGenerateVertices (psFuncData);
		break;
	case FUNCTYPE_CURVE:
		CurveGenerateVertices (psFuncData);
		break;
	default:
		// Do absolutely nothing
		break;
	}
}

bool GetTimeDependent (FuncPersist * psFuncData) {
	return (psFuncData->uTimeDependent != 0);
}

bool GetCentreTimeDependent (FuncPersist const * psFuncData) {
	bool boCentreTimeDependent;
	
	switch (psFuncData->eType) {
	case FUNCTYPE_SPHERICAL:
		boCentreTimeDependent = SphericalGetCentreTimeDependent (psFuncData);
		break;
	case FUNCTYPE_CURVE:
		boCentreTimeDependent = CurveGetCentreTimeDependent (psFuncData);
		break;
	default:
		boCentreTimeDependent = FALSE;
		break;
	}

	return boCentreTimeDependent;
}

void SetFunctionTime (double fTime, FuncPersist * psFuncData) {
	switch (psFuncData->eType) {
	case FUNCTYPE_SPHERICAL:
		SphericalSetFunctionTime (fTime, psFuncData);
		SphericalUpdateCentre (psFuncData);
		break;
	case FUNCTYPE_CURVE:
		CurveSetFunctionTime (fTime, psFuncData);
		CurveUpdateCentre (psFuncData);
		break;
	default:
		// Do nothing
		break;
	}

	if (psFuncData->psVariableT) {
		SetVariable (psFuncData->psVariableT, fTime);
	}

	SetShaderTime (fTime, psFuncData->psShaderData);
	SetShaderTime (fTime, psFuncData->psShaderShadowData);
}

void SetFunctionTexture (GLuint uTexture, FuncPersist * psFuncData) {
	psFuncData->uTexture = uTexture;
}

GLuint GetFunctionTexture (FuncPersist * psFuncData) {
	return psFuncData->uTexture;
}

void LoadTexture (char const * const szFilename, FuncPersist * psFuncData) {
	// Free up any texture
	if (psFuncData->uTexture != TEXTURE_NONE) {
		UnloadTexture (psFuncData->uTexture);
		psFuncData->uTexture = TEXTURE_NONE;
	}

	psFuncData->uTexture = LoadTextureRaw (szFilename, 2048, 2048, true);
}

void SetTextureValues (char const * const szFilename, char const * const szXScale, char const * const szYScale, char const * const szXOffset, char const * const szYOffset, FuncPersist * psFuncData) {
	// Load a new texture file if the filename has changed
	if (g_strcmp0 (szFilename, psFuncData->szTexFile->str) != 0) {
		g_string_assign (psFuncData->szTexFile, szFilename);
		if (psFuncData->uTexture != TEXTURE_NONE) {
			UnloadTexture (psFuncData->uTexture);
			psFuncData->uTexture = TEXTURE_NONE;
		}
		if (g_strcmp0 (psFuncData->szTexFile->str, "") != 0) {
			LoadTexture (szFilename, psFuncData);
		}
	}

	g_string_assign (psFuncData->szTexXScale, szXScale);
	g_string_assign (psFuncData->szTexYScale, szYScale);
	g_string_assign (psFuncData->szTexXOffset, szXOffset);
	g_string_assign (psFuncData->szTexYOffset, szYOffset);
}

char const * GetXCentreString (FuncPersist * psFuncData) {
	char const * szResult = NULL;

	switch (psFuncData->eType) {
	case FUNCTYPE_CARTESIAN:
		szResult = CartesianGetXCentreString (psFuncData);
		break;
	case FUNCTYPE_SPHERICAL:
		szResult = SphericalGetXCentreString (psFuncData);
		break;
	case FUNCTYPE_CURVE:
		szResult = CurveGetXCentreString (psFuncData);
		break;
	default:
		// Do absolutely nothing
		break;
	}

	return szResult;
}

char const * GetYCentreString (FuncPersist * psFuncData) {
	char const * szResult = NULL;

	switch (psFuncData->eType) {
	case FUNCTYPE_CARTESIAN:
		szResult = CartesianGetYCentreString (psFuncData);
		break;
	case FUNCTYPE_SPHERICAL:
		szResult = SphericalGetYCentreString (psFuncData);
		break;
	case FUNCTYPE_CURVE:
		szResult = CurveGetYCentreString (psFuncData);
		break;
	default:
		// Do absolutely nothing
		break;
	}

	return szResult;
}

char const * GetZCentreString (FuncPersist * psFuncData) {
	char const * szResult = NULL;

	switch (psFuncData->eType) {
	case FUNCTYPE_CARTESIAN:
		szResult = CartesianGetZCentreString (psFuncData);
		break;
	case FUNCTYPE_SPHERICAL:
		szResult = SphericalGetZCentreString (psFuncData);
		break;
	case FUNCTYPE_CURVE:
		szResult = CurveGetZCentreString (psFuncData);
		break;
	default:
		// Do absolutely nothing
		break;
	}

	return szResult;
}

void GetCentre (double * afCentre, FuncPersist const * psFuncData) {
	switch (psFuncData->eType) {
	case FUNCTYPE_CARTESIAN:
		CartesianGetCentre (afCentre, psFuncData);
		break;
	case FUNCTYPE_SPHERICAL:
		SphericalGetCentre (afCentre, psFuncData);
		break;
	case FUNCTYPE_CURVE:
		CurveGetCentre (afCentre, psFuncData);
		break;
	default:
		// Do absolutely nothing
		break;
	}
}

void UpdateCentre (FuncPersist * psFuncData) {
	switch (psFuncData->eType) {
	case FUNCTYPE_CARTESIAN:
		CartesianUpdateCentre (psFuncData);
		break;
	case FUNCTYPE_SPHERICAL:
		SphericalUpdateCentre (psFuncData);
		break;
	case FUNCTYPE_CURVE:
		CurveUpdateCentre (psFuncData);
		break;
	default:
		// Do absolutely nothing
		break;
	}
}

void SetFunctionCentre (char const * const szXCentre, char const * const szYCentre, char const * const szZCentre, FuncPersist * psFuncData) {
	switch (psFuncData->eType) {
	case FUNCTYPE_CARTESIAN:
		CartesianSetFunctionCentre (szXCentre, szYCentre, szZCentre, psFuncData);
		break;
	case FUNCTYPE_SPHERICAL:
		SphericalSetFunctionCentre (szXCentre, szYCentre, szZCentre, psFuncData);
		break;
	case FUNCTYPE_CURVE:
		CurveSetFunctionCentre (szXCentre, szYCentre, szZCentre, psFuncData);
		break;
	default:
		// Do absolutely nothing
		break;
	}
}

int OutputStoredVertices (Recall * hFile, bool boBinary, bool boScreenCoords, bool boExportAlpha, double fMultiplier, double fScale, FuncPersist const * psFuncData) {
	int nVertices;

	switch (psFuncData->eType) {
	case FUNCTYPE_CARTESIAN:
		nVertices = CartesianOutputStoredVertices (hFile, boBinary, boScreenCoords, boExportAlpha, fMultiplier, fScale, psFuncData);
		break;
	case FUNCTYPE_SPHERICAL:
		nVertices = SphericalOutputStoredVertices (hFile, boBinary, boScreenCoords, boExportAlpha, fMultiplier, fScale, psFuncData);
		break;
	case FUNCTYPE_CURVE:
		nVertices = CurveOutputStoredVertices (hFile, boBinary, boScreenCoords, boExportAlpha, fMultiplier, fScale, psFuncData);
		break;
	default:
		// No vertices
		nVertices = 0;
		break;
	}

	return nVertices;
}

int OutputStoredIndices (Recall * hFile, bool boBinary, double fMultiplier, int nOffset, FuncPersist const * psFuncData) {
	int nIndices;

	switch (psFuncData->eType) {
	case FUNCTYPE_CARTESIAN:
		nIndices = CartesianOutputStoredIndices (hFile, boBinary, fMultiplier, nOffset, psFuncData);
		break;
	case FUNCTYPE_SPHERICAL:
		nIndices = SphericalOutputStoredIndices (hFile, boBinary, fMultiplier, nOffset, psFuncData);
		break;
	case FUNCTYPE_CURVE:
		nIndices = CurveOutputStoredIndices (hFile, boBinary, fMultiplier, nOffset, psFuncData);
		break;
	default:
		// No vertices
		nIndices = 0;
		break;
	}

	return nIndices;
}

int CountStoredVertices (double fMultiplier, FuncPersist const * psFuncData) {
	int nVertices;

	switch (psFuncData->eType) {
	case FUNCTYPE_CARTESIAN:
		nVertices = CartesianCountStoredVertices (fMultiplier, psFuncData);
		break;
	case FUNCTYPE_SPHERICAL:
		nVertices = SphericalCountStoredVertices (fMultiplier, psFuncData);
		break;
	case FUNCTYPE_CURVE:
		nVertices = CurveCountStoredVertices (fMultiplier, psFuncData);
		break;
	default:
		// No vertices
		nVertices = 0;
		break;
	}

	return nVertices;
}

int CountStoredFaces (double fMultiplier, FuncPersist const * psFuncData) {
	int nFaces;

	switch (psFuncData->eType) {
	case FUNCTYPE_CARTESIAN:
		nFaces = CartesianCountStoredFaces (fMultiplier, psFuncData);
		break;
	case FUNCTYPE_SPHERICAL:
		nFaces = SphericalCountStoredFaces (fMultiplier, psFuncData);
		break;
	case FUNCTYPE_CURVE:
		nFaces = CurveCountStoredFaces (fMultiplier, psFuncData);
		break;
	default:
		// No vertices
		nFaces = 0;
		break;
	}

	return nFaces;
}

int OutputStoredTrianglesSTL (Recall * hFile, bool boBinary, bool boScreenCoords, double fMultiplier, double fScale, FuncPersist const * psFuncData) {
	int nTriangles;

	switch (psFuncData->eType) {
	case FUNCTYPE_CARTESIAN:
		nTriangles = CartesianOutputStoredTrianglesSTL (hFile, boBinary, boScreenCoords, fMultiplier, fScale, psFuncData);
		break;
	case FUNCTYPE_SPHERICAL:
		nTriangles = SphericalOutputStoredTrianglesSTL (hFile, boBinary, boScreenCoords, fMultiplier, fScale, psFuncData);
		break;
	case FUNCTYPE_CURVE:
		nTriangles = CurveOutputStoredTrianglesSTL (hFile, boBinary, boScreenCoords, fMultiplier, fScale, psFuncData);
		break;
	default:
		// No triangles
		nTriangles = 0;
		break;
	}
	
	return nTriangles;
}

void DrawGraph (FuncPersist * psFuncData) {
	int nXIndex;
	GLfloat * afVertices;
	GLfloat * afNormals;
	GLushort * auIndices;
	GLfloat * afColours;
	GLfloat * afTextureCoords;
	int nYVertices;
	int nXVertices;
	GLfloat afGraphColour[4];
	bool boColours;
	FUNCTYPE eFuncType;
	GLuint uTexture;
	//GLuint uBumpMap;
	GLuint uShaderProgram;
	GLint nVar;
	Matrix4 * pmLightTransform;
	Matrix4 mLightTranslate;
	float fShadowBias;
	float fShadowBlur;
	static GLdouble afClipEquations[4][4] = {{-1.0, 0.0, 0.0, AXIS_XHSIZE},
		{1.0, 0.0, 0.0, AXIS_XHSIZE},
		{0.0, -1.0, 0.0, AXIS_YHSIZE},
		{0.0, 1.0, 0.0, AXIS_YHSIZE}};

	eFuncType = GetFunctionType (psFuncData);

	afVertices = GetVertices (psFuncData);
	afNormals = GetNormals (psFuncData);
	auIndices = GetIndices (psFuncData);
	GetVertexDimensions (& nXVertices, & nYVertices, psFuncData);

	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, gBoardSpecular);
	glMaterialfv (GL_FRONT_AND_BACK, GL_SHININESS, gBoardShininess);
	glMaterialfv (GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, gMatDiffuse);

	glPushMatrix();

	ActivateFunctionShader (psFuncData);
	uShaderProgram = GetShaderProgram (psFuncData->psShaderData);

	if ((eFuncType == FUNCTYPE_SPHERICAL) || (eFuncType == FUNCTYPE_CURVE)) {
		// Enable the spherical clipping planes
		glClipPlane (GL_CLIP_PLANE0, afClipEquations[0]);
		glClipPlane (GL_CLIP_PLANE1, afClipEquations[1]);
		glClipPlane (GL_CLIP_PLANE2, afClipEquations[2]);
		glClipPlane (GL_CLIP_PLANE3, afClipEquations[3]);
		glEnable (GL_CLIP_PLANE0);
		glEnable (GL_CLIP_PLANE1);
		glEnable (GL_CLIP_PLANE2);
		glEnable (GL_CLIP_PLANE3);

		RecentreGraph (psFuncData);
		glMultMatrixf (psFuncData->mStructureTransform.afM);
	}

	pmLightTransform = GetLightTransform (psFuncData->psShadowData);
	MultMatrixMatrix4 (& mLightTranslate, pmLightTransform, & psFuncData->mStructureTransform);

	boColours = GetColour (afGraphColour, psFuncData);

	uTexture = GetShadowTexture (psFuncData->psShadowData);
	glActiveTexture (GL_TEXTURE2);
	glBindTexture (GL_TEXTURE_2D, uTexture);
	nVar = glGetUniformLocation (uShaderProgram, "tShadow");
	glUniform1i (nVar, 2);

	// Transfer the audio spectrum analysis texture
	uTexture = GetTexture (TEXNAME_AUDIO, NULL, psFuncData->psTexData);
	glActiveTexture (GL_TEXTURE3);
	glBindTexture (GL_TEXTURE_2D, uTexture);
	nVar = glGetUniformLocation (uShaderProgram, "tAudio");
	glUniform1i (nVar, 3);

	//uBumpMap = GetFunctionBumpTexture (psFuncData);
	uTexture = GetFunctionTexture (psFuncData);
	if (uTexture != TEXTURE_NONE) {
		glEnable (GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable (GL_TEXTURE_2D);

		glActiveTexture (GL_TEXTURE0);
		glBindTexture (GL_TEXTURE_2D, uTexture);
		nVar = glGetUniformLocation (uShaderProgram, "tTexture");
		glUniform1i (nVar, 0);

		//glActiveTexture (GL_TEXTURE1);
		//glBindTexture (GL_TEXTURE_2D, uBumpMap);
		//nVar = glGetUniformLocation (uShaderProgram, "tBumpMap");
		//glUniform1i (nVar, 1);

		glActiveTexture (GL_TEXTURE0);

		//glBindTexture (GL_TEXTURE_2D, uTexture);
		glEnableClientState (GL_TEXTURE_COORD_ARRAY);
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}
	afTextureCoords = GetTextureCoords (psFuncData);

	glEnableClientState (GL_VERTEX_ARRAY);
	glEnableClientState (GL_NORMAL_ARRAY);

	//nVar = glGetUniformLocation (uShaderProgram, "fTextureStrength");
	//glUniform1fv (nVar, 1, (GLfloat *)(& psFuncData->fTextureStrength));

	//nVar = glGetUniformLocation (uShaderProgram, "fBumpScale");
	//glUniform1fv (nVar, 1, (GLfloat *)(& psFuncData->fBumpScale));


	nVar = glGetUniformLocation (uShaderProgram, "mLightTransform");
	glUniformMatrix4fv (nVar, 1, false, (GLfloat *)(mLightTranslate.aafM));

	fShadowBias = GetShadowBias (psFuncData->psShadowData);
	nVar = glGetUniformLocation (uShaderProgram, "fShadowBias");
	glUniform1fv (nVar, 1, (GLfloat *)(& fShadowBias));

	fShadowBlur = 2.0f / (float)ShadowGetScreenWidth (psFuncData->psShadowData);
	nVar = glGetUniformLocation (uShaderProgram, "fShadowBlurX");
	glUniform1fv (nVar, 1, (GLfloat *)(& fShadowBlur));

	fShadowBlur = 2.0f / (float)ShadowGetScreenHeight (psFuncData->psShadowData);
	nVar = glGetUniformLocation (uShaderProgram, "fShadowBlurY");
	glUniform1fv (nVar, 1, (GLfloat *)(& fShadowBlur));

	if (boColours) {
		glMaterialfv (GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, afGraphColour);

		for (nXIndex = 0; nXIndex < nXVertices - 1; nXIndex++) {
			glVertexPointer (3, GL_FLOAT, 0, afVertices + (nXIndex * (nYVertices * 3)));
			glNormalPointer (GL_FLOAT, 0, afNormals + (nXIndex * (nYVertices * 3)));
			if (uTexture != TEXTURE_NONE) {
				glTexCoordPointer (2, GL_FLOAT, 0, afTextureCoords + (nXIndex * (nYVertices * 2)));
			}
			glDrawElements (GL_TRIANGLE_STRIP, nYVertices * 2, GL_UNSIGNED_SHORT, auIndices);
		}
	}
	else {
		afColours = GetColours (psFuncData);
		glEnable (GL_COLOR_MATERIAL);
		glEnableClientState (GL_COLOR_ARRAY);

		for (nXIndex = 0; nXIndex < nXVertices - 1; nXIndex++) {
			glVertexPointer (3, GL_FLOAT, 0, afVertices + (nXIndex * (nYVertices * 3)));
			glNormalPointer (GL_FLOAT, 0, afNormals + (nXIndex * (nYVertices * 3)));
			glColorPointer (4, GL_FLOAT, 0, afColours + (nXIndex * (nYVertices * 4)));
			if (uTexture != TEXTURE_NONE) {
				glTexCoordPointer (2, GL_FLOAT, 0, afTextureCoords + (nXIndex * (nYVertices * 2)));
			}
			glDrawElements (GL_TRIANGLE_STRIP, nYVertices * 2, GL_UNSIGNED_SHORT, auIndices);
		}
		glDisableClientState (GL_COLOR_ARRAY);

		glDisable (GL_COLOR_MATERIAL);
	}
	glDisableClientState (GL_VERTEX_ARRAY);
	glDisableClientState (GL_NORMAL_ARRAY);

	if (uTexture != TEXTURE_NONE) {
		glDisableClientState (GL_TEXTURE_COORD_ARRAY);
		glDisable (GL_TEXTURE_2D);
		glDisable (GL_BLEND);
	}

	if ((eFuncType == FUNCTYPE_SPHERICAL) || (eFuncType == FUNCTYPE_CURVE)) {
		// Disable the spherical clipping planes
		glDisable (GL_CLIP_PLANE0);
		glDisable (GL_CLIP_PLANE1);
		glDisable (GL_CLIP_PLANE2);
		glDisable (GL_CLIP_PLANE3);
	}

	DeactivateFunctionShader (psFuncData);

	glPopMatrix ();

	glDisable (GL_BLEND);
}

void DrawGraphShadow (FuncPersist * psFuncData) {
	int nXIndex;
	GLfloat * afVertices;
	GLushort * auIndices;
	int nYVertices;
	int nXVertices;
	FUNCTYPE eFuncType;
	GLuint uTexture;
	GLint nVar;
	GLuint uShaderProgram;
	static GLdouble afClipEquations[4][4] = {{-1.0, 0.0, 0.0, AXIS_XHSIZE},
		{1.0, 0.0, 0.0, AXIS_XHSIZE},
		{0.0, -1.0, 0.0, AXIS_YHSIZE},
		{0.0, 1.0, 0.0, AXIS_YHSIZE}};

	eFuncType = GetFunctionType (psFuncData);

	afVertices = GetVertices (psFuncData);
	auIndices = GetIndices (psFuncData);
	GetVertexDimensions (& nXVertices, & nYVertices, psFuncData);

	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glPushMatrix();

	ActivateFunctionShaderShadow (psFuncData);
	uShaderProgram = GetShaderProgram (psFuncData->psShaderShadowData);

	if ((eFuncType == FUNCTYPE_SPHERICAL) || (eFuncType == FUNCTYPE_CURVE)) {
		// Enable the spherical clipping planes
		glClipPlane (GL_CLIP_PLANE0, afClipEquations[0]);
		glClipPlane (GL_CLIP_PLANE1, afClipEquations[1]);
		glClipPlane (GL_CLIP_PLANE2, afClipEquations[2]);
		glClipPlane (GL_CLIP_PLANE3, afClipEquations[3]);
		glEnable (GL_CLIP_PLANE0);
		glEnable (GL_CLIP_PLANE1);
		glEnable (GL_CLIP_PLANE2);
		glEnable (GL_CLIP_PLANE3);

		RecentreGraph (psFuncData);
		glMultMatrixf (psFuncData->mStructureTransform.afM);
	}

	// Transfer the audio spectrum analysis texture
	uTexture = GetTexture (TEXNAME_AUDIO, NULL, psFuncData->psTexData);
	glActiveTexture (GL_TEXTURE3);
	glBindTexture (GL_TEXTURE_2D, uTexture);
	nVar = glGetUniformLocation (uShaderProgram, "tAudio");
	glUniform1i (nVar, 3);

	glEnableClientState (GL_VERTEX_ARRAY);

	for (nXIndex = 0; nXIndex < nXVertices - 1; nXIndex++) {
		glVertexPointer (3, GL_FLOAT, 0, afVertices + (nXIndex * (nYVertices * 3)));
		glDrawElements (GL_TRIANGLE_STRIP, nYVertices * 2, GL_UNSIGNED_SHORT, auIndices);
	}

	glDisableClientState (GL_VERTEX_ARRAY);

	if ((eFuncType == FUNCTYPE_SPHERICAL) || (eFuncType == FUNCTYPE_CURVE)) {
		// Disable the spherical clipping planes
		glDisable (GL_CLIP_PLANE0);
		glDisable (GL_CLIP_PLANE1);
		glDisable (GL_CLIP_PLANE2);
		glDisable (GL_CLIP_PLANE3);
	}

	DeactivateFunctionShaderShadow (psFuncData);

	glPopMatrix ();

	glDisable (GL_BLEND);
}

void RecentreGraph (FuncPersist * psFuncData) {
	GLdouble afCentre[3];
	Vector3 vTranslate;
	double afRange[6];

	GetCentre (afCentre, psFuncData);
	GetFunctionRange (afRange, psFuncData);

	vTranslate.fX = (((afCentre[0] - afRange[0]) * (2 * AXIS_XHSIZE)) / (afRange[3]));
	vTranslate.fY = (((afCentre[1] - afRange[1]) * (2 * AXIS_YHSIZE)) / (afRange[4]));
	vTranslate.fZ = (((afCentre[2] - afRange[2]) * (2 * AXIS_ZHSIZE)) / (afRange[5]));

	//glTranslatef (vTranslate.fX, vTranslate.fY, vTranslate.fZ);
	MatrixSetIdentity4 (& psFuncData->mStructureTransform);
	MatrixTranslate4 (& psFuncData->mStructureTransform, & vTranslate);
}

void AssignControlVarsToFunction (FnControlPersist * psFnControlData, FuncPersist * psFuncData) {
	AssignControlVarsToVariables (psFuncData->psVariables, psFnControlData);

	switch (psFuncData->eType) {
	case FUNCTYPE_CARTESIAN:
		CartesianAssignControlVarsToFunction (psFnControlData, psFuncData);
		break;
	case FUNCTYPE_SPHERICAL:
		SphericalAssignControlVarsToFunction (psFnControlData, psFuncData);
		break;
	case FUNCTYPE_CURVE:
		CurveAssignControlVarsToFunction (psFnControlData, psFuncData);
		break;
	default:
		// Do nothing
		break;
	}
}

void AssignControlVarsToFunctionPopulate (FnControlPersist * psFnControlData, FuncPersist * psFuncData) {
	bool boFound;

	boFound = AssignControlVarsToVariables (psFuncData->psVariables, psFnControlData);
	psFuncData->psVariables = FreeVariables (psFuncData->psVariables);

	switch (psFuncData->eType) {
	case FUNCTYPE_CARTESIAN:
		CartesianAssignControlVarsToFunction (psFnControlData, psFuncData);
		break;
	case FUNCTYPE_SPHERICAL:
		SphericalAssignControlVarsToFunction (psFnControlData, psFuncData);
		break;
	case FUNCTYPE_CURVE:
		CurveAssignControlVarsToFunction (psFnControlData, psFuncData);
		break;
	default:
		// Do nothing
		break;
	}

	if (boFound) {
		PopulateVertices (psFuncData);
	}
}

void SetFunctionShadowData (ShadowPersist * psShadowData, FuncPersist * psFuncData) {
	psFuncData->psShadowData = psShadowData;
}

void SetFunctionTransform (Matrix4 const * pmTransform, FuncPersist * psFuncData) {
	psFuncData->mStructureTransform = *pmTransform;
}

Matrix4 * GetFunctionTransform (FuncPersist * psFuncData) {
	return & psFuncData->mStructureTransform;
}

void OutputVoxelSlice (unsigned char * pcData, int nResolution, int nChannels, int nSlice, FuncPersist * psFuncData) {
	// TODO: Actually store something in the slice
	switch (psFuncData->eType) {
	case FUNCTYPE_CARTESIAN:
		CartesianOutputVoxelSlice (pcData, nResolution, nChannels, nSlice, psFuncData);
		break;
	case FUNCTYPE_SPHERICAL:
		SphericalOutputVoxelSlice (pcData, nResolution, nChannels, nSlice, psFuncData);
		break;
	case FUNCTYPE_CURVE:
		CurveOutputVoxelSlice (pcData, nResolution, nChannels, nSlice, psFuncData);
		break;
	default:
		// Do nothing
		break;
	}
}

void SetFunctionMaterialFill (bool boMaterialFill, FuncPersist * psFuncData) {
	psFuncData->boMaterialFill = boMaterialFill;
}

bool GetFunctionMaterialFill (FuncPersist * psFuncData) {
	return psFuncData->boMaterialFill;
}

void SetFunctionMaterialThickness (float fMaterialThickness, FuncPersist * psFuncData) {
	psFuncData->fMaterialThickness = fMaterialThickness;
}

float GetFunctionMaterialThickness (FuncPersist * psFuncData) {
	return psFuncData->fMaterialThickness;
}

