#include <PalmOS.h>
#include "Quartermaster.h"
#include "Quartermaster_Rsc.h"

/*********************************************************************
 * Internal functions
 *********************************************************************/

/***********************************************************************
 *
 * FUNCTION:     DrawGroceryList
 *
 * DESCRIPTION:  ListDrawFunction for grocery list (LstSetDrawFunction)
 *
 * PARAMETERS:   list index of item, drawing boundry
 *
 * RETURNED:     nothing
 *
 ***********************************************************************/
static void DrawGroceryList(Int16 itemNum, RectanglePtr bounds, Char** data) {
	MemHandle groceryH;
	UInt32* groceryP;
	MemHandle ingredientH;
	Char* ingredientP;
	UInt16 index;

	if (itemNum >= DmNumRecords(gGroceryDB)) return;
	
	groceryH = DmQueryRecord(gGroceryDB, itemNum);
	
	if (!groceryH) return;
	
	groceryP = MemHandleLock(groceryH);
	
	if (DmFindRecordByID(gIngredientDB, *groceryP, &index) == errNone) {
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
	MemHandleUnlock(groceryH);
}

static Boolean GroceryDoCommand(UInt16 command) {
	FormPtr frmP;
	Boolean handled = false;
	ListType* lst;
	UInt16 selection;
	UInt32 id;

	switch(command) {
		case GroceryAdd:
		// Add check to see if ingredient is in pantry and display notice
			frmP = FrmGetActiveForm();
	   		lst = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, groceryOptionsList));
	   		selection = LstGetSelection(lst); 
			if (selection != noListSelection) {
				id = IDFromIndex(gIngredientDB, selection);
				if (InDatabase(gPantryDB, id)) {
					if (FrmAlert(InPantryAlert) != 0) // alert if ingredient is in pantry
						return true; // exits early if cancel button chosen
				}
				AddIdToDatabase(gGroceryDB, id);
				lst = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, groceryList));
				LstSetListChoices(lst, NULL, DmNumRecords(gGroceryDB));
				LstDrawList(lst);
			}
			handled = true;
			break;

		case GroceryDelete:
			frmP = FrmGetActiveForm();
	   		lst = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, groceryList));
	   		selection = LstGetSelection(lst); 
			if (selection != noListSelection) {
				DmRemoveRecord(gGroceryDB, selection); //maybe make more robust
				lst = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, groceryList));
				LstSetListChoices(lst, NULL, DmNumRecords(gGroceryDB));
				LstDrawList(lst);
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
 * FUNCTION:     GroceryHandleEvent
 *
 * DESCRIPTION:  Event handler for grocery list
 *
 * PARAMETERS:   EventPtr
 *
 * RETURNED:     handled boolean
 *
 ***********************************************************************/
Boolean GroceryHandleEvent(EventPtr eventP) {
   FormPtr frmP;
   Boolean handled = false;
   ListType* lst;

	switch (eventP->eType) {
		case frmOpenEvent:
			frmP = FrmGetActiveForm();			
			FrmDrawForm (frmP);

			lst = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, groceryOptionsList));
			LstSetListChoices(lst, NULL, DmNumRecords(gIngredientDB));
			LstSetDrawFunction(lst, DrawIngredientList);
	    	LstDrawList(lst);
	    	LstSetSelection(lst, -1);

			lst = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, groceryList));
			LstSetListChoices(lst, NULL, DmNumRecords(gGroceryDB));
			LstSetDrawFunction(lst, DrawGroceryList);
	    	LstDrawList(lst);
	    	LstSetSelection(lst, -1); 

			handled = true;
			break;
			
		case ctlSelectEvent:
			return GroceryDoCommand(eventP->data.ctlSelect.controlID);

		case menuEvent: 
			return MainMenuDoCommand(eventP->data.menu.itemID);
			
		default:		
			break;
	}
	return handled;
}
