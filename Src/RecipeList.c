#include <PalmOS.h>
#include "Quartermaster.h"
#include "Quartermaster_Rsc.h"

/*********************************************************************
 * Internal variables
 *********************************************************************/

typedef struct {
    Char** namePtrs;      // dynamically allocated array of string pointers
    Char* nameStorage;    // single big memory block holding all recipe names
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
 	Char* storagePtr;
    MemHandle recH;
    MemPtr *recP;
    RecipeRecord recipe;
    UInt16 i;

	if (ctx.namePtrs) {
		MemPtrFree(ctx.namePtrs);
	    ctx.namePtrs = NULL;
	}
	if (ctx.nameStorage) { 
		MemPtrFree(ctx.nameStorage);
       	ctx.nameStorage = NULL;
    }
    
	if (numRecords == 0) {
		LstSetListChoices(list, NULL, 0);
		return errNone;
	}

    ctx.namePtrs    = MemPtrNew(numRecords * sizeof(Char *));
    ctx.nameStorage = MemPtrNew(numRecords * 16);
    storagePtr      = ctx.nameStorage;

    if (!ctx.namePtrs || !ctx.nameStorage) {
        return memErrNotEnoughSpace;
    }
    
    for (i = 0; i < numRecords; i++) {
        recH = DmQueryRecord(gRecipeDB, i);
        if (!recH) continue;

        recP = MemHandleLock(recH);
        recipe = RecipeGetRecord(recP);
        StrNCopy(storagePtr, recipe.name, 15);
        MemHandleUnlock(recH);
                
        storagePtr[15] = '\0';
        ctx.namePtrs[i] = storagePtr;
        storagePtr += StrLen(storagePtr) + 1; //max 16
    }
    
    MemPtrResize(ctx.nameStorage, (storagePtr - ctx.nameStorage));
    LstSetListChoices(list, ctx.namePtrs, numRecords);
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
    FormPtr frmP = FrmGetActiveForm();
	Boolean handled = false;
	Err err;
	Int16 selection;
    ListType* list;
	
	switch (command) {
	   	case RecipeListView:
	   		list = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, RecipeList));
	   		selection = LstGetSelection(list); //Should always be equivalent to the database index of selection
			if (selection != noListSelection) {
				OpenRecipeForm(selection);
			}
	   		handled = true;
	   		break;
	   		
	   	case RecipeListDelete:
	   		list = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, RecipeList));
	   		selection = LstGetSelection(list);
			if (selection != noListSelection) {
				if (confirmChoice(0)) {
					err = RemoveRecipe(selection);
					if (err != errNone) displayError(err); //Non-fatal error if delete fails
					err = PopulateRecipeList(list);
					if (err != errNone) displayError(err);
				}
			}
	   		handled = true;
	   		break;
	   		
	   	case RecipeListEdit:
	   		list = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, RecipeList));
	   		selection = LstGetSelection(list);
	   		if (selection != noListSelection) {
	   	    	err = OpenEditRecipeForm(selection, false);
	   	        if (err != errNone)
	   				displayError(err);
	   	    }
	   	    handled = true;
	   	    break;

	   	case RecipeListNew:
			OpenEditRecipeForm(0, true);
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
 * FUNCTION:     MainMenuDoCommand
 *
 * DESCRIPTION:  Event handler for main menu shared across several forms
 *
 * PARAMETERS:   command
 *
 * RETURNED:     handled boolean
 *
 ***********************************************************************/
Boolean MainMenuDoCommand(UInt16 command)
{
	FormType * frmP;
	Boolean handled = false;

	switch (command)
	{
		case OptionsAboutQuartermaster:
			MenuEraseStatus(0);

			frmP = FrmInitForm (AboutForm);
			FrmDoDialog (frmP);                    
			FrmDeleteForm (frmP);

			handled = true;
			break;
		case ViewRecipes:
			FrmGotoForm(formRecipeList);
			handled = true;
			break;	
		case ViewPantry:
			FrmGotoForm(formPantry);
			handled = true;
			break;	
	}

	return handled;
}

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
   FormPtr frmP;
   Boolean handled = false;
   Err err;
   ListType* list;

	switch (eventP->eType) {
		case frmOpenEvent:
			frmP = FrmGetActiveForm();
			FrmDrawForm (frmP);
			list = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, RecipeList));
			err  = PopulateRecipeList(list);
			if (err != errNone) displayError(err);
			handled = true;
			break;
			
		case ctlSelectEvent:
			return RecipeListDoButtonCommand(eventP->data.ctlSelect.controlID);
			break;
			
		case frmCloseEvent:
			if (ctx.namePtrs) {
				MemPtrFree(ctx.namePtrs);
			    ctx.namePtrs = NULL;
			}
			if (ctx.nameStorage) { 
				MemPtrFree(ctx.nameStorage);
		       	ctx.nameStorage = NULL;
		    }
			break;
			
		case menuEvent: //Likely change later
			return MainMenuDoCommand(eventP->data.menu.itemID);
			
		default:		
			break;
	}
	return handled;
}
