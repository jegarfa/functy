///////////////////////////////////////////////////////////////////
// Functy
// 3D graph drawing utility
//
// David Llewellyn-Jones
// http://www.flypig.co.uk
//
// Summer 2014
///////////////////////////////////////////////////////////////////

#ifndef EXPORTSVX_H
#define EXPORTSVX_H

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

typedef struct _ExportSVXPersist ExportSVXPersist;

///////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////
// Function prototypes

ExportSVXPersist * NewExportSVXPersist (char const * szFilename, int nResolution, VisPersist * psVisData);
void DeleteExportSVXPersist (ExportSVXPersist * psExportSVXData);

void FilledCuboidSliceSVX (unsigned char * pcData, int nResolution, int nChannels, Vector3 * avCorners, float fZSlice);

bool ExportStartSVX (LongPollPersist * psLongPollData, void * psData);
bool ExportStepSVX (LongPollPersist * psLongPollData, void * psData);
bool ExportFinishSVX (LongPollPersist * psLongPollData, void * psData);
bool ExportCancelSVX (LongPollPersist * psLongPollData, void * psData);


///////////////////////////////////////////////////////////////////
// Function definitions

#endif /* EXPORTSVX_H */


