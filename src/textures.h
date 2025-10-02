///////////////////////////////////////////////////////////////////
// Functy
// 3D graph drawing utility
//
// David Llewellyn-Jones
// http://www.flypig.co.uk
//
// Spring 2009
///////////////////////////////////////////////////////////////////

#ifndef TEXTURES_H
#define TEXTURES_H

///////////////////////////////////////////////////////////////////
// Includes

#include "utils.h"

///////////////////////////////////////////////////////////////////
// Defines

#define TEXTURE_NONE (0)

///////////////////////////////////////////////////////////////////
// Structures and enumerations

typedef struct _TexPersist TexPersist;

typedef enum _TEXNAME {
	TEXNAME_INVALID = -1,
	
	TEXNAME_AUDIO,
	TEXNAME_GRID,
	
	TEXNAME_NUM
} TEXNAME;

///////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////
// Function prototypes

TexPersist * NewTexPersist (int nMaxTextures);
void DeleteTexPersist (TexPersist * psTexData);
void LoadTextures (TexPersist * psTexData);
GLuint GetTexture (TEXNAME eTexture, GLfloat afTexCoords[], TexPersist * psTexData);
GLuint GetTexturePortion (TEXNAME eTexture, GLfloat afPortion[], GLfloat afTexCoords[], TexPersist * psTexData);
GLuint LoadTextureRaw (char const * const szFilename, int const nWidth, int const nHeight, bool boAlpha);
GLuint CreateBlankTexture (int const nWidth, int const nHeight, bool boAlpha);
GLuint CreateBlankTextureSingleChannel (int const nWidth, int const nHeight);
void GetTextureCoord (TEXNAME eTexture, GLfloat fXIn, GLfloat fYIn, GLfloat * pfXOut, GLfloat * pfYOut, TexPersist * psTexData);
void UnloadTexture (GLuint uTexture);
void UpdateTextureNoAlpha (TEXNAME eTexture, int nWidth, int nHeight, GLubyte * cData, TexPersist * psTexData);
void UpdateTextureSingleChannel (TEXNAME eTexture, int nWidth, int nHeight, GLubyte * cData, TexPersist * psTexData);

///////////////////////////////////////////////////////////////////
// Function definitions

#endif /* TEXTURES_H */

