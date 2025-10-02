///////////////////////////////////////////////////////////////////
// Functy
// 3D graph drawing utility
//
// David Llewellyn-Jones
// http://www.flypig.co.uk
//
// Autumn 2014
///////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////
// Includes

#include "longpoll.h"

///////////////////////////////////////////////////////////////////
// Defines

#define LONGPOLL_TIMEOUT (10)

///////////////////////////////////////////////////////////////////
// Structures and enumerations

struct _LongPollPersist {
	bool boDone;
	bool boCancelled;
	float fProgress;
	VisPersist * psVisData;
	GtkBuilder * psXML;

	// Context data
	void * psData;
	
	// Virtual functions
	LongPollCallback pmStart;
	LongPollCallback pmStep;
	LongPollCallback pmFinish;
	LongPollCallback pmCancel;
};

///////////////////////////////////////////////////////////////////
// Global variables

///////////////////////////////////////////////////////////////////
// Function prototypes

static gboolean LongPollTimeout (gpointer psData);
bool LongPollStart (LongPollPersist * psLongPollData);
void LongPollTimeoutDestroyNotify (gpointer psData);

///////////////////////////////////////////////////////////////////
// Function definitions

LongPollPersist * NewLongPollPersist (void) {
	LongPollPersist * psLongPollData;

	psLongPollData = g_new0 (LongPollPersist, 1);

	psLongPollData->boDone = FALSE;
	psLongPollData->boCancelled = FALSE;
	psLongPollData->fProgress = 0.0f;
	psLongPollData->psXML = NULL;
	psLongPollData->psData = NULL;

	psLongPollData->pmStart = NULL;
	psLongPollData->pmStep = NULL;
	psLongPollData->pmFinish = NULL;
	psLongPollData->pmCancel = NULL;
	
	return psLongPollData;
}

void DeleteLongPollPersist (LongPollPersist * psLongPollData) {
	g_free (psLongPollData);
}

bool LongPoll (GtkBuilder * psXML, void * psData, LongPollCallback pmStart, LongPollCallback pmStep, LongPollCallback pmFinish, LongPollCallback pmCancel) {
	LongPollPersist * psLongPollData;
	bool boSuccess;

	psLongPollData = NewLongPollPersist ();

	psLongPollData->psXML = psXML;
	psLongPollData->psData = psData;
	psLongPollData->pmStart = pmStart;
	psLongPollData->pmStep = pmStep;
	psLongPollData->pmFinish = pmFinish;
	psLongPollData->pmCancel = pmCancel;

	boSuccess = LongPollStart (psLongPollData);

	return boSuccess;
}

bool LongPollStart (LongPollPersist * psLongPollData) {
	GtkDialog * psDialog;
	GtkWindow * psParent;
	GtkWidget * psWidget;
	GtkWidget * psProgress;
	int nResult;
	bool boSuccess;

	boSuccess = TRUE;

	psParent = GTK_WINDOW (gtk_builder_get_object (psLongPollData->psXML, "MainWindow"));
	psDialog = GTK_DIALOG (gtk_builder_get_object (psLongPollData->psXML, "LongPollDialog"));

	gtk_window_set_transient_for (GTK_WINDOW (psDialog), psParent);

	psWidget = GTK_WIDGET (gtk_builder_get_object (psLongPollData->psXML, "ActivityMain"));
	gtk_label_set_text (GTK_LABEL (psWidget), "");
	psWidget = GTK_WIDGET (gtk_builder_get_object (psLongPollData->psXML, "ActivitySub"));
	gtk_label_set_text (GTK_LABEL (psWidget), "Working");
	psProgress = GTK_WIDGET (gtk_builder_get_object (psLongPollData->psXML, "LongPollProgress"));
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (psProgress), 0.0f);

	if (psLongPollData->pmStart != NULL) {
		boSuccess = psLongPollData->pmStart (psLongPollData, psLongPollData->psData);
	}

	/* int nTimeoutID = */ g_timeout_add_full (G_PRIORITY_DEFAULT_IDLE, LONGPOLL_TIMEOUT, LongPollTimeout, psLongPollData, LongPollTimeoutDestroyNotify);

	nResult = gtk_dialog_run (GTK_DIALOG (psDialog));

	gtk_widget_hide (GTK_WIDGET (psDialog));

	if ((nResult == 0) || (nResult == -4)) {
		boSuccess = FALSE;
		if (psLongPollData->pmCancel != NULL) {
			boSuccess = psLongPollData->pmCancel (psLongPollData, psLongPollData->psData);
		}
		psLongPollData->boCancelled = TRUE;
	}

	return boSuccess;
}

static gboolean LongPollTimeout (gpointer psData) {
	LongPollPersist * psLongPollData = (LongPollPersist *)psData;
	bool boContinue;
	GtkWidget * psProgress;
	GtkDialog * psDialog;
	bool boResult;
	
	boContinue = TRUE;
	boResult = TRUE;

	psProgress = GTK_WIDGET (gtk_builder_get_object (psLongPollData->psXML, "LongPollProgress"));
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (psProgress), psLongPollData->fProgress);

	boContinue = (!psLongPollData->boDone) && (!psLongPollData->boCancelled);

	if (!psLongPollData->boCancelled) {
		if (!psLongPollData->boDone) {
			if (psLongPollData->pmStep != NULL) {
				boResult = psLongPollData->pmStep (psLongPollData, psLongPollData->psData);
			}
		}
		else {
			if (psLongPollData->pmFinish != NULL) {
				boResult = psLongPollData->pmFinish (psLongPollData, psLongPollData->psData);
			}

			psDialog = GTK_DIALOG (gtk_builder_get_object (psLongPollData->psXML, "LongPollDialog"));
			gtk_dialog_response (psDialog, 1);
		}

		if (boResult == FALSE) {
			psDialog = GTK_DIALOG (gtk_message_dialog_new (NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Error occured during operation"));
			gtk_dialog_run (psDialog);
			gtk_widget_destroy (GTK_WIDGET (psDialog));

			psDialog = GTK_DIALOG (gtk_builder_get_object (psLongPollData->psXML, "LongPollDialog"));
			gtk_dialog_response (psDialog, -1);

			boContinue = FALSE;
		}
	}
	
	return boContinue;
}

void LongPollTimeoutDestroyNotify (gpointer psData) {
	LongPollPersist * psLongPollData = (LongPollPersist *)psData;

	DeleteLongPollPersist (psLongPollData);
}

void LongPollSetProgress (float fProgress, LongPollPersist * psLongPollData) {
	psLongPollData->fProgress = fProgress;
}

void LongPollDone (LongPollPersist * psLongPollData) {
	psLongPollData->boDone = TRUE;
}

void LongPollSetActivityMain (char const * const szText, LongPollPersist * psLongPollData) {
	GtkWidget * psWidget;

	psWidget = GTK_WIDGET (gtk_builder_get_object (psLongPollData->psXML, "ActivityMain"));
	gtk_label_set_text (GTK_LABEL (psWidget), szText);
}

void LongPollSetActivitySub (LongPollPersist * psLongPollData, const char * format, ...) {
	GtkWidget * psWidget;
	GString * szText;
	va_list args;

	va_start (args, format);
	// Create a temporary string to store the result in
	szText = g_string_new ("");
	g_string_vprintf (szText, format, args);
	va_end (args);

	psWidget = GTK_WIDGET (gtk_builder_get_object (psLongPollData->psXML, "ActivitySub"));
	gtk_label_set_text (GTK_LABEL (psWidget), szText->str);

	// Free up the temporary copy of the data	
	g_string_free (szText, TRUE);
}









