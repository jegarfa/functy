///////////////////////////////////////////////////////////////////
// VecSym
// Vector Symbolic structures
//
// David Llewellyn-Jones
// http://www.flypig.co.uk
//
// Spring 2012
///////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////
// Includes

#include "vecsym.h"

///////////////////////////////////////////////////////////////////
// Defines

///////////////////////////////////////////////////////////////////
// Structures and enumerations

///////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////
// Function prototypes

///////////////////////////////////////////////////////////////////
// Function definitions

VecSym3 * CreateVecSym3 (Operation * psOpX, Operation * psOpY, Operation * psOpZ) {
	VecSym3 * psResult;

	psResult = g_new(VecSym3, 1);
	psResult->psX = psOpX;
	psResult->psY = psOpY;
	psResult->psZ = psOpZ;

	return psResult;
}

VecSym3 * CreateFromStringsVecSym3 (char const * szOpX, char const * szOpY, char const * szOpZ) {
	VecSym3 * psResult;

	psResult = g_new(VecSym3, 1);
	psResult->psX = StringToOperation (szOpX);
	psResult->psY = StringToOperation (szOpY);
	psResult->psZ = StringToOperation (szOpZ);

	return psResult;
}

void DeleteVecSym3 (VecSym3 * psFree) {
	if (psFree) {
		FreeRecursive (psFree->psX);
		FreeRecursive (psFree->psY);
		FreeRecursive (psFree->psZ);
		g_free (psFree);
	}
}

VecSym3 * SetVecSym3 (VecSym3 * psResult, Operation * psOpX, Operation * psOpY, Operation * psOpZ) {
	if (psResult) {
		psResult->psX = psOpX;
		psResult->psY = psOpY;
		psResult->psZ = psOpZ;
	}
	return psResult;
}

VecSym3 * SetVecSym3Copy (VecSym3 * psResult, Operation * psOpX, Operation * psOpY, Operation * psOpZ) {
	if (psResult) {
		psResult->psX = CopyRecursive (psOpX);
		psResult->psY = CopyRecursive (psOpY);
		psResult->psZ = CopyRecursive (psOpZ);
	}
	return psResult;
}

VecSym3 * CopyVecSym3 (VecSym3 * pvOp) {
	VecSym3 * psResult;

	psResult = g_new(VecSym3, 1);
	psResult->psX = CopyRecursive (pvOp->psX);
	psResult->psY = CopyRecursive (pvOp->psY);
	psResult->psZ = CopyRecursive (pvOp->psZ);

	return psResult;
}

VecSym3 * AddVecSyms (VecSym3 const * pv1, VecSym3 const * pv2) {
	VecSym3 * psResult;

	psResult = g_new(VecSym3, 1);
	psResult->psX = CreateBinary (OPBINARY_ADD, pv1->psX, pv2->psX);
	psResult->psY = CreateBinary (OPBINARY_ADD, pv1->psY, pv2->psY);
	psResult->psZ = CreateBinary (OPBINARY_ADD, pv1->psZ, pv2->psZ);

	return psResult;
}

VecSym3 * CrossProdVecSyms (VecSym3 const * pv1, VecSym3 const * pv2) {
	VecSym3 * psResult;
	Operation * psLHS;
	Operation * psRHS;

	psResult = g_new(VecSym3, 1);
	
	psLHS = CreateBinary (OPBINARY_MUL, CopyRecursive (pv1->psY), CopyRecursive (pv2->psZ));
	psRHS = CreateBinary (OPBINARY_MUL, CopyRecursive (pv1->psZ), CopyRecursive (pv2->psY));
	psResult->psX = CreateBinary (OPBINARY_SUB, psLHS, psRHS);

	psLHS = CreateBinary (OPBINARY_MUL, CopyRecursive (pv1->psZ), CopyRecursive (pv2->psX));
	psRHS = CreateBinary (OPBINARY_MUL, CopyRecursive (pv1->psX), CopyRecursive (pv2->psZ));
	psResult->psY = CreateBinary (OPBINARY_SUB, psLHS, psRHS);

	psLHS = CreateBinary (OPBINARY_MUL, CopyRecursive (pv1->psX), CopyRecursive (pv2->psY));
	psRHS = CreateBinary (OPBINARY_MUL, CopyRecursive (pv1->psY), CopyRecursive (pv2->psX));
	psResult->psZ = CreateBinary (OPBINARY_SUB, psLHS, psRHS);

	return psResult;
}

VecSym3 * DifferentiateVecSym (VecSym3 const * pvDiff, Operation * psWRT) {
	VecSym3 * psResult;

	psResult = g_new(VecSym3, 1);
	psResult->psX = DifferentiateOperation (pvDiff->psX, psWRT);
	psResult->psY = DifferentiateOperation (pvDiff->psY, psWRT);
	psResult->psZ = DifferentiateOperation (pvDiff->psZ, psWRT);

	return psResult;
}

Variable * CreateVariablesVecSym (VecSym3 const * pvOp, Variable * psVariables) {
	psVariables = CreateVariables (pvOp->psX, psVariables);
	psVariables = CreateVariables (pvOp->psY, psVariables);
	psVariables = CreateVariables (pvOp->psZ, psVariables);
	
	return psVariables;
}

VecSym3 * UberSimplifyVecSym (VecSym3 * pvOp) {
	pvOp->psX = UberSimplifyOperation (pvOp->psX);
	pvOp->psY = UberSimplifyOperation (pvOp->psY);
	pvOp->psZ = UberSimplifyOperation (pvOp->psZ);
	
	return pvOp;
}

Vector3 ApproximateVecSym (VecSym3 * pvOp) {
	Vector3 vReturn;

	vReturn.fX = ApproximateOperation (pvOp->psX);
	vReturn.fY = ApproximateOperation (pvOp->psY);
	vReturn.fZ = ApproximateOperation (pvOp->psZ);

	return vReturn;
}

VecSym3 * NormalVecSyms (VecSym3 * pvOp) {
	VecSym3 * pvResult;
	Operation * psLength;

	pvResult = g_new(VecSym3, 1);
	psLength = CreateBinary (OPBINARY_POW, CreateBinary (OPBINARY_ADD, CreateBinary (OPBINARY_ADD, CreateBinary (OPBINARY_POW, CopyRecursive (pvOp->psX), CreateInteger (2)), CreateBinary (OPBINARY_POW, CopyRecursive (pvOp->psY), CreateInteger (2))), CreateBinary (OPBINARY_POW, CopyRecursive (pvOp->psZ), CreateInteger (2))), CreateBinary (OPBINARY_DIVIDE, CreateInteger (1), CreateInteger (2)));
	psLength = UberSimplifyOperation (psLength);

	pvResult->psX = CreateBinary (OPBINARY_DIVIDE, CopyRecursive (pvOp->psX), CopyRecursive (psLength));
	pvResult->psY = CreateBinary (OPBINARY_DIVIDE, CopyRecursive (pvOp->psY), CopyRecursive (psLength));
	pvResult->psZ = CreateBinary (OPBINARY_DIVIDE, CopyRecursive (pvOp->psZ), CopyRecursive (psLength));

	return pvResult;
}


