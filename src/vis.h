///////////////////////////////////////////////////////////////////
// Functy
// 3D graph drawing utility
//
// David Llewellyn-Jones
// http://www.flypig.co.uk
//
// Spring 2009
///////////////////////////////////////////////////////////////////

#ifndef VIS_H
#define VIS_H

///////////////////////////////////////////////////////////////////
// Includes

#include "utils.h"
#include "function.h"
#include "controlvar.h"
#include "audio.h"
#include "global.h"

///////////////////////////////////////////////////////////////////
// Defines

#define RANGE(VAR,MIN,MAX) if (VAR < (MIN)) VAR=(MIN); if (VAR > (MAX)) VAR=(MAX);
//#define MIN(VAR1,VAR2) ((VAR1) < (VAR2) ? : (VAR1) : (VAR2))
//#define MAX(VAR1,VAR2) ((VAR1) > (VAR2) ? : (VAR1) : (VAR2))

//#define LEFT_BUTTON GLUT_LEFT_BUTTON
//#define RIGHT_BUTTON GLUT_RIGHT_BUTTON
//#define BUTTON_DOWN GLUT_DOWN
//#define BUTTON_UP GLUT_UP
#define LEFT_BUTTON 1
#define CENTRE_BUTTON 2
#define RIGHT_BUTTON 3
#define BUTTON_DOWN GDK_BUTTON_PRESS
#define BUTTON_UP GDK_BUTTON_RELEASE

#define SCREENWIDTH         (800)
#define SCREENHEIGHT        (600)

#define AXIS_XSIZE          (40.0)
#define AXIS_YSIZE          (40.0)
#define AXIS_ZSIZE          (40.0)
#define AXIS_XHSIZE         (AXIS_XSIZE / 2.0)
#define AXIS_YHSIZE         (AXIS_YSIZE / 2.0)
#define AXIS_ZHSIZE         (AXIS_ZSIZE / 2.0)

///////////////////////////////////////////////////////////////////
// Structures and enumerations

typedef struct _VisPersist VisPersist;

///////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////
// Function prototypes

VisPersist * NewVisPersist (GlobalPersist const * psGlobalData);
void DeleteVisPersist (VisPersist * psVisData);

void Redraw (VisPersist * psVisData);
void Mouse (int button, int state, int x, int y, VisPersist * psVisData);
void Key (unsigned int key, int x, int y, unsigned int uKeyModifiers, VisPersist * psVisData);
void KeyUp (unsigned int key, int x, int y, unsigned int uKeyModifiers, VisPersist * psVisData);
void Motion (int nX, int nY, VisPersist * psVisData);
void Reshape (int w, int h, VisPersist * psVisData);
void Idle (VisPersist * psVisData);
void Init (VisPersist * psVisData);
void Deinit (VisPersist * psVisData);
void Realise (VisPersist * psVisData);
void ToggleFullScreen (VisPersist * psVisData);
void ToggleClearWhite (VisPersist * psVisData);
void ToggleSpin (VisPersist * psVisData);
void ToggleWireframe (VisPersist * psVisData);
void ToggleDrawAxes (VisPersist * psVisData);
void GetDisplayProperties (float * pfViewRadius, float * pfLinkLen, float * pfCentring, float * pfRigidity, float * pfForce, float * pfResistance, VisPersist * psVisData);
void SetDisplayProperties (float fViewRadius, float fLinkLen, float fCentring, float fRigidity, float fForce, float fResistance, VisPersist * psVisData);
void SetClearWhite (bool boClearWhite, VisPersist * psVisData);
bool GetClearWhite (VisPersist * psVisData);
void SetFullScreen (bool boFullScreen, VisPersist * psVisData);
bool GetFullScreen (VisPersist * psVisData);
void SetSpin (bool boSpin, VisPersist * psVisData);
bool GetSpin (VisPersist * psVisData);
gboolean GetMoving (VisPersist * psVisData);
void SetDrawAxes (bool boDrawAxes, VisPersist * psVisData);
bool GetDrawAxes (VisPersist * psVisData);
void SetWireframe (bool boWireframe, VisPersist * psVisData);
bool GetWireframe (VisPersist * psVisData);
void SetShadow (bool boShadow, VisPersist * psVisData);
bool GetShadow (VisPersist * psVisData);
void SetFocusBlur (bool boFocusBlur, VisPersist * psVisData);
bool GetFocusBlur (VisPersist * psVisData);
void SetFocusBlurNear (float fFocusBlurNear, VisPersist * psVisData);
float GetFocusBlurNear (VisPersist * psVisData);
void SetFocusBlurFar (float fFocusBlurFar, VisPersist * psVisData);
float GetFocusBlurFar (VisPersist * psVisData);
void SetShadersActive (bool boShaderActive, VisPersist * psVisData);
bool GetShadersActive (VisPersist * psVisData);
void RenderBitmapString (float fX, float fY, void * pFont, char const * szString);
void RenderBitmapStringCursor (float fX, float fY, void * pFont, char const * szString, int nCursorPos);
void RenderCursor (float fX, float fY, void * pFont, char const * szString, int nCursorPos);
int BitmapStringWidth (void * pFont, char const * szString);
void PlotFunction (char const * const szFunction, int nFunctionNum, VisPersist * psVisData);
GSList * GetFunctionList (VisPersist * psVisData);
unsigned int GetFunctionCount (VisPersist * psVisData);
FuncPersist * AddNewFunction (FUNCTYPE eType, VisPersist * psVisData);
void DeleteFunction (FuncPersist * psFuncData, VisPersist * psVisData);
void TransferFunctionRange (FuncPersist * psFuncData, VisPersist * psVisData);
void GetVisRange (double * afRange, VisPersist * psVisData);
void SetVisRange (double * afRange, VisPersist * psVisData);
bool UpdateRange (VisPersist * psVisData);
void DeleteAllFunctions (VisPersist * psVisData);
void SetView (float fViewRadius, float fRotation, float fElevation, VisPersist * psVisData);
void GetView (float * pfViewRadius, float * pfRotation, float * pfElevation, VisPersist * psVisData);
void ScaleFunctionRange (double fScale, VisPersist * psVisData);
bool ExportModelFilePLY (char const * szFilename, bool boBinary, bool boScreenCoords, bool boExportAlpha, double fMultiplier, double fScale, VisPersist * psVisData);
bool ExportModelFileSTL (char const * szFilename, bool boBinary, bool boScreenCoords, double fMultiplier, double fScale, VisPersist * psVisData);
bool ExportBitmapFilePNG (char const * szFilename, int nHeight, int nWidth, VisPersist * psVisData);
void SetPauseAnimation (bool boPause, VisPersist * psVisData);
bool GetPauseAnimation (VisPersist * psVisData);
ControlvarPersist * SetControlVar (char const * const szVarName, char const * const szVarValue, VisPersist * psVisData);
FnControlPersist * GetControlVarList (VisPersist * psVisData);
void AssignControlVarsToFunctionList (VisPersist * psVisData);
void AssignControlVarsSingle (FuncPersist * psFuncData, VisPersist * psVisData);
int GetScreenHeight (VisPersist * psVisData);
int GetScreenWidth (VisPersist * psVisData);
bool ExportBitmap (char const * szFilename, char const * szType, int nHeight, int nWidth, VisPersist * psVisData);
void SetScreenBuffer (GLuint uBuffer, VisPersist * psVisData);
void SetScreenBufferToScreen (VisPersist * psVisData);
double GetCurrentVisTime (VisPersist * psVisData);
AudioPersist * GetAudioData (VisPersist * psVisData);

///////////////////////////////////////////////////////////////////
// Function definitions

#endif /* VIS_H */


