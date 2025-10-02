///////////////////////////////////////////////////////////////////
// Functy
// 3D graph drawing utility
//
// David Llewellyn-Jones
// http://www.flypig.co.uk
//
// Spring 2009
///////////////////////////////////////////////////////////////////

#ifndef FUNCTY_H
#define FUNCTY_H

///////////////////////////////////////////////////////////////////
// Includes

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "vis.h"
#include "function.h"
#include "global.h"

///////////////////////////////////////////////////////////////////
// Defines

///////////////////////////////////////////////////////////////////
// Structures and enumerations

typedef struct _FunctyPersist FunctyPersist;

typedef enum _BUTTONBARSTYLE {
	BUTTONBARSTYLE_INVALID = -1,

	BUTTONBARSTYLE_NONE,
	BUTTONBARSTYLE_ICONS,
	BUTTONBARSTYLE_ICONSTEXT,

	BUTTONBARSTYLE_NUM
} BUTTONBARSTYLE;

struct _FunctyPersist {
	GlobalPersist * psGlobalData;
	VisPersist * psVisData;
	GtkWidget * psDrawingArea;
	gboolean boTimeoutContinue;
	guint TimeoutID;
	GtkBuilder * psXML;
	FuncPersist * psFuncEdit;
	double fNewColour;
	bool boPausedModal;
	bool boFileLoaded;
	GString * szFilename;
	bool boFolderSet;
	bool boFolderSetAnim;
	bool boExportedModelPLY;
	bool boExportedModelSTL;
	bool boExportedModelSVX;
	bool boExportedModelVDB;
	bool boExportedAnim;
	GString * szExportModelNamePLY;
	GString * szExportModelNameSTL;
	GString * szExportModelNameSVX;
	GString * szExportModelNameVDB;
	GString * szExportAnimName;
	bool boBinary;
	double fExportMultiplier;
	double fExportScale;
	bool boExportScreen;
	bool boExportAlpha;
	double fExportTimeStart;
	double fExportTimeEnd;
	int nExportFrames;
	int nVoxelResolution;
	GString * szFolder;
	GString * szFolderAnim;
	FUNCTYPE eNextFunctionType;
	//bool boButtonBarHidden;
	BUTTONBARSTYLE eButtonBarStyle;
	bool boShowPanelLeft;
	bool boShowPanelBottom;
	bool boExportedBitmap;
	GString * szExportBitmapName;
	bool boBitmapScreenDimensions;
	int nBitmapWidth;
	int nBitmapHeight;
	char * szLicence;
};

///////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////
// Function prototypes

FunctyPersist * NewFunctyPersist (void);
void DeleteFunctyPersist (FunctyPersist * psFunctyData);

void SetFullScreenWindow (bool boFullScreen, FunctyPersist * psFunctyData);
void SetExportBinary (bool boBinary, FunctyPersist * psFunctyData);
void SetExportMultiplier (double fExportMultiplier, FunctyPersist * psFunctyData);
void SetExportScaleFactor (double fExportScale, FunctyPersist * psFunctyData);
void SetExportScreen (bool boExportScreen, FunctyPersist * psFunctyData);
void SetExportAlpha (bool boExportAlpha, FunctyPersist * psFunctyData);
void SetExportTimeStart (double fExportTimeStart, FunctyPersist * psFunctyData);
void SetExportTimeEnd (double fExportTimeEnd, FunctyPersist * psFunctyData);
void SetExportFrames (int nExportFrames, FunctyPersist * psFunctyData);

///////////////////////////////////////////////////////////////////
// Function definitions

#endif /* FUNCTY_H */

