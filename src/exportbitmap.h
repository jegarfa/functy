///////////////////////////////////////////////////////////////////
// Functy
// 3D graph drawing utility
//
// David Llewellyn-Jones
// http://www.flypig.co.uk
//
// Summer 2012
///////////////////////////////////////////////////////////////////

#ifndef EXPORTBITMAP_H
#define EXPORTBITMAP_H

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

typedef struct _ExportBitmapPersist ExportBitmapPersist;

///////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////
// Function prototypes

ExportBitmapPersist * NewExportBitmapPersist (char const * szFilename, char const * szType, int nHeight, int nWidth, double fTimeStart, double fTimeEnd, int nFrames,  VisPersist * psVisData);
void DeleteExportBitmapPersist (ExportBitmapPersist * psExportBitmapData);

bool ExportBitmapFile (char const * szFilename, char const * szType, int nHeight, int nWidth, VisPersist * psVisData);
bool ExportBitmapMemory (gchar ** ppcBuffer, gsize * puSize, char const * szType, int nHeight, int nWidth, VisPersist * psVisData);

bool ExportStartAnimatedBitmap (LongPollPersist * psLongPollData, void * psData);
bool ExportStepAnimatedBitmap (LongPollPersist * psLongPollData, void * psData);
bool ExportFinishAnimatedBitmap (LongPollPersist * psLongPollData, void * psData);
bool ExportCancelAnimatedBitmap (LongPollPersist * psLongPollData, void * psData);

///////////////////////////////////////////////////////////////////
// Function definitions

#endif /* EXPORTBITMAP_H */


