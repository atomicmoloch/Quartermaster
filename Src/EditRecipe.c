#include <PalmOS.h>
#include "Quartermaster.h"
#include "Quartermaster_Rsc.h"

/*********************************************************************
 * Internal variables
 *********************************************************************/
typedef struct {        
    UInt16 recipeIndex;         
    Boolean isNew;             
    
    UInt8 numIngredients;
    UInt8 ingredientCounts[recipeMaxIngredients];
    UInt8 ingredientFracs[recipeMaxIngredients];
    UInt8 ingredientDenoms[recipeMaxIngredients];
    
    Char** ingredientNames;
    Char* ingredientStorage;
    UInt16 ingredientStorageSize;
    
    Char** unitNames;
    Char* unitStorage;
    UInt16 unitStorageSize;
    
	Char** namePtrs;   //Ingredient names for add ingredient form
	Char* nameStorage; //(only loaded while add ingredient form open)

} EditRecipeContext;

static EditRecipeContext ctx;

/*********************************************************************
 * Internal functions
 *********************************************************************/
 
/***********************************************************************
 *
 * FUNCTION:     saveRecipe
 *
 * DESCRIPTION:  Saves loaded recipe into recipe database
 *				 Deletes old entry and inserts new entry,
 *				 automatically cleaning up deleted ingredient/unit entries
 *
 * PARAMETERS:   Button id
 *
 * RETURNED:     handled boolean
 *
 ***********************************************************************/
static Err saveRecipe() {
	FormPtr frmP = FrmGetActiveForm();
	Char* name;
	Char* steps;
	Err err;
	
	name  = FldGetTextPtr(FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, EditRecipeName)));
	steps = FldGetTextPtr(FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, EditRecipeSteps)));
	steps = (steps != NULL) ? steps : "";

	if (!name) return dmErrInvalidParam;
	if (!ctx.isNew) RemoveRecipe(ctx.recipeIndex);	
	err = AddRecipe(name,
			 (const Char**)ctx.ingredientNames,
			 (const Char**)ctx.unitNames, 
			 ctx.numIngredients,
			 ctx.ingredientCounts, 
			 ctx.ingredientFracs, 
			 ctx.ingredientDenoms, 
			 steps);	 
	return err;
}

/***********************************************************************
 *
 * FUNCTION:     DeleteIngredient
 *
 * DESCRIPTION:  Deletes ingredient from recipe, 
 *				 shifting ingredient ids left
 *
 * PARAMETERS:   selection index
 *
 * RETURNED:     err
 *
 ***********************************************************************/
static Err DeleteIngredient(UInt16 index) {
    UInt16 i;

    if (index >= ctx.numIngredients)
        return dmErrIndexOutOfRange;

    for (i = index; i < ctx.numIngredients - 1; i++) {
        ctx.ingredientCounts[i] = ctx.ingredientCounts[i + 1];
        ctx.ingredientFracs[i]  = ctx.ingredientFracs[i + 1];
        ctx.ingredientDenoms[i] = ctx.ingredientDenoms[i + 1];
		
        ctx.ingredientNames[i]  = ctx.ingredientNames[i + 1];
        ctx.unitNames[i] = ctx.unitNames[i + 1];
    }

    ctx.numIngredients--;

    ctx.ingredientCounts[ctx.numIngredients] = 0;
    ctx.ingredientFracs[ctx.numIngredients]  = 0;
    ctx.ingredientDenoms[ctx.numIngredients] = 0;
    ctx.ingredientNames[ctx.numIngredients]  = NULL;
    ctx.unitNames[ctx.numIngredients]        = NULL;

    return errNone;
}



/***********************************************************************
 *
 * FUNCTION:     AddIngredientForm
 *
 * DESCRIPTION:  Initializes Add Ingredient Form
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     err
 *
 ***********************************************************************/
Err AddIngredientForm() {
	FormType *frmP;
	ListType *lst;
	Boolean handled = false;
	Boolean done = false;
	UInt16 numRecords = DmNumRecords(gIngredientDB);
	Char* storagePtr = NULL;
    MemHandle recH;
    Char* recP;
	UInt16 i;

	frmP = FrmInitForm(formAddIngredient);
	FrmSetEventHandler(frmP, AddIngredientHandleEvent);
	FrmSetActiveForm(frmP);
	FrmDrawForm(frmP);
	lst = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, listAddIngredient));
	
    
	if (numRecords == 0) {
		LstSetListChoices(lst, NULL, 0);
	} else {
	    ctx.namePtrs    = MemPtrNew(numRecords * sizeof(Char *));
	    ctx.nameStorage = MemPtrNew(numRecords * 16);
	    storagePtr  = ctx.nameStorage;

	    if (!ctx.namePtrs || !ctx.nameStorage) {
	    	if (ctx.namePtrs) MemPtrFree(ctx.namePtrs);
	        return memErrNotEnoughSpace;
	    }
	    
	    for (i = 0; i < numRecords; i++) {
	        recH = DmQueryRecord(gIngredientDB, i);
	        if (!recH) continue;

	        recP = MemHandleLock(recH);
	        StrNCopy(storagePtr, recP, 15);
	        MemHandleUnlock(recH);
	                
	        storagePtr[15] = '\0';
	        ctx.namePtrs[i] = storagePtr;
	        storagePtr += StrLen(storagePtr) + 1; //max 16
	    }
	    
	    MemPtrResize(ctx.nameStorage, (storagePtr - ctx.nameStorage));
	    LstSetListChoices(lst, ctx.namePtrs, numRecords);
	    LstDrawList(lst);
	}
	return errNone;
}


/***********************************************************************
 *
 * FUNCTION:     EditRecipeDoCommand
 *
 * DESCRIPTION:  Handles menu item and button presses 
 *
 * PARAMETERS:   Menu item/button id
 *
 * RETURNED:     handled boolean
 *
 ***********************************************************************/
static Boolean EditRecipeDoCommand(UInt16 command) {
	Boolean handled = false;
	FormPtr frmP;
    ListType *lst;
    UInt8 selection;
    Err err;
	
	switch (command) {
	   	case EditRecipeAddIng:
	   		if (ctx.numIngredients == recipeMaxIngredients) {
	   			displayCustomError(11);
	   		} else {
	   			/*frmP = FrmInitForm(formAddIngredient);
	   			selection = FrmDoDialog(frmP);
	   			FrmDeleteForm(frmP); */
	   			AddIngredientForm();
	   		}
	   		handled = true;
	   		break;
	   	case EditRecipeRemoveIng:
			frmP = FrmGetActiveForm();
	   		lst = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, EditRecipeIngredients));
	   		selection = LstGetSelection(lst); 
			if (selection != noListSelection) {
				err = DeleteIngredient(selection);
				if (err != errNone) displayError(err);
			}
			LstSetListChoices(lst, ctx.ingredientNames, ctx.numIngredients);
			LstDrawList(lst);
	   		handled = true;
	   		break;
	    case EditRecipeSave:
	   	case EditRecipeMenuSave:
	   		err = saveRecipe();
	   		if (err == dmErrInvalidParam)
	   			displayCustomError(10);
	   		else if (err != errNone)
	   			displayError(err);
	   		else
		   		FrmGotoForm(formRecipeList);
	   		handled = true;
	   		break;
	    case EditRecipeCancel:
	   	case EditRecipeMenuCancel:
	   		FrmGotoForm(formRecipeList);
	   		handled = true;
	   		break;	
		default:
			break;
 	} 
 	
 	return handled;
}

/*********************************************************************
 * External functions
 *********************************************************************/

/***********************************************************************
 *
 * FUNCTION:     AddIngredientHandleEvent
 *
 * DESCRIPTION:  Event handler for add ingredient subform
 *
 * PARAMETERS:   Event
 *
 * RETURNED:     handled boolean
 *
 ***********************************************************************/
Boolean AddIngredientHandleEvent(EventPtr eventP) {
	FormPtr frmP;
    FieldType *fld;
    ListType *lst;
	Boolean handled = false;
	Boolean done = false;
	UInt16 numRecords = DmNumRecords(gRecipeDB);
    MemHandle recH;
    Char* recP;
    Char *nameP, *unitP, *wholeP, *fracP, *denomP;
    UInt16 selection;
    EventType event;
    Err err;
    UInt16 i;


	if (eventP->eType == ctlSelectEvent) {
		switch (eventP->data.ctlSelect.controlID) {
			case AddIngredientCancel: 
				if (ctx.namePtrs) MemPtrFree(ctx.namePtrs);
				if (ctx.nameStorage) MemPtrFree(ctx.nameStorage);
				FrmReturnToForm(formEditRecipe);
				handled = true;
				break; 
			case AddIngredientSave: 
				frmP = FrmGetActiveForm();
				nameP  = FldGetTextPtr(FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, formAddName))); 
				unitP  = FldGetTextPtr(FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, formAddUnit)));
				wholeP = FldGetTextPtr(FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, formAddWhole))); 
				fracP  = FldGetTextPtr(FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, formAddFrac))); 
				denomP = FldGetTextPtr(FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, formAddDenom)));
				unitP = (unitP != NULL) ? unitP : "";
				
				if (nameP) {
					ctx.ingredientCounts[ctx.numIngredients] = wholeP ? StrAToI(wholeP) : 0;
					ctx.ingredientFracs[ctx.numIngredients]  = fracP  ? StrAToI(fracP)  : 0;
					ctx.ingredientDenoms[ctx.numIngredients] = denomP ? StrAToI(denomP) : 0;
					
					selection = (StrLen(nameP) < 32) ? StrLen(nameP) : 31;
					if (ctx.ingredientStorageSize == 0) {
						ctx.ingredientStorage = MemPtrNew(selection + 1);
						ctx.ingredientNames[ctx.numIngredients] = ctx.ingredientStorage;
						StrNCopy(ctx.ingredientStorage, nameP, selection);
						ctx.ingredientStorage[selection] = '\0';
						ctx.ingredientStorageSize = selection + 1;
					}
					else {
						recP = MemPtrNew(ctx.ingredientStorageSize + selection + 1);
						err = MemMove(recP, ctx.ingredientStorage, ctx.ingredientStorageSize);
						if (err != errNone) {
							displayError(err);
							if (recP) MemPtrFree(recP);
							if (ctx.namePtrs) MemPtrFree(ctx.namePtrs);
							if (ctx.nameStorage) MemPtrFree(ctx.nameStorage);
							FrmReturnToForm(formEditRecipe);
							break;
						}
						for (i = 0; i < ctx.numIngredients; i++) {
							ctx.ingredientNames[i] = (UInt32)(ctx.ingredientNames[i] - ctx.ingredientStorage) + recP;
						}
						StrNCopy(recP + ctx.ingredientStorageSize, nameP, selection);
						recP[ctx.ingredientStorageSize + selection] = '\0';
						ctx.ingredientNames[ctx.numIngredients] = recP + ctx.ingredientStorageSize;
						
						MemPtrFree(ctx.ingredientStorage);
						ctx.ingredientStorage = recP;
						ctx.ingredientStorageSize += selection + 1; 
					/* 	err = MemPtrResize(ctx.ingredientStorage, ctx.ingredientStorageSize + selection + 1);
						if (err != errNone) {
							displayError(err);
							if (ctx.namePtrs) MemPtrFree(ctx.namePtrs);
							if (ctx.nameStorage) MemPtrFree(ctx.nameStorage);
							FrmReturnToForm(formEditRecipe);
							break;
						}
						StrNCopy(ctx.ingredientStorage + ctx.ingredientStorageSize, nameP, selection);
						ctx.ingredientStorage[ctx.ingredientStorageSize + selection] = '\0';
						ctx.ingredientNames[ctx.numIngredients] = ctx.ingredientStorage + ctx.ingredientStorageSize;
						ctx.ingredientStorageSize += selection + 1; */
					}
					
					selection = (StrLen(unitP) < 32) ? StrLen(unitP) : 31;
					if (ctx.unitStorageSize == 0) {
						ctx.unitStorage = MemPtrNew(selection + 1);
						ctx.unitNames[ctx.numIngredients] = ctx.unitStorage;
						StrNCopy(ctx.unitStorage, unitP, selection);
						ctx.unitStorage[selection] = '\0';
						ctx.unitStorageSize = selection + 1;
					}
					else {
						recP = MemPtrNew(ctx.unitStorageSize + selection + 1);
						err = MemMove(recP, ctx.unitStorage, ctx.unitStorageSize);
						if (err != errNone) {
							displayError(err);
							if (recP) MemPtrFree(recP);
							if (ctx.namePtrs) MemPtrFree(ctx.namePtrs);
							if (ctx.nameStorage) MemPtrFree(ctx.nameStorage);
							FrmReturnToForm(formEditRecipe);
							break;
						}
						for (i = 0; i < ctx.numIngredients; i++) {
							ctx.unitNames[i] = (UInt32)(ctx.unitNames[i] - ctx.unitStorage) + recP;
						}
						StrNCopy(recP + ctx.unitStorageSize, unitP, selection);
						recP[ctx.unitStorageSize + selection] = '\0';
						ctx.unitNames[ctx.numIngredients] = recP + ctx.unitStorageSize;
						
						MemPtrFree(ctx.unitStorage);
						ctx.unitStorage = recP;
						ctx.unitStorageSize += selection + 1;
					}
					
					ctx.numIngredients++;
					if (ctx.namePtrs) MemPtrFree(ctx.namePtrs);
					if (ctx.nameStorage) MemPtrFree(ctx.nameStorage);
					FrmReturnToForm(formEditRecipe);
					
				    MemSet(&event, sizeof(event), 0);
				    event.eType = frmUpdateEvent;
				    event.data.frmUpdate.formID = formEditRecipe;
				    EvtAddEventToQueue(&event); //Note: EvtAddEventToQueue makes copy of passed event
				} else {
					displayCustomError(12);
				}
				handled = true;
				break; 
			case AddIngredientSelect: 
				frmP = FrmGetActiveForm();
				lst = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, listAddIngredient));
				selection = LstGetSelection(lst); 
				if (selection != noListSelection) { 
					recH = DmQueryRecord(gIngredientDB, selection); 
					recP = MemHandleLock(recH); 
					fld = (FieldType*)FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, formAddName)); 
					FldInsert(fld, recP, StrLen(recP)); 
					MemHandleUnlock(recH); 
				}
				handled = true;
				break;
		}
	}
	else if (eventP->eType == appStopEvent) { //maybe should handle formcloseevent instead? 
		if (ctx.namePtrs) MemPtrFree(ctx.namePtrs);
		if (ctx.nameStorage) MemPtrFree(ctx.nameStorage);
		FrmReturnToForm(formEditRecipe);
	}
	
	return handled;
} 

/***********************************************************************
 *
 * FUNCTION:     EditRecipeHandleEvent
 *
 * DESCRIPTION:  Event handler for formEditRecipe
 *
 * PARAMETERS:   Event
 *
 * RETURNED:     handled boolean
 *
 ***********************************************************************/
Boolean EditRecipeHandleEvent(EventPtr eventP) {
	FormPtr frmP;
	Boolean handled = false;
	MemHandle recipeH = NULL;
    MemPtr *recipeP = NULL;
    RecipeRecord recipe;
    FieldType *fld;
    ListType *lst;
    Char *steps;

	switch (eventP->eType) {
		case frmOpenEvent:
			frmP = FrmGetActiveForm();
			FrmDrawForm(frmP);
			
			if (!ctx.isNew) {
				recipeH = DmQueryRecord(gRecipeDB, ctx.recipeIndex);
				recipeP = MemHandleLock(recipeH);
				recipe = RecipeGetRecord(recipeP);
				MemHandleUnlock(recipeH);
				
				fld = (FieldType*)FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, EditRecipeName));
				FldInsert(fld, recipe.name, StrLen(recipe.name));
			
	        	fld = (FieldType*)FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, EditRecipeSteps));
	        	steps = RecipeGetStepsPtr(recipeP);
	        	FldInsert(fld, steps, StrLen(steps));

	        	lst = (ListType*)FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, EditRecipeIngredients));
			    LstSetListChoices(lst, ctx.ingredientNames, ctx.numIngredients);
			    LstDrawList(lst);
			    LstSetSelection(lst, -1);
			    
			}
			handled = true;
			break;
		
        case frmUpdateEvent:
       		frmP = FrmGetActiveForm();
        	lst = (ListType*)FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, EditRecipeIngredients));
			LstSetListChoices(lst, ctx.ingredientNames, ctx.numIngredients);
			LstDrawList(lst);
			LstSetSelection(lst, -1);
			break;
			
		case ctlSelectEvent:
			return EditRecipeDoCommand(eventP->data.ctlSelect.controlID);

		case menuEvent: 
			return EditRecipeDoCommand(eventP->data.menu.itemID);

		case frmCloseEvent:
			if (ctx.ingredientNames) {
				MemPtrFree(ctx.ingredientNames);
		       	ctx.ingredientNames = NULL;
		    }
			if (ctx.ingredientStorage) { 
				MemPtrFree(ctx.ingredientStorage);
		       	ctx.ingredientStorage = NULL;
		    }
			if (ctx.unitNames) {
				MemPtrFree(ctx.unitNames);
		       	ctx.unitNames = NULL;
		    }
			if (ctx.unitStorage) { 
				MemPtrFree(ctx.unitStorage);
		       	ctx.unitStorage = NULL;
		    }
			break;
			
		case keyDownEvent:
			frmP = FrmGetActiveForm();
			if (eventP->data.keyDown.chr == pageUpChr) {
				fld = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, EditRecipeSteps));
				if (FldScrollable(fld, winUp))
					FldScrollField(fld, 1, winUp);
			
			} else if (eventP->data.keyDown.chr == pageDownChr) {
				fld = FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, EditRecipeSteps));
				if (FldScrollable(fld, winDown))
					FldScrollField(fld, 1, winDown);
			}

		default:
			break;
	}
	return handled;
}

/***********************************************************************
 *
 * FUNCTION:     OpenEditRecipeForm
 *
 * DESCRIPTION:  Initializes edit recipe form context
 *				 Deconstructs recipe entry into component parts
 *
 * PARAMETERS:   recipeDB selection index, boolean if entry is new
 *
 * RETURNED:     Err
 *
 ***********************************************************************/
Err OpenEditRecipeForm(UInt16 selection, Boolean isNew) {    
    MemHandle recipeH = NULL;
	MemPtr recipeP = NULL;
	RecipeRecord recipe;
    Char buf[32];
    Char *storagePtr;
    UInt8 i;

    MemSet(&ctx, sizeof(EditRecipeContext), 0);
    ctx.isNew       = isNew;
    ctx.recipeIndex = selection;

    if (!isNew) {
        recipeH = DmQueryRecord(gRecipeDB, selection);
        if (recipeH) {
            recipeP = MemHandleLock(recipeH);
            recipe = RecipeGetRecord(recipeP);
            MemHandleUnlock(recipeH);
            ctx.numIngredients = recipe.numIngredients;
            
            if (ctx.numIngredients > 0) {
				MemMove(ctx.ingredientCounts,
					        recipe.ingredientCounts,
					        ctx.numIngredients * sizeof(UInt8));

				MemMove(ctx.ingredientFracs,
					        recipe.ingredientFracs,
					        ctx.numIngredients * sizeof(UInt8));

				MemMove(ctx.ingredientDenoms,
					        recipe.ingredientDenoms,
					        ctx.numIngredients * sizeof(UInt8));
	            
				ctx.ingredientNames   = MemPtrNew(sizeof(Char) * recipeMaxIngredients);
	    		ctx.ingredientStorage = MemPtrNew(recipe.numIngredients * 32);
	            storagePtr            = ctx.ingredientStorage;
	                
	   			if (!ctx.ingredientNames || !ctx.ingredientStorage)
	   				return memErrNotEnoughSpace;
	    			                
			    for (i = 0; i < recipe.numIngredients; i++) {
			        IngredientNameByID(buf, 32, recipe.ingredientIDs[i]);
			        StrNCopy(storagePtr, buf, 31);
					storagePtr[31] = '\0';
					ctx.ingredientNames[i] = storagePtr;
					storagePtr += StrLen(storagePtr) + 1; //max 32
				}
				    
				MemPtrResize(ctx.ingredientStorage, (storagePtr - ctx.ingredientStorage));
				ctx.ingredientStorageSize = storagePtr - ctx.ingredientStorage;
	                
				ctx.unitNames   = MemPtrNew(sizeof(Char) * recipeMaxIngredients);
				ctx.unitStorage = MemPtrNew(recipe.numIngredients * 32);
	            storagePtr      = ctx.unitStorage;
	                
	            if (!ctx.unitNames || !ctx.unitStorage)
	            	return memErrNotEnoughSpace;
	                
				for (i = 0; i < recipe.numIngredients; i++) {
			        UnitNameByID(buf, 32, recipe.ingredientUnits[i]);
					StrNCopy(storagePtr, buf, 31);
					storagePtr[31] = '\0';
					ctx.unitNames[i] = storagePtr;
					storagePtr += StrLen(storagePtr) + 1; //max 32
				}    
				MemPtrResize(ctx.unitStorage, (storagePtr - ctx.unitStorage));
				ctx.unitStorageSize = storagePtr - ctx.unitStorage;
			}
        }
    } 

    FrmGotoForm(formEditRecipe); 
    return errNone;   
}
