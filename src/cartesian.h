///////////////////////////////////////////////////////////////////
// Functy
// 3D graph drawing utility
//
// David Llewellyn-Jones
// http://www.flypig.co.uk
//
// Spring 2009
///////////////////////////////////////////////////////////////////

#ifndef CARTESIAN_H
#define CARTESIAN_H

///////////////////////////////////////////////////////////////////
// Includes

#include "recall.h"

///////////////////////////////////////////////////////////////////
// Defines

///////////////////////////////////////////////////////////////////
// Structures and enumerations

typedef struct _CartesianPersist CartesianPersist;

///////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////
// Function prototypes

CartesianPersist * NewCartesianPersist ();
void DeleteCartesianPersist (CartesianPersist * psCartesianData);

void CartesianSetFunction (char const * const szFunction, FuncPersist * psFuncData);
void CartesianSetFunctionColours (char const * const szRed, char const * const szGreen, char const * const szBlue, char const * const szAlpha, FuncPersist * psFuncData);
void CartesianPopulateVertices (FuncPersist * psFuncData);
void CartesianGetVertexDimensions (int * pnXVertices, int * pnYVertices, FuncPersist * psFuncData);
void CartesianGenerateVertices (FuncPersist * psFuncData);
void CartesianSetFunctionPosition (double fXMin, double fYMin, double fZMin, FuncPersist * psFuncData);
char * CartesianGenerateVertexShader (FuncPersist * psFuncData);
char * CartesianGenerateFragmentShader (FuncPersist * psFuncData);
char * CartesianGenerateVertexShaderShadow (FuncPersist * psFuncData);
char * CartesianGenerateFragmentShaderShadow (FuncPersist * psFuncData);
void CartesianInitShader (FuncPersist * psFuncData);
void CartesianSetShaderActive (bool boActive, FuncPersist * psFuncData);
void CartesianSetShaderShadowActive (bool boActive, FuncPersist * psFuncData);
void CartesianUpdateCentre (FuncPersist * psFuncData);
void CartesianSetFunctionCentre (char const * const szXCentre, char const * const szYCentre, char const * const szZCentre, FuncPersist * psFuncData);
void CartesianGetCentre (double * afCentre, FuncPersist const * psFuncData);
char const * CartesianGetXCentreString (FuncPersist * psFuncData);
char const * CartesianGetYCentreString (FuncPersist * psFuncData);
char const * CartesianGetZCentreString (FuncPersist * psFuncData);
int CartesianOutputStoredVertices (Recall * hFile, bool boBinary, bool boScreenCoords, bool boExportAlpha, double fMultiplier, double fScale, FuncPersist const * psFuncData);
int CartesianOutputStoredIndices (Recall * hFile, bool boBinary, double fMultiplier, int nOffset, FuncPersist const * psFuncData);
int CartesianCountStoredVertices (double fMultiplier, FuncPersist const * psFuncData);
int CartesianCountStoredFaces (double fMultiplier, FuncPersist const * psFuncData);
int CartesianOutputStoredTrianglesSTL (Recall * hFile, bool boBinary, bool boScreenCoords, double fMultiplier, double fScale, FuncPersist const * psFuncData);
bool CartesianAssignControlVarsToFunction (FnControlPersist * psFnControlData, FuncPersist * psFuncData);
void CartesianOutputVoxelSlice (unsigned char * pcData, int nResolution, int nChannels, int nSlice, FuncPersist * psFuncData);

///////////////////////////////////////////////////////////////////
// Function definitions

#endif /* CARTESIAN_H */


