///////////////////////////////////////////////////////////////////
// Enzyme
// 3D Functy/Dandelion/programming game
//
// David Llewellyn-Jones
//
// October 2016
///////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
// Includes

#include "utils.h"

#include "global.h"

///////////////////////////////////////////////////////////////////
// Defines

///////////////////////////////////////////////////////////////////
// Structures and enumerations

struct _GlobalPersist {
	GString * psDataDir;
};

///////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////
// Function prototypes

///////////////////////////////////////////////////////////////////
// Function definitions

GlobalPersist * NewGlobalPersist () {
	GlobalPersist * psGlobalData;

	// Allocate some memory for the new structures
	psGlobalData = g_new0 (GlobalPersist, 1);

	psGlobalData->psDataDir = g_string_new (FUNCTYDIR);

	return psGlobalData;
}

void DeleteGlobalPersist (GlobalPersist * psGlobalData) {
	if (psGlobalData->psDataDir) {
		g_string_free(psGlobalData->psDataDir, TRUE);
	}

	// Free up the structures
	g_free (psGlobalData);
}

void SetDataDir (char const * szDir, GlobalPersist * psGlobalData) {
	psGlobalData->psDataDir = g_string_assign (psGlobalData->psDataDir, szDir);
}

void GenerateDataPath (GString * szLeaf, GlobalPersist const * psGlobalData) {
	if (szLeaf) {
		g_string_prepend (szLeaf, psGlobalData->psDataDir->str);
	}
}


