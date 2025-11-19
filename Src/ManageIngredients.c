#include <PalmOS.h>
#include "Quartermaster.h"
#include "Quartermaster_Rsc.h"

/*********************************************************************
 * Internal functions
 *********************************************************************/

/***********************************************************************
 *
 * FUNCTION:     ManageIngredientsDoCommand
 *
 * DESCRIPTION:  Handles button presses on ManageIngredients list
 *
 * PARAMETERS:   command
 *
 * RETURNED:     handled boolean
 *
 ***********************************************************************/
static Boolean ManageIngredientsDoCommand(UInt16 command) {
	FormPtr frmP;
	Boolean handled = false;
	ListType* lst;
	UInt16 selection;

	switch(command) {
		case IngredientAdd:
			frmP = FrmInitForm(formManualAddIngredient);
			FrmSetEventHandler(frmP, ManualAddIngredientHandleEvent);
			FrmSetActiveForm(frmP);
			FrmDrawForm(frmP);
			handled = true;
			break;

		case IngredientDelete:
			frmP = FrmGetActiveForm();
	   		lst = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, ingredientList));
	   		selection = LstGetSelection(lst); 
			if (selection != noListSelection) {
				displayErrorIf(RemoveIngredient(IDFromIndex(gIngredientDB, selection)));
				// is this too many chained functions lol
				lst = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, ingredientList));
				LstSetListChoices(lst, NULL, DmNumRecords(gIngredientDB));
		    	LstDrawList(lst);
		    	LstSetSelection(lst, -1);
			}
			handled = true;
			break;
	}
	return handled;
} 

/*********************************************************************
 * Public functions
 *********************************************************************/

/***********************************************************************
 *
 * FUNCTION:     ManualAddIngredientHandleEvent
 *
 * DESCRIPTION:  Event handler for add ingredient subform
 *
 * PARAMETERS:   Event
 *
 * RETURNED:     handled boolean
 *
 ***********************************************************************/
Boolean ManualAddIngredientHandleEvent(EventPtr eventP) {
	FormPtr frmP;
	Boolean handled = false;
	Boolean done = false;
	UInt16 numRecords = DmNumRecords(gRecipeDB);
    Char *nameP;
    UInt32 ingredientID;
    EventType event;


	if (eventP->eType == ctlSelectEvent) {
		switch (eventP->data.ctlSelect.controlID) {
			case cancelManualAddIngredient: 
				FrmReturnToForm(formManageIngredients);
				handled = true;
				break; 
			case saveManualAddIngredient: 
				frmP = FrmGetActiveForm();	
				nameP = FldGetTextPtr(FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, fieldManualAddIngredient)));
				if (nameP) {
					ingredientID = IngredientIDByName(nameP);
					if (ingredientID == -1) displayError(errAddingIngred);
				} else {
					displayError(errIngredNameBlank);
				}
				FrmReturnToForm(formManageIngredients);
			    MemSet(&event, sizeof(event), 0); // Ensures that list rerenders
			    event.eType = frmUpdateEvent;
			    event.data.frmUpdate.formID = formManageIngredients;
			    EvtAddEventToQueue(&event);
				handled = true;
				break;
		}
	}
	else if (eventP->eType == appStopEvent) {  
		FrmReturnToForm(formManageIngredients); // so that on app resumption it's back to a normal form
	}
	
	return handled;
} 

/***********************************************************************
 *
 * FUNCTION:     ManageIngredientsHandleEvent
 *
 * DESCRIPTION:  Event handler for manual ingredient management interface
 *
 * PARAMETERS:   EventPtr
 *
 * RETURNED:     handled boolean
 *
 ***********************************************************************/
Boolean ManageIngredientsHandleEvent(EventPtr eventP) {
   FormPtr frmP;
   Boolean handled = false;
   ListType* lst;

	switch (eventP->eType) {
		case frmOpenEvent:
			frmP = FrmGetActiveForm();			
			FrmDrawForm (frmP);

			lst = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, ingredientList));
			LstSetListChoices(lst, NULL, DmNumRecords(gIngredientDB));
			LstSetDrawFunction(lst, DrawIngredientList);
	    	LstDrawList(lst);
	    	LstSetSelection(lst, -1);

			handled = true;
			break;
			
		case frmUpdateEvent:
			frmP = FrmGetActiveForm();	
			lst = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, ingredientList));
			LstSetListChoices(lst, NULL, DmNumRecords(gIngredientDB));
	    	LstDrawList(lst);
	    	LstSetSelection(lst, -1);
	    	break;
			
		case ctlSelectEvent:
			return ManageIngredientsDoCommand(eventP->data.ctlSelect.controlID);

		case menuEvent: 
			return MainMenuDoCommand(eventP->data.menu.itemID);
			
		default:		
			break;
	}
	return handled;
}
