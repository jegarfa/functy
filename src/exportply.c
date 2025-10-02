///////////////////////////////////////////////////////////////////
// Functy
// 3D graph drawing utility
//
// David Llewellyn-Jones
// http://www.flypig.co.uk
//
// Summer 2012
///////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////
// Includes

#include <errno.h>
#include <zip.h>
#include <unistd.h>

#include "exportply.h"
#include "filesave.h"
#include "spherical.h"
#include "curve.h"

///////////////////////////////////////////////////////////////////
// Defines

#ifdef WORDS_BIGENDIAN
#define PLY_ENDIANNESS "binary_big_endian"
#else
#define PLY_ENDIANNESS "binary_little_endian"
#endif

///////////////////////////////////////////////////////////////////
// Structures and enumerations

struct _ExportPLYPersist {
	GString * szFilename;
	bool boBinary;
	bool boScreenCoords;
	bool boExportAlpha;
	float fMultiplier;
	double fScale;
	double fTimeStart;
	double fTimeEnd;
	int nFrames;

	struct zip * psZipArchive;
	double fTime;
	double fTimeIncrement;
	VisPersist * psVisData;
	int nFrame;	
};

///////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////
// Function prototypes

void ExportModelRecallPLY (Recall * hFile, bool boBinary, bool boScreenCoords, bool boExportAlpha, double fMultiplier, double fScale, GSList const * const psFunctions);

///////////////////////////////////////////////////////////////////
// Function definitions

ExportPLYPersist * NewExportPLYPersist (char const * szFilename, bool boBinary, bool boScreenCoords, bool boExportAlpha, float fMultiplier, double fScale, double fTimeStart, double fTimeEnd, int nFrames, VisPersist * psVisData) {
	ExportPLYPersist * psExportPLYData;

	psExportPLYData = g_new0 (ExportPLYPersist, 1);

	psExportPLYData->szFilename = g_string_new (szFilename);
	psExportPLYData->boBinary = boBinary;
	psExportPLYData->boScreenCoords = boScreenCoords;
	psExportPLYData->boExportAlpha = boExportAlpha;
	psExportPLYData->fMultiplier = fMultiplier;
	psExportPLYData->fScale = fScale;
	psExportPLYData->fTimeStart = fTimeStart;
	psExportPLYData->fTimeEnd = fTimeEnd;
	psExportPLYData->nFrames = nFrames;

	psExportPLYData->psZipArchive = NULL;
	psExportPLYData->fTime = 0.0f;
	psExportPLYData->fTimeIncrement = 1.0f;
	psExportPLYData->psVisData = psVisData;
	psExportPLYData->nFrame = 0;

	return psExportPLYData;
}

void DeleteExportPLYPersist (ExportPLYPersist * psExportPLYData) {
	g_string_free (psExportPLYData->szFilename, TRUE);

	g_free (psExportPLYData);
}



bool ExportModelPLY (char const * szFilename, bool boBinary, bool boScreenCoords, bool boExportAlpha, double fMultiplier, double fScale, GSList const * const psFunctions) {
	Recall * hFile;
	bool boResult;

	boResult = FALSE;

	hFile = recopen (szFilename, "m");

	if (hFile) {
		ExportModelRecallPLY (hFile, boBinary, boScreenCoords, boExportAlpha, fMultiplier, fScale, psFunctions);

		recclose (hFile);
		boResult = TRUE;
	}

	return boResult;
}

void ExportModelRecallPLY (Recall * hFile, bool boBinary, bool boScreenCoords, bool boExportAlpha, double fMultiplier, double fScale, GSList const * const psFunctions) {
	int nVertices;
	int nFaces;
	int nOffset;
	GSList const * psFuncList;
	GSList * psOffsets;
	GSList * psOffsetCurrent;
	LocaleRestore * psRestore;

	psRestore = ClearLocale ();

	// Calculate the number of vertices
	nVertices = 0;
	psFuncList = psFunctions;
	while (psFuncList) {
		nVertices += CountStoredVertices (fMultiplier, (FuncPersist const *)(psFuncList->data));
		psFuncList = g_slist_next (psFuncList);
	}

	// Calculate the number of faces
	nFaces = 0;
	psFuncList = psFunctions;
	while (psFuncList) {
		nFaces += CountStoredFaces (fMultiplier, (FuncPersist const *)(psFuncList->data));
		psFuncList = g_slist_next (psFuncList);
	}


	recprintf (hFile, "ply\n");
	if (boBinary) {
		recprintf (hFile, "format " PLY_ENDIANNESS " 1.0 1.0\n");
	}
	else {
		recprintf (hFile, "format ascii 1.0\n");
	}
	recprintf (hFile, "comment Functy generated\n");
	recprintf (hFile, "element vertex %d\n", nVertices);
	recprintf (hFile, "property float32 x\n");
	recprintf (hFile, "property float32 y\n");
	recprintf (hFile, "property float32 z\n");
	recprintf (hFile, "property uint8 red\n");
	recprintf (hFile, "property uint8 green\n");
	recprintf (hFile, "property uint8 blue\n");
	if (boExportAlpha) {
		recprintf (hFile, "property uint8 alpha\n");
	}
	recprintf (hFile, "property float32 nx\n");
	recprintf (hFile, "property float32 ny\n");
	recprintf (hFile, "property float32 nz\n");
	recprintf (hFile, "element face %d\n", nFaces);
	recprintf (hFile, "property list uchar int vertex_indices\n");
	recprintf (hFile, "end_header\n");

	// Output the vertices
	psOffsets = NULL;
	psFuncList = psFunctions;
	while (psFuncList) {
		nVertices = OutputStoredVertices (hFile, boBinary, boScreenCoords, boExportAlpha, fMultiplier, fScale, (FuncPersist const *)(psFuncList->data));
		psOffsets = g_slist_append (psOffsets, GINT_TO_POINTER (nVertices));
		psFuncList = g_slist_next (psFuncList);
	}
	
	// Output the indices
	nOffset = 0;
	psOffsetCurrent = psOffsets;
	psFuncList = psFunctions;
	while (psFuncList) {
		OutputStoredIndices (hFile, boBinary, fMultiplier, nOffset, (FuncPersist const *)(psFuncList->data));
		nOffset += GPOINTER_TO_INT (psOffsetCurrent->data);
		psOffsetCurrent = g_slist_next (psOffsetCurrent);
		psFuncList = g_slist_next (psFuncList);
	}
	g_slist_free (psOffsets);


	RestoreLocale (psRestore);
}







bool ExportStartAnimatedPLY (LongPollPersist * psLongPollData, void * psData) {
	ExportPLYPersist * psExportPLYData = (ExportPLYPersist *)psData;
	bool boSuccess;
	int nResult;
	zip_uint64_t uErrorLength;
	char * szError;
	int nError = 0;

	boSuccess = TRUE;
	LongPollSetProgress (0.0f, psLongPollData);
	LongPollSetActivityMain ("Exporting PLY animation", psLongPollData);
	LongPollSetActivitySub (psLongPollData, "Initialising");

	// Check whether the file exists and delete it if it does
	nResult = access (psExportPLYData->szFilename->str, F_OK);
	if (nResult == 0) {
		nResult = remove (psExportPLYData->szFilename->str);
		if (nResult != 0) {
			printf ("Error deleting existing file.");
			boSuccess = FALSE;
		}
	}

	if (boSuccess) {
		// Create the archive
		//psZipArchive = zip_open (szFilename, (ZIP_CREATE | ZIP_TRUNCATE), & nError);
		psExportPLYData->psZipArchive = zip_open (psExportPLYData->szFilename->str, (ZIP_CREATE), & nError);

		if (psExportPLYData->psZipArchive == NULL) {
			uErrorLength = zip_error_to_str (NULL, 0, nError, errno);
			szError = g_malloc (uErrorLength + 1);
			uErrorLength = zip_error_to_str (szError, uErrorLength + 1, nError, errno);
			printf ("Error creating archive: %s\n", szError);
			g_free (szError);
			boSuccess = FALSE;
		}
	
		psExportPLYData->fTimeIncrement = (psExportPLYData->fTimeEnd - psExportPLYData->fTimeStart) / ((double)psExportPLYData->nFrames);

		psExportPLYData->fTime = psExportPLYData->fTimeStart;
		psExportPLYData->nFrame = 0;
	}
	
	return boSuccess;
}

bool ExportStepAnimatedPLY (LongPollPersist * psLongPollData, void * psData) {
	ExportPLYPersist * psExportPLYData = (ExportPLYPersist *)psData;
	bool boSuccess;
	float fProgress;
	GString * szFileIncrement;
	GString * szFileNameFormat;
	Recall * hFile;
	GSList const * psFuncList;
	FuncPersist * psFuncData;
	FUNCTYPE eFuncType;
	void * pData;
	unsigned long int uSize;
	struct zip_source * pzZipSource;
	zip_int64_t nPosition;
	GSList const * psFunctions;

	boSuccess = TRUE;

	fProgress = (float)psExportPLYData->nFrame / (float)psExportPLYData->nFrames;
	LongPollSetProgress (fProgress, psLongPollData);
	LongPollSetActivitySub (psLongPollData, "Exporting frame %d", psExportPLYData->nFrame);

	// Create the format for the filename
	szFileNameFormat = g_string_new ("");
	// Format is a decimal integer with EXPORTANIM_FRAMES_EXP digits padded with zeros, followed by .ply
	g_string_printf (szFileNameFormat, "%%0%dd.ply", EXPORTANIM_FRAMES_EXP);
	szFileIncrement = g_string_new ("");


	// Set the filename
	g_string_printf (szFileIncrement, szFileNameFormat->str, psExportPLYData->nFrame);

	// Set the time for all of the functions
	psFunctions = GetFunctionList (psExportPLYData->psVisData);
	psFuncList = psFunctions;
	while (psFuncList) {
		psFuncData = (FuncPersist *)(psFuncList->data);
		eFuncType = GetFunctionType (psFuncData);

		switch (eFuncType) {
		case FUNCTYPE_SPHERICAL:
			SphericalSetFunctionTime (psExportPLYData->fTime, psFuncData);
			SphericalUpdateCentre (psFuncData);
			break;
		case FUNCTYPE_CURVE:
			CurveSetFunctionTime (psExportPLYData->fTime, psFuncData);
			CurveUpdateCentre (psFuncData);
			break;
		default:
			// Do nothing
			break;
		}

		SetFunctionTime (psExportPLYData->fTime, (FuncPersist *)(psFuncList->data));
		psFuncList = g_slist_next (psFuncList);
	}

	hFile = recopen (NULL, "m");

	if (hFile) {
		ExportModelRecallPLY (hFile, psExportPLYData->boBinary, psExportPLYData->boScreenCoords, psExportPLYData->boExportAlpha, psExportPLYData->fMultiplier, psExportPLYData->fScale, psFunctions);

		uSize = MemoryGetSize (hFile);
		pData = MemoryDetachData (hFile);

		if (pData != NULL) {
			// Add memory to the archive as a file
			// This will be freed automatically once it's no longer needed
			pzZipSource = zip_source_buffer (psExportPLYData->psZipArchive, pData, uSize, 1);

			nPosition = zip_add (psExportPLYData->psZipArchive, szFileIncrement->str, pzZipSource);

			if (nPosition < 0) {
				zip_source_free (pzZipSource);
				printf ("Error adding file: %s\n", zip_strerror (psExportPLYData->psZipArchive));
				boSuccess = FALSE;
			}
		}

		// We're done with our memory stream, so we can close it
		recclose (hFile);
		//printf ("Output frame %d out of %d.\n", (psExportPLYData->nFrame + 1), psExportPLYData->nFrames);
	}
	else {
		printf ("File couuld not be opened.\n");
		boSuccess = FALSE;
	}

	g_string_free (szFileIncrement, TRUE);
	g_string_free (szFileNameFormat, TRUE);

	psExportPLYData->fTime += psExportPLYData->fTimeIncrement;

	psExportPLYData->nFrame++;
	if (psExportPLYData->nFrame >= psExportPLYData->nFrames) {
		LongPollSetActivitySub (psLongPollData, "Compressing");
		LongPollDone (psLongPollData);
	}

	return boSuccess;
}

bool ExportFinishAnimatedPLY (LongPollPersist * psLongPollData, void * psData) {
	ExportPLYPersist * psExportPLYData = (ExportPLYPersist *)psData;
	bool boSuccess;
	int nError = 0;

	boSuccess = TRUE;
	LongPollSetProgress (1.0f, psLongPollData);

	// Close the archive
	//printf ("Compressing (this may take some time).\n");
	nError = zip_close (psExportPLYData->psZipArchive);

	if (nError < 0) {
		printf ("Error closing archive: %s\n", zip_strerror (psExportPLYData->psZipArchive));
	}

	DeleteExportPLYPersist (psExportPLYData);

	return boSuccess;
}

bool ExportCancelAnimatedPLY (LongPollPersist * psLongPollData, void * psData) {
	ExportPLYPersist * psExportPLYData = (ExportPLYPersist *)psData;

	// Discard any changes
	zip_discard (psExportPLYData->psZipArchive);

	DeleteExportPLYPersist (psExportPLYData);

	return FALSE;
}


