#include <PalmOS.h>
#include "Quartermaster.h"
#include "Quartermaster_Rsc.h"

/*********************************************************************
 * Internal variables
 *********************************************************************/

typedef struct {
	MemHandle results;
	UInt16 numResults;
} RecipeListContext;

static RecipeListContext ctx = {0};

/*********************************************************************
 * Internal functions
 *********************************************************************/
 

static Int16 TranslateIndex(Int16 index) {
	UInt16* resultP;

	if (ctx.results == NULL) {
		return index;
	} else {
		if (index >= ctx.numResults) return noListSelection;
		resultP = MemHandleLock(ctx.results);
		return resultP[index];
		MemHandleUnlock(ctx.results);
	}	
} 
 
 /***********************************************************************
 *
 * FUNCTION:     DrawRecipeList
 *
 * DESCRIPTION:  ListDrawFunction for recipe list (LstSetDrawFunction)
 *
 * PARAMETERS:   list index of item, drawing boundry
 *
 * RETURNED:     nothing
 *
 ***********************************************************************/
static void DrawRecipeList(Int16 itemNum, RectanglePtr bounds, Char** data) {
	UInt16* resultP;
	MemHandle nameH;
	Char* nameP;
	
	if (ctx.results == NULL) {
		if (itemNum >= DmNumRecords(gRecipeDB)) return;
	
        nameH = DmQueryRecord(gRecipeDB, itemNum);
        if (!nameH) return;
        
        nameP = MemHandleLock(nameH);
        
		WinDrawTruncChars(
			nameP,
			StrLen(nameP),
			bounds->topLeft.x,
			bounds->topLeft.y,
			bounds->extent.x
		);
		
		MemHandleUnlock(nameH);
	} else {
		if (itemNum >= ctx.numResults) return;
		
		resultP = MemHandleLock(ctx.results);
	
        nameH = DmQueryRecord(gRecipeDB, resultP[itemNum]);
        MemHandleUnlock(ctx.results);
        
        if (!nameH) return;
        
        nameP = MemHandleLock(nameH);
        
		WinDrawTruncChars(
			nameP,
			StrLen(nameP),
			bounds->topLeft.x,
			bounds->topLeft.y,
			bounds->extent.x
		);
		
		MemHandleUnlock(nameH);
	}
}

static Err PopulateRecipeList(ListType* lst) {
	if (ctx.results == NULL)
		LstSetListChoices(lst, NULL, DmNumRecords(gRecipeDB));
	else
		LstSetListChoices(lst, NULL, ctx.numResults);
	LstSetDrawFunction(lst, DrawRecipeList);
	LstDrawList(lst);
	LstSetSelection(lst, -1);
	
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
    ListType* list;
	Err err;
	
	switch (command) {
	   	case RecipeListView:
	   		list = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, RecipeList));
	   		selection = TranslateIndex(LstGetSelection(list)); 
			if (selection != noListSelection) {
				OpenRecipeForm(selection);
			}
	   		handled = true;
	   		break;
	   		
	   	case RecipeListDelete:
	   		list = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, RecipeList));
	   		selection = TranslateIndex(LstGetSelection(list));
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
	   		selection = TranslateIndex(LstGetSelection(list));
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
	   	    
	   	case RecipeListClear: // clear search results
			if (ctx.results) {
				MemHandleFree(ctx.results);
			    ctx.results = NULL;
			}
			ctx.numResults = 0;
			list = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, RecipeList));
			err  = PopulateRecipeList(list);
			if (err != errNone) displayError(err);	
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
   ListType* lst;

	switch (eventP->eType) {
		case frmOpenEvent:
			frmP = FrmGetActiveForm();
			FrmDrawForm (frmP);
			
			lst = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, RecipeList));
			err  = PopulateRecipeList(lst);
			if (err != errNone) displayError(err);	
			handled = true;	
			break;
			
		case ctlSelectEvent:
			return RecipeListDoButtonCommand(eventP->data.ctlSelect.controlID);
			break;
			
		case frmCloseEvent:
			if (ctx.results) {
				MemHandleFree(ctx.results);
			    ctx.results = NULL;
			}
			ctx.numResults = 0;
			break;
			
		case menuEvent: //Likely change later
			return MainMenuDoCommand(eventP->data.menu.itemID);
			
		default:		
			break;
	}
	return handled;
}

/***********************************************************************
 *
 * FUNCTION:     OpenRecipeList
 *
 * DESCRIPTION:  Opens and initializes RecipeList with the results of a search query
 *
 * PARAMETERS:   MemHandle to list of recipe indices
 *
 * RETURNED:     nothing
 *
 ***********************************************************************/
void OpenRecipeList(MemHandle results, UInt16 num) {
    ctx.results    = results;
    ctx.numResults = num;
    FrmGotoForm(formRecipeList);
}
