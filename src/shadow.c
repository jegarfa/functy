///////////////////////////////////////////////////////////////////
// Enzyme
// 3D Functy/Dandelion/programming game
//
// David Llewellyn-Jones
//
// April 2014
///////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
// Includes

#include "utils.h"
#include "vis.h"
#include "shadow.h"

///////////////////////////////////////////////////////////////////
// Defines

#define BIAS_DEFAULT (0.0005)

///////////////////////////////////////////////////////////////////
// Structures and enumerations

struct _ShadowPersist {
  GLuint uShadowTexture;
  Matrix4 mLightTransform;
	Matrix4 mModel;
	Matrix4 mProjection;
	bool boShow;
	float fBias;
	int nScreenWidth;
	int nScreenHeight;
};

///////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////
// Function prototypes

///////////////////////////////////////////////////////////////////
// Function definitions

ShadowPersist * NewShadowPersist () {
	ShadowPersist * psShadowData;

	// Allocate some memory for the new structures
	psShadowData = g_new0 (ShadowPersist, 1);

	psShadowData->uShadowTexture = 0u;
	psShadowData->fBias = BIAS_DEFAULT;
	psShadowData->boShow = TRUE;
	psShadowData->nScreenWidth = 640.0f;
	psShadowData->nScreenHeight = 512.0f;

	return psShadowData;
}

void DeleteShadowPersist (ShadowPersist * psShadowData) {
	// Free up the structures
	g_free (psShadowData);
}

void SetShadowTexture (GLuint uShadowTexture, ShadowPersist * psShadowData) {
	psShadowData->uShadowTexture = uShadowTexture;
}

GLuint GetShadowTexture (ShadowPersist * psShadowData) {
	return psShadowData->uShadowTexture;
}

Matrix4 * GetLightTransform (ShadowPersist * psShadowData) {
	return & psShadowData->mLightTransform;
}

Matrix4 * GetShadowModel (ShadowPersist * psShadowData) {
	return & psShadowData->mModel;
}

Matrix4 * GetShadowProjection (ShadowPersist * psShadowData) {
	return & psShadowData->mProjection;
}

float GetShadowBias (ShadowPersist * psShadowData) {
	float fBias;
	
	if (psShadowData->boShow == TRUE) {
		fBias = psShadowData->fBias;
	}
	else {
		fBias = 2.0f;
	}

	return fBias;
}

void SetShadowBias (float fBias, ShadowPersist * psShadowData) {
	psShadowData->fBias = fBias;
}

void SetShadowShow (bool boShow, ShadowPersist * psShadowData) {
	psShadowData->boShow = boShow;
}

void ShadowSetScreenDimensions (int nScreenWidth, int nScreenHeight, ShadowPersist * psShadowData) {
	psShadowData->nScreenWidth = nScreenWidth;
	psShadowData->nScreenHeight = nScreenHeight;
}

int ShadowGetScreenWidth (ShadowPersist * psShadowData) {
	return psShadowData->nScreenWidth;
}

int ShadowGetScreenHeight (ShadowPersist * psShadowData) {
	return psShadowData->nScreenHeight;
}



