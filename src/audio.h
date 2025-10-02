///////////////////////////////////////////////////////////////////
// Functy
// 3D graph drawing utility
//
// David Llewellyn-Jones
// http://www.flypig.co.uk
//
// Summer 2015
///////////////////////////////////////////////////////////////////

#ifndef AUDIO_H
#define AUDIO_H

///////////////////////////////////////////////////////////////////
// Includes

#include <pulse/pulseaudio.h>
#include "textures.h"
#include "symbolic.h"

///////////////////////////////////////////////////////////////////
// Defines

#define AUDIO_SOURCE_NAME_MAX (256)
#define AUDIO_BARS (120)

///////////////////////////////////////////////////////////////////
// Structures and enumerations

typedef struct _AudioPersist AudioPersist;

///////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////
// Function prototypes

AudioPersist * NewAudioPersist ();
void DeleteAudioPersist (AudioPersist * psAudioData);
void SetAudioTextureData (TexPersist * psTexData, AudioPersist * psAudioData);
void SetAudioPause (bool boPause, AudioPersist * psAudioData);
int GetAudioPower (int nBar, AudioPersist * psAudioData);
void AudioReturnSources (pa_source_info_cb_t pfCallback, void * psUserData, AudioPersist * psAudioData);
pa_stream * AudioGetRecordStream (AudioPersist * psAudioData);
void RecordStart (char const * const szSource, AudioPersist * psAudioData);
void RecordStop (AudioPersist * psAudioData);
bool AudioCheckRecordActive (AudioPersist * psAudioData);

double AudioApproximate (double fVar1, void * psContext);
Operation * AudioDifferentiate (Operation * psOp, Operation * psWRT, void * psContext);
Operation * AudioSimplify (Operation * psOp, void * psContext);

///////////////////////////////////////////////////////////////////
// Function definitions

#endif /* AUDIO_H */


