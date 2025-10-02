///////////////////////////////////////////////////////////////////
// Functy
// 3D graph drawing utility
//
// David Llewellyn-Jones
// http://www.flypig.co.uk
//
// Spring 2009
///////////////////////////////////////////////////////////////////

#ifndef FUNCTION_PRIVATE_H
#define FUNCTION_PRIVATE_H

///////////////////////////////////////////////////////////////////
// Includes

#include "cartesian.h"
#include "spherical.h"
#include "curve.h"
#include "shader.h"

#include <symbolic.h>

///////////////////////////////////////////////////////////////////
// Defines

#define SETBIT(var, value, bit) var ^= (-(value) ^ (var)) & (1 << bit)

///////////////////////////////////////////////////////////////////
// Structures and enumerations

struct _FuncPersist {
	FUNCTYPE eType;
	union {
		CartesianPersist * psCartesianData;
		SphericalPersist * psSphericalData;
		CurvePersist * psCurveData;
		void * psNone;
	} Func;

	// The following relate to graph coordinates
	double fXMin;
	double fYMin;
	double fZMin;
	double fXWidth;
	double fYWidth;
	double fZWidth;

	// The following relate to screen coordinates
	double fAccuracy;
  Matrix4 mStructureTransform;

	// The actual function
	Operation * psFunction;
	Variable * psVariables;
	Variable * psVariableT;

	// Colour information
	bool boColourFunction;
	float fRed;
	float fGreen;
	float fBlue;
	float fAlpha;
	Operation * psRed;
	Operation * psGreen;
	Operation * psBlue;
	Operation * psAlpha;
	
	// Texture information
	GLuint uTexture;
	GString * szTexFile;
	GString * szTexXScale;
	GString * szTexYScale;
	GString * szTexXOffset;
	GString * szTexYOffset;

	// Handy data to help with the UI
	GString * szName;
	GString * szFunction;
	GString * szRed;
	GString * szGreen;
	GString * szBlue;
	GString * szAlpha;

	// Storage for the OpenGL vertex and normal data
	GLfloat * afVertices;
	GLfloat * afNormals;
	GLushort * auIndices;
	GLfloat * afColours;
	GLfloat * afTextureCoords;

	// Time related variables
	// bit 0: Function or colours includes time
	// bit 1: Function includes audio
	// bit 2: Colours include audio
	unsigned int uTimeDependent;

	// Shader related variables
	char * szShaderVertexSource;
	char * szShaderFragmentSource;
	ShaderPersist * psShaderData;
	GString * szShaderFunction;
	GString * szShaderRed;
	GString * szShaderGreen;
	GString * szShaderBlue;
	GString * szShaderAlpha;
	char * szShaderShadowVertexSource;
	char * szShaderShadowFragmentSource;
	ShaderPersist * psShaderShadowData;

	// Control variables - this is only a pointer, do doesn't need to be freed
	FnControlPersist * psFnControlData;

	// Shadow variables
	ShadowPersist * psShadowData;

	// Textures
	TexPersist * psTexData;

	// Material variables
	bool boMaterialFill;
	float fMaterialThickness;

	// Audio
	UserFunc * psUserFuncs;

	// Global
	GlobalPersist const * psGlobalData;
};

///////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////
// Function prototypes

void LoadVertexShader (char const * const szFilename, FuncPersist * psFuncData);
void LoadFragmentShader (char const * const szFilename, FuncPersist * psFuncData);
void FreeVertexBuffers (FuncPersist * psFuncData);
void LoadVertexShaderShadow (char const * const szFilename, FuncPersist * psFuncData);
void LoadFragmentShaderShadow (char const * const szFilename, FuncPersist * psFuncData);


///////////////////////////////////////////////////////////////////
// Function definitions

#endif /* FUNCTION_PRIVATE_H */


