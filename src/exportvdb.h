///////////////////////////////////////////////////////////////////
// Functy
// 3D graph drawing utility
//
// David Llewellyn-Jones
// http://www.flypig.co.uk
//
// Summer 2014
///////////////////////////////////////////////////////////////////

#ifndef EXPORTVDB_H
#define EXPORTVDB_H

///////////////////////////////////////////////////////////////////
// Includes

#include "vis.h"
#include "function.h"
#include "longpoll.h"

///////////////////////////////////////////////////////////////////
// Defines

#define EXPORTVOXEL_SLICE_EXP (4)

///////////////////////////////////////////////////////////////////
// Structures and enumerations

typedef struct _ExportVDBPersist ExportVDBPersist;

///////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////
// Function prototypes

ExportVDBPersist * NewExportVDBPersist (char const * szFilename, int nResolution, VisPersist * psVisData);
void DeleteExportVDBPersist (ExportVDBPersist * psExportVDBData);

bool ExportStartVDB (LongPollPersist * psLongPollData, void * psData);
bool ExportStepVDB (LongPollPersist * psLongPollData, void * psData);
bool ExportFinishVDB (LongPollPersist * psLongPollData, void * psData);
bool ExportCancelVDB (LongPollPersist * psLongPollData, void * psData);


///////////////////////////////////////////////////////////////////
// Function definitions

#endif /* EXPORTVDB_H */


