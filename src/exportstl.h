///////////////////////////////////////////////////////////////////
// Functy
// 3D graph drawing utility
//
// David Llewellyn-Jones
// http://www.flypig.co.uk
//
// Summer 2012
///////////////////////////////////////////////////////////////////

#ifndef EXPORTSTL_H
#define EXPORTSTL_H

///////////////////////////////////////////////////////////////////
// Includes

#include "vis.h"
#include "function.h"
#include "longpoll.h"

///////////////////////////////////////////////////////////////////
// Defines

#define EXPORTANIM_FRAMES_EXP (5)

///////////////////////////////////////////////////////////////////
// Structures and enumerations

typedef struct _ExportSTLPersist ExportSTLPersist;

///////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////
// Function prototypes

ExportSTLPersist * NewExportSTLPersist (char const * szFilename, bool boBinary, bool boScreenCoords, float fMultiplier, double fScale, double fTimeStart, double fTimeEnd, int nFrames, VisPersist * psVisData);
void DeleteExportSTLPersist (ExportSTLPersist * psExportSTLData);

bool ExportModelSTL (char const * szFilename, bool boBinary, bool boScreenCoords, double fMultiplier, double fScale, GSList const * const psFunctions);


bool ExportStartAnimatedSTL (LongPollPersist * psLongPollData, void * psData);	
bool ExportStepAnimatedSTL (LongPollPersist * psLongPollData, void * psData);
bool ExportFinishAnimatedSTL (LongPollPersist * psLongPollData, void * psData);
bool ExportCancelAnimatedSTL (LongPollPersist * psLongPollData, void * psData);

///////////////////////////////////////////////////////////////////
// Function definitions

#endif /* EXPORTSTL_H */


