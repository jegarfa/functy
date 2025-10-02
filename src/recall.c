///////////////////////////////////////////////////////////////////
// Recall
// File and memory management
//
// David Llewellyn-Jones
// http://www.flypig.co.uk
//
// Summer 2013
///////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////
// Includes

#include "recall.h"

#include <string.h>
#include <gtk/gtk.h>

///////////////////////////////////////////////////////////////////
// Defines

#define RECALL_MEMORY_BLOCKSIZE (10*1024)

///////////////////////////////////////////////////////////////////
// Structures and enumerations

typedef enum _RECALLMODE {
	RECALLMODE_INVALID = -1,

	RECALLMODE_MEMORY,
	RECALLMODE_FILEWRITE,

	RECALLMODE_NUM
} RECALLMODE;

typedef struct _RecallMemory {
	char * pStore;
	unsigned long int uAllocated;
	unsigned long int uFilled;
	unsigned long int uBlockSize;
	GString * szFilename;
} RecallMemory;

typedef struct _RecallFile {
	FILE * psFile;
} RecallFile;

struct _Recall {
	RECALLMODE eMode;

	// Type specific data
	union {
		RecallMemory * psMemory;
		RecallFile * psFile;
		void * psNone;
	} Store;

	// Virtual functions
	size_t (*recwrite) (const void * ptr, size_t size, size_t count, Recall * stream);
	int (*recprintf) (Recall * stream, const char * format, va_list arg);
	int (*recclose) (Recall * stream);
};

///////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////
// Function prototypes

void SetupRecall (const char * szFilename, Recall * psRecall);
void Memory_SetupRecall (const char * szFilename, Recall * psRecall);
void FileWrite_SetupRecall (const char * szFilename, const char * mode, Recall * psRecall);

size_t Memory_recwrite (const void * ptr, size_t size, size_t count, Recall * stream);
int Memory_recprintf (Recall * stream, const char * format, va_list arg);
int Memory_recclose (Recall * stream);
void Memory_realloc (unsigned long int uSize, Recall * psRecall);

size_t FileWrite_recwrite (const void * ptr, size_t size, size_t count, Recall * stream);
int FileWrite_recprintf (Recall * stream, const char * format, va_list arg);
int FileWrite_recclose (Recall * stream);

///////////////////////////////////////////////////////////////////
// Function definitions

Recall * recopen (const char * filename, const char * mode) {
	Recall * psRecall;
	RECALLMODE eMode;

	psRecall = g_new0 (Recall, 1);

	// Figure out the mode
	eMode = RECALLMODE_FILEWRITE;
	if (mode != NULL) {
		if (strchr (mode, 'm') != NULL) {
			eMode = RECALLMODE_MEMORY;
		}
	}

	psRecall->eMode = eMode;
	
	switch (eMode) {
		case RECALLMODE_MEMORY:
			Memory_SetupRecall (filename, psRecall);
			break;
		case RECALLMODE_FILEWRITE:
			FileWrite_SetupRecall (filename, mode, psRecall);
			break;
		default:
			SetupRecall (filename, psRecall);
			break;
	}
	
	return psRecall;	
}

void SetupRecall (const char * szFilename, Recall * psRecall) {
	psRecall->Store.psNone = NULL;
}

void Memory_SetupRecall (const char * szFilename, Recall * psRecall) {
	psRecall->Store.psMemory = g_new0 (RecallMemory, 1);

	psRecall->recwrite = Memory_recwrite;
	psRecall->recprintf = Memory_recprintf;
	psRecall->recclose = Memory_recclose;
	
	psRecall->Store.psMemory->pStore = NULL;
	psRecall->Store.psMemory->uAllocated = 0u;
	psRecall->Store.psMemory->uFilled = 0u;

	psRecall->Store.psMemory->uBlockSize = RECALL_MEMORY_BLOCKSIZE;

	if (szFilename != NULL) {
		psRecall->Store.psMemory->szFilename = g_string_new (szFilename);
	}
	else {
		psRecall->Store.psMemory->szFilename = g_string_new ("");
	}
}

void FileWrite_SetupRecall (const char * szFilename, const char * mode, Recall * psRecall) {
	psRecall->Store.psFile = g_new0 (RecallFile, 1);

	psRecall->recwrite = FileWrite_recwrite;
	psRecall->recprintf = FileWrite_recprintf;
	psRecall->recclose = FileWrite_recclose;

	psRecall->Store.psFile->psFile = fopen (szFilename, mode);
}

size_t recwrite (const void * ptr, size_t size, size_t count, Recall * stream) {
	size_t nResult;

	if (stream != NULL) {
		nResult = stream->recwrite (ptr, size, count, stream);
	}
	else {
		nResult = 0;
	}
	
	return nResult;
}

int recprintf (Recall * stream, const char * format, ...) {
	int nResult;

	if (stream != NULL) {
		va_list args;
		va_start (args, format);
		nResult = stream->recprintf (stream, format, args);
		va_end (args);
	}
	else {
		nResult = 0;
	}
	
	return nResult;
}

int recclose (Recall * stream) {
	int nResult;

	if (stream != NULL) {
		nResult = stream->recclose (stream);
	}
	else {
		nResult = 0;
	}

	g_free (stream);
	
	return nResult;
}

size_t Memory_recwrite (const void * ptr, size_t size, size_t count, Recall * stream) {
	unsigned long int uMemoryNeeded;
	size_t uResult;

	uMemoryNeeded = stream->Store.psMemory->uFilled + (size * count);

	Memory_realloc (uMemoryNeeded, stream);

	// Copy the data into the memory
	memcpy (stream->Store.psMemory->pStore + stream->Store.psMemory->uFilled, ptr, (size * count));
	stream->Store.psMemory->uFilled += (size * count);
	uResult = count;

	return uResult;
}

int Memory_recprintf (Recall * stream, const char * format, va_list arg) {
	int nLength;
	unsigned long int uMemoryNeeded;
	GString * szText;

	// Create a temporary string to store the result in
	szText = g_string_new ("");
	g_string_vprintf (szText, format, arg);
	nLength = szText->len;

	uMemoryNeeded = stream->Store.psMemory->uFilled + nLength;

	Memory_realloc (uMemoryNeeded, stream);

	// Store the new data
	memcpy (stream->Store.psMemory->pStore + stream->Store.psMemory->uFilled, szText->str, nLength);
	stream->Store.psMemory->uFilled += szText->len;

	// Free up the temporary copy of the data	
	g_string_free (szText, TRUE);

	return nLength;
}

int Memory_recclose (Recall * stream) {
	FILE * psFile;
	int nResult = 0;

	// Save out the data if a filename was provided
	if (stream->Store.psMemory->szFilename->len > 0) {
		psFile = fopen (stream->Store.psMemory->szFilename->str, "wb");
		if (psFile != NULL) {
			fwrite (stream->Store.psMemory->pStore, 1, stream->Store.psMemory->uFilled, psFile);
			nResult = fclose (psFile);
		}
	}

	if (stream->Store.psMemory->pStore != NULL) {
		g_free (stream->Store.psMemory->pStore);
		stream->Store.psMemory->pStore = NULL;
	}

	g_free (stream->Store.psMemory);
	stream->Store.psMemory = NULL;
	
	return nResult;	
}

void Memory_realloc (unsigned long int uSize, Recall * psRecall) {
	unsigned long int uAllocate;

	if (uSize > psRecall->Store.psMemory->uAllocated) {
		// Allocate more memory
		uAllocate = (((unsigned long int)(uSize / psRecall->Store.psMemory->uBlockSize)) + 1) * psRecall->Store.psMemory->uBlockSize;

		psRecall->Store.psMemory->pStore = g_realloc (psRecall->Store.psMemory->pStore, uAllocate);
		psRecall->Store.psMemory->uAllocated = uAllocate;
	}
}

size_t FileWrite_recwrite (const void * ptr, size_t size, size_t count, Recall * stream) {
	return fwrite (ptr, size, count, stream->Store.psFile->psFile);
}

int FileWrite_recprintf (Recall * stream, const char * format, va_list arg) {
	int nResult;

	nResult = vfprintf (stream->Store.psFile->psFile, format, arg);
	
	return nResult;
}

int FileWrite_recclose (Recall * stream) {
	int nResult;
	nResult = fclose (stream->Store.psFile->psFile);
	
	g_free (stream->Store.psFile);
	stream->Store.psFile = NULL;
	
	return nResult;
}

void * MemoryGetData (Recall * psRecall) {
	void * pAddress = NULL;
	
	if (psRecall->eMode == RECALLMODE_MEMORY) {
		pAddress = psRecall->Store.psMemory->pStore;
	}
	
	return pAddress;
}

unsigned long int MemoryGetSize (Recall * psRecall) {
	unsigned long int uSize = 0u;
	
	if (psRecall->eMode == RECALLMODE_MEMORY) {
		uSize = psRecall->Store.psMemory->uFilled;
	}
	
	return uSize;
}

void * MemoryDetachData (Recall * psRecall) {
	void * pAddress = NULL;
	
	if (psRecall->eMode == RECALLMODE_MEMORY) {
		pAddress = psRecall->Store.psMemory->pStore;
		psRecall->Store.psMemory->pStore = NULL;
		psRecall->Store.psMemory->uAllocated = 0u;
		psRecall->Store.psMemory->uFilled = 0u;
	}
	
	return pAddress;
}



