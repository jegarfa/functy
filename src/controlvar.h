///////////////////////////////////////////////////////////////////
// Functy
// 3D graph drawing utility
//
// David Llewellyn-Jones
// http://www.flypig.co.uk
//
// Autumn 2013
///////////////////////////////////////////////////////////////////

#ifndef CONTROLVAR_H
#define CONTROLVAR_H

///////////////////////////////////////////////////////////////////
// Includes

#include "symbolic.h"

///////////////////////////////////////////////////////////////////
// Defines

#define CONTROLVARS_MAX (10)

///////////////////////////////////////////////////////////////////
// Structures and enumerations

typedef struct _ControlvarPersist ControlvarPersist;

typedef struct _FnControlPersist {
	GSList * psControlvarList;
} FnControlPersist;

///////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////
// Function prototypes

FnControlPersist * NewFnControlPersist ();
void DeleteFnControlPersist (FnControlPersist * psFnControlData);

ControlvarPersist * NewControlvarPersist ();
void DeleteControlvarPersist (ControlvarPersist * psControlvarData);

void ClearControlvarList (FnControlPersist * psFnControlData);
void SetControlvarName (char const * const szName, ControlvarPersist * psControlvarData);
void SetControlvarValue (char const * const szValue, ControlvarPersist * psControlvarData);
char const * GetControlvarName (ControlvarPersist * psControlvarData);
char const * GetControlvarValueString (ControlvarPersist * psControlvarData);
double GetControlvarValueDouble (ControlvarPersist * psControlvarData);
gint ControlvarCompare (gconstpointer a, gconstpointer b);
bool AssignControlVarsToVariables (Variable * psVariables, FnControlPersist * psFnControlData);
void AddUndefinedControlVars (Variable * psVariables, FnControlPersist * psFnControlData);

///////////////////////////////////////////////////////////////////
// Function definitions

#endif /* CONTROLVAR_H */


