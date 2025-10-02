///////////////////////////////////////////////////////////////////
// Functy
// 3D graph drawing utility
//
// David Llewellyn-Jones
// http://www.flypig.co.uk
//
// Summer 2012
///////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////
// Includes

#include <errno.h>
#include <zip.h>
#include <unistd.h>

#include "exportbitmap.h"
#include "filesave.h"
#include "spherical.h"
#include "curve.h"

///////////////////////////////////////////////////////////////////
// Defines

///////////////////////////////////////////////////////////////////
// Structures and enumerations

struct _ExportBitmapPersist {
	GString * szFilename;
	GString * szType;
	int nHeight;
	int nWidth;
	double fTimeStart;
	double fTimeEnd;
	int nFrames;

	struct zip * psZipArchive;
	double fTime;
	double fTimeIncrement;
	VisPersist * psVisData;
	int nFrame;	
};

///////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////
// Function prototypes

bool ExportBitmapFileMemory (char const * szFilename, gchar ** ppcBuffer, gsize * puSize, char const * szType, int nHeight, int nWidth, VisPersist * psVisData);
void FreePixelBuffer (guchar * pixels, gpointer data);

///////////////////////////////////////////////////////////////////
// Function definitions

ExportBitmapPersist * NewExportBitmapPersist (char const * szFilename, char const * szType, int nHeight, int nWidth, double fTimeStart, double fTimeEnd, int nFrames,  VisPersist * psVisData) {
	ExportBitmapPersist * psExportBitmapData;

	psExportBitmapData = g_new0 (ExportBitmapPersist, 1);

	psExportBitmapData->szFilename = g_string_new (szFilename);
	psExportBitmapData->szType = g_string_new (szType);
	psExportBitmapData->nHeight = nHeight;
	psExportBitmapData->nWidth = nWidth;
	psExportBitmapData->fTimeStart = fTimeStart;
	psExportBitmapData->fTimeEnd = fTimeEnd;
	psExportBitmapData->nFrames = nFrames;

	psExportBitmapData->psZipArchive = NULL;
	psExportBitmapData->fTime = 0.0f;
	psExportBitmapData->fTimeIncrement = 1.0f;
	psExportBitmapData->psVisData = psVisData;
	psExportBitmapData->nFrame = 0;

	return psExportBitmapData;
}

void DeleteExportBitmapPersist (ExportBitmapPersist * psExportBitmapData) {
	g_string_free (psExportBitmapData->szFilename, TRUE);
	g_string_free (psExportBitmapData->szType, TRUE);

	g_free (psExportBitmapData);
}




// Callback for freeing the pixel buffer data
void FreePixelBuffer (guchar * pixels, gpointer data) {
	// Free the data
	g_free (pixels);
}

bool ExportBitmapFile (char const * szFilename, char const * szType, int nHeight, int nWidth, VisPersist * psVisData) {
	return ExportBitmapFileMemory (szFilename, NULL, NULL, szType, nHeight, nWidth, psVisData);
}

// If ppcBuffer is provided then *ppsBuffer must be freed when no longer needed
bool ExportBitmapMemory (gchar ** ppcBuffer, gsize * puSize, char const * szType, int nHeight, int nWidth, VisPersist * psVisData) {
	return ExportBitmapFileMemory (NULL, ppcBuffer, puSize, szType, nHeight, nWidth, psVisData);
}

// Will output a bitmap to file, or memory, or both
// Simply provide a filename or a buffer as non-null inputs
// Providing both will generate both
// If ppcBuffer is provided then *ppsBuffer must be freed when no longer needed
bool ExportBitmapFileMemory (char const * szFilename, gchar ** ppcBuffer, gsize * puSize, char const * szType, int nHeight, int nWidth, VisPersist * psVisData) {
	bool boSuccess;
	int nScreenWidth;
	int nScreenHeight;
	GLuint uTexture;
	GLuint uRenderBuffer;
	GLuint uFrameBuffer;
	unsigned char * psData;
	GError * psError = NULL;
	GdkPixbuf * psImage;
	int nX;
	int nY;
	unsigned char cPixel;
	GLenum eSuccess;

	boSuccess = FALSE;

	nScreenWidth = GetScreenWidth (psVisData);
	nScreenHeight = GetScreenHeight (psVisData);

	Reshape (nWidth, nHeight, psVisData);

	// Create texture to store the bitmap knot image to
	glGenTextures (1, & uTexture);
	glBindTexture(GL_TEXTURE_2D, uTexture);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri (GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA8, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glBindTexture (GL_TEXTURE_2D, 0);

	// Create the render buffer to use as a depth buffer
	glGenRenderbuffers (1, & uRenderBuffer);
	glBindRenderbuffer (GL_RENDERBUFFER, uRenderBuffer);
	glRenderbufferStorage (GL_RENDERBUFFER, GL_DEPTH_COMPONENT, nWidth, nHeight);
	glBindRenderbuffer (GL_RENDERBUFFER, 0);

	// Create a framebuffer object to attach the texture and render buffer to
	glGenFramebuffers (1, & uFrameBuffer);
	glBindFramebuffer (GL_FRAMEBUFFER, uFrameBuffer);

	// Attach the texture (colour) and render buffer (depth)
	glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, uTexture, 0);
	glFramebufferRenderbuffer (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, uRenderBuffer);

	// Check if everything worked
	eSuccess = glCheckFramebufferStatus (GL_FRAMEBUFFER);
	if (eSuccess == GL_FRAMEBUFFER_COMPLETE) {
		// Use screen buffer
		glBindFramebuffer (GL_FRAMEBUFFER, 0);

		// Render to the framebuffer
		glBindFramebuffer (GL_FRAMEBUFFER, uFrameBuffer);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Draw the knot
		SetScreenBuffer (uFrameBuffer, psVisData);
		Redraw (psVisData);
		SetScreenBufferToScreen (psVisData);

		// Unbind frame buffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// Bind the texture so we can extract the data and save it to disk
		glBindTexture (GL_TEXTURE_2D, uTexture);

		// Create some space to store the texture data in
		psData = g_new (unsigned char, (nHeight * nWidth * 4));

		// Extract the data from the texture
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glGetTexImage (GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, psData);

		// We're done with the texture
		glBindTexture(GL_TEXTURE_2D, 0);

		// Flip the image
		for (nY = 0; nY < (nHeight / 2); nY++) {
			for (nX = 0; nX < (nWidth * 4); nX++) {
				cPixel = psData[(nY * nWidth * 4) + nX];
				psData[(nY * nWidth * 4) + nX] = psData[(((nHeight - 1) * nWidth * 4) - (nY * nWidth * 4)) + nX];
				psData[(((nHeight - 1) * nWidth * 4) - (nY * nWidth * 4)) + nX] = cPixel;
			}
		}

		// Convert the texture data into a Pixel Buffer object
		psImage = gdk_pixbuf_new_from_data (psData, GDK_COLORSPACE_RGB, TRUE, 8, nWidth, nHeight, nWidth * 4, FreePixelBuffer, NULL);
		if (psImage) {
			// Save the image to disk
			psError = NULL;

			if (szFilename) {
				gdk_pixbuf_save (psImage, szFilename, szType, & psError, NULL);
			}

			if (ppcBuffer) {
				gdk_pixbuf_save_to_buffer (psImage, ppcBuffer, puSize, szType, & psError, NULL);
			}

			if (psError) {
				printf ("%s", psError->message);
			}
			else {
				boSuccess = TRUE;
			}
		}
		g_object_unref (G_OBJECT (psImage));
		psData = NULL;
	}

	// Tidy things up
	glDeleteTextures (1, & uTexture);
	uTexture = 0;
	glDeleteFramebuffers (1, & uFrameBuffer);
	uFrameBuffer = 0;
	glDeleteRenderbuffers (1, & uRenderBuffer);
	uRenderBuffer = 0;

	// Ensure the screen image gets rendered at the correct size
	Reshape (nScreenWidth, nScreenHeight, psVisData);

	return boSuccess;
}

bool ExportStartAnimatedBitmap (LongPollPersist * psLongPollData, void * psData) {
	ExportBitmapPersist * psExportBitmapData = (ExportBitmapPersist *)psData;
	bool boSuccess;
	int nResult;
	int nError = 0;
	zip_uint64_t uErrorLength;
	char * szError;

	boSuccess = TRUE;
	LongPollSetProgress (0.0f, psLongPollData);
	LongPollSetActivityMain ("Exporting bitmap animation", psLongPollData);
	LongPollSetActivitySub (psLongPollData, "Initialising");

	// Check whether the file exists and delete it if it does
	nResult = access (psExportBitmapData->szFilename->str, F_OK);
	if (nResult == 0) {
		nResult = remove (psExportBitmapData->szFilename->str);
		if (nResult != 0) {
			printf ("Error deleting existing file.");
			boSuccess = FALSE;
		}
	}

	if (boSuccess) {
		// Create the archive
		//psZipArchive = zip_open (szFilename, (ZIP_CREATE | ZIP_TRUNCATE), & nError);
		psExportBitmapData->psZipArchive = zip_open (psExportBitmapData->szFilename->str, (ZIP_CREATE), & nError);

		if (psExportBitmapData->psZipArchive == NULL) {
			uErrorLength = zip_error_to_str (NULL, 0, nError, errno);
			szError = g_malloc (uErrorLength + 1);
			uErrorLength = zip_error_to_str (szError, uErrorLength + 1, nError, errno);
			printf ("Error creating archive: %s\n", szError);
			g_free (szError);
			boSuccess = FALSE;
		}
	
		psExportBitmapData->fTimeIncrement = (psExportBitmapData->fTimeEnd - psExportBitmapData->fTimeStart) / ((double)psExportBitmapData->nFrames);

		psExportBitmapData->fTime = psExportBitmapData->fTimeStart;
	}

	return boSuccess;
}

bool ExportStepAnimatedBitmap (LongPollPersist * psLongPollData, void * psData) {
	ExportBitmapPersist * psExportBitmapData = (ExportBitmapPersist *)psData;
	bool boSuccess;
	GString * szFileIncrement;
	GString * szFileNameFormat;
	GSList const * psFuncList;
	FuncPersist * psFuncData;
	FUNCTYPE eFuncType;
	gchar * pData;
	gsize uSize;
	struct zip_source * pzZipSource;
	zip_int64_t nPosition;
	bool boMemorySuccess;
	float fProgress;

	boSuccess = TRUE;

	fProgress = (float)psExportBitmapData->nFrame / (float)psExportBitmapData->nFrames;
	LongPollSetProgress (fProgress, psLongPollData);
	LongPollSetActivitySub (psLongPollData, "Exporting frame %d", psExportBitmapData->nFrame);

	// Create the format for the filename
	szFileNameFormat = g_string_new ("");
	// Format is a decimal integer with EXPORTANIM_FRAMES_EXP digits padded with zeros, followed by .extension
	g_string_printf (szFileNameFormat, "%%0%dd.%s", EXPORTANIM_FRAMES_EXP, psExportBitmapData->szType->str);
	szFileIncrement = g_string_new ("");

	// Set the filename
	g_string_printf (szFileIncrement, szFileNameFormat->str, psExportBitmapData->nFrame);

	// Set the time for all of the functions
	psFuncList = GetFunctionList (psExportBitmapData->psVisData);
	while (psFuncList) {	
		psFuncData = (FuncPersist *)(psFuncList->data);
		eFuncType = GetFunctionType (psFuncData);

		switch (eFuncType) {
		case FUNCTYPE_SPHERICAL:
			SphericalSetFunctionTime (psExportBitmapData->fTime, psFuncData);
			SphericalUpdateCentre (psFuncData);
			break;
		case FUNCTYPE_CURVE:
			CurveSetFunctionTime (psExportBitmapData->fTime, psFuncData);
			CurveUpdateCentre (psFuncData);
			break;
		default:
			// Do nothing
			break;
		}

		SetFunctionTime (psExportBitmapData->fTime, (FuncPersist *)(psFuncList->data));
		psFuncList = g_slist_next (psFuncList);
	}

	// Render the bitmap to memory
	boMemorySuccess = ExportBitmapMemory (& pData, & uSize, psExportBitmapData->szType->str, psExportBitmapData->nHeight, psExportBitmapData->nWidth, psExportBitmapData->psVisData);

	if (boMemorySuccess) {
		if (pData != NULL) {
			// Add memory to the archive as a file
			// This will be freed automatically once it's no longer needed
			pzZipSource = zip_source_buffer (psExportBitmapData->psZipArchive, pData, uSize, 1);

			nPosition = zip_add (psExportBitmapData->psZipArchive, szFileIncrement->str, pzZipSource);

			if (nPosition < 0) {
				zip_source_free (pzZipSource);
				printf ("Error adding file: %s\n", zip_strerror (psExportBitmapData->psZipArchive));
				boSuccess = FALSE;
			}
		}

		//printf ("Output frame %d out of %d.\n", (psExportBitmapData->nFrame + 1), psExportBitmapData->nFrames);
	}
	else {
		printf ("File couuld not be opened.\n");
		boSuccess = FALSE;
		
		if (pData != NULL) {
			// There's no attempt to add the data to the archive in this case
			// so we have to free it ourselves
			g_free (pData);
			pData = NULL;
		}
	}
				
	g_string_free (szFileIncrement, TRUE);
	g_string_free (szFileNameFormat, TRUE);

	psExportBitmapData->fTime += psExportBitmapData->fTimeIncrement;

	psExportBitmapData->nFrame++;
	if (psExportBitmapData->nFrame >= psExportBitmapData->nFrames) {
		LongPollSetActivitySub (psLongPollData, "Compressing");
		LongPollDone (psLongPollData);
	}

	return boSuccess;
}

bool ExportFinishAnimatedBitmap (LongPollPersist * psLongPollData, void * psData) {
	ExportBitmapPersist * psExportBitmapData = (ExportBitmapPersist *)psData;
	bool boSuccess;
	int nError = 0;

	boSuccess = TRUE;
	LongPollSetProgress (1.0f, psLongPollData);

	// Close the archive
	//printf ("Compressing (this may take some time).\n");
	nError = zip_close (psExportBitmapData->psZipArchive);

	if (nError < 0) {
		printf ("Error closing archive: %s\n", zip_strerror (psExportBitmapData->psZipArchive));
	}

	DeleteExportBitmapPersist (psExportBitmapData);

	return boSuccess;
}

bool ExportCancelAnimatedBitmap (LongPollPersist * psLongPollData, void * psData) {
	ExportBitmapPersist * psExportBitmapData = (ExportBitmapPersist *)psData;

	// Discard any changes
	zip_discard (psExportBitmapData->psZipArchive);

	DeleteExportBitmapPersist (psExportBitmapData);

	return FALSE;
}





