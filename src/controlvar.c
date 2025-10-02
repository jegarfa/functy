///////////////////////////////////////////////////////////////////
// Functy
// 3D graph drawing utility
//
// David Llewellyn-Jones
// http://www.flypig.co.uk
//
// Autumn 2013
///////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////
// Includes

#include <symbolic.h>
#include <gtk/gtk.h>
#include <string.h>

#include "controlvar.h"

///////////////////////////////////////////////////////////////////
// Defines

///////////////////////////////////////////////////////////////////
// Structures and enumerations

struct _ControlvarPersist {
	Operation * psName;
	Operation * psValue;
	GString * szName;
	GString * szValue;
};

///////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////
// Function prototypes

void DeleteControlvarDataCallback (gpointer data, gpointer user_data);

///////////////////////////////////////////////////////////////////
// Function definitions

FnControlPersist * NewFnControlPersist () {
	FnControlPersist * psFnControlData;
	
	psFnControlData = g_new0 (FnControlPersist, 1);
	
	psFnControlData->psControlvarList = NULL;
	
	return psFnControlData;
}

void DeleteFnControlPersist (FnControlPersist * psFnControlData) {
	ClearControlvarList (psFnControlData);

	g_free (psFnControlData);
}

void ClearControlvarList (FnControlPersist * psFnControlData) {
	// Delete all of the control varaibles
	g_slist_foreach (psFnControlData->psControlvarList, DeleteControlvarDataCallback, NULL);
	g_slist_free (psFnControlData->psControlvarList);
	psFnControlData->psControlvarList = NULL;
}

void DeleteControlvarDataCallback (gpointer data, gpointer user_data) {
	DeleteControlvarPersist ((ControlvarPersist *)data);
}

ControlvarPersist * NewControlvarPersist () {
	ControlvarPersist * psControlvarData;

	psControlvarData = g_new0 (ControlvarPersist, 1);

	psControlvarData->psName = NULL;
	psControlvarData->psValue = NULL;

	psControlvarData->szName = g_string_new ("a");
	psControlvarData->szValue = g_string_new ("0.0");

	return psControlvarData;
}

void DeleteControlvarPersist (ControlvarPersist * psControlvarData) {

	FreeRecursive (psControlvarData->psName);
	FreeRecursive (psControlvarData->psValue);

	if (psControlvarData->szName) {
		g_string_free (psControlvarData->szName, TRUE);
	}
	if (psControlvarData->szValue) {
		g_string_free (psControlvarData->szValue, TRUE);
	}

	g_free (psControlvarData);
}

void SetControlvarName (char const * const szName, ControlvarPersist * psControlvarData) {
	g_string_assign (psControlvarData->szName, szName);

	psControlvarData->psName = StringToOperation (szName);
	psControlvarData->psName = UberSimplifyOperation (psControlvarData->psName);
}

void SetControlvarValue (char const * const szValue, ControlvarPersist * psControlvarData) {
	g_string_assign (psControlvarData->szValue, szValue);

	psControlvarData->psValue = StringToOperation (szValue);
	psControlvarData->psValue = UberSimplifyOperation (psControlvarData->psValue);
}

char const * GetControlvarName (ControlvarPersist * psControlvarData) {
	return psControlvarData->szName->str;
}

char const * GetControlvarValueString (ControlvarPersist * psControlvarData) {
	return psControlvarData->szValue->str;
}

double GetControlvarValueDouble (ControlvarPersist * psControlvarData) {
	double fValue;
	fValue = ApproximateOperation (psControlvarData->psValue);

	return fValue;
}

gint ControlvarCompare (gconstpointer a, gconstpointer b) {
	gint nResult;
	ControlvarPersist const * psA = (ControlvarPersist *)a;
	ControlvarPersist const * psB = (ControlvarPersist *)b;
	
	nResult = strcmp (psA->szName->str, psB->szName->str);

	return nResult;	
}

bool AssignControlVarsToVariables (Variable * psVariables, FnControlPersist * psFnControlData) {
	GSList * psToAssign;
	Variable * psVarFound;
	char const * szVarName;
	double fValue;
	bool boFound;

	boFound = FALSE;
	psToAssign = psFnControlData->psControlvarList;
	while (psToAssign) {
		szVarName = GetControlvarName ((ControlvarPersist *)psToAssign->data);

		psVarFound = FindVariable (psVariables, szVarName);
		
		if (psVarFound) {
			fValue = GetControlvarValueDouble ((ControlvarPersist *)psToAssign->data);
			SetVariable (psVarFound, fValue);
			boFound = TRUE;
		}

		psToAssign = g_slist_next (psToAssign);
	}
	
	return boFound;
}

void AddUndefinedControlVars (Variable * psVariables, FnControlPersist * psFnControlData) {
	Variable * psVarAdd;
	char const * szName;
	char const * szRequired = "xyzaprt";
	bool boAdd;
	int nCheck;
	ControlvarPersist * psControlvarData;
	GSList * psFind;

	psVarAdd = VariableFirst (psVariables);
	while (psVarAdd) {
		// Filter out the required variables
		szName = VariableName (psVarAdd);

		boAdd = TRUE;
		if (szName) {
			if (strlen (szName) == 1) {
				nCheck = 0;
				while ((szRequired[nCheck] != 0) && boAdd) {
					if (szRequired[nCheck] == szName[0]) {
						boAdd = FALSE;
					}
					nCheck++;
				}
			}
		}
		
		if (boAdd) {
			psControlvarData = NewControlvarPersist ();
			SetControlvarName (szName, psControlvarData);
			SetControlvarValue ("1", psControlvarData);
			
			psFind = g_slist_find_custom (psFnControlData->psControlvarList, psControlvarData, ControlvarCompare);
			
			if (psFind == NULL) {
				psFnControlData->psControlvarList = g_slist_append (psFnControlData->psControlvarList, psControlvarData);
			}
			else {
				DeleteControlvarPersist (psControlvarData);
			}
		}
	
		psVarAdd = VariableNext (psVarAdd);
	}
}

