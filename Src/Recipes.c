#include <PalmOS.h>
#include "Quartermaster.h"
#include "Quartermaster_Rsc.h"

/*********************************************************************
 * Internal variables
 *********************************************************************/

typedef struct {
    MemHandle namePtrs;      // dynamically allocated array of string pointers
    MemHandle nameStorage;    // single big memory block holding all recipe names
    UInt16 numRecipes;    // how many recipes loaded
} RecipeListContext;

static RecipeListContext ctx = {0};

/*********************************************************************
 * Internal functions
 *********************************************************************/

/***********************************************************************
 *
 * FUNCTION:     PopulateRecipeList
 *
 * DESCRIPTION:  Populates list containing names of all recipes in 
 *				 the recipe database
 *
 * PARAMETERS:   Pointer to list
 *
 * RETURNED:     Err
 *
 ***********************************************************************/
static Err PopulateRecipeList(ListType* list) {
 	UInt16 numRecords = DmNumRecords(gRecipeDB);
    Char *storagePtr;
    Char **namePtr;
    MemHandle recH;
    RecipeRecord *recP;
    UInt16 i;


    ctx.numRecipes = numRecords;
    ctx.namePtrs = MemHandleNew(numRecords * sizeof(Char *));
    
    if (!ctx.namePtrs) {
        return memErrNotEnoughSpace;
    }

    ctx.nameStorage = MemHandleNew(numRecords * 32);
    if (!ctx.nameStorage) {
        MemHandleFree(ctx.namePtrs);
        ctx.namePtrs = NULL;
        return memErrNotEnoughSpace;
    }

    storagePtr = MemHandleLock(ctx.nameStorage);
    namePtr = MemHandleLock(ctx.namePtrs);

    for (i = 0; i < numRecords; i++) {
        recH = DmQueryRecord(gRecipeDB, i);
        if (!recH) continue;

        recP = MemHandleLock(recH);

        StrNCopy(storagePtr, recP->name, 31);
        storagePtr[31] = '\0';
        namePtr[i] = storagePtr;
        storagePtr += 32;  // advance to next slot
        MemHandleUnlock(recH);
    }
    
    LstSetListChoices(list, namePtr, numRecords);
    LstDrawList(list);
    MemHandleUnlock(ctx.nameStorage);
    MemHandleUnlock(ctx.namePtrs);
    return errNone;
}

/***********************************************************************
 *
 * FUNCTION:     RecipeListDoButtonCommand
 *
 * DESCRIPTION:  Handles button presses on recipe list
 *
 * PARAMETERS:   command ID
 *
 * RETURNED:     handled boolean
 *
 ***********************************************************************/

static Boolean RecipeListDoButtonCommand(UInt16 command) {
    FormPtr frmP = FrmGetActiveForm();
	Boolean handled = false;
	Int16 selection;
    MemHandle recH;
    ListType* list;
	
	switch (command) {
	   	case RecipeListView:
	   		list = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, RecipeList));
	   		selection = LstGetSelection(list);
			if (selection != noListSelection) {
				recH = DmQueryRecord(gRecipeDB, selection);
				OpenRecipeForm(recH);
			}
	   		handled = true;
	   		break;
	   		
	   	case RecipeListDelete:
	   		list = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, RecipeList));
	   		selection = LstGetSelection(list);
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

/*********************************************************************
 * External functions
 *********************************************************************/

/***********************************************************************
 *
 * FUNCTION:     RecipeListHandleEvent
 *
 * DESCRIPTION:  Event handler for formRecipeList
 *
 * PARAMETERS:   Event
 *
 * RETURNED:     handled boolean
 *
 ***********************************************************************/

Boolean RecipeListHandleEvent(EventPtr eventP) {
   FormPtr frmP = FrmGetActiveForm();
   Boolean handled = false;
   ListType* list;

	switch (eventP->eType) {
		case frmOpenEvent:
			FrmDrawForm (frmP);
			list = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, RecipeList));
			PopulateRecipeList(list);
			handled = true;
			break;
			
		case ctlSelectEvent:
			return RecipeListDoButtonCommand(eventP->data.ctlSelect.controlID);
			break;
			
		case frmCloseEvent:
	        MemHandleFree(ctx.namePtrs);
	        ctx.namePtrs = NULL;
	        MemHandleFree(ctx.nameStorage);
       		ctx.nameStorage = NULL;
			break;
			
		case menuEvent: //Likely change later
			return MainFormDoCommand(eventP->data.menu.itemID);
			
		default:		
			break;
	}
	return handled;
}