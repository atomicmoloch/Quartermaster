#include <PalmOS.h>
#include "Quartermaster.h"
#include "Quartermaster_Rsc.h"

/*********************************************************************
 * Internal variables
 *********************************************************************/

typedef struct {
    Char **namePtrs;      // dynamically allocated array of string pointers
    Char *nameStorage;    // single big memory block holding all recipe names
    UInt16 numRecipes;    // how many recipes loaded
} RecipeListContext;

static RecipeListContext recipeListCtx = {0};

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
    UInt16 i;
    MemHandle recH;
    RecipeRecord *recP;
    Char *writePtr;

    recipeListCtx.numRecipes = numRecords;
    recipeListCtx.namePtrs = MemPtrNew(numRecords * sizeof(Char *));
    
    if (!recipeListCtx.namePtrs) {
        return memErrNotEnoughSpace;
    }

    recipeListCtx.nameStorage = MemPtrNew(numRecords * 32);
    if (!recipeListCtx.nameStorage) {
        MemPtrFree(recipeListCtx.namePtrs);
        recipeListCtx.namePtrs = NULL;
        return memErrNotEnoughSpace;
    }

    writePtr = recipeListCtx.nameStorage;

    for (i = 0; i < numRecords; i++) {
        recH = DmQueryRecord(gRecipeDB, i);
        if (!recH) continue;

        recP = MemHandleLock(recH);

        StrNCopy(writePtr, recP->name, 31);
        writePtr[31] = '\0';

        recipeListCtx.namePtrs[i] = writePtr;
        writePtr += 32;  // advance to next slot

        MemHandleUnlock(recH);
    }

    LstSetListChoices(list, (Char **)recipeListCtx.namePtrs, numRecords);
    LstDrawList(list);
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
	Boolean handled = false;
	Int16 selection;
    MemHandle recH;
    ListType* list;
    FormPtr frmP = FrmGetActiveForm();
	
	switch (command) {
	   	case btnViewRecipe:
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
   Boolean handled = false;
   FormPtr frmP = FrmGetActiveForm();
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
	        MemPtrFree(recipeListCtx.namePtrs);
	        recipeListCtx.namePtrs = NULL;
	        MemPtrFree(recipeListCtx.nameStorage);
       		recipeListCtx.nameStorage = NULL;
			break;
			
		case menuEvent: //Likely change later
			return MainFormDoCommand(eventP->data.menu.itemID);
			
		default:		
			break;
	}
	return handled;
}