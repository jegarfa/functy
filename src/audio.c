///////////////////////////////////////////////////////////////////
// Functy
// 3D graph drawing utility
//
// David Llewellyn-Jones
// http://www.flypig.co.uk
//
// Summer 2015
///////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////
// Includes

#include <symbolic.h>
#include <gtk/gtk.h>
#include <string.h>
#include <pulse/error.h>
#include <pulse/glib-mainloop.h>
#include <fftw3.h>
#include <math.h>

//#define NCURSESBARS (1)

#ifdef NCURSESBARS
#include <locale.h>
#include <curses.h>
#endif // NCURSESBARS

#include "audio.h"

///////////////////////////////////////////////////////////////////
// Defines

#define RATE (44100)
#define CHANNELS (1)
#define FPS (30)
#define GAIN (1.0)
#define UPPER_FREQ (3520.0)
#define SCALE (78)
#define SOURCE NULL
#define LATENCY (300000)

///////////////////////////////////////////////////////////////////
// Structures and enumerations

struct _AudioPersist {
	// General
	int nBufferSize;
	GLubyte auBars[AUDIO_BARS];
	int nBufferPos;
	TexPersist * psTexData;
	bool boPause;

	// PulseAudio structures
	pa_glib_mainloop * psMainLoop;
	pa_context * psContext;
	float * pfWindow;
	float * pfBuffer;
	int nPulseAudioState;
	pa_stream * psRecordStream;

	// FFTW structures
	double * pfIn;
	fftw_complex * pOut;
	fftw_plan sPlan;
};

///////////////////////////////////////////////////////////////////
// Global variables


///////////////////////////////////////////////////////////////////
// Function prototypes

void AudioInitialise (AudioPersist * psAudioData);
float AudioHanningWindow (int nFrequency, int nMaxFrequency) ;
void CalculateBars (fftw_complex * acFFTData, int nDataSize, GLubyte * auBars, int nNumBars);
void PulseAudioStateCallback (pa_context * psContext, void * psUserData);
static void PulseAudioStreamReadCallback (pa_stream * psRecordStream, size_t nLength, void * psUserData);
void AudioAnalyse (AudioPersist * psAudioData);
void AudioGetSources (AudioPersist * psAudioData);
void AudioGetSourceCallback (pa_context * psContext, const pa_source_info * psSourceInfo, int nEol, void * psUserData);

///////////////////////////////////////////////////////////////////
// Function definitions

AudioPersist * NewAudioPersist () {
	AudioPersist * psAudioData;
	
	psAudioData = g_new0 (AudioPersist, 1);
	
	AudioInitialise (psAudioData);
	
	return psAudioData;
}

void AudioInitialise (AudioPersist * psAudioData) {
	int nFrequency;
	pa_mainloop_api * psMainLoopAPI;
	int nBar;

	// Used the excellent pasa command line PulseAudio spectrum analyser for inspiration
	// https://gitlab.com/nitroxis/pasa
	psAudioData-> psMainLoop = pa_glib_mainloop_new (NULL);
	psMainLoopAPI = pa_glib_mainloop_get_api (psAudioData-> psMainLoop);
	psAudioData->psContext = pa_context_new (psMainLoopAPI, "Simple PA test application");
	pa_context_connect (psAudioData->psContext, NULL, 0, NULL);
	pa_context_set_state_callback (psAudioData->psContext, PulseAudioStateCallback, psAudioData);

	psAudioData->nPulseAudioState = 0;
	psAudioData->nBufferSize = RATE / FPS;
	psAudioData->pfWindow = g_new (float, psAudioData->nBufferSize);
	psAudioData->pfBuffer = g_new (float, psAudioData->nBufferSize);
	psAudioData->psRecordStream = NULL;
	psAudioData->boPause = FALSE;

	for (nBar = 0; nBar < AUDIO_BARS; nBar++) {
		psAudioData->auBars[nBar] = 0;
	}

	for (nFrequency = 0; nFrequency < psAudioData->nBufferSize; nFrequency++) {
		psAudioData->pfWindow[nFrequency] = AudioHanningWindow (nFrequency, psAudioData->nBufferSize);
	}
	
	psAudioData->pfIn = (double *)fftw_malloc (sizeof (double) * psAudioData->nBufferSize);
	psAudioData->pOut = (fftw_complex*)fftw_malloc (sizeof (fftw_complex) * psAudioData->nBufferSize);
	psAudioData->sPlan = fftw_plan_dft_r2c_1d (psAudioData->nBufferSize, psAudioData->pfIn, psAudioData->pOut, FFTW_MEASURE);

#ifdef NCURSESBARS
	// Setup ncurses
	setlocale (LC_ALL, "");
	initscr ();
	curs_set (0);
#endif // NCURSESBARS
}

void DeleteAudioPersist (AudioPersist * psAudioData) {
	fftw_destroy_plan (psAudioData->sPlan);
	fftw_free (psAudioData->pfIn);
	fftw_free (psAudioData->pOut);

	pa_context_disconnect (psAudioData->psContext);
	pa_context_unref (psAudioData->psContext);
	pa_glib_mainloop_free (psAudioData-> psMainLoop);

	g_free (psAudioData);
}

void SetAudioTextureData (TexPersist * psTexData, AudioPersist * psAudioData) {
	psAudioData->psTexData = psTexData;
}

void SetAudioPause (bool boPause, AudioPersist * psAudioData) {
	psAudioData->boPause = boPause;
}

float AudioHanningWindow (int nFrequency, int nMaxFrequency) {
	float fActual;
	fActual = 0.5f * (1.0f - cosf (2.0f * M_PI * nFrequency / (nMaxFrequency - 1.0f)));
	return fActual;
}

void RecordStart (char const * const szSource, AudioPersist * psAudioData) {
	pa_sample_spec sSampleSpec;
	pa_buffer_attr sBufferAttributes;
	int nResult;

	RecordStop (psAudioData);

	sSampleSpec.rate = RATE;
	sSampleSpec.channels = CHANNELS;
	sSampleSpec.format = PA_SAMPLE_FLOAT32LE;
	psAudioData->psRecordStream = pa_stream_new (psAudioData->psContext, "Record", & sSampleSpec, NULL);
	if (!psAudioData->psRecordStream) {
		printf("pa_stream_new failed\n");
	}
	pa_stream_set_read_callback (psAudioData->psRecordStream, PulseAudioStreamReadCallback, psAudioData);
	sBufferAttributes.maxlength = pa_usec_to_bytes (LATENCY, & sSampleSpec);
	sBufferAttributes.tlength = pa_usec_to_bytes (LATENCY, & sSampleSpec);
	sBufferAttributes.prebuf = (uint32_t)-1;
	sBufferAttributes.minreq = pa_usec_to_bytes (0, & sSampleSpec);
	sBufferAttributes.fragsize = (uint32_t)-1;

	nResult = pa_stream_connect_record (psAudioData->psRecordStream, szSource, & sBufferAttributes, PA_STREAM_INTERPOLATE_TIMING | PA_STREAM_ADJUST_LATENCY | PA_STREAM_AUTO_TIMING_UPDATE);
	if (nResult < 0) {
		printf("pa_stream_connect_record\n");
	}

	psAudioData->nBufferPos = 0;
}

void RecordStop (AudioPersist * psAudioData) {
	int nBar;

	if (psAudioData->psRecordStream != NULL) {
		pa_stream_disconnect (psAudioData->psRecordStream);
		psAudioData->psRecordStream = NULL;
	}

	for (nBar = 0; nBar < AUDIO_BARS; nBar++) {
		psAudioData->auBars[nBar] = 0;
	}

	// Transfer to texture
	UpdateTextureSingleChannel (TEXNAME_AUDIO, AUDIO_BARS, 1, psAudioData->auBars, psAudioData->psTexData);
}

bool AudioCheckRecordActive (AudioPersist * psAudioData) {
	return (psAudioData->psRecordStream != NULL);
}

void CalculateBars (fftw_complex * acFFTData, int nDataSize, GLubyte * auBars, int nNumBars) {
	// todo: use the float-point value and implement proper interpolation.
	double barWidthD = UPPER_FREQ / (FPS * AUDIO_BARS);
	int barWidth = (int)ceil(barWidthD);
	int nBar;
	int nMidBar;
	int nDataPoint;
	double fRe;
	double fIm;
	double fPower;
	double fScale;

	fScale = 2.0 / nDataSize * GAIN;
	
	// Interpolate bars.
	nDataPoint = 0;
	for (nBar = 0; nBar < nNumBars; nBar++) {
		// Calculate the average.
		fPower = 0.0;
		for (nMidBar = 0; nMidBar < barWidth && nDataPoint < nDataSize; nDataPoint++, nMidBar++) {
			fRe = acFFTData[nDataPoint][0] * fScale;
			fIm = acFFTData[nDataPoint][1] * fScale;
			fPower += fRe * fRe + fIm * fIm;
		}
		fPower *= (1.0 / barWidth);
		if (fPower < 1e-15) {
			fPower = 1e-15;
		}

		// Compute the decibels
		int dB = SCALE + (int)(10.0 * log10 (fPower));
		if (dB > SCALE) {
			dB = SCALE;
		}
		if (dB < 0) {
			dB = 0;
		}

		// Set the bar height
		auBars[nBar] = dB;
	}
}

// This callback gets called when our context changes state. We really only
// care about when it's ready or if it has failed
void PulseAudioStateCallback (pa_context * psContext, void * psUserData) {
	AudioPersist * psAudioData = (AudioPersist *)psUserData;
	pa_context_state_t eState;
	
	eState = pa_context_get_state (psContext);
	switch (eState) {
		// These are just here for reference
		case PA_CONTEXT_UNCONNECTED:
		case PA_CONTEXT_CONNECTING:
		case PA_CONTEXT_AUTHORIZING:
		case PA_CONTEXT_SETTING_NAME:
		default:
			break;
		case PA_CONTEXT_FAILED:
		case PA_CONTEXT_TERMINATED:
			psAudioData->nPulseAudioState = 2;
			break;
		case PA_CONTEXT_READY:
			psAudioData->nPulseAudioState = 1;
			//RecordStart (SOURCE, psAudioData);
			break;
	}
}

static void PulseAudioStreamReadCallback (pa_stream * psRecordStream, size_t nLength, void * psUserData) {
	AudioPersist * psAudioData = (AudioPersist *)psUserData;
	char const * acStreamData;
	size_t nBytes;
	int nCopyPos;

	pa_stream_peek (psRecordStream, (void const **)(& acStreamData), & nBytes);

	if ((nBytes > 0) && (acStreamData != NULL)) {
		// Copy into buffer
		nCopyPos = 0;

		while (nCopyPos < nBytes) {
			((char *)psAudioData->pfBuffer)[psAudioData->nBufferPos] = acStreamData[nCopyPos];
			nCopyPos++;
			psAudioData->nBufferPos++;
			if (psAudioData->nBufferPos >= (psAudioData->nBufferSize * sizeof (float))) {
				AudioAnalyse (psAudioData);
				psAudioData->nBufferPos = 0;
			}
		}

		pa_stream_drop (psRecordStream);
	}
}

void AudioAnalyse (AudioPersist * psAudioData) {
	int nBar;

	if (psAudioData->psRecordStream && (!psAudioData->boPause)) {
		for (nBar = 0; nBar < psAudioData->nBufferSize; nBar++) {
			psAudioData->pfIn[nBar] = (double)(psAudioData->pfWindow[nBar] * psAudioData->pfBuffer[nBar]);
		}

		fftw_execute (psAudioData->sPlan);
		CalculateBars (psAudioData->pOut, psAudioData->nBufferSize, psAudioData->auBars, AUDIO_BARS);

		// Transfer to texture
		UpdateTextureSingleChannel (TEXNAME_AUDIO, AUDIO_BARS, 1, psAudioData->auBars, psAudioData->psTexData);
	}
#ifdef NCURSESBARS
	erase ();

	for (nBar = 0; nBar < AUDIO_BARS; nBar++) {
		move (LINES - psAudioData->auBars[nBar], nBar);
		vline (ACS_VLINE, psAudioData->auBars[nBar]);
	}
	
	refresh ();
#endif // NCURSESBARS
}

int GetAudioPower (int nBar, AudioPersist * psAudioData) {
	int nPower = 0;
	
	if ((nBar > 0) && (nBar < AUDIO_BARS)) {
		nPower = psAudioData->auBars[nBar];
	}
	return nPower;
}

void AudioGetSources (AudioPersist * psAudioData) {
	printf ("List sources\n");
	pa_context_get_source_info_list (psAudioData->psContext, AudioGetSourceCallback, psAudioData);
}

void AudioGetSourceCallback (pa_context * psContext, const pa_source_info * psSourceInfo, int nEol, void * psUserData) {
	//AudioPersist * psAudioData = (AudioPersist *)psUserData;
	if ((psSourceInfo) && (nEol <= 1)) {
		printf ("%d: \t%s\n", psSourceInfo->index, psSourceInfo->name);
	}
	else {
		printf ("Null\n");
	}
}

void AudioReturnSources (pa_source_info_cb_t pfCallback, void * psUserData, AudioPersist * psAudioData) {
	pa_context_get_source_info_list (psAudioData->psContext, pfCallback, psUserData);
}

pa_stream * AudioGetRecordStream (AudioPersist * psAudioData) {
	return psAudioData->psRecordStream;
}








// Test user function approximate
double AudioApproximate (double fVar1, void * psContext) {
	AudioPersist * psAudioData = (AudioPersist *)psContext;
	int nStart;
	int nEnd;
	float fInterpolate;
	float fAmplitude;
	float fFrequency;

	fFrequency = fmodf (fVar1 * AUDIO_BARS, AUDIO_BARS);
	nStart = (int)(fFrequency);
	nEnd = (nStart + 1) % AUDIO_BARS;
	fInterpolate = (fFrequency) - floor (fFrequency);

	fAmplitude = ((fInterpolate * psAudioData->auBars[nStart]) + ((1.0 - fInterpolate) * psAudioData->auBars[nEnd])) / 256.0;
	
	return fAmplitude;
}

// Test user function differentiate
Operation * AudioDifferentiate (Operation * psOp, Operation * psWRT, void * psContext) {
  Operation * psReturn = NULL;
  psReturn = CreateInteger (0);

	return psReturn;
}

// Test user function simplify
Operation * AudioSimplify (Operation * psOp, void * psContext) {
	Operation * psReturn = NULL;
	psReturn = psOp;

	return psReturn;
}





