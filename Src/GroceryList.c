#include <PalmOS.h>
#include "Quartermaster.h"
#include "Quartermaster_Rsc.h"


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

		/*	lst = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, pantryList));
			LstSetListChoices(lst, NULL, DmNumRecords(gPantryDB));
			LstSetDrawFunction(lst, DrawPantryList);
	    	LstDrawList(lst);
	    	LstSetSelection(lst, -1); */

			handled = true;
			break;
			
		//case ctlSelectEvent:
		//	return PantryDoCommand(eventP->data.ctlSelect.controlID);

		case menuEvent: 
			return MainMenuDoCommand(eventP->data.menu.itemID);
			
		default:		
			break;
	}
	return handled;
}
