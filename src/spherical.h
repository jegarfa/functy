///////////////////////////////////////////////////////////////////
// Functy
// 3D graph drawing utility
//
// David Llewellyn-Jones
// http://www.flypig.co.uk
//
// Spring 2009
///////////////////////////////////////////////////////////////////

#ifndef SPHERICAL_H
#define SPHERICAL_H

///////////////////////////////////////////////////////////////////
// Includes

#include "recall.h"

///////////////////////////////////////////////////////////////////
// Defines

///////////////////////////////////////////////////////////////////
// Structures and enumerations

typedef struct _SphericalPersist SphericalPersist;

///////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////
// Function prototypes

SphericalPersist * NewSphericalPersist ();
void DeleteSphericalPersist (SphericalPersist * psSphericalData);

void SphericalSetFunction (char const * const szFunction, FuncPersist * psFuncData);
void SphericalSetFunctionColours (char const * const szRed, char const * const szGreen, char const * const szBlue, char const * const szAlpha, FuncPersist * psFuncData);
void SphericalPopulateVertices (FuncPersist * psFuncData);
void SphericalGetVertexDimensions (int * pnAVertices, int * pnPVertices, FuncPersist * psFuncData);
void SphericalGenerateVertices (FuncPersist * psFuncData);
char const * SphericalGetXCentreString (FuncPersist * psFuncData);
char const * SphericalGetYCentreString (FuncPersist * psFuncData);
char const * SphericalGetZCentreString (FuncPersist * psFuncData);
void SphericalSetFunctionCentre (char const * const szXCentre, char const * const szYCentre, char const * const szZCentre, FuncPersist * psFuncData);
bool SphericalGetCentreTimeDependent (FuncPersist const * psFuncData);
void SphericalGetCentre (double * afCentre, FuncPersist const * psFuncData);
void SphericalUpdateCentre (FuncPersist * psFuncData);
void SphericalSetFunctionTime (double fTime, FuncPersist * psFuncData);
void SphericalSetFunctionPosition (double fXMin, double fYMin, double fZMin, FuncPersist * psFuncData);
char * SphericalGenerateVertexShader (FuncPersist * psFuncData);
char * SphericalGenerateFragmentShader (FuncPersist * psFuncData);
char * SphericalGenerateVertexShaderShadow (FuncPersist * psFuncData);
char * SphericalGenerateFragmentShaderShadow (FuncPersist * psFuncData);
void SphericalInitShader (FuncPersist * psFuncData);
void SphericalSetShaderActive (bool boActive, FuncPersist * psFuncData);
void SphericalSetShaderShadowActive (bool boActive, FuncPersist * psFuncData);
int SphericalOutputStoredVertices (Recall * hFile, bool boBinary, bool boScreenCoords, bool boExportAlpha, double fMultiplier, double fScale, FuncPersist const * psFuncData);
int SphericalOutputStoredIndices (Recall * hFile, bool boBinary, double fMultiplier, int nOffset, FuncPersist const * psFuncData);
int SphericalCountStoredVertices (double fMultiplier, FuncPersist const * psFuncData);
int SphericalCountStoredFaces (double fMultiplier, FuncPersist const * psFuncData);
int SphericalOutputStoredTrianglesSTL (Recall * hFile, bool boBinary, bool boScreenCoords, double fMultiplier, double fScale, FuncPersist const * psFuncData);
bool SphericalAssignControlVarsToFunction (FnControlPersist * psFnControlData, FuncPersist * psFuncData);
void SphericalOutputVoxelSlice (unsigned char * pcData, int nResolution, int nChannels, int nSlice, FuncPersist * psFuncData);

///////////////////////////////////////////////////////////////////
// Function definitions

#endif /* SPHERICAL_H */


