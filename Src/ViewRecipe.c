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
    Int16 contentHeight;   // total height of rendered content
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
 * RETURNED:     nothing
 *
 ***********************************************************************/

void DrawRecipe(FormType *form)
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
    WinDrawChars(recipeP->name, StrLen(recipeP->name), 0, y);
    y += FntLineHeight() + 2;

    // Draw ingredients
    FntSetFont(stdFont);
    for (i = 0; i < recipeMaxIngredients && recipeP->ingredientIDs[i] != 0; i++) {
        // Format: "count unit ingredient"
        IngredientNameByID(recipeP->ingredientIDs[i], namebuf);
        UnitNameByID(recipeP->ingredientUnits[i], unitbuf);
        StrPrintF(buf, "%u %s %s", 
                recipeP->ingredientCounts[i],
                unitbuf,
                namebuf);
        WinDrawChars(buf, StrLen(buf), 0, y);
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

        WinDrawChars(lineStart, len, 0, y);
        y += FntLineHeight();
        if (*lineEnd == '\n') lineEnd++;
        lineStart = lineEnd;
    }

    ctx.contentHeight = y; // total content height for scrollbar
    MemHandleUnlock(ctx.recipe);
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
	Int16 maxScroll = 0;

	switch (eventP->eType) {
		case frmOpenEvent:
			FrmDrawForm (frmP);
			DrawRecipe(frmP);
			FrmGetFormBounds(frmP, &r);
			displayHeight = r.extent.y;
			if (ctx.contentHeight > displayHeight) {
				maxScroll = ctx.contentHeight - displayHeight;
			}
            SclSetScrollBar(FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, ViewRecipeScrollbar)),
                            0, maxScroll, 0, displayHeight);
			handled = true;
			break;
			
        case sclRepeatEvent:
            ctx.scrollPos += eventP->data.sclRepeat.newValue - eventP->data.sclRepeat.value;
            if (ctx.scrollPos < 0) ctx.scrollPos = 0;
            if (ctx.scrollPos > maxScroll) ctx.scrollPos = maxScroll;
            FrmUpdateForm(formViewRecipe, 0);
            return true;
            
        case frmUpdateEvent:
            DrawRecipe(frmP);
            return true;
            
		case menuEvent: //Likely change later
			return MainFormDoCommand(eventP->data.menu.itemID);
			
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
    ctx.contentHeight = 0;
    FrmGotoForm(formViewRecipe);
}