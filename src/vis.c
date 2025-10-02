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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <stdlib.h>
#include <gdk/gdkkeysyms.h>

#include "vis.h"
#include "textures.h"
#include "spherical.h"
#include "curve.h"
#include "exportply.h"
#include "exportstl.h"
#include "exportbitmap.h"
#include "exportsvx.h"
#include "shader.h"
#include "shadow.h"

///////////////////////////////////////////////////////////////////
// Defines

#define SQUARE_NUM          (10)

#define MOUSE_ROTATE_SCALE  (400.0f)
#define RADIUSSTEP          (0.3f)
#define PSISTEP             (3.14159265f / 150.0f)
#define POINTTOHALFLIFE     (0.1f)
#define POINTTOMINMOVE      (0.002f)
#define POINTTOELLEVATION   (3.0f)

#define MGL_SCALE           (1.0f)
#define MGL_WIDTH           (0.0f)
#define MGL_HEIGHT          (0.0f)
#define MGL_DEPTH           (0.0f)

#define MAXKEYS             (256)
#define SELBUFSIZE          (512)

#define VIEW_RADIUS         (50.0f)

#define NUMBERTEXT_FONT			(GLUT_BITMAP_HELVETICA_10)
#define NUMBERTEXT_XOFF     (3)
#define NUMBERTEXT_YOFF     (3)
#define NUMBERTEXT_COLOUR   1.0, 1.0, 1.0
#define NUMBERTEXT_COLINV   0.0, 0.0, 0.0

#define MOMENTUM_MIN        (0.0005f)
#define MOMENTUM_RES        (0.98f)
#define MOMENTUMFUNC_MIN    (0.0005f)
#define MOMENTUMFUNC_RES    (0.95f)

#define NUMBER_MAX          (10)

#define FUNCTION1           "4 * cos ((((x * x) + (y * y))**(1/2)))"
#define FUNCTION2           "0 - (4 * cos ((((x * x) + (y * y))**(1/2))))"

#define RANGE_UPDATE_TIME   (0.1)

#define GRIDLINES_MAX       (14)

#define FRAMEBUFFER_TEXTURES (3)
#define SHADOWSHIFT_UP (20.0f)
#define SHADOWSHIFT_SIDEWAYS (SHADOWSHIFT_UP)

///////////////////////////////////////////////////////////////////
// Structures and enumerations

typedef enum _DRAGRESULT {
	DRAGRESULT_INVALID = -1,

	DRAGRESULT_MOVE,
	DRAGRESULT_LINK,
	DRAGRESULT_ILLEGAL,

	DRAGRESULT_NUM
} DRAGRESULT;

struct _VisPersist {
	double fCurrentTime;
	double fPrevTime;
	double fSpinTime;
	double fFunctionTime;
	double fFunctionTimePrev;
	double fRangeChangeTime;
	float fViewRadius;
	bool boPaused;

	float fRotation;
	float fElevation;

	// The following relate to graph coordinates
	double fXMin;
	double fYMin;
	double fZMin;
	double fXWidth;
	double fYWidth;
	double fZWidth;
	double fXGrid;
	double fYGrid;
	double fZGrid;

	// The following relate to screen coordinates
	Vector3 vCamera;
	Vector3 vUp;
	Vector3 vLightPos;
	float fX;
	float fY;
	float fZ;
	float fXn;
	float fYn;
	float fZn;
	bool boSpin;
	int nXMouse;
	int nYMouse;
	int boMoving;
	double fXMouseFunction;
	double fYMouseFunction;
	int boMovingFunction;
	int nScreenWidth;
	int nScreenHeight;
	int nPrevScreenWidth;
	int nPrevScreenHeight;
	bool boFullScreen;
	bool boClearWhite;
	bool boDrawAxes;
	bool boWireframe;
	float fHalfLife;
	bool boArrived;
	bool aboKeyDown[MAXKEYS];
	float fMomentum;
	float fXMomentum;
	float fYMomentum;
	float fZMomentum;
	float fMomentumFunction;
	float fXMomentumFunction;
	float fYMomentumFunction;
	float fZMomentumFunction;
	bool boRangeChange;
	bool boShader;
	bool boShadow;
	bool boFocusBlur;
	float fFocusBlurNear;
	float fFocusBlurFar;

	GLint anViewPort[4];
	GLdouble afModel[16];
	GLdouble afProjection[16];

	// Other structures
	GSList * psFuncList;
	FnControlPersist * psFnControlData;

	TexPersist * psTexData;
	GLfloat afTexCoords[4];

	// Frame buffer data
	GLuint uScreenBuffer;
	GLuint uFrameBuffer;
	GLuint uShadowBuffer;
	GLuint auFrameTextures[FRAMEBUFFER_TEXTURES];
	ShaderPersist * psScreenShader;
	ShaderPersist * psDebugDepthShader;

	// Shadow variables
	ShadowPersist * psShadowData;

	// Audio
	AudioPersist * psAudioData;
	UserFunc * psUserFuncs;

	// Global
	GlobalPersist const * psGlobalData;
};

///////////////////////////////////////////////////////////////////
// Global variables

static GLfloat gMatDiffuse[] = { 1.0, 1.0, 1.0, 1.0 };

static GLfloat gBoardSpecular[] = { 0.7, 0.7, 0.7, 0.9 };
static GLfloat gBoardShininess[] = { 10.0 };
static GLfloat gGridColour[] = { 1.0, 1.0, 1.0, 1.0 };
//static GLfloat gGraphColour[] = { 1.0, 0.5, 0.5, 0.9 };

///////////////////////////////////////////////////////////////////
// Function prototypes

void Render (VisPersist * psVisData);
void RenderTextInSpace (char const * szText, GLdouble fX, GLdouble fY, GLdouble fZ);
void DrawTextOverlay (VisPersist * psVisData);
void ChangeView (float fTheta, float fPhi, float fPsi, float fRadius, VisPersist * psVisData);
void KeyIdle (VisPersist * psVisData);
bool PointTowards (float fXPos, float fYPos, float fZPos, float fRadius, VisPersist * psVisData);
void ResetAnimation (VisPersist * psVisData);
void Spin (VisPersist * psVisData);
void MomentumSpin (VisPersist * psVisData);
void MomentumFunction (VisPersist * psVisData);
bool Intersect (float fX0, float fY0, float fX1, float fY1, float fA0, float fB0);
void inline swap (float * pfVar1, float * pfVar2);
void inline order (float * pfVar1, float * pfVar2);
void DrawAxes (VisPersist * psVisData);
void DrawAxesNumbers (VisPersist * psVisData);
void DrawGraphs (VisPersist * psVisData);
void DrawGraphsShadow (VisPersist * psVisData);
GLfloat SelectFunction (int nXPos, int nYPos, VisPersist * psVisData);
void ConvertCoords (GLdouble fXMousePos, GLdouble fYMousePos, float fZPos, double * pfX, double * pfY, double * pfZ, VisPersist * psVisData);
void ShuntFunctionPosition (double fX, double fY, double fZ, VisPersist * psVisData);
void DrawFloor (VisPersist * psVisData);
void DrawWallY (double fY, unsigned int uCullOrient, double fNormal, VisPersist * psVisData);
void DrawWallX (double fX, unsigned int uCullOrient, double fNormal, VisPersist * psVisData);
void DrawNumbersX (double fY, double fZ, VisPersist * psVisData);
void DrawNumbersY (double fX, double fZ, VisPersist * psVisData);
void DrawNumbersZ (double fX, double fY, VisPersist * psVisData);
void DeleteFuncDataCallback (gpointer data, gpointer user_data);
void ShuntFunctionPositionCallback (gpointer data, gpointer user_data);
void AnimateFunction (VisPersist * psVisData);
void SetFunctionPositionCallback (gpointer data, gpointer user_data);
void CalculateGridScale (VisPersist * psVisData);
//void InitShaders (VisPersist * psVisData);
void ToggleShaders (VisPersist * psVisData);
void AssignControlVarsCallback (gpointer data, gpointer user_data);
void FramebufferCreate (VisPersist * psVisData);
GLenum CheckFramebufferStatus ();
void FramebufferDestroy (VisPersist * psVisData);
void FramebufferResize (VisPersist * psVisData);
void RenderToScreenBuffer (VisPersist * psVisData);
void RenderToFrameBuffer (VisPersist * psVisData);
void RenderToShadowBuffer (VisPersist * psVisData);
void RenderFramebufferToScreen (VisPersist * psVisData);
void RenderDebugOverlay (VisPersist * psVisData);
void RenderShadow (VisPersist * psVisData);
void CalculateLightPos (VisPersist * psVisData);
void SetLightTransformMatrix (VisPersist * psVisData);
void FreePixelBuffer (guchar * pixels, gpointer data);

///////////////////////////////////////////////////////////////////
// Function definitions

void inline swap (float * pfVar1, float * pfVar2) {
	float fTemp;

	fTemp = * pfVar1;
	*pfVar1 = *pfVar2;
	*pfVar2 = fTemp;
}

void inline order (float * pfVar1, float * pfVar2) {
	if (*pfVar1 > *pfVar2) {
		swap (pfVar1, pfVar2);
	}
}

VisPersist * NewVisPersist (GlobalPersist const * psGlobalData) {
	VisPersist * psVisData;
	int nFramebuffer;
	//FuncPersist * psFuncData;

	psVisData = g_new0 (VisPersist, 1);

	psVisData->psGlobalData = psGlobalData;

	psVisData->fViewRadius = VIEW_RADIUS;

	psVisData->fRotation = 0.0f;
	psVisData->fElevation = 0.0f;
	psVisData->boPaused = FALSE;

	// The following relate to graph coordinates
	psVisData->fXMin = -5.0;
	psVisData->fYMin = -5.0;
	psVisData->fZMin = -5.0;
	psVisData->fXWidth = 10.0;
	psVisData->fYWidth = 10.0;
	psVisData->fZWidth = 10.0;
	psVisData->fXGrid = 1.0;
	psVisData->fYGrid = 1.0;
	psVisData->fZGrid = 1.0;

	// The following relate to screen coordinates
	SetVector3 (psVisData->vCamera, 0.0f, 0.0f, 1.0f);
	SetVector3 (psVisData->vUp, 0.0f, 1.0f, 0.0f);
	SetVector3 (psVisData->vLightPos, 1.0f, 1.0f, 0.0f);
	CalculateLightPos (psVisData);
	psVisData->boSpin = FALSE;
	psVisData->nXMouse = 0;
	psVisData->nYMouse = 0;
	psVisData->boMoving = FALSE;
	psVisData->fXMouseFunction = 0.0;
	psVisData->fYMouseFunction = 0.0;
	psVisData->boMovingFunction = FALSE;
	psVisData->nScreenWidth = SCREENWIDTH;
	psVisData->nScreenHeight = SCREENHEIGHT;
	psVisData->nPrevScreenWidth = SCREENWIDTH;
	psVisData->nPrevScreenHeight = SCREENHEIGHT;
	psVisData->boFullScreen = false;
	psVisData->boClearWhite = false;
	psVisData->boDrawAxes = true;
	psVisData->boWireframe = false;
	psVisData->fMomentum = 0.0f;
	psVisData->fXMomentum = 0.0f;
	psVisData->fYMomentum = 0.0f;
	psVisData->fZMomentum = 0.0f;
	psVisData->fMomentumFunction = 0.0f;
	psVisData->fXMomentumFunction = 0.0f;
	psVisData->fYMomentumFunction = 0.0f;
	psVisData->fZMomentumFunction = 0.0f;
	psVisData->boRangeChange = FALSE;
	psVisData->boShader = TRUE;

	psVisData->boShadow = TRUE;
	psVisData->boFocusBlur = TRUE;
	psVisData->fFocusBlurNear = 0.99;
	psVisData->fFocusBlurFar = 0.0;

	// Make sure the function list is empty
	psVisData->psFuncList = NULL;
	psVisData->psFnControlData = NewFnControlPersist ();



	/*
	// Add a function to the list
	psFuncData = NewFuncPersist (FUNCTYPE_SPHERICAL, 0.02, psVisData->psGlobalData);
	SetFunction (FUNCTION1, psFuncData);
	SetFunctionRange (psVisData->fXMin, psVisData->fYMin, psVisData->fZMin, 
		psVisData->fXWidth, psVisData->fYWidth, psVisData->fZWidth, psFuncData);
	SetFunctionColours ("1.0", "0.5", "0.5", "0.9", psFuncData);
	psVisData->psFuncList = g_slist_prepend (psVisData->psFuncList, psFuncData);

	// Add a function to the list
	psFuncData = NewFuncPersist (FUNCTYPE_SPHERICAL, 0.02, psVisData->psGlobalData);
	SetFunction (FUNCTION2, psFuncData);
	SetFunctionRange (psVisData->fXMin, psVisData->fYMin, psVisData->fZMin, 
		psVisData->fXWidth, psVisData->fYWidth, psVisData->fZWidth, psFuncData);
	SetFunctionColours ("0.5", "0.5", "1.0", "0.9", psFuncData);
	psVisData->psFuncList = g_slist_prepend (psVisData->psFuncList, psFuncData);
	*/

	psVisData->psTexData = NewTexPersist (TEXNAME_NUM);

	// Frame buffer data
	psVisData->uScreenBuffer = 0u;
	psVisData->uFrameBuffer = 0u;
	psVisData->uShadowBuffer = 0u;
	for (nFramebuffer = 0; nFramebuffer < FRAMEBUFFER_TEXTURES; nFramebuffer++) {
		psVisData->auFrameTextures[nFramebuffer] = 0u;
	}
	psVisData->psScreenShader = NULL;
	psVisData->psDebugDepthShader = NULL;

	// Shadow data
	psVisData->psShadowData = NewShadowPersist ();

	// Audio data
	psVisData->psAudioData = NewAudioPersist ();
	SetAudioTextureData (psVisData->psTexData, psVisData->psAudioData);
	psVisData->psUserFuncs = NULL;
	psVisData->psUserFuncs = AddNewUserFunc (psVisData->psUserFuncs, "audio");
	SetUserFuncCallbacks (psVisData->psUserFuncs, AudioApproximate, AudioDifferentiate, AudioSimplify, psVisData->psAudioData);

	return psVisData;
}

void DeleteVisPersist (VisPersist * psVisData) {

	// Delete all of the functions
	g_slist_foreach (psVisData->psFuncList, DeleteFuncDataCallback, NULL);
	g_slist_free (psVisData->psFuncList);

	// Delete all of the control varaibles
	DeleteFnControlPersist (psVisData->psFnControlData);

	// Free the textures
	DeleteTexPersist (psVisData->psTexData);
	psVisData->psTexData = NULL;

	// Free the shader if there is one
	if (psVisData->psScreenShader != NULL) {
		DeleteShaderPersist (psVisData->psScreenShader);
		psVisData->psScreenShader = NULL;
	}

	// Free the debug shader if there is one
	if (psVisData->psDebugDepthShader != NULL) {
		DeleteShaderPersist (psVisData->psDebugDepthShader);
		psVisData->psDebugDepthShader = NULL;
	}

	// Free the shadow data
	DeleteShadowPersist (psVisData->psShadowData);
	psVisData->psShadowData = NULL;

	// Free the audio data
	DeleteAudioPersist (psVisData->psAudioData);
	psVisData->psAudioData = NULL;
	psVisData->psUserFuncs = FreeUserFuncs (psVisData->psUserFuncs);

	// Unlink the global data
	psVisData->psGlobalData = NULL;

	g_free (psVisData);
}

void DeleteFuncDataCallback (gpointer data, gpointer user_data) {
	DeleteFuncPersist ((FuncPersist *)data);
}

FuncPersist * AddNewFunction (FUNCTYPE eType, VisPersist * psVisData) {
	FuncPersist * psFuncData;
	GString * szName;
	unsigned int uFunctionCount;

	psFuncData = NewFuncPersist (eType, psVisData->psGlobalData);
	SetFunctionTextureData (psVisData->psTexData, psFuncData);
	SetFunctionTexture (TEXTURE_NONE, psFuncData);
	SetFunctionControlVars (psVisData->psFnControlData, psFuncData);
	SetFunctionShadowData (psVisData->psShadowData, psFuncData);
	SetFunctionUserFuncs (psVisData->psUserFuncs, psFuncData);

	uFunctionCount = GetFunctionCount (psVisData);
	szName = g_string_new ("");
	// Set an initial function name
	switch (eType) {
		case FUNCTYPE_CARTESIAN:
			g_string_printf (szName, "Cartesian %u", uFunctionCount);
			break;
		case FUNCTYPE_SPHERICAL:
			g_string_printf (szName, "Spherical %u", uFunctionCount);
			break;
		case FUNCTYPE_CURVE:
			g_string_printf (szName, "Curve %u", uFunctionCount);
			break;
		default:
			g_string_printf (szName, "Function %u", uFunctionCount);
			break;
	}

	SetFunctionName (szName->str, psFuncData);
	g_string_free (szName, TRUE);

	psVisData->psFuncList = g_slist_append (psVisData->psFuncList, psFuncData);

	return psFuncData;
}

void DeleteFunction (FuncPersist * psFuncData, VisPersist * psVisData) {
	// There should only be one, but we'll remove all just in case
	psVisData->psFuncList = g_slist_remove_all (psVisData->psFuncList, psFuncData);

	// Actually delete the function
	DeleteFuncPersist (psFuncData);
}

void DeleteAllFunctions (VisPersist * psVisData) {
	// Delete all of the functions
	g_slist_foreach (psVisData->psFuncList, DeleteFuncDataCallback, NULL);
	g_slist_free (psVisData->psFuncList);

	psVisData->psFuncList = NULL;
}

void Realise (VisPersist * psVisData) {
	static GLfloat afLightAmbient[] = {0.3, 0.3, 0.3, 1.0, 0.5, 0.5, 0.5, 1.0};
	static GLfloat afLightDiffuse[] = {0.5, 0.5, 0.5, 1.0, 0.5, 0.3, 0.0, 1.0};
	static GLfloat afLightSpecular[] = {0.2, 0.2, 0.2, 1.0, 0.3, 0.5, 1.0, 0.0};
	static GLfloat afLightPosition[] = {0.0, 10.0, 0.0, 0.0, 1.0, 1.0, 1.0, 0.0};

	glLightfv (GL_LIGHT0, GL_DIFFUSE, afLightDiffuse);
	glLightfv (GL_LIGHT0, GL_SPECULAR, afLightSpecular);
	glLightfv (GL_LIGHT0, GL_POSITION, afLightPosition);
	//glLightfv (GL_LIGHT0, GL_AMBIENT, light_ambient);
	glLightModelfv (GL_LIGHT_MODEL_AMBIENT, afLightAmbient);
	glLightModeli (GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);

	//glLightfv (GL_LIGHT1, GL_AMBIENT, afLightAmbient + 4);
	//glLightfv (GL_LIGHT1, GL_DIFFUSE, afLightDiffuse + 4);
	//glLightfv (GL_LIGHT1, GL_SPECULAR, afLightSpecular + 4);
	//glLightfv (GL_LIGHT1, GL_POSITION, afLightPosition + 4);

	glEnable (GL_LIGHTING);
	glEnable (GL_LIGHT0);
	//glEnable (GL_LIGHT1);
	glEnable (GL_CULL_FACE);
	glEnable (GL_DEPTH_TEST);
	glEnable (GL_NORMALIZE);
	glLightModeli (GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	//glEnable (GL_TEXTURE_2D);
	glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glClearColor (0.0f, 0.0f, 0.0f, 0.0f);
}

void Unrealise (VisPersist * psVisData) {
	FramebufferDestroy (psVisData);
}

void FramebufferCreate (VisPersist * psVisData) {
	// Generate colour and depth buffer textures for the framebuffer
	glGenTextures (FRAMEBUFFER_TEXTURES, psVisData->auFrameTextures);

	// Initialise the framebuffer colour texture
	glBindTexture (GL_TEXTURE_2D, psVisData->auFrameTextures[0]);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, psVisData->nScreenWidth, psVisData->nScreenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	// Initialise the framebuffer depth texture
	glBindTexture (GL_TEXTURE_2D, psVisData->auFrameTextures[1]);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D (GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, psVisData->nScreenWidth, psVisData->nScreenHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	// Initialise the framebuffer shadow texture
	glBindTexture (GL_TEXTURE_2D, psVisData->auFrameTextures[2]);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);

	glTexImage2D (GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, psVisData->nScreenWidth, psVisData->nScreenHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	glBindTexture (GL_TEXTURE_2D, 0);

	// Generate the actual framebuffer
	glGenFramebuffers (1, & psVisData->uFrameBuffer);
	glBindFramebuffer (GL_FRAMEBUFFER, psVisData->uFrameBuffer);

	// Assign the textures to the framebuffer
	glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, psVisData->auFrameTextures[0], 0);
	glFramebufferTexture2D (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, psVisData->auFrameTextures[1], 0);
	CheckFramebufferStatus ();

	// Generate the shadow framebuffer
	glGenFramebuffers (1, & psVisData->uShadowBuffer);
	glBindFramebuffer (GL_FRAMEBUFFER, psVisData->uShadowBuffer);

	// Assign the textures to the framebuffer
	glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, psVisData->auFrameTextures[0], 0);
	glFramebufferTexture2D (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, psVisData->auFrameTextures[2], 0);
	CheckFramebufferStatus ();

	SetShadowTexture (psVisData->auFrameTextures[2], psVisData->psShadowData);

	// Reset the framebuffer back to its default
	glBindFramebuffer (GL_FRAMEBUFFER, 0);
}

GLenum CheckFramebufferStatus () {
	GLenum eFramebufferStatus;

	// Check whether everything worked okay
	//printf ("Size: %d x %d\n", psVisData->nScreenWidth, psVisData->nScreenHeight);
	eFramebufferStatus = glCheckFramebufferStatus (GL_DRAW_FRAMEBUFFER);
	switch (eFramebufferStatus) {
		case GL_FRAMEBUFFER_UNDEFINED:
			fprintf (stderr, "Framebuffer undefined.\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT :
			fprintf (stderr, "Framebuffer incomplete attachment,\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT :
			fprintf (stderr, "Framebuffer incomplete missing attachment,\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER :
			fprintf (stderr, "Framebuffer incomplete draw buffer.\n");
			break;
		case GL_FRAMEBUFFER_UNSUPPORTED :
			fprintf (stderr, "Framebuffer unsupported.\n");
			break;
		case GL_FRAMEBUFFER_COMPLETE:
			// Framebuffer okay: do nothing
			break;
		default:
			fprintf (stderr, "Framebuffer status undefined.\n");
	}

	return eFramebufferStatus;
}

void FramebufferDestroy (VisPersist * psVisData) {
	int nFramebuffer;
	
	for (nFramebuffer = 0; nFramebuffer < FRAMEBUFFER_TEXTURES; nFramebuffer++) {
			// Delete the framebuffer colour/depth/stencil texture
			if (psVisData->auFrameTextures[nFramebuffer] > 0) {
				glDeleteTextures (1, & psVisData->auFrameTextures[nFramebuffer]);
				psVisData->auFrameTextures[nFramebuffer] = 0u;
			}
	}

	// Delete the framebuffer
	if (psVisData->uFrameBuffer > 0) {
		glDeleteFramebuffers (1, & psVisData->uFrameBuffer);
		psVisData->uFrameBuffer = 0u;
	}

	// Delete the shadow framebuffer
	if (psVisData->uShadowBuffer > 0) {
		glDeleteFramebuffers (1, & psVisData->uShadowBuffer);
		psVisData->uShadowBuffer = 0u;
		SetShadowTexture (0u, psVisData->psShadowData);
	}
}

void FramebufferResize (VisPersist * psVisData) {
	// Destroy the previous framebuffer objects
	FramebufferDestroy (psVisData);
	
	// Creater new framebuffer objects
	FramebufferCreate (psVisData);
}

void RenderToFrameBuffer (VisPersist * psVisData) {
	// Set the framebuffer to the framebuffer texturee
	glBindFramebuffer (GL_FRAMEBUFFER, psVisData->uFrameBuffer);
}

void RenderToScreenBuffer (VisPersist * psVisData) {
	// Set the framebuffer to be the screen
	glBindFramebuffer (GL_FRAMEBUFFER, psVisData->uScreenBuffer);
}

void RenderToShadowBuffer (VisPersist * psVisData) {
	// Set the framebuffer to be the screen
	glBindFramebuffer (GL_FRAMEBUFFER, psVisData->uShadowBuffer);
}

void RenderFramebufferToScreen (VisPersist * psVisData) {
	GLuint uShaderProgram;
	static const GLfloat afVertices[(4 * 3)] = {1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f};
	static const GLfloat afTexCoords[(4 * 2)] = {1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f};
	static const GLubyte auIndices[(3 * 2)] = {0, 1, 2, 2, 3, 0};
	GLint nFocusBlurNear;
	GLint nFocusBlurFar;
	GLfloat fFocusBlurNear;
	GLfloat fFocusBlurFar;

	// Set the co-ordinate system to be the screen in the range (0, 1)
	glPushAttrib (GL_TRANSFORM_BIT | GL_VIEWPORT_BIT);
	glMatrixMode (GL_PROJECTION);
	glPushMatrix ();
	glLoadIdentity ();
	gluOrtho2D (0, 1, 0, 1);
	glMatrixMode (GL_MODELVIEW);
	glPushMatrix ();
	glLoadIdentity ();

	glClearColor (1.0f, 0.0f, 0.0f, 1.0f);
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	ActivateShader (psVisData->psScreenShader);
	uShaderProgram = GetShaderProgram (psVisData->psScreenShader);

	glActiveTexture (GL_TEXTURE1);
	glBindTexture (GL_TEXTURE_2D, psVisData->auFrameTextures[1]);
	glUniform1i (glGetUniformLocation (uShaderProgram, "framebufferDepth" ), 1);

	glActiveTexture (GL_TEXTURE0);
	glBindTexture (GL_TEXTURE_2D, psVisData->auFrameTextures[0]);
	glUniform1i (glGetUniformLocation (uShaderProgram, "framebufferTexture" ), 0);


	fFocusBlurNear = psVisData->fFocusBlurNear;
	fFocusBlurFar = fFocusBlurNear + (psVisData->fFocusBlurFar * (1.0 - fFocusBlurNear));
	if (!psVisData->boFocusBlur) {
		fFocusBlurNear = 0.0f;
		fFocusBlurFar = 1.0f;
	}

	nFocusBlurNear = glGetUniformLocation (uShaderProgram, "fFocusNear");
	glUniform1fv (nFocusBlurNear, 1, (GLfloat *)(& fFocusBlurNear));
	nFocusBlurFar = glGetUniformLocation (uShaderProgram, "fFocusFar");
	glUniform1fv (nFocusBlurFar, 1, (GLfloat *)(& fFocusBlurFar));


	glDisable (GL_DEPTH_TEST);
	glDisable (GL_TEXTURE_2D);

	// Render the box
	glColor4f (1.0, 1.0, 1.0, 1.0);

	glEnableClientState (GL_VERTEX_ARRAY);
	glEnableClientState (GL_TEXTURE_COORD_ARRAY);

	glVertexPointer (3, GL_FLOAT, 0, afVertices);
	glTexCoordPointer (2, GL_FLOAT, 0, afTexCoords);
	glDrawElements (GL_TRIANGLES, (3 * 2), GL_UNSIGNED_BYTE, auIndices);

	// Tidy up
	glBindTexture (GL_TEXTURE_2D, 0);
	glDisable (GL_TEXTURE_2D);
	glEnable (GL_DEPTH_TEST);

	DeactivateShader (psVisData->psScreenShader);

	glDisableClientState (GL_VERTEX_ARRAY);
	glDisableClientState (GL_TEXTURE_COORD_ARRAY);

	// Reset the co-ordinate system back
	glPopMatrix ();
	glMatrixMode (GL_PROJECTION);
	glPopMatrix ();
	glPopAttrib ();

	//Render (psVisData);
}

void RenderDebugOverlay (VisPersist * psVisData) {
	GLuint uShaderProgram;
	float fScale = 0.5f;
	GLfloat afVertices[(4 * 3)] = {0.5f, 0.0f, 0.0f, 0.5f, 0.5f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f};
	static const GLfloat afTexCoords[(4 * 2)] = {1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f};
	static const GLubyte auIndices[(3 * 2)] = {0, 1, 2, 2, 3, 0};
	afVertices[0] = fScale;
	afVertices[3] = fScale;
	afVertices[4] = fScale;
	afVertices[7] = fScale;

	// Set the co-ordinate system to be the screen in the range (0, 1)
	glPushAttrib (GL_TRANSFORM_BIT | GL_VIEWPORT_BIT);
	glMatrixMode (GL_PROJECTION);
	glPushMatrix ();
	glLoadIdentity ();
	gluOrtho2D (0, 1, 0, 1);
	glMatrixMode (GL_MODELVIEW);
	glPushMatrix ();
	glLoadIdentity ();

	ActivateShader (psVisData->psDebugDepthShader);
	uShaderProgram = GetShaderProgram (psVisData->psDebugDepthShader);

	glDisable (GL_DEPTH_TEST);
	glEnable (GL_TEXTURE_2D);

	glActiveTexture (GL_TEXTURE0);
	glBindTexture (GL_TEXTURE_2D, GetShadowTexture (psVisData->psShadowData));
	//glBindTexture (GL_TEXTURE_2D, psVisData->auFrameTextures[2]);
	glUniform1i (glGetUniformLocation (uShaderProgram, "framebufferDepth" ), 0);

	// Render the box
	glColor4f (1.0, 1.0, 1.0, 1.0);

	glEnableClientState (GL_VERTEX_ARRAY);
	glEnableClientState (GL_TEXTURE_COORD_ARRAY);

	glVertexPointer (3, GL_FLOAT, 0, afVertices);
	glTexCoordPointer (2, GL_FLOAT, 0, afTexCoords);
	glDrawElements (GL_TRIANGLES, (3 * 2), GL_UNSIGNED_BYTE, auIndices);

	// Tidy up
	glBindTexture (GL_TEXTURE_2D, 0);
	glDisable (GL_TEXTURE_2D);
	glEnable (GL_DEPTH_TEST);

	DeactivateShader (psVisData->psDebugDepthShader);

	glDisableClientState (GL_VERTEX_ARRAY);
	glDisableClientState (GL_TEXTURE_COORD_ARRAY);

	// Reset the co-ordinate system back
	glPopMatrix ();
	glMatrixMode (GL_PROJECTION);
	glPopMatrix ();
	glPopAttrib ();
}

void Init (VisPersist * psVisData) {
	int nCount;
	char * szShaderVertexSource;
	char * szShaderFragmentSource;
	struct timeb sTime;
	GString * szPath;

	for (nCount = 0; nCount < MAXKEYS; nCount++) {
		psVisData->aboKeyDown[nCount] = false;
	}

	// Inititalise the shader
	psVisData->psScreenShader = NewShaderPersist ();

	szPath = g_string_new ("/shaders/screen.vs");
	GenerateDataPath (szPath, psVisData->psGlobalData);
	szShaderVertexSource = LoadShaderFile (szPath->str);

	szPath = g_string_assign (szPath, "/shaders/screen.fs");
	GenerateDataPath (szPath, psVisData->psGlobalData);
	szShaderFragmentSource = LoadShaderFile (szPath->str);

	// Generate the shaders
	ShaderRegenerateVertex (szShaderVertexSource, psVisData->psScreenShader);
	ShaderRegenerateFragment (szShaderFragmentSource, psVisData->psScreenShader);

	// Free up the previously loaded shader source
	g_free (szShaderVertexSource);
	g_free (szShaderFragmentSource);

	// Initialise the debug depth shader
	psVisData->psDebugDepthShader = NewShaderPersist ();

	szPath = g_string_assign (szPath, "/shaders/depth.vs");
	GenerateDataPath (szPath, psVisData->psGlobalData);
	szShaderVertexSource = LoadShaderFile (szPath->str);

	szPath = g_string_assign (szPath, "/shaders/depth.fs");
	GenerateDataPath (szPath, psVisData->psGlobalData);
	szShaderFragmentSource = LoadShaderFile (szPath->str);

	g_string_free (szPath, TRUE);

	// Generate the debug shaders
	ShaderRegenerateVertex (szShaderVertexSource, psVisData->psDebugDepthShader);
	ShaderRegenerateFragment (szShaderFragmentSource, psVisData->psDebugDepthShader);

	// Free up the previously loaded debug shader source
	g_free (szShaderVertexSource);
	g_free (szShaderFragmentSource);

	// Set up the initial camera position
	ChangeView (0.0f, M_PI_2 / 2.5f, 0.0f, 0.0f, psVisData);

	// Load textures
	LoadTextures (psVisData->psTexData);

	ftime (& sTime);
	psVisData->fCurrentTime = (double)(sTime.time) + (double)(sTime.millitm) / 1000.0;
	if (psVisData->fCurrentTime < 0) {
		psVisData->fCurrentTime = 0.0;
	}
	psVisData->fPrevTime = psVisData->fCurrentTime;
	psVisData->fSpinTime = psVisData->fCurrentTime;
	psVisData->fFunctionTime = 0.0;
	psVisData->fFunctionTimePrev = psVisData->fCurrentTime;
	psVisData->fRangeChangeTime = psVisData->fCurrentTime;
	psVisData->boPaused = FALSE;
}

void Deinit (VisPersist * psVisData) {
	DeleteVisPersist (psVisData);
}

void Spin (VisPersist * psVisData) {
	double fTimeChange;

	fTimeChange = psVisData->fCurrentTime - psVisData->fSpinTime;
	if (fTimeChange > 0.005f) {
		if (fTimeChange > 0.05f) {
			fTimeChange = 0.05f;
		}
		ChangeView (0.2f * fTimeChange, 0.03f * sin (0.045f * psVisData->fCurrentTime)* fTimeChange, 0.0f, 0.0f, psVisData);
		psVisData->fSpinTime = psVisData->fCurrentTime;
	}
}

void AnimateFunction (VisPersist * psVisData) {
	GSList * psFuncList;
	double fTimeChange;
	FuncPersist * psFuncData;

	fTimeChange = psVisData->fCurrentTime - psVisData->fFunctionTimePrev;
	if (fTimeChange > 0.005f) {
		if (fTimeChange > 0.05f) {
			fTimeChange = 0.05f;
		}

		if (!psVisData->boPaused) {
			psVisData->fFunctionTime += fTimeChange;
		}

		// Check all of the functions in case they need animating

		psFuncList = psVisData->psFuncList;

		while (psFuncList) {
			psFuncData = (FuncPersist *)(psFuncList->data);

			if (GetTimeDependent (psFuncData)) {
				SetFunctionTime (psVisData->fFunctionTime, psFuncData);
				if (!psVisData->boShader) {
					PopulateVertices (psFuncData);
				}
			}
			else {
				if (GetCentreTimeDependent (psFuncData)) {
					SetFunctionTime (psVisData->fFunctionTime, psFuncData);
				}
			}

			psFuncList = g_slist_next (psFuncList);
		}
		psVisData->fFunctionTimePrev = psVisData->fCurrentTime;
	}
}

void DrawSquare (GLdouble fXCentre, GLdouble fYCentre, GLdouble fZCentre, GLfloat afTexCoords[]) {
	glPushMatrix ();
	glTranslatef (fXCentre, fYCentre, fZCentre);
	glScalef ((AXIS_XSIZE / SQUARE_NUM), (AXIS_YSIZE / SQUARE_NUM), 1.0f);

	glBegin (GL_QUADS);
	glNormal3f (0.0f, 0.0f, 1.0f);

	if (afTexCoords) {
		// Draw a square and include texture coordinates
		glTexCoord2f (afTexCoords[1] * 2, afTexCoords[2] * 2);
		glVertex3f (0.0f, 0.0f, 0.0f);
		glTexCoord2f (afTexCoords[0] * 2, afTexCoords[2] * 2);
		glVertex3f (1.0f, 0.0f, 0.0f);
		glTexCoord2f (afTexCoords[0] * 2, afTexCoords[3] * 2);
		glVertex3f (1.0f, 1.0f, 0.0f);
		glTexCoord2f (afTexCoords[1] * 2, afTexCoords[3] * 2);
		glVertex3f (0.0f, 1.0f, 0.0f);
	}
	else {
		// Draw square without texture coordinates
		glVertex3f (0.0f, 0.0f, 0.0f);
		glVertex3f (1.0f, 0.0f, 0.0f);
		glVertex3f (1.0f, 1.0f, 0.0f);
		glVertex3f (0.0f, 1.0f, 0.0f);
	}
	glEnd();

	glPopMatrix ();
}

void RenderTextInSpace (char const * szText, GLdouble fX, GLdouble fY, GLdouble fZ) {
	GLdouble afModel[16];
	GLdouble afProjection[16];
	GLint anViewpoert[4];

	glGetDoublev (GL_MODELVIEW_MATRIX, afModel);
	glGetDoublev (GL_PROJECTION_MATRIX, afProjection);
	glGetIntegerv (GL_VIEWPORT, anViewpoert);

	gluProject (fX, fY, fZ, afModel, afProjection, anViewpoert, & fX, & fY, & fZ);

	glDisable (GL_LIGHTING);
	fX += NUMBERTEXT_XOFF;
	fY += NUMBERTEXT_YOFF;

	RenderBitmapString ((float)fX, (float)fY, NUMBERTEXT_FONT, szText);
	glEnable (GL_LIGHTING);
}

void Render (VisPersist * psVisData) {
	glLoadIdentity ();
	gluLookAt ((psVisData->fViewRadius) * psVisData->vCamera.fX, (psVisData->fViewRadius) * psVisData->vCamera.fY, (psVisData->fViewRadius) * psVisData->vCamera.fZ, 0.0, 0.0, 0.0, psVisData->vUp.fX, psVisData->vUp.fY, psVisData->vUp.fZ);
	//gluLookAt ((psVisData->fViewRadius) * psVisData->fX, (psVisData->fViewRadius) * psVisData->fY, (psVisData->fViewRadius) * psVisData->fZ, 0.0, 0.0, 0.0, psVisData->fXn, psVisData->fYn, psVisData->fZn);

	glGetDoublev (GL_MODELVIEW_MATRIX, psVisData->afModel);
	glGetDoublev (GL_PROJECTION_MATRIX, psVisData->afProjection);
	glGetIntegerv (GL_VIEWPORT, psVisData->anViewPort);

	// Draw the graph axes
	if (psVisData->boDrawAxes) {
		DrawAxes (psVisData);
	}

	// Draw the graph
	glDisable (GL_CULL_FACE);
	DrawGraphs (psVisData);
	glEnable (GL_CULL_FACE);

	//glFlush ();

	//glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, gSphereSpecular);
	//glMaterialfv (GL_FRONT_AND_BACK, GL_SHININESS, gSphereShininess);
	//glMaterialfv (GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, gSphereDiffuseOn);
}

void SetLightTransformMatrix (VisPersist * psVisData) {
	//Matrix4 mPreInverse;
	Matrix4 * pmLightTransform;
	Matrix4 * pmModelShadow;
	Matrix4 * pmProjectionShadow;

	pmLightTransform = GetLightTransform (psVisData->psShadowData);
	pmModelShadow = GetShadowModel (psVisData->psShadowData);
	pmProjectionShadow = GetShadowProjection (psVisData->psShadowData);

	glGetFloatv (GL_MODELVIEW_MATRIX, pmModelShadow->afM);
	glGetFloatv (GL_PROJECTION_MATRIX, pmProjectionShadow->afM);
	MultMatrixMatrix4 (pmLightTransform, pmProjectionShadow, pmModelShadow);

	//*pmLightTransform = psVisData->mProjectionShadow;
	//Invert4 (pmLightTransform, & mPreInverse);
	//afMVPShadowInvert
}

void CalculateLightPos (VisPersist * psVisData) {
	Vector3 vSideways;

	vSideways = CrossProduct (& psVisData->vCamera, & psVisData->vUp);
	psVisData->vLightPos.fX = (psVisData->fViewRadius) * psVisData->vCamera.fX + (psVisData->vUp.fX * SHADOWSHIFT_UP) - (vSideways.fX * SHADOWSHIFT_SIDEWAYS);
	psVisData->vLightPos.fY = (psVisData->fViewRadius) * psVisData->vCamera.fY + (psVisData->vUp.fY * SHADOWSHIFT_UP) - (vSideways.fY * SHADOWSHIFT_SIDEWAYS);
	psVisData->vLightPos.fZ = (psVisData->fViewRadius) * psVisData->vCamera.fZ + (psVisData->vUp.fZ * SHADOWSHIFT_UP) - (vSideways.fZ * SHADOWSHIFT_SIDEWAYS);

	//psVisData->vLightPos.fX = (psVisData->fViewRadius) * psVisData->vCamera.fX;
	//psVisData->vLightPos.fY = (psVisData->fViewRadius) * psVisData->vCamera.fY;
	//psVisData->vLightPos.fZ = (psVisData->fViewRadius) * psVisData->vCamera.fZ;
}

void RenderShadow (VisPersist * psVisData) {
	CalculateLightPos (psVisData);
	
	glLoadIdentity ();
	gluLookAt (psVisData->vLightPos.fX, psVisData->vLightPos.fY, psVisData->vLightPos.fZ, 0.0, 0.0, 0.0, psVisData->vUp.fX, psVisData->vUp.fY, psVisData->vUp.fZ);
	//gluLookAt ((psVisData->fViewRadius) * psVisData->vCamera.fX, (psVisData->fViewRadius) * psVisData->vCamera.fY, (psVisData->fViewRadius) * psVisData->vCamera.fZ, 0.0, 0.0, 0.0, psVisData->vUp.fX, psVisData->vUp.fY, psVisData->vUp.fZ);

	glClearColor (0.0f, 0.0f, 0.0f, 1.0f);
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	SetLightTransformMatrix (psVisData);

	if (psVisData->boShadow) {
		// Draw the graph axes
		if (psVisData->boDrawAxes) {
			glDisable (GL_CULL_FACE);
			DrawAxes (psVisData);
			glEnable (GL_CULL_FACE);
		}

		// Draw the graph
		glDisable (GL_CULL_FACE);
		//glFrontFace (GL_CW);
		DrawGraphsShadow (psVisData);
		glEnable (GL_CULL_FACE);
		//glFrontFace (GL_CCW);
	}
}

void DrawAxes (VisPersist * psVisData) {
	//int nXSize;
	//int nYSize;
	//int nZSize;

	glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);

	//nXSize = (AXIS_XSIZE / SQUARE_NUM);
	//nYSize = (AXIS_YSIZE / SQUARE_NUM);
	//nZSize = (AXIS_ZSIZE / SQUARE_NUM);

	glMaterialfv (GL_FRONT, GL_SPECULAR, gBoardSpecular);
	glMaterialfv (GL_FRONT, GL_SHININESS, gBoardShininess);
	glMaterialfv (GL_FRONT, GL_AMBIENT_AND_DIFFUSE, gMatDiffuse);
	glMaterialfv (GL_FRONT, GL_AMBIENT_AND_DIFFUSE, gGridColour);

	// Draw the floor
	DrawFloor (psVisData);

	// Draw the walls
	DrawWallX (AXIS_XHSIZE, GL_CW, -1.0f, psVisData);
	DrawWallX (-AXIS_XHSIZE, GL_CCW, 1.0f, psVisData);
	DrawWallY (AXIS_YHSIZE, GL_CCW, -1.0f, psVisData);
	DrawWallY (-AXIS_YHSIZE, GL_CW, 1.0f, psVisData);

	glMaterialfv (GL_FRONT, GL_SPECULAR, gBoardSpecular);
	glMaterialfv (GL_FRONT, GL_SHININESS, gBoardShininess);
	glMaterialfv (GL_FRONT, GL_AMBIENT_AND_DIFFUSE, gGridColour);

	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
}

void DrawAxesNumbers (VisPersist * psVisData) {
	int nOctant;

	// Draw the numbers
	if (psVisData->boClearWhite) {
		glColor3f (NUMBERTEXT_COLINV);
	}
	else {
		glColor3f (NUMBERTEXT_COLOUR);
	}

	glDisable (GL_DEPTH_TEST);
	if (psVisData->fRotation < M_PI) {
		DrawNumbersX (-AXIS_YHSIZE, AXIS_ZHSIZE, psVisData);
	}
	else {
		DrawNumbersX (AXIS_YHSIZE, AXIS_ZHSIZE, psVisData);
	}

	if ((psVisData->fRotation < M_PI_2) || (psVisData->fRotation > (M_TWOPI - M_PI_2))) {
		DrawNumbersY (AXIS_XHSIZE, AXIS_ZHSIZE, psVisData);
	}
	else {
		DrawNumbersY (-AXIS_XHSIZE, AXIS_ZHSIZE, psVisData);
	}

	nOctant = (int)(psVisData->fRotation / M_PI_4);
	switch (nOctant) {
		case 1:
		case 4:
			DrawNumbersZ (AXIS_XHSIZE, AXIS_YHSIZE, psVisData);
			break;
		case 3:
		case 6:
			DrawNumbersZ (AXIS_XHSIZE, -AXIS_YHSIZE, psVisData);
			break;
		case 5:
		case 0:
			DrawNumbersZ (-AXIS_XHSIZE, -AXIS_YHSIZE, psVisData);
			break;
		case 7:
		case 2:
			DrawNumbersZ (-AXIS_XHSIZE, AXIS_YHSIZE, psVisData);
			break;
	}

	glEnable (GL_DEPTH_TEST);
}

void DrawFloor (VisPersist * psVisData) {
	double fX;
	double fY;
	double fXFirst;
	double fYFirst;
	double fXLast;
	double fYLast;

	double fXMin;
	double fYMin;
	double fXMax;
	double fYMax;
	double fXGrid;
	double fYGrid;

	fXFirst = ceil (psVisData->fXMin / psVisData->fXGrid) * psVisData->fXGrid;
	fYFirst = ceil (psVisData->fYMin / psVisData->fYGrid) * psVisData->fYGrid;

	fXMin = -AXIS_XHSIZE;
	fYMin = -AXIS_YHSIZE;
	fXGrid = ((psVisData->fXGrid / psVisData->fXWidth) * AXIS_XSIZE);
	fYGrid = ((psVisData->fYGrid / psVisData->fYWidth) * AXIS_YSIZE);
	fXMax = AXIS_XHSIZE;
	fYMax = AXIS_YHSIZE;

	fXFirst = ((((fXFirst - psVisData->fXMin) / psVisData->fXWidth) * AXIS_XSIZE) - AXIS_XHSIZE);
	fYFirst = ((((fYFirst - psVisData->fYMin) / psVisData->fYWidth) * AXIS_YSIZE) - AXIS_YHSIZE);
	fY = fYFirst;

	// Main area
	for (fX = fXFirst; fX < fXMax - fXGrid; fX += fXGrid) {
		glBegin (GL_QUAD_STRIP);
		glNormal3f (0.0f, 0.0f, 1.0f);
		for (fY = fYFirst; fY < fYMax; fY += fYGrid) {
			// Draw square without texture coordinates
			glVertex3f (fX, fY, 0.0f);
			glVertex3f (fX + fXGrid, fY, 0.0f);
		}
		glEnd ();
	}

	fXLast = fX - fXGrid;
	fYLast = fY - fYGrid;

	// Start edges
	glBegin (GL_QUAD_STRIP);
	glNormal3f (0.0f, 0.0f, 1.0f);
	for (fY = fYFirst; fY < fYMax; fY += fYGrid) {
		// Draw square without texture coordinates
		glVertex3f (fXMin, fY, 0.0f);
		glVertex3f (fXFirst, fY, 0.0f);
	}
	glEnd ();

	glBegin (GL_QUAD_STRIP);
	glNormal3f (0.0f, 0.0f, 1.0f);
	for (fX = fXFirst; fX < fXMax; fX += fXGrid) {
		// Draw square without texture coordinates
		glVertex3f (fX, fYFirst, 0.0f);
		glVertex3f (fX, fYMin, 0.0f);
	}
	glEnd ();

	// End edges
	glBegin (GL_QUAD_STRIP);
	glNormal3f (0.0f, 0.0f, 1.0f);
	for (fY = fYFirst; fY < fYMax; fY += fYGrid) {
		// Draw square without texture coordinates
		glVertex3f (fXLast, fY, 0.0f);
		glVertex3f (fXMax, fY, 0.0f);
	}
	glEnd ();

	glBegin (GL_QUAD_STRIP);
	glNormal3f (0.0f, 0.0f, 1.0f);
	for (fX = fXFirst; fX < fXMax; fX += fXGrid) {
		// Draw square without texture coordinates
		glVertex3f (fX, fYMax, 0.0f);
		glVertex3f (fX, fYLast, 0.0f);
	}
	glEnd ();

	// Corners
	glBegin (GL_QUADS);
	glNormal3f (0.0f, 0.0f, 1.0f);

	glVertex3f (fXMin, fYMin, 0.0f);
	glVertex3f (fXFirst, fYMin, 0.0f);
	glVertex3f (fXFirst, fYFirst, 0.0f);
	glVertex3f (fXMin, fYFirst, 0.0f);

	glVertex3f (fXLast, fYMin, 0.0f);
	glVertex3f (fXMax, fYMin, 0.0f);
	glVertex3f (fXMax, fYFirst, 0.0f);
	glVertex3f (fXLast, fYFirst, 0.0f);

	glVertex3f (fXMin, fYLast, 0.0f);
	glVertex3f (fXFirst, fYLast, 0.0f);
	glVertex3f (fXFirst, fYMax, 0.0f);
	glVertex3f (fXMin, fYMax, 0.0f);

	glVertex3f (fXLast, fYLast, 0.0f);
	glVertex3f (fXMax, fYLast, 0.0f);
	glVertex3f (fXMax, fYMax, 0.0f);
	glVertex3f (fXLast, fYMax, 0.0f);

	glEnd ();
}

void DrawNumbersX (double fY, double fZ, VisPersist * psVisData) {
	double fX;
	double fXFirst;
	//double fXMin;
	double fXMax;
	double fXGrid;
	char szNumber[NUMBER_MAX];
	double fNumber;

	fNumber = ceil (psVisData->fXMin / psVisData->fXGrid) * psVisData->fXGrid;

	//fXMin = -AXIS_XHSIZE;
	fXGrid = ((psVisData->fXGrid / psVisData->fXWidth) * AXIS_XSIZE);
	fXMax = AXIS_XHSIZE;

	fXFirst = ((((fNumber - psVisData->fXMin) / psVisData->fXWidth) * AXIS_XSIZE) - AXIS_XHSIZE);

	for (fX = fXFirst; fX < fXMax; fX += fXGrid) {
		snprintf (szNumber, NUMBER_MAX, "%g", fNumber);
		RenderTextInSpace (szNumber, fX, fY, fZ);
		fNumber += psVisData->fXGrid;
	}

	//RenderTextInSpace ("Yo!", fXMin, fY, fZ);
	//RenderTextInSpace ("Yo!", fXMax, fY, fZ);
}

void DrawNumbersY (double fX, double fZ, VisPersist * psVisData) {
	double fY;
	double fYFirst;
	//double fYMin;
	double fYMax;
	double fYGrid;
	char szNumber[NUMBER_MAX];
	double fNumber;

	fNumber = ceil (psVisData->fYMin / psVisData->fYGrid) * psVisData->fYGrid;

	//fYMin = -AXIS_YHSIZE;
	fYGrid = ((psVisData->fYGrid / psVisData->fYWidth) * AXIS_YSIZE);
	fYMax = AXIS_YHSIZE;

	fYFirst = ((((fNumber - psVisData->fYMin) / psVisData->fYWidth) * AXIS_YSIZE) - AXIS_YHSIZE);

	for (fY = fYFirst; fY < fYMax; fY += fYGrid) {
		snprintf (szNumber, NUMBER_MAX, "%g", fNumber);
		RenderTextInSpace (szNumber, fX, fY, fZ);
		fNumber += psVisData->fYGrid;
	}

	//RenderTextInSpace ("Yo!", fX, fYMin, fZ);
	//RenderTextInSpace ("Yo!", fX, fYMax, fZ);
}

void DrawNumbersZ (double fX, double fY, VisPersist * psVisData) {
	double fZ;
	double fZFirst;
	//double fZMin;
	double fZMax;
	double fZGrid;
	char szNumber[NUMBER_MAX];
	double fNumber;

	fNumber = ceil (psVisData->fZMin / psVisData->fZGrid) * psVisData->fZGrid;

	//fZMin = -AXIS_ZHSIZE;
	fZGrid = ((psVisData->fZGrid / psVisData->fZWidth) * AXIS_ZSIZE);
	fZMax = AXIS_ZHSIZE;

	fZFirst = ((((fNumber - psVisData->fZMin) / psVisData->fZWidth) * AXIS_ZSIZE) - AXIS_ZHSIZE);

	for (fZ = fZFirst; fZ < fZMax; fZ += fZGrid) {
		snprintf (szNumber, NUMBER_MAX, "%g", fNumber);
		RenderTextInSpace (szNumber, fX, fY, fZ);
		fNumber += psVisData->fZGrid;
	}

	//RenderTextInSpace ("Yo!", fX, fY, fZMin);
	//RenderTextInSpace ("Yo!", fX, fY, fZMax);
}

void DrawWallY (double fY, unsigned int uCullOrient, double fNormal, VisPersist * psVisData) {
	double fX;
	double fZ;
	double fXFirst;
	double fZFirst;
	double fXLast;
	double fZLast;

	double fXMin;
	double fZMin;
	double fXMax;
	double fZMax;
	double fXGrid;
	double fZGrid;

	fXFirst = ceil (psVisData->fXMin / psVisData->fXGrid) * psVisData->fXGrid;
	fZFirst = ceil (psVisData->fZMin / psVisData->fZGrid) * psVisData->fZGrid;

	fXMin = -AXIS_XHSIZE;
	fZMin = -AXIS_ZHSIZE;
	fXGrid = ((psVisData->fXGrid / psVisData->fXWidth) * AXIS_XSIZE);
	fZGrid = ((psVisData->fZGrid / psVisData->fZWidth) * AXIS_ZSIZE);
	fXMax = AXIS_XHSIZE;
	fZMax = AXIS_ZHSIZE;

	fXFirst = ((((fXFirst - psVisData->fXMin) / psVisData->fXWidth) * AXIS_XSIZE) - AXIS_XHSIZE);
	fZFirst = ((((fZFirst - psVisData->fZMin) / psVisData->fZWidth) * AXIS_ZSIZE) - AXIS_ZHSIZE);
	fZ = fZFirst;

	glFrontFace (uCullOrient);

	// Main area
	for (fX = fXFirst; fX < fXMax - fXGrid; fX += fXGrid) {
		glBegin (GL_QUAD_STRIP);
		glNormal3f (0.0f, fNormal, 0.0f);
		for (fZ = fZFirst; fZ < fZMax; fZ += fZGrid) {
			// Draw square without texture coordinates
			glVertex3f (fX, fY, fZ);
			glVertex3f (fX + fXGrid, fY, fZ);
		}
		glEnd ();
	}

	fXLast = fX - fXGrid;
	fZLast = fZ - fZGrid;

	// Start edges
	glBegin (GL_QUAD_STRIP);
	glNormal3f (0.0f, fNormal, 0.0f);
	for (fZ = fZFirst; fZ < fZMax; fZ += fZGrid) {
		// Draw square without texture coordinates
		glVertex3f (fXMin, fY, fZ);
		glVertex3f (fXFirst, fY, fZ);
	}
	glEnd ();

	glBegin (GL_QUAD_STRIP);
	glNormal3f (0.0f, fNormal, 0.0f);
	for (fX = fXFirst; fX < fXMax; fX += fXGrid) {
		// Draw square without texture coordinates
		glVertex3f (fX, fY, fZFirst);
		glVertex3f (fX, fY, fZMin);
	}
	glEnd ();

	// End edges
	glBegin (GL_QUAD_STRIP);
	glNormal3f (0.0f, fNormal, 0.0f);
	for (fZ = fZFirst; fZ < fZMax; fZ += fZGrid) {
		// Draw square without texture coordinates
		glVertex3f (fXLast, fY, fZ);
		glVertex3f (fXMax, fY, fZ);
	}
	glEnd ();

	glBegin (GL_QUAD_STRIP);
	glNormal3f (0.0f, fNormal, 0.0f);
	for (fX = fXFirst; fX < fXMax; fX += fXGrid) {
		// Draw square without texture coordinates
		glVertex3f (fX, fY, fZMax);
		glVertex3f (fX, fY, fZLast);
	}
	glEnd ();

	// Corners
	glBegin (GL_QUADS);
	glNormal3f (0.0f, fNormal, 0.0f);

	glVertex3f (fXMin, fY, fZMin);
	glVertex3f (fXFirst, fY, fZMin);
	glVertex3f (fXFirst, fY, fZFirst);
	glVertex3f (fXMin, fY, fZFirst);

	glVertex3f (fXLast, fY, fZMin);
	glVertex3f (fXMax, fY, fZMin);
	glVertex3f (fXMax, fY, fZFirst);
	glVertex3f (fXLast, fY, fZFirst);

	glVertex3f (fXMin, fY, fZLast);
	glVertex3f (fXFirst, fY, fZLast);
	glVertex3f (fXFirst, fY, fZMax);
	glVertex3f (fXMin, fY, fZMax);

	glVertex3f (fXLast, fY, fZLast);
	glVertex3f (fXMax, fY, fZLast);
	glVertex3f (fXMax, fY, fZMax);
	glVertex3f (fXLast, fY, fZMax);

	glEnd ();

	glFrontFace (GL_CCW);
}

void DrawWallX (double fX, unsigned int uCullOrient, double fNormal, VisPersist * psVisData) {
	double fY;
	double fZ;
	double fYFirst;
	double fZFirst;
	double fYLast;
	double fZLast;

	double fYMin;
	double fZMin;
	double fYMax;
	double fZMax;
	double fYGrid;
	double fZGrid;

	fYFirst = ceil (psVisData->fYMin / psVisData->fYGrid) * psVisData->fYGrid;
	fZFirst = ceil (psVisData->fZMin / psVisData->fZGrid) * psVisData->fZGrid;

	fYMin = -AXIS_YHSIZE;
	fZMin = -AXIS_ZHSIZE;
	fYGrid = ((psVisData->fYGrid / psVisData->fYWidth) * AXIS_YSIZE);
	fZGrid = ((psVisData->fZGrid / psVisData->fZWidth) * AXIS_ZSIZE);
	fYMax = AXIS_YHSIZE;
	fZMax = AXIS_ZHSIZE;

	fYFirst = ((((fYFirst - psVisData->fYMin) / psVisData->fYWidth) * AXIS_YSIZE) - AXIS_YHSIZE);
	fZFirst = ((((fZFirst - psVisData->fZMin) / psVisData->fZWidth) * AXIS_ZSIZE) - AXIS_ZHSIZE);
	fZ = fZFirst;

	glFrontFace (uCullOrient);

	// Main area
	for (fY = fYFirst; fY < fYMax - fYGrid; fY += fYGrid) {
		glBegin (GL_QUAD_STRIP);
		glNormal3f (fNormal, 0.0f, 0.0f);
		for (fZ = fZFirst; fZ < fZMax; fZ += fZGrid) {
			// Draw square without texture coordinates
			glVertex3f (fX, fY, fZ);
			glVertex3f (fX, fY + fYGrid, fZ);
		}
		glEnd ();
	}

	fYLast = fY - fYGrid;
	fZLast = fZ - fZGrid;

	// Start edges
	glBegin (GL_QUAD_STRIP);
	glNormal3f (fNormal, 0.0f, 0.0f);
	for (fZ = fZFirst; fZ < fZMax; fZ += fZGrid) {
		// Draw square without texture coordinates
		glVertex3f (fX, fYMin, fZ);
		glVertex3f (fX, fYFirst, fZ);
	}
	glEnd ();

	glBegin (GL_QUAD_STRIP);
	glNormal3f (fNormal, 0.0f, 0.0f);
	for (fY = fYFirst; fY < fYMax; fY += fYGrid) {
		// Draw square without texture coordinates
		glVertex3f (fX, fY, fZFirst);
		glVertex3f (fX, fY, fZMin);
	}
	glEnd ();

	// End edges
	glBegin (GL_QUAD_STRIP);
	glNormal3f (fNormal, 0.0f, 0.0f);
	for (fZ = fZFirst; fZ < fZMax; fZ += fZGrid) {
		// Draw square without texture coordinates
		glVertex3f (fX, fYLast, fZ);
		glVertex3f (fX, fYMax, fZ);
	}
	glEnd ();

	glBegin (GL_QUAD_STRIP);
	glNormal3f (fNormal, 0.0f, 0.0f);
	for (fY = fYFirst; fY < fYMax; fY += fYGrid) {
		// Draw square without texture coordinates
		glVertex3f (fX, fY, fZMax);
		glVertex3f (fX, fY, fZLast);
	}
	glEnd ();

	// Corners
	glBegin (GL_QUADS);
	glNormal3f (fNormal, 0.0f, 0.0f);

	glVertex3f (fX, fYMin, fZMin);
	glVertex3f (fX, fYFirst, fZMin);
	glVertex3f (fX, fYFirst, fZFirst);
	glVertex3f (fX, fYMin, fZFirst);

	glVertex3f (fX, fYLast, fZMin);
	glVertex3f (fX, fYMax, fZMin);
	glVertex3f (fX, fYMax, fZFirst);
	glVertex3f (fX, fYLast, fZFirst);

	glVertex3f (fX, fYMin, fZLast);
	glVertex3f (fX, fYFirst, fZLast);
	glVertex3f (fX, fYFirst, fZMax);
	glVertex3f (fX, fYMin, fZMax);

	glVertex3f (fX, fYLast, fZLast);
	glVertex3f (fX, fYMax, fZLast);
	glVertex3f (fX, fYMax, fZMax);
	glVertex3f (fX, fYLast, fZMax);

	glEnd ();

	glFrontFace (GL_CCW);
}

void DrawGraphs (VisPersist * psVisData) {
	GSList * psFuncList;

	glColor4f (1.0f, 1.0f, 1.0f, 1.0f);

	if (psVisData->boWireframe) {
		// Turn on wireframe
		glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
	}

	// Render the graphs
	psFuncList = psVisData->psFuncList;
	while (psFuncList) {
		DrawGraph ((FuncPersist *)(psFuncList->data));

		psFuncList = g_slist_next (psFuncList);
	}

	if (psVisData->boWireframe) {
		// Turn off wireframe
		glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	}
}

void DrawGraphsShadow (VisPersist * psVisData) {
	GSList * psFuncList;

	glColor4f (1.0f, 1.0f, 1.0f, 1.0f);

	if (psVisData->boWireframe) {
		// Turn on wireframe
		glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
	}

	// Render the graphs
	psFuncList = psVisData->psFuncList;
	while (psFuncList) {
		DrawGraphShadow ((FuncPersist *)(psFuncList->data));

		psFuncList = g_slist_next (psFuncList);
	}

	if (psVisData->boWireframe) {
		// Turn off wireframe
		glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	}
}

void DrawTextOverlay (VisPersist * psVisData) {
	GString * szString;
	//int nStringWidth;

	szString = g_string_new ("");

	glDisable (GL_LIGHTING);
	if (psVisData->boClearWhite) {
		glColor3f (0.0, 0.0, 0.0);
	}
	else {
		glColor3f (1.0, 1.0, 1.0);
	}

	g_string_printf (szString, "Hello!");

	//nStringWidth = BitmapStringWidth (GLUT_BITMAP_HELVETICA_18, szString->str);
	//RenderBitmapString (psVisData->nScreenWidth - nStringWidth - 16.0, psVisData->nScreenHeight - 32, GLUT_BITMAP_HELVETICA_18, szString->str);

	glEnable (GL_LIGHTING);

	g_string_free (szString, TRUE);
}

void WindowPos2f (GLfloat fX, GLfloat fY) {
	GLfloat fXn, fYn;

	glPushAttrib (GL_TRANSFORM_BIT | GL_VIEWPORT_BIT);
	glMatrixMode (GL_PROJECTION);
	glPushMatrix ();
	glLoadIdentity ();
	glMatrixMode (GL_MODELVIEW);
	glPushMatrix ();
	glLoadIdentity ();

	glDepthRange (0, 0);
	glViewport ((int) fX - 1, (int) fY - 1, 2, 2);
	fXn = fX - (int) fX;
	fYn = fY - (int) fY;
	glRasterPos4f (fXn, fYn, 0.0, 1);
	glPopMatrix ();
	glMatrixMode (GL_PROJECTION);
	glPopMatrix ();

	glPopAttrib ();
}

void RenderBitmapString (float fX, float fY, void * pFont, char const * szString) {
	int nPos;

	WindowPos2f (fX, fY);
	nPos = 0;
	while (szString[nPos] >= 32) {
		glutBitmapCharacter (pFont, szString[nPos]);
		nPos++;
	}
}

void RenderCursor (float fX, float fY, void * pFont, char const * szString, int nCursorPos) {
	int nPos;
	struct timeb sTime;

	ftime (& sTime);

	if (sTime.millitm < 700) {
		nPos = 0;
		while ((nPos < nCursorPos) && (szString[nPos] >= 32)) {
			fX += glutBitmapWidth (pFont, szString[nPos]);
			nPos++;
		}

		WindowPos2f (fX, fY);
		glutBitmapCharacter (pFont, '_');
		glutBitmapWidth (pFont, szString[nPos]);
	}
}

int BitmapStringWidth (void * pFont, char const * szString) {
	int nWidth = 0;
	int nPos = 0;
	while (szString[nPos] >= 32) {
		nWidth += glutBitmapWidth (pFont, szString[nPos]);
		nPos++;
	}

	return nWidth;
}

bool PointTowards (float fXPos, float fYPos, float fZPos, float fRadius, VisPersist * psVisData) {
	float fTheta;
	float fPhi;
	float fPsi;

	float fX;
	float fY;
	float fZ;
	float fXp;
	float fYp;
	float fZp;
	float fXn;
	float fYn;
	float fZn;
	bool boArrived = true;

	fXp = (-MGL_WIDTH/2.0f + (float)fXPos) / MGL_SCALE;
	fYp = POINTTOELLEVATION + (MGL_HEIGHT/2.0f - (float)fYPos) / MGL_SCALE;
	fZp = (-MGL_DEPTH/2.0f + (float)fZPos) / MGL_SCALE;

	fTheta = DotProdAngle (fXp, fZp, psVisData->vCamera.fX, psVisData->vCamera.fZ) * psVisData->fHalfLife;
	if (absf (fTheta) > POINTTOMINMOVE) {
		fX = (psVisData->vCamera.fX * cos (fTheta)) + (psVisData->vCamera.fZ * sin (fTheta));
		fZ = - (psVisData->vCamera.fX * sin (fTheta)) + (psVisData->vCamera.fZ * cos (fTheta));
		fXn = (psVisData->vUp.fX * cos (fTheta)) + (psVisData->vUp.fZ * sin (fTheta));
		fZn = - (psVisData->vUp.fX * sin (fTheta)) + (psVisData->vUp.fZ * cos (fTheta));
		psVisData->vCamera.fX = fX;
		psVisData->vCamera.fZ = fZ;
		psVisData->vUp.fX = fXn;
		psVisData->vUp.fZ = fZn;
		boArrived = false;
	}

	fPhi = DotProdAngle (fYp, fZp, psVisData->vCamera.fY, psVisData->vCamera.fZ) * psVisData->fHalfLife;
	if (absf (fPhi) > POINTTOMINMOVE) {
		fY = (psVisData->vCamera.fY * cos (fPhi)) + (psVisData->vCamera.fZ * sin (fPhi));
		fZ = - (psVisData->vCamera.fY * sin (fPhi)) + (psVisData->vCamera.fZ * cos (fPhi));
		fYn = (psVisData->vUp.fY * cos (fPhi)) + (psVisData->vUp.fZ * sin (fPhi));
		fZn = - (psVisData->vUp.fY * sin (fPhi)) + (psVisData->vUp.fZ * cos (fPhi));
		psVisData->vCamera.fY = fY;
		psVisData->vCamera.fZ = fZ;
		psVisData->vUp.fY = fYn;
		psVisData->vUp.fZ = fZn;
		boArrived = false;
	}

	fPsi = DotProdAngle (fXp, fYp, psVisData->vCamera.fX, psVisData->vCamera.fY) * psVisData->fHalfLife;
	if (absf (fPsi) > POINTTOMINMOVE) {
		fX = (psVisData->vCamera.fX * cos (fPsi)) + (psVisData->vCamera.fY * sin (fPsi));
		fY = - (psVisData->vCamera.fX * sin (fPsi)) + (psVisData->vCamera.fY * cos (fPsi));
		fXn = (psVisData->vUp.fX * cos (fPsi)) + (psVisData->vUp.fY * sin (fPsi));
		fYn = - (psVisData->vUp.fX * sin (fPsi)) + (psVisData->vUp.fY * cos (fPsi));
		psVisData->vCamera.fX = fX;
		psVisData->vCamera.fY = fY;
		psVisData->vUp.fX = fXn;
		psVisData->vUp.fY = fYn;
		boArrived = false;
	}

	//psVisData->vCamera.fY = (psVisData->vCamera.fY * cos (fPhi)) + (psVisData->vCamera.fZ * sin (fPhi));
	//psVisData->vCamera.fZ = - (psVisData->vCamera.fY * sin (fPhi)) + (psVisData->vCamera.fZ * cos (fPhi));

	//psVisData->vCamera.fX = (psVisData->vCamera.fX * cos (fPsi)) + (psVisData->vCamera.fY * sin (fPsi));
	//psVisData->vCamera.fY = - (psVisData->vCamera.fX * sin (fPsi)) + (psVisData->vCamera.fY * cos (fPsi));

	Normalise3f (& psVisData->vCamera.fX, & psVisData->vCamera.fY, & psVisData->vCamera.fZ);

	return boArrived;
}

void Idle (VisPersist * psVisData) {
	struct timeb sTime;
	//float fTimeDelta;

	psVisData->fPrevTime = psVisData->fCurrentTime;
	ftime (& sTime);
	psVisData->fCurrentTime = (double)(sTime.time) + (double)(sTime.millitm) / 1000.0;
	if (psVisData->fCurrentTime < 0) psVisData->fCurrentTime = 0.0;
	//fTimeDelta = psVisData->fCurrentTime - psVisData->fPrevTime;

	KeyIdle (psVisData);

	MomentumSpin (psVisData);

	MomentumFunction (psVisData);

	AnimateFunction (psVisData);

	if ((psVisData->boSpin) && (!psVisData->boMoving)) {
		Spin (psVisData);
	}

	//glutPostRedisplay();
}

void Reshape (int nWidth, int nHeight, VisPersist * psVisData) {
	psVisData->nScreenWidth = nWidth;
	psVisData->nScreenHeight = nHeight;

	if (!psVisData->boFullScreen) {
		psVisData->nPrevScreenWidth = psVisData->nScreenWidth;
		psVisData->nPrevScreenHeight = psVisData->nScreenHeight;
	}
	ShadowSetScreenDimensions (psVisData->nScreenWidth, psVisData->nScreenHeight, psVisData->psShadowData);

	FramebufferResize (psVisData);

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	glViewport(0, 0, nWidth, nHeight);
	gluPerspective (60, (float)nWidth / (float)nHeight, 1, 100);
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();

	//gluLookAt ((psVisData->fViewRadius) * psVisData->vCamera.fX, (psVisData->fViewRadius) * psVisData->vCamera.fY, (psVisData->fViewRadius) * psVisData->vCamera.fZ, 0.0, 0.0, 0.0, psVisData->vUp.fX, psVisData->vUp.fY, psVisData->vUp.fZ);

	gluLookAt (0.0f, 25.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
}

void Redraw (VisPersist * psVisData) {
	if (psVisData->boShader) {	
		// Render to the shadow buffer
		RenderToShadowBuffer (psVisData);
	}
	glClearColor (0.0f, 0.0f, 0.0f, 0.0f);
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glPushMatrix ();
	RenderShadow (psVisData);
	glPopMatrix ();

	if (psVisData->boShader) {	
		// Render to the framebuffer
		RenderToFrameBuffer (psVisData);
	}

	if (psVisData->boClearWhite) {
		glClearColor (1.0f, 1.0f, 1.0f, 0.0f);
	}
	else {
		glClearColor (0.0f, 0.0f, 0.0f, 0.0f);
	}
	// No stencil buffer in use for this application
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glPushMatrix ();

	Render (psVisData);

	glPopMatrix ();

	if (psVisData->boShader) {
		// Finally render to the screenbuffer
		RenderToScreenBuffer (psVisData);

		// Render the framebuffer to screen
		RenderFramebufferToScreen (psVisData);
	}

	// Render the debug texture overlay to the screen
	//RenderDebugOverlay (psVisData);
	glLoadIdentity ();
	gluLookAt ((psVisData->fViewRadius) * psVisData->vCamera.fX, (psVisData->fViewRadius) * psVisData->vCamera.fY, (psVisData->fViewRadius) * psVisData->vCamera.fZ, 0.0, 0.0, 0.0, psVisData->vUp.fX, psVisData->vUp.fY, psVisData->vUp.fZ);

	DrawTextOverlay (psVisData);
	if (psVisData->boDrawAxes) {
		DrawAxesNumbers (psVisData);
	}

	glFlush ();
	//glutSwapBuffers();
}

void Mouse (int button, int state, int x, int y, VisPersist * psVisData) {
	//BPiece * psComparePiece;
	//GSList * psListItem;
	GLfloat z;
	double fX;
	double fY;
	double fZ;

	if (button == LEFT_BUTTON && state == BUTTON_DOWN) {
		psVisData->nXMouse = x;
		psVisData->nYMouse = y;

		psVisData->boMoving = TRUE;
		psVisData->fMomentum = 0.0f;
		psVisData->fXMomentum = 0.0f;
		psVisData->fYMomentum = 0.0f;
		psVisData->fZMomentum = 0.0f;
	}

	if (button == LEFT_BUTTON && state == BUTTON_UP) {
		psVisData->boMoving = FALSE;
		psVisData->fMomentum = 1.0f;
	}

	if (button == RIGHT_BUTTON && state == BUTTON_DOWN) {
		z = SelectFunction (x, y, psVisData);
		ConvertCoords ((GLdouble)x, (GLdouble)y, (GLdouble)z, & fX, & fY, & fZ, psVisData);

		psVisData->fXMouseFunction = fX;
		psVisData->fYMouseFunction = fY;

		psVisData->boMovingFunction = TRUE;
		psVisData->fMomentumFunction = 0.0f;
		psVisData->fXMomentumFunction = 0.0f;
		psVisData->fYMomentumFunction = 0.0f;
		psVisData->fZMomentumFunction = 0.0f;
	}

	if (button == RIGHT_BUTTON && state == BUTTON_UP) {
		psVisData->boMovingFunction = FALSE;
		psVisData->fMomentumFunction = 1.0f;
	}

}

void ConvertCoords (GLdouble fXMousePos, GLdouble fYMousePos, float fZPos, double * pfX, double * pfY, double * pfZ, VisPersist * psVisData) {
	GLdouble fX;
	GLdouble fY;
	GLdouble fZ;

	gluUnProject (fXMousePos, ((GLdouble)psVisData->anViewPort[3]) - fYMousePos, fZPos, psVisData->afModel, psVisData->afProjection, psVisData->anViewPort, & fX, & fY, & fZ);
	if (pfX) {
		*pfX = (fX + AXIS_XHSIZE) / AXIS_XSIZE;
	}
	if (pfY) {
		*pfY = (fY + AXIS_YHSIZE) / AXIS_YSIZE;
	}
	if (pfZ) {
		*pfZ = (fZ + AXIS_ZHSIZE) / AXIS_ZSIZE;
	}
}

GLfloat SelectFunction (int nXPos, int nYPos, VisPersist * psVisData) {
	GLint anViewPort[4];
	GLfloat fZPos = 0.0;

	glClear (GL_DEPTH_BUFFER_BIT);

	glGetIntegerv (GL_VIEWPORT, anViewPort);

	glMatrixMode (GL_PROJECTION);
	glPushMatrix ();
	glLoadIdentity ();
	gluPickMatrix (nXPos, anViewPort[3] - nYPos, 1, 1, anViewPort);

	gluPerspective (60, (double)psVisData->nScreenWidth / (double)psVisData->nScreenHeight, 1, 100);
	glMatrixMode (GL_MODELVIEW);

	glPushMatrix ();
	glLoadIdentity ();

	gluLookAt ((psVisData->fViewRadius) * psVisData->vCamera.fX, (psVisData->fViewRadius) * psVisData->vCamera.fY, (psVisData->fViewRadius) * psVisData->vCamera.fZ, 0.0, 0.0, 0.0, psVisData->vUp.fX, psVisData->vUp.fY, psVisData->vUp.fZ);

	// Draw the floor
	glBegin (GL_QUADS);
	glVertex3f (-AXIS_XHSIZE * 100, -AXIS_YHSIZE * 100, 0.0f);
	glVertex3f (AXIS_XHSIZE * 100, -AXIS_YHSIZE * 100, 0.0f);
	glVertex3f (AXIS_XHSIZE * 100, AXIS_YHSIZE * 100, 0.0f);
	glVertex3f (-AXIS_XHSIZE * 100, AXIS_YHSIZE * 100, 0.0f);
	glEnd ();

	glMatrixMode (GL_PROJECTION);
	glPopMatrix ();
	glMatrixMode (GL_MODELVIEW);
	glPopMatrix ();
	glFlush ();

	glReadPixels (nXPos, anViewPort[3] - nYPos, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, & fZPos);

	return fZPos;
}

void MomentumSpin (VisPersist * psVisData) {
	//if (psVisData->boMoving) {
	//	psVisData->fXMomentum = 0.0f;
	//	psVisData->fYMomentum = 0.0f;
	//	psVisData->fZMomentum = 0.0f;
	//}
	if (psVisData->fMomentum > MOMENTUM_MIN) {
		ChangeView ((psVisData->fXMomentum * psVisData->fMomentum), (psVisData->fYMomentum * psVisData->fMomentum),(psVisData->fZMomentum * psVisData->fMomentum), 0.0f, psVisData);

		psVisData->fMomentum *= (pow (MOMENTUM_RES, 100.0 * (psVisData->fCurrentTime - psVisData->fPrevTime)));
	}
}

void MomentumFunction (VisPersist * psVisData) {
	//if (psVisData->boMoving) {
	//	psVisData->fXMomentum = 0.0f;
	//	psVisData->fYMomentum = 0.0f;
	//	psVisData->fZMomentum = 0.0f;
	//}
	if (psVisData->fMomentumFunction > MOMENTUMFUNC_MIN) {

		ShuntFunctionPosition (psVisData->fXMomentumFunction * psVisData->fMomentumFunction, psVisData->fYMomentumFunction * psVisData->fMomentumFunction, 0.0, psVisData);

		psVisData->fMomentumFunction *= (pow (MOMENTUMFUNC_RES, 100.0 * (psVisData->fCurrentTime - psVisData->fPrevTime)));
	}
}

void ToggleFullScreen (VisPersist * psVisData) {
	//if (psVisData->boFullScreen) {
	//	glutReshapeWindow (psVisData->nPrevScreenWidth, psVisData->nPrevScreenHeight);
	//}
	//else {
	//	glutFullScreen ();
	//}
	psVisData->boFullScreen = !psVisData->boFullScreen;
}

bool GetFullScreen (VisPersist * psVisData) {
	return psVisData->boFullScreen;
}

void SetFullScreen (bool boFullScreen, VisPersist * psVisData) {
	psVisData->boFullScreen = boFullScreen;
}

void ToggleClearWhite (VisPersist * psVisData) {
	psVisData->boClearWhite = !psVisData->boClearWhite;
}

void ToggleSpin (VisPersist * psVisData) {
	psVisData->boSpin = !psVisData->boSpin;
}

void ToggleShaders (VisPersist * psVisData) {
	SetShadersActive (!psVisData->boShader, psVisData);
}

void ToggleWireframe (VisPersist * psVisData) {
	psVisData->boWireframe = !psVisData->boWireframe;
}

void ToggleDrawAxes (VisPersist * psVisData) {
	psVisData->boDrawAxes = !psVisData->boDrawAxes;
}

void SetClearWhite (bool boClearWhite, VisPersist * psVisData) {
	psVisData->boClearWhite = boClearWhite;
}

bool GetClearWhite (VisPersist * psVisData) {
	return psVisData->boClearWhite;
}

void SetSpin (bool boSpin, VisPersist * psVisData) {
	psVisData->boSpin = boSpin;
}

bool GetSpin (VisPersist * psVisData) {
	return psVisData->boSpin;
}

void SetDrawAxes (bool boDrawAxes, VisPersist * psVisData) {
	psVisData->boDrawAxes = boDrawAxes;
}

bool GetDrawAxes (VisPersist * psVisData) {
	return psVisData->boDrawAxes;
}

void SetWireframe (bool boWireframe, VisPersist * psVisData) {
	psVisData->boWireframe = boWireframe;
}

bool GetWireframe (VisPersist * psVisData) {
	return psVisData->boWireframe;
}

bool GetShadersActive (VisPersist * psVisData) {
	return psVisData->boShader;
}

void SetShadersActive (bool boShaderActive, VisPersist * psVisData) {
	GSList * psFuncList;

	if (boShaderActive != psVisData->boShader) {
		psVisData->boShader = boShaderActive;

		// Cycle through all of the functions
		psFuncList = psVisData->psFuncList;
		while (psFuncList) {
			SetFunctionShaderActive (psVisData->boShader, (FuncPersist *)(psFuncList->data));

			psFuncList = g_slist_next (psFuncList);
		}
	}
}

void SetShadow (bool boShadow, VisPersist * psVisData) {
	psVisData->boShadow = boShadow;
}

bool GetShadow (VisPersist * psVisData) {
	return psVisData->boShadow;
}

void SetFocusBlur (bool boFocusBlur, VisPersist * psVisData) {
	psVisData->boFocusBlur = boFocusBlur;
}

bool GetFocusBlur (VisPersist * psVisData) {
	return psVisData->boFocusBlur;
}

void SetFocusBlurNear (float fFocusBlurNear, VisPersist * psVisData) {
	psVisData->fFocusBlurNear = fFocusBlurNear;
}

float GetFocusBlurNear (VisPersist * psVisData) {
	return psVisData->fFocusBlurNear;
}

void SetFocusBlurFar (float fFocusBlurFar, VisPersist * psVisData) {
	psVisData->fFocusBlurFar = fFocusBlurFar;
}

float GetFocusBlurFar (VisPersist * psVisData) {
	return psVisData->fFocusBlurFar;
}

void GetDisplayProperties (float * pfViewRadius, float * pfLinkLen, float * pfCentring, float * pfRigidity, float * pfForce, float * pfResistance, VisPersist * psVisData) {
	if (pfViewRadius) {
		* pfViewRadius = psVisData->fViewRadius;
	}
}

void SetDisplayProperties (float fViewRadius, float fLinkLen, float fCentring, float fRigidity, float fForce, float fResistance, VisPersist * psVisData) {
	psVisData->fViewRadius = fViewRadius;

	RANGE (psVisData->fViewRadius, 5.0, 80.0)
}

void Key (unsigned int key, int x, int y, unsigned int uKeyModifiers, VisPersist * psVisData) {
	//GLint nKeyModifiers;
	//nKeyModifiers = glutGetModifiers ();

	if (key < MAXKEYS) {
		psVisData->aboKeyDown[key] = true;
	}

	switch (key) {
	//case '\r':
	//	if (uKeyModifiers & GLUT_ACTIVE_ALT) ToggleFullScreen (VisPersist * psVisData);
	//	break;
	case 'q':
		gtk_main_quit ();
		break;
	case 'i':
		ToggleClearWhite (psVisData);
		break;
	case 'w':
		ToggleWireframe (psVisData);
		break;
	case 'a':
		ToggleDrawAxes (psVisData);
		break;
	case 'r':
		ResetAnimation (psVisData);
		break;
	case 'o':
		ToggleSpin (psVisData);
		break;
	case 'm':
		break;
	case 's':
		ToggleShaders (psVisData);
		break;
	case ' ':
		SetPauseAnimation ((!psVisData->boPaused), psVisData);
		break;
	case GDK_Up:
		break;
	case GDK_Down:
		break;
	case GDK_Return:
		break;
	}
}

void KeyUp (unsigned int key, int x, int y, unsigned int uKeyModifiers, VisPersist * psVisData) {
	if (key < MAXKEYS) {
		psVisData->aboKeyDown[key] = false;
	}

	switch (key) {
		case 's':
			// Do nothing
			break;
	}
}

void KeyIdle (VisPersist * psVisData) {
	if (psVisData->aboKeyDown['=']) ChangeView (0.0f, 0.0f, 0.0f, -RADIUSSTEP, psVisData);
	if (psVisData->aboKeyDown['+']) ChangeView (0.0f, 0.0f, 0.0f, -RADIUSSTEP, psVisData);
	if (psVisData->aboKeyDown['-']) ChangeView (0.0f, 0.0f, 0.0f, +RADIUSSTEP, psVisData);
	if (psVisData->aboKeyDown['_']) ChangeView (0.0f, 0.0f, 0.0f, +RADIUSSTEP, psVisData);
	//if (psVisData->aboKeyDown[',']) ChangeView (0.0f, 0.0f, +PSISTEP, 0.0f, psVisData);
	//if (psVisData->aboKeyDown['.']) ChangeView (0.0f, 0.0f, -PSISTEP, 0.0f, psVisData);

	//if (gboKeyLeft) {
	//	gvVel.fX -= SPHERESPEED;
	//	gvVel.fX *= SPHEREAIRRES;
	//}
	//if (gboKeyRight) {
	//	gvVel.fX += SPHERESPEED;
	//	gvVel.fX *= SPHEREAIRRES;
	//}
	//if (gboKeyUp) {
	//	gvVel.fY += SPHERESPEED;
	//	gvVel.fY *= SPHEREAIRRES;
	//}
	//if (gboKeyDown) {
	//	gvVel.fY -= SPHERESPEED;
	//	gvVel.fY *= SPHEREAIRRES;
	//}

	//if (gboMouseRight) {
	//	gfVVel[(int)(WIDTH / 2) + 2][(int)(HEIGHT / 2) - 3] += STRENGTH;
	//}
}

/*
void ChangeView (float fTheta, float fPhi, float fPsi, float fRadius, VisPersist * psVisData) {
	float fA;
	float fB;
	float fX;
	float fY;
	float fZ;
	float fXn;
	float fYn;
	float fZn;

	float fXv;
	float fYv;
	float fZv;

	psVisData->fViewRadius += fRadius;

	// Phi
	fA = cos (fPhi);
	fB = sin (fPhi);

	fX = (fA * psVisData->vCamera.fX) + (fB * psVisData->vUp.fX);
	fY = (fA * psVisData->vCamera.fY) + (fB * psVisData->vUp.fY);
	fZ = (fA * psVisData->vCamera.fZ) + (fB * psVisData->vUp.fZ);

	fXn = - (fB * psVisData->vCamera.fX) + (fA * psVisData->vUp.fX);
	fYn = - (fB * psVisData->vCamera.fY) + (fA * psVisData->vUp.fY);
	fZn = - (fB * psVisData->vCamera.fZ) + (fA * psVisData->vUp.fZ);

	psVisData->vCamera.fX = fX;
	psVisData->vCamera.fY = fY;
	psVisData->vCamera.fZ = fZ;

	psVisData->vUp.fX = fXn;
	psVisData->vUp.fY = fYn;
	psVisData->vUp.fZ = fZn;

	// Theta
	fXv = (psVisData->vCamera.fY * psVisData->vUp.fZ) - (psVisData->vCamera.fZ * psVisData->vUp.fY);
	fYv = (psVisData->vCamera.fZ * psVisData->vUp.fX) - (psVisData->vCamera.fX * psVisData->vUp.fZ);
	fZv = (psVisData->vCamera.fX * psVisData->vUp.fY) - (psVisData->vCamera.fY * psVisData->vUp.fX);

	fA = cos (fTheta);
	fB = sin (fTheta);

	fX = (fA * psVisData->vCamera.fX) + (fB * fXv);
	fY = (fA * psVisData->vCamera.fY) + (fB * fYv);
	fZ = (fA * psVisData->vCamera.fZ) + (fB * fZv);

	psVisData->vCamera.fX = fX;
	psVisData->vCamera.fY = fY;
	psVisData->vCamera.fZ = fZ;

	// Psi
	fA = cos (fPsi);
	fB = sin (fPsi);

	fXv = (psVisData->vCamera.fY * psVisData->vUp.fZ) - (psVisData->vCamera.fZ * psVisData->vUp.fY);
	fYv = (psVisData->vCamera.fZ * psVisData->vUp.fX) - (psVisData->vCamera.fX * psVisData->vUp.fZ);
	fZv = (psVisData->vCamera.fX * psVisData->vUp.fY) - (psVisData->vCamera.fY * psVisData->vUp.fX);

	fXn = (fA * fXn) - (fB * fXv);
	fYn = (fA * fYn) - (fB * fYv);
	fZn = (fA * fZn) - (fB * fZv);

	psVisData->vUp.fX = fXn;
	psVisData->vUp.fY = fYn;
	psVisData->vUp.fZ = fZn;

	// Normalise vectors (they should already be, but we make sure to avoid
	// cumulative rounding errors)

	Normalise3f (& psVisData->vCamera.fX, & psVisData->vCamera.fY, & psVisData->vCamera.fZ);
	Normalise3f (& psVisData->vUp.fX, & psVisData->vUp.fY, & psVisData->vUp.fZ);
}
*/

void ChangeView (float fTheta, float fPhi, float fPsi, float fRadius, VisPersist * psVisData) {
	psVisData->fViewRadius += fRadius;

	RANGE (psVisData->fViewRadius, 5.0, 80.0)

	// Rotation
	psVisData->fRotation += fTheta;
	if (psVisData->fRotation > M_TWOPI) {
		psVisData->fRotation -= floor (psVisData->fRotation / M_TWOPI) * M_TWOPI;
	}
	if (psVisData->fRotation < 0.0) {
		psVisData->fRotation += ceil (-psVisData->fRotation / M_TWOPI) * M_TWOPI;
	}

	// Elevation
	psVisData->fElevation += fPhi;
	if (psVisData->fElevation < (3.14159265 / 20.0f)) psVisData->fElevation = (3.14159265 / 20.0f);
	if (psVisData->fElevation > (3.14159265 / 2.0f)) psVisData->fElevation = (3.14159265 / 2.0f);

	// Calculate camera position
	psVisData->vCamera.fX = - cos (psVisData->fElevation) * cos (psVisData->fRotation);
	psVisData->vCamera.fY = cos (psVisData->fElevation) * sin (psVisData->fRotation);
	psVisData->vCamera.fZ = sin (psVisData->fElevation);

	// Calculate camera up direction
	psVisData->vUp.fX = - cos ((3.14159265 / 2.0f) - psVisData->fElevation) * cos ((3.14159265) + psVisData->fRotation);
	psVisData->vUp.fY = cos ((3.14159265 / 2.0f) - psVisData->fElevation) * sin ((3.14159265)+ psVisData->fRotation);
	psVisData->vUp.fZ = sin ((3.14159265 / 2.0f) - psVisData->fElevation);

	// Normalise vectors (they should already be, but we make sure to avoid
	// cumulative rounding errors)

	//Normalise (& psVisData->vCamera.fX, & psVisData->vCamera.fY, & psVisData->vCamera.fZ);
	//Normalise (& psVisData->vUp.gfX, & psVisData->vUp.fY, & psVisData->vUp.fZ);
}

void SetView (float fViewRadius, float fRotation, float fElevation, VisPersist * psVisData) {
	psVisData->fViewRadius = fViewRadius;
	psVisData->fRotation = fRotation;
	psVisData->fElevation = fElevation;

	ChangeView (0.0f, 0.0f, 0.0f, 0.0f, psVisData);
}

void GetView (float * pfViewRadius, float * pfRotation, float * pfElevation, VisPersist * psVisData) {
	if (pfViewRadius) {
		* pfViewRadius = psVisData->fViewRadius;
	}
	if (pfRotation) {
		* pfRotation = psVisData->fRotation;
	}
	if (pfElevation) {
		* pfElevation = psVisData->fElevation;
	}
}

void Motion (int nX, int nY, VisPersist * psVisData) {
	GLfloat z;
	double fX;
	double fY;
	double fZ;

	if (psVisData->boMoving) {
		//ChangeView ((nX - psVisData->nXMouse) / MOUSE_ROTATE_SCALE, (nY - psVisData->nYMouse) / MOUSE_ROTATE_SCALE, 0.0f, 0.0f, psVisData);

		psVisData->fXMomentum = ((float)nX - (float)psVisData->nXMouse) / MOUSE_ROTATE_SCALE;
		psVisData->fYMomentum = ((float)nY - (float)psVisData->nYMouse) / MOUSE_ROTATE_SCALE;
		psVisData->fZMomentum = 0.0f;

		ChangeView (psVisData->fXMomentum, psVisData->fYMomentum, 0.0f, 0.0f, psVisData);

		psVisData->nXMouse = nX;
		psVisData->nYMouse = nY;
		//glutPostRedisplay ();
	}

	if (psVisData->boMovingFunction) {
		z = SelectFunction (nX, nY, psVisData);
		if ((z > 0.0f) && (z < 1.0f)) {
			ConvertCoords ((GLdouble)nX, (GLdouble)nY, (GLdouble)z, & fX, & fY, & fZ, psVisData);

			psVisData->fXMomentumFunction = (fX - psVisData->fXMouseFunction) * psVisData->fXWidth;
			psVisData->fYMomentumFunction = (fY - psVisData->fYMouseFunction) * psVisData->fYWidth;
			psVisData->fZMomentumFunction = 0.0f;

			ShuntFunctionPosition (psVisData->fXMomentumFunction, psVisData->fYMomentumFunction, 0.0, psVisData);

			psVisData->fXMouseFunction = fX;
			psVisData->fYMouseFunction = fY;
		}
	}
}

void ScaleFunctionRange (double fScale, VisPersist * psVisData) {

	psVisData->fXMin -= psVisData->fXWidth * (fScale - 1.0) / 2.0;
	psVisData->fXWidth *= fScale;

	psVisData->fYMin -= psVisData->fYWidth * (fScale - 1.0) / 2.0;
	psVisData->fYWidth *= fScale;

	psVisData->fZMin -= psVisData->fZWidth * (fScale - 1.0) / 2.0;
	psVisData->fZWidth *= fScale;

	CalculateGridScale (psVisData);

	psVisData->boRangeChange = TRUE;

	// We need to scale the range of every function
	g_slist_foreach (psVisData->psFuncList, SetFunctionPositionCallback, psVisData);
}

void TransferFunctionRange (FuncPersist * psFuncData, VisPersist * psVisData) {
	SetFunctionRange (psVisData->fXMin, psVisData->fYMin, psVisData->fZMin, psVisData->fXWidth, psVisData->fYWidth, psVisData->fZWidth, psFuncData);
}

void ShuntFunctionPosition (double fX, double fY, double fZ, VisPersist * psVisData) {
	psVisData->fXMin -= fX;
	psVisData->fYMin -= fY;
	psVisData->fZMin -= fZ;

	psVisData->boRangeChange = TRUE;

	// We need to shunt every function
	g_slist_foreach (psVisData->psFuncList, ShuntFunctionPositionCallback, psVisData);
}

void ShuntFunctionPositionCallback (gpointer data, gpointer user_data) {
	VisPersist * psVisData = (VisPersist *)user_data;
	FuncPersist * psFuncData = (FuncPersist *)data;

	SetFunctionPosition (psVisData->fXMin, psVisData->fYMin, psVisData->fZMin, psFuncData);
}

void SetFunctionPositionCallback (gpointer data, gpointer user_data) {
	VisPersist * psVisData = (VisPersist *)user_data;
	FuncPersist * psFuncData = (FuncPersist *)data;

	TransferFunctionRange (psFuncData, psVisData);
}

void PlotFunction (char const * const szFunction, int nFunctionNum, VisPersist * psVisData) {
	FuncPersist * psFuncData;

	psFuncData = g_slist_nth_data (psVisData->psFuncList, nFunctionNum);

	if (psFuncData) {
		SetFunction (szFunction, psFuncData);
		PopulateVertices (psFuncData);
	}
}

gboolean GetMoving (VisPersist * psVisData) {
	return psVisData->boMoving;
}

/*
int main (int argc, char **argv) {
	char szResults[_MAX_PATH];

	srand (time (NULL));
	glutInitWindowSize (SCREENWIDTH, SCREENHEIGHT);
	glutInit (&argc, argv);
	glutInitDisplayMode (GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutCreateWindow ("test");
	glutDisplayFunc (Redraw);
	glutMouseFunc (Mouse);
	glutKeyboardFunc (Key);
	glutKeyboardUpFunc (KeyUp);
	glutIgnoreKeyRepeat (true);
	glutMotionFunc (Motion);
	glutReshapeFunc (Reshape);
	glutIdleFunc (Idle);
	//glutCreateMenu (menu);
	//glutAddMenuEntry ("Nothing", 1);
	//glutAddMenuEntry ("Nothing else", 2);
	//glutAttachMenu (GLUT_RIGHT_BUTTON);

	if (argc < 2) {
		szResults[0] = 0;
	}
	else {
		strncpy (szResults, argv[1], _MAX_PATH);
	}
	Init (szResults);

	glutMainLoop ();
	return 0;
}
*/

void ResetAnimation (VisPersist * psVisData) {
	struct timeb sTime;

	// Reset the time
	ftime (& sTime);

	psVisData->fCurrentTime = (double)(sTime.time) + (double)(sTime.millitm) / 1000.0;
	if (psVisData->fCurrentTime < 0) psVisData->fCurrentTime = 0.0;
	psVisData->fPrevTime = psVisData->fCurrentTime;
	psVisData->fSpinTime = psVisData->fCurrentTime;
	psVisData->fFunctionTime = 0.0;
	psVisData->fFunctionTimePrev = psVisData->fCurrentTime;
	psVisData->fRangeChangeTime = psVisData->fCurrentTime;
}

GSList * GetFunctionList (VisPersist * psVisData) {
	return psVisData->psFuncList;
}

unsigned int GetFunctionCount (VisPersist * psVisData) {
	unsigned int uCount;
	
	uCount = g_slist_length (psVisData->psFuncList);
	
	return uCount;
}

void GetVisRange (double * afRange, VisPersist * psVisData) {
	if (afRange) {
		afRange[0] = psVisData->fXMin;
		afRange[1] = psVisData->fYMin;
		afRange[2] = psVisData->fZMin;
		afRange[3] = psVisData->fXWidth;
		afRange[4] = psVisData->fYWidth;
		afRange[5] = psVisData->fZWidth;
	}
}

void SetVisRange (double * afRange, VisPersist * psVisData) {
	if (afRange) {
		psVisData->fXMin = afRange[0];
		psVisData->fYMin = afRange[1];
		psVisData->fZMin = afRange[2];
		psVisData->fXWidth = afRange[3];
		psVisData->fYWidth = afRange[4];
		psVisData->fZWidth = afRange[5];

		CalculateGridScale (psVisData);

		psVisData->boRangeChange = TRUE;

		// We need to set the position every function
		g_slist_foreach (psVisData->psFuncList, SetFunctionPositionCallback, psVisData);
	}
}

void CalculateGridScale (VisPersist * psVisData) {
	double fGrid;
	double fRescale;

	// Recalculate the grid spacing
	// We find the nearest power of 10 that provides between 5 and 50 gridlines
	// We then divide the number by 2 until we have at most 14 gridlines
	// This should ensure we have between 5 and 14 lines split into sensible widths

	fGrid = pow (10, floor ((log (fabs (psVisData->fXWidth)) - log (5)) / log (10)));
	fRescale = ceil (log (psVisData->fXWidth / (GRIDLINES_MAX * fGrid)) / log (2));
	psVisData->fXGrid = fGrid * pow (2, fRescale);

	fGrid = pow (10, floor ((log (fabs (psVisData->fYWidth)) - log (5)) / log (10)));
	fRescale = ceil (log (psVisData->fYWidth / (GRIDLINES_MAX * fGrid)) / log (2));
	psVisData->fYGrid = fGrid * pow (2, fRescale);

	fGrid = pow (10, floor ((log (fabs (psVisData->fZWidth)) - log (5)) / log (10)));
	fRescale = ceil (log (psVisData->fZWidth / (GRIDLINES_MAX * fGrid)) / log (2));
	psVisData->fZGrid = fGrid * pow (2, fRescale);
}

bool UpdateRange (VisPersist * psVisData) {
	bool boReturn;

	if (psVisData->boRangeChange
		&& ((psVisData->fCurrentTime - psVisData->fRangeChangeTime) > RANGE_UPDATE_TIME)) {
		boReturn = TRUE;
		psVisData->fRangeChangeTime = psVisData->fCurrentTime;
		psVisData->boRangeChange = FALSE;
	}
	else {
		boReturn = FALSE;
	}

	return boReturn;
}

bool ExportModelFilePLY (char const * szFilename, bool boBinary, bool boScreenCoords, bool boExportAlpha, double fMultiplier, double fScale, VisPersist * psVisData) {
	bool boSuccess;

	boSuccess = ExportModelPLY (szFilename, boBinary, boScreenCoords, boExportAlpha, fMultiplier, fScale, psVisData->psFuncList);

	return boSuccess;
}

bool ExportModelFileSTL (char const * szFilename, bool boBinary, bool boScreenCoords, double fMultiplier, double fScale, VisPersist * psVisData) {
	bool boSuccess;

	boSuccess = ExportModelSTL (szFilename, boBinary, boScreenCoords, fMultiplier, fScale, psVisData->psFuncList);

	return boSuccess;
}

bool ExportBitmapFilePNG (char const * szFilename, int nHeight, int nWidth, VisPersist * psVisData) {
	bool boSuccess;

	boSuccess = ExportBitmapFile (szFilename, "png", nHeight, nWidth, psVisData);

	return boSuccess;
}

void SetPauseAnimation (bool boPause, VisPersist * psVisData) {
	psVisData->boPaused = boPause;
	SetAudioPause (boPause, psVisData->psAudioData);
}

bool GetPauseAnimation (VisPersist * psVisData) {
	return psVisData->boPaused;
}

/*
bool ExportModelFileAnimated (char const * szFilename, bool boBinary, bool boScreenCoords, bool boExportAlpha, float fMultiplier, double fTimeStart, double fTimeEnd, int nFrames, VisPersist * psVisData) {
	bool boSuccess;
	GSList const * psFuncList;
	double fTime;
	double fTimeIncrement;
	int nFrame;
	GString * szFileIncrement;
	FuncPersist * psFuncData;
	FUNCTYPE eFuncType;
	
	fTimeIncrement = (fTimeEnd - fTimeStart) / ((double)nFrames);
	szFileIncrement = g_string_new ("");
	boSuccess = TRUE;

	fTime = fTimeStart;
	for (nFrame = 0; nFrame < nFrames; nFrame++) {
		// Set the filename
		g_string_printf (szFileIncrement, "%s%03d.ply", szFilename, nFrame);

		// Set the time for all of the functions
		psFuncList = psVisData->psFuncList;
		while (psFuncList) {
			psFuncData = (FuncPersist *)(psFuncList->data);
			eFuncType = GetFunctionType (psFuncData);

			switch (eFuncType) {
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

			SetFunctionTime (fTime, (FuncPersist *)(psFuncList->data));
			psFuncList = g_slist_next (psFuncList);
		}
		boSuccess &= ExportModel (szFileIncrement->str, boBinary, boScreenCoords, boExportAlpha, fMultiplier, psVisData->psFuncList);
		
		fTime += fTimeIncrement;
	}
	
	g_string_free (szFileIncrement, TRUE);

	return boSuccess;
}
*/

ControlvarPersist * SetControlVar (char const * const szVarName, char const * const szVarValue, VisPersist * psVisData) {
	ControlvarPersist * psControlvarData;
	GSList * psControlvarList;
	char const * szSearchName;

	// First check whether the varaible is already there
	psControlvarData = NULL;
	psControlvarList = psVisData->psFnControlData->psControlvarList;
	while (psControlvarList) {
		szSearchName = GetControlvarName (((ControlvarPersist *)(psControlvarList->data)));

		if (strcmp (szSearchName, szVarName) == 0) {
			psControlvarData = ((ControlvarPersist *)(psControlvarList->data));
		}
		
		psControlvarList = g_slist_next (psControlvarList);
	}

	if (psControlvarData == NULL) {
		// Create the variable
		psControlvarData = NewControlvarPersist ();
		psVisData->psFnControlData->psControlvarList = g_slist_append (psVisData->psFnControlData->psControlvarList, psControlvarData);
	}
	
	// Set the variable value
	SetControlvarName (szVarName, psControlvarData);
	SetControlvarValue (szVarValue, psControlvarData);

	return psControlvarData;
}

FnControlPersist * GetControlVarList (VisPersist * psVisData) {
	return psVisData->psFnControlData;
}

void AssignControlVarsToFunctionList (VisPersist * psVisData) {
	g_slist_foreach (psVisData->psFuncList, AssignControlVarsCallback, psVisData->psFnControlData);
}

void AssignControlVarsCallback (gpointer data, gpointer user_data) {
	AssignControlVarsToFunctionPopulate ((FnControlPersist *)user_data, (FuncPersist *)data);
}

void AssignControlVarsSingle (FuncPersist * psFuncData, VisPersist * psVisData) {
	AssignControlVarsToFunction (psVisData->psFnControlData, psFuncData);
}

int GetScreenHeight (VisPersist * psVisData) {
	return psVisData->nScreenHeight;
}

int GetScreenWidth (VisPersist * psVisData) {
	return psVisData->nScreenWidth;
}

void SetScreenBuffer (GLuint uBuffer, VisPersist * psVisData) {
	psVisData->uScreenBuffer = uBuffer;
}

void SetScreenBufferToScreen (VisPersist * psVisData) {
	psVisData->uScreenBuffer = 0u;
}

double GetCurrentVisTime (VisPersist * psVisData) {
	return psVisData->fCurrentTime;
}

AudioPersist * GetAudioData (VisPersist * psVisData) {
	return psVisData->psAudioData;
}

