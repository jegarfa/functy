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

#include "exportstl.h"
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

struct _ExportSTLPersist {
	GString * szFilename;
	bool boBinary;
	bool boScreenCoords;
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

void ExportModelRecallSTL (Recall * hFile, bool boBinary, bool boScreenCoords, double fMultiplier, double fScale, GSList const * const psFunctions);

///////////////////////////////////////////////////////////////////
// Function definitions

ExportSTLPersist * NewExportSTLPersist (char const * szFilename, bool boBinary, bool boScreenCoords, float fMultiplier, double fScale, double fTimeStart, double fTimeEnd, int nFrames, VisPersist * psVisData) {
	ExportSTLPersist * psExportSTLData;

	psExportSTLData = g_new0 (ExportSTLPersist, 1);

	psExportSTLData->szFilename = g_string_new (szFilename);
	psExportSTLData->boBinary = boBinary;
	psExportSTLData->boScreenCoords = boScreenCoords;
	psExportSTLData->fMultiplier = fMultiplier;
	psExportSTLData->fScale = fScale;
	psExportSTLData->fTimeStart = fTimeStart;
	psExportSTLData->fTimeEnd = fTimeEnd;
	psExportSTLData->nFrames = nFrames;

	psExportSTLData->psZipArchive = NULL;
	psExportSTLData->fTime = 0.0f;
	psExportSTLData->fTimeIncrement = 1.0f;
	psExportSTLData->psVisData = psVisData;
	psExportSTLData->nFrame = 0;

	return psExportSTLData;
}

void DeleteExportSTLPersist (ExportSTLPersist * psExportSTLData) {
	g_string_free (psExportSTLData->szFilename, TRUE);

	g_free (psExportSTLData);
}






















bool ExportModelSTL (char const * szFilename, bool boBinary, bool boScreenCoords, double fMultiplier, double fScale, GSList const * const psFunctions) {
	Recall * hFile;
	bool boResult;

	boResult = FALSE;

	hFile = recopen (szFilename, "m");

	if (hFile) {
		ExportModelRecallSTL (hFile, boBinary, boScreenCoords, fMultiplier, fScale, psFunctions);

		recclose (hFile);
		boResult = TRUE;
	}

	return boResult;
}

void ExportModelRecallSTL (Recall * hFile, bool boBinary, bool boScreenCoords, double fMultiplier, double fScale, GSList const * const psFunctions) {
	int nVertices;
	int nFaces;
	GSList const * psFuncList;
	LocaleRestore * psRestore;
	char acHeader[80] = "For I dance\n\nAnd drink, and sing,\n\nTill some blind hand\n\nShall brush my wing.\n\n\n";
	uint32_t uTriangles;

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


	if (boBinary) {
		// Header 80 bytes
		recwrite (acHeader, sizeof(char), 80, hFile);

		// Calculate the number of triangles
		uTriangles = 0;
		psFuncList = psFunctions;
		while (psFuncList) {
			uTriangles += CountStoredFaces (fMultiplier, (FuncPersist const *)(psFuncList->data));
			psFuncList = g_slist_next (psFuncList);
		}
		recwrite (& uTriangles, sizeof(uint32_t), 1, hFile);
	}
	else {
		recprintf (hFile, "solid functy\n");
	}

	// Output the vertices
	psFuncList = psFunctions;
	while (psFuncList) {
		OutputStoredTrianglesSTL (hFile, boBinary, boScreenCoords, fMultiplier, fScale, (FuncPersist const *)(psFuncList->data));
		psFuncList = g_slist_next (psFuncList);
	}
	
	if (!boBinary) {
		recprintf (hFile, "endsolid\n");
	}

	RestoreLocale (psRestore);
}



bool ExportStartAnimatedSTL (LongPollPersist * psLongPollData, void * psData) {
	ExportSTLPersist * psExportSTLData = (ExportSTLPersist *)psData;
	bool boSuccess;
	int nResult;
	int nError = 0;
	char * szError;
	zip_uint64_t uErrorLength;

	boSuccess = TRUE;
	LongPollSetProgress (0.0f, psLongPollData);
	LongPollSetActivityMain ("Exporting STL animation", psLongPollData);
	LongPollSetActivitySub (psLongPollData, "Initialising");

	// Check whether the file exists and delete it if it does
	nResult = access (psExportSTLData->szFilename->str, F_OK);
	if (nResult == 0) {
		nResult = remove (psExportSTLData->szFilename->str);
		if (nResult != 0) {
			printf ("Error deleting existing file.");
			boSuccess = FALSE;
		}
	}

	if (boSuccess) {
		// Create the archive
		//psZipArchive = zip_open (szFilename, (ZIP_CREATE | ZIP_TRUNCATE), & nError);
		psExportSTLData->psZipArchive = zip_open (psExportSTLData->szFilename->str, (ZIP_CREATE), & nError);

		if (psExportSTLData->psZipArchive == NULL) {
			uErrorLength = zip_error_to_str (NULL, 0, nError, errno);
			szError = g_malloc (uErrorLength + 1);
			uErrorLength = zip_error_to_str (szError, uErrorLength + 1, nError, errno);
			printf ("Error creating archive: %s\n", szError);
			g_free (szError);
			boSuccess = FALSE;
		}
	
		psExportSTLData->fTimeIncrement = (psExportSTLData->fTimeEnd - psExportSTLData->fTimeStart) / ((double)psExportSTLData->nFrames);

		psExportSTLData->fTime = psExportSTLData->fTimeStart;
	}

	return boSuccess;
}

bool ExportStepAnimatedSTL (LongPollPersist * psLongPollData, void * psData) {
	ExportSTLPersist * psExportSTLData = (ExportSTLPersist *)psData;
	bool boSuccess;
	Recall * hFile;
	zip_int64_t nPosition;
	struct zip_source * pzZipSource;
	void * pData;
	unsigned long int uSize;
	GString * szFileIncrement;
	GString * szFileNameFormat;
	FuncPersist * psFuncData;
	FUNCTYPE eFuncType;
	GSList const * psFunctions;
	GSList const * psFuncList;
	float fProgress;

	boSuccess = TRUE;

	fProgress = (float)psExportSTLData->nFrame / (float)psExportSTLData->nFrames;
	LongPollSetProgress (fProgress, psLongPollData);
	LongPollSetActivitySub (psLongPollData, "Exporting frame %d", psExportSTLData->nFrame);

	// Create the format for the filename
	szFileNameFormat = g_string_new ("");
	// Format is a decimal integer with EXPORTANIM_FRAMES_EXP digits padded with zeros, followed by .stl
	g_string_printf (szFileNameFormat, "%%0%dd.stl", EXPORTANIM_FRAMES_EXP);
	szFileIncrement = g_string_new ("");

	// Set the filename
	g_string_printf (szFileIncrement, szFileNameFormat->str, psExportSTLData->nFrame);

	// Set the time for all of the functions

	psFunctions = GetFunctionList (psExportSTLData->psVisData);
	psFuncList = psFunctions;
	while (psFuncList) {
		psFuncData = (FuncPersist *)(psFuncList->data);
		eFuncType = GetFunctionType (psFuncData);

		switch (eFuncType) {
		case FUNCTYPE_SPHERICAL:
			SphericalSetFunctionTime (psExportSTLData->fTime, psFuncData);
			SphericalUpdateCentre (psFuncData);
			break;
		case FUNCTYPE_CURVE:
			CurveSetFunctionTime (psExportSTLData->fTime, psFuncData);
			CurveUpdateCentre (psFuncData);
			break;
		default:
			// Do nothing
			break;
		}

		SetFunctionTime (psExportSTLData->fTime, (FuncPersist *)(psFuncList->data));
		psFuncList = g_slist_next (psFuncList);
	}

	hFile = recopen (NULL, "m");

	if (hFile) {
		ExportModelRecallSTL (hFile, psExportSTLData->boBinary, psExportSTLData->boScreenCoords, psExportSTLData->fMultiplier, psExportSTLData->fScale, psFunctions);

		uSize = MemoryGetSize (hFile);
		pData = MemoryDetachData (hFile);

		if (pData != NULL) {
			// Add memory to the archive as a file
			// This will be freed automatically once it's no longer needed
			pzZipSource = zip_source_buffer (psExportSTLData->psZipArchive, pData, uSize, 1);

			nPosition = zip_add (psExportSTLData->psZipArchive, szFileIncrement->str, pzZipSource);

			if (nPosition < 0) {
				zip_source_free (pzZipSource);
				printf ("Error adding file: %s\n", zip_strerror (psExportSTLData->psZipArchive));
				boSuccess = FALSE;
			}
		}

		// We're done with our memory stream, so we can close it
		recclose (hFile);
		//printf ("Output frame %d out of %d.\n", (psExportSTLData->nFrame + 1), psExportSTLData->nFrames);
	}
		else {
		printf ("File couuld not be opened.\n");
		boSuccess = FALSE;
	}

	g_string_free (szFileIncrement, TRUE);
	g_string_free (szFileNameFormat, TRUE);

	psExportSTLData->fTime += psExportSTLData->fTimeIncrement;

	psExportSTLData->nFrame++;
	if (psExportSTLData->nFrame >= psExportSTLData->nFrames) {
		LongPollSetActivitySub (psLongPollData, "Compressing");
		LongPollDone (psLongPollData);
	}

	return boSuccess;
}

bool ExportFinishAnimatedSTL (LongPollPersist * psLongPollData, void * psData) {
	ExportSTLPersist * psExportSTLData = (ExportSTLPersist *)psData;
	bool boSuccess;
	int nError = 0;

	boSuccess = TRUE;
	LongPollSetProgress (1.0f, psLongPollData);

	// Close the archive
	//printf ("Compressing (this may take some time).\n");
	nError = zip_close (psExportSTLData->psZipArchive);

	if (nError < 0) {
		printf ("Error closing archive: %s\n", zip_strerror (psExportSTLData->psZipArchive));
	}

	DeleteExportSTLPersist (psExportSTLData);

	return boSuccess;
}

bool ExportCancelAnimatedSTL (LongPollPersist * psLongPollData, void * psData) {
	ExportSTLPersist * psExportSTLData = (ExportSTLPersist *)psData;

	// Discard any changes
	zip_discard (psExportSTLData->psZipArchive);

	DeleteExportSTLPersist (psExportSTLData);

	return FALSE;
}


