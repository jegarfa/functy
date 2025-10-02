///////////////////////////////////////////////////////////////////
// Functy
// 3D graph drawing utility
//
// David Llewellyn-Jones
// http://www.flypig.co.uk
//
// Spring 2009
//
// GTK code inspired by GtkGLExt logo demo
// by Naofumi Yasufuku <naofumi@users.sourceforge.net>
///////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////
// Includes

#include "utils.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkgl.h>
#include <gio/gio.h>

#include "functy.h"
#include "filesave.h"
#include "spherical.h"
#include "curve.h"
#include "exportply.h"
#include "exportstl.h"
#include "exportsvx.h"
#include "exportvdb.h"
#include "exportbitmap.h"
#include "controlvar.h"
#include "longpoll.h"

///////////////////////////////////////////////////////////////////
// Defines

#define UPDATE_TIMEOUT (10)
#define FULLSCREEN_BUTTONBARSHOW (8)
#define RADIUS_SCROLL (1)
#define FUNCTION_NEW_CARTESIAN "0"
#define FUNCTION_NEW_SPHERICAL "1"
#define FUNCTION_NEW_CURVE_RADIUS "1"
#define FUNCTION_NEW_CURVE_X "0"
#define FUNCTION_NEW_CURVE_Y "0"
#define FUNCTION_NEW_CURVE_Z "a"
#define ALPHA_NEW (0.9)
#define COLOUR_NEW_SHIFT (2.3)
#define RANGE_FORMAT "%g"
#define DEFAULT_FILENAME "Functions.xml"
#define DEFAULT_EXPORTMODELNAME "Model"
#define DEFAULT_EXPORTANIMNAME "AnimFrames.zip"
#define RANGE_SCALEUP (1.08)
#define RANGE_SCALEDOWN (1.0/RANGE_SCALEUP)
#define SPHERICAL_XCENTRE (0.0)
#define SPHERICAL_YCENTRE (0.0)
#define SPHERICAL_ZCENTRE (0.0)
#define CURVE_XCENTRE (0.0)
#define CURVE_YCENTRE (0.0)
#define CURVE_ZCENTRE (0.0)
#define EXPORTANIM_TIME_MAX (86400)
#define DEFAULT_EXPORTBITMAPNAME "Bitmap.png"
#define DEFAULT_VOXELRESOLUTION (128)
#define DEFAULT_BITMAPWIDTH (512)
#define DEFAULT_BITMAPHEIGHT (512)

///////////////////////////////////////////////////////////////////
// Structures and enumerations

typedef enum _FUNCSCOL {
	FUNCSCOL_INVALID = -1,

	FUNCSCOL_FUNCDATA,
	FUNCSCOL_FUNCTION,

	FUNCSCOL_NUM
} _FUNCSCOL;

typedef struct _BitmapSize {
  GtkWidget * psWindowSize;
  GtkWidget * psWidth;
  GtkWidget * psHeight;
  GtkWidget * psWidthLabel;
  GtkWidget * psHeightLabel;
} BitmapSize;

typedef struct _AudioComboData {
	GtkComboBoxText * psCombo;
	AudioPersist * psAudioData;
	int nIndex;
	int nSelectedIndex;
} AudioComboData;

///////////////////////////////////////////////////////////////////
// Global variables

static char * szArvOptionDataDir = NULL;

static GOptionEntry asEntries[] = {
	{"datadir", 'd', G_OPTION_FLAG_NONE, G_OPTION_ARG_FILENAME, & szArvOptionDataDir, "Data directory to use (defaults to '" FUNCTYDIR "')", "DIR"},
	{NULL}
};

///////////////////////////////////////////////////////////////////
// Function prototypes

static void Realize (GtkWidget * psWidget, gpointer psData);
static gboolean ConfigureEvent (GtkWidget * psWidget, GdkEventConfigure * psEvent, gpointer psData);
static gboolean ExposeEvent (GtkWidget * psWidget, GdkEventExpose * psEvent, gpointer psData);
static gboolean ButtonPressEvent (GtkWidget * psWidget, GdkEventButton * psEvent, gpointer * psData);
static gboolean ButtonReleaseEvent (GtkWidget * psWidget, GdkEventButton * psEvent, gpointer * psData);
//static gboolean MotionNotifyEvent (GtkWidget * psWidget, GdkEventMotion * psEvent, gpointer psData);
static gboolean ScrollEvent (GtkWidget * psWidget, GdkEventScroll * psEvent, gpointer psData);
static gboolean KeyPressEvent (GtkWidget * psWidget, GdkEventKey * psEvent, gpointer psData);
static gboolean KeyReleaseEvent (GtkWidget * psWidget, GdkEventKey * psEvent, gpointer psData);
static gboolean Timeout (gpointer psData);
static void TimeoutAdd (FunctyPersist * psFunctyData);
static void TimeoutRemove (FunctyPersist * psFunctyData);
static gboolean MapEvent (GtkWidget * psWidget, GdkEventAny * psEvent, gpointer psData);
static gboolean UnmapEvent (GtkWidget * psWidget, GdkEventAny * psEvent, gpointer psData);
static gboolean VisibilityNotifyEvent (GtkWidget * psWidget, GdkEventVisibility * psEvent, gpointer psData);
void ToggleFullScreenWindow (FunctyPersist * psFunctyData);
void ToggleButtonBar (FunctyPersist * psFunctyData);
//static gboolean PlotFunctionPress (GtkWidget * psWidget, gpointer psData);
static gboolean PlotFunctionApply (GtkWidget * psWidget, gpointer psData);
static gboolean PlotFunctionCancel (GtkWidget * psWidget, gpointer psData);
static gboolean ToggleSpinPress (GtkToggleButton * psWidget, gpointer psData);
static gboolean ToggleInvertPress (GtkToggleButton * psWidget, gpointer psData);
//static gboolean ToggleFullscreenPress (GtkToggleButton * psWidget, gpointer psData);
static gboolean ToggleDrawAxesPress (GtkWidget * psWidget, gpointer psData);
static gboolean ToggleWireframePress (GtkWidget * psWidget, gpointer psData);
static gboolean ToggleUseShadersPress (GtkWidget * psWidget, gpointer psData);
static gboolean ToggleShadowPress (GtkWidget * psWidget, gpointer psData);
static gboolean ToggleFocusBlurPress (GtkWidget * psWidget, gpointer psData);
static gboolean TogglePausePress (GtkWidget * psWidget, gpointer psData);
static gboolean FullscreenPress (GtkWidget * psWidget, gpointer psData);
void TogglePanelLeft (FunctyPersist * psFunctyData);
void TogglePanelBottom (FunctyPersist * psFunctyData);
static gboolean TogglePanelLeftPress (GtkWidget * psWidget, gpointer psData);
static gboolean TogglePanelBottomPress (GtkWidget * psWidget, gpointer psData);
//static gboolean ViewPress (GtkWidget * psWidget, gpointer psData);
void EditFunctionRowDoubleClick (GtkTreeView * psTreeView, GtkTreePath * psPath, GtkTreeViewColumn * psColumn, gpointer psData);
void EditFunctionRowChanged (GtkTreeSelection * psSelection, gpointer psData);
//static gboolean EditFunctionPress (GtkWidget * psWidget, gpointer psData);
void EditFunctionFromList (GtkTreeModel * psTreeModel, GtkTreeIter * psTreeIter, FunctyPersist * psFunctyData);
void EditFunction (FuncPersist * psFuncData, FunctyPersist * psFunctyData);
void PopulateFunctionList (GtkListStore * psFunctionListModel, FunctyPersist * psFunctyData);
void AddFunction (FuncPersist * psFuncData, GtkListStore * psFunctionListModel);
void FunctionCellDataFunc (GtkTreeViewColumn * psTreeColumn, GtkCellRenderer * psCell, GtkTreeModel * psTreeModel, GtkTreeIter * psIter, gpointer psData);
void SetColourButton (FunctyPersist * psFunctyData);
void SetColourButtonCallback (GtkColorButton * psWidget, gpointer psData);
void SetViewWindowState (FunctyPersist * psFunctyData);
void SetRangeWindowState (FunctyPersist * psFunctyData);
//static gboolean RangeSetPress (GtkWidget * psWidget, gpointer psData);
static gboolean RangeSetApply (GtkWidget * psWidget, gpointer psData);
static gboolean RangeSetCancel (GtkWidget * psWidget, gpointer psData);
gboolean RangeMinChange (GtkWidget *psWidget, GdkEventKey * psEvent, gpointer psData);
gboolean RangeMaxChange (GtkWidget *psWidget, GdkEventKey * psEvent, gpointer psData);
void GetNextColour (double * pfRed, double * pfGreen, double * pfBlue, double * pfAlpha, FunctyPersist * psFunctyData);
static void ClearAllFunctions (FunctyPersist * psFunctyData);
static gboolean LoadFilePress (GtkWidget * psWidget, gpointer psData);
static gboolean SaveFilePress (GtkWidget * psWidget, gpointer psData);
void AudioSourceComboCallback (pa_context * psContext, const pa_source_info * psSourceInfo, int nEol, void * psUserData);
static gboolean AudioConfigPress (GtkWidget * psWidget, gpointer psData);
static gboolean AudioConfigClose (GtkWidget * psWidget, gpointer psData);
static gboolean AudioConfigDestroy (GtkWidget * psWidget, GdkEvent * psEvent, gpointer psData);
static gboolean AudioConfigOkay (GtkWidget * psWidget, gpointer psData);
static gboolean ExportFilePLYPress (GtkWidget * psWidget, gpointer psData);
static gboolean ExportFileSTLPress (GtkWidget * psWidget, gpointer psData);
static gboolean ExportFileSVXPress (GtkWidget * psWidget, gpointer psData);
static gboolean ExportFileVDBPress (GtkWidget * psWidget, gpointer psData);
static gboolean ExportAnimFilePLYPress (GtkWidget * psWidget, gpointer psData);
static gboolean ExportAnimFileSTLPress (GtkWidget * psWidget, gpointer psData);
static gboolean ExportAnimFilePNGPress (GtkWidget * psWidget, gpointer psData);
void PauseAnimationModal (bool boPauseModal, FunctyPersist * psFunctyData);
void NewCartesianFunction (FunctyPersist * psFunctyData);
void NewSphericalFunction (FunctyPersist * psFunctyData);
void NewCurveFunction (FunctyPersist * psFunctyData);
static gboolean AddCartesianPress (GtkWidget * psWidget, gpointer psData);
static gboolean AddSphericalPress (GtkWidget * psWidget, gpointer psData);
static gboolean AddCurvePress (GtkWidget * psWidget, gpointer psData);
gchar * GetTitleFilename (char const * szFilename);
void SetMainWindowTitle (char const * szFilename, FunctyPersist * psFunctyData);
void SetTextViewText (GtkWidget * psTextView, gchar const * const szText);
gchar * GetTextViewText (GtkWidget * psTextView);
//static gboolean ControlVariablesPress (GtkWidget * psWidget, gpointer psData);
static gboolean ControlVariablesApply (GtkWidget * psWidget, gpointer psData);
static gboolean ControlVariablesCancel (GtkWidget * psWidget, gpointer psData);
//static gboolean ControlvarPress (GtkWidget * psWidget, gpointer psData);
void SetControlvarWindowState (FunctyPersist * psFunctyData);
static gboolean ControlvarSliderChanged (GtkWidget * psWidget, gpointer psData);
static gboolean FocusBlurNearSliderChanged (GtkWidget * psWidget, gpointer psData);
static gboolean FocusBlurFarSliderChanged (GtkWidget * psWidget, gpointer psData);
void SetFocusBlurNearState (FunctyPersist * psFunctyData);
void SetFocusFarNearState (FunctyPersist * psFunctyData);
void SetFunctionPropertiesWindow (FUNCTYPE eType, bool boNew, FunctyPersist * psFunctyData);
void SetViewDrawAxis (FunctyPersist * psFunctyData);
void SetViewSpin (FunctyPersist * psFunctyData);
void SetViewInvert (FunctyPersist * psFunctyData);
void SetViewWireframe (FunctyPersist * psFunctyData);
void SetViewUseShaders (FunctyPersist * psFunctyData);
void SetViewShadow (FunctyPersist * psFunctyData);
void SetViewFocusBlur (FunctyPersist * psFunctyData);
void SetViewPauseAnimation (FunctyPersist * psFunctyData);
static gboolean DeleteFunctionPress (GtkWidget * psWidget, gpointer psData);
void SynchroniseUI (FunctyPersist * psFunctyData);
static void SetButtonBarStyleNone (GtkWidget * psWidget, gpointer psData);
static void SetButtonBarStyleIcons (GtkWidget * psWidget, gpointer psData);
static void SetButtonBarStyleIconsText (GtkWidget * psWidget, gpointer psData);
void SynchroniseButtonBarStyle (FunctyPersist * psFunctyData);
void SynchronisePanels (FunctyPersist * psFunctyData);
static gboolean BitmapWindowSizeToggle (GtkWidget * psWidget, gpointer psData);
static gboolean AboutShow (GtkWidget * psWidget, gpointer psData);
static gboolean DocumentationShow (GtkWidget * psWidget, gpointer psData);
static gboolean HomepageShow (GtkWidget * psWidget, gpointer psData);
void SetMaterialFill (bool boMaterialFill, FunctyPersist * psFunctyData);
static void ToggleMaterialFill (GtkWidget * psWidget, gpointer psData);

///////////////////////////////////////////////////////////////////
// Function definitions

FunctyPersist * NewFunctyPersist (void) {
	FunctyPersist * psFunctyData;

	psFunctyData = g_new0 (FunctyPersist, 1);

	psFunctyData->psGlobalData = NewGlobalPersist ();
	psFunctyData->psVisData = NULL;
	psFunctyData->psDrawingArea = NULL;
	psFunctyData->boTimeoutContinue = FALSE;
	psFunctyData->TimeoutID = 0;
	psFunctyData->psFuncEdit = NULL;
	psFunctyData->fNewColour = 0.0;
	psFunctyData->boPausedModal = FALSE;
	psFunctyData->boFileLoaded = FALSE;
	psFunctyData->szFilename = g_string_new (DEFAULT_FILENAME);
	psFunctyData->boFolderSet = FALSE;
	psFunctyData->szFolder = g_string_new ("");
	psFunctyData->boFolderSetAnim = FALSE;
	psFunctyData->szFolderAnim = g_string_new ("");
	psFunctyData->boExportedModelPLY = FALSE;
	psFunctyData->boExportedModelSTL = FALSE;
	psFunctyData->boExportedModelSVX = FALSE;
	psFunctyData->boExportedModelVDB = FALSE;
	psFunctyData->boExportedAnim = FALSE;
	psFunctyData->szExportModelNamePLY = g_string_new (DEFAULT_EXPORTMODELNAME ".ply");
	psFunctyData->szExportModelNameSTL = g_string_new (DEFAULT_EXPORTMODELNAME ".stl");
	psFunctyData->szExportModelNameSVX = g_string_new (DEFAULT_EXPORTMODELNAME ".svx");
	psFunctyData->szExportModelNameVDB = g_string_new (DEFAULT_EXPORTMODELNAME ".vdb");
	psFunctyData->szExportAnimName = g_string_new (DEFAULT_EXPORTANIMNAME);
	psFunctyData->boBinary = TRUE;
	psFunctyData->fExportMultiplier = 1.0;
	psFunctyData->fExportScale = 1.0;
	psFunctyData->boExportScreen = TRUE;
	psFunctyData->boExportAlpha = TRUE;
	psFunctyData->fExportTimeStart = 0.0;
	psFunctyData->fExportTimeEnd = 1.0;
	psFunctyData->nExportFrames = 50;
	psFunctyData->nVoxelResolution = DEFAULT_VOXELRESOLUTION;
	psFunctyData->eNextFunctionType = FUNCTYPE_INVALID;
	psFunctyData->eButtonBarStyle = BUTTONBARSTYLE_ICONS;
	psFunctyData->boShowPanelLeft = TRUE;
	psFunctyData->boShowPanelBottom = TRUE;
  psFunctyData->boExportedBitmap = FALSE;
  psFunctyData->szExportBitmapName = g_string_new (DEFAULT_EXPORTBITMAPNAME);
  psFunctyData->boBitmapScreenDimensions = TRUE;
  psFunctyData->nBitmapWidth = DEFAULT_BITMAPWIDTH;
  psFunctyData->nBitmapHeight = DEFAULT_BITMAPHEIGHT;
  psFunctyData->szLicence = NULL;

	return psFunctyData;
}

void DeleteFunctyPersist (FunctyPersist * psFunctyData) {
	if (psFunctyData->psGlobalData) {
		DeleteGlobalPersist (psFunctyData->psGlobalData);
		psFunctyData->psGlobalData = NULL;
	}

	if (psFunctyData->psVisData) {
		DeleteVisPersist (psFunctyData->psVisData);
		psFunctyData->psVisData = NULL;
	}

	if (psFunctyData->szFilename) {
		g_string_free (psFunctyData->szFilename, TRUE);
	}
	if (psFunctyData->szFolder) {
		g_string_free (psFunctyData->szFolder, TRUE);
	}

	if (psFunctyData->szFolderAnim) {
		g_string_free (psFunctyData->szFolderAnim, TRUE);
	}
	if (psFunctyData->szExportModelNamePLY) {
		g_string_free (psFunctyData->szExportModelNamePLY, TRUE);
	}
	if (psFunctyData->szExportModelNameSTL) {
		g_string_free (psFunctyData->szExportModelNameSTL, TRUE);
	}
	if (psFunctyData->szExportModelNameSVX) {
		g_string_free (psFunctyData->szExportModelNameSVX, TRUE);
	}
	if (psFunctyData->szExportModelNameVDB) {
		g_string_free (psFunctyData->szExportModelNameVDB, TRUE);
	}
	if (psFunctyData->szExportAnimName) {
		g_string_free (psFunctyData->szExportAnimName, TRUE);
	}
  if (psFunctyData->szExportBitmapName) {
    g_string_free (psFunctyData->szExportBitmapName, TRUE);
  }
  if (psFunctyData->szLicence) {
  	g_free (psFunctyData->szLicence);
  	psFunctyData->szLicence = NULL;
  }

	g_free (psFunctyData);
}

static void Realize (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
	GdkGLContext * psGlContext;
	GdkGLDrawable * psGlDrawable;
	bool boDrawSuccess;

	psGlContext = gtk_widget_get_gl_context (psWidget);
	psGlDrawable = gtk_widget_get_gl_drawable (psWidget);

	// Begin OpenGL setup
	boDrawSuccess = gdk_gl_drawable_gl_begin (psGlDrawable, psGlContext);
	if (boDrawSuccess) {
		Realise (psFunctyData->psVisData);

		gdk_gl_drawable_gl_end (psGlDrawable);
	}
	// End OpenGL setup
}

static gboolean ConfigureEvent (GtkWidget * psWidget, GdkEventConfigure * psEvent, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
	GdkGLContext * psGlContext;
	GdkGLDrawable * psGlDrawable;
	GLfloat fWidth;
	GLfloat fHeight;
	bool boDrawSuccess;

	psGlContext = gtk_widget_get_gl_context (psWidget);
	psGlDrawable = gtk_widget_get_gl_drawable (psWidget);

	fWidth = psWidget->allocation.width;
	fHeight = psWidget->allocation.height;

	// Begin OpenGL window resize
	boDrawSuccess = gdk_gl_drawable_gl_begin (psGlDrawable, psGlContext);
	if (boDrawSuccess) {
		Reshape (fWidth, fHeight, psFunctyData->psVisData);

		gdk_gl_drawable_gl_end (psGlDrawable);
	}
	// End OpenGL window resize

	return boDrawSuccess;
}

static gboolean ExposeEvent (GtkWidget * psWidget, GdkEventExpose * psEvent, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
	GdkGLContext * psGlContext;
	GdkGLDrawable * psGlDrawable;
	bool boDrawSuccess;
	bool boDoubleBuffered;

	psGlContext = gtk_widget_get_gl_context (psWidget);
	psGlDrawable = gtk_widget_get_gl_drawable (psWidget);

	// Begin OpenGL render
	boDrawSuccess = gdk_gl_drawable_gl_begin (psGlDrawable, psGlContext);
	if (boDrawSuccess) {
		// Redraw the visualisation
		Redraw (psFunctyData->psVisData);

		// Swap buffers
		boDoubleBuffered = gdk_gl_drawable_is_double_buffered (psGlDrawable);
		if (boDoubleBuffered) {
			gdk_gl_drawable_swap_buffers (psGlDrawable);
		}
		else {
			glFlush ();
		}

		gdk_gl_drawable_gl_end (psGlDrawable);
	}
	// End OpenGL render

	return boDrawSuccess;
}

static gboolean ButtonPressEvent (GtkWidget * psWidget, GdkEventButton * psEvent, gpointer * psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
	//gboolean boMoving;

	//boMoving = GetMoving (psFunctyData->psVisData);

	Mouse (psEvent->button, psEvent->type, psEvent->x, psEvent->y, psFunctyData->psVisData);

	return FALSE;
}

static gboolean ButtonReleaseEvent (GtkWidget * psWidget, GdkEventButton * psEvent, gpointer * psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;

	Mouse (psEvent->button, psEvent->type, psEvent->x, psEvent->y, psFunctyData->psVisData);

	return FALSE;
}

/*
static gboolean MotionNotifyEvent (GtkWidget * psWidget, GdkEventMotion * psEvent, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
	//float fWidth;
	//float fHeight;
	float fX;
	float fY;

	//fWidth = psWidget->allocation.width;
	//fHeight = psWidget->allocation.height;
	fX = psEvent->x;
	fY = psEvent->y;

	Motion ((int)fX, (int)fY, psFunctyData->psVisData);
	gdk_window_invalidate_rect (psWidget->window, & psWidget->allocation, FALSE);

	return TRUE;
}
*/

static gboolean KeyPressEvent (GtkWidget * psWidget, GdkEventKey * psEvent, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
	guint uModifiers;
	bool boCapture;
	bool boFullScreen;

	uModifiers = gtk_accelerator_get_default_mod_mask ();
	boFullScreen = GetFullScreen (psFunctyData->psVisData);

	if ((((psEvent->state & uModifiers) & (GDK_MOD1_MASK)) == 0) && !boFullScreen) {
		boCapture = FALSE;
	}
	else {
		boCapture = TRUE;
		switch (psEvent->keyval) {
			case GDK_Escape:
				ToggleFullScreenWindow (psFunctyData);
				break;
			case 'b':
				ToggleButtonBar (psFunctyData);
				break;
			case GDK_Return:
				if ((psEvent->state & uModifiers) == GDK_MOD1_MASK) {
					ToggleFullScreenWindow (psFunctyData);
					//SetViewWindowState (psFunctyData);
				}
				else {
					Key (psEvent->keyval, 0, 0, (psEvent->state & uModifiers), psFunctyData->psVisData);
					SetViewWindowState (psFunctyData);
				}
				break;
			default:
				Key (psEvent->keyval, 0, 0, (psEvent->state & uModifiers), psFunctyData->psVisData);
				SetViewWindowState (psFunctyData);
				break;
		}
	}

	return boCapture;
}

static gboolean KeyReleaseEvent (GtkWidget * psWidget, GdkEventKey * psEvent, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
	guint uModifiers;

	uModifiers = gtk_accelerator_get_default_mod_mask ();

	KeyUp (psEvent->keyval, 0, 0, (psEvent->state & uModifiers), psFunctyData->psVisData);

	return TRUE;
}

static gboolean ScrollEvent (GtkWidget * psWidget, GdkEventScroll * psEvent, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
	float fViewRadius;
	float fLinkLen;
	float fCentring;
	float fRigidity;
	float fForce;
	float fResistance;
	guint uModifiers;

	uModifiers = gtk_accelerator_get_default_mod_mask ();

	if ((psEvent->state & uModifiers) == GDK_CONTROL_MASK) {
		switch (psEvent->direction) {
			case GDK_SCROLL_UP:
				ScaleFunctionRange (RANGE_SCALEUP, psFunctyData->psVisData);
				break;
			case GDK_SCROLL_DOWN:
				ScaleFunctionRange (RANGE_SCALEDOWN, psFunctyData->psVisData);
				break;
			default:
				// Do nothing
				break;
		}
	}
	else {
		GetDisplayProperties (& fViewRadius, & fLinkLen, & fCentring, & fRigidity, & fForce, & fResistance, psFunctyData->psVisData);

		switch (psEvent->direction) {
			case GDK_SCROLL_UP:
				fViewRadius += RADIUS_SCROLL;
				break;
			case GDK_SCROLL_DOWN:
				fViewRadius -= RADIUS_SCROLL;
				break;
			default:
				// Do nothing
				break;
		}

		SetDisplayProperties (fViewRadius, fLinkLen, fCentring, fRigidity, fForce, fResistance, psFunctyData->psVisData);
	}

	return TRUE;
}

void ToggleButtonBar (FunctyPersist * psFunctyData) {

	psFunctyData->eButtonBarStyle = (BUTTONBARSTYLE)(((int)psFunctyData->eButtonBarStyle) + 1);
	if ((int)psFunctyData->eButtonBarStyle >= (int)BUTTONBARSTYLE_NUM) {
		psFunctyData->eButtonBarStyle = (BUTTONBARSTYLE)((int)BUTTONBARSTYLE_INVALID + 1);
	}

	SynchroniseButtonBarStyle (psFunctyData);
}

void ToggleFullScreenWindow (FunctyPersist * psFunctyData) {
	GtkWidget * psWindow;
	GtkWidget * psWidget;
	gboolean boCurrentState;

	boCurrentState = GetFullScreen (psFunctyData->psVisData);

	if (boCurrentState) {
		// Set to not fullscreen
		if (psFunctyData->boShowPanelBottom) {
			psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "PaneBottom"));
			gtk_widget_show (psWidget);
		}
		if (psFunctyData->boShowPanelLeft) {
			psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "PaneLeft"));
			gtk_widget_show (psWidget);
		}
		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuBar"));
		gtk_widget_show (psWidget);
		SynchroniseButtonBarStyle (psFunctyData);

		psWindow = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MainWindow"));
		gtk_window_unfullscreen (GTK_WINDOW (psWindow));
	}
	else {
		// Set to fullscreen
		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "PaneBottom"));
		gtk_widget_hide (psWidget);
		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "PaneLeft"));
		gtk_widget_hide (psWidget);
		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuBar"));
		gtk_widget_hide (psWidget);
		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "ButtonBar"));
		gtk_widget_hide (psWidget);

		psWindow = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MainWindow"));
		gtk_window_fullscreen (GTK_WINDOW (psWindow));
	}

	ToggleFullScreen (psFunctyData->psVisData);
}

void SetFullScreenWindow (bool boFullScreen, FunctyPersist * psFunctyData) {
	gboolean boCurrentState;

	boCurrentState = GetFullScreen (psFunctyData->psVisData);

	if (boFullScreen != boCurrentState) {
		ToggleFullScreenWindow (psFunctyData);
	}
}

void TogglePanelLeft (FunctyPersist * psFunctyData) {
	GtkWidget * psWidget;

	psFunctyData->boShowPanelLeft = !psFunctyData->boShowPanelLeft;
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "PaneLeft"));
	if (psFunctyData->boShowPanelLeft) {
		gtk_widget_show (psWidget);
	}
	else {
		gtk_widget_hide (psWidget);
	}
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuPanelLeft"));
	g_signal_handlers_block_by_func (psWidget, G_CALLBACK (TogglePanelLeftPress), psFunctyData);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (psWidget), psFunctyData->boShowPanelLeft);
	g_signal_handlers_unblock_by_func (psWidget, G_CALLBACK (TogglePanelLeftPress), psFunctyData);
}

void TogglePanelBottom (FunctyPersist * psFunctyData) {
	GtkWidget * psWidget;

	psFunctyData->boShowPanelBottom = !psFunctyData->boShowPanelBottom;
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "PaneBottom"));
	if (psFunctyData->boShowPanelBottom) {
		gtk_widget_show (psWidget);
	}
	else {
		gtk_widget_hide (psWidget);
	}
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuPanelBottom"));
	g_signal_handlers_block_by_func (psWidget, G_CALLBACK (TogglePanelBottomPress), psFunctyData);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (psWidget), psFunctyData->boShowPanelBottom);
	g_signal_handlers_unblock_by_func (psWidget, G_CALLBACK (TogglePanelBottomPress), psFunctyData);
}

static gboolean Timeout (gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
	int nX;
	int nY;
	GdkModifierType sState;

	if (psFunctyData->boTimeoutContinue) {
		// Update the pointer position
		// This differs from the standard motion callback, as it's called even if the pointer isn't moving
		// Update the range window
		if (UpdateRange (psFunctyData->psVisData)) {
			SetRangeWindowState (psFunctyData);
		}

		gdk_window_get_pointer (psFunctyData->psDrawingArea->window, & nX, & nY, & sState);
		Motion (nX, nY, psFunctyData->psVisData);

		// Call the idle function
		Idle (psFunctyData->psVisData);

		// Invalidate the drawing area
		gdk_window_invalidate_rect (psFunctyData->psDrawingArea->window, & psFunctyData->psDrawingArea->allocation, FALSE);

		// Update drawing area synchronously
		gdk_window_process_updates (psFunctyData->psDrawingArea->window, FALSE);
	}
	else {
		psFunctyData->TimeoutID = 0;
	}

	return psFunctyData->boTimeoutContinue;
}

static void TimeoutAdd (FunctyPersist * psFunctyData) {
	if (!psFunctyData->boPausedModal) {
		psFunctyData->boTimeoutContinue = TRUE;
		if (psFunctyData->TimeoutID == 0) {
			//psFunctyData->TimeoutID = g_timeout_add (UPDATE_TIMEOUT, Timeout, psFunctyData);
			psFunctyData->TimeoutID = g_timeout_add_full (G_PRIORITY_DEFAULT_IDLE, UPDATE_TIMEOUT, Timeout, psFunctyData, NULL);
			//psFunctyData->TimeoutID = g_timeout_add_full (G_PRIORITY_HIGH_IDLE, UPDATE_TIMEOUT, Timeout, psFunctyData, NULL);

			//psFunctyData->TimeoutID = g_idle_add (Timeout, psFunctyData);
		}
	}
}

static void TimeoutRemove (FunctyPersist * psFunctyData) {
	psFunctyData->boTimeoutContinue = FALSE;
}

static gboolean MapEvent (GtkWidget * psWidget, GdkEventAny * psEvent, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;

	TimeoutAdd (psFunctyData);

	return TRUE;
}

static gboolean UnmapEvent (GtkWidget * psWidget, GdkEventAny * psEvent, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;

	TimeoutRemove (psFunctyData);

	return TRUE;
}

static gboolean VisibilityNotifyEvent (GtkWidget * psWidget, GdkEventVisibility * psEvent, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;

	if (psEvent->state == GDK_VISIBILITY_FULLY_OBSCURED) {
		TimeoutRemove (psFunctyData);
	}
	else {
		TimeoutAdd (psFunctyData);
	}

	return TRUE;
}

//static gboolean ViewPress (GtkWidget * psWidget, gpointer psData) {
//	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
//	GtkWidget * psFunction;
//
//	SetViewWindowState (psFunctyData);
//
//	psFunction = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "View"));
//	gtk_widget_show (psFunction);
//
//	return TRUE;
//}

//static gboolean RangePress (GtkWidget * psWidget, gpointer psData) {
//	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
//	GtkWidget * psFunction;
//
//	SetRangeWindowState (psFunctyData);
//
//	psFunction = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Range"));
//	gtk_widget_show (psFunction);
//
//	return TRUE;
//}

//static gboolean RangeSetPress (GtkWidget * psWidget, gpointer psData) {
//	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
//	GtkWidget * psFunction;
//
//	psFunction = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Range"));
//	gtk_widget_hide (psFunction);
//
//	return RangeSetApply (psWidget, psData);
//}

static gboolean RangeSetApply (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
	double afRange[6];

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "XMin"));
	afRange[0] = strtod (gtk_entry_get_text (GTK_ENTRY (psWidget)), NULL);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "YMin"));
	afRange[1] = strtod (gtk_entry_get_text (GTK_ENTRY (psWidget)), NULL);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "ZMin"));
	afRange[2] = strtod (gtk_entry_get_text (GTK_ENTRY (psWidget)), NULL);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "XWidth"));
	afRange[3] = strtod (gtk_entry_get_text (GTK_ENTRY (psWidget)), NULL);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "YWidth"));
	afRange[4] = strtod (gtk_entry_get_text (GTK_ENTRY (psWidget)), NULL);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "ZWidth"));
	afRange[5] = strtod (gtk_entry_get_text (GTK_ENTRY (psWidget)), NULL);

	SetVisRange (afRange, psFunctyData->psVisData);

	return TRUE;
}

static gboolean RangeSetCancel (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;

	SetRangeWindowState (psFunctyData);

	return TRUE;
}

gboolean RangeMinChange (GtkWidget *psWidget, GdkEventKey * psEvent, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
	double fMin;
	double fWidth;
	GtkWidget * psEntry;
	GString * szValue;

	szValue = g_string_new ("");

	psEntry = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "XMin"));
	fMin = strtod (gtk_entry_get_text (GTK_ENTRY (psEntry)), NULL);
	psEntry = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "XWidth"));
	fWidth = strtod (gtk_entry_get_text (GTK_ENTRY (psEntry)), NULL);
	g_string_printf (szValue, RANGE_FORMAT, (fMin + fWidth));
	psEntry = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "XMax"));
	gtk_entry_set_text (GTK_ENTRY (psEntry), szValue->str);

	psEntry = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "YMin"));
	fMin = strtod (gtk_entry_get_text (GTK_ENTRY (psEntry)), NULL);
	psEntry = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "YWidth"));
	fWidth = strtod (gtk_entry_get_text (GTK_ENTRY (psEntry)), NULL);
	g_string_printf (szValue, RANGE_FORMAT, (fMin + fWidth));
	psEntry = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "YMax"));
	gtk_entry_set_text (GTK_ENTRY (psEntry), szValue->str);

	psEntry = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "ZMin"));
	fMin = strtod (gtk_entry_get_text (GTK_ENTRY (psEntry)), NULL);
	psEntry = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "ZWidth"));
	fWidth = strtod (gtk_entry_get_text (GTK_ENTRY (psEntry)), NULL);
	g_string_printf (szValue, RANGE_FORMAT, (fMin + fWidth));
	psEntry = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "ZMax"));
	gtk_entry_set_text (GTK_ENTRY (psEntry), szValue->str);

	g_string_free (szValue, TRUE);

	return FALSE;
}

gboolean RangeMaxChange (GtkWidget *psWidget, GdkEventKey * psEvent, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
	double fMin;
	double fMax;
	GtkWidget * psEntry;
	GString * szValue;

	szValue = g_string_new ("");

	psEntry = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "XMin"));
	fMin = strtod (gtk_entry_get_text (GTK_ENTRY (psEntry)), NULL);
	psEntry = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "XMax"));
	fMax = strtod (gtk_entry_get_text (GTK_ENTRY (psEntry)), NULL);
	g_string_printf (szValue, RANGE_FORMAT, (fMax - fMin));
	psEntry = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "XWidth"));
	gtk_entry_set_text (GTK_ENTRY (psEntry), szValue->str);

	psEntry = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "YMin"));
	fMin = strtod (gtk_entry_get_text (GTK_ENTRY (psEntry)), NULL);
	psEntry = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "YMax"));
	fMax = strtod (gtk_entry_get_text (GTK_ENTRY (psEntry)), NULL);
	g_string_printf (szValue, RANGE_FORMAT, (fMax - fMin));
	psEntry = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "YWidth"));
	gtk_entry_set_text (GTK_ENTRY (psEntry), szValue->str);

	psEntry = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "ZMin"));
	fMin = strtod (gtk_entry_get_text (GTK_ENTRY (psEntry)), NULL);
	psEntry = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "ZMax"));
	fMax = strtod (gtk_entry_get_text (GTK_ENTRY (psEntry)), NULL);
	g_string_printf (szValue, RANGE_FORMAT, (fMax - fMin));
	psEntry = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "ZWidth"));
	gtk_entry_set_text (GTK_ENTRY (psEntry), szValue->str);

	g_string_free (szValue, TRUE);

	return FALSE;
}

//static gboolean PlotFunctionPress (GtkWidget * psWidget, gpointer psData) {
//	//FunctyPersist * psFunctyData = (FunctyPersist * )psData;
//	//GtkWidget * psFunction;
//
//	//psFunction = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Function"));
//	//gtk_widget_hide (psFunction);
//
//	return PlotFunctionApply (psWidget, psData);
//}

static gboolean PlotFunctionApply (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
	GtkWidget * psFunction;
	GtkWidget * psFunctionList;
	GtkWidget * psColourWidget;
	GtkWidget * psTexWidget;
	GtkWidget * psPhysicalWidget;
	gchar * szFunction;
	char * szFunctionEdit;
	FuncPersist * psFuncData;
	GtkListStore * psFunctionListModel;
	GtkWidget * psWidgetName;
	GtkWidget * psWidgetAcc;
	double fAccuracy;
	float fThickness;
	bool boFill;
	char * szRed;
	char * szGreen;
	char * szBlue;
	char * szAlpha;
	char * szXCentre;
	char * szYCentre;
	char * szZCentre;
	FUNCTYPE eFuncType;
	char const * szName;
	char const * szFilename;
	char * szXScale;
	char * szYScale;
	char * szXOffset;
	char * szYOffset;
	gchar * szXFunction;
	gchar * szYFunction;
	gchar * szZFunction;

	psColourWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Red"));
	szRed = GetTextViewText (psColourWidget);
	psColourWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Green"));
	szGreen = GetTextViewText (psColourWidget);
	psColourWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Blue"));
	szBlue = GetTextViewText (psColourWidget);
	psColourWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Alpha"));
	szAlpha = GetTextViewText (psColourWidget);

	psTexWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TexFile"));
	szFilename = gtk_entry_get_text (GTK_ENTRY (psTexWidget));
	psTexWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TexXScale"));
	szXScale = GetTextViewText (psTexWidget);
	psTexWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TexYScale"));
	szYScale = GetTextViewText (psTexWidget);
	psTexWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TexXOffset"));
	szXOffset = GetTextViewText (psTexWidget);
	psTexWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TexYOffset"));
	szYOffset = GetTextViewText (psTexWidget);

	psFunctionList = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "FunctionList"));

	if (psFunctyData->psFuncEdit) {
		eFuncType = GetFunctionType (psFunctyData->psFuncEdit);

		// Amend the selected function
		psWidgetName = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "FunctionName"));
		szName = gtk_entry_get_text (GTK_ENTRY (psWidgetName));
		SetFunctionName (szName, psFunctyData->psFuncEdit);

		SetFunctionColours (szRed, szGreen, szBlue, szAlpha, psFunctyData->psFuncEdit);
		SetTextureValues (szFilename, szXScale, szYScale, szXOffset, szYOffset, psFunctyData->psFuncEdit);

		// Set function physical properties
		psPhysicalWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MaterialThickness"));
		fThickness = gtk_spin_button_get_value (GTK_SPIN_BUTTON (psPhysicalWidget));
		SetFunctionMaterialThickness (fThickness, psFunctyData->psFuncEdit);
		psPhysicalWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MaterialFill"));
		boFill = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (psPhysicalWidget));
		SetFunctionMaterialFill (boFill, psFunctyData->psFuncEdit);

		switch (eFuncType) {
		case FUNCTYPE_SPHERICAL:
			psFunction = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "FunctionEdit"));
			szFunctionEdit = GetTextViewText (psFunction);
			SetFunction (szFunctionEdit, psFunctyData->psFuncEdit);
			g_free (szFunctionEdit);

			// Set up the centre functions
			psColourWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "XCentre"));
			szXCentre = GetTextViewText (psColourWidget);
			psColourWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "YCentre"));
			szYCentre = GetTextViewText (psColourWidget);
			psColourWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "ZCentre"));
			szZCentre = GetTextViewText (psColourWidget);

			SphericalSetFunctionCentre (szXCentre, szYCentre, szZCentre, psFunctyData->psFuncEdit);

			g_free (szXCentre);
			g_free (szYCentre);
			g_free (szZCentre);

			psWidgetAcc = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Accuracy"));
			fAccuracy = (1.0 / gtk_spin_button_get_value (GTK_SPIN_BUTTON (psWidgetAcc)));
			SetFunctionAccuracy (fAccuracy, psFunctyData->psFuncEdit);
			break;
		case FUNCTYPE_CARTESIAN:
			psFunction = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "FunctionEdit"));
			szFunctionEdit = GetTextViewText (psFunction);
			SetFunction (szFunctionEdit, psFunctyData->psFuncEdit);
			g_free (szFunctionEdit);

			psWidgetAcc = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Accuracy"));
			fAccuracy = (1.0 / gtk_spin_button_get_value (GTK_SPIN_BUTTON (psWidgetAcc)));
			SetFunctionAccuracy (fAccuracy, psFunctyData->psFuncEdit);
			break;
		case FUNCTYPE_CURVE:
			psFunction = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "RadiusEdit"));
			szFunction = GetTextViewText (psFunction);
			psFunction = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "CurveXEdit"));
			szXFunction = GetTextViewText (psFunction);
			psFunction = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "CurveYEdit"));
			szYFunction = GetTextViewText (psFunction);
			psFunction = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "CurveZEdit"));
			szZFunction = GetTextViewText (psFunction);
			CurveSetFunction (szXFunction, szYFunction, szZFunction, szFunction, psFunctyData->psFuncEdit);
			g_free (szFunction);
			g_free (szXFunction);
			g_free (szYFunction);
			g_free (szZFunction);

			// Set up the centre functions
			psColourWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "XCentre"));
			szXCentre = GetTextViewText (psColourWidget);
			psColourWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "YCentre"));
			szYCentre = GetTextViewText (psColourWidget);
			psColourWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "ZCentre"));
			szZCentre = GetTextViewText (psColourWidget);

			CurveSetFunctionCentre (szXCentre, szYCentre, szZCentre, psFunctyData->psFuncEdit);

			g_free (szXCentre);
			g_free (szYCentre);
			g_free (szZCentre);

			psWidgetAcc = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "AccuracyRadiusCurve"));
			fAccuracy = (1.0 / gtk_spin_button_get_value (GTK_SPIN_BUTTON (psWidgetAcc)));
			CurveSetFunctionAccuracyRadius (fAccuracy, psFunctyData->psFuncEdit);

			psWidgetAcc = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "AccuracyCurve"));
			fAccuracy = (1.0 / gtk_spin_button_get_value (GTK_SPIN_BUTTON (psWidgetAcc)));
			SetFunctionAccuracy (fAccuracy, psFunctyData->psFuncEdit);
			break;
		default:
			// Do nothing
			break;
		}

		AssignControlVarsSingle (psFunctyData->psFuncEdit, psFunctyData->psVisData);

		PopulateVertices (psFunctyData->psFuncEdit);

		gdk_window_invalidate_rect (psFunctionList->window, NULL, TRUE);
	}
	else {
		// Add new function
		psFuncData = AddNewFunction (psFunctyData->eNextFunctionType, psFunctyData->psVisData);
		SetFunctionColours (szRed, szGreen, szBlue, szAlpha, psFuncData);
		SetTextureValues (szFilename, szXScale, szYScale, szXOffset, szYOffset, psFuncData);

		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "FunctionName"));
		szName = gtk_entry_get_text (GTK_ENTRY (psWidget));
		SetFunctionName (szName, psFuncData);

		switch (psFunctyData->eNextFunctionType) {
		case FUNCTYPE_SPHERICAL:
			psWidgetAcc = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Accuracy"));
			fAccuracy = (1.0 / gtk_spin_button_get_value (GTK_SPIN_BUTTON (psWidgetAcc)));
			SetFunctionAccuracy (fAccuracy, psFuncData);

			psFunction = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "FunctionEdit"));
			szFunctionEdit = GetTextViewText (psFunction);
			SetFunction (szFunctionEdit, psFuncData);
			g_free (szFunctionEdit);

			// Set up the centre functions
			psColourWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "XCentre"));
			szXCentre = GetTextViewText (psColourWidget);
			psColourWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "YCentre"));
			szYCentre = GetTextViewText (psColourWidget);
			psColourWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "ZCentre"));
			szZCentre = GetTextViewText (psColourWidget);

			SphericalSetFunctionCentre (szXCentre, szYCentre, szZCentre, psFuncData);

			g_free (szXCentre);
			g_free (szYCentre);
			g_free (szZCentre);
			break;
		case FUNCTYPE_CARTESIAN:
			psWidgetAcc = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Accuracy"));
			fAccuracy = (1.0 / gtk_spin_button_get_value (GTK_SPIN_BUTTON (psWidgetAcc)));
			SetFunctionAccuracy (fAccuracy, psFuncData);

			psFunction = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "FunctionEdit"));
			szFunctionEdit = GetTextViewText (psFunction);
			SetFunction (szFunctionEdit, psFuncData);
			g_free (szFunctionEdit);
			break;
		case FUNCTYPE_CURVE:
			psWidgetAcc = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "AccuracyRadiusCurve"));
			fAccuracy = (1.0 / gtk_spin_button_get_value (GTK_SPIN_BUTTON (psWidgetAcc)));
			CurveSetFunctionAccuracyRadius (fAccuracy, psFuncData);

			psWidgetAcc = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "AccuracyCurve"));
			fAccuracy = (1.0 / gtk_spin_button_get_value (GTK_SPIN_BUTTON (psWidgetAcc)));
			SetFunctionAccuracy (fAccuracy, psFuncData);

			psFunction = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "RadiusEdit"));
			szFunction = GetTextViewText (psFunction);
			psFunction = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "CurveXEdit"));
			szXFunction = GetTextViewText (psFunction);
			psFunction = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "CurveYEdit"));
			szYFunction = GetTextViewText (psFunction);
			psFunction = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "CurveZEdit"));
			szZFunction = GetTextViewText (psFunction);
			CurveSetFunction (szXFunction, szYFunction, szZFunction, szFunction, psFuncData);
			g_free (szFunction);
			g_free (szXFunction);
			g_free (szYFunction);
			g_free (szZFunction);

			// Set up the centre functions
			psColourWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "XCentre"));
			szXCentre = GetTextViewText (psColourWidget);
			psColourWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "YCentre"));
			szYCentre = GetTextViewText (psColourWidget);
			psColourWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "ZCentre"));
			szZCentre = GetTextViewText (psColourWidget);

			CurveSetFunctionCentre (szXCentre, szYCentre, szZCentre, psFuncData);

			g_free (szXCentre);
			g_free (szYCentre);
			g_free (szZCentre);
			break;
		default:
			// Do nothing
			break;
		}

		TransferFunctionRange (psFuncData, psFunctyData->psVisData);

		psFunctionListModel = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (psFunctionList)));
		AddFunction (psFuncData, psFunctionListModel);

		psFunctyData->psFuncEdit = psFuncData;

		AssignControlVarsSingle (psFunctyData->psFuncEdit, psFunctyData->psVisData);
	}

	g_free (szRed);
	g_free (szGreen);
	g_free (szBlue);
	g_free (szAlpha);

	g_free (szXScale);
	g_free (szYScale);
	g_free (szXOffset);
	g_free (szYOffset);

	FunctionShadersRegenerate (psFunctyData->psFuncEdit);

	SetColourButton (psFunctyData);
	SetControlvarWindowState (psFunctyData);

	return TRUE;
}

static gboolean PlotFunctionCancel (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;

	if (psFunctyData->psFuncEdit != NULL) {
		EditFunction (psFunctyData->psFuncEdit, psFunctyData);
	}
	else {
		SetFunctionPropertiesWindow (FUNCTYPE_INVALID, FALSE, psFunctyData);
	}

	return TRUE;
}

static void ClearAllFunctions (FunctyPersist * psFunctyData) {
	//GtkWidget * psFunction;
	GtkListStore * psFunctionListModel;
	GtkWidget * psFunctionList;

	// Close the selected function window
	//psFunction = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Function"));
	//gtk_widget_hide (psFunction);

	// No function selected
	psFunctyData->psFuncEdit = NULL;

	// Remove all of the rows from the function list
	psFunctionList = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "FunctionList"));
	psFunctionListModel = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (psFunctionList)));
	gtk_list_store_clear (psFunctionListModel);

	// Delete all of the functions
	DeleteAllFunctions (psFunctyData->psVisData);
}

static gboolean DeleteFunctionPress (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
	//GtkWidget * psFunction;
	GtkWidget * psFunctionList;
	FuncPersist * psFuncData;
	GtkListStore * psFunctionListModel;
	GtkTreeSelection * psSelection;
	gboolean boFound;
	GtkTreeModel * psTreeModel;
	GtkTreeIter sTreeIter;

	psFunctionList = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "FunctionList"));

	psSelection = gtk_tree_view_get_selection	(GTK_TREE_VIEW (psFunctionList));

	boFound = gtk_tree_selection_get_selected (psSelection, & psTreeModel, & sTreeIter);

	if (boFound) {
		gtk_tree_model_get (psTreeModel, & sTreeIter, FUNCSCOL_FUNCDATA, & psFuncData, -1);

		// Only remove it if it is a function, and not the "New" option
		if (psFuncData) {
			if (psFuncData == psFunctyData->psFuncEdit) {
				// Close the selected function window
				//psFunction = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Function"));
				//gtk_widget_hide (psFunction);
				SetFunctionPropertiesWindow (FUNCTYPE_INVALID, FALSE, psFunctyData);

				// No function selected
				psFunctyData->psFuncEdit = NULL;
			}

			// Remove the currently selected row from the UI list
			psFunctionListModel = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (psFunctionList)));
			gtk_list_store_remove (psFunctionListModel, & sTreeIter);

			// Delete the function from the function list
			DeleteFunction (psFuncData, psFunctyData->psVisData);
		}
	}

	return TRUE;
}

static gboolean ToggleSpinPress (GtkToggleButton * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
	bool boActive;

	boActive = !GetSpin (psFunctyData->psVisData);
	SetSpin (boActive, psFunctyData->psVisData);
	SetViewSpin (psFunctyData);

	return TRUE;
}

static gboolean ToggleInvertPress (GtkToggleButton * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
	bool boActive;

	boActive = !GetClearWhite (psFunctyData->psVisData);
	SetClearWhite (boActive, psFunctyData->psVisData);
	SetViewInvert (psFunctyData);

	return TRUE;
}

static gboolean FullscreenPress (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;

	ToggleFullScreenWindow (psFunctyData);

	return TRUE;
}

static gboolean TogglePanelLeftPress (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;

	TogglePanelLeft (psFunctyData);

	return TRUE;
}

static gboolean TogglePanelBottomPress (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;

	TogglePanelBottom (psFunctyData);

	return TRUE;
}

static gboolean ToggleDrawAxesPress (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
	bool boActive;

	boActive = !GetDrawAxes (psFunctyData->psVisData);
	SetDrawAxes (boActive, psFunctyData->psVisData);
	SetViewDrawAxis (psFunctyData);

	return TRUE;
}

static gboolean ToggleWireframePress (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
	bool boActive;

	boActive = !GetWireframe (psFunctyData->psVisData);
	SetWireframe (boActive, psFunctyData->psVisData);
	SetViewWireframe (psFunctyData);

	return TRUE;
}

static gboolean ToggleUseShadersPress (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
	bool boActive;

	boActive = !GetShadersActive (psFunctyData->psVisData);
	SetShadersActive (boActive, psFunctyData->psVisData);
	SetViewUseShaders (psFunctyData);

	return TRUE;
}

static gboolean ToggleShadowPress (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
	bool boActive;

	boActive = !GetShadow (psFunctyData->psVisData);
	SetShadow (boActive, psFunctyData->psVisData);

	SetViewShadow (psFunctyData);

	return TRUE;
}

static gboolean ToggleFocusBlurPress (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
	bool boActive;

	boActive = !GetFocusBlur (psFunctyData->psVisData);
	SetFocusBlur (boActive, psFunctyData->psVisData);
	SetViewFocusBlur (psFunctyData);

	return TRUE;
}

static gboolean TogglePausePress (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
	bool boActive;

	boActive = !GetPauseAnimation (psFunctyData->psVisData);
	SetPauseAnimation (boActive, psFunctyData->psVisData);
	SetViewPauseAnimation (psFunctyData);

	return TRUE;
}

void SetViewDrawAxis (FunctyPersist * psFunctyData) {
	GtkWidget * psWidget;
	bool boActive;
	static const GCallback pfCallback = G_CALLBACK (ToggleDrawAxesPress);

	boActive = GetDrawAxes (psFunctyData->psVisData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "DrawAxes"));
	g_signal_handlers_block_by_func (psWidget, pfCallback, psFunctyData);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (psWidget), boActive);
	g_signal_handlers_unblock_by_func (psWidget, pfCallback, psFunctyData);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuDrawAxes"));
	g_signal_handlers_block_by_func (psWidget, pfCallback, psFunctyData);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (psWidget), boActive);
	g_signal_handlers_unblock_by_func (psWidget, pfCallback, psFunctyData);
}

void SetViewSpin (FunctyPersist * psFunctyData) {
	GtkWidget * psWidget;
	bool boActive;
	static const GCallback pfCallback = G_CALLBACK (ToggleSpinPress);

	boActive = GetSpin (psFunctyData->psVisData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Spin"));
	g_signal_handlers_block_by_func (psWidget, pfCallback, psFunctyData);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (psWidget), boActive);
	g_signal_handlers_unblock_by_func (psWidget, pfCallback, psFunctyData);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuSpin"));
	g_signal_handlers_block_by_func (psWidget, pfCallback, psFunctyData);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (psWidget), boActive);
	g_signal_handlers_unblock_by_func (psWidget, pfCallback, psFunctyData);
}

void SetViewInvert (FunctyPersist * psFunctyData) {
	GtkWidget * psWidget;
	bool boActive;
	static const GCallback pfCallback = G_CALLBACK (ToggleInvertPress);

	boActive = GetClearWhite (psFunctyData->psVisData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Invert"));
	g_signal_handlers_block_by_func (psWidget, pfCallback, psFunctyData);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (psWidget), boActive);
	g_signal_handlers_unblock_by_func (psWidget, pfCallback, psFunctyData);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuInvert"));
	g_signal_handlers_block_by_func (psWidget, pfCallback, psFunctyData);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (psWidget), boActive);
	g_signal_handlers_unblock_by_func (psWidget, pfCallback, psFunctyData);
}

void SetViewWireframe (FunctyPersist * psFunctyData) {
	GtkWidget * psWidget;
	bool boActive;
	static const GCallback pfCallback = G_CALLBACK (ToggleWireframePress);

	boActive = GetWireframe (psFunctyData->psVisData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Wireframe"));
	g_signal_handlers_block_by_func (psWidget, pfCallback, psFunctyData);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (psWidget), boActive);
	g_signal_handlers_unblock_by_func (psWidget, pfCallback, psFunctyData);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuWireframe"));
	g_signal_handlers_block_by_func (psWidget, pfCallback, psFunctyData);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (psWidget), boActive);
	g_signal_handlers_unblock_by_func (psWidget, pfCallback, psFunctyData);
}

void SetViewUseShaders (FunctyPersist * psFunctyData) {
	GtkWidget * psWidget;
	bool boActive;
	static const GCallback pfCallback = G_CALLBACK (ToggleUseShadersPress);

	boActive = GetShadersActive (psFunctyData->psVisData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "UseShaders"));
	g_signal_handlers_block_by_func (psWidget, pfCallback, psFunctyData);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (psWidget), boActive);
	g_signal_handlers_unblock_by_func (psWidget, pfCallback, psFunctyData);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuUseShaders"));
	g_signal_handlers_block_by_func (psWidget, pfCallback, psFunctyData);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (psWidget), boActive);
	g_signal_handlers_unblock_by_func (psWidget, pfCallback, psFunctyData);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "ShaderSettings"));
	gtk_widget_set_sensitive (psWidget, boActive);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuShadow"));
	gtk_widget_set_sensitive (psWidget, boActive);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuFocusBlur"));
	gtk_widget_set_sensitive (psWidget, boActive);
}

void SetViewShadow (FunctyPersist * psFunctyData) {
	GtkWidget * psWidget;
	bool boActive;
	static const GCallback pfCallback = G_CALLBACK (ToggleShadowPress);

	boActive = GetShadow (psFunctyData->psVisData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Shadow"));
	g_signal_handlers_block_by_func (psWidget, pfCallback, psFunctyData);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (psWidget), boActive);
	g_signal_handlers_unblock_by_func (psWidget, pfCallback, psFunctyData);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuShadow"));
	g_signal_handlers_block_by_func (psWidget, pfCallback, psFunctyData);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (psWidget), boActive);
	g_signal_handlers_unblock_by_func (psWidget, pfCallback, psFunctyData);
}

void SetViewFocusBlur (FunctyPersist * psFunctyData) {
	GtkWidget * psWidget;
	bool boActive;
	static const GCallback pfCallback = G_CALLBACK (ToggleFocusBlurPress);

	boActive = GetFocusBlur (psFunctyData->psVisData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "FocusBlur"));
	g_signal_handlers_block_by_func (psWidget, pfCallback, psFunctyData);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (psWidget), boActive);
	g_signal_handlers_unblock_by_func (psWidget, pfCallback, psFunctyData);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuFocusBlur"));
	g_signal_handlers_block_by_func (psWidget, pfCallback, psFunctyData);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (psWidget), boActive);
	g_signal_handlers_unblock_by_func (psWidget, pfCallback, psFunctyData);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "FocusBlurRanges"));
	gtk_widget_set_sensitive (psWidget, boActive);
}

void SetViewPauseAnimation (FunctyPersist * psFunctyData) {
	GtkWidget * psWidget;
	bool boActive;
	static const GCallback pfCallback = G_CALLBACK (TogglePausePress);

	boActive = GetPauseAnimation (psFunctyData->psVisData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Pause"));
	if (boActive) {
		gtk_tool_button_set_stock_id (GTK_TOOL_BUTTON (psWidget), GTK_STOCK_MEDIA_PLAY);
		gtk_tool_button_set_label (GTK_TOOL_BUTTON (psWidget), "Play");
	}
	else {
		gtk_tool_button_set_stock_id (GTK_TOOL_BUTTON (psWidget), GTK_STOCK_MEDIA_PAUSE);
		gtk_tool_button_set_label (GTK_TOOL_BUTTON (psWidget), "Pause");
	}

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuPause"));
	g_signal_handlers_block_by_func (psWidget, pfCallback, psFunctyData);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (psWidget), boActive);
	g_signal_handlers_unblock_by_func (psWidget, pfCallback, psFunctyData);
}

void SetViewWindowState (FunctyPersist * psFunctyData) {
	// Draw axis UI elements
	SetViewDrawAxis (psFunctyData);

	// Draw the spin UI elements
	SetViewSpin (psFunctyData);

	// Draw the invert UI elements
	SetViewInvert (psFunctyData);

	// Draw the wireframe UI elements
	SetViewWireframe (psFunctyData);

	// Draw the use shaders UI elements
	SetViewUseShaders (psFunctyData);
	SetViewShadow (psFunctyData);
	SetViewFocusBlur (psFunctyData);
	SetFocusBlurNearState (psFunctyData);
	SetFocusFarNearState (psFunctyData);

	// Draw the pause UI elements
	SetViewPauseAnimation (psFunctyData);
}

void SetMaterialFill (bool boMaterialFill, FunctyPersist * psFunctyData) {
	GtkWidget * psWidgetThickness;

	if (psFunctyData->psFuncEdit) {
		SetFunctionMaterialFill (boMaterialFill, psFunctyData->psFuncEdit);

		psWidgetThickness = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MaterialThickness"));
		gtk_widget_set_sensitive (psWidgetThickness, !boMaterialFill);
		psWidgetThickness = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MaterialThicknessLabel"));
		gtk_widget_set_sensitive (psWidgetThickness, !boMaterialFill);
	}
}

static void ToggleMaterialFill (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
	bool boMaterialFill;

	if (psFunctyData->psFuncEdit) {
		boMaterialFill = !GetFunctionMaterialFill (psFunctyData->psFuncEdit);
		SetMaterialFill (boMaterialFill, psFunctyData);
	}
}

static void SetButtonBarStyleNone (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;

	psFunctyData->eButtonBarStyle = BUTTONBARSTYLE_NONE;
	SynchroniseButtonBarStyle (psFunctyData);
}

static void SetButtonBarStyleIcons (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;

	psFunctyData->eButtonBarStyle = BUTTONBARSTYLE_ICONS;
	SynchroniseButtonBarStyle (psFunctyData);
}

static void SetButtonBarStyleIconsText (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;

	psFunctyData->eButtonBarStyle = BUTTONBARSTYLE_ICONSTEXT;
	SynchroniseButtonBarStyle (psFunctyData);
}

void SynchroniseButtonBarStyle (FunctyPersist * psFunctyData) {
	GtkWidget * psWidget;

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuButtonBarNone"));
	g_signal_handlers_block_by_func (psWidget, G_CALLBACK (SetButtonBarStyleNone), psFunctyData);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (psWidget), (psFunctyData->eButtonBarStyle == BUTTONBARSTYLE_NONE));
	g_signal_handlers_unblock_by_func (psWidget, G_CALLBACK (SetButtonBarStyleNone), psFunctyData);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuButtonBarIcons"));
	g_signal_handlers_block_by_func (psWidget, G_CALLBACK (SetButtonBarStyleIcons), psFunctyData);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (psWidget), (psFunctyData->eButtonBarStyle == BUTTONBARSTYLE_ICONS));
	g_signal_handlers_unblock_by_func (psWidget, G_CALLBACK (SetButtonBarStyleIcons), psFunctyData);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuButtonBarIconsText"));
	g_signal_handlers_block_by_func (psWidget, G_CALLBACK (SetButtonBarStyleIconsText), psFunctyData);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (psWidget), (psFunctyData->eButtonBarStyle == BUTTONBARSTYLE_ICONSTEXT));
	g_signal_handlers_unblock_by_func (psWidget, G_CALLBACK (SetButtonBarStyleIconsText), psFunctyData);
	
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "ButtonBar"));
	switch (psFunctyData->eButtonBarStyle) {
		case BUTTONBARSTYLE_NONE:
			gtk_widget_hide (psWidget);
			break;
		case BUTTONBARSTYLE_ICONS:
			gtk_toolbar_set_style (GTK_TOOLBAR (psWidget), GTK_TOOLBAR_ICONS);
			gtk_widget_show (psWidget);
			break;
		case BUTTONBARSTYLE_ICONSTEXT:
			gtk_toolbar_set_style (GTK_TOOLBAR (psWidget), GTK_TOOLBAR_BOTH);
			gtk_widget_show (psWidget);
			break;
		default:
			printf ("Invalid button bar state.\n");
			break;
	}
}

void SynchronisePanels (FunctyPersist * psFunctyData) {
	GtkWidget * psWidget;

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "PaneLeft"));
	if (psFunctyData->boShowPanelLeft) {
		gtk_widget_show (psWidget);
	}
	else {
		gtk_widget_hide (psWidget);
	}
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuPanelLeft"));
	g_signal_handlers_block_by_func (psWidget, G_CALLBACK (TogglePanelLeftPress), psFunctyData);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (psWidget), psFunctyData->boShowPanelLeft);
	g_signal_handlers_unblock_by_func (psWidget, G_CALLBACK (TogglePanelLeftPress), psFunctyData);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "PaneBottom"));
	if (psFunctyData->boShowPanelBottom) {
		gtk_widget_show (psWidget);
	}
	else {
		gtk_widget_hide (psWidget);
	}
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuPanelBottom"));
	g_signal_handlers_block_by_func (psWidget, G_CALLBACK (TogglePanelBottomPress), psFunctyData);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (psWidget), psFunctyData->boShowPanelBottom);
	g_signal_handlers_unblock_by_func (psWidget, G_CALLBACK (TogglePanelBottomPress), psFunctyData);
}


void SetRangeWindowState (FunctyPersist * psFunctyData) {
	GtkWidget * psWidget;
	GString * szValue;
	double afRange[6];

	GetVisRange (afRange, psFunctyData->psVisData);

	szValue = g_string_new ("");

	g_string_printf (szValue, RANGE_FORMAT, afRange[0]);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "XMin"));
	gtk_entry_set_text (GTK_ENTRY (psWidget), szValue->str);

	g_string_printf (szValue, RANGE_FORMAT, afRange[1]);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "YMin"));
	gtk_entry_set_text (GTK_ENTRY (psWidget), szValue->str);

	g_string_printf (szValue, RANGE_FORMAT, afRange[2]);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "ZMin"));
	gtk_entry_set_text (GTK_ENTRY (psWidget), szValue->str);

	g_string_printf (szValue, RANGE_FORMAT, afRange[3]);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "XWidth"));
	gtk_entry_set_text (GTK_ENTRY (psWidget), szValue->str);

	g_string_printf (szValue, RANGE_FORMAT, afRange[4]);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "YWidth"));
	gtk_entry_set_text (GTK_ENTRY (psWidget), szValue->str);

	g_string_printf (szValue, RANGE_FORMAT, afRange[5]);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "ZWidth"));
	gtk_entry_set_text (GTK_ENTRY (psWidget), szValue->str);

	g_string_printf (szValue, RANGE_FORMAT, afRange[0] + afRange[3]);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "XMax"));
	gtk_entry_set_text (GTK_ENTRY (psWidget), szValue->str);

	g_string_printf (szValue, RANGE_FORMAT, afRange[1] + afRange[4]);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "YMax"));
	gtk_entry_set_text (GTK_ENTRY (psWidget), szValue->str);

	g_string_printf (szValue, RANGE_FORMAT, afRange[2] + afRange[5]);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "ZMax"));
	gtk_entry_set_text (GTK_ENTRY (psWidget), szValue->str);

	g_string_free (szValue, TRUE);
}

void PopulateFunctionList (GtkListStore * psFunctionListModel, FunctyPersist * psFunctyData) {
	GSList * psFuncList;

	psFuncList = GetFunctionList (psFunctyData->psVisData);
	//AddFunction (NULL, psFunctionListModel);

	while (psFuncList) {
		AddFunction ((FuncPersist *)(psFuncList->data), psFunctionListModel);
		psFuncList = g_slist_next (psFuncList);
	}
}

void AddFunction (FuncPersist * psFuncData, GtkListStore * psFunctionListModel) {
	GtkTreeIter sListIter;

	gtk_list_store_append (psFunctionListModel, & sListIter);
	gtk_list_store_set (psFunctionListModel, & sListIter, FUNCSCOL_FUNCDATA, psFuncData, FUNCSCOL_FUNCTION, "", -1);
}

void FunctionCellDataFunc (GtkTreeViewColumn * psTreeColumn, GtkCellRenderer * psCell, GtkTreeModel * psTreeModel, GtkTreeIter * psIter, gpointer psData) {
	FuncPersist * psFuncData;
	char const * szFunction;
	gtk_tree_model_get (psTreeModel, psIter, FUNCSCOL_FUNCDATA, & psFuncData, -1);

	if (psFuncData) {
		szFunction = GetFunctionName (psFuncData);
		g_object_set (psCell, "text", szFunction, NULL);
	}
	else {
		g_object_set (psCell, "text", "New...", NULL);
	}
}

void EditFunctionRowDoubleClick (GtkTreeView * psTreeView, GtkTreePath * psPath, GtkTreeViewColumn * psColumn, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
	gboolean boFound;
	GtkTreeModel * psTreeModel;
	GtkTreeIter sTreeIter;

	// Populate the Function window using information taken from the
	// row clicked on by the user

	psTreeModel = gtk_tree_view_get_model (psTreeView);

	boFound = gtk_tree_model_get_iter (psTreeModel, & sTreeIter, psPath);

	if (boFound) {
		EditFunctionFromList (psTreeModel, & sTreeIter, psFunctyData);
	}
}

void EditFunctionRowChanged (GtkTreeSelection * psSelection, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
	gboolean boFound;
	GtkTreeModel * psTreeModel;
	GtkTreeIter sTreeIter;

	boFound = gtk_tree_selection_get_selected (psSelection, & psTreeModel, & sTreeIter);

	if (boFound) {
		EditFunctionFromList (psTreeModel, & sTreeIter, psFunctyData);
	}
}

//static gboolean EditFunctionPress (GtkWidget * psWidget, gpointer psData) {
//	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
//	GtkWidget * psFunctionList;
//	GtkTreeSelection * psSelection;
//	gboolean boFound;
//	GtkTreeModel * psTreeModel;
//	GtkTreeIter sTreeIter;
//
//	// Populate the Function window using information taken from the
//	// currently selected function row
//
//	psFunctionList = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "FunctionList"));
//
//	psSelection = gtk_tree_view_get_selection	(GTK_TREE_VIEW (psFunctionList));
//
//	boFound = gtk_tree_selection_get_selected (psSelection, & psTreeModel, & sTreeIter);
//
//	if (boFound) {
//		EditFunctionFromList (psTreeModel, & sTreeIter, psFunctyData);
//	}
//	else {
//		SetFunctionPropertiesWindow (FUNCTYPE_INVALID, FALSE, psFunctyData);
//	}
//
//	return TRUE;
//}

void EditFunctionFromList (GtkTreeModel * psTreeModel, GtkTreeIter * psTreeIter, FunctyPersist * psFunctyData) {
	FuncPersist * psFuncData;

	// Populate the Function window using information taken from the supplied row
	gtk_tree_model_get (psTreeModel, psTreeIter, FUNCSCOL_FUNCDATA, & psFuncData, -1);

	EditFunction (psFuncData, psFunctyData);
}

void EditFunction (FuncPersist * psFuncData, FunctyPersist * psFunctyData) {
	GtkWidget * psFunctionEdit;
	GtkWidget * psWidget;
	char const * szFunction;
	char const * szName;
	double fAccuracy;
	FUNCTYPE eFuncType;
	GtkWidget * psNotebook;
	int nPageRemoved;
	int nPage;
	float fThickness;
	bool boFill;

	// Populate the Function window using information about the given functions
	psFunctyData->psFuncEdit = psFuncData;
	psFunctionEdit = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "FunctionEdit"));

	// Find the notebook to allow us to sort out the visible tabs
	psNotebook = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "FunctionNotebook"));

	if (psFuncData) {
		// Set the function edit box
		eFuncType = GetFunctionType (psFuncData);

		szName = GetFunctionName (psFuncData);
		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "FunctionName"));
		gtk_entry_set_text (GTK_ENTRY (psWidget), szName);

		szFunction = GetFunctionString (psFunctyData->psFuncEdit);
		SetTextViewText (psFunctionEdit, szFunction);

		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Red"));
		szFunction = GetRedString (psFunctyData->psFuncEdit);
		SetTextViewText (psWidget, szFunction);

		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Green"));
		szFunction = GetGreenString (psFunctyData->psFuncEdit);
		SetTextViewText (psWidget, szFunction);

		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Blue"));
		szFunction = GetBlueString (psFunctyData->psFuncEdit);
		SetTextViewText (psWidget, szFunction);

		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Alpha"));
		szFunction = GetAlphaString (psFunctyData->psFuncEdit);
		SetTextViewText (psWidget, szFunction);

		fAccuracy = GetFunctionAccuracy (psFuncData);
		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Accuracy"));
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (psWidget), (1.0 / fAccuracy));

		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TexFile"));
		szFunction = GetTexFileString (psFunctyData->psFuncEdit);
		gtk_entry_set_text (GTK_ENTRY (psWidget), szFunction);

		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TexXScale"));
		szFunction = GetTexXScaleString (psFunctyData->psFuncEdit);
		SetTextViewText (psWidget, szFunction);

		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TexYScale"));
		szFunction = GetTexYScaleString (psFunctyData->psFuncEdit);
		SetTextViewText (psWidget, szFunction);

		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TexXOffset"));
		szFunction = GetTexXOffsetString (psFunctyData->psFuncEdit);
		SetTextViewText (psWidget, szFunction);

		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TexYOffset"));
		szFunction = GetTexYOffsetString (psFunctyData->psFuncEdit);
		SetTextViewText (psWidget, szFunction);

		SetColourButton (psFunctyData);

		// Set the values for the physical properties
		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MaterialThickness"));
		fThickness = GetFunctionMaterialThickness (psFunctyData->psFuncEdit);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (psWidget), fThickness);

		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MaterialFill"));
		boFill = GetFunctionMaterialFill (psFunctyData->psFuncEdit);
		g_signal_handlers_block_by_func (psWidget, ToggleMaterialFill, (gpointer)psFunctyData);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (psWidget), boFill);
		g_signal_handlers_unblock_by_func (psWidget, ToggleMaterialFill, (gpointer)psFunctyData);

		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MaterialThickness"));
		gtk_widget_set_sensitive (psWidget, !boFill);
		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MaterialThicknessLabel"));
		gtk_widget_set_sensitive (psWidget, !boFill);

		switch (eFuncType) {
		case FUNCTYPE_CARTESIAN:
			// Hide the centre tab
			psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TabContentsCentre"));
			gtk_widget_hide (psWidget);

			// Show the function tab
			psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TabContentsFunction"));
			gtk_widget_show (psWidget);

			// Hide the curve tab
			psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TabContentsCurve"));
			nPageRemoved = gtk_notebook_page_num (GTK_NOTEBOOK (psNotebook), psWidget);
			nPage = gtk_notebook_get_current_page (GTK_NOTEBOOK(psNotebook));
			if (nPage == nPageRemoved) {
				gtk_notebook_set_current_page (GTK_NOTEBOOK(psNotebook), (nPage - 1));
			}
			gtk_widget_hide (psWidget);

			// Set the toolbox title
			//psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "EditTitle"));
			//gtk_label_set_text (GTK_LABEL (psWidget), "Edit Cartesian function");
			SetFunctionPropertiesWindow (FUNCTYPE_CARTESIAN, FALSE, psFunctyData);
			break;
		case FUNCTYPE_SPHERICAL:
			// Show the centre tab
			psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TabContentsCentre"));
			gtk_widget_show (psWidget);

			// Show the function tab
			psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TabContentsFunction"));
			gtk_widget_show (psWidget);

			// Hide the curve tab
			psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TabContentsCurve"));
			nPageRemoved = gtk_notebook_page_num (GTK_NOTEBOOK (psNotebook), psWidget);
			nPage = gtk_notebook_get_current_page (GTK_NOTEBOOK(psNotebook));
			if (nPage == nPageRemoved) {
				gtk_notebook_set_current_page (GTK_NOTEBOOK(psNotebook), (nPage - 1));
			}
			gtk_widget_hide (psWidget);

			// Set the toolbox title
			//psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "EditTitle"));
			//gtk_label_set_text (GTK_LABEL (psWidget), "Edit Spherical function");
			SetFunctionPropertiesWindow (FUNCTYPE_SPHERICAL, FALSE, psFunctyData);

			// Set up the centre values
			szFunction = SphericalGetXCentreString (psFunctyData->psFuncEdit);
			psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "XCentre"));
			SetTextViewText (psWidget, szFunction);

			szFunction = SphericalGetYCentreString (psFunctyData->psFuncEdit);
			psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "YCentre"));
			SetTextViewText (psWidget, szFunction);

			szFunction = SphericalGetZCentreString (psFunctyData->psFuncEdit);
			psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "ZCentre"));
			SetTextViewText (psWidget, szFunction);
			break;
		case FUNCTYPE_CURVE:
			// Show the centre tab
			psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TabContentsCentre"));
			gtk_widget_show (psWidget);

			// Show the curve tab
			psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TabContentsCurve"));
			gtk_widget_show (psWidget);

			// Hide the function tab
			psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TabContentsFunction"));
			nPageRemoved = gtk_notebook_page_num (GTK_NOTEBOOK (psNotebook), psWidget);
			nPage = gtk_notebook_get_current_page (GTK_NOTEBOOK(psNotebook));
			if (nPage == nPageRemoved) {
				gtk_notebook_set_current_page (GTK_NOTEBOOK(psNotebook), (nPage + 1));
			}
			gtk_widget_hide (psWidget);

			// Set the toolbox title
			//psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "EditTitle"));
			//gtk_label_set_text (GTK_LABEL (psWidget), "Edit Curve function");
			SetFunctionPropertiesWindow (FUNCTYPE_CURVE, FALSE, psFunctyData);

			// Set up the curve values
			szFunction = CurveGetRadiusString (psFunctyData->psFuncEdit);
			psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "RadiusEdit"));
			SetTextViewText (psWidget, szFunction);

			szFunction = CurveGetXFunctionString (psFunctyData->psFuncEdit);
			psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "CurveXEdit"));
			SetTextViewText (psWidget, szFunction);

			szFunction = CurveGetYFunctionString (psFunctyData->psFuncEdit);
			psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "CurveYEdit"));
			SetTextViewText (psWidget, szFunction);

			szFunction = CurveGetZFunctionString (psFunctyData->psFuncEdit);
			psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "CurveZEdit"));
			SetTextViewText (psWidget, szFunction);

			fAccuracy = GetFunctionAccuracy (psFuncData);
			psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "AccuracyCurve"));
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (psWidget), (1.0 / fAccuracy));

			fAccuracy = CurveGetFunctionAccuracyRadius (psFuncData);
			psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "AccuracyRadiusCurve"));
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (psWidget), (1.0 / fAccuracy));

			// Set up the centre values
			szFunction = CurveGetXCentreString (psFunctyData->psFuncEdit);
			psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "XCentre"));
			SetTextViewText (psWidget, szFunction);

			szFunction = CurveGetYCentreString (psFunctyData->psFuncEdit);
			psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "YCentre"));
			SetTextViewText (psWidget, szFunction);

			szFunction = CurveGetZCentreString (psFunctyData->psFuncEdit);
			psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "ZCentre"));
			SetTextViewText (psWidget, szFunction);
			break;
		default:
			// Do nothing
			break;
		}

		//psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Function"));
		//gtk_widget_show (psWidget);
	}
	else {
		// If the function data is NULL we'll start to edit a new function
		// This should never happen the way the interface is currently set up
		NewCartesianFunction (psFunctyData);
		// Set the window title
		//psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Function"));
		//gtk_window_set_title (GTK_WINDOW (psWidget), "Edit Cartesian function");
		SetFunctionPropertiesWindow (FUNCTYPE_CARTESIAN, FALSE, psFunctyData);

		// Hide the centre tab
		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TabContentsCentre"));
		gtk_widget_hide (psWidget);

		// Show the function tab
		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TabContentsFunction"));
		gtk_widget_show (psWidget);

		// Hide the curve tab
		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TabContentsCurve"));
		nPageRemoved = gtk_notebook_page_num (GTK_NOTEBOOK (psNotebook), psWidget);
		nPage = gtk_notebook_get_current_page (GTK_NOTEBOOK(psNotebook));
		if (nPage == nPageRemoved) {
			gtk_notebook_set_current_page (GTK_NOTEBOOK(psNotebook), (nPage - 1));
		}
		gtk_widget_hide (psWidget);
	}
}

void NewCartesianFunction (FunctyPersist * psFunctyData) {
	GtkWidget * psWidget;
	GtkWidget * psFunctionEdit;
	GtkWidget * psColourWidget;
	GtkWidget * psTexWidget;
	GtkWidget * psNotebook;
	GString * szName;
	GString * szValue;
	GdkColor sColour;
	double fRed;
	double fGreen;
	double fBlue;
	double fAlpha;
	int nPageRemoved;
	int nPage;
	unsigned int uFunctionCount;

	psFunctyData->eNextFunctionType = FUNCTYPE_CARTESIAN;
	psFunctyData->psFuncEdit = NULL;

	uFunctionCount = GetFunctionCount (psFunctyData->psVisData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "FunctionName"));
	szName = g_string_new ("");
	g_string_printf (szName, "Cartesian %u", uFunctionCount);
	SetTextViewText (psWidget, szName->str);
	g_string_free (szName, TRUE);

	psFunctionEdit = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "FunctionEdit"));

	SetTextViewText (psFunctionEdit, FUNCTION_NEW_CARTESIAN);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Accuracy"));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (psWidget), (1.0 / ACCURACY_NEW));

	szValue = g_string_new ("");

	GetNextColour (& fRed, & fGreen, & fBlue, & fAlpha, psFunctyData);

	sColour.red = (guint16)floor ((65535.0 * fRed) + 0.5);
	g_string_printf (szValue, "%.3f", fRed);
	psColourWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Red"));
	SetTextViewText (psColourWidget, szValue->str);

	sColour.green = (guint16)floor ((65535.0 * fGreen) + 0.5);
	g_string_printf (szValue, "%.3f", fGreen);
	psColourWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Green"));
	SetTextViewText (psColourWidget, szValue->str);

	sColour.blue = (guint16)floor ((65535.0 * fBlue) + 0.5);
	g_string_printf (szValue, "%.3f", fBlue);
	psColourWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Blue"));
	SetTextViewText (psColourWidget, szValue->str);

	g_string_printf (szValue, "%.3f", fAlpha);
	psColourWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Alpha"));
	SetTextViewText (psColourWidget, szValue->str);

	g_string_free (szValue, TRUE);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Colour"));
	gtk_color_button_set_color (GTK_COLOR_BUTTON (psWidget), & sColour);
	gtk_color_button_set_alpha (GTK_COLOR_BUTTON (psWidget), (guint16)floor ((65535.0 * ALPHA_NEW) + 0.5));

	psTexWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TexFile"));
	gtk_entry_set_text (GTK_ENTRY (psTexWidget), "");

	psTexWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TexXScale"));
	SetTextViewText (psTexWidget, "1.0");

	psTexWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TexYScale"));
	SetTextViewText (psTexWidget, "1.0");

	psTexWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TexXOffset"));
	SetTextViewText (psTexWidget, "0.0");

	psTexWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TexYOffset"));
	SetTextViewText (psTexWidget, "0.0");

	// Set the values for the physical properties
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MaterialThickness"));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (psWidget), MATERIALTHICKNESS_NEW);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MaterialFill"));
	g_signal_handlers_block_by_func (psWidget, ToggleMaterialFill, (gpointer)psFunctyData);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (psWidget), FALSE);
	g_signal_handlers_unblock_by_func (psWidget, ToggleMaterialFill, (gpointer)psFunctyData);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MaterialThickness"));
	gtk_widget_set_sensitive (psWidget, TRUE);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MaterialThicknessLabel"));
	gtk_widget_set_sensitive (psWidget, TRUE);

	// Sort out the visible tabs of the notebook
	psNotebook = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "FunctionNotebook"));

	// Hide the centre tab
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TabContentsCentre"));
	gtk_widget_hide (psWidget);

	// Show the function tab
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TabContentsFunction"));
	gtk_widget_show (psWidget);

	// Hide the curve tab
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TabContentsCurve"));
	nPageRemoved = gtk_notebook_page_num (GTK_NOTEBOOK (psNotebook), psWidget);
	nPage = gtk_notebook_get_current_page (GTK_NOTEBOOK(psNotebook));
	if (nPage == nPageRemoved) {
		gtk_notebook_set_current_page (GTK_NOTEBOOK(psNotebook), (nPage - 1));
	}
	gtk_widget_hide (psWidget);

	// Set the toolbox title
	//psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "EditTitle"));
	//gtk_label_set_text (GTK_LABEL (psWidget), "New Cartesian function");
	SetFunctionPropertiesWindow (FUNCTYPE_CARTESIAN, TRUE, psFunctyData);

	//psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Function"));
	//gtk_widget_show (psWidget);
}

static gboolean AddCartesianPress (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;

	NewCartesianFunction (psFunctyData);

	return TRUE;
}

void NewSphericalFunction (FunctyPersist * psFunctyData) {
	GtkWidget * psWidget;
	GtkWidget * psFunctionEdit;
	GtkWidget * psColourWidget;
	GtkWidget * psTexWidget;
	GtkWidget * psNotebook;
	GString * szName;
	GString * szValue;
	GdkColor sColour;
	double fRed;
	double fGreen;
	double fBlue;
	double fAlpha;
	int nPageRemoved;
	int nPage;
	unsigned int uFunctionCount;

	psFunctyData->eNextFunctionType = FUNCTYPE_SPHERICAL;
	psFunctyData->psFuncEdit = NULL;

	uFunctionCount = GetFunctionCount (psFunctyData->psVisData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "FunctionName"));
	szName = g_string_new ("");
	g_string_printf (szName, "Spherical %u", uFunctionCount);
	SetTextViewText (psWidget, szName->str);
	g_string_free (szName, TRUE);

	psFunctionEdit = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "FunctionEdit"));

	SetTextViewText (psFunctionEdit, FUNCTION_NEW_SPHERICAL);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Accuracy"));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (psWidget), (1.0 / ACCURACY_NEW));

	szValue = g_string_new ("");

	GetNextColour (& fRed, & fGreen, & fBlue, & fAlpha, psFunctyData);

	sColour.red = (guint16)floor ((65535.0 * fRed) + 0.5);
	g_string_printf (szValue, "%.3f", fRed);
	psColourWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Red"));
	SetTextViewText (psColourWidget, szValue->str);

	sColour.green = (guint16)floor ((65535.0 * fGreen) + 0.5);
	g_string_printf (szValue, "%.3f", fGreen);
	psColourWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Green"));
	SetTextViewText (psColourWidget, szValue->str);

	sColour.blue = (guint16)floor ((65535.0 * fBlue) + 0.5);
	g_string_printf (szValue, "%.3f", fBlue);
	psColourWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Blue"));
	SetTextViewText (psColourWidget, szValue->str);

	g_string_printf (szValue, "%.3f", fAlpha);
	psColourWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Alpha"));
	SetTextViewText (psColourWidget, szValue->str);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Colour"));
	gtk_color_button_set_color (GTK_COLOR_BUTTON (psWidget), & sColour);
	gtk_color_button_set_alpha (GTK_COLOR_BUTTON (psWidget), (guint16)floor ((65535.0 * ALPHA_NEW) + 0.5));

	psTexWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TexFile"));
	gtk_entry_set_text (GTK_ENTRY (psTexWidget), "");

	psTexWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TexXScale"));
	SetTextViewText (psTexWidget, "1.0");

	psTexWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TexYScale"));
	SetTextViewText (psTexWidget, "1.0");

	psTexWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TexXOffset"));
	SetTextViewText (psTexWidget, "0.0");

	psTexWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TexYOffset"));
	SetTextViewText (psTexWidget, "0.0");

	// Set up the centre values
	g_string_printf (szValue, "%.3f", SPHERICAL_XCENTRE);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "XCentre"));
	SetTextViewText (psWidget, szValue->str);

	g_string_printf (szValue, "%.3f", SPHERICAL_YCENTRE);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "YCentre"));
	SetTextViewText (psWidget, szValue->str);

	g_string_printf (szValue, "%.3f", SPHERICAL_ZCENTRE);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "ZCentre"));
	SetTextViewText (psWidget, szValue->str);

	// Sort out the visible tabs of the notebook
	psNotebook = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "FunctionNotebook"));

	// Show the centre tab
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TabContentsCentre"));
	gtk_widget_show (psWidget);

	// Show the function tab
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TabContentsFunction"));
	gtk_widget_show (psWidget);

	// Hide the curve tab
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TabContentsCurve"));
	nPageRemoved = gtk_notebook_page_num (GTK_NOTEBOOK (psNotebook), psWidget);
	nPage = gtk_notebook_get_current_page (GTK_NOTEBOOK(psNotebook));
	if (nPage == nPageRemoved) {
		gtk_notebook_set_current_page (GTK_NOTEBOOK(psNotebook), (nPage - 1));
	}
	gtk_widget_hide (psWidget);

	// Set the toolbox title
	//psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "EditTitle"));
	//gtk_label_set_text (GTK_LABEL (psWidget), "New Spherical function");
	SetFunctionPropertiesWindow (FUNCTYPE_SPHERICAL, TRUE, psFunctyData);

	g_string_free (szValue, TRUE);

	//psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Function"));
	//gtk_widget_show (psWidget);
}

static gboolean AddSphericalPress (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;

	NewSphericalFunction (psFunctyData);

	return TRUE;
}

void NewCurveFunction (FunctyPersist * psFunctyData) {
	GtkWidget * psWidget;
	GtkWidget * psRadiusEdit;
	GtkWidget * psCurveXEdit;
	GtkWidget * psCurveYEdit;
	GtkWidget * psCurveZEdit;
	GtkWidget * psColourWidget;
	GtkWidget * psTexWidget;
	GtkWidget * psNotebook;
	GString * szName;
	GString * szValue;
	GdkColor sColour;
	double fRed;
	double fGreen;
	double fBlue;
	double fAlpha;
	int nPageRemoved;
	int nPage;
	unsigned int uFunctionCount;

	psFunctyData->eNextFunctionType = FUNCTYPE_CURVE;
	psFunctyData->psFuncEdit = NULL;

	uFunctionCount = GetFunctionCount (psFunctyData->psVisData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "FunctionName"));
	szName = g_string_new ("");
	g_string_printf (szName, "Curve %u", uFunctionCount);
	SetTextViewText (psWidget, szName->str);
	g_string_free (szName, TRUE);

	psRadiusEdit = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "RadiusEdit"));
	psCurveXEdit = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "CurveXEdit"));
	psCurveYEdit = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "CurveYEdit"));
	psCurveZEdit = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "CurveZEdit"));

	SetTextViewText (psRadiusEdit, FUNCTION_NEW_CURVE_RADIUS);
	SetTextViewText (psCurveXEdit, FUNCTION_NEW_CURVE_X);
	SetTextViewText (psCurveYEdit, FUNCTION_NEW_CURVE_Y);
	SetTextViewText (psCurveZEdit, FUNCTION_NEW_CURVE_Z);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "AccuracyCurve"));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (psWidget), (1.0 / ACCURACY_NEW));

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "AccuracyRadiusCurve"));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (psWidget), (1.0 / ACCURACY_NEW));

	szValue = g_string_new ("");

	GetNextColour (& fRed, & fGreen, & fBlue, & fAlpha, psFunctyData);

	sColour.red = (guint16)floor ((65535.0 * fRed) + 0.5);
	g_string_printf (szValue, "%.3f", fRed);
	psColourWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Red"));
	SetTextViewText (psColourWidget, szValue->str);

	sColour.green = (guint16)floor ((65535.0 * fGreen) + 0.5);
	g_string_printf (szValue, "%.3f", fGreen);
	psColourWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Green"));
	SetTextViewText (psColourWidget, szValue->str);

	sColour.blue = (guint16)floor ((65535.0 * fBlue) + 0.5);
	g_string_printf (szValue, "%.3f", fBlue);
	psColourWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Blue"));
	SetTextViewText (psColourWidget, szValue->str);

	g_string_printf (szValue, "%.3f", fAlpha);
	psColourWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Alpha"));
	SetTextViewText (psColourWidget, szValue->str);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Colour"));
	gtk_color_button_set_color (GTK_COLOR_BUTTON (psWidget), & sColour);
	gtk_color_button_set_alpha (GTK_COLOR_BUTTON (psWidget), (guint16)floor ((65535.0 * ALPHA_NEW) + 0.5));

	psTexWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TexFile"));
	gtk_entry_set_text (GTK_ENTRY (psTexWidget), "");

	psTexWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TexXScale"));
	SetTextViewText (psTexWidget, "1.0");

	psTexWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TexYScale"));
	SetTextViewText (psTexWidget, "1.0");

	psTexWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TexXOffset"));
	SetTextViewText (psTexWidget, "0.0");

	psTexWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TexYOffset"));
	SetTextViewText (psTexWidget, "0.0");

	// Set up the centre values
	g_string_printf (szValue, "%.3f", CURVE_XCENTRE);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "XCentre"));
	SetTextViewText (psWidget, szValue->str);

	g_string_printf (szValue, "%.3f", CURVE_YCENTRE);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "YCentre"));
	SetTextViewText (psWidget, szValue->str);

	g_string_printf (szValue, "%.3f", CURVE_ZCENTRE);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "ZCentre"));
	SetTextViewText (psWidget, szValue->str);

	// Sort out the visible tabs of the notebook
	psNotebook = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "FunctionNotebook"));

	// Show the centre tab
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TabContentsCentre"));
	gtk_widget_show (psWidget);

	// Show the curve tab
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TabContentsCurve"));
	gtk_widget_show (psWidget);

	// Hide the function tab
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TabContentsFunction"));
	nPageRemoved = gtk_notebook_page_num (GTK_NOTEBOOK (psNotebook), psWidget);
	nPage = gtk_notebook_get_current_page (GTK_NOTEBOOK(psNotebook));
	if (nPage == nPageRemoved) {
		gtk_notebook_set_current_page (GTK_NOTEBOOK(psNotebook), (nPage + 1));
	}
	gtk_widget_hide (psWidget);

	// Set the toolbox title
	//psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "EditTitle"));
	//gtk_label_set_text (GTK_LABEL (psWidget), "New Curve function");
	SetFunctionPropertiesWindow (FUNCTYPE_CURVE, TRUE, psFunctyData);

	g_string_free (szValue, TRUE);

	//psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Function"));
	//gtk_widget_show (psWidget);
}

static gboolean AddCurvePress (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;

	NewCurveFunction (psFunctyData);

	return TRUE;
}

void SetFunctionPropertiesWindow (FUNCTYPE eType, bool boNew, FunctyPersist * psFunctyData) {
	GtkWidget * psWidget;
	GtkWidget * psNotebook;
	GtkTreeSelection * psSelection;

	if (eType == FUNCTYPE_INVALID) {
		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "FunctionEdit"));
		SetTextViewText (psWidget, "");

		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Accuracy"));
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (psWidget), 0.0);

		// Sort out the visible tabs of the notebook
		psNotebook = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "FunctionNotebook"));

		// Hide the centre tab
		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TabContentsCentre"));
		gtk_widget_hide (psWidget);

		// Show the function tab
		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TabContentsFunction"));
		gtk_widget_show (psWidget);

		// Hide the curve tab
		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TabContentsCurve"));
		gtk_widget_hide (psWidget);

		gtk_notebook_set_current_page (GTK_NOTEBOOK(psNotebook), 0);

		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "FunctionProperties"));
		gtk_widget_set_sensitive (psWidget, FALSE);
		
		// Deactivate the action buttons
		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "PlotApply"));
		gtk_widget_set_sensitive (psWidget, FALSE);
		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "DeleteFunction"));
		gtk_widget_set_sensitive (psWidget, FALSE);
		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "PlotCancel"));
		gtk_widget_set_sensitive (psWidget, FALSE);
		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuDeleteFunction"));
		gtk_widget_set_sensitive (psWidget, FALSE);

		// Clear the selection row
		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "FunctionList"));
		psSelection = gtk_tree_view_get_selection (GTK_TREE_VIEW (psWidget));
		gtk_tree_selection_unselect_all (psSelection);
	}
	else {
		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "FunctionProperties"));
		gtk_widget_set_sensitive (psWidget, TRUE);

		// Activate the action buttons
		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "PlotApply"));
		gtk_widget_set_sensitive (psWidget, TRUE);
		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "DeleteFunction"));
		gtk_widget_set_sensitive (psWidget, TRUE);
		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "PlotCancel"));
		gtk_widget_set_sensitive (psWidget, TRUE);
		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuDeleteFunction"));
		gtk_widget_set_sensitive (psWidget, TRUE);
	}

	if (boNew) {
		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "DeleteFunction"));
		gtk_widget_set_sensitive (psWidget, FALSE);
		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuDeleteFunction"));
		gtk_widget_set_sensitive (psWidget, FALSE);

		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "EditTitle"));
		switch (eType) {
			case FUNCTYPE_CARTESIAN:
				gtk_label_set_text (GTK_LABEL (psWidget), "New Cartesian function");
				break;
			case FUNCTYPE_SPHERICAL:
				gtk_label_set_text (GTK_LABEL (psWidget), "New Spherical function");
				break;
			case FUNCTYPE_CURVE:
				gtk_label_set_text (GTK_LABEL (psWidget), "New Curve function");
				break;
			default:
				gtk_label_set_text (GTK_LABEL (psWidget), "Function properties");
				break;
		}
	}
	else {
		if (eType != FUNCTYPE_INVALID) {
			psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "DeleteFunction"));
			gtk_widget_set_sensitive (psWidget, TRUE);
			psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuDeleteFunction"));
			gtk_widget_set_sensitive (psWidget, TRUE);
		}

		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "EditTitle"));
		switch (eType) {
			case FUNCTYPE_CARTESIAN:
				gtk_label_set_text (GTK_LABEL (psWidget), "Edit Cartesian function");
				break;
			case FUNCTYPE_SPHERICAL:
				gtk_label_set_text (GTK_LABEL (psWidget), "Edit Spherical function");
				break;
			case FUNCTYPE_CURVE:
				gtk_label_set_text (GTK_LABEL (psWidget), "Edit Curve function");
				break;
			default:
				gtk_label_set_text (GTK_LABEL (psWidget), "Function properties");
				break;
		}
	}
}

void GetNextColour (double * pfRed, double * pfGreen, double * pfBlue, double * pfAlpha, FunctyPersist * psFunctyData) {
	if (pfRed) {
		* pfRed = 0.4 + 0.6 * ((cos (psFunctyData->fNewColour) + 1.0) / 2.0);
	}
	if (pfGreen) {
		* pfGreen = 0.4 + 0.6 * ((cos (psFunctyData->fNewColour + (2.0 * M_PI / 3.0)) + 1.0) / 2.0);
	}
	if (pfBlue) {
		* pfBlue = 0.4 + 0.6 * ((cos (psFunctyData->fNewColour + (4.0 * M_PI / 3.0)) + 1.0) / 2.0);
	}
	if (pfAlpha) {
		* pfAlpha = ALPHA_NEW;
	}

	psFunctyData->fNewColour += COLOUR_NEW_SHIFT;
}

void SetColourButton (FunctyPersist * psFunctyData) {
	GtkWidget * psWidget;
	bool boColour;
	GLfloat afGraphColour[4];
	GdkColor sColour;

	boColour = GetColour (afGraphColour, psFunctyData->psFuncEdit);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Colour"));
	if (boColour) {
		sColour.red = (guint16)floor ((65535.0 * afGraphColour[0]) + 0.5);
		sColour.green = (guint16)floor ((65535.0 * afGraphColour[1]) + 0.5);
		sColour.blue = (guint16)floor ((65535.0 * afGraphColour[2]) + 0.5);
		gtk_color_button_set_color (GTK_COLOR_BUTTON (psWidget), & sColour);

		gtk_color_button_set_alpha (GTK_COLOR_BUTTON (psWidget), (guint16)floor ((65535.0 * afGraphColour[3]) + 0.5));
	}
	else {
		gtk_color_button_set_alpha (GTK_COLOR_BUTTON (psWidget), (guint16)0);
	}
}

void SetColourButtonCallback (GtkColorButton * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
	GdkColor sColour;
	guint16 uAlpha;
	GtkWidget * psColourWidget;
	GString * szValue;

	szValue = g_string_new ("");

	gtk_color_button_get_color (psWidget, & sColour);
	uAlpha = gtk_color_button_get_alpha (psWidget);

	g_string_printf (szValue, "%.3f", (double)sColour.red / 65535.0);
	psColourWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Red"));
	SetTextViewText (psColourWidget, szValue->str);

	g_string_printf (szValue, "%.3f", (double)sColour.green / 65535.0);
	psColourWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Green"));
	SetTextViewText (psColourWidget, szValue->str);

	g_string_printf (szValue, "%.3f", (double)sColour.blue / 65535.0);
	psColourWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Blue"));
	SetTextViewText (psColourWidget, szValue->str);

	g_string_printf (szValue, "%.3f", (double)uAlpha / 65535.0);
	psColourWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Alpha"));
	SetTextViewText (psColourWidget, szValue->str);

	g_string_free (szValue, TRUE);
}

//static gboolean ControlVariablesPress (GtkWidget * psWidget, gpointer psData) {
//	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
//	GtkWidget * psControlVariables;
//
//	psControlVariables = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "ControlVariables"));
//	gtk_widget_hide (psControlVariables);
//
//	return ControlVariablesApply (psWidget, psData);
//}

static gboolean ControlVariablesApply (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
	char * szVarName;
	char * szVarValue;
	int nVariable;
	GtkWidget * psVarWidget;
	GString * szVarWidget = NULL;
	FnControlPersist * psFnControlData;

	szVarWidget = g_string_new ("");

	// Clear the existing list of control variables
	psFnControlData = GetControlVarList (psFunctyData->psVisData);
	ClearControlvarList (psFnControlData);

	// Set up the control variables
	for (nVariable = 0; nVariable < CONTROLVARS_MAX; nVariable++) {
		g_string_printf (szVarWidget, "var%d", nVariable);
		psVarWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, szVarWidget->str));
		szVarName = GetTextViewText (psVarWidget);

		if (szVarName && (szVarName[0] != 0)) {
			g_string_printf (szVarWidget, "val%d", nVariable);
			psVarWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, szVarWidget->str));
			szVarValue = GetTextViewText (psVarWidget);

			if (szVarValue && (szVarValue[0] != 0)) {
				SetControlVar (szVarName, szVarValue, psFunctyData->psVisData);
			}
			
			g_free (szVarValue);
		}
		
		g_free (szVarName);
	}

	SetControlvarWindowState (psFunctyData);
	AssignControlVarsToFunctionList (psFunctyData->psVisData);

	return TRUE;
}

static gboolean ControlVariablesCancel (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;

	SetControlvarWindowState (psFunctyData);

	return TRUE;
}

//static gboolean ControlvarPress (GtkWidget * psWidget, gpointer psData) {
//	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
//	//GtkWidget * psFunction;
//
//	SetControlvarWindowState (psFunctyData);
//
//	//psFunction = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "ControlVariables"));
//	//gtk_widget_show (psFunction);
//
//	return TRUE;
//}

void SetControlvarWindowState (FunctyPersist * psFunctyData) {
	FnControlPersist * psFnControlData;
	GSList * psControlvarList;
	char const * szName;
	char const * szValue;
	int nVariable;
	GtkWidget * psVarWidget;
	GString * szVarWidget = NULL;
	double fValue;

	psFnControlData = GetControlVarList (psFunctyData->psVisData);

	psControlvarList = psFnControlData->psControlvarList;

	szVarWidget = g_string_new ("");

	// Set up the variables that exist
	nVariable = 0;
	while (psControlvarList) {
		szName = GetControlvarName ((ControlvarPersist *)psControlvarList->data);
		szValue = GetControlvarValueString ((ControlvarPersist *)psControlvarList->data);
		fValue = GetControlvarValueDouble ((ControlvarPersist *)psControlvarList->data);

		if (nVariable < CONTROLVARS_MAX) {
			g_string_printf (szVarWidget, "var%d", nVariable);
			psVarWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, szVarWidget->str));
			SetTextViewText (psVarWidget, szName);

			g_string_printf (szVarWidget, "val%d", nVariable);
			psVarWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, szVarWidget->str));
			SetTextViewText (psVarWidget, szValue);

			g_string_printf (szVarWidget, "slider%d", nVariable);
			psVarWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, szVarWidget->str));
			g_signal_handlers_block_by_func (psVarWidget, G_CALLBACK (ControlvarSliderChanged), psFunctyData);
			gtk_range_set_value (GTK_RANGE (psVarWidget), fValue);
			g_signal_handlers_unblock_by_func (psVarWidget, G_CALLBACK (ControlvarSliderChanged), psFunctyData);
		}

		nVariable++;
		psControlvarList = g_slist_next (psControlvarList);
	}

	// Clear the remaining entries
	while (nVariable < CONTROLVARS_MAX) {
		if (nVariable < CONTROLVARS_MAX) {
			g_string_printf (szVarWidget, "var%d", nVariable);
			psVarWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, szVarWidget->str));
			SetTextViewText (psVarWidget, "");

			g_string_printf (szVarWidget, "val%d", nVariable);
			psVarWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, szVarWidget->str));
			SetTextViewText (psVarWidget, "");

			g_string_printf (szVarWidget, "slider%d", nVariable);
			psVarWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, szVarWidget->str));
			g_signal_handlers_block_by_func (psVarWidget, G_CALLBACK (ControlvarSliderChanged), psFunctyData);
			gtk_range_set_value (GTK_RANGE (psVarWidget), 0.0);
			g_signal_handlers_unblock_by_func (psVarWidget, G_CALLBACK (ControlvarSliderChanged), psFunctyData);
		}

		nVariable++;
	}

	g_string_free (szVarWidget, TRUE);
}

static gboolean ControlvarSliderChanged (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
	GtkWidget * psVarWidget;
	float fValue;
	char const * szName;
	char * szVarName;
	GString * szVarWidget;
	GString * szValue;
	int nVariable;

	// Transfer value to spin button
	szName = gtk_buildable_get_name (GTK_BUILDABLE (psWidget));
	sscanf (szName, "slider%d", & nVariable);
	szVarWidget = g_string_new ("");

	if ((nVariable >= 0) && (nVariable < CONTROLVARS_MAX)) {
		g_string_printf (szVarWidget, "var%d", nVariable);
		psVarWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, szVarWidget->str));
		szVarName = GetTextViewText (psVarWidget);

		if (szVarName && (szVarName[0] != 0)) {
			g_string_printf (szVarWidget, "val%d", nVariable);
			psVarWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, szVarWidget->str));

			szValue = g_string_new ("");
			fValue = gtk_range_get_value (GTK_RANGE (psWidget));
			fValue = round((fValue * 1000)) / 1000.0f;
			g_string_printf (szValue, "%.03f", fValue);

			SetControlVar (szVarName, szValue->str, psFunctyData->psVisData);

			SetTextViewText (psVarWidget, szValue->str);
			g_string_free (szValue, TRUE);
			AssignControlVarsToFunctionList (psFunctyData->psVisData);
		}
		
		g_free (szVarName);
	}
	g_string_free (szVarWidget, TRUE);

	return TRUE;
}

static gboolean FocusBlurNearSliderChanged (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
	float fValue;

	fValue = gtk_range_get_value (GTK_RANGE (psWidget));
	SetFocusBlurNear (fValue, psFunctyData->psVisData);

	return TRUE;
}

static gboolean FocusBlurFarSliderChanged (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
	float fValue;

	fValue = gtk_range_get_value (GTK_RANGE (psWidget));
	SetFocusBlurFar (fValue, psFunctyData->psVisData);

	return TRUE;
}

void SetFocusBlurNearState (FunctyPersist * psFunctyData) {
	GtkWidget * psWidget;
	float fValue;
	
	fValue = GetFocusBlurNear (psFunctyData->psVisData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "FocusBlurNear"));
	g_signal_handlers_block_by_func (psWidget, G_CALLBACK (FocusBlurNearSliderChanged), psFunctyData);
	gtk_range_set_value (GTK_RANGE (psWidget), fValue);
	g_signal_handlers_unblock_by_func (psWidget, G_CALLBACK (FocusBlurNearSliderChanged), psFunctyData);
}

void SetFocusFarNearState (FunctyPersist * psFunctyData) {
	GtkWidget * psWidget;
	float fValue;
	
	fValue = GetFocusBlurFar (psFunctyData->psVisData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "FocusBlurFar"));
	g_signal_handlers_block_by_func (psWidget, G_CALLBACK (FocusBlurFarSliderChanged), psFunctyData);
	gtk_range_set_value (GTK_RANGE (psWidget), fValue);
	g_signal_handlers_unblock_by_func (psWidget, G_CALLBACK (FocusBlurFarSliderChanged), psFunctyData);
}

static gboolean AudioConfigPress (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
	GtkDialog * psDialog;
	GtkListStore * psModel;
	GtkWidget * psDialogWidget;
	AudioPersist * psAudioData;
	AudioComboData * psAudioComboData;
	bool boActive;

	psAudioComboData = g_new (AudioComboData, 1);
	psAudioData = GetAudioData (psFunctyData->psVisData);

	// Set up the active toggle
	psDialogWidget = GTK_WIDGET (gtk_builder_get_object (psFunctyData->psXML, "AudioActive"));
	boActive = AudioCheckRecordActive (psAudioData);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (psDialogWidget), boActive);

	// Set up the callback userdata structure
	psAudioComboData->psCombo = GTK_COMBO_BOX_TEXT (gtk_builder_get_object (psFunctyData->psXML, "AudioSourceList"));
	gtk_widget_set_sensitive (GTK_WIDGET (psAudioComboData->psCombo), boActive);
	psAudioComboData->psAudioData = psAudioData;
	psAudioComboData->nIndex = 0;
	psAudioComboData->nSelectedIndex = 0;

	psModel = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (psAudioComboData->psCombo)));
	gtk_list_store_clear (psModel);

	AudioReturnSources (AudioSourceComboCallback, psAudioComboData, psAudioData);

	psDialog = GTK_DIALOG (gtk_builder_get_object (psFunctyData->psXML, "AudioConfig"));
	gtk_widget_show (GTK_WIDGET (psDialog));

	return TRUE;
}

void AudioActiveChangedCallback (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
	bool boActive;
	GtkWidget * psAudioCombo;

	psAudioCombo = GTK_WIDGET (gtk_builder_get_object (psFunctyData->psXML, "AudioSourceList"));
	boActive = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (psWidget));
	gtk_widget_set_sensitive (GTK_WIDGET (psAudioCombo), boActive);
}

void AudioSourceComboCallback (pa_context * psContext, const pa_source_info * psSourceInfo, int nEol, void * psUserData) {
	AudioComboData * psAudioComboData = (AudioComboData *)psUserData;
	char const * szCurrentSource;
	pa_stream * psRecordStream;

	if ((psSourceInfo) && (nEol <= 1)) {
		// Add the item to the list
		gtk_combo_box_text_append_text (psAudioComboData->psCombo, psSourceInfo->name);

		psRecordStream = AudioGetRecordStream (psAudioComboData->psAudioData);
		szCurrentSource = NULL;
		if (psRecordStream) {
			szCurrentSource = pa_stream_get_device_name (psRecordStream);
			if (szCurrentSource && psSourceInfo->name && (strncmp (szCurrentSource, psSourceInfo->name, AUDIO_SOURCE_NAME_MAX) == 0)) {
				psAudioComboData->nSelectedIndex = psAudioComboData->nIndex;
			}
		}
		
		psAudioComboData->nIndex++;
	}
	else {
		// Last item in the list
		gtk_combo_box_set_active (GTK_COMBO_BOX (psAudioComboData->psCombo), psAudioComboData->nSelectedIndex);
		
		g_free (psAudioComboData);
	}
}

static gboolean AudioConfigDestroy (GtkWidget * psWidget, GdkEvent * psEvent, gpointer psData) {
	return AudioConfigClose (psWidget, psData);
}

static gboolean AudioConfigClose (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
	GtkDialog * psDialog;
	
	psDialog = GTK_DIALOG (gtk_builder_get_object (psFunctyData->psXML, "AudioConfig"));
	gtk_widget_hide (GTK_WIDGET (psDialog));

	return TRUE;
}

static gboolean AudioConfigOkay (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
	GtkComboBox * psCombo;
	gchar * szSourceSelected;
	AudioPersist * psAudioData;
	GtkWidget * psDialogWidget;
	bool boActive;

	psCombo = GTK_COMBO_BOX (gtk_builder_get_object (psFunctyData->psXML, "AudioSourceList"));
	psAudioData = GetAudioData (psFunctyData->psVisData);

	szSourceSelected = gtk_combo_box_get_active_text (psCombo);

	// Set up the active toggle
	psDialogWidget = GTK_WIDGET (gtk_builder_get_object (psFunctyData->psXML, "AudioActive"));
	boActive = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (psDialogWidget));

	if (boActive) {
		RecordStart (szSourceSelected, psAudioData);
	}
	else {
		RecordStop (psAudioData);
	}

	return AudioConfigClose (psWidget, psData);
}

static gboolean LoadFilePress (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
	GtkWidget * psDialogue;
	GtkWindow * psParent;
	char * szFilename;
	char * szFolder;
	GtkListStore * psFunctionListModel;
	GtkWidget * psFunctionList;
	bool boLoaded;
	GtkFileFilter * psFilterXML;
	GtkFileFilter * psFilterAll;
	FnControlPersist * psFnControlData;

	psFilterXML = gtk_file_filter_new ();
	gtk_file_filter_add_mime_type (psFilterXML, "text/xml");
	gtk_file_filter_add_mime_type (psFilterXML, "application/xml");
	gtk_file_filter_set_name (psFilterXML, "XML Files");

	psFilterAll = gtk_file_filter_new ();
	gtk_file_filter_add_pattern (psFilterAll, "*");
	gtk_file_filter_set_name (psFilterAll, "All Files");


	psFunctionList = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "FunctionList"));
	psFunctionListModel = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (psFunctionList)));
	psParent = GTK_WINDOW (GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MainWindow")));

	psDialogue = gtk_file_chooser_dialog_new ("Open File", psParent, 
		GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, 
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

	g_object_set (G_OBJECT (psDialogue), "local-only", FALSE, NULL);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (psDialogue), psFilterXML);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (psDialogue), psFilterAll);

	if (psFunctyData->boFolderSet) {
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (psDialogue), psFunctyData->szFolder->str);
	}

	PauseAnimationModal (TRUE, psFunctyData);

	if (gtk_dialog_run (GTK_DIALOG (psDialogue)) == GTK_RESPONSE_ACCEPT) {
		szFilename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (psDialogue));
		ClearAllFunctions (psFunctyData);

		psFnControlData = GetControlVarList (psFunctyData->psVisData);
		ClearControlvarList (psFnControlData);

		boLoaded = LoadFile (szFilename, FALSE, psFunctyData);
		PopulateFunctionList (psFunctionListModel, psFunctyData);
		if (boLoaded) {
			g_string_assign (psFunctyData->szFilename, szFilename);

			szFolder = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (psDialogue));
			psFunctyData->boFolderSet = (szFolder != NULL);
			if (szFolder) {
				g_string_assign (psFunctyData->szFolder, szFolder);
				g_free (szFolder);
				szFolder = NULL;
			}

			// Set the window title using the file display name
			SynchroniseUI (psFunctyData);
			SetMainWindowTitle (szFilename, psFunctyData);
		}
		psFunctyData->boFileLoaded = boLoaded;
		g_free (szFilename);
		szFilename = NULL;
	}
	gtk_widget_destroy (psDialogue);

	PauseAnimationModal (FALSE, psFunctyData);

	return TRUE;
}

static gboolean SaveFilePress (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
	GtkWidget * psDialogue;
	GtkWindow * psParent;
	char * szFilename;
	char * szFolder;
	bool boLoaded;
	GtkFileFilter * psFilterXML;
	GtkFileFilter * psFilterAll;

	psFilterXML = gtk_file_filter_new ();
	gtk_file_filter_add_mime_type (psFilterXML, "text/xml");
	gtk_file_filter_add_mime_type (psFilterXML, "application/xml");
	gtk_file_filter_set_name (psFilterXML, "XML Files");

	psFilterAll = gtk_file_filter_new ();
	gtk_file_filter_add_pattern (psFilterAll, "*");
	gtk_file_filter_set_name (psFilterAll, "All Files");

	psParent = GTK_WINDOW (GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MainWindow")));

	psDialogue = gtk_file_chooser_dialog_new ("Save File", psParent, 
		GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, 
		GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);

	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (psDialogue), TRUE);
	g_object_set (G_OBJECT (psDialogue), "local-only", FALSE, NULL);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (psDialogue), psFilterXML);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (psDialogue), psFilterAll);

	PauseAnimationModal (TRUE, psFunctyData);

	if (psFunctyData->boFolderSet) {
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (psDialogue), psFunctyData->szFolder->str);
	}
	else {
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (psDialogue), g_get_home_dir ());
	}

	if (!psFunctyData->boFileLoaded) {
		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (psDialogue), DEFAULT_FILENAME);
	}
	else {
		gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (psDialogue), psFunctyData->szFilename->str);
	}

	if (gtk_dialog_run (GTK_DIALOG (psDialogue)) == GTK_RESPONSE_ACCEPT) {
		szFilename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (psDialogue));
		boLoaded = SaveFile (szFilename, FALSE, psFunctyData);
		if (boLoaded) {
			g_string_assign (psFunctyData->szFilename, szFilename);

			szFolder = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (psDialogue));
			psFunctyData->boFolderSet = (szFolder != NULL);
			if (szFolder) {
				g_string_assign (psFunctyData->szFolder, szFolder);
				g_free (szFolder);
				szFolder = NULL;
			}

			// Set the window title using the file display name
			SetMainWindowTitle (szFilename, psFunctyData);
		}
		psFunctyData->boFileLoaded = boLoaded;
		g_free (szFilename);
		szFilename = NULL;
	}
	gtk_widget_destroy (psDialogue);

	PauseAnimationModal (FALSE, psFunctyData);

	return TRUE;
}

static gboolean ExportFilePLYPress (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;

	GtkWidget * psDialogue;
	GtkWindow * psParent;
	char * szFilename;
	char * szFolder;
	bool boExported;
	GtkFileFilter * psFilterPLY;
	GtkFileFilter * psFilterAll;
	GtkWidget * psBinary;
	GtkWidget * psMultiplier;
	GtkWidget * psScale;
	GtkWidget * psScreenCoords;
	GtkWidget * psOptionsFile;
	GtkWidget * psAlpha;
	GtkWidget * psWidgetAdd;
	GtkAdjustment * psAdjustment;

	psFilterPLY = gtk_file_filter_new ();
	gtk_file_filter_add_pattern (psFilterPLY, "*.ply");
	gtk_file_filter_add_mime_type (psFilterPLY, "text/ply");
	gtk_file_filter_add_mime_type (psFilterPLY, "application/ply");
	gtk_file_filter_set_name (psFilterPLY, "Stanford Triangle Format");

	psFilterAll = gtk_file_filter_new ();
	gtk_file_filter_add_pattern (psFilterAll, "*");
	gtk_file_filter_set_name (psFilterAll, "All Files");

	psParent = GTK_WINDOW (GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MainWindow")));

	psDialogue = gtk_file_chooser_dialog_new ("Export Model in PLY", psParent, 
		GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, 
		GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);

	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (psDialogue), TRUE);
	g_object_set (G_OBJECT (psDialogue), "local-only", FALSE, NULL);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (psDialogue), psFilterPLY);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (psDialogue), psFilterAll);

	PauseAnimationModal (TRUE, psFunctyData);

	if (psFunctyData->boFolderSet) {
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (psDialogue), psFunctyData->szFolder->str);
	}
	else {
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (psDialogue), g_get_home_dir ());
	}

	if (!psFunctyData->boExportedModelPLY) {
		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (psDialogue), DEFAULT_EXPORTMODELNAME ".ply");
	}
	else {
		gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (psDialogue), psFunctyData->szExportModelNamePLY->str);
	}

	psOptionsFile = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (psOptionsFile);

	// Add file attribute widgets
	psWidgetAdd = gtk_label_new ("Scale");
	gtk_label_set_justify (GTK_LABEL (psWidgetAdd), GTK_JUSTIFY_RIGHT);
	gtk_widget_show (psWidgetAdd);
	gtk_box_pack_start (GTK_BOX (psOptionsFile), psWidgetAdd, FALSE, TRUE, 0);

	psAdjustment = GTK_ADJUSTMENT (gtk_adjustment_new (psFunctyData->fExportScale, 0.001, 10000.0, 0.1, 10.0, 0.0));
	psScale = gtk_spin_button_new (psAdjustment, 0.1, 3);
	gtk_widget_show (psScale);
	gtk_box_pack_start (GTK_BOX (psOptionsFile), psScale, FALSE, TRUE, 0);

	psWidgetAdd = gtk_label_new ("Accuracy");
	gtk_label_set_justify (GTK_LABEL (psWidgetAdd), GTK_JUSTIFY_RIGHT);
	gtk_widget_show (psWidgetAdd);
	gtk_box_pack_start (GTK_BOX (psOptionsFile), psWidgetAdd, FALSE, TRUE, 0);

	psAdjustment = GTK_ADJUSTMENT (gtk_adjustment_new (psFunctyData->fExportMultiplier, 0.00, 1000.0, 0.1, 1.0, 0.0));
	psMultiplier = gtk_spin_button_new (psAdjustment, 1.0, 3);
	gtk_widget_show (psMultiplier);
	gtk_box_pack_start (GTK_BOX (psOptionsFile), psMultiplier, FALSE, TRUE, 0);

	psBinary = gtk_check_button_new_with_label ("Binary");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (psBinary), psFunctyData->boBinary);
	gtk_widget_show (psBinary);
	gtk_box_pack_start (GTK_BOX (psOptionsFile), psBinary, FALSE, TRUE, 0);

	psAlpha = gtk_check_button_new_with_label ("Include alpha");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (psAlpha), psFunctyData->boExportAlpha);
	gtk_widget_show (psAlpha);
	gtk_box_pack_start (GTK_BOX (psOptionsFile), psAlpha, FALSE, TRUE, 0);

	psScreenCoords = gtk_check_button_new_with_label ("Screen coords");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (psScreenCoords), psFunctyData->boExportScreen);
	gtk_widget_show (psScreenCoords);
	gtk_box_pack_start (GTK_BOX (psOptionsFile), psScreenCoords, FALSE, TRUE, 0);

	gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (psDialogue), psOptionsFile);

	if (gtk_dialog_run (GTK_DIALOG (psDialogue)) == GTK_RESPONSE_ACCEPT) {
		psFunctyData->boBinary = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (psBinary));
		psFunctyData->fExportMultiplier = gtk_spin_button_get_value (GTK_SPIN_BUTTON (psMultiplier));
		psFunctyData->fExportScale = gtk_spin_button_get_value (GTK_SPIN_BUTTON (psScale));
		psFunctyData->boExportScreen = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (psScreenCoords));
		psFunctyData->boExportAlpha = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (psAlpha));
		szFilename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (psDialogue));

		// Actually perform the model export
		boExported = ExportModelFilePLY (szFilename, psFunctyData->boBinary, psFunctyData->boExportScreen, psFunctyData->boExportAlpha, psFunctyData->fExportMultiplier, psFunctyData->fExportScale, psFunctyData->psVisData);

		if (boExported) {
			g_string_assign (psFunctyData->szExportModelNamePLY, szFilename);

			szFolder = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (psDialogue));
			psFunctyData->boFolderSet = (szFolder != NULL);
			if (szFolder) {
				g_string_assign (psFunctyData->szFolder, szFolder);
				g_free (szFolder);
				szFolder = NULL;
			}
		}
		psFunctyData->boExportedModelPLY = boExported;
		g_free (szFilename);
		szFilename = NULL;
	}
	gtk_widget_destroy (psDialogue);

	PauseAnimationModal (FALSE, psFunctyData);

	return TRUE;
}

static gboolean ExportFileSTLPress (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;

	GtkWidget * psDialogue;
	GtkWindow * psParent;
	char * szFilename;
	char * szFolder;
	bool boExported;
	GtkFileFilter * psFilterSTL;
	GtkFileFilter * psFilterAll;
	GtkWidget * psBinary;
	GtkWidget * psMultiplier;
	GtkWidget * psScale;
	GtkWidget * psScreenCoords;
	GtkWidget * psOptionsFile;
	GtkWidget * psWidgetAdd;
	GtkAdjustment * psAdjustment;

	psFilterSTL = gtk_file_filter_new ();
	gtk_file_filter_add_pattern (psFilterSTL, "*.stl");
	gtk_file_filter_add_mime_type (psFilterSTL, "text/stl");
	gtk_file_filter_add_mime_type (psFilterSTL, "application/stl");
	gtk_file_filter_set_name (psFilterSTL, "Standard Tessellation Language");

	psFilterAll = gtk_file_filter_new ();
	gtk_file_filter_add_pattern (psFilterAll, "*");
	gtk_file_filter_set_name (psFilterAll, "All Files");

	psParent = GTK_WINDOW (GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MainWindow")));

	psDialogue = gtk_file_chooser_dialog_new ("Export Model in STL", psParent, 
		GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, 
		GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);

	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (psDialogue), TRUE);
	g_object_set (G_OBJECT (psDialogue), "local-only", FALSE, NULL);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (psDialogue), psFilterSTL);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (psDialogue), psFilterAll);

	PauseAnimationModal (TRUE, psFunctyData);

	if (psFunctyData->boFolderSet) {
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (psDialogue), psFunctyData->szFolder->str);
	}
	else {
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (psDialogue), g_get_home_dir ());
	}

	if (!psFunctyData->boExportedModelSTL) {
		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (psDialogue), DEFAULT_EXPORTMODELNAME ".stl");
	}
	else {
		gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (psDialogue), psFunctyData->szExportModelNameSTL->str);
	}

	psOptionsFile = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (psOptionsFile);

	// Add file attribute widgets
	psWidgetAdd = gtk_label_new ("Scale");
	gtk_label_set_justify (GTK_LABEL (psWidgetAdd), GTK_JUSTIFY_RIGHT);
	gtk_widget_show (psWidgetAdd);
	gtk_box_pack_start (GTK_BOX (psOptionsFile), psWidgetAdd, FALSE, TRUE, 0);

	psAdjustment = GTK_ADJUSTMENT (gtk_adjustment_new (psFunctyData->fExportScale, 0.001, 10000.0, 0.1, 10.0, 0.0));
	psScale = gtk_spin_button_new (psAdjustment, 0.1, 3);
	gtk_widget_show (psScale);
	gtk_box_pack_start (GTK_BOX (psOptionsFile), psScale, FALSE, TRUE, 0);

	psWidgetAdd = gtk_label_new ("Accuracy");
	gtk_label_set_justify (GTK_LABEL (psWidgetAdd), GTK_JUSTIFY_RIGHT);
	gtk_widget_show (psWidgetAdd);
	gtk_box_pack_start (GTK_BOX (psOptionsFile), psWidgetAdd, FALSE, TRUE, 0);

	psAdjustment = GTK_ADJUSTMENT (gtk_adjustment_new (psFunctyData->fExportMultiplier, 0.00, 1000.0, 0.1, 1.0, 0.0));
	psMultiplier = gtk_spin_button_new (psAdjustment, 1.0, 3);
	gtk_widget_show (psMultiplier);
	gtk_box_pack_start (GTK_BOX (psOptionsFile), psMultiplier, FALSE, TRUE, 0);

	psBinary = gtk_check_button_new_with_label ("Binary");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (psBinary), psFunctyData->boBinary);
	gtk_widget_show (psBinary);
	gtk_box_pack_start (GTK_BOX (psOptionsFile), psBinary, FALSE, TRUE, 0);

	psScreenCoords = gtk_check_button_new_with_label ("Screen coords");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (psScreenCoords), psFunctyData->boExportScreen);
	gtk_widget_show (psScreenCoords);
	gtk_box_pack_start (GTK_BOX (psOptionsFile), psScreenCoords, FALSE, TRUE, 0);

	gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (psDialogue), psOptionsFile);

	if (gtk_dialog_run (GTK_DIALOG (psDialogue)) == GTK_RESPONSE_ACCEPT) {
		psFunctyData->boBinary = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (psBinary));
		psFunctyData->fExportMultiplier = gtk_spin_button_get_value (GTK_SPIN_BUTTON (psMultiplier));
		psFunctyData->fExportScale = gtk_spin_button_get_value (GTK_SPIN_BUTTON (psScale));
		psFunctyData->boExportScreen = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (psScreenCoords));
		szFilename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (psDialogue));

		// Actually perform the model export
		boExported = ExportModelFileSTL (szFilename, psFunctyData->boBinary, psFunctyData->boExportScreen, psFunctyData->fExportMultiplier, psFunctyData->fExportScale, psFunctyData->psVisData);

		if (boExported) {
			g_string_assign (psFunctyData->szExportModelNameSTL, szFilename);

			szFolder = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (psDialogue));
			psFunctyData->boFolderSet = (szFolder != NULL);
			if (szFolder) {
				g_string_assign (psFunctyData->szFolder, szFolder);
				g_free (szFolder);
				szFolder = NULL;
			}
		}
		psFunctyData->boExportedModelSTL = boExported;
		g_free (szFilename);
		szFilename = NULL;
	}
	gtk_widget_destroy (psDialogue);

	PauseAnimationModal (FALSE, psFunctyData);

	return TRUE;
}

static gboolean ExportFileSVXPress (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;

	GtkWidget * psDialogue;
	GtkWindow * psParent;
	char * szFilename;
	char * szFolder;
	bool boExported;
	GtkFileFilter * psFilterSVX;
	GtkFileFilter * psFilterAll;
	GtkWidget * psVoxelResolution;
	GtkWidget * psOptionsFile;
	GtkWidget * psWidgetAdd;
	GtkAdjustment * psAdjustment;
	ExportSVXPersist * psExportSVXData;

	psFilterSVX = gtk_file_filter_new ();
	gtk_file_filter_add_pattern (psFilterSVX, "*.svx");
	gtk_file_filter_add_mime_type (psFilterSVX, "text/svx");
	gtk_file_filter_add_mime_type (psFilterSVX, "application/svx");
	gtk_file_filter_set_name (psFilterSVX, "Simple Voxels Format");

	psFilterAll = gtk_file_filter_new ();
	gtk_file_filter_add_pattern (psFilterAll, "*");
	gtk_file_filter_set_name (psFilterAll, "All Files");

	psParent = GTK_WINDOW (GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MainWindow")));

	psDialogue = gtk_file_chooser_dialog_new ("Export Model in SVX", psParent, GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);

	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (psDialogue), TRUE);
	g_object_set (G_OBJECT (psDialogue), "local-only", FALSE, NULL);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (psDialogue), psFilterSVX);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (psDialogue), psFilterAll);

	PauseAnimationModal (TRUE, psFunctyData);

	if (psFunctyData->boFolderSet) {
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (psDialogue), psFunctyData->szFolder->str);
	}
	else {
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (psDialogue), g_get_home_dir ());
	}

	if (!psFunctyData->boExportedModelSVX) {
		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (psDialogue), DEFAULT_EXPORTMODELNAME ".svx");
	}
	else {
		gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (psDialogue), psFunctyData->szExportModelNameSVX->str);
	}

	psOptionsFile = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (psOptionsFile);

	// Add file attribute widgets
	psWidgetAdd = gtk_label_new ("Resolution");
	gtk_label_set_justify (GTK_LABEL (psWidgetAdd), GTK_JUSTIFY_RIGHT);
	gtk_widget_show (psWidgetAdd);
	gtk_box_pack_start (GTK_BOX (psOptionsFile), psWidgetAdd, FALSE, TRUE, 0);

	psAdjustment = GTK_ADJUSTMENT (gtk_adjustment_new (psFunctyData->nVoxelResolution, 1.0, 4096.0, 1.0, 10.0, 0.0));
	psVoxelResolution = gtk_spin_button_new (psAdjustment, 1, 0);
	gtk_widget_show (psVoxelResolution);
	gtk_box_pack_start (GTK_BOX (psOptionsFile), psVoxelResolution, FALSE, TRUE, 0);

	gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (psDialogue), psOptionsFile);

	if (gtk_dialog_run (GTK_DIALOG (psDialogue)) == GTK_RESPONSE_ACCEPT) {
		psFunctyData->nVoxelResolution = gtk_spin_button_get_value (GTK_SPIN_BUTTON (psVoxelResolution));
		szFilename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (psDialogue));

		// Actually perform the model export
		gtk_widget_hide (psDialogue);
		psExportSVXData = NewExportSVXPersist (szFilename, psFunctyData->nVoxelResolution, psFunctyData->psVisData);
		boExported = LongPoll (psFunctyData->psXML, psExportSVXData, ExportStartSVX, ExportStepSVX, ExportFinishSVX, ExportCancelSVX);

		if (boExported) {
			g_string_assign (psFunctyData->szExportModelNameSVX, szFilename);

			szFolder = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (psDialogue));
			psFunctyData->boFolderSet = (szFolder != NULL);
			if (szFolder) {
				g_string_assign (psFunctyData->szFolder, szFolder);
				g_free (szFolder);
				szFolder = NULL;
			}
		}
		psFunctyData->boExportedModelSVX = boExported;
		g_free (szFilename);
		szFilename = NULL;
	}
	gtk_widget_destroy (psDialogue);

	PauseAnimationModal (FALSE, psFunctyData);

	return TRUE;
}

static gboolean ExportFileVDBPress (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;

	GtkWidget * psDialogue;
	GtkWindow * psParent;
	char * szFilename;
	char * szFolder;
	bool boExported;
	GtkFileFilter * psFilterVDB;
	GtkFileFilter * psFilterAll;
	GtkWidget * psVoxelResolution;
	GtkWidget * psOptionsFile;
	GtkWidget * psWidgetAdd;
	GtkAdjustment * psAdjustment;
	ExportVDBPersist * psExportVDBData;

	psFilterVDB = gtk_file_filter_new ();
	gtk_file_filter_add_pattern (psFilterVDB, "*.vdb");
	gtk_file_filter_add_mime_type (psFilterVDB, "text/vdb");
	gtk_file_filter_add_mime_type (psFilterVDB, "application/vdb");
	gtk_file_filter_set_name (psFilterVDB, "OpenVDB Format");

	psFilterAll = gtk_file_filter_new ();
	gtk_file_filter_add_pattern (psFilterAll, "*");
	gtk_file_filter_set_name (psFilterAll, "All Files");

	psParent = GTK_WINDOW (GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MainWindow")));

	psDialogue = gtk_file_chooser_dialog_new ("Export Model in OpenVDB", psParent, GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);

	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (psDialogue), TRUE);
	g_object_set (G_OBJECT (psDialogue), "local-only", FALSE, NULL);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (psDialogue), psFilterVDB);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (psDialogue), psFilterAll);

	PauseAnimationModal (TRUE, psFunctyData);

	if (psFunctyData->boFolderSet) {
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (psDialogue), psFunctyData->szFolder->str);
	}
	else {
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (psDialogue), g_get_home_dir ());
	}

	if (!psFunctyData->boExportedModelVDB) {
		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (psDialogue), DEFAULT_EXPORTMODELNAME ".vdb");
	}
	else {
		gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (psDialogue), psFunctyData->szExportModelNameVDB->str);
	}

	psOptionsFile = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (psOptionsFile);

	// Add file attribute widgets
	psWidgetAdd = gtk_label_new ("Resolution");
	gtk_label_set_justify (GTK_LABEL (psWidgetAdd), GTK_JUSTIFY_RIGHT);
	gtk_widget_show (psWidgetAdd);
	gtk_box_pack_start (GTK_BOX (psOptionsFile), psWidgetAdd, FALSE, TRUE, 0);

	psAdjustment = GTK_ADJUSTMENT (gtk_adjustment_new (psFunctyData->nVoxelResolution, 1.0, 4096.0, 1.0, 10.0, 0.0));
	psVoxelResolution = gtk_spin_button_new (psAdjustment, 1, 0);
	gtk_widget_show (psVoxelResolution);
	gtk_box_pack_start (GTK_BOX (psOptionsFile), psVoxelResolution, FALSE, TRUE, 0);

	gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (psDialogue), psOptionsFile);

	if (gtk_dialog_run (GTK_DIALOG (psDialogue)) == GTK_RESPONSE_ACCEPT) {
		psFunctyData->nVoxelResolution = gtk_spin_button_get_value (GTK_SPIN_BUTTON (psVoxelResolution));
		szFilename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (psDialogue));

		// Actually perform the model export
		gtk_widget_hide (psDialogue);
		psExportVDBData = NewExportVDBPersist (szFilename, psFunctyData->nVoxelResolution, psFunctyData->psVisData);
		boExported = LongPoll (psFunctyData->psXML, psExportVDBData, ExportStartVDB, ExportStepVDB, ExportFinishVDB, ExportCancelVDB);

		if (boExported) {
			g_string_assign (psFunctyData->szExportModelNameVDB, szFilename);

			szFolder = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (psDialogue));
			psFunctyData->boFolderSet = (szFolder != NULL);
			if (szFolder) {
				g_string_assign (psFunctyData->szFolder, szFolder);
				g_free (szFolder);
				szFolder = NULL;
			}
		}
		psFunctyData->boExportedModelVDB = boExported;
		g_free (szFilename);
		szFilename = NULL;
	}
	gtk_widget_destroy (psDialogue);

	PauseAnimationModal (FALSE, psFunctyData);

	return TRUE;
}

static gboolean ExportAnimFilePLYPress (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;

	GtkWidget * psDialogue;
	GtkWindow * psParent;
	char * szFilename;
	char * szFolder;
	bool boExported;
	GtkFileFilter * psFilterAnim;
	GtkFileFilter * psFilterAll;
	GtkWidget * psBinary;
	GtkWidget * psMultiplier;
	GtkWidget * psScale;
	GtkWidget * psScreenCoords;
	GtkWidget * psTimeStart;
	GtkWidget * psTimeEnd;
	GtkWidget * psFrames;
	GtkWidget * psOptions;
	GtkWidget * psOptionsFile;
	GtkWidget * psOptionsAnim;
	GtkWidget * psAlpha;
	GtkWidget * psWidgetAdd;
	GtkAdjustment * psAdjustment;
	ExportPLYPersist * psExportPLYData;

	psFilterAnim = gtk_file_filter_new ();
	gtk_file_filter_add_pattern (psFilterAnim, "*.zip");
	gtk_file_filter_add_mime_type (psFilterAnim, "binary/zip");
	gtk_file_filter_add_mime_type (psFilterAnim, "application/zip");
	gtk_file_filter_set_name (psFilterAnim, "Zip archive");

	psFilterAll = gtk_file_filter_new ();
	gtk_file_filter_add_pattern (psFilterAll, "*");
	gtk_file_filter_set_name (psFilterAll, "All Files");

	psParent = GTK_WINDOW (GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MainWindow")));

	psDialogue = gtk_file_chooser_dialog_new ("Export Animation Frames in PLY", psParent, 
		GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, 
		GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);

	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (psDialogue), TRUE);
	g_object_set (G_OBJECT (psDialogue), "local-only", FALSE, NULL);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (psDialogue), psFilterAnim);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (psDialogue), psFilterAll);

	PauseAnimationModal (TRUE, psFunctyData);

	if (psFunctyData->boFolderSetAnim) {
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (psDialogue), psFunctyData->szFolderAnim->str);
	}
	else {
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (psDialogue), g_get_home_dir ());
	}

	if (!psFunctyData->boExportedAnim) {
		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (psDialogue), DEFAULT_EXPORTANIMNAME);
	}
	else {
		gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (psDialogue), psFunctyData->szExportAnimName->str);
	}

	psOptions = gtk_vbox_new (FALSE, 8);
	gtk_widget_show (psOptions);

	psOptionsFile = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (psOptionsFile);
	gtk_box_pack_start (GTK_BOX (psOptions), psOptionsFile, FALSE, TRUE, 0);

	psOptionsAnim = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (psOptionsAnim);
	gtk_box_pack_start (GTK_BOX (psOptions), psOptionsAnim, FALSE, TRUE, 0);

	// Add file attribute widgets
	psWidgetAdd = gtk_label_new ("Scale");
	gtk_label_set_justify (GTK_LABEL (psWidgetAdd), GTK_JUSTIFY_RIGHT);
	gtk_widget_show (psWidgetAdd);
	gtk_box_pack_start (GTK_BOX (psOptionsFile), psWidgetAdd, FALSE, TRUE, 0);

	psAdjustment = GTK_ADJUSTMENT (gtk_adjustment_new (psFunctyData->fExportScale, 0.001, 10000.0, 0.1, 10.0, 0.0));
	psScale = gtk_spin_button_new (psAdjustment, 0.1, 3);
	gtk_widget_show (psScale);
	gtk_box_pack_start (GTK_BOX (psOptionsFile), psScale, FALSE, TRUE, 0);

	psWidgetAdd = gtk_label_new ("Accuracy");
	gtk_label_set_justify (GTK_LABEL (psWidgetAdd), GTK_JUSTIFY_RIGHT);
	gtk_widget_show (psWidgetAdd);
	gtk_box_pack_start (GTK_BOX (psOptionsFile), psWidgetAdd, FALSE, TRUE, 0);

	psAdjustment = GTK_ADJUSTMENT (gtk_adjustment_new (psFunctyData->fExportMultiplier, 0.00, 1000.0, 0.1, 1.0, 0.0));
	psMultiplier = gtk_spin_button_new (psAdjustment, 1.0, 3);
	gtk_widget_show (psMultiplier);
	gtk_box_pack_start (GTK_BOX (psOptionsFile), psMultiplier, FALSE, TRUE, 0);

	psBinary = gtk_check_button_new_with_label ("Binary");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (psBinary), psFunctyData->boBinary);
	gtk_widget_show (psBinary);
	gtk_box_pack_start (GTK_BOX (psOptionsFile), psBinary, FALSE, TRUE, 0);

	psAlpha = gtk_check_button_new_with_label ("Include alpha");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (psAlpha), psFunctyData->boExportAlpha);
	gtk_widget_show (psAlpha);
	gtk_box_pack_start (GTK_BOX (psOptionsFile), psAlpha, FALSE, TRUE, 0);

	psScreenCoords = gtk_check_button_new_with_label ("Screen coords");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (psScreenCoords), psFunctyData->boExportScreen);
	gtk_widget_show (psScreenCoords);
	gtk_box_pack_start (GTK_BOX (psOptionsFile), psScreenCoords, FALSE, TRUE, 0);

	// Add animation attribute widgets
	psWidgetAdd = gtk_label_new ("Start time");
	gtk_label_set_justify (GTK_LABEL (psWidgetAdd), GTK_JUSTIFY_RIGHT);
	gtk_widget_show (psWidgetAdd);
	gtk_box_pack_start (GTK_BOX (psOptionsAnim), psWidgetAdd, FALSE, TRUE, 0);

	psAdjustment = GTK_ADJUSTMENT (gtk_adjustment_new (psFunctyData->fExportTimeStart, 0, EXPORTANIM_TIME_MAX, 1, 1000, 0));
	psTimeStart = gtk_spin_button_new (psAdjustment, 1, 3);
	gtk_widget_show (psTimeStart);
	gtk_box_pack_start (GTK_BOX (psOptionsAnim), psTimeStart, FALSE, TRUE, 0);

	psWidgetAdd = gtk_label_new ("End time");
	gtk_label_set_justify (GTK_LABEL (psWidgetAdd), GTK_JUSTIFY_RIGHT);
	gtk_widget_show (psWidgetAdd);
	gtk_box_pack_start (GTK_BOX (psOptionsAnim), psWidgetAdd, FALSE, TRUE, 0);

	psAdjustment = GTK_ADJUSTMENT (gtk_adjustment_new (psFunctyData->fExportTimeEnd, 0, EXPORTANIM_TIME_MAX, 1, 1000, 0));
	psTimeEnd = gtk_spin_button_new (psAdjustment, 1, 3);
	gtk_widget_show (psTimeEnd);
	gtk_box_pack_start (GTK_BOX (psOptionsAnim), psTimeEnd, FALSE, TRUE, 0);

	psWidgetAdd = gtk_label_new ("Frames");
	gtk_label_set_justify (GTK_LABEL (psWidgetAdd), GTK_JUSTIFY_RIGHT);
	gtk_widget_show (psWidgetAdd);
	gtk_box_pack_start (GTK_BOX (psOptionsAnim), psWidgetAdd, FALSE, TRUE, 0);

	psAdjustment = GTK_ADJUSTMENT (gtk_adjustment_new (psFunctyData->nExportFrames, 0, pow(10, EXPORTANIM_FRAMES_EXP), 1, 10, 0));
	psFrames = gtk_spin_button_new (psAdjustment, 1, 0);
	gtk_widget_show (psFrames);
	gtk_box_pack_start (GTK_BOX (psOptionsAnim), psFrames, FALSE, TRUE, 0);

	// Install the widgets into the filesave dialogue
	gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (psDialogue), psOptions);

	if (gtk_dialog_run (GTK_DIALOG (psDialogue)) == GTK_RESPONSE_ACCEPT) {
		psFunctyData->boBinary = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (psBinary));
		psFunctyData->fExportMultiplier = gtk_spin_button_get_value (GTK_SPIN_BUTTON (psMultiplier));
		psFunctyData->fExportScale = gtk_spin_button_get_value (GTK_SPIN_BUTTON (psScale));
		psFunctyData->boExportScreen = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (psScreenCoords));
		psFunctyData->boExportAlpha = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (psAlpha));
		psFunctyData->fExportTimeStart = gtk_spin_button_get_value (GTK_SPIN_BUTTON (psTimeStart));
		psFunctyData->fExportTimeEnd = gtk_spin_button_get_value (GTK_SPIN_BUTTON (psTimeEnd));
		psFunctyData->nExportFrames = gtk_spin_button_get_value (GTK_SPIN_BUTTON (psFrames));
		szFilename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (psDialogue));

		// Actually perform the model export
		gtk_widget_hide (psDialogue);
		psExportPLYData = NewExportPLYPersist (szFilename, psFunctyData->boBinary, psFunctyData->boExportScreen, psFunctyData->boExportAlpha, psFunctyData->fExportMultiplier, psFunctyData->fExportScale, psFunctyData->fExportTimeStart, psFunctyData->fExportTimeEnd, psFunctyData->nExportFrames, psFunctyData->psVisData);
		boExported = LongPoll (psFunctyData->psXML, psExportPLYData, ExportStartAnimatedPLY, ExportStepAnimatedPLY, ExportFinishAnimatedPLY, ExportCancelAnimatedPLY);

		if (boExported) {
			g_string_assign (psFunctyData->szExportAnimName, szFilename);

			szFolder = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (psDialogue));
			psFunctyData->boFolderSet = (szFolder != NULL);
			if (szFolder) {
				g_string_assign (psFunctyData->szFolder, szFolder);
				g_free (szFolder);
				szFolder = NULL;
			}
		}
		psFunctyData->boExportedAnim = boExported;
		g_free (szFilename);
		szFilename = NULL;
	}
	gtk_widget_destroy (psDialogue);

	PauseAnimationModal (FALSE, psFunctyData);

	return TRUE;
}

static gboolean ExportAnimFileSTLPress (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;

	GtkWidget * psDialogue;
	GtkWindow * psParent;
	char * szFilename;
	char * szFolder;
	bool boExported;
	GtkFileFilter * psFilterAnim;
	GtkFileFilter * psFilterAll;
	GtkWidget * psBinary;
	GtkWidget * psMultiplier;
	GtkWidget * psScale;
	GtkWidget * psScreenCoords;
	GtkWidget * psTimeStart;
	GtkWidget * psTimeEnd;
	GtkWidget * psFrames;
	GtkWidget * psOptions;
	GtkWidget * psOptionsFile;
	GtkWidget * psOptionsAnim;
	GtkWidget * psWidgetAdd;
	GtkAdjustment * psAdjustment;
	ExportSTLPersist * psExportSTLData;

	psFilterAnim = gtk_file_filter_new ();
	gtk_file_filter_add_pattern (psFilterAnim, "*.zip");
	gtk_file_filter_add_mime_type (psFilterAnim, "binary/zip");
	gtk_file_filter_add_mime_type (psFilterAnim, "application/zip");
	gtk_file_filter_set_name (psFilterAnim, "Zip archive");

	psFilterAll = gtk_file_filter_new ();
	gtk_file_filter_add_pattern (psFilterAll, "*");
	gtk_file_filter_set_name (psFilterAll, "All Files");

	psParent = GTK_WINDOW (GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MainWindow")));

	psDialogue = gtk_file_chooser_dialog_new ("Export Animation Frames in STL", psParent, 
		GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, 
		GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);

	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (psDialogue), TRUE);

	g_object_set (G_OBJECT (psDialogue), "local-only", FALSE, NULL);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (psDialogue), psFilterAnim);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (psDialogue), psFilterAll);

	PauseAnimationModal (TRUE, psFunctyData);

	if (psFunctyData->boFolderSetAnim) {
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (psDialogue), psFunctyData->szFolderAnim->str);
	}
	else {
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (psDialogue), g_get_home_dir ());
	}

	if (!psFunctyData->boExportedAnim) {
		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (psDialogue), DEFAULT_EXPORTANIMNAME);
	}
	else {
		gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (psDialogue), psFunctyData->szExportAnimName->str);
	}

	psOptions = gtk_vbox_new (FALSE, 8);
	gtk_widget_show (psOptions);

	psOptionsFile = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (psOptionsFile);
	gtk_box_pack_start (GTK_BOX (psOptions), psOptionsFile, FALSE, TRUE, 0);

	psOptionsAnim = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (psOptionsAnim);
	gtk_box_pack_start (GTK_BOX (psOptions), psOptionsAnim, FALSE, TRUE, 0);

	// Add file attribute widgets
	psWidgetAdd = gtk_label_new ("Scale");
	gtk_label_set_justify (GTK_LABEL (psWidgetAdd), GTK_JUSTIFY_RIGHT);
	gtk_widget_show (psWidgetAdd);
	gtk_box_pack_start (GTK_BOX (psOptionsFile), psWidgetAdd, FALSE, TRUE, 0);

	psAdjustment = GTK_ADJUSTMENT (gtk_adjustment_new (psFunctyData->fExportScale, 0.001, 10000.0, 0.1, 10.0, 0.0));
	psScale = gtk_spin_button_new (psAdjustment, 0.1, 3);
	gtk_widget_show (psScale);
	gtk_box_pack_start (GTK_BOX (psOptionsFile), psScale, FALSE, TRUE, 0);

	psWidgetAdd = gtk_label_new ("Accuracy");
	gtk_label_set_justify (GTK_LABEL (psWidgetAdd), GTK_JUSTIFY_RIGHT);
	gtk_widget_show (psWidgetAdd);
	gtk_box_pack_start (GTK_BOX (psOptionsFile), psWidgetAdd, FALSE, TRUE, 0);

	psAdjustment = GTK_ADJUSTMENT (gtk_adjustment_new (psFunctyData->fExportMultiplier, 0.00, 1000.0, 0.1, 1.0, 0.0));
	psMultiplier = gtk_spin_button_new (psAdjustment, 1.0, 3);
	gtk_widget_show (psMultiplier);
	gtk_box_pack_start (GTK_BOX (psOptionsFile), psMultiplier, FALSE, TRUE, 0);

	psBinary = gtk_check_button_new_with_label ("Binary");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (psBinary), psFunctyData->boBinary);
	gtk_widget_show (psBinary);
	gtk_box_pack_start (GTK_BOX (psOptionsFile), psBinary, FALSE, TRUE, 0);

	psScreenCoords = gtk_check_button_new_with_label ("Screen coords");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (psScreenCoords), psFunctyData->boExportScreen);
	gtk_widget_show (psScreenCoords);
	gtk_box_pack_start (GTK_BOX (psOptionsFile), psScreenCoords, FALSE, TRUE, 0);

	// Add animation attribute widgets
	psWidgetAdd = gtk_label_new ("Start time");
	gtk_label_set_justify (GTK_LABEL (psWidgetAdd), GTK_JUSTIFY_RIGHT);
	gtk_widget_show (psWidgetAdd);
	gtk_box_pack_start (GTK_BOX (psOptionsAnim), psWidgetAdd, FALSE, TRUE, 0);

	psAdjustment = GTK_ADJUSTMENT (gtk_adjustment_new (psFunctyData->fExportTimeStart, 0, EXPORTANIM_TIME_MAX, 1, 1000, 0));
	psTimeStart = gtk_spin_button_new (psAdjustment, 1, 3);
	gtk_widget_show (psTimeStart);
	gtk_box_pack_start (GTK_BOX (psOptionsAnim), psTimeStart, FALSE, TRUE, 0);

	psWidgetAdd = gtk_label_new ("End time");
	gtk_label_set_justify (GTK_LABEL (psWidgetAdd), GTK_JUSTIFY_RIGHT);
	gtk_widget_show (psWidgetAdd);
	gtk_box_pack_start (GTK_BOX (psOptionsAnim), psWidgetAdd, FALSE, TRUE, 0);

	psAdjustment = GTK_ADJUSTMENT (gtk_adjustment_new (psFunctyData->fExportTimeEnd, 0, EXPORTANIM_TIME_MAX, 1, 1000, 0));
	psTimeEnd = gtk_spin_button_new (psAdjustment, 1, 3);
	gtk_widget_show (psTimeEnd);
	gtk_box_pack_start (GTK_BOX (psOptionsAnim), psTimeEnd, FALSE, TRUE, 0);

	psWidgetAdd = gtk_label_new ("Frames");
	gtk_label_set_justify (GTK_LABEL (psWidgetAdd), GTK_JUSTIFY_RIGHT);
	gtk_widget_show (psWidgetAdd);
	gtk_box_pack_start (GTK_BOX (psOptionsAnim), psWidgetAdd, FALSE, TRUE, 0);

	psAdjustment = GTK_ADJUSTMENT (gtk_adjustment_new (psFunctyData->nExportFrames, 0, pow(10, EXPORTANIM_FRAMES_EXP), 1, 10, 0));
	psFrames = gtk_spin_button_new (psAdjustment, 1, 0);
	gtk_widget_show (psFrames);
	gtk_box_pack_start (GTK_BOX (psOptionsAnim), psFrames, FALSE, TRUE, 0);

	// Install the widgets into the filesave dialogue
	gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (psDialogue), psOptions);

	if (gtk_dialog_run (GTK_DIALOG (psDialogue)) == GTK_RESPONSE_ACCEPT) {
		psFunctyData->boBinary = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (psBinary));
		psFunctyData->fExportMultiplier = gtk_spin_button_get_value (GTK_SPIN_BUTTON (psMultiplier));
		psFunctyData->fExportScale = gtk_spin_button_get_value (GTK_SPIN_BUTTON (psScale));
		psFunctyData->boExportScreen = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (psScreenCoords));
		psFunctyData->fExportTimeStart = gtk_spin_button_get_value (GTK_SPIN_BUTTON (psTimeStart));
		psFunctyData->fExportTimeEnd = gtk_spin_button_get_value (GTK_SPIN_BUTTON (psTimeEnd));
		psFunctyData->nExportFrames = gtk_spin_button_get_value (GTK_SPIN_BUTTON (psFrames));
		szFilename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (psDialogue));

		// Actually perform the model export
		gtk_widget_hide (psDialogue);
		psExportSTLData = NewExportSTLPersist (szFilename, psFunctyData->boBinary, psFunctyData->boExportScreen, psFunctyData->fExportMultiplier, psFunctyData->fExportScale, psFunctyData->fExportTimeStart, psFunctyData->fExportTimeEnd, psFunctyData->nExportFrames, psFunctyData->psVisData);
		boExported = LongPoll (psFunctyData->psXML, psExportSTLData, ExportStartAnimatedSTL, ExportStepAnimatedSTL, ExportFinishAnimatedSTL, ExportCancelAnimatedSTL);

		if (boExported) {
			g_string_assign (psFunctyData->szExportAnimName, szFilename);

			szFolder = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (psDialogue));
			psFunctyData->boFolderSet = (szFolder != NULL);
			if (szFolder) {
				g_string_assign (psFunctyData->szFolder, szFolder);
				g_free (szFolder);
				szFolder = NULL;
			}
		}
		psFunctyData->boExportedAnim = boExported;
		g_free (szFilename);
		szFilename = NULL;
	}
	gtk_widget_destroy (psDialogue);

	PauseAnimationModal (FALSE, psFunctyData);

	return TRUE;
}

static gboolean BitmapWindowSizeToggle (GtkWidget * psWidget, gpointer psData) {
	BitmapSize * psBitmapSizeData = (BitmapSize * )psData;
	bool boWindowSize;
	
	boWindowSize = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (psWidget));
	
	gtk_widget_set_sensitive (psBitmapSizeData->psWidth, !boWindowSize);
	gtk_widget_set_sensitive (psBitmapSizeData->psWidthLabel, !boWindowSize);
	gtk_widget_set_sensitive (psBitmapSizeData->psHeight, !boWindowSize);
	gtk_widget_set_sensitive (psBitmapSizeData->psHeightLabel, !boWindowSize);

	return TRUE;
}

static gboolean ExportBitmapPress (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
	GtkWidget * psDialogue;
	GtkWindow * psParent;
	char * szFilename;
	char * szFolder;
	bool boExported;
	GtkFileFilter * psFilterPNG;
	GtkFileFilter * psFilterAll;
	BitmapSize sSizeWidgets;
	GtkWidget * psWidgetAdd;
	GtkWidget * psOptions;
	GtkWidget * psAlign;

	psFilterPNG = gtk_file_filter_new ();
	gtk_file_filter_add_pattern (psFilterPNG, "*.png");
	gtk_file_filter_add_mime_type (psFilterPNG, "image/png");
	gtk_file_filter_add_mime_type (psFilterPNG, "application/png");
	gtk_file_filter_set_name (psFilterPNG, "Portable Network Graphics");

	psFilterAll = gtk_file_filter_new ();
	gtk_file_filter_add_pattern (psFilterAll, "*");
	gtk_file_filter_set_name (psFilterAll, "All Files");

	psParent = GTK_WINDOW (GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MainWindow")));

	psDialogue = gtk_file_chooser_dialog_new ("Export Bitmap", psParent, GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);

	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (psDialogue), TRUE);
	g_object_set (G_OBJECT (psDialogue), "local-only", FALSE, NULL);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (psDialogue), psFilterPNG);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (psDialogue), psFilterAll);

	if (psFunctyData->boFolderSet) {
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (psDialogue), psFunctyData->szFolder->str);
	}
	else {
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (psDialogue), g_get_home_dir ());
	}

	if (!psFunctyData->boExportedBitmap) {
		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (psDialogue), DEFAULT_EXPORTBITMAPNAME);
	}
	else {
		gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (psDialogue), psFunctyData->szExportBitmapName->str);
	}

	psOptions = gtk_hbox_new (FALSE, 8);

	psAlign = gtk_alignment_new (0.0, 0.5, 0, 0);
	gtk_box_pack_start (GTK_BOX (psOptions), psAlign, FALSE, TRUE, 0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (psAlign), 0, 0, 0, 20);
	gtk_widget_show(psAlign);

	psWidgetAdd = gtk_check_button_new_with_label ("Window size");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (psWidgetAdd), psFunctyData->boBitmapScreenDimensions);
	gtk_container_add (GTK_CONTAINER (psAlign), psWidgetAdd);
	gtk_widget_show (psWidgetAdd);
	sSizeWidgets.psWindowSize = psWidgetAdd;

	psAlign = gtk_alignment_new (1.0, 0.5, 0, 0);
	gtk_box_pack_start (GTK_BOX (psOptions), psAlign, FALSE, TRUE, 0);
	gtk_widget_show(psAlign);

	psWidgetAdd = gtk_label_new ("Width");
	gtk_container_add (GTK_CONTAINER (psAlign), psWidgetAdd);
	gtk_widget_show (psWidgetAdd);
	sSizeWidgets.psWidthLabel = psWidgetAdd;

	psAlign = gtk_alignment_new (0.0, 0.5, 0, 0);
	gtk_box_pack_start (GTK_BOX (psOptions), psAlign, FALSE, TRUE, 0);
	gtk_widget_show(psAlign);

	psWidgetAdd = gtk_spin_button_new_with_range (1, 2048, 1);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (psWidgetAdd), psFunctyData->nBitmapWidth);
	gtk_container_add (GTK_CONTAINER (psAlign), psWidgetAdd);
	gtk_widget_show (psWidgetAdd);
	sSizeWidgets.psWidth = psWidgetAdd;

	psAlign = gtk_alignment_new (1.0, 0.5, 0, 0);
	gtk_box_pack_start (GTK_BOX (psOptions), psAlign, FALSE, TRUE, 0);
	gtk_widget_show(psAlign);

	psWidgetAdd = gtk_label_new ("Height");
	gtk_container_add (GTK_CONTAINER (psAlign), psWidgetAdd);
	gtk_widget_show (psWidgetAdd);
	sSizeWidgets.psHeightLabel = psWidgetAdd;

	psAlign = gtk_alignment_new (0.0, 0.5, 0, 0);
	gtk_box_pack_start (GTK_BOX (psOptions), psAlign, FALSE, TRUE, 0);
	gtk_widget_show(psAlign);

	psWidgetAdd = gtk_spin_button_new_with_range (1, 2048, 1);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (psWidgetAdd), psFunctyData->nBitmapHeight);
	gtk_container_add (GTK_CONTAINER (psAlign), psWidgetAdd);
	gtk_widget_show (psWidgetAdd);
	sSizeWidgets.psHeight = psWidgetAdd;

	g_signal_connect (sSizeWidgets.psWindowSize, "toggled", G_CALLBACK (BitmapWindowSizeToggle), (gpointer)(& sSizeWidgets));

	gtk_widget_show (psOptions);
	gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (psDialogue), psOptions);

	BitmapWindowSizeToggle (sSizeWidgets.psWindowSize, (gpointer)(& sSizeWidgets));

	if (gtk_dialog_run (GTK_DIALOG (psDialogue)) == GTK_RESPONSE_ACCEPT) {
		psFunctyData->boBitmapScreenDimensions = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (sSizeWidgets.psWindowSize));

		if (psFunctyData->boBitmapScreenDimensions) {
			psFunctyData->nBitmapWidth = GetScreenWidth (psFunctyData->psVisData);
			psFunctyData->nBitmapHeight = GetScreenHeight (psFunctyData->psVisData);
		}
		else {
			psFunctyData->nBitmapWidth = gtk_spin_button_get_value (GTK_SPIN_BUTTON (sSizeWidgets.psWidth));
			psFunctyData->nBitmapHeight = gtk_spin_button_get_value (GTK_SPIN_BUTTON (sSizeWidgets.psHeight));
		}

		szFilename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (psDialogue));

		boExported = ExportBitmapFilePNG (szFilename, psFunctyData->nBitmapHeight, psFunctyData->nBitmapWidth, psFunctyData->psVisData);

		if (boExported) {
			g_string_assign (psFunctyData->szExportBitmapName, szFilename);

			szFolder = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (psDialogue));
			psFunctyData->boFolderSet = (szFolder != NULL);
			if (szFolder) {
				g_string_assign (psFunctyData->szFolder, szFolder);
				g_free (szFolder);
				szFolder = NULL;
			}
		}
		psFunctyData->boExportedBitmap = boExported;
		g_free (szFilename);
		szFilename = NULL;
	}
	else {
		psFunctyData->boBitmapScreenDimensions = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (sSizeWidgets.psWindowSize));
		psFunctyData->nBitmapWidth = gtk_spin_button_get_value (GTK_SPIN_BUTTON (sSizeWidgets.psWidth));
		psFunctyData->nBitmapHeight = gtk_spin_button_get_value (GTK_SPIN_BUTTON (sSizeWidgets.psHeight));
	}

	gtk_widget_destroy (psDialogue);

	return TRUE;
}

static gboolean ExportAnimFilePNGPress (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;

	GtkWidget * psDialogue;
	GtkWindow * psParent;
	char * szFilename;
	char * szFolder;
	bool boExported;
	GtkFileFilter * psFilterAnim;
	GtkFileFilter * psFilterAll;
	GtkWidget * psTimeStart;
	GtkWidget * psTimeEnd;
	GtkWidget * psFrames;
	GtkWidget * psOptions;
	GtkWidget * psOptionsFile;
	GtkWidget * psOptionsAnim;
	GtkWidget * psWidgetAdd;
	GtkAdjustment * psAdjustment;
	GtkWidget * psAlign;
	BitmapSize sSizeWidgets;
	ExportBitmapPersist * psExportBitmapData;

	psFilterAnim = gtk_file_filter_new ();
	gtk_file_filter_add_pattern (psFilterAnim, "*.zip");
	gtk_file_filter_add_mime_type (psFilterAnim, "binary/zip");
	gtk_file_filter_add_mime_type (psFilterAnim, "application/zip");
	gtk_file_filter_set_name (psFilterAnim, "Zip archive");

	psFilterAll = gtk_file_filter_new ();
	gtk_file_filter_add_pattern (psFilterAll, "*");
	gtk_file_filter_set_name (psFilterAll, "All Files");

	psParent = GTK_WINDOW (GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MainWindow")));

	psDialogue = gtk_file_chooser_dialog_new ("Export Animation Frames in PNG", psParent, 
		GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, 
		GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);

	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (psDialogue), TRUE);

	g_object_set (G_OBJECT (psDialogue), "local-only", FALSE, NULL);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (psDialogue), psFilterAnim);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (psDialogue), psFilterAll);

	PauseAnimationModal (TRUE, psFunctyData);

	if (psFunctyData->boFolderSetAnim) {
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (psDialogue), psFunctyData->szFolderAnim->str);
	}
	else {
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (psDialogue), g_get_home_dir ());
	}

	if (!psFunctyData->boExportedAnim) {
		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (psDialogue), DEFAULT_EXPORTANIMNAME);
	}
	else {
		gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (psDialogue), psFunctyData->szExportAnimName->str);
	}

	psOptions = gtk_vbox_new (FALSE, 8);
	gtk_widget_show (psOptions);

	psOptionsFile = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (psOptionsFile);
	gtk_box_pack_start (GTK_BOX (psOptions), psOptionsFile, FALSE, TRUE, 0);

	psOptionsAnim = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (psOptionsAnim);
	gtk_box_pack_start (GTK_BOX (psOptions), psOptionsAnim, FALSE, TRUE, 0);

	// Add file attribute widgets
	psAlign = gtk_alignment_new (0.0, 0.5, 0, 0);
	gtk_box_pack_start (GTK_BOX (psOptionsFile), psAlign, FALSE, TRUE, 0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (psAlign), 0, 0, 0, 20);
	gtk_widget_show(psAlign);

	psWidgetAdd = gtk_check_button_new_with_label ("Window size");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (psWidgetAdd), psFunctyData->boBitmapScreenDimensions);
	gtk_container_add (GTK_CONTAINER (psAlign), psWidgetAdd);
	gtk_widget_show (psWidgetAdd);
	sSizeWidgets.psWindowSize = psWidgetAdd;

	psAlign = gtk_alignment_new (1.0, 0.5, 0, 0);
	gtk_box_pack_start (GTK_BOX (psOptionsFile), psAlign, FALSE, TRUE, 0);
	gtk_widget_show(psAlign);

	psWidgetAdd = gtk_label_new ("Width");
	gtk_container_add (GTK_CONTAINER (psAlign), psWidgetAdd);
	gtk_widget_show (psWidgetAdd);
	sSizeWidgets.psWidthLabel = psWidgetAdd;

	psAlign = gtk_alignment_new (0.0, 0.5, 0, 0);
	gtk_box_pack_start (GTK_BOX (psOptionsFile), psAlign, FALSE, TRUE, 0);
	gtk_widget_show(psAlign);

	psWidgetAdd = gtk_spin_button_new_with_range (1, 2048, 1);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (psWidgetAdd), psFunctyData->nBitmapWidth);
	gtk_container_add (GTK_CONTAINER (psAlign), psWidgetAdd);
	gtk_widget_show (psWidgetAdd);
	sSizeWidgets.psWidth = psWidgetAdd;

	psAlign = gtk_alignment_new (1.0, 0.5, 0, 0);
	gtk_box_pack_start (GTK_BOX (psOptionsFile), psAlign, FALSE, TRUE, 0);
	gtk_widget_show(psAlign);

	psWidgetAdd = gtk_label_new ("Height");
	gtk_container_add (GTK_CONTAINER (psAlign), psWidgetAdd);
	gtk_widget_show (psWidgetAdd);
	sSizeWidgets.psHeightLabel = psWidgetAdd;

	psAlign = gtk_alignment_new (0.0, 0.5, 0, 0);
	gtk_box_pack_start (GTK_BOX (psOptionsFile), psAlign, FALSE, TRUE, 0);
	gtk_widget_show(psAlign);

	psWidgetAdd = gtk_spin_button_new_with_range (1, 2048, 1);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (psWidgetAdd), psFunctyData->nBitmapHeight);
	gtk_container_add (GTK_CONTAINER (psAlign), psWidgetAdd);
	gtk_widget_show (psWidgetAdd);
	sSizeWidgets.psHeight = psWidgetAdd;

	g_signal_connect (sSizeWidgets.psWindowSize, "toggled", G_CALLBACK (BitmapWindowSizeToggle), (gpointer)(& sSizeWidgets));

	BitmapWindowSizeToggle (sSizeWidgets.psWindowSize, (gpointer)(& sSizeWidgets));

	// Add animation attribute widgets
	psWidgetAdd = gtk_label_new ("Start time");
	gtk_label_set_justify (GTK_LABEL (psWidgetAdd), GTK_JUSTIFY_RIGHT);
	gtk_widget_show (psWidgetAdd);
	gtk_box_pack_start (GTK_BOX (psOptionsAnim), psWidgetAdd, FALSE, TRUE, 0);

	psAdjustment = GTK_ADJUSTMENT (gtk_adjustment_new (psFunctyData->fExportTimeStart, 0, EXPORTANIM_TIME_MAX, 1, 1000, 0));
	psTimeStart = gtk_spin_button_new (psAdjustment, 1, 3);
	gtk_widget_show (psTimeStart);
	gtk_box_pack_start (GTK_BOX (psOptionsAnim), psTimeStart, FALSE, TRUE, 0);

	psWidgetAdd = gtk_label_new ("End time");
	gtk_label_set_justify (GTK_LABEL (psWidgetAdd), GTK_JUSTIFY_RIGHT);
	gtk_widget_show (psWidgetAdd);
	gtk_box_pack_start (GTK_BOX (psOptionsAnim), psWidgetAdd, FALSE, TRUE, 0);

	psAdjustment = GTK_ADJUSTMENT (gtk_adjustment_new (psFunctyData->fExportTimeEnd, 0, EXPORTANIM_TIME_MAX, 1, 1000, 0));
	psTimeEnd = gtk_spin_button_new (psAdjustment, 1, 3);
	gtk_widget_show (psTimeEnd);
	gtk_box_pack_start (GTK_BOX (psOptionsAnim), psTimeEnd, FALSE, TRUE, 0);

	psWidgetAdd = gtk_label_new ("Frames");
	gtk_label_set_justify (GTK_LABEL (psWidgetAdd), GTK_JUSTIFY_RIGHT);
	gtk_widget_show (psWidgetAdd);
	gtk_box_pack_start (GTK_BOX (psOptionsAnim), psWidgetAdd, FALSE, TRUE, 0);

	psAdjustment = GTK_ADJUSTMENT (gtk_adjustment_new (psFunctyData->nExportFrames, 0, pow(10, EXPORTANIM_FRAMES_EXP), 1, 10, 0));
	psFrames = gtk_spin_button_new (psAdjustment, 1, 0);
	gtk_widget_show (psFrames);
	gtk_box_pack_start (GTK_BOX (psOptionsAnim), psFrames, FALSE, TRUE, 0);

	// Install the widgets into the filesave dialogue
	gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (psDialogue), psOptions);

	if (gtk_dialog_run (GTK_DIALOG (psDialogue)) == GTK_RESPONSE_ACCEPT) {
		psFunctyData->boBitmapScreenDimensions = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (sSizeWidgets.psWindowSize));

		if (psFunctyData->boBitmapScreenDimensions) {
			psFunctyData->nBitmapWidth = GetScreenWidth (psFunctyData->psVisData);
			psFunctyData->nBitmapHeight = GetScreenHeight (psFunctyData->psVisData);
		}
		else {
			psFunctyData->nBitmapWidth = gtk_spin_button_get_value (GTK_SPIN_BUTTON (sSizeWidgets.psWidth));
			psFunctyData->nBitmapHeight = gtk_spin_button_get_value (GTK_SPIN_BUTTON (sSizeWidgets.psHeight));
		}
		psFunctyData->fExportTimeStart = gtk_spin_button_get_value (GTK_SPIN_BUTTON (psTimeStart));
		psFunctyData->fExportTimeEnd = gtk_spin_button_get_value (GTK_SPIN_BUTTON (psTimeEnd));
		psFunctyData->nExportFrames = gtk_spin_button_get_value (GTK_SPIN_BUTTON (psFrames));
		szFilename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (psDialogue));

		// Actually perform the model export
		gtk_widget_hide (psDialogue);
		psExportBitmapData = NewExportBitmapPersist (szFilename, "png", psFunctyData->nBitmapHeight, psFunctyData->nBitmapWidth, psFunctyData->fExportTimeStart, psFunctyData->fExportTimeEnd, psFunctyData->nExportFrames, psFunctyData->psVisData);
		boExported = LongPoll (psFunctyData->psXML, psExportBitmapData, ExportStartAnimatedBitmap, ExportStepAnimatedBitmap, ExportFinishAnimatedBitmap, ExportCancelAnimatedBitmap);

		if (boExported) {
			g_string_assign (psFunctyData->szExportAnimName, szFilename);

			szFolder = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (psDialogue));
			psFunctyData->boFolderSet = (szFolder != NULL);
			if (szFolder) {
				g_string_assign (psFunctyData->szFolder, szFolder);
				g_free (szFolder);
				szFolder = NULL;
			}
		}
		psFunctyData->boExportedAnim = boExported;
		g_free (szFilename);
		szFilename = NULL;
	}
	else {
		psFunctyData->boBitmapScreenDimensions = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (sSizeWidgets.psWindowSize));
		psFunctyData->nBitmapWidth = gtk_spin_button_get_value (GTK_SPIN_BUTTON (sSizeWidgets.psWidth));
		psFunctyData->nBitmapHeight = gtk_spin_button_get_value (GTK_SPIN_BUTTON (sSizeWidgets.psHeight));
	}
	gtk_widget_destroy (psDialogue);

	PauseAnimationModal (FALSE, psFunctyData);

	return TRUE;
}

void PauseAnimationModal (bool boPauseModal, FunctyPersist * psFunctyData) {
	psFunctyData->boPausedModal = boPauseModal;
	if (boPauseModal) {
		TimeoutRemove (psFunctyData);
	}
	else {
		TimeoutAdd (psFunctyData);
	}
}

void LoadLicence (FunctyPersist * psFunctyData) {
	GFile * psLicence;
	GString * szPath;

	szPath = g_string_new ("/COPYING");
	GenerateDataPath (szPath, psFunctyData->psGlobalData);
	psLicence = g_file_new_for_path (szPath->str);
	g_string_free (szPath, TRUE);

	g_file_load_contents (psLicence, NULL, & psFunctyData->szLicence, NULL, NULL, NULL);
	g_object_unref (psLicence);
}

static gboolean AboutShow (GtkWidget * psWidget, gpointer psData) {
	FunctyPersist * psFunctyData = (FunctyPersist * )psData;
	GtkWidget * psWindow;
	GdkPixbuf * psIcon;

	psWindow = GTK_WIDGET (gtk_builder_get_object (psFunctyData->psXML, "MainWindow"));
	psIcon = gtk_window_get_icon (GTK_WINDOW (psWindow));

	gtk_show_about_dialog (GTK_WINDOW (psWindow),
		"program-name", "Functy",
		"comments", "3D graph drawing package. Render Cartesian, spherical and parametric curve functions on the GPU.",
		"copyright", "Copyright © 2016 David Llewellyn-Jones",
		"logo", psIcon,
		"title", "About Functy",
		"wrap-license", TRUE,
		"license", psFunctyData->szLicence,
		"website", "http://functy.sourceforge.net/",
		"version", VERSION,
		NULL);

	return TRUE;
}

static gboolean DocumentationShow (GtkWidget * psWidget, gpointer psData) {
	bool boSuccess;

	boSuccess = gtk_show_uri (NULL, "https://sourceforge.net/p/functy/wiki/Home/", GDK_CURRENT_TIME, NULL);

	return boSuccess;
}

static gboolean HomepageShow (GtkWidget * psWidget, gpointer psData) {
	bool boSuccess;

	boSuccess = gtk_show_uri (NULL, "http://functy.sourceforge.net/", GDK_CURRENT_TIME, NULL);

	return boSuccess;
}

int main (int argc, char *argv[]) {
	GdkGLConfig * GlConfig;
	GtkWidget * psWindow;
	GtkWidget * psWidget;
	VisPersist * psVisData;
	FunctyPersist * psFunctyData;
	GtkListStore * psFunctionListModel;
	GtkCellRenderer * psListCellRenderer;
	GtkTreeViewColumn * psListColumn;
	int nSlider;
	GString * szWidgetName;
	GtkTreeSelection * psSelection;
	GtkAdjustment * psAdjustment;
	GString * szPath;
	GOptionContext * psOptionsContext;
	GError * psError = NULL;

	// Initialise various libraries
	gtk_init (&argc, &argv);
	gtk_gl_init (&argc, &argv);
	glutInit (&argc, argv);

	psOptionsContext = g_option_context_new ("- realtime display of 3D mathematical functions");
	g_option_context_add_main_entries (psOptionsContext, asEntries, NULL);
	g_option_context_add_group (psOptionsContext, gtk_get_option_group (TRUE));
	if (!g_option_context_parse (psOptionsContext, &argc, &argv, & psError)) {
		g_print ("Option parsing failed: %s\n", psError->message);
		exit (1);
	}

	// Create new persistent structures
	psFunctyData = NewFunctyPersist ();
	psVisData = NewVisPersist (psFunctyData->psGlobalData);
	psFunctyData->psVisData = psVisData;

	if (szArvOptionDataDir) {
		SetDataDir (szArvOptionDataDir, psFunctyData->psGlobalData);
	}

	// First try double buffering
	GlConfig = gdk_gl_config_new_by_mode (GDK_GL_MODE_RGBA | GDK_GL_MODE_DEPTH	| GDK_GL_MODE_DOUBLE | GDK_GL_MODE_ALPHA | GDK_GL_MODE_STENCIL);
	if (GlConfig == NULL) {
		g_print ("Failed to initialise double buffered visual; trying single buffer.\n");

		// If that fails, we'll try single buffered
		GlConfig = gdk_gl_config_new_by_mode (GDK_GL_MODE_RGBA | GDK_GL_MODE_DEPTH | GDK_GL_MODE_ALPHA | GDK_GL_MODE_STENCIL);
		if (GlConfig == NULL) {
			g_print ("OpenGL visual could not be initialised.\n");
			exit (1);
		}
	}

	// Load the licence file
	LoadLicence (psFunctyData);

	// Load the user interface
	psFunctyData->psXML = gtk_builder_new ();
	szPath = g_string_new("/functy.ui");
	GenerateDataPath (szPath, psFunctyData->psGlobalData);
	gtk_builder_add_from_file (psFunctyData->psXML, szPath->str, NULL);
	g_string_free (szPath, TRUE);

	// Main window
	psWindow = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MainWindow"));
	gtk_container_set_reallocate_redraws (GTK_CONTAINER (psWindow), TRUE);
	psFunctyData->psDrawingArea = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "DrawingArea"));

	// Automatically redraw the window children change allocation
	gtk_container_set_reallocate_redraws (GTK_CONTAINER (psWindow), TRUE);

	// Set up OpenGL for the drawing area widget
	gtk_widget_set_gl_capability (psFunctyData->psDrawingArea, GlConfig, NULL, TRUE, GDK_GL_RGBA_TYPE);

	// Connect up relevant signals
	gtk_builder_connect_signals (psFunctyData->psXML, NULL);

	g_signal_connect_after (G_OBJECT (psFunctyData->psDrawingArea), "realize", G_CALLBACK (Realize), (gpointer)psFunctyData);
	g_signal_connect (G_OBJECT (psFunctyData->psDrawingArea), "configure_event", G_CALLBACK (ConfigureEvent), (gpointer)psFunctyData);
	g_signal_connect (G_OBJECT (psFunctyData->psDrawingArea), "expose_event", G_CALLBACK (ExposeEvent), (gpointer)psFunctyData);

	g_signal_connect (G_OBJECT (psFunctyData->psDrawingArea), "button_press_event", G_CALLBACK (ButtonPressEvent), (gpointer)psFunctyData);
	g_signal_connect (G_OBJECT (psFunctyData->psDrawingArea), "button_release_event", G_CALLBACK (ButtonReleaseEvent), (gpointer)psFunctyData);
	//g_signal_connect (G_OBJECT (psFunctyData->psDrawingArea), "motion_notify_event", G_CALLBACK (MotionNotifyEvent), (gpointer)psFunctyData);
	g_signal_connect (G_OBJECT (psFunctyData->psDrawingArea), "scroll_event", G_CALLBACK (ScrollEvent), (gpointer)psFunctyData);

	g_signal_connect (G_OBJECT (psFunctyData->psDrawingArea), "map_event", G_CALLBACK (MapEvent), (gpointer)psFunctyData);
	g_signal_connect (G_OBJECT (psFunctyData->psDrawingArea), "unmap_event", G_CALLBACK (UnmapEvent), (gpointer)psFunctyData);
	g_signal_connect (G_OBJECT (psFunctyData->psDrawingArea), "visibility_notify_event", G_CALLBACK (VisibilityNotifyEvent), (gpointer)psFunctyData);

	g_signal_connect (G_OBJECT (psWindow), "key_press_event", G_CALLBACK (KeyPressEvent), (gpointer)psFunctyData);
	g_signal_connect (G_OBJECT (psWindow), "key_release_event", G_CALLBACK (KeyReleaseEvent), (gpointer)psFunctyData);

	//g_signal_connect (G_OBJECT (psFunctyData->psDrawingArea), "key_press_event", G_CALLBACK (KeyPressEvent), (gpointer)psFunctyData);
	//g_signal_connect (G_OBJECT (psFunctyData->psDrawingArea), "key_release_event", G_CALLBACK (KeyReleaseEvent), (gpointer)psFunctyData);



	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "PlotApply"));
	g_signal_connect (psWidget, "clicked", G_CALLBACK (PlotFunctionApply), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "DeleteFunction"));
	g_signal_connect (psWidget, "clicked", G_CALLBACK (DeleteFunctionPress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "PlotCancel"));
	g_signal_connect (psWidget, "clicked", G_CALLBACK (PlotFunctionCancel), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Open"));
	g_signal_connect (psWidget, "clicked", G_CALLBACK (LoadFilePress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "SaveAs"));
	g_signal_connect (psWidget, "clicked", G_CALLBACK (SaveFilePress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "ExportModelPLY"));
	g_signal_connect (psWidget, "clicked", G_CALLBACK (ExportFilePLYPress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "ExportAnimPLY"));
	g_signal_connect (psWidget, "clicked", G_CALLBACK (ExportAnimFilePLYPress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Pause"));
	g_signal_connect (psWidget, "clicked", G_CALLBACK (TogglePausePress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TogglePanelLeft"));
	g_signal_connect (psWidget, "clicked", G_CALLBACK (TogglePanelLeftPress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TogglePanelBottom"));
	g_signal_connect (psWidget, "clicked", G_CALLBACK (TogglePanelBottomPress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "FullscreenButton"));
	g_signal_connect (psWidget, "clicked", G_CALLBACK (FullscreenPress), (gpointer)psFunctyData);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "AddCartesian"));
	g_signal_connect (psWidget, "clicked", G_CALLBACK (AddCartesianPress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "AddSpherical"));
	g_signal_connect (psWidget, "clicked", G_CALLBACK (AddSphericalPress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "AddCurve"));
	g_signal_connect (psWidget, "clicked", G_CALLBACK (AddCurvePress), (gpointer)psFunctyData);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "NewCartesian"));
	g_signal_connect (psWidget, "clicked", G_CALLBACK (AddCartesianPress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "NewSpherical"));
	g_signal_connect (psWidget, "clicked", G_CALLBACK (AddSphericalPress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "NewCurve"));
	g_signal_connect (psWidget, "clicked", G_CALLBACK (AddCurvePress), (gpointer)psFunctyData);


	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "VariablesApply"));
	g_signal_connect (psWidget, "clicked", G_CALLBACK (ControlVariablesApply), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "VariablesCancel"));
	g_signal_connect (psWidget, "clicked", G_CALLBACK (ControlVariablesCancel), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "RangeApply"));
	g_signal_connect (psWidget, "clicked", G_CALLBACK (RangeSetApply), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "RangeCancel"));
	g_signal_connect (psWidget, "clicked", G_CALLBACK (RangeSetCancel), (gpointer)psFunctyData);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "AudioConfigOkay"));
	g_signal_connect (psWidget, "clicked", G_CALLBACK (AudioConfigOkay), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "AudioConfigCancel"));
	g_signal_connect (psWidget, "clicked", G_CALLBACK (AudioConfigClose), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "AudioConfig"));
	g_signal_connect (psWidget, "delete-event", G_CALLBACK (AudioConfigDestroy), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "AudioActive"));
	g_signal_connect (GTK_TOGGLE_BUTTON (psWidget), "toggled", G_CALLBACK (AudioActiveChangedCallback), (gpointer)psFunctyData);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuOpen"));
	g_signal_connect (psWidget, "activate", G_CALLBACK (LoadFilePress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuSaveAs"));
	g_signal_connect (psWidget, "activate", G_CALLBACK (SaveFilePress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuAudioConfig"));
	g_signal_connect (psWidget, "activate", G_CALLBACK (AudioConfigPress), (gpointer)psFunctyData);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuExportModelPLY"));
	g_signal_connect (psWidget, "activate", G_CALLBACK (ExportFilePLYPress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuExportModelSTL"));
	g_signal_connect (psWidget, "activate", G_CALLBACK (ExportFileSTLPress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuExportModelSVX"));
	g_signal_connect (psWidget, "activate", G_CALLBACK (ExportFileSVXPress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuExportModelVDB"));
	g_signal_connect (psWidget, "activate", G_CALLBACK (ExportFileVDBPress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuExportBitmapPNG"));
	g_signal_connect (psWidget, "activate", G_CALLBACK (ExportBitmapPress), (gpointer)psFunctyData);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuExportAnimPLY"));
	g_signal_connect (psWidget, "activate", G_CALLBACK (ExportAnimFilePLYPress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuExportAnimSTL"));
	g_signal_connect (psWidget, "activate", G_CALLBACK (ExportAnimFileSTLPress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuExportAnimPNG"));
	g_signal_connect (psWidget, "activate", G_CALLBACK (ExportAnimFilePNGPress), (gpointer)psFunctyData);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuFullscreen"));
	g_signal_connect (psWidget, "activate", G_CALLBACK (FullscreenPress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuPanelLeft"));
	g_signal_connect (psWidget, "activate", G_CALLBACK (TogglePanelLeftPress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuPanelBottom"));
	g_signal_connect (psWidget, "activate", G_CALLBACK (TogglePanelBottomPress), (gpointer)psFunctyData);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuDrawAxes"));
	g_signal_connect (psWidget, "toggled", G_CALLBACK (ToggleDrawAxesPress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuSpin"));
	g_signal_connect (psWidget, "toggled", G_CALLBACK (ToggleSpinPress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuInvert"));
	g_signal_connect (psWidget, "toggled", G_CALLBACK (ToggleInvertPress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuWireframe"));
	g_signal_connect (psWidget, "toggled", G_CALLBACK (ToggleWireframePress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuUseShaders"));
	g_signal_connect (psWidget, "toggled", G_CALLBACK (ToggleUseShadersPress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuShadow"));
	g_signal_connect (psWidget, "toggled", G_CALLBACK (ToggleShadowPress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuFocusBlur"));
	g_signal_connect (psWidget, "toggled", G_CALLBACK (ToggleFocusBlurPress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuPause"));
	g_signal_connect (psWidget, "toggled", G_CALLBACK (TogglePausePress), (gpointer)psFunctyData);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuAddCartesian"));
	g_signal_connect (psWidget, "activate", G_CALLBACK (AddCartesianPress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuAddSpherical"));
	g_signal_connect (psWidget, "activate", G_CALLBACK (AddSphericalPress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuAddCurve"));
	g_signal_connect (psWidget, "activate", G_CALLBACK (AddCurvePress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuDeleteFunction"));
	g_signal_connect (psWidget, "activate", G_CALLBACK (DeleteFunctionPress), (gpointer)psFunctyData);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuButtonBarNone"));
	g_signal_connect (psWidget, "toggled", G_CALLBACK (SetButtonBarStyleNone), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuButtonBarIcons"));
	g_signal_connect (psWidget, "toggled", G_CALLBACK (SetButtonBarStyleIcons), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuButtonBarIconsText"));
	g_signal_connect (psWidget, "toggled", G_CALLBACK (SetButtonBarStyleIconsText), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuAbout"));
	g_signal_connect (psWidget, "activate", G_CALLBACK (AboutShow), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuDocumentation"));
	g_signal_connect (psWidget, "activate", G_CALLBACK (DocumentationShow), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MenuHomepage"));
	g_signal_connect (psWidget, "activate", G_CALLBACK (HomepageShow), (gpointer)psFunctyData);


	/****************************************************************
	 * Removed during move from multiple dialogues to single window

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Plot"));
	g_signal_connect (psWidget, "clicked", G_CALLBACK (PlotFunctionPress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "OpenView"));
	g_signal_connect (psWidget, "clicked", G_CALLBACK (ViewPress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "OpenRange"));
	g_signal_connect (psWidget, "clicked", G_CALLBACK (RangePress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "RangeOkay"));
	g_signal_connect (psWidget, "clicked", G_CALLBACK (RangeSetPress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "VariablesOkay"));
	g_signal_connect (psWidget, "clicked", G_CALLBACK (ControlVariablesPress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "OpenVariables"));
	g_signal_connect (psWidget, "clicked", G_CALLBACK (ControlvarPress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "EditFunction"));
	g_signal_connect (psWidget, "clicked", G_CALLBACK (EditFunctionPress), (gpointer)psFunctyData);


	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Fullscreen"));
	g_signal_connect (psWidget, "toggled", G_CALLBACK (ToggleFullscreenPress), (gpointer)psFunctyData);







	****************************************************************/



	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "FunctionName"));
	g_signal_connect (psWidget, "activate", G_CALLBACK (PlotFunctionApply), (gpointer)psFunctyData);
	//psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "FunctionEdit"));
	//g_signal_connect (psWidget, "activate", G_CALLBACK (PlotFunctionApply), (gpointer)psFunctyData);
	//psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "RadiusEdit"));
	//g_signal_connect (psWidget, "activate", G_CALLBACK (PlotFunctionApply), (gpointer)psFunctyData);
	//psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "CurveXEdit"));
	//g_signal_connect (psWidget, "activate", G_CALLBACK (PlotFunctionApply), (gpointer)psFunctyData);
	//psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "CurveYEdit"));
	//g_signal_connect (psWidget, "activate", G_CALLBACK (PlotFunctionApply), (gpointer)psFunctyData);
	//psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "CurveZEdit"));
	//g_signal_connect (psWidget, "activate", G_CALLBACK (PlotFunctionApply), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "AccuracyCurve"));
	g_signal_connect (psWidget, "activate", G_CALLBACK (PlotFunctionApply), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "AccuracyRadiusCurve"));
	g_signal_connect (psWidget, "activate", G_CALLBACK (PlotFunctionApply), (gpointer)psFunctyData);
	//psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Red"));
	//g_signal_connect (psWidget, "activate", G_CALLBACK (PlotFunctionApply), (gpointer)psFunctyData);
	//psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Green"));
	//g_signal_connect (psWidget, "activate", G_CALLBACK (PlotFunctionApply), (gpointer)psFunctyData);
	//psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Blue"));
	//g_signal_connect (psWidget, "activate", G_CALLBACK (PlotFunctionApply), (gpointer)psFunctyData);
	//psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Alpha"));
	//g_signal_connect (psWidget, "activate", G_CALLBACK (PlotFunctionApply), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Accuracy"));
	g_signal_connect (psWidget, "activate", G_CALLBACK (PlotFunctionApply), (gpointer)psFunctyData);
	//psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "XCentre"));
	//g_signal_connect (psWidget, "activate", G_CALLBACK (PlotFunctionApply), (gpointer)psFunctyData);
	//psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "YCentre"));
	//g_signal_connect (psWidget, "activate", G_CALLBACK (PlotFunctionApply), (gpointer)psFunctyData);
	//psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "ZCentre"));
	//g_signal_connect (psWidget, "activate", G_CALLBACK (PlotFunctionApply), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TexFile"));
	g_signal_connect (psWidget, "activate", G_CALLBACK (PlotFunctionApply), (gpointer)psFunctyData);
	//psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TexXScale"));
	//g_signal_connect (psWidget, "activate", G_CALLBACK (PlotFunctionApply), (gpointer)psFunctyData);
	//psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TexYScale"));
	//g_signal_connect (psWidget, "activate", G_CALLBACK (PlotFunctionApply), (gpointer)psFunctyData);
	//psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TexXOffset"));
	//g_signal_connect (psWidget, "activate", G_CALLBACK (PlotFunctionApply), (gpointer)psFunctyData);
	//psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "TexYOffset"));
	//g_signal_connect (psWidget, "activate", G_CALLBACK (PlotFunctionApply), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MaterialFill"));
	g_signal_connect (psWidget, "activate", G_CALLBACK (PlotFunctionApply), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MaterialFill"));
	g_signal_connect (psWidget, "toggled", G_CALLBACK (ToggleMaterialFill), (gpointer)psFunctyData);



	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Spin"));
	g_signal_connect (psWidget, "toggled", G_CALLBACK (ToggleSpinPress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Invert"));
	g_signal_connect (psWidget, "toggled", G_CALLBACK (ToggleInvertPress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "DrawAxes"));
	g_signal_connect (psWidget, "toggled", G_CALLBACK (ToggleDrawAxesPress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Wireframe"));
	g_signal_connect (psWidget, "toggled", G_CALLBACK (ToggleWireframePress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "UseShaders"));
	g_signal_connect (psWidget, "toggled", G_CALLBACK (ToggleUseShadersPress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Shadow"));
	g_signal_connect (psWidget, "toggled", G_CALLBACK (ToggleShadowPress), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "FocusBlur"));
	g_signal_connect (psWidget, "toggled", G_CALLBACK (ToggleFocusBlurPress), (gpointer)psFunctyData);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "XMin"));
	g_signal_connect (psWidget, "key-release-event", G_CALLBACK (RangeMinChange), (gpointer)psFunctyData);
	g_signal_connect (psWidget, "activate", G_CALLBACK (RangeSetApply), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "YMin"));
	g_signal_connect (psWidget, "key-release-event", G_CALLBACK (RangeMinChange), (gpointer)psFunctyData);
	g_signal_connect (psWidget, "activate", G_CALLBACK (RangeSetApply), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "ZMin"));
	g_signal_connect (psWidget, "key-release-event", G_CALLBACK (RangeMinChange), (gpointer)psFunctyData);
	g_signal_connect (psWidget, "activate", G_CALLBACK (RangeSetApply), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "XWidth"));
	g_signal_connect (psWidget, "key-release-event", G_CALLBACK (RangeMinChange), (gpointer)psFunctyData);
	g_signal_connect (psWidget, "activate", G_CALLBACK (RangeSetApply), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "YWidth"));
	g_signal_connect (psWidget, "key-release-event", G_CALLBACK (RangeMinChange), (gpointer)psFunctyData);
	g_signal_connect (psWidget, "activate", G_CALLBACK (RangeSetApply), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "ZWidth"));
	g_signal_connect (psWidget, "key-release-event", G_CALLBACK (RangeMinChange), (gpointer)psFunctyData);
	g_signal_connect (psWidget, "activate", G_CALLBACK (RangeSetApply), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "XMax"));
	g_signal_connect (psWidget, "key-release-event", G_CALLBACK (RangeMaxChange), (gpointer)psFunctyData);
	g_signal_connect (psWidget, "activate", G_CALLBACK (RangeSetApply), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "YMax"));
	g_signal_connect (psWidget, "key-release-event", G_CALLBACK (RangeMaxChange), (gpointer)psFunctyData);
	g_signal_connect (psWidget, "activate", G_CALLBACK (RangeSetApply), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "ZMax"));
	g_signal_connect (psWidget, "key-release-event", G_CALLBACK (RangeMaxChange), (gpointer)psFunctyData);
	g_signal_connect (psWidget, "activate", G_CALLBACK (RangeSetApply), (gpointer)psFunctyData);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "Colour"));
	g_signal_connect (psWidget, "color-set", G_CALLBACK (SetColourButtonCallback), (gpointer)psFunctyData);

	// Attach the sliders
	szWidgetName = g_string_new ("");
	for (nSlider = 0; nSlider < CONTROLVARS_MAX; nSlider++) {
		g_string_printf (szWidgetName, "slider%d", nSlider);
		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, szWidgetName->str));

		psAdjustment = GTK_ADJUSTMENT ( gtk_adjustment_new (0.0, 0.0, 1.0, 0.1, 0.1, 0.0));
		gtk_range_set_adjustment (GTK_RANGE (psWidget), psAdjustment);
		
		g_signal_connect (psWidget, "value_changed", G_CALLBACK (ControlvarSliderChanged), (gpointer)psFunctyData);

		// Activate the value on enter
		g_string_printf (szWidgetName, "val%d", nSlider);
		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, szWidgetName->str));
		g_signal_connect (psWidget, "activate", G_CALLBACK (ControlVariablesApply), (gpointer)psFunctyData);

		g_string_printf (szWidgetName, "var%d", nSlider);
		psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, szWidgetName->str));
		g_signal_connect (psWidget, "activate", G_CALLBACK (ControlVariablesApply), (gpointer)psFunctyData);
	}
	g_string_free (szWidgetName, TRUE);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "FocusBlurNear"));
	g_signal_connect (psWidget, "value_changed", G_CALLBACK (FocusBlurNearSliderChanged), (gpointer)psFunctyData);
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "FocusBlurFar"));
	g_signal_connect (psWidget, "value_changed", G_CALLBACK (FocusBlurFarSliderChanged), (gpointer)psFunctyData);

	// Function list
	psFunctionListModel = gtk_list_store_new (FUNCSCOL_NUM, G_TYPE_POINTER, G_TYPE_STRING);
	PopulateFunctionList (psFunctionListModel, psFunctyData);

	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "FunctionList"));
	gtk_tree_view_set_model (GTK_TREE_VIEW (psWidget), GTK_TREE_MODEL (psFunctionListModel));

	psListCellRenderer = gtk_cell_renderer_text_new ();
	psListColumn = gtk_tree_view_column_new_with_attributes ("Function", psListCellRenderer, "text", FUNCSCOL_FUNCTION, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (psWidget), psListColumn);

	gtk_tree_view_column_set_cell_data_func (psListColumn, psListCellRenderer, FunctionCellDataFunc, NULL, NULL);

	// Use the following for double click
	//g_signal_connect (GTK_TREE_VIEW (psWidget), "row-activated", G_CALLBACK (EditFunctionRowDoubleClick), (gpointer)psFunctyData);

	// Use the following for single click
	psSelection = gtk_tree_view_get_selection (GTK_TREE_VIEW (psWidget));
	g_signal_connect (GTK_TREE_SELECTION (psSelection), "changed", G_CALLBACK (EditFunctionRowChanged), (gpointer)psFunctyData);


	// Display the window
	gtk_widget_show (psWindow);

	// Initialise visualisation
	Init (psVisData);

	// Load the configuration file if there is one
	ClearAllFunctions (psFunctyData);
	LoadFile (DEFAULT_SETTINGS_FILE, TRUE, psFunctyData);
	PopulateFunctionList (psFunctionListModel, psFunctyData);
	SynchroniseUI (psFunctyData);
	SetMainWindowTitle ("", psFunctyData);

	// Activate the idle update
	TimeoutAdd (psFunctyData);

	// Main loop
	gtk_main ();

	// Deactivate the idle update
	TimeoutRemove (psFunctyData);

	// Save out the configuration file
	SaveFile (DEFAULT_SETTINGS_FILE, TRUE, psFunctyData);

	Deinit (psVisData);

	// Delete persistent structures
	g_object_unref (G_OBJECT (psFunctyData->psXML));
	psFunctyData->psVisData = NULL;
	DeleteFunctyPersist (psFunctyData);

	return 0;
}

void SynchroniseUI (FunctyPersist * psFunctyData) {
	SetControlvarWindowState (psFunctyData);
	SetViewWindowState (psFunctyData);
	SetFunctionPropertiesWindow (FUNCTYPE_INVALID, FALSE, psFunctyData);
	SynchroniseButtonBarStyle (psFunctyData);
	SynchronisePanels (psFunctyData);
}

gchar * GetTitleFilename (char const * szFilename) {
	char * szDisplayName;

	szDisplayName = g_filename_display_basename (szFilename);

	return szDisplayName;
}

void SetMainWindowTitle (char const * szFilename, FunctyPersist * psFunctyData) {
	GtkWidget * psWidget;
	GString * szTitle;
	gchar * szDisplayName;

	// Figure out what to call the window
	szDisplayName = GetTitleFilename (szFilename);
	if ((strlen (szDisplayName) > 0) && (strlen (szFilename) > 0)) {
		szTitle = g_string_new ("");
		g_string_printf (szTitle, "%s - Functy", szDisplayName);
	}
	else {
		szTitle = g_string_new ("Functy");
	}

	// Set the window title
	psWidget = GTK_WIDGET (gtk_builder_get_object(psFunctyData->psXML, "MainWindow"));
	gtk_window_set_title (GTK_WINDOW (psWidget), szTitle->str);

	g_free (szDisplayName);
	g_string_free (szTitle, TRUE);
}

void SetExportBinary (bool boBinary, FunctyPersist * psFunctyData) {
	psFunctyData->boBinary = boBinary;
}

void SetExportMultiplier (double fExportMultiplier, FunctyPersist * psFunctyData) {
	psFunctyData->fExportMultiplier = fExportMultiplier;
}

void SetExportScaleFactor (double fExportScale, FunctyPersist * psFunctyData) {
	psFunctyData->fExportScale = fExportScale;
}

void SetExportScreen (bool boExportScreen, FunctyPersist * psFunctyData) {
	psFunctyData->boExportScreen = boExportScreen;
}

void SetExportAlpha (bool boExportAlpha, FunctyPersist * psFunctyData) {
	psFunctyData->boExportAlpha = boExportAlpha;
}

void SetExportTimeStart (double fExportTimeStart, FunctyPersist * psFunctyData) {
	psFunctyData->fExportTimeStart = fExportTimeStart;
}

void SetExportTimeEnd (double fExportTimeEnd, FunctyPersist * psFunctyData) {
	psFunctyData->fExportTimeEnd = fExportTimeEnd;
}

void SetExportFrames (int nExportFrames, FunctyPersist * psFunctyData) {
	psFunctyData->nExportFrames = nExportFrames;
}

void SetTextViewText (GtkWidget * psTextView, gchar const * const szText) {
	GtkTextBuffer * psTextBuffer;

	if (GTK_IS_TEXT_VIEW (psTextView)) {
		psTextBuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (psTextView));
		gtk_text_buffer_set_text (psTextBuffer, szText, -1);
	}
	else {
		gtk_entry_set_text (GTK_ENTRY (psTextView), szText);
	}
}

gchar * GetTextViewText (GtkWidget * psTextView) {
	GtkTextBuffer * psTextBuffer;
	GtkTextIter sIterStart;
	GtkTextIter sIterEnd;
	gchar * szText;
	gchar const * szBuffer;

	if (GTK_IS_TEXT_VIEW (psTextView)) {
		psTextBuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (psTextView));
		gtk_text_buffer_get_start_iter (psTextBuffer, & sIterStart);
		gtk_text_buffer_get_end_iter (psTextBuffer, & sIterEnd);
		szText = gtk_text_buffer_get_text (psTextBuffer, & sIterStart, & sIterEnd, FALSE);
	}
	else {
		szBuffer = gtk_entry_get_text (GTK_ENTRY (psTextView));
		szText = g_strdup (szBuffer);
	}

	return szText;
}




