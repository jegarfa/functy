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
#include <unistd.h>

#include "ovdbc.h"
#include "exportvdb.h"
#include "filesave.h"

///////////////////////////////////////////////////////////////////
// Defines

#define TIMESTRING_LEN (32)
#define EDGE_CHECKS_TETRAHEDRON (6)

///////////////////////////////////////////////////////////////////
// Structures and enumerations

struct _ExportVDBPersist {
	GString * szFilename;
	OVDBCPersist * psOVDBCData;
	int nResolution;
	int nSlice;
	VisPersist * psVisData;
};

///////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////
// Function prototypes

///////////////////////////////////////////////////////////////////
// Function definitions

ExportVDBPersist * NewExportVDBPersist (char const * szFilename, int nResolution, VisPersist * psVisData) {
	ExportVDBPersist * psExportVDBData;

	psExportVDBData = g_new0 (ExportVDBPersist, 1);

	psExportVDBData->szFilename = g_string_new (szFilename);
	psExportVDBData->psOVDBCData = NewOVDBCPersist ();
	psExportVDBData->nResolution = nResolution;
	psExportVDBData->nSlice = 0;
	psExportVDBData->psVisData = psVisData;

	return psExportVDBData;
}

void DeleteExportVDBPersist (ExportVDBPersist * psExportVDBData) {
	g_string_free (psExportVDBData->szFilename, TRUE);

	if (psExportVDBData->psOVDBCData) {
		DeleteOVDBCPersist (psExportVDBData->psOVDBCData);
		psExportVDBData->psOVDBCData = NULL;
	}

	g_free (psExportVDBData);
}

bool ExportStartVDB (LongPollPersist * psLongPollData, void * psData) {
	ExportVDBPersist * psExportVDBData = (ExportVDBPersist *)psData;
	bool boSuccess;

	boSuccess = TRUE;
	LongPollSetProgress (0.0f, psLongPollData);
	LongPollSetActivityMain ("Exporting OpenVDB voxel model", psLongPollData);
	LongPollSetActivitySub (psLongPollData, "Initialising");

	psExportVDBData->nSlice = 0;

	AssignControlVarsToFunctionList (psExportVDBData->psVisData);

	return boSuccess;
}

bool ExportStepVDB (LongPollPersist * psLongPollData, void * psData) {
	ExportVDBPersist * psExportVDBData = (ExportVDBPersist *)psData;
	bool boSuccess;
	unsigned char * pcData;
	GSList const * psFuncList;
	int nX;
	int nY;
	int nChannel;
	int nChannels;
	FuncPersist * psFuncData;
	float fProgress;
	Vector3 vPos;

	boSuccess = TRUE;
	fProgress = (float)psExportVDBData->nSlice / (float)psExportVDBData->nResolution;
	LongPollSetProgress (fProgress, psLongPollData);
	LongPollSetActivitySub (psLongPollData, "Exporting slice %d", psExportVDBData->nSlice);

	nChannels = 1;

	// Create the memory to store the slice
	pcData = g_new (unsigned char, (psExportVDBData->nResolution * psExportVDBData->nResolution * nChannels));

	if (pcData != NULL) {
		// Clear the slice
		for (nX = 0; nX < psExportVDBData->nResolution; nX++) {
			for (nY = 0; nY < psExportVDBData->nResolution; nY++) {
				for (nChannel = 0; nChannel < nChannels; nChannel++) {
					pcData[(((nX + (nY * psExportVDBData->nResolution)) * nChannels) + nChannel)] = 0;
				}
			}
		}

		// Render slices for each function
		psFuncList = GetFunctionList (psExportVDBData->psVisData);
		while (psFuncList) {	
			psFuncData = (FuncPersist *)(psFuncList->data);
			// Render the function slice
			OutputVoxelSlice (pcData, psExportVDBData->nResolution, nChannels, psExportVDBData->nSlice, psFuncData);

			psFuncList = g_slist_next (psFuncList);
		}

		// Transfer the data to the OpenVDB grid
		nChannel = 0;
		for (nX = 0; nX < psExportVDBData->nResolution; nX++) {
			for (nY = 0; nY < psExportVDBData->nResolution; nY++) {
				if (pcData[(((nX + (nY * psExportVDBData->nResolution)) * nChannels) + nChannel)] != 0) {
					SetVector3 (vPos, nX, psExportVDBData->nSlice, nY)
					SetGridValueOVDBC (& vPos, psExportVDBData->psOVDBCData);
				}
			}
		}

		free (pcData);
	}
	else {
		printf ("Couldn't allocate memory for voxel slice.\n");
		boSuccess = FALSE;
	}

	psExportVDBData->nSlice++;
	if (psExportVDBData->nSlice >= psExportVDBData->nResolution) {
		LongPollSetActivitySub (psLongPollData, "Saving");
		LongPollDone (psLongPollData);
	}

	return boSuccess;
}

bool ExportFinishVDB (LongPollPersist * psLongPollData, void * psData) {
	ExportVDBPersist * psExportVDBData = (ExportVDBPersist *)psData;
	bool boSuccess;

	boSuccess = TRUE;
	LongPollSetProgress (1.0f, psLongPollData);

	OutputGrid (psExportVDBData->szFilename->str, psExportVDBData->psOVDBCData);

	DeleteExportVDBPersist (psExportVDBData);
	
	return boSuccess;
}

bool ExportCancelVDB (LongPollPersist * psLongPollData, void * psData) {
	ExportVDBPersist * psExportVDBData = (ExportVDBPersist *)psData;

	DeleteExportVDBPersist (psExportVDBData);

	return FALSE;
}





