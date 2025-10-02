///////////////////////////////////////////////////////////////////
// Functy
// 3D graph drawing utility
//
// David Llewellyn-Jones
// http://www.flypig.co.uk
//
// Spring 2009
///////////////////////////////////////////////////////////////////

#ifndef CURVE_H
#define CURVE_H

///////////////////////////////////////////////////////////////////
// Includes

#include "recall.h"

///////////////////////////////////////////////////////////////////
// Defines

///////////////////////////////////////////////////////////////////
// Structures and enumerations

typedef struct _CurvePersist CurvePersist;

///////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////
// Function prototypes

CurvePersist * NewCurvePersist ();
void DeleteCurvePersist (CurvePersist * psCurveData);

void CurveSetFunction (char const * const szXFunction, char const * const szYFunction, char const * const szZFunction, char const * const szRadius, FuncPersist * psFuncData);
void CurveDeriveFunctions (FuncPersist * psFuncData);
void CurveSetFunctionColours (char const * const szRed, char const * const szGreen, char const * const szBlue, char const * const szAlpha, FuncPersist * psFuncData);
void CurvePopulateVertices (FuncPersist * psFuncData);
void CurveGetVertexDimensions (int * pnAVertices, int * pnPVertices, FuncPersist * psFuncData);
void CurveGenerateVertices (FuncPersist * psFuncData);
char const * CurveGetXFunctionString (FuncPersist * psFuncData);
char const * CurveGetYFunctionString (FuncPersist * psFuncData);
char const * CurveGetZFunctionString (FuncPersist * psFuncData);
char const * CurveGetRadiusString (FuncPersist * psFuncData);
void CurveSetFunctionPosition (double fXMin, double fYMin, double fZMin, FuncPersist * psFuncData);
char * CurveGenerateVertexShader (FuncPersist * psFuncData);
char * CurveGenerateFragmentShader (FuncPersist * psFuncData);
char * CurveGenerateVertexShaderShadow (FuncPersist * psFuncData);
char * CurveGenerateFragmentShaderShadow (FuncPersist * psFuncData);
void CurveInitShader (FuncPersist * psFuncData);
void CurveSetShaderActive (bool boActive, FuncPersist * psFuncData);
void CurveSetShaderShadowActive (bool boActive, FuncPersist * psFuncData);
void CurveSetFunctionAccuracyRadius (double fAccuracy, FuncPersist * psFuncData);
double CurveGetFunctionAccuracyRadius (FuncPersist * psFuncData);
char const * CurveGetXCentreString (FuncPersist * psFuncData);
char const * CurveGetYCentreString (FuncPersist * psFuncData);
char const * CurveGetZCentreString (FuncPersist * psFuncData);
void CurveSetFunctionCentre (char const * const szXCentre, char const * const szYCentre, char const * const szZCentre, FuncPersist * psFuncData);
bool CurveGetCentreTimeDependent (FuncPersist const * psFuncData);
void CurveGetCentre (double * afCentre, FuncPersist const * psFuncData);
void CurveUpdateCentre (FuncPersist * psFuncData);
void CurveSetFunctionTime (double fTime, FuncPersist * psFuncData);
int CurveOutputStoredVertices (Recall * hFile, bool boBinary, bool boScreenCoords, bool boExportAlpha, double fMultiplier, double fScale, FuncPersist const * psFuncData);
int CurveOutputStoredIndices (Recall * hFile, bool boBinary, double fMultiplier, int nOffset, FuncPersist const * psFuncData);
int CurveCountStoredVertices (double fMultiplier, FuncPersist const * psFuncData);
int CurveCountStoredFaces (double fMultiplier, FuncPersist const * psFuncData);
int CurveOutputStoredTrianglesSTL (Recall * hFile, bool boBinary, bool boScreenCoords, double fMultiplier, double fScale, FuncPersist const * psFuncData);
bool CurveAssignControlVarsToFunction (FnControlPersist * psFnControlData, FuncPersist * psFuncData);
void CurveOutputVoxelSlice (unsigned char * pcData, int nResolution, int nChannels, int nSlice, FuncPersist * psFuncData);

///////////////////////////////////////////////////////////////////
// Function definitions

#endif /* CURVE_H */


