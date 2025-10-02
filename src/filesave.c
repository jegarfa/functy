///////////////////////////////////////////////////////////////////
// Functy FileSave.c
// Load/Save Functy function information
//
// David Llewellyn-Jones
// http://www.flypig.co.uk
//
// December 2012
///////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
// Includes

#include <string.h>
#include <locale.h>

#include "filesave.h"
#include "spherical.h"
#include "curve.h"
#include "controlvar.h"

///////////////////////////////////////////////////////////////////
// Defines

#define FILE_BUFFER_SIZE (1024)
#define MAX_LEVELS (4)

///////////////////////////////////////////////////////////////////
// Structures and enumerations

typedef enum {
	FILESAVETAG_INVALID = -1,

	FILESAVETAG_SETTINGS,
	FILESAVETAG_RANGE,
	FILESAVETAG_FUNCTION,
	FILESAVETAG_CONTROLVAR,

	FILESAVETAG_SHOWAXES,
	FILESAVETAG_SPIN,
	FILESAVETAG_INVERT,
	FILESAVETAG_WIREFRAME,
	FILESAVETAG_FULLSCREEN,
	FILESAVETAG_BUTTONBARSTYLE,
	FILESAVETAG_SHOWPANELLEFT,
	FILESAVETAG_SHOWPANELBOTTOM,
	FILESAVETAG_EXPORTBINARY,
	FILESAVETAG_EXPORTALPHA,
	FILESAVETAG_EXPORTMULTIPLIER,
	FILESAVETAG_EXPORTSCALE,
	FILESAVETAG_EXPORTSCREEN,
	FILESAVETAG_EXPORTTIMESTART,
	FILESAVETAG_EXPORTTIMEEND,
	FILESAVETAG_EXPORTFRAMES,
	FILESAVETAG_USESHADERS,
	FILESAVETAG_SHADOWS,
	FILESAVETAG_FOCUSBLUR,
	FILESAVETAG_FOCUSBLURNEAR,
	FILESAVETAG_FOCUSBLURFAR,

	FILESAVETAG_RADIUS,
	FILESAVETAG_ROTATION,
	FILESAVETAG_ELEVATION,

	FILESAVETAG_MIN,
	FILESAVETAG_WIDTH,
	FILESAVETAG_COORD,

	FILESAVETAG_NAME,
	FILESAVETAG_EQUATION,
	FILESAVETAG_RED,
	FILESAVETAG_GREEN,
	FILESAVETAG_BLUE,
	FILESAVETAG_ALPHA,
	FILESAVETAG_ACCURACY,
	FILESAVETAG_MATERIALTHICKNESS,
	FILESAVETAG_MATERIALFILL,

	FILESAVETAG_TEXFILE,
	FILESAVETAG_TEXXSCALE,
	FILESAVETAG_TEXYSCALE,
	FILESAVETAG_TEXXOFFSET,
	FILESAVETAG_TEXYOFFSET,

	FILESAVETAG_CENTREX,
	FILESAVETAG_CENTREY,
	FILESAVETAG_CENTREZ,

	FILESAVETAG_EQUATIONX,
	FILESAVETAG_EQUATIONY,
	FILESAVETAG_EQUATIONZ,
	FILESAVETAG_ACCURACYRADIUS,

	FILESAVETAG_VARNAME,
	FILESAVETAG_VARVALUE,

	FILESAVETAG_OTHER,

	FILESAVETAG_NUM
} FILESAVETAG;

typedef struct _FilesavePersist {
	FunctyPersist * psFunctyData;
	FILESAVETAG eTag[MAX_LEVELS];
	int nLevel;
	GString * szText;
	FuncPersist * psFuncData;
	bool boConfigure;
	ControlvarPersist * psControlvarData;
} FilesavePersist;

struct _LocaleRestore {
	GString * szLocale;
};

///////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////
// Function prototypes

FilesavePersist * NewFilesavePersist (FunctyPersist * psFunctyData);
void DeleteFilesavePersist (FilesavePersist * psFilesaveData);
void TextFuncty (GMarkupParseContext * pContext, gchar const * szText, gsize nTextLen, gpointer psData, GError * * ppsError);
void EndElementFuncty (GMarkupParseContext * psContext, gchar const * szElementName, gpointer psData, GError * * ppsError);
void StartElementFuncty (GMarkupParseContext * psContext, gchar const * szElementName, gchar const * * aszAttributeNames, gchar const * * aszAttributeValues, gpointer psData, GError * * ppsError);
void DestroyUserDataFuncty (gpointer psData);

void StartElementSettings (FILESAVETAG eTag, gchar const * * aszAttributeNames, gchar const * * aszAttributeValues, FilesavePersist * psFilesaveData);
void StartElementRange (FILESAVETAG eTag, gchar const * * aszAttributeNames, gchar const * * aszAttributeValues, FilesavePersist * psFilesaveData);
void StartElementFunction (FILESAVETAG eTag, gchar const * * aszAttributeNames, gchar const * * aszAttributeValues, FilesavePersist * psFilesaveData);
void EndElementFunction (FILESAVETAG eTag, FilesavePersist * psFilesaveData);
void StartElementControlvar (FILESAVETAG eTag, gchar const * * aszAttributeNames, gchar const * * aszAttributeValues, FilesavePersist * psFilesaveData);
void EndElementControlvar (FILESAVETAG eTag, FilesavePersist * psFilesaveData);

///////////////////////////////////////////////////////////////////
// Function definitions

FilesavePersist * NewFilesavePersist (FunctyPersist * psFunctyData) {
	FilesavePersist * psFilesaveData = NULL;
	int nLevel;

	psFilesaveData = g_new0 (FilesavePersist, 1);
	psFilesaveData->psFunctyData = psFunctyData;
	for (nLevel = 0; nLevel < MAX_LEVELS; nLevel++) {
		psFilesaveData->eTag[nLevel] = FILESAVETAG_INVALID;
	}
	psFilesaveData->nLevel = -1;
	psFilesaveData->szText = g_string_new ("");
	psFilesaveData->psFuncData = NULL;
	psFilesaveData->boConfigure = FALSE;
	psFilesaveData->psControlvarData = NULL;

	return psFilesaveData;
}

void DeleteFilesavePersist (FilesavePersist * psFilesaveData) {
	if (psFilesaveData->psControlvarData) {
		DeleteControlvarPersist (psFilesaveData->psControlvarData);
		psFilesaveData->psControlvarData = NULL;
	}

	g_string_free (psFilesaveData->szText, TRUE);
	g_free (psFilesaveData);
}

bool SaveFile (char const * szFilename, bool boConfigure, FunctyPersist * psFunctyData) {
	gboolean boSuccess = TRUE;
	FILE * fhFile;
	GSList * psFuncList;

	bool boValue;
	double fValue;
	double afRange[6];
	char const * szValue;
	float fRadius;
	float fRotation;
	float fElevation;
	gchar * szEscaped;
	FUNCTYPE eFuncType;
	LocaleRestore * psRestore;
	FnControlPersist * psFnControlData;
	GSList * psControlvarList;

	psRestore = ClearLocale ();

	fhFile = fopen (szFilename, "wb");
	if (fhFile) {
		// Preamble
		fprintf(fhFile, "<?xml version='1.0' encoding='UTF-8' ?>\n\n");
		fprintf(fhFile, "<functy>\n");

		// System info
		fprintf(fhFile, "\t<settings>\n");
		boValue = GetDrawAxes (psFunctyData->psVisData);
		fprintf (fhFile, "\t\t<show-axes bool=\"%d\"/>\n", boValue);
		boValue = GetClearWhite (psFunctyData->psVisData);
		fprintf (fhFile, "\t\t<invert bool=\"%d\"/>\n", boValue);
		boValue = GetWireframe (psFunctyData->psVisData);
		fprintf (fhFile, "\t\t<wireframe bool=\"%d\"/>\n", boValue);

		if (boConfigure) {
			boValue = GetSpin (psFunctyData->psVisData);
			fprintf(fhFile, "\t\t<spin bool=\"%d\"/>\n", boValue);
			boValue = GetFullScreen (psFunctyData->psVisData);
			fprintf(fhFile, "\t\t<fullscreen bool=\"%d\"/>\n", boValue);
			fprintf(fhFile, "\t\t<buttonbarstyle int=\"%d\"/>\n", psFunctyData->eButtonBarStyle);
			fprintf(fhFile, "\t\t<showpanelleft bool=\"%d\"/>\n", psFunctyData->boShowPanelLeft);
			fprintf(fhFile, "\t\t<showpanelbottom bool=\"%d\"/>\n", psFunctyData->boShowPanelBottom);

			GetView (& fRadius, & fRotation, & fElevation, psFunctyData->psVisData);
			fprintf (fhFile, "\t\t<radius double=\"%f\"/>\n", fRadius);
			fprintf (fhFile, "\t\t<rotation double=\"%f\"/>\n", fRotation);
			fprintf (fhFile, "\t\t<elevation double=\"%f\"/>\n", fElevation);
			fprintf (fhFile, "\t\t<exportbinary bool=\"%d\"/>\n", psFunctyData->boBinary);
			fprintf (fhFile, "\t\t<exportmultiplier double=\"%f\"/>\n", psFunctyData->fExportMultiplier);
			fprintf (fhFile, "\t\t<exportscale double=\"%f\"/>\n", psFunctyData->fExportScale);
			fprintf (fhFile, "\t\t<exportscreen bool=\"%d\"/>\n", psFunctyData->boExportScreen);
			fprintf (fhFile, "\t\t<exportalpha bool=\"%d\"/>\n", psFunctyData->boExportAlpha);
			fprintf (fhFile, "\t\t<exporttimestart double=\"%f\"/>\n", psFunctyData->fExportTimeStart);
			fprintf (fhFile, "\t\t<exporttimeend double=\"%f\"/>\n", psFunctyData->fExportTimeEnd);
			fprintf (fhFile, "\t\t<exportframes int=\"%d\"/>\n", psFunctyData->nExportFrames);

			boValue = GetShadersActive (psFunctyData->psVisData);
			fprintf (fhFile, "\t\t<useshaders bool=\"%d\"/>\n", boValue);
			boValue = GetShadow (psFunctyData->psVisData);
			fprintf (fhFile, "\t\t<shadows bool=\"%d\"/>\n", boValue);
			boValue = GetFocusBlur (psFunctyData->psVisData);
			fprintf (fhFile, "\t\t<focusblur bool=\"%d\"/>\n", boValue);
			fValue = GetFocusBlurNear (psFunctyData->psVisData);
			fprintf(fhFile, "\t\t<focusblurnear double=\"%f\"/>\n", fValue);
			fValue = GetFocusBlurFar (psFunctyData->psVisData);
			fprintf(fhFile, "\t\t<focusblurfar double=\"%f\"/>\n", fValue);
		}
		fprintf(fhFile, "\t</settings>\n");

		// Range
		fprintf(fhFile, "\t<range>\n");
		GetVisRange (afRange, psFunctyData->psVisData);

		fprintf (fhFile, "\t\t<min>\n");
		fprintf (fhFile, "\t\t\t<coord x=\"%f\" y=\"%f\" z=\"%f\"/>\n", afRange[0], afRange[1], afRange[2]);
		fprintf (fhFile, "\t\t</min>\n");
		fprintf (fhFile, "\t\t<width>\n");
		fprintf (fhFile, "\t\t\t<coord x=\"%f\" y=\"%f\" z=\"%f\"/>\n", afRange[3], afRange[4], afRange[5]);
		fprintf (fhFile, "\t\t</width>\n");
		fprintf (fhFile, "\t</range>\n");

		// Control variables
		psFnControlData = GetControlVarList (psFunctyData->psVisData);
		if (psFnControlData) {
			psControlvarList = psFnControlData->psControlvarList;
			while (psControlvarList) {
				fprintf(fhFile, "\t<controlvar>\n");
				szValue = GetControlvarName ((ControlvarPersist *)psControlvarList->data);
				szEscaped = g_markup_printf_escaped ("\t\t<varname>%s</varname>\n", szValue);
				fprintf(fhFile, "%s", szEscaped);
				g_free (szEscaped);

				szValue = GetControlvarValueString ((ControlvarPersist *)psControlvarList->data);
				szEscaped = g_markup_printf_escaped ("\t\t<varvalue>%s</varvalue>\n", szValue);
				fprintf(fhFile, "%s", szEscaped);
				g_free (szEscaped);

				fprintf (fhFile, "\t</controlvar>\n");
			
				psControlvarList = g_slist_next (psControlvarList);
			}
		}

		// Functions
		psFuncList = GetFunctionList (psFunctyData->psVisData);
		while (psFuncList) {
			eFuncType = GetFunctionType ((FuncPersist *)(psFuncList->data));
			switch (eFuncType) {
			case FUNCTYPE_CARTESIAN:
				fprintf(fhFile, "\t<function type=\"cartesian\">\n");
				break;
			case FUNCTYPE_SPHERICAL:
				fprintf(fhFile, "\t<function type=\"spherical\">\n");
				break;
			case FUNCTYPE_CURVE:
				fprintf(fhFile, "\t<function type=\"curve\">\n");
				break;
			default:
				fprintf(fhFile, "\t<function>\n");
				break;
			}

			szValue = GetFunctionName ((FuncPersist *)(psFuncList->data));
			szEscaped = g_markup_printf_escaped ("\t\t<name>%s</name>\n", szValue);
			fprintf(fhFile, "%s", szEscaped);
			g_free (szEscaped);

			szValue = GetFunctionString ((FuncPersist *)(psFuncList->data));
			szEscaped = g_markup_printf_escaped ("\t\t<equation>%s</equation>\n", szValue);
			fprintf(fhFile, "%s", szEscaped);
			g_free (szEscaped);

			szValue = GetRedString ((FuncPersist *)(psFuncList->data));
			szEscaped = g_markup_printf_escaped ("\t\t<red>%s</red>\n", szValue);
			fprintf(fhFile, "%s", szEscaped);
			g_free (szEscaped);

			szValue = GetGreenString ((FuncPersist *)(psFuncList->data));
			szEscaped = g_markup_printf_escaped ("\t\t<green>%s</green>\n", szValue);
			fprintf(fhFile, "%s", szEscaped);
			g_free (szEscaped);

			szValue = GetBlueString ((FuncPersist *)(psFuncList->data));
			szEscaped = g_markup_printf_escaped ("\t\t<blue>%s</blue>\n", szValue);
			fprintf(fhFile, "%s", szEscaped);
			g_free (szEscaped);

			szValue = GetAlphaString ((FuncPersist *)(psFuncList->data));
			szEscaped = g_markup_printf_escaped ("\t\t<alpha>%s</alpha>\n", szValue);
			fprintf(fhFile, "%s", szEscaped);
			g_free (szEscaped);

			fValue = GetFunctionAccuracy ((FuncPersist *)(psFuncList->data));
			fprintf(fhFile, "\t\t<accuracy double=\"%f\"/>\n", fValue);

			fValue = GetFunctionMaterialThickness ((FuncPersist *)(psFuncList->data));
			fprintf(fhFile, "\t\t<material-thickness double=\"%f\"/>\n", fValue);

			boValue = GetFunctionMaterialFill ((FuncPersist *)(psFuncList->data));
			fprintf (fhFile, "\t\t<material-fill bool=\"%d\"/>\n", boValue);

			switch (eFuncType) {
			case FUNCTYPE_SPHERICAL:
				szValue = SphericalGetXCentreString ((FuncPersist *)(psFuncList->data));
				szEscaped = g_markup_printf_escaped ("\t\t<centre-x>%s</centre-x>\n", szValue);
				fprintf(fhFile, "%s", szEscaped);
				g_free (szEscaped);

				szValue = SphericalGetYCentreString ((FuncPersist *)(psFuncList->data));
				szEscaped = g_markup_printf_escaped ("\t\t<centre-y>%s</centre-y>\n", szValue);
				fprintf(fhFile, "%s", szEscaped);
				g_free (szEscaped);

				szValue = SphericalGetZCentreString ((FuncPersist *)(psFuncList->data));
				szEscaped = g_markup_printf_escaped ("\t\t<centre-z>%s</centre-z>\n", szValue);
				fprintf(fhFile, "%s", szEscaped);
				g_free (szEscaped);
				break;
			case FUNCTYPE_CURVE:
				fValue = CurveGetFunctionAccuracyRadius ((FuncPersist *)(psFuncList->data));
				fprintf(fhFile, "\t\t<accuracy-radius double=\"%f\"/>\n", fValue);

				szValue = CurveGetXFunctionString ((FuncPersist *)(psFuncList->data));
				szEscaped = g_markup_printf_escaped ("\t\t<equation-x>%s</equation-x>\n", szValue);
				fprintf(fhFile, "%s", szEscaped);
				g_free (szEscaped);

				szValue = CurveGetYFunctionString ((FuncPersist *)(psFuncList->data));
				szEscaped = g_markup_printf_escaped ("\t\t<equation-y>%s</equation-y>\n", szValue);
				fprintf(fhFile, "%s", szEscaped);
				g_free (szEscaped);

				szValue = CurveGetZFunctionString ((FuncPersist *)(psFuncList->data));
				szEscaped = g_markup_printf_escaped ("\t\t<equation-z>%s</equation-z>\n", szValue);
				fprintf(fhFile, "%s", szEscaped);
				g_free (szEscaped);

				szValue = CurveGetXCentreString ((FuncPersist *)(psFuncList->data));
				szEscaped = g_markup_printf_escaped ("\t\t<centre-x>%s</centre-x>\n", szValue);
				fprintf(fhFile, "%s", szEscaped);
				g_free (szEscaped);

				szValue = CurveGetYCentreString ((FuncPersist *)(psFuncList->data));
				szEscaped = g_markup_printf_escaped ("\t\t<centre-y>%s</centre-y>\n", szValue);
				fprintf(fhFile, "%s", szEscaped);
				g_free (szEscaped);

				szValue = CurveGetZCentreString ((FuncPersist *)(psFuncList->data));
				szEscaped = g_markup_printf_escaped ("\t\t<centre-z>%s</centre-z>\n", szValue);
				fprintf(fhFile, "%s", szEscaped);
				g_free (szEscaped);
				break;
			default:
				// Do nothing
				break;
			}

			szValue = GetTexFileString ((FuncPersist *)(psFuncList->data));
			szEscaped = g_markup_printf_escaped ("\t\t<texture-file>%s</texture-file>\n", szValue);
			fprintf(fhFile, "%s", szEscaped);
			g_free (szEscaped);

			szValue = GetTexXScaleString ((FuncPersist *)(psFuncList->data));
			szEscaped = g_markup_printf_escaped ("\t\t<texture-x-scale>%s</texture-x-scale>\n", szValue);
			fprintf(fhFile, "%s", szEscaped);
			g_free (szEscaped);

			szValue = GetTexYScaleString ((FuncPersist *)(psFuncList->data));
			szEscaped = g_markup_printf_escaped ("\t\t<texture-y-scale>%s</texture-y-scale>\n", szValue);
			fprintf(fhFile, "%s", szEscaped);
			g_free (szEscaped);

			szValue = GetTexXOffsetString ((FuncPersist *)(psFuncList->data));
			szEscaped = g_markup_printf_escaped ("\t\t<texture-x-offset>%s</texture-x-offset>\n", szValue);
			fprintf(fhFile, "%s", szEscaped);
			g_free (szEscaped);

			szValue = GetTexYOffsetString ((FuncPersist *)(psFuncList->data));
			szEscaped = g_markup_printf_escaped ("\t\t<texture-y-offset>%s</texture-y-offset>\n", szValue);
			fprintf(fhFile, "%s", szEscaped);
			g_free (szEscaped);

			fprintf(fhFile, "\t</function>\n");
			psFuncList = g_slist_next (psFuncList);
		}

		// Conclusion
		fprintf(fhFile, "</functy>\n\n");
		fclose (fhFile);
		//printf ("Saved file\n");
	}
	else {
		boSuccess = FALSE;
	}

	RestoreLocale (psRestore);

	return boSuccess;
}

bool LoadFile (char const * szFilename, bool boConfigure, FunctyPersist * psFunctyData) {
	gboolean boSuccess = FALSE;
	FilesavePersist * psFilesaveData = NULL;
	GMarkupParseContext * psParseContext;
	GMarkupParser * psParser;
	FILE * fhFile;
	gchar szFileBuffer[FILE_BUFFER_SIZE];
	int nRead;
	LocaleRestore * psRestore;

	psRestore = ClearLocale ();

	psFilesaveData = NewFilesavePersist (psFunctyData);
	psFilesaveData->boConfigure = boConfigure;

	psParser = g_new0 (GMarkupParser, 1);
	psParser->start_element = StartElementFuncty;
	psParser->end_element = EndElementFuncty;
	psParser->text = TextFuncty;
	psParser->passthrough = NULL;
	psParser->error = NULL;

	fhFile = fopen (szFilename, "rb");
	if (fhFile) {
		boSuccess = TRUE;
		psParseContext = g_markup_parse_context_new (psParser, 0, (gpointer)psFilesaveData, DestroyUserDataFuncty);

		nRead = fread (szFileBuffer, sizeof (char), FILE_BUFFER_SIZE, fhFile);
		while (boSuccess && (nRead > 0)) {
			boSuccess = g_markup_parse_context_parse (psParseContext, szFileBuffer, nRead, NULL);
			nRead = fread (szFileBuffer, sizeof (char), FILE_BUFFER_SIZE, fhFile);
		}

		if (boSuccess) {
			g_markup_parse_context_end_parse (psParseContext, NULL);
			//printf ("Loaded file\n");
		}
		g_markup_parse_context_free (psParseContext);
		fclose (fhFile);
	}
	g_free (psParser);
	// The context is destroyed automatically by the parser
	//DeleteSettingsPersist (psSettingsData);

	RestoreLocale (psRestore);

	return boSuccess;
}

void StartElementFuncty (GMarkupParseContext * psContext, gchar const * szElementName, gchar const * * aszAttributeNames, gchar const * * aszAttributeValues, gpointer psData, GError * * ppsError) {
	FilesavePersist * psFilesaveData = (FilesavePersist *)psData;

	psFilesaveData->nLevel++;

	g_string_set_size (psFilesaveData->szText, 0);

	if ((psFilesaveData->nLevel >= 0) && (psFilesaveData->nLevel < MAX_LEVELS)) {
		if (strcmp (szElementName, "functy") == 0) {
			psFilesaveData->nLevel = -1;
			psFilesaveData->eTag[0] = FILESAVETAG_INVALID;
		}
		else if (strcmp (szElementName, "settings") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_SETTINGS;
		}
		else if (strcmp (szElementName, "range") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_RANGE;
		}
		else if (strcmp (szElementName, "function") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_FUNCTION;
		}
		else if (strcmp (szElementName, "controlvar") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_CONTROLVAR;
		}
		else if (strcmp (szElementName, "min") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_MIN;
		}
		else if (strcmp (szElementName, "width") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_WIDTH;
		}
		else if (strcmp (szElementName, "show-axes") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_SHOWAXES;
		}
		else if (strcmp (szElementName, "spin") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_SPIN;
		}
		else if (strcmp (szElementName, "invert") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_INVERT;
		}
		else if (strcmp (szElementName, "wireframe") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_WIREFRAME;
		}
		else if (strcmp (szElementName, "fullscreen") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_FULLSCREEN;
		}
		else if (strcmp (szElementName, "buttonbarstyle") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_BUTTONBARSTYLE;
		}
		else if (strcmp (szElementName, "showpanelleft") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_SHOWPANELLEFT;
		}
		else if (strcmp (szElementName, "showpanelbottom") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_SHOWPANELBOTTOM;
		}
		else if (strcmp (szElementName, "useshaders") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_USESHADERS;
		}
		else if (strcmp (szElementName, "shadows") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_SHADOWS;
		}
		else if (strcmp (szElementName, "focusblur") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_FOCUSBLUR;
		}
		else if (strcmp (szElementName, "focusblurnear") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_FOCUSBLURNEAR;
		}
		else if (strcmp (szElementName, "focusblurfar") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_FOCUSBLURFAR;
		}
		else if (strcmp (szElementName, "radius") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_RADIUS;
		}
		else if (strcmp (szElementName, "rotation") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_ROTATION;
		}
		else if (strcmp (szElementName, "elevation") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_ELEVATION;
		}
		else if (strcmp (szElementName, "exportbinary") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_EXPORTBINARY;
		}
		else if (strcmp (szElementName, "exportmultiplier") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_EXPORTMULTIPLIER;
		}
		else if (strcmp (szElementName, "exportscale") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_EXPORTSCALE;
		}
		else if (strcmp (szElementName, "exportscreen") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_EXPORTSCREEN;
		}
		else if (strcmp (szElementName, "exporttimestart") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_EXPORTTIMESTART;
		}
		else if (strcmp (szElementName, "exporttimeend") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_EXPORTTIMEEND;
		}
		else if (strcmp (szElementName, "exportframes") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_EXPORTFRAMES;
		}
		else if (strcmp (szElementName, "name") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_NAME;
		}
		else if (strcmp (szElementName, "equation") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_EQUATION;
		}
		else if (strcmp (szElementName, "red") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_RED;
		}
		else if (strcmp (szElementName, "green") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_GREEN;
		}
		else if (strcmp (szElementName, "blue") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_BLUE;
		}
		else if (strcmp (szElementName, "alpha") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_ALPHA;
		}
		else if (strcmp (szElementName, "accuracy") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_ACCURACY;
		}
		else if (strcmp (szElementName, "material-thickness") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_MATERIALTHICKNESS;
		}
		else if (strcmp (szElementName, "material-fill") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_MATERIALFILL;
		}
		else if (strcmp (szElementName, "centre-x") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_CENTREX;
		}
		else if (strcmp (szElementName, "centre-y") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_CENTREY;
		}
		else if (strcmp (szElementName, "centre-z") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_CENTREZ;
		}
		else if (strcmp (szElementName, "coord") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_COORD;
		}
		else if (strcmp (szElementName, "accuracy-radius") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_ACCURACYRADIUS;
		}
		else if (strcmp (szElementName, "equation-x") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_EQUATIONX;
		}
		else if (strcmp (szElementName, "equation-y") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_EQUATIONY;
		}
		else if (strcmp (szElementName, "equation-z") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_EQUATIONZ;
		}
		else if (strcmp (szElementName, "texture-file") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_TEXFILE;
		}
		else if (strcmp (szElementName, "texture-x-scale") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_TEXXSCALE;
		}
		else if (strcmp (szElementName, "texture-y-scale") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_TEXYSCALE;
		}
		else if (strcmp (szElementName, "texture-x-offset") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_TEXXOFFSET;
		}
		else if (strcmp (szElementName, "texture-y-offset") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_TEXYOFFSET;
		}
		else if (strcmp (szElementName, "varname") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_VARNAME;
		}
		else if (strcmp (szElementName, "varvalue") == 0) {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_VARVALUE;
		}
		else {
			psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_OTHER;
		}
	}

	switch (psFilesaveData->eTag[0]) {
	case FILESAVETAG_SETTINGS:
		StartElementSettings (psFilesaveData->eTag[1], aszAttributeNames, aszAttributeValues, psFilesaveData);
		break;
	case FILESAVETAG_RANGE:
		if (psFilesaveData->eTag[2] == FILESAVETAG_COORD) {
			StartElementRange (psFilesaveData->eTag[1], aszAttributeNames, aszAttributeValues, psFilesaveData);
		}
		break;
	case FILESAVETAG_FUNCTION:
		StartElementFunction (psFilesaveData->eTag[1], aszAttributeNames, aszAttributeValues, psFilesaveData);
		break;
	case FILESAVETAG_CONTROLVAR:
		StartElementControlvar (psFilesaveData->eTag[1], aszAttributeNames, aszAttributeValues, psFilesaveData);
		break;
	default:
		// Do nothing
		break;
	}
}

void EndElementFuncty (GMarkupParseContext * psContext, gchar const * szElementName, gpointer psData, GError * * ppsError) {
	FilesavePersist * psFilesaveData = (FilesavePersist *)psData;
	bool boShaderActive;

	if ((psFilesaveData->nLevel >= 0) && (psFilesaveData->nLevel < MAX_LEVELS)) {
		switch (psFilesaveData->eTag[0]) {
		case FILESAVETAG_FUNCTION:
			if (psFilesaveData->nLevel == 0) {
				TransferFunctionRange (psFilesaveData->psFuncData, psFilesaveData->psFunctyData->psVisData);
				boShaderActive = GetShadersActive (psFilesaveData->psFunctyData->psVisData);
				SetFunctionShaderActive (boShaderActive, psFilesaveData->psFuncData);
				FunctionShadersRegenerate (psFilesaveData->psFuncData);
			}
			if (psFilesaveData->nLevel == 1) {
				EndElementFunction (psFilesaveData->eTag[1], psFilesaveData);
			}
			break;
		case FILESAVETAG_CONTROLVAR:
			if ((psFilesaveData->nLevel >= 0) && (psFilesaveData->nLevel <= 1)) {
				EndElementControlvar (psFilesaveData->eTag[1], psFilesaveData);
			}
			break;
		default:
			// Do nothing
			break;
		}

		psFilesaveData->eTag[psFilesaveData->nLevel] = FILESAVETAG_INVALID;
	}
	psFilesaveData->nLevel--;

	g_string_set_size (psFilesaveData->szText, 0);
}

void TextFuncty (GMarkupParseContext * pContext, gchar const * szText, gsize nTextLen, gpointer psData, GError * * ppsError) {
	FilesavePersist * psFilesaveData = (FilesavePersist *)psData;

	g_string_append_len (psFilesaveData->szText, szText, nTextLen);
}

void DestroyUserDataFuncty (gpointer psData) {
	FilesavePersist * psFilesaveData = (FilesavePersist *)psData;
	DeleteFilesavePersist (psFilesaveData);
}

void StartElementSettings (FILESAVETAG eTag, gchar const * * aszAttributeNames, gchar const * * aszAttributeValues, FilesavePersist * psFilesaveData) {
	bool boBoolFound;
	bool boDoubleFound;
	bool boIntFound;
	bool boValue = FALSE;
	double fValue = 0.0f;
	int nValue = 0.0f;
	int nAttribute;
	float fRadius;
	float fRotation;
	float fElevation;

	boBoolFound = FALSE;
	boDoubleFound = FALSE;
	boIntFound = FALSE;
	nAttribute = 0;
	while (aszAttributeNames[nAttribute] != NULL) {
		if (strcmp (aszAttributeNames[nAttribute], "bool") == 0) {
			boValue = atoi (aszAttributeValues[nAttribute]);
			boBoolFound = TRUE;
		}
		if (strcmp (aszAttributeNames[nAttribute], "double") == 0) {
			fValue = atof (aszAttributeValues[nAttribute]);
			boDoubleFound = TRUE;
		}
		if (strcmp (aszAttributeNames[nAttribute], "int") == 0) {
			nValue = atoi (aszAttributeValues[nAttribute]);
			boIntFound = TRUE;
		}
		nAttribute++;
	}

	if (boBoolFound) {
		switch (eTag) {
		case FILESAVETAG_SHOWAXES:
			SetDrawAxes (boValue, psFilesaveData->psFunctyData->psVisData);
			break;
		case FILESAVETAG_SPIN:
			if (psFilesaveData->boConfigure) {
				SetSpin (boValue, psFilesaveData->psFunctyData->psVisData);
			}
			break;
		case FILESAVETAG_INVERT:
			SetClearWhite (boValue, psFilesaveData->psFunctyData->psVisData);
			break;
		case FILESAVETAG_WIREFRAME:
			SetWireframe (boValue, psFilesaveData->psFunctyData->psVisData);
			break;
		case FILESAVETAG_FULLSCREEN:
			if (psFilesaveData->boConfigure) {
				SetFullScreenWindow (boValue, psFilesaveData->psFunctyData);
			}
			break;
		case FILESAVETAG_SHOWPANELLEFT:
			if (psFilesaveData->boConfigure) {
				psFilesaveData->psFunctyData->boShowPanelLeft = boValue;
			}
			break;
		case FILESAVETAG_SHOWPANELBOTTOM:
			if (psFilesaveData->boConfigure) {
				psFilesaveData->psFunctyData->boShowPanelBottom = boValue;
			}
			break;
		case FILESAVETAG_USESHADERS:
			if (psFilesaveData->boConfigure) {
				SetShadersActive (boValue, psFilesaveData->psFunctyData->psVisData);
			}
			break;
		case FILESAVETAG_SHADOWS:
			if (psFilesaveData->boConfigure) {
				SetShadow (boValue, psFilesaveData->psFunctyData->psVisData);
			}
			break;
		case FILESAVETAG_FOCUSBLUR:
			if (psFilesaveData->boConfigure) {
				SetFocusBlur (boValue, psFilesaveData->psFunctyData->psVisData);
			}
			break;
		case FILESAVETAG_EXPORTBINARY:
			if (psFilesaveData->boConfigure) {
				SetExportBinary (boValue, psFilesaveData->psFunctyData);
			}
			break;
		case FILESAVETAG_EXPORTSCREEN:
			if (psFilesaveData->boConfigure) {
				SetExportScreen (boValue, psFilesaveData->psFunctyData);
			}
			break;
		case FILESAVETAG_EXPORTALPHA:
			if (psFilesaveData->boConfigure) {
				SetExportAlpha (boValue, psFilesaveData->psFunctyData);
			}
			break;
		default:
			// Do nothing
			break;
		}
	}

	if (boDoubleFound) {
		GetView (& fRadius, & fRotation, & fElevation, psFilesaveData->psFunctyData->psVisData);
		switch (eTag) {
		case FILESAVETAG_RADIUS:
			if (psFilesaveData->boConfigure) {
				SetView (fValue, fRotation, fElevation, psFilesaveData->psFunctyData->psVisData);
			}
			break;
		case FILESAVETAG_ROTATION:
			if (psFilesaveData->boConfigure) {
				SetView (fRadius, fValue, fElevation, psFilesaveData->psFunctyData->psVisData);
			}
			break;
		case FILESAVETAG_ELEVATION:
			if (psFilesaveData->boConfigure) {
				SetView (fRadius, fRotation, fValue, psFilesaveData->psFunctyData->psVisData);
			}
			break;
		case FILESAVETAG_EXPORTMULTIPLIER:
			if (psFilesaveData->boConfigure) {
				SetExportMultiplier (fValue, psFilesaveData->psFunctyData);
			}
			break;
		case FILESAVETAG_EXPORTSCALE:
			if (psFilesaveData->boConfigure) {
				SetExportScaleFactor (fValue, psFilesaveData->psFunctyData);
			}
			break;
		case FILESAVETAG_EXPORTTIMESTART:
			if (psFilesaveData->boConfigure) {
				SetExportTimeStart (fValue, psFilesaveData->psFunctyData);
			}
			break;
		case FILESAVETAG_EXPORTTIMEEND:
			if (psFilesaveData->boConfigure) {
				SetExportTimeEnd (fValue, psFilesaveData->psFunctyData);
			}
			break;
		case FILESAVETAG_FOCUSBLURNEAR:
			if (psFilesaveData->boConfigure) {
				SetFocusBlurNear (fValue, psFilesaveData->psFunctyData->psVisData);
			}
			break;
		case FILESAVETAG_FOCUSBLURFAR:
			if (psFilesaveData->boConfigure) {
				SetFocusBlurFar (fValue, psFilesaveData->psFunctyData->psVisData);
			}
			break;
		default:
			// Do nothing
			break;
		}
	}

	if (boIntFound) {
		switch (eTag) {
		case FILESAVETAG_EXPORTFRAMES:
			if (psFilesaveData->boConfigure) {
				SetExportFrames (nValue, psFilesaveData->psFunctyData);
			}
			break;
		case FILESAVETAG_BUTTONBARSTYLE:
			if (psFilesaveData->boConfigure) {
				psFilesaveData->psFunctyData->eButtonBarStyle = ((BUTTONBARSTYLE)nValue);
			}
			break;
		default:
			// Do nothing
			break;
		}
	}
}

void StartElementRange (FILESAVETAG eTag, gchar const * * aszAttributeNames, gchar const * * aszAttributeValues, FilesavePersist * psFilesaveData) {
	double afRange[6];
	int nAttribute;

	GetVisRange (afRange, psFilesaveData->psFunctyData->psVisData);

	switch (eTag) {
	case FILESAVETAG_MIN:
		nAttribute = 0;
		while (aszAttributeNames[nAttribute] != NULL) {
			if (strcmp (aszAttributeNames[nAttribute], "x") == 0) {
				afRange[0] = atof (aszAttributeValues[nAttribute]);
			}
			if (strcmp (aszAttributeNames[nAttribute], "y") == 0) {
				afRange[1] = atof (aszAttributeValues[nAttribute]);
			}
			if (strcmp (aszAttributeNames[nAttribute], "z") == 0) {
				afRange[2] = atof (aszAttributeValues[nAttribute]);
			}
			nAttribute++;
		}
		break;
	case FILESAVETAG_WIDTH:
		nAttribute = 0;
		while (aszAttributeNames[nAttribute] != NULL) {
			if (strcmp (aszAttributeNames[nAttribute], "x") == 0) {
				afRange[3] = atof (aszAttributeValues[nAttribute]);
			}
			if (strcmp (aszAttributeNames[nAttribute], "y") == 0) {
				afRange[4] = atof (aszAttributeValues[nAttribute]);
			}
			if (strcmp (aszAttributeNames[nAttribute], "z") == 0) {
				afRange[5] = atof (aszAttributeValues[nAttribute]);
			}
			nAttribute++;
		}
		break;
	default:
		// Do nothing
		break;
	}

	SetVisRange (afRange, psFilesaveData->psFunctyData->psVisData);
}

void StartElementFunction (FILESAVETAG eTag, gchar const * * aszAttributeNames, gchar const * * aszAttributeValues, FilesavePersist * psFilesaveData) {
	bool boBoolFound;
	bool boDoubleFound;
	double fValue = 0.0f;
	bool boValue = FALSE;
	int nAttribute;
	FUNCTYPE eFuncType = FUNCTYPE_CARTESIAN;

	if (eTag == FILESAVETAG_INVALID) {
		nAttribute = 0;
		while (aszAttributeNames[nAttribute] != NULL) {
			if (strcmp (aszAttributeNames[nAttribute], "type") == 0) {
				if (strcmp (aszAttributeValues[nAttribute], "cartesian") == 0) {
					eFuncType = FUNCTYPE_CARTESIAN;
				}
				else if (strcmp (aszAttributeValues[nAttribute], "spherical") == 0) {
					eFuncType = FUNCTYPE_SPHERICAL;
				}
				else if (strcmp (aszAttributeValues[nAttribute], "curve") == 0) {
					eFuncType = FUNCTYPE_CURVE;
				}
			}
			nAttribute++;
		}

		// Create a new function
		psFilesaveData->psFuncData = AddNewFunction (eFuncType, psFilesaveData->psFunctyData->psVisData);
	}

	if (psFilesaveData->psFuncData) {
		boDoubleFound = FALSE;
		nAttribute = 0;
		while (aszAttributeNames[nAttribute] != NULL) {
			if (strcmp (aszAttributeNames[nAttribute], "bool") == 0) {
				boValue = atoi (aszAttributeValues[nAttribute]);
				boBoolFound = TRUE;
			}
			if (strcmp (aszAttributeNames[nAttribute], "double") == 0) {
				fValue = atof (aszAttributeValues[nAttribute]);
				boDoubleFound = TRUE;
			}
			nAttribute++;
		}

		if (boBoolFound) {
			switch (eTag) {
			case FILESAVETAG_MATERIALFILL:
				SetFunctionMaterialFill (boValue, psFilesaveData->psFuncData);
				break;
			default:
				// Do nothing
				break;
			}
		}

		if (boDoubleFound) {
			switch (eTag) {
			case FILESAVETAG_ACCURACY:
				SetFunctionAccuracy (fValue, psFilesaveData->psFuncData);
				break;
			case FILESAVETAG_MATERIALTHICKNESS:
				SetFunctionMaterialThickness (fValue, psFilesaveData->psFuncData);
				break;
			case FILESAVETAG_ACCURACYRADIUS:
				CurveSetFunctionAccuracyRadius (fValue, psFilesaveData->psFuncData);
				break;
			default:
				// Do nothing
				break;
			}
		}
	}
}

void EndElementFunction (FILESAVETAG eTag, FilesavePersist * psFilesaveData) {
	char const * szRed;
	char const * szGreen;
	char const * szBlue;
	char const * szAlpha;
	char const * szXCentre = NULL;
	char const * szYCentre = NULL;
	char const * szZCentre = NULL;
	char const * szTexFile = NULL;
	char const * szTexXScale = NULL;
	char const * szTexYScale = NULL;
	char const * szTexXOffset = NULL;
	char const * szTexYOffset = NULL;

	FUNCTYPE eFuncType;

	eFuncType = GetFunctionType (psFilesaveData->psFuncData);
	szRed = GetRedString (psFilesaveData->psFuncData);
	szGreen = GetGreenString (psFilesaveData->psFuncData);
	szBlue = GetBlueString (psFilesaveData->psFuncData);
	szAlpha = GetAlphaString (psFilesaveData->psFuncData);

	switch (eFuncType) {
	case FUNCTYPE_SPHERICAL:
		szXCentre = SphericalGetXCentreString (psFilesaveData->psFuncData);
		szYCentre = SphericalGetYCentreString (psFilesaveData->psFuncData);
		szZCentre = SphericalGetZCentreString (psFilesaveData->psFuncData);
		break;
	case FUNCTYPE_CURVE:
		szXCentre = CurveGetXCentreString (psFilesaveData->psFuncData);
		szYCentre = CurveGetYCentreString (psFilesaveData->psFuncData);
		szZCentre = CurveGetZCentreString (psFilesaveData->psFuncData);
		break;
	default:
		// Do nothing
		break;
	}
	szTexFile = GetTexFileString (psFilesaveData->psFuncData);
	szTexXScale = GetTexXScaleString (psFilesaveData->psFuncData);
	szTexYScale = GetTexYScaleString (psFilesaveData->psFuncData);
	szTexXOffset = GetTexXOffsetString (psFilesaveData->psFuncData);
	szTexYOffset = GetTexYOffsetString (psFilesaveData->psFuncData);

	if (psFilesaveData->psFuncData) {
		switch (eTag) {
		case FILESAVETAG_NAME:
			SetFunctionName (psFilesaveData->szText->str, psFilesaveData->psFuncData);
			break;
		case FILESAVETAG_EQUATION:
			SetFunction (psFilesaveData->szText->str, psFilesaveData->psFuncData);
			break;
		case FILESAVETAG_RED:
			SetFunctionColours (psFilesaveData->szText->str, szGreen, szBlue, szAlpha, psFilesaveData->psFuncData);
			break;
		case FILESAVETAG_GREEN:
			SetFunctionColours (szRed, psFilesaveData->szText->str, szBlue, szAlpha, psFilesaveData->psFuncData);
			break;
		case FILESAVETAG_BLUE:
			SetFunctionColours (szRed, szGreen, psFilesaveData->szText->str, szAlpha, psFilesaveData->psFuncData);
			break;
		case FILESAVETAG_ALPHA:
			SetFunctionColours (szRed, szGreen, szBlue, psFilesaveData->szText->str, psFilesaveData->psFuncData);
			break;
		case FILESAVETAG_CENTREX:
			switch (eFuncType) {
			case FUNCTYPE_SPHERICAL:
				SphericalSetFunctionCentre (psFilesaveData->szText->str, szYCentre, szZCentre, psFilesaveData->psFuncData);
				break;
			case FUNCTYPE_CURVE:
				CurveSetFunctionCentre (psFilesaveData->szText->str, szYCentre, szZCentre, psFilesaveData->psFuncData);
				break;
			default:
				// Do Nothing
				break;
			}
			break;
		case FILESAVETAG_CENTREY:
			switch (eFuncType) {
			case FUNCTYPE_SPHERICAL:
				SphericalSetFunctionCentre (szXCentre, psFilesaveData->szText->str, szZCentre, psFilesaveData->psFuncData);
				break;
			case FUNCTYPE_CURVE:
				CurveSetFunctionCentre (szXCentre, psFilesaveData->szText->str, szZCentre, psFilesaveData->psFuncData);
				break;
			default:
				// Do Nothing
				break;
			}
			break;
		case FILESAVETAG_CENTREZ:
			switch (eFuncType) {
			case FUNCTYPE_SPHERICAL:
				SphericalSetFunctionCentre (szXCentre, szYCentre, psFilesaveData->szText->str, psFilesaveData->psFuncData);
				break;
			case FUNCTYPE_CURVE:
				CurveSetFunctionCentre (szXCentre, szYCentre, psFilesaveData->szText->str, psFilesaveData->psFuncData);
				break;
			default:
				// Do Nothing
				break;
			}
			break;
		case FILESAVETAG_EQUATIONX:
			CurveSetFunction (psFilesaveData->szText->str, NULL, NULL, NULL, psFilesaveData->psFuncData);
			break;
		case FILESAVETAG_EQUATIONY:
			CurveSetFunction (NULL, psFilesaveData->szText->str, NULL, NULL, psFilesaveData->psFuncData);
			break;
		case FILESAVETAG_EQUATIONZ:
			CurveSetFunction (NULL, NULL, psFilesaveData->szText->str, NULL, psFilesaveData->psFuncData);
			break;
		case FILESAVETAG_TEXFILE:
			SetTextureValues (psFilesaveData->szText->str, szTexXScale, szTexYScale, szTexXOffset, szTexYOffset, psFilesaveData->psFuncData);
			break;
		case FILESAVETAG_TEXXSCALE:
			SetTextureValues (szTexFile, psFilesaveData->szText->str, szTexYScale, szTexXOffset, szTexYOffset, psFilesaveData->psFuncData);
			break;
		case FILESAVETAG_TEXYSCALE:
			SetTextureValues (szTexFile, szTexXScale, psFilesaveData->szText->str, szTexXOffset, szTexYOffset, psFilesaveData->psFuncData);
			break;
		case FILESAVETAG_TEXXOFFSET:
			SetTextureValues (szTexFile, szTexXScale, szTexYScale, psFilesaveData->szText->str, szTexYOffset, psFilesaveData->psFuncData);
			break;
		case FILESAVETAG_TEXYOFFSET:
			SetTextureValues (szTexFile, szTexXScale, szTexYScale, szTexXOffset, psFilesaveData->szText->str, psFilesaveData->psFuncData);
			break;
		default:
			// Do nothing
			break;
		}
	}
}

void StartElementControlvar (FILESAVETAG eTag, gchar const * * aszAttributeNames, gchar const * * aszAttributeValues, FilesavePersist * psFilesaveData) {
	if (eTag == FILESAVETAG_INVALID) {
		psFilesaveData->psControlvarData = NewControlvarPersist ();
	}
}

void EndElementControlvar (FILESAVETAG eTag, FilesavePersist * psFilesaveData) {
	FnControlPersist * psFnControlData;

	if (psFilesaveData->psControlvarData) {
		switch (eTag) {
		case FILESAVETAG_VARNAME:
			SetControlvarName (psFilesaveData->szText->str, psFilesaveData->psControlvarData);
			break;
		case FILESAVETAG_VARVALUE:
			SetControlvarValue (psFilesaveData->szText->str, psFilesaveData->psControlvarData);
			break;
		case FILESAVETAG_INVALID:
			psFnControlData = GetControlVarList (psFilesaveData->psFunctyData->psVisData);
			psFnControlData->psControlvarList = g_slist_append (psFnControlData->psControlvarList, psFilesaveData->psControlvarData);
			psFilesaveData->psControlvarData = NULL;
			break;
		default:
			// Do nothing
			break;
		}
	}
}

LocaleRestore * ClearLocale () {
	LocaleRestore * psRestore;
	char * szCurrent;

	szCurrent = setlocale (LC_NUMERIC, NULL);
	psRestore = g_new (LocaleRestore, 1);
	if (szCurrent) {
		psRestore->szLocale = g_string_new (szCurrent);
	}
	else {
		psRestore->szLocale = NULL;
	}

	setlocale (LC_NUMERIC, "C");
	
	return psRestore;
}

void RestoreLocale (LocaleRestore * psRestore) {
	if (psRestore) {
		if (psRestore->szLocale) {
			setlocale (LC_NUMERIC, psRestore->szLocale->str);
			g_string_free (psRestore->szLocale, TRUE);
		}
		else {
			setlocale (LC_NUMERIC, "");
		}
		g_free (psRestore);
	}
}

