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
#include <png.h>

#include "exportsvx.h"
#include "filesave.h"

///////////////////////////////////////////////////////////////////
// Defines

#define TIMESTRING_LEN (32)
#define EDGE_CHECKS_TETRAHEDRON (6)

///////////////////////////////////////////////////////////////////
// Structures and enumerations

struct _ExportSVXPersist {
	GString * szFilename;
	int nResolution;
	int nSlice;
	struct zip * psZipArchive;
	VisPersist * psVisData;
};

///////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////
// Function prototypes

void FreePixelBufferSVX (guchar * pixels, gpointer data);
bool ExportManifestSVX (Recall * hFile, int nResolution, VisPersist * psVisData);
bool ExportPNGSVX (Recall * hFile, unsigned char * pcData, int nHeight, int nWidth, int nChannels, VisPersist * psVisData);
static void ExportPNGWriteCallbackSVX (png_structp psImage, png_bytep pcData, png_size_t nLength);
void RasteriseTriangleSVX (unsigned char * pcData, int nResolution, int nChannels, Vector3 const * pvV1, Vector3 const * pvV2, Vector3 const * pvV3);
void FilledTetrahedronSliceSVX (unsigned char * pcData, int nResolution, int nChannels, Vector3 * avVertices, float fZSlice);
void ReorderQuadVerticesSVX (Vector3 * avBounds, int nVertices);

///////////////////////////////////////////////////////////////////
// Function definitions

ExportSVXPersist * NewExportSVXPersist (char const * szFilename, int nResolution, VisPersist * psVisData) {
	ExportSVXPersist * psExportSVXData;

	psExportSVXData = g_new0 (ExportSVXPersist, 1);

	psExportSVXData->szFilename = g_string_new (szFilename);
	psExportSVXData->nResolution = nResolution;
	psExportSVXData->nSlice = 0;
	psExportSVXData->psZipArchive = NULL;
	psExportSVXData->psVisData = psVisData;

	return psExportSVXData;
}

void DeleteExportSVXPersist (ExportSVXPersist * psExportSVXData) {
	g_string_free (psExportSVXData->szFilename, TRUE);

	g_free (psExportSVXData);
}





// Callback for freeing the pixel buffer data
void FreePixelBufferSVX (guchar * pixels, gpointer data) {
	// Free the data
	g_free (pixels);
}

bool ExportStartSVX (LongPollPersist * psLongPollData, void * psData) {
	ExportSVXPersist * psExportSVXData = (ExportSVXPersist *)psData;
	bool boSuccess;
	unsigned long int uSize;
	Recall * hFile;
	int nError = 0;
	zip_uint64_t uErrorLength;
	char * szError;
	unsigned char * pcData;
	zip_int64_t nPosition;
	struct zip_source * pzZipSource;
	int nResult;

	boSuccess = TRUE;
	LongPollSetProgress (0.0f, psLongPollData);
	LongPollSetActivityMain ("Exporting SVX voxel model", psLongPollData);
	LongPollSetActivitySub (psLongPollData, "Initialising");

	psExportSVXData->nSlice = 0;

	AssignControlVarsToFunctionList (psExportSVXData->psVisData);

	// Check whether the file exists and delete it if it does
	nResult = access (psExportSVXData->szFilename->str, F_OK);
	if (nResult == 0) {
		nResult = remove (psExportSVXData->szFilename->str);
		if (nResult != 0) {
			printf ("Error deleting existing file.");
			boSuccess = FALSE;
		}
	}

	if (boSuccess) {
		// Create the archive
		psExportSVXData->psZipArchive = zip_open (psExportSVXData->szFilename->str, (ZIP_CREATE), & nError);

		if (psExportSVXData->psZipArchive == NULL) {
			uErrorLength = zip_error_to_str (NULL, 0, nError, errno);
			szError = g_malloc (uErrorLength + 1);
			uErrorLength = zip_error_to_str (szError, uErrorLength + 1, nError, errno);
			printf ("Error creating archive: %s\n", szError);
			g_free (szError);
			boSuccess = FALSE;
		}
		else {
			// Create the manifest file
			hFile = recopen (NULL, "m");
			ExportManifestSVX (hFile, psExportSVXData->nResolution, psExportSVXData->psVisData);

			uSize = MemoryGetSize (hFile);
			pcData = MemoryDetachData (hFile);

			if (pcData != NULL) {
				// Add memory to the archive as a file
				// This will be freed automatically once it's no longer needed
				pzZipSource = zip_source_buffer (psExportSVXData->psZipArchive, pcData, uSize, 1);

				nPosition = zip_add (psExportSVXData->psZipArchive, "manifest.xml", pzZipSource);

				if (nPosition < 0) {
					zip_source_free (pzZipSource);
					printf ("Error adding file: %s\n", zip_strerror (psExportSVXData->psZipArchive));
					boSuccess = FALSE;
				}
			}

			// We're done with our memory stream, so we can close it
			recclose (hFile);
		}
	}

	return boSuccess;
}

bool ExportStepSVX (LongPollPersist * psLongPollData, void * psData) {
	ExportSVXPersist * psExportSVXData = (ExportSVXPersist *)psData;
	bool boSuccess;
	GString * szFileIncrement;
	unsigned char * pcData;
	GSList const * psFuncList;
	int nX;
	int nY;
	int nChannel;
	int nChannels;
	FuncPersist * psFuncData;
	struct zip_source * pzZipSource;
	zip_int64_t nPosition;
	unsigned long int uSize;
	Recall * hFile;
	gchar * pcBuffer;
	GString * szFileNameFormat;
	float fProgress;

	boSuccess = TRUE;
	fProgress = (float)psExportSVXData->nSlice / (float)psExportSVXData->nResolution;
	LongPollSetProgress (fProgress, psLongPollData);
	LongPollSetActivitySub (psLongPollData, "Exporting slice %d", psExportSVXData->nSlice);

	nChannels = 1;

	// Create the format for the filename
	szFileNameFormat = g_string_new ("");
	// Format is a decimal integer with EXPORTVOXEL_SLICE_EXP digits padded with zeros, followed by .extension
	g_string_printf (szFileNameFormat, "density/slice%%0%dd.%s", EXPORTVOXEL_SLICE_EXP, "png");

	// Set the filename
	szFileIncrement = g_string_new ("");
	g_string_printf (szFileIncrement, szFileNameFormat->str, psExportSVXData->nSlice);

	// Create the memory to store the slice
	pcData = g_new (unsigned char, (psExportSVXData->nResolution * psExportSVXData->nResolution * nChannels));

	if (pcData != NULL) {
		// Clear the slice
		for (nX = 0; nX < psExportSVXData->nResolution; nX++) {
			for (nY = 0; nY < psExportSVXData->nResolution; nY++) {
				for (nChannel = 0; nChannel < nChannels; nChannel++) {
					pcData[(((nX + (nY * psExportSVXData->nResolution)) * nChannels) + nChannel)] = 0;
				}
			}
		}

		// Render slices for each function
		psFuncList = GetFunctionList (psExportSVXData->psVisData);
		while (psFuncList) {	
			psFuncData = (FuncPersist *)(psFuncList->data);
			// Render the function slice
			OutputVoxelSlice (pcData, psExportSVXData->nResolution, nChannels, psExportSVXData->nSlice, psFuncData);

			psFuncList = g_slist_next (psFuncList);
		}

		// Create a PNG image from the data
		// Gdk only supports RGB PNG output, but for SVX we need indexed (greyscale)
		// Hence we have to use libpng directly to create the PNG in memory
		hFile = recopen (NULL, "m");
		boSuccess = ExportPNGSVX (hFile, pcData, psExportSVXData->nResolution, psExportSVXData->nResolution, nChannels, psExportSVXData->psVisData);
		uSize = MemoryGetSize (hFile);
		pcBuffer = MemoryDetachData (hFile);

		if (pcBuffer != NULL) {
			// Add memory to the archive as a file
			// This will be freed automatically once it's no longer needed
			pzZipSource = zip_source_buffer (psExportSVXData->psZipArchive, pcBuffer, uSize, 1);

			nPosition = zip_add (psExportSVXData->psZipArchive, szFileIncrement->str, pzZipSource);

			if (nPosition < 0) {
				zip_source_free (pzZipSource);
				printf ("Error adding file: %s\n", zip_strerror (psExportSVXData->psZipArchive));
				boSuccess = FALSE;
			}
		}
		else {
			printf ("Failed to create PNG voxel slice.\n");
			boSuccess = FALSE;
		}
	
		// We're done with the PNG data, so we can close the stream
		recclose (hFile);
		//printf ("Output slice %d out of %d.\n", (psExportSVXData->nSlice + 1), psExportSVXData->nResolution);
		free (pcData);
	}
	else {
		printf ("Couldn't allocate memory for voxel slice.\n");
		boSuccess = FALSE;
	}

	g_string_free (szFileNameFormat, TRUE);
	g_string_free (szFileIncrement, TRUE);

	psExportSVXData->nSlice++;
	if (psExportSVXData->nSlice >= psExportSVXData->nResolution) {
		LongPollSetActivitySub (psLongPollData, "Compressing");
		LongPollDone (psLongPollData);
	}

	return boSuccess;
}

bool ExportFinishSVX (LongPollPersist * psLongPollData, void * psData) {
	ExportSVXPersist * psExportSVXData = (ExportSVXPersist *)psData;
	bool boSuccess;
	int nError = 0;

	boSuccess = TRUE;
	LongPollSetProgress (1.0f, psLongPollData);

	// Close the archive
	//printf ("Compressing (this may take some time).\n");
	nError = zip_close (psExportSVXData->psZipArchive);

	if (nError < 0) {
		printf ("Error closing archive: %s\n", zip_strerror (psExportSVXData->psZipArchive));
		boSuccess = FALSE;
	}

	DeleteExportSVXPersist (psExportSVXData);
	
	return boSuccess;
}

bool ExportCancelSVX (LongPollPersist * psLongPollData, void * psData) {
	ExportSVXPersist * psExportSVXData = (ExportSVXPersist *)psData;

	// Discard any changes
	zip_discard (psExportSVXData->psZipArchive);

	DeleteExportSVXPersist (psExportSVXData);

	return FALSE;
}




bool ExportManifestSVX (Recall * hFile, int nResolution, VisPersist * psVisData) {
	float fSize;
	double afRange[6];
	float fMinWidth;
	struct tm * pTm;
	time_t ulTime;
	char szTime[TIMESTRING_LEN];
	int nLength;

	GetVisRange (afRange, psVisData);
	fMinWidth = MIN (afRange[3], MIN (afRange[4], afRange[5]));
	fSize = fMinWidth / ((float)nResolution);

	recprintf (hFile, "<?xml version=\"1.0\"?>\n\n");

	recprintf (hFile, "<grid version=\"1.0\" gridSizeX=\"%d\" gridSizeY=\"%d\" gridSizeZ=\"%d\" voxelSize=\"%f\" subvoxelBits=\"%d\" slicesOrientation=\"%s\" >\n\n", nResolution, nResolution, nResolution, fSize, 8, "Y");

	recprintf (hFile, "\t<channels>\n");

	recprintf (hFile, "\t\t<channel type=\"DENSITY\" bits=\"8\" slices=\"density/slice%%04d.png\" />\n");

	recprintf (hFile, "\t</channels>\n\n");

	recprintf (hFile, "\t<material>\n");
	recprintf (hFile, "\t\t<material id=\"1\" urn=\"urn:shapeways:materials/1\" />\n");
	recprintf (hFile, "\t</material>\n\n");

	recprintf (hFile, "\t<metadata>\n");
	recprintf (hFile, "\t\t<entry key=\"generatedBy\" value=\"functy\" />\n");

	ulTime = (unsigned long)GetCurrentVisTime (psVisData);
	pTm = gmtime (& ulTime);
	nLength = strftime (szTime, TIMESTRING_LEN, "%c", pTm);
	if (nLength > 0) {
		recprintf (hFile, "\t\t<entry key=\"creationDate\" value=\"%s\" />\n", szTime);
	}

	recprintf (hFile, "\t</metadata>\n");

	recprintf (hFile, "</grid>\n");

	return TRUE;
}

bool ExportPNGSVX (Recall * hFile, unsigned char * pcData, int nHeight, int nWidth, int nChannels, VisPersist * psVisData) {
	png_structp psImage;
	png_infop psInfo;
	unsigned char ** apcRows;
	int nRow;
	
	psImage = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	psInfo = png_create_info_struct (psImage);

	if (setjmp (png_jmpbuf (psImage))) {
		png_destroy_write_struct (& psImage, & psInfo);
		return FALSE;
	}

	if (nChannels == 3) {
		png_set_IHDR (psImage, psInfo, nWidth, nHeight, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	}
	else {
		png_set_IHDR (psImage, psInfo, nWidth, nHeight, 8, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	}

	apcRows = (unsigned char **)g_new (unsigned char **, nWidth);
	for (nRow = 0; nRow < nHeight; nRow++) {
		apcRows[nRow] = pcData + (nWidth * nRow * nChannels);
	}

	png_set_rows (psImage, psInfo, apcRows);
	png_set_write_fn (psImage, hFile, ExportPNGWriteCallbackSVX, NULL);
	png_write_png (psImage, psInfo, PNG_TRANSFORM_IDENTITY, NULL);
	
	if (psInfo) {
		png_free_data (psImage, psInfo, PNG_FREE_ALL, -1);
	}
	if (psImage) {
		png_destroy_write_struct (& psImage, NULL);
	}
	if (apcRows) {
		g_free (apcRows);
	}

	return TRUE;
}

static void ExportPNGWriteCallbackSVX (png_structp psImage, png_bytep pcData, png_size_t nLength) {
	Recall * hFile;
	
	hFile = (Recall *)png_get_io_ptr (psImage);
	recwrite (pcData, sizeof(char), nLength, hFile);
}

// Intersects a plane with a cuboid to create a quadrilateral
// Use pixel coordinates for this call
// avCorners should be of size 8
void FilledCuboidSliceSVX (unsigned char * pcData, int nResolution, int nChannels, Vector3 * avCorners, float fZSlice) {
	Vector3 avVertices[4];
	static const int anTetrahedron[5][4] = {{7, 4, 3, 6}, {0, 1, 3, 4}, {5, 4, 6, 1}, {2, 1, 3, 6}, {4, 6, 1, 3}};
	int nTetrahedron;
	int nVertex;
	
	for (nTetrahedron = 0; nTetrahedron < 5; nTetrahedron++) {
		for (nVertex = 0; nVertex < 4; nVertex++) {
			avVertices[nVertex] = avCorners[anTetrahedron[nTetrahedron][nVertex]];
		}
		FilledTetrahedronSliceSVX (pcData, nResolution, nChannels, avVertices, fZSlice);
	}
}

// Intersects a plane with a tetrahedron to create a triangle or quadrilateral
// Use pixel coordinates for this call
// avCorners should be of size 4
void FilledTetrahedronSliceSVX (unsigned char * pcData, int nResolution, int nChannels, Vector3 * avVertices, float fZSlice) {
	Vector3 avBounds[EDGE_CHECKS_TETRAHEDRON];
	static const int nStart[EDGE_CHECKS_TETRAHEDRON] = {0, 0, 0, 1, 2, 3};
	static const int nEnd[EDGE_CHECKS_TETRAHEDRON] =   {1, 2, 3, 2 ,3, 1};
	int nCheck;
	int nFound;
	Vector3 vDirection;
	float fCross;

	// Cycle through the edges to establish whetheer and where they cross the plane	
	nFound = 0;
	for (nCheck = 0; nCheck < EDGE_CHECKS_TETRAHEDRON; nCheck++) {
		// Calculate whether the edge crosses the x-y plane at height fZSlide
		if (((avVertices[nStart[nCheck]].fZ < fZSlice) && (avVertices[nEnd[nCheck]].fZ >= fZSlice)) || ((avVertices[nStart[nCheck]].fZ >= fZSlice) && (avVertices[nEnd[nCheck]].fZ < fZSlice))) {
			// Calculate where they cross
			vDirection = SubtractVectors (& avVertices[nEnd[nCheck]], & avVertices[nStart[nCheck]]);
			fCross = (fZSlice - avVertices[nStart[nCheck]].fZ) / (avVertices[nEnd[nCheck]].fZ - avVertices[nStart[nCheck]].fZ);
			ScaleVectorDirect (& vDirection, fCross);
			// Store the result
			avBounds[nFound] = AddVectors (& vDirection, & avVertices[nStart[nCheck]]);
			nFound++;
		}
	}

	// We may need to reorder the vertices to ensure we render a simple rather than a complex (intersecting) quadrilateral	
	ReorderQuadVerticesSVX (avBounds, nFound);

	// Check whether we ended up with a quadrilateral or a triangle
	if (((nFound > 4) || (nFound < 3)) && (nFound > 0)) {
		// Something else
		printf ("Crossing points found (%d)\n", nFound);
	}
	if (nFound == 4) {
		// Render the quadrialteral as two triangles
		RasteriseTriangleSVX (pcData, nResolution, nChannels, & avBounds[0], & avBounds[1], & avBounds[2]);
		RasteriseTriangleSVX (pcData, nResolution, nChannels, & avBounds[0], & avBounds[2], & avBounds[3]);
	}
	if (nFound == 3) {
		// Render the trinble
		RasteriseTriangleSVX (pcData, nResolution, nChannels, & avBounds[0], & avBounds[1], & avBounds[2]);
	}
}

// Orders the vertices to create a simple rather than a complex quadrilateral 
void ReorderQuadVerticesSVX (Vector3 * avBounds, int nVertices) {
	int nVertex;
	Matrix2 mCoefficients;
	Vector2 vConstants;
	bool boInverted;
	Vector2 vResult;
	Vector3 vSwitch;

	// First check for doubles
	if (nVertices > 4) {
		printf ("There are %d vertices:\n", nVertices);
		for (nVertex = 0; nVertex < nVertices; nVertex++) {
			PrintVector (& avBounds[nVertex]);
		}
		printf ("\n");
	}
	
	if (nVertices == 4) {
		// Calculate the directions between corners
		mCoefficients.fA1 = avBounds[3].fX - avBounds[0].fX;
		mCoefficients.fA2 = avBounds[3].fY - avBounds[0].fY;
		mCoefficients.fB1 = avBounds[1].fX - avBounds[2].fX;
		mCoefficients.fB2 = avBounds[1].fY - avBounds[2].fY;

		// Calculate the difference in starting points
		vConstants.fX = avBounds[1].fX - avBounds[0].fX;
		vConstants.fY = avBounds[1].fY - avBounds[0].fY;

		// Solve the simultaneous equations if they can be
		boInverted = MatrixInvert2x2 (& mCoefficients, & mCoefficients);
		if (boInverted) {
			MultMatrix2x2Vector2 (& vResult, & mCoefficients, & vConstants);
			// Check whether the lines cross
			if ((vResult.fX > 0.0) && (vResult.fX < 1.0) && (vResult.fY > 0.0) && (vResult.fY < 1.0)) {
				// The lines cross, so we need to reorder the vertices
				vSwitch = avBounds[0];
				avBounds[0] = avBounds[1];
				avBounds[1] = vSwitch;
			}
		}
	}
}

// Use pixel coordinates for this call
void RasteriseTriangleSVX (unsigned char * pcData, int nResolution, int nChannels, Vector3 const * pvV1, Vector3 const * pvV2, Vector3 const * pvV3) {
	Vector3 const * apvCorner[3];
	Vector3 const * pvSwitch;
	int nX;
	int nY;
	float fXStart;
	float fXEnd;
	float fYStart;
	float fYMid;
	float fYEnd;
	float fXStartDelta;
	float fXEndDelta;
	int nChannel;
	
	// Re-order the vectors by y-value (lowest first)
	apvCorner[0] = pvV1;
	apvCorner[1] = pvV2;
	apvCorner[2] = pvV3;

	if (apvCorner[0]->fY > apvCorner[1]->fY) {
		apvCorner[0] = pvV2;
		apvCorner[1] = pvV1;
	}
	if (apvCorner[0]->fY > apvCorner[2]->fY) {
		apvCorner[2] = apvCorner[0];
		apvCorner[0] = pvV3;
	}
	if (apvCorner[1]->fY > apvCorner[2]->fY) {
		pvSwitch = apvCorner[2];
		apvCorner[2] = apvCorner[1];
		apvCorner[1] = pvSwitch;
	}

	// The triangle is split into two smaller trianges with a shared edge parallel to the x-axis
	// Rasterise the lower triangle
	// Start at the bottom and render upwards
	fXStart = round (apvCorner[0]->fX);
	fXEnd = fXStart;
	fYStart = floor (apvCorner[0]->fY);
	fYMid = round (apvCorner[1]->fY);
	fYEnd = ceil (apvCorner[2]->fY);

	// Trianges are rasterised a line at a time parallel to the x-axis
	// Calculate where the line should end
	// This is only needed in case there's no lower triangle to render
	if ((fYEnd - fYStart) != 0.0f) {
		fXEndDelta = (round (apvCorner[2]->fX) - round (apvCorner[0]->fX)) / (fYEnd - fYStart);
	}
	else {
		fXEndDelta = 1.0f;
	}

	if (abs (fYMid - fYStart) > 0.0f) {
		// Calculate the change in start and end position for the line (w.r.t. y)
		fXStartDelta = (round (apvCorner[1]->fX) - round (apvCorner[0]->fX)) / (fYMid - fYStart);
		fXEndDelta = (round (apvCorner[2]->fX) - round (apvCorner[0]->fX)) /  (fYEnd - fYStart);

		// Check whether we should render left-to-right or right-to-left
		if ((fXEndDelta - fXStartDelta) >= 0.0) {
			// Render left-to-right
			for (nY = fYStart; nY < fYMid; nY++) {
				// Update the start and end position of the line parallel to the x-axis
				fXStart += fXStartDelta;
				fXEnd += fXEndDelta;
				// Render a line
				for (nX = round (fXStart) - 1; nX <= round (fXEnd) + 1; nX++) {
					// Plot the point to the bitmap
					if ((nX >= 0) && (nX < nResolution) && (nY >= 0) && (nY < nResolution)) {
						// Render a pixel
						for (nChannel = 0; nChannel < nChannels; nChannel++) {
							pcData[(((nX + ((nResolution - nY - 1) * nResolution)) * nChannels) + nChannel)] = 255;
						}
					}
				}
			}
		}
		else {
			// Render right-to-left
			for (nY = fYStart; nY < fYMid; nY++) {
				// Update the start and end position of the line parallel to the x-axis
				fXStart += fXStartDelta;
				fXEnd += fXEndDelta;
				for (nX = round (fXStart) + 1; nX >= round (fXEnd) - 1; nX--) {
					// Plot the point to the bitmap
					if ((nX >= 0) && (nX < nResolution) && (nY >= 0) && (nY < nResolution)) {
						// Render a line
						for (nChannel = 0; nChannel < nChannels; nChannel++) {
							// Render a pixel
							pcData[(((nX + ((nResolution - nY - 1) * nResolution)) * nChannels) + nChannel)] = 255;
						}
					}
				}
			}
		}
	}

	// Rasterise the upper triangle
	// Start at the bottom and render upwards
	fXStart = round (apvCorner[1]->fX);
	fYStart = ceil (apvCorner[1]->fY);
	// fXEnd stays the same
	fYEnd = ceil (apvCorner[2]->fY);
	
	if (abs (fYEnd - fYMid) > 0.0f) {
		// Calculate the change in start position for the line (w.r.t. y)
		// The change in end position is the same as for the lower triangle
		fXStartDelta = (round (apvCorner[2]->fX) - round (apvCorner[1]->fX)) / (fYEnd - fYMid);

		// Check whether we should render left-to-right or right-to-left
		if (fXStart <= fXEnd) {
			// Render left-to-right
			for (nY = fYMid; nY < fYEnd; nY++) {
				// Update the start and end position of the line parallel to the x-axis
				fXStart += fXStartDelta;
				fXEnd += fXEndDelta;
				// Render a line
				for (nX = round (fXStart) - 1; nX <= round (fXEnd) + 1; nX++) {
					// Plot the point to the bitmap
					if ((nX >= 0) && (nX < nResolution) && (nY >= 0) && (nY < nResolution)) {
						// Render a pixel
						for (nChannel = 0; nChannel < nChannels; nChannel++) {
							pcData[(((nX + ((nResolution - nY - 1) * nResolution)) * nChannels) + nChannel)] = 255;
						}
					}
				}
			}
		}
		else {
			// Render right-to-left
			for (nY = fYMid; nY < fYEnd; nY++) {
				// Update the start and end position of the line parallel to the x-axis
				fXStart += fXStartDelta;
				fXEnd += fXEndDelta;
				// Render a line
				for (nX = round (fXStart) + 1; nX >= round (fXEnd) - 1; nX--) {
					// Plot the point to the bitmap
					if ((nX >= 0) && (nX < nResolution) && (nY >= 0) && (nY < nResolution)) {
						for (nChannel = 0; nChannel < nChannels; nChannel++) {
							// Render a pixel
							pcData[(((nX + ((nResolution - nY - 1) * nResolution)) * nChannels) + nChannel)] = 255;
						}
					}
				}
			}
		}
	}
}




