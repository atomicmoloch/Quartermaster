#include <PalmOS.h>
#include "Quartermaster.h"
#include "Quartermaster_Rsc.h"

/*********************************************************************
 * Internal variables
 *********************************************************************/

// Stores recipe handle and scroll information
typedef struct {
    MemHandle recipe;  	   // pointer to the recipe to display
    Int16 scrollPos;       // vertical scroll position in pixels
 	Int16 maxScroll;
} RecipeFormContext;

static RecipeFormContext ctx;


/*********************************************************************
 * Internal functions
 *********************************************************************/

/***********************************************************************
 *
 * FUNCTION:     DrawRecipe
 *
 * DESCRIPTION:  Draws the recipe on screen, accounting for scrolling
 *
 * PARAMETERS:   Pointer to current form
 *
 * RETURNED:     content height (used in initial maxScroll calculation)
 *
 ***********************************************************************/
static UInt16 DrawRecipe(FormType *form)
{
    MemPtr *recipeP;
    RecipeRecord recipe;
    Char buf[80];
    Char namebuf[32];
    Char unitbuf[32];
	Char qtyBuf[16];
    Char *lineStart;
    Char *lineEnd;
    UInt16 lineBreakOffset;
    RectangleType r;
    UInt16 len;
    UInt16 y;
    UInt16 i;
    
    recipeP = MemHandleLock(ctx.recipe);
    recipe  = RecipeGetRecord(recipeP); 
    
    // Creates drawing window based on scrollbar
    FrmGetObjectBounds(form, FrmGetObjectIndex(form, ViewRecipeScrollbar), &r);
    r.extent.x  = r.topLeft.x;
    r.topLeft.x = 0;
    WinEraseRectangle(&r, 0);
    

    y = r.topLeft.y - ctx.scrollPos; // start at top minus scroll offset

    // Draw recipe name
    FntSetFont(boldFont);
    if (y >= r.topLeft.y && y < r.topLeft.y + r.extent.y) WinDrawChars(recipe.name, StrLen(recipe.name), 0, y);
    y += FntLineHeight() + 2;

    // Draw ingredients
    FntSetFont(stdFont);
    for (i = 0; i < recipeMaxIngredients && recipe.ingredientIDs[i] != 0; i++) {
        IngredientNameByID(namebuf, 32, recipe.ingredientIDs[i]);
        UnitNameByID(unitbuf, 32, recipe.ingredientUnits[i]);
        
		FormatQuantity(qtyBuf, recipe.ingredientCounts[i],
                       recipe.ingredientFracs[i],
                       recipe.ingredientDenoms[i]);
		if (qtyBuf[0] != '\0')
			// Allows unitless ingredients (e.g. pinch salt)
   			StrPrintF(buf, "%s %s %s", qtyBuf, unitbuf, namebuf);
		else
    		StrPrintF(buf, "%s %s", unitbuf, namebuf);
        
        lineBreakOffset = FntWordWrap(buf, r.extent.x);
        
        if (lineBreakOffset < StrLen(buf)) {
        	if (y >= r.topLeft.y && y < r.topLeft.y + r.extent.y) WinDrawChars(buf, lineBreakOffset, 0, y);
        	y += FntLineHeight();
        	if (y >= r.topLeft.y && y < r.topLeft.y + r.extent.y) WinDrawChars(((Char*)buf + lineBreakOffset), StrLen(((Char*)buf + lineBreakOffset)), 0, y);
        } else {
        	if (y >= r.topLeft.y && y < r.topLeft.y + r.extent.y) WinDrawChars(buf, StrLen(buf), 0, y);
        }
        y += FntLineHeight();
    }
    y += 4;

    // Draw steps
    lineStart = RecipeGetStepsPtr(recipeP);
    while (*lineStart) {
        lineEnd = lineStart;
        len = 0;
        
        lineBreakOffset = FntWordWrap(lineStart, r.extent.x);
        while (*lineEnd && len < lineBreakOffset && *lineEnd != '\n') { 
        	lineEnd++; len++; 
        }

        if (y >= r.topLeft.y && y < r.topLeft.y + r.extent.y) WinDrawChars(lineStart, len, 0, y);
        y += FntLineHeight();
        if (*lineEnd == '\n') lineEnd++;
        lineStart = lineEnd;
    }
    
    r.topLeft.y += r.extent.y;
    WinEraseRectangle(&r, 0);

    MemHandleUnlock(ctx.recipe);
    return y;
}

static Boolean ViewRecipeDoCommand(UInt16 command) {
	Boolean handled = false;
	MemPtr recipeP;
	RecipeRecord recipe;
	UInt16 i;

	switch(command) {
		case AddAll:
		    recipeP = MemHandleLock(ctx.recipe);
		    recipe  = RecipeGetRecord(recipeP); 
		    for (i = 0; i < recipe.numIngredients; i++) {
		    	AddIdToDatabase(gGroceryDB, recipe.ingredientIDs[i]);
		    }
		    MemHandleUnlock(ctx.recipe);
		    handled = true;
			break;
			
		case AddMissing:
		    recipeP = MemHandleLock(ctx.recipe);
		    recipe  = RecipeGetRecord(recipeP); 
		    for (i = 0; i < recipe.numIngredients; i++) {
		    	if (!EntryInDatabase(gPantryDB, recipe.ingredientIDs[i]))
			    	AddIdToDatabase(gGroceryDB, recipe.ingredientIDs[i]);
		    }
			MemHandleUnlock(ctx.recipe);
		    handled = true;
			break;
	}
	
	if (!handled)
		handled = MainMenuDoCommand(command);
	
	return handled;
}

/*********************************************************************
 * External functions
 *********************************************************************/

/***********************************************************************
 *
 * FUNCTION:     ViewRecipeHandleEvent
 *
 * DESCRIPTION:  Event handler for formViewRecipe
 *
 * PARAMETERS:   Event
 *
 * RETURNED:     handled boolean
 *
 ***********************************************************************/
Boolean ViewRecipeHandleEvent(EventPtr eventP) {
	FormPtr frmP = FrmGetActiveForm();
	Boolean handled = false;
	RectangleType r;
	Coord displayHeight;
	Coord contentHeight;

	switch (eventP->eType) {
		case frmOpenEvent:
			FrmDrawForm (frmP);
			contentHeight = DrawRecipe(frmP);
			FrmGetFormBounds(frmP, &r);
			displayHeight = r.extent.y;
			if (contentHeight > displayHeight) {
				ctx.maxScroll = contentHeight - displayHeight + FntLineHeight();
				//Leaves one extra line of blank space
			}
            SclSetScrollBar(FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, ViewRecipeScrollbar)),
                            0, 0, ctx.maxScroll, displayHeight-1);
			handled = true;
			break;
			
        case sclExitEvent:
            ctx.scrollPos = (eventP->data.sclExit.newValue);
            if (ctx.scrollPos < 0) ctx.scrollPos = 0;
            DrawRecipe(frmP); //Change to frmUpdateForm if more is needed on form updates
            return true;
            
        case frmUpdateEvent:
            DrawRecipe(frmP);
            return true;
            
		case menuEvent:
			return ViewRecipeDoCommand(eventP->data.menu.itemID);
			
		case keyDownEvent:
			FrmGetFormBounds(frmP, &r);
			displayHeight = r.extent.y;
			if (eventP->data.keyDown.chr == pageUpChr) {
				ctx.scrollPos -= FntLineHeight();
				if (ctx.scrollPos < 0) ctx.scrollPos = 0;
			
			} else if (eventP->data.keyDown.chr == pageDownChr) {
				ctx.scrollPos += FntLineHeight();
				if (ctx.scrollPos > ctx.maxScroll) ctx.scrollPos = ctx.maxScroll;
			}
			SclSetScrollBar(FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, ViewRecipeScrollbar)),
                            ctx.scrollPos, 0, ctx.maxScroll, displayHeight-1);
			DrawRecipe(frmP);
			
		default:		
			break;
	}
	return handled;
}

/***********************************************************************
 *
 * FUNCTION:     OpenRecipeForm
 *
 * DESCRIPTION:  Opens and initializes ViewRecipe form to a given recipe
 *
 * PARAMETERS:   Index of recipeDB entry
 *
 * RETURNED:     nothing
 *
 ***********************************************************************/
void OpenRecipeForm(UInt16 selection) {
	Err err;

    ctx.recipe    = DmQueryRecord(gRecipeDB, selection);
    if (ctx.recipe) {
	    ctx.scrollPos = 0;
	    ctx.maxScroll = 0;
	    FrmGotoForm(formViewRecipe);
	} else {
		err = DmGetLastErr();
		displayError(err);
	}
}