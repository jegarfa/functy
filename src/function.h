///////////////////////////////////////////////////////////////////
// Functy
// 3D graph drawing utility
//
// David Llewellyn-Jones
// http://www.flypig.co.uk
//
// Spring 2009
///////////////////////////////////////////////////////////////////

#ifndef FUNCTION_H
#define FUNCTION_H

///////////////////////////////////////////////////////////////////
// Includes

#include "recall.h"
#include "controlvar.h"
#include "shadow.h"
#include "global.h"

///////////////////////////////////////////////////////////////////
// Defines

#define ACCURACY_NEW (0.02)
#define ACCURACY_MIN (0.00000001)
#define MATERIALTHICKNESS_NEW (0.1)

///////////////////////////////////////////////////////////////////
// Structures and enumerations

typedef struct _FuncPersist FuncPersist;

typedef enum _FUNCTYPE {
	FUNCTYPE_INVALID = -1,

	FUNCTYPE_CARTESIAN,
	FUNCTYPE_SPHERICAL,
	FUNCTYPE_CURVE,

	FUNCTYPE_NUM
} FUNCTYPE;

///////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////
// Function prototypes

FuncPersist * NewFuncPersist (FUNCTYPE eType, GlobalPersist const * psGlobalData);
void DeleteFuncPersist (FuncPersist * psFuncData);

FUNCTYPE GetFunctionType (FuncPersist * psFuncData);
void SetFunction (char const * const szFunction, FuncPersist * psFuncData);
void SetFunctionName (char const * const szName, FuncPersist * psFuncData);
void SetFunctionColours (char const * const szRed, char const * const szGreen, char const * const szBlue, char const * const szAlpha, FuncPersist * psFuncData);
void SetFunctionTextureData (TexPersist * psTexData, FuncPersist * psFuncData);
void SetFunctionUserFuncs (UserFunc * psUserFuncs, FuncPersist * psFuncData);
char const * GetFunctionName (FuncPersist * psFuncData);
char const * GetFunctionString (FuncPersist * psFuncData);
char const * GetRedString (FuncPersist * psFuncData);
char const * GetGreenString (FuncPersist * psFuncData);
char const * GetBlueString (FuncPersist * psFuncData);
char const * GetAlphaString (FuncPersist * psFuncData);
char const * GetTexFileString (FuncPersist * psFuncData);
char const * GetTexXOffsetString (FuncPersist * psFuncData);
char const * GetTexYOffsetString (FuncPersist * psFuncData);
char const * GetTexXScaleString (FuncPersist * psFuncData);
char const * GetTexYScaleString (FuncPersist * psFuncData);
void SetFunctionRange (double fXMin, double fYMin, double fZMin, double fXWidth, double fYWidth, double fZWidth, FuncPersist * psFuncData);
void GetFunctionRange (double * afRange, FuncPersist const * psFuncData);
void SetFunctionPosition (double fXMin, double fYMin, double fZMin, FuncPersist * psFuncData);
void SetFunctionAccuracy (double fAccuracy, FuncPersist * psFuncData);
double GetFunctionAccuracy (FuncPersist * psFuncData);
GLfloat * GetVertices (FuncPersist * psFuncData);
GLfloat * GetNormals (FuncPersist * psFuncData);
GLushort * GetIndices (FuncPersist * psFuncData);
GLfloat * GetColours (FuncPersist * psFuncData);
GLfloat * GetTextureCoords (FuncPersist * psFuncData);
bool GetColour (float * afGraphColour, FuncPersist * psFuncData);
void GetVertexDimensions (int * pnXVertices, int * pnYVertices, FuncPersist * psFuncData);
void PopulateVertices (FuncPersist * psFuncData);
bool GetTimeDependent (FuncPersist * psFuncData);
bool GetCentreTimeDependent (FuncPersist const * psFuncData);
void SetFunctionTime (double fTime, FuncPersist * psFuncData);
void SetFunctionTexture (GLuint uTexture, FuncPersist * psFuncData);
GLuint GetFunctionTexture (FuncPersist * psFuncData);
void SetTextureValues (char const * const szFilename, char const * const szXScale, char const * const szYScale, char const * const szXOffset, char const * const szYOffset, FuncPersist * psFuncData);
void InitShader (FuncPersist * psFuncData);
void SetFunctionShaderActive (bool boActive, FuncPersist * psFuncData);
void SetFunctionShaderShadowActive (bool boActive, FuncPersist * psFuncData);
void ActivateFunctionShader (FuncPersist * psFuncData);
void DeactivateFunctionShader (FuncPersist * psFuncData);
void ActivateFunctionShaderShadow (FuncPersist * psFuncData);
void DeactivateFunctionShaderShadow (FuncPersist * psFuncData);
void FunctionShadersRegenerate (FuncPersist * psFuncData);
void UpdateCentre (FuncPersist * psFuncData);
void SetFunctionCentre (char const * const szXCentre, char const * const szYCentre, char const * const szZCentre, FuncPersist * psFuncData);
void GetCentre (double * afCentre, FuncPersist const * psFuncData);
char const * GetXCentreString (FuncPersist * psFuncData);
char const * GetYCentreString (FuncPersist * psFuncData);
char const * GetZCentreString (FuncPersist * psFuncData);
int OutputStoredVertices (Recall * hFile, bool boBinary, bool boScreenCoords, bool boExportAlpha, double fMultiplier, double fScale, FuncPersist const * psFuncData);
int OutputStoredIndices (Recall * hFile, bool boBinary, double fMultiplier, int nOffset, FuncPersist const * psFuncData);
int CountStoredVertices (double fMultiplier, FuncPersist const * psFuncData);
int CountStoredFaces (double fMultiplier, FuncPersist const * psFuncData);
int OutputStoredTrianglesSTL (Recall * hFile, bool boBinary, bool boScreenCoords, double fMultiplier, double fScale, FuncPersist const * psFuncData);
void RecentreGraph (FuncPersist * psFuncData);
void DrawGraph (FuncPersist * psFuncData);
void DrawGraphShadow (FuncPersist * psFuncData);
void AssignControlVarsToFunction (FnControlPersist * psFnControlData, FuncPersist * psFuncData);
void AssignControlVarsToFunctionPopulate (FnControlPersist * psFnControlData, FuncPersist * psFuncData);
void SetFunctionControlVars (FnControlPersist * psFnControlData, FuncPersist * psFuncData);
void SetFunctionShadowData (ShadowPersist * psShadowData, FuncPersist * psFuncData);
void SetFunctionTransform (Matrix4 const * pmTransform, FuncPersist * psFuncData);
Matrix4 * GetFunctionTransform (FuncPersist * psFuncData);
void OutputVoxelSlice (unsigned char * pcData, int nResolution, int nChannels, int nSlice, FuncPersist * psFuncData);
void SetFunctionMaterialFill (bool boMaterialFill, FuncPersist * psFuncData);
bool GetFunctionMaterialFill (FuncPersist * psFuncData);
void SetFunctionMaterialThickness (float fMaterialThickness, FuncPersist * psFuncData);
float GetFunctionMaterialThickness (FuncPersist * psFuncData);

///////////////////////////////////////////////////////////////////
// Function definitions

#endif /* FUNCTION_H */


