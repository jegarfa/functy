///////////////////////////////////////////////////////////////////
// Enzyme
// 3D Functy/Dandelion/programming game
//
// David Llewellyn-Jones
//
// April 2014
///////////////////////////////////////////////////////////////////

#ifndef SHADOW_H
#define SHADOW_H

///////////////////////////////////////////////////////////////////
// Includes

#include "textures.h"

///////////////////////////////////////////////////////////////////
// Defines

///////////////////////////////////////////////////////////////////
// Structures and enumerations

typedef struct _ShadowPersist ShadowPersist;

///////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////
// Function prototypes

ShadowPersist * NewShadowPersist ();
void DeleteShadowPersist (ShadowPersist * psShadowData);

void SetShadowTexture (GLuint uShadowTexture, ShadowPersist * psShadowData);
GLuint GetShadowTexture (ShadowPersist * psShadowData);
Matrix4 * GetLightTransform (ShadowPersist * psShadowData);
Matrix4 * GetShadowModel (ShadowPersist * psShadowData);
Matrix4 * GetShadowProjection (ShadowPersist * psShadowData);
float GetShadowBias (ShadowPersist * psShadowData);
void SetShadowBias (float fBias, ShadowPersist * psShadowData);
void SetShadowShow (bool boShow, ShadowPersist * psShadowData);
void ShadowSetScreenDimensions (int nScreenWidth, int nScreenHeight, ShadowPersist * psShadowData);
int ShadowGetScreenWidth (ShadowPersist * psShadowData);
int ShadowGetScreenHeight (ShadowPersist * psShadowData);

///////////////////////////////////////////////////////////////////
// Function definitions

#endif /* SHADOW_H */

