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
 * External functions
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

UInt16 DrawRecipe(FormType *form)
{
    RectangleType r;
    UInt16 y;
    Char buf[64];
    Char namebuf[32];
    Char unitbuf[16];
    RecipeRecord *recipeP = MemHandleLock(ctx.recipe);
    UInt16 i;
    Char *lineStart;
    UInt16 lineBreakOffset;
    Char *lineEnd;
    UInt16 len;
    
    // Get drawing rectangle
    FrmGetObjectBounds(form, FrmGetObjectIndex(form, ViewRecipeScrollbar), &r);
    r.extent.x = r.topLeft.x;
    r.topLeft.x = 0;
    WinEraseRectangle(&r, 0);
    

    y = r.topLeft.y - ctx.scrollPos; // start at top minus scroll offset

    // Draw recipe name (bold)
    FntSetFont(boldFont);
    if (y >= r.topLeft.y && y < r.topLeft.y + r.extent.y) WinDrawChars(recipeP->name, StrLen(recipeP->name), 0, y);
    y += FntLineHeight() + 2;

    // Draw ingredients
    FntSetFont(stdFont);
    for (i = 0; i < recipeMaxIngredients && recipeP->ingredientIDs[i] != 0; i++) {
        // Format: "count unit ingredient"
        IngredientNameByID(recipeP->ingredientIDs[i], namebuf);
        UnitNameByID(recipeP->ingredientUnits[i], unitbuf);
        if (recipeP->ingredientCounts[i] == 0) {
        	StrPrintF(buf, "%s %s", 
                unitbuf,
                namebuf);
        }
        else {
        	StrPrintF(buf, "%u %s %s", 
                recipeP->ingredientCounts[i],
                unitbuf,
                namebuf);
        }
        if (y >= r.topLeft.y && y < r.topLeft.y + r.extent.y) WinDrawChars(buf, StrLen(buf), 0, y);
        y += FntLineHeight();
    }
    y += 4;

    // Draw steps (multi-line)
    lineStart = (Char*)recipeP + sizeof(RecipeRecord);
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
	Boolean handled = false;
	FormPtr frmP = FrmGetActiveForm();
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
            
		case menuEvent: //Likely change later
			return MainFormDoCommand(eventP->data.menu.itemID);
			
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
 * PARAMETERS:   MemHandle to a recipe database entry
 *
 * RETURNED:     nothing
 *
 ***********************************************************************/
void OpenRecipeForm(MemHandle recipe) {
    ctx.recipe = recipe;
    ctx.scrollPos = 0;
    ctx.maxScroll = 0;
    FrmGotoForm(formViewRecipe);
}