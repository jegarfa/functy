///////////////////////////////////////////////////////////////////
// Functy
// 3D graph drawing utility
//
// David Llewellyn-Jones
// http://www.flypig.co.uk
//
// Summer 2012
///////////////////////////////////////////////////////////////////

#ifndef EXPORTPLY_H
#define EXPORTPLY_H

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

typedef struct _ExportPLYPersist ExportPLYPersist;

///////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////
// Function prototypes

ExportPLYPersist * NewExportPLYPersist (char const * szFilename, bool boBinary, bool boScreenCoords, bool boExportAlpha, float fMultiplier, double fScale, double fTimeStart, double fTimeEnd, int nFrames, VisPersist * psVisData);
void DeleteExportPLYPersist (ExportPLYPersist * psExportPLYData);

bool ExportModelPLY (char const * szFilename, bool boBinary, bool boScreenCoords, bool boExportAlpha, double fMultiplier, double fScale, GSList const * const psFunctions);

bool ExportStartAnimatedPLY (LongPollPersist * psLongPollData, void * psData);
bool ExportStepAnimatedPLY (LongPollPersist * psLongPollData, void * psData);
bool ExportFinishAnimatedPLY (LongPollPersist * psLongPollData, void * psData);
bool ExportCancelAnimatedPLY (LongPollPersist * psLongPollData, void * psData);

///////////////////////////////////////////////////////////////////
// Function definitions

#endif /* EXPORTPLY_H */


