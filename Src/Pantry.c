#include <PalmOS.h>
#include "Quartermaster.h"
#include "Quartermaster_Rsc.h"

typedef struct {
	Char** ingredientNames;
	Char*  ingredientStorage;
} PantryContext;

static PantryContext ctx;

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
 * FUNCTION:     PopulateIngredientList
 *
 * DESCRIPTION:  Populates ingredient list with LstSetArrayChoices
 *				 (may replace)
 *
 * PARAMETERS:   Form pointer
 *
 * RETURNED:     err
 *
 ***********************************************************************/
static Err PopulateIngredientList(FormType *frmP) {
	ListType *lst;
	Boolean handled = false;
	Boolean done = false;
	UInt16 numRecords = DmNumRecords(gIngredientDB);
	Char* storagePtr = NULL;
    MemHandle recH;
    Char* recP;
	UInt16 i;

	frmP = FrmGetActiveForm();
	lst = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, optionsList));
	
    
	if (numRecords == 0) {
		LstSetListChoices(lst, NULL, 0);
	} else {
	    ctx.ingredientNames   = MemPtrNew(numRecords * sizeof(Char *));
	    ctx.ingredientStorage = MemPtrNew(numRecords * 16);
	    storagePtr            = ctx.ingredientStorage;

	    if (!ctx.ingredientNames || !ctx.ingredientStorage) {
	    	if (ctx.ingredientNames) MemPtrFree(ctx.ingredientNames);
	        return memErrNotEnoughSpace;
	    }
	    
	    for (i = 0; i < numRecords; i++) {
	        recH = DmQueryRecord(gIngredientDB, i);
	        if (!recH) continue;

	        recP = MemHandleLock(recH);
	        StrNCopy(storagePtr, recP, 15);
	        MemHandleUnlock(recH);
	                
	        storagePtr[15] = '\0';
	        ctx.ingredientNames[i] = storagePtr;
	        storagePtr += StrLen(storagePtr) + 1; //max 16
	    }
	    
	    MemPtrResize(ctx.ingredientStorage, (storagePtr - ctx.ingredientStorage));
	    LstSetListChoices(lst, ctx.ingredientNames, numRecords);
	    LstDrawList(lst);
	    LstSetSelection(lst, -1);
	}
	
	return errNone;

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
	   		lst = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, optionsList));
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
		case FuzzySearch:
			numResults = PantryFuzzySearch(&results);
			OpenRecipeList(results, numResults);
			handled = true;
			break;
	}
	return handled;
}

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
   Err err;
   ListType* lst;

	switch (eventP->eType) {
		case frmOpenEvent:
			frmP = FrmGetActiveForm();			
			FrmDrawForm (frmP);

			err = PopulateIngredientList(frmP);
			if (err != errNone) displayError(err);

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
			
		case frmCloseEvent:
			if (ctx.ingredientNames) {
				MemPtrFree(ctx.ingredientNames);
				ctx.ingredientNames = NULL;
			}
			if (ctx.ingredientStorage) {
				MemPtrFree(ctx.ingredientStorage);
				ctx.ingredientStorage = NULL;
			}
			break;
			
		default:		
			break;
	}
	return handled;
}
