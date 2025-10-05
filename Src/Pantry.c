#include <PalmOS.h>
#include "Quartermaster.h"
#include "Quartermaster_Rsc.h"

typedef struct {
	Char** ingredientNames;
	Char*  ingredientStorage;
	
	Char** pantryNames;
	Char*  pantryStorage;
} PantryContext;

static PantryContext ctx;

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
	    ctx.ingredientNames    = MemPtrNew(numRecords * sizeof(Char *));
	    ctx.ingredientStorage = MemPtrNew(numRecords * 16);
	    storagePtr  = ctx.ingredientStorage;

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
	}
	
	return errNone;

}

static Err PantryDoCommand(UInt16 command) {
	FormPtr frmP;
	Boolean handled = false;
	ListType* lst;
	UInt16 selection;

	switch(command) {
		case PantryAdd:
			frmP = FrmGetActiveForm();
	   		lst = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, RecipeList));
	   		selection = LstGetSelection(lst); 
			if (selection != noListSelection) {
				AddIdToDatabase(gPantryDB, IDFromIndex(gPantryDB, selection));
			}
			break;
		case PantryDelete:
			handled = true;
			break;
	}
	return handled;
}


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
			if (ctx.pantryNames) {
				MemPtrFree(ctx.pantryNames);
				ctx.pantryNames = NULL;
			}
			if (ctx.pantryStorage) {
				MemPtrFree(ctx.pantryStorage);
				ctx.pantryStorage = NULL;
			}
			break;
			
		default:		
			break;
	}
	return handled;
}