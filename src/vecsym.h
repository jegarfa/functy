///////////////////////////////////////////////////////////////////
// VecSym
// Vector Symbolic structures
//
// David Llewellyn-Jones
// http://www.flypig.co.uk
//
// Spring 2012
///////////////////////////////////////////////////////////////////

#ifndef VECSYM_H
#define VECSYM_H

///////////////////////////////////////////////////////////////////
// Includes

#include "utils.h"

#include <symbolic.h>

///////////////////////////////////////////////////////////////////
// Defines

///////////////////////////////////////////////////////////////////
// Structures and enumerations

typedef struct _VecSym3 {
	union {
		struct {
			Operation * psX;
			Operation * psY;
			Operation * psZ;
		};
		Operation * apsV[3];
	};
} VecSym3;

///////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////
// Function prototypes

VecSym3 * CreateVecSym3 (Operation * psOpX, Operation * psOpY, Operation * psOpZ);
VecSym3 * CreateFromStringsVecSym3 (char const * szOpX, char const * szOpY, char const * szOpZ);
void DeleteVecSym3 (VecSym3 * psFree);
VecSym3 * CopyVecSym3 (VecSym3 * pvOp);
VecSym3 * SetVecSym3 (VecSym3 * psResult, Operation * psOpX, Operation * psOpY, Operation * psOpZ);
VecSym3 * SetVecSym3Copy (VecSym3 * psResult, Operation * psOpX, Operation * psOpY, Operation * psOpZ);
VecSym3 * AddVecSyms (VecSym3 const * pv1, VecSym3 const * pv2);
VecSym3 * CrossProdVecSyms (VecSym3 const * pv1, VecSym3 const * pv2);
VecSym3 * DifferentiateVecSym (VecSym3 const * pvDiff, Operation * psWRT);
Variable * CreateVariablesVecSym (VecSym3 const * pvOp, Variable * psVariables);
VecSym3 * UberSimplifyVecSym (VecSym3 * pvOp);
Vector3 ApproximateVecSym (VecSym3 * pvOp);
VecSym3 * NormalVecSyms (VecSym3 * pvOp);

///////////////////////////////////////////////////////////////////
// Function definitions

#endif /* VECSYM_H */

