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

#include <stdlib.h>
#include <openvdb/openvdb.h>
#include <openvdb/tools/LevelSetUtil.h>
extern "C" {
#include "ovdbc.h"
} /* extern "C" */

#include <glib.h>

using namespace std;

///////////////////////////////////////////////////////////////////
// Defines

///////////////////////////////////////////////////////////////////
// Structures and enumerations

struct _OVDBCPersist {
	openvdb::FloatGrid::Ptr psGrid;
	openvdb::FloatGrid::Accessor sAccessor;
};

///////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////
// Function prototypes

///////////////////////////////////////////////////////////////////
// Function definitions

extern "C" {

OVDBCPersist * NewOVDBCPersist () {
	OVDBCPersist * psOVDBCData;

	psOVDBCData = g_new0 (OVDBCPersist, 1);

	openvdb::initialize ();
	psOVDBCData->psGrid = openvdb::FloatGrid::create (2.0);

	psOVDBCData->sAccessor = psOVDBCData->psGrid->getAccessor();
	
	return psOVDBCData;
}

void DeleteOVDBCPersist (OVDBCPersist * psOVDBCData) {
	g_free (psOVDBCData);
}

void OutputGrid (char const * const szFilename, OVDBCPersist * psOVDBCData) {
	psOVDBCData->psGrid->setTransform (openvdb::math::Transform::createLinearTransform (0.5));
	psOVDBCData->psGrid->setGridClass (openvdb::GRID_FOG_VOLUME);
	psOVDBCData->psGrid->setName ("LevelSetFuncty");
	//psOVDBCData->psGrid->pruneGrid();

	openvdb::io::File file (szFilename);
	openvdb::GridPtrVec vGrids;
	vGrids.push_back (psOVDBCData->psGrid);
	//openvdb::tools::sdfToFogVolume<openvdb::FloatGrid>(grid.operator*());
	file.write (vGrids);
	file.close ();

	// Destroy the vector and its contents	
	vGrids.clear();
}

void SetGridValueOVDBC (Vector3 const * const pvPos, OVDBCPersist * psOVDBCData) {
	openvdb::Coord sCoord;
	sCoord[0] = pvPos->fX;
	sCoord[1] = pvPos->fY;
	sCoord[2] = pvPos->fZ;

	psOVDBCData->sAccessor.setValue (sCoord, 1.0);
}

} /* extern "C" */

