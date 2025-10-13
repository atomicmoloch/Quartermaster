#include <PalmOS.h>
#include "Quartermaster.h"
#include "Quartermaster_Rsc.h"

/*********************************************************************
 * Internal functions
 *********************************************************************/

/***********************************************************************
 *
 * FUNCTION:     DrawPantryList
 *
 * DESCRIPTION:  ListDrawFunction for pantry list (LstSetDrawFunction)
 *
 * PARAMETERS:   list index of item, drawing boundry
 *
 * RETURNED:     nothing
 *
 ***********************************************************************/
static void DrawPantryList(Int16 itemNum, RectanglePtr bounds, Char** data) {
	MemHandle pantryH;
	UInt32* pantryP;
	MemHandle ingredientH;
	Char* ingredientP;
	UInt16 index;

	if (itemNum >= DmNumRecords(gPantryDB)) return;
	
	pantryH = DmQueryRecord(gPantryDB, itemNum);
	
	if (!pantryH) return;
	
	pantryP = MemHandleLock(pantryH);
	
	if (DmFindRecordByID(gIngredientDB, *pantryP, &index) == errNone) {
		ingredientH = DmQueryRecord(gIngredientDB, index);
		if (ingredientH) {
			ingredientP = MemHandleLock(ingredientH);
			
			WinDrawTruncChars(
				ingredientP,
				StrLen(ingredientP),
				bounds->topLeft.x,
				bounds->topLeft.y,
				bounds->extent.x
			);
			MemHandleUnlock(ingredientH);
		}
	}
	MemHandleUnlock(pantryH);
} 

/***********************************************************************
 *
 * FUNCTION:     PantryDoCommand
 *
 * DESCRIPTION:  Handles pantry button and menu events
 *
 * PARAMETERS:   button/menu item id
 *
 * RETURNED:     handled boolean
 *
 ***********************************************************************/
static Boolean PantryDoCommand(UInt16 command) {
	FormPtr frmP;
	Boolean handled = false;
	ListType* lst;
	UInt16 selection;
	MemHandle results;
	UInt16 numResults;

	switch(command) {
		case PantryAdd:
			frmP = FrmGetActiveForm();
	   		lst = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, pantryOptionsList));
	   		selection = LstGetSelection(lst); 
			if (selection != noListSelection) {
				AddIdToDatabase(gPantryDB, IDFromIndex(gIngredientDB, selection));
				lst = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, pantryList));
				LstSetListChoices(lst, NULL, DmNumRecords(gPantryDB));
				LstDrawList(lst);
			}
			break;
		case PantryDelete:
			frmP = FrmGetActiveForm();
	   		lst = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, pantryList));
	   		selection = LstGetSelection(lst); 
			if (selection != noListSelection) {
				DmRemoveRecord(gPantryDB, selection); //maybe make more robust
				lst = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, pantryList));
				LstSetListChoices(lst, NULL, DmNumRecords(gPantryDB));
				LstDrawList(lst);
			}
			handled = true;
			break;
		case StrictSearch:
			numResults = PantryStrictSearch(&results);
			if (numResults > 0)
				OpenRecipeList(results, numResults);
			else
				displayCustomError(30);
			handled = true;
			break;
		case FuzzySearch:
			numResults = PantryFuzzySearch(&results);
			if (numResults > 0)
				OpenRecipeList(results, numResults);
			else
				displayCustomError(30);
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
 * FUNCTION:     PantryHandleEvent
 *
 * DESCRIPTION:  Event handler for pantry
 *
 * PARAMETERS:   EventPtr
 *
 * RETURNED:     handled boolean
 *
 ***********************************************************************/
Boolean PantryHandleEvent(EventPtr eventP) {
   FormPtr frmP;
   Boolean handled = false;
   ListType* lst;

	switch (eventP->eType) {
		case frmOpenEvent:
			frmP = FrmGetActiveForm();			
			FrmDrawForm (frmP);

			lst = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, pantryOptionsList));
			LstSetListChoices(lst, NULL, DmNumRecords(gIngredientDB));
			LstSetDrawFunction(lst, DrawIngredientList);
	    	LstDrawList(lst);
	    	LstSetSelection(lst, -1);

			lst = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, pantryList));
			LstSetListChoices(lst, NULL, DmNumRecords(gPantryDB));
			LstSetDrawFunction(lst, DrawPantryList);
	    	LstDrawList(lst);
	    	LstSetSelection(lst, -1);

			handled = true;
			break;
			
		case ctlSelectEvent:
			return PantryDoCommand(eventP->data.ctlSelect.controlID);

		case menuEvent: 
			return PantryDoCommand(eventP->data.menu.itemID);
			
		default:		
			break;
	}
	return handled;
}
