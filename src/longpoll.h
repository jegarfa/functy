///////////////////////////////////////////////////////////////////
// Functy
// 3D graph drawing utility
//
// David Llewellyn-Jones
// http://www.flypig.co.uk
//
// Autumn 2014
///////////////////////////////////////////////////////////////////

#ifndef LONGPOLL_H
#define LONGPOLL_H

///////////////////////////////////////////////////////////////////
// Includes

#include "vis.h"

///////////////////////////////////////////////////////////////////
// Defines

///////////////////////////////////////////////////////////////////
// Structures and enumerations

typedef struct _LongPollPersist LongPollPersist;
typedef bool (* LongPollCallback) (LongPollPersist * psLongPollData, void * psData);

///////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////
// Function prototypes

LongPollPersist * NewLongPollPersist (void);
void DeleteLongPollPersist (LongPollPersist * psLongPollData);

bool LongPoll (GtkBuilder * psXML, void * psData, LongPollCallback pmStart, LongPollCallback pmStep, LongPollCallback pmFinish, LongPollCallback pmCancel);
void LongPollSetProgress (float fProgress, LongPollPersist * psLongPollData);
void LongPollDone (LongPollPersist * psLongPollData);
void LongPollSetActivityMain (char const * const szText, LongPollPersist * psLongPollData);
void LongPollSetActivitySub (LongPollPersist * psLongPollData, const char * format, ...);

///////////////////////////////////////////////////////////////////
// Function definitions

#endif /* LONGPOLL_H */


