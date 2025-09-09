#include <PalmOS.h>
#include "Quartermaster.h"
#include "Quartermaster_Rsc.h"


void PopulateRecipeList(LstPtr list) {
    UInt16 numRecords = DmNumRecords(gRecipeDB);
    MemHandle recH;
    RecipeRecord *recP;

    // Clear any existing items in the list
    LstEraseList(list);

    for (UInt16 i = 0; i < numRecords; i++) {
        recH = DmQueryRecord(gRecipeDB, i);
        if (!recH) continue;

        recP = MemHandleLock(recH);

        // Add recipe name to list
        LstAppendItem(list, recP, false);

        MemHandleUnlock(recH);
    }
}

Boolean RecipeListDoButtonCommand(UInt16 command) {
	Boolean handled = false;
	Int16 selection;
    MemHandle recH;
    RecipeRecord *recP;
	
	switch (command) {
	   	case btnViewRecipe:
	   		selection = LstGetSelection(RecipeList);
			if (selection != noListSelection) {
				recH = DmQueryRecord(gRecipeDB, selection);
				OpenRecipeForm(recH);
			}
	   		handled = true;
	   		break;

		default:
			break;
 	}

   return (handled);
}

Boolean RecipeListHandleEvent(EventPtr eventP) {
   Boolean handled = false;
   FormPtr frmP = FrmGetActiveForm();

	switch (eventP->eType) {
		case frmOpenEvent:
			FrmDrawForm (frmP);
			LstPtr list = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, RecipeList));
			PopulateRecipeList(list);
			handled = true;
			break;
			
		case ctlSelectEvent:
			return RecipeListDoButtonCommand(eventP->data.ctlSelect.controlID);
			break;
			
		default:		
			break;
	}
	return handled;
}